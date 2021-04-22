/* File descriptor input/output. Handles dealing with file descriptors */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>

/* Writes to fd */
void writeToFd(int fd, char* string){    // Write and read to server

        // Holds amount written
        ssize_t actualWrite;

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
        writeToFd(1, "\nEnter a command: ");
    }

    // Allocate space for first character
    fdString = calloc(1, sizeof(char));

    while( (actual = read(fd, temp, 1)) > 0){
        // Allocate space for new character and insert into buffer
        // The count +2 ensure room for the null terminator
        if( temp[0] == '\n' ){
            break;
        }
        fdString = realloc(fdString, sizeof (char) * (count + 2));
        fdString[count++] = temp[0];
    }
    if( actual < 0 ){
        perror("read");
        exit(-1);
    }

    fdString[count] = '\0';    // add null terminator

    if( fd == 0){
        writeToFd(1, "\n");
    }

    return fdString;
}    

/* Split the supplied string around spaces into an array of string tokens */
/* Returns a string array where first string is the command, and second is the parameter */
char** tokenSplit(char* string){

    char* command = calloc(5, sizeof(char));                 // Limited to longest command + \0
    char* parameter = calloc( PATH_MAX + 2, sizeof(char));;  // Should be limited to max size of path + \n and \0
    char** tokens = calloc(2, sizeof(char*));

    tokens[0] = command;
    tokens[1] = parameter;

    int currentToken = -1;          // Start at -1 because preincrement
    int currentChar = 0;
    int lastCharWasSpace = 0;       // Flag to determine if last char was space

    // Loop over characters in string
    for(int i = 0; i < strlen(string); i++){
        // Skip spaces, set last space flag, reset char count to 0
        if( isspace(string[i]) ){
            currentChar = 0;
            lastCharWasSpace = 1;
            continue;
        }
        // Increment token count if last space was char (new word)
        if( lastCharWasSpace){
            currentToken++;
            if( currentToken > 1 ){
                fprintf(stderr, "User input contained too many tokens\n");
                tokens = NULL;
                return tokens;
            }
            lastCharWasSpace = 0;
        }
        if( currentToken < 0 ){
            currentToken = 0;
        }
        // Place char in correct spot and increment char
        tokens[currentToken][currentChar] = string[i];
        currentChar ++;

    }

    return tokens;
}

/* Stat the file and make sure that it exists, is of the correct type, and has correct permissions */
/* Path is the path to the file/directtory, type is either "dir" or "reg", perm is an integer using */
/* created using a bitwise or of the macros R_OK, W_OK, X_OK as with the access system call */
/* Returns 0 on success or errno on error                                                   */
int statfile(char *path, char *type, int perm){
    struct stat statp;

    // Error out it path does not exist
    if( lstat(path, &statp) < 0 ){
        return errno;
    }

    // Ensure path is of the right type
    if( strcmp( type, "dir" ) == 0 ){
        if( !S_ISDIR(statp.st_mode) ){
            return ENOTDIR;             // return errno value for not directory
        }
    }else if( strcmp( type, "reg") == 0){
        if( !S_ISREG(statp.st_mode) ){
            return EISDIR;              // return errno value for directory
        }
    }else{
        return EINVAL;                  // return errno for invalid argument
    }

    // Check that permissions match
    if( access(path, perm) < 0 ){
        return errno;
    }

    return 0;
}
