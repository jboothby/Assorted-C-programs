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
	char initialDirectory[PATH_MAX];
	if( argc > 1 ){
		if( strlen(argv[1]) > PATH_MAX){
		       	fprintf(stderr,"Supplied directory name too long\n");
			return( -1 );
		}
		// Descend into startind directory. Print error and return if not successful
		chdir(argv[1]);
		if( chdir < 0 ){
			fprintf(stderr,"Error on directory %s\n", argv[1]);
			fprintf(stderr,"%s\n", strerror(errno));
			return( -errno);
		}
		// copy directory path into buffer
		getcwd(initialDirectory, 256);

	}else{
		// copy directory path into buffer
		getcwd(initialDirectory, 256);
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

/*	Method to be called recursively. Counts number of files in directory that are
 *	readable to the current process
 */
int readable(char *inputPath){

	DIR *dirp = opendir(inputPath);		// initialize directory pointer
	int count = 0;
	printf("Descending: %s\n", inputPath);
	chdir(inputPath);			// descend into directory

	// skip empty directories or those that cannot be opened
	if( dirp == NULL ){
		fprintf(stderr,"%s is NULL or cannot be opened\n", inputPath);
		fprintf(stderr,"%s\n", strerror(errno));
	      	return count;
	}

	struct dirent* direntp;			// initialize directory entry pointer
	//  loop until direntp is NULL (End-of-directory or error
	errno = 0;
	while( (direntp = readdir(dirp)) != NULL ){

		// skip "." and ".." directories
		if( strcmp( direntp->d_name, ".") == 0 || strcmp( direntp->d_name, "..") == 0){
				continue;
		}

		// if symbolic link, ignore and continue
		if( direntp->d_type == DT_LNK){
			continue;
		}

		// if reguar file, check for readable
		if( direntp->d_type == DT_REG){
			printf("Counting %s\n", direntp->d_name);
			count++;
			continue;
		}

		// if directory, recurse
		if( direntp->d_type == DT_DIR){
			// make new buffer for current path with directory name appended
			char newPath[ strlen(inputPath) + strlen( direntp->d_name) + 2];
			strcpy(newPath, inputPath);
			strcat(newPath, "/");
			strcat(newPath, direntp->d_name);
			count += readable(newPath);
		}
	}

	closedir(dirp);
	return count;

}


