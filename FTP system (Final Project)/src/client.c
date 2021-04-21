/* Client side of the FTP System. See header for functionality */
#include <clientHandler.h>

/* --------------------- Function Prototypes -------------------- */
int processCommands(const char* hostname, int controlfd);   // Process commands
char* parseArgs(int c, char** v);                           // Parse command line arguments

/* Handles program control */
int main(int argc, char* argv[]){

    const char* hostname;
    int controlfd;

    hostname = parseArgs(argc, argv);
    controlfd = attemptConnection(hostname, PORTNUM);

    printf("\nConnected to host %s on port %d\n", hostname, PORTNUM);

    printf("\nWelcome to the MFTP system. Your available commands are as follows\n\n\
            exit            - terminate all child processes of the server and then exit\n\
            cd <pathname>   - change the current working directory to <pathname>\n\
            rcd <pathname>  - change the current working directory of the server to <pathname>\n\
            ls              - execute the ls -l command on the client, show output 20 lines at a time. press space to show more\n\
            rls             - execute the ls -l command on the server, show output 20 lines at a time. press space to show more\n\
            get <pathname>  - retrieve <pathname> from the server and store it locally in current directory. \n\
            show <pathname> - retrive the contents of <pathname> and show 20 lines at a time. Press space to show more.\n\
            put <pathname>  - transmite the contents of <pathname> to server and store in server cwd. Must be a regular file and readable.\n");
    
    processCommands(hostname, controlfd);

    return 0;
}

/* Get commands from user and process them appropriately */
int processCommands(const char* hostname, int controlfd){

    char *command;                          // Holds return from readFromFd for stdin
    char **tokens = NULL;                   // Result of splitting command into tokens

    // Inifite loop, breaks on exit command
    for(;;){

        // Read command from stdin, then split into tokens
        command = readFromFd(0);
        if( debug ) printf("Command string '%s'\n", command);
        if( (tokens = tokenSplit(command)) == NULL){
            free(command);
            continue;
        }
        free(command);

        /* EXIT COMMAND EXECUTION BLOCK */
        if( strcmp(tokens[0], "exit") == 0 ){

            if( debug) printf("Executing exit command\n");
            quit(controlfd, tokens);


        /* CD COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "cd") == 0){

            if( debug) printf("Executing command <cd> with parameter <%s>\n", tokens[1]);

            if( cdLocal(tokens[1]) < 0  && debug){
                writeToFd(2, "cd command did not execute properly\n");
            }

        /* RCD COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "rcd") == 0){

            if( debug) printf("Executing command <rcd> with parameter <%s>\n", tokens[1]);

            if( cdRemote(tokens[1], controlfd) < 0  && debug){
                writeToFd(2, "rcd command did not execute properly\n");
            }

        /* LS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "ls") == 0){

            if( debug) printf("Executing command <ls>\n");

            if( lsLocal() < 0  && debug){
                writeToFd(2, "ls command did not execute properly\n");
            }

        /* RLS COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "rls") == 0){

            if( strlen(tokens[1]) > 0 ){
                printf("Too many tokens, rls takes no parameters\n");
                continue;
            }

            if( debug) printf("Executing command <rls>\n");

            if( lsRemote(hostname, controlfd) < 0 && debug){
                writeToFd(2, "rls command did not execute properly\n");
            }

        /* GET COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "get") == 0){

            if( debug) printf("Executing command <get> with parameter <%s>\n", tokens[1]);

            // Call get with save flag set to 1
            if( get(tokens[1], hostname, controlfd, 1) < 0 && debug){
                writeToFd(2, "Get command did not execute properly\n");
            }

        /* SHOW COMMAND EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "show") == 0){

            if( debug) printf("Executing command <show> with parameter <%s>\n", tokens[1]);

            // Call get with the save flag set to 0
            if( get(tokens[1], hostname, controlfd, 0) < 0 && debug){
                writeToFd(2, "Show command did not execute properly\n");
            }

        /* PUT EXECUTION BLOCK */
        }else if( strcmp(tokens[0], "put") == 0){

            if( debug) printf("Executing command <put> with parameter <%s>\n", tokens[1]);

            // Call put with the path, hostname, and controlfd
            if( put(tokens[1], hostname, controlfd) < 0 && debug){
                writeToFd(2, "Put command did not execute properly\n");
            }


        }else{
            printf("Command %s not recognized, ignoring\n", tokens[0]);

        }

        free(tokens[0]);
        free(tokens[1]);
        free(tokens);
    }
    return 0;
}

/* Parse through command line arguments. Set debug flag if needed, return hostname argument */
char* parseArgs(int argnum, char** arguments){
    char *usageMsg = "Usage:\n"
                    "\t./client <-d> hostname\n"
                    "\n\t-d: optional debugging flag for verbose output\n"
                    "\thostname: symbolic hostname or IPv4 address to the server\n";

    // If there are not enough arguments, error out
    if( argnum < 2 ){
        fprintf(stderr, "%s\n", usageMsg);
        exit(-1);
    }

    // If the first arg is -d and there are 2 args
    if( strcmp( arguments[1], "-d") == 0) {
        if( argnum == 3 ){
            debug = 1;
            printf("Parent: Debug output enabled\n");
            return(arguments[2]);
        }else{
            fprintf(stderr, "%s\n", usageMsg);
            exit(-1);
        }
    }

    // If there is only 1 argument return it as hostname
    if( argnum == 2 ){
        return(arguments[1]);
    }

    fprintf(stderr, "%s\n", usageMsg);
    exit(-1);
}
