#include "ringbuffer.h"
#include <math.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

void read_request(struct ring_buffer* rbuff, int fd) {

  int size = rbuff->request_writes - rbuff->request_reads;

  if(size < 1)
    return;

  struct request* next = &rbuff->buffer[rbuff->request_reads % MAXSIZE].request_client;
 // printf("next->response = %d", next->response);
#ifdef DEBUG
  printf("PID of client is %d Sector requested is %d\n", next->pid, next->sector);
#endif

  //Convert PID and sector number to string.
  char pid[200], sector[200];
  //itoa(next->pid, pid, 10);
  sprintf(pid, "%d", next->pid);
  sprintf(sector, "%d\n", next->sector);
  //

  //Construct path for file with sector requests.
  char path[200];
  strcpy(path, "sectors.");
  strcat(path, pid);
  //
  
  //Write sector requests into corresponding sector requests file.
  FILE * secfile = fopen(path,"a+");
  fwrite(sector, strlen(sector) , 1, secfile);
  fclose(secfile);  
  //
  
  //Construct path for file with read contents of the sector
  strcpy(path, "read.");
  strcat(path, pid);
  //

  //Read the sector from virtual image file and write into corresponding files. 
  char res[512];
  void* result;
  posix_memalign(&result, 512, 512);
  
  lseek(fd, (next->sector)*512, SEEK_SET);
  read(fd, result, 512);
  strncpy(res,(char*)result,512);
  
  //printf("%s\n", strerror(errno));
  //printf("errno %d\n", errno);
  //fread(result,512, 1, fd);
  //printf("fd %d\n", fd);
  //int i;
  //for(i=0;i<512;i++)
  //    printf("%x ", ((int*)result)[i]);
	
  FILE * resfile = fopen(path, "a+");
  fwrite(result, 512 , 1, resfile);
  fclose(resfile);
  strncpy(rbuff->buffer[rbuff->response_writes % MAXSIZE].response_server, res, 512);
  //printf("Retrieved %s from sector\n", result);
  //printf("Received request (%d, %d) = %d\n", next->x, next->p, result);
  rbuff->request_reads++;
  //printf("RESULT %s", (char *)result);
  

  rbuff->response_writes++;
#ifdef DEBUG
  printf("Writing into response ready at next->response %d\n", next->response);
#endif
  free(result);

  pthread_cond_signal(&rbuff->response_ready[next->response]);
 
}

void process_requests(struct ring_buffer* rbuff, int fd) {
  while((rbuff->request_writes - rbuff->request_reads) > 0) 
    read_request(rbuff,fd);
}

int write_request(struct ring_buffer* rbuff, int sector, pid_t pid, int tid) {
  struct request* next;

  pthread_mutex_lock(&rbuff->data_mutex);
  int size = rbuff->request_writes - rbuff->request_reads;

  if(size > MAXSIZE - 1)
    return -1;

  next = &rbuff->buffer[rbuff->request_writes % MAXSIZE].request_client;
  next->sector = sector;
  next->pid = pid;
  next->tid = tid;
 
  rbuff->request_writes++;
  int response = next->response;

  pthread_cond_signal(&rbuff->nonempty);
  pthread_mutex_unlock(&rbuff->data_mutex);
  return response;
}

void init_ring_buffer(struct ring_buffer* rbuff) {
  rbuff->request_writes = 0;
  rbuff->request_reads = 0;

  rbuff->response_writes = 0;
  rbuff->response_reads = 0;

  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);  
  pthread_mutex_init(&rbuff->data_mutex, &mattr);
  
  pthread_condattr_t  cattr;
  pthread_condattr_init(&cattr);
  pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&rbuff->nonempty, &cattr);
  

  int i;
  for(i = 0; i < MAXSIZE; i++) {
    pthread_cond_init(&rbuff->response_ready[i], &cattr);
    rbuff->buffer[i].request_client.response = i;
  }

}
