/*
 * Selector manipulation functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <string.h>
#include "winerror.h"
#include "wine/winbase16.h"
#include "ldt.h"
#include "miscemu.h"
#include "selectors.h"
#include "stackframe.h"
#include "process.h"
#include "server.h"
#include "debugtools.h"
#include "toolhelp.h"

DEFAULT_DEBUG_CHANNEL(selector);


/***********************************************************************
 *           AllocSelectorArray   (KERNEL.206)
 */
WORD WINAPI AllocSelectorArray16( WORD count )
{
    WORD i, sel, size = 0;
    ldt_entry entry;

    if (!count) return 0;
    for (i = FIRST_LDT_ENTRY_TO_ALLOC; i < LDT_SIZE; i++)
    {
        if (!IS_LDT_ENTRY_FREE(i)) size = 0;
        else if (++size >= count) break;
    }
    if (i == LDT_SIZE) return 0;
    sel = i - size + 1;

    entry.base           = 0;
    entry.type           = SEGMENT_DATA;
    entry.seg_32bit      = FALSE;
    entry.read_only      = FALSE;
    entry.limit_in_pages = FALSE;
    entry.limit          = 1;  /* avoid 0 base and limit */

    for (i = 0; i < count; i++)
    {
        /* Mark selector as allocated */
        ldt_flags_copy[sel + i] |= LDT_FLAGS_ALLOCATED;
        LDT_SetEntry( sel + i, &entry );
    }
    return ENTRY_TO_SELECTOR( sel );
}


/***********************************************************************
 *           AllocSelector   (KERNEL.175)
 */
WORD WINAPI AllocSelector16( WORD sel )
{
    WORD newsel, count, i;

    count = sel ? ((GET_SEL_LIMIT(sel) >> 16) + 1) : 1;
    newsel = AllocSelectorArray16( count );
    TRACE("(%04x): returning %04x\n",
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
WORD WINAPI FreeSelector16( WORD sel )
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
                          BOOL is32bit, BOOL readonly )
{
    WORD sel, count;

    if (!size) return 0;
    count = (size + 0xffff) / 0x10000;
    sel = AllocSelectorArray16( count );
    if (sel) SELECTOR_SetEntries( sel, base, size, type, is32bit, readonly );
    return sel;
}


/***********************************************************************
 *           SELECTOR_MoveBlock
 *
 * Move a block of selectors in linear memory.
 */
void SELECTOR_MoveBlock( WORD sel, const void *new_base )
{
    WORD i, count = (GET_SEL_LIMIT(sel) >> 16) + 1;

    for (i = 0; i < count; i++)
    {
        ldt_entry entry;
        LDT_GetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
        entry.base = (unsigned long)new_base;
        LDT_SetEntry( SELECTOR_TO_ENTRY(sel) + i, &entry );
    }
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

    TRACE("(%04x,%d)\n", sel, count );
    sel &= ~(__AHINCR - 1);  /* clear bottom bits of selector */
    nextsel = sel + (count << __AHSHIFT);

#ifdef __i386__
    {
        /* Check if we are freeing current %fs or %gs selector */
        if ((__get_fs() >= sel) && (__get_fs() < nextsel))
        {
            WARN("Freeing %%fs selector (%04x), not good.\n", __get_fs() );
            __set_fs( 0 );
        }
        if ((__get_gs() >= sel) && (__get_gs() < nextsel)) __set_gs( 0 );
    }
#endif  /* __i386__ */

    memset( &entry, 0, sizeof(entry) );  /* clear the LDT entries */
    for (i = SELECTOR_TO_ENTRY(sel); count; i++, count--)
    {
        LDT_SetEntry( i, &entry );
        ldt_flags_copy[i] &= ~LDT_FLAGS_ALLOCATED;
    }
}


/***********************************************************************
 *           SELECTOR_ReallocBlock
 *
 * Change the size of a block of selectors.
 */
WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size )
{
    ldt_entry entry;
    WORD i, oldcount, newcount;

    if (!size) size = 1;
    oldcount = (GET_SEL_LIMIT(sel) >> 16) + 1;
    newcount = (size + 0xffff) >> 16;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );

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
            sel = AllocSelectorArray16( newcount );
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
    if (sel) SELECTOR_SetEntries( sel, base, size, entry.type,
                                  entry.seg_32bit, entry.read_only );
    return sel;
}


/***********************************************************************
 *           PrestoChangoSelector   (KERNEL.177)
 */
WORD WINAPI PrestoChangoSelector16( WORD selSrc, WORD selDst )
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
WORD WINAPI AllocCStoDSAlias16( WORD sel )
{
    WORD newsel;
    ldt_entry entry;

    newsel = AllocSelectorArray16( 1 );
    TRACE("(%04x): returning %04x\n",
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
WORD WINAPI AllocDStoCSAlias16( WORD sel )
{
    WORD newsel;
    ldt_entry entry;

    newsel = AllocSelectorArray16( 1 );
    TRACE("(%04x): returning %04x\n",
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
void WINAPI LongPtrAdd16( DWORD ptr, DWORD add )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
    entry.base += add;
    LDT_SetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
}


/***********************************************************************
 *           GetSelectorBase   (KERNEL.186)
 */
DWORD WINAPI WIN16_GetSelectorBase( WORD sel )
{
    /*
     * Note: For Win32s processes, the whole linear address space is
     *       shifted by 0x10000 relative to the OS linear address space.
     *       See the comment in msdos/vxd.c.
     */

    DWORD base = GetSelectorBase( sel );
    return W32S_WINE2APP( base, W32S_APPLICATION() ? W32S_OFFSET : 0 );
}
DWORD WINAPI GetSelectorBase( WORD sel )
{
    DWORD base = GET_SEL_BASE(sel);

    /* if base points into DOSMEM, assume we have to
     * return pointer into physical lower 1MB */

    return DOSMEM_MapLinearToDos( (LPVOID)base );
}


/***********************************************************************
 *           SetSelectorBase   (KERNEL.187)
 */
DWORD WINAPI WIN16_SetSelectorBase( WORD sel, DWORD base )
{
    /*
     * Note: For Win32s processes, the whole linear address space is
     *       shifted by 0x10000 relative to the OS linear address space.
     *       See the comment in msdos/vxd.c.
     */

    SetSelectorBase( sel,
	W32S_APP2WINE( base, W32S_APPLICATION() ? W32S_OFFSET : 0 ) );
    return sel;
}
WORD WINAPI SetSelectorBase( WORD sel, DWORD base )
{
    ldt_entry entry;

    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );

    entry.base = (DWORD)DOSMEM_MapDosToLinear(base);

    LDT_SetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    return sel;
}


/***********************************************************************
 *           GetSelectorLimit   (KERNEL.188)
 */
DWORD WINAPI GetSelectorLimit16( WORD sel )
{
    return GET_SEL_LIMIT(sel);
}


/***********************************************************************
 *           SetSelectorLimit   (KERNEL.189)
 */
WORD WINAPI SetSelectorLimit16( WORD sel, DWORD limit )
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
WORD WINAPI SelectorAccessRights16( WORD sel, WORD op, WORD val )
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
 *           IsBadCodePtr16   (KERNEL.336)
 */
BOOL16 WINAPI IsBadCodePtr16( SEGPTR lpfn )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(lpfn);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if (entry.type != SEGMENT_CODE) return TRUE;
    if (OFFSETOF(lpfn) > GET_SEL_LIMIT(sel)) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadStringPtr16   (KERNEL.337)
 */
BOOL16 WINAPI IsBadStringPtr16( SEGPTR ptr, UINT16 size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) && entry.read_only) return TRUE;
    if (strlen(PTR_SEG_TO_LIN(ptr)) < size) size = strlen(PTR_SEG_TO_LIN(ptr)) + 1;
    if (size && (OFFSETOF(ptr) + size - 1 > GET_SEL_LIMIT(sel))) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeReadPtr16   (KERNEL.346)
 */
BOOL16 WINAPI IsBadHugeReadPtr16( SEGPTR ptr, DWORD size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) && entry.read_only) return TRUE;
    if (size && (OFFSETOF(ptr) + size - 1 > GET_SEL_LIMIT(sel))) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           IsBadHugeWritePtr16   (KERNEL.347)
 */
BOOL16 WINAPI IsBadHugeWritePtr16( SEGPTR ptr, DWORD size )
{
    WORD sel;
    ldt_entry entry;

    sel = SELECTOROF(ptr);
    if (!sel) return TRUE;
    if (IS_SELECTOR_FREE(sel)) return TRUE;
    LDT_GetEntry( SELECTOR_TO_ENTRY(sel), &entry );
    if ((entry.type == SEGMENT_CODE) || entry.read_only) return TRUE;
    if (size && (OFFSETOF(ptr) + size - 1 > GET_SEL_LIMIT(sel))) return TRUE;
    return FALSE;
}

/***********************************************************************
 *           IsBadReadPtr16   (KERNEL.334)
 */
BOOL16 WINAPI IsBadReadPtr16( SEGPTR ptr, UINT16 size )
{
    return IsBadHugeReadPtr16( ptr, size );
}


/***********************************************************************
 *           IsBadWritePtr16   (KERNEL.335)
 */
BOOL16 WINAPI IsBadWritePtr16( SEGPTR ptr, UINT16 size )
{
    return IsBadHugeWritePtr16( ptr, size );
}


/***********************************************************************
 *           IsBadFlatReadWritePtr16   (KERNEL.627)
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
DWORD WINAPI MemoryWrite16( WORD sel, DWORD offset, void *buffer, DWORD count )
{
    if (IS_SELECTOR_FREE(sel)) return 0;
    if (offset > GET_SEL_LIMIT(sel)) return 0;
    if (offset + count > GET_SEL_LIMIT(sel) + 1)
        count = GET_SEL_LIMIT(sel) + 1 - offset;
    memcpy( ((char *)GET_SEL_BASE(sel)) + offset, buffer, count );
    return count;
}

/************************************* Win95 pointer mapping functions *
 *
 */

/***********************************************************************
 *           MapSL   (KERNEL32.523)
 *
 * Maps fixed segmented pointer to linear.
 */
LPVOID WINAPI MapSL( SEGPTR sptr )
{
    return (LPVOID)PTR_SEG_TO_LIN(sptr);
}

/***********************************************************************
 *           MapSLFix   (KERNEL32.524)
 *
 * FIXME: MapSLFix and UnMapSLFixArray should probably prevent
 * unexpected linear address change when GlobalCompact() shuffles
 * moveable blocks.
 */

LPVOID WINAPI MapSLFix( SEGPTR sptr )
{
    return (LPVOID)PTR_SEG_TO_LIN(sptr);
}

/***********************************************************************
 *           UnMapSLFixArray   (KERNEL32.701)
 */

void WINAPI UnMapSLFixArray( SEGPTR sptr[], INT length, CONTEXT86 *context )
{
    /* Must not change EAX, hence defined as 'register' function */
}

/***********************************************************************
 *           MapLS   (KERNEL32.522)
 *
 * Maps linear pointer to segmented.
 */
SEGPTR WINAPI MapLS( LPVOID ptr )
{
    if (!HIWORD(ptr))
        return (SEGPTR)ptr;
    else
    {
        WORD sel = SELECTOR_AllocBlock( ptr, 0x10000, SEGMENT_DATA, FALSE, FALSE );
        return PTR_SEG_OFF_TO_SEGPTR( sel, 0 );
    }
}


/***********************************************************************
 *           UnMapLS   (KERNEL32.700)
 *
 * Free mapped selector.
 */
void WINAPI UnMapLS( SEGPTR sptr )
{
    if (SELECTOROF(sptr)) 
        SELECTOR_FreeBlock( SELECTOROF(sptr), 1 );
}

/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE hthread, DWORD sel, LPLDT_ENTRY ldtent)
{
#ifdef __i386__
    struct get_selector_entry_request *req = get_req_buffer();

    if (!(sel & 4))  /* GDT selector */
    {
        sel &= ~3;  /* ignore RPL */
        if (!sel)  /* null selector */
        {
            memset( ldtent, 0, sizeof(*ldtent) );
            return TRUE;
        }
        ldtent->BaseLow                   = 0;
        ldtent->HighWord.Bits.BaseMid     = 0;
        ldtent->HighWord.Bits.BaseHi      = 0;
        ldtent->LimitLow                  = 0xffff;
        ldtent->HighWord.Bits.LimitHi     = 0xf;
        ldtent->HighWord.Bits.Dpl         = 3;
        ldtent->HighWord.Bits.Sys         = 0;
        ldtent->HighWord.Bits.Pres        = 1;
        ldtent->HighWord.Bits.Granularity = 1;
        ldtent->HighWord.Bits.Default_Big = 1;
        ldtent->HighWord.Bits.Type        = 0x12;
        /* it has to be one of the system GDT selectors */
        if (sel == (__get_ds() & ~3)) return TRUE;
        if (sel == (__get_ss() & ~3)) return TRUE;
        if (sel == (__get_cs() & ~3))
        {
            ldtent->HighWord.Bits.Type |= 8;  /* code segment */
            return TRUE;
        }
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    req->handle = hthread;
    req->entry = sel >> __AHSHIFT;
    if (server_call( REQ_GET_SELECTOR_ENTRY )) return FALSE;

    if (!(req->flags & LDT_FLAGS_ALLOCATED))
    {
        SetLastError( ERROR_MR_MID_NOT_FOUND );  /* sic */
        return FALSE;
    }
    if (req->flags & LDT_FLAGS_BIG) req->limit >>= 12;
    ldtent->BaseLow                   = req->base & 0x0000ffff;
    ldtent->HighWord.Bits.BaseMid     = (req->base & 0x00ff0000) >> 16;
    ldtent->HighWord.Bits.BaseHi      = (req->base & 0xff000000) >> 24;
    ldtent->LimitLow                  = req->limit & 0x0000ffff;
    ldtent->HighWord.Bits.LimitHi     = (req->limit & 0x000f0000) >> 16;
    ldtent->HighWord.Bits.Dpl         = 3;
    ldtent->HighWord.Bits.Sys         = 0;
    ldtent->HighWord.Bits.Pres        = 1;
    ldtent->HighWord.Bits.Granularity = (req->flags & LDT_FLAGS_BIG) !=0;
    ldtent->HighWord.Bits.Default_Big = (req->flags & LDT_FLAGS_32BIT) != 0;
    ldtent->HighWord.Bits.Type        = ((req->flags & LDT_FLAGS_TYPE) << 2) | 0x10;
    if (!(req->flags & LDT_FLAGS_READONLY)) ldtent->HighWord.Bits.Type |= 0x2;
    return TRUE;
#else
    SetLastError( ERROR_NOT_IMPLEMENTED );
    return FALSE;
#endif
}


/**********************************************************************
 * 		SMapLS*		(KERNEL32)
 * These functions map linear pointers at [EBP+xxx] to segmented pointers
 * and return them.
 * Win95 uses some kind of alias structs, which it stores in [EBP+x] to
 * unravel them at SUnMapLS. We just store the segmented pointer there.
 */
static void
x_SMapLS_IP_EBP_x(CONTEXT86 *context,int argoff) {
    DWORD	val,ptr; 

    val =*(DWORD*)(EBP_reg(context)+argoff);
    if (val<0x10000) {
	ptr=val;
	*(DWORD*)(EBP_reg(context)+argoff) = 0;
    } else {
    	ptr = MapLS((LPVOID)val);
	*(DWORD*)(EBP_reg(context)+argoff) = ptr;
    }
    EAX_reg(context) = ptr;
}

/***********************************************************************
 *		SMapLS_IP_EBP_8 (KERNEL32.601)
 */
void WINAPI SMapLS_IP_EBP_8 (CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context, 8);}

/***********************************************************************
 *		SMapLS_IP_EBP_12 (KERNEL32.593)
 */
void WINAPI SMapLS_IP_EBP_12(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,12);}

/***********************************************************************
 *		SMapLS_IP_EBP_16 (KERNEL32.594)
 */
void WINAPI SMapLS_IP_EBP_16(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,16);}

/***********************************************************************
 *		SMapLS_IP_EBP_20 (KERNEL32.595)
 */
void WINAPI SMapLS_IP_EBP_20(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,20);}

/***********************************************************************
 *		SMapLS_IP_EBP_24 (KERNEL32.596)
 */
void WINAPI SMapLS_IP_EBP_24(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,24);}

/***********************************************************************
 *		SMapLS_IP_EBP_28 (KERNEL32.597)
 */
void WINAPI SMapLS_IP_EBP_28(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,28);}

/***********************************************************************
 *		SMapLS_IP_EBP_32 (KERNEL32.598)
 */
void WINAPI SMapLS_IP_EBP_32(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,32);}

/***********************************************************************
 *		SMapLS_IP_EBP_36 (KERNEL32.599)
 */
void WINAPI SMapLS_IP_EBP_36(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,36);}

/***********************************************************************
 *		SMapLS_IP_EBP_40 (KERNEL32.600)
 */
void WINAPI SMapLS_IP_EBP_40(CONTEXT86 *context) {x_SMapLS_IP_EBP_x(context,40);}

/***********************************************************************
 *		SMapLS (KERNEL32.592)
 */
void WINAPI SMapLS( CONTEXT86 *context )
{
    if (EAX_reg(context)>=0x10000) {
	EAX_reg(context) = MapLS((LPVOID)EAX_reg(context));
	EDX_reg(context) = EAX_reg(context);
    } else {
	EDX_reg(context) = 0;
    }
}

/***********************************************************************
 *		SUnMapLS (KERNEL32.602)
 */

void WINAPI SUnMapLS( CONTEXT86 *context )
{
    if (EAX_reg(context)>=0x10000)
	UnMapLS((SEGPTR)EAX_reg(context));
}

static void
x_SUnMapLS_IP_EBP_x(CONTEXT86 *context,int argoff) {
	if (*(DWORD*)(EBP_reg(context)+argoff))
		UnMapLS(*(DWORD*)(EBP_reg(context)+argoff));
	*(DWORD*)(EBP_reg(context)+argoff)=0;
}

/***********************************************************************
 *		SUnMapLS_IP_EBP_8 (KERNEL32.611)
 */
void WINAPI SUnMapLS_IP_EBP_8 (CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context, 8); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_12 (KERNEL32.603)
 */
void WINAPI SUnMapLS_IP_EBP_12(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,12); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_16 (KERNEL32.604)
 */
void WINAPI SUnMapLS_IP_EBP_16(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,16); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_20 (KERNEL32.605)
 */
void WINAPI SUnMapLS_IP_EBP_20(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,20); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_24 (KERNEL32.606)
 */
void WINAPI SUnMapLS_IP_EBP_24(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,24); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_28 (KERNEL32.607)
 */
void WINAPI SUnMapLS_IP_EBP_28(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,28); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_32 (KERNEL32.608)
 */
void WINAPI SUnMapLS_IP_EBP_32(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,32); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_36 (KERNEL32.609)
 */
void WINAPI SUnMapLS_IP_EBP_36(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,36); }

/***********************************************************************
 *		SUnMapLS_IP_EBP_40 (KERNEL32.610)
 */
void WINAPI SUnMapLS_IP_EBP_40(CONTEXT86 *context) { x_SUnMapLS_IP_EBP_x(context,40); }

/**********************************************************************
 * 		AllocMappedBuffer	(KERNEL32.38)
 *
 * This is a undocumented KERNEL32 function that 
 * SMapLS's a GlobalAlloc'ed buffer.
 *
 * Input:   EDI register: size of buffer to allocate
 * Output:  EDI register: pointer to buffer
 *
 * Note: The buffer is preceeded by 8 bytes:
 *        ...
 *       edi+0   buffer
 *       edi-4   SEGPTR to buffer
 *       edi-8   some magic Win95 needs for SUnMapLS
 *               (we use it for the memory handle)
 *
 *       The SEGPTR is used by the caller!
 */

void WINAPI AllocMappedBuffer( CONTEXT86 *context )
{
    HGLOBAL handle = GlobalAlloc(0, EDI_reg(context) + 8);
    DWORD *buffer = (DWORD *)GlobalLock(handle);
    SEGPTR ptr = 0;

    if (buffer)
        if (!(ptr = MapLS(buffer + 2)))
        {
            GlobalUnlock(handle);
            GlobalFree(handle);
        }

    if (!ptr)
        EAX_reg(context) = EDI_reg(context) = 0;
    else
    {
        buffer[0] = handle;
        buffer[1] = ptr;

        EAX_reg(context) = (DWORD) ptr;
        EDI_reg(context) = (DWORD)(buffer + 2);
    }
}

/**********************************************************************
 * 		FreeMappedBuffer	(KERNEL32.39)
 *
 * Free a buffer allocated by AllocMappedBuffer
 *
 * Input: EDI register: pointer to buffer
 */

void WINAPI FreeMappedBuffer( CONTEXT86 *context )
{
    if (EDI_reg(context))
    {
        DWORD *buffer = (DWORD *)EDI_reg(context) - 2;

        UnMapLS(buffer[1]);

        GlobalUnlock(buffer[0]);
        GlobalFree(buffer[0]);
    }
}


/***********************************************************************
 *           UTSelectorOffsetToLinear       (WIN32S16.48)
 *
 * rough guesswork, but seems to work (I had no "reasonable" docu)
 */
LPVOID WINAPI UTSelectorOffsetToLinear16(SEGPTR sptr)
{
        return PTR_SEG_TO_LIN(sptr);
}

/***********************************************************************
 *           UTLinearToSelectorOffset       (WIN32S16.49)
 *
 * FIXME: I don't know if that's the right way to do linear -> segmented
 */
SEGPTR WINAPI UTLinearToSelectorOffset16(LPVOID lptr)
{
    return (SEGPTR)lptr;
}

#ifdef __i386__
__ASM_GLOBAL_FUNC( __get_cs, "movw %cs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __get_ds, "movw %ds,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __get_es, "movw %es,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __get_fs, "movw %fs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __get_gs, "movw %gs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __get_ss, "movw %ss,%ax\n\tret" )
__ASM_GLOBAL_FUNC( __set_fs, "movl 4(%esp),%eax\n\tmovw %ax,%fs\n\tret" )
__ASM_GLOBAL_FUNC( __set_gs, "movl 4(%esp),%eax\n\tmovw %ax,%gs\n\tret" )
#endif
