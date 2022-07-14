CC=gcc
CFLAGS=-Wall -g
OBJ=server.o server_utilities.o

all: server

server.o: server.c
	$(CC) -c -o $@ $< $(CFLAGS)

server_utilities.o: server_utilities.c
	$(CC) -c -o $@ $< $(CFLAGS)	

server: server.o server_utilities.o
	$(CC) -pthread -o $@ server.o server_utilities.o $(CFLAGS)
