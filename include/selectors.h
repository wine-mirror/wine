/*
 * Selector definitions
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SELECTORS_H
#define __WINE_SELECTORS_H

#include "windef.h"
#include "wine/library.h"

extern WORD SELECTOR_AllocBlock( const void *base, DWORD size, unsigned char flags );
extern WORD SELECTOR_ReallocBlock( WORD sel, const void *base, DWORD size );
extern void SELECTOR_FreeBlock( WORD sel );

/* Determine if sel is a system selector (i.e. not managed by Wine) */
#define IS_SELECTOR_SYSTEM(sel) \
   (!((sel) & 4) || ((LOWORD(sel) >> 3) < WINE_LDT_FIRST_ENTRY))
#define IS_SELECTOR_32BIT(sel) \
   (IS_SELECTOR_SYSTEM(sel) || (wine_ldt_copy.flags[LOWORD(sel) >> 3] & WINE_LDT_FLAGS_32BIT))

#endif /* __WINE_SELECTORS_H */
