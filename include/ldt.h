/*
 * LDT copy
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_LDT_H
#define __WINE_LDT_H

#include "windef.h"
#include "wine/library.h"

#define __AHSHIFT  3  /* don't change! */
#define __AHINCR   (1 << __AHSHIFT)

/* Convert a segmented ptr (16:16) to a linear (32) pointer */

#define PTR_SEG_OFF_TO_LIN(seg,off) \
   ((void*)((char*)wine_ldt_copy.base[LOWORD(seg) >> __AHSHIFT] + (unsigned int)(off)))
#define PTR_REAL_TO_LIN(seg,off) \
   ((void*)(((unsigned int)(seg) << 4) + LOWORD(off)))
#define PTR_SEG_TO_LIN(ptr) \
   PTR_SEG_OFF_TO_LIN(SELECTOROF(ptr),OFFSETOF(ptr))
#define PTR_SEG_OFF_TO_SEGPTR(seg,off) \
   ((SEGPTR)MAKELONG(off,seg))
#define PTR_SEG_OFF_TO_HUGEPTR(seg,off) \
   PTR_SEG_OFF_TO_SEGPTR( (seg) + (HIWORD(off) << __AHSHIFT), LOWORD(off) )

extern UINT W32S_offset;

#define W32S_APP2WINE(addr) ((addr)? (DWORD)(addr) + W32S_offset : 0)
#define W32S_WINE2APP(addr) ((addr)? (DWORD)(addr) - W32S_offset : 0)

#define FIRST_LDT_ENTRY_TO_ALLOC  17

#define IS_SELECTOR_FREE(sel) (!(wine_ldt_copy.flags[LOWORD(sel) >> __AHSHIFT] & WINE_LDT_FLAGS_ALLOCATED))

/* Determine if sel is a system selector (i.e. not managed by Wine) */
#define IS_SELECTOR_SYSTEM(sel) \
   (!((sel) & 4) || ((LOWORD(sel) >> __AHSHIFT) < FIRST_LDT_ENTRY_TO_ALLOC))
#define IS_SELECTOR_32BIT(sel) \
   (IS_SELECTOR_SYSTEM(sel) || (wine_ldt_copy.flags[LOWORD(sel) >> __AHSHIFT] & WINE_LDT_FLAGS_32BIT))

#endif  /* __WINE_LDT_H */
