/*
 * Window stations and desktops
 *
 * Copyright 2002 Alexandre Julliard
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

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "ntuser.h"
#include "ddk/wdm.h"
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winstation);
WINE_DECLARE_DEBUG_CHANNEL(win);


#define DESKTOP_ALL_ACCESS 0x01ff

struct shared_input_cache
{
    const shared_object_t *object;
    UINT64 id;
    DWORD tid;
};

struct session_thread_data
{
    const shared_object_t *shared_desktop;         /* thread desktop shared session cached object */
    const shared_object_t *shared_queue;           /* thread message queue shared session cached object */
    struct shared_input_cache shared_input;        /* current thread input shared session cached object */
    struct shared_input_cache shared_foreground;   /* foreground thread input shared session cached object */
    struct shared_input_cache other_thread_input;  /* other thread input shared session cached object */
};

struct session_block
{
    struct list entry;      /* entry in the session block list */
    const char *data;       /* base pointer for the mmaped data */
    SIZE_T      offset;     /* offset of data in the session shared mapping */
    SIZE_T      size;       /* size of the mmaped data */
};

static pthread_mutex_t session_lock = PTHREAD_MUTEX_INITIALIZER;
static struct list session_blocks = LIST_INIT(session_blocks);
const session_shm_t *shared_session;

static struct session_thread_data *get_session_thread_data(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    if (!thread_info->session_data) thread_info->session_data = calloc(1, sizeof(*thread_info->session_data));
    return thread_info->session_data;
}

static void shared_object_acquire_seqlock( const shared_object_t *object, UINT64 *seq )
{
    while ((*seq = ReadNoFence64( &object->seq )) & 1) YieldProcessor();
    __SHARED_READ_FENCE;
}

static BOOL shared_object_release_seqlock( const shared_object_t *object, UINT64 seq )
{
    __SHARED_READ_FENCE;
    return ReadNoFence64( &object->seq ) == seq;
}

static object_id_t shared_object_get_id( const shared_object_t *object )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    do
    {
        shared_object_acquire_seqlock( object, &lock.seq );
        lock.id = object->id;
    } while (!shared_object_release_seqlock( object, lock.seq ));
    return lock.id;
}

static NTSTATUS map_shared_session_block( SIZE_T offset, SIZE_T size, struct session_block **ret )
{
    static const WCHAR nameW[] =
    {
        '\\','K','e','r','n','e','l','O','b','j','e','c','t','s','\\',
        '_','_','w','i','n','e','_','s','e','s','s','i','o','n',0
    };
    UNICODE_STRING name = RTL_CONSTANT_STRING( nameW );
    LARGE_INTEGER off = {.QuadPart = offset - (offset % system_info.AllocationGranularity)};
    struct session_block *block;
    OBJECT_ATTRIBUTES attr;
    unsigned int status;
    HANDLE handle;

    assert( offset + size > offset );

    if (!(block = calloc( 1, sizeof(*block) ))) return STATUS_NO_MEMORY;

    InitializeObjectAttributes( &attr, &name, 0, NULL, NULL );
    if ((status = NtOpenSection( &handle, SECTION_MAP_READ, &attr )))
        WARN( "Failed to open shared session section, status %#x\n", status );
    else
    {
        if ((status = NtMapViewOfSection( handle, GetCurrentProcess(), (void **)&block->data, 0, 0,
                                          &off, &block->size, ViewUnmap, 0, PAGE_READONLY )))
            WARN( "Failed to map shared session block, status %#x\n", status );
        else
        {
            list_add_tail( &session_blocks, &block->entry );
            block->offset = off.QuadPart;
            assert( block->offset + block->size > block->offset );
        }
        NtClose( handle );
    }

    if (status) free( block );
    else *ret = block;
    return status;
}

static NTSTATUS find_shared_session_block( SIZE_T offset, SIZE_T size, struct session_block **ret )
{
    struct session_block *block;
    UINT status;

    assert( offset + size > offset );

    pthread_mutex_lock( &session_lock );

    LIST_FOR_EACH_ENTRY( block, &session_blocks, struct session_block, entry )
    {
        if (block->offset < offset && offset + size <= block->offset + block->size)
        {
            *ret = block;
            pthread_mutex_unlock( &session_lock );
            return STATUS_SUCCESS;
        }
    }

    if ((status = map_shared_session_block( offset, size, ret )))
    {
        WARN( "Failed to map session block for offset %s, size %s, status %#x\n",
            wine_dbgstr_longlong(offset), wine_dbgstr_longlong(size), status );
    }

    pthread_mutex_unlock( &session_lock );

    return status;
}

static const shared_object_t *find_shared_session_object( struct obj_locator locator )
{
    struct session_block *block = NULL;
    const shared_object_t *object;
    NTSTATUS status;

    if (locator.id && !(status = find_shared_session_block( locator.offset, sizeof(*object), &block )))
    {
        object = (const shared_object_t *)(block->data + locator.offset - block->offset);
        if (locator.id == shared_object_get_id( object )) return object;
        WARN( "Session object id doesn't match expected id %s\n", wine_dbgstr_longlong(locator.id) );
    }

    return NULL;
}

void shared_session_init(void)
{
    struct session_block *block;
    UINT status;

    if ((status = find_shared_session_block( 0, sizeof(*shared_session), &block )))
        ERR( "Failed to map initial shared session block, status %#x\n", status );
    else
        shared_session = (const session_shm_t *)block->data;
}

NTSTATUS get_shared_desktop( struct object_lock *lock, const desktop_shm_t **desktop_shm )
{
    struct session_thread_data *data = get_session_thread_data();
    const shared_object_t *object;

    TRACE( "lock %p, desktop_shm %p\n", lock, desktop_shm );

    if (!(object = data->shared_desktop))
    {
        struct obj_locator locator;

        SERVER_START_REQ( get_thread_desktop )
        {
            req->tid = GetCurrentThreadId();
            wine_server_call( req );
            locator = reply->locator;
        }
        SERVER_END_REQ;

        data->shared_desktop = find_shared_session_object( locator );
        if (!(object = data->shared_desktop)) return STATUS_INVALID_HANDLE;
        memset( lock, 0, sizeof(*lock) );
    }

    if (!lock->id || !shared_object_release_seqlock( object, lock->seq ))
    {
        shared_object_acquire_seqlock( object, &lock->seq );
        *desktop_shm = &object->shm.desktop;
        lock->id = object->id;
        return STATUS_PENDING;
    }

    return STATUS_SUCCESS;
}

NTSTATUS get_shared_queue( struct object_lock *lock, const queue_shm_t **queue_shm )
{
    struct session_thread_data *data = get_session_thread_data();
    const shared_object_t *object;

    TRACE( "lock %p, queue_shm %p\n", lock, queue_shm );

    if (!(object = data->shared_queue))
    {
        struct obj_locator locator;

        SERVER_START_REQ( get_msg_queue )
        {
            wine_server_call( req );
            locator = reply->locator;
        }
        SERVER_END_REQ;

        data->shared_queue = find_shared_session_object( locator );
        if (!(object = data->shared_queue)) return STATUS_INVALID_HANDLE;
        memset( lock, 0, sizeof(*lock) );
    }

    if (!lock->id || !shared_object_release_seqlock( object, lock->seq ))
    {
        shared_object_acquire_seqlock( object, &lock->seq );
        *queue_shm = &object->shm.queue;
        lock->id = object->id;
        return STATUS_PENDING;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS try_get_shared_input( UINT tid, struct object_lock *lock, const input_shm_t **input_shm,
                                      struct shared_input_cache *cache )
{
    const shared_object_t *object;
    BOOL valid = TRUE;

    if (!(object = cache->object))
    {
        struct obj_locator locator;

        SERVER_START_REQ( get_thread_input )
        {
            req->tid = tid;
            wine_server_call( req );
            locator = reply->locator;
        }
        SERVER_END_REQ;

        cache->id = locator.id;
        cache->object = find_shared_session_object( locator );
        if (!(object = cache->object)) return STATUS_INVALID_HANDLE;
        memset( lock, 0, sizeof(*lock) );
    }

    /* check object validity by comparing ids, within the object seqlock */
    if ((valid = cache->id == object->id) && !tid)
    {
        /* check that a previously locked foreground thread input is still foreground */
        valid = !!object->shm.input.foreground;
    }

    if (!lock->id || !shared_object_release_seqlock( object, lock->seq ))
    {
        shared_object_acquire_seqlock( object, &lock->seq );
        if (!(lock->id = object->id)) lock->id = -1;
        *input_shm = &object->shm.input;
        return STATUS_PENDING;
    }

    if (!valid) memset( cache, 0, sizeof(*cache) ); /* object has been invalidated, clear the cache and start over */
    return STATUS_SUCCESS;
}

NTSTATUS get_shared_input( UINT tid, struct object_lock *lock, const input_shm_t **input_shm )
{
    struct session_thread_data *data = get_session_thread_data();
    struct shared_input_cache *cache;
    UINT status;

    TRACE( "tid %u, lock %p, input_shm %p\n", tid, lock, input_shm );

    if (tid == GetCurrentThreadId()) cache = &data->shared_input;
    else if (!tid) cache = &data->shared_foreground;
    else cache = &data->other_thread_input;

    if (tid != cache->tid) memset( cache, 0, sizeof(*cache) );
    cache->tid = tid;

    do { status = try_get_shared_input( tid, lock, input_shm, cache ); }
    while (!status && !cache->id);

    return status;
}

BOOL is_virtual_desktop(void)
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const desktop_shm_t *desktop_shm;
    BOOL ret = FALSE;
    UINT status;

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
        ret = !!(desktop_shm->flags & DF_WINE_VIRTUAL_DESKTOP);
    if (status) ret = FALSE;

    return ret;
}

BOOL is_service_process(void)
{
    static const WCHAR wine_service_station_name[] = {'_','_','w','i','n','e','s','e','r','v','i','c','e','_','w','i','n','s','t','a','t','i','o','n',0};
    WCHAR name[MAX_PATH];

    return NtUserGetObjectInformation( NtUserGetProcessWindowStation(), UOI_NAME, name, sizeof(name), NULL ) &&
           !wcscmp( name, wine_service_station_name );
}

/***********************************************************************
 *           NtUserCreateWindowStation  (win32u.@)
 */
HWINSTA WINAPI NtUserCreateWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access, ULONG arg3,
                                          ULONG arg4, ULONG arg5, ULONG arg6, ULONG arg7 )
{
    HANDLE ret;

    if (attr->ObjectName->Length >= MAX_PATH * sizeof(WCHAR))
    {
        RtlSetLastWin32Error( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

    SERVER_START_REQ( create_winstation )
    {
        req->flags      = 0;
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        wine_server_call_err( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *           NtUserOpenWindowStation  (win32u.@)
 */
HWINSTA WINAPI NtUserOpenWindowStation( OBJECT_ATTRIBUTES *attr, ACCESS_MASK access )
{
    HANDLE ret = 0;

    SERVER_START_REQ( open_winstation )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserCloseWindowStation  (win32u.@)
 */
BOOL WINAPI NtUserCloseWindowStation( HWINSTA handle )
{
    BOOL ret;
    SERVER_START_REQ( close_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUSerGetProcessWindowStation  (win32u.@)
 */
HWINSTA WINAPI NtUserGetProcessWindowStation(void)
{
    HWINSTA ret = 0;

    SERVER_START_REQ( get_process_winstation )
    {
        if (!wine_server_call_err( req ))
            ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserSetProcessWindowStation  (win32u.@)
 */
BOOL WINAPI NtUserSetProcessWindowStation( HWINSTA handle )
{
    BOOL ret;

    SERVER_START_REQ( set_process_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    reset_monitor_update_serial();
    return ret;
}

/***********************************************************************
 *           NtUserCreateDesktopEx   (win32u.@)
 */
HDESK WINAPI NtUserCreateDesktopEx( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *device,
                                    DEVMODEW *devmode, DWORD flags, ACCESS_MASK access,
                                    ULONG heap_size )
{
    WCHAR buffer[MAX_PATH];
    HANDLE ret;

    if ((device && device->Length) || (devmode && !(flags & DF_WINE_VIRTUAL_DESKTOP)))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (attr->ObjectName->Length >= MAX_PATH * sizeof(WCHAR))
    {
        RtlSetLastWin32Error( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( create_desktop )
    {
        req->flags      = flags;
        req->access     = access;
        req->attributes = attr->Attributes;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        wine_server_call_err( req );
        ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    if (!devmode) return ret;

    lstrcpynW( buffer, attr->ObjectName->Buffer, attr->ObjectName->Length / sizeof(WCHAR) + 1 );
    if (!user_driver->pCreateDesktop( buffer, devmode->dmPelsWidth, devmode->dmPelsHeight ))
    {
        NtUserCloseDesktop( ret );
        return 0;
    }

    /* force update display cache to use virtual desktop display settings */
    if (flags & DF_WINE_VIRTUAL_DESKTOP) update_display_cache( TRUE );
    return ret;
}

/***********************************************************************
 *           NtUserOpenDesktop   (win32u.@)
 */
HDESK WINAPI NtUserOpenDesktop( OBJECT_ATTRIBUTES *attr, DWORD flags, ACCESS_MASK access )
{
    HANDLE ret = 0;

    access |= DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS;

    if (attr->ObjectName->Length >= MAX_PATH * sizeof(WCHAR))
    {
        RtlSetLastWin32Error( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }

    SERVER_START_REQ( open_desktop )
    {
        req->winsta     = wine_server_obj_handle( attr->RootDirectory );
        req->flags      = flags;
        req->access     = access;
        req->attributes = attr->Attributes;
        wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
 }

/***********************************************************************
 *           NtUserCloseDesktop  (win32u.@)
 */
BOOL WINAPI NtUserCloseDesktop( HDESK handle )
{
    BOOL ret;
    SERVER_START_REQ( close_desktop )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserGetThreadDesktop   (win32u.@)
 */
HDESK WINAPI NtUserGetThreadDesktop( DWORD thread )
{
    HDESK ret = 0;

    SERVER_START_REQ( get_thread_desktop )
    {
        req->tid = thread;
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           NtUserSetThreadDesktop   (win32u.@)
 */
BOOL WINAPI NtUserSetThreadDesktop( HDESK handle )
{
    BOOL ret, was_virtual_desktop = is_virtual_desktop();
    struct obj_locator locator;

    SERVER_START_REQ( set_thread_desktop )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = !wine_server_call_err( req );
        locator = reply->locator;
    }
    SERVER_END_REQ;

    if (ret)  /* reset the desktop windows */
    {
        struct user_thread_info *thread_info = get_user_thread_info();
        struct session_thread_data *data = get_session_thread_data();
        data->shared_desktop = find_shared_session_object( locator );
        memset( &data->shared_foreground, 0, sizeof(data->shared_foreground) );
        thread_info->client_info.top_window = 0;
        thread_info->client_info.msg_window = 0;
        if (was_virtual_desktop != is_virtual_desktop()) update_display_cache( TRUE );
    }
    return ret;
}

/***********************************************************************
 *           NtUserOpenInputDesktop   (win32u.@)
 */
HDESK WINAPI NtUserOpenInputDesktop( DWORD flags, BOOL inherit, ACCESS_MASK access )
{
    HANDLE ret = 0;

    TRACE( "(%x,%i,%x)\n", flags, inherit, access );

    access |= DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS;

    if (flags)
        FIXME( "partial stub flags %08x\n", flags );

    SERVER_START_REQ( open_input_desktop )
    {
        req->flags      = flags;
        req->access     = access;
        req->attributes = inherit ? OBJ_INHERIT : 0;
        if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    return ret;
}

BOOL WINAPI NtUserSwitchDesktop( HDESK desktop )
{
    TRACE( "desktop %p\n", desktop );

    SERVER_START_REQ( set_input_desktop )
    {
        req->handle = wine_server_obj_handle( desktop );
        if (wine_server_call_err( req )) return FALSE;
    }
    SERVER_END_REQ;

    return TRUE;
}

/******************************************************************************
 *              NtUserBuildNameList   (win32u.@)
 */
NTSTATUS WINAPI NtUserBuildNameList( HWINSTA handle, ULONG size, struct ntuser_name_list *list,
                                     ULONG *ret_size )
{
    const ULONG header_size = offsetof( struct ntuser_name_list, strings[1] ); /* header + final null */
    WCHAR *buffer;
    NTSTATUS status;
    ULONG count, result, total;

    if (size <= header_size) return STATUS_INVALID_HANDLE;
    if (!(buffer = malloc( size - header_size ))) return STATUS_NO_MEMORY;

    SERVER_START_REQ( enum_winstation )
    {
        req->handle = wine_server_obj_handle( handle );
        wine_server_set_reply( req, buffer, size - header_size );
        status = wine_server_call( req );
        result = wine_server_reply_size( reply );
        total  = reply->total;
        count  = reply->count;
    }
    SERVER_END_REQ;

    if (!status || status == STATUS_BUFFER_TOO_SMALL)
    {
        list->size = header_size + result;
        list->count = count;
        memcpy( list->strings, buffer, result );
        list->strings[result / sizeof(WCHAR)] = 0;
        *ret_size = header_size + total;
    }
    return status;
}


/***********************************************************************
 *           NtUserGetObjectInformation   (win32u.@)
 */
BOOL WINAPI NtUserGetObjectInformation( HANDLE handle, INT index, void *info,
                                        DWORD len, DWORD *needed )
{
    BOOL ret;

    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
    static const WCHAR window_stationW[] = {'W','i','n','d','o','w','S','t','a','t','i','o','n',0};

    switch(index)
    {
    case UOI_FLAGS:
        {
            USEROBJECTFLAGS *obj_flags = info;
            if (needed) *needed = sizeof(*obj_flags);
            if (len < sizeof(*obj_flags))
            {
                RtlSetLastWin32Error( ERROR_BUFFER_OVERFLOW );
                return FALSE;
            }
            SERVER_START_REQ( set_user_object_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = 0;
                ret = !wine_server_call_err( req );
                if (ret)
                {
                    /* FIXME: inherit flag */
                    obj_flags->dwFlags = reply->old_obj_flags;
                }
            }
            SERVER_END_REQ;
        }
        return ret;

    case UOI_TYPE:
        SERVER_START_REQ( set_user_object_info )
        {
            req->handle = wine_server_obj_handle( handle );
            req->flags  = 0;
            ret = !wine_server_call_err( req );
            if (ret)
            {
                size_t size = reply->is_desktop ? sizeof(desktopW) : sizeof(window_stationW);
                if (needed) *needed = size;
                if (len < size)
                {
                    RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
                    ret = FALSE;
                }
                else memcpy( info, reply->is_desktop ? desktopW : window_stationW, size );
            }
        }
        SERVER_END_REQ;
        return ret;

    case UOI_NAME:
        {
            WCHAR buffer[MAX_PATH];
            SERVER_START_REQ( set_user_object_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->flags  = 0;
                wine_server_set_reply( req, buffer, sizeof(buffer) - sizeof(WCHAR) );
                ret = !wine_server_call_err( req );
                if (ret)
                {
                    size_t size = wine_server_reply_size( reply );
                    buffer[size / sizeof(WCHAR)] = 0;
                    size += sizeof(WCHAR);
                    if (needed) *needed = size;
                    if (len < size)
                    {
                        RtlSetLastWin32Error( ERROR_INSUFFICIENT_BUFFER );
                        ret = FALSE;
                    }
                    else memcpy( info, buffer, size );
                }
            }
            SERVER_END_REQ;
        }
        return ret;

    case UOI_USER_SID:
        FIXME( "not supported index %d\n", index );
        /* fall through */
    default:
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}

/***********************************************************************
 *           NtUserSetObjectInformation   (win32u.@)
 */
BOOL WINAPI NtUserSetObjectInformation( HANDLE handle, INT index, void *info, DWORD len )
{
    BOOL ret;
    const USEROBJECTFLAGS *obj_flags = info;

    if (index != UOI_FLAGS || !info || len < sizeof(*obj_flags))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    /* FIXME: inherit flag */
    SERVER_START_REQ( set_user_object_info )
    {
        req->handle    = wine_server_obj_handle( handle );
        req->flags     = SET_USER_OBJECT_SET_FLAGS;
        req->obj_flags = obj_flags->dwFlags;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

#ifdef _WIN64
static inline TEB64 *NtCurrentTeb64(void) { return NULL; }
#else
static inline TEB64 *NtCurrentTeb64(void) { return (TEB64 *)NtCurrentTeb()->GdiBatchCount; }
#endif

HWND get_desktop_window(void)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    BOOL is_service;

    if (thread_info->top_window) return UlongToHandle( thread_info->top_window );

    /* don't create an actual explorer desktop window for services */
    is_service = is_service_process();

    SERVER_START_REQ( get_desktop_window )
    {
        req->force = is_service;
        if (!wine_server_call( req ))
        {
            thread_info->top_window = reply->top_window;
            thread_info->msg_window = reply->msg_window;
        }
    }
    SERVER_END_REQ;

    if (!thread_info->top_window)
    {
        static const WCHAR appnameW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s',
            '\\','s','y','s','t','e','m','3','2','\\','e','x','p','l','o','r','e','r','.','e','x','e',0};
        static const WCHAR cmdlineW[] = {'"','C',':','\\','w','i','n','d','o','w','s','\\',
            's','y','s','t','e','m','3','2','\\','e','x','p','l','o','r','e','r','.','e','x','e','"',
            ' ','/','d','e','s','k','t','o','p',0};
        static const WCHAR system_dir[] = {'C',':','\\','w','i','n','d','o','w','s','\\',
            's','y','s','t','e','m','3','2','\\',0};
        RTL_USER_PROCESS_PARAMETERS params = { sizeof(params), sizeof(params) };
        PS_ATTRIBUTE_LIST ps_attr;
        PS_CREATE_INFO create_info;
        WCHAR desktop[MAX_PATH];
        PEB *peb = NtCurrentTeb()->Peb;
        HANDLE process, thread;
        unsigned int status;

        SERVER_START_REQ( set_user_object_info )
        {
            req->handle = wine_server_obj_handle( NtUserGetThreadDesktop(GetCurrentThreadId()) );
            req->flags  = SET_USER_OBJECT_GET_FULL_NAME;
            wine_server_set_reply( req, desktop, sizeof(desktop) - sizeof(WCHAR) );
            if (!wine_server_call( req ))
            {
                size_t size = wine_server_reply_size( reply );
                desktop[size / sizeof(WCHAR)] = 0;
                TRACE( "starting explorer for desktop %s\n", debugstr_w(desktop) );
            }
            else
                desktop[0] = 0;
        }
        SERVER_END_REQ;

        params.Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
        params.Environment     = peb->ProcessParameters->Environment;
        params.EnvironmentSize = peb->ProcessParameters->EnvironmentSize;
        params.hStdError       = peb->ProcessParameters->hStdError;
        RtlInitUnicodeString( &params.CurrentDirectory.DosPath, system_dir );
        RtlInitUnicodeString( &params.ImagePathName, appnameW + 4 );
        RtlInitUnicodeString( &params.CommandLine, cmdlineW );
        RtlInitUnicodeString( &params.WindowTitle, appnameW + 4 );
        RtlInitUnicodeString( &params.Desktop, desktop );

        ps_attr.TotalLength = sizeof(ps_attr);
        ps_attr.Attributes[0].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
        ps_attr.Attributes[0].Size         = sizeof(appnameW) - sizeof(WCHAR);
        ps_attr.Attributes[0].ValuePtr     = (WCHAR *)appnameW;
        ps_attr.Attributes[0].ReturnLength = NULL;

        if (NtCurrentTeb64() && !NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR])
        {
            NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = TRUE;
            status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                          NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, &params,
                                          &create_info, &ps_attr );
            NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = FALSE;
        }
        else
            status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                          NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, &params,
                                          &create_info, &ps_attr );
        if (!status)
        {
            NtResumeThread( thread, NULL );
            TRACE_(win)( "started explorer\n" );
            NtUserWaitForInputIdle( process, 10000, FALSE );
            NtClose( thread );
            NtClose( process );
        }
        else ERR_(win)( "failed to start explorer %x\n", status );

        SERVER_START_REQ( get_desktop_window )
        {
            req->force = 1;
            if (!wine_server_call( req ))
            {
                thread_info->top_window = reply->top_window;
                thread_info->msg_window = reply->msg_window;
            }
        }
        SERVER_END_REQ;
    }

    if (!thread_info->top_window) ERR_(win)( "failed to create desktop window\n" );
    else user_driver->pSetDesktopWindow( UlongToHandle( thread_info->top_window ));

    register_builtin_classes();
    return UlongToHandle( thread_info->top_window );
}

static HANDLE get_winstations_dir_handle(void)
{
    char bufferA[64];
    WCHAR buffer[64];
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE dir;

    snprintf( bufferA, sizeof(bufferA), "\\Sessions\\%u\\Windows\\WindowStations", NtCurrentTeb()->Peb->SessionId );
    str.Buffer = buffer;
    str.MaximumLength = asciiz_to_unicode( buffer, bufferA );
    str.Length = str.MaximumLength - sizeof(WCHAR);
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = NtOpenDirectoryObject( &dir, DIRECTORY_CREATE_OBJECT | DIRECTORY_TRAVERSE, &attr );
    return status ? 0 : dir;
}

/***********************************************************************
 *           get_default_desktop
 *
 * Get the name of the desktop to use for this app if not specified explicitly.
 */
static const WCHAR *get_default_desktop( void *buf, size_t buf_size )
{
    const WCHAR *p, *appname = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = buf;
    WCHAR *buffer = buf;
    HKEY tmpkey, appkey;
    DWORD len;

    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};

    if ((p = wcsrchr( appname, '/' ))) appname = p + 1;
    if ((p = wcsrchr( appname, '\\' ))) appname = p + 1;
    len = lstrlenW(appname);
    if (len > MAX_PATH) return defaultW;
    memcpy( buffer, appname, len * sizeof(WCHAR) );
    asciiz_to_unicode( buffer + len, "\\Explorer" );

    /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Explorer */
    if ((tmpkey = reg_open_hkcu_key( "Software\\Wine\\AppDefaults" )))
    {
        appkey = reg_open_key( tmpkey, buffer, lstrlenW(buffer) * sizeof(WCHAR) );
        NtClose( tmpkey );
        if (appkey)
        {
            len = query_reg_ascii_value( appkey, "Desktop", info, buf_size );
            NtClose( appkey );
            if (len) return (const WCHAR *)info->Data;
        }
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
    if ((appkey = reg_open_hkcu_key( "Software\\Wine\\Explorer" )))
    {
        len = query_reg_ascii_value( appkey, "Desktop", info, buf_size );
        NtClose( appkey );
        if (len) return (const WCHAR *)info->Data;
    }

    return defaultW;
}

/***********************************************************************
 *           winstation_init
 *
 * Connect to the process window station and desktop.
 */
void winstation_init(void)
{
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
    WCHAR *winstation = NULL, *desktop = NULL, *buffer = NULL;
    HANDLE handle, dir = NULL;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING str;

    static const WCHAR winsta0[] = {'W','i','n','S','t','a','0',0};

    if (params->Desktop.Length)
    {
        buffer = malloc( params->Desktop.Length + sizeof(WCHAR) );
        memcpy( buffer, params->Desktop.Buffer, params->Desktop.Length );
        buffer[params->Desktop.Length / sizeof(WCHAR)] = 0;
        if ((desktop = wcschr( buffer, '\\' )))
        {
            *desktop++ = 0;
            winstation = buffer;
        }
        else desktop = buffer;
    }

    /* set winstation if explicitly specified, or if we don't have one yet */
    if (buffer || !NtUserGetProcessWindowStation())
    {
        str.Buffer = (WCHAR *)(winstation ? winstation : winsta0);
        str.Length = str.MaximumLength = lstrlenW( str.Buffer ) * sizeof(WCHAR);
        dir = get_winstations_dir_handle();
        InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                    dir, NULL );

        handle = NtUserCreateWindowStation( &attr, STANDARD_RIGHTS_REQUIRED | WINSTA_ALL_ACCESS, 0, 0, 0, 0, 0 );
        if (handle)
        {
            NtUserSetProcessWindowStation( handle );
            /* only WinSta0 is visible */
            if (!winstation || !wcsicmp( winstation, winsta0 ))
            {
                USEROBJECTFLAGS flags;
                flags.fInherit  = FALSE;
                flags.fReserved = FALSE;
                flags.dwFlags   = WSF_VISIBLE;
                NtUserSetObjectInformation( handle, UOI_FLAGS, &flags, sizeof(flags) );
            }
        }
    }
    if (buffer || !NtUserGetThreadDesktop( GetCurrentThreadId() ))
    {
        char buffer[4096];
        str.Buffer = (WCHAR *)(desktop ? desktop : get_default_desktop( buffer, sizeof(buffer) ));
        str.Length = str.MaximumLength = lstrlenW( str.Buffer ) * sizeof(WCHAR);
        if (!dir) dir = get_winstations_dir_handle();
        InitializeObjectAttributes( &attr, &str, OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                    dir, NULL );

        handle = NtUserCreateDesktopEx( &attr, NULL, NULL, 0, STANDARD_RIGHTS_REQUIRED | DESKTOP_ALL_ACCESS, 0 );
        if (handle) NtUserSetThreadDesktop( handle );
    }
    NtClose( dir );
    free( buffer );
}
