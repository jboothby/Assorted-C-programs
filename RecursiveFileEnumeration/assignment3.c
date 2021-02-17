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
		if( strlen(argv[1]) > 255){
				fprintf(stderr,"Supplied directory name too long\n");
				return( -1 );
		}
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
	closedir(dirp);

	// Call recursive function 'readable'
	printf("There are %d readable files in the file structure starting at %s\n",
			readable(initialDirectory), initialDirectory);

}

int readable(char *inputPath){

	DIR *dirp = opendir(inputPath);
	int count = 0;

	// skip empty directories or those that cannot be opened
	if( dirp == NULL ){
	      return count;
	}

	struct dirent* direntp;
	//  loop until direntp is NULL (End-of-directory or error
	errno = 0;
	while( (direntp = readdir(dirp)) != NULL ){

		// skip "." and ".." directories
		if( strcmp( direntp->d_name, ".") == 0 || strcmp( direntp->d_name, "..") == 0){
				printf("Skipping %s", direntp->d_name);
				continue;
		}


	}
	closedir(dirp);
	return count;

}


