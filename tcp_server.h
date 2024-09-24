#pragma once

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
#define RECV_BUF_SIZE 1024
#define RECV_MESSAGE_SIZE 4096
#define POLL_TIMEOUT_MS 500

class TCPServer;

struct Connection
{
    char remote_addr[MAX_LENGTH_REMOTE_IP];
    int remote_port;

    int pos;
    int socket;
    int message_count;
    bool running;

    TCPServer* server;
    pthread_t client_thread = 0;

    static void* clientLoop(void*);
    void processMessage(char* message, int message_len);
    bool sendMessage(const char* format, ...);
};

class TCPServer
{
        friend struct Connection;

    private:

        Connection connections[MAX_ACTIVE_CONNECTIONS];
        LList<MAX_ACTIVE_CONNECTIONS> connections_list;
        int server_sock = -1;
        pthread_t server_thread = 0;
        int tcp_port;
        int bindAddr;
        int message_count = 0;

        bool makeSocket();
        bool setReuseAddr(bool reuse = true);
        bool bindToEP();
        bool listenOnSocket();

        static bool setNonBlockingMode(int& socket);
        static bool pollOnSocket(int socket, int timeout_ms);

        bool setupSocket();
        void closeSocket();
        inline bool isSocketClosed() { return server_sock == -1; }

        static void* serverLoop(void*);

        void acceptClient();

    public:
        bool running = false;

        bool reuse_address = true;
        int backlog = 10;
        bool closeOnMaxConnections = true;

        bool StartListening(int port, int addr = INADDR_ANY);
        bool StartAccepting();
        bool Stop();
};