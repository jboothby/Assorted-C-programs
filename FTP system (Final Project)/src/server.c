/* Server side of the FTP system */

#include <mftp.h>

/* ----------- Defines and Globals --------------*/
static int debug = 0;       // Debug flag for verbose output

/* ------------ Function Prototypes ------------- */

int serverConnection( int pnum );           // Handle starting server connection on port <pnum>
int parseArgs(int argnum, char** arguments);// Parse arguments, return port number if -p flag or -1 for default
int processCommands(int controlfd);         // 

/* Handles program control */
int main(int argc, char * argv[]){

    // Make the server connetion on -p parameter value, or default
    int argVal = parseArgs(argc, argv);
    if( argVal ){
        serverConnection(argVal);
    }else{
        serverConnection(PORTNUM);
    }

    return 0;
}

// Spin up a server on pnum and wait for a connection
// Forks off new process for each connection
int serverConnection(int pnum){
    int err;

    if( debug ){
        printf("Setting up server on port <%d>\n", pnum);
    }

    // Declare socket and set options
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if( listenfd < 0){
        perror("socket");
        exit(-errno);
    }
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); 
    if( err < 0 ){
        perror("setsocketopt");
        exit(-errno);
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
        exit(1);
    }

    // Make the listen call to spin up server
    // Set the queue to size 4
    err = listen( listenfd, 4);
    if( err < 0 ){
        perror("listen");
        exit(-errno);
    }

    // Go into accept (block and wait for connection) and fork loop
    int connectfd;
    socklen_t length = sizeof(struct sockaddr_in);          // save to use in accept funtion call for brevity
    struct sockaddr_in clientAddr;                          // contains client address structure

    for(;;){
        connectfd = accept(listenfd, (struct sockaddr*) &clientAddr, &length);      // wait on connection
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

            printf("Connected to client (%s) on port number (%d)\n", hostName, pnum);

            // send output to stdout
            if(debug){
                printf("Forked process <%d>\n",procId);
                fflush(stdout);
            } 

            wait(&err);                 // wait for all children to exit

            if(debug){
                printf("Ended process <%d>. Waiting for next connection...\n", procId);
                fflush(stdout);
            }
        }else{                                          // child block
            printf("Child block executing current command\n");
            fflush(stdout);
            processCommands(connectfd); // Hand execution over to command handler
            exit(0);                    // exit the child
        }
    }
    return 0;
}

/* Read and process commands from the client until the client exits */
int processCommands(int controlfd){
    printf("Reached Process commands with controlfd %d\n", controlfd);
    char* command;
    char* parameter;
    for(;;){
        command = readFromFd(controlfd);
        switch(command[0]){
            case 'D':
                printf("D from client\n");
                writeToFd(controlfd, "A\n");
                break;
            case 'C':
                printf("C from client\n");
                writeToFd(controlfd, "A\n");
                break;
            case 'L':
                printf("L from client\n");
                writeToFd(controlfd, "A\n");
                break;
            case 'G':
                printf("G from client\n");
                writeToFd(controlfd, "A\n");
                break;
            case 'P':
                printf("P from client\n");
                writeToFd(controlfd, "A\n");
                break;
            case 'Q':
                printf("Q from client\n");
                writeToFd(controlfd, "A\n");
                return 0;
            default:
                printf("Error. Invalid command from client\n");
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

