/*
 * GDI palette definitions
 *
 * Copyright 1994 Alexandre Julliard
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

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include "gdi.h"

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

  /* GDI logical palette object */
typedef struct tagPALETTEOBJ
{
    GDIOBJHDR                    header;
    int                          *mapping;
    LOGPALETTE                   logpalette; /* _MUST_ be the last field */
} PALETTEOBJ;

typedef struct tagPALETTE_DRIVER
{
  int  (*pSetMapping)(struct tagPALETTEOBJ *, UINT, UINT, BOOL);
  int  (*pUpdateMapping)(struct tagPALETTEOBJ *);
  BOOL (*pIsDark)(int pixel);
} PALETTE_DRIVER;

extern PALETTE_DRIVER *PALETTE_Driver;

extern HPALETTE16 PALETTE_Init(void);
extern int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer );
extern BOOL PALETTE_DeleteObject( HPALETTE16 hpalette, PALETTEOBJ *palette );
extern BOOL PALETTE_UnrealizeObject( HPALETTE16 hpalette, PALETTEOBJ *palette);
     
extern HPALETTE16 WINAPI CreateHalftonePalette16(HDC16 hdc);
extern HPALETTE WINAPI CreateHalftonePalette(HDC hdc);

#endif /* __WINE_PALETTE_H */
