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

#define GLOBAL_MAX_ALLOC_SIZE 0x00ff0000  /* Largest allocation is 16M - 64K */

#define HGLOBAL_TO_SEL(handle) \
     ((handle == (HGLOBAL)-1) ? CURRENT_DS : GlobalHandleToSel(handle))

/***********************************************************************
 *           GlobalAlloc   (KERNEL.15)
 */
HGLOBAL GlobalAlloc( WORD flags, DWORD size )
{
    WORD sel;
    void *ptr;

    dprintf_global( stddeb, "GlobalAlloc: %ld flags=%04x\n", size, flags );

      /* Fixup the size */

    if (size >= GLOBAL_MAX_ALLOC_SIZE - 0x0f) return 0;
    if (size == 0) size = 0x10;
    else size = (size + 0x0f) & ~0x0f;

      /* Allocate the linear memory */

    ptr = malloc( size );
    if (!ptr) return 0;

      /* Allocate the selector(s) */

    sel = SELECTOR_AllocBlock( ptr, size, SEGMENT_DATA, 0, 0 );
    if (!sel)
    {
        free( ptr );
        return 0;
    }

    if (flags & GMEM_ZEROINIT) memset( ptr, 0, size );

      /* Return the handle */
      /* If allocating a GMEM_FIXED block, the handle is the selector. */
      /* Otherwise, the handle is the selector-1 (don't ask me why :-) */
    if (flags & GMEM_MOVEABLE) return sel - 1;
    else return sel;
}


/***********************************************************************
 *           GlobalReAlloc   (KERNEL.16)
 */
HGLOBAL GlobalReAlloc( HGLOBAL handle, DWORD size, WORD flags )
{
    WORD sel;
    DWORD oldsize;
    void *ptr;

    dprintf_global( stddeb, "GlobalReAlloc: %04x %ld flags=%04x\n",
                    handle, size, flags );

      /* Fixup the size */

    if (size >= 0x00ff0000-0x0f) return 0;  /* No allocation > 16Mb-64Kb */
    if (size == 0) size = 0x10;
    else size = (size + 0x0f) & ~0x0f;

      /* Reallocate the linear memory */

    sel = GlobalHandleToSel( handle );
    ptr = (void *)GET_SEL_BASE( sel );
    oldsize = GlobalSize( handle );
    if (size == oldsize) return handle;  /* Nothing to do */

    ptr = realloc( ptr, size );
    if (!ptr)
    {
        FreeSelector( sel );
        return 0;
    }

      /* Reallocate the selector(s) */

    sel = SELECTOR_ReallocBlock( sel, ptr, size, SEGMENT_DATA, 0, 0 );
    if (!sel)
    {
        free( ptr );
        return 0;
    }

    if ((oldsize < size) && (flags & GMEM_ZEROINIT))
        memset( (char *)ptr + oldsize, 0, size - oldsize );

    if (sel == GlobalHandleToSel( handle ))
        return handle;  /* Selector didn't change */

    if (flags & GMEM_MOVEABLE) return sel - 1;
    else return sel;
}


/***********************************************************************
 *           GlobalFree   (KERNEL.17)
 */
HGLOBAL GlobalFree( HGLOBAL handle )
{
    WORD sel;
    void *ptr;

    dprintf_global( stddeb, "GlobalFree: %04x\n", handle );
    sel = GlobalHandleToSel( handle );
    ptr = (void *)GET_SEL_BASE(sel);
    if (FreeSelector( sel )) return handle;  /* failed */
    free( ptr );
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
    return (SEGPTR)MAKELONG( 0, HGLOBAL_TO_SEL(handle) );
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
    return (LPSTR)GET_SEL_BASE( HGLOBAL_TO_SEL(handle) );
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
    return GET_SEL_LIMIT( HGLOBAL_TO_SEL(handle) ) + 1;
}


/***********************************************************************
 *           GlobalHandle   (KERNEL.21)
 */
DWORD GlobalHandle( WORD sel )
{
      /* FIXME: what about GMEM_FIXED blocks? */
    WORD handle = sel & ~1;
    dprintf_global( stddeb, "GlobalHandle(%04x): returning %08lx\n",
                    sel, MAKELONG( handle, sel ) );
    return MAKELONG( handle, sel );
}


/***********************************************************************
 *           GlobalFlags   (KERNEL.22)
 */
WORD GlobalFlags( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalFlags: %04x\n", handle );
    return 0;  /* lock count is always 0 */
}


/***********************************************************************
 *           LockSegment   (KERNEL.23)
 */
HGLOBAL LockSegment( HGLOBAL handle )
{
    dprintf_global( stddeb, "LockSegment: %04x\n", handle );
    if (handle == (HGLOBAL)-1) handle = CURRENT_DS;
    return handle;
}


/***********************************************************************
 *           UnlockSegment   (KERNEL.24)
 */
void UnlockSegment( HGLOBAL handle )
{
    dprintf_global( stddeb, "UnlockSegment: %04x\n", handle );
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
LPSTR GlobalWire( HGLOBAL handle )
{
    return GlobalLock( handle );
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
    /* Nothing to do, as we can't page-lock a block (and we don't want to) */
    return 1;  /* lock count */
}


/***********************************************************************
 *           GlobalPageUnlock   (KERNEL.192)
 */
WORD GlobalPageUnlock( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalPageUnlock: %04x\n", handle );
    return 0;  /* lock count */
}


/***********************************************************************
 *           GlobalFix   (KERNEL.197)
 */
void GlobalFix( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalFix: %04x\n", handle );
    /* Nothing to do, as all the blocks are fixed in linear memory anyway */
}


/***********************************************************************
 *           GlobalUnfix   (KERNEL.198)
 */
void GlobalUnfix( HGLOBAL handle )
{
    dprintf_global( stddeb, "GlobalUnfix: %04x\n", handle );
    /* This one is even more complicated that GlobalFix() :-) */
}


/***********************************************************************
 *           GlobalHandleToSel   (TOOLHELP.50)
 */
WORD GlobalHandleToSel( HGLOBAL handle )
{
    dprintf_toolhelp( stddeb, "GlobalHandleToSel: %04x\n", handle );
    if (!(handle & 7))
    {
        fprintf( stderr, "Program attempted invalid selector conversion\n" );
        return handle - 1;
    }
    return handle | 1;
}


/**********************************************************************
 *                      MemManInfo (toolhelp.72)
 */
/*
BOOL MemManInfo(LPMEMMANINFO lpmmi)
{
	return 1;
}
*/
