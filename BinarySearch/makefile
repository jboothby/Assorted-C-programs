CC = gcc
CCFLAGS = -g -Wall -Iinclude
EXEC = okmain
OBJS = ok.o okmain.o

$(EXEC): $(OBJS)
	$(CC) $(CCFLAGS) -o $(EXEC) $(OBJS)

ok.o: ok.c
	$(CC) $(CCFLAGS) -c ok.c
okmain.o: okmain.c
	$(CC) $(CCFLAGS) -c okmain.c

run: $(EXEC) 	
	./$(EXEC)
