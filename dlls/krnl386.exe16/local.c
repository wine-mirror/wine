/*
 * 16-bit local heap functions
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Huw Davies
 * Copyright 1998 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Note:
 * All local heap functions need the current DS as first parameter
 * when called from the emulation library, so they take one more
 * parameter than usual.
 */

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "wownt32.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(local);

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
#define ARENA_HEADER( handle) ((handle) - ARENA_HEADER_SIZE)

  /* Arena types (stored in 'prev' field of the arena) */
#define LOCAL_ARENA_FREE       0
#define LOCAL_ARENA_FIXED      1

/* LocalNotify() msgs */

#define LN_OUTOFMEM	0
#define LN_MOVE		1
#define LN_DISCARD	2

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
 *
 * An unused handle has lock = flags = 0xff. In windows addr is that of next
 * free handle, at the moment in wine we set it to 0.
 *
 * A discarded block's handle has lock = addr = 0 and flags = 0x40
 * (LMEM_DISCARDED >> 8)
 */

#define MOVEABLE_PREFIX sizeof(HLOCAL16)

#pragma pack(push,1)

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
    FARPROC16 notify;           /* 1e Pointer to LocalNotify() function */
    WORD lock;                  /* 22 Lock count for the heap */
    WORD extra;                 /* 24 Extra bytes to allocate when expanding */
    WORD minsize;               /* 26 Minimum size of the heap */
    WORD magic;                 /* 28 Magic number */
} LOCALHEAPINFO;

typedef struct
{
    DWORD dwSize;                /* 00 */
    DWORD dwMemReserved;         /* 04 */
    DWORD dwMemCommitted;        /* 08 */
    DWORD dwTotalFree;           /* 0C */
    DWORD dwLargestFreeBlock;    /* 10 */
    DWORD dwcFreeHandles;        /* 14 */
} LOCAL32INFO;

typedef struct
{
    DWORD dwSize;                /* 00 */
    WORD hHandle;                /* 04 */
    DWORD dwAddress;             /* 06 */
    DWORD dwSizeBlock;           /* 0A */
    WORD wFlags;                 /* 0E */
    WORD wType;                  /* 10 */
    WORD hHeap;                  /* 12 */
    WORD wHeapType;              /* 14 */
    DWORD dwNext;                /* 16 */
    DWORD dwNextAlt;             /* 1A */
} LOCAL32ENTRY;

#pragma pack(pop)

#define LOCAL_HEAP_MAGIC  0x484c  /* 'LH' */

  /* All local heap allocations are aligned on 4-byte boundaries */
#define LALIGN(word)          (((word) + 3) & ~3)

#define ARENA_PTR(ptr,arena)       ((LOCALARENA *)((char *)(ptr)+(arena)))
#define ARENA_PREV(ptr,arena)      (ARENA_PTR((ptr),(arena))->prev & ~3)
#define ARENA_NEXT(ptr,arena)      (ARENA_PTR((ptr),(arena))->next)
#define ARENA_FLAGS(ptr,arena)     (ARENA_PTR((ptr),(arena))->prev & 3)

  /* determine whether the handle belongs to a fixed or a moveable block */
#define HANDLE_FIXED(handle) (((handle) & 3) == 0)
#define HANDLE_MOVEABLE(handle) (((handle) & 3) == 2)


/* 32-bit heap definitions */

#define HTABLE_SIZE      0x10000
#define HTABLE_PAGESIZE  0x1000
#define HTABLE_NPAGES    (HTABLE_SIZE / HTABLE_PAGESIZE)

#pragma pack(push,1)
typedef struct _LOCAL32HEADER
{
    WORD     freeListFirst[HTABLE_NPAGES];
    WORD     freeListSize[HTABLE_NPAGES];
    WORD     freeListLast[HTABLE_NPAGES];

    DWORD    selectorTableOffset;
    WORD     selectorTableSize;
    WORD     selectorDelta;

    DWORD    segment;
    LPBYTE   base;

    DWORD    limit;
    DWORD    flags;

    DWORD    magic;
    HANDLE heap;

} LOCAL32HEADER;
#pragma pack(pop)

#define LOCAL32_MAGIC    ((DWORD)('L' | ('H'<<8) | ('3'<<16) | ('2'<<24)))


static inline BOOL16 call_notify_func( FARPROC16 proc, WORD msg, HLOCAL16 handle, WORD arg )
{
    DWORD ret;
    WORD args[3];

    if (!proc) return FALSE;
    args[2] = msg;
    args[1] = handle;
    args[0] = arg;
    WOWCallback16Ex( (DWORD)proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}


/***********************************************************************
 *           LOCAL_GetHeap
 *
 * Return a pointer to the local heap, making sure it exists.
 */
static LOCALHEAPINFO *LOCAL_GetHeap( HANDLE16 ds )
{
    LOCALHEAPINFO *pInfo;
    INSTANCEDATA *ptr = MapSL( MAKESEGPTR( ds, 0 ));
    TRACE("Heap at %p, %04x\n", ptr, (ptr != NULL ? ptr->heap : 0xFFFF));
    if (!ptr || !ptr->heap) return NULL;
    if (IsBadReadPtr16( (SEGPTR)MAKELONG(ptr->heap,ds), sizeof(LOCALHEAPINFO)))
    {
	WARN("Bad pointer\n");
        return NULL;
    }
    pInfo = (LOCALHEAPINFO*)((char*)ptr + ptr->heap);
    if (pInfo->magic != LOCAL_HEAP_MAGIC)
    {
	WARN("Bad magic\n");
	return NULL;
    }
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

    TRACE("%04x, next %04x\n", block, next );
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

    TRACE("\n");
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

    ptr = MapSL( MAKESEGPTR( ds, 0 ));
    pInfo = LOCAL_GetHeap( ds );

    if (!pInfo)
    {
        ERR( "Local Heap corrupted!  ds=%04x\n", ds );
        return;
    }
    TRACE( "Local Heap  ds=%04x first=%04x last=%04x items=%d\n",
             ds, pInfo->first, pInfo->last, pInfo->items );

    arena = pInfo->first;
    for (;;)
    {
        LOCALARENA *pArena = ARENA_PTR(ptr,arena);
        TRACE( "  %04x: prev=%04x next=%04x type=%d\n", arena,
               pArena->prev & ~3, pArena->next, pArena->prev & 3 );
        if (arena == pInfo->first)
	{
            TRACE( "        size=%d free_prev=%04x free_next=%04x\n",
                     pArena->size, pArena->free_prev, pArena->free_next );
	}
        if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
        {
            TRACE( "        size=%d free_prev=%04x free_next=%04x\n",
                     pArena->size, pArena->free_prev, pArena->free_next );
            if (pArena->next == arena) break;  /* last one */
            if (ARENA_PTR(ptr,pArena->free_next)->free_prev != arena)
            {
                TRACE( "*** arena->free_next->free_prev != arena\n" );
                break;
            }
        }
        if (pArena->next == arena)
        {
            TRACE( "*** last block is not marked free\n" );
            break;
        }
        if ((ARENA_PTR(ptr,pArena->next)->prev & ~3) != arena)
        {
            TRACE( "*** arena->next->prev != arena (%04x, %04x)\n",
                     pArena->next, ARENA_PTR(ptr,pArena->next)->prev);
            break;
        }
        arena = pArena->next;
    }
}


/***********************************************************************
 *           LocalInit   (KERNEL.4)
 */
BOOL16 WINAPI LocalInit16( HANDLE16 selector, WORD start, WORD end )
{
    char *ptr;
    WORD heapInfoArena, freeArena, lastArena;
    LOCALHEAPINFO *pHeapInfo;
    LOCALARENA *pArena, *pFirstArena, *pLastArena;
    BOOL16 ret = FALSE;

      /* The initial layout of the heap is: */
      /* - first arena         (FIXED)      */
      /* - heap info structure (FIXED)      */
      /* - large free block    (FREE)       */
      /* - last arena          (FREE)       */

    TRACE("%04x %04x-%04x\n", selector, start, end);
    if (!selector) selector = CURRENT_DS;

    if (TRACE_ON(local))
    {
        /* If TRACE_ON(local) is set, the global heap blocks are */
        /* cleared before use, so we can test for double initialization. */
        if (LOCAL_GetHeap(selector))
        {
            ERR("Heap %04x initialized twice.\n", selector);
            LOCAL_PrintHeap(selector);
        }
    }

    if (start == 0)
    {
        /* start == 0 means: put the local heap at the end of the segment */

        DWORD size = GlobalSize16( GlobalHandle16( selector ) );
	start = (WORD)(size > 0xffff ? 0xffff : size) - 1;
        if ( end > 0xfffe ) end = 0xfffe;
        start -= end;
        end += start;
    }
    ptr = MapSL( MAKESEGPTR( selector, 0 ) );

    start = LALIGN( max( start, sizeof(INSTANCEDATA) ) );
    heapInfoArena = LALIGN(start + sizeof(LOCALARENA) );
    freeArena = LALIGN( heapInfoArena + ARENA_HEADER_SIZE
                        + sizeof(LOCALHEAPINFO) );
    lastArena = (end - sizeof(LOCALARENA)) & ~3;

      /* Make sure there's enough space.       */

    if (freeArena + sizeof(LOCALARENA) >= lastArena) goto done;

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
    ret = TRUE;

 done:
    CURRENT_STACK16->ecx = ret;  /* must be returned in cx too */
    return ret;
}


/***********************************************************************
 *           LOCAL_GrowHeap
 */
static BOOL16 LOCAL_GrowHeap( HANDLE16 ds )
{
    HANDLE16 hseg;
    LONG oldsize;
    LONG end;
    LOCALHEAPINFO *pHeapInfo;
    WORD freeArena, lastArena;
    LOCALARENA *pArena, *pLastArena;
    char *ptr;

    hseg = GlobalHandle16( ds );
    /* maybe mem allocated by Virtual*() ? */
    if (!hseg) return FALSE;

    oldsize = GlobalSize16( hseg );
    /* if nothing can be gained, return */
    if (oldsize > 0xfff0) return FALSE;
    hseg = GlobalReAlloc16( hseg, 0x10000, GMEM_FIXED );
    ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    pHeapInfo = LOCAL_GetHeap( ds );
    if (pHeapInfo == NULL) {
	ERR("Heap not found\n" );
	return FALSE;
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

    TRACE("Heap expanded\n" );
    LOCAL_PrintHeap( ds );
    return TRUE;
}


/***********************************************************************
 *           LOCAL_FreeArena
 */
static HLOCAL16 LOCAL_FreeArena( WORD ds, WORD arena )
{
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pPrev;

    TRACE("%04x ds=%04x\n", arena, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return arena;

    pArena = ARENA_PTR( ptr, arena );
    if ((pArena->prev & 3) == LOCAL_ARENA_FREE)
    {
	/* shouldn't happen */
        ERR("Trying to free block %04x twice!\n",
                 arena );
	LOCAL_PrintHeap( ds );
	return arena;
    }

      /* Check if we can merge with the previous block */

    pPrev = ARENA_PTR( ptr, pArena->prev & ~3 );
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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;
    WORD freespace = 0;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n" );
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
static UINT16 LOCAL_Compact( HANDLE16 ds, UINT16 minfree, UINT16 flags )
{
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pMoveArena, *pFinalArena;
    WORD arena, movearena, finalarena, table;
    WORD count, movesize, size;
    WORD freespace;
    LOCALHANDLEENTRY *pEntry;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n" );
        LOCAL_PrintHeap(ds);
        return 0;
    }
    TRACE("ds = %04x, minfree = %04x, flags = %04x\n",
		 ds, minfree, flags);
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NOCOMPACT))
    {
        TRACE("Returning %04x.\n", freespace);
        return freespace;
    }
    TRACE("Compacting heap %04x.\n", ds);
    table = pInfo->htable;
    while(table)
    {
        pEntry = (LOCALHANDLEENTRY *)(ptr + table + sizeof(WORD));
        for(count = *(WORD *)(ptr + table); count > 0; count--, pEntry++)
        {
            if((pEntry->lock == 0) && (pEntry->flags != (LMEM_DISCARDED >> 8)))
            {
                /* OK we can move this one if we want */
                TRACE("handle %04x (block %04x) can be moved.\n",
			     (WORD)((char *)pEntry - ptr), pEntry->addr);
                movearena = ARENA_HEADER(pEntry->addr - MOVEABLE_PREFIX);
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
                    TRACE("Moving it to %04x.\n", finalarena);
                    pFinalArena = ARENA_PTR(ptr, finalarena);
                    size = pFinalArena->size;
                    LOCAL_RemoveFreeBlock(ptr, finalarena);
                    LOCAL_ShrinkArena( ds, finalarena, movesize );
                    /* Copy the arena to its new location */
                    memcpy((char *)pFinalArena + ARENA_HEADER_SIZE,
                           (char *)pMoveArena + ARENA_HEADER_SIZE,
                           movesize - ARENA_HEADER_SIZE );
                    /* Free the old location */
                    LOCAL_FreeArena(ds, movearena);
                    call_notify_func(pInfo->notify, LN_MOVE,
                                     (WORD)((char *)pEntry - ptr), pEntry->addr);
                    /* Update handle table entry */
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE + MOVEABLE_PREFIX;
                }
                else if((ARENA_PTR(ptr, pMoveArena->prev & ~3)->prev & 3)
			       == LOCAL_ARENA_FREE)
                {
                    /* Previous arena is free (but < movesize)  */
                    /* so we can 'slide' movearena down into it */
                    finalarena = pMoveArena->prev & ~3;
                    LOCAL_GrowArenaDownward( ds, movearena, movesize );
                    /* Update handle table entry */
                    pEntry->addr = finalarena + ARENA_HEADER_SIZE + MOVEABLE_PREFIX;
                }
            }
        }
        table = *(WORD *)pEntry;
    }
    freespace = LOCAL_GetFreeSpace(ds, minfree ? 0 : 1);
    if(freespace >= minfree || (flags & LMEM_NODISCARD))
    {
        TRACE("Returning %04x.\n", freespace);
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
                TRACE("Discarding handle %04x (block %04x).\n",
                              (char *)pEntry - ptr, pEntry->addr);
                LOCAL_FreeArena(ds, ARENA_HEADER(pEntry->addr - MOVEABLE_PREFIX));
                call_notify_func(pInfo->notify, LN_DISCARD, (char *)pEntry - ptr, pEntry->flags);
                pEntry->addr = 0;
                pEntry->flags = (LMEM_DISCARDED >> 8);
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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n" );
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
    TRACE("not enough space\n" );
    LOCAL_PrintHeap(ds);
    return 0;
}


/***********************************************************************
 *           get_heap_name
 */
static const char *get_heap_name( WORD ds )
{
    HINSTANCE16 inst = LoadLibrary16( "GDI" );
    if (ds == GlobalHandleToSel16( inst ))
    {
        FreeLibrary16( inst );
        return "GDI";
    }
    FreeLibrary16( inst );
    inst = LoadLibrary16( "USER" );
    if (ds == GlobalHandleToSel16( inst ))
    {
        FreeLibrary16( inst );
        return "USER";
    }
    FreeLibrary16( inst );
    return "local";
}

/***********************************************************************
 *           LOCAL_GetBlock
 * The segment may get moved around in this function, so all callers
 * should reset their pointer variables.
 */
static HLOCAL16 LOCAL_GetBlock( HANDLE16 ds, WORD size, WORD flags )
{
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n");
	LOCAL_PrintHeap(ds);
	return 0;
    }

    size += ARENA_HEADER_SIZE;
    size = LALIGN( max( size, sizeof(LOCALARENA) ) );

#if 0
notify_done:
#endif
      /* Find a suitable free block */
    arena = LOCAL_FindFreeBlock( ds, size );
    if (arena == 0) {
	/* no space: try to make some */
	LOCAL_Compact( ds, size, flags );
	arena = LOCAL_FindFreeBlock( ds, size );
    }
    if (arena == 0) {
	/* still no space: try to grow the segment */
	if (!(LOCAL_GrowHeap( ds )))
	{
#if 0
	    /* FIXME: doesn't work correctly yet */
	    if (call_notify_func(pInfo->notify, LN_OUTOFMEM, ds - 20, size)) /* FIXME: "size" correct ? (should indicate bytes needed) */
		goto notify_done;
#endif
            ERR( "not enough space in %s heap %04x for %d bytes\n",
                 get_heap_name(ds), ds, size );
	    return 0;
	}
	ptr = MapSL( MAKESEGPTR( ds, 0 ) );
	pInfo = LOCAL_GetHeap( ds );
	arena = LOCAL_FindFreeBlock( ds, size );
    }
    if (arena == 0) {
        ERR( "not enough space in %s heap %04x for %d bytes\n",
             get_heap_name(ds), ds, size );
#if 0
        /* FIXME: "size" correct ? (should indicate bytes needed) */
        if (call_notify_func(pInfo->notify, LN_OUTOFMEM, ds, size)) goto notify_done;
#endif
	return 0;
    }

      /* Make a block out of the free arena */
    pArena = ARENA_PTR( ptr, arena );
    TRACE("size = %04x, arena %04x size %04x\n", size, arena, pArena->size );
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
    char *ptr;
    LOCALHEAPINFO *pInfo;
    LOCALHANDLEENTRY *pEntry;
    HLOCAL16 handle;
    int i;

    TRACE("\n" );
    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n");
        LOCAL_PrintHeap(ds);
        return FALSE;
    }

    if (!(handle = LOCAL_GetBlock( ds, pInfo->hdelta * sizeof(LOCALHANDLEENTRY)
                                   + 2 * sizeof(WORD), LMEM_FIXED )))
        return FALSE;
    if (!(ptr = MapSL( MAKESEGPTR( ds, 0 ) )))
        ERR("ptr == NULL after GetBlock.\n");
    if (!(pInfo = LOCAL_GetHeap( ds )))
        ERR("pInfo == NULL after GetBlock.\n");

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
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    LOCALHANDLEENTRY *pEntry = NULL;
    WORD table;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("Local heap not found\n");
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
	ptr = MapSL( MAKESEGPTR( ds, 0 ) );
	pInfo = LOCAL_GetHeap( ds );
        pEntry = (LOCALHANDLEENTRY *)(ptr + pInfo->htable + sizeof(WORD));
    }

    /* Now allocate this entry */

    pEntry->lock = 0;
    pEntry->flags = 0;
    TRACE("(%04x): %04x\n", ds, ((char *)pEntry - ptr) );
    return (HLOCAL16)((char *)pEntry - ptr);
}


/***********************************************************************
 *           LOCAL_FreeHandleEntry
 *
 * Free a handle table entry.
 */
static void LOCAL_FreeHandleEntry( HANDLE16 ds, HLOCAL16 handle )
{
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
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
        ERR("Invalid entry %04x\n", handle);
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

    TRACE("(%04x): freeing table %04x\n", ds, table);
    *pTable = *(WORD *)pEntry;
    LOCAL_FreeArena( ds, ARENA_HEADER( table ) );
}


/***********************************************************************
 *           LocalFree   (KERNEL.7)
 */
HLOCAL16 WINAPI LocalFree16( HLOCAL16 handle )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );

    TRACE("%04x ds=%04x\n", handle, ds );

    if (!handle) { WARN("Handle is 0.\n" ); return 0; }
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
            TRACE("real block at %04x\n", pEntry->addr );
            if (LOCAL_FreeArena( ds, ARENA_HEADER(pEntry->addr - MOVEABLE_PREFIX) ))
                return handle; /* couldn't free it */
        }
        LOCAL_FreeHandleEntry( ds, handle );
        return 0;  /* OK */
    }
}


/***********************************************************************
 *           LocalAlloc   (KERNEL.5)
 */
HLOCAL16 WINAPI LocalAlloc16( UINT16 flags, WORD size )
{
    HANDLE16 ds = CURRENT_DS;
    HLOCAL16 handle = 0;
    char *ptr;

    TRACE("%04x %d ds=%04x\n", flags, size, ds );

    if(size > 0 && size <= 4) size = 5;
    if (flags & LMEM_MOVEABLE)
    {
	LOCALHANDLEENTRY *plhe;
	HLOCAL16 hmem;

	if(size)
	{
	    if (!(hmem = LOCAL_GetBlock( ds, size + MOVEABLE_PREFIX, flags )))
		goto exit;
        }
	else /* We just need to allocate a discarded handle */
	    hmem = 0;
	if (!(handle = LOCAL_GetNewHandleEntry( ds )))
        {
	    WARN("Couldn't get handle.\n");
	    if(hmem)
		LOCAL_FreeArena( ds, ARENA_HEADER(hmem) );
	    goto exit;
	}
	ptr = MapSL( MAKESEGPTR( ds, 0 ) );
	plhe = (LOCALHANDLEENTRY *)(ptr + handle);
	plhe->lock = 0;
	if(hmem)
	{
	    plhe->addr = hmem + MOVEABLE_PREFIX;
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
	if(size) handle = LOCAL_GetBlock( ds, size, flags );
    }

exit:
    CURRENT_STACK16->ecx = handle;  /* must be returned in cx too */
    return handle;
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL.6)
 */
HLOCAL16 WINAPI LocalReAlloc16( HLOCAL16 handle, WORD size, UINT16 flags )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
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

    TRACE("%04x %d %04x ds=%04x\n", handle, size, flags, ds );
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
		WARN("Dicarded block has non-zero addr.\n");
	    TRACE("ReAllocating discarded block\n");
	    if(size <= 4) size = 5;
	    if (!(hl = LOCAL_GetBlock( ds, size + MOVEABLE_PREFIX, flags)))
		return 0;
            ptr = MapSL( MAKESEGPTR( ds, 0 ) );  /* Reload ptr */
            pEntry = (LOCALHANDLEENTRY *) (ptr + handle);
	    pEntry->addr = hl + MOVEABLE_PREFIX;
            pEntry->flags = 0;
            pEntry->lock = 0;
	    *(HLOCAL16 *)(ptr + hl) = handle;
            return handle;
	}
	if (((blockhandle = pEntry->addr - MOVEABLE_PREFIX) & 3) != 0)
	{
	    ERR("(%04x,%04x): invalid handle\n",
                     ds, handle );
	    return 0;
        }
	if (*(HLOCAL16 *)(ptr + blockhandle) != handle) {
	    ERR("Back ptr to handle is invalid\n");
	    return 0;
        }
    }

    if (flags & LMEM_MODIFY)
    {
        if (HANDLE_MOVEABLE(handle))
	{
	    pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
	    pEntry->flags = (flags & 0x0f00) >> 8;
	    TRACE("Changing flags to %x.\n", pEntry->flags);
	}
	return handle;
    }

    if (!size)
    {
        if (flags & LMEM_MOVEABLE)
        {
	    if (HANDLE_FIXED(handle))
	    {
                TRACE("Freeing fixed block.\n");
                return LocalFree16( handle );
            }
	    else /* Moveable block */
	    {
		pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
		if (pEntry->lock == 0)
		{
		    /* discards moveable blocks */
                    TRACE("Discarding block\n");
                    LOCAL_FreeArena(ds, ARENA_HEADER(pEntry->addr - MOVEABLE_PREFIX));
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
		return LocalFree16( handle );
	    }
        }
        return 0;
    }

    arena = ARENA_HEADER( blockhandle );
    TRACE("arena is %04x\n", arena );
    pArena = ARENA_PTR( ptr, arena );

    if(size <= 4) size = 5;
    if(HANDLE_MOVEABLE(handle)) size += MOVEABLE_PREFIX;
    oldsize = pArena->next - arena - ARENA_HEADER_SIZE;
    nextarena = LALIGN(blockhandle + size);

      /* Check for size reduction */

    if (nextarena <= pArena->next)
    {
	TRACE("size reduction, making new free block\n");
	LOCAL_ShrinkArena(ds, arena, nextarena - arena);
        TRACE("returning %04x\n", handle );
        return handle;
    }

      /* Check if the next block is free and large enough */

    pNext = ARENA_PTR( ptr, pArena->next );
    if (((pNext->prev & 3) == LOCAL_ARENA_FREE) &&
        (nextarena <= pNext->next))
    {
	TRACE("size increase, making new free block\n");
        LOCAL_GrowArenaUpward(ds, arena, nextarena - arena);
        if (flags & LMEM_ZEROINIT)
        {
            char *oldend = (char *)pArena + ARENA_HEADER_SIZE + oldsize;
            char *newend = ptr + pArena->next;
            TRACE("Clearing memory from %p to %p (DS -> %p)\n", oldend, newend, ptr);
            memset(oldend, 0, newend - oldend);
        }

        TRACE("returning %04x\n", handle );
        return handle;
    }

    /* Now we have to allocate a new block, but not if (fixed block or locked
       block) and no LMEM_MOVEABLE */

    if (!(flags & LMEM_MOVEABLE))
    {
	if (HANDLE_FIXED(handle))
        {
            ERR("Needed to move fixed block, but LMEM_MOVEABLE not specified.\n");
            return 0;
        }
	else
	{
	    if(((LOCALHANDLEENTRY *)(ptr + handle))->lock != 0)
	    {
		ERR("Needed to move locked block, but LMEM_MOVEABLE not specified.\n");
		return 0;
	    }
        }
    }

    hmem = LOCAL_GetBlock( ds, size, flags );
    ptr = MapSL( MAKESEGPTR( ds, 0 ));  /* Reload ptr                             */
    if(HANDLE_MOVEABLE(handle))         /* LOCAL_GetBlock might have triggered    */
    {                                   /* a compaction, which might in turn have */
      blockhandle = pEntry->addr - MOVEABLE_PREFIX; /* moved the very block we are resizing */
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
                ERR("Can't restore saved block\n" );
                HeapFree( GetProcessHeap(), 0, buffer );
                return 0;
            }
            size = oldsize;
        }
        ptr = MapSL( MAKESEGPTR( ds, 0 ) );  /* Reload ptr */
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
	TRACE("fixing handle\n");
        pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        pEntry->addr = hmem + MOVEABLE_PREFIX;
	/* Back ptr should still be correct */
	if(*(HLOCAL16 *)(ptr + hmem) != handle)
	    ERR("back ptr is invalid.\n");
	hmem = handle;
    }
    if (size == oldsize) hmem = 0;  /* Realloc failed */
    TRACE("returning %04x\n", hmem );
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
        if (pEntry->flags == (LMEM_DISCARDED >> 8)) return 0;
        if (pEntry->lock < 0xfe) pEntry->lock++;
        handle = pEntry->addr;
    }
    TRACE("%04x returning %04x\n", old_handle, handle );
    return handle;
}


/***********************************************************************
 *           LocalUnlock   (KERNEL.9)
 */
BOOL16 WINAPI LocalUnlock16( HLOCAL16 handle )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );

    TRACE("%04x\n", handle );
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
 *           LocalSize   (KERNEL.10)
 */
UINT16 WINAPI LocalSize16( HLOCAL16 handle )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALARENA *pArena;

    TRACE("%04x ds=%04x\n", handle, ds );

    if (!handle) return 0;
    if (HANDLE_MOVEABLE( handle ))
    {
        handle = *(WORD *)(ptr + handle);
        if (!handle) return 0;
        pArena = ARENA_PTR( ptr, ARENA_HEADER(handle - MOVEABLE_PREFIX) );
    }
    else
        pArena = ARENA_PTR( ptr, ARENA_HEADER(handle) );

    return pArena->next - handle;
}


/***********************************************************************
 *           LocalFlags   (KERNEL.12)
 */
UINT16 WINAPI LocalFlags16( HLOCAL16 handle )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );

    if (HANDLE_MOVEABLE(handle))
    {
        LOCALHANDLEENTRY *pEntry = (LOCALHANDLEENTRY *)(ptr + handle);
        TRACE("(%04x,%04x): returning %04x\n",
                       ds, handle, pEntry->lock | (pEntry->flags << 8) );
        return pEntry->lock | (pEntry->flags << 8);
    }
    else
    {
        TRACE("(%04x,%04x): returning 0\n",
                       ds, handle );
        return 0;
    }
}


/***********************************************************************
 *           LocalHeapSize   (KERNEL.162)
 */
WORD WINAPI LocalHeapSize16(void)
{
    HANDLE16 ds = CURRENT_DS;
    LOCALHEAPINFO *pInfo = LOCAL_GetHeap( ds );
    return pInfo ? pInfo->last - pInfo->first : 0;
}


/***********************************************************************
 *           LocalCountFree   (KERNEL.161)
 */
WORD WINAPI LocalCountFree16(void)
{
    HANDLE16 ds = CURRENT_DS;
    WORD arena, total;
    LOCALARENA *pArena;
    LOCALHEAPINFO *pInfo;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("(%04x): Local heap not found\n", ds );
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
    TRACE("(%04x): returning %d\n", ds, total);
    return total;
}


/***********************************************************************
 *           LocalHandle   (KERNEL.11)
 */
HLOCAL16 WINAPI LocalHandle16( WORD addr )
{
    HANDLE16 ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    LOCALHEAPINFO *pInfo;
    WORD table;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("(%04x): Local heap not found\n", ds );
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
 *           LocalLock   (KERNEL.8)
 *
 * Note: only the offset part of the pointer is returned by the relay code.
 */
SEGPTR WINAPI LocalLock16( HLOCAL16 handle )
{
    WORD ds = CURRENT_DS;
    char *ptr = MapSL( MAKESEGPTR( ds, 0 ) );
    return MAKESEGPTR( ds, LOCAL_InternalLock( ptr, handle ) );
}


/***********************************************************************
 *           LocalCompact   (KERNEL.13)
 */
UINT16 WINAPI LocalCompact16( UINT16 minfree )
{
    TRACE("%04x\n", minfree );
    return LOCAL_Compact( CURRENT_DS, minfree, 0 );
}


/***********************************************************************
 *           LocalNotify   (KERNEL.14)
 *
 * Installs a callback function that is called for local memory events
 * Callback function prototype is
 * BOOL16 NotifyFunc(WORD wMsg, HLOCAL16 hMem, WORD wArg)
 * wMsg:
 * - LN_OUTOFMEM
 *   NotifyFunc seems to be responsible for allocating some memory,
 *   returns TRUE for success.
 *   wArg = number of bytes needed additionally
 * - LN_MOVE
 *   hMem = handle; wArg = old mem location
 * - LN_DISCARD
 *   NotifyFunc seems to be strongly encouraged to return TRUE,
 *   otherwise LogError() gets called.
 *   hMem = handle; wArg = flags
 */
FARPROC16 WINAPI LocalNotify16( FARPROC16 func )
{
    LOCALHEAPINFO *pInfo;
    FARPROC16 oldNotify;
    HANDLE16 ds = CURRENT_DS;

    if (!(pInfo = LOCAL_GetHeap( ds )))
    {
        ERR("(%04x): Local heap not found\n", ds );
	LOCAL_PrintHeap( ds );
	return 0;
    }
    TRACE("(%04x): %p\n", ds, func );
    FIXME("Half implemented\n");
    oldNotify = pInfo->notify;
    pInfo->notify = func;
    return oldNotify;
}


/***********************************************************************
 *           LocalShrink   (KERNEL.121)
 */
UINT16 WINAPI LocalShrink16( HGLOBAL16 handle, UINT16 newsize )
{
    TRACE("%04x %04x\n", handle, newsize );
    return 0;
}


/***********************************************************************
 *           GetHeapSpaces   (KERNEL.138)
 */
DWORD WINAPI GetHeapSpaces16( HMODULE16 module )
{
    NE_MODULE *pModule;
    WORD oldDS = CURRENT_DS;
    DWORD spaces;

    if (!(pModule = NE_GetPtr( module ))) return 0;
    CURRENT_DS = GlobalHandleToSel16((NE_SEG_TABLE( pModule ) + pModule->ne_autodata - 1)->hSeg);
    spaces = MAKELONG( LocalCountFree16(), LocalHeapSize16() );
    CURRENT_DS = oldDS;
    return spaces;
}


/***********************************************************************
 *           LocalHandleDelta   (KERNEL.310)
 */
WORD WINAPI LocalHandleDelta16( WORD delta )
{
    LOCALHEAPINFO *pInfo;

    if (!(pInfo = LOCAL_GetHeap( CURRENT_DS )))
    {
        ERR("Local heap not found\n");
	LOCAL_PrintHeap( CURRENT_DS );
	return 0;
    }
    if (delta) pInfo->hdelta = delta;
    TRACE("returning %04x\n", pInfo->hdelta);
    return pInfo->hdelta;
}


/***********************************************************************
 * 32-bit local heap functions (Win95; undocumented)
 */

/***********************************************************************
 *           K208   (KERNEL.208)
 */
HANDLE WINAPI Local32Init16( WORD segment, DWORD tableSize,
                             DWORD heapSize, DWORD flags )
{
    DWORD totSize, segSize = 0;
    LPBYTE base;
    LOCAL32HEADER *header;
    HANDLE heap;
    WORD *selectorTable;
    WORD selectorEven, selectorOdd;
    int i, nrBlocks;

    /* Determine new heap size */

    if ( segment )
    {
        if ( (segSize = GetSelectorLimit16( segment )) == 0 )
            return 0;
        else
            segSize++;
    }

    if ( heapSize == (DWORD)-1 )
        heapSize = 1024*1024;   /* FIXME */

    heapSize = (heapSize + 0xffff) & 0xffff0000;
    segSize  = (segSize  + 0x0fff) & 0xfffff000;
    totSize  = segSize + HTABLE_SIZE + heapSize;


    /* Allocate memory and initialize heap */

    if ( !(base = VirtualAlloc( NULL, totSize, MEM_RESERVE, PAGE_READWRITE )) )
        return 0;

    if ( !VirtualAlloc( base, segSize + HTABLE_PAGESIZE,
                        MEM_COMMIT, PAGE_READWRITE ) )
    {
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }

    if (!(heap = RtlCreateHeap( 0, base + segSize + HTABLE_SIZE, heapSize, 0x10000, NULL, NULL )))
    {
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }


    /* Set up header and handle table */

    header = (LOCAL32HEADER *)(base + segSize);
    header->base    = base;
    header->limit   = HTABLE_PAGESIZE-1;
    header->flags   = 0;
    header->magic   = LOCAL32_MAGIC;
    header->heap    = heap;

    header->freeListFirst[0] = sizeof(LOCAL32HEADER);
    header->freeListLast[0]  = HTABLE_PAGESIZE - 4;
    header->freeListSize[0]  = (HTABLE_PAGESIZE - sizeof(LOCAL32HEADER)) / 4;

    for (i = header->freeListFirst[0]; i < header->freeListLast[0]; i += 4)
        *(DWORD *)((LPBYTE)header + i) = i+4;

    header->freeListFirst[1] = 0xffff;


    /* Set up selector table */

    nrBlocks      = (totSize + 0x7fff) >> 15;
    selectorTable = HeapAlloc( header->heap,  0, nrBlocks * 2 );
    selectorEven  = SELECTOR_AllocBlock( base, totSize, LDT_FLAGS_DATA );
    selectorOdd   = SELECTOR_AllocBlock( base + 0x8000, totSize - 0x8000, LDT_FLAGS_DATA );
    if ( !selectorTable || !selectorEven || !selectorOdd )
    {
        HeapFree( header->heap, 0, selectorTable );
        if ( selectorEven  ) SELECTOR_FreeBlock( selectorEven );
        if ( selectorOdd   ) SELECTOR_FreeBlock( selectorOdd );
        HeapDestroy( header->heap );
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }

    header->selectorTableOffset = (LPBYTE)selectorTable - header->base;
    header->selectorTableSize   = nrBlocks * 4;  /* ??? Win95 does it this way! */
    header->selectorDelta       = selectorEven - selectorOdd;
    header->segment             = segment? segment : selectorEven;

    for (i = 0; i < nrBlocks; i++)
        selectorTable[i] = (i & 1)? selectorOdd  + ((i >> 1) << __AHSHIFT)
                                  : selectorEven + ((i >> 1) << __AHSHIFT);

    /* Move old segment */

    if ( segment )
    {
        /* FIXME: This is somewhat ugly and relies on implementation
                  details about 16-bit global memory handles ... */

        LPBYTE oldBase = (LPBYTE)GetSelectorBase( segment );
        memcpy( base, oldBase, segSize );
        GLOBAL_MoveBlock( segment, base, totSize );
        HeapFree( GetProcessHeap(), 0, oldBase );
    }

    return header;
}

/***********************************************************************
 *           Local32_SearchHandle
 */
static LPDWORD Local32_SearchHandle( LOCAL32HEADER *header, DWORD addr )
{
    LPDWORD handle;

    for ( handle = (LPDWORD)((LPBYTE)header + sizeof(LOCAL32HEADER));
          handle < (LPDWORD)((LPBYTE)header + header->limit);
          handle++)
    {
        if (*handle == addr)
            return handle;
    }

    return NULL;
}

/***********************************************************************
 *           Local32_ToHandle
 */
static VOID Local32_ToHandle( LOCAL32HEADER *header, INT16 type,
                              DWORD addr, LPDWORD *handle, LPBYTE *ptr )
{
    *handle = NULL;
    *ptr    = NULL;

    switch (type)
    {
        case -2:    /* 16:16 pointer, no handles */
            *ptr    = MapSL( addr );
            *handle = (LPDWORD)*ptr;
            break;

        case -1:    /* 32-bit offset, no handles */
            *ptr    = header->base + addr;
            *handle = (LPDWORD)*ptr;
            break;

        case 0:     /* handle */
            if (    addr >= sizeof(LOCAL32HEADER)
                 && addr <  header->limit && !(addr & 3)
                 && *(LPDWORD)((LPBYTE)header + addr) >= HTABLE_SIZE )
            {
                *handle = (LPDWORD)((LPBYTE)header + addr);
                *ptr    = header->base + **handle;
            }
            break;

        case 1:     /* 16:16 pointer */
            *ptr    = MapSL( addr );
            *handle = Local32_SearchHandle( header, *ptr - header->base );
            break;

        case 2:     /* 32-bit offset */
            *ptr    = header->base + addr;
            *handle = Local32_SearchHandle( header, *ptr - header->base );
            break;
    }
}

/***********************************************************************
 *           Local32_FromHandle
 */
static VOID Local32_FromHandle( LOCAL32HEADER *header, INT16 type,
                                DWORD *addr, LPDWORD handle, LPBYTE ptr )
{
    *addr = 0;
    switch (type)
    {
        case -2:    /* 16:16 pointer */
        case  1:
        {
            WORD *selTable = (LPWORD)(header->base + header->selectorTableOffset);
            DWORD offset   = ptr - header->base;
            *addr = MAKELONG( offset & 0x7fff, selTable[offset >> 15] );
        }
        break;

        case -1:    /* 32-bit offset */
        case  2:
            *addr = ptr - header->base;
            break;

        case  0:    /* handle */
            *addr = (LPBYTE)handle - (LPBYTE)header;
            break;
    }
}

/***********************************************************************
 *           K209   (KERNEL.209)
 */
DWORD WINAPI Local32Alloc16( HANDLE heap, DWORD size, INT16 type, DWORD flags )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;
    DWORD addr;

    /* Allocate memory */
    ptr = HeapAlloc( header->heap,
                     (flags & LMEM_MOVEABLE)? HEAP_ZERO_MEMORY : 0, size );
    if (!ptr) return 0;


    /* Allocate handle if requested */
    if (type >= 0)
    {
        int page, i;

        /* Find first page of handle table with free slots */
        for (page = 0; page < HTABLE_NPAGES; page++)
            if (header->freeListFirst[page] != 0)
                break;
        if (page == HTABLE_NPAGES)
        {
            WARN("Out of handles!\n" );
            HeapFree( header->heap, 0, ptr );
            return 0;
        }

        /* If virgin page, initialize it */
        if (header->freeListFirst[page] == 0xffff)
        {
            if ( !VirtualAlloc( (LPBYTE)header + (page << 12),
                                0x1000, MEM_COMMIT, PAGE_READWRITE ) )
            {
                WARN("Cannot grow handle table!\n" );
                HeapFree( header->heap, 0, ptr );
                return 0;
            }

            header->limit += HTABLE_PAGESIZE;

            header->freeListFirst[page] = 0;
            header->freeListLast[page]  = HTABLE_PAGESIZE - 4;
            header->freeListSize[page]  = HTABLE_PAGESIZE / 4;

            for (i = 0; i < HTABLE_PAGESIZE; i += 4)
                *(DWORD *)((LPBYTE)header + i) = i+4;

            if (page < HTABLE_NPAGES-1)
                header->freeListFirst[page+1] = 0xffff;
        }

        /* Allocate handle slot from page */
        handle = (LPDWORD)((LPBYTE)header + header->freeListFirst[page]);
        if (--header->freeListSize[page] == 0)
            header->freeListFirst[page] = header->freeListLast[page] = 0;
        else
            header->freeListFirst[page] = *handle;

        /* Store 32-bit offset in handle slot */
        *handle = ptr - header->base;
    }
    else
    {
        handle = (LPDWORD)ptr;
        header->flags |= 1;
    }


    /* Convert handle to requested output type */
    Local32_FromHandle( header, type, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K210   (KERNEL.210)
 */
DWORD WINAPI Local32ReAlloc16( HANDLE heap, DWORD addr, INT16 type,
                             DWORD size, DWORD flags )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;

    if (!addr)
        return Local32Alloc16( heap, size, type, flags );

    /* Retrieve handle and pointer */
    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return FALSE;

    /* Reallocate memory block */
    ptr = HeapReAlloc( header->heap,
                       (flags & LMEM_MOVEABLE)? HEAP_ZERO_MEMORY : 0,
                       ptr, size );
    if (!ptr) return 0;

    /* Modify handle */
    if (type >= 0)
        *handle = ptr - header->base;
    else
        handle = (LPDWORD)ptr;

    /* Convert handle to requested output type */
    Local32_FromHandle( header, type, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K211   (KERNEL.211)
 */
BOOL WINAPI Local32Free16( HANDLE heap, DWORD addr, INT16 type )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;

    /* Retrieve handle and pointer */
    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return FALSE;

    /* Free handle if necessary */
    if (type >= 0)
    {
        int offset = (LPBYTE)handle - (LPBYTE)header;
        int page   = offset >> 12;

        /* Return handle slot to page free list */
        if (header->freeListSize[page]++ == 0)
            header->freeListFirst[page] = header->freeListLast[page]  = offset;
        else
            *(LPDWORD)((LPBYTE)header + header->freeListLast[page]) = offset,
            header->freeListLast[page] = offset;

        *handle = 0;

        /* Shrink handle table when possible */
        while (page > 0 && header->freeListSize[page] == HTABLE_PAGESIZE / 4)
        {
            if ( VirtualFree( (LPBYTE)header +
                              (header->limit & ~(HTABLE_PAGESIZE-1)),
                              HTABLE_PAGESIZE, MEM_DECOMMIT ) )
                break;

            header->limit -= HTABLE_PAGESIZE;
            header->freeListFirst[page] = 0xffff;
            page--;
        }
    }

    /* Free memory */
    return HeapFree( header->heap, 0, ptr );
}

/***********************************************************************
 *           K213   (KERNEL.213)
 */
DWORD WINAPI Local32Translate16( HANDLE heap, DWORD addr, INT16 type1, INT16 type2 )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, type1, addr, &handle, &ptr );
    if (!handle) return 0;

    Local32_FromHandle( header, type2, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K214   (KERNEL.214)
 */
DWORD WINAPI Local32Size16( HANDLE heap, DWORD addr, INT16 type )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return 0;

    return HeapSize( header->heap, 0, ptr );
}

/***********************************************************************
 *           K215   (KERNEL.215)
 */
BOOL WINAPI Local32ValidHandle16( HANDLE heap, WORD addr )
{
    LOCAL32HEADER *header = heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, 0, addr, &handle, &ptr );
    return handle != NULL;
}

/***********************************************************************
 *           K229   (KERNEL.229)
 */
WORD WINAPI Local32GetSegment16( HANDLE heap )
{
    LOCAL32HEADER *header = heap;
    return header->segment;
}

/***********************************************************************
 *           Local32_GetHeap
 */
static LOCAL32HEADER *Local32_GetHeap( HGLOBAL16 handle )
{
    WORD selector = GlobalHandleToSel16( handle );
    DWORD base  = GetSelectorBase( selector );
    DWORD limit = GetSelectorLimit16( selector );

    /* Hmmm. This is a somewhat stupid heuristic, but Windows 95 does
       it this way ... */

    if ( limit > 0x10000 && ((LOCAL32HEADER *)base)->magic == LOCAL32_MAGIC )
        return (LOCAL32HEADER *)base;

    base  += 0x10000;
    limit -= 0x10000;

    if ( limit > 0x10000 && ((LOCAL32HEADER *)base)->magic == LOCAL32_MAGIC )
        return (LOCAL32HEADER *)base;

    return NULL;
}

/***********************************************************************
 *           Local32Info   (KERNEL.444)
 */
BOOL16 WINAPI Local32Info16( LOCAL32INFO *pLocal32Info, HGLOBAL16 handle )
{
    PROCESS_HEAP_ENTRY entry;
    int i;

    LOCAL32HEADER *header = Local32_GetHeap( handle );
    if ( !header ) return FALSE;

    if ( !pLocal32Info || pLocal32Info->dwSize < sizeof(LOCAL32INFO) )
        return FALSE;

    pLocal32Info->dwMemReserved = 0;
    pLocal32Info->dwMemCommitted = 0;
    pLocal32Info->dwTotalFree = 0;
    pLocal32Info->dwLargestFreeBlock = 0;

    while (HeapWalk( header->heap, &entry ))
    {
        if (entry.wFlags & PROCESS_HEAP_REGION)
        {
            pLocal32Info->dwMemReserved += entry.Region.dwCommittedSize
                                           + entry.Region.dwUnCommittedSize;
            pLocal32Info->dwMemCommitted = entry.Region.dwCommittedSize;
        }
        else if (!(entry.wFlags & PROCESS_HEAP_ENTRY_BUSY))
        {
            DWORD size = entry.cbData + entry.cbOverhead;
            pLocal32Info->dwTotalFree += size;
            if (size > pLocal32Info->dwLargestFreeBlock) pLocal32Info->dwLargestFreeBlock = size;
        }
    }

    pLocal32Info->dwcFreeHandles = 0;
    for ( i = 0; i < HTABLE_NPAGES; i++ )
    {
        if ( header->freeListFirst[i] == 0xffff ) break;
        pLocal32Info->dwcFreeHandles += header->freeListSize[i];
    }
    pLocal32Info->dwcFreeHandles += (HTABLE_NPAGES - i) * HTABLE_PAGESIZE/4;

    return TRUE;
}

/***********************************************************************
 *           Local32First   (KERNEL.445)
 */
BOOL16 WINAPI Local32First16( LOCAL32ENTRY *pLocal32Entry, HGLOBAL16 handle )
{
    FIXME("(%p, %04X): stub!\n", pLocal32Entry, handle );
    return FALSE;
}

/***********************************************************************
 *           Local32Next   (KERNEL.446)
 */
BOOL16 WINAPI Local32Next16( LOCAL32ENTRY *pLocal32Entry )
{
    FIXME("(%p): stub!\n", pLocal32Entry );
    return FALSE;
}
