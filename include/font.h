/*
 * GDI font definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_FONT_H
#define __WINE_FONT_H

#include "gdi.h"

#ifndef WINELIB
#pragma pack(1)
#endif

  /* GDI logical font object */
typedef struct
{
    GDIOBJHDR   header;
    LOGFONT     logfont WINE_PACKED;
} FONTOBJ;

#ifndef WINELIB
#pragma pack(4)
#endif

extern BOOL FONT_Init( void );
extern int FONT_GetObject( FONTOBJ * font, int count, LPSTR buffer );
extern HFONT FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font );

#endif /* __WINE_FONT_H */
