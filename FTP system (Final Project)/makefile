CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude
CLIENT = client
SERVER = server

.PHONEY: all clean

all: client server

client:
	$(CC) $(CCFLAGS) -o $(CLIENT) $(SRC_DIR)/client.c

server:
	$(CC) $(CCFLAGS) -o $(SERVER) $(SRC_DIR)/server.c 

clean:
	rm -f $(CLIENT) $(SERVER)
