/***********************************************************************
name: Joseph Boothby
	ok -- see if a word is in the online dictionary
description:	
	See CS 360 IO lecture for details.
***********************************************************************/

/* Includes and definitions */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/**********************************************************************
Search for given word in dictionary using file descriptor fd.
Return the line number the word was found on, negative of the last line searched
if not found, or the error number if an error occurs.
**********************************************************************/
int ok(char *dictionaryName, char *word, int length) {

	/* Declare variables */
	int fd, EOFoffset, currentOffset, currentLine, got, upper, lower, lineFound;
	char buffer[length];	
	char wordCopy[length];

	/* Write word into a copy array so it can be space padded and truncated */
	// copy chars from word into wordCopy until end of word, or length
	for( int i = 0; i < strlen(word) && i < length; i++){
		wordCopy[i] = word[i];
	}
	// fill remaining room in wordCopy with spaces to match dictionary format */
	for( int i = strlen(word); i < length; i++){
		wordCopy[i] = ' ';
	}

	/* open the dictionary file in read-only mode, then error check */
	fd = open(dictionaryName, O_RDONLY);
	if( fd < 0 ){
		fprintf( stderr, "%s\n", strerror(errno));
		return(errno);
	}

	/* Get length of file with lseek, then reset file offset */
	EOFoffset = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	/* Set upper and lower which are the upper and lower limit of line count left to check */
	lower = 1;
	upper = EOFoffset / length;	// total bytes divided by width gives line numbers

	/* Loop until word is found, or no lines left to check (lineFound will be negative last line checked) */
	lineFound = 0;
	while( !lineFound ){

		/* Calculate offset based on lower and upper line limits */
		currentLine = ( ( upper + lower ) / 2 );
		currentOffset = ( currentLine - 1 ) * length;// becuase offset is 0 indexed, but line is not

		/* seek to proper offset */
		lseek( fd, currentOffset, SEEK_SET );
		
		/* Read next line in file, return if error */
		got = read( fd, buffer, length );
		if( got < 0){
			fprintf( stderr, "%s\n", strerror(errno));
			close(fd);
			return errno;
		}

		/* Compare current line with word, break if equal */
		if( strncmp( buffer, wordCopy, length - 1 ) == 0 ){ // length - 1 to ignore linefeed
			lineFound = currentLine;
			break;
		}

		/* If current line is not equal to word, compare to determine
		 * Which direction to jump for the binary search. Iterate
		 * through characters in wordCopy and buffer until char that
		 * does not match is found. If wordCopy is less, then jump halfway
		 * between lower and upper and update upper to be current line
		 * do opposite if wordCopy is more */
		for( int i = 0; i < length; i++ ){

			/* If buffer letter is larger, then jump down */
			if( buffer[i] > wordCopy[i] ){
				if( lower == currentLine ){		// if there is no more to go down, then word is not in dictionary
					lineFound = -currentLine;
				}
				else{					// otherwise, set upper to current to jump down halfway
					upper = currentLine;
				}
				break;					// break for loop and continue to next word
			}
			/* If buffer letter is smaller, then jump up */
			else if( buffer[i] < wordCopy[i] ){
				if( upper == currentLine ){		// if there there is no more to go up, then word is not in dictionary
					lineFound = -currentLine;
				}
				else if( upper - lower == 1){		// if the distance bewteen lower and upper is 1, set lower to upper
					lower = upper;			// this avoids problems with integer division never rounding up
				}
				else{					// otherwise, set lower to current to jump up halfway
					lower = currentLine;
				}
				break;					// break for loop and continue to next word
			}
		}
	}
					
	close( fd );		// close file
	return lineFound;	// this will contain line found, or negative value of last line searched
}
