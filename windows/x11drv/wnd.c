/*
 * X11 windows driver
 *
 * Copyright 1993, 1994, 1995, 1996 Alexandre Julliard
 *                             1993 David Metcalfe
 *                       1995, 1996 Alex Korobka
 */

#include "config.h"

#include <X11/Xatom.h>

#include "ts_xlib.h"
#include "ts_xutil.h"
#include "ts_shape.h"

#include <stdlib.h>
#include <string.h>

#include "bitmap.h"
#include "debugtools.h"
#include "gdi.h"
#include "options.h"
#include "message.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(win);

extern Atom wmChangeState;

#define HAS_DLGFRAME(style,exStyle) \
((!((style) & WS_THICKFRAME)) && (((style) & WS_DLGFRAME) || ((exStyle) & WS_EX_DLGMODALFRAME)))

/**********************************************************************/

WND_DRIVER X11DRV_WND_Driver =
{
  X11DRV_WND_ForceWindowRaise,
  X11DRV_WND_PreSizeMove,
  X11DRV_WND_PostSizeMove,
  X11DRV_WND_SurfaceCopy,
  X11DRV_WND_SetHostAttr
};


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
 *		X11DRV_WND_IsZeroSizeWnd
 *
 * Return TRUE if the window has a height or widht less or equal to 0
 */
static BOOL X11DRV_WND_IsZeroSizeWnd(WND *wndPtr)
{
    if ( (wndPtr->rectWindow.left >= wndPtr->rectWindow.right) ||
         (wndPtr->rectWindow.top >= wndPtr->rectWindow.bottom) )
        return TRUE;
    else
        return FALSE;
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

  if (X11DRV_WND_IsZeroSizeWnd(wndPtr))
  {
    WIN_ReleaseDesktop();
    return;
  }
  
  if( !wndPtr || !X11DRV_WND_GetXWindow(wndPtr) || (wndPtr->dwExStyle & WS_EX_MANAGED) )
  {
      WIN_ReleaseDesktop();
    return;
  }
  
  /* Raise all windows up to wndPtr according to their Z order.
   * (it would be easier with sibling-related Below but it doesn't
   * work very well with SGI mwm for instance)
   */
  winChanges.stack_mode = Above;
  while (wndPtr)
    {
      if ( !X11DRV_WND_IsZeroSizeWnd(wndPtr) && X11DRV_WND_GetXWindow(wndPtr) )         
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
 * Used by X11DRV_WND_SetWindowPos().
 */
static Window X11DRV_WND_FindDesktopXWindow( WND *wndPtr )
{
  if (!(wndPtr->dwExStyle & WS_EX_MANAGED))
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
    BOOL isZeroSizeWnd = FALSE;
    BOOL forceMapWindow = FALSE;
    WND *winposPtr = WIN_FindWndPtr( winpos->hwnd );
    if ( !winposPtr ) return;

    /* find out if after this function we will end out with a zero-size window */
    if (X11DRV_WND_IsZeroSizeWnd(winposPtr))
    {
        /* if current size is 0, and no resizing */
        if (winpos->flags & SWP_NOSIZE)
            isZeroSizeWnd = TRUE;
        else if ((winpos->cx > 0) && (winpos->cy > 0)) 
        {
            /* if this function is setting a new size > 0 for the window, we
               should map the window if WS_VISIBLE is set */
            if ((winposPtr->dwStyle & WS_VISIBLE) && !(winpos->flags & SWP_HIDEWINDOW))
                forceMapWindow = TRUE;
        }
    }
    /* if resizing to 0 */
    if ( !(winpos->flags & SWP_NOSIZE) && ((winpos->cx <= 0) || (winpos->cy <= 0)) )
            isZeroSizeWnd = TRUE;

    if(!wndPtr->hwndSelf) wndPtr = NULL; /* FIXME: WND destroyed, shouldn't happen!!! */
  
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
	  
	  if ((winposPtr->dwExStyle & WS_EX_MANAGED) &&
               HAS_DLGFRAME(winposPtr->dwStyle,winposPtr->dwExStyle))
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
	if (!(winpos->flags & SWP_NOZORDER) && !isZeroSizeWnd)
	{
	  winChanges.stack_mode = Below;
	  changeMask |= CWStackMode;
	  
	  if (winpos->hwndInsertAfter == HWND_TOP) winChanges.stack_mode = Above;
	  else if (winpos->hwndInsertAfter != HWND_BOTTOM)
	    {
	      WND*   insertPtr = WIN_FindWndPtr( winpos->hwndInsertAfter );
	      Window stack[2];

          /* If the window where we should do the insert is zero-sized (not mapped)
             don't used this window since it will possibly crash the X server,
             use the "non zero-sized" window above */ 
	      if (X11DRV_WND_IsZeroSizeWnd(insertPtr))
          {
              /* find the window on top of the zero sized window */ 
              WND *pDesktop = WIN_GetDesktop();
              WND *wndPrev = pDesktop->child;
              WND *wndZeroSized = insertPtr;

              while (1)
              {
                  if (wndPrev == wndZeroSized)
                      break;  /* zero-sized window is on top */

                  while (wndPrev && (wndPrev->next != wndZeroSized))
                      wndPrev = wndPrev->next;

                  /* check if the window found is not zero-sized */
                  if (X11DRV_WND_IsZeroSizeWnd(wndPrev))
                  {
                      wndZeroSized = wndPrev; /* restart the search */
                      wndPrev = pDesktop->child;
                  }
                  else
                      break;   /* "above" window is found */
              }
              WIN_ReleaseDesktop();
              
              if (wndPrev == wndZeroSized)
              {
                  /* the zero-sized window is on top */
                  /* so set the window on top */
                  winChanges.stack_mode = Above;
              }
              else
              {
                  stack[0] = X11DRV_WND_FindDesktopXWindow( wndPrev );
                  stack[1] = X11DRV_WND_FindDesktopXWindow( winposPtr );

                  TSXRestackWindows(display, stack, 2);
                  changeMask &= ~CWStackMode;
              }
          }
          else  /* Normal behavior, windows are not zero-sized */
          {
              stack[0] = X11DRV_WND_FindDesktopXWindow( insertPtr );
              stack[1] = X11DRV_WND_FindDesktopXWindow( winposPtr );
	      
	          TSXRestackWindows(display, stack, 2);
	          changeMask &= ~CWStackMode;
          }

          WIN_ReleaseWndPtr(insertPtr);
	    }
	}
	if (changeMask && X11DRV_WND_GetXWindow(winposPtr))
	{
	    TSXReconfigureWMWindow( display, X11DRV_WND_GetXWindow(winposPtr), 0, changeMask, &winChanges );
	    if( winposPtr->clsStyle & (CS_VREDRAW | CS_HREDRAW) )
		X11DRV_WND_SetGravity( winposPtr, ForgetGravity );
	}
    }
    
    /* don't map the window if it's a zero size window */
    if ( ((winpos->flags & SWP_SHOWWINDOW) && !isZeroSizeWnd) || forceMapWindow )
    {
	if(X11DRV_WND_GetXWindow(wndPtr)) 
	   TSXMapWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
    }
    WIN_ReleaseWndPtr(winposPtr);
}

/*****************************************************************
 *		X11DRV_WND_PreSizeMove
 */
void X11DRV_WND_PreSizeMove(WND *wndPtr)
{
  /* Grab the server only when moving top-level windows without desktop */
  if ((X11DRV_GetXRootWindow() == DefaultRootWindow(display)) &&
      (wndPtr->parent->hwndSelf == GetDesktopWindow()))
    TSXGrabServer( display );
}

/*****************************************************************
 *		 X11DRV_WND_PostSizeMove
 */
void X11DRV_WND_PostSizeMove(WND *wndPtr)
{
  if ((X11DRV_GetXRootWindow() == DefaultRootWindow(display)) &&
      (wndPtr->parent->hwndSelf == GetDesktopWindow()))
    TSXUngrabServer( display );
}

/*****************************************************************
 *		 X11DRV_WND_SurfaceCopy
 *
 * Copies rect to (rect.left + dx, rect.top + dy). 
 */
void X11DRV_WND_SurfaceCopy(WND* wndPtr, HDC hdc, INT dx, INT dy, 
			    const RECT *rect, BOOL bUpdate)
{
    X11DRV_PDEVICE *physDev;
    POINT dst, src;
    DC *dcPtr = DC_GetDCPtr( hdc );

    if (!dcPtr) return;
    physDev = (X11DRV_PDEVICE *)dcPtr->physDev;
    dst.x = (src.x = dcPtr->DCOrgX + rect->left) + dx;
    dst.y = (src.y = dcPtr->DCOrgY + rect->top) + dy;
  
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
    GDI_ReleaseObj( hdc );

    if (bUpdate) /* Make sure exposure events have been processed */
        X11DRV_Synchronize();
}

/***********************************************************************
 *              X11DRV_SetWMHint
 */
static BOOL X11DRV_SetWMHint(Display* display, WND* wndPtr, int hint, int val)
{
    XWMHints* wm_hints = TSXGetWMHints( display, X11DRV_WND_GetXWindow(wndPtr) );
    if (!wm_hints) wm_hints = TSXAllocWMHints();
    if (wm_hints)
    {
        wm_hints->flags = hint;
	switch( hint )
	{
	    case InputHint:
		 wm_hints->input = val;
		 break;

	    case StateHint:
		 wm_hints->initial_state = val;
		 break;

	    case IconPixmapHint:
		 wm_hints->icon_pixmap = (Pixmap)val;
		 break;

	    case IconWindowHint:
		 wm_hints->icon_window = (Window)val;
		 break;
	}

        TSXSetWMHints( display, X11DRV_WND_GetXWindow(wndPtr), wm_hints );
        TSXFree(wm_hints);
        return TRUE;
    }
    return FALSE;
}


void X11DRV_WND_SetGravity( WND* wnd, int value )
{
    X11DRV_WND_DATA *data = wnd->pDriverData;

    if (data->window && data->bit_gravity != value )
    {
        XSetWindowAttributes win_attr;
        win_attr.bit_gravity = value;
        data->bit_gravity = value;
        TSXChangeWindowAttributes( display, data->window, CWBitGravity, &win_attr );
    }
}


/***********************************************************************
 *              X11DRV_WND_SetHostAttr
 *
 * This function returns TRUE if the attribute is supported and the
 * action was successful. Otherwise it should return FALSE and Wine will try 
 * to get by without the functionality provided by the host window system.
 */
BOOL X11DRV_WND_SetHostAttr(WND* wnd, INT ha, INT value)
{
    Window w;

    if( (w = X11DRV_WND_GetXWindow(wnd)) )
    {
	switch( ha )
	{
	case HAK_ICONICSTATE: /* called when a window is minimized/restored */

            /* don't do anything if it'a zero size window */ 
            if (X11DRV_WND_IsZeroSizeWnd(wnd))
                return TRUE;

		    if( (wnd->dwExStyle & WS_EX_MANAGED) )
		    {
			if( value )
			{
			    if( wnd->dwStyle & WS_VISIBLE )
			    {
				XClientMessageEvent ev;

				/* FIXME: set proper icon */

				ev.type = ClientMessage;
				ev.display = display;
				ev.message_type = wmChangeState;
				ev.format = 32;
				ev.data.l[0] = IconicState;
				ev.window = w;

				if( TSXSendEvent (display,
		RootWindow( display, XScreenNumberOfScreen(X11DRV_GetXScreen()) ), 
		True, (SubstructureRedirectMask | SubstructureNotifyMask), (XEvent*)&ev))
				{
				    XEvent xe;
				    TSXFlush (display);
				    while( !TSXCheckTypedWindowEvent( display, w, UnmapNotify, &xe) );
				}
				else 
				    break;
			    }
			    else
				X11DRV_SetWMHint( display, wnd, StateHint, IconicState );
			}
			else
			{
			    if( !(wnd->flags & WS_VISIBLE) )
				X11DRV_SetWMHint( display, wnd, StateHint, NormalState );
			    else
			    {
				XEvent xe;
				TSXMapWindow(display, w );
				while( !TSXCheckTypedWindowEvent( display, w, MapNotify, &xe) );
			    }
			}
			return TRUE;
		    }
		    break;
	}
    }
    return FALSE;
}
