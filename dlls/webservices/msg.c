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

static const struct prop_desc msg_props[] =
{
    { sizeof(WS_MESSAGE_STATE), TRUE },         /* WS_MESSAGE_PROPERTY_STATE */
    { sizeof(WS_HEAP *), TRUE },                /* WS_MESSAGE_PROPERTY_HEAP */
    { sizeof(WS_ENVELOPE_VERSION), TRUE },      /* WS_MESSAGE_PROPERTY_ENVELOPE_VERSION */
    { sizeof(WS_ADDRESSING_VERSION), TRUE },    /* WS_MESSAGE_PROPERTY_ADDRESSING_VERSION */
    { sizeof(WS_XML_BUFFER *), TRUE },          /* WS_MESSAGE_PROPERTY_HEADER_BUFFER */
    { sizeof(WS_XML_NODE_POSITION *), TRUE },   /* WS_MESSAGE_PROPERTY_HEADER_POSITION */
    { sizeof(WS_XML_READER *), TRUE },          /* WS_MESSAGE_PROPERTY_BODY_READER */
    { sizeof(WS_XML_WRITER *), TRUE },          /* WS_MESSAGE_PROPERTY_BODY_WRITER */
    { sizeof(BOOL), TRUE },                     /* WS_MESSAGE_PROPERTY_IS_ADDRESSED */
};

struct msg
{
    WS_MESSAGE_STATE          state;
    WS_ENVELOPE_VERSION       version_env;
    WS_ADDRESSING_VERSION     version_addr;
    BOOL                      is_addressed;
    ULONG                     prop_count;
    struct prop               prop[sizeof(msg_props)/sizeof(msg_props[0])];
};

static struct msg *alloc_msg(void)
{
    static const ULONG count = sizeof(msg_props)/sizeof(msg_props[0]);
    struct msg *ret;
    ULONG size = sizeof(*ret) + prop_size( msg_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    ret->state = WS_MESSAGE_STATE_EMPTY;
    prop_init( msg_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_msg( struct msg *msg )
{
    heap_free( msg );
}

static HRESULT create_msg( WS_ENVELOPE_VERSION env_version, WS_ADDRESSING_VERSION addr_version,
                           const WS_MESSAGE_PROPERTY *properties, ULONG count, WS_MESSAGE **handle )
{
    struct msg *msg;
    HRESULT hr;
    ULONG i;

    if (!(msg = alloc_msg())) return E_OUTOFMEMORY;

    for (i = 0; i < count; i++)
    {
        if (properties[i].id == WS_MESSAGE_PROPERTY_ENVELOPE_VERSION ||
            properties[i].id == WS_MESSAGE_PROPERTY_ADDRESSING_VERSION)
        {
            free_msg( msg );
            return E_INVALIDARG;
        }
        hr = prop_set( msg->prop, msg->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_msg( msg );
            return hr;
        }
    }

    msg->version_env  = env_version;
    msg->version_addr = addr_version;

    *handle = (WS_MESSAGE *)msg;
    return S_OK;
}

/**************************************************************************
 *          WsCreateMessage		[webservices.@]
 */
HRESULT WINAPI WsCreateMessage( WS_ENVELOPE_VERSION env_version, WS_ADDRESSING_VERSION addr_version,
                                const WS_MESSAGE_PROPERTY *properties, ULONG count, WS_MESSAGE **handle,
                                WS_ERROR *error )
{
    TRACE( "%u %u %p %u %p %p\n", env_version, addr_version, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !env_version || !addr_version) return E_INVALIDARG;
    return create_msg( env_version, addr_version, properties, count, handle );
}

/**************************************************************************
 *          WsCreateMessageForChannel		[webservices.@]
 */
HRESULT WINAPI WsCreateMessageForChannel( WS_CHANNEL *channel_handle, const WS_MESSAGE_PROPERTY *properties,
                                          ULONG count, WS_MESSAGE **handle, WS_ERROR *error )
{
    WS_ENVELOPE_VERSION version_env;
    WS_ADDRESSING_VERSION version_addr;
    HRESULT hr;

    TRACE( "%p %p %u %p %p\n", channel_handle, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel_handle || !handle) return E_INVALIDARG;

    if ((hr = WsGetChannelProperty( channel_handle, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION, &version_env,
                                    sizeof(version_env), NULL )) != S_OK || !version_env)
        version_env = WS_ENVELOPE_VERSION_SOAP_1_2;

    if ((hr = WsGetChannelProperty( channel_handle, WS_CHANNEL_PROPERTY_ADDRESSING_VERSION, &version_addr,
                                    sizeof(version_addr), NULL )) != S_OK || !version_addr)
        version_addr = WS_ADDRESSING_VERSION_1_0;

    return create_msg( version_env, version_addr, properties, count, handle );
}

/**************************************************************************
 *          WsFreeMessage		[webservices.@]
 */
void WINAPI WsFreeMessage( WS_MESSAGE *handle )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p\n", handle );
    free_msg( msg );
}

/**************************************************************************
 *          WsGetMessageProperty		[webservices.@]
 */
HRESULT WINAPI WsGetMessageProperty( WS_MESSAGE *handle, WS_MESSAGE_PROPERTY_ID id, void *buf,
                                     ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;

    switch (id)
    {
    case WS_MESSAGE_PROPERTY_STATE:
        if (!buf || size != sizeof(msg->state)) return E_INVALIDARG;
        *(WS_MESSAGE_STATE *)buf = msg->state;
        return S_OK;

    case WS_MESSAGE_PROPERTY_ENVELOPE_VERSION:
        if (!buf || size != sizeof(msg->version_env)) return E_INVALIDARG;
        *(WS_ENVELOPE_VERSION *)buf = msg->version_env;
        return S_OK;

    case WS_MESSAGE_PROPERTY_ADDRESSING_VERSION:
        if (!buf || size != sizeof(msg->version_addr)) return E_INVALIDARG;
        *(WS_ADDRESSING_VERSION *)buf = msg->version_addr;
        return S_OK;

    case WS_MESSAGE_PROPERTY_IS_ADDRESSED:
        if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;
        *(BOOL *)buf = msg->is_addressed;
        return S_OK;

    default:
        return prop_get( msg->prop, msg->prop_count, id, buf, size );
    }
}

/**************************************************************************
 *          WsSetMessageProperty		[webservices.@]
 */
HRESULT WINAPI WsSetMessageProperty( WS_MESSAGE *handle, WS_MESSAGE_PROPERTY_ID id, const void *value,
                                     ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p %u %p %u\n", handle, id, value, size );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;

    switch (id)
    {
    case WS_MESSAGE_PROPERTY_STATE:
    case WS_MESSAGE_PROPERTY_ENVELOPE_VERSION:
    case WS_MESSAGE_PROPERTY_ADDRESSING_VERSION:
    case WS_MESSAGE_PROPERTY_IS_ADDRESSED:
        if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;
        return E_INVALIDARG;

    default:
        break;
    }
    return prop_set( msg->prop, msg->prop_count, id, value, size );
}
