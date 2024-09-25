#include "tcp_server.h"

bool TCPServer::makeSocket()
{
    if (server_sock != -1)
        close(server_sock);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("can't create socket");
        return false;
    }

    return true;
}

bool TCPServer::setReuseAddr()
{
    if (server_sock == -1)
        return false;

    int optval = reuse_address ? 1 : 0;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        perror("Error setting SO_REUSEADDR");
        close(server_sock);
        server_sock = -1;
        return false;
    }

    return true;
}

bool TCPServer::bindToEP()
{
    if (server_sock == -1)
        return false;

    sockaddr_in ls_addr;
    memset(&ls_addr, 0, sizeof(ls_addr));
    ls_addr.sin_family = AF_INET;
    ls_addr.sin_port = htons(tcp_port);
    ls_addr.sin_addr.s_addr = bindAddr;

    if (bind(server_sock, (sockaddr *)&ls_addr, sizeof(ls_addr)))
    {
        perror("can't bind the socket");
        close(server_sock);
        server_sock = -1;
        return false;
    }

    return true;
}

bool TCPServer::listenOnSocket()
{
    if (server_sock == -1)
        return false;

    if (listen(server_sock, backlog))
    {
        perror("can't listen");
        close(server_sock);
        server_sock = -1;
        return false;
    }

    return true;
}

bool TCPServer::setNonBlockingMode(int &socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0)
    {
        perror("can't get socket flags");
        close(socket);
        socket = -1;
        return false;
    }
    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK))
    {
        perror("can't set socket flag O_NONBLOCK");
        close(socket);
        socket = -1;
        return false;
    }
    return true;
}

bool TCPServer::pollOnSocket(int socket, int timeout_ms)
{
    pollfd pfd;
    pfd.fd = socket;
    pfd.events = POLLIN;
    return poll(&pfd, 1, timeout_ms) == 1;
}

bool TCPServer::setupSocket()
{
    if (!makeSocket())
        return false;

    if (!setNonBlockingMode(server_sock))
        return false;

    if (!setReuseAddr())
        return false;

    if (!bindToEP())
        return false;

    if (!listenOnSocket())
        return false;

    return true;
}

bool TCPServer::SetupListening(int port, int addr)
{
    bindAddr = addr;
    tcp_port = port;

    return setupSocket();
}

void TCPServer::closeSocket()
{
    if (server_sock == -1)
        return;

    close(server_sock);
    server_sock = -1;
}

void *TCPServer::serverLoop(void *param)
{
    auto server = (TCPServer *)param;
    while (server->running)
    {
        if (server->connections_list.Count() >= MAX_ACTIVE_CONNECTIONS)
        {
            if (!server->isSocketClosed() && server->closeOnMaxConnections)
            {
                fprintf(stderr, "too many active connections\n");
                server->closeSocket();
            }

            sleep(1);
            continue;
        }

        if (server->isSocketClosed())
            if (!server->setupSocket())
            {
                fprintf(stderr, "can't setup the listening socket\n");
                server->running = false;
                return NULL;
            }

        if (!pollOnSocket(server->server_sock, POLL_TIMEOUT_MS))
            continue;

        server->acceptClient();
    }

    //closing the listening socket
    server->closeSocket();

    //close all connections and wait their threads
    int pos = server->connections_list.Head();
    while (pos != -1)
    {
        Connection *conn = &server->connections[pos];
        conn->closeAndWaitConnection();
        pos = server->connections_list.Head();
    }

    return NULL;
}

void TCPServer::connectionComplete(Connection *conn)
{
    connections_list.RemoveAt(conn->pos);
    conn->pos = -1;
}

void TCPServer::acceptClient()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(server_sock, (sockaddr *)&client_addr, &client_len);
    if (client_socket < 0)
    {
        perror("can't accept client");
        usleep(100'000);
        return;
    }

    // initialize client object
    int pos = connections_list.AddPos();
    Connection *conn = &connections[pos];
    conn->pos = pos;
    conn->socket = client_socket;
    conn->server = this;
    conn->message_count = 0;

    // get remote address and port
    inet_ntop(AF_INET, &client_addr.sin_addr, conn->remote_addr, MAX_LENGTH_REMOTE_IP);
    conn->remote_port = client_addr.sin_port;

    // set non-blocking mode
    if (!setNonBlockingMode(conn->socket))
    {
        perror("can't set O_NONBLOCK to client socket");
        close(conn->socket);
        connections_list.RemoveAt(conn->pos);
        usleep(100'000);
        return;
    }

    // start the client thread
    if (!conn->start())
    {
        close(client_socket);
        connections_list.RemoveAt(pos);
        usleep(100'000);
        return;
    }

    printf("%d] client accepted %s:%d\n", pos, conn->remote_addr, conn->remote_port);
}

bool TCPServer::Start()
{
    if (running)
        return true;

    running = true;
    message_count = 0;

    if (pthread_create(&server_thread, NULL, serverLoop, this))
    {
        perror("can't run a thread");
        running = false;
        return false;
    }

    return true;
}

bool TCPServer::Stop()
{
    if (!running)
        return true;

    running = false;
    return true;
}

void TCPServer::WaitServer()
{
    if (!running)
        return;

    void *retVal;
    pthread_join(server_thread, &retVal);
}

int TCPServer::getConnectionCount()
{
    return connections_list.Count();
}

int TCPServer::getMessageCount()
{
    return message_count;
}

void TCPServer::incMessageCount()
{
    lock(message_count_lock)
        message_count++;
}
