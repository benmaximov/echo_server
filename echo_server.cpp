#include "tcp_server.h"
#include <stdlib.h>

#define ECHO_TCP_PORT   2121
//------------------------------------------------------------------------------------
//message processor for clent messages

void processMessage(Connection* conn, char *message, int message_len)
{
    if (!strcasecmp(message, "stats"))
    {
        conn->sendMessage("client count: %d\n", conn->server->getConnectionCount());
        conn->sendMessage("client messages: %d\n", conn->message_count);
        conn->sendMessage("server messages: %d\n", conn->server->getMessageCount());
    }
    else if (!strcasecmp(message, "close"))
    {
        conn->sendMessage("Goodbye\n");
        shutdown(conn->socket, SHUT_RDWR);
    }
    else if (!strcasecmp(message, "shutdown"))
    {
        conn->server->Stop();
    }
    else
    {
        conn->sendMessage("%s\n", message);
        // increase counters
        conn->message_count++;
        conn->server->incMessageCount();
    }
}

//------------------------------------------------------------------------------------
//main program

int main(int argc, char *argv[])
{
    TCPServer server;
    int port = ECHO_TCP_PORT; //default port number is TCP:2121

    //check args for overriding
    for (int i = 1; i < argc; i++)
    {
        if (!strncmp(argv[i], "-p", 2))
            port = atoi(argv[i] + 2);
        if (!strcmp(argv[i], "-d"))
            server.debug_printing = true;
    }

    //check if port number is valid
    if (port < 1 || port > 0xFFFF)
    {
        fprintf(stderr, "invalid port number\n");
        return 1;
    }

    server.ProcessMessagePtr = &processMessage;

    //activate server
    if (!server.SetupListening(port))
        return 1;

    if (!server.Start())
        return 1;

    printf("server started on port %d\n", port);

    // wait during server operation
    server.WaitServer();

    printf("finished\n");
    return 0;
}