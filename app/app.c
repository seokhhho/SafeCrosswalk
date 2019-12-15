#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <fcntl.h> 
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h> 
#include <sys/types.h> 
#include <sys/sysmacros.h>  
#include <linux/spi/spidev.h>

#define LOADCELL_MAJOR_NUMBER	502
#define LOADCELL_MINOR_NUMBER	100
#define LOADCELL_DEV_PATH_NAME		"/dev/loadcell_dev"
#define LOADCELL_THRESHOLD		10000

#define LED_PED_GREEN	18
#define LED_CAR_RED		23
#define LED_CAR_YELLOW	24
#define LED_CAR_GREEN	25

#define HS_TRIG	2
#define HS_ECHO	3

#define HS_MAX	1400
#define HS_MIN	50
#define HS_WAIT_THRESHOLD	10
#define HS_INCOME_THRESHOLD	30
#define TERM_COMING	1

#define SPI_DEV0_PATH	"/dev/spidev0.0"
#define SPI_CHANNEL	0
#define SPI_SPEED	1000000
#define SPI_DELAY	0
#define SPI_BPW		8
#define ADC_CHANNEL	0
#define WATER_THRESHOLD 300

#define MAIN_WAITTIME	3

int isPedestrianComing = 0;
long weight = -1;
double distance = -1;
int adc_value = -1;
long tare = -1;

void *hs_read(void *data){
	//printf("hs_read() called\n");
	while(1){
		digitalWrite(HS_TRIG, HIGH);
		delayMicroseconds(10);
		digitalWrite(HS_TRIG, LOW);
		unsigned int echoStart = millis();
		while(digitalRead(HS_ECHO) == LOW && millis()-echoStart < 1000) {
			// do nothing
		}
		if (millis()-echoStart < 1000) {
		// Mark start
		unsigned int start = micros();
		while(digitalRead(HS_ECHO) == HIGH) {
			// do nothing
		}
		// Mark end
		unsigned int end = micros();
		unsigned int delta = end-start;
		
		if(delta < HS_MAX && delta > HS_MIN){
			distance = 34029 * delta / 2000000.0;	
			//printf("Distance: %f\n", distance);
		}
		else distance = -1;
		}
	}
    
}

void *water_read(void *data){
	struct spi_ioc_transfer spi;
	int spiFds = *(int *)data;
	
	
	while (1) {
	unsigned char buffer[3];
	buffer[0] = 1;
	buffer[1] = (8 + ADC_CHANNEL) << 4;
	buffer[2] = 0;
	
	memset(&spi, 0, sizeof(spi));
	spi.tx_buf = (unsigned long)buffer;
	spi.rx_buf = (unsigned long)buffer;
	spi.len = 3;
	spi.delay_usecs = (uint16_t) SPI_DELAY;
	spi.speed_hz = (uint16_t) SPI_SPEED;
	spi.bits_per_word = (uint8_t) SPI_BPW;
		ioctl(spiFds, SPI_IOC_MESSAGE(1), &spi);
		adc_value = ((buffer[1] & 3) << 8) + buffer[2];
		//printf("%d\n",adc_value);
		delay(100);
	}
}

void *incoming_validate(void *data){
	double buf1 = 0;
	double buf2 = 0;
	double buf3 = 0;
	//printf("thread opened\n");
	while(distance<0){
		//printf("distance not ready\n");
		sleep(1);
	}
	
	buf1 = distance;
	sleep(TERM_COMING);
	
	while(distance<0){
		//printf("distance not ready\n");
		sleep(1);
	}
	buf2 = distance;
	sleep(TERM_COMING);
	while(TRUE){
		while(distance<0){
			isPedestrianComing = FALSE;
			//printf("distance not ready\n");
			sleep(1);
		}
		buf3 = distance;
		if(buf3 < HS_INCOME_THRESHOLD
			& ((isPedestrianComing == TRUE)
			|| (buf2 - buf1 < 0
			&& buf3 - buf2 < 0))){
				isPedestrianComing = TRUE;
				//printf("%2.1f %2.1f %2.1f true\n",buf1,buf2,buf3);
		}
		else{
				isPedestrianComing = FALSE;
				//printf("%2.1f %2.1f %2.1f false\n",buf1,buf2,buf3);
		}
		sleep(TERM_COMING);
		
		while(distance<0){
			isPedestrianComing = FALSE;
			//printf("distance not ready\n");
			sleep(1);
		}
		buf1 = distance;
		if(buf1 < HS_INCOME_THRESHOLD
			& ((isPedestrianComing == TRUE)
			|| (buf3 - buf2 < 0
			&& buf1 - buf3 < 0))){
				isPedestrianComing = TRUE;
				//printf("%2.1f %2.1f %2.1f true\n",buf2,buf3,buf1);
		}
		else{
				isPedestrianComing = FALSE;
				//printf("%2.1f %2.1f %2.1f false\n",buf2,buf3,buf1);
		}
		sleep(TERM_COMING);
		
		while(distance<0){
			isPedestrianComing = FALSE;
			//printf("distance not ready\n");
			sleep(1);
		}
		buf2 = distance;
		if(buf2 < HS_INCOME_THRESHOLD
			& ((isPedestrianComing == TRUE)
			|| (buf1 - buf3 < 0
			&& buf2 - buf1 < 0))){
				isPedestrianComing = TRUE;
				//printf("%2.1f %2.1f %2.1f true\n",buf3,buf1,buf2);
		}
		else{
				isPedestrianComing = FALSE;
				//printf("%2.1f %2.1f %2.1f false\n",buf3,buf1,buf2);
		}
		sleep(TERM_COMING);
	}
}
/*
void *weight_read(void *data){
	int loadcell_fd = *(int*)data;
	int i, test;
	long buffer[1];
	long sample[5];
	long avg;
	
	while(1){
		*buffer = -1;
		while(*buffer < 3000 || *buffer >= 200000){
			usleep(100);
			read(loadcell_fd,buffer,4);
		}
		
	}
	
	test = 0;
	while(test<5){
		test = 0;
		avg=0;
		for(i=0;i<5;i++){
			read(loadcell_fd,buffer,4);
			while(*buffer < 0 || *buffer >= 0x7fffff){
				usleep(100000);
				read(loadcell_fd,buffer,4);
			}
			//printf("%ld ",*buffer);
			sample[i] = *buffer;
			avg += sample[i];
		}
		avg /= 5;
		for(i=0;i<5;i++){
			if((sample[i] - avg) < (avg*0.1) 
				&& (sample[i] - avg) > -(avg*0.1)){
				test++;
				//printf("test+ ");
			}
			//else printf("test- ");
		}
		//printf("avg: %ld \n",avg);
	}
	tare = avg;
	
	while(1){
		test = 0;
		avg = 0;
		for(i=0;i<3;i++){
			read(loadcell_fd,buffer,4);
			while(*buffer < 0 || *buffer >= 0x7fffff){
				usleep(100000);
				read(loadcell_fd,buffer,4);
			}
			sample[i] = *buffer;
			avg += sample[i];
		}
		avg /= 3;
		//printf("avg: %ld / ",avg);
		for(i=0;i<3;i++){
			if((sample[i] - avg) < (avg*0.1) 
				&& (sample[i] - avg) > -(avg*0.1)){
				test++;
				//printf("%ld+ ",sample[i]);
			}
			//else printf("%ld- ",sample[i]);
		}
		//printf("\n");
		if(test == 3){
			weight = avg - tare;
			//printf("weighted : %ld",weight);
		}
		sleep(1);
	}
}
*/

int main(void){
	pthread_t waterThread, hsReadThread, incomeCheckThread, weightThread;	
	//setbuf(stdout, NULL);
	
	int rc = wiringPiSetupGpio();
	if (rc != 0) {
		printf("Failed to wiringPiSetupGpio()\n");
		return 0;
	}
	
	// setup trafficlight
	pinMode(LED_PED_GREEN, OUTPUT);
	pinMode(LED_CAR_RED, OUTPUT);
	pinMode(LED_CAR_YELLOW, OUTPUT);
	pinMode(LED_CAR_GREEN, OUTPUT);
/*
	// setup weight
	int loadcell_dev = open(LOADCELL_DEV_PATH_NAME, O_RDWR < 0);
	if(loadcell_dev < 0){
		printf("Failed to open loadcell device\n");
		return -1;
	}
	if(pthread_create(&weightThread, NULL, weight_read, (void *)&loadcell_dev) < 0){
		perror("thread create error: ");
		exit(0);
	}
	
	// waiting tare
	printf("init weight tare, plz wait..\n");
	while(1){
		if(tare>0) break;
	}
	printf("tare init done: %ld\n",tare);
	*/
	
	// setup water
	int waterdev = open(SPI_DEV0_PATH, O_RDWR < 0);
	if(waterdev < 0){
		printf("Failed to open water device\n");
		return -1;
	}
	
	if(pthread_create(&waterThread, NULL, water_read, (void *)&waterdev) < 0){
		perror("thread create error: ");
		exit(0);
	}
	
	// setup hs
	pinMode(HS_TRIG, OUTPUT);
	pinMode(HS_ECHO, INPUT);
	digitalWrite(HS_TRIG, LOW);
	if(pthread_create(&hsReadThread, NULL, hs_read, (void *)NULL) < 0){
		perror("thread create error: ");
		exit(0);
	}
	
	if(pthread_create(&incomeCheckThread, NULL, incoming_validate, (void *)NULL) < 0){
		perror("thread create error: ");
		exit(0);
	}
	
	int yellowTime;
	int pedestrianTime;
	int extendCount;
	
	while(1){	
		digitalWrite(LED_PED_GREEN,LOW);	// pedestrian = red
		digitalWrite(LED_CAR_RED,LOW);
		digitalWrite(LED_CAR_YELLOW,LOW);
		digitalWrite(LED_CAR_GREEN,HIGH);	// car = green
		
		while(distance<0){
			printf("waiting pedestrian now..\n");
			sleep(MAIN_WAITTIME);
		}
		if(distance < HS_WAIT_THRESHOLD){
			printf("pedestrian accepted. validating now..\n");
			sleep(MAIN_WAITTIME);
			if(distance < HS_WAIT_THRESHOLD && distance != -1){
				// pedestrian waited enough, give him signal
				printf("pedestrian validated. turn on yellow\n");
				if(adc_value > WATER_THRESHOLD){
					// road is slippy, long yellow light time
					yellowTime = MAIN_WAITTIME *2;
				}
				else{
					yellowTime = MAIN_WAITTIME;
				}

				digitalWrite(LED_CAR_YELLOW,HIGH);
				digitalWrite(LED_CAR_GREEN,LOW); // car = yellow
				printf("yellow-> sleep %d\n",yellowTime);
				sleep(yellowTime);
				
				digitalWrite(LED_PED_GREEN,HIGH); // pedestrian = green
				digitalWrite(LED_CAR_RED,HIGH);
				digitalWrite(LED_CAR_YELLOW,LOW);  // car = red
				pedestrianTime = MAIN_WAITTIME *5;
				extendCount = 0;
				while(pedestrianTime > 0){
					sleep(MAIN_WAITTIME);
					pedestrianTime -= MAIN_WAITTIME;
					if(weight < LOADCELL_THRESHOLD
						&& isPedestrianComing == TRUE
						&& extendCount < 3){
						// car is NOT waiting, pedestrian coming.
						pedestrianTime += MAIN_WAITTIME;
						extendCount++;
						digitalWrite(LED_PED_GREEN,LOW);
						usleep(100000);
						digitalWrite(LED_PED_GREEN,HIGH);
						usleep(100000);
						digitalWrite(LED_PED_GREEN,LOW);
						usleep(100000);
						digitalWrite(LED_PED_GREEN,HIGH);
						usleep(100000);
						printf("pedestrian ++, pedestrian = %d\n", pedestrianTime);
					}
					else{
						printf("pedestrian --, pedestrian = %d\n", pedestrianTime);
					}
				}
			}
			else{
				printf("pedestrian declined, move to first step..\n");
			}
		}
	}

	
	digitalWrite(LED_PED_GREEN,LOW);
	digitalWrite(LED_CAR_RED,LOW);
	digitalWrite(LED_CAR_GREEN,LOW);
	
	return 0;
}
