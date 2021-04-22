/* Common functions needed for both mftp and mftpServe that relate to file i/o */
#ifndef FDIO_H
#define FDIO_H

/* Write string to the fd, uses only the write system call              */
/* Prints error and exits with -1 on error                              */
void writeToFd(int fd, char* string);

/* Read from the supplied fd until a newline or EOF is detected         */
/* Saves data into dynamically allocated character array and returns    */
/* If FD is 0 (stdin), will prompt user for input                       */
/* Prints error and exits -1 on error, else returns character array     */
char* readFromFd(int fd);

/* Split the supplied string around whitespace into an array of  two tokens */
/* Returns a string array where first string is the command, and second is parameter */
char** tokenSplit(char* string);

/* Stat the file and make sure that it exists, is of correct type, and */
/* Has the correct permissions. Path is the path to the file/directory  */
/* Type is either "dir" or "reg", perm is an integer created using a    */
/* bitwise OR of the macros R_OK, W_OK, X_OK as used in the access      */
/* System call. Return 0 on success, errno on error                     */
int statfile(char *path, char *type, int perm);
#endif
