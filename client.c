#include "ringbuffer.h"
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>


#define MAX_THREADS 20
float max_time_per_request=0;
float min_time_per_request=1e15;
float avg_time_per_thread=0;
float time_per_thread[MAX_THREADS];
int standard_deviation=0;
int requests_per_second=0;


int tid_ctr=0;
int NUM_THREADS;
int MAX_REQUESTS;

void* make_requests(void* param) {
  struct ring_buffer* rbuff = (struct ring_buffer*)param; 
  pid_t pid = getpid();
  int tid = tid_ctr++;
  int i;
 loop:
  for  (i = 0; i < MAX_REQUESTS; i++) {
    struct timeval start, end, diff;
    gettimeofday(&start, NULL);
    
    
    int response = write_request(rbuff, rand()%rbuff->num_sectors_on_img, pid, tid);
    
    if(response == -1)
      continue;

    pthread_mutex_lock(&rbuff->data_mutex);
    //usleep(100);
    while(1){if(&rbuff->response_ready[response]) break;}
	
    char result[512];
    memcpy(result,rbuff->buffer[rbuff->response_reads % MAXSIZE].response_server,512);
	
    rbuff->response_reads++;
    pthread_mutex_unlock(&rbuff->data_mutex);
    gettimeofday(&end, NULL);
    timersub(&end, &start, &diff);
    unsigned long time = diff.tv_sec*100000000l + diff.tv_usec*1000l;
#ifdef DEBUG
    printf("Request (%d, PID %d TID %d). Time elapsed(us) %ld.", i, pid, tid, time);
    printf(" Start (%ld s, %ld us). End (%ld s, %ld us).\n", start.tv_sec, start.tv_usec, end.tv_sec, end.tv_usec);
#endif
	
	//if(tid == 0 && i == 0) min_time_per_request = (float)time;
	//Stats
	if(max_time_per_request < time)
		max_time_per_request = (float)time;

	if(min_time_per_request > time)
		min_time_per_request = (float)time;

	avg_time_per_thread += (float)time; //Divide by NUM_THREADS to get average time for all threads.
	time_per_thread[tid] += (float)time; //Divide by MAX_REQUESTS to get time for each thread. Use these to calculate SD.
	
    usleep(100);
  
  }

}

int main(int argc, char *argv[]) {

	if(argc < 3)
	{
		printf("The syntax is ./client <number_of_threads> <num_requests>");
		exit(-1);
        }

	NUM_THREADS = atoi(argv[1]);
	MAX_REQUESTS = atoi(argv[2]);

	int k;
  	for(k=0; k<MAX_THREADS; k++)
	time_per_thread[k]=0;



  pthread_t threads[NUM_THREADS];
  int fd = shm_open(DATA, O_RDWR | O_CREAT, S_IRWXU | S_IRWXO);

  struct ring_buffer* shm_rbuff;
  shm_rbuff = (struct ring_buffer*)mmap(0, sizeof(struct ring_buffer),
					PROT_EXEC | PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0);
  
  close(fd);
  if(shm_rbuff == MAP_FAILED) {
    perror("Unable to map shared memory.");
    shm_unlink(DATA);
    return 1;
  }

  

  int i;
  for(i = 0; i < NUM_THREADS; i++) {
    if((pthread_create(&threads[i], NULL, make_requests, 
		       (void*)shm_rbuff)) != 0) {
      perror("Unable to create thread");
      break;
    }
  }

  for(i = 0; i <NUM_THREADS; i++) 
    pthread_join(threads[i], NULL);
  
  
  float sd=0; 
  float avg = (avg_time_per_thread/(NUM_THREADS*MAX_REQUESTS));

  for(i=0; i<NUM_THREADS; i++)
    sd +=  pow(((time_per_thread[i]/MAX_REQUESTS) - avg),2);
 
    sd = sqrt(sd/NUM_THREADS);
  
  printf("%lu|%lu|%lu|%f|%f\n", (unsigned long) max_time_per_request, (unsigned long) avg , (unsigned long) min_time_per_request, sd, (NUM_THREADS*MAX_REQUESTS)/(avg_time_per_thread/100000000.0));
  
  munmap(shm_rbuff, sizeof(struct ring_buffer));  
}
