/*
 * Graphics paths (BeginPath, EndPath etc.)
 *
 * Copyright 1997, 1998 Martin Boehme
 */

#ifndef __WINE_PATH_H
#define __WINE_PATH_H

#include "wintypes.h"

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
   POINT32      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL32       newStroke;
} GdiPath;

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)
/* Returns TRUE if the specified path is in the open state, i.e. in the
 * state where points will be added to the path, or FALSE otherwise. This
 * function is implemented as a macro for performance reasons.
 */

extern void   PATH_InitGdiPath(GdiPath *pPath);
extern void   PATH_DestroyGdiPath(GdiPath *pPath);
extern BOOL32 PATH_AssignGdiPath(GdiPath *pPathDest,
   const GdiPath *pPathSrc);

extern BOOL32 PATH_MoveTo(HDC32 hdc);
extern BOOL32 PATH_LineTo(HDC32 hdc, INT32 x, INT32 y);
extern BOOL32 PATH_Rectangle(HDC32 hdc, INT32 x1, INT32 y1,
   INT32 x2, INT32 y2);
extern BOOL32 PATH_Ellipse(HDC32 hdc, INT32 x1, INT32 y1,
   INT32 x2, INT32 y2);
extern BOOL32 PATH_Arc(HDC32 hdc, INT32 x1, INT32 y1, INT32 x2, INT32 y2,
   INT32 xStart, INT32 yStart, INT32 xEnd, INT32 yEnd);

#endif /* __WINE_PATH_H */
