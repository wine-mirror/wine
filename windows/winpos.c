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
    printf("ShowWindow: hwnd=%04X, cmd=%d\n", hwnd, cmd);
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
    NCCALCSIZE_PARAMS *params;
    HANDLE hparams;
    LONG result;

    if (!(hparams = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(*params) )))
	return 0;
    params = (NCCALCSIZE_PARAMS *) USER_HEAP_ADDR( hparams );
    params->rgrc[0] = *newWindowRect;
    if (calcValidRect)
    {
	params->rgrc[1] = *oldWindowRect;
	params->rgrc[2] = *oldClientRect;
	params->lppos = winpos;
    }
    result = SendMessage( hwnd, WM_NCCALCSIZE, calcValidRect, (LONG)params);
    *newClientRect = params->rgrc[0];
    USER_HEAP_FREE( hparams );
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
	WINPOS_GetMinMaxInfo( winpos->hwnd, &maxSize, NULL, NULL, NULL );
	winpos->cx = min( winpos->cx, maxSize.x );
	winpos->cy = min( winpos->cy, maxSize.y );
    }
    return 0;
}


/***********************************************************************
 *           WINPOS_InternalSetWindowPos
 *
 * Helper function for SetWindowPos.
 */
static BOOL WINPOS_InternalSetWindowPos( WINDOWPOS *winpos )
{
    HWND hwndAfter;
    WND *wndPtr;
    RECT newWindowRect, newClientRect;
    int flags, result;
    int changeMask = 0;
    XWindowChanges winChanges;

      /* Send WM_WINDOWPOSCHANGING message */

    if (!(winpos->flags & SWP_NOSENDCHANGING))
	SendMessage( winpos->hwnd, WM_WINDOWPOSCHANGING, 0, (LONG)winpos );

      /* Check window handle */

    if (winpos->hwnd == GetDesktopWindow()) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( winpos->hwnd ))) return FALSE;

      /* Check dimensions */

    if (winpos->cx <= 0) winpos->cx = 1;
    if (winpos->cy <= 0) winpos->cy = 1;

      /* Check flags */

    flags = winpos->flags;
    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	flags |= SWP_NOMOVE | SWP_NOSIZE;
    if (winpos->hwnd == hwndActive) flags |= SWP_NOACTIVATE; /*Already active*/

      /* Check hwndAfter */

    hwndAfter = winpos->hwndInsertAfter;
    if (!(flags & (SWP_NOZORDER | SWP_NOACTIVATE)))
    {
	  /* Ignore TOPMOST flags when activating a window */
          /* _and_ moving it in Z order. */
	if ((hwndAfter == HWND_TOPMOST) || (hwndAfter == HWND_NOTOPMOST))
	    hwndAfter = HWND_TOP;	
    }
      /* TOPMOST not supported yet */
    if ((hwndAfter == HWND_TOPMOST) || (hwndAfter == HWND_NOTOPMOST))
	hwndAfter = HWND_TOP;
      /* hwndAfter must be a sibling of the window */
    if ((hwndAfter != HWND_TOP) && (hwndAfter != HWND_BOTTOM) &&
	(GetParent(winpos->hwnd) != GetParent(hwndAfter))) return FALSE;

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = wndPtr->rectClient;

    if (!(flags & SWP_NOSIZE))
    {
	newWindowRect.right  = newWindowRect.left + winpos->cx;
	newWindowRect.bottom = newWindowRect.top + winpos->cy;
	winChanges.width     = winpos->cx;
	winChanges.height    = winpos->cy;
	changeMask |= CWWidth | CWHeight;
    }
    if (!(flags & SWP_NOMOVE))
    {
	newWindowRect.left    = winpos->x;
	newWindowRect.top     = winpos->y;
	newWindowRect.right  += winpos->x - wndPtr->rectWindow.left;
	newWindowRect.bottom += winpos->y - wndPtr->rectWindow.top;
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    WND *parentPtr = WIN_FindWndPtr(wndPtr->hwndParent);
	    winChanges.x = winpos->x + parentPtr->rectClient.left 
		           - parentPtr->rectWindow.left;
	    winChanges.y = winpos->y + parentPtr->rectClient.top
		           - parentPtr->rectWindow.top;
	}
	else
	{
	    winChanges.x = winpos->x;
	    winChanges.y = winpos->y;
	}
	changeMask |= CWX | CWY;
    }

      /* Reposition window in Z order */

    if (!(flags & SWP_NOZORDER))
    {
	WIN_UnlinkWindow( winpos->hwnd );
	WIN_LinkWindow( winpos->hwnd, hwndAfter );
	if (hwndAfter == HWND_TOP) winChanges.stack_mode = Above;
	else winChanges.stack_mode = Below;
	if ((hwndAfter != HWND_TOP) && (hwndAfter != HWND_BOTTOM))
	{
	    WND * insertPtr = WIN_FindWndPtr( hwndAfter );
	    winChanges.sibling = insertPtr->window;
	    changeMask |= CWSibling;
	}
	changeMask |= CWStackMode;
    }

      /* Send WM_NCCALCSIZE message to get new client area */

    result = WINPOS_SendNCCalcSize( winpos->hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    winpos, &newClientRect );
    /* ....  Should handle result here */

      /* Perform the moving and resizing */

    if (changeMask) XConfigureWindow( display, wndPtr->window,
				      changeMask, &winChanges );
    wndPtr->rectWindow = newWindowRect;
    wndPtr->rectClient = newClientRect;

    if (flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
	XMapWindow( display, wndPtr->window );
	MSG_Synchronize();
	if (flags & SWP_NOREDRAW)  /* Validate the whole window */
	    RedrawWindow( winpos->hwnd, NULL, 0, RDW_VALIDATE );
    }
    else if (flags & SWP_HIDEWINDOW)
    {
	wndPtr->dwStyle &= ~WS_VISIBLE;
	XUnmapWindow( display, wndPtr->window );
	if ((winpos->hwnd == GetFocus()) || IsChild(winpos->hwnd, GetFocus()))
	    SetFocus( GetParent(winpos->hwnd) );  /* Revert focus to parent */
	if (winpos->hwnd == hwndActive)
	{
	      /* Activate previously active window if possible */
	    HWND newActive = wndPtr->hwndPrevActive;
	    if (!IsWindow(newActive) || (newActive == winpos->hwnd))
	    {
		newActive = GetTopWindow(GetDesktopWindow());
		if (newActive == winpos->hwnd) newActive = wndPtr->hwndNext;
	    }	    
	    WINPOS_ChangeActiveWindow( newActive, FALSE );
	}
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
    {
	if (!(wndPtr->dwStyle & WS_CHILD))
	    WINPOS_ChangeActiveWindow( winpos->hwnd, FALSE );
    }
    
      /* Send WM_NCPAINT message if needed */

    if ((flags & (SWP_FRAMECHANGED | SWP_SHOWWINDOW)) ||
	(!(flags & SWP_NOSIZE)) || (!(flags & SWP_NOMOVE)) ||
	(!(flags & SWP_NOACTIVATE)) || (!(flags & SWP_NOZORDER)))
	    SendMessage( winpos->hwnd, WM_NCPAINT, 1, 0L );
#if 0
    if ((flags & (SWP_FRAMECHANGED | SWP_SHOWWINDOW)) &&
	(!(flags & SWP_NOREDRAW)) && 
	(wndPtr->dwStyle & WS_VISIBLE))
	InvalidateRect(winpos->hwnd, NULL, TRUE);
#endif

      /* And last, send the WM_WINDOWPOSCHANGED message */

    SendMessage( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LONG)winpos );
    return TRUE;
}

					
/***********************************************************************
 *           SetWindowPos   (USER.232)
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
    BOOL res;

#ifdef DEBUG_WIN
    printf( "SetWindowPos: %04X %d %d,%d %dx%d 0x%x\n",
	    hwnd, hwndInsertAfter, x, y, cx, cy, flags );
#endif

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

    res = WINPOS_InternalSetWindowPos( winPos );

    USER_HEAP_FREE( hmem );
    return res;
}
