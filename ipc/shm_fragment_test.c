/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_fragment_test.c
 * Purpose:   Test data fragments and free list items. Allocate and free blocks.
 ***************************************************************************
 */
#include <assert.h>
#include <stdio.h>		   
#include <stddebug.h>
#define DEBUG_DEFINE_VARIABLES	   /* just avoid dumb errors */
#include <debug.h>		   /* for "stddeb" */
#include <stdlib.h>
#include <string.h>
#include "shm_block.h"
#include "shm_fragment.h"
#include "xmalloc.h"

#define DO_FREE(id) (-id)
#define LIST_LENGTH 20

int main()
{
  struct shm_block *block;
  char *ret;
  int size;
  int i;

  /* important: The test will work only for the current implementation of */
  /* allocation, if the implementation will change, the list should also */
  /* cahnge. */
  static int sizes[LIST_LENGTH]={
    SHM_MINBLOCK,		   /* 0: should fail */
    0x3fe0-4,			   /* 1: */
    0x4000-4,			   /* 2: */
    0x4000-4,			   /* 3: */
    0x4000-4+1,			   /* 4: should fail */
    0x4000-4,			   /* 5: */
    /* allocated(5,3,2,1) free() */
    -5,				   /* 6: */
    0x1c00-4,			   /* 7: */
    0x1400-4,			   /* 8: */
    0x1000-4,			   /* 9: */
    /* allocated(9,8,7,3,2,1) free() */
    -9,				   /* 10: */
    -3,				   /* 11: */
    -1,				   /* 12: */
    /* allocated(8,7,2) free(9,3,1) */
    0x1000-4,			   /* 13: */
    -13,			   /* 14: */
    0x1000+1-4,			   /* 15: */
    /* allocated(8,7,15,2) free(9,[3-15],1) */
    -2,				   /* 16: */
    /* allocated(8,7,15) free(9,[3-15],1+2) */
    -8,				   /* 17: */
    -7,				   /* 18: */
    -15				   /* 19: */
    };
  
  static char *ptr[LIST_LENGTH];
  
  block=xmalloc(SHM_MINBLOCK);

  /* setup first item in the free list */
  shm_FragmentInit(block, sizeof(*block), SHM_MINBLOCK);
  
  fprintf(stddeb,"After shm_FragmentInit\n");
  shm_print_free_list(block);

  for(i=0 ; i < LIST_LENGTH; i++) {
     size=sizes[i];
     if (size>0) {		   /* allocate */
	ret=shm_FragPtrAlloc(block, size);
	ptr[i]=ret;
	fprintf(stddeb,
		"%d: After shm_FragmentAlloc(block, 0x%06x) == ",
		i, size);
	if (ret==NULL)
	   fprintf(stddeb, "NULL\n");
	else {
	   fprintf(stddeb, "0x%06x\n", (int)ret-(int)block);
	   memset( ret,0, size );  /* test boundaries */
	}
     } else {			   /* free */
	/* free shm fragment */
	ret=ptr[-sizes[i]];
	fprintf(stddeb, "%d: Doing shm_FragmentFree(block, ", i);
	if (ret==NULL)
	   fprintf(stddeb, "NULL)\n");
	else 
	   fprintf(stddeb, "0x%06x)\n", (int)ret-(int)block);
	fflush(stddeb);
	shm_FragPtrFree(block, ret);
     }
     shm_print_free_list(block);
  }
  return 0;
}
