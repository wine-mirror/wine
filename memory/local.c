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

  /* Arena types (stored in 'prev' field of the arena) */
#define LOCAL_ARENA_FREE       0
#define LOCAL_ARENA_FIXED      1
#define LOCAL_ARENA_MOVEABLE   3



typedef struct
{
    WORD addr;                /* Address of the MOVEABLE block */
    BYTE flags;               /* Flags for this block */
    BYTE lock;                /* Lock count */
} LOCALHANDLEENTRY;

typedef struct
{
    WORD check;               /* Heap checking flag */
    WORD freeze;              /* Heap frozen flag */
    WORD items;               /* Count of items on the heap */
    WORD first;               /* First item of the heap */
    WORD pad1;                /* Always 0 */
    WORD last;                /* Last item of the heap */
    WORD pad2;                /* Always 0 */
    BYTE ncompact;            /* Compactions counter */
    BYTE dislevel;            /* Discard level */
    DWORD distotal;           /* Total bytes discarded */
    WORD htable;              /* Pointer to handle table */
    WORD hfree;               /* Pointer to free handle table */
    WORD hdelta;              /* Delta to expand the handle table */
    WORD expand;              /* Pointer to expand function (unused) */
    WORD pstat;               /* Pointer to status structure (unused) */
    DWORD notify WINE_PACKED; /* Pointer to LocalNotify() function */
    WORD lock;                /* Lock count for the heap */
    WORD extra;               /* Extra bytes to allocate when expanding */
    WORD minsize;             /* Minimum size of the heap */
    WORD magic;               /* Magic number */
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


/***********************************************************************
 *           LOCAL_GetHeap
 *
 * Return a pointer to the local heap, making sure it exists.
 */
static LOCALHEAPINFO *LOCAL_GetHeap( WORD ds )
{
    LOCALHEAPINFO *pInfo;
    INSTANCEDATA *ptr = (INSTANCEDATA *)PTR_SEG_OFF_TO_LIN( ds, 0 );
    if (!ptr->heap) return 0;
    pInfo = (LOCALHEAPINFO*)((char*)ptr + ptr->heap);
    if (pInfo->magic != LOCAL_HEAP_MAGIC) return NULL;
    return pInfo;
}


/***********************************************************************
 *           LOCAL_AddFreeBlock
 *
 * Make a block free, inserting it in the free-list.
 * 'block' is the handle of the block arena; 'baseptr' points to
 * the beginning of the data segment containing the heap.
 */
static void LOCAL_AddFreeBlock( char *baseptr, WORD block )
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
            printf( "*** arena->next->prev != arena\n" );
            break;
        }
        arena = pArena->next;
    }
}


/***********************************************************************
 *           LocalInit   (KERNEL.4)
 */
HLOCAL LocalInit( WORD selector, WORD start, WORD end )
{
    char *ptr;
    WORD heapInfoArena, freeArena, lastArena;
    LOCALHEAPINFO *pHeapInfo;
    LOCALARENA *pArena, *pFirstArena, *pLastArena;

      /* The initial layout of the heap is: */
      /* - first arena         (FIXED)      */
      /* - heap info structure (FIXED)      */
      /* - large free block    (FREE)       */
      /* - last arena          (FREE)       */

    dprintf_local(stddeb, "LocalInit: %04x %04x-%04x\n", selector, start, end);
    if (!selector) selector = CURRENT_DS;
    ptr = PTR_SEG_OFF_TO_LIN( selector, 0 );
    pHeapInfo = LOCAL_GetHeap(selector);
      /* If there's already a local heap in this segment, */
      /* we simply return TRUE. Helps some programs, but  */
      /* does not seem to be 100% correct yet (there are  */
      /* still some "heap corrupted" messages in LocalAlloc */
    if (pHeapInfo)  {
      dprintf_local(stddeb,"LocalInit: Heap %04x initialized twice.\n",selector);
      if (debugging_local) LOCAL_PrintHeap(selector);
      return TRUE;
    }
    start = LALIGN( max( start, sizeof(INSTANCEDATA) ) );
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
    pLastArena->prev      = heapInfoArena | LOCAL_ARENA_FREE;
    pLastArena->next      = lastArena;  /* this one */
    pLastArena->size      = LALIGN(sizeof(LOCALARENA));
    pLastArena->free_prev = freeArena;
    pLastArena->free_next = lastArena;  /* this one */

      /* Store the local heap address in the instance data */

    ((INSTANCEDATA *)ptr)->heap = heapInfoArena + ARENA_HEADER_SIZE;
    return TRUE;
}


/***********************************************************************
 *           LOCAL_Alloc
 *
 * Implementation of LocalAlloc().
 */
HLOCAL LOCAL_Alloc( WORD ds, WORD flags, WORD size )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena;
    WORD arena;

    dprintf_local( stddeb, "LocalAlloc: %04x %d ds=%04x\n", flags, size, ds );

      /* Find a suitable free block */

    if (!(pInfo = LOCAL_GetHeap( ds ))) {
	  LOCAL_PrintHeap(ds);
      return 0;
    }
    size += ARENA_HEADER_SIZE;
    size = LALIGN( max( size, sizeof(LOCALARENA) ) );
    arena = pInfo->first;
    pArena = ARENA_PTR( ptr, arena );
    for (;;)
    {
        if (arena == pArena->free_next) {
	  LOCAL_PrintHeap(ds);
	   return 0;  /* not found */
	}
        arena = pArena->free_next;
        pArena = ARENA_PTR( ptr, arena );
        if (pArena->size >= size) break;
    }

      /* Make a block out of the free arena */

    if (pArena->size > size + LALIGN(sizeof(LOCALARENA)))
    {
        LOCAL_AddBlock( ptr, arena, arena+size );
        LOCAL_AddFreeBlock( ptr, arena+size );
        pInfo->items++;
    }
    LOCAL_RemoveFreeBlock( ptr, arena );

    dprintf_local( stddeb, "LocalAlloc: returning %04x\n",
                   arena + ARENA_HEADER_SIZE );
    return arena + ARENA_HEADER_SIZE;
}


/***********************************************************************
 *           LOCAL_ReAlloc
 *
 * Implementation of LocalReAlloc().
 */
HLOCAL LOCAL_ReAlloc( WORD ds, HLOCAL handle, WORD size, WORD flags )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pNext;
    WORD arena, newhandle;

    dprintf_local( stddeb, "LocalReAlloc: %04x %d %04x ds=%04x\n",
                   handle, size, flags, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return 0;
    arena = handle - ARENA_HEADER_SIZE;
    pArena = ARENA_PTR( ptr, arena );
    if (!size) size = 1;
    size = LALIGN( size );

      /* Check for size reduction */

    if (size < pArena->next - handle)
    {
        if (handle + size < pArena->next - LALIGN(sizeof(LOCALARENA)))
        {
              /* It is worth making a new free block */
            LOCAL_AddBlock( ptr, arena, handle + size );
            LOCAL_AddFreeBlock( ptr, handle + size );
            pInfo->items++;
        }
        dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", handle );
        return handle;
    }

      /* Check if the next block is free */

    pNext = ARENA_PTR( ptr, pArena->next );
    if (((pNext->prev & 3) == LOCAL_ARENA_FREE) &&
        (size <= pNext->next - handle))
    {
        LOCAL_RemoveBlock( ptr, pArena->next );
        if (handle + size < pArena->next - LALIGN(sizeof(LOCALARENA)))
        {
              /* It is worth making a new free block */
            LOCAL_AddBlock( ptr, arena, handle + size );
            LOCAL_AddFreeBlock( ptr, handle + size );
            pInfo->items++;
        }
        dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", handle );
        return handle;
    }

      /* Now we have to allocate a new block */

    newhandle = LOCAL_Alloc( ds, flags, size );
    if (!newhandle) return 0;
    memcpy( ptr + newhandle, ptr + handle, pArena->next - handle );
    LOCAL_Free( ds, handle );
    dprintf_local( stddeb, "LocalReAlloc: returning %04x\n", newhandle );
    return newhandle;
}


/***********************************************************************
 *           LOCAL_Free
 *
 * Implementation of LocalFree().
 */
HLOCAL LOCAL_Free( WORD ds, HLOCAL handle )
{
    char *ptr = PTR_SEG_OFF_TO_LIN( ds, 0 );
    LOCALHEAPINFO *pInfo;
    LOCALARENA *pArena, *pPrev, *pNext;
    WORD arena;

    dprintf_local( stddeb, "LocalFree: %04x ds=%04x\n", handle, ds );
    if (!(pInfo = LOCAL_GetHeap( ds ))) return handle;
    arena = handle - ARENA_HEADER_SIZE;
    pArena = ARENA_PTR( ptr, arena );
    if ((pArena->prev & 3) == LOCAL_ARENA_FREE) return handle;

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
        LOCAL_AddFreeBlock( ptr, arena );
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
 *           LOCAL_Size
 *
 * Implementation of LocalSize().
 */
WORD LOCAL_Size( WORD ds, HLOCAL handle )
{
    LOCALARENA *pArena = PTR_SEG_OFF_TO_LIN( ds, handle - ARENA_HEADER_SIZE );
    return pArena->next - handle;
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
 *           LocalAlloc   (KERNEL.5)
 */
HLOCAL LocalAlloc( WORD flags, WORD size )
{
    return LOCAL_Alloc( CURRENT_DS, flags, size );
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL.6)
 */
HLOCAL LocalReAlloc( HLOCAL handle, WORD flags, WORD size )
{
    return LOCAL_ReAlloc( CURRENT_DS, handle, flags, size );
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
WORD LocalLock( HLOCAL handle )
{
    return handle;
}


/***********************************************************************
 *           LocalUnlock   (KERNEL.9)
 */
BOOL LocalUnlock( HLOCAL handle )
{
    return TRUE;
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
    dprintf_local( stddeb, "LocalHandle: %04x\n", addr );
    return addr;
}


/***********************************************************************
 *           LocalFlags   (KERNEL.12)
 */
WORD LocalFlags( HLOCAL handle )
{
    dprintf_local( stddeb, "LocalFlags: %04x\n", handle );
    return 0;
}


/***********************************************************************
 *           LocalCompact   (KERNEL.13)
 */
WORD LocalCompact( WORD minfree )
{
}


/***********************************************************************
 *           LocalNotify   (KERNEL.14)
 */
FARPROC LocalNotify( FARPROC func )
{
}


/***********************************************************************
 *           LocalShrink   (KERNEL.121)
 */
WORD LocalShrink( HLOCAL handle, WORD newsize )
{
}


/***********************************************************************
 *           GetHeapSpaces   (KERNEL.138)
 */
DWORD GetHeapSpaces( HMODULE module )
{
    return MAKELONG( 0x7fff, 0xffff );
}


/***********************************************************************
 *           LocalCountFree   (KERNEL.161)
 */
void LocalCountFree()
{
}


/***********************************************************************
 *           LocalHeapSize   (KERNEL.162)
 */
WORD LocalHeapSize()
{
    return LOCAL_HeapSize( CURRENT_DS );
}


/***********************************************************************
 *           LocalHandleDelta   (KERNEL.310)
 */
WORD LocalHandleDelta( WORD delta )
{
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
