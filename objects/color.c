/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 * Copyright 1996 Alex Korobka
 */

#include <stdlib.h>
#include "ts_xlib.h"
#include <string.h>
#include "windows.h"
#include "options.h"
#include "gdi.h"
#include "color.h"
#include "debug.h"
#include "xmalloc.h"

/* Palette indexed mode:

 *	logical palette -> mapping -> pixel
 *				     
 *
 * Windows needs contiguous color space ( from 0 to n ) but 
 * it is possible only with the private colormap. Otherwise we
 * have to map DC palette indices to real pixel values. With 
 * private colormaps it boils down to the identity mapping. The
 * other special case is when we have a fixed color visual with 
 * the screendepth > 8 - we abandon palette mappings altogether 
 * because pixel values can be calculated without X server 
 * assistance.
 *
 * Windows palette manager is described in the
 * http://premium.microsoft.com/msdn/library/techart/f30/f34/f40/d4d/sa942.htm
 */

typedef struct
{ 
    Colormap    colorMap;
    UINT16      size;
    UINT16      flags;
    INT32	monoPlane;	 /* bit plane different for white and black pixels */

    INT32	(*mapColor)( DC*, COLORREF );
} CSPACE;

static CSPACE cSpace = {0, 0, 0};

static int COLOR_Redshift   = 0; /* to handle abortive COLOR_VIRTUAL visuals */
static int COLOR_Redmax     = 0;
static int COLOR_Greenshift = 0;
static int COLOR_Greenmax   = 0;
static int COLOR_Blueshift  = 0;
static int COLOR_Bluemax    = 0;
static int COLOR_Graymax    = 0;

/* System color space. 
 *
 * First 10 and last 10 colors in COLOR_sysPalette are
 * "guarded". RealizePalette changes only the rest of colorcells. For
 * currently inactive window it changes only DC palette mappings.
 */

#define NB_COLORCUBE_START_INDEX	63

Visual* 		visual = NULL;

static PALETTEENTRY* COLOR_sysPal = NULL;    /* current system palette */
static int COLOR_gapStart = 256;
static int COLOR_gapEnd = -1;
static int COLOR_gapFilled = 0;
static int COLOR_max = 256;

  /* First free dynamic color cell, 0 = full palette, -1 = fixed palette */
static int            COLOR_firstFree = 0; 
static unsigned char  COLOR_freeList[256];

  /* Maps entry in the system palette to X pixel value */
int* COLOR_PaletteToPixel = NULL;

  /* Maps pixel to the entry in the system palette */
int* COLOR_PixelToPalette = NULL;

static const PALETTEENTRY __sysPalTemplate[NB_RESERVED_COLORS] = 
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

   /* Map an EGA index (0..15) to a pixel value in the system color space.  */

int COLOR_mapEGAPixel[16];

/***********************************************************************
 *	Misc auxiliary functions
 */
Colormap COLOR_GetColormap(void)
{
    return cSpace.colorMap;
}

BOOL32 COLOR_GetMonoPlane(INT32* plane)
{
    *plane = cSpace.monoPlane;
    return (cSpace.flags & COLOR_WHITESET) ? TRUE : FALSE;
}

UINT16 COLOR_GetSystemPaletteSize(void)
{
    return (cSpace.flags & COLOR_PRIVATE) ? cSpace.size : 256;
}

UINT16 COLOR_GetSystemPaletteFlags(void)
{
    return cSpace.flags;
}

const PALETTEENTRY* COLOR_GetSystemPaletteTemplate(void)
{
    return __sysPalTemplate;
}

COLORREF COLOR_GetSystemPaletteEntry(UINT32 i)
{
 return *(COLORREF*)(COLOR_sysPal + i) & 0x00ffffff;
}

void COLOR_FormatSystemPalette(void)
{
 /* Build free list so we'd have an easy way to find
  * out if there are any available colorcells. 
  */

  int i, j = COLOR_firstFree = NB_RESERVED_COLORS/2;

  COLOR_sysPal[j].peFlags = 0;
  for( i = NB_RESERVED_COLORS/2 + 1 ; i < 256 - NB_RESERVED_COLORS/2 ; i++ )
    if( i < COLOR_gapStart || i > COLOR_gapEnd )
      {
	COLOR_sysPal[i].peFlags = 0;  /* unused tag */
	COLOR_freeList[j] = i;	  /* next */
        j = i;
      }
  COLOR_freeList[j] = 0;
}

BOOL32 COLOR_CheckSysColor(COLORREF c)
{
  int i;
  for( i = 0; i < NB_RESERVED_COLORS; i++ )
       if( c == (*(COLORREF*)(__sysPalTemplate + i) & 0x00ffffff) )
	   return 0;
  return 1;
}

/***********************************************************************
 *      Colormap Initialization
 */
static void COLOR_FillDefaultColors(void)
{
 /* initialize unused entries to what Windows uses as a color 
  * cube - based on Greg Kreider's code. 
  */

  int i = 0, idx = 0;
  int red, no_r, inc_r;
  int green, no_g, inc_g; 
  int blue, no_b, inc_b;
  
  if (cSpace.size <= NB_RESERVED_COLORS)
  	return;
  while (i*i*i < (cSpace.size - NB_RESERVED_COLORS)) i++;
  no_r = no_g = no_b = --i;
  if ((no_r * (no_g+1) * no_b) < (cSpace.size - NB_RESERVED_COLORS)) no_g++;
  if ((no_r * no_g * (no_b+1)) < (cSpace.size - NB_RESERVED_COLORS)) no_b++;
  inc_r = (255 - NB_COLORCUBE_START_INDEX)/no_r;
  inc_g = (255 - NB_COLORCUBE_START_INDEX)/no_g;
  inc_b = (255 - NB_COLORCUBE_START_INDEX)/no_b;

  idx = COLOR_firstFree;

  if( idx != -1 )
    for (blue = NB_COLORCUBE_START_INDEX; blue < 256 && idx; blue += inc_b )
     for (green = NB_COLORCUBE_START_INDEX; green < 256 && idx; green += inc_g )
      for (red = NB_COLORCUBE_START_INDEX; red < 256 && idx; red += inc_r )
      {
	 /* weird but true */

	 if( red == NB_COLORCUBE_START_INDEX && green == red && blue == green ) continue;

         COLOR_sysPal[idx].peRed = red;
         COLOR_sysPal[idx].peGreen = green;
         COLOR_sysPal[idx].peBlue = blue;
         
	 /* set X color */

	 if( cSpace.flags & COLOR_VIRTUAL )
	 {
            if (COLOR_Redmax != 255) no_r = (red * COLOR_Redmax) / 255;
            if (COLOR_Greenmax != 255) no_g = (green * COLOR_Greenmax) / 255;
            if (COLOR_Bluemax != 255) no_b = (blue * COLOR_Bluemax) / 255;

            COLOR_PaletteToPixel[idx] = (no_r << COLOR_Redshift) | (no_g << COLOR_Greenshift) | (no_b << COLOR_Blueshift);
	 }
	 else if( !(cSpace.flags & COLOR_FIXED) )
	 {
	   XColor color = { color.pixel = (COLOR_PaletteToPixel)? COLOR_PaletteToPixel[idx] : idx ,
	                    COLOR_sysPal[idx].peRed << 8,
			    COLOR_sysPal[idx].peGreen << 8,
			    COLOR_sysPal[idx].peGreen << 8,
			    (DoRed | DoGreen | DoBlue) };
	   TSXStoreColor(display, cSpace.colorMap, &color);
	 }

	 idx = COLOR_freeList[idx];
      }

  /* try to fill some entries in the "gap" with
   * what's already in the colormap - they will be
   * mappable to but not changeable. */

  if( COLOR_gapStart < COLOR_gapEnd && COLOR_PixelToPalette )
  {
    XColor	xc;
    int		r, g, b, max;

    max = COLOR_max - (256 - (COLOR_gapEnd - COLOR_gapStart));
    for ( i = 0, idx = COLOR_gapStart; i < 256 && idx <= COLOR_gapEnd; i++ )
      if( COLOR_PixelToPalette[i] == 0 )
	{
	  xc.pixel = i;

	  TSXQueryColor(display, cSpace.colorMap, &xc);
	  r = xc.red>>8; g = xc.green>>8; b = xc.blue>>8;

	  if( xc.pixel < 256 && COLOR_CheckSysColor(RGB(r, g, b)) &&
	      TSXAllocColor(display, cSpace.colorMap, &xc) )
	  {
	     COLOR_PixelToPalette[xc.pixel] = idx;
	     COLOR_PaletteToPixel[idx] = xc.pixel;
           *(COLORREF*)(COLOR_sysPal + idx) = RGB(r, g, b);
	     COLOR_sysPal[idx++].peFlags |= PC_SYS_USED;
	     if( --max <= 0 ) break;
	  }
	}
    COLOR_gapFilled = idx - COLOR_gapStart;    
  }
}

/***********************************************************************
 *           COLOR_BuildPrivateMap/COLOR_BuildSharedMap
 *
 * Allocate colorcells and initialize mapping tables.
 */
static BOOL32 COLOR_BuildPrivateMap(CSPACE* cs)
{
    /* Private colormap - identity mapping */

    XColor color;
    int i; 

    COLOR_sysPal = (PALETTEENTRY*)xmalloc(sizeof(PALETTEENTRY)*cs->size);

    TRACE(palette,"Building private map - %i palette entries\n", cs->size);

      /* Allocate system palette colors */ 

    for( i=0; i < cs->size; i++ )
    {
       if( i < NB_RESERVED_COLORS/2 )
       {
         color.red   = __sysPalTemplate[i].peRed * 65535 / 255;
         color.green = __sysPalTemplate[i].peGreen * 65535 / 255;
         color.blue  = __sysPalTemplate[i].peBlue * 65535 / 255;
	 COLOR_sysPal[i] = __sysPalTemplate[i];
       }
       else if( i >= cs->size - NB_RESERVED_COLORS/2 )
       {
	 int j = NB_RESERVED_COLORS + i - cs->size;
         color.red   = __sysPalTemplate[j].peRed * 65535 / 255;
         color.green = __sysPalTemplate[j].peGreen * 65535 / 255;
         color.blue  = __sysPalTemplate[j].peBlue * 65535 / 255;
	 COLOR_sysPal[i] = __sysPalTemplate[j];
       }

       color.flags = DoRed | DoGreen | DoBlue;
       color.pixel = i;
       TSXStoreColor(display, cs->colorMap, &color);

       /* Set EGA mapping if color is from the first or last eight */

       if (i < 8)
           COLOR_mapEGAPixel[i] = color.pixel;
       else if (i >= cs->size - 8 )
           COLOR_mapEGAPixel[i - (cs->size - 16)] = color.pixel;
    }

    COLOR_PixelToPalette = COLOR_PaletteToPixel = NULL;

    COLOR_gapStart = 256; COLOR_gapEnd = -1;

    COLOR_firstFree = (cs->size > NB_RESERVED_COLORS)?NB_RESERVED_COLORS/2 : -1;
    return FALSE;
}

static BOOL32 COLOR_BuildSharedMap(CSPACE* cs)
{
   XColor		color;
   unsigned long        sysPixel[NB_RESERVED_COLORS];
   unsigned long*	pixDynMapping = NULL;
   unsigned long	plane_masks[1];
   int			i, j, warn = 0;
   int			diff, r, g, b, max = 256, bp = 0, wp = 1;
   int			step = 1;

   /* read "AllocSystemColors" from wine.conf */

   COLOR_max = PROFILE_GetWineIniInt( "options", "AllocSystemColors", 256);
   if (COLOR_max > 256) COLOR_max = 256;
   else if (COLOR_max < 20) COLOR_max = 20;
   TRACE(palette,"%d colors configured.\n", COLOR_max);
   
   TRACE(palette,"Building shared map - %i palette entries\n", cs->size);

   /* Be nice and allocate system colors as read-only */

   for( i = 0; i < NB_RESERVED_COLORS; i++ )
     { 
        color.red   = __sysPalTemplate[i].peRed * 65535 / 255;
        color.green = __sysPalTemplate[i].peGreen * 65535 / 255;
        color.blue  = __sysPalTemplate[i].peBlue * 65535 / 255;
        color.flags = DoRed | DoGreen | DoBlue;

        if (!TSXAllocColor( display, cSpace.colorMap, &color ))
        { 
	     XColor	best, c;
	     
             if( !warn++ ) 
	     {
		  WARN(palette, "Not enough colors for the full system palette.\n");

	          bp = BlackPixel(display, DefaultScreen(display));
	          wp = WhitePixel(display, DefaultScreen(display));

	          max = (0xffffffff)>>(32 - screenDepth);
	          if( max > 256 ) 
	          {
	  	      step = max/256;
		      max = 256;
	          }
	     }

	     /* reinit color (XAllocColor() may change it)
	      * and map to the best shared colorcell */

             color.red   = __sysPalTemplate[i].peRed * 65535 / 255;
             color.green = __sysPalTemplate[i].peGreen * 65535 / 255;
             color.blue  = __sysPalTemplate[i].peBlue * 65535 / 255;

	     best.pixel = best.red = best.green = best.blue = 0;
	     for( c.pixel = 0, diff = 0x7fffffff; c.pixel < max; c.pixel += step )
	     {
		TSXQueryColor(display, cSpace.colorMap, &c);
		r = (c.red - color.red)>>8; 
		g = (c.green - color.green)>>8; 
		b = (c.blue - color.blue)>>8;
		r = r*r + g*g + b*b;
		if( r < diff ) { best = c; diff = r; }
	     }

	     if( TSXAllocColor(display, cSpace.colorMap, &best) )
		 color.pixel = best.pixel;
	     else color.pixel = (i < NB_RESERVED_COLORS/2)? bp : wp;
        }

        sysPixel[i] = color.pixel;

        TRACE(palette,"syscolor(%lx) -> pixel %i\n",
		      *(COLORREF*)(__sysPalTemplate+i), (int)color.pixel);

        /* Set EGA mapping if color in the first or last eight */

        if (i < 8)
            COLOR_mapEGAPixel[i] = color.pixel;
        else if (i >= NB_RESERVED_COLORS - 8 )
            COLOR_mapEGAPixel[i - (NB_RESERVED_COLORS-16)] = color.pixel;
     }

   /* now allocate changeable set */

   if( !(cSpace.flags & COLOR_FIXED) )  
     {
	int c_min = 0, c_max = cs->size, c_val;

	TRACE(palette,"Dynamic colormap... \n");

	/* comment this out if you want to debug palette init */

	TSXGrabServer(display);

	/* let's become the first client that actually follows 
	 * X guidelines and does binary search...
	 */

	pixDynMapping = (unsigned long*)xmalloc(sizeof(long)*cs->size);
        while( c_max - c_min > 0 )
          {
             c_val = (c_max + c_min)/2 + (c_max + c_min)%2;

             if( !TSXAllocColorCells(display, cs->colorMap, False,
                                   plane_masks, 0, pixDynMapping, c_val) )
                 c_max = c_val - 1;
             else
               {
                 TSXFreeColors(display, cs->colorMap, pixDynMapping, c_val, 0);
                 c_min = c_val;
               }
          }

	if( c_min > COLOR_max - NB_RESERVED_COLORS) 
	    c_min = COLOR_max - NB_RESERVED_COLORS;

	c_min = (c_min/2) + (c_min/2);		/* need even set for split palette */

	if( c_min > 0 )
	  if( !TSXAllocColorCells(display, cs->colorMap, False,
                                plane_masks, 0, pixDynMapping, c_min) )
	    {
	      WARN(palette,"Inexplicable failure during colorcell allocation.\n");
	      c_min = 0;
	    }

        cs->size = c_min + NB_RESERVED_COLORS;

	TSXUngrabServer(display);

	TRACE(palette,"adjusted size %i colorcells\n", cs->size);
     }
   else if( cSpace.flags & COLOR_VIRTUAL ) 
	{
          /* virtual colorspace - ToPhysical takes care of 
           * color translations but we have to allocate full palette 
	   * to maintain compatibility
	   */
	  cs->size = 256;
	  TRACE(palette,"Virtual colorspace - screendepth %i\n", screenDepth);
	}
   else cs->size = NB_RESERVED_COLORS;	/* system palette only - however we can alloc a bunch
			                 * of colors and map to them */

   TRACE(palette,"Shared system palette uses %i colors.\n", cs->size);

   /* set gap to account for pixel shortage. It has to be right in the center
    * of the system palette because otherwise raster ops get screwed. */

   if( cs->size >= 256 )
     { COLOR_gapStart = 256; COLOR_gapEnd = -1; }
   else
     { COLOR_gapStart = cs->size/2; COLOR_gapEnd = 255 - cs->size/2; }

   COLOR_firstFree = ( cs->size > NB_RESERVED_COLORS && 
		      (cs->flags & COLOR_VIRTUAL || !(cs->flags & COLOR_FIXED)) ) 
		     ? NB_RESERVED_COLORS/2 : -1;

   COLOR_sysPal = (PALETTEENTRY*)xmalloc(sizeof(PALETTEENTRY)*256);

   /* setup system palette entry <-> pixel mappings and fill in 20 fixed entries */

   if( screenDepth <= 8 )
     {
       COLOR_PixelToPalette = (int*)xmalloc(sizeof(int)*256);
       memset( COLOR_PixelToPalette, 0, 256*sizeof(int) );
     }

   /* for hicolor visuals PaletteToPixel mapping is used to skip
    * RGB->pixel calculation in COLOR_ToPhysical(). 
    */

   COLOR_PaletteToPixel = (int*)xmalloc(sizeof(int)*256);

   for( i = j = 0; i < 256; i++ )
   {
      if( i >= COLOR_gapStart && i <= COLOR_gapEnd ) 
      {
         COLOR_PaletteToPixel[i] = 0;
         COLOR_sysPal[i].peFlags = 0;	/* mark as unused */
         continue;
      }

      if( i < NB_RESERVED_COLORS/2 )
      {
        COLOR_PaletteToPixel[i] = sysPixel[i];
        COLOR_sysPal[i] = __sysPalTemplate[i];
      }
      else if( i >= 256 - NB_RESERVED_COLORS/2 )
      {
        COLOR_PaletteToPixel[i] = sysPixel[(i + NB_RESERVED_COLORS) - 256]; 
        COLOR_sysPal[i] = __sysPalTemplate[(i + NB_RESERVED_COLORS) - 256];
      }
      else if( pixDynMapping )
             COLOR_PaletteToPixel[i] = pixDynMapping[j++];
           else
             COLOR_PaletteToPixel[i] = i;

      TRACE(palette,"index %i -> pixel %i\n", i, COLOR_PaletteToPixel[i]);

      if( COLOR_PixelToPalette )
          COLOR_PixelToPalette[COLOR_PaletteToPixel[i]] = i;
   }

   if( pixDynMapping ) free(pixDynMapping);
   return TRUE;
}

/***********************************************************************
 *	     COLOR_Computeshifts
 *
 * Calculate conversion parameters for direct mapped visuals
 */
static void COLOR_Computeshifts(unsigned long maskbits, int *shift, int *max)
{
    int i;

    if (maskbits==0)
    {
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
 * Initialize color management.
 */
BOOL32 COLOR_Init(void)
{
    int	mask, white, black;

    visual = DefaultVisual( display, DefaultScreen(display) );

    TRACE(palette,"initializing palette manager...\n");

    white = WhitePixelOfScreen( screen );
    black = BlackPixelOfScreen( screen );
    cSpace.monoPlane = 1;
    for( mask = 1; !((white & mask)^(black & mask)); mask <<= 1 )
	 cSpace.monoPlane++;
    cSpace.flags = (white & mask) ? COLOR_WHITESET : 0;
    cSpace.size = visual->map_entries;

    switch(visual->class)
    {
    case DirectColor:
	cSpace.flags |= COLOR_VIRTUAL;
    case GrayScale:
    case PseudoColor:
	if (Options.usePrivateMap)
	{
	    XSetWindowAttributes win_attr;

	    cSpace.colorMap = TSXCreateColormap( display, rootWindow,
						 visual, AllocAll );
	    if (cSpace.colorMap)
	    {
	        cSpace.flags |= (COLOR_PRIVATE | COLOR_WHITESET);

		cSpace.monoPlane = 1;
		for( white = cSpace.size - 1; !(white & 1); white >>= 1 )
		     cSpace.monoPlane++;

	        if( rootWindow != DefaultRootWindow(display) )
	        {
		    win_attr.colormap = cSpace.colorMap;
		    TSXChangeWindowAttributes( display, rootWindow,
					 CWColormap, &win_attr );
		}
		break;
	    }
	}
	cSpace.colorMap = DefaultColormapOfScreen( screen );
        break;

    case StaticGray:
	cSpace.colorMap = DefaultColormapOfScreen( screen );
	cSpace.flags |= COLOR_FIXED;
        COLOR_Graymax = (1<<screenDepth)-1;
        break;

    case TrueColor:
	cSpace.flags |= COLOR_VIRTUAL;
    case StaticColor: {
    	int *depths,nrofdepths;
	/* FIXME: hack to detect XFree32 XF_VGA16 ... We just have
	 * depths 1 and 4
	 */
	depths=TSXListDepths(display,DefaultScreen(display),&nrofdepths);
	if ((nrofdepths==2) && ((depths[0]==4) || depths[1]==4)) {
	    cSpace.monoPlane = 1;
	    for( white = cSpace.size - 1; !(white & 1); white >>= 1 )
	        cSpace.monoPlane++;
    	    cSpace.flags = (white & mask) ? COLOR_WHITESET : 0;
	    cSpace.colorMap = DefaultColormapOfScreen( screen );
	    TSXFree(depths);
	    break;
	}
	TSXFree(depths);
        cSpace.colorMap = DefaultColormapOfScreen( screen );
        cSpace.flags |= COLOR_FIXED;
        COLOR_Computeshifts(visual->red_mask, &COLOR_Redshift, &COLOR_Redmax);
        COLOR_Computeshifts(visual->green_mask, &COLOR_Greenshift, &COLOR_Greenmax);
        COLOR_Computeshifts(visual->blue_mask, &COLOR_Blueshift, &COLOR_Bluemax);
	break;
    }
    }

    TRACE(palette," visual class %i (%i)\n", 
		    visual->class, cSpace.monoPlane);

    memset(COLOR_freeList, 0, 256*sizeof(unsigned char));

    if (cSpace.flags & COLOR_PRIVATE)
        COLOR_BuildPrivateMap( &cSpace );
    else
        COLOR_BuildSharedMap( &cSpace );

    /* Build free list */

    if( COLOR_firstFree != -1 )
        COLOR_FormatSystemPalette();

    COLOR_FillDefaultColors();

    return TRUE;
}

/***********************************************************************
 *           COLOR_Cleanup
 *
 * Free external colors we grabbed in the FillDefaultPalette()
 */
void COLOR_Cleanup(void)
{
  if( COLOR_gapFilled )
      TSXFreeColors(display, cSpace.colorMap, 
		  (unsigned long*)(COLOR_PaletteToPixel + COLOR_gapStart), 
		  COLOR_gapFilled, 0);
}

/***********************************************************************
 *           COLOR_IsSolid
 *
 * Check whether 'color' can be represented with a solid color.
 */
BOOL32 COLOR_IsSolid( COLORREF color )
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
                              int* mapping, COLORREF col, BOOL32 skipReserved )
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
 *           COLOR_LookupSystemPixel
 */
int COLOR_LookupSystemPixel(COLORREF col)
{
 int            i, best = 0, diff = 0x7fffffff;
 int            size = COLOR_GetSystemPaletteSize();
 int            r,g,b;

 for( i = 0; i < size && diff ; i++ )
    {
      if( i == NB_RESERVED_COLORS/2 )
      {
      	int newi = size - NB_RESERVED_COLORS/2;
	if (newi>i) i=newi;
      }

      r = COLOR_sysPal[i].peRed - GetRValue(col);
      g = COLOR_sysPal[i].peGreen - GetGValue(col);
      b = COLOR_sysPal[i].peBlue - GetBValue(col);

      r = r*r + g*g + b*b;

      if( r < diff ) { best = i; diff = r; }
    }
 
 return (COLOR_PaletteToPixel)? COLOR_PaletteToPixel[best] : best;
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

/***********************************************************************
 *           COLOR_ToLogical
 *
 * Return RGB color for given X pixel.
 */
COLORREF COLOR_ToLogical(int pixel)
{
    XColor color;

#if 0
    /* truecolor visual */

    if (screenDepth >= 24) return pixel;
#endif

    /* check for hicolor visuals first */

    if ( cSpace.flags & COLOR_FIXED && !COLOR_Graymax )
    {
         color.red = (pixel >> COLOR_Redshift) & COLOR_Redmax;
         color.green = (pixel >> COLOR_Greenshift) & COLOR_Greenmax;
         color.blue = (pixel >> COLOR_Blueshift) & COLOR_Bluemax;
         return RGB((color.red * 255)/COLOR_Redmax,
                    (color.green * 255)/COLOR_Greenmax,
                    (color.blue * 255)/COLOR_Bluemax);
    }

    /* check if we can bypass X */

    if ((screenDepth <= 8) && (pixel < 256) && 
       !(cSpace.flags & (COLOR_VIRTUAL | COLOR_FIXED)) )
         return  ( *(COLORREF*)(COLOR_sysPal + 
		   ((COLOR_PixelToPalette)?COLOR_PixelToPalette[pixel]:pixel)) ) & 0x00ffffff;

    color.pixel = pixel;
    TSXQueryColor(display, cSpace.colorMap, &color);
    return RGB(color.red >> 8, color.green >> 8, color.blue >> 8);
}


/***********************************************************************
 *           COLOR_ToPhysical
 *
 * Return the physical color closest to 'color'.
 */
int COLOR_ToPhysical( DC *dc, COLORREF color )
{
    WORD 		 index = 0;
    HPALETTE16		 hPal = (dc)? dc->w.hPalette: STOCK_DEFAULT_PALETTE;
    unsigned char	 spec_type = color >> 24;
    PALETTEOBJ* 	 palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );

    if ( cSpace.flags & COLOR_FIXED )
    {
        /* there is no colormap limitation; we are going to have to compute
         * the pixel value from the visual information stored earlier 
	 */

	unsigned 	long red, green, blue;
	unsigned 	idx = 0;

	switch(spec_type)
        {
          case 2: /* PALETTERGB - not sure if we really need to search palette */
	
	    idx = COLOR_PaletteLookupPixel( palPtr->logpalette.palPalEntry,
					    palPtr->logpalette.palNumEntries,
					    NULL, color, FALSE);

            if( palPtr->mapping )
	    {
	        GDI_HEAP_UNLOCK( hPal );
		return palPtr->mapping[idx];
	    }

	    color = *(COLORREF*)(palPtr->logpalette.palPalEntry + idx);
	    break;

          case 1: /* PALETTEINDEX */

            if( (idx = color & 0xffff) >= palPtr->logpalette.palNumEntries)
            {
                WARN(palette, "RGB(%lx) : idx %d is out of bounds, assuming black\n", color, idx);
		GDI_HEAP_UNLOCK( hPal );
                return 0;
            }

            if( palPtr->mapping ) 
	    {
		GDI_HEAP_UNLOCK( hPal );
		return palPtr->mapping[idx];
	    }
	    color = *(COLORREF*)(palPtr->logpalette.palPalEntry + idx);
	    break;

	  default:
	    color &= 0xffffff;
	    /* fall through to RGB */

	  case 0: /* RGB */
	    if( dc && (dc->w.bitsPerPixel == 1) )
	    {
		GDI_HEAP_UNLOCK( hPal );
		return (((color >> 16) & 0xff) +
			((color >> 8) & 0xff) + (color & 0xff) > 255*3/2) ? 1 : 0;
	    }

	}

        red = GetRValue(color); green = GetGValue(color); blue = GetBValue(color);

	if (COLOR_Graymax)
        {
	    /* grayscale only; return scaled value */
	    GDI_HEAP_UNLOCK( hPal );
            return ( (red * 30 + green * 69 + blue * 11) * COLOR_Graymax) / 25500;
	}
	else
        {
	    /* scale each individually and construct the TrueColor pixel value */
	    if (COLOR_Redmax != 255) red = (red * COLOR_Redmax) / 255;
            if (COLOR_Greenmax != 255) green = (green * COLOR_Greenmax) / 255;
	    if (COLOR_Bluemax != 255) blue = (blue * COLOR_Bluemax) / 255;

	    GDI_HEAP_UNLOCK( hPal );
	    return (red << COLOR_Redshift) | (green << COLOR_Greenshift) | (blue << COLOR_Blueshift);
        }
    }
    else 
    {

	/* palPtr can be NULL when DC is being destroyed */

	if( !palPtr ) return 0;
	else if( !palPtr->mapping ) 
            WARN(palette, "Palette %04x is not realized\n", dc->w.hPalette);

	switch(spec_type)	/* we have to peruse DC and system palette */
    	{
	    default:
		color &= 0xffffff;
		/* fall through to RGB */

       	    case 0:  /* RGB */
		if( dc && (dc->w.bitsPerPixel == 1) )
		{
		    GDI_HEAP_UNLOCK( hPal );
		    return (((color >> 16) & 0xff) +
			    ((color >> 8) & 0xff) + (color & 0xff) > 255*3/2) ? 1 : 0;
		}

	    	index = COLOR_PaletteLookupPixel( COLOR_sysPal, 256, 
						  COLOR_PaletteToPixel, color, FALSE);

		/* TRACE(palette,"RGB(%lx) -> pixel %i\n", color, index);
		 */
	    	break;
       	    case 1:  /* PALETTEINDEX */
		index = color & 0xffff;

	        if( index >= palPtr->logpalette.palNumEntries )
		    WARN(palette, "RGB(%lx) : index %i is out of bounds\n", color, index); 
		else if( palPtr->mapping ) index = palPtr->mapping[index];

		/*  TRACE(palette,"PALETTEINDEX(%04x) -> pixel %i\n", (WORD)color, index);
		 */
		break;
            case 2:  /* PALETTERGB */
		index = COLOR_PaletteLookupPixel( palPtr->logpalette.palPalEntry, 
                                             palPtr->logpalette.palNumEntries,
                                             palPtr->mapping, color, FALSE);
		/* TRACE(palette,"PALETTERGB(%lx) -> pixel %i\n", color, index);
		 */
		break;
	}
    }

    GDI_HEAP_UNLOCK( hPal );
    return index;
}

/***********************************************************************
 *           COLOR_SetMapping
 *
 * Set the color-mapping table for selected palette. 
 * Return number of entries which mapping has changed.
 */
int COLOR_SetMapping( PALETTEOBJ* palPtr, UINT32 uStart, UINT32 uNum, BOOL32 mapOnly )
{
    char flag;
    int  prevMapping = (palPtr->mapping) ? 1 : 0;
    int  index, iRemapped = 0;

    /* reset dynamic system palette entries */

    if( !mapOnly && COLOR_firstFree != -1)
         COLOR_FormatSystemPalette();

    /* initialize palette mapping table */
 
    palPtr->mapping = (int*)xrealloc(palPtr->mapping, sizeof(int)*
                                     palPtr->logpalette.palNumEntries);

    for( uNum += uStart; uStart < uNum; uStart++ )
    {
        index = -1;
        flag = PC_SYS_USED;

        switch( palPtr->logpalette.palPalEntry[uStart].peFlags & 0x07 )
        {
	case PC_EXPLICIT:   /* palette entries are indices into system palette */
            index = *(WORD*)(palPtr->logpalette.palPalEntry + uStart);
            if( index > 255 || (index >= COLOR_gapStart && index <= COLOR_gapEnd) ) 
            {
                WARN(palette,"PC_EXPLICIT: idx %d out of system palette, assuming black.\n", index); 
                index = 0;
            }
            break;

	case PC_RESERVED:   /* forbid future mappings to this entry */
            flag |= PC_SYS_RESERVED;

            /* fall through */
	default:	    /* try to collapse identical colors */
            index = COLOR_PaletteLookupExactIndex(COLOR_sysPal, 256,  
                             *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart));
            /* fall through */
	case PC_NOCOLLAPSE: 
            if( index < 0 )
            {
                if( COLOR_firstFree > 0 && !(cSpace.flags & COLOR_FIXED) )
                {
                    XColor color;
                    index = COLOR_firstFree;  /* ought to be available */
                    COLOR_firstFree = COLOR_freeList[index];

                    color.pixel = (COLOR_PaletteToPixel) ? COLOR_PaletteToPixel[index] : index;
                    color.red = palPtr->logpalette.palPalEntry[uStart].peRed << 8;
                    color.green = palPtr->logpalette.palPalEntry[uStart].peGreen << 8;
                    color.blue = palPtr->logpalette.palPalEntry[uStart].peBlue << 8;
                    color.flags = DoRed | DoGreen | DoBlue;
                    TSXStoreColor(display, cSpace.colorMap, &color);

                    COLOR_sysPal[index] = palPtr->logpalette.palPalEntry[uStart];
                    COLOR_sysPal[index].peFlags = flag;
		    COLOR_freeList[index] = 0;

                    if( COLOR_PaletteToPixel ) index = COLOR_PaletteToPixel[index];
                    break;
                }
                else if ( cSpace.flags & COLOR_VIRTUAL ) 
                {
                    index = COLOR_ToPhysical( NULL, 0x00ffffff &
                             *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart));
                    break;     
                }

                /* we have to map to existing entry in the system palette */

                index = COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL, 
                       *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart), TRUE);
            }
	    palPtr->logpalette.palPalEntry[uStart].peFlags |= PC_SYS_USED;

            if( COLOR_PaletteToPixel ) index = COLOR_PaletteToPixel[index];
            break;
        }

        if( !prevMapping || palPtr->mapping[uStart] != index ) iRemapped++;
        palPtr->mapping[uStart] = index;

        TRACE(palette,"entry %i (%lx) -> pixel %i\n", uStart, 
				*(COLORREF*)(palPtr->logpalette.palPalEntry + uStart), index);
	
    }
    return iRemapped;
}

