CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude
CLIENT = client
SERVER = server
OBJS = fdio.o

.PHONEY: all clean memcheck

all: fdio.o client server

client:
	$(CC) $(CCFLAGS) -o $(CLIENT) $(SRC_DIR)/client.c $(OBJS)

server:
	$(CC) $(CCFLAGS) -o $(SERVER) $(SRC_DIR)/server.c 

fdio.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/fdio.c

memcheck: $(CLIENT)
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./$(CLIENT) -d localhost
	
clean:
	rm -f $(CLIENT) $(SERVER) $(OBJS)
