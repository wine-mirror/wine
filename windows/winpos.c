/*
 * Window position related functions.
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "win.h"

extern Display * XT_display;
extern Screen * XT_screen;


/***********************************************************************
 *           GetWindowRect   (USER.32)
 */
void GetWindowRect( HWND hwnd, LPRECT rect ) 
{
    WND * wndPtr = WIN_FindWndPtr( hwnd ); 
    if (!wndPtr) return;
    
    *rect = wndPtr->rectWindow;
    if (wndPtr->hwndParent)
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
	hwndFrom = wndPtr->hwndParent;
    }

      /* Translate origin to destination window coords */
    while(hwndTo)
    {
	wndPtr = WIN_FindWndPtr( hwndTo );
	origin.x -= wndPtr->rectClient.left;
	origin.y -= wndPtr->rectClient.top;
	hwndTo = wndPtr->hwndParent;
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
	case SW_MINIMIZE:
	    wndPtr->dwStyle |= WS_MINIMIZE;
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWNA:
	case SW_SHOWNOACTIVATE:
	case SW_MAXIMIZE:
	case SW_SHOWMAXIMIZED:
	case SW_SHOW:
	case SW_NORMAL:
	case SW_SHOWNORMAL:
	    wndPtr->dwStyle &= ~WS_MINIMIZE;
	    swpflags |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
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
 *           SetWindowPos   (USER.232)
 */
/* Unimplemented flags: SWP_NOREDRAW, SWP_NOACTIVATE
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
#ifdef USE_XLIB
    XWindowChanges winChanges;
    int changeMask = 0;
#endif

#ifdef DEBUG_WIN
    printf( "SetWindowPos: %d %d %d,%d %dx%d 0x%x\n",
	    hwnd, hwndInsertAfter, x, y, cx, cy, flags );
#endif

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
    if (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW))
	flags |= SWP_NOMOVE | SWP_NOSIZE;

      /* Send WM_WINDOWPOSCHANGING message */

    if (!(hmem = GlobalAlloc( GMEM_MOVEABLE,sizeof(WINDOWPOS) ))) return FALSE;
    winPos = (WINDOWPOS *)GlobalLock( hmem );
    winPos->hwnd = hwnd;
    winPos->hwndInsertAfter = hwndInsertAfter;
    winPos->x = x;
    winPos->y = y;
    winPos->cx = cx;
    winPos->cy = cy;
    winPos->flags = flags;
    SendMessage( hwnd, WM_WINDOWPOSCHANGING, 0, (LONG)winPos );

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
	hwndInsertAfter = winPos->hwndInsertAfter;

	  /* TOPMOST not supported yet */
	if ((hwndInsertAfter == HWND_TOPMOST) ||
	    (hwndInsertAfter == HWND_NOTOPMOST)) hwndInsertAfter = HWND_TOP;

	  /* Make sure hwndInsertAfter is a sibling of hwnd */
	if ((hwndInsertAfter != HWND_TOP) && (hwndInsertAfter != HWND_BOTTOM))
	    if (wndPtr->hwndParent != GetParent(hwndInsertAfter)) goto Abort;

	WIN_UnlinkWindow( hwnd );
	WIN_LinkWindow( hwnd, hwndInsertAfter );
    }

      /* Recalculate client area position */

    if (winPos->flags & SWP_FRAMECHANGED)
    {
	  /* Send WM_NCCALCSIZE message */
	NCCALCSIZE_PARAMS *params;
	HANDLE hparams;
	
	if (!(hparams = GlobalAlloc( GMEM_MOVEABLE, sizeof(*params) )))
	    goto Abort;
	params = (NCCALCSIZE_PARAMS *) GlobalLock( hparams );
	params->rgrc[0] = newWindowRect;
	params->rgrc[1] = wndPtr->rectWindow;
	params->rgrc[2] = wndPtr->rectClient;
	params->lppos = winPos;
	calcsize_result = SendMessage(hwnd, WM_NCCALCSIZE, TRUE, (LONG)params);
	GlobalUnlock( hparams );
	GlobalFree( hparams );
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
#ifdef USE_XLIB
    if (!(winPos->flags & SWP_NOMOVE))
    {
	WND * parentPtr;
	winChanges.x = newWindowRect.left;
	winChanges.y = newWindowRect.top;
	if (wndPtr->hwndParent)
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
    if (changeMask) XConfigureWindow( XT_display, wndPtr->window,
				      changeMask, &winChanges );
#endif

    if (winPos->flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
#ifdef USE_XLIB
	XMapWindow( XT_display, wndPtr->window );
#else		
	if (wndPtr->shellWidget) XtMapWidget( wndPtr->shellWidget );
	else XtMapWidget( wndPtr->winWidget );
#endif
    }
    else if (winPos->flags & SWP_HIDEWINDOW)
    {
	wndPtr->dwStyle &= ~WS_VISIBLE;
#ifdef USE_XLIB
	XUnmapWindow( XT_display, wndPtr->window );
#else		
	if (wndPtr->shellWidget) XtUnmapWidget( wndPtr->shellWidget );
	else XtUnmapWidget( wndPtr->winWidget );
#endif	
    }

      /* Finally send the WM_WINDOWPOSCHANGED message */
    wndPtr->rectWindow = newWindowRect;
    wndPtr->rectClient = newClientRect;
    SendMessage( hwnd, WM_WINDOWPOSCHANGED, 0, (LONG)winPos );
    GlobalUnlock( hmem );
    GlobalFree( hmem );

    return TRUE;

 Abort:  /* Fatal error encountered */
    if (hmem)
    {
	GlobalUnlock( hmem );
	GlobalFree( hmem );
    }
    return FALSE;
}


