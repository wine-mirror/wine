/*
 * GDI font definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_FONT_H
#define __WINE_FONT_H

#include "gdi.h"

#pragma pack(1)

  /* GDI logical font object */
typedef struct
{
    GDIOBJHDR   header;
    LOGFONT     logfont WINE_PACKED;
} FONTOBJ;

#pragma pack(4)

extern BOOL FONT_Init( void );
extern int FONT_GetObject( FONTOBJ * font, int count, LPSTR buffer );
extern HFONT FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font );

#endif /* __WINE_FONT_H */
