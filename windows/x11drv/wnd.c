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
#include "debug.h"
#include "display.h"
#include "dce.h"
#include "options.h"
#include "message.h"
#include "heap.h"
#include "win.h"
#include "windef.h"
#include "class.h"
#include "x11drv.h"

/**********************************************************************/

extern Cursor X11DRV_MOUSE_XCursor;  /* Current X cursor */

/**********************************************************************/

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

Atom wmProtocols = None;
Atom wmDeleteWindow = None;
Atom dndProtocol = None;
Atom dndSelection = None;

/***********************************************************************
 *		X11DRV_WND_GetXWindow
 *
 * Return the X window associated to a window.
 */
Window X11DRV_WND_GetXWindow(WND *wndPtr)
{
    return wndPtr && wndPtr->pDriverData ? 
      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window : 0;
}

/***********************************************************************
 *		X11DRV_WND_FindXWindow
 *
 * Return the the first X window associated to a window chain.
 */
Window X11DRV_WND_FindXWindow(WND *wndPtr)
{
    while (wndPtr && 
	   !((X11DRV_WND_DATA *) wndPtr->pDriverData)->window) 
      wndPtr = wndPtr->parent;
    return wndPtr ?
      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window : 0;
}

/***********************************************************************
 *              X11DRV_WND_GetXScreen
 *
 * Return the X screen associated to the window.
 */
Screen *X11DRV_WND_GetXScreen(WND *wndPtr)
{
  while(wndPtr->parent) wndPtr = wndPtr->parent;
  return X11DRV_DESKTOP_GetXScreen((struct tagDESKTOP *) wndPtr->wExtra);
}

/***********************************************************************
 *              X11DRV_WND_GetXRootWindow
 *
 * Return the X display associated to the window.
 */
Window X11DRV_WND_GetXRootWindow(WND *wndPtr)
{
  while(wndPtr->parent) wndPtr = wndPtr->parent;
  return X11DRV_DESKTOP_GetXRootWindow((struct tagDESKTOP *) wndPtr->wExtra);
}

/***********************************************************************
 *		X11DRV_WND_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
static void X11DRV_WND_RegisterWindow(WND *wndPtr)
{
  TSXSetWMProtocols( display, X11DRV_WND_GetXWindow(wndPtr), &wmDeleteWindow, 1 );
  
  if (!winContext) winContext = TSXUniqueContext();
  TSXSaveContext( display, X11DRV_WND_GetXWindow(wndPtr), winContext, (char *) wndPtr );
}

/**********************************************************************
 *		X11DRV_WND_Initialize
 */
void X11DRV_WND_Initialize(WND *wndPtr)
{
  X11DRV_WND_DATA *pWndDriverData = 
    (X11DRV_WND_DATA *) HeapAlloc(SystemHeap, 0, sizeof(X11DRV_WND_DATA));

  wndPtr->pDriverData = (void *) pWndDriverData;

  pWndDriverData->window = 0;
}

/**********************************************************************
 *		X11DRV_WND_Finalize
 */
void X11DRV_WND_Finalize(WND *wndPtr)
{
  X11DRV_WND_DATA *pWndDriverData =
    (X11DRV_WND_DATA *) wndPtr->pDriverData;

  if (!wndPtr->pDriverData) {
     ERR(win,"Trying to destroy window again. Not good.\n");
     return;
  }
  if(pWndDriverData->window)
    {
      ERR(win, 
	  "WND destroyed without destroying "
	  "the associated X Window (%ld)\n", 
	  pWndDriverData->window
      );
    }
  HeapFree(SystemHeap, 0, wndPtr->pDriverData);
  wndPtr->pDriverData = NULL;
}

/**********************************************************************
 *		X11DRV_WND_CreateDesktopWindow
 */
BOOL X11DRV_WND_CreateDesktopWindow(WND *wndPtr, CLASS *classPtr, BOOL bUnicode)
{
    if (wmProtocols == None)
        wmProtocols = TSXInternAtom( display, "WM_PROTOCOLS", True );
    if (wmDeleteWindow == None)
        wmDeleteWindow = TSXInternAtom( display, "WM_DELETE_WINDOW", True );
    if( dndProtocol == None )
	dndProtocol = TSXInternAtom( display, "DndProtocol" , False );
    if( dndSelection == None )
	dndSelection = TSXInternAtom( display, "DndSelection" , False );

    ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = 
      X11DRV_WND_GetXRootWindow( wndPtr );
    X11DRV_WND_RegisterWindow( wndPtr );

    return TRUE;
}

/**********************************************************************
 *		X11DRV_WND_CreateWindow
 */
BOOL X11DRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCTA *cs, BOOL bUnicode)
{
  /* Create the X window (only for top-level windows, and then only */
  /* when there's no desktop window) */
  
  if (!(cs->style & WS_CHILD) && 
      (X11DRV_WND_GetXRootWindow(wndPtr) == DefaultRootWindow(display)))
  {
      Window    wGroupLeader;
      XWMHints* wm_hints;
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
      wndPtr->flags |= WIN_NATIVE;

      win_attr.bit_gravity   = BGNorthWest;
      win_attr.colormap      = X11DRV_COLOR_GetColormap();
      win_attr.backing_store = Options.backingstore ? WhenMapped : NotUseful;
      win_attr.save_under    = ((classPtr->style & CS_SAVEBITS) != 0);
      win_attr.cursor        = X11DRV_MOUSE_XCursor;

      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->bit_gravity = BGNorthWest;
      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = 
	TSXCreateWindow( display, 
			 X11DRV_WND_GetXRootWindow(wndPtr), 
			 cs->x, cs->y, cs->cx, cs->cy, 
			 0, CopyFromParent, 
			 InputOutput, CopyFromParent,
			 CWEventMask | CWOverrideRedirect |
			 CWColormap | CWCursor | CWSaveUnder |
			 CWBackingStore | CWBitGravity, 
			 &win_attr );
      
      if(!(wGroupLeader = X11DRV_WND_GetXWindow(wndPtr)))
	return FALSE;

      if (wndPtr->flags & WIN_MANAGED) {
	XClassHint *class_hints = TSXAllocClassHint();

	if (class_hints) {
	  class_hints->res_name = "wineManaged";
	  class_hints->res_class = "Wine";
	  TSXSetClassHint( display, ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window, class_hints );
	  TSXFree (class_hints);
	}

	if (cs->dwExStyle & WS_EX_DLGMODALFRAME) {
	  XSizeHints* size_hints = TSXAllocSizeHints();
	  
	  if (size_hints) {
	      size_hints->min_width = size_hints->max_width = cs->cx;
	      size_hints->min_height = size_hints->max_height = cs->cy;
                size_hints->flags = (PSize | PMinSize | PMaxSize);
                TSXSetWMSizeHints( display, X11DRV_WND_GetXWindow(wndPtr), size_hints,
				   XA_WM_NORMAL_HINTS );
                TSXFree(size_hints);
            }
        }
      }
      
      if (cs->hwndParent)  /* Get window owner */
      {
	  Window w;
          WND *tmpWnd = WIN_FindWndPtr(cs->hwndParent);

	  w = X11DRV_WND_FindXWindow( tmpWnd );
	  if (w != None)
	  {
	      TSXSetTransientForHint( display, X11DRV_WND_GetXWindow(wndPtr), w );
	      wGroupLeader = w;
	  }
          WIN_ReleaseWndPtr(tmpWnd);
      }

      wm_hints = TSXAllocWMHints();
      {
	  wm_hints->flags = InputHint | StateHint | WindowGroupHint;
	  wm_hints->input = True;
	  if( wndPtr->dwStyle & WS_VISIBLE )
	      wm_hints->initial_state = (wndPtr->dwStyle & WS_MINIMIZE &&
					wndPtr->flags & WIN_MANAGED ) ? 
					IconicState : NormalState;
	  else
	      wm_hints->initial_state = WithdrawnState;
	  wm_hints->window_group = wGroupLeader;

	  TSXSetWMHints( display, X11DRV_WND_GetXWindow(wndPtr), wm_hints );
	  TSXFree(wm_hints);
      }
      X11DRV_WND_RegisterWindow( wndPtr );
  }
  return TRUE;
}

/***********************************************************************
 *		X11DRV_WND_DestroyWindow
 */
BOOL X11DRV_WND_DestroyWindow(WND *wndPtr)
{
   if (X11DRV_WND_GetXWindow(wndPtr))
     {
       XEvent xe;
       TSXDeleteContext( display, X11DRV_WND_GetXWindow(wndPtr), winContext );
       TSXDestroyWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
       while( TSXCheckWindowEvent(display, X11DRV_WND_GetXWindow(wndPtr), NoEventMask, &xe) );
       ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = None;
     }

   return TRUE;
}

/*****************************************************************
 *		X11DRV_WND_SetParent
 */
WND *X11DRV_WND_SetParent(WND *wndPtr, WND *pWndParent)
{
    WND *pDesktop = WIN_GetDesktop();
    
    if( wndPtr && pWndParent && (wndPtr != pDesktop) )
    {
	WND* pWndPrev = wndPtr->parent;

	if( pWndParent != pWndPrev )
	{
	    if ( X11DRV_WND_GetXWindow(wndPtr) )
	    {
		/* Toplevel window needs to be reparented.  Used by Tk 8.0 */

		TSXDestroyWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
		((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = None;
	    }

	    WIN_UnlinkWindow(wndPtr->hwndSelf);
	    wndPtr->parent = pWndParent;

            /* Create an X counterpart for reparented top-level windows
	     * when not in the desktop mode. */

            if( pWndParent == pDesktop )
            {
                wndPtr->dwStyle &= ~WS_CHILD;
                wndPtr->wIDmenu = 0;
                if( X11DRV_WND_GetXRootWindow(wndPtr) == DefaultRootWindow(display) )
                {
                    CREATESTRUCTA cs;
                    cs.lpCreateParams = NULL;
                    cs.hInstance = 0; /* not used if following call */
                    cs.hMenu = 0; /* not used in following call */
                    cs.hwndParent = pWndParent->hwndSelf;
                    cs.cy = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
                    if (!cs.cy)
                      cs.cy = 1;
                    cs.cx = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
                    if (!cs.cx)
                      cs.cx = 1;
                    cs.y = wndPtr->rectWindow.top;
                    cs.x = wndPtr->rectWindow.left;
                    cs.style = wndPtr->dwStyle;
                    cs.lpszName = 0; /* not used in following call */
                    cs.lpszClass = 0; /*not used in following call */
                    cs.dwExStyle = wndPtr->dwExStyle;
                    X11DRV_WND_CreateWindow(wndPtr, wndPtr->class,
                                            &cs, FALSE);
                }
            }
            else /* a child window */
            {
                if( !( wndPtr->dwStyle & WS_CHILD ) )
                {
                    wndPtr->dwStyle |= WS_CHILD;
                    if( wndPtr->wIDmenu != 0)
                    {
                        DestroyMenu( (HMENU) wndPtr->wIDmenu );
                        wndPtr->wIDmenu = 0;
                    }
                }
            }
	    WIN_LinkWindow(wndPtr->hwndSelf, HWND_TOP);
	}
        WIN_ReleaseDesktop();
	return pWndPrev;
    } /* failure */
    WIN_ReleaseDesktop();
    return 0;
}

/***********************************************************************
 *		X11DRV_WND_ForceWindowRaise
 *
 * Raise a window on top of the X stacking order, while preserving 
 * the correct Windows Z order.
 */
void X11DRV_WND_ForceWindowRaise(WND *wndPtr)
{
  XWindowChanges winChanges;
  WND *wndPrev,*pDesktop = WIN_GetDesktop();
  
  if( !wndPtr || !X11DRV_WND_GetXWindow(wndPtr) || (wndPtr->flags & WIN_MANAGED) )
    return;
  
  /* Raise all windows up to wndPtr according to their Z order.
   * (it would be easier with sibling-related Below but it doesn't
   * work very well with SGI mwm for instance)
   */
  winChanges.stack_mode = Above;
  while (wndPtr)
    {
      if (X11DRV_WND_GetXWindow(wndPtr)) 
	TSXReconfigureWMWindow( display, X11DRV_WND_GetXWindow(wndPtr), 0,
				CWStackMode, &winChanges );
      wndPrev = pDesktop->child;
      if (wndPrev == wndPtr) break;
      while (wndPrev && (wndPrev->next != wndPtr)) wndPrev = wndPrev->next;
      wndPtr = wndPrev;
    }
  WIN_ReleaseDesktop();
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
    return X11DRV_WND_GetXWindow(wndPtr);
  else
    {
      Window window, root, parent, *children;
      int nchildren;
      window = X11DRV_WND_GetXWindow(wndPtr);
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
void X11DRV_WND_SetWindowPos(WND *wndPtr, const WINDOWPOS *winpos, BOOL bChangePos)
{
    XWindowChanges winChanges;
    int changeMask = 0;
    WND *winposPtr = WIN_FindWndPtr( winpos->hwnd );

    if(!wndPtr->hwndSelf) wndPtr = NULL; /* FIXME: WND destroyed, shouldn't happend!!! */
  
    if (!(winpos->flags & SWP_SHOWWINDOW) && (winpos->flags & SWP_HIDEWINDOW))
    {
      if(X11DRV_WND_GetXWindow(wndPtr)) 
	TSXUnmapWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
    }

    if(bChangePos)
    {
      if ( !(winpos->flags & SWP_NOSIZE))
	{
	  winChanges.width     = (winpos->cx > 0 ) ? winpos->cx : 1;
	  winChanges.height    = (winpos->cy > 0 ) ? winpos->cy : 1;
	  changeMask |= CWWidth | CWHeight;
	  
	  /* Tweak dialog window size hints */
	  
	  if ((winposPtr->flags & WIN_MANAGED) &&
	      (winposPtr->dwExStyle & WS_EX_DLGMODALFRAME))
	    {
	      XSizeHints *size_hints = TSXAllocSizeHints();
	      
	      if (size_hints)
		{
		  long supplied_return;
		  
		  TSXGetWMSizeHints( display, X11DRV_WND_GetXWindow(winposPtr), size_hints,
				     &supplied_return, XA_WM_NORMAL_HINTS);
		  size_hints->min_width = size_hints->max_width = winpos->cx;
		  size_hints->min_height = size_hints->max_height = winpos->cy;
		  TSXSetWMSizeHints( display, X11DRV_WND_GetXWindow(winposPtr), size_hints,
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
              
              WIN_ReleaseWndPtr(insertPtr);
	    }
	}
      if (changeMask)
	{
	  TSXReconfigureWMWindow( display, X11DRV_WND_GetXWindow(winposPtr), 0, changeMask, &winChanges );
	}
    }

    if ( winpos->flags & SWP_SHOWWINDOW )
    {
      if(X11DRV_WND_GetXWindow(wndPtr)) TSXMapWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
    }
    WIN_ReleaseWndPtr(winposPtr);
}

/*****************************************************************
 *		X11DRV_WND_SetText
 */
void X11DRV_WND_SetText(WND *wndPtr, LPCSTR text)
{   
  if (!X11DRV_WND_GetXWindow(wndPtr))
    return;

  TSXStoreName( display, X11DRV_WND_GetXWindow(wndPtr), text );
  TSXSetIconName( display, X11DRV_WND_GetXWindow(wndPtr), text );
}

/*****************************************************************
 *		X11DRV_WND_SetFocus
 *
 * Set the X focus.
 * Explicit colormap management seems to work only with OLVWM.
 */
void X11DRV_WND_SetFocus(WND *wndPtr)
{
  HWND hwnd =  wndPtr->hwndSelf;
  XWindowAttributes win_attr;
  Window win;
  
  /* Only mess with the X focus if there's */
  /* no desktop window and no window manager. */
  if ((X11DRV_WND_GetXRootWindow(wndPtr) != DefaultRootWindow(display))
      || Options.managed) return;
  
  if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
      if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
	TSXUninstallColormap( display, X11DRV_COLOR_GetColormap() );
      return;
    }
  
  /* Set X focus and install colormap */
  
  if (!(win = X11DRV_WND_FindXWindow(wndPtr))) return;
  if (!TSXGetWindowAttributes( display, win, &win_attr ) ||
      (win_attr.map_state != IsViewable))
    return;  /* If window is not viewable, don't change anything */
  
  TSXSetInputFocus( display, win, RevertToParent, CurrentTime );
  if (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE)
    TSXInstallColormap( display, X11DRV_COLOR_GetColormap() );
  
  EVENT_Synchronize();
}

/*****************************************************************
 *		X11DRV_WND_PreSizeMove
 */
void X11DRV_WND_PreSizeMove(WND *wndPtr)
{
  if (!(wndPtr->dwStyle & WS_CHILD) && 
      (X11DRV_WND_GetXRootWindow(wndPtr) == DefaultRootWindow(display)))
    TSXGrabServer( display );
}

/*****************************************************************
 *		 X11DRV_WND_PostSizeMove
 */
void X11DRV_WND_PostSizeMove(WND *wndPtr)
{
  if (!(wndPtr->dwStyle & WS_CHILD) && 
      (X11DRV_GetXRootWindow() == DefaultRootWindow(display)))
    TSXUngrabServer( display );
}

/*****************************************************************
 *		 X11DRV_WND_SurfaceCopy
 *
 * Copies rect to (rect.left + dx, rect.top + dy). 
 */
void X11DRV_WND_SurfaceCopy(WND* wndPtr, DC *dcPtr, INT dx, INT dy, 
			    const RECT *rect, BOOL bUpdate)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dcPtr->physDev;
    POINT dst, src;
  
    dst.x = (src.x = dcPtr->w.DCOrgX + rect->left) + dx;
    dst.y = (src.y = dcPtr->w.DCOrgY + rect->top) + dy;
  
    if (bUpdate) /* handles non-Wine windows hanging over the copied area */
	TSXSetGraphicsExposures( display, physDev->gc, True );
    TSXSetFunction( display, physDev->gc, GXcopy );
    TSXCopyArea( display, physDev->drawable, physDev->drawable,
                 physDev->gc, src.x, src.y,
                 rect->right - rect->left,
                 rect->bottom - rect->top,
                 dst.x, dst.y );
    if (bUpdate)
	TSXSetGraphicsExposures( display, physDev->gc, False );

    if (bUpdate) /* Make sure exposure events have been processed */
	EVENT_Synchronize();
}

/***********************************************************************
 *		X11DRV_WND_SetDrawable
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
void X11DRV_WND_SetDrawable(WND *wndPtr, DC *dc, WORD flags, BOOL bSetClipOrigin)
{
    X11DRV_PDEVICE *physDev = (X11DRV_PDEVICE *)dc->physDev;

    if (!wndPtr)  /* Get a DC for the whole screen */
    {
        dc->w.DCOrgX = 0;
        dc->w.DCOrgY = 0;
        physDev->drawable = X11DRV_WND_GetXRootWindow(wndPtr);
        TSXSetSubwindowMode( display, physDev->gc, IncludeInferiors );
    }
    else
    {
        if (flags & DCX_WINDOW)
        {
            dc->w.DCOrgX  = wndPtr->rectWindow.left;
            dc->w.DCOrgY  = wndPtr->rectWindow.top;
        }
        else
        {
            dc->w.DCOrgX  = wndPtr->rectClient.left;
            dc->w.DCOrgY  = wndPtr->rectClient.top;
        }
        while (!X11DRV_WND_GetXWindow(wndPtr))
        {
            wndPtr = wndPtr->parent;
            dc->w.DCOrgX += wndPtr->rectClient.left;
            dc->w.DCOrgY += wndPtr->rectClient.top;
        }
        dc->w.DCOrgX -= wndPtr->rectWindow.left;
        dc->w.DCOrgY -= wndPtr->rectWindow.top;
        physDev->drawable = X11DRV_WND_GetXWindow(wndPtr);

#if 0
	/* This is needed when we reuse a cached DC because
	 * SetDCState() called by ReleaseDC() screws up DC
	 * origins for child windows.
	 */

	if( bSetClipOrigin )
	    TSXSetClipOrigin( display, physDev->gc, dc->w.DCOrgX, dc->w.DCOrgY );
#endif
    }
}

/***********************************************************************
 *              X11DRV_WND_SetHostAttr
 */
BOOL X11DRV_WND_SetHostAttr(WND* wnd, INT ha, INT value)
{
    Window w;

    if( (w = X11DRV_WND_GetXWindow(wnd)) )
    {
	XSetWindowAttributes win_attr;

	switch( ha )
	{
	case HAK_BITGRAVITY:

		    if( ((X11DRV_WND_DATA *) wnd->pDriverData)->bit_gravity != value )
		    {
		        win_attr.bit_gravity = value;
		        ((X11DRV_WND_DATA *) wnd->pDriverData)->bit_gravity = value;
		        TSXChangeWindowAttributes( display, w, CWBitGravity, &win_attr );
		    }
		    return TRUE;

	case HAK_ACCEPTFOCUS:

		if( (wnd->flags & WIN_MANAGED) )
		{
		    XWMHints* wm_hints = TSXAllocWMHints();

		    if( wm_hints )
		    {
			wm_hints->flags = InputHint;
			wm_hints->input = value;
			TSXSetWMHints( display, X11DRV_WND_GetXWindow(wnd), wm_hints );
			TSXFree( wm_hints );
			return TRUE;
		    }
		}
		break;
	}
    }
    return FALSE;
}

/***********************************************************************
 *		X11DRV_WND_IsSelfClipping
 */
BOOL X11DRV_WND_IsSelfClipping(WND *wndPtr)
{
  if( X11DRV_WND_GetXWindow(wndPtr) ) 
      return TRUE; /* X itself will do the clipping */

  return FALSE;
}

#endif /* !defined(X_DISPLAY_MISSING) */
