/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_main_blk.c
 * Purpose:   Main Wine's shared memory block
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#define inline __inline__
#include <sys/types.h>
#include <sys/sem.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stddebug.h>
#include <debug.h>
#include "shm_fragment.h"
#include "shm_block.h"
#include "shm_main_blk.h"
#include "shm_semaph.h"

#define WineKey (   'W'+((int)'i'<<8)+((int)'n'<<16)+((int)'e'<<24)  )
#define SHM_KEY_RANGE 8

/* main block (set during initialization) */
struct shm_main_block *main_block=NULL;
static char *shm_header="Wine - Windows emulator DDE mechanism";
static int main_shm_id;

static void shm_main_refresh();

/* for debugging only */
static void print_perm(struct ipc_perm *perm)
{
  printf("Permission:\n");
  printf("\tKey=%d,   mode=%03o,   sequence #=%d\n",
	 (int)perm->key,perm->mode, perm->seq);
  printf("\towner: uid=%d, gid=%d ;"  ,perm->uid, perm->gid);
  printf("  creator: uid=%d, gid=%d\n",perm->cuid,perm->cgid);
}

/* for debugging only */
/* print_shm_info: print shared memory descriptor info */
static void print_shm_info(int shm_id)
{
  struct shmid_ds ds;
  shmctl(shm_id, IPC_STAT, &ds );

  printf("shm_id=%d, Size=0x%08x , Number of attaches=%d\n",
	 shm_id, ds.shm_segsz, (int)ds.shm_nattch);
  if (ds.shm_atime)
     printf("Last attach=%s",ctime(&ds.shm_atime));
  if (ds.shm_dtime)
     printf("Last detach=%s",ctime(&ds.shm_dtime));
  printf("Last change=%s",ctime(&ds.shm_ctime));
  printf("pid: creator=%d, last operator=%d\n",
	 (int)ds.shm_cpid,(int)ds.shm_lpid);
  print_perm( &ds.shm_perm);

}

int proc_exist(pid_t pid)
{
  if ( kill(pid,0) == 0)	   /* dummy signal to test existence */
     return 1;
  else if (errno==ESRCH)	   /* "no such process" */
     return 0;
  else
     return 1;
}

/* setup a new main shm block (only construct a shm block object). */
static void shm_setup_main_block()
{
  dprintf_shm(stddeb,"creating data structure\n");
  main_block->build_lock=1;
  strcpy(main_block->magic, shm_header);

  shm_setup_block(&main_block->block,sizeof(*main_block),SHM_MINBLOCK);

  dde_proc_init(main_block->proc);
  ATOM_GlobalInit();
  shm_sem_init(&main_block->sem);

  /* main block set and data structure is stable now */
  main_block->build_lock=0;
}

/* Delete everything related to main_block */
void shm_delete_all(int shmid)
{
  int proc_idx;

  if (shmid == -1) 
    shmid= main_shm_id;
  
  shmctl( shmid, IPC_RMID, NULL);
  
  for (proc_idx= 0 ; proc_idx < DDE_PROCS ; proc_idx++)
     dde_proc_done( &main_block->proc[proc_idx] );
  
  shm_sem_done(&main_block->sem);
  shmdt( (void *) main_block);
  main_block= NULL;
}

int DDE_no_of_attached()
{
  struct shmid_ds shm_info;
  
  if (shmctl(main_shm_id, IPC_STAT, &shm_info) == -1)
    return -1;

  return shm_info.shm_nattch;
}
/*
** Test if shm_id is MainBlock and attach it (if it is),
** Return 1 if ok, 0 otherwise.
*/
static int attach_MainBlock(int shm_id)
{
  struct shmid_ds shm_info;

  if (shmctl(shm_id, IPC_STAT, &shm_info) == -1)
     return 0;

  /* Make sure we don't work on somebody else's block */
  if (shm_info.shm_perm.cuid != getuid()) { /* creator is not me */
     dprintf_shm(stddeb,"Creator is not me!\n");
     return 0;
  }

  dprintf_shm(stddeb,"shared memory exist, attaching anywhere\n");
  main_block=(struct shm_main_block *)shmat(shm_id, 0, 0);
  if ( (int)main_block==-1) {
     dprintf_shm(stddeb,"Attach failed\n");
     return 0;
  }

  if (strcmp(main_block->magic, shm_header) != 0) {
     dprintf_shm(stddeb,"Detaching, wrong magic\n");
     shmdt((void *)main_block);
     return 0;
  }

  if (debugging_shm)
     print_shm_info(shm_id);

  /* Is it an old unused block ? */
  if (shm_info.shm_nattch == 0) {
     dprintf_shm(stddeb,"No attaches, deleting old data\n");
     shm_delete_all(shm_id);
     return 0;
  }

  /* Wait for data structure to stabilize */
  while (main_block->build_lock)
     usleep(10000);

  main_shm_id= shm_id;

  shm_main_refresh();
  return 1;
}

/* (Function used by the constructor)
 * Try to get existing shared memory with key="Wine", size=SHM_MINBLOCK
 * complete user permission.
 * If such block is found - return true (1),  else return false (0)
 */
static int shm_locate_MainBlock(key_t shm_key)
{
    int shm_id;			/* Descriptor to this shared memory */
    int i;

    dprintf_shm(stddeb,"shm_locate_MainBlock: trying to attach, key=0x%x\n",
		shm_key);
    for (i=0 ; i < SHM_KEY_RANGE ; i++) {
       dprintf_shm(stddeb,"iteration=%d\n", i);

       shm_id= shmget ( shm_key+i, SHM_MINBLOCK ,0700);

       if (shm_id != -1) {
	 if ( attach_MainBlock(shm_id) ) {
	   return 1;		   /* success! */
	 }
       } else {
	  switch(errno) {
#ifdef EIDRM
	    case EIDRM:		   /* segment destroyed */
#endif
	    case EACCES:	   /* no user permision */
	      break;

	    case ENOMEM:	   /* no free memory */
	    case ENOENT:	   /* this key does not exist */
	    default :
	      dprintf_shm(stddeb,"shmget failed, errno=%d, %s\n",
			  errno, strerror(errno) );
	      return 0;		   /* Failed */
	  }
       } /* if .. else */
    } /* for */
    return 0;
}

/* (Function used by the constructor)
 * Try to allocate new shared memory with key="Wine", size=SHM_MINBLOCK
 * with complete user permission.
 * If allocation succeeds - return true (1),  else return false (0)
 */
static int shm_create_MainBlock(key_t MainShmKey)
{
  int shm_id;
  int flags= 0700 | IPC_CREAT | IPC_EXCL;
  int i;

  dprintf_shm(stddeb,"creating shared memory\n");

  /* try to allocate shared memory with key="Wine", size=SHM_MINBLOCK, */
  /* complete user permission */
  for (i=0 ; i < SHM_KEY_RANGE ; i++) {
     shm_id= shmget ( (key_t) MainShmKey, SHM_MINBLOCK, flags);
     if (shm_id != -1)
	break;
  }
  if (shm_id == -1) {
     dprintf_shm(stddeb,"failed to create shared memory\n");
     return 0;
  }
  dprintf_shm(stddeb,"shared memory created, attaching\n");
  main_block=(struct shm_main_block*) shmat(shm_id, 0,0);
  if (debugging_shm)
     print_shm_info(shm_id);
  main_shm_id= shm_id;
  shm_setup_main_block();
  dde_wnd_setup();
  return 1;

}

/* link to the dde shared memory block */
/* RETURN: 0 on success, non zero on failure */
int shm_init(void)
{
  if ( !shm_locate_MainBlock(WineKey)
       && !shm_create_MainBlock(WineKey)) { 
     fflush(stdout);
     fprintf(stderr,"shm_init: failed to init main shm block\n");
     exit(1);
  }

  dde_proc_add(main_block->proc);
  return 0;
}

static void shm_main_refresh()
{
  int proc_idx;
  
  for (proc_idx= 0 ; proc_idx < DDE_PROCS ; proc_idx++)
     dde_proc_refresh( &main_block->proc[proc_idx] );
}

#endif  /* CONFIG_IPC */
