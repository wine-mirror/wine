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

extern void DC_InitDC( DC* dc );
extern BOOL DC_SetupGCForPatBlt( DC * dc, GC gc, BOOL fMapColors );
extern BOOL DC_SetupGCForBrush( DC * dc );
extern BOOL DC_SetupGCForPen( DC * dc );
extern BOOL DC_SetupGCForText( DC * dc );
extern BOOL DC_CallHookProc( DC * dc, WORD code, LPARAM lParam );

extern const int DC_XROPfunction[];

#endif /* __WINE_DC_H */
