/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995, 1996, 1999 Alex Korobka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


/***********************************************************************
 *		SwitchToThisWindow (USER32.@)
 */
void WINAPI SwitchToThisWindow( HWND hwnd, BOOL alt_tab )
{
    if (IsIconic( hwnd )) NtUserShowWindow( hwnd, SW_RESTORE );
    else BringWindowToTop( hwnd );
}


/***********************************************************************
 *		GetWindowRect (USER32.@)
 */
BOOL WINAPI GetWindowRect( HWND hwnd, LPRECT rect )
{
    BOOL ret = NtUserGetWindowRect( hwnd, rect );
    if (ret) TRACE( "hwnd %p %s\n", hwnd, wine_dbgstr_rect(rect) );
    return ret;
}


/***********************************************************************
 *		GetWindowRgn (USER32.@)
 */
int WINAPI GetWindowRgn( HWND hwnd, HRGN hrgn )
{
    return NtUserGetWindowRgnEx( hwnd, hrgn, 0 );
}

/***********************************************************************
 *		GetWindowRgnBox (USER32.@)
 */
int WINAPI GetWindowRgnBox( HWND hwnd, LPRECT prect )
{
    int ret = ERROR;
    HRGN hrgn;

    if (!prect)
        return ERROR;

    if ((hrgn = CreateRectRgn(0, 0, 0, 0)))
    {
        if ((ret = GetWindowRgn( hwnd, hrgn )) != ERROR )
            ret = GetRgnBox( hrgn, prect );
        DeleteObject(hrgn);
    }

    return ret;
}


/***********************************************************************
 *		GetClientRect (USER32.@)
 */
BOOL WINAPI GetClientRect( HWND hwnd, LPRECT rect )
{
    return NtUserGetClientRect( hwnd, rect );
}


/*******************************************************************
 *		WindowFromPoint (USER32.@)
 */
HWND WINAPI WindowFromPoint( POINT pt )
{
    return NtUserWindowFromPoint( pt.x, pt.y );
}


/*******************************************************************
 *		ChildWindowFromPoint (USER32.@)
 */
HWND WINAPI ChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, CWP_ALL );
}

/*******************************************************************
 *		RealChildWindowFromPoint (USER32.@)
 */
HWND WINAPI RealChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y,
                                         CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE );
}

/*******************************************************************
 *		ChildWindowFromPointEx (USER32.@)
 */
HWND WINAPI ChildWindowFromPointEx( HWND parent, POINT pt, UINT flags )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, flags );
}


/*******************************************************************
 *		MapWindowPoints (USER32.@)
 */
INT WINAPI MapWindowPoints( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count )
{
    return NtUserMapWindowPoints( hwnd_from, hwnd_to, points, count );
}


/*******************************************************************
 *		ClientToScreen (USER32.@)
 */
BOOL WINAPI ClientToScreen( HWND hwnd, POINT *pt )
{
    return NtUserClientToScreen( hwnd, pt );
}


/*******************************************************************
 *		ScreenToClient (USER32.@)
 */
BOOL WINAPI ScreenToClient( HWND hwnd, POINT *pt )
{
    return NtUserScreenToClient( hwnd, pt );
}


/***********************************************************************
 *		IsIconic (USER32.@)
 */
BOOL WINAPI IsIconic(HWND hWnd)
{
    return (GetWindowLongW( hWnd, GWL_STYLE ) & WS_MINIMIZE) != 0;
}


/***********************************************************************
 *		IsZoomed (USER32.@)
 */
BOOL WINAPI IsZoomed(HWND hWnd)
{
    return (GetWindowLongW( hWnd, GWL_STYLE ) & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *		AllowSetForegroundWindow (USER32.@)
 */
BOOL WINAPI AllowSetForegroundWindow( DWORD procid )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/*******************************************************************
 *		LockSetForegroundWindow (USER32.@)
 */
BOOL WINAPI LockSetForegroundWindow( UINT lockcode )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/***********************************************************************
 *		BringWindowToTop (USER32.@)
 */
BOOL WINAPI BringWindowToTop( HWND hwnd )
{
    return NtUserSetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
}


/***********************************************************************
 *		GetInternalWindowPos (USER32.@)
 */
UINT WINAPI GetInternalWindowPos( HWND hwnd, LPRECT rectWnd,
                                      LPPOINT ptIcon )
{
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof(wndpl);
    if (NtUserGetWindowPlacement( hwnd, &wndpl ))
    {
	if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
	if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
	return wndpl.showCmd;
    }
    return 0;
}


/***********************************************************************
 *		AnimateWindow (USER32.@)
 *		Shows/Hides a window with an animation
 *		NO ANIMATION YET
 */
BOOL WINAPI AnimateWindow(HWND hwnd, DWORD dwTime, DWORD dwFlags)
{
	FIXME("partial stub\n");

	/* If trying to show/hide and it's already   *
	 * shown/hidden or invalid window, fail with *
	 * invalid parameter                         */
	if(!IsWindow(hwnd) ||
	   (IsWindowVisible(hwnd) && !(dwFlags & AW_HIDE)) ||
	   (!IsWindowVisible(hwnd) && (dwFlags & AW_HIDE)))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	NtUserShowWindow( hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA) );

	return TRUE;
}


/***********************************************************************
 *		BeginDeferWindowPos (USER32.@)
 */
HDWP WINAPI BeginDeferWindowPos( INT count )
{
    return NtUserBeginDeferWindowPos( count );
}


/***********************************************************************
 *		DeferWindowPos (USER32.@)
 */
HDWP WINAPI DeferWindowPos( HDWP hdwp, HWND hwnd, HWND after, INT x, INT y,
                            INT cx, INT cy, UINT flags )
{
    return NtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy, flags, 0, 0 );
}


/***********************************************************************
 *		EndDeferWindowPos (USER32.@)
 */
BOOL WINAPI EndDeferWindowPos( HDWP hdwp )
{
    return NtUserEndDeferWindowPosEx( hdwp, FALSE );
}


/***********************************************************************
 *		ArrangeIconicWindows (USER32.@)
 */
UINT WINAPI ArrangeIconicWindows( HWND parent )
{
    return NtUserArrangeIconicWindows( parent );
}
