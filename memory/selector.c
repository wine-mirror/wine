/*
 * Selector manipulation functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#include <string.h>
#include "windows.h"
#include "ldt.h"
#include "miscemu.h"
#include "selectors.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"


#define FIRST_LDT_ENTRY_TO_ALLOC  17


/***********************************************************************
 *           AllocSelectorArray   (KERNEL.206)
 */
WORD WINAPI AllocSelectorArray( WORD count )
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
WORD WINAPI AllocSelector( WORD sel )
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
WORD WINAPI FreeSelector( WORD sel )
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

#ifdef __i386__
    {
        /* Check if we are freeing current %fs or %gs selector */

        WORD fs, gs;

        __asm__("movw %%fs,%w0":"=r" (fs));
        if ((fs >= sel) && (fs < nextsel))
        {
            fprintf( stderr, "SELECTOR_FreeBlock: freeing %%fs selector (%04x), not good.\n", fs );
            __asm__("movw %w0,%%fs"::"r" (0));
        }
        __asm__("movw %%gs,%w0":"=r" (gs));
        if ((gs >= sel) && (gs < nextsel))
            __asm__("movw %w0,%%gs"::"r" (0));
    }
#endif  /* __i386__ */

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
	frame = PTR_SEG_TO_LIN( frame->saved_ss_sp );
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
WORD WINAPI PrestoChangoSelector( WORD selSrc, WORD selDst )
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
WORD WINAPI AllocCStoDSAlias( WORD sel )
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
WORD WINAPI AllocDStoCSAlias( WORD sel )
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
void WINAPI LongPtrAdd( DWORD ptr, DWORD add )
{
    ldt_entry entry;
    LDT_GetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
    entry.base += add;
    LDT_SetEntry( SELECTOR_TO_ENTRY(SELECTOROF(ptr)), &entry );
}


/***********************************************************************
 *           GetSelectorBase   (KERNEL.186)
 */
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
DWORD WINAPI GetSelectorLimit( WORD sel )
{
    return GET_SEL_LIMIT(sel);
}


/***********************************************************************
 *           SetSelectorLimit   (KERNEL.189)
 */
WORD WINAPI SetSelectorLimit( WORD sel, DWORD limit )
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
WORD WINAPI SelectorAccessRights( WORD sel, WORD op, WORD val )
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
    if (OFFSETOF(lpfn) > entry.limit) return TRUE;
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
    if (strlen(PTR_SEG_TO_LIN(ptr)) < size) size = strlen(PTR_SEG_TO_LIN(ptr));
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
    if (OFFSETOF(ptr) + size - 1 > entry.limit) return TRUE;
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
 *           MemoryRead   (TOOLHELP.78)
 */
DWORD WINAPI MemoryRead( WORD sel, DWORD offset, void *buffer, DWORD count )
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
DWORD WINAPI MemoryWrite( WORD sel, DWORD offset, void *buffer, DWORD count )
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
 * NOTE: MapSLFix and UnMapSLFixArray are probably needed to prevent
 * unexpected linear address change when GlobalCompact() shuffles
 * moveable blocks.
 */

/***********************************************************************
 *           MapSL   (KERNEL32.662)
 *
 * Maps fixed segmented pointer to linear.
 */
LPVOID WINAPI MapSL( SEGPTR sptr )
{
    return (LPVOID)PTR_SEG_TO_LIN(sptr);
}


/***********************************************************************
 *           MapLS   (KERNEL32.679)
 *
 * Maps linear pointer to segmented.
 */
SEGPTR WINAPI MapLS( LPVOID ptr )
{
    WORD sel = SELECTOR_AllocBlock( ptr, 0x10000, SEGMENT_DATA, FALSE, FALSE );
    return PTR_SEG_OFF_TO_SEGPTR( sel, 0 );
}


/***********************************************************************
 *           UnMapLS   (KERNEL32.680)
 *
 * Free mapped selector.
 */
void WINAPI UnMapLS( SEGPTR sptr )
{
    if (!__winelib) SELECTOR_FreeBlock( SELECTOROF(sptr), 1 );
}

/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32)
 * FIXME: add #ifdef i386 for non x86
 */
BOOL32 WINAPI GetThreadSelectorEntry( HANDLE32 hthread, DWORD sel,
                                      LPLDT_ENTRY ldtent)
{
    ldt_entry	ldtentry;

    LDT_GetEntry(SELECTOR_TO_ENTRY(sel),&ldtentry);
    ldtent->BaseLow = ldtentry.base & 0x0000ffff;
    ldtent->HighWord.Bits.BaseMid = (ldtentry.base & 0x00ff0000) >> 16;
    ldtent->HighWord.Bits.BaseHi = (ldtentry.base & 0xff000000) >> 24;
    ldtent->LimitLow = ldtentry.limit & 0x0000ffff;
    ldtent->HighWord.Bits.LimitHi = (ldtentry.limit & 0x00ff0000) >> 16;
    ldtent->HighWord.Bits.Dpl = 3;
    ldtent->HighWord.Bits.Sys = 0;
    ldtent->HighWord.Bits.Pres = 1;
    ldtent->HighWord.Bits.Type = 0x10|(ldtentry.type << 2);
    if (ldtentry.read_only)
    	ldtent->HighWord.Bits.Type|=0x2;
    ldtent->HighWord.Bits.Granularity = ldtentry.limit_in_pages;
    ldtent->HighWord.Bits.Default_Big = ldtentry.seg_32bit;
    return TRUE;
}


/**********************************************************************
 * 		SMapLS*		(KERNEL32)
 * These functions map linear pointers at [EBP+xxx] to segmented pointers
 * and return them.
 * Win95 uses some kind of alias structs, which it stores in [EBP+x] to
 * unravel them at SUnMapLS. We just store the segmented pointer there.
 */
static void
x_SMapLS_IP_EBP_x(CONTEXT *context,int argoff) {
    DWORD	val,ptr; 

    val =*(DWORD*)(EBP_reg(context)+argoff);
    if (val<0x10000) {
	ptr=val;
	*(DWORD*)(EBP_reg(context)+argoff) = 0;
    } else {
    	ptr = MapLS((LPVOID)val);
	*(DWORD*)(EBP_reg(context)+argoff) = ptr;
    }
    fprintf(stderr,"[EBP+%d] %08lx => %08lx\n",argoff,val,ptr);
    EAX_reg(context) = ptr;
}

void WINAPI SMapLS_IP_EBP_8(CONTEXT *context)  {x_SMapLS_IP_EBP_x(context,8);}
void WINAPI SMapLS_IP_EBP_12(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,12);}
void WINAPI SMapLS_IP_EBP_16(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,16);}
void WINAPI SMapLS_IP_EBP_20(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,20);}
void WINAPI SMapLS_IP_EBP_24(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,24);}
void WINAPI SMapLS_IP_EBP_28(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,28);}
void WINAPI SMapLS_IP_EBP_32(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,32);}
void WINAPI SMapLS_IP_EBP_36(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,36);}
void WINAPI SMapLS_IP_EBP_40(CONTEXT *context) {x_SMapLS_IP_EBP_x(context,40);}

void WINAPI SMapLS(CONTEXT *context)
{
    if (EAX_reg(context)>=0x10000) {
	EAX_reg(context) = MapLS((LPVOID)EAX_reg(context));
	EDX_reg(context) = EAX_reg(context);
    } else {
	EDX_reg(context) = 0;
    }
}

void WINAPI SUnMapLS(CONTEXT *context)
{
    if (EAX_reg(context)>=0x10000)
	UnMapLS((SEGPTR)EAX_reg(context));
}

static void
x_SUnMapLS_IP_EBP_x(CONTEXT *context,int argoff) {
	if (*(DWORD*)(EBP_reg(context)+argoff))
		UnMapLS(*(DWORD*)(EBP_reg(context)+argoff));
	*(DWORD*)(EBP_reg(context)+argoff)=0;
}
void WINAPI SUnMapLS_IP_EBP_8(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,12); }
void WINAPI SUnMapLS_IP_EBP_12(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,12); }
void WINAPI SUnMapLS_IP_EBP_16(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,16); }
void WINAPI SUnMapLS_IP_EBP_20(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,20); }
void WINAPI SUnMapLS_IP_EBP_24(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,24); }
void WINAPI SUnMapLS_IP_EBP_28(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,28); }
void WINAPI SUnMapLS_IP_EBP_32(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,32); }
void WINAPI SUnMapLS_IP_EBP_36(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,36); }
void WINAPI SUnMapLS_IP_EBP_40(CONTEXT *context) { x_SUnMapLS_IP_EBP_x(context,40); }

/**********************************************************************
 *           WOWGetVDMPointer	(KERNEL32.55)
 * Get linear from segmented pointer. (MSDN lib)
 */
LPVOID WINAPI WOWGetVDMPointer(DWORD vp,DWORD nrofbytes,BOOL32 protected)
{
    /* FIXME: add size check too */
    fprintf(stdnimp,"WOWGetVDMPointer(%08lx,%ld,%d)\n",vp,nrofbytes,protected);
    if (protected)
	return PTR_SEG_TO_LIN(vp);
    else
	return DOSMEM_MapRealToLinear(vp);
}

/**********************************************************************
 *           GetVDMPointer32W   (KERNEL.516)
 */
LPVOID WINAPI GetVDMPointer32W(DWORD vp,DWORD mode)
{
    return WOWGetVDMPointer(vp,0,mode);
}

/**********************************************************************
 *           WOWGetVDMPointerFix (KERNEL32.55)
 * Dito, but fix heapsegment (MSDN lib)
 */
LPVOID WINAPI WOWGetVDMPointerFix(DWORD vp,DWORD nrofbytes,BOOL32 protected)
{
    /* FIXME: fix heapsegment */
    fprintf(stdnimp,"WOWGetVDMPointerFix(%08lx,%ld,%d)\n",vp,nrofbytes,protected);
    return WOWGetVDMPointer(vp,nrofbytes,protected);
}

/**********************************************************************
 *           WOWGetVDMPointerUnFix (KERNEL32.56)
 */
void WINAPI WOWGetVDMPointerUnfix(DWORD vp)
{
    fprintf(stdnimp,"WOWGetVDMPointerUnfix(%08lx), STUB\n",vp);
    /* FIXME: unfix heapsegment */
}

