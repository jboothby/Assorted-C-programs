

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


void writeToFd(int fd, char* string){    

        
        ssize_t actualWrite;

        
        actualWrite = write(fd, string, strlen(string));
        if( actualWrite < 0 ){
            perror("write");
            exit(-1);
        }
}



char* readFromFd(int fd){

    int count = 0;          
    ssize_t actual;         
    char* fdString;         
    char temp[1];           
    
    
    if( fd == 0){
        writeToFd(1, "\nEnter a command: ");
    }

    
    fdString = calloc(1, sizeof(char));

    while( (actual = read(fd, temp, 1)) > 0){
        
        
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

    fdString[count] = '\0';    

    if( fd == 0){
        writeToFd(1, "\n");
    }

    return fdString;
}    



char** tokenSplit(char* string){

    char* command = calloc(5, sizeof(char));                 
    char* parameter = calloc( PATH_MAX + 2, sizeof(char));;  
    char** tokens = calloc(2, sizeof(char*));

    tokens[0] = command;
    tokens[1] = parameter;

    int currentToken = -1;          
    int currentChar = 0;
    int lastCharWasSpace = 0;       

    
    for(int i = 0; i < strlen(string); i++){
        
        if( isspace(string[i]) ){
            currentChar = 0;
            lastCharWasSpace = 1;
            continue;
        }
        
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
        
        tokens[currentToken][currentChar] = string[i];
        currentChar ++;

    }

    return tokens;
}





int statfile(char *path, char *type, int perm){
    struct stat statp;

    
    if( lstat(path, &statp) < 0 ){
        return errno;
    }

    
    if( strcmp( type, "dir" ) == 0 ){
        if( !S_ISDIR(statp.st_mode) ){
            return ENOTDIR;             
        }
    }else if( strcmp( type, "reg") == 0){
        if( !S_ISREG(statp.st_mode) ){
            return EISDIR;              
        }
    }else{
        return EINVAL;                  
    }

    
    if( access(path, perm) < 0 ){
        return errno;
    }

    return 0;
}
