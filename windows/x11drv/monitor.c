/*
 * X11 monitor driver
 *
 * Copyright 1998 Patrik Stridvall
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <stdlib.h>
#include <X11/cursorfont.h>
#include "ts_xlib.h"
#include "ts_xutil.h"

#include "debug.h"
#include "heap.h"
#include "monitor.h"
#include "options.h"
#include "windef.h"
#include "x11drv.h"

/**********************************************************************/

extern Display *display;

/***********************************************************************
 *              X11DRV_MONITOR_GetXScreen
 *
 * Return the X screen associated to the MONITOR.
 */
Screen *X11DRV_MONITOR_GetXScreen(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pX11Monitor->screen;
}

/***********************************************************************
 *              X11DRV_MONITOR_GetXRootWindow
 *
 * Return the X screen associated to the MONITOR.
 */
Window X11DRV_MONITOR_GetXRootWindow(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pX11Monitor->rootWindow;
}

/***********************************************************************
 *		X11DRV_MONITOR_CreateDesktop
 * FIXME 
 *   The desktop should create created in X11DRV_DESKTOP_Initialize
 *   instead but it can't be yet because some code depends on that
 *   the desktop always exists.
 *
 */
static void X11DRV_MONITOR_CreateDesktop(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  int x, y, flags;
  unsigned int width = 640, height = 480;  /* Default size = 640x480 */
  char *name = "Wine desktop";
  XSizeHints *size_hints;
  XWMHints   *wm_hints;
  XClassHint *class_hints;
  XSetWindowAttributes win_attr;
  XTextProperty window_name;
  Atom XA_WM_DELETE_WINDOW;

  flags = TSXParseGeometry( Options.desktopGeometry, &x, &y, &width, &height );
  pX11Monitor->width  = width;
  pX11Monitor->height = height;

  /* Create window */
  
  win_attr.background_pixel = BlackPixel(display, 0);
  win_attr.event_mask = 
    ExposureMask | KeyPressMask | KeyReleaseMask |
    PointerMotionMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask;
  win_attr.cursor = TSXCreateFontCursor( display, XC_top_left_arrow );

  pX11Monitor->rootWindow =
    TSXCreateWindow( display, 
		     DefaultRootWindow(display),
		     x, y, width, height, 0,
		     CopyFromParent, InputOutput, CopyFromParent,
		     CWBackPixel | CWEventMask | CWCursor, &win_attr);
  
  /* Set window manager properties */

  size_hints  = TSXAllocSizeHints();
  wm_hints    = TSXAllocWMHints();
  class_hints = TSXAllocClassHint();
  if (!size_hints || !wm_hints || !class_hints)
    {
      MSG("Not enough memory for window manager hints.\n" );
      exit(1);
    }
  size_hints->min_width = size_hints->max_width = width;
  size_hints->min_height = size_hints->max_height = height;
  size_hints->flags = PMinSize | PMaxSize;
  if (flags & (XValue | YValue)) size_hints->flags |= USPosition;
  if (flags & (WidthValue | HeightValue)) size_hints->flags |= USSize;
  else size_hints->flags |= PSize;

  wm_hints->flags = InputHint | StateHint;
  wm_hints->input = True;
  wm_hints->initial_state = NormalState;
  class_hints->res_name = Options.argv[0]; /* FIXME: Options.argv0 insteed? */
  class_hints->res_class = "Wine";

  TSXStringListToTextProperty( &name, 1, &window_name );
  TSXSetWMProperties( display, pX11Monitor->rootWindow, &window_name, &window_name,
                      Options.argv, *Options.argc, size_hints, wm_hints, class_hints );
  XA_WM_DELETE_WINDOW = TSXInternAtom( display, "WM_DELETE_WINDOW", False );
  TSXSetWMProtocols( display, pX11Monitor->rootWindow, &XA_WM_DELETE_WINDOW, 1 );
  TSXFree( size_hints );
  TSXFree( wm_hints );
  TSXFree( class_hints );

  /* Map window */

  TSXMapWindow( display, pX11Monitor->rootWindow );
}

/***********************************************************************
 *              X11DRV_MONITOR_Initialize
 */
void X11DRV_MONITOR_Initialize(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor = (X11DRV_MONITOR_DATA *) 
    HeapAlloc(SystemHeap, 0, sizeof(X11DRV_MONITOR_DATA));

  int depth_count, i;
  int *depth_list;

  pMonitor->pDriverData = pX11Monitor;

  pX11Monitor->screen  = DefaultScreenOfDisplay( display );

  pX11Monitor->width   = WidthOfScreen( pX11Monitor->screen );
  pX11Monitor->height  = HeightOfScreen( pX11Monitor->screen );

  pX11Monitor->depth   = Options.screenDepth;
  if (pX11Monitor->depth)  /* -depth option specified */
    {
      depth_list = TSXListDepths(display, DefaultScreen(display), &depth_count);
      for (i = 0; i < depth_count; i++)
	if (depth_list[i] == pX11Monitor->depth) break;
      TSXFree( depth_list );
      if (i >= depth_count)
	{
	  MSG( "%s: Depth %d not supported on this screen.\n",
	       Options.programName, pX11Monitor->depth );
	  exit(1);
	}
    }
  else
    pX11Monitor->depth  = DefaultDepthOfScreen( pX11Monitor->screen );

  if (Options.synchronous) TSXSynchronize( display, True );

  if (Options.desktopGeometry)
    X11DRV_MONITOR_CreateDesktop(pMonitor);
  else 
    pX11Monitor->rootWindow =
      DefaultRootWindow( display );
}

/***********************************************************************
 *              X11DRV_MONITOR_Finalize
 */
void X11DRV_MONITOR_Finalize(MONITOR *pMonitor)
{
  HeapFree(SystemHeap, 0, pMonitor->pDriverData);
}

/***********************************************************************
 *              X11DRV_MONITOR_GetWidth
 *
 * Return the width of the monitor
 */
int X11DRV_MONITOR_GetWidth(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pX11Monitor->width;
}

/***********************************************************************
 *              X11DRV_MONITOR_GetHeight
 *
 * Return the height of the monitor
 */
int X11DRV_MONITOR_GetHeight(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pX11Monitor->height;
}

/***********************************************************************
 *              X11DRV_MONITOR_GetDepth
 *
 * Return the depth of the monitor
 */
int X11DRV_MONITOR_GetDepth(MONITOR *pMonitor)
{
  X11DRV_MONITOR_DATA *pX11Monitor =
    (X11DRV_MONITOR_DATA *) pMonitor->pDriverData;

  return pX11Monitor->depth;
}

#endif /* X_DISPLAY_MISSING */


