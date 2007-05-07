/*
 * Selector manipulation functions
 *
 * Copyright 1995 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <string.h>

#include "wine/winbase16.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "kernel_private.h"
#include "toolhelp.h"

WINE_DEFAULT_DEBUG_CHANNEL(selector);

#define LDT_SIZE 8192

/* get the number of selectors needed to cover up to the selector limit */
static inline WORD get_sel_count( WORD sel )
{
    return (wine_ldt_copy.limit[sel >> __AHSHIFT] >> 16) + 1;
}


/***********************************************************************
 *           AllocSelectorArray   (KERNEL.206)
 */
WORD WINAPI AllocSelectorArray16( WORD count )
{
    WORD i, sel = wine_ldt_alloc_entries( count );

    if (sel)
    {
        LDT_ENTRY entry;
        wine_ldt_set_base( &entry, 0 );
        wine_ldt_set_limit( &entry, 1 ); /* avoid 0 base and limit */
        wine_ldt_set_flags( &entry, WINE_LDT_FLAGS_DATA );
        for (i = 0; i < count; i++) wine_ldt_set_entry( sel + (i << __AHSHIFT), &entry );
    }
    return sel;
}


/***********************************************************************
 *           AllocSelector   (KERNEL.175)
 */
WORD WINAPI AllocSelector16( WORD sel )
{
    WORD newsel, count, i;

    count = sel ? get_sel_count(sel) : 1;
    newsel = wine_ldt_alloc_entries( count );
    TRACE("(%04x): returning %04x\n", sel, newsel );
    if (!newsel) return 0;
    if (!sel) return newsel;  /* nothing to copy */
    for (i = 0; i < count; i++)
    {
        LDT_ENTRY entry;
        wine_ldt_get_entry( sel + (i << __AHSHIFT), &entry );
        wine_ldt_set_entry( newsel + (i << __AHSHIFT), &entry );
    }
    return newsel;
}


/***********************************************************************
 *           FreeSelector   (KERNEL.176)
 */
WORD WINAPI FreeSelector16( WORD sel )
{
    LDT_ENTRY entry;

    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return sel;  /* error */
#ifdef __i386__
    /* Check if we are freeing current %fs selector */
    if (!((wine_get_fs() ^ sel) & ~3))
        WARN("Freeing %%fs selector (%04x), not good.\n", wine_get_fs() );
#endif  /* __i386__ */
    wine_ldt_free_entries( sel, 1 );
    return 0;
}


/***********************************************************************
 *           SELECTOR_SetEntries
 *
 * Set the LDT entries for an array of selectors.
 */
static void SELECTOR_SetEntries( WORD sel, const void *base, DWORD size, unsigned char flags )
{
    LDT_ENTRY entry;
    WORD i, count;

    wine_ldt_set_base( &entry, base );
    wine_ldt_set_limit( &entry, size - 1 );
    wine_ldt_set_flags( &entry, flags );
    count = (size + 0xffff) / 0x10000;
    for (i = 0; i < count; i++)
    {
        wine_ldt_set_entry( sel + (i << __AHSHIFT), &entry );
        wine_ldt_set_base( &entry, (char*)wine_ldt_get_base(&entry) + 0x10000);
        /* yep, Windows sets limit like that, not 64K sel units */
        wine_ldt_set_limit( &entry, wine_ldt_get_limit(&entry) - 0x10000 );
    }
}


/***********************************************************************
 *           SELECTOR_AllocBlock
 *
 * Allocate selectors for a block of linear memory.
 */
WORD SELECTOR_AllocBlock( const void *base, DWORD size, unsigned char flags )
{
    WORD sel, count;

    if (!size) return 0;
    count = (size + 0xffff) / 0x10000;
    sel = wine_ldt_alloc_entries( count );
    if (sel) SELECTOR_SetEntries( sel, base, size, flags );
    return sel;
}


/***********************************************************************
 *           SELECTOR_FreeBlock
 *
 * Free a block of selectors.
 */
void SELECTOR_FreeBlock( WORD sel )
{
    WORD i, count = get_sel_count( sel );

    TRACE("(%04x,%d)\n", sel, count );
    for (i = 0; i < count; i++) FreeSelector16( sel + (i << __AHSHIFT) );
}


/***********************************************************************
 *           SELECTOR_ReallocBlock
 *
 * Change the size of a block of selectors.
 */
WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size )
{
    LDT_ENTRY entry;
    int oldcount, newcount;

    if (!size) size = 1;
    wine_ldt_get_entry( sel, &entry );
    oldcount = (wine_ldt_get_limit(&entry) >> 16) + 1;
    newcount = (size + 0xffff) >> 16;

    sel = wine_ldt_realloc_entries( sel, oldcount, newcount );
    if (sel) SELECTOR_SetEntries( sel, base, size, wine_ldt_get_flags(&entry) );
    return sel;
}


/***********************************************************************
 *           PrestoChangoSelector   (KERNEL.177)
 */
WORD WINAPI PrestoChangoSelector16( WORD selSrc, WORD selDst )
{
    LDT_ENTRY entry;
    wine_ldt_get_entry( selSrc, &entry );
    /* toggle the executable bit */
    entry.HighWord.Bits.Type ^= (WINE_LDT_FLAGS_CODE ^ WINE_LDT_FLAGS_DATA);
    wine_ldt_set_entry( selDst, &entry );
    return selDst;
}


/***********************************************************************
 *           AllocCStoDSAlias   (KERNEL.170)
 *           AllocAlias         (KERNEL.172)
 */
WORD WINAPI AllocCStoDSAlias16( WORD sel )
{
    WORD newsel;
    LDT_ENTRY entry;

    newsel = wine_ldt_alloc_entries( 1 );
    TRACE("(%04x): returning %04x\n",
                      sel, newsel );
    if (!newsel) return 0;
    wine_ldt_get_entry( sel, &entry );
    entry.HighWord.Bits.Type = WINE_LDT_FLAGS_DATA;
    wine_ldt_set_entry( newsel, &entry );
    return newsel;
}


/***********************************************************************
 *           AllocDStoCSAlias   (KERNEL.171)
 */
WORD WINAPI AllocDStoCSAlias16( WORD sel )
{
    WORD newsel;
    LDT_ENTRY entry;

    newsel = wine_ldt_alloc_entries( 1 );
    TRACE("(%04x): returning %04x\n",
                      sel, newsel );
    if (!newsel) return 0;
    wine_ldt_get_entry( sel, &entry );
    entry.HighWord.Bits.Type = WINE_LDT_FLAGS_CODE;
    wine_ldt_set_entry( newsel, &entry );
    return newsel;
}


/***********************************************************************
 *           LongPtrAdd   (KERNEL.180)
 */
void WINAPI LongPtrAdd16( DWORD ptr, DWORD add )
{
    LDT_ENTRY entry;
    wine_ldt_get_entry( SELECTOROF(ptr), &entry );
    wine_ldt_set_base( &entry, (char *)wine_ldt_get_base(&entry) + add );
    wine_ldt_set_entry( SELECTOROF(ptr), &entry );
}


/***********************************************************************
 *             GetSelectorBase   (KERNEL.186)
 */
DWORD WINAPI GetSelectorBase( WORD sel )
{
    void *base = wine_ldt_copy.base[sel >> __AHSHIFT];

    /* if base points into DOSMEM, assume we have to
     * return pointer into physical lower 1MB */

    return DOSMEM_MapLinearToDos( base );
}


/***********************************************************************
 *             SetSelectorBase   (KERNEL.187)
 */
WORD WINAPI SetSelectorBase( WORD sel, DWORD base )
{
    LDT_ENTRY entry;
    wine_ldt_get_entry( sel, &entry );
    wine_ldt_set_base( &entry, DOSMEM_MapDosToLinear(base) );
    wine_ldt_set_entry( sel, &entry );
    return sel;
}


/***********************************************************************
 *           GetSelectorLimit   (KERNEL.188)
 */
DWORD WINAPI GetSelectorLimit16( WORD sel )
{
    return wine_ldt_copy.limit[sel >> __AHSHIFT];
}


/***********************************************************************
 *           SetSelectorLimit   (KERNEL.189)
 */
WORD WINAPI SetSelectorLimit16( WORD sel, DWORD limit )
{
    LDT_ENTRY entry;
    wine_ldt_get_entry( sel, &entry );
    wine_ldt_set_limit( &entry, limit );
    wine_ldt_set_entry( sel, &entry );
    return sel;
}


/***********************************************************************
 *           SelectorAccessRights   (KERNEL.196)
 */
WORD WINAPI SelectorAccessRights16( WORD sel, WORD op, WORD val )
{
    LDT_ENTRY entry;
    wine_ldt_get_entry( sel, &entry );

    if (op == 0)  /* get */
    {
        return entry.HighWord.Bytes.Flags1 | ((entry.HighWord.Bytes.Flags2 << 8) & 0xf0);
    }
    else  /* set */
    {
        entry.HighWord.Bytes.Flags1 = LOBYTE(val) | 0xf0;
        entry.HighWord.Bytes.Flags2 = (entry.HighWord.Bytes.Flags2 & 0x0f) | (HIBYTE(val) & 0xf0);
        wine_ldt_set_entry( sel, &entry );
        return 0;
    }
}


/***********************************************************************
 *           IsBadCodePtr   (KERNEL.336)
 */
BOOL16 WINAPI IsBadCodePtr16( SEGPTR lpfn )
{
    WORD sel;
    LDT_ENTRY entry;

    sel = SELECTOROF(lpfn);
    if (!sel) return TRUE;
    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return TRUE;
    /* check for code segment, ignoring conforming, read-only and accessed bits */
    if ((entry.HighWord.Bits.Type ^ WINE_LDT_FLAGS_CODE) & 0x18) return TRUE;
    if (OFFSETOF(lpfn) > wine_ldt_get_limit(&entry)) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadStringPtr   (KERNEL.337)
 */
BOOL16 WINAPI IsBadStringPtr16( SEGPTR ptr, UINT16 size )
{
    WORD sel;
    LDT_ENTRY entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return TRUE;
    /* check for data or readable code segment */
    if (!(entry.HighWord.Bits.Type & 0x10)) return TRUE;  /* system descriptor */
    if ((entry.HighWord.Bits.Type & 0x0a) == 0x08) return TRUE;  /* non-readable code segment */
    if (strlen(MapSL(ptr)) < size) size = strlen(MapSL(ptr)) + 1;
    if (size && (OFFSETOF(ptr) + size - 1 > wine_ldt_get_limit(&entry))) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeReadPtr   (KERNEL.346)
 */
BOOL16 WINAPI IsBadHugeReadPtr16( SEGPTR ptr, DWORD size )
{
    WORD sel;
    LDT_ENTRY entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return TRUE;
    /* check for data or readable code segment */
    if (!(entry.HighWord.Bits.Type & 0x10)) return TRUE;  /* system descriptor */
    if ((entry.HighWord.Bits.Type & 0x0a) == 0x08) return TRUE;  /* non-readable code segment */
    if (size && (OFFSETOF(ptr) + size - 1 > wine_ldt_get_limit( &entry ))) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeWritePtr   (KERNEL.347)
 */
BOOL16 WINAPI IsBadHugeWritePtr16( SEGPTR ptr, DWORD size )
{
    WORD sel;
    LDT_ENTRY entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return TRUE;
    /* check for writable data segment, ignoring expand-down and accessed flags */
    if ((entry.HighWord.Bits.Type ^ WINE_LDT_FLAGS_DATA) & ~5) return TRUE;
    if (size && (OFFSETOF(ptr) + size - 1 > wine_ldt_get_limit( &entry ))) return TRUE;
    return FALSE;
}

/***********************************************************************
 *           IsBadReadPtr   (KERNEL.334)
 */
BOOL16 WINAPI IsBadReadPtr16( SEGPTR ptr, UINT16 size )
{
    return IsBadHugeReadPtr16( ptr, size );
}


/***********************************************************************
 *           IsBadWritePtr   (KERNEL.335)
 */
BOOL16 WINAPI IsBadWritePtr16( SEGPTR ptr, UINT16 size )
{
    return IsBadHugeWritePtr16( ptr, size );
}


/***********************************************************************
 *           IsBadFlatReadWritePtr   (KERNEL.627)
 */
BOOL16 WINAPI IsBadFlatReadWritePtr16( SEGPTR ptr, DWORD size, BOOL16 bWrite )
{
    return bWrite? IsBadHugeWritePtr16( ptr, size )
                 : IsBadHugeReadPtr16( ptr, size );
}


/***********************************************************************
 *           MemoryRead   (TOOLHELP.78)
 */
DWORD WINAPI MemoryRead16( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    LDT_ENTRY entry;
    DWORD limit;

    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return 0;
    limit = wine_ldt_get_limit( &entry );
    if (offset > limit) return 0;
    if (offset + count > limit + 1) count = limit + 1 - offset;
    memcpy( buffer, (char *)wine_ldt_get_base(&entry) + offset, count );
    return count;
}


/***********************************************************************
 *           MemoryWrite   (TOOLHELP.79)
 */
DWORD WINAPI MemoryWrite16( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    LDT_ENTRY entry;
    DWORD limit;

    wine_ldt_get_entry( sel, &entry );
    if (wine_ldt_is_empty( &entry )) return 0;
    limit = wine_ldt_get_limit( &entry );
    if (offset > limit) return 0;
    if (offset + count > limit) count = limit + 1 - offset;
    memcpy( (char *)wine_ldt_get_base(&entry) + offset, buffer, count );
    return count;
}

/************************************* Win95 pointer mapping functions *
 *
 */

struct mapls_entry
{
    struct mapls_entry *next;
    void               *addr;   /* linear address */
    int                 count;  /* ref count */
    WORD                sel;    /* selector */
};

static struct mapls_entry *first_entry;


/***********************************************************************
 *           MapLS   (KERNEL32.@)
 *           MapLS   (KERNEL.358)
 *
 * Maps linear pointer to segmented.
 */
SEGPTR WINAPI MapLS( LPCVOID ptr )
{
    struct mapls_entry *entry, *free = NULL;
    const void *base;
    SEGPTR ret = 0;

    if (!HIWORD(ptr)) return (SEGPTR)LOWORD(ptr);

    base = (const char *)ptr - ((unsigned int)ptr & 0x7fff);
    HeapLock( GetProcessHeap() );
    for (entry = first_entry; entry; entry = entry->next)
    {
        if (entry->addr == base) break;
        if (!entry->count) free = entry;
    }

    if (!entry)
    {
        if (!free)  /* no free entry found, create a new one */
        {
            if (!(free = HeapAlloc( GetProcessHeap(), 0, sizeof(*free) ))) goto done;
            if (!(free->sel = SELECTOR_AllocBlock( base, 0x10000, WINE_LDT_FLAGS_DATA )))
            {
                HeapFree( GetProcessHeap(), 0, free );
                goto done;
            }
            free->count = 0;
            free->next = first_entry;
            first_entry = free;
        }
        SetSelectorBase( free->sel, (DWORD)base );
        free->addr = (void*)base;
        entry = free;
    }
    entry->count++;
    ret = MAKESEGPTR( entry->sel, (const char *)ptr - (char *)entry->addr );
 done:
    HeapUnlock( GetProcessHeap() );
    return ret;
}

/***********************************************************************
 *           UnMapLS   (KERNEL32.@)
 *           UnMapLS   (KERNEL.359)
 *
 * Free mapped selector.
 */
void WINAPI UnMapLS( SEGPTR sptr )
{
    struct mapls_entry *entry;
    WORD sel = SELECTOROF(sptr);

    if (sel)
    {
        HeapLock( GetProcessHeap() );
        for (entry = first_entry; entry; entry = entry->next) if (entry->sel == sel) break;
        if (entry && entry->count > 0) entry->count--;
        HeapUnlock( GetProcessHeap() );
    }
}

/***********************************************************************
 *           MapSL   (KERNEL32.@)
 *           MapSL   (KERNEL.357)
 *
 * Maps fixed segmented pointer to linear.
 */
LPVOID WINAPI MapSL( SEGPTR sptr )
{
    return (char *)wine_ldt_copy.base[SELECTOROF(sptr) >> __AHSHIFT] + OFFSETOF(sptr);
}

/***********************************************************************
 *           MapSLFix   (KERNEL32.@)
 *
 * FIXME: MapSLFix and UnMapSLFixArray should probably prevent
 * unexpected linear address change when GlobalCompact() shuffles
 * moveable blocks.
 */

LPVOID WINAPI MapSLFix( SEGPTR sptr )
{
    return MapSL(sptr);
}

/***********************************************************************
 *           UnMapSLFixArray   (KERNEL32.@)
 *
 * Must not change EAX, hence defined as asm function.
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC( UnMapSLFixArray, "ret $8" )
#endif


/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32.@)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE hthread, DWORD sel, LPLDT_ENTRY ldtent )
{
    THREAD_DESCRIPTOR_INFORMATION       tdi;
    NTSTATUS                            status;

    tdi.Selector = sel;
    status = NtQueryInformationThread( hthread, ThreadDescriptorTableEntry,
                                       &tdi, sizeof(tdi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    *ldtent = tdi.Entry;
    return TRUE;
}


#ifdef __i386__

/***********************************************************************
 *		SMapLS (KERNEL32.@)
 */
__ASM_GLOBAL_FUNC( SMapLS,
                   "xor %edx,%edx\n\t"
                   "testl $0xffff0000,%eax\n\t"
                   "jz 1f\n\t"
                   "pushl %eax\n\t"
                   "call " __ASM_NAME("MapLS") "\n\t"
                   "movl %eax,%edx\n"
                   "1:\tret" )

/***********************************************************************
 *		SUnMapLS (KERNEL32.@)
 */
__ASM_GLOBAL_FUNC( SUnMapLS,
                   "pushl %eax\n\t"  /* preserve eax */
                   "pushl %eax\n\t"
                   "call " __ASM_NAME("UnMapLS") "\n\t"
                   "popl %eax\n\t"
                   "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_8 (KERNEL32.@)
 *		SMapLS_IP_EBP_12 (KERNEL32.@)
 *		SMapLS_IP_EBP_16 (KERNEL32.@)
 *		SMapLS_IP_EBP_20 (KERNEL32.@)
 *		SMapLS_IP_EBP_24 (KERNEL32.@)
 *		SMapLS_IP_EBP_28 (KERNEL32.@)
 *		SMapLS_IP_EBP_32 (KERNEL32.@)
 *		SMapLS_IP_EBP_36 (KERNEL32.@)
 *		SMapLS_IP_EBP_40 (KERNEL32.@)
 *
 * These functions map linear pointers at [EBP+xxx] to segmented pointers
 * and return them.
 * Win95 uses some kind of alias structs, which it stores in [EBP+x] to
 * unravel them at SUnMapLS. We just store the segmented pointer there.
 */
#define DEFINE_SMapLS(n) \
  __ASM_GLOBAL_FUNC( SMapLS_IP_EBP_ ## n, \
                     "movl " #n "(%ebp),%eax\n\t" \
                     "call " __ASM_NAME("SMapLS") "\n\t" \
                     "movl %edx," #n "(%ebp)\n\t" \
                     "ret" )

DEFINE_SMapLS(8)
DEFINE_SMapLS(12)
DEFINE_SMapLS(16)
DEFINE_SMapLS(20)
DEFINE_SMapLS(24)
DEFINE_SMapLS(28)
DEFINE_SMapLS(32)
DEFINE_SMapLS(36)
DEFINE_SMapLS(40)


/***********************************************************************
 *		SUnMapLS_IP_EBP_8 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_12 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_16 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_20 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_24 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_28 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_32 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_36 (KERNEL32.@)
 *		SUnMapLS_IP_EBP_40 (KERNEL32.@)
 */

#define DEFINE_SUnMapLS(n) \
  __ASM_GLOBAL_FUNC( SUnMapLS_IP_EBP_ ## n, \
                     "pushl %eax\n\t"  /* preserve eax */ \
                     "pushl " #n "(%ebp)\n\t" \
                     "call " __ASM_NAME("UnMapLS") "\n\t" \
                     "movl $0," #n "(%ebp)\n\t" \
                     "popl %eax\n\t" \
                     "ret" )

DEFINE_SUnMapLS(8)
DEFINE_SUnMapLS(12)
DEFINE_SUnMapLS(16)
DEFINE_SUnMapLS(20)
DEFINE_SUnMapLS(24)
DEFINE_SUnMapLS(28)
DEFINE_SUnMapLS(32)
DEFINE_SUnMapLS(36)
DEFINE_SUnMapLS(40)

#endif  /* __i386__ */
