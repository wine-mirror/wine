/*
 * WIN32 clipboard implementation
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Alex Korobka
 * Copyright 1999 Noel Borthwick
 * Copyright 2003 Ulrich Czekalla for CodeWeavers
 * Copyright 2016 Alexandre Julliard
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
 *
 */

#if 0
#pragma makedep unix
#endif

#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

static pthread_mutex_t clipboard_mutex = PTHREAD_MUTEX_INITIALIZER;

struct cached_format
{
    struct list entry;       /* entry in cache list */
    UINT        format;      /* format id */
    UINT        seqno;       /* sequence number when the data was set */
    HANDLE      handle;      /* original data handle */
};

static struct list cached_formats = LIST_INIT( cached_formats );
static struct list formats_to_free = LIST_INIT( formats_to_free );


/* get a debug string for a format id */
static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];
    DWORD le = RtlGetLastWin32Error();
    BOOL r = NtUserGetClipboardFormatName( id, buffer, ARRAYSIZE(buffer) );
    RtlSetLastWin32Error(le);

    if (r)
        return wine_dbg_sprintf( "%04x %s", id, debugstr_w(buffer) );

    switch (id)
    {
#define BUILTIN(id) case id: return #id;
    BUILTIN(CF_TEXT)
    BUILTIN(CF_BITMAP)
    BUILTIN(CF_METAFILEPICT)
    BUILTIN(CF_SYLK)
    BUILTIN(CF_DIF)
    BUILTIN(CF_TIFF)
    BUILTIN(CF_OEMTEXT)
    BUILTIN(CF_DIB)
    BUILTIN(CF_PALETTE)
    BUILTIN(CF_PENDATA)
    BUILTIN(CF_RIFF)
    BUILTIN(CF_WAVE)
    BUILTIN(CF_UNICODETEXT)
    BUILTIN(CF_ENHMETAFILE)
    BUILTIN(CF_HDROP)
    BUILTIN(CF_LOCALE)
    BUILTIN(CF_DIBV5)
    BUILTIN(CF_OWNERDISPLAY)
    BUILTIN(CF_DSPTEXT)
    BUILTIN(CF_DSPBITMAP)
    BUILTIN(CF_DSPMETAFILEPICT)
    BUILTIN(CF_DSPENHMETAFILE)
#undef BUILTIN
    default: return wine_dbg_sprintf( "%04x", id );
    }
}

/* retrieve a data format from the cache */
static struct cached_format *get_cached_format( UINT format )
{
    struct cached_format *cache;

    LIST_FOR_EACH_ENTRY( cache, &cached_formats, struct cached_format, entry )
        if (cache->format == format) return cache;
    return NULL;
}

/* free a single cached format */
static void free_cached_data( struct cached_format *cache )
{
    struct free_cached_data_params params;
    void *ret_ptr;
    ULONG ret_len;

    switch (cache->format)
    {
    case CF_BITMAP:
    case CF_PALETTE:
        make_gdi_object_system( cache->handle, FALSE );
    case CF_DSPBITMAP:
        NtGdiDeleteObjectApp( cache->handle );
        break;

    default:
        params.format = cache->format;
        params.handle = cache->handle;
        KeUserModeCallback( NtUserFreeCachedClipboardData, &params, sizeof(params),
                            &ret_ptr, &ret_len );
        break;
    }
    free( cache );
}

/* free all the data in the cache */
static void free_cached_formats( struct list *list )
{
    struct list *ptr;

    while ((ptr = list_head( list )))
    {
        list_remove( ptr );
        free_cached_data( LIST_ENTRY( ptr, struct cached_format, entry ));
    }
}

/* clear global memory formats; special types are freed on EmptyClipboard */
static void invalidate_memory_formats( struct list *free_list )
{
    struct cached_format *cache, *next;

    LIST_FOR_EACH_ENTRY_SAFE( cache, next, &cached_formats, struct cached_format, entry )
    {
        switch (cache->format)
        {
        case CF_BITMAP:
        case CF_DSPBITMAP:
        case CF_PALETTE:
        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            continue;
        default:
            list_remove( &cache->entry );
            list_add_tail( free_list, &cache->entry );
            break;
        }
    }
}

/**************************************************************************
 *           NtUserOpenClipboard    (win32u.@)
 */
BOOL WINAPI NtUserOpenClipboard( HWND hwnd, ULONG unk )
{
    struct list free_list = LIST_INIT( free_list );
    BOOL ret;
    HWND owner;

    TRACE( "%p\n", hwnd );

    user_driver->pUpdateClipboard();

    pthread_mutex_lock( &clipboard_mutex );

    SERVER_START_REQ( open_clipboard )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
        owner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    if (ret && !is_current_process_window( owner )) invalidate_memory_formats( &free_list );

    pthread_mutex_unlock( &clipboard_mutex );
    free_cached_formats( &free_list );
    return ret;
}

/**************************************************************************
 *           NtUserCloseClipboard    (win32u.@)
 */
BOOL WINAPI NtUserCloseClipboard(void)
{
    HWND viewer = 0, owner = 0;
    BOOL ret;

    TRACE( "\n" );

    SERVER_START_REQ( close_clipboard )
    {
        if ((ret = !wine_server_call_err( req )))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (viewer) NtUserMessageCall( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                                   0, NtUserSendNotifyMessage, FALSE );
    return ret;
}

/**************************************************************************
 *           NtUserEmptyClipboard    (win32u.@)
 */
BOOL WINAPI NtUserEmptyClipboard(void)
{
    BOOL ret;
    HWND owner = NtUserGetClipboardOwner();
    struct list free_list = LIST_INIT( free_list );

    TRACE( "owner %p\n", owner );

    if (owner)
        send_message_timeout( owner, WM_DESTROYCLIPBOARD, 0, 0, SMTO_ABORTIFHUNG, 5000, FALSE );

    pthread_mutex_lock( &clipboard_mutex );

    SERVER_START_REQ( empty_clipboard )
    {
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (ret)
    {
        list_move_tail( &free_list, &formats_to_free );
        list_move_tail( &free_list, &cached_formats );
    }

    pthread_mutex_unlock( &clipboard_mutex );
    free_cached_formats( &free_list );
    return ret;
}

/**************************************************************************
 *           NtUserCountClipboardFormats    (win32u.@)
 */
INT WINAPI NtUserCountClipboardFormats(void)
{
    INT count = 0;

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        wine_server_call( req );
        count = reply->count;
    }
    SERVER_END_REQ;

    TRACE( "returning %d\n", count );
    return count;
}

/**************************************************************************
 *	     NtUserIsClipboardFormatAvailable    (win32u.@)
 */
BOOL WINAPI NtUserIsClipboardFormatAvailable( UINT format )
{
    BOOL ret = FALSE;

    if (!format) return FALSE;

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        req->format = format;
        if (!wine_server_call_err( req )) ret = (reply->count > 0);
    }
    SERVER_END_REQ;
    TRACE( "%s -> %u\n", debugstr_format( format ), ret );
    return ret;
}

/**************************************************************************
 *	     NtUserGetUpdatedClipboardFormats    (win32u.@)
 */
BOOL WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    BOOL ret;

    if (!out_size)
    {
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        if (formats) wine_server_set_reply( req, formats, size * sizeof(*formats) );
        ret = !wine_server_call_err( req );
        *out_size = reply->count;
    }
    SERVER_END_REQ;

    TRACE( "%p %u returning %u formats, ret %u\n", formats, size, *out_size, ret );
    if (!ret && !formats && *out_size) RtlSetLastWin32Error( ERROR_NOACCESS );
    return ret;
}

/**************************************************************************
 *	     NtUserGetPriorityClipboardFormat    (win32u.@)
 */
INT WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count )
{
    int i;

    TRACE( "%p %u\n", list, count );

    if (NtUserCountClipboardFormats() == 0)
        return 0;

    for (i = 0; i < count; i++)
        if (NtUserIsClipboardFormatAvailable( list[i] ))
            return list[i];

    return -1;
}

/**************************************************************************
 *	     NtUserGetClipboardFormatName    (win32u.@)
 */
INT WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen )
{
    char buf[sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION *abi = (ATOM_BASIC_INFORMATION *)buf;
    UINT length = 0;

    if (format < MAXINTATOM || format > 0xffff) return 0;
    if (maxlen <= 0)
    {
        RtlSetLastWin32Error( ERROR_MORE_DATA );
        return 0;
    }
    if (!set_ntstatus( NtQueryInformationAtom( format, AtomBasicInformation,
                                               buf, sizeof(buf), NULL )))
        return 0;

    length = min( abi->NameLength / sizeof(WCHAR), maxlen - 1 );
    if (length) memcpy( buffer, abi->Name, length * sizeof(WCHAR) );
    buffer[length] = 0;
    return length;
}

/**************************************************************************
 *	     NtUserGetClipboardOwner    (win32u.@)
 */
HWND WINAPI NtUserGetClipboardOwner(void)
{
    HWND owner = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) owner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", owner );
    return owner;
}

/**************************************************************************
 *           NtUserSetClipboardViewer    (win32u.@)
 */
HWND WINAPI NtUserSetClipboardViewer( HWND hwnd )
{
    HWND prev = 0, owner = 0;

    SERVER_START_REQ( set_clipboard_viewer )
    {
        req->viewer = wine_server_user_handle( hwnd );
        if (!wine_server_call_err( req ))
        {
            prev = wine_server_ptr_handle( reply->old_viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (hwnd)
        NtUserMessageCall( hwnd, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                           NULL, NtUserSendNotifyMessage, FALSE );

    TRACE( "%p returning %p\n", hwnd, prev );
    return prev;
}

/**************************************************************************
 *           NtUserGetClipboardViewer    (win32u.@)
 */
HWND WINAPI NtUserGetClipboardViewer(void)
{
    HWND viewer = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) viewer = wine_server_ptr_handle( reply->viewer );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", viewer );
    return viewer;
}

/**************************************************************************
 *           NtUserChangeClipboardChain    (win32u.@)
 */
BOOL WINAPI NtUserChangeClipboardChain( HWND hwnd, HWND next )
{
    NTSTATUS status;
    HWND viewer;

    if (!hwnd) return FALSE;

    SERVER_START_REQ( set_clipboard_viewer )
    {
        req->viewer = wine_server_user_handle( next );
        req->previous = wine_server_user_handle( hwnd );
        status = wine_server_call( req );
        viewer = wine_server_ptr_handle( reply->old_viewer );
    }
    SERVER_END_REQ;

    if (status == STATUS_PENDING)
        return !send_message( viewer, WM_CHANGECBCHAIN, (WPARAM)hwnd, (LPARAM)next );

    if (status) RtlSetLastWin32Error( RtlNtStatusToDosError( status ));
    return !status;
}

/**************************************************************************
 *	     NtUserGetOpenClipboardWindow    (win32u.@)
 */
HWND WINAPI NtUserGetOpenClipboardWindow(void)
{
    HWND window = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) window = wine_server_ptr_handle( reply->window );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", window );
    return window;
}

/**************************************************************************
 *	     NtUserGetClipboardSequenceNumber    (win32u.@)
 */
DWORD WINAPI NtUserGetClipboardSequenceNumber(void)
{
    unsigned int seqno = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) seqno = reply->seqno;
    }
    SERVER_END_REQ;

    TRACE( "returning %u\n", seqno );
    return seqno;
}

/**************************************************************************
 *	     NtUserEnumClipboardFormats    (win32u.@)
 */
UINT WINAPI NtUserEnumClipboardFormats( UINT format )
{
    UINT ret = 0;

    SERVER_START_REQ( enum_clipboard_formats )
    {
        req->previous = format;
        if (!wine_server_call_err( req ))
        {
            ret = reply->format;
            RtlSetLastWin32Error( ERROR_SUCCESS );
        }
    }
    SERVER_END_REQ;

    TRACE( "%s -> %s\n", debugstr_format( format ), debugstr_format( ret ));
    return ret;
}

/**************************************************************************
 *	     NtUserAddClipboardFormatListener    (win32u.@)
 */
BOOL WINAPI NtUserAddClipboardFormatListener( HWND hwnd )
{
    BOOL ret;

    SERVER_START_REQ( add_clipboard_listener )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/**************************************************************************
 *	     NtUserRemoveClipboardFormatListener    (win32u.@)
 */
BOOL WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd )
{
    BOOL ret;

    SERVER_START_REQ( remove_clipboard_listener )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/**************************************************************************
 *           release_clipboard_owner
 */
void release_clipboard_owner( HWND hwnd )
{
    HWND viewer = 0, owner = 0;

    send_message( hwnd, WM_RENDERALLFORMATS, 0, 0 );

    SERVER_START_REQ( release_clipboard )
    {
        req->owner = wine_server_user_handle( hwnd );
        if (!wine_server_call( req ))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (viewer)
        NtUserMessageCall( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                           0, NtUserSendNotifyMessage, FALSE );
}

/**************************************************************************
 *           NtUserSetClipboardData    (win32u.@)
 */
NTSTATUS WINAPI NtUserSetClipboardData( UINT format, HANDLE data, struct set_clipboard_params *params )
{
    struct cached_format *cache = NULL, *prev = NULL;
    LCID lcid;
    void *ptr = NULL;
    data_size_t size = 0;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%s %p\n", debugstr_format( format ), data );

    if (params->cache_only)
    {
        pthread_mutex_lock( &clipboard_mutex );
        if ((cache = get_cached_format( format )) && cache->seqno == params->seqno)
            cache->handle = data;
        else
            status = STATUS_UNSUCCESSFUL;
        pthread_mutex_unlock( &clipboard_mutex );
        return status;
    }

    if (params->data)
    {
        ptr = params->data;
        size = params->size;
        if (data)
        {
            if (!(cache = malloc( sizeof(*cache) ))) goto done;
            cache->format = format;
            cache->handle = data;
        }

        if (format == CF_BITMAP || format == CF_PALETTE)
        {
            make_gdi_object_system( cache->handle, TRUE );
        }
    }
    NtQueryDefaultLocale( TRUE, &lcid );

    pthread_mutex_lock( &clipboard_mutex );

    SERVER_START_REQ( set_clipboard_data )
    {
        req->format = format;
        req->lcid = lcid;
        wine_server_add_data( req, ptr, size );
        if (!(status = wine_server_call( req )))
        {
            if (cache) cache->seqno = reply->seqno;
        }
    }
    SERVER_END_REQ;

    if (!status)
    {
        /* free the previous entry if any */
        if ((prev = get_cached_format( format ))) list_remove( &prev->entry );
        if (cache) list_add_tail( &cached_formats, &cache->entry );
    }
    else free( cache );

    pthread_mutex_unlock( &clipboard_mutex );
    if (prev) free_cached_data( prev );

done:
    return status;
}

/**************************************************************************
 *           NtUserGetClipboardData    (win32u.@)
 */
HANDLE WINAPI NtUserGetClipboardData( UINT format, struct get_clipboard_params *params )
{
    struct cached_format *cache = NULL;
    unsigned int status;
    UINT from, data_seqno;
    size_t size;
    HWND owner;
    BOOL render = TRUE;

    for (;;)
    {
        pthread_mutex_lock( &clipboard_mutex );

        if (!params->data_only) cache = get_cached_format( format );

        SERVER_START_REQ( get_clipboard_data )
        {
            req->format = format;
            req->render = render;
            if (cache && cache->handle)
            {
                req->cached = 1;
                req->seqno = cache->seqno;
            }
            wine_server_set_reply( req, params->data, params->size );
            status = wine_server_call( req );
            from = reply->from;
            size = reply->total;
            data_seqno = reply->seqno;
            owner = wine_server_ptr_handle( reply->owner );
        }
        SERVER_END_REQ;

        params->size = size;

        if (!status && size)
        {
            if (cache)
            {
                if (cache->handle && data_seqno == cache->seqno)  /* we can reuse the cached data */
                {
                    HANDLE ret = cache->handle;
                    pthread_mutex_unlock( &clipboard_mutex );
                    TRACE( "%s returning %p\n", debugstr_format( format ), ret );
                    return ret;
                }

                /* cache entry is stale, remove it */
                list_remove( &cache->entry );
                list_add_tail( &formats_to_free, &cache->entry );
            }

            if (params->data_only)
            {
                pthread_mutex_unlock( &clipboard_mutex );
                return params->data;
            }

            /* allocate new cache entry */
            if (!(cache = malloc( sizeof(*cache) )))
            {
                pthread_mutex_unlock( &clipboard_mutex );
                return 0;
            }

            cache->format = format;
            cache->seqno  = data_seqno;
            cache->handle = NULL;
            params->seqno = cache->seqno;
            list_add_tail( &cached_formats, &cache->entry );
            pthread_mutex_unlock( &clipboard_mutex );
            TRACE( "%s needs unmarshaling\n", debugstr_format( format ) );
            params->data_size = ~0;
            return 0;
        }
        pthread_mutex_unlock( &clipboard_mutex );

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            params->data_size = size;
            return 0;
        }
        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            RtlSetLastWin32Error( ERROR_NOT_FOUND ); /* no such format */
            return 0;
        }
        if (status)
        {
            RtlSetLastWin32Error( RtlNtStatusToDosError( status ));
            TRACE( "%s error %08x\n", debugstr_format( format ), status );
            return 0;
        }
        if (render)  /* try rendering it */
        {
            render = FALSE;
            if (from)
            {
                struct render_synthesized_format_params params = { .format = format, .from = from };
                ULONG ret_len;
                void *ret_ptr;
                KeUserModeCallback( NtUserRenderSynthesizedFormat, &params, sizeof(params),
                                    &ret_ptr, &ret_len );
                continue;
            }
            if (owner)
            {
                TRACE( "%s sending WM_RENDERFORMAT to %p\n", debugstr_format( format ), owner );
                send_message( owner, WM_RENDERFORMAT, format, 0 );
                continue;
            }
        }
        TRACE( "%s returning 0\n", debugstr_format( format ));
        return 0;
    }
}

LRESULT drag_drop_call( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, void *data )
{
    void *ret_ptr;
    ULONG ret_len;

    TRACE( "hwnd %p, msg %#x, wparam %#zx, lparam %#lx, data %p\n", hwnd, msg, wparam, lparam, data );

    switch (msg)
    {
    case WINE_DRAG_DROP_ENTER:
        return KeUserModeCallback( NtUserDragDropEnter, (struct format_entry *)lparam, wparam, &ret_ptr, &ret_len );
    case WINE_DRAG_DROP_LEAVE:
        return KeUserModeCallback( NtUserDragDropLeave, 0, 0, &ret_ptr, &ret_len );
    case WINE_DRAG_DROP_DRAG:
    {
        RECT rect = {LOWORD(wparam), HIWORD(wparam), LOWORD(wparam), HIWORD(wparam)};
        struct drag_drop_drag_params params =
        {
            .hwnd = hwnd,
            .effect = lparam,
        };

        rect = map_rect_raw_to_virt( rect, get_thread_dpi() );
        params.point.x = rect.left;
        params.point.y = rect.top;

        if (KeUserModeCallback( NtUserDragDropDrag, &params, sizeof(params), &ret_ptr, &ret_len ) || ret_len != sizeof(DWORD))
            return DROPEFFECT_NONE;
        return *(DWORD *)ret_ptr;
    }
    case WINE_DRAG_DROP_DROP:
    {
        struct drag_drop_drop_params params = {.hwnd = hwnd};
        if (KeUserModeCallback( NtUserDragDropDrop, &params, sizeof(params), &ret_ptr, &ret_len ) || ret_len != sizeof(DWORD))
            return DROPEFFECT_NONE;
        return *(DWORD *)ret_ptr;
    }

    case WINE_DRAG_DROP_POST:
    {
        struct drag_drop_post_params *params;
        const DROPFILES *drop = (DROPFILES *)lparam;
        RECT rect = {drop->pt.x, drop->pt.y, drop->pt.x, drop->pt.y};
        UINT drop_size = wparam, size;
        NTSTATUS status;

        size = offsetof(struct drag_drop_post_params, drop) + drop_size;
        if (!(params = malloc( size ))) return STATUS_NO_MEMORY;
        params->hwnd = hwnd;
        params->drop_size = drop_size;
        memcpy( &params->drop, drop, drop_size );

        rect = map_rect_raw_to_virt( rect, get_thread_dpi() );
        params->drop.pt.x = rect.left;
        params->drop.pt.y = rect.top;

        status = KeUserModeCallback( NtUserDragDropPost, params, size, &ret_ptr, &ret_len );
        free( params );
        return status;
    }

    default:
        FIXME( "Unknown NtUserDragDropCall msg %#x\n", msg );
        break;
    }

    return -1;
}
