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
#include "color.h"
#include "debugtools.h"
#include "dce.h"
#include "options.h"
#include "message.h"
#include "heap.h"
#include "win.h"
#include "windef.h"
#include "class.h"
#include "x11drv.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(win);

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
((!((style) & WS_THICKFRAME)) && (((style) & WS_DLGFRAME) || ((exStyle) & WS_EX_DLGMODALFRAME)))

/**********************************************************************/

extern Cursor X11DRV_MOUSE_XCursor;  /* Current X cursor */
extern BOOL   X11DRV_CreateBitmap( HBITMAP );
extern Pixmap X11DRV_BITMAP_Pixmap( HBITMAP );

/**********************************************************************/

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

Atom wmProtocols = None;
Atom wmDeleteWindow = None;
Atom dndProtocol = None;
Atom dndSelection = None;
Atom wmChangeState = None;

Atom kwmDockWindow = None;

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
 *		X11DRV_WND_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
static void X11DRV_WND_RegisterWindow(WND *wndPtr)
{
  TSXSetWMProtocols( display, X11DRV_WND_GetXWindow(wndPtr), &wmDeleteWindow, 1 );
  
  if (!winContext) winContext = TSXUniqueContext();
  TSXSaveContext( display, X11DRV_WND_GetXWindow(wndPtr), 
                  winContext, (char *) wndPtr->hwndSelf );
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
     ERR("Trying to destroy window again. Not good.\n");
     return;
  }
  if(pWndDriverData->window)
    {
      ERR(
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
    if( wmChangeState == None )
        wmChangeState = TSXInternAtom (display, "WM_CHANGE_STATE", False);
    if (kwmDockWindow == None)
        kwmDockWindow = TSXInternAtom( display, "KWM_DOCKWINDOW", False );

    ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = X11DRV_GetXRootWindow();
    X11DRV_WND_RegisterWindow( wndPtr );

    return TRUE;
}

/**********************************************************************
 *		X11DRV_WND_IconChanged
 *
 * hIcon or hIconSm has changed (or is being initialised for the
 * first time). Complete the X11 driver-specific initialisation.
 *
 * This is not entirely correct, may need to create
 * an icon window and set the pixmap as a background
 */
static void X11DRV_WND_IconChanged(WND *wndPtr)
{

    HICON16 hIcon = NC_IconForWindow(wndPtr); 

    if( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap )
        DeleteObject( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap );

    if( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask )
        DeleteObject( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask);

    if (!hIcon)
    {
        ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap= 0;
        ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask= 0;
    }
    else
    {
	HBITMAP hbmOrig;
	RECT rcMask;
	BITMAP bmMask;
	ICONINFO ii;
	HDC hDC;

	GetIconInfo(hIcon, &ii);

	X11DRV_CreateBitmap(ii.hbmMask);
	X11DRV_CreateBitmap(ii.hbmColor);

	GetObjectA(ii.hbmMask, sizeof(bmMask), &bmMask);
	rcMask.top    = 0;
	rcMask.left   = 0;
	rcMask.right  = bmMask.bmWidth;
	rcMask.bottom = bmMask.bmHeight;

	hDC = CreateCompatibleDC(0);
	hbmOrig = SelectObject(hDC, ii.hbmMask);
	InvertRect(hDC, &rcMask);
	SelectObject(hDC, hbmOrig);
	DeleteDC(hDC);

        ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap = ii.hbmColor;
        ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask= ii.hbmMask;
    }
    return;
}

static void X11DRV_WND_SetIconHints(WND *wndPtr, XWMHints *hints)
{
    if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap)
    {
	hints->icon_pixmap
	    = X11DRV_BITMAP_Pixmap(((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap);
	hints->flags |= IconPixmapHint;
    }
    else
	hints->flags &= ~IconPixmapHint;

    if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask)
    {
	hints->icon_mask
	    = X11DRV_BITMAP_Pixmap(((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask);
	hints->flags |= IconMaskHint;
    }
    else
	hints->flags &= ~IconMaskHint;
}

/**********************************************************************
 *		X11DRV_WND_UpdateIconHints
 *
 * hIcon or hIconSm has changed (or is being initialised for the
 * first time). Complete the X11 driver-specific initialisation
 * and set the window hints.
 *
 * This is not entirely correct, may need to create
 * an icon window and set the pixmap as a background
 */
static void X11DRV_WND_UpdateIconHints(WND *wndPtr)
{
    XWMHints* wm_hints;

    X11DRV_WND_IconChanged(wndPtr);

    wm_hints = TSXGetWMHints( display, X11DRV_WND_GetXWindow(wndPtr) );
    if (!wm_hints) wm_hints = TSXAllocWMHints();
    if (wm_hints)
    {
        X11DRV_WND_SetIconHints(wndPtr, wm_hints);
        TSXSetWMHints( display, X11DRV_WND_GetXWindow(wndPtr), wm_hints );
        TSXFree( wm_hints );
    }
}


/**********************************************************************
 *		X11DRV_WND_CreateWindow
 */
BOOL X11DRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCTA *cs, BOOL bUnicode)
{
  /* Create the X window (only for top-level windows, and then only */
  /* when there's no desktop window) */
  
  if ((X11DRV_GetXRootWindow() == DefaultRootWindow(display))
      && (wndPtr->parent->hwndSelf == GetDesktopWindow()))
  {
      Window    wGroupLeader;
      XWMHints* wm_hints;
      XSetWindowAttributes win_attr;
      
      /* Create "managed" windows only if a title bar or resizable */
      /* frame is required. */
        if (WIN_WindowNeedsWMBorder(cs->style, cs->dwExStyle)) {
	  win_attr.event_mask = ExposureMask | KeyPressMask |
	    KeyReleaseMask | PointerMotionMask |
	    ButtonPressMask | ButtonReleaseMask |
	    FocusChangeMask | StructureNotifyMask;
	  win_attr.override_redirect = FALSE;
	  wndPtr->flags |= WIN_MANAGED;
	} else {
	  win_attr.event_mask = ExposureMask | KeyPressMask |
	    KeyReleaseMask | PointerMotionMask |
	    ButtonPressMask | ButtonReleaseMask |
	    FocusChangeMask;
	  win_attr.override_redirect = TRUE;
	}
      wndPtr->flags |= WIN_NATIVE;

      win_attr.bit_gravity   = (classPtr->style & (CS_VREDRAW | CS_HREDRAW)) ? BGForget : BGNorthWest;
      win_attr.colormap      = X11DRV_PALETTE_PaletteXColormap;
      win_attr.backing_store = NotUseful;
      win_attr.save_under    = ((classPtr->style & CS_SAVEBITS) != 0);
      win_attr.cursor        = X11DRV_MOUSE_XCursor;

      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap = 0;
      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask = 0;
      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->bit_gravity = win_attr.bit_gravity;

      /* Zero-size X11 window hack.  X doesn't like them, and will crash */
      /* with a BadValue unless we do something ugly like this. */
      /* FIXME:  there must be a better way.  */
        if (cs->cx <= 0) cs->cx = 1;
        if (cs->cy <= 0) cs->cy = 1;
      /* EMXIF */

      ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = 
	TSXCreateWindow( display, X11DRV_GetXRootWindow(), 
			 cs->x, cs->y, cs->cx, cs->cy, 
			 0, CopyFromParent, 
			 InputOutput, CopyFromParent,
			 CWEventMask | CWOverrideRedirect |
			 CWColormap | CWCursor | CWSaveUnder |
			 CWBackingStore | CWBitGravity, 
			 &win_attr );
      
      if(!(wGroupLeader = X11DRV_WND_GetXWindow(wndPtr)))
	return FALSE;

      /* If we are the systray, we need to be managed to be noticed by KWM */

      if (wndPtr->dwExStyle & WS_EX_TRAYWINDOW)
	X11DRV_WND_DockWindow(wndPtr);

      if (wndPtr->flags & WIN_MANAGED) 
      {
	  XClassHint *class_hints = TSXAllocClassHint();
	  XSizeHints* size_hints = TSXAllocSizeHints();
	  
	  if (class_hints) 
	  {
	      class_hints->res_name = "wineManaged";
	      class_hints->res_class = "Wine";
	      TSXSetClassHint( display, ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window, class_hints );
	      TSXFree (class_hints);
	  }

	  if (size_hints) 
	  {
              size_hints->win_gravity = StaticGravity;
              size_hints->x = cs->x;
              size_hints->y = cs->y;
              size_hints->flags = PWinGravity|PPosition;

              if (HAS_DLGFRAME(cs->style,cs->dwExStyle))
	      {
	          size_hints->min_width = size_hints->max_width = cs->cx;
	          size_hints->min_height = size_hints->max_height = cs->cy;
                  size_hints->flags |= PMinSize | PMaxSize;
              }

              TSXSetWMSizeHints( display, X11DRV_WND_GetXWindow(wndPtr), 
                                 size_hints, XA_WM_NORMAL_HINTS );
              TSXFree(size_hints);
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

      if ((wm_hints = TSXAllocWMHints()))
      {
	  wm_hints->flags = InputHint | StateHint | WindowGroupHint;
	  wm_hints->input = True;

	  if( wndPtr->flags & WIN_MANAGED )
	  {
	      X11DRV_WND_IconChanged(wndPtr);
	      X11DRV_WND_SetIconHints(wndPtr, wm_hints);

	      wm_hints->initial_state = (wndPtr->dwStyle & WS_MINIMIZE) 
					? IconicState : NormalState;
	  }
	  else
	      wm_hints->initial_state = NormalState;
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
   Window w;
   if ((w = X11DRV_WND_GetXWindow(wndPtr)))
   {
       XEvent xe;
       TSXDeleteContext( display, w, winContext );
       TSXDestroyWindow( display, w );
       while( TSXCheckWindowEvent(display, w, NoEventMask, &xe) );

       ((X11DRV_WND_DATA *) wndPtr->pDriverData)->window = None;
       if( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap )
       {
	   DeleteObject( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap );
	   ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconBitmap = 0;
       }
       if( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask )
       {
	   DeleteObject( ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask);
	   ((X11DRV_WND_DATA *) wndPtr->pDriverData)->hWMIconMask= 0;
       }
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
                if( X11DRV_GetXRootWindow() == DefaultRootWindow(display) )
                {
                    CREATESTRUCTA cs;
                    cs.lpCreateParams = NULL;
                    cs.hInstance = 0; /* not used in following call */
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
 * Used by X11DRV_WND_SetWindowPos().
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
    if ( !winposPtr ) return;

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
	  
	  if ((winposPtr->flags & WIN_MANAGED) &&
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
	if (changeMask && X11DRV_WND_GetXWindow(winposPtr))
	{
	    TSXReconfigureWMWindow( display, X11DRV_WND_GetXWindow(winposPtr), 0, changeMask, &winChanges );
	    if( winposPtr->class->style & (CS_VREDRAW | CS_HREDRAW) )
		X11DRV_WND_SetHostAttr( winposPtr, HAK_BITGRAVITY, BGForget );
	}
    }

    if ( winpos->flags & SWP_SHOWWINDOW )
    {
	if(X11DRV_WND_GetXWindow(wndPtr)) 
	   TSXMapWindow( display, X11DRV_WND_GetXWindow(wndPtr) );
    }
    WIN_ReleaseWndPtr(winposPtr);
}

/*****************************************************************
 *		X11DRV_WND_SetText
 */
void X11DRV_WND_SetText(WND *wndPtr, LPCWSTR text)
{   
    UINT count;
    char *buffer;
    static UINT text_cp = (UINT)-1;
    Window win;

    if (!(win = X11DRV_WND_GetXWindow(wndPtr))) return;

    if(text_cp == (UINT)-1)
    {
	text_cp = PROFILE_GetWineIniInt("x11drv", "TextCP", CP_ACP);
	TRACE("text_cp = %u\n", text_cp);
    }

    /* allocate new buffer for window text */
    count = WideCharToMultiByte(text_cp, 0, text, -1, NULL, 0, NULL, NULL);
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WCHAR) )))
    {
	ERR("Not enough memory for window text\n");
	return;
    }
    WideCharToMultiByte(text_cp, 0, text, -1, buffer, count, NULL, NULL);

    TSXStoreName( display, win, buffer );
    TSXSetIconName( display, win, buffer );
    HeapFree( GetProcessHeap(), 0, buffer );
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
  WND *w = wndPtr;
  
  /* Only mess with the X focus if there's */
  /* no desktop window and if the window is not managed by the WM. */
  if ((X11DRV_GetXRootWindow() != DefaultRootWindow(display))) return;
  while (w && !((X11DRV_WND_DATA *) w->pDriverData)->window)
      w = w->parent;
  if (!w) w = wndPtr;
  if (w->flags & WIN_MANAGED) return;

  if (!hwnd)	/* If setting the focus to 0, uninstall the colormap */
    {
      if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
	TSXUninstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
      return;
    }
  
  /* Set X focus and install colormap */
  
  if (!(win = X11DRV_WND_FindXWindow(wndPtr))) return;
  if (!TSXGetWindowAttributes( display, win, &win_attr ) ||
      (win_attr.map_state != IsViewable))
    return;  /* If window is not viewable, don't change anything */
  
  TSXSetInputFocus( display, win, RevertToParent, CurrentTime );
  if (X11DRV_PALETTE_PaletteFlags & X11DRV_PALETTE_PRIVATE)
    TSXInstallColormap( display, X11DRV_PALETTE_PaletteXColormap );
  
  EVENT_Synchronize();
}

/*****************************************************************
 *		X11DRV_WND_PreSizeMove
 */
void X11DRV_WND_PreSizeMove(WND *wndPtr)
{
  if (!(wndPtr->dwStyle & WS_CHILD) && (X11DRV_GetXRootWindow() == DefaultRootWindow(display)))
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
    INT dcOrgXCopy = 0, dcOrgYCopy = 0;
    BOOL offsetClipRgn = FALSE;

    if (!wndPtr)  /* Get a DC for the whole screen */
    {
        dc->w.DCOrgX = 0;
        dc->w.DCOrgY = 0;
        physDev->drawable = X11DRV_GetXRootWindow();
        TSXSetSubwindowMode( display, physDev->gc, IncludeInferiors );
    }
    else
    {
        /*
         * This function change the coordinate system (DCOrgX,DCOrgY)
         * values. When it moves the origin, other data like the current clipping
         * region will not be moved to that new origin. In the case of DCs that are class
         * or window DCs that clipping region might be a valid value from a previous use
         * of the DC and changing the origin of the DC without moving the clip region
         * results in a clip region that is not placed properly in the DC.
         * This code will save the dc origin, let the SetDrawable
         * modify the origin and reset the clipping. When the clipping is set,
         * it is moved according to the new DC origin.
         */
         if ( (wndPtr->class->style & (CS_OWNDC | CS_CLASSDC)) && (dc->w.hClipRgn > 0))
         {
             dcOrgXCopy = dc->w.DCOrgX;
             dcOrgYCopy = dc->w.DCOrgY;
             offsetClipRgn = TRUE;
         }

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

        /* reset the clip region, according to the new origin */
        if ( offsetClipRgn )
        {
             OffsetRgn(dc->w.hClipRgn, dc->w.DCOrgX - dcOrgXCopy,dc->w.DCOrgY - dcOrgYCopy);
        }
        
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
	XSetWindowAttributes win_attr;

	switch( ha )
	{
	case HAK_ICONICSTATE: /* called when a window is minimized/restored */

		    if( (wnd->flags & WIN_MANAGED) )
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

	case HAK_BITGRAVITY: /* called when a window is resized */

		    if( ((X11DRV_WND_DATA *) wnd->pDriverData)->bit_gravity != value )
		    {
		        win_attr.bit_gravity = value;
		        ((X11DRV_WND_DATA *) wnd->pDriverData)->bit_gravity = value;
		        TSXChangeWindowAttributes( display, w, CWBitGravity, &win_attr );
		    }
		   return TRUE;

	case HAK_ICONS:	/* called when the icons change */
	    if ( (wnd->flags & WIN_MANAGED) )
		X11DRV_WND_UpdateIconHints(wnd);
	    return TRUE;

	case HAK_ACCEPTFOCUS: /* called when a window is disabled/enabled */

		if( (wnd->flags & WIN_MANAGED) )
		    return X11DRV_SetWMHint( display, wnd, InputHint, value );
	}
    }
    return FALSE;
}

/***********************************************************************
 *		X11DRV_WND_IsSelfClipping
 */
BOOL X11DRV_WND_IsSelfClipping(WND *wndPtr)
{
  return X11DRV_WND_GetXWindow(wndPtr) != None;
}

/***********************************************************************
 *		X11DRV_WND_DockWindow
 *
 * Set the X Property of the window that tells the windowmanager we really
 * want to be in the systray
 *
 * KDE: set "KWM_DOCKWINDOW", type "KWM_DOCKWINDOW" to 1 before a window is 
 * 	mapped.
 *
 * all others: to be added ;)
 */
void X11DRV_WND_DockWindow(WND *wndPtr)
{ 
  int data = 1;
  Window win = X11DRV_WND_GetXWindow(wndPtr);
  if (kwmDockWindow == None) 
	  return; /* no KDE running */
  TSXChangeProperty(
    display,win,kwmDockWindow,kwmDockWindow,32,PropModeReplace,(char*)&data,1
  );
}


/***********************************************************************
 *		X11DRV_WND_SetWindowRgn
 *
 * Assign specified region to window (for non-rectangular windows)
 */
void X11DRV_WND_SetWindowRgn(WND *wndPtr, HRGN hrgnWnd)
{
#ifdef HAVE_LIBXSHAPE
    Window win = X11DRV_WND_GetXWindow(wndPtr);

    if (!win) return;

    if (!hrgnWnd)
    {
        TSXShapeCombineMask( display, win, ShapeBounding, 0, 0, None, ShapeSet );
    }
    else
    {
        XRectangle *aXRect;
        DWORD size;
        DWORD dwBufferSize = GetRegionData(hrgnWnd, 0, NULL);
        PRGNDATA pRegionData = HeapAlloc(GetProcessHeap(), 0, dwBufferSize);
        if (!pRegionData) return;

        GetRegionData(hrgnWnd, dwBufferSize, pRegionData);
        size = pRegionData->rdh.nCount;
        /* convert region's "Windows rectangles" to XRectangles */
        aXRect = HeapAlloc(GetProcessHeap(), 0, size * sizeof(*aXRect) );
        if (aXRect)
        {
            XRectangle* pCurrRect = aXRect;
            RECT *pRect = (RECT*) pRegionData->Buffer;
            for (; pRect < ((RECT*) pRegionData->Buffer) + size ; ++pRect, ++pCurrRect)
            {
                pCurrRect->x      = pRect->left;
                pCurrRect->y      = pRect->top;
                pCurrRect->height = pRect->bottom - pRect->top;
                pCurrRect->width  = pRect->right  - pRect->left;

                TRACE("Rectangle %04d of %04ld data: X=%04d, Y=%04d, Height=%04d, Width=%04d.\n",
                      pRect - (RECT*) pRegionData->Buffer,
                      size,
                      pCurrRect->x,
                      pCurrRect->y,
                      pCurrRect->height,
                      pCurrRect->width);
            }

            /* shape = non-rectangular windows (X11/extensions) */
            TSXShapeCombineRectangles( display, win, ShapeBounding, 
                                       0, 0, aXRect,
                                       pCurrRect - aXRect, ShapeSet, YXBanded );
            HeapFree(GetProcessHeap(), 0, aXRect );
        }
        HeapFree(GetProcessHeap(), 0, pRegionData);
    }
#endif  /* HAVE_LIBXSHAPE */
}
