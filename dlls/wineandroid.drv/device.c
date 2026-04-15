/*
 * Android pseudo-device handling
 *
 * Copyright 2014-2017 Alexandre Julliard
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

#define __ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "android.h"
#include "wine/server.h"
#include "wine/debug.h"

#include <dlfcn.h>

#define ANDROID_LOG_ERR ANDROID_LOG_ERROR
#define ANDROID_LOG_TRACE ANDROID_LOG_VERBOSE
#define ANDROID_LOG_FIXME 16

static int log_flags;

#define LOG(level, fmt, ...) \
    do { \
        if (!(log_flags & (1 << __WINE_DBCL_INIT)) || (log_flags & (1 << __WINE_DBCL_ ## level))) \
        { \
            int prio = (ANDROID_LOG_ ## level == ANDROID_LOG_FIXME) ? \
                       ANDROID_LOG_WARN : ANDROID_LOG_ ## level; \
            p__android_log_print(prio, "wineandroid.drv", "%s: %s" fmt, \
                                 __func__, \
                                 (ANDROID_LOG_ ## level == ANDROID_LOG_FIXME) ? "FIXME: " : "", \
                                 ##__VA_ARGS__); \
        } \
    } while (0)

#define DBGSTR_RECT_FMT "(%d,%d)-(%d,%d)"
#define DBGSTR_RECT(rect) (int)(rect)->left, (int)(rect)->top, (int)(rect)->right, (int)(rect)->bottom


WINE_DEFAULT_DEBUG_CHANNEL(android);

#ifndef SYNC_IOC_WAIT
#define SYNC_IOC_WAIT _IOW('>', 0, __s32)
#endif

static HWND capture_window;
static HWND desktop_window;

static pthread_mutex_t dispatch_ioctl_lock = PTHREAD_MUTEX_INITIALIZER;

#define ANDROIDCONTROLTYPE  ((ULONG)'A')
#define ANDROID_IOCTL(n) CTL_CODE(ANDROIDCONTROLTYPE, n, METHOD_BUFFERED, FILE_READ_ACCESS)

enum android_ioctl
{
    IOCTL_CREATE_WINDOW,
    IOCTL_DESTROY_WINDOW,
    IOCTL_WINDOW_POS_CHANGED,
    IOCTL_SET_WINDOW_PARENT,
    IOCTL_DEQUEUE_BUFFER,
    IOCTL_QUEUE_BUFFER,
    IOCTL_CANCEL_BUFFER,
    IOCTL_QUERY,
    IOCTL_PERFORM,
    IOCTL_SET_SWAP_INT,
    IOCTL_SET_CAPTURE,
    IOCTL_SET_CURSOR,
    NB_IOCTLS
};

#define NB_CACHED_BUFFERS 4

/* data about the native window in the context of the Java process */
struct native_win_data
{
    struct ANativeWindow       *parent;
    struct AHardwareBuffer *buffers[NB_CACHED_BUFFERS];
    HWND                        hwnd;
    BOOL                        opengl;
    int                         generation;
    int                         api;
    int                         buffer_format;
    int                         swap_interval;
    int                         buffer_lru[NB_CACHED_BUFFERS];
};

/* wrapper for a native window in the context of the client (non-Java) process */
struct native_win_wrapper
{
    struct ANativeWindow          win;
    struct
    {
        struct AHardwareBuffer   *self;
        int                       buffer_id;
        int                       generation;
    } buffers[NB_CACHED_BUFFERS];
    struct AHardwareBuffer       *locked_buffer;
    HWND                          hwnd;
    BOOL                          opengl;
    LONG                          ref;
};

#define IPC_SOCKET_NAME "\0\\Device\\WineAndroid"
#define IPC_SOCKET_ADDR_LEN ((socklen_t)(offsetof(struct sockaddr_un, sun_path) + sizeof(IPC_SOCKET_NAME) - 1))

static const struct sockaddr_un ipc_addr = {
    .sun_family = AF_UNIX,
    .sun_path = IPC_SOCKET_NAME,
};

struct ioctl_header
{
    int  hwnd;
    BOOL opengl;
};

struct ioctl_android_create_window
{
    struct ioctl_header hdr;
    int                 parent;
    BOOL                is_desktop;
};

struct ioctl_android_destroy_window
{
    struct ioctl_header hdr;
};

struct ioctl_android_window_pos_changed
{
    struct ioctl_header hdr;
    RECT                window_rect;
    RECT                client_rect;
    RECT                visible_rect;
    int                 style;
    int                 flags;
    int                 after;
    int                 owner;
};

struct ioctl_android_dequeueBuffer
{
    struct ioctl_header hdr;
    int buffer_id;
    int generation;
};

struct ioctl_android_queueBuffer
{
    struct ioctl_header hdr;
    int                 buffer_id;
    int                 generation;
};

struct ioctl_android_cancelBuffer
{
    struct ioctl_header hdr;
    int                 buffer_id;
    int                 generation;
};

struct ioctl_android_query
{
    struct ioctl_header hdr;
    int                 what;
    int                 value;
};

struct ioctl_android_perform
{
    struct ioctl_header hdr;
    int                 operation;
    int                 args[4];
};

struct ioctl_android_set_swap_interval
{
    struct ioctl_header hdr;
    int                 interval;
};

struct ioctl_android_set_window_parent
{
    struct ioctl_header hdr;
    int                 parent;
};

struct ioctl_android_set_capture
{
    struct ioctl_header hdr;
};

struct ioctl_android_set_cursor
{
    struct ioctl_header hdr;
    int                 id;
    int                 width;
    int                 height;
    int                 hotspotx;
    int                 hotspoty;
    int                 bits[1];
};

static struct native_win_data *data_map[65536];

static unsigned int data_map_idx( HWND hwnd, BOOL opengl )
{
    /* window handles are always even, so use low-order bit for opengl flag */
    return LOWORD(hwnd) + !!opengl;
}

static struct native_win_data *get_native_win_data( HWND hwnd, BOOL opengl )
{
    struct native_win_data *data = data_map[data_map_idx( hwnd, opengl )];

    if (data && data->hwnd == hwnd && !data->opengl == !opengl) return data;
    LOG( WARN, "unknown win %p opengl %u\n", hwnd, opengl );
    return NULL;
}

static struct native_win_data *get_ioctl_native_win_data( const struct ioctl_header *hdr )
{
    return get_native_win_data( LongToHandle(hdr->hwnd), hdr->opengl );
}

static int get_ioctl_win_parent( HWND parent )
{
    if (parent != NtUserGetDesktopWindow() && !NtUserGetAncestor( parent, GA_PARENT ))
        return HandleToLong( HWND_MESSAGE );
    return HandleToLong( parent );
}

static void wait_fence_and_close( int fence )
{
    __s32 timeout = 1000;  /* FIXME: should be -1 for infinite timeout */

    if (fence == -1) return;
    ioctl( fence, SYNC_IOC_WAIT, &timeout );
    close( fence );
}

static inline struct ANativeWindowBuffer *anwb_from_ahb(AHardwareBuffer *ahb)
{
    static ptrdiff_t off = (ptrdiff_t)-1;

    if (!ahb) return NULL;

    if (off == (ptrdiff_t)-1)
    {
        struct ANativeWindowBuffer *fake =
        (struct ANativeWindowBuffer *)(uintptr_t)0x10000u;

        AHardwareBuffer *h = pANativeWindowBuffer_getHardwareBuffer(fake);
        off = (const char *)h - (const char *)fake;
    }

    return (struct ANativeWindowBuffer *)((char *)ahb - off);
}

static AHardwareBuffer *ahb_from_anwb( struct native_win_wrapper *win, struct ANativeWindowBuffer *buffer, int *buffer_id, int *generation )
{
    AHardwareBuffer *ahb;
    unsigned int i;

    if (!buffer) return NULL;

    ahb = pANativeWindowBuffer_getHardwareBuffer(buffer);

    if (win)
    {
        for (i = 0; i < NB_CACHED_BUFFERS; ++i)
        {
            if (win->buffers[i].self != ahb) continue;

            if (buffer_id) *buffer_id = win->buffers[i].buffer_id;
            if (generation) *generation = win->buffers[i].generation;
            break;
        }
    }

    return ahb;
}

/* insert a buffer index at the head of the LRU list */
static void insert_buffer_lru( struct native_win_data *win, int index )
{
    unsigned int i;

    for (i = 0; i < NB_CACHED_BUFFERS; i++)
    {
        if (win->buffer_lru[i] == index) break;
        if (win->buffer_lru[i] == -1) break;
    }

    assert( i < NB_CACHED_BUFFERS );
    memmove( win->buffer_lru + 1, win->buffer_lru, i * sizeof(win->buffer_lru[0]) );
    win->buffer_lru[0] = index;
}

static int register_buffer( struct native_win_data *win, struct AHardwareBuffer *buffer, int *is_new )
{
    unsigned int i;

    *is_new = 0;
    for (i = 0; i < NB_CACHED_BUFFERS; i++)
    {
        if (win->buffers[i] == buffer) goto done;
        if (!win->buffers[i]) break;
    }

    if (i == NB_CACHED_BUFFERS)
    {
        /* reuse the least recently used buffer */
        i = win->buffer_lru[NB_CACHED_BUFFERS - 1];
        assert( i < NB_CACHED_BUFFERS );

        LOG( TRACE, "%p %p evicting buffer %p id %d from cache\n",
               win->hwnd, win->parent, win->buffers[i], i );
        pAHardwareBuffer_release(win->buffers[i]);
    }

    win->buffers[i] = buffer;

    pAHardwareBuffer_acquire(buffer);
    *is_new = 1;
    LOG( TRACE, "%p %p %p -> %d\n", win->hwnd, win->parent, buffer, i );

done:
    insert_buffer_lru( win, i );
    return i;
}

static struct ANativeWindowBuffer *get_registered_buffer( struct native_win_data *win, int id )
{
    if (id < 0 || id >= NB_CACHED_BUFFERS || !win->buffers[id])
    {
        LOG( ERR, "unknown buffer %d for %p %p\n", id, win->hwnd, win->parent );
        return NULL;
    }
    return anwb_from_ahb(win->buffers[id]);
}

static void release_native_window( struct native_win_data *data )
{
    unsigned int i;

    if (data->parent) pANativeWindow_release( data->parent );
    for (i = 0; i < NB_CACHED_BUFFERS; i++)
    {
        if (data->buffers[i]) pAHardwareBuffer_release(data->buffers[i]);
        data->buffer_lru[i] = -1;
    }
    memset( data->buffers, 0, sizeof(data->buffers) );
}

static void free_native_win_data( struct native_win_data *data )
{
    unsigned int idx = data_map_idx( data->hwnd, data->opengl );

    InterlockedCompareExchangePointer( (void **)&capture_window, 0, data->hwnd );
    release_native_window( data );
    free( data );
    data_map[idx] = NULL;
}

static struct native_win_data *create_native_win_data( HWND hwnd, BOOL opengl )
{
    unsigned int i, idx = data_map_idx( hwnd, opengl );
    struct native_win_data *data = data_map[idx];

    if (data)
    {
        LOG( WARN, "data for %p not freed correctly\n", data->hwnd );
        free_native_win_data( data );
    }
    if (!(data = calloc( 1, sizeof(*data) ))) return NULL;
    data->hwnd = hwnd;
    data->opengl = opengl;
    if (!opengl) data->api = NATIVE_WINDOW_API_CPU;
    data->buffer_format = PF_BGRA_8888;
    data_map[idx] = data;
    for (i = 0; i < NB_CACHED_BUFFERS; i++) data->buffer_lru[i] = -1;
    return data;
}

/* register a native window received from the UI thread for use in ioctls */
void register_native_window( HWND hwnd, struct ANativeWindow *win, BOOL opengl )
{
    struct native_win_data *data = NULL;

    pthread_mutex_lock(&dispatch_ioctl_lock);
    data = get_native_win_data( hwnd, opengl );

    if (!win) goto end;  /* do nothing and hold on to the window until we get a new surface */

    if (!data || data->parent == win)
    {
        pANativeWindow_release( win );
        LOG( TRACE, "%p -> %p win %p (unchanged)\n", hwnd, data, win );
        goto end;
    }

    release_native_window( data );
    data->parent = win;
    data->generation++;
    if (data->api) win->perform( win, NATIVE_WINDOW_API_CONNECT, data->api );
    win->perform( win, NATIVE_WINDOW_SET_BUFFERS_FORMAT, data->buffer_format );
    win->setSwapInterval( win, data->swap_interval );
    LOG( TRACE, "%p -> %p win %p\n", hwnd, data, win );
end:
    pthread_mutex_unlock(&dispatch_ioctl_lock);
}

/* get the capture window stored in the desktop process */
HWND get_capture_window(void)
{
    return capture_window;
}

static jobject load_java_method( JNIEnv* env, jmethodID *method, const char *name, const char *args )
{
    if (!*method)
    {
        jclass class;

        class = (*env)->GetObjectClass( env, java_object );
        *method = (*env)->GetMethodID( env, class, name, args );
        if (!*method)
        {
            LOG( FIXME, "method %s not found\n", name );
            return NULL;
        }
    }
    return java_object;
}

static void create_desktop_view( JNIEnv* env )
{
    static jmethodID method;
    jobject object;

    if (!(object = load_java_method( env, &method, "createDesktopView", "()V" ))) return;

    (*env)->CallVoidMethod( env, object, method );
}

static int createWindow_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_create_window *res = data;
    struct native_win_data *win_data;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = create_native_win_data( LongToHandle(res->hdr.hwnd), res->hdr.opengl )))
        return -ENOMEM;

    LOG( TRACE, "hwnd %08x opengl %u parent %08x\n", res->hdr.hwnd, res->hdr.opengl, res->parent );

    if (!(object = load_java_method( env, &method, "createWindow", "(IZZI)V" ))) return -ENOSYS;

    (*env)->CallVoidMethod( env, object, method, res->hdr.hwnd, res->is_desktop, res->hdr.opengl, res->parent );
    return 0;
}

static int destroyWindow_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_destroy_window *res = data;
    struct native_win_data *win_data;

    if (in_size < sizeof(*res)) return -EINVAL;

    win_data = get_ioctl_native_win_data( &res->hdr );

    LOG( TRACE, "hwnd %08x opengl %u\n", res->hdr.hwnd, res->hdr.opengl );

    if (!(object = load_java_method( env, &method, "destroyWindow", "(I)V" ))) return -ENOSYS;

    (*env)->CallVoidMethod( env, object, method, res->hdr.hwnd );
    if (win_data) free_native_win_data( win_data );
    return 0;
}

static int windowPosChanged_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_window_pos_changed *res = data;

    if (in_size < sizeof(*res)) return -EINVAL;

    LOG( TRACE, "hwnd %08x win " DBGSTR_RECT_FMT " client " DBGSTR_RECT_FMT " visible " DBGSTR_RECT_FMT " style %08x flags %08x after %08x owner %08x\n",
                res->hdr.hwnd, DBGSTR_RECT(&res->window_rect), DBGSTR_RECT(&res->client_rect),
                DBGSTR_RECT(&res->visible_rect), res->style, res->flags, res->after, res->owner );

    if (!(object = load_java_method( env, &method, "windowPosChanged", "(IIIIIIIIIIIIIIIII)V" )))
        return -ENOSYS;

    (*env)->CallVoidMethod( env, object, method, res->hdr.hwnd, res->flags, res->after, res->owner, res->style,
                            res->window_rect.left, res->window_rect.top, res->window_rect.right, res->window_rect.bottom,
                            res->client_rect.left, res->client_rect.top, res->client_rect.right, res->client_rect.bottom,
                            res->visible_rect.left, res->visible_rect.top, res->visible_rect.right, res->visible_rect.bottom );
    return 0;
}

static int dequeueBuffer_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ANativeWindow *parent;
    struct ioctl_android_dequeueBuffer *res = data;
    struct native_win_data *win_data;
    struct ANativeWindowBuffer *buffer = NULL;
    AHardwareBuffer *ahb = NULL;
    int fence, ret, is_new;

    if (out_size < sizeof( *res ) || in_size < sizeof(res->hdr)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    if (!(parent = win_data->parent)) return -EWOULDBLOCK;

    res->buffer_id = -1;
    res->generation = 0;
    *ret_size = sizeof(*res);

    ret = parent->dequeueBuffer( parent, &buffer, &fence );
    if (ret)
    {
        LOG( ERR, "%08x failed %d\n", res->hdr.hwnd, ret );
        return ret;
    }

    if (!buffer)
    {
        LOG( TRACE, "got invalid buffer\n" );
        return STATUS_UNSUCCESSFUL;
    }

    LOG( TRACE, "%08x got buffer %p fence %d\n", res->hdr.hwnd, buffer, fence );
    ahb = pANativeWindowBuffer_getHardwareBuffer( buffer );

    res->buffer_id = register_buffer( win_data, ahb, &is_new );
    res->generation = win_data->generation;

    if (is_new)
    {
        int sv[2] = { -1, -1 };

        if (!ahb)
        {
            wait_fence_and_close( fence );
            return STATUS_UNSUCCESSFUL;
        }

        if (socketpair( AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sv ) < 0)
        {
            wait_fence_and_close( fence );
            return STATUS_UNSUCCESSFUL;
        }

        ret = pAHardwareBuffer_sendHandleToUnixSocket( ahb, sv[0] );
        close( sv[0] );
        if (ret)
        {
            close( sv[1] );
            wait_fence_and_close( fence );
            return ret;
        }

        *reply_fd = sv[1];
    }

    wait_fence_and_close( fence );
    return 0;
}

static int cancelBuffer_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_cancelBuffer *res = data;
    struct ANativeWindow *parent;
    struct ANativeWindowBuffer *buffer;
    struct native_win_data *win_data;
    int ret;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    if (!(parent = win_data->parent)) return -EWOULDBLOCK;
    if (res->generation != win_data->generation) return 0;  /* obsolete buffer, ignore */

    if (!(buffer = get_registered_buffer( win_data, res->buffer_id ))) return -ENOENT;

    LOG( TRACE, "%08x buffer %p\n", res->hdr.hwnd, buffer );
    ret = parent->cancelBuffer( parent, buffer, -1 );
    return ret;
}

static int queueBuffer_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_queueBuffer *res = data;
    struct ANativeWindow *parent;
    struct ANativeWindowBuffer *buffer;
    struct native_win_data *win_data;
    int ret;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    if (!(parent = win_data->parent)) return -EWOULDBLOCK;
    if (res->generation != win_data->generation) return 0;  /* obsolete buffer, ignore */

    if (!(buffer = get_registered_buffer( win_data, res->buffer_id ))) return -ENOENT;

    LOG( TRACE, "%08x buffer %p\n", res->hdr.hwnd, buffer );
    ret = parent->queueBuffer( parent, buffer, -1 );
    return ret;
}

static int query_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_query *res = data;
    struct ANativeWindow *parent;
    struct native_win_data *win_data;
    int ret;

    if (in_size < sizeof(*res) || out_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    if (!(parent = win_data->parent)) return -EWOULDBLOCK;

    *ret_size = sizeof( *res );
    ret = parent->query( parent, res->what, &res->value );
    return ret;
}

static int perform_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_perform *res = data;
    struct ANativeWindow *parent;
    struct native_win_data *win_data;
    int ret = -ENOENT;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    if (!(parent = win_data->parent)) return -EWOULDBLOCK;

    switch (res->operation)
    {
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        ret = parent->perform( parent, res->operation, res->args[0] );
        if (!ret) win_data->buffer_format = res->args[0];
        break;
    case NATIVE_WINDOW_API_CONNECT:
        ret = parent->perform( parent, res->operation, res->args[0] );
        if (!ret) win_data->api = res->args[0];
        break;
    case NATIVE_WINDOW_API_DISCONNECT:
        ret = parent->perform( parent, res->operation, res->args[0] );
        if (!ret) win_data->api = 0;
        break;
    case NATIVE_WINDOW_SET_USAGE:
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
    case NATIVE_WINDOW_SET_SCALING_MODE:
        ret = parent->perform( parent, res->operation, res->args[0] );
        break;
    case NATIVE_WINDOW_SET_BUFFER_COUNT:
        ret = parent->perform( parent, res->operation, (size_t)res->args[0] );
        break;
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
        ret = parent->perform( parent, res->operation, res->args[0], res->args[1] );
        break;
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        ret = parent->perform( parent, res->operation, res->args[0], res->args[1], res->args[2] );
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        ret = parent->perform( parent, res->operation, res->args[0] | ((int64_t)res->args[1] << 32) );
        break;
    case NATIVE_WINDOW_CONNECT:
    case NATIVE_WINDOW_DISCONNECT:
    case NATIVE_WINDOW_UNLOCK_AND_POST:
        ret = parent->perform( parent, res->operation );
        break;
    case NATIVE_WINDOW_SET_CROP:
    {
        android_native_rect_t rect;
        rect.left   = res->args[0];
        rect.top    = res->args[1];
        rect.right  = res->args[2];
        rect.bottom = res->args[3];
        ret = parent->perform( parent, res->operation, &rect );
        break;
    }
    case NATIVE_WINDOW_LOCK:
    default:
        LOG( FIXME, "unsupported perform op %d\n", res->operation );
        break;
    }
    return ret;
}

static int setSwapInterval_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_set_swap_interval *res = data;
    struct ANativeWindow *parent;
    struct native_win_data *win_data;
    int ret;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;
    win_data->swap_interval = res->interval;

    if (!(parent = win_data->parent)) return 0;
    ret = parent->setSwapInterval( parent, res->interval );
    return ret;
}

static int setWindowParent_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_set_window_parent *res = data;
    struct native_win_data *win_data;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return -ENOENT;

    LOG( TRACE, "hwnd %08x parent %08x\n", res->hdr.hwnd, res->parent );

    if (!(object = load_java_method( env, &method, "setParent", "(II)V" ))) return -ENOSYS;

    (*env)->CallVoidMethod( env, object, method, res->hdr.hwnd, res->parent );
    return 0;
}

static int setCapture_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    struct ioctl_android_set_capture *res = data;

    if (in_size < sizeof(*res)) return -EINVAL;

    if (res->hdr.hwnd && !get_ioctl_native_win_data( &res->hdr )) return -ENOENT;

    LOG( TRACE, "hwnd %08x\n", res->hdr.hwnd );

    InterlockedExchangePointer( (void **)&capture_window, LongToHandle( res->hdr.hwnd ));
    return 0;
}

static int setCursor_ioctl( JNIEnv* env, void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd )
{
    static jmethodID method;
    jobject object;
    int size;
    struct ioctl_android_set_cursor *res = data;

    if (in_size < offsetof( struct ioctl_android_set_cursor, bits )) return -EINVAL;

    if (res->width < 0 || res->height < 0 || res->width > 256 || res->height > 256)
        return -EINVAL;

    size = res->width * res->height;
    if (in_size != offsetof( struct ioctl_android_set_cursor, bits[size] ))
        return -EINVAL;

    LOG( TRACE, "hwnd %08x size %d\n", res->hdr.hwnd, size );

    if (!(object = load_java_method( env, &method, "setCursor", "(IIIII[I)V" )))
        return -ENOSYS;

    if (size)
    {
        jintArray array = (*env)->NewIntArray( env, size );
        (*env)->SetIntArrayRegion( env, array, 0, size, (jint *)res->bits );
        (*env)->CallVoidMethod( env, object, method, 0, res->width, res->height,
                                    res->hotspotx, res->hotspoty, array );
        (*env)->DeleteLocalRef( env, array );
    }
    else (*env)->CallVoidMethod( env, object, method, res->id, 0, 0, 0, 0, NULL );

    return 0;
}

typedef int (*ioctl_func)( JNIEnv* env, void *in, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size, int *reply_fd );
static const ioctl_func ioctl_funcs[] =
{
    createWindow_ioctl,         /* IOCTL_CREATE_WINDOW */
    destroyWindow_ioctl,        /* IOCTL_DESTROY_WINDOW */
    windowPosChanged_ioctl,     /* IOCTL_WINDOW_POS_CHANGED */
    setWindowParent_ioctl,      /* IOCTL_SET_WINDOW_PARENT */
    dequeueBuffer_ioctl,        /* IOCTL_DEQUEUE_BUFFER */
    queueBuffer_ioctl,          /* IOCTL_QUEUE_BUFFER */
    cancelBuffer_ioctl,         /* IOCTL_CANCEL_BUFFER */
    query_ioctl,                /* IOCTL_QUERY */
    perform_ioctl,              /* IOCTL_PERFORM */
    setSwapInterval_ioctl,      /* IOCTL_SET_SWAP_INT */
    setCapture_ioctl,           /* IOCTL_SET_CAPTURE */
    setCursor_ioctl,            /* IOCTL_SET_CURSOR */
};

static ALooper *looper;
static JNIEnv *looper_env; /* JNIEnv for the main thread looper. Must only be used from that thread. */

void looper_init( JNIEnv* env, jobject obj )
{
    looper_env = env;
    if (!(looper = pALooper_forThread()))
    {
        LOG( ERR, "No looper for current thread\n" );
        abort();
    }
    pALooper_acquire( looper );
}

/* Handle a single ioctl request from a client socket.
 * Returns 0 if a request was handled successfully and the caller may
 * continue draining the socket, -1 if there is nothing more to read
 * for now, and 1 if the client fd should be closed.
 */
static int handle_ioctl_message( JNIEnv *env, int fd )
{
    char buffer[1024], control[CMSG_SPACE(sizeof(int))];
    int code = 0, status = -EINVAL, reply_fd = -1;
    ULONG_PTR reply_size = 0;
    ssize_t ret;
    struct iovec iov[2] = { { &code, sizeof(code) }, { buffer, sizeof(buffer) } };
    struct iovec reply_iov[2] = { { &status, sizeof(status) }, { buffer, 0 } };
    struct msghdr msg = { NULL, 0, iov, 2, NULL, 0, 0 };
    struct msghdr reply = { NULL, 0, reply_iov, 2, NULL, 0, 0 };
    struct cmsghdr *cmsg;

    ret = recvmsg( fd, &msg, MSG_DONTWAIT );
    if (ret < 0)
    {
        if (errno == EINTR) return 0;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return -1;
        return 1;
    }

    if (!ret || ret < sizeof(code)) return 1;
    ret -= sizeof(code);

    if ((unsigned int)code < NB_IOCTLS)
    {
        if (ret >= sizeof(struct ioctl_header))
        {
            pthread_mutex_lock( &dispatch_ioctl_lock );
            status = ioctl_funcs[code]( env, buffer, ret, sizeof(buffer), &reply_size, &reply_fd );
            pthread_mutex_unlock( &dispatch_ioctl_lock );
        }
    }
    else
    {
        LOG( FIXME, "ioctl %x not supported\n", code );
        status = -ENOTSUP;
    }

    reply_iov[1].iov_len = reply_size;
    if (reply_fd != -1)
    {
        reply.msg_control = control;
        reply.msg_controllen = sizeof(control);
        cmsg = CMSG_FIRSTHDR( &reply );
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN( sizeof(reply_fd) );
        memcpy( CMSG_DATA(cmsg), &reply_fd, sizeof(reply_fd) );
        reply.msg_controllen = cmsg->cmsg_len;
    }

    ret = sendmsg( fd, &reply, 0 );
    if (reply_fd != -1) close( reply_fd );
    return ret < 0 ? 1 : 0;
}

static int looper_handle_client( int fd, int events, void *data )
{
    for (;;)
    {
        int ret = (events & (ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR)) ? 1 : handle_ioctl_message( looper_env, fd );

        if (!ret) continue;

        if (ret > 0)
        {
            pALooper_removeFd( looper, fd );
            close( fd );
        }
        break;
    }

    return 1;
}

static int looper_handle_listen( int fd, int events, void *data )
{
    for (;;)
    {
        int client = accept4( fd, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK );

        if (client < 0)
        {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            LOG( ERR,  "accept4 failed: %s\n", strerror( errno ) );
            break;
        }

        if (pALooper_addFd( looper, client, client, ALOOPER_EVENT_INPUT | ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_ERROR, looper_handle_client, NULL ) != 1) {
            LOG( ERR, "Failed to add client to ALooper\n" );
            close( client );
        }
    }

    return 1;
}

static void *bootstrap_looper_thread( void *arg )
{
    JNIEnv *env;
    jmethodID method = NULL;
    jobject object = NULL;
    int sockfd;

    if (!java_vm || (*java_vm)->AttachCurrentThread( java_vm, &env, 0 ) != JNI_OK)
    {
        LOG( ERR,  "Failed to attach current thread\n" );
        return NULL;
    }

    if (!(object = load_java_method( env, &method, "obtainLooper", "()V" )))
    {
        LOG( ERR,  "Failed to obtain looper\n" );
        abort();
    }
    (*env)->CallVoidMethod( env, object, method );

    sockfd = socket( AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0 );
    if (sockfd < 0)
    {
        LOG( ERR,  "Failed to open server socket: %s\n", strerror( errno ) );
        abort();
    }

    if (bind( sockfd, (const struct sockaddr *)&ipc_addr, IPC_SOCKET_ADDR_LEN ) < 0 ||
        listen( sockfd, 32 ) < 0)
    {
        LOG( ERR, "Failed to bind server socket: %s\n", strerror( errno ) );
        close(sockfd);
        abort();
    }

    if (pALooper_addFd( looper, sockfd, sockfd, ALOOPER_EVENT_INPUT, looper_handle_listen, NULL ) != 1) {
        LOG( ERR, "Failed to add listening socket to main looper\n" );
        close(sockfd);
        abort();
    }

    create_desktop_view( env );

    (*java_vm)->DetachCurrentThread( java_vm );
    return NULL;
}

void start_android_device(void)
{
    pthread_t t;

    log_flags = __wine_dbg_get_channel_flags(&__wine_dbch_android);

    /* Use a temporary bootstrap thread to request the main thread looper
     * without interfering with the current Wine/JVM execution context
     * (including register and thread-state assumptions). Actual ioctl
     * dispatch then runs from the main thread looper.
     */
    if (!pthread_create( &t, NULL, bootstrap_looper_thread, NULL ))
        pthread_join(t, NULL);
    else
    {
        LOG( ERR, "Failed to spawn looper bootstrap thread\n" );
        abort();
    }
}


/* Client-side ioctl support */


static int android_ioctl( enum android_ioctl code, void *in, DWORD in_size, void *out, DWORD *out_size, int *recv_fd )
{
    static int device_fd = -1;
    static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;
    int status, err = -ENOENT;
    ssize_t ret;
    char control[CMSG_SPACE(sizeof(int))];
    struct iovec iov[2] = { { &status, sizeof(status) }, { out, out_size ? *out_size : 0 } };
    struct msghdr msg = { NULL, 0, iov, (out && out_size) ? 2 : 1,
                          recv_fd ? control : NULL, recv_fd ? sizeof(control) : 0, 0 };
    struct cmsghdr *cmsg;

    pthread_mutex_lock( &device_mutex );

    if (recv_fd) *recv_fd = -1;

    if (device_fd == -1)
    {
        device_fd = socket( AF_UNIX, SOCK_SEQPACKET, 0 );
        if (device_fd < 0) goto done;
        if (connect( device_fd, (const struct sockaddr *)&ipc_addr, IPC_SOCKET_ADDR_LEN ) < 0)
        {
            close( device_fd );
            device_fd = -1;
            goto done;
        }
    }

    ret = writev( device_fd, (struct iovec[]){ { &code, sizeof(code) }, { in, in_size } }, 2 );
    if (ret <= 0 || ret != sizeof(code) + in_size) goto disconnected;

    ret = recvmsg( device_fd, &msg, 0 );
    if (ret <= 0 || ret < sizeof(status)) goto disconnected;

    if (out && out_size) *out_size = ret - sizeof(status);
    err = status;

    if (recv_fd)
        for (cmsg = CMSG_FIRSTHDR( &msg ); cmsg; cmsg = CMSG_NXTHDR( &msg, cmsg ))
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS &&
                cmsg->cmsg_len >= CMSG_LEN(sizeof(int)))
            {
                memcpy( recv_fd, CMSG_DATA(cmsg), sizeof(int) );
                break;
            }

    goto done;

disconnected:
    close( device_fd );
    device_fd = -1;
    WARN( "parent process is gone\n" );
    NtTerminateProcess( 0, 1 );
    err = -ENOENT;

done:
    pthread_mutex_unlock( &device_mutex );
    return err;
}

static void win_incRef( struct android_native_base_t *base )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)base;
    InterlockedIncrement( &win->ref );
}

static void win_decRef( struct android_native_base_t *base )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)base;
    InterlockedDecrement( &win->ref );
}

static int dequeueBuffer( struct ANativeWindow *window, struct ANativeWindowBuffer **buffer, int *fence )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_dequeueBuffer res = {0};
    DWORD size = sizeof(res);
    int ret, buffer_fd = -1;

    res.hdr.hwnd = HandleToLong( win->hwnd );
    res.hdr.opengl = win->opengl;
    res.buffer_id = -1;
    res.generation = 0;

    ret = android_ioctl( IOCTL_DEQUEUE_BUFFER, &res, size, &res, &size, &buffer_fd );
    if (ret) return ret;
    if (size < sizeof(res)) return -EINVAL;

    if (res.buffer_id < 0 || res.buffer_id >= NB_CACHED_BUFFERS) return -EINVAL;

    if (buffer_fd != -1)
    {
        AHardwareBuffer *ahb = NULL;
        ret = pAHardwareBuffer_recvHandleFromUnixSocket( buffer_fd, &ahb );
        close( buffer_fd );
        if (ret) return ret;

        if (win->buffers[res.buffer_id].self)
            pAHardwareBuffer_release( win->buffers[res.buffer_id].self );

        win->buffers[res.buffer_id].self = ahb;
        win->buffers[res.buffer_id].buffer_id = res.buffer_id;
        win->buffers[res.buffer_id].generation = res.generation;
    }

    if (!win->buffers[res.buffer_id].self) return -EINVAL;

    *buffer = anwb_from_ahb(win->buffers[res.buffer_id].self);
    *fence = -1;

    TRACE( "hwnd %p, buffer %p id %d gen %d fence %d\n",
           win->hwnd, *buffer, res.buffer_id, res.generation, *fence );
    return 0;
}

static int cancelBuffer( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fence )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_cancelBuffer cancel;

    TRACE( "hwnd %p buffer %p fence %d\n", win->hwnd, buffer, fence );

    if (!ahb_from_anwb( win, buffer, &cancel.buffer_id, &cancel.generation ))
    {
        wait_fence_and_close( fence );
        return -EINVAL;
    }

    cancel.hdr.hwnd = HandleToLong( win->hwnd );
    cancel.hdr.opengl = win->opengl;
    wait_fence_and_close( fence );
    return android_ioctl( IOCTL_CANCEL_BUFFER, &cancel, sizeof(cancel), NULL, NULL, NULL );
}

static int queueBuffer( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fence )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_queueBuffer queue;

    TRACE( "hwnd %p buffer %p fence %d\n", win->hwnd, buffer, fence );

    if (!ahb_from_anwb( win, buffer, &queue.buffer_id, &queue.generation ))
    {
        wait_fence_and_close( fence );
        return -EINVAL;
    }

    queue.hdr.hwnd = HandleToLong( win->hwnd );
    queue.hdr.opengl = win->opengl;
    wait_fence_and_close( fence );
    return android_ioctl( IOCTL_QUEUE_BUFFER, &queue, sizeof(queue), NULL, NULL, NULL );
}

static int dequeueBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer **buffer )
{
    int fence, ret = dequeueBuffer( window, buffer, &fence );

    if (!ret) wait_fence_and_close( fence );
    return ret;
}

static int cancelBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return cancelBuffer( window, buffer, -1 );
}

static int lockBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return 0;  /* nothing to do */
}

static int queueBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return queueBuffer( window, buffer, -1 );
}

static int setSwapInterval( struct ANativeWindow *window, int interval )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_set_swap_interval swap;

    TRACE( "hwnd %p interval %d\n", win->hwnd, interval );
    swap.hdr.hwnd = HandleToLong( win->hwnd );
    swap.hdr.opengl = win->opengl;
    swap.interval = interval;
    return android_ioctl( IOCTL_SET_SWAP_INT, &swap, sizeof(swap), NULL, NULL, NULL );
}

static int query( const ANativeWindow *window, int what, int *value )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_query query;
    DWORD size = sizeof( query );
    int ret;

    query.hdr.hwnd = HandleToLong( win->hwnd );
    query.hdr.opengl = win->opengl;
    query.what = what;
    ret = android_ioctl( IOCTL_QUERY, &query, sizeof(query), &query, &size, NULL );
    TRACE( "hwnd %p what %d got %d -> %p\n", win->hwnd, what, query.value, value );
    if (!ret) *value = query.value;
    return ret;
}

static int perform( ANativeWindow *window, int operation, ... )
{
    static const char * const names[] =
    {
        "SET_USAGE", "CONNECT", "DISCONNECT", "SET_CROP", "SET_BUFFER_COUNT", "SET_BUFFERS_GEOMETRY",
        "SET_BUFFERS_TRANSFORM", "SET_BUFFERS_TIMESTAMP", "SET_BUFFERS_DIMENSIONS", "SET_BUFFERS_FORMAT",
        "SET_SCALING_MODE", "LOCK", "UNLOCK_AND_POST", "API_CONNECT", "API_DISCONNECT",
        "SET_BUFFERS_USER_DIMENSIONS", "SET_POST_TRANSFORM_CROP"
    };

    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_perform perf;
    va_list args;

    perf.hdr.hwnd  = HandleToLong( win->hwnd );
    perf.hdr.opengl = win->opengl;
    perf.operation = operation;
    memset( perf.args, 0, sizeof(perf.args) );

    va_start( args, operation );
    switch (operation)
    {
    case NATIVE_WINDOW_SET_USAGE:
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
    case NATIVE_WINDOW_SET_SCALING_MODE:
    case NATIVE_WINDOW_API_CONNECT:
    case NATIVE_WINDOW_API_DISCONNECT:
        perf.args[0] = va_arg( args, int );
        TRACE( "hwnd %p %s arg %d\n", win->hwnd, names[operation], perf.args[0] );
        break;
    case NATIVE_WINDOW_SET_BUFFER_COUNT:
        perf.args[0] = va_arg( args, size_t );
        TRACE( "hwnd %p %s count %d\n", win->hwnd, names[operation], perf.args[0] );
        break;
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
        perf.args[0] = va_arg( args, int );
        perf.args[1] = va_arg( args, int );
        TRACE( "hwnd %p %s arg %dx%d\n", win->hwnd, names[operation], perf.args[0], perf.args[1] );
        break;
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        perf.args[0] = va_arg( args, int );
        perf.args[1] = va_arg( args, int );
        perf.args[2] = va_arg( args, int );
        TRACE( "hwnd %p %s arg %dx%d %d\n", win->hwnd, names[operation],
               perf.args[0], perf.args[1], perf.args[2] );
        break;
    case NATIVE_WINDOW_SET_CROP:
    {
        android_native_rect_t *rect = va_arg( args, android_native_rect_t * );
        perf.args[0] = rect->left;
        perf.args[1] = rect->top;
        perf.args[2] = rect->right;
        perf.args[3] = rect->bottom;
        TRACE( "hwnd %p %s rect %d,%d-%d,%d\n", win->hwnd, names[operation],
               perf.args[0], perf.args[1], perf.args[2], perf.args[3] );
        break;
    }
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
    {
        int64_t timestamp = va_arg( args, int64_t );
        perf.args[0] = timestamp;
        perf.args[1] = timestamp >> 32;
        TRACE( "hwnd %p %s arg %08x%08x\n", win->hwnd, names[operation], perf.args[1], perf.args[0] );
        break;
    }
    case NATIVE_WINDOW_LOCK:
    {
        struct ANativeWindowBuffer *buffer = NULL;
        struct ANativeWindow_Buffer *buffer_ret = va_arg( args, ANativeWindow_Buffer * );
        struct AHardwareBuffer* b = NULL;
        ARect *bounds = va_arg( args, ARect * );
        int ret = window->dequeueBuffer_DEPRECATED( window, &buffer );
        if (!ret && !buffer)
        {
            ret = -EWOULDBLOCK;
            TRACE( "got invalid buffer\n" );
        }
        if (!ret)
        {
            if (!(b = ahb_from_anwb((struct native_win_wrapper*) window, buffer, NULL, NULL))) {
                ret = -EINVAL;
            }

            if (b && (ret = pAHardwareBuffer_lock( b, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, NULL, &buffer_ret->bits )))
            {
                WARN( "AHardwareBuffer_lock %p failed %d %s\n", win->hwnd, ret, strerror(-ret) );
                window->cancelBuffer( window, buffer, -1 );
            }
        }
        if (!ret)
        {
            AHardwareBuffer_Desc d = {0};
            pAHardwareBuffer_describe(b, &d);
            buffer_ret->width  = d.width;
            buffer_ret->height = d.height;
            buffer_ret->stride = d.stride;
            buffer_ret->format = d.format;
            win->locked_buffer = b;
            if (bounds)
            {
                bounds->left   = 0;
                bounds->top    = 0;
                bounds->right  = d.width;
                bounds->bottom = d.height;
            }
        }
        va_end( args );
        TRACE( "hwnd %p %s bits %p ret %d %s\n", win->hwnd, names[operation], buffer_ret->bits, ret, strerror(-ret) );
        return ret;
    }
    case NATIVE_WINDOW_UNLOCK_AND_POST:
    {
        int ret = -EINVAL;
        if (win->locked_buffer)
        {
            pAHardwareBuffer_unlock(win->locked_buffer, NULL);
            ret = window->queueBuffer( window, anwb_from_ahb(win->locked_buffer), -1 );
            win->locked_buffer = NULL;
        }
        va_end( args );
        TRACE( "hwnd %p %s ret %d\n", win->hwnd, names[operation], ret );
        return ret;
    }
    case NATIVE_WINDOW_CONNECT:
    case NATIVE_WINDOW_DISCONNECT:
        TRACE( "hwnd %p %s\n", win->hwnd, names[operation] );
        break;
    case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP:
    default:
        FIXME( "unsupported perform hwnd %p op %d %s\n", win->hwnd, operation,
               operation < ARRAY_SIZE( names ) ? names[operation] : "???" );
        break;
    }
    va_end( args );
    return android_ioctl( IOCTL_PERFORM, &perf, sizeof(perf), NULL, NULL, NULL );
}

struct ANativeWindow *create_ioctl_window( HWND hwnd, BOOL opengl )
{
    struct ioctl_android_create_window req;
    struct native_win_wrapper *win = calloc( 1, sizeof(*win) );

    if (!win) return NULL;

    win->win.common.magic             = ANDROID_NATIVE_WINDOW_MAGIC;
    win->win.common.version           = sizeof(ANativeWindow);
    win->win.common.incRef            = win_incRef;
    win->win.common.decRef            = win_decRef;
    win->win.setSwapInterval          = setSwapInterval;
    win->win.dequeueBuffer_DEPRECATED = dequeueBuffer_DEPRECATED;
    win->win.lockBuffer_DEPRECATED    = lockBuffer_DEPRECATED;
    win->win.queueBuffer_DEPRECATED   = queueBuffer_DEPRECATED;
    win->win.query                    = query;
    win->win.perform                  = perform;
    win->win.cancelBuffer_DEPRECATED  = cancelBuffer_DEPRECATED;
    win->win.dequeueBuffer            = dequeueBuffer;
    win->win.queueBuffer              = queueBuffer;
    win->win.cancelBuffer             = cancelBuffer;
    win->ref  = 1;
    win->hwnd = hwnd;
    win->opengl = opengl;
    TRACE( "-> %p %p opengl=%u\n", win, win->hwnd, opengl );

    req.hdr.hwnd = HandleToLong( win->hwnd );
    req.hdr.opengl = win->opengl;
    req.parent = get_ioctl_win_parent( NtUserGetAncestor( hwnd, GA_PARENT ));
    req.is_desktop = hwnd == desktop_window;
    android_ioctl( IOCTL_CREATE_WINDOW, &req, sizeof(req), NULL, NULL, NULL );

    return &win->win;
}

struct ANativeWindow *grab_ioctl_window( struct ANativeWindow *window )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    InterlockedIncrement( &win->ref );
    return window;
}

void release_ioctl_window( struct ANativeWindow *window )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    unsigned int i;

    if (InterlockedDecrement( &win->ref ) > 0) return;

    TRACE( "%p %p\n", win, win->hwnd );
    for (i = 0; i < ARRAY_SIZE( win->buffers ); i++)
        if (win->buffers[i].self) pAHardwareBuffer_release(win->buffers[i].self);

    destroy_ioctl_window( win->hwnd, win->opengl );
    free( win );
}

void destroy_ioctl_window( HWND hwnd, BOOL opengl )
{
    struct ioctl_android_destroy_window req;

    req.hdr.hwnd = HandleToLong( hwnd );
    req.hdr.opengl = opengl;
    android_ioctl( IOCTL_DESTROY_WINDOW, &req, sizeof(req), NULL, NULL, NULL );
}

int ioctl_window_pos_changed( HWND hwnd, const struct window_rects *rects,
                              UINT style, UINT flags, HWND after, HWND owner )
{
    struct ioctl_android_window_pos_changed req;

    req.hdr.hwnd     = HandleToLong( hwnd );
    req.hdr.opengl   = FALSE;
    req.window_rect  = rects->window;
    req.client_rect  = rects->client;
    req.visible_rect = rects->visible;
    req.style        = style;
    req.flags        = flags;
    req.after        = HandleToLong( after );
    req.owner        = HandleToLong( owner );
    return android_ioctl( IOCTL_WINDOW_POS_CHANGED, &req, sizeof(req), NULL, NULL, NULL );
}

int ioctl_set_window_parent( HWND hwnd, HWND parent )
{
    struct ioctl_android_set_window_parent req;

    req.hdr.hwnd = HandleToLong( hwnd );
    req.hdr.opengl = FALSE;
    req.parent = get_ioctl_win_parent( parent );
    return android_ioctl( IOCTL_SET_WINDOW_PARENT, &req, sizeof(req), NULL, NULL, NULL );
}

int ioctl_set_capture( HWND hwnd )
{
    struct ioctl_android_set_capture req;

    req.hdr.hwnd  = HandleToLong( hwnd );
    req.hdr.opengl = FALSE;
    return android_ioctl( IOCTL_SET_CAPTURE, &req, sizeof(req), NULL, NULL, NULL );
}

int ioctl_set_cursor( int id, int width, int height,
                      int hotspotx, int hotspoty, const unsigned int *bits )
{
    struct ioctl_android_set_cursor *req;
    unsigned int size = offsetof( struct ioctl_android_set_cursor, bits[width * height] );
    int ret;

    if (!(req = malloc( size ))) return -ENOMEM;
    req->hdr.hwnd   = 0;  /* unused */
    req->hdr.opengl = FALSE;
    req->id       = id;
    req->width    = width;
    req->height   = height;
    req->hotspotx = hotspotx;
    req->hotspoty = hotspoty;
    memcpy( req->bits, bits, width * height * sizeof(req->bits[0]) );
    ret = android_ioctl( IOCTL_SET_CURSOR, req, size, NULL, NULL, NULL );
    free( req );
    return ret;
}

/**********************************************************************
 *           ANDROID_SetDesktopWindow
 */
void ANDROID_SetDesktopWindow( HWND hwnd )
{
    desktop_window = hwnd;
}
