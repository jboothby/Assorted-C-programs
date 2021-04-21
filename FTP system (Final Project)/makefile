CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude
CLIENT = mftp
SERVER = mftpserve
SERVEROBJS = mftpserveHandler.o
CLIENTOBJS = mftpHandler.o
COMMONOBJS = fdio.o

.PHONEY: clean memcheck

all: mftp mftpserve

mftp: $(COMMONOBJS) $(CLIENTOBJS)
	$(CC) $(CCFLAGS) -o $(CLIENT) $(SRC_DIR)/mftp.c $(COMMONOBJS) $(CLIENTOBJS)

mftpserve:	$(COMMONOBJS) $(SERVEROBJS)
	$(CC) $(CCFLAGS) -o $(SERVER) $(SRC_DIR)/mftpserve.c $(COMMONOBJS) $(SERVEROBJS)

fdio.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/fdio.c

mftpserveHandler.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/mftpserveHandler.c

mftpHandler.o:
	$(CC) $(CCFLAGS) -c $(SRC_DIR)/mftpHandler.c

memcheckClient: $(CLIENT)
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./$(CLIENT) -d localhost

memcheckServer: $(CLIENT)
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./$(SERVER) -d
	
clean:
	rm -f $(CLIENT) $(SERVER) $(COMMONOBJS) $(SERVEROBJS) $(CLIENTOBJS)
