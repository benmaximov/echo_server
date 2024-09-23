#include "llist.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#define MAX_ACTIVE_CONNECTIONS 200
#define MAX_LENGTH_REMOTE_IP 200
#define RECV_BUF_SIZE 2048
#define SEND_BUF_SIZE 2048

class Connection
{
public:
    char remote_addr[MAX_LENGTH_REMOTE_IP];
    int remote_port;

    int pos;
    int socket;
    int message_count;
};

Connection connections[MAX_ACTIVE_CONNECTIONS];
LList<MAX_ACTIVE_CONNECTIONS> llist;

void *clientLoop(void *arg)
{
    Connection *conn = (Connection *)arg;

    int flags = fcntl(conn->socket, F_GETFL, 0);
    if (fcntl(conn->socket, F_SETFL, flags | O_NONBLOCK))
    {
        perror("can't set O_NONBLOCK");
        close(conn->socket);
        llist.RemoveAt(conn->pos);
    }

    pollfd pfd;
    pfd.fd = conn->socket;
    pfd.events = POLLIN;

    unsigned char send_buf[SEND_BUF_SIZE];
    int send_buf_len = 0;

    while (true)
    {
        if (poll(&pfd, 1, 500) == 1)
        {
            unsigned char recv_buf[RECV_BUF_SIZE];
            int recv_sz = recv(conn->socket, recv_buf, RECV_BUF_SIZE, MSG_NOSIGNAL);

            if (recv_sz == 0)
            {
                //disconnected
                close(conn->socket);
                llist.RemoveAt(conn->pos);
                printf("disconnected %s:%d\n", conn->remote_addr, conn->remote_port);
                break;
            }

            if (recv_sz < 0)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR)
                    continue;
                printf("disconnected %s:%d\n", conn->remote_addr, conn->remote_port);
                close(conn->socket);
                llist.RemoveAt(conn->pos);
                break;
            }

            for (int i = 0; i < recv_sz; i++)
            {
                send_buf[send_buf_len++] = recv_buf[i];

                if (recv_buf[i] == '\n')
                {
                    send_buf[send_buf_len - 1] = 0;
                    if (!strcasecmp((char*)send_buf, "stats"))
                    {
                        send_buf_len = sprintf((char*)send_buf, "connections: %d\nmessages: %d\n", llist.Count(), conn->message_count);
                        send(conn->socket, send_buf, send_buf_len, MSG_NOSIGNAL);
                    }
                    else
                    {
                        send(conn->socket, send_buf, send_buf_len, MSG_NOSIGNAL);
                        conn->message_count++;
                    }
                    send_buf_len = 0;
                }
            }

            //printf("thread %d send_buf_len= %d\n", conn->pos, send_buf_len);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1)
    {
        perror("can't create socket");
        return -1;
    }

    int optval = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        perror("Error setting SO_REUSEADDR");
        close(server_sock);
        return -1;
    }

    int port = 2121;

    sockaddr_in ls_addr;
    memset(&ls_addr, 0, sizeof(ls_addr));
    ls_addr.sin_family = AF_INET;
    ls_addr.sin_port = htons(port);
    ls_addr.sin_addr.s_addr = INADDR_ANY; // inet_addr("192.168.1.10")

    if (bind(server_sock, (sockaddr *)&ls_addr, sizeof(ls_addr)))
    {
        perror("can't bind the socket");
        close(server_sock);
        return -1;
    }

    int backlog = 10;
    if (listen(server_sock, backlog))
    {
        perror("can't listen");
        close(server_sock);
        return -1;
    }

    printf("Server started on port %d\n", port);

    while (true)
    {
        if (llist.Count() >= MAX_ACTIVE_CONNECTIONS)
        {
            printf("too many active connections\n");
            sleep(1);
            continue;
        }

        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_sock = accept(server_sock, (sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("can't accept client");
            usleep(100'000);
            continue;
        }

        int pos = llist.AddPos();
        Connection *conn = &connections[pos];
        conn->pos = pos;
        conn->socket = client_sock;
        conn->message_count = 0;

        inet_ntop(AF_INET, &client_addr.sin_addr, conn->remote_addr, MAX_LENGTH_REMOTE_IP);
        conn->remote_port = client_addr.sin_port;

        printf("accepted %s:%d\n", conn->remote_addr, conn->remote_port);

        pthread_t myThread;
        if (pthread_create(&myThread, NULL, clientLoop, conn))
        {
            perror("can't run a thread");
            close(client_sock);
        }
    }

    /*for (int i = 0; i < 10; i++)
    {
        int p = llist.AddPos();
        printf("%d\n", p);
    }

    llist.RemoveAt(4);
    llist.RemoveAt(2);
    llist.RemoveAt(0);
    llist.RemoveAt(6);
    llist.RemoveAt(8);

    int p = llist.Head();
    while (p != -1)
    {
        printf("N%d\n", p);
        p = llist.Next(p);
    }

    p = llist.Tail();
    while (p != -1)
    {
        printf("P%d\n", p);
        p = llist.Prev(p);
    }

    for (int i = 0; i < 20; i++)
        printf("%d: %d\n", i, llist.ValidItemAtPos(i));

    for (int i = 0; i < 10; i++)
    {
        int p = llist.AddPos();
        printf("%d\n", p);
    }*/
}