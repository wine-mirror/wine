/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 */

#include "sysmetrics.h"
#include "user.h"
#include "win.h"
#include "message.h"
#include "stackframe.h"
#include "winpos.h"
#include "nonclient.h"
#include "stddebug.h"
/* #define DEBUG_WIN */
#include "debug.h"

static HWND hwndActive = 0;  /* Currently active window */


/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 * The new position is stored into wndPtr->ptIconPos.
 */
static void WINPOS_FindIconPos( HWND hwnd )
{
    RECT rectParent;
    short x, y, xspacing, yspacing;
    WND * wndPtr = WIN_FindWndPtr( hwnd );

    if (!wndPtr) return;
    GetClientRect( wndPtr->hwndParent, &rectParent );
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
            HWND hwndChild = GetWindow( wndPtr->hwndParent, GW_CHILD );
            while (hwndChild)
            {
                WND *childPtr = WIN_FindWndPtr( hwndChild );
                if ((childPtr->dwStyle & WS_MINIMIZE) && (hwndChild != hwnd))
                {
                    if ((childPtr->rectWindow.left < x + xspacing) &&
                        (childPtr->rectWindow.right >= x) &&
                        (childPtr->rectWindow.top <= y) &&
                        (childPtr->rectWindow.bottom > y - yspacing))
                        break;  /* There's a window in there */
                }
                
                hwndChild = childPtr->hwndNext;
            }
            if (!hwndChild)
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
WORD ArrangeIconicWindows( HWND parent )
{
    RECT rectParent;
    HWND hwndChild;
    short x, y, xspacing, yspacing;

    GetClientRect( parent, &rectParent );
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
 *           GetWindowRect   (USER.32)
 */
void GetWindowRect( HWND hwnd, LPRECT rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->dwStyle & WS_CHILD)
	MapWindowPoints( wndPtr->hwndParent, 0, (POINT *)rect, 2 );
}


/***********************************************************************
 *           GetClientRect   (USER.33)
 */
void GetClientRect( HWND hwnd, LPRECT rect ) 
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
 *         ClientToScreen   (USER.28)
 */
void ClientToScreen( HWND hwnd, LPPOINT lppnt )
{
    MapWindowPoints( hwnd, 0, lppnt, 1 );
}


/*******************************************************************
 *         ScreenToClient   (USER.29)
 */
void ScreenToClient( HWND hwnd, LPPOINT lppnt )
{
    MapWindowPoints( 0, hwnd, lppnt, 1 );
}


/*******************************************************************
 *         WindowFromPoint   (USER.30)
 */
HWND WindowFromPoint( POINT pt )
{
    HWND hwndRet = 0;
    HWND hwnd = GetDesktopWindow();

    while(hwnd)
    {
	  /* If point is in window, and window is visible,   */
	  /* not disabled and not transparent, then explore  */
	  /* its children. Otherwise, go to the next window. */

	WND *wndPtr = WIN_FindWndPtr( hwnd );
	if ((pt.x >= wndPtr->rectWindow.left) &&
	    (pt.x < wndPtr->rectWindow.right) &&
	    (pt.y >= wndPtr->rectWindow.top) &&
	    (pt.y < wndPtr->rectWindow.bottom) &&
	    !(wndPtr->dwStyle & WS_DISABLED) &&
	    (wndPtr->dwStyle & WS_VISIBLE) &&
	    !(wndPtr->dwExStyle & WS_EX_TRANSPARENT))
	{
	    hwndRet = hwnd;
              /* If window is minimized, ignore its children */
            if (wndPtr->dwStyle & WS_MINIMIZE) break;
	    pt.x -= wndPtr->rectClient.left;
	    pt.y -= wndPtr->rectClient.top;
	    hwnd = wndPtr->hwndChild;
	}
	else hwnd = wndPtr->hwndNext;
    }
    return hwndRet;
}


/*******************************************************************
 *         ChildWindowFromPoint   (USER.191)
 */
HWND ChildWindowFromPoint( HWND hwndParent, POINT pt )
{
    RECT rect;
    HWND hwnd;
    
    GetWindowRect( hwndParent, &rect );
    if (!PtInRect( &rect, pt )) return 0;
    hwnd = GetTopWindow( hwndParent );
    while (hwnd)
    {
	GetWindowRect( hwnd, &rect );
	if (PtInRect( &rect, pt )) return hwnd;
	hwnd = GetWindow( hwnd, GW_HWNDNEXT );
    }
    return hwndParent;
}


/*******************************************************************
 *         MapWindowPoints   (USER.258)
 */
void MapWindowPoints( HWND hwndFrom, HWND hwndTo, LPPOINT lppt, WORD count )
{
    WND * wndPtr;
    POINT * curpt;
    POINT origin = { 0, 0 };
    WORD i;

      /* Translate source window origin to screen coords */
    while(hwndFrom)
    {
	wndPtr = WIN_FindWndPtr( hwndFrom );
	origin.x += wndPtr->rectClient.left;
	origin.y += wndPtr->rectClient.top;
	hwndFrom = (wndPtr->dwStyle & WS_CHILD) ? wndPtr->hwndParent : 0;
    }

      /* Translate origin to destination window coords */
    while(hwndTo)
    {
	wndPtr = WIN_FindWndPtr( hwndTo );
	origin.x -= wndPtr->rectClient.left;
	origin.y -= wndPtr->rectClient.top;
	hwndTo = (wndPtr->dwStyle & WS_CHILD) ? wndPtr->hwndParent : 0;
    }    

      /* Translate points */
    for (i = 0, curpt = lppt; i < count; i++, curpt++)
    {
	curpt->x += origin.x;
	curpt->y += origin.y;
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
    if (!wndPtr || (wndPtr->dwStyle & WS_CHILD)) return 0;
    SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
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
    dprintf_win(stddeb, "MoveWindow: %d %d,%d %dx%d %d\n", 
	    hwnd, x, y, cx, cy, repaint );
    return SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}


/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL ShowWindow( HWND hwnd, int cmd ) 
{    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    BOOL wasVisible;
    POINT maxSize;
    int swpflags = 0;
    short x = 0, y = 0, cx = 0, cy = 0;

    if (!wndPtr) return FALSE;

    dprintf_win(stddeb,"ShowWindow: hwnd=%04X, cmd=%d\n", hwnd, cmd);

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;

    switch(cmd)
    {
        case SW_HIDE:
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
	case SW_RESTORE:
	    swpflags |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if (wndPtr->dwStyle & WS_MINIMIZE)
            {
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

    SendMessage( hwnd, WM_SHOWWINDOW, (cmd != SW_HIDE), 0 );
    SetWindowPos( hwnd, HWND_TOP, x, y, cx, cy, swpflags );

      /* Send WM_SIZE and WM_MOVE messages if not already done */
    if (!(wndPtr->flags & WIN_GOT_SIZEMSG))
    {
	int wParam = SIZE_RESTORED;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
	wndPtr->flags |= WIN_GOT_SIZEMSG;
	SendMessage( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessage( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

    return wasVisible;
}


/***********************************************************************
 *           GetInternalWindowPos   (USER.460)
 */
WORD GetInternalWindowPos( HWND hwnd, LPRECT rectWnd, LPPOINT ptIcon )
{
    WINDOWPLACEMENT wndpl;
    if (!GetWindowPlacement( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/***********************************************************************
 *           SetInternalWindowPos   (USER.461)
 */
void SetInternalWindowPos( HWND hwnd, WORD showCmd, LPRECT rect, LPPOINT pt )
{
    WINDOWPLACEMENT wndpl;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    wndpl.length  = sizeof(wndpl);
    wndpl.flags   = (pt != NULL) ? WPF_SETMINPOSITION : 0;
    wndpl.showCmd = showCmd;
    if (pt) wndpl.ptMinPosition = *pt;
    wndpl.rcNormalPosition = (rect != NULL) ? *rect : wndPtr->rectNormal;
    wndpl.ptMaxPosition = wndPtr->ptMaxPos;
    SetWindowPlacement( hwnd, &wndpl );
}


/***********************************************************************
 *           GetWindowPlacement   (USER.370)
 */
BOOL GetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *wndpl )
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
 *           SetWindowPlacement   (USER.371)
 */
BOOL SetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *wndpl )
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


/*******************************************************************
 *         WINPOS_ChangeActiveWindow
 *
 * Change the active window and send the corresponding messages.
 */
HWND WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg )
{
    HWND prevActive = hwndActive;
    if (hwnd == hwndActive) return 0;
    if (hwndActive)
    {
	if (!SendMessage( hwndActive, WM_NCACTIVATE, FALSE, 0 )) return 0;
	SendMessage( hwndActive, WM_ACTIVATE, WA_INACTIVE,
		     MAKELONG( IsIconic(hwndActive), hwnd ) );
	/* Send WM_ACTIVATEAPP here */
    }

    hwndActive = hwnd;
    if (hwndActive)
    {
	WND *wndPtr = WIN_FindWndPtr( hwndActive );
	wndPtr->hwndPrevActive = prevActive;

	/* Send WM_ACTIVATEAPP here */
	SendMessage( hwnd, WM_NCACTIVATE, TRUE, 0 );
	SendMessage( hwnd, WM_ACTIVATE, mouseMsg ? WA_CLICKACTIVE : WA_ACTIVE,
		     MAKELONG( IsIconic(hwnd), prevActive ) );
    }
    return prevActive;
}


/***********************************************************************
 *           WINPOS_SendNCCalcSize
 *
 * Send a WM_NCCALCSIZE message to a window.
 * All parameters are read-only except newClientRect.
 * oldWindowRect, oldClientRect and winpos must be non-NULL only
 * when calcValidRect is TRUE.
 */
LONG WINPOS_SendNCCalcSize( HWND hwnd, BOOL calcValidRect, RECT *newWindowRect,
			    RECT *oldWindowRect, RECT *oldClientRect,
			    WINDOWPOS *winpos, RECT *newClientRect )
{
    NCCALCSIZE_PARAMS params;
    LONG result;

    params.rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
	params.rgrc[1] = *oldWindowRect;
	params.rgrc[2] = *oldClientRect;
	params.lppos = winpos;
    }
    result = SendMessage( hwnd, WM_NCCALCSIZE, calcValidRect,
                          MAKE_SEGPTR( &params ) );
    *newClientRect = params.rgrc[0];
    return result;
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging( WINDOWPOS *winpos )
{
    POINT maxSize;
    WND *wndPtr = WIN_FindWndPtr( winpos->hwnd );
    if (!wndPtr || (winpos->flags & SWP_NOSIZE)) return 0;
    if ((wndPtr->dwStyle & WS_THICKFRAME) ||
	(wndPtr->dwStyle & (WS_POPUP | WS_CHILD) == 0))
    {
	NC_GetMinMaxInfo( winpos->hwnd, &maxSize, NULL, NULL, NULL );
	winpos->cx = min( winpos->cx, maxSize.x );
	winpos->cy = min( winpos->cy, maxSize.y );
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
    HWND hwndCur;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

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
        if (!wndPtr->hwndNext) return;  /* Already at the bottom */
        movingUp = FALSE;
    }
    else
    {
        if (wndPtr->hwndNext == hwndAfter) return;  /* Already placed right */

          /* Determine which window we encounter first in Z-order */
        hwndCur = GetWindow( wndPtr->hwndParent, GW_CHILD );
        while ((hwndCur != hwnd) && (hwndCur != hwndAfter))
            hwndCur = GetWindow( hwndCur, GW_HWNDNEXT );
        movingUp = (hwndCur == hwndAfter);
    }

    if (movingUp)
    {
        HWND hwndPrevAfter = wndPtr->hwndNext;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        hwndCur = wndPtr->hwndNext;
        while (hwndCur != hwndPrevAfter)
        {
            WND *curPtr = WIN_FindWndPtr( hwndCur );
            RECT rect = curPtr->rectWindow;
            OffsetRect( &rect, -wndPtr->rectClient.left,
                        -wndPtr->rectClient.top );
            RedrawWindow( hwnd, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                          RDW_FRAME | RDW_ERASE );
            hwndCur = curPtr->hwndNext;
        }
    }
    else  /* Moving down */
    {
        hwndCur = wndPtr->hwndNext;
        WIN_UnlinkWindow( hwnd );
        WIN_LinkWindow( hwnd, hwndAfter );
        while (hwndCur != hwnd)
        {
            WND *curPtr = WIN_FindWndPtr( hwndCur );
            RECT rect = wndPtr->rectWindow;
            OffsetRect( &rect, -curPtr->rectClient.left,
                        -curPtr->rectClient.top );
            RedrawWindow( hwndCur, &rect, 0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                          RDW_FRAME | RDW_ERASE );
            hwndCur = curPtr->hwndNext;
        }
    }
}


/***********************************************************************
 *           WINPOS_SetXWindosPos
 *
 * SetWindowPos() for an X window. Used by the real SetWindowPos().
 */
static void WINPOS_SetXWindowPos( WINDOWPOS *winpos )
{
    XWindowChanges winChanges;
    int changeMask = 0;
    WND *wndPtr = WIN_FindWndPtr( winpos->hwnd );

    if (!(winpos->flags & SWP_NOSIZE))
    {
        winChanges.width     = winpos->cx;
        winChanges.height    = winpos->cy;
        changeMask |= CWWidth | CWHeight;
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
    if (changeMask)
        XConfigureWindow( display, wndPtr->window, changeMask, &winChanges );
}


/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
BOOL SetWindowPos( HWND hwnd, HWND hwndInsertAfter, INT x, INT y,
		   INT cx, INT cy, WORD flags )
{
    WINDOWPOS winpos;
    WND *wndPtr;
    RECT newWindowRect, newClientRect;
    int result;

      /* Check window handle */

    if (hwnd == GetDesktopWindow()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

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
    if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM) &&
	(wndPtr->hwndParent != WIN_FindWndPtr(hwndInsertAfter)->hwndParent))
        return FALSE;

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
	SendMessage( hwnd, WM_WINDOWPOSCHANGING, 0, MAKE_SEGPTR(&winpos) );

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
    }

      /* Reposition window in Z order */

    if (!(winpos.flags & SWP_NOZORDER))
    {
        if (wndPtr->window)
        {
            WIN_UnlinkWindow( winpos.hwnd );
            WIN_LinkWindow( winpos.hwnd, hwndInsertAfter );
        }
        else WINPOS_MoveWindowZOrder( winpos.hwnd, hwndInsertAfter );
    }

      /* Send WM_NCCALCSIZE message to get new client area */

    result = WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    &winpos, &newClientRect );
    /* ....  Should handle result here */

      /* Perform the moving and resizing */

    if (wndPtr->window)
    {
        WINPOS_SetXWindowPos( &winpos );
        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;
    }
    else
    {
        RECT oldWindowRect = wndPtr->rectWindow;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

        if (!(flags & SWP_NOREDRAW) &&
            (!(flags & SWP_NOSIZE) || !(flags & SWP_NOMOVE) ||
             !(flags & SWP_NOZORDER)))
        {
            HRGN hrgn1 = CreateRectRgnIndirect( &oldWindowRect );
            HRGN hrgn2 = CreateRectRgnIndirect( &wndPtr->rectWindow );
            HRGN hrgn3 = CreateRectRgn( 0, 0, 0, 0 );
            CombineRgn( hrgn3, hrgn1, hrgn2, RGN_DIFF );
            RedrawWindow( wndPtr->hwndParent, NULL, hrgn3,
                          RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE );
            if ((oldWindowRect.left != wndPtr->rectWindow.left) ||
                (oldWindowRect.top != wndPtr->rectWindow.top))
            {
                RedrawWindow( winpos.hwnd, NULL, 0, RDW_INVALIDATE |
                              RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE );
            }
            DeleteObject( hrgn1 );
            DeleteObject( hrgn2 );
            DeleteObject( hrgn3 );
        }
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
                RedrawWindow( winpos.hwnd, NULL, 0,
                              RDW_INVALIDATE | RDW_FRAME | RDW_ERASE );
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
                RedrawWindow( wndPtr->hwndParent, &wndPtr->rectWindow, 0,
                              RDW_INVALIDATE | RDW_FRAME |
                              RDW_ALLCHILDREN | RDW_ERASE );
        }
        if ((winpos.hwnd == GetFocus()) || IsChild(winpos.hwnd, GetFocus()))
            SetFocus( GetParent(winpos.hwnd) );  /* Revert focus to parent */
	if (winpos.hwnd == hwndActive)
	{
	      /* Activate previously active window if possible */
	    HWND newActive = wndPtr->hwndPrevActive;
	    if (!IsWindow(newActive) || (newActive == winpos.hwnd))
	    {
		newActive = GetTopWindow(GetDesktopWindow());
		if (newActive == winpos.hwnd) newActive = wndPtr->hwndNext;
	    }	    
	    WINPOS_ChangeActiveWindow( newActive, FALSE );
	}
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
    {
	if (!(wndPtr->dwStyle & WS_CHILD))
	    WINPOS_ChangeActiveWindow( winpos.hwnd, FALSE );
    }
    
      /* Repaint the window */

    if (wndPtr->window) MSG_Synchronize();  /* Wait for all expose events */
    if ((flags & SWP_FRAMECHANGED) && !(flags & SWP_NOREDRAW))
        RedrawWindow( winpos.hwnd, NULL, 0,
                      RDW_INVALIDATE | RDW_FRAME | RDW_ERASE );
    if (!(flags & SWP_DEFERERASE))
        RedrawWindow( wndPtr->hwndParent, NULL, 0,
                      RDW_ALLCHILDREN | RDW_ERASENOW );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    if (!(winpos.flags & SWP_NOSENDCHANGING))
        SendMessage( winpos.hwnd, WM_WINDOWPOSCHANGED,
                     0, MAKE_SEGPTR(&winpos) );

    return TRUE;
}

					
/***********************************************************************
 *           BeginDeferWindowPos   (USER.259)
 */
HDWP BeginDeferWindowPos( INT count )
{
    HDWP handle;
    DWP *pDWP;

    if (count <= 0) return 0;
    handle = USER_HEAP_ALLOC( sizeof(DWP) + (count-1)*sizeof(WINDOWPOS) );
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
HDWP DeferWindowPos( HDWP hdwp, HWND hwnd, HWND hwndAfter, INT x, INT y,
                     INT cx, INT cy, WORD flags )
{
    DWP *pDWP;
    int i;
    HDWP newhdwp = hdwp;
    HWND parent;

    pDWP = (DWP *) USER_HEAP_LIN_ADDR( hdwp );
    if (!pDWP) return 0;

      /* All the windows of a DeferWindowPos() must have the same parent */

    parent = WIN_FindWndPtr( hwnd )->hwndParent;
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
                      sizeof(DWP) + pDWP->suggestedCount*sizeof(WINDOWPOS) );
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
BOOL EndDeferWindowPos( HDWP hdwp )
{
    DWP *pDWP;
    WINDOWPOS *winpos;
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
    printf("STUB TileChildWindows(%04X, %d)\n", parent, action);
}

/***********************************************************************
 *           CascageChildWindows   (USER.198)
 */
void CascadeChildWindows( HWND parent, WORD action )
{
    printf("STUB CascadeChildWindows(%04X, %d)\n", parent, action);
}
