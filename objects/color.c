/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 1996 Alex Korobka
 */

#include "color.h"
#include "debug.h"
#include "palette.h"
#include "windef.h"

DEFAULT_DEBUG_CHANNEL(palette)


/***********************************************************************
 * System color space. 
 *
 * First 10 and last 10 colors in COLOR_sysPalette are
 * "guarded". RealizePalette changes only the rest of colorcells. For
 * currently inactive window it changes only DC palette mappings.
 */

PALETTEENTRY *COLOR_sysPal = NULL; /* current system palette */

int COLOR_gapStart = 256;
int COLOR_gapEnd = -1;
int COLOR_gapFilled = 0;
int COLOR_max = 256;

const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS] = 
{
    /* first 10 entries in the system palette */
    /* red  green blue  flags */
    { 0x00, 0x00, 0x00, PC_SYS_USED },
    { 0x80, 0x00, 0x00, PC_SYS_USED },
    { 0x00, 0x80, 0x00, PC_SYS_USED },
    { 0x80, 0x80, 0x00, PC_SYS_USED },
    { 0x00, 0x00, 0x80, PC_SYS_USED },
    { 0x80, 0x00, 0x80, PC_SYS_USED },
    { 0x00, 0x80, 0x80, PC_SYS_USED },
    { 0xc0, 0xc0, 0xc0, PC_SYS_USED },
    { 0xc0, 0xdc, 0xc0, PC_SYS_USED },
    { 0xa6, 0xca, 0xf0, PC_SYS_USED },

    /* ... c_min/2 dynamic colorcells */

    /* ... gap (for sparse palettes) */

    /* ... c_min/2 dynamic colorcells */

    { 0xff, 0xfb, 0xf0, PC_SYS_USED },
    { 0xa0, 0xa0, 0xa4, PC_SYS_USED },
    { 0x80, 0x80, 0x80, PC_SYS_USED },
    { 0xff, 0x00, 0x00, PC_SYS_USED },
    { 0x00, 0xff, 0x00, PC_SYS_USED },
    { 0xff, 0xff, 0x00, PC_SYS_USED },
    { 0x00, 0x00, 0xff, PC_SYS_USED },
    { 0xff, 0x00, 0xff, PC_SYS_USED },
    { 0x00, 0xff, 0xff, PC_SYS_USED },
    { 0xff, 0xff, 0xff, PC_SYS_USED }     /* last 10 */
};

/***********************************************************************
 *           COLOR_GetSystemPaletteTemplate
 */
const PALETTEENTRY* COLOR_GetSystemPaletteTemplate(void)
{
    return COLOR_sysPalTemplate;
}

/***********************************************************************
 *           COLOR_GetSystemPaletteEntry
 */

COLORREF COLOR_GetSystemPaletteEntry(UINT i)
{
    return *(COLORREF*)(COLOR_sysPal + i) & 0x00ffffff;
}

/***********************************************************************
 *           COLOR_IsSolid
 *
 * Check whether 'color' can be represented with a solid color.
 */
BOOL COLOR_IsSolid( COLORREF color )
{
    int i;
    const PALETTEENTRY *pEntry = COLOR_sysPal;

    if (color & 0xff000000) return TRUE;		/* indexed color */

    if (!color || (color == 0xffffff)) return TRUE;	/* black or white */

    for (i = 0; i < 256 ; i++, pEntry++)
    {
      if( i < COLOR_gapStart || i > COLOR_gapEnd )
	if ((GetRValue(color) == pEntry->peRed) &&
	    (GetGValue(color) == pEntry->peGreen) &&
	    (GetBValue(color) == pEntry->peBlue)) return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *	     COLOR_PaletteLookupPixel
 */
int COLOR_PaletteLookupPixel( PALETTEENTRY* palPalEntry, int size,
                              int* mapping, COLORREF col, BOOL skipReserved )
{
    int i, best = 0, diff = 0x7fffffff;
    int r,g,b;

    for( i = 0; i < size && diff ; i++ )
    {
        if( !(palPalEntry[i].peFlags & PC_SYS_USED) ||
            (skipReserved && palPalEntry[i].peFlags  & PC_SYS_RESERVED) )
            continue;

        r = palPalEntry[i].peRed - GetRValue(col);
        g = palPalEntry[i].peGreen - GetGValue(col);
        b = palPalEntry[i].peBlue - GetBValue(col);

        r = r*r + g*g + b*b;

        if( r < diff ) { best = i; diff = r; }
    }
    return (mapping) ? mapping[best] : best;
}

/***********************************************************************
 *	     COLOR_PaletteLookupExactIndex
 */
int COLOR_PaletteLookupExactIndex( PALETTEENTRY* palPalEntry, int size,
                                   COLORREF col )
{
    int i;
    BYTE r = GetRValue(col), g = GetGValue(col), b = GetBValue(col);
    for( i = 0; i < size; i++ )
    {
        if( palPalEntry[i].peFlags & PC_SYS_USED ) 	/* skips gap */
            if( palPalEntry[i].peRed == r &&
                palPalEntry[i].peGreen == g &&
                palPalEntry[i].peBlue == b )
                return i;
    }
    return -1;
}

/***********************************************************************
 *           COLOR_LookupNearestColor
 */
COLORREF COLOR_LookupNearestColor( PALETTEENTRY* palPalEntry, int size, COLORREF color )
{
  unsigned char		spec_type = color >> 24;
  int			i;

  /* we need logical palette for PALETTERGB and PALETTEINDEX colorrefs */

  if( spec_type == 2 ) /* PALETTERGB */
    color = *(COLORREF*)
	     (palPalEntry + COLOR_PaletteLookupPixel(palPalEntry,size,NULL,color,FALSE));

  else if( spec_type == 1 ) /* PALETTEINDEX */
  {
    if( (i = color & 0x0000ffff) >= size ) 
      {
	WARN(palette, "RGB(%lx) : idx %d is out of bounds, assuming NULL\n", color, i);
	color = *(COLORREF*)palPalEntry;
      }
    else color = *(COLORREF*)(palPalEntry + i);
  }

  color &= 0x00ffffff;
  return (0x00ffffff & *(COLORREF*)
         (COLOR_sysPal + COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL, color, FALSE)));
}
