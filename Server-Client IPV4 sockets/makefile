CC = gcc
SRC_DIR = src
CCFLAGS = -g -Iinclude
LFLAGS = -lpthread
EXEC = assignment8
OBJS = assignment8.o

.PHONEY: clean server client memcheck

${EXEC}: ${OBJS}
	${CC} ${CCFLAGS} -o ${EXEC} ${OBJS} ${LFLAGS}

assignment8.o: ${SRC_DIR}/assignment8.c
	${CC} ${CCFLAGS} -c ${SRC_DIR}/assignment8.c ${LFLAGS}

server: ${EXEC}
	./${EXEC} server

client: ${EXEC}
	./${EXEC} client localhost

memcheck: ${EXEC}
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./${EXEC}

clean:
	rm -f ${OBJS} ${EXEC}
