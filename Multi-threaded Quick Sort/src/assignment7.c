#include "assignment7.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#define SORT_THRESHOLD      40


typedef struct _sortParams {
    char** array;
    int left;
    int right;
} SortParams;

/* a queue that holds jobs to complete */
typedef struct node {
    SortParams *data;
    struct node* next;
}node;

/* Function Prototypes */
static void *produce(SortParams *p);
static void *consumer();
    
static int maximumThreads;              /* maximum # of threads to be used */
static pthread_t *threads;              /* Array of thread ids */

/* Condition, threads wait for a job */
static pthread_cond_t jobReady = PTHREAD_COND_INITIALIZER;
static int jobs;

/* Protects available_threads var */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Declare global queue to hold jobs to be completed */
static node *queueHead = NULL;
static node *queueTail = NULL;

/* Check to see if global jobQueue is empty */
static int isEmpty(){
    if( queueHead == NULL )
        return 1;
    return 0;
}

/* Insert into the global jobQueue */
static void pushJob(SortParams *p){

    /* Allocate new job */
    node *temp = (node*) malloc(sizeof(node));
    temp->data = p;
    temp->next = NULL; 

    if( isEmpty() )            /* If the queue is empty, make this the top */
        queueHead = temp;
    else                       /* If the queue is not empty, make this the end */
        queueTail->next = temp;
    
    queueTail = temp;
}

/* Pop job off of jobQueue */
static SortParams* popJob(){
    
    if( isEmpty() ){         /* Edge case, return if queue empty */
        fprintf(stderr,"No job to pop\n");
        return NULL;
    }
    
    SortParams *retData;    /* Holder for return data */

    node *temp = queueHead; /* Store head in temp*/
    retData = temp->data;   /* Extract data */

    queueHead = temp->next;  /* Point head to next item in queue */
    free(temp);              /* Free allocated node */

    return retData;
}

/* This is an implementation of insert sort, which although it is */
/* n-squared, is faster at sorting short lists than quick sort,   */
/* due to its lack of recursive procedure call overhead.          */

static void insertSort(char** array, int left, int right) {
    int i, j;
    for (i = left + 1; i <= right; i++) {
        char* pivot = array[i];
        j = i - 1;
        while (j >= left && (strcmp(array[j],pivot) > 0)) {
            array[j + 1] = array[j];
            j--;
        }
        array[j + 1] = pivot;
    }
}

/* Recursive quick sort, but with a provision to use */
/* insert sort when the range gets small.            */

static void quickSort(void* p) {
    
    SortParams* params = (SortParams*) p;
    char** array = params->array;
    int left = params->left;
    int right = params->right;
    int i = left, j = right;

    if (j - i > SORT_THRESHOLD) {           /* if the sort range is substantial, use quick sort */

        int m = (i + j) >> 1;               /* pick pivot as median of         */
        char* temp, *pivot;                 /* first, last and middle elements */
        if (strcmp(array[i],array[m]) > 0) {
            temp = array[i]; array[i] = array[m]; array[m] = temp;
        }
        if (strcmp(array[m],array[j]) > 0) {
            temp = array[m]; array[m] = array[j]; array[j] = temp;
            if (strcmp(array[i],array[m]) > 0) {
                temp = array[i]; array[i] = array[m]; array[m] = temp;
            }
        }
        pivot = array[m];

        for (;;) {
            while (strcmp(array[i],pivot) < 0) i++; /* move i down to first element greater than or equal to pivot */
            while (strcmp(array[j],pivot) > 0) j--; /* move j up to first element less than or equal to pivot      */
            if (i < j) {
                char* temp = array[i];      /* if i and j have not passed each other */
                array[i++] = array[j];      /* swap their respective elements and    */
                array[j--] = temp;          /* advance both i and j                  */
            } else if (i == j) {
                i++; j--;
            } else break;                   /* if i > j, this partitioning is done  */
        }
        
        SortParams first;  first.array = array; first.left = left; first.right = j;
        // quickSort(&first);
        produce(&first);                  /* sort the left partition  */

        SortParams second; second.array = array; second.left = i; second.right = right;
        //quickSort(&second);
        produce(&second);                 /* sort the right partition */
                
    } else insertSort(array,i,j);           /* for a small range use insert sort */
}

/* Produce a job using input */
static void *produce(SortParams *p){
    printf("Producing job, first word %s, last word %s. Thread id (%ld)\n", p->array[0], p->array[p->right], pthread_self());
    pthread_mutex_lock(&mutex);             // Lock mutex to acced job queue
    jobs++;
    pushJob(p);                             // push new SortParams onto queue
    pthread_cond_signal(&jobReady);         // Signal threads to wake up
    pthread_mutex_unlock(&mutex);           // Unlock Mutex
}
        

/* Keep threads in pool waiting to be signalled to do a subarray sort */
static void *consumer(){
    // TODO: CHECK CONSUMER PSEUDOCODE AGAIN
    SortParams *temp;
    while(1){
        /* Wait on new job in queue */
        pthread_mutex_lock(&mutex);
        while( isEmpty() ){
            pthread_cond_wait(&jobReady, &mutex);
        }
        if (!isEmpty())
            temp = popJob();

        pthread_mutex_unlock(&mutex);
        /* If job exists, sort the next one */
        if( temp != NULL ){
            printf("Sorting job, first word %s, last word %s. Thread id (%ld)\n", temp->array[0], temp->array[temp->right], pthread_self());
            quickSort( temp );
            jobs--;
        }
        
        if(jobs == 0){
            return;
        }
    
    }
}

/* user interface routine to set the number of threads sortT is permitted to use */

void setSortThreads(int count) {
    maximumThreads = count;
    threads = (pthread_t*)calloc(maximumThreads, sizeof(threads));   /* Allocate space for thread id array */
    jobs = 0;                                                        /* Initialize jobs to 0 (nothing to do yet) */
}

/* user callable sort procedure, sorts array of count strings, beginning at address array */

void sortThreaded(char** array, unsigned int count) {
    int s, i;

    SortParams parameters;
    parameters.array = array; parameters.left = 0; parameters.right = count - 1;


    // Create each thread in pool and send to consumer
    for( i = 0; i < maximumThreads; i++){
       s = pthread_create(&threads[i], NULL, consumer, NULL);
       printf("Creating thread id (%ld)\n", threads[i]);
       if( s > 0){
           printf("Error <%d> on pthread_create\n", s);
           exit(-1);
       }
    }
    //quickSort(&parameters);
    produce(&parameters);

    // Wait for thread to join before returning
    for( i = 0; i < maximumThreads; i++){
        s = pthread_join(threads[i], NULL);
        printf("Joining thread %d\n", i);
        if( s > 0 ){
            printf("Error <%d> on pthread_join\n", s);
            exit(-1);
        }
    }

    if( threads != NULL ){
        free( threads );
    }
    
    return;
}

