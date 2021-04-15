/* Client side of the Ftp syste */
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
#include <ctype.h>

/* ------------------- Defines and Globals ---------------------- */
#define PORTNUM 37896   // Port number for connection to server

static int debug = 0;   // Debug flag for verbose output

/* ------------------- Define terminal output colors ----------- */
#define KGRN "\x1b[32m"
#define KRED "\x1b[31m"
#define KWHT "\x1b[37m"

/* --------------------- Function Prototypes -------------------- */
char* parseArgs(int c, char** v);                           // Parse command line arguments
int attemptConnection( const char *address, int pnum);      // Handle spinning up client connection
int processCommands(const char* hostname, int commandfd);   // Process commands
char** tokenSplit(char* string);                            // Split string into space separated tokens
int cdLocal(char* path);                                    // cd to path on local machine
int lsLocal();                                              // execute ls command on local
int lsRemote(const char* hostname, int commandfd);          // execute ls on server
int cdRemote(const char* path, int commandfd);              // execute cd on server
int get(char* path, const char *hostname, int commandfd);// get file from the server
int makeDataConnection(const char* hostname, int commandfd);// self explanatory

/* Handles program control */
int main(int argc, char* argv[]){

    const char* hostname;
    int commandfd;

    hostname = parseArgs(argc, argv);
    commandfd = attemptConnection(hostname, PORTNUM);
    processCommands(hostname, commandfd);

    return 0;
}

/* Get commands from user and process them appropriately */
int processCommands(const char* hostname, int commandfd){

    char *command;                          // Holds return from readFromFd for stdin
    char **tokens = NULL;                   // Result of splitting command into tokens
    char *serverResponse;                   // Holds response from server
    int err;
    

    for(;;){

        // Read command from stdin, then split into tokens
        command = readFromFd(0);
        if( (tokens = tokenSplit(command)) == NULL){
            free(command);
            continue;
        }
        free(command);

        err = 0;       // reset err variable
        

        /* EXIT COMMAND EXECUTION BLOCK */
        if( strcmp(tokens[0], "exit") == 0 ){

            // Write command to server, and read response 
            writeToFd(commandfd, "Q\n");
            serverResponse = readFromFd(commandfd);
            if( serverResponse[0] == 'E'){
                printf("Error %s from server\n", serverResponse + 1);
            }
            
            // Free allocated memory
            free(tokens[0]);
            free(tokens[1]);
            free(tokens);
            free(serverResponse);

            // Close socket
            close(commandfd);

            printf("Exiting...\n");

            exit(0);

        /* CD COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "cd") == 0){
            char currentDir[PATH_MAX];

            // Change directory
            err = chdir(tokens[1]);
            if( err < 0 ){
                perror("chdir");
            }

            // Output change if debug flag set
            if( debug ){
                getcwd(currentDir, PATH_MAX);
                if( currentDir == NULL){
                    perror("getcwd");
                }
                printf("Directory changed to %s\n", currentDir);
            }

        /* RCD COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "rcd") == 0){

            if( cdRemote(tokens[1], commandfd) < 0  && debug){
                writeToFd(2, "rcd command did not execute properly\n");
            }

        /* LS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "ls") == 0){

            if( lsLocal() < 0  && debug){
                writeToFd(2, "ls command did not execute properly\n");
            }

        /* RLS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "rls") == 0){

            if( lsRemote(hostname, commandfd) < 0){
                writeToFd(2, "rls command did not execute properly\n");
            }

        }else if( strcmp(tokens[0], "get") == 0){
            // TODO: Implement get (stream file through fd)
            // Init data connection
            if( get(tokens[1], hostname, commandfd) < 0){
                writeToFd(2, "get command did not execute properly\n");
            }

        }else if( strcmp(tokens[0], "put") == 0){
            // TODO: Implment put (stream file through fd)
            // Init data connection
            printf("Reached put execution block\n");

        }else if( strcmp(tokens[0], "show") == 0){
            printf("Reached show execution block\n");

        }else{
            printf("Command %s not recognized\n", tokens[0]);

        }

        free(tokens[0]);
        free(tokens[1]);
        free(tokens);
    }
    return 0;
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

    freeaddrinfo(actualData);

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

/* Executes ls -l | more -20 on client side */
int lsLocal(){
    int pipefd[2];
    int err, rdr, wtr;

    // Fork program to execute ls and more
    if( fork() ){// Parent just waits on children to exit
        wait(&err);
        if( err < 0 ){
            perror("pipe");
            return -1;
        }
    }else{// Child process forks again to run each program

        // Initiate pipe and rename reader and writer sides
        if( pipe(pipefd) < 0 ){
            perror("pipe");
            return -1;
        }
        rdr = pipefd[0]; wtr = pipefd[1];

        if( fork() ){   // Execute more -20 in the parent
            close(wtr);
            close(0); dup(rdr); close(rdr);     // make stdin go to reader
            if( execlp("more", "more",  "-20", (char *) NULL) == -1){
                perror("exec");
                return -1;
            }
        }else{          // Execute ls -l in the child
            close(rdr);
            close(1); dup(wtr); close(wtr); // make stdout go to writer
            if( execlp("ls", "ls", "-l", "-a", (char *) NULL) == -1){
                perror("exec");
                return -1;
            }
        }
    }

    return 0;
}

/* Execute ls -l | more -20 on server side */
int lsRemote(const char* hostname, int commandfd){
    char* serverResponse;
    int datafd;
    int err;

    // Make data connection, and continue if successful
    if( (datafd = makeDataConnection(hostname, commandfd)) > 0){

        // Write ls command to server
        writeToFd(commandfd, "L\n");
        serverResponse = readFromFd(commandfd);
        // Return if error
        if( serverResponse[0] == 'E'){
            fprintf(stderr, "Error: %s from server\n", serverResponse + 1);
            free( serverResponse );
            close(datafd);
            return -1;
        }
        free(serverResponse);

        // Fork off to pipe the datafd information into stdin for more
        if( fork() ){
            close(datafd);

            // Wait for child (more -20) to finish
            wait(&err);
            if( err < 0){
                perror("wait");
                return -1;
            }

            return 0;
        }else{
            // Redirect data socket to stdin
            close(0); dup(datafd); close(datafd);

            // Send everything in the datafd to more
            if( execlp("more", "more", "-20", (char*) NULL) == -1 ){
                perror("exec");
            }
        }
    }
    return -1; //If we got here, there was an error with datafd
}

/* Initiate the data connection, return the fd */
int makeDataConnection(const char* hostname, int commandfd){
    char* serverResponse;
    char* substr;
    int datafd;

    // Write command to server to open data socket
    writeToFd(commandfd, "D\n");
    serverResponse = readFromFd(commandfd);

    // If server sends back error, print it
    if ( serverResponse[0] == 'E'){
        fprintf(stderr, "Error: %s from server\n", serverResponse + 1);
        free( serverResponse );
        return(-1);
    }else{
        // If server sends back port number, connect to it
        substr = serverResponse + 1;
        if( debug )
            printf("Attempting connection on portnum: <%s>\n", substr);
        // Convert string number to integer, and make connection on that port
        datafd = attemptConnection(hostname, atoi(substr)); 
        free(serverResponse);
    }
    return datafd;
}

/* Execute the cd command on server */
/* Returns 0 on success, -1 on error */
int cdRemote(const char* path, int commandfd){
    char *serverResponse;

    // Create command string using path
    char cdWithPath[strlen(path) + 3];    // for C, Path, \n, \0
    sprintf(cdWithPath, "C%s\n", path);

    // Write command to server
    writeToFd(commandfd, cdWithPath);

    // Grab server response
    serverResponse = readFromFd(commandfd);
    if( serverResponse[0] == 'E' ){
        fprintf(stderr, "Error: %s from server\n", serverResponse + 1);
        free( serverResponse );
        return -1;
    }

    free(serverResponse);
    return 0;
}

/* Get the filed specified by <path> from the server at address <address> */
/* Save this file in the cwd. Returns 0 on success, -1 on error */
int get(char* path, const char *hostname, int commandfd){
    char* serverResponse;
    int datafd, filefd;
    char buf[256];
    int actualRead, actualWrite;
    int returnStatus = 0;

    // Filename is everything after last /, or path if no /
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = (strrchr(path, '/') + 1);
    }else{
        filename = path;
    }

    // Create command string to send to server
    char getWithPath[strlen(path) + 3];
    sprintf(getWithPath, "G%s\n", path);

    // Make data connection to server
    if( (datafd = makeDataConnection(hostname, commandfd)) < 0){
        close(datafd);
        return -1;
    }

    // Write command to server
    writeToFd(commandfd, getWithPath);

    serverResponse = readFromFd(commandfd);
    if( serverResponse[0] == 'E'){
        fprintf(stderr, "Error: %s from server\n", serverResponse + 1);
        return -1;
    }
    free(serverResponse);

    // Create new file and open it
    filefd = open(filename, O_CREAT|O_WRONLY, 0700); 

    // Read file from datafd, write to fild
    while( (actualRead = read(datafd, buf, sizeof(buf)/ sizeof(char))) > 0){
        actualWrite = write(filefd, buf, actualRead);
        // on write error, delete file, and return -1
        if( actualWrite < 0){
            perror("write");
            returnStatus = -1;
            break;
        }
    }
    // On read error, delete file and return
    if( actualRead < 0 ){
        perror("read");
        returnStatus = -1;
    }

    // If there was an error, delete the file we just created
    if( returnStatus == -1 ){
        unlink(filename);
    }

    // Close the file descriptors
    close(filefd);
    close(datafd);

    printf("File transfer successful\n");

    return returnStatus;
}

