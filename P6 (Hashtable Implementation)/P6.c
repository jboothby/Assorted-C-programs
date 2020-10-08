//Joseph Boothhby 
//This program will allow a person to insert, delete, search, and print
//a hash table
//Any time i use (buffer+3), buffer is an array, and the +3 is some pointer manipulation to igore the first three characters of the array
//An array's name is a pointer to the first element in the array, so array+3 is a pointer to the 4th element.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LENGTH	20	//length of hash table

typedef struct word{
	char *string;	//holds the char array that will be a word, must be malloc'd
	int deleted;
	int origKey;
}word;

struct word hashtable[LENGTH];	//globally defined hashtable

/*-----------------------------------------------------------------*/
//Function Prototypes
void getInput();		//receive and parse inputs
void print();			//display the hashtable
int search(char buffer[1024]);	//search for a word in the hashtable, returns -1 if not found, or location in hashtable if found.
void insert(char buffer[1024]);	//insert a word into the hashtable
void delete(char buffer[1024]);	//delete a word from the hashtable
int key(char buffer[1024]);	//returns a value from the hash function
void quit();			//free's all unfreed data and exits the program
void help();			//display the help menu
/*-----------------------------------------------------------------*/
//This function will receive and parse inputs
void getInput(){
	char buffer[1024];
	char mode=0;
	char letter=0;
	
	printf("\n>");

	fgets(buffer,1024,stdin);
	if(buffer[strlen(buffer)-1]=='\n')	//remove trailing newline if it exists
		buffer[strlen(buffer)-1]='\0';
	

	mode=buffer[0];		//stores first char to look for #
	letter=buffer[1];	//stores second char to look for what's after #
	

	if(mode=='#'){		//if first char is a #
		switch(letter){
			case 'h':	//if second char is h
				help();
				break;
			case 'd':	//if second char is a d
				delete(buffer+3);
				break;
			case 's':	//if second char is an s
					//send our buffer, minus the first three chars (#s )to the search function
				if(search(buffer+3)!=(-1)) printf("\nThat word is in the hashtable\n");
				else printf("\nThat word is not in the hashtable!\n");
				break;
			case 'p':	//if second char is a p
				print();
				break;
			case 'Q':	//if second char is a Q
				quit();
				break;
			default:	//if first char is #, but second char is not listed above
				help();
				break;
		}
	}

	else{
		if(buffer[0]=='\0'){	//if the first char is an end of string, then the string is null
			printf("\nNULL strings are not accepted\n");
			help();		
		}
			//if search doesn't return -1, then the string is already in the table
		else if(search(buffer)!=(-1)) printf("\nThat string is already in the hashtable!\n");
		else  insert(buffer);
	}

}

/*-----------------------------------------------------------------*/
//This function will display the hashtable
void print(){
	int i;
	char deleted[10];
	printf("\nYou have reached the print function\n");
	printf("\n%-12s%-s		%-20s	%s\n","Location","Key","String","Status");
	printf("---------------------------------------------------------------\n");
	
	//check deleted flag, and copy an appropriate message to our 'deleted' string
	for(i=0;i<LENGTH;i++){
		if(hashtable[i].deleted==1){
			strcpy(deleted,"DELETED\0");
		}
		else if(hashtable[i].deleted==2){
			strcpy(deleted,"OCCUPIED\0");
		}
		else{
			strcpy(deleted,"EMPTY\0");
		}
								//print formatted output	
		printf("%-12d (%d)	%-20s	<%s>\n",i,hashtable[i].origKey,hashtable[i].string,deleted);
	}
	
}
/*----------------------------------------------------------------*/
//This function will search for a word in the hashtable
int search(char buffer[1024]){
	int keyVal;
	int collideCount=0;

	keyVal=key(buffer);	//find the value that they key should be

	//loop until either you find a location that is both null, and has not been deleted, or until the number of collisions is greater than 20.
	while((((hashtable[keyVal].string) != NULL) || (hashtable[keyVal].deleted!=0)) && (collideCount<=LENGTH)){
		if(hashtable[keyVal].string == NULL){	//this line adds 7 then mod 20 to avoid seg fault from strcmp with NULL
			keyVal=(keyVal+7)%LENGTH;
			collideCount=collideCount+1;
		}
		else if(strcmp(buffer,hashtable[keyVal].string)==0){	//if string in table matches word, return 1
			return(keyVal);
		}
		else{
			keyVal=(keyVal+7)%LENGTH;	//if location was not null, but string also didn't match, add 7 mod 20
			collideCount=collideCount+1;
		}
	}
	return(-1);		//if while didn't return 1, the word not found so return 0
}
/*----------------------------------------------------------------*/
//This function will insert a word into the hashtable
void insert(char buffer[1024]){
	int collideCount,keyVal,origKey;
	collideCount=0;
	keyVal=key(buffer);	//sum all char values, then reduce %20,can't call it key because i have a function key
	origKey=keyVal;		//copy of key for printing reasons.

	while(hashtable[keyVal].string != NULL && collideCount <= LENGTH){	//continue while the location is not empty and you have not encountered 20 collisions
		collideCount=collideCount+1;			
		keyVal=(keyVal+7)%LENGTH;			//add 7 to key and reduce modulo 20
	}
	if(collideCount >= LENGTH){				//if we collided 20 times, the table is full
		printf("\nSorry, but the hashtable is full!\n");
		return;
	}
		
	hashtable[keyVal].deleted=2;				//the value of 2 tells me that this is occupied, used for printing
	hashtable[keyVal].origKey=origKey;			//store the actual key value
	hashtable[keyVal].string=malloc(strlen(buffer+1));	//dynamically allocate space
	strcpy(hashtable[keyVal].string,buffer);		//copy the word into the table
}
/*----------------------------------------------------------------*/
//This function will delete a word from the hashtable
void delete(char buffer[1024]){

	int keyVal;
	if(buffer[0]=='\0'){			//if the first char in the buffer is the end of a string
		printf("\nHow you gonna delete a NULL? That table is full of NULLs. Try again!\n");
		return;
	}
	keyVal=search(buffer);			//set key to word to be deleted, or -1 if that word doesn't exist

	if(keyVal==(-1)){
		printf("\nThat word is not in the hashtable!\n");
		return;
	}
	
	free(hashtable[keyVal].string);		//free the word that we malloc'd
	hashtable[keyVal].string=NULL;		//set string = null
	hashtable[keyVal].deleted=1;		//set deleted flag
	hashtable[keyVal].origKey=0;		//return key value to 0
}
/*----------------------------------------------------------------*/
//This function will display the help menu
void help(){
	printf("\nYou have reached the help function\n");
	printf("If you have reached this function, your input format was incorrect\n");
	printf("Or I guess you could have done it on purpose with #h\n");
	printf("Either way, to avoid this failure in the future, please follow the directives below\n\n");
	printf("#h		sends you to this menu\n");
	printf("#d word		delete that word from the hashtable\n");
	printf("#s word		search for that word in the hashtable\n");
	printf("#p 		display the entire hash table\n");
	printf("#Q		exits the program\n");
	printf("(anything else)	inserts that string into the hashtable\n");
}
/*----------------------------------------------------------------*/
//This function will return the value of the key
int key(char buffer[1024]){
	int key=0;
	int i;
	for(i=0;i<strlen(buffer);i++){		//go through buffer 1 char at a time summing the values
		key=key+buffer[i];
	}
	key=key%LENGTH;				//reduce the sum % 20
	return(key);				//return the value of key
}	
/*-----------------------------------------------------------------*/
//This function will free all mallocs and then exit
void quit(){
	int i;
	for(i=0;i<LENGTH;i++){			//free anything that is not NULL and hasn't already been free'd
		if((hashtable[i].string!=NULL) && (hashtable[i].deleted!=1)){
			free(hashtable[i].string);
		}
	}
	exit(0);
}
/*------------------------------------------------------------------*/
//This function just drives the program with an infinite while loop
int main(){
	while(1) getInput();
}
/*------------------------------------------------------------------*/
	


