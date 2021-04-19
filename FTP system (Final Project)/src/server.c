
/* Server side of the FTP system */

#include <mftp.h>

/* ----------- Defines and Globals --------------*/
static int debug = 0;       // Debug flag for verbose output


/* ------------ Structures --------------------- */
typedef struct connectData{       // Holds data about a connection
    int listenfd;                   // file descriptor for accept
    int portnum;                    // Port for connection
    int errnum;                     // Value of errno if error occurs
}connectData;
    


/* ------------ Function Prototypes ------------- */

struct connectData connection(int pnum, int queueLength);   // Start a connection on pnum, or random if pnum = 0
int serverConnection( struct connectData cdata );           // Handle starting server connection on port <pnum>
int dataConnection(int controlfd);                          // Start a data connection
int parseArgs(int argnum, char** arguments);                // Parse arguments, return port number if -p flag or -1 for default
int processCommands(int controlfd);                         // Loop and handle the commands from the client
int cd(int controlfd, char *path);                          // Change directory to path
int quit(int controlfd);                                    // Client disconnect, close fd

/* Handles program control */
int main(int argc, char * argv[]){

    // Make the server connetion on -p parameter value, or default
    // Using the listenfd from the connection
    int argVal = parseArgs(argc, argv);
    if( argVal ){
        serverConnection(connection(argVal, 4));
    }else{
        serverConnection(connection(PORTNUM, 4));
    }

    return 0;
}

/* Start a connection on the given port with a queue of given length */
/* Returns an integer array, where [0] is the listenfd, and [1] is the portnum */
struct connectData connection(int pnum, int queueLength){

    int err;
    connectData cdata;
    cdata.errnum = 0;

    if( debug ){
        printf("Setting up connection using pnum <%d>\n", pnum);
    }

    // Declare socket and set options
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if( listenfd < 0){
        perror("socket");
        cdata.errnum = errno;
        return cdata;
    }
    // ensure socket can be reused (avoid error)
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); 
    if( err < 0 ){
        perror("setsocketopt");
        cdata.errnum = errno;
        return cdata;
    }

    if( debug ){
        printf("Created socket with file descriptor <%d>\n", listenfd);
    }

    // Declare sockaddress structure and set parameters
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));             // initialize mem block to 0
    servAddr.sin_family = AF_INET;                      // Family set to ipv4 socket
    servAddr.sin_port = htons(pnum);                    // bind to port num
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);       // Accept connections from any address

    // Attempt to bind the socket
    if( bind( listenfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 ){
        perror("bind");
        cdata.errnum = errno;
        return cdata;
    }

    // Populate a strcture with the socket info
    struct sockaddr_in socketinfo = {0};
    socklen_t length = sizeof(struct sockaddr_in);  // length for getsockname
    getsockname(listenfd, (struct sockaddr*) &socketinfo, &length);

    // Assign return values
    cdata.listenfd = listenfd;
    cdata.portnum = ntohs(socketinfo.sin_port);

    if( debug ){
        printf("Bound socket to port %d\n", cdata.portnum);
    }

    // Make the listen call to spin up server
    // Set the queue to size 4
    err = listen( listenfd, queueLength);
    if( err < 0 ){
        perror("listen");
        cdata.errnum = errno;
        return cdata;
    }

    if( debug ){
        printf("Listening on socket with queue size <%d>\n", queueLength);
    }

    return cdata;
}

/* Start a data connection, return the file descriptor or -1 on error */
int dataConnection(int controlfd){

    int datafd;                                         // File descriptor for data connection
    struct sockaddr_in clientAddr;                      // stucture holds client information
    socklen_t length = sizeof(struct sockaddr_in); 

    char ackWithPort[8];    // Enough for A*****\n\0

    if( debug ){
        printf("Child <%d>: Starting Data connection\n", getpid());
    }

    // Start new fd with random port
    connectData cdata = connection(0, 1); 

    // Catch possible error results from connection
    if( cdata.errnum ){
        writeToFd(controlfd, "E");
        writeToFd(controlfd, strerror(cdata.errnum));
        writeToFd(controlfd, "\n");
        return -1;
    }

    if( debug ){
        printf("Child <%d>: Created data connection on port <%d>\n", getpid(), cdata.portnum);
    }

    // Create ack string, write to fd
    sprintf(ackWithPort, "A%d\n", cdata.portnum);
    writeToFd(controlfd, ackWithPort);            

    // Wait on connection
    datafd = accept(cdata.listenfd, (struct sockaddr*) &clientAddr, &length);
    if( datafd < 0 ){
        if( debug ){
            perror("accept");
        }
        writeToFd(controlfd, "E");
        writeToFd(controlfd, strerror(errno));
        writeToFd(controlfd, "\n");
        return -1;
    }

    // Get address and resolve to hostname
    char hostName[NI_MAXHOST];
    int hostEntry;
    hostEntry = getnameinfo((struct sockaddr*) &clientAddr,
                            sizeof(clientAddr),
                            hostName,
                            sizeof(hostName),
                            NULL, 0, NI_NUMERICSERV);
    if( hostEntry != 0 && debug ){
        printf("Error: %s\n", gai_strerror(hostEntry));
    }

    if( debug ){
        printf("Child <%d>: Connection successful with host <%s> on port <%d>, using fd <%d>\n", getpid(), hostName, cdata.portnum, datafd);
    }

    return datafd;
}
    

// Spin up a server on pnum and wait for a connection
// Forks off new process for each connection
int serverConnection(struct connectData cdata){

    int err;

    // Go into accept (block and wait for connection) and fork loop
    int connectfd;
    socklen_t length = sizeof(struct sockaddr_in);          // save to use in accept funtion call for brevity
    struct sockaddr_in clientAddr;                          // contains client address structure

    for(;;){

        // Attempt to clean up child processes
        while(waitpid(0, &err, WNOHANG)>0);
        if(err < 0){
            perror("wait");
        }

        // Wait on the connection at the listenfd
        connectfd = accept(cdata.listenfd, (struct sockaddr*) &clientAddr, &length);      // wait on connection
        if( connectfd < 0 ){
            perror("accept");
            exit(-errno);
        }

        // Get address and resolve to hostname
        char hostName[NI_MAXHOST];
        int hostEntry;
        hostEntry = getnameinfo((struct sockaddr*) &clientAddr,
                                sizeof(clientAddr),
                                hostName,
                                sizeof(hostName),
                                NULL, 0, NI_NUMERICSERV);
        if( hostEntry != 0 ){
            fprintf(stderr, "Error: %s\n", gai_strerror(hostEntry));
            exit(1);
        }

        // Fork off a child to handle the connection on a separate process
        int procId = fork();
        if( procId ){                                   // parent block
            // Close parent copy of file descriptor
            close(connectfd);

            printf("Parent: Connected to client (%s) using port <%d>\n", hostName, cdata.portnum);

            // send output to stdout
            if(debug){
                printf("Parent: Forked process <%d>\n",procId);
                fflush(stdout);
            } 


        }else{                                          // child block
            printf("Child <%d>: Child block executing current command\n", getpid());
            fflush(stdout);
            processCommands(connectfd); // Hand execution over to command handler
            exit(0);                    // exit the child
        }
    }
    return 0;
}

/* Read and process commands from the client until the client exits */
int processCommands(int controlfd){
    printf("Child <%d>: Processing commands from controlfd <%d>\n", getpid(), controlfd);
    char* command;
    int datafd;
    for(;;){
        command = readFromFd(controlfd);
        if( strlen(command) == 0 ){
            continue;
        }
        switch(command[0]){
            case 'D':
                printf("Child <%d>: D from client\n", getpid());
                datafd = dataConnection(controlfd);
                break;
            case 'C':
                if( cd(controlfd, command + 1) < 0 && debug){
                    printf("Child <%d>: cd function returned non-zero status\n", getpid());
                }
                break;
            case 'L':
                printf("Child <%d>: L from client\n", getpid());
                writeToFd(controlfd, "A\n");
                close(datafd);
                break;
            case 'G':
                printf("Child <%d>: G from client\n", getpid());
                writeToFd(controlfd, "A\n");
                break;
            case 'P':
                printf("Child <%d>: P from client\n", getpid());
                writeToFd(controlfd, "A\n");
                break;
            case 'Q':
                if( quit(controlfd) < 0 && debug ){
                    printf("Child <%d>: quit function returned non-zero status\n", getpid());
                }
                return 0;
            default:
                printf("Error. Invalid command <%c> from client\n", command[0]);
        }

        free(command);
    }
    return 0;
}


/* Parse command line arguments. Return portnumber if -p flag used, 0 for default portnum, exit on error*/
int parseArgs(int argnum, char** arguments){
    int portnum;

    char* portMsg = "Provided port number was not a valid number\n\n";

    char* usageMsg = "Usage:\n"
                    "\t./mftpserve <-d> <-p portnum>\n"
                    "\n\t-d: Optional debugging flaf for verbose output\n"
                    "\t-p portnum: Use port number (portnum) for connection. Otherwise defaults to 37896\n\n";

    // If no arguments, set default and return
    if( argnum == 1){
        return 0;
    }

    // Check for debug flag
    if( strcmp(arguments[1], "-d") == 0){
        writeToFd(1, "Debug flag set\n");
        debug = 1;
        
        // If -p flag, check to see if next arg is a number
        if( argnum == 4 && strcmp(arguments[2], "-p") == 0){
            // Return portnum if valid
            if( (portnum = atoi(arguments[3])) ){
                return portnum;
            }else{
                writeToFd(2, portMsg);
                exit(-1);
            }
        }
    // Check for p flag as second arg
    }else if( argnum == 3 && strcmp(arguments[1], "-p") == 0){
        // return portnum if valid
        if( (portnum = atoi(arguments[2])) ){
            return portnum;
        }else{
            writeToFd(2, portMsg);
            exit(-1);
        }
    // If neither -d or -p, print usage message
    }else{
        writeToFd(2, usageMsg);
        exit(-1);
    }

    // Default value
    return 0;
}

/* Execute chdir to the given path. Return 0 on success, -1 on error */
int cd(int controlfd, char *path){

    int err;
    char currentDir[PATH_MAX];

    if( debug ){
        printf("Child <%d>: Changing directory to %s\n", getpid(), path);
    }

    // Change directory to path
    err = chdir(path);
    if( err < 0 ){
        // If error, write error message to controlfd
        writeToFd(controlfd, "E");
        writeToFd(controlfd, strerror(errno));
        writeToFd(controlfd, "\n");
        if( debug ){
            printf("Child <%d>: Error: %s\n", getpid(), strerror(errno));
        }
        return -1;
    }else{
        // If no error, write acknowledgment
        writeToFd(controlfd, "A\n");
    }

    // Output new diretory
    if( debug ){
        getcwd(currentDir, PATH_MAX);
        if( currentDir == NULL ){
            perror("getcwd");
        }
        printf("Child <%d>: Directory changed to %s\n", getpid(), currentDir);
    }

    return 0;
}

/* Perform actions to disconnect client */
/* Returns 0 on success, -1 on error */
int quit(int controlfd){
    
    if( debug ){
        printf("Child <%d>: Exit command detected. Ending connection\n", getpid());
    }

    // Close the control file descriptor, write acknowledge
    writeToFd(controlfd, "A\n");
    if( close(controlfd) < 0 ){
        if( debug ){
            perror("close");
        }
        return -1;
    }

    return 0;
}
