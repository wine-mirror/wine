/*
 * Color functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <X11/Xlib.h>

#include "windows.h"

extern Display * XT_display;
extern Screen * XT_screen;


/* 
 * We try to use a private color map if possible, because Windows programs
 * assume that palette(0) == Black and palette(max-1) == White.
 */

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
	if (XParseColor( XT_display, DefaultColormapOfScreen( XT_screen ),
			 SysColors[i], &color ))
	{
	    if (XAllocColor( XT_display, DefaultColormapOfScreen( XT_screen ), 
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

    for (i = 0; i < NB_SYS_COLORS; i++)
    {
	if (!XParseColor( XT_display, map, SysColors[i], &color ))
	    color.red = color.green = color.blue = color.flags = 0;
	if (i < NB_SYS_COLORS/2) color.pixel = i;
	else color.pixel = (1 << depth) - NB_SYS_COLORS + i;
	if (color.pixel < size) XStoreColor( XT_display, map, &color );
    }
    return TRUE;
}


/***********************************************************************
 *           COLOR_Init
 */
BOOL COLOR_Init()
{
    Visual * visual = DefaultVisual( XT_display, DefaultScreen(XT_display) );
    
    switch(visual->class)
    {
      case GrayScale:
      case PseudoColor:
      case DirectColor:

#ifdef USE_PRIVATE_MAP
	COLOR_WinColormap = XCreateColormap( XT_display,
					     DefaultRootWindow(XT_display),
					     visual, AllocAll );
	if (COLOR_WinColormap)
	    COLOR_BuildMap(COLOR_WinColormap,
			   DefaultDepth(XT_display, DefaultScreen(XT_display)),
			   visual->map_entries );
	else COLOR_FillDefaultMap();
	break;
#endif  /* USE_PRIVATE_MAP */

      case StaticGray:
      case StaticColor:
      case TrueColor:
	COLOR_FillDefaultMap();
	COLOR_WinColormap = CopyFromParent;
	break;	
    }
    return TRUE;
}
