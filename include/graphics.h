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
extern BOOL32 GRAPH_DrawBitmap( HDC32 hdc, HBITMAP32 hbitmap,
                                int xdest, int ydest, int xsrc, int ysrc,
                                int width, int height );

#endif /* __WINE_GRAPHICS_H */
