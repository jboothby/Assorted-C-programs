#ifndef MFTP_H
#define MFTP_H

/* Header files common to server and client */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fdio.h>
#include <dirent.h>
#include <ctype.h>

/* ------------------- Defines and Globals ---------------------- */
#define PORTNUM 37896   // Port number for connection to server

#endif
