/*
 * Local heap functions
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Huw Davies
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
#include "global.h"
#include "heap.h"
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
    FARPROC16 notify WINE_PACKED; /* 1e Pointer to LocalNotify() function */
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
#define ARENA_PREV(ptr,arena)      (ARENA_PTR((ptr),(arena))->prev & ~3)
#define ARENA_NEXT(ptr,arena)      (ARENA_PTR((ptr),(arena))->next)
#define ARENA_FLAGS(ptr,arena)     (ARENA_PTR((ptr),(arena))->prev & 3)

  /* determine whether the handle belongs to a fixed or a moveable block */
#define HANDLE_FIXED(handle) (((handle) & 3) == 0)
#define HANDLE_MOVEABLE(handle) (((handle) & 3) == 2)

/***********************************************************************
 *           LOCAL_GetHeap
 *
 * Return a pointer to the local heap, making sure it exists.
 */
static LOCALHEAPINFO *LOCAL_GetHeap( HANDLE16 ds )
{
    LOCALHEAPINFO *pInfo;
    INSTANCEDATA *ptr = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( ds, 0 );
    dprintf_local( stddeb, "Heap at %p, %04x\n", ptr, ptr->heap );
    if (!ptr || !ptr->heap) return NULL;
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

    pNew->prev = (prev & ~3) | LOCAL_ARENA_FIXED;
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
static void LOCAL_PrintHeap( HANDLE16 ds )
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
BOOL16 LocalInit( HANDLE16 selector, WORD start, WORD end )
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

    if (debugging_heap)
    {
        /* If debugging_heap is set, the global heap blocks are cleared */
        /* before use, so we can test for double initialization. */
        if (LOCAL_GetHeap(selector))
        {
            fprintf( stderr, "LocalInit: Heap %04x initialized twice.\n", selector);
            if (debugging_local) LOCAL_PrintHeap(selector);
        }
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
		start = GlobalSize16( GlobalHandle16( selector ) );
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
static void LOCAL_GrowHeap( HANDLE16 ds )
{
    HANDLE16 hseg = GlobalHandle16( ds );
    LONG oldsize = GlobalSize16( hseg );
    LONG end;
    LOCALHEAPINFO *pHeapInfo;
    WORD freeArena, lastArena;
    LOCALARENA *pArena, *pLastArena;
    char *ptr;
    
    /* if nothing can be gained, return */
    if (oldsize > 0xfff0) return;
    hseg = GlobalReAlloc16( hseg, 0x10000, GMEM_FIXED );
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    pHeapInfo = LOCAL_GetHeap( ds );
    if (pHeapInfo == NULL) {
	fprintf( stderr, "Local_GrowHeap: heap not found\n" );
	return;
    }
    end = GlobalSize16( hseg );
    lastArena = (end - sizeof(LOCALARENA)) & ~3;

      /* Update the HeapInfo */
    pHeapInfo->items++;
    freeArena = pHeapInfo->last;
    pHeapInfo->last = lastArena;
    pHeapInfo->minsize += end - oldsize;
    
      /* grow the old last block */
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
    
    /* If block before freeArena is also free then merge them */
    if((ARENA_PTR(ptr, (pArena->prev & ~3))->prev & 3) == LOCAL_ARENA_FREE)
    {
        LOCAL_RemoveBlock(ptr, freeArena);
        pHeapInfo->items--;
    }

    dprintf_local( stddeb, "Heap expanded\n" );
    LOCAL_PrintHeap( ds );
}


/***********************************************************************
 *           LOCAL_FreeArena
 */
static HLOCAL16 LOCAL_FreeArena( WORD ds, WORD arena )
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
 *           LOCAL_ShrinkArena
 *
 * Shrink an arena by creating a free block at its end if possible.
 * 'size' includes the arena header, and must be aligned.
 */
static void LOCAL_ShrinkArena( WORD ds, WORD arena, WORD size )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALARENA *pArena = ARENA_PTR( ptr, arena );

    if (arena + size + LALIGN(sizeof(LOCALARENA)) < pArena->next)
    {
        LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );
        if (!pInfo) return;
        LOCAL_AddBlock( ptr, arena, arena + size );
        pInfo->items++;
        LOCAL_FreeArena( ds, arena + size );
    }
}


/***********************************************************************
 *           LOCAL_GrowArenaDownward
 *
 * Grow an arena downward by using the previous arena (must be free).
 */
static void LOCAL_GrowArenaDownward( WORD ds, WORD arena, WORD newsize )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena = ARENA_PTR( ptr, arena );
    WORD prevArena = pArena->prev & ~3;
    LOCALARENA *pPrevArena = ARENA_PTR( ptr, prevArena );
    WORD offset, size;
    char *p;

    if (!(pInfo = LOCAL_GetHeap( ds ))) return;
    offset = pPrevArena->size;
    size = pArena->next - arena - ARENA_HEADER_SIZE;
    LOCAL_RemoveFreeBlock( ptr, prevArena );
    LOCAL_RemoveBlock( ptr, arena );
    pInfo->items--;
    p = (char *)pPrevArena + ARENA_HEADER_SIZE;
    while (offset < size)
    {
        memcpy( p, p + offset, offset );
        p += offset;
        size -= offset;
    }
    if (size) memcpy( p, p + offset, size );
    LOCAL_ShrinkArena( ds, prevArena, newsize );
}


/***********************************************************************
 *           LOCAL_GetFreeSpace
 */
static WORD LOCAL_GetFreeSpace(WORD ds, WORD countdiscard)
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;
    WORD freespace = 0;
    
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_GetFreeSpace: Local heap not found\n" );
        LOCAL_PrintHeap(ds);
        return 0;
    }
    arena = pInfo->first;
    pArena = ARENA_PTR( ptr, arena );
    while (arena != pArena->free_next)
    {
        arena = pArena->free_next;
        pArena = ARENA_PTR( ptr, arena );
        if (pArena->size >= freespace) freespace = pArena->size;
    }
    /* FIXME doesn't yet calculate space that would become free if everything
       were discarded when countdiscard == 1 */
    if (freespace < ARENA_HEADER_SIZE) freespace = 0;
    else freespace -= ARENA_HEADER_SIZE;
    return freespace;
}


/***********************************************************************
 *           LOCAL_Compact
 */
static WORD LOCAL_Compact( HANDLE16 ds, WORD minfree, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pMoveArena, *pFinalArena;
    WORD arena, movearena, finalarena, table;
    WORD count, movesize, size;
    WORD freespace;
    LOCALHANDLEENTRY *pEntry;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local_Compact: Local heap not found\n" );
        LOCAL_PrintHeap(ds);
        return 0;
    }
    dprintf_local(stddeb,
                  "LOCAL_Compact: ds = %04x, minfree = %04x, flags = %04x\n",
                  ds, minfree, flags);
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NOCOMPACT))
    {
        dprintf_local(stddeb, "Returning %04x.\n", freespace);
        return freespace;
    }
    dprintf_local(stddeb, "Local_Compact: Compacting heap %04x.\n", ds);
    table = pInfo->htable;
    while(table)
    {
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for(count = *(WORD *)(ptr + table); count > 0; count--, pEntry++)
        {
            if((pEntry->lock == 0) && !(pEntry->flags &
                                        (LMEM_DISCARDED >> 8)))
            {
                /* OK we can move this one if we want */
                dprintf_local(stddeb,
                              "handle %04x (block %04x) can be moved.\n",
                              (WORD)((char *)pEntry - ptr), pEntry->addr);
                movearena = ARENA_HEADER(pEntry->addr);
                pMoveArena = ARENA_PTR(ptr, movearena);
                movesize = pMoveArena->next - movearena;
                arena = pInfo->first;
                pArena = ARENA_PTR(ptr, arena);
                size = 0xffff;
                finalarena = 0;
                /* Try to find the smallest arena that will do, */
                /* which is below us in memory */
                for(;;)
                {
                    arena = pArena->free_next;
                    pArena = ARENA_PTR(ptr, arena);
                    if(arena >= movearena)
                        break;
                    if(arena == pArena->free_next)
                        break;
                    if((pArena->size >= movesize) && (pArena->size < size))
                    {
                        size = pArena->size;
                        finalarena = arena;
                    }
                }
                if (finalarena) /* Actually got somewhere to move */
                {
                    dprintf_local(stddeb, "Moving it to %04x.\n", finalarena);
                    pFinalArena = ARENA_PTR(ptr, finalarena);
                    size = pFinalArena->size;
                    LOCAL_RemoveFreeBlock(ptr, finalarena);
                    LOCAL_ShrinkArena( ds, finalarena, movesize );
                    /* Copy the arena to it's new location */
                    memcpy((char *)pFinalArena + ARENA_HEADER_SIZE,
                           (char *)pMoveArena + ARENA_HEADER_SIZE,
                           movesize - ARENA_HEADER_SIZE );
                    /* Free the old location */  
                    LOCAL_FreeArena(ds, movearena);
                    /* Update handle table entry */
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE;
                }
                else if((ARENA_PTR(ptr, pMoveArena->prev & ~3)->prev & 3)
			       == LOCAL_ARENA_FREE)
                {
                    /* Previous arena is free (but < movesize)  */
                    /* so we can 'slide' movearena down into it */
                    finalarena = pMoveArena->prev & ~3;
                    LOCAL_GrowArenaDownward( ds, movearena, movesize );
                    /* Update handle table entry */
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE;
                }
            }
        }
        table = *(WORD *)pEntry;
    }
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NODISCARD))
    {
        dprintf_local(stddeb, "Returning %04x.\n", freespace);
        return freespace;
    }

    table = pInfo->htable;
    while(table)
    {
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for(count = *(WORD *)(ptr + table); count > 0; count--, pEntry++)
        {
            if((pEntry->flags & (LMEM_DISCARDABLE >> 8)) &&
               pEntry->lock == 0 && !(pEntry->flags & (LMEM_DISCARDED >> 8)))
            {
                dprintf_local(stddeb, "Discarding handle %04x (block %04x).\n",
                              (char *)pEntry - ptr, pEntry->addr);
                LOCAL_FreeArena(ds, pEntry->addr);
                pEntry->addr = 0;
                pEntry->flags |= (LMEM_DISCARDED >> 8);
                /* Call localnotify proc */
            }
        }
        table = *(WORD *)pEntry;
    }
    return LOCAL_Compact(ds, 0xffff, LMEM_NODISCARD);
}


/***********************************************************************
 *           LOCAL_FindFreeBlock
 */
static HLOCAL16 LOCAL_FindFreeBlock( HANDLE16 ds, WORD size )
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
static HLOCAL16 LOCAL_GetBlock( HANDLE16 ds, WORD size, WORD flags )
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
    dprintf_local(stddeb, "LOCAL_GetBlock size = %04x, arena %04x size %04x\n",
                  size, arena, pArena->size );
    LOCAL_RemoveFreeBlock( ptr, arena );
    LOCAL_ShrinkArena( ds, arena, size );

    if (flags & LMEM_ZEROINIT) memset( (char *)pArena + ARENA_HEADER_SIZE, 0,
                                       size - ARENA_HEADER_SIZE );
    return arena + ARENA_HEADER_SIZE;
}


/***********************************************************************
 *           LOCAL_NewHTable
 */
static BOOL16 LOCAL_NewHTable( HANDLE16 ds )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALHANDLEENTRY *pEntry;
    HLOCAL16 handle;
    int i;

    dprintf_local( stddeb, "Local_NewHTable\n" );
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "Local heap not found\n");
        LOCAL_PrintHeap(ds);
        return FALSE;
    }

    if (!(handle = LOCAL_GetBlock( ds, pInfo->hdelta * sizeof(LOCALHANDLEENTRY)
                                   + 2 * sizeof(WORD), LMEM_FIXED )))
        return FALSE;
    if (!(ptr = PTR_SEG_OFF_TO_LIN( ds, 0 )))
        fprintf(stderr, "LOCAL_NewHTable: ptr == NULL after GetBlock.\n");
    if (!(pInfo = LOCAL_GetHeap( ds )))
        fprintf(stderr,"LOCAL_NewHTable: pInfo == NULL after GetBlock.\n");

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
static HLOCAL16 LOCAL_GetNewHandleEntry( HANDLE16 ds )
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
    return (HLOCAL16)((char *)pEntry - ptr);
}


/***********************************************************************
 *           LOCAL_FreeHandleEntry
 *
 * Free a handle table entry.
 */
static void LOCAL_FreeHandleEntry( HANDLE16 ds, HLOCAL16 handle )
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

    table = *pTable;
    pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
    count = *(WORD *)(ptr + table);
    for (i = count; i > 0; i--, pEntry++) if (pEntry->lock != 0xff) return;
    
    /* Remove the table from the linked list and free it */

    dprintf_local( stddeb, "LOCAL_FreeHandleEntry(%04x): freeing table %04x\n",
                   ds, table);
    *pTable = *(WORD *)pEntry;
    LOCAL_FreeArena( ds, ARENA_HEADER( table ) );
}


/***********************************************************************
 *           LOCAL_Free
 *
 * Implementation of LocalFree().
 */
HLOCAL16 LOCAL_Free( HANDLE16 ds, HLOCAL16 handle )
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
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        if (!(pEntry->flags & (LMEM_DISCARDED >> 8)))
        {
            dprintf_local( stddeb, "LocalFree: real block at %04x\n",
			   pEntry->addr );
            if (LOCAL_FreeArena( ds, ARENA_HEADER(pEntry->addr) ))
                return handle; /* couldn't free it */
        }
        LOCAL_FreeHandleEntry( ds, handle );
        return 0;  /* OK */
    }
}


/***********************************************************************
 *           LOCAL_Alloc
 *
 * Implementation of LocalAlloc().
 */
HLOCAL16 LOCAL_Alloc( HANDLE16 ds, WORD flags, WORD size )
{
    char *ptr;
    HLOCAL16 handle;
    
    dprintf_local( stddeb, "LocalAlloc: %04x %d ds=%04x\n", flags, size, ds );

    if (flags & LMEM_MOVEABLE)
    {
	LOCALHANDLEENTRY *plhe;
	HLOCAL16 hmem;
	
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
HLOCAL16 LOCAL_ReAlloc( HANDLE16 ds, HLOCAL16 handle, WORD size, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pNext;
    LOCALHANDLEENTRY *pEntry;
    WORD arena, newhandle, blockhandle, oldsize;
    LONG nextarena;

    if (!handle) return LOCAL_Alloc( ds, size, flags );

    dprintf_local( stddeb, "LocalReAlloc: %04x %d %04x ds=%04x\n",
                   handle, size, flags, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return 0;
    
    if (HANDLE_FIXED( handle )) blockhandle = handle;
    else
    {
	pEntry = (LOCALHANDLEENTRY *) (ptr + handle);
	if(pEntry->flags & (LMEM_DISCARDED >> 8))
        {
	    dprintf_local(stddeb, "ReAllocating discarded block\n");
	    if (!(pEntry->addr = LOCAL_GetBlock( ds, size, flags))) return 0;
            ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr */
            pEntry = (LOCALHANDLEENTRY *) (ptr + handle);
            pEntry->flags = (BYTE) (flags >> 8);
            pEntry->lock = 0;
            return handle;
	}    
	if (!(blockhandle = pEntry->addr))
	{
	    fprintf( stderr, "Local_ReAlloc(%04x,%04x): invalid handle\n",
                     ds, handle );
	    return 0;
        }
    }

    arena = ARENA_HEADER( blockhandle );
    dprintf_local( stddeb, "LocalReAlloc: arena is %04x\n", arena );
    pArena = ARENA_PTR( ptr, arena );
    
    if ((flags & LMEM_MODIFY) && (flags & LMEM_DISCARDABLE))
    {
        if (HANDLE_FIXED(handle))
        {
            fprintf(stderr,"LocalReAlloc: LMEM_MODIFY & LMEM_DISCARDABLE on a fixed handle.\n");
            return handle;
        }
        dprintf_local( stddeb, "Making block discardable.\n" );
        pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        pEntry->flags |= (LMEM_DISCARDABLE >> 8);
        return handle;
    }

    if ((flags & LMEM_MODIFY) || (flags & LMEM_DISCARDABLE))
    {
        fprintf(stderr,"LocalReAlloc: flags %04x. MODIFY & DISCARDABLE should both be set\n", flags);
        return handle;
    }

    if (!size)
    {
        if (HANDLE_FIXED(handle))
        {
            if (flags & LMEM_MOVEABLE)
            {
                dprintf_local(stddeb, "Freeing fixed block.\n");
                return LOCAL_Free( ds, handle );
            }
            else size = 1;
        }
        else
        {
            pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
            if (pEntry->lock == 0)
            {
                /* discards moveable blocks is this right? */
                dprintf_local(stddeb,"Discarding block\n");
                LOCAL_FreeArena(ds, ARENA_HEADER(pEntry->addr));
                pEntry->addr = 0;
                pEntry->flags = (LMEM_DISCARDED >> 8);
                return 0;
            }
            return handle;
        }
    }

    size = LALIGN( size );
    oldsize = pArena->next - arena - ARENA_HEADER_SIZE;
    nextarena = LALIGN(blockhandle + size);

      /* Check for size reduction */

    if (nextarena <= pArena->next)
    {
        if (nextarena < pArena->next - LALIGN(sizeof(LOCALARENA)))
        {
	    dprintf_local( stddeb, "size reduction, making new free block\n");
              /* It is worth making a new free block */
            LOCAL_AddBlock( ptr, arena, nextarena );
            pInfo->items++;
            LOCAL_FreeArena( ds, nextarena );
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
            pInfo->items++;
            LOCAL_FreeArena( ds, nextarena );
        }
        dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", handle );
        return handle;
    }

    /* Now we have to allocate a new block, but not if fixed block and no
       LMEM_MOVEABLE */

    if (HANDLE_FIXED(handle) && !(flags & LMEM_MOVEABLE))
    {
        dprintf_local(stddeb, "Needed to move fixed block, but LMEM_MOVEABLE not specified.\n");
        return 0;  /* FIXME: should we free it here? */
    }

    newhandle = LOCAL_GetBlock( ds, size, flags );
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr */
    if (!newhandle)
    {
        /* Remove the block from the heap and try again */
        LPSTR buffer = HeapAlloc( SystemHeap, 0, oldsize );
        if (!buffer) return 0;
        memcpy( buffer, ptr + (arena + ARENA_HEADER_SIZE), oldsize );
        LOCAL_FreeArena( ds, arena );
        if (!(newhandle = LOCAL_GetBlock( ds, size, flags )))
        {
            if (!(newhandle = LOCAL_GetBlock( ds, oldsize, flags )))
            {
                fprintf( stderr, "LocalRealloc: can't restore saved block\n" );
                HeapFree( SystemHeap, 0, buffer );
                return 0;
            }
            size = oldsize;
        }
        ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr */
        memcpy( ptr + newhandle, buffer, oldsize );
        HeapFree( SystemHeap, 0, buffer );
    }
    else
    {
        memcpy( ptr + newhandle, ptr + (arena + ARENA_HEADER_SIZE), oldsize );
        LOCAL_FreeArena( ds, arena );
    }
    if (HANDLE_MOVEABLE( handle ))
    {
	dprintf_local( stddeb, "LocalReAlloc: fixing handle\n");
        pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        pEntry->addr = newhandle;
        pEntry->lock = 0;
	newhandle = handle;
    }
    if (size == oldsize) newhandle = 0;  /* Realloc failed */
    dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", newhandle );
    return newhandle;
}


/***********************************************************************
 *           LOCAL_InternalLock
 */
static HLOCAL16 LOCAL_InternalLock( LPSTR heap, HLOCAL16 handle )
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
LPSTR LOCAL_Lock( HANDLE16 ds, HLOCAL16 handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    return handle ? ptr + LOCAL_InternalLock( ptr, handle ) : NULL;
}


/***********************************************************************
 *           LOCAL_Unlock
 */
BOOL16 LOCAL_Unlock( HANDLE16 ds, HLOCAL16 handle )
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
WORD LOCAL_Size( HANDLE16 ds, HLOCAL16 handle )
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
WORD LOCAL_Flags( HANDLE16 ds, HLOCAL16 handle )
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
WORD LOCAL_HeapSize( HANDLE16 ds )
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
WORD LOCAL_CountFree( HANDLE16 ds )
{
    WORD arena, total;
    LOCALARENA *pArena;
    LOCALHEAPINFO *pInfo;
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_Handle(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }

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
HLOCAL16 LOCAL_Handle( HANDLE16 ds, WORD addr )
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
            if (pEntry->addr == addr) return (HLOCAL16)((char *)pEntry - ptr);
        table = *(WORD *)pEntry;
    }

    return (HLOCAL16)addr;  /* Fixed block handle is addr */
}


/***********************************************************************
 *           LocalAlloc16   (KERNEL.5)
 */
HLOCAL16 LocalAlloc16( UINT16 flags, WORD size )
{
    return LOCAL_Alloc( CURRENT_DS, flags, size );
}


/***********************************************************************
 *           LocalReAlloc16   (KERNEL.6)
 */
HLOCAL16 LocalReAlloc16( HLOCAL16 handle, WORD size, UINT16 flags )
{
    return LOCAL_ReAlloc( CURRENT_DS, handle, size, flags );
}


/***********************************************************************
 *           LocalFree16   (KERNEL.7)
 */
HLOCAL16 LocalFree16( HLOCAL16 handle )
{
    return LOCAL_Free( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalLock16   (KERNEL.8)
 *
 * Note: only the offset part of the pointer is returned by the relay code.
 */
SEGPTR LocalLock16( HLOCAL16 handle )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    return PTR_SEG_OFF_TO_SEGPTR( ds, LOCAL_InternalLock( ptr, handle ) );
}


/***********************************************************************
 *           LocalUnlock16   (KERNEL.9)
 */
BOOL16 LocalUnlock16( HLOCAL16 handle )
{
    return LOCAL_Unlock( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalSize16   (KERNEL.10)
 */
UINT16 LocalSize16( HLOCAL16 handle )
{
    return LOCAL_Size( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalHandle16   (KERNEL.11)
 */
HLOCAL16 LocalHandle16( WORD addr )
{ 
    return LOCAL_Handle( CURRENT_DS, addr );
}


/***********************************************************************
 *           LocalFlags16   (KERNEL.12)
 */
UINT16 LocalFlags16( HLOCAL16 handle )
{
    return LOCAL_Flags( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalCompact16   (KERNEL.13)
 */
UINT16 LocalCompact16( UINT16 minfree )
{
    dprintf_local( stddeb, "LocalCompact: %04x\n", minfree );
    return LOCAL_Compact( CURRENT_DS, minfree, 0 );
}


/***********************************************************************
 *           LocalNotify   (KERNEL.14)
 */
FARPROC16 LocalNotify( FARPROC16 func )
{
    LOCALHEAPINFO *pInfo;
    FARPROC16 oldNotify;
    HANDLE16 ds = CURRENT_DS;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        fprintf( stderr, "LOCAL_Notify(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }
    dprintf_local( stddeb, "LocalNotify(%04x): %08lx\n", ds, (DWORD)func );
    oldNotify = pInfo->notify;
    pInfo->notify = func;
    return oldNotify;
}


/***********************************************************************
 *           LocalShrink16   (KERNEL.121)
 */
UINT16 LocalShrink16( HGLOBAL16 handle, UINT16 newsize )
{
    dprintf_local( stddeb, "LocalShrink: %04x %04x\n", handle, newsize );
    return 0;
}


/***********************************************************************
 *           GetHeapSpaces   (KERNEL.138)
 */
DWORD GetHeapSpaces( HMODULE16 module )
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
BOOL16 LocalInfo( LOCALINFO *pLocalInfo, HGLOBAL16 handle )
{
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap(SELECTOROF(WIN16_GlobalLock16(handle)));
    if (!pInfo) return FALSE;
    pLocalInfo->wcItems = pInfo->items;
    return TRUE;
}


/***********************************************************************
 *           LocalFirst   (TOOLHELP.57)
 */
BOOL16 LocalFirst( LOCALENTRY *pLocalEntry, HGLOBAL16 handle )
{
    WORD ds = GlobalHandleToSel( handle );
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
BOOL16 LocalNext( LOCALENTRY *pLocalEntry )
{
    WORD ds = GlobalHandleToSel( pLocalEntry->hHeap );
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


/***********************************************************************
 *           LocalAlloc32   (KERNEL32.371)
 */
HLOCAL32 LocalAlloc32( UINT32 flags, DWORD size )
{
    return (HLOCAL32)GlobalAlloc32( flags, size );
}


/***********************************************************************
 *           LocalCompact32   (KERNEL32.372)
 */
UINT32 LocalCompact32( UINT32 minfree )
{
    return 0;  /* LocalCompact does nothing in Win32 */
}


/***********************************************************************
 *           LocalFlags32   (KERNEL32.374)
 */
UINT32 LocalFlags32( HLOCAL32 handle )
{
    return GlobalFlags32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalFree32   (KERNEL32.375)
 */
HLOCAL32 LocalFree32( HLOCAL32 handle )
{
    return (HLOCAL32)GlobalFree32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalHandle32   (KERNEL32.376)
 */
HLOCAL32 LocalHandle32( LPCVOID ptr )
{
    return (HLOCAL32)GlobalHandle32( ptr );
}


/***********************************************************************
 *           LocalLock32   (KERNEL32.377)
 */
LPVOID LocalLock32( HLOCAL32 handle )
{
    return GlobalLock32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalReAlloc32   (KERNEL32.378)
 */
HLOCAL32 LocalReAlloc32( HLOCAL32 handle, DWORD size, UINT32 flags )
{
    return (HLOCAL32)GlobalReAlloc32( (HGLOBAL32)handle, size, flags );
}


/***********************************************************************
 *           LocalShrink32   (KERNEL32.379)
 */
UINT32 LocalShrink32( HGLOBAL32 handle, UINT32 newsize )
{
    return 0;  /* LocalShrink does nothing in Win32 */
}


/***********************************************************************
 *           LocalSize32   (KERNEL32.380)
 */
UINT32 LocalSize32( HLOCAL32 handle )
{
    return GlobalSize32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalUnlock32   (KERNEL32.381)
 */
BOOL32 LocalUnlock32( HLOCAL32 handle )
{
    return GlobalUnlock32( (HGLOBAL32)handle );
}
