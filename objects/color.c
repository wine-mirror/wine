/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <X11/Xlib.h>

#include "windows.h"
#include "options.h"

extern Display * display;
extern Screen * screen;


Colormap COLOR_WinColormap = 0;


  /* System colors */

static const char * SysColors[] = 
{
      /* Low pixel values (0..7) */

    "black", "red4", "green4", "yellow4",
    "blue4", "magenta4", "cyan4", "gray50",

      /* High pixel values (max-7..max) */

    "gray75", "red", "green", "yellow",
    "blue", "magenta", "cyan", "white"
};

#define NB_SYS_COLORS   (sizeof(SysColors) / sizeof(SysColors[0]))


/***********************************************************************
 *           COLOR_FillDefaultMap
 *
 * Try to allocate colors from default screen map (used when we
 * don't want to or can't use a private map).
 */
static int COLOR_FillDefaultMap()
{
    XColor color;
    int i, total = 0;

    for (i = 0; i < NB_SYS_COLORS; i++)
    {
	if (XParseColor( display, DefaultColormapOfScreen( screen ),
			 SysColors[i], &color ))
	{
	    if (XAllocColor( display, DefaultColormapOfScreen( screen ), 
			     &color ))
		total++;
	}
    }
    return total;
}


/***********************************************************************
 *           COLOR_BuildMap
 *
 * Fill the private colormap.
 */
static BOOL COLOR_BuildMap( Colormap map, int depth, int size )
{
    XColor color;
    int i;

      /* Fill the whole map with a range of colors */

    if ((1 << depth) > NB_SYS_COLORS)
    {
	int red_incr, green_incr, blue_incr;
	int r, g, b;
	
	blue_incr  = 0x10000 >> (depth / 3);
	red_incr   = 0x10000 >> ((depth + 1) / 3);
	green_incr = 0x10000 >> ((depth + 2) / 3);

	for (i = 0, r = red_incr - 1; r < 0x10000; r += red_incr)
	    for (g = green_incr - 1; g < 0x10000; g += green_incr)
		for (b = blue_incr - 1; b < 0x10000; b += blue_incr)
		{
		    if (i >= size) break;
		    color.pixel = i++;
		    color.red   = r;
		    color.green = g;
		    color.blue  = b;
		    XStoreColor( display, map, &color );
		}
    }
    
      /* Store the system palette colors */

    for (i = 0; i < NB_SYS_COLORS; i++)
    {
	if (!XParseColor( display, map, SysColors[i], &color ))
	    color.red = color.green = color.blue = color.flags = 0;
	if (i < NB_SYS_COLORS/2) color.pixel = i;
	else color.pixel = (1 << depth) - NB_SYS_COLORS + i;
	if (color.pixel < size) XStoreColor( display, map, &color );
    }
    return TRUE;
}


/***********************************************************************
 *           COLOR_Init
 */
BOOL COLOR_Init()
{
    Visual * visual = DefaultVisual( display, DefaultScreen(display) );
    
    switch(visual->class)
    {
    case GrayScale:
    case PseudoColor:
    case DirectColor:
	if (Options.usePrivateMap)
	{
	    COLOR_WinColormap = XCreateColormap( display,
						 DefaultRootWindow(display),
						 visual, AllocAll );
	    if (COLOR_WinColormap)
	    {
		COLOR_BuildMap(COLOR_WinColormap,
			       DefaultDepth(display, DefaultScreen(display)),
			       visual->map_entries );
		break;
	    }
	}
	/* Fall through */
    case StaticGray:
    case StaticColor:
    case TrueColor:
	COLOR_FillDefaultMap();
	COLOR_WinColormap = DefaultColormapOfScreen( screen );
	break;	
    }
    return TRUE;
}
