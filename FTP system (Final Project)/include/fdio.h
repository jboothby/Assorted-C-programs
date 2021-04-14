#ifndef FDIO_H
#define FDIO_H

/* Write string to the fd, uses only the write system call              */
/* Prints error and exits with -1 on error                              */
void writeToFd(char* string, int fd);

/* Read from the supplied fd until a newline or EOF is detected         */
/* Saves data into dynamically allocated character array and returns    */
/* If FD is 0 (stdin), will prompt user for input                       */
/* Prints error and exits -1 on error, else returns character array     */
char* readFromFd(int fd);

#endif