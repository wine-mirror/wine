/* $Id: heap.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef HEAP_H
#define HEAP_H

#include "segmem.h"
#include "atom.h"
#include "stackframe.h"

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
    unsigned short delta;		/* Number saved for Windows compat. */
} LHEAP;

extern void HEAP_Init(MDESC **free_list, void *start, int length);
extern void *HEAP_Alloc(MDESC **free_list, int flags, int bytes);
extern int  HEAP_Free(MDESC **free_list, void *block);
extern void *HEAP_ReAlloc(MDESC **free_list, void *old_block, 
			  int new_size, unsigned int flags);

extern LHEAP *HEAP_LocalFindHeap(unsigned short owner);
extern unsigned int HEAP_LocalSize(MDESC **free_list, unsigned int handle);
extern void HEAP_LocalInit(unsigned short owner, void *start, int length);

extern void *WIN16_LocalAlloc(int flags, int bytes);
extern int WIN16_LocalCompact(int min_free);
extern unsigned int WIN16_LocalFlags(unsigned int handle);
extern unsigned int WIN16_LocalFree(unsigned int handle);
extern void *WIN16_LocalLock(unsigned int handle);
extern void *WIN16_LocalReAlloc(unsigned int handle, int flags, int bytes);
extern unsigned int WIN16_LocalUnlock(unsigned int handle);

/* Use ds instead of owner of cs */
#define HEAP_OWNER	(pStack16Frame->ds)
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
    struct global_mem_desc_s *next;	/* Next GDESC in list              */
    struct global_mem_desc_s *prev;	/* Previous GDESC in list          */
    unsigned short handle;		/* Handle of this block.	   */
    short          sequence;		/* Block sequence # in huge block  */
    void          *addr;		/* Address allocated with mmap()   */
    int            length;		/* Length of block		   */
    int            lock_count;		/* Block lock count		   */
    unsigned short alias;		/* Offset-zero alias selector      */
    unsigned int   alias_key;		/* Offset-zero alias sh. mem. key  */
    void          *linear_addr;		/* Linear address of huge block    */
    int            linear_key;		/* Linear shared memory key        */
    int            linear_count;	/* Linear lock count               */
} GDESC;

extern GDESC *GlobalList;

extern void *GlobalQuickAlloc(int size);
extern unsigned int GlobalHandleFromPointer(void *block);
extern GDESC *GlobalGetGDesc(unsigned int block);
extern void *GlobalLinearLock(unsigned int block);
extern unsigned int GlobalLinearUnlock(unsigned int block);

#endif /* HEAP_H */
