/* Client side of the FTP System */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fdio.h>
#include <dirent.h>

/* ------------------- Defines and Globals ---------------------- */
#define PORTNUM 37896   // Port number for connection to server

static int debug = 0;   // Debug flag for verbose output

/* --------------------- Function Prototypes -------------------- */
char* parseArgs(int c, char** v);                           // Parse command line arguments
int attemptConnection( const char *address, int pnum);      // Handle spinning up client connection
int processCommands(int commandfd);                         // Process commands
char** tokenSplit(char* string);                            // Split string into space separated tokens

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
    char **tokens;                          // Result of splitting command into tokens
    char *serverResponse;                   // Holds response from server

    for(;;){

        command = readFromFd(0);
        tokens = tokenSplit(command);

        if( strcmp(command, "exit\n") == 0 ){
            writeToFd("Q\n", commandfd);
            serverResponse = readFromFd(commandfd);
            printf("Server response: %s", serverResponse);
            free(command);
            free(serverResponse);
            printf("Exiting...\n");
            exit(0);
        }else if( strcmp(command, "cd\n") == 0){
            // TODO: Implement local CD, no server comms needed
            printf("Reached cd execution block\n");
        }else if( strcmp(command, "rcd\n") == 0){
            // TODO: Implement remote CD, no data connectio needed
            // Do regex matching on input
            printf("Reached rcd execution block\n");
        }else if( strcmp(command, "ls\n") == 0){
            // TODO: Implment local LS, no server comms needed
            printf("Reached ls exeuction block\n");
        }else if( strcmp(command, "rls\n") == 0){
            writeToFd("L\n", commandfd);
            serverResponse = readFromFd(commandfd);
            printf("Server repsonse: %s\n", serverResponse);
            free(serverResponse);
        }else if( strcmp(command, "get\n") == 0){
            // TODO: Implement get (stream file through fd)
            // Init data connection
            printf("Reached get execution block\n");
        }else if( strcmp(command, "put\n") == 0){
            // TODO: Implment put (stream file through fd)
            // Init data connection
            printf("Reached put execution block\n");
        }else if( strcmp(command, "show\n") == 0){
            printf("Reached show execution block\n");
        }else{
            printf("Command %s not recognized\n", command);
        }

        free( command );
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

/* Split the supplied string around spaces into an array of string tokens */
char** tokenSplit(char* string){

    printf("Reached token split \n");
    char* command = calloc(4, sizeof(char));                 // Limited to longest command + \0
    char* parameter = calloc( PATH_MAX + 2, sizeof(char));;  // Should be limited to max size of path + \n and \0
    char** tokens = calloc(2, sizeof(char*));

    tokens[0] = command;
    tokens[1] = parameter;

    int currentToken = -1;          // Start at -1 because preincrement
    int currentChar = 0;
    int lastCharWasSpace = 0;       // Flag to determine if last char was space

    printf("Beginning loop \n");

    // Loop over characters in string
    for(int i = 0; i < strlen(string); i++){
        // Skip spaces, set last space flag, reset char count to 0
        if( string[i] == ' ' ){
            currentChar = 0;
            lastCharWasSpace = 1;
            continue;
        }
        // Increment token count if last space was char (new word)
        if( lastCharWasSpace){
            currentToken++;
            lastCharWasSpace = 0;
        }
        if( currentToken < 0 ){
            currentToken = 0;
        }
        printf("Putting char %c into spot %d of token %d\n", string[i], currentChar, currentToken);
        // Place char in correct spot and increment char
        tokens[currentToken][currentChar] = string[i];
        currentChar ++;
    }

    printf("Split %s into tokens: <%s>, <%s>\n", string, tokens[0], tokens[1]);

    return tokens;
}
