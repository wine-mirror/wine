/*
 * Internal graphics functions prototypes
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#ifndef __WINE_GRAPHICS_H
#define __WINE_GRAPHICS_H

#include "windows.h"

extern void GRAPH_DrawReliefRect( HDC32 hdc, const RECT32 *rect,
                                  INT32 highlight_size, INT32 shadow_size,
                                  BOOL32 pressed );
extern BOOL32 GRAPH_DrawLines( HDC32 hdc, LPPOINT32 pXY, INT32 N, HPEN32 hPen);
extern void GRAPH_DrawRectangle( HDC32 hdc, INT32 x, INT32 y, 
				 INT32 width, INT32 height, HPEN32 hPen);
extern BOOL32 GRAPH_DrawBitmap( HDC32 hdc, HBITMAP32 hbitmap,
                                INT32 xdest, INT32 ydest, INT32 xsrc,
                                INT32 ysrc, INT32 width, INT32 height, BOOL32 bMono );
extern BOOL32 GRAPH_SelectClipMask( HDC32 hdc, HBITMAP32 hMono,
                                    INT32 x, INT32 y );

#endif /* __WINE_GRAPHICS_H */
