/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_block.c
 * Purpose:   Treat a shared memory block.
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#define inline __inline__
#include <sys/types.h>
#include <sys/sem.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include "debug.h"
#include "global.h"
#include "selectors.h"
#include "shm_fragment.h"
#include "shm_block.h"
#include "shm_semaph.h"
#include "dde_proc.h"
#include "xmalloc.h"

/* How each shmid is maped to local pointer */
/* Only attached shm blocks are in this construct */
struct local_shm_map *shm_map=NULL;

/* setup a new shm block (construct a shm block object).
 *    block: The pointer to the memory block (local mapping)
 *    first: The first data byte (excluding header stuff),
 *           if 0  (zero) Use the default.
 *    size:  The size of the memory block.
 */
void shm_setup_block(struct shm_block *block, int first, int size)
{
  TRACE(shm,"Setting up shm block at 0x%08x\n",(int )block);
  /* setup block internal data structure */
  if (first <= 0) {
     first=sizeof(*block);
     /* round up - so everything starts on cache line boundary
      * (assume cache line=32 bytes, may be bigger/smaller for
      *  different processors and different L2 caches .)
      */
     first=(first+0x1f) & ~0x1f;
  }
  block->free=size-first;
  block->next_shm_id=-1; /* IPC shm ID (for initial linking) */
  block->proc_idx= curr_proc_idx;
  /* block->size is initialized in shm_FragmentInit */
  shm_FragmentInit(block, first, size);  /* first item in the free list */
  
  TRACE(shm, "block was set up at 0x%08x, size=0x%04xKB, 1st usable=%02x\n",
	      (int )block,size/1024,first);
}

/* shm_attach_block: attach existing shm block, setup selectors
 * shm_id - id of the block to attach.
 * proc_idx - if not -1, puts this data into local mapping
 * map - localy mapped info about this block.
 */
/* NOTE: there is no check if this block is already attached.
 *       Attaching the same block more than once - is possible
 *       In case of doubt use shm_locate_block.
 */
struct shm_block *shm_attach_block(int shm_id, int proc_idx,
				   struct local_shm_map *map)
{
  struct shm_block *block;
  struct shmid_ds ds;
  struct local_shm_map *this;
  
  shmctl(shm_id, IPC_STAT, &ds );

  block=(struct shm_block*)shmat(shm_id, NULL, 0);
  if (block==NULL || block == (struct shm_block*) -1) return NULL;

  this=(struct local_shm_map *)xmalloc(sizeof(*this));
  this->next= shm_map;
  shm_map   = this;
  this->shm_id= shm_id;
  this->ptr = block;
  
  if (proc_idx < 0)
      this->proc_idx=block->proc_idx;
  else 
      this->proc_idx=proc_idx;

  if (map != NULL) {
     memcpy(map, this, sizeof(map));
     map->next= NULL;		/* don't pass private info */
  }

  return block;
}

struct shm_block *shm_create_block(int first, int size, int *shm_id) 
{
  struct shm_block *block;
  
  if (size==0)
     size=SHM_MINBLOCK;
  else
     /* round up size to a multiple of SHM_MINBLOCK */
     size= (size+SHM_MINBLOCK-1) & ~(SHM_MINBLOCK-1);
  *shm_id= shmget ( IPC_PRIVATE, size ,0700);
  if (*shm_id==-1)
     return NULL;
  block=shm_attach_block(*shm_id, curr_proc_idx, NULL);
  if (block!=NULL)
     shm_setup_block(block, first, size);

  return block;
}

/*
** Locate attached block. (return it, or NULL on failure)
** shm_id is the block we look for.
** *map - will get all the info related to this local map + proc_idx
**        (may be NULL)
** *seg - will get the segment this block is attached to.
*/
struct shm_block *shm_locate_attached_block(int shm_id,
					    struct local_shm_map *map)
{
  struct local_shm_map *curr;
  
  for (curr=  shm_map ; curr != NULL ; curr= curr->next) {
     if (curr->shm_id == shm_id) {
	if (map) {
	    memcpy(map, curr, sizeof(*curr) );
	    map->next = NULL;	/* this is private info ! */
	}
	return curr->ptr;
    }
  }
  
  /* block not found !  */
  return 0;
}

/* shm_locate_block: see shm_attach_block.
   In addition to shm_attach_block, make sure this
   block is not already attached.
   */
struct shm_block *shm_locate_block(int shm_id, struct local_shm_map *map)
{
  
  struct shm_block *ret;
  ret= shm_locate_attached_block(shm_id, map);
  if (ret!=NULL)
     return ret;
  /* block not found ! , try to attach */
  return shm_attach_block(shm_id, -1, map);
}

static void forget_attached(int shmid)
{
  struct local_shm_map *curr, **point_to_curr;
  
  for (curr= shm_map,    point_to_curr= &shm_map ;
       curr != NULL ;
       curr= curr->next, point_to_curr= &curr->next ) {
     if (curr->shm_id == shmid) {
	*point_to_curr= curr->next;
	return;
     }
  }
}

/* delete chain of shm blocks (pointing to each other)
 * Do it in reverse order. (This is what the recursion is for)
 */
void shm_delete_chain(int *shmid)
{
  struct shm_block *block;

  if (*shmid == -1)
      return;

  block= shm_locate_block(*shmid, NULL);
  forget_attached( *shmid );
  if (block == NULL)
      return;
  shm_delete_chain(&block->next_shm_id);
  shmctl(*shmid, IPC_RMID, NULL);
  *shmid=-1;
  shmdt((char *)block);
}

#endif  /* CONFIG_IPC */
