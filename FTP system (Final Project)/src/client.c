/* Client side of the FTP System */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* ------------------- Defines and Globals ---------------------- */
#define PORTNUM 37896   // Port number for connection to server

static int debug = 0;   // Debug flag for verbose output

/* --------------------- Function Prototypes -------------------- */
char* parseArgs(int c, char** v);                           // Parse command line arguments
int attemptConnection( const char *address, int pnum);      // Handle spinning up client connection
char* readFromFd(int fd);                                   // Prompt for and return string from stdin
int processCommands(int commandfd);                         // Process commands
void serverWrite(char* command, int commandfd);             // Write to server


/* Handles program control */
int main(int argc, char* argv[]){

    const char* hostname;
    int commandfd;

    hostname = parseArgs(argc, argv);
    commandfd = attemptConnection(hostname, PORTNUM);
    processCommands(commandfd);

    return 0;
}

/* Get commands from user and process them appropriately */
int processCommands(int commandfd){

    char *command;                          // Holds return from readFromFd for stdin
    char *serverResponse;                   // Holds response from server

    for(;;){

        command = readFromFd(0);
    
        if( strcmp(command, "exit\n") == 0 ){
            serverWrite("Q\n", commandfd);
            free(command);
            printf("Exiting...\n");
            exit(0);
        }else if( strcmp(command, "rls\n") == 0){
            serverWrite("L\n", commandfd);
            serverResponse = readFromFd(commandfd);
            printf("Server repsonse: %s\n", serverResponse);
            free(serverResponse);
        }

        free( command );
    }
}

/* Writes to server, reads response from commandfd into dynamically allocated array */
void serverWrite(char* command, int commandfd){    // Write and read to server

        // Holds amount written
        ssize_t actualWrite;

        // Debug output
        printf("Writing <");
        for( int i = 0; i <= strlen(command); i++){
            printf("(%d)", command[i]);
        }
        printf("> to server\n");
        

        // Write command to server
        actualWrite = write(commandfd, command, strlen(command));
        if( actualWrite < 0 ){
            perror("write");
            exit(-1);
        }


}

/* Create the sockets, resolve address, then connect on specified port */
int attemptConnection( const char* address, int pnum){
    int sockfd;                             // socket file descriptor
    int err;                                // Holds error code

    struct addrinfo serv, *actualData;      // address info for the server
    memset(&serv, 0, sizeof(serv));         

    serv.ai_socktype = SOCK_STREAM;         
    serv.ai_family = AF_INET;   
    
    char port[6];                        // Length 6 because port max is 65535 + \0
    sprintf(port, "%d", pnum);           // convert portnum to string for dns lookup
    
    // Dns lookup
    err = getaddrinfo(address, port, &serv, &actualData);
    if( err ){
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(-1);
    }

    // Create socket with server 
    sockfd = socket(actualData -> ai_family, actualData -> ai_socktype, 0);
    if( sockfd < 0 ){
        perror("socket");
        exit(-1);
    }

    // Attempt server connection
    err = connect( sockfd, actualData -> ai_addr, actualData -> ai_addrlen);
    if( err < 0 ){
        perror("connect");
        exit(-1);
    }

    return sockfd;
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

/* Parse through command line arguments. Set debug flag if needed, return hostname argument */
char* parseArgs(int argnum, char** arguments){
    char *usageMsg = "Usage:\n"
                    "\t./client <-d> hostname\n"
                    "\n\t-d: optional debugging flag for verbose output\n"
                    "\thostname: symbolic hostname or IPv4 address to the server\n";

    // If there are not enough arguments, error out
    if( argnum < 2 ){
        fprintf(stderr, "%s\n", usageMsg);
        exit(-1);
    }

    // If the first arg is -d and there are 2 args
    if( strcmp( arguments[1], "-d") == 0) {
        if( argnum == 3 ){
            debug = 1;
            return(arguments[2]);
        }else{
            fprintf(stderr, "%s\n", usageMsg);
            exit(-1);
        }
    }

    // If there is only 1 argument return it as hostname
    if( argnum == 2 ){
        return(arguments[1]);
    }

    fprintf(stderr, "%s\n", usageMsg);
    exit(-1);
}
