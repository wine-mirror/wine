/*
 * Local heap functions
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Huw Davies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Note:
 * All local heap functions need the current DS as first parameter
 * when called from the emulation library, so they take one more
 * parameter than usual.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "instance.h"
#include "local.h"
#include "module.h"
#include "stackframe.h"
#include "toolhelp.h"
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

#include "pshpack1.h"

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

#include "poppack.h"

#define LOCAL_HEAP_MAGIC  0x484c  /* 'LH' */

  /* All local heap allocations are aligned on 4-byte boundaries */
#define LALIGN(word)          (((word) + 3) & ~3)

#define ARENA_PTR(ptr,arena)       ((LOCALARENA *)((char*)(ptr)+(arena)))
#define ARENA_PREV(ptr,arena)      (ARENA_PTR((ptr),(arena))->prev & ~3)
#define ARENA_NEXT(ptr,arena)      (ARENA_PTR((ptr),(arena))->next)
#define ARENA_FLAGS(ptr,arena)     (ARENA_PTR((ptr),(arena))->prev & 3)


/***********************************************************************
 *           LocalInit   (KERNEL.4)
 */
BOOL16 WINAPI LocalInit16( HANDLE16 selector, WORD start, WORD end )
{
    char *ptr;
    WORD heapInfoArena, freeArena, lastArena;
    LOCALHEAPINFO *pHeapInfo;
    LOCALARENA *pArena, *pFirstArena, *pLastArena;
    NE_MODULE *pModule;
    BOOL16 ret = FALSE;

      /* The initial layout of the heap is: */
      /* - first arena         (FIXED)      */
      /* - heap info structure (FIXED)      */
      /* - large free block    (FREE)       */
      /* - last arena          (FREE)       */

    TRACE("%04x %04x-%04x\n", selector, start, end);
    if (!selector) selector = CURRENT_DS;

    if (start == 0)
    {
        /* start == 0 means: put the local heap at the end of the segment */

        DWORD size = GlobalSize16( GlobalHandle16( selector ) );
	start = (WORD)(size > 0xffff ? 0xffff : size) - 1;
        if ( end > 0xfffe ) end = 0xfffe;
        start -= end;
        end += start;

        /* Paranoid check */

	if ((pModule = NE_GetPtr( GlobalHandle16( selector ) )))
	{
	    SEGTABLEENTRY *pSeg = NE_SEG_TABLE( pModule );
            int segNr;

            for ( segNr = 0; segNr < pModule->seg_count; segNr++, pSeg++ )
                if ( GlobalHandleToSel16(pSeg->hSeg) == selector )
                    break;

            if ( segNr < pModule->seg_count )
            {
                WORD minsize = pSeg->minsize;
                if ( pModule->ss == segNr+1 )
                    minsize += pModule->stack_size;

		TRACE(" new start %04x, minstart: %04x\n", start, minsize);
	    }
	}
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
    ret = TRUE;

 done:
    CURRENT_STACK16->ecx = ret;  /* must be returned in cx too */
    return ret;
}


/***********************************************************************
 *           LocalAlloc   (KERNEL32.@)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL WINAPI LocalAlloc(
                UINT flags, /* [in] Allocation attributes */
                SIZE_T size /* [in] Number of bytes to allocate */
) {
    return (HLOCAL)GlobalAlloc( flags, size );
}


/***********************************************************************
 *           LocalCompact   (KERNEL32.@)
 */
SIZE_T WINAPI LocalCompact( UINT minfree )
{
    return 0;  /* LocalCompact does nothing in Win32 */
}


/***********************************************************************
 *           LocalFlags   (KERNEL32.@)
 * RETURNS
 *	Value specifying allocation flags and lock count.
 *	LMEM_INVALID_HANDLE: Failure
 */
UINT WINAPI LocalFlags(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalFlags( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalFree   (KERNEL32.@)
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 */
HLOCAL WINAPI LocalFree(
                HLOCAL handle /* [in] Handle of memory object */
) {
    return (HLOCAL)GlobalFree( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalHandle   (KERNEL32.@)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL WINAPI LocalHandle(
                LPCVOID ptr /* [in] Address of local memory object */
) {
    return (HLOCAL)GlobalHandle( ptr );
}


/***********************************************************************
 *           LocalLock   (KERNEL32.@)
 * Locks a local memory object and returns pointer to the first byte
 * of the memory block.
 *
 * RETURNS
 *	Pointer: Success
 *	NULL: Failure
 */
LPVOID WINAPI LocalLock(
              HLOCAL handle /* [in] Address of local memory object */
) {
    return GlobalLock( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalReAlloc   (KERNEL32.@)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HLOCAL WINAPI LocalReAlloc(
                HLOCAL handle, /* [in] Handle of memory object */
                SIZE_T size,   /* [in] New size of block */
                UINT flags     /* [in] How to reallocate object */
) {
    return (HLOCAL)GlobalReAlloc( (HGLOBAL)handle, size, flags );
}


/***********************************************************************
 *           LocalShrink   (KERNEL32.@)
 */
SIZE_T WINAPI LocalShrink( HGLOBAL handle, UINT newsize )
{
    return 0;  /* LocalShrink does nothing in Win32 */
}


/***********************************************************************
 *           LocalSize   (KERNEL32.@)
 * RETURNS
 *	Size: Success
 *	0: Failure
 */
SIZE_T WINAPI LocalSize(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalSize( (HGLOBAL)handle );
}


/***********************************************************************
 *           LocalUnlock   (KERNEL32.@)
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 */
BOOL WINAPI LocalUnlock(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalUnlock( (HGLOBAL)handle );
}
