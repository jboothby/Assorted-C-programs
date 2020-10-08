//Joseph Boothby
//Programming assignment 9
//This program will read an input file called adj.data, and read values from that file
//to make and adjacency matrix for a directed graph
//The program will find the shortest path between any user entered source
//and destination. The source and destination must be entered as command line
//arguments in that order.
//updated 11:06 03/10/14

/*----------header files---------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*---------defines---------------------------------------------------------------*/
#define INFILE "adj.data"

/*--------struct definitions-----------------------------------------------------*/
typedef struct queue{
	int q[128];	//actual array for queue
	int head;	//positions of head and tail
	int tail;
	int size;	//size of queue
}queue;

/*-----global adjacency matrix---------------------------------------------------*/
int adj[128][128];

/*---------function prototypes---------------------------------------------------*/
int findShort(int dest, int visited[128],struct queue *q);	//this function returns the value of the shortest distance
void pushQueue(int num, struct queue *q);			//function for adding values to the queue
int popQueue(struct queue *q);					//removes item from the queue and returns it's value
void nullLoad();						//fills the adjacency matrix with 0's
void arrayLoad(FILE *fp);					//reads values from the file into the matrix
void fileRead();						//opens the file and sets up pointers
/*-------------------------------------------------------------------------------*/

int main(int argc, char **argv){
	if(argc==3){					//if we have the correct amount of arguments
		int sourceNode,destNode;
		int visited[128];			//array for storing nodes we have already visited
		struct queue *q;
		q=malloc(sizeof(struct queue));		//make space for the queue

		if((1!=sscanf(argv[1],"%d",&sourceNode)) || (sourceNode>127) || (sourceNode<0)){	//exit if source node is not an integer or is a bad value
			fprintf(stderr,"\nYour source node needs to be a number between 0 and 127\n");
			exit(1);
		}
		if((1!=sscanf(argv[2],"%d",&destNode)) || (destNode>127) || (destNode<0)){					//exits if dest node ""                                ""
			fprintf(stderr,"\nYour destination node needs to be a number between 0 and 127\n");
			exit(1);
		}
		
		nullLoad();				//fill the matrix with 0's
		fileRead();				//open the file, and read in values
		pushQueue(sourceNode,q);		//put the first values for source and distance onto the queue
		pushQueue(0,q);				//distance from source to source is 0

		printf("\nThe minimum distance is %d\n",findShort(destNode,visited,q));


	}
	else{
		fprintf(stderr,"\nThis program requires two arguments\n");	//if not exactly three arguments
	}
}

/*---------------------------------------------------------------------------------------*/
//This function opens the file where the adj matrix is stored
//exits the program if the file can't be opened
void fileRead(){
	FILE *fp;
	fp = fopen(INFILE,"r");	//open adj.data for read

	if(fp==NULL){		//if adj.data opened incorrectly
		fprintf(stderr,"\nError opening adj.data\n");
		exit(1);
	}
	arrayLoad(fp);		//load values from file into matrix
}

/*---------------------------------------------------------------------------------------*/
//This function will read values from the file until it discover end of file
//or it discover a value in the incorrect format
void arrayLoad(FILE *fp){
	int start,finish;
	while (!feof(fp)){	//if we haven't reached the end of file
		if(fscanf(fp,"%d %d",&start,&finish) != 2) break;	//if in wrong format break out of loop
		adj[start][finish]=1;					//else load a 1 into matrix	
	}
}

/*--------------------------------------------------------------------------------------*/
//This function will load all 0's into the matrix so i don't have any weird values
//it's pretty self explainatory in it's execution
void nullLoad(){
	int i,j;
	for(i=0;i<128;i++){
		for(j=0;j<128;j++){
			adj[i][j]=0;
		}
	}
}

/*---------------------------------------------------------------------------------------*/
//This function is for loading values into the queue
//I stole it from program 5 and it works quite nicely
			
void pushQueue(int num, struct queue *q){
	if(q->size==128){
		fprintf(stderr,"\nError: Queue Overflow!\n");
		exit(1);
	}
	
	q->q[q->tail]=num;	//add new value onto queue at position of tail
	q->tail=(q->tail+1)%128;//increment the talue value, modulo 128
	q->size=q->size+1;	//increment the size of the queue
}

/*--------------------------------------------------------------------------------------*/
//This function removes and item from the queue and returns it's value
//I also stole it from program 5

int popQueue(struct queue *q){
	int temp;
	if(q->size==0){		//if queue underflows, then we searched all paths with no connection found
		fprintf(stderr,"\nQueue Underflow!\n");
		fprintf(stderr,"\nThis means that there is no connection from your");
		fprintf(stderr," source to that destination!\n");
		exit(1);
	}

	temp=q->q[q->head];		//save value from the head of the queue
	q->q[q->head]=0;		//set the head value to 0
	q->head=(q->head+1)%128;	//increment the head pointer
	q->size=q->size-1;		//decrement the size

	return(temp);			//return the old value from the head of the queue
}

/*------------------------------------------------------------------------------------*/
//This is the function that does all of the work
//It works quite well, and i'm glad i didn't have to use recursion

int findShort(int dest, int visited[128], struct queue *q){
	int popped,distance,i,total;
	popped=popQueue(q);		//Get first values from the queue for source node and distance
	distance=popQueue(q);
	while(popped!=dest){		//While we haven't reached our destination node
		distance++;		//Increment the distance to the next node
		for(i=0;i<128;i++){	//Add all connections that we haven't visited before, and that have a connection to our node
			if(adj[popped][i]==1&&visited[i]!=1){
				visited[i]=1;	//put flag in visited array so we know we've been here
				pushQueue(i,q);	//add new node to the queue
				pushQueue(distance,q);//add the distance with it
			}
		}
		popped=popQueue(q);	//pop off value at the head of the queue as new source
		distance=popQueue(q);	//set distance = to the distance of the new source
	}
	return(distance);
}
/*------------------------------------------------------------------------------------*/
		
