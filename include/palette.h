/*
 * GDI palette definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include "gdi.h"

  /* GDI logical palette object */
typedef struct
{
    GDIOBJHDR   header;
    int        *mapping;
    LOGPALETTE  logpalette; /* _MUST_ be the last field */
} PALETTEOBJ;

extern int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer );
extern BOOL32 PALETTE_DeleteObject( HPALETTE16 hpalette, PALETTEOBJ *palette );
extern BOOL32 PALETTE_UnrealizeObject( HPALETTE16 hpalette, PALETTEOBJ *palette);
     
#endif /* __WINE_PALETTE_H */
