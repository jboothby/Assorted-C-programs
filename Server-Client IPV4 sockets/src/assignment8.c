#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#define PORT_NUM 49999          // Port for socket connection

/*--------------Function Prototypes-------------------*/
int server();                       // handles spinning up the server
int client(const char *address);    // handles creating a client and connection

// Simple method prints a usage error if command line arguments are wrong
void usageMessage(){
        printf("Usage:\n\t./assignment8 client address\n\t./assignment8 server\n\n"); 
        exit(1);
}

// This funciton parses command line arguments and hands off to server and client
int main(int argc, char const *argv[]){

    // Parse command line arguments
    assert(argc > 1);

    // If first argument is "server" and there is exactly one argument, call server
    if( strcmp(argv[1], "server") == 0){
        if(argc == 2){
            server();
        }else{
            usageMessage();
        }
    // If client is first argument, and there are exactly two arguments, call client
    }else if( strcmp(argv[1], "client") == 0){
        if( argc == 3){
            client(argv[2]);
        }else{
            usageMessage();
        }
    // In any other case, print usage message and exit
    }else{
        usageMessage();
    }
    return 0;
}

// Spin up a server on PORT_NUM and wait for a connection
// Forks off new process for each connection
int server(){
    int err;

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
    servAddr.sin_port = htons(PORT_NUM);                // bind to port num
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);       // Accept connections from any address

    // Attempt to bind the socket
    if( bind( listenfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0 ){
        perror("bind");
        exit(1);
    }

    // Make the listen call to spin up server
    err = listen( listenfd, 1);
    if( err < 0 ){
        perror("listen");
        exit(-errno);
    }

    // Go into accept (block and wait for connection) and fork loop
    int connectfd;
    int length = sizeof(struct sockaddr_in);           // save to use in accept funtion call for brevity
    struct sockaddr_in clientAddr;                     // contains client address structure
    int connections = 0;                               // save number of connection attempts
    time_t currentTime;                                // Holds current time since epoch in seconds
    char timeWrite[20] = {0};                          // Buffer to hold human readable version of date time
    for(;;){
        connectfd = accept(listenfd, (struct sockaddr*) &clientAddr, &length);      // wait on connection
        if( connectfd < 0 ){
            perror("accept");
            exit(-errno);
        }

        ++connections;  // increment connections

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
            // send output to stdout
            fprintf(stdout, "%s %d\n", hostName, connections);
            close(connectfd);
        }else{                                          // child block
            // Create output of current time that is 19 bytes (including newline);
            time(&currentTime);
            strncpy(timeWrite, ctime(&currentTime), 18);
            strcat(timeWrite, "\n");
            
            err = write(connectfd, timeWrite, strlen(timeWrite));        // write a test string to output
            if( err < 0 ){
                perror("write");
                exit(-errno);   
            }
            exit(0);                    // exit the child
        }
        while(wait(NULL) > 0){};        // wait for all children to exit
    }
    return 0;
}

// Attempts to connect to the specified address on PORT_NUM
int client(const char *address){
    int socketfd;                               // socket file descriptor
    struct addrinfo serv, *actualData;          // address info for server
    memset(&serv, 0, sizeof(serv));             // zero out memory in struct
    int err;                                    // error holder

    serv.ai_socktype = SOCK_STREAM;             // set socktype to stream
    serv.ai_family = AF_INET;                   // set sock family to ipv4

    // Attempt to get get address info (dns lookup)
    err = getaddrinfo(address, "49999", &serv, &actualData);
    if( err ){
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(-err);
    }

    // Create socket with server
    socketfd = socket(actualData -> ai_family, actualData -> ai_socktype, 0);
    if( socketfd < 0){
        perror("socket");
        exit(-errno);
    }

    // Try to connect to server
    err = connect( socketfd, actualData -> ai_addr, actualData -> ai_addrlen);
    if( err < 0 ){
        perror("connect");
        exit(-errno);
    }

    // read and print to stdout
    int numRead;
    char buf[100] = {0};
    while((numRead = read(socketfd, buf, 100)) > 0){ // get data from socket
        err = write(1, buf, numRead);                // write data from socket to stdout
        if( err < 0 ){
            perror("write");
            exit(-errno);
        }
    } 
    return 0;
}
