/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_block.ch
 * Purpose:   treat a shared memory block.
 ***************************************************************************
 */
#ifndef __WINE_SHM_BLOCK_H
#define __WINE_SHM_BLOCK_H

#ifdef CONFIG_IPC

#include <sys/shm.h>
#include "windef.h"
#define SEGSIZE                 0x10000 /* 64 */ 
#define SHM_GRANULARITY         SEGSIZE
#define SHM_MINBLOCK            SHM_GRANULARITY
#define SHM_MAXBLOCK            (((int)SHMMAX/(int)SHM_GRANULARITY)*  \
				 SHM_GRANULARITY)
#define PTR2REL(block,ptr) (REL_PTR) ( (char *) (ptr) - (char *) (block) )
#define REL2PTR(block,rel) (void *) ( (char *) (block) + (rel) )

typedef  int REL_PTR;

/* full info for each shm block. */
struct shm_block {
	/* private */
	int next_shm_id;	   /* IPC shm ID (for initial linking) */
    
	/* public (read only) */
	int size;		   /* size of the shm block */
	int free;		   /* how much of the block is free */
        int proc_idx;		   /* The index of the owner */
	
	/* public - writable for shm_fragment */
	REL_PTR free_list;	   /* first item in the free list */
};

/* used for mapping local attachments */
struct local_shm_map {
	struct local_shm_map *next;
	int shm_id;
	int proc_idx;
	
	/* 32 bit pointer to the beginning of the block */
	struct shm_block *ptr;
};
extern struct local_shm_map *shm_map;
void shm_setup_block(struct shm_block *block, REL_PTR first, int size);

/* shm_create_block:
 *   allocate and setup a new block:
 *   first - first non header byte.
 *   size  - block size (in bytes).
 *   shm_id- IPC shared memory ID.
 */
struct shm_block *shm_create_block(REL_PTR first, int size, int *shm_id);

/* shm_locate_block:
 *   locate existing block according to shm_id,
 *   Attach the block if needed. Assume the shm_id is wine's
 *   Set selectors also.
 */
struct shm_block *shm_locate_block(int shm_id, struct local_shm_map *map);

/* shm_locate_attached_block: 
 *   locate existing block according to shm_id,
 *   Blocks are never attached.
 * if proc_idx is not NULL, it will be set to owner's index.
 * map - localy mapped info about block may be NULL;
 */
struct shm_block *shm_locate_attached_block(int shm_id, 
					    struct local_shm_map *map);

/* shm_attach_block: attach existing shm block, setup selectors
 * shm_id - id of the block to attach.
 * proc_idx - if not -1, puts this data into local mapping
 * map - localy mapped info about this block. (may be NULL)
 * NOTE: same block can be attached many times
 */
struct shm_block *shm_attach_block(int shm_id, int proc_idx,
				   struct local_shm_map *map);

/* delete chain of shm blocks (pointing to each other */
void shm_delete_chain(int *shmid);

#endif  /* CONFIG_IPC */
#endif  /* __WINE_SHM_BLOCK_H */
