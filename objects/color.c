/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include "windows.h"
#include "options.h"
#include "gdi.h"
#include "color.h"
#include "palette.h"
#include "xmalloc.h"

Colormap COLOR_WinColormap = 0;
int COLOR_FixedMap = 0;

int COLOR_Redshift = 0;
int COLOR_Redmax = 0;
int COLOR_Greenshift = 0;
int COLOR_Greenmax = 0;
int COLOR_Blueshift = 0;
int COLOR_Bluemax = 0;
int COLOR_Graymax = 0;

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
    if (screenDepth <= 8)
    {
        if (!(hSysColorTranslation = GDI_HEAP_ALLOC(sizeof(WORD)*NB_RESERVED_COLORS )))
            return FALSE;
        if (!(hRevSysColorTranslation = GDI_HEAP_ALLOC( sizeof(WORD)*size )))
            return FALSE;
        colorTranslation = (WORD *) GDI_HEAP_LIN_ADDR( hSysColorTranslation );
        revTranslation   = (WORD *) GDI_HEAP_LIN_ADDR( hRevSysColorTranslation );
    }
    else colorTranslation = revTranslation = NULL;

    if ((COLOR_WinColormap == DefaultColormapOfScreen(screen)) && (screenDepth <= 8))
    {
        COLOR_PaletteToPixel = (int *)xmalloc( sizeof(int) * size );
        COLOR_PixelToPalette = (int *)xmalloc( sizeof(int) * size );
        for (i = 0; i < size; i++)  /* Set the default mapping */
            COLOR_PaletteToPixel[i] = COLOR_PixelToPalette[i] = i;
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
            else if (COLOR_PaletteToPixel)
            {
                COLOR_PaletteToPixel[pixel] = color.pixel;
                COLOR_PixelToPalette[color.pixel] = pixel;
            }
        }
        if (colorTranslation) colorTranslation[i] = color.pixel;
        if (revTranslation) revTranslation[color.pixel] = i;
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

void
COLOR_Computeshifts(unsigned long maskbits, int *shift, int *max)
{
    int i;

    if(maskbits==0) {
        *shift=0;
        *max=0;
        return;
    }

    for(i=0;!(maskbits&1);i++)
        maskbits >>= 1;

    *shift = i;
    *max = maskbits;
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
        COLOR_FixedMap = 0;
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
	COLOR_WinColormap = DefaultColormapOfScreen( screen );
        break;
    case StaticGray:
	COLOR_WinColormap = DefaultColormapOfScreen( screen );
	COLOR_FixedMap = 1;
        COLOR_Graymax = (1<<screenDepth)-1;
        break;
    case StaticColor:
    case TrueColor:
	COLOR_WinColormap = DefaultColormapOfScreen( screen );
	COLOR_FixedMap = 1;
        COLOR_Computeshifts(visual->red_mask, &COLOR_Redshift, &COLOR_Redmax);
        COLOR_Computeshifts(visual->green_mask, &COLOR_Greenshift, &COLOR_Greenmax);
        COLOR_Computeshifts(visual->blue_mask, &COLOR_Blueshift, &COLOR_Bluemax);
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
 *
 * ESW: This still needs a lot of work; in particular, what happens when
 * we have really large (> 8 bit) DirectColor boards?
 * But it should work better in 16 and 24 bit modes now.
 * (At least it pays attention to the X server's description
 * in TrueColor mode, instead of blindly passing through the
 * color spec and swapping red to blue or losing blue and part of the
 * green entirely, depending on what mode one is in!)
 *
 */
int COLOR_ToPhysical( DC *dc, COLORREF color )
{
    WORD index = 0;
    WORD *mapping;

    if (dc && (dc->w.bitsPerPixel == 1) && ((color >> 24) == 0))
    {
	/* monochrome */
        if (((color >> 16) & 0xff) +
            ((color >> 8) & 0xff) + (color & 0xff) > 255*3/2)
            return 1;  /* white */
        else return 0;  /* black */
    }

    if (COLOR_FixedMap)
    {
        /* there is no colormap possible; we are going to have to compute
           the pixel value from the visual information stored earlier */

	unsigned long red, green, blue;
	unsigned idx;
	PALETTEOBJ * palPtr;

	switch(color >> 24)
        {
        case 0: /* RGB */
        case 2: /* PALETTERGB -- needs some work, but why bother; we've got a REALLY LARGE number of colors...? */
        default:
            red = GetRValue(color);
            green = GetGValue(color);
            blue = GetBValue(color);
            break;

        case 1: /* PALETTEIDX -- hmm, get the real color from the stock palette */
            palPtr = (PALETTEOBJ *) GDI_GetObjPtr( STOCK_DEFAULT_PALETTE, PALETTE_MAGIC);
            idx = color & 0xffff;
            if (idx >= palPtr->logpalette.palNumEntries)
            {
                fprintf(stderr, "COLOR_ToPhysical(%lx) : idx %d is out of bounds, assuming black\n", color, idx);
                /* out of bounds */
                red = green = blue = 0;
            }
            else
            {
                red = palPtr->logpalette.palPalEntry[idx].peRed;
                green = palPtr->logpalette.palPalEntry[idx].peGreen;
                blue = palPtr->logpalette.palPalEntry[idx].peBlue;
            }
	}

	if (COLOR_Graymax)
        {
	    /* grayscale only; return scaled value */
            return ( (red * 30 + green * 69 + blue * 11) * COLOR_Graymax) / 25500;
	}
	else
        {
	    /* scale each individually and construct the TrueColor pixel value */
	    if (COLOR_Redmax != 255) red = (red * COLOR_Redmax) / 255;
            if (COLOR_Greenmax != 255) green = (green * COLOR_Greenmax) / 255;
	    if (COLOR_Bluemax != 255) blue = (blue * COLOR_Bluemax) / 255;

	    return (red << COLOR_Redshift) | (green << COLOR_Greenshift) | (blue << COLOR_Blueshift);
        }
    }
    else switch(color >> 24)
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
    if (dc&&dc->u.x.pal.mappingSize)
    {
        if (index >= dc->u.x.pal.mappingSize)
        {
            fprintf(stderr, "COLOR_ToPhysical(%lx) : idx %d is >= dc->u.x.pal.mappingSize, assuming pixel 0\n", color, index);
            return 0;
        }
        mapping = (WORD *) GDI_HEAP_LIN_ADDR( dc->u.x.pal.hMapping );
    }
    else
    {
        if (index >= NB_RESERVED_COLORS)
        {
            fprintf(stderr, "COLOR_ToPhysical(%lx) : idx %d is >= NB_RESERVED_COLORS, assuming pixel 0\n", color, index);
	    return 0;
        }
        mapping = (WORD *) GDI_HEAP_LIN_ADDR( hSysColorTranslation );
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
	dc->u.x.pal.hMapping = GDI_HEAP_ALLOC( sizeof(WORD) * size );
	pmap = (WORD *) GDI_HEAP_LIN_ADDR( map );
	pnewmap = (WORD *) GDI_HEAP_LIN_ADDR( dc->u.x.pal.hMapping );
	memcpy( pnewmap, pmap, sizeof(WORD)*size );
          /* Build reverse table */
        dc->u.x.pal.hRevMapping = GDI_HEAP_ALLOC(sizeof(WORD)*COLOR_ColormapSize);
        pmap = (WORD *) GDI_HEAP_LIN_ADDR( dc->u.x.pal.hRevMapping );
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
