/*
 * Window position related functions.
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "sysmetrics.h"
#include "user.h"
#include "win.h"
#include "message.h"

static HWND hwndActive = 0;  /* Currently active window */


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
	    pt.x -= wndPtr->rectClient.left;
	    pt.y -= wndPtr->rectClient.top;
	    hwndRet = hwnd;
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
#ifdef DEBUG_WIN    
    printf( "MoveWindow: %d %d,%d %dx%d %d\n", hwnd, x, y, cx, cy, repaint );
#endif
    return SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}


/***********************************************************************
 *           ShowWindow   (USER.42)
 */
BOOL ShowWindow( HWND hwnd, int cmd ) 
{    
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    BOOL wasVisible;
    int swpflags = 0;

#ifdef DEBUG_WIN
    printf("ShowWindow: hwnd=%d, cmd=%d\n", hwnd, cmd);
#endif
    
    if (!wndPtr) return FALSE;
    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;
    switch(cmd)
    {
        case SW_HIDE:
	    if (!wasVisible) return FALSE;  /* Nothing to do */
	    swpflags |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
	case SW_SHOWMINIMIZED:
	case SW_SHOWMAXIMIZED:
	case SW_MINIMIZE:
	    wndPtr->dwStyle |= WS_MINIMIZE;
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWNA:
	case SW_MAXIMIZE:
	case SW_SHOW:
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_NORMAL:
	case SW_SHOWNORMAL:
	case SW_SHOWNOACTIVATE:
	case SW_RESTORE:
	    wndPtr->dwStyle &= ~WS_MINIMIZE;
	    wndPtr->dwStyle &= ~WS_MAXIMIZE;
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    if (cmd == SW_SHOWNOACTIVATE)
	    {
		swpflags |= SWP_NOZORDER;
		if (GetActiveWindow()) swpflags |= SWP_NOACTIVATE;
	    }
	    break;
    }
    SendMessage( hwnd, WM_SHOWWINDOW, (cmd != SW_HIDE), 0 );
    SetWindowPos( hwnd, 0, 0, 0, 0, 0, swpflags );

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
 *         WINPOS_GetMinMaxInfo
 *
 * Send a WM_GETMINMAXINFO to the window.
 */
void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos,
			   POINT *minTrack, POINT *maxTrack )
{
    HANDLE minmaxHandle;
    MINMAXINFO MinMax, *pMinMax;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    MinMax.ptMaxSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxSize.y = SYSMETRICS_CYSCREEN;
    MinMax.ptMaxPosition = wndPtr->ptMaxPos;
    MinMax.ptMinTrackSize.x = SYSMETRICS_CXMINTRACK;
    MinMax.ptMinTrackSize.y = SYSMETRICS_CYMINTRACK;
    MinMax.ptMaxTrackSize.x = SYSMETRICS_CXSCREEN;
    MinMax.ptMaxTrackSize.y = SYSMETRICS_CYSCREEN;

    minmaxHandle = USER_HEAP_ALLOC( LMEM_MOVEABLE, sizeof(MINMAXINFO) );
    if (minmaxHandle)
    {
	pMinMax = (MINMAXINFO *) USER_HEAP_ADDR( minmaxHandle );
	memcpy( pMinMax, &MinMax, sizeof(MinMax) );	
	SendMessage( hwnd, WM_GETMINMAXINFO, 0, (LONG)pMinMax );
    }
    else pMinMax = &MinMax;

      /* Some sanity checks */

    pMinMax->ptMaxTrackSize.x = max( pMinMax->ptMaxTrackSize.x,
				     pMinMax->ptMinTrackSize.x );
    pMinMax->ptMaxTrackSize.y = max( pMinMax->ptMaxTrackSize.y,
				     pMinMax->ptMinTrackSize.y );
    
    if (maxSize) *maxSize = pMinMax->ptMaxSize;
    if (maxPos) *maxPos = pMinMax->ptMaxPosition;
    if (minTrack) *minTrack = pMinMax->ptMinTrackSize;
    if (maxTrack) *maxTrack = pMinMax->ptMaxTrackSize;
    if (minmaxHandle) USER_HEAP_FREE( minmaxHandle );
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
	/* Send WM_ACTIVATEAPP here */
	SendMessage( hwnd, WM_NCACTIVATE, TRUE, 0 );
	SendMessage( hwnd, WM_ACTIVATE, mouseMsg ? WA_CLICKACTIVE : WA_ACTIVE,
		     MAKELONG( IsIconic(hwnd), prevActive ) );
    }
    return prevActive;
}


/***********************************************************************
 *           SetWindowPos   (USER.232)
 */
/* Unimplemented flags: SWP_NOREDRAW
 */
/* Note: all this code should be in the DeferWindowPos() routines,
 * and SetWindowPos() should simply call them.  This will be implemented
 * some day...
 */
BOOL SetWindowPos( HWND hwnd, HWND hwndInsertAfter, short x, short y,
		   short cx, short cy, WORD flags )
{
    WINDOWPOS *winPos;
    HANDLE hmem = 0;
    RECT newWindowRect, newClientRect;
    WND *wndPtr;
    int calcsize_result = 0;
    XWindowChanges winChanges;
    int changeMask = 0;

#ifdef DEBUG_WIN
    printf( "SetWindowPos: %d %d %d,%d %dx%d 0x%x\n",
	    hwnd, hwndInsertAfter, x, y, cx, cy, flags );
#endif

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	flags |= SWP_NOMOVE | SWP_NOSIZE;

      /* Send WM_WINDOWPOSCHANGING message */

    if (!(hmem = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(WINDOWPOS) )))
	return FALSE;
    winPos = (WINDOWPOS *)USER_HEAP_ADDR( hmem );
    winPos->hwnd = hwnd;
    winPos->hwndInsertAfter = hwndInsertAfter;
    winPos->x = x;
    winPos->y = y;
    winPos->cx = cx;
    winPos->cy = cy;
    winPos->flags = flags;
    SendMessage( hwnd, WM_WINDOWPOSCHANGING, 0, (LONG)winPos );
    hwndInsertAfter = winPos->hwndInsertAfter;

      /* Some sanity checks */

    if (!IsWindow( hwnd ) || (hwnd == GetDesktopWindow())) goto Abort;
    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	flags |= SWP_NOMOVE | SWP_NOSIZE;
    if (!(flags & (SWP_NOZORDER | SWP_NOACTIVATE)))
    {
	if (hwnd != hwndActive) hwndInsertAfter = HWND_TOP;
	else if ((hwndInsertAfter == HWND_TOPMOST) ||
		 (hwndInsertAfter == HWND_NOTOPMOST))
	    hwndInsertAfter = HWND_TOP;	
    }

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = wndPtr->rectClient;

    if (!(winPos->flags & SWP_NOSIZE))
    {
	newWindowRect.right  = newWindowRect.left + winPos->cx;
	newWindowRect.bottom = newWindowRect.top + winPos->cy;
    }

    if (!(winPos->flags & SWP_NOMOVE))
    {
	newWindowRect.left    = winPos->x;
	newWindowRect.top     = winPos->y;
	newWindowRect.right  += winPos->x - wndPtr->rectWindow.left;
	newWindowRect.bottom += winPos->y - wndPtr->rectWindow.top;
    }

      /* Reposition window in Z order */

    if (!(winPos->flags & SWP_NOZORDER))
    {
	  /* TOPMOST not supported yet */
	if ((hwndInsertAfter == HWND_TOPMOST) ||
	    (hwndInsertAfter == HWND_NOTOPMOST)) hwndInsertAfter = HWND_TOP;

	  /* Make sure hwndInsertAfter is a sibling of hwnd */
	if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM))
	    if (GetParent(hwnd) != GetParent(hwndInsertAfter)) goto Abort;

	WIN_UnlinkWindow( hwnd );
	WIN_LinkWindow( hwnd, hwndInsertAfter );
    }

      /* Recalculate client area position */

    if (winPos->flags & SWP_FRAMECHANGED)
    {
	  /* Send WM_NCCALCSIZE message */
	NCCALCSIZE_PARAMS *params;
	HANDLE hparams;
	
	if (!(hparams = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(*params) )))
	    goto Abort;
	params = (NCCALCSIZE_PARAMS *) USER_HEAP_ADDR( hparams );
	params->rgrc[0] = newWindowRect;
	params->rgrc[1] = wndPtr->rectWindow;
	params->rgrc[2] = wndPtr->rectClient;
	params->lppos = winPos;
	calcsize_result = SendMessage(hwnd, WM_NCCALCSIZE, TRUE, (LONG)params);
	USER_HEAP_FREE( hparams );
	newClientRect = params->rgrc[0];
	/* Handle result here */
    }
    else
    {
	newClientRect.left   = newWindowRect.left + wndPtr->rectClient.left
	                       - wndPtr->rectWindow.left;
	newClientRect.top    = newWindowRect.top + wndPtr->rectClient.top
	                       - wndPtr->rectWindow.top;
	newClientRect.right  = newWindowRect.right + wndPtr->rectClient.right
	                       - wndPtr->rectWindow.right;
	newClientRect.bottom = newWindowRect.bottom + wndPtr->rectClient.bottom
	                       - wndPtr->rectWindow.bottom;
    }
    
      /* Perform the moving and resizing */

    if (!(winPos->flags & SWP_NOMOVE))
    {
	WND * parentPtr;
	winChanges.x = newWindowRect.left;
	winChanges.y = newWindowRect.top;
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
	    winChanges.x += parentPtr->rectClient.left-parentPtr->rectWindow.left;
	    winChanges.y += parentPtr->rectClient.top-parentPtr->rectWindow.top;
	}
	changeMask |= CWX | CWY;
    }
    if (!(winPos->flags & SWP_NOSIZE))
    {
	winChanges.width  = newWindowRect.right - newWindowRect.left;
	winChanges.height = newWindowRect.bottom - newWindowRect.top;
	changeMask |= CWWidth | CWHeight;
    }
    if (!(winPos->flags & SWP_NOZORDER))
    {
	if (hwndInsertAfter == HWND_TOP) winChanges.stack_mode = Above;
	else winChanges.stack_mode = Below;
	if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM))
	{
	    WND * insertPtr = WIN_FindWndPtr( hwndInsertAfter );
	    winChanges.sibling = insertPtr->window;
	    changeMask |= CWSibling;
	}
	changeMask |= CWStackMode;
    }
    if (changeMask) XConfigureWindow( display, wndPtr->window,
				      changeMask, &winChanges );

    if (winPos->flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
	XMapWindow( display, wndPtr->window );
	MSG_Synchronize();
	if (!(winPos->flags & SWP_NOREDRAW))
	    RedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE |
			  RDW_ERASENOW | RDW_FRAME );
	else RedrawWindow( hwnd, NULL, 0, RDW_VALIDATE );
	
    }
    else if (winPos->flags & SWP_HIDEWINDOW)
    {
	wndPtr->dwStyle &= ~WS_VISIBLE;
	XUnmapWindow( display, wndPtr->window );
    }

    if (!(winPos->flags & SWP_NOACTIVATE))
    {
	if (!(wndPtr->dwStyle & WS_CHILD))
	    WINPOS_ChangeActiveWindow( hwnd, FALSE );
    }
    
      /* Send WM_NCPAINT message if needed */
    if ((winPos->flags & (SWP_FRAMECHANGED | SWP_SHOWWINDOW)) ||
	(!(winPos->flags & SWP_NOSIZE)) ||
	(!(winPos->flags & SWP_NOMOVE)) ||
	(!(winPos->flags & SWP_NOACTIVATE)) ||
	(!(winPos->flags & SWP_NOZORDER)))
	    SendMessage( hwnd, WM_NCPAINT, 1, 0L );

      /* Finally send the WM_WINDOWPOSCHANGED message */
    wndPtr->rectWindow = newWindowRect;
    wndPtr->rectClient = newClientRect;
    SendMessage( hwnd, WM_WINDOWPOSCHANGED, 0, (LONG)winPos );
    USER_HEAP_FREE( hmem );

    return TRUE;

 Abort:  /* Fatal error encountered */
    if (hmem) USER_HEAP_FREE( hmem );
    return FALSE;
}
