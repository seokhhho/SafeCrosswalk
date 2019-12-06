#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// http://www.digipine.com/index.php?mid=clan&document_srl=584

#define TRUE 1
#define FALSE 0

#define THRESHOLD_ULTRASONIC_COMING	1234
#define TERM_COMING	1

int isPedestrianComing = 0;

void *t_function(void *data){
	int buf1 = 0;
	int buf2 = 0;
	int buf3 = 0;
	
	// ultrasonic.read(&buf1);
	sleep(TERM_COMING);
	// ultrasonic.read(&buf2);
	sleep(TERM_COMING);
	while(TRUE){
		// ultrasonic.read(&buf3);
		if(buf3 > THRESHOLD_ULTRASONIC_COMING
			&& buf2 - buf1 > 0
			&& buf3 - buf2 > 0){
				isPedestrianComing = TRUE;
				printf("true\n");
		}
		else{
				isPedestrianComing = FALSE;
				printf("false\n");
		}
		sleep(TERM_COMING);
		
		// ultrasonic.read(&buf1);
		if(buf1 > THRESHOLD_ULTRASONIC_COMING
			&& buf3 - buf2 > 0
			&& buf1 - buf3 > 0){
				isPedestrianComing = TRUE;
				printf("true\n");
		}
		else{
				isPedestrianComing = FALSE;
				printf("false\n");
		}
		sleep(TERM_COMING);
		
		// ultrasonic.read(&buf2);
		if(buf2 > THRESHOLD_ULTRASONIC_COMING
			&& buf1 - buf3 > 0
			&& buf2 - buf1 > 0){
				isPedestrianComing = TRUE;
				printf("true\n");
		}
		else{
				isPedestrianComing = FALSE;
				printf("false\n");
		}
		sleep(TERM_COMING);
	}
}

int main(void){
	pthread_t incomeCheckThread;
	int status;
	
	if(pthread_create(&incomeCheckThread, NULL, t_function, (void *)NULL) < 0){
		perror("thread create error: ");
		exit(0);
	}
	
	while(1){
		printf("main read: %d\n", isPedestrianComing);
		sleep(1);
	}
	
	pthread_join(incomeCheckThread, (void *)&status);
	printf("thread join : %d\n", status);
	
	return 0;
}
