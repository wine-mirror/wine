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

#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "winerror.h"
#include "controls.h"
#include "win.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define HAS_THICKFRAME(style) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

#define PLACE_MIN		0x0001
#define PLACE_MAX		0x0002
#define PLACE_RECT		0x0004


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


/***********************************************************************
 *           list_children_from_point
 *
 * Get the list of children that can contain point from the server.
 * Point is in screen coordinates.
 * Returned list must be freed by caller.
 */
static HWND *list_children_from_point( HWND hwnd, POINT pt )
{
    HWND *list;
    int i, size = 128;

    for (;;)
    {
        int count = 0;

        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) break;

        SERVER_START_REQ( get_window_children_from_point )
        {
            req->parent = wine_server_user_handle( hwnd );
            req->x = pt.x;
            req->y = pt.y;
            req->dpi = get_thread_dpi();
            wine_server_set_reply( req, list, (size-1) * sizeof(user_handle_t) );
            if (!wine_server_call( req )) count = reply->count;
        }
        SERVER_END_REQ;
        if (count && count < size)
        {
            /* start from the end since HWND is potentially larger than user_handle_t */
            for (i = count - 1; i >= 0; i--)
                list[i] = wine_server_ptr_handle( ((user_handle_t *)list)[i] );
            list[count] = 0;
            return list;
        }
        HeapFree( GetProcessHeap(), 0, list );
        if (!count) break;
        size = count + 1;  /* restart with a large enough buffer */
    }
    return NULL;
}


/***********************************************************************
 *           WINPOS_WindowFromPoint
 *
 * Find the window and hittest for a given point.
 */
HWND WINPOS_WindowFromPoint( HWND hwndScope, POINT pt, INT *hittest )
{
    int i, res;
    HWND ret, *list;
    POINT win_pt;

    if (!hwndScope) hwndScope = GetDesktopWindow();

    *hittest = HTNOWHERE;

    if (!(list = list_children_from_point( hwndScope, pt ))) return 0;

    /* now determine the hittest */

    for (i = 0; list[i]; i++)
    {
        LONG style = GetWindowLongW( list[i], GWL_STYLE );

        /* If window is minimized or disabled, return at once */
        if (style & WS_DISABLED)
        {
            *hittest = HTERROR;
            break;
        }
        /* Send WM_NCCHITTEST (if same thread) */
        if (!WIN_IsCurrentThread( list[i] ))
        {
            *hittest = HTCLIENT;
            break;
        }
        win_pt = point_thread_to_win_dpi( list[i], pt );
        res = SendMessageW( list[i], WM_NCHITTEST, 0, MAKELPARAM( win_pt.x, win_pt.y ));
        if (res != HTTRANSPARENT)
        {
            *hittest = res;  /* Found the window */
            break;
        }
        /* continue search with next window in z-order */
    }
    ret = list[i];
    HeapFree( GetProcessHeap(), 0, list );
    TRACE( "scope %p (%d,%d) returning %p\n", hwndScope, pt.x, pt.y, ret );
    return ret;
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


/*******************************************************************
 *           get_work_rect
 *
 * Get the work area that a maximized window can cover, depending on style.
 */
static BOOL get_work_rect( HWND hwnd, RECT *rect )
{
    HMONITOR monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );
    MONITORINFO mon_info;
    DWORD style;

    if (!monitor) return FALSE;

    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfoW( monitor, &mon_info );
    *rect = mon_info.rcMonitor;

    style = GetWindowLongW( hwnd, GWL_STYLE );
    if (style & WS_MAXIMIZEBOX)
    {
        if ((style & WS_CAPTION) == WS_CAPTION || !(style & (WS_CHILD | WS_POPUP)))
            *rect = mon_info.rcWork;
    }
    return TRUE;
}


/***********************************************************************
 *		GetInternalWindowPos (USER32.@)
 */
UINT WINAPI GetInternalWindowPos( HWND hwnd, LPRECT rectWnd,
                                      LPPOINT ptIcon )
{
    WINDOWPLACEMENT wndpl;

    wndpl.length = sizeof(wndpl);
    if (GetWindowPlacement( hwnd, &wndpl ))
    {
	if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
	if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
	return wndpl.showCmd;
    }
    return 0;
}


static RECT get_maximized_work_rect( HWND hwnd )
{
    RECT work_rect = { 0 };

    if ((GetWindowLongW( hwnd, GWL_STYLE ) & (WS_MINIMIZE | WS_MAXIMIZE)) == WS_MAXIMIZE)
    {
        if (!get_work_rect( hwnd, &work_rect ))
            work_rect = get_primary_monitor_rect();
    }
    return work_rect;
}


/*******************************************************************
 *           update_maximized_pos
 *
 * For top level windows covering the work area, we might have to
 * "forget" the maximized position. Windows presumably does this
 * to avoid situations where the border style changes, which would
 * lead the window to be outside the screen, or the window gets
 * reloaded on a different screen, and the "saved" position no
 * longer applies to it (despite being maximized).
 *
 * Some applications (e.g. Imperiums: Greek Wars) depend on this.
 */
static void update_maximized_pos( WND *wnd, RECT *work_rect )
{
    if (wnd->parent && wnd->parent != GetDesktopWindow())
        return;

    if (wnd->dwStyle & WS_MAXIMIZE)
    {
        if (wnd->window_rect.left  <= work_rect->left  && wnd->window_rect.top    <= work_rect->top &&
            wnd->window_rect.right >= work_rect->right && wnd->window_rect.bottom >= work_rect->bottom)
            wnd->max_pos.x = wnd->max_pos.y = -1;
    }
    else
        wnd->max_pos.x = wnd->max_pos.y = -1;
}


/***********************************************************************
 *		GetWindowPlacement (USER32.@)
 *
 * Win95:
 * Fails if wndpl->length of Win95 (!) apps is invalid.
 */
BOOL WINAPI GetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *wndpl )
{
    return NtUserGetWindowPlacement( hwnd, wndpl );
}

/* make sure the specified rect is visible on screen */
static void make_rect_onscreen( RECT *rect )
{
    MONITORINFO info;
    HMONITOR monitor = MonitorFromRect( rect, MONITOR_DEFAULTTONEAREST );

    info.cbSize = sizeof(info);
    if (!monitor || !GetMonitorInfoW( monitor, &info )) return;
    /* FIXME: map coordinates from rcWork to rcMonitor */
    if (rect->right <= info.rcWork.left)
    {
        rect->right += info.rcWork.left - rect->left;
        rect->left = info.rcWork.left;
    }
    else if (rect->left >= info.rcWork.right)
    {
        rect->left += info.rcWork.right - rect->right;
        rect->right = info.rcWork.right;
    }
    if (rect->bottom <= info.rcWork.top)
    {
        rect->bottom += info.rcWork.top - rect->top;
        rect->top = info.rcWork.top;
    }
    else if (rect->top >= info.rcWork.bottom)
    {
        rect->top += info.rcWork.bottom - rect->bottom;
        rect->bottom = info.rcWork.bottom;
    }
}

/* make sure the specified point is visible on screen */
static void make_point_onscreen( POINT *pt )
{
    RECT rect;

    SetRect( &rect, pt->x, pt->y, pt->x + 1, pt->y + 1 );
    make_rect_onscreen( &rect );
    pt->x = rect.left;
    pt->y = rect.top;
}


/***********************************************************************
 *           WINPOS_SetPlacement
 */
static BOOL WINPOS_SetPlacement( HWND hwnd, const WINDOWPLACEMENT *wndpl, UINT flags )
{
    DWORD style;
    RECT work_rect = get_maximized_work_rect( hwnd );
    WND *pWnd = WIN_GetPtr( hwnd );
    WINDOWPLACEMENT wp = *wndpl;

    if (flags & PLACE_MIN) make_point_onscreen( &wp.ptMinPosition );
    if (flags & PLACE_MAX) make_point_onscreen( &wp.ptMaxPosition );
    if (flags & PLACE_RECT) make_rect_onscreen( &wp.rcNormalPosition );

    TRACE( "%p: setting min %d,%d max %d,%d normal %s flags %x adjusted to min %d,%d max %d,%d normal %s\n",
           hwnd, wndpl->ptMinPosition.x, wndpl->ptMinPosition.y,
           wndpl->ptMaxPosition.x, wndpl->ptMaxPosition.y,
           wine_dbgstr_rect(&wndpl->rcNormalPosition), flags,
           wp.ptMinPosition.x, wp.ptMinPosition.y, wp.ptMaxPosition.x, wp.ptMaxPosition.y,
           wine_dbgstr_rect(&wp.rcNormalPosition) );

    if (!pWnd || pWnd == WND_OTHER_PROCESS || pWnd == WND_DESKTOP) return FALSE;

    if (flags & PLACE_MIN) pWnd->min_pos = point_thread_to_win_dpi( hwnd, wp.ptMinPosition );
    if (flags & PLACE_MAX)
    {
        pWnd->max_pos = point_thread_to_win_dpi( hwnd, wp.ptMaxPosition );
        update_maximized_pos( pWnd, &work_rect );
    }
    if (flags & PLACE_RECT) pWnd->normal_rect = rect_thread_to_win_dpi( hwnd, wp.rcNormalPosition );

    style = pWnd->dwStyle;

    WIN_ReleasePtr( pWnd );

    if( style & WS_MINIMIZE )
    {
        if (flags & PLACE_MIN)
        {
            NtUserSetWindowPos( hwnd, 0, wp.ptMinPosition.x, wp.ptMinPosition.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    else if( style & WS_MAXIMIZE )
    {
        if (flags & PLACE_MAX)
            NtUserSetWindowPos( hwnd, 0, wp.ptMaxPosition.x, wp.ptMaxPosition.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
    }
    else if( flags & PLACE_RECT )
        NtUserSetWindowPos( hwnd, 0, wp.rcNormalPosition.left, wp.rcNormalPosition.top,
                            wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                            wp.rcNormalPosition.bottom - wp.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE );

    NtUserShowWindow( hwnd, wndpl->showCmd );

    if (IsIconic( hwnd ))
    {
        /* SDK: ...valid only the next time... */
        if( wndpl->flags & WPF_RESTORETOMAXIMIZED )
            win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );
    }
    return TRUE;
}


/***********************************************************************
 *		SetWindowPlacement (USER32.@)
 *
 * Win95:
 * Fails if wndpl->length of Win95 (!) apps is invalid.
 */
BOOL WINAPI SetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl )
{
    UINT flags = PLACE_MAX | PLACE_RECT;
    if (!wpl) return FALSE;
    if (wpl->flags & WPF_SETMINPOSITION) flags |= PLACE_MIN;
    return WINPOS_SetPlacement( hwnd, wpl, flags );
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
 *		SetInternalWindowPos (USER32.@)
 */
void WINAPI SetInternalWindowPos( HWND hwnd, UINT showCmd,
                                    LPRECT rect, LPPOINT pt )
{
    WINDOWPLACEMENT wndpl;
    UINT flags;

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


/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static BOOL can_activate_window( HWND hwnd )
{
    LONG style;

    if (!hwnd) return FALSE;
    style = GetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}


/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
void WINPOS_ActivateOtherWindow(HWND hwnd)
{
    HWND hwndTo, fg;

    if ((GetWindowLongW( hwnd, GWL_STYLE ) & WS_POPUP) && (hwndTo = GetWindow( hwnd, GW_OWNER )))
    {
        hwndTo = NtUserGetAncestor( hwndTo, GA_ROOT );
        if (can_activate_window( hwndTo )) goto done;
    }

    hwndTo = hwnd;
    for (;;)
    {
        if (!(hwndTo = GetWindow( hwndTo, GW_HWNDNEXT ))) break;
        if (can_activate_window( hwndTo )) goto done;
    }

    hwndTo = GetTopWindow( 0 );
    for (;;)
    {
        if (hwndTo == hwnd)
        {
            hwndTo = 0;
            break;
        }
        if (can_activate_window( hwndTo )) goto done;
        if (!(hwndTo = GetWindow( hwndTo, GW_HWNDNEXT ))) break;
    }

 done:
    fg = NtUserGetForegroundWindow();
    TRACE("win = %p fg = %p\n", hwndTo, fg);
    if (!fg || (hwnd == fg))
    {
        if (SetForegroundWindow( hwndTo )) return;
    }
    if (!NtUserSetActiveWindow( hwndTo )) NtUserSetActiveWindow( 0 );
}


/***********************************************************************
 *           WINPOS_HandleWindowPosChanging
 *
 * Default handling for a WM_WINDOWPOSCHANGING. Called from DefWindowProc().
 */
LONG WINPOS_HandleWindowPosChanging( HWND hwnd, WINDOWPOS *winpos )
{
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );

    if (winpos->flags & SWP_NOSIZE) return 0;
    if ((style & WS_THICKFRAME) || ((style & (WS_POPUP | WS_CHILD)) == 0))
    {
	MINMAXINFO info = NtUserGetMinMaxInfo( hwnd );
        winpos->cx = min( winpos->cx, info.ptMaxTrackSize.x );
        winpos->cy = min( winpos->cy, info.ptMaxTrackSize.y );
	if (!(style & WS_MINIMIZE))
	{
            winpos->cx = max( winpos->cx, info.ptMinTrackSize.x );
            winpos->cy = max( winpos->cy, info.ptMinTrackSize.y );
	}
    }
    return 0;
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


/***********************************************************************
 *           draw_moving_frame
 *
 * Draw the frame used when moving or resizing window.
 */
static void draw_moving_frame( HWND parent, HDC hdc, RECT *screen_rect, BOOL thickframe )
{
    RECT rect = *screen_rect;

    if (parent) MapWindowPoints( 0, parent, (POINT *)&rect, 2 );
    if (thickframe)
    {
        const int width = GetSystemMetrics(SM_CXFRAME);
        const int height = GetSystemMetrics(SM_CYFRAME);

        HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
        PatBlt( hdc, rect.left, rect.top,
                rect.right - rect.left - width, height, PATINVERT );
        PatBlt( hdc, rect.left, rect.top + height, width,
                rect.bottom - rect.top - height, PATINVERT );
        PatBlt( hdc, rect.left + width, rect.bottom - 1,
                rect.right - rect.left - width, -height, PATINVERT );
        PatBlt( hdc, rect.right - 1, rect.top, -width,
                rect.bottom - rect.top - height, PATINVERT );
        SelectObject( hdc, hbrush );
    }
    else DrawFocusRect( hdc, &rect );
}


/***********************************************************************
 *           start_size_move
 *
 * Initialization of a move or resize, when initiated from a menu choice.
 * Return hit test code for caption or sizing border.
 */
static LONG start_size_move( HWND hwnd, WPARAM wParam, POINT *capturePoint, LONG style )
{
    LONG hittest = 0;
    POINT pt;
    MSG msg;
    RECT rectWindow;

    GetWindowRect( hwnd, &rectWindow );

    if ((wParam & 0xfff0) == SC_MOVE)
    {
        /* Move pointer at the center of the caption */
        RECT rect = rectWindow;
        /* Note: to be exactly centered we should take the different types
         * of border into account, but it shouldn't make more than a few pixels
         * of difference so let's not bother with that */
        rect.top += GetSystemMetrics(SM_CYBORDER);
        if (style & WS_SYSMENU)
            rect.left += GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MINIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        if (style & WS_MAXIMIZEBOX)
            rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        pt.x = (rect.right + rect.left) / 2;
        pt.y = rect.top + GetSystemMetrics(SM_CYSIZE)/2;
        hittest = HTCAPTION;
        *capturePoint = pt;
    }
    else  /* SC_SIZE */
    {
        NtUserSetCursor( LoadCursorW( 0, (LPWSTR)IDC_SIZEALL ) );
        pt.x = pt.y = 0;
        while(!hittest)
        {
            if (!GetMessageW( &msg, 0, 0, 0 )) return 0;
            if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                pt.x = min( max( msg.pt.x, rectWindow.left ), rectWindow.right - 1 );
                pt.y = min( max( msg.pt.y, rectWindow.top ), rectWindow.bottom - 1 );
                hittest = SendMessageW( hwnd, WM_NCHITTEST, 0, MAKELONG( pt.x, pt.y ) );
                if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT)) hittest = 0;
                break;

            case WM_LBUTTONUP:
                return 0;

            case WM_KEYDOWN:
                switch(msg.wParam)
                {
                case VK_UP:
                    hittest = HTTOP;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.top + GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_DOWN:
                    hittest = HTBOTTOM;
                    pt.x =(rectWindow.left+rectWindow.right)/2;
                    pt.y = rectWindow.bottom - GetSystemMetrics(SM_CYFRAME) / 2;
                    break;
                case VK_LEFT:
                    hittest = HTLEFT;
                    pt.x = rectWindow.left + GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RIGHT:
                    hittest = HTRIGHT;
                    pt.x = rectWindow.right - GetSystemMetrics(SM_CXFRAME) / 2;
                    pt.y =(rectWindow.top+rectWindow.bottom)/2;
                    break;
                case VK_RETURN:
                case VK_ESCAPE:
                    return 0;
                }
                break;
            default:
                TranslateMessage( &msg );
                DispatchMessageW( &msg );
                break;
            }
        }
        *capturePoint = pt;
    }
    NtUserSetCursorPos( pt.x, pt.y );
    SendMessageW( hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           WINPOS_SysCommandSizeMove
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
void WINPOS_SysCommandSizeMove( HWND hwnd, WPARAM wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect, origRect;
    HDC hdc;
    HWND parent;
    LONG hittest = (LONG)(wParam & 0x0f);
    WPARAM syscommand = wParam & 0xfff0;
    MINMAXINFO minmax;
    POINT capturePoint, pt;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL    thickframe = HAS_THICKFRAME( style );
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = TRUE;
    HMONITOR mon = 0;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd)) return;

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;
    NtUserClipCursor( NULL );

    TRACE("hwnd %p command %04lx, hittest %d, pos %d,%d\n",
          hwnd, syscommand, hittest, pt.x, pt.y);

    if (syscommand == SC_MOVE)
    {
        if (!hittest) hittest = start_size_move( hwnd, wParam, &capturePoint, style );
        if (!hittest) return;
    }
    else  /* SC_SIZE */
    {
        if ( hittest && (syscommand != SC_MOUSEMENU) )
            hittest += (HTLEFT - WMSZ_LEFT);
        else
        {
            set_capture_window( hwnd, GUI_INMOVESIZE, NULL );
            hittest = start_size_move( hwnd, wParam, &capturePoint, style );
            if (!hittest)
            {
                set_capture_window( 0, GUI_INMOVESIZE, NULL );
                return;
            }
        }
    }

      /* Get min/max info */

    minmax = NtUserGetMinMaxInfo( hwnd );
    WIN_GetRectangles( hwnd, COORDS_PARENT, &sizingRect, NULL );
    origRect = sizingRect;
    if (style & WS_CHILD)
    {
        parent = GetParent(hwnd);
        GetClientRect( parent, &mouseRect );
        MapWindowPoints( parent, 0, (LPPOINT)&mouseRect, 2 );
        MapWindowPoints( parent, 0, (LPPOINT)&sizingRect, 2 );
    }
    else
    {
        parent = 0;
        mouseRect = get_virtual_screen_rect();
        mon = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
    }

    if (ON_LEFT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.right-minmax.ptMaxTrackSize.x+capturePoint.x-sizingRect.left );
        mouseRect.right = min( mouseRect.right, sizingRect.right-minmax.ptMinTrackSize.x+capturePoint.x-sizingRect.left );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.left+minmax.ptMinTrackSize.x+capturePoint.x-sizingRect.right );
        mouseRect.right = min( mouseRect.right, sizingRect.left+minmax.ptMaxTrackSize.x+capturePoint.x-sizingRect.right );
    }
    if (ON_TOP_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.bottom-minmax.ptMaxTrackSize.y+capturePoint.y-sizingRect.top );
        mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minmax.ptMinTrackSize.y+capturePoint.y-sizingRect.top);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.top+minmax.ptMinTrackSize.y+capturePoint.y-sizingRect.bottom );
        mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+minmax.ptMaxTrackSize.y+capturePoint.y-sizingRect.bottom );
    }

    /* Retrieve a default cache DC (without using the window style) */
    hdc = NtUserGetDCEx( parent, 0, DCX_CACHE );

    /* we only allow disabling the full window drag for child windows */
    if (parent) SystemParametersInfoW( SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0 );

    /* repaint the window before moving it around */
    NtUserRedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    SendMessageW( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_capture_window( hwnd, GUI_INMOVESIZE, NULL );

    while(1)
    {
        int dx = 0, dy = 0;

        if (!GetMessageW( &msg, 0, 0, 0 )) break;
        if (NtUserCallMsgFilter( &msg, MSGF_SIZE )) continue;

        /* Exit on button-up, Return, or Esc */
        if ((msg.message == WM_LBUTTONUP) ||
            ((msg.message == WM_KEYDOWN) &&
             ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

        if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
            continue;  /* We are not interested in other messages */
        }

        pt = msg.pt;

        if (msg.message == WM_KEYDOWN) switch(msg.wParam)
        {
        case VK_UP:    pt.y -= 8; break;
        case VK_DOWN:  pt.y += 8; break;
        case VK_LEFT:  pt.x -= 8; break;
        case VK_RIGHT: pt.x += 8; break;
        }

        pt.x = max( pt.x, mouseRect.left );
        pt.x = min( pt.x, mouseRect.right - 1 );
        pt.y = max( pt.y, mouseRect.top );
        pt.y = min( pt.y, mouseRect.bottom - 1 );

        if (!parent)
        {
            HMONITOR newmon;
            MONITORINFO info;

            if ((newmon = MonitorFromPoint( pt, MONITOR_DEFAULTTONULL )))
                mon = newmon;

            info.cbSize = sizeof(info);
            if (mon && GetMonitorInfoW( mon, &info ))
            {
                pt.x = max( pt.x, info.rcWork.left );
                pt.x = min( pt.x, info.rcWork.right - 1 );
                pt.y = max( pt.y, info.rcWork.top );
                pt.y = min( pt.y, info.rcWork.bottom - 1 );
            }
        }

        dx = pt.x - capturePoint.x;
        dy = pt.y - capturePoint.y;

        if (dx || dy)
        {
            if( !moved )
            {
                moved = TRUE;
                if (!DragFullWindows)
                    draw_moving_frame( parent, hdc, &sizingRect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) NtUserSetCursorPos( pt.x, pt.y );
            else
            {
                if (!DragFullWindows) draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                if (hittest == HTCAPTION || hittest == HTBORDER) OffsetRect( &sizingRect, dx, dy );
                if (ON_LEFT_BORDER(hittest)) sizingRect.left += dx;
                else if (ON_RIGHT_BORDER(hittest)) sizingRect.right += dx;
                if (ON_TOP_BORDER(hittest)) sizingRect.top += dy;
                else if (ON_BOTTOM_BORDER(hittest)) sizingRect.bottom += dy;
                capturePoint = pt;

                /* determine the hit location */
                if (syscommand == SC_SIZE && hittest != HTBORDER)
                {
                    WPARAM wpSizingHit = 0;

                    if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                        wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                    SendMessageW( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&sizingRect );
                }
                else
                    SendMessageW( hwnd, WM_MOVING, 0, (LPARAM)&sizingRect );

                if (!DragFullWindows)
                    draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                else
                {
                    RECT rect = sizingRect;
                    MapWindowPoints( 0, parent, (POINT *)&rect, 2 );
                    NtUserSetWindowPos( hwnd, 0, rect.left, rect.top,
                                        rect.right - rect.left, rect.bottom - rect.top,
                                        (hittest == HTCAPTION) ? SWP_NOSIZE : 0 );
                }
            }
        }
    }

    if (moved && !DragFullWindows)
    {
        draw_moving_frame( parent, hdc, &sizingRect, thickframe );
    }

    set_capture_window( 0, GUI_INMOVESIZE, NULL );
    NtUserReleaseDC( parent, hdc );
    if (parent) MapWindowPoints( 0, parent, (POINT *)&sizingRect, 2 );

    if (HOOK_CallHooks( WH_CBT, HCBT_MOVESIZE, (WPARAM)hwnd, (LPARAM)&sizingRect, TRUE ))
        moved = FALSE;

    SendMessageW( hwnd, WM_EXITSIZEMOVE, 0, 0 );
    SendMessageW( hwnd, WM_SETVISIBLE, !IsIconic(hwnd), 0L);

    /* window moved or resized */
    if (moved)
    {
        /* if the moving/resizing isn't canceled call SetWindowPos
         * with the new position or the new size of the window
         */
        if (!((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
        {
            /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
            if (!DragFullWindows)
                NtUserSetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
                                    sizingRect.right - sizingRect.left,
                                    sizingRect.bottom - sizingRect.top,
                                    ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
        else
        { /* restore previous size/position */
            if(DragFullWindows)
                NtUserSetWindowPos( hwnd, 0, origRect.left, origRect.top,
                                    origRect.right - origRect.left,
                                    origRect.bottom - origRect.top,
                                    ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
    }

    if (IsIconic(hwnd))
    {
        /* Single click brings up the system menu when iconized */

        if( !moved )
        {
            if(style & WS_SYSMENU )
                SendMessageW( hwnd, WM_SYSCOMMAND,
                              SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
        }
    }
}
