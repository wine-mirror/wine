/*
 * Win32 heap functions
 *
 * Copyright 1996 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "thread.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* address where we try to map the system heap */
#define SYSTEM_HEAP_BASE  ((void*)0x65430000)
#define SYSTEM_HEAP_SIZE  0x100000   /* Default heap size = 1Mb */

static HANDLE systemHeap;   /* globally shared heap */

/***********************************************************************
 *           HEAP_CreateSystemHeap
 *
 * Create the system heap.
 */
inline static HANDLE HEAP_CreateSystemHeap(void)
{
    int created;
    void *base;
    HANDLE map, event;
    UNICODE_STRING event_name;
    OBJECT_ATTRIBUTES event_attr;

    if (!(map = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, SEC_COMMIT | PAGE_READWRITE,
                                    0, SYSTEM_HEAP_SIZE, "__SystemHeap" ))) return 0;
    created = (GetLastError() != ERROR_ALREADY_EXISTS);

    if (!(base = MapViewOfFileEx( map, FILE_MAP_ALL_ACCESS, 0, 0, 0, SYSTEM_HEAP_BASE )))
    {
        /* pre-defined address not available */
        ERR( "system heap base address %p not available\n", SYSTEM_HEAP_BASE );
        return 0;
    }

    /* create the system heap event */
    RtlCreateUnicodeStringFromAsciiz( &event_name, "__SystemHeapEvent" );
    event_attr.Length = sizeof(event_attr);
    event_attr.RootDirectory = 0;
    event_attr.ObjectName = &event_name;
    event_attr.Attributes = 0;
    event_attr.SecurityDescriptor = NULL;
    event_attr.SecurityQualityOfService = NULL;
    NtCreateEvent( &event, EVENT_ALL_ACCESS, &event_attr, TRUE, FALSE );

    if (created)  /* newly created heap */
    {
        systemHeap = RtlCreateHeap( HEAP_SHARED, base, SYSTEM_HEAP_SIZE,
                                    SYSTEM_HEAP_SIZE, NULL, NULL );
        NtSetEvent( event, NULL );
    }
    else
    {
        /* wait for the heap to be initialized */
        WaitForSingleObject( event, INFINITE );
        systemHeap = (HANDLE)base;
    }
    CloseHandle( map );
    return systemHeap;
}


/***********************************************************************
 *           HeapCreate   (KERNEL32.@)
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
 *           HeapCompact   (KERNEL32.@)
 */
SIZE_T WINAPI HeapCompact( HANDLE heap, DWORD flags )
{
    return RtlCompactHeap( heap, flags );
}


/***********************************************************************
 *           HeapLock   (KERNEL32.@)
 * Attempts to acquire the critical section object for a specified heap.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapLock(
              HANDLE heap /* [in] Handle of heap to lock for exclusive access */
) {
    return RtlLockHeap( heap );
}


/***********************************************************************
 *           HeapUnlock   (KERNEL32.@)
 * Releases ownership of the critical section object.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapUnlock(
              HANDLE heap /* [in] Handle to the heap to unlock */
) {
    return RtlUnlockHeap( heap );
}


/***********************************************************************
 *           HeapValidate   (KERNEL32.@)
 * Validates a specified heap.
 *
 * NOTES
 *	Flags is ignored.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapValidate(
              HANDLE heap, /* [in] Handle to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              LPCVOID block  /* [in] Optional pointer to memory block to validate */
) {
    return RtlValidateHeap( heap, flags, block );
}


/***********************************************************************
 *           HeapWalk   (KERNEL32.@)
 * Enumerates the memory blocks in a specified heap.
 *
 * TODO
 *   - handling of PROCESS_HEAP_ENTRY_MOVEABLE and
 *     PROCESS_HEAP_ENTRY_DDESHARE (needs heap.c support)
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
BOOL WINAPI HeapWalk(
              HANDLE heap,               /* [in]  Handle to heap to enumerate */
              LPPROCESS_HEAP_ENTRY entry /* [out] Pointer to structure of enumeration info */
) {
    NTSTATUS ret = RtlWalkHeap( heap, entry );
    if (ret) SetLastError( RtlNtStatusToDosError(ret) );
    return !ret;
}


/***********************************************************************
 *           GetProcessHeap    (KERNEL32.@)
 */
HANDLE WINAPI GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}


/***********************************************************************
 *           GetProcessHeaps    (KERNEL32.@)
 */
DWORD WINAPI GetProcessHeaps( DWORD count, HANDLE *heaps )
{
    return RtlGetProcessHeaps( count, heaps );
}



/* FIXME: these functions are needed for dlls that aren't properly separated yet */

LPVOID WINAPI HeapAlloc( HANDLE heap, DWORD flags, SIZE_T size )
{
    return RtlAllocateHeap( heap, flags, size );
}

BOOL WINAPI HeapFree( HANDLE heap, DWORD flags, LPVOID ptr )
{
    return RtlFreeHeap( heap, flags, ptr );
}

LPVOID WINAPI HeapReAlloc( HANDLE heap, DWORD flags, LPVOID ptr, SIZE_T size )
{
    return RtlReAllocateHeap( heap, flags, ptr, size );
}

SIZE_T WINAPI HeapSize( HANDLE heap, DWORD flags, LPVOID ptr )
{
    return RtlSizeHeap( heap, flags, ptr );
}
