/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_fragment.h
 * Purpose:   Data fragments and free list items. Allocate and free blocks.
 ***************************************************************************
 */
#ifndef __WINE_SHM_FRAGMENT_H
#define __WINE_SHM_FRAGMENT_H

#include "shm_block.h"

#define NIL ((int) 0)
/* memory fragment: used or free (when free - it's an item of "free list",
 * when allocated it contains the data, and it's size) 
 */
struct shm_fragment {
	int size;		/* fragment's size */
	
	/* The type of info depends on fragment's status (free/allocated) */
	union info {		
		int next;	/* next free fragment */
		char data[1];	/* the data */
	} info;
};

/* setup first item in the free list */
void shm_FragmentInit(struct shm_block *block,REL_PTR first,int size);

/* allocate shm fragment. return: offset to data in fragment, or NULL */
REL_PTR shm_FragmentAlloc(struct shm_block *block, int size);

/* like shm_FragmentAlloc, returns pointer instead of offset */
char *shm_FragPtrAlloc(struct shm_block *block, int size);

/* free shm fragment - according to offset */
void shm_FragmentFree(struct shm_block *block, int ofs);

/* free shm fragment - according to pointer */
void shm_FragPtrFree(struct shm_block *block, void *ptr);

/* This is used for debugging only */
void shm_print_free_list(struct shm_block *block);

#endif /* __WINE_SHM_FRAGMENT_H */
