/*
 * Structure definitions for Win32 -- used only internally
 *
 * Copyright 1996 Martin von Loewis
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

#ifndef __WINE_STRUCT32_H
#define __WINE_STRUCT32_H

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winuser16.h"

extern void STRUCT32_MINMAXINFO32to16( const MINMAXINFO*, MINMAXINFO16* );
extern void STRUCT32_MINMAXINFO16to32( const MINMAXINFO16*, MINMAXINFO* );
extern void STRUCT32_WINDOWPOS32to16( const WINDOWPOS*, WINDOWPOS16* );
extern void STRUCT32_WINDOWPOS16to32( const WINDOWPOS16*, WINDOWPOS* );

void STRUCT32_MSG16to32(const MSG16 *msg16,MSG *msg32);
void STRUCT32_MSG32to16(const MSG *msg32,MSG16 *msg16);

void STRUCT32_CREATESTRUCT32Ato16(const CREATESTRUCTA*,CREATESTRUCT16*);
void STRUCT32_CREATESTRUCT16to32A(const CREATESTRUCT16*,CREATESTRUCTA*);
void STRUCT32_MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA*,
                                      MDICREATESTRUCT16*);
void STRUCT32_MDICREATESTRUCT16to32A( const MDICREATESTRUCT16*,
                                      MDICREATESTRUCTA*);
#endif  /* __WINE_STRUCT32_H */
