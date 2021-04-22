

#include <mftp.h>
#include <mftpserveHandler.h>


struct connectData connection(int pnum, int queueLength);   
int serverConnection( struct connectData cdata );           
int dataConnection(int controlfd);                          
int parseArgs(int argnum, char** arguments);                
int processCommands(int controlfd);                         


int main(int argc, char * argv[]){

    
    
    int argVal = parseArgs(argc, argv);
    if( argVal ){
        serverConnection(connection(argVal, 4));
    }else{
        serverConnection(connection(PORTNUM, 4));
    }

    return 0;
}


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
                free(command);
                return 0;
            default:
                printf("Error. Invalid command <%c> from client\n", command[0]);
        }

        free(command);
    }
    return 0;
}



struct connectData connection(int pnum, int queueLength){

    int err;
    char caller[8];
    connectData cdata;
    cdata.errnum = 0;

    
    if( pnum == 0 ){
        sprintf(caller, "Child");
    }else{
        sprintf(caller, "Parent");
    }

    if( debug ) printf("%s <%d>: Setting up connection using pnum <%d>\n", caller, getpid(), pnum);

    
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if( listenfd < 0){
        perror("socket");
        cdata.errnum = errno;
        return cdata;
    }
    
    err = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); 
    if( err < 0 ){
        perror("setsocketopt");
        close(listenfd);
        cdata.errnum = errno;
        return cdata;
    }

    if( debug ) printf("%s <%d>: Created socket with file descriptor <%d>\n", caller, getpid(), listenfd);

    
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));             
    servAddr.sin_family = AF_INET;                      
    servAddr.sin_port = htons(pnum);                    
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);       

    
    if( bind( listenfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 ){
        perror("bind");
        cdata.errnum = errno;
        close(listenfd);
        return cdata;
    }

    
    struct sockaddr_in socketinfo = {0};
    socklen_t length = sizeof(struct sockaddr_in);  
    getsockname(listenfd, (struct sockaddr*) &socketinfo, &length);

    
    cdata.listenfd = listenfd;
    cdata.portnum = ntohs(socketinfo.sin_port);

    if( debug ) printf("%s <%d>: Bound socket to port <%d>\n", caller, getpid(), cdata.portnum);
    

    
    
    err = listen( listenfd, queueLength);
    if( err < 0 ){
        perror("listen");
        cdata.errnum = errno;
        return cdata;
    }

    if( debug )  printf("%s <%d>: Listening on socket with queue size <%d>\n", caller, getpid(), queueLength);

    return cdata;
}


int dataConnection(int controlfd){

    int datafd;                                         
    struct sockaddr_in clientAddr;                      
    socklen_t length = sizeof(struct sockaddr_in); 

    char ackWithPort[8];    

    if( debug ) printf("Child <%d>: Starting Data connection\n", getpid());

    
    connectData cdata = connection(0, 1); 

    
    if( cdata.errnum ){
        writeToFd(controlfd, "E");
        writeToFd(controlfd, strerror(cdata.errnum));
        writeToFd(controlfd, "\n");
        return -1;
    }

    if( debug ) printf("Child <%d>: Created data connection on port <%d>\n", getpid(), cdata.portnum);

    
    sprintf(ackWithPort, "A%d\n", cdata.portnum);
    writeToFd(controlfd, ackWithPort);            

    
    datafd = accept(cdata.listenfd, (struct sockaddr*) &clientAddr, &length);
    if( datafd < 0 ){
        error(controlfd, errno);
        return -1;
    }

    
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

    if( debug ) printf("Child <%d>: Connection successful with host <%s> on port <%d>, using fd <%d>\n", getpid(), hostName, cdata.portnum, datafd);

    close(cdata.listenfd);
    return datafd;
}
    



int serverConnection(struct connectData cdata){

    int err;

    
    int connectfd;
    socklen_t length = sizeof(struct sockaddr_in);          
    struct sockaddr_in clientAddr;                          

    for(;;){

        
        while(waitpid(0, &err, WNOHANG)>0);
        if(err < 0){
            perror("wait");
        }

        
        connectfd = accept(cdata.listenfd, (struct sockaddr*) &clientAddr, &length);      
        if( connectfd < 0 ){
            perror("accept");
            exit(-errno);
        }


        
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

        
        int procId = fork();
        if( procId ){                                   
            
            close(connectfd);
            printf("Parent <%d>: Connected to client (%s) using port <%d>\n", getpid(), hostName, cdata.portnum);

            
            if(debug) printf("Parent <%d> : Forked process <%d>\n", getpid(), procId);


        }else{                                          
            if( debug) printf("Child <%d>: Child block executing current command\n", getpid());
            fflush(stdout);
            processCommands(connectfd); 
            exit(0);                    
        }
    }
    return 0;
}




int parseArgs(int argnum, char** arguments){
    int portnum;

    char* portMsg = "Provided port number was not a valid number\n\n";

    char* usageMsg = "Usage:\n"
                    "\t./mftpserve <-d> <-p portnum>\n"
                    "\n\t-d: Optional debugging flaf for verbose output\n"
                    "\t-p portnum: Use port number (portnum) for connection. Otherwise defaults to 37896\n\n";

    
    if( argnum == 1){
        printf("Debug flag not set\n");
        return 0;
    }

    
    if( strcmp(arguments[1], "-d") == 0){
        writeToFd(1, "Debug flag set\n");
        debug = 1;
        
        
        if( argnum == 4 && strcmp(arguments[2], "-p") == 0){
            
            if( (portnum = atoi(arguments[3])) ){
                return portnum;
            }else{
                writeToFd(2, portMsg);
                exit(-1);
            }
        }
    
    }else if( argnum == 3 && strcmp(arguments[1], "-p") == 0){
        
        if( (portnum = atoi(arguments[2])) ){
            return portnum;
        }else{
            writeToFd(2, portMsg);
            exit(-1);
        }
    
    }else{
        writeToFd(2, usageMsg);
        exit(-1);
    }

    
    return 0;
}
