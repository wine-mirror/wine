#ifndef __WINE_COLOR_H
#define __WINE_COLOR_H

#include "gdi.h"

extern HPALETTE16 COLOR_Init(void);
extern int COLOR_ToPhysical( DC *dc, COLORREF color );
extern void COLOR_SetMapping( DC *dc, HANDLE map, HANDLE revMap, WORD size );
extern BOOL COLOR_IsSolid( COLORREF color );

extern Colormap COLOR_WinColormap;
extern int COLOR_mapEGAPixel[16];
extern int* COLOR_PaletteToPixel;
extern int* COLOR_PixelToPalette;
extern int COLOR_ColormapSize;

#endif /* __WINE_COLOR_H */
