/* This function uses a binary search on the supplied dictionary
   To attempt to find the supplied word. Length is the width of the
   dictionary. The program uses lseek to avoid reading more than one
   line of the dictionary into memory at a time.
   Params:
	dictionaryName: Filename of dictionary in local directory
	word:		Word to search dictionary for
	length: 	Width of each line in dictionary
   Returns:
	Line number of dictionary where word is found, OR
	Negative line number last searched in dictionary if word is not found, OR
	Errno is error occurs opening, reading, or seeking file.
*/
#ifndef OK_H
#define OK_H

int ok(char *dictionaryName, char *word, int length);

#endif
