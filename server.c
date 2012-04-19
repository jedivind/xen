#define _GNU_SOURCE
#include "ringbuffer.h"
#include <sys/mman.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>


int main() {
  
  struct ring_buffer rbuff;
  init_ring_buffer(&rbuff);
  
  int fd = shm_open(DATA, O_RDWR | O_CREAT, S_IRWXU | S_IRWXO);
  int vimg = open("disk1.img", O_RDONLY | O_DIRECT);
  
  int pos = lseek(vimg, 0, SEEK_END);
 // printf("Length of file is %d ", pos);
 // printf("Number of sectors is %d\n", pos/512);
  write(fd, &rbuff, sizeof(struct ring_buffer));
  struct ring_buffer* shm_rbuff;
	
  
  shm_rbuff = (struct ring_buffer *)mmap(0, sizeof(struct ring_buffer),
					 PROT_EXEC | PROT_READ | PROT_WRITE,
					 MAP_SHARED, fd, 0);
  close(fd);  
  if(shm_rbuff == MAP_FAILED) {
    perror("Unable to map shared memory.");
    shm_unlink(DATA);
    return 1;
  }
  
  pthread_mutex_lock(&shm_rbuff->data_mutex);
  shm_rbuff->num_sectors_on_img = pos/512;
  pthread_mutex_unlock(&shm_rbuff->data_mutex);
  
  while(1) {
    pthread_mutex_lock(&shm_rbuff->data_mutex);
    pthread_cond_wait(&shm_rbuff->nonempty, &shm_rbuff->data_mutex);
    process_requests(shm_rbuff, vimg); 
    pthread_mutex_unlock(&shm_rbuff->data_mutex);
  }  

 munmap(shm_rbuff, sizeof(struct ring_buffer));
 shm_unlink(DATA);
 close(vimg);
}
