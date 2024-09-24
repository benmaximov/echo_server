#include "tcp_server.h"
#include <stdarg.h>
#include <string.h>

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

bool TCPServer::setReuseAddr(bool reuse)
{
    if (server_sock == -1)
        return false;

    int optval = reuse ? 1 : 0;
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

    if (!setReuseAddr(reuse_address))
        return false;

    if (!bindToEP())
        return false;

    if (!listenOnSocket())
        return false;

    return true;
}

bool TCPServer::StartListening(int port, int addr)
{
    bindAddr = addr;
    tcp_port = port;
    message_count = 0;

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

    return NULL;
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
    conn->running = true;

    // get remote address and port
    inet_ntop(AF_INET, &client_addr.sin_addr, conn->remote_addr, MAX_LENGTH_REMOTE_IP);
    conn->remote_port = client_addr.sin_port;

    // set non-blocking mode
    if (!setNonBlockingMode(conn->socket))
    {
        perror("can't set O_NONBLOCK to client socket");
        close(conn->socket);
        connections_list.RemoveAt(conn->pos);
        return;
    }

    // start the client thread
    if (pthread_create(&conn->client_thread, NULL, Connection::clientLoop, conn))
    {
        perror("can't run client thread");
        connections_list.RemoveAt(pos);
        close(client_socket);
        usleep(100'000);
        return;
    }

    printf("%d] client accepted %s:%d\n", pos, conn->remote_addr, conn->remote_port);
}

bool TCPServer::StartAccepting()
{
    if (running)
        return true;

    running = true;

    if (pthread_create(&server_thread, NULL, serverLoop, this))
    {
        perror("can't run a thread");
        return false;
    }

    return true;
}

bool TCPServer::Stop()
{
    if (!running)
        return true;

    running = false;

    void *retVal;
    pthread_join(server_thread, &retVal);
    return true;
}

void *Connection::clientLoop(void *param)
{
    Connection *conn = (Connection *)param;
    unsigned char message[RECV_MESSAGE_SIZE];
    int message_len = 0;

    while (conn->running)
    {
        if (!TCPServer::pollOnSocket(conn->socket, POLL_TIMEOUT_MS))
            continue;

        unsigned char recv_buf[RECV_BUF_SIZE];
        int recv_sz = recv(conn->socket, recv_buf, RECV_BUF_SIZE, MSG_NOSIGNAL);

        if (recv_sz == 0)
        {
            // disconnected
            close(conn->socket);
            conn->server->connections_list.RemoveAt(conn->pos);
            printf("%d] disconnected %s:%d\n", conn->pos, conn->remote_addr, conn->remote_port);
            conn->running = false;
            break;
        }

        if (recv_sz < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
                continue;
            perror("socket receive");
            close(conn->socket);
            conn->server->connections_list.RemoveAt(conn->pos);
            conn->running = false;
            break;
        }

        for (int i = 0; i < recv_sz; i++)
        {
            if (recv_buf[i] == '\r' || recv_buf[i] == '\n')
            {
                message[message_len] = 0;
                printf("%d> %s\n", conn->pos, message);
                conn->processMessage((char*)message, message_len);
                message_len = 0;
            }
            else if (message_len < RECV_MESSAGE_SIZE - 1)
                message[message_len++] = recv_buf[i];
        }
    }

    return NULL;
}

void Connection::processMessage(char *message, int message_len)
{
    if (!strcasecmp(message, "stats"))
    {
        sendMessage("client count: %d\n", server->connections_list.Count());
        sendMessage("client messages: %d\n", message_count);
        sendMessage("server messages: %d\n", server->message_count);
    }
    else if (!strcasecmp(message, "close"))
    {
        sendMessage("Goodbye\n", message_count);
        shutdown(socket, SHUT_RDWR);
    }
    else if (!strcasecmp(message, "shutdown"))
    {
        server->Stop();
    }
    else
    {
        message_count++;
        server->message_count++;
        sendMessage("%s\n", message);
    }
}

bool Connection::sendMessage(const char *format, ...)
{
    char send_buffer[RECV_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
    int length = vsnprintf(send_buffer, RECV_MESSAGE_SIZE, format, args);
    va_end(args);
    return send(socket, send_buffer, length, MSG_NOSIGNAL) > 0;
}
