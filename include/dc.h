/*
 * GDI Device Context function prototypes
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#ifndef __WINE_DC_H
#define __WINE_DC_H

#include "gdi.h"

#define CLIP_INTERSECT	0x0001
#define CLIP_EXCLUDE	0x0002
#define CLIP_KEEPRGN	0x0004

extern DC * DC_AllocDC( const DC_FUNCTIONS *funcs );
extern DC * DC_GetDCPtr( HDC hdc );
extern void DC_InitDC( DC * dc );
extern void DC_UpdateXforms( DC * dc );


/* objects/clipping.c */
INT CLIPPING_IntersectClipRect( DC * dc, INT left, INT top,
    INT right, INT bottom, UINT flags );
INT CLIPPING_IntersectVisRect( DC * dc, INT left, INT top,
    INT right, INT bottom, BOOL exclude );
extern void CLIPPING_UpdateGCRegion( DC * dc );

#endif /* __WINE_DC_H */
