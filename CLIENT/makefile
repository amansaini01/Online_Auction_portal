CC=gcc
CFLAGS=-Wall -g
OBJ= client.o

all: client

client.o: client.c
	$(CC) -c -o $@ $< $(CFLAGS)
	
client: client.o
	gcc -o $@ client.o $(CFLAGS)
