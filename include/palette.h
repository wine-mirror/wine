/*
 * GDI palette definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_PALETTE_H
#define __WINE_PALETTE_H

#include "gdi.h"

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

  /* GDI logical palette object */
typedef struct
{
    GDIOBJHDR   header;
    int        *mapping;
    LOGPALETTE  logpalette; /* _MUST_ be the last field */
} PALETTEOBJ;

extern HPALETTE16 PALETTE_Init(void);
extern int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer );
extern BOOL PALETTE_DeleteObject( HPALETTE16 hpalette, PALETTEOBJ *palette );
extern BOOL PALETTE_UnrealizeObject( HPALETTE16 hpalette, PALETTEOBJ *palette);
     
extern HPALETTE16 WINAPI CreateHalftonePalette16(HDC16 hdc);
extern HPALETTE WINAPI CreateHalftonePalette(HDC hdc);

#endif /* __WINE_PALETTE_H */
