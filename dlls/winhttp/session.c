/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winhttp.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static void set_last_error( DWORD error )
{
    /* FIXME */
    SetLastError( error );
}

/***********************************************************************
 *          WinHttpCheckPlatform (winhttp.@)
 */
BOOL WINAPI WinHttpCheckPlatform( void )
{
    TRACE("\n");
    return TRUE;
}

/***********************************************************************
 *          session_destroy (internal)
 */
static void session_destroy( object_header_t *hdr )
{
    session_t *session = (session_t *)hdr;

    TRACE("%p\n", session);

    heap_free( session->agent );
    heap_free( session->proxy_server );
    heap_free( session->proxy_bypass );
    heap_free( session->proxy_username );
    heap_free( session->proxy_password );
    heap_free( session );
}

static const object_vtbl_t session_vtbl =
{
    session_destroy,
    NULL,
    NULL
};

/***********************************************************************
 *          WinHttpOpen (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpen( LPCWSTR agent, DWORD access, LPCWSTR proxy, LPCWSTR bypass, DWORD flags )
{
    session_t *session;
    HINTERNET handle = NULL;

    TRACE("%s, %u, %s, %s, 0x%08x\n", debugstr_w(agent), access, debugstr_w(proxy), debugstr_w(bypass), flags);

    if (!(session = heap_alloc_zero( sizeof(session_t) ))) return NULL;

    session->hdr.type = WINHTTP_HANDLE_TYPE_SESSION;
    session->hdr.vtbl = &session_vtbl;
    session->hdr.flags = flags;
    session->hdr.refs = 1;
    session->access = access;

    if (agent && !(session->agent = strdupW( agent ))) goto end;
    if (proxy && !(session->proxy_server = strdupW( proxy ))) goto end;
    if (bypass && !(session->proxy_bypass = strdupW( bypass ))) goto end;

    if (!(handle = alloc_handle( &session->hdr ))) goto end;
    session->hdr.handle = handle;

end:
    release_object( &session->hdr );
    TRACE("returning %p\n", handle);
    return handle;
}

/***********************************************************************
 *          connect_destroy (internal)
 */
static void connect_destroy( object_header_t *hdr )
{
    connect_t *connect = (connect_t *)hdr;

    TRACE("%p\n", connect);

    release_object( &connect->session->hdr );

    heap_free( connect->hostname );
    heap_free( connect->servername );
    heap_free( connect->username );
    heap_free( connect->password );
    heap_free( connect );
}

static const object_vtbl_t connect_vtbl =
{
    connect_destroy,
    NULL,
    NULL
};

/***********************************************************************
 *          WinHttpConnect (winhttp.@)
 */
HINTERNET WINAPI WinHttpConnect( HINTERNET hsession, LPCWSTR server, INTERNET_PORT port, DWORD reserved )
{
    connect_t *connect;
    session_t *session;
    HINTERNET hconnect = NULL;

    TRACE("%p, %s, %u, %x\n", hsession, debugstr_w(server), port, reserved);

    if (!server)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    if (!(session = (session_t *)grab_object( hsession )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (session->hdr.type != WINHTTP_HANDLE_TYPE_SESSION)
    {
        release_object( &session->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(connect = heap_alloc_zero( sizeof(connect_t) )))
    {
        release_object( &session->hdr );
        return NULL;
    }
    connect->hdr.type = WINHTTP_HANDLE_TYPE_CONNECT;
    connect->hdr.vtbl = &connect_vtbl;
    connect->hdr.refs = 1;
    connect->hdr.flags = session->hdr.flags;
    connect->hdr.callback = session->hdr.callback;
    connect->hdr.notify_mask = session->hdr.notify_mask;

    addref_object( &session->hdr );
    connect->session = session;
    list_add_head( &session->hdr.children, &connect->hdr.entry );

    if (server && !(connect->hostname = strdupW( server ))) goto end;
    connect->hostport = port;

    if (!(hconnect = alloc_handle( &connect->hdr ))) goto end;
    connect->hdr.handle = hconnect;

end:
    release_object( &connect->hdr );

    TRACE("returning %p\n", hconnect);
    return hconnect;
}

/***********************************************************************
 *          request_destroy (internal)
 */
static void request_destroy( object_header_t *hdr )
{
    request_t *request = (request_t *)hdr;
    int i;

    TRACE("%p\n", request);

    release_object( &request->connect->hdr );

    heap_free( request->verb );
    heap_free( request->path );
    heap_free( request->version );
    heap_free( request->raw_headers );
    heap_free( request->status_text );
    for (i = 0; i < request->num_headers; i++)
    {
        heap_free( request->headers[i].field );
        heap_free( request->headers[i].value );
    }
    heap_free( request->headers );
    heap_free( request );
}

static const object_vtbl_t request_vtbl =
{
    request_destroy,
    NULL,
    NULL
};

/***********************************************************************
 *          WinHttpOpenRequest (winhttp.@)
 */
HINTERNET WINAPI WinHttpOpenRequest( HINTERNET hconnect, LPCWSTR verb, LPCWSTR object, LPCWSTR version,
                                     LPCWSTR referrer, LPCWSTR *types, DWORD flags )
{
    request_t *request;
    connect_t *connect;
    HINTERNET hrequest = NULL;

    TRACE("%p, %s, %s, %s, %s, %p, 0x%08x\n", hconnect, debugstr_w(verb), debugstr_w(object),
          debugstr_w(version), debugstr_w(referrer), types, flags);

    if (!(connect = (connect_t *)grab_object( hconnect )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (connect->hdr.type != WINHTTP_HANDLE_TYPE_CONNECT)
    {
        release_object( &connect->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(request = heap_alloc_zero( sizeof(request_t) )))
    {
        release_object( &connect->hdr );
        return NULL;
    }
    request->hdr.type = WINHTTP_HANDLE_TYPE_REQUEST;
    request->hdr.vtbl = &request_vtbl;
    request->hdr.refs = 1;
    request->hdr.flags = flags;
    request->hdr.callback = connect->hdr.callback;
    request->hdr.notify_mask = connect->hdr.notify_mask;

    addref_object( &connect->hdr );
    request->connect = connect;
    list_add_head( &connect->hdr.children, &request->hdr.entry );

    if (verb && !(request->verb = strdupW( verb ))) goto end;
    if (object && !(request->path = strdupW( object ))) goto end;
    if (version && !(request->version = strdupW( version ))) goto end;

    if (!(hrequest = alloc_handle( &request->hdr ))) goto end;
    request->hdr.handle = hrequest;

end:
    release_object( &request->hdr );

    TRACE("returning %p\n", hrequest);
    return hrequest;
}

/***********************************************************************
 *          WinHttpCloseHandle (winhttp.@)
 */
BOOL WINAPI WinHttpCloseHandle( HINTERNET handle )
{
    object_header_t *hdr;

    TRACE("%p\n", handle);

    if (!(hdr = grab_object( handle )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    release_object( hdr );
    free_handle( handle );
    return TRUE;
}
