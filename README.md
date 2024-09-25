# Echo Server
Simple & Customizable Echo TCP Server

# Overview
This TCP server implementations can handle predefined number of concurrent connections, each of them running in its own thread. Each connection handler looks for a new-line terminator (can be <CR>, <LF> or <CR><LF>) and sends the message for processing.
Processing of the messages is done in an external function, that can be easily replace if needed.
The current processing is checking for service command messages that can be one of these:
- stats - sends server and connection status, as the current connections count, count of the server processed messages, and count of the active connection messages
- quit - actively closes the current connection
- shutdown - closes all connections and shuts down the server

