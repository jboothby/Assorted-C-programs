/* This program is an implemenation of using mutexes to solve
   a multi-threaded version of The dining philosopher problem
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_EAT 100    // time that each philosopher eats before leaving table

// Return random integer with gaussian distribution
int randomGaussian(int mean, int stddev); 

// This function will be excuted by each child process (philosopher)
// as they eat for a total of 100 seconds
// The ID is the order than the children were created, and is used to 
// keep track of their position at the table
int philosophize(void *arg);

// Create global mutex array to be chopsticks
static pthread_mutex_t mutex[5];

/* The main function handles forking to create the children and initializing*/
/* the semaphore array                                                      */
int main(int argc, char *argv[]){
    int i, j, k, err;

    // Define pthread array to hold Identifiers of the threads
    pthread_t threadID[5];

    // Define array of ints for the philosopher IDs. This way values are static
    int philID[5];

    // Initialize the mutex variables in the global array and the ints in philID array
    for(i = 0; i < 5; i++){
       if( (err = pthread_mutex_init(&mutex[i], NULL) > 0)){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
       }
       philID[i] = i;
    }

    // Start execution of each thread
    for(j = 0; j < 5; j++){
       if( (err = pthread_create(&threadID[j], NULL, (void *)philosophize, &philID[j])) > 0){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
       }
    }

    // Wait for each thread to end
    for(k = 0; k < 5; k++){
        if( (err = pthread_join(threadID[k], NULL)) > 0){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
        }
        printf("Joined thread %d (thread ID %ld) back into main\n", k, threadID[k]);
    }

    return(0);
    
}


/* Function for the execution of each philospher's stately duties (eating and thinking)*/
int philosophize(void *arg){

    int ID = *(int *) arg;
    printf("Spawn Philosopher %d (thread ID %ld)\n", ID, pthread_self());

    int cumulativeEat = 0;          // integer that tracks total time eating
    srand(time(0) * ID);            // change random number generator seed

    int leftStick = ID;             // number of left chopstick semaphore
    int rightStick = (ID + 1) % 5;  // number of right chipstick semaphore

    // Make every even philosopher a righty, and the odd ones a lefty (to avoid 'all left first' deadlock)
    int grabFirst = ID % 2 == 0 ? leftStick : rightStick;    // grab lowest number chopstick first
    int grabSecond = ID % 2 == 0 ? rightStick : leftStick;   // grab higher number chopstick second

    // Infinite loop until cumulative eat is high enough
    for(;;){ 

        int err;        // holds potential error values from pthread API calls

        // Make calls to random for eat a sleep times. Make zero if negative
        int eatTime = randomGaussian(9,3);
        int sleepTime = randomGaussian(11,7);
        eatTime = eatTime > 0 ? eatTime : 0;
        sleepTime = sleepTime > 0 ? sleepTime : 0;

        // Start thinking (presumably about shadows in caves)
        printf("Philosopher %d is thinking for %d seconds (%d)\n", ID, sleepTime, cumulativeEat);
        sleep( sleepTime );             // sleep for sleepTime, or 0, whichever is greater

        // wait until mutex is ready (both necessary chopsticks are free) , then eat
        printf("Philosopher %d wants chopsticks %d and %d.\n", ID, leftStick, rightStick);
        if( (err = pthread_mutex_lock(&mutex[grabFirst]) > 0) ){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
        }
        printf("Philosopher %d is picking up chopstick %d.\n", ID, grabFirst);
        if( (err = pthread_mutex_lock(&mutex[grabSecond]) > 0)){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
        } 
        printf("Philosopher %d is picking up chopstick %d.\n", ID, grabSecond); 

        // Eat for specified time
        printf("Philosopher %d is eating for %d seconds (%d)\n", ID, eatTime, cumulativeEat);
        sleep( eatTime );               // sleep for eatTime, or 0, whichever is greater
        cumulativeEat += eatTime;       // increment time spent eating

        // Put down chopsticks ( return semaphore resources )
        printf("Philosopher %d is laying down chopsticks %d and %d.\n", ID, leftStick, rightStick);
        if( (err = pthread_mutex_unlock(&mutex[grabFirst])) > 0){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
        }
        if( (err = pthread_mutex_unlock(&mutex[grabSecond])) > 0){
            fprintf(stderr,"%d %s\n", errno, strerror(errno));
            exit(-errno);
        }

        // exit when total time eating reaches specified seconds
        if( cumulativeEat > MAX_EAT ){
            printf("Philosopher %d is leaving the table after eating for %d seconds.\n", ID, cumulativeEat);
            return(0);
        }
    }
}


/* successive calls to randomGaussian produce integer return values */
/* having a gaussian distribution with the given mean and standard  */
/* deviation.  Return values may be negative.                       */
int randomGaussian(int mean, int stddev) {
    double mu = 0.5 + (double) mean;
    double sigma = fabs((double) stddev);
    double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
    double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
    if (rand() & (1 << 5)) 
        return (int) floor(mu + sigma * cos(f2) * f1);
    else            
        return (int) floor(mu + sigma * sin(f2) * f1);
}
