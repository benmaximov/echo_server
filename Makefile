CXX = g++
DEBUG = -g
GTEST_LIBS = -lgtest -lgtest_main

all: echo_server
testing: llist_testing tcp_server_testing

echo_server: echo_server.cpp tcp_server.cpp tcp_server_connection.cpp tcp_server.h llist_safe.h
	$(CXX) $(DEBUG) echo_server.cpp tcp_server.cpp tcp_server_connection.cpp -o $@

llist_testing: llist_testing.cpp llist_safe.h
	$(CXX) llist_testing.cpp -o $@ $(GTEST_LIBS)

tcp_server_testing: tcp_server_testing.cpp tcp_server.cpp tcp_server_connection.cpp tcp_server.h llist_safe.h
	$(CXX) tcp_server_testing.cpp tcp_server.cpp tcp_server_connection.cpp -o $@ $(GTEST_LIBS)
