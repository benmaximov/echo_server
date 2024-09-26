# Echo Server
Simple & Customizable Echo TCP Server

# Overview
This TCP server implementations can handle a predefined number of concurrent connections (ex 200), each of them running in its own thread. Each connection handler asynchronously looks for a new-line terminator (can be <CR>, <LF> or <CR><LF>) in the incoming data in order to separate different messages.
Each message is sent for processing in an external function, that can easily be replaced if needed.

The current processing is checking for predefined service command messages that can be any of the following:
- stats - sends server and connection status, which include the active connections count, the counts of the processed messages on the current connection and in the server
- quit - actively closes the current connection
- shutdown - closes all connections and shuts down the server
- any other message - is echoed back to the client with line termination <LF>

The server internally keeps track of each connection and the number of processed messages in each connection and in the whole server.
There is an option to close the listening socket when the maximum connection count is reached in order to prevent overwhelming the server with additional connection requests. When the number of active connections drops below the maximum limit, the listening socket is reopened to accept new incoming connections.

This design ensures that the server remains responsive and can easily adapt to new requirements by modifying the message processing logic as needed, while maintaining efficient management of resources and connections.

# Setup instructions

1. The project is compiled under x86_64 Linux with `g++` compiler
    the project can be built with `make`

    compiling and running:
    <pre>
        make
        ./echo_server [-p&lt;tcp_port&gt;] [-d]</pre>

    the default TCP port is 2121
    - -p option is for setting another TCP port
    - -d option is for printing debug info

2. For unit testing, it is used [Google C++ Unit Testing Framework](https://google.github.io/googletest/).

    installation:
    <pre>
        apt install cmake
        apt install libgtest-dev
        cd /usr/src/gtest
        cmake .
        make</pre>

    compiling and running the unit tests:
    <pre>
        make testing
        ./tcp_server_testing</pre>

# Summary of design decisions

- multi-threading - chosen because of the requirement to handle multiple simultaneous connections. Benefits - code is easier to read and modify, the software is more responsive. Drawbacks - some common execution contexts has to be isolated with mutex locks.
Other option could be using select/epoll on mutiple sockets.
- double linked-list for the connection pool - used to keep track of the connection resources and active connections count. Fast `add` and `remove` times of O(1). Easier to get connection id. Other option could be using a hashset on the socket ids or remote endpoints.
- non-blocking sockets - offering more control and responsiveness when forcefully closing connections. Other option could be using timeouts on  blocking sockets
- message size limit - there are several options when no "new-line" arrives in the designated buffer:
    - send to the client the current chunk, and start over with empty buffer
        - pros: no data is lost, the buffer can be smaller
        - cons: not quite fulfilling the task, as "new-line" may never arrive
    - extend the buffer and continue to record the data
        - pros: it may finally get the "new-line" and do the work correctly
        - cons: additional memory allocation could lead to performance issues, and even not guarantee that the message will be correctly terminated
    - skip the bytes if the buffer overflows (currently the chosen option)
        - pros: buffer with predefined size, statically allocated is better for the performance. Easier data manipulation
        - cons: will trim the longer messages
- external message processing function - can be easily replaced to change the server's function or add/modify additional service commands
- SO_REUSEADDR option for the listening socket allows a quick restart of the app in the development and testing scenarios
- error handling - potentially can lead to losing the current connection or server start failure

# Testing approach

- testing for the double-linked list class
    - adding and removing items, maintaing the correct structure of the list
    - thread-safety for adding and removing items
    - enumeration is done in a single thread, so no thread-safety tests are implemented for it
- testing the tcp_server class
    - basic echo test - simple test with one connection to check the basic funcionallity
    - empty message test - checking if empty messages are echoed correctly
    - concurrent connections test - simultaneously activating the maximum number of allowed connections and test them with a message
    - large message test - testing the server with the maximum allowed message length
    - message size overflow test - testing the server with longer size than the dedicated buffer
    - connections open&close test - opening and closing connections with larger count that the maximum allowed connections, keeping single currently open connection

