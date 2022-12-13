# Written by Nathan Hutchins for lab5
CC = gcc

all: client server

client: client.h
	gcc client.c -o client

server: server.h
	gcc server.c -o server

clean:
	rm -f client
	rm -f server
	rm -f *.o
	clear