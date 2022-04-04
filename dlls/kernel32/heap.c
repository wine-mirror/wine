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

/* address where we try to map the system heap */
#define SYSTEM_HEAP_BASE  ((void*)0x80000000)
#define SYSTEM_HEAP_SIZE  0x1000000   /* Default heap size = 16Mb */

static HANDLE systemHeap;   /* globally shared heap */


/***********************************************************************
 *           HEAP_CreateSystemHeap
 *
 * Create the system heap.
 */
static inline HANDLE HEAP_CreateSystemHeap(void)
{
    int created;
    void *base;
    HANDLE map, event;

    /* create the system heap event first */
    event = CreateEventA( NULL, TRUE, FALSE, "__wine_system_heap_event" );

    if (!(map = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE,
                                    0, SYSTEM_HEAP_SIZE, "__wine_system_heap" ))) return 0;
    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!(base = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, SYSTEM_HEAP_BASE )))
    {
        /* pre-defined address not available */
        ERR( "system heap base address %p not available\n", SYSTEM_HEAP_BASE );
        return 0;
    }

    if (created)  /* newly created heap */
    {
        systemHeap = RtlCreateHeap( HEAP_SHARED, base, SYSTEM_HEAP_SIZE,
                                    SYSTEM_HEAP_SIZE, NULL, NULL );
        SetEvent( event );
    }
    else
    {
        /* wait for the heap to be initialized */
        WaitForSingleObject( event, INFINITE );
        systemHeap = base;
    }
    CloseHandle( map );
    return systemHeap;
}


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

    if ( flags & HEAP_SHARED )
    {
        if (!systemHeap) HEAP_CreateSystemHeap();
        else WARN( "Shared Heap requested, returning system heap.\n" );
        ret = systemHeap;
    }
    else
    {
        ret = RtlCreateHeap( flags, NULL, maxSize, initialSize, NULL, NULL );
        if (!ret) SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }
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
    if (heap == systemHeap)
    {
        WARN( "attempt to destroy system heap, returning TRUE!\n" );
        return TRUE;
    }
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

#define POINTER_TO_HANDLE( p ) (*(((const HGLOBAL *)( p )) - 2))
/* align the storage needed for the HLOCAL on an 8-byte boundary thus
 * LocalAlloc/LocalReAlloc'ing with LMEM_MOVEABLE of memory with
 * size = 8*k, where k=1,2,3,... allocs exactly the given size.
 * The Minolta DiMAGE Image Viewer heavily relies on this, corrupting
 * the output jpeg's > 1 MB if not */
#define HLOCAL_STORAGE      (sizeof(HLOCAL) * 2)

static inline struct mem_entry *unsafe_mem_from_HLOCAL( HLOCAL handle )
{
    struct mem_entry *mem = CONTAINING_RECORD( handle, struct mem_entry, ptr );
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
 *
 * Get the handle associated with the pointer to a global memory block.
 *
 * RETURNS
 *      Handle: Success
 *      NULL: Failure
 */
HGLOBAL WINAPI GlobalHandle( const void *ptr )
{
    struct mem_entry *mem;
    HGLOBAL handle;
    LPCVOID test;

    TRACE_(globalmem)( "ptr %p\n", ptr );

    if (!ptr)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    RtlLockHeap( GetProcessHeap() );
    __TRY
    {
        handle = 0;

        /* note that if ptr is a pointer to a block allocated by           */
        /* GlobalAlloc with GMEM_MOVEABLE then magic test in HeapValidate  */
        /* will fail.                                                      */
        if ((ptr = unsafe_ptr_from_HLOCAL( (HLOCAL)ptr )))
        {
            if (HeapValidate( GetProcessHeap(), HEAP_NO_SERIALIZE, ptr ))
            {
                handle = (HGLOBAL)ptr; /* valid fixed block */
                break;
            }
            handle = POINTER_TO_HANDLE( ptr );
        }
        else handle = (HGLOBAL)ptr;

        /* Now test handle either passed in or retrieved from pointer */
        if ((mem = unsafe_mem_from_HLOCAL( handle )))
        {
            test = mem->ptr;
            if (HeapValidate( GetProcessHeap(), HEAP_NO_SERIALIZE, (const char *)test - HLOCAL_STORAGE )) /* obj(-handle) valid arena? */
                break; /* valid moveable block */
        }
        handle = 0;
        SetLastError( ERROR_INVALID_HANDLE );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_HANDLE );
        handle = 0;
    }
    __ENDTRY
    RtlUnlockHeap( GetProcessHeap() );

    return handle;
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
    return LocalReAlloc( handle, size, flags );
}


/***********************************************************************
 *           GlobalSize   (KERNEL32.@)
 *
 * Get the size of a global memory object.
 *
 * PARAMS
 *  handle [I] Handle of the global memory object
 *
 * RETURNS
 *  Failure: 0
 *  Success: Size in Bytes of the global memory object
 *
 * NOTES
 *   When the handle is invalid, last error is set to ERROR_INVALID_HANDLE
 *
 */
SIZE_T WINAPI GlobalSize( HGLOBAL handle )
{
    struct mem_entry *mem;
    SIZE_T retval;
    void *ptr;

    TRACE_(globalmem)( "handle %p\n", handle );

    if (!((ULONG_PTR)handle >> 16))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if ((ptr = unsafe_ptr_from_HLOCAL( handle )))
    {
        retval = HeapSize( GetProcessHeap(), 0, ptr );
        if (retval == ~(SIZE_T)0) /* It might be a GMEM_MOVEABLE data pointer */
        {
            retval = HeapSize( GetProcessHeap(), 0, (char *)ptr - HLOCAL_STORAGE );
            if (retval != ~(SIZE_T)0) retval -= HLOCAL_STORAGE;
        }
    }
    else
    {
        RtlLockHeap( GetProcessHeap() );
        if ((mem = unsafe_mem_from_HLOCAL( handle )))
        {
            if (!mem->ptr) /* handle case of GlobalAlloc( ??,0) */
                retval = 0;
            else
            {
                retval = HeapSize( GetProcessHeap(), 0, (char *)mem->ptr - HLOCAL_STORAGE );
                if (retval != ~(SIZE_T)0) retval -= HLOCAL_STORAGE;
            }
        }
        else
        {
            WARN_(globalmem)( "invalid handle %p\n", handle );
            SetLastError( ERROR_INVALID_HANDLE );
            retval = 0;
        }
        RtlUnlockHeap( GetProcessHeap() );
    }
    if (retval == ~(SIZE_T)0) retval = 0;
    return retval;
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
 *
 * Get the handle associated with the pointer to a local memory block.
 *
 * RETURNS
 *	Handle: Success
 *	NULL: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
HLOCAL WINAPI LocalHandle(
                LPCVOID ptr /* [in] Address of local memory block */
) {
    return GlobalHandle( ptr );
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
 *
 * Get the size of a local memory object.
 *
 * RETURNS
 *	Size: Success
 *	0: Failure
 *
 * NOTES
 *  Windows memory management does not provide a separate local heap
 *  and global heap.
 */
SIZE_T WINAPI LocalSize(
              HLOCAL handle /* [in] Handle of memory object */
) {
    return GlobalSize( handle );
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
