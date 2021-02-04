#include <stdio.h>
#include <string.h>
#include "ok.h"

int main( int argc, char *argv[]){
	char buffer[1024];
	int retValue;

	while( strcmp( buffer, "exit") != 0){
		printf("Please enter a word to search webster_16 for: ");
		fscanf(stdin, "%s", buffer);
		retValue = ok("webster_16", strdup(buffer), 16);
		if( retValue < 0 ){
			printf("Word Not Found, ");
		}else{
			printf("Word Found, ");
		}
		printf(" return value: %d\n", retValue );
	}
} 
