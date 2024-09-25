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

1. The project is compiled under Linux OS with g++ compiler.
    the project can be built with `make`

2. For the unit testing it is used the Google C++ Unit Testing Framework.
    prerequirements:
    <pre>
        apt install cmake
    </pre>

    installation:
    <pre>
        apt install libgtest-dev
        cd /usr/src/gtest
        cmake .
        make
    </pre>

