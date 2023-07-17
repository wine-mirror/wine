/*
 * Window related functions
 *
 * Copyright 1993, 1994, 1995, 1996, 2001, 2013-2017 Alexandre Julliard
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

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define OEMRESOURCE
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "android.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

/* private window data */
struct android_win_data
{
    HWND           hwnd;           /* hwnd that this private data belongs to */
    HWND           parent;         /* parent hwnd for child windows */
    RECT           window_rect;    /* USER window rectangle relative to parent */
    RECT           whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT           client_rect;    /* client area relative to parent */
    ANativeWindow *window;         /* native window wrapper that forwards calls to the desktop process */
    struct window_surface *surface;
};

#define SWP_AGG_NOPOSCHANGE (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

pthread_mutex_t win_data_mutex;

static struct android_win_data *win_data_context[32768];

static inline int context_idx( HWND hwnd )
{
    return LOWORD( hwnd ) >> 1;
}

static void set_surface_region( struct window_surface *window_surface, HRGN win_region );

/* only for use on sanitized BITMAPINFO structures */
static inline int get_dib_info_size( const BITMAPINFO *info, UINT coloruse )
{
    if (info->bmiHeader.biCompression == BI_BITFIELDS)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD);
    if (coloruse == DIB_PAL_COLORS)
        return sizeof(BITMAPINFOHEADER) + info->bmiHeader.biClrUsed * sizeof(WORD);
    return FIELD_OFFSET( BITMAPINFO, bmiColors[info->bmiHeader.biClrUsed] );
}

static inline int get_dib_stride( int width, int bpp )
{
    return ((width * bpp + 31) >> 3) & ~3;
}

static inline int get_dib_image_size( const BITMAPINFO *info )
{
    return get_dib_stride( info->bmiHeader.biWidth, info->bmiHeader.biBitCount )
        * abs( info->bmiHeader.biHeight );
}

static BOOL intersect_rect( RECT *dst, const RECT *src1, const RECT *src2 )
{
    dst->left   = max(src1->left, src2->left);
    dst->top    = max(src1->top, src2->top);
    dst->right  = min(src1->right, src2->right);
    dst->bottom = min(src1->bottom, src2->bottom);
    return !IsRectEmpty( dst );
}


/**********************************************************************
 *	     get_win_monitor_dpi
 */
static UINT get_win_monitor_dpi( HWND hwnd )
{
    return NtUserGetSystemDpiForProcess( NULL );  /* FIXME: get monitor dpi */
}


/***********************************************************************
 *           alloc_win_data
 */
static struct android_win_data *alloc_win_data( HWND hwnd )
{
    struct android_win_data *data;

    if ((data = calloc( 1, sizeof(*data) )))
    {
        data->hwnd = hwnd;
        data->window = create_ioctl_window( hwnd, FALSE,
                                            (float)get_win_monitor_dpi( hwnd ) / NtUserGetDpiForWindow( hwnd ));
        pthread_mutex_lock( &win_data_mutex );
        win_data_context[context_idx(hwnd)] = data;
    }
    return data;
}


/***********************************************************************
 *           free_win_data
 */
static void free_win_data( struct android_win_data *data )
{
    win_data_context[context_idx( data->hwnd )] = NULL;
    pthread_mutex_unlock( &win_data_mutex );
    if (data->window) release_ioctl_window( data->window );
    free( data );
}


/***********************************************************************
 *           get_win_data
 *
 * Lock and return the data structure associated with a window.
 */
static struct android_win_data *get_win_data( HWND hwnd )
{
    struct android_win_data *data;

    if (!hwnd) return NULL;
    pthread_mutex_lock( &win_data_mutex );
    if ((data = win_data_context[context_idx(hwnd)]) && data->hwnd == hwnd) return data;
    pthread_mutex_unlock( &win_data_mutex );
    return NULL;
}


/***********************************************************************
 *           release_win_data
 *
 * Release the data returned by get_win_data.
 */
static void release_win_data( struct android_win_data *data )
{
    if (data) pthread_mutex_unlock( &win_data_mutex );
}


/***********************************************************************
 *           get_ioctl_window
 */
static struct ANativeWindow *get_ioctl_window( HWND hwnd )
{
    struct ANativeWindow *ret;
    struct android_win_data *data = get_win_data( hwnd );

    if (!data || !data->window) return NULL;
    ret = grab_ioctl_window( data->window );
    release_win_data( data );
    return ret;
}


/* Handling of events coming from the Java side */

struct java_event
{
    struct list      entry;
    union event_data data;
};

static struct list event_queue = LIST_INIT( event_queue );
static struct java_event *current_event;
static int event_pipe[2];
static DWORD desktop_tid;

/***********************************************************************
 *           send_event
 */
int send_event( const union event_data *data )
{
    int res;

    if ((res = write( event_pipe[1], data, sizeof(*data) )) != sizeof(*data))
    {
        p__android_log_print( ANDROID_LOG_ERROR, "wine", "failed to send event" );
        return -1;
    }
    return 0;
}


/***********************************************************************
 *           desktop_changed
 *
 * JNI callback, runs in the context of the Java thread.
 */
void desktop_changed( JNIEnv *env, jobject obj, jint width, jint height )
{
    union event_data data;

    memset( &data, 0, sizeof(data) );
    data.type = DESKTOP_CHANGED;
    data.desktop.width = width;
    data.desktop.height = height;
    p__android_log_print( ANDROID_LOG_INFO, "wine", "desktop_changed: %ux%u", width, height );
    send_event( &data );
}


/***********************************************************************
 *           config_changed
 *
 * JNI callback, runs in the context of the Java thread.
 */
void config_changed( JNIEnv *env, jobject obj, jint dpi )
{
    union event_data data;

    memset( &data, 0, sizeof(data) );
    data.type = CONFIG_CHANGED;
    data.cfg.dpi = dpi;
    p__android_log_print( ANDROID_LOG_INFO, "wine", "config_changed: %u dpi", dpi );
    send_event( &data );
}


/***********************************************************************
 *           surface_changed
 *
 * JNI callback, runs in the context of the Java thread.
 */
void surface_changed( JNIEnv *env, jobject obj, jint win, jobject surface, jboolean client )
{
    union event_data data;

    memset( &data, 0, sizeof(data) );
    data.surface.hwnd = LongToHandle( win );
    data.surface.client = client;
    if (surface)
    {
        int width, height;
        ANativeWindow *win = pANativeWindow_fromSurface( env, surface );

        if (win->query( win, NATIVE_WINDOW_WIDTH, &width ) < 0) width = 0;
        if (win->query( win, NATIVE_WINDOW_HEIGHT, &height ) < 0) height = 0;
        data.surface.window = win;
        data.surface.width = width;
        data.surface.height = height;
        p__android_log_print( ANDROID_LOG_INFO, "wine", "surface_changed: %p %s %ux%u",
                              data.surface.hwnd, client ? "client" : "whole", width, height );
    }
    data.type = SURFACE_CHANGED;
    send_event( &data );
}


/***********************************************************************
 *           motion_event
 *
 * JNI callback, runs in the context of the Java thread.
 */
jboolean motion_event( JNIEnv *env, jobject obj, jint win, jint action, jint x, jint y, jint state, jint vscroll )
{
    static LONG button_state;
    union event_data data;
    int prev_state;

    int mask = action & AMOTION_EVENT_ACTION_MASK;

    if (!( mask == AMOTION_EVENT_ACTION_DOWN ||
           mask == AMOTION_EVENT_ACTION_UP ||
           mask == AMOTION_EVENT_ACTION_CANCEL ||
           mask == AMOTION_EVENT_ACTION_SCROLL ||
           mask == AMOTION_EVENT_ACTION_MOVE ||
           mask == AMOTION_EVENT_ACTION_HOVER_MOVE ||
           mask == AMOTION_EVENT_ACTION_BUTTON_PRESS ||
           mask == AMOTION_EVENT_ACTION_BUTTON_RELEASE ))
        return JNI_FALSE;

    /* make sure a subsequent AMOTION_EVENT_ACTION_UP is not treated as a touch event */
    if (mask == AMOTION_EVENT_ACTION_BUTTON_RELEASE) state |= 0x80000000;

    prev_state = InterlockedExchange( &button_state, state );

    data.type = MOTION_EVENT;
    data.motion.hwnd = LongToHandle( win );
    data.motion.input.type           = INPUT_MOUSE;
    data.motion.input.mi.dx          = x;
    data.motion.input.mi.dy          = y;
    data.motion.input.mi.mouseData   = 0;
    data.motion.input.mi.time        = 0;
    data.motion.input.mi.dwExtraInfo = 0;
    data.motion.input.mi.dwFlags     = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    switch (action & AMOTION_EVENT_ACTION_MASK)
    {
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_BUTTON_PRESS:
        if ((state & ~prev_state) & AMOTION_EVENT_BUTTON_PRIMARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
        if ((state & ~prev_state) & AMOTION_EVENT_BUTTON_SECONDARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
        if ((state & ~prev_state) & AMOTION_EVENT_BUTTON_TERTIARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
        if (!(state & ~prev_state)) /* touch event */
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
        break;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_CANCEL:
    case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
        if ((prev_state & ~state) & AMOTION_EVENT_BUTTON_PRIMARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
        if ((prev_state & ~state) & AMOTION_EVENT_BUTTON_SECONDARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
        if ((prev_state & ~state) & AMOTION_EVENT_BUTTON_TERTIARY)
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
        if (!(prev_state & ~state)) /* touch event */
            data.motion.input.mi.dwFlags |= MOUSEEVENTF_LEFTUP;
        break;
    case AMOTION_EVENT_ACTION_SCROLL:
        data.motion.input.mi.dwFlags |= MOUSEEVENTF_WHEEL;
        data.motion.input.mi.mouseData = vscroll < 0 ? -WHEEL_DELTA : WHEEL_DELTA;
        break;
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
        break;
    default:
        return JNI_FALSE;
    }
    send_event( &data );
    return JNI_TRUE;
}


/***********************************************************************
 *           init_event_queue
 */
static void init_event_queue(void)
{
    HANDLE handle;
    int ret;

    if (pipe2( event_pipe, O_CLOEXEC | O_NONBLOCK ) == -1)
    {
        ERR( "could not create data\n" );
        NtTerminateProcess( 0, 1 );
    }
    if (wine_server_fd_to_handle( event_pipe[0], GENERIC_READ | SYNCHRONIZE, 0, &handle ))
    {
        ERR( "Can't allocate handle for event fd\n" );
        NtTerminateProcess( 0, 1 );
    }
    SERVER_START_REQ( set_queue_fd )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (ret)
    {
        ERR( "Can't store handle for event fd %x\n", ret );
        NtTerminateProcess( 0, 1 );
    }
    NtClose( handle );
    desktop_tid = GetCurrentThreadId();
}


/***********************************************************************
 *           pull_events
 *
 * Pull events from the event pipe and add them to the queue
 */
static void pull_events(void)
{
    struct java_event *event;
    int res;

    for (;;)
    {
        if (!(event = malloc( sizeof(*event) ))) break;

        res = read( event_pipe[0], &event->data, sizeof(event->data) );
        if (res != sizeof(event->data)) break;
        list_add_tail( &event_queue, &event->entry );
    }
    free( event );
}


/***********************************************************************
 *           process_events
 */
static int process_events( DWORD mask )
{
    DPI_AWARENESS_CONTEXT context;
    struct java_event *event, *next, *previous;
    unsigned int count = 0;

    assert( GetCurrentThreadId() == desktop_tid );

    pull_events();

    previous = current_event;

    LIST_FOR_EACH_ENTRY_SAFE( event, next, &event_queue, struct java_event, entry )
    {
        switch (event->data.type)
        {
        case SURFACE_CHANGED:
            break;  /* always process it to unblock other threads */
        case MOTION_EVENT:
            if (event->data.motion.input.mi.dwFlags & (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_RIGHTDOWN|
                                                       MOUSEEVENTF_MIDDLEDOWN|MOUSEEVENTF_LEFTUP|
                                                       MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEUP))
            {
                if (mask & QS_MOUSEBUTTON) break;
            }
            else if (mask & QS_MOUSEMOVE) break;
            continue;  /* skip it */
        case KEYBOARD_EVENT:
            if (mask & QS_KEY) break;
            continue;  /* skip it */
        default:
            if (mask & QS_SENDMESSAGE) break;
            continue;  /* skip it */
        }

        /* remove it first, in case we process events recursively */
        list_remove( &event->entry );
        current_event = event;

        switch (event->data.type)
        {
        case DESKTOP_CHANGED:
            TRACE( "DESKTOP_CHANGED %ux%u\n", event->data.desktop.width, event->data.desktop.height );
            context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
            screen_width = event->data.desktop.width;
            screen_height = event->data.desktop.height;
            init_monitors( screen_width, screen_height );
            NtUserSetWindowPos( NtUserGetDesktopWindow(), 0, 0, 0, screen_width, screen_height,
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW );
            SetThreadDpiAwarenessContext( context );
            break;

        case CONFIG_CHANGED:
            TRACE( "CONFIG_CHANGED dpi %u\n", event->data.cfg.dpi );
            set_screen_dpi( event->data.cfg.dpi );
            break;

        case SURFACE_CHANGED:
            TRACE("SURFACE_CHANGED %p %p %s size %ux%u\n", event->data.surface.hwnd,
                  event->data.surface.window, event->data.surface.client ? "client" : "whole",
                  event->data.surface.width, event->data.surface.height );

            register_native_window( event->data.surface.hwnd, event->data.surface.window, event->data.surface.client );
            break;

        case MOTION_EVENT:
            {
                HWND capture = get_capture_window();

                if (event->data.motion.input.mi.dwFlags & (MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_RIGHTDOWN|MOUSEEVENTF_MIDDLEDOWN))
                    TRACE( "BUTTONDOWN pos %d,%d hwnd %p flags %x\n",
                           (int)event->data.motion.input.mi.dx, (int)event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, (int)event->data.motion.input.mi.dwFlags );
                else if (event->data.motion.input.mi.dwFlags & (MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEUP))
                    TRACE( "BUTTONUP pos %d,%d hwnd %p flags %x\n",
                           (int)event->data.motion.input.mi.dx, (int)event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, (int)event->data.motion.input.mi.dwFlags );
                else
                    TRACE( "MOUSEMOVE pos %d,%d hwnd %p flags %x\n",
                           (int)event->data.motion.input.mi.dx, (int)event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, (int)event->data.motion.input.mi.dwFlags );
                if (!capture && (event->data.motion.input.mi.dwFlags & MOUSEEVENTF_ABSOLUTE))
                {
                    RECT rect;
                    SetRect( &rect, event->data.motion.input.mi.dx, event->data.motion.input.mi.dy,
                             event->data.motion.input.mi.dx + 1, event->data.motion.input.mi.dy + 1 );

                    SERVER_START_REQ( update_window_zorder )
                    {
                        req->window      = wine_server_user_handle( event->data.motion.hwnd );
                        req->rect.left   = rect.left;
                        req->rect.top    = rect.top;
                        req->rect.right  = rect.right;
                        req->rect.bottom = rect.bottom;
                        wine_server_call( req );
                    }
                    SERVER_END_REQ;
                }
                __wine_send_input( capture ? capture : event->data.motion.hwnd, &event->data.motion.input, NULL );
            }
            break;

        case KEYBOARD_EVENT:
            if (event->data.kbd.input.ki.dwFlags & KEYEVENTF_KEYUP)
                TRACE("KEYUP hwnd %p vkey %x '%c' scancode %x\n", event->data.kbd.hwnd,
                      event->data.kbd.input.ki.wVk, event->data.kbd.input.ki.wVk,
                      event->data.kbd.input.ki.wScan );
            else
                TRACE("KEYDOWN hwnd %p vkey %x '%c' scancode %x\n", event->data.kbd.hwnd,
                      event->data.kbd.input.ki.wVk, event->data.kbd.input.ki.wVk,
                      event->data.kbd.input.ki.wScan );
            update_keyboard_lock_state( event->data.kbd.input.ki.wVk, event->data.kbd.lock_state );
            __wine_send_input( 0, &event->data.kbd.input, NULL );
            break;

        default:
            FIXME( "got event %u\n", event->data.type );
        }
        free( event );
        count++;
        /* next may have been removed by a recursive call, so reset it to the beginning of the list */
        next = LIST_ENTRY( event_queue.next, struct java_event, entry );
    }
    current_event = previous;
    return count;
}


/***********************************************************************
 *           wait_events
 */
static int wait_events( int timeout )
{
    assert( GetCurrentThreadId() == desktop_tid );

    for (;;)
    {
        struct pollfd pollfd;
        int ret;

        pollfd.fd = event_pipe[0];
        pollfd.events = POLLIN | POLLHUP;
        ret = poll( &pollfd, 1, timeout );
        if (ret == -1 && errno == EINTR) continue;
        if (ret && (pollfd.revents & (POLLHUP | POLLERR))) ret = -1;
        return ret;
    }
}


/* Window surface support */

struct android_window_surface
{
    struct window_surface header;
    HWND                  hwnd;
    ANativeWindow        *window;
    RECT                  bounds;
    BOOL                  byteswap;
    RGNDATA              *region_data;
    HRGN                  region;
    BYTE                  alpha;
    COLORREF              color_key;
    void                 *bits;
    pthread_mutex_t       mutex;
    BITMAPINFO            info;   /* variable size, must be last */
};

static struct android_window_surface *get_android_surface( struct window_surface *surface )
{
    return (struct android_window_surface *)surface;
}

static inline void reset_bounds( RECT *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static inline void add_bounds_rect( RECT *bounds, const RECT *rect )
{
    if (rect->left >= rect->right || rect->top >= rect->bottom) return;
    bounds->left   = min( bounds->left, rect->left );
    bounds->top    = min( bounds->top, rect->top );
    bounds->right  = max( bounds->right, rect->right );
    bounds->bottom = max( bounds->bottom, rect->bottom );
}

/* store the palette or color mask data in the bitmap info structure */
static void set_color_info( BITMAPINFO *info, BOOL has_alpha )
{
    DWORD *colors = (DWORD *)info->bmiColors;

    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biBitCount = 32;
    if (has_alpha)
    {
        info->bmiHeader.biCompression = BI_RGB;
        return;
    }
    info->bmiHeader.biCompression = BI_BITFIELDS;
    colors[0] = 0xff0000;
    colors[1] = 0x00ff00;
    colors[2] = 0x0000ff;
}

/* apply the window region to a single line of the destination image. */
static void apply_line_region( DWORD *dst, int width, int x, int y, const RECT *rect, const RECT *end )
{
    while (rect < end && rect->top <= y && width > 0)
    {
        if (rect->left > x)
        {
            memset( dst, 0, min( rect->left - x, width ) * sizeof(*dst) );
            dst += rect->left - x;
            width -= rect->left - x;
            x = rect->left;
        }
        if (rect->right > x)
        {
            dst += rect->right - x;
            width -= rect->right - x;
            x = rect->right;
        }
        rect++;
    }
    if (width > 0) memset( dst, 0, width * sizeof(*dst) );
}

/***********************************************************************
 *           android_surface_lock
 */
static void android_surface_lock( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    pthread_mutex_lock( &surface->mutex );
}

/***********************************************************************
 *           android_surface_unlock
 */
static void android_surface_unlock( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    pthread_mutex_unlock( &surface->mutex );
}

/***********************************************************************
 *           android_surface_get_bitmap_info
 */
static void *android_surface_get_bitmap_info( struct window_surface *window_surface, BITMAPINFO *info )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    memcpy( info, &surface->info, get_dib_info_size( &surface->info, DIB_RGB_COLORS ));
    return surface->bits;
}

/***********************************************************************
 *           android_surface_get_bounds
 */
static RECT *android_surface_get_bounds( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    return &surface->bounds;
}

/***********************************************************************
 *           android_surface_set_region
 */
static void android_surface_set_region( struct window_surface *window_surface, HRGN region )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    TRACE( "updating surface %p hwnd %p with %p\n", surface, surface->hwnd, region );

    window_surface->funcs->lock( window_surface );
    if (!region)
    {
        if (surface->region) NtGdiDeleteObjectApp( surface->region );
        surface->region = 0;
    }
    else
    {
        if (!surface->region) surface->region = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        NtGdiCombineRgn( surface->region, region, 0, RGN_COPY );
    }
    window_surface->funcs->unlock( window_surface );
    set_surface_region( &surface->header, (HRGN)1 );
}

/***********************************************************************
 *           android_surface_flush
 */
static void android_surface_flush( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );
    ANativeWindow_Buffer buffer;
    ARect rc;
    RECT rect;
    BOOL needs_flush;

    window_surface->funcs->lock( window_surface );
    SetRect( &rect, 0, 0, surface->header.rect.right - surface->header.rect.left,
             surface->header.rect.bottom - surface->header.rect.top );
    needs_flush = intersect_rect( &rect, &rect, &surface->bounds );
    reset_bounds( &surface->bounds );
    window_surface->funcs->unlock( window_surface );
    if (!needs_flush) return;

    TRACE( "flushing %p hwnd %p surface %s rect %s bits %p alpha %02x key %08x region %u rects\n",
           surface, surface->hwnd, wine_dbgstr_rect( &surface->header.rect ),
           wine_dbgstr_rect( &rect ), surface->bits, surface->alpha, (int)surface->color_key,
           surface->region_data ? (int)surface->region_data->rdh.nCount : 0 );

    rc.left   = rect.left;
    rc.top    = rect.top;
    rc.right  = rect.right;
    rc.bottom = rect.bottom;

    if (!surface->window->perform( surface->window, NATIVE_WINDOW_LOCK, &buffer, &rc ))
    {
        const RECT *rgn_rect = NULL, *end = NULL;
        unsigned int *src, *dst;
        int x, y, width;

        rect.left   = rc.left;
        rect.top    = rc.top;
        rect.right  = rc.right;
        rect.bottom = rc.bottom;
        intersect_rect( &rect, &rect, &surface->header.rect );

        if (surface->region_data)
        {
            rgn_rect = (RECT *)surface->region_data->Buffer;
            end = rgn_rect + surface->region_data->rdh.nCount;
        }
        src = (unsigned int *)surface->bits
            + (rect.top - surface->header.rect.top) * surface->info.bmiHeader.biWidth
            + (rect.left - surface->header.rect.left);
        dst = (unsigned int *)buffer.bits + rect.top * buffer.stride + rect.left;
        width = min( rect.right - rect.left, buffer.stride );

        for (y = rect.top; y < min( buffer.height, rect.bottom); y++)
        {
            if (surface->info.bmiHeader.biCompression == BI_RGB)
                memcpy( dst, src, width * sizeof(*dst) );
            else if (surface->alpha == 255)
                for (x = 0; x < width; x++) dst[x] = src[x] | 0xff000000;
            else
                for (x = 0; x < width; x++)
                    dst[x] = ((surface->alpha << 24) |
                              (((BYTE)(src[x] >> 16) * surface->alpha / 255) << 16) |
                              (((BYTE)(src[x] >> 8) * surface->alpha / 255) << 8) |
                              (((BYTE)src[x] * surface->alpha / 255)));

            if (surface->color_key != CLR_INVALID)
                for (x = 0; x < width; x++) if ((src[x] & 0xffffff) == surface->color_key) dst[x] = 0;

            if (rgn_rect)
            {
                while (rgn_rect < end && rgn_rect->bottom <= y) rgn_rect++;
                apply_line_region( dst, width, rect.left, y, rgn_rect, end );
            }

            src += surface->info.bmiHeader.biWidth;
            dst += buffer.stride;
        }
        surface->window->perform( surface->window, NATIVE_WINDOW_UNLOCK_AND_POST );
    }
    else TRACE( "Unable to lock surface %p window %p buffer %p\n",
                surface, surface->hwnd, surface->window );
}

/***********************************************************************
 *           android_surface_destroy
 */
static void android_surface_destroy( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    TRACE( "freeing %p bits %p\n", surface, surface->bits );

    free( surface->region_data );
    if (surface->region) NtGdiDeleteObjectApp( surface->region );
    release_ioctl_window( surface->window );
    free( surface->bits );
    free( surface );
}

static const struct window_surface_funcs android_surface_funcs =
{
    android_surface_lock,
    android_surface_unlock,
    android_surface_get_bitmap_info,
    android_surface_get_bounds,
    android_surface_set_region,
    android_surface_flush,
    android_surface_destroy
};

static BOOL is_argb_surface( struct window_surface *surface )
{
    return surface && surface->funcs == &android_surface_funcs &&
        get_android_surface( surface )->info.bmiHeader.biCompression == BI_RGB;
}

/***********************************************************************
 *           set_color_key
 */
static void set_color_key( struct android_window_surface *surface, COLORREF key )
{
    if (key == CLR_INVALID)
        surface->color_key = CLR_INVALID;
    else if (surface->info.bmiHeader.biBitCount <= 8)
        surface->color_key = CLR_INVALID;
    else if (key & (1 << 24))  /* PALETTEINDEX */
        surface->color_key = 0;
    else if (key >> 16 == 0x10ff)  /* DIBINDEX */
        surface->color_key = 0;
    else if (surface->info.bmiHeader.biBitCount == 24)
        surface->color_key = key;
    else
        surface->color_key = (GetRValue(key) << 16) | (GetGValue(key) << 8) | GetBValue(key);
}

/***********************************************************************
 *           set_surface_region
 */
static void set_surface_region( struct window_surface *window_surface, HRGN win_region )
{
    struct android_window_surface *surface = get_android_surface( window_surface );
    struct android_win_data *win_data;
    HRGN region = win_region;
    RGNDATA *data = NULL;
    DWORD size;
    int offset_x, offset_y;

    if (window_surface->funcs != &android_surface_funcs) return;  /* we may get the null surface */

    if (!(win_data = get_win_data( surface->hwnd ))) return;
    offset_x = win_data->window_rect.left - win_data->whole_rect.left;
    offset_y = win_data->window_rect.top - win_data->whole_rect.top;
    release_win_data( win_data );

    if (win_region == (HRGN)1)  /* hack: win_region == 1 means retrieve region from server */
    {
        region = NtGdiCreateRectRgn( 0, 0, win_data->window_rect.right - win_data->window_rect.left,
                                     win_data->window_rect.bottom - win_data->window_rect.top );
        if (NtUserGetWindowRgnEx( surface->hwnd, region, 0 ) == ERROR && !surface->region) goto done;
    }

    NtGdiOffsetRgn( region, offset_x, offset_y );
    if (surface->region) NtGdiCombineRgn( region, region, surface->region, RGN_AND );

    if (!(size = NtGdiGetRegionData( region, 0, NULL ))) goto done;
    if (!(data = malloc( size ))) goto done;

    if (!NtGdiGetRegionData( region, size, data ))
    {
        free( data );
        data = NULL;
    }

done:
    window_surface->funcs->lock( window_surface );
    free( surface->region_data );
    surface->region_data = data;
    *window_surface->funcs->get_bounds( window_surface ) = surface->header.rect;
    window_surface->funcs->unlock( window_surface );
    if (region != win_region) NtGdiDeleteObjectApp( region );
}

/***********************************************************************
 *           create_surface
 */
static struct window_surface *create_surface( HWND hwnd, const RECT *rect,
                                              BYTE alpha, COLORREF color_key, BOOL src_alpha )
{
    struct android_window_surface *surface;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    pthread_mutexattr_t attr;

    surface = calloc( 1, FIELD_OFFSET( struct android_window_surface, info.bmiColors[3] ));
    if (!surface) return NULL;
    set_color_info( &surface->info, src_alpha );
    surface->info.bmiHeader.biWidth       = width;
    surface->info.bmiHeader.biHeight      = -height; /* top-down */
    surface->info.bmiHeader.biPlanes      = 1;
    surface->info.bmiHeader.biSizeImage   = get_dib_image_size( &surface->info );

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &surface->mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    surface->header.funcs = &android_surface_funcs;
    surface->header.rect  = *rect;
    surface->header.ref   = 1;
    surface->hwnd         = hwnd;
    surface->window       = get_ioctl_window( hwnd );
    surface->alpha        = alpha;
    set_color_key( surface, color_key );
    set_surface_region( &surface->header, (HRGN)1 );
    reset_bounds( &surface->bounds );

    if (!(surface->bits = malloc( surface->info.bmiHeader.biSizeImage )))
        goto failed;

    TRACE( "created %p hwnd %p %s bits %p-%p\n", surface, hwnd, wine_dbgstr_rect(rect),
           surface->bits, (char *)surface->bits + surface->info.bmiHeader.biSizeImage );

    return &surface->header;

failed:
    android_surface_destroy( &surface->header );
    return NULL;
}

/***********************************************************************
 *           set_surface_layered
 */
static void set_surface_layered( struct window_surface *window_surface, BYTE alpha, COLORREF color_key )
{
    struct android_window_surface *surface = get_android_surface( window_surface );
    COLORREF prev_key;
    BYTE prev_alpha;

    if (window_surface->funcs != &android_surface_funcs) return;  /* we may get the null surface */

    window_surface->funcs->lock( window_surface );
    prev_key = surface->color_key;
    prev_alpha = surface->alpha;
    surface->alpha = alpha;
    set_color_key( surface, color_key );
    if (alpha != prev_alpha || surface->color_key != prev_key)  /* refresh */
        *window_surface->funcs->get_bounds( window_surface ) = surface->header.rect;
    window_surface->funcs->unlock( window_surface );
}

/***********************************************************************
 *           get_mono_icon_argb
 *
 * Return a monochrome icon/cursor bitmap bits in ARGB format.
 */
static unsigned int *get_mono_icon_argb( HDC hdc, HBITMAP bmp, unsigned int *width, unsigned int *height )
{
    BITMAP bm;
    char *mask;
    unsigned int i, j, stride, mask_size, bits_size, *bits = NULL, *ptr;

    if (!NtGdiExtGetObjectW( bmp, sizeof(bm), &bm )) return NULL;
    stride = ((bm.bmWidth + 15) >> 3) & ~1;
    mask_size = stride * bm.bmHeight;
    if (!(mask = malloc( mask_size ))) return NULL;
    if (!NtGdiGetBitmapBits( bmp, mask_size, mask )) goto done;

    bm.bmHeight /= 2;
    bits_size = bm.bmWidth * bm.bmHeight * sizeof(*bits);
    if (!(bits = malloc( bits_size ))) goto done;

    ptr = bits;
    for (i = 0; i < bm.bmHeight; i++)
        for (j = 0; j < bm.bmWidth; j++, ptr++)
        {
            int and = ((mask[i * stride + j / 8] << (j % 8)) & 0x80);
            int xor = ((mask[(i + bm.bmHeight) * stride + j / 8] << (j % 8)) & 0x80);
            if (!xor && and)
                *ptr = 0;
            else if (xor && !and)
                *ptr = 0xffffffff;
            else
                /* we can't draw "invert" pixels, so render them as black instead */
                *ptr = 0xff000000;
        }

    *width = bm.bmWidth;
    *height = bm.bmHeight;

done:
    free( mask );
    return bits;
}

/***********************************************************************
 *           get_bitmap_argb
 *
 * Return the bitmap bits in ARGB format. Helper for setting icons and cursors.
 */
static unsigned int *get_bitmap_argb( HDC hdc, HBITMAP color, HBITMAP mask, unsigned int *width,
                                      unsigned int *height )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    BITMAP bm;
    unsigned int *ptr, *bits = NULL;
    unsigned char *mask_bits = NULL;
    int i, j;
    BOOL has_alpha = FALSE;

    if (!color) return get_mono_icon_argb( hdc, mask, width, height );

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
    if (!(bits = malloc( bm.bmWidth * bm.bmHeight * sizeof(unsigned int) )))
        goto failed;
    if (!NtGdiGetDIBitsInternal( hdc, color, 0, bm.bmHeight, bits, info, DIB_RGB_COLORS, 0, 0 ))
        goto failed;

    *width = bm.bmWidth;
    *height = bm.bmHeight;

    for (i = 0; i < bm.bmWidth * bm.bmHeight; i++)
        if ((has_alpha = (bits[i] & 0xff000000) != 0)) break;

    if (!has_alpha)
    {
        unsigned int width_bytes = (bm.bmWidth + 31) / 32 * 4;
        /* generate alpha channel from the mask */
        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * bm.bmHeight;
        if (!(mask_bits = malloc( info->bmiHeader.biSizeImage ))) goto failed;
        if (!NtGdiGetDIBitsInternal( hdc, mask, 0, bm.bmHeight, mask_bits, info, DIB_RGB_COLORS, 0, 0 ))
            goto failed;
        ptr = bits;
        for (i = 0; i < bm.bmHeight; i++)
            for (j = 0; j < bm.bmWidth; j++, ptr++)
                if (!((mask_bits[i * width_bytes + j / 8] << (j % 8)) & 0x80)) *ptr |= 0xff000000;
        free( mask_bits );
    }

    return bits;

failed:
    free( bits );
    free( mask_bits );
    *width = *height = 0;
    return NULL;
}


enum android_system_cursors
{
    TYPE_ARROW = 1000,
    TYPE_CONTEXT_MENU = 1001,
    TYPE_HAND = 1002,
    TYPE_HELP = 1003,
    TYPE_WAIT = 1004,
    TYPE_CELL = 1006,
    TYPE_CROSSHAIR = 1007,
    TYPE_TEXT = 1008,
    TYPE_VERTICAL_TEXT = 1009,
    TYPE_ALIAS = 1010,
    TYPE_COPY = 1011,
    TYPE_NO_DROP = 1012,
    TYPE_ALL_SCROLL = 1013,
    TYPE_HORIZONTAL_DOUBLE_ARROW = 1014,
    TYPE_VERTICAL_DOUBLE_ARROW = 1015,
    TYPE_TOP_RIGHT_DIAGONAL_DOUBLE_ARROW = 1016,
    TYPE_TOP_LEFT_DIAGONAL_DOUBLE_ARROW = 1017,
    TYPE_ZOOM_IN = 1018,
    TYPE_ZOOM_OUT = 1019,
    TYPE_GRAB = 1020,
    TYPE_GRABBING = 1021,
};

struct system_cursors
{
    WORD id;
    enum android_system_cursors android_id;
};

static const struct system_cursors user32_cursors[] =
{
    { OCR_NORMAL,      TYPE_ARROW },
    { OCR_IBEAM,       TYPE_TEXT },
    { OCR_WAIT,        TYPE_WAIT },
    { OCR_CROSS,       TYPE_CROSSHAIR },
    { OCR_SIZE,        TYPE_ALL_SCROLL },
    { OCR_SIZEALL,     TYPE_ALL_SCROLL },
    { OCR_SIZENWSE,    TYPE_TOP_LEFT_DIAGONAL_DOUBLE_ARROW },
    { OCR_SIZENESW,    TYPE_TOP_RIGHT_DIAGONAL_DOUBLE_ARROW },
    { OCR_SIZEWE,      TYPE_HORIZONTAL_DOUBLE_ARROW },
    { OCR_SIZENS,      TYPE_VERTICAL_DOUBLE_ARROW },
    { OCR_NO,          TYPE_NO_DROP },
    { OCR_HAND,        TYPE_HAND },
    { OCR_HELP,        TYPE_HELP },
    { 0 }
};

static const struct system_cursors comctl32_cursors[] =
{
    /* 102 TYPE_MOVE doesn't exist */
    { 104, TYPE_COPY },
    { 105, TYPE_ARROW },
    { 106, TYPE_HORIZONTAL_DOUBLE_ARROW },
    { 107, TYPE_HORIZONTAL_DOUBLE_ARROW },
    { 108, TYPE_GRABBING },
    { 135, TYPE_VERTICAL_DOUBLE_ARROW },
    { 0 }
};

static const struct system_cursors ole32_cursors[] =
{
    { 1, TYPE_NO_DROP },
    /* 2 TYPE_MOVE doesn't exist */
    { 3, TYPE_COPY },
    { 4, TYPE_ALIAS },
    { 0 }
};

static const struct system_cursors riched20_cursors[] =
{
    { 105, TYPE_GRABBING },
    { 109, TYPE_COPY },
    /* 110 TYPE_MOVE doesn't exist */
    { 111, TYPE_NO_DROP },
    { 0 }
};

static const struct
{
    const struct system_cursors *cursors;
    WCHAR name[16];
} module_cursors[] =
{
    { user32_cursors, {'u','s','e','r','3','2','.','d','l','l',0} },
    { comctl32_cursors, {'c','o','m','c','t','l','3','2','.','d','l','l',0} },
    { ole32_cursors, {'o','l','e','3','2','.','d','l','l',0} },
    { riched20_cursors, {'r','i','c','h','e','d','2','0','.','d','l','l',0} }
};

static int get_cursor_system_id( const ICONINFOEXW *info )
{
    const struct system_cursors *cursors;
    const WCHAR *module;
    unsigned int i;

    if (info->szResName[0]) return 0;  /* only integer resources are supported here */

    if ((module = wcsrchr( info->szModName, '\\' ))) module++;
    else module = info->szModName;
    for (i = 0; i < ARRAY_SIZE( module_cursors ); i++)
        if (!wcsicmp( module, module_cursors[i].name )) break;
    if (i == ARRAY_SIZE( module_cursors )) return 0;

    cursors = module_cursors[i].cursors;
    for (i = 0; cursors[i].id; i++)
        if (cursors[i].id == info->wResID) return cursors[i].android_id;

    return 0;
}


LRESULT ANDROID_DesktopWindowProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_PARENTNOTIFY:
        if (LOWORD(wp) == WM_DESTROY) destroy_ioctl_window( (HWND)lp, FALSE );
        break;
    }
    return NtUserMessageCall( hwnd, msg, wp, lp, 0, NtUserDefWindowProc, FALSE );
}


/***********************************************************************
 *           ANDROID_ProcessEvents
 */
BOOL ANDROID_ProcessEvents( DWORD mask )
{
    if (GetCurrentThreadId() == desktop_tid)
    {
        /* don't process nested events */
        if (current_event) mask = 0;
        return process_events( mask );
    }
    return FALSE;
}

/**********************************************************************
 *           ANDROID_CreateWindow
 */
BOOL ANDROID_CreateWindow( HWND hwnd )
{
    TRACE( "%p\n", hwnd );

    if (hwnd == NtUserGetDesktopWindow())
    {
        struct android_win_data *data;

        init_event_queue();
        start_android_device();
        if (!(data = alloc_win_data( hwnd ))) return FALSE;
        release_win_data( data );
    }
    return TRUE;
}


/***********************************************************************
 *           ANDROID_DestroyWindow
 */
void ANDROID_DestroyWindow( HWND hwnd )
{
    struct android_win_data *data;

    if (!(data = get_win_data( hwnd ))) return;

    if (data->surface) window_surface_release( data->surface );
    data->surface = NULL;
    destroy_gl_drawable( hwnd );
    free_win_data( data );
}


/***********************************************************************
 *           create_win_data
 *
 * Create a data window structure for an existing window.
 */
static struct android_win_data *create_win_data( HWND hwnd, const RECT *window_rect,
                                                 const RECT *client_rect )
{
    struct android_win_data *data;
    HWND parent;

    if (!(parent = NtUserGetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop or HWND_MESSAGE */

    if (!(data = alloc_win_data( hwnd ))) return NULL;

    data->parent = (parent == NtUserGetDesktopWindow()) ? 0 : parent;
    data->whole_rect = data->window_rect = *window_rect;
    data->client_rect = *client_rect;
    return data;
}


static inline BOOL get_surface_rect( const RECT *visible_rect, RECT *surface_rect )
{
    if (!intersect_rect( surface_rect, visible_rect, &virtual_screen_rect )) return FALSE;
    OffsetRect( surface_rect, -visible_rect->left, -visible_rect->top );
    surface_rect->left &= ~31;
    surface_rect->top  &= ~31;
    surface_rect->right  = max( surface_rect->left + 32, (surface_rect->right + 31) & ~31 );
    surface_rect->bottom = max( surface_rect->top + 32, (surface_rect->bottom + 31) & ~31 );
    return TRUE;
}


/***********************************************************************
 *           ANDROID_WindowPosChanging
 */
BOOL ANDROID_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                const RECT *window_rect, const RECT *client_rect, RECT *visible_rect,
                                struct window_surface **surface )
{
    struct android_win_data *data = get_win_data( hwnd );
    RECT surface_rect;
    DWORD flags;
    COLORREF key;
    BYTE alpha;
    BOOL layered = NtUserGetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED;

    TRACE( "win %p window %s client %s style %08x flags %08x\n",
           hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
           (int)NtUserGetWindowLongW( hwnd, GWL_STYLE ), swp_flags );

    if (!data && !(data = create_win_data( hwnd, window_rect, client_rect ))) return TRUE;

    *visible_rect = *window_rect;

    /* create the window surface if necessary */

    if (data->parent) goto done;
    if (swp_flags & SWP_HIDEWINDOW) goto done;
    if (is_argb_surface( data->surface )) goto done;
    if (!get_surface_rect( visible_rect, &surface_rect )) goto done;

    if (data->surface)
    {
        if (!memcmp( &data->surface->rect, &surface_rect, sizeof(surface_rect) ))
        {
            /* existing surface is good enough */
            window_surface_add_ref( data->surface );
            if (*surface) window_surface_release( *surface );
            *surface = data->surface;
            goto done;
        }
    }
    if (!(swp_flags & SWP_SHOWWINDOW) && !(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE))
        goto done;

    if (!layered || !NtUserGetLayeredWindowAttributes( hwnd, &key, &alpha, &flags )) flags = 0;
    if (!(flags & LWA_ALPHA)) alpha = 255;
    if (!(flags & LWA_COLORKEY)) key = CLR_INVALID;

    if (*surface) window_surface_release( *surface );
    *surface = create_surface( data->hwnd, &surface_rect, alpha, key, FALSE );

done:
    release_win_data( data );
    return TRUE;
}


/***********************************************************************
 *           ANDROID_WindowPosChanged
 */
void ANDROID_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                               const RECT *window_rect, const RECT *client_rect,
                               const RECT *visible_rect, const RECT *valid_rects,
                               struct window_surface *surface )
{
    struct android_win_data *data;
    UINT new_style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    HWND owner = 0;

    if (!(data = get_win_data( hwnd ))) return;

    data->window_rect = *window_rect;
    data->whole_rect  = *visible_rect;
    data->client_rect = *client_rect;

    if (!is_argb_surface( data->surface ))
    {
        if (surface) window_surface_add_ref( surface );
        if (data->surface) window_surface_release( data->surface );
        data->surface = surface;
    }
    if (!data->parent) owner = NtUserGetWindowRelative( hwnd, GW_OWNER );
    release_win_data( data );

    if (!(swp_flags & SWP_NOZORDER)) insert_after = NtUserGetWindowRelative( hwnd, GW_HWNDPREV );

    TRACE( "win %p window %s client %s style %08x owner %p after %p flags %08x\n", hwnd,
           wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
           new_style, owner, insert_after, swp_flags );

    ioctl_window_pos_changed( hwnd, window_rect, client_rect, visible_rect,
                              new_style, swp_flags, insert_after, owner );
}


/***********************************************************************
 *           ANDROID_ShowWindow
 */
UINT ANDROID_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    if (!(NtUserGetWindowLongW( hwnd, GWL_STYLE ) & WS_MINIMIZE)) return swp;
    /* always hide icons off-screen */
    if (rect->left != -32000 || rect->top != -32000)
    {
        OffsetRect( rect, -32000 - rect->left, -32000 - rect->top );
        swp &= ~(SWP_NOMOVE | SWP_NOCLIENTMOVE);
    }
    return swp;
}


/*****************************************************************
 *	     ANDROID_SetParent
 */
void ANDROID_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    struct android_win_data *data;

    if (parent == old_parent) return;
    if (!(data = get_win_data( hwnd ))) return;

    TRACE( "win %p parent %p -> %p\n", hwnd, old_parent, parent );

    data->parent = (parent == NtUserGetDesktopWindow()) ? 0 : parent;
    ioctl_set_window_parent( hwnd, parent, (float)get_win_monitor_dpi( hwnd ) / NtUserGetDpiForWindow( hwnd ));
    release_win_data( data );
}


/***********************************************************************
 *           ANDROID_SetCapture
 */
void ANDROID_SetCapture( HWND hwnd, UINT flags )
{
    if (!(flags & (GUI_INMOVESIZE | GUI_INMENUMODE))) return;
    ioctl_set_capture( hwnd );
}


static BOOL get_icon_info( HICON handle, ICONINFOEXW *ret )
{
    UNICODE_STRING module, res_name;
    ICONINFO info;

    module.Buffer = ret->szModName;
    module.MaximumLength = sizeof(ret->szModName) - sizeof(WCHAR);
    res_name.Buffer = ret->szResName;
    res_name.MaximumLength = sizeof(ret->szResName) - sizeof(WCHAR);
    if (!NtUserGetIconInfo( handle, &info, &module, &res_name, NULL, 0 )) return FALSE;
    ret->fIcon    = info.fIcon;
    ret->xHotspot = info.xHotspot;
    ret->yHotspot = info.yHotspot;
    ret->hbmColor = info.hbmColor;
    ret->hbmMask  = info.hbmMask;
    ret->wResID   = res_name.Length ? 0 : LOWORD(res_name.Buffer);
    ret->szModName[module.Length] = 0;
    ret->szResName[res_name.Length] = 0;
    return TRUE;
}


/***********************************************************************
 *           ANDROID_SetCursor
 */
void ANDROID_SetCursor( HWND hwnd, HCURSOR handle )
{
    if (handle)
    {
        unsigned int width = 0, height = 0, *bits = NULL;
        ICONINFOEXW info;
        int id;

        if (!get_icon_info( handle, &info )) return;

        if (!(id = get_cursor_system_id( &info )))
        {
            HDC hdc = NtGdiCreateCompatibleDC( 0 );
            bits = get_bitmap_argb( hdc, info.hbmColor, info.hbmMask, &width, &height );
            NtGdiDeleteObjectApp( hdc );

            /* make sure hotspot is valid */
            if (info.xHotspot >= width || info.yHotspot >= height)
            {
                info.xHotspot = width / 2;
                info.yHotspot = height / 2;
            }
        }
        ioctl_set_cursor( id, width, height, info.xHotspot, info.yHotspot, bits );
        free( bits );
        NtGdiDeleteObjectApp( info.hbmColor );
        NtGdiDeleteObjectApp( info.hbmMask );
    }
    else ioctl_set_cursor( 0, 0, 0, 0, 0, NULL );
}


/***********************************************************************
 *           ANDROID_SetWindowStyle
 */
void ANDROID_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
    struct android_win_data *data;
    DWORD changed = style->styleNew ^ style->styleOld;

    if (hwnd == NtUserGetDesktopWindow()) return;
    if (!(data = get_win_data( hwnd ))) return;

    if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED)) /* changing WS_EX_LAYERED resets attributes */
    {
        if (is_argb_surface( data->surface ))
        {
            if (data->surface) window_surface_release( data->surface );
            data->surface = NULL;
        }
        else if (data->surface) set_surface_layered( data->surface, 255, CLR_INVALID );
    }
    release_win_data( data );
}


/***********************************************************************
 *           ANDROID_SetWindowRgn
 */
void ANDROID_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    struct android_win_data *data;

    if ((data = get_win_data( hwnd )))
    {
        if (data->surface) set_surface_region( data->surface, hrgn );
        release_win_data( data );
    }
    else FIXME( "not supported on other process window %p\n", hwnd );
}


/***********************************************************************
 *	     ANDROID_SetLayeredWindowAttributes
 */
void ANDROID_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    struct android_win_data *data;

    if (!(flags & LWA_ALPHA)) alpha = 255;
    if (!(flags & LWA_COLORKEY)) key = CLR_INVALID;

    if ((data = get_win_data( hwnd )))
    {
        if (data->surface) set_surface_layered( data->surface, alpha, key );
        release_win_data( data );
    }
}


/*****************************************************************************
 *           ANDROID_UpdateLayeredWindow
 */
BOOL ANDROID_UpdateLayeredWindow( HWND hwnd, const UPDATELAYEREDWINDOWINFO *info,
                                  const RECT *window_rect )
{
    struct window_surface *surface;
    struct android_win_data *data;
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, 0 };
    COLORREF color_key = (info->dwFlags & ULW_COLORKEY) ? info->crKey : CLR_INVALID;
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *bmi = (BITMAPINFO *)buffer;
    void *src_bits, *dst_bits;
    RECT rect, src_rect;
    HDC hdc = 0;
    HBITMAP dib;
    BOOL ret = FALSE;

    if (!(data = get_win_data( hwnd ))) return FALSE;

    rect = *window_rect;
    OffsetRect( &rect, -window_rect->left, -window_rect->top );

    surface = data->surface;
    if (!is_argb_surface( surface ))
    {
        if (surface) window_surface_release( surface );
        surface = NULL;
    }

    if (!surface || !EqualRect( &surface->rect, &rect ))
    {
        data->surface = create_surface( data->hwnd, &rect, 255, color_key, TRUE );
        if (surface) window_surface_release( surface );
        surface = data->surface;
    }
    else set_surface_layered( surface, 255, color_key );

    if (surface) window_surface_add_ref( surface );
    release_win_data( data );

    if (!surface) return FALSE;
    if (!info->hdcSrc)
    {
        window_surface_release( surface );
        return TRUE;
    }

    dst_bits = surface->funcs->get_info( surface, bmi );

    if (!(dib = NtGdiCreateDIBSection( info->hdcDst, NULL, 0, bmi, DIB_RGB_COLORS, 0, 0, 0, &src_bits )))
        goto done;
    if (!(hdc = NtGdiCreateCompatibleDC( 0 ))) goto done;

    NtGdiSelectBitmap( hdc, dib );

    surface->funcs->lock( surface );

    if (info->prcDirty)
    {
        intersect_rect( &rect, &rect, info->prcDirty );
        memcpy( src_bits, dst_bits, bmi->bmiHeader.biSizeImage );
        NtGdiPatBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, BLACKNESS );
    }
    src_rect = rect;
    if (info->pptSrc) OffsetRect( &src_rect, info->pptSrc->x, info->pptSrc->y );
    NtGdiTransformPoints( info->hdcSrc, (POINT *)&src_rect, (POINT *)&src_rect, 2, NtGdiDPtoLP );

    if (info->dwFlags & ULW_ALPHA) blend = *info->pblend;
    ret = NtGdiAlphaBlend( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                           info->hdcSrc, src_rect.left, src_rect.top,
                           src_rect.right - src_rect.left, src_rect.bottom - src_rect.top,
                           *(DWORD *)&blend, 0 );
    if (ret)
    {
        memcpy( dst_bits, src_bits, bmi->bmiHeader.biSizeImage );
        add_bounds_rect( surface->funcs->get_bounds( surface ), &rect );
    }

    surface->funcs->unlock( surface );
    surface->funcs->flush( surface );

done:
    window_surface_release( surface );
    if (hdc) NtGdiDeleteObjectApp( hdc );
    if (dib) NtGdiDeleteObjectApp( dib );
    return ret;
}


/**********************************************************************
 *           ANDROID_WindowMessage
 */
LRESULT ANDROID_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    struct android_win_data *data;

    switch (msg)
    {
    case WM_ANDROID_REFRESH:
        if (wp)  /* opengl client window */
        {
            update_gl_drawable( hwnd );
        }
        else if ((data = get_win_data( hwnd )))
        {
            struct window_surface *surface = data->surface;
            if (surface)
            {
                surface->funcs->lock( surface );
                *surface->funcs->get_bounds( surface ) = surface->rect;
                surface->funcs->unlock( surface );
                if (is_argb_surface( surface )) surface->funcs->flush( surface );
            }
            release_win_data( data );
        }
        return 0;
    default:
        FIXME( "got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, (long)wp, lp );
        return 0;
    }
}


/***********************************************************************
 *           ANDROID_CreateDesktop
 */
BOOL ANDROID_CreateDesktop( const WCHAR *name, UINT width, UINT height )
{
    /* wait until we receive the surface changed event */
    while (!screen_width)
    {
        if (wait_events( 2000 ) != 1)
        {
            ERR( "wait timed out\n" );
            break;
        }
        process_events( QS_ALLINPUT );
    }
    return 0;
}
