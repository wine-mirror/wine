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

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOCLIENTSIZE | SWP_NOZORDER)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)
#define SWP_AGG_NOCLIENTCHANGE \
        (SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_BORDER)))

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
    if (IsIconic( hwnd )) ShowWindow( hwnd, SW_RESTORE );
    else BringWindowToTop( hwnd );
}


/***********************************************************************
 *		GetWindowRect (USER32.@)
 */
BOOL WINAPI GetWindowRect( HWND hwnd, LPRECT rect )
{
    BOOL ret = NtUserCallHwndParam( hwnd, (UINT_PTR)rect, NtUserGetWindowRect );
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
    return NtUserCallHwndParam( hwnd, (UINT_PTR)rect, NtUserGetClientRect );
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
HWND WINAPI ChildWindowFromPoint( HWND hwndParent, POINT pt )
{
    return ChildWindowFromPointEx( hwndParent, pt, CWP_ALL );
}

/*******************************************************************
 *		RealChildWindowFromPoint (USER32.@)
 */
HWND WINAPI RealChildWindowFromPoint( HWND hwndParent, POINT pt )
{
    return ChildWindowFromPointEx( hwndParent, pt, CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE );
}

/*******************************************************************
 *		ChildWindowFromPointEx (USER32.@)
 */
HWND WINAPI ChildWindowFromPointEx( HWND hwndParent, POINT pt, UINT uFlags)
{
    /* pt is in the client coordinates */
    HWND *list;
    int i;
    RECT rect;
    HWND retvalue;

    GetClientRect( hwndParent, &rect );
    if (!PtInRect( &rect, pt )) return 0;
    if (!(list = WIN_ListChildren( hwndParent ))) return hwndParent;

    for (i = 0; list[i]; i++)
    {
        if (!WIN_GetRectangles( list[i], COORDS_PARENT, &rect, NULL )) continue;
        if (!PtInRect( &rect, pt )) continue;
        if (uFlags & (CWP_SKIPINVISIBLE|CWP_SKIPDISABLED))
        {
            LONG style = GetWindowLongW( list[i], GWL_STYLE );
            if ((uFlags & CWP_SKIPINVISIBLE) && !(style & WS_VISIBLE)) continue;
            if ((uFlags & CWP_SKIPDISABLED) && (style & WS_DISABLED)) continue;
        }
        if (uFlags & CWP_SKIPTRANSPARENT)
        {
            if (GetWindowLongW( list[i], GWL_EXSTYLE ) & WS_EX_TRANSPARENT) continue;
        }
        break;
    }
    retvalue = list[i];
    HeapFree( GetProcessHeap(), 0, list );
    if (!retvalue) retvalue = hwndParent;
    return retvalue;
}


/*******************************************************************
 *         WINPOS_GetWinOffset
 *
 * Calculate the offset between the origin of the two windows. Used
 * to implement MapWindowPoints.
 */
static BOOL WINPOS_GetWinOffset( HWND hwndFrom, HWND hwndTo, BOOL *mirrored, POINT *ret_offset )
{
    WND * wndPtr;
    POINT offset;
    BOOL mirror_from, mirror_to, ret;
    HWND hwnd;

    offset.x = offset.y = 0;
    *mirrored = mirror_from = mirror_to = FALSE;

    /* Translate source window origin to screen coords */
    if (hwndFrom)
    {
        if (!(wndPtr = WIN_GetPtr( hwndFrom )))
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return FALSE;
        }
        if (wndPtr == WND_OTHER_PROCESS) goto other_process;
        if (wndPtr != WND_DESKTOP)
        {
            if (wndPtr->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_from = TRUE;
                offset.x += wndPtr->client_rect.right - wndPtr->client_rect.left;
            }
            while (wndPtr->parent)
            {
                offset.x += wndPtr->client_rect.left;
                offset.y += wndPtr->client_rect.top;
                hwnd = wndPtr->parent;
                WIN_ReleasePtr( wndPtr );
                if (!(wndPtr = WIN_GetPtr( hwnd ))) break;
                if (wndPtr == WND_OTHER_PROCESS) goto other_process;
                if (wndPtr == WND_DESKTOP) break;
                if (wndPtr->flags & WIN_CHILDREN_MOVED)
                {
                    WIN_ReleasePtr( wndPtr );
                    goto other_process;
                }
            }
            if (wndPtr && wndPtr != WND_DESKTOP) WIN_ReleasePtr( wndPtr );
            offset = point_win_to_thread_dpi( hwndFrom, offset );
        }
    }

    /* Translate origin to destination window coords */
    if (hwndTo)
    {
        if (!(wndPtr = WIN_GetPtr( hwndTo )))
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return FALSE;
        }
        if (wndPtr == WND_OTHER_PROCESS) goto other_process;
        if (wndPtr != WND_DESKTOP)
        {
            POINT pt = { 0, 0 };
            if (wndPtr->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_to = TRUE;
                pt.x += wndPtr->client_rect.right - wndPtr->client_rect.left;
            }
            while (wndPtr->parent)
            {
                pt.x += wndPtr->client_rect.left;
                pt.y += wndPtr->client_rect.top;
                hwnd = wndPtr->parent;
                WIN_ReleasePtr( wndPtr );
                if (!(wndPtr = WIN_GetPtr( hwnd ))) break;
                if (wndPtr == WND_OTHER_PROCESS) goto other_process;
                if (wndPtr == WND_DESKTOP) break;
                if (wndPtr->flags & WIN_CHILDREN_MOVED)
                {
                    WIN_ReleasePtr( wndPtr );
                    goto other_process;
                }
            }
            if (wndPtr && wndPtr != WND_DESKTOP) WIN_ReleasePtr( wndPtr );
            pt = point_win_to_thread_dpi( hwndTo, pt );
            offset.x -= pt.x;
            offset.y -= pt.y;
        }
    }

    *mirrored = mirror_from ^ mirror_to;
    if (mirror_from) offset.x = -offset.x;
    *ret_offset = offset;
    return TRUE;

 other_process:  /* one of the parents may belong to another process, do it the hard way */
    SERVER_START_REQ( get_windows_offset )
    {
        req->from = wine_server_user_handle( hwndFrom );
        req->to   = wine_server_user_handle( hwndTo );
        req->dpi  = get_thread_dpi();
        if ((ret = !wine_server_call_err( req )))
        {
            ret_offset->x = reply->x;
            ret_offset->y = reply->y;
            *mirrored = reply->mirror;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/*******************************************************************
 *		MapWindowPoints (USER32.@)
 */
INT WINAPI MapWindowPoints( HWND hwndFrom, HWND hwndTo, LPPOINT lppt, UINT count )
{
    BOOL mirrored;
    POINT offset;
    UINT i;

    if (!WINPOS_GetWinOffset( hwndFrom, hwndTo, &mirrored, &offset )) return 0;

    for (i = 0; i < count; i++)
    {
        lppt[i].x += offset.x;
        lppt[i].y += offset.y;
        if (mirrored) lppt[i].x = -lppt[i].x;
    }
    if (mirrored && count == 2)  /* special case for rectangle */
    {
        int tmp = lppt[0].x;
        lppt[0].x = lppt[1].x;
        lppt[1].x = tmp;
    }
    return MAKELONG( LOWORD(offset.x), LOWORD(offset.y) );
}


/*******************************************************************
 *		ClientToScreen (USER32.@)
 */
BOOL WINAPI ClientToScreen( HWND hwnd, LPPOINT lppnt )
{
    POINT offset;
    BOOL mirrored;

    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    if (!WINPOS_GetWinOffset( hwnd, 0, &mirrored, &offset )) return FALSE;
    lppnt->x += offset.x;
    lppnt->y += offset.y;
    if (mirrored) lppnt->x = -lppnt->x;
    return TRUE;
}


/*******************************************************************
 *		ScreenToClient (USER32.@)
 */
BOOL WINAPI ScreenToClient( HWND hwnd, LPPOINT lppnt )
{
    POINT offset;
    BOOL mirrored;

    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    if (!WINPOS_GetWinOffset( 0, hwnd, &mirrored, &offset )) return FALSE;
    lppnt->x += offset.x;
    lppnt->y += offset.y;
    if (mirrored) lppnt->x = -lppnt->x;
    return TRUE;
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


/*******************************************************************
 *           WINPOS_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
MINMAXINFO WINPOS_GetMinMaxInfo( HWND hwnd )
{
    MINMAXINFO info;
    NtUserCallHwndParam( hwnd, (UINT_PTR)&info, NtUserGetMinMaxInfo );
    return info;
}


static POINT get_first_minimized_child_pos( const RECT *parent, const MINIMIZEDMETRICS *mm,
                                            int width, int height )
{
    POINT ret;

    if (mm->iArrange & ARW_STARTRIGHT)
        ret.x = parent->right - mm->iHorzGap - width;
    else
        ret.x = parent->left + mm->iHorzGap;
    if (mm->iArrange & ARW_STARTTOP)
        ret.y = parent->top + mm->iVertGap;
    else
        ret.y = parent->bottom - mm->iVertGap - height;

    return ret;
}

static void get_next_minimized_child_pos( const RECT *parent, const MINIMIZEDMETRICS *mm,
                                          int width, int height, POINT *pos )
{
    BOOL next;

    if (mm->iArrange & ARW_UP) /* == ARW_DOWN */
    {
        if (mm->iArrange & ARW_STARTTOP)
        {
            pos->y += height + mm->iVertGap;
            if ((next = pos->y + height > parent->bottom))
                pos->y = parent->top + mm->iVertGap;
        }
        else
        {
            pos->y -= height + mm->iVertGap;
            if ((next = pos->y < parent->top))
                pos->y = parent->bottom - mm->iVertGap - height;
        }

        if (next)
        {
            if (mm->iArrange & ARW_STARTRIGHT)
                pos->x -= width + mm->iHorzGap;
            else
                pos->x += width + mm->iHorzGap;
        }
    }
    else
    {
        if (mm->iArrange & ARW_STARTRIGHT)
        {
            pos->x -= width + mm->iHorzGap;
            if ((next = pos->x < parent->left))
                pos->x = parent->right - mm->iHorzGap - width;
        }
        else
        {
            pos->x += width + mm->iHorzGap;
            if ((next = pos->x + width > parent->right))
                pos->x = parent->left + mm->iHorzGap;
        }

        if (next)
        {
            if (mm->iArrange & ARW_STARTTOP)
                pos->y += height + mm->iVertGap;
            else
                pos->y -= height + mm->iVertGap;
        }
    }
}

static POINT get_minimized_pos( HWND hwnd, POINT pt )
{
    RECT rect, rectParent;
    HWND parent, child;
    HRGN hrgn, tmp;
    MINIMIZEDMETRICS metrics;
    int width, height;

    parent = NtUserGetAncestor( hwnd, GA_PARENT );
    if (parent == GetDesktopWindow())
    {
        MONITORINFO mon_info;
        HMONITOR monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );

        mon_info.cbSize = sizeof( mon_info );
        GetMonitorInfoW( monitor, &mon_info );
        rectParent = mon_info.rcWork;
    }
    else GetClientRect( parent, &rectParent );

    if ((pt.x >= rectParent.left) && (pt.x + GetSystemMetrics( SM_CXMINIMIZED ) < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + GetSystemMetrics( SM_CYMINIMIZED ) < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    width = GetSystemMetrics( SM_CXMINIMIZED );
    height = GetSystemMetrics( SM_CYMINIMIZED );

    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );

    /* Check if another icon already occupies this spot */
    /* FIXME: this is completely inefficient */

    hrgn = CreateRectRgn( 0, 0, 0, 0 );
    tmp = CreateRectRgn( 0, 0, 0, 0 );
    for (child = GetWindow( parent, GW_CHILD ); child; child = GetWindow( child, GW_HWNDNEXT ))
    {
        if (child == hwnd) continue;
        if ((GetWindowLongW( child, GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != (WS_VISIBLE|WS_MINIMIZE))
            continue;
        if (WIN_GetRectangles( child, COORDS_PARENT, &rect, NULL ))
        {
            SetRectRgn( tmp, rect.left, rect.top, rect.right, rect.bottom );
            CombineRgn( hrgn, hrgn, tmp, RGN_OR );
        }
    }
    DeleteObject( tmp );

    pt = get_first_minimized_child_pos( &rectParent, &metrics, width, height );
    for (;;)
    {
        SetRect( &rect, pt.x, pt.y, pt.x + width, pt.y + height );
        if (!RectInRegion( hrgn, &rect ))
            break;

        get_next_minimized_child_pos( &rectParent, &metrics, width, height, &pt );
    }

    DeleteObject( hrgn );
    return pt;
}


/***********************************************************************
 *           WINPOS_MinMaximize
 */
UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect )
{
    UINT swpFlags = 0;
    LONG old_style;
    MINMAXINFO minmax;
    WINDOWPLACEMENT wpl;

    TRACE("%p %u\n", hwnd, cmd );

    wpl.length = sizeof(wpl);
    GetWindowPlacement( hwnd, &wpl );

    if (HOOK_CallHooks( WH_CBT, HCBT_MINMAX, (WPARAM)hwnd, cmd, TRUE ))
        return SWP_NOSIZE | SWP_NOMOVE;

    if (IsIconic( hwnd ))
    {
        switch (cmd)
        {
        case SW_SHOWMINNOACTIVE:
        case SW_SHOWMINIMIZED:
        case SW_FORCEMINIMIZE:
        case SW_MINIMIZE:
            wpl.ptMinPosition = get_minimized_pos( hwnd, wpl.ptMinPosition );

            SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                     wpl.ptMinPosition.x + GetSystemMetrics(SM_CXMINIMIZED),
                     wpl.ptMinPosition.y + GetSystemMetrics(SM_CYMINIMIZED) );
            return SWP_NOSIZE | SWP_NOMOVE;
        }
        if (!SendMessageW( hwnd, WM_QUERYOPEN, 0, 0 )) return SWP_NOSIZE | SWP_NOMOVE;
        swpFlags |= SWP_NOCOPYBITS;
    }

    switch( cmd )
    {
    case SW_SHOWMINNOACTIVE:
    case SW_SHOWMINIMIZED:
    case SW_FORCEMINIMIZE:
    case SW_MINIMIZE:
        if (IsZoomed( hwnd )) win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );
        else win_set_flags( hwnd, 0, WIN_RESTORE_MAX );

        if (GetFocus() == hwnd)
        {
            if (GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD)
                NtUserSetFocus( NtUserGetAncestor( hwnd, GA_PARENT ));
            else
                NtUserSetFocus( 0 );
        }

        old_style = WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );

        wpl.ptMinPosition = get_minimized_pos( hwnd, wpl.ptMinPosition );

        if (!(old_style & WS_MINIMIZE)) swpFlags |= SWP_STATECHANGED;
        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 wpl.ptMinPosition.x + GetSystemMetrics(SM_CXMINIMIZED),
                 wpl.ptMinPosition.y + GetSystemMetrics(SM_CYMINIMIZED) );
        swpFlags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        old_style = GetWindowLongW( hwnd, GWL_STYLE );
        if ((old_style & WS_MAXIMIZE) && (old_style & WS_VISIBLE)) return SWP_NOSIZE | SWP_NOMOVE;

        minmax = WINPOS_GetMinMaxInfo( hwnd );

        old_style = WIN_SetStyle( hwnd, WS_MAXIMIZE, WS_MINIMIZE );
        if (old_style & WS_MINIMIZE)
            win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );

        if (!(old_style & WS_MAXIMIZE)) swpFlags |= SWP_STATECHANGED;
        SetRect( rect, minmax.ptMaxPosition.x, minmax.ptMaxPosition.y,
                 minmax.ptMaxPosition.x + minmax.ptMaxSize.x, minmax.ptMaxPosition.y + minmax.ptMaxSize.y );
        break;

    case SW_SHOWNOACTIVATE:
        win_set_flags( hwnd, 0, WIN_RESTORE_MAX );
        /* fall through */
    case SW_SHOWNORMAL:
    case SW_RESTORE:
    case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
        old_style = WIN_SetStyle( hwnd, 0, WS_MINIMIZE | WS_MAXIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            if (win_get_flags( hwnd ) & WIN_RESTORE_MAX)
            {
                /* Restore to maximized position */
                minmax = WINPOS_GetMinMaxInfo( hwnd );
                WIN_SetStyle( hwnd, WS_MAXIMIZE, 0 );
                swpFlags |= SWP_STATECHANGED;
                SetRect( rect, minmax.ptMaxPosition.x, minmax.ptMaxPosition.y,
                         minmax.ptMaxPosition.x + minmax.ptMaxSize.x, minmax.ptMaxPosition.y + minmax.ptMaxSize.y );
                break;
            }
        }
        else if (!(old_style & WS_MAXIMIZE)) break;

        swpFlags |= SWP_STATECHANGED;

        /* Restore to normal position */

        *rect = wpl.rcNormalPosition;
        break;
    }

    return swpFlags;
}


/***********************************************************************
 *              show_window
 *
 * Implementation of ShowWindow and ShowWindowAsync.
 */
static BOOL show_window( HWND hwnd, INT cmd )
{
    WND *wndPtr;
    HWND parent;
    DPI_AWARENESS_CONTEXT context;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL wasVisible = (style & WS_VISIBLE) != 0;
    BOOL showFlag = TRUE;
    RECT newPos = {0, 0, 0, 0};
    UINT new_swp, swp = 0;

    TRACE("hwnd=%p, cmd=%d, wasVisible %d\n", hwnd, cmd, wasVisible);

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) goto done;
            showFlag = FALSE;
            swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (style & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
        case SW_MINIMIZE:
        case SW_FORCEMINIMIZE: /* FIXME: Does not work if thread is hung. */
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            swp |= WINPOS_MinMaximize( hwnd, cmd, &newPos );
            if ((style & WS_MINIMIZE) && wasVisible) goto done;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            if (!wasVisible) swp |= SWP_SHOWWINDOW;
            swp |= SWP_FRAMECHANGED;
            swp |= WINPOS_MinMaximize( hwnd, SW_MAXIMIZE, &newPos );
            if ((style & WS_MAXIMIZE) && wasVisible) goto done;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (style & WS_CHILD) swp |= SWP_NOZORDER;
            break;
	case SW_SHOW:
            if (wasVisible) goto done;
	    swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (style & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWNOACTIVATE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_RESTORE:
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
            if (!wasVisible) swp |= SWP_SHOWWINDOW;
            if (style & (WS_MINIMIZE | WS_MAXIMIZE))
            {
                swp |= SWP_FRAMECHANGED;
                swp |= WINPOS_MinMaximize( hwnd, cmd, &newPos );
            }
            else
            {
                if (wasVisible) goto done;
                swp |= SWP_NOSIZE | SWP_NOMOVE;
            }
            if (style & WS_CHILD && !(swp & SWP_STATECHANGED)) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	    break;
        default:
            goto done;
    }

    if ((showFlag != wasVisible || cmd == SW_SHOWNA) && cmd != SW_SHOWMAXIMIZED && !(swp & SWP_STATECHANGED))
    {
        SendMessageW( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) goto done;
    }

    if (IsRectEmpty( &newPos )) new_swp = swp;
    else if ((new_swp = USER_Driver->pShowWindow( hwnd, cmd, &newPos, swp )) == ~0)
    {
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) new_swp = swp;
        else if (IsIconic( hwnd ) && (newPos.left != -32000 || newPos.top != -32000))
        {
            OffsetRect( &newPos, -32000 - newPos.left, -32000 - newPos.top );
            new_swp = swp & ~(SWP_NOMOVE | SWP_NOCLIENTMOVE);
        }
        else new_swp = swp;
    }
    swp = new_swp;

    parent = NtUserGetAncestor( hwnd, GA_PARENT );
    if (parent && !IsWindowVisible( parent ) && !(swp & SWP_STATECHANGED))
    {
        /* if parent is not visible simply toggle WS_VISIBLE and return */
        if (showFlag) WIN_SetStyle( hwnd, WS_VISIBLE, 0 );
        else WIN_SetStyle( hwnd, 0, WS_VISIBLE );
    }
    else
        NtUserSetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                            newPos.right - newPos.left, newPos.bottom - newPos.top, swp );

    if (cmd == SW_HIDE)
    {
        HWND hFocus;

        /* FIXME: This will cause the window to be activated irrespective
         * of whether it is owned by the same thread. Has to be done
         * asynchronously.
         */

        if (hwnd == GetActiveWindow())
            WINPOS_ActivateOtherWindow(hwnd);

        /* Revert focus to parent */
        hFocus = GetFocus();
        if (hwnd == hFocus)
        {
            HWND parent = NtUserGetAncestor(hwnd, GA_PARENT);
            if (parent == GetDesktopWindow()) parent = 0;
            NtUserSetFocus( parent );
        }
        goto done;
    }

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) goto done;

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;
        RECT client;
        LPARAM lparam;

        WIN_GetRectangles( hwnd, COORDS_PARENT, NULL, &client );
        lparam = MAKELONG( client.right - client.left, client.bottom - client.top );
	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
        else if (wndPtr->dwStyle & WS_MINIMIZE)
        {
            wParam = SIZE_MINIMIZED;
            lparam = 0;
        }
        WIN_ReleasePtr( wndPtr );

        SendMessageW( hwnd, WM_SIZE, wParam, lparam );
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG( client.left, client.top ));
    }
    else WIN_ReleasePtr( wndPtr );

    /* if previous state was minimized Windows sets focus to the window */
    if (style & WS_MINIMIZE)
    {
        NtUserSetFocus( hwnd );
        /* Send a WM_ACTIVATE message for a top level window, even if the window is already active */
        if (NtUserGetAncestor( hwnd, GA_ROOT ) == hwnd && !(swp & SWP_NOACTIVATE))
            SendMessageW( hwnd, WM_ACTIVATE, WA_ACTIVE, 0 );
    }

done:
    SetThreadDpiAwarenessContext( context );
    return wasVisible;
}


/***********************************************************************
 *		ShowWindowAsync (USER32.@)
 *
 * doesn't wait; returns immediately.
 * used by threads to toggle windows in other (possibly hanging) threads
 */
BOOL WINAPI ShowWindowAsync( HWND hwnd, INT cmd )
{
    HWND full_handle;

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ((full_handle = WIN_IsCurrentThread( hwnd )))
        return show_window( full_handle, cmd );

    return SendNotifyMessageW( hwnd, WM_WINE_SHOWWINDOW, cmd, 0 );
}


/***********************************************************************
 *		ShowWindow (USER32.@)
 */
BOOL WINAPI ShowWindow( HWND hwnd, INT cmd )
{
    HWND full_handle;

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if ((full_handle = WIN_IsCurrentThread( hwnd )))
        return show_window( full_handle, cmd );

    if ((cmd == SW_HIDE) && !(GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        return FALSE;

    if ((cmd == SW_SHOW) && (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        return TRUE;

    return SendMessageW( hwnd, WM_WINE_SHOWWINDOW, cmd, 0 );
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
    return NtUserCallHwndParam( hwnd, (UINT_PTR)wndpl, NtUserGetWindowPlacement );
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

    ShowWindow( hwnd, wndpl->showCmd );

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

	ShowWindow(hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA));

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
	MINMAXINFO info = WINPOS_GetMinMaxInfo( hwnd );
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
 *		update_surface_region
 */
static void update_surface_region( HWND hwnd )
{
    NTSTATUS status;
    HRGN region = 0;
    RGNDATA *data;
    size_t size = 256;
    WND *win = WIN_GetPtr( hwnd );

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return;
    if (!win->surface) goto done;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( RGNDATA, Buffer[size] )))) goto done;

        SERVER_START_REQ( get_surface_region )
        {
            req->window = wine_server_user_handle( hwnd );
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                if (reply_size)
                {
                    data->rdh.dwSize   = sizeof(data->rdh);
                    data->rdh.iType    = RDH_RECTANGLES;
                    data->rdh.nCount   = reply_size / sizeof(RECT);
                    data->rdh.nRgnSize = reply_size;
                    region = ExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                    OffsetRgn( region, -reply->visible_rect.left, -reply->visible_rect.top );
                }
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) goto done;

    win->surface->funcs->set_region( win->surface, region );
    if (region) DeleteObject( region );

done:
    WIN_ReleasePtr( win );
}


/***********************************************************************
 *		set_window_pos
 *
 * Backend implementation of SetWindowPos.
 */
BOOL set_window_pos( HWND hwnd, HWND insert_after, UINT swp_flags,
                     const RECT *window_rect, const RECT *client_rect, const RECT *valid_rects )
{
    WND *win;
    HWND surface_win = 0, parent = NtUserGetAncestor( hwnd, GA_PARENT );
    BOOL ret, needs_update = FALSE;
    RECT visible_rect, old_visible_rect, old_window_rect, old_client_rect, extra_rects[3];
    struct window_surface *old_surface, *new_surface = NULL;
    struct window_surface *dummy_surface = (struct window_surface *)NtUserCallHwnd( 0, NtUserGetDummySurface );

    if (!parent || parent == GetDesktopWindow())
    {
        new_surface = dummy_surface;  /* provide a default surface for top-level windows */
        window_surface_add_ref( new_surface );
    }
    visible_rect = *window_rect;
    if (!(ret = USER_Driver->pWindowPosChanging( hwnd, insert_after, swp_flags,
                                                 window_rect, client_rect, &visible_rect, &new_surface )))
    {
        if (IsRectEmpty( window_rect )) visible_rect = *window_rect;
        else
        {
            visible_rect = get_virtual_screen_rect();
            IntersectRect( &visible_rect, &visible_rect, window_rect );
        }
    }

    WIN_GetRectangles( hwnd, COORDS_SCREEN, &old_window_rect, NULL );
    if (IsRectEmpty( &valid_rects[0] )) valid_rects = NULL;

    if (!(win = WIN_GetPtr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        if (new_surface) window_surface_release( new_surface );
        return FALSE;
    }

    /* create or update window surface for top-level windows if the driver doesn't implement WindowPosChanging */
    if (!ret && new_surface && !IsRectEmpty( &visible_rect ) &&
        (!(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED) ||
           NtUserGetLayeredWindowAttributes( hwnd, NULL, NULL, NULL )))
    {
        window_surface_release( new_surface );
        if ((new_surface = win->surface)) window_surface_add_ref( new_surface );
        create_offscreen_window_surface( &visible_rect, &new_surface );
    }

    old_visible_rect = win->visible_rect;
    old_client_rect = win->client_rect;
    old_surface = win->surface;
    if (old_surface != new_surface) swp_flags |= SWP_FRAMECHANGED;  /* force refreshing non-client area */
    if (new_surface == dummy_surface) swp_flags |= SWP_NOREDRAW;
    else if (old_surface == dummy_surface)
    {
        swp_flags |= SWP_NOCOPYBITS;
        valid_rects = NULL;
    }

    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = wine_server_user_handle( hwnd );
        req->previous      = wine_server_user_handle( insert_after );
        req->swp_flags     = swp_flags;
        req->window.left   = window_rect->left;
        req->window.top    = window_rect->top;
        req->window.right  = window_rect->right;
        req->window.bottom = window_rect->bottom;
        req->client.left   = client_rect->left;
        req->client.top    = client_rect->top;
        req->client.right  = client_rect->right;
        req->client.bottom = client_rect->bottom;
        if (!EqualRect( window_rect, &visible_rect ) || new_surface || valid_rects)
        {
            extra_rects[0] = extra_rects[1] = visible_rect;
            if (new_surface)
            {
                extra_rects[1] = new_surface->rect;
                OffsetRect( &extra_rects[1], visible_rect.left, visible_rect.top );
            }
            if (valid_rects) extra_rects[2] = valid_rects[0];
            else SetRectEmpty( &extra_rects[2] );
            wine_server_add_data( req, extra_rects, sizeof(extra_rects) );
        }
        if (new_surface) req->paint_flags |= SET_WINPOS_PAINT_SURFACE;
        if (win->pixel_format) req->paint_flags |= SET_WINPOS_PIXEL_FORMAT;

        if ((ret = !wine_server_call( req )))
        {
            win->dwStyle    = reply->new_style;
            win->dwExStyle  = reply->new_ex_style;
            win->window_rect  = *window_rect;
            win->client_rect  = *client_rect;
            win->visible_rect = visible_rect;
            win->surface      = new_surface;
            surface_win       = wine_server_ptr_handle( reply->surface_win );
            needs_update      = reply->needs_update;
            if (GetWindowLongW( win->parent, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
            {
                RECT client;
                GetClientRect( win->parent, &client );
                mirror_rect( &client, &win->window_rect );
                mirror_rect( &client, &win->client_rect );
                mirror_rect( &client, &win->visible_rect );
            }
            /* if an RTL window is resized the children have moved */
            if (win->dwExStyle & WS_EX_LAYOUTRTL &&
                client_rect->right - client_rect->left != old_client_rect.right - old_client_rect.left)
                win->flags |= WIN_CHILDREN_MOVED;
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        if (needs_update) update_surface_region( surface_win );
        if (((swp_flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) ||
            (swp_flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW | SWP_STATECHANGED | SWP_FRAMECHANGED)))
            invalidate_dce( win, &old_window_rect );
    }

    WIN_ReleasePtr( win );

    if (ret)
    {
        TRACE( "win %p surface %p -> %p\n", hwnd, old_surface, new_surface );
        register_window_surface( old_surface, new_surface );
        if (old_surface)
        {
            if (valid_rects)
            {
                move_window_bits( hwnd, old_surface, new_surface, &visible_rect,
                                  &old_visible_rect, window_rect, valid_rects );
                valid_rects = NULL;  /* prevent the driver from trying to also move the bits */
            }
            window_surface_release( old_surface );
        }
        else if (surface_win && surface_win != hwnd)
        {
            if (valid_rects)
            {
                RECT rects[2];
                int x_offset = old_visible_rect.left - visible_rect.left;
                int y_offset = old_visible_rect.top - visible_rect.top;

                /* if all that happened is that the whole window moved, copy everything */
                if (!(swp_flags & SWP_FRAMECHANGED) &&
                    old_visible_rect.right  - visible_rect.right  == x_offset &&
                    old_visible_rect.bottom - visible_rect.bottom == y_offset &&
                    old_client_rect.left    - client_rect->left   == x_offset &&
                    old_client_rect.right   - client_rect->right  == x_offset &&
                    old_client_rect.top     - client_rect->top    == y_offset &&
                    old_client_rect.bottom  - client_rect->bottom == y_offset &&
                    EqualRect( &valid_rects[0], client_rect ))
                {
                    rects[0] = visible_rect;
                    rects[1] = old_visible_rect;
                    valid_rects = rects;
                }
                move_window_bits_parent( hwnd, surface_win, window_rect, valid_rects );
                valid_rects = NULL;  /* prevent the driver from trying to also move the bits */
            }
        }

        USER_Driver->pWindowPosChanged( hwnd, insert_after, swp_flags, window_rect,
                                        client_rect, &visible_rect, valid_rects, new_surface );
    }
    else if (new_surface) window_surface_release( new_surface );

    return ret;
}


/***********************************************************************
 *		BeginDeferWindowPos (USER32.@)
 */
HDWP WINAPI BeginDeferWindowPos( INT count )
{
    return UlongToHandle( NtUserCallOneParam( count, NtUserBeginDeferWindowPos ));
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
    int width, height, count = 0;
    RECT rectParent;
    HWND hwndChild;
    POINT pt;
    MINIMIZEDMETRICS metrics;

    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );
    width = GetSystemMetrics( SM_CXMINIMIZED );
    height = GetSystemMetrics( SM_CYMINIMIZED );

    if (parent == GetDesktopWindow())
    {
        MONITORINFO mon_info;
        HMONITOR monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTOPRIMARY );

        mon_info.cbSize = sizeof( mon_info );
        GetMonitorInfoW( monitor, &mon_info );
        rectParent = mon_info.rcWork;
    }
    else GetClientRect( parent, &rectParent );

    pt = get_first_minimized_child_pos( &rectParent, &metrics, width, height );

    hwndChild = GetWindow( parent, GW_CHILD );
    while (hwndChild)
    {
        if( IsIconic( hwndChild ) )
        {
            NtUserSetWindowPos( hwndChild, 0, pt.x, pt.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            get_next_minimized_child_pos( &rectParent, &metrics, width, height, &pt );
            count++;
        }
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return count;
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
            if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

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

    minmax = WINPOS_GetMinMaxInfo( hwnd );
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
        if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

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
