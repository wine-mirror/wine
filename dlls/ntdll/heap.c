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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define RUNNING_ON_VALGRIND 0  /* FIXME */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* Note: the heap data structures are loosely based on what Pietrek describes in his
 * book 'Windows 95 System Programming Secrets', with some adaptations for
 * better compatibility with NT.
 */

typedef struct tagARENA_INUSE
{
    DWORD  size;                    /* Block size; must be the first field */
    DWORD  magic : 24;              /* Magic number */
    DWORD  unused_bytes : 8;        /* Number of bytes in the block not used by user data (max value is HEAP_MIN_DATA_SIZE+HEAP_MIN_SHRINK_SIZE) */
} ARENA_INUSE;

typedef struct tagARENA_FREE
{
    DWORD                 size;     /* Block size; must be the first field */
    DWORD                 magic;    /* Magic number */
    struct list           entry;    /* Entry in free list */
} ARENA_FREE;

typedef struct
{
    struct list           entry;      /* entry in heap large blocks list */
    SIZE_T                data_size;  /* size of user data */
    SIZE_T                block_size; /* total size of virtual memory block */
    DWORD                 pad[2];     /* padding to ensure 16-byte alignment of data */
    DWORD                 size;       /* fields for compatibility with normal arenas */
    DWORD                 magic;      /* these must remain at the end of the structure */
} ARENA_LARGE;

#define ARENA_FLAG_FREE        0x00000001  /* flags OR'ed with arena size */
#define ARENA_FLAG_PREV_FREE   0x00000002
#define ARENA_SIZE_MASK        (~3)
#define ARENA_LARGE_SIZE       0xfedcba90  /* magic value for 'size' field in large blocks */

/* Value for arena 'magic' field */
#define ARENA_INUSE_MAGIC      0x455355
#define ARENA_PENDING_MAGIC    0xbedead
#define ARENA_FREE_MAGIC       0x45455246
#define ARENA_LARGE_MAGIC      0x6752614c

#define ARENA_INUSE_FILLER     0x55
#define ARENA_TAIL_FILLER      0xab
#define ARENA_FREE_FILLER      0xfeeefeee

/* everything is aligned on 8 byte boundaries (16 for Win64) */
#define ALIGNMENT              (2*sizeof(void*))
#define LARGE_ALIGNMENT        16  /* large blocks have stricter alignment */
#define ARENA_OFFSET           (ALIGNMENT - sizeof(ARENA_INUSE))

C_ASSERT( sizeof(ARENA_LARGE) % LARGE_ALIGNMENT == 0 );

#define ROUND_SIZE(size)       ((((size) + ALIGNMENT - 1) & ~(ALIGNMENT-1)) + ARENA_OFFSET)

#define QUIET                  1           /* Suppress messages  */
#define NOISY                  0           /* Report all errors  */

/* minimum data size (without arenas) of an allocated block */
/* make sure that it's larger than a free list entry */
#define HEAP_MIN_DATA_SIZE    ROUND_SIZE(2 * sizeof(struct list))
#define HEAP_MIN_ARENA_SIZE   (HEAP_MIN_DATA_SIZE + sizeof(ARENA_INUSE))
/* minimum size that must remain to shrink an allocated block */
#define HEAP_MIN_SHRINK_SIZE  (HEAP_MIN_DATA_SIZE+sizeof(ARENA_FREE))
/* minimum size to start allocating large blocks */
#define HEAP_MIN_LARGE_BLOCK_SIZE  0x7f000
/* extra size to add at the end of block for tail checking */
#define HEAP_TAIL_EXTRA_SIZE(flags) \
    ((flags & HEAP_TAIL_CHECKING_ENABLED) || RUNNING_ON_VALGRIND ? ALIGNMENT : 0)

/* There will be a free list bucket for every arena size up to and including this value */
#define HEAP_MAX_SMALL_FREE_LIST 0x100
C_ASSERT( HEAP_MAX_SMALL_FREE_LIST % ALIGNMENT == 0 );
#define HEAP_NB_SMALL_FREE_LISTS (((HEAP_MAX_SMALL_FREE_LIST - HEAP_MIN_ARENA_SIZE) / ALIGNMENT) + 1)

/* Max size of the blocks on the free lists above HEAP_MAX_SMALL_FREE_LIST */
static const SIZE_T HEAP_freeListSizes[] =
{
    0x200, 0x400, 0x1000, ~(SIZE_T)0
};
#define HEAP_NB_FREE_LISTS (ARRAY_SIZE( HEAP_freeListSizes ) + HEAP_NB_SMALL_FREE_LISTS)

typedef union
{
    ARENA_FREE  arena;
    void       *alignment[4];
} FREE_LIST_ENTRY;

struct tagHEAP;

typedef struct tagSUBHEAP
{
    void               *base;       /* Base address of the sub-heap memory block */
    SIZE_T              size;       /* Size of the whole sub-heap */
    SIZE_T              min_commit; /* Minimum committed size */
    SIZE_T              commitSize; /* Committed size of the sub-heap */
    struct list         entry;      /* Entry in sub-heap list */
    struct tagHEAP     *heap;       /* Main heap structure */
    DWORD               headerSize; /* Size of the heap header */
    DWORD               magic;      /* Magic number */
} SUBHEAP;

#define SUBHEAP_MAGIC    ((DWORD)('S' | ('U'<<8) | ('B'<<16) | ('H'<<24)))

typedef struct tagHEAP
{                                  /* win32/win64 */
    DWORD_PTR        unknown1[2];   /* 0000/0000 */
    DWORD            ffeeffee;      /* 0008/0010 */
    DWORD            auto_flags;    /* 000c/0014 */
    DWORD_PTR        unknown2[7];   /* 0010/0018 */
    DWORD            unknown3[2];   /* 002c/0050 */
    DWORD_PTR        unknown4[3];   /* 0034/0058 */
    DWORD            flags;         /* 0040/0070 */
    DWORD            force_flags;   /* 0044/0074 */
    /* end of the Windows 10 compatible struct layout */

    BOOL             shared;        /* System shared heap */
    SUBHEAP          subheap;       /* First sub-heap */
    struct list      entry;         /* Entry in process heap list */
    struct list      subheap_list;  /* Sub-heap list */
    struct list      large_list;    /* Large blocks list */
    SIZE_T           grow_size;     /* Size of next subheap for growing heap */
    DWORD            magic;         /* Magic number */
    DWORD            pending_pos;   /* Position in pending free requests ring */
    ARENA_INUSE    **pending_free;  /* Ring buffer for pending free requests */
    RTL_CRITICAL_SECTION critSection; /* Critical section for serialization */
    FREE_LIST_ENTRY *freeList;      /* Free lists */
} HEAP;

#define HEAP_MAGIC       ((DWORD)('H' | ('E'<<8) | ('A'<<16) | ('P'<<24)))

#define HEAP_DEF_SIZE        0x110000   /* Default heap size = 1Mb + 64Kb */
#define COMMIT_MASK          0xffff  /* bitmask for commit/decommit granularity */
#define MAX_FREE_PENDING     1024    /* max number of free requests to delay */

/* some undocumented flags (names are made up) */
#define HEAP_PRIVATE          0x00001000
#define HEAP_PAGE_ALLOCS      0x01000000
#define HEAP_VALIDATE         0x10000000
#define HEAP_VALIDATE_ALL     0x20000000
#define HEAP_VALIDATE_PARAMS  0x40000000

static HEAP *processHeap;  /* main process heap */

static BOOL HEAP_IsRealArena( HEAP *heapPtr, DWORD flags, LPCVOID block, BOOL quiet );

/* mark a block of memory as free for debugging purposes */
static inline void mark_block_free( void *ptr, SIZE_T size, DWORD flags )
{
    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        SIZE_T i;
        for (i = 0; i < size / sizeof(DWORD); i++) ((DWORD *)ptr)[i] = ARENA_FREE_FILLER;
    }
#if defined(VALGRIND_MAKE_MEM_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_NOACCESS( ptr, size ));
#elif defined( VALGRIND_MAKE_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_NOACCESS( ptr, size ));
#endif
}

/* mark a block of memory as initialized for debugging purposes */
static inline void mark_block_initialized( void *ptr, SIZE_T size )
{
#if defined(VALGRIND_MAKE_MEM_DEFINED)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_DEFINED( ptr, size ));
#elif defined(VALGRIND_MAKE_READABLE)
    VALGRIND_DISCARD( VALGRIND_MAKE_READABLE( ptr, size ));
#endif
}

/* mark a block of memory as uninitialized for debugging purposes */
static inline void mark_block_uninitialized( void *ptr, SIZE_T size )
{
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_UNDEFINED( ptr, size ));
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ));
#endif
}

/* mark a block of memory as a tail block */
static inline void mark_block_tail( void *ptr, SIZE_T size, DWORD flags )
{
    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        mark_block_uninitialized( ptr, size );
        memset( ptr, ARENA_TAIL_FILLER, size );
    }
#if defined(VALGRIND_MAKE_MEM_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_NOACCESS( ptr, size ));
#elif defined( VALGRIND_MAKE_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_NOACCESS( ptr, size ));
#endif
}

/* initialize contents of a newly created block of memory */
static inline void initialize_block( void *ptr, SIZE_T size, SIZE_T unused, DWORD flags )
{
    if (flags & HEAP_ZERO_MEMORY)
    {
        mark_block_initialized( ptr, size );
        memset( ptr, 0, size );
    }
    else
    {
        mark_block_uninitialized( ptr, size );
        if (flags & HEAP_FREE_CHECKING_ENABLED)
        {
            memset( ptr, ARENA_INUSE_FILLER, size );
            mark_block_uninitialized( ptr, size );
        }
    }

    mark_block_tail( (char *)ptr + size, unused, flags );
}

/* notify that a new block of memory has been allocated for debugging purposes */
static inline void notify_alloc( void *ptr, SIZE_T size, BOOL init )
{
#ifdef VALGRIND_MALLOCLIKE_BLOCK
    VALGRIND_MALLOCLIKE_BLOCK( ptr, size, 0, init );
#endif
}

/* notify that a block of memory has been freed for debugging purposes */
static inline void notify_free( void const *ptr )
{
#ifdef VALGRIND_FREELIKE_BLOCK
    VALGRIND_FREELIKE_BLOCK( ptr, 0 );
#endif
}

static inline void notify_realloc( void const *ptr, SIZE_T size_old, SIZE_T size_new )
{
#ifdef VALGRIND_RESIZEINPLACE_BLOCK
    /* zero is not a valid size */
    VALGRIND_RESIZEINPLACE_BLOCK( ptr, size_old ? size_old : 1, size_new ? size_new : 1, 0 );
#endif
}

static void subheap_notify_free_all(SUBHEAP const *subheap)
{
#ifdef VALGRIND_FREELIKE_BLOCK
    char const *ptr = (char const *)subheap->base + subheap->headerSize;

    if (!RUNNING_ON_VALGRIND) return;

    while (ptr < (char const *)subheap->base + subheap->size)
    {
        if (*(const DWORD *)ptr & ARENA_FLAG_FREE)
        {
            ARENA_FREE const *pArena = (ARENA_FREE const *)ptr;
            if (pArena->magic!=ARENA_FREE_MAGIC) ERR("bad free_magic @%p\n", pArena);
            ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
        }
        else
        {
            ARENA_INUSE const *pArena = (ARENA_INUSE const *)ptr;
            if (pArena->magic == ARENA_INUSE_MAGIC) notify_free(pArena + 1);
            else if (pArena->magic != ARENA_PENDING_MAGIC) ERR("bad inuse_magic @%p\n", pArena);
            ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
        }
    }
#endif
}

/* locate a free list entry of the appropriate size */
/* size is the size of the whole block including the arena header */
static inline unsigned int get_freelist_index( SIZE_T size )
{
    unsigned int i;

    if (size <= HEAP_MAX_SMALL_FREE_LIST)
        return (size - HEAP_MIN_ARENA_SIZE) / ALIGNMENT;

    for (i = HEAP_NB_SMALL_FREE_LISTS; i < HEAP_NB_FREE_LISTS - 1; i++)
        if (size <= HEAP_freeListSizes[i - HEAP_NB_SMALL_FREE_LISTS]) break;
    return i;
}

/* get the memory protection type to use for a given heap */
static inline ULONG get_protection_type( DWORD flags )
{
    return (flags & HEAP_CREATE_ENABLE_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
}

static RTL_CRITICAL_SECTION_DEBUG process_heap_critsect_debug =
{
    0, 0, NULL,  /* will be set later */
    { &process_heap_critsect_debug.ProcessLocksList, &process_heap_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": main process heap section") }
};


/***********************************************************************
 *           HEAP_Dump
 */
static void HEAP_Dump( HEAP *heap )
{
    unsigned int i;
    SUBHEAP *subheap;
    char *ptr;

    TRACE( "Heap: %p\n", heap );
    TRACE( "Next: %p  Sub-heaps:", LIST_ENTRY( heap->entry.next, HEAP, entry ) );
    LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry ) TRACE( " %p", subheap );

    TRACE( "\nFree lists:\n Block   Stat   Size    Id\n" );
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        TRACE( "%p free %08lx prev=%p next=%p\n",
                 &heap->freeList[i].arena, i < HEAP_NB_SMALL_FREE_LISTS ?
                 HEAP_MIN_ARENA_SIZE + i * ALIGNMENT : HEAP_freeListSizes[i - HEAP_NB_SMALL_FREE_LISTS],
                 LIST_ENTRY( heap->freeList[i].arena.entry.prev, ARENA_FREE, entry ),
                 LIST_ENTRY( heap->freeList[i].arena.entry.next, ARENA_FREE, entry ));

    LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
    {
        SIZE_T freeSize = 0, usedSize = 0, arenaSize = subheap->headerSize;
        TRACE( "\n\nSub-heap %p: base=%p size=%08lx committed=%08lx\n",
                 subheap, subheap->base, subheap->size, subheap->commitSize );

        TRACE( "\n Block    Arena   Stat   Size    Id\n" );
        ptr = (char *)subheap->base + subheap->headerSize;
        while (ptr < (char *)subheap->base + subheap->size)
        {
            if (*(DWORD *)ptr & ARENA_FLAG_FREE)
            {
                ARENA_FREE *pArena = (ARENA_FREE *)ptr;
                TRACE( "%p %08x free %08x prev=%p next=%p\n",
                         pArena, pArena->magic,
                         pArena->size & ARENA_SIZE_MASK,
                         LIST_ENTRY( pArena->entry.prev, ARENA_FREE, entry ),
                         LIST_ENTRY( pArena->entry.next, ARENA_FREE, entry ) );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_FREE);
                freeSize += pArena->size & ARENA_SIZE_MASK;
            }
            else if (*(DWORD *)ptr & ARENA_FLAG_PREV_FREE)
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                TRACE( "%p %08x Used %08x back=%p\n",
                        pArena, pArena->magic, pArena->size & ARENA_SIZE_MASK, *((ARENA_FREE **)pArena - 1) );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_INUSE);
                usedSize += pArena->size & ARENA_SIZE_MASK;
            }
            else
            {
                ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;
                TRACE( "%p %08x %s %08x\n",
                         pArena, pArena->magic, pArena->magic == ARENA_INUSE_MAGIC ? "used" : "pend",
                         pArena->size & ARENA_SIZE_MASK );
                ptr += sizeof(*pArena) + (pArena->size & ARENA_SIZE_MASK);
                arenaSize += sizeof(ARENA_INUSE);
                usedSize += pArena->size & ARENA_SIZE_MASK;
            }
        }
        TRACE( "\nTotal: Size=%08lx Committed=%08lx Free=%08lx Used=%08lx Arenas=%08lx (%ld%%)\n\n",
	      subheap->size, subheap->commitSize, freeSize, usedSize,
	      arenaSize, (arenaSize * 100) / subheap->size );
    }
}


static void HEAP_DumpEntry( LPPROCESS_HEAP_ENTRY entry )
{
    WORD rem_flags;
    TRACE( "Dumping entry %p\n", entry );
    TRACE( "lpData\t\t: %p\n", entry->lpData );
    TRACE( "cbData\t\t: %08x\n", entry->cbData);
    TRACE( "cbOverhead\t: %08x\n", entry->cbOverhead);
    TRACE( "iRegionIndex\t: %08x\n", entry->iRegionIndex);
    TRACE( "WFlags\t\t: ");
    if (entry->wFlags & PROCESS_HEAP_REGION)
        TRACE( "PROCESS_HEAP_REGION ");
    if (entry->wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
        TRACE( "PROCESS_HEAP_UNCOMMITTED_RANGE ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_BUSY)
        TRACE( "PROCESS_HEAP_ENTRY_BUSY ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE)
        TRACE( "PROCESS_HEAP_ENTRY_MOVEABLE ");
    if (entry->wFlags & PROCESS_HEAP_ENTRY_DDESHARE)
        TRACE( "PROCESS_HEAP_ENTRY_DDESHARE ");
    rem_flags = entry->wFlags &
        ~(PROCESS_HEAP_REGION | PROCESS_HEAP_UNCOMMITTED_RANGE |
          PROCESS_HEAP_ENTRY_BUSY | PROCESS_HEAP_ENTRY_MOVEABLE|
          PROCESS_HEAP_ENTRY_DDESHARE);
    if (rem_flags)
        TRACE( "Unknown %08x", rem_flags);
    TRACE( "\n");
    if ((entry->wFlags & PROCESS_HEAP_ENTRY_BUSY )
        && (entry->wFlags & PROCESS_HEAP_ENTRY_MOVEABLE))
    {
        /* Treat as block */
        TRACE( "BLOCK->hMem\t\t:%p\n", entry->u.Block.hMem);
    }
    if (entry->wFlags & PROCESS_HEAP_REGION)
    {
        TRACE( "Region.dwCommittedSize\t:%08x\n",entry->u.Region.dwCommittedSize);
        TRACE( "Region.dwUnCommittedSize\t:%08x\n",entry->u.Region.dwUnCommittedSize);
        TRACE( "Region.lpFirstBlock\t:%p\n",entry->u.Region.lpFirstBlock);
        TRACE( "Region.lpLastBlock\t:%p\n",entry->u.Region.lpLastBlock);
    }
}

/***********************************************************************
 *           HEAP_GetPtr
 * RETURNS
 *	Pointer to the heap
 *	NULL: Failure
 */
static HEAP *HEAP_GetPtr(
             HANDLE heap /* [in] Handle to the heap */
) {
    HEAP *heapPtr = heap;
    if (!heapPtr || (heapPtr->magic != HEAP_MAGIC))
    {
        ERR("Invalid heap %p!\n", heap );
        return NULL;
    }
    if ((heapPtr->flags & HEAP_VALIDATE_ALL) && !HEAP_IsRealArena( heapPtr, 0, NULL, NOISY ))
    {
        if (TRACE_ON(heap))
        {
            HEAP_Dump( heapPtr );
            assert( FALSE );
        }
        return NULL;
    }
    return heapPtr;
}


/***********************************************************************
 *           HEAP_InsertFreeBlock
 *
 * Insert a free block into the free list.
 */
static inline void HEAP_InsertFreeBlock( HEAP *heap, ARENA_FREE *pArena, BOOL last )
{
    FREE_LIST_ENTRY *pEntry = heap->freeList + get_freelist_index( pArena->size + sizeof(*pArena) );
    if (last)
    {
        /* insert at end of free list, i.e. before the next free list entry */
        pEntry++;
        if (pEntry == &heap->freeList[HEAP_NB_FREE_LISTS]) pEntry = heap->freeList;
        list_add_before( &pEntry->arena.entry, &pArena->entry );
    }
    else
    {
        /* insert at head of free list */
        list_add_after( &pEntry->arena.entry, &pArena->entry );
    }
    pArena->size |= ARENA_FLAG_FREE;
}


/***********************************************************************
 *           HEAP_FindSubHeap
 * Find the sub-heap containing a given address.
 *
 * RETURNS
 *	Pointer: Success
 *	NULL: Failure
 */
static SUBHEAP *HEAP_FindSubHeap(
                const HEAP *heap, /* [in] Heap pointer */
                LPCVOID ptr ) /* [in] Address */
{
    SUBHEAP *sub;
    LIST_FOR_EACH_ENTRY( sub, &heap->subheap_list, SUBHEAP, entry )
        if ((ptr >= sub->base) &&
            ((const char *)ptr < (const char *)sub->base + sub->size - sizeof(ARENA_INUSE)))
            return sub;
    return NULL;
}


/***********************************************************************
 *           HEAP_Commit
 *
 * Make sure the heap storage is committed for a given size in the specified arena.
 */
static inline BOOL HEAP_Commit( SUBHEAP *subheap, ARENA_INUSE *pArena, SIZE_T data_size )
{
    void *ptr = (char *)(pArena + 1) + data_size + sizeof(ARENA_FREE);
    SIZE_T size = (char *)ptr - (char *)subheap->base;
    size = (size + COMMIT_MASK) & ~COMMIT_MASK;
    if (size > subheap->size) size = subheap->size;
    if (size <= subheap->commitSize) return TRUE;
    size -= subheap->commitSize;
    ptr = (char *)subheap->base + subheap->commitSize;
    if (NtAllocateVirtualMemory( NtCurrentProcess(), &ptr, 0,
                                 &size, MEM_COMMIT, get_protection_type( subheap->heap->flags ) ))
    {
        WARN("Could not commit %08lx bytes at %p for heap %p\n",
                 size, ptr, subheap->heap );
        return FALSE;
    }
    subheap->commitSize += size;
    return TRUE;
}


/***********************************************************************
 *           HEAP_Decommit
 *
 * If possible, decommit the heap storage from (including) 'ptr'.
 */
static inline BOOL HEAP_Decommit( SUBHEAP *subheap, void *ptr )
{
    void *addr;
    SIZE_T decommit_size;
    SIZE_T size = (char *)ptr - (char *)subheap->base;

    /* round to next block and add one full block */
    size = ((size + COMMIT_MASK) & ~COMMIT_MASK) + COMMIT_MASK + 1;
    size = max( size, subheap->min_commit );
    if (size >= subheap->commitSize) return TRUE;
    decommit_size = subheap->commitSize - size;
    addr = (char *)subheap->base + size;

    if (NtFreeVirtualMemory( NtCurrentProcess(), &addr, &decommit_size, MEM_DECOMMIT ))
    {
        WARN("Could not decommit %08lx bytes at %p for heap %p\n",
             decommit_size, (char *)subheap->base + size, subheap->heap );
        return FALSE;
    }
    subheap->commitSize -= decommit_size;
    return TRUE;
}


/***********************************************************************
 *           HEAP_CreateFreeBlock
 *
 * Create a free block at a specified address. 'size' is the size of the
 * whole block, including the new arena.
 */
static void HEAP_CreateFreeBlock( SUBHEAP *subheap, void *ptr, SIZE_T size )
{
    ARENA_FREE *pFree;
    char *pEnd;
    BOOL last;
    DWORD flags = subheap->heap->flags;

    /* Create a free arena */
    mark_block_uninitialized( ptr, sizeof(ARENA_FREE) );
    pFree = ptr;
    pFree->magic = ARENA_FREE_MAGIC;

    /* If debugging, erase the freed block content */

    pEnd = (char *)ptr + size;
    if (pEnd > (char *)subheap->base + subheap->commitSize)
        pEnd = (char *)subheap->base + subheap->commitSize;
    if (pEnd > (char *)(pFree + 1)) mark_block_free( pFree + 1, pEnd - (char *)(pFree + 1), flags );

    /* Check if next block is free also */

    if (((char *)ptr + size < (char *)subheap->base + subheap->size) &&
        (*(DWORD *)((char *)ptr + size) & ARENA_FLAG_FREE))
    {
        /* Remove the next arena from the free list */
        ARENA_FREE *pNext = (ARENA_FREE *)((char *)ptr + size);
        list_remove( &pNext->entry );
        size += (pNext->size & ARENA_SIZE_MASK) + sizeof(*pNext);
        mark_block_free( pNext, sizeof(ARENA_FREE), flags );
    }

    /* Set the next block PREV_FREE flag and pointer */

    last = ((char *)ptr + size >= (char *)subheap->base + subheap->size);
    if (!last)
    {
        DWORD *pNext = (DWORD *)((char *)ptr + size);
        *pNext |= ARENA_FLAG_PREV_FREE;
        mark_block_initialized( (ARENA_FREE **)pNext - 1, sizeof( ARENA_FREE * ) );
        *((ARENA_FREE **)pNext - 1) = pFree;
    }

    /* Last, insert the new block into the free list */

    pFree->size = size - sizeof(*pFree);
    HEAP_InsertFreeBlock( subheap->heap, pFree, last );
}


/***********************************************************************
 *           HEAP_MakeInUseBlockFree
 *
 * Turn an in-use block into a free block. Can also decommit the end of
 * the heap, and possibly even free the sub-heap altogether.
 */
static void HEAP_MakeInUseBlockFree( SUBHEAP *subheap, ARENA_INUSE *pArena )
{
    HEAP *heap = subheap->heap;
    ARENA_FREE *pFree;
    SIZE_T size;

    if (heap->pending_free)
    {
        ARENA_INUSE *prev = heap->pending_free[heap->pending_pos];
        heap->pending_free[heap->pending_pos] = pArena;
        heap->pending_pos = (heap->pending_pos + 1) % MAX_FREE_PENDING;
        pArena->magic = ARENA_PENDING_MAGIC;
        mark_block_free( pArena + 1, pArena->size & ARENA_SIZE_MASK, heap->flags );
        if (!prev) return;
        pArena = prev;
        subheap = HEAP_FindSubHeap( heap, pArena );
    }

    /* Check if we can merge with previous block */

    size = (pArena->size & ARENA_SIZE_MASK) + sizeof(*pArena);
    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        pFree = *((ARENA_FREE **)pArena - 1);
        size += (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
        /* Remove it from the free list */
        list_remove( &pFree->entry );
    }
    else pFree = (ARENA_FREE *)pArena;

    /* Create a free block */

    HEAP_CreateFreeBlock( subheap, pFree, size );
    size = (pFree->size & ARENA_SIZE_MASK) + sizeof(ARENA_FREE);
    if ((char *)pFree + size < (char *)subheap->base + subheap->size)
        return;  /* Not the last block, so nothing more to do */

    /* Free the whole sub-heap if it's empty and not the original one */

    if (((char *)pFree == (char *)subheap->base + subheap->headerSize) &&
        (subheap != &subheap->heap->subheap))
    {
        void *addr = subheap->base;

        size = 0;
        /* Remove the free block from the list */
        list_remove( &pFree->entry );
        /* Remove the subheap from the list */
        list_remove( &subheap->entry );
        /* Free the memory */
        subheap->magic = 0;
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
        return;
    }

    /* Decommit the end of the heap */

    if (!(subheap->heap->shared)) HEAP_Decommit( subheap, pFree + 1 );
}


/***********************************************************************
 *           HEAP_ShrinkBlock
 *
 * Shrink an in-use block.
 */
static void HEAP_ShrinkBlock(SUBHEAP *subheap, ARENA_INUSE *pArena, SIZE_T size)
{
    if ((pArena->size & ARENA_SIZE_MASK) >= size + HEAP_MIN_SHRINK_SIZE)
    {
        HEAP_CreateFreeBlock( subheap, (char *)(pArena + 1) + size,
                              (pArena->size & ARENA_SIZE_MASK) - size );
	/* assign size plus previous arena flags */
        pArena->size = size | (pArena->size & ~ARENA_SIZE_MASK);
    }
    else
    {
        /* Turn off PREV_FREE flag in next block */
        char *pNext = (char *)(pArena + 1) + (pArena->size & ARENA_SIZE_MASK);
        if (pNext < (char *)subheap->base + subheap->size)
            *(DWORD *)pNext &= ~ARENA_FLAG_PREV_FREE;
    }
}


/***********************************************************************
 *           allocate_large_block
 */
static void *allocate_large_block( HEAP *heap, DWORD flags, SIZE_T size )
{
    ARENA_LARGE *arena;
    SIZE_T block_size = sizeof(*arena) + ROUND_SIZE(size) + HEAP_TAIL_EXTRA_SIZE(flags);
    LPVOID address = NULL;

    if (block_size < size) return NULL;  /* overflow */
    if (NtAllocateVirtualMemory( NtCurrentProcess(), &address, 0, &block_size,
                                 MEM_COMMIT, get_protection_type( flags )))
    {
        WARN("Could not allocate block for %08lx bytes\n", size );
        return NULL;
    }
    arena = address;
    arena->data_size = size;
    arena->block_size = block_size;
    arena->size = ARENA_LARGE_SIZE;
    arena->magic = ARENA_LARGE_MAGIC;
    mark_block_tail( (char *)(arena + 1) + size, block_size - sizeof(*arena) - size, flags );
    list_add_tail( &heap->large_list, &arena->entry );
    notify_alloc( arena + 1, size, flags & HEAP_ZERO_MEMORY );
    return arena + 1;
}


/***********************************************************************
 *           free_large_block
 */
static void free_large_block( HEAP *heap, DWORD flags, void *ptr )
{
    ARENA_LARGE *arena = (ARENA_LARGE *)ptr - 1;
    LPVOID address = arena;
    SIZE_T size = 0;

    list_remove( &arena->entry );
    NtFreeVirtualMemory( NtCurrentProcess(), &address, &size, MEM_RELEASE );
}


/***********************************************************************
 *           realloc_large_block
 */
static void *realloc_large_block( HEAP *heap, DWORD flags, void *ptr, SIZE_T size )
{
    ARENA_LARGE *arena = (ARENA_LARGE *)ptr - 1;
    void *new_ptr;

    if (arena->block_size - sizeof(*arena) >= size)
    {
        SIZE_T unused = arena->block_size - sizeof(*arena) - size;

        /* FIXME: we could remap zero-pages instead */
#ifdef VALGRIND_RESIZEINPLACE_BLOCK
        if (RUNNING_ON_VALGRIND)
            notify_realloc( arena + 1, arena->data_size, size );
        else
#endif
        if (size > arena->data_size)
            initialize_block( (char *)ptr + arena->data_size, size - arena->data_size, unused, flags );
        else
            mark_block_tail( (char *)ptr + size, unused, flags );
        arena->data_size = size;
        return ptr;
    }
    if (flags & HEAP_REALLOC_IN_PLACE_ONLY) return NULL;
    if (!(new_ptr = allocate_large_block( heap, flags, size )))
    {
        WARN("Could not allocate block for %08lx bytes\n", size );
        return NULL;
    }
    memcpy( new_ptr, ptr, arena->data_size );
    free_large_block( heap, flags, ptr );
    notify_free( ptr );
    return new_ptr;
}


/***********************************************************************
 *           find_large_block
 */
static ARENA_LARGE *find_large_block( HEAP *heap, const void *ptr )
{
    ARENA_LARGE *arena;

    LIST_FOR_EACH_ENTRY( arena, &heap->large_list, ARENA_LARGE, entry )
        if (ptr == arena + 1) return arena;

    return NULL;
}


/***********************************************************************
 *           validate_large_arena
 */
static BOOL validate_large_arena( HEAP *heap, const ARENA_LARGE *arena, BOOL quiet )
{
    DWORD flags = heap->flags;

    if ((ULONG_PTR)arena % page_size)
    {
        if (quiet == NOISY)
        {
            ERR( "Heap %p: invalid large arena pointer %p\n", heap, arena );
            if (TRACE_ON(heap)) HEAP_Dump( heap );
        }
        else if (WARN_ON(heap))
        {
            WARN( "Heap %p: unaligned arena pointer %p\n", heap, arena );
            if (TRACE_ON(heap)) HEAP_Dump( heap );
        }
        return FALSE;
    }
    if (arena->size != ARENA_LARGE_SIZE || arena->magic != ARENA_LARGE_MAGIC)
    {
        if (quiet == NOISY)
        {
            ERR( "Heap %p: invalid large arena %p values %x/%x\n",
                 heap, arena, arena->size, arena->magic );
            if (TRACE_ON(heap)) HEAP_Dump( heap );
        }
        else if (WARN_ON(heap))
        {
            WARN( "Heap %p: invalid large arena %p values %x/%x\n",
                  heap, arena, arena->size, arena->magic );
            if (TRACE_ON(heap)) HEAP_Dump( heap );
        }
        return FALSE;
    }
    if (arena->data_size > arena->block_size - sizeof(*arena))
    {
        ERR( "Heap %p: invalid large arena %p size %lx/%lx\n",
             heap, arena, arena->data_size, arena->block_size );
        return FALSE;
    }
    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        SIZE_T i, unused = arena->block_size - sizeof(*arena) - arena->data_size;
        const unsigned char *data = (const unsigned char *)(arena + 1) + arena->data_size;

        for (i = 0; i < unused; i++)
        {
            if (data[i] == ARENA_TAIL_FILLER) continue;
            ERR("Heap %p: block %p tail overwritten at %p (byte %lu/%lu == 0x%02x)\n",
                heap, arena + 1, data + i, i, unused, data[i] );
            return FALSE;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           HEAP_CreateSubHeap
 */
static SUBHEAP *HEAP_CreateSubHeap( HEAP *heap, LPVOID address, DWORD flags,
                                    SIZE_T commitSize, SIZE_T totalSize )
{
    SUBHEAP *subheap;
    FREE_LIST_ENTRY *pEntry;
    unsigned int i;

    if (!address)
    {
        if (!commitSize) commitSize = COMMIT_MASK + 1;
        totalSize = min( totalSize, 0xffff0000 );  /* don't allow a heap larger than 4GB */
        if (totalSize < commitSize) totalSize = commitSize;
        if (flags & HEAP_SHARED) commitSize = totalSize;  /* always commit everything in a shared heap */
        commitSize = min( totalSize, (commitSize + COMMIT_MASK) & ~COMMIT_MASK );

        /* allocate the memory block */
        if (NtAllocateVirtualMemory( NtCurrentProcess(), &address, 0, &totalSize,
                                     MEM_RESERVE, get_protection_type( flags ) ))
        {
            WARN("Could not allocate %08lx bytes\n", totalSize );
            return NULL;
        }
        if (NtAllocateVirtualMemory( NtCurrentProcess(), &address, 0,
                                     &commitSize, MEM_COMMIT, get_protection_type( flags ) ))
        {
            WARN("Could not commit %08lx bytes for sub-heap %p\n", commitSize, address );
            return NULL;
        }
    }

    if (heap)
    {
        /* If this is a secondary subheap, insert it into list */

        subheap = address;
        subheap->base       = address;
        subheap->heap       = heap;
        subheap->size       = totalSize;
        subheap->min_commit = 0x10000;
        subheap->commitSize = commitSize;
        subheap->magic      = SUBHEAP_MAGIC;
        subheap->headerSize = ROUND_SIZE( sizeof(SUBHEAP) );
        list_add_head( &heap->subheap_list, &subheap->entry );
    }
    else
    {
        /* If this is a primary subheap, initialize main heap */

        heap = address;
        heap->ffeeffee      = 0xffeeffee;
        heap->auto_flags    = (flags & HEAP_GROWABLE);
        heap->flags         = (flags & ~HEAP_SHARED);
        heap->shared        = (flags & HEAP_SHARED) != 0;
        heap->magic         = HEAP_MAGIC;
        heap->grow_size     = max( HEAP_DEF_SIZE, totalSize );
        list_init( &heap->subheap_list );
        list_init( &heap->large_list );

        subheap = &heap->subheap;
        subheap->base       = address;
        subheap->heap       = heap;
        subheap->size       = totalSize;
        subheap->min_commit = commitSize;
        subheap->commitSize = commitSize;
        subheap->magic      = SUBHEAP_MAGIC;
        subheap->headerSize = ROUND_SIZE( sizeof(HEAP) );
        list_add_head( &heap->subheap_list, &subheap->entry );

        /* Build the free lists */

        heap->freeList = (FREE_LIST_ENTRY *)((char *)heap + subheap->headerSize);
        subheap->headerSize += HEAP_NB_FREE_LISTS * sizeof(FREE_LIST_ENTRY);
        list_init( &heap->freeList[0].arena.entry );
        for (i = 0, pEntry = heap->freeList; i < HEAP_NB_FREE_LISTS; i++, pEntry++)
        {
            pEntry->arena.size = 0 | ARENA_FLAG_FREE;
            pEntry->arena.magic = ARENA_FREE_MAGIC;
            if (i) list_add_after( &pEntry[-1].arena.entry, &pEntry->arena.entry );
        }

        /* Initialize critical section */

        if (!processHeap)  /* do it by hand to avoid memory allocations */
        {
            heap->critSection.DebugInfo      = &process_heap_critsect_debug;
            heap->critSection.LockCount      = -1;
            heap->critSection.RecursionCount = 0;
            heap->critSection.OwningThread   = 0;
            heap->critSection.LockSemaphore  = 0;
            heap->critSection.SpinCount      = 0;
            process_heap_critsect_debug.CriticalSection = &heap->critSection;
        }
        else
        {
            RtlInitializeCriticalSection( &heap->critSection );
            heap->critSection.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": HEAP.critSection");
        }

        if (heap->shared)
        {
            /* let's assume that only one thread at a time will try to do this */
            HANDLE sem = heap->critSection.LockSemaphore;
            if (!sem) NtCreateSemaphore( &sem, SEMAPHORE_ALL_ACCESS, NULL, 0, 1 );

            NtDuplicateObject( NtCurrentProcess(), sem, NtCurrentProcess(), &sem, 0, 0,
                               DUPLICATE_MAKE_GLOBAL | DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE );
            heap->critSection.LockSemaphore = sem;
            RtlFreeHeap( processHeap, 0, heap->critSection.DebugInfo );
            heap->critSection.DebugInfo = NULL;
        }
    }

    /* Create the first free block */

    HEAP_CreateFreeBlock( subheap, (LPBYTE)subheap->base + subheap->headerSize,
                          subheap->size - subheap->headerSize );

    return subheap;
}


/***********************************************************************
 *           HEAP_FindFreeBlock
 *
 * Find a free block at least as large as the requested size, and make sure
 * the requested size is committed.
 */
static ARENA_FREE *HEAP_FindFreeBlock( HEAP *heap, SIZE_T size,
                                       SUBHEAP **ppSubHeap )
{
    SUBHEAP *subheap;
    struct list *ptr;
    SIZE_T total_size;
    FREE_LIST_ENTRY *pEntry = heap->freeList + get_freelist_index( size + sizeof(ARENA_INUSE) );

    /* Find a suitable free list, and in it find a block large enough */

    ptr = &pEntry->arena.entry;
    while ((ptr = list_next( &heap->freeList[0].arena.entry, ptr )))
    {
        ARENA_FREE *pArena = LIST_ENTRY( ptr, ARENA_FREE, entry );
        SIZE_T arena_size = (pArena->size & ARENA_SIZE_MASK) +
                            sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
        if (arena_size >= size)
        {
            subheap = HEAP_FindSubHeap( heap, pArena );
            if (!HEAP_Commit( subheap, (ARENA_INUSE *)pArena, size )) return NULL;
            *ppSubHeap = subheap;
            return pArena;
        }
    }

    /* If no block was found, attempt to grow the heap */

    if (!(heap->flags & HEAP_GROWABLE))
    {
        WARN("Not enough space in heap %p for %08lx bytes\n", heap, size );
        return NULL;
    }
    /* make sure that we have a big enough size *committed* to fit another
     * last free arena in !
     * So just one heap struct, one first free arena which will eventually
     * get used, and a second free arena that might get assigned all remaining
     * free space in HEAP_ShrinkBlock() */
    total_size = size + ROUND_SIZE(sizeof(SUBHEAP)) + sizeof(ARENA_INUSE) + sizeof(ARENA_FREE);
    if (total_size < size) return NULL;  /* overflow */

    if ((subheap = HEAP_CreateSubHeap( heap, NULL, heap->flags, total_size,
                                       max( heap->grow_size, total_size ) )))
    {
        if (heap->grow_size < 128 * 1024 * 1024) heap->grow_size *= 2;
    }
    else while (!subheap)  /* shrink the grow size again if we are running out of space */
    {
        if (heap->grow_size <= total_size || heap->grow_size <= 4 * 1024 * 1024) return NULL;
        heap->grow_size /= 2;
        subheap = HEAP_CreateSubHeap( heap, NULL, heap->flags, total_size,
                                      max( heap->grow_size, total_size ) );
    }

    TRACE("created new sub-heap %p of %08lx bytes for heap %p\n",
          subheap, subheap->size, heap );

    *ppSubHeap = subheap;
    return (ARENA_FREE *)((char *)subheap->base + subheap->headerSize);
}


/***********************************************************************
 *           HEAP_IsValidArenaPtr
 *
 * Check that the pointer is inside the range possible for arenas.
 */
static BOOL HEAP_IsValidArenaPtr( const HEAP *heap, const ARENA_FREE *ptr )
{
    unsigned int i;
    const SUBHEAP *subheap = HEAP_FindSubHeap( heap, ptr );
    if (!subheap) return FALSE;
    if ((const char *)ptr >= (const char *)subheap->base + subheap->headerSize) return TRUE;
    if (subheap != &heap->subheap) return FALSE;
    for (i = 0; i < HEAP_NB_FREE_LISTS; i++)
        if (ptr == &heap->freeList[i].arena) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           HEAP_ValidateFreeArena
 */
static BOOL HEAP_ValidateFreeArena( SUBHEAP *subheap, ARENA_FREE *pArena )
{
    DWORD flags = subheap->heap->flags;
    SIZE_T size;
    ARENA_FREE *prev, *next;
    char *heapEnd = (char *)subheap->base + subheap->size;

    /* Check for unaligned pointers */
    if ((ULONG_PTR)pArena % ALIGNMENT != ARENA_OFFSET)
    {
        ERR("Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
        return FALSE;
    }

    /* Check magic number */
    if (pArena->magic != ARENA_FREE_MAGIC)
    {
        ERR("Heap %p: invalid free arena magic %08x for %p\n", subheap->heap, pArena->magic, pArena );
        return FALSE;
    }
    /* Check size flags */
    if (!(pArena->size & ARENA_FLAG_FREE) ||
        (pArena->size & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %p: bad flags %08x for free arena %p\n",
            subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check arena size */
    size = pArena->size & ARENA_SIZE_MASK;
    if ((char *)(pArena + 1) + size > heapEnd)
    {
        ERR("Heap %p: bad size %08lx for free arena %p\n", subheap->heap, size, pArena );
        return FALSE;
    }
    /* Check that next pointer is valid */
    next = LIST_ENTRY( pArena->entry.next, ARENA_FREE, entry );
    if (!HEAP_IsValidArenaPtr( subheap->heap, next ))
    {
        ERR("Heap %p: bad next ptr %p for arena %p\n",
            subheap->heap, next, pArena );
        return FALSE;
    }
    /* Check that next arena is free */
    if (!(next->size & ARENA_FLAG_FREE) || (next->magic != ARENA_FREE_MAGIC))
    {
        ERR("Heap %p: next arena %p invalid for %p\n",
            subheap->heap, next, pArena );
        return FALSE;
    }
    /* Check that prev pointer is valid */
    prev = LIST_ENTRY( pArena->entry.prev, ARENA_FREE, entry );
    if (!HEAP_IsValidArenaPtr( subheap->heap, prev ))
    {
        ERR("Heap %p: bad prev ptr %p for arena %p\n",
            subheap->heap, prev, pArena );
        return FALSE;
    }
    /* Check that prev arena is free */
    if (!(prev->size & ARENA_FLAG_FREE) || (prev->magic != ARENA_FREE_MAGIC))
    {
	/* this often means that the prev arena got overwritten
	 * by a memory write before that prev arena */
        ERR("Heap %p: prev arena %p invalid for %p\n",
            subheap->heap, prev, pArena );
        return FALSE;
    }
    /* Check that next block has PREV_FREE flag */
    if ((char *)(pArena + 1) + size < heapEnd)
    {
        if (!(*(DWORD *)((char *)(pArena + 1) + size) & ARENA_FLAG_PREV_FREE))
        {
            ERR("Heap %p: free arena %p next block has no PREV_FREE flag\n",
                subheap->heap, pArena );
            return FALSE;
        }
        /* Check next block back pointer */
        if (*((ARENA_FREE **)((char *)(pArena + 1) + size) - 1) != pArena)
        {
            ERR("Heap %p: arena %p has wrong back ptr %p\n",
                subheap->heap, pArena,
                *((ARENA_FREE **)((char *)(pArena+1) + size) - 1));
            return FALSE;
        }
    }
    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        DWORD *ptr = (DWORD *)(pArena + 1);
        char *end = (char *)(pArena + 1) + size;

        if (end >= heapEnd) end = (char *)subheap->base + subheap->commitSize;
        else end -= sizeof(ARENA_FREE *);
        while (ptr < (DWORD *)end)
        {
            if (*ptr != ARENA_FREE_FILLER)
            {
                ERR("Heap %p: free block %p overwritten at %p by %08x\n",
                    subheap->heap, (ARENA_INUSE *)pArena + 1, ptr, *ptr );
                return FALSE;
            }
            ptr++;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           HEAP_ValidateInUseArena
 */
static BOOL HEAP_ValidateInUseArena( const SUBHEAP *subheap, const ARENA_INUSE *pArena, BOOL quiet )
{
    SIZE_T size;
    DWORD i, flags = subheap->heap->flags;
    const char *heapEnd = (const char *)subheap->base + subheap->size;

    /* Check for unaligned pointers */
    if ((ULONG_PTR)pArena % ALIGNMENT != ARENA_OFFSET)
    {
        if ( quiet == NOISY )
        {
            ERR( "Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
            if ( TRACE_ON(heap) )
                HEAP_Dump( subheap->heap );
        }
        else if ( WARN_ON(heap) )
        {
            WARN( "Heap %p: unaligned arena pointer %p\n", subheap->heap, pArena );
            if ( TRACE_ON(heap) )
                HEAP_Dump( subheap->heap );
        }
        return FALSE;
    }

    /* Check magic number */
    if (pArena->magic != ARENA_INUSE_MAGIC && pArena->magic != ARENA_PENDING_MAGIC)
    {
        if (quiet == NOISY) {
            ERR("Heap %p: invalid in-use arena magic %08x for %p\n", subheap->heap, pArena->magic, pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }  else if (WARN_ON(heap)) {
            WARN("Heap %p: invalid in-use arena magic %08x for %p\n", subheap->heap, pArena->magic, pArena );
            if (TRACE_ON(heap))
               HEAP_Dump( subheap->heap );
        }
        return FALSE;
    }
    /* Check size flags */
    if (pArena->size & ARENA_FLAG_FREE)
    {
        ERR("Heap %p: bad flags %08x for in-use arena %p\n",
            subheap->heap, pArena->size & ~ARENA_SIZE_MASK, pArena );
        return FALSE;
    }
    /* Check arena size */
    size = pArena->size & ARENA_SIZE_MASK;
    if ((const char *)(pArena + 1) + size > heapEnd ||
        (const char *)(pArena + 1) + size < (const char *)(pArena + 1))
    {
        ERR("Heap %p: bad size %08lx for in-use arena %p\n", subheap->heap, size, pArena );
        return FALSE;
    }
    /* Check next arena PREV_FREE flag */
    if (((const char *)(pArena + 1) + size < heapEnd) &&
        (*(const DWORD *)((const char *)(pArena + 1) + size) & ARENA_FLAG_PREV_FREE))
    {
        ERR("Heap %p: in-use arena %p next block %p has PREV_FREE flag %x\n",
            subheap->heap, pArena, (const char *)(pArena + 1) + size,*(const DWORD *)((const char *)(pArena + 1) + size) );
        return FALSE;
    }
    /* Check prev free arena */
    if (pArena->size & ARENA_FLAG_PREV_FREE)
    {
        const ARENA_FREE *pPrev = *((const ARENA_FREE * const*)pArena - 1);
        /* Check prev pointer */
        if (!HEAP_IsValidArenaPtr( subheap->heap, pPrev ))
        {
            ERR("Heap %p: bad back ptr %p for arena %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
        /* Check that prev arena is free */
        if (!(pPrev->size & ARENA_FLAG_FREE) ||
            (pPrev->magic != ARENA_FREE_MAGIC))
        {
            ERR("Heap %p: prev arena %p invalid for in-use %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
        /* Check that prev arena is really the previous block */
        if ((const char *)(pPrev + 1) + (pPrev->size & ARENA_SIZE_MASK) != (const char *)pArena)
        {
            ERR("Heap %p: prev arena %p is not prev for in-use %p\n",
                subheap->heap, pPrev, pArena );
            return FALSE;
        }
    }
    /* Check unused size */
    if (pArena->unused_bytes > size)
    {
        ERR("Heap %p: invalid unused size %08x/%08lx\n", subheap->heap, pArena->unused_bytes, size );
        return FALSE;
    }
    /* Check unused bytes */
    if (pArena->magic == ARENA_PENDING_MAGIC)
    {
        const DWORD *ptr = (const DWORD *)(pArena + 1);
        const DWORD *end = (const DWORD *)((const char *)ptr + size);

        while (ptr < end)
        {
            if (*ptr != ARENA_FREE_FILLER)
            {
                ERR("Heap %p: free block %p overwritten at %p by %08x\n",
                    subheap->heap, pArena + 1, ptr, *ptr );
                if (!*ptr) { HEAP_Dump( subheap->heap ); DbgBreakPoint(); }
                return FALSE;
            }
            ptr++;
        }
    }
    else if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        const unsigned char *data = (const unsigned char *)(pArena + 1) + size - pArena->unused_bytes;

        for (i = 0; i < pArena->unused_bytes; i++)
        {
            if (data[i] == ARENA_TAIL_FILLER) continue;
            ERR("Heap %p: block %p tail overwritten at %p (byte %u/%u == 0x%02x)\n",
                subheap->heap, pArena + 1, data + i, i, pArena->unused_bytes, data[i] );
            return FALSE;
        }
    }
    return TRUE;
}


/***********************************************************************
 *           HEAP_IsRealArena  [Internal]
 * Validates a block is a valid arena.
 *
 * RETURNS
 *	TRUE: Success
 *	FALSE: Failure
 */
static BOOL HEAP_IsRealArena( HEAP *heapPtr,   /* [in] ptr to the heap */
              DWORD flags,   /* [in] Bit flags that control access during operation */
              LPCVOID block, /* [in] Optional pointer to memory block to validate */
              BOOL quiet )   /* [in] Flag - if true, HEAP_ValidateInUseArena
                              *             does not complain    */
{
    SUBHEAP *subheap;
    BOOL ret = FALSE;
    const ARENA_LARGE *large_arena;

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    /* calling HeapLock may result in infinite recursion, so do the critsect directly */
    if (!(flags & HEAP_NO_SERIALIZE))
        RtlEnterCriticalSection( &heapPtr->critSection );

    if (block)  /* only check this single memory block */
    {
        const ARENA_INUSE *arena = (const ARENA_INUSE *)block - 1;

        if (!(subheap = HEAP_FindSubHeap( heapPtr, arena )) ||
            ((const char *)arena < (char *)subheap->base + subheap->headerSize))
        {
            if (!(large_arena = find_large_block( heapPtr, block )))
            {
                if (quiet == NOISY)
                    ERR("Heap %p: block %p is not inside heap\n", heapPtr, block );
                else if (WARN_ON(heap))
                    WARN("Heap %p: block %p is not inside heap\n", heapPtr, block );
            }
            else ret = validate_large_arena( heapPtr, large_arena, quiet );
        }
        else ret = HEAP_ValidateInUseArena( subheap, arena, quiet );
        goto done;
    }

    LIST_FOR_EACH_ENTRY( subheap, &heapPtr->subheap_list, SUBHEAP, entry )
    {
        char *ptr = (char *)subheap->base + subheap->headerSize;
        while (ptr < (char *)subheap->base + subheap->size)
        {
            if (*(DWORD *)ptr & ARENA_FLAG_FREE)
            {
                if (!HEAP_ValidateFreeArena( subheap, (ARENA_FREE *)ptr )) goto done;
                ptr += sizeof(ARENA_FREE) + (*(DWORD *)ptr & ARENA_SIZE_MASK);
            }
            else
            {
                if (!HEAP_ValidateInUseArena( subheap, (ARENA_INUSE *)ptr, NOISY )) goto done;
                ptr += sizeof(ARENA_INUSE) + (*(DWORD *)ptr & ARENA_SIZE_MASK);
            }
        }
    }

    LIST_FOR_EACH_ENTRY( large_arena, &heapPtr->large_list, ARENA_LARGE, entry )
        if (!validate_large_arena( heapPtr, large_arena, quiet )) goto done;

    ret = TRUE;

done:
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    return ret;
}


/***********************************************************************
 *           validate_block_pointer
 *
 * Minimum validation needed to catch bad parameters in heap functions.
 */
static BOOL validate_block_pointer( HEAP *heap, SUBHEAP **ret_subheap, const ARENA_INUSE *arena )
{
    SUBHEAP *subheap;
    BOOL ret = FALSE;

    if (!(*ret_subheap = subheap = HEAP_FindSubHeap( heap, arena )))
    {
        ARENA_LARGE *large_arena = find_large_block( heap, arena + 1 );

        if (!large_arena)
        {
            WARN( "Heap %p: pointer %p is not inside heap\n", heap, arena + 1 );
            return FALSE;
        }
        if ((heap->flags & HEAP_VALIDATE) && !validate_large_arena( heap, large_arena, QUIET ))
            return FALSE;
        return TRUE;
    }

    if ((const char *)arena < (char *)subheap->base + subheap->headerSize)
        WARN( "Heap %p: pointer %p is inside subheap %p header\n", subheap->heap, arena + 1, subheap );
    else if (subheap->heap->flags & HEAP_VALIDATE)  /* do the full validation */
        ret = HEAP_ValidateInUseArena( subheap, arena, QUIET );
    else if ((ULONG_PTR)arena % ALIGNMENT != ARENA_OFFSET)
        WARN( "Heap %p: unaligned arena pointer %p\n", subheap->heap, arena );
    else if (arena->magic == ARENA_PENDING_MAGIC)
        WARN( "Heap %p: block %p used after free\n", subheap->heap, arena + 1 );
    else if (arena->magic != ARENA_INUSE_MAGIC)
        WARN( "Heap %p: invalid in-use arena magic %08x for %p\n", subheap->heap, arena->magic, arena );
    else if (arena->size & ARENA_FLAG_FREE)
        ERR( "Heap %p: bad flags %08x for in-use arena %p\n",
             subheap->heap, arena->size & ~ARENA_SIZE_MASK, arena );
    else if ((const char *)(arena + 1) + (arena->size & ARENA_SIZE_MASK) > (const char *)subheap->base + subheap->size ||
             (const char *)(arena + 1) + (arena->size & ARENA_SIZE_MASK) < (const char *)(arena + 1))
        ERR( "Heap %p: bad size %08x for in-use arena %p\n",
             subheap->heap, arena->size & ARENA_SIZE_MASK, arena );
    else
        ret = TRUE;

    return ret;
}

static DWORD heap_flags_from_global_flag( DWORD flag )
{
    DWORD ret = 0;

    if (flag & FLG_HEAP_ENABLE_TAIL_CHECK)
        ret |= HEAP_TAIL_CHECKING_ENABLED;
    if (flag & FLG_HEAP_ENABLE_FREE_CHECK)
        ret |= HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_PARAMETERS)
        ret |= HEAP_VALIDATE_PARAMS | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_VALIDATE_ALL)
        ret |= HEAP_VALIDATE_ALL | HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED;
    if (flag & FLG_HEAP_DISABLE_COALESCING)
        ret |= HEAP_DISABLE_COALESCE_ON_FREE;
    if (flag & FLG_HEAP_PAGE_ALLOCS)
        ret |= HEAP_PAGE_ALLOCS;

    return ret;
}

/***********************************************************************
 *           heap_set_debug_flags
 */
static void heap_set_debug_flags( HANDLE handle )
{
    HEAP *heap = HEAP_GetPtr( handle );
    ULONG global_flags = RtlGetNtGlobalFlags();
    DWORD flags, force_flags;

    if (TRACE_ON(heap)) global_flags |= FLG_HEAP_VALIDATE_ALL;
    if (WARN_ON(heap)) global_flags |= FLG_HEAP_VALIDATE_PARAMETERS;

    flags = heap_flags_from_global_flag( global_flags );
    force_flags = (heap->flags | flags) & ~(HEAP_SHARED|HEAP_DISABLE_COALESCE_ON_FREE);

    if (global_flags & FLG_HEAP_ENABLE_TAGGING) flags |= HEAP_SHARED;
    if (!(global_flags & FLG_HEAP_PAGE_ALLOCS)) force_flags &= ~(HEAP_GROWABLE|HEAP_PRIVATE);

    if (RUNNING_ON_VALGRIND) flags = 0; /* no sense in validating since Valgrind catches accesses */

    heap->flags |= flags;
    heap->force_flags |= force_flags;

    if (flags & (HEAP_FREE_CHECKING_ENABLED | HEAP_TAIL_CHECKING_ENABLED))  /* fix existing blocks */
    {
        SUBHEAP *subheap;
        ARENA_LARGE *large;

        LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
        {
            char *ptr = (char *)subheap->base + subheap->headerSize;
            char *end = (char *)subheap->base + subheap->commitSize;
            while (ptr < end)
            {
                ARENA_INUSE *arena = (ARENA_INUSE *)ptr;
                SIZE_T size = arena->size & ARENA_SIZE_MASK;
                if (arena->size & ARENA_FLAG_FREE)
                {
                    SIZE_T count = size;

                    ptr += sizeof(ARENA_FREE) + size;
                    if (ptr >= end) count = end - (char *)((ARENA_FREE *)arena + 1);
                    else count -= sizeof(ARENA_FREE *);
                    mark_block_free( (ARENA_FREE *)arena + 1, count, flags );
                }
                else
                {
                    if (arena->magic == ARENA_PENDING_MAGIC)
                        mark_block_free( arena + 1, size, flags );
                    else
                        mark_block_tail( (char *)(arena + 1) + size - arena->unused_bytes,
                                         arena->unused_bytes, flags );
                    ptr += sizeof(ARENA_INUSE) + size;
                }
            }
        }

        LIST_FOR_EACH_ENTRY( large, &heap->large_list, ARENA_LARGE, entry )
            mark_block_tail( (char *)(large + 1) + large->data_size,
                             large->block_size - sizeof(*large) - large->data_size, flags );
    }

    if ((heap->flags & HEAP_GROWABLE) && !heap->pending_free &&
        ((flags & HEAP_FREE_CHECKING_ENABLED) || RUNNING_ON_VALGRIND))
    {
        heap->pending_free = RtlAllocateHeap( handle, HEAP_ZERO_MEMORY,
                                              MAX_FREE_PENDING * sizeof(*heap->pending_free) );
        heap->pending_pos = 0;
    }
}


/***********************************************************************
 *           RtlCreateHeap   (NTDLL.@)
 *
 * Create a new Heap.
 *
 * PARAMS
 *  flags      [I] HEAP_ flags from "winnt.h"
 *  addr       [I] Desired base address
 *  totalSize  [I] Total size of the heap, or 0 for a growable heap
 *  commitSize [I] Amount of heap space to commit
 *  unknown    [I] Not yet understood
 *  definition [I] Heap definition
 *
 * RETURNS
 *  Success: A HANDLE to the newly created heap.
 *  Failure: a NULL HANDLE.
 */
HANDLE WINAPI RtlCreateHeap( ULONG flags, PVOID addr, SIZE_T totalSize, SIZE_T commitSize,
                             PVOID unknown, PRTL_HEAP_DEFINITION definition )
{
    SUBHEAP *subheap;

    /* Allocate the heap block */

    if (processHeap) flags |= HEAP_PRIVATE;
    if (!processHeap || !totalSize || (flags & HEAP_SHARED)) flags |= HEAP_GROWABLE;
    if (!totalSize) totalSize = HEAP_DEF_SIZE;

    if (!(subheap = HEAP_CreateSubHeap( NULL, addr, flags, commitSize, totalSize ))) return 0;

    heap_set_debug_flags( subheap->heap );

    /* link it into the per-process heap list */
    if (processHeap)
    {
        HEAP *heapPtr = subheap->heap;
        RtlEnterCriticalSection( &processHeap->critSection );
        list_add_head( &processHeap->entry, &heapPtr->entry );
        RtlLeaveCriticalSection( &processHeap->critSection );
    }
    else if (!addr)
    {
        processHeap = subheap->heap;  /* assume the first heap we create is the process main heap */
        list_init( &processHeap->entry );
    }

    return subheap->heap;
}


/***********************************************************************
 *           RtlDestroyHeap   (NTDLL.@)
 *
 * Destroy a Heap created with RtlCreateHeap().
 *
 * PARAMS
 *  heap [I] Heap to destroy.
 *
 * RETURNS
 *  Success: A NULL HANDLE, if heap is NULL or it was destroyed
 *  Failure: The Heap handle, if heap is the process heap.
 */
HANDLE WINAPI RtlDestroyHeap( HANDLE heap )
{
    HEAP *heapPtr = HEAP_GetPtr( heap );
    SUBHEAP *subheap, *next;
    ARENA_LARGE *arena, *arena_next;
    SIZE_T size;
    void *addr;

    TRACE("%p\n", heap );
    if (!heapPtr && heap && (((HEAP *)heap)->flags & HEAP_VALIDATE_PARAMS) &&
        NtCurrentTeb()->Peb->BeingDebugged)
    {
        DbgPrint( "Attempt to destroy an invalid heap\n" );
        DbgBreakPoint();
    }
    if (!heapPtr) return heap;

    if (heap == processHeap) return heap; /* cannot delete the main process heap */

    /* remove it from the per-process list */
    RtlEnterCriticalSection( &processHeap->critSection );
    list_remove( &heapPtr->entry );
    RtlLeaveCriticalSection( &processHeap->critSection );

    heapPtr->critSection.DebugInfo->Spare[0] = 0;
    RtlDeleteCriticalSection( &heapPtr->critSection );

    LIST_FOR_EACH_ENTRY_SAFE( arena, arena_next, &heapPtr->large_list, ARENA_LARGE, entry )
    {
        list_remove( &arena->entry );
        size = 0;
        addr = arena;
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    }
    LIST_FOR_EACH_ENTRY_SAFE( subheap, next, &heapPtr->subheap_list, SUBHEAP, entry )
    {
        if (subheap == &heapPtr->subheap) continue;  /* do this one last */
        subheap_notify_free_all(subheap);
        list_remove( &subheap->entry );
        size = 0;
        addr = subheap->base;
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    }
    subheap_notify_free_all(&heapPtr->subheap);
    RtlFreeHeap( GetProcessHeap(), 0, heapPtr->pending_free );
    size = 0;
    addr = heapPtr->subheap.base;
    NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    return 0;
}


/***********************************************************************
 *           RtlAllocateHeap   (NTDLL.@)
 *
 * Allocate a memory block from a Heap.
 *
 * PARAMS
 *  heap  [I] Heap to allocate block from
 *  flags [I] HEAP_ flags from "winnt.h"
 *  size  [I] Size of the memory block to allocate
 *
 * RETURNS
 *  Success: A pointer to the newly allocated block
 *  Failure: NULL.
 *
 * NOTES
 *  This call does not SetLastError().
 */
void * WINAPI DECLSPEC_HOTPATCH RtlAllocateHeap( HANDLE heap, ULONG flags, SIZE_T size )
{
    ARENA_FREE *pArena;
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;
    HEAP *heapPtr = HEAP_GetPtr( heap );
    SIZE_T rounded_size;

    /* Validate the parameters */

    if (!heapPtr) return NULL;
    flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY;
    flags |= heapPtr->flags;
    rounded_size = ROUND_SIZE(size) + HEAP_TAIL_EXTRA_SIZE( flags );
    if (rounded_size < size)  /* overflow */
    {
        if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
        return NULL;
    }
    if (rounded_size < HEAP_MIN_DATA_SIZE) rounded_size = HEAP_MIN_DATA_SIZE;

    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    if (rounded_size >= HEAP_MIN_LARGE_BLOCK_SIZE && (flags & HEAP_GROWABLE))
    {
        void *ret = allocate_large_block( heap, flags, size );
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
        if (!ret && (flags & HEAP_GENERATE_EXCEPTIONS)) RtlRaiseStatus( STATUS_NO_MEMORY );
        TRACE("(%p,%08x,%08lx): returning %p\n", heap, flags, size, ret );
        return ret;
    }

    /* Locate a suitable free block */

    if (!(pArena = HEAP_FindFreeBlock( heapPtr, rounded_size, &subheap )))
    {
        TRACE("(%p,%08x,%08lx): returning NULL\n",
                  heap, flags, size  );
        if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
        if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
        return NULL;
    }

    /* Remove the arena from the free list */

    list_remove( &pArena->entry );

    /* Build the in-use arena */

    pInUse = (ARENA_INUSE *)pArena;

    /* in-use arena is smaller than free arena,
     * so we have to add the difference to the size */
    pInUse->size  = (pInUse->size & ~ARENA_FLAG_FREE) + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
    pInUse->magic = ARENA_INUSE_MAGIC;

    /* Shrink the block */

    HEAP_ShrinkBlock( subheap, pInUse, rounded_size );
    pInUse->unused_bytes = (pInUse->size & ARENA_SIZE_MASK) - size;

    notify_alloc( pInUse + 1, size, flags & HEAP_ZERO_MEMORY );
    initialize_block( pInUse + 1, size, pInUse->unused_bytes, flags );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%p,%08x,%08lx): returning %p\n", heap, flags, size, pInUse + 1 );
    return pInUse + 1;
}


/***********************************************************************
 *           RtlFreeHeap   (NTDLL.@)
 *
 * Free a memory block allocated with RtlAllocateHeap().
 *
 * PARAMS
 *  heap  [I] Heap that block was allocated from
 *  flags [I] HEAP_ flags from "winnt.h"
 *  ptr   [I] Block to free
 *
 * RETURNS
 *  Success: TRUE, if ptr is NULL or was freed successfully.
 *  Failure: FALSE.
 */
BOOLEAN WINAPI DECLSPEC_HOTPATCH RtlFreeHeap( HANDLE heap, ULONG flags, void *ptr )
{
    ARENA_INUSE *pInUse;
    SUBHEAP *subheap;
    HEAP *heapPtr;

    /* Validate the parameters */

    if (!ptr) return TRUE;  /* freeing a NULL ptr isn't an error in Win2k */

    heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return FALSE;
    }

    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    /* Inform valgrind we are trying to free memory, so it can throw up an error message */
    notify_free( ptr );

    /* Some sanity checks */
    pInUse  = (ARENA_INUSE *)ptr - 1;
    if (!validate_block_pointer( heapPtr, &subheap, pInUse )) goto error;

    if (!subheap)
        free_large_block( heapPtr, flags, ptr );
    else
        HEAP_MakeInUseBlockFree( subheap, pInUse );

    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    TRACE("(%p,%08x,%p): returning TRUE\n", heap, flags, ptr );
    return TRUE;

error:
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
    TRACE("(%p,%08x,%p): returning FALSE\n", heap, flags, ptr );
    return FALSE;
}


/***********************************************************************
 *           RtlReAllocateHeap   (NTDLL.@)
 *
 * Change the size of a memory block allocated with RtlAllocateHeap().
 *
 * PARAMS
 *  heap  [I] Heap that block was allocated from
 *  flags [I] HEAP_ flags from "winnt.h"
 *  ptr   [I] Block to resize
 *  size  [I] Size of the memory block to allocate
 *
 * RETURNS
 *  Success: A pointer to the resized block (which may be different).
 *  Failure: NULL.
 */
PVOID WINAPI RtlReAllocateHeap( HANDLE heap, ULONG flags, PVOID ptr, SIZE_T size )
{
    ARENA_INUSE *pArena;
    HEAP *heapPtr;
    SUBHEAP *subheap;
    SIZE_T oldBlockSize, oldActualSize, rounded_size;
    void *ret;

    if (!ptr) return NULL;
    if (!(heapPtr = HEAP_GetPtr( heap )))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return NULL;
    }

    /* Validate the parameters */

    flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY |
             HEAP_REALLOC_IN_PLACE_ONLY;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    rounded_size = ROUND_SIZE(size) + HEAP_TAIL_EXTRA_SIZE(flags);
    if (rounded_size < size) goto oom;  /* overflow */
    if (rounded_size < HEAP_MIN_DATA_SIZE) rounded_size = HEAP_MIN_DATA_SIZE;

    pArena = (ARENA_INUSE *)ptr - 1;
    if (!validate_block_pointer( heapPtr, &subheap, pArena )) goto error;
    if (!subheap)
    {
        if (!(ret = realloc_large_block( heapPtr, flags, ptr, size ))) goto oom;
        goto done;
    }

    /* Check if we need to grow the block */

    oldBlockSize = (pArena->size & ARENA_SIZE_MASK);
    oldActualSize = (pArena->size & ARENA_SIZE_MASK) - pArena->unused_bytes;
    if (rounded_size > oldBlockSize)
    {
        char *pNext = (char *)(pArena + 1) + oldBlockSize;

        if (rounded_size >= HEAP_MIN_LARGE_BLOCK_SIZE && (flags & HEAP_GROWABLE))
        {
            if (flags & HEAP_REALLOC_IN_PLACE_ONLY) goto oom;
            if (!(ret = allocate_large_block( heapPtr, flags, size ))) goto oom;
            memcpy( ret, pArena + 1, oldActualSize );
            notify_free( pArena + 1 );
            HEAP_MakeInUseBlockFree( subheap, pArena );
            goto done;
        }
        if ((pNext < (char *)subheap->base + subheap->size) &&
            (*(DWORD *)pNext & ARENA_FLAG_FREE) &&
            (oldBlockSize + (*(DWORD *)pNext & ARENA_SIZE_MASK) + sizeof(ARENA_FREE) >= rounded_size))
        {
            /* The next block is free and large enough */
            ARENA_FREE *pFree = (ARENA_FREE *)pNext;
            list_remove( &pFree->entry );
            pArena->size += (pFree->size & ARENA_SIZE_MASK) + sizeof(*pFree);
            if (!HEAP_Commit( subheap, pArena, rounded_size )) goto oom;
            notify_realloc( pArena + 1, oldActualSize, size );
            HEAP_ShrinkBlock( subheap, pArena, rounded_size );
        }
        else  /* Do it the hard way */
        {
            ARENA_FREE *pNew;
            ARENA_INUSE *pInUse;
            SUBHEAP *newsubheap;

            if ((flags & HEAP_REALLOC_IN_PLACE_ONLY) ||
                !(pNew = HEAP_FindFreeBlock( heapPtr, rounded_size, &newsubheap )))
                goto oom;

            /* Build the in-use arena */

            list_remove( &pNew->entry );
            pInUse = (ARENA_INUSE *)pNew;
            pInUse->size = (pInUse->size & ~ARENA_FLAG_FREE)
                           + sizeof(ARENA_FREE) - sizeof(ARENA_INUSE);
            pInUse->magic = ARENA_INUSE_MAGIC;
            HEAP_ShrinkBlock( newsubheap, pInUse, rounded_size );

            mark_block_initialized( pInUse + 1, oldActualSize );
            notify_alloc( pInUse + 1, size, FALSE );
            memcpy( pInUse + 1, pArena + 1, oldActualSize );

            /* Free the previous block */

            notify_free( pArena + 1 );
            HEAP_MakeInUseBlockFree( subheap, pArena );
            subheap = newsubheap;
            pArena  = pInUse;
        }
    }
    else
    {
        notify_realloc( pArena + 1, oldActualSize, size );
        HEAP_ShrinkBlock( subheap, pArena, rounded_size );
    }

    pArena->unused_bytes = (pArena->size & ARENA_SIZE_MASK) - size;

    /* Clear the extra bytes if needed */

    if (size > oldActualSize)
        initialize_block( (char *)(pArena + 1) + oldActualSize, size - oldActualSize,
                          pArena->unused_bytes, flags );
    else
        mark_block_tail( (char *)(pArena + 1) + size, pArena->unused_bytes, flags );

    /* Return the new arena */

    ret = pArena + 1;
done:
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    TRACE("(%p,%08x,%p,%08lx): returning %p\n", heap, flags, ptr, size, ret );
    return ret;

oom:
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    if (flags & HEAP_GENERATE_EXCEPTIONS) RtlRaiseStatus( STATUS_NO_MEMORY );
    RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_NO_MEMORY );
    TRACE("(%p,%08x,%p,%08lx): returning NULL\n", heap, flags, ptr, size );
    return NULL;

error:
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
    TRACE("(%p,%08x,%p,%08lx): returning NULL\n", heap, flags, ptr, size );
    return NULL;
}


/***********************************************************************
 *           RtlCompactHeap   (NTDLL.@)
 *
 * Compact the free space in a Heap.
 *
 * PARAMS
 *  heap  [I] Heap that block was allocated from
 *  flags [I] HEAP_ flags from "winnt.h"
 *
 * RETURNS
 *  The number of bytes compacted.
 *
 * NOTES
 *  This function is a harmless stub.
 */
ULONG WINAPI RtlCompactHeap( HANDLE heap, ULONG flags )
{
    static BOOL reported;
    if (!reported++) FIXME( "(%p, 0x%x) stub\n", heap, flags );
    return 0;
}


/***********************************************************************
 *           RtlLockHeap   (NTDLL.@)
 *
 * Lock a Heap.
 *
 * PARAMS
 *  heap  [I] Heap to lock
 *
 * RETURNS
 *  Success: TRUE. The Heap is locked.
 *  Failure: FALSE, if heap is invalid.
 */
BOOLEAN WINAPI RtlLockHeap( HANDLE heap )
{
    HEAP *heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr) return FALSE;
    RtlEnterCriticalSection( &heapPtr->critSection );
    return TRUE;
}


/***********************************************************************
 *           RtlUnlockHeap   (NTDLL.@)
 *
 * Unlock a Heap.
 *
 * PARAMS
 *  heap  [I] Heap to unlock
 *
 * RETURNS
 *  Success: TRUE. The Heap is unlocked.
 *  Failure: FALSE, if heap is invalid.
 */
BOOLEAN WINAPI RtlUnlockHeap( HANDLE heap )
{
    HEAP *heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr) return FALSE;
    RtlLeaveCriticalSection( &heapPtr->critSection );
    return TRUE;
}


/***********************************************************************
 *           RtlSizeHeap   (NTDLL.@)
 *
 * Get the actual size of a memory block allocated from a Heap.
 *
 * PARAMS
 *  heap  [I] Heap that block was allocated from
 *  flags [I] HEAP_ flags from "winnt.h"
 *  ptr   [I] Block to get the size of
 *
 * RETURNS
 *  Success: The size of the block.
 *  Failure: -1, heap or ptr are invalid.
 *
 * NOTES
 *  The size may be bigger than what was passed to RtlAllocateHeap().
 */
SIZE_T WINAPI RtlSizeHeap( HANDLE heap, ULONG flags, const void *ptr )
{
    SIZE_T ret;
    const ARENA_INUSE *pArena;
    SUBHEAP *subheap;
    HEAP *heapPtr = HEAP_GetPtr( heap );

    if (!heapPtr)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_HANDLE );
        return ~(SIZE_T)0;
    }
    flags &= HEAP_NO_SERIALIZE;
    flags |= heapPtr->flags;
    if (!(flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    pArena = (const ARENA_INUSE *)ptr - 1;
    if (!validate_block_pointer( heapPtr, &subheap, pArena ))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
        ret = ~(SIZE_T)0;
    }
    else if (!subheap)
    {
        const ARENA_LARGE *large_arena = (const ARENA_LARGE *)ptr - 1;
        ret = large_arena->data_size;
    }
    else
    {
        ret = (pArena->size & ARENA_SIZE_MASK) - pArena->unused_bytes;
    }
    if (!(flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );

    TRACE("(%p,%08x,%p): returning %08lx\n", heap, flags, ptr, ret );
    return ret;
}


/***********************************************************************
 *           RtlValidateHeap   (NTDLL.@)
 *
 * Determine if a block is a valid allocation from a heap.
 *
 * PARAMS
 *  heap  [I] Heap that block was allocated from
 *  flags [I] HEAP_ flags from "winnt.h"
 *  ptr   [I] Block to check
 *
 * RETURNS
 *  Success: TRUE. The block was allocated from heap.
 *  Failure: FALSE, if heap is invalid or ptr was not allocated from it.
 */
BOOLEAN WINAPI RtlValidateHeap( HANDLE heap, ULONG flags, LPCVOID ptr )
{
    HEAP *heapPtr = HEAP_GetPtr( heap );
    if (!heapPtr) return FALSE;
    return HEAP_IsRealArena( heapPtr, flags, ptr, QUIET );
}


/***********************************************************************
 *           RtlWalkHeap    (NTDLL.@)
 *
 * FIXME
 *  The PROCESS_HEAP_ENTRY flag values seem different between this
 *  function and HeapWalk(). To be checked.
 */
NTSTATUS WINAPI RtlWalkHeap( HANDLE heap, PVOID entry_ptr )
{
    LPPROCESS_HEAP_ENTRY entry = entry_ptr; /* FIXME */
    HEAP *heapPtr = HEAP_GetPtr(heap);
    SUBHEAP *sub, *currentheap = NULL;
    NTSTATUS ret;
    char *ptr;
    int region_index = 0;

    if (!heapPtr || !entry) return STATUS_INVALID_PARAMETER;

    if (!(heapPtr->flags & HEAP_NO_SERIALIZE)) RtlEnterCriticalSection( &heapPtr->critSection );

    /* FIXME: enumerate large blocks too */

    /* set ptr to the next arena to be examined */

    if (!entry->lpData) /* first call (init) ? */
    {
        TRACE("begin walking of heap %p.\n", heap);
        currentheap = &heapPtr->subheap;
        ptr = (char*)currentheap->base + currentheap->headerSize;
    }
    else
    {
        ptr = entry->lpData;
        LIST_FOR_EACH_ENTRY( sub, &heapPtr->subheap_list, SUBHEAP, entry )
        {
            if ((ptr >= (char *)sub->base) &&
                (ptr < (char *)sub->base + sub->size))
            {
                currentheap = sub;
                break;
            }
            region_index++;
        }
        if (currentheap == NULL)
        {
            ERR("no matching subheap found, shouldn't happen !\n");
            ret = STATUS_NO_MORE_ENTRIES;
            goto HW_end;
        }

        if (((ARENA_INUSE *)ptr - 1)->magic == ARENA_INUSE_MAGIC ||
            ((ARENA_INUSE *)ptr - 1)->magic == ARENA_PENDING_MAGIC)
        {
            ARENA_INUSE *pArena = (ARENA_INUSE *)ptr - 1;
            ptr += pArena->size & ARENA_SIZE_MASK;
        }
        else if (((ARENA_FREE *)ptr - 1)->magic == ARENA_FREE_MAGIC)
        {
            ARENA_FREE *pArena = (ARENA_FREE *)ptr - 1;
            ptr += pArena->size & ARENA_SIZE_MASK;
        }
        else
            ptr += entry->cbData; /* point to next arena */

        if (ptr > (char *)currentheap->base + currentheap->size - 1)
        {   /* proceed with next subheap */
            struct list *next = list_next( &heapPtr->subheap_list, &currentheap->entry );
            if (!next)
            {  /* successfully finished */
                TRACE("end reached.\n");
                ret = STATUS_NO_MORE_ENTRIES;
                goto HW_end;
            }
            currentheap = LIST_ENTRY( next, SUBHEAP, entry );
            ptr = (char *)currentheap->base + currentheap->headerSize;
        }
    }

    entry->wFlags = 0;
    if (*(DWORD *)ptr & ARENA_FLAG_FREE)
    {
        ARENA_FREE *pArena = (ARENA_FREE *)ptr;

        /*TRACE("free, magic: %04x\n", pArena->magic);*/

        entry->lpData = pArena + 1;
        entry->cbData = pArena->size & ARENA_SIZE_MASK;
        entry->cbOverhead = sizeof(ARENA_FREE);
        entry->wFlags = PROCESS_HEAP_UNCOMMITTED_RANGE;
    }
    else
    {
        ARENA_INUSE *pArena = (ARENA_INUSE *)ptr;

        /*TRACE("busy, magic: %04x\n", pArena->magic);*/

        entry->lpData = pArena + 1;
        entry->cbData = pArena->size & ARENA_SIZE_MASK;
        entry->cbOverhead = sizeof(ARENA_INUSE);
        entry->wFlags = (pArena->magic == ARENA_PENDING_MAGIC) ?
                        PROCESS_HEAP_UNCOMMITTED_RANGE : PROCESS_HEAP_ENTRY_BUSY;
        /* FIXME: can't handle PROCESS_HEAP_ENTRY_MOVEABLE
        and PROCESS_HEAP_ENTRY_DDESHARE yet */
    }

    entry->iRegionIndex = region_index;

    /* first element of heap ? */
    if (ptr == (char *)currentheap->base + currentheap->headerSize)
    {
        entry->wFlags |= PROCESS_HEAP_REGION;
        entry->u.Region.dwCommittedSize = currentheap->commitSize;
        entry->u.Region.dwUnCommittedSize =
                currentheap->size - currentheap->commitSize;
        entry->u.Region.lpFirstBlock = /* first valid block */
                (char *)currentheap->base + currentheap->headerSize;
        entry->u.Region.lpLastBlock  = /* first invalid block */
                (char *)currentheap->base + currentheap->size;
    }
    ret = STATUS_SUCCESS;
    if (TRACE_ON(heap)) HEAP_DumpEntry(entry);

HW_end:
    if (!(heapPtr->flags & HEAP_NO_SERIALIZE)) RtlLeaveCriticalSection( &heapPtr->critSection );
    return ret;
}


/***********************************************************************
 *           RtlGetProcessHeaps    (NTDLL.@)
 *
 * Get the Heaps belonging to the current process.
 *
 * PARAMS
 *  count [I] size of heaps
 *  heaps [O] Destination array for heap HANDLE's
 *
 * RETURNS
 *  Success: The number of Heaps allocated by the process.
 *  Failure: 0.
 */
ULONG WINAPI RtlGetProcessHeaps( ULONG count, HANDLE *heaps )
{
    ULONG total = 1;  /* main heap */
    struct list *ptr;

    RtlEnterCriticalSection( &processHeap->critSection );
    LIST_FOR_EACH( ptr, &processHeap->entry ) total++;
    if (total <= count)
    {
        *heaps++ = processHeap;
        LIST_FOR_EACH( ptr, &processHeap->entry )
            *heaps++ = LIST_ENTRY( ptr, HEAP, entry );
    }
    RtlLeaveCriticalSection( &processHeap->critSection );
    return total;
}

/***********************************************************************
 *           RtlQueryHeapInformation    (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryHeapInformation( HANDLE heap, HEAP_INFORMATION_CLASS info_class,
                                         PVOID info, SIZE_T size_in, PSIZE_T size_out)
{
    switch (info_class)
    {
    case HeapCompatibilityInformation:
        if (size_out) *size_out = sizeof(ULONG);

        if (size_in < sizeof(ULONG))
            return STATUS_BUFFER_TOO_SMALL;

        *(ULONG *)info = 0; /* standard heap */
        return STATUS_SUCCESS;

    default:
        FIXME("Unknown heap information class %u\n", info_class);
        return STATUS_INVALID_INFO_CLASS;
    }
}

/***********************************************************************
 *           RtlSetHeapInformation    (NTDLL.@)
 */
NTSTATUS WINAPI RtlSetHeapInformation( HANDLE heap, HEAP_INFORMATION_CLASS info_class, PVOID info, SIZE_T size)
{
    FIXME("%p %d %p %ld stub\n", heap, info_class, info, size);
    return STATUS_SUCCESS;
}
