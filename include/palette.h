/*
 * GDI palette definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include "gdi.h"

#pragma pack(1)

  /* GDI logical palette object */
typedef struct
{
    GDIOBJHDR   header;
    LOGPALETTE  logpalette WINE_PACKED;
} PALETTEOBJ;

#pragma pack(4)

extern int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer );
     
#endif /* __WINE_PALETTE_H */
