/*
 * Global heap functions
 *
 * Copyright 1995 Alexandre Julliard
 */
/* 0xffff sometimes seems to mean: CURRENT_DS */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "wine/winbase16.h"
#include "global.h"
#include "heap.h"
#include "toolhelp.h"
#include "selectors.h"
#include "miscemu.h"
#include "stackframe.h"
#include "module.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(global)

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

#define VALID_HANDLE(handle) (((handle)>>__AHSHIFT)<globalArenaSize)
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

void debug_handles(void)
{
    int printed=0;
    int i;
    for (i = globalArenaSize-1 ; i>=0 ; i--) {
	if (pGlobalArena[i].size!=0 && (pGlobalArena[i].handle & 0x8000)){
	    printed=1;
	    DPRINTF("0x%08x, ",pGlobalArena[i].handle);
	}
    }
    if (printed)
	DPRINTF("\n");
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
        SELECTOR_FreeBlock( sel, selcount );
        return 0;
    }

      /* Fill the arena block */

    pArena->base = (DWORD)ptr;
    pArena->size = GET_SEL_LIMIT(sel) + 1;
    pArena->handle = (flags & GMEM_MOVEABLE) ? sel - 1 : sel;
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
    GLOBALARENA *pArena;

    if (!handle) return TRUE;
    sel = GlobalHandleToSel16( handle ); 
    if (!VALID_HANDLE(sel))
    	return FALSE;
    pArena = GET_ARENA_PTR(sel);
    SELECTOR_FreeBlock( sel, (pArena->size + 0xffff) / 0x10000 );
    memset( pArena, 0, sizeof(GLOBALARENA) );
    return TRUE;
}

/***********************************************************************
 *           GLOBAL_MoveBlock
 */
BOOL16 GLOBAL_MoveBlock( HGLOBAL16 handle, const void *ptr, DWORD size )
{
    WORD sel;
    GLOBALARENA *pArena;

    if (!handle) return TRUE;
    sel = GlobalHandleToSel16( handle ); 
    if (!VALID_HANDLE(sel))
    	return FALSE;
    pArena = GET_ARENA_PTR(sel);
    if (pArena->selCount != 1)
        return FALSE;

    pArena->base = (DWORD)ptr;
    pArena->size = size;

    SELECTOR_MoveBlock( sel, ptr );
    SetSelectorLimit16( sel, size-1 );

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

    TRACE("%ld flags=%04x\n", size, flags );

    /* If size is 0, create a discarded block */

    if (size == 0) return GLOBAL_CreateBlock( flags, NULL, 1, hOwner, isCode,
                                              is32Bit, isReadOnly, NULL );

    /* Fixup the size */

    if (size >= GLOBAL_MAX_ALLOC_SIZE - 0x1f) return 0;
    size = (size + 0x1f) & ~0x1f;

    /* Allocate the linear memory */
    ptr = HeapAlloc( SystemHeap, 0, size );
      /* FIXME: free discardable blocks and try again? */
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

/***********************************************************************
 *           GlobalAlloc16   (KERNEL.15)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HGLOBAL16 WINAPI GlobalAlloc16(
                 UINT16 flags, /* [in] Object allocation attributes */
                 DWORD size    /* [in] Number of bytes to allocate */
) {
    HANDLE16 owner = GetCurrentPDB16();

    if (flags & GMEM_DDESHARE)
        owner = GetExePtr(owner);  /* Make it a module handle */
    return GLOBAL_Alloc( flags, size, owner, FALSE, FALSE, FALSE );
}


/***********************************************************************
 *           GlobalReAlloc16   (KERNEL.16)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HGLOBAL16 WINAPI GlobalReAlloc16(
                 HGLOBAL16 handle, /* [in] Handle of global memory object */
                 DWORD size,       /* [in] New size of block */
                 UINT16 flags      /* [in] How to reallocate object */
) {
    WORD selcount;
    DWORD oldsize;
    void *ptr;
    GLOBALARENA *pArena, *pNewArena;
    WORD sel = GlobalHandleToSel16( handle );

    TRACE("%04x %ld flags=%04x\n",
                    handle, size, flags );
    if (!handle) return 0;
    
    if (!VALID_HANDLE(handle)) {
    	WARN("Invalid handle 0x%04x!\n", handle);
    	return 0;
    }
    pArena = GET_ARENA_PTR( handle );

      /* Discard the block if requested */

    if ((size == 0) && (flags & GMEM_MOVEABLE) && !(flags & GMEM_MODIFY))
    {
        if (!(pArena->flags & GA_MOVEABLE) ||
            !(pArena->flags & GA_DISCARDABLE) ||
            (pArena->lockCount > 0) || (pArena->pageLockCount > 0)) return 0;
        HeapFree( SystemHeap, 0, (void *)pArena->base );
        pArena->base = 0;

        /* Note: we rely on the fact that SELECTOR_ReallocBlock won't 
         * change the selector if we are shrinking the block.
	 * FIXME: shouldn't we keep selectors until the block is deleted?
	 */
        SELECTOR_ReallocBlock( sel, 0, 1 );
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
    TRACE("oldsize %08lx\n",oldsize);
    if (ptr && (size == oldsize)) return handle;  /* Nothing to do */

    if (((char *)ptr >= DOSMEM_MemoryBase(0)) &&
        ((char *)ptr <= DOSMEM_MemoryBase(0) + 0x100000))
        ptr = DOSMEM_ResizeBlock(0, ptr, size, NULL);
    else
        ptr = HeapReAlloc( SystemHeap, 0, ptr, size );
    if (!ptr)
    {
        SELECTOR_FreeBlock( sel, (oldsize + 0xffff) / 0x10000 );
        memset( pArena, 0, sizeof(GLOBALARENA) );
        return 0;
    }

      /* Reallocate the selector(s) */

    sel = SELECTOR_ReallocBlock( sel, ptr, size );
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
        SELECTOR_FreeBlock( sel, selcount );
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
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 */
HGLOBAL16 WINAPI GlobalFree16(
                 HGLOBAL16 handle /* [in] Handle of global memory object */
) {
    void *ptr;

    if (!VALID_HANDLE(handle)) {
    	WARN("Invalid handle 0x%04x passed to GlobalFree16!\n",handle);
    	return 0;
    }
    ptr = (void *)GET_ARENA_PTR(handle)->base;

    TRACE("%04x\n", handle );
    if (!GLOBAL_FreeBlock( handle )) return handle;  /* failed */
    if (ptr) HeapFree( SystemHeap, 0, ptr );
    return 0;
}


/***********************************************************************
 *           WIN16_GlobalLock16   (KERNEL.18)
 *
 * This is the GlobalLock16() function used by 16-bit code.
 */
SEGPTR WINAPI WIN16_GlobalLock16( HGLOBAL16 handle )
{
    WORD sel = GlobalHandleToSel16( handle );
    TRACE("(%04x) -> %08lx\n", handle, MAKELONG( 0, sel ) );

    if (handle)
    {
	if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;

	if (!VALID_HANDLE(handle)) {
	    WARN("Invalid handle 0x%04x passed to WIN16_GlobalLock16!\n",handle);
	    sel = 0;
	}
	if (!GET_ARENA_PTR(handle)->base) 
            sel = 0;
        else
            GET_ARENA_PTR(handle)->lockCount++;
    }

    CURRENT_STACK16->ecx = sel;  /* selector must be returned in CX as well */
    return PTR_SEG_OFF_TO_SEGPTR( sel, 0 );
}


/***********************************************************************
 *           GlobalLock16   (KERNEL.18)
 *
 * This is the GlobalLock16() function used by 32-bit code.
 * 
 * RETURNS
 *	Pointer to first byte of memory block
 *	NULL: Failure
 */
LPVOID WINAPI GlobalLock16(
              HGLOBAL16 handle /* [in] Handle of global memory object */
) {
    if (!handle) return 0;
    if (!VALID_HANDLE(handle))
	return (LPVOID)0;
    GET_ARENA_PTR(handle)->lockCount++;
    return (LPVOID)GET_ARENA_PTR(handle)->base;
}


/***********************************************************************
 *           GlobalUnlock16   (KERNEL.19)
 * NOTES
 *	Should the return values be cast to booleans?
 *
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 */
BOOL16 WINAPI GlobalUnlock16(
              HGLOBAL16 handle /* [in] Handle of global memory object */
) {
    GLOBALARENA *pArena = GET_ARENA_PTR(handle);
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalUnlock16!\n",handle);
	return 0;
    }
    TRACE("%04x\n", handle );
    if (pArena->lockCount) pArena->lockCount--;
    return pArena->lockCount;
}

/***********************************************************************
 *     GlobalChangeLockCount               (KERNEL.365)
 *
 * This is declared as a register function as it has to preserve
 * *all* registers, even AX/DX !
 *
 */
void WINAPI GlobalChangeLockCount16( HGLOBAL16 handle, INT16 delta,
                                     CONTEXT86 *context )
{
    if ( delta == 1 )
        GlobalLock16( handle );
    else if ( delta == -1 )
        GlobalUnlock16( handle );
    else
        ERR("(%04X, %d): strange delta value\n", handle, delta );
}

/***********************************************************************
 *           GlobalSize16   (KERNEL.20)
 * RETURNS
 *	Size in bytes of object
 *	0: Failure
 */
DWORD WINAPI GlobalSize16(
             HGLOBAL16 handle /* [in] Handle of global memory object */
) {
    TRACE("%04x\n", handle );
    if (!handle) return 0;
    if (!VALID_HANDLE(handle))
	return 0;
    return GET_ARENA_PTR(handle)->size;
}


/***********************************************************************
 *           GlobalHandle16   (KERNEL.21)
 * NOTES
 *	Why is GlobalHandleToSel used here with the sel as input?
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
DWORD WINAPI GlobalHandle16(
             WORD sel /* [in] Address of global memory block */
) {
    TRACE("%04x\n", sel );
    if (!VALID_HANDLE(sel)) {
	WARN("Invalid handle 0x%04x passed to GlobalHandle16!\n",sel);
	return 0;
    }
    return MAKELONG( GET_ARENA_PTR(sel)->handle, GlobalHandleToSel16(sel) );
}

/***********************************************************************
 *           GlobalHandleNoRIP   (KERNEL.159)
 */
DWORD WINAPI GlobalHandleNoRIP16( WORD sel )
{
    int i;
    for (i = globalArenaSize-1 ; i>=0 ; i--) {
        if (pGlobalArena[i].size!=0 && pGlobalArena[i].handle == sel)
		return MAKELONG( GET_ARENA_PTR(sel)->handle, GlobalHandleToSel16(sel) );
    }
    return 0;
}


/***********************************************************************
 *           GlobalFlags16   (KERNEL.22)
 * NOTES
 *	Should this return GMEM_INVALID_HANDLE instead of 0 on invalid
 *	handle?
 *
 * RETURNS
 *	Value specifying flags and lock count
 *	GMEM_INVALID_HANDLE: Invalid handle
 */
UINT16 WINAPI GlobalFlags16(
              HGLOBAL16 handle /* [in] Handle of global memory object */
) {
    GLOBALARENA *pArena;

    TRACE("%04x\n", handle );
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalFlags16!\n",handle);
	return 0;
    }
    pArena = GET_ARENA_PTR(handle);
    return pArena->lockCount |
           ((pArena->flags & GA_DISCARDABLE) ? GMEM_DISCARDABLE : 0) |
           ((pArena->base == 0) ? GMEM_DISCARDED : 0);
}


/***********************************************************************
 *           LockSegment16   (KERNEL.23)
 */
HGLOBAL16 WINAPI LockSegment16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to LockSegment16!\n",handle);
	return 0;
    }
    GET_ARENA_PTR(handle)->lockCount++;
    return handle;
}


/***********************************************************************
 *           UnlockSegment16   (KERNEL.24)
 */
void WINAPI UnlockSegment16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to UnlockSegment16!\n",handle);
	return;
    }
    GET_ARENA_PTR(handle)->lockCount--;
    /* FIXME: this ought to return the lock count in CX (go figure...) */
}


/***********************************************************************
 *           GlobalCompact16   (KERNEL.25)
 */
DWORD WINAPI GlobalCompact16( DWORD desired )
{
    return GLOBAL_MAX_ALLOC_SIZE;
}


/***********************************************************************
 *           GlobalFreeAll   (KERNEL.26)
 */
void WINAPI GlobalFreeAll16( HGLOBAL16 owner )
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
 *           GlobalWire16   (KERNEL.111)
 */
SEGPTR WINAPI GlobalWire16( HGLOBAL16 handle )
{
    return WIN16_GlobalLock16( handle );
}


/***********************************************************************
 *           GlobalUnWire16   (KERNEL.112)
 */
BOOL16 WINAPI GlobalUnWire16( HGLOBAL16 handle )
{
    return !GlobalUnlock16( handle );
}


/***********************************************************************
 *           SetSwapAreaSize16   (KERNEL.106)
 */
LONG WINAPI SetSwapAreaSize16( WORD size )
{
    FIXME("(%d) - stub!\n", size );
    return MAKELONG( size, 0xffff );
}


/***********************************************************************
 *           GlobalLRUOldest   (KERNEL.163)
 */
HGLOBAL16 WINAPI GlobalLRUOldest16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GlobalLRUNewest   (KERNEL.164)
 */
HGLOBAL16 WINAPI GlobalLRUNewest16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (handle == (HGLOBAL16)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           GetFreeSpace16   (KERNEL.169)
 */
DWORD WINAPI GetFreeSpace16( UINT16 wFlags )
{
    MEMORYSTATUS ms;
    GlobalMemoryStatus( &ms );
    return ms.dwAvailVirtual;
}

/***********************************************************************
 *           GlobalDOSAlloc   (KERNEL.184)
 * RETURNS
 *	Address (HW=Paragraph segment; LW=Selector)
 */
DWORD WINAPI GlobalDOSAlloc16(
             DWORD size /* [in] Number of bytes to be allocated */
) {
   UINT16    uParagraph;
   LPVOID    lpBlock = DOSMEM_GetBlock( 0, size, &uParagraph );

   if( lpBlock )
   {
       HMODULE16 hModule = GetModuleHandle16("KERNEL");
       WORD	 wSelector;
   
       wSelector = GLOBAL_CreateBlock(GMEM_FIXED, lpBlock, size, 
				      hModule, 0, 0, 0, NULL );
       return MAKELONG(wSelector,uParagraph);
   }
   return 0;
}


/***********************************************************************
 *           GlobalDOSFree      (KERNEL.185)
 * RETURNS
 *	NULL: Success
 *	sel: Failure
 */
WORD WINAPI GlobalDOSFree16(
            WORD sel /* [in] Selector */
) {
   DWORD   block = GetSelectorBase(sel);

   if( block && block < 0x100000 ) 
   {
       LPVOID lpBlock = DOSMEM_MapDosToLinear( block );
       if( DOSMEM_FreeBlock( 0, lpBlock ) )
	   GLOBAL_FreeBlock( sel );
       sel = 0;
   }
   return sel;
}


/***********************************************************************
 *           GlobalPageLock   (KERNEL.191)
 */
WORD WINAPI GlobalPageLock16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalPageLock!\n",handle);
	return 0;
    }
    return ++(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalPageUnlock   (KERNEL.192)
 */
WORD WINAPI GlobalPageUnlock16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalPageUnlock!\n",handle);
	return 0;
    }
    return --(GET_ARENA_PTR(handle)->pageLockCount);
}


/***********************************************************************
 *           GlobalFix16   (KERNEL.197)
 */
WORD WINAPI GlobalFix16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalFix16!\n",handle);
	return 0;
    }
    GET_ARENA_PTR(handle)->lockCount++;

    return GlobalHandleToSel16(handle);
}


/***********************************************************************
 *           GlobalUnfix16   (KERNEL.198)
 */
void WINAPI GlobalUnfix16( HGLOBAL16 handle )
{
    TRACE("%04x\n", handle );
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalUnfix16!\n",handle);
	return;
    }
    GET_ARENA_PTR(handle)->lockCount--;
}


/***********************************************************************
 *           FarSetOwner   (KERNEL.403)
 */
void WINAPI FarSetOwner16( HGLOBAL16 handle, HANDLE16 hOwner )
{
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to FarSetOwner!\n",handle);
	return;
    }
    GET_ARENA_PTR(handle)->hOwner = hOwner;
}


/***********************************************************************
 *           FarGetOwner   (KERNEL.404)
 */
HANDLE16 WINAPI FarGetOwner16( HGLOBAL16 handle )
{
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to FarGetOwner!\n",handle);
	return 0;
    }
    return GET_ARENA_PTR(handle)->hOwner;
}


/***********************************************************************
 *           GlobalHandleToSel   (TOOLHELP.50)
 */
WORD WINAPI GlobalHandleToSel16( HGLOBAL16 handle )
{
    if (!handle) return 0;
    if (!VALID_HANDLE(handle)) {
	WARN("Invalid handle 0x%04x passed to GlobalHandleToSel!\n",handle);
	return 0;
    }
    if (!(handle & 7))
    {
        WARN("Program attempted invalid selector conversion\n" );
        return handle - 1;
    }
    return handle | 7;
}


/***********************************************************************
 *           GlobalFirst   (TOOLHELP.51)
 */
BOOL16 WINAPI GlobalFirst16( GLOBALENTRY *pGlobal, WORD wFlags )
{
    if (wFlags == GLOBAL_LRU) return FALSE;
    pGlobal->dwNext = 0;
    return GlobalNext16( pGlobal, wFlags );
}


/***********************************************************************
 *           GlobalNext   (TOOLHELP.52)
 */
BOOL16 WINAPI GlobalNext16( GLOBALENTRY *pGlobal, WORD wFlags)
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
    pGlobal->wFlags       = (GetCurrentPDB16() == pArena->hOwner);
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
BOOL16 WINAPI GlobalInfo16( GLOBALINFO *pInfo )
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
BOOL16 WINAPI GlobalEntryHandle16( GLOBALENTRY *pGlobal, HGLOBAL16 hItem )
{
    GLOBALARENA *pArena = GET_ARENA_PTR(hItem);

    pGlobal->dwAddress    = pArena->base;
    pGlobal->dwBlockSize  = pArena->size;
    pGlobal->hBlock       = pArena->handle;
    pGlobal->wcLock       = pArena->lockCount;
    pGlobal->wcPageLock   = pArena->pageLockCount;
    pGlobal->wFlags       = (GetCurrentPDB16() == pArena->hOwner);
    pGlobal->wHeapPresent = FALSE;
    pGlobal->hOwner       = pArena->hOwner;
    pGlobal->wType        = GT_UNKNOWN;
    pGlobal->wData        = 0;
    pGlobal->dwNext++;
    return TRUE;
}


/***********************************************************************
 *           GlobalEntryModule   (TOOLHELP.55)
 */
BOOL16 WINAPI GlobalEntryModule16( GLOBALENTRY *pGlobal, HMODULE16 hModule,
                                 WORD wSeg )
{
    return FALSE;
}


/***********************************************************************
 *           MemManInfo   (TOOLHELP.72)
 */
BOOL16 WINAPI MemManInfo16( MEMMANINFO *info )
{
    MEMORYSTATUS status;

    /*
     * Not unsurprisingly although the documention says you 
     * _must_ provide the size in the dwSize field, this function
     * (under Windows) always fills the structure and returns true.
     */
    GlobalMemoryStatus( &status );
    info->wPageSize            = VIRTUAL_GetPageSize();
    info->dwLargestFreeBlock   = status.dwAvailVirtual;
    info->dwMaxPagesAvailable  = info->dwLargestFreeBlock / info->wPageSize;
    info->dwMaxPagesLockable   = info->dwMaxPagesAvailable;
    info->dwTotalLinearSpace   = status.dwTotalVirtual / info->wPageSize;
    info->dwTotalUnlockedPages = info->dwTotalLinearSpace;
    info->dwFreePages          = info->dwMaxPagesAvailable;
    info->dwTotalPages         = info->dwTotalLinearSpace;
    info->dwFreeLinearSpace    = info->dwMaxPagesAvailable;
    info->dwSwapFilePages      = status.dwTotalPageFile / info->wPageSize;
    return TRUE;
}

/***********************************************************************
 *           GetFreeMemInfo   (KERNEL.316)
 */
DWORD WINAPI GetFreeMemInfo16(void)
{
    MEMMANINFO info;
    MemManInfo16( &info );
    return MAKELONG( info.dwTotalLinearSpace, info.dwMaxPagesAvailable );
}

/*
 * Win32 Global heap functions (GlobalXXX).
 * These functions included in Win32 for compatibility with 16 bit Windows
 * Especially the moveable blocks and handles are oldish. 
 * But the ability to directly allocate memory with GPTR and LPTR is widely
 * used.
 *
 * The handle stuff looks horrible, but it's implemented almost like Win95
 * does it. 
 *
 */

#define MAGIC_GLOBAL_USED 0x5342
#define GLOBAL_LOCK_MAX   0xFF
#define HANDLE_TO_INTERN(h)  ((PGLOBAL32_INTERN)(((char *)(h))-2))
#define INTERN_TO_HANDLE(i)  ((HGLOBAL) &((i)->Pointer))
#define POINTER_TO_HANDLE(p) (*(((HGLOBAL *)(p))-1))
#define ISHANDLE(h)          (((DWORD)(h)&2)!=0)
#define ISPOINTER(h)         (((DWORD)(h)&2)==0)

typedef struct __GLOBAL32_INTERN
{
   WORD         Magic;
   LPVOID       Pointer WINE_PACKED;
   BYTE         Flags;
   BYTE         LockCount;
} GLOBAL32_INTERN, *PGLOBAL32_INTERN;

/***********************************************************************
 *           GLOBAL_GetHeap
 *
 * Returns the appropriate heap to be used. If the object was created
 * With GMEM_DDESHARE we allocated it on the system heap.
 */
static HANDLE GLOBAL_GetHeap( HGLOBAL hmem )
{
   HANDLE heap;
    
   TRACE("() hmem=%x\n", hmem);
   
   /* Get the appropriate heap to be used for this object */
   if (ISPOINTER(hmem))
      heap = GetProcessHeap();
   else
   {
      PGLOBAL32_INTERN pintern;
      pintern=HANDLE_TO_INTERN(hmem);
      
      /* If it was DDESHARE it was created on the shared system heap */
      pintern=HANDLE_TO_INTERN(hmem);
      heap = ( pintern->Flags & (GMEM_DDESHARE >> 8) )
           ? SystemHeap : GetProcessHeap();
   }

   return heap;
}

/***********************************************************************
 *           GlobalAlloc32   (KERNEL32.315)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HGLOBAL WINAPI GlobalAlloc(
                 UINT flags, /* [in] Object allocation attributes */
                 DWORD size    /* [in] Number of bytes to allocate */
) {
   PGLOBAL32_INTERN     pintern;
   DWORD		hpflags;
   LPVOID               palloc;

   if(flags&GMEM_ZEROINIT)
      hpflags=HEAP_ZERO_MEMORY;
   else
      hpflags=0;
   
   TRACE("() flags=%04x\n",  flags );
   
   if((flags & GMEM_MOVEABLE)==0) /* POINTER */
   {
      palloc=HeapAlloc(GetProcessHeap(), hpflags, size);
      return (HGLOBAL) palloc;
   }
   else  /* HANDLE */
   {
      HANDLE heap;
       
      /* If DDESHARE is set, create on the shared system heap */
      heap = (flags & GMEM_DDESHARE) ? SystemHeap : GetProcessHeap();

      /* HeapLock(heap); */

      pintern=HeapAlloc(heap, 0,  sizeof(GLOBAL32_INTERN));
      if(size)
      {
	 palloc=HeapAlloc(heap, hpflags, size+sizeof(HGLOBAL));
	 *(HGLOBAL *)palloc=INTERN_TO_HANDLE(pintern);
	 pintern->Pointer=(char *) palloc+sizeof(HGLOBAL);
      }
      else
	 pintern->Pointer=NULL;
      pintern->Magic=MAGIC_GLOBAL_USED;
      pintern->Flags=flags>>8;
      pintern->LockCount=0;
      
      /* HeapUnlock(heap); */
       
      return INTERN_TO_HANDLE(pintern);
   }
}


/***********************************************************************
 *           GlobalLock32   (KERNEL32.326)
 * RETURNS
 *	Pointer to first byte of block
 *	NULL: Failure
 */
LPVOID WINAPI GlobalLock(
              HGLOBAL hmem /* [in] Handle of global memory object */
) {
   PGLOBAL32_INTERN pintern;
   LPVOID           palloc;

   if(ISPOINTER(hmem))
      return (LPVOID) hmem;

   /* HeapLock(GetProcessHeap()); */
   
   pintern=HANDLE_TO_INTERN(hmem);
   if(pintern->Magic==MAGIC_GLOBAL_USED)
   {
      if(pintern->LockCount<GLOBAL_LOCK_MAX)
	 pintern->LockCount++;
      palloc=pintern->Pointer;
   }
   else
   {
      WARN("invalid handle\n");
      palloc=(LPVOID) NULL;
   }
   /* HeapUnlock(GetProcessHeap()); */;
   return palloc;
}


/***********************************************************************
 *           GlobalUnlock32   (KERNEL32.332)
 * RETURNS
 *	TRUE: Object is still locked
 *	FALSE: Object is unlocked
 */
BOOL WINAPI GlobalUnlock(
              HGLOBAL hmem /* [in] Handle of global memory object */
) {
   PGLOBAL32_INTERN       pintern;
   BOOL                 locked;

   if(ISPOINTER(hmem))
      return FALSE;

   /* HeapLock(GetProcessHeap()); */
   pintern=HANDLE_TO_INTERN(hmem);
   
   if(pintern->Magic==MAGIC_GLOBAL_USED)
   {
      if((pintern->LockCount<GLOBAL_LOCK_MAX)&&(pintern->LockCount>0))
	 pintern->LockCount--;

      locked=(pintern->LockCount==0) ? FALSE : TRUE;
   }
   else
   {
      WARN("invalid handle\n");
      locked=FALSE;
   }
   /* HeapUnlock(GetProcessHeap()); */
   return locked;
}


/***********************************************************************
 *           GlobalHandle32   (KERNEL32.325)
 * Returns the handle associated with the specified pointer.
 *
 * NOTES
 *	Since there in only one goto, can it be removed and the return
 *	be put 'inline'?
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HGLOBAL WINAPI GlobalHandle(
                 LPCVOID pmem /* [in] Pointer to global memory block */
) {
    HGLOBAL handle;
    HANDLE heap = GLOBAL_GetHeap( POINTER_TO_HANDLE(pmem) );

    if (!HEAP_IsInsideHeap( heap, 0, pmem )) goto error;
    handle = POINTER_TO_HANDLE(pmem);
    if (HEAP_IsInsideHeap( heap, 0, (LPCVOID)handle ))
    {
        if (HANDLE_TO_INTERN(handle)->Magic == MAGIC_GLOBAL_USED)
            return handle;  /* valid moveable block */
    }
    /* maybe FIXED block */
    if (HeapValidate( heap, 0, pmem ))
        return (HGLOBAL)pmem;  /* valid fixed block */

error:
    SetLastError( ERROR_INVALID_HANDLE );
    return 0;
}


/***********************************************************************
 *           GlobalReAlloc32   (KERNEL32.328)
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 */
HGLOBAL WINAPI GlobalReAlloc(
                 HGLOBAL hmem, /* [in] Handle of global memory object */
                 DWORD size,     /* [in] New size of block */
                 UINT flags    /* [in] How to reallocate object */
) {
   LPVOID               palloc;
   HGLOBAL            hnew;
   PGLOBAL32_INTERN     pintern;
   HANDLE heap = GLOBAL_GetHeap( hmem );

   hnew = 0;
   /* HeapLock(heap); */
   if(flags & GMEM_MODIFY) /* modify flags */
   {
      if( ISPOINTER(hmem) && (flags & GMEM_MOVEABLE))
      {
	 /* make a fixed block moveable
	  * actually only NT is able to do this. But it's soo simple
	  */
         if (hmem == 0)
         {
	     ERR("GlobalReAlloc32 with null handle!\n");
             SetLastError( ERROR_NOACCESS );
    	     return 0;
         }
	 size=HeapSize(heap, 0, (LPVOID) hmem);
	 hnew=GlobalAlloc( flags, size);
	 palloc=GlobalLock(hnew);
	 memcpy(palloc, (LPVOID) hmem, size);
	 GlobalUnlock(hnew);
	 GlobalFree(hmem);
      }
      else if( ISPOINTER(hmem) &&(flags & GMEM_DISCARDABLE))
      {
	 /* change the flags to make our block "discardable" */
	 pintern=HANDLE_TO_INTERN(hmem);
	 pintern->Flags = pintern->Flags | (GMEM_DISCARDABLE >> 8);
	 hnew=hmem;
      }
      else
      {
	 SetLastError(ERROR_INVALID_PARAMETER);
	 hnew = 0;
      }
   }
   else
   {
      if(ISPOINTER(hmem))
      {
	 /* reallocate fixed memory */
	 hnew=(HGLOBAL)HeapReAlloc(heap, 0, (LPVOID) hmem, size);
      }
      else
      {
	 /* reallocate a moveable block */
	 pintern=HANDLE_TO_INTERN(hmem);
	 if(pintern->LockCount>1) {
	    ERR("handle 0x%08lx is still locked, cannot realloc!\n",(DWORD)hmem);
	    SetLastError(ERROR_INVALID_HANDLE);
	 } else if(size!=0)
	 {
	    hnew=hmem;
	    if(pintern->Pointer)
	    {
	       palloc=HeapReAlloc(heap, 0,
				  (char *) pintern->Pointer-sizeof(HGLOBAL),
				  size+sizeof(HGLOBAL) );
	       pintern->Pointer=(char *) palloc+sizeof(HGLOBAL);
	    }
	    else
	    {
	       palloc=HeapAlloc(heap, 0, size+sizeof(HGLOBAL));
	       *(HGLOBAL *)palloc=hmem;
	       pintern->Pointer=(char *) palloc+sizeof(HGLOBAL);
	    }
	 }
	 else
	 {
	    if(pintern->Pointer)
	    {
	       HeapFree(heap, 0, (char *) pintern->Pointer-sizeof(HGLOBAL));
	       pintern->Pointer=NULL;
	    }
	 }
      }
   }
   /* HeapUnlock(heap); */
   return hnew;
}


/***********************************************************************
 *           GlobalFree32   (KERNEL32.322)
 * RETURNS
 *	NULL: Success
 *	Handle: Failure
 */
HGLOBAL WINAPI GlobalFree(
                 HGLOBAL hmem /* [in] Handle of global memory object */
) {
   PGLOBAL32_INTERN pintern;
   HGLOBAL        hreturned = 0;
   HANDLE heap = GLOBAL_GetHeap( hmem );
   
   if(ISPOINTER(hmem)) /* POINTER */
   {
      if(!HeapFree(heap, 0, (LPVOID) hmem)) hmem = 0;
   }
   else  /* HANDLE */
   {
      /* HeapLock(heap); */
      pintern=HANDLE_TO_INTERN(hmem);
      
      if(pintern->Magic==MAGIC_GLOBAL_USED)
      {	 
	 if(pintern->LockCount!=0)
	    SetLastError(ERROR_INVALID_HANDLE);
	 if(pintern->Pointer)
	    if(!HeapFree(heap, 0,
	                 (char *)(pintern->Pointer)-sizeof(HGLOBAL)))
	       hreturned=hmem;
	 if(!HeapFree(heap, 0, pintern))
	    hreturned=hmem;
      }      
      /* HeapUnlock(heap); */
   }
   return hreturned;
}


/***********************************************************************
 *           GlobalSize32   (KERNEL32.329)
 * RETURNS
 *	Size in bytes of the global memory object
 *	0: Failure
 */
DWORD WINAPI GlobalSize(
             HGLOBAL hmem /* [in] Handle of global memory object */
) {
   DWORD                retval;
   PGLOBAL32_INTERN     pintern;
   HANDLE heap          = GLOBAL_GetHeap( hmem );

   if(ISPOINTER(hmem)) 
   {
      retval=HeapSize(heap, 0,  (LPVOID) hmem);
   }
   else
   {
      /* HeapLock(heap); */
      pintern=HANDLE_TO_INTERN(hmem);
      
      if(pintern->Magic==MAGIC_GLOBAL_USED)
      {
        if (!pintern->Pointer) /* handle case of GlobalAlloc( ??,0) */
            return 0;
	 retval=HeapSize(heap, 0,
	                 (char *)(pintern->Pointer)-sizeof(HGLOBAL))-4;
	 if (retval == 0xffffffff-4) retval = 0;
      }
      else
      {
	 WARN("invalid handle\n");
	 retval=0;
      }
      /* HeapUnlock(heap); */
   }
   /* HeapSize returns 0xffffffff on failure */
   if (retval == 0xffffffff) retval = 0;
   return retval;
}


/***********************************************************************
 *           GlobalWire32   (KERNEL32.333)
 */
LPVOID WINAPI GlobalWire(HGLOBAL hmem)
{
   return GlobalLock( hmem );
}


/***********************************************************************
 *           GlobalUnWire32   (KERNEL32.330)
 */
BOOL WINAPI GlobalUnWire(HGLOBAL hmem)
{
   return GlobalUnlock( hmem);
}


/***********************************************************************
 *           GlobalFix32   (KERNEL32.320)
 */
VOID WINAPI GlobalFix(HGLOBAL hmem)
{
    GlobalLock( hmem );
}


/***********************************************************************
 *           GlobalUnfix32   (KERNEL32.331)
 */
VOID WINAPI GlobalUnfix(HGLOBAL hmem)
{
   GlobalUnlock( hmem);
}


/***********************************************************************
 *           GlobalFlags32   (KERNEL32.321)
 * Returns information about the specified global memory object
 *
 * NOTES
 *	Should this return GMEM_INVALID_HANDLE on invalid handle?
 *
 * RETURNS
 *	Value specifying allocation flags and lock count
 *	GMEM_INVALID_HANDLE: Failure
 */
UINT WINAPI GlobalFlags(
              HGLOBAL hmem /* [in] Handle to global memory object */
) {
   DWORD                retval;
   PGLOBAL32_INTERN     pintern;
   
   if(ISPOINTER(hmem))
   {
      retval=0;
   }
   else
   {
      /* HeapLock(GetProcessHeap()); */
      pintern=HANDLE_TO_INTERN(hmem);
      if(pintern->Magic==MAGIC_GLOBAL_USED)
      {               
	 retval=pintern->LockCount + (pintern->Flags<<8);
	 if(pintern->Pointer==0)
	    retval|= GMEM_DISCARDED;
      }
      else
      {
	 WARN("Invalid handle: %04x", hmem);
	 retval=0;
      }
      /* HeapUnlock(GetProcessHeap()); */
   }
   return retval;
}


/***********************************************************************
 *           GlobalCompact32   (KERNEL32.316)
 */
DWORD WINAPI GlobalCompact( DWORD minfree )
{
    return 0;  /* GlobalCompact does nothing in Win32 */
}


/***********************************************************************
 *           GlobalMemoryStatus   (KERNEL32.327)
 * RETURNS
 *	None
 */
VOID WINAPI GlobalMemoryStatus(
            LPMEMORYSTATUS lpmem
) {
#ifdef linux
    FILE *f = fopen( "/proc/meminfo", "r" );
    if (f)
    {
        char buffer[256];
        int total, used, free, shared, buffers, cached;

        lpmem->dwLength = sizeof(MEMORYSTATUS);
        lpmem->dwTotalPhys = lpmem->dwAvailPhys = 0;
        lpmem->dwTotalPageFile = lpmem->dwAvailPageFile = 0;
        while (fgets( buffer, sizeof(buffer), f ))
        {
	    /* old style /proc/meminfo ... */
            if (sscanf( buffer, "Mem: %d %d %d %d %d %d", &total, &used, &free, &shared, &buffers, &cached ))
            {
                lpmem->dwTotalPhys += total;
                lpmem->dwAvailPhys += free + buffers + cached;
            }
            if (sscanf( buffer, "Swap: %d %d %d", &total, &used, &free ))
            {
                lpmem->dwTotalPageFile += total;
                lpmem->dwAvailPageFile += free;
            }

	    /* new style /proc/meminfo ... */
	    if (sscanf(buffer, "MemTotal: %d", &total))
	    	lpmem->dwTotalPhys = total*1024;
	    if (sscanf(buffer, "MemFree: %d", &free))
	    	lpmem->dwAvailPhys = free*1024;
	    if (sscanf(buffer, "SwapTotal: %d", &total))
	        lpmem->dwTotalPageFile = total*1024;
	    if (sscanf(buffer, "SwapFree: %d", &free))
	        lpmem->dwAvailPageFile = free*1024;
	    if (sscanf(buffer, "Buffers: %d", &buffers))
	        lpmem->dwAvailPhys += buffers*1024;
	    if (sscanf(buffer, "Cached: %d", &cached))
	        lpmem->dwAvailPhys += cached*1024;
        }
        fclose( f );

        if (lpmem->dwTotalPhys)
        {
            lpmem->dwTotalVirtual = lpmem->dwTotalPhys+lpmem->dwTotalPageFile;
            lpmem->dwAvailVirtual = lpmem->dwAvailPhys+lpmem->dwAvailPageFile;
            lpmem->dwMemoryLoad = (lpmem->dwTotalVirtual-lpmem->dwAvailVirtual)
                                      / (lpmem->dwTotalVirtual / 100);
            return;
        }
    }
#endif
    /* FIXME: should do something for other systems */
    lpmem->dwMemoryLoad    = 0;
    lpmem->dwTotalPhys     = 16*1024*1024;
    lpmem->dwAvailPhys     = 16*1024*1024;
    lpmem->dwTotalPageFile = 16*1024*1024;
    lpmem->dwAvailPageFile = 16*1024*1024;
    lpmem->dwTotalVirtual  = 32*1024*1024;
    lpmem->dwAvailVirtual  = 32*1024*1024;
}


/**********************************************************************
 *           WOWGlobalAllocLock (KERNEL32.62)
 *
 * Combined GlobalAlloc and GlobalLock.
 */
SEGPTR WINAPI WOWGlobalAllocLock16(DWORD flags,DWORD cb,HGLOBAL16 *hmem)
{
    HGLOBAL16 xhmem;
    xhmem = GlobalAlloc16(flags,cb);
    if (hmem) *hmem = xhmem;
    return WIN16_GlobalLock16(xhmem);
}


/**********************************************************************
 *           WOWGlobalUnlockFree (KERNEL32.64)
 *
 * Combined GlobalUnlock and GlobalFree.
 */
WORD WINAPI WOWGlobalUnlockFree16(DWORD vpmem) {
    if (!GlobalUnlock16(HIWORD(vpmem)))
    	return 0;
    return GlobalFree16(HIWORD(vpmem));
}


/***********************************************************************
 *           A20Proc16   (KERNEL.165)
 */
void WINAPI A20Proc16( WORD unused )
{
    /* this is also a NOP in Windows */
}

/***********************************************************************
 *           LimitEMSPages16   (KERNEL.156)
 */
DWORD WINAPI LimitEMSPages16( DWORD unused )
{
    return 0;
}
