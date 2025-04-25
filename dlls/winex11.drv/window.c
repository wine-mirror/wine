/*
 * Window related functions
 *
 * Copyright 1993, 1994, 1995, 1996, 2001 Alexandre Julliard
 * Copyright 1993 David Metcalfe
 * Copyright 1995, 1996 Alex Korobka
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#ifdef HAVE_LIBXSHAPE
#include <X11/extensions/shape.h>
#endif /* HAVE_LIBXSHAPE */
#ifdef HAVE_X11_EXTENSIONS_XINPUT2_H
#include <X11/extensions/XInput2.h>
#endif

/* avoid conflict with field names in included win32 headers */
#undef Status

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "x11drv.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"
#include "wine/server.h"
#include "mwm.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);
WINE_DECLARE_DEBUG_CHANNEL(systray);

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10

#define _NET_WM_STATE_REMOVE  0
#define _NET_WM_STATE_ADD     1
#define _NET_WM_STATE_TOGGLE  2

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static const unsigned int net_wm_state_atoms[NB_NET_WM_STATES] =
{
    XATOM__KDE_NET_WM_STATE_SKIP_SWITCHER,
    XATOM__NET_WM_STATE_FULLSCREEN,
    XATOM__NET_WM_STATE_ABOVE,
    XATOM__NET_WM_STATE_MAXIMIZED_VERT,
    XATOM__NET_WM_STATE_SKIP_PAGER,
    XATOM__NET_WM_STATE_SKIP_TASKBAR
};

#define SWP_AGG_NOPOSCHANGE (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

/* is cursor clipping active? */
BOOL clipping_cursor = FALSE;

/* X context to associate a hwnd to an X window */
XContext winContext = 0;

/* X context to associate a struct x11drv_win_data to an hwnd */
static XContext win_data_context = 0;
static XContext host_window_context = 0;

static const WCHAR whole_window_prop[] =
    {'_','_','w','i','n','e','_','x','1','1','_','w','h','o','l','e','_','w','i','n','d','o','w',0};
static const WCHAR clip_window_prop[] =
    {'_','_','w','i','n','e','_','x','1','1','_','c','l','i','p','_','w','i','n','d','o','w',0};
static const WCHAR focus_time_prop[] =
    {'_','_','w','i','n','e','_','x','1','1','_','f','o','c','u','s','_','t','i','m','e',0};

static pthread_mutex_t win_data_mutex = PTHREAD_MUTEX_INITIALIZER;

static void host_window_add_ref( struct host_window *win )
{
    int ref = ++win->refcount;
    TRACE( "host window %p/%lx increasing refcount to %d\n", win, win->window, ref );
}

static void host_window_release( struct host_window *win )
{
    int ref = --win->refcount;

    TRACE( "host window %p/%lx decreasing refcount to %d\n", win, win->window, ref );

    if (!ref)
    {
        struct x11drv_thread_data *data = x11drv_thread_data();

        if (!win->destroyed) XSelectInput( data->display, win->window, 0 );
        XDeleteContext( data->display, win->window, host_window_context );
        if (win->parent) host_window_release( win->parent );
        free( win->children );
        free( win );
    }
}

POINT host_window_map_point( struct host_window *win, int x, int y )
{
    POINT pos = {x, y};

    while (win)
    {
        pos.x += win->rect.left;
        pos.y += win->rect.top;
        win = win->parent;
    }

    return pos;
}

static unsigned int find_host_window_child( struct host_window *win, Window child )
{
    unsigned int i;
    for (i = 0; i < win->children_count; i++) if (win->children[i].window == child) break;
    return i;
}

static int host_window_error( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadWindow);
}

struct host_window *get_host_window( Window window, BOOL create )
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    Window xparent = 0, xroot, *xchildren;
    struct host_window *win;
    XWindowAttributes attr;
    unsigned int nchildren;

    if (window == root_window) return NULL;
    if (!XFindContext( data->display, window, host_window_context, (XPointer *)&win )) return win;

    if (!create || !(win = calloc( 1, sizeof(*win) ))) return NULL;
    win->window = window;

    X11DRV_expect_error( data->display, host_window_error, NULL );
    XSelectInput( data->display, window, StructureNotifyMask );
    if (!XGetWindowAttributes( data->display, window, &attr )) memset( &attr, 0, sizeof(attr) );
    if (!XQueryTree( data->display, window, &xroot, &xparent, &xchildren, &nchildren )) xparent = root_window;
    else XFree( xchildren );
    if (X11DRV_check_error()) WARN( "window %lx already destroyed\n", window );

    host_window_set_parent( win, xparent );
    SetRect( &win->rect, attr.x, attr.y, attr.x + attr.width, attr.y + attr.height );
    if (win->parent) host_window_configure_child( win->parent, win->window, win->rect, FALSE );

    TRACE( "created host window %p/%lx, parent %lx rect %s\n", win, win->window,
           xparent, wine_dbgstr_rect(&win->rect) );
    XSaveContext( data->display, window, host_window_context, (char *)win );
    return win;
}

static void host_window_reparent( struct host_window **win, Window parent, Window window )
{
    struct host_window *old = *win, *new = get_host_window( parent, TRUE );
    unsigned int index;
    RECT rect = {0};
    void *tmp;

    if ((*win = new)) host_window_add_ref( new );

    if (old && (index = find_host_window_child( old, window )) < old->children_count)
    {
        rect = old->children[index].rect;
        old->children[index] = old->children[old->children_count - 1];
        old->children_count--;
    }

    TRACE( "parent %lx, window %lx, rect %s, old %p/%lx -> new %p/%lx\n", parent, window,
           wine_dbgstr_rect(&rect), old, old ? old->window : 0, new, new ? new->window : 0 );

    if (new && (index = find_host_window_child( new, window )) == new->children_count)
    {
        if (!(tmp = realloc( new->children, (index + 1) * sizeof(*new->children) ))) return;
        new->children = tmp;

        OffsetRect( &rect, -rect.left, -rect.top );
        new->children[index].window = window;
        new->children[index].rect = rect;

        new->children_count++;
    }

    if (old) host_window_release( old );
}

RECT host_window_configure_child( struct host_window *win, Window window, RECT rect, BOOL root_coords )
{
    unsigned int index;

    TRACE( "host win %p/%lx, window %lx, rect %s, root_coords %u\n", win, win->window,
           window, wine_dbgstr_rect(&rect), root_coords );

    if (root_coords)
    {
        POINT offset = host_window_map_point( win, 0, 0 );
        OffsetRect( &rect, -offset.x, -offset.y );
    }

    index = find_host_window_child( win, window );
    if (index < win->children_count) win->children[index].rect = rect;
    return rect;
}

void host_window_set_parent( struct host_window *win, Window parent )
{
    TRACE( "host window %p/%lx, parent %lx\n", win, win->window, parent );
    host_window_reparent( &win->parent, parent, win->window );
}

/***********************************************************************
 * http://standards.freedesktop.org/startup-notification-spec
 */
static void remove_startup_notification( struct x11drv_win_data *data )
{
    static LONG startup_notification_removed = 0;
    char message[1024];
    const char *id;
    int i;
    int pos;
    XEvent xevent;
    const char *src;
    int srclen;

    if (InterlockedCompareExchange(&startup_notification_removed, 1, 0) != 0)
        return;

    if (!(id = getenv( "DESKTOP_STARTUP_ID" )) || !id[0]) return;
    if ((src = strstr( id, "_TIME" ))) window_set_user_time( data, atol( src + 5 ), FALSE );

    pos = snprintf(message, sizeof(message), "remove: ID=");
    message[pos++] = '"';
    for (i = 0; id[i] && pos < sizeof(message) - 3; i++)
    {
        if (id[i] == '"' || id[i] == '\\')
            message[pos++] = '\\';
        message[pos++] = id[i];
    }
    message[pos++] = '"';
    message[pos++] = '\0';
    unsetenv( "DESKTOP_STARTUP_ID" );

    xevent.xclient.type = ClientMessage;
    xevent.xclient.message_type = x11drv_atom(_NET_STARTUP_INFO_BEGIN);
    xevent.xclient.display = data->display;
    xevent.xclient.window = data->whole_window;
    xevent.xclient.format = 8;

    src = message;
    srclen = strlen(src) + 1;

    while (srclen > 0)
    {
        int msglen = srclen;
        if (msglen > 20)
            msglen = 20;
        memset(&xevent.xclient.data.b[0], 0, 20);
        memcpy(&xevent.xclient.data.b[0], src, msglen);
        src += msglen;
        srclen -= msglen;

        XSendEvent( data->display, DefaultRootWindow( data->display ), False, PropertyChangeMask, &xevent );
        xevent.xclient.message_type = x11drv_atom(_NET_STARTUP_INFO);
    }
}

static BOOL is_managed( HWND hwnd )
{
    struct x11drv_win_data *data = get_win_data( hwnd );
    BOOL ret = data && data->managed;
    release_win_data( data );
    return ret;
}

static HWND *build_hwnd_list(void)
{
    NTSTATUS status;
    HWND *list;
    ULONG count = 128;

    for (;;)
    {
        if (!(list = malloc( count * sizeof(*list) ))) return NULL;
        status = NtUserBuildHwndList( 0, 0, 0, 0, 0, count, list, &count );
        if (!status) return list;
        free( list );
        if (status != STATUS_BUFFER_TOO_SMALL) return NULL;
    }
}

static BOOL has_owned_popups( HWND hwnd )
{
    HWND *list;
    UINT i;
    BOOL ret = FALSE;

    if (!(list = build_hwnd_list())) return FALSE;

    for (i = 0; list[i] != HWND_BOTTOM; i++)
    {
        if (list[i] == hwnd) break;  /* popups are always above owner */
        if (NtUserGetWindowRelative( list[i], GW_OWNER ) != hwnd) continue;
        if ((ret = is_managed( list[i] ))) break;
    }

    free( list );
    return ret;
}

/* returns the HWND for the X11 window, or the desktop window if it isn't a Wine window */
static HWND hwnd_from_window( Display *display, Window window )
{
    HWND hwnd, desktop = NtUserGetDesktopWindow();
    HWND *list;
    UINT i;

    if (!window) return 0;
    if (window == root_window) return desktop;
    if (!XFindContext( display, window, winContext, (char **)&hwnd )) return hwnd;

    if (!(list = build_hwnd_list())) return desktop;

    for (i = 0; list[i] != HWND_BOTTOM; i++)
        if (window == X11DRV_get_whole_window( list[i] ))
            break;
    hwnd = list[i] == HWND_BOTTOM ? desktop : list[i];

    free( list );

    return hwnd;
}

/***********************************************************************
 *              alloc_win_data
 */
static struct x11drv_win_data *alloc_win_data( Display *display, HWND hwnd )
{
    struct x11drv_win_data *data;

    if ((data = calloc( 1, sizeof(*data) )))
    {
        data->display = display;
        data->vis = default_visual;
        data->hwnd = hwnd;
        data->user_time = -1;
        pthread_mutex_lock( &win_data_mutex );
        XSaveContext( gdi_display, (XID)hwnd, win_data_context, (char *)data );
    }
    return data;
}


/***********************************************************************
 *		is_window_managed
 *
 * Check if a given window should be managed
 */
static BOOL is_window_managed( HWND hwnd, UINT swp_flags, BOOL fullscreen )
{
    DWORD style, ex_style;

    if (!managed_mode) return FALSE;

    /* child windows are not managed */
    style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    if ((style & (WS_CHILD|WS_POPUP)) == WS_CHILD) return FALSE;
    /* activated windows are managed */
    if (!(swp_flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW))) return TRUE;
    if (hwnd == get_active_window()) return TRUE;
    /* windows with caption are managed */
    if ((style & WS_CAPTION) == WS_CAPTION) return TRUE;
    /* windows with thick frame are managed */
    if (style & WS_THICKFRAME) return TRUE;
    if (style & WS_POPUP)
    {
        /* popup with sysmenu == caption are managed */
        if (style & WS_SYSMENU) return TRUE;
        /* full-screen popup windows are managed */
        if (fullscreen) return TRUE;
    }
    /* application windows are managed */
    ex_style = NtUserGetWindowLongW( hwnd, GWL_EXSTYLE );
    if (ex_style & WS_EX_APPWINDOW) return TRUE;
    /* windows that own popups are managed */
    if (has_owned_popups( hwnd )) return TRUE;
    /* default: not managed */
    return FALSE;
}


/***********************************************************************
 *		is_window_resizable
 *
 * Check if window should be made resizable by the window manager
 */
static inline BOOL is_window_resizable( struct x11drv_win_data *data, DWORD style )
{
    if (style & WS_THICKFRAME) return TRUE;
    /* Metacity needs the window to be resizable to make it fullscreen */
    return data->is_fullscreen;
}

/***********************************************************************
 *              get_mwm_decorations
 */
static unsigned long get_mwm_decorations_for_style( DWORD style, DWORD ex_style )
{
    unsigned long ret = 0;

    if (ex_style & WS_EX_TOOLWINDOW) return 0;
    if (ex_style & WS_EX_LAYERED) return 0;

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        ret |= MWM_DECOR_TITLE | MWM_DECOR_BORDER;
        if (style & WS_SYSMENU) ret |= MWM_DECOR_MENU;
        if (style & WS_MINIMIZEBOX) ret |= MWM_DECOR_MINIMIZE;
        if (style & WS_MAXIMIZEBOX) ret |= MWM_DECOR_MAXIMIZE;
    }
    return ret;
}


/***********************************************************************
 *              get_mwm_decorations
 */
static unsigned long get_mwm_decorations( struct x11drv_win_data *data, DWORD style, DWORD ex_style )
{
    if (EqualRect( &data->rects.window, &data->rects.visible )) return 0;
    return get_mwm_decorations_for_style( style, ex_style );
}


/***********************************************************************
 *              get_window_attributes
 *
 * Fill the window attributes structure for an X window.
 */
static int get_window_attributes( struct x11drv_win_data *data, XSetWindowAttributes *attr )
{
    attr->colormap          = data->whole_colormap ? data->whole_colormap : default_colormap;
    attr->save_under        = ((NtUserGetClassLongW( data->hwnd, GCL_STYLE ) & CS_SAVEBITS) != 0);
    attr->bit_gravity       = NorthWestGravity;
    attr->backing_store     = NotUseful;
    attr->border_pixel      = 0;
    attr->background_pixel  = 0;
    attr->event_mask        = (ExposureMask | PointerMotionMask |
                               ButtonPressMask | ButtonReleaseMask | EnterWindowMask |
                               KeyPressMask | KeyReleaseMask | FocusChangeMask |
                               KeymapStateMask | StructureNotifyMask | PropertyChangeMask);

    return (CWSaveUnder | CWColormap | CWBorderPixel | CWBackPixel |
            CWEventMask | CWBitGravity | CWBackingStore);
}


/***********************************************************************
 *              sync_window_style
 *
 * Change the X window attributes when the window style has changed.
 */
static void sync_window_style( struct x11drv_win_data *data )
{
    if (data->whole_window != root_window && !data->embedded)
    {
        XSetWindowAttributes attr;
        int mask = get_window_attributes( data, &attr );

        XChangeWindowAttributes( data->display, data->whole_window, mask, &attr );
        x11drv_xinput2_enable( data->display, data->whole_window );
    }
}

static void sync_empty_window_shape( struct x11drv_win_data *data, struct window_surface *surface )
{
#ifdef HAVE_LIBXSHAPE
    if (IsRectEmpty( &data->rects.window ))  /* set an empty shape */
    {
        static XRectangle empty_rect;
        XShapeCombineRectangles( data->display, data->whole_window, ShapeBounding, 0, 0,
                                 &empty_rect, 1, ShapeSet, YXBanded );
    }
    else
    {
        XShapeCombineMask( gdi_display, data->whole_window, ShapeBounding, 0, 0, None, ShapeSet );
        /* invalidate surface shape to make sure it gets updated again */
        if (surface) window_surface_set_shape( surface, 0 );
    }
#endif
}

/***********************************************************************
 *              sync_window_region
 *
 * Update the X11 window region.
 */
static void sync_window_region( struct x11drv_win_data *data, HRGN win_region )
{
#ifdef HAVE_LIBXSHAPE
    HRGN hrgn = win_region;

    if (!data->whole_window) return;
    if (client_side_graphics) return; /* use surface shape instead */
    data->shaped = FALSE;

    if (hrgn == (HRGN)1)  /* hack: win_region == 1 means retrieve region from server */
    {
        if (!(hrgn = NtGdiCreateRectRgn( 0, 0, 0, 0 ))) return;
        if (NtUserGetWindowRgnEx( data->hwnd, hrgn, 0 ) == ERROR)
        {
            NtGdiDeleteObjectApp( hrgn );
            hrgn = 0;
        }
    }

    if (!hrgn)
    {
        XShapeCombineMask( data->display, data->whole_window, ShapeBounding, 0, 0, None, ShapeSet );
    }
    else
    {
        RGNDATA *pRegionData;

        if (NtUserGetWindowLongW( data->hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
            NtUserMirrorRgn( data->hwnd, hrgn );
        if ((pRegionData = X11DRV_GetRegionData( hrgn, 0 )))
        {
            XShapeCombineRectangles( data->display, data->whole_window, ShapeBounding,
                                     data->rects.window.left - data->rects.visible.left,
                                     data->rects.window.top - data->rects.visible.top,
                                     (XRectangle *)pRegionData->Buffer,
                                     pRegionData->rdh.nCount, ShapeSet, YXBanded );
            free( pRegionData );
            data->shaped = TRUE;
        }
    }
    if (hrgn && hrgn != win_region) NtGdiDeleteObjectApp( hrgn );
#endif  /* HAVE_LIBXSHAPE */
}


/***********************************************************************
 *              sync_window_opacity
 */
static void sync_window_opacity( Display *display, Window win, BYTE alpha, DWORD flags )
{
    unsigned long opacity = 0xffffffff;

    if (flags & LWA_ALPHA) opacity = (0xffffffff / 0xff) * alpha;

    if (opacity == 0xffffffff)
        XDeleteProperty( display, win, x11drv_atom(_NET_WM_WINDOW_OPACITY) );
    else
        XChangeProperty( display, win, x11drv_atom(_NET_WM_WINDOW_OPACITY),
                         XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&opacity, 1 );
}


/***********************************************************************
 *              sync_window_text
 */
static void sync_window_text( Display *display, Window win, const WCHAR *text )
{
    DWORD count, len;
    char *buffer, *utf8_buffer;
    XTextProperty prop;

    /* allocate new buffer for window text */
    len = lstrlenW( text );
    count = len * 3 + 1;
    if (!(buffer = malloc( count ))) return;
    ntdll_wcstoumbs( text, len + 1, buffer, count, FALSE );

    RtlUnicodeToUTF8N( NULL, 0, &count, text, len * sizeof(WCHAR) );
    if (!(utf8_buffer = malloc( count )))
    {
        free( buffer );
        return;
    }
    RtlUnicodeToUTF8N( utf8_buffer, count, &count, text, len * sizeof(WCHAR) );

    if (XmbTextListToTextProperty( display, &buffer, 1, XStdICCTextStyle, &prop ) == Success)
    {
        XSetWMName( display, win, &prop );
        XSetWMIconName( display, win, &prop );
        XFree( prop.value );
    }
    /*
      Implements a NET_WM UTF-8 title. It should be without a trailing \0,
      according to the standard
      ( http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text ).
    */
    XChangeProperty( display, win, x11drv_atom(_NET_WM_NAME), x11drv_atom(UTF8_STRING),
                     8, PropModeReplace, (unsigned char *) utf8_buffer, count);

    free( utf8_buffer );
    free( buffer );
}


/***********************************************************************
 *              get_bitmap_argb
 *
 * Return the bitmap bits in ARGB format. Helper for setting icon hints.
 */
static unsigned long *get_bitmap_argb( HDC hdc, HBITMAP color, HBITMAP mask, unsigned int *size )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    BITMAP bm;
    unsigned long *bits = NULL;
    unsigned int *ptr;
    unsigned char *mask_bits = NULL;
    int i, j;
    BOOL has_alpha = FALSE;

    if (!NtGdiExtGetObjectW( color, sizeof(bm), &bm )) return NULL;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = bm.bmWidth;
    info->bmiHeader.biHeight = -bm.bmHeight;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    *size = bm.bmWidth * bm.bmHeight + 2;
    if (!(bits = malloc( *size * sizeof(*bits) ))) goto failed;
    ptr = (unsigned int *)bits;
    if (!NtGdiGetDIBitsInternal( hdc, color, 0, bm.bmHeight, ptr + 2, info, DIB_RGB_COLORS, 0, 0 ))
        goto failed;

    ptr[0] = bm.bmWidth;
    ptr[1] = bm.bmHeight;

    for (i = 0; i < bm.bmWidth * bm.bmHeight; i++)
        if ((has_alpha = (ptr[i + 2] & 0xff000000) != 0)) break;

    if (!has_alpha)
    {
        unsigned int width_bytes = (bm.bmWidth + 31) / 32 * 4;
        /* generate alpha channel from the mask */
        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * bm.bmHeight;
        if (!(mask_bits = malloc( info->bmiHeader.biSizeImage ))) goto failed;
        if (!NtGdiGetDIBitsInternal( hdc, mask, 0, bm.bmHeight, mask_bits, info, DIB_RGB_COLORS, 0, 0 ))
            goto failed;
        ptr = (unsigned int *)bits + 2;
        for (i = 0; i < bm.bmHeight; i++)
            for (j = 0; j < bm.bmWidth; j++, ptr++)
                if (!((mask_bits[i * width_bytes + j / 8] << (j % 8)) & 0x80)) *ptr |= 0xff000000;
        free( mask_bits );
    }

    /* convert to array of longs */
    if (bits && sizeof(long) > sizeof(int))
    {
        ptr = (unsigned int *)bits;
        for (i = *size - 1; i >= 0; i--) bits[i] = ptr[i];
    }
    return bits;

failed:
    free( bits );
    free( mask_bits );
    return NULL;
}


/***********************************************************************
 *              create_icon_pixmaps
 */
static BOOL create_icon_pixmaps( HDC hdc, const ICONINFO *icon, Pixmap *icon_ret, Pixmap *mask_ret )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    XVisualInfo vis = default_visual;
    struct gdi_image_bits bits;
    Pixmap color_pixmap = 0, mask_pixmap = 0;
    int lines;
    unsigned int i;

    bits.ptr = NULL;
    bits.free = NULL;
    bits.is_copy = TRUE;

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biBitCount = 0;
    if (!(lines = NtGdiGetDIBitsInternal( hdc, icon->hbmColor, 0, 0, NULL, info, DIB_RGB_COLORS, 0, 0 )))
        goto failed;
    if (!(bits.ptr = malloc( info->bmiHeader.biSizeImage ))) goto failed;
    if (!NtGdiGetDIBitsInternal( hdc, icon->hbmColor, 0, lines, bits.ptr, info, DIB_RGB_COLORS, 0, 0 ))
        goto failed;

    color_pixmap = create_pixmap_from_image( hdc, &vis, info, &bits, DIB_RGB_COLORS );
    free( bits.ptr );
    bits.ptr = NULL;
    if (!color_pixmap) goto failed;

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biBitCount = 0;
    if (!(lines = NtGdiGetDIBitsInternal( hdc, icon->hbmMask, 0, 0, NULL, info, DIB_RGB_COLORS, 0, 0 )))
        goto failed;
    if (!(bits.ptr = malloc( info->bmiHeader.biSizeImage ))) goto failed;
    if (!NtGdiGetDIBitsInternal( hdc, icon->hbmMask, 0, lines, bits.ptr, info, DIB_RGB_COLORS, 0, 0 ))
        goto failed;

    /* invert the mask */
    for (i = 0; i < info->bmiHeader.biSizeImage / sizeof(DWORD); i++) ((DWORD *)bits.ptr)[i] ^= ~0u;

    vis.depth = 1;
    mask_pixmap = create_pixmap_from_image( hdc, &vis, info, &bits, DIB_RGB_COLORS );
    free( bits.ptr );
    bits.ptr = NULL;
    if (!mask_pixmap) goto failed;

    *icon_ret = color_pixmap;
    *mask_ret = mask_pixmap;
    return TRUE;

failed:
    if (color_pixmap) XFreePixmap( gdi_display, color_pixmap );
    free( bits.ptr );
    return FALSE;
}


static HICON get_icon_info( HICON icon, ICONINFO *ii )
{
    return icon && NtUserGetIconInfo( icon, ii, NULL, NULL, NULL, 0 ) ? icon : NULL;
}

/***********************************************************************
 *              fetch_icon_data
 */
static void fetch_icon_data( HWND hwnd, HICON icon_big, HICON icon_small )
{
    struct x11drv_win_data *data;
    ICONINFO ii, ii_small;
    HDC hDC;
    unsigned int size;
    unsigned long *bits;
    Pixmap icon_pixmap, mask_pixmap;

    icon_big = get_icon_info( icon_big, &ii );
    if (!icon_big)
    {
        icon_big = get_icon_info( (HICON)send_message( hwnd, WM_GETICON, ICON_BIG, 0 ), &ii );
        if (!icon_big)
            icon_big = get_icon_info( (HICON)NtUserGetClassLongPtrW( hwnd, GCLP_HICON ), &ii );
        if (!icon_big)
        {
            icon_big = LoadImageW( 0, (const WCHAR *)IDI_WINLOGO, IMAGE_ICON, 0, 0,
                                   LR_SHARED | LR_DEFAULTSIZE );
            icon_big = get_icon_info( icon_big, &ii );
        }
    }

    icon_small = get_icon_info( icon_small, &ii_small );
    if (!icon_small)
    {
        icon_small = get_icon_info( (HICON)send_message( hwnd, WM_GETICON, ICON_SMALL, 0 ), &ii_small );
        if (!icon_small)
            icon_small = get_icon_info( (HICON)NtUserGetClassLongPtrW( hwnd, GCLP_HICONSM ), &ii_small );
    }

    if (!icon_big) return;

    hDC = NtGdiCreateCompatibleDC(0);
    bits = get_bitmap_argb( hDC, ii.hbmColor, ii.hbmMask, &size );
    if (bits && icon_small)
    {
        unsigned int size_small;
        unsigned long *bits_small, *new;

        if ((bits_small = get_bitmap_argb( hDC, ii_small.hbmColor, ii_small.hbmMask, &size_small )) &&
            (bits_small[0] != bits[0] || bits_small[1] != bits[1]))  /* size must be different */
        {
            if ((new = realloc( bits, (size + size_small) * sizeof(unsigned long) )))
            {
                bits = new;
                memcpy( bits + size, bits_small, size_small * sizeof(unsigned long) );
                size += size_small;
            }
        }
        free( bits_small );
        NtGdiDeleteObjectApp( ii_small.hbmColor );
        NtGdiDeleteObjectApp( ii_small.hbmMask );
    }

    if (!create_icon_pixmaps( hDC, &ii, &icon_pixmap, &mask_pixmap )) icon_pixmap = mask_pixmap = 0;

    NtGdiDeleteObjectApp( ii.hbmColor );
    NtGdiDeleteObjectApp( ii.hbmMask );
    NtGdiDeleteObjectApp( hDC );

    if ((data = get_win_data( hwnd )))
    {
        if (data->icon_pixmap) XFreePixmap( gdi_display, data->icon_pixmap );
        if (data->icon_mask) XFreePixmap( gdi_display, data->icon_mask );
        free( data->icon_bits );
        data->icon_pixmap = icon_pixmap;
        data->icon_mask = mask_pixmap;
        data->icon_bits = bits;
        data->icon_size = size;
        release_win_data( data );
    }
    else
    {
        if (icon_pixmap) XFreePixmap( gdi_display, icon_pixmap );
        if (mask_pixmap) XFreePixmap( gdi_display, mask_pixmap );
        free( bits );
    }
}


/***********************************************************************
 *              set_size_hints
 *
 * set the window size hints
 */
static void set_size_hints( struct x11drv_win_data *data, DWORD style )
{
    XSizeHints* size_hints;

    if (!(size_hints = XAllocSizeHints())) return;

    size_hints->win_gravity = StaticGravity;
    size_hints->flags |= PWinGravity;

    /* don't update size hints if window is not in normal state */
    if (!(style & (WS_MINIMIZE | WS_MAXIMIZE)))
    {
        if (data->hwnd != NtUserGetDesktopWindow())  /* don't force position of desktop */
        {
            POINT pt = virtual_screen_to_root( data->rects.visible.left, data->rects.visible.top );
            size_hints->x = pt.x;
            size_hints->y = pt.y;
            size_hints->flags |= PPosition;
        }
        else size_hints->win_gravity = NorthWestGravity;

        if (!is_window_resizable( data, style ))
        {
            size_hints->max_width = data->rects.visible.right - data->rects.visible.left;
            size_hints->max_height = data->rects.visible.bottom - data->rects.visible.top;
            if (size_hints->max_width <= 0 ||size_hints->max_height <= 0)
                size_hints->max_width = size_hints->max_height = 1;
            size_hints->min_width = size_hints->max_width;
            size_hints->min_height = size_hints->max_height;
            size_hints->flags |= PMinSize | PMaxSize;
        }
    }
    XSetWMNormalHints( data->display, data->whole_window, size_hints );
    XFree( size_hints );
}


/***********************************************************************
 *              set_mwm_hints
 */
static void set_mwm_hints( struct x11drv_win_data *data, UINT style, UINT ex_style )
{
    MwmHints mwm_hints;

    if (data->hwnd == NtUserGetDesktopWindow())
    {
        mwm_hints.functions        = MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE | MWM_FUNC_CLOSE;
        if (is_desktop_fullscreen())
        {
            mwm_hints.decorations = 0;
            mwm_hints.functions |= MWM_FUNC_RESIZE;  /* some WMs need this to make it fullscreen */
        }
        else mwm_hints.decorations = MWM_DECOR_TITLE | MWM_DECOR_BORDER | MWM_DECOR_MENU | MWM_DECOR_MINIMIZE;
    }
    else
    {
        mwm_hints.decorations = get_mwm_decorations( data, style, ex_style );
        mwm_hints.functions = MWM_FUNC_MOVE;
        if (is_window_resizable( data, style )) mwm_hints.functions |= MWM_FUNC_RESIZE;
        if (!(style & WS_DISABLED))
        {
            mwm_hints.functions |= MWM_FUNC_CLOSE;
            if (style & WS_MINIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MINIMIZE;
            if (style & WS_MAXIMIZEBOX) mwm_hints.functions |= MWM_FUNC_MAXIMIZE;

            /* The window can be programmatically minimized even without
               a minimize box button. Allow the WM to restore it. */
            if (style & WS_MINIMIZE)    mwm_hints.functions |= MWM_FUNC_MINIMIZE | MWM_FUNC_MAXIMIZE;
            /* The window can be programmatically maximized even without
               a maximize box button. Allow the WM to maximize it. */
            if (style & WS_MAXIMIZE)    mwm_hints.functions |= MWM_FUNC_MAXIMIZE;
        }
    }

    TRACE( "%p setting mwm hints to %lx,%lx (style %x exstyle %x)\n",
           data->hwnd, mwm_hints.decorations, mwm_hints.functions, style, ex_style );

    mwm_hints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;
    mwm_hints.input_mode = 0;
    mwm_hints.status = 0;
    XChangeProperty( data->display, data->whole_window, x11drv_atom(_MOTIF_WM_HINTS),
                     x11drv_atom(_MOTIF_WM_HINTS), 32, PropModeReplace,
                     (unsigned char*)&mwm_hints, sizeof(mwm_hints)/sizeof(long) );
}


/***********************************************************************
 *              set_style_hints
 */
static void set_style_hints( struct x11drv_win_data *data, DWORD style, DWORD ex_style )
{
    Window group_leader = data->whole_window;
    HWND owner = NtUserGetWindowRelative( data->hwnd, GW_OWNER );
    Window owner_win = 0;
    XWMHints *wm_hints;
    Atom window_type;

    if (owner)
    {
        owner = NtUserGetAncestor( owner, GA_ROOT );
        owner_win = X11DRV_get_whole_window( owner );
    }

    if (owner_win)
    {
        XSetTransientForHint( data->display, data->whole_window, owner_win );
        group_leader = owner_win;
    }

    /* Only use dialog type for owned popups. Metacity allows making fullscreen
     * only normal windows, and doesn't handle correctly TRANSIENT_FOR hint for
     * dialogs owned by fullscreen windows.
     */
    if (((style & WS_POPUP) || (ex_style & WS_EX_DLGMODALFRAME)) && owner)
        window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_DIALOG);
    else
        window_type = x11drv_atom(_NET_WM_WINDOW_TYPE_NORMAL);

    XChangeProperty(data->display, data->whole_window, x11drv_atom(_NET_WM_WINDOW_TYPE),
		    XA_ATOM, 32, PropModeReplace, (unsigned char*)&window_type, 1);

    if ((wm_hints = XAllocWMHints()))
    {
        wm_hints->flags = InputHint | StateHint | WindowGroupHint;
        wm_hints->input = !use_take_focus && !(style & WS_DISABLED);
        wm_hints->initial_state = (style & WS_MINIMIZE) ? IconicState : NormalState;
        wm_hints->window_group = group_leader;
        if (data->icon_pixmap)
        {
            wm_hints->icon_pixmap = data->icon_pixmap;
            wm_hints->icon_mask = data->icon_mask;
            wm_hints->flags |= IconPixmapHint | IconMaskHint;
        }
        XSetWMHints( data->display, data->whole_window, wm_hints );
        XFree( wm_hints );
    }

    if (data->icon_bits)
        XChangeProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_ICON),
                         XA_CARDINAL, 32, PropModeReplace,
                         (unsigned char *)data->icon_bits, data->icon_size );
    else
        XDeleteProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_ICON) );

}


/***********************************************************************
 *              set_initial_wm_hints
 *
 * Set the window manager hints that don't change over the lifetime of a window.
 */
static void set_initial_wm_hints( Display *display, Window window )
{
    long i;
    Atom protocols[3];
    Atom dndVersion = WINE_XDND_VERSION;
    XClassHint *class_hints;

    /* wm protocols */
    i = 0;
    protocols[i++] = x11drv_atom(WM_DELETE_WINDOW);
    protocols[i++] = x11drv_atom(_NET_WM_PING);
    if (use_take_focus) protocols[i++] = x11drv_atom(WM_TAKE_FOCUS);
    XChangeProperty( display, window, x11drv_atom(WM_PROTOCOLS),
                     XA_ATOM, 32, PropModeReplace, (unsigned char *)protocols, i );

    /* class hints */
    if ((class_hints = XAllocClassHint()))
    {
        class_hints->res_name = process_name;
        class_hints->res_class = process_name;
        XSetClassHint( display, window, class_hints );
        XFree( class_hints );
    }

    /* set the WM_CLIENT_MACHINE and WM_LOCALE_NAME properties */
    XSetWMProperties(display, window, NULL, NULL, NULL, 0, NULL, NULL, NULL);
    /* set the pid. together, these properties are needed so the window manager can kill us if we freeze */
    i = getpid();
    XChangeProperty(display, window, x11drv_atom(_NET_WM_PID),
                    XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&i, 1);

    XChangeProperty( display, window, x11drv_atom(XdndAware),
                     XA_ATOM, 32, PropModeReplace, (unsigned char*)&dndVersion, 1 );
}


/***********************************************************************
 *              make_owner_managed
 *
 * If the window is managed, make sure its owner window is too.
 */
static void make_owner_managed( HWND hwnd )
{
    static const UINT flags = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW |
                              SWP_DEFERERASE | SWP_NOSENDCHANGING | SWP_STATECHANGED;
    HWND owner;

    if (!(owner = NtUserGetWindowRelative( hwnd, GW_OWNER ))) return;
    if (is_managed( owner )) return;
    if (!is_managed( hwnd )) return;

    NtUserSetWindowPos( owner, 0, 0, 0, 0, 0, flags );
}


/***********************************************************************
 *              set_wm_hints
 *
 * Set all the window manager hints for a window.
 */
static void set_wm_hints( struct x11drv_win_data *data )
{
    DWORD style, ex_style;

    if (data->hwnd == NtUserGetDesktopWindow())
    {
        /* force some styles for the desktop to get the correct decorations */
        style = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        ex_style = WS_EX_APPWINDOW;
    }
    else
    {
        style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE );
        ex_style = NtUserGetWindowLongW( data->hwnd, GWL_EXSTYLE );
    }

    set_size_hints( data, style );
    set_mwm_hints( data, style, ex_style );
    set_style_hints( data, style, ex_style );
}


/***********************************************************************
 *     init_clip_window
 */
Window init_clip_window(void)
{
    struct x11drv_thread_data *data = x11drv_init_thread_data();

    if (!data->clip_window &&
        (data->clip_window = (Window)NtUserGetProp( NtUserGetDesktopWindow(), clip_window_prop )))
    {
        XSelectInput( data->display, data->clip_window, StructureNotifyMask );
    }
    return data->clip_window;
}


/***********************************************************************
 *     window_set_user_time
 */
void window_set_user_time( struct x11drv_win_data *data, Time time, BOOL init )
{
    if (init && data->managed) NtUserSetProp( data->hwnd, focus_time_prop, (HANDLE)time );
    else if (!init && !time) time = 1; /* time == 0 has reserved semantics */

    if (init && !data->user_time == !time) return;
    if (!init && data->user_time == time) return;
    data->user_time = time;

    TRACE( "window %p/%lx, requesting _NET_WM_USER_TIME %ld serial %lu\n", data->hwnd, data->whole_window,
           data->user_time, NextRequest( data->display ) );
    if (init && time) XDeleteProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_USER_TIME) );
    else XChangeProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_USER_TIME), XA_CARDINAL,
                          32, PropModeReplace, (unsigned char *)&time, 1 );
}

/* Update _NET_WM_FULLSCREEN_MONITORS when _NET_WM_STATE_FULLSCREEN is set to support fullscreen
 * windows spanning multiple monitors */
static void update_net_wm_fullscreen_monitors( struct x11drv_win_data *data )
{
    long monitors[4];
    XEvent xev;

    if (!(data->pending_state.net_wm_state & (1 << NET_WM_STATE_FULLSCREEN)) || is_virtual_desktop()
        || NtUserGetWindowLongW( data->hwnd, GWL_STYLE ) & WS_MINIMIZE)
        return;

    /* If the current display device handler cannot detect dynamic device changes, do not use
     * _NET_WM_FULLSCREEN_MONITORS because xinerama_get_fullscreen_monitors() may report wrong
     * indices because of stale xinerama monitor information */
    if (!X11DRV_DisplayDevices_SupportEventHandlers())
        return;

    xinerama_get_fullscreen_monitors( &data->rects.visible, monitors );

    if (data->pending_state.wm_state == WithdrawnState)
    {
        TRACE( "window %p/%lx, requesting _NET_WM_FULLSCREEN_MONITORS %ld,%ld,%ld,%ld serial %lu\n", data->hwnd, data->whole_window,
               monitors[0], monitors[1], monitors[2], monitors[3], NextRequest( data->display ) );
        if (monitors[0] == -1) XDeleteProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_FULLSCREEN_MONITORS) );
        else XChangeProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_FULLSCREEN_MONITORS),
                              XA_CARDINAL, 32, PropModeReplace, (unsigned char *)monitors, 4 );
    }
    else
    {
        xev.xclient.type = ClientMessage;
        xev.xclient.window = data->whole_window;
        xev.xclient.message_type = x11drv_atom(_NET_WM_FULLSCREEN_MONITORS);
        xev.xclient.serial = 0;
        xev.xclient.display = data->display;
        xev.xclient.send_event = True;
        xev.xclient.format = 32;
        xev.xclient.data.l[4] = 1;
        memcpy( xev.xclient.data.l, monitors, sizeof(monitors) );

        TRACE( "window %p/%lx, requesting _NET_WM_FULLSCREEN_MONITORS %ld,%ld,%ld,%ld serial %lu\n", data->hwnd, data->whole_window,
               monitors[0], monitors[1], monitors[2], monitors[3], NextRequest( data->display ) );
        XSendEvent( data->display, DefaultRootWindow( data->display ), False,
                    SubstructureRedirectMask | SubstructureNotifyMask, &xev );
    }
}

static void window_set_net_wm_state( struct x11drv_win_data *data, UINT new_state )
{
    UINT i, count, old_state = data->pending_state.net_wm_state;

    new_state &= x11drv_thread_data()->net_wm_state_mask;
    data->desired_state.net_wm_state = new_state;
    if (!data->whole_window) return; /* no window, nothing to update */
    if (data->wm_state_serial) return; /* another WM_STATE update is pending, wait for it to complete */
    /* we ignore and override previous _NET_WM_STATE update requests */
    if (old_state == new_state) return; /* states are the same, nothing to update */

    if (data->pending_state.wm_state == IconicState) return; /* window is iconic, don't update its state now */
    if (data->pending_state.wm_state == WithdrawnState)  /* set the _NET_WM_STATE atom directly */
    {
        Atom atoms[NB_NET_WM_STATES + 1];

        for (i = count = 0; i < NB_NET_WM_STATES; i++)
        {
            if (!(new_state & (1 << i))) continue;
            atoms[count++] = X11DRV_Atoms[net_wm_state_atoms[i] - FIRST_XATOM];
            if (net_wm_state_atoms[i] == XATOM__NET_WM_STATE_MAXIMIZED_VERT)
                atoms[count++] = x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ);
        }

        data->pending_state.net_wm_state = new_state;
        data->net_wm_state_serial = NextRequest( data->display );
        TRACE( "window %p/%lx, requesting _NET_WM_STATE %#x serial %lu\n", data->hwnd, data->whole_window,
               data->pending_state.net_wm_state, data->net_wm_state_serial );
        XChangeProperty( data->display, data->whole_window, x11drv_atom(_NET_WM_STATE), XA_ATOM,
                         32, PropModeReplace, (unsigned char *)atoms, count );
    }
    else  /* ask the window manager to do it for us */
    {
        XEvent xev;

        xev.xclient.type = ClientMessage;
        xev.xclient.window = data->whole_window;
        xev.xclient.message_type = x11drv_atom(_NET_WM_STATE);
        xev.xclient.serial = 0;
        xev.xclient.display = data->display;
        xev.xclient.send_event = True;
        xev.xclient.format = 32;
        xev.xclient.data.l[3] = 1;
        xev.xclient.data.l[4] = 0;

        for (i = 0; i < NB_NET_WM_STATES; i++)
        {
            if (!((old_state ^ new_state) & (1 << i))) continue;

            xev.xclient.data.l[0] = (new_state & (1 << i)) ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
            xev.xclient.data.l[1] = X11DRV_Atoms[net_wm_state_atoms[i] - FIRST_XATOM];
            xev.xclient.data.l[2] = ((net_wm_state_atoms[i] == XATOM__NET_WM_STATE_MAXIMIZED_VERT) ?
                                     x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ) : 0);

            data->pending_state.net_wm_state = new_state;
            data->net_wm_state_serial = NextRequest( data->display );
            TRACE( "window %p/%lx, requesting _NET_WM_STATE %#x serial %lu\n", data->hwnd, data->whole_window,
                   data->pending_state.net_wm_state, data->net_wm_state_serial );
            XSendEvent( data->display, DefaultRootWindow( data->display ), False,
                        SubstructureRedirectMask | SubstructureNotifyMask, &xev );
        }
    }

    XFlush( data->display );
}

static void window_set_config( struct x11drv_win_data *data, const RECT *new_rect, BOOL above )
{
    static const UINT fullscreen_mask = (1 << NET_WM_STATE_MAXIMIZED) | (1 << NET_WM_STATE_FULLSCREEN);
    UINT style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE ), mask = 0;
    const RECT *old_rect = &data->pending_state.rect;
    XWindowChanges changes;

    data->desired_state.rect = *new_rect;
    if (!data->whole_window) return; /* no window, nothing to update */
    if (EqualRect( old_rect, new_rect ) && !above) return; /* rects are the same, no need to be raised, nothing to update */

    if (data->pending_state.wm_state == NormalState && data->net_wm_state_serial &&
        !(data->pending_state.net_wm_state & fullscreen_mask) &&
        (data->current_state.net_wm_state & fullscreen_mask))
    {
        /* Some window managers are sending a ConfigureNotify event with the fullscreen size when
         * exiting a fullscreen window, with a serial that we cannot predict. Handling that event
         * will override the Win32 window size and make the window fullscreen again.
         */
        WARN( "window %p/%lx is exiting maximize/fullscreen, delaying request\n", data->hwnd, data->whole_window );
        return;
    }

    /* resizing a managed maximized window is not allowed */
    if ((old_rect->right - old_rect->left != new_rect->right - new_rect->left ||
         old_rect->bottom - old_rect->top != new_rect->bottom - new_rect->top) &&
        (!(style & WS_MAXIMIZE) || !data->managed))
    {
        changes.width = new_rect->right - new_rect->left;
        changes.height = new_rect->bottom - new_rect->top;
        /* if window rect is empty force size to 1x1 */
        if (changes.width <= 0 || changes.height <= 0) changes.width = changes.height = 1;
        if (changes.width > 65535) changes.width = 65535;
        if (changes.height > 65535) changes.height = 65535;
        mask |= CWWidth | CWHeight;
    }

    /* only the size is allowed to change for the desktop window or systray docked windows */
    if ((old_rect->left != new_rect->left || old_rect->top != new_rect->top) &&
        (data->whole_window != root_window && !data->embedded))
    {
        POINT pt = virtual_screen_to_root( new_rect->left, new_rect->top );
        changes.x = pt.x;
        changes.y = pt.y;
        mask |= CWX | CWY;
    }

    if (above)
    {
        changes.stack_mode = Above;
        mask |= CWStackMode;
    }

    data->pending_state.rect = *new_rect;
    data->configure_serial = NextRequest( data->display );
    TRACE( "window %p/%lx, requesting config %s mask %#x above %u, serial %lu\n", data->hwnd, data->whole_window,
           wine_dbgstr_rect(new_rect), mask, above, data->configure_serial );
    XReconfigureWMWindow( data->display, data->whole_window, data->vis.screen, mask, &changes );
}

/***********************************************************************
 *     update_net_wm_states
 */
static void update_net_wm_states( struct x11drv_win_data *data )
{
    static const UINT fullscreen_mask = (1 << NET_WM_STATE_MAXIMIZED) | (1 << NET_WM_STATE_FULLSCREEN);
    UINT style, ex_style, new_state = 0;

    if (!data->managed || data->embedded) return;
    if (data->whole_window == root_window)
    {
        if (is_virtual_desktop()) window_set_net_wm_state( data, is_desktop_fullscreen() ? (1 << NET_WM_STATE_FULLSCREEN) : 0 );
        return;
    }

    style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE );
    if (style & WS_MINIMIZE) new_state |= data->desired_state.net_wm_state & fullscreen_mask;
    if (data->is_fullscreen)
    {
        if ((style & WS_MAXIMIZE) && (style & WS_CAPTION) == WS_CAPTION)
            new_state |= (1 << NET_WM_STATE_MAXIMIZED);
        else if (!(style & WS_MINIMIZE))
            new_state |= (1 << NET_WM_STATE_FULLSCREEN);
    }
    else if (style & WS_MAXIMIZE)
        new_state |= (1 << NET_WM_STATE_MAXIMIZED);

    ex_style = NtUserGetWindowLongW( data->hwnd, GWL_EXSTYLE );
    if (ex_style & WS_EX_TOPMOST)
        new_state |= (1 << NET_WM_STATE_ABOVE);
    if (!data->add_taskbar)
    {
        if (data->skip_taskbar || (ex_style & WS_EX_NOACTIVATE)
            || (ex_style & WS_EX_TOOLWINDOW && !(ex_style & WS_EX_APPWINDOW)))
            new_state |= (1 << NET_WM_STATE_SKIP_TASKBAR) | (1 << NET_WM_STATE_SKIP_PAGER) | (1 << KDE_NET_WM_STATE_SKIP_SWITCHER);
        else if (!(ex_style & WS_EX_APPWINDOW) && NtUserGetWindowRelative( data->hwnd, GW_OWNER ))
            new_state |= (1 << NET_WM_STATE_SKIP_TASKBAR);
    }

    window_set_net_wm_state( data, new_state );
    update_net_wm_fullscreen_monitors( data );
}

/***********************************************************************
 *     read_net_wm_states
 */
UINT get_window_net_wm_state( Display *display, Window window )
{
    Atom type, *state;
    int format;
    unsigned long i, j, count, remaining;
    UINT new_state = 0;
    BOOL maximized_horz = FALSE;

    if (!XGetWindowProperty( display, window, x11drv_atom(_NET_WM_STATE), 0,
                             65536/sizeof(CARD32), False, XA_ATOM, &type, &format, &count,
                             &remaining, (unsigned char **)&state ))
    {
        if (type == XA_ATOM && format == 32)
        {
            for (i = 0; i < count; i++)
            {
                if (state[i] == x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ))
                    maximized_horz = TRUE;
                for (j=0; j < NB_NET_WM_STATES; j++)
                {
                    if (state[i] == X11DRV_Atoms[net_wm_state_atoms[j] - FIRST_XATOM])
                    {
                        new_state |= 1 << j;
                    }
                }
            }
        }
        XFree( state );
    }

    if (!maximized_horz)
        new_state &= ~(1 << NET_WM_STATE_MAXIMIZED);

    return new_state;
}


/***********************************************************************
 *     set_xembed_flags
 */
static void set_xembed_flags( struct x11drv_win_data *data, unsigned long flags )
{
    unsigned long info[2];

    info[0] = 0; /* protocol version */
    info[1] = flags;
    XChangeProperty( data->display, data->whole_window, x11drv_atom(_XEMBED_INFO),
                     x11drv_atom(_XEMBED_INFO), 32, PropModeReplace, (unsigned char*)info, 2 );
}

static void window_set_wm_state( struct x11drv_win_data *data, UINT new_state, BOOL activate )
{
    UINT old_state = data->pending_state.wm_state;
    HWND foreground = NtUserGetForegroundWindow();

    data->desired_state.wm_state = new_state;
    data->desired_state.activate = activate;
    if (!data->whole_window) return; /* no window, nothing to update */
    if (data->wm_state_serial && !data->current_state.wm_state != !data->pending_state.wm_state)
        return; /* another map/unmap WM_STATE update is pending, wait for it to complete */
    if (old_state == new_state) return; /* states are the same, nothing to update */

    /* When transitioning a window from IconicState to NormalState and the window is managed, go
     * through WithdrawnState. This is needed because Mutter doesn't unmap windows when making
     * windows iconic/minimized as Mutter needs to support live preview for minimized windows. So on
     * Mutter, a window can be both iconic and mapped. If the window is mapped, then XMapWindow()
     * will have no effect according to the  XMapWindow() documentation. Thus we have to transition
     * to WithdrawnState first, then to NormalState */
    if (data->managed && MAKELONG(old_state, new_state) == MAKELONG(IconicState, NormalState))
    {
        WARN( "window %p/%lx is iconic, remapping to workaround Mutter issues.\n", data->hwnd, data->whole_window );
        window_set_wm_state( data, WithdrawnState, FALSE );
        window_set_wm_state( data, NormalState, activate );
        return;
    }

    switch (MAKELONG(old_state, new_state))
    {
    case MAKELONG(WithdrawnState, IconicState):
    case MAKELONG(WithdrawnState, NormalState):
        remove_startup_notification( data );
        set_wm_hints( data );
        update_net_wm_states( data );
        sync_window_style( data );
        update_net_wm_fullscreen_monitors( data );
        break;
    case MAKELONG(IconicState, NormalState):
    case MAKELONG(NormalState, IconicState):
        set_wm_hints( data );
        break;
    }

    if (new_state == NormalState)
    {
        /* try forcing activation if the window is supposed to be foreground or if it is fullscreen */
        if (data->hwnd == foreground || data->is_fullscreen) activate = TRUE;
        window_set_user_time( data, activate ? -1 : 0, TRUE );
    }

    data->pending_state.wm_state = new_state;
    data->pending_state.activate = activate;
    data->wm_state_serial = NextRequest( data->display );
    TRACE( "window %p/%lx, requesting WM_STATE %#x -> %#x serial %lu, foreground %p, activate %u\n", data->hwnd, data->whole_window,
           old_state, new_state, data->wm_state_serial, NtUserGetForegroundWindow(), activate );

    switch (MAKELONG(old_state, new_state))
    {
    case MAKELONG(WithdrawnState, IconicState):
    case MAKELONG(WithdrawnState, NormalState):
    case MAKELONG(IconicState, NormalState):
        if (data->embedded) set_xembed_flags( data, XEMBED_MAPPED );
        else XMapWindow( data->display, data->whole_window );
        break;
    case MAKELONG(NormalState, WithdrawnState):
    case MAKELONG(IconicState, WithdrawnState):
        if (data->embedded) set_xembed_flags( data, 0 );
        else if (!data->managed) XUnmapWindow( data->display, data->whole_window );
        else XWithdrawWindow( data->display, data->whole_window, data->vis.screen );
        break;
    case MAKELONG(NormalState, IconicState):
        if (!data->embedded) XIconifyWindow( data->display, data->whole_window, data->vis.screen );
        break;
    }

    /* override redirect windows won't receive WM_STATE property changes */
    if (!data->managed) data->wm_state_serial = 0;

    XFlush( data->display );
}

static void window_set_managed( struct x11drv_win_data *data, BOOL new_managed )
{
    XSetWindowAttributes attr = {.override_redirect = !new_managed};
    UINT wm_state = data->desired_state.wm_state, activate = data->desired_state.activate;
    BOOL old_managed = data->managed;

    if (!data->whole_window) return; /* no window, nothing to update */
    if (old_managed == new_managed) return; /* states are the same, nothing to update */
    if (!new_managed)
    {
        ERR( "Changing window to unmanaged is not supported\n" );
        return;
    }

    window_set_wm_state( data, WithdrawnState, FALSE ); /* no WM_STATE is pending, requested immediately */

    data->managed = new_managed;
    TRACE( "window %p/%lx, requesting override-redirect %u -> %u serial %lu\n", data->hwnd, data->whole_window,
           !old_managed, !new_managed, NextRequest( data->display ) );
    XChangeWindowAttributes( data->display, data->whole_window, CWOverrideRedirect, &attr );

    window_set_wm_state( data, wm_state, activate ); /* queue another WM_STATE request with the desired state */
}


/***********************************************************************
 *     map_window
 */
static void map_window( HWND hwnd, DWORD new_style, BOOL activate )
{
    struct x11drv_win_data *data;

    make_owner_managed( hwnd );

    if (!(data = get_win_data( hwnd ))) return;
    TRACE( "win %p/%lx\n", data->hwnd, data->whole_window );
    window_set_wm_state( data, (new_style & WS_MINIMIZE) ? IconicState : NormalState, activate );
    release_win_data( data );
}

static UINT window_update_client_state( struct x11drv_win_data *data )
{
    UINT old_style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE ), new_style;

    if (!data->managed) return 0; /* unmanaged windows are managed by the Win32 side */
    if (data->desired_state.wm_state == WithdrawnState) return 0; /* ignore state changes on invisible windows */

    if (data->wm_state_serial) return 0; /* another WM_STATE update is pending, wait for it to complete */
    if (data->net_wm_state_serial) return 0; /* another _NET_WM_STATE update is pending, wait for it to complete */
    if (data->configure_serial) return 0; /* another config update is pending, wait for it to complete */

    new_style = old_style & ~(WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE);
    if (data->current_state.wm_state != WithdrawnState) new_style |= WS_VISIBLE;
    if (data->current_state.wm_state == IconicState) new_style |= WS_MINIMIZE;
    if (data->current_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)) new_style |= WS_MAXIMIZE;

    if ((old_style & WS_MINIMIZE) && !(new_style & WS_MINIMIZE))
    {
        if ((old_style & WS_CAPTION) == WS_CAPTION && (data->current_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)))
        {
            if ((old_style & WS_MAXIMIZEBOX) && !(old_style & WS_DISABLED))
            {
                TRACE( "restoring to max %p/%lx\n", data->hwnd, data->whole_window );
                return SC_MAXIMIZE;
            }
        }
        else if (old_style & (WS_MINIMIZE | WS_MAXIMIZE))
        {
            BOOL activate = (old_style & (WS_MINIMIZE | WS_VISIBLE)) == (WS_MINIMIZE | WS_VISIBLE);
            TRACE( "restoring win %p/%lx\n", data->hwnd, data->whole_window );
            return MAKELONG(SC_RESTORE, activate);
        }
    }
    if (!(old_style & WS_MINIMIZE) && (new_style & WS_MINIMIZE))
    {
        if ((old_style & WS_MINIMIZEBOX) && !(old_style & WS_DISABLED))
        {
            TRACE( "minimizing win %p/%lx\n", data->hwnd, data->whole_window );
            return SC_MINIMIZE;
        }
    }

    return 0;
}

static UINT window_update_client_config( struct x11drv_win_data *data )
{
    static const UINT fullscreen_mask = (1 << NET_WM_STATE_MAXIMIZED) | (1 << NET_WM_STATE_FULLSCREEN);
    UINT old_style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE ), flags;
    RECT rect, old_rect = data->rects.window, new_rect;

    if (!data->managed) return 0; /* unmanaged windows are managed by the Win32 side */
    if (data->desired_state.wm_state != NormalState) return 0; /* ignore config changes on invisible/minimized windows */

    if (data->wm_state_serial) return 0; /* another WM_STATE update is pending, wait for it to complete */
    if (data->net_wm_state_serial) return 0; /* another _NET_WM_STATE update is pending, wait for it to complete */
    if (data->configure_serial) return 0; /* another config update is pending, wait for it to complete */

    if ((old_style & WS_CAPTION) == WS_CAPTION || !data->is_fullscreen)
    {
        if ((data->current_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)) && !(old_style & WS_MAXIMIZE))
        {
            TRACE( "window %p/%lx is maximized\n", data->hwnd, data->whole_window );
            return SC_MAXIMIZE;
        }
        if (!(data->current_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)) && (old_style & WS_MAXIMIZE))
        {
            TRACE( "window %p/%lx is no longer maximized\n", data->hwnd, data->whole_window );
            return SC_RESTORE;
        }
    }

    flags = SWP_NOACTIVATE | SWP_NOZORDER;
    rect = new_rect = window_rect_from_visible( &data->rects, data->current_state.rect );
    if (new_rect.left == old_rect.left && new_rect.top == old_rect.top) flags |= SWP_NOMOVE;
    else OffsetRect( &rect, old_rect.left - new_rect.left, old_rect.top - new_rect.top );
    if (rect.right == old_rect.right && rect.bottom == old_rect.bottom) flags |= SWP_NOSIZE;
    else if (IsRectEmpty( &rect )) flags |= SWP_NOSIZE;

    /* don't sync win32 position for offscreen windows */
    if ((data->is_offscreen = !is_window_rect_mapped( &new_rect ))) flags |= SWP_NOMOVE;

    if ((flags & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE)) return 0;

    /* avoid event feedback loops from window rect adjustments of maximized / fullscreen windows */
    if (data->current_state.net_wm_state & fullscreen_mask) flags |= SWP_NOSENDCHANGING;

    TRACE( "window %p/%lx config changed %s -> %s, flags %#x\n", data->hwnd, data->whole_window,
           wine_dbgstr_rect(&old_rect), wine_dbgstr_rect(&new_rect), flags );
    return MAKELONG(SC_MOVE, flags);
}

/***********************************************************************
 *      GetWindowStateUpdates   (X11DRV.@)
 */
BOOL X11DRV_GetWindowStateUpdates( HWND hwnd, UINT *state_cmd, UINT *config_cmd, RECT *rect, HWND *foreground )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    struct x11drv_win_data *data;
    HWND old_foreground;

    *state_cmd = *config_cmd = 0;
    *foreground = 0;

    if (!(old_foreground = NtUserGetForegroundWindow())) old_foreground = NtUserGetDesktopWindow();
    if (NtUserGetWindowThread( old_foreground, NULL ) == GetCurrentThreadId() &&
        !window_has_pending_wm_state( old_foreground, NormalState ) &&
        !thread_data->net_active_window_serial)
    {
        *foreground = hwnd_from_window( thread_data->display, thread_data->current_state.net_active_window );
        if (*foreground == old_foreground) *foreground = 0;
    }

    if ((data = get_win_data( hwnd )))
    {
        *state_cmd = window_update_client_state( data );
        *config_cmd = window_update_client_config( data );
        *rect = window_rect_from_visible( &data->rects, data->current_state.rect );
        release_win_data( data );
    }

    if (!*state_cmd && !*config_cmd && !*foreground) return FALSE;
    TRACE( "hwnd %p, returning state_cmd %#x, config_cmd %#x, rect %s, foreground %p\n",
           hwnd, *state_cmd, *config_cmd, wine_dbgstr_rect(rect), *foreground );
    return TRUE;
}

static BOOL handle_state_change( unsigned long serial, unsigned long *expect_serial, UINT size, const void *value,
                                 void *desired, void *pending, void *current, const char *expected,
                                 const char *prefix, const char *received, const char *reason )
{
    if (serial < *expect_serial) reason = "old ";
    else if (!*expect_serial && !memcmp( current, value, size )) reason = "no-op ";

    if (reason)
    {
        WARN( "Ignoring %s%s%s%s\n", prefix, reason, received, expected );
        return FALSE;
    }

    if (!*expect_serial) reason = "unexpected ";
    else if (memcmp( pending, value, size )) reason = "mismatch ";

    if (!reason) TRACE( "%s%s%s\n", prefix, received, expected );
    else
    {
        WARN( "%s%s%s%s\n", prefix, reason, received, expected );
        /* avoid requesting the same state again */
        memcpy( desired, value, size );
        memcpy( pending, value, size );
    }

    memcpy( current, value, size );
    *expect_serial = 0;
    return TRUE;
}

void window_wm_state_notify( struct x11drv_win_data *data, unsigned long serial, UINT value, Time time )
{
    UINT *desired = &data->desired_state.wm_state, *pending = &data->pending_state.wm_state, *current = &data->current_state.wm_state;
    unsigned long *expect_serial = &data->wm_state_serial;
    const char *reason = NULL, *expected, *received, *prefix;

    prefix = wine_dbg_sprintf( "window %p/%lx ", data->hwnd, data->whole_window );
    received = wine_dbg_sprintf( "WM_STATE %#x/%lu", value, serial );
    expected = *expect_serial ? wine_dbg_sprintf( ", expected %#x/%lu", *pending, *expect_serial ) : "";
    /* ignore Metacity/Mutter transient NormalState during WithdrawnState <-> IconicState transitions */
    if (value == NormalState && *current + *pending == IconicState) reason = "transient ";

    if (!handle_state_change( serial, expect_serial, sizeof(value), &value, desired, pending,
                              current, expected, prefix, received, reason ))
        return;
    data->current_state.activate = data->pending_state.activate;

    /* send any pending changes from the desired state */
    window_set_wm_state( data, data->desired_state.wm_state, data->desired_state.activate );
    window_set_net_wm_state( data, data->desired_state.net_wm_state );
    window_set_config( data, &data->desired_state.rect, FALSE );

    if (data->current_state.wm_state == NormalState) NtUserSetProp( data->hwnd, focus_time_prop, (HANDLE)time );
    else if (!data->wm_state_serial) NtUserRemoveProp( data->hwnd, focus_time_prop );
}

void window_net_wm_state_notify( struct x11drv_win_data *data, unsigned long serial, UINT value )
{
    UINT *desired = &data->desired_state.net_wm_state, *pending = &data->pending_state.net_wm_state, *current = &data->current_state.net_wm_state;
    unsigned long *expect_serial = &data->net_wm_state_serial;
    const char *expected, *received, *prefix;

    prefix = wine_dbg_sprintf( "window %p/%lx ", data->hwnd, data->whole_window );
    received = wine_dbg_sprintf( "_NET_WM_STATE %#x/%lu", value, serial );
    expected = *expect_serial ? wine_dbg_sprintf( ", expected %#x/%lu", *pending, *expect_serial ) : "";

    if (!handle_state_change( serial, expect_serial, sizeof(value), &value, desired, pending,
                              current, expected, prefix, received, NULL ))
        return;

    /* send any pending changes from the desired state */
    window_set_wm_state( data, data->desired_state.wm_state, data->desired_state.activate );
    window_set_net_wm_state( data, data->desired_state.net_wm_state );
    window_set_config( data, &data->desired_state.rect, FALSE );
}

void window_configure_notify( struct x11drv_win_data *data, unsigned long serial, const RECT *value )
{
    RECT *desired = &data->desired_state.rect, *pending = &data->pending_state.rect, *current = &data->current_state.rect;
    unsigned long *expect_serial = &data->configure_serial;
    const char *expected, *received, *prefix;

    prefix = wine_dbg_sprintf( "window %p/%lx ", data->hwnd, data->whole_window );
    received = wine_dbg_sprintf( "config %s/%lu", wine_dbgstr_rect(value), serial );
    expected = *expect_serial ? wine_dbg_sprintf( ", expected %s/%lu", wine_dbgstr_rect(pending), *expect_serial ) : "";

    handle_state_change( serial, expect_serial, sizeof(*value), value, desired, pending,
                         current, expected, prefix, received, NULL );
}

void net_active_window_notify( unsigned long serial, Window value, Time time )
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    Window *desired = &data->desired_state.net_active_window, *pending = &data->pending_state.net_active_window, *current = &data->current_state.net_active_window;
    unsigned long *expect_serial = &data->net_active_window_serial;
    const char *expected, *received;
    HWND current_hwnd, pending_hwnd;

    current_hwnd = hwnd_from_window( data->display, value );
    pending_hwnd = hwnd_from_window( data->display, *pending );

    received = wine_dbg_sprintf( "_NET_ACTIVE_WINDOW %p/%lx serial %lu time %lu", current_hwnd, value, serial, time );
    expected = *expect_serial ? wine_dbg_sprintf( ", expected %p/%lx serial %lu", pending_hwnd, *pending, *expect_serial ) : "";
    if (!handle_state_change( serial, expect_serial, sizeof(value), &value, desired, pending,
                              current, expected, "", received, NULL ))
        return;

    NtUserPostMessage( NtUserGetForegroundWindow(), WM_WINE_WINDOW_STATE_CHANGED, 0, 0 );
}

Window get_net_active_window( Display *display )
{
    unsigned long count, remaining;
    Window window = None, *value;
    int format;
    Atom type;

    if (!XGetWindowProperty( display, DefaultRootWindow( display ), x11drv_atom(_NET_ACTIVE_WINDOW), 0,
                             65536 / sizeof(Window), False, XA_WINDOW, &type, &format, &count,
                             &remaining, (unsigned char **)&value ))
    {
        if (type == XA_WINDOW && format == 32) window = *value;
        XFree( value );
    }

    return window;
}

void net_active_window_init( struct x11drv_thread_data *data )
{
    Window window = get_net_active_window( data->display );

    data->desired_state.net_active_window = window;
    data->pending_state.net_active_window = window;
    data->current_state.net_active_window = window;
}

static BOOL window_set_pending_activate( HWND hwnd )
{
    struct x11drv_win_data *data;
    BOOL pending;

    if (!(data = get_win_data( hwnd ))) return FALSE;
    if ((pending = !!data->wm_state_serial)) data->pending_state.activate = TRUE;
    release_win_data( data );

    return pending;
}

void set_net_active_window( HWND hwnd, HWND previous )
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    Window window;
    XEvent xev;

    if (!is_net_supported( x11drv_atom(_NET_ACTIVE_WINDOW) )) return;
    if (!(window = X11DRV_get_whole_window( hwnd ))) return;
    if (data->pending_state.net_active_window == window) return;
    if (window_set_pending_activate( hwnd )) return;

    xev.xclient.type = ClientMessage;
    xev.xclient.window = window;
    xev.xclient.message_type = x11drv_atom(_NET_ACTIVE_WINDOW);
    xev.xclient.serial = 0;
    xev.xclient.display = data->display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 2; /* source: pager */
    xev.xclient.data.l[1] = 0; /* timestamp */
    xev.xclient.data.l[2] = X11DRV_get_whole_window( previous ); /* current active */
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;

    data->pending_state.net_active_window = window;
    data->net_active_window_serial = NextRequest( data->display );
    TRACE( "requesting _NET_ACTIVE_WINDOW %p/%lx serial %lu\n", hwnd, window, data->net_active_window_serial );
    XSendEvent( data->display, DefaultRootWindow( data->display ), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev );
    XFlush( data->display );
}

BOOL window_has_pending_wm_state( HWND hwnd, UINT state )
{
    struct x11drv_win_data *data;
    BOOL pending;

    if (!(data = get_win_data( hwnd ))) return FALSE;
    if (state != -1 && data->desired_state.wm_state != state) pending = FALSE;
    else pending = !!data->wm_state_serial;
    release_win_data( data );

    return pending;
}

BOOL window_should_take_focus( HWND hwnd, Time time )
{
    Time focus_time = (UINT_PTR)NtUserGetProp( hwnd, focus_time_prop );
    return !focus_time || time > focus_time;
}

/***********************************************************************
 *     make_window_embedded
 */
void make_window_embedded( struct x11drv_win_data *data )
{
    /* the window cannot be mapped before being embedded */
    window_set_wm_state( data, WithdrawnState, FALSE );
    if (data->managed) WARN( "Window is already managed, should wait for WithdrawnState\n" );
    else window_set_managed( data, TRUE );
    data->embedded = TRUE;
    window_set_wm_state( data, NormalState, FALSE );
}


/***********************************************************************
 *		sync_window_position
 *
 * Synchronize the X window position with the Windows one
 */
static void sync_window_position( struct x11drv_win_data *data, UINT swp_flags, const struct window_rects *old_rects )
{
    DWORD style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE );
    DWORD ex_style = NtUserGetWindowLongW( data->hwnd, GWL_EXSTYLE );
    RECT new_rect, window_rect;
    BOOL above = FALSE;

    if (data->managed && ((style & WS_MINIMIZE) || data->desired_state.wm_state == IconicState)) return;

    if (!(swp_flags & SWP_NOZORDER) || (swp_flags & SWP_SHOWWINDOW))
    {
        /* find window that this one must be after */
        HWND prev = NtUserGetWindowRelative( data->hwnd, GW_HWNDPREV );
        while (prev && !(NtUserGetWindowLongW( prev, GWL_STYLE ) & WS_VISIBLE))
            prev = NtUserGetWindowRelative( prev, GW_HWNDPREV );
        if (!prev) above = TRUE;  /* top child */
        /* should use stack_mode Below but most window managers don't get it right */
        /* and Above with a sibling doesn't work so well either, so we ignore it */
    }

    set_size_hints( data, style );
    set_mwm_hints( data, style, ex_style );
    update_net_wm_states( data );

    new_rect = data->rects.visible;

    /* if the window has been moved offscreen by the window manager, we didn't tell the Win32 side about it */
    window_rect = window_rect_from_visible( old_rects, data->desired_state.rect );
    if (data->is_offscreen) OffsetRect( &new_rect, window_rect.left - old_rects->window.left,
                                        window_rect.top - old_rects->window.top );

    window_set_config( data, &new_rect, above );
}


/***********************************************************************
 *		sync_client_position
 *
 * Synchronize the X client window position with the Windows one
 */
static void sync_client_position( struct x11drv_win_data *data, const struct window_rects *old_rects )
{
    int mask = 0;
    XWindowChanges changes;

    if (!data->client_window) return;

    changes.x      = data->rects.client.left - data->rects.visible.left;
    changes.y      = data->rects.client.top - data->rects.visible.top;
    if (changes.x != old_rects->client.left - old_rects->visible.left) mask |= CWX;
    if (changes.y != old_rects->client.top  - old_rects->visible.top)  mask |= CWY;

    if (mask)
    {
        TRACE( "setting client win %lx pos %d,%d changes=%x\n",
               data->client_window, changes.x, changes.y, mask );
        XConfigureWindow( gdi_display, data->client_window, mask, &changes );
    }
}


/***********************************************************************
 *		move_window_bits
 *
 * Move the window bits when a window is moved.
 */
static void move_window_bits( HWND hwnd, Window window, const struct window_rects *old_rects,
                              const struct window_rects *new_rects, const RECT *valid_rects )
{
    RECT src_rect = valid_rects[1], dst_rect = valid_rects[0];
    HDC hdc_src, hdc_dst;
    INT code;
    HRGN rgn;
    HWND parent = 0;

    if (!window)
    {
        OffsetRect( &dst_rect, -new_rects->window.left, -new_rects->window.top );
        parent = NtUserGetAncestor( hwnd, GA_PARENT );
        hdc_src = NtUserGetDCEx( parent, 0, DCX_CACHE );
        hdc_dst = NtUserGetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    }
    else
    {
        OffsetRect( &dst_rect, -new_rects->client.left, -new_rects->client.top );
        /* make src rect relative to the old position of the window */
        OffsetRect( &src_rect, -old_rects->client.left, -old_rects->client.top );
        if (dst_rect.left == src_rect.left && dst_rect.top == src_rect.top) return;
        hdc_src = hdc_dst = NtUserGetDCEx( hwnd, 0, DCX_CACHE );
    }

    rgn = NtGdiCreateRectRgn( dst_rect.left, dst_rect.top, dst_rect.right, dst_rect.bottom );
    NtGdiExtSelectClipRgn( hdc_dst, rgn, RGN_COPY );
    NtGdiDeleteObjectApp( rgn );
    /* WS_CLIPCHILDREN doesn't exclude children from the window update
     * region, and ExcludeUpdateRgn call may inappropriately clip valid
     * child window contents from the copied parent window bits, but we
     * still want to avoid copying invalid window bits when possible.
     */
    if (!(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_CLIPCHILDREN ))
        NtUserExcludeUpdateRgn( hdc_dst, hwnd );

    code = X11DRV_START_EXPOSURES;
    NtGdiExtEscape( hdc_dst, NULL, 0, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );

    TRACE( "copying bits for win %p/%lx %s -> %s\n",
           hwnd, window, wine_dbgstr_rect(&src_rect), wine_dbgstr_rect(&dst_rect) );
    NtGdiBitBlt( hdc_dst, dst_rect.left, dst_rect.top,
                 dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top,
                 hdc_src, src_rect.left, src_rect.top, SRCCOPY, 0, 0 );

    rgn = 0;
    code = X11DRV_END_EXPOSURES;
    NtGdiExtEscape( hdc_dst, NULL, 0, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, sizeof(rgn), (LPSTR)&rgn );

    NtUserReleaseDC( hwnd, hdc_dst );
    if (hdc_src != hdc_dst) NtUserReleaseDC( parent, hdc_src );

    if (rgn)
    {
        if (!window)
        {
            /* map region to client rect since we are using DCX_WINDOW */
            NtGdiOffsetRgn( rgn, new_rects->window.left - new_rects->client.left,
                            new_rects->window.top - new_rects->client.top );
            NtUserRedrawWindow( hwnd, NULL, rgn, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN );
        }
        else NtUserRedrawWindow( hwnd, NULL, rgn, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
        NtGdiDeleteObjectApp( rgn );
    }
}


/***********************************************************************
 *              get_dummy_parent
 *
 * Create a dummy parent window for child windows that don't have a true X11 parent.
 */
Window get_dummy_parent(void)
{
    static Window dummy_parent;

    if (!dummy_parent)
    {
        XSetWindowAttributes attrib;

        attrib.override_redirect = True;
        attrib.border_pixel = 0;
        attrib.colormap = default_colormap;

#ifdef HAVE_LIBXSHAPE
        {
            static XRectangle empty_rect;
            dummy_parent = XCreateWindow( gdi_display, root_window, 0, 0, 1, 1, 0,
                                          default_visual.depth, InputOutput, default_visual.visual,
                                          CWColormap | CWBorderPixel | CWOverrideRedirect, &attrib );
            XShapeCombineRectangles( gdi_display, dummy_parent, ShapeBounding, 0, 0, &empty_rect, 1,
                                     ShapeSet, YXBanded );
        }
#else
        dummy_parent = XCreateWindow( gdi_display, root_window, -1, -1, 1, 1, 0, default_visual.depth,
                                      InputOutput, default_visual.visual,
                                      CWColormap | CWBorderPixel | CWOverrideRedirect, &attrib );
        WARN("Xshape support is not compiled in. Applications under XWayland may have poor performance.\n");
#endif
        XMapWindow( gdi_display, dummy_parent );
    }
    return dummy_parent;
}

static void client_window_events_enable( struct x11drv_win_data *data, Window client_window )
{
    XSaveContext( data->display, client_window, winContext, (char *)data->hwnd );
    XSync( data->display, False ); /* make sure client_window is known from data->display */
    XSelectInput( data->display, client_window, ExposureMask );
}

static void client_window_events_disable( struct x11drv_win_data *data, Window client_window )
{
    XSelectInput( data->display, client_window, 0 );
    XFlush( data->display ); /* make sure XSelectInput doesn't use client_window after this point */
    XDeleteContext( data->display, client_window, winContext );
}

/**********************************************************************
 *		detach_client_window
 */
void detach_client_window( struct x11drv_win_data *data, Window client_window )
{
    if (data->client_window != client_window || !client_window) return;

    TRACE( "%p/%lx detaching client window %lx\n", data->hwnd, data->whole_window, client_window );

    if (data->whole_window)
    {
        client_window_events_disable( data, client_window );
        XReparentWindow( gdi_display, client_window, get_dummy_parent(), 0, 0 );
    }

    data->client_window = 0;
}


/**********************************************************************
 *             attach_client_window
 */
void attach_client_window( struct x11drv_win_data *data, Window client_window )
{
    if (data->client_window == client_window || !client_window) return;

    TRACE( "%p/%lx attaching client window %lx\n", data->hwnd, data->whole_window, client_window );

    detach_client_window( data, data->client_window );

    if (data->whole_window)
    {
        client_window_events_enable( data, client_window );
        XReparentWindow( gdi_display, client_window, data->whole_window, data->rects.client.left - data->rects.visible.left,
                         data->rects.client.top - data->rects.visible.top );
    }

    data->client_window = client_window;
}


/**********************************************************************
 *      destroy_client_window
 */
void destroy_client_window( HWND hwnd, Window client_window )
{
    struct x11drv_win_data *data;

    TRACE( "%p destroying client window %lx\n", hwnd, client_window );

    if ((data = get_win_data( hwnd )))
    {
        if (data->client_window == client_window)
        {
            if (data->whole_window) client_window_events_disable( data, client_window );
            data->client_window = 0;
        }
        release_win_data( data );
    }

    XDestroyWindow( gdi_display, client_window );
}


/**********************************************************************
 *		create_client_window
 */
Window create_client_window( HWND hwnd, const XVisualInfo *visual, Colormap colormap )
{
    Window dummy_parent = get_dummy_parent();
    struct x11drv_win_data *data = get_win_data( hwnd );
    XSetWindowAttributes attr;
    Window ret;
    int x, y, cx, cy;
    RECT client_rect;

    if (!data)
    {
        /* explicitly create data for HWND_MESSAGE windows since they can be used for OpenGL */
        HWND parent = NtUserGetAncestor( hwnd, GA_PARENT );
        if (parent == NtUserGetDesktopWindow() || NtUserGetAncestor( parent, GA_PARENT )) return 0;
        if (!(data = alloc_win_data( thread_init_display(), hwnd ))) return 0;
        NtUserGetClientRect( hwnd, &data->rects.client, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) );
        data->rects.window = data->rects.visible = data->rects.client;
    }

    detach_client_window( data, data->client_window );

    attr.colormap = colormap;
    attr.bit_gravity = NorthWestGravity;
    attr.win_gravity = NorthWestGravity;
    attr.backing_store = NotUseful;
    attr.border_pixel = 0;

    x = data->rects.client.left - data->rects.visible.left;
    y = data->rects.client.top - data->rects.visible.top;

    NtUserGetClientRect( hwnd, &client_rect, NtUserGetDpiForWindow( hwnd ) );
    cx = min( max( 1, client_rect.right - client_rect.left ), 65535 );
    cy = min( max( 1, client_rect.bottom - client_rect.top ), 65535 );

    XSync( gdi_display, False ); /* make sure whole_window is known from gdi_display */
    ret = data->client_window = XCreateWindow( gdi_display,
                                               data->whole_window ? data->whole_window : dummy_parent,
                                               x, y, cx, cy, 0, default_visual.depth, InputOutput,
                                               visual->visual, CWBitGravity | CWWinGravity |
                                               CWBackingStore | CWColormap | CWBorderPixel, &attr );
    if (data->client_window)
    {
        XMapWindow( gdi_display, data->client_window );
        if (data->whole_window)
        {
            XFlush( gdi_display ); /* make sure client_window is created for XSelectInput */
            client_window_events_enable( data, data->client_window );
        }
        TRACE( "%p xwin %lx/%lx\n", data->hwnd, data->whole_window, data->client_window );
    }
    release_win_data( data );
    return ret;
}


/**********************************************************************
 *		create_whole_window
 *
 * Create the whole X window for a given window
 */
static void create_whole_window( struct x11drv_win_data *data )
{
    int cx, cy, mask;
    XSetWindowAttributes attr;
    WCHAR text[1024];
    COLORREF key;
    BYTE alpha;
    DWORD layered_flags;
    HRGN win_rgn;
    POINT pos;

    if ((win_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 )) &&
        NtUserGetWindowRgnEx( data->hwnd, win_rgn, 0 ) == ERROR)
    {
        NtGdiDeleteObjectApp( win_rgn );
        win_rgn = 0;
    }
    data->shaped = (win_rgn != 0);

    if (data->vis.visualid != default_visual.visualid)
        data->whole_colormap = XCreateColormap( data->display, root_window, data->vis.visual, AllocNone );

    data->managed = is_window_managed( data->hwnd, SWP_NOACTIVATE, FALSE );
    mask = get_window_attributes( data, &attr ) | CWOverrideRedirect;
    attr.override_redirect = !data->managed;

    if (!(cx = data->rects.visible.right - data->rects.visible.left)) cx = 1;
    else if (cx > 65535) cx = 65535;
    if (!(cy = data->rects.visible.bottom - data->rects.visible.top)) cy = 1;
    else if (cy > 65535) cy = 65535;

    pos = virtual_screen_to_root( data->rects.visible.left, data->rects.visible.top );
    data->whole_window = XCreateWindow( data->display, root_window, pos.x, pos.y,
                                        cx, cy, 0, data->vis.depth, InputOutput,
                                        data->vis.visual, mask, &attr );
    if (!data->whole_window) goto done;
    SetRect( &data->current_state.rect, pos.x, pos.y, pos.x + cx, pos.y + cy );
    data->pending_state.rect = data->current_state.rect;
    data->desired_state.rect = data->current_state.rect;

    x11drv_xinput2_enable( data->display, data->whole_window );
    set_initial_wm_hints( data->display, data->whole_window );
    set_wm_hints( data );

    XSaveContext( data->display, data->whole_window, winContext, (char *)data->hwnd );
    NtUserSetProp( data->hwnd, whole_window_prop, (HANDLE)data->whole_window );

    /* set the window text */
    if (!NtUserInternalGetWindowText( data->hwnd, text, ARRAY_SIZE( text ))) text[0] = 0;
    sync_window_text( data->display, data->whole_window, text );

    /* set the window region */
    if (IsRectEmpty( &data->rects.window )) sync_empty_window_shape( data, NULL );
    else if (win_rgn) sync_window_region( data, win_rgn );

    /* set the window opacity */
    if (!NtUserGetLayeredWindowAttributes( data->hwnd, &key, &alpha, &layered_flags )) layered_flags = 0;
    sync_window_opacity( data->display, data->whole_window, alpha, layered_flags );

    XFlush( data->display );  /* make sure the window exists before we start painting to it */

done:
    if (win_rgn) NtGdiDeleteObjectApp( win_rgn );
}


/**********************************************************************
 *		destroy_whole_window
 *
 * Destroy the whole X window for a given window.
 */
static void destroy_whole_window( struct x11drv_win_data *data, BOOL already_destroyed )
{
    TRACE( "win %p xwin %lx/%lx\n", data->hwnd, data->whole_window, data->client_window );

    if (!data->whole_window)
    {
        if (data->embedded) return;
    }
    else
    {
        if (!already_destroyed) detach_client_window( data, data->client_window );
        else if (data->client_window) client_window_events_disable( data, data->client_window );
        XDeleteContext( data->display, data->whole_window, winContext );
        if (!already_destroyed)
        {
            XSync( gdi_display, False ); /* make sure XReparentWindow requests have completed before destroying whole_window */
            XDestroyWindow( data->display, data->whole_window );
        }
    }
    if (data->whole_colormap) XFreeColormap( data->display, data->whole_colormap );
    data->whole_window = data->client_window = 0;
    data->whole_colormap = 0;
    data->managed = FALSE;

    memset( &data->desired_state, 0, sizeof(data->desired_state) );
    memset( &data->pending_state, 0, sizeof(data->pending_state) );
    memset( &data->current_state, 0, sizeof(data->current_state) );
    data->wm_state_serial = 0;
    data->net_wm_state_serial = 0;
    data->configure_serial = 0;

    if (data->xic)
    {
        XUnsetICFocus( data->xic );
        XDestroyIC( data->xic );
        data->xic = 0;
    }
    /* Outlook stops processing messages after destroying a dialog, so we need an explicit flush */
    XFlush( data->display );
    NtUserRemoveProp( data->hwnd, whole_window_prop );

    /* It's possible that we are in a different thread, when called from
     * set_window_visual, and about to recreate the window. In this case
     * just set a window flag to indicate the parent isn't valid and let
     * the thread eventually replace it with the proper one later on.
     */
    if (data->display != thread_init_display()) data->parent_invalid = 1;
    else if (data->parent)
    {
        host_window_release( data->parent );
        data->parent = NULL;
    }
}


/**********************************************************************
 *		set_window_visual
 *
 * Change the visual by destroying and recreating the X window if needed.
 */
void set_window_visual( struct x11drv_win_data *data, const XVisualInfo *vis, BOOL use_alpha )
{
    BOOL same_visual = (data->vis.visualid == vis->visualid);
    Window client_window = data->client_window;

    if (!data->use_alpha == !use_alpha && same_visual) return;
    data->use_alpha = use_alpha;

    if (same_visual) return;
    client_window = data->client_window;
    /* detach the client before re-creating whole_window */
    detach_client_window( data, client_window );
    destroy_whole_window( data, FALSE );
    data->vis = *vis;
    create_whole_window( data );
    /* attach the client back to the re-created whole_window */
    attach_client_window( data, client_window );
}


/*****************************************************************
 *		SetWindowText   (X11DRV.@)
 */
void X11DRV_SetWindowText( HWND hwnd, LPCWSTR text )
{
    Window win;

    if ((win = X11DRV_get_whole_window( hwnd )) && win != DefaultRootWindow(gdi_display))
    {
        Display *display = thread_init_display();
        sync_window_text( display, win, text );
    }
}


/***********************************************************************
 *		SetWindowStyle   (X11DRV.@)
 *
 * Update the X state of a window to reflect a style change
 */
void X11DRV_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
    struct x11drv_win_data *data;
    DWORD changed = style->styleNew ^ style->styleOld;

    if (hwnd == NtUserGetDesktopWindow()) return;
    if (!(data = get_win_data( hwnd ))) return;
    if (!data->whole_window) goto done;

    if (offset == GWL_STYLE && (changed & WS_DISABLED)) set_wm_hints( data );

    if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED)) /* changing WS_EX_LAYERED resets attributes */
    {
        data->layered = FALSE;
        set_window_visual( data, &default_visual, FALSE );
        sync_window_opacity( data->display, data->whole_window, 0, 0 );
    }
done:
    release_win_data( data );
}


/***********************************************************************
 *		DestroyWindow   (X11DRV.@)
 */
void X11DRV_DestroyWindow( HWND hwnd )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    struct x11drv_win_data *data;

    if (!(data = get_win_data( hwnd ))) return;

    destroy_whole_window( data, FALSE );
    if (thread_data->last_focus == hwnd) thread_data->last_focus = 0;
    if (thread_data->last_xic_hwnd == hwnd) thread_data->last_xic_hwnd = 0;
    if (data->icon_pixmap) XFreePixmap( gdi_display, data->icon_pixmap );
    if (data->icon_mask) XFreePixmap( gdi_display, data->icon_mask );
    if (data->parent) host_window_release( data->parent );
    free( data->icon_bits );
    XDeleteContext( gdi_display, (XID)hwnd, win_data_context );
    release_win_data( data );
    free( data );
    destroy_gl_drawable( hwnd );
}


/***********************************************************************
 *		X11DRV_DestroyNotify
 */
BOOL X11DRV_DestroyNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;
    BOOL embedded;

    if (!(data = get_win_data( hwnd ))) return FALSE;
    embedded = data->embedded;
    if (!embedded) FIXME( "window %p/%lx destroyed from the outside\n", hwnd, data->whole_window );

    destroy_whole_window( data, TRUE );
    release_win_data( data );
    if (embedded) send_message( hwnd, WM_CLOSE, 0, 0 );
    return TRUE;
}


/* initialize the desktop window id in the desktop manager process */
static BOOL create_desktop_win_data( Window win, HWND hwnd )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    Display *display = thread_data->display;
    struct x11drv_win_data *data;

    if (!(data = alloc_win_data( display, hwnd ))) return FALSE;
    data->whole_window = win;
    window_set_managed( data, TRUE );
    NtUserSetProp( data->hwnd, whole_window_prop, (HANDLE)win );
    set_initial_wm_hints( display, win );
    if (is_desktop_fullscreen()) window_set_net_wm_state( data, (1 << NET_WM_STATE_FULLSCREEN) );
    release_win_data( data );
    if (thread_data->clip_window) XReparentWindow( display, thread_data->clip_window, win, 0, 0 );
    return TRUE;
}

/**********************************************************************
 *		SetDesktopWindow   (X11DRV.@)
 */
void X11DRV_SetDesktopWindow( HWND hwnd )
{
    unsigned int width, height;

    /* retrieve the real size of the desktop */
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->relative = COORDS_CLIENT;
        wine_server_call( req );
        width  = reply->window.right;
        height = reply->window.bottom;
    }
    SERVER_END_REQ;

    if (!width && !height)  /* not initialized yet */
    {
        RECT rect = NtUserGetVirtualScreenRect( MDT_DEFAULT );

        SERVER_START_REQ( set_window_pos )
        {
            req->handle        = wine_server_user_handle( hwnd );
            req->previous      = 0;
            req->swp_flags     = SWP_NOZORDER;
            req->window        = wine_server_rectangle( rect );
            req->client        = req->window;
            wine_server_call( req );
        }
        SERVER_END_REQ;

        if (!is_virtual_desktop()) return;
        if (!create_desktop_win_data( root_window, hwnd ))
        {
            ERR( "Failed to create virtual desktop window data\n" );
            root_window = DefaultRootWindow( gdi_display );
        }
    }
    else
    {
        Window win = (Window)NtUserGetProp( hwnd, whole_window_prop );
        if (win && win != root_window) X11DRV_init_desktop( win, width, height );
    }
}


#define WM_WINE_NOTIFY_ACTIVITY WM_USER
#define WM_WINE_DELETE_TAB      (WM_USER + 1)
#define WM_WINE_ADD_TAB         (WM_USER + 2)

/**********************************************************************
 *           DesktopWindowProc   (X11DRV.@)
 */
LRESULT X11DRV_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_WINE_NOTIFY_ACTIVITY:
    {
        static ULONG last = 0;
        ULONG now = NtGetTickCount();
        /* calling XResetScreenSaver too often can cause performance
         * problems, so throttle it */
        if (now > last + 5000)
        {
            XResetScreenSaver( gdi_display );
            XFlush( gdi_display );
            last = now;
        }
        break;
    }
    case WM_WINE_DELETE_TAB:
        send_notify_message( (HWND)wp, WM_X11DRV_DELETE_TAB, 0, 0 );
        break;
    case WM_WINE_ADD_TAB:
        send_notify_message( (HWND)wp, WM_X11DRV_ADD_TAB, 0, 0 );
        break;
    }
    return NtUserMessageCall( hwnd, msg, wp, lp, 0, NtUserDefWindowProc, FALSE );
}

/**********************************************************************
 *		CreateWindow   (X11DRV.@)
 */
BOOL X11DRV_CreateWindow( HWND hwnd )
{
    if (hwnd == NtUserGetDesktopWindow())
    {
        struct x11drv_thread_data *data = x11drv_init_thread_data();
        XSetWindowAttributes attr;

        /* create the cursor clipping window */
        attr.override_redirect = TRUE;
        attr.event_mask = StructureNotifyMask | FocusChangeMask;
        data->clip_window = XCreateWindow( data->display, root_window, 0, 0, 1, 1, 0, 0,
                                           InputOnly, default_visual.visual,
                                           CWOverrideRedirect | CWEventMask, &attr );
        XFlush( data->display );
        NtUserSetProp( hwnd, clip_window_prop, (HANDLE)data->clip_window );
        X11DRV_DisplayDevices_RegisterEventHandlers();
    }
    return TRUE;
}


/***********************************************************************
 *		get_win_data
 *
 * Lock and return the X11 data structure associated with a window.
 */
struct x11drv_win_data *get_win_data( HWND hwnd )
{
    char *data;

    if (!hwnd) return NULL;
    pthread_mutex_lock( &win_data_mutex );
    if (!XFindContext( gdi_display, (XID)hwnd, win_data_context, &data ))
        return (struct x11drv_win_data *)data;
    pthread_mutex_unlock( &win_data_mutex );
    return NULL;
}


/***********************************************************************
 *		release_win_data
 *
 * Release the data returned by get_win_data.
 */
void release_win_data( struct x11drv_win_data *data )
{
    if (data) pthread_mutex_unlock( &win_data_mutex );
}

/* update the whole window parent host window, must be called from the window's owner thread */
void set_window_parent( struct x11drv_win_data *data, Window parent )
{
    if (!data->whole_window) return; /* only keep track of parent if we have a toplevel */
    TRACE( "window %p/%lx, parent %lx\n", data->hwnd, data->whole_window, parent );
    host_window_reparent( &data->parent, parent, data->whole_window );
    if (data->parent)
    {
        RECT rect = data->rects.visible;
        OffsetRect( &rect, -rect.left, -rect.top );
        host_window_configure_child( data->parent, data->whole_window, rect, FALSE );
    }
    data->parent_invalid = 0;
}


/***********************************************************************
 *		X11DRV_create_win_data
 *
 * Create an X11 data window structure for an existing window.
 */
static struct x11drv_win_data *X11DRV_create_win_data( HWND hwnd, const struct window_rects *rects )
{
    Display *display;
    struct x11drv_win_data *data;
    HWND parent;

    if (!(parent = NtUserGetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop */

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != NtUserGetDesktopWindow() && !NtUserGetAncestor( parent, GA_PARENT )) return NULL;

    if (NtUserGetWindowThread( hwnd, NULL ) != GetCurrentThreadId()) return NULL;

    /* Recreate the parent gl_drawable now that we know there are child windows
     * that will need clipping support.
     */
    sync_gl_drawable( parent, TRUE );

    display = thread_init_display();
    init_clip_window();  /* make sure the clip window is initialized in this thread */

    if (!(data = alloc_win_data( display, hwnd ))) return NULL;
    data->rects = *rects;

    if (parent == NtUserGetDesktopWindow())
    {
        create_whole_window( data );
        TRACE( "win %p/%lx window %s whole %s client %s\n",
               hwnd, data->whole_window, wine_dbgstr_rect( &data->rects.window ),
               wine_dbgstr_rect( &data->rects.visible ), wine_dbgstr_rect( &data->rects.client ));
    }
    return data;
}


/***********************************************************************
 *              SystrayDockInit   (X11DRV.@)
 */
void X11DRV_SystrayDockInit( HWND hwnd )
{
    Display *display;

    if (is_virtual_desktop()) return;

    systray_hwnd = hwnd;
    display = thread_init_display();
    if (DefaultScreen( display ) == 0)
        systray_atom = x11drv_atom(_NET_SYSTEM_TRAY_S0);
    else
    {
        char systray_buffer[29]; /* strlen(_NET_SYSTEM_TRAY_S4294967295)+1 */
        sprintf( systray_buffer, "_NET_SYSTEM_TRAY_S%u", DefaultScreen( display ) );
        systray_atom = XInternAtom( display, systray_buffer, False );
    }
    XSelectInput( display, root_window, StructureNotifyMask | PropertyChangeMask );
}


/***********************************************************************
 *              SystrayDockClear   (X11DRV.@)
 */
void X11DRV_SystrayDockClear( HWND hwnd )
{
    Window win = X11DRV_get_whole_window( hwnd );
    if (win) XClearArea( gdi_display, win, 0, 0, 0, 0, True );
}


/***********************************************************************
 *              SystrayDockRemove   (X11DRV.@)
 */
BOOL X11DRV_SystrayDockRemove( HWND hwnd )
{
    struct x11drv_win_data *data;
    BOOL ret = FALSE;

    if ((data = get_win_data( hwnd )))
    {
        if ((ret = data->embedded)) window_set_wm_state( data, WithdrawnState, FALSE );
        release_win_data( data );
    }

    return ret;
}


/* find the X11 window owner the system tray selection */
static Window get_systray_selection_owner( Display *display )
{
    return XGetSelectionOwner( display, systray_atom );
}


static void get_systray_visual_info( Display *display, Window systray_window, XVisualInfo *info )
{
    XVisualInfo *list, template;
    VisualID *visual_id;
    Atom type;
    int format, num;
    unsigned long count, remaining;

    *info = default_visual;
    if (XGetWindowProperty( display, systray_window, x11drv_atom(_NET_SYSTEM_TRAY_VISUAL), 0,
                            65536/sizeof(CARD32), False, XA_VISUALID, &type, &format, &count,
                            &remaining, (unsigned char **)&visual_id ))
        return;

    if (type == XA_VISUALID && format == 32)
    {
        template.visualid = visual_id[0];
        if ((list = XGetVisualInfo( display, VisualIDMask, &template, &num )))
        {
            *info = list[0];
            TRACE_(systray)( "systray window %lx got visual %lx\n", systray_window, info->visualid );
            XFree( list );
        }
    }
    XFree( visual_id );
}


/***********************************************************************
 *              SystrayDockInsert   (X11DRV.@)
 */
BOOL X11DRV_SystrayDockInsert( HWND hwnd, UINT cx, UINT cy, void *icon )
{
    Display *display = thread_init_display();
    Window systray_window, window;
    XEvent ev;
    XVisualInfo visual;
    struct x11drv_win_data *data;

    if (!(systray_window = get_systray_selection_owner( display ))) return FALSE;

    get_systray_visual_info( display, systray_window, &visual );

    if (!(data = get_win_data( hwnd ))) return FALSE;
    set_window_visual( data, &visual, TRUE );
    make_window_embedded( data );
    window = data->whole_window;
    release_win_data( data );

    NtUserSetWindowPos( hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER );
    NtUserShowWindow( hwnd, SW_SHOWNA );

    TRACE_(systray)( "icon window %p/%lx\n", hwnd, window );

    /* send the docking request message */
    ev.xclient.type = ClientMessage;
    ev.xclient.window = systray_window;
    ev.xclient.message_type = x11drv_atom( _NET_SYSTEM_TRAY_OPCODE );
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
    ev.xclient.data.l[2] = window;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;
    XSendEvent( display, systray_window, False, NoEventMask, &ev );

    return TRUE;
}


/***********************************************************************
 *		X11DRV_get_whole_window
 *
 * Return the X window associated with the full area of a window
 */
Window X11DRV_get_whole_window( HWND hwnd )
{
    struct x11drv_win_data *data = get_win_data( hwnd );
    Window ret;

    if (!data)
    {
        if (hwnd == NtUserGetDesktopWindow()) return root_window;
        return (Window)NtUserGetProp( hwnd, whole_window_prop );
    }
    ret = data->whole_window;
    release_win_data( data );
    return ret;
}


/***********************************************************************
 *		X11DRV_GetDC   (X11DRV.@)
 */
void X11DRV_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                   const RECT *top_rect, DWORD flags )
{
    struct x11drv_escape_set_drawable escape;
    struct x11drv_win_data *data;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.mode = IncludeInferiors;
    escape.drawable = 0;

    escape.dc_rect.left         = win_rect->left - top_rect->left;
    escape.dc_rect.top          = win_rect->top - top_rect->top;
    escape.dc_rect.right        = win_rect->right - top_rect->left;
    escape.dc_rect.bottom       = win_rect->bottom - top_rect->top;

    if ((data = get_win_data( top )))
    {
        escape.drawable = data->whole_window;
        escape.visual = data->vis;
        /* special case: when repainting the root window, clip out top-level windows */
        if (top == hwnd && data->whole_window == root_window) escape.mode = ClipByChildren;
        release_win_data( data );
    }
    else
    {
        escape.drawable = X11DRV_get_whole_window( top );
        escape.visual = default_visual; /* FIXME: use the right visual for other process window */
    }

    if (!escape.drawable) return; /* don't create a GC for foreign windows */
    NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}


/***********************************************************************
 *		X11DRV_ReleaseDC  (X11DRV.@)
 */
void X11DRV_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct x11drv_escape_set_drawable escape;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = root_window;
    escape.mode = IncludeInferiors;
    escape.dc_rect = NtUserGetVirtualScreenRect( MDT_DEFAULT );
    OffsetRect( &escape.dc_rect, -2 * escape.dc_rect.left, -2 * escape.dc_rect.top );
    NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}


/*************************************************************************
 *		ScrollDC   (X11DRV.@)
 */
BOOL X11DRV_ScrollDC( HDC hdc, INT dx, INT dy, HRGN update )
{
    RECT rect;
    BOOL ret;
    HRGN expose_rgn = 0;

    NtGdiGetAppClipBox( hdc, &rect );

    if (update)
    {
        INT code = X11DRV_START_EXPOSURES;
        NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );

        ret = NtGdiBitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                           hdc, rect.left - dx, rect.top - dy, SRCCOPY, 0, 0 );

        code = X11DRV_END_EXPOSURES;
        NtGdiExtEscape( hdc, NULL, 0, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code,
                        sizeof(expose_rgn), (LPSTR)&expose_rgn );
        if (expose_rgn)
        {
            NtGdiCombineRgn( update, update, expose_rgn, RGN_OR );
            NtGdiDeleteObjectApp( expose_rgn );
        }
    }
    else ret = NtGdiBitBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                            hdc, rect.left - dx, rect.top - dy, SRCCOPY, 0, 0 );

    return ret;
}


/***********************************************************************
 *		SetCapture  (X11DRV.@)
 */
void X11DRV_SetCapture( HWND hwnd, UINT flags )
{
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    struct x11drv_win_data *data;

    if (!(flags & (GUI_INMOVESIZE | GUI_INMENUMODE))) return;

    if (hwnd)
    {
        if (!(data = get_win_data( NtUserGetAncestor( hwnd, GA_ROOT )))) return;
        if (data->whole_window)
        {
            XFlush( gdi_display );
            XGrabPointer( data->display, data->whole_window, False,
                          PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
                          GrabModeAsync, GrabModeAsync, None, None, CurrentTime );
            thread_data->grab_hwnd = data->hwnd;
        }
        release_win_data( data );
    }
    else  /* release capture */
    {
        if (!(data = get_win_data( thread_data->grab_hwnd ))) return;
        XFlush( gdi_display );
        XUngrabPointer( data->display, CurrentTime );
        XFlush( data->display );
        thread_data->grab_hwnd = NULL;
        release_win_data( data );
    }
}


/*****************************************************************
 *		SetParent   (X11DRV.@)
 */
void X11DRV_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    struct x11drv_win_data *data;

    if (parent == old_parent) return;
    if (!(data = get_win_data( hwnd ))) return;
    if (data->embedded) goto done;

    if (parent != NtUserGetDesktopWindow()) /* a child window */
    {
        if (old_parent == NtUserGetDesktopWindow())
        {
            /* destroy the old X windows */
            destroy_whole_window( data, FALSE );
        }
    }
    else  /* new top level window */
    {
        create_whole_window( data );
    }
done:
    release_win_data( data );
    set_gl_drawable_parent( hwnd, parent );

    /* Recreate the parent gl_drawable now that we know there are child windows
     * that will need clipping support.
     */
    sync_gl_drawable( parent, TRUE );

    fetch_icon_data( hwnd, 0, 0 );
}


/***********************************************************************
 *		WindowPosChanging   (X11DRV.@)
 */
BOOL X11DRV_WindowPosChanging( HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects )
{
    struct x11drv_win_data *data = get_win_data( hwnd );
    BOOL ret = FALSE;

    TRACE( "hwnd %p, swp_flags %#x, shaped %u, rects %s\n", hwnd, swp_flags, shaped, debugstr_window_rects( rects ) );

    if (!data && !(data = X11DRV_create_win_data( hwnd, rects ))) return FALSE; /* use default surface */
    data->shaped = shaped;

    ret = !!data->whole_window; /* use default surface if we don't have a window */
    release_win_data( data );

    return ret;
}

/***********************************************************************
 *      MoveWindowBits   (X11DRV.@)
 */
void X11DRV_MoveWindowBits( HWND hwnd, const struct window_rects *old_rects,
                            const struct window_rects *new_rects, const RECT *valid_rects )
{
    struct x11drv_win_data *data;
    Window window;

    if (!(data = get_win_data( hwnd ))) return;
    window = data->whole_window;
    release_win_data( data );

    /* if all that happened is that the whole window moved, copy everything */
    if (EqualRect( &valid_rects[0], &new_rects->window ) && EqualRect( &valid_rects[1], &old_rects->window ))
    {
        /* if we have an X window the bits will be moved by the X server */
        if (!window && (valid_rects[0].left - valid_rects[1].left || valid_rects[0].top - valid_rects[1].top))
            move_window_bits( hwnd, 0, old_rects, new_rects, valid_rects );
    }
    else
    {
        move_window_bits( hwnd, window, old_rects, new_rects, valid_rects );
    }
}

/***********************************************************************
 *      GetWindowStyleMasks   (X11DRV.@)
 */
BOOL X11DRV_GetWindowStyleMasks( HWND hwnd, UINT style, UINT ex_style, UINT *style_mask, UINT *ex_style_mask )
{
    unsigned long decor = get_mwm_decorations_for_style( style, ex_style );
    struct x11drv_win_data *data;

    if ((data = get_win_data( hwnd )))
    {
        if (!data->managed) decor = 0;
        release_win_data( data );
    }

    *style_mask = ex_style = 0;
    if (decor & MWM_DECOR_TITLE) *style_mask |= WS_CAPTION;
    if (decor & MWM_DECOR_BORDER)
    {
        *style_mask |= WS_DLGFRAME | WS_THICKFRAME;
        *ex_style_mask |= WS_EX_DLGMODALFRAME;
    }

    return TRUE;
}


/***********************************************************************
 *		WindowPosChanged   (X11DRV.@)
 */
void X11DRV_WindowPosChanged( HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                              const struct window_rects *new_rects, struct window_surface *surface )
{
    struct x11drv_win_data *data;
    UINT new_style = NtUserGetWindowLongW( hwnd, GWL_STYLE ), old_style;
    struct window_rects old_rects;
    BOOL was_fullscreen, activate = !(swp_flags & SWP_NOACTIVATE);

    sync_gl_drawable( hwnd, FALSE );

    if (!(data = get_win_data( hwnd ))) return;
    if (is_window_managed( hwnd, swp_flags, fullscreen )) window_set_managed( data, TRUE );

    old_style = new_style & ~(WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE);
    if (data->desired_state.wm_state != WithdrawnState) old_style |= WS_VISIBLE;
    if (data->desired_state.wm_state == IconicState) old_style |= WS_MINIMIZE;
    if (data->desired_state.net_wm_state & (1 << NET_WM_STATE_MAXIMIZED)) old_style |= WS_MAXIMIZE;

    old_rects = data->rects;
    was_fullscreen = data->is_fullscreen;
    data->rects = *new_rects;
    data->is_fullscreen = fullscreen;

    TRACE( "win %p/%lx new_rects %s style %08x flags %08x\n", hwnd, data->whole_window,
           debugstr_window_rects(new_rects), new_style, swp_flags );

    XFlush( gdi_display );  /* make sure painting is done before we move the window */

    sync_client_position( data, &old_rects );

    if (!data->whole_window)
    {
        release_win_data( data );
        return;
    }

    if (old_style & WS_VISIBLE)
    {
        if (((swp_flags & SWP_HIDEWINDOW) && !(new_style & WS_VISIBLE)) ||
            (!(new_style & WS_MINIMIZE) && !is_window_rect_mapped( &new_rects->window ) && is_window_rect_mapped( &old_rects.window )))
        {
            window_set_wm_state( data, WithdrawnState, FALSE );
            release_win_data( data );
            if (was_fullscreen) NtUserClipCursor( NULL );
            if (!(data = get_win_data( hwnd ))) return;
        }
    }

    /* don't change position if we are about to minimize or maximize a managed window */
    if (!(data->managed && (swp_flags & SWP_STATECHANGED) && (new_style & (WS_MINIMIZE|WS_MAXIMIZE))))
    {
        sync_window_position( data, swp_flags, &old_rects );
#ifdef HAVE_LIBXSHAPE
        if (IsRectEmpty( &old_rects.window ) != IsRectEmpty( &new_rects->window ))
            sync_empty_window_shape( data, surface );
        if (data->shaped)
        {
            int old_x_offset = old_rects.window.left - old_rects.visible.left;
            int old_y_offset = old_rects.window.top - old_rects.visible.top;
            int new_x_offset = new_rects->window.left - new_rects->visible.left;
            int new_y_offset = new_rects->window.top - new_rects->visible.top;
            if (old_x_offset != new_x_offset || old_y_offset != new_y_offset)
                XShapeOffsetShape( data->display, data->whole_window, ShapeBounding,
                                   new_x_offset - old_x_offset, new_y_offset - old_y_offset );
        }
#endif
    }

    if ((new_style & WS_VISIBLE) &&
        ((new_style & WS_MINIMIZE) || is_window_rect_mapped( &new_rects->window )))
    {
        if (!(old_style & WS_VISIBLE))
        {
            BOOL needs_icon = !data->icon_pixmap;
            BOOL needs_map = TRUE;

            /* layered windows are mapped only once their attributes are set */
            if (NtUserGetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED)
                needs_map = data->layered || IsRectEmpty( &new_rects->window );
            release_win_data( data );
            if (needs_icon) fetch_icon_data( hwnd, 0, 0 );
            if (needs_map) map_window( hwnd, new_style, activate );
            return;
        }
        else if ((swp_flags & SWP_STATECHANGED) && ((old_style ^ new_style) & WS_MINIMIZE))
        {
            window_set_wm_state( data, (new_style & WS_MINIMIZE) ? IconicState : NormalState, activate );
            update_net_wm_states( data );
        }
        else
        {
            if (swp_flags & (SWP_FRAMECHANGED|SWP_STATECHANGED)) set_wm_hints( data );
            update_net_wm_states( data );
        }
    }

    XFlush( data->display );  /* make sure changes are done before we start painting again */
    release_win_data( data );
}

/* check if the window icon should be hidden (i.e. moved off-screen) */
static BOOL hide_icon( struct x11drv_win_data *data )
{
    static const WCHAR trayW[] = {'S','h','e','l','l','_','T','r','a','y','W','n','d',0};
    UNICODE_STRING str = RTL_CONSTANT_STRING( trayW );

    if (data->managed) return TRUE;
    /* hide icons in desktop mode when the taskbar is active */
    if (!is_virtual_desktop()) return FALSE;
    return NtUserIsWindowVisible( NtUserFindWindowEx( 0, 0, &str, NULL, 0 ));
}

/***********************************************************************
 *           ShowWindow   (X11DRV.@)
 */
UINT X11DRV_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    int x, y;
    unsigned int width, height, border, depth;
    Window root, top;
    POINT pos;
    DWORD style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    struct x11drv_thread_data *thread_data = x11drv_thread_data();
    struct x11drv_win_data *data = get_win_data( hwnd );

    if (!data || !data->whole_window) goto done;
    if (style & WS_MINIMIZE)
    {
        if (((rect->left != -32000 || rect->top != -32000)) && hide_icon( data ))
        {
            OffsetRect( rect, -32000 - rect->left, -32000 - rect->top );
            swp &= ~(SWP_NOMOVE | SWP_NOCLIENTMOVE);
        }
        goto done;
    }
    if (!data->managed || data->desired_state.wm_state != NormalState) goto done;

    /* only fetch the new rectangle if the ShowWindow was a result of a window manager event */

    if (!thread_data->current_event || thread_data->current_event->xany.window != data->whole_window)
        goto done;

    if (thread_data->current_event->type != ConfigureNotify &&
        thread_data->current_event->type != PropertyNotify)
        goto done;

    TRACE( "win %p/%lx cmd %d at %s flags %08x\n",
           hwnd, data->whole_window, cmd, wine_dbgstr_rect(rect), swp );

    XGetGeometry( thread_data->display, data->whole_window,
                  &root, &x, &y, &width, &height, &border, &depth );
    XTranslateCoordinates( thread_data->display, data->whole_window, root, 0, 0, &x, &y, &top );
    pos = root_to_virtual_screen( x, y );
    SetRect( rect, pos.x, pos.y, pos.x + width, pos.y + height );
    *rect = window_rect_from_visible( &data->rects, *rect );
    swp &= ~(SWP_NOMOVE | SWP_NOCLIENTMOVE | SWP_NOSIZE | SWP_NOCLIENTSIZE);

done:
    release_win_data( data );
    return swp;
}


/**********************************************************************
 *		SetWindowIcon (X11DRV.@)
 *
 * hIcon or hIconSm has changed (or is being initialised for the
 * first time). Complete the X11 driver-specific initialisation
 * and set the window hints.
 */
void X11DRV_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
    struct x11drv_win_data *data;

    if (!(data = get_win_data( hwnd ))) return;
    if (!data->whole_window) goto done;
    release_win_data( data );  /* release the lock, fetching the icon requires sending messages */

    if (type == ICON_BIG) fetch_icon_data( hwnd, icon, 0 );
    else fetch_icon_data( hwnd, 0, icon );

    if (!(data = get_win_data( hwnd ))) return;
    set_wm_hints( data );
done:
    release_win_data( data );
}


/***********************************************************************
 *		SetWindowRgn  (X11DRV.@)
 *
 * Assign specified region to window (for non-rectangular windows)
 */
void X11DRV_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    struct x11drv_win_data *data;

    if ((data = get_win_data( hwnd )))
    {
        sync_window_region( data, hrgn );
        release_win_data( data );
    }
    else if (X11DRV_get_whole_window( hwnd ))
    {
        send_message( hwnd, WM_X11DRV_SET_WIN_REGION, 0, 0 );
    }
}


/***********************************************************************
 *		SetLayeredWindowAttributes  (X11DRV.@)
 *
 * Set transparency attributes for a layered window.
 */
void X11DRV_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    struct x11drv_win_data *data = get_win_data( hwnd );

    if (data)
    {
        set_window_visual( data, &default_visual, FALSE );

        if (data->whole_window)
            sync_window_opacity( data->display, data->whole_window, alpha, flags );

        data->layered = TRUE;
        if (data->desired_state.wm_state == WithdrawnState)  /* mapping is delayed until attributes are set */
        {
            DWORD style = NtUserGetWindowLongW( data->hwnd, GWL_STYLE );

            if ((style & WS_VISIBLE) &&
                ((style & WS_MINIMIZE) || is_window_rect_mapped( &data->rects.window )))
            {
                release_win_data( data );
                map_window( hwnd, style, TRUE );
                return;
            }
        }
        release_win_data( data );
    }
    else
    {
        Window win = X11DRV_get_whole_window( hwnd );
        if (win)
        {
            sync_window_opacity( gdi_display, win, alpha, flags );
            if (flags & LWA_COLORKEY)
                FIXME( "LWA_COLORKEY not supported on foreign process window %p\n", hwnd );
        }
    }
}


/***********************************************************************
 *              UpdateLayeredWindow   (X11DRV.@)
 */
void X11DRV_UpdateLayeredWindow( HWND hwnd, BYTE alpha, UINT flags )
{
    struct x11drv_win_data *data;
    BOOL mapped;

    if (!(data = get_win_data( hwnd ))) return;

    if (data->whole_window)
        sync_window_opacity( data->display, data->whole_window, alpha, flags );

    mapped = data->desired_state.wm_state != WithdrawnState;
    release_win_data( data );

    /* layered windows are mapped only once their attributes are set */
    if (!mapped)
    {
        DWORD style = NtUserGetWindowLongW( hwnd, GWL_STYLE );

        if ((style & WS_VISIBLE) && ((style & WS_MINIMIZE) || is_window_rect_mapped( &data->rects.window )))
            map_window( hwnd, style, TRUE );
    }
}


/* Add a window to taskbar */
static void taskbar_add_tab( HWND hwnd )
{
    struct x11drv_win_data *data;

    TRACE("hwnd %p\n", hwnd);

    data = get_win_data( hwnd );
    if (!data)
        return;

    data->add_taskbar = TRUE;
    data->skip_taskbar = FALSE;
    update_net_wm_states( data );
    release_win_data( data );
}

/* Delete a window from taskbar */
static void taskbar_delete_tab( HWND hwnd )
{
    struct x11drv_win_data *data;

    TRACE("hwnd %p\n", hwnd);

    data = get_win_data( hwnd );
    if (!data)
        return;

    data->skip_taskbar = TRUE;
    data->add_taskbar = FALSE;
    update_net_wm_states( data );
    release_win_data( data );
}

/**********************************************************************
 *           X11DRV_WindowMessage   (X11DRV.@)
 */
LRESULT X11DRV_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    struct x11drv_win_data *data;

    switch(msg)
    {
    case WM_X11DRV_UPDATE_CLIPBOARD:
        return update_clipboard( hwnd );
    case WM_X11DRV_SET_WIN_REGION:
        if ((data = get_win_data( hwnd )))
        {
            sync_window_region( data, (HRGN)1 );
            release_win_data( data );
        }
        return 0;
    case WM_X11DRV_DELETE_TAB:
        taskbar_delete_tab( hwnd );
        return 0;
    case WM_X11DRV_ADD_TAB:
        taskbar_add_tab( hwnd );
        return 0;
    default:
        FIXME( "got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, (long)wp, lp );
        return 0;
    }
}


/***********************************************************************
 *              is_net_supported
 */
BOOL is_net_supported( Atom atom )
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    BOOL supported;
    int i;

    for (i = 0; i < data->net_supported_count; i++)
        if (data->net_supported[i] == atom) break;
    supported = i < data->net_supported_count;

    return supported;
}


/***********************************************************************
 *              start_screensaver
 */
static LRESULT start_screensaver(void)
{
    if (!is_virtual_desktop())
    {
        const char *argv[3] = { "xdg-screensaver", "activate", NULL };
        int pid = __wine_unix_spawnvp( (char **)argv, FALSE );
        if (pid > 0)
        {
            TRACE( "started process %d\n", pid );
            return 0;
        }
    }
    return -1;
}


/***********************************************************************
 *           SysCommand   (X11DRV.@)
 *
 * Perform WM_SYSCOMMAND handling.
 */
LRESULT X11DRV_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam, const POINT *pos )
{
    WPARAM hittest = wparam & 0x0f;
    int dir;
    struct x11drv_win_data *data;

    if (!(data = get_win_data( hwnd )))
    {
        if (wparam == SC_SCREENSAVE && hwnd == NtUserGetDesktopWindow()) return start_screensaver();
        return -1;
    }
    if (!data->whole_window || !data->managed || data->desired_state.wm_state == WithdrawnState) goto failed;

    switch (wparam & 0xfff0)
    {
    case SC_MOVE:
        if (!hittest) dir = _NET_WM_MOVERESIZE_MOVE_KEYBOARD;
        else dir = _NET_WM_MOVERESIZE_MOVE;
        break;
    case SC_SIZE:
        /* windows without WS_THICKFRAME are not resizable through the window manager */
        if (!(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_THICKFRAME)) goto failed;

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
        case 9:                dir = _NET_WM_MOVERESIZE_MOVE; break;
        default:               dir = _NET_WM_MOVERESIZE_SIZE_KEYBOARD; break;
        }
        break;

    case SC_KEYMENU:
        /* prevent a simple ALT press+release from activating the system menu,
         * as that can get confusing on managed windows */
        if ((WCHAR)lparam) goto failed;  /* got an explicit char */
        if (NtUserGetWindowLongPtrW( hwnd, GWLP_ID )) goto failed;  /* window has a real menu */
        if (!(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_SYSMENU)) goto failed;  /* no system menu */
        TRACE( "ignoring SC_KEYMENU wp %lx lp %lx\n", (long)wparam, lparam );
        release_win_data( data );
        return 0;

    default:
        goto failed;
    }

    if (NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_MAXIMIZE) goto failed;

    if (!is_net_supported( x11drv_atom(_NET_WM_MOVERESIZE) ))
    {
        TRACE( "_NET_WM_MOVERESIZE not supported\n" );
        goto failed;
    }

    release_win_data( data );
    move_resize_window( hwnd, dir, *pos );
    return 0;

failed:
    release_win_data( data );
    return -1;
}

void X11DRV_FlashWindowEx( FLASHWINFO *pfinfo )
{
    struct x11drv_win_data *data = get_win_data( pfinfo->hwnd );
    XEvent xev;

    if (!data)
        return;

    if (data->pending_state.wm_state != WithdrawnState)
    {
        xev.type = ClientMessage;
        xev.xclient.window = data->whole_window;
        xev.xclient.message_type = x11drv_atom( _NET_WM_STATE );
        xev.xclient.serial = 0;
        xev.xclient.display = data->display;
        xev.xclient.send_event = True;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = pfinfo->dwFlags ?  _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        xev.xclient.data.l[1] = x11drv_atom( _NET_WM_STATE_DEMANDS_ATTENTION );
        xev.xclient.data.l[2] = 0;
        xev.xclient.data.l[3] = 1;
        xev.xclient.data.l[4] = 0;

        XSendEvent( data->display, DefaultRootWindow( data->display ), False,
                    SubstructureNotifyMask, &xev );
    }
    release_win_data( data );
}

void net_supported_init( struct x11drv_thread_data *data )
{
    unsigned long count, remaining;
    int format, i;
    Atom type;

    if (!XGetWindowProperty( data->display, DefaultRootWindow( data->display ), x11drv_atom(_NET_SUPPORTED), 0, 65536 / sizeof(CARD32),
                             False, XA_ATOM, &type, &format, &count, &remaining, (unsigned char **)&data->net_supported ))
        data->net_supported_count = get_property_size( format, count ) / sizeof(Atom);

    for (i = 0; i < NB_NET_WM_STATES; i++)
    {
        Atom atom = X11DRV_Atoms[net_wm_state_atoms[i] - FIRST_XATOM];
        if (is_net_supported( atom )) data->net_wm_state_mask |= (1 << i);
    }
}

void init_win_context(void)
{
    init_recursive_mutex( &win_data_mutex );

    winContext = XUniqueContext();
    win_data_context = XUniqueContext();
    host_window_context = XUniqueContext();
    cursor_context = XUniqueContext();
}
