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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/server.h"
#include "controls.h"
#include "user_private.h"
#include "wine/gdi_driver.h"
#include "win.h"
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

#define EMPTYPOINT(pt) ((pt).x == -1 && (pt).y == -1)

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

typedef struct
{
    struct user_object obj;
    INT       actualCount;
    INT       suggestedCount;
    HWND      hwndParent;
    WINDOWPOS *winPos;
} DWP;


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
    BOOL ret = WIN_GetRectangles( hwnd, COORDS_SCREEN, rect, NULL );
    if (ret) TRACE( "hwnd %p %s\n", hwnd, wine_dbgstr_rect(rect) );
    return ret;
}


/***********************************************************************
 *		GetWindowRgn (USER32.@)
 */
int WINAPI GetWindowRgn ( HWND hwnd, HRGN hrgn )
{
    int nRet = ERROR;
    NTSTATUS status;
    HRGN win_rgn = 0;
    RGNDATA *data;
    size_t size = 256;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return ERROR;
        }
        SERVER_START_REQ( get_window_region )
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
                    win_rgn = ExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                }
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    else if (win_rgn)
    {
        nRet = CombineRgn( hrgn, win_rgn, 0, RGN_COPY );
        DeleteObject( win_rgn );
    }
    return nRet;
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
 *		SetWindowRgn (USER32.@)
 */
int WINAPI SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL bRedraw )
{
    static const RECT empty_rect;
    BOOL ret;

    if (hrgn)
    {
        RGNDATA *data;
        DWORD size;

        if (!(size = GetRegionData( hrgn, 0, NULL ))) return FALSE;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
        if (!GetRegionData( hrgn, size, data ))
        {
            HeapFree( GetProcessHeap(), 0, data );
            return FALSE;
        }
        SERVER_START_REQ( set_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            req->redraw = (bRedraw != 0);
            if (data->rdh.nCount)
                wine_server_add_data( req, data->Buffer, data->rdh.nCount * sizeof(RECT) );
            else
                wine_server_add_data( req, &empty_rect, sizeof(empty_rect) );
            ret = !wine_server_call_err( req );
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    }
    else  /* clear existing region */
    {
        SERVER_START_REQ( set_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            req->redraw = (bRedraw != 0);
            ret = !wine_server_call_err( req );
        }
        SERVER_END_REQ;
    }

    if (ret)
    {
        UINT swp_flags = SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE;
        if (!bRedraw) swp_flags |= SWP_NOREDRAW;
        SetWindowPos( hwnd, 0, 0, 0, 0, 0, swp_flags );
        USER_Driver->pSetWindowRgn( hwnd, hrgn, bRedraw );
        if (hrgn) DeleteObject( hrgn );
    }
    return ret;
}


/***********************************************************************
 *		GetClientRect (USER32.@)
 */
BOOL WINAPI GetClientRect( HWND hwnd, LPRECT rect )
{
    return WIN_GetRectangles( hwnd, COORDS_CLIENT, NULL, rect );
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
        if (style & WS_MINIMIZE)
        {
            *hittest = HTCAPTION;
            break;
        }
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
    INT hittest;
    return WINPOS_WindowFromPoint( 0, pt, &hittest );
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

/* map coordinates of a window region */
void map_window_region( HWND from, HWND to, HRGN hrgn )
{
    BOOL mirrored;
    POINT offset;
    UINT i, size;
    RGNDATA *data;
    HRGN new_rgn;
    RECT *rect;

    if (!WINPOS_GetWinOffset( from, to, &mirrored, &offset )) return;

    if (!mirrored)
    {
        OffsetRgn( hrgn, offset.x, offset.y );
        return;
    }
    if (!(size = GetRegionData( hrgn, 0, NULL ))) return;
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return;
    GetRegionData( hrgn, size, data );
    rect = (RECT *)data->Buffer;
    for (i = 0; i < data->rdh.nCount; i++)
    {
        int tmp = -(rect[i].left + offset.x);
        rect[i].left    = -(rect[i].right + offset.x);
        rect[i].right   = tmp;
        rect[i].top    += offset.y;
        rect[i].bottom += offset.y;
    }
    if ((new_rgn = ExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data )))
    {
        CombineRgn( hrgn, new_rgn, 0, RGN_COPY );
        DeleteObject( new_rgn );
    }
    HeapFree( GetProcessHeap(), 0, data );
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
    return SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
}


/***********************************************************************
 *		MoveWindow (USER32.@)
 */
BOOL WINAPI MoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy,
                            BOOL repaint )
{
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
    TRACE("%p %d,%d %dx%d %d\n", hwnd, x, y, cx, cy, repaint );
    return SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}


/***********************************************************************
 *           WINPOS_RedrawIconTitle
 */
BOOL WINPOS_RedrawIconTitle( HWND hWnd )
{
    HWND icon_title = 0;
    WND *win = WIN_GetPtr( hWnd );

    if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
    {
	icon_title = win->icon_title;
        WIN_ReleasePtr( win );
    }
    if (!icon_title) return FALSE;
    SendMessageW( icon_title, WM_SHOWWINDOW, TRUE, 0 );
    InvalidateRect( icon_title, NULL, TRUE );
    return TRUE;
}

/***********************************************************************
 *           WINPOS_ShowIconTitle
 */
static void WINPOS_ShowIconTitle( HWND hwnd, BOOL bShow )
{
    WND *win = WIN_GetPtr( hwnd );
    HWND title = 0;

    TRACE("%p %i\n", hwnd, (bShow != 0) );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return;
    if (win->window_rect.left == -32000 || win->window_rect.top == -32000)
    {
        TRACE( "not showing title for hidden icon %p\n", hwnd );
        bShow = FALSE;
    }
    else title = win->icon_title;
    WIN_ReleasePtr( win );

    if (bShow)
    {
        if (!title)
        {
            title = ICONTITLE_Create( hwnd );
            if (!(win = WIN_GetPtr( hwnd )) || win == WND_OTHER_PROCESS)
            {
                DestroyWindow( title );
                return;
            }
            win->icon_title = title;
            WIN_ReleasePtr( win );
        }
        if (!IsWindowVisible(title))
        {
            SendMessageW( title, WM_SHOWWINDOW, TRUE, 0 );
            SetWindowPos( title, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                          SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
        }
    }
    else if (title) ShowWindow( title, SW_HIDE );
}

/*******************************************************************
 *           WINPOS_GetMinMaxInfo
 *
 * Get the minimized and maximized information for a window.
 */
MINMAXINFO WINPOS_GetMinMaxInfo( HWND hwnd )
{
    DPI_AWARENESS_CONTEXT context;
    MINMAXINFO MinMax;
    HMONITOR monitor;
    INT xinc, yinc;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG adjustedStyle;
    LONG exstyle = GetWindowLongW( hwnd, GWL_EXSTYLE );
    RECT rc;
    WND *win;

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));

    /* Compute default values */

    GetWindowRect(hwnd, &rc);
    MinMax.ptReserved.x = rc.left;
    MinMax.ptReserved.y = rc.top;

    if ((style & WS_CAPTION) == WS_CAPTION)
        adjustedStyle = style & ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
    else
        adjustedStyle = style;

    GetClientRect(GetAncestor(hwnd,GA_PARENT), &rc);
    AdjustWindowRectEx(&rc, adjustedStyle, ((style & WS_POPUP) && GetMenu(hwnd)), exstyle);

    xinc = -rc.left;
    yinc = -rc.top;

    MinMax.ptMaxSize.x = rc.right - rc.left;
    MinMax.ptMaxSize.y = rc.bottom - rc.top;
    if (style & (WS_DLGFRAME | WS_BORDER))
    {
        MinMax.ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
        MinMax.ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
    }
    else
    {
        MinMax.ptMinTrackSize.x = 2 * xinc;
        MinMax.ptMinTrackSize.y = 2 * yinc;
    }
    MinMax.ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
    MinMax.ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);
    MinMax.ptMaxPosition.x = -xinc;
    MinMax.ptMaxPosition.y = -yinc;

    if ((win = WIN_GetPtr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        if (!EMPTYPOINT(win->max_pos)) MinMax.ptMaxPosition = win->max_pos;
        WIN_ReleasePtr( win );
    }

    SendMessageW( hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax );

    /* if the app didn't change the values, adapt them for the current monitor */

    if ((monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY )))
    {
        RECT rc_work;
        MONITORINFO mon_info;

        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );

        rc_work = mon_info.rcMonitor;

        if (style & WS_MAXIMIZEBOX)
        {
            if ((style & WS_CAPTION) == WS_CAPTION || !(style & (WS_CHILD | WS_POPUP)))
                rc_work = mon_info.rcWork;
        }

        if (MinMax.ptMaxSize.x == GetSystemMetrics(SM_CXSCREEN) + 2 * xinc &&
            MinMax.ptMaxSize.y == GetSystemMetrics(SM_CYSCREEN) + 2 * yinc)
        {
            MinMax.ptMaxSize.x = (rc_work.right - rc_work.left) + 2 * xinc;
            MinMax.ptMaxSize.y = (rc_work.bottom - rc_work.top) + 2 * yinc;
        }
        if (MinMax.ptMaxPosition.x == -xinc && MinMax.ptMaxPosition.y == -yinc)
        {
            MinMax.ptMaxPosition.x = rc_work.left - xinc;
            MinMax.ptMaxPosition.y = rc_work.top - yinc;
        }
    }

      /* Some sanity checks */

    TRACE("%d %d / %d %d / %d %d / %d %d\n",
                      MinMax.ptMaxSize.x, MinMax.ptMaxSize.y,
                      MinMax.ptMaxPosition.x, MinMax.ptMaxPosition.y,
                      MinMax.ptMaxTrackSize.x, MinMax.ptMaxTrackSize.y,
                      MinMax.ptMinTrackSize.x, MinMax.ptMinTrackSize.y);
    MinMax.ptMaxTrackSize.x = max( MinMax.ptMaxTrackSize.x,
                                   MinMax.ptMinTrackSize.x );
    MinMax.ptMaxTrackSize.y = max( MinMax.ptMaxTrackSize.y,
                                   MinMax.ptMinTrackSize.y );

    SetThreadDpiAwarenessContext( context );
    return MinMax;
}


/***********************************************************************
 *           WINPOS_FindIconPos
 *
 * Find a suitable place for an iconic window.
 */
static POINT WINPOS_FindIconPos( HWND hwnd, POINT pt )
{
    RECT rect, rectParent;
    HWND parent, child;
    HRGN hrgn, tmp;
    int x, y, xspacing, yspacing;
    MINIMIZEDMETRICS metrics;

    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );

    parent = GetAncestor( hwnd, GA_PARENT );
    if (parent == GetDesktopWindow())
    {
        MONITORINFO mon_info;
        HMONITOR monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY );

        mon_info.cbSize = sizeof( mon_info );
        GetMonitorInfoW( monitor, &mon_info );
        rectParent = mon_info.rcWork;
    }
    else GetClientRect( parent, &rectParent );

    if ((pt.x >= rectParent.left) && (pt.x + GetSystemMetrics(SM_CXICON) < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + GetSystemMetrics(SM_CYICON) < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    xspacing = GetSystemMetrics(SM_CXICONSPACING);
    yspacing = GetSystemMetrics(SM_CYICONSPACING);

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

    for (y = 0; y < (rectParent.bottom - rectParent.top) / yspacing; y++)
    {
        if (metrics.iArrange & ARW_STARTTOP)
        {
            rect.top = rectParent.top + y * yspacing;
            rect.bottom = rect.top + yspacing;
        }
        else
        {
            rect.bottom = rectParent.bottom - y * yspacing;
            rect.top = rect.bottom - yspacing;
        }
        for (x = 0; x < (rectParent.right - rectParent.left) / xspacing; x++)
        {
            if (metrics.iArrange & ARW_STARTRIGHT)
            {
                rect.right = rectParent.right - x * xspacing;
                rect.left = rect.right - xspacing;
            }
            else
            {
                rect.left = rectParent.left + x * xspacing;
                rect.right = rect.left + xspacing;
            }
            if (!RectInRegion( hrgn, &rect ))
            {
                /* No window was found, so it's OK for us */
                pt.x = rect.left + (xspacing - GetSystemMetrics(SM_CXICON)) / 2;
                pt.y = rect.top + (yspacing - GetSystemMetrics(SM_CYICON)) / 2;
                DeleteObject( hrgn );
                return pt;
            }
        }
    }
    DeleteObject( hrgn );
    pt.x = pt.y = 0;
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
            wpl.ptMinPosition = WINPOS_FindIconPos( hwnd, wpl.ptMinPosition );

            SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                     wpl.ptMinPosition.x + GetSystemMetrics(SM_CXICON),
                     wpl.ptMinPosition.y + GetSystemMetrics(SM_CYICON) );
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
                SetFocus(GetAncestor(hwnd, GA_PARENT));
            else
                SetFocus(0);
        }

        old_style = WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );

        wpl.ptMinPosition = WINPOS_FindIconPos( hwnd, wpl.ptMinPosition );

        if (!(old_style & WS_MINIMIZE)) swpFlags |= SWP_STATECHANGED;
        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 wpl.ptMinPosition.x + GetSystemMetrics(SM_CXICON),
                 wpl.ptMinPosition.y + GetSystemMetrics(SM_CYICON) );
        swpFlags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        old_style = GetWindowLongW( hwnd, GWL_STYLE );
        if ((old_style & WS_MAXIMIZE) && (old_style & WS_VISIBLE)) return SWP_NOSIZE | SWP_NOMOVE;

        minmax = WINPOS_GetMinMaxInfo( hwnd );

        old_style = WIN_SetStyle( hwnd, WS_MAXIMIZE, WS_MINIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );
            WINPOS_ShowIconTitle( hwnd, FALSE );
        }

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
            WINPOS_ShowIconTitle( hwnd, FALSE );
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
    UINT swp = 0;

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

    swp = USER_Driver->pShowWindow( hwnd, cmd, &newPos, swp );

    parent = GetAncestor( hwnd, GA_PARENT );
    if (parent && !IsWindowVisible( parent ) && !(swp & SWP_STATECHANGED))
    {
        /* if parent is not visible simply toggle WS_VISIBLE and return */
        if (showFlag) WIN_SetStyle( hwnd, WS_VISIBLE, 0 );
        else WIN_SetStyle( hwnd, 0, WS_VISIBLE );
    }
    else
        SetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                      newPos.right - newPos.left, newPos.bottom - newPos.top, swp );

    if (cmd == SW_HIDE)
    {
        HWND hFocus;

        WINPOS_ShowIconTitle( hwnd, FALSE );

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
            HWND parent = GetAncestor(hwnd, GA_PARENT);
            if (parent == GetDesktopWindow()) parent = 0;
            SetFocus(parent);
        }
        goto done;
    }

    if (IsIconic(hwnd)) WINPOS_ShowIconTitle( hwnd, TRUE );

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
    if (style & WS_MINIMIZE) SetFocus( hwnd );

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


/***********************************************************************
 *		GetWindowPlacement (USER32.@)
 *
 * Win95:
 * Fails if wndpl->length of Win95 (!) apps is invalid.
 */
BOOL WINAPI GetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *wndpl )
{
    WND *pWnd = WIN_GetPtr( hwnd );

    if (!pWnd) return FALSE;

    if (pWnd == WND_DESKTOP)
    {
        wndpl->length  = sizeof(*wndpl);
        wndpl->showCmd = SW_SHOWNORMAL;
        wndpl->flags = 0;
        wndpl->ptMinPosition.x = -1;
        wndpl->ptMinPosition.y = -1;
        wndpl->ptMaxPosition.x = -1;
        wndpl->ptMaxPosition.y = -1;
        GetWindowRect( hwnd, &wndpl->rcNormalPosition );
        return TRUE;
    }
    if (pWnd == WND_OTHER_PROCESS)
    {
        if (!IsWindow( hwnd )) return FALSE;
        FIXME( "not supported on other process window %p\n", hwnd );
        /* provide some dummy information */
        wndpl->length  = sizeof(*wndpl);
        wndpl->showCmd = SW_SHOWNORMAL;
        wndpl->flags = 0;
        wndpl->ptMinPosition.x = -1;
        wndpl->ptMinPosition.y = -1;
        wndpl->ptMaxPosition.x = -1;
        wndpl->ptMaxPosition.y = -1;
        GetWindowRect( hwnd, &wndpl->rcNormalPosition );
        return TRUE;
    }

    /* update the placement according to the current style */
    if (pWnd->dwStyle & WS_MINIMIZE)
    {
        pWnd->min_pos.x = pWnd->window_rect.left;
        pWnd->min_pos.y = pWnd->window_rect.top;
    }
    else if (pWnd->dwStyle & WS_MAXIMIZE)
    {
        pWnd->max_pos.x = pWnd->window_rect.left;
        pWnd->max_pos.y = pWnd->window_rect.top;
    }
    else
    {
        pWnd->normal_rect = pWnd->window_rect;
    }

    wndpl->length  = sizeof(*wndpl);
    if( pWnd->dwStyle & WS_MINIMIZE )
        wndpl->showCmd = SW_SHOWMINIMIZED;
    else
        wndpl->showCmd = ( pWnd->dwStyle & WS_MAXIMIZE ) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL ;
    if( pWnd->flags & WIN_RESTORE_MAX )
        wndpl->flags = WPF_RESTORETOMAXIMIZED;
    else
        wndpl->flags = 0;
    wndpl->ptMinPosition = EMPTYPOINT(pWnd->min_pos) ? pWnd->min_pos : point_win_to_thread_dpi( hwnd, pWnd->min_pos );
    wndpl->ptMaxPosition = EMPTYPOINT(pWnd->max_pos) ? pWnd->max_pos : point_win_to_thread_dpi( hwnd, pWnd->max_pos );
    wndpl->rcNormalPosition = rect_win_to_thread_dpi( hwnd, pWnd->normal_rect );
    WIN_ReleasePtr( pWnd );

    TRACE( "%p: returning min %d,%d max %d,%d normal %s\n",
           hwnd, wndpl->ptMinPosition.x, wndpl->ptMinPosition.y,
           wndpl->ptMaxPosition.x, wndpl->ptMaxPosition.y,
           wine_dbgstr_rect(&wndpl->rcNormalPosition) );
    return TRUE;
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
    WND *pWnd = WIN_GetPtr( hwnd );
    WINDOWPLACEMENT wp = *wndpl;

    if (flags & PLACE_MIN) make_point_onscreen( &wp.ptMinPosition );
    if (flags & PLACE_MAX) make_point_onscreen( &wp.ptMaxPosition );
    if (flags & PLACE_RECT) make_rect_onscreen( &wp.rcNormalPosition );

    TRACE( "%p: setting min %d,%d max %d,%d normal %s flags %x ajusted to min %d,%d max %d,%d normal %s\n",
           hwnd, wndpl->ptMinPosition.x, wndpl->ptMinPosition.y,
           wndpl->ptMaxPosition.x, wndpl->ptMaxPosition.y,
           wine_dbgstr_rect(&wndpl->rcNormalPosition), flags,
           wp.ptMinPosition.x, wp.ptMinPosition.y, wp.ptMaxPosition.x, wp.ptMaxPosition.y,
           wine_dbgstr_rect(&wp.rcNormalPosition) );

    if (!pWnd || pWnd == WND_OTHER_PROCESS || pWnd == WND_DESKTOP) return FALSE;

    if (flags & PLACE_MIN) pWnd->min_pos = point_thread_to_win_dpi( hwnd, wp.ptMinPosition );
    if (flags & PLACE_MAX) pWnd->max_pos = point_thread_to_win_dpi( hwnd, wp.ptMaxPosition );
    if (flags & PLACE_RECT) pWnd->normal_rect = rect_thread_to_win_dpi( hwnd, wp.rcNormalPosition );

    style = pWnd->dwStyle;

    WIN_ReleasePtr( pWnd );

    if( style & WS_MINIMIZE )
    {
        if (flags & PLACE_MIN)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            SetWindowPos( hwnd, 0, wp.ptMinPosition.x, wp.ptMinPosition.y, 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    else if( style & WS_MAXIMIZE )
    {
        if (flags & PLACE_MAX)
            SetWindowPos( hwnd, 0, wp.ptMaxPosition.x, wp.ptMaxPosition.y, 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
    }
    else if( flags & PLACE_RECT )
        SetWindowPos( hwnd, 0, wp.rcNormalPosition.left, wp.rcNormalPosition.top,
                      wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                      wp.rcNormalPosition.bottom - wp.rcNormalPosition.top,
                      SWP_NOZORDER | SWP_NOACTIVATE );

    ShowWindow( hwnd, wndpl->showCmd );

    if (IsIconic( hwnd ))
    {
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE) WINPOS_ShowIconTitle( hwnd, TRUE );

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
        hwndTo = GetAncestor( hwndTo, GA_ROOT );
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
    fg = GetForegroundWindow();
    TRACE("win = %p fg = %p\n", hwndTo, fg);
    if (!fg || (hwnd == fg))
    {
        if (SetForegroundWindow( hwndTo )) return;
    }
    if (!SetActiveWindow( hwndTo )) SetActiveWindow(0);
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
 *           dump_winpos_flags
 */
static void dump_winpos_flags(UINT flags)
{
    static const DWORD dumped_flags = (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW |
                                       SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW |
                                       SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER |
                                       SWP_NOSENDCHANGING | SWP_DEFERERASE | SWP_ASYNCWINDOWPOS |
                                       SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_STATECHANGED);
    TRACE("flags:");
    if(flags & SWP_NOSIZE) TRACE(" SWP_NOSIZE");
    if(flags & SWP_NOMOVE) TRACE(" SWP_NOMOVE");
    if(flags & SWP_NOZORDER) TRACE(" SWP_NOZORDER");
    if(flags & SWP_NOREDRAW) TRACE(" SWP_NOREDRAW");
    if(flags & SWP_NOACTIVATE) TRACE(" SWP_NOACTIVATE");
    if(flags & SWP_FRAMECHANGED) TRACE(" SWP_FRAMECHANGED");
    if(flags & SWP_SHOWWINDOW) TRACE(" SWP_SHOWWINDOW");
    if(flags & SWP_HIDEWINDOW) TRACE(" SWP_HIDEWINDOW");
    if(flags & SWP_NOCOPYBITS) TRACE(" SWP_NOCOPYBITS");
    if(flags & SWP_NOOWNERZORDER) TRACE(" SWP_NOOWNERZORDER");
    if(flags & SWP_NOSENDCHANGING) TRACE(" SWP_NOSENDCHANGING");
    if(flags & SWP_DEFERERASE) TRACE(" SWP_DEFERERASE");
    if(flags & SWP_ASYNCWINDOWPOS) TRACE(" SWP_ASYNCWINDOWPOS");
    if(flags & SWP_NOCLIENTSIZE) TRACE(" SWP_NOCLIENTSIZE");
    if(flags & SWP_NOCLIENTMOVE) TRACE(" SWP_NOCLIENTMOVE");
    if(flags & SWP_STATECHANGED) TRACE(" SWP_STATECHANGED");

    if(flags & ~dumped_flags) TRACE(" %08x", flags & ~dumped_flags);
    TRACE("\n");
}


/***********************************************************************
 *           map_dpi_winpos
 */
static void map_dpi_winpos( WINDOWPOS *winpos )
{
    UINT dpi_from = get_thread_dpi();
    UINT dpi_to = GetDpiForWindow( winpos->hwnd );

    if (!dpi_from) dpi_from = get_win_monitor_dpi( winpos->hwnd );
    if (dpi_from == dpi_to) return;
    winpos->x  = MulDiv( winpos->x, dpi_to, dpi_from );
    winpos->y  = MulDiv( winpos->y, dpi_to, dpi_from );
    winpos->cx = MulDiv( winpos->cx, dpi_to, dpi_from );
    winpos->cy = MulDiv( winpos->cy, dpi_to, dpi_from );
}

/***********************************************************************
 *           SWP_DoWinPosChanging
 */
static BOOL SWP_DoWinPosChanging( WINDOWPOS *pWinpos, RECT *old_window_rect, RECT *old_client_rect,
                                  RECT *new_window_rect, RECT *new_client_rect )
{
    WND *wndPtr;

    /* Send WM_WINDOWPOSCHANGING message */

    if (!(pWinpos->flags & SWP_NOSENDCHANGING)
           && !((pWinpos->flags & SWP_AGG_NOCLIENTCHANGE) && (pWinpos->flags & SWP_SHOWWINDOW)))
        SendMessageW( pWinpos->hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)pWinpos );

    if (!(wndPtr = WIN_GetPtr( pWinpos->hwnd )) ||
        wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return FALSE;

    /* Calculate new position and size */

    WIN_GetRectangles( pWinpos->hwnd, COORDS_PARENT, old_window_rect, old_client_rect );
    *new_window_rect = *old_window_rect;
    *new_client_rect = *old_client_rect;

    if (!(pWinpos->flags & SWP_NOSIZE))
    {
        if (wndPtr->dwStyle & WS_MINIMIZE)
        {
            new_window_rect->right  = new_window_rect->left + GetSystemMetrics(SM_CXICON);
            new_window_rect->bottom = new_window_rect->top + GetSystemMetrics(SM_CYICON);
        }
        else
        {
            new_window_rect->right  = new_window_rect->left + pWinpos->cx;
            new_window_rect->bottom = new_window_rect->top + pWinpos->cy;
        }
    }
    if (!(pWinpos->flags & SWP_NOMOVE))
    {
        new_window_rect->left    = pWinpos->x;
        new_window_rect->top     = pWinpos->y;
        new_window_rect->right  += pWinpos->x - old_window_rect->left;
        new_window_rect->bottom += pWinpos->y - old_window_rect->top;

        OffsetRect( new_client_rect, pWinpos->x - old_window_rect->left,
                                     pWinpos->y - old_window_rect->top );
    }
    pWinpos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

    TRACE( "hwnd %p, after %p, swp %d,%d %dx%d flags %08x current %s style %08x new %s\n",
           pWinpos->hwnd, pWinpos->hwndInsertAfter, pWinpos->x, pWinpos->y,
           pWinpos->cx, pWinpos->cy, pWinpos->flags,
           wine_dbgstr_rect( old_window_rect ), wndPtr->dwStyle,
           wine_dbgstr_rect( new_window_rect ));

    WIN_ReleasePtr( wndPtr );
    return TRUE;
}

/***********************************************************************
 *           get_valid_rects
 *
 * Compute the valid rects from the old and new client rect and WVR_* flags.
 * Helper for WM_NCCALCSIZE handling.
 */
static inline void get_valid_rects( const RECT *old_client, const RECT *new_client, UINT flags,
                                    RECT *valid )
{
    int cx, cy;

    if (flags & WVR_REDRAW)
    {
        SetRectEmpty( &valid[0] );
        SetRectEmpty( &valid[1] );
        return;
    }

    if (flags & WVR_VALIDRECTS)
    {
        if (!IntersectRect( &valid[0], &valid[0], new_client ) ||
            !IntersectRect( &valid[1], &valid[1], old_client ))
        {
            SetRectEmpty( &valid[0] );
            SetRectEmpty( &valid[1] );
            return;
        }
        flags = WVR_ALIGNLEFT | WVR_ALIGNTOP;
    }
    else
    {
        valid[0] = *new_client;
        valid[1] = *old_client;
    }

    /* make sure the rectangles have the same size */
    cx = min( valid[0].right - valid[0].left, valid[1].right - valid[1].left );
    cy = min( valid[0].bottom - valid[0].top, valid[1].bottom - valid[1].top );

    if (flags & WVR_ALIGNBOTTOM)
    {
        valid[0].top = valid[0].bottom - cy;
        valid[1].top = valid[1].bottom - cy;
    }
    else
    {
        valid[0].bottom = valid[0].top + cy;
        valid[1].bottom = valid[1].top + cy;
    }
    if (flags & WVR_ALIGNRIGHT)
    {
        valid[0].left = valid[0].right - cx;
        valid[1].left = valid[1].right - cx;
    }
    else
    {
        valid[0].right = valid[0].left + cx;
        valid[1].right = valid[1].left + cx;
    }
}


/***********************************************************************
 *           SWP_DoOwnedPopups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 *
 * FIXME: hide/show owned popups when owner visibility changes.
 */
static HWND SWP_DoOwnedPopups(HWND hwnd, HWND hwndInsertAfter)
{
    HWND owner, *list = NULL;
    unsigned int i;

    TRACE("(%p) hInsertAfter = %p\n", hwnd, hwndInsertAfter );

    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) return hwndInsertAfter;

    if ((owner = GetWindow( hwnd, GW_OWNER )))
    {
        /* make sure this popup stays above the owner */

        if (hwndInsertAfter != HWND_TOPMOST)
        {
            if (!(list = WIN_ListChildren( GetDesktopWindow() ))) return hwndInsertAfter;

            for (i = 0; list[i]; i++)
            {
                BOOL topmost = (GetWindowLongW( list[i], GWL_EXSTYLE ) & WS_EX_TOPMOST) != 0;

                if (list[i] == owner)
                {
                    if (i > 0) hwndInsertAfter = list[i-1];
                    else hwndInsertAfter = topmost ? HWND_TOPMOST : HWND_TOP;
                    break;
                }

                if (hwndInsertAfter == HWND_TOP || hwndInsertAfter == HWND_NOTOPMOST)
                {
                    if (!topmost) break;
                }
                else if (list[i] == hwndInsertAfter) break;
            }
        }
    }

    if (hwndInsertAfter == HWND_BOTTOM) goto done;
    if (!list && !(list = WIN_ListChildren( GetDesktopWindow() ))) goto done;

    i = 0;
    if (hwndInsertAfter == HWND_TOP || hwndInsertAfter == HWND_NOTOPMOST)
    {
        if (hwndInsertAfter == HWND_NOTOPMOST || !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_TOPMOST))
        {
            /* skip all the topmost windows */
            while (list[i] && (GetWindowLongW( list[i], GWL_EXSTYLE ) & WS_EX_TOPMOST)) i++;
        }
    }
    else if (hwndInsertAfter != HWND_TOPMOST)
    {
        /* skip windows that are already placed correctly */
        for (i = 0; list[i]; i++)
        {
            if (list[i] == hwndInsertAfter) break;
            if (list[i] == hwnd) goto done;  /* nothing to do if window is moving backwards in z-order */
        }
    }

    for ( ; list[i]; i++)
    {
        if (list[i] == hwnd) break;
        if (GetWindow( list[i], GW_OWNER ) != hwnd) continue;
        TRACE( "moving %p owned by %p after %p\n", list[i], hwnd, hwndInsertAfter );
        SetWindowPos( list[i], hwndInsertAfter, 0, 0, 0, 0,
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE );
        hwndInsertAfter = list[i];
    }

done:
    HeapFree( GetProcessHeap(), 0, list );
    return hwndInsertAfter;
}

/***********************************************************************
 *           SWP_DoNCCalcSize
 */
static UINT SWP_DoNCCalcSize( WINDOWPOS *pWinpos, const RECT *old_window_rect, const RECT *old_client_rect,
                              const RECT *new_window_rect, RECT *new_client_rect, RECT *validRects,
                              int parent_x, int parent_y )
{
    UINT wvrFlags = 0;

      /* Send WM_NCCALCSIZE message to get new client area */
    if( (pWinpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
        NCCALCSIZE_PARAMS params;
        WINDOWPOS winposCopy;

        params.rgrc[0] = *new_window_rect;
        params.rgrc[1] = *old_window_rect;
        params.rgrc[2] = *old_client_rect;
        params.lppos = &winposCopy;
        winposCopy = *pWinpos;

        wvrFlags = SendMessageW( pWinpos->hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params );

        *new_client_rect = params.rgrc[0];

        TRACE( "hwnd %p old win %s old client %s new win %s new client %s\n", pWinpos->hwnd,
               wine_dbgstr_rect(old_window_rect), wine_dbgstr_rect(old_client_rect),
               wine_dbgstr_rect(new_window_rect), wine_dbgstr_rect(new_client_rect) );

        if (new_client_rect->left != old_client_rect->left - parent_x ||
            new_client_rect->top != old_client_rect->top - parent_y)
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;

        if( (new_client_rect->right - new_client_rect->left !=
             old_client_rect->right - old_client_rect->left))
            pWinpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvrFlags &= ~WVR_HREDRAW;

        if (new_client_rect->bottom - new_client_rect->top !=
            old_client_rect->bottom - old_client_rect->top)
            pWinpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvrFlags &= ~WVR_VREDRAW;

        validRects[0] = params.rgrc[1];
        validRects[1] = params.rgrc[2];
    }
    else
    {
        if (!(pWinpos->flags & SWP_NOMOVE) &&
            (new_client_rect->left != old_client_rect->left - parent_x ||
             new_client_rect->top != old_client_rect->top - parent_y))
            pWinpos->flags &= ~SWP_NOCLIENTMOVE;
    }

    if (pWinpos->flags & (SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_HIDEWINDOW))
    {
        SetRectEmpty( &validRects[0] );
        SetRectEmpty( &validRects[1] );
    }
    else get_valid_rects( old_client_rect, new_client_rect, wvrFlags, validRects );

    return wvrFlags;
}

/* fix redundant flags and values in the WINDOWPOS structure */
static BOOL fixup_flags( WINDOWPOS *winpos, const RECT *old_window_rect, int parent_x, int parent_y )
{
    HWND parent;
    WND *wndPtr = WIN_GetPtr( winpos->hwnd );
    BOOL ret = TRUE;

    if (!wndPtr || wndPtr == WND_OTHER_PROCESS)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    winpos->hwnd = wndPtr->obj.handle;  /* make it a full handle */

    /* Finally make sure that all coordinates are valid */
    if (winpos->x < -32768) winpos->x = -32768;
    else if (winpos->x > 32767) winpos->x = 32767;
    if (winpos->y < -32768) winpos->y = -32768;
    else if (winpos->y > 32767) winpos->y = 32767;

    if (winpos->cx < 0) winpos->cx = 0;
    else if (winpos->cx > 32767) winpos->cx = 32767;
    if (winpos->cy < 0) winpos->cy = 0;
    else if (winpos->cy > 32767) winpos->cy = 32767;

    parent = GetAncestor( winpos->hwnd, GA_PARENT );
    if (!IsWindowVisible( parent )) winpos->flags |= SWP_NOREDRAW;

    if (wndPtr->dwStyle & WS_VISIBLE) winpos->flags &= ~SWP_SHOWWINDOW;
    else
    {
        winpos->flags &= ~SWP_HIDEWINDOW;
        if (!(winpos->flags & SWP_SHOWWINDOW)) winpos->flags |= SWP_NOREDRAW;
    }

    if ((old_window_rect->right - old_window_rect->left == winpos->cx) &&
        (old_window_rect->bottom - old_window_rect->top == winpos->cy))
        winpos->flags |= SWP_NOSIZE;    /* Already the right size */

    if ((old_window_rect->left - parent_x == winpos->x) && (old_window_rect->top - parent_y == winpos->y))
        winpos->flags |= SWP_NOMOVE;    /* Already the right position */

    if ((wndPtr->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
        if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW)) && /* Bring to the top when activating */
            (winpos->flags & SWP_NOZORDER ||
             (winpos->hwndInsertAfter != HWND_TOPMOST && winpos->hwndInsertAfter != HWND_NOTOPMOST)))
        {
            winpos->flags &= ~SWP_NOZORDER;
            winpos->hwndInsertAfter = HWND_TOP;
        }
    }

    /* Check hwndInsertAfter */
    if (winpos->flags & SWP_NOZORDER) goto done;

    if (winpos->hwndInsertAfter == HWND_TOP)
    {
        if (GetWindow(winpos->hwnd, GW_HWNDFIRST) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_BOTTOM)
    {
        if (!(wndPtr->dwExStyle & WS_EX_TOPMOST) && GetWindow(winpos->hwnd, GW_HWNDLAST) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_TOPMOST)
    {
        if ((wndPtr->dwExStyle & WS_EX_TOPMOST) && GetWindow(winpos->hwnd, GW_HWNDFIRST) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_NOTOPMOST)
    {
        if (!(wndPtr->dwExStyle & WS_EX_TOPMOST))
            winpos->flags |= SWP_NOZORDER;
    }
    else
    {
        if ((winpos->hwnd == winpos->hwndInsertAfter) ||
            (winpos->hwnd == GetWindow( winpos->hwndInsertAfter, GW_HWNDNEXT )))
            winpos->flags |= SWP_NOZORDER;
    }
 done:
    WIN_ReleasePtr( wndPtr );
    return ret;
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
    HWND surface_win = 0, parent = GetAncestor( hwnd, GA_PARENT );
    BOOL ret, needs_update = FALSE;
    RECT visible_rect, old_visible_rect, old_window_rect, old_client_rect, extra_rects[3];
    struct window_surface *old_surface, *new_surface = NULL;

    if (!parent || parent == GetDesktopWindow())
    {
        new_surface = &dummy_surface;  /* provide a default surface for top-level windows */
        window_surface_add_ref( new_surface );
    }
    visible_rect = *window_rect;
    USER_Driver->pWindowPosChanging( hwnd, insert_after, swp_flags,
                                     window_rect, client_rect, &visible_rect, &new_surface );

    WIN_GetRectangles( hwnd, COORDS_SCREEN, &old_window_rect, NULL );
    if (IsRectEmpty( &valid_rects[0] )) valid_rects = NULL;

    if (!(win = WIN_GetPtr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        if (new_surface) window_surface_release( new_surface );
        return FALSE;
    }
    old_visible_rect = win->visible_rect;
    old_client_rect = win->client_rect;
    old_surface = win->surface;
    if (old_surface != new_surface) swp_flags |= SWP_FRAMECHANGED;  /* force refreshing non-client area */
    if (new_surface == &dummy_surface) swp_flags |= SWP_NOREDRAW;
    else if (old_surface == &dummy_surface)
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
 *		USER_SetWindowPos
 *
 *     User32 internal function
 */
BOOL USER_SetWindowPos( WINDOWPOS * winpos, int parent_x, int parent_y )
{
    RECT old_window_rect, old_client_rect, new_window_rect, new_client_rect, valid_rects[2];
    UINT orig_flags;
    BOOL ret = FALSE;
    DPI_AWARENESS_CONTEXT context;

    orig_flags = winpos->flags;

    /* First, check z-order arguments.  */
    if (!(winpos->flags & SWP_NOZORDER))
    {
        /* fix sign extension */
        if (winpos->hwndInsertAfter == (HWND)0xffff) winpos->hwndInsertAfter = HWND_TOPMOST;
        else if (winpos->hwndInsertAfter == (HWND)0xfffe) winpos->hwndInsertAfter = HWND_NOTOPMOST;

        if (!(winpos->hwndInsertAfter == HWND_TOP ||
              winpos->hwndInsertAfter == HWND_BOTTOM ||
              winpos->hwndInsertAfter == HWND_TOPMOST ||
              winpos->hwndInsertAfter == HWND_NOTOPMOST))
        {
            HWND parent = GetAncestor( winpos->hwnd, GA_PARENT );
            HWND insertafter_parent = GetAncestor( winpos->hwndInsertAfter, GA_PARENT );

            /* hwndInsertAfter must be a sibling of the window */
            if (!insertafter_parent) return FALSE;
            if (insertafter_parent != parent) return TRUE;
        }
    }

    /* Make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
    if (!(winpos->flags & SWP_NOMOVE))
    {
        if (winpos->x < -32768) winpos->x = -32768;
        else if (winpos->x > 32767) winpos->x = 32767;
        if (winpos->y < -32768) winpos->y = -32768;
        else if (winpos->y > 32767) winpos->y = 32767;
    }
    if (!(winpos->flags & SWP_NOSIZE))
    {
        if (winpos->cx < 0) winpos->cx = 0;
        else if (winpos->cx > 32767) winpos->cx = 32767;
        if (winpos->cy < 0) winpos->cy = 0;
        else if (winpos->cy > 32767) winpos->cy = 32767;
    }

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( winpos->hwnd ));

    if (!SWP_DoWinPosChanging( winpos, &old_window_rect, &old_client_rect,
                               &new_window_rect, &new_client_rect )) goto done;

    /* Fix redundant flags */
    if (!fixup_flags( winpos, &old_window_rect, parent_x, parent_y )) goto done;

    if((winpos->flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) != SWP_NOZORDER)
    {
        if (GetAncestor( winpos->hwnd, GA_PARENT ) == GetDesktopWindow())
            winpos->hwndInsertAfter = SWP_DoOwnedPopups( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* Common operations */

    SWP_DoNCCalcSize( winpos, &old_window_rect, &old_client_rect,
                      &new_window_rect, &new_client_rect, valid_rects, parent_x, parent_y );

    if (!set_window_pos( winpos->hwnd, winpos->hwndInsertAfter, winpos->flags,
                         &new_window_rect, &new_client_rect, valid_rects ))
        goto done;


    if( winpos->flags & SWP_HIDEWINDOW )
        HideCaret(winpos->hwnd);
    else if (winpos->flags & SWP_SHOWWINDOW)
        ShowCaret(winpos->hwnd);

    if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW)))
    {
        /* child windows get WM_CHILDACTIVATE message */
        if ((GetWindowLongW( winpos->hwnd, GWL_STYLE ) & (WS_CHILD | WS_POPUP)) == WS_CHILD)
            SendMessageW( winpos->hwnd, WM_CHILDACTIVATE, 0, 0 );
        else
            SetForegroundWindow( winpos->hwnd );
    }

    if(!(orig_flags & SWP_DEFERERASE))
    {
        /* erase parent when hiding or resizing child */
        if ((orig_flags & SWP_HIDEWINDOW) ||
         (!(orig_flags & SWP_SHOWWINDOW) &&
          (winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOGEOMETRYCHANGE))
        {
            HWND parent = GetAncestor( winpos->hwnd, GA_PARENT );
            if (!parent || parent == GetDesktopWindow()) parent = winpos->hwnd;
            erase_now( parent, 0 );
        }

        /* Give newly shown windows a chance to redraw */
        if(((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
                && !(orig_flags & SWP_AGG_NOCLIENTCHANGE) && (orig_flags & SWP_SHOWWINDOW))
        {
            erase_now(winpos->hwnd, 0);
        }
    }

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE("\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS);

    if (((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
            && !((orig_flags & SWP_AGG_NOCLIENTCHANGE) && (orig_flags & SWP_SHOWWINDOW)))
    {
        /* WM_WINDOWPOSCHANGED is sent even if SWP_NOSENDCHANGING is set
           and always contains final window position.
         */
        winpos->x  = new_window_rect.left;
        winpos->y  = new_window_rect.top;
        winpos->cx = new_window_rect.right - new_window_rect.left;
        winpos->cy = new_window_rect.bottom - new_window_rect.top;
        SendMessageW( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );
    }
    ret = TRUE;
done:
    SetThreadDpiAwarenessContext( context );
    return ret;
}

/***********************************************************************
 *		SetWindowPos (USER32.@)
 */
BOOL WINAPI SetWindowPos( HWND hwnd, HWND hwndInsertAfter,
                          INT x, INT y, INT cx, INT cy, UINT flags )
{
    WINDOWPOS winpos;

    TRACE("hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
          hwnd, hwndInsertAfter, x, y, cx, cy, flags);
    if(TRACE_ON(win)) dump_winpos_flags(flags);

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    winpos.hwnd = WIN_GetFullHandle(hwnd);
    winpos.hwndInsertAfter = WIN_GetFullHandle(hwndInsertAfter);
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    winpos.flags = flags;

    map_dpi_winpos( &winpos );

    if (WIN_IsCurrentThread( hwnd ))
        return USER_SetWindowPos( &winpos, 0, 0 );

    return SendMessageW( winpos.hwnd, WM_WINE_SETWINDOWPOS, 0, (LPARAM)&winpos );
}


/***********************************************************************
 *		BeginDeferWindowPos (USER32.@)
 */
HDWP WINAPI BeginDeferWindowPos( INT count )
{
    HDWP handle = 0;
    DWP *pDWP;

    TRACE("%d\n", count);

    if (count < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    /* Windows allows zero count, in which case it allocates context for 8 moves */
    if (count == 0) count = 8;

    if (!(pDWP = HeapAlloc( GetProcessHeap(), 0, sizeof(DWP)))) return 0;

    pDWP->actualCount    = 0;
    pDWP->suggestedCount = count;
    pDWP->hwndParent     = 0;

    if (!(pDWP->winPos = HeapAlloc( GetProcessHeap(), 0, count * sizeof(WINDOWPOS) )) ||
        !(handle = alloc_user_handle( &pDWP->obj, USER_DWP )))
    {
        HeapFree( GetProcessHeap(), 0, pDWP->winPos );
        HeapFree( GetProcessHeap(), 0, pDWP );
    }

    TRACE("returning hdwp %p\n", handle);
    return handle;
}


/***********************************************************************
 *		DeferWindowPos (USER32.@)
 */
HDWP WINAPI DeferWindowPos( HDWP hdwp, HWND hwnd, HWND hwndAfter,
                                INT x, INT y, INT cx, INT cy,
                                UINT flags )
{
    DWP *pDWP;
    int i;
    HDWP retvalue = hdwp;
    WINDOWPOS winpos;

    TRACE("hdwp %p, hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
          hdwp, hwnd, hwndAfter, x, y, cx, cy, flags);

    winpos.hwnd = WIN_GetFullHandle( hwnd );
    if (is_desktop_window( winpos.hwnd ) || !IsWindow( winpos.hwnd ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    winpos.hwndInsertAfter = WIN_GetFullHandle(hwndAfter);
    winpos.flags = flags;
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    map_dpi_winpos( &winpos );

    if (!(pDWP = get_user_handle_ptr( hdwp, USER_DWP ))) return 0;
    if (pDWP == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p?\n", hdwp );
        return 0;
    }

    for (i = 0; i < pDWP->actualCount; i++)
    {
        if (pDWP->winPos[i].hwnd == winpos.hwnd)
        {
              /* Merge with the other changes */
            if (!(flags & SWP_NOZORDER))
            {
                pDWP->winPos[i].hwndInsertAfter = winpos.hwndInsertAfter;
            }
            if (!(flags & SWP_NOMOVE))
            {
                pDWP->winPos[i].x = winpos.x;
                pDWP->winPos[i].y = winpos.y;
            }
            if (!(flags & SWP_NOSIZE))
            {
                pDWP->winPos[i].cx = winpos.cx;
                pDWP->winPos[i].cy = winpos.cy;
            }
            pDWP->winPos[i].flags &= flags | ~(SWP_NOSIZE | SWP_NOMOVE |
                                               SWP_NOZORDER | SWP_NOREDRAW |
                                               SWP_NOACTIVATE | SWP_NOCOPYBITS|
                                               SWP_NOOWNERZORDER);
            pDWP->winPos[i].flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                              SWP_FRAMECHANGED);
            goto END;
        }
    }
    if (pDWP->actualCount >= pDWP->suggestedCount)
    {
        WINDOWPOS *newpos = HeapReAlloc( GetProcessHeap(), 0, pDWP->winPos,
                                         pDWP->suggestedCount * 2 * sizeof(WINDOWPOS) );
        if (!newpos)
        {
            retvalue = 0;
            goto END;
        }
        pDWP->suggestedCount *= 2;
        pDWP->winPos = newpos;
    }
    pDWP->winPos[pDWP->actualCount++] = winpos;
END:
    release_user_handle_ptr( pDWP );
    return retvalue;
}


/***********************************************************************
 *		EndDeferWindowPos (USER32.@)
 */
BOOL WINAPI EndDeferWindowPos( HDWP hdwp )
{
    DWP *pDWP;
    WINDOWPOS *winpos;
    int i;

    TRACE("%p\n", hdwp);

    if (!(pDWP = free_user_handle( hdwp, USER_DWP ))) return FALSE;
    if (pDWP == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p?\n", hdwp );
        return FALSE;
    }

    for (i = 0, winpos = pDWP->winPos; i < pDWP->actualCount; i++, winpos++)
    {
        TRACE("hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
               winpos->hwnd, winpos->hwndInsertAfter, winpos->x, winpos->y,
               winpos->cx, winpos->cy, winpos->flags);

        if (WIN_IsCurrentThread( winpos->hwnd ))
            USER_SetWindowPos( winpos, 0, 0 );
        else
            SendMessageW( winpos->hwnd, WM_WINE_SETWINDOWPOS, 0, (LPARAM)winpos );
    }
    HeapFree( GetProcessHeap(), 0, pDWP->winPos );
    HeapFree( GetProcessHeap(), 0, pDWP );
    return TRUE;
}


/***********************************************************************
 *		ArrangeIconicWindows (USER32.@)
 */
UINT WINAPI ArrangeIconicWindows( HWND parent )
{
    RECT rectParent;
    HWND hwndChild;
    INT x, y, xspacing, yspacing;
    POINT pt;
    MINIMIZEDMETRICS metrics;

    metrics.cbSize = sizeof(metrics);
    SystemParametersInfoW( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );

    if (parent == GetDesktopWindow())
    {
        MONITORINFO mon_info;
        HMONITOR monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTOPRIMARY );

        mon_info.cbSize = sizeof( mon_info );
        GetMonitorInfoW( monitor, &mon_info );
        rectParent = mon_info.rcWork;
    }
    else GetClientRect( parent, &rectParent );

    x = y = 0;
    xspacing = GetSystemMetrics(SM_CXICONSPACING);
    yspacing = GetSystemMetrics(SM_CYICONSPACING);

    hwndChild = GetWindow( parent, GW_CHILD );
    while (hwndChild)
    {
        if( IsIconic( hwndChild ) )
        {
            WINPOS_ShowIconTitle( hwndChild, FALSE );

            if (metrics.iArrange & ARW_STARTRIGHT)
                pt.x = rectParent.right - (x + 1) * xspacing;
            else
                pt.x = rectParent.left + x * xspacing;
            if (metrics.iArrange & ARW_STARTTOP)
                pt.y = rectParent.top + y * yspacing;
            else
                pt.y = rectParent.bottom - (y + 1) * yspacing;

            SetWindowPos( hwndChild, 0, pt.x + (xspacing - GetSystemMetrics(SM_CXICON)) / 2,
                          pt.y + (yspacing - GetSystemMetrics(SM_CYICON)) / 2, 0, 0,
                          SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
	    if( IsWindow(hwndChild) )
                WINPOS_ShowIconTitle(hwndChild , TRUE );

            if (++x >= (rectParent.right - rectParent.left) / xspacing)
            {
                x = 0;
                y++;
            }
        }
        hwndChild = GetWindow( hwndChild, GW_HWNDNEXT );
    }
    return yspacing;
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
        SetCursor( LoadCursorW( 0, (LPWSTR)IDC_SIZEALL ) );
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
    SetCursorPos( pt.x, pt.y );
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
    HCURSOR hDragCursor = 0, hOldCursor = 0;
    MINMAXINFO minmax;
    POINT capturePoint, pt;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL    thickframe = HAS_THICKFRAME( style );
    BOOL    iconic = style & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = TRUE;
    HMONITOR mon = 0;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd)) return;

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;
    ClipCursor( NULL );

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
    hdc = GetDCEx( parent, 0, DCX_CACHE );

    if( iconic ) /* create a cursor for dragging */
    {
        hDragCursor = (HCURSOR)GetClassLongPtrW( hwnd, GCLP_HICON);
        if( !hDragCursor ) hDragCursor = (HCURSOR)SendMessageW( hwnd, WM_QUERYDRAGICON, 0, 0L);
        if( !hDragCursor ) iconic = FALSE;
    }

    /* we only allow disabling the full window drag for child windows */
    if (parent) SystemParametersInfoW( SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0 );

    /* repaint the window before moving it around */
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

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

                if( iconic ) /* ok, no system popup tracking */
                {
                    hOldCursor = SetCursor(hDragCursor);
                    ShowCursor( TRUE );
                    WINPOS_ShowIconTitle( hwnd, FALSE );
                }
                else if(!DragFullWindows)
                    draw_moving_frame( parent, hdc, &sizingRect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
            else
            {
                if(!iconic && !DragFullWindows) draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                if (hittest == HTCAPTION) OffsetRect( &sizingRect, dx, dy );
                if (ON_LEFT_BORDER(hittest)) sizingRect.left += dx;
                else if (ON_RIGHT_BORDER(hittest)) sizingRect.right += dx;
                if (ON_TOP_BORDER(hittest)) sizingRect.top += dy;
                else if (ON_BOTTOM_BORDER(hittest)) sizingRect.bottom += dy;
                capturePoint = pt;

                /* determine the hit location */
                if (syscommand == SC_SIZE)
                {
                    WPARAM wpSizingHit = 0;

                    if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                        wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                    SendMessageW( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&sizingRect );
                }
                else
                    SendMessageW( hwnd, WM_MOVING, 0, (LPARAM)&sizingRect );

                if (!iconic)
                {
                    if(!DragFullWindows)
                        draw_moving_frame( parent, hdc, &sizingRect, thickframe );
                    else
                    {
                        RECT rect = sizingRect;
                        MapWindowPoints( 0, parent, (POINT *)&rect, 2 );
                        SetWindowPos( hwnd, 0, rect.left, rect.top,
                                      rect.right - rect.left, rect.bottom - rect.top,
                                      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
                    }
                }
            }
        }
    }

    if( iconic )
    {
        if( moved ) /* restore cursors, show icon title later on */
        {
            ShowCursor( FALSE );
            SetCursor( hOldCursor );
        }
    }
    else if (moved && !DragFullWindows)
    {
        draw_moving_frame( parent, hdc, &sizingRect, thickframe );
    }

    set_capture_window( 0, GUI_INMOVESIZE, NULL );
    ReleaseDC( parent, hdc );
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
            if(!DragFullWindows || iconic)
                SetWindowPos( hwnd, 0, sizingRect.left, sizingRect.top,
                              sizingRect.right - sizingRect.left,
                              sizingRect.bottom - sizingRect.top,
                              ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
        else
        { /* restore previous size/position */
            if(DragFullWindows)
                SetWindowPos( hwnd, 0, origRect.left, origRect.top,
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
        else WINPOS_ShowIconTitle( hwnd, TRUE );
    }
}
