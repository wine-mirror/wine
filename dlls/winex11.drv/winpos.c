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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/wingdi16.h"

#include "x11drv.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

#define SWP_AGG_NOPOSCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

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

    if (event->window == data->whole_window)
    {
        rect.left = data->whole_rect.left + event->x;
        rect.top  = data->whole_rect.top + event->y;
        flags |= RDW_FRAME;
    }
    else
    {
        rect.left = data->client_rect.left + event->x;
        rect.top  = data->client_rect.top + event->y;
    }
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    if (event->window == root_window)
        OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );

    SERVER_START_REQ( update_window_zorder )
    {
        req->window      = hwnd;
        req->rect.left   = rect.left;
        req->rect.top    = rect.top;
        req->rect.right  = rect.right;
        req->rect.bottom = rect.bottom;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* make position relative to client area instead of parent */
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
    new_style = GetWindowLongW( hwnd, GWL_STYLE );
    changed = new_style ^ old_style;

    if ((changed & WS_VISIBLE) && (new_style & WS_VISIBLE))
    {
        /* we don't unmap windows, that causes trouble with the window manager */
        if (!(data = X11DRV_get_win_data( hwnd )) &&
            !(data = X11DRV_create_win_data( hwnd ))) return;

        if (data->whole_window && X11DRV_is_window_rect_mapped( &data->window_rect ))
        {
            X11DRV_set_wm_hints( display, data );
            if (!data->mapped)
            {
                TRACE( "mapping win %p/%lx\n", hwnd, data->whole_window );
                wait_for_withdrawn_state( display, data, TRUE );
                X11DRV_sync_window_style( display, data );
                wine_tsx11_lock();
                XMapWindow( display, data->whole_window );
                wine_tsx11_unlock();
                data->mapped = TRUE;
                data->iconic = (new_style & WS_MINIMIZE) != 0;
            }
        }
    }

    if (changed & WS_DISABLED)
    {
        data = X11DRV_get_win_data( hwnd );
        if (data && data->wm_hints)
        {
            wine_tsx11_lock();
            data->wm_hints->input = !(new_style & WS_DISABLED);
            XSetWMHints( display, data->whole_window, data->wm_hints );
            wine_tsx11_unlock();
        }
    }
}


/***********************************************************************
 *     update_net_wm_states
 */
static void update_net_wm_states( Display *display, struct x11drv_win_data *data )
{
    static const unsigned int state_atoms[NB_NET_WM_STATES] =
    {
        XATOM__NET_WM_STATE_FULLSCREEN,
        XATOM__NET_WM_STATE_ABOVE,
        XATOM__NET_WM_STATE_MAXIMIZED_VERT,
        XATOM__NET_WM_STATE_SKIP_PAGER,
        XATOM__NET_WM_STATE_SKIP_TASKBAR
    };

    DWORD i, style, ex_style, new_state = 0;
    XEvent xev;

    if (!data->managed) return;
    if (!data->mapped) return;

    style = GetWindowLongW( data->hwnd, GWL_STYLE );
    if (style & WS_MAXIMIZE) new_state |= (1 << NET_WM_STATE_MAXIMIZED);

    if (!(style & WS_MAXIMIZE) &&
        data->whole_rect.left <= 0 && data->whole_rect.right >= screen_width &&
        data->whole_rect.top <= 0 && data->whole_rect.bottom >= screen_height)
        new_state |= (1 << NET_WM_STATE_FULLSCREEN);

    ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );
    if (ex_style & WS_EX_TOPMOST)
        new_state |= (1 << NET_WM_STATE_ABOVE);
    if (ex_style & WS_EX_TOOLWINDOW)
        new_state |= (1 << NET_WM_STATE_SKIP_TASKBAR) | (1 << NET_WM_STATE_SKIP_PAGER);

    xev.xclient.type = ClientMessage;
    xev.xclient.window = data->whole_window;
    xev.xclient.message_type = x11drv_atom(_NET_WM_STATE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[3] = 1;

    for (i = 0; i < NB_NET_WM_STATES; i++)
    {
        if (!((data->net_wm_state ^ new_state) & (1 << i))) continue;  /* unchanged */

        TRACE( "setting wm state %u for window %p/%lx to %u prev %u\n",
               i, data->hwnd, data->whole_window,
               (new_state & (1 << i)) != 0, (data->net_wm_state & (1 << i)) != 0 );

        xev.xclient.data.l[0] = (new_state & (1 << i)) ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        xev.xclient.data.l[1] = X11DRV_Atoms[state_atoms[i] - FIRST_XATOM];
        xev.xclient.data.l[2] = ((state_atoms[i] == XATOM__NET_WM_STATE_MAXIMIZED_VERT) ?
                                 x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ) : 0);
        wine_tsx11_lock();
        XSendEvent( display, root_window, False, SubstructureRedirectMask | SubstructureNotifyMask, &xev );
        wine_tsx11_unlock();
    }
    data->net_wm_state = new_state;
}


/***********************************************************************
 *		move_window_bits
 *
 * Move the window bits when a window is moved.
 */
static void move_window_bits( struct x11drv_win_data *data, const RECT *old_rect, const RECT *new_rect,
                              const RECT *old_client_rect )
{
    RECT src_rect = *old_rect;
    RECT dst_rect = *new_rect;
    HDC hdc_src, hdc_dst;
    INT code;
    HRGN rgn = 0;
    HWND parent = 0;

    if (!data->whole_window)
    {
        OffsetRect( &dst_rect, -data->window_rect.left, -data->window_rect.top );
        parent = GetAncestor( data->hwnd, GA_PARENT );
        hdc_src = GetDCEx( parent, 0, DCX_CACHE );
        hdc_dst = GetDCEx( data->hwnd, 0, DCX_CACHE | DCX_WINDOW );
    }
    else
    {
        OffsetRect( &dst_rect, -data->client_rect.left, -data->client_rect.top );
        /* make src rect relative to the old position of the window */
        OffsetRect( &src_rect, -old_client_rect->left, -old_client_rect->top );
        if (dst_rect.left == src_rect.left && dst_rect.top == src_rect.top) return;
        hdc_src = hdc_dst = GetDCEx( data->hwnd, 0, DCX_CACHE );
    }

    code = X11DRV_START_EXPOSURES;
    ExtEscape( hdc_dst, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );

    TRACE( "copying bits for win %p/%lx/%lx %s -> %s\n",
           data->hwnd, data->whole_window, data->client_window,
           wine_dbgstr_rect(&src_rect), wine_dbgstr_rect(&dst_rect) );
    BitBlt( hdc_dst, dst_rect.left, dst_rect.top,
            dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top,
            hdc_src, src_rect.left, src_rect.top, SRCCOPY );

    code = X11DRV_END_EXPOSURES;
    ExtEscape( hdc_dst, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(rgn), (LPSTR)&rgn );

    ReleaseDC( data->hwnd, hdc_dst );
    if (hdc_src != hdc_dst) ReleaseDC( parent, hdc_src );

    if (rgn)
    {
        if (!data->whole_window)
        {
            /* map region to client rect since we are using DCX_WINDOW */
            OffsetRgn( rgn, data->window_rect.left - data->client_rect.left,
                       data->window_rect.top - data->client_rect.top );
            RedrawWindow( data->hwnd, NULL, rgn,
                          RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN );
        }
        else RedrawWindow( data->hwnd, NULL, rgn, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
        DeleteObject( rgn );
    }
}

/***********************************************************************
 *		SetWindowPos   (X11DRV.@)
 */
void X11DRV_SetWindowPos( HWND hwnd, HWND insert_after, UINT swp_flags,
                          const RECT *rectWindow, const RECT *rectClient,
                          const RECT *visible_rect, const RECT *valid_rects )
{
    Display *display = thread_display();
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );
    DWORD new_style = GetWindowLongW( hwnd, GWL_STYLE );
    RECT old_window_rect, old_whole_rect, old_client_rect;

    if (!data)
    {
        /* create the win data if the window is being made visible */
        if (!(new_style & WS_VISIBLE)) return;
        if (!(data = X11DRV_create_win_data( hwnd ))) return;
    }

    /* check if we need to switch the window to managed */
    if (!data->managed && data->whole_window && is_window_managed( hwnd, swp_flags, rectWindow ))
    {
        TRACE( "making win %p/%lx managed\n", hwnd, data->whole_window );
        data->managed = TRUE;
        SetPropA( hwnd, managed_prop, (HANDLE)1 );
        if (data->mapped)
        {
            wine_tsx11_lock();
            XUnmapWindow( display, data->whole_window );
            wine_tsx11_unlock();
            data->mapped = FALSE;
        }
    }

    old_window_rect = data->window_rect;
    old_whole_rect  = data->whole_rect;
    old_client_rect = data->client_rect;
    data->window_rect = *rectWindow;
    data->whole_rect  = *rectWindow;
    data->client_rect = *rectClient;
    X11DRV_window_to_X_rect( data, &data->whole_rect );
    if (memcmp( &visible_rect, &data->whole_rect, sizeof(RECT) ))
    {
        TRACE( "%p: need to update visible rect %s -> %s\n", hwnd,
               wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(&data->whole_rect) );
        SERVER_START_REQ( set_window_visible_rect )
        {
            req->handle         = hwnd;
            req->flags          = swp_flags;
            req->visible.left   = data->whole_rect.left;
            req->visible.top    = data->whole_rect.top;
            req->visible.right  = data->whole_rect.right;
            req->visible.bottom = data->whole_rect.bottom;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    TRACE( "win %p window %s client %s style %08x flags %08x\n",
           hwnd, wine_dbgstr_rect(rectWindow), wine_dbgstr_rect(rectClient), new_style, swp_flags );

    if (!IsRectEmpty( &valid_rects[0] ))
    {
        int x_offset = old_whole_rect.left - data->whole_rect.left;
        int y_offset = old_whole_rect.top - data->whole_rect.top;

        /* if all that happened is that the whole window moved, copy everything */
        if (!(swp_flags & SWP_FRAMECHANGED) &&
            old_whole_rect.right   - data->whole_rect.right   == x_offset &&
            old_whole_rect.bottom  - data->whole_rect.bottom  == y_offset &&
            old_client_rect.left   - data->client_rect.left   == x_offset &&
            old_client_rect.right  - data->client_rect.right  == x_offset &&
            old_client_rect.top    - data->client_rect.top    == y_offset &&
            old_client_rect.bottom - data->client_rect.bottom == y_offset &&
            !memcmp( &valid_rects[0], &data->client_rect, sizeof(RECT) ))
        {
            /* if we have an X window the bits will be moved by the X server */
            if (!data->whole_window)
                move_window_bits( data, &old_whole_rect, &data->whole_rect, &old_client_rect );
        }
        else
            move_window_bits( data, &valid_rects[1], &valid_rects[0], &old_client_rect );
    }

    X11DRV_sync_client_position( display, data, swp_flags, &old_client_rect, &old_whole_rect );

    if (!data->whole_window || data->lock_changes) return;  /* nothing more to do */

    if (data->mapped && (!(new_style & WS_VISIBLE) || !X11DRV_is_window_rect_mapped( rectWindow )))
    {
        TRACE( "unmapping win %p/%lx\n", hwnd, data->whole_window );
        wait_for_withdrawn_state( display, data, FALSE );
        wine_tsx11_lock();
        if (data->managed) XWithdrawWindow( display, data->whole_window, DefaultScreen(display) );
        else XUnmapWindow( display, data->whole_window );
        wine_tsx11_unlock();
        data->mapped = FALSE;
        data->net_wm_state = 0;
    }

    /* don't change position if we are about to minimize or maximize a managed window */
    if (!(data->managed && (swp_flags & SWP_STATECHANGED) && (new_style & (WS_MINIMIZE|WS_MAXIMIZE))))
        X11DRV_sync_window_position( display, data, swp_flags, &old_client_rect, &old_whole_rect );

    if ((new_style & WS_VISIBLE) &&
        ((new_style & WS_MINIMIZE) || X11DRV_is_window_rect_mapped( rectWindow )))
    {
        if (!data->mapped || (swp_flags & (SWP_FRAMECHANGED|SWP_STATECHANGED)))
            X11DRV_set_wm_hints( display, data );

        if (!data->mapped)
        {
            TRACE( "mapping win %p/%lx\n", hwnd, data->whole_window );
            wait_for_withdrawn_state( display, data, TRUE );
            X11DRV_sync_window_style( display, data );
            wine_tsx11_lock();
            XMapWindow( display, data->whole_window );
            XFlush( display );
            wine_tsx11_unlock();
            data->mapped = TRUE;
            data->iconic = (new_style & WS_MINIMIZE) != 0;
        }
        else if ((swp_flags & SWP_STATECHANGED) && (!data->iconic != !(new_style & WS_MINIMIZE)))
        {
            data->iconic = (new_style & WS_MINIMIZE) != 0;
            TRACE( "changing win %p iconic state to %u\n", data->hwnd, data->iconic );
            wine_tsx11_lock();
            if (data->iconic)
                XIconifyWindow( display, data->whole_window, DefaultScreen(display) );
            else if (X11DRV_is_window_rect_mapped( rectWindow ))
                XMapWindow( display, data->whole_window );
            wine_tsx11_unlock();
        }
        update_net_wm_states( display, data );
    }
}


/**********************************************************************
 *		X11DRV_MapNotify
 */
void X11DRV_MapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;
    int state;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->mapped) return;

    if (!data->managed)
    {
        HWND hwndFocus = GetFocus();
        if (hwndFocus && IsChild( hwnd, hwndFocus )) X11DRV_SetFocus(hwndFocus);  /* FIXME */
        return;
    }
    if (!data->iconic) return;

    state = get_window_wm_state( event->xmap.display, data );
    if (state == NormalState)
    {
        int x, y;
        unsigned int width, height, border, depth;
        Window root, top;
        WINDOWPLACEMENT wp;
        RECT rect;

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

        wp.length = sizeof(wp);
        GetWindowPlacement( hwnd, &wp );
        wp.flags = 0;
        wp.showCmd = SW_RESTORE;
        wp.rcNormalPosition = rect;

        TRACE( "restoring win %p/%lx\n", hwnd, data->whole_window );
        data->iconic = FALSE;
        data->lock_changes++;
        SetWindowPlacement( hwnd, &wp );
        data->lock_changes--;
    }
    else TRACE( "win %p/%lx ignoring since state=%d\n", hwnd, data->whole_window, state );
}


/**********************************************************************
 *              X11DRV_UnmapNotify
 */
void X11DRV_UnmapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;
    int state;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->managed || !data->mapped || data->iconic) return;

    state = get_window_wm_state( event->xunmap.display, data );
    if (state == IconicState)
    {
        TRACE( "minimizing win %p/%lx\n", hwnd, data->whole_window );
        data->iconic = TRUE;
        data->lock_changes++;
        ShowWindow( hwnd, SW_MINIMIZE );
        data->lock_changes--;
    }
    else TRACE( "win %p/%lx ignoring since state=%d\n", hwnd, data->whole_window, state );
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
        /* update the full screen state */
        update_net_wm_states( display, data );
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
 *		X11DRV_resize_desktop
 */
void X11DRV_resize_desktop( unsigned int width, unsigned int height )
{
    HWND hwnd = GetDesktopWindow();
    struct desktop_resize_data resize_data;

    SetRect( &resize_data.old_screen_rect, 0, 0, screen_width, screen_height );
    resize_data.old_virtual_rect = virtual_screen_rect;

    xinerama_init( width, height );

    if (GetWindowThreadProcessId( hwnd, NULL ) != GetCurrentThreadId())
    {
        SendMessageW( hwnd, WM_X11DRV_RESIZE_DESKTOP, 0, MAKELPARAM( width, height ) );
    }
    else
    {
        TRACE( "desktop %p change to (%dx%d)\n", hwnd, width, height );
        SetWindowPos( hwnd, 0, virtual_screen_rect.left, virtual_screen_rect.top,
                      virtual_screen_rect.right - virtual_screen_rect.left,
                      virtual_screen_rect.bottom - virtual_screen_rect.top,
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE );
        SendMessageTimeoutW( HWND_BROADCAST, WM_DISPLAYCHANGE, screen_bpp,
                             MAKELPARAM( width, height ), SMTO_ABORTIFHUNG, 2000, NULL );
    }

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
 *              is_netwm_supported
 */
static BOOL is_netwm_supported( Display *display, Atom atom )
{
    static Atom *net_supported;
    static int net_supported_count = -1;
    int i;

    wine_tsx11_lock();
    if (net_supported_count == -1)
    {
        Atom type;
        int format;
        unsigned long count, remaining;

        if (!XGetWindowProperty( display, DefaultRootWindow(display), x11drv_atom(_NET_SUPPORTED), 0,
                                 ~0UL, False, XA_ATOM, &type, &format, &count,
                                 &remaining, (unsigned char **)&net_supported ))
            net_supported_count = count * (format / 8) / sizeof(Atom);
        else
            net_supported_count = 0;
    }
    wine_tsx11_unlock();

    for (i = 0; i < net_supported_count; i++)
        if (net_supported[i] == atom) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           SysCommandSizeMove   (X11DRV.@)
 *
 * Perform SC_MOVE and SC_SIZE commands.
 */
BOOL X11DRV_SysCommandSizeMove( HWND hwnd, WPARAM wparam )
{
    WPARAM syscommand = wparam & 0xfff0;
    WPARAM hittest = wparam & 0x0f;
    DWORD dwPoint;
    int x, y, dir;
    XEvent xev;
    Display *display = thread_display();
    struct x11drv_win_data *data;

    if (!(data = X11DRV_get_win_data( hwnd ))) return FALSE;
    if (!data->whole_window || !data->managed) return FALSE;

    if (!is_netwm_supported( display, x11drv_atom(_NET_WM_MOVERESIZE) ))
    {
        TRACE( "_NET_WM_MOVERESIZE not supported\n" );
        return FALSE;
    }

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

    dwPoint = GetMessagePos();
    x = (short)LOWORD(dwPoint);
    y = (short)HIWORD(dwPoint);

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
