#include <mftp.h>

/* ------------------- Defines and Globals ---------------------- */
#define PORTNUM 37896   // Port number for connection to server

static int debug = 0;   // Debug flag for verbose output

/* --------------------- Function Prototypes -------------------- */
char* parseArgs(int c, char** v);                           // Parse command line arguments
int attemptConnection( const char *address, int pnum);      // Handle spinning up a socket connection
int makeDataConnection(const char* hostname, int controlfd);// data socket wrapper for attemptConnection
int processCommands(const char* hostname, int controlfd);   // Process commands
int morePipe(int datafd);                                   // Pipe the results from the fd into more
int cdLocal(char* path);                                    // cd to path on local machine
int lsLocal();                                              // execute ls command on local
int lsRemote(const char* hostname, int controlfd);          // execute ls on server
int cdRemote(const char* path, int controlfd);              // execute cd on server
int get(char* path, const char *hostname, int controlfd, int save);// get file from the server
int put(char* path, const char *hostname, int controlfd);   // transfer file from client to server

/* Handles program control */
int main(int argc, char* argv[]){

    const char* hostname;
    int controlfd;

    hostname = parseArgs(argc, argv);
    controlfd = attemptConnection(hostname, PORTNUM);
    processCommands(hostname, controlfd);

    return 0;
}

/* Get commands from user and process them appropriately */
int processCommands(const char* hostname, int controlfd){

    char *command;                          // Holds return from readFromFd for stdin
    char **tokens = NULL;                   // Result of splitting command into tokens
    char *serverResponse;                   // Holds response from server
    int err;
    

    // Inifite loop, breaks on exit command
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
            writeToFd(controlfd, "Q\n");
            serverResponse = readFromFd(controlfd);
            if( serverResponse[0] == 'E'){
                writeToFd(2, "Exit command did not execute properly\n");
            }
            
            // Free allocated memory
            free(tokens[0]);
            free(tokens[1]);
            free(tokens);
            free(serverResponse);

            // Close socket
            close(controlfd);

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

            if( cdRemote(tokens[1], controlfd) < 0  && debug){
                writeToFd(2, "rcd command did not execute properly\n");
            }

        /* LS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "ls") == 0){

            if( lsLocal() < 0  && debug){
                writeToFd(2, "ls command did not execute properly\n");
            }

        /* RLS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "rls") == 0){

            if( lsRemote(hostname, controlfd) < 0){
                writeToFd(2, "rls command did not execute properly\n");
            }

        /* GET COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "get") == 0){

            // Call get with save flag set to 1
            if( get(tokens[1], hostname, controlfd, 1) < 0){
                writeToFd(2, "Get command did not execute properly\n");
            }

        /* SHOW COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "show") == 0){

            // Call get with the save flag set to 0
            if( get(tokens[1], hostname, controlfd, 0) < 0){
                writeToFd(2, "Show command did not execute properly\n");
            }

        /* PUT EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "put") == 0){

            // Call put with the path, hostname, and controlfd
            if( put(tokens[1], hostname, controlfd) < 0){
                writeToFd(2, "Put command did not execute properly\n");
            }


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
int lsRemote(const char* hostname, int controlfd){
    char* serverResponse;
    int datafd;

    // Make data connection, and continue if successful
    if( (datafd = makeDataConnection(hostname, controlfd)) > 0){

        // Write ls command to server
        writeToFd(controlfd, "L\n");
        serverResponse = readFromFd(controlfd);
        // Return if error
        if( serverResponse[0] == 'E'){
            fprintf(stderr, "%s\n", serverResponse + 1);
            free( serverResponse );
            close(datafd);
            return -1;
        }
        free(serverResponse);

        // Send file descriptor to more pipe
        if( morePipe(datafd) < 0){
            return -1;
        }else{
            return 0;
        }

    }
    return -1; //If we got here, there was an error with datafd
}

/* Initiate the data connection, return the fd */
int makeDataConnection(const char* hostname, int controlfd){
    char* serverResponse;
    char* substr;
    int datafd;

    // Write command to server to open data socket
    writeToFd(controlfd, "D\n");
    serverResponse = readFromFd(controlfd);

    // If server sends back error, print it
    if ( serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free( serverResponse );
        return(-1);
    }else{
        // If server sends back port number, connect to it
        substr = serverResponse + 1;
        if( debug )
            printf("Attempting connection on portnum: <%s>\n", substr);
        // Convert string number to integer, and make connection on that port
        datafd = attemptConnection(hostname, atoi(substr)); 
        if( debug )
            printf("Data connection successful\n");
        free(serverResponse);
    }
    return datafd;
}

/* Execute the cd command on server */
/* Returns 0 on success, -1 on error */
int cdRemote(const char* path, int controlfd){
    char *serverResponse;

    // Create command string using path
    char cdWithPath[strlen(path) + 3];    // for C, Path, \n, \0
    sprintf(cdWithPath, "C%s\n", path);

    // Write command to server
    writeToFd(controlfd, cdWithPath);

    // Grab server response
    serverResponse = readFromFd(controlfd);
    if( serverResponse[0] == 'E' ){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free( serverResponse );
        return -1;
    }

    free(serverResponse);
    return 0;
}

/* Get the filed specified by <path> from the server at address <address> */
/* Save this file in the cwd. Returns 0 on success, -1 on error */
int get(char* path, const char *hostname, int controlfd, int save){
    char* serverResponse;
    int datafd, outputfd;
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
    if( (datafd = makeDataConnection(hostname, controlfd)) < 0){
        close(datafd);
        return -1;
    }

    // Write command to server
    writeToFd(controlfd, getWithPath);

    // Get server response, print error if exists
    serverResponse = readFromFd(controlfd);
    if( serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free(serverResponse);
        return -1;
    }
    free(serverResponse);

    // If the save flag is not set, pass this off to more pipe
    if(!save){
        if( morePipe(datafd) < 0 ){
            return -1;
        }else{
            return 0;
        }
    }
        

    // Create new file and open it
    outputfd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0700); 
    if( outputfd < 0 ){
        perror("open");
        close(datafd);
        return -1;
    }

    // Read file from datafd, write to new file
    while( (actualRead = read(datafd, buf, sizeof(buf)/ sizeof(char))) > 0){
        actualWrite = write(outputfd, buf, actualRead);
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
    close(outputfd);
    close(datafd);

    printf("File transfer successful\n");

    return returnStatus;
}

/* Transfer the file specified by path to the server at location hostname */
int put(char* path, const char *hostname, int controlfd){
    char *serverResponse;
    char buf[256];
    int filefd, datafd;
    int actualRead, actualWrite;

    // Filename is everything after last /, or path if no /
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = (strrchr(path, '/') + 1);
    }else{
        filename = path;
    }

    // Create command string to send to server
    char putWithPath[strlen(path) + 3];
    sprintf(putWithPath, "P%s\n", filename);

    // Make data connection to server
    if( (datafd = makeDataConnection(hostname, controlfd)) < 0){
        close(datafd);
        return -1;
    }

    // Write contents of local file at path into datafd
    filefd = open(path, O_RDONLY);
    if( filefd < 0){
        perror("Write");
        close(datafd);
        return -1;
    }

    // Read data frome file, write to datafd
    while((actualRead = read(filefd, buf, sizeof(buf)/sizeof(char))) > 0){
        actualWrite = write(datafd, buf, actualRead);
        if( actualWrite < 0){
            perror("Write");
            close(filefd);
            close(datafd);
            return -1;
        }
    }
    if(actualRead < 0){
        perror("Read");
        close(filefd);
        close(datafd);
        return -1;
    }

    // Close data fd to signal transfer complete
    close(datafd);

    // Write command to server
    writeToFd(controlfd, putWithPath);
    serverResponse = readFromFd(controlfd);
    if (serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free(serverResponse);
        close(filefd);
        return -1;
    }

    // If no errors, return 0 for success
    free(serverResponse);
    close(filefd);

    return 0;

}

/* Pipe the results from the data fd into the more command */
int morePipe(int datafd){

        int err;

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
        
        return 0;
}
