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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"


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
