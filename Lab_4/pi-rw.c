/* System Includes */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sched.h>
#include <math.h>

#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)
#define MAXFILENAMELENGTH 80

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

double calculatePi(long iterations, int outputFD){

  	double x, y;
  	double inCircle = 0.0;
  	double inSquare = 0.0;
  	double pCircle = 0.0;
	double piCalc = 0.0;
	long i;
	char output[1024];
  
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

  	write(outputFD, output, strlen(output)*sizeof(char));

	return piCalc;
}


int main(int argc, char *argv[]){

	long iterations = atol(argv[1]);;
	struct sched_param param;
	int policy;
	int numProcs = atoi(argv[3]);
	int outputFD;
	char outputFilename[MAXFILENAMELENGTH];
	char outputFilenameBase[MAXFILENAMELENGTH];
	int rv;
	pid_t pid;
	int i;

	if(argc < 2){
		perror("Too few arguments.");
		exit(EXIT_FAILURE);
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

	/* Set process to max priority for given scheduler */
	param.sched_priority = sched_get_priority_max(policy);

	/* Set new scheduler policy */
	fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
  	fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
  	if(sched_setscheduler(0, policy, &param)){
    	perror("Error setting scheduler policy");
    	exit(EXIT_FAILURE);
  	}

	fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

	strncpy(outputFilenameBase, "pi-rwout", MAXFILENAMELENGTH);
  	for(i = 0; i < numProcs; i++)
  	{
    	pid = fork();
    	if(pid == 0)
    	{
      		break; 
    	}
  	}
  	if(pid == 0)
  	{
		rv = snprintf(outputFilename, MAXFILENAMELENGTH, "%s-%d",
		    outputFilenameBase, getpid());
		if(rv > MAXFILENAMELENGTH){
			fprintf(stderr, "Output filenmae length exceeds limit of %d characters.\n",
			MAXFILENAMELENGTH);
			exit(EXIT_FAILURE);
		}
		outputFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		calculatePi(iterations, outputFD);
 	}
  	else
  	{
    	printf("Parent Process\n"); 
    	while ((pid = waitpid(-1, NULL, 0))) {
			if (errno == ECHILD) {
	  			break;
			}
    	}
    }

	close(outputFD);
	remove(outputFilename);

	return 0;
}		
