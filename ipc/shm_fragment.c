/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      shm_fragment.c
 * Purpose:   Data fragments and free list items. Allocate and free blocks.
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#include <stdio.h>		   /* for debugging only */
#include <debug.h>		   /* for "stddeb" */

#include "shm_fragment.h"
#include "shm_block.h"

/******************************************************************************
 *
 * Free list: all fragments are ordered according to memory location.
 *            new fragments are inserted in this way.
 *
 ******************************************************************************
 */

#define FRAG_PTR(block,ofs) ((struct shm_fragment *) ((char *) block + ofs) )
#define NEXT_FRAG(block,ofs) ( FRAG_PTR(block,ofs)->info.next )

/* setup first item in the free list */
void shm_FragmentInit(struct shm_block *block,int first, int size)
{
  struct shm_fragment *fragment;
  
  /* round up to nearest 16 byte boundary */
  first=(first+15)& ~15;
  block->free_list=first;
  
  /* make all the block (exluding the header) free */
  fragment= FRAG_PTR(block, first);
  block->free=  fragment->size=  size-first;
  fragment->info.next=0;
}

void shm_FragPtrFree(struct shm_block *block, void *ptr)
{
  /* ptr points to fragment->info.data, find pointer to fragment,
   * find the offset of this pointer in block.
   */
  if (ptr)
     shm_FragmentFree(block, PTR2REL(block, ptr));
}
void shm_FragmentFree(struct shm_block *block, int fragment_ofs)
{
  struct shm_fragment *fragment=NULL;
  int prev;
  int next;

  fragment_ofs-=(int )&fragment->info.data;
  fragment= FRAG_PTR(block, fragment_ofs);

  block->free+=fragment->size;
  /* scan free list to find candidates for merging with fragment */
  for (prev=0, next=block->free_list;
       (next!=0) && (fragment_ofs > next)  ;
       prev=next, next=NEXT_FRAG(block,next) )
     ;
  
  /* insert fragment between, prev and next
   *  prev==0: fragment will be the first item in free list
   *  next==0: fragment will be the last item in free list
   */


  /* update fragment (point to next, or merge with next) */
  
  if ( fragment_ofs+fragment->size == next ) {
     /* merge with the next free block */
     fragment->size+=    FRAG_PTR(block,next)->size;
     fragment->info.next=FRAG_PTR(block,next)->info.next;
  } else 
     /* fragment should be inserted before the next fragment or end of */
     /* list. (not merged)  */
     fragment->info.next=next;
  /* now fragment has all the information about the rest of the list */


  /* upate prev fragment (point or merge with fragment) */
  
  if (prev==0) 		   /* first item in free list */
     block->free_list=fragment_ofs;
  else if ( prev+FRAG_PTR(block,prev)->size == fragment_ofs ) {
     /* merge fragment with previous fragment */
     FRAG_PTR(block,prev)->size+=    fragment->size;
     FRAG_PTR(block,prev)->info.next=fragment->info.next;
  } else 
     /* insert fragment after previous fragment */
     FRAG_PTR(block,prev)->info.next=fragment_ofs;
}

/* use "first fit" algorithm,
 * return: offset to data in fragment.
 */
int shm_FragmentAlloc(struct shm_block *block, int size)
{
  int prev;
  int candidate;
  struct shm_fragment *fragment;
  struct shm_fragment *ret_fragment;

  if (size <= 0)
     return NIL;
  /* add size of  "fragment->size" */
  size+= (char *)&fragment->info.data - (char *)fragment ;

  /* round "size" to nearest 16 byte value */
  size= (size+15) & ~15;
  if (size > block->free)
     return NIL;
  /* scan free list to find candidates for allocation */
  for (prev=0, candidate=block->free_list;
       candidate!=0 ;
       prev=candidate, candidate= fragment->info.next )
  { 
     fragment=FRAG_PTR(block,candidate);
     if (fragment->size >= size)
	break;
  }
  
  if (candidate == 0)
     return NIL;

  block->free-=size;
  if (fragment->size == size) {
     if (prev == 0) 
	block->free_list= fragment->info.next;
     else
	FRAG_PTR(block,prev)->info.next= fragment->info.next;
     return PTR2REL(block, &fragment->info.data);
  }

  /* fragment->size > size */

  /* Split fragment in two, return one part, put the other in free list.  */
  /* The part that starts at the old location - will stay in the free list. */
  fragment->size -= size;
  
  ret_fragment=FRAG_PTR(block, candidate + fragment->size);
  ret_fragment->size= size;
  return PTR2REL(block, ret_fragment->info.data);
}

/* like shm_FragmentAlloc, returns pointer instead of offset */
char *shm_FragPtrAlloc(struct shm_block *block, int size)
{
  int ofs;
  ofs= shm_FragmentAlloc(block,size);
  if (ofs == NIL)
     return NULL;
  else
     return (char *) REL2PTR(block, ofs);
}
/* This is used for debugging only */
void shm_print_free_list(struct shm_block *block)
{
  struct shm_fragment *fragment;
  int item;

  item=block->free_list;
  if (item==0) {
     fprintf(stddeb,"no free fragments");
  } else {
     for (; item ; item=fragment->info.next) {
	fragment=FRAG_PTR(block,item);
	fprintf(stddeb,"{0x%04x,0x%04x} ",item,fragment->size);
     }
  }
  fprintf(stddeb," [total free=%04x]\n",block->free);
  fflush(stddeb);
}

#endif  /* CONFIG_IPC */
