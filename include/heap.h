/* $Id: heap.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef HEAP_H
#define HEAP_H

#include "segmem.h"
#include "regfunc.h"
#include "atom.h"

/**********************************************************************
 * LOCAL HEAP STRUCTURES AND FUNCTIONS
 */
typedef struct heap_mem_desc_s
{
    struct heap_mem_desc_s *prev, *next;
    unsigned short length;
    unsigned char  lock;
    unsigned char  flags;
} MDESC;

typedef struct heap_local_heap_s
{
    struct heap_local_heap_s *next;
    MDESC *free_list;
    ATOMTABLE *local_table;
    unsigned short selector;
} LHEAP;

extern void HEAP_Init(MDESC **free_list, void *start, int length);
extern void *HEAP_Alloc(MDESC **free_list, int flags, int bytes);
extern int  HEAP_Free(MDESC **free_list, void *block);
extern void *HEAP_ReAlloc(MDESC **free_list, void *old_block, 
			  int new_size, unsigned int flags);
extern LHEAP *HEAP_LocalFindHeap(unsigned short owner);

#define HEAP_OWNER	(Segments[Stack16Frame[11] >> 3].owner)
#define LOCALHEAP()	(&HEAP_LocalFindHeap(HEAP_OWNER)->free_list)
#define LOCALATOMTABLE() (&HEAP_LocalFindHeap(HEAP_OWNER)->local_table)

/**********************************************************************
 * GLOBAL HEAP STRUCTURES AND FUNCTIONS:
 *
 * Global memory pool descriptor.  Segments MUST be maintained in segment
 * ascending order.  If not the reallocation routine will die a horrible
 * death.
 *
 * handle  = 0, this descriptor contains the address of a free pool.
 *        != 0, this describes an allocated block.
 *
 * sequence = 0, this is not a huge block
 *          > 0, this is a portion of a huge block
 *          =-1, this is a free segment
 *
 * addr	      - address of this memory block.
 *
 * length     - used to maintain huge blocks.
 */
typedef struct global_mem_desc_s
{
    struct global_mem_desc_s *next;
    struct global_mem_desc_s *prev;
    unsigned short handle;
    short sequence;
    void *addr;
    int length;
    int lock_count;
    void *linear_addr;
    int linear_key;
    int linear_count;
} GDESC;

extern GDESC *GlobalList;

extern void *GlobalQuickAlloc(int size);
extern unsigned int GlobalHandleFromPointer(void *block);
extern GDESC *GlobalGetGDesc(unsigned int block);
extern void *GlobalLinearLock(unsigned int block);
extern unsigned int GlobalLinearUnlock(unsigned int block);

#endif /* HEAP_H */
