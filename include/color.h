#ifndef __WINE_COLOR_H
#define __WINE_COLOR_H

#include "gdi.h"

extern HPALETTE COLOR_Init(void);
extern int COLOR_ToPhysical( DC *dc, COLORREF color );
extern void COLOR_SetMapping( DC *dc, HANDLE, WORD );
extern BOOL COLOR_IsSolid( COLORREF color );

extern Colormap COLOR_WinColormap;
extern int COLOR_mapEGAPixel[16];

#endif /* __WINE_COLOR_H */
