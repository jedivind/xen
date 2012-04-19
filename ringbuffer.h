#include <pthread.h>
#include <stdio.h>

//#define DEBUG

#define MAXSIZE 9999
#define DATA "data"


struct request {
  int response;
  int sector;
  int pid; 
  int tid;
};

struct buf
{
	struct request request_client;
	char response_server[512];
        
};

struct ring_buffer {
  
  struct buf buffer[MAXSIZE];
  unsigned int request_writes;
  unsigned int request_reads;

  pthread_mutex_t data_mutex;
  pthread_cond_t nonempty;
  pthread_cond_t response_ready[MAXSIZE];
  
  
  int num_sectors_on_img;
  unsigned int response_writes;
  unsigned int response_reads;
};

void init_ring_buffer(struct ring_buffer* rbuff);
int write_request(struct ring_buffer* rbuff, int sector, pid_t pid, int tid);
void read_request(struct ring_buffer* rbuff, int fd);
void process_requests(struct ring_buffer* rbuff, int fd);

