/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/
#include <stdlib.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include "windows.h"
#include "options.h"
#include "gdi.h"
#include "color.h"


Colormap COLOR_WinColormap = 0;

  /* System palette static colors */

#define NB_RESERVED_COLORS  20

  /* The first and last eight colors are EGA colors */
static PALETTEENTRY COLOR_sysPaletteEntries[NB_RESERVED_COLORS] =
{
    /* red  green blue  flags */
    { 0x00, 0x00, 0x00, 0 },
    { 0x80, 0x00, 0x00, 0 },
    { 0x00, 0x80, 0x00, 0 },
    { 0x80, 0x80, 0x00, 0 },
    { 0x00, 0x00, 0x80, 0 },
    { 0x80, 0x00, 0x80, 0 },
    { 0x00, 0x80, 0x80, 0 },
    { 0xc0, 0xc0, 0xc0, 0 },
    { 0xc0, 0xdc, 0xc0, 0 },
    { 0xa6, 0xca, 0xf0, 0 },

    { 0xff, 0xfb, 0xf0, 0 },
    { 0xa0, 0xa0, 0xa4, 0 },
    { 0x80, 0x80, 0x80, 0 },
    { 0xff, 0x00, 0x00, 0 },
    { 0x00, 0xff, 0x00, 0 },
    { 0xff, 0xff, 0x00, 0 },
    { 0x00, 0x00, 0xff, 0 },
    { 0xff, 0x00, 0xff, 0 },
    { 0x00, 0xff, 0xff, 0 },
    { 0xff, 0xff, 0xff, 0 }
};

static HANDLE hSysColorTranslation = 0;
static HANDLE hRevSysColorTranslation = 0;

   /* Map an EGA index (0..15) to a pixel value. Used for dithering. */
int COLOR_mapEGAPixel[16];

int* COLOR_PaletteToPixel = NULL;
int* COLOR_PixelToPalette = NULL;
int COLOR_ColormapSize = 0;

/***********************************************************************
 *           COLOR_BuildMap
 *
 * Fill the private colormap.
 */
static BOOL COLOR_BuildMap( Colormap map, int depth, int size )
{
    XColor color;
    int r, g, b, red_incr, green_incr, blue_incr;
    int index = 0;

      /* Fill the whole map with a range of colors */

    blue_incr  = 0x10000 >> (depth / 3);
    red_incr   = 0x10000 >> ((depth + 1) / 3);
    green_incr = 0x10000 >> ((depth + 2) / 3);

    for (r = red_incr - 1; r < 0x10000; r += red_incr)
	for (g = green_incr - 1; g < 0x10000; g += green_incr)
	    for (b = blue_incr - 1; b < 0x10000; b += blue_incr)
	    {
		if (index >= size) break;
		color.pixel = index++;
		color.red   = r;
		color.green = g;
		color.blue  = b;
		XStoreColor( display, map, &color );
	    }

    return TRUE;
}


/***********************************************************************
 *           COLOR_InitPalette
 *
 * Create the system palette.
 */
static HPALETTE COLOR_InitPalette(void)
{
    int i, size, pixel;
    XColor color;
    HPALETTE hpalette;
    LOGPALETTE * palPtr;
    WORD *colorTranslation, *revTranslation;

    size = DefaultVisual( display, DefaultScreen(display) )->map_entries;
    COLOR_ColormapSize = size;
    if (!(hSysColorTranslation = GDI_HEAP_ALLOC( GMEM_MOVEABLE,
				            sizeof(WORD)*NB_RESERVED_COLORS )))
        return FALSE;
    if (!(hRevSysColorTranslation = GDI_HEAP_ALLOC( GMEM_MOVEABLE,
                                                    sizeof(WORD)*size )))
        return FALSE;
    colorTranslation = (WORD *) GDI_HEAP_ADDR( hSysColorTranslation );
    revTranslation   = (WORD *) GDI_HEAP_ADDR( hRevSysColorTranslation );

    if (COLOR_WinColormap == DefaultColormapOfScreen(screen))
    {
        COLOR_PaletteToPixel = (int *)malloc( sizeof(int) * size );
        COLOR_PixelToPalette = (int *)malloc( sizeof(int) * size );
    }

    for (i = 0; i < NB_RESERVED_COLORS; i++)
    {
	color.red   = COLOR_sysPaletteEntries[i].peRed * 65535 / 255;
	color.green = COLOR_sysPaletteEntries[i].peGreen * 65535 / 255;
	color.blue  = COLOR_sysPaletteEntries[i].peBlue * 65535 / 255;
	color.flags = DoRed | DoGreen | DoBlue;

        if (i < NB_RESERVED_COLORS/2)
        {
            /* Bottom half of the colormap */
            pixel = i;
            if (pixel >= size/2) continue;
        }
        else
        {
            /* Top half of the colormap */
            pixel = size - NB_RESERVED_COLORS + i;
            if (pixel < size/2) continue;
        }
	if (COLOR_WinColormap != DefaultColormapOfScreen(screen))
	{
            color.pixel = pixel;
	    XStoreColor( display, COLOR_WinColormap, &color );
	}
	else
        {
            if (!XAllocColor( display, COLOR_WinColormap, &color ))
            {
                fprintf(stderr, "Warning: Not enough free colors. Try using the -privatemap option.\n" );
                color.pixel = color.red = color.green = color.blue = 0;
            }
            else
            {
                COLOR_PaletteToPixel[pixel] = color.pixel;
                COLOR_PixelToPalette[color.pixel] = pixel;
            }
        }
	colorTranslation[i] = color.pixel;
        revTranslation[color.pixel] = i;
	  /* Set EGA mapping if color in the first or last eight */
	if (i < 8)
	    COLOR_mapEGAPixel[i] = color.pixel;
	else if (i >= NB_RESERVED_COLORS-8)
	    COLOR_mapEGAPixel[i - (NB_RESERVED_COLORS-16)] = color.pixel;
    }

    palPtr = malloc( sizeof(LOGPALETTE) + (NB_RESERVED_COLORS-1)*sizeof(PALETTEENTRY) );
    if (!palPtr) return FALSE;
    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    memcpy( palPtr->palPalEntry, COLOR_sysPaletteEntries,
	    sizeof(COLOR_sysPaletteEntries) );
    hpalette = CreatePalette( palPtr );
    free( palPtr );
    return hpalette;
}


/***********************************************************************
 *           COLOR_Init
 *
 * Initialize color map and system palette.
 */
HPALETTE COLOR_Init(void)
{
    Visual * visual = DefaultVisual( display, DefaultScreen(display) );
    
    switch(visual->class)
    {
    case GrayScale:
    case PseudoColor:
    case DirectColor:
	if (Options.usePrivateMap)
	{
	    COLOR_WinColormap = XCreateColormap( display, rootWindow,
						 visual, AllocAll );
	    if (COLOR_WinColormap)
	    {
		COLOR_BuildMap( COLOR_WinColormap, screenDepth,
			        visual->map_entries );
		if (rootWindow != DefaultRootWindow(display))
		{
		    XSetWindowAttributes win_attr;
		    win_attr.colormap = COLOR_WinColormap;
		    XChangeWindowAttributes( display, rootWindow,
					     CWColormap, &win_attr );
		}
		break;
	    }
	}
	/* Fall through */
    case StaticGray:
    case StaticColor:
    case TrueColor:
	COLOR_WinColormap = DefaultColormapOfScreen( screen );
	break;	
    }
    return COLOR_InitPalette();
}


/***********************************************************************
 *           COLOR_IsSolid
 *
 * Check whether 'color' can be represented with a solid color.
 */
BOOL COLOR_IsSolid( COLORREF color )
{
    int i;
    PALETTEENTRY *pEntry = COLOR_sysPaletteEntries;

    if (color & 0xff000000) return TRUE;
    if (!color || (color == 0xffffff)) return TRUE;
    for (i = NB_RESERVED_COLORS; i > 0; i--, pEntry++)
    {
	if ((GetRValue(color) == pEntry->peRed) &&
	    (GetGValue(color) == pEntry->peGreen) &&
	    (GetBValue(color) == pEntry->peBlue)) return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           COLOR_ToPhysical
 *
 * Return the physical color closest to 'color'.
 */
int COLOR_ToPhysical( DC *dc, COLORREF color )
{
    WORD index = 0;
    WORD *mapping;

    if (screenDepth > 8) return color;
    switch(color >> 24)
    {
    case 0:  /* RGB */
	index = GetNearestPaletteIndex( STOCK_DEFAULT_PALETTE, color );
	break;
    case 1:  /* PALETTEINDEX */
	index = color & 0xffff;
	break;
    case 2:  /* PALETTERGB */
	if (dc) index = GetNearestPaletteIndex( dc->w.hPalette, color );
        else index = 0;
	break;
    }
    if (dc)
    {
        if (index >= dc->u.x.pal.mappingSize) return 0;
        mapping = (WORD *) GDI_HEAP_ADDR( dc->u.x.pal.hMapping );
    }
    else
    {
        if (index >= NB_RESERVED_COLORS) return 0;
        mapping = (WORD *) GDI_HEAP_ADDR( hSysColorTranslation );
    }
    if (mapping) return mapping[index];
    else return index;  /* Identity mapping */
}


/***********************************************************************
 *           COLOR_SetMapping
 *
 * Set the color-mapping table in a DC.
 */
void COLOR_SetMapping( DC *dc, HANDLE map, HANDLE revMap, WORD size )
{
    WORD *pmap, *pnewmap;
    WORD i;

    if (dc->u.x.pal.hMapping && (dc->u.x.pal.hMapping != hSysColorTranslation))
	GDI_HEAP_FREE( dc->u.x.pal.hMapping );
    if (dc->u.x.pal.hRevMapping &&
        (dc->u.x.pal.hRevMapping != hRevSysColorTranslation))
	GDI_HEAP_FREE( dc->u.x.pal.hRevMapping );
    if (map && (map != hSysColorTranslation))
    {
	  /* Copy mapping table */
	dc->u.x.pal.hMapping = GDI_HEAP_ALLOC(GMEM_MOVEABLE,sizeof(WORD)*size);
	pmap = (WORD *) GDI_HEAP_ADDR( map );
	pnewmap = (WORD *) GDI_HEAP_ADDR( dc->u.x.pal.hMapping );
	memcpy( pnewmap, pmap, sizeof(WORD)*size );
          /* Build reverse table */
        dc->u.x.pal.hRevMapping = GDI_HEAP_ALLOC( GMEM_MOVEABLE,
                                             sizeof(WORD)*COLOR_ColormapSize );
        pmap = (WORD *) GDI_HEAP_ADDR( dc->u.x.pal.hRevMapping );
        for (i = 0; i < size; i++) pmap[pnewmap[i]] = i;
    }
    else
    {
        dc->u.x.pal.hMapping = map;
        dc->u.x.pal.hRevMapping = map ? hRevSysColorTranslation : 0;
    }
    dc->u.x.pal.mappingSize = size;
}


/***********************************************************************
 *           GetNearestColor    (GDI.154)
 */
COLORREF GetNearestColor( HDC hdc, COLORREF color )
{
    WORD index;
    DC *dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (screenDepth > 8) return color;
    index = (WORD)(COLOR_ToPhysical( dc, color & 0xffffff ) & 0xffff);
    return PALETTEINDEX( index );
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
WORD RealizeDefaultPalette( HDC hdc )
{
    DC *dc;
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    dc->w.hPalette = STOCK_DEFAULT_PALETTE;
    COLOR_SetMapping( dc, hSysColorTranslation,
                      hRevSysColorTranslation, NB_RESERVED_COLORS );
    return NB_RESERVED_COLORS;
}
