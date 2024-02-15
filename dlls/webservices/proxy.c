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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc proxy_props[] =
{
    { sizeof(ULONG), FALSE, TRUE },             /* WS_PROXY_PROPERTY_CALL_TIMEOUT */
    { sizeof(WS_MESSAGE_PROPERTIES), FALSE },   /* WS_PROXY_PROPERTY_MESSAGE_PROPERTIES */
    { sizeof(USHORT), FALSE, TRUE },            /* WS_PROXY_PROPERTY_MAX_CALL_POOL_SIZE */
    { sizeof(WS_SERVICE_PROXY_STATE), TRUE },   /* WS_PROXY_PROPERTY_STATE */
    { sizeof(ULONG), FALSE, TRUE },             /* WS_PROXY_PROPERTY_MAX_PENDING_CALLS */
    { sizeof(ULONG), FALSE, TRUE },             /* WS_PROXY_PROPERTY_MAX_CLOSE_TIMEOUT */
    { sizeof(LANGID), FALSE, TRUE },            /* WS_PROXY_FAULT_LANG_ID */
};

struct proxy
{
    ULONG                   magic;
    CRITICAL_SECTION        cs;
    WS_SERVICE_PROXY_STATE  state;
    WS_CHANNEL             *channel;
    ULONG                   prop_count;
    struct prop             prop[ARRAY_SIZE( proxy_props )];
};

#define PROXY_MAGIC (('P' << 24) | ('R' << 16) | ('O' << 8) | 'X')

static struct proxy *alloc_proxy(void)
{
    static const ULONG count = ARRAY_SIZE( proxy_props );
    struct proxy *ret;
    ULONG size = sizeof(*ret) + prop_size( proxy_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;

    ret->magic      = PROXY_MAGIC;
    InitializeCriticalSectionEx( &ret->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": proxy.cs");

    prop_init( proxy_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void reset_proxy( struct proxy *proxy )
{
    WsResetChannel( proxy->channel, NULL );
    proxy->state = WS_SERVICE_PROXY_STATE_CREATED;
}

static void free_proxy( struct proxy *proxy )
{
    reset_proxy( proxy );
    WsFreeChannel( proxy->channel );

    proxy->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &proxy->cs );
    free( proxy );
}

static HRESULT create_proxy( WS_CHANNEL *channel, const WS_PROXY_PROPERTY *properties, ULONG count,
                             WS_SERVICE_PROXY **handle )
{
    struct proxy *proxy;
    HRESULT hr;
    ULONG i;

    if (!(proxy = alloc_proxy())) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( proxy->prop, proxy->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_proxy( proxy );
            return hr;
        }
    }

    proxy->channel = channel;

    *handle = (WS_SERVICE_PROXY *)proxy;
    return S_OK;
}

/**************************************************************************
 *          WsCreateServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsCreateServiceProxy( const WS_CHANNEL_TYPE type, const WS_CHANNEL_BINDING binding,
                                     const WS_SECURITY_DESCRIPTION *desc,
                                     const WS_PROXY_PROPERTY *proxy_props, ULONG proxy_props_count,
                                     const WS_CHANNEL_PROPERTY *channel_props,
                                     const ULONG channel_props_count, WS_SERVICE_PROXY **handle,
                                     WS_ERROR *error )
{
    WS_CHANNEL *channel;
    HRESULT hr;

    TRACE( "%u %u %p %p %lu %p %lu %p %p\n", type, binding, desc, proxy_props, proxy_props_count,
           channel_props, channel_props_count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if ((hr = WsCreateChannel( type, binding, channel_props, channel_props_count, NULL, &channel,
                               NULL )) != S_OK) return hr;

    if ((hr = create_proxy( channel, proxy_props, proxy_props_count, handle )) != S_OK)
    {
        WsFreeChannel( channel );
        return hr;
    }

    TRACE( "created %p\n", *handle );
    return S_OK;
}

/**************************************************************************
 *          WsCreateServiceProxyFromTemplate		[webservices.@]
 */
HRESULT WINAPI WsCreateServiceProxyFromTemplate( WS_CHANNEL_TYPE channel_type,
                                                 const WS_PROXY_PROPERTY *properties, const ULONG count,
                                                 WS_BINDING_TEMPLATE_TYPE type, void *value, ULONG size,
                                                 const void *desc, ULONG desc_size, WS_SERVICE_PROXY **handle,
                                                 WS_ERROR *error )
{
    const WS_CHANNEL_PROPERTY *channel_props = NULL;
    ULONG channel_props_count = 0;
    WS_CHANNEL_BINDING binding;
    WS_CHANNEL *channel;
    HRESULT hr;

    TRACE( "%u %p %lu %u %p %lu %p %lu %p %p\n", channel_type, properties, count, type, value, size, desc,
           desc_size, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!desc || !handle) return E_INVALIDARG;
    FIXME( "ignoring description\n" );

    switch (type)
    {
    case WS_HTTP_BINDING_TEMPLATE_TYPE:
    {
        WS_HTTP_BINDING_TEMPLATE *http = value;
        if (http)
        {
            channel_props = http->channelProperties.properties;
            channel_props_count = http->channelProperties.propertyCount;
        }
        binding = WS_HTTP_CHANNEL_BINDING;
        break;
    }
    case WS_HTTP_SSL_BINDING_TEMPLATE_TYPE:
    {
        WS_HTTP_SSL_BINDING_TEMPLATE *https = value;
        if (https)
        {
            channel_props = https->channelProperties.properties;
            channel_props_count = https->channelProperties.propertyCount;
        }
        binding = WS_HTTP_CHANNEL_BINDING;
        break;
    }
    default:
        FIXME( "template type %u not implemented\n", type );
        return E_NOTIMPL;
    }

    if ((hr = WsCreateChannel( channel_type, binding, channel_props, channel_props_count, NULL,
                               &channel, NULL )) != S_OK) return hr;

    if ((hr = create_proxy( channel, properties, count, handle )) != S_OK)
    {
        WsFreeChannel( channel );
        return hr;
    }

    TRACE( "created %p\n", *handle );
    return S_OK;
}

/**************************************************************************
 *          WsResetServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsResetServiceProxy( WS_SERVICE_PROXY *handle, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!proxy) return E_INVALIDARG;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return E_INVALIDARG;
    }

    if (proxy->state != WS_SERVICE_PROXY_STATE_CREATED && proxy->state != WS_SERVICE_PROXY_STATE_CLOSED)
        hr = WS_E_INVALID_OPERATION;
    else
        reset_proxy( proxy );

    LeaveCriticalSection( &proxy->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsFreeServiceProxy		[webservices.@]
 */
void WINAPI WsFreeServiceProxy( WS_SERVICE_PROXY *handle )
{
    struct proxy *proxy = (struct proxy *)handle;

    TRACE( "%p\n", handle );

    if (!proxy) return;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return;
    }

    proxy->magic = 0;

    LeaveCriticalSection( &proxy->cs );
    free_proxy( proxy );
}

/**************************************************************************
 *          WsGetServiceProxyProperty		[webservices.@]
 */
HRESULT WINAPI WsGetServiceProxyProperty( WS_SERVICE_PROXY *handle, WS_PROXY_PROPERTY_ID id,
                                          void *buf, ULONG size, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!proxy) return E_INVALIDARG;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return E_INVALIDARG;
    }

    switch (id)
    {
    case WS_PROXY_PROPERTY_STATE:
        if (!buf || size != sizeof(proxy->state)) hr = E_INVALIDARG;
        else *(WS_SERVICE_PROXY_STATE *)buf = proxy->state;
        break;

    default:
        hr = prop_get( proxy->prop, proxy->prop_count, id, buf, size );
    }

    LeaveCriticalSection( &proxy->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsOpenServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsOpenServiceProxy( WS_SERVICE_PROXY *handle, const WS_ENDPOINT_ADDRESS *endpoint,
                                   const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, endpoint, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!proxy || !endpoint) return E_INVALIDARG;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return E_INVALIDARG;
    }

    if ((hr = WsOpenChannel( proxy->channel, endpoint, NULL, NULL )) == S_OK)
        proxy->state = WS_SERVICE_PROXY_STATE_OPEN;

    LeaveCriticalSection( &proxy->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsCloseServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsCloseServiceProxy( WS_SERVICE_PROXY *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!proxy) return E_INVALIDARG;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return E_INVALIDARG;
    }

    if ((hr = WsCloseChannel( proxy->channel, NULL, NULL )) == S_OK)
        proxy->state = WS_SERVICE_PROXY_STATE_CLOSED;

    LeaveCriticalSection( &proxy->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsAbortServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsAbortServiceProxy( WS_SERVICE_PROXY *handle, WS_ERROR *error )
{
    FIXME( "%p %p\n", handle, error );
    return E_NOTIMPL;
}

static HRESULT set_send_context( WS_MESSAGE *msg, const WS_CALL_PROPERTY *props, ULONG count )
{
    ULONG i;
    for (i = 0; i < count; i++)
    {
        if (props[i].id == WS_CALL_PROPERTY_SEND_MESSAGE_CONTEXT)
        {
            if (props[i].valueSize != sizeof(WS_PROXY_MESSAGE_CALLBACK_CONTEXT)) return E_INVALIDARG;
            message_set_send_context( msg, props[i].value );
            break;
        }
    }
    return S_OK;
}

static HRESULT set_receive_context( WS_MESSAGE *msg, const WS_CALL_PROPERTY *props, ULONG count )
{
    ULONG i;
    for (i = 0; i < count; i++)
    {
        if (props[i].id == WS_CALL_PROPERTY_RECEIVE_MESSAGE_CONTEXT)
        {
            if (props[i].valueSize != sizeof(WS_PROXY_MESSAGE_CALLBACK_CONTEXT)) return E_INVALIDARG;
            message_set_receive_context( msg, props[i].value );
            break;
        }
    }
    return S_OK;
}

static HRESULT write_message( WS_MESSAGE *msg, WS_XML_WRITER *writer, const WS_ELEMENT_DESCRIPTION *desc,
                              const WS_PARAMETER_DESCRIPTION *params, ULONG count, const void **args )
{
    HRESULT hr;
    message_do_send_callback( msg );
    if ((hr = WsWriteEnvelopeStart( msg, writer, NULL, NULL, NULL )) != S_OK) return hr;
    if ((hr = write_input_params( writer, desc, params, count, args )) != S_OK) return hr;
    return WsWriteEnvelopeEnd( msg, NULL );
}

static HRESULT set_output( WS_XML_WRITER *writer )
{
    WS_XML_WRITER_TEXT_ENCODING text = { {WS_XML_WRITER_ENCODING_TYPE_TEXT}, WS_CHARSET_UTF8 };
    WS_XML_WRITER_BUFFER_OUTPUT buf = { {WS_XML_WRITER_OUTPUT_TYPE_BUFFER} };
    return WsSetOutput( writer, &text.encoding, &buf.output, NULL, 0, NULL );
}

static HRESULT send_message( WS_CHANNEL *channel, WS_MESSAGE *msg, WS_MESSAGE_DESCRIPTION *desc,
                             WS_PARAMETER_DESCRIPTION *params, ULONG count, const void **args )
{
    WS_XML_WRITER *writer;
    HRESULT hr;

    if ((hr = channel_address_message( channel, msg )) != S_OK) return hr;
    if ((hr = message_set_action( msg, desc->action )) != S_OK) return hr;
    if ((hr = WsCreateWriter( NULL, 0, &writer, NULL )) != S_OK) return hr;
    if ((hr = set_output( writer )) != S_OK) goto done;
    if ((hr = write_message( msg, writer, desc->bodyElementDescription, params, count, args )) != S_OK) goto done;
    hr = channel_send_message( channel, msg );

done:
    WsFreeWriter( writer );
    return hr;
}

static HRESULT read_message( WS_MESSAGE *msg, WS_XML_READER *reader, WS_HEAP *heap,
                             const WS_ELEMENT_DESCRIPTION *desc, const WS_PARAMETER_DESCRIPTION *params,
                             ULONG count, const void **args, WS_ERROR *error )
{
    HRESULT hr;
    if ((hr = WsReadEnvelopeStart( msg, reader, NULL, NULL, NULL )) != S_OK) return hr;
    message_do_receive_callback( msg );
    if ((hr = message_read_fault( msg, heap, error )) != S_OK) return hr;
    if ((hr = read_output_params( reader, heap, desc, params, count, args )) != S_OK) return hr;
    return WsReadEnvelopeEnd( msg, NULL );
}

static HRESULT receive_message( WS_CHANNEL *channel, WS_MESSAGE *msg, WS_MESSAGE_DESCRIPTION *desc,
                                WS_PARAMETER_DESCRIPTION *params, ULONG count,
                                WS_HEAP *heap, const void **args, WS_ERROR *error )
{
    WS_XML_READER *reader;
    HRESULT hr;

    if ((hr = message_set_action( msg, desc->action )) != S_OK) return hr;
    if ((hr = channel_receive_message( channel, msg )) != S_OK) return hr;
    if ((hr = channel_get_reader( channel, &reader )) != S_OK) return hr;
    return read_message( msg, reader, heap, desc->bodyElementDescription, params, count, args, error );
}

static HRESULT create_input_message( WS_CHANNEL *channel, const WS_CALL_PROPERTY *properties,
                                     ULONG count, WS_MESSAGE **ret )
{
    WS_MESSAGE *msg;
    HRESULT hr;

    if ((hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL )) != S_OK) return hr;
    if ((hr = WsInitializeMessage( msg, WS_REQUEST_MESSAGE, NULL, NULL )) != S_OK ||
        (hr = set_send_context( msg, properties, count )) != S_OK)
    {
        WsFreeMessage( msg );
        return hr;
    }
    *ret = msg;
    return S_OK;
}

static HRESULT create_output_message( WS_CHANNEL *channel, const WS_CALL_PROPERTY *properties,
                                      ULONG count, WS_MESSAGE **ret )
{
    WS_MESSAGE *msg;
    HRESULT hr;

    if ((hr = WsCreateMessageForChannel( channel, NULL, 0, &msg, NULL )) != S_OK) return hr;
    if ((hr = set_receive_context( msg, properties, count )) != S_OK)
    {
        WsFreeMessage( msg );
        return hr;
    }
    *ret = msg;
    return S_OK;
}

/**************************************************************************
 *          WsCall		[webservices.@]
 */
HRESULT WINAPI WsCall( WS_SERVICE_PROXY *handle, const WS_OPERATION_DESCRIPTION *desc, const void **args,
                       WS_HEAP *heap, const WS_CALL_PROPERTY *properties, const ULONG count,
                       const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;
    WS_MESSAGE *msg = NULL;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %p %p %p %p %lu %p %p\n", handle, desc, args, heap, properties, count, ctx, error );
    if (ctx) FIXME( "ignoring ctx parameter\n" );
    for (i = 0; i < count; i++)
    {
        if (properties[i].id != WS_CALL_PROPERTY_SEND_MESSAGE_CONTEXT &&
            properties[i].id != WS_CALL_PROPERTY_RECEIVE_MESSAGE_CONTEXT)
        {
            FIXME( "unimplemented call property %u\n", properties[i].id );
            return E_NOTIMPL;
        }
    }

    if (!proxy || !desc || (desc->parameterCount && !args)) return E_INVALIDARG;

    EnterCriticalSection( &proxy->cs );

    if (proxy->magic != PROXY_MAGIC)
    {
        LeaveCriticalSection( &proxy->cs );
        return E_INVALIDARG;
    }

    if ((hr = create_input_message( proxy->channel, properties, count, &msg )) != S_OK) goto done;
    if ((hr = send_message( proxy->channel, msg, desc->inputMessageDescription, desc->parameterDescription,
                            desc->parameterCount, args )) != S_OK) goto done;
    WsFreeMessage( msg );
    msg = NULL;

    if ((hr = create_output_message( proxy->channel, properties, count, &msg )) != S_OK) goto done;
    hr = receive_message( proxy->channel, msg, desc->outputMessageDescription, desc->parameterDescription,
                          desc->parameterCount, heap, args, error );

done:
    WsFreeMessage( msg );
    LeaveCriticalSection( &proxy->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}
