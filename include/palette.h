/*
 * GDI palette definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include "gdi.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* GDI logical palette object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPALETTE  logpalette WINE_PACKED;
} PALETTEOBJ;

#ifndef WINELIB
#pragma pack(4)
#endif

extern int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer );
     
#endif /* __WINE_FONT_H */
