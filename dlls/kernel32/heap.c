/*
 * Win32 heap functions
 *
 * Copyright 1995, 1996 Alexandre Julliard
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

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winternl.h"

#include "kernel_private.h"

#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(globalmem);


/***********************************************************************
 *           HeapCreate   (KERNEL32.@)
 *
 * Create a heap object.
 *
 * RETURNS
 *	Handle of heap: Success
 *	NULL: Failure
 */
HANDLE WINAPI HeapCreate(
                DWORD flags,       /* [in] Heap allocation flag */
                SIZE_T initialSize, /* [in] Initial heap size */
                SIZE_T maxSize      /* [in] Maximum heap size */
) {
    HANDLE ret;

    ret = RtlCreateHeap( flags, NULL, maxSize, initialSize, NULL, NULL );
    if (!ret) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return ret;
}


/***********************************************************************
 *           HeapDestroy   (KERNEL32.@)
 *
 * Destroy a heap object.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapDestroy( HANDLE heap /* [in] Handle of heap */ )
{
    if (!RtlDestroyHeap( heap )) return TRUE;
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}


/***********************************************************************
 * Global/local heap functions, keep in sync with kernelbase/memory.c
 ***********************************************************************/

#define MEM_FLAG_USED        1
#define MEM_FLAG_MOVEABLE    2
#define MEM_FLAG_DISCARDABLE 4
#define MEM_FLAG_DISCARDED   8
#define MEM_FLAG_DDESHARE    0x8000

struct mem_entry
{
    union
    {
        struct
        {
            WORD flags;
            BYTE lock;
        };
        void *next_free;
    };
    void *ptr;
};

C_ASSERT(sizeof(struct mem_entry) == 2 * sizeof(void *));

struct kernelbase_global_data *kernelbase_global_data;

static inline struct mem_entry *unsafe_mem_from_HLOCAL( HLOCAL handle )
{
    struct mem_entry *mem = CONTAINING_RECORD( *(volatile HANDLE *)&handle, struct mem_entry, ptr );
    struct kernelbase_global_data *data = kernelbase_global_data;
    if (((UINT_PTR)handle & ((sizeof(void *) << 1) - 1)) != sizeof(void *)) return NULL;
    if (mem < data->mem_entries || mem >= data->mem_entries_end) return NULL;
    if (!(mem->flags & MEM_FLAG_USED)) return NULL;
    return mem;
}

static inline void *unsafe_ptr_from_HLOCAL( HLOCAL handle )
{
    if (((UINT_PTR)handle & ((sizeof(void *) << 1) - 1))) return NULL;
    return handle;
}


/***********************************************************************
 *           GlobalLock   (KERNEL32.@)
 *
 * Lock a global memory object and return a pointer to first byte of the memory
 *
 * PARAMS
 *  handle [I] Handle of the global memory object
 *
 * RETURNS
 *  Success: Pointer to first byte of the memory block
 *  Failure: NULL
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
void *WINAPI GlobalLock( HGLOBAL handle )
{
    return LocalLock( handle );
}


/***********************************************************************
 *           GlobalUnlock   (KERNEL32.@)
 *
 * Unlock a global memory object.
 *
 * PARAMS
 *  handle [I] Handle of the global memory object
 *
 * RETURNS
 *  Success: Object is still locked
 *  Failure: FALSE (The Object is unlocked)
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
BOOL WINAPI GlobalUnlock( HGLOBAL handle )
{
    if (unsafe_ptr_from_HLOCAL( handle )) return TRUE;
    return LocalUnlock( handle );
}


/***********************************************************************
 *           GlobalHandle   (KERNEL32.@)
 */
HGLOBAL WINAPI GlobalHandle( const void *ptr )
{
    return LocalHandle( ptr );
}

/***********************************************************************
 *           GlobalReAlloc   (KERNEL32.@)
 *
 * Change the size or attributes of a global memory object.
 *
 * RETURNS
 *      Handle: Success
 *      NULL: Failure
 */
HGLOBAL WINAPI GlobalReAlloc( HGLOBAL handle, SIZE_T size, UINT flags )
{
    struct mem_entry *mem;
    void *ptr;

    if (!(flags & GMEM_MODIFY) && (mem = unsafe_mem_from_HLOCAL( handle )) &&
        mem->lock && (!size || (flags & GMEM_DISCARDABLE)))
        return 0;

    if (!(handle = LocalReAlloc( handle, size, flags ))) return 0;

    /* GlobalReAlloc allows changing GMEM_FIXED to GMEM_MOVEABLE with GMEM_MODIFY */
    if ((flags & (GMEM_MOVEABLE | GMEM_MODIFY)) == (GMEM_MOVEABLE | GMEM_MODIFY) &&
        (ptr = unsafe_ptr_from_HLOCAL( handle )))
    {
        if (!(handle = LocalAlloc( flags, 0 ))) return 0;
        RtlSetUserValueHeap( GetProcessHeap(), 0, ptr, handle );
        mem = unsafe_mem_from_HLOCAL( handle );
        mem->flags &= ~MEM_FLAG_DISCARDED;
        mem->ptr = ptr;
    }

    return handle;
}


/***********************************************************************
 *           GlobalSize   (KERNEL32.@)
 */
SIZE_T WINAPI GlobalSize( HGLOBAL handle )
{
    return LocalSize( handle );
}


/***********************************************************************
 *           GlobalWire   (KERNEL32.@)
 */
void *WINAPI GlobalWire( HGLOBAL handle )
{
    return GlobalLock( handle );
}


/***********************************************************************
 *           GlobalUnWire   (KERNEL32.@)
 */
BOOL WINAPI GlobalUnWire( HGLOBAL handle )
{
    return GlobalUnlock( handle );
}


/***********************************************************************
 *           GlobalFix   (KERNEL32.@)
 */
VOID WINAPI GlobalFix( HGLOBAL handle )
{
    GlobalLock( handle );
}


/***********************************************************************
 *           GlobalUnfix   (KERNEL32.@)
 */
VOID WINAPI GlobalUnfix( HGLOBAL handle )
{
    GlobalUnlock( handle );
}


/***********************************************************************
 *           GlobalFlags   (KERNEL32.@)
 *
 * Get information about a global memory object.
 *
 * PARAMS
 *  handle [I] Handle of the global memory object
 *
 * RETURNS
 *  Failure: GMEM_INVALID_HANDLE, when the provided handle is invalid
 *  Success: Value specifying allocation flags and lock count
 *
 */
UINT WINAPI GlobalFlags( HGLOBAL handle )
{
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    UINT flags;

    if (unsafe_ptr_from_HLOCAL( handle )) return 0;

    RtlLockHeap( heap );
    if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        flags = mem->lock;
        if (mem->flags & MEM_FLAG_DISCARDABLE) flags |= GMEM_DISCARDABLE;
        if (mem->flags & MEM_FLAG_DISCARDED) flags |= GMEM_DISCARDED;
        if (mem->flags & MEM_FLAG_DDESHARE) flags |= GMEM_DDESHARE;
    }
    else
    {
        WARN_(globalmem)( "invalid handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
        flags = GMEM_INVALID_HANDLE;
    }
    RtlUnlockHeap( heap );

    return flags;
}


/***********************************************************************
 *           GlobalCompact   (KERNEL32.@)
 */
SIZE_T WINAPI GlobalCompact( DWORD minfree )
{
    return 0;  /* GlobalCompact does nothing in Win32 */
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
 *
 * Get information about a local memory object.
 *
 * RETURNS
 *	Value specifying allocation flags and lock count.
 *	LMEM_INVALID_HANDLE: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
UINT WINAPI LocalFlags( HLOCAL handle )
{
    UINT flags = GlobalFlags( handle );
    if (flags & GMEM_DISCARDABLE) flags |= LMEM_DISCARDABLE;
    return flags;
}


/***********************************************************************
 *           LocalHandle   (KERNEL32.@)
 */
HLOCAL WINAPI LocalHandle( const void *ptr )
{
    HANDLE heap = GetProcessHeap();
    HLOCAL handle = (HANDLE)ptr;
    ULONG flags;

    TRACE_(globalmem)( "ptr %p\n", ptr );

    if (!ptr)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    RtlLockHeap( heap );
    if (!HeapValidate( heap, HEAP_NO_SERIALIZE, ptr ) ||
        !RtlGetUserInfoHeap( heap, HEAP_NO_SERIALIZE, (void *)ptr, &handle, &flags ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        handle = 0;
    }
    RtlUnlockHeap( heap );

    return handle;
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
 */
SIZE_T WINAPI LocalSize( HLOCAL handle )
{
    HANDLE heap = GetProcessHeap();
    struct mem_entry *mem;
    SIZE_T ret = 0;
    void *ptr;

    TRACE_(globalmem)( "handle %p\n", handle );

    RtlLockHeap( heap );
    if ((ptr = unsafe_ptr_from_HLOCAL( handle )) &&
        HeapValidate( heap, HEAP_NO_SERIALIZE, ptr ))
        ret = HeapSize( heap, HEAP_NO_SERIALIZE, ptr );
    else if ((mem = unsafe_mem_from_HLOCAL( handle )))
    {
        if (!mem->ptr) ret = 0;
        else ret = HeapSize( heap, HEAP_NO_SERIALIZE, mem->ptr );
    }
    else
    {
        WARN_(globalmem)( "invalid handle %p\n", handle );
        SetLastError( ERROR_INVALID_HANDLE );
    }
    RtlUnlockHeap( heap );

    if (ret == ~(SIZE_T)0) return 0;
    return ret;
}


/***********************************************************************
 *           GlobalMemoryStatus   (KERNEL32.@)
 * Provides information about the status of the memory, so apps can tell
 * roughly how much they are able to allocate
 *
 * RETURNS
 *      None
 */
VOID WINAPI GlobalMemoryStatus( LPMEMORYSTATUS lpBuffer )
{
    MEMORYSTATUSEX memstatus;
    OSVERSIONINFOW osver;
#ifndef _WIN64
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader( GetModuleHandleW(0) );
#endif

    /* Because GlobalMemoryStatus is identical to GlobalMemoryStatusEX save
       for one extra field in the struct, and the lack of a bug, we simply
       call GlobalMemoryStatusEx and copy the values across. */
    memstatus.dwLength = sizeof(memstatus);
    GlobalMemoryStatusEx(&memstatus);

    lpBuffer->dwLength = sizeof(*lpBuffer);
    lpBuffer->dwMemoryLoad = memstatus.dwMemoryLoad;

    /* Windows 2000 and later report -1 when values are greater than 4 Gb.
     * NT reports values modulo 4 Gb.
     */

    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionExW(&osver);

    lpBuffer->dwTotalPhys = memstatus.ullTotalPhys;
    lpBuffer->dwAvailPhys = memstatus.ullAvailPhys;
    lpBuffer->dwTotalPageFile = memstatus.ullTotalPageFile;
    lpBuffer->dwAvailPageFile = memstatus.ullAvailPageFile;
    lpBuffer->dwTotalVirtual = memstatus.ullTotalVirtual;
    lpBuffer->dwAvailVirtual = memstatus.ullAvailVirtual;

#ifndef _WIN64
    if ( osver.dwMajorVersion >= 5 || osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
    {
        lpBuffer->dwTotalPhys = min( memstatus.ullTotalPhys, MAXDWORD );
        lpBuffer->dwAvailPhys = min( memstatus.ullAvailPhys, MAXDWORD );
        /* Limit value for apps that do not expect so much memory. Remove last 512 kb to make Sacrifice demo happy. */
        lpBuffer->dwTotalPageFile = min( memstatus.ullTotalPageFile, 0xfff7ffff );
        lpBuffer->dwAvailPageFile = min( memstatus.ullAvailPageFile, MAXDWORD );
        lpBuffer->dwTotalVirtual = min( memstatus.ullTotalVirtual, MAXDWORD );
        lpBuffer->dwAvailVirtual = min( memstatus.ullAvailVirtual, MAXDWORD );
    }

    /* values are limited to 2Gb unless the app has the IMAGE_FILE_LARGE_ADDRESS_AWARE flag */
    /* page file sizes are not limited (Adobe Illustrator 8 depends on this) */
    if (!(nt->FileHeader.Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE))
    {
        if (lpBuffer->dwTotalPhys > MAXLONG) lpBuffer->dwTotalPhys = MAXLONG;
        if (lpBuffer->dwAvailPhys > MAXLONG) lpBuffer->dwAvailPhys = MAXLONG;
        if (lpBuffer->dwTotalVirtual > MAXLONG) lpBuffer->dwTotalVirtual = MAXLONG;
        if (lpBuffer->dwAvailVirtual > MAXLONG) lpBuffer->dwAvailVirtual = MAXLONG;
    }

    /* work around for broken photoshop 4 installer */
    if ( lpBuffer->dwAvailPhys +  lpBuffer->dwAvailPageFile >= 2U*1024*1024*1024)
         lpBuffer->dwAvailPageFile = 2U*1024*1024*1024 -  lpBuffer->dwAvailPhys - 1;

    /* limit page file size for really old binaries */
    if (nt->OptionalHeader.MajorSubsystemVersion < 4 ||
        nt->OptionalHeader.MajorOperatingSystemVersion < 4)
    {
        if (lpBuffer->dwTotalPageFile > MAXLONG) lpBuffer->dwTotalPageFile = MAXLONG;
        if (lpBuffer->dwAvailPageFile > MAXLONG) lpBuffer->dwAvailPageFile = MAXLONG;
    }
#endif

    TRACE("Length %lu, MemoryLoad %lu, TotalPhys %Ix, AvailPhys %Ix,"
          " TotalPageFile %Ix, AvailPageFile %Ix, TotalVirtual %Ix, AvailVirtual %Ix\n",
          lpBuffer->dwLength, lpBuffer->dwMemoryLoad, lpBuffer->dwTotalPhys,
          lpBuffer->dwAvailPhys, lpBuffer->dwTotalPageFile, lpBuffer->dwAvailPageFile,
          lpBuffer->dwTotalVirtual, lpBuffer->dwAvailVirtual );
}
