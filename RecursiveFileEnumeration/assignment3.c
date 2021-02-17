/*	Joseph Boothby
 *	CS360
 *	Assignment 3
 *
 *	This program will count the number of regular files readable by
 *	the current process in the the supplied directory and all child
 *	directories. If no directory is supplied, current working directory
 *	is assumed.
 */
/* --------------- Imports ------------ */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
/* --------- Function Prototypes -------*/
int enumFiles(char *inputPath);		// recursive function for enumerating files in a directory

/* Main function that handles edge cases from input and calls recursive function */
int readable(char *inputPath){

	// init buffer for path name
	char initialDirectory[PATH_MAX];

	// If there was a parameter supplied, determine if file or directory, act accordingly
	if( inputPath != NULL ){

		// Attempt to stat the supplied path, return error if not successful
		struct stat statp;
	       	if( lstat( inputPath, &statp ) < 0){
			return( -errno);	
		}

		// Check to see if path is a regular file
		if( S_ISREG(statp.st_mode) ){
			// Return 1 if file is readable, 0 if not
			if( access( inputPath, R_OK) == 0){
				return 1;
			}else{
				return 0;
			}
		}

		// Descend into starting directory.  return error if not successful
		if( chdir(inputPath) < 0 ){
			return( -errno);
		}

		// copy directory path into buffer
		getcwd(initialDirectory, 256);

	// if there was no parameter supplied, use cwd
	}else{
		getcwd(initialDirectory, 256);
	}

	// Check that initial directory is readable
	DIR *dirp = opendir(initialDirectory);
	if( dirp == NULL ){
		return( -errno);
	}
	closedir(dirp);

	// Call recursive function 'enumFiles'
	return enumFiles(initialDirectory);
}

/*	function to be called recursively. Counts number of files in directory that are
 *	readable to the current process
 */
int enumFiles(char *inputPath){

	DIR *dirp = opendir(inputPath);		// initialize directory pointer
	int count = 0;				// init counter for files 
	chdir(inputPath);			// descend into directory

	// skip empty directories or those that cannot be opened
	if( dirp == NULL ){
	      	return count;
	}

	struct dirent* direntp;			// initialize directory entry pointer
	errno = 0;				// reset errno
	//  loop until direntp is NULL (End-of-directory or error
	while( (direntp = readdir(dirp)) != NULL ){

		// skip "." and ".." directories
		if( strcmp( direntp->d_name, ".") == 0 || strcmp( direntp->d_name, "..") == 0){
			continue;
		}

		// if symbolic link, ignore and continue
		if( direntp->d_type == DT_LNK){
			continue;
		}

		// create a new absolute path using current path and filename of directory entry
		char entryPath[ strlen(inputPath) + strlen( direntp->d_name) + 2];
		strcpy(entryPath, inputPath);
		strcat(entryPath, "/");
		strcat(entryPath, direntp->d_name);

		// if reguar file, check for readable
		if( direntp->d_type == DT_REG){
			// only increment counter if file is readable
			if( access( entryPath, R_OK ) == 0){
				count++;
				continue;
			}
		}

		// if directory, recurse
		if( direntp->d_type == DT_DIR){
			count += enumFiles(entryPath);
		}
	}

	// close directory pointer and return count
	closedir(dirp);
	return count;
}
