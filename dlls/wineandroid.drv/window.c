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
    struct window_rects rects;     /* window rects in monitor DPI, relative to parent client area */
    ANativeWindow *window;         /* native window wrapper that forwards calls to the desktop process */
    ANativeWindow *client;         /* native client surface wrapper that forwards calls to the desktop process */
    BOOL           has_surface;    /* whether the client surface has been created on the Java side */
};

#define SWP_AGG_NOPOSCHANGE (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

pthread_mutex_t win_data_mutex;

static struct android_win_data *win_data_context[32768];

static inline int context_idx( HWND hwnd )
{
    return LOWORD( hwnd ) >> 1;
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
                                            (float)NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) / NtUserGetDpiForWindow( hwnd ));
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
    if (data->client) release_ioctl_window( data->client );
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
            screen_width = event->data.desktop.width;
            screen_height = event->data.desktop.height;
            init_monitors( screen_width, screen_height );
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
                           event->data.motion.input.mi.dx, event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, event->data.motion.input.mi.dwFlags );
                else if (event->data.motion.input.mi.dwFlags & (MOUSEEVENTF_LEFTUP|MOUSEEVENTF_RIGHTUP|MOUSEEVENTF_MIDDLEUP))
                    TRACE( "BUTTONUP pos %d,%d hwnd %p flags %x\n",
                           event->data.motion.input.mi.dx, event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, event->data.motion.input.mi.dwFlags );
                else
                    TRACE( "MOUSEMOVE pos %d,%d hwnd %p flags %x\n",
                           event->data.motion.input.mi.dx, event->data.motion.input.mi.dy,
                           event->data.motion.hwnd, event->data.motion.input.mi.dwFlags );
                if (!capture && (event->data.motion.input.mi.dwFlags & MOUSEEVENTF_ABSOLUTE))
                {
                    RECT rect;
                    SetRect( &rect, event->data.motion.input.mi.dx, event->data.motion.input.mi.dy,
                             event->data.motion.input.mi.dx + 1, event->data.motion.input.mi.dy + 1 );

                    SERVER_START_REQ( update_window_zorder )
                    {
                        req->window      = wine_server_user_handle( event->data.motion.hwnd );
                        req->rect        = wine_server_rectangle( rect );
                        wine_server_call( req );
                    }
                    SERVER_END_REQ;
                }
                NtUserSendHardwareInput( capture ? capture : event->data.motion.hwnd, 0, &event->data.motion.input, 0 );
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
            NtUserSendHardwareInput( 0, 0, &event->data.kbd.input, 0 );
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
    ANativeWindow        *window;
    UINT                  clip_count;
    RECT                 *clip_rects;
};

static struct android_window_surface *get_android_surface( struct window_surface *surface )
{
    return (struct android_window_surface *)surface;
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
 *           android_surface_set_clip
 */
static void android_surface_set_clip( struct window_surface *window_surface, const RECT *rects, UINT count )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    free( surface->clip_rects );
    surface->clip_rects = NULL;

    if (!count || !(surface->clip_rects = malloc( count * sizeof(*rects) ))) return;
    memcpy( surface->clip_rects, rects, count * sizeof(*rects) );
    surface->clip_count = count;
}

/***********************************************************************
 *           android_surface_flush
 */
static BOOL android_surface_flush( struct window_surface *window_surface, const RECT *rect, const RECT *dirty,
                                   const BITMAPINFO *color_info, const void *color_bits, BOOL shape_changed,
                                   const BITMAPINFO *shape_info, const void *shape_bits )
{
    struct android_window_surface *surface = get_android_surface( window_surface );
    ANativeWindow_Buffer buffer;
    ARect rc;

    rc.left   = dirty->left;
    rc.top    = dirty->top;
    rc.right  = dirty->right;
    rc.bottom = dirty->bottom;

    if (!surface->window->perform( surface->window, NATIVE_WINDOW_LOCK, &buffer, &rc ))
    {
        const RECT *rgn_rect = surface->clip_rects, *end = surface->clip_rects + surface->clip_count;
        UINT alpha_mask = window_surface->alpha_mask, alpha_bits = window_surface->alpha_bits;
        COLORREF color_key = window_surface->color_key;
        BYTE alpha = alpha_bits >> 24;
        DWORD *src, *dst;
        int x, y, width;
        RECT locked;

        locked.left   = rc.left;
        locked.top    = rc.top;
        locked.right  = rc.right;
        locked.bottom = rc.bottom;
        intersect_rect( &locked, &locked, rect );

        src = (DWORD *)color_bits + (locked.top - rect->top) * color_info->bmiHeader.biWidth +
              (locked.left - rect->left);
        dst = (DWORD *)buffer.bits + locked.top * buffer.stride + locked.left;
        width = min( locked.right - locked.left, buffer.stride );

        for (y = locked.top; y < min( buffer.height, locked.bottom ); y++)
        {
            if (alpha_mask)
                memcpy( dst, src, width * sizeof(*dst) );
            else if (alpha == 255)
                for (x = 0; x < width; x++) dst[x] = src[x] | 0xff000000;
            else
                for (x = 0; x < width; x++)
                    dst[x] = ((alpha << 24) |
                              (((BYTE)(src[x] >> 16) * alpha / 255) << 16) |
                              (((BYTE)(src[x] >> 8) * alpha / 255) <<  8) |
                              (((BYTE)src[x] * alpha / 255)));

            if (color_key != CLR_INVALID)
                for (x = 0; x < width; x++) if ((src[x] & 0xffffff) == color_key) dst[x] = 0;

            if (rgn_rect)
            {
                while (rgn_rect < end && rgn_rect->bottom <= y) rgn_rect++;
                apply_line_region( dst, width, locked.left, y, rgn_rect, end );
            }

            src += color_info->bmiHeader.biWidth;
            dst += buffer.stride;
        }
        surface->window->perform( surface->window, NATIVE_WINDOW_UNLOCK_AND_POST );
    }
    else TRACE( "Unable to lock surface %p window %p buffer %p\n",
                surface, window_surface->hwnd, surface->window );

    return TRUE;
}

/***********************************************************************
 *           android_surface_destroy
 */
static void android_surface_destroy( struct window_surface *window_surface )
{
    struct android_window_surface *surface = get_android_surface( window_surface );

    TRACE( "freeing %p\n", surface );

    free( surface->clip_rects );
    release_ioctl_window( surface->window );
}

static const struct window_surface_funcs android_surface_funcs =
{
    android_surface_set_clip,
    android_surface_flush,
    android_surface_destroy
};


/***********************************************************************
 *           create_surface
 */
static struct window_surface *create_surface( HWND hwnd, const RECT *rect )
{
    struct android_window_surface *surface;
    int width = rect->right - rect->left, height = rect->bottom - rect->top;
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct window_surface *window_surface;

    memset( info, 0, sizeof(*info) );
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = width;
    info->bmiHeader.biHeight      = -height; /* top-down */
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biSizeImage   = get_dib_image_size(info);
    info->bmiHeader.biCompression = BI_RGB;

    if ((window_surface = window_surface_create( sizeof(*surface), &android_surface_funcs, hwnd, rect, info, 0 )))
    {
        surface = get_android_surface( window_surface );
        surface->window = get_ioctl_window( hwnd );
    }

    return window_surface;
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
    free_win_data( data );
}


/***********************************************************************
 *           create_win_data
 *
 * Create a data window structure for an existing window.
 */
static struct android_win_data *create_win_data( HWND hwnd, const struct window_rects *rects )
{
    struct android_win_data *data;
    HWND parent;

    if (!(parent = NtUserGetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop or HWND_MESSAGE */

    if (!(data = alloc_win_data( hwnd ))) return NULL;

    data->parent = (parent == NtUserGetDesktopWindow()) ? 0 : parent;
    data->rects = *rects;
    return data;
}


/***********************************************************************
 *           ANDROID_WindowPosChanging
 */
BOOL ANDROID_WindowPosChanging( HWND hwnd, UINT swp_flags, BOOL shaped, const struct window_rects *rects )
{
    struct android_win_data *data = get_win_data( hwnd );

    TRACE( "hwnd %p, swp_flags %#x, shaped %u, rects %s\n", hwnd, swp_flags, shaped, debugstr_window_rects( rects ) );

    if (!data && !(data = create_win_data( hwnd, rects ))) return FALSE; /* use default surface */
    release_win_data(data);

    return TRUE;
}


/***********************************************************************
 *           ANDROID_CreateWindowSurface
 */
BOOL ANDROID_CreateWindowSurface( HWND hwnd, BOOL layered, const RECT *surface_rect, struct window_surface **surface )
{
    struct window_surface *previous;
    struct android_win_data *data;

    TRACE( "hwnd %p, layered %u, surface_rect %s, surface %p\n", hwnd, layered, wine_dbgstr_rect( surface_rect ), surface );

    if ((previous = *surface) && previous->funcs == &android_surface_funcs) return TRUE;
    if (!(data = get_win_data( hwnd ))) return TRUE; /* use default surface */
    if (previous) window_surface_release( previous );

    *surface = create_surface( data->hwnd, surface_rect );

    release_win_data( data );
    return TRUE;
}


/***********************************************************************
 *           ANDROID_WindowPosChanged
 */
void ANDROID_WindowPosChanged( HWND hwnd, HWND insert_after, HWND owner_hint, UINT swp_flags, BOOL fullscreen,
                               const struct window_rects *new_rects, struct window_surface *surface )
{
    struct android_win_data *data;
    UINT new_style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    HWND owner = 0;

    if (!(data = get_win_data( hwnd ))) return;
    data->rects = *new_rects;

    if (!data->parent) owner = NtUserGetWindowRelative( hwnd, GW_OWNER );
    release_win_data( data );

    if (!(swp_flags & SWP_NOZORDER)) insert_after = NtUserGetWindowRelative( hwnd, GW_HWNDPREV );

    TRACE( "win %p new_rects %s style %08x owner %p after %p flags %08x\n", hwnd,
           debugstr_window_rects(new_rects), new_style, owner, insert_after, swp_flags );

    ioctl_window_pos_changed( hwnd, new_rects, new_style, swp_flags, insert_after, owner );
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
    ioctl_set_window_parent( hwnd, parent, (float)NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) / NtUserGetDpiForWindow( hwnd ));
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


/**********************************************************************
 *           ANDROID_WindowMessage
 */
LRESULT ANDROID_WindowMessage( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    switch (msg)
    {
    case WM_ANDROID_REFRESH:
        if (wp)  /* opengl client window */
        {
            struct android_win_data *data;

            if ((data = get_win_data( hwnd )))
            {
                data->has_surface = TRUE;
                release_win_data( data );
            }

            detach_client_surfaces( hwnd );
        }
        else
        {
            NtUserExposeWindowSurface( hwnd, 0, NULL, 0 );
        }
        return 0;
    default:
        FIXME( "got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, (long)wp, lp );
        return 0;
    }
}

ANativeWindow *get_client_window( HWND hwnd )
{
    struct android_win_data *data;
    ANativeWindow *client;

    if (!(data = get_win_data( hwnd ))) return NULL;
    if (!data->client) data->client = create_ioctl_window( hwnd, TRUE, 1.0f );
    client = grab_ioctl_window( data->client );
    release_win_data( data );

    return client;
}

BOOL has_client_surface( HWND hwnd )
{
    struct android_win_data *data;
    BOOL ret;

    if (!(data = get_win_data( hwnd ))) return FALSE;
    ret = data->has_surface;
    release_win_data( data );

    return ret;
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
