/* 
 * X11DRV OEM bitmap objects
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "ts_xlib.h"

#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "debugtools.h"
#include "gdi.h"
#include "monitor.h"
#include "options.h"
#include "palette.h"
#include "windef.h"
#include "xmalloc.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(palette)

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

extern PALETTEENTRY *COLOR_sysPal;
extern int COLOR_gapStart;
extern int COLOR_gapEnd;
extern int COLOR_gapFilled;
extern int COLOR_max;

extern const PALETTEENTRY COLOR_sysPalTemplate[NB_RESERVED_COLORS]; 

Colormap X11DRV_PALETTE_PaletteXColormap = 0;
UINT16   X11DRV_PALETTE_PaletteFlags     = 0;

static int X11DRV_PALETTE_Redshift   = 0; /* to handle abortive X11DRV_PALETTE_VIRTUAL visuals */
static int X11DRV_PALETTE_Redmax     = 0;
static int X11DRV_PALETTE_Greenshift = 0;
static int X11DRV_PALETTE_Greenmax   = 0;
static int X11DRV_PALETTE_Blueshift  = 0;
static int X11DRV_PALETTE_Bluemax    = 0;
static int X11DRV_PALETTE_Graymax    = 0;

/* First free dynamic color cell, 0 = full palette, -1 = fixed palette */
static int           X11DRV_PALETTE_firstFree = 0; 
static unsigned char X11DRV_PALETTE_freeList[256];

/**********************************************************************/

   /* Map an EGA index (0..15) to a pixel value in the system color space.  */

int X11DRV_PALETTE_mapEGAPixel[16];

/**********************************************************************/

#define NB_COLORCUBE_START_INDEX	63

/* Maps entry in the system palette to X pixel value */
int *X11DRV_PALETTE_PaletteToXPixel = NULL;

/* Maps pixel to the entry in the system palette */
int *X11DRV_PALETTE_XPixelToPalette = NULL;

/**********************************************************************/

static BOOL X11DRV_PALETTE_BuildPrivateMap(void);
static BOOL X11DRV_PALETTE_BuildSharedMap(void);
static void X11DRV_PALETTE_ComputeShifts(unsigned long maskbits, int *shift, int *max);
static void X11DRV_PALETTE_FillDefaultColors(void);
static void X11DRV_PALETTE_FormatSystemPalette(void);
static BOOL X11DRV_PALETTE_CheckSysColor(COLORREF c);
static int X11DRV_PALETTE_LookupSystemXPixel(COLORREF col);

/***********************************************************************
 *           COLOR_Init
 *
 * Initialize color management.
 */
BOOL X11DRV_PALETTE_Init(void)
{
    int	mask, white, black;
    int monoPlane;

    Visual *visual = DefaultVisual( display, DefaultScreen(display) );

    TRACE("initializing palette manager...\n");

    white = WhitePixelOfScreen( X11DRV_GetXScreen() );
    black = BlackPixelOfScreen( X11DRV_GetXScreen() );
    monoPlane = 1;
    for( mask = 1; !((white & mask)^(black & mask)); mask <<= 1 )
	 monoPlane++;
    X11DRV_PALETTE_PaletteFlags = (white & mask) ? X11DRV_PALETTE_WHITESET : 0;
    X11DRV_DevCaps.sizePalette = visual->map_entries;

    switch(visual->class)
    {
    case DirectColor:
	X11DRV_PALETTE_PaletteFlags |= X11DRV_PALETTE_VIRTUAL;
    case GrayScale:
    case PseudoColor:
	if (PROFILE_GetWineIniBool( "x11drv", "PrivateColorMap", 0 ))
	{
	    XSetWindowAttributes win_attr;

	    X11DRV_PALETTE_PaletteXColormap = TSXCreateColormap( display, X11DRV_GetXRootWindow(),
						 visual, AllocAll );
	    if (X11DRV_PALETTE_PaletteXColormap)
	    {
	        X11DRV_PALETTE_PaletteFlags |= (X11DRV_PALETTE_PRIVATE | X11DRV_PALETTE_WHITESET);

		monoPlane = 1;
		for( white = X11DRV_DevCaps.sizePalette - 1; !(white & 1); white >>= 1 )
		     monoPlane++;

	        if( X11DRV_GetXRootWindow() != DefaultRootWindow(display) )
	        {
		    win_attr.colormap = X11DRV_PALETTE_PaletteXColormap;
		    TSXChangeWindowAttributes( display, X11DRV_GetXRootWindow(),
					 CWColormap, &win_attr );
		}
		break;
	    }
	}
	X11DRV_PALETTE_PaletteXColormap = DefaultColormapOfScreen( X11DRV_GetXScreen() );
        break;

    case StaticGray:
	X11DRV_PALETTE_PaletteXColormap = DefaultColormapOfScreen( X11DRV_GetXScreen() );
	X11DRV_PALETTE_PaletteFlags |= X11DRV_PALETTE_FIXED;
        X11DRV_PALETTE_Graymax = (1<<MONITOR_GetDepth(&MONITOR_PrimaryMonitor))-1;
        break;

    case TrueColor:
	X11DRV_PALETTE_PaletteFlags |= X11DRV_PALETTE_VIRTUAL;
    case StaticColor: {
    	int *depths,nrofdepths;
	/* FIXME: hack to detect XFree32 XF_VGA16 ... We just have
	 * depths 1 and 4
	 */
	depths=TSXListDepths(display,DefaultScreen(display),&nrofdepths);
	if ((nrofdepths==2) && ((depths[0]==4) || depths[1]==4)) {
	    monoPlane = 1;
	    for( white = X11DRV_DevCaps.sizePalette - 1; !(white & 1); white >>= 1 )
	        monoPlane++;
    	    X11DRV_PALETTE_PaletteFlags = (white & mask) ? X11DRV_PALETTE_WHITESET : 0;
	    X11DRV_PALETTE_PaletteXColormap = DefaultColormapOfScreen( X11DRV_GetXScreen() );
	    TSXFree(depths);
	    break;
	}
	TSXFree(depths);
        X11DRV_PALETTE_PaletteXColormap = DefaultColormapOfScreen( X11DRV_GetXScreen() );
        X11DRV_PALETTE_PaletteFlags |= X11DRV_PALETTE_FIXED;
        X11DRV_PALETTE_ComputeShifts(visual->red_mask, &X11DRV_PALETTE_Redshift, &X11DRV_PALETTE_Redmax);
	X11DRV_PALETTE_ComputeShifts(visual->green_mask, &X11DRV_PALETTE_Greenshift, &X11DRV_PALETTE_Greenmax);
        X11DRV_PALETTE_ComputeShifts(visual->blue_mask, &X11DRV_PALETTE_Blueshift, &X11DRV_PALETTE_Bluemax);
	break;
    }
    }

    TRACE(" visual class %i (%i)\n",  visual->class, monoPlane);

    memset(X11DRV_PALETTE_freeList, 0, 256*sizeof(unsigned char));

    if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
        X11DRV_PALETTE_BuildPrivateMap();
    else
        X11DRV_PALETTE_BuildSharedMap();

    /* Build free list */

    if( X11DRV_PALETTE_firstFree != -1 )
        X11DRV_PALETTE_FormatSystemPalette();

    X11DRV_PALETTE_FillDefaultColors();

    if( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL ) 
	X11DRV_DevCaps.sizePalette = 0;
    else
    {
	X11DRV_DevCaps.rasterCaps |= RC_PALETTE;
	X11DRV_DevCaps.sizePalette = visual->map_entries;
    }

    return TRUE;
}

/***********************************************************************
 *           X11DRV_PALETTE_Cleanup
 *
 * Free external colors we grabbed in the FillDefaultPalette()
 */
void X11DRV_PALETTE_Cleanup(void)
{
  if( COLOR_gapFilled )
    TSXFreeColors(display, X11DRV_PALETTE_PaletteXColormap, 
		  (unsigned long*)(X11DRV_PALETTE_PaletteToXPixel + COLOR_gapStart), 
		  COLOR_gapFilled, 0);
}

/***********************************************************************
 *		X11DRV_PALETTE_ComputeShifts
 *
 * Calculate conversion parameters for direct mapped visuals
 */
static void X11DRV_PALETTE_ComputeShifts(unsigned long maskbits, int *shift, int *max)
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
 *           X11DRV_PALETTE_BuildPrivateMap
 *
 * Allocate colorcells and initialize mapping tables.
 */
static BOOL X11DRV_PALETTE_BuildPrivateMap(void)
{
    /* Private colormap - identity mapping */

    XColor color;
    int i; 

    COLOR_sysPal = (PALETTEENTRY*)xmalloc(sizeof(PALETTEENTRY)*X11DRV_DevCaps.sizePalette);

    TRACE("Building private map - %i palette entries\n", X11DRV_DevCaps.sizePalette);

      /* Allocate system palette colors */ 

    for( i=0; i < X11DRV_DevCaps.sizePalette; i++ )
    {
       if( i < NB_RESERVED_COLORS/2 )
       {
         color.red   = COLOR_sysPalTemplate[i].peRed * 65535 / 255;
         color.green = COLOR_sysPalTemplate[i].peGreen * 65535 / 255;
         color.blue  = COLOR_sysPalTemplate[i].peBlue * 65535 / 255;
	 COLOR_sysPal[i] = COLOR_sysPalTemplate[i];
       }
       else if( i >= X11DRV_DevCaps.sizePalette - NB_RESERVED_COLORS/2 )
       {
	 int j = NB_RESERVED_COLORS + i - X11DRV_DevCaps.sizePalette;
         color.red   = COLOR_sysPalTemplate[j].peRed * 65535 / 255;
         color.green = COLOR_sysPalTemplate[j].peGreen * 65535 / 255;
         color.blue  = COLOR_sysPalTemplate[j].peBlue * 65535 / 255;
	 COLOR_sysPal[i] = COLOR_sysPalTemplate[j];
       }

       color.flags = DoRed | DoGreen | DoBlue;
       color.pixel = i;
       TSXStoreColor(display, X11DRV_PALETTE_PaletteXColormap, &color);

       /* Set EGA mapping if color is from the first or last eight */

       if (i < 8)
           X11DRV_PALETTE_mapEGAPixel[i] = color.pixel;
       else if (i >= X11DRV_DevCaps.sizePalette - 8 )
           X11DRV_PALETTE_mapEGAPixel[i - (X11DRV_DevCaps.sizePalette - 16)] = color.pixel;
    }

    X11DRV_PALETTE_XPixelToPalette = X11DRV_PALETTE_PaletteToXPixel = NULL;

    COLOR_gapStart = 256; COLOR_gapEnd = -1;

    X11DRV_PALETTE_firstFree = (X11DRV_DevCaps.sizePalette > NB_RESERVED_COLORS)?NB_RESERVED_COLORS/2 : -1;

    return FALSE;
}

/***********************************************************************
 *           X11DRV_PALETTE_BuildSharedMap
 *
 * Allocate colorcells and initialize mapping tables.
 */
static BOOL X11DRV_PALETTE_BuildSharedMap(void)
{
   XColor		color;
   unsigned long        sysPixel[NB_RESERVED_COLORS];
   unsigned long*	pixDynMapping = NULL;
   unsigned long	plane_masks[1];
   int			i, j, warn = 0;
   int			diff, r, g, b, max = 256, bp = 0, wp = 1;
   int			step = 1;

   /* read "AllocSystemColors" from wine.conf */

   COLOR_max = PROFILE_GetWineIniInt( "x11drv", "AllocSystemColors", 256);
   if (COLOR_max > 256) COLOR_max = 256;
   else if (COLOR_max < 20) COLOR_max = 20;
   TRACE("%d colors configured.\n", COLOR_max);
   
   TRACE("Building shared map - %i palette entries\n", X11DRV_DevCaps.sizePalette);

   /* Be nice and allocate system colors as read-only */

   for( i = 0; i < NB_RESERVED_COLORS; i++ )
     { 
        color.red   = COLOR_sysPalTemplate[i].peRed * 65535 / 255;
        color.green = COLOR_sysPalTemplate[i].peGreen * 65535 / 255;
        color.blue  = COLOR_sysPalTemplate[i].peBlue * 65535 / 255;
        color.flags = DoRed | DoGreen | DoBlue;

        if (!TSXAllocColor( display, X11DRV_PALETTE_PaletteXColormap, &color ))
        { 
	     XColor	best, c;
	     
             if( !warn++ ) 
	     {
		  WARN("Not enough colors for the full system palette.\n");

	          bp = BlackPixel(display, DefaultScreen(display));
	          wp = WhitePixel(display, DefaultScreen(display));

	          max = (0xffffffff)>>(32 - MONITOR_GetDepth(&MONITOR_PrimaryMonitor));
	          if( max > 256 ) 
	          {
	  	      step = max/256;
		      max = 256;
	          }
	     }

	     /* reinit color (XAllocColor() may change it)
	      * and map to the best shared colorcell */

             color.red   = COLOR_sysPalTemplate[i].peRed * 65535 / 255;
             color.green = COLOR_sysPalTemplate[i].peGreen * 65535 / 255;
             color.blue  = COLOR_sysPalTemplate[i].peBlue * 65535 / 255;

	     best.pixel = best.red = best.green = best.blue = 0;
	     for( c.pixel = 0, diff = 0x7fffffff; c.pixel < max; c.pixel += step )
	     {
		TSXQueryColor(display, X11DRV_PALETTE_PaletteXColormap, &c);
		r = (c.red - color.red)>>8; 
		g = (c.green - color.green)>>8; 
		b = (c.blue - color.blue)>>8;
		r = r*r + g*g + b*b;
		if( r < diff ) { best = c; diff = r; }
	     }

	     if( TSXAllocColor(display, X11DRV_PALETTE_PaletteXColormap, &best) )
		 color.pixel = best.pixel;
	     else color.pixel = (i < NB_RESERVED_COLORS/2)? bp : wp;
        }

        sysPixel[i] = color.pixel;

        TRACE("syscolor(%lx) -> pixel %i\n",
		      *(COLORREF*)(COLOR_sysPalTemplate+i), (int)color.pixel);

        /* Set EGA mapping if color in the first or last eight */

        if (i < 8)
            X11DRV_PALETTE_mapEGAPixel[i] = color.pixel;
        else if (i >= NB_RESERVED_COLORS - 8 )
            X11DRV_PALETTE_mapEGAPixel[i - (NB_RESERVED_COLORS-16)] = color.pixel;
     }

   /* now allocate changeable set */

   if( !(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED) )  
     {
	int c_min = 0, c_max = X11DRV_DevCaps.sizePalette, c_val;

	TRACE("Dynamic colormap... \n");

	/* comment this out if you want to debug palette init */

	TSXGrabServer(display);

	/* let's become the first client that actually follows 
	 * X guidelines and does binary search...
	 */

	pixDynMapping = (unsigned long*)xmalloc(sizeof(long)*X11DRV_DevCaps.sizePalette);
        while( c_max - c_min > 0 )
          {
             c_val = (c_max + c_min)/2 + (c_max + c_min)%2;

             if( !TSXAllocColorCells(display, X11DRV_PALETTE_PaletteXColormap, False,
                                   plane_masks, 0, pixDynMapping, c_val) )
                 c_max = c_val - 1;
             else
               {
                 TSXFreeColors(display, X11DRV_PALETTE_PaletteXColormap, pixDynMapping, c_val, 0);
                 c_min = c_val;
               }
          }

	if( c_min > COLOR_max - NB_RESERVED_COLORS) 
	    c_min = COLOR_max - NB_RESERVED_COLORS;

	c_min = (c_min/2) + (c_min/2);		/* need even set for split palette */

	if( c_min > 0 )
	  if( !TSXAllocColorCells(display, X11DRV_PALETTE_PaletteXColormap, False,
                                plane_masks, 0, pixDynMapping, c_min) )
	    {
	      WARN("Inexplicable failure during colorcell allocation.\n");
	      c_min = 0;
	    }

        X11DRV_DevCaps.sizePalette = c_min + NB_RESERVED_COLORS;

	TSXUngrabServer(display);

	TRACE("adjusted size %i colorcells\n", X11DRV_DevCaps.sizePalette);
     }
   else if( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL ) 
	{
          /* virtual colorspace - ToPhysical takes care of 
           * color translations but we have to allocate full palette 
	   * to maintain compatibility
	   */
	  X11DRV_DevCaps.sizePalette = 256;
	  TRACE("Virtual colorspace - screendepth %i\n", MONITOR_GetDepth(&MONITOR_PrimaryMonitor));
	}
   else X11DRV_DevCaps.sizePalette = NB_RESERVED_COLORS;	/* system palette only - however we can alloc a bunch
			                 * of colors and map to them */

   TRACE("Shared system palette uses %i colors.\n", X11DRV_DevCaps.sizePalette);

   /* set gap to account for pixel shortage. It has to be right in the center
    * of the system palette because otherwise raster ops get screwed. */

   if( X11DRV_DevCaps.sizePalette >= 256 )
     { COLOR_gapStart = 256; COLOR_gapEnd = -1; }
   else
     { COLOR_gapStart = X11DRV_DevCaps.sizePalette/2; COLOR_gapEnd = 255 - X11DRV_DevCaps.sizePalette/2; }

   X11DRV_PALETTE_firstFree = ( X11DRV_DevCaps.sizePalette > NB_RESERVED_COLORS && 
		      (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL || !(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED)) ) 
		     ? NB_RESERVED_COLORS/2 : -1;

   COLOR_sysPal = (PALETTEENTRY*)xmalloc(sizeof(PALETTEENTRY)*256);

   /* setup system palette entry <-> pixel mappings and fill in 20 fixed entries */

   if( MONITOR_GetDepth(&MONITOR_PrimaryMonitor) <= 8 )
     {
       X11DRV_PALETTE_XPixelToPalette = (int*)xmalloc(sizeof(int)*256);
       memset( X11DRV_PALETTE_XPixelToPalette, 0, 256*sizeof(int) );
     }

   /* for hicolor visuals PaletteToPixel mapping is used to skip
    * RGB->pixel calculation in X11DRV_PALETTE_ToPhysical(). 
    */

   X11DRV_PALETTE_PaletteToXPixel = (int*)xmalloc(sizeof(int)*256);

   for( i = j = 0; i < 256; i++ )
   {
      if( i >= COLOR_gapStart && i <= COLOR_gapEnd ) 
      {
         X11DRV_PALETTE_PaletteToXPixel[i] = 0;
         COLOR_sysPal[i].peFlags = 0;	/* mark as unused */
         continue;
      }

      if( i < NB_RESERVED_COLORS/2 )
      {
        X11DRV_PALETTE_PaletteToXPixel[i] = sysPixel[i];
        COLOR_sysPal[i] = COLOR_sysPalTemplate[i];
      }
      else if( i >= 256 - NB_RESERVED_COLORS/2 )
      {
        X11DRV_PALETTE_PaletteToXPixel[i] = sysPixel[(i + NB_RESERVED_COLORS) - 256]; 
        COLOR_sysPal[i] = COLOR_sysPalTemplate[(i + NB_RESERVED_COLORS) - 256];
      }
      else if( pixDynMapping )
             X11DRV_PALETTE_PaletteToXPixel[i] = pixDynMapping[j++];
           else
             X11DRV_PALETTE_PaletteToXPixel[i] = i;

      TRACE("index %i -> pixel %i\n", i, X11DRV_PALETTE_PaletteToXPixel[i]);

      if( X11DRV_PALETTE_XPixelToPalette )
          X11DRV_PALETTE_XPixelToPalette[X11DRV_PALETTE_PaletteToXPixel[i]] = i;
   }

   if( pixDynMapping ) free(pixDynMapping);

   return TRUE;
}

/***********************************************************************
 *      Colormap Initialization
 */
static void X11DRV_PALETTE_FillDefaultColors(void)
{
 /* initialize unused entries to what Windows uses as a color 
  * cube - based on Greg Kreider's code. 
  */

  int i = 0, idx = 0;
  int red, no_r, inc_r;
  int green, no_g, inc_g; 
  int blue, no_b, inc_b;
  
  if (X11DRV_DevCaps.sizePalette <= NB_RESERVED_COLORS)
  	return;
  while (i*i*i < (X11DRV_DevCaps.sizePalette - NB_RESERVED_COLORS)) i++;
  no_r = no_g = no_b = --i;
  if ((no_r * (no_g+1) * no_b) < (X11DRV_DevCaps.sizePalette - NB_RESERVED_COLORS)) no_g++;
  if ((no_r * no_g * (no_b+1)) < (X11DRV_DevCaps.sizePalette - NB_RESERVED_COLORS)) no_b++;
  inc_r = (255 - NB_COLORCUBE_START_INDEX)/no_r;
  inc_g = (255 - NB_COLORCUBE_START_INDEX)/no_g;
  inc_b = (255 - NB_COLORCUBE_START_INDEX)/no_b;

  idx = X11DRV_PALETTE_firstFree;

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

	 if( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL )
	 {
            if (X11DRV_PALETTE_Redmax != 255) no_r = (red * X11DRV_PALETTE_Redmax) / 255;
            if (X11DRV_PALETTE_Greenmax != 255) no_g = (green * X11DRV_PALETTE_Greenmax) / 255;
            if (X11DRV_PALETTE_Bluemax != 255) no_b = (blue * X11DRV_PALETTE_Bluemax) / 255;

            X11DRV_PALETTE_PaletteToXPixel[idx] = (no_r << X11DRV_PALETTE_Redshift) | (no_g << X11DRV_PALETTE_Greenshift) | (no_b << X11DRV_PALETTE_Blueshift);
	 }
	 else if( !(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED) )
	 {
	   XColor color;
	   color.pixel = (X11DRV_PALETTE_PaletteToXPixel)? X11DRV_PALETTE_PaletteToXPixel[idx] : idx;
	   color.red = COLOR_sysPal[idx].peRed << 8;
	   color.green = COLOR_sysPal[idx].peGreen << 8;
	   color.blue =  COLOR_sysPal[idx].peBlue << 8;
	   color.flags = DoRed | DoGreen | DoBlue;
	   TSXStoreColor(display, X11DRV_PALETTE_PaletteXColormap, &color);
	 }
	 idx = X11DRV_PALETTE_freeList[idx];
      }

  /* try to fill some entries in the "gap" with
   * what's already in the colormap - they will be
   * mappable to but not changeable. */

  if( COLOR_gapStart < COLOR_gapEnd && X11DRV_PALETTE_XPixelToPalette )
  {
    XColor	xc;
    int		r, g, b, max;

    max = COLOR_max - (256 - (COLOR_gapEnd - COLOR_gapStart));
    for ( i = 0, idx = COLOR_gapStart; i < 256 && idx <= COLOR_gapEnd; i++ )
      if( X11DRV_PALETTE_XPixelToPalette[i] == 0 )
	{
	  xc.pixel = i;

	  TSXQueryColor(display, X11DRV_PALETTE_PaletteXColormap, &xc);
	  r = xc.red>>8; g = xc.green>>8; b = xc.blue>>8;

	  if( xc.pixel < 256 && X11DRV_PALETTE_CheckSysColor(RGB(r, g, b)) &&
	      TSXAllocColor(display, X11DRV_PALETTE_PaletteXColormap, &xc) )
	  {
	     X11DRV_PALETTE_XPixelToPalette[xc.pixel] = idx;
	     X11DRV_PALETTE_PaletteToXPixel[idx] = xc.pixel;
           *(COLORREF*)(COLOR_sysPal + idx) = RGB(r, g, b);
	     COLOR_sysPal[idx++].peFlags |= PC_SYS_USED;
	     if( --max <= 0 ) break;
	  }
	}
    COLOR_gapFilled = idx - COLOR_gapStart;    
  }
}


/***********************************************************************
 *           X11DRV_PALETTE_ToLogical
 *
 * Return RGB color for given X pixel.
 */
COLORREF X11DRV_PALETTE_ToLogical(int pixel)
{
    XColor color;

#if 0
    /* truecolor visual */

    if (MONITOR_GetDepth(&MONITOR_PrimaryMonitor) >= 24) return pixel;
#endif

    /* check for hicolor visuals first */

    if ( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED && !X11DRV_PALETTE_Graymax )
    {
         color.red = (pixel >> X11DRV_PALETTE_Redshift) & X11DRV_PALETTE_Redmax;
         color.green = (pixel >> X11DRV_PALETTE_Greenshift) & X11DRV_PALETTE_Greenmax;
         color.blue = (pixel >> X11DRV_PALETTE_Blueshift) & X11DRV_PALETTE_Bluemax;
         return RGB(MulDiv(color.red, 255, X11DRV_PALETTE_Redmax),
		    MulDiv(color.green, 255, X11DRV_PALETTE_Greenmax),
		    MulDiv(color.blue, 255, X11DRV_PALETTE_Bluemax));
    }

    /* check if we can bypass X */

    if ((MONITOR_GetDepth(&MONITOR_PrimaryMonitor) <= 8) && (pixel < 256) && 
       !(X11DRV_PALETTE_PaletteFlags & (X11DRV_PALETTE_VIRTUAL | X11DRV_PALETTE_FIXED)) )
         return  ( *(COLORREF*)(COLOR_sysPal + 
		   ((X11DRV_PALETTE_XPixelToPalette)?X11DRV_PALETTE_XPixelToPalette[pixel]:pixel)) ) & 0x00ffffff;

    color.pixel = pixel;
    TSXQueryColor(display, X11DRV_PALETTE_PaletteXColormap, &color);
    return RGB(color.red >> 8, color.green >> 8, color.blue >> 8);
}

/***********************************************************************
 *           X11DRV_PALETTE_ToPhysical
 *
 * Return the physical color closest to 'color'.
 */
int X11DRV_PALETTE_ToPhysical( DC *dc, COLORREF color )
{
    WORD 		 index = 0;
    HPALETTE16		 hPal = (dc)? dc->w.hPalette: STOCK_DEFAULT_PALETTE;
    unsigned char	 spec_type = color >> 24;
    PALETTEOBJ* 	 palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );

    /* palPtr can be NULL when DC is being destroyed */
    if( !palPtr ) return 0;

    if ( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED )
    {
        /* there is no colormap limitation; we are going to have to compute
         * the pixel value from the visual information stored earlier 
	 */

	unsigned 	long red, green, blue;
	unsigned 	idx = 0;

	switch(spec_type)
        {
          case 1: /* PALETTEINDEX */

            if( (idx = color & 0xffff) >= palPtr->logpalette.palNumEntries)
            {
                WARN("RGB(%lx) : idx %d is out of bounds, assuming black\n", color, idx);
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

	if (X11DRV_PALETTE_Graymax)
        {
	    /* grayscale only; return scaled value */
	    GDI_HEAP_UNLOCK( hPal );
            return ( (red * 30 + green * 69 + blue * 11) * X11DRV_PALETTE_Graymax) / 25500;
	}
	else
        {
	    /* scale each individually and construct the TrueColor pixel value */
	    if (X11DRV_PALETTE_Redmax != 255)
		red = MulDiv(red, X11DRV_PALETTE_Redmax, 255);
	    if (X11DRV_PALETTE_Greenmax != 255)
		green = MulDiv(green, X11DRV_PALETTE_Greenmax, 255);
	    if (X11DRV_PALETTE_Bluemax != 255)
		blue = MulDiv(blue, X11DRV_PALETTE_Bluemax, 255);

	    GDI_HEAP_UNLOCK( hPal );
	    return (red << X11DRV_PALETTE_Redshift) | (green << X11DRV_PALETTE_Greenshift) | (blue << X11DRV_PALETTE_Blueshift);
        }
    }
    else 
    {

	if( !palPtr->mapping ) 
            WARN("Palette %04x is not realized\n", dc->w.hPalette);

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
						  X11DRV_PALETTE_PaletteToXPixel, color, FALSE);

		/* TRACE(palette,"RGB(%lx) -> pixel %i\n", color, index);
		 */
	    	break;
       	    case 1:  /* PALETTEINDEX */
		index = color & 0xffff;

	        if( index >= palPtr->logpalette.palNumEntries )
		    WARN("RGB(%lx) : index %i is out of bounds\n", color, index); 
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
 *           X11DRV_PALETTE_LookupSystemXPixel
 */
static int X11DRV_PALETTE_LookupSystemXPixel(COLORREF col)
{
 int            i, best = 0, diff = 0x7fffffff;
 int            size = X11DRV_DevCaps.sizePalette;
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
 
 return (X11DRV_PALETTE_PaletteToXPixel)? X11DRV_PALETTE_PaletteToXPixel[best] : best;
}

/***********************************************************************
 *           X11DRV_PALETTE_FormatSystemPalette
 */
static void X11DRV_PALETTE_FormatSystemPalette(void)
{
 /* Build free list so we'd have an easy way to find
  * out if there are any available colorcells. 
  */

  int i, j = X11DRV_PALETTE_firstFree = NB_RESERVED_COLORS/2;

  COLOR_sysPal[j].peFlags = 0;
  for( i = NB_RESERVED_COLORS/2 + 1 ; i < 256 - NB_RESERVED_COLORS/2 ; i++ )
    if( i < COLOR_gapStart || i > COLOR_gapEnd )
      {
	COLOR_sysPal[i].peFlags = 0;  /* unused tag */
	X11DRV_PALETTE_freeList[j] = i;	  /* next */
        j = i;
      }
  X11DRV_PALETTE_freeList[j] = 0;
}

/***********************************************************************
 *           X11DRV_PALETTE_CheckSysColor
 */
static BOOL X11DRV_PALETTE_CheckSysColor(COLORREF c)
{
  int i;
  for( i = 0; i < NB_RESERVED_COLORS; i++ )
       if( c == (*(COLORREF*)(COLOR_sysPalTemplate + i) & 0x00ffffff) )
	   return 0;
  return 1;
}

/***********************************************************************
 *           X11DRV_PALETTE_SetMapping
 *
 * Set the color-mapping table for selected palette. 
 * Return number of entries which mapping has changed.
 */
int X11DRV_PALETTE_SetMapping( PALETTEOBJ* palPtr, UINT uStart, UINT uNum, BOOL mapOnly )
{
    char flag;
    int  prevMapping = (palPtr->mapping) ? 1 : 0;
    int  index, iRemapped = 0;

    /* reset dynamic system palette entries */

    if( !mapOnly && X11DRV_PALETTE_firstFree != -1)
         X11DRV_PALETTE_FormatSystemPalette();

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
                WARN("PC_EXPLICIT: idx %d out of system palette, assuming black.\n", index); 
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
                if( X11DRV_PALETTE_firstFree > 0 && !(X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_FIXED) )
                {
                    XColor color;
                    index = X11DRV_PALETTE_firstFree;  /* ought to be available */
                    X11DRV_PALETTE_firstFree = X11DRV_PALETTE_freeList[index];

                    color.pixel = (X11DRV_PALETTE_PaletteToXPixel) ? X11DRV_PALETTE_PaletteToXPixel[index] : index;
                    color.red = palPtr->logpalette.palPalEntry[uStart].peRed << 8;
                    color.green = palPtr->logpalette.palPalEntry[uStart].peGreen << 8;
                    color.blue = palPtr->logpalette.palPalEntry[uStart].peBlue << 8;
                    color.flags = DoRed | DoGreen | DoBlue;
                    TSXStoreColor(display, X11DRV_PALETTE_PaletteXColormap, &color);

                    COLOR_sysPal[index] = palPtr->logpalette.palPalEntry[uStart];
                    COLOR_sysPal[index].peFlags = flag;
		    X11DRV_PALETTE_freeList[index] = 0;

                    if( X11DRV_PALETTE_PaletteToXPixel ) index = X11DRV_PALETTE_PaletteToXPixel[index];
                    break;
                }
                else if ( X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_VIRTUAL ) 
                {
                    index = X11DRV_PALETTE_ToPhysical( NULL, 0x00ffffff &
                             *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart));
                    break;     
                }

                /* we have to map to existing entry in the system palette */

                index = COLOR_PaletteLookupPixel(COLOR_sysPal, 256, NULL, 
                       *(COLORREF*)(palPtr->logpalette.palPalEntry + uStart), TRUE);
            }
	    palPtr->logpalette.palPalEntry[uStart].peFlags |= PC_SYS_USED;

            if( X11DRV_PALETTE_PaletteToXPixel ) index = X11DRV_PALETTE_PaletteToXPixel[index];
            break;
        }

        if( !prevMapping || palPtr->mapping[uStart] != index ) iRemapped++;
        palPtr->mapping[uStart] = index;

        TRACE("entry %i (%lx) -> pixel %i\n", uStart, 
				*(COLORREF*)(palPtr->logpalette.palPalEntry + uStart), index);
	
    }
    return iRemapped;
}

/***********************************************************************
 *           X11DRV_PALETTE_UpdateMapping
 *
 * Update the color-mapping table for selected palette. 
 * Return number of entries which mapping has changed.
 */
int X11DRV_PALETTE_UpdateMapping(PALETTEOBJ *palPtr)
{
  int i, index, realized = 0;    
  
  if (!X11DRV_DevCaps.sizePalette)
    return 0;

  for( i = 0; i < 20; i++ )
    {
      index = X11DRV_PALETTE_LookupSystemXPixel(*(COLORREF*)(palPtr->logpalette.palPalEntry + i));
      
      /* mapping is allocated in COLOR_InitPalette() */
      
      if( index != palPtr->mapping[i] ) { palPtr->mapping[i]=index; realized++; }
    }
  
    return realized;
}

/**************************************************************************
 *		X11DRV_PALETTE_IsDark
 */
BOOL X11DRV_PALETTE_IsDark(int pixel)
{
  COLORREF col = X11DRV_PALETTE_ToLogical(pixel);
  return (GetRValue(col) + GetGValue(col) + GetBValue(col)) <= 0x180;
}

#endif /* !defined(X_DISPLAY_MISSING) */
