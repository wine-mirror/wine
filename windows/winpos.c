/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995, 1996 Alex Korobka
 */

#include <string.h>
#include "sysmetrics.h"
#include "heap.h"
#include "module.h"
#include "user.h"
#include "win.h"
#include "hook.h"
#include "message.h"
#include "queue.h"
#include "options.h"
#include "task.h"
#include "winpos.h"
#include "dce.h"
#include "nonclient.h"
#include "debug.h"
#include "x11drv.h"

#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

#define EMPTYPOINT(pt)          ((*(LONG*)&(pt)) == -1)

#define PLACE_MIN		0x0001
#define PLACE_MAX		0x0002
#define PLACE_RECT		0x0004

#define SMC_NOCOPY		0x0001
#define SMC_NOPARENTERASE	0x0002
#define SMC_DRAWFRAME		0x0004
#define SMC_SETXPOS		0x0008

/* ----- internal variables ----- */

static HWND32 hwndActive      = 0;  /* Currently active window */
static HWND32 hwndPrevActive  = 0;  /* Previously active window */
static HWND32 hGlobalShellWindow=0; /*the shell*/

static LPCSTR atomInternalPos;

extern MESSAGEQUEUE* pActiveQueue;

/***********************************************************************
 *           WINPOS_CreateInternalPosAtom
 */
BOOL32 WINPOS_CreateInternalPosAtom()
{
    LPSTR str = "SysIP";
    atomInternalPos = (LPCSTR)(DWORD)GlobalAddAtom32A(str);
    return (atomInternalPos) ? TRUE : FALSE;
}

/***********************************************************************
 *           WINPOS_CheckInternalPos
 *
 * Called when a window is destroyed.
 */
void WINPOS_CheckInternalPos( HWND32 hwnd )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS) GetProp32A( hwnd, atomInternalPos );

    if( hwnd == hwndPrevActive ) hwndPrevActive = 0;
    if( hwnd == hwndActive )
    {
	hwndActive = 0; 
	WARN(win, "\tattempt to activate destroyed window!\n");
    }

    if( lpPos )
    {
	if( IsWindow32(lpPos->hwndIconTitle) ) 
	    DestroyWindow32( lpPos->hwndIconTitle );
	HeapFree( SystemHeap, 0, lpPos );
    }
}

/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 */
static POINT16 WINPOS_FindIconPos( WND* wndPtr, POINT16 pt )
{
    RECT16 rectParent;
    short x, y, xspacing, yspacing;

    GetClientRect16( wndPtr->parent->hwndSelf, &rectParent );
    if ((pt.x >= rectParent.left) && (pt.x + SYSMETRICS_CXICON < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + SYSMETRICS_CYICON < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    xspacing = SYSMETRICS_CXICONSPACING;
    yspacing = SYSMETRICS_CYICONSPACING;

    y = rectParent.bottom;
    for (;;)
    {
        for (x = rectParent.left; x <= rectParent.right-xspacing; x += xspacing)
        {
              /* Check if another icon already occupies this spot */
            WND *childPtr = wndPtr->parent->child;
            while (childPtr)
            {
                if ((childPtr->dwStyle & WS_MINIMIZE) && (childPtr != wndPtr))
                {
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                        break;  /* There's a window in there */
                }
                childPtr = childPtr->next;
            }
            if (!childPtr) /* No window was found, so it's OK for us */
            {
		pt.x = x + (xspacing - SYSMETRICS_CXICON) / 2;
		pt.y = y - (yspacing + SYSMETRICS_CYICON) / 2;
		return pt;
            }
        }
        y -= yspacing;
    }
}


/***********************************************************************
 *           ArrangeIconicWindows16   (USER.170)
 */
UINT16 WINAPI ArrangeIconicWindows16( HWND16 parent) 
{
    return ArrangeIconicWindows32(parent);
}
/***********************************************************************
 *           ArrangeIconicWindows32   (USER32.7)
 */
UINT32 WINAPI ArrangeIconicWindows32( HWND32 parent )
{
    RECT32 rectParent;
    HWND32 hwndChild;
    INT32 x, y, xspacing, yspacing;

    GetClientRect32( parent, &rectParent );
    x = rectParent.left;
    y = rectParent.bottom;
    xspacing = SYSMETRICS_CXICONSPACING;
    yspacing = SYSMETRICS_CYICONSPACING;

    hwndChild = GetWindow32( parent, GW_CHILD );
    while (hwndChild)
    {
        if( IsIconic32( hwndChild ) )
        {
	    WINPOS_ShowIconTitle( WIN_FindWndPtr(hwndChild), FALSE );
            SetWindowPos32( hwndChild, 0, x + (xspacing - SYSMETRICS_CXICON) / 2,
                            y - yspacing - SYSMETRICS_CYICON/2, 0, 0,
                            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	    if( IsWindow32(hwndChild) )
	        WINPOS_ShowIconTitle( WIN_FindWndPtr(hwndChild), TRUE );
            if (x <= rectParent.right - xspacing) x += xspacing;
            else
            {
                x = rectParent.left;
                y -= yspacing;
            }
        }
        hwndChild = GetWindow32( hwndChild, GW_HWNDNEXT );
    }
    return yspacing;
}


/***********************************************************************
 *             SwitchToThisWindow16   (USER.172)
 */
void WINAPI SwitchToThisWindow16( HWND16 hwnd, BOOL16 restore )
{
    SwitchToThisWindow32( hwnd, restore );
}


/***********************************************************************
 *             SwitchToThisWindow32   (USER32.539)
 */
void WINAPI SwitchToThisWindow32( HWND32 hwnd, BOOL32 restore )
{
    ShowWindow32( hwnd, restore ? SW_RESTORE : SW_SHOWMINIMIZED );
}


/***********************************************************************
 *           GetWindowRect16   (USER.32)
 */
void WINAPI GetWindowRect16( HWND16 hwnd, LPRECT16 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    CONV_RECT32TO16( &wndPtr->rectWindow, rect );
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints16( wndPtr->parent->hwndSelf, 0, (POINT16 *)rect, 2 );
}


/***********************************************************************
 *           GetWindowRect32   (USER32.308)
 */
BOOL32 WINAPI GetWindowRect32( HWND32 hwnd, LPRECT32 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return FALSE;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints32( wndPtr->parent->hwndSelf, 0, (POINT32 *)rect, 2 );
    return TRUE;
}


/***********************************************************************
 *           GetWindowRgn32 
 */
BOOL32 WINAPI GetWindowRgn32 ( HWND32 hwnd, HRGN32 hrgn )

{
  RECT32    rect;
  WND * wndPtr = WIN_FindWndPtr( hwnd ); 
  if (!wndPtr) return (ERROR);

  FIXME (win, "GetWindowRgn32: doesn't really do regions\n"); 
  
  memset (&rect, 0, sizeof(rect));

  GetWindowRect32 ( hwnd, &rect );

  FIXME (win, "Check whether a valid region here\n");

  SetRectRgn32 ( hrgn, rect.left, rect.top, rect.right, rect.bottom );

  return (SIMPLEREGION);
}

/***********************************************************************
 *           SetWindowRgn32 
 */
INT32 WINAPI SetWindowRgn32( HWND32 hwnd, HRGN32 hrgn,BOOL32 bRedraw)

{

  FIXME (win, "SetWindowRgn32: stub\n"); 
  return TRUE;
}

/***********************************************************************
 *           SetWindowRgn16 
 */
INT16 WINAPI SetWindowRgn16( HWND16 hwnd, HRGN16 hrgn,BOOL16 bRedraw)

{

  FIXME (win, "SetWindowRgn16: stub\n"); 
  return TRUE;
}


/***********************************************************************
 *           GetClientRect16   (USER.33)
 */
void WINAPI GetClientRect16( HWND16 hwnd, LPRECT16 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	rect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	rect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
}


/***********************************************************************
 *           GetClientRect32   (USER32.220)
 */
void WINAPI GetClientRect32( HWND32 hwnd, LPRECT32 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    rect->left = rect->top = rect->right = rect->bottom = 0;
    if (wndPtr) 
    {
	rect->right  = wndPtr->rectClient.right - wndPtr->rectClient.left;
	rect->bottom = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
    }
}


/*******************************************************************
 *         ClientToScreen16   (USER.28)
 */
void WINAPI ClientToScreen16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( hwnd, 0, lppnt, 1 );
}


/*******************************************************************
 *         ClientToScreen32   (USER32.52)
 */
BOOL32 WINAPI ClientToScreen32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( hwnd, 0, lppnt, 1 );
    return TRUE;
}


/*******************************************************************
 *         ScreenToClient16   (USER.29)
 */
void WINAPI ScreenToClient16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( 0, hwnd, lppnt, 1 );
}


/*******************************************************************
 *         ScreenToClient32   (USER32.447)
 */
void WINAPI ScreenToClient32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( 0, hwnd, lppnt, 1 );
}


/***********************************************************************
 *           WINPOS_WindowFromPoint
 *
 * Find the window and hittest for a given point.
 */
INT16 WINPOS_WindowFromPoint( WND* wndScope, POINT16 pt, WND **ppWnd )
{
    WND *wndPtr;
    INT16 hittest = HTERROR;
    POINT16 xy = pt;

   *ppWnd = NULL;
    wndPtr = wndScope->child;
    if( wndScope->flags & WIN_MANAGED )
    {
	/* this prevents mouse clicks from going "through" scrollbars in managed mode */
	if( pt.x < wndScope->rectClient.left || pt.x >= wndScope->rectClient.right ||
	    pt.y < wndScope->rectClient.top || pt.y >= wndScope->rectClient.bottom )
	    goto hittest;
    }
    MapWindowPoints16( GetDesktopWindow16(), wndScope->hwndSelf, &xy, 1 );

    for (;;)
    {
        while (wndPtr)
        {
            /* If point is in window, and window is visible, and it  */
            /* is enabled (or it's a top-level window), then explore */
            /* its children. Otherwise, go to the next window.       */

            if ((wndPtr->dwStyle & WS_VISIBLE) &&
                (!(wndPtr->dwStyle & WS_DISABLED) ||
                 ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)) &&
                (xy.x >= wndPtr->rectWindow.left) &&
                (xy.x < wndPtr->rectWindow.right) &&
                (xy.y >= wndPtr->rectWindow.top) &&
                (xy.y < wndPtr->rectWindow.bottom))
            {
                *ppWnd = wndPtr;  /* Got a suitable window */

                /* If window is minimized or disabled, return at once */
                if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;
                if (wndPtr->dwStyle & WS_DISABLED) return HTERROR;

                /* If point is not in client area, ignore the children */
                if ((xy.x < wndPtr->rectClient.left) ||
                    (xy.x >= wndPtr->rectClient.right) ||
                    (xy.y < wndPtr->rectClient.top) ||
                    (xy.y >= wndPtr->rectClient.bottom)) break;

                xy.x -= wndPtr->rectClient.left;
                xy.y -= wndPtr->rectClient.top;
                wndPtr = wndPtr->child;
            }
            else wndPtr = wndPtr->next;
        }

hittest:
        /* If nothing found, try the scope window */
        if (!*ppWnd) *ppWnd = wndScope;

        /* Send the WM_NCHITTEST message (only if to the same task) */
        if ((*ppWnd)->hmemTaskQ == GetFastQueue())
	{
            hittest = (INT16)SendMessage16( (*ppWnd)->hwndSelf, WM_NCHITTEST, 
						 0, MAKELONG( pt.x, pt.y ) );
	    if (hittest != HTTRANSPARENT) return hittest;  /* Found the window */
	}
	else return HTCLIENT;

        /* If no children found in last search, make point relative to parent */
        if (!wndPtr)
        {
            xy.x += (*ppWnd)->rectClient.left;
            xy.y += (*ppWnd)->rectClient.top;
        }

        /* Restart the search from the next sibling */
        wndPtr = (*ppWnd)->next;
        *ppWnd = (*ppWnd)->parent;
    }
}


/*******************************************************************
 *         WindowFromPoint16   (USER.30)
 */
HWND16 WINAPI WindowFromPoint16( POINT16 pt )
{
    WND *pWnd;
    WINPOS_WindowFromPoint( WIN_GetDesktop(), pt, &pWnd );
    return pWnd->hwndSelf;
}


/*******************************************************************
 *         WindowFromPoint32   (USER32.582)
 */
HWND32 WINAPI WindowFromPoint32( POINT32 pt )
{
    WND *pWnd;
    POINT16 pt16;
    CONV_POINT32TO16( &pt, &pt16 );
    WINPOS_WindowFromPoint( WIN_GetDesktop(), pt16, &pWnd );
    return (HWND32)pWnd->hwndSelf;
}


/*******************************************************************
 *         ChildWindowFromPoint16   (USER.191)
 */
HWND16 WINAPI ChildWindowFromPoint16( HWND16 hwndParent, POINT16 pt )
{
    POINT32 pt32;
    CONV_POINT16TO32( &pt, &pt32 );
    return (HWND16)ChildWindowFromPoint32( hwndParent, pt32 );
}


/*******************************************************************
 *         ChildWindowFromPoint32   (USER32.49)
 */
HWND32 WINAPI ChildWindowFromPoint32( HWND32 hwndParent, POINT32 pt )
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT32 rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect32( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect32( &wnd->rectWindow, pt )) return wnd->hwndSelf;
        wnd = wnd->next;
    }
    return hwndParent;
}

/*******************************************************************
 *         ChildWindowFromPointEx16   (USER.50)
 */
HWND16 WINAPI ChildWindowFromPointEx16( HWND16 hwndParent, POINT16 pt, UINT16 uFlags)
{
    POINT32 pt32;
    CONV_POINT16TO32( &pt, &pt32 );
    return (HWND16)ChildWindowFromPointEx32( hwndParent, pt32, uFlags );
}


/*******************************************************************
 *         ChildWindowFromPointEx32   (USER32.50)
 */
HWND32 WINAPI ChildWindowFromPointEx32( HWND32 hwndParent, POINT32 pt,
		UINT32 uFlags)
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT32 rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect32( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect32( &wnd->rectWindow, pt )) {
		if ( (uFlags & CWP_SKIPINVISIBLE) && 
				!(wnd->dwStyle & WS_VISIBLE) )
		        wnd = wnd->next;
		else if ( (uFlags & CWP_SKIPDISABLED) && 
				(wnd->dwStyle & WS_DISABLED) )
		        wnd = wnd->next;
		else if ( (uFlags & CWP_SKIPTRANSPARENT) && 
				(wnd->dwExStyle & WS_EX_TRANSPARENT) )
		        wnd = wnd->next;
		else
			return wnd->hwndSelf;
	}
    }
    return hwndParent;
}


/*******************************************************************
 *         WINPOS_GetWinOffset
 *
 * Calculate the offset between the origin of the two windows. Used
 * to implement MapWindowPoints.
 */
static void WINPOS_GetWinOffset( HWND32 hwndFrom, HWND32 hwndTo,
                                 POINT32 *offset )
{
    WND * wndPtr;

    offset->x = offset->y = 0;
    if (hwndFrom == hwndTo ) return;

      /* Translate source window origin to screen coords */
    if (hwndFrom)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndFrom )))
        {
            ERR(win,"bad hwndFrom = %04x\n",hwndFrom);
            return;
        }
        while (wndPtr->parent)
        {
            offset->x += wndPtr->rectClient.left;
            offset->y += wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }
    }

      /* Translate origin to destination window coords */
    if (hwndTo)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwndTo )))
        {
            ERR(win,"bad hwndTo = %04x\n", hwndTo );
            return;
        }
        while (wndPtr->parent)
        {
            offset->x -= wndPtr->rectClient.left;
            offset->y -= wndPtr->rectClient.top;
            wndPtr = wndPtr->parent;
        }    
    }
}


/*******************************************************************
 *         MapWindowPoints16   (USER.258)
 */
void WINAPI MapWindowPoints16( HWND16 hwndFrom, HWND16 hwndTo,
                               LPPOINT16 lppt, UINT16 count )
{
    POINT32 offset;

    WINPOS_GetWinOffset( hwndFrom, hwndTo, &offset );
    while (count--)
    {
	lppt->x += offset.x;
	lppt->y += offset.y;
        lppt++;
    }
}


/*******************************************************************
 *         MapWindowPoints32   (USER32.386)
 */
void WINAPI MapWindowPoints32( HWND32 hwndFrom, HWND32 hwndTo,
                               LPPOINT32 lppt, UINT32 count )
{
    POINT32 offset;

    WINPOS_GetWinOffset( hwndFrom, hwndTo, &offset );
    while (count--)
    {
	lppt->x += offset.x;
	lppt->y += offset.y;
        lppt++;
    }
}


/***********************************************************************
 *           IsIconic16   (USER.31)
 */
BOOL16 WINAPI IsIconic16(HWND16 hWnd)
{
    return IsIconic32(hWnd);
}


/***********************************************************************
 *           IsIconic32   (USER32.345)
 */
BOOL32 WINAPI IsIconic32(HWND32 hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MINIMIZE) != 0;
}
 
 
/***********************************************************************
 *           IsZoomed   (USER.272)
 */
BOOL16 WINAPI IsZoomed16(HWND16 hWnd)
{
    return IsZoomed32(hWnd);
}


/***********************************************************************
 *           IsZoomed   (USER32.352)
 */
BOOL32 WINAPI IsZoomed32(HWND32 hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *         GetActiveWindow    (USER.60)
 */
HWND16 WINAPI GetActiveWindow16(void)
{
    return (HWND16)hwndActive;
}

/*******************************************************************
 *         GetActiveWindow    (USER32.205)
 */
HWND32 WINAPI GetActiveWindow32(void)
{
    return (HWND32)hwndActive;
}


/*******************************************************************
 *         WINPOS_CanActivate
 */
static BOOL32 WINPOS_CanActivate(WND* pWnd)
{
    if( pWnd && ((pWnd->dwStyle & (WS_DISABLED | WS_VISIBLE | WS_CHILD)) 
	== WS_VISIBLE) ) return TRUE;
    return FALSE;
}


/*******************************************************************
 *         SetActiveWindow16    (USER.59)
 */
HWND16 WINAPI SetActiveWindow16( HWND16 hwnd )
{
    return SetActiveWindow32(hwnd);
}


/*******************************************************************
 *         SetActiveWindow32    (USER32.463)
 */
HWND32 WINAPI SetActiveWindow32( HWND32 hwnd )
{
    HWND32 prev = hwndActive;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if ( !WINPOS_CanActivate(wndPtr) ) return 0;

    WINPOS_SetActiveWindow( hwnd, 0, 0 );
    return prev;
}


/*******************************************************************
 *         GetForegroundWindow16    (USER.608)
 */
HWND16 WINAPI GetForegroundWindow16(void)
{
    return (HWND16)GetForegroundWindow32();
}


/*******************************************************************
 *         SetForegroundWindow16    (USER.609)
 */
BOOL16 WINAPI SetForegroundWindow16( HWND16 hwnd )
{
    return SetForegroundWindow32( hwnd );
}


/*******************************************************************
 *         GetForegroundWindow32    (USER32.241)
 */
HWND32 WINAPI GetForegroundWindow32(void)
{
    return GetActiveWindow32();
}


/*******************************************************************
 *         SetForegroundWindow32    (USER32.482)
 */
BOOL32 WINAPI SetForegroundWindow32( HWND32 hwnd )
{
    SetActiveWindow32( hwnd );
    return TRUE;
}


/*******************************************************************
 *         GetShellWindow16    (USER.600)
 */
HWND16 WINAPI GetShellWindow16(void)
{
    return GetShellWindow32();
}

/*******************************************************************
 *         SetShellWindow32    (USER32.504)
 */
HWND32 WINAPI SetShellWindow32(HWND32 hwndshell)
{   WARN(win, "(hWnd=%08x) semi stub\n",hwndshell );

    hGlobalShellWindow = hwndshell;
    return hGlobalShellWindow;
}


/*******************************************************************
 *         GetShellWindow32    (USER32.287)
 */
HWND32 WINAPI GetShellWindow32(void)
{   WARN(win, "(hWnd=%x) semi stub\n",hGlobalShellWindow );

    return hGlobalShellWindow;
}


/***********************************************************************
 *           BringWindowToTop16   (USER.45)
 */
BOOL16 WINAPI BringWindowToTop16( HWND16 hwnd )
{
    return BringWindowToTop32(hwnd);
}


/***********************************************************************
 *           BringWindowToTop32   (USER32.11)
 */
BOOL32 WINAPI BringWindowToTop32( HWND32 hwnd )
{
    return SetWindowPos32( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
}


/***********************************************************************
 *           MoveWindow16   (USER.56)
 */
BOOL16 WINAPI MoveWindow16( HWND16 hwnd, INT16 x, INT16 y, INT16 cx, INT16 cy,
                            BOOL16 repaint )
{
    return MoveWindow32(hwnd,x,y,cx,cy,repaint);
}


/***********************************************************************
 *           MoveWindow32   (USER32.399)
 */
BOOL32 WINAPI MoveWindow32( HWND32 hwnd, INT32 x, INT32 y, INT32 cx, INT32 cy,
                            BOOL32 repaint )
{    
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
    TRACE(win, "%04x %d,%d %dx%d %d\n", 
	    hwnd, x, y, cx, cy, repaint );
    return SetWindowPos32( hwnd, 0, x, y, cx, cy, flags );
}

/***********************************************************************
 *           WINPOS_InitInternalPos
 */
static LPINTERNALPOS WINPOS_InitInternalPos( WND* wnd, POINT32 pt, 
					     LPRECT32 restoreRect )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS) GetProp32A( wnd->hwndSelf,
                                                      atomInternalPos );
    if( !lpPos )
    {
	/* this happens when the window is minimized/maximized 
	 * for the first time (rectWindow is not adjusted yet) */

	lpPos = HeapAlloc( SystemHeap, 0, sizeof(INTERNALPOS) );
	if( !lpPos ) return NULL;

	SetProp32A( wnd->hwndSelf, atomInternalPos, (HANDLE32)lpPos );
	lpPos->hwndIconTitle = 0; /* defer until needs to be shown */
        CONV_RECT32TO16( &wnd->rectWindow, &lpPos->rectNormal );
	*(UINT32*)&lpPos->ptIconPos = *(UINT32*)&lpPos->ptMaxPos = 0xFFFFFFFF;
    }

    if( wnd->dwStyle & WS_MINIMIZE ) 
	CONV_POINT32TO16( &pt, &lpPos->ptIconPos );
    else if( wnd->dwStyle & WS_MAXIMIZE ) 
	CONV_POINT32TO16( &pt, &lpPos->ptMaxPos );
    else if( restoreRect ) 
	CONV_RECT32TO16( restoreRect, &lpPos->rectNormal );

    return lpPos;
}

/***********************************************************************
 *           WINPOS_RedrawIconTitle
 */
BOOL32 WINPOS_RedrawIconTitle( HWND32 hWnd )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS)GetProp32A( hWnd, atomInternalPos );
    if( lpPos )
    {
	if( lpPos->hwndIconTitle )
	{
	    SendMessage32A( lpPos->hwndIconTitle, WM_SHOWWINDOW, TRUE, 0);
	    InvalidateRect32( lpPos->hwndIconTitle, NULL, TRUE );
	    return TRUE;
	}
    }
    return FALSE;
}

/***********************************************************************
 *           WINPOS_ShowIconTitle
 */
BOOL32 WINPOS_ShowIconTitle( WND* pWnd, BOOL32 bShow )
{
    LPINTERNALPOS lpPos = (LPINTERNALPOS)GetProp32A( pWnd->hwndSelf, atomInternalPos );

    if( lpPos && !(pWnd->flags & WIN_MANAGED))
    {
	HWND16 hWnd = lpPos->hwndIconTitle;

	TRACE(win,"0x%04x %i\n", pWnd->hwndSelf, (bShow != 0) );

	if( !hWnd )
	    lpPos->hwndIconTitle = hWnd = ICONTITLE_Create( pWnd );
	if( bShow )
        {
	    pWnd = WIN_FindWndPtr(hWnd);

	    if( !(pWnd->dwStyle & WS_VISIBLE) )
	    {
		SendMessage32A( hWnd, WM_SHOWWINDOW, TRUE, 0 );
		SetWindowPos32( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
			        SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
	    }
	}
	else ShowWindow32( hWnd, SW_HIDE );
    }
    return FALSE;
}

/*******************************************************************
 *           WINPOS_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
void WINPOS_GetMinMaxInfo( WND *wndPtr, POINT32 *maxSize, POINT32 *maxPos,
			   POINT32 *minTrack, POINT32 *maxTrack )
{
    LPINTERNALPOS lpPos;
    MINMAXINFO32 MinMax;
    INT32 xinc, yinc;

    /* Compute default values */

    MinMax.ptMaxSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxSize.y = SYSMETRICS_CYSCREEN;
    MinMax.ptMinTrackSize.x = SYSMETRICS_CXMINTRACK;
    MinMax.ptMinTrackSize.y = SYSMETRICS_CYMINTRACK;
    MinMax.ptMaxTrackSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxTrackSize.y = SYSMETRICS_CYSCREEN;

    if (wndPtr->flags & WIN_MANAGED) xinc = yinc = 0;
    else if (HAS_DLGFRAME( wndPtr->dwStyle, wndPtr->dwExStyle ))
    {
        xinc = SYSMETRICS_CXDLGFRAME;
        yinc = SYSMETRICS_CYDLGFRAME;
    }
    else
    {
        xinc = yinc = 0;
        if (HAS_THICKFRAME(wndPtr->dwStyle))
        {
            xinc += SYSMETRICS_CXFRAME;
            yinc += SYSMETRICS_CYFRAME;
        }
        if (wndPtr->dwStyle & WS_BORDER)
        {
            xinc += SYSMETRICS_CXBORDER;
            yinc += SYSMETRICS_CYBORDER;
        }
    }
    MinMax.ptMaxSize.x += 2 * xinc;
    MinMax.ptMaxSize.y += 2 * yinc;

    lpPos = (LPINTERNALPOS)GetProp32A( wndPtr->hwndSelf, atomInternalPos );
    if( lpPos && !EMPTYPOINT(lpPos->ptMaxPos) )
	CONV_POINT16TO32( &lpPos->ptMaxPos, &MinMax.ptMaxPosition );
    else
    {
        MinMax.ptMaxPosition.x = -xinc;
        MinMax.ptMaxPosition.y = -yinc;
    }

    SendMessage32A( wndPtr->hwndSelf, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax );

      /* Some sanity checks */

    TRACE(win,"%d %d / %d %d / %d %d / %d %d\n",
                      MinMax.ptMaxSize.x, MinMax.ptMaxSize.y,
                      MinMax.ptMaxPosition.x, MinMax.ptMaxPosition.y,
                      MinMax.ptMaxTrackSize.x, MinMax.ptMaxTrackSize.y,
                      MinMax.ptMinTrackSize.x, MinMax.ptMinTrackSize.y);
    MinMax.ptMaxTrackSize.x = MAX( MinMax.ptMaxTrackSize.x,
                                   MinMax.ptMinTrackSize.x );
    MinMax.ptMaxTrackSize.y = MAX( MinMax.ptMaxTrackSize.y,
                                   MinMax.ptMinTrackSize.y );

    if (maxSize) *maxSize = MinMax.ptMaxSize;
    if (maxPos) *maxPos = MinMax.ptMaxPosition;
    if (minTrack) *minTrack = MinMax.ptMinTrackSize;
    if (maxTrack) *maxTrack = MinMax.ptMaxTrackSize;
}

/***********************************************************************
 *           WINPOS_MinMaximize
 *
 * Fill in lpRect and return additional flags to be used with SetWindowPos().
 * This function assumes that 'cmd' is different from the current window
 * state.
 */
UINT16 WINPOS_MinMaximize( WND* wndPtr, UINT16 cmd, LPRECT16 lpRect )
{
    UINT16 swpFlags = 0;
    POINT32 pt;
    POINT32 size = { wndPtr->rectWindow.left, wndPtr->rectWindow.top };
    LPINTERNALPOS lpPos = WINPOS_InitInternalPos( wndPtr, size,
                                                  &wndPtr->rectWindow );

    TRACE(win,"0x%04x %u\n", wndPtr->hwndSelf, cmd );

    if (lpPos && !HOOK_CallHooks16(WH_CBT, HCBT_MINMAX, wndPtr->hwndSelf, cmd))
    {
	if( wndPtr->dwStyle & WS_MINIMIZE )
	{
	    if( !SendMessage32A( wndPtr->hwndSelf, WM_QUERYOPEN, 0, 0L ) )
		return (SWP_NOSIZE | SWP_NOMOVE);
	    swpFlags |= SWP_NOCOPYBITS;
	}
	switch( cmd )
	{
	    case SW_MINIMIZE:
		 if( wndPtr->dwStyle & WS_MAXIMIZE)
		 {
		     wndPtr->flags |= WIN_RESTORE_MAX;
		     wndPtr->dwStyle &= ~WS_MAXIMIZE;
                 }
                 else
		     wndPtr->flags &= ~WIN_RESTORE_MAX;
		 wndPtr->dwStyle |= WS_MINIMIZE;

		 lpPos->ptIconPos = WINPOS_FindIconPos( wndPtr, lpPos->ptIconPos );

		 SetRect16( lpRect, lpPos->ptIconPos.x, lpPos->ptIconPos.y,
				    SYSMETRICS_CXICON, SYSMETRICS_CYICON );
		 swpFlags |= SWP_NOCOPYBITS;
		 break;

	    case SW_MAXIMIZE:
                CONV_POINT16TO32( &lpPos->ptMaxPos, &pt );
                WINPOS_GetMinMaxInfo( wndPtr, &size, &pt, NULL, NULL );
                CONV_POINT32TO16( &pt, &lpPos->ptMaxPos );

		 if( wndPtr->dwStyle & WS_MINIMIZE )
		 {
		     WINPOS_ShowIconTitle( wndPtr, FALSE );
		     wndPtr->dwStyle &= ~WS_MINIMIZE;
		 }
                 wndPtr->dwStyle |= WS_MAXIMIZE;

		 SetRect16( lpRect, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y,
				    size.x, size.y );
		 break;

	    case SW_RESTORE:
		 if( wndPtr->dwStyle & WS_MINIMIZE )
		 {
		     wndPtr->dwStyle &= ~WS_MINIMIZE;
		     WINPOS_ShowIconTitle( wndPtr, FALSE );
		     if( wndPtr->flags & WIN_RESTORE_MAX)
		     {
			 /* Restore to maximized position */
                         CONV_POINT16TO32( &lpPos->ptMaxPos, &pt );
                         WINPOS_GetMinMaxInfo( wndPtr, &size, &pt, NULL, NULL);
                         CONV_POINT32TO16( &pt, &lpPos->ptMaxPos );
			 wndPtr->dwStyle |= WS_MAXIMIZE;
			 SetRect16( lpRect, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y, size.x, size.y );
			 break;
		     }
		 } 
		 else 
		     if( !(wndPtr->dwStyle & WS_MAXIMIZE) ) return (UINT16)(-1);
 		     else wndPtr->dwStyle &= ~WS_MAXIMIZE;

		 /* Restore to normal position */

		*lpRect = lpPos->rectNormal; 
		 lpRect->right -= lpRect->left; 
		 lpRect->bottom -= lpRect->top;

		 break;
	}
    } else swpFlags |= SWP_NOSIZE | SWP_NOMOVE;
    return swpFlags;
}

/***********************************************************************
 *           ShowWindowAsync32   (USER32.535)
 *
 * doesn't wait; returns immediately.
 * used by threads to toggle windows in other (possibly hanging) threads
 */
BOOL32 WINAPI ShowWindowAsync32( HWND32 hwnd, INT32 cmd )
{
    /* FIXME: does ShowWindow32() return immediately ? */
    return ShowWindow32(hwnd, cmd);
}


/***********************************************************************
 *           ShowWindow16   (USER.42)
 */
BOOL16 WINAPI ShowWindow16( HWND16 hwnd, INT16 cmd ) 
{    
    return ShowWindow32(hwnd,cmd);
}


/***********************************************************************
 *           ShowWindow32   (USER32.534)
 */
BOOL32 WINAPI ShowWindow32( HWND32 hwnd, INT32 cmd ) 
{    
    WND* 	wndPtr = WIN_FindWndPtr( hwnd );
    BOOL32 	wasVisible, showFlag;
    RECT16 	newPos = {0, 0, 0, 0};
    int 	swp = 0;

    if (!wndPtr) return FALSE;

    TRACE(win,"hwnd=%04x, cmd=%d\n", hwnd, cmd);

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
	    swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swp |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swp |= SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MINIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_MINIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_MAXIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOW:
	    swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWNOACTIVATE:
            swp |= SWP_NOZORDER;
            if (GetActiveWindow32()) swp |= SWP_NOACTIVATE;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	case SW_RESTORE:
	    swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;

            if( wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_RESTORE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible)
    {
        SendMessage32A( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow32( hwnd )) return wasVisible;
    }

    if ((wndPtr->dwStyle & WS_CHILD) &&
        !IsWindowVisible32( wndPtr->parent->hwndSelf ) &&
        (swp & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE) )
    {
        /* Don't call SetWindowPos32() on invisible child windows */
        if (cmd == SW_HIDE) wndPtr->dwStyle &= ~WS_VISIBLE;
        else wndPtr->dwStyle |= WS_VISIBLE;
    }
    else
    {
        /* We can't activate a child window */
        if (wndPtr->dwStyle & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        SetWindowPos32( hwnd, HWND_TOP, 
			newPos.left, newPos.top, newPos.right, newPos.bottom, swp );
        if (!IsWindow32( hwnd )) return wasVisible;
	else if( wndPtr->dwStyle & WS_MINIMIZE ) WINPOS_ShowIconTitle( wndPtr, TRUE );
    }

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
	SendMessage32A( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessage32A( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

    return wasVisible;
}


/***********************************************************************
 *           GetInternalWindowPos16   (USER.460)
 */
UINT16 WINAPI GetInternalWindowPos16( HWND16 hwnd, LPRECT16 rectWnd,
                                      LPPOINT16 ptIcon )
{
    WINDOWPLACEMENT16 wndpl;
    if (GetWindowPlacement16( hwnd, &wndpl )) 
    {
	if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
	if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
	return wndpl.showCmd;
    }
    return 0;
}


/***********************************************************************
 *           GetInternalWindowPos32   (USER32.245)
 */
UINT32 WINAPI GetInternalWindowPos32( HWND32 hwnd, LPRECT32 rectWnd,
                                      LPPOINT32 ptIcon )
{
    WINDOWPLACEMENT32 wndpl;
    if (GetWindowPlacement32( hwnd, &wndpl ))
    {
	if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
	if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
	return wndpl.showCmd;
    }
    return 0;
}

/***********************************************************************
 *           GetWindowPlacement16   (USER.370)
 */
BOOL16 WINAPI GetWindowPlacement16( HWND16 hwnd, WINDOWPLACEMENT16 *wndpl )
{
    WND *pWnd = WIN_FindWndPtr( hwnd );
    if( pWnd )
    {
	LPINTERNALPOS lpPos = (LPINTERNALPOS)WINPOS_InitInternalPos( pWnd,
			     *(LPPOINT32)&pWnd->rectWindow.left, &pWnd->rectWindow );
	wndpl->length  = sizeof(*wndpl);
	if( pWnd->dwStyle & WS_MINIMIZE )
	    wndpl->showCmd = SW_SHOWMINIMIZED;
	else 
	    wndpl->showCmd = ( pWnd->dwStyle & WS_MAXIMIZE )
			     ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL ;
	if( pWnd->flags & WIN_RESTORE_MAX )
	    wndpl->flags = WPF_RESTORETOMAXIMIZED;
	else
	    wndpl->flags = 0;
	wndpl->ptMinPosition = lpPos->ptIconPos;
	wndpl->ptMaxPosition = lpPos->ptMaxPos;
	wndpl->rcNormalPosition = lpPos->rectNormal;
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           GetWindowPlacement32   (USER32.307)
 */
BOOL32 WINAPI GetWindowPlacement32( HWND32 hwnd, WINDOWPLACEMENT32 *pwpl32 )
{
    if( pwpl32 )
    {
	WINDOWPLACEMENT16 wpl;
	wpl.length = sizeof(wpl);
        if( GetWindowPlacement16( hwnd, &wpl ) )
	{
	    pwpl32->length = sizeof(*pwpl32);
	    pwpl32->flags = wpl.flags;
	    pwpl32->showCmd = wpl.showCmd;
	    CONV_POINT16TO32( &wpl.ptMinPosition, &pwpl32->ptMinPosition );
	    CONV_POINT16TO32( &wpl.ptMaxPosition, &pwpl32->ptMaxPosition );
	    CONV_RECT16TO32( &wpl.rcNormalPosition, &pwpl32->rcNormalPosition );
	    return TRUE;
	}
    }
    return FALSE;
}


/***********************************************************************
 *           WINPOS_SetPlacement
 */
static BOOL32 WINPOS_SetPlacement( HWND32 hwnd, const WINDOWPLACEMENT16 *wndpl,
						UINT32 flags )
{
    WND *pWnd = WIN_FindWndPtr( hwnd );
    if( pWnd )
    {
	LPINTERNALPOS lpPos = (LPINTERNALPOS)WINPOS_InitInternalPos( pWnd,
			     *(LPPOINT32)&pWnd->rectWindow.left, &pWnd->rectWindow );

	if( flags & PLACE_MIN ) lpPos->ptIconPos = wndpl->ptMinPosition;
	if( flags & PLACE_MAX ) lpPos->ptMaxPos = wndpl->ptMaxPosition;
	if( flags & PLACE_RECT) lpPos->rectNormal = wndpl->rcNormalPosition;

	if( pWnd->dwStyle & WS_MINIMIZE )
	{
	    WINPOS_ShowIconTitle( pWnd, FALSE );
	    if( wndpl->flags & WPF_SETMINPOSITION && !EMPTYPOINT(lpPos->ptIconPos))
		SetWindowPos32( hwnd, 0, lpPos->ptIconPos.x, lpPos->ptIconPos.y,
				0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	} 
	else if( pWnd->dwStyle & WS_MAXIMIZE )
	{
	    if( !EMPTYPOINT(lpPos->ptMaxPos) )
		SetWindowPos32( hwnd, 0, lpPos->ptMaxPos.x, lpPos->ptMaxPos.y,
				0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	}
	else if( flags & PLACE_RECT )
		SetWindowPos32( hwnd, 0, lpPos->rectNormal.left, lpPos->rectNormal.top,
				lpPos->rectNormal.right - lpPos->rectNormal.left,
				lpPos->rectNormal.bottom - lpPos->rectNormal.top,
				SWP_NOZORDER | SWP_NOACTIVATE );

	ShowWindow32( hwnd, wndpl->showCmd );
	if( IsWindow32(hwnd) && pWnd->dwStyle & WS_MINIMIZE )
	{
	    if( pWnd->dwStyle & WS_VISIBLE ) WINPOS_ShowIconTitle( pWnd, TRUE );

	    /* SDK: ...valid only the next time... */
	    if( wndpl->flags & WPF_RESTORETOMAXIMIZED ) pWnd->flags |= WIN_RESTORE_MAX;
	}
	return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           SetWindowPlacement16   (USER.371)
 */
BOOL16 WINAPI SetWindowPlacement16(HWND16 hwnd, const WINDOWPLACEMENT16 *wndpl)
{
    return WINPOS_SetPlacement( hwnd, wndpl,
                                PLACE_MIN | PLACE_MAX | PLACE_RECT );
}

/***********************************************************************
 *           SetWindowPlacement32   (USER32.519)
 */
BOOL32 WINAPI SetWindowPlacement32( HWND32 hwnd, const WINDOWPLACEMENT32 *pwpl32 )
{
    if( pwpl32 )
    {
	WINDOWPLACEMENT16 wpl = { sizeof(WINDOWPLACEMENT16), 
		pwpl32->flags, pwpl32->showCmd, { pwpl32->ptMinPosition.x,
		pwpl32->ptMinPosition.y }, { pwpl32->ptMaxPosition.x,
		pwpl32->ptMaxPosition.y }, { pwpl32->rcNormalPosition.left,
		pwpl32->rcNormalPosition.top, pwpl32->rcNormalPosition.right,
		pwpl32->rcNormalPosition.bottom } };

        return WINPOS_SetPlacement( hwnd, &wpl, PLACE_MIN | PLACE_MAX | PLACE_RECT );
    }
    return FALSE;
}


/***********************************************************************
 *           SetInternalWindowPos16   (USER.461)
 */
void WINAPI SetInternalWindowPos16( HWND16 hwnd, UINT16 showCmd,
                                    LPRECT16 rect, LPPOINT16 pt )
{
    if( IsWindow16(hwnd) )
    {
	WINDOWPLACEMENT16 wndpl;
	UINT32 flags;

	wndpl.length  = sizeof(wndpl);
	wndpl.showCmd = showCmd;
	wndpl.flags = flags = 0;

	if( pt )
	{
	    flags |= PLACE_MIN;
	    wndpl.flags |= WPF_SETMINPOSITION;
	    wndpl.ptMinPosition = *pt;
	}
	if( rect )
	{
	    flags |= PLACE_RECT;
	    wndpl.rcNormalPosition = *rect;
	}
	WINPOS_SetPlacement( hwnd, &wndpl, flags );
    }
}


/***********************************************************************
 *           SetInternalWindowPos32   (USER32.483)
 */
void WINAPI SetInternalWindowPos32( HWND32 hwnd, UINT32 showCmd,
                                    LPRECT32 rect, LPPOINT32 pt )
{
    if( IsWindow32(hwnd) )
    {
	WINDOWPLACEMENT16 wndpl;
	UINT32 flags;

	wndpl.length  = sizeof(wndpl);
	wndpl.showCmd = showCmd;
	wndpl.flags = flags = 0;

	if( pt )
	{
            flags |= PLACE_MIN;
            wndpl.flags |= WPF_SETMINPOSITION;
            CONV_POINT32TO16( pt, &wndpl.ptMinPosition );
	}
	if( rect )
	{
            flags |= PLACE_RECT;
            CONV_RECT32TO16( rect, &wndpl.rcNormalPosition );
	}
        WINPOS_SetPlacement( hwnd, &wndpl, flags );
    }
}

/*******************************************************************
 *	   WINPOS_SetActiveWindow
 *
 * SetActiveWindow() back-end. This is the only function that
 * can assign active status to a window. It must be called only
 * for the top level windows.
 */
BOOL32 WINPOS_SetActiveWindow( HWND32 hWnd, BOOL32 fMouse, BOOL32 fChangeFocus)
{
    CBTACTIVATESTRUCT16* cbtStruct;
    WND*     wndPtr, *wndTemp;
    HQUEUE16 hOldActiveQueue, hNewActiveQueue;
    WORD     wIconized = 0;

    /* paranoid checks */
    if( hWnd == GetDesktopWindow32() || hWnd == hwndActive ) return 0;

/*  if (wndPtr && (GetFastQueue() != wndPtr->hmemTaskQ))
 *	return 0;
 */
    wndPtr = WIN_FindWndPtr(hWnd);
    hOldActiveQueue = (pActiveQueue)?pActiveQueue->self : 0;

    if( (wndTemp = WIN_FindWndPtr(hwndActive)) )
	wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
    else
	TRACE(win,"no current active window.\n");

    /* call CBT hook chain */
    if ((cbtStruct = SEGPTR_NEW(CBTACTIVATESTRUCT16)))
    {
        LRESULT wRet;
        cbtStruct->fMouse     = fMouse;
        cbtStruct->hWndActive = hwndActive;
        wRet = HOOK_CallHooks16( WH_CBT, HCBT_ACTIVATE, (WPARAM16)hWnd,
                                 (LPARAM)SEGPTR_GET(cbtStruct) );
        SEGPTR_FREE(cbtStruct);
        if (wRet) return wRet;
    }

    /* set prev active wnd to current active wnd and send notification */
    if ((hwndPrevActive = hwndActive) && IsWindow32(hwndPrevActive))
    {
        if (!SendMessage32A( hwndPrevActive, WM_NCACTIVATE, FALSE, 0 ))
        {
	    if (GetSysModalWindow16() != hWnd) return 0;
	    /* disregard refusal if hWnd is sysmodal */
        }

#if 1
	SendMessage32A( hwndPrevActive, WM_ACTIVATE,
                        MAKEWPARAM( WA_INACTIVE, wIconized ),
                        (LPARAM)hWnd );
#else
	/* FIXME: must be SendMessage16() because 32A doesn't do
	 * intertask at this time */
	SendMessage16( hwndPrevActive, WM_ACTIVATE, WA_INACTIVE,
				MAKELPARAM( (HWND16)hWnd, wIconized ) );
#endif

	/* check if something happened during message processing */
	if( hwndPrevActive != hwndActive ) return 0;
    }

    /* set active wnd */
    hwndActive = hWnd;

    /* send palette messages */
    if (hWnd && SendMessage16( hWnd, WM_QUERYNEWPALETTE, 0, 0L))
	SendMessage16((HWND16)-1, WM_PALETTEISCHANGING, (WPARAM16)hWnd, 0L );

    /* if prev wnd is minimized redraw icon title */
    if( IsIconic32( hwndPrevActive ) ) WINPOS_RedrawIconTitle(hwndPrevActive);

    /* managed windows will get ConfigureNotify event */  
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD) && !(wndPtr->flags & WIN_MANAGED))
    {
	/* check Z-order and bring hWnd to the top */
	for (wndTemp = WIN_GetDesktop()->child; wndTemp; wndTemp = wndTemp->next)
	    if (wndTemp->dwStyle & WS_VISIBLE) break;

	if( wndTemp != wndPtr )
	    SetWindowPos32(hWnd, HWND_TOP, 0,0,0,0, 
			   SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
        if (!IsWindow32(hWnd)) return 0;
    }

    hNewActiveQueue = wndPtr ? wndPtr->hmemTaskQ : 0;

    /* send WM_ACTIVATEAPP if necessary */
    if (hOldActiveQueue != hNewActiveQueue)
    {
        WND **list, **ppWnd;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hOldActiveQueue)
                   SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                   0, QUEUE_GetQueueTask(hNewActiveQueue) );
            }
            HeapFree( SystemHeap, 0, list );
        }

	pActiveQueue = (hNewActiveQueue)
		       ? (MESSAGEQUEUE*) GlobalLock16(hNewActiveQueue) : NULL;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                if (!IsWindow32( (*ppWnd)->hwndSelf )) continue;

                if ((*ppWnd)->hmemTaskQ == hNewActiveQueue)
                   SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                                  1, QUEUE_GetQueueTask( hOldActiveQueue ) );
            }
            HeapFree( SystemHeap, 0, list );
        }
	if (!IsWindow32(hWnd)) return 0;
    }

    if (hWnd)
    {
        /* walk up to the first unowned window */
        wndTemp = wndPtr;
        while (wndTemp->owner) wndTemp = wndTemp->owner;
        /* and set last active owned popup */
        wndTemp->hwndLastActive = hWnd;

        wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
        SendMessage32A( hWnd, WM_NCACTIVATE, TRUE, 0 );
#if 1
        SendMessage32A( hWnd, WM_ACTIVATE,
		 MAKEWPARAM( (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE, wIconized),
		 (LPARAM)hwndPrevActive );
#else
        SendMessage16(hWnd, WM_ACTIVATE, (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE,
                      MAKELPARAM( (HWND16)hwndPrevActive, wIconized) );
#endif

        if( !IsWindow32(hWnd) ) return 0;
    }

    /* change focus if possible */
    if( fChangeFocus && GetFocus32() )
	if( WIN_GetTopParent(GetFocus32()) != hwndActive )
	    FOCUS_SwitchFocus( GetFocus32(),
			       (wndPtr && (wndPtr->dwStyle & WS_MINIMIZE))?
			       0:
			       hwndActive
	    );

    if( !hwndPrevActive && wndPtr )
        (*wndPtr->pDriver->pForceWindowRaise)(wndPtr);

    /* if active wnd is minimized redraw icon title */
    if( IsIconic32(hwndActive) ) WINPOS_RedrawIconTitle(hwndActive);

    return (hWnd == hwndActive);
}

/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
BOOL32 WINPOS_ActivateOtherWindow(WND* pWnd)
{
  BOOL32	bRet = 0;
  WND*  	pWndTo = NULL;

  if( pWnd->hwndSelf == hwndPrevActive )
      hwndPrevActive = 0;

  if( hwndActive != pWnd->hwndSelf && 
    ( hwndActive || QUEUE_IsExitingQueue(pWnd->hmemTaskQ)) )
      return 0;

  if( !(pWnd->dwStyle & WS_POPUP) || !(pWnd->owner) ||
      !WINPOS_CanActivate((pWndTo = WIN_GetTopParentPtr(pWnd->owner))) ) 
  {
      WND* pWndPtr = WIN_GetTopParentPtr(pWnd);

      pWndTo = WIN_FindWndPtr(hwndPrevActive);

      while( !WINPOS_CanActivate(pWndTo) ) 
      {
	 /* by now owned windows should've been taken care of */

	  pWndTo = pWndPtr->next;
	  pWndPtr = pWndTo;
	  if( !pWndTo ) break;
      }
  }

  bRet = WINPOS_SetActiveWindow( pWndTo ? pWndTo->hwndSelf : 0, FALSE, TRUE );

  /* switch desktop queue to current active */
  if( pWndTo ) WIN_GetDesktop()->hmemTaskQ = pWndTo->hmemTaskQ;

  hwndPrevActive = 0;
  return bRet;  
}

/*******************************************************************
 *	   WINPOS_ChangeActiveWindow
 *
 */
BOOL32 WINPOS_ChangeActiveWindow( HWND32 hWnd, BOOL32 mouseMsg )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if (!hWnd) return WINPOS_SetActiveWindow( 0, mouseMsg, TRUE );

    if( !wndPtr ) return FALSE;

    /* child windows get WM_CHILDACTIVATE message */
    if( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD )
	return SendMessage32A(hWnd, WM_CHILDACTIVATE, 0, 0L);

        /* owned popups imply owner activation - not sure */
    if ((wndPtr->dwStyle & WS_POPUP) && wndPtr->owner &&
        (wndPtr->owner->dwStyle & WS_VISIBLE ) &&
        !(wndPtr->owner->dwStyle & WS_DISABLED ))
    {
        if (!(wndPtr = wndPtr->owner)) return FALSE;
	hWnd = wndPtr->hwndSelf;
    }

    if( hWnd == hwndActive ) return FALSE;

    if( !WINPOS_SetActiveWindow(hWnd ,mouseMsg ,TRUE) )
	return FALSE;

    /* switch desktop queue to current active */
    if( wndPtr->parent == WIN_GetDesktop())
        WIN_GetDesktop()->hmemTaskQ = wndPtr->hmemTaskQ;

    return TRUE;
}


/***********************************************************************
 *           WINPOS_SendNCCalcSize
 *
 * Send a WM_NCCALCSIZE message to a window.
 * All parameters are read-only except newClientRect.
 * oldWindowRect, oldClientRect and winpos must be non-NULL only
 * when calcValidRect is TRUE.
 */
LONG WINPOS_SendNCCalcSize( HWND32 hwnd, BOOL32 calcValidRect,
                            RECT32 *newWindowRect, RECT32 *oldWindowRect,
                            RECT32 *oldClientRect, WINDOWPOS32 *winpos,
                            RECT32 *newClientRect )
{
    NCCALCSIZE_PARAMS32 params;
    WINDOWPOS32 winposCopy;
    LONG result;

    params.rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
        winposCopy = *winpos;
	params.rgrc[1] = *oldWindowRect;
	params.rgrc[2] = *oldClientRect;
	params.lppos = &winposCopy;
    }
    result = SendMessage32A( hwnd, WM_NCCALCSIZE, calcValidRect,
                             (LPARAM)&params );
    TRACE(win, "%d,%d-%d,%d\n",
                 params.rgrc[0].left, params.rgrc[0].top,
                 params.rgrc[0].right, params.rgrc[0].bottom );
    *newClientRect = params.rgrc[0];
    return result;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging16
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging16( WND *wndPtr, WINDOWPOS16 *winpos )
{
    POINT32 maxSize, minTrack;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	WINPOS_GetMinMaxInfo( wndPtr, &maxSize, NULL, &minTrack, NULL );
	if (maxSize.x < winpos->cx) winpos->cx = maxSize.x;
	if (maxSize.y < winpos->cy) winpos->cy = maxSize.y;
	if (!(wndPtr->dwStyle & WS_MINIMIZE))
	{
	    if (winpos->cx < minTrack.x ) winpos->cx = minTrack.x;
	    if (winpos->cy < minTrack.y ) winpos->cy = minTrack.y;
	}
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging32
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging32( WND *wndPtr, WINDOWPOS32 *winpos )
{
    POINT32 maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	WINPOS_GetMinMaxInfo( wndPtr, &maxSize, NULL, NULL, NULL );
	winpos->cx = MIN( winpos->cx, maxSize.x );
	winpos->cy = MIN( winpos->cy, maxSize.y );
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_MoveWindowZOrder
 *
 * Move a window in Z order, invalidating everything that needs it.
 * Only necessary for windows without associated X window.
 */
static void WINPOS_MoveWindowZOrder( HWND32 hwnd, HWND32 hwndAfter )
{
    BOOL32 movingUp;
    WND *pWndAfter, *pWndCur, *wndPtr = WIN_FindWndPtr( hwnd );

    /* We have two possible cases:
     * - The window is moving up: we have to invalidate all areas
     *   of the window that were covered by other windows
     * - The window is moving down: we have to invalidate areas
     *   of other windows covered by this one.
     */

    if (hwndAfter == HWND_TOP)
    {
        movingUp = TRUE;
    }
    else if (hwndAfter == HWND_BOTTOM)
    {
        if (!wndPtr->next) return;  /* Already at the bottom */
        movingUp = FALSE;
    }
    else
    {
        if (!(pWndAfter = WIN_FindWndPtr( hwndAfter ))) return;
        if (wndPtr->next == pWndAfter) return;  /* Already placed right */

          /* Determine which window we encounter first in Z-order */
        pWndCur = wndPtr->parent->child;
        while ((pWndCur != wndPtr) && (pWndCur != pWndAfter))
            pWndCur = pWndCur->next;
        movingUp = (pWndCur == pWndAfter);
    }

    if (movingUp)
    {
        WND *pWndPrevAfter = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        pWndCur = wndPtr->next;
        while (pWndCur != pWndPrevAfter)
        {
            RECT32 rect = { pWndCur->rectWindow.left,
			    pWndCur->rectWindow.top,
			    pWndCur->rectWindow.right,
			    pWndCur->rectWindow.bottom };
            OffsetRect32( &rect, -wndPtr->rectClient.left,
                          -wndPtr->rectClient.top );
            PAINT_RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                              RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
    else  /* Moving down */
    {
        pWndCur = wndPtr->next;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        while (pWndCur != wndPtr)
        {
            RECT32 rect = { pWndCur->rectWindow.left,
                            pWndCur->rectWindow.top,
                            pWndCur->rectWindow.right,
                            pWndCur->rectWindow.bottom };
            OffsetRect32( &rect, -pWndCur->rectClient.left,
                          -pWndCur->rectClient.top );
            PAINT_RedrawWindow( pWndCur->hwndSelf, &rect, 0, RDW_INVALIDATE |
                              RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE, 0 );
            pWndCur = pWndCur->next;
        }
    }
}

/***********************************************************************
 *           WINPOS_ReorderOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
HWND32 WINPOS_ReorderOwnedPopups(HWND32 hwndInsertAfter,WND* wndPtr,WORD flags)
{
 WND* 	w = WIN_GetDesktop()->child;

  if( wndPtr->dwStyle & WS_POPUP && wndPtr->owner )
  {
   /* implement "local z-order" between the top and owner window */

     HWND32 hwndLocalPrev = HWND_TOP;

     if( hwndInsertAfter != HWND_TOP )
     {
	while( w != wndPtr->owner )
	{
          if (w != wndPtr) hwndLocalPrev = w->hwndSelf;
	  if( hwndLocalPrev == hwndInsertAfter ) break;
	  w = w->next;
	}
	hwndInsertAfter = hwndLocalPrev;
     }

  }
  else if( wndPtr->dwStyle & WS_CHILD ) return hwndInsertAfter;

  w = WIN_GetDesktop()->child;
  while( w )
  {
    if( w == wndPtr ) break;

    if( w->dwStyle & WS_POPUP && w->owner == wndPtr )
    {
      SetWindowPos32(w->hwndSelf, hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                     SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE);
      hwndInsertAfter = w->hwndSelf;
    }
    w = w->next;
  }

  return hwndInsertAfter;
}

/***********************************************************************
 *	     WINPOS_SizeMoveClean
 *
 * Make window look nice without excessive repainting
 *
 * the pain:
 *
 * visible regions are in window coordinates
 * update regions are in window client coordinates
 * client and window rectangles are in parent client coordinates
 *
 * FIXME: Move visible and update regions to the same coordinate system
 *	 (either parent client or window). This is a lot of work though.
 */
static UINT32 WINPOS_SizeMoveClean( WND* Wnd, HRGN32 oldVisRgn,
                                    LPRECT32 lpOldWndRect,
                                    LPRECT32 lpOldClientRect, UINT32 uFlags )
{
 HRGN32 newVisRgn = DCE_GetVisRgn(Wnd->hwndSelf,DCX_WINDOW | DCX_CLIPSIBLINGS);
 HRGN32 dirtyRgn = CreateRectRgn32(0,0,0,0);
 int  other, my;

 TRACE(win,"cleaning up...new wnd=(%i %i-%i %i) old wnd=(%i %i-%i %i)\n",
	      Wnd->rectWindow.left, Wnd->rectWindow.top,
	      Wnd->rectWindow.right, Wnd->rectWindow.bottom,
	      lpOldWndRect->left, lpOldWndRect->top,
	      lpOldWndRect->right, lpOldWndRect->bottom);
 TRACE(win,"\tnew client=(%i %i-%i %i) old client=(%i %i-%i %i)\n",
	      Wnd->rectClient.left, Wnd->rectClient.top,
	      Wnd->rectClient.right, Wnd->rectClient.bottom,
	      lpOldClientRect->left, lpOldClientRect->top,
	      lpOldClientRect->right,lpOldClientRect->bottom );

 if( (lpOldWndRect->right - lpOldWndRect->left) != (Wnd->rectWindow.right - Wnd->rectWindow.left) ||
     (lpOldWndRect->bottom - lpOldWndRect->top) != (Wnd->rectWindow.bottom - Wnd->rectWindow.top) )
     uFlags |= SMC_DRAWFRAME;

 CombineRgn32( dirtyRgn, newVisRgn, 0, RGN_COPY);

 if( !(uFlags & SMC_NOCOPY) )
   CombineRgn32( newVisRgn, newVisRgn, oldVisRgn, RGN_AND ); 

 /* map regions to the parent client area */
 
 OffsetRgn32( dirtyRgn, Wnd->rectWindow.left, Wnd->rectWindow.top );
 OffsetRgn32( oldVisRgn, lpOldWndRect->left, lpOldWndRect->top );

 /* compute invalidated region outside Wnd - (in client coordinates of the parent window) */

 other = CombineRgn32(dirtyRgn, oldVisRgn, dirtyRgn, RGN_DIFF);

 /* map visible region to the Wnd client area */

 OffsetRgn32( newVisRgn, Wnd->rectWindow.left - Wnd->rectClient.left,
                         Wnd->rectWindow.top - Wnd->rectClient.top );

 /* substract previously invalidated region from the Wnd visible region */

 my =  (Wnd->hrgnUpdate > 1) ? CombineRgn32( newVisRgn, newVisRgn,
                                             Wnd->hrgnUpdate, RGN_DIFF)
                             : COMPLEXREGION;

 if( uFlags & SMC_NOCOPY )	/* invalidate Wnd visible region */
   {
     if (my != NULLREGION)
	 PAINT_RedrawWindow( Wnd->hwndSelf, NULL, newVisRgn, RDW_INVALIDATE |
	  RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if(uFlags & SMC_DRAWFRAME)
	 Wnd->flags |= WIN_NEEDS_NCPAINT;
   }
 else			/* bitblt old client area */
   { 
     HDC32 hDC;
     int   update;
     HRGN32 updateRgn;
     int   xfrom,yfrom,xto,yto,width,height;

     if( uFlags & SMC_DRAWFRAME )
       {
	 /* copy only client area, frame will be redrawn anyway */

         xfrom = lpOldClientRect->left; yfrom = lpOldClientRect->top;
         xto = Wnd->rectClient.left; yto = Wnd->rectClient.top;
         width = lpOldClientRect->right - xfrom; height = lpOldClientRect->bottom - yfrom;
	 updateRgn = CreateRectRgn32( 0, 0, width, height );
	 CombineRgn32( newVisRgn, newVisRgn, updateRgn, RGN_AND );
	 SetRectRgn32( updateRgn, 0, 0, Wnd->rectClient.right - xto,
                       Wnd->rectClient.bottom - yto );
       }
     else
       {
         xfrom = lpOldWndRect->left; yfrom = lpOldWndRect->top;
         xto = Wnd->rectWindow.left; yto = Wnd->rectWindow.top;
         width = lpOldWndRect->right - xfrom; height = lpOldWndRect->bottom - yfrom;
	 updateRgn = CreateRectRgn32( xto - Wnd->rectClient.left,
				      yto - Wnd->rectClient.top,
				Wnd->rectWindow.right - Wnd->rectClient.left,
			        Wnd->rectWindow.bottom - Wnd->rectClient.top );
       }

     CombineRgn32( newVisRgn, newVisRgn, updateRgn, RGN_AND );

     /* substract new visRgn from target rect to get a region that won't be copied */

     update = CombineRgn32( updateRgn, updateRgn, newVisRgn, RGN_DIFF );

     /* Blt valid bits using parent window DC */

     if( my != NULLREGION && (xfrom != xto || yfrom != yto) )
       {
	 
	 /* compute clipping region in parent client coordinates */

	 OffsetRgn32( newVisRgn, Wnd->rectClient.left, Wnd->rectClient.top );
	 CombineRgn32( oldVisRgn, oldVisRgn, newVisRgn, RGN_OR );

         hDC = GetDCEx32( Wnd->parent->hwndSelf, oldVisRgn,
                          DCX_KEEPCLIPRGN | DCX_INTERSECTRGN |
                          DCX_CACHE | DCX_CLIPSIBLINGS);

         BitBlt32( hDC, xto, yto, width, height, hDC, xfrom, yfrom, SRCCOPY );
         ReleaseDC32( Wnd->parent->hwndSelf, hDC); 
       }

     if( update != NULLREGION )
         PAINT_RedrawWindow( Wnd->hwndSelf, NULL, updateRgn, RDW_INVALIDATE |
                         RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
     else if( uFlags & SMC_DRAWFRAME ) Wnd->flags |= WIN_NEEDS_NCPAINT;
     DeleteObject32( updateRgn );
   }

 /* erase uncovered areas */

 if( !(uFlags & SMC_NOPARENTERASE) && (other != NULLREGION ) )
      PAINT_RedrawWindow( Wnd->parent->hwndSelf, NULL, dirtyRgn,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE, RDW_C_USEHRGN );
 DeleteObject32(dirtyRgn);
 DeleteObject32(newVisRgn);
 return uFlags;
}

/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
BOOL16 WINAPI SetWindowPos16( HWND16 hwnd, HWND16 hwndInsertAfter,
                              INT16 x, INT16 y, INT16 cx, INT16 cy, WORD flags)
{
    return SetWindowPos32(hwnd,(INT32)(INT16)hwndInsertAfter,x,y,cx,cy,flags);
}

/***********************************************************************
 *           SetWindowPos   (USER32.520)
 */
BOOL32 WINAPI SetWindowPos32( HWND32 hwnd, HWND32 hwndInsertAfter,
                              INT32 x, INT32 y, INT32 cx, INT32 cy, WORD flags)
{
    WINDOWPOS32 winpos;
    WND *	wndPtr;
    RECT32 	newWindowRect, newClientRect, oldWindowRect;
    HRGN32	visRgn = 0;
    HWND32	tempInsertAfter= 0;
    int 	result = 0;
    UINT32 	uFlags = 0;
    BOOL32      resync = FALSE;

    TRACE(win,"hwnd %04x, (%i,%i)-(%i,%i) flags %08x\n", 
						 hwnd, x, y, x+cx, y+cy, flags);  
      /* Check window handle */

    if (hwnd == GetDesktopWindow32()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    if(wndPtr->dwStyle & WS_VISIBLE)
        flags &= ~SWP_SHOWWINDOW;
    else
    {
	uFlags |= SMC_NOPARENTERASE; 
	flags &= ~SWP_HIDEWINDOW;
	if (!(flags & SWP_SHOWWINDOW)) flags |= SWP_NOREDRAW;
    }

/*     Check for windows that may not be resized 
       FIXME: this should be done only for Windows 3.0 programs 
       if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW ) )
           flags |= SWP_NOSIZE | SWP_NOMOVE;
*/
      /* Check dimensions */

    if (cx <= 0) cx = 1;
    if (cy <= 0) cy = 1;

      /* Check flags */

    if (hwnd == hwndActive) flags |= SWP_NOACTIVATE;   /* Already active */
    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == cy))
        flags |= SWP_NOSIZE;    /* Already the right size */
    if ((wndPtr->rectWindow.left == x) && (wndPtr->rectWindow.top == y))
        flags |= SWP_NOMOVE;    /* Already the right position */

      /* Check hwndInsertAfter */

    if (!(flags & (SWP_NOZORDER | SWP_NOACTIVATE)))
    {
	  /* Ignore TOPMOST flags when activating a window */
          /* _and_ moving it in Z order. */
	if ((hwndInsertAfter == HWND_TOPMOST) ||
            (hwndInsertAfter == HWND_NOTOPMOST))
	    hwndInsertAfter = HWND_TOP;	
    }
      /* TOPMOST not supported yet */
    if ((hwndInsertAfter == HWND_TOPMOST) ||
        (hwndInsertAfter == HWND_NOTOPMOST)) hwndInsertAfter = HWND_TOP;

      /* hwndInsertAfter must be a sibling of the window */
    if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM))
       {
	 WND* wnd = WIN_FindWndPtr(hwndInsertAfter);

	 if( wndPtr ) {
	   if( wnd->parent != wndPtr->parent ) return FALSE;
	   if( wnd->next == wndPtr ) flags |= SWP_NOZORDER;
	 }
       }
    else if (!((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
    {
         /* FIXME: the following optimization is no good for "X-ed" windows */
       if (hwndInsertAfter == HWND_TOP)
	   flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
       else /* HWND_BOTTOM */
	   flags |= ( wndPtr->next )? 0: SWP_NOZORDER;
    }

      /* Fill the WINDOWPOS structure */

    winpos.hwnd = hwnd;
    winpos.hwndInsertAfter = hwndInsertAfter;
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    winpos.flags = flags;
    
      /* Send WM_WINDOWPOSCHANGING message */

    if (!(winpos.flags & SWP_NOSENDCHANGING))
	SendMessage32A( hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&winpos );

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = (wndPtr->dwStyle & WS_MINIMIZE) ? wndPtr->rectWindow
						    : wndPtr->rectClient;

    if (!(winpos.flags & SWP_NOSIZE))
    {
        newWindowRect.right  = newWindowRect.left + winpos.cx;
        newWindowRect.bottom = newWindowRect.top + winpos.cy;
    }
    if (!(winpos.flags & SWP_NOMOVE))
    {
        newWindowRect.left    = winpos.x;
        newWindowRect.top     = winpos.y;
        newWindowRect.right  += winpos.x - wndPtr->rectWindow.left;
        newWindowRect.bottom += winpos.y - wndPtr->rectWindow.top;

	OffsetRect32( &newClientRect, winpos.x - wndPtr->rectWindow.left, 
                                      winpos.y - wndPtr->rectWindow.top );
    }

    winpos.flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

      /* Reposition window in Z order */

    if (!(winpos.flags & SWP_NOZORDER))
    {
	/* reorder owned popups if hwnd is top-level window 
         */
	if( wndPtr->parent == WIN_GetDesktop() )
	    hwndInsertAfter = WINPOS_ReorderOwnedPopups( hwndInsertAfter,
							 wndPtr, winpos.flags );

        if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
        {
            WIN_UnlinkWindow( winpos.hwnd );
            WIN_LinkWindow( winpos.hwnd, hwndInsertAfter );
        }
        else WINPOS_MoveWindowZOrder( winpos.hwnd, hwndInsertAfter );
    }

    if ( !((X11DRV_WND_DATA *) wndPtr->pDriverData)->window && !(winpos.flags & SWP_NOREDRAW) && 
	((winpos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED)) 
		      != (SWP_NOMOVE | SWP_NOSIZE)) )
          visRgn = DCE_GetVisRgn(hwnd, DCX_WINDOW | DCX_CLIPSIBLINGS);


      /* Send WM_NCCALCSIZE message to get new client area */
    if( (winpos.flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
         result = WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    &winpos, &newClientRect );

         /* FIXME: WVR_ALIGNxxx */

         if( newClientRect.left != wndPtr->rectClient.left ||
             newClientRect.top != wndPtr->rectClient.top )
             winpos.flags &= ~SWP_NOCLIENTMOVE;

         if( (newClientRect.right - newClientRect.left !=
              wndPtr->rectClient.right - wndPtr->rectClient.left) ||
	     (newClientRect.bottom - newClientRect.top !=
	      wndPtr->rectClient.bottom - wndPtr->rectClient.top) )
	     winpos.flags &= ~SWP_NOCLIENTSIZE;
    }
    else
      if( !(flags & SWP_NOMOVE) && (newClientRect.left != wndPtr->rectClient.left ||
				    newClientRect.top != wndPtr->rectClient.top) )
	    winpos.flags &= ~SWP_NOCLIENTMOVE;

    /* Update active DCEs 
     * TODO: Optimize conditions that trigger DCE update.
     */

    if( (((winpos.flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) && 
					 wndPtr->dwStyle & WS_VISIBLE) || 
	(flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)) ) 
    {
        RECT32 rect;

        UnionRect32(&rect, &newWindowRect, &wndPtr->rectWindow);
	DCE_InvalidateDCE(wndPtr, &rect);
    }

    /* change geometry */

    oldWindowRect = wndPtr->rectWindow;

    if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
    {
        RECT32 oldClientRect = wndPtr->rectClient;

        tempInsertAfter = winpos.hwndInsertAfter;

        winpos.hwndInsertAfter = hwndInsertAfter;

	/* postpone geometry change */

	if( !(flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) )
	{
              wndPtr->pDriver->pSetWindowPos(wndPtr, &winpos, TRUE);
	      winpos.hwndInsertAfter = tempInsertAfter;
	}
	else  uFlags |= SMC_SETXPOS;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

	if( !(flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) )
        {
	  if( (oldClientRect.left - oldWindowRect.left !=
	       newClientRect.left - newWindowRect.left) ||
	      (oldClientRect.top - oldWindowRect.top !=
	       newClientRect.top - newWindowRect.top) ||
              (winpos.flags & SWP_NOCOPYBITS) )
	  {
	      /* if the client area moved as a result of WM_NCCALCSIZE returning 
	       * obscure WVR_ALIGNxxx flags then we simply redraw the whole thing
	       *
	       * TODO: use WINPOS_SizeMoveClean() if there is no SWP_NOCOPYBITS 
	       */

	      PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, RDW_INVALIDATE |
                              RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE, 0 );
	  }
	  else
	      if( winpos.flags & SWP_FRAMECHANGED )
	      {
		WORD wErase = 0;
		RECT32 rect;

	        if( newClientRect.right > oldClientRect.right ) /* redraw exposed client area on the right */
                {
		    rect.top = 0; rect.bottom = newClientRect.bottom - newClientRect.top;
		    rect.left = oldClientRect.right - newClientRect.left;
		    rect.right = newClientRect.right - newClientRect.left;
		    wErase = 1;
		    PAINT_RedrawWindow( wndPtr->hwndSelf, &rect, 0,
                                      RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW | RDW_ALLCHILDREN, 0 );
                }
		if( newClientRect.bottom > oldClientRect.bottom ) /* redraw exposed client area on the bottom */
                {
		    rect.left = 0; rect.right = ((wErase)?oldClientRect.right:newClientRect.right) - newClientRect.left;
		    rect.top = oldClientRect.bottom - newClientRect.top;
		    rect.bottom = newClientRect.bottom - newClientRect.top;
		    wErase = 1;
		    PAINT_RedrawWindow( wndPtr->hwndSelf, &rect, 0,
                                      RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW | RDW_ALLCHILDREN, 0 );
                }
		if( !wErase ) /* just update the nonclient area */
		    wndPtr->flags |= WIN_NEEDS_NCPAINT; 
	      }
        }
	uFlags |= SMC_NOPARENTERASE; /* X windows do not have eraseable parents */
    }
    else /* not an X window */
    {
	RECT32 oldClientRect = wndPtr->rectClient;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

	if( oldClientRect.bottom - oldClientRect.top ==
	    newClientRect.bottom - newClientRect.top ) result &= ~WVR_VREDRAW;

	if( oldClientRect.right - oldClientRect.left ==
	    newClientRect.right - newClientRect.left ) result &= ~WVR_HREDRAW;

        if( !(flags & (SWP_NOREDRAW | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) )
	{
	    uFlags |=  ((winpos.flags & SWP_NOCOPYBITS) || 
			(result >= WVR_HREDRAW && result < WVR_VALIDRECTS)) ? SMC_NOCOPY : 0;
	    uFlags |=  (winpos.flags & SWP_FRAMECHANGED) ? SMC_DRAWFRAME : 0;

	    if( (winpos.flags & SWP_AGG_NOGEOMETRYCHANGE) != SWP_AGG_NOGEOMETRYCHANGE )
		uFlags = WINPOS_SizeMoveClean(wndPtr, visRgn, &oldWindowRect, 
							      &oldClientRect, uFlags);
	    else
	    { 
		/* adjust the frame and do not erase the parent */

		if( winpos.flags & SWP_FRAMECHANGED ) wndPtr->flags |= WIN_NEEDS_NCPAINT;
		if( winpos.flags & SWP_NOZORDER ) uFlags |= SMC_NOPARENTERASE;
	    }
	}
        DeleteObject32(visRgn);
    }

    if (flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
        if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
        {
	    HWND32 focus, curr;

	    wndPtr->pDriver->pSetWindowPos(wndPtr, &winpos, uFlags & SMC_SETXPOS );
	    if( uFlags & SMC_SETXPOS )
	    {
              winpos.hwndInsertAfter = tempInsertAfter;
	    }

            if (wndPtr->flags & WIN_MANAGED) resync = TRUE;

	    /* If focus was set to an unmapped window, reset X focus now */
	    focus = curr = GetFocus32();
	    while (curr) {
		if (curr == hwnd) {
		    SetFocus32( 0 );
		    SetFocus32( focus );
		    break;
		}
		curr = GetParent32(curr);
	    }
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
                PAINT_RedrawWindow( winpos.hwnd, NULL, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN |
                                RDW_FRAME | RDW_ERASENOW | RDW_ERASE, 0 );
        }
    }
    else if (flags & SWP_HIDEWINDOW)
    {
        wndPtr->dwStyle &= ~WS_VISIBLE;

        if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
        {
	    wndPtr->pDriver->pSetWindowPos(wndPtr, &winpos, uFlags & SMC_SETXPOS );
	    if( uFlags & SMC_SETXPOS )
	    {
              winpos.hwndInsertAfter = tempInsertAfter;
	    }
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
                PAINT_RedrawWindow( wndPtr->parent->hwndSelf, &oldWindowRect,
                                    0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                                       RDW_ERASE | RDW_ERASENOW, 0 );
	    uFlags |= SMC_NOPARENTERASE;
        }

        if ((winpos.hwnd == GetFocus32()) ||
            IsChild32( winpos.hwnd, GetFocus32()))
        {
            /* Revert focus to parent */
            SetFocus32( GetParent32(winpos.hwnd) );
        }
	if (hwnd == CARET_GetHwnd()) DestroyCaret32();

	if (winpos.hwnd == hwndActive)
	    WINPOS_ActivateOtherWindow( wndPtr );
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
	    WINPOS_ChangeActiveWindow( winpos.hwnd, FALSE );
    
      /* Repaint the window */

    if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
        EVENT_Synchronize();  /* Wait for all expose events */

    if (!GetCapture32())
        EVENT_DummyMotionNotify(); /* Simulate a mouse event to set the cursor */

    if (!(flags & SWP_DEFERERASE) && !(uFlags & SMC_NOPARENTERASE) )
        PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0, RDW_ALLCHILDREN | RDW_ERASENOW, 0 );
    else if( wndPtr->parent == WIN_GetDesktop() && wndPtr->parent->flags & WIN_NEEDS_ERASEBKGND )
	PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0, RDW_NOCHILDREN | RDW_ERASENOW, 0 );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE(win,"\tstatus flags = %04x\n", winpos.flags & SWP_AGG_STATUSFLAGS);

    if ( resync ||
        (((winpos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) && 
         !(winpos.flags & SWP_NOSENDCHANGING)) )
    {
        SendMessage32A( winpos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&winpos );
        if (resync) EVENT_Synchronize ();
    }

    return TRUE;
}

					
/***********************************************************************
 *           BeginDeferWindowPos16   (USER.259)
 */
HDWP16 WINAPI BeginDeferWindowPos16( INT16 count )
{
    return BeginDeferWindowPos32( count );
}


/***********************************************************************
 *           BeginDeferWindowPos32   (USER32.9)
 */
HDWP32 WINAPI BeginDeferWindowPos32( INT32 count )
{
    HDWP32 handle;
    DWP *pDWP;

    if (count <= 0) return 0;
    handle = USER_HEAP_ALLOC( sizeof(DWP) + (count-1)*sizeof(WINDOWPOS32) );
    if (!handle) return 0;
    pDWP = (DWP *) USER_HEAP_LIN_ADDR( handle );
    pDWP->actualCount    = 0;
    pDWP->suggestedCount = count;
    pDWP->valid          = TRUE;
    pDWP->wMagic         = DWP_MAGIC;
    pDWP->hwndParent     = 0;
    return handle;
}


/***********************************************************************
 *           DeferWindowPos16   (USER.260)
 */
HDWP16 WINAPI DeferWindowPos16( HDWP16 hdwp, HWND16 hwnd, HWND16 hwndAfter,
                                INT16 x, INT16 y, INT16 cx, INT16 cy,
                                UINT16 flags )
{
    return DeferWindowPos32( hdwp, hwnd, (INT32)(INT16)hwndAfter,
                             x, y, cx, cy, flags );
}


/***********************************************************************
 *           DeferWindowPos32   (USER32.128)
 */
HDWP32 WINAPI DeferWindowPos32( HDWP32 hdwp, HWND32 hwnd, HWND32 hwndAfter,
                                INT32 x, INT32 y, INT32 cx, INT32 cy,
                                UINT32 flags )
{
    DWP *pDWP;
    int i;
    HDWP32 newhdwp = hdwp;
    /* HWND32 parent; */
    WND *pWnd;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return 0;
    if (hwnd == GetDesktopWindow32()) return 0;

    if (!(pWnd=WIN_FindWndPtr( hwnd ))) {
        USER_HEAP_FREE( hdwp );
        return 0;
    }
    	
/* Numega Bounds Checker Demo dislikes the following code.
   In fact, I've not been able to find any "same parent" requirement in any docu
   [AM 980509]
 */
#if 0
    /* All the windows of a DeferWindowPos() must have the same parent */
    parent = pWnd->parent->hwndSelf;
    if (pDWP->actualCount == 0) pDWP->hwndParent = parent;
    else if (parent != pDWP->hwndParent)
    {
        USER_HEAP_FREE( hdwp );
        return 0;
    }
#endif

    for (i = 0; i < pDWP->actualCount; i++)
    {
        if (pDWP->winPos[i].hwnd == hwnd)
        {
              /* Merge with the other changes */
            if (!(flags & SWP_NOZORDER))
            {
                pDWP->winPos[i].hwndInsertAfter = hwndAfter;
            }
            if (!(flags & SWP_NOMOVE))
            {
                pDWP->winPos[i].x = x;
                pDWP->winPos[i].y = y;
            }                
            if (!(flags & SWP_NOSIZE))
            {
                pDWP->winPos[i].cx = cx;
                pDWP->winPos[i].cy = cy;
            }
            pDWP->winPos[i].flags &= flags | ~(SWP_NOSIZE | SWP_NOMOVE |
                                               SWP_NOZORDER | SWP_NOREDRAW |
                                               SWP_NOACTIVATE | SWP_NOCOPYBITS|
                                               SWP_NOOWNERZORDER);
            pDWP->winPos[i].flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                              SWP_FRAMECHANGED);
            return hdwp;
        }
    }
    if (pDWP->actualCount >= pDWP->suggestedCount)
    {
        newhdwp = USER_HEAP_REALLOC( hdwp,
                      sizeof(DWP) + pDWP->suggestedCount*sizeof(WINDOWPOS32) );
        if (!newhdwp) return 0;
        pDWP = (DWP *) USER_HEAP_LIN_ADDR( newhdwp );
        pDWP->suggestedCount++;
    }
    pDWP->winPos[pDWP->actualCount].hwnd = hwnd;
    pDWP->winPos[pDWP->actualCount].hwndInsertAfter = hwndAfter;
    pDWP->winPos[pDWP->actualCount].x = x;
    pDWP->winPos[pDWP->actualCount].y = y;
    pDWP->winPos[pDWP->actualCount].cx = cx;
    pDWP->winPos[pDWP->actualCount].cy = cy;
    pDWP->winPos[pDWP->actualCount].flags = flags;
    pDWP->actualCount++;
    return newhdwp;
}


/***********************************************************************
 *           EndDeferWindowPos16   (USER.261)
 */
BOOL16 WINAPI EndDeferWindowPos16( HDWP16 hdwp )
{
    return EndDeferWindowPos32( hdwp );
}


/***********************************************************************
 *           EndDeferWindowPos32   (USER32.173)
 */
BOOL32 WINAPI EndDeferWindowPos32( HDWP32 hdwp )
{
    DWP *pDWP;
    WINDOWPOS32 *winpos;
    BOOL32 res = TRUE;
    int i;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return FALSE;
    for (i = 0, winpos = pDWP->winPos; i < pDWP->actualCount; i++, winpos++)
    {
        if (!(res = SetWindowPos32( winpos->hwnd, winpos->hwndInsertAfter,
                                    winpos->x, winpos->y, winpos->cx,
                                    winpos->cy, winpos->flags ))) break;
    }
    USER_HEAP_FREE( hdwp );
    return res;
}


/***********************************************************************
 *           TileChildWindows   (USER.199)
 */
void WINAPI TileChildWindows( HWND16 parent, WORD action )
{
    FIXME(win, "(%04x, %d): stub\n", parent, action);
}

/***********************************************************************
 *           CascageChildWindows   (USER.198)
 */
void WINAPI CascadeChildWindows( HWND16 parent, WORD action )
{
    FIXME(win, "(%04x, %d): stub\n", parent, action);
}
/***********************************************************************
 *           GetProgmanWindow			[USER32.289]
 */
HRESULT WINAPI GetProgmanWindow ( )
{	FIXME(win,"stub\n");
	return 0;
}
/***********************************************************************
 *           GetTaskmanWindow			[USER32.304]
 */
HRESULT WINAPI GetTaskmanWindow ( )
{	FIXME(win,"stub\n");
	return 0;
}
/***********************************************************************
 *           SetProgmanWindow			[USER32.522]
 */
HRESULT WINAPI SetProgmanWindow ( DWORD x )
{	FIXME(win,"0x%08lx stub\n",x);
	return 0;
}
/***********************************************************************
 *           SetShellWindowEx			[USER32.531]
 */
HRESULT WINAPI SetShellWindowEx ( DWORD x, DWORD y )
{	FIXME(win,"0x%08lx 0x%08lx stub\n",x,y);
	return 0;
}
/***********************************************************************
 *           SetTaskmanWindow			[USER32.537]
 */
HRESULT WINAPI SetTaskmanWindow ( DWORD x )
{	FIXME(win,"0x%08lx stub\n",x);
	return 0;
}
