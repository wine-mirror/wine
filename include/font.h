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
    LOGFONT16   logfont WINE_PACKED;
} FONTOBJ;

#pragma pack(4)

#define FONTCACHE 	32	/* dynamic font cache size */

extern BOOL32 FONT_Init( UINT16* pTextCaps );
extern INT16  FONT_GetObject16( FONTOBJ * font, INT16 count, LPSTR buffer );
extern INT32  FONT_GetObject32A( FONTOBJ * font, INT32 count, LPSTR buffer );

#endif /* __WINE_FONT_H */
