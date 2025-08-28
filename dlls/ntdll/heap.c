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
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(heap);

/* HeapCompatibilityInformation values */

#define HEAP_STD 0
#define HEAP_LAL 1
#define HEAP_LFH 2


/* undocumented RtlWalkHeap structure */

struct rtl_heap_entry
{
    LPVOID lpData;
    SIZE_T cbData; /* differs from PROCESS_HEAP_ENTRY */
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags; /* value differs from PROCESS_HEAP_ENTRY */
    union {
        struct {
            HANDLE hMem;
            DWORD dwReserved[3];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    };
};

/* rtl_heap_entry flags, names made up */

#define RTL_HEAP_ENTRY_BUSY         0x0001
#define RTL_HEAP_ENTRY_REGION       0x0002
#define RTL_HEAP_ENTRY_BLOCK        0x0010
#define RTL_HEAP_ENTRY_UNCOMMITTED  0x1000
#define RTL_HEAP_ENTRY_COMMITTED    0x4000
#define RTL_HEAP_ENTRY_LFH          0x8000


/* header for heap blocks */

#define REGION_ALIGN  0x10000
#define BLOCK_ALIGN   (2 * sizeof(void *))

struct DECLSPEC_ALIGN(8) block
{
    /* block size in multiple of BLOCK_ALIGN */
    WORD block_size;
    /* unused size (used block) / high size bits (free block) */
    WORD tail_size;
    /* offset to region base / first group block (LFH block) */
    WORD base_offset;
    BYTE block_type;
    BYTE block_flags;
};

C_ASSERT( sizeof(struct block) == 8 );

/* block specific flags */

#define BLOCK_FLAG_FREE        0x01
#define BLOCK_FLAG_PREV_FREE   0x02
#define BLOCK_FLAG_FREE_LINK   0x03
#define BLOCK_FLAG_LARGE       0x04
#define BLOCK_FLAG_LFH         0x80 /* block is handled by the LFH frontend */
#define BLOCK_FLAG_USER_INFO   0x08 /* user flags bits 3-6 */
#define BLOCK_FLAG_USER_MASK   0x78

#define BLOCK_USER_FLAGS( heap_flags ) (((heap_flags) >> 5) & BLOCK_FLAG_USER_MASK)
#define HEAP_USER_FLAGS( block_flags ) (((block_flags) & BLOCK_FLAG_USER_MASK) << 5)

/* entry to link free blocks in free lists */

struct DECLSPEC_ALIGN(BLOCK_ALIGN) entry
{
    struct block block;
    struct list entry;
};

C_ASSERT( sizeof(struct entry) == 2 * BLOCK_ALIGN );

typedef struct
{
    SIZE_T __pad[sizeof(SIZE_T) / sizeof(DWORD)];
    struct list           entry;      /* entry in heap large blocks list */
    SIZE_T                data_size;  /* size of user data */
    SIZE_T                block_size; /* total size of virtual memory block */
    void                 *user_value;
    struct block block;
} ARENA_LARGE;

/* block must be last and aligned */
C_ASSERT( sizeof(ARENA_LARGE) == offsetof(ARENA_LARGE, block) + sizeof(struct block) );
C_ASSERT( sizeof(ARENA_LARGE) == 4 * BLOCK_ALIGN );

#define BLOCK_TYPE_USED        'u'
#define BLOCK_TYPE_DEAD        'D'
#define BLOCK_TYPE_FREE        'F'
#define BLOCK_TYPE_LARGE       'L'

#define BLOCK_FILL_USED        0xbaadf00d
#define BLOCK_FILL_TAIL        0xab
#define BLOCK_FILL_FREE        0xfeeefeee

#define ROUND_ADDR(addr, mask) ((void *)((UINT_PTR)(addr) & ~(UINT_PTR)(mask)))
#define ROUND_SIZE(size, mask) ((((SIZE_T)(size) + (mask)) & ~(SIZE_T)(mask)))
#define FIELD_BITS(type, field) (sizeof(((type *)0)->field) * 8)
#define FIELD_MAX(type, field) (((SIZE_T)1 << FIELD_BITS(type, field)) - 1)

#define HEAP_MIN_BLOCK_SIZE   ROUND_SIZE(sizeof(struct entry) + BLOCK_ALIGN, BLOCK_ALIGN - 1)

C_ASSERT( sizeof(struct block) <= HEAP_MIN_BLOCK_SIZE );
C_ASSERT( sizeof(struct entry) <= HEAP_MIN_BLOCK_SIZE );

/* used block size is coded into block_size */
#define HEAP_MAX_USED_BLOCK_SIZE    (FIELD_MAX( struct block, block_size ) * BLOCK_ALIGN)
/* free block size is coded into block_size + tail_size */
#define HEAP_MAX_FREE_BLOCK_SIZE    (HEAP_MAX_USED_BLOCK_SIZE + (FIELD_MAX( struct block, tail_size ) << 16) * BLOCK_ALIGN)
/* subheap distance is coded into base_offset */
#define HEAP_MAX_BLOCK_REGION_SIZE  (FIELD_MAX( struct block, base_offset ) * REGION_ALIGN)

C_ASSERT( HEAP_MAX_USED_BLOCK_SIZE == 512 * 1024 * (sizeof(void *) / 4) - BLOCK_ALIGN );
C_ASSERT( HEAP_MAX_FREE_BLOCK_SIZE >= 128 * 1024 * 1024 * (sizeof(void *) / 4) - BLOCK_ALIGN );
C_ASSERT( HEAP_MAX_BLOCK_REGION_SIZE >= 128 * 1024 * 1024 * (sizeof(void *) / 4) - BLOCK_ALIGN );
C_ASSERT( HEAP_MAX_FREE_BLOCK_SIZE >= HEAP_MAX_BLOCK_REGION_SIZE );

/* minimum size to start allocating large blocks */
#define HEAP_MIN_LARGE_BLOCK_SIZE  (HEAP_MAX_USED_BLOCK_SIZE - 0x1000)

#define FREE_LIST_LINEAR_BITS 2
#define FREE_LIST_LINEAR_MASK ((1 << FREE_LIST_LINEAR_BITS) - 1)
#define FREE_LIST_COUNT ((FIELD_BITS( struct block, block_size ) - FREE_LIST_LINEAR_BITS + 1) * (1 << FREE_LIST_LINEAR_BITS) + 1)
/* for reference, update this when changing parameters */
C_ASSERT( FREE_LIST_COUNT == 0x3d );

typedef struct DECLSPEC_ALIGN(BLOCK_ALIGN) tagSUBHEAP
{
    SIZE_T __pad[sizeof(SIZE_T) / sizeof(DWORD)];
    SIZE_T block_size;
    SIZE_T data_size;
    struct list entry;
    void *user_value;
    struct block block;
} SUBHEAP;

/* block must be last and aligned */
C_ASSERT( sizeof(SUBHEAP) == offsetof(SUBHEAP, block) + sizeof(struct block) );
C_ASSERT( sizeof(SUBHEAP) == 4 * BLOCK_ALIGN );


/* LFH block size bins */

#define BIN_SIZE_MIN_0        0
#define BIN_SIZE_MIN_1    0x100
#define BIN_SIZE_MIN_2    0x200
#define BIN_SIZE_MIN_3    0x400
#define BIN_SIZE_MIN_4    0x800
#define BIN_SIZE_MIN_5   0x1000
#define BIN_SIZE_MIN_6   0x2000
#define BIN_SIZE_MIN_7   0x4000
#define BIN_SIZE_MAX     0x8000

#define BIN_SIZE_STEP_0                   (16)
#define BIN_SIZE_STEP_1  (BIN_SIZE_MIN_1 >> 4)
#define BIN_SIZE_STEP_2  (BIN_SIZE_MIN_2 >> 4)
#define BIN_SIZE_STEP_3  (BIN_SIZE_MIN_3 >> 4)
#define BIN_SIZE_STEP_4  (BIN_SIZE_MIN_4 >> 4)
#define BIN_SIZE_STEP_5  (BIN_SIZE_MIN_5 >> 4)
#define BIN_SIZE_STEP_6  (BIN_SIZE_MIN_6 >> 4)
#define BIN_SIZE_STEP_7  (BIN_SIZE_MIN_7 >> 4)

#define BLOCK_BIN_SIZE_N( n, bin )   (BIN_SIZE_MIN_##n + (bin + 1) * BIN_SIZE_STEP_##n)
#define BLOCK_SIZE_BIN_N( n, size )  (((size) - 1 - BIN_SIZE_MIN_##n) / BIN_SIZE_STEP_##n)

#define BLOCK_BIN_SIZE( bin )   ((bin) >= 0x80 ? ~(SIZE_T)0 : \
                                 (bin) >= 0x70 ? BLOCK_BIN_SIZE_N( 7, bin - 0x70 ) : \
                                 (bin) >= 0x60 ? BLOCK_BIN_SIZE_N( 6, bin - 0x60 ) : \
                                 (bin) >= 0x50 ? BLOCK_BIN_SIZE_N( 5, bin - 0x50 ) : \
                                 (bin) >= 0x40 ? BLOCK_BIN_SIZE_N( 4, bin - 0x40 ) : \
                                 (bin) >= 0x30 ? BLOCK_BIN_SIZE_N( 3, bin - 0x30 ) : \
                                 (bin) >= 0x20 ? BLOCK_BIN_SIZE_N( 2, bin - 0x20 ) : \
                                 (bin) >= 0x10 ? BLOCK_BIN_SIZE_N( 1, bin - 0x10 ) : \
                                 BLOCK_BIN_SIZE_N( 0, bin ))

#define BLOCK_SIZE_BIN( size )  ((size) > BIN_SIZE_MAX   ? 0x80 : \
                                 (size) > BIN_SIZE_MIN_7 ? 0x70 + BLOCK_SIZE_BIN_N( 7, size ) : \
                                 (size) > BIN_SIZE_MIN_6 ? 0x60 + BLOCK_SIZE_BIN_N( 6, size ) : \
                                 (size) > BIN_SIZE_MIN_5 ? 0x50 + BLOCK_SIZE_BIN_N( 5, size ) : \
                                 (size) > BIN_SIZE_MIN_4 ? 0x40 + BLOCK_SIZE_BIN_N( 4, size ) : \
                                 (size) > BIN_SIZE_MIN_3 ? 0x30 + BLOCK_SIZE_BIN_N( 3, size ) : \
                                 (size) > BIN_SIZE_MIN_2 ? 0x20 + BLOCK_SIZE_BIN_N( 2, size ) : \
                                 (size) > BIN_SIZE_MIN_1 ? 0x10 + BLOCK_SIZE_BIN_N( 1, size ) : \
                                 (size) <= BIN_SIZE_MIN_0 ?   0 : BLOCK_SIZE_BIN_N( 0, size ))

#define BLOCK_SIZE_BIN_COUNT    (BLOCK_SIZE_BIN( BIN_SIZE_MAX + 1 ) + 1)

/* macros sanity checks */
C_ASSERT( BLOCK_SIZE_BIN( 0 ) == 0 );
C_ASSERT( BLOCK_SIZE_BIN( 0x10 ) == 0 );
C_ASSERT( BLOCK_BIN_SIZE( 0 ) == BIN_SIZE_MIN_0 + 1 * BIN_SIZE_STEP_0 );
C_ASSERT( BLOCK_SIZE_BIN( 0x11 ) == 1 );
C_ASSERT( BLOCK_BIN_SIZE( 1 ) == BIN_SIZE_MIN_0 + 2 * BIN_SIZE_STEP_0 );
C_ASSERT( BLOCK_SIZE_BIN( BIN_SIZE_MAX ) == 0x7f );
C_ASSERT( BLOCK_BIN_SIZE( 0x7f ) == BIN_SIZE_MAX );
C_ASSERT( BLOCK_SIZE_BIN( BIN_SIZE_MAX + 1 ) == 0x80 );
C_ASSERT( BLOCK_BIN_SIZE( 0x80 ) == ~(SIZE_T)0 );

/* difference between block classes and all possible validation overhead must fit into block tail_size */
C_ASSERT( BIN_SIZE_STEP_7 + 3 * BLOCK_ALIGN <= FIELD_MAX( struct block, tail_size ) );

static BYTE affinity_mapping[] = {20,6,31,15,14,29,27,4,18,24,26,13,0,9,2,30,17,7,23,25,10,19,12,3,22,21,5,16,1,28,11,8};
static LONG next_thread_affinity;

/* a bin, tracking heap blocks of a certain size */
struct bin
{
    /* counters for LFH activation */
    LONG count_alloc;
    LONG count_freed;
    LONG enabled;

    /* list of groups with free blocks */
    SLIST_HEADER groups;

    /* array of affinity reserved groups, interleaved with other bins to keep
     * all pointers of the same affinity and different bin grouped together,
     * and pointers of the same bin and different affinity away from each other,
     * hopefully in separate cache lines.
     */
    struct group **affinity_group_base;
};

static inline struct group **bin_get_affinity_group( struct bin *bin, BYTE affinity )
{
    return bin->affinity_group_base + affinity * BLOCK_SIZE_BIN_COUNT;
}

struct heap
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

    LONG             compat_info;   /* HeapCompatibilityInformation / heap frontend type */
    struct list      entry;         /* Entry in process heap list */
    struct list      subheap_list;  /* Sub-heap list */
    struct list      large_list;    /* Large blocks list */
    SIZE_T           grow_size;     /* Size of next subheap for growing heap */
    SIZE_T           min_size;      /* Minimum committed size */
    DWORD            magic;         /* Magic number */
    DWORD            pending_pos;   /* Position in pending free requests ring */
    struct block   **pending_free;  /* Ring buffer for pending free requests */
    RTL_CRITICAL_SECTION cs;
    struct entry     free_lists[FREE_LIST_COUNT];
    struct bin      *bins;
    SUBHEAP          subheap;
};

/* subheap must be last and aligned */
C_ASSERT( sizeof(struct heap) == offsetof(struct heap, subheap) + sizeof(SUBHEAP) );
C_ASSERT( sizeof(struct heap) % BLOCK_ALIGN == 0 );
C_ASSERT( offsetof(struct heap, subheap) <= REGION_ALIGN - 1 );

#define HEAP_MAGIC       ((DWORD)('H' | ('E'<<8) | ('A'<<16) | ('P'<<24)))

#define HEAP_INITIAL_SIZE      0x10000
#define HEAP_INITIAL_GROW_SIZE 0x100000
#define HEAP_MAX_GROW_SIZE     0xfd0000

C_ASSERT( HEAP_MIN_LARGE_BLOCK_SIZE <= HEAP_INITIAL_GROW_SIZE );

#define MAX_FREE_PENDING     1024    /* max number of free requests to delay */

/* some undocumented flags (names are made up) */
#define HEAP_PRIVATE          0x00001000
#define HEAP_ADD_USER_INFO    0x00000100
#define HEAP_USER_FLAGS_MASK  0x00000f00
#define HEAP_PAGE_ALLOCS      0x01000000
#define HEAP_VALIDATE         0x10000000
#define HEAP_VALIDATE_ALL     0x20000000
#define HEAP_VALIDATE_PARAMS  0x40000000
#define HEAP_CHECKING_ENABLED 0x80000000

static struct heap *process_heap;  /* main process heap */

static NTSTATUS heap_free_block_lfh( struct heap *heap, ULONG flags, struct block *block );

/* check if memory range a contains memory range b */
static inline BOOL contains( const void *a, SIZE_T a_size, const void *b, SIZE_T b_size )
{
    const void *a_end = (char *)a + a_size, *b_end = (char *)b + b_size;
    return a <= b && b <= b_end && b_end <= a_end;
}

static inline UINT block_get_flags( const struct block *block )
{
    return block->block_flags;
}

static inline UINT block_get_type( const struct block *block )
{
    return block->block_type;
}

static inline void block_set_base( struct block *block, const void *base )
{
    const char *offset = ROUND_ADDR( block, REGION_ALIGN - 1 );
    block->base_offset = (offset - (char *)base) / REGION_ALIGN;
}

static inline void block_set_type( struct block *block, UINT type )
{
    block->block_type = type;
}

static inline SUBHEAP *block_get_subheap( const struct heap *heap, const struct block *block )
{
    char *offset = ROUND_ADDR( block, REGION_ALIGN - 1 );
    void *base = offset - (SIZE_T)block->base_offset * REGION_ALIGN;
    if (base != (void *)heap) return base;
    else return (SUBHEAP *)&heap->subheap;
}

static inline UINT block_get_overhead( const struct block *block )
{
    if (block_get_flags( block ) & BLOCK_FLAG_FREE) return sizeof(*block) + sizeof(struct list);
    return sizeof(*block) + block->tail_size;
}

/* return the size of a block, including its header */
static inline UINT block_get_size( const struct block *block )
{
    UINT block_size = block->block_size;
    if (block_get_flags( block ) & BLOCK_FLAG_FREE) block_size += (UINT)block->tail_size << 16;
    return block_size * BLOCK_ALIGN;
}

static inline void block_set_size( struct block *block, UINT block_size )
{
    block_size /= BLOCK_ALIGN;
    if (block_get_flags( block ) & BLOCK_FLAG_FREE) block->tail_size = block_size >> 16;
    block->block_size = block_size;
}

static inline void block_set_flags( struct block *block, BYTE clear, BYTE set )
{
    UINT block_size = block_get_size( block );
    block->block_flags &= ~clear;
    block->block_flags |= set;
    block_set_size( block, block_size );
}

static inline void *subheap_base( const SUBHEAP *subheap )
{
    return ROUND_ADDR( subheap, REGION_ALIGN - 1 );
}

static inline SIZE_T subheap_overhead( const SUBHEAP *subheap )
{
    return (char *)&subheap->block - (char *)subheap_base( subheap );
}

static inline SIZE_T subheap_size( const SUBHEAP *subheap )
{
    return subheap->block_size + subheap_overhead( subheap );
}

static inline const void *subheap_commit_end( const SUBHEAP *subheap )
{
    return (char *)(subheap + 1) + subheap->data_size;
}

static void subheap_set_bounds( SUBHEAP *subheap, char *commit_end, char *end )
{
    subheap->block_size = end - (char *)&subheap->block;
    subheap->data_size = commit_end - (char *)(subheap + 1);
}

static inline void *first_block( const SUBHEAP *subheap )
{
    return (void *)&subheap->block;
}

static inline const void *last_block( const SUBHEAP *subheap )
{
    return (char *)subheap_commit_end( subheap ) - sizeof(struct block);
}

static inline struct block *next_block( const SUBHEAP *subheap, const struct block *block )
{
    const char *data = (char *)(block + 1), *next, *last = last_block( subheap );
    next = (char *)block + block_get_size( block );
    if (!contains( data, last - (char *)data, next, sizeof(*block) )) return NULL;
    return (struct block *)next;
}

static inline BOOL check_subheap( const SUBHEAP *subheap, const struct heap *heap )
{
    if (subheap->user_value != heap) return FALSE;

    return contains( &subheap->block, subheap->block_size, subheap + 1, subheap->data_size );
}

static BOOL heap_validate( const struct heap *heap );

/* mark a block of memory as innacessible for debugging purposes */
static inline void valgrind_make_noaccess( void const *ptr, SIZE_T size )
{
#if defined(VALGRIND_MAKE_MEM_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_NOACCESS( ptr, size ) );
#elif defined(VALGRIND_MAKE_NOACCESS)
    VALGRIND_DISCARD( VALGRIND_MAKE_NOACCESS( ptr, size ) );
#endif
}

/* mark a block of memory as uninitialized for debugging purposes */
static inline void valgrind_make_writable( void const *ptr, SIZE_T size )
{
#if defined(VALGRIND_MAKE_MEM_UNDEFINED)
    VALGRIND_DISCARD( VALGRIND_MAKE_MEM_UNDEFINED( ptr, size ) );
#elif defined(VALGRIND_MAKE_WRITABLE)
    VALGRIND_DISCARD( VALGRIND_MAKE_WRITABLE( ptr, size ) );
#endif
}

/* mark a block of memory as free for debugging purposes */
static inline void mark_block_free( void *ptr, SIZE_T size, DWORD flags )
{
    if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        SIZE_T i;
        for (i = 0; i < size / sizeof(DWORD); i++) ((DWORD *)ptr)[i] = BLOCK_FILL_FREE;
    }
    valgrind_make_noaccess( ptr, size );
}

/* mark a block of memory as a tail block */
static inline void mark_block_tail( struct block *block, DWORD flags )
{
    char *tail = (char *)block + block_get_size( block ) - block->tail_size;
    if (flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        valgrind_make_writable( tail, BLOCK_ALIGN );
        memset( tail, BLOCK_FILL_TAIL, BLOCK_ALIGN );
    }
    valgrind_make_noaccess( tail, BLOCK_ALIGN );
    if (flags & HEAP_ADD_USER_INFO)
    {
        if (flags & HEAP_TAIL_CHECKING_ENABLED || RUNNING_ON_VALGRIND) tail += BLOCK_ALIGN;
        valgrind_make_writable( tail, BLOCK_ALIGN );
        memset( tail, 0, BLOCK_ALIGN );
    }
}

/* initialize contents of a newly created block of memory */
static inline void initialize_block( struct block *block, SIZE_T old_size, SIZE_T size, DWORD flags )
{
    char *data = (char *)(block + 1);
    SIZE_T i, aligned_size;

    if (size <= old_size) return;

    if (flags & HEAP_ZERO_MEMORY)
    {
        aligned_size = ROUND_SIZE( size, sizeof(void *) - 1 );
        valgrind_make_writable( data + old_size, aligned_size - old_size );
        memset( data + old_size, 0, aligned_size - old_size );
        valgrind_make_noaccess( data + size, aligned_size - size );
    }
    else if (flags & HEAP_FREE_CHECKING_ENABLED)
    {
        valgrind_make_writable( data + old_size, size - old_size );
        i = ROUND_SIZE( old_size, sizeof(DWORD) - 1 ) / sizeof(DWORD);
        for (; i < size / sizeof(DWORD); ++i) ((DWORD *)data)[i] = BLOCK_FILL_USED;
    }
}

/* notify that a new block of memory has been allocated for debugging purposes */
static inline void valgrind_notify_alloc( void const *ptr, SIZE_T size, BOOL init )
{
#ifdef VALGRIND_MALLOCLIKE_BLOCK
    VALGRIND_MALLOCLIKE_BLOCK( ptr, size, 0, init );
#endif
}

/* notify that a block of memory has been freed for debugging purposes */
static inline void valgrind_notify_free( void const *ptr )
{
#ifdef VALGRIND_FREELIKE_BLOCK
    VALGRIND_FREELIKE_BLOCK( ptr, 0 );
#endif
}

static inline void valgrind_notify_resize( void const *ptr, SIZE_T size_old, SIZE_T size_new )
{
#ifdef VALGRIND_RESIZEINPLACE_BLOCK
    /* zero is not a valid size */
    VALGRIND_RESIZEINPLACE_BLOCK( ptr, size_old ? size_old : 1, size_new ? size_new : 1, 0 );
#endif
}

static void valgrind_notify_free_all( SUBHEAP *subheap, const struct heap *heap )
{
#ifdef VALGRIND_FREELIKE_BLOCK
    struct block *block;

    if (!RUNNING_ON_VALGRIND) return;
    if (!check_subheap( subheap, heap )) return;

    for (block = first_block( subheap ); block; block = next_block( subheap, block ))
    {
        if (block_get_flags( block ) & BLOCK_FLAG_FREE) continue;
        if (block_get_type( block ) == BLOCK_TYPE_USED) valgrind_notify_free( block + 1 );
    }
#endif
}

/* get the memory protection type to use for a given heap */
static inline ULONG get_protection_type( DWORD flags )
{
    return (flags & HEAP_CREATE_ENABLE_EXECUTE) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
}

static RTL_CRITICAL_SECTION_DEBUG process_heap_cs_debug =
{
    0, 0, NULL,  /* will be set later */
    { &process_heap_cs_debug.ProcessLocksList, &process_heap_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": main process heap section") }
};

static inline ULONG heap_get_flags( const struct heap *heap, ULONG flags )
{
    if (flags & (HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED)) flags |= HEAP_CHECKING_ENABLED;
    flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY | HEAP_REALLOC_IN_PLACE_ONLY | HEAP_CHECKING_ENABLED | HEAP_USER_FLAGS_MASK;
    return heap->flags | flags;
}

static inline void heap_lock( struct heap *heap, ULONG flags )
{
    if (flags & HEAP_NO_SERIALIZE) return;
    RtlEnterCriticalSection( &heap->cs );
}

static inline void heap_unlock( struct heap *heap, ULONG flags )
{
    if (flags & HEAP_NO_SERIALIZE) return;
    RtlLeaveCriticalSection( &heap->cs );
}

static void heap_set_status( const struct heap *heap, ULONG flags, NTSTATUS status )
{
    if (status == STATUS_NO_MEMORY && (flags & HEAP_GENERATE_EXCEPTIONS)) RtlRaiseStatus( status );
    if (status) RtlSetLastWin32ErrorAndNtStatusFromNtStatus( status );
}

static SIZE_T get_free_list_block_size( unsigned int index )
{
    DWORD log = index >> FREE_LIST_LINEAR_BITS;
    DWORD linear = index & FREE_LIST_LINEAR_MASK;

    if (log == 0) return index * BLOCK_ALIGN;

    return (((1 << FREE_LIST_LINEAR_BITS) + linear) << (log - 1)) * BLOCK_ALIGN;
}

/*
 * Given a size, return its index in the block size list for freelists.
 *
 * With FREE_LIST_LINEAR_BITS=2, the list looks like this
 * (with respect to size / BLOCK_ALIGN):
 *   0,
 *   1,   2,   3,   4,   5,   6,   7,   8,
 *  10,  12,  14,  16,  20,  24,  28,  32,
 *  40,  48,  56,  64,  80,  96, 112, 128,
 * 160, 192, 224, 256, 320, 384, 448, 512,
 * ...
 */
static unsigned int get_free_list_index( SIZE_T block_size )
{
    DWORD bit, log, linear;

    if (block_size > get_free_list_block_size( FREE_LIST_COUNT - 1 ))
        return FREE_LIST_COUNT - 1;

    block_size /= BLOCK_ALIGN;
    /* find the highest bit */
    if (!BitScanReverse( &bit, block_size ) || bit < FREE_LIST_LINEAR_BITS)
    {
        /* for small values, the index is same as block_size. */
        log = 0;
        linear = block_size;
    }
    else
    {
        /* the highest bit is always set, ignore it and encode the next FREE_LIST_LINEAR_BITS bits
         * as a linear scale, combined with the shift as a log scale, in the free list index. */
        log = bit - FREE_LIST_LINEAR_BITS + 1;
        linear = (block_size >> (bit - FREE_LIST_LINEAR_BITS)) & FREE_LIST_LINEAR_MASK;
    }

    return (log << FREE_LIST_LINEAR_BITS) + linear;
}

/* locate a free list entry of the appropriate size */
static inline struct entry *find_free_list( struct heap *heap, SIZE_T block_size, BOOL last )
{
    unsigned int index = get_free_list_index( block_size );
    if (last && ++index == FREE_LIST_COUNT) index = 0;
    return &heap->free_lists[index];
}

static void heap_dump( const struct heap *heap )
{
    const struct block *block;
    const ARENA_LARGE *large;
    const SUBHEAP *subheap;
    unsigned int i;

    TRACE( "heap: %p\n", heap );
    TRACE( "  next %p\n", LIST_ENTRY( heap->entry.next, struct heap, entry ) );

    TRACE( "  bins:\n" );
    for (i = 0; heap->bins && i < BLOCK_SIZE_BIN_COUNT; i++)
    {
        const struct bin *bin = heap->bins + i;
        ULONG alloc = ReadNoFence( &bin->count_alloc ), freed = ReadNoFence( &bin->count_freed );
        if (!alloc && !freed) continue;
        TRACE( "    %3u: size %#4Ix, alloc %ld, freed %ld, enabled %lu\n", i, BLOCK_BIN_SIZE( i ),
               alloc, freed, ReadNoFence( &bin->enabled ) );
    }

    TRACE( "  free_lists: %p\n", heap->free_lists );
    for (i = 0; i < FREE_LIST_COUNT; i++)
        TRACE( "    %p: size %#8Ix, prev %p, next %p\n", heap->free_lists + i, get_free_list_block_size( i ),
               LIST_ENTRY( heap->free_lists[i].entry.prev, struct entry, entry ),
               LIST_ENTRY( heap->free_lists[i].entry.next, struct entry, entry ) );

    TRACE( "  subheaps: %p\n", &heap->subheap_list );
    LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
    {
        SIZE_T free_size = 0, used_size = 0, overhead = 0;
        const char *base = subheap_base( subheap );

        TRACE( "    %p: base %p first %p last %p end %p\n", subheap, base, first_block( subheap ),
               last_block( subheap ), base + subheap_size( subheap ) );

        if (!check_subheap( subheap, heap )) return;

        overhead += subheap_overhead( subheap );
        for (block = first_block( subheap ); block; block = next_block( subheap, block ))
        {
            if (block_get_flags( block ) & BLOCK_FLAG_FREE)
            {
                TRACE( "      %p: (free) type %#10x, size %#8x, flags %#4x, prev %p, next %p\n", block,
                       block_get_type( block ), block_get_size( block ), block_get_flags( block ),
                       LIST_ENTRY( ((struct entry *)block)->entry.prev, struct entry, entry ),
                       LIST_ENTRY( ((struct entry *)block)->entry.next, struct entry, entry ) );

                overhead += block_get_overhead( block );
                free_size += block_get_size( block ) - block_get_overhead( block );
            }
            else
            {
                TRACE( "      %p: (used) type %#10x, size %#8x, flags %#4x, unused %#4x", block,
                       block_get_type( block ), block_get_size( block ), block_get_flags( block ),
                       block->tail_size );
                if (!(block_get_flags( block ) & BLOCK_FLAG_PREV_FREE)) TRACE( "\n" );
                else TRACE( ", back %p\n", *((struct block **)block - 1) );

                overhead += block_get_overhead( block );
                used_size += block_get_size( block ) - block_get_overhead( block );
            }
        }

        TRACE( "      total %#Ix, used %#Ix, free %#Ix, overhead %#Ix (%Iu%%)\n", used_size + free_size + overhead,
               used_size, free_size, overhead, (overhead * 100) / subheap_size( subheap ) );
    }

    TRACE( "  large blocks: %p\n", &heap->large_list );
    LIST_FOR_EACH_ENTRY( large, &heap->large_list, ARENA_LARGE, entry )
    {
        TRACE( "    %p: (large) type %#10x, size %#8x, flags %#4x, total_size %#10Ix, alloc_size %#10Ix, prev %p, next %p\n",
               large, block_get_type( &large->block ), block_get_size( &large->block ), block_get_flags( &large->block ), large->block_size,
               large->data_size, LIST_ENTRY( large->entry.prev, ARENA_LARGE, entry ), LIST_ENTRY( large->entry.next, ARENA_LARGE, entry ) );
    }

    if (heap->pending_free)
    {
        TRACE( "  pending blocks: %p\n", heap->pending_free );
        for (i = 0; i < MAX_FREE_PENDING; ++i)
        {
            if (!(block = heap->pending_free[i])) break;

            TRACE( "   %c%p: (pend) type %#10x, size %#8x, flags %#4x, unused %#4x", i == heap->pending_pos ? '*' : ' ',
                   block, block_get_type( block ), block_get_size( block ), block_get_flags( block ), block->tail_size );
            if (!(block_get_flags( block ) & BLOCK_FLAG_PREV_FREE)) TRACE( "\n" );
            else TRACE( ", back %p\n", *((struct block **)block - 1) );
        }
    }
}

static const char *debugstr_heap_entry( struct rtl_heap_entry *entry )
{
    const char *str = wine_dbg_sprintf( "data %p, size %#Ix, overhead %#x, region %#x, flags %#x", entry->lpData,
                                        entry->cbData, entry->cbOverhead, entry->iRegionIndex, entry->wFlags );
    if (!(entry->wFlags & RTL_HEAP_ENTRY_REGION)) return str;
    return wine_dbg_sprintf( "%s, commit %#lx, uncommit %#lx, first %p, last %p", str, entry->Region.dwCommittedSize,
                             entry->Region.dwUnCommittedSize, entry->Region.lpFirstBlock, entry->Region.lpLastBlock );
}

static struct heap *unsafe_heap_from_handle( HANDLE handle, ULONG flags, ULONG *heap_flags )
{
    struct heap *heap = handle;
    BOOL valid = TRUE;

    if (!heap || (heap->magic != HEAP_MAGIC))
    {
        ERR( "Invalid handle %p!\n", handle );
        return NULL;
    }
    if (heap->flags & HEAP_VALIDATE_ALL)
    {
        heap_lock( heap, 0 );
        valid = heap_validate( heap );
        heap_unlock( heap, 0 );

        if (!valid && TRACE_ON(heap))
        {
            heap_dump( heap );
            assert( FALSE );
        }
    }

    *heap_flags = heap_get_flags( heap, flags );
    return valid ? heap : NULL;
}


static SUBHEAP *find_subheap( const struct heap *heap, const struct block *block, BOOL heap_walk )
{
    SUBHEAP *subheap;

    LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
    {
        SIZE_T blocks_size = (char *)last_block( subheap ) - (char *)first_block( subheap );
        if (!check_subheap( subheap, heap )) return NULL;
        if (contains( first_block( subheap ), blocks_size, block, sizeof(*block) )) return subheap;
        /* outside of blocks region, possible corruption or heap_walk */
        if (contains( subheap_base( subheap ), subheap_size( subheap ), block, 1 )) return heap_walk ? subheap : NULL;
    }

    return NULL;
}


static inline BOOL subheap_commit( const struct heap *heap, SUBHEAP *subheap, const struct block *block, SIZE_T block_size )
{
    const char *end = (char *)subheap_base( subheap ) + subheap_size( subheap ), *commit_end;
    SIZE_T size;
    void *addr;

    commit_end = (char *)block + block_size + sizeof(struct entry);
    commit_end = ROUND_ADDR( (char *)commit_end + REGION_ALIGN - 1, REGION_ALIGN - 1 );

    if (commit_end > end) commit_end = end;
    if (commit_end <= (char *)subheap_commit_end( subheap )) return TRUE;

    addr = (void *)subheap_commit_end( subheap );
    size = commit_end - (char *)addr;

    if (NtAllocateVirtualMemory( NtCurrentProcess(), &addr, 0, &size, MEM_COMMIT,
                                 get_protection_type( heap->flags ) ))
    {
        WARN( "Could not commit %#Ix bytes at %p for heap %p\n", size, addr, heap );
        return FALSE;
    }

    subheap->data_size = (char *)commit_end - (char *)(subheap + 1);
    return TRUE;
}

static inline BOOL subheap_decommit( const struct heap *heap, SUBHEAP *subheap, const void *commit_end )
{
    char *base = subheap_base( subheap );
    SIZE_T size;
    void *addr;

    commit_end = ROUND_ADDR( (char *)commit_end + REGION_ALIGN - 1, REGION_ALIGN - 1 );
    if (subheap == &heap->subheap) commit_end = max( (char *)commit_end, (char *)base + heap->min_size );
    if (commit_end >= subheap_commit_end( subheap )) return TRUE;

    size = (char *)subheap_commit_end( subheap ) - (char *)commit_end;
    addr = (void *)commit_end;

    if (NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_DECOMMIT ))
    {
        WARN( "Could not decommit %#Ix bytes at %p for heap %p\n", size, addr, heap );
        return FALSE;
    }

    subheap->data_size = (char *)commit_end - (char *)(subheap + 1);
    return TRUE;
}

static void block_init_free( struct block *block, ULONG flags, SUBHEAP *subheap, SIZE_T block_size )
{
    const char *end = (char *)block + block_size, *commit_end = subheap_commit_end( subheap );
    struct entry *entry = (struct entry *)block;

    valgrind_make_writable( block, sizeof(*entry) );
    block_set_type( block, BLOCK_TYPE_FREE );
    block_set_base( block, subheap_base( subheap ) );
    block_set_flags( block, ~0, BLOCK_FLAG_FREE );
    block_set_size( block, block_size );

    /* If debugging, erase the freed block content */

    if (end > commit_end) end = commit_end;
    if (end > (char *)(entry + 1)) mark_block_free( entry + 1, end - (char *)(entry + 1), flags );
}

static void insert_free_block( struct heap *heap, ULONG flags, SUBHEAP *subheap, struct block *block )
{
    struct entry *entry = (struct entry *)block, *list;
    struct block *next;

    if ((next = next_block( subheap, block )))
    {
        /* set the next block PREV_FREE flag and back pointer */
        block_set_flags( next, 0, BLOCK_FLAG_PREV_FREE );
        valgrind_make_writable( (struct block **)next - 1, sizeof(struct block *) );
        *((struct block **)next - 1) = block;
    }

    list = find_free_list( heap, block_get_size( block ), !next );
    if (!next) list_add_before( &list->entry, &entry->entry );
    else list_add_after( &list->entry, &entry->entry );
}


static struct block *heap_delay_free( struct heap *heap, ULONG flags, struct block *block )
{
    struct block *tmp;

    if (!heap->pending_free) return block;

    block_set_type( block, BLOCK_TYPE_DEAD );
    mark_block_free( block + 1, block_get_size( block ) - sizeof(*block), flags );

    heap_lock( heap, flags );

    tmp = heap->pending_free[heap->pending_pos];
    heap->pending_free[heap->pending_pos] = block;
    heap->pending_pos = (heap->pending_pos + 1) % MAX_FREE_PENDING;

    heap_unlock( heap, flags );

    return tmp;
}


static NTSTATUS heap_free_block( struct heap *heap, ULONG flags, struct block *block )
{
    SUBHEAP *subheap = block_get_subheap( heap, block );
    SIZE_T block_size = block_get_size( block );
    struct entry *entry;
    struct block *next;

    if ((next = next_block( subheap, block )) && (block_get_flags( next ) & BLOCK_FLAG_FREE))
    {
        /* merge with next block if it is free */
        entry = (struct entry *)next;
        block_size += block_get_size( &entry->block );
        list_remove( &entry->entry );
        next = next_block( subheap, next );
    }

    if (block_get_flags( block ) & BLOCK_FLAG_PREV_FREE)
    {
        /* merge with previous block if it is free */
        entry = *((struct entry **)block - 1);
        block_size += block_get_size( &entry->block );
        list_remove( &entry->entry );
        block = &entry->block;
    }

    if (block == first_block( subheap ) && !next && subheap != &heap->subheap)
    {
        /* free the subheap if it's empty and not the main one */
        void *addr = subheap_base( subheap );
        SIZE_T size = 0;

        list_remove( &subheap->entry );
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
        return STATUS_SUCCESS;
    }

    block_init_free( block, flags, subheap, block_size );
    insert_free_block( heap, flags, subheap, block );

    /* keep room for a full committed block as hysteresis */
    if (!next) subheap_decommit( heap, subheap, (char *)((struct entry *)block + 1) + REGION_ALIGN );

    return STATUS_SUCCESS;
}


static struct block *split_block( struct heap *heap, ULONG flags, struct block *block,
                                  SIZE_T old_block_size, SIZE_T block_size )
{
    SUBHEAP *subheap = block_get_subheap( heap, block );

    if (old_block_size >= block_size + HEAP_MIN_BLOCK_SIZE)
    {
        block_set_size( block, block_size );
        return next_block( subheap, block );
    }

    block_set_size( block, old_block_size );
    return NULL;
}


static void *allocate_region( struct heap *heap, ULONG flags, SIZE_T *region_size, SIZE_T *commit_size )
{
    const SIZE_T align = 0x400 * sizeof(void*);  /* minimum alignment for virtual allocations */
    void *addr = NULL;
    NTSTATUS status;

    if (heap && !(flags & HEAP_GROWABLE))
    {
        WARN( "Heap %p isn't growable, cannot allocate %#Ix bytes\n", heap, *region_size );
        return NULL;
    }

    *region_size = ROUND_SIZE( *region_size, align - 1 );
    *commit_size = ROUND_SIZE( *commit_size, align - 1 );

    /* allocate the memory block */
    if ((status = NtAllocateVirtualMemory( NtCurrentProcess(), &addr, 0, region_size, MEM_RESERVE,
                                           get_protection_type( flags ) )))
    {
        WARN( "Could not allocate %#Ix bytes, status %#lx\n", *region_size, status );
        return NULL;
    }
    if ((status = NtAllocateVirtualMemory( NtCurrentProcess(), &addr, 0, commit_size, MEM_COMMIT,
                                           get_protection_type( flags ) )))
    {
        WARN( "Could not commit %#Ix bytes, status %#lx\n", *commit_size, status );
        return NULL;
    }

    return addr;
}


static NTSTATUS heap_allocate_large( struct heap *heap, ULONG flags, SIZE_T block_size,
                                     SIZE_T size, void **ret )
{
    ARENA_LARGE *arena;
    SIZE_T total_size = ROUND_SIZE( sizeof(*arena) + size, REGION_ALIGN - 1 );
    struct block *block;

    if (total_size < size) return STATUS_NO_MEMORY;  /* overflow */
    if (!(arena = allocate_region( heap, flags, &total_size, &total_size ))) return STATUS_NO_MEMORY;

    block = &arena->block;
    arena->data_size = size;
    arena->block_size = (char *)arena + total_size - (char *)block;

    block_set_type( block, BLOCK_TYPE_LARGE );
    block_set_base( block, arena );
    block_set_flags( block, ~0, BLOCK_FLAG_LARGE | BLOCK_USER_FLAGS( flags ) );
    block_set_size( block, 0 );

    heap_lock( heap, flags );
    list_add_tail( &heap->large_list, &arena->entry );
    heap_unlock( heap, flags );

    valgrind_make_noaccess( (char *)block + sizeof(*block) + arena->data_size,
                            arena->block_size - sizeof(*block) - arena->data_size );

    *ret = block + 1;
    return STATUS_SUCCESS;
}


static NTSTATUS heap_free_large( struct heap *heap, ULONG flags, struct block *block )
{
    ARENA_LARGE *arena = CONTAINING_RECORD( block, ARENA_LARGE, block );
    LPVOID address = arena;
    SIZE_T size = 0;

    heap_lock( heap, flags );
    list_remove( &arena->entry );
    heap_unlock( heap, flags );

    return NtFreeVirtualMemory( NtCurrentProcess(), &address, &size, MEM_RELEASE );
}


static ARENA_LARGE *find_arena_large( const struct heap *heap, const struct block *block, BOOL heap_walk )
{
    ARENA_LARGE *arena;

    LIST_FOR_EACH_ENTRY( arena, &heap->large_list, ARENA_LARGE, entry )
    {
        if (contains( &arena->block, arena->block_size, block, 1 ))
            return !heap_walk || block == &arena->block ? arena : NULL;
    }

    return NULL;
}

static BOOL validate_large_block( const struct heap *heap, const struct block *block )
{
    const ARENA_LARGE *arena = CONTAINING_RECORD( block, ARENA_LARGE, block );
    const char *err = NULL;

    if (ROUND_ADDR( block, REGION_ALIGN - 1 ) != arena)
        err = "invalid block BLOCK_ALIGN";
    else if (block_get_size( block ))
        err = "invalid block size";
    else if (!(block_get_flags( block ) & BLOCK_FLAG_LARGE))
        err = "invalid block flags";
    else if (block_get_type( block ) != BLOCK_TYPE_LARGE)
        err = "invalid block type";
    else if (!contains( block, arena->block_size, block + 1, arena->data_size ))
        err = "invalid block size";

    if (err)
    {
        ERR( "heap %p, block %p: %s\n", heap, block, err );
        if (TRACE_ON(heap)) heap_dump( heap );
    }

    return !err;
}


static SUBHEAP *create_subheap( struct heap *heap, DWORD flags, SIZE_T total_size, SIZE_T commit_size )
{
    SIZE_T block_size;
    SUBHEAP *subheap;

    commit_size = ROUND_SIZE( max( commit_size, REGION_ALIGN ), REGION_ALIGN - 1 );
    total_size = min( max( commit_size, total_size ), 0xffff0000 );  /* don't allow a heap larger than 4GB */

    if (!(subheap = allocate_region( heap, flags, &total_size, &commit_size ))) return NULL;

    subheap->user_value = heap;
    subheap_set_bounds( subheap, (char *)subheap + commit_size, (char *)subheap + total_size );
    block_size = (SIZE_T)ROUND_ADDR( subheap_size( subheap ) - subheap_overhead( subheap ), BLOCK_ALIGN - 1 );
    block_init_free( first_block( subheap ), flags, subheap, block_size );

    list_add_head( &heap->subheap_list, &subheap->entry );

    return subheap;
}


static struct block *find_free_block( struct heap *heap, ULONG flags, SIZE_T block_size )
{
    struct list *ptr = &find_free_list( heap, block_size, FALSE )->entry;
    struct entry *entry;
    struct block *block;
    SIZE_T total_size;
    SUBHEAP *subheap;

    /* Find a suitable free list, and in it find a block large enough */

    while ((ptr = list_next( &heap->free_lists[0].entry, ptr )))
    {
        entry = LIST_ENTRY( ptr, struct entry, entry );
        block = &entry->block;
        if (block_get_flags( block ) == BLOCK_FLAG_FREE_LINK) continue;
        if (block_get_size( block ) >= block_size)
        {
            if (!subheap_commit( heap, block_get_subheap( heap, block ), block, block_size )) return NULL;
            list_remove( &entry->entry );
            return block;
        }
    }

    /* make sure we can fit the block and a free entry at the end */
    total_size = sizeof(SUBHEAP) + block_size + sizeof(struct entry);
    if (total_size < block_size) return NULL;  /* overflow */

    if ((subheap = create_subheap( heap, flags, max( heap->grow_size, total_size ), total_size )))
    {
        heap->grow_size = min( heap->grow_size * 2, HEAP_MAX_GROW_SIZE );
    }
    else while (!subheap)  /* shrink the grow size again if we are running out of space */
    {
        if (heap->grow_size <= total_size || heap->grow_size <= 4 * 1024 * 1024) return NULL;
        heap->grow_size /= 2;
        subheap = create_subheap( heap, flags, max( heap->grow_size, total_size ), total_size );
    }

    TRACE( "created new sub-heap %p of %#Ix bytes for heap %p\n", subheap, subheap_size( subheap ), heap );

    return first_block( subheap );
}


static BOOL is_valid_free_block( const struct heap *heap, const struct block *block )
{
    unsigned int i;

    if (find_subheap( heap, block, FALSE )) return TRUE;
    for (i = 0; i < FREE_LIST_COUNT; i++) if (block == &heap->free_lists[i].block) return TRUE;
    return FALSE;
}

static BOOL validate_free_block( const struct heap *heap, const SUBHEAP *subheap, const struct block *block )
{
    const char *err = NULL, *base = subheap_base( subheap ), *commit_end = subheap_commit_end( subheap );
    const struct entry *entry = (struct entry *)block;
    const struct block *prev, *next;
    DWORD flags = heap->flags;

    if ((ULONG_PTR)(block + 1) % BLOCK_ALIGN)
        err = "invalid block BLOCK_ALIGN";
    else if (block_get_type( block ) != BLOCK_TYPE_FREE)
        err = "invalid block header";
    else if (!(block_get_flags( block ) & BLOCK_FLAG_FREE) || (block_get_flags( block ) & BLOCK_FLAG_PREV_FREE))
        err = "invalid block flags";
    else if (!contains( base, subheap_size( subheap ), block, block_get_size( block ) ))
        err = "invalid block size";
    else if (!is_valid_free_block( heap, (next = &LIST_ENTRY( entry->entry.next, struct entry, entry )->block) ))
        err = "invalid next free block pointer";
    else if (!(block_get_flags( next ) & BLOCK_FLAG_FREE) || block_get_type( next ) != BLOCK_TYPE_FREE)
        err = "invalid next free block header";
    else if (!is_valid_free_block( heap, (prev = &LIST_ENTRY( entry->entry.prev, struct entry, entry )->block) ))
        err = "invalid previous free block pointer";
    else if (!(block_get_flags( prev ) & BLOCK_FLAG_FREE) || block_get_type( prev ) != BLOCK_TYPE_FREE)
        err = "invalid previous free block header";
    else if ((next = next_block( subheap, block )))
    {
        if (!(block_get_flags( next ) & BLOCK_FLAG_PREV_FREE))
            err = "invalid next block flags";
        if (*((struct block **)next - 1) != block)
            err = "invalid next block back pointer";
    }

    if (!err && (flags & HEAP_FREE_CHECKING_ENABLED))
    {
        const char *ptr = (char *)(entry + 1), *end = (char *)block + block_get_size( block );
        if (next) end -= sizeof(struct block *);
        if (end > commit_end) end = commit_end;
        while (!err && ptr < end)
        {
            if (*(DWORD *)ptr != BLOCK_FILL_FREE) err = "free block overwritten";
            ptr += sizeof(DWORD);
        }
    }

    if (err)
    {
        ERR( "heap %p, block %p: %s\n", heap, block, err );
        if (TRACE_ON(heap)) heap_dump( heap );
    }

    return !err;
}


static BOOL validate_used_block( const struct heap *heap, const SUBHEAP *subheap, const struct block *block,
                                 unsigned int expected_block_type )
{
    const char *err = NULL, *base = NULL, *commit_end;
    DWORD flags = heap->flags;
    const struct block *next;
    ARENA_LARGE *arena_large;
    int i;

    if (subheap)
    {
        base = subheap_base( subheap );
        commit_end = subheap_commit_end( subheap );
    }
    else if ((arena_large = find_arena_large( heap, block, FALSE )))
    {
        if (!validate_large_block( heap, &arena_large->block )) return FALSE;
        if (block == &arena_large->block) return TRUE;

        if (contains( &arena_large->block + 1, arena_large->data_size, block, sizeof(*block) )
            && block_get_flags( block ) & BLOCK_FLAG_LFH)
        {
            base = (char *)arena_large;
            commit_end = (char *)(arena_large + 1) + arena_large->data_size;
        }
    }
    if (!base)
    {
        WARN( "heap %p, ptr %p: block region not found.\n", heap, block + 1 );
        return FALSE;
    }

    if ((ULONG_PTR)(block + 1) % BLOCK_ALIGN)
        err = "invalid block BLOCK_ALIGN";
    else if (block_get_type( block ) != BLOCK_TYPE_USED && block_get_type( block ) != BLOCK_TYPE_DEAD)
        err = "invalid block header";
    else if (expected_block_type && block_get_type( block ) != expected_block_type)
        err = "invalid block type";
    else if (block_get_flags( block ) & BLOCK_FLAG_FREE)
        err = "invalid block flags";
    else if (!contains( base, commit_end - base, block, block_get_size( block ) ))
        err = "invalid block size";
    else if (block->tail_size > block_get_size( block ) - sizeof(*block))
        err = "invalid block unused size";
    else if (!(block_get_flags( block ) & BLOCK_FLAG_LFH) /* LFH blocks do not use BLOCK_FLAG_PREV_FREE or back pointer */
             && (next = next_block( subheap, block )) && (block_get_flags( next ) & BLOCK_FLAG_PREV_FREE))
        err = "invalid next block flags";
    else if (block_get_flags( block ) & BLOCK_FLAG_PREV_FREE)
    {
        const struct block *prev = *((struct block **)block - 1);
        if (!is_valid_free_block( heap, prev ))
            err = "invalid previous block pointer";
        else if (!(block_get_flags( prev ) & BLOCK_FLAG_FREE) || block_get_type( prev ) != BLOCK_TYPE_FREE)
            err = "invalid previous block flags";
        if ((char *)prev + block_get_size( prev ) != (char *)block)
            err = "invalid previous block size";
    }

    if (!err && block_get_type( block ) == BLOCK_TYPE_DEAD)
    {
        const char *ptr = (char *)(block + 1), *end = (char *)block + block_get_size( block );
        while (!err && ptr < end)
        {
            if (*(DWORD *)ptr != BLOCK_FILL_FREE) err = "free block overwritten";
            ptr += sizeof(DWORD);
        }
    }
    else if (!err && (flags & HEAP_TAIL_CHECKING_ENABLED))
    {
        const unsigned char *tail = (unsigned char *)block + block_get_size( block ) - block->tail_size;
        for (i = 0; !err && i < BLOCK_ALIGN; i++) if (tail[i] != BLOCK_FILL_TAIL) err = "invalid block tail";
    }

    if (err)
    {
        ERR( "heap %p, block %p: %s\n", heap, block, err );
        if (TRACE_ON(heap)) heap_dump( heap );
    }

    return !err;
}


static BOOL heap_validate_ptr( const struct heap *heap, const void *ptr )
{
    const struct block *block = (struct block *)ptr - 1;

    return validate_used_block( heap, find_subheap( heap, block, FALSE ), block, BLOCK_TYPE_USED );
}

static BOOL heap_validate( const struct heap *heap )
{
    const ARENA_LARGE *large_arena;
    const struct block *block;
    const SUBHEAP *subheap;

    LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
    {
        if (!check_subheap( subheap, heap ))
        {
            ERR( "heap %p, subheap %p corrupted sizes or user_value\n", heap, subheap );
            if (TRACE_ON(heap)) heap_dump( heap );
            return FALSE;
        }

        for (block = first_block( subheap ); block; block = next_block( subheap, block ))
        {
            if (block_get_flags( block ) & BLOCK_FLAG_FREE)
            {
                if (!validate_free_block( heap, subheap, block )) return FALSE;
            }
            else
            {
                if (!validate_used_block( heap, subheap, block, 0 )) return FALSE;
            }
        }
    }

    if (heap->pending_free)
    {
        unsigned int i;

        for (i = 0; i < MAX_FREE_PENDING; i++)
        {
            if (!(block = heap->pending_free[i])) break;

            if (!validate_used_block( heap, find_subheap( heap, block, FALSE ), block, BLOCK_TYPE_DEAD ))
            {
                ERR( "heap %p: failed to to validate delayed free block %p\n", heap, block );
                return FALSE;
            }
        }

        for (; i < MAX_FREE_PENDING; i++)
        {
            if ((block = heap->pending_free[i]))
            {
                ERR( "heap %p: unexpected delayed freed block %p at slot %u\n", heap, block, i );
                if (TRACE_ON(heap)) heap_dump( heap );
                return FALSE;
            }
        }
    }

    LIST_FOR_EACH_ENTRY( large_arena, &heap->large_list, ARENA_LARGE, entry )
        if (!validate_large_block( heap, &large_arena->block )) return FALSE;

    return TRUE;
}

static inline struct block *unsafe_block_from_ptr( struct heap *heap, ULONG flags, const void *ptr )
{
    struct block *block = (struct block *)ptr - 1;
    const char *err = NULL;
    SUBHEAP *subheap;

    if (flags & HEAP_VALIDATE)
    {
        heap_lock( heap, flags );
        if (!heap_validate_ptr( heap, ptr )) block = NULL;
        heap_unlock( heap, flags );
        return block;
    }

    if ((ULONG_PTR)ptr % BLOCK_ALIGN)
        err = "invalid ptr alignment";
    else if (block_get_type( block ) == BLOCK_TYPE_DEAD)
        err = "delayed freed block";
    else if (block_get_type( block ) == BLOCK_TYPE_FREE)
        err = "already freed block";
    else if (block_get_flags( block ) & BLOCK_FLAG_LFH)
    {
        /* LFH block base_offset points to the group, not the subheap */
        if (block_get_type( block ) != BLOCK_TYPE_USED)
            err = "invalid block type";
    }
    else if ((subheap = block_get_subheap( heap, block )) >= (SUBHEAP *)block)
        err = "invalid base offset";
    else if (block_get_type( block ) == BLOCK_TYPE_USED)
    {
        const char *base = subheap_base( subheap ), *commit_end = subheap_commit_end( subheap );
        if (subheap->user_value != heap) err = "mismatching heap";
        else if (!contains( base, commit_end - base, block, block_get_size( block ) )) err = "invalid block size";
    }
    else if (block_get_type( block ) == BLOCK_TYPE_LARGE)
    {
        ARENA_LARGE *large = subheap_base( subheap );
        if (block != &large->block) err = "invalid large block";
    }
    else
    {
        err = "invalid block type";
    }

    if (err) WARN( "heap %p, block %p: %s\n", heap, block, err );
    return err ? NULL : block;
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
    ULONG global_flags = RtlGetNtGlobalFlags();
    DWORD dummy, flags, force_flags;
    struct heap *heap;

    if (TRACE_ON(heap)) global_flags |= FLG_HEAP_VALIDATE_ALL;
    if (WARN_ON(heap)) global_flags |= FLG_HEAP_VALIDATE_PARAMETERS;

    heap = unsafe_heap_from_handle( handle, 0, &dummy );
    flags = heap_flags_from_global_flag( global_flags );
    force_flags = (heap->flags | flags) & ~(HEAP_SHARED|HEAP_DISABLE_COALESCE_ON_FREE);

    if (global_flags & FLG_HEAP_ENABLE_TAGGING) flags |= HEAP_SHARED;
    if (!(global_flags & FLG_HEAP_PAGE_ALLOCS)) force_flags &= ~(HEAP_GROWABLE|HEAP_PRIVATE);

    if (RUNNING_ON_VALGRIND) flags = 0; /* no sense in validating since Valgrind catches accesses */

    heap->flags |= flags;
    heap->force_flags |= force_flags;

    if (flags & (HEAP_FREE_CHECKING_ENABLED | HEAP_TAIL_CHECKING_ENABLED))  /* fix existing blocks */
    {
        struct block *block;
        SUBHEAP *subheap;

        LIST_FOR_EACH_ENTRY( subheap, &heap->subheap_list, SUBHEAP, entry )
        {
            const char *commit_end = subheap_commit_end( subheap );

            if (!check_subheap( subheap, heap )) break;

            for (block = first_block( subheap ); block; block = next_block( subheap, block ))
            {
                if (block_get_flags( block ) & BLOCK_FLAG_FREE)
                {
                    char *data = (char *)block + block_get_overhead( block ), *end = (char *)block + block_get_size( block );
                    if (next_block( subheap, block )) end -= sizeof(struct block *);
                    if (end > commit_end) mark_block_free( data, commit_end - data, flags );
                    else mark_block_free( data, end - data, flags );
                }
                else
                {
                    if (block_get_type( block ) == BLOCK_TYPE_DEAD) mark_block_free( block + 1, block_get_size( block ) - sizeof(*block), flags );
                    else mark_block_tail( block, flags );
                }
            }
        }
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
 */
HANDLE WINAPI RtlCreateHeap( ULONG flags, void *addr, SIZE_T total_size, SIZE_T commit_size,
                             void *lock, RTL_HEAP_PARAMETERS *params )
{
    struct entry *entry;
    struct heap *heap;
    SIZE_T block_size;
    SUBHEAP *subheap;
    unsigned int i;

    TRACE( "flags %#lx, addr %p, total_size %#Ix, commit_size %#Ix, lock %p, params %p\n",
           flags, addr, total_size, commit_size, lock, params );

    flags &= ~(HEAP_TAIL_CHECKING_ENABLED|HEAP_FREE_CHECKING_ENABLED);
    if (process_heap) flags |= HEAP_PRIVATE;
    if (!process_heap || !total_size || (flags & HEAP_SHARED)) flags |= HEAP_GROWABLE;
    if (!total_size) total_size = commit_size + HEAP_INITIAL_SIZE;

    if (!(heap = addr))
    {
        if (!commit_size) commit_size = REGION_ALIGN;
        total_size = min( max( total_size, commit_size ), 0xffff0000 );  /* don't allow a heap larger than 4GB */
        commit_size = min( total_size, ROUND_SIZE( commit_size, REGION_ALIGN - 1 ) );
        if (!(heap = allocate_region( NULL, flags, &total_size, &commit_size ))) return 0;
    }

    heap->ffeeffee      = 0xffeeffee;
    heap->auto_flags    = (flags & HEAP_GROWABLE);
    heap->flags         = (flags & ~HEAP_SHARED);
    heap->compat_info   = HEAP_STD;
    heap->magic         = HEAP_MAGIC;
    heap->grow_size     = HEAP_INITIAL_GROW_SIZE;
    heap->min_size      = commit_size;
    list_init( &heap->subheap_list );
    list_init( &heap->large_list );

    list_init( &heap->free_lists[0].entry );
    for (i = 0, entry = heap->free_lists; i < FREE_LIST_COUNT; i++, entry++)
    {
        block_set_flags( &entry->block, ~0, BLOCK_FLAG_FREE_LINK );
        block_set_size( &entry->block, 0 );
        block_set_type( &entry->block, BLOCK_TYPE_FREE );
        block_set_base( &entry->block, heap );
        if (i) list_add_after( &entry[-1].entry, &entry->entry );
    }

    if (!process_heap)  /* do it by hand to avoid memory allocations */
    {
        heap->cs.DebugInfo      = &process_heap_cs_debug;
        heap->cs.LockCount      = -1;
        heap->cs.RecursionCount = 0;
        heap->cs.OwningThread   = 0;
        heap->cs.LockSemaphore  = 0;
        heap->cs.SpinCount      = 0;
        process_heap_cs_debug.CriticalSection = &heap->cs;
    }
    else
    {
        RtlInitializeCriticalSectionEx( &heap->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
        heap->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": heap.cs");
    }

    subheap = &heap->subheap;
    subheap->user_value = heap;
    subheap_set_bounds( subheap, (char *)heap + commit_size, (char *)heap + total_size );
    block_size = (SIZE_T)ROUND_ADDR( subheap_size( subheap ) - subheap_overhead( subheap ), BLOCK_ALIGN - 1 );
    block_init_free( first_block( subheap ), flags, subheap, block_size );

    insert_free_block( heap, flags, subheap, first_block( subheap ) );
    list_add_head( &heap->subheap_list, &subheap->entry );

    heap_set_debug_flags( heap );

    if (heap->flags & HEAP_GROWABLE)
    {
        SIZE_T size = (sizeof(struct bin) + sizeof(struct group *) * ARRAY_SIZE(affinity_mapping)) * BLOCK_SIZE_BIN_COUNT;
        NtAllocateVirtualMemory( NtCurrentProcess(), (void *)&heap->bins,
                                 0, &size, MEM_COMMIT, PAGE_READWRITE );

        for (i = 0; heap->bins && i < BLOCK_SIZE_BIN_COUNT; ++i)
        {
            RtlInitializeSListHead( &heap->bins[i].groups );
            /* offset affinity_group_base to interleave the bin affinity group pointers */
            heap->bins[i].affinity_group_base = (struct group **)(heap->bins + BLOCK_SIZE_BIN_COUNT) + i;
        }
    }

    /* link it into the per-process heap list */
    if (process_heap)
    {
        RtlEnterCriticalSection( &process_heap->cs );
        list_add_head( &process_heap->entry, &heap->entry );
        RtlLeaveCriticalSection( &process_heap->cs );
    }
    else if (!addr)
    {
        process_heap = heap;  /* assume the first heap we create is the process main heap */
        list_init( &process_heap->entry );
    }

    return heap;
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
HANDLE WINAPI RtlDestroyHeap( HANDLE handle )
{
    SUBHEAP *subheap, *next;
    ARENA_LARGE *arena, *arena_next;
    struct block **pending, **tmp;
    struct heap *heap;
    ULONG heap_flags;
    SIZE_T size;
    void *addr;

    TRACE( "handle %p\n", handle );

    if (!(heap = unsafe_heap_from_handle( handle, 0, &heap_flags )) && handle &&
        (((struct heap *)handle)->flags & HEAP_VALIDATE_PARAMS) &&
        NtCurrentTeb()->Peb->BeingDebugged)
    {
        DbgPrint( "Attempt to destroy an invalid heap\n" );
        DbgBreakPoint();
    }
    if (!heap) return handle;

    if ((pending = heap->pending_free))
    {
        heap->pending_free = NULL;
        for (tmp = pending; *tmp && tmp != pending + MAX_FREE_PENDING; ++tmp)
        {
            if (!heap_free_block_lfh( heap, heap->flags, *tmp )) continue;
            heap_free_block( heap, heap->flags, *tmp );
        }
        RtlFreeHeap( handle, 0, pending );
    }

    if (heap == process_heap) return handle; /* cannot delete the main process heap */

    /* remove it from the per-process list */
    RtlEnterCriticalSection( &process_heap->cs );
    list_remove( &heap->entry );
    RtlLeaveCriticalSection( &process_heap->cs );

    heap->cs.DebugInfo->Spare[0] = 0;
    RtlDeleteCriticalSection( &heap->cs );

    LIST_FOR_EACH_ENTRY_SAFE( arena, arena_next, &heap->large_list, ARENA_LARGE, entry )
    {
        list_remove( &arena->entry );
        size = 0;
        addr = arena;
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    }
    LIST_FOR_EACH_ENTRY_SAFE( subheap, next, &heap->subheap_list, SUBHEAP, entry )
    {
        if (subheap == &heap->subheap) continue;  /* do this one last */
        valgrind_notify_free_all( subheap, heap );
        list_remove( &subheap->entry );
        size = 0;
        addr = ROUND_ADDR( subheap, REGION_ALIGN - 1 );
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    }
    valgrind_notify_free_all( &heap->subheap, heap );
    if ((addr = heap->bins))
    {
        size = 0;
        NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    }
    size = 0;
    addr = heap;
    NtFreeVirtualMemory( NtCurrentProcess(), &addr, &size, MEM_RELEASE );
    return 0;
}

static SIZE_T heap_get_block_size( const struct heap *heap, ULONG flags, SIZE_T size )
{
    static const ULONG padd_flags = HEAP_VALIDATE | HEAP_VALIDATE_ALL | HEAP_VALIDATE_PARAMS | HEAP_ADD_USER_INFO;
    static const ULONG check_flags = HEAP_TAIL_CHECKING_ENABLED | HEAP_FREE_CHECKING_ENABLED | HEAP_CHECKING_ENABLED;
    SIZE_T overhead, block_size;

    if ((flags & check_flags)) overhead = BLOCK_ALIGN;
    else overhead = sizeof(struct block);

    if ((flags & HEAP_TAIL_CHECKING_ENABLED) || RUNNING_ON_VALGRIND) overhead += BLOCK_ALIGN;
    if (flags & padd_flags) overhead += BLOCK_ALIGN;

    if (size < BLOCK_ALIGN) size = BLOCK_ALIGN;
    block_size = ROUND_SIZE( size + overhead, BLOCK_ALIGN - 1 );

    if (block_size < size) return ~0U;  /* overflow */
    if (block_size < HEAP_MIN_BLOCK_SIZE) block_size = HEAP_MIN_BLOCK_SIZE;
    return block_size;
}

static NTSTATUS heap_allocate_block( struct heap *heap, ULONG flags, SIZE_T block_size, SIZE_T size, void **ret )
{
    struct block *block, *next;
    SIZE_T old_block_size;
    SUBHEAP *subheap;

    /* Locate a suitable free block */

    if (!(block = find_free_block( heap, flags, block_size ))) return STATUS_NO_MEMORY;
    /* read the free block size, changing block type or flags may alter it */
    old_block_size = block_get_size( block );
    subheap = block_get_subheap( heap, block );

    if ((next = split_block( heap, flags, block, old_block_size, block_size )))
    {
        block_init_free( next, flags, subheap, old_block_size - block_size );
        insert_free_block( heap, flags, subheap, next );
    }

    block_set_type( block, BLOCK_TYPE_USED );
    block_set_flags( block, ~0, BLOCK_USER_FLAGS( flags ) );
    block->tail_size = block_get_size( block ) - sizeof(*block) - size;
    initialize_block( block, 0, size, flags );
    mark_block_tail( block, flags );

    if ((next = next_block( subheap, block ))) block_set_flags( next, BLOCK_FLAG_PREV_FREE, 0 );

    *ret = block + 1;
    return STATUS_SUCCESS;
}

/* Low Fragmentation Heap frontend */

/* header for every LFH block group */
struct DECLSPEC_ALIGN(BLOCK_ALIGN) group
{
    SLIST_ENTRY entry;
    /* one bit for each free block and the highest bit for GROUP_FLAG_FREE */
    LONG free_bits;
    /* affinity of the thread which last allocated from this group */
    LONG affinity;
    /* first block of a group, required for alignment */
    struct block first_block;
};

#define GROUP_BLOCK_COUNT     (sizeof(((struct group *)0)->free_bits) * 8 - 1)
#define GROUP_FLAG_FREE       (1u << GROUP_BLOCK_COUNT)

static inline UINT block_get_group_index( const struct block *block )
{
    return block->base_offset;
}

static inline struct group *block_get_group( const struct block *block )
{
    SIZE_T block_size = block_get_size( block );
    void *first_block = (char *)block - block_get_group_index( block ) * block_size;
    return CONTAINING_RECORD( first_block, struct group, first_block );
}

static inline void block_set_group( struct block *block, SIZE_T block_size, const struct group *group )
{
    SIZE_T offset = (char *)block - (char *)&group->first_block;
    block->base_offset = offset / block_size;
}

static inline struct block *group_get_block( struct group *group, SIZE_T block_size, UINT index )
{
    char *first_block = (char *)&group->first_block;
    return (struct block *)(first_block + index * block_size);
}

/* lookup a free block using the group free_bits, the current thread must own the group */
static inline struct block *group_find_free_block( struct group *group, SIZE_T block_size )
{
    ULONG i, free_bits = ReadNoFence( &group->free_bits );
    /* free_bits will never be 0 as the group is unlinked when it's fully used */
    BitScanForward( &i, free_bits );
    InterlockedAnd( &group->free_bits, ~(1 << i) );
    return group_get_block( group, block_size, i );
}

/* allocate a new group block using non-LFH allocation, returns a group owned by current thread */
static struct group *group_allocate( struct heap *heap, ULONG flags, SIZE_T block_size )
{
    SIZE_T i, group_size, group_block_size;
    struct group *group;
    NTSTATUS status;

    group_size = offsetof( struct group, first_block ) + GROUP_BLOCK_COUNT * block_size;
    group_block_size = heap_get_block_size( heap, flags, group_size );

    heap_lock( heap, flags );

    if (group_block_size >= HEAP_MIN_LARGE_BLOCK_SIZE)
        status = heap_allocate_large( heap, flags & ~HEAP_ZERO_MEMORY, group_block_size, group_size, (void **)&group );
    else
        status = heap_allocate_block( heap, flags & ~HEAP_ZERO_MEMORY, group_block_size, group_size, (void **)&group );

    heap_unlock( heap, flags );

    if (status) return NULL;

    block_set_flags( (struct block *)group - 1, 0, BLOCK_FLAG_LFH );
    group->free_bits = ~GROUP_FLAG_FREE;

    for (i = 0; i < GROUP_BLOCK_COUNT; ++i)
    {
        struct block *block = group_get_block( group, block_size, i );
        valgrind_make_writable( block, sizeof(*block) );
        block_set_type( block, BLOCK_TYPE_FREE );
        block_set_flags( block, ~0, BLOCK_FLAG_FREE | BLOCK_FLAG_LFH );
        block_set_group( block, block_size, group );
        block_set_size( block, block_size );
        mark_block_free( block + 1, (char *)block + block_size - (char *)(block + 1), flags );
    }

    return group;
}

/* release a fully freed group to the non-LFH subheap, group must be owned by current thread */
static NTSTATUS group_release( struct heap *heap, ULONG flags, struct bin *bin, struct group *group )
{
    struct block *block = (struct block *)group - 1;
    NTSTATUS status;

    heap_lock( heap, flags );

    block_set_flags( block, BLOCK_FLAG_LFH, 0 );

    if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
        status = heap_free_large( heap, flags, block );
    else
        status = heap_free_block( heap, flags, block );

    heap_unlock( heap, flags );

    return status;
}

static inline ULONG heap_current_thread_affinity(void)
{
    ULONG affinity;

    if (!(affinity = NtCurrentTeb()->HeapVirtualAffinity))
    {
        affinity = InterlockedIncrement( &next_thread_affinity );
        affinity = affinity_mapping[affinity % ARRAY_SIZE(affinity_mapping)];
        NtCurrentTeb()->HeapVirtualAffinity = affinity;
    }

    return affinity;
}

/* acquire a group from the bin, thread takes ownership of a shared group or allocates a new one */
static struct group *heap_acquire_bin_group( struct heap *heap, ULONG flags, SIZE_T block_size, struct bin *bin )
{
    ULONG affinity = NtCurrentTeb()->HeapVirtualAffinity;
    struct group *group;
    SLIST_ENTRY *entry;

    if ((group = InterlockedExchangePointer( (void *)bin_get_affinity_group( bin, affinity ), NULL )))
        return group;

    if ((entry = RtlInterlockedPopEntrySList( &bin->groups )))
        return CONTAINING_RECORD( entry, struct group, entry );

    return group_allocate( heap, flags, block_size );
}

/* release a thread owned and fully freed group to the bin shared group, or free its memory */
static NTSTATUS heap_release_bin_group( struct heap *heap, ULONG flags, struct bin *bin, struct group *group )
{
    ULONG affinity = group->affinity;

    /* using InterlockedExchangePointer here would possibly return a group that has used blocks,
     * we prefer keeping our fully freed group instead for reduced memory consumption.
     */
    if (!InterlockedCompareExchangePointer( (void *)bin_get_affinity_group( bin, affinity ), group, NULL ))
        return STATUS_SUCCESS;

    /* try re-using the block group instead of releasing it */
    if (RtlQueryDepthSList( &bin->groups ) <= ARRAY_SIZE(affinity_mapping))
    {
        RtlInterlockedPushEntrySList( &bin->groups, &group->entry );
        return STATUS_SUCCESS;
    }

    return group_release( heap, flags, bin, group );
}

static struct block *find_free_bin_block( struct heap *heap, ULONG flags, SIZE_T block_size, struct bin *bin )
{
    ULONG affinity = heap_current_thread_affinity();
    struct block *block;
    struct group *group;

    /* acquire a group, the thread will own it and no other thread can clear free bits.
     * some other thread might still set the free bits if they are freeing blocks.
     */
    if (!(group = heap_acquire_bin_group( heap, flags, block_size, bin ))) return NULL;
    group->affinity = affinity;

    block = group_find_free_block( group, block_size );

    /* serialize with heap_free_block_lfh: atomically set GROUP_FLAG_FREE when the free bits are all 0. */
    if (ReadNoFence( &group->free_bits ) || InterlockedCompareExchange( &group->free_bits, GROUP_FLAG_FREE, 0 ))
    {
        /* if GROUP_FLAG_FREE isn't set, thread is responsible for putting it back into group list. */
        if ((group = InterlockedExchangePointer( (void *)bin_get_affinity_group( bin, affinity ), group )))
            RtlInterlockedPushEntrySList( &bin->groups, &group->entry );
    }

    return block;
}

static NTSTATUS heap_allocate_block_lfh( struct heap *heap, ULONG flags, SIZE_T block_size,
                                         SIZE_T size, void **ret )
{
    struct bin *bin, *last = heap->bins + BLOCK_SIZE_BIN_COUNT - 1;
    struct block *block;

    bin = heap->bins + BLOCK_SIZE_BIN( block_size );
    if (bin == last) return STATUS_UNSUCCESSFUL;

    /* paired with WriteRelease in bin_try_enable. */
    if (!ReadAcquire( &bin->enabled )) return STATUS_UNSUCCESSFUL;

    block_size = BLOCK_BIN_SIZE( BLOCK_SIZE_BIN( block_size ) );

    if ((block = find_free_bin_block( heap, flags, block_size, bin )))
    {
        block_set_type( block, BLOCK_TYPE_USED );
        block_set_flags( block, (BYTE)~BLOCK_FLAG_LFH, BLOCK_USER_FLAGS( flags ) );
        block->tail_size = block_size - sizeof(*block) - size;
        initialize_block( block, 0, size, flags );
        mark_block_tail( block, flags );
        *ret = block + 1;
    }

    return block ? STATUS_SUCCESS : STATUS_NO_MEMORY;
}

static NTSTATUS heap_free_block_lfh( struct heap *heap, ULONG flags, struct block *block )
{
    struct bin *bin, *last = heap->bins + BLOCK_SIZE_BIN_COUNT - 1;
    SIZE_T i, block_size = block_get_size( block );
    struct group *group = block_get_group( block );
    NTSTATUS status = STATUS_SUCCESS;

    if (!(block_get_flags( block ) & BLOCK_FLAG_LFH)) return STATUS_UNSUCCESSFUL;

    bin = heap->bins + BLOCK_SIZE_BIN( block_size );
    if (bin == last) return STATUS_UNSUCCESSFUL;

    i = block_get_group_index( block );
    valgrind_make_writable( block, sizeof(*block) );
    block_set_type( block, BLOCK_TYPE_FREE );
    block_set_flags( block, (BYTE)~BLOCK_FLAG_LFH, BLOCK_FLAG_FREE );
    mark_block_free( block + 1, (char *)block + block_size - (char *)(block + 1), flags );

    /* if this was the last used block in a group and GROUP_FLAG_FREE was set */
    if (InterlockedOr( &group->free_bits, 1 << i ) == ~(1 << i))
    {
        /* thread now owns the group, and can release it to its bin */
        group->free_bits = ~GROUP_FLAG_FREE;
        status = heap_release_bin_group( heap, flags, bin, group );
    }

    return status;
}

static void bin_try_enable( struct heap *heap, struct bin *bin )
{
    ULONG alloc = ReadNoFence( &bin->count_alloc ), freed = ReadNoFence( &bin->count_freed );
    SIZE_T block_size = BLOCK_BIN_SIZE( bin - heap->bins );
    BOOL enable = FALSE;

    if (bin == heap->bins && alloc > 0x10) enable = TRUE;
    else if (bin - heap->bins < 0x30 && alloc > 0x800) enable = TRUE;
    else if (bin - heap->bins < 0x30 && alloc - freed > 0x10) enable = TRUE;
    else if (alloc - freed > 0x400000 / block_size) enable = TRUE;
    if (!enable) return;

    if (ReadNoFence( &heap->compat_info ) != HEAP_LFH)
    {
        ULONG info = HEAP_LFH;
        RtlSetHeapInformation( heap, HeapCompatibilityInformation, &info, sizeof(info) );
    }

    /* paired with ReadAcquire in heap_allocate_block_lfh.
     *
     * The acq/rel barrier on the enabled flag is protecting compat_info
     * (i.e. compat_info := LFH happens-before enabled := TRUE), so that
     * a caller that observes LFH block allocation (alloc request
     * succeeds without heap lock) will never observe HEAP_STD when it
     * queries the heap.
     */
    WriteRelease( &bin->enabled, TRUE );
}

static void heap_thread_detach_bin_groups( struct heap *heap )
{
    ULONG i, affinity = NtCurrentTeb()->HeapVirtualAffinity;

    if (!heap->bins) return;

    for (i = 0; i < BLOCK_SIZE_BIN_COUNT; ++i)
    {
        struct bin *bin = heap->bins + i;
        struct group *group;
        if (!(group = InterlockedExchangePointer( (void *)bin_get_affinity_group( bin, affinity ), NULL ))) continue;
        RtlInterlockedPushEntrySList( &bin->groups, &group->entry );
    }
}

void heap_thread_detach(void)
{
    struct heap *heap;

    RtlEnterCriticalSection( &process_heap->cs );

    LIST_FOR_EACH_ENTRY( heap, &process_heap->entry, struct heap, entry )
        heap_thread_detach_bin_groups( heap );

    heap_thread_detach_bin_groups( process_heap );

    RtlLeaveCriticalSection( &process_heap->cs );
}

/***********************************************************************
 *           RtlAllocateHeap   (NTDLL.@)
 */
void *WINAPI DECLSPEC_HOTPATCH RtlAllocateHeap( HANDLE handle, ULONG flags, SIZE_T size )
{
    struct heap *heap;
    SIZE_T block_size;
    void *ptr = NULL;
    ULONG heap_flags;
    NTSTATUS status;

    heap = unsafe_heap_from_handle( handle, flags, &heap_flags );
    if ((block_size = heap_get_block_size( heap, heap_flags, size )) == ~0U)
        status = STATUS_NO_MEMORY;
    else if (block_size >= HEAP_MIN_LARGE_BLOCK_SIZE)
        status = heap_allocate_large( heap, heap_flags, block_size, size, &ptr );
    else if (heap->bins && !heap_allocate_block_lfh( heap, heap_flags, block_size, size, &ptr ))
        status = STATUS_SUCCESS;
    else
    {
        heap_lock( heap, heap_flags );
        status = heap_allocate_block( heap, heap_flags, block_size, size, &ptr );
        heap_unlock( heap, heap_flags );

        if (!status && heap->bins)
        {
            SIZE_T bin = BLOCK_SIZE_BIN( block_get_size( (struct block *)ptr - 1 ) );
            InterlockedIncrement( &heap->bins[bin].count_alloc );
            if (!ReadNoFence( &heap->bins[bin].enabled )) bin_try_enable( heap, &heap->bins[bin] );
        }
    }

    if (!status) valgrind_notify_alloc( ptr, size, flags & HEAP_ZERO_MEMORY );

    TRACE( "handle %p, flags %#lx, size %#Ix, return %p, status %#lx.\n", handle, flags, size, ptr, status );
    heap_set_status( heap, flags, status );
    return ptr;
}


/***********************************************************************
 *           RtlFreeHeap   (NTDLL.@)
 */
BOOLEAN WINAPI DECLSPEC_HOTPATCH RtlFreeHeap( HANDLE handle, ULONG flags, void *ptr )
{
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    NTSTATUS status;

    if (!ptr) return TRUE;

    valgrind_notify_free( ptr );

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        status = STATUS_INVALID_PARAMETER;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
        status = STATUS_INVALID_PARAMETER;
    else if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
        status = heap_free_large( heap, heap_flags, block );
    else if (!(block = heap_delay_free( heap, heap_flags, block )))
        status = STATUS_SUCCESS;
    else if (!heap_free_block_lfh( heap, heap_flags, block ))
        status = STATUS_SUCCESS;
    else
    {
        SIZE_T block_size = block_get_size( block ), bin = BLOCK_SIZE_BIN( block_size );

        heap_lock( heap, heap_flags );
        status = heap_free_block( heap, heap_flags, block );
        heap_unlock( heap, heap_flags );

        if (!status && heap->bins) InterlockedIncrement( &heap->bins[bin].count_freed );
    }

    TRACE( "handle %p, flags %#lx, ptr %p, return %u, status %#lx.\n", handle, flags, ptr, !status, status );
    heap_set_status( heap, flags, status );
    return !status;
}

static NTSTATUS heap_resize_large( struct heap *heap, ULONG flags, struct block *block, SIZE_T block_size,
                                   SIZE_T size, SIZE_T *old_size, void **ret )
{
    ARENA_LARGE *large = CONTAINING_RECORD( block, ARENA_LARGE, block );
    SIZE_T old_block_size = large->block_size;
    *old_size = large->data_size;

    if (old_block_size < block_size) return STATUS_NO_MEMORY;

    /* FIXME: we could remap zero-pages instead */
    valgrind_notify_resize( block + 1, *old_size, size );
    initialize_block( block, *old_size, size, flags );

    large->data_size = size;
    valgrind_make_noaccess( (char *)block + sizeof(*block) + large->data_size,
                            old_block_size - sizeof(*block) - large->data_size );

    *ret = block + 1;
    return STATUS_SUCCESS;
}

static NTSTATUS heap_resize_block( struct heap *heap, ULONG flags, struct block *block, SIZE_T block_size,
                                   SIZE_T size, SIZE_T old_block_size, SIZE_T *old_size, void **ret )
{
    SUBHEAP *subheap = block_get_subheap( heap, block );
    struct block *next;

    if (block_size > old_block_size)
    {
        /* need to grow block, make sure it's followed by large enough free block */
        if (!(next = next_block( subheap, block ))) return STATUS_NO_MEMORY;
        if (!(block_get_flags( next ) & BLOCK_FLAG_FREE)) return STATUS_NO_MEMORY;
        if (block_size > old_block_size + block_get_size( next )) return STATUS_NO_MEMORY;
        if (!subheap_commit( heap, subheap, block, block_size )) return STATUS_NO_MEMORY;
    }

    if ((next = next_block( subheap, block )) && (block_get_flags( next ) & BLOCK_FLAG_FREE))
    {
        /* merge with next block if it is free */
        struct entry *entry = (struct entry *)next;
        list_remove( &entry->entry );
        old_block_size += block_get_size( next );
    }

    if ((next = split_block( heap, flags, block, old_block_size, block_size )))
    {
        block_init_free( next, flags, subheap, old_block_size - block_size );
        insert_free_block( heap, flags, subheap, next );
    }

    valgrind_notify_resize( block + 1, *old_size, size );
    block_set_flags( block, BLOCK_FLAG_USER_MASK & ~BLOCK_FLAG_USER_INFO, BLOCK_USER_FLAGS( flags ) );
    block->tail_size = block_get_size( block ) - sizeof(*block) - size;
    initialize_block( block, *old_size, size, flags );
    mark_block_tail( block, flags );

    if ((next = next_block( subheap, block ))) block_set_flags( next, BLOCK_FLAG_PREV_FREE, 0 );

    *ret = block + 1;
    return STATUS_SUCCESS;
}

static NTSTATUS heap_resize_block_lfh( struct block *block, ULONG flags, SIZE_T block_size, SIZE_T size, SIZE_T *old_size, void **ret )
{
    /* as native LFH does it with different block size: refuse to resize even though we could */
    if (ROUND_SIZE( *old_size, BLOCK_ALIGN - 1) != ROUND_SIZE( size, BLOCK_ALIGN - 1)) return STATUS_NO_MEMORY;
    if (size >= *old_size) return STATUS_NO_MEMORY;

    block_size = BLOCK_BIN_SIZE( BLOCK_SIZE_BIN( block_size ) );
    block_set_flags( block, BLOCK_FLAG_USER_MASK & ~BLOCK_FLAG_USER_INFO, BLOCK_USER_FLAGS( flags ) );
    block->tail_size = block_size - sizeof(*block) - size;
    initialize_block( block, *old_size, size, flags );
    mark_block_tail( block, flags );

    *ret = block + 1;
    return STATUS_SUCCESS;
}

static NTSTATUS heap_resize_in_place( struct heap *heap, ULONG flags, struct block *block, SIZE_T block_size,
                                      SIZE_T size, SIZE_T *old_size, void **ret )
{
    SIZE_T old_bin, old_block_size;
    NTSTATUS status;

    if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
        return heap_resize_large( heap, flags, block, block_size, size, old_size, ret );

    old_block_size = block_get_size( block );
    *old_size = old_block_size - block_get_overhead( block );
    old_bin = BLOCK_SIZE_BIN( old_block_size );

    if (block_size >= HEAP_MIN_LARGE_BLOCK_SIZE) return STATUS_NO_MEMORY;  /* growing small block to large block */

    if (block_get_flags( block ) & BLOCK_FLAG_LFH)
        return heap_resize_block_lfh( block, flags, block_size, size, old_size, ret );

    heap_lock( heap, flags );
    status = heap_resize_block( heap, flags, block, block_size, size, old_block_size, old_size, ret );
    heap_unlock( heap, flags );

    if (!status && heap->bins)
    {
        SIZE_T new_bin = BLOCK_SIZE_BIN( block_size );
        InterlockedIncrement( &heap->bins[old_bin].count_freed );
        InterlockedIncrement( &heap->bins[new_bin].count_alloc );
        if (!ReadNoFence( &heap->bins[new_bin].enabled )) bin_try_enable( heap, &heap->bins[new_bin] );
    }

    return status;
}

/***********************************************************************
 *           RtlReAllocateHeap   (NTDLL.@)
 */
void *WINAPI RtlReAllocateHeap( HANDLE handle, ULONG flags, void *ptr, SIZE_T size )
{
    SIZE_T block_size, old_size;
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    void *ret = NULL;
    NTSTATUS status;

    if (!ptr) return NULL;

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        status = STATUS_INVALID_HANDLE;
    else if ((block_size = heap_get_block_size( heap, heap_flags, size )) == ~0U)
        status = STATUS_NO_MEMORY;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
        status = STATUS_INVALID_PARAMETER;
    else if ((status = heap_resize_in_place( heap, heap_flags, block, block_size, size,
                                        &old_size, &ret )))
    {
        if (flags & HEAP_REALLOC_IN_PLACE_ONLY)
            status = STATUS_NO_MEMORY;
        else if (!(ret = RtlAllocateHeap( heap, flags, size )))
            status = STATUS_NO_MEMORY;
        else
        {
            memcpy( ret, ptr, min( size, old_size ) );
            RtlFreeHeap( heap, flags, ptr );
            status = STATUS_SUCCESS;
        }
    }

    TRACE( "handle %p, flags %#lx, ptr %p, size %#Ix, return %p, status %#lx.\n", handle, flags, ptr, size, ret, status );
    heap_set_status( heap, flags, status );
    return ret;
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
ULONG WINAPI RtlCompactHeap( HANDLE handle, ULONG flags )
{
    static BOOL reported;
    if (!reported++) FIXME( "handle %p, flags %#lx stub!\n", handle, flags );
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
BOOLEAN WINAPI RtlLockHeap( HANDLE handle )
{
    struct heap *heap;
    ULONG heap_flags;
    if (!(heap = unsafe_heap_from_handle( handle, 0, &heap_flags ))) return FALSE;
    heap_lock( heap, heap_flags );
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
BOOLEAN WINAPI RtlUnlockHeap( HANDLE handle )
{
    struct heap *heap;
    ULONG heap_flags;
    if (!(heap = unsafe_heap_from_handle( handle, 0, &heap_flags ))) return FALSE;
    heap_unlock( heap, heap_flags );
    return TRUE;
}


static NTSTATUS heap_size( const struct heap *heap, struct block *block, SIZE_T *size )
{
    if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
    {
        const ARENA_LARGE *large_arena = CONTAINING_RECORD( block, ARENA_LARGE, block );
        *size = large_arena->data_size;
    }
    else *size = block_get_size( block ) - block_get_overhead( block );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           RtlSizeHeap   (NTDLL.@)
 */
SIZE_T WINAPI RtlSizeHeap( HANDLE handle, ULONG flags, const void *ptr )
{
    SIZE_T size = ~(SIZE_T)0;
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    NTSTATUS status;

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        status = STATUS_INVALID_PARAMETER;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
        status = STATUS_INVALID_PARAMETER;
    else
    {
        heap_lock( heap, heap_flags );
        status = heap_size( heap, block, &size );
        heap_unlock( heap, heap_flags );
    }

    TRACE( "handle %p, flags %#lx, ptr %p, return %#Ix, status %#lx.\n", handle, flags, ptr, size, status );
    heap_set_status( heap, flags, status );
    return size;
}


/***********************************************************************
 *           RtlValidateHeap   (NTDLL.@)
 */
BOOLEAN WINAPI RtlValidateHeap( HANDLE handle, ULONG flags, const void *ptr )
{
    struct heap *heap;
    ULONG heap_flags;
    BOOLEAN ret;

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        ret = FALSE;
    else
    {
        heap_lock( heap, heap_flags );
        if (ptr) ret = heap_validate_ptr( heap, ptr );
        else ret = heap_validate( heap );
        heap_unlock( heap, heap_flags );
    }

    TRACE( "handle %p, flags %#lx, ptr %p, return %u.\n", handle, flags, ptr, !!ret );
    return ret;
}

static NTSTATUS heap_walk_blocks( const struct heap *heap, const SUBHEAP *subheap,
                                  const struct block *block, struct rtl_heap_entry *entry )
{
    const char *base = subheap_base( subheap ), *commit_end = subheap_commit_end( subheap ), *end = base + subheap_size( subheap );
    const struct block *blocks = first_block( subheap );

    if (entry->lpData == commit_end) return STATUS_NO_MORE_ENTRIES;
    if (entry->lpData == base) block = blocks;
    else if (!(block = next_block( subheap, block )))
    {
        entry->lpData = (void *)commit_end;
        entry->cbData = end - commit_end;
        entry->cbOverhead = 0;
        entry->iRegionIndex = 0;
        entry->wFlags = RTL_HEAP_ENTRY_UNCOMMITTED;
        return STATUS_SUCCESS;
    }

    if (block_get_flags( block ) & BLOCK_FLAG_FREE)
    {
        entry->lpData = (char *)block + block_get_overhead( block );
        entry->cbData = block_get_size( block ) - block_get_overhead( block );
        /* FIXME: last free block should not include uncommitted range, which also has its own overhead */
        if (!contains( blocks, commit_end - 4 * BLOCK_ALIGN - (char *)blocks, block, block_get_size( block ) ))
            entry->cbData = commit_end - 4 * BLOCK_ALIGN - (char *)entry->lpData;
        entry->cbOverhead = 2 * BLOCK_ALIGN;
        entry->iRegionIndex = 0;
        entry->wFlags = 0;
    }
    else
    {
        entry->lpData = (void *)(block + 1);
        entry->cbData = block_get_size( block ) - block_get_overhead( block );
        entry->cbOverhead = block_get_overhead( block );
        entry->iRegionIndex = 0;
        entry->wFlags = RTL_HEAP_ENTRY_COMMITTED|RTL_HEAP_ENTRY_BLOCK|RTL_HEAP_ENTRY_BUSY;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS heap_walk( const struct heap *heap, struct rtl_heap_entry *entry )
{
    const char *data = entry->lpData;
    const ARENA_LARGE *large;
    const struct block *block;
    const struct list *next;
    const SUBHEAP *subheap;
    NTSTATUS status;
    char *base;

    if (!data || entry->wFlags & RTL_HEAP_ENTRY_REGION) block = (struct block *)data;
    else if (entry->wFlags & RTL_HEAP_ENTRY_BUSY) block = (struct block *)data - 1;
    else block = (struct block *)(data - sizeof(struct list)) - 1;

    if ((large = find_arena_large( heap, block, TRUE )))
    {
        next = &large->entry;
    }
    else if ((subheap = find_subheap( heap, block, TRUE )))
    {
        if (!(status = heap_walk_blocks( heap, subheap, block, entry ))) return STATUS_SUCCESS;
        else if (status != STATUS_NO_MORE_ENTRIES) return status;
        next = &subheap->entry;
    }
    else
    {
        if (entry->lpData) return STATUS_INVALID_PARAMETER;
        next = &heap->subheap_list;
    }

    if (!large && (next = list_next( &heap->subheap_list, next )))
    {
        subheap = LIST_ENTRY( next, SUBHEAP, entry );
        base = subheap_base( subheap );
        entry->lpData = base;
        entry->cbData = subheap_overhead( subheap );
        entry->cbOverhead = 0;
        entry->iRegionIndex = 0;
        entry->wFlags = RTL_HEAP_ENTRY_REGION;
        entry->Region.dwCommittedSize = (char *)subheap_commit_end( subheap ) - base;
        entry->Region.dwUnCommittedSize = subheap_size( subheap ) - entry->Region.dwCommittedSize;
        entry->Region.lpFirstBlock = base + entry->cbData;
        entry->Region.lpLastBlock = base + subheap_size( subheap );
        return STATUS_SUCCESS;
    }

    if (!next) next = &heap->large_list;
    if ((next = list_next( &heap->large_list, next )))
    {
        large = LIST_ENTRY( next, ARENA_LARGE, entry );
        entry->lpData = (void *)(large + 1);
        entry->cbData = large->data_size;
        entry->cbOverhead = 0;
        entry->iRegionIndex = 64;
        entry->wFlags = RTL_HEAP_ENTRY_COMMITTED|RTL_HEAP_ENTRY_BLOCK|RTL_HEAP_ENTRY_BUSY;
        return STATUS_SUCCESS;
    }

    return STATUS_NO_MORE_ENTRIES;
}

/***********************************************************************
 *           RtlWalkHeap    (NTDLL.@)
 */
NTSTATUS WINAPI RtlWalkHeap( HANDLE handle, void *entry_ptr )
{
    struct rtl_heap_entry *entry = entry_ptr;
    struct heap *heap;
    ULONG heap_flags;
    NTSTATUS status;

    if (!entry) return STATUS_INVALID_PARAMETER;

    if (!(heap = unsafe_heap_from_handle( handle, 0, &heap_flags )))
        status = STATUS_INVALID_HANDLE;
    else
    {
        heap_lock( heap, heap_flags );
        status = heap_walk( heap, entry );
        heap_unlock( heap, heap_flags );
    }

    TRACE( "handle %p, entry %p %s, return %#lx\n", handle, entry,
           status ? "<empty>" : debugstr_heap_entry(entry), status );
    return status;
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

    RtlEnterCriticalSection( &process_heap->cs );
    LIST_FOR_EACH( ptr, &process_heap->entry ) total++;
    if (total <= count)
    {
        *heaps++ = process_heap;
        LIST_FOR_EACH( ptr, &process_heap->entry )
            *heaps++ = LIST_ENTRY( ptr, struct heap, entry );
    }
    RtlLeaveCriticalSection( &process_heap->cs );
    return total;
}

/***********************************************************************
 *           RtlQueryHeapInformation    (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryHeapInformation( HANDLE handle, HEAP_INFORMATION_CLASS info_class,
                                         void *info, SIZE_T size_in, SIZE_T *size_out )
{
    struct heap *heap;
    ULONG flags;

    TRACE( "handle %p, info_class %u, info %p, size_in %Iu, size_out %p.\n", handle, info_class, info, size_in, size_out );

    switch (info_class)
    {
    case HeapCompatibilityInformation:
        if (!(heap = unsafe_heap_from_handle( handle, 0, &flags ))) return STATUS_ACCESS_VIOLATION;
        if (size_out) *size_out = sizeof(ULONG);
        if (size_in < sizeof(ULONG)) return STATUS_BUFFER_TOO_SMALL;
        *(ULONG *)info = ReadNoFence( &heap->compat_info );
        return STATUS_SUCCESS;

    default:
        FIXME( "HEAP_INFORMATION_CLASS %u not implemented!\n", info_class );
        return STATUS_INVALID_INFO_CLASS;
    }
}

/***********************************************************************
 *           RtlQueryProcessHeapInformation    (NTDLL.@)
 */
NTSTATUS WINAPI RtlQueryProcessHeapInformation( PDEBUG_BUFFER debug_buffer )
{
    FIXME("(%p): stub\n", debug_buffer);
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           RtlSetHeapInformation    (NTDLL.@)
 */
NTSTATUS WINAPI RtlSetHeapInformation( HANDLE handle, HEAP_INFORMATION_CLASS info_class, void *info, SIZE_T size )
{
    struct heap *heap;
    ULONG flags;

    TRACE( "handle %p, info_class %u, info %p, size %Iu.\n", handle, info_class, info, size );

    switch (info_class)
    {
    case HeapCompatibilityInformation:
    {
        ULONG compat_info;

        if (size < sizeof(ULONG)) return STATUS_BUFFER_TOO_SMALL;
        if (!(heap = unsafe_heap_from_handle( handle, 0, &flags ))) return STATUS_INVALID_HANDLE;
        if (heap->flags & HEAP_NO_SERIALIZE) return STATUS_INVALID_PARAMETER;

        compat_info = *(ULONG *)info;
        if (compat_info != HEAP_STD && compat_info != HEAP_LFH)
        {
            FIXME( "HeapCompatibilityInformation %lu not implemented!\n", compat_info );
            return STATUS_UNSUCCESSFUL;
        }
        if (InterlockedCompareExchange( &heap->compat_info, compat_info, HEAP_STD ) != HEAP_STD)
            return STATUS_UNSUCCESSFUL;
        return STATUS_SUCCESS;
    }

    default:
        FIXME( "HEAP_INFORMATION_CLASS %u not implemented!\n", info_class );
        return STATUS_SUCCESS;
    }
}

/***********************************************************************
 *           RtlGetUserInfoHeap    (NTDLL.@)
 */
BOOLEAN WINAPI RtlGetUserInfoHeap( HANDLE handle, ULONG flags, void *ptr, void **user_value, ULONG *user_flags )
{
    NTSTATUS status = STATUS_SUCCESS;
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    char *tmp;

    TRACE( "handle %p, flags %#lx, ptr %p, user_value %p, user_flags %p semi-stub!\n",
           handle, flags, ptr, user_value, user_flags );

    *user_flags = 0;

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        status = STATUS_SUCCESS;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
    {
        status = STATUS_INVALID_PARAMETER;
        *user_value = 0;
    }
    else if (!(*user_flags = HEAP_USER_FLAGS(block_get_flags( block ))))
        WARN( "Block %p wasn't allocated with user info\n", ptr );
    else if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
    {
        const ARENA_LARGE *large = CONTAINING_RECORD( block, ARENA_LARGE, block );
        *user_flags = *user_flags & ~HEAP_ADD_USER_INFO;
        *user_value = large->user_value;
    }
    else
    {
        heap_lock( heap, heap_flags );

        tmp = (char *)block + block_get_size( block ) - block->tail_size + sizeof(void *);
        if ((heap_flags & HEAP_TAIL_CHECKING_ENABLED) || RUNNING_ON_VALGRIND) tmp += BLOCK_ALIGN;
        *user_flags = *user_flags & ~HEAP_ADD_USER_INFO;
        *user_value = *(void **)tmp;

        heap_unlock( heap, heap_flags );
    }

    heap_set_status( heap, flags, status );
    return !status;
}

/***********************************************************************
 *           RtlSetUserValueHeap    (NTDLL.@)
 */
BOOLEAN WINAPI RtlSetUserValueHeap( HANDLE handle, ULONG flags, void *ptr, void *user_value )
{
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    BOOLEAN ret;
    char *tmp;

    TRACE( "handle %p, flags %#lx, ptr %p, user_value %p.\n", handle, flags, ptr, user_value );

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        ret = TRUE;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
        ret = FALSE;
    else if (!(block_get_flags( block ) & BLOCK_FLAG_USER_INFO))
        ret = FALSE;
    else if (block_get_flags( block ) & BLOCK_FLAG_LARGE)
    {
        ARENA_LARGE *large = CONTAINING_RECORD( block, ARENA_LARGE, block );
        large->user_value = user_value;
        ret = TRUE;
    }
    else
    {
        heap_lock( heap, heap_flags );

        tmp = (char *)block + block_get_size( block ) - block->tail_size + sizeof(void *);
        if ((heap_flags & HEAP_TAIL_CHECKING_ENABLED) || RUNNING_ON_VALGRIND) tmp += BLOCK_ALIGN;
        *(void **)tmp = user_value;
        ret = TRUE;

        heap_unlock( heap, heap_flags );
    }

    return ret;
}

/***********************************************************************
 *           RtlSetUserFlagsHeap    (NTDLL.@)
 */
BOOLEAN WINAPI RtlSetUserFlagsHeap( HANDLE handle, ULONG flags, void *ptr, ULONG clear, ULONG set )
{
    struct block *block;
    struct heap *heap;
    ULONG heap_flags;
    BOOLEAN ret;

    TRACE( "handle %p, flags %#lx, ptr %p, clear %#lx, set %#lx.\n", handle, flags, ptr, clear, set );

    if ((clear | set) & ~(0xe00))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(heap = unsafe_heap_from_handle( handle, flags, &heap_flags )))
        ret = TRUE;
    else if (!(block = unsafe_block_from_ptr( heap, heap_flags, ptr )))
        ret = FALSE;
    else if (!(block_get_flags( block ) & BLOCK_FLAG_USER_INFO))
        ret = FALSE;
    else
    {
        block_set_flags( block, BLOCK_USER_FLAGS( clear ), BLOCK_USER_FLAGS( set ) );
        ret = TRUE;
    }

    return ret;
}
