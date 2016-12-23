/*
 * File: pi-sched.c
 * Author: Andy Sayler
 * Project: CSCI 3753 Programming Assignment 3
 * Create Date: 2012/03/07
 * Modify Date: 2012/03/09
 * Description:
 * 	This file contains a simple program for statistically
 *      calculating pi using a specific scheduling policy.
 */

/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>

#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

void calculatePi(long iterations){
	
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
	long i;    

	/* Calculate pi using statistical methode across all iterations*/
    for(i=0; i<iterations; i++){
	x = (random() % (RADIUS * 2)) - RADIUS;
	y = (random() % (RADIUS * 2)) - RADIUS;
	if(zeroDist(x,y) < RADIUS){
	    inCircle++;
	}
	inSquare++;
    }

    /* Finish calculation */
    pCircle = inCircle/inSquare;
    piCalc = pCircle * 4.0;

    /* Print result */
    fprintf(stdout, "pi = %f\n", piCalc);
}

void newProc(int numProcs, long iterations){
	pid_t pid;
	long i;
	
	for(i = 0; i < numProcs; i++){
		pid = fork();
		if(pid == 0){ 
			break; 
		}
	}

	if(pid == 0){
		calculatePi(iterations);
	}
	else{
		while((pid = waitpid(-1, NULL, 0))) {
			if(errno == ECHILD){
				break;
			}
		}
	}

}

int main(int argc, char* argv[]){

    long iterations;
    struct sched_param param;
    int policy;
	int numProcs;

    /* Process program arguments to select iterations and policy */
    /* Set default iterations if not supplied */
    if(argc < 2){
	iterations = DEFAULT_ITERATIONS;
    }
    /* Set default policy if not supplied */
    if(argc < 3){
	policy = SCHED_OTHER;
    }
	/* Set default processes if not supplied */
	if(argc < 4){
		numProcs = 1;
	}
    /* Set iterations and number of processes if supplied */
    if(argc > 1){
		iterations = atol(argv[1]);
		numProcs = atoi(argv[3]);
		if(iterations < 1){
	    	fprintf(stderr, "Bad iterations value\n");
	    	exit(EXIT_FAILURE);
		}
		if(numProcs < 1){
			fprintf(stderr, "Processes must be 1 or greater\n");
			exit(EXIT_FAILURE);
		}
    }
	
    /* Set policy if supplied */
    if(argc > 2){
	if(!strcmp(argv[2], "SCHED_OTHER")){
	    policy = SCHED_OTHER;
	}
	else if(!strcmp(argv[2], "SCHED_FIFO")){
	    policy = SCHED_FIFO;
	}
	else if(!strcmp(argv[2], "SCHED_RR")){
	    policy = SCHED_RR;
	}
	else{
	    fprintf(stderr, "Unhandeled scheduling policy\n");
	    exit(EXIT_FAILURE);
	}
    }
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));
	
	newProc(numProcs, iterations);
	
    return 0;
}