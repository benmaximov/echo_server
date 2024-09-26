#include <gtest/gtest.h>
#include "tcp_server.h"

#define TEST_TCP_PORT 2122

TCPServer server;

void simpleEchoMessage(Connection *conn, char *message, int message_len)
{
    conn->sendMessage("%s\n", message);
}

TEST(TCPServer, BasicEchoTest)
{
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket timeout
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // connect
    ASSERT_EQ(connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)), 0);

    const char *message = "test message\n";
    int msg_len = strlen(message);

    // send
    ASSERT_EQ(send(sockfd, (unsigned char *)message, msg_len, 0), msg_len);

    // recv
    unsigned char recv_buf[200];
    ASSERT_EQ(recv(sockfd, recv_buf, sizeof(recv_buf), 0), msg_len);

    recv_buf[msg_len] = 0;

    EXPECT_STREQ((char *)recv_buf, message);
    close(sockfd);

    server.Stop();
    server.WaitServer();
}

TEST(TCPServer, EmptyMessageTest)
{
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket timeout
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // connect
    ASSERT_EQ(connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)), 0);

    const char *message = "\n";
    int msg_len = strlen(message);

    // send
    ASSERT_EQ(send(sockfd, (unsigned char *)message, msg_len, 0), msg_len);

    // recv
    unsigned char recv_buf[200];
    ASSERT_EQ(recv(sockfd, recv_buf, sizeof(recv_buf), 0), msg_len);

    recv_buf[msg_len] = 0;

    ASSERT_STREQ((char *)recv_buf, message);
    close(sockfd);

    server.Stop();
    server.WaitServer();
}

void setNonBlockingMode(int &socket)
{
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

bool poll(int socket, int poll_type, int timeout_ms)
{
    pollfd pfd;
    pfd.fd = socket;
    pfd.events = poll_type;
    return poll(&pfd, 1, timeout_ms) == 1;
}

TEST(TCPServer, ConcurrentConnections)
{
    //server.debug_printing = true;
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    // set socket timeout
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    int sockfd[MAX_ACTIVE_CONNECTIONS];
    // connect all
    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
    {
        sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        setNonBlockingMode(sockfd[i]);
        ASSERT_EQ(connect(sockfd[i], (sockaddr *)&server_addr, sizeof(server_addr)), -1);
        ASSERT_EQ(errno, EINPROGRESS);
    }
    // wait connections to complete
    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
        ASSERT_TRUE(poll(sockfd[i], POLLOUT, 5000));

    char message[200];

    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
    {
        sprintf(message, "test message #%d\n", i);
        ASSERT_EQ(send(sockfd[i], (unsigned char *)message, strlen(message), 0), strlen(message));
    }

    unsigned char recv_buf[200];

    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
    {
        sprintf(message, "test message #%d\n", i);
        int msg_len = strlen(message);
        int recv_bytes = 0;

        while (true)
        {
            int bytes = recv(sockfd[i], recv_buf + recv_bytes, sizeof(recv_buf) - recv_bytes, 0);
            if (bytes < 0)
            {
                ASSERT_EQ(errno, EWOULDBLOCK);
                usleep(100'000); //100 ms
                continue;
            }
            ASSERT_GT(bytes, 0);
            recv_bytes += bytes;
            ASSERT_LE(recv_bytes, msg_len);
            if (recv_bytes == msg_len)
            {
                recv_buf[recv_bytes] = 0;
                ASSERT_STREQ((char*)recv_buf, message);
                break;
            }
        }
    }

    usleep(2'000'000); //wait 2 sec

    //ensure no more data is present
    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
    {
        ASSERT_EQ(recv(sockfd[i], recv_buf, sizeof(recv_buf), 0), -1);
        ASSERT_EQ(errno, EWOULDBLOCK);
    }

    server.Stop();
    server.WaitServer();

    //ensure all connections are closed
    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS; i++)
        ASSERT_EQ(recv(sockfd[i], recv_buf, sizeof(recv_buf), 0), 0);
}

TEST(TCPServer, LargeMessageTest)
{
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket timeout
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // connect
    ASSERT_EQ(connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)), 0);

    char message[RECV_MESSAGE_SIZE + 1];

    //build message
    char ch = 'A';
    for (int i = 0; i < RECV_MESSAGE_SIZE - 1; i++)
    {
        message[i] = ch;
        ch = ch == 'Z' ? 'A' : (ch + 1);
    }
    message[RECV_MESSAGE_SIZE - 1] = '\n';
    message[RECV_MESSAGE_SIZE] = 0;

    // send
    ASSERT_EQ(send(sockfd, (unsigned char *)message, RECV_MESSAGE_SIZE, 0), RECV_MESSAGE_SIZE);

    // recv
    unsigned char recv_buf[RECV_MESSAGE_SIZE + 100];
    ASSERT_EQ(recv(sockfd, recv_buf, sizeof(recv_buf), 0), RECV_MESSAGE_SIZE);

    recv_buf[RECV_MESSAGE_SIZE] = 0;

    EXPECT_STREQ((char *)recv_buf, message);
    close(sockfd);

    server.Stop();
    server.WaitServer();
}

TEST(TCPServer, MessageSizeOverflow)
{
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket timeout
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // connect
    ASSERT_EQ(connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)), 0);

    char message[RECV_MESSAGE_SIZE + 1];

    //build message
    char ch = 'A';
    for (int i = 0; i < RECV_MESSAGE_SIZE; i++)
    {
        message[i] = ch;
        ch = ch == 'Z' ? 'A' : (ch + 1);
    }
    message[RECV_MESSAGE_SIZE] = 0;

    // send
    ASSERT_EQ(send(sockfd, (unsigned char *)message, RECV_MESSAGE_SIZE, 0), RECV_MESSAGE_SIZE);

    // send more
    const char* addmsg = "additional data\n";
    ASSERT_EQ(send(sockfd, addmsg, strlen(addmsg), 0), strlen(addmsg));


    // recv
    unsigned char recv_buf[RECV_MESSAGE_SIZE + 100];
    ASSERT_EQ(recv(sockfd, recv_buf, sizeof(recv_buf), 0), RECV_MESSAGE_SIZE);

    recv_buf[RECV_MESSAGE_SIZE] = 0;

    message[RECV_MESSAGE_SIZE - 1] = '\n';
    EXPECT_STREQ((char *)recv_buf, message);

    //no more data
    usleep(100'000);
    ASSERT_FALSE(poll(sockfd, POLLIN, 500));

    close(sockfd);

    server.Stop();
    server.WaitServer();
}

TEST(TCPServer, ConnectionsOpenClose)
{
    server.ProcessMessagePtr = &simpleEchoMessage;
    server.SetupListening(TEST_TCP_PORT);
    server.Start();

    usleep(100'000);

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TEST_TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    for (int i = 0; i < MAX_ACTIVE_CONNECTIONS + 10; i++)
    {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_EQ(connect(sockfd, (sockaddr *)&server_addr, sizeof(server_addr)), 0);
        usleep(50'000);
        ASSERT_EQ(server.getConnectionCount(), 1);
        ASSERT_EQ(close(sockfd), 0);
    }

    server.Stop();
    server.WaitServer();
}