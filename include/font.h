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

/* may be switched... */
#define GGO_BITMAP	0x4F4D
#define GGO_NATIVE	0x4F50
typedef struct
{
	UINT	gmBlackBoxX;
	UINT	gmBlackBoxY;
	POINT16	gmptGlyphOrigin;
	int	gmCellIncX;
	int	gmCellIncY;
} GLYPHMETRICS,*LPGLYPHMETRICS;
typedef struct
{
	DWORD	eM11; /* all type FIXED in Borlands Handbook */
	DWORD	eM12; 
	DWORD	eM21;
	DWORD	eM22;
} MAT2,*LPMAT2;

#ifndef WINELIB
#pragma pack(4)
#endif

extern BOOL FONT_Init( void );
extern int FONT_GetObject( FONTOBJ * font, int count, LPSTR buffer );
extern HFONT FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font );

#endif /* __WINE_FONT_H */
