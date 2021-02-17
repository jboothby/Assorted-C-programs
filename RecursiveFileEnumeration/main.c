// simple driver for assignment3
#include "assignment3.h"
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[]){
	char buffer[256];
	char cwd[256];
	getcwd(cwd,256);
	for(;;){
		printf("Please enter the location to run readable on: ");
		scanf("%s", buffer);
		printf("\nThere are %d readable files under this location\n", readable(buffer));
		chdir(cwd);
	}
}
	
