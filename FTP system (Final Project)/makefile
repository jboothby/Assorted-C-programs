CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude
CLIENT = client
SERVER = server
SERVEROBJS = serverHandler.o
COMMONOBJS = fdio.o

.PHONEY: clean memcheck

all: client server

client: $(COMMONOBJS)
	$(CC) $(CCFLAGS) -o $(CLIENT) $(SRC_DIR)/client.c $(COMMONOBJS)

server:	$(COMMONOBJS) $(SERVEROBJS)
	$(CC) $(CCFLAGS) -o $(SERVER) $(SRC_DIR)/server.c $(COMMONOBJS) $(SERVEROBJS)

fdio.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/fdio.c

serverHandler.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/serverHandler.c

memcheck: $(CLIENT)
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./$(CLIENT) -d localhost
	
clean:
	rm -f $(CLIENT) $(SERVER) $(COMMONOBJS) $(SERVEROBJS)
