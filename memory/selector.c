/*
 * Selector manipulation functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <string.h>
#include "windows.h"
#include "ldt.h"
#include "selectors.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"


#define FIRST_LDT_ENTRY_TO_ALLOC  6


/***********************************************************************
 *           AllocSelectorArray   (KERNEL.206)
 */
WORD AllocSelectorArray( WORD count )
{
    WORD i, size = 0;

    if (!count) return 0;
    for (i = FIRST_LDT_ENTRY_TO_ALLOC; i < LDT_SIZE; i++)
    {
        if (!IS_LDT_ENTRY_FREE(i)) size = 0;
        else if (++size >= count) break;
    }
    if (i == LDT_SIZE) return 0;
    /* Mark selector as allocated */
    while (size--) ldt_flags_copy[i--] |= LDT_FLAGS_ALLOCATED;
    return ENTRY_TO_SELECTOR( i + 1 );
}


/***********************************************************************
 *           AllocSelector   (KERNEL.175)
 */
WORD AllocSelector( WORD sel )
{
    WORD newsel, count, i;

    count = sel ? ((GET_SEL_LIMIT(sel) >> 16) + 1) : 1;
    newsel = AllocSelectorArray( count );
    dprintf_selector( stddeb, "AllocSelector(%04x): returning %04x\n",
                      sel, newsel );
    if (!newsel) return 0;
    if (!sel) return newsel;  /* nothing to copy */
    for (i = 0; i < count; i++)
    {
        ldt_entry entry;
        LDT_GetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
        LDT_SetEntry( SELECTOR_TO_ENTRY(newsel) + i, &entry );
    }
    return newsel;
}


/***********************************************************************
 *           FreeSelector   (KERNEL.176)
 */
WORD FreeSelector( WORD sel )
{
    WORD i, count;
    ldt_entry entry;

    dprintf_selector( stddeb, "FreeSelector(%04x)\n", sel );
    if (IS_SELECTOR_FREE(sel)) return sel;  /* error */
    count = (GET_SEL_LIMIT(sel) >> 16) + 1;
    memset( &entry, 0, sizeof(entry) );  /* clear the LDT entries */
    /* FIXME: is it correct to free the whole array? */
    for (i = SELECTOR_TO_ENTRY(sel); count; i++, count--)
    {
        LDT_SetEntry( i, &entry );
        ldt_flags_copy[i] &= ~LDT_FLAGS_ALLOCATED;
    }

    /* Clear the saved 16-bit selector */
#ifndef WINELIB
    if (CURRENT_STACK16)
    {
        /* FIXME: maybe we ought to walk up the stack and fix all frames */
        if (CURRENT_STACK16->ds == sel) CURRENT_STACK16->ds = 0;
        if (CURRENT_STACK16->es == sel) CURRENT_STACK16->es = 0;
    }
#endif
    return 0;
}


/***********************************************************************
 *           SELECTOR_SetEntries
 *
 * Set the LDT entries for an array of selectors.
 */
static void SELECTOR_SetEntries( WORD sel, void *base, DWORD size,
                                 enum seg_type type, BOOL is32bit,
                                 BOOL readonly )
{
    ldt_entry entry;
    WORD i, count;

      /* The limit for the first selector is the whole */
      /* block. The next selectors get a 64k limit.    */
    entry.base           = (unsigned long)base;
    entry.type           = type;
    entry.seg_32bit      = is32bit;
    entry.read_only      = readonly;
    entry.limit_in_pages = (size > 0x100000);
    if (entry.limit_in_pages) entry.limit = ((size + 0xfff) >> 12) - 1;
    else entry.limit = size - 1;
    count = (size + 0xffff) / 0x10000;
    for (i = 0; i < count; i++)
    {
        LDT_SetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
        entry.base += 0x10000;
        size -= 0x10000;
        entry.limit = (size > 0x10000) ? 0xffff : size-1;
        entry.limit_in_pages = 0;
    }
}


/***********************************************************************
 *           SELECTOR_AllocBlock
 *
 * Allocate selectors for a block of linear memory.
 */
WORD SELECTOR_AllocBlock( void *base, DWORD size, enum seg_type type,
                          BOOL is32bit, BOOL readonly )
{
    WORD sel, count;

    if (!size) return 0;
    count = (size + 0xffff) / 0x10000;
    sel = AllocSelectorArray( count );
    if (sel) SELECTOR_SetEntries( sel, base, size, type, is32bit, readonly );
    return sel;
}


/***********************************************************************
 *           SELECTOR_ReallocBlock
 *
 * Change the size of a block of selectors.
 */
WORD SELECTOR_ReallocBlock( WORD sel, void *base, DWORD size,
                            enum seg_type type, BOOL is32bit, BOOL readonly )
{
    WORD i, oldcount, newcount;
    ldt_entry entry;

    if (!size) size = 1;
    oldcount = (GET_SEL_LIMIT(sel) >> 16) + 1;
    newcount = (size + 0xffff) >> 16;

    if (oldcount < newcount)  /* We need to add selectors */
    {
          /* Check if the next selectors are free */
        if (SELECTOR_TO_ENTRY(sel) + newcount > LDT_SIZE) i = oldcount;
        else
            for (i = oldcount; i < newcount; i++)
                if (!IS_LDT_ENTRY_FREE(SELECTOR_TO_ENTRY(sel)+i)) break;

        if (i < newcount)  /* they are not free */
        {
            FreeSelector( sel );
            sel = AllocSelectorArray( newcount );
        }
        else  /* mark the selectors as allocated */
        {
            for (i = oldcount; i < newcount; i++)
                ldt_flags_copy[SELECTOR_TO_ENTRY(sel)+i] |=LDT_FLAGS_ALLOCATED;
        }
    }
    else if (oldcount > newcount) /* We need to remove selectors */
    {
        memset( &entry, 0, sizeof(entry) );  /* clear the LDT entries */
        for (i = oldcount; i < newcount; i++)
        {
            LDT_SetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
            ldt_flags_copy[SELECTOR_TO_ENTRY(sel) + i] &= ~LDT_FLAGS_ALLOCATED;
        }
    }
    if (sel) SELECTOR_SetEntries( sel, base, size, type, is32bit, readonly );
    return sel;
}


/***********************************************************************
 *           PrestoChangoSelector   (KERNEL.177)
 */
WORD PrestoChangoSelector( WORD selSrc, WORD selDst )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY( selSrc ), &entry );
    entry.type ^= SEGMENT_CODE;  /* toggle the executable bit */
    LDT_SetEntry( SELECTOR_TO_ENTRY( selDst ), &entry );
    return selDst;
}


/***********************************************************************
 *           AllocCStoDSAlias   (KERNEL.170)
 */
WORD AllocCStoDSAlias( WORD sel )
{
    WORD newsel;
    ldt_entry entry;

    newsel = AllocSelectorArray( 1 );
    dprintf_selector( stddeb, "AllocCStoDSAlias(%04x): returning %04x\n",
                      sel, newsel );
    if (!newsel) return 0;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    entry.type = SEGMENT_DATA;
    LDT_SetEntry( SELECTOR_TO_ENTRY(newsel), &entry );
    return newsel;
}


/***********************************************************************
 *           AllocDStoCSAlias   (KERNEL.171)
 */
WORD AllocDStoCSAlias( WORD sel )
{
    WORD newsel;
    ldt_entry entry;

    newsel = AllocSelectorArray( 1 );
    dprintf_selector( stddeb, "AllocDStoCSAlias(%04x): returning %04x\n",
                      sel, newsel );
    if (!newsel) return 0;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    entry.type = SEGMENT_CODE;
    LDT_SetEntry( SELECTOR_TO_ENTRY(newsel), &entry );
    return newsel;
}


/***********************************************************************
 *           LongPtrAdd   (KERNEL.180)
 */
void LongPtrAdd( DWORD ptr, DWORD add )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
    entry.base += add;
    LDT_SetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
}


/***********************************************************************
 *           GetSelectorBase   (KERNEL.186)
 */
DWORD GetSelectorBase( WORD sel )
{
    return GET_SEL_BASE(sel);
}


/***********************************************************************
 *           SetSelectorBase   (KERNEL.187)
 */
WORD SetSelectorBase( WORD sel, DWORD base )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    entry.base = base;
    LDT_SetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    return sel;
}


/***********************************************************************
 *           GetSelectorLimit   (KERNEL.188)
 */
DWORD GetSelectorLimit( WORD sel )
{
    return GET_SEL_LIMIT(sel);
}


/***********************************************************************
 *           SetSelectorLimit   (KERNEL.189)
 */
WORD SetSelectorLimit( WORD sel, DWORD limit )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    entry.limit = limit;
    LDT_SetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    return sel;
}


/***********************************************************************
 *           SelectorAccessRights   (KERNEL.196)
 */
WORD SelectorAccessRights( WORD sel, WORD op, WORD val )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if (op == 0)  /* get */
    {
        return 1 /* accessed */ |
               (entry.read_only << 1) |
               (entry.type << 2) |
               (entry.seg_32bit << 14) |
               (entry.limit_in_pages << 15);
    }
    else  /* set */
    {
        entry.read_only = val & 2;
        entry.type = (val >> 2) & 3;
        entry.seg_32bit = val & 0x4000;
        entry.limit_in_pages = val & 0x8000;
        LDT_SetEntry( SELECTOR_TO_ENTRY(sel), &entry );
        return 0;
    }
}


/***********************************************************************
 *           IsBadCodePtr   (KERNEL.336)
 */
BOOL IsBadCodePtr( SEGPTR lpfn )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(lpfn);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if (entry.type != SEGMENT_CODE) return TRUE;
    if (OFFSETOF(lpfn) > entry.limit) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadStringPtr   (KERNEL.337)
 */
BOOL IsBadStringPtr( SEGPTR ptr, WORD size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) && entry.read_only) return TRUE;
    if (strlen(PTR_SEG_TO_LIN(ptr)) < size) size = strlen(PTR_SEG_TO_LIN(ptr));
    if (OFFSETOF(ptr) + size > entry.limit) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeReadPtr   (KERNEL.346)
 */
BOOL IsBadHugeReadPtr( SEGPTR ptr, DWORD size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) && entry.read_only) return TRUE;
    if (OFFSETOF(ptr) + size > entry.limit) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeWritePtr   (KERNEL.347)
 */
BOOL IsBadHugeWritePtr( SEGPTR ptr, DWORD size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) || entry.read_only) return TRUE;
    if (OFFSETOF(ptr) + size > entry.limit) return TRUE;
    return FALSE;
}

/***********************************************************************
 *           IsBadReadPtr   (KERNEL.334)
 */
BOOL IsBadReadPtr( SEGPTR ptr, WORD size )
{
    return IsBadHugeReadPtr( ptr, size );
}


/***********************************************************************
 *           IsBadWritePtr   (KERNEL.335)
 */
BOOL IsBadWritePtr( SEGPTR ptr, WORD size )
{
    return IsBadHugeWritePtr( ptr, size );
}


/***********************************************************************
 *           MemoryRead   (TOOLHELP.78)
 */
DWORD MemoryRead( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    if (IS_SELECTOR_FREE(sel)) return 0;
    if (offset > GET_SEL_LIMIT(sel)) return 0;
    if (offset + count > GET_SEL_LIMIT(sel) + 1)
        count = GET_SEL_LIMIT(sel) + 1 - offset;
    memcpy( buffer, ((char *)GET_SEL_BASE(sel)) + offset, count );
    return count;
}


/***********************************************************************
 *           MemoryWrite   (TOOLHELP.79)
 */
DWORD MemoryWrite( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    if (IS_SELECTOR_FREE(sel)) return 0;
    if (offset > GET_SEL_LIMIT(sel)) return 0;
    if (offset + count > GET_SEL_LIMIT(sel) + 1)
        count = GET_SEL_LIMIT(sel) + 1 - offset;
    memcpy( ((char *)GET_SEL_BASE(sel)) + offset, buffer, count );
    return count;
}
