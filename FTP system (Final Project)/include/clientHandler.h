/* Client side function handler of the FTP System */

#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <mftp.h>

/* ------------ Function Prototypes ------------- */
/* Disconnect from the server.                      */
/* Write Q command to server, then free tokens      */
/* Then close the fd, and exit                      */
void quit(int controlfd, char** tokens);

/* Change the local directory to the path.          */
/* Report acknowledgement or error to controlfd     */
/* Return 0 on success, -1 on error                 */
int cdLocal(char* path);

/* Change the server directory to the path.         */
/* Get acknowledgement or error to controlfd        */
/* Return 0 on success, -1 on error                 */
int cdRemote(const char* path, int controlfd);

/* Fork off a new process to execute the linux ls   */
/* command. Pipe the output from ls into more -20   */
/* Return 0 on success, -1 on error                 */
int lsLocal();

/* Print the contents of the server cwd to stdout   */
/* Open a data connection, then read from that fd   */
/* And pipe all of the results to more to print to  */
/* Stdout. Get error or success from server         */
/* Return 0 on success, -1 on error                 */
int lsRemote(const char* hostname, int controlfd);

/* Read the file at path, write into the datafd     */
/* datafd is obtained from controlfd inside function*/
/* Report acknowledgment or error to controlfd      */
/* Return 0 on success, -1 on error                 */
int put(char* path, const char *hostname, int controlfd);

/* Read the contents of this file from datafd       */
/* If save flag is not set, display contents with   */
/* a pipe to more -20. If save flag is set then     */
/* Create the file with filename at path            */
/* Write the file into the newly created one        */
/* Report acknowledgment or error to controlfd      */
/* Return 0 on success, -1 on error                 */
int get(char* path, const char *hostname, int controlfd, int save);

/* Forks off a new process to handle the following: */
/* Redirect the datafd into stdin, then exec the    */
/* Linux more -20 command to print the information  */
/* stdout 20 lines at a time                        */
/* returns 0 on succss, -1 on error                 */
int morePipe(int datafd);

/* Create a connection with the host at address     */
/* Using port pnum. If pnum is zero, it will use    */
/* a random available port for the connection       */
/* returns the file descripter for the socket on    */
/* success, or -1 on error                          */
int attemptConnection( const char *address, int pnum);

/* Create a data connection with the server         */
/* This is a wrapper for attemptConnection and      */
/* will use 0 to specify a random port number       */
/* Then writes this port number to the controlfd    */
/* so that the server can connect to it             */
/* returns the file descripter for the socket on    */
/* success, or -1 on error                          */
int makeDataConnection(const char* hostname, int controlfd);

#endif

