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
    if (IS_SELECTOR_FREE(sel)) return sel;  /* error */
    SELECTOR_FreeBlock( sel, 1 );
    return 0;
}


/***********************************************************************
 *           SELECTOR_SetEntries
 *
 * Set the LDT entries for an array of selectors.
 */
static void SELECTOR_SetEntries( WORD sel, const void *base, DWORD size,
                                 enum seg_type type, BOOL32 is32bit,
                                 BOOL32 readonly )
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
    /* Make sure base and limit are not 0 together if the size is not 0 */
    if (!base && !entry.limit && size) entry.limit = 1;
    count = (size + 0xffff) / 0x10000;
    for (i = 0; i < count; i++)
    {
        LDT_SetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
        entry.base += 0x10000;
        /* Apparently the next selectors should *not* get a 64k limit. */
        /* Can't remember where I read they should... --AJ */
        entry.limit -= entry.limit_in_pages ? 0x10 : 0x10000;
    }
}


/***********************************************************************
 *           SELECTOR_AllocBlock
 *
 * Allocate selectors for a block of linear memory.
 */
WORD SELECTOR_AllocBlock( const void *base, DWORD size, enum seg_type type,
                          BOOL32 is32bit, BOOL32 readonly )
{
    WORD sel, count;

    if (!size) return 0;
    count = (size + 0xffff) / 0x10000;
    sel = AllocSelectorArray( count );
    if (sel) SELECTOR_SetEntries( sel, base, size, type, is32bit, readonly );
    return sel;
}


/***********************************************************************
 *           SELECTOR_FreeBlock
 *
 * Free a block of selectors.
 */
void SELECTOR_FreeBlock( WORD sel, WORD count )
{
    WORD i, nextsel;
    ldt_entry entry;
    STACK16FRAME *frame;

    dprintf_selector( stddeb, "SELECTOR_FreeBlock(%04x,%d)\n", sel, count );
    sel &= ~(__AHINCR - 1);  /* clear bottom bits of selector */
    nextsel = sel + (count << __AHSHIFT);
    memset( &entry, 0, sizeof(entry) );  /* clear the LDT entries */
    for (i = SELECTOR_TO_ENTRY(sel); count; i++, count--)
    {
        LDT_SetEntry( i, &entry );
        ldt_flags_copy[i] &= ~LDT_FLAGS_ALLOCATED;
    }

    /* Clear the saved 16-bit selector */
    frame = CURRENT_STACK16;
    while (frame)
    {
        if ((frame->ds >= sel) && (frame->ds < nextsel)) frame->ds = 0;
        if ((frame->es >= sel) && (frame->es < nextsel)) frame->es = 0;
	frame = PTR_SEG_OFF_TO_LIN(frame->saved_ss, frame->saved_sp);
    }
}


/***********************************************************************
 *           SELECTOR_ReallocBlock
 *
 * Change the size of a block of selectors.
 */
WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size,
                           enum seg_type type, BOOL32 is32bit, BOOL32 readonly)
{
    WORD i, oldcount, newcount;

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
            SELECTOR_FreeBlock( sel, oldcount );
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
        SELECTOR_FreeBlock( ENTRY_TO_SELECTOR(SELECTOR_TO_ENTRY(sel)+newcount),
                            oldcount - newcount );
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
    extern char* DOSMEM_dosmem;
    DWORD base;

    base = GET_SEL_BASE(sel);

#ifndef WINELIB
    /* if base points into DOSMEM, assume we have to
     * return pointer into physical lower 1MB
     */
    if ((base >=  (DWORD)DOSMEM_dosmem)  &&
        (base <  ((DWORD)DOSMEM_dosmem+0x100000))) 
    	base = base - (DWORD)DOSMEM_dosmem;
#endif
    return base;
}


/***********************************************************************
 *           SetSelectorBase   (KERNEL.187)
 */
WORD SetSelectorBase( WORD sel, DWORD base )
{
    extern char* DOSMEM_dosmem;
    ldt_entry entry;

    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
#ifndef WINELIB
    if (base < 0x100000)
    {
    	/* Assume pointers in the lower 1MB range are
	 * in fact physical addresses into DOS memory.
	 * Translate the base to our internal representation
	 *
	 * (NETAPI.DLL of Win95 does use SetSelectorBase this way)
	 */
	entry.base = (DWORD)(DOSMEM_dosmem+base);
    }
    else entry.base = base;
#endif
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
    entry.limit_in_pages = (limit >= 0x100000);
    if (entry.limit_in_pages) entry.limit = limit >> 12;
    else entry.limit = limit;
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
        return 0x01 | /* accessed */
               0x10 | /* not system */
               0x60 | /* DPL 3 */
               0x80 | /* present */
               ((entry.read_only == 0) << 1) |
               (entry.type << 2) |
               (entry.seg_32bit << 14) |
               (entry.limit_in_pages << 15);
    }
    else  /* set */
    {
        entry.read_only = ((val & 2) == 0);
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
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
