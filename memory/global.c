/*
 * Global heap functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "windows.h"
#include "global.h"
#include "heap.h"
#include "toolhelp.h"
#include "selectors.h"
#include "dde_mem.h"
#include "stackframe.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"

  /* Global arena block */
typedef struct
{
    DWORD     base;          /* Base address (0 if discarded) */
    DWORD     size;          /* Size in bytes (0 indicates a free block) */
    HGLOBAL16 handle;        /* Handle for this block */
    HGLOBAL16 hOwner;        /* Owner of this block */
    BYTE      lockCount;     /* Count of GlobalFix() calls */
    BYTE      pageLockCount; /* Count of GlobalPageLock() calls */
    BYTE      flags;         /* Allocation flags */
    BYTE      selCount;      /* Number of selectors allocated for this block */
#ifdef CONFIG_IPC
    int       shmid;
#endif
} GLOBALARENA;

  /* Flags definitions */
#define GA_MOVEABLE     0x02  /* same as GMEM_MOVEABLE */
#define GA_DGROUP       0x04
#define GA_DISCARDABLE  0x08
#define GA_IPCSHARE     0x10  /* same as GMEM_DDESHARE */

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
    if (((sel >> __AHSHIFT) + selcount) > globalArenaSize)
    {
        int newsize = ((sel >> __AHSHIFT) + selcount + 0xff) & ~0xff;
        GLOBALARENA *pNewArena = realloc( pGlobalArena,
                                          newsize * sizeof(GLOBALARENA) );
        if (!pNewArena) return 0;
        pGlobalArena = pNewArena;
        memset( pGlobalArena + globalArenaSize, 0,
                (newsize - globalArenaSize) * sizeof(GLOBALARENA) );
        globalArenaSize = newsize;
    }
    return pGlobalArena + (sel >> __AHSHIFT);
}


void debug_handles()
{
    int printed=0;
    int i;
    for (i = globalArenaSize-1 ; i>=0 ; i--) {
	if (pGlobalArena[i].size!=0 && (pGlobalArena[i].handle & 0x8000)){
	    printed=1;
	    printf("0x%08x, ",pGlobalArena[i].handle);
	}
    }
    if (printed)
	printf("\n");
}


/***********************************************************************
 *           GLOBAL_CreateBlock
 *
 * Create a global heap block for a fixed range of linear memory.
 */
HGLOBAL16 GLOBAL_CreateBlock( WORD flags, const void *ptr, DWORD size,
                              HGLOBAL16 hOwner, BOOL16 isCode,
                              BOOL16 is32Bit, BOOL16 isReadOnly,
                              SHMDATA *shmdata  )
{
    WORD sel, selcount;
    GLOBALARENA *pArena;

      /* Allocate the selector(s) */

    sel = SELECTOR_AllocBlock( ptr, size,
			      isCode ? SEGMENT_CODE : SEGMENT_DATA,
			      is32Bit, isReadOnly );
    
    if (!sel) return 0;
    selcount = (size + 0xffff) / 0x10000;

    if (!(pArena = GLOBAL_GetArena( sel, selcount )))
    {
        FreeSelector( sel );
        return 0;
    }

      /* Fill the arena block */

    pArena->base = (DWORD)ptr;
    pArena->size = GET_SEL_LIMIT(sel) + 1;

#ifdef CONFIG_IPC
    if ((flags & GMEM_DDESHARE) && Options.ipc)
    {
	pArena->handle = shmdata->handle;
	pArena->shmid  = shmdata->shmid;
	shmdata->sel   = sel;
    }
    else
    {
	pArena->handle = (flags & GMEM_MOVEABLE) ? sel - 1 : sel;
	pArena->shmid  = 0;
    }
#else
    pArena->handle = (flags & GMEM_MOVEABLE) ? sel - 1 : sel;
#endif
    pArena->hOwner = hOwner;
    pArena->lockCount = 0;
    pArena->pageLockCount = 0;
    pArena->flags = flags & GA_MOVEABLE;
    if (flags & GMEM_DISCARDABLE) pArena->flags |= GA_DISCARDABLE;
    if (flags & GMEM_DDESHARE) pArena->flags |= GA_IPCSHARE;
    if (!isCode) pArena->flags |= GA_DGROUP;
    pArena->selCount = selcount;
    if (selcount > 1)  /* clear the next arena blocks */
        memset( pArena + 1, 0, (selcount - 1) * sizeof(GLOBALARENA) );

    return pArena->handle;
}


/***********************************************************************
 *           GLOBAL_FreeBlock
 *
 * Free a block allocated by GLOBAL_CreateBlock, without touching
 * the associated linear memory range.
 */
BOOL16 GLOBAL_FreeBlock( HGLOBAL16 handle )
{
    WORD sel;

    if (!handle) return TRUE;
    sel = GlobalHandleToSel( handle );
    if (FreeSelector( sel )) return FALSE;  /* failed */
    memset( GET_ARENA_PTR(sel), 0, sizeof(GLOBALARENA) );
    return TRUE;
}


/***********************************************************************
 *           GLOBAL_Alloc
 *
 * Implementation of GlobalAlloc16()
 */
HGLOBAL16 GLOBAL_Alloc( UINT16 flags, DWORD size, HGLOBAL16 hOwner,
                        BOOL16 isCode, BOOL16 is32Bit, BOOL16 isReadOnly )
{
    void *ptr;
    HGLOBAL16 handle;
    SHMDATA shmdata;

    dprintf_global( stddeb, "GlobalAlloc: %ld flags=%04x\n", size, flags );

    /* If size is 0, create a discarded block */

    if (size == 0) return GLOBAL_CreateBlock( flags, NULL, 1, hOwner, isCode,
                                              is32Bit, isReadOnly, NULL );

    /* Fixup the size */

    if (size >= GLOBAL_MAX_ALLOC_SIZE - 0x1f) return 0;
    size = (size + 0x1f) & ~0x1f;

      /* Allocate the linear memory */

#ifdef CONFIG_IPC
    if ((flags & GMEM_DDESHARE) && Options.ipc)
        ptr = DDE_malloc(flags, size, &shmdata);
    else 
#endif  /* CONFIG_IPC */
    {
	ptr = HeapAlloc( SystemHeap, 0, size );
    }
    if (!ptr) return 0;

      /* Allocate the selector(s) */

    handle = GLOBAL_CreateBlock( flags, ptr, size, hOwner,
				isCode, is32Bit, isReadOnly, &shmdata);
    if (!handle)
    {
        HeapFree( SystemHeap, 0, ptr );
        return 0;
    }

    if (flags & GMEM_ZEROINIT) memset( ptr, 0, size );
    return handle;
}


#ifdef CONFIG_IPC
/***********************************************************************
 *           GLOBAL_FindArena
 *
 * Find the arena  for a given handle
 * (when handle is not serial - e.g. DDE)
 */
static GLOBALARENA *GLOBAL_FindArena( HGLOBAL handle)
{
    int i;
    for (i = globalArenaSize-1 ; i>=0 ; i--) {
	if (pGlobalArena[i].size!=0 && pGlobalArena[i].handle == handle)
	    return ( &pGlobalArena[i] );
    }
    return NULL;
}


/***********************************************************************
 *           DDE_GlobalHandleToSel
 */

WORD DDE_GlobalHandleToSel( HGLOBAL handle )
{
    GLOBALARENA *pArena;
    SEGPTR segptr;
    
    pArena= GLOBAL_FindArena(handle);
    if (pArena) {
	int ArenaIdx = pArena - pGlobalArena;

	/* See if synchronized to the shared memory  */
	return DDE_SyncHandle(handle, ( ArenaIdx << __AHSHIFT) | 7);
    }

    /* attach the block */
    DDE_AttachHandle(handle, &segptr);

    return SELECTOROF( segptr );
}
#endif  /* CONFIG_IPC */


/***********************************************************************
 *           GlobalAlloc16   (KERNEL.15)
 */
HGLOBAL16 GlobalAlloc16( UINT16 flags, DWORD size )
{
    HANDLE16 owner = GetCurrentPDB();

    if (flags & GMEM_DDESHARE)
        owner = GetExePtr(owner);  /* Make it a module handle */
    return GLOBAL_Alloc( flags, size, owner, FALSE, FALSE, FALSE );
}


/***********************************************************************
 *           GlobalReAlloc16   (KERNEL.16)
 */
HGLOBAL16 GlobalReAlloc16( HGLOBAL16 handle, DWORD size, UINT16 flags )
{
    WORD selcount;
    DWORD oldsize;
    void *ptr;
    GLOBALARENA *pArena, *pNewArena;
    WORD sel = GlobalHandleToSel( handle );

    dprintf_global( stddeb, "GlobalReAlloc16: %04x %ld flags=%04x\n",
                    handle, size, flags );
    if (!handle) return 0;
    
#ifdef CONFIG_IPC
    if (Options.ipc && (flags & GMEM_DDESHARE || is_dde_handle(handle))) {
	fprintf(stdnimp,
               "GlobalReAlloc16: shared memory reallocating unimplemented\n"); 
	return 0;
    }
#endif  /* CONFIG_IPC */

    pArena = GET_ARENA_PTR( handle );

      /* Discard the block if requested */

    if ((size == 0) && (flags & GMEM_MOVEABLE) && !(flags & GMEM_MODIFY))
    {
        if (!(pArena->flags & GA_MOVEABLE) ||
            !(pArena->flags & GA_DISCARDABLE) ||
            (pArena->lockCount > 0) || (pArena->pageLockCount > 0)) return 0;
        HeapFree( SystemHeap, 0, (void *)pArena->base );
        pArena->base = 0;
        /* Note: we rely on the fact that SELECTOR_ReallocBlock won't */
        /* change the selector if we are shrinking the block */
        SELECTOR_ReallocBlock( sel, 0, 1, SEGMENT_DATA, 0, 0 );
        return handle;
    }

      /* Fixup the size */

    if (size > GLOBAL_MAX_ALLOC_SIZE - 0x20) return 0;
    if (size == 0) size = 0x20;
    else size = (size + 0x1f) & ~0x1f;

      /* Change the flags */

    if (flags & GMEM_MODIFY)
    {
          /* Change the flags, leaving GA_DGROUP alone */
        pArena->flags = (pArena->flags & GA_DGROUP) | (flags & GA_MOVEABLE);
        if (flags & GMEM_DISCARDABLE) pArena->flags |= GA_DISCARDABLE;
        return handle;
    }

      /* Reallocate the linear memory */

    ptr = (void *)pArena->base;
    oldsize = pArena->size;
    dprintf_global(stddeb,"oldsize %08lx\n",oldsize);
    if (ptr && (size == oldsize)) return handle;  /* Nothing to do */

    ptr = HeapReAlloc( SystemHeap, 0, ptr, size );
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
        HeapFree( SystemHeap, 0, ptr );
        memset( pArena, 0, sizeof(GLOBALARENA) );
        return 0;
    }
    selcount = (size + 0xffff) / 0x10000;

    if (!(pNewArena = GLOBAL_GetArena( sel, selcount )))
    {
        HeapFree( SystemHeap, 0, ptr );
        FreeSelector( sel );
        return 0;
    }

      /* Fill the new arena block */

    if (pNewArena != pArena) memcpy( pNewArena, pArena, sizeof(GLOBALARENA) );
    pNewArena->base = (DWORD)ptr;
    pNewArena->size = GET_SEL_LIMIT(sel) + 1;
    pNewArena->selCount = selcount;
    pNewArena->handle = (pNewArena->flags & GA_MOVEABLE) ? sel - 1 : sel;

    if (selcount > 1)  /* clear the next arena blocks */
        memset( pNewArena + 1, 0, (selcount - 1) * sizeof(GLOBALARENA) );

    if ((oldsize < size) && (flags & GMEM_ZEROINIT))
        memset( (char *)ptr + oldsize, 0, size - oldsize );
    return pNewArena->handle;
}


/***********************************************************************
 *           GlobalFree16   (KERNEL.17)
 */
HGLOBAL16 GlobalFree16( HGLOBAL16 handle )
{
    void *ptr = GlobalLock16( handle );

    dprintf_global( stddeb, "GlobalFree16: %04x\n", handle );
    if (!GLOBAL_FreeBlock( handle )) return handle;  /* failed */
#ifdef CONFIG_IPC
    if (is_dde_handle(handle)) return DDE_GlobalFree(handle);
#endif  /* CONFIG_IPC */
    if (ptr) HeapFree( SystemHeap, 0, ptr );
    return 0;
}


/***********************************************************************
 *           WIN16_GlobalLock16   (KERNEL.18)
 *
 * This is the GlobalLock16() function used by 16-bit code.
 */
SEGPTR WIN16_GlobalLock16( HGLOBAL16 handle )
{
#ifndef WINELIB
    dprintf_global( stddeb, "WIN16_GlobalLock16(%04x) -> %08lx\n",
                    handle, MAKELONG( 0, GlobalHandleToSel(handle)) );
    if (!handle) return 0;

#ifdef CONFIG_IPC
    if (is_dde_handle(handle))
        return PTR_SEG_OFF_TO_SEGPTR( DDE_GlobalHandleToSel(handle), 0 );
#endif  /* CONFIG_IPC */

    if (!GET_ARENA_PTR(handle)->base) return (SEGPTR)0;
    return PTR_SEG_OFF_TO_SEGPTR( GlobalHandleToSel(handle), 0 );
#else  /* WINELIB */
    return GlobalLock16( handle );
#endif  /* WINELIB */
}


/***********************************************************************
 *           GlobalLock16   (KERNEL.18)
 *
 * This is the GlobalLock16() function used by 32-bit code.
 */
LPVOID GlobalLock16( HGLOBAL16 handle )
{
    if (!handle) return 0;
#ifdef CONFIG_IPC
    if (is_dde_handle(handle)) return DDE_AttachHandle(handle, NULL);
#endif
    return (LPVOID)GET_ARENA_PTR(handle)->base;
}


/***********************************************************************
 *           GlobalUnlock16   (KERNEL.19)
 */
BOOL16 GlobalUnlock16( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalUnlock16: %04x\n", handle );
    return 0;
}


/***********************************************************************
 *           GlobalSize16   (KERNEL.20)
 */
DWORD GlobalSize16( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalSize16: %04x\n", handle );
    if (!handle) return 0;
    return GET_ARENA_PTR(handle)->size;
}


/***********************************************************************
 *           GlobalHandle16   (KERNEL.21)
 */
DWORD GlobalHandle16( WORD sel )
{
    dprintf_global( stddeb, "GlobalHandle16: %04x\n", sel );
    return MAKELONG( GET_ARENA_PTR(sel)->handle, GlobalHandleToSel(sel) );
}


/***********************************************************************
 *           GlobalFlags16   (KERNEL.22)
 */
UINT16 GlobalFlags16( HGLOBAL16 handle )
{
    GLOBALARENA *pArena;

    dprintf_global( stddeb, "GlobalFlags16: %04x\n", handle );
    pArena = GET_ARENA_PTR(handle);
    return pArena->lockCount |
           ((pArena->flags & GA_DISCARDABLE) ? GMEM_DISCARDABLE : 0) |
           ((pArena->base == 0) ? GMEM_DISCARDED : 0);
}


/***********************************************************************
 *           LockSegment16   (KERNEL.23)
 */
HGLOBAL16 LockSegment16( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "LockSegment: %04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    GET_ARENA_PTR(handle)->lockCount++;
    return handle;
}


/***********************************************************************
 *           UnlockSegment16   (KERNEL.24)
 */
void UnlockSegment16( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "UnlockSegment: %04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    GET_ARENA_PTR(handle)->lockCount--;
    /* FIXME: this ought to return the lock count in CX (go figure...) */
}


/***********************************************************************
 *           GlobalCompact16   (KERNEL.25)
 */
DWORD GlobalCompact16( DWORD desired )
{
    return GLOBAL_MAX_ALLOC_SIZE;
}


/***********************************************************************
 *           GlobalFreeAll   (KERNEL.26)
 */
void GlobalFreeAll( HGLOBAL16 owner )
{
    DWORD i;
    GLOBALARENA *pArena;

    pArena = pGlobalArena;
    for (i = 0; i < globalArenaSize; i++, pArena++)
    {
        if ((pArena->size != 0) && (pArena->hOwner == owner))
            GlobalFree16( pArena->handle );
    }
}


/***********************************************************************
 *           GlobalWire   (KERNEL.111)
 */
SEGPTR GlobalWire( HGLOBAL16 handle )
{
    return WIN16_GlobalLock16( handle );
}


/***********************************************************************
 *           GlobalUnWire   (KERNEL.112)
 */
BOOL16 GlobalUnWire( HGLOBAL16 handle )
{
    return GlobalUnlock16( handle );
}


/***********************************************************************
 *           GlobalDOSAlloc   (KERNEL.184)
 */
DWORD GlobalDOSAlloc( DWORD size )
{
    WORD sel = GlobalAlloc16( GMEM_FIXED, size );
    if (!sel) return 0;
    return MAKELONG( sel, sel /* this one ought to be a real-mode segment */ );
}


/***********************************************************************
 *           GlobalDOSFree   (KERNEL.185)
 */
WORD GlobalDOSFree( WORD sel )
{
    return GlobalFree16( GlobalHandle16(sel) ) ? sel : 0;
}


/***********************************************************************
 *           SetSwapAreaSize   (KERNEL.106)
 */
LONG SetSwapAreaSize( WORD size )
{
    dprintf_global(stdnimp, "STUB: SetSwapAreaSize(%d)\n", size );
    return MAKELONG( size, 0xffff );
}


/***********************************************************************
 *           GlobalLRUOldest   (KERNEL.163)
 */
HGLOBAL16 GlobalLRUOldest( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalLRUOldest: %04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GlobalLRUNewest   (KERNEL.164)
 */
HGLOBAL16 GlobalLRUNewest( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalLRUNewest: %04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GetFreeSpace   (KERNEL.169)
 */
DWORD GetFreeSpace( UINT16 wFlags )
{
    return GLOBAL_MAX_ALLOC_SIZE;
}


/***********************************************************************
 *           GlobalPageLock   (KERNEL.191)
 */
WORD GlobalPageLock( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalPageLock: %04x\n", handle );
    return ++(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalPageUnlock   (KERNEL.192)
 */
WORD GlobalPageUnlock( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalPageUnlock: %04x\n", handle );
    return --(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalFix   (KERNEL.197)
 */
void GlobalFix( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalFix: %04x\n", handle );
    GET_ARENA_PTR(handle)->lockCount++;
}


/***********************************************************************
 *           GlobalUnfix   (KERNEL.198)
 */
void GlobalUnfix( HGLOBAL16 handle )
{
    dprintf_global( stddeb, "GlobalUnfix: %04x\n", handle );
    GET_ARENA_PTR(handle)->lockCount--;
}


/***********************************************************************
 *           FarSetOwner   (KERNEL.403)
 */
void FarSetOwner( HANDLE16 handle, HANDLE16 hOwner )
{
    GET_ARENA_PTR(handle)->hOwner = hOwner;
}


/***********************************************************************
 *           FarGetOwner   (KERNEL.404)
 */
HANDLE16 FarGetOwner( HANDLE16 handle )
{
    return GET_ARENA_PTR(handle)->hOwner;
}


/***********************************************************************
 *           GlobalHandleToSel   (TOOLHELP.50)
 */
WORD GlobalHandleToSel( HGLOBAL16 handle )
{
    dprintf_toolhelp( stddeb, "GlobalHandleToSel: %04x\n", handle );
    if (!handle) return 0;
#ifdef CONFIG_IPC
    if (is_dde_handle(handle)) return DDE_GlobalHandleToSel(handle);
#endif
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
BOOL16 GlobalFirst( GLOBALENTRY *pGlobal, WORD wFlags )
{
    if (wFlags == GLOBAL_LRU) return FALSE;
    pGlobal->dwNext = 0;
    return GlobalNext( pGlobal, wFlags );
}


/***********************************************************************
 *           GlobalNext   (TOOLHELP.52)
 */
BOOL16 GlobalNext( GLOBALENTRY *pGlobal, WORD wFlags)
{
    GLOBALARENA *pArena;

    if (pGlobal->dwNext >= globalArenaSize) return FALSE;
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
BOOL16 GlobalInfo( GLOBALINFO *pInfo )
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
BOOL16 GlobalEntryHandle( GLOBALENTRY *pGlobal, HGLOBAL16 hItem )
{
    return FALSE;
}


/***********************************************************************
 *           GlobalEntryModule   (TOOLHELP.55)
 */
BOOL16 GlobalEntryModule( GLOBALENTRY *pGlobal, HMODULE16 hModule, WORD wSeg )
{
    return FALSE;
}


/***********************************************************************
 *           MemManInfo   (TOOLHELP.72)
 */
BOOL16 MemManInfo( MEMMANINFO *pInfo )
{
#ifdef linux
    /* FIXME: does not take into account the dwSize member
     * could be corrupting memory therefore
     */
    /* shamefully stolen from free */
    DWORD availmem = 0;
    DWORD totalmem = 0;
    FILE *meminfo;
    char buf[80];
    int col[5];
    int n;

    if ((meminfo = fopen("/proc/meminfo", "r")) < 0) {
        perror("wine: open");
        return FALSE;
    }

    fgets(buf, 80, meminfo); /* read first line */
    while ( fgets(buf, 80, meminfo) ) {
        n = sscanf( buf, "%*s %d %d %d %d %d", &col[0], &col[1], &col[2], &col[3], &col[4]);
        if ( n < 1 ) continue; /* escape the loop at the top */
        totalmem += col[0];
        availmem += col[2] + col[4];
    }

    fprintf(stderr,"MemManInfo called with dwSize = %ld\n",pInfo->dwSize);
    if (pInfo->dwSize) {
        pInfo->wPageSize = getpagesize();
        pInfo->dwLargestFreeBlock = availmem;
        pInfo->dwTotalLinearSpace = totalmem / pInfo->wPageSize;
        pInfo->dwMaxPagesAvailable = pInfo->dwLargestFreeBlock / pInfo->wPageSize;
        pInfo->dwMaxPagesLockable = pInfo->dwMaxPagesLockable;
        /* FIXME: the next three are not quite correct */
        pInfo->dwTotalUnlockedPages = pInfo->dwMaxPagesAvailable;
        pInfo->dwFreePages = pInfo->dwMaxPagesAvailable;
        pInfo->dwTotalPages = pInfo->dwMaxPagesAvailable;
        /* FIXME: the three above are not quite correct */
        pInfo->dwFreeLinearSpace = pInfo->dwMaxPagesAvailable;
        pInfo->dwSwapFilePages = 0L;
    }
    return TRUE;
#else
    return TRUE;
#endif
}


/***********************************************************************
 *           GlobalAlloc32   (KERNEL32.315)
 */
HGLOBAL32 GlobalAlloc32( UINT32 flags, DWORD size )
{
    DWORD heapFlags = 0;

    if (flags & GMEM_MOVEABLE)
        fprintf( stderr, "GlobalAlloc32: unimplemented flag GMEM_MOVEABLE\n" );

    if (flags & GMEM_ZEROINIT) heapFlags |= HEAP_ZERO_MEMORY;
    return (HGLOBAL32)HeapAlloc( GetProcessHeap(), heapFlags, size );
}


/***********************************************************************
 *           GlobalCompact32   (KERNEL32.316)
 */
DWORD GlobalCompact32( DWORD minfree )
{
    return 0;  /* GlobalCompact does nothing in Win32 */
}


/***********************************************************************
 *           GlobalFlags32   (KERNEL32.321)
 */
UINT32 GlobalFlags32( HGLOBAL32 handle )
{
    return 0;
}


/***********************************************************************
 *           GlobalFree32   (KERNEL32.322)
 */
HGLOBAL32 GlobalFree32( HGLOBAL32 handle )
{
    return HeapFree( GetProcessHeap(), 0, (LPVOID)handle ) ? 0 : handle;
}


/***********************************************************************
 *           GlobalHandle32   (KERNEL32.325)
 */
HGLOBAL32 GlobalHandle32( LPCVOID ptr )
{
    return (HGLOBAL32)ptr;
}


/***********************************************************************
 *           GlobalLock32   (KERNEL32.326)
 */
LPVOID GlobalLock32( HGLOBAL32 handle )
{
    return (LPVOID)handle;
}


/***********************************************************************
 *           GlobalReAlloc32   (KERNEL32.328)
 */
HGLOBAL32 GlobalReAlloc32( HGLOBAL32 handle, DWORD size, UINT32 flags )
{
    if (flags & GMEM_MODIFY)
    {
        fprintf( stderr, "GlobalReAlloc32: GMEM_MODIFY not supported\n" );
        return 0;
    }

    return (HGLOBAL32)HeapReAlloc( GetProcessHeap(), 0, (LPVOID)handle, size );
}


/***********************************************************************
 *           GlobalSize32   (KERNEL32.329)
 */
DWORD GlobalSize32( HGLOBAL32 handle )
{
    return HeapSize( GetProcessHeap(), 0, (LPVOID)handle );
}


/***********************************************************************
 *           GlobalUnlock32   (KERNEL32.332)
 */
BOOL32 GlobalUnlock32( HGLOBAL32 handle )
{
    return TRUE;
}
