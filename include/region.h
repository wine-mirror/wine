/*
 * GDI region definitions
 *
 * Copyright 1998 Huw Davies
 */

#ifndef __WINE_REGION_H
#define __WINE_REGION_H

#include "wingdi.h"
#include "gdi.h"

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


