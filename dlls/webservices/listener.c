/*
 * Copyright 2017 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"
#include "sock.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

HINSTANCE webservices_instance;

static BOOL winsock_loaded;

static BOOL WINAPI winsock_startup( INIT_ONCE *once, void *param, void **ctx )
{
    int ret;
    WSADATA data;
    if (!(ret = WSAStartup( MAKEWORD(1,1), &data ))) winsock_loaded = TRUE;
    else ERR( "WSAStartup failed: %d\n", ret );
    return TRUE;
}

void winsock_init(void)
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce( &once, winsock_startup, NULL, NULL );
}

_locale_t c_locale;

/******************************************************************
 *      DllMain (webservices.@)
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        webservices_instance = hinst;
        DisableThreadLibraryCalls( hinst );
        c_locale = _create_locale( LC_ALL, "C" );
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        if (winsock_loaded) WSACleanup();
        _free_locale( c_locale );
        break;
    }
    return TRUE;
}

static const struct prop_desc listener_props[] =
{
    { sizeof(ULONG), FALSE },                               /* WS_LISTENER_PROPERTY_LISTEN_BACKLOG */
    { sizeof(WS_IP_VERSION), FALSE },                       /* WS_LISTENER_PROPERTY_IP_VERSION */
    { sizeof(WS_LISTENER_STATE), TRUE },                    /* WS_LISTENER_PROPERTY_STATE */
    { sizeof(WS_CALLBACK_MODEL), FALSE },                   /* WS_LISTENER_PROPERTY_ASYNC_CALLBACK_MODEL */
    { sizeof(WS_CHANNEL_TYPE), TRUE },                      /* WS_LISTENER_PROPERTY_CHANNEL_TYPE */
    { sizeof(WS_CHANNEL_BINDING), TRUE },                   /* WS_LISTENER_PROPERTY_CHANNEL_BINDING */
    { sizeof(ULONG), FALSE },                               /* WS_LISTENER_PROPERTY_CONNECT_TIMEOUT */
    { sizeof(BOOL), FALSE },                                /* WS_LISTENER_PROPERTY_IS_MULTICAST */
    { 0, FALSE },                                           /* WS_LISTENER_PROPERTY_MULTICAST_INTERFACES */
    { sizeof(BOOL), FALSE },                                /* WS_LISTENER_PROPERTY_MULTICAST_LOOPBACK */
    { sizeof(ULONG), FALSE },                               /* WS_LISTENER_PROPERTY_CLOSE_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_LISTENER_PROPERTY_TO_HEADER_MATCHING_OPTIONS */
    { sizeof(ULONG), FALSE },                               /* WS_LISTENER_PROPERTY_TRANSPORT_URL_MATCHING_OPTIONS */
    { sizeof(WS_CUSTOM_LISTENER_CALLBACKS), FALSE },        /* WS_LISTENER_PROPERTY_CUSTOM_LISTENER_CALLBACKS */
    { 0, FALSE },                                           /* WS_LISTENER_PROPERTY_CUSTOM_LISTENER_PARAMETERS */
    { sizeof(void *), TRUE },                               /* WS_LISTENER_PROPERTY_CUSTOM_LISTENER_INSTANCE */
    { sizeof(WS_DISALLOWED_USER_AGENT_SUBSTRINGS), FALSE }  /* WS_LISTENER_PROPERTY_DISALLOWED_USER_AGENT */
};

struct listener
{
    ULONG                   magic;
    CRITICAL_SECTION        cs;
    WS_CHANNEL_TYPE         type;
    WS_CHANNEL_BINDING      binding;
    WS_LISTENER_STATE       state;
    HANDLE                  wait;
    HANDLE                  cancel;
    WS_CHANNEL             *channel;
    union
    {
        struct
        {
            SOCKET socket;
        } tcp;
        struct
        {
            SOCKET socket;
        } udp;
    } u;
    ULONG                   prop_count;
    struct prop             prop[ARRAY_SIZE( listener_props )];
};

#define LISTENER_MAGIC (('L' << 24) | ('I' << 16) | ('S' << 8) | 'T')

static struct listener *alloc_listener(void)
{
    static const ULONG count = ARRAY_SIZE( listener_props );
    struct listener *ret;
    ULONG size = sizeof(*ret) + prop_size( listener_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;

    ret->magic = LISTENER_MAGIC;
    if (!(ret->wait = CreateEventW( NULL, FALSE, FALSE, NULL )))
    {
        free( ret );
        return NULL;
    }
    if (!(ret->cancel = CreateEventW( NULL, FALSE, FALSE, NULL )))
    {
        CloseHandle( ret->wait );
        free( ret );
        return NULL;
    }
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": listener.cs");

    prop_init( listener_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void reset_listener( struct listener *listener )
{
    listener->state = WS_LISTENER_STATE_CREATED;
    SetEvent( listener->cancel );
    listener->channel = NULL;

    switch (listener->binding)
    {
    case WS_TCP_CHANNEL_BINDING:
        closesocket( listener->u.tcp.socket );
        listener->u.tcp.socket = -1;
        break;

    case WS_UDP_CHANNEL_BINDING:
        closesocket( listener->u.udp.socket );
        listener->u.udp.socket = -1;
        break;

    default: break;
    }
}

static void free_listener( struct listener *listener )
{
    reset_listener( listener );

    CloseHandle( listener->wait );
    CloseHandle( listener->cancel );

    listener->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &listener->cs );
    free( listener );
}

static HRESULT create_listener( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
                                const WS_LISTENER_PROPERTY *properties, ULONG count, struct listener **ret )
{
    struct listener *listener;
    HRESULT hr;
    ULONG i;

    if (!(listener = alloc_listener())) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( listener->prop, listener->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_listener( listener );
            return hr;
        }
    }

    listener->type    = type;
    listener->binding = binding;

    switch (listener->binding)
    {
    case WS_TCP_CHANNEL_BINDING:
        listener->u.tcp.socket = -1;
        break;

    case WS_UDP_CHANNEL_BINDING:
        listener->u.udp.socket = -1;
        break;

    default: break;
    }

    *ret = listener;
    return S_OK;
}

/**************************************************************************
 *          WsCreateListener		[webservices.@]
 */
HRESULT WINAPI WsCreateListener( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
                                 const WS_LISTENER_PROPERTY *properties, ULONG count,
                                 const WS_SECURITY_DESCRIPTION *desc, WS_LISTENER **handle,
                                 WS_ERROR *error )
{
    struct listener *listener;
    HRESULT hr;

    TRACE( "%u %u %p %lu %p %p %p\n", type, binding, properties, count, desc, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if (type != WS_CHANNEL_TYPE_DUPLEX_SESSION && type != WS_CHANNEL_TYPE_DUPLEX)
    {
        FIXME( "channel type %u not implemented\n", type );
        return E_NOTIMPL;
    }
    if (binding != WS_TCP_CHANNEL_BINDING && binding != WS_UDP_CHANNEL_BINDING)
    {
        FIXME( "channel binding %u not implemented\n", binding );
        return E_NOTIMPL;
    }

    if ((hr = create_listener( type, binding, properties, count, &listener )) != S_OK) return hr;

    TRACE( "created %p\n", listener );
    *handle = (WS_LISTENER *)listener;
    return S_OK;
}

/**************************************************************************
 *          WsFreeListener		[webservices.@]
 */
void WINAPI WsFreeListener( WS_LISTENER *handle )
{
    struct listener *listener = (struct listener *)handle;

    TRACE( "%p\n", handle );

    if (!listener) return;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return;
    }

    listener->magic = 0;

    LeaveCriticalSection( &listener->cs );
    free_listener( listener );
}

HRESULT resolve_hostname( const WCHAR *host, USHORT port, struct sockaddr *addr, int *addr_len, int flags )
{
    WCHAR service[6];
    ADDRINFOW hints, *res, *info;
    HRESULT hr = WS_E_ADDRESS_NOT_AVAILABLE;

    memset( &hints, 0, sizeof(hints) );
    hints.ai_flags  = flags;
    hints.ai_family = AF_INET;

    *addr_len = 0;
    swprintf( service, ARRAY_SIZE(service), L"%u", port );
    if (GetAddrInfoW( host, service, &hints, &res )) return HRESULT_FROM_WIN32( WSAGetLastError() );

    info = res;
    while (info && info->ai_family != AF_INET) info = info->ai_next;
    if (info)
    {
        memcpy( addr, info->ai_addr, info->ai_addrlen );
        *addr_len = info->ai_addrlen;
        hr = S_OK;
    }

    FreeAddrInfoW( res );
    return hr;
}

HRESULT parse_url( const WS_STRING *str, WS_URL_SCHEME_TYPE *scheme, WCHAR **host, USHORT *port )
{
    WS_HEAP *heap;
    WS_NETTCP_URL *url;
    HRESULT hr;

    if ((hr = WsCreateHeap( 1 << 8, 0, NULL, 0, &heap, NULL )) != S_OK) return hr;
    if ((hr = WsDecodeUrl( str, 0, heap, (WS_URL **)&url, NULL )) != S_OK)
    {
        WsFreeHeap( heap );
        return hr;
    }

    if (url->host.length == 1 && (url->host.chars[0] == '+' || url->host.chars[0] == '*')) *host = NULL;
    else
    {
        if (!(*host = malloc( (url->host.length + 1) * sizeof(WCHAR) )))
        {
            WsFreeHeap( heap );
            return E_OUTOFMEMORY;
        }
        memcpy( *host, url->host.chars, url->host.length * sizeof(WCHAR) );
        (*host)[url->host.length] = 0;
    }
    *scheme = url->url.scheme;
    *port = url->port;

    WsFreeHeap( heap );
    return hr;
}

static HRESULT open_listener_tcp( struct listener *listener, const WS_STRING *url )
{
    struct sockaddr_storage storage;
    struct sockaddr *addr = (struct sockaddr *)&storage;
    int addr_len, on = 1;
    WS_URL_SCHEME_TYPE scheme;
    WCHAR *host;
    USHORT port;
    HRESULT hr;

    if ((hr = parse_url( url, &scheme, &host, &port )) != S_OK) return hr;
    if (scheme != WS_URL_NETTCP_SCHEME_TYPE)
    {
        free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len, AI_PASSIVE );
    free( host );
    if (hr != S_OK) return hr;

    if ((listener->u.tcp.socket = socket( addr->sa_family, SOCK_STREAM, 0 )) == -1)
        return HRESULT_FROM_WIN32( WSAGetLastError() );

    if (setsockopt( listener->u.tcp.socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on) ) < 0)
    {
        closesocket( listener->u.tcp.socket );
        listener->u.tcp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    if (bind( listener->u.tcp.socket, addr, addr_len ) < 0)
    {
        closesocket( listener->u.tcp.socket );
        listener->u.tcp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    if (listen( listener->u.tcp.socket, 0 ) < 0)
    {
        closesocket( listener->u.tcp.socket );
        listener->u.tcp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    listener->state = WS_LISTENER_STATE_OPEN;
    return S_OK;
}

static HRESULT open_listener_udp( struct listener *listener, const WS_STRING *url )
{
    struct sockaddr_storage storage;
    struct sockaddr *addr = (struct sockaddr *)&storage;
    int addr_len;
    WS_URL_SCHEME_TYPE scheme;
    WCHAR *host;
    USHORT port;
    HRESULT hr;

    if ((hr = parse_url( url, &scheme, &host, &port )) != S_OK) return hr;
    if (scheme != WS_URL_SOAPUDP_SCHEME_TYPE)
    {
        free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len, AI_PASSIVE );
    free( host );
    if (hr != S_OK) return hr;

    if ((listener->u.udp.socket = socket( addr->sa_family, SOCK_DGRAM, 0 )) == -1)
        return HRESULT_FROM_WIN32( WSAGetLastError() );

    if (bind( listener->u.udp.socket, addr, addr_len ) < 0)
    {
        closesocket( listener->u.udp.socket );
        listener->u.udp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    listener->state = WS_LISTENER_STATE_OPEN;
    return S_OK;
}

static HRESULT open_listener( struct listener *listener, const WS_STRING *url )
{
    switch (listener->binding)
    {
    case WS_TCP_CHANNEL_BINDING:
        return open_listener_tcp( listener, url );

    case WS_UDP_CHANNEL_BINDING:
        return open_listener_udp( listener, url );

    default:
        ERR( "unhandled binding %u\n", listener->binding );
        return E_NOTIMPL;
    }
}

/**************************************************************************
 *          WsOpenListener		[webservices.@]
 */
HRESULT WINAPI WsOpenListener( WS_LISTENER *handle, const WS_STRING *url, const WS_ASYNC_CONTEXT *ctx,
                               WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr;

    TRACE( "%p %s %p %p\n", handle, url ? debugstr_wn(url->chars, url->length) : "null", ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!listener || !url) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    if (listener->state != WS_LISTENER_STATE_CREATED) hr = WS_E_INVALID_OPERATION;
    else hr = open_listener( listener, url );

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static void close_listener( struct listener *listener )
{
    reset_listener( listener );
    listener->state = WS_LISTENER_STATE_CLOSED;
}

/**************************************************************************
 *          WsCloseListener		[webservices.@]
 */
HRESULT WINAPI WsCloseListener( WS_LISTENER *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!listener) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    close_listener( listener );

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsResetListener		[webservices.@]
 */
HRESULT WINAPI WsResetListener( WS_LISTENER *handle, WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!listener) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    if (listener->state != WS_LISTENER_STATE_CREATED && listener->state != WS_LISTENER_STATE_CLOSED)
        hr = WS_E_INVALID_OPERATION;
    else
        reset_listener( listener );

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetListenerProperty		[webservices.@]
 */
HRESULT WINAPI WsGetListenerProperty( WS_LISTENER *handle, WS_LISTENER_PROPERTY_ID id, void *buf,
                                      ULONG size, WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!listener) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    switch (id)
    {
    case WS_LISTENER_PROPERTY_STATE:
        if (!buf || size != sizeof(listener->state)) hr = E_INVALIDARG;
        else *(WS_LISTENER_STATE *)buf = listener->state;
        break;

    case WS_LISTENER_PROPERTY_CHANNEL_TYPE:
        if (!buf || size != sizeof(listener->type)) hr = E_INVALIDARG;
        else *(WS_CHANNEL_TYPE *)buf = listener->type;
        break;

    case WS_LISTENER_PROPERTY_CHANNEL_BINDING:
        if (!buf || size != sizeof(listener->binding)) hr = E_INVALIDARG;
        else *(WS_CHANNEL_BINDING *)buf = listener->binding;
        break;

    default:
        hr = prop_get( listener->prop, listener->prop_count, id, buf, size );
    }

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetListenerProperty		[webservices.@]
 */
HRESULT WINAPI WsSetListenerProperty( WS_LISTENER *handle, WS_LISTENER_PROPERTY_ID id, const void *value,
                                      ULONG size, WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu %p\n", handle, id, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!listener) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    hr = prop_set( listener->prop, listener->prop_count, id, value, size );

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsAcceptChannel		[webservices.@]
 */
HRESULT WINAPI WsAcceptChannel( WS_LISTENER *handle, WS_CHANNEL *channel_handle, const WS_ASYNC_CONTEXT *ctx,
                                WS_ERROR *error )
{
    struct listener *listener = (struct listener *)handle;
    HRESULT hr = E_NOTIMPL;
    HANDLE wait, cancel;

    TRACE( "%p %p %p %p\n", handle, channel_handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!listener || !channel_handle) return E_INVALIDARG;

    EnterCriticalSection( &listener->cs );

    if (listener->magic != LISTENER_MAGIC)
    {
        LeaveCriticalSection( &listener->cs );
        return E_INVALIDARG;
    }

    if (listener->state != WS_LISTENER_STATE_OPEN || (listener->channel && listener->channel != channel_handle))
    {
        hr = WS_E_INVALID_OPERATION;
    }
    else
    {
        wait = listener->wait;
        cancel = listener->cancel;
        listener->channel = channel_handle;

        switch (listener->binding)
        {
        case WS_TCP_CHANNEL_BINDING:
        {
            SOCKET socket = listener->u.tcp.socket;

            LeaveCriticalSection( &listener->cs );
            hr = channel_accept_tcp( socket, wait, cancel, channel_handle );
            TRACE( "returning %#lx\n", hr );
            return hr;
        }
        case WS_UDP_CHANNEL_BINDING:
        {
            SOCKET socket = listener->u.udp.socket;

            LeaveCriticalSection( &listener->cs );
            hr = channel_accept_udp( socket, wait, cancel, channel_handle );
            TRACE( "returning %#lx\n", hr );
            return hr;
        }
        default:
            FIXME( "listener binding %u not supported\n", listener->binding );
            break;
        }
    }

    LeaveCriticalSection( &listener->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}
