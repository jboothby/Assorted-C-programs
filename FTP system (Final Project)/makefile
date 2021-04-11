CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude

.PHONEY: all clean

all: client server

client: client.o
	$(CC) $(CCFLAGS) client.c

server: server.o
	$(CC) $(CCFLAGS) server.c

clean:
	rm -f ${OBJS} ${EXEC}
