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
#include "task.h"
#include "global.h"
#include "heap.h"
#include "instance.h"
#include "local.h"
#include "module.h"
#include "stackframe.h"
#include "toolhelp.h"
#include "debug.h"

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

/*
 * We make addr = 4n + 2 and set *((WORD *)addr - 1) = &addr like Windows does
 * in case something actually relies on this.
 * Note the ARENA_HEADER(addr) still produces the desired result ie. 4n - 4
 *
 * An unused handle has lock = flags = 0xff. In windows addr is that of next
 * free handle, at the moment in wine we set it to 0.
 *
 * A discarded block's handle has lock = addr = 0 and flags = 0x40
 * (LMEM_DISCARDED >> 8)
 */

#pragma pack(1)

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

#pragma pack(4)

#define LOCAL_HEAP_MAGIC  0x484c  /* 'LH' */

WORD USER_HeapSel = 0;  /* USER heap selector */
WORD GDI_HeapSel = 0;   /* GDI heap selector */

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
    TRACE(local, "Heap at %p, %04x\n", ptr, ptr->heap );
    if (!ptr || !ptr->heap) return NULL;
    if (IsBadReadPtr16( (SEGPTR)MAKELONG(ptr->heap,ds), sizeof(LOCALHEAPINFO)))
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

    TRACE(local, "Local_MakeBlockFree %04x, next %04x\n", block, next );
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

    TRACE(local, "Local_RemoveBlock\n");
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
    char *ptr;
    LOCALHEAPINFO *pInfo;
    WORD arena;

    /* FIXME - the test should be done when calling the function! 
               plus is not clear that we should print this info
               only when TRACE_ON is on! */
    if(!TRACE_ON(local)) return;

    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    pInfo = LOCAL_GetHeap( ds );

    if (!pInfo)
    {
        DUMP( "Local Heap corrupted!  ds=%04x\n", ds );
        return;
    }
    DUMP( "Local Heap  ds=%04x first=%04x last=%04x items=%d\n",
	  ds, pInfo->first, pInfo->last, pInfo->items );

    arena = pInfo->first;
    for (;;)
    {
        LOCALARENA *pArena = ARENA_PTR(ptr,arena);
        DUMP( "  %04x: prev=%04x next=%04x type=%d\n", arena,
	      pArena->prev & ~3, pArena->next, pArena->prev & 3 );
        if (arena == pInfo->first)
	{
            DUMP( "        size=%d free_prev=%04x free_next=%04x\n",
		  pArena->size, pArena->free_prev, pArena->free_next );
	}
        if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
        {
            DUMP( "        size=%d free_prev=%04x free_next=%04x\n",
		  pArena->size, pArena->free_prev, pArena->free_next );
            if (pArena->next == arena) break;  /* last one */
            if (ARENA_PTR(ptr,pArena->free_next)->free_prev != arena)
            {
                DUMP( "*** arena->free_next->free_prev != arena\n" );
                break;
            }
        }
        if (pArena->next == arena)
        {
	    DUMP( "*** last block is not marked free\n" );
            break;
        }
        if ((ARENA_PTR(ptr,pArena->next)->prev & ~3) != arena)
        {
            DUMP( "*** arena->next->prev != arena (%04x, %04x)\n",
		  pArena->next, ARENA_PTR(ptr,pArena->next)->prev);
            break;
        }
        arena = pArena->next;
    }
}


/***********************************************************************
 *           LocalInit   (KERNEL.4)
 */
BOOL16 WINAPI LocalInit( HANDLE16 selector, WORD start, WORD end )
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

    TRACE(local, "%04x %04x-%04x\n", selector, start, end);
    if (!selector) selector = CURRENT_DS;

    if (TRACE_ON(heap))
    {
        /* If TRACE_ON(heap) is set, the global heap blocks are */
        /* cleared before use, so we can test for double initialization. */
        if (LOCAL_GetHeap(selector))
        {
            WARN(local, "Heap %04x initialized twice.\n", selector);
            LOCAL_PrintHeap(selector);
        }
    }

    if (start == 0) {
      /* Check if the segment is the DGROUP of a module */

	if ((pModule = NE_GetPtr( selector )))
	{
	    SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule ) + pModule->dgroup - 1;
	    if (pModule->dgroup && (GlobalHandleToSel(pSeg->hSeg) == selector))
	    {
		/* We can't just use the simple method of using the value
                 * of minsize + stacksize, since there are programs that
                 * resize the data segment before calling InitTask(). So,
                 * we must put it at the end of the segment */
		start = GlobalSize16( GlobalHandle16( selector ) );
		start -= end;
		end += start;
		TRACE(local," new start %04x, minstart: %04x\n", start, pSeg->minsize + pModule->stack_size);
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
	WARN(local, "Heap not found\n" );
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

    TRACE(local, "Heap expanded\n" );
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

    TRACE(local, "%04x ds=%04x\n", arena, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return arena;

    pArena = ARENA_PTR( ptr, arena );
    if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
    {
	/* shouldn't happen */
        WARN(local, "Trying to free block %04x twice!\n",
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
 *           LOCAL_GrowArenaUpward
 *
 * Grow an arena upward by using the next arena (must be free and big
 * enough). Newsize includes the arena header and must be aligned.
 */
static void LOCAL_GrowArenaUpward( WORD ds, WORD arena, WORD newsize )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena = ARENA_PTR( ptr, arena );
    WORD nextArena = pArena->next;

    if (!(pInfo = LOCAL_GetHeap( ds ))) return;
    LOCAL_RemoveBlock( ptr, nextArena );
    pInfo->items--;
    LOCAL_ShrinkArena( ds, arena, newsize );
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
        WARN(local, "Local heap not found\n" );
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
WORD LOCAL_Compact( HANDLE16 ds, UINT16 minfree, UINT16 flags )
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
        WARN(local, "Local heap not found\n" );
        LOCAL_PrintHeap(ds);
        return 0;
    }
    TRACE(local, "ds = %04x, minfree = %04x, flags = %04x\n",
		 ds, minfree, flags);
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NOCOMPACT))
    {
        TRACE(local, "Returning %04x.\n", freespace);
        return freespace;
    }
    TRACE(local, "Compacting heap %04x.\n", ds);
    table = pInfo->htable;
    while(table)
    {
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for(count = *(WORD *)(ptr + table); count > 0; count--, pEntry++)
        {
            if((pEntry->lock == 0) && (pEntry->flags != (LMEM_DISCARDED >> 8)))
            {
                /* OK we can move this one if we want */
                TRACE(local, "handle %04x (block %04x) can be moved.\n",
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
                    TRACE(local, "Moving it to %04x.\n", finalarena);
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
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE + sizeof(HLOCAL16) ;
                }
                else if((ARENA_PTR(ptr, pMoveArena->prev & ~3)->prev & 3)
			       == LOCAL_ARENA_FREE)
                {
                    /* Previous arena is free (but < movesize)  */
                    /* so we can 'slide' movearena down into it */
                    finalarena = pMoveArena->prev & ~3;
                    LOCAL_GrowArenaDownward( ds, movearena, movesize );
                    /* Update handle table entry */
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE + sizeof(HLOCAL16) ;
                }
            }
        }
        table = *(WORD *)pEntry;
    }
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NODISCARD))
    {
        TRACE(local, "Returning %04x.\n", freespace);
        return freespace;
    }

    table = pInfo->htable;
    while(table)
    {
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for(count = *(WORD *)(ptr + table); count > 0; count--, pEntry++)
        {
            if(pEntry->addr && pEntry->lock == 0 &&
	     (pEntry->flags & (LMEM_DISCARDABLE >> 8)))
	    {
                TRACE(local, "Discarding handle %04x (block %04x).\n",
                              (char *)pEntry - ptr, pEntry->addr);
                LOCAL_FreeArena(ds, ARENA_HEADER(pEntry->addr));
                pEntry->addr = 0;
                pEntry->flags = (LMEM_DISCARDED >> 8);
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
        WARN(local, "Local heap not found\n" );
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
    WARN(local, "not enough space\n" );
    LOCAL_PrintHeap(ds);
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
        WARN(local, "Local heap not found\n");
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
        if (ds == GDI_HeapSel) { 
	    WARN(local, "not enough space in GDI local heap "
			 "(%04x) for %d bytes\n", ds, size );
	} else if (ds == USER_HeapSel) {
	    WARN(local, "not enough space in USER local heap "
			 "(%04x) for %d bytes\n", ds, size );
	} else {
	    WARN(local, "not enough space in local heap "
			 "%04x for %d bytes\n", ds, size );
	}
	return 0;
    }

      /* Make a block out of the free arena */
    pArena = ARENA_PTR( ptr, arena );
    TRACE(local, "LOCAL_GetBlock size = %04x, arena %04x size %04x\n",
                  size, arena, pArena->size );
    LOCAL_RemoveFreeBlock( ptr, arena );
    LOCAL_ShrinkArena( ds, arena, size );

    if (flags & LMEM_ZEROINIT)
	memset((char *)pArena + ARENA_HEADER_SIZE, 0, size-ARENA_HEADER_SIZE);
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

    TRACE(local, "Local_NewHTable\n" );
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        WARN(local, "Local heap not found\n");
        LOCAL_PrintHeap(ds);
        return FALSE;
    }

    if (!(handle = LOCAL_GetBlock( ds, pInfo->hdelta * sizeof(LOCALHANDLEENTRY)
                                   + 2 * sizeof(WORD), LMEM_FIXED )))
        return FALSE;
    if (!(ptr = PTR_SEG_OFF_TO_LIN( ds, 0 )))
        WARN(local, "ptr == NULL after GetBlock.\n");
    if (!(pInfo = LOCAL_GetHeap( ds )))
        WARN(local,"pInfo == NULL after GetBlock.\n");

    /* Fill the entry table */

    *(WORD *)(ptr + handle) = pInfo->hdelta;
    pEntry = (LOCALHANDLEENTRY *)(ptr + handle + sizeof(WORD));
    for (i = pInfo->hdelta; i > 0; i--, pEntry++) {
	pEntry->lock = pEntry->flags = 0xff;
	pEntry->addr = 0;
    }
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
        WARN(local, "Local heap not found\n");
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
    pEntry->flags = 0;
    TRACE(local, "(%04x): %04x\n",
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
        WARN(local, "Invalid entry %04x\n", handle);
        LOCAL_PrintHeap( ds );
        return;
    }

    /* Make the entry free */

    pEntry->addr = 0;  /* just in case */
    pEntry->lock = 0xff;
    pEntry->flags = 0xff; 
    /* Now check if all entries in this table are free */

    table = *pTable;
    pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
    count = *(WORD *)(ptr + table);
    for (i = count; i > 0; i--, pEntry++) if (pEntry->lock != 0xff) return;
    
    /* Remove the table from the linked list and free it */

    TRACE(local, "(%04x): freeing table %04x\n",
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

    TRACE(local, "%04x ds=%04x\n", handle, ds );
    
    if (!handle) { WARN(local, "Handle is 0.\n" ); return 0; }
    if (HANDLE_FIXED( handle ))
    {
        if (!LOCAL_FreeArena( ds, ARENA_HEADER( handle ) )) return 0;  /* OK */
        else return handle;  /* couldn't free it */
    }
    else
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        if (pEntry->flags != (LMEM_DISCARDED >> 8))
        {
            TRACE(local, "real block at %04x\n",
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
 *
 */
HLOCAL16 LOCAL_Alloc( HANDLE16 ds, WORD flags, WORD size )
{
    char *ptr;
    HLOCAL16 handle;
    
    TRACE(local, "%04x %d ds=%04x\n", flags, size, ds );

    if(size > 0 && size <= 4) size = 5;
    if (flags & LMEM_MOVEABLE)
    {
	LOCALHANDLEENTRY *plhe;
	HLOCAL16 hmem;

	if(size)
	{
	    if (!(hmem = LOCAL_GetBlock( ds, size + sizeof(HLOCAL16), flags )))
		return 0;
        }
	else /* We just need to allocate a discarded handle */
	    hmem = 0;
	if (!(handle = LOCAL_GetNewHandleEntry( ds )))
        {
	    WARN(local, "Couldn't get handle.\n");
	    if(hmem)
		LOCAL_FreeArena( ds, ARENA_HEADER(hmem) );
	    return 0;
	}
	ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
	plhe = (LOCALHANDLEENTRY *)(ptr + handle);
	plhe->lock = 0;
	if(hmem)
	{
	    plhe->addr = hmem + sizeof(HLOCAL16);
	    plhe->flags = (BYTE)((flags & 0x0f00) >> 8);
	    *(HLOCAL16 *)(ptr + hmem) = handle;
	}
	else
	{
	    plhe->addr = 0;
	    plhe->flags = LMEM_DISCARDED >> 8;
        }
    }
    else /* FIXED */
    {
	if(!size)
	    return 0;
	handle = LOCAL_GetBlock( ds, size, flags );
    }
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
    LOCALHANDLEENTRY *pEntry = NULL;
    WORD arena, oldsize;
    HLOCAL16 hmem, blockhandle;
    LONG nextarena;

    if (!handle) return 0;
    if(HANDLE_MOVEABLE(handle) &&
     ((LOCALHANDLEENTRY *)(ptr + handle))->lock == 0xff) /* An unused handle */
	return 0;

    TRACE(local, "%04x %d %04x ds=%04x\n",
                   handle, size, flags, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return 0;
    
    if (HANDLE_FIXED( handle ))
	blockhandle = handle;
    else
    {
	pEntry = (LOCALHANDLEENTRY *) (ptr + handle);
	if(pEntry->flags == (LMEM_DISCARDED >> 8))
        {
	    HLOCAL16 hl;
	    if(pEntry->addr)
		WARN(local,"Dicarded block has non-zero addr.\n");
	    TRACE(local, "ReAllocating discarded block\n");
	    if(size <= 4) size = 5;
	    if (!(hl = LOCAL_GetBlock( ds, size + sizeof(HLOCAL16), flags)))
		return 0;
            ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr */
            pEntry = (LOCALHANDLEENTRY *) (ptr + handle);
	    pEntry->addr = hl + sizeof(HLOCAL16);
            pEntry->flags = 0;
            pEntry->lock = 0;
	    *(HLOCAL16 *)(ptr + hl) = handle;
            return handle;
	}
	if (((blockhandle = pEntry->addr) & 3) != 2)
	{
	    WARN(local, "(%04x,%04x): invalid handle\n",
                     ds, handle );
	    return 0;
        }
	if(*((HLOCAL16 *)(ptr + blockhandle) - 1) != handle) {
	    WARN(local, "Back ptr to handle is invalid\n");
	    return 0;
        }
    }

    if (flags & LMEM_MODIFY)
    {
        if (HANDLE_MOVEABLE(handle))
	{
	    pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
	    pEntry->flags = (flags & 0x0f00) >> 8;
	    TRACE(local, "Changing flags to %x.\n", pEntry->flags);
	}
	return handle;
    }

    if (!size)
    {
        if (flags & LMEM_MOVEABLE)
        {
	    if (HANDLE_FIXED(handle))
	    {
                TRACE(local, "Freeing fixed block.\n");
                return LOCAL_Free( ds, handle );
            }
	    else /* Moveable block */
	    {
		pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
		if (pEntry->lock == 0)
		{
		    /* discards moveable blocks */
                    TRACE(local,"Discarding block\n");
                    LOCAL_FreeArena(ds, ARENA_HEADER(pEntry->addr));
                    pEntry->addr = 0;
                    pEntry->flags = (LMEM_DISCARDED >> 8);
                    return handle;
	        }
	    }
	    return 0;
        }
        else if(flags == 0)
        {
            pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
            if (pEntry->lock == 0)
            {
		/* Frees block */
		return LOCAL_Free( ds, handle );
	    }
        }
        return 0;
    }

    arena = ARENA_HEADER( blockhandle );
    TRACE(local, "arena is %04x\n", arena );
    pArena = ARENA_PTR( ptr, arena );

    if(size <= 4) size = 5;
    oldsize = pArena->next - arena - ARENA_HEADER_SIZE;
    nextarena = LALIGN(blockhandle + size);

      /* Check for size reduction */

    if (nextarena <= pArena->next)
    {
	TRACE(local, "size reduction, making new free block\n");
	LOCAL_ShrinkArena(ds, arena, nextarena - arena);
        TRACE(local, "returning %04x\n", handle );
        return handle;
    }

      /* Check if the next block is free and large enough */

    pNext = ARENA_PTR( ptr, pArena->next );
    if (((pNext->prev & 3) == LOCAL_ARENA_FREE) &&
        (nextarena <= pNext->next))
    {
	TRACE(local, "size increase, making new free block\n");
        LOCAL_GrowArenaUpward(ds, arena, nextarena - arena);
        TRACE(local, "returning %04x\n", handle );
        return handle;
    }

    /* Now we have to allocate a new block, but not if (fixed block or locked
       block) and no LMEM_MOVEABLE */

    if (!(flags & LMEM_MOVEABLE))
    {
	if (HANDLE_FIXED(handle))
        {
            WARN(local, "Needed to move fixed block, but LMEM_MOVEABLE not specified.\n");
            return 0;
        }
	else
	{
	    if(((LOCALHANDLEENTRY *)(ptr + handle))->lock != 0)
	    {
		WARN(local, "Needed to move locked block, but LMEM_MOVEABLE not specified.\n");
		return 0;
	    }
        }
    }
    if(HANDLE_MOVEABLE(handle)) size += sizeof(HLOCAL16);
    hmem = LOCAL_GetBlock( ds, size, flags );
    ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr                             */
    if(HANDLE_MOVEABLE(handle))         /* LOCAL_GetBlock might have triggered    */
    {                                   /* a compaction, which might in turn have */
      blockhandle = pEntry->addr ;      /* moved the very block we are resizing   */
      arena = ARENA_HEADER( blockhandle );   /* thus, we reload arena, too        */
    }
    if (!hmem)
    {
        /* Remove the block from the heap and try again */
        LPSTR buffer = HeapAlloc( GetProcessHeap(), 0, oldsize );
        if (!buffer) return 0;
        memcpy( buffer, ptr + arena + ARENA_HEADER_SIZE, oldsize );
        LOCAL_FreeArena( ds, arena );
        if (!(hmem = LOCAL_GetBlock( ds, size, flags )))
        {
            if (!(hmem = LOCAL_GetBlock( ds, oldsize, flags )))
            {
                WARN(local, "Can't restore saved block\n" );
                HeapFree( GetProcessHeap(), 0, buffer );
                return 0;
            }
            size = oldsize;
        }
        ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );  /* Reload ptr */
        memcpy( ptr + hmem, buffer, oldsize );
        HeapFree( GetProcessHeap(), 0, buffer );
    }
    else
    {
        memcpy( ptr + hmem, ptr + (arena + ARENA_HEADER_SIZE), oldsize );
        LOCAL_FreeArena( ds, arena );
    }
    if (HANDLE_MOVEABLE( handle ))
    {
	TRACE(local, "fixing handle\n");
        pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        pEntry->addr = hmem + sizeof(HLOCAL16);
	/* Back ptr should still be correct */
	if(*(HLOCAL16 *)(ptr + hmem) != handle)
	    WARN(local, "back ptr is invalid.\n");
	hmem = handle;
    }
    if (size == oldsize) hmem = 0;  /* Realloc failed */
    TRACE(local, "returning %04x\n", hmem );
    return hmem;
}


/***********************************************************************
 *           LOCAL_InternalLock
 */
static HLOCAL16 LOCAL_InternalLock( LPSTR heap, HLOCAL16 handle )
{
    HLOCAL16 old_handle = handle;

    if (HANDLE_MOVEABLE(handle))
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(heap + handle);
	if (pEntry->flags == LMEM_DISCARDED) return 0;
        if (pEntry->lock < 0xfe) pEntry->lock++;
        handle = pEntry->addr;
    }
    TRACE(local, "%04x returning %04x\n", 
		   old_handle, handle );
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
 *           LOCAL_LockSegptr
 */
SEGPTR LOCAL_LockSegptr( HANDLE16 ds, HLOCAL16 handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    return PTR_SEG_OFF_TO_SEGPTR( ds, LOCAL_InternalLock( ptr, handle ) );
}


/***********************************************************************
 *           LOCAL_Unlock
 */
BOOL16 LOCAL_Unlock( HANDLE16 ds, HLOCAL16 handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );

    TRACE(local, "%04x\n", handle );
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

    TRACE(local, "%04x ds=%04x\n", handle, ds );

    if (HANDLE_MOVEABLE( handle )) handle = *(WORD *)(ptr + handle);
    if (!handle) return 0;
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
        TRACE(local, "(%04x,%04x): returning %04x\n",
                       ds, handle, pEntry->lock | (pEntry->flags << 8) );
        return pEntry->lock | (pEntry->flags << 8);
    }
    else
    {
        TRACE(local, "(%04x,%04x): returning 0\n",
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
        WARN(local, "(%04x): Local heap not found\n", ds );
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
    TRACE(local, "(%04x): returning %d\n", ds, total);
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
        WARN(local, "(%04x): Local heap not found\n", ds );
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
HLOCAL16 WINAPI LocalAlloc16( UINT16 flags, WORD size )
{
    return LOCAL_Alloc( CURRENT_DS, flags, size );
}


/***********************************************************************
 *           LocalReAlloc16   (KERNEL.6)
 */
HLOCAL16 WINAPI LocalReAlloc16( HLOCAL16 handle, WORD size, UINT16 flags )
{
    return LOCAL_ReAlloc( CURRENT_DS, handle, size, flags );
}


/***********************************************************************
 *           LocalFree16   (KERNEL.7)
 */
HLOCAL16 WINAPI LocalFree16( HLOCAL16 handle )
{
    return LOCAL_Free( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalLock16   (KERNEL.8)
 *
 * Note: only the offset part of the pointer is returned by the relay code.
 */
SEGPTR WINAPI LocalLock16( HLOCAL16 handle )
{
    return LOCAL_LockSegptr( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalUnlock16   (KERNEL.9)
 */
BOOL16 WINAPI LocalUnlock16( HLOCAL16 handle )
{
    return LOCAL_Unlock( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalSize16   (KERNEL.10)
 */
UINT16 WINAPI LocalSize16( HLOCAL16 handle )
{
    return LOCAL_Size( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalHandle16   (KERNEL.11)
 */
HLOCAL16 WINAPI LocalHandle16( WORD addr )
{ 
    return LOCAL_Handle( CURRENT_DS, addr );
}


/***********************************************************************
 *           LocalFlags16   (KERNEL.12)
 */
UINT16 WINAPI LocalFlags16( HLOCAL16 handle )
{
    return LOCAL_Flags( CURRENT_DS, handle );
}


/***********************************************************************
 *           LocalCompact16   (KERNEL.13)
 */
UINT16 WINAPI LocalCompact16( UINT16 minfree )
{
    TRACE(local, "%04x\n", minfree );
    return LOCAL_Compact( CURRENT_DS, minfree, 0 );
}


/***********************************************************************
 *           LocalNotify   (KERNEL.14)
 */
FARPROC16 WINAPI LocalNotify( FARPROC16 func )
{
    LOCALHEAPINFO *pInfo;
    FARPROC16 oldNotify;
    HANDLE16 ds = CURRENT_DS;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR(local, "(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }
    TRACE(local, "(%04x): %08lx\n", ds, (DWORD)func );
    FIXME(local, "Half implemented\n");
    oldNotify = pInfo->notify;
    pInfo->notify = func;
    return oldNotify;
}


/***********************************************************************
 *           LocalShrink16   (KERNEL.121)
 */
UINT16 WINAPI LocalShrink16( HGLOBAL16 handle, UINT16 newsize )
{
    TRACE(local, "%04x %04x\n", handle, newsize );
    return 0;
}


/***********************************************************************
 *           GetHeapSpaces   (KERNEL.138)
 */
DWORD WINAPI GetHeapSpaces( HMODULE16 module )
{
    NE_MODULE *pModule;
    WORD ds;

    if (!(pModule = NE_GetPtr( module ))) return 0;
    ds =
    GlobalHandleToSel((NE_SEG_TABLE( pModule ) + pModule->dgroup - 1)->hSeg);
    return MAKELONG( LOCAL_CountFree( ds ), LOCAL_HeapSize( ds ) );
}


/***********************************************************************
 *           LocalCountFree   (KERNEL.161)
 */
WORD WINAPI LocalCountFree(void)
{
    return LOCAL_CountFree( CURRENT_DS );
}


/***********************************************************************
 *           LocalHeapSize   (KERNEL.162)
 */
WORD WINAPI LocalHeapSize(void)
{
    TRACE(local, "(void)\n" );
    return LOCAL_HeapSize( CURRENT_DS );
}


/***********************************************************************
 *           LocalHandleDelta   (KERNEL.310)
 */
WORD WINAPI LocalHandleDelta( WORD delta )
{
    LOCALHEAPINFO *pInfo;

    if (!(pInfo = LOCAL_GetHeap( CURRENT_DS )))
    {
        ERR(local, "Local heap not found\n");
	LOCAL_PrintHeap( CURRENT_DS );
	return 0;
    }
    if (delta) pInfo->hdelta = delta;
    TRACE(local, "returning %04x\n", pInfo->hdelta);
    return pInfo->hdelta;
}


/***********************************************************************
 *           LocalInfo   (TOOLHELP.56)
 */
BOOL16 WINAPI LocalInfo( LOCALINFO *pLocalInfo, HGLOBAL16 handle )
{
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap(SELECTOROF(WIN16_GlobalLock16(handle)));
    if (!pInfo) return FALSE;
    pLocalInfo->wcItems = pInfo->items;
    return TRUE;
}


/***********************************************************************
 *           LocalFirst   (TOOLHELP.57)
 */
BOOL16 WINAPI LocalFirst( LOCALENTRY *pLocalEntry, HGLOBAL16 handle )
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
BOOL16 WINAPI LocalNext( LOCALENTRY *pLocalEntry )
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
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL32 WINAPI LocalAlloc32(
                UINT32 flags, /* [in] Allocation attributes */
                DWORD size    /* [in] Number of bytes to allocate */
) {
    return (HLOCAL32)GlobalAlloc32( flags, size );
}


/***********************************************************************
 *           LocalCompact32   (KERNEL32.372)
 */
UINT32 WINAPI LocalCompact32( UINT32 minfree )
{
    return 0;  /* LocalCompact does nothing in Win32 */
}


/***********************************************************************
 *           LocalFlags32   (KERNEL32.374)
 * RETURNS
 *	Value specifying allocation flags and lock count.
 *	LMEM_INVALID_HANDLE: Failure
 */
UINT32 WINAPI LocalFlags32(
              HLOCAL32 handle /* [in] Handle of memory object */
) {
    return GlobalFlags32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalFree32   (KERNEL32.375)
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 */
HLOCAL32 WINAPI LocalFree32(
                HLOCAL32 handle /* [in] Handle of memory object */
) {
    return (HLOCAL32)GlobalFree32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalHandle32   (KERNEL32.376)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL32 WINAPI LocalHandle32(
                LPCVOID ptr /* [in] Address of local memory object */
) {
    return (HLOCAL32)GlobalHandle32( ptr );
}


/***********************************************************************
 *           LocalLock32   (KERNEL32.377)
 * Locks a local memory object and returns pointer to the first byte
 * of the memory block.
 *
 * RETURNS
 *	Pointer: Success
 *	NULL: Failure
 */
LPVOID WINAPI LocalLock32(
              HLOCAL32 handle /* [in] Address of local memory object */
) {
    return GlobalLock32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalReAlloc32   (KERNEL32.378)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL32 WINAPI LocalReAlloc32(
                HLOCAL32 handle, /* [in] Handle of memory object */
                DWORD size,      /* [in] New size of block */
                UINT32 flags     /* [in] How to reallocate object */
) {
    return (HLOCAL32)GlobalReAlloc32( (HGLOBAL32)handle, size, flags );
}


/***********************************************************************
 *           LocalShrink32   (KERNEL32.379)
 */
UINT32 WINAPI LocalShrink32( HGLOBAL32 handle, UINT32 newsize )
{
    return 0;  /* LocalShrink does nothing in Win32 */
}


/***********************************************************************
 *           LocalSize32   (KERNEL32.380)
 * RETURNS
 *	Size: Success
 *	0: Failure
 */
UINT32 WINAPI LocalSize32(
              HLOCAL32 handle /* [in] Handle of memory object */
) {
    return GlobalSize32( (HGLOBAL32)handle );
}


/***********************************************************************
 *           LocalUnlock32   (KERNEL32.381)
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 */
BOOL32 WINAPI LocalUnlock32(
              HLOCAL32 handle /* [in] Handle of memory object */
) {
    return GlobalUnlock32( (HGLOBAL32)handle );
}
