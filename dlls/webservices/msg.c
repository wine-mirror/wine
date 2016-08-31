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
#include "wine/unicode.h"
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

struct header
{
    WS_HEADER_TYPE type;
    BOOL           mapped;
    WS_XML_STRING  name;
    WS_XML_STRING  ns;
    union
    {
        WS_XML_BUFFER *buf;
        WS_XML_STRING *text;
    } u;
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
    WS_XML_READER            *reader_body;
    ULONG                     header_count;
    ULONG                     header_size;
    struct header           **header;
    ULONG                     prop_count;
    struct prop               prop[sizeof(msg_props)/sizeof(msg_props[0])];
};

#define HEADER_ARRAY_SIZE 2
static struct msg *alloc_msg(void)
{
    static const ULONG count = sizeof(msg_props)/sizeof(msg_props[0]);
    struct msg *ret;
    ULONG size = sizeof(*ret) + prop_size( msg_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    if (!(ret->header = heap_alloc( HEADER_ARRAY_SIZE * sizeof(struct header *) )))
    {
        heap_free( ret );
        return NULL;
    }
    ret->state       = WS_MESSAGE_STATE_EMPTY;
    ret->header_size = HEADER_ARRAY_SIZE;
    prop_init( msg_props, count, ret->prop, &ret[1] );
    ret->prop_count  = count;
    return ret;
}

static void free_header( struct header *header )
{
    heap_free( header->name.bytes );
    heap_free( header->ns.bytes );
    if (header->mapped) heap_free( header->u.text );
    heap_free( header );
}

static void free_msg( struct msg *msg )
{
    ULONG i;
    if (!msg) return;
    WsFreeWriter( msg->writer );
    WsFreeHeap( msg->heap );
    heap_free( msg->addr.chars );
    for (i = 0; i < msg->header_count; i++) free_header( msg->header[i] );
    heap_free( msg->header );
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

    case WS_ADDRESSING_VERSION_TRANSPORT:
        str->bytes  = NULL;
        str->length = 0;
        return S_OK;

    default:
        ERR( "unhandled addressing version %u\n", ver );
        return E_NOTIMPL;
    }
}

static const WS_XML_STRING *get_header_name( WS_HEADER_TYPE type )
{
    static const WS_XML_STRING action = {6, (BYTE *)"Action"}, to = {2, (BYTE *)"To"};
    static const WS_XML_STRING msgid = {9, (BYTE *)"MessageID"}, relto = {9, (BYTE *)"RelatesTo"};
    static const WS_XML_STRING from = {4, (BYTE *)"From"}, replyto = {7, (BYTE *)"ReplyTo"};
    static const WS_XML_STRING faultto = {7, (BYTE *)"FaultTo"};

    switch (type)
    {
    case WS_ACTION_HEADER:      return &action;
    case WS_TO_HEADER:          return &to;
    case WS_MESSAGE_ID_HEADER:  return &msgid;
    case WS_RELATES_TO_HEADER:  return &relto;
    case WS_FROM_HEADER:        return &from;
    case WS_REPLY_TO_HEADER:    return &replyto;
    case WS_FAULT_TO_HEADER:    return &faultto;
    default:
        ERR( "unknown type %u\n", type );
        return NULL;
    }
}

static HRESULT write_envelope_start( struct msg *msg, WS_XML_WRITER *writer )
{
    static const char anonymous[] = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
    static const WS_XML_STRING prefix_s = {1, (BYTE *)"s"}, prefix_a = {1, (BYTE *)"a"};
    static const WS_XML_STRING envelope = {8, (BYTE *)"Envelope"}, header = {6, (BYTE *)"Header"};
    static const WS_XML_STRING msgid = {9, (BYTE *)"MessageID"}, replyto = {7, (BYTE *)"ReplyTo"};
    static const WS_XML_STRING address = {7, (BYTE *)"Address"}, body = {4, (BYTE *)"Body"};
    WS_XML_STRING ns_env, ns_addr;
    WS_XML_UTF8_TEXT urn, addr;
    HRESULT hr;
    ULONG i;

    if ((hr = get_env_namespace( msg->version_env, &ns_env )) != S_OK) return hr;
    if ((hr = get_addr_namespace( msg->version_addr, &ns_addr )) != S_OK) return hr;

    if ((hr = WsWriteStartElement( writer, &prefix_s, &envelope, &ns_env, NULL )) != S_OK) return hr;
    if (msg->version_addr < WS_ADDRESSING_VERSION_TRANSPORT &&
        (hr = WsWriteXmlnsAttribute( writer, &prefix_a, &ns_addr, FALSE, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartElement( writer, &prefix_s, &header, &ns_env, NULL )) != S_OK) return hr;

    if (msg->version_addr < WS_ADDRESSING_VERSION_TRANSPORT)
    {
        if ((hr = WsWriteStartElement( writer, &prefix_a, &msgid, &ns_addr, NULL )) != S_OK) return hr;
        urn.text.textType = WS_XML_TEXT_TYPE_UNIQUE_ID;
        memcpy( &urn.value, &msg->id, sizeof(msg->id) );
        if ((hr = WsWriteText( writer, &urn.text, NULL )) != S_OK) return hr;
        if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </a:MessageID> */
    }

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->mapped) continue;
        if ((hr = WsWriteXmlBuffer( writer, msg->header[i]->u.buf, NULL )) != S_OK) return hr;
    }

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

static HRESULT write_envelope_end( WS_XML_WRITER *writer )
{
    HRESULT hr;
    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </s:Body> */
    return WsWriteEndElement( writer, NULL ); /* </s:Envelope> */
}

static HRESULT write_envelope( struct msg *msg )
{
    HRESULT hr;
    if (!msg->writer && (hr = WsCreateWriter( NULL, 0, &msg->writer, NULL )) != S_OK) return hr;
    if (!msg->buf && (hr = WsCreateXmlBuffer( msg->heap, NULL, 0, &msg->buf, NULL )) != S_OK) return hr;
    if ((hr = WsSetOutputToBuffer( msg->writer, msg->buf, NULL, 0, NULL )) != S_OK) return hr;
    if ((hr = write_envelope_start( msg, msg->writer )) != S_OK) return hr;
    return write_envelope_end( msg->writer );
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

    if (!handle || !writer) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    if ((hr = write_envelope( msg )) != S_OK) return hr;
    if ((hr = write_envelope_start( msg, writer )) != S_OK) return hr;

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

    if ((hr = write_envelope_end( msg->writer_body )) != S_OK) return hr;

    msg->state = WS_MESSAGE_STATE_DONE;
    return S_OK;
}

/**************************************************************************
 *          WsWriteBody		[webservices.@]
 */
HRESULT WINAPI WsWriteBody( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc, WS_WRITE_OPTION option,
                            const void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p %08x %p %u %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !desc) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_WRITING) return WS_E_INVALID_OPERATION;

    if (desc->elementLocalName &&
        (hr = WsWriteStartElement( msg->writer_body, NULL, desc->elementLocalName, desc->elementNs,
                                   NULL )) != S_OK) return hr;

    if ((hr = WsWriteType( msg->writer_body, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription,
                           option, value, size, NULL )) != S_OK) return hr;

    if (desc->elementLocalName) hr = WsWriteEndElement( msg->writer_body, NULL );
    return hr;
}

static BOOL match_current_element( WS_XML_READER *reader, const WS_XML_STRING *localname )
{
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;

    if (WsGetReaderNode( reader, &node, NULL ) != S_OK) return FALSE;
    if (node->nodeType != WS_XML_NODE_TYPE_ELEMENT) return FALSE;
    elem = (const WS_XML_ELEMENT_NODE *)node;
    return WsXmlStringEquals( elem->localName, localname, NULL ) == S_OK;
}

static HRESULT read_envelope_start( WS_XML_READER *reader )
{
    static const WS_XML_STRING envelope = {8, (BYTE *)"Envelope"}, body = {4, (BYTE *)"Body"};
    HRESULT hr;

    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
    if (!match_current_element( reader, &envelope )) return WS_E_INVALID_FORMAT;
    /* FIXME: read headers */
    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
    if (!match_current_element( reader, &body )) return WS_E_INVALID_FORMAT;
    return WsReadNode( reader, NULL );
}

/**************************************************************************
 *          WsReadEnvelopeStart		[webservices.@]
 */
HRESULT WINAPI WsReadEnvelopeStart( WS_MESSAGE *handle, WS_XML_READER *reader, WS_MESSAGE_DONE_CALLBACK cb,
                                    void *state, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p %p\n", handle, reader, cb, state, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (cb)
    {
        FIXME( "callback not supported\n" );
        return E_NOTIMPL;
    }

    if (!handle || !reader) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_EMPTY) return WS_E_INVALID_OPERATION;

    if ((hr = read_envelope_start( reader )) != S_OK) return hr;

    msg->reader_body = reader;
    msg->state       = WS_MESSAGE_STATE_READING;
    return S_OK;
}

static HRESULT read_envelope_end( WS_XML_READER *reader )
{
    HRESULT hr;
    if ((hr = WsReadEndElement( reader, NULL )) != S_OK) return hr; /* </s:Body> */
    return WsReadEndElement( reader, NULL ); /* </s:Envelope> */
}

/**************************************************************************
 *          WsReadEnvelopeEnd		[webservices.@]
 */
HRESULT WINAPI WsReadEnvelopeEnd( WS_MESSAGE *handle, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_READING) return WS_E_INVALID_OPERATION;

    if ((hr = read_envelope_end( msg->reader_body )) != S_OK) return hr;

    msg->state = WS_MESSAGE_STATE_DONE;
    return S_OK;
}

/**************************************************************************
 *          WsReadBody		[webservices.@]
 */
HRESULT WINAPI WsReadBody( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc, WS_READ_OPTION option,
                           WS_HEAP *heap, void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p %p %08x %p %p %u %p\n", handle, desc, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !desc) return E_INVALIDARG;
    if (msg->state != WS_MESSAGE_STATE_READING) return WS_E_INVALID_OPERATION;

    return WsReadElement( msg->reader_body, desc, option, heap, value, size, NULL );
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
    return write_envelope( msg );
}

static HRESULT grow_header_array( struct msg *msg, ULONG size )
{
    struct header **tmp;
    if (size <= msg->header_size) return S_OK;
    if (!(tmp = heap_realloc( msg->header, 2 * msg->header_size * sizeof(struct header *) )))
        return E_OUTOFMEMORY;
    msg->header = tmp;
    msg->header_size *= 2;
    return S_OK;
}

static struct header *alloc_header( WS_HEADER_TYPE type, BOOL mapped, const WS_XML_STRING *name,
                                    const WS_XML_STRING *ns )
{
    struct header *ret;
    if (!(ret = heap_alloc_zero( sizeof(*ret) ))) return NULL;
    if (name && name->length)
    {
        if (!(ret->name.bytes = heap_alloc( name->length )))
        {
            free_header( ret );
            return NULL;
        }
        memcpy( ret->name.bytes, name->bytes, name->length );
        ret->name.length = name->length;
    }
    if (ns && ns->length)
    {
        if (!(ret->ns.bytes = heap_alloc( ns->length )))
        {
            free_header( ret );
            return NULL;
        }
        memcpy( ret->ns.bytes, ns->bytes, ns->length );
        ret->ns.length = ns->length;
    }
    ret->type   = type;
    ret->mapped = mapped;
    return ret;
}

static HRESULT write_standard_header( WS_XML_WRITER *writer, const WS_XML_STRING *name, WS_TYPE value_type,
                                      WS_WRITE_OPTION option, const void *value, ULONG size )
{
    static const WS_XML_STRING prefix_s = {1, (BYTE *)"s"}, prefix_a = {1, (BYTE *)"a"};
    static const WS_XML_STRING understand = {14, (BYTE *)"mustUnderstand"}, ns = {0, NULL};
    WS_XML_INT32_TEXT one = {{WS_XML_TEXT_TYPE_INT32}, 1};
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, &prefix_a, name, &ns, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartAttribute( writer, &prefix_s, &understand, &ns, FALSE, NULL )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &one.text, NULL )) != S_OK) return hr;
    if ((hr = WsWriteEndAttribute( writer, NULL )) != S_OK) return hr;
    if ((hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, value_type, NULL, option, value, size,
                           NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL );
}

static HRESULT build_standard_header( WS_HEAP *heap, WS_HEADER_TYPE type, WS_TYPE value_type,
                                      WS_WRITE_OPTION option, const void *value, ULONG size,
                                      struct header **ret )
{
    const WS_XML_STRING *name = get_header_name( type );
    struct header *header;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buf;
    HRESULT hr;

    if (!(header = alloc_header( type, FALSE, name, NULL ))) return E_OUTOFMEMORY;

    if ((hr = WsCreateWriter( NULL, 0, &writer, NULL )) != S_OK) goto done;
    if ((hr = WsCreateXmlBuffer( heap, NULL, 0, &buf, NULL )) != S_OK) goto done;
    if ((hr = WsSetOutputToBuffer( writer, buf, NULL, 0, NULL )) != S_OK) goto done;
    if ((hr = write_standard_header( writer, name, value_type, option, value, size )) != S_OK)
        goto done;

    header->u.buf = buf;

done:
    if (hr != S_OK) free_header( header );
    else *ret = header;
    WsFreeWriter( writer );
    return hr;
}

/**************************************************************************
 *          WsSetHeader		[webservices.@]
 */
HRESULT WINAPI WsSetHeader( WS_MESSAGE *handle, WS_HEADER_TYPE type, WS_TYPE value_type,
                            WS_WRITE_OPTION option, const void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    struct header *header;
    BOOL found = FALSE;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %u %u %08x %p %u %p\n", handle, type, value_type, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type == type)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        if ((hr = grow_header_array( msg, msg->header_count + 1 )) != S_OK) return hr;
        i = msg->header_count;
    }

    if ((hr = build_standard_header( msg->heap, type, value_type, option, value, size, &header )) != S_OK)
        return hr;

    if (!found) msg->header_count++;
    else free_header( msg->header[i] );

    msg->header[i] = header;
    return write_envelope( msg );
}

static void remove_header( struct msg *msg, ULONG i )
{
    free_header( msg->header[i] );
    memmove( &msg->header[i], &msg->header[i + 1], (msg->header_count - i) * sizeof(struct header *) );
    msg->header_count--;
}

/**************************************************************************
 *          WsRemoveHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveHeader( WS_MESSAGE *handle, WS_HEADER_TYPE type, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    BOOL removed = FALSE;
    ULONG i;

    TRACE( "%p %u %p\n", handle, type, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;
    if (type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER) return E_INVALIDARG;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type == type)
        {
            remove_header( msg, i );
            removed = TRUE;
            break;
        }
    }

    if (removed) return write_envelope( msg );
    return S_OK;
}

static HRESULT build_mapped_header( const WS_XML_STRING *name, WS_TYPE type, WS_WRITE_OPTION option,
                                    const void *value, ULONG size, struct header **ret )
{
    struct header *header;

    if (!(header = alloc_header( 0, TRUE, name, NULL ))) return E_OUTOFMEMORY;
    switch (type)
    {
    case WS_WSZ_TYPE:
    {
        int len;
        const WCHAR *src;

        if (option != WS_WRITE_REQUIRED_POINTER || size != sizeof(WCHAR *))
        {
            free_header( header );
            return E_INVALIDARG;
        }
        src = *(const WCHAR **)value;
        len = WideCharToMultiByte( CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL ) - 1;
        if (!(header->u.text = alloc_xml_string( NULL, len )))
        {
            free_header( header );
            return E_OUTOFMEMORY;
        }
        WideCharToMultiByte( CP_UTF8, 0, src, -1, (char *)header->u.text->bytes, len, NULL, NULL );
        break;
    }
    case WS_XML_STRING_TYPE:
    {
        const WS_XML_STRING *str = value;

        if (option != WS_WRITE_REQUIRED_VALUE)
        {
            FIXME( "unhandled write option %u\n", option );
            free_header( header );
            return E_NOTIMPL;
        }
        if (size != sizeof(*str))
        {
            free_header( header );
            return E_INVALIDARG;
        }
        if (!(header->u.text = alloc_xml_string( NULL, str->length )))
        {
            free_header( header );
            return E_OUTOFMEMORY;
        }
        memcpy( header->u.text->bytes, str->bytes, str->length );
        break;
    }
    case WS_STRING_TYPE:
    {
        int len;
        const WS_STRING *str = value;

        if (option != WS_WRITE_REQUIRED_VALUE)
        {
            FIXME( "unhandled write option %u\n", option );
            free_header( header );
            return E_NOTIMPL;
        }
        if (size != sizeof(*str))
        {
            free_header( header );
            return E_INVALIDARG;
        }
        len = WideCharToMultiByte( CP_UTF8, 0, str->chars, str->length, NULL, 0, NULL, NULL );
        if (!(header->u.text = alloc_xml_string( NULL, len )))
        {
            free_header( header );
            return E_OUTOFMEMORY;
        }
        WideCharToMultiByte( CP_UTF8, 0, str->chars, str->length, (char *)header->u.text->bytes,
                             len, NULL, NULL );
        break;
    }
    default:
        FIXME( "unhandled type %u\n", type );
        free_header( header );
        return E_NOTIMPL;
    }

    *ret = header;
    return S_OK;
}

/**************************************************************************
 *          WsAddMappedHeader		[webservices.@]
 */
HRESULT WINAPI WsAddMappedHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, WS_TYPE type,
                                  WS_WRITE_OPTION option, const void *value, ULONG size , WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    struct header *header;
    BOOL found = FALSE;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %s %u %08x %p %u %p\n", handle, debugstr_xmlstr(name), type, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !name) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type || !msg->header[i]->mapped) continue;
        if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        if ((hr = grow_header_array( msg, msg->header_count + 1 )) != S_OK) return hr;
        i = msg->header_count;
    }

    if ((hr = build_mapped_header( name, type, option, value, size, &header )) != S_OK) return hr;

    if (!found) msg->header_count++;
    else free_header( msg->header[i] );

    msg->header[i] = header;
    return S_OK;
}

/**************************************************************************
 *          WsRemoveMappedHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveMappedHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    ULONG i;

    TRACE( "%p %s %p\n", handle, debugstr_xmlstr(name), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !name) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type || !msg->header[i]->mapped) continue;
        if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK)
        {
            remove_header( msg, i );
            break;
        }
    }

    return S_OK;
}

static HRESULT write_custom_header( WS_XML_WRITER *writer, const WS_XML_STRING *name, const WS_XML_STRING *ns,
                                    WS_TYPE type, const void *desc, WS_WRITE_OPTION option, const void *value,
                                    ULONG size )
{
    HRESULT hr;
    if ((hr = WsWriteStartElement( writer, NULL, name, ns, NULL )) != S_OK) return hr;
    if ((hr = WsWriteType( writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, type, desc, option, value, size,
                           NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL );
}

static HRESULT build_custom_header( WS_HEAP *heap, const WS_XML_STRING *name, const WS_XML_STRING *ns,
                                    WS_TYPE type, const void *desc, WS_WRITE_OPTION option, const void *value,
                                    ULONG size, struct header **ret )
{
    struct header *header;
    WS_XML_WRITER *writer;
    WS_XML_BUFFER *buf;
    HRESULT hr;

    if (!(header = alloc_header( 0, FALSE, name, ns ))) return E_OUTOFMEMORY;

    if ((hr = WsCreateWriter( NULL, 0, &writer, NULL )) != S_OK) goto done;
    if ((hr = WsCreateXmlBuffer( heap, NULL, 0, &buf, NULL )) != S_OK) goto done;
    if ((hr = WsSetOutputToBuffer( writer, buf, NULL, 0, NULL )) != S_OK) goto done;
    if ((hr = write_custom_header( writer, name, ns, type, desc, option, value, size )) != S_OK) goto done;

    header->u.buf = buf;

done:
    if (hr != S_OK) free_header( header );
    else *ret = header;
    WsFreeWriter( writer );
    return hr;
}

/**************************************************************************
 *          WsAddCustomHeader		[webservices.@]
 */
HRESULT WINAPI WsAddCustomHeader( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size, ULONG attrs, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    struct header *header;
    HRESULT hr;

    TRACE( "%p %p %08x %p %u %08x %p\n", handle, desc, option, value, size, attrs, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !desc) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    if ((hr = grow_header_array( msg, msg->header_count + 1 )) != S_OK) return hr;
    if ((hr = build_custom_header( msg->heap, desc->elementLocalName, desc->elementNs, desc->type,
                                   desc->typeDescription, option, value, size, &header )) != S_OK) return hr;

    msg->header[msg->header_count++] = header;
    return write_envelope( msg );
}

/**************************************************************************
 *          WsRemoveCustomHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveCustomHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, const WS_XML_STRING *ns,
                                     WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    BOOL removed = FALSE;
    ULONG i;

    TRACE( "%p %s %s %p\n", handle, debugstr_xmlstr(name), debugstr_xmlstr(ns), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !name || !ns) return E_INVALIDARG;
    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) return WS_E_INVALID_OPERATION;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type || msg->header[i]->mapped) continue;
        if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK &&
            WsXmlStringEquals( ns, &msg->header[i]->ns, NULL ) == S_OK)
        {
            remove_header( msg, i );
            removed = TRUE;
            break;
        }
    }

    if (removed) return write_envelope( msg );
    return S_OK;
}
