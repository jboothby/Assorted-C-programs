

#include <mftpserveHandler.h>


int cd(int controlfd, char *path);                          
int quit(int controlfd);                                    
int ls(int controlfd, int datafd);                          
int get(int controlfd, int datafd, char* path);             
int put(int controlfd, int datafd, char* path);             
void error(int controlfd, int errnum);                      




int cd(int controlfd, char *path){

    int err;
    char currentDir[PATH_MAX];

    
    if( (err = statfile(path, "dir", X_OK)) != 0 ){
        error(controlfd, err);
        return -1;
    }

    
    err = chdir(path);
    if( err < 0 ){
        
        error(controlfd, errno);
        return -1;
    }else{
        
        writeToFd(controlfd, "A\n");
    }

    
    if( debug ){
        getcwd(currentDir, PATH_MAX);
        if( currentDir == NULL ){
            perror("getcwd");
        }
        printf("Child <%d>: Directory changed to %s\n", getpid(), currentDir);
    }

    return 0;
}



int quit(int controlfd){
    
    if( debug ){
        printf("Child <%d>: Exit command detected. Ending connection\n", getpid());
    }

    
    writeToFd(controlfd, "A\n");
    if( close(controlfd) < 0 ){
        if( debug ){
            perror("close");
        }
        return -1;
    }

    return 0;
}



int ls(int controlfd, int datafd){
   int err; 
   int procId;

   
   procId = fork();
   
   if( procId ){

       close( datafd ); 

       if( debug ){
           printf("Child <%d>: spawned new process <%d> to handle ls\n", getpid(), procId);
       }

       wait(&err);      

       
       writeToFd(controlfd, "A\n");

       if( debug ){
           printf("Child <%d>: Finished executing ls\n", getpid());
       }
   }else{
       
       close(1); dup(datafd); close(datafd);

       
       if( execlp("ls", "ls", "-l", "-a", (char *) NULL) == -1){
           error(controlfd, errno);
           return -1;
       }

   }

   return 0;
}



int get(int controlfd, int datafd, char* path){
    int filefd;
    char buf[256];
    int actualRead, actualWrite, err;

    if( debug ){
        printf("Child <%d>: Getting file at <%s>\n", getpid(), path);
    }

    
    if( (err = statfile(path, "reg", R_OK)) != 0 ){
        error(controlfd, err);
        close(datafd);
        return -1;
    }

    
    filefd = open(path, O_RDONLY);
    if( filefd < 0 ){
        error(controlfd, errno);
        close(datafd);
        return -1;
    }

    
    writeToFd(controlfd, "A\n");

    
    while((actualRead = read(filefd, buf, sizeof(buf)/sizeof(char))) > 0){
        
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



int put(int controlfd, int datafd, char* path){

    int filefd;
    char buf[256];
    int actualRead, actualWrite;

    
    char *filename;
    if( strchr(path, '/') != NULL ){
        filename = strrchr(path, '/') + 1;  
    }else{
        filename = path;
    }


    if( debug ){
        printf("Child <%d>: Creating file <%s>, saving to local\n", getpid(), filename);
    } 

    
    filefd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0700);
    if( filefd < 0 ){
        error(controlfd, errno);
        close(filefd);
        return -1;
    }

    
    writeToFd(controlfd, "A\n");

    
    while( (actualRead = read(datafd, buf, sizeof(buf)/sizeof(char))) > 0){
        actualWrite = write(filefd, buf, actualRead);
        
        if( actualWrite < 0 ){
            if( debug ){
                perror("write");
            }
            unlink(filename);
            close(filefd);
            return -1;
        }
    }
    
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


void error(int controlfd, int errnum){
    
    writeToFd(controlfd, "E");
    writeToFd(controlfd, strerror(errnum));
    writeToFd(controlfd, "\n");

    
    if( debug ){
        printf("Process <%d>: Error: %s\n", getpid(), strerror(errnum));
    }
}
