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

#include "windef.h"
#include "winbase.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

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
    ULONG                   prop_count;
    struct prop             prop[sizeof(listener_props)/sizeof(listener_props[0])];
};

#define LISTENER_MAGIC (('L' << 24) | ('I' << 16) | ('S' << 8) | 'T')

static struct listener *alloc_listener(void)
{
    static const ULONG count = sizeof(listener_props)/sizeof(listener_props[0]);
    struct listener *ret;
    ULONG size = sizeof(*ret) + prop_size( listener_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ret->magic      = LISTENER_MAGIC;
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": listener.cs");

    prop_init( listener_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_listener( struct listener *listener )
{
    listener->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &listener->cs );
    heap_free( listener );
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

    TRACE( "%u %u %p %u %p %p %p\n", type, binding, properties, count, desc, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (desc) FIXME( "ignoring security description\n" );

    if (!handle) return E_INVALIDARG;

    if (type != WS_CHANNEL_TYPE_DUPLEX_SESSION)
    {
        FIXME( "channel type %u not implemented\n", type );
        return E_NOTIMPL;
    }
    if (binding != WS_TCP_CHANNEL_BINDING)
    {
        FIXME( "channel binding %u not implemented\n", binding );
        return E_NOTIMPL;
    }

    if ((hr = create_listener( type, binding, properties, count, &listener )) != S_OK) return hr;

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
