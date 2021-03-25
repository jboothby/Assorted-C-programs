CC = gcc
SRC_DIR = src
CCFLAGS = -g -Wall -Iinclude
LFLAGS = -lpthread
EXEC = sort
OBJS = sort.o getWord.o assignment7.o

.PHONEY: clean run memcheck

${EXEC}: ${OBJS}
	${CC} ${CCFLAGS} -o ${EXEC} ${OBJS} ${LFLAGS}

assignment7.o: ${SRC_DIR}/assignment7.c
	${CC} ${CCFLAGS} -c ${SRC_DIR}/assignment7.c ${LFLAGS}

getWord.o: ${SRC_DIR}/getWord.c
	${CC} ${CCFLAGS} -c ${SRC_DIR}/getWord.c

sort.o: ${SRC_DIR}/sort.c
	${CC} ${CCFLAGS} -c ${SRC_DIR}/sort.c

run: ${EXEC}
	./${EXEC} ./dataset/webster_random

memcheck: ${EXEC}
	valgrind --tool=memcheck --track-origins=yes --leak-check=full ./${EXEC} ./dataset/webster_random

clean:
	rm -f ${OBJS} ${EXEC}
