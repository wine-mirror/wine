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
#include "kernel16_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(selector);

#define LDT_SIZE 8192

static ULONG bitmap_data[LDT_SIZE / 32];
static RTL_BITMAP ldt_bitmap = { LDT_SIZE, bitmap_data };
static const LDT_ENTRY null_entry;
static WORD first_ldt_entry = 32;

/* get the number of selectors needed to cover up to the selector limit */
static inline WORD get_sel_count( WORD sel )
{
    return (wine_ldt_copy.limit[sel >> __AHSHIFT] >> 16) + 1;
}

static inline int is_gdt_sel( WORD sel )
{
    return !(sel & 4);
}

/***********************************************************************
 *           init_selectors
 */
void init_selectors(void)
{
    if (!is_gdt_sel( wine_get_gs() )) first_ldt_entry += 512;
    if (!is_gdt_sel( wine_get_fs() )) first_ldt_entry += 512;
    RtlSetBits( &ldt_bitmap, 0, first_ldt_entry );
}

/***********************************************************************
 *           ldt_is_system
 */
BOOL ldt_is_system( WORD sel )
{
    return is_gdt_sel( sel ) || ((sel >> 3) < first_ldt_entry);
}

/***********************************************************************
 *           ldt_get_ptr
 */
void *ldt_get_ptr( WORD sel, DWORD offset )
{
    ULONG index = sel >> 3;

    if (ldt_is_system( sel )) return (void *)offset;
    if (!(wine_ldt_copy.flags[index] & WINE_LDT_FLAGS_32BIT)) offset &= 0xffff;
    return (char *)wine_ldt_copy.base[index] + offset;
}

/***********************************************************************
 *           ldt_get_entry
 */
BOOL ldt_get_entry( WORD sel, LDT_ENTRY *entry )
{
    ULONG index = sel >> 3;

    if (ldt_is_system( sel ) || !(wine_ldt_copy.flags[index] & WINE_LDT_FLAGS_ALLOCATED))
    {
        *entry = null_entry;
        return FALSE;
    }
    wine_ldt_set_base(  entry, wine_ldt_copy.base[index] );
    wine_ldt_set_limit( entry, wine_ldt_copy.limit[index] );
    wine_ldt_set_flags( entry, wine_ldt_copy.flags[index] );
    return TRUE;
}

/***********************************************************************
 *           ldt_set_entry
 */
void ldt_set_entry( WORD sel, LDT_ENTRY entry )
{
    NtSetLdtEntries( sel, entry, 0, null_entry );
}


static ULONG alloc_entries( ULONG count )
{
    ULONG idx = RtlFindClearBitsAndSet( &ldt_bitmap, count, first_ldt_entry );

    if (idx == ~0u) return 0;
    return (idx << 3) | 7;
}

static void free_entries( ULONG sel, ULONG count )
{
    RtlClearBits( &ldt_bitmap, sel >> 3, count );
    while (count--)
    {
        ldt_set_entry( sel, null_entry );
        sel += 8;
    }
}


/***********************************************************************
 *           AllocSelectorArray   (KERNEL.206)
 */
WORD WINAPI AllocSelectorArray16( WORD count )
{
    WORD i, sel = alloc_entries( count );

    if (sel)
    {
        LDT_ENTRY entry;
        wine_ldt_set_base( &entry, 0 );
        wine_ldt_set_limit( &entry, 1 ); /* avoid 0 base and limit */
        wine_ldt_set_flags( &entry, WINE_LDT_FLAGS_DATA );
        for (i = 0; i < count; i++) ldt_set_entry( sel + (i << 3), entry );
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
    newsel = alloc_entries( count );
    TRACE("(%04x): returning %04x\n", sel, newsel );
    if (!newsel) return 0;
    if (!sel) return newsel;  /* nothing to copy */
    for (i = 0; i < count; i++)
    {
        LDT_ENTRY entry;
        if (!ldt_get_entry( sel + (i << 3), &entry )) break;
        ldt_set_entry( newsel + (i << 3), entry );
    }
    return newsel;
}


/***********************************************************************
 *           FreeSelector   (KERNEL.176)
 */
WORD WINAPI FreeSelector16( WORD sel )
{
    WORD idx = sel >> 3;

    if (idx < first_ldt_entry) return sel;  /* error */
    if (!RtlAreBitsSet( &ldt_bitmap, idx, 1 )) return sel;  /* error */
    free_entries( sel, 1 );
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
    WORD i, count = (size + 0xffff) / 0x10000;

    wine_ldt_set_flags( &entry, flags );
    for (i = 0; i < count; i++)
    {
        wine_ldt_set_base( &entry, base );
        wine_ldt_set_limit( &entry, size - 1 );
        ldt_set_entry( sel + (i << 3), entry );
        base = (const char *)base + 0x10000;
        size -= 0x10000; /* yep, Windows sets limit like that, not 64K sel units */
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
    if ((sel = alloc_entries( count ))) SELECTOR_SetEntries( sel, base, size, flags );
    return sel;
}


/***********************************************************************
 *           SELECTOR_FreeBlock
 *
 * Free a block of selectors.
 */
void SELECTOR_FreeBlock( WORD sel )
{
    WORD idx = sel >> 3, count = get_sel_count( sel );

    TRACE("(%04x,%d)\n", sel, count );

    if (idx < first_ldt_entry) return;  /* error */
    if (!RtlAreBitsSet( &ldt_bitmap, idx, count )) return;  /* error */
    free_entries( sel, count );
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
    if (!ldt_get_entry( sel, &entry )) return sel;
    oldcount = (wine_ldt_get_limit(&entry) >> 16) + 1;
    newcount = (size + 0xffff) >> 16;

    if (oldcount < newcount)  /* we need to add selectors */
    {
        ULONG idx = sel >> 3;

        if (RtlAreBitsClear( &ldt_bitmap, idx + oldcount, newcount - oldcount ))
        {
            RtlSetBits( &ldt_bitmap, idx + oldcount, newcount - oldcount );
        }
        else
        {
            free_entries( sel, oldcount );
            sel = alloc_entries( newcount );
        }
    }
    else if (oldcount > newcount)
    {
        free_entries( sel + (newcount << 3), oldcount - newcount );
    }
    if (sel) SELECTOR_SetEntries( sel, base, size, wine_ldt_get_flags(&entry) );
    return sel;
}


/***********************************************************************
 *           PrestoChangoSelector   (KERNEL.177)
 */
WORD WINAPI PrestoChangoSelector16( WORD selSrc, WORD selDst )
{
    LDT_ENTRY entry;

    if (!ldt_get_entry( selSrc, &entry )) return selDst;
    /* toggle the executable bit */
    entry.HighWord.Bits.Type ^= (WINE_LDT_FLAGS_CODE ^ WINE_LDT_FLAGS_DATA);
    ldt_set_entry( selDst, entry );
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

    if (!ldt_get_entry( sel, &entry )) return 0;
    newsel = AllocSelector16( 0 );
    TRACE("(%04x): returning %04x\n", sel, newsel );
    if (!newsel) return 0;
    entry.HighWord.Bits.Type = WINE_LDT_FLAGS_DATA;
    ldt_set_entry( newsel, entry );
    return newsel;
}


/***********************************************************************
 *           AllocDStoCSAlias   (KERNEL.171)
 */
WORD WINAPI AllocDStoCSAlias16( WORD sel )
{
    WORD newsel;
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return 0;
    newsel = AllocSelector16( 0 );
    TRACE("(%04x): returning %04x\n", sel, newsel );
    if (!newsel) return 0;
    entry.HighWord.Bits.Type = WINE_LDT_FLAGS_CODE;
    ldt_set_entry( newsel, entry );
    return newsel;
}


/***********************************************************************
 *           LongPtrAdd   (KERNEL.180)
 */
void WINAPI LongPtrAdd16( DWORD ptr, DWORD add )
{
    LDT_ENTRY entry;

    if (!ldt_get_entry( SELECTOROF(ptr), &entry )) return;
    wine_ldt_set_base( &entry, (char *)wine_ldt_get_base(&entry) + add );
    ldt_set_entry( SELECTOROF(ptr), entry );
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

    if (!ldt_get_entry( sel, &entry )) return 0;
    wine_ldt_set_base( &entry, DOSMEM_MapDosToLinear(base) );
    ldt_set_entry( sel, entry );
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

    if (!ldt_get_entry( sel, &entry )) return 0;
    wine_ldt_set_limit( &entry, limit );
    ldt_set_entry( sel, entry );
    return sel;
}


/***********************************************************************
 *           SelectorAccessRights   (KERNEL.196)
 */
WORD WINAPI SelectorAccessRights16( WORD sel, WORD op, WORD val )
{
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return 0;
    if (op == 0)  /* get */
    {
        return entry.HighWord.Bytes.Flags1 | ((entry.HighWord.Bytes.Flags2 & 0xf0) << 8);
    }
    else  /* set */
    {
        entry.HighWord.Bytes.Flags1 = LOBYTE(val) | 0xf0;
        entry.HighWord.Bytes.Flags2 = (entry.HighWord.Bytes.Flags2 & 0x0f) | (HIBYTE(val) & 0xf0);
        ldt_set_entry( sel, entry );
        return 0;
    }
}


/***********************************************************************
 *           IsBadCodePtr   (KERNEL.336)
 */
BOOL16 WINAPI IsBadCodePtr16( SEGPTR ptr )
{
    WORD sel = SELECTOROF( ptr );
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return TRUE;
    if (wine_ldt_is_empty( &entry )) return TRUE;
    /* check for code segment, ignoring conforming, read-only and accessed bits */
    if ((entry.HighWord.Bits.Type ^ WINE_LDT_FLAGS_CODE) & 0x18) return TRUE;
    if (OFFSETOF(ptr) > wine_ldt_get_limit(&entry)) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadStringPtr   (KERNEL.337)
 */
BOOL16 WINAPI IsBadStringPtr16( SEGPTR ptr, UINT16 size )
{
    WORD sel = SELECTOROF( ptr );
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return TRUE;
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
    WORD sel = SELECTOROF( ptr );
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return TRUE;
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
    WORD sel = SELECTOROF( ptr );
    LDT_ENTRY entry;

    if (!ldt_get_entry( sel, &entry )) return TRUE;
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

    base = (const char *)ptr - ((ULONG_PTR)ptr & 0x7fff);
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
__ASM_STDCALL_FUNC( UnMapSLFixArray, 8, "ret $8" )

/***********************************************************************
 *		SMapLS (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS, 0,
                   "xor %edx,%edx\n\t"
                   "testl $0xffff0000,%eax\n\t"
                   "jz 1f\n\t"
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("MapLS",4) "\n\t"
                   "movl %eax,%edx\n"
                   "1:\tret" )

/***********************************************************************
 *		SUnMapLS (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS, 0,
                   "pushl %eax\n\t"  /* preserve eax */
                   "pushl %eax\n\t"
                   "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                   "popl %eax\n\t"
                   "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_8 (KERNEL32.@)
 *
 * These functions map linear pointers at [EBP+xxx] to segmented pointers
 * and return them.
 * Win95 uses some kind of alias structs, which it stores in [EBP+x] to
 * unravel them at SUnMapLS. We just store the segmented pointer there.
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_8, 0,
                    "movl 8(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,8(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_12 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_12, 0,
                    "movl 12(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,12(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_16 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_16, 0,
                    "movl 16(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,16(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_20 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_20, 0,
                    "movl 20(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,20(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_24 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_24, 0,
                    "movl 24(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,24(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_28 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_28, 0,
                    "movl 28(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,28(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_32 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_32, 0,
                    "movl 32(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,32(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_36 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_36, 0,
                    "movl 36(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,36(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SMapLS_IP_EBP_40 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SMapLS_IP_EBP_40, 0,
                    "movl 40(%ebp),%eax\n\t"
                    "call " __ASM_STDCALL("SMapLS",0) "\n\t"
                    "movl %edx,40(%ebp)\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_8 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_8, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 8(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,8(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_12 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_12, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 12(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,12(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_16 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_16, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 16(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,16(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_20 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_20, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 20(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,20(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_24 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_24, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 24(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,24(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_28 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_28, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 28(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,28(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_32 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_32, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 32(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,32(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_36 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_36, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 36(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,36(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )

/***********************************************************************
 *		SUnMapLS_IP_EBP_40 (KERNEL32.@)
 */
__ASM_STDCALL_FUNC( SUnMapLS_IP_EBP_40, 0,
                    "pushl %eax\n\t"  /* preserve eax */
                    "pushl 40(%ebp)\n\t"
                    "call " __ASM_STDCALL("UnMapLS",4) "\n\t"
                    "movl $0,40(%ebp)\n\t"
                    "popl %eax\n\t"
                    "ret" )
