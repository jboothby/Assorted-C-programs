#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ok.h"

int main( int argc, char *argv[]){
	char wordBuffer[1024];
	char dictBuffer[1024];
	int width = 16;
	int retValue;

	while(1){
		printf("Please enter a dict and word to search for: ");
		fscanf(stdin, "%s %s",dictBuffer, wordBuffer);
		if( strcmp( dictBuffer, "exit") == 0) return 0;

		if( strncmp( dictBuffer, "tiny_9", 6 ) == 0){
			width = 9;
		}
		else{
			width = 16;
		}

		retValue = ok(strdup(dictBuffer), strdup(wordBuffer), width);

		if( retValue < 0 ){
			printf("Word Not Found, ");
		}else{
			printf("Word Found, ");
		}

		printf(" return value: %d\n", retValue );
	}
} 
