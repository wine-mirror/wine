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

#include <stdio.h>
#include <stdarg.h>

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
    WS_CHANNEL     *channel;
    ULONG           prop_count;
    struct prop     prop[sizeof(proxy_props)/sizeof(proxy_props[0])];
};

static struct proxy *alloc_proxy(void)
{
    static const ULONG count = sizeof(proxy_props)/sizeof(proxy_props[0]);
    struct proxy *ret;
    ULONG size = sizeof(*ret) + prop_size( proxy_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    prop_init( proxy_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_proxy( struct proxy *proxy )
{
    if (!proxy) return;
    WsFreeChannel( proxy->channel );
    heap_free( proxy );
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

    TRACE( "%u %u %p %p %u %p %u %p %p\n", type, binding, desc, proxy_props, proxy_props_count,
           channel_props, channel_props_count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if ((hr = WsCreateChannel( type, binding, channel_props, channel_props_count, NULL, &channel,
                               NULL )) != S_OK) return hr;

    if ((hr = create_proxy( channel, proxy_props, proxy_props_count, handle )) != S_OK)
        WsFreeChannel( channel );

    return hr;
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

    TRACE( "%u %p %u %u %p %u %p %u %p %p\n", channel_type, properties, count, type, value, size, desc,
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

    if ((hr = create_proxy( channel, properties, count, handle )) != S_OK) WsFreeChannel( channel );
    return hr;
}

/**************************************************************************
 *          WsFreeServiceProxy		[webservices.@]
 */
void WINAPI WsFreeServiceProxy( WS_SERVICE_PROXY *handle )
{
    struct proxy *proxy = (struct proxy *)handle;

    TRACE( "%p\n", handle );
    free_proxy( proxy );
}

/**************************************************************************
 *          WsGetServiceProxyProperty		[webservices.@]
 */
HRESULT WINAPI WsGetServiceProxyProperty( WS_SERVICE_PROXY *handle, WS_PROXY_PROPERTY_ID id,
                                          void *buf, ULONG size, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    return prop_get( proxy->prop, proxy->prop_count, id, buf, size );
}

/**************************************************************************
 *          WsOpenServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsOpenServiceProxy( WS_SERVICE_PROXY *handle, const WS_ENDPOINT_ADDRESS *endpoint,
                                   const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;

    TRACE( "%p %p %p %p\n", handle, endpoint, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!handle || !endpoint) return E_INVALIDARG;
    return WsOpenChannel( proxy->channel, endpoint, NULL, NULL );
}

/**************************************************************************
 *          WsCloseServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsCloseServiceProxy( WS_SERVICE_PROXY *handle, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct proxy *proxy = (struct proxy *)handle;

    TRACE( "%p %p %p\n", handle, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!handle) return E_INVALIDARG;
    return WsCloseChannel( proxy->channel, NULL, NULL );
}

/**************************************************************************
 *          WsAbortServiceProxy		[webservices.@]
 */
HRESULT WINAPI WsAbortServiceProxy( WS_SERVICE_PROXY *handle, WS_ERROR *error )
{
    FIXME( "%p %p\n", handle, error );
    return E_NOTIMPL;
}

/**************************************************************************
 *          WsCall		[webservices.@]
 */
HRESULT WINAPI WsCall( WS_SERVICE_PROXY *handle, const WS_OPERATION_DESCRIPTION *desc, const void **args,
                       WS_HEAP *heap, const WS_CALL_PROPERTY *properties, const ULONG count,
                       const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    FIXME( "%p %p %p %p %p %u %p %p\n", handle, desc, args, heap, properties, count, ctx, error );
    return E_NOTIMPL;
}
