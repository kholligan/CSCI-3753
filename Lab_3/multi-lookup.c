#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define RESOLVER_THREAD_COUNT 10
#define MAX_IPADDR_LEN 2049 
#define MIN_RESOLVER_THREADS 2

void* RequesterThreadPool(void*);
void* ResolverThreadPool();

pthread_mutex_t queueLock;
pthread_mutex_t outputfileLock;

queue domainnameQueue;
FILE* outputfp = NULL;
int allHostnamesInQueue = 0;

int main(int argc, char* argv[]){

	int rc; 
	int NUM_THREADS = argc - 1;
	int numFiles = argc - 2;
	int i;
	
	//Create the queue
	queue_init(&domainnameQueue, -1);

	//Create the requester thread array, the resolver thread array
	//and the attributes
	pthread_t threads[argc-1];
	pthread_t resolverthreads[RESOLVER_THREAD_COUNT];
	pthread_attr_t tattr;
	pthread_attr_init(&tattr);

	//Create the mutex locks for the queue and the outputfile
	pthread_mutex_init(&queueLock, NULL);
	pthread_mutex_init(&outputfileLock, NULL);

	/* Check Arguments */
	if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
	}

	/* Open Output File */
	outputfp = fopen(argv[(argc-1)], "w");
	if(!outputfp){
		perror("Error Opening Output File");
		return EXIT_FAILURE;
	}

	/* Loop through Input Files */
	for(i = 1; i < NUM_THREADS; i++){
		
		//create threads based on the number of files (1 less than NUM_THREADS)
		//Pass the attributes and the file as the args
		rc = pthread_create(&(threads[i-1]),&tattr,RequesterThreadPool,argv[i]);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}

	//Create resolver threads
	for(i = 0; i < RESOLVER_THREAD_COUNT; ++i)
	{
		rc = pthread_create(&(resolverthreads[i]), &tattr, ResolverThreadPool, NULL);
		if (rc)
		{
			 printf("ERROR; return code from pthread_create() is %d\n", rc);
			 exit(EXIT_FAILURE);
		}
	}
	
	//Call pthread_join for the number of files sent to the requester
	//pthread_join suspends execution of calling thread until target thread
	//terminates.
	//Waits for all the threads to finish
	for(i = 0; i < numFiles; ++i)
	{
		rc = pthread_join(threads[i], NULL);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}
	allHostnamesInQueue = 1;
	
	//Wait for resolver threads to finish
	for(i = 0; i < RESOLVER_THREAD_COUNT; ++i)
	{
		rc = pthread_join(resolverthreads[i], NULL);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}


	//close the output file, clean queue, and destroy mutexes
	fclose(outputfp);
	queue_cleanup(&domainnameQueue);
	pthread_mutex_destroy(&queueLock);
	pthread_mutex_destroy(&outputfileLock);

	return EXIT_SUCCESS;
}


void* RequesterThreadPool(void* threadargs){
	char hostname[SBUFSIZE]; //Store the hostname
	char errorstr[SBUFSIZE]; //Store errors
	char* fileName = threadargs; //get the file from the passed args
	char* queueValues; //Value to be stored in queue
	FILE* inputfp = fopen(threadargs, "r");
	
    
	//Check that input file is valid
	if(!inputfp){
		sprintf(errorstr, "Error Opening Input File: %s",fileName);
		perror(errorstr);	
		return NULL;
	}
	
	//Read hostnames from file
	while(fscanf(inputfp, INPUTFS, hostname) > 0){
		int addedToQueue = 0;

		//Spin in this while loop until the host name is added to queue
		//In case queue is full or locked when trying to add
		while(!addedToQueue)
		{
			pthread_mutex_lock(&queueLock); //Lock the queue
			//If queue is full, unlock the queue lock so other threads don't get
			//locked out while waiting for a space to become available
			if(queue_is_full(&domainnameQueue))
			{
				pthread_mutex_unlock(&queueLock);
			}
			else
			{
				//create new space on the heap for the hostname
				queueValues = malloc(SBUFSIZE);
				//copy host name to queue value
				strncpy(queueValues, hostname, SBUFSIZE);
				//push hostname on the queue
				queue_push(&domainnameQueue, queueValues);
				pthread_mutex_unlock(&queueLock); //unlock the queue
				addedToQueue = 1;
			}
		}
	}
	
	//close the input file
	fclose(inputfp);

	return NULL;
}

void* ResolverThreadPool(){
	
	char* hostname;
	char ipstrs[MAX_IPADDR_LEN]; //make the str long enough to hold multiple ip addresses
	
	//Keep running lookups until queue is empty and all the requesters have completed adding hostnames to queue
	while(!queue_is_empty(&domainnameQueue) || !allHostnamesInQueue){
		
		//Lock the queue before popping hostname
		pthread_mutex_lock(&queueLock);
		if(!queue_is_empty(&domainnameQueue)) //test if empty
		{
			//acquire hostname
			hostname = queue_pop(&domainnameQueue);

			pthread_mutex_unlock(&queueLock); //unlock queue
			//Acquire ipaddresses from util
			if(multidnslookup(hostname, ipstrs, sizeof(ipstrs)) == UTIL_FAILURE)
			{
				fprintf(stderr, "dnslookup error: %s\n", hostname);
				strncpy(ipstrs, "", sizeof(ipstrs));
			}

			pthread_mutex_lock(&outputfileLock);
			fprintf(outputfp, "%s,%s\n", hostname, ipstrs);
			pthread_mutex_unlock(&outputfileLock);
			
			//release the memory allocated to hostname
			free(hostname);
		}

		else
		{
			//release lock if queue is empty, while waiting for it to fill
			pthread_mutex_unlock(&queueLock);
		}
	
	}
	
	return NULL;
}
