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
    struct channel *channel;
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
    free_channel( proxy->channel );
    heap_free( proxy );
}

static HRESULT create_proxy( struct channel *channel, const WS_PROXY_PROPERTY *properties, ULONG count,
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
    struct channel *channel;
    HRESULT hr;

    TRACE( "%u %u %p %p %u %p %u %p %p\n", type, binding, desc, proxy_props, proxy_props_count,
           channel_props, channel_props_count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if ((hr = create_channel( type, binding, channel_props, channel_props_count, &channel )) != S_OK)
        return hr;

    if ((hr = create_proxy( channel, proxy_props, proxy_props_count, handle )) != S_OK)
        free_channel( channel );

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
