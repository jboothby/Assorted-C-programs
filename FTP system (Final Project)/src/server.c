/* Server side of the FTP system */

#include <serverHandler.h>
/* ------------ Function Prototypes ------------- */

struct connectData connection(int pnum, int queueLength);   // Start a connection on pnum, or random if pnum = 0
int serverConnection( struct connectData cdata );           // Handle starting server connection on port <pnum>
int dataConnection(int controlfd);                          // Start a data connection
int parseArgs(int argnum, char** arguments);                // Parse arguments, return port number if -p flag or -1 for default
int processCommands(int controlfd);                         // Loop and handle the commands from the client

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

/* Read and process commands from the client until the client exits */
int processCommands(int controlfd){
    if( debug ) printf("Child <%d>: Processing commands from controlfd <%d>\n", getpid(), controlfd);
    char* command;
    int datafd;
    for(;;){
        command = readFromFd(controlfd);
        if( strlen(command) == 0 ){
            continue;
        }
        switch(command[0]){
            case 'D':
                if( (datafd = dataConnection(controlfd)) < 0 && debug){
                    printf("Child <%d>: data connction returned non-zero status\n", getpid());
                }
                break;
            case 'C':
                if( cd(controlfd, command + 1) < 0 && debug){
                    printf("Child <%d>: cd function returned non-zero status\n", getpid());
                }
                break;
            case 'L':
                if( ls(controlfd, datafd) < 0 && debug){
                    printf("Child <%d>: ls function returned non-zero status\n", getpid());
                }
                close(datafd);
                break;
            case 'G':
                if( (get(controlfd, datafd, command + 1) < 0) && debug ){
                    printf("Child <%d>: get function returned non-zero status\n", getpid());
                }
                close(datafd);
                break;
            case 'P':
                if( (put(controlfd, datafd, command + 1) < 0) && debug){
                    printf("Child <%d>: put function returned non-zero status\n", getpid());
                }
                close(datafd);
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

/* Start a connection on the given port with a queue of given length */
/* Returns an integer array, where [0] is the listenfd, and [1] is the portnum */
struct connectData connection(int pnum, int queueLength){

    int err;
    char caller[8];
    connectData cdata;
    cdata.errnum = 0;

    // Save who called this function
    if( pnum == 0 ){
        sprintf(caller, "Child");
    }else{
        sprintf(caller, "Parent");
    }

    if( debug ){
        printf("%s <%d>: Setting up connection using pnum <%d>\n", caller, getpid(), pnum);
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
        close(listenfd);
        cdata.errnum = errno;
        return cdata;
    }

    if( debug ){
        printf("%s <%d>: Created socket with file descriptor <%d>\n", caller, getpid(), listenfd);
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
        close(listenfd);
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
        printf("%s <%d>: Bound socket to port <%d>\n", caller, getpid(), cdata.portnum);
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
        printf("%s <%d>: Listening on socket with queue size <%d>\n", caller, getpid(), queueLength);
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
        error(controlfd, errno);
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

    close(cdata.listenfd);
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
            printf("Parent <%d>: Connected to client (%s) using port <%d>\n", getpid(), hostName, cdata.portnum);

            // send output to stdout
            if(debug){
                printf("Parent <%d> : Forked process <%d>\n", getpid(), procId);
                fflush(stdout);
            } 


        }else{                                          // child block
            if( debug) printf("Child <%d>: Child block executing current command\n", getpid());
            fflush(stdout);
            processCommands(connectfd); // Hand execution over to command handler
            exit(0);                    // exit the child
        }
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
        printf("Debug flag not set\n");
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
