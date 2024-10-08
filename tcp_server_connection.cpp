#include "tcp_server.h"
#include <stdarg.h>

// client thread
void *Connection::clientLoop(void *param)
{
    Connection *conn = (Connection *)param;
    unsigned char message[RECV_MESSAGE_SIZE];
    int message_len = 0;
    char last_term = '\0';

    while (conn->running)
    {
        unsigned char recv_buf[RECV_BUF_SIZE];
        int recv_sz = recv(conn->socket, recv_buf, RECV_BUF_SIZE, MSG_NOSIGNAL);

        if (recv_sz == 0)
        {
            // disconnected
            if (conn->server->debug_printing)
                printf("%d] disconnected %s:%d\n", conn->pos, conn->remote_addr, conn->remote_port);

            close(conn->socket);
            break;
        }

        if (recv_sz < 0)
        {
            perror("socket receive");
            close(conn->socket);
            break;
        }

        for (int i = 0; i < recv_sz; i++)
        {
            if (recv_buf[i] == '\r' || recv_buf[i] == '\n')
            {
                //handle windows' line-endings
                if (last_term == '\r' && recv_buf[i] == '\n')
                {
                    last_term = '\n';
                    continue;
                }

                // null-terminate the recieved message for easier processing
                message[message_len] = 0;

                if (conn->server->debug_printing)
                    printf("%d> %s\n", conn->pos, message);

                // process the message with the external proc
                if (conn->server->ProcessMessagePtr)
                    conn->server->ProcessMessagePtr(conn, (char *)message, message_len);

                // reset the message counter
                message_len = 0;
            }
            else if (message_len < RECV_MESSAGE_SIZE - 1)
                message[message_len++] = recv_buf[i];

            last_term = recv_buf[i];
        }
    }

    conn->server->connectionComplete(conn);
    conn->running = false;

    return NULL;
}

// send messages with va_args
bool Connection::sendMessage(const char *format, ...)
{
    char send_buffer[RECV_MESSAGE_SIZE + 1];

    va_list args;
    va_start(args, format);
    int length = vsnprintf(send_buffer, RECV_MESSAGE_SIZE + 1, format, args);
    va_end(args);

    if (send(socket, send_buffer, length, MSG_NOSIGNAL) <= 0)
        return false;

    return true;
}

// start the connection thread
bool Connection::start()
{
    if (running)
        return false;
    running = true;

    if (pthread_create(&client_thread, NULL, Connection::clientLoop, this))
    {
        perror("can't run client thread");
        return false;
    }

    return true;
}

// close the connection and wait its thread
void Connection::closeAndWaitConnection()
{
    if (!running)
        return;

    if (server->debug_printing)
        printf("%d] closing...\n", pos);

    shutdown(socket, SHUT_RDWR);
    void *retVal;
    pthread_join(client_thread, &retVal);
}
