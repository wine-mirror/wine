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
#include "rpc.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"
#include "sock.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc channel_props[] =
{
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE */
    { sizeof(UINT64), FALSE },                              /* WS_CHANNEL_PROPERTY_MAX_STREAMED_MESSAGE_SIZE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_STREAMED_START_SIZE */
    { sizeof(ULONG), FALSE },                               /* WS_CHANNEL_PROPERTY_MAX_STREAMED_FLUSH_SIZE */
    { sizeof(WS_ENCODING), TRUE },                          /* WS_CHANNEL_PROPERTY_ENCODING */
    { sizeof(WS_ENVELOPE_VERSION), FALSE },                 /* WS_CHANNEL_PROPERTY_ENVELOPE_VERSION */
    { sizeof(WS_ADDRESSING_VERSION), FALSE },               /* WS_CHANNEL_PROPERTY_ADDRESSING_VERSION */
    { sizeof(ULONG), TRUE },                                /* WS_CHANNEL_PROPERTY_MAX_SESSION_DICTIONARY_SIZE */
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

struct task
{
    struct list   entry;
    void        (*proc)( struct task * );
};

struct queue
{
    CRITICAL_SECTION cs;
    HANDLE           wait;
    HANDLE           cancel;
    HANDLE           ready;
    struct list      tasks;
};

static struct task *dequeue_task( struct queue *queue )
{
    struct task *task;

    EnterCriticalSection( &queue->cs );
    TRACE( "%u tasks queued\n", list_count( &queue->tasks ) );
    task = LIST_ENTRY( list_head( &queue->tasks ), struct task, entry );
    if (task) list_remove( &task->entry );
    LeaveCriticalSection( &queue->cs );

    TRACE( "returning task %p\n", task );
    return task;
}

static void CALLBACK queue_runner( TP_CALLBACK_INSTANCE *instance, void *ctx )
{
    struct queue *queue = ctx;
    HANDLE handles[] = { queue->wait, queue->cancel };

    SetEvent( queue->ready );
    for (;;)
    {
        DWORD err = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
        switch (err)
        {
        case WAIT_OBJECT_0:
        {
            struct task *task;
            while ((task = dequeue_task( queue )))
            {
                task->proc( task );
                free( task );
            }
            break;
        }
        case WAIT_OBJECT_0 + 1:
            TRACE( "cancelled\n" );
            SetEvent( queue->ready );
            return;

        default:
            ERR( "wait failed %lu\n", err );
            return;
        }
    }
}

static HRESULT start_queue( struct queue *queue )
{
    HRESULT hr = E_OUTOFMEMORY;

    if (queue->wait) return S_OK;
    list_init( &queue->tasks );
    if (!(queue->wait = CreateEventW( NULL, FALSE, FALSE, NULL ))) goto error;
    if (!(queue->cancel = CreateEventW( NULL, FALSE, FALSE, NULL ))) goto error;
    if (!(queue->ready = CreateEventW( NULL, FALSE, FALSE, NULL ))) goto error;
    if (!TrySubmitThreadpoolCallback( queue_runner, queue, NULL )) hr = HRESULT_FROM_WIN32( GetLastError() );
    else
    {
        WaitForSingleObject( queue->ready, INFINITE );
        return S_OK;
    }

error:
    CloseHandle( queue->wait );
    queue->wait   = NULL;
    CloseHandle( queue->cancel );
    queue->cancel = NULL;
    CloseHandle( queue->ready );
    queue->ready  = NULL;
    return hr;
}

static HRESULT queue_task( struct queue *queue, struct task *task )
{
    HRESULT hr;
    if ((hr = start_queue( queue )) != S_OK) return hr;

    EnterCriticalSection( &queue->cs );
    TRACE( "queueing task %p\n", task );
    list_add_tail( &queue->tasks, &task->entry );
    LeaveCriticalSection( &queue->cs );

    SetEvent( queue->wait );
    return WS_S_ASYNC;
};

enum session_state
{
    SESSION_STATE_UNINITIALIZED,
    SESSION_STATE_SETUP_COMPLETE,
    SESSION_STATE_SHUTDOWN,
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
    WS_ENCODING             encoding;
    enum session_state      session_state;
    struct dictionary       dict_send;
    struct dictionary       dict_recv;
    struct queue            send_q;
    struct queue            recv_q;
    union
    {
        struct
        {
            HINTERNET session;
            HINTERNET connect;
            HINTERNET request;
            WCHAR    *path;
            DWORD     flags;
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
    char                   *send_buf;
    ULONG                   send_buflen;
    ULONG                   send_size;
    ULONG                   dict_size;
    ULONG                   prop_count;
    struct prop             prop[ARRAY_SIZE( channel_props )];
};

#define CHANNEL_MAGIC (('C' << 24) | ('H' << 16) | ('A' << 8) | 'N')

static struct channel *alloc_channel(void)
{
    static const ULONG count = ARRAY_SIZE( channel_props );
    struct channel *ret;
    ULONG size = sizeof(*ret) + prop_size( channel_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;

    ret->magic      = CHANNEL_MAGIC;
    InitializeCriticalSectionEx( &ret->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    InitializeCriticalSectionEx( &ret->send_q.cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    InitializeCriticalSectionEx( &ret->recv_q.cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": channel.cs");
    ret->send_q.cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": channel.send_q.cs");
    ret->recv_q.cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": channel.recv_q.cs");

    prop_init( channel_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void clear_addr( WS_ENDPOINT_ADDRESS *addr )
{
    free( addr->url.chars );
    addr->url.chars  = NULL;
    addr->url.length = 0;
}

static void clear_queue( struct queue *queue )
{
    struct list *ptr;

    SetEvent( queue->cancel );
    WaitForSingleObject( queue->ready, INFINITE );

    while ((ptr = list_head( &queue->tasks )))
    {
        struct task *task = LIST_ENTRY( ptr, struct task, entry );
        list_remove( &task->entry );
        free( task );
    }

    CloseHandle( queue->wait );
    queue->wait   = NULL;
    CloseHandle( queue->cancel );
    queue->cancel = NULL;
    CloseHandle( queue->ready );
    queue->ready   = NULL;
}

static void abort_channel( struct channel *channel )
{
    clear_queue( &channel->send_q );
    clear_queue( &channel->recv_q );
}

/**************************************************************************
 *          WsAbortChannel		[webservices.@]
 */
HRESULT WINAPI WsAbortChannel( WS_CHANNEL *handle, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p %p\n", handle, error );

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    abort_channel( channel );

    LeaveCriticalSection( &channel->cs );
    return S_OK;
}

static void reset_channel( struct channel *channel )
{
    channel->state         = WS_CHANNEL_STATE_CREATED;
    channel->session_state = SESSION_STATE_UNINITIALIZED;
    clear_addr( &channel->addr );
    init_dict( &channel->dict_send, channel->dict_size );
    init_dict( &channel->dict_recv, channel->dict_size );
    channel->msg           = NULL;
    channel->read_size     = 0;
    channel->send_size     = 0;

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        WinHttpCloseHandle( channel->u.http.request );
        channel->u.http.request = NULL;
        WinHttpCloseHandle( channel->u.http.connect );
        channel->u.http.connect = NULL;
        WinHttpCloseHandle( channel->u.http.session );
        channel->u.http.session = NULL;
        free( channel->u.http.path );
        channel->u.http.path    = NULL;
        channel->u.http.flags   = 0;
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

static void free_header_mappings( WS_HTTP_HEADER_MAPPING **mappings, ULONG count )
{
    ULONG i;
    for (i = 0; i < count; i++) free( mappings[i] );
    free( mappings );
}

static void free_message_mapping( const WS_HTTP_MESSAGE_MAPPING *mapping )
{
    free_header_mappings( mapping->requestHeaderMappings, mapping->requestHeaderMappingCount );
    free_header_mappings( mapping->responseHeaderMappings, mapping->responseHeaderMappingCount );
}

static void free_props( struct channel *channel )
{
    struct prop *prop = &channel->prop[WS_CHANNEL_PROPERTY_HTTP_MESSAGE_MAPPING];
    WS_HTTP_MESSAGE_MAPPING *mapping = (WS_HTTP_MESSAGE_MAPPING *)prop->value;
    free_message_mapping( mapping );
}

static void free_channel( struct channel *channel )
{
    abort_channel( channel );
    reset_channel( channel );

    WsFreeWriter( channel->writer );
    WsFreeReader( channel->reader );

    free( channel->read_buf );
    free( channel->send_buf );
    free_props( channel );

    channel->send_q.cs.DebugInfo->Spare[0] = 0;
    channel->recv_q.cs.DebugInfo->Spare[0] = 0;
    channel->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &channel->send_q.cs );
    DeleteCriticalSection( &channel->recv_q.cs );
    DeleteCriticalSection( &channel->cs );
    free( channel );
}

static WS_HTTP_HEADER_MAPPING *dup_header_mapping( const WS_HTTP_HEADER_MAPPING *src )
{
    WS_HTTP_HEADER_MAPPING *dst;

    if (!(dst = malloc( sizeof(*dst) + src->headerName.length ))) return NULL;

    dst->headerName.bytes     = (BYTE *)(dst + 1);
    memcpy( dst->headerName.bytes, src->headerName.bytes, src->headerName.length );
    dst->headerName.length    = src->headerName.length;
    dst->headerMappingOptions = src->headerMappingOptions;
    return dst;
}

static HRESULT dup_message_mapping( const WS_HTTP_MESSAGE_MAPPING *src, WS_HTTP_MESSAGE_MAPPING *dst )
{
    ULONG i, size;

    size = src->requestHeaderMappingCount * sizeof(*dst->responseHeaderMappings);
    if (!(dst->requestHeaderMappings = malloc( size ))) return E_OUTOFMEMORY;

    for (i = 0; i < src->requestHeaderMappingCount; i++)
    {
        if (!(dst->requestHeaderMappings[i] = dup_header_mapping( src->requestHeaderMappings[i] )))
        {
            free_header_mappings( dst->requestHeaderMappings, i );
            return E_OUTOFMEMORY;
        }
    }

    size = src->responseHeaderMappingCount * sizeof(*dst->responseHeaderMappings);
    if (!(dst->responseHeaderMappings = malloc( size )))
    {
        free( dst->responseHeaderMappings );
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < src->responseHeaderMappingCount; i++)
    {
        if (!(dst->responseHeaderMappings[i] = dup_header_mapping( src->responseHeaderMappings[i] )))
        {
            free_header_mappings( dst->responseHeaderMappings, i );
            return E_OUTOFMEMORY;
        }
    }

    dst->requestMappingOptions      = src->requestMappingOptions;
    dst->responseMappingOptions     = src->responseMappingOptions;
    dst->requestHeaderMappingCount  = src->requestHeaderMappingCount;
    dst->responseHeaderMappingCount = src->responseHeaderMappingCount;
    return S_OK;
}

static HRESULT create_channel( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
                               const WS_CHANNEL_PROPERTY *properties, ULONG count, struct channel **ret )
{
    struct channel *channel;
    ULONG i, msg_size = 65536;
    WS_ENVELOPE_VERSION env_version = WS_ENVELOPE_VERSION_SOAP_1_2;
    WS_ADDRESSING_VERSION addr_version = WS_ADDRESSING_VERSION_1_0;
    HRESULT hr;

    if (!(channel = alloc_channel())) return E_OUTOFMEMORY;

    prop_set( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE,
              &msg_size, sizeof(msg_size) );
    prop_set( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION,
              &env_version, sizeof(env_version) );
    prop_set( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_ADDRESSING_VERSION,
              &addr_version, sizeof(addr_version) );

    channel->type    = type;
    channel->binding = binding;

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        channel->encoding = WS_ENCODING_XML_UTF8;
        break;

    case WS_TCP_CHANNEL_BINDING:
        channel->u.tcp.socket = -1;
        channel->encoding     = WS_ENCODING_XML_BINARY_SESSION_1;
        channel->dict_size    = 2048;
        channel->dict_send.str_bytes_max = channel->dict_size;
        channel->dict_recv.str_bytes_max = channel->dict_size;
        break;

    case WS_UDP_CHANNEL_BINDING:
        channel->u.udp.socket = -1;
        channel->encoding     = WS_ENCODING_XML_UTF8;
        break;

    default: break;
    }

    for (i = 0; i < count; i++)
    {
        const WS_CHANNEL_PROPERTY *prop = &properties[i];

        TRACE( "property id %u value %p size %lu\n", prop->id, prop->value, prop->valueSize );
        if (prop->valueSize == sizeof(ULONG) && prop->value) TRACE( " value %#lx\n", *(ULONG *)prop->value );

        switch (prop->id)
        {
        case WS_CHANNEL_PROPERTY_ENCODING:
            if (!prop->value || prop->valueSize != sizeof(channel->encoding))
            {
                free_channel( channel );
                return E_INVALIDARG;
            }
            channel->encoding = *(WS_ENCODING *)prop->value;
            break;

        case WS_CHANNEL_PROPERTY_HTTP_MESSAGE_MAPPING:
        {
            const WS_HTTP_MESSAGE_MAPPING *src = (WS_HTTP_MESSAGE_MAPPING *)prop->value;
            WS_HTTP_MESSAGE_MAPPING dst;

            if (!prop->value || prop->valueSize != sizeof(*src))
            {
                free_channel( channel );
                return E_INVALIDARG;
            }

            if ((hr = dup_message_mapping( src, &dst )) != S_OK) return hr;

            if ((hr = prop_set( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_HTTP_MESSAGE_MAPPING, &dst,
                                sizeof(dst) )) != S_OK)
            {
                free_message_mapping( &dst );
                free_channel( channel );
                return hr;
            }
            break;

        }
        case WS_CHANNEL_PROPERTY_MAX_SESSION_DICTIONARY_SIZE:
            if (channel->binding != WS_TCP_CHANNEL_BINDING || !prop->value || prop->valueSize != sizeof(channel->dict_size))
            {
                free_channel( channel );
                return E_INVALIDARG;
            }

            channel->dict_size = *(ULONG *)prop->value;
            channel->dict_send.str_bytes_max = channel->dict_size;
            channel->dict_recv.str_bytes_max = channel->dict_size;
            break;

        default:
            if ((hr = prop_set( channel->prop, channel->prop_count, prop->id, prop->value, prop->valueSize )) != S_OK)
            {
                free_channel( channel );
                return hr;
            }
            break;
        }
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

    TRACE( "%u %u %p %lu %p %p %p\n", type, binding, properties, count, desc, handle, error );
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

    TRACE( "created %p\n", channel );
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

    TRACE( "%p %p %lu %p %p\n", listener_handle, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!listener_handle || !handle) return E_INVALIDARG;

    if ((hr = WsGetListenerProperty( listener_handle, WS_LISTENER_PROPERTY_CHANNEL_TYPE, &type,
                                     sizeof(type), NULL )) != S_OK) return hr;

    if ((hr = WsGetListenerProperty( listener_handle, WS_LISTENER_PROPERTY_CHANNEL_BINDING, &binding,
                                     sizeof(binding), NULL )) != S_OK) return hr;

    if ((hr = create_channel( type, binding, properties, count, &channel )) != S_OK) return hr;

    TRACE( "created %p\n", channel );
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
    HRESULT hr = S_OK;

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
        hr = WS_E_INVALID_OPERATION;
    else
    {
        abort_channel( channel );
        reset_channel( channel );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetChannelProperty		[webservices.@]
 */
HRESULT WINAPI WsGetChannelProperty( WS_CHANNEL *handle, WS_CHANNEL_PROPERTY_ID id, void *buf,
                                     ULONG size, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
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

    case WS_CHANNEL_PROPERTY_ENCODING:
        if (!buf || size != sizeof(channel->encoding)) hr = E_INVALIDARG;
        else *(WS_ENCODING *)buf = channel->encoding;
        break;

    case WS_CHANNEL_PROPERTY_STATE:
        if (!buf || size != sizeof(channel->state)) hr = E_INVALIDARG;
        else *(WS_CHANNEL_STATE *)buf = channel->state;
        break;

    case WS_CHANNEL_PROPERTY_MAX_SESSION_DICTIONARY_SIZE:
        if (channel->binding != WS_TCP_CHANNEL_BINDING || !buf || size != sizeof(channel->dict_size))
            hr = E_INVALIDARG;
        else
            *(ULONG *)buf = channel->dict_size;
        break;

    default:
        hr = prop_get( channel->prop, channel->prop_count, id, buf, size );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
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

    TRACE( "%p %u %p %lu %p\n", handle, id, value, size, error );
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
    TRACE( "returning %#lx\n", hr );
    return hr;
}

enum frame_record_type
{
    FRAME_RECORD_TYPE_VERSION,
    FRAME_RECORD_TYPE_MODE,
    FRAME_RECORD_TYPE_VIA,
    FRAME_RECORD_TYPE_KNOWN_ENCODING,
    FRAME_RECORD_TYPE_EXTENSIBLE_ENCODING,
    FRAME_RECORD_TYPE_UNSIZED_ENVELOPE,
    FRAME_RECORD_TYPE_SIZED_ENVELOPE,
    FRAME_RECORD_TYPE_END,
    FRAME_RECORD_TYPE_FAULT,
    FRAME_RECORD_TYPE_UPGRADE_REQUEST,
    FRAME_RECORD_TYPE_UPGRADE_RESPONSE,
    FRAME_RECORD_TYPE_PREAMBLE_ACK,
    FRAME_RECORD_TYPE_PREAMBLE_END,
};

static HRESULT send_byte( SOCKET socket, BYTE byte )
{
    int count = send( socket, (char *)&byte, 1, 0 );
    if (count < 0) return HRESULT_FROM_WIN32( WSAGetLastError() );
    if (count != 1) return WS_E_OTHER;
    return S_OK;
}

struct async
{
    HRESULT hr;
    HANDLE  done;
};

static void CALLBACK async_callback( HRESULT hr, WS_CALLBACK_MODEL model, void *state )
{
    struct async *async = state;
    async->hr = hr;
    SetEvent( async->done );
}

static void async_init( struct async *async, WS_ASYNC_CONTEXT *ctx )
{
    async->done = CreateEventW( NULL, FALSE, FALSE, NULL );
    async->hr = E_FAIL;
    ctx->callback = async_callback;
    ctx->callbackState = async;
}

static HRESULT async_wait( struct async *async )
{
    DWORD err;
    if ((err = WaitForSingleObject( async->done, INFINITE )) == WAIT_OBJECT_0) return async->hr;
    return HRESULT_FROM_WIN32( err );
}

static HRESULT shutdown_session( struct channel *channel )
{
    HRESULT hr;

    if ((channel->type != WS_CHANNEL_TYPE_OUTPUT_SESSION &&
         channel->type != WS_CHANNEL_TYPE_DUPLEX_SESSION) ||
         channel->session_state >= SESSION_STATE_SHUTDOWN) return WS_E_INVALID_OPERATION;

    switch (channel->binding)
    {
    case WS_TCP_CHANNEL_BINDING:
        if ((hr = send_byte( channel->u.tcp.socket, FRAME_RECORD_TYPE_END )) != S_OK) return hr;
        channel->session_state = SESSION_STATE_SHUTDOWN;
        return S_OK;

    default:
        FIXME( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }
}

struct shutdown_session
{
    struct task      task;
    struct channel  *channel;
    WS_ASYNC_CONTEXT ctx;
};

static void shutdown_session_proc( struct task *task )
{
    struct shutdown_session *s = (struct shutdown_session *)task;
    HRESULT hr;

    hr = shutdown_session( s->channel );

    TRACE( "calling %p(%#lx)\n", s->ctx.callback, hr );
    s->ctx.callback( hr, WS_LONG_CALLBACK, s->ctx.callbackState );
    TRACE( "%p returned\n", s->ctx.callback );
}

static HRESULT queue_shutdown_session( struct channel *channel, const WS_ASYNC_CONTEXT *ctx )
{
    struct shutdown_session *s;

    if (!(s = malloc( sizeof(*s) ))) return E_OUTOFMEMORY;
    s->task.proc = shutdown_session_proc;
    s->channel   = channel;
    s->ctx       = *ctx;
    return queue_task( &channel->send_q, &s->task );
}

static HRESULT check_state( struct channel *channel, WS_CHANNEL_STATE state_expected )
{
    if (channel->state == WS_CHANNEL_STATE_FAULTED) return WS_E_OBJECT_FAULTED;
    if (channel->state != state_expected) return WS_E_INVALID_OPERATION;
    return S_OK;
}

HRESULT WINAPI WsShutdownSessionChannel( WS_CHANNEL *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_shutdown_session( channel, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static void close_channel( struct channel *channel )
{
    reset_channel( channel );
    channel->state = WS_CHANNEL_STATE_CLOSED;
}

struct close_channel
{
    struct task      task;
    struct channel  *channel;
    WS_ASYNC_CONTEXT ctx;
};

static void close_channel_proc( struct task *task )
{
    struct close_channel *c = (struct close_channel *)task;

    close_channel( c->channel );

    TRACE( "calling %p(S_OK)\n", c->ctx.callback );
    c->ctx.callback( S_OK, WS_LONG_CALLBACK, c->ctx.callbackState );
    TRACE( "%p returned\n", c->ctx.callback );
}

static HRESULT queue_close_channel( struct channel *channel, const WS_ASYNC_CONTEXT *ctx )
{
    struct close_channel *c;

    if (!(c = malloc( sizeof(*c) ))) return E_OUTOFMEMORY;
    c->task.proc = close_channel_proc;
    c->channel   = channel;
    c->ctx       = *ctx;
    return queue_task( &channel->send_q, &c->task );
}

/**************************************************************************
 *          WsCloseChannel		[webservices.@]
 */
HRESULT WINAPI WsCloseChannel( WS_CHANNEL *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_close_channel( channel, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT parse_http_url( const WCHAR *url, ULONG len, URL_COMPONENTS *uc )
{
    HRESULT hr = E_OUTOFMEMORY;
    WCHAR *tmp;
    DWORD err;

    memset( uc, 0, sizeof(*uc) );
    uc->dwStructSize      = sizeof(*uc);
    uc->dwHostNameLength  = 128;
    uc->lpszHostName      = malloc( uc->dwHostNameLength * sizeof(WCHAR) );
    uc->dwUrlPathLength   = 128;
    uc->lpszUrlPath       = malloc( uc->dwUrlPathLength * sizeof(WCHAR) );
    uc->dwExtraInfoLength = 128;
    uc->lpszExtraInfo     = malloc( uc->dwExtraInfoLength * sizeof(WCHAR) );
    if (!uc->lpszHostName || !uc->lpszUrlPath || !uc->lpszExtraInfo) goto error;

    if (!WinHttpCrackUrl( url, len, ICU_DECODE, uc ))
    {
        if ((err = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
        {
            hr = HRESULT_FROM_WIN32( err );
            goto error;
        }
        if (!(tmp = realloc( uc->lpszHostName, uc->dwHostNameLength * sizeof(WCHAR) ))) goto error;
        uc->lpszHostName = tmp;
        if (!(tmp = realloc( uc->lpszUrlPath, uc->dwUrlPathLength * sizeof(WCHAR) ))) goto error;
        uc->lpszUrlPath = tmp;
        if (!(tmp = realloc( uc->lpszExtraInfo, uc->dwExtraInfoLength * sizeof(WCHAR) ))) goto error;
        uc->lpszExtraInfo = tmp;
        WinHttpCrackUrl( url, len, ICU_DECODE, uc );
    }

    return S_OK;

error:
    free( uc->lpszHostName );
    free( uc->lpszUrlPath );
    free( uc->lpszExtraInfo );
    return hr;
}

static HRESULT open_channel_http( struct channel *channel )
{
    HINTERNET ses = NULL, con = NULL;
    URL_COMPONENTS uc;
    HRESULT hr;

    if (channel->u.http.connect) return S_OK;

    if ((hr = parse_http_url( channel->addr.url.chars, channel->addr.url.length, &uc )) != S_OK) return hr;
    if (!(channel->u.http.path = malloc( (uc.dwUrlPathLength + uc.dwExtraInfoLength + 1) * sizeof(WCHAR) )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    else
    {
        lstrcpyW( channel->u.http.path, uc.lpszUrlPath );
        if (uc.dwExtraInfoLength) lstrcatW( channel->u.http.path, uc.lpszExtraInfo );
    }

    channel->u.http.flags = WINHTTP_FLAG_REFRESH;
    switch (uc.nScheme)
    {
    case INTERNET_SCHEME_HTTP: break;
    case INTERNET_SCHEME_HTTPS:
        channel->u.http.flags |= WINHTTP_FLAG_SECURE;
        break;

    default:
        hr = WS_E_INVALID_ENDPOINT_URL;
        goto done;
    }

    if (!(ses = WinHttpOpen( L"MS-WebServices/1.0", 0, NULL, NULL, 0 )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto done;
    }
    if (!(con = WinHttpConnect( ses, uc.lpszHostName, uc.nPort, 0 )))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto done;
    }

    channel->u.http.session = ses;
    channel->u.http.connect = con;

done:
    if (hr != S_OK)
    {
        WinHttpCloseHandle( con );
        WinHttpCloseHandle( ses );
    }
    free( uc.lpszHostName );
    free( uc.lpszUrlPath );
    free( uc.lpszExtraInfo );
    return hr;
}

static HRESULT open_channel_tcp( struct channel *channel )
{
    struct sockaddr_storage storage;
    struct sockaddr *addr = (struct sockaddr *)&storage;
    BOOL nodelay = FALSE;
    int addr_len;
    WS_URL_SCHEME_TYPE scheme;
    WCHAR *host;
    USHORT port;
    HRESULT hr;

    if (channel->u.tcp.socket != -1) return S_OK;

    if ((hr = parse_url( &channel->addr.url, &scheme, &host, &port )) != S_OK) return hr;
    if (scheme != WS_URL_NETTCP_SCHEME_TYPE)
    {
        free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len, 0 );
    free( host );
    if (hr != S_OK) return hr;

    if ((channel->u.tcp.socket = socket( addr->sa_family, SOCK_STREAM, 0 )) == -1)
        return HRESULT_FROM_WIN32( WSAGetLastError() );

    if (connect( channel->u.tcp.socket, addr, addr_len ) < 0)
    {
        closesocket( channel->u.tcp.socket );
        channel->u.tcp.socket = -1;
        return HRESULT_FROM_WIN32( WSAGetLastError() );
    }

    prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_NO_DELAY, &nodelay, sizeof(nodelay) );
    setsockopt( channel->u.tcp.socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay) );
    return S_OK;
}

static HRESULT open_channel_udp( struct channel *channel )
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
        free( host );
        return WS_E_INVALID_ENDPOINT_URL;
    }

    winsock_init();

    hr = resolve_hostname( host, port, addr, &addr_len, 0 );
    free( host );
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

static HRESULT open_channel( struct channel *channel, const WS_ENDPOINT_ADDRESS *endpoint )
{
    HRESULT hr;

    if (endpoint->headers || endpoint->extensions || endpoint->identity)
    {
        FIXME( "headers, extensions or identity not supported\n" );
        return E_NOTIMPL;
    }

    TRACE( "endpoint %s\n", debugstr_wn(endpoint->url.chars, endpoint->url.length) );

    if (!(channel->addr.url.chars = malloc( endpoint->url.length * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    memcpy( channel->addr.url.chars, endpoint->url.chars, endpoint->url.length * sizeof(WCHAR) );
    channel->addr.url.length = endpoint->url.length;

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        hr = open_channel_http( channel );
        break;

    case WS_TCP_CHANNEL_BINDING:
        hr = open_channel_tcp( channel );
        break;

    case WS_UDP_CHANNEL_BINDING:
        hr = open_channel_udp( channel );
        break;

    default:
        ERR( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }

    if (hr == S_OK) channel->state = WS_CHANNEL_STATE_OPEN;
    return hr;
}

struct open_channel
{
    struct task                task;
    struct channel            *channel;
    const WS_ENDPOINT_ADDRESS *endpoint;
    WS_ASYNC_CONTEXT           ctx;
};

static void open_channel_proc( struct task *task )
{
    struct open_channel *o = (struct open_channel *)task;
    HRESULT hr;

    hr = open_channel( o->channel, o->endpoint );

    TRACE( "calling %p(%#lx)\n", o->ctx.callback, hr );
    o->ctx.callback( hr, WS_LONG_CALLBACK, o->ctx.callbackState );
    TRACE( "%p returned\n", o->ctx.callback );
}

static HRESULT queue_open_channel( struct channel *channel, const WS_ENDPOINT_ADDRESS *endpoint,
                                   const WS_ASYNC_CONTEXT *ctx )
{
    struct open_channel *o;

    if (!(o = malloc( sizeof(*o) ))) return E_OUTOFMEMORY;
    o->task.proc = open_channel_proc;
    o->channel   = channel;
    o->endpoint  = endpoint;
    o->ctx       = *ctx;
    return queue_task( &channel->send_q, &o->task );
}

/**************************************************************************
 *          WsOpenChannel		[webservices.@]
 */
HRESULT WINAPI WsOpenChannel( WS_CHANNEL *handle, const WS_ENDPOINT_ADDRESS *endpoint, const WS_ASYNC_CONTEXT *ctx,
                              WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, endpoint, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !endpoint) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_CREATED )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_open_channel( channel, endpoint, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT send_message_http( HINTERNET request, BYTE *data, ULONG len )
{
    if (!WinHttpSendRequest( request, NULL, 0, data, len, len, 0 ))
        return HRESULT_FROM_WIN32( GetLastError() );

    if (!WinHttpReceiveResponse( request, NULL ))
        return HRESULT_FROM_WIN32( GetLastError() );
    return S_OK;
}

static ULONG get_max_buffer_size( struct channel *channel )
{
    ULONG size;
    prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE, &size, sizeof(size) );
    return size;
}

static HRESULT write_bytes( struct channel *channel, BYTE *bytes, ULONG len )
{
    if (!channel->send_buf)
    {
        channel->send_buflen = get_max_buffer_size( channel );
        if (!(channel->send_buf = malloc( channel->send_buflen ))) return E_OUTOFMEMORY;
    }
    if (channel->send_size + len >= channel->send_buflen) return WS_E_QUOTA_EXCEEDED;

    memcpy( channel->send_buf + channel->send_size, bytes, len );
    channel->send_size += len;
    return S_OK;
}

static inline HRESULT write_byte( struct channel *channel, BYTE byte )
{
    return write_bytes( channel, &byte, 1 );
}

static HRESULT write_size( struct channel *channel, ULONG size )
{
    HRESULT hr;
    if (size < 0x80) return write_byte( channel, size );
    if ((hr = write_byte( channel, (size & 0x7f) | 0x80 )) != S_OK) return hr;
    if ((size >>= 7) < 0x80) return write_byte( channel, size );
    if ((hr = write_byte( channel, (size & 0x7f) | 0x80 )) != S_OK) return hr;
    if ((size >>= 7) < 0x80) return write_byte( channel, size );
    if ((hr = write_byte( channel, (size & 0x7f) | 0x80 )) != S_OK) return hr;
    if ((size >>= 7) < 0x80) return write_byte( channel, size );
    if ((hr = write_byte( channel, (size & 0x7f) | 0x80 )) != S_OK) return hr;
    if ((size >>= 7) < 0x08) return write_byte( channel, size );
    return E_INVALIDARG;
}

static inline ULONG size_length( ULONG size )
{
    if (size < 0x80) return 1;
    if (size < 0x4000) return 2;
    if (size < 0x200000) return 3;
    if (size < 0x10000000) return 4;
    return 5;
}

static ULONG string_table_size( const struct dictionary *dict )
{
    ULONG i, size = 0;
    for (i = 0; i < dict->dict.stringCount; i++)
    {
        if (dict->sequence[i] == dict->current_sequence)
            size += size_length( dict->dict.strings[i].length ) + dict->dict.strings[i].length;
    }
    return size;
}

static HRESULT write_string_table( struct channel *channel, const struct dictionary *dict )
{
    ULONG i;
    HRESULT hr;
    for (i = 0; i < dict->dict.stringCount; i++)
    {
        if (dict->sequence[i] != dict->current_sequence) continue;
        if ((hr = write_size( channel, dict->dict.strings[i].length )) != S_OK) return hr;
        if ((hr = write_bytes( channel, dict->dict.strings[i].bytes, dict->dict.strings[i].length )) != S_OK) return hr;
    }
    return S_OK;
}

static HRESULT string_to_utf8( const WS_STRING *str, unsigned char **ret, int *len )
{
    *len = WideCharToMultiByte( CP_UTF8, 0, str->chars, str->length, NULL, 0, NULL, NULL );
    if (!(*ret = malloc( *len ))) return E_OUTOFMEMORY;
    WideCharToMultiByte( CP_UTF8, 0, str->chars, str->length, (char *)*ret, *len, NULL, NULL );
    return S_OK;
}

enum session_mode
{
    SESSION_MODE_INVALID    = 0,
    SESSION_MODE_SINGLETON  = 1,
    SESSION_MODE_DUPLEX     = 2,
    SESSION_MODE_SIMPLEX    = 3,
};

static enum session_mode map_channel_type( struct channel *channel )
{
    switch (channel->type)
    {
    case WS_CHANNEL_TYPE_DUPLEX_SESSION: return SESSION_MODE_DUPLEX;
    default:
        FIXME( "unhandled channel type %#x\n", channel->type );
        return SESSION_MODE_INVALID;
    }
}

enum known_encoding
{
    KNOWN_ENCODING_SOAP11_UTF8           = 0x00,
    KNOWN_ENCODING_SOAP11_UTF16          = 0x01,
    KNOWN_ENCODING_SOAP11_UTF16LE        = 0x02,
    KNOWN_ENCODING_SOAP12_UTF8           = 0x03,
    KNOWN_ENCODING_SOAP12_UTF16          = 0x04,
    KNOWN_ENCODING_SOAP12_UTF16LE        = 0x05,
    KNOWN_ENCODING_SOAP12_MTOM           = 0x06,
    KNOWN_ENCODING_SOAP12_BINARY         = 0x07,
    KNOWN_ENCODING_SOAP12_BINARY_SESSION = 0x08,
};

static enum known_encoding map_channel_encoding( struct channel *channel )
{
    WS_ENVELOPE_VERSION version;

    prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION, &version, sizeof(version) );

    switch (version)
    {
    case WS_ENVELOPE_VERSION_SOAP_1_1:
        switch (channel->encoding)
        {
        case WS_ENCODING_XML_UTF8:      return KNOWN_ENCODING_SOAP11_UTF8;
        case WS_ENCODING_XML_UTF16LE:   return KNOWN_ENCODING_SOAP11_UTF16LE;
        default:
            FIXME( "unhandled version/encoding %u/%u\n", version, channel->encoding );
            return 0;
        }
    case WS_ENVELOPE_VERSION_SOAP_1_2:
        switch (channel->encoding)
        {
        case WS_ENCODING_XML_UTF8:              return KNOWN_ENCODING_SOAP12_UTF8;
        case WS_ENCODING_XML_UTF16LE:           return KNOWN_ENCODING_SOAP12_UTF16LE;
        case WS_ENCODING_XML_BINARY_1:          return KNOWN_ENCODING_SOAP12_BINARY;
        case WS_ENCODING_XML_BINARY_SESSION_1:  return KNOWN_ENCODING_SOAP12_BINARY_SESSION;
        default:
            FIXME( "unhandled version/encoding %u/%u\n", version, channel->encoding );
            return 0;
        }
    default:
        ERR( "unhandled version %u\n", version );
        return 0;
    }
}

#define FRAME_VERSION_MAJOR 1
#define FRAME_VERSION_MINOR 0

static HRESULT write_preamble( struct channel *channel )
{
    unsigned char *url;
    HRESULT hr;
    int len;

    if ((hr = write_byte( channel, FRAME_RECORD_TYPE_VERSION )) != S_OK) return hr;
    if ((hr = write_byte( channel, FRAME_VERSION_MAJOR )) != S_OK) return hr;
    if ((hr = write_byte( channel, FRAME_VERSION_MINOR )) != S_OK) return hr;

    if ((hr = write_byte( channel, FRAME_RECORD_TYPE_MODE )) != S_OK) return hr;
    if ((hr = write_byte( channel, map_channel_type(channel) )) != S_OK) return hr;

    if ((hr = write_byte( channel, FRAME_RECORD_TYPE_VIA )) != S_OK) return hr;
    if ((hr = string_to_utf8( &channel->addr.url, &url, &len )) != S_OK) return hr;
    if ((hr = write_size( channel, len )) != S_OK) goto done;
    if ((hr = write_bytes( channel, url, len )) != S_OK) goto done;

    if ((hr = write_byte( channel, FRAME_RECORD_TYPE_KNOWN_ENCODING )) != S_OK) goto done;
    if ((hr = write_byte( channel, map_channel_encoding(channel) )) != S_OK) goto done;
    hr = write_byte( channel, FRAME_RECORD_TYPE_PREAMBLE_END );

done:
    free( url );
    return hr;
}

static HRESULT send_bytes( SOCKET socket, char *bytes, int len )
{
    int count = send( socket, bytes, len, 0 );
    if (count < 0) return HRESULT_FROM_WIN32( WSAGetLastError() );
    if (count != len) return WS_E_OTHER;
    return S_OK;
}

static HRESULT send_preamble( struct channel *channel )
{
    HRESULT hr;
    if ((hr = write_preamble( channel )) != S_OK) return hr;
    if ((hr = send_bytes( channel->u.tcp.socket, channel->send_buf, channel->send_size )) != S_OK) return hr;
    channel->send_size = 0;
    return S_OK;
}

static HRESULT receive_bytes( struct channel *channel, unsigned char *bytes, int len )
{
    int count = recv( channel->u.tcp.socket, (char *)bytes, len, 0 );
    if (count < 0) return HRESULT_FROM_WIN32( WSAGetLastError() );
    if (count != len) return WS_E_INVALID_FORMAT;
    return S_OK;
}

static HRESULT receive_preamble_ack( struct channel *channel )
{
    unsigned char byte;
    HRESULT hr;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    if (byte != FRAME_RECORD_TYPE_PREAMBLE_ACK) return WS_E_INVALID_FORMAT;
    channel->session_state = SESSION_STATE_SETUP_COMPLETE;
    return S_OK;
}

static HRESULT write_sized_envelope( struct channel *channel, BYTE *data, ULONG len )
{
    ULONG table_size = string_table_size( &channel->dict_send );
    HRESULT hr;

    if ((hr = write_byte( channel, FRAME_RECORD_TYPE_SIZED_ENVELOPE )) != S_OK) return hr;
    if ((hr = write_size( channel, size_length(table_size) + table_size + len )) != S_OK) return hr;
    if ((hr = write_size( channel, table_size )) != S_OK) return hr;
    if ((hr = write_string_table( channel, &channel->dict_send )) != S_OK) return hr;
    return write_bytes( channel, data, len );
}

static HRESULT send_sized_envelope( struct channel *channel, BYTE *data, ULONG len )
{
    HRESULT hr;
    if ((hr = write_sized_envelope( channel, data, len )) != S_OK) return hr;
    if ((hr = send_bytes( channel->u.tcp.socket, channel->send_buf, channel->send_size )) != S_OK) return hr;
    channel->send_size = 0;
    return S_OK;
}

static HRESULT open_http_request( struct channel *channel, HINTERNET *req )
{
    if ((*req = WinHttpOpenRequest( channel->u.http.connect, L"POST", channel->u.http.path,
                                    NULL, NULL, NULL, channel->u.http.flags ))) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

static HRESULT send_message_bytes( struct channel *channel, WS_MESSAGE *msg )
{
    WS_XML_WRITER *writer;
    WS_BYTES buf;
    HRESULT hr;

    channel->msg = msg;
    WsGetMessageProperty( channel->msg, WS_MESSAGE_PROPERTY_BODY_WRITER, &writer, sizeof(writer), NULL );
    WsGetWriterProperty( writer, WS_XML_WRITER_PROPERTY_BYTES, &buf, sizeof(buf), NULL );

    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        if (channel->u.http.request)
        {
            WinHttpCloseHandle( channel->u.http.request );
            channel->u.http.request = NULL;
        }
        if ((hr = open_http_request( channel, &channel->u.http.request )) != S_OK) return hr;
        if ((hr = message_insert_http_headers( msg, channel->u.http.request )) != S_OK) return hr;
        return send_message_http( channel->u.http.request, buf.bytes, buf.length );

    case WS_TCP_CHANNEL_BINDING:
        if (channel->type & WS_CHANNEL_TYPE_SESSION)
        {
            switch (channel->session_state)
            {
            case SESSION_STATE_UNINITIALIZED:
                if ((hr = send_preamble( channel )) != S_OK) return hr;
                if ((hr = receive_preamble_ack( channel )) != S_OK) return hr;
                /* fall through */

            case SESSION_STATE_SETUP_COMPLETE:
                return send_sized_envelope( channel, buf.bytes, buf.length );

            default:
                ERR( "unhandled session state %u\n", channel->session_state );
                return WS_E_OTHER;
            }
        }
        /* fall through */

    case WS_UDP_CHANNEL_BINDING:
        return WsFlushWriter( writer, 0, NULL, NULL );

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
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    hr = send_message_bytes( channel, msg );
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;

    LeaveCriticalSection( &channel->cs );
    return hr;
}

static HRESULT CALLBACK dict_cb( void *state, const WS_XML_STRING *str, BOOL *found, ULONG *id, WS_ERROR *error )
{
    struct dictionary *dict = state;
    HRESULT hr = S_OK;
    BYTE *bytes;
    int index;

    if ((index = find_string( dict, str->bytes, str->length, id )) == -1)
    {
        *found = TRUE;
        return S_OK;
    }

    if (str->length + dict->str_bytes + 1 > dict->str_bytes_max)
    {
        WARN( "max string bytes exceeded\n" );
        *found = FALSE;
        return hr;
    }

    if (!(bytes = malloc( str->length ))) return E_OUTOFMEMORY;
    memcpy( bytes, str->bytes, str->length );
    if ((hr = insert_string( dict, bytes, str->length, index, id )) == S_OK)
    {
        *found = TRUE;
        return S_OK;
    }
    free( bytes );

    *found = FALSE;
    return hr;
}

static CALLBACK HRESULT write_callback( void *state, const WS_BYTES *buf, ULONG count, const WS_ASYNC_CONTEXT *ctx,
                                        WS_ERROR *error )
{
    SOCKET socket = *(SOCKET *)state;
    if (send( socket, (const char *)buf->bytes, buf->length, 0 ) < 0)
    {
        TRACE( "send failed %u\n", WSAGetLastError() );
    }
    return S_OK;
}

static HRESULT init_writer( struct channel *channel )
{
    WS_XML_WRITER_BUFFER_OUTPUT buf = {{WS_XML_WRITER_OUTPUT_TYPE_BUFFER}};
    WS_XML_WRITER_STREAM_OUTPUT stream = {{WS_XML_WRITER_OUTPUT_TYPE_STREAM}};
    WS_XML_WRITER_TEXT_ENCODING text = {{WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8};
    WS_XML_WRITER_BINARY_ENCODING bin = {{WS_XML_WRITER_ENCODING_TYPE_BINARY}};
    const WS_XML_WRITER_ENCODING *encoding;
    const WS_XML_WRITER_OUTPUT *output;
    WS_XML_WRITER_PROPERTY prop;
    ULONG max_size = (1 << 17);
    HRESULT hr;

    prop.id        = WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE;
    prop.value     = &max_size;
    prop.valueSize = sizeof(max_size);
    if (!channel->writer && (hr = WsCreateWriter( &prop, 1, &channel->writer, NULL )) != S_OK) return hr;

    switch (channel->encoding)
    {
    case WS_ENCODING_XML_UTF8:
        encoding = &text.encoding;
        if (channel->binding == WS_UDP_CHANNEL_BINDING ||
            (channel->binding == WS_TCP_CHANNEL_BINDING && !(channel->type & WS_CHANNEL_TYPE_SESSION)))
        {
            stream.writeCallback      = write_callback;
            stream.writeCallbackState = (channel->binding == WS_UDP_CHANNEL_BINDING) ?
                                        &channel->u.udp.socket : &channel->u.tcp.socket;
            output = &stream.output;
        }
        else output = &buf.output;
        break;

    case WS_ENCODING_XML_BINARY_SESSION_1:
        bin.staticDictionary = (WS_XML_DICTIONARY *)&dict_builtin_static.dict;
        /* fall through */

    case WS_ENCODING_XML_BINARY_1:
        encoding = &bin.encoding;
        output = &buf.output;
        break;

    default:
        FIXME( "unhandled encoding %u\n", channel->encoding );
        return WS_E_NOT_SUPPORTED;
    }

    return WsSetOutput( channel->writer, encoding, output, NULL, 0, NULL );
}

static HRESULT write_message( struct channel *channel, WS_MESSAGE *msg, const WS_ELEMENT_DESCRIPTION *desc,
                              WS_WRITE_OPTION option, const void *body, ULONG size )
{
    HRESULT hr;
    if ((hr = writer_set_lookup( channel->writer, TRUE )) != S_OK) return hr;
    if ((hr = WsWriteEnvelopeStart( msg, channel->writer, NULL, NULL, NULL )) != S_OK) return hr;
    if ((hr = writer_set_lookup( channel->writer, FALSE )) != S_OK) return hr;
    channel->dict_send.current_sequence++;
    if ((hr = writer_set_dict_callback( channel->writer, dict_cb, &channel->dict_send )) != S_OK) return hr;
    if ((hr = WsWriteBody( msg, desc, option, body, size, NULL )) != S_OK) return hr;
    return WsWriteEnvelopeEnd( msg, NULL );
}

static HRESULT send_message( struct channel *channel, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION *desc,
                             WS_WRITE_OPTION option, const void *body, ULONG size )
{
    HRESULT hr;
    if ((hr = WsAddressMessage( msg, &channel->addr, NULL )) != S_OK) goto done;
    if ((hr = message_set_action( msg, desc->action )) != S_OK) goto done;
    if ((hr = init_writer( channel )) != S_OK) goto done;
    if ((hr = write_message( channel, msg, desc->bodyElementDescription, option, body, size )) != S_OK) goto done;
    hr = send_message_bytes( channel, msg );

done:
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct send_message
{
    struct task                   task;
    struct channel               *channel;
    WS_MESSAGE                   *msg;
    const WS_MESSAGE_DESCRIPTION *desc;
    WS_WRITE_OPTION               option;
    const void                   *body;
    ULONG                         size;
    WS_ASYNC_CONTEXT              ctx;
};

static void send_message_proc( struct task *task )
{
    struct send_message *s = (struct send_message *)task;
    HRESULT hr;

    hr = send_message( s->channel, s->msg, s->desc, s->option, s->body, s->size );

    TRACE( "calling %p(%#lx)\n", s->ctx.callback, hr );
    s->ctx.callback( hr, WS_LONG_CALLBACK, s->ctx.callbackState );
    TRACE( "%p returned\n", s->ctx.callback );
}

static HRESULT queue_send_message( struct channel *channel, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION *desc,
                                   WS_WRITE_OPTION option, const void *body, ULONG size, const WS_ASYNC_CONTEXT *ctx )
{
    struct send_message *s;

    if (!(s = malloc( sizeof(*s) ))) return E_OUTOFMEMORY;
    s->task.proc = send_message_proc;
    s->channel   = channel;
    s->msg       = msg;
    s->desc      = desc;
    s->option    = option;
    s->body      = body;
    s->size      = size;
    s->ctx       = *ctx;
    return queue_task( &channel->send_q, &s->task );
}

/**************************************************************************
 *          WsSendMessage		[webservices.@]
 */
HRESULT WINAPI WsSendMessage( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION *desc,
                              WS_WRITE_OPTION option, const void *body, ULONG size, const WS_ASYNC_CONTEXT *ctx,
                              WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %u %p %lu %p %p\n", handle, msg, desc, option, body, size, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg || !desc) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    WsInitializeMessage( msg, WS_BLANK_MESSAGE, NULL, NULL );

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_send_message( channel, msg, desc, option, body, size, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSendReplyMessage		[webservices.@]
 */
HRESULT WINAPI WsSendReplyMessage( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION *desc,
                                   WS_WRITE_OPTION option, const void *body, ULONG size, WS_MESSAGE *request,
                                   const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    GUID id;
    HRESULT hr;

    TRACE( "%p %p %p %u %p %lu %p %p %p\n", handle, msg, desc, option, body, size, request, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg || !desc || !request) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    WsInitializeMessage( msg, WS_REPLY_MESSAGE, NULL, NULL );
    if ((hr = message_get_id( request, &id )) != S_OK) goto done;
    if ((hr = message_set_request_id( msg, &id )) != S_OK) goto done;

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_send_message( channel, msg, desc, option, body, size, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

done:
    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT resize_read_buffer( struct channel *channel, ULONG size )
{
    if (!channel->read_buf)
    {
        if (!(channel->read_buf = malloc( size ))) return E_OUTOFMEMORY;
        channel->read_buflen = size;
        return S_OK;
    }
    if (channel->read_buflen < size)
    {
        char *tmp;
        ULONG new_size = max( size, channel->read_buflen * 2 );
        if (!(tmp = realloc( channel->read_buf, new_size ))) return E_OUTOFMEMORY;
        channel->read_buf = tmp;
        channel->read_buflen = new_size;
    }
    return S_OK;
}

static CALLBACK HRESULT read_callback( void *state, void *buf, ULONG buflen, ULONG *retlen,
                                       const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    SOCKET socket = *(SOCKET *)state;
    int ret;

    if ((ret = recv( socket, buf, buflen, 0 )) >= 0) *retlen = ret;
    else
    {
        TRACE( "recv failed %u\n", WSAGetLastError() );
        *retlen = 0;
    }
    return S_OK;
}

static HRESULT init_reader( struct channel *channel )
{
    WS_XML_READER_BUFFER_INPUT buf = {{WS_XML_READER_INPUT_TYPE_BUFFER}};
    WS_XML_READER_STREAM_INPUT stream = {{WS_XML_READER_INPUT_TYPE_STREAM}};
    WS_XML_READER_TEXT_ENCODING text = {{WS_XML_READER_ENCODING_TYPE_TEXT}};
    WS_XML_READER_BINARY_ENCODING bin = {{WS_XML_READER_ENCODING_TYPE_BINARY}};
    const WS_XML_READER_ENCODING *encoding;
    const WS_XML_READER_INPUT *input;
    HRESULT hr;

    if (!channel->reader && (hr = WsCreateReader( NULL, 0, &channel->reader, NULL )) != S_OK) return hr;

    switch (channel->encoding)
    {
    case WS_ENCODING_XML_UTF8:
        text.charSet = WS_CHARSET_UTF8;
        encoding = &text.encoding;

        if (channel->binding == WS_UDP_CHANNEL_BINDING ||
            (channel->binding == WS_TCP_CHANNEL_BINDING && !(channel->type & WS_CHANNEL_TYPE_SESSION)))
        {
            stream.readCallback      = read_callback;
            stream.readCallbackState = (channel->binding == WS_UDP_CHANNEL_BINDING) ?
                                        &channel->u.udp.socket : &channel->u.tcp.socket;
            input = &stream.input;
        }
        else
        {
            buf.encodedData     = channel->read_buf;
            buf.encodedDataSize = channel->read_size;
            input = &buf.input;
        }
        break;

    case WS_ENCODING_XML_BINARY_SESSION_1:
        bin.staticDictionary  = (WS_XML_DICTIONARY *)&dict_builtin_static.dict;
        bin.dynamicDictionary = &channel->dict_recv.dict;
        /* fall through */

    case WS_ENCODING_XML_BINARY_1:
        encoding = &bin.encoding;

        buf.encodedData     = channel->read_buf;
        buf.encodedDataSize = channel->read_size;
        input = &buf.input;
        break;

    default:
        FIXME( "unhandled encoding %u\n", channel->encoding );
        return WS_E_NOT_SUPPORTED;
    }

    return WsSetInput( channel->reader, encoding, input, NULL, 0, NULL );
}

static const WS_HTTP_MESSAGE_MAPPING *get_http_message_mapping( struct channel *channel )
{
    const struct prop *prop = &channel->prop[WS_CHANNEL_PROPERTY_HTTP_MESSAGE_MAPPING];
    return (const WS_HTTP_MESSAGE_MAPPING *)prop->value;
}

static HRESULT map_http_response_headers( struct channel *channel, WS_MESSAGE *msg )
{
    const WS_HTTP_MESSAGE_MAPPING *mapping = get_http_message_mapping( channel );
    return message_map_http_response_headers( msg, channel->u.http.request, mapping );
}

#define INITIAL_READ_BUFFER_SIZE 4096
static HRESULT receive_message_bytes_http( struct channel *channel, WS_MESSAGE *msg )
{
    DWORD len, bytes_read, offset = 0, size = INITIAL_READ_BUFFER_SIZE;
    ULONG max_len = get_max_buffer_size( channel );
    HRESULT hr;

    if ((hr = map_http_response_headers( channel, msg )) != S_OK) return hr;

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

    return S_OK;
}

static HRESULT receive_message_sized( struct channel *channel, unsigned int size )
{
    unsigned int offset = 0, to_read = size;
    int bytes_read;
    HRESULT hr;

    if ((hr = resize_read_buffer( channel, size )) != S_OK) return hr;

    channel->read_size = 0;
    while (channel->read_size < size)
    {
        if ((bytes_read = recv( channel->u.tcp.socket, channel->read_buf + offset, to_read, 0 )) < 0)
        {
            return HRESULT_FROM_WIN32( WSAGetLastError() );
        }
        if (!bytes_read) break;
        channel->read_size += bytes_read;
        to_read -= bytes_read;
        offset += bytes_read;
    }
    if (channel->read_size != size) return WS_E_INVALID_FORMAT;
    return S_OK;
}

static HRESULT receive_size( struct channel *channel, unsigned int *size )
{
    unsigned char byte;
    HRESULT hr;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    *size = byte & 0x7f;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    *size += (byte & 0x7f) << 7;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    *size += (byte & 0x7f) << 14;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    *size += (byte & 0x7f) << 21;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = receive_bytes( channel, &byte, 1 )) != S_OK) return hr;
    if (byte & ~0x0f) return WS_E_INVALID_FORMAT;
    *size += byte << 28;
    return S_OK;
}

static WS_ENCODING map_known_encoding( enum known_encoding encoding )
{
    switch (encoding)
    {
    case KNOWN_ENCODING_SOAP11_UTF8:
    case KNOWN_ENCODING_SOAP12_UTF8:           return WS_ENCODING_XML_UTF8;
    case KNOWN_ENCODING_SOAP11_UTF16:
    case KNOWN_ENCODING_SOAP12_UTF16:          return WS_ENCODING_XML_UTF16BE;
    case KNOWN_ENCODING_SOAP11_UTF16LE:
    case KNOWN_ENCODING_SOAP12_UTF16LE:        return WS_ENCODING_XML_UTF16LE;
    case KNOWN_ENCODING_SOAP12_BINARY:         return WS_ENCODING_XML_BINARY_1;
    case KNOWN_ENCODING_SOAP12_BINARY_SESSION: return WS_ENCODING_XML_BINARY_SESSION_1;
    default:
        WARN( "unhandled encoding %u, assuming UTF8\n", encoding );
        return WS_ENCODING_XML_UTF8;
    }
}

static HRESULT receive_preamble( struct channel *channel )
{
    unsigned char type;
    HRESULT hr;

    for (;;)
    {
        if ((hr = receive_bytes( channel, &type, 1 )) != S_OK) return hr;
        if (type == FRAME_RECORD_TYPE_PREAMBLE_END) break;
        switch (type)
        {
        case FRAME_RECORD_TYPE_VERSION:
        {
            unsigned char major, minor;
            if ((hr = receive_bytes( channel, &major, 1 )) != S_OK) return hr;
            if ((hr = receive_bytes( channel, &minor, 1 )) != S_OK) return hr;
            TRACE( "major %u minor %u\n", major, major );
            break;
        }
        case FRAME_RECORD_TYPE_MODE:
        {
            unsigned char mode;
            if ((hr = receive_bytes( channel, &mode, 1 )) != S_OK) return hr;
            TRACE( "mode %u\n", mode );
            break;
        }
        case FRAME_RECORD_TYPE_VIA:
        {
            unsigned int size;
            unsigned char *url;

            if ((hr = receive_size( channel, &size )) != S_OK) return hr;
            if (!(url = malloc( size ))) return E_OUTOFMEMORY;
            if ((hr = receive_bytes( channel, url, size )) != S_OK)
            {
                free( url );
                return hr;
            }
            TRACE( "transport URL %s\n", debugstr_an((char *)url, size) );
            free( url ); /* FIXME: verify */
            break;
        }
        case FRAME_RECORD_TYPE_KNOWN_ENCODING:
        {
            unsigned char encoding;
            if ((hr = receive_bytes( channel, &encoding, 1 )) != S_OK) return hr;
            TRACE( "encoding %u\n", encoding );
            channel->encoding = map_known_encoding( encoding );
            break;
        }
        default:
            WARN( "unhandled record type %u\n", type );
            return WS_E_INVALID_FORMAT;
        }
    }

    return S_OK;
}

static HRESULT receive_sized_envelope( struct channel *channel )
{
    unsigned char type;
    unsigned int size;
    HRESULT hr;

    if ((hr = receive_bytes( channel, &type, 1 )) != S_OK) return hr;
    if (type == FRAME_RECORD_TYPE_END) return WS_S_END;
    if (type != FRAME_RECORD_TYPE_SIZED_ENVELOPE) return WS_E_INVALID_FORMAT;
    if ((hr = receive_size( channel, &size )) != S_OK) return hr;
    if ((hr = receive_message_sized( channel, size )) != S_OK) return hr;
    return S_OK;
}

static HRESULT read_size( const BYTE **ptr, ULONG len, ULONG *size )
{
    const BYTE *buf = *ptr;

    if (len < 1) return WS_E_INVALID_FORMAT;
    *size = buf[0] & 0x7f;
    if (!(buf[0] & 0x80))
    {
        *ptr += 1;
        return S_OK;
    }
    if (len < 2) return WS_E_INVALID_FORMAT;
    *size += (buf[1] & 0x7f) << 7;
    if (!(buf[1] & 0x80))
    {
        *ptr += 2;
        return S_OK;
    }
    if (len < 3) return WS_E_INVALID_FORMAT;
    *size += (buf[2] & 0x7f) << 14;
    if (!(buf[2] & 0x80))
    {
        *ptr += 3;
        return S_OK;
    }
    if (len < 4) return WS_E_INVALID_FORMAT;
    *size += (buf[3] & 0x7f) << 21;
    if (!(buf[3] & 0x80))
    {
        *ptr += 4;
        return S_OK;
    }
    if (len < 5 || (buf[4] & ~0x07)) return WS_E_INVALID_FORMAT;
    *size += buf[4] << 28;
    *ptr += 5;
    return S_OK;
}

static HRESULT build_dict( const BYTE *buf, ULONG buflen, struct dictionary *dict, ULONG *used )
{
    ULONG size, strings_size, strings_offset;
    const BYTE *ptr = buf;
    BYTE *bytes;
    int index;
    HRESULT hr;

    if ((hr = read_size( &ptr, buflen, &strings_size )) != S_OK) return hr;
    strings_offset = ptr - buf;
    if (buflen < strings_offset + strings_size) return WS_E_INVALID_FORMAT;
    *used = strings_offset + strings_size;
    if (!strings_size) return S_OK;

    UuidCreate( &dict->dict.guid );
    dict->dict.isConst = FALSE;

    buflen -= strings_offset;
    ptr = buf + strings_offset;
    while (ptr < buf + strings_size)
    {
        if ((hr = read_size( &ptr, buflen, &size )) != S_OK)
        {
            init_dict( dict, 0 );
            return hr;
        }
        if (size > buflen)
        {
            init_dict( dict, 0 );
            return WS_E_INVALID_FORMAT;
        }
        if (size + dict->str_bytes + 1 > dict->str_bytes_max)
        {
            hr = WS_E_QUOTA_EXCEEDED;
            goto error;
        }
        buflen -= size;
        if (!(bytes = malloc( size )))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        memcpy( bytes, ptr, size );
        if ((index = find_string( dict, bytes, size, NULL )) == -1) /* duplicate */
        {
            free( bytes );
            ptr += size;
            continue;
        }
        if ((hr = insert_string( dict, bytes, size, index, NULL )) != S_OK)
        {
            free( bytes );
            init_dict( dict, 0 );
            return hr;
        }
        ptr += size;
    }
    return S_OK;

error:
    init_dict( dict, 0 );
    return hr;
}

static HRESULT send_preamble_ack( struct channel *channel )
{
    HRESULT hr;
    if ((hr = send_byte( channel->u.tcp.socket, FRAME_RECORD_TYPE_PREAMBLE_ACK )) != S_OK) return hr;
    channel->session_state = SESSION_STATE_SETUP_COMPLETE;
    return S_OK;
}

static HRESULT receive_message_bytes_session( struct channel *channel )
{
    HRESULT hr;

    if ((hr = receive_sized_envelope( channel )) != S_OK) return hr;
    if (channel->encoding == WS_ENCODING_XML_BINARY_SESSION_1)
    {
        ULONG size;
        if ((hr = build_dict( (const BYTE *)channel->read_buf, channel->read_size, &channel->dict_recv,
                              &size )) != S_OK) return hr;
        channel->read_size -= size;
        memmove( channel->read_buf, channel->read_buf + size, channel->read_size );
    }

    return S_OK;
}

static HRESULT receive_message_bytes( struct channel *channel, WS_MESSAGE *msg )
{
    switch (channel->binding)
    {
    case WS_HTTP_CHANNEL_BINDING:
        return receive_message_bytes_http( channel, msg );

    case WS_TCP_CHANNEL_BINDING:
        if (channel->type & WS_CHANNEL_TYPE_SESSION)
        {
            HRESULT hr;
            switch (channel->session_state)
            {
            case SESSION_STATE_UNINITIALIZED:
                if ((hr = receive_preamble( channel )) != S_OK) return hr;
                if ((hr = send_preamble_ack( channel )) != S_OK) return hr;
                /* fall through */

            case SESSION_STATE_SETUP_COMPLETE:
                return receive_message_bytes_session( channel );

            default:
                ERR( "unhandled session state %u\n", channel->session_state );
                return WS_E_OTHER;
            }
        }
        return S_OK; /* nothing to do, data is read through stream callback */

    case WS_UDP_CHANNEL_BINDING:
        return S_OK;

    default:
        ERR( "unhandled binding %u\n", channel->binding );
        return E_NOTIMPL;
    }
}

HRESULT channel_receive_message( WS_CHANNEL *handle, WS_MESSAGE *msg )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if ((hr = receive_message_bytes( channel, msg )) == S_OK) hr = init_reader( channel );
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;

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
                             WS_READ_OPTION option, WS_HEAP *heap, void *body, ULONG size, WS_ERROR *error )
{
    HRESULT hr;
    if ((hr = WsReadEnvelopeStart( handle, reader, NULL, NULL, NULL )) != S_OK) return hr;
    if ((hr = message_read_fault( handle, heap, error )) != S_OK) return hr;
    if ((hr = WsReadBody( handle, desc, option, heap, body, size, NULL )) != S_OK) return hr;
    return WsReadEnvelopeEnd( handle, NULL );
}

static HRESULT receive_message( struct channel *channel, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION **desc,
                                ULONG count, WS_RECEIVE_OPTION option, WS_READ_OPTION read_option, WS_HEAP *heap,
                                void *value, ULONG size, ULONG *index, WS_ERROR *error )
{
    HRESULT hr;
    ULONG i;

    if ((hr = receive_message_bytes( channel, msg )) != S_OK) goto done;
    if ((hr = init_reader( channel )) != S_OK) goto done;

    for (i = 0; i < count; i++)
    {
        const WS_ELEMENT_DESCRIPTION *body = desc[i]->bodyElementDescription;
        if ((hr = read_message( msg, channel->reader, body, read_option, heap, value, size, error )) == S_OK)
        {
            if (index) *index = i;
            break;
        }
        if ((hr = WsResetMessage( msg, NULL )) != S_OK) goto done;
        if ((hr = init_reader( channel )) != S_OK) goto done;
    }
    hr = (i == count) ? WS_E_INVALID_FORMAT : S_OK;

done:
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct receive_message
{
    struct task                    task;
    struct channel                *channel;
    WS_MESSAGE                    *msg;
    const WS_MESSAGE_DESCRIPTION **desc;
    ULONG                          count;
    WS_RECEIVE_OPTION              option;
    WS_READ_OPTION                 read_option;
    WS_HEAP                       *heap;
    void                          *value;
    ULONG                          size;
    ULONG                         *index;
    WS_ASYNC_CONTEXT               ctx;
    WS_ERROR                      *error;
};

static void receive_message_proc( struct task *task )
{
    struct receive_message *r = (struct receive_message *)task;
    HRESULT hr;

    hr = receive_message( r->channel, r->msg, r->desc, r->count, r->option, r->read_option, r->heap, r->value,
                          r->size, r->index, r->error );

    TRACE( "calling %p(%#lx)\n", r->ctx.callback, hr );
    r->ctx.callback( hr, WS_LONG_CALLBACK, r->ctx.callbackState );
    TRACE( "%p returned\n", r->ctx.callback );
}

static HRESULT queue_receive_message( struct channel *channel, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION **desc,
                                      ULONG count, WS_RECEIVE_OPTION option, WS_READ_OPTION read_option,
                                      WS_HEAP *heap, void *value, ULONG size, ULONG *index,
                                      const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct receive_message *r;

    if (!(r = malloc( sizeof(*r) ))) return E_OUTOFMEMORY;
    r->task.proc   = receive_message_proc;
    r->channel     = channel;
    r->msg         = msg;
    r->desc        = desc;
    r->count       = count;
    r->option      = option;
    r->read_option = read_option;
    r->heap        = heap;
    r->value       = value;
    r->size        = size;
    r->index       = index;
    r->ctx         = *ctx;
    r->error       = error;
    return queue_task( &channel->recv_q, &r->task );
}

/**************************************************************************
 *          WsReceiveMessage		[webservices.@]
 */
HRESULT WINAPI WsReceiveMessage( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_MESSAGE_DESCRIPTION **desc,
                                 ULONG count, WS_RECEIVE_OPTION option, WS_READ_OPTION read_option, WS_HEAP *heap,
                                 void *value, ULONG size, ULONG *index, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %lu %u %u %p %p %lu %p %p %p\n", handle, msg, desc, count, option, read_option, heap,
           value, size, index, ctx, error );

    if (!channel || !msg || !desc || !count) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_receive_message( channel, msg, desc, count, option, read_option, heap, value, size, index,
                                ctx ? ctx : &ctx_local, error );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT request_reply( struct channel *channel, WS_MESSAGE *request,
                              const WS_MESSAGE_DESCRIPTION *request_desc, WS_WRITE_OPTION write_option,
                              const void *request_body, ULONG request_size, WS_MESSAGE *reply,
                              const WS_MESSAGE_DESCRIPTION *reply_desc, WS_READ_OPTION read_option,
                              WS_HEAP *heap, void *value, ULONG size, WS_ERROR *error )
{
    HRESULT hr;

    if ((hr = send_message( channel, request, request_desc, write_option, request_body, request_size )) != S_OK)
        return hr;

    return receive_message( channel, reply, &reply_desc, 1, WS_RECEIVE_OPTIONAL_MESSAGE, read_option, heap,
                            value, size, NULL, error );
}

struct request_reply
{
    struct task                   task;
    struct channel               *channel;
    WS_MESSAGE                   *request;
    const WS_MESSAGE_DESCRIPTION *request_desc;
    WS_WRITE_OPTION               write_option;
    const void                   *request_body;
    ULONG                         request_size;
    WS_MESSAGE                   *reply;
    const WS_MESSAGE_DESCRIPTION *reply_desc;
    WS_READ_OPTION                read_option;
    WS_HEAP                      *heap;
    void                         *value;
    ULONG                         size;
    WS_ASYNC_CONTEXT              ctx;
    WS_ERROR                     *error;
};

static void request_reply_proc( struct task *task )
{
    struct request_reply *r = (struct request_reply *)task;
    HRESULT hr;

    hr = request_reply( r->channel, r->request, r->request_desc, r->write_option, r->request_body, r->request_size,
                        r->reply, r->reply_desc, r->read_option, r->heap, r->value, r->size, r->error );

    TRACE( "calling %p(%#lx)\n", r->ctx.callback, hr );
    r->ctx.callback( hr, WS_LONG_CALLBACK, r->ctx.callbackState );
    TRACE( "%p returned\n", r->ctx.callback );
}

static HRESULT queue_request_reply( struct channel *channel, WS_MESSAGE *request,
                                    const WS_MESSAGE_DESCRIPTION *request_desc, WS_WRITE_OPTION write_option,
                                    const void *request_body, ULONG request_size, WS_MESSAGE *reply,
                                    const WS_MESSAGE_DESCRIPTION *reply_desc, WS_READ_OPTION read_option,
                                    WS_HEAP *heap, void *value, ULONG size, const WS_ASYNC_CONTEXT *ctx,
                                    WS_ERROR *error )
{
    struct request_reply *r;

    if (!(r = malloc( sizeof(*r) ))) return E_OUTOFMEMORY;
    r->task.proc    = request_reply_proc;
    r->channel      = channel;
    r->request      = request;
    r->request_desc = request_desc;
    r->write_option = write_option;
    r->request_body = request_body;
    r->request_size = request_size;
    r->reply        = reply;
    r->reply_desc   = reply_desc;
    r->read_option  = read_option;
    r->heap         = heap;
    r->value        = value;
    r->size         = size;
    r->ctx          = *ctx;
    r->error        = error;
    return queue_task( &channel->recv_q, &r->task );
}

/**************************************************************************
 *          WsRequestReply		[webservices.@]
 */
HRESULT WINAPI WsRequestReply( WS_CHANNEL *handle, WS_MESSAGE *request, const WS_MESSAGE_DESCRIPTION *request_desc,
                               WS_WRITE_OPTION write_option, const void *request_body, ULONG request_size,
                               WS_MESSAGE *reply, const WS_MESSAGE_DESCRIPTION *reply_desc, WS_READ_OPTION read_option,
                               WS_HEAP *heap, void *value, ULONG size, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %u %p %lu %p %p %u %p %p %lu %p %p\n", handle, request, request_desc, write_option,
           request_body, request_size, reply, reply_desc, read_option, heap, value, size, ctx, error );

    if (!channel || !request || !reply) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    WsInitializeMessage( request, WS_REQUEST_MESSAGE, NULL, NULL );

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_request_reply( channel, request, request_desc, write_option, request_body, request_size, reply,
                              reply_desc, read_option, heap, value, size, ctx ? ctx : &ctx_local, error );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT read_message_start( struct channel *channel, WS_MESSAGE *msg )
{
    HRESULT hr;
    if ((hr = receive_message_bytes( channel, msg )) == S_OK && (hr = init_reader( channel )) == S_OK)
        hr = WsReadEnvelopeStart( msg, channel->reader, NULL, NULL, NULL );
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct read_message_start
{
    struct task      task;
    struct channel  *channel;
    WS_MESSAGE      *msg;
    WS_ASYNC_CONTEXT ctx;
};

static void read_message_start_proc( struct task *task )
{
    struct read_message_start *r = (struct read_message_start *)task;
    HRESULT hr;

    hr = read_message_start( r->channel, r->msg );

    TRACE( "calling %p(%#lx)\n", r->ctx.callback, hr );
    r->ctx.callback( hr, WS_LONG_CALLBACK, r->ctx.callbackState );
    TRACE( "%p returned\n", r->ctx.callback );
}

static HRESULT queue_read_message_start( struct channel *channel, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx )
{
    struct read_message_start *r;

    if (!(r = malloc( sizeof(*r) ))) return E_OUTOFMEMORY;
    r->task.proc = read_message_start_proc;
    r->channel   = channel;
    r->msg       = msg;
    r->ctx       = *ctx;
    return queue_task( &channel->recv_q, &r->task );
}

/**************************************************************************
 *          WsReadMessageStart		[webservices.@]
 */
HRESULT WINAPI WsReadMessageStart( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_read_message_start( channel, msg, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT read_message_end( struct channel *channel, WS_MESSAGE *msg )
{
    HRESULT hr = WsReadEnvelopeEnd( msg, NULL );
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct read_message_end
{
    struct task      task;
    struct channel  *channel;
    WS_MESSAGE      *msg;
    WS_ASYNC_CONTEXT ctx;
};

static void read_message_end_proc( struct task *task )
{
    struct read_message_end *r = (struct read_message_end *)task;
    HRESULT hr;

    hr = read_message_end( r->channel, r->msg );

    TRACE( "calling %p(%#lx)\n", r->ctx.callback, hr );
    r->ctx.callback( hr, WS_LONG_CALLBACK, r->ctx.callbackState );
    TRACE( "%p returned\n", r->ctx.callback );
}

static HRESULT queue_read_message_end( struct channel *channel, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx )
{
    struct read_message_end *r;

    if (!(r = malloc( sizeof(*r) ))) return E_OUTOFMEMORY;
    r->task.proc = read_message_end_proc;
    r->channel   = channel;
    r->msg       = msg;
    r->ctx       = *ctx;
    return queue_task( &channel->recv_q, &r->task );
}

/**************************************************************************
 *          WsReadMessageEnd		[webservices.@]
 */
HRESULT WINAPI WsReadMessageEnd( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_read_message_end( channel, msg, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_message_start( struct channel *channel, WS_MESSAGE *msg )
{
    HRESULT hr;
    if ((hr = init_writer( channel )) != S_OK) goto done;
    if ((hr = WsAddressMessage( msg, &channel->addr, NULL )) != S_OK) goto done;
    hr = WsWriteEnvelopeStart( msg, channel->writer, NULL, NULL, NULL );

done:
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct write_message_start
{
    struct task      task;
    struct channel  *channel;
    WS_MESSAGE      *msg;
    WS_ASYNC_CONTEXT ctx;
};

static void write_message_start_proc( struct task *task )
{
    struct write_message_start *w = (struct write_message_start *)task;
    HRESULT hr;

    hr = write_message_start( w->channel, w->msg );

    TRACE( "calling %p(%#lx)\n", w->ctx.callback, hr );
    w->ctx.callback( hr, WS_LONG_CALLBACK, w->ctx.callbackState );
    TRACE( "%p returned\n", w->ctx.callback );
}

static HRESULT queue_write_message_start( struct channel *channel, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx )
{
    struct write_message_start *w;

    if (!(w = malloc( sizeof(*w) ))) return E_OUTOFMEMORY;
    w->task.proc = write_message_start_proc;
    w->channel   = channel;
    w->msg       = msg;
    w->ctx       = *ctx;
    return queue_task( &channel->send_q, &w->task );
}

/**************************************************************************
 *          WsWriteMessageStart		[webservices.@]
 */
HRESULT WINAPI WsWriteMessageStart( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_write_message_start( channel, msg, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_message_end( struct channel *channel, WS_MESSAGE *msg )
{
    HRESULT hr;
    if ((hr = WsWriteEnvelopeEnd( msg, NULL )) == S_OK) hr = send_message_bytes( channel, msg );
    if (hr != S_OK) channel->state = WS_CHANNEL_STATE_FAULTED;
    return hr;
}

struct write_message_end
{
    struct task      task;
    struct channel  *channel;
    WS_MESSAGE      *msg;
    WS_ASYNC_CONTEXT ctx;
};

static void write_message_end_proc( struct task *task )
{
    struct write_message_end *w = (struct write_message_end *)task;
    HRESULT hr;

    hr = write_message_end( w->channel, w->msg );

    TRACE( "calling %p(%#lx)\n", w->ctx.callback, hr );
    w->ctx.callback( hr, WS_LONG_CALLBACK, w->ctx.callbackState );
    TRACE( "%p returned\n", w->ctx.callback );
}

static HRESULT queue_write_message_end( struct channel *channel, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx )
{
    struct write_message_start *w;

    if (!(w = malloc( sizeof(*w) ))) return E_OUTOFMEMORY;
    w->task.proc = write_message_end_proc;
    w->channel   = channel;
    w->msg       = msg;
    w->ctx       = *ctx;
    return queue_task( &channel->send_q, &w->task );
}

/**************************************************************************
 *          WsWriteMessageEnd		[webservices.@]
 */
HRESULT WINAPI WsWriteMessageEnd( WS_CHANNEL *handle, WS_MESSAGE *msg, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;
    WS_ASYNC_CONTEXT ctx_local;
    struct async async;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, msg, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel || !msg) return E_INVALIDARG;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }
    if ((hr = check_state( channel, WS_CHANNEL_STATE_OPEN )) != S_OK)
    {
        LeaveCriticalSection( &channel->cs );
        return hr;
    }

    if (!ctx) async_init( &async, &ctx_local );
    hr = queue_write_message_end( channel, msg, ctx ? ctx : &ctx_local );
    if (!ctx)
    {
        if (hr == WS_S_ASYNC) hr = async_wait( &async );
        CloseHandle( async.done );
    }

    LeaveCriticalSection( &channel->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static void set_blocking( SOCKET socket, BOOL blocking )
{
    ULONG state = !blocking;
    ioctlsocket( socket, FIONBIO, &state );
}

static HRESULT sock_accept( SOCKET socket, HANDLE wait, HANDLE cancel, SOCKET *ret )
{
    HANDLE handles[] = { wait, cancel };
    HRESULT hr = S_OK;

    if (WSAEventSelect( socket, handles[0], FD_ACCEPT )) return HRESULT_FROM_WIN32( WSAGetLastError() );

    switch (WSAWaitForMultipleEvents( 2, handles, FALSE, WSA_INFINITE, FALSE ))
    {
    case 0:
        if ((*ret = accept( socket, NULL, NULL )) != -1)
        {
            WSAEventSelect( *ret, NULL, 0 );
            set_blocking( *ret, TRUE );
            break;
        }
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        break;

    case 1:
        hr = WS_E_OPERATION_ABORTED;
        break;

    default:
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        break;
    }

    return hr;
}

HRESULT channel_accept_tcp( SOCKET socket, HANDLE wait, HANDLE cancel, WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = sock_accept( socket, wait, cancel, &channel->u.tcp.socket )) == S_OK)
    {
        BOOL nodelay = FALSE;
        prop_get( channel->prop, channel->prop_count, WS_CHANNEL_PROPERTY_NO_DELAY, &nodelay, sizeof(nodelay) );
        setsockopt( channel->u.tcp.socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(nodelay) );
        channel->state = WS_CHANNEL_STATE_OPEN;
    }

    LeaveCriticalSection( &channel->cs );
    return hr;
}

static HRESULT sock_wait( SOCKET socket, HANDLE wait, HANDLE cancel )
{
    HANDLE handles[] = { wait, cancel };
    HRESULT hr;

    if (WSAEventSelect( socket, handles[0], FD_READ )) return HRESULT_FROM_WIN32( WSAGetLastError() );

    switch (WSAWaitForMultipleEvents( 2, handles, FALSE, WSA_INFINITE, FALSE ))
    {
    case 0:
        hr = S_OK;
        break;

    case 1:
        hr = WS_E_OPERATION_ABORTED;
        break;

    default:
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        break;
    }

    WSAEventSelect( socket, NULL, 0 );
    set_blocking( socket, TRUE );
    return hr;
}

HRESULT channel_accept_udp( SOCKET socket, HANDLE wait, HANDLE cancel, WS_CHANNEL *handle )
{
    struct channel *channel = (struct channel *)handle;
    HRESULT hr;

    EnterCriticalSection( &channel->cs );

    if (channel->magic != CHANNEL_MAGIC)
    {
        LeaveCriticalSection( &channel->cs );
        return E_INVALIDARG;
    }

    if ((hr = sock_wait( socket, wait, cancel )) == S_OK)
    {
        channel->u.udp.socket = socket;
        channel->state = WS_CHANNEL_STATE_OPEN;
    }

    LeaveCriticalSection( &channel->cs );
    return hr;
}

HRESULT channel_address_message( WS_CHANNEL *handle, WS_MESSAGE *msg )
{
    struct channel *channel = (struct channel *)handle;
    return WsAddressMessage( msg, &channel->addr, NULL );
}
