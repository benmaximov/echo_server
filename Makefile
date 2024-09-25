CXX = g++ -g -lpthread
GTEST_LIBS = -lgtest -lgtest_main -lpthread

all: echo_server

echo_server: echo_server.cpp tcp_server.cpp tcp_server_connection.cpp tcp_server.h llist.h
	$(CXX) echo_server.cpp tcp_server.cpp tcp_server_connection.cpp -o $@

test: tests.cpp
	$(CXX) tests.cpp -o $@ $(GTEST_LIBS)
