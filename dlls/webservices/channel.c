/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "webservices_private.h"
#include "sock.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc channel_props[] =
{
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE */
    { sizeof(UINT64), FALSE },                              /* WS_CHANNEL_PROPERTY_MAX_STREAMED_MESSAGE_SIZE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_STREAMED_START_SIZE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_STREAMED_FLUSH_SIZE */
    { sizeof(WS_ENCODING), FALSE },                         /* WS_CHANNEL_PROPERTY_ENCODING */
    { sizeof(WS_ENVELOPE_VERSION), FALSE },                 /* WS_CHANNEL_PROPERTY_ENVELOPE_VERSION */
    { sizeof(WS_ADDRESSING_VERSION), FALSE },               /* WS_CHANNEL_PROPERTY_ADDRESSING_VERSION */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_SESSION_DICTIONARY_SIZE */
    { sizeof(WS_CHANNEL_STATE), TRUE },                     /* WS_CHANNEL_PROPERTY_STATE */
    { sizeof(WS_CALLBACK_MODEL), FALSE },                   /* WS_CHANNEL_PROPERTY_ASYNC_CALLBACK_MODEL */
    { sizeof(WS_IP_VERSION), FALSE },                       /* WS_CHANNEL_PROPERTY_IP_VERSION */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_RESOLVE_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_CONNECT_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_SEND_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_RECEIVE_RESPONSE_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_RECEIVE_TIMEOUT */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_CLOSE_TIMEOUT */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_ENABLE_TIMEOUTS */
    { sizeof(WS_TRANSFER_MODE), FALSE },                    /* WS_CHANNEL_PROPERTY_TRANSFER_MODE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MULTICAST_INTERFACE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MULTICAST_HOPS */
    { sizeof(WS_ENDPOINT_ADDRESS), TRUE },                  /* WS_CHANNEL_PROPERTY_REMOTE_ADDRESS */
    { sizeof(SOCKADDR_STORAGE), TRUE },                     /* WS_CHANNEL_PROPERTY_REMOTE_IP_ADDRESS */
    { sizeof(ULONGLONG), TRUE },                            /* WS_CHANNEL_PROPERTY_HTTP_CONNECTION_ID */
    { sizeof(WS_CUSTOM_CHANNEL_CALLBACKS), FALSE },         /* WS_CHANNEL_PROPERTY_CUSTOM_CHANNEL_CALLBACKS */
    { 0, FALSE },                                           /* WS_CHANNEL_PROPERTY_CUSTOM_CHANNEL_PARAMETERS */
    { sizeof(void *), FALSE },                              /* WS_CHANNEL_PROPERTY_CUSTOM_CHANNEL_INSTANCE */
    { sizeof(WS_STRING), TRUE },                            /* WS_CHANNEL_PROPERTY_TRANSPORT_URL */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_NO_DELAY */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_SEND_KEEP_ALIVES */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_KEEP_ALIVE_TIME */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_KEEP_ALIVE_INTERVAL */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_HTTP_SERVER_CONNECTIONS */
    { sizeof(BOOL), TRUE },                                 /* WS_CHANNEL_PROPERTY_IS_SESSION_SHUT_DOWN */
    { sizeof(WS_CHANNEL_TYPE), TRUE },                      /* WS_CHANNEL_PROPERTY_CHANNEL_TYPE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_TRIM_BUFFERED_MESSAGE_SIZE */
    { sizeof(WS_CHANNEL_ENCODER), FALSE },                  /* WS_CHANNEL_PROPERTY_ENCODER */
    { sizeof(WS_CHANNEL_DECODER), FALSE },                  /* WS_CHANNEL_PROPERTY_DECODER */
    { sizeof(WS_PROTECTION_LEVEL), TRUE },                  /* WS_CHANNEL_PROPERTY_PROTECTION_LEVEL */
    { sizeof(WS_COOKIE_MODE), FALSE },                      /* WS_CHANNEL_PROPERTY_COOKIE_MODE */
    { sizeof(WS_HTTP_PROXY_SETTING_MODE), FALSE },          /* WS_CHANNEL_PROPERTY_HTTP_PROXY_SETTING_MODE */
    { sizeof(WS_CUSTOM_HTTP_PROXY), FALSE },                /* WS_CHANNEL_PROPERTY_CUSTOM_HTTP_PROXY */
    { sizeof(WS_HTTP_MESSAGE_MAPPING), FALSE },             /* WS_CHANNEL_PROPERTY_HTTP_MESSAGE_MAPPING */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_ENABLE_HTTP_REDIRECT */
    { sizeof(WS_HTTP_REDIRECT_CALLBACK_CONTEXT), FALSE },   /* WS_CHANNEL_PROPERTY_HTTP_REDIRECT_CALLBACK_CONTEXT */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_FAULTS_AS_ERRORS */
    { sizeof(BOOL), FALSE },                                /* WS_CHANNEL_PROPERTY_ALLOW_UNSECURED_FAULTS */
    { sizeof(WCHAR *), TRUE },                              /* WS_CHANNEL_PROPERTY_HTTP_SERVER_SPN */
    { sizeof(WCHAR *), TRUE },                              /* WS_CHANNEL_PROPERTY_HTTP_PROXY_SPN */
    { sizeof(ULONG), FALSE }                                /* WS_CHANNEL_PROPERTY_MAX_HTTP_REQUEST_HEADERS_BUFFER_SIZE */
};

struct channel
{
    ULONG                   magic;
    CRITICAL_SECTION        cs;
    WS_CHANNEL_TYPE         type;
    WS_CHANNEL_BINDING      binding;
    WS_CHANNEL_STATE        state;
    WS_ENDPOINT_ADDRESS     addr;
    WS_XML_WRITER          *writer;
    WS_XML_READER          *reader;
    WS_MESSAGE             *msg;
    union
    {
        struct
        {
            HINTERNET session;
            HINTERNET connect;
            HINTERNET request;
        } http;
        struct
        {
            SOCKET socket;
        } tcp;
        struct
        {
            SOCKET socket;
        } udp;
    } u;
    char                   *read_buf;
    ULONG                   read_buflen;
    ULONG                   read_size;
    ULONG                   prop_count;
    struct prop             prop[sizeof(channel_props)/sizeof(channel_props[0])];
};

#define CHANNEL_MAGIC (('C' << 24) | ('H' << 16) | ('A' << 8) | 'N')

static struct channel *alloc_channel(void)
{
    static const ULONG count = sizeof(channel_props)/sizeof(channel_props[0]);
    struct channel *ret;
    ULONG size = sizeof(*ret) + prop_size( channel_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ret->magic      = CHANNEL_MAGIC;
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": channel.cs");

    prop_init( channel_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void reset_channel( struct channel *channel )
{
    channel->state           = WS_CHANNEL_STATE_CREATED;
    heap_free( channel->addr.url.chars );
    channel->addr.url.chars  = NULL;
    channel->addr.url.length = 0;
    channel->msg             = NULL;
    channel->read_size       = 0;

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        WinHttpCloseHandle( channel->u.http.request );
        channel->u.http.request = NULL;
        WinHttpCloseHandle( channel->u.http.connect );
        channel->u.http.connect = NULL;
        WinHttpCloseHandle( channel->u.http.session );
        channel->u.http.session = NULL;
        break;

    case WS_TCP_CHANNEL_BINDING:
        closesocket( channel->u.tcp.socket );
        channel->u.tcp.socket = -1;
        break;

    case WS_UDP_CHANNEL_BINDING:
        closesocket( channel->u.udp.socket );
        channel->u.udp.socket = -1;
        break;

    default: break;
    }
}

static void free_channel( struct channel *channel )
{
    reset_channel( channel );

    WsFreeWriter( channel->writer );
    WsFreeReader( channel->reader );

    heap_free( channel->read_buf );

    channel->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &channel->cs );
    heap_free( channel );
}

static HRESULT create_channel( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
                               const WS_CHANNEL_PROPERTY *properties, ULONG count, struct channel **ret )
{
    struct channel *channel;
    ULONG i, msg_size = 65536;
    HRESULT hr;

    if (!(channel = alloc_channel())) return E_OUTOFMEMORY;

    prop_set( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE,
              &msg_size, sizeof(msg_size) );

    for (i = 0; i < count; i++)
    {
        hr = prop_set( channel->prop, channel->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_channel( channel );
            return hr;
        }
    }

    channel->type    = type;
    channel->binding = binding;

    switch (channel->binding)
    {
    case WS_TCP_CHANNEL_BINDING:
        channel->u.tcp.socket = -1;
        break;

    case WS_UDP_CHANNEL_BINDING:
        channel->u.udp.socket = -1;
        break;

    default: break;
    }

    *ret = channel;
    return S_OK;
}

/**************************************************************************
 *          WsCreateChannel		[webservices.@]
 */
HRESULT WINAPI WsCreateChannel( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
                                const WS_CHANNEL_PROPERTY *properties, ULONG count,
                                const WS_SECURITY_DESCRIPTION *desc, WS_CHANNEL **handle,
                                WS_ERROR *error )
{
    struct channel *channel;
    HRESULT hr;

    TRACE( "%u %u %p %u %p %p %p\n", type, binding, properties, count, desc, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if (type != WS_CHANNEL_TYPE_REQUEST && type != WS_CHANNEL_TYPE_DUPLEX &&
        type != WS_CHANNEL_TYPE_DUPLEX_SESSION)
    {
        FIXME( "channel type %u not implemented\n", type );
        return E_NOTIMPL;
    }
    if (binding != WS_HTTP_CHANNEL_BINDING && binding != WS_TCP_CHANNEL_BINDING &&
        binding != WS_UDP_CHANNEL_BINDING)
    {
        FIXME( "channel binding %u not implemented\n", binding );
        return E_NOTIMPL;
    }

    if ((hr = create_channel( type, binding, properties, count, &channel )) != S_OK) return hr;

    *handle = (WS_CHANNEL *)channel;
    return S_OK;
}

/**************************************************************************
 *          WsCreateChannelForListener		[webservices.@]
 */
HRESULT WINAPI WsCreateChannelForListener( WS_LISTENER *listener_handle, const WS_CHANNEL_PROPERTY *properties,
                                           ULONG count, WS_CHANNEL **handle, WS_ERROR *error )
{
    struct channel *channel;
    WS_CHANNEL_TYPE type;
    WS_CHANNEL_BINDING binding;
    HRESULT hr;

    TRACE( "%p %p %u %p %p\n", listener_handle, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!listener_handle || !handle) return E_INVALIDARG;

    if ((hr = WsGetListenerProperty( listener_handle, WS_LISTENER_PROPERTY_CHANNEL_TYPE, &type,
                                     sizeof(type), NULL )) != S_OK) return hr;

    if ((hr = WsGetListenerProperty( listener_handle, WS_LISTENER_PROPERTY_CHANNEL_BINDING, &binding,
                                     sizeof(binding), NULL )) != S_OK) return hr;

    if ((hr = create_channel( type, binding, properties, count, &channel )) != S_OK) return hr;

    *handle = (WS_CHANNEL *)channel;
    return S_OK;
}

/**************************************************************************
 *          WsFreeChannel		[webservices.@]
 */
void WINAPI WsFreeChannel( WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p\n", handle );

    if (!channel) return;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return;
    }

    channel->magic = 0;

    LeaveCriticalSection( &channel->cs );
    free_channel( channel );
}

/**************************************************************************
 *          WsResetChannel		[webservices.@]
 */
HRESULT WINAPI WsResetChannel( WS_CHANNEL *handle, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if (channel->state != WS_CHANNEL_STATE_CREATED && channel->state != WS_CHANNEL_STATE_CLOSED)
    {
        LeaveCriticalSection( &channel->cs );
        return WS_E_INVALID_OPERATION;
    }

    reset_channel( channel );

    LeaveCriticalSection( &channel->cs );
    return S_OK;
}

/**************************************************************************
 *          WsGetChannelProperty		[webservices.@]
 */
HRESULT WINAPI WsGetChannelProperty( WS_CHANNEL *handle, WS_CHANNEL_PROPERTY_ID id, void *buf,
                                     ULONG size, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    switch (id)
    {
    case WS_CHANNEL_PROPERTY_CHANNEL_TYPE:
        if (!buf || size != sizeof(channel->type)) hr = E_INVALIDARG;
        else *(WS_CHANNEL_TYPE *)buf = channel->type;
        break;

    default:
        hr = prop_get( channel->prop, channel->prop_count, id, buf, size );
    }

    LeaveCriticalSection( &channel->cs );
    return hr;
}

/**************************************************************************
 *          WsSetChannelProperty		[webservices.@]
 */
HRESULT WINAPI WsSetChannelProperty( WS_CHANNEL *handle, WS_CHANNEL_PROPERTY_ID id, const void *value,
                                     ULONG size, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %u\n", handle, id, value, size );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    hr = prop_set( channel->prop, channel->prop_count, id, value, size );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

static HRESULT open_channel( struct channel *channel, const WS_ENDPOINT_ADDRESS *endpoint )
{
    if (endpoint->headers || endpoint->extensions || endpoint->identity)
    {
        FIXME( "headers, extensions or identity not supported\n" );
        return E_NOTIMPL;
    }

    if (!(channel->addr.url.chars = heap_alloc( endpoint->url.length * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    memcpy( channel->addr.url.chars, endpoint->url.chars, endpoint->url.length * sizeof(WCHAR) );
    channel->addr.url.length = endpoint->url.length;

    channel->state = WS_CHANNEL_STATE_OPEN;
    return S_OK;
}

/**************************************************************************
 *          WsOpenChannel		[webservices.@]
 */
HRESULT WINAPI WsOpenChannel( WS_CHANNEL *handle, const WS_ENDPOINT_ADDRESS *endpoint,
                              const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, endpoint, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !endpoint) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if (channel->state != WS_CHANNEL_STATE_CREATED)
    {
        LeaveCriticalSection( &channel->cs );
        return WS_E_INVALID_OPERATION;
    }

    hr = open_channel( channel, endpoint );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

static void close_channel( struct channel *channel )
{
    reset_channel( channel );
    channel->state = WS_CHANNEL_STATE_CLOSED;
}

/**************************************************************************
 *          WsCloseChannel		[webservices.@]
 */
HRESULT WINAPI WsCloseChannel( WS_CHANNEL *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    close_channel( channel );

    LeaveCriticalSection( &channel->cs );
    return S_OK;
}

static HRESULT parse_http_url( const WCHAR *url, ULONG len, URL_COMPONENTS *uc )
{
    HRESULT hr = E_OUTOFMEMORY;
    WCHAR *tmp;
    DWORD err;

    memset( uc, 0, sizeof(*uc) );
    uc->dwStructSize      = sizeof(*uc);
    uc->dwHostNameLength  = 128;
    uc->lpszHostName      = heap_alloc( uc->dwHostNameLength * sizeof(WCHAR) );
    uc->dwUrlPathLength   = 128;
    uc->lpszUrlPath       = heap_alloc( uc->dwUrlPathLength * sizeof(WCHAR) );
    uc->dwExtraInfoLength = 128;
    uc->lpszExtraInfo     = heap_alloc( uc->dwExtraInfoLength * sizeof(WCHAR) );
    if (!uc->lpszHostName || !uc->lpszUrlPath || !uc->lpszExtraInfo) goto error;

    if (!WinHttpCrackUrl( url, len, ICU_DECODE, uc ))
    {
        if ((err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32( err );
            goto error;
        }
        if (!(tmp = heap_realloc( uc->lpszHostName, uc->dwHostNameLength * sizeof(WCHAR) ))) goto error;
        uc->lpszHostName = tmp;
        if (!(tmp = heap_realloc( uc->lpszUrlPath, uc->dwUrlPathLength * sizeof(WCHAR) ))) goto error;
        uc->lpszUrlPath = tmp;
        if (!(tmp = heap_realloc( uc->lpszExtraInfo, uc->dwExtraInfoLength * sizeof(WCHAR) ))) goto error;
        uc->lpszExtraInfo = tmp;
        WinHttpCrackUrl( url, len, ICU_DECODE, uc );
    }

    return S_OK;

error:
    heap_free( uc->lpszHostName );
    heap_free( uc->lpszUrlPath );
    heap_free( uc->lpszExtraInfo );
    return hr;
}

static HRESULT connect_channel_http( struct channel *channel )
{
    static const WCHAR agentW[] =
        {'M','S','-','W','e','b','S','e','r','v','i','c','e','s','/','1','.','0',0};
    static const WCHAR postW[] =
        {'P','O','S','T',0};
    HINTERNET ses = NULL, con = NULL, req = NULL;
    WCHAR *path;
    URL_COMPONENTS uc;
    DWORD flags = 0;
    HRESULT hr;

    if (channel->u.http.request) return S_OK;

    if ((hr = parse_http_url( channel->addr.url.chars, channel->addr.url.length, &uc )) != S_OK) return hr;
    if (!uc.dwExtraInfoLength) path = uc.lpszUrlPath;
    else if (!(path = heap_alloc( (uc.dwUrlPathLength + uc.dwExtraInfoLength + 1) * sizeof(WCHAR) )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    else
    {
        strcpyW( path, uc.lpszUrlPath );
        strcatW( path, uc.lpszExtraInfo );
    }

    switch (uc.nScheme)
    {
    case INTERNET_SCHEME_HTTP: break;
    case INTERNET_SCHEME_HTTPS:
        flags |= WINHTTP_FLAG_SECURE;
        break;

    default:
        hr = WS_E_INVALID_ENDPOINT_URL;
        goto done;
    }

    if (!(ses = WinHttpOpen( agentW, 0, NULL, NULL, 0 )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto done;
    }
    if (!(con = WinHttpConnect( ses, uc.lpszHostName, uc.nPort, 0 )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto done;
    }
    if (!(req = WinHttpOpenRequest( con, postW, path, NULL, NULL, NULL, flags )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto done;
    }

    if ((hr = message_insert_http_headers( channel->msg, req )) != S_OK) goto done;

    channel->u.http.session = ses;
    channel->u.http.connect = con;
    channel->u.http.request = req;

done:
    if (hr != S_OK)
    {
        WinHttpCloseHandle( req );
        WinHttpCloseHandle( con );
        WinHttpCloseHandle( ses );
    }
    heap_free( uc.lpszHostName );
    heap_free( uc.lpszUrlPath );
    heap_free( uc.lpszExtraInfo );
    if (path != uc.lpszUrlPath) heap_free( path );
    return hr;
}

static HRESULT connect_channel_tcp( struct channel *channel )
{
    struct sockaddr_storage storage;
    struct sockaddr *addr = (struct sockaddr *)&storage;
    int addr_len;
    WS_URL_SCHEME_TYPE scheme;
    WCHAR *host;
    USHORT port;
    HRESULT hr;

    if (channel->u.tcp.socket != -1) return S_OK;

    if ((hr = parse_url( &channel->addr.url, &scheme, &host, &port )) != S_OK) return hr;
    if (scheme != WS_URL_NETTCP_SCHEME_TYPE)
    {
        heap_free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len );
    heap_free( host );
    if (hr != S_OK) return hr;

    if ((channel->u.tcp.socket = socket( addr->sa_family, SOCK_STREAM, 0 )) == -1)
        return HRESULT_FROM_WIN32( WSAGetLastError() );

    if (connect( channel->u.tcp.socket, addr, addr_len ) < 0)
    {
        closesocket( channel->u.tcp.socket );
        channel->u.tcp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    return S_OK;
}

static HRESULT connect_channel_udp( struct channel *channel )
{
    struct sockaddr_storage storage;
    struct sockaddr *addr = (struct sockaddr *)&storage;
    int addr_len;
    WS_URL_SCHEME_TYPE scheme;
    WCHAR *host;
    USHORT port;
    HRESULT hr;

    if (channel->u.udp.socket != -1) return S_OK;

    if ((hr = parse_url( &channel->addr.url, &scheme, &host, &port )) != S_OK) return hr;
    if (scheme != WS_URL_SOAPUDP_SCHEME_TYPE)
    {
        heap_free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len );
    heap_free( host );
    if (hr != S_OK) return hr;

    if ((channel->u.udp.socket = socket( addr->sa_family, SOCK_DGRAM, 0 )) == -1)
        return HRESULT_FROM_WIN32( WSAGetLastError() );

    if (connect( channel->u.udp.socket, addr, addr_len ) < 0)
    {
        closesocket( channel->u.udp.socket );
        channel->u.udp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    return S_OK;
}

static HRESULT connect_channel( struct channel *channel )
{
    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        return connect_channel_http( channel );

    case WS_TCP_CHANNEL_BINDING:
        return connect_channel_tcp( channel );

    case WS_UDP_CHANNEL_BINDING:
        return connect_channel_udp( channel );

    default:
        ERR( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }
}

static HRESULT write_message( WS_MESSAGE *handle, WS_XML_WRITER *writer, const WS_ELEMENT_DESCRIPTION *desc,
                              WS_WRITE_OPTION option, const void *body, ULONG size )
{
    HRESULT hr;
    if ((hr = WsWriteEnvelopeStart( handle, writer, NULL, NULL, NULL )) != S_OK) return hr;
    if ((hr = WsWriteBody( handle, desc, option, body, size, NULL )) != S_OK) return hr;
    return WsWriteEnvelopeEnd( handle, NULL );
}

static HRESULT send_message_http( HANDLE request, BYTE *data, ULONG len )
{
    if (!WinHttpSendRequest( request, NULL, 0, data, len, len, 0 ))
        return HRESULT_FROM_WIN32( GetLastError() );

    if (!WinHttpReceiveResponse( request, NULL ))
        return HRESULT_FROM_WIN32( GetLastError() );
    return S_OK;
}

static HRESULT send_message_sock( SOCKET socket, BYTE *data, ULONG len )
{
    if (send( socket, (const char *)data, len, 0 ) < 0)
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    return S_OK;
}

static HRESULT send_message( struct channel *channel, WS_MESSAGE *msg )
{
    WS_XML_WRITER *writer;
    WS_BYTES buf;
    HRESULT hr;

    channel->msg = msg;
    if ((hr = connect_channel( channel )) != S_OK) return hr;

    WsGetMessageProperty( channel->msg, WS_MESSAGE_PROPERTY_BODY_WRITER, &writer, sizeof(writer), NULL );
    WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &buf, sizeof(buf), NULL );

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        return send_message_http( channel->u.http.request, buf.bytes, buf.length );

    case WS_TCP_CHANNEL_BINDING:
        return send_message_sock( channel->u.tcp.socket, buf.bytes, buf.length );

    case WS_UDP_CHANNEL_BINDING:
        return send_message_sock( channel->u.udp.socket, buf.bytes, buf.length );

    default:
        ERR( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }
}

HRESULT channel_send_message( WS_CHANNEL *handle, WS_MESSAGE *msg )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    hr = send_message( channel, msg );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING text = { {WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8 };
    WS_XML_WRITER_BUFFER_OUTPUT buf = { {WS_XML_WRITER_OUTPUT_TYPE_BUFFER} };
    return WsSetOutput( writer, &text.encoding, &buf.output, NULL, 0, NULL );
}

static HRESULT init_writer( struct channel *channel )
{
    HRESULT hr;
    if (!channel->writer && (hr = WsCreateWriter( NULL, 0, &channel->writer, NULL )) != S_OK) return hr;
    return set_output( channel->writer );
}

/**************************************************************************
 *          WsSendMessage		[webservices.@]
 */
HRESULT WINAPI WsSendMessage( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION *desc,
                              WS_WRITE_OPTION option, const void *body, ULONG size, const WS_ASYNC_CONTEXT *ctx,
                              WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %08x %p %u %p %p\n", handle, msg, desc, option, body, size, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !msg || !desc) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL )) != S_OK) goto done;
    if ((hr = WsAddressMessage( msg, &channel->addr, NULL )) != S_OK) goto done;
    if ((hr = message_set_action( msg, desc->action )) != S_OK) goto done;

    if ((hr = init_writer( channel )) != S_OK) goto done;
    if ((hr = write_message( msg, channel->writer, desc->bodyElementDescription, option, body, size )) != S_OK)
        goto done;
    hr = send_message( channel, msg );

done:
    LeaveCriticalSection( &channel->cs );
    return hr;
}

static HRESULT resize_read_buffer( struct channel *channel, ULONG size )
{
    if (!channel->read_buf)
    {
        if (!(channel->read_buf = heap_alloc( size ))) return E_OUTOFMEMORY;
        channel->read_buflen = size;
        return S_OK;
    }
    if (channel->read_buflen < size)
    {
        char *tmp;
        ULONG new_size = max( size, channel->read_buflen * 2 );
        if (!(tmp = heap_realloc( channel->read_buf, new_size ))) return E_OUTOFMEMORY;
        channel->read_buf = tmp;
        channel->read_buflen = new_size;
    }
    return S_OK;
}

static HRESULT set_input( WS_XML_READER *reader, char *data, ULONG size )
{
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8};
    WS_XML_READER_BUFFER_INPUT buf;

    buf.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
    buf.encodedData     = data;
    buf.encodedDataSize = size;
    return WsSetInput( reader, &text.encoding, &buf.input, NULL, 0, NULL );
}

static HRESULT init_reader( struct channel *channel )
{
    HRESULT hr;
    if (!channel->reader && (hr = WsCreateReader( NULL, 0, &channel->reader, NULL )) != S_OK) return hr;
    return set_input( channel->reader, channel->read_buf, channel->read_size );
}

#define INITIAL_READ_BUFFER_SIZE 4096
static HRESULT receive_message_http( struct channel *channel )
{
    DWORD len, bytes_read, offset = 0, size = INITIAL_READ_BUFFER_SIZE;
    ULONG max_len;
    HRESULT hr;

    prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE,
              &max_len, sizeof(max_len) );

    if ((hr = resize_read_buffer( channel, size )) != S_OK) return hr;
    channel->read_size = 0;
    for (;;)
    {
        if (!WinHttpQueryDataAvailable( channel->u.http.request, &len ))
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        if (!len) break;
        if (channel->read_size + len > max_len) return WS_E_QUOTA_EXCEEDED;
        if ((hr = resize_read_buffer( channel, channel->read_size + len )) != S_OK) return hr;

        if (!WinHttpReadData( channel->u.http.request, channel->read_buf + offset, len, &bytes_read ))
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        if (!bytes_read) break;
        channel->read_size += bytes_read;
        offset += bytes_read;
    }

    return init_reader( channel );
}

static HRESULT receive_message_sock( struct channel *channel, SOCKET socket )
{
    int bytes_read;
    ULONG max_len;
    HRESULT hr;

    prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE,
              &max_len, sizeof(max_len) );

    if ((hr = resize_read_buffer( channel, max_len )) != S_OK) return hr;

    channel->read_size = 0;
    if ((bytes_read = recv( socket, channel->read_buf, max_len, 0 )) < 0)
    {
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }
    channel->read_size = bytes_read;

    return init_reader( channel );
}

static HRESULT receive_message( struct channel *channel )
{
    HRESULT hr;

    if ((hr = connect_channel( channel )) != S_OK) return hr;

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        return receive_message_http( channel );

    case WS_TCP_CHANNEL_BINDING:
        return receive_message_sock( channel, channel->u.tcp.socket );

    case WS_UDP_CHANNEL_BINDING:
        return receive_message_sock( channel, channel->u.udp.socket );

    default:
        ERR( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }
}

HRESULT channel_receive_message( WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    hr = receive_message( channel );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

HRESULT channel_get_reader( WS_CHANNEL *handle, WS_XML_READER **reader )
{
    struct channel *channel = (struct channel *)handle;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    *reader = channel->reader;

    LeaveCriticalSection( &channel->cs );
    return S_OK;
}

static HRESULT read_message( WS_MESSAGE *handle, WS_XML_READER *reader, const WS_ELEMENT_DESCRIPTION *desc,
                             WS_READ_OPTION option, WS_HEAP *heap, void *body, ULONG size )
{
    HRESULT hr;
    if ((hr = WsReadEnvelopeStart( handle, reader, NULL, NULL, NULL )) != S_OK) return hr;
    if ((hr = WsReadBody( handle, desc, option, heap, body, size, NULL )) != S_OK) return hr;
    return WsReadEnvelopeEnd( handle, NULL );
}

/**************************************************************************
 *          WsReceiveMessage		[webservices.@]
 */
HRESULT WINAPI WsReceiveMessage( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION **desc,
                                 ULONG count, WS_RECEIVE_OPTION option, WS_READ_OPTION read_option, WS_HEAP *heap,
                                 void *value, ULONG size, ULONG *index, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %u %08x %08x %p %p %u %p %p %p\n", handle, msg, desc, count, option, read_option, heap,
           value, size, index, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );
    if (index)
    {
        FIXME( "index parameter not supported\n" );
        return E_NOTIMPL;
    }
    if (count != 1)
    {
        FIXME( "no support for multiple descriptions\n" );
        return E_NOTIMPL;
    }
    if (option != WS_RECEIVE_REQUIRED_MESSAGE)
    {
        FIXME( "receive option %08x not supported\n", option );
        return E_NOTIMPL;
    }

    if (!channel || !msg || !desc || !count) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = receive_message( channel )) != S_OK) goto done;
    hr = read_message( msg, channel->reader, desc[0]->bodyElementDescription, read_option, heap, value, size );

done:
    LeaveCriticalSection( &channel->cs );
    return hr;
}

/**************************************************************************
 *          WsReadMessageStart		[webservices.@]
 */
HRESULT WINAPI WsReadMessageStart( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx,
                                   WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = receive_message( channel )) == S_OK)
    {
        hr = WsReadEnvelopeStart( msg, channel->reader, NULL, NULL, NULL );
    }

    LeaveCriticalSection( &channel->cs );
    return hr;
}

/**************************************************************************
 *          WsReadMessageEnd		[webservices.@]
 */
HRESULT WINAPI WsReadMessageEnd( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx,
                                 WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    hr = WsReadEnvelopeEnd( msg, NULL );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

/**************************************************************************
 *          WsWriteMessageStart		[webservices.@]
 */
HRESULT WINAPI WsWriteMessageStart( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx,
                                    WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = init_writer( channel )) != S_OK) goto done;
    if ((hr = WsAddressMessage( msg, &channel->addr, NULL )) != S_OK) goto done;
    hr = WsWriteEnvelopeStart( msg, channel->writer, NULL, NULL, NULL );

done:
    LeaveCriticalSection( &channel->cs );
    return hr;
}

/**************************************************************************
 *          WsWriteMessageEnd		[webservices.@]
 */
HRESULT WINAPI WsWriteMessageEnd( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx,
                                  WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = WsWriteEnvelopeEnd( msg, NULL )) == S_OK) hr = send_message( channel, msg );

    LeaveCriticalSection( &channel->cs );
    return hr;
}

HRESULT channel_accept_tcp( SOCKET socket, WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((channel->u.tcp.socket = accept( socket, NULL, NULL )) == -1)
    {
        LeaveCriticalSection( &channel->cs );
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    LeaveCriticalSection( &channel->cs );
    return S_OK;
}

static HRESULT sock_wait( SOCKET socket )
{
    fd_set read;

    FD_ZERO( &read );
    FD_SET( socket, &read );
    if (select( socket + 1, &read, NULL, NULL, NULL ) < 0) return HRESULT_FROM_WIN32( WSAGetLastError() );
    return S_OK;
}

HRESULT channel_accept_udp( SOCKET socket, WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = sock_wait( socket )) == S_OK) channel->u.udp.socket = socket;

    LeaveCriticalSection( &channel->cs );
    return hr;
}
