/*
 * Selector definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "windef.h"
#include "wine/library.h"

extern WORD SELECTOR_AllocBlock( const void *base, DWORD size, unsigned char flags );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_FreeBlock( WORD sel );

extern UINT W32S_offset;

#define W32S_APP2WINE(addr) ((addr)? (DWORD)(addr) + W32S_offset : 0)
#define W32S_WINE2APP(addr) ((addr)? (DWORD)(addr) - W32S_offset : 0)

#define FIRST_LDT_ENTRY_TO_ALLOC  17

#define IS_SELECTOR_FREE(sel) (!(wine_ldt_copy.flags[LOWORD(sel) >> 3] & WINE_LDT_FLAGS_ALLOCATED))

/* Determine if sel is a system selector (i.e. not managed by Wine) */
#define IS_SELECTOR_SYSTEM(sel) \
   (!((sel) & 4) || ((LOWORD(sel) >> 3) < FIRST_LDT_ENTRY_TO_ALLOC))
#define IS_SELECTOR_32BIT(sel) \
   (IS_SELECTOR_SYSTEM(sel) || (wine_ldt_copy.flags[LOWORD(sel) >> 3] & WINE_LDT_FLAGS_32BIT))

#endif /* __WINE_SELECTORS_H */
