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

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const char ns_env_1_1[] = "http://schemas.xmlsoap.org/soap/envelope/";
static const char ns_env_1_2[] = "http://www.w3.org/2003/05/soap-envelope";
static const char ns_addr_0_9[] = "http://schemas.xmlsoap.org/ws/2004/08/addressing";
static const char ns_addr_1_0[] = "http://www.w3.org/2005/08/addressing";

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
    WS_MESSAGE_INITIALIZATION init;
    WS_MESSAGE_STATE          state;
    GUID                      id;
    WS_ENVELOPE_VERSION       version_env;
    WS_ADDRESSING_VERSION     version_addr;
    BOOL                      is_addressed;
    WS_STRING                 addr;
    WS_HEAP                  *heap;
    WS_XML_BUFFER            *buf;
    WS_XML_WRITER            *writer;
    WS_XML_WRITER            *writer_body;
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
    if (!msg) return;
    WsFreeWriter( msg->writer );
    WsFreeHeap( msg->heap );
    heap_free( msg->addr.chars );
    heap_free( msg );
}

#define HEAP_MAX_SIZE (1 << 16)

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

    if ((hr = WsCreateHeap( HEAP_MAX_SIZE, 0, NULL, 0, &msg->heap, NULL )) != S_OK)
    {
        free_msg( msg );
        return hr;
    }
    if ((hr = WsCreateXmlBuffer( msg->heap, NULL, 0, &msg->buf, NULL )) != S_OK)
    {
        free_msg( msg );
        return hr;
    }

    UuidCreate( &msg->id );
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
 *          WsInitializeMessage		[webservices.@]
 */
HRESULT WINAPI WsInitializeMessage( WS_MESSAGE *handle, WS_MESSAGE_INITIALIZATION init,
                                    WS_MESSAGE *src_handle, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p %u %p %p\n", handle, init, src_handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (src_handle)
    {
        FIXME( "src message not supported\n" );
        return E_NOTIMPL;
    }

    if (!handle || init > WS_FAULT_MESSAGE) return E_INVALIDARG;
    if (msg->state >= WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    msg->init  = init;
    msg->state = WS_MESSAGE_STATE_INITIALIZED;
    return S_OK;
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

    case WS_MESSAGE_PROPERTY_HEAP:
        if (!buf || size != sizeof(msg->heap)) return E_INVALIDARG;
        *(WS_HEAP **)buf = msg->heap;
        return S_OK;

    case WS_MESSAGE_PROPERTY_ENVELOPE_VERSION:
        if (!buf || size != sizeof(msg->version_env)) return E_INVALIDARG;
        *(WS_ENVELOPE_VERSION *)buf = msg->version_env;
        return S_OK;

    case WS_MESSAGE_PROPERTY_ADDRESSING_VERSION:
        if (!buf || size != sizeof(msg->version_addr)) return E_INVALIDARG;
        *(WS_ADDRESSING_VERSION *)buf = msg->version_addr;
        return S_OK;

    case WS_MESSAGE_PROPERTY_HEADER_BUFFER:
        if (!buf || size != sizeof(msg->buf)) return E_INVALIDARG;
        *(WS_XML_BUFFER **)buf = msg->buf;
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

/**************************************************************************
 *          WsAddressMessage		[webservices.@]
 */
HRESULT WINAPI WsAddressMessage( WS_MESSAGE *handle, const WS_ENDPOINT_ADDRESS *addr, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p %p %p\n", handle, addr, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (addr && (addr->headers || addr->extensions || addr->identity))
    {
        FIXME( "headers, extensions or identity not supported\n" );
        return E_NOTIMPL;
    }

    if (!handle) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED || msg->is_addressed) return WS_E_INVALID_OPERATION;

    if (addr && addr->url.length)
    {
        if (!(msg->addr.chars = heap_alloc( addr->url.length * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
        memcpy( msg->addr.chars, addr->url.chars, addr->url.length * sizeof(WCHAR) );
        msg->addr.length = addr->url.length;
    }

    msg->is_addressed = TRUE;
    return S_OK;
}

static HRESULT get_env_namespace( WS_ENVELOPE_VERSION ver, WS_XML_STRING *str )
{
    switch (ver)
    {
    case WS_ENVELOPE_VERSION_SOAP_1_1:
        str->bytes  = (BYTE *)ns_env_1_1;
        str->length = sizeof(ns_env_1_1)/sizeof(ns_env_1_1[0]) - 1;
        return S_OK;

    case WS_ENVELOPE_VERSION_SOAP_1_2:
        str->bytes  = (BYTE *)ns_env_1_2;
        str->length = sizeof(ns_env_1_2)/sizeof(ns_env_1_2[0]) - 1;
        return S_OK;

    default:
        ERR( "unhandled envelope version %u\n", ver );
        return E_NOTIMPL;
    }
}

static HRESULT get_addr_namespace( WS_ADDRESSING_VERSION ver, WS_XML_STRING *str )
{
    switch (ver)
    {
    case WS_ADDRESSING_VERSION_0_9:
        str->bytes  = (BYTE *)ns_addr_0_9;
        str->length = sizeof(ns_addr_0_9)/sizeof(ns_addr_0_9[0]) - 1;
        return S_OK;

    case WS_ADDRESSING_VERSION_1_0:
        str->bytes  = (BYTE *)ns_addr_1_0;
        str->length = sizeof(ns_addr_1_0)/sizeof(ns_addr_1_0[0]) - 1;
        return S_OK;

    default:
        ERR( "unhandled adressing version %u\n", ver );
        return E_NOTIMPL;
    }
}

static HRESULT write_envelope_start( struct msg *msg, WS_XML_WRITER *writer )
{
    static const char anonymous[] = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
    WS_XML_STRING prefix_s = {1, (BYTE *)"s"}, prefix_a = {1, (BYTE *)"a"}, body = {4, (BYTE *)"Body"};
    WS_XML_STRING envelope = {8, (BYTE *)"Envelope"}, header = {6, (BYTE *)"Header"};
    WS_XML_STRING msgid = {9, (BYTE *)"MessageID"}, replyto = {7, (BYTE *)"ReplyTo"};
    WS_XML_STRING address = {7, (BYTE *)"Address"}, ns_env, ns_addr;
    WS_XML_UTF8_TEXT urn, addr;
    HRESULT hr;

    if ((hr = get_env_namespace( msg->version_env, &ns_env )) != S_OK) return hr;
    if ((hr = get_addr_namespace( msg->version_addr, &ns_addr )) != S_OK) return hr;

    if ((hr = WsWriteStartElement( writer, &prefix_s, &envelope, &ns_env, NULL )) != S_OK) return hr;
    if ((hr = WsWriteXmlnsAttribute( writer, &prefix_a, &ns_addr, FALSE, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartElement( writer, &prefix_s, &header, &ns_env, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartElement( writer, &prefix_a, &msgid, &ns_addr, NULL )) != S_OK) return hr;

    urn.text.textType = WS_XML_TEXT_TYPE_UNIQUE_ID;
    memcpy( &urn.value, &msg->id, sizeof(msg->id) );
    if ((hr = WsWriteText( writer, &urn.text, NULL )) != S_OK) return hr;

    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </a:MessageID> */
    if (msg->version_addr == WS_ADDRESSING_VERSION_0_9)
    {
        if ((hr = WsWriteStartElement( writer, &prefix_a, &replyto, &ns_addr, NULL )) != S_OK) return hr;
        if ((hr = WsWriteStartElement( writer, &prefix_a, &address, &ns_addr, NULL )) != S_OK) return hr;

        addr.text.textType = WS_XML_TEXT_TYPE_UTF8;
        addr.value.bytes   = (BYTE *)anonymous;
        addr.value.length  = sizeof(anonymous) - 1;
        if ((hr = WsWriteText( writer, &addr.text, NULL )) != S_OK) return hr;
        if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </a:Address> */
        if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </a:ReplyTo> */
    }

    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </s:Header> */
    return WsWriteStartElement( writer, &prefix_s, &body, &ns_env, NULL ); /* <s:Body> */
}

static HRESULT write_envelope_end( struct msg *msg, WS_XML_WRITER *writer )
{
    HRESULT hr;
    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </s:Body> */
    return WsWriteEndElement( writer, NULL ); /* </s:Envelope> */
}

/**************************************************************************
 *          WsWriteEnvelopeStart		[webservices.@]
 */
HRESULT WINAPI WsWriteEnvelopeStart( WS_MESSAGE *handle, WS_XML_WRITER *writer,
                                     WS_MESSAGE_DONE_CALLBACK cb, void *state, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p %p\n", handle, writer, cb, state, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (cb)
    {
        FIXME( "callback not supported\n" );
        return E_NOTIMPL;
    }

    if (!handle) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    if ((hr = WsCreateWriter( NULL, 0, &msg->writer, NULL )) != S_OK) return hr;
    if ((hr = WsSetOutputToBuffer( msg->writer, msg->buf, NULL, 0, NULL )) != S_OK) return hr;
    if ((hr = write_envelope_start( msg, msg->writer )) != S_OK) return hr;
    if ((hr = write_envelope_start( msg, writer )) != S_OK) return hr;
    if ((hr = write_envelope_end( msg, msg->writer )) != S_OK) return hr;

    msg->writer_body = writer;
    msg->state       = WS_MESSAGE_STATE_WRITING;
    return S_OK;
}

/**************************************************************************
 *          WsWriteEnvelopeEnd		[webservices.@]
 */
HRESULT WINAPI WsWriteEnvelopeEnd( WS_MESSAGE *handle, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_WRITING) return WS_E_INVALID_OPERATION;

    if ((hr = write_envelope_end( msg, msg->writer_body )) != S_OK) return hr;

    msg->state = WS_MESSAGE_STATE_DONE;
    return S_OK;
}
