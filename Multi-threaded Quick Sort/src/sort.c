#include "assignment7.h"
#include "getWord.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[]){

    FILE* fp;                                   /* File pointer to input */
    char **array = calloc(1, sizeof(char*));    /* Dynamically allocated array to hold strings */
    int count = 0;                              /* Index of last item in array */
    char* temp;                                 /* Holds each temporary string */
    clock_t start, stop;                         /* Track how long program takes to execute */
    
    /* Check that argument was supplied */
    if( argc < 2 ){
        printf("Please provide a filename as an argument\n");
        exit(0);
    }

    /* Open the file */
    fp = fopen(argv[1], "r");

    /* Iterate over file and copy each string into dynamically allocated array */
    while( (temp = getNextWord(fp)) != NULL){
        array = realloc( array , sizeof( char*) * (count + 1));         /* Allocate new index in array */
        array[count] = calloc(1, sizeof(char) * (strlen(temp) + 1));    /* Allocate space for string at index */
        strcpy(array[count], temp);                                     /* Copy string into array */
        free(temp);                                                     /* Free temp variable because getNextWord strdup */
        count++;                                                        /* Increment index to end of array */
    }

    
    start = clock();                /* Start counter */
    setSortThreads(5);              /* Initialize threads for sorting */
    sortThreaded( array, count);    /* Sort the array */
    stop = clock();                 /* Stop the counter */

    printf("Execution took %.6f seconds\n", ((double)stop - start) / CLOCKS_PER_SEC);

    /* Read array back and free all allocated memory */
    for( int i = 0; i < count; i++){
        //printf("%s\n", array[i]);
        free(array[i]);
    }
    free(array);
    fclose(fp);

    return (0);

}

