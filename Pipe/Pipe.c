/***********************************************************************
name: Joseph Boothby
	assignment4 -- acts as a pipe using ":" to seperate programs.
description:	
	See CS 360 Processes and Exec/Pipes lecture for helpful tips.
***********************************************************************/

/* Includes and definitions */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

/**********************************************************************
./assignment4 <arg1> : <arg2>

    Where: <arg1> and <arg2> are optional parameters that specify the programs
    to be run. If <arg1> is specified but <arg2> is not, then <arg1> should be
    run as though there was not a colon. Same for if <arg2> is specified but
    <arg1> is not.
**********************************************************************/
int main(int argc, char *argv[]){

	int fd[2];			/* file descriptors for pipe */
	int rdr, wtr;		 	/* Names for writer and reader ends of pipe */
	int midpoint = 0;		/* Location of colon in argv */

	/* Print error if not enough arguments */
	char *errMsg = "\nNot Enough Arguments\n\nUsage: ./assignment4 <arg1> : <arg2>\n\n";
	if( argc < 2 ){
		write( 2, errMsg, strlen(errMsg) );
		return(1);
	}

	/* find where the colon is (if any) */
	for( int i = 0; i < argc; i++){
		if( strcmp(argv[i], ":") == 0){
			midpoint = i;
			break;
		}
	}

	// slice argv around the colon into two subarrays
	int lsSize = midpoint == 0 ? argc : midpoint;
	char *ls[lsSize];		/* Left side */
	char *rs[argc - midpoint + 1];	/* Right side */

	for( int i = 1; i < argc; i++){
		if( i < midpoint || midpoint == 0){	/* If before midpoint, or there is no colon, fill left*/
			ls[i - 1] = argv[i];
		}else if( i > midpoint){		/* If after midpoint, fill right side */
			rs[i -( midpoint + 1)] = argv[i];
		}
	}

	/* NULL terminate the charcter arrays for execvp */
	ls[lsSize - 1] = NULL;
	rs[argc - midpoint] = NULL;

	/* Execute single program if only one argument supplied */
	if( rs[0] == NULL ){
		if( execvp( ls[0], ls ) == -1 )
			fprintf(stderr,"%s\n", strerror(errno));
		return 0;
	}
	if( ls[0] == NULL ){
		if( execvp( rs[0], rs ) == -1 )
			fprintf(stderr,"%s\n", strerror(errno));
		return 0;
	}

	/* initiate pipe and name the reader and writer ends */
	assert( pipe(fd) >= 0);
	rdr = fd[0]; wtr = fd[1];

	/* Fork program and exec both programs in parallel */
	if( fork() ){	/* parent is the receiving program */
		close(wtr);
		close(0); dup(rdr); close(rdr);	// make rdr point to stdin
		if( execvp(rs[0], rs) == -1 ) 		// exec right program
			fprintf(stderr,"%s\n", strerror(errno));
	}else{		/* child is the sending program */
		close(rdr);
		close(1); dup(wtr); close(wtr);	// make wtr point to stdout
		if( execvp(ls[0], ls) == -1 )		// exec left side
			fprintf(stderr,"%s\n", strerror(errno));
	}
}
