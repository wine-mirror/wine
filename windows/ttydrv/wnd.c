/*
 * TTY window driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 */

#include "config.h"

#include "class.h"
#include "dc.h"
#include "heap.h"
#include "ttydrv.h"
#include "win.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_WND_GetCursesWindow
 *
 * Return the Curses window associated to a window.
 */
#ifdef HAVE_LIBCURSES
WINDOW *TTYDRV_WND_GetCursesWindow(WND *wndPtr)
{
    return wndPtr && wndPtr->pDriverData ? 
      ((TTYDRV_WND_DATA *) wndPtr->pDriverData)->window : 0;
}
#endif /* defined(HAVE_LIBCURSES) */

/***********************************************************************
 *              TTYDRV_WND_GetCursesRootWindow
 *
 * Return the Curses root window of the Curses window associated 
 * to a window.
 */
#ifdef HAVE_LIBCURSES
WINDOW *TTYDRV_WND_GetCursesRootWindow(WND *wndPtr)
{
  while(wndPtr->parent) wndPtr = wndPtr->parent;
  return TTYDRV_DESKTOP_GetCursesRootWindow((struct tagDESKTOP *) wndPtr->wExtra);
}
#endif /* defined(HAVE_LIBCURSES) */

/**********************************************************************
 *		TTYDRV_WND_Initialize
 */
void TTYDRV_WND_Initialize(WND *wndPtr)
{
  TTYDRV_WND_DATA *pWndDriverData = 
    (TTYDRV_WND_DATA *) HeapAlloc(SystemHeap, 0, sizeof(TTYDRV_WND_DATA));

  TRACE("(%p)\n", wndPtr);

  wndPtr->pDriverData = (void *) pWndDriverData;

#ifdef HAVE_LIBCURSES
  pWndDriverData->window = NULL;
#endif /* defined(HAVE_LIBCURSES) */
}

/**********************************************************************
 *		TTYDRV_WND_Finalize
 */
void TTYDRV_WND_Finalize(WND *wndPtr)
{
  TTYDRV_WND_DATA *pWndDriverData =
    (TTYDRV_WND_DATA *) wndPtr->pDriverData;

  TRACE("(%p)\n", wndPtr);

  if(!pWndDriverData) {
    ERR("WND already destroyed\n");
    return;
  }

#ifdef HAVE_LIBCURSES
  if(pWndDriverData->window) {
    ERR("WND destroyed without destroying the associated Curses Windows");
  }
#endif /* defined(HAVE_LIBCURSES) */

  HeapFree(SystemHeap, 0, pWndDriverData);
  wndPtr->pDriverData = NULL;
}

/**********************************************************************
 *		TTYDRV_WND_CreateDesktopWindow
 */
BOOL TTYDRV_WND_CreateDesktopWindow(WND *wndPtr, CLASS *classPtr, BOOL bUnicode)
{
  TTYDRV_WND_DATA *pWndDriverData =
    (TTYDRV_WND_DATA *) wndPtr->pDriverData;

  TRACE("(%p, %p, %d)\n", wndPtr, classPtr, bUnicode);

  if(!pWndDriverData) { ERR("WND never initialized\n"); return FALSE; }

#ifdef HAVE_LIBCURSES
  pWndDriverData->window = TTYDRV_WND_GetCursesRootWindow(wndPtr);
#endif /* defined(HAVE_LIBCURSES) */

  return TRUE;
}

/**********************************************************************
 *		TTYDRV_WND_CreateWindow
 */
BOOL TTYDRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCTA *cs, BOOL bUnicode)
{
#ifdef HAVE_LIBCURSES
  WINDOW *window, *rootWindow;
  INT cellWidth=8, cellHeight=8; /* FIXME: Hardcoded */

  TRACE("(%p, %p, %p, %d)\n", wndPtr, classPtr, cs, bUnicode);

  /* Only create top-level windows */
  if(cs->style & WS_CHILD)
    return TRUE;

  rootWindow = TTYDRV_WND_GetCursesRootWindow(wndPtr);

  window = subwin(rootWindow, cs->cy/cellHeight, cs->cx/cellWidth,
		  cs->y/cellHeight, cs->x/cellWidth);
  werase(window);
  wrefresh(window);
		  
  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p, %p, %p, %d): stub\n", wndPtr, classPtr, cs, bUnicode);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}

/***********************************************************************
 *		TTYDRV_WND_DestroyWindow
 */
BOOL TTYDRV_WND_DestroyWindow(WND *wndPtr)
{
#ifdef HAVE_LIBCURSES
  WINDOW *window;

  TRACE("(%p)\n", wndPtr);

  window = TTYDRV_WND_GetCursesWindow(wndPtr);
  if(window && window != TTYDRV_WND_GetCursesRootWindow(wndPtr)) {
    delwin(window);
  }

  return TRUE;
#else /* defined(HAVE_LIBCURSES) */
  FIXME("(%p): stub\n", wndPtr);

  return TRUE;
#endif /* defined(HAVE_LIBCURSES) */
}

/*****************************************************************
 *		TTYDRV_WND_SetParent
 */
WND *TTYDRV_WND_SetParent(WND *wndPtr, WND *pWndParent)
{
  FIXME("(%p, %p): stub\n", wndPtr, pWndParent);

  return NULL;
}

/***********************************************************************
 *		TTYDRV_WND_ForceWindowRaise
 */
void TTYDRV_WND_ForceWindowRaise(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/***********************************************************************
 *           TTYDRV_WINPOS_SetWindowPos
 */
void TTYDRV_WND_SetWindowPos(WND *wndPtr, const WINDOWPOS *winpos, BOOL bSMC_SETXPOS)
{
  FIXME("(%p, %p, %d): stub\n", wndPtr, winpos, bSMC_SETXPOS);
}

/*****************************************************************
 *		TTYDRV_WND_SetText
 */
void TTYDRV_WND_SetText(WND *wndPtr, LPCSTR text)
{
  FIXME("(%p, %s): stub\n", wndPtr, debugstr_a(text));
}

/*****************************************************************
 *		TTYDRV_WND_SetFocus
 */
void TTYDRV_WND_SetFocus(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		TTYDRV_WND_PreSizeMove
 */
void TTYDRV_WND_PreSizeMove(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		 TTYDRV_WND_PostSizeMove
 */
void TTYDRV_WND_PostSizeMove(WND *wndPtr)
{
  FIXME("(%p): stub\n", wndPtr);
}

/*****************************************************************
 *		 TTYDRV_WND_ScrollWindow
 */
void TTYDRV_WND_ScrollWindow(
  WND *wndPtr, DC *dcPtr, INT dx, INT dy, 
  const RECT *clipRect, BOOL bUpdate)
{
  FIXME("(%p, %p, %d, %d, %p, %d): stub\n", 
	wndPtr, dcPtr, dx, dy, clipRect, bUpdate);
}

/***********************************************************************
 *		TTYDRV_WND_SetDrawable
 */
void TTYDRV_WND_SetDrawable(WND *wndPtr, DC *dc, WORD flags, BOOL bSetClipOrigin)
{
  FIXME("(%p, %p, %d, %d): semistub\n", wndPtr, dc, flags, bSetClipOrigin);

  if (!wndPtr)  {
    /* Get a DC for the whole screen */
    dc->w.DCOrgX = 0;
    dc->w.DCOrgY = 0;
  } else {
    if (flags & DCX_WINDOW) {
      dc->w.DCOrgX = wndPtr->rectWindow.left;
      dc->w.DCOrgY = wndPtr->rectWindow.top;
    } else {
      dc->w.DCOrgX = wndPtr->rectClient.left;
      dc->w.DCOrgY = wndPtr->rectClient.top;
    }
  }
}

/***********************************************************************
 *              TTYDRV_WND_SetHostAttr
 */
BOOL TTYDRV_WND_SetHostAttr(WND *wndPtr, INT attr, INT value)
{
  FIXME("(%p): stub\n", wndPtr);

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_WND_IsSelfClipping
 */
BOOL TTYDRV_WND_IsSelfClipping(WND *wndPtr)
{
  FIXME("(%p): semistub\n", wndPtr);

  return FALSE;
}
