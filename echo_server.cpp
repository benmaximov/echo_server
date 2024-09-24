#include "tcp_server.h"
#include <stdlib.h>

int main(int argc, char *argv[])
{
    TCPServer server;
    int port = 2121; //default port number is TCP:2121

    //check args for overriding
    for (int i = 1; i < argc; i++)
        if (!strncmp(argv[i], "-p", 2))
            port = atoi(argv[i] + 2);

    //check if port number is valid
    if (port < 1 || port > 0xFFFF)
    {
        fprintf(stderr, "invalid port number\n");
        return 1;
    }

    //activate server
    if (!server.StartListening(port))
        return 1;

    printf("server started on port %d\n", port);
    server.StartAccepting();

    // wait during server operation
    while (server.running)
        sleep(1);

    printf("finished\n");
    return 0;
}