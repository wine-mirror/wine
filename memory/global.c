/*
 * Global heap functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "toolhelp.h"
#include "selectors.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

  /* Global arena block */
typedef struct
{
    DWORD    base;           /* Base address  */
    DWORD    size;           /* Size in bytes */
    HGLOBAL  handle;         /* Handle for this block */
    HGLOBAL  hOwner;         /* Owner of this block */
    BYTE     lockCount;      /* Count of GlobalFix() calls */
    BYTE     pageLockCount;  /* Count of GlobalPageLock() calls */
    BYTE     flags;          /* Allocation flags */
    BYTE     selCount;       /* Number of selectors allocated for this block */
} GLOBALARENA;

  /* Flags definitions */
#define GA_DGROUP       0x04
#define GA_DISCARDABLE  0x08

  /* Arena array */
static GLOBALARENA *pGlobalArena = NULL;
static int globalArenaSize = 0;

#define GLOBAL_MAX_ALLOC_SIZE 0x00ff0000  /* Largest allocation is 16M - 64K */

#define GET_ARENA_PTR(handle)  (pGlobalArena + ((handle) >> __AHSHIFT))

/***********************************************************************
 *           GLOBAL_GetArena
 *
 * Return the arena for a given selector, growing the arena array if needed.
 */
static GLOBALARENA *GLOBAL_GetArena( WORD sel, WORD selcount )
{
    if (((sel >> __AHSHIFT) + selcount) >= globalArenaSize)
    {
        GLOBALARENA *pNewArena = realloc( pGlobalArena,
                               (globalArenaSize + 256) * sizeof(GLOBALARENA) );
        if (!pNewArena) return 0;
        pGlobalArena = pNewArena;
        memset( pGlobalArena + globalArenaSize, 0, 256 * sizeof(GLOBALARENA) );
        globalArenaSize += 256;
    }
    return pGlobalArena + (sel >> __AHSHIFT);
}


/***********************************************************************
 *           GLOBAL_Alloc
 *
 * Implementation of GlobalAlloc()
 */
HGLOBAL GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL hOwner,
                      BOOL isCode, BOOL isReadOnly )
{
    WORD sel, selcount;
    void *ptr;
    GLOBALARENA *pArena;

      /* Fixup the size */

    if (size >= GLOBAL_MAX_ALLOC_SIZE - 0x0f) return 0;
    if (size == 0) size = 0x10;
    else size = (size + 0x0f) & ~0x0f;

      /* Allocate the linear memory */

    ptr = malloc( size );
    if (!ptr) return 0;

      /* Allocate the selector(s) */

    sel = SELECTOR_AllocBlock( ptr, size, isCode ? SEGMENT_CODE : SEGMENT_DATA,
                               0, isReadOnly );
    if (!sel)
    {
        free( ptr );
        return 0;
    }
    selcount = (size + 0xffff) / 0x10000;

    if (!(pArena = GLOBAL_GetArena( sel, selcount )))
    {
        free( ptr );
        FreeSelector( sel );
        return 0;
    }

      /* Fill the arena block */

    pArena->base = (DWORD)ptr;
    pArena->size = GET_SEL_LIMIT(sel) + 1;
    pArena->handle = (flags & GMEM_MOVEABLE) ? sel - 1 : sel;
    pArena->hOwner = hOwner;
    pArena->lockCount = 0;
    pArena->pageLockCount = 0;
    pArena->flags = flags & 0xff;
    if (flags & GMEM_DISCARDABLE) pArena->flags |= GA_DISCARDABLE;
    if (!isCode) pArena->flags |= GA_DGROUP;
    pArena->selCount = selcount;
    if (selcount > 1)  /* clear the next arena blocks */
        memset( pArena + 1, 0, (selcount - 1) * sizeof(GLOBALARENA) );

    if (flags & GMEM_ZEROINIT) memset( ptr, 0, size );
    return pArena->handle;
}


/***********************************************************************
 *           GlobalAlloc   (KERNEL.15)
 */
HGLOBAL GlobalAlloc( WORD flags, DWORD size )
{
    dprintf_global( stddeb, "GlobalAlloc: %ld flags=%04x\n", size, flags );

    return GLOBAL_Alloc( flags, size, GetCurrentPDB(), 0, 0 );
}


/***********************************************************************
 *           GlobalReAlloc   (KERNEL.16)
 */
HGLOBAL GlobalReAlloc( HGLOBAL handle, DWORD size, WORD flags )
{
    WORD sel, selcount;
    DWORD oldsize;
    void *ptr;
    GLOBALARENA *pArena, *pNewArena;

    dprintf_global( stddeb, "GlobalReAlloc: %04x %ld flags=%04x\n",
                    handle, size, flags );
    if (!handle)
    {
        printf( "GlobalReAlloc: handle is 0.\n" );
        return 0;
    }

      /* Fixup the size */

    if (size >= 0x00ff0000-0x0f) return 0;  /* No allocation > 16Mb-64Kb */
    if (size == 0) size = 0x10;
    else size = (size + 0x0f) & ~0x0f;

      /* Reallocate the linear memory */

    pArena = GET_ARENA_PTR( handle );
    sel = GlobalHandleToSel( handle );
    ptr = (void *)pArena->base;
    oldsize = pArena->size;
    if (size == oldsize) return handle;  /* Nothing to do */

    ptr = realloc( ptr, size );
    if (!ptr)
    {
        FreeSelector( sel );
        memset( pArena, 0, sizeof(GLOBALARENA) );
        return 0;
    }

      /* Reallocate the selector(s) */

    sel = SELECTOR_ReallocBlock( sel, ptr, size, SEGMENT_DATA, 0, 0 );
    if (!sel)
    {
        free( ptr );
        memset( pArena, 0, sizeof(GLOBALARENA) );
        return 0;
    }
    selcount = (size + 0xffff) / 0x10000;

    if (!(pNewArena = GLOBAL_GetArena( sel, selcount )))
    {
        free( ptr );
        FreeSelector( sel );
        return 0;
    }

      /* Fill the new arena block */

    if (pNewArena != pArena) memcpy( pNewArena, pArena, sizeof(GLOBALARENA) );
    pNewArena->base = (DWORD)ptr;
    pNewArena->size = GET_SEL_LIMIT(sel) + 1;
    pNewArena->selCount = selcount;
    if (flags & GMEM_MODIFY)
    {
          /* Change the flags, leaving GA_DGROUP alone */
        pNewArena->flags = (pNewArena->flags & GA_DGROUP) |
                           (flags & ~GA_DGROUP);
        if (flags & GMEM_DISCARDABLE) pNewArena->flags |= GA_DISCARDABLE;
    }
    pNewArena->handle = (pNewArena->flags & GMEM_MOVEABLE) ? sel - 1 : sel;

    if (selcount > 1)  /* clear the next arena blocks */
        memset( pNewArena + 1, 0, (selcount - 1) * sizeof(GLOBALARENA) );

    if ((oldsize < size) && (flags & GMEM_ZEROINIT))
        memset( (char *)ptr + oldsize, 0, size - oldsize );
    return pNewArena->handle;
}


/***********************************************************************
 *           GlobalFree   (KERNEL.17)
 */
HGLOBAL GlobalFree( HGLOBAL handle )
{
    WORD sel;
    void *ptr;

    dprintf_global( stddeb, "GlobalFree: %04x\n", handle );
    if (!handle) return 0;
    sel = GlobalHandleToSel( handle );
    ptr = (void *)GET_SEL_BASE(sel);
    if (FreeSelector( sel )) return handle;  /* failed */
    free( ptr );
    memset( GET_ARENA_PTR(handle), 0, sizeof(GLOBALARENA) );
    return 0;
}


/***********************************************************************
 *           WIN16_GlobalLock   (KERNEL.18)
 *
 * This is the GlobalLock() function used by 16-bit code.
 */
SEGPTR WIN16_GlobalLock( HGLOBAL handle )
{
    dprintf_global( stddeb, "WIN16_GlobalLock: %04x\n", handle );
    if (!handle) return 0;
    return (SEGPTR)MAKELONG( 0, GlobalHandleToSel(handle) );
}


/***********************************************************************
 *           GlobalLock   (KERNEL.18)
 *
 * This is the GlobalLock() function used by 32-bit code.
 */
LPSTR GlobalLock( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalLock: %04x\n", handle );
    if (!handle) return 0;
    return (LPSTR)GET_ARENA_PTR(handle)->base;
}


/***********************************************************************
 *           GlobalUnlock   (KERNEL.19)
 */
BOOL GlobalUnlock( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalUnlock: %04x\n", handle );
    return 0;
}


/***********************************************************************
 *           GlobalSize   (KERNEL.20)
 */
DWORD GlobalSize( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalSize: %04x\n", handle );
    if (!handle) return 0;
    return GET_ARENA_PTR(handle)->size;
}


/***********************************************************************
 *           GlobalHandle   (KERNEL.21)
 */
DWORD GlobalHandle( WORD sel )
{
    dprintf_global( stddeb, "GlobalHandle: %04x\n", sel );
    return MAKELONG( GET_ARENA_PTR(sel)->handle, sel );
}


/***********************************************************************
 *           GlobalFlags   (KERNEL.22)
 */
WORD GlobalFlags( HGLOBAL handle )
{
    GLOBALARENA *pArena;

    dprintf_global( stddeb, "GlobalFlags: %04x\n", handle );
    pArena = GET_ARENA_PTR(handle);
    return pArena->lockCount |
           ((pArena->flags & GA_DISCARDABLE) ? GMEM_DISCARDABLE : 0);
}


/***********************************************************************
 *           LockSegment   (KERNEL.23)
 */
HGLOBAL LockSegment( HGLOBAL handle )
{
    dprintf_global( stddeb, "LockSegment: %04x\n", handle );
    if (handle == (HGLOBAL)-1) handle = CURRENT_DS;
    GET_ARENA_PTR(handle)->lockCount++;
    return handle;
}


/***********************************************************************
 *           UnlockSegment   (KERNEL.24)
 */
void UnlockSegment( HGLOBAL handle )
{
    dprintf_global( stddeb, "UnlockSegment: %04x\n", handle );
    if (handle == (HGLOBAL)-1) handle = CURRENT_DS;
    GET_ARENA_PTR(handle)->lockCount--;
    /* FIXME: this ought to return the lock count in CX (go figure...) */
}


/***********************************************************************
 *           GlobalCompact   (KERNEL.25)
 */
DWORD GlobalCompact( DWORD desired )
{
    return GLOBAL_MAX_ALLOC_SIZE;
}


/***********************************************************************
 *           GlobalWire   (KERNEL.111)
 */
SEGPTR GlobalWire( HGLOBAL handle )
{
    return WIN16_GlobalLock( handle );
}


/***********************************************************************
 *           GlobalUnWire   (KERNEL.112)
 */
BOOL GlobalUnWire( HGLOBAL handle )
{
    return GlobalUnlock( handle );
}


/***********************************************************************
 *           GlobalDOSAlloc   (KERNEL.184)
 */
DWORD GlobalDOSAlloc( DWORD size )
{
    WORD sel = GlobalAlloc( GMEM_FIXED, size );
    if (!sel) return 0;
    return MAKELONG( sel, sel /* this one ought to be a real-mode segment */ );
}


/***********************************************************************
 *           GlobalDOSFree   (KERNEL.185)
 */
WORD GlobalDOSFree( WORD sel )
{
    return GlobalFree( GlobalHandle(sel) ) ? sel : 0;
}


/***********************************************************************
 *           SetSwapAreaSize   (KERNEL.106)
 */
LONG SetSwapAreaSize( WORD size )
{
    dprintf_heap(stdnimp, "STUB: SetSwapAreaSize(%d)\n", size );
    return MAKELONG( size, 0xffff );
}


/***********************************************************************
 *           GlobalLRUOldest   (KERNEL.163)
 */
HGLOBAL GlobalLRUOldest( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalLRUOldest: %04x\n", handle );
    if (handle == (HGLOBAL)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GlobalLRUNewest   (KERNEL.164)
 */
HGLOBAL GlobalLRUNewest( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalLRUNewest: %04x\n", handle );
    if (handle == (HGLOBAL)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GetFreeSpace   (KERNEL.169)
 */
DWORD GetFreeSpace( UINT wFlags )
{
    return GLOBAL_MAX_ALLOC_SIZE;
}


/***********************************************************************
 *           GlobalPageLock   (KERNEL.191)
 */
WORD GlobalPageLock( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalPageLock: %04x\n", handle );
    return ++(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalPageUnlock   (KERNEL.192)
 */
WORD GlobalPageUnlock( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalPageUnlock: %04x\n", handle );
    return --(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalFix   (KERNEL.197)
 */
void GlobalFix( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalFix: %04x\n", handle );
    GET_ARENA_PTR(handle)->lockCount++;
}


/***********************************************************************
 *           GlobalUnfix   (KERNEL.198)
 */
void GlobalUnfix( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalUnfix: %04x\n", handle );
    GET_ARENA_PTR(handle)->lockCount--;
}


/***********************************************************************
 *           FarSetOwner   (KERNEL.403)
 */
void FarSetOwner( HANDLE handle, WORD hOwner )
{
    GET_ARENA_PTR(handle)->hOwner = hOwner;
}


/***********************************************************************
 *           FarGetOwner   (KERNEL.404)
 */
WORD FarGetOwner( HANDLE handle )
{
    return GET_ARENA_PTR(handle)->hOwner;
}


/***********************************************************************
 *           GlobalHandleToSel   (TOOLHELP.50)
 */
WORD GlobalHandleToSel( HGLOBAL handle )
{
    dprintf_toolhelp( stddeb, "GlobalHandleToSel: %04x\n", handle );
    if (!handle) return 0;
    if (!(handle & 7))
    {
        fprintf( stderr, "Program attempted invalid selector conversion\n" );
        return handle - 1;
    }
    return handle | 7;
}


/***********************************************************************
 *           GlobalFirst   (TOOLHELP.51)
 */
BOOL GlobalFirst( GLOBALENTRY *pGlobal, WORD wFlags )
{
    if (wFlags == GLOBAL_LRU) return FALSE;
    pGlobal->dwNext = 0;
    return GlobalNext( pGlobal, wFlags );
}


/***********************************************************************
 *           GlobalNext   (TOOLHELP.52)
 */
BOOL GlobalNext( GLOBALENTRY *pGlobal, WORD wFlags)
{
    GLOBALARENA *pArena;

    pArena = pGlobalArena + pGlobal->dwNext;
    if (wFlags == GLOBAL_FREE)  /* only free blocks */
    {
        int i;
        for (i = pGlobal->dwNext; i < globalArenaSize; i++, pArena++)
            if (pArena->size == 0) break;  /* block is free */
        if (i >= globalArenaSize) return FALSE;
        pGlobal->dwNext = i;
    }

    pGlobal->dwAddress    = pArena->base;
    pGlobal->dwBlockSize  = pArena->size;
    pGlobal->hBlock       = pArena->handle;
    pGlobal->wcLock       = pArena->lockCount;
    pGlobal->wcPageLock   = pArena->pageLockCount;
    pGlobal->wFlags       = (GetCurrentPDB() == pArena->hOwner);
    pGlobal->wHeapPresent = FALSE;
    pGlobal->hOwner       = pArena->hOwner;
    pGlobal->wType        = GT_UNKNOWN;
    pGlobal->wData        = 0;
    pGlobal->dwNext++;
    return TRUE;
}


/***********************************************************************
 *           GlobalInfo   (TOOLHELP.53)
 */
BOOL GlobalInfo( GLOBALINFO *pInfo )
{
    int i;
    GLOBALARENA *pArena;

    pInfo->wcItems = globalArenaSize;
    pInfo->wcItemsFree = 0;
    pInfo->wcItemsLRU = 0;
    for (i = 0, pArena = pGlobalArena; i < globalArenaSize; i++, pArena++)
        if (pArena->size == 0) pInfo->wcItemsFree++;
    return TRUE;
}


/***********************************************************************
 *           GlobalEntryHandle   (TOOLHELP.54)
 */
BOOL GlobalEntryHandle( GLOBALENTRY *pGlobal, HGLOBAL hItem )
{
    return FALSE;
}


/***********************************************************************
 *           GlobalEntryModule   (TOOLHELP.55)
 */
BOOL GlobalEntryModule( GLOBALENTRY *pGlobal, HMODULE hModule, WORD wSeg )
{
    return FALSE;
}


/***********************************************************************
 *           MemManInfo   (TOOLHELP.72)
 */
BOOL MemManInfo( MEMMANINFO *pInfo )
{
    return TRUE;
}
