/* Server side function handler of the FTP system */

#ifndef SERVERHANDLER_H
#define SERVERHANDLER_H

#include <mftp.h>
/* ------------ Structures --------------------- */
typedef struct connectData{       // Holds data about a connection
    int listenfd;                 // file descriptor for accept
    int portnum;                  // Port for connection
    int errnum;                   // Value of errno if error occurs
}connectData;

/* ------------ Function Prototypes ------------- */

/* Change the local directory to the path.          */
/* Report acknowledgement or error to controlfd     */
/* Return 0 on success, -1 on error                 */
int cd(int controlfd, char *path);

/* Disconnect from the client.                      */
/* Report acknowledgment or error to controlfd      */
/* Then close the fd.                               */
/* Return 0 on success, -1 on error                 */
int quit(int controlfd);

/* Fork off a new process to execute the linux ls   */
/* command. Pipe the output from ls into the datafd */
/* Then report success or error to controlfd        */
/* Return 0 on success, -1 on error                 */
int ls(int controlfd, int datafd);

/* Read the file at path, write into the datafd     */
/* Report acknowledgment or error to controlfd      */
/* Return 0 on success, -1 on error                 */
int get(int controlfd, int datafd, char* path);

/* Create the file with filename at path            */
/* Read the contents of this file from datafd       */
/* Write the file into the newly created one        */
/* Report acknowledgment or error to controlfd      */
/* Return 0 on success, -1 on error                 */
int put(int controlfd, int datafd, char* path);

/* Write error with value errnum to controlfd       */
/* Write this error to sdout if debug flag true     */
void error(int controlfd, int errnum);

#endif
