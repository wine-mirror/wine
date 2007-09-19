/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995, 2001 Alexandre Julliard
 * Copyright 1995, 1996, 1999 Alex Korobka
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef HAVE_LIBXSHAPE
#include <X11/extensions/shape.h>
#endif /* HAVE_LIBXSHAPE */
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/wingdi16.h"

#include "x11drv.h"
#include "win.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

#define SWP_AGG_NOPOSCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

#define SWP_EX_NOCOPY       0x0001
#define SWP_EX_PAINTSELF    0x0002
#define SWP_EX_NONCLIENT    0x0004

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

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */

#define _NET_WM_STATE_REMOVE  0
#define _NET_WM_STATE_ADD     1
#define _NET_WM_STATE_TOGGLE  2

static const char managed_prop[] = "__wine_x11_managed";

/***********************************************************************
 *           X11DRV_Expose
 */
void X11DRV_Expose( HWND hwnd, XEvent *xev )
{
    XExposeEvent *event = &xev->xexpose;
    RECT rect;
    struct x11drv_win_data *data;
    int flags = RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN;

    TRACE( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->window, event->x, event->y, event->width, event->height );

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    rect.left   = event->x;
    rect.top    = event->y;
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    if (event->window == root_window)
        OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );

    if (rect.left < data->client_rect.left ||
        rect.top < data->client_rect.top ||
        rect.right > data->client_rect.right ||
        rect.bottom > data->client_rect.bottom) flags |= RDW_FRAME;

    SERVER_START_REQ( update_window_zorder )
    {
        req->window      = hwnd;
        req->rect.left   = rect.left + data->whole_rect.left;
        req->rect.top    = rect.top + data->whole_rect.top;
        req->rect.right  = rect.right + data->whole_rect.left;
        req->rect.bottom = rect.bottom + data->whole_rect.top;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* make position relative to client area instead of window */
    OffsetRect( &rect, -data->client_rect.left, -data->client_rect.top );
    RedrawWindow( hwnd, &rect, 0, flags );
}

/***********************************************************************
 *		SetWindowStyle   (X11DRV.@)
 *
 * Update the X state of a window to reflect a style change
 */
void X11DRV_SetWindowStyle( HWND hwnd, DWORD old_style )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    DWORD new_style, changed;

    if (hwnd == GetDesktopWindow()) return;
    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    new_style = GetWindowLongW( hwnd, GWL_STYLE );
    changed = new_style ^ old_style;

    if (changed & WS_VISIBLE)
    {
        if (data->whole_window && X11DRV_is_window_rect_mapped( &data->window_rect ))
        {
            if (new_style & WS_VISIBLE)
            {
                TRACE( "mapping win %p\n", hwnd );
                X11DRV_sync_window_style( display, data );
                X11DRV_set_wm_hints( display, data );
                wine_tsx11_lock();
                XMapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
            /* we don't unmap windows, that causes trouble with the window manager */
        }
        invalidate_dce( hwnd, &data->window_rect );
    }

    if (changed & WS_DISABLED)
    {
        if (data->whole_window && data->wm_hints)
        {
            wine_tsx11_lock();
            data->wm_hints->input = !(new_style & WS_DISABLED);
            XSetWMHints( display, data->whole_window, data->wm_hints );
            wine_tsx11_unlock();
        }
    }
}


/***********************************************************************
 *     update_fullscreen_state
 *
 * Use the NETWM protocol to set the fullscreen state.
 * This only works for mapped windows.
 */
static void update_fullscreen_state( Display *display, Window win, BOOL new_fs_state )
{
    XEvent xev;

    TRACE("setting fullscreen state for window %lx to %s\n", win, new_fs_state ? "true" : "false");

    xev.xclient.type = ClientMessage;
    xev.xclient.window = win;
    xev.xclient.message_type = x11drv_atom(_NET_WM_STATE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = new_fs_state ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = x11drv_atom(_NET_WM_STATE_FULLSCREEN);
    xev.xclient.data.l[2] = 0;
    wine_tsx11_lock();
    XSendEvent(display, root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    wine_tsx11_unlock();
}

/***********************************************************************
 *     fullscreen_state_changed
 *
 * Check if the fullscreen state of a given window has changed
 */
static BOOL fullscreen_state_changed( const struct x11drv_win_data *data,
                                      const RECT *old_client_rect, const RECT *old_screen_rect,
                                      BOOL *new_fs_state )
{
    BOOL old_fs_state = FALSE;

    *new_fs_state = FALSE;

    if (old_client_rect->left <= 0 && old_client_rect->right >= old_screen_rect->right &&
        old_client_rect->top <= 0 && old_client_rect->bottom >= old_screen_rect->bottom)
        old_fs_state = TRUE;

    if (data->client_rect.left <= 0 && data->client_rect.right >= screen_width &&
        data->client_rect.top <= 0 && data->client_rect.bottom >= screen_height)
        *new_fs_state = TRUE;

#if 0 /* useful to debug fullscreen state problems */
    TRACE("hwnd %p, old rect %s, new rect %s, old screen %s, new screen (0,0-%d,%d)\n",
           data->hwnd,
           wine_dbgstr_rect(old_client_rect), wine_dbgstr_rect(&data->client_rect),
           wine_dbgstr_rect(old_screen_rect), screen_width, screen_height);
    TRACE("old fs state %d\n, new fs state = %d\n", old_fs_state, *new_fs_state);
#endif
    return *new_fs_state != old_fs_state;
}


/***********************************************************************
 *		SetWindowPos   (X11DRV.@)
 */
BOOL X11DRV_SetWindowPos( HWND hwnd, HWND insert_after, const RECT *rectWindow,
                                   const RECT *rectClient, UINT swp_flags, const RECT *valid_rects )
{
    Display *display = thread_display();
    struct x11drv_win_data *data;
    RECT new_whole_rect, old_client_rect, old_screen_rect;
    WND *win;
    DWORD old_style, new_style;
    BOOL ret, make_managed = FALSE;

    if (!(data = X11DRV_get_win_data( hwnd ))) return FALSE;

    /* check if we need to switch the window to managed */
    if (!data->managed && data->whole_window && managed_mode &&
        root_window == DefaultRootWindow( display ) &&
        data->whole_window != root_window)
    {
        if (is_window_managed( hwnd, swp_flags, rectWindow ))
        {
            TRACE( "making win %p/%lx managed\n", hwnd, data->whole_window );
            make_managed = TRUE;
            data->managed = TRUE;
            SetPropA( hwnd, managed_prop, (HANDLE)1 );
        }
    }

    new_whole_rect = *rectWindow;
    X11DRV_window_to_X_rect( data, &new_whole_rect );

    old_client_rect = data->client_rect;

    if (!(win = WIN_GetPtr( hwnd ))) return FALSE;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) ERR( "cannot set rectangles of other process window %p\n", hwnd );
        return FALSE;
    }
    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = hwnd;
        req->previous      = insert_after;
        req->flags         = swp_flags;
        req->window.left   = rectWindow->left;
        req->window.top    = rectWindow->top;
        req->window.right  = rectWindow->right;
        req->window.bottom = rectWindow->bottom;
        req->client.left   = rectClient->left;
        req->client.top    = rectClient->top;
        req->client.right  = rectClient->right;
        req->client.bottom = rectClient->bottom;
        if (memcmp( rectWindow, &new_whole_rect, sizeof(RECT) ) || !IsRectEmpty( &valid_rects[0] ))
        {
            wine_server_add_data( req, &new_whole_rect, sizeof(new_whole_rect) );
            if (!IsRectEmpty( &valid_rects[0] ))
                wine_server_add_data( req, valid_rects, 2 * sizeof(*valid_rects) );
        }
        ret = !wine_server_call( req );
        new_style = reply->new_style;
    }
    SERVER_END_REQ;

    if (win == WND_DESKTOP || data->whole_window == DefaultRootWindow(gdi_display))
    {
        data->whole_rect = data->client_rect = data->window_rect = *rectWindow;
        if (win != WND_DESKTOP)
        {
            win->rectWindow   = *rectWindow;
            win->rectClient   = *rectClient;
            win->dwStyle      = new_style;
            WIN_ReleasePtr( win );
        }
        return ret;
    }

    if (ret)
    {
        /* invalidate DCEs */

        if ((((swp_flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) && (new_style & WS_VISIBLE)) ||
             (swp_flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)))
        {
            RECT rect;
            UnionRect( &rect, rectWindow, &win->rectWindow );
            invalidate_dce( hwnd, &rect );
        }

        win->rectWindow   = *rectWindow;
        win->rectClient   = *rectClient;
        old_style         = win->dwStyle;
        win->dwStyle      = new_style;
        data->window_rect = *rectWindow;

        TRACE( "win %p window %s client %s style %08x\n",
               hwnd, wine_dbgstr_rect(rectWindow), wine_dbgstr_rect(rectClient), new_style );

        if (make_managed && (old_style & WS_VISIBLE))
        {
            wine_tsx11_lock();
            XUnmapWindow( display, data->whole_window );
            wine_tsx11_unlock();
            old_style &= ~WS_VISIBLE;  /* force it to be mapped again below */
        }

        if (!IsRectEmpty( &valid_rects[0] ))
        {
            int x_offset = 0, y_offset = 0;

            if (data->whole_window)
            {
                /* the X server will move the bits for us */
                x_offset = data->whole_rect.left - new_whole_rect.left;
                y_offset = data->whole_rect.top - new_whole_rect.top;
            }

            if (x_offset != valid_rects[1].left - valid_rects[0].left ||
                y_offset != valid_rects[1].top - valid_rects[0].top)
            {
                /* FIXME: should copy the window bits here */
                RECT invalid_rect = valid_rects[0];

                /* invalid_rects are relative to the client area */
                OffsetRect( &invalid_rect, -rectClient->left, -rectClient->top );
                RedrawWindow( hwnd, &invalid_rect, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
            }
        }


        if (data->whole_window && !data->lock_changes)
        {
            if ((old_style & WS_VISIBLE) && !(new_style & WS_VISIBLE))
            {
                /* window got hidden, unmap it */
                TRACE( "unmapping win %p\n", hwnd );
                wine_tsx11_lock();
                XUnmapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
            else if ((new_style & WS_VISIBLE) && !X11DRV_is_window_rect_mapped( rectWindow ))
            {
                /* resizing to zero size or off screen -> unmap */
                TRACE( "unmapping zero size or off-screen win %p\n", hwnd );
                wine_tsx11_lock();
                XUnmapWindow( display, data->whole_window );
                wine_tsx11_unlock();
            }
        }

        X11DRV_sync_window_position( display, data, swp_flags, rectClient, &new_whole_rect );

        if (data->whole_window && !data->lock_changes)
        {
            BOOL new_fs_state, mapped = FALSE;

            if ((new_style & WS_VISIBLE) && !(new_style & WS_MINIMIZE) &&
                X11DRV_is_window_rect_mapped( rectWindow ))
            {
                if (!(old_style & WS_VISIBLE))
                {
                    /* window got shown, map it */
                    TRACE( "mapping win %p\n", hwnd );
                    mapped = TRUE;
                }
                else if ((swp_flags & (SWP_NOSIZE | SWP_NOMOVE)) != (SWP_NOSIZE | SWP_NOMOVE))
                {
                    /* resizing from zero size to non-zero -> map */
                    TRACE( "mapping non zero size or off-screen win %p\n", hwnd );
                    mapped = TRUE;
                }

                if (mapped || (swp_flags & SWP_FRAMECHANGED))
                    X11DRV_set_wm_hints( display, data );

                if (mapped)
                {
                    X11DRV_sync_window_style( display, data );
                    wine_tsx11_lock();
                    XMapWindow( display, data->whole_window );
                    XFlush( display );
                    wine_tsx11_unlock();
                }
                SetRect( &old_screen_rect, 0, 0, screen_width, screen_height );
                if (fullscreen_state_changed( data, &old_client_rect, &old_screen_rect, &new_fs_state ) || mapped)
                    update_fullscreen_state( display, data->whole_window, new_fs_state );
            }
        }
    }
    WIN_ReleasePtr( win );
    return ret;
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
    int xspacing, yspacing;

    parent = GetAncestor( hwnd, GA_PARENT );
    GetClientRect( parent, &rectParent );
    if ((pt.x >= rectParent.left) && (pt.x + GetSystemMetrics(SM_CXICON) < rectParent.right) &&
        (pt.y >= rectParent.top) && (pt.y + GetSystemMetrics(SM_CYICON) < rectParent.bottom))
        return pt;  /* The icon already has a suitable position */

    xspacing = GetSystemMetrics(SM_CXICONSPACING);
    yspacing = GetSystemMetrics(SM_CYICONSPACING);

    /* Check if another icon already occupies this spot */
    /* FIXME: this is completely inefficient */

    hrgn = CreateRectRgn( 0, 0, 0, 0 );
    tmp = CreateRectRgn( 0, 0, 0, 0 );
    for (child = GetWindow( parent, GW_HWNDFIRST ); child; child = GetWindow( child, GW_HWNDNEXT ))
    {
        WND *childPtr;
        if (child == hwnd) continue;
        if ((GetWindowLongW( child, GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != (WS_VISIBLE|WS_MINIMIZE))
            continue;
        if (!(childPtr = WIN_GetPtr( child )) || childPtr == WND_OTHER_PROCESS)
            continue;
        SetRectRgn( tmp, childPtr->rectWindow.left, childPtr->rectWindow.top,
                    childPtr->rectWindow.right, childPtr->rectWindow.bottom );
        CombineRgn( hrgn, hrgn, tmp, RGN_OR );
        WIN_ReleasePtr( childPtr );
    }
    DeleteObject( tmp );

    for (rect.bottom = rectParent.bottom; rect.bottom >= yspacing; rect.bottom -= yspacing)
    {
        for (rect.left = rectParent.left; rect.left <= rectParent.right - xspacing; rect.left += xspacing)
        {
            rect.right = rect.left + xspacing;
            rect.top = rect.bottom - yspacing;
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
    WND *wndPtr;
    UINT swpFlags = 0;
    POINT size;
    LONG old_style;
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
        if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
        if( wndPtr->dwStyle & WS_MAXIMIZE) wndPtr->flags |= WIN_RESTORE_MAX;
        else wndPtr->flags &= ~WIN_RESTORE_MAX;
        WIN_ReleasePtr( wndPtr );

        old_style = WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );

        X11DRV_set_iconic_state( hwnd );

        wpl.ptMinPosition = WINPOS_FindIconPos( hwnd, wpl.ptMinPosition );

        if (!(old_style & WS_MINIMIZE)) swpFlags |= SWP_STATECHANGED;
        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON) );
        swpFlags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        old_style = GetWindowLongW( hwnd, GWL_STYLE );
        if ((old_style & WS_MAXIMIZE) && (old_style & WS_VISIBLE)) return SWP_NOSIZE | SWP_NOMOVE;

        WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL );

        old_style = WIN_SetStyle( hwnd, WS_MAXIMIZE, WS_MINIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( hwnd );
        }
        if (!(old_style & WS_MAXIMIZE)) swpFlags |= SWP_STATECHANGED;
        SetRect( rect, wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, size.x, size.y );
        break;

    case SW_SHOWNOACTIVATE:
    case SW_SHOWNORMAL:
    case SW_RESTORE:
        old_style = WIN_SetStyle( hwnd, 0, WS_MINIMIZE | WS_MAXIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            BOOL restore_max;

            WINPOS_ShowIconTitle( hwnd, FALSE );
            X11DRV_set_iconic_state( hwnd );

            if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
            restore_max = (wndPtr->flags & WIN_RESTORE_MAX) != 0;
            WIN_ReleasePtr( wndPtr );
            if (restore_max)
            {
                /* Restore to maximized position */
                WINPOS_GetMinMaxInfo( hwnd, &size, &wpl.ptMaxPosition, NULL, NULL);
                WIN_SetStyle( hwnd, WS_MAXIMIZE, 0 );
                swpFlags |= SWP_STATECHANGED;
                SetRect( rect, wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, size.x, size.y );
                break;
            }
        }
        else if (!(old_style & WS_MAXIMIZE)) break;

        swpFlags |= SWP_STATECHANGED;

        /* Restore to normal position */

        *rect = wpl.rcNormalPosition;
        rect->right -= rect->left;
        rect->bottom -= rect->top;

        break;
    }

    return swpFlags;
}


/***********************************************************************
 *              ShowWindow   (X11DRV.@)
 */
BOOL X11DRV_ShowWindow( HWND hwnd, INT cmd )
{
    WND *wndPtr;
    HWND parent;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    BOOL wasVisible = (style & WS_VISIBLE) != 0;
    BOOL showFlag = TRUE;
    RECT newPos = {0, 0, 0, 0};
    UINT swp = 0;

    TRACE("hwnd=%p, cmd=%d, wasVisible %d\n", hwnd, cmd, wasVisible);

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
            showFlag = FALSE;
            swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (hwnd != GetActiveWindow())
                swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
        case SW_MINIMIZE:
        case SW_FORCEMINIMIZE: /* FIXME: Does not work if thread is hung. */
            if (style & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            swp |= WINPOS_MinMaximize( hwnd, cmd, &newPos );
            if ((style & WS_MINIMIZE) && wasVisible) return TRUE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            if (!wasVisible) swp |= SWP_SHOWWINDOW;
            swp |= SWP_FRAMECHANGED;
            swp |= WINPOS_MinMaximize( hwnd, SW_MAXIMIZE, &newPos );
            if ((style & WS_MAXIMIZE) && wasVisible) return TRUE;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (style & WS_CHILD) swp |= SWP_NOZORDER;
            break;
	case SW_SHOW:
            if (wasVisible) return TRUE;
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
                if (wasVisible) return TRUE;
                swp |= SWP_NOSIZE | SWP_NOMOVE;
            }
            if (style & WS_CHILD && !(swp & SWP_STATECHANGED)) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	    break;
    }

    if ((showFlag != wasVisible || cmd == SW_SHOWNA) && cmd != SW_SHOWMAXIMIZED && !(swp & SWP_STATECHANGED))
    {
        SendMessageW( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) return wasVisible;
    }

    parent = GetAncestor( hwnd, GA_PARENT );
    if (parent && !IsWindowVisible( parent ) && !(swp & SWP_STATECHANGED))
    {
        /* if parent is not visible simply toggle WS_VISIBLE and return */
        if (showFlag) WIN_SetStyle( hwnd, WS_VISIBLE, 0 );
        else WIN_SetStyle( hwnd, 0, WS_VISIBLE );
    }
    else
        SetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                      newPos.right, newPos.bottom, LOWORD(swp) );

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
        if (hwnd == hFocus || IsChild(hwnd, hFocus))
        {
            HWND parent = GetAncestor(hwnd, GA_PARENT);
            if (parent == GetDesktopWindow()) parent = 0;
            SetFocus(parent);
        }
    }

    if (IsIconic(hwnd)) WINPOS_ShowIconTitle( hwnd, TRUE );

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return wasVisible;

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;
        RECT client = wndPtr->rectClient;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) wParam = SIZE_MINIMIZED;
        WIN_ReleasePtr( wndPtr );

        SendMessageW( hwnd, WM_SIZE, wParam,
                      MAKELONG( client.right - client.left, client.bottom - client.top ));
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG( client.left, client.top ));
    }
    else WIN_ReleasePtr( wndPtr );

    /* if previous state was minimized Windows sets focus to the window */
    if (style & WS_MINIMIZE) SetFocus( hwnd );

    return wasVisible;
}


/**********************************************************************
 *		X11DRV_MapNotify
 */
void X11DRV_MapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;
    HWND hwndFocus = GetFocus();
    WND *win;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if (data->managed && (win->dwStyle & WS_VISIBLE) && (win->dwStyle & WS_MINIMIZE))
    {
        int x, y;
        unsigned int width, height, border, depth;
        Window root, top;
        RECT rect;
        LONG style = WS_VISIBLE;

        /* FIXME: hack */
        wine_tsx11_lock();
        XGetGeometry( event->xmap.display, data->whole_window, &root, &x, &y, &width, &height,
                        &border, &depth );
        XTranslateCoordinates( event->xmap.display, data->whole_window, root, 0, 0, &x, &y, &top );
        wine_tsx11_unlock();
        rect.left   = x;
        rect.top    = y;
        rect.right  = x + width;
        rect.bottom = y + height;
        OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );
        X11DRV_X_to_window_rect( data, &rect );

        invalidate_dce( hwnd, &data->window_rect );

        if (win->flags & WIN_RESTORE_MAX) style |= WS_MAXIMIZE;
        WIN_SetStyle( hwnd, style, WS_MINIMIZE );
        WIN_ReleasePtr( win );

        SendMessageW( hwnd, WM_SHOWWINDOW, SW_RESTORE, 0 );
        data->lock_changes++;
        SetWindowPos( hwnd, 0, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_STATECHANGED );
        data->lock_changes--;
    }
    else WIN_ReleasePtr( win );
    if (hwndFocus && IsChild( hwnd, hwndFocus )) X11DRV_SetFocus(hwndFocus);  /* FIXME */
}


/**********************************************************************
 *              X11DRV_UnmapNotify
 */
void X11DRV_UnmapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;
    WND *win;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (!(win = WIN_GetPtr( hwnd ))) return;

    if ((win->dwStyle & WS_VISIBLE) && data->managed &&
        X11DRV_is_window_rect_mapped( &win->rectWindow ))
    {
        if (win->dwStyle & WS_MAXIMIZE)
            win->flags |= WIN_RESTORE_MAX;
        else if (!(win->dwStyle & WS_MINIMIZE))
            win->flags &= ~WIN_RESTORE_MAX;

        WIN_SetStyle( hwnd, WS_MINIMIZE, WS_MAXIMIZE );
        WIN_ReleasePtr( win );

        EndMenu();
        SendMessageW( hwnd, WM_SHOWWINDOW, SW_MINIMIZE, 0 );
        data->lock_changes++;
        SetWindowPos( hwnd, 0, 0, 0, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                      SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_STATECHANGED );
        data->lock_changes--;
    }
    else WIN_ReleasePtr( win );
}

struct desktop_resize_data
{
    RECT  old_screen_rect;
    RECT  old_virtual_rect;
};

static BOOL CALLBACK update_windows_on_desktop_resize( HWND hwnd, LPARAM lparam )
{
    struct x11drv_win_data *data;
    Display *display = thread_display();
    struct desktop_resize_data *resize_data = (struct desktop_resize_data *)lparam;
    int mask = 0;

    if (!(data = X11DRV_get_win_data( hwnd ))) return TRUE;

    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE)
    {
        BOOL new_fs_state;

        if (fullscreen_state_changed( data, &data->client_rect, &resize_data->old_screen_rect, &new_fs_state ))
            update_fullscreen_state( display, data->whole_window, new_fs_state );
    }

    if (resize_data->old_virtual_rect.left != virtual_screen_rect.left) mask |= CWX;
    if (resize_data->old_virtual_rect.top != virtual_screen_rect.top) mask |= CWY;
    if (mask && data->whole_window)
    {
        XWindowChanges changes;

        wine_tsx11_lock();
        changes.x = data->whole_rect.left - virtual_screen_rect.left;
        changes.y = data->whole_rect.top - virtual_screen_rect.top;
        XReconfigureWMWindow( display, data->whole_window,
                              DefaultScreen(display), mask, &changes );
        wine_tsx11_unlock();
    }
    return TRUE;
}


/***********************************************************************
 *		X11DRV_handle_desktop_resize
 */
void X11DRV_handle_desktop_resize( unsigned int width, unsigned int height )
{
    HWND hwnd = GetDesktopWindow();
    struct x11drv_win_data *data;
    struct desktop_resize_data resize_data;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    SetRect( &resize_data.old_screen_rect, 0, 0, screen_width, screen_height );
    resize_data.old_virtual_rect = virtual_screen_rect;

    screen_width  = width;
    screen_height = height;
    xinerama_init();
    TRACE("desktop %p change to (%dx%d)\n", hwnd, width, height);
    data->lock_changes++;
    X11DRV_SetWindowPos( hwnd, 0, &virtual_screen_rect, &virtual_screen_rect,
                         SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE, NULL );
    data->lock_changes--;
    ClipCursor(NULL);
    SendMessageTimeoutW( HWND_BROADCAST, WM_DISPLAYCHANGE, screen_depth,
                         MAKELPARAM( width, height ), SMTO_ABORTIFHUNG, 2000, NULL );

    EnumWindows( update_windows_on_desktop_resize, (LPARAM)&resize_data );
}


/***********************************************************************
 *		X11DRV_ConfigureNotify
 */
void X11DRV_ConfigureNotify( HWND hwnd, XEvent *xev )
{
    XConfigureEvent *event = &xev->xconfigure;
    struct x11drv_win_data *data;
    RECT rect;
    UINT flags;
    int cx, cy, x = event->x, y = event->y;

    if (!hwnd) return;
    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    /* Get geometry */

    if (!event->send_event)  /* normal event, need to map coordinates to the root */
    {
        Window child;
        wine_tsx11_lock();
        XTranslateCoordinates( event->display, data->whole_window, root_window,
                               0, 0, &x, &y, &child );
        wine_tsx11_unlock();
    }
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + event->width;
    rect.bottom = y + event->height;
    OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );
    TRACE( "win %p new X rect %d,%d,%dx%d (event %d,%d,%dx%d)\n",
           hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
           event->x, event->y, event->width, event->height );
    X11DRV_X_to_window_rect( data, &rect );

    x     = rect.left;
    y     = rect.top;
    cx    = rect.right - rect.left;
    cy    = rect.bottom - rect.top;
    flags = SWP_NOACTIVATE | SWP_NOZORDER;

    /* Compare what has changed */

    GetWindowRect( hwnd, &rect );
    if (rect.left == x && rect.top == y) flags |= SWP_NOMOVE;
    else
        TRACE( "%p moving from (%d,%d) to (%d,%d)\n",
               hwnd, rect.left, rect.top, x, y );

    if ((rect.right - rect.left == cx && rect.bottom - rect.top == cy) ||
        IsIconic(hwnd) ||
        (IsRectEmpty( &rect ) && event->width == 1 && event->height == 1))
    {
        if (flags & SWP_NOMOVE) return;  /* if nothing changed, don't do anything */
        flags |= SWP_NOSIZE;
    }
    else
        TRACE( "%p resizing from (%dx%d) to (%dx%d)\n",
               hwnd, rect.right - rect.left, rect.bottom - rect.top, cx, cy );

    data->lock_changes++;
    SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
    data->lock_changes--;
}


/***********************************************************************
 *		SetWindowRgn  (X11DRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
int X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    struct x11drv_win_data *data;

    if (!(data = X11DRV_get_win_data( hwnd )))
    {
        if (IsWindow( hwnd ))
            FIXME( "not supported on other thread window %p\n", hwnd );
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

#ifdef HAVE_LIBXSHAPE
    if (data->whole_window)
    {
        Display *display = thread_display();

        if (!hrgn)
        {
            wine_tsx11_lock();
            XShapeCombineMask( display, data->whole_window,
                               ShapeBounding, 0, 0, None, ShapeSet );
            wine_tsx11_unlock();
        }
        else
        {
            RGNDATA *pRegionData = X11DRV_GetRegionData( hrgn, 0 );
            if (pRegionData)
            {
                wine_tsx11_lock();
                XShapeCombineRectangles( display, data->whole_window, ShapeBounding,
                                         data->window_rect.left - data->whole_rect.left,
                                         data->window_rect.top - data->whole_rect.top,
                                         (XRectangle *)pRegionData->Buffer,
                                         pRegionData->rdh.nCount,
                                         ShapeSet, YXBanded );
                wine_tsx11_unlock();
                HeapFree(GetProcessHeap(), 0, pRegionData);
            }
        }
    }
#endif  /* HAVE_LIBXSHAPE */

    invalidate_dce( hwnd, &data->window_rect );
    return TRUE;
}


/***********************************************************************
 *           draw_moving_frame
 *
 * Draw the frame used when moving or resizing window.
 *
 * FIXME:  This causes problems in Win95 mode.  (why?)
 */
static void draw_moving_frame( HDC hdc, RECT *rect, BOOL thickframe )
{
    if (thickframe)
    {
        const int width = GetSystemMetrics(SM_CXFRAME);
        const int height = GetSystemMetrics(SM_CYFRAME);

        HBRUSH hbrush = SelectObject( hdc, GetStockObject( GRAY_BRUSH ) );
        PatBlt( hdc, rect->left, rect->top,
                rect->right - rect->left - width, height, PATINVERT );
        PatBlt( hdc, rect->left, rect->top + height, width,
                rect->bottom - rect->top - height, PATINVERT );
        PatBlt( hdc, rect->left + width, rect->bottom - 1,
                rect->right - rect->left - width, -height, PATINVERT );
        PatBlt( hdc, rect->right - 1, rect->top, -width,
                rect->bottom - rect->top - height, PATINVERT );
        SelectObject( hdc, hbrush );
    }
    else DrawFocusRect( hdc, rect );
}


/***********************************************************************
 *           start_size_move
 *
 * Initialisation of a move or resize, when initiatied from a menu choice.
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
         * of border into account, but it shouldn't make more that a few pixels
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
        pt.x = pt.y = 0;
        while(!hittest)
        {
            GetMessageW( &msg, 0, WM_KEYFIRST, WM_MOUSELAST );
            if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

            switch(msg.message)
            {
            case WM_MOUSEMOVE:
                pt = msg.pt;
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
                case VK_ESCAPE: return 0;
                }
            }
        }
        *capturePoint = pt;
    }
    SetCursorPos( pt.x, pt.y );
    SendMessageW( hwnd, WM_SETCURSOR, (WPARAM)hwnd, MAKELONG( hittest, WM_MOUSEMOVE ));
    return hittest;
}


/***********************************************************************
 *           set_movesize_capture
 */
static void set_movesize_capture( HWND hwnd )
{
    HWND previous = 0;

    SERVER_START_REQ( set_capture_window )
    {
        req->handle = hwnd;
        req->flags  = CAPTURE_MOVESIZE;
        if (!wine_server_call_err( req ))
        {
            previous = reply->previous;
            hwnd = reply->full_handle;
        }
    }
    SERVER_END_REQ;
    if (previous && previous != hwnd)
        SendMessageW( previous, WM_CAPTURECHANGED, 0, (LPARAM)hwnd );
}

/***********************************************************************
 *           X11DRV_WMMoveResizeWindow
 *
 * Tells the window manager to initiate a move or resize operation.
 *
 * SEE
 *  http://freedesktop.org/Standards/wm-spec/1.3/ar01s04.html
 *  or search for "_NET_WM_MOVERESIZE"
 */
static BOOL X11DRV_WMMoveResizeWindow( HWND hwnd, int x, int y, WPARAM wparam )
{
    WPARAM syscommand = wparam & 0xfff0;
    WPARAM hittest = wparam & 0x0f;
    int dir;
    XEvent xev;
    Display *display = thread_display();

    if (syscommand == SC_MOVE)
    {
        if (!hittest) dir = _NET_WM_MOVERESIZE_MOVE_KEYBOARD;
        else dir = _NET_WM_MOVERESIZE_MOVE;
    }
    else
    {
        /* windows without WS_THICKFRAME are not resizable through the window manager */
        if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_THICKFRAME)) return FALSE;

        switch (hittest)
        {
        case WMSZ_LEFT:        dir = _NET_WM_MOVERESIZE_SIZE_LEFT; break;
        case WMSZ_RIGHT:       dir = _NET_WM_MOVERESIZE_SIZE_RIGHT; break;
        case WMSZ_TOP:         dir = _NET_WM_MOVERESIZE_SIZE_TOP; break;
        case WMSZ_TOPLEFT:     dir = _NET_WM_MOVERESIZE_SIZE_TOPLEFT; break;
        case WMSZ_TOPRIGHT:    dir = _NET_WM_MOVERESIZE_SIZE_TOPRIGHT; break;
        case WMSZ_BOTTOM:      dir = _NET_WM_MOVERESIZE_SIZE_BOTTOM; break;
        case WMSZ_BOTTOMLEFT:  dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT; break;
        case WMSZ_BOTTOMRIGHT: dir = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT; break;
        default:               dir = _NET_WM_MOVERESIZE_SIZE_KEYBOARD; break;
        }
    }

    TRACE("hwnd %p, x %d, y %d, dir %d\n", hwnd, x, y, dir);

    xev.xclient.type = ClientMessage;
    xev.xclient.window = X11DRV_get_whole_window(hwnd);
    xev.xclient.message_type = x11drv_atom(_NET_WM_MOVERESIZE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = x; /* x coord */
    xev.xclient.data.l[1] = y; /* y coord */
    xev.xclient.data.l[2] = dir; /* direction */
    xev.xclient.data.l[3] = 1; /* button */
    xev.xclient.data.l[4] = 0; /* unused */

    /* need to ungrab the pointer that may have been automatically grabbed
     * with a ButtonPress event */
    wine_tsx11_lock();
    XUngrabPointer( display, CurrentTime );
    XSendEvent(display, root_window, False, SubstructureNotifyMask, &xev);
    wine_tsx11_unlock();
    return TRUE;
}

/***********************************************************************
 *           SysCommandSizeMove   (X11DRV.@)
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
void X11DRV_SysCommandSizeMove( HWND hwnd, WPARAM wParam )
{
    MSG msg;
    RECT sizingRect, mouseRect, origRect;
    HDC hdc;
    HWND parent;
    LONG hittest = (LONG)(wParam & 0x0f);
    WPARAM syscommand = wParam & 0xfff0;
    HCURSOR hDragCursor = 0, hOldCursor = 0;
    POINT minTrack, maxTrack;
    POINT capturePoint, pt;
    LONG style = GetWindowLongA( hwnd, GWL_STYLE );
    BOOL    thickframe = HAS_THICKFRAME( style );
    BOOL    iconic = style & WS_MINIMIZE;
    BOOL    moved = FALSE;
    DWORD     dwPoint = GetMessagePos ();
    BOOL DragFullWindows = FALSE;
    BOOL grab;
    Window parent_win, whole_win;
    Display *old_gdi_display = NULL;
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    struct x11drv_win_data *data;

    pt.x = (short)LOWORD(dwPoint);
    pt.y = (short)HIWORD(dwPoint);
    capturePoint = pt;

    if (IsZoomed(hwnd) || !IsWindowVisible(hwnd)) return;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    TRACE("hwnd %p (%smanaged), command %04lx, hittest %d, pos %d,%d\n",
          hwnd, data->managed ? "" : "NOT ", syscommand, hittest, pt.x, pt.y);

    /* if we are managed then we let the WM do all the work */
    if (data->managed && X11DRV_WMMoveResizeWindow( hwnd, pt.x, pt.y, wParam )) return;

    SystemParametersInfoA(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

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
            set_movesize_capture( hwnd );
            hittest = start_size_move( hwnd, wParam, &capturePoint, style );
            if (!hittest)
            {
                set_movesize_capture(0);
                return;
            }
        }
    }

      /* Get min/max info */

    WINPOS_GetMinMaxInfo( hwnd, NULL, NULL, &minTrack, &maxTrack );
    GetWindowRect( hwnd, &sizingRect );
    if (style & WS_CHILD)
    {
        parent = GetParent(hwnd);
        /* make sizing rect relative to parent */
        MapWindowPoints( 0, parent, (POINT*)&sizingRect, 2 );
        GetClientRect( parent, &mouseRect );
    }
    else
    {
        parent = 0;
        mouseRect = virtual_screen_rect;
    }
    origRect = sizingRect;

    if (ON_LEFT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x );
        mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x );
    }
    else if (ON_RIGHT_BORDER(hittest))
    {
        mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x );
        mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x );
    }
    if (ON_TOP_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y );
        mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y);
    }
    else if (ON_BOTTOM_BORDER(hittest))
    {
        mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y );
        mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y );
    }
    if (parent) MapWindowPoints( parent, 0, (LPPOINT)&mouseRect, 2 );

    /* Retrieve a default cache DC (without using the window style) */
    hdc = GetDCEx( parent, 0, DCX_CACHE );

    if( iconic ) /* create a cursor for dragging */
    {
        hDragCursor = (HCURSOR)GetClassLongPtrW( hwnd, GCLP_HICON);
        if( !hDragCursor ) hDragCursor = (HCURSOR)SendMessageW( hwnd, WM_QUERYDRAGICON, 0, 0L);
        if( !hDragCursor ) iconic = FALSE;
    }

    /* repaint the window before moving it around */
    RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );

    SendMessageW( hwnd, WM_ENTERSIZEMOVE, 0, 0 );
    set_movesize_capture( hwnd );

    /* grab the server only when moving top-level windows without desktop */
    grab = (!DragFullWindows && !parent && (root_window == DefaultRootWindow(gdi_display)));

    if (grab)
    {
        wine_tsx11_lock();
        XSync( gdi_display, False );
        XGrabServer( thread_data->display );
        XSync( thread_data->display, False );
        /* switch gdi display to the thread display, since the server is grabbed */
        old_gdi_display = gdi_display;
        gdi_display = thread_data->display;
        wine_tsx11_unlock();
    }
    whole_win = X11DRV_get_whole_window( GetAncestor(hwnd,GA_ROOT) );
    parent_win = parent ? X11DRV_get_whole_window( GetAncestor(parent,GA_ROOT) ) : root_window;

    wine_tsx11_lock();
    XGrabPointer( thread_data->display, whole_win, False,
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                  GrabModeAsync, GrabModeAsync, parent_win, None, CurrentTime );
    wine_tsx11_unlock();
    thread_data->grab_window = whole_win;

    while(1)
    {
        int dx = 0, dy = 0;

        if (!GetMessageW( &msg, 0, WM_KEYFIRST, WM_MOUSELAST )) break;
        if (CallMsgFilterW( &msg, MSGF_SIZE )) continue;

        /* Exit on button-up, Return, or Esc */
        if ((msg.message == WM_LBUTTONUP) ||
            ((msg.message == WM_KEYDOWN) &&
             ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

        if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
            continue;  /* We are not interested in other messages */

        pt = msg.pt;

        if (msg.message == WM_KEYDOWN) switch(msg.wParam)
        {
        case VK_UP:    pt.y -= 8; break;
        case VK_DOWN:  pt.y += 8; break;
        case VK_LEFT:  pt.x -= 8; break;
        case VK_RIGHT: pt.x += 8; break;
        }

        pt.x = max( pt.x, mouseRect.left );
        pt.x = min( pt.x, mouseRect.right );
        pt.y = max( pt.y, mouseRect.top );
        pt.y = min( pt.y, mouseRect.bottom );

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
                    draw_moving_frame( hdc, &sizingRect, thickframe );
            }

            if (msg.message == WM_KEYDOWN) SetCursorPos( pt.x, pt.y );
            else
            {
                RECT newRect = sizingRect;
                WPARAM wpSizingHit = 0;

                if (hittest == HTCAPTION) OffsetRect( &newRect, dx, dy );
                if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
                else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
                if (ON_TOP_BORDER(hittest)) newRect.top += dy;
                else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
                if(!iconic && !DragFullWindows) draw_moving_frame( hdc, &sizingRect, thickframe );
                capturePoint = pt;

                /* determine the hit location */
                if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                    wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                SendMessageW( hwnd, WM_SIZING, wpSizingHit, (LPARAM)&newRect );

                if (!iconic)
                {
                    if(!DragFullWindows)
                        draw_moving_frame( hdc, &newRect, thickframe );
                    else
                        SetWindowPos( hwnd, 0, newRect.left, newRect.top,
                                      newRect.right - newRect.left,
                                      newRect.bottom - newRect.top,
                                      ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
                }
                sizingRect = newRect;
            }
        }
    }

    set_movesize_capture(0);
    if( iconic )
    {
        if( moved ) /* restore cursors, show icon title later on */
        {
            ShowCursor( FALSE );
            SetCursor( hOldCursor );
        }
    }
    else if (moved && !DragFullWindows)
        draw_moving_frame( hdc, &sizingRect, thickframe );

    ReleaseDC( parent, hdc );

    wine_tsx11_lock();
    XUngrabPointer( thread_data->display, CurrentTime );
    if (grab)
    {
        XSync( thread_data->display, False );
        XUngrabServer( thread_data->display );
        XSync( thread_data->display, False );
        gdi_display = old_gdi_display;
    }
    wine_tsx11_unlock();
    thread_data->grab_window = None;

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
            if(!DragFullWindows)
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
