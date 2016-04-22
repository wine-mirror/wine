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
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc channel_props[] =
{
    { sizeof(ULONG), FALSE },                   /* WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE */
    { sizeof(UINT64), FALSE },                  /* WS_CHANNEL_PROPERTY_MAX_STREAMED_MESSAGE_SIZE */
    { sizeof(ULONG), FALSE },                   /* WS_CHANNEL_PROPERTY_MAX_STREAMED_START_SIZE */
    { sizeof(ULONG), FALSE },                   /* WS_CHANNEL_PROPERTY_MAX_STREAMED_FLUSH_SIZE */
    { sizeof(WS_ENCODING), FALSE },             /* WS_CHANNEL_PROPERTY_ENCODING */
    { sizeof(WS_ENVELOPE_VERSION), FALSE },     /* WS_CHANNEL_PROPERTY_ENVELOPE_VERSION */
    { sizeof(WS_ADDRESSING_VERSION), FALSE },   /* WS_CHANNEL_PROPERTY_ADDRESSING_VERSION */
    { sizeof(ULONG), FALSE },                   /* WS_CHANNEL_PROPERTY_MAX_SESSION_DICTIONARY_SIZE */
    { sizeof(WS_CHANNEL_STATE), TRUE },         /* WS_CHANNEL_PROPERTY_STATE */
};

static struct channel *alloc_channel(void)
{
    static const ULONG count = sizeof(channel_props)/sizeof(channel_props[0]);
    struct channel *ret;
    ULONG size = sizeof(*ret) + prop_size( channel_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    prop_init( channel_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

void free_channel( struct channel *channel )
{
    heap_free( channel );
}

HRESULT create_channel( WS_CHANNEL_TYPE type, WS_CHANNEL_BINDING binding,
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

    if (type != WS_CHANNEL_TYPE_REQUEST)
    {
        FIXME( "channel type %u not implemented\n", type );
        return E_NOTIMPL;
    }
    if (binding != WS_HTTP_CHANNEL_BINDING)
    {
        FIXME( "channel binding %u not implemented\n", binding );
        return E_NOTIMPL;
    }

    if ((hr = create_channel( type, binding, properties, count, &channel )) != S_OK)
        return hr;

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
    free_channel( channel );
}

/**************************************************************************
 *          WsGetChannelProperty		[webservices.@]
 */
HRESULT WINAPI WsGetChannelProperty( WS_CHANNEL *handle, WS_CHANNEL_PROPERTY_ID id, void *buf,
                                     ULONG size, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    return prop_get( channel->prop, channel->prop_count, id, buf, size );
}

/**************************************************************************
 *          WsSetChannelProperty		[webservices.@]
 */
HRESULT WINAPI WsSetChannelProperty( WS_CHANNEL *handle, WS_CHANNEL_PROPERTY_ID id, const void *value,
                                     ULONG size, WS_ERROR *error )
{
    struct channel *channel = (struct channel *)handle;

    TRACE( "%p %u %p %u\n", handle, id, value, size );
    if (error) FIXME( "ignoring error parameter\n" );

    return prop_set( channel->prop, channel->prop_count, id, value, size );
}

HRESULT open_channel( struct channel *channel, const WS_ENDPOINT_ADDRESS *endpoint )
{
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

    TRACE( "%p %p %p %p\n", handle, endpoint, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!endpoint) return E_INVALIDARG;
    if (channel->state != WS_CHANNEL_STATE_CREATED) return WS_E_INVALID_OPERATION;

    return open_channel( channel, endpoint );
}

HRESULT close_channel( struct channel *channel )
{
    channel->state = WS_CHANNEL_STATE_CLOSED;
    return S_OK;
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

    return close_channel( channel );
}
