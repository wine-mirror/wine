/*
 * Local heap functions
 *
 * Copyright 1995 Alexandre Julliard
 */

/*
 * Note:
 * All local heap functions need the current DS as first parameter
 * when called from the emulation library, so they take one more
 * parameter than usual.
 */

#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "ldt.h"
#include "instance.h"
#include "local.h"
#include "module.h"
#include "stackframe.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"


#ifndef WINELIB
#pragma pack(1)
#endif

typedef struct
{
/* Arena header */
    WORD prev;          /* Previous arena | arena type */
    WORD next;          /* Next arena */
/* Start of the memory block or free-list info */
    WORD size;          /* Size of the free block */
    WORD free_prev;     /* Previous free block */
    WORD free_next;     /* Next free block */
} LOCALARENA;

#define ARENA_HEADER_SIZE      4
#define ARENA_HEADER( handle) ( ((handle) & ~3) - ARENA_HEADER_SIZE)

  /* Arena types (stored in 'prev' field of the arena) */
#define LOCAL_ARENA_FREE       0
#define LOCAL_ARENA_FIXED      1
#define LOCAL_ARENA_MOVEABLE   3

/* Layout of a handle entry table
 *
 * WORD                     count of entries
 * LOCALHANDLEENTRY[count]  entries
 * WORD                     near ptr to next table
 */
typedef struct
{
    WORD addr;                /* Address of the MOVEABLE block */
    BYTE flags;               /* Flags for this block */
    BYTE lock;                /* Lock count */
} LOCALHANDLEENTRY;

typedef struct
{
    WORD check;                 /* 00 Heap checking flag */
    WORD freeze;                /* 02 Heap frozen flag */
    WORD items;                 /* 04 Count of items on the heap */
    WORD first;                 /* 06 First item of the heap */
    WORD pad1;                  /* 08 Always 0 */
    WORD last;                  /* 0a Last item of the heap */
    WORD pad2;                  /* 0c Always 0 */
    BYTE ncompact;              /* 0e Compactions counter */
    BYTE dislevel;              /* 0f Discard level */
    DWORD distotal;             /* 10 Total bytes discarded */
    WORD htable;                /* 14 Pointer to handle table */
    WORD hfree;                 /* 16 Pointer to free handle table */
    WORD hdelta;                /* 18 Delta to expand the handle table */
    WORD expand;                /* 1a Pointer to expand function (unused) */
    WORD pstat;                 /* 1c Pointer to status structure (unused) */
    FARPROC notify WINE_PACKED; /* 1e Pointer to LocalNotify() function */
    WORD lock;                  /* 22 Lock count for the heap */
    WORD extra;                 /* 24 Extra bytes to allocate when expanding */
    WORD minsize;               /* 26 Minimum size of the heap */
    WORD magic;                 /* 28 Magic number */
} LOCALHEAPINFO;

#ifndef WINELIB
#pragma pack(4)
#endif

#define LOCAL_HEAP_MAGIC  0x484c  /* 'LH' */


  /* All local heap allocations are aligned on 4-byte boundaries */
#define LALIGN(word)          (((word) + 3) & ~3)

#define ARENA_PTR(ptr,arena)       ((LOCALARENA *)((char*)(ptr)+(arena)))
#define ARENA_PREV(ptr,arena)      (ARENA_PTR(ptr,arena)->prev & ~3)
#define ARENA_NEXT(ptr,arena)      (ARENA_PTR(ptr,arena)->next)
#define ARENA_FLAGS(ptr,arena)     (ARENA_PTR(ptr,arena)->prev & 3)

  /* determine whether the handle belongs to a fixed or a moveable block */
#define HANDLE_FIXED(handle) (((handle) & 3) == 0)
#define HANDLE_MOVEABLE(handle) (((handle) & 3) == 2)

/***********************************************************************
 *           LOCAL_GetHeap
 *
 * Return a pointer to the local heap, making sure it exists.
 */
static LOCALHEAPINFO *LOCAL_GetHeap( WORD ds )
{
    LOCALHEAPINFO *pInfo;
    INSTANCEDATA *ptr = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( ds, 0 );
    dprintf_local( stddeb, "Heap at %p, %04x\n", ptr, ptr->heap );
    if (!ptr->heap) return NULL;
    if (IsBadReadPtr((SEGPTR)MAKELONG( ptr->heap, ds ), sizeof(LOCALHEAPINFO)))
        return NULL;
    pInfo = (LOCALHEAPINFO*)((char*)ptr + ptr->heap);
    if (pInfo->magic != LOCAL_HEAP_MAGIC) return NULL;
    return pInfo;
}


/***********************************************************************
 *           LOCAL_MakeBlockFree
 *
 * Make a block free, inserting it in the free-list.
 * 'block' is the handle of the block arena; 'baseptr' points to
 * the beginning of the data segment containing the heap.
 */
static void LOCAL_MakeBlockFree( char *baseptr, WORD block )
{
    LOCALARENA *pArena, *pNext;
    WORD next;

      /* Mark the block as free */

    pArena = ARENA_PTR( baseptr, block );
    pArena->prev = (pArena->prev & ~3) | LOCAL_ARENA_FREE;
    pArena->size = pArena->next - block;
    
      /* Find the next free block (last block is always free) */

    next = pArena->next;
    for (;;)
    {
        pNext = ARENA_PTR( baseptr, next );
        if ((pNext->prev & 3) == LOCAL_ARENA_FREE) break;
        next = pNext->next;
    }

    dprintf_local( stddeb, "Local_AddFreeBlock %04x, next %04x\n", block, next );
      /* Insert the free block in the free-list */

    pArena->free_prev = pNext->free_prev;
    pArena->free_next = next;
    ARENA_PTR(baseptr,pNext->free_prev)->free_next = block;
    pNext->free_prev  = block;
}


/***********************************************************************
 *           LOCAL_RemoveFreeBlock
 *
 * Remove a block from the free-list.
 * 'block' is the handle of the block arena; 'baseptr' points to
 * the beginning of the data segment containing the heap.
 */
static void LOCAL_RemoveFreeBlock( char *baseptr, WORD block )
{
      /* Mark the block as fixed */

    LOCALARENA *pArena = ARENA_PTR( baseptr, block );
    pArena->prev = (pArena->prev & ~3) | LOCAL_ARENA_FIXED;

      /* Remove it from the list */

    ARENA_PTR(baseptr,pArena->free_prev)->free_next = pArena->free_next;
    ARENA_PTR(baseptr,pArena->free_next)->free_prev = pArena->free_prev;
}


/***********************************************************************
 *           LOCAL_AddBlock
 *
 * Insert a new block in the heap.
 * 'new' is the handle of the new block arena; 'baseptr' points to
 * the beginning of the data segment containing the heap; 'prev' is
 * the block before the new one.
 */
static void LOCAL_AddBlock( char *baseptr, WORD prev, WORD new )
{
    LOCALARENA *pPrev = ARENA_PTR( baseptr, prev );
    LOCALARENA *pNew  = ARENA_PTR( baseptr, new );

    pNew->prev = prev | LOCAL_ARENA_FIXED;
    pNew->next = pPrev->next;
    ARENA_PTR(baseptr,pPrev->next)->prev &= 3;
    ARENA_PTR(baseptr,pPrev->next)->prev |= new;
    pPrev->next = new;
}


/***********************************************************************
 *           LOCAL_RemoveBlock
 *
 * Remove a block from the heap.
 * 'block' is the handle of the block arena; 'baseptr' points to
 * the beginning of the data segment containing the heap.
 */
static void LOCAL_RemoveBlock( char *baseptr, WORD block )
{
    LOCALARENA *pArena, *pTmp;

      /* Remove the block from the free-list */

    dprintf_local( stddeb, "Local_RemoveBlock\n");
    pArena = ARENA_PTR( baseptr, block );
    if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
        LOCAL_RemoveFreeBlock( baseptr, block );

      /* If the previous block is free, expand its size */

    pTmp = ARENA_PTR( baseptr, pArena->prev & ~3 );
    if ((pTmp->prev & 3) == LOCAL_ARENA_FREE)
        pTmp->size += pArena->next - block;

      /* Remove the block from the linked list */

    pTmp->next = pArena->next;
    pTmp = ARENA_PTR( baseptr, pArena->next );
    pTmp->prev = (pTmp->prev & 3) | (pArena->prev & ~3);
}


/***********************************************************************
 *           LOCAL_PrintHeap
 */
static void LOCAL_PrintHeap( WORD ds )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );
    WORD arena;

    if (!debugging_local) return;
    if (!pInfo)
    {
        printf( "Local Heap corrupted!  ds=%04x\n", ds );
        return;
    }
    printf( "Local Heap  ds=%04x first=%04x last=%04x items=%d\n",
            ds, pInfo->first, pInfo->last, pInfo->items );

    arena = pInfo->first;
    for (;;)
    {
        LOCALARENA *pArena = ARENA_PTR(ptr,arena);
        printf( "  %04x: prev=%04x next=%04x type=%d\n", arena,
                pArena->prev & ~3, pArena->next, pArena->prev & 3 );
        if (arena == pInfo->first)
	{
            printf( "        size=%d free_prev=%04x free_next=%04x\n",
                    pArena->size, pArena->free_prev, pArena->free_next );
	}
        if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
        {
            printf( "        size=%d free_prev=%04x free_next=%04x\n",
                    pArena->size, pArena->free_prev, pArena->free_next );
            if (pArena->next == arena) break;  /* last one */
            if (ARENA_PTR(ptr,pArena->free_next)->free_prev != arena)
            {
                printf( "*** arena->free_next->free_prev != arena\n" );
                break;
            }
        }
        if (pArena->next == arena)
        {
            printf( "*** last block is not marked free\n" );
            break;
        }
        if ((ARENA_PTR(ptr,pArena->next)->prev & ~3) != arena)
        {
            printf( "*** arena->next->prev != arena (%04x, %04x)\n",
		   pArena->next, ARENA_PTR(ptr,pArena->next)->prev);
            break;
        }
        arena = pArena->next;
    }
}


/***********************************************************************
 *           LocalInit   (KERNEL.4)
 */
BOOL LocalInit( HANDLE selector, WORD start, WORD end )
{
    char *ptr;
    WORD heapInfoArena, freeArena, lastArena;
    LOCALHEAPINFO *pHeapInfo;
    LOCALARENA *pArena, *pFirstArena, *pLastArena;
    NE_MODULE *pModule;
    
      /* The initial layout of the heap is: */
      /* - first arena         (FIXED)      */
      /* - heap info structure (FIXED)      */
      /* - large free block    (FREE)       */
      /* - last arena          (FREE)       */

    dprintf_local(stddeb, "LocalInit: %04x %04x-%04x\n", selector, start, end);
    if (!selector) selector = CURRENT_DS;
    pHeapInfo = LOCAL_GetHeap(selector);

    if (pHeapInfo)  {
      fprintf( stderr, "LocalInit: Heap %04x initialized twice.\n", selector);
      if (debugging_local) LOCAL_PrintHeap(selector);
    }

    if (start == 0) {
      /* Check if the segment is the DGROUP of a module */

	if ((pModule = MODULE_GetPtr( GetExePtr( selector ) )))
	{
	    SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
	    if (pModule->dgroup && (pSeg->selector == selector)) {
		/* We can't just use the simple method of using the value
                 * of minsize + stacksize, since there are programs that
                 * resize the data segment before calling InitTask(). So,
                 * we must put it at the end of the segment */
		start = GlobalSize( GlobalHandle( selector ) );
		start -= end;
		end += start;
		dprintf_local( stddeb," new start %04x, minstart: %04x\n", start, pSeg->minsize + pModule->stack_size);
	    }
	}
    }
    ptr = PTR_SEG_OFF_TO_LIN( selector, 0 );

    start = LALIGN( MAX( start, sizeof(INSTANCEDATA) ) );
    heapInfoArena = LALIGN(start + sizeof(LOCALARENA) );
    freeArena = LALIGN( heapInfoArena + ARENA_HEADER_SIZE
                        + sizeof(LOCALHEAPINFO) );
    lastArena = (end - sizeof(LOCALARENA)) & ~3;

      /* Make sure there's enough space.       */

    if (freeArena + sizeof(LOCALARENA) >= lastArena) return FALSE;

      /* Initialise the first arena */

    pFirstArena = ARENA_PTR( ptr, start );
    pFirstArena->prev      = start | LOCAL_ARENA_FIXED;
    pFirstArena->next      = heapInfoArena;
    pFirstArena->size      = LALIGN(sizeof(LOCALARENA));
    pFirstArena->free_prev = start;  /* this one */
    pFirstArena->free_next = freeArena;

      /* Initialise the arena of the heap info structure */

    pArena = ARENA_PTR( ptr, heapInfoArena );
    pArena->prev = start | LOCAL_ARENA_FIXED;
    pArena->next = freeArena;

      /* Initialise the heap info structure */

    pHeapInfo = (LOCALHEAPINFO *) (ptr + heapInfoArena + ARENA_HEADER_SIZE );
    memset( pHeapInfo, 0, sizeof(LOCALHEAPINFO) );
    pHeapInfo->items   = 4;
    pHeapInfo->first   = start;
    pHeapInfo->last    = lastArena;
    pHeapInfo->htable  = 0;
    pHeapInfo->hdelta  = 0x20;
    pHeapInfo->extra   = 0x200;
    pHeapInfo->minsize = lastArena - freeArena;
    pHeapInfo->magic   = LOCAL_HEAP_MAGIC;

      /* Initialise the large free block */

    pArena = ARENA_PTR( ptr, freeArena );
    pArena->prev      = heapInfoArena | LOCAL_ARENA_FREE;
    pArena->next      = lastArena;
    pArena->size      = lastArena - freeArena;
    pArena->free_prev = start;
    pArena->free_next = lastArena;

      /* Initialise the last block */

    pLastArena = ARENA_PTR( ptr, lastArena );
    pLastArena->prev      = freeArena | LOCAL_ARENA_FREE;
    pLastArena->next      = lastArena;  /* this one */
    pLastArena->size      = LALIGN(sizeof(LOCALARENA));
    pLastArena->free_prev = freeArena;
    pLastArena->free_next = lastArena;  /* this one */

      /* Store the local heap address in the instance data */

    ((INSTANCEDATA *)ptr)->heap = heapInfoArena + ARENA_HEADER_SIZE;
    LOCAL_PrintHeap( selector );
    return TRUE;
}

/***********************************************************************
 *           LOCAL_GrowHeap
 */
static void LOCAL_GrowHeap( WORD ds )
{
    HANDLE hseg = GlobalHandle( ds );
    LONG oldsize = GlobalSize( hseg );
    LONG end;
    LOCALHEAPINFO *pHeapInfo;
    WORD freeArena, lastArena;
    LOCALARENA *pArena, *pLastArena;
    char *ptr;
    
    /* if nothing can be gained, return */
    if (oldsize > 0xfff0) return;
    hseg = GlobalReAlloc( hseg, 0x10000, GMEM_FIXED );
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    pHeapInfo = LOCAL_GetHeap( ds );
    if (pHeapInfo == NULL) {
	fprintf( stderr, "Local_GrowHeap: heap not found\n" );
	return;
    }
    end = GlobalSize( hseg );
    lastArena = (end - sizeof(LOCALARENA)) & ~3;

      /* Update the HeapInfo */
    pHeapInfo->items++;
    freeArena = pHeapInfo->last;
    pHeapInfo->last = lastArena;
    pHeapInfo->minsize += end - oldsize;
    
      /* grow the old last block */
      /* FIXME: merge two adjacent free blocks */
    pArena = ARENA_PTR( ptr, freeArena );
    pArena->size      = lastArena - freeArena;
    pArena->next      = lastArena;
    pArena->free_next = lastArena;
    
      /* Initialise the new last block */

    pLastArena = ARENA_PTR( ptr, lastArena );
    pLastArena->prev      = freeArena | LOCAL_ARENA_FREE;
    pLastArena->next      = lastArena;  /* this one */
    pLastArena->size      = LALIGN(sizeof(LOCALARENA));
    pLastArena->free_prev = freeArena;
    pLastArena->free_next = lastArena;  /* this one */
    
    dprintf_local( stddeb, "Heap expanded\n" );
    LOCAL_PrintHeap( ds );
}

/***********************************************************************
 *           LOCAL_Compact
 */
static WORD LOCAL_Compact( WORD ds, WORD minfree, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;
    WORD freespace = 0;
    
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local_FindFreeBlock: Local heap not found\n" );
	LOCAL_PrintHeap(ds);
	return 0;
    }

    arena = pInfo->first;
    pArena = ARENA_PTR( ptr, arena );
    while (arena != pArena->free_next) {
        arena = pArena->free_next;
        pArena = ARENA_PTR( ptr, arena );
        if (pArena->size >= freespace) freespace = pArena->size;
    }

    if(freespace < ARENA_HEADER_SIZE)
	freespace = 0;
    else
	freespace -= ARENA_HEADER_SIZE;
    
    if (flags & LMEM_NOCOMPACT) return freespace;
    
    if (flags & LMEM_NODISCARD) return freespace;
    return freespace;
}

/***********************************************************************
 *           LOCAL_FindFreeBlock
 */
static HLOCAL LOCAL_FindFreeBlock( WORD ds, WORD size )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local_FindFreeBlock: Local heap not found\n" );
	LOCAL_PrintHeap(ds);
	return 0;
    }

    arena = pInfo->first;
    pArena = ARENA_PTR( ptr, arena );
    for (;;) {
        arena = pArena->free_next;
        pArena = ARENA_PTR( ptr, arena );
	if (arena == pArena->free_next) break;
        if (pArena->size >= size) return arena;
    }
    dprintf_local( stddeb, "Local_FindFreeBlock: not enough space\n" );
    if (debugging_local) LOCAL_PrintHeap(ds);
    return 0;
}

/***********************************************************************
 *           LOCAL_GetBlock
 * The segment may get moved around in this function, so all callers
 * should reset their pointer variables.
 */
static HLOCAL LOCAL_GetBlock( WORD ds, WORD size, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local_GetBlock: Local heap not found\n");
	LOCAL_PrintHeap(ds);
	return 0;
    }
    
    size += ARENA_HEADER_SIZE;
    size = LALIGN( MAX( size, sizeof(LOCALARENA) ) );

      /* Find a suitable free block */
    arena = LOCAL_FindFreeBlock( ds, size );
    if (arena == 0) {
	/* no space: try to make some */
	LOCAL_Compact( ds, size, flags );
	arena = LOCAL_FindFreeBlock( ds, size );
    }
    if (arena == 0) {
	/* still no space: try to grow the segment */
	LOCAL_GrowHeap( ds );
	ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
	pInfo = LOCAL_GetHeap( ds );
	arena = LOCAL_FindFreeBlock( ds, size );
    }
    if (arena == 0) {
	fprintf( stderr, "Local_GetBlock: not enough space in heap %04x for %d bytes\n",
                 ds, size );
	return 0;
    }

      /* Make a block out of the free arena */
    pArena = ARENA_PTR( ptr, arena );
    dprintf_local( stddeb, "LOCAL_GetBlock size = %04x, arena at %04x size %04x\n", size,
		   arena, pArena->size );
    if (pArena->size > size + LALIGN(sizeof(LOCALARENA)))
    {
        LOCAL_AddBlock( ptr, arena, arena+size );
        LOCAL_MakeBlockFree( ptr, arena+size );
        pInfo->items++;
    }
    LOCAL_RemoveFreeBlock( ptr, arena );

    if (flags & LMEM_ZEROINIT) {
	memset( (char *)pArena + ARENA_HEADER_SIZE, 0, size - ARENA_HEADER_SIZE );
    }

    dprintf_local( stddeb, "Local_GetBlock: arena at %04x\n", arena );
    return arena + ARENA_HEADER_SIZE;
}


/***********************************************************************
 *           LOCAL_NewHTable
 */
static BOOL LOCAL_NewHTable( WORD ds )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALHANDLEENTRY *pEntry;
    HLOCAL handle;
    int i;

    dprintf_local( stddeb, "Local_NewHTable\n" );
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local heap not found\n");
        LOCAL_PrintHeap(ds);
        return FALSE;
    }

    handle = LOCAL_GetBlock( ds, pInfo->hdelta * sizeof(LOCALHANDLEENTRY)
                                  + 2 * sizeof(WORD), LMEM_FIXED );
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    pInfo = LOCAL_GetHeap( ds );
    if (handle == 0) return FALSE;

    /* Fill the entry table */

    *(WORD *)(ptr + handle) = pInfo->hdelta;
    pEntry = (LOCALHANDLEENTRY *)(ptr + handle + sizeof(WORD));
    for (i = pInfo->hdelta; i > 0; i--) (pEntry++)->lock = 0xff;
    *(WORD *)pEntry = pInfo->htable;
    pInfo->htable = handle;
    return TRUE;
}


/***********************************************************************
 *           LOCAL_GetNewHandleEntry
 */
static HLOCAL LOCAL_GetNewHandleEntry( WORD ds )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALHANDLEENTRY *pEntry = NULL;
    WORD table;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_GetNewHandleEntry: Local heap not found\n");
	LOCAL_PrintHeap(ds);
	return 0;
    }

    /* Find a free slot in existing tables */

    table = pInfo->htable;
    while (table)
    {
        WORD count = *(WORD *)(ptr + table);
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for (; count > 0; count--, pEntry++)
            if (pEntry->lock == 0xff) break;
        if (count) break;
        table = *(WORD *)pEntry;
    }

    if (!table)  /* We need to create a new table */
    {
        if (!LOCAL_NewHTable( ds )) return 0;
	ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
	pInfo = LOCAL_GetHeap( ds );
        pEntry = (LOCALHANDLEENTRY *)(ptr + pInfo->htable + sizeof(WORD));
    }

    /* Now allocate this entry */

    pEntry->lock = 0;
    dprintf_local( stddeb, "LOCAL_GetNewHandleEntry(%04x): %04x\n",
                   ds, ((char *)pEntry - ptr) );
    return (HLOCAL)((char *)pEntry - ptr);
}


/***********************************************************************
 *           LOCAL_FreeArena
 */
static HLOCAL LOCAL_FreeArena( WORD ds, WORD arena )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pPrev, *pNext;

    dprintf_local( stddeb, "LocalFreeArena: %04x ds=%04x\n", arena, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return arena;

    pArena = ARENA_PTR( ptr, arena );
    if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
    {
	/* shouldn't happen */
        fprintf( stderr, "LocalFreeArena: Trying to free block %04x twice!\n",
                 arena );
	LOCAL_PrintHeap( ds );
	return arena;
    }

      /* Check if we can merge with the previous block */

    pPrev = ARENA_PTR( ptr, pArena->prev & ~3 );
    pNext = ARENA_PTR( ptr, pArena->next );
    if ((pPrev->prev & 3) == LOCAL_ARENA_FREE)
    {
        arena  = pArena->prev & ~3;
        pArena = pPrev;
        LOCAL_RemoveBlock( ptr, pPrev->next );
        pInfo->items--;
    }
    else  /* Make a new free block */
    {
        LOCAL_MakeBlockFree( ptr, arena );
    }

      /* Check if we can merge with the next block */

    if ((pArena->next == pArena->free_next) &&
        (pArena->next != pInfo->last))
    {
        LOCAL_RemoveBlock( ptr, pArena->next );
        pInfo->items--;
    }
    return 0;
}


/***********************************************************************
 *           LOCAL_FreeHandleEntry
 *
 * Free a handle table entry.
 */
static void LOCAL_FreeHandleEntry( WORD ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
    LOCALHEAPINFO *pInfo;
    WORD *pTable;
    WORD table, count, i;

    if (!(pInfo = LOCAL_GetHeap( ds ))) return;

    /* Find the table where this handle comes from */

    pTable = &pInfo->htable;
    while (*pTable)
    {
        WORD size = (*(WORD *)(ptr + *pTable)) * sizeof(LOCALHANDLEENTRY);
        if ((handle >= *pTable + sizeof(WORD)) &&
            (handle < *pTable + sizeof(WORD) + size)) break;  /* Found it */
        pTable = (WORD *)(ptr + *pTable + sizeof(WORD) + size);
    }
    if (!*pTable)
    {
        fprintf(stderr, "LOCAL_FreeHandleEntry: invalid entry %04x\n", handle);
        LOCAL_PrintHeap( ds );
        return;
    }

    /* Make the entry free */

    pEntry->addr = 0;  /* just in case */
    pEntry->lock = 0xff;

    /* Now check if all entries in this table are free */

    pEntry = (LOCALHANDLEENTRY *)(ptr + *pTable + sizeof(WORD));
    count = *(WORD *)(ptr + *pTable);
    for (i = count; i > 0; i--, pEntry++) if (pEntry->lock != 0xff) return;
    
    /* Remove the table from the linked list and free it */

    table = *pTable;
    dprintf_local( stddeb, "LOCAL_FreeHandleEntry(%04x): freeing table %04x\n",
                   ds, table);
    *pTable = *((WORD *)(ptr + count * sizeof(*pEntry)) + 1);
    LOCAL_FreeArena( ds, ARENA_HEADER( table ) );
}


/***********************************************************************
 *           LOCAL_Free
 *
 * Implementation of LocalFree().
 */
HLOCAL LOCAL_Free( HANDLE ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );

    dprintf_local( stddeb, "LocalFree: %04x ds=%04x\n", handle, ds );
    
    if (!handle) { fprintf( stderr, "LOCAL_Free: handle is 0.\n" ); return 0; }
    if (HANDLE_FIXED( handle ))
    {
        if (!LOCAL_FreeArena( ds, ARENA_HEADER( handle ) )) return 0;  /* OK */
        else return handle;  /* couldn't free it */
    }
    else
    {
	WORD arena = ARENA_HEADER( *(WORD *)(ptr + handle) );
        dprintf_local( stddeb, "LocalFree: real block at %04x\n", arena );
        if (LOCAL_FreeArena( ds, arena )) return handle; /* couldn't free it */
        LOCAL_FreeHandleEntry( ds, handle );
        return 0;  /* OK */
    }
}


/***********************************************************************
 *           LOCAL_Alloc
 *
 * Implementation of LocalAlloc().
 */
HLOCAL LOCAL_Alloc( HANDLE ds, WORD flags, WORD size )
{
    char *ptr;
    HLOCAL handle;
    
    dprintf_local( stddeb, "LocalAlloc: %04x %d ds=%04x\n", flags, size, ds );

    if (flags & LMEM_MOVEABLE)
    {
	LOCALHANDLEENTRY *plhe;
	HLOCAL hmem;
	
	if (!(hmem = LOCAL_GetBlock( ds, size, flags ))) return 0;
	if (!(handle = LOCAL_GetNewHandleEntry( ds )))
        {
	    fprintf( stderr, "LocalAlloc: couldn't get handle\n");
	    LOCAL_FreeArena( ds, ARENA_HEADER(hmem) );
	    return 0;
	}
	ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
	plhe = (LOCALHANDLEENTRY *)(ptr + handle);
	plhe->addr = hmem;
        plhe->flags = (BYTE)(flags >> 8);
	plhe->lock = 0;
    }
    else handle = LOCAL_GetBlock( ds, size, flags );

    return handle;
}


/***********************************************************************
 *           LOCAL_ReAlloc
 *
 * Implementation of LocalReAlloc().
 */
HLOCAL LOCAL_ReAlloc( HANDLE ds, HLOCAL handle, WORD size, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pNext;
    WORD arena, newhandle, blockhandle;
    LONG nextarena;

    if (!handle) return LOCAL_Alloc( ds, size, flags );

    dprintf_local( stddeb, "LocalReAlloc: %04x %d %04x ds=%04x\n",
                   handle, size, flags, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return 0;
    
    if (HANDLE_FIXED( handle )) blockhandle = handle;
    else blockhandle = *(WORD *)(ptr + handle);

    arena = ARENA_HEADER( blockhandle );
    dprintf_local( stddeb, "LocalReAlloc: arena is %04x\n", arena );
    pArena = ARENA_PTR( ptr, arena );
    
    if (flags & LMEM_MODIFY) {
      dprintf_local( stddeb, "LMEM_MODIFY set\n");
      return handle;
    }
    if (!size) size = 1;
    size = LALIGN( size );
    nextarena = LALIGN(blockhandle + size);

      /* Check for size reduction */

    if (nextarena < pArena->next)
    {
        if (nextarena < pArena->next - LALIGN(sizeof(LOCALARENA)))
        {
	    dprintf_local( stddeb, "size reduction, making new free block\n");
              /* It is worth making a new free block */
            LOCAL_AddBlock( ptr, arena, nextarena );
            LOCAL_MakeBlockFree( ptr, nextarena );
            pInfo->items++;
        }
        dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", handle );
        return handle;
    }

      /* Check if the next block is free */

    pNext = ARENA_PTR( ptr, pArena->next );
    if (((pNext->prev & 3) == LOCAL_ARENA_FREE) &&
        (nextarena <= pNext->next))
    {
        LOCAL_RemoveBlock( ptr, pArena->next );
        if (nextarena < pArena->next - LALIGN(sizeof(LOCALARENA)))
        {
	    dprintf_local( stddeb, "size increase, making new free block\n");
              /* It is worth making a new free block */
            LOCAL_AddBlock( ptr, arena, nextarena );
            LOCAL_MakeBlockFree( ptr, nextarena );
            pInfo->items++;
        }
        dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", handle );
        return handle;
    }

      /* Now we have to allocate a new block */

    newhandle = LOCAL_GetBlock( ds, size, flags );
    if (newhandle == 0) return 0;
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    memcpy( ptr + newhandle, ptr + (arena + ARENA_HEADER_SIZE), size );
    LOCAL_FreeArena( ds, arena );
    if (HANDLE_MOVEABLE( handle ))
    {
	dprintf_local( stddeb, "LocalReAlloc: fixing handle\n");
	*(WORD *)(ptr + handle) = newhandle;
	newhandle = handle;
    }
    dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", newhandle );
    return newhandle;
}


/***********************************************************************
 *           LOCAL_InternalLock
 */
static HLOCAL LOCAL_InternalLock( LPSTR heap, HLOCAL handle )
{
    dprintf_local( stddeb, "LocalLock: %04x ", handle );
    if (HANDLE_MOVEABLE(handle))
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(heap + handle);
        if (pEntry->lock < 0xfe) pEntry->lock++;
        handle = pEntry->addr;
    }
    dprintf_local( stddeb, "returning %04x\n", handle );
    return handle;
}


/***********************************************************************
 *           LOCAL_Lock
 */
LPSTR LOCAL_Lock( HANDLE ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    return handle ? ptr + LOCAL_InternalLock( ptr, handle ) : NULL;
}


/***********************************************************************
 *           LOCAL_Unlock
 */
BOOL LOCAL_Unlock( WORD ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );

    dprintf_local( stddeb, "LocalUnlock: %04x\n", handle );
    if (HANDLE_MOVEABLE(handle))
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        if (!pEntry->lock || (pEntry->lock == 0xff)) return FALSE;
        /* For moveable block, return the new lock count */
        /* (see _Windows_Internals_ p. 197) */
        return --pEntry->lock;
    }
    else return FALSE;
}


/***********************************************************************
 *           LOCAL_Size
 *
 * Implementation of LocalSize().
 */
WORD LOCAL_Size( WORD ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( CURRENT_DS, 0 );
    LOCALARENA *pArena;

    dprintf_local( stddeb, "LocalSize: %04x ds=%04x\n", handle, ds );

    if (HANDLE_MOVEABLE( handle )) handle = *(WORD *)(ptr + handle);
    pArena = ARENA_PTR( ptr, ARENA_HEADER(handle) );
    return pArena->next - handle;
}


/***********************************************************************
 *           LOCAL_Flags
 *
 * Implementation of LocalFlags().
 */
WORD LOCAL_Flags( WORD ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );

    if (HANDLE_MOVEABLE(handle))
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        dprintf_local( stddeb, "LOCAL_Flags(%04x,%04x): returning %04x\n",
                       ds, handle, pEntry->lock | (pEntry->flags << 8) );
        return pEntry->lock | (pEntry->flags << 8);
    }
    else
    {
        dprintf_local( stddeb, "LOCAL_Flags(%04x,%04x): returning 0\n",
                       ds, handle );
        return 0;
    }
}


/***********************************************************************
 *           LOCAL_HeapSize
 *
 * Implementation of LocalHeapSize().
 */
WORD LOCAL_HeapSize( WORD ds )
{
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );
    if (!pInfo) return 0;
    return pInfo->last - pInfo->first;
}


/***********************************************************************
 *           LOCAL_CountFree
 *
 * Implementation of LocalCountFree().
 */
WORD LOCAL_CountFree( WORD ds )
{
    WORD arena, total;
    LOCALARENA *pArena;
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );

    total = 0;
    arena = pInfo->first;
    pArena = ARENA_PTR( ptr, arena );
    for (;;)
    {
        arena = pArena->free_next;
        pArena = ARENA_PTR( ptr, arena );
	if (arena == pArena->free_next) break;
        total += pArena->size;
    }
    dprintf_local( stddeb, "LOCAL_CountFree(%04x): returning %d\n", ds, total);
    return total;
}


/***********************************************************************
 *           LOCAL_Handle
 *
 * Implementation of LocalHandle().
 */
HLOCAL LOCAL_Handle( WORD ds, WORD addr )
{ 
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    WORD table;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_Handle(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }

    /* Find the address in the entry tables */

    table = pInfo->htable;
    while (table)
    {
        WORD count = *(WORD *)(ptr + table);
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY*)(ptr+table+sizeof(WORD));
        for (; count > 0; count--, pEntry++)
            if (pEntry->addr == addr) return (HLOCAL)((char *)pEntry - ptr);
        table = *(WORD *)pEntry;
    }

    return (HLOCAL)addr;  /* Fixed block handle is addr */
}


/***********************************************************************
 *           LocalAlloc   (KERNEL.5)
 */
HLOCAL LocalAlloc( WORD flags, WORD size )
{
    return LOCAL_Alloc( CURRENT_DS, flags, size );
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL.6)
 */
HLOCAL LocalReAlloc( HLOCAL handle, WORD size, WORD flags )
{
    return LOCAL_ReAlloc( CURRENT_DS, handle, size, flags );
}


/***********************************************************************
 *           LocalFree   (KERNEL.7)
 */
HLOCAL LocalFree( HLOCAL handle )
{
    return LOCAL_Free( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalLock   (KERNEL.8)
 */
NPVOID LocalLock( HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( CURRENT_DS, 0 );
    return (NPVOID)LOCAL_InternalLock( ptr, handle );
}


/***********************************************************************
 *           LocalUnlock   (KERNEL.9)
 */
BOOL LocalUnlock( HLOCAL handle )
{
    return LOCAL_Unlock( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalSize   (KERNEL.10)
 */
WORD LocalSize( HLOCAL handle )
{
    return LOCAL_Size( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalHandle   (KERNEL.11)
 */
HLOCAL LocalHandle( WORD addr )
{ 
    return LOCAL_Handle( CURRENT_DS, addr );
}


/***********************************************************************
 *           LocalFlags   (KERNEL.12)
 */
WORD LocalFlags( HLOCAL handle )
{
    return LOCAL_Flags( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalCompact   (KERNEL.13)
 */
WORD LocalCompact( WORD minfree )
{
    dprintf_local( stddeb, "LocalCompact: %04x\n", minfree );
    return LOCAL_Compact( CURRENT_DS, minfree, 0 );
}


/***********************************************************************
 *           LocalNotify   (KERNEL.14)
 */
FARPROC LocalNotify( FARPROC func )
{
    LOCALHEAPINFO *pInfo;
    FARPROC oldNotify;
    WORD ds = CURRENT_DS;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_Notify(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }
    dprintf_local( stddeb, "LocalNotify(%04x): %08lx\n", ds, func );
    oldNotify = pInfo->notify;
    pInfo->notify = func;
    return oldNotify;
}


/***********************************************************************
 *           LocalShrink   (KERNEL.121)
 */
WORD LocalShrink( HLOCAL handle, WORD newsize )
{
    dprintf_local( stddeb, "LocalShrink: %04x %04x\n", handle, newsize );
    return 0;
}


/***********************************************************************
 *           GetHeapSpaces   (KERNEL.138)
 */
DWORD GetHeapSpaces( HMODULE module )
{
    NE_MODULE *pModule;
    WORD ds;

    module = GetExePtr( module );
    if (!(pModule = MODULE_GetPtr( module ))) return 0;
    ds = (NE_SEG_TABLE( pModule ) + pModule->dgroup - 1)->selector;
    return MAKELONG( LOCAL_CountFree( ds ), LOCAL_HeapSize( ds ) );
}


/***********************************************************************
 *           LocalCountFree   (KERNEL.161)
 */
WORD LocalCountFree(void)
{
    return LOCAL_CountFree( CURRENT_DS );
}


/***********************************************************************
 *           LocalHeapSize   (KERNEL.162)
 */
WORD LocalHeapSize()
{
    dprintf_local( stddeb, "LocalHeapSize:\n" );
    return LOCAL_HeapSize( CURRENT_DS );
}


/***********************************************************************
 *           LocalHandleDelta   (KERNEL.310)
 */
WORD LocalHandleDelta( WORD delta )
{
    LOCALHEAPINFO *pInfo;

    if (!(pInfo = LOCAL_GetHeap( CURRENT_DS )))
    {
        fprintf( stderr, "LocalHandleDelta: Local heap not found\n");
	LOCAL_PrintHeap( CURRENT_DS );
	return 0;
    }
    if (delta) pInfo->hdelta = delta;
    dprintf_local(stddeb, "LocalHandleDelta: returning %04x\n", pInfo->hdelta);
    return pInfo->hdelta;
}


/***********************************************************************
 *           LocalInfo   (TOOLHELP.56)
 */
BOOL LocalInfo( LOCALINFO *pLocalInfo, HGLOBAL handle )
{
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap(SELECTOROF(WIN16_GlobalLock(handle)));
    if (!pInfo) return FALSE;
    pLocalInfo->wcItems = pInfo->items;
    return TRUE;
}


/***********************************************************************
 *           LocalFirst   (TOOLHELP.57)
 */
BOOL LocalFirst( LOCALENTRY *pLocalEntry, HGLOBAL handle )
{
    WORD ds = SELECTOROF( WIN16_GlobalLock( handle ) );
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );
    if (!pInfo) return FALSE;

    pLocalEntry->hHandle   = pInfo->first + ARENA_HEADER_SIZE;
    pLocalEntry->wAddress  = pLocalEntry->hHandle;
    pLocalEntry->wFlags    = LF_FIXED;
    pLocalEntry->wcLock    = 0;
    pLocalEntry->wType     = LT_NORMAL;
    pLocalEntry->hHeap     = handle;
    pLocalEntry->wHeapType = NORMAL_HEAP;
    pLocalEntry->wNext     = ARENA_PTR(ptr,pInfo->first)->next;
    pLocalEntry->wSize     = pLocalEntry->wNext - pLocalEntry->hHandle;
    return TRUE;
}


/***********************************************************************
 *           LocalNext   (TOOLHELP.58)
 */
BOOL LocalNext( LOCALENTRY *pLocalEntry )
{
    WORD ds = SELECTOROF( pLocalEntry->hHeap );
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALARENA *pArena;

    if (!LOCAL_GetHeap( ds )) return FALSE;
    if (!pLocalEntry->wNext) return FALSE;
    pArena = ARENA_PTR( ptr, pLocalEntry->wNext );

    pLocalEntry->hHandle   = pLocalEntry->wNext + ARENA_HEADER_SIZE;
    pLocalEntry->wAddress  = pLocalEntry->hHandle;
    pLocalEntry->wFlags    = (pArena->prev & 3) + 1;
    pLocalEntry->wcLock    = 0;
    pLocalEntry->wType     = LT_NORMAL;
    if (pArena->next != pLocalEntry->wNext)  /* last one? */
        pLocalEntry->wNext = pArena->next;
    else
        pLocalEntry->wNext = 0;
    pLocalEntry->wSize     = pLocalEntry->wNext - pLocalEntry->hHandle;
    return TRUE;
}
