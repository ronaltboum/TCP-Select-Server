CC = g++
CFLAGS = -std=c++11 -g -Wall -O2 -pedantic -pthread -g


default:
	make all
		
TestProg: message.o peer.o TcpServer.o TestProg.o
	$(CC) -o TestProg $^ -pthread
	
TestProg.o: TestProg.cpp
	$(CC) -c $(CFLAGS) $<	

TcpServer.o: TcpServer.cpp
	$(CC) -c $(CFLAGS) $<

peer.o: peer.c
	$(CC) -c $(CFLAGS) $<

message.o: message.c
	$(CC) -c $(CFLAGS) $<

#all: 
all: TestProg

clean:
	rm -f message.o peer.o TcpServer.o TestProg.o TestProg
