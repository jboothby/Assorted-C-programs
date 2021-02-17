CC = gcc
CCFLAGS = -g -Wall -Iinclude
EXEC = readable
OBJS = assignment3.o

$(EXEC): $(OBJS)
	$(CC) $(CCFLAGS) -o $(EXEC) $(OBJS)

assigment3.o: assignment3.c
	$(CC) $(CCFLAGS) -c assignment3.c

run: $(EXEC) 	
	./$(EXEC)

clean:
	rm -f $(OBJS) $(EXEC)
