/*
 * X11 windows driver
 *
 * Copyright 1993, 1994 Alexandre Julliard
 * Copyright 1998 Patrik Stridvall
 */

#include <string.h>
#include <X11/Xatom.h>
#include "ts_xlib.h"
#include "color.h"
#include "cursoricon.h"
#include "dce.h"
#include "options.h"
#include "message.h"
#include "win.h"
#include "windows.h"
#include "x11drv.h"

static BOOL32 X11DRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCT32A *cs, BOOL32 bUnicode);
static WND *X11DRV_WND_SetParent(WND *wndPtr, WND *pWndParent);

WND_DRIVER X11DRV_WND_Driver =
{
    X11DRV_WND_CreateWindow,
    X11DRV_WND_SetParent
};

/**********************************************************************
 *		X11DRV_WND_CreateWindow	[Internal]
 */
static BOOL32 X11DRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCT32A *cs, BOOL32 bUnicode)
{
  /* Create the X window (only for top-level windows, and then only */
  /* when there's no desktop window) */
  
  if (!(cs->style & WS_CHILD) && (rootWindow == DefaultRootWindow(display)))
    {
      XSetWindowAttributes win_attr;
      
      if (Options.managed && ((cs->style & (WS_DLGFRAME | WS_THICKFRAME)) ||
			      (cs->dwExStyle & WS_EX_DLGMODALFRAME)))
        {
	  win_attr.event_mask = ExposureMask | KeyPressMask |
	    KeyReleaseMask | PointerMotionMask |
	    ButtonPressMask | ButtonReleaseMask |
	    FocusChangeMask | StructureNotifyMask;
	  win_attr.override_redirect = FALSE;
	  wndPtr->flags |= WIN_MANAGED;
	}
      else
        {
	  win_attr.event_mask = ExposureMask | KeyPressMask |
	    KeyReleaseMask | PointerMotionMask |
	    ButtonPressMask | ButtonReleaseMask |
	    FocusChangeMask;
	  win_attr.override_redirect = TRUE;
	}
      win_attr.colormap      = COLOR_GetColormap();
      win_attr.backing_store = Options.backingstore ? WhenMapped : NotUseful;
      win_attr.save_under    = ((classPtr->style & CS_SAVEBITS) != 0);
      win_attr.cursor        = CURSORICON_XCursor;
      wndPtr->window = TSXCreateWindow( display, rootWindow, cs->x, cs->y,
                                        cs->cx, cs->cy, 0, CopyFromParent,
                                        InputOutput, CopyFromParent,
                                        CWEventMask | CWOverrideRedirect |
                                        CWColormap | CWCursor | CWSaveUnder |
                                        CWBackingStore, &win_attr );

      if ((wndPtr->flags & WIN_MANAGED) &&
	  (cs->dwExStyle & WS_EX_DLGMODALFRAME))
        {
	  XSizeHints* size_hints = TSXAllocSizeHints();
	  
	  if (size_hints)
            {
	      size_hints->min_width = size_hints->max_width = cs->cx;
	      size_hints->min_height = size_hints->max_height = cs->cy;
                size_hints->flags = (PSize | PMinSize | PMaxSize);
                TSXSetWMSizeHints( display, wndPtr->window, size_hints,
				   XA_WM_NORMAL_HINTS );
                TSXFree(size_hints);
            }
        }
      
      if (cs->hwndParent)  /* Get window owner */
	{
	  Window win = WIN_GetXWindow( cs->hwndParent );
	  if (win) TSXSetTransientForHint( display, wndPtr->window, win );
	}
      EVENT_RegisterWindow( wndPtr );
    }
  return TRUE;
}

/*****************************************************************
 *		X11DRV_WND_SetParent	[Internal]
 */
static WND *X11DRV_WND_SetParent(WND *wndPtr, WND *pWndParent)
{
    if( wndPtr && pWndParent && (wndPtr != WIN_GetDesktop()) )
    {
	WND* pWndPrev = wndPtr->parent;

	if( pWndParent != pWndPrev )
	{
	    BOOL32 bFixupDCE = IsWindowVisible32(wndPtr->hwndSelf);

	    if ( wndPtr->window )
	    {
		/* Toplevel window needs to be reparented.  Used by Tk 8.0 */

		TSXDestroyWindow( display, wndPtr->window );
		wndPtr->window = None;
	    }
	    else if( bFixupDCE )
		DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );

	    WIN_UnlinkWindow(wndPtr->hwndSelf);
	    wndPtr->parent = pWndParent;

	    /* FIXME: Create an X counterpart for reparented top-level windows
	     * when not in the desktop mode. */
     
	    if ( pWndParent != WIN_GetDesktop() ) wndPtr->dwStyle |= WS_CHILD;
	    WIN_LinkWindow(wndPtr->hwndSelf, HWND_BOTTOM);

	    if( bFixupDCE )
	    {
	        DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );
		UpdateWindow32(wndPtr->hwndSelf);
	    }
	}
	return pWndPrev;
    } /* failure */
    return 0;
}
