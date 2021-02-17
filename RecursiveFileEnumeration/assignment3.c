/*	This program will count the number of regular files readable by
 *	the current process in the the supplied directory and all child
 *	directories. If no directory is supplied, current working directory
 *	is assumed.
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

int readable(char *inputPath);

int main(int argc, char *argv[]){

	char buf[256];

	if( argc > 1 ){
		printf("Starting at directory %s\n", argv[1]);
	}else{
		getcwd(buf, 256);
		printf("Starting at directory %s\n", buf);
	}
}


