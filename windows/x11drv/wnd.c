/*
 * X11 windows driver
 *
 * Copyright 1993, 1994, 1995, 1996 Alexandre Julliard
 *                             1993 David Metcalfe
 *                       1995, 1996 Alex Korobka
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include "ts_xlib.h"
#include "ts_xutil.h"

#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "display.h"
#include "dce.h"
#include "options.h"
#include "message.h"
#include "win.h"
#include "windows.h"
#include "x11drv.h"

/**********************************************************************/

extern Cursor DISPLAY_XCursor;  /* Current X cursor */

/**********************************************************************/

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

Atom wmProtocols = None;
Atom wmDeleteWindow = None;
Atom dndProtocol = None;
Atom dndSelection = None;

/***********************************************************************
 *           X11DRV_WND_GetXWindow
 *
 * Return the X window associated to a window.
 */
Window X11DRV_WND_GetXWindow(HWND32 hwnd)
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    while (wndPtr && !wndPtr->window) wndPtr = wndPtr->parent;
    return wndPtr ? wndPtr->window : 0;
}

/***********************************************************************
 *		X11DRV_WND_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
static void X11DRV_WND_RegisterWindow(WND *pWnd)
{
  TSXSetWMProtocols( display, pWnd->window, &wmDeleteWindow, 1 );
  
  if (!winContext) winContext = TSXUniqueContext();
  TSXSaveContext( display, pWnd->window, winContext, (char *)pWnd );
}

/**********************************************************************
 *		X11DRV_WND_CreateDesktopWindow
 */
BOOL32 X11DRV_WND_CreateDesktopWindow(WND *wndPtr, CLASS *classPtr, BOOL32 bUnicode)
{
    if (wmProtocols == None)
        wmProtocols = TSXInternAtom( display, "WM_PROTOCOLS", True );
    if (wmDeleteWindow == None)
        wmDeleteWindow = TSXInternAtom( display, "WM_DELETE_WINDOW", True );
    if( dndProtocol == None )
	dndProtocol = TSXInternAtom( display, "DndProtocol" , False );
    if( dndSelection == None )
	dndSelection = TSXInternAtom( display, "DndSelection" , False );

    wndPtr->window = rootWindow;
    wndPtr->expose_event = NULL;
    X11DRV_WND_RegisterWindow( wndPtr );

    return TRUE;
}

/**********************************************************************
 *		X11DRV_WND_CreateWindow
 */
BOOL32 X11DRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCT32A *cs, BOOL32 bUnicode)
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
      win_attr.cursor        = DISPLAY_XCursor;
      wndPtr->window = TSXCreateWindow( display, rootWindow, cs->x, cs->y,
                                        cs->cx, cs->cy, 0, CopyFromParent,
                                        InputOutput, CopyFromParent,
                                        CWEventMask | CWOverrideRedirect |
                                        CWColormap | CWCursor | CWSaveUnder |
                                        CWBackingStore, &win_attr );
     
      if(!wndPtr->window)
	return FALSE;

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
	  Window win = X11DRV_WND_GetXWindow( cs->hwndParent );
	  if (win) TSXSetTransientForHint( display, wndPtr->window, win );
	}
      X11DRV_WND_RegisterWindow( wndPtr );
    }
  return TRUE;
}

/***********************************************************************
 *		X11DRV_WND_DestroyWindow
 */
BOOL32 X11DRV_WND_DestroyWindow(WND *pWnd)
{
   if (pWnd->expose_event) 
     {
       free( pWnd->expose_event );
       pWnd->expose_event = NULL;
     }

   if (pWnd->window)
     {
       XEvent xe;
       TSXDeleteContext( display, pWnd->window, winContext );
       TSXDestroyWindow( display, pWnd->window );
       while( TSXCheckWindowEvent(display, pWnd->window, NoEventMask, &xe) );
       pWnd->window = None;
     }

   return TRUE;
}

/*****************************************************************
 *		X11DRV_WND_SetParent
 */
WND *X11DRV_WND_SetParent(WND *wndPtr, WND *pWndParent)
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

/***********************************************************************
 *		X11DRV_WND_ForceWindowRaise
 *
 * Raise a window on top of the X stacking order, while preserving 
 * the correct Windows Z order.
 */
void X11DRV_WND_ForceWindowRaise(WND *pWnd)
{
  XWindowChanges winChanges;
  WND *wndPrev;
  
  if( !pWnd || !pWnd->window || (pWnd->flags & WIN_MANAGED) )
    return;
  
  /* Raise all windows up to pWnd according to their Z order.
   * (it would be easier with sibling-related Below but it doesn't
   * work very well with SGI mwm for instance)
   */
  winChanges.stack_mode = Above;
  while (pWnd)
    {
      if (pWnd->window) TSXReconfigureWMWindow( display, pWnd->window, 0,
                                                CWStackMode, &winChanges );
      wndPrev = WIN_GetDesktop()->child;
      if (wndPrev == pWnd) break;
      while (wndPrev && (wndPrev->next != pWnd)) wndPrev = wndPrev->next;
      pWnd = wndPrev;
    }
}

/***********************************************************************
 *		X11DRV_WND_FindDesktopXWindow	[Internal]
 *
 * Find the actual X window which needs be restacked.
 * Used by X11DRV_SetWindowPos().
 */
static Window X11DRV_WND_FindDesktopXWindow( WND *wndPtr )
{
  if (!(wndPtr->flags & WIN_MANAGED))
    return wndPtr->window;
  else
    {
      Window window, root, parent, *children;
      int nchildren;
      window = wndPtr->window;
      for (;;)
        {
	  TSXQueryTree( display, window, &root, &parent,
                        &children, &nchildren );
	  TSXFree( children );
	  if (parent == root)
	    return window;
	  window = parent;
        }
    }
}

/***********************************************************************
 *           WINPOS_SetXWindowPos
 *
 * SetWindowPos() for an X window. Used by the real SetWindowPos().
 */
void X11DRV_WND_SetWindowPos(WND *wndPtr, const WINDOWPOS32 *winpos, BOOL32 bSMC_SETXPOS)
{
  XWindowChanges winChanges;
  int changeMask = 0;
  WND *winposPtr = WIN_FindWndPtr( winpos->hwnd );
  
  if (!(winpos->flags & SWP_SHOWWINDOW) && (winpos->flags & SWP_HIDEWINDOW))
    {
      if(wndPtr && wndPtr->window) TSXUnmapWindow( display, wndPtr->window );
    }

  if(bSMC_SETXPOS)
    {
      if ( !(winpos->flags & SWP_NOSIZE))
	{
	  winChanges.width     = winpos->cx;
	  winChanges.height    = winpos->cy;
	  changeMask |= CWWidth | CWHeight;
	  
	  /* Tweak dialog window size hints */
	  
	  if ((winposPtr->flags & WIN_MANAGED) &&
	      (winposPtr->dwExStyle & WS_EX_DLGMODALFRAME))
	    {
	      XSizeHints *size_hints = TSXAllocSizeHints();
	      
	      if (size_hints)
		{
		  long supplied_return;
		  
		  TSXGetWMSizeHints( display, winposPtr->window, size_hints,
				     &supplied_return, XA_WM_NORMAL_HINTS);
		  size_hints->min_width = size_hints->max_width = winpos->cx;
		  size_hints->min_height = size_hints->max_height = winpos->cy;
		  TSXSetWMSizeHints( display, winposPtr->window, size_hints,
				     XA_WM_NORMAL_HINTS );
		  TSXFree(size_hints);
		}
	    }
	}
      if (!(winpos->flags & SWP_NOMOVE))
	{
	  winChanges.x = winpos->x;
	  winChanges.y = winpos->y;
	  changeMask |= CWX | CWY;
	}
      if (!(winpos->flags & SWP_NOZORDER))
	{
	  winChanges.stack_mode = Below;
	  changeMask |= CWStackMode;
	  
	  if (winpos->hwndInsertAfter == HWND_TOP) winChanges.stack_mode = Above;
	  else if (winpos->hwndInsertAfter != HWND_BOTTOM)
	    {
	      WND*   insertPtr = WIN_FindWndPtr( winpos->hwndInsertAfter );
	      Window stack[2];
	      
	      stack[0] = X11DRV_WND_FindDesktopXWindow( insertPtr );
	      stack[1] = X11DRV_WND_FindDesktopXWindow( winposPtr );
	      
	      /* for stupid window managers (i.e. all of them) */
	      
	      TSXRestackWindows(display, stack, 2); 
	      changeMask &= ~CWStackMode;
	    }
	}
      if (changeMask)
	{
	  TSXReconfigureWMWindow( display, winposPtr->window, 0, changeMask, &winChanges );
	}
    }

  if ( winpos->flags & SWP_SHOWWINDOW )
    {
      if(wndPtr && wndPtr->window) TSXMapWindow( display, wndPtr->window );
    }
}

/*****************************************************************
 *		X11DRV_WND_SetText
 */
void X11DRV_WND_SetText(WND *wndPtr, LPCSTR text)
{   
  if (!wndPtr->window)
    return;

  TSXStoreName( display, wndPtr->window, text );
  TSXSetIconName( display, wndPtr->window, text );
}

/*****************************************************************
 *		X11DRV_WND_SetFocus
 *
 * Set the X focus.
 * Explicit colormap management seems to work only with OLVWM.
 */
void X11DRV_WND_SetFocus(WND *wndPtr)
{
  HWND32 hwnd =  wndPtr->hwndSelf;
  XWindowAttributes win_attr;
  
  /* Only mess with the X focus if there's */
  /* no desktop window and no window manager. */
  if ((rootWindow != DefaultRootWindow(display)) || Options.managed) return;
  
  if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
      if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
	TSXUninstallColormap( display, COLOR_GetColormap() );
      return;
    }
  
  /* Set X focus and install colormap */
  
  if (!wndPtr->window) return;
  
  if (!TSXGetWindowAttributes( display, wndPtr->window, &win_attr ) ||
      (win_attr.map_state != IsViewable))
    return;  /* If window is not viewable, don't change anything */
  
  TSXSetInputFocus( display,wndPtr->window, RevertToParent, CurrentTime );
  if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
    TSXInstallColormap( display, COLOR_GetColormap() );
  
  EVENT_Synchronize();
}

/*****************************************************************
 *		X11DRV_WND_PreSizeMove
 */
void X11DRV_WND_PreSizeMove(WND *wndPtr)
{
  if (!(wndPtr->dwStyle & WS_CHILD) && (rootWindow == DefaultRootWindow(display)))
    TSXGrabServer( display );
}

/*****************************************************************
 *		 X11DRV_WND_PostSizeMove
 */
void X11DRV_WND_PostSizeMove(WND *wndPtr)
{
  if (!(wndPtr->dwStyle & WS_CHILD) && (rootWindow == DefaultRootWindow(display)))
    TSXUngrabServer( display );
}

#endif /* !defined(X_DISPLAY_MISSING) */
