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

	// Check arguments and set start directory appropriately
	char initialDirectory[256];
	if( argc > 1 ){
		strncpy(initialDirectory, argv[1], strlen(argv[1]) + 1);
		printf("Starting at directory %s\n", initialDirectory);
	}else{
		getcwd(initialDirectory, 256);
		printf("Starting at directory %s\n", initialDirectory);
	}

	// Check that initial directory is readable
	DIR *dirp = opendir(initialDirectory);
	if( dirp == NULL ){
		fprintf(stderr, "Error opening directory: %s\n", initialDirectory);
		fprintf(stderr, "%s\n", strerror(errno));
		return( -errno);
	}

}


