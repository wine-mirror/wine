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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winbase.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "winnt.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "selectors.h"
#include "global.h"
#include "thread.h"
#include "toolhelp.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* address where we try to map the system heap */
#define SYSTEM_HEAP_BASE  ((void*)0x65430000)
#define SYSTEM_HEAP_SIZE  0x100000   /* Default heap size = 1Mb */


#define HTABLE_SIZE      0x10000
#define HTABLE_PAGESIZE  0x1000
#define HTABLE_NPAGES    (HTABLE_SIZE / HTABLE_PAGESIZE)

#include "pshpack1.h"
typedef struct _LOCAL32HEADER
{
    WORD     freeListFirst[HTABLE_NPAGES];
    WORD     freeListSize[HTABLE_NPAGES];
    WORD     freeListLast[HTABLE_NPAGES];

    DWORD    selectorTableOffset;
    WORD     selectorTableSize;
    WORD     selectorDelta;

    DWORD    segment;
    LPBYTE   base;

    DWORD    limit;
    DWORD    flags;

    DWORD    magic;
    HANDLE heap;

} LOCAL32HEADER;
#include "poppack.h"

#define LOCAL32_MAGIC    ((DWORD)('L' | ('H'<<8) | ('3'<<16) | ('2'<<24)))

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
    HANDLE *pdb = (HANDLE *)NtCurrentTeb()->process;
    return pdb[0x18 / sizeof(HANDLE)];  /* get dword at offset 0x18 in pdb */
}


/***********************************************************************
 *           GetProcessHeaps    (KERNEL32.@)
 */
DWORD WINAPI GetProcessHeaps( DWORD count, HANDLE *heaps )
{
    return RtlGetProcessHeaps( count, heaps );
}


/***********************************************************************
 * 32-bit local heap functions (Win95; undocumented)
 */

/***********************************************************************
 *           K208   (KERNEL.208)
 */
HANDLE WINAPI Local32Init16( WORD segment, DWORD tableSize,
                             DWORD heapSize, DWORD flags )
{
    DWORD totSize, segSize = 0;
    LPBYTE base;
    LOCAL32HEADER *header;
    HANDLE heap;
    WORD *selectorTable;
    WORD selectorEven, selectorOdd;
    int i, nrBlocks;

    /* Determine new heap size */

    if ( segment )
    {
        if ( (segSize = GetSelectorLimit16( segment )) == 0 )
            return 0;
        else
            segSize++;
    }

    if ( heapSize == -1L )
        heapSize = 1024L*1024L;   /* FIXME */

    heapSize = (heapSize + 0xffff) & 0xffff0000;
    segSize  = (segSize  + 0x0fff) & 0xfffff000;
    totSize  = segSize + HTABLE_SIZE + heapSize;


    /* Allocate memory and initialize heap */

    if ( !(base = VirtualAlloc( NULL, totSize, MEM_RESERVE, PAGE_READWRITE )) )
        return 0;

    if ( !VirtualAlloc( base, segSize + HTABLE_PAGESIZE,
                        MEM_COMMIT, PAGE_READWRITE ) )
    {
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }

    if (!(heap = RtlCreateHeap( 0, base + segSize + HTABLE_SIZE, heapSize, 0x10000, NULL, NULL )))
    {
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }


    /* Set up header and handle table */

    header = (LOCAL32HEADER *)(base + segSize);
    header->base    = base;
    header->limit   = HTABLE_PAGESIZE-1;
    header->flags   = 0;
    header->magic   = LOCAL32_MAGIC;
    header->heap    = heap;

    header->freeListFirst[0] = sizeof(LOCAL32HEADER);
    header->freeListLast[0]  = HTABLE_PAGESIZE - 4;
    header->freeListSize[0]  = (HTABLE_PAGESIZE - sizeof(LOCAL32HEADER)) / 4;

    for (i = header->freeListFirst[0]; i < header->freeListLast[0]; i += 4)
        *(DWORD *)((LPBYTE)header + i) = i+4;

    header->freeListFirst[1] = 0xffff;


    /* Set up selector table */

    nrBlocks      = (totSize + 0x7fff) >> 15;
    selectorTable = (LPWORD) HeapAlloc( header->heap,  0, nrBlocks * 2 );
    selectorEven  = SELECTOR_AllocBlock( base, totSize, WINE_LDT_FLAGS_DATA );
    selectorOdd   = SELECTOR_AllocBlock( base + 0x8000, totSize - 0x8000, WINE_LDT_FLAGS_DATA );
    if ( !selectorTable || !selectorEven || !selectorOdd )
    {
        if ( selectorTable ) HeapFree( header->heap, 0, selectorTable );
        if ( selectorEven  ) SELECTOR_FreeBlock( selectorEven );
        if ( selectorOdd   ) SELECTOR_FreeBlock( selectorOdd );
        HeapDestroy( header->heap );
        VirtualFree( base, 0, MEM_RELEASE );
        return 0;
    }

    header->selectorTableOffset = (LPBYTE)selectorTable - header->base;
    header->selectorTableSize   = nrBlocks * 4;  /* ??? Win95 does it this way! */
    header->selectorDelta       = selectorEven - selectorOdd;
    header->segment             = segment? segment : selectorEven;

    for (i = 0; i < nrBlocks; i++)
        selectorTable[i] = (i & 1)? selectorOdd  + ((i >> 1) << __AHSHIFT)
                                  : selectorEven + ((i >> 1) << __AHSHIFT);

    /* Move old segment */

    if ( segment )
    {
        /* FIXME: This is somewhat ugly and relies on implementation
                  details about 16-bit global memory handles ... */

        LPBYTE oldBase = (LPBYTE)GetSelectorBase( segment );
        memcpy( base, oldBase, segSize );
        GLOBAL_MoveBlock( segment, base, totSize );
        HeapFree( GetProcessHeap(), 0, oldBase );
    }

    return (HANDLE)header;
}

/***********************************************************************
 *           Local32_SearchHandle
 */
static LPDWORD Local32_SearchHandle( LOCAL32HEADER *header, DWORD addr )
{
    LPDWORD handle;

    for ( handle = (LPDWORD)((LPBYTE)header + sizeof(LOCAL32HEADER));
          handle < (LPDWORD)((LPBYTE)header + header->limit);
          handle++)
    {
        if (*handle == addr)
            return handle;
    }

    return NULL;
}

/***********************************************************************
 *           Local32_ToHandle
 */
static VOID Local32_ToHandle( LOCAL32HEADER *header, INT16 type,
                              DWORD addr, LPDWORD *handle, LPBYTE *ptr )
{
    *handle = NULL;
    *ptr    = NULL;

    switch (type)
    {
        case -2:    /* 16:16 pointer, no handles */
            *ptr    = MapSL( addr );
            *handle = (LPDWORD)*ptr;
            break;

        case -1:    /* 32-bit offset, no handles */
            *ptr    = header->base + addr;
            *handle = (LPDWORD)*ptr;
            break;

        case 0:     /* handle */
            if (    addr >= sizeof(LOCAL32HEADER)
                 && addr <  header->limit && !(addr & 3)
                 && *(LPDWORD)((LPBYTE)header + addr) >= HTABLE_SIZE )
            {
                *handle = (LPDWORD)((LPBYTE)header + addr);
                *ptr    = header->base + **handle;
            }
            break;

        case 1:     /* 16:16 pointer */
            *ptr    = MapSL( addr );
            *handle = Local32_SearchHandle( header, *ptr - header->base );
            break;

        case 2:     /* 32-bit offset */
            *ptr    = header->base + addr;
            *handle = Local32_SearchHandle( header, *ptr - header->base );
            break;
    }
}

/***********************************************************************
 *           Local32_FromHandle
 */
static VOID Local32_FromHandle( LOCAL32HEADER *header, INT16 type,
                                DWORD *addr, LPDWORD handle, LPBYTE ptr )
{
    switch (type)
    {
        case -2:    /* 16:16 pointer */
        case  1:
        {
            WORD *selTable = (LPWORD)(header->base + header->selectorTableOffset);
            DWORD offset   = (LPBYTE)ptr - header->base;
            *addr = MAKELONG( offset & 0x7fff, selTable[offset >> 15] );
        }
        break;

        case -1:    /* 32-bit offset */
        case  2:
            *addr = ptr - header->base;
            break;

        case  0:    /* handle */
            *addr = (LPBYTE)handle - (LPBYTE)header;
            break;
    }
}

/***********************************************************************
 *           K209   (KERNEL.209)
 */
DWORD WINAPI Local32Alloc16( HANDLE heap, DWORD size, INT16 type, DWORD flags )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;
    DWORD addr;

    /* Allocate memory */
    ptr = HeapAlloc( header->heap,
                     (flags & LMEM_MOVEABLE)? HEAP_ZERO_MEMORY : 0, size );
    if (!ptr) return 0;


    /* Allocate handle if requested */
    if (type >= 0)
    {
        int page, i;

        /* Find first page of handle table with free slots */
        for (page = 0; page < HTABLE_NPAGES; page++)
            if (header->freeListFirst[page] != 0)
                break;
        if (page == HTABLE_NPAGES)
        {
            WARN("Out of handles!\n" );
            HeapFree( header->heap, 0, ptr );
            return 0;
        }

        /* If virgin page, initialize it */
        if (header->freeListFirst[page] == 0xffff)
        {
            if ( !VirtualAlloc( (LPBYTE)header + (page << 12),
                                0x1000, MEM_COMMIT, PAGE_READWRITE ) )
            {
                WARN("Cannot grow handle table!\n" );
                HeapFree( header->heap, 0, ptr );
                return 0;
            }

            header->limit += HTABLE_PAGESIZE;

            header->freeListFirst[page] = 0;
            header->freeListLast[page]  = HTABLE_PAGESIZE - 4;
            header->freeListSize[page]  = HTABLE_PAGESIZE / 4;

            for (i = 0; i < HTABLE_PAGESIZE; i += 4)
                *(DWORD *)((LPBYTE)header + i) = i+4;

            if (page < HTABLE_NPAGES-1)
                header->freeListFirst[page+1] = 0xffff;
        }

        /* Allocate handle slot from page */
        handle = (LPDWORD)((LPBYTE)header + header->freeListFirst[page]);
        if (--header->freeListSize[page] == 0)
            header->freeListFirst[page] = header->freeListLast[page] = 0;
        else
            header->freeListFirst[page] = *handle;

        /* Store 32-bit offset in handle slot */
        *handle = ptr - header->base;
    }
    else
    {
        handle = (LPDWORD)ptr;
        header->flags |= 1;
    }


    /* Convert handle to requested output type */
    Local32_FromHandle( header, type, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K210   (KERNEL.210)
 */
DWORD WINAPI Local32ReAlloc16( HANDLE heap, DWORD addr, INT16 type,
                             DWORD size, DWORD flags )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;

    if (!addr)
        return Local32Alloc16( heap, size, type, flags );

    /* Retrieve handle and pointer */
    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return FALSE;

    /* Reallocate memory block */
    ptr = HeapReAlloc( header->heap,
                       (flags & LMEM_MOVEABLE)? HEAP_ZERO_MEMORY : 0,
                       ptr, size );
    if (!ptr) return 0;

    /* Modify handle */
    if (type >= 0)
        *handle = ptr - header->base;
    else
        handle = (LPDWORD)ptr;

    /* Convert handle to requested output type */
    Local32_FromHandle( header, type, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K211   (KERNEL.211)
 */
BOOL WINAPI Local32Free16( HANDLE heap, DWORD addr, INT16 type )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;

    /* Retrieve handle and pointer */
    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return FALSE;

    /* Free handle if necessary */
    if (type >= 0)
    {
        int offset = (LPBYTE)handle - (LPBYTE)header;
        int page   = offset >> 12;

        /* Return handle slot to page free list */
        if (header->freeListSize[page]++ == 0)
            header->freeListFirst[page] = header->freeListLast[page]  = offset;
        else
            *(LPDWORD)((LPBYTE)header + header->freeListLast[page]) = offset,
            header->freeListLast[page] = offset;

        *handle = 0;

        /* Shrink handle table when possible */
        while (page > 0 && header->freeListSize[page] == HTABLE_PAGESIZE / 4)
        {
            if ( VirtualFree( (LPBYTE)header +
                              (header->limit & ~(HTABLE_PAGESIZE-1)),
                              HTABLE_PAGESIZE, MEM_DECOMMIT ) )
                break;

            header->limit -= HTABLE_PAGESIZE;
            header->freeListFirst[page] = 0xffff;
            page--;
        }
    }

    /* Free memory */
    return HeapFree( header->heap, 0, ptr );
}

/***********************************************************************
 *           K213   (KERNEL.213)
 */
DWORD WINAPI Local32Translate16( HANDLE heap, DWORD addr, INT16 type1, INT16 type2 )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, type1, addr, &handle, &ptr );
    if (!handle) return 0;

    Local32_FromHandle( header, type2, &addr, handle, ptr );
    return addr;
}

/***********************************************************************
 *           K214   (KERNEL.214)
 */
DWORD WINAPI Local32Size16( HANDLE heap, DWORD addr, INT16 type )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, type, addr, &handle, &ptr );
    if (!handle) return 0;

    return HeapSize( header->heap, 0, ptr );
}

/***********************************************************************
 *           K215   (KERNEL.215)
 */
BOOL WINAPI Local32ValidHandle16( HANDLE heap, WORD addr )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    LPDWORD handle;
    LPBYTE ptr;

    Local32_ToHandle( header, 0, addr, &handle, &ptr );
    return handle != NULL;
}

/***********************************************************************
 *           K229   (KERNEL.229)
 */
WORD WINAPI Local32GetSegment16( HANDLE heap )
{
    LOCAL32HEADER *header = (LOCAL32HEADER *)heap;
    return header->segment;
}

/***********************************************************************
 *           Local32_GetHeap
 */
static LOCAL32HEADER *Local32_GetHeap( HGLOBAL16 handle )
{
    WORD selector = GlobalHandleToSel16( handle );
    DWORD base  = GetSelectorBase( selector );
    DWORD limit = GetSelectorLimit16( selector );

    /* Hmmm. This is a somewhat stupid heuristic, but Windows 95 does
       it this way ... */

    if ( limit > 0x10000 && ((LOCAL32HEADER *)base)->magic == LOCAL32_MAGIC )
        return (LOCAL32HEADER *)base;

    base  += 0x10000;
    limit -= 0x10000;

    if ( limit > 0x10000 && ((LOCAL32HEADER *)base)->magic == LOCAL32_MAGIC )
        return (LOCAL32HEADER *)base;

    return NULL;
}

/***********************************************************************
 *           Local32Info   (KERNEL.444)
 *           Local32Info   (TOOLHELP.84)
 */
BOOL16 WINAPI Local32Info16( LOCAL32INFO *pLocal32Info, HGLOBAL16 handle )
{
    PROCESS_HEAP_ENTRY entry;
    int i;

    LOCAL32HEADER *header = Local32_GetHeap( handle );
    if ( !header ) return FALSE;

    if ( !pLocal32Info || pLocal32Info->dwSize < sizeof(LOCAL32INFO) )
        return FALSE;

    pLocal32Info->dwMemReserved = 0;
    pLocal32Info->dwMemCommitted = 0;
    pLocal32Info->dwTotalFree = 0;
    pLocal32Info->dwLargestFreeBlock = 0;

    while (HeapWalk( header->heap, &entry ))
    {
        if (entry.wFlags & PROCESS_HEAP_REGION)
        {
            pLocal32Info->dwMemReserved += entry.u.Region.dwCommittedSize
                                           + entry.u.Region.dwUnCommittedSize;
            pLocal32Info->dwMemCommitted = entry.u.Region.dwCommittedSize;
        }
        else if (!(entry.wFlags & PROCESS_HEAP_ENTRY_BUSY))
        {
            DWORD size = entry.cbData + entry.cbOverhead;
            pLocal32Info->dwTotalFree += size;
            if (size > pLocal32Info->dwLargestFreeBlock) pLocal32Info->dwLargestFreeBlock = size;
        }
    }

    pLocal32Info->dwcFreeHandles = 0;
    for ( i = 0; i < HTABLE_NPAGES; i++ )
    {
        if ( header->freeListFirst[i] == 0xffff ) break;
        pLocal32Info->dwcFreeHandles += header->freeListSize[i];
    }
    pLocal32Info->dwcFreeHandles += (HTABLE_NPAGES - i) * HTABLE_PAGESIZE/4;

    return TRUE;
}

/***********************************************************************
 *           Local32First   (KERNEL.445)
 *           Local32First   (TOOLHELP.85)
 */
BOOL16 WINAPI Local32First16( LOCAL32ENTRY *pLocal32Entry, HGLOBAL16 handle )
{
    FIXME("(%p, %04X): stub!\n", pLocal32Entry, handle );
    return FALSE;
}

/***********************************************************************
 *           Local32Next   (KERNEL.446)
 *           Local32Next   (TOOLHELP.86)
 */
BOOL16 WINAPI Local32Next16( LOCAL32ENTRY *pLocal32Entry )
{
    FIXME("(%p): stub!\n", pLocal32Entry );
    return FALSE;
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

void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
    RtlEnterCriticalSection( crit );
}

BOOL WINAPI TryEnterCriticalSection( CRITICAL_SECTION *crit )
{
    return RtlTryEnterCriticalSection( crit );
}

void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    RtlDeleteCriticalSection( crit );
}

void WINAPI LeaveCriticalSection( CRITICAL_SECTION *crit )
{
    RtlLeaveCriticalSection( crit );
}
