/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995,1996 Alex Korobka
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "sysmetrics.h"
#include "heap.h"
#include "module.h"
#include "user.h"
#include "win.h"
#include "hook.h"
#include "message.h"
#include "queue.h"
#include "stackframe.h"
#include "options.h"
#include "winpos.h"
#include "dce.h"
#include "nonclient.h"
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

/* ----- external functions ----- */

extern void 	FOCUS_SwitchFocus( HWND , HWND );
extern HRGN 	DCE_GetVisRgn( HWND, WORD );
extern HWND	CARET_GetHwnd();
extern BOOL     DCE_InvalidateDCE(WND*, RECT16* );

/* ----- internal variables ----- */

static HWND hwndActive      = 0;  /* Currently active window */
static HWND hwndPrevActive  = 0;  /* Previously active window */

/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 * The new position is stored into wndPtr->ptIconPos.
 */
void WINPOS_FindIconPos( HWND hwnd )
{
    RECT16 rectParent;
    short x, y, xspacing, yspacing;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr || !wndPtr->parent) return;
    GetClientRect16( wndPtr->parent->hwndSelf, &rectParent );
    if ((wndPtr->ptIconPos.x >= rectParent.left) &&
        (wndPtr->ptIconPos.x + SYSMETRICS_CXICON < rectParent.right) &&
        (wndPtr->ptIconPos.y >= rectParent.top) &&
        (wndPtr->ptIconPos.y + SYSMETRICS_CYICON < rectParent.bottom))
        return;  /* The icon already has a suitable position */

    xspacing = yspacing = 70;  /* FIXME: This should come from WIN.INI */
    y = rectParent.bottom;
    for (;;)
    {
        for (x = rectParent.left; x<=rectParent.right-xspacing; x += xspacing)
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
            if (!childPtr)
            {
                  /* No window was found, so it's OK for us */
                wndPtr->ptIconPos.x = x + (xspacing - SYSMETRICS_CXICON) / 2;
                wndPtr->ptIconPos.y = y - (yspacing + SYSMETRICS_CYICON) / 2;
                return;
            }
        }
        y -= yspacing;
    }
}


/***********************************************************************
 *           ArrangeIconicWindows   (USER.170)
 */
UINT ArrangeIconicWindows( HWND parent )
{
    RECT16 rectParent;
    HWND hwndChild;
    INT x, y, xspacing, yspacing;

    GetClientRect16( parent, &rectParent );
    x = rectParent.left;
    y = rectParent.bottom;
    xspacing = yspacing = 70;  /* FIXME: This should come from WIN.INI */
    hwndChild = GetWindow( parent, GW_CHILD );
    while (hwndChild)
    {
        if (IsIconic( hwndChild ))
        {
            SetWindowPos( hwndChild, 0, x + (xspacing - SYSMETRICS_CXICON) / 2,
                          y - (yspacing + SYSMETRICS_CYICON) / 2, 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            if (x <= rectParent.right - xspacing) x += xspacing;
            else
            {
                x = rectParent.left;
                y -= yspacing;
            }
        }
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return yspacing;
}


/***********************************************************************
 *           GetWindowRect16   (USER.32)
 */
void GetWindowRect16( HWND16 hwnd, LPRECT16 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints16( wndPtr->parent->hwndSelf, 0, (POINT16 *)rect, 2 );
}


/***********************************************************************
 *           GetWindowRect32   (USER.32)
 */
void GetWindowRect32( HWND32 hwnd, LPRECT32 rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    CONV_RECT16TO32( &wndPtr->rectWindow, rect );
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints32( wndPtr->parent->hwndSelf, 0, (POINT32 *)rect, 2 );
}


/***********************************************************************
 *           GetClientRect16   (USER.33)
 */
void GetClientRect16( HWND16 hwnd, LPRECT16 rect ) 
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
 *           GetClientRect32   (USER32.219)
 */
void GetClientRect32( HWND32 hwnd, LPRECT32 rect ) 
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
BOOL16 ClientToScreen16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( hwnd, 0, lppnt, 1 );
    return TRUE;
}


/*******************************************************************
 *         ClientToScreen32   (USER32.51)
 */
BOOL32 ClientToScreen32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( hwnd, 0, lppnt, 1 );
    return TRUE;
}


/*******************************************************************
 *         ScreenToClient16   (USER.29)
 */
void ScreenToClient16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( 0, hwnd, lppnt, 1 );
}


/*******************************************************************
 *         ScreenToClient32   (USER32.446)
 */
void ScreenToClient32( HWND32 hwnd, LPPOINT32 lppnt )
{
    MapWindowPoints32( 0, hwnd, lppnt, 1 );
}


/***********************************************************************
 *           WINPOS_WindowFromPoint
 *
 * Find the window and hittest for a given point.
 */
INT16 WINPOS_WindowFromPoint( POINT16 pt, WND **ppWnd )
{
    WND *wndPtr;
    INT16 hittest = HTERROR;
    INT16 x, y;

    *ppWnd = NULL;
    x = pt.x;
    y = pt.y;
    wndPtr = WIN_GetDesktop()->child;
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
                (x >= wndPtr->rectWindow.left) &&
                (x < wndPtr->rectWindow.right) &&
                (y >= wndPtr->rectWindow.top) &&
                (y < wndPtr->rectWindow.bottom))
            {
                *ppWnd = wndPtr;  /* Got a suitable window */

                /* If window is minimized or disabled, return at once */
                if (wndPtr->dwStyle & WS_MINIMIZE) return HTCAPTION;
                if (wndPtr->dwStyle & WS_DISABLED) return HTERROR;

                /* If point is not in client area, ignore the children */
                if ((x < wndPtr->rectClient.left) ||
                    (x >= wndPtr->rectClient.right) ||
                    (y < wndPtr->rectClient.top) ||
                    (y >= wndPtr->rectClient.bottom)) break;

                x -= wndPtr->rectClient.left;
                y -= wndPtr->rectClient.top;
                wndPtr = wndPtr->child;
            }
            else wndPtr = wndPtr->next;
        }

        /* If nothing found, return the desktop window */
        if (!*ppWnd)
        {
            *ppWnd = WIN_GetDesktop();
            return HTCLIENT;
        }

        /* Send the WM_NCHITTEST message (only if to the same task) */
        if ((*ppWnd)->hmemTaskQ != GetTaskQueue(0)) return HTCLIENT;
        hittest = (INT)SendMessage16( (*ppWnd)->hwndSelf, WM_NCHITTEST, 0,
                                    MAKELONG( pt.x, pt.y ) );
        if (hittest != HTTRANSPARENT) return hittest;  /* Found the window */

        /* If no children found in last search, make point relative to parent*/
        if (!wndPtr)
        {
            x += (*ppWnd)->rectClient.left;
            y += (*ppWnd)->rectClient.top;
        }

        /* Restart the search from the next sibling */
        wndPtr = (*ppWnd)->next;
        *ppWnd = (*ppWnd)->parent;
    }
}


/*******************************************************************
 *         WindowFromPoint16   (USER.30)
 */
HWND16  WindowFromPoint16( POINT16 pt )
{
    WND *pWnd;
    WINPOS_WindowFromPoint( pt, &pWnd );
    return pWnd->hwndSelf;
}


/*******************************************************************
 *         WindowFromPoint32   (USER32.581)
 */
HWND32 WindowFromPoint32( POINT32 pt )
{
    WND *pWnd;
    POINT16 pt16;
    CONV_POINT32TO16( &pt, &pt16 );
    WINPOS_WindowFromPoint( pt16, &pWnd );
    return (HWND32)pWnd->hwndSelf;
}


/*******************************************************************
 *         ChildWindowFromPoint16   (USER.191)
 */
HWND16 ChildWindowFromPoint16( HWND16 hwndParent, POINT16 pt )
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT16 rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect16( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect16( &wnd->rectWindow, pt )) return wnd->hwndSelf;
        wnd = wnd->next;
    }
    return hwndParent;
}


/*******************************************************************
 *         ChildWindowFromPoint32   (USER32.)
 */
HWND32 ChildWindowFromPoint32( HWND32 hwndParent, POINT32 pt )
{
    POINT16 pt16;
    CONV_POINT32TO16( &pt, &pt16 );
    return (HWND32)ChildWindowFromPoint16( hwndParent, pt16 );
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
            fprintf(stderr,"MapWindowPoints: bad hwndFrom = %04x\n",hwndFrom);
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
            fprintf(stderr,"MapWindowPoints: bad hwndTo = %04x\n", hwndTo );
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
void MapWindowPoints16( HWND16 hwndFrom, HWND16 hwndTo,
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
 *         MapWindowPoints32   (USER32.385)
 */
void MapWindowPoints32( HWND32 hwndFrom, HWND32 hwndTo,
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
 *           IsIconic   (USER.31)
 */
BOOL IsIconic(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MINIMIZE) != 0;
}
 
 
/***********************************************************************
 *           IsZoomed   (USER.272)
 */
BOOL IsZoomed(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *         GetActiveWindow    (USER.60)
 */
HWND GetActiveWindow()
{
    return hwndActive;
}


/*******************************************************************
 *         SetActiveWindow    (USER.59)
 */
HWND SetActiveWindow( HWND hwnd )
{
    HWND prev = hwndActive;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr || (wndPtr->dwStyle & WS_DISABLED) ||
	!(wndPtr->dwStyle & WS_VISIBLE)) return 0;

    WINPOS_SetActiveWindow( hwnd, 0, 0 );
    return prev;
}


/***********************************************************************
 *           BringWindowToTop   (USER.45)
 */
BOOL BringWindowToTop( HWND hwnd )
{
    return SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}


/***********************************************************************
 *           MoveWindow   (USER.56)
 */
BOOL MoveWindow( HWND hwnd, short x, short y, short cx, short cy, BOOL repaint)
{    
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
    dprintf_win(stddeb, "MoveWindow: %04x %d,%d %dx%d %d\n", 
	    hwnd, x, y, cx, cy, repaint );
    return SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}


/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL ShowWindow( HWND hwnd, int cmd ) 
{    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    BOOL32 wasVisible, showFlag;
    POINT16 maxSize;
    int swpflags = 0;
    short x = 0, y = 0, cx = 0, cy = 0;

    if (!wndPtr) return FALSE;

    dprintf_win(stddeb,"ShowWindow: hwnd=%04x, cmd=%d\n", hwnd, cmd);

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
	    swpflags |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swpflags |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swpflags |= SWP_FRAMECHANGED;
            if (!(wndPtr->dwStyle & WS_MINIMIZE))
            {
                if (wndPtr->dwStyle & WS_MAXIMIZE)
                {
                    wndPtr->flags |= WIN_RESTORE_MAX;
                    wndPtr->dwStyle &= ~WS_MAXIMIZE;
                }
                else
                {
                    wndPtr->flags &= ~WIN_RESTORE_MAX;
                    wndPtr->rectNormal = wndPtr->rectWindow;
                }
                wndPtr->dwStyle |= WS_MINIMIZE;
                WINPOS_FindIconPos( hwnd );
                x  = wndPtr->ptIconPos.x;
                y  = wndPtr->ptIconPos.y;
                cx = SYSMETRICS_CXICON;
                cy = SYSMETRICS_CYICON;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE: */
            swpflags |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if (!(wndPtr->dwStyle & WS_MAXIMIZE))
            {
                  /* Store the current position and find the maximized size */
                if (!(wndPtr->dwStyle & WS_MINIMIZE))
                    wndPtr->rectNormal = wndPtr->rectWindow; 

                NC_GetMinMaxInfo( hwnd, &maxSize,
                                  &wndPtr->ptMaxPos, NULL, NULL );
                x  = wndPtr->ptMaxPos.x;
                y  = wndPtr->ptMaxPos.y;

		if( wndPtr->dwStyle & WS_MINIMIZE )
		    if( !SendMessage16( hwnd, WM_QUERYOPEN, 0, 0L ) )
			{
		         swpflags |= SWP_NOSIZE | SWP_NOMOVE;
			 break;
			}

                cx = maxSize.x;
                cy = maxSize.y;
                wndPtr->dwStyle &= ~WS_MINIMIZE;
                wndPtr->dwStyle |= WS_MAXIMIZE;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOW:
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWNOACTIVATE:
            swpflags |= SWP_NOZORDER;
            if (GetActiveWindow()) swpflags |= SWP_NOACTIVATE;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	case SW_RESTORE:
	    swpflags |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;

            if (wndPtr->dwStyle & WS_MINIMIZE)
            {
                if( !SendMessage16( hwnd, WM_QUERYOPEN, 0, 0L) )
                  {
                    swpflags |= SWP_NOSIZE | SWP_NOMOVE;
                    break;
                  }
                wndPtr->ptIconPos.x = wndPtr->rectWindow.left;
                wndPtr->ptIconPos.y = wndPtr->rectWindow.top;
                wndPtr->dwStyle &= ~WS_MINIMIZE;
                if (wndPtr->flags & WIN_RESTORE_MAX)
                {
                    /* Restore to maximized position */
                    NC_GetMinMaxInfo( hwnd, &maxSize, &wndPtr->ptMaxPos,
                                      NULL, NULL );
                    x  = wndPtr->ptMaxPos.x;
                    y  = wndPtr->ptMaxPos.y;
                    cx = maxSize.x;
                    cy = maxSize.y;
                   wndPtr->dwStyle |= WS_MAXIMIZE;
                }
                else  /* Restore to normal position */
                {
                    x  = wndPtr->rectNormal.left;
                    y  = wndPtr->rectNormal.top;
                    cx = wndPtr->rectNormal.right - wndPtr->rectNormal.left;
                    cy = wndPtr->rectNormal.bottom - wndPtr->rectNormal.top;
                }
            }
            else if (wndPtr->dwStyle & WS_MAXIMIZE)
            {
                wndPtr->ptMaxPos.x = wndPtr->rectWindow.left;
                wndPtr->ptMaxPos.y = wndPtr->rectWindow.top;
                wndPtr->dwStyle &= ~WS_MAXIMIZE;
                x  = wndPtr->rectNormal.left;
                y  = wndPtr->rectNormal.top;
                cx = wndPtr->rectNormal.right - wndPtr->rectNormal.left;
                cy = wndPtr->rectNormal.bottom - wndPtr->rectNormal.top;
            }
            else swpflags |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible)
    {
        SendMessage16( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) return wasVisible;
    }

    if ((wndPtr->dwStyle & WS_CHILD) &&
        !IsWindowVisible( wndPtr->parent->hwndSelf ) &&
        (swpflags & SWP_NOSIZE) && (swpflags & SWP_NOMOVE))
    {
        /* Don't call SetWindowPos() on invisible child windows */
        if (cmd == SW_HIDE) wndPtr->dwStyle &= ~WS_VISIBLE;
        else wndPtr->dwStyle |= WS_VISIBLE;
    }
    else
    {
        /* We can't activate a child window */
        if (wndPtr->dwStyle & WS_CHILD)
            swpflags |= SWP_NOACTIVATE | SWP_NOZORDER;
        SetWindowPos( hwnd, HWND_TOP, x, y, cx, cy, swpflags );
        if (!IsWindow( hwnd )) return wasVisible;
    }

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
	SendMessage16( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessage16( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

    if (hwnd == GetFocus())
    { 
	SetFocus( (wndPtr->dwStyle & WS_CHILD)? wndPtr->parent->hwndSelf: 0 );
	if (hwnd == CARET_GetHwnd()) DestroyCaret();
    }

    return wasVisible;
}


/***********************************************************************
 *           GetInternalWindowPos16   (USER.460)
 */
UINT16 GetInternalWindowPos16( HWND16 hwnd, LPRECT16 rectWnd, LPPOINT16 ptIcon)
{
    WINDOWPLACEMENT16 wndpl;
    if (!GetWindowPlacement16( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/***********************************************************************
 *           GetInternalWindowPos32   (USER32.244)
 */
UINT32 GetInternalWindowPos32( HWND32 hwnd, LPRECT32 rectWnd, LPPOINT32 ptIcon)
{
    WINDOWPLACEMENT32 wndpl;
    if (!GetWindowPlacement32( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/***********************************************************************
 *           SetInternalWindowPos16   (USER.461)
 */
void SetInternalWindowPos16( HWND16 hwnd, UINT16 showCmd,
                             LPRECT16 rect, LPPOINT16 pt )
{
    WINDOWPLACEMENT16 wndpl;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    wndpl.length  = sizeof(wndpl);
    wndpl.flags   = (pt != NULL) ? WPF_SETMINPOSITION : 0;
    wndpl.showCmd = showCmd;
    if (pt) wndpl.ptMinPosition = *pt;
    wndpl.rcNormalPosition = (rect != NULL) ? *rect : wndPtr->rectNormal;
    wndpl.ptMaxPosition = wndPtr->ptMaxPos;
    SetWindowPlacement16( hwnd, &wndpl );
}


/***********************************************************************
 *           SetInternalWindowPos32   (USER32.482)
 */
void SetInternalWindowPos32( HWND32 hwnd, UINT32 showCmd,
                             LPRECT32 rect, LPPOINT32 pt )
{
    WINDOWPLACEMENT32 wndpl;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    wndpl.length  = sizeof(wndpl);
    wndpl.flags   = (pt != NULL) ? WPF_SETMINPOSITION : 0;
    wndpl.showCmd = showCmd;
    if (pt) wndpl.ptMinPosition = *pt;
    if (rect) wndpl.rcNormalPosition = *rect;
    else CONV_RECT16TO32( &wndPtr->rectNormal, &wndpl.rcNormalPosition );
    CONV_POINT16TO32( &wndPtr->ptMaxPos, &wndpl.ptMaxPosition );
    SetWindowPlacement32( hwnd, &wndpl );
}


/***********************************************************************
 *           GetWindowPlacement16   (USER.370)
 */
BOOL16 GetWindowPlacement16( HWND16 hwnd, WINDOWPLACEMENT16 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    wndpl->length  = sizeof(*wndpl);
    wndpl->flags   = 0;
    wndpl->showCmd = IsZoomed(hwnd) ? SW_SHOWMAXIMIZED : 
	             (IsIconic(hwnd) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL);
    wndpl->ptMinPosition = wndPtr->ptIconPos;
    wndpl->ptMaxPosition = wndPtr->ptMaxPos;
    wndpl->rcNormalPosition = wndPtr->rectNormal;
    return TRUE;
}


/***********************************************************************
 *           GetWindowPlacement32   (USER32.306)
 */
BOOL32 GetWindowPlacement32( HWND32 hwnd, WINDOWPLACEMENT32 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    wndpl->length  = sizeof(*wndpl);
    wndpl->flags   = 0;
    wndpl->showCmd = IsZoomed(hwnd) ? SW_SHOWMAXIMIZED : 
	             (IsIconic(hwnd) ? SW_SHOWMINIMIZED : SW_SHOWNORMAL);
    CONV_POINT16TO32( &wndPtr->ptIconPos, &wndpl->ptMinPosition );
    CONV_POINT16TO32( &wndPtr->ptMaxPos, &wndpl->ptMaxPosition );
    CONV_RECT16TO32( &wndPtr->rectNormal, &wndpl->rcNormalPosition );
    return TRUE;
}


/***********************************************************************
 *           SetWindowPlacement16   (USER.371)
 */
BOOL16 SetWindowPlacement16( HWND16 hwnd, const WINDOWPLACEMENT16 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (wndpl->flags & WPF_SETMINPOSITION)
	wndPtr->ptIconPos = wndpl->ptMinPosition;
    if ((wndpl->flags & WPF_RESTORETOMAXIMIZED) &&
	(wndpl->showCmd == SW_SHOWMINIMIZED)) wndPtr->flags |= WIN_RESTORE_MAX;
    wndPtr->ptMaxPos   = wndpl->ptMaxPosition;
    wndPtr->rectNormal = wndpl->rcNormalPosition;
    ShowWindow( hwnd, wndpl->showCmd );
    return TRUE;
}


/***********************************************************************
 *           SetWindowPlacement32   (USER32.518)
 */
BOOL32 SetWindowPlacement32( HWND32 hwnd, const WINDOWPLACEMENT32 *wndpl )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (wndpl->flags & WPF_SETMINPOSITION)
	CONV_POINT32TO16( &wndpl->ptMinPosition, &wndPtr->ptIconPos );
    if ((wndpl->flags & WPF_RESTORETOMAXIMIZED) &&
	(wndpl->showCmd == SW_SHOWMINIMIZED)) wndPtr->flags |= WIN_RESTORE_MAX;
    CONV_POINT32TO16( &wndpl->ptMaxPosition, &wndPtr->ptMaxPos );
    CONV_RECT32TO16( &wndpl->rcNormalPosition, &wndPtr->rectNormal );
    ShowWindow( hwnd, wndpl->showCmd );
    return TRUE;
}


/*******************************************************************
 *	   WINPOS_SetActiveWindow
 *
 * back-end to SetActiveWindow
 */
BOOL WINPOS_SetActiveWindow( HWND hWnd, BOOL fMouse, BOOL fChangeFocus )
{
    WND                   *wndPtr          = WIN_FindWndPtr(hWnd);
    WND                   *wndTemp         = WIN_FindWndPtr(hwndActive);
    CBTACTIVATESTRUCT16   *cbtStruct;
    WORD                   wIconized=0;

    /* FIXME: When proper support for cooperative multitasking is in place 
     *        hActiveQ will be global 
     */

    HANDLE                 hActiveQ = 0;   

    /* paranoid checks */
    if( !hWnd || hWnd == GetDesktopWindow() || hWnd == hwndActive )
	return 0;

    if( GetTaskQueue(0) != wndPtr->hmemTaskQ )
	return 0;

    if( wndTemp )
	wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
    else
	dprintf_win(stddeb,"WINPOS_ActivateWindow: no current active window.\n");

    /* call CBT hook chain */
    if ((cbtStruct = SEGPTR_NEW(CBTACTIVATESTRUCT16)))
    {
        LRESULT wRet;
        cbtStruct->fMouse     = fMouse;
        cbtStruct->hWndActive = hwndActive;
        wRet = HOOK_CallHooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hWnd,
                               (LPARAM)SEGPTR_GET(cbtStruct) );
        SEGPTR_FREE(cbtStruct);
        if (wRet) return wRet;
    }

    /* set prev active wnd to current active wnd and send notification */
    if ((hwndPrevActive = hwndActive) && IsWindow(hwndPrevActive))
    {
        if (!SendMessage16( hwndPrevActive, WM_NCACTIVATE, FALSE, 0 ))
        {
	    if (GetSysModalWindow16() != hWnd) return 0;
	    /* disregard refusal if hWnd is sysmodal */
        }

	SendMessage32A( hwndPrevActive, WM_ACTIVATE,
                        MAKEWPARAM( WA_INACTIVE, wIconized ),
                        (LPARAM)hWnd );

	/* check if something happened during message processing */
	if( hwndPrevActive != hwndActive ) return 0;
    }

    /* set active wnd */
    hwndActive = hWnd;

    /* send palette messages */
    if( SendMessage16( hWnd, WM_QUERYNEWPALETTE, 0, 0L) )
	SendMessage16((HWND16)-1, WM_PALETTEISCHANGING, (WPARAM)hWnd, 0L );

    /* if prev wnd is minimized redraw icon title 
  if( hwndPrevActive )
    {
        wndTemp = WIN_FindWndPtr( WIN_GetTopParent( hwndPrevActive ) );
        if(wndTemp)
          if(wndTemp->dwStyle & WS_MINIMIZE)
            RedrawIconTitle(hwndPrevActive); 
      } 
  */

    /* managed windows will get ConfigureNotify event */  
    if (!(wndPtr->dwStyle & WS_CHILD) && !(wndPtr->flags & WIN_MANAGED))
    {
	/* check Z-order and bring hWnd to the top */
	for (wndTemp = WIN_GetDesktop()->child; wndTemp; wndTemp = wndTemp->next)
	    if (wndTemp->dwStyle & WS_VISIBLE) break;

	if( wndTemp != wndPtr )
	    SetWindowPos(hWnd, HWND_TOP, 0,0,0,0, 
			 SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
    }

    if( !IsWindow(hWnd) ) return 0;

    if (hwndPrevActive)
    {
        wndTemp = WIN_FindWndPtr( hwndPrevActive );
        if (wndTemp) hActiveQ = wndTemp->hmemTaskQ;
    }

    /* send WM_ACTIVATEAPP if necessary */
    if (hActiveQ != wndPtr->hmemTaskQ)
    {
        WND **list, **ppWnd;

        if ((list = WIN_BuildWinArray( WIN_GetDesktop() )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                /* Make sure that the window still exists */
                if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
                if ((*ppWnd)->hmemTaskQ != hActiveQ) continue;
                SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                               0, QUEUE_GetQueueTask(wndPtr->hmemTaskQ) );
            }
            HeapFree( SystemHeap, 0, list );
        }

        if ((list = WIN_BuildWinArray( WIN_GetDesktop() )))
        {
            for (ppWnd = list; *ppWnd; ppWnd++)
            {
                /* Make sure that the window still exists */
                if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
                if ((*ppWnd)->hmemTaskQ != wndPtr->hmemTaskQ) continue;
                SendMessage16( (*ppWnd)->hwndSelf, WM_ACTIVATEAPP,
                               1, QUEUE_GetQueueTask( hActiveQ ) );
            }
            HeapFree( SystemHeap, 0, list );
        }
	if (!IsWindow(hWnd)) return 0;
    }

    /* walk up to the first unowned window */
    wndTemp = wndPtr;
    while (wndTemp->owner) wndTemp = wndTemp->owner;
    /* and set last active owned popup */
    wndTemp->hwndLastActive = hWnd;

    wIconized = HIWORD(wndTemp->dwStyle & WS_MINIMIZE);
    SendMessage16( hWnd, WM_NCACTIVATE, TRUE, 0 );
    SendMessage32A( hWnd, WM_ACTIVATE,
		 MAKEWPARAM( (fMouse) ? WA_CLICKACTIVE : WA_ACTIVE, wIconized),
		 (LPARAM)hwndPrevActive );

    if( !IsWindow(hWnd) ) return 0;

    /* change focus if possible */
    if( fChangeFocus && GetFocus() )
	if( WIN_GetTopParent(GetFocus()) != hwndActive )
	    FOCUS_SwitchFocus( GetFocus(),
			       (wndPtr->dwStyle & WS_MINIMIZE)? 0: hwndActive);

    /* if active wnd is minimized redraw icon title 
  if( hwndActive )
      {
        wndPtr = WIN_FindWndPtr(hwndActive);
        if(wndPtr->dwStyle & WS_MINIMIZE)
           RedrawIconTitle(hwndActive);
    }
  */
    return (hWnd == hwndActive);
}


/*******************************************************************
 *	   WINPOS_ChangeActiveWindow
 *
 */
BOOL WINPOS_ChangeActiveWindow( HWND hWnd, BOOL mouseMsg )
{
    WND *wndPtr = WIN_FindWndPtr(hWnd);

    if( !wndPtr ) return FALSE;

    /* child windows get WM_CHILDACTIVATE message */
    if( (wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD )
	return SendMessage16(hWnd, WM_CHILDACTIVATE, 0, 0L);

        /* owned popups imply owner activation - not sure */
    if ((wndPtr->dwStyle & WS_POPUP) && wndPtr->owner &&
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
LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect,
                            RECT16 *newWindowRect, RECT16 *oldWindowRect,
                            RECT16 *oldClientRect, SEGPTR winpos,
                            RECT16 *newClientRect )
{
    NCCALCSIZE_PARAMS16 *params;
    LONG result;

    if (!(params = SEGPTR_NEW(NCCALCSIZE_PARAMS16))) return 0;
    params->rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
	params->rgrc[1] = *oldWindowRect;
	params->rgrc[2] = *oldClientRect;
	params->lppos = winpos;
    }
    result = SendMessage16( hwnd, WM_NCCALCSIZE, calcValidRect,
                          (LPARAM)SEGPTR_GET( params ) );
    dprintf_win(stddeb, "WINPOS_SendNCCalcSize: %d %d %d %d\n",
		(int)params->rgrc[0].top,    (int)params->rgrc[0].left,
		(int)params->rgrc[0].bottom, (int)params->rgrc[0].right);
    *newClientRect = params->rgrc[0];
    SEGPTR_FREE(params);
    return result;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging16
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging16( WND *wndPtr, WINDOWPOS16 *winpos )
{
    POINT16 maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	NC_GetMinMaxInfo( winpos->hwnd, &maxSize, NULL, NULL, NULL );
	winpos->cx = MIN( winpos->cx, maxSize.x );
	winpos->cy = MIN( winpos->cy, maxSize.y );
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
    POINT16 maxSize;
    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) == 0))
    {
	NC_GetMinMaxInfo( winpos->hwnd, &maxSize, NULL, NULL, NULL );
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
static void WINPOS_MoveWindowZOrder( HWND hwnd, HWND hwndAfter )
{
    BOOL movingUp;
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
            RECT16 rect = pWndCur->rectWindow;
            OffsetRect16( &rect, -wndPtr->rectClient.left,
                          -wndPtr->rectClient.top );
            RedrawWindow16( hwnd, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                            RDW_FRAME | RDW_ERASE );
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
            RECT16 rect = wndPtr->rectWindow;
            OffsetRect16( &rect, -pWndCur->rectClient.left,
                          -pWndCur->rectClient.top );
            RedrawWindow16( pWndCur->hwndSelf, &rect, 0, RDW_INVALIDATE |
                            RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE );
            pWndCur = pWndCur->next;
        }
    }
}

/***********************************************************************
 *           WINPOS_ReorderOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above owner window
 */
HWND WINPOS_ReorderOwnedPopups(HWND hwndInsertAfter, WND* wndPtr, WORD flags)
{
 WND* 	w = WIN_GetDesktop();

 w = w->child;

 /* if we are dealing with owned popup... 
  */
 if( wndPtr->dwStyle & WS_POPUP && wndPtr->owner && hwndInsertAfter != HWND_TOP )
   {
     BOOL bFound = FALSE;
     HWND hwndLocalPrev = HWND_TOP;
     HWND hwndNewAfter = 0;

     while( w )
       {
         if( !bFound && hwndInsertAfter == hwndLocalPrev )
             hwndInsertAfter = HWND_TOP;

         if( w->dwStyle & WS_POPUP && w->owner == wndPtr->owner )
           {
             bFound = TRUE;

             if( hwndInsertAfter == HWND_TOP )
               {
                 hwndInsertAfter = hwndLocalPrev;
                 break;
               }
             hwndNewAfter = hwndLocalPrev;
           }

         if( w == wndPtr->owner )
           {
             /* basically HWND_BOTTOM */
             hwndInsertAfter = hwndLocalPrev;

             if( bFound )
                 hwndInsertAfter = hwndNewAfter;
             break;
           }

           if( w != wndPtr )
               hwndLocalPrev = w->hwndSelf;

           w = w->next;
        }
   }
 else 
   /* or overlapped top-level window... 
    */
   if( !(wndPtr->dwStyle & WS_CHILD) )
      while( w )
        {
          if( w == wndPtr ) break;

          if( w->dwStyle & WS_POPUP && w->owner == wndPtr )
            {
              SetWindowPos(w->hwndSelf, hwndInsertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
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
 */
static void WINPOS_SizeMoveClean(WND* Wnd, HRGN oldVisRgn, LPRECT16 lpOldWndRect, LPRECT16 lpOldClientRect, BOOL bNoCopy )
{
 HRGN newVisRgn    = DCE_GetVisRgn(Wnd->hwndSelf, DCX_WINDOW | DCX_CLIPSIBLINGS );
 HRGN dirtyRgn     = CreateRectRgn(0,0,0,0);
 int  other, my;

 dprintf_win(stddeb,"cleaning up...new wnd=(%i %i-%i %i) old wnd=(%i %i-%i %i)\n\
\t\tnew client=(%i %i-%i %i) old client=(%i %i-%i %i)\n",
		   Wnd->rectWindow.left, Wnd->rectWindow.top, Wnd->rectWindow.right, Wnd->rectWindow.bottom,
		   lpOldWndRect->left, lpOldWndRect->top, lpOldWndRect->right, lpOldWndRect->bottom,
		   Wnd->rectClient.left,Wnd->rectClient.top,Wnd->rectClient.right,Wnd->rectClient.bottom,
		   lpOldClientRect->left,lpOldClientRect->top,lpOldClientRect->right,lpOldClientRect->bottom);

 CombineRgn( dirtyRgn, newVisRgn, 0, RGN_COPY);

 if( !bNoCopy )
   {
     HRGN hRgn = CreateRectRgn( lpOldClientRect->left - lpOldWndRect->left, lpOldClientRect->top - lpOldWndRect->top,
				lpOldClientRect->right - lpOldWndRect->left, lpOldClientRect->bottom - lpOldWndRect->top);
     CombineRgn( newVisRgn, newVisRgn, oldVisRgn, RGN_AND ); 
     CombineRgn( newVisRgn, newVisRgn, hRgn, RGN_AND );
     DeleteObject(hRgn);
   }

 /* map regions to the parent client area */
 
 OffsetRgn(dirtyRgn, Wnd->rectWindow.left, Wnd->rectWindow.top);
 OffsetRgn(oldVisRgn, lpOldWndRect->left, lpOldWndRect->top);

 /* compute invalidated region outside Wnd - (in client coordinates of the parent window) */

 other = CombineRgn(dirtyRgn, oldVisRgn, dirtyRgn, RGN_DIFF);

 /* map visible region to the Wnd client area */

 OffsetRgn( newVisRgn, Wnd->rectWindow.left - Wnd->rectClient.left,
                       Wnd->rectWindow.top - Wnd->rectClient.top );

 /* substract previously invalidated region from the Wnd visible region */

 my =  (Wnd->hrgnUpdate > 1)? CombineRgn( newVisRgn, newVisRgn, Wnd->hrgnUpdate, RGN_DIFF)
                            : COMPLEXREGION;

 if( bNoCopy )		/* invalidate Wnd visible region */
   {
     if (my != NULLREGION)  RedrawWindow32( Wnd->hwndSelf, NULL, newVisRgn, RDW_INVALIDATE |
		            RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE );
   } 
 else			/* bitblt old client area */
   { 
     HDC   hDC;
     int   update;
     HRGN  updateRgn;

     /* client rect */

     updateRgn = CreateRectRgn( 0,0, Wnd->rectClient.right - Wnd->rectClient.left,
				Wnd->rectClient.bottom - Wnd->rectClient.top );

     /* clip visible region with client rect */

     my = CombineRgn( newVisRgn, newVisRgn, updateRgn, RGN_AND );

     /* substract result from client rect to get region that won't be copied */

     update = CombineRgn( updateRgn, updateRgn, newVisRgn, RGN_DIFF );

     /* Blt valid bits using parent window DC */

     if( my != NULLREGION )
       {
	 int xfrom = lpOldClientRect->left;
	 int yfrom = lpOldClientRect->top;
	 int xto = Wnd->rectClient.left;
	 int yto = Wnd->rectClient.top;

	 /* check if we can skip copying */

	 if( xfrom != xto || yfrom != yto )
	   {
	     /* compute clipping region in parent client coordinates */

	     OffsetRgn( newVisRgn, Wnd->rectClient.left, Wnd->rectClient.top);
	     CombineRgn( oldVisRgn, oldVisRgn, newVisRgn, RGN_OR );

             hDC = GetDCEx( Wnd->parent->hwndSelf, oldVisRgn, DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CACHE | DCX_CLIPSIBLINGS);

             BitBlt(hDC, xto, yto, lpOldClientRect->right - lpOldClientRect->left, 
				   lpOldClientRect->bottom - lpOldClientRect->top,
				   hDC, xfrom, yfrom, SRCCOPY );
    
             ReleaseDC( Wnd->parent->hwndSelf, hDC); 
	  }
       }

     if( update != NULLREGION )
         RedrawWindow32( Wnd->hwndSelf, NULL, updateRgn, RDW_INVALIDATE |
                         RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE );
     DeleteObject( updateRgn );
   }

 /* erase uncovered areas */

 if( other != NULLREGION )
     RedrawWindow32( Wnd->parent->hwndSelf, NULL, dirtyRgn,
                     RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE );

 DeleteObject(dirtyRgn);
 DeleteObject(newVisRgn);
}


/***********************************************************************
 *           WINPOS_ForceXWindowRaise
 */
void WINPOS_ForceXWindowRaise( WND* pWnd )
{
    XWindowChanges winChanges;
    WND *wndStop, *wndLast;

    if (!pWnd->window) return;
        
    wndLast = wndStop = pWnd;
    winChanges.stack_mode = Above;
    XReconfigureWMWindow( display, pWnd->window, 0, CWStackMode, &winChanges );

    /* Recursively raise owned popups according to their z-order 
     * (it would be easier with sibling-related Below but it doesn't
     * work very well with SGI mwm for instance)
     */
    while (wndLast)
    {
        WND *wnd = WIN_GetDesktop()->child;
        wndLast = NULL;
        while (wnd != wndStop)
        {
            if (wnd->owner == pWnd &&
                (wnd->dwStyle & WS_POPUP) &&
                (wnd->dwStyle & WS_VISIBLE))
                wndLast = wnd;
            wnd = wnd->next;
        }
        if (wndLast)
        {
            WINPOS_ForceXWindowRaise( wndLast );
            wndStop = wndLast;
        }
    }
}


/***********************************************************************
 *           WINPOS_SetXWindowPos
 *
 * SetWindowPos() for an X window. Used by the real SetWindowPos().
 */
static void WINPOS_SetXWindowPos( WINDOWPOS16 *winpos )
{
    XWindowChanges winChanges;
    int changeMask = 0;
    WND *wndPtr = WIN_FindWndPtr( winpos->hwnd );

    if (!(winpos->flags & SWP_NOSIZE))
    {
        winChanges.width     = winpos->cx;
        winChanges.height    = winpos->cy;
        changeMask |= CWWidth | CWHeight;

        /* Tweak dialog window size hints */

        if ((wndPtr->flags & WIN_MANAGED) &&
            (wndPtr->dwExStyle & WS_EX_DLGMODALFRAME))
        {
            XSizeHints *size_hints = XAllocSizeHints();

            if (size_hints)
            {
                long supplied_return;

                XGetWMSizeHints( display, wndPtr->window, size_hints,
                                 &supplied_return, XA_WM_NORMAL_HINTS);
                size_hints->min_width = size_hints->max_width = winpos->cx;
                size_hints->min_height = size_hints->max_height = winpos->cy;
                XSetWMSizeHints( display, wndPtr->window, size_hints,
                                 XA_WM_NORMAL_HINTS );
                XFree(size_hints);
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
        if (winpos->hwndInsertAfter == HWND_TOP) winChanges.stack_mode = Above;
        else winChanges.stack_mode = Below;
        if ((winpos->hwndInsertAfter != HWND_TOP) &&
            (winpos->hwndInsertAfter != HWND_BOTTOM))
        {
            WND * insertPtr = WIN_FindWndPtr( winpos->hwndInsertAfter );
            winChanges.sibling = insertPtr->window;
            changeMask |= CWSibling;
        }
        changeMask |= CWStackMode;
    }
    if (!changeMask) return;
    if (wndPtr->flags & WIN_MANAGED)
        XReconfigureWMWindow( display, wndPtr->window, 0,
                              changeMask, &winChanges );
    else XConfigureWindow( display, wndPtr->window, changeMask, &winChanges );
}


/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
BOOL SetWindowPos( HWND hwnd, HWND hwndInsertAfter, INT x, INT y,
		   INT cx, INT cy, WORD flags )
{
    WINDOWPOS16 winpos;
    WND *	wndPtr;
    RECT16 	newWindowRect, newClientRect;
    HRGN	visRgn = 0;
    int 	result = 0;

    dprintf_win(stddeb,"SetWindowPos: hwnd %04x, (%i,%i)-(%i,%i) flags %08x\n", 
						 hwnd, x, y, x+cx, y+cy, flags);  
      /* Check window handle */

    if (hwnd == GetDesktopWindow()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

    if (wndPtr->dwStyle & WS_VISIBLE) flags &= ~SWP_SHOWWINDOW;
    else
    {
	flags &= ~SWP_HIDEWINDOW;
	if (!(flags & SWP_SHOWWINDOW)) flags |= SWP_NOREDRAW;
    }

/*     Check for windows that may not be resized 
       FIXME: this should be done only for Windows 3.0 programs 
 */    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW ) )
           flags |= SWP_NOSIZE | SWP_NOMOVE;

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
	 if( wnd->parent != wndPtr->parent ) return FALSE;
	 if( wnd->next == wndPtr ) flags |= SWP_NOZORDER;
       }
    else
       if (hwndInsertAfter == HWND_TOP)
	   flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
       else /* HWND_BOTTOM */
	   flags |= ( wndPtr->next )? 0: SWP_NOZORDER;

      /* Fill the WINDOWPOS structure */

    winpos.hwnd = hwnd;
    winpos.hwndInsertAfter = hwndInsertAfter;
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    winpos.flags = flags;
    
      /* Send WM_WINDOWPOSCHANGING message */

    if (!(flags & SWP_NOSENDCHANGING))
	SendMessage16( hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)MAKE_SEGPTR(&winpos) );

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = wndPtr->rectClient;

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

	OffsetRect16( &newClientRect, winpos.x - wndPtr->rectWindow.left, 
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
							 wndPtr, flags );

        if (wndPtr->window)
        {
            WIN_UnlinkWindow( winpos.hwnd );
            WIN_LinkWindow( winpos.hwnd, hwndInsertAfter );
        }
        else WINPOS_MoveWindowZOrder( winpos.hwnd, hwndInsertAfter );
    }

    if ( !wndPtr->window && !(flags & SWP_NOREDRAW) && 
        (!(flags & SWP_NOMOVE) || !(flags & SWP_NOSIZE) || (flags & SWP_FRAMECHANGED)) )
          visRgn = DCE_GetVisRgn(hwnd, DCX_WINDOW | DCX_CLIPSIBLINGS);


      /* Send WM_NCCALCSIZE message to get new client area */
    if( (flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
      {
         result = WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    MAKE_SEGPTR(&winpos), &newClientRect );

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

    /* Update active DCEs */

    if( !(flags & SWP_NOZORDER) || (flags & SWP_HIDEWINDOW) || (flags & SWP_SHOWWINDOW)
                                || (memcmp(&newWindowRect,&wndPtr->rectWindow,sizeof(RECT16))
                                    && wndPtr->dwStyle & WS_VISIBLE ) )
      {
        RECT16 rect;

        UnionRect16(&rect,&newWindowRect,&wndPtr->rectWindow);
        DCE_InvalidateDCE(wndPtr->parent, &rect);
      }

    /* Perform the moving and resizing */

    if (wndPtr->window)
    {
        RECT16 oldWindowRect = wndPtr->rectWindow;
        RECT16 oldClientRect = wndPtr->rectClient;

        HWND bogusInsertAfter = winpos.hwndInsertAfter;

        winpos.hwndInsertAfter = hwndInsertAfter;
        WINPOS_SetXWindowPos( &winpos );

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;
        winpos.hwndInsertAfter = bogusInsertAfter;

	/*  FIXME: should do something like WINPOS_SizeMoveClean */

	if( (oldClientRect.left - oldWindowRect.left !=
	     newClientRect.left - newWindowRect.left) ||
	    (oldClientRect.top - oldWindowRect.top !=
	     newClientRect.top - newWindowRect.top) )

	    RedrawWindow32( wndPtr->hwndSelf, NULL, 0, RDW_INVALIDATE |
                            RDW_ALLCHILDREN | RDW_FRAME | RDW_ERASE );
	else
	    if( winpos.flags & SWP_FRAMECHANGED )
	      {
		WORD wErase = 0;
		RECT32 rect;

	        if( oldClientRect.right > newClientRect.right ) 
                {
		    rect.left = newClientRect.right; rect.top = newClientRect.top;
		    rect.right = oldClientRect.right; rect.bottom = newClientRect.bottom;
		    wErase = 1;
		    RedrawWindow32( wndPtr->hwndSelf, &rect, 0,
                                RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN );
                }
		if( oldClientRect.bottom > newClientRect.bottom )
                {
		    rect.left = newClientRect.left; rect.top = newClientRect.bottom;
		    rect.right = (wErase)?oldClientRect.right:newClientRect.right;
		    rect.bottom = oldClientRect.bottom;
		    wErase = 1;
		    RedrawWindow32( wndPtr->hwndSelf, &rect, 0,
                                RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN );
                }
		if( !wErase ) wndPtr->flags |= WIN_NEEDS_NCPAINT;
	      }
    }
    else
    {
        RECT16 oldWindowRect = wndPtr->rectWindow;
	RECT16 oldClientRect = wndPtr->rectClient;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

        if( !(flags & SWP_NOREDRAW) )
	  {
	    BOOL bNoCopy = (flags & SWP_NOCOPYBITS) || 
			   (result >= WVR_HREDRAW && result < WVR_VALIDRECTS);

	    if( (winpos.flags & SWP_AGG_NOGEOMETRYCHANGE) != SWP_AGG_NOGEOMETRYCHANGE )
	        /* optimize cleanup by BitBlt'ing where possible */

	        WINPOS_SizeMoveClean(wndPtr, visRgn, &oldWindowRect, &oldClientRect, bNoCopy);
	    else
	       if( winpos.flags & SWP_FRAMECHANGED )
        	  RedrawWindow32( winpos.hwnd, NULL, 0, RDW_NOCHILDREN | RDW_FRAME ); 

	  }
        DeleteObject(visRgn);
    }

    if (flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
        if (wndPtr->window)
        {
            XMapWindow( display, wndPtr->window );
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
                RedrawWindow32( winpos.hwnd, NULL, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN |
                                RDW_FRAME | RDW_ERASE );
        }
    }
    else if (flags & SWP_HIDEWINDOW)
    {
	wndPtr->dwStyle &= ~WS_VISIBLE;
        if (wndPtr->window)
        {
            XUnmapWindow( display, wndPtr->window );
        }
        else
        {
            if (!(flags & SWP_NOREDRAW))
                RedrawWindow16( wndPtr->parent->hwndSelf, &wndPtr->rectWindow, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE );
        }

        if ((winpos.hwnd == GetFocus()) || IsChild(winpos.hwnd, GetFocus()))
            SetFocus( GetParent(winpos.hwnd) );  /* Revert focus to parent */

	if (winpos.hwnd == hwndActive)
	{
	      /* Activate previously active window if possible */
	    HWND newActive = hwndPrevActive;
	    if (!IsWindow(newActive) || (newActive == winpos.hwnd))
	    {
		newActive = GetTopWindow( GetDesktopWindow() );
		if (newActive == winpos.hwnd)
                    newActive = wndPtr->next ? wndPtr->next->hwndSelf : 0;
	    }	    
	    WINPOS_ChangeActiveWindow( newActive, FALSE );
	}
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
	    WINPOS_ChangeActiveWindow( winpos.hwnd, FALSE );
    
      /* Repaint the window */

    if (wndPtr->window) EVENT_Synchronize();  /* Wait for all expose events */

    EVENT_DummyMotionNotify(); /* Simulate a mouse event to set the cursor */

    if (!(flags & SWP_DEFERERASE))
        RedrawWindow32( wndPtr->parent->hwndSelf, NULL, 0,
                        RDW_ALLCHILDREN | RDW_ERASENOW );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    dprintf_win(stddeb,"\tstatus flags = %04x\n", winpos.flags & SWP_AGG_STATUSFLAGS);

    if ( ((winpos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) && 
	!(winpos.flags & SWP_NOSENDCHANGING))
        SendMessage16( winpos.hwnd, WM_WINDOWPOSCHANGED,
                       0, (LPARAM)MAKE_SEGPTR(&winpos) );

    return TRUE;
}

					
/***********************************************************************
 *           BeginDeferWindowPos   (USER.259)
 */
HDWP16 BeginDeferWindowPos( INT count )
{
    HDWP16 handle;
    DWP *pDWP;

    if (count <= 0) return 0;
    handle = USER_HEAP_ALLOC( sizeof(DWP) + (count-1)*sizeof(WINDOWPOS16) );
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
 *           DeferWindowPos   (USER.260)
 */
HDWP16 DeferWindowPos( HDWP16 hdwp, HWND hwnd, HWND hwndAfter, INT x, INT y,
                       INT cx, INT cy, UINT flags )
{
    DWP *pDWP;
    int i;
    HDWP16 newhdwp = hdwp;
    HWND parent;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return 0;
    if (hwnd == GetDesktopWindow()) return 0;

      /* All the windows of a DeferWindowPos() must have the same parent */

    parent = WIN_FindWndPtr( hwnd )->parent->hwndSelf;
    if (pDWP->actualCount == 0) pDWP->hwndParent = parent;
    else if (parent != pDWP->hwndParent)
    {
        USER_HEAP_FREE( hdwp );
        return 0;
    }

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
            pDWP->winPos[i].flags &= flags & (SWP_NOSIZE | SWP_NOMOVE |
                                              SWP_NOZORDER | SWP_NOREDRAW |
                                              SWP_NOACTIVATE | SWP_NOCOPYBITS |
                                              SWP_NOOWNERZORDER);
            pDWP->winPos[i].flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                              SWP_FRAMECHANGED);
            return hdwp;
        }
    }
    if (pDWP->actualCount >= pDWP->suggestedCount)
    {
        newhdwp = USER_HEAP_REALLOC( hdwp,
                      sizeof(DWP) + pDWP->suggestedCount*sizeof(WINDOWPOS16) );
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
 *           EndDeferWindowPos   (USER.261)
 */
BOOL EndDeferWindowPos( HDWP16 hdwp )
{
    DWP *pDWP;
    WINDOWPOS16 *winpos;
    BOOL res = TRUE;
    int i;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return FALSE;
    for (i = 0, winpos = pDWP->winPos; i < pDWP->actualCount; i++, winpos++)
    {
        if (!(res = SetWindowPos( winpos->hwnd, winpos->hwndInsertAfter,
                                  winpos->x, winpos->y, winpos->cx, winpos->cy,
                                  winpos->flags ))) break;
    }
    USER_HEAP_FREE( hdwp );
    return res;
}


/***********************************************************************
 *           TileChildWindows   (USER.199)
 */
void TileChildWindows( HWND parent, WORD action )
{
    printf("STUB TileChildWindows(%04x, %d)\n", parent, action);
}

/***********************************************************************
 *           CascageChildWindows   (USER.198)
 */
void CascadeChildWindows( HWND parent, WORD action )
{
    printf("STUB CascadeChildWindows(%04x, %d)\n", parent, action);
}
