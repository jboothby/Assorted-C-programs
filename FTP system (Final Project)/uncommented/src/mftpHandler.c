

#include <mftpHandler.h>

int lsLocal(){
    int pipefd[2];
    int err, rdr, wtr;

    
    if( fork() ){
        wait(&err);
        if( err < 0 ){
            perror("Error");
            return -1;
        }
    }else{

        
        if( pipe(pipefd) < 0 ){
            perror("Error");
            return -1;
        }
        rdr = pipefd[0]; wtr = pipefd[1];

        if( fork() ){   
            close(wtr);
            close(0); dup(rdr); close(rdr);     
            if( execlp("more", "more",  "-20", (char *) NULL) == -1){
                perror("Error");
                return -1;
            }
        }else{          
            close(rdr);
            close(1); dup(wtr); close(wtr); 
            if( execlp("ls", "ls", "-l", "-a", (char *) NULL) == -1){
                perror("Error");
                return -1;
            }
        }
    }

    return 0;
}


int lsRemote(const char* hostname, int controlfd){
    char* serverResponse;
    int datafd;

    
    if( (datafd = makeDataConnection(hostname, controlfd)) > 0){

        
        writeToFd(controlfd, "L\n");
        if( debug ) printf("Sent command L to server\n");

        serverResponse = readFromFd(controlfd);
        if( debug ) printf("Received server response '%s'\n", serverResponse);

        
        if( serverResponse[0] == 'E'){
            fprintf(stderr, "%s\n", serverResponse + 1);
            free( serverResponse );
            close(datafd);
            return -1;
        }
        free(serverResponse);

        
        if( morePipe(datafd) < 0){
            return -1;
        }else{
            return 0;
        }

    }
    return -1; 
}



int cdLocal(char* path){
       
            int err;
            char currentDir[PATH_MAX];

            
            if( (err = statfile(path, "dir", X_OK)) != 0 ){
                printf("Error: %s\n", strerror(err));
                return -1;
            }

            
            err = chdir(path);
            if( err < 0 ){
                perror("Error");
                return -1;
            }

            
            getcwd(currentDir, PATH_MAX);
            if( currentDir == NULL){
                perror("Error");
                return -1;
            }
            printf("Directory changed to %s\n", currentDir);

            return 0;
}



int cdRemote(const char* path, int controlfd){
    char *serverResponse;

    
    char cdWithPath[strlen(path) + 3];    
    sprintf(cdWithPath, "C%s\n", path);

    
    writeToFd(controlfd, cdWithPath);
    if( debug ) printf("Send command %s to server\n", cdWithPath);

    
    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Received server response '%s'\n", serverResponse);

    if( serverResponse[0] == 'E' ){
        fprintf(stderr, "Error: %s\n", serverResponse + 1);
        free( serverResponse );
        return -1;
    }

    printf("Server directory changed to %s\n", path);

    free(serverResponse);
    return 0;
}



int get(char* path, const char *hostname, int controlfd, int save){
    char* serverResponse;
    int datafd, outputfd;
    char buf[256];
    int actualRead, actualWrite;
    int returnStatus = 0;

    
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = (strrchr(path, '/') + 1);
    }else{
        filename = path;
    }

    
    char getWithPath[strlen(path) + 3];
    sprintf(getWithPath, "G%s\n", path);

    
    if( (datafd = makeDataConnection(hostname, controlfd)) < 0){
        close(datafd);
        return -1;
    }

    
    writeToFd(controlfd, getWithPath);
    if( debug ) printf("Sent %s command to server\n", getWithPath);

    
    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Received server response '%s'\n", serverResponse);

    if( serverResponse[0] == 'E'){
        fprintf(stderr, "Error: %s\n", serverResponse + 1);
        free(serverResponse);
        return -1;
    }
    free(serverResponse);

    
    if(!save){
        if( debug ) printf("Save flag not set, displaying contents\n");
        if( morePipe(datafd) < 0 ){
            return -1;
        }else{
            return 0;
        }
    }
        
    if( debug ) printf("Save flag set, saving file\n");

    
    outputfd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0700); 
    if( outputfd < 0 ){
        perror("Error");
        close(datafd);
        return -1;
    }

    
    while( (actualRead = read(datafd, buf, sizeof(buf)/ sizeof(char))) > 0){
        actualWrite = write(outputfd, buf, actualRead);
        
        if( actualWrite < 0){
            perror("Error");
            returnStatus = -1;
            break;
        }
    }
    
    if( actualRead < 0 ){
        perror("Error");
        returnStatus = -1;
    }

    
    if( returnStatus == -1 ){
        printf("Error during file transfer, unlinking file '%s'\n", filename);
        unlink(filename);
    }

    
    close(outputfd);
    close(datafd);

    printf("Data transfer complete, saved file %s\n", filename);

    return returnStatus;
}


int put(char* path, const char *hostname, int controlfd){
    char *serverResponse;
    char buf[256];
    int filefd, datafd;
    int actualRead, actualWrite, err;

    
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = (strrchr(path, '/') + 1);
    }else{
        filename = path;
    }

    
    if( (err = statfile(path, "reg", R_OK)) != 0 ){
        printf("Error: %s\n", strerror(err));
        return -1;
    }

    
    char putWithPath[strlen(path) + 3];
    sprintf(putWithPath, "P%s\n", filename);

    
    if( (datafd = makeDataConnection(hostname, controlfd)) < 0){
        close(datafd);
        return -1;
    }

    if( debug ) printf("Opening file '%s'\n", path);

    
    filefd = open(path, O_RDONLY);
    if( filefd < 0){
        perror("Error");
        close(datafd);
        return -1;
    }

    if( debug ) printf("File opened, writing contents into fd %d\n", datafd);

    
    while((actualRead = read(filefd, buf, sizeof(buf)/sizeof(char))) > 0){
        actualWrite = write(datafd, buf, actualRead);
        if( actualWrite < 0){
            perror("Error");
            close(filefd);
            close(datafd);
            return -1;
        }
    }
    if(actualRead < 0){
        perror("Error");
        close(filefd);
        close(datafd);
        return -1;
    }

    if( debug ) printf("Finished writing file into fd %d\n", datafd);

    
    close(datafd);

    
    writeToFd(controlfd, putWithPath);
    if( debug ) printf("Sent command %s to server\nWaiting on server response", putWithPath);

    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Received '%s' from server\n", serverResponse);

    if (serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free(serverResponse);
        close(filefd);
        return -1;
    }

    printf("Finished tranferring file %s to server\n", filename);

    
    free(serverResponse);
    close(filefd);

    return 0;

}


int morePipe(int datafd){

    int err;
    int procId = fork();

    
    if( procId ){
        close(datafd);

        if( debug ) printf("Forked off process %d to handle more\n", procId);

        
        wait(&err);
        if( err < 0){
            perror("Error");
            return -1;
        }

        if( debug ) printf("More command completed\n");

        return 0;
    }else{
        
        close(0); dup(datafd); close(datafd);

        
        if( execlp("more", "more", "-20", (char*) NULL) == -1 ){
            perror("Error");
        }
    }

    return 0;
}


void quit(int controlfd, char** tokens){

            char* serverResponse;

            
            writeToFd(controlfd, "Q\n");
            serverResponse = readFromFd(controlfd);
            if( serverResponse[0] == 'E'){
                printf("Error: %s\n", serverResponse + 1);
            }
            
            
            free(serverResponse);
            free(tokens[0]);
            free(tokens[1]);
            free(tokens);

            
            close(controlfd);

            printf("Exiting...\n");

            exit(0);
}


int attemptConnection( const char* address, int pnum){
    int sockfd;                             
    int err;                                

    struct addrinfo serv, *actualData;      
    memset(&serv, 0, sizeof(serv));         

    serv.ai_socktype = SOCK_STREAM;         
    serv.ai_family = AF_INET;   
    
    char port[6];                        
    sprintf(port, "%d", pnum);           
    
    
    err = getaddrinfo(address, port, &serv, &actualData);
    if( err ){
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        exit(-1);
    }

    
    sockfd = socket(actualData -> ai_family, actualData -> ai_socktype, 0);
    if( sockfd < 0 ){
        perror("Error");
        exit(-1);
    }

    if( debug ) printf("Created socket with descripter %d\n", sockfd);

    
    err = connect( sockfd, actualData -> ai_addr, actualData -> ai_addrlen);
    if( err < 0 ){
        perror("Error");
        exit(-1);
    }

    if( debug ) printf("Connected to %s on port %d\n", address, pnum);

    freeaddrinfo(actualData);

    return sockfd;
}


int makeDataConnection(const char* hostname, int controlfd){
    char* serverResponse;
    char* substr;
    int datafd;

    
    writeToFd(controlfd, "D\n");
    if( debug ) printf("Sent D command to server\nAwaiting server response\n");

    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Recieved server response '%s'\n", serverResponse);

    
    if ( serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free( serverResponse );
        return(-1);
    }else{
        
        substr = serverResponse + 1;
        if( debug ) printf("Attempting connection on portnum: <%s>\n", substr);

        
        datafd = attemptConnection(hostname, atoi(substr)); 
        if( debug ) printf("Data connection successful\n");
        free(serverResponse);
    }
    return datafd;
}
