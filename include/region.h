/*
 * GDI region definitions
 *
 * Copyright 1998 Huw Davies
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

#ifndef __WINE_REGION_H
#define __WINE_REGION_H

#include "gdi.h"
#include "windef.h"
#include "wingdi.h"

typedef struct {
    INT size;
    INT numRects;
    INT type; /* NULL, SIMPLE or COMPLEX */
    RECT *rects;
    RECT extents;
} WINEREGION;

  /* GDI logical region object */
typedef struct
{
    GDIOBJHDR   header;
    WINEREGION  *rgn;
} RGNOBJ;

extern BOOL REGION_DeleteObject( HRGN hrgn, RGNOBJ * obj );
extern BOOL REGION_UnionRectWithRgn( HRGN hrgn, const RECT *lpRect );
extern HRGN REGION_CropRgn( HRGN hDst, HRGN hSrc, const RECT *lpRect, const POINT *lpPt );
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y );
extern BOOL REGION_LPTODP( HDC hdc, HRGN hDest, HRGN hSrc );

#endif  /* __WINE_REGION_H */


