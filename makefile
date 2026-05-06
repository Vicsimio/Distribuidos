CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

all: server

server: server.o
	$(CC) $(CFLAGS) -o server server.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -f *.o server