/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997, 1998 Martin Boehme
 */

#ifndef __WINE_PATH_H
#define __WINE_PATH_H

#include "windef.h"

/* It should not be necessary to access the contents of the GdiPath
 * structure directly; if you find that the exported functions don't
 * allow you to do what you want, then please place a new exported
 * function that does this job in path.c.
 */

typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)
/* Returns TRUE if the specified path is in the open state, i.e. in the
 * state where points will be added to the path, or FALSE otherwise. This
 * function is implemented as a macro for performance reasons.
 */

extern void   PATH_InitGdiPath(GdiPath *pPath);
extern void   PATH_DestroyGdiPath(GdiPath *pPath);
extern BOOL PATH_AssignGdiPath(GdiPath *pPathDest,
   const GdiPath *pPathSrc);

extern BOOL PATH_MoveTo(HDC hdc);
extern BOOL PATH_LineTo(HDC hdc, INT x, INT y);
extern BOOL PATH_Rectangle(HDC hdc, INT x1, INT y1,
   INT x2, INT y2);
extern BOOL PATH_Ellipse(HDC hdc, INT x1, INT y1,
   INT x2, INT y2);
extern BOOL PATH_Arc(HDC hdc, INT x1, INT y1, INT x2, INT y2,
   INT xStart, INT yStart, INT xEnd, INT yEnd);
extern BOOL PATH_PolyBezierTo(HDC hdc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyBezier(HDC hdc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolylineTo(HDC hdc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polyline(HDC hdc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polygon(HDC hdc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyPolyline(HDC hdc, const POINT *pt, const DWORD *counts,
			      DWORD polylines);
extern BOOL PATH_PolyPolygon(HDC hdc, const POINT *pt, const INT *counts,
			     UINT polygons);
#endif /* __WINE_PATH_H */


