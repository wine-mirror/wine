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
extern DC * DC_GetDCPtr( HDC32 hdc );
extern void DC_InitDC( DC * dc );
extern BOOL32 DC_SetupGCForPatBlt( DC * dc, GC gc, BOOL32 fMapColors );
extern BOOL32 DC_SetupGCForBrush( DC * dc );
extern BOOL32 DC_SetupGCForPen( DC * dc );
extern BOOL32 DC_SetupGCForText( DC * dc );

extern const int DC_XROPfunction[];

#endif /* __WINE_DC_H */
