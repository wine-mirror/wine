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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "android.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

extern NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event );
static HANDLE stop_event;
static HANDLE thread;
static JNIEnv *jni_env;

#define ANDROIDCONTROLTYPE  ((ULONG)'A')
#define ANDROID_IOCTL(n) CTL_CODE(ANDROIDCONTROLTYPE, n, METHOD_BUFFERED, FILE_READ_ACCESS)

enum android_ioctl
{
    IOCTL_CREATE_WINDOW,
    IOCTL_DESTROY_WINDOW,
    IOCTL_WINDOW_POS_CHANGED,
    IOCTL_QUERY,
    IOCTL_PERFORM,
    NB_IOCTLS
};

/* data about the native window in the context of the Java process */
struct native_win_data
{
    struct ANativeWindow       *parent;
    HWND                        hwnd;
    int                         api;
    int                         buffer_format;
};

/* wrapper for a native window in the context of the client (non-Java) process */
struct native_win_wrapper
{
    struct ANativeWindow          win;
    HWND                          hwnd;
    LONG                          ref;
};

struct ioctl_header
{
    int  hwnd;
};

struct ioctl_android_create_window
{
    struct ioctl_header hdr;
    int                 parent;
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

static inline DWORD current_client_id(void)
{
    return HandleToUlong( PsGetCurrentProcessId() );
}

#ifdef __i386__  /* the Java VM uses %fs for its own purposes, so we need to wrap the calls */
static WORD orig_fs, java_fs;
static inline void wrap_java_call(void)   { wine_set_fs( java_fs ); }
static inline void unwrap_java_call(void) { wine_set_fs( orig_fs ); }
#else
static inline void wrap_java_call(void) { }
static inline void unwrap_java_call(void) { }
#endif  /* __i386__ */

static struct native_win_data *data_map[65536];

static unsigned int data_map_idx( HWND hwnd )
{
    return LOWORD(hwnd);
}

static struct native_win_data *get_native_win_data( HWND hwnd )
{
    struct native_win_data *data = data_map[data_map_idx( hwnd )];

    if (data && data->hwnd == hwnd) return data;
    WARN( "unknown win %p\n", hwnd );
    return NULL;
}

static struct native_win_data *get_ioctl_native_win_data( const struct ioctl_header *hdr )
{
    return get_native_win_data( LongToHandle(hdr->hwnd) );
}

static void release_native_window( struct native_win_data *data )
{
    if (data->parent) pANativeWindow_release( data->parent );
}

static void free_native_win_data( struct native_win_data *data )
{
    unsigned int idx = data_map_idx( data->hwnd );

    release_native_window( data );
    HeapFree( GetProcessHeap(), 0, data );
    data_map[idx] = NULL;
}

static struct native_win_data *create_native_win_data( HWND hwnd )
{
    unsigned int idx = data_map_idx( hwnd );
    struct native_win_data *data = data_map[idx];

    if (data)
    {
        WARN( "data for %p not freed correctly\n", data->hwnd );
        free_native_win_data( data );
    }
    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) ))) return NULL;
    data->hwnd = hwnd;
    data->api = NATIVE_WINDOW_API_CPU;
    data->buffer_format = PF_BGRA_8888;
    data_map[idx] = data;
    return data;
}

static void CALLBACK register_native_window_callback( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    HWND hwnd = (HWND)arg1;
    struct ANativeWindow *win = (struct ANativeWindow *)arg2;
    struct native_win_data *data = get_native_win_data( hwnd );

    if (!data || data->parent == win)
    {
        if (win) pANativeWindow_release( win );
        TRACE( "%p -> %p win %p (unchanged)\n", hwnd, data, win );
        return;
    }

    release_native_window( data );
    data->parent = win;
    if (win)
    {
        wrap_java_call();
        if (data->api) win->perform( win, NATIVE_WINDOW_API_CONNECT, data->api );
        win->perform( win, NATIVE_WINDOW_SET_BUFFERS_FORMAT, data->buffer_format );
        unwrap_java_call();
    }
    TRACE( "%p -> %p win %p\n", hwnd, data, win );
}

/* register a native window received from the Java side for use in ioctls */
void register_native_window( HWND hwnd, struct ANativeWindow *win )
{
    NtQueueApcThread( thread, register_native_window_callback, (ULONG_PTR)hwnd, (ULONG_PTR)win, 0 );
}

static NTSTATUS android_error_to_status( int err )
{
    switch (err)
    {
    case 0:            return STATUS_SUCCESS;
    case -ENOMEM:      return STATUS_NO_MEMORY;
    case -ENOSYS:      return STATUS_NOT_SUPPORTED;
    case -EINVAL:      return STATUS_INVALID_PARAMETER;
    case -ENOENT:      return STATUS_INVALID_HANDLE;
    case -EPERM:       return STATUS_ACCESS_DENIED;
    case -ENODEV:      return STATUS_NO_SUCH_DEVICE;
    case -EEXIST:      return STATUS_DUPLICATE_NAME;
    case -EPIPE:       return STATUS_PIPE_DISCONNECTED;
    case -ENODATA:     return STATUS_NO_MORE_FILES;
    case -ETIMEDOUT:   return STATUS_IO_TIMEOUT;
    case -EBADMSG:     return STATUS_INVALID_DEVICE_REQUEST;
    case -EWOULDBLOCK: return STATUS_DEVICE_NOT_READY;
    default:
        FIXME( "unmapped error %d\n", err );
        return STATUS_UNSUCCESSFUL;
    }
}

static int status_to_android_error( NTSTATUS status )
{
    switch (status)
    {
    case STATUS_SUCCESS:                return 0;
    case STATUS_NO_MEMORY:              return -ENOMEM;
    case STATUS_NOT_SUPPORTED:          return -ENOSYS;
    case STATUS_INVALID_PARAMETER:      return -EINVAL;
    case STATUS_BUFFER_OVERFLOW:        return -EINVAL;
    case STATUS_INVALID_HANDLE:         return -ENOENT;
    case STATUS_ACCESS_DENIED:          return -EPERM;
    case STATUS_NO_SUCH_DEVICE:         return -ENODEV;
    case STATUS_DUPLICATE_NAME:         return -EEXIST;
    case STATUS_PIPE_DISCONNECTED:      return -EPIPE;
    case STATUS_NO_MORE_FILES:          return -ENODATA;
    case STATUS_IO_TIMEOUT:             return -ETIMEDOUT;
    case STATUS_INVALID_DEVICE_REQUEST: return -EBADMSG;
    case STATUS_DEVICE_NOT_READY:       return -EWOULDBLOCK;
    default:
        FIXME( "unmapped status %08x\n", status );
        return -EINVAL;
    }
}

static jobject load_java_method( jmethodID *method, const char *name, const char *args )
{
    jobject object = wine_get_java_object();

    if (!*method)
    {
        jclass class;

        wrap_java_call();
        class = (*jni_env)->GetObjectClass( jni_env, object );
        *method = (*jni_env)->GetMethodID( jni_env, class, name, args );
        unwrap_java_call();
        if (!*method)
        {
            FIXME( "method %s not found\n", name );
            return NULL;
        }
    }
    return object;
}

static void create_desktop_window( HWND hwnd )
{
    static jmethodID method;
    jobject object;

    if (!(object = load_java_method( &method, "createDesktopWindow", "(I)V" ))) return;

    wrap_java_call();
    (*jni_env)->CallVoidMethod( jni_env, object, method, HandleToLong( hwnd ));
    unwrap_java_call();
}

static NTSTATUS createWindow_ioctl( void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_create_window *res = data;
    struct native_win_data *win_data;
    DWORD pid = current_client_id();

    if (in_size < sizeof(*res)) return STATUS_INVALID_PARAMETER;

    if (!(win_data = create_native_win_data( LongToHandle(res->hdr.hwnd) )))
        return STATUS_NO_MEMORY;

    TRACE( "hwnd %08x parent %08x\n", res->hdr.hwnd, res->parent );

    if (!(object = load_java_method( &method, "createWindow", "(III)V" ))) return STATUS_NOT_SUPPORTED;

    wrap_java_call();
    (*jni_env)->CallVoidMethod( jni_env, object, method, res->hdr.hwnd, res->parent, pid );
    unwrap_java_call();
    return STATUS_SUCCESS;
}

static NTSTATUS destroyWindow_ioctl( void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_destroy_window *res = data;
    struct native_win_data *win_data;

    if (in_size < sizeof(*res)) return STATUS_INVALID_PARAMETER;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return STATUS_INVALID_HANDLE;

    TRACE( "hwnd %08x\n", res->hdr.hwnd );

    if (!(object = load_java_method( &method, "destroyWindow", "(I)V" ))) return STATUS_NOT_SUPPORTED;

    wrap_java_call();
    (*jni_env)->CallVoidMethod( jni_env, object, method, res->hdr.hwnd );
    unwrap_java_call();
    free_native_win_data( win_data );
    return STATUS_SUCCESS;
}

static NTSTATUS windowPosChanged_ioctl( void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size )
{
    static jmethodID method;
    jobject object;
    struct ioctl_android_window_pos_changed *res = data;

    if (in_size < sizeof(*res)) return STATUS_INVALID_PARAMETER;

    TRACE( "hwnd %08x win %s client %s visible %s style %08x flags %08x after %08x owner %08x\n",
           res->hdr.hwnd, wine_dbgstr_rect(&res->window_rect), wine_dbgstr_rect(&res->client_rect),
           wine_dbgstr_rect(&res->visible_rect), res->style, res->flags, res->after, res->owner );

    if (!(object = load_java_method( &method, "windowPosChanged", "(IIIIIIIIIIIIIIIII)V" )))
        return STATUS_NOT_SUPPORTED;

    wrap_java_call();
    (*jni_env)->CallVoidMethod( jni_env, object, method, res->hdr.hwnd, res->flags, res->after, res->owner, res->style,
                                res->window_rect.left, res->window_rect.top, res->window_rect.right, res->window_rect.bottom,
                                res->client_rect.left, res->client_rect.top, res->client_rect.right, res->client_rect.bottom,
                                res->visible_rect.left, res->visible_rect.top, res->visible_rect.right, res->visible_rect.bottom );
    unwrap_java_call();
    return STATUS_SUCCESS;
}

static NTSTATUS query_ioctl( void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size )
{
    struct ioctl_android_query *res = data;
    struct ANativeWindow *parent;
    struct native_win_data *win_data;
    int ret;

    if (in_size < sizeof(*res)) return STATUS_INVALID_PARAMETER;
    if (out_size < sizeof(*res)) return STATUS_BUFFER_OVERFLOW;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return STATUS_INVALID_HANDLE;
    if (!(parent = win_data->parent)) return STATUS_DEVICE_NOT_READY;

    *ret_size = sizeof( *res );
    wrap_java_call();
    ret = parent->query( parent, res->what, &res->value );
    unwrap_java_call();
    return android_error_to_status( ret );
}

static NTSTATUS perform_ioctl( void *data, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size )
{
    struct ioctl_android_perform *res = data;
    struct ANativeWindow *parent;
    struct native_win_data *win_data;
    int ret = -ENOENT;

    if (in_size < sizeof(*res)) return STATUS_INVALID_PARAMETER;

    if (!(win_data = get_ioctl_native_win_data( &res->hdr ))) return STATUS_INVALID_HANDLE;
    if (!(parent = win_data->parent)) return STATUS_DEVICE_NOT_READY;

    switch (res->operation)
    {
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0] );
        unwrap_java_call();
        if (!ret) win_data->buffer_format = res->args[0];
        break;
    case NATIVE_WINDOW_API_CONNECT:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0] );
        unwrap_java_call();
        if (!ret) win_data->api = res->args[0];
        break;
    case NATIVE_WINDOW_API_DISCONNECT:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0] );
        unwrap_java_call();
        if (!ret) win_data->api = 0;
        break;
    case NATIVE_WINDOW_SET_USAGE:
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
    case NATIVE_WINDOW_SET_SCALING_MODE:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0] );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_SET_BUFFER_COUNT:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, (size_t)res->args[0] );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0], res->args[1] );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0], res->args[1], res->args[2] );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        wrap_java_call();
        ret = parent->perform( parent, res->operation, res->args[0] | ((int64_t)res->args[1] << 32) );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_CONNECT:
    case NATIVE_WINDOW_DISCONNECT:
    case NATIVE_WINDOW_UNLOCK_AND_POST:
        wrap_java_call();
        ret = parent->perform( parent, res->operation );
        unwrap_java_call();
        break;
    case NATIVE_WINDOW_SET_CROP:
    {
        android_native_rect_t rect;
        rect.left   = res->args[0];
        rect.top    = res->args[1];
        rect.right  = res->args[2];
        rect.bottom = res->args[3];
        wrap_java_call();
        ret = parent->perform( parent, res->operation, &rect );
        unwrap_java_call();
        break;
    }
    case NATIVE_WINDOW_LOCK:
    default:
        FIXME( "unsupported perform op %d\n", res->operation );
        break;
    }
    return android_error_to_status( ret );
}

typedef NTSTATUS (*ioctl_func)( void *in, DWORD in_size, DWORD out_size, ULONG_PTR *ret_size );
static const ioctl_func ioctl_funcs[] =
{
    createWindow_ioctl,         /* IOCTL_CREATE_WINDOW */
    destroyWindow_ioctl,        /* IOCTL_DESTROY_WINDOW */
    windowPosChanged_ioctl,     /* IOCTL_WINDOW_POS_CHANGED */
    query_ioctl,                /* IOCTL_QUERY */
    perform_ioctl,              /* IOCTL_PERFORM */
};

static NTSTATUS WINAPI ioctl_callback( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    DWORD code = (irpsp->Parameters.DeviceIoControl.IoControlCode - ANDROID_IOCTL(0)) >> 2;

    if (code < NB_IOCTLS)
    {
        struct ioctl_header *header = irp->AssociatedIrp.SystemBuffer;
        DWORD in_size = irpsp->Parameters.DeviceIoControl.InputBufferLength;
        ioctl_func func = ioctl_funcs[code];

        if (in_size >= sizeof(*header))
        {
            irp->IoStatus.Information = 0;
            irp->IoStatus.u.Status = func( irp->AssociatedIrp.SystemBuffer, in_size,
                                           irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                                           &irp->IoStatus.Information );
        }
        else irp->IoStatus.u.Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        FIXME( "ioctl %x not supported\n", irpsp->Parameters.DeviceIoControl.IoControlCode );
        irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
    }
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS CALLBACK init_android_driver( DRIVER_OBJECT *driver, UNICODE_STRING *name )
{
    static const WCHAR device_nameW[] = {'\\','D','e','v','i','c','e','\\','W','i','n','e','A','n','d','r','o','i','d',0 };
    static const WCHAR device_linkW[] = {'\\','?','?','\\','W','i','n','e','A','n','d','r','o','i','d',0 };

    UNICODE_STRING nameW, linkW;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ioctl_callback;

    RtlInitUnicodeString( &nameW, device_nameW );
    RtlInitUnicodeString( &linkW, device_linkW );

    if ((status = IoCreateDevice( driver, 0, &nameW, 0, 0, FALSE, &device ))) return status;
    return IoCreateSymbolicLink( &linkW, &nameW );
}

static DWORD CALLBACK device_thread( void *arg )
{
    static const WCHAR driver_nameW[] = {'\\','D','r','i','v','e','r','\\','W','i','n','e','A','n','d','r','o','i','d',0 };

    HANDLE start_event = arg;
    UNICODE_STRING nameW;
    NTSTATUS status;
    JavaVM *java_vm;
    DWORD ret;

    TRACE( "starting process %x\n", GetCurrentProcessId() );

    if (!(java_vm = wine_get_java_vm())) return 0;  /* not running under Java */

#ifdef __i386__
    orig_fs = wine_get_fs();
    (*java_vm)->AttachCurrentThread( java_vm, &jni_env, 0 );
    java_fs = wine_get_fs();
    wine_set_fs( orig_fs );
    if (java_fs != orig_fs) TRACE( "%%fs changed from %04x to %04x by Java VM\n", orig_fs, java_fs );
#else
    (*java_vm)->AttachCurrentThread( java_vm, &jni_env, 0 );
#endif

    create_desktop_window( GetDesktopWindow() );

    RtlInitUnicodeString( &nameW, driver_nameW );
    if ((status = IoCreateDriver( &nameW, init_android_driver )))
    {
        FIXME( "failed to create driver error %x\n", status );
        return status;
    }

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    SetEvent( start_event );

    ret = wine_ntoskrnl_main_loop( stop_event );

    (*java_vm)->DetachCurrentThread( java_vm );
    return ret;
}

void start_android_device(void)
{
    HANDLE handles[2];

    handles[0] = CreateEventW( NULL, TRUE, FALSE, NULL );
    handles[1] = thread = CreateThread( NULL, 0, device_thread, handles[0], 0, NULL );
    WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    CloseHandle( handles[0] );
}


/* Client-side ioctl support */


static int android_ioctl( enum android_ioctl code, void *in, DWORD in_size, void *out, DWORD *out_size )
{
    static const WCHAR deviceW[] = {'\\','\\','.','\\','W','i','n','e','A','n','d','r','o','i','d',0 };
    static HANDLE device;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    if (!device)
    {
        HANDLE file = CreateFileW( deviceW, GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (file == INVALID_HANDLE_VALUE) return -ENOENT;
        if (InterlockedCompareExchangePointer( &device, file, NULL )) CloseHandle( file );
    }

    status = NtDeviceIoControlFile( device, NULL, NULL, NULL, &iosb, ANDROID_IOCTL(code),
                                    in, in_size, out, out_size ? *out_size : 0 );
    if (status == STATUS_FILE_DELETED)
    {
        WARN( "parent process is gone\n" );
        ExitProcess( 1 );
    }
    if (out_size) *out_size = iosb.Information;
    return status_to_android_error( status );
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
    return 0;
}

static int cancelBuffer( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fence )
{
    return 0;
}

static int queueBuffer( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer, int fence )
{
    return 0;
}

static int dequeueBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer **buffer )
{
    return 0;
}

static int cancelBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return 0;
}

static int lockBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return 0;  /* nothing to do */
}

static int queueBuffer_DEPRECATED( struct ANativeWindow *window, struct ANativeWindowBuffer *buffer )
{
    return 0;
}

static int setSwapInterval( struct ANativeWindow *window, int interval )
{
    return 0;
}

static int query( const ANativeWindow *window, int what, int *value )
{
    struct native_win_wrapper *win = (struct native_win_wrapper *)window;
    struct ioctl_android_query query;
    DWORD size = sizeof( query );
    int ret;

    query.hdr.hwnd = HandleToLong( win->hwnd );
    query.what = what;
    ret = android_ioctl( IOCTL_QUERY, &query, sizeof(query), &query, &size );
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
    case NATIVE_WINDOW_UNLOCK_AND_POST:
    case NATIVE_WINDOW_CONNECT:
    case NATIVE_WINDOW_DISCONNECT:
        TRACE( "hwnd %p %s\n", win->hwnd, names[operation] );
        break;
    case NATIVE_WINDOW_SET_POST_TRANSFORM_CROP:
    default:
        FIXME( "unsupported perform hwnd %p op %d %s\n", win->hwnd, operation,
               operation < sizeof(names)/sizeof(names[0]) ? names[operation] : "???" );
        break;
    }
    va_end( args );
    return android_ioctl( IOCTL_PERFORM, &perf, sizeof(perf), NULL, NULL );
}

struct ANativeWindow *create_ioctl_window( HWND hwnd )
{
    struct ioctl_android_create_window req;
    struct native_win_wrapper *win = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*win) );
    HWND parent = GetAncestor( hwnd, GA_PARENT );

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
    TRACE( "-> %p %p\n", win, win->hwnd );

    req.hdr.hwnd = HandleToLong( hwnd );
    req.parent = parent == GetDesktopWindow() ? 0 : HandleToLong( parent );
    android_ioctl( IOCTL_CREATE_WINDOW, &req, sizeof(req), NULL, NULL );

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

    if (InterlockedDecrement( &win->ref ) > 0) return;

    TRACE( "%p %p\n", win, win->hwnd );

    destroy_ioctl_window( win->hwnd );
    HeapFree( GetProcessHeap(), 0, win );
}

void destroy_ioctl_window( HWND hwnd )
{
    struct ioctl_android_destroy_window req;

    req.hdr.hwnd = HandleToLong( hwnd );
    android_ioctl( IOCTL_DESTROY_WINDOW, &req, sizeof(req), NULL, NULL );
}

int ioctl_window_pos_changed( HWND hwnd, const RECT *window_rect, const RECT *client_rect,
                              const RECT *visible_rect, UINT style, UINT flags, HWND after, HWND owner )
{
    struct ioctl_android_window_pos_changed req;

    req.hdr.hwnd     = HandleToLong( hwnd );
    req.window_rect  = *window_rect;
    req.client_rect  = *client_rect;
    req.visible_rect = *visible_rect;
    req.style        = style;
    req.flags        = flags;
    req.after        = HandleToLong( after );
    req.owner        = HandleToLong( owner );
    return android_ioctl( IOCTL_WINDOW_POS_CHANGED, &req, sizeof(req), NULL, NULL );
}
