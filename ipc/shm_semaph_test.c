/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_semaph_test.c
 * Purpose:   Test semaphores handleingr shared memory operations.
 ***************************************************************************
 */
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "shm_semaph.h"
#include <sys/shm.h>
#define DEBUG_DEFINE_VARIABLES
#include <stddebug.h>
#include <debug.h>

static volatile int * volatile data;
static int isparent=0;
#define DELAY (rand()%10)
shm_sem sem;

static void read_write(int num)
{
  int i,j ;
  volatile float dummy=0;
  int val;

  srand(num+time(NULL));
  for (i=0x3fff;i>=0;i--) {
     if((i&0x7ff)==0 && isparent)
	fprintf(stderr,"0x%06x\r",i);
     shm_write_wait(sem);
     *data= num;
     for (j=DELAY ; j>=0;j--)
	dummy*=2;
     if (*data!=num) {
	fprintf(stderr,"\nbad shm_write_wait(), num=%d\n",num);
	shm_write_signal(sem);
	return;
     }
     shm_write_signal(sem);
     for (j=DELAY ; j>=0 ;j--)
	dummy*=2;
     shm_read_wait(sem);
     val=*data;
     for (j=DELAY; j>=0 ;j--)
	dummy*=0.5;
     if (*data!=val) {
	fprintf(stderr,"\nbad shm_read_wait(), num=%d,val=%d,*data=%d\n",
		num,val,*data);
	shm_read_signal(sem);
	return;
     }
     shm_read_signal(sem);
  }
  if (isparent)
     fputc('\n',stderr);
}
static void child1()
{
  read_write(2);
}
static void child2()
{
  read_write(10);
}
static void parent()
{
  isparent=1;
  read_write(60);
}

int main()
{
  int shmid;
  int ret1, ret2;
  int pid1, pid2;
  int stat=0;
  
  shm_sem_init(&sem);
  shmid=shmget(IPC_PRIVATE, 0x100, IPC_CREAT | 0700);
  data= (int *)shmat ( shmid, NULL, 0);
  *data=0;
  
  switch (pid1=fork()) {
    case -1:
      perror("fork 1");
      return 1;
    case 0:
      fprintf(stderr,"child1\n");
      child1();
      fprintf(stderr,"child1 done\n");
      return 0;
    default :
  }
  switch (pid2=fork()) {
    case -1:
      perror("fork 2");
      stat|=1;
      break;
    case 0:
      fprintf(stderr,"child2\n");
      child2();
      fprintf(stderr,"child2 done\n");
      return 0;
    default :
  }
  fprintf(stderr,"parent\n");
  if (pid2>0) {			   /* if second fork did not fail */
     parent();
     fprintf(stderr,"parent done, waiting for child2\n");
     waitpid(pid2,&ret2,WUNTRACED);
     stat|=ret2;
  }
  fprintf(stderr,"parent done, waiting for child1\n");
  waitpid(pid1,&ret1,WUNTRACED);
  stat|=ret1;
  fprintf(stderr,"all done\n");

  shmctl(shmid, IPC_RMID,NULL);
  shm_sem_done(&sem);
  return stat;
}
