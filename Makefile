CC = g++ -g


all: echo_server

echo_server: echo_server.cpp llist.h
	$(CC) echo_server.cpp -o echo_server
