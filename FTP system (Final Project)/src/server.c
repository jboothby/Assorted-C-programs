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
int ls(int controlfd, int datafd);                          // Execute the ls command, pipe the results into datafd
int get(int controlfd, int datafd, char* path);             // Get the file specified by path, write into the datafd
int put(int controlfd, int datafd, char* path);             // Create the file in the fd and save it locally
void error(int controlfd, int errnum);                      // Handle errors by printing to the controlfd

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
            printf("Child <%d>: Child block executing current command\n", getpid());
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
/* Returns 0 on success or -1 on error*/
int cd(int controlfd, char *path){

    int err;
    char currentDir[PATH_MAX];

    // Change directory to path
    err = chdir(path);
    if( err < 0 ){
        // If error, write error message to controlfd
        error(controlfd, errno);
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

/* Execute the ls command on local. Send results through datafd */
/* Returns 0 on success or -1 on error*/
int ls(int controlfd, int datafd){
   int err; 
   int procId;

   // For off new process to handle ls
   procId = fork();
   // In parent, just wait for child to exit
   if( procId ){

       close( datafd ); // close datafd, only child needs it

       if( debug ){
           printf("Child <%d>: spawned new process <%d> to handle ls\n", getpid(), procId);
       }

       wait(&err);      // Wait for child to finish executing

       // Write to indicate command executed properly
       writeToFd(controlfd, "A\n");

       if( debug ){
           printf("Child <%d>: Finished executing ls\n", getpid());
       }
   }else{
       // rename stdout to datafd
       close(1); dup(datafd); close(datafd);

       // Execute ls command, write error if it happens
       if( execlp("ls", "ls", "-l", "-a", (char *) NULL) == -1){
           error(controlfd, errno);
           return -1;
       }

   }

   return 0;
}

/* Read the file at <path> and write into datafd*/
/* Returns 0 on success or -1 on error*/
int get(int controlfd, int datafd, char* path){
    int filefd;
    char buf[256];
    int actualRead, actualWrite;

    if( debug ){
        printf("Child <%d>: Getting file at <%s>\n", getpid(), path);
    }

    // Open file, handle errors
    filefd = open(path, O_RDONLY);
    if( filefd < 0 ){
        error(controlfd, errno);
        close(datafd);
        return -1;
    }

    // Send acknowledge to client
    writeToFd(controlfd, "A\n");

    // Read contents of file into buffer
    while((actualRead = read(filefd, buf, sizeof(buf)/sizeof(char))) > 0){
        // Write contents of buffer into datafd
        actualWrite = write(datafd, buf, actualRead);
        if( actualWrite < 0 ){
            error(controlfd, errno);
            close(datafd);
            return -1;
        }
    }
    if( actualRead < 0 ){
        error(controlfd, errno);
        close(datafd);
        return -1;
    }

    return 0;
}

/* Read file from datafd, create a file and save file locally */
/* returns 0 on success or -1 on error */
int put(int controlfd, int datafd, char* path){

    int filefd;
    char buf[256];
    int actualRead, actualWrite;

    // If there are slashes, the filename is everything after the last one
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = strrchr(path, '/') + 1;  // +1 is pointer magic to skip the last /
    }else{
        filename = path;
    }


    if( debug ){
        printf("Child <%d>: Creating file <%s>, saving to local\n", getpid(), filename);
    } 

    // Create the new file
    filefd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0700);
    if( filefd < 0 ){
        error(controlfd, errno);
        close(filefd);
        return -1;
    }

    // Write success to client
    writeToFd(controlfd, "A\n");

    // Read from the datafd, write to file
    while( (actualRead = read(datafd, buf, sizeof(buf)/sizeof(char))) > 0){
        actualWrite = write(filefd, buf, actualRead);
        // On write error, delete file 
        if( actualWrite < 0 ){
            if( debug ){
                perror("write");
            }
            unlink(filename);
            close(filefd);
            return -1;
        }
    }
    // On read error, delete file
    if( actualRead < 0 ){
        if( debug ){
            perror("read");
        }
        unlink(filename);
        close(filefd);
        return -1;
    }

    if( debug ){
        printf("Child <%d>: File transfer successful\n", getpid());
    }

    return 0;
}

/* Handle error output properly */
void error(int controlfd, int errnum){
    // Send error output to client
    writeToFd(controlfd, "E");
    writeToFd(controlfd, strerror(errnum));
    writeToFd(controlfd, "\n");

    // Print errors on server side if debug output
    if( debug ){
        printf("Process <%d>: Error: %s\n", getpid(), strerror(errnum));
    }
}
