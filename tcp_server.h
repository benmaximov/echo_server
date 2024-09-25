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

//connection holder struct
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
    bool sendMessage(const char* format, ...);
    void closeAndWaitConnection();
    bool start();
};

//server holder class
class TCPServer
{
    private:

        Connection connections[MAX_ACTIVE_CONNECTIONS];
        LList<MAX_ACTIVE_CONNECTIONS> connections_list;
        int server_sock = -1;
        pthread_t server_thread = 0;
        int tcp_port;
        int bindAddr;
        int message_count = 0;

        bool makeSocket();
        bool setReuseAddr();
        bool bindToEP();
        bool listenOnSocket();

        static bool setNonBlockingMode(int& socket);
        static void* serverLoop(void*);

        bool setupSocket();
        void closeSocket();
        inline bool isSocketClosed() { return server_sock == -1; }

        void acceptClient();
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    private:
        //used from Connection struct

        friend struct Connection;
        void connectionComplete(Connection* conn);
        static bool pollOnSocket(int socket, int timeout_ms);

    public:
        //used from outside

        int getConnectionCount();
        int getMessageCount();
        void incMessageCount();


    public:

        void (*ProcessMessagePtr)(Connection* conn, char *, int);

        bool running = false;

        int backlog = 10;
        bool reuse_address = true;
        bool closeOnMaxConnections = true;

        bool SetupListening(int port, int addr = INADDR_ANY);
        bool Start();
        bool Stop();
        void WaitServer();
};