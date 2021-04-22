/* Server side function handler of the FTP system */

#include <mftpserveHandler.h>
/* ------------ Function Prototypes ------------- */

int cd(int controlfd, char *path);                          // Change directory to path
int quit(int controlfd);                                    // Client disconnect, close fd
int ls(int controlfd, int datafd);                          // Execute the ls command, pipe the results into datafd
int get(int controlfd, int datafd, char* path);             // Get the file specified by path, write into the datafd
int put(int controlfd, int datafd, char* path);             // Create the file in the fd and save it locally
void error(int controlfd, int errnum);                      // Handle errors by printing to the controlfd


/* Execute chdir to the given path. Return 0 on success, -1 on error */
/* Returns 0 on success or -1 on error*/
int cd(int controlfd, char *path){

    int err;
    char currentDir[PATH_MAX];
    printf("Child <%d>: Received cd command with parameter <%s>\n", getpid(), path);

    // Make sure file has proper type and permissions
    if( (err = statfile(path, "dir", X_OK)) != 0 ){
        error(controlfd, err);
        return -1;
    }

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
    getcwd(currentDir, PATH_MAX);
    if( currentDir == NULL ){
        perror("Error");
    }

    printf("Child <%d>: Directory changed to %s\n", getpid(), currentDir);

    return 0;
}

/* Perform actions to disconnect client */
/* Returns 0 on success, -1 on error */
int quit(int controlfd){
    
    printf("Child <%d>: Exit command detected. Ending connection\n", getpid());

    // Close the control file descriptor, write acknowledge
    writeToFd(controlfd, "A\n");
    if( close(controlfd) < 0 ){
        error(controlfd, errno);
        return -1;
    }

    printf("Child <%d>: Exit success\n", getpid());

    return 0;
}

/* Execute the ls command on local. Send results through datafd */
/* Returns 0 on success or -1 on error*/
int ls(int controlfd, int datafd){
   int err; 
   char currentDir[PATH_MAX];
   int procId;

    printf("Child <%d>: Recieved ls command\n", getpid());

    // get current directory
    getcwd(currentDir, PATH_MAX);
    if( currentDir == NULL ){
        error(controlfd, errno);
        return -1;
    }

    // check that file type and permissions are correct
    if( (err = statfile(currentDir, "dir", R_OK)) != 0 ){
        error(controlfd, err);
        close(datafd);
        return -1;
    }

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

       printf("Child <%d>: Finished executing ls successfully\n", getpid());
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
    int actualRead, actualWrite, err;

    printf("Child <%d>: Recieved get command with parameter <%s>\n", getpid(), path);

    if( debug ){
        printf("Child <%d>: Getting file at <%s>\n", getpid(), path);
    }

    // check that file type and permissions are correct
    if( (err = statfile(path, "reg", R_OK)) != 0 ){
        error(controlfd, err);
        close(datafd);
        return -1;
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
    printf("Child <%d>: Finished transmitting file to client successfully\n", getpid());

    return 0;
}

/* Read file from datafd, create a file and save file locally */
/* returns 0 on success or -1 on error */
int put(int controlfd, int datafd, char* path){

    int filefd;
    char buf[256];
    int actualRead, actualWrite;

    printf("Child<%d>: Received put command with parameter <%s>\n", getpid(), path);

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
                perror("Error");
            }
            unlink(filename);
            close(filefd);
            return -1;
        }
    }
    // On read error, delete file
    if( actualRead < 0 ){
        if( debug ){
            perror("Error");
        }
        unlink(filename);
        close(filefd);
        return -1;
    }

    printf("Child <%d>: File transfer successful\n", getpid());

    return 0;
}

/* Handle error output properly */
void error(int controlfd, int errnum){
    // Send error output to client
    writeToFd(controlfd, "E");
    writeToFd(controlfd, strerror(errnum));
    writeToFd(controlfd, "\n");

    // Print errors on server side if debug output
    fprintf(stderr, "Process <%d>: Error: %s\n", getpid(), strerror(errnum));
}
