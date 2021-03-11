/* This program is an implemenation of using semphores to solve
   a multi-process version of The dining philosopher problem
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_EAT 100    // time that each philosopher eats before leaving table

// Return random integer with gaussian distribution
int randomGaussian(int mean, int stddev); 

// This function will be excuted by each child process (philosopher)
// as they eat for a total of 100 seconds
// The ID is the order than the children were created, and is used to 
// keep track of their position at the table
int philosophize(int ID, int semID);


/* The main function handles forking to create the children and initializing*/
/* the semaphore array                                                      */
int main(int argc, char *argv[]){
    int i;
    int semID;

    // Define semaphore array and initialize all values to 1
    semID = semget(IPC_PRIVATE, 5, IPC_CREAT | IPC_EXCL | 0600);
    struct sembuf init[5] = { {0,1,0}, {1,1,0}, {2,1,0}, {3,1,0}, {4,1,0}};
    if( semop(semID, init, 5) < 0){
        fprintf(stderr, "Errno: %d\n%s", errno, strerror(errno));
    }

    // Fork off 5 children and send each to the execute function
    i = 0;
    while( i < 5 ){
        int procID = fork();    // make new child
        if(procID){             // parent block, just increment count
            i++;
        }
        else{                   // send new child out to do aristotle things
            printf("Forked Philosopher %d, (process %d)\n", i, getpid());
            philosophize(i, semID);
        }
    }
    // wait for all children to exit
    while(wait(NULL) > 0);
    
    // Clean up semaphore artifacts
    if( semctl(semID, 0, IPC_RMID) < 0){
        fprintf(stderr, "Errno: %d\n%s", errno, strerror(errno));
        exit(-errno);
    }

    return(0);
    
}


/* Function for the execution of each philospher's stately duties (eating and thinking)*/
int philosophize(int ID, int semID){
    int cumulativeEat = 0;          // integer that tracks total time eating
    srand(time(0) * ID);            // change random number generator seed

    int leftStick = ID;             // number of left chopstick semaphore
    int rightStick = (ID + 1) % 5;  // number of right chipstick semaphore

    struct sembuf eat[2] = { {leftStick, -1, 0}, {rightStick, -1, 0} }; // left stick is same number as philosopher
    struct sembuf think[2] = { {leftStick, 1, 0}, {rightStick, 1, 0} }; // right stick is +1 (%5 wraps number 4's right to 0)

    // Infinite loop until cumulative eat is high enough
    for(;;){ 

        int eatTime = randomGaussian(9,3);
        int sleepTime = randomGaussian(11,7);
        eatTime = eatTime > 0 ? eatTime : 0;
        sleepTime = sleepTime > 0 ? sleepTime : 0;

        // Start thinking (presumably about shadows in caves)
        printf("Philosopher %d is thinking for %d seconds (%d)\n", ID, sleepTime, cumulativeEat);
        sleep( sleepTime );             // sleep for sleepTime, or 0, whichever is greater

        // wait until semaphore is ready (both necessary chopsticks are free) , then eat
        printf("Philosopher %d wants chopsticks %d and %d.\n", ID, leftStick, rightStick);
        if( semop(semID, eat, 2) < 0 ){
            fprintf(stderr, "Errno: %d\n%s",errno, strerror(errno));
            exit(-errno);
        }

        // Eat for specified time
        printf("Philosopher %d is picking up chopsticks %d and %d.\n", ID, leftStick, rightStick);
        printf("Philosopher %d is eating for %d seconds (%d)\n", ID, eatTime, cumulativeEat);
        sleep( eatTime );               // sleep for eatTime, or 0, whichever is greater
        cumulativeEat += eatTime;       // increment time spent eating

        // Put down chopsticks ( return semaphore resources )
        printf("Philosopher %d is laying down chopsticks %d and %d.\n", ID, leftStick, rightStick);
        if( semop(semID, think, 2) < 0){
            fprintf(stderr, "Errno: %d\n%s", errno, strerror(errno));
            exit(-errno);
        }

        // exit when total time eating reaches specified seconds
        if( cumulativeEat > MAX_EAT ){
            printf("Philosopher %d is leaving the table after eating for %d seconds.\n", ID, cumulativeEat);
            exit(0);
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
