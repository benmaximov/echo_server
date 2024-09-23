#include "llist.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

class Connection
{
public:
    int pos;
    int socket;
    pthread_t thread;
};

static void *clientLoop(void *arg)
{
    while (true)
    {
        printf("thread running\n");
        sleep(1);
    }
}

int main(int argc, char *argv[])
{

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("can't create socket");
        return -1;
    }

    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        perror("Error setting SO_REUSEADDR");
        close(sock);
        return -1;
    }

    int port = 2121;

    sockaddr_in ls_addr;
    memset(&ls_addr, 0, sizeof(ls_addr));
    ls_addr.sin_family = AF_INET;
    ls_addr.sin_port = htons(port);
    ls_addr.sin_addr.s_addr = INADDR_ANY; // inet_addr("192.168.1.10")

    if (bind(sock, (sockaddr *)&ls_addr, sizeof(ls_addr)))
    {
        perror("can't bind the socket");
        close(sock);
        return -1;
    }

    int backlog = 1;
    if (listen(sock, backlog))
    {
        perror("can't listen");
        close(sock);
        return -1;
    }

    printf("Server started on port %d\n", port);
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_sock = accept(sock, (sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("can't accept client");
        }

        char client_ip[200];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 200);

        printf("accepted %s\n", client_ip);

        pthread_t myThread;
        if (pthread_create(&myThread, NULL, clientLoop, NULL))
        {
            perror("can't run a thread");
        }
    }

    LList<100> llist;

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