/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_semaph.c
 * Purpose:   Handle semaphores for shared memory operations.
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#include <assert.h>
#include <unistd.h>
#include <sys/sem.h>
#include <errno.h>
#include "debugtools.h"
#include "shm_semaph.h"

DEFAULT_DEBUG_CHANNEL(sem)
#define SEM_READ  0
#define SEM_WRITE 1

/* IMPORTANT: Make sure that killed process will not lock everything.
 *            If possible, restrict usage of these functions.
 */
void shm_read_wait(shm_sem semid)
{
  struct sembuf sop[2];
  int ret;
  
  TRACE("(%d)\n",semid);
  sop[0].sem_num=SEM_READ;
  sop[0].sem_op=1;		   /* add this read instance */
  sop[0].sem_flg=SEM_UNDO;	   /* undo in case process dies */
  
  sop[1].sem_num=SEM_WRITE;
  sop[1].sem_op=0;		   /* wait until no writing instance exists */
  sop[1].sem_flg=SEM_UNDO;

  do {
     ret=semop (semid,sop , 2);
  } while (ret<0 && errno==EINTR);  /* interrupted system call? */
  
  if (ret<0) 
     WARN("(semid=%d,errno=%d): Failed semaphore lock for read\n",
         semid, errno);
}
void shm_write_wait(shm_sem semid)
{
  struct sembuf sop[3];
  int ret;
  
  TRACE("(%d)\n",semid);
  sop[0].sem_num=SEM_READ;
  sop[0].sem_op=0;		   /* wait until no reading instance exist */
  sop[0].sem_flg=SEM_UNDO;		   

  sop[1].sem_num=SEM_WRITE;
  sop[1].sem_op=1;		   /* writing is in progress - disable read */
  sop[1].sem_flg=SEM_UNDO;	   /* undo if process dies */

  sop[2].sem_num=SEM_READ;
  sop[2].sem_op=1;		   /* disable new writes */
  sop[2].sem_flg=SEM_UNDO;
  
  do {
     ret=semop (semid,sop , 3);
  } while (ret<0 && errno==EINTR);  /* interrupted system call? */

  if (ret<0) 			   /* test for the error */
     WARN("(semid=%d,errno=%d): Failed semaphore lock for write\n",
	     semid, errno);
}
void shm_write_signal(shm_sem semid)
{
  struct sembuf sop[2];
  int ret;

  TRACE("(%d)\n",semid);
  sop[0].sem_num=SEM_READ;
  sop[0].sem_op=-1;	
  sop[0].sem_flg=IPC_NOWAIT | SEM_UNDO;	/* no reason to wait */

  sop[1].sem_num=SEM_WRITE;
  sop[1].sem_op=-1;		   
  sop[1].sem_flg=IPC_NOWAIT | SEM_UNDO;	/* no reason to wait */

  do {
     ret=semop (semid,sop , 2);
  } while (ret<0 && errno==EINTR);  /* interrupted system call? */

  if (ret<0) 			   /* test for the error */
     WARN("(semid=%d,errno=%d): Failed semaphore unlock for write\n",
	     semid, errno);
}

void shm_read_signal(shm_sem semid)
{
  struct sembuf sop[2];
  int ret;

  TRACE("(%d)\n",semid);
  sop[0].sem_num=SEM_READ;
  sop[0].sem_op=-1;	
  sop[0].sem_flg=IPC_NOWAIT | SEM_UNDO;	/* no reason to wait */

  do {
     ret=semop (semid,sop , 1);
  } while (ret<0 && errno==EINTR);  /* interrupted system call? */

  if (ret<0) 			   /* test for the error */
     WARN("(semid=%d,errno=%d): Failed semaphore unlock for read\n",
	     semid, errno);
}

void shm_sem_init(shm_sem *sptr)
{
  shm_sem semid;
  union semun arg;

  semid=semget (IPC_PRIVATE, 2, 0700 | IPC_CREAT);

  arg.val=0;
  semctl (semid, 0, SETVAL, arg);
  semctl (semid, 1, SETVAL, arg);
  *sptr=semid;
}

void shm_sem_done(shm_sem *semptr)
{
  union semun arg;

  semctl (*semptr, 0, IPC_RMID , arg);
  semctl (*semptr, 1, IPC_RMID , arg);

  *semptr= -1;
}

#endif  /* CONFIG_IPC */
