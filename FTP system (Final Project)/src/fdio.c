/* Handle reading and writing from a file descriptor */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* Writes to fd */
void writeToFd(char* string, int fd){    // Write and read to server

        // Holds amount written
        ssize_t actualWrite;

        // Debug output
        printf("Writing <");
        for( int i = 0; i <= strlen(string); i++){
            printf("(%d)", string[i]);
        }
        printf("> to fd: %d\n", fd);
        

        // Write string to server
        actualWrite = write(fd, string, strlen(string));
        if( actualWrite < 0 ){
            perror("write");
            exit(-1);
        }


}

/* Read a newline terminated string from the fd string */
/* Prompt user for command if fd is stdin */
char* readFromFd(int fd){

    int count = 0;          // current position in buffer
    ssize_t actual;         // number read from read
    char* fdString;         // buffer holds input string
    char temp[1];           // holds current character
    
    // Prompt user for input if fd is stdin
    if( fd == 0){
        actual = write(1, "Enter a command: ", strlen("Enter a command: "));
        if( actual < 0 ){
            perror("write");
            exit(-1);
        }
    }

    // Allocate space for first character
    fdString = calloc(1, sizeof(char));

    while( (actual = read(fd, temp, 1)) > 0){
        // Allocate space for new character and insert into buffer
        // The count +2 ensure room for the null terminator
        fdString = realloc(fdString, sizeof (char) * count + 2);
        fdString[count++] = temp[0];
        if( temp[0] == '\n' ){
            break;
        }
    }
    if( actual < 0 ){
        perror("read");
        exit(-1);
    }

    fdString[count+1] = '\0';    // add null terminator

    return fdString;
}    
