/* Function handlers for the client side connection of the FTP */

#include <clientHandler.h>

/* Executes ls -l | more -20 on client side */
int lsLocal(){
    int pipefd[2];
    int err, rdr, wtr;

    // Fork program to execute ls and more
    if( fork() ){// Parent just waits on children to exit
        wait(&err);
        if( err < 0 ){
            perror("Error");
            return -1;
        }
    }else{// Child process forks again to run each program

        // Initiate pipe and rename reader and writer sides
        if( pipe(pipefd) < 0 ){
            perror("Error");
            return -1;
        }
        rdr = pipefd[0]; wtr = pipefd[1];

        if( fork() ){   // Execute more -20 in the parent
            close(wtr);
            close(0); dup(rdr); close(rdr);     // make stdin go to reader
            if( execlp("more", "more",  "-20", (char *) NULL) == -1){
                perror("Error");
                return -1;
            }
        }else{          // Execute ls -l in the child
            close(rdr);
            close(1); dup(wtr); close(wtr); // make stdout go to writer
            if( execlp("ls", "ls", "-l", "-a", (char *) NULL) == -1){
                perror("Error");
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
        if( debug ) printf("Sent command L to server\n");

        serverResponse = readFromFd(controlfd);
        if( debug ) printf("Received server response '%s'\n", serverResponse);

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

/* Execute the cd command on server */
/* Returns 0 on success, -1 on error */
int cdRemote(const char* path, int controlfd){
    char *serverResponse;

    // Create command string using path
    char cdWithPath[strlen(path) + 3];    // for C, Path, \n, \0
    sprintf(cdWithPath, "C%s\n", path);

    // Write command to server
    writeToFd(controlfd, cdWithPath);
    if( debug ) printf("Send command %s to server\n", cdWithPath);

    // Grab server response
    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Received server response '%s'\n", serverResponse);

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
    if( debug ) printf("Sent %s command to server\n", getWithPath);

    // Get server response, print error if exists
    serverResponse = readFromFd(controlfd);
    if( debug ) printf("Received server response '%s'\n", serverResponse);

    if( serverResponse[0] == 'E'){
        fprintf(stderr, "%s\n", serverResponse + 1);
        free(serverResponse);
        return -1;
    }
    free(serverResponse);

    // If the save flag is not set, pass this off to more pipe
    if(!save){
        if( debug ) printf("Save flag not set, displaying contents\n");
        if( morePipe(datafd) < 0 ){
            return -1;
        }else{
            return 0;
        }
    }
        
    if( debug ) printf("Save flag set, saving file\n");

    // Create new file and open it
    outputfd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0700); 
    if( outputfd < 0 ){
        perror("Error");
        close(datafd);
        return -1;
    }

    // Read file from datafd, write to new file
    while( (actualRead = read(datafd, buf, sizeof(buf)/ sizeof(char))) > 0){
        actualWrite = write(outputfd, buf, actualRead);
        // on write error, delete file, and return -1
        if( actualWrite < 0){
            perror("Error");
            returnStatus = -1;
            break;
        }
    }
    // On read error, delete file and return
    if( actualRead < 0 ){
        perror("Error");
        returnStatus = -1;
    }

    // If there was an error, delete the file we just created
    if( returnStatus == -1 ){
        if( debug ) printf("Error during file transfer, unlinking file '%s'\n", filename);
        unlink(filename);
    }

    // Close the file descriptors
    close(outputfd);
    close(datafd);

    printf("Data transfer complete\n");

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

    if( debug ) printf("Opening file '%s'\n", path);

    // Write contents of local file at path into datafd
    filefd = open(path, O_RDONLY);
    if( filefd < 0){
        perror("Error");
        close(datafd);
        return -1;
    }

    if( debug ) printf("File opened, writing contents into fd %d\n", datafd);

    // Read data frome file, write to datafd
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

    // Close data fd to signal transfer complete
    close(datafd);

    // Write command to server
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

    // If no errors, return 0 for success
    free(serverResponse);
    close(filefd);

    return 0;

}

/* Pipe the results from the data fd into the more command */
int morePipe(int datafd){

    int err;
    int procId = fork();

    // Fork off to pipe the datafd information into stdin for more
    if( procId ){
        close(datafd);

        if( debug ) printf("Forked off process %d to handle more\n", procId);

        // Wait for child (more -20) to finish
        wait(&err);
        if( err < 0){
            perror("Error");
            return -1;
        }

        if( debug ) printf("More command completed\n");

        return 0;
    }else{
        // Redirect data socket to stdin
        close(0); dup(datafd); close(datafd);

        // Send everything in the datafd to more
        if( execlp("more", "more", "-20", (char*) NULL) == -1 ){
            perror("Error");
        }
    }

    return 0;
}

/* Change the directory on local machine to path */
/* Return 0 on success, or -1 on error           */
int cdLocal(char* path){
       
            int err;

            char currentDir[PATH_MAX];

            // Change directory
            err = chdir(path);
            if( err < 0 ){
                perror("Error");
                return -1;
            }

            // Output change if debug flag set
            if( debug ){
                getcwd(currentDir, PATH_MAX);
                if( currentDir == NULL){
                    perror("Error");
                    return -1;
                }
                printf("Directory changed to %s\n", currentDir);
            }

            return 0;
}

/* Send exit command to server, then free all memory and exit */
void quit(int controlfd, char** tokens){

            char* serverResponse;

            // Write command to server, and read response 
            writeToFd(controlfd, "Q\n");
            serverResponse = readFromFd(controlfd);
            if( serverResponse[0] == 'E'){
                printf("Error: %s\n", serverResponse + 1);
            }
            
            // Free allocated memory
            free(serverResponse);
            free(tokens[0]);
            free(tokens[1]);
            free(tokens);

            // Close socket
            close(controlfd);

            printf("Exiting...\n");

            exit(0);
}
