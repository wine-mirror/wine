/*
 * Internal graphics functions prototypes
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#ifndef __WINE_GRAPHICS_H
#define __WINE_GRAPHICS_H

extern void GRAPH_DrawReliefRect( HDC hdc, RECT *rect, int highlight_size,
                                  int shadow_size, BOOL pressed );
extern BOOL GRAPH_DrawBitmap( HDC hdc, HBITMAP hbitmap, int xdest, int ydest,
                              int xsrc, int ysrc, int width, int height );

#endif /* __WINE_GRAPHICS_H */
