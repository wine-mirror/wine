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

static const struct
{
    ULONG size;
    BOOL  readonly;
}
channel_props[] =
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
    ULONG i, size = sizeof(*ret) + count * sizeof(WS_CHANNEL_PROPERTY);
    char *ptr;

    for (i = 0; i < count; i++) size += channel_props[i].size;
    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ptr = (char *)&ret->prop[count];
    for (i = 0; i < count; i++)
    {
        ret->prop[i].value = ptr;
        ret->prop[i].valueSize = channel_props[i].size;
        ptr += ret->prop[i].valueSize;
    }
    ret->prop_count = count;
    return ret;
}

static HRESULT set_channel_prop( struct channel *channel, WS_CHANNEL_PROPERTY_ID id, const void *value,
                                 ULONG size )
{
    if (id >= channel->prop_count || size != channel_props[id].size || channel_props[id].readonly)
        return E_INVALIDARG;

    memcpy( channel->prop[id].value, value, size );
    return S_OK;
}

static HRESULT get_channel_prop( struct channel *channel, WS_CHANNEL_PROPERTY_ID id, void *buf, ULONG size )
{
    if (id >= channel->prop_count || size != channel_props[id].size)
        return E_INVALIDARG;

    memcpy( buf, channel->prop[id].value, channel->prop[id].valueSize );
    return S_OK;
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

    set_channel_prop( channel, WS_CHANNEL_PROPERTY_MAX_BUFFERED_MESSAGE_SIZE, &msg_size, sizeof(msg_size) );

    for (i = 0; i < count; i++)
    {
        hr = set_channel_prop( channel, properties[i].id, properties[i].value, properties[i].valueSize );
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

    return get_channel_prop( channel, id, buf, size );
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

    return set_channel_prop( channel, id, value, size );
}
