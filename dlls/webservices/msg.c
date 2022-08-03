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
#include "rpc.h"
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
    { sizeof(WS_HEAP_PROPERTIES), TRUE },       /* WS_MESSAGE_PROPERTY_HEAP_PROPERTIES */
    { sizeof(WS_XML_READER_PROPERTIES), TRUE }, /* WS_MESSAGE_PROPERTY_XML_READER_PROPERTIES */
    { sizeof(WS_XML_WRITER_PROPERTIES), TRUE }, /* WS_MESSAGE_PROPERTY_XML_WRITER_PROPERTIES */
    { sizeof(BOOL), FALSE },                    /* WS_MESSAGE_PROPERTY_IS_FAULT */
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
    ULONG                               magic;
    CRITICAL_SECTION                    cs;
    WS_MESSAGE_INITIALIZATION           init;
    WS_MESSAGE_STATE                    state;
    GUID                                id;
    GUID                                id_req;
    WS_ENVELOPE_VERSION                 version_env;
    WS_ADDRESSING_VERSION               version_addr;
    BOOL                                is_addressed;
    WS_STRING                           addr;
    WS_XML_STRING                      *action;
    WS_HEAP                            *heap;
    WS_XML_BUFFER                      *buf;
    WS_XML_WRITER                      *writer;
    WS_XML_WRITER                      *writer_body;
    WS_XML_READER                      *reader;
    WS_XML_READER                      *reader_body;
    ULONG                               header_count;
    ULONG                               header_size;
    struct header                     **header;
    WS_PROXY_MESSAGE_CALLBACK_CONTEXT   ctx_send;
    WS_PROXY_MESSAGE_CALLBACK_CONTEXT   ctx_receive;
    ULONG                               prop_count;
    struct prop                         prop[ARRAY_SIZE( msg_props )];
};

#define MSG_MAGIC (('M' << 24) | ('E' << 16) | ('S' << 8) | 'S')
#define HEADER_ARRAY_SIZE 2

static struct msg *alloc_msg(void)
{
    static const ULONG count = ARRAY_SIZE( msg_props );
    struct msg *ret;
    ULONG size = sizeof(*ret) + prop_size( msg_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;
    if (!(ret->header = malloc( HEADER_ARRAY_SIZE * sizeof(struct header *) )))
    {
        free( ret );
        return NULL;
    }
    ret->magic       = MSG_MAGIC;
    ret->state       = WS_MESSAGE_STATE_EMPTY;
    ret->header_size = HEADER_ARRAY_SIZE;

    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": msg.cs");

    prop_init( msg_props, count, ret->prop, &ret[1] );
    ret->prop_count  = count;
    return ret;
}

static void free_header( struct header *header )
{
    free( header->name.bytes );
    free( header->ns.bytes );
    if (header->mapped) free_xml_string( header->u.text );
    free( header );
}

static void reset_msg( struct msg *msg )
{
    BOOL isfault = FALSE;
    ULONG i;

    msg->state         = WS_MESSAGE_STATE_EMPTY;
    msg->init          = 0;
    UuidCreate( &msg->id );
    memset( &msg->id_req, 0, sizeof(msg->id_req) );
    msg->is_addressed  = FALSE;
    free( msg->addr.chars );
    msg->addr.chars    = NULL;
    msg->addr.length   = 0;

    free_xml_string( msg->action );
    msg->action = NULL;

    WsResetHeap( msg->heap, NULL );
    msg->buf           = NULL; /* allocated on msg->heap */
    msg->writer_body   = NULL; /* owned by caller */
    msg->reader_body   = NULL; /* owned by caller */

    for (i = 0; i < msg->header_count; i++)
    {
        free_header( msg->header[i] );
        msg->header[i] = NULL;
    }
    msg->header_count  = 0;

    memset( &msg->ctx_send, 0, sizeof(msg->ctx_send) );
    memset( &msg->ctx_receive, 0, sizeof(msg->ctx_receive) );

    prop_set( msg->prop, msg->prop_count, WS_MESSAGE_PROPERTY_IS_FAULT, &isfault, sizeof(isfault) );
}

static void free_msg( struct msg *msg )
{
    reset_msg( msg );

    WsFreeWriter( msg->writer );
    WsFreeReader( msg->reader );
    WsFreeHeap( msg->heap );
    free( msg->header );

    msg->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &msg->cs );
    free( msg );
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
    HRESULT hr;

    TRACE( "%u %u %p %lu %p %p\n", env_version, addr_version, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !env_version || !addr_version ||
        (env_version == WS_ENVELOPE_VERSION_NONE && addr_version != WS_ADDRESSING_VERSION_TRANSPORT))
    {
        return E_INVALIDARG;
    }

    if ((hr = create_msg( env_version, addr_version, properties, count, handle )) != S_OK) return hr;
    TRACE( "created %p\n", *handle );
    return S_OK;
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

    TRACE( "%p %p %lu %p %p\n", channel_handle, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!channel_handle || !handle) return E_INVALIDARG;

    if ((hr = WsGetChannelProperty( channel_handle, WS_CHANNEL_PROPERTY_ENVELOPE_VERSION, &version_env,
                                    sizeof(version_env), NULL )) != S_OK || !version_env)
        version_env = WS_ENVELOPE_VERSION_SOAP_1_2;

    if ((hr = WsGetChannelProperty( channel_handle, WS_CHANNEL_PROPERTY_ADDRESSING_VERSION, &version_addr,
                                    sizeof(version_addr), NULL )) != S_OK || !version_addr)
        version_addr = WS_ADDRESSING_VERSION_1_0;

    if ((hr = create_msg( version_env, version_addr, properties, count, handle )) != S_OK) return hr;
    TRACE( "created %p\n", *handle );
    return S_OK;
}

/**************************************************************************
 *          WsFreeMessage		[webservices.@]
 */
void WINAPI WsFreeMessage( WS_MESSAGE *handle )
{
    struct msg *msg = (struct msg *)handle;

    TRACE( "%p\n", handle );

    if (!msg) return;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return;
    }

    msg->magic = 0;

    LeaveCriticalSection( &msg->cs );
    free_msg( msg );
}

/**************************************************************************
 *          WsResetMessage		[webservices.@]
 */
HRESULT WINAPI WsResetMessage( WS_MESSAGE *handle, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    reset_msg( msg );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetMessageProperty		[webservices.@]
 */
HRESULT WINAPI WsGetMessageProperty( WS_MESSAGE *handle, WS_MESSAGE_PROPERTY_ID id, void *buf,
                                     ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    switch (id)
    {
    case WS_MESSAGE_PROPERTY_STATE:
        if (!buf || size != sizeof(msg->state)) hr = E_INVALIDARG;
        else *(WS_MESSAGE_STATE *)buf = msg->state;
        break;

    case WS_MESSAGE_PROPERTY_HEAP:
        if (!buf || size != sizeof(msg->heap)) hr = E_INVALIDARG;
        else *(WS_HEAP **)buf = msg->heap;
        break;

    case WS_MESSAGE_PROPERTY_ENVELOPE_VERSION:
        if (!buf || size != sizeof(msg->version_env)) hr = E_INVALIDARG;
        else *(WS_ENVELOPE_VERSION *)buf = msg->version_env;
        break;

    case WS_MESSAGE_PROPERTY_ADDRESSING_VERSION:
        if (!buf || size != sizeof(msg->version_addr)) hr = E_INVALIDARG;
        else *(WS_ADDRESSING_VERSION *)buf = msg->version_addr;
        break;

    case WS_MESSAGE_PROPERTY_HEADER_BUFFER:
        if (!buf || size != sizeof(msg->buf)) hr = E_INVALIDARG;
        else *(WS_XML_BUFFER **)buf = msg->buf;
        break;

    case WS_MESSAGE_PROPERTY_BODY_READER:
        if (!buf || size != sizeof(msg->reader_body)) hr = E_INVALIDARG;
        else *(WS_XML_READER **)buf = msg->reader_body;
        break;

    case WS_MESSAGE_PROPERTY_BODY_WRITER:
        if (!buf || size != sizeof(msg->writer_body)) hr = E_INVALIDARG;
        else *(WS_XML_WRITER **)buf = msg->writer_body;
        break;

    case WS_MESSAGE_PROPERTY_IS_ADDRESSED:
        if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
        else *(BOOL *)buf = msg->is_addressed;
        break;

    case WS_MESSAGE_PROPERTY_HEAP_PROPERTIES:
    case WS_MESSAGE_PROPERTY_XML_READER_PROPERTIES:
    case WS_MESSAGE_PROPERTY_XML_WRITER_PROPERTIES:
        FIXME( "property %u not supported\n", id );
        hr = E_NOTIMPL;
        break;

    default:
        hr = prop_get( msg->prop, msg->prop_count, id, buf, size );
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetMessageProperty		[webservices.@]
 */
HRESULT WINAPI WsSetMessageProperty( WS_MESSAGE *handle, WS_MESSAGE_PROPERTY_ID id, const void *value,
                                     ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu %p\n", handle, id, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    switch (id)
    {
    case WS_MESSAGE_PROPERTY_STATE:
    case WS_MESSAGE_PROPERTY_ENVELOPE_VERSION:
    case WS_MESSAGE_PROPERTY_ADDRESSING_VERSION:
    case WS_MESSAGE_PROPERTY_IS_ADDRESSED:
        if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
        else hr = E_INVALIDARG;
        break;

    default:
        hr = prop_set( msg->prop, msg->prop_count, id, value, size );
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsAddressMessage		[webservices.@]
 */
HRESULT WINAPI WsAddressMessage( WS_MESSAGE *handle, const WS_ENDPOINT_ADDRESS *addr, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, addr, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (addr && (addr->headers || addr->extensions || addr->identity))
    {
        FIXME( "headers, extensions or identity not supported\n" );
        return E_NOTIMPL;
    }

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED || msg->is_addressed) hr = WS_E_INVALID_OPERATION;
    else if (addr && addr->url.length)
    {
        if (!(msg->addr.chars = malloc( addr->url.length * sizeof(WCHAR) ))) hr = E_OUTOFMEMORY;
        else
        {
            memcpy( msg->addr.chars, addr->url.chars, addr->url.length * sizeof(WCHAR) );
            msg->addr.length = addr->url.length;
        }
    }
    if (hr == S_OK) msg->is_addressed = TRUE;

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static const WS_XML_STRING *get_env_namespace( WS_ENVELOPE_VERSION version )
{
    static const WS_XML_STRING namespaces[] =
    {
        {41, (BYTE *)"http://schemas.xmlsoap.org/soap/envelope/"},
        {39, (BYTE *)"http://www.w3.org/2003/05/soap-envelope"},
        {0, NULL},
    };

    if (version < WS_ENVELOPE_VERSION_SOAP_1_1 || version > WS_ENVELOPE_VERSION_NONE)
    {
        ERR( "unknown version %u\n", version );
        return NULL;
    }

    return &namespaces[version - 1];
}

static const WS_XML_STRING *get_addr_namespace( WS_ADDRESSING_VERSION version )
{
    static const WS_XML_STRING namespaces[] =
    {
        {48, (BYTE *)"http://schemas.xmlsoap.org/ws/2004/08/addressing"},
        {36, (BYTE *)"http://www.w3.org/2005/08/addressing"},
        {55, (BYTE *)"http://schemas.microsoft.com/ws/2005/05/addressing/none"},
    };

    if (version < WS_ADDRESSING_VERSION_0_9 || version > WS_ADDRESSING_VERSION_TRANSPORT)
    {
        ERR( "unknown version %u\n", version );
        return NULL;
    }

    return &namespaces[version - 1];
}

static const WS_XML_STRING *get_header_name( WS_HEADER_TYPE type )
{
    static const WS_XML_STRING headers[] =
    {
        {6, (BYTE *)"Action"},
        {2, (BYTE *)"To"},
        {9, (BYTE *)"MessageID"},
        {9, (BYTE *)"RelatesTo"},
        {4, (BYTE *)"From"},
        {7, (BYTE *)"ReplyTo"},
        {7, (BYTE *)"FaultTo"},
    };

    if (type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER)
    {
        ERR( "unknown type %u\n", type );
        return NULL;
    }

    return &headers[type - 1];
}

static HRESULT write_must_understand( WS_XML_WRITER *writer, const WS_XML_STRING *prefix, const WS_XML_STRING *ns )
{
    static const WS_XML_STRING understand = {14, (BYTE *)"mustUnderstand"};
    WS_XML_INT32_TEXT one = {{WS_XML_TEXT_TYPE_INT32}, 1};
    HRESULT hr;

    if ((hr = WsWriteStartAttribute( writer, prefix, &understand, ns, FALSE, NULL )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &one.text, NULL )) != S_OK) return hr;
    return WsWriteEndAttribute( writer, NULL );
}

static HRESULT write_action_header( WS_XML_WRITER *writer, const WS_XML_STRING *prefix_env,
                                    const WS_XML_STRING *ns_env, const WS_XML_STRING *prefix_addr,
                                    const WS_XML_STRING *ns_addr, const WS_XML_STRING *text )
{
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}};
    const WS_XML_STRING *action = get_header_name( WS_ACTION_HEADER );
    HRESULT hr;

    if (!text || !text->length) return S_OK;
    utf8.value.length = text->length;
    utf8.value.bytes  = text->bytes;

    if ((hr = WsWriteStartElement( writer, prefix_addr, action, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = write_must_understand( writer, prefix_env, ns_env )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &utf8.text, NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL ); /* </a:Action> */
}

static HRESULT write_to_header( WS_XML_WRITER *writer, const WS_XML_STRING *prefix_env,
                                const WS_XML_STRING *ns_env, const WS_XML_STRING *prefix_addr,
                                const WS_XML_STRING *ns_addr, const WS_STRING *addr )
{
    WS_XML_UTF16_TEXT utf16 = {{WS_XML_TEXT_TYPE_UTF16}, (BYTE *)addr->chars, addr->length * sizeof(WCHAR)};
    const WS_XML_STRING *to = get_header_name( WS_TO_HEADER );
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, prefix_addr, to, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = write_must_understand( writer, prefix_env, ns_env )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &utf16.text, NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL ); /* </a:To> */
}

static HRESULT write_replyto_header( WS_XML_WRITER *writer, const WS_XML_STRING *prefix_env,
                                     const WS_XML_STRING *ns_env, const WS_XML_STRING *prefix_addr,
                                     const WS_XML_STRING *ns_addr )
{
    static const char anonymous[] = "http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
    WS_XML_UTF8_TEXT utf8 = {{WS_XML_TEXT_TYPE_UTF8}, {sizeof(anonymous) - 1, (BYTE *)anonymous}};
    const WS_XML_STRING address = {7, (BYTE *)"Address"}, *replyto = get_header_name( WS_REPLY_TO_HEADER );
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, prefix_addr, replyto, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartElement( writer, prefix_addr, &address, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &utf8.text, NULL )) != S_OK) return hr;
    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </a:Address> */
    return WsWriteEndElement( writer, NULL ); /* </a:ReplyTo> */
}

static HRESULT write_msgid_header( WS_XML_WRITER *writer, const WS_XML_STRING *prefix_env,
                                   const WS_XML_STRING *ns_env, const WS_XML_STRING *prefix_addr,
                                   const WS_XML_STRING *ns_addr, const GUID *guid )
{
    WS_XML_UNIQUE_ID_TEXT id = {{WS_XML_TEXT_TYPE_UNIQUE_ID}, *guid};
    const WS_XML_STRING *msgid = get_header_name( WS_MESSAGE_ID_HEADER );
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, prefix_addr, msgid, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &id.text, NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL ); /* </a:MessageID> */
}

static HRESULT write_relatesto_header( WS_XML_WRITER *writer, const WS_XML_STRING *prefix_env,
                                       const WS_XML_STRING *ns_env, const WS_XML_STRING *prefix_addr,
                                       const WS_XML_STRING *ns_addr, const GUID *guid )
{
    WS_XML_UNIQUE_ID_TEXT id = {{WS_XML_TEXT_TYPE_UNIQUE_ID}, *guid};
    const WS_XML_STRING *relatesto = get_header_name( WS_RELATES_TO_HEADER );
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, prefix_addr, relatesto, ns_addr, NULL )) != S_OK) return hr;
    if ((hr = WsWriteText( writer, &id.text, NULL )) != S_OK) return hr;
    return WsWriteEndElement( writer, NULL ); /* </a:RelatesTo> */
}

static HRESULT write_headers( struct msg *msg, WS_MESSAGE_INITIALIZATION init, WS_XML_WRITER *writer,
                              const WS_XML_STRING *prefix_env, const WS_XML_STRING *ns_env,
                              const WS_XML_STRING *prefix_addr, const WS_XML_STRING *ns_addr )
{
    static const WS_XML_STRING header = {6, (BYTE *)"Header"};
    HRESULT hr;
    ULONG i;

    if ((hr = WsWriteXmlnsAttribute( writer, prefix_addr, ns_addr, FALSE, NULL )) != S_OK) return hr;
    if ((hr = WsWriteStartElement( writer, prefix_env, &header, ns_env, NULL )) != S_OK) return hr;

    if ((hr = write_action_header( writer, prefix_env, ns_env, prefix_addr, ns_addr, msg->action )) != S_OK)
        return hr;

    if (init == WS_REPLY_MESSAGE || init == WS_FAULT_MESSAGE)
    {
        if ((hr = write_relatesto_header( writer, prefix_env, ns_env, prefix_addr, ns_addr, &msg->id_req )) != S_OK)
            return hr;
    }
    else
    {
        if (init == WS_REQUEST_MESSAGE)
        {
            if ((hr = write_msgid_header(writer, prefix_env, ns_env, prefix_addr, ns_addr, &msg->id)) != S_OK)
                return hr;
            if (msg->version_addr == WS_ADDRESSING_VERSION_0_9 &&
                (hr = write_replyto_header(writer, prefix_env, ns_env, prefix_addr, ns_addr)) != S_OK)
                return hr;
        }

        if (msg->addr.length &&
            (hr = write_to_header(writer, prefix_env, ns_env, prefix_addr, ns_addr, &msg->addr)) != S_OK)
            return hr;
    }

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->mapped) continue;
        if ((hr = WsWriteXmlBuffer( writer, msg->header[i]->u.buf, NULL )) != S_OK) return hr;
    }

    return WsWriteEndElement( writer, NULL ); /* </s:Header> */
}

static ULONG count_envelope_headers( struct msg *msg )
{
    ULONG i, ret = 0;
    for (i = 0; i < msg->header_count; i++)
    {
        if (!msg->header[i]->mapped) ret++;
    }
    return ret;
}

static HRESULT write_headers_transport( struct msg *msg, WS_XML_WRITER *writer, const WS_XML_STRING *prefix,
                                        const WS_XML_STRING *ns )
{
    static const WS_XML_STRING header = {6, (BYTE *)"Header"};
    HRESULT hr = S_OK;
    ULONG i;

    if (count_envelope_headers( msg ) || !msg->action)
    {
        if ((hr = WsWriteStartElement( writer, prefix, &header, ns, NULL )) != S_OK) return hr;

        for (i = 0; i < msg->header_count; i++)
        {
            if (msg->header[i]->mapped) continue;
            if ((hr = WsWriteXmlBuffer( writer, msg->header[i]->u.buf, NULL )) != S_OK) return hr;
        }

        hr = WsWriteEndElement( writer, NULL ); /* </s:Header> */
    }

    return hr;
}

static HRESULT write_envelope_start( struct msg *msg, WS_MESSAGE_INITIALIZATION init, WS_XML_WRITER *writer )
{
    static const WS_XML_STRING envelope = {8, (BYTE *)"Envelope"}, body = {4, (BYTE *)"Body"};
    static const WS_XML_STRING prefix_s = {1, (BYTE *)"s"}, prefix_a = {1, (BYTE *)"a"};
    const WS_XML_STRING *prefix_env = (msg->version_env == WS_ENVELOPE_VERSION_NONE) ? NULL : &prefix_s;
    const WS_XML_STRING *prefix_addr = (msg->version_addr == WS_ADDRESSING_VERSION_TRANSPORT) ? NULL : &prefix_a;
    const WS_XML_STRING *ns_env = get_env_namespace( msg->version_env );
    const WS_XML_STRING *ns_addr = get_addr_namespace( msg->version_addr );
    HRESULT hr;

    if ((hr = WsWriteStartElement( writer, prefix_env, &envelope, ns_env, NULL )) != S_OK) return hr;

    if (msg->version_addr == WS_ADDRESSING_VERSION_TRANSPORT)
        hr = write_headers_transport( msg, writer, prefix_env, ns_env );
    else
        hr = write_headers( msg, init, writer, prefix_env, ns_env, prefix_addr, ns_addr );
    if (hr != S_OK) return hr;

    return WsWriteStartElement( writer, prefix_env, &body, ns_env, NULL ); /* <s:Body> */
}

static HRESULT write_envelope_end( WS_XML_WRITER *writer )
{
    HRESULT hr;
    if ((hr = WsWriteEndElement( writer, NULL )) != S_OK) return hr; /* </s:Body> */
    return WsWriteEndElement( writer, NULL ); /* </s:Envelope> */
}

static HRESULT write_envelope( struct msg *msg, WS_MESSAGE_INITIALIZATION init )
{
    HRESULT hr;
    if (!msg->writer && (hr = WsCreateWriter( NULL, 0, &msg->writer, NULL )) != S_OK) return hr;
    if (!msg->buf && (hr = WsCreateXmlBuffer( msg->heap, NULL, 0, &msg->buf, NULL )) != S_OK) return hr;
    if ((hr = WsSetOutputToBuffer( msg->writer, msg->buf, NULL, 0, NULL )) != S_OK) return hr;
    if ((hr = write_envelope_start( msg, init, msg->writer )) != S_OK) return hr;
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

    if (!msg || !writer) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else if ((hr = write_envelope( msg, msg->init )) == S_OK &&
             (hr = write_envelope_start( msg, msg->init, writer )) == S_OK)
    {
        msg->writer_body = writer;
        msg->state       = WS_MESSAGE_STATE_WRITING;
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_WRITING) hr = WS_E_INVALID_OPERATION;
    else if ((hr = write_envelope_end( msg->writer_body )) == S_OK)
        msg->state = WS_MESSAGE_STATE_DONE;

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteBody		[webservices.@]
 */
HRESULT WINAPI WsWriteBody( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc, WS_WRITE_OPTION option,
                            const void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p %u %p %lu %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !desc) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_WRITING)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }

    if (desc->elementLocalName &&
        (hr = WsWriteStartElement( msg->writer_body, NULL, desc->elementLocalName, desc->elementNs,
                                   NULL )) != S_OK) goto done;

    if ((hr = WsWriteType( msg->writer_body, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription,
                           option, value, size, NULL )) != S_OK) goto done;

    if (desc->elementLocalName) hr = WsWriteEndElement( msg->writer_body, NULL );

done:
    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsFlushBody		[webservices.@]
 */
HRESULT WINAPI WsFlushBody( WS_MESSAGE *handle, ULONG size, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %lu %p %p\n", handle, size, ctx, error );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    hr = WsFlushWriter( msg->writer_body, size, ctx, error );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
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

static BOOL match_current_element_with_ns( WS_XML_READER *reader, const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;

    if (WsGetReaderNode( reader, &node, NULL ) != S_OK) return FALSE;
    if (node->nodeType != WS_XML_NODE_TYPE_ELEMENT) return FALSE;
    elem = (const WS_XML_ELEMENT_NODE *)node;
    return WsXmlStringEquals( elem->localName, localname, NULL ) == S_OK &&
           WsXmlStringEquals( elem->ns, ns, NULL ) == S_OK;
}

static HRESULT read_message_id( WS_XML_READER *reader, GUID *ret )
{
    const WS_XML_NODE *node;
    const WS_XML_TEXT_NODE *text;
    HRESULT hr;

    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
    if ((hr = WsGetReaderNode( reader, &node, NULL )) != S_OK) return hr;
    if (node->nodeType != WS_XML_NODE_TYPE_TEXT) return WS_E_INVALID_FORMAT;
    text = (const WS_XML_TEXT_NODE *)node;

    switch (text->text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text->text;
        if (utf8->value.length != 45 || memcmp( utf8->value.bytes, "urn:uuid:", 9 ))
            return WS_E_INVALID_FORMAT;

        return str_to_guid( utf8->value.bytes + 9, utf8->value.length - 9, ret );
    }
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    {
        const WS_XML_UNIQUE_ID_TEXT *id = (const WS_XML_UNIQUE_ID_TEXT *)text->text;
        *ret = id->value;
        return S_OK;
    }
    default:
        FIXME( "unhandled text type %u\n", text->text->textType );
        return E_NOTIMPL;
    }
}

static HRESULT read_envelope_start( struct msg *msg, WS_XML_READER *reader )
{
    static const WS_XML_STRING envelope = {8, (BYTE *)"Envelope"}, body = {4, (BYTE *)"Body"};
    static const WS_XML_STRING header = {6, (BYTE *)"Header"}, msgid = {9, (BYTE *)"MessageID"};
    static const WS_XML_STRING fault = {5, (BYTE *)"Fault"};
    const WS_XML_STRING *ns_env = get_env_namespace( msg->version_env );
    BOOL isfault;
    HRESULT hr;

    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
    if (!match_current_element( reader, &envelope )) return WS_E_INVALID_FORMAT;
    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
    if (match_current_element( reader, &header ))
    {
        for (;;)
        {
            if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
            if (match_current_element( reader, &msgid ) && (hr = read_message_id( reader, &msg->id_req )) != S_OK)
                return hr;
            if (match_current_element( reader, &body )) break;
        }
    }
    if (!match_current_element( reader, &body )) return WS_E_INVALID_FORMAT;
    if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;

    isfault = match_current_element_with_ns( reader, &fault, ns_env );
    return prop_set( msg->prop, msg->prop_count, WS_MESSAGE_PROPERTY_IS_FAULT, &isfault, sizeof(isfault) );
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

    if (!msg || !reader) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_EMPTY) hr = WS_E_INVALID_OPERATION;
    else if ((hr = read_envelope_start( msg, reader )) == S_OK &&
             (hr = create_header_buffer( reader, msg->heap, &msg->buf )) == S_OK)
    {
        msg->reader_body = reader;
        msg->state       = WS_MESSAGE_STATE_READING;
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_READING) hr = WS_E_INVALID_OPERATION;
    else if ((hr = WsReadEndElement( msg->reader_body, NULL )) == S_OK)
        msg->state = WS_MESSAGE_STATE_DONE;

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadBody		[webservices.@]
 */
HRESULT WINAPI WsReadBody( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc, WS_READ_OPTION option,
                           WS_HEAP *heap, void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    WS_ELEMENT_DESCRIPTION tmp;
    WS_FAULT_DESCRIPTION fault_desc;
    HRESULT hr;

    TRACE( "%p %p %u %p %p %lu %p\n", handle, desc, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !desc) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state != WS_MESSAGE_STATE_READING) hr = WS_E_INVALID_OPERATION;
    else
    {
        if (!desc->typeDescription)
        {
            if (desc->type == WS_FAULT_TYPE)
            {
                memcpy( &tmp, desc, sizeof(*desc) );
                fault_desc.envelopeVersion = msg->version_env;
                tmp.typeDescription = &fault_desc;
                desc = &tmp;
            }
            else if (desc->type == WS_ENDPOINT_ADDRESS_TYPE)
                FIXME( "!desc->typeDescription with WS_ENDPOINT_ADDRESS_TYPE\n" );
        }

        hr = WsReadElement( msg->reader_body, desc, option, heap, value, size, NULL );
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsFillBody		[webservices.@]
 */
HRESULT WINAPI WsFillBody( WS_MESSAGE *handle, ULONG size, const WS_ASYNC_CONTEXT *ctx, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %lu %p %p\n", handle, size, ctx, error );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    hr = WsFillReader( msg->reader_body, size, ctx, error );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsInitializeMessage		[webservices.@]
 */
HRESULT WINAPI WsInitializeMessage( WS_MESSAGE *handle, WS_MESSAGE_INITIALIZATION init,
                                    WS_MESSAGE *src_handle, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", handle, init, src_handle, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (src_handle)
    {
        FIXME( "src message not supported\n" );
        return E_NOTIMPL;
    }

    if (!msg || init > WS_FAULT_MESSAGE) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state >= WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else if ((hr = write_envelope( msg, init )) == S_OK)
    {
        msg->init  = init;
        msg->state = WS_MESSAGE_STATE_INITIALIZED;
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT grow_header_array( struct msg *msg, ULONG size )
{
    struct header **tmp;
    if (size <= msg->header_size) return S_OK;
    if (!(tmp = realloc( msg->header, 2 * msg->header_size * sizeof(struct header *) ))) return E_OUTOFMEMORY;
    msg->header = tmp;
    msg->header_size *= 2;
    return S_OK;
}

static struct header *alloc_header( WS_HEADER_TYPE type, BOOL mapped, const WS_XML_STRING *name,
                                    const WS_XML_STRING *ns )
{
    struct header *ret;
    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    if (name && name->length)
    {
        if (!(ret->name.bytes = malloc( name->length )))
        {
            free_header( ret );
            return NULL;
        }
        memcpy( ret->name.bytes, name->bytes, name->length );
        ret->name.length = name->length;
    }
    if (ns && ns->length)
    {
        if (!(ret->ns.bytes = malloc( ns->length )))
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

static HRESULT write_standard_header( struct msg *msg, WS_HEADER_TYPE type, WS_TYPE value_type,
                                      WS_WRITE_OPTION option, const void *value, ULONG size )
{
    static const WS_XML_STRING ns = {0, NULL};
    static const WS_XML_STRING prefix_s = {1, (BYTE *)"s"}, prefix_a = {1, (BYTE *)"a"};
    const WS_XML_STRING *prefix_env = (msg->version_env == WS_ENVELOPE_VERSION_NONE) ? NULL : &prefix_s;
    const WS_XML_STRING *prefix_addr = (msg->version_addr == WS_ADDRESSING_VERSION_TRANSPORT) ? NULL : &prefix_a;
    const WS_XML_STRING *localname = get_header_name( type );
    HRESULT hr;

    if ((hr = WsWriteStartElement( msg->writer, prefix_addr, localname, &ns, NULL )) != S_OK) return hr;
    if ((hr = write_must_understand( msg->writer, prefix_env, &ns )) != S_OK) return hr;
    if (msg->version_addr == WS_ADDRESSING_VERSION_TRANSPORT)
    {
        const WS_XML_STRING *ns_addr = get_addr_namespace( WS_ADDRESSING_VERSION_TRANSPORT );
        if ((hr = WsWriteXmlnsAttribute( msg->writer, NULL, ns_addr, FALSE, NULL )) != S_OK) return hr;
    }
    if ((hr = WsWriteType( msg->writer, WS_ELEMENT_CONTENT_TYPE_MAPPING, value_type, NULL, option, value, size,
                           NULL )) != S_OK) return hr;
    return WsWriteEndElement( msg->writer, NULL );
}

static HRESULT build_standard_header( struct msg *msg, WS_HEADER_TYPE type, WS_TYPE value_type,
                                      WS_WRITE_OPTION option, const void *value, ULONG size,
                                      struct header **ret )
{
    struct header *header;
    WS_XML_BUFFER *buf;
    HRESULT hr;

    if (!(header = alloc_header( type, FALSE, get_header_name(type), NULL ))) return E_OUTOFMEMORY;

    if (!msg->writer && (hr = WsCreateWriter( NULL, 0, &msg->writer, NULL )) != S_OK) goto done;
    if ((hr = WsCreateXmlBuffer( msg->heap, NULL, 0, &buf, NULL )) != S_OK) goto done;
    if ((hr = WsSetOutputToBuffer( msg->writer, buf, NULL, 0, NULL )) != S_OK) goto done;
    if ((hr = write_standard_header( msg, type, value_type, option, value, size )) != S_OK) goto done;

    header->u.buf = buf;

done:
    if (hr != S_OK) free_header( header );
    else *ret = header;
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

    TRACE( "%p %u %u %u %p %lu %p\n", handle, type, value_type, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }

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
        if ((hr = grow_header_array( msg, msg->header_count + 1 )) != S_OK) goto done;
        i = msg->header_count;
    }

    if ((hr = build_standard_header( msg, type, value_type, option, value, size, &header )) != S_OK)
        goto done;

    if (!found) msg->header_count++;
    else free_header( msg->header[i] );

    msg->header[i] = header;
    hr = write_envelope( msg, msg->init );

done:
    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT find_header( WS_XML_READER *reader, const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    const WS_XML_NODE *node;
    const WS_XML_ELEMENT_NODE *elem;
    HRESULT hr;

    for (;;)
    {
        if ((hr = WsReadNode( reader, NULL )) != S_OK) return hr;
        if ((hr = WsGetReaderNode( reader, &node, NULL )) != S_OK) return hr;
        if (node->nodeType == WS_XML_NODE_TYPE_EOF) return WS_E_INVALID_FORMAT;
        if (node->nodeType != WS_XML_NODE_TYPE_ELEMENT) continue;

        elem = (const WS_XML_ELEMENT_NODE *)node;
        if (WsXmlStringEquals( elem->localName, localname, NULL ) == S_OK &&
            WsXmlStringEquals( elem->ns, ns, NULL ) == S_OK) break;
    }

    return S_OK;
}

static HRESULT get_standard_header( struct msg *msg, WS_HEADER_TYPE type, WS_TYPE value_type,
                                    WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size )
{
    const WS_XML_STRING *localname = get_header_name( type );
    const WS_XML_STRING *ns = get_addr_namespace( msg->version_addr );
    HRESULT hr;

    if (!heap) heap = msg->heap;
    if (!msg->reader && (hr = WsCreateReader( NULL, 0, &msg->reader, NULL )) != S_OK) return hr;
    if ((hr = WsSetInputToBuffer( msg->reader, msg->buf, NULL, 0, NULL )) != S_OK) return hr;

    if ((hr = find_header( msg->reader, localname, ns )) != S_OK) return hr;
    return read_header( msg->reader, localname, ns, value_type, NULL, option, heap, value, size );
}

/**************************************************************************
 *          WsGetHeader		[webservices.@]
 */
HRESULT WINAPI WsGetHeader( WS_MESSAGE *handle, WS_HEADER_TYPE type, WS_TYPE value_type, WS_READ_OPTION option,
                            WS_HEAP *heap, void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %u %u %u %p %p %lu %p\n", handle, type, value_type, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER || option < WS_READ_REQUIRED_VALUE ||
        option > WS_READ_OPTIONAL_POINTER) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else hr = get_standard_header( msg, type, value_type, option, heap, value, size );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static void remove_header( struct msg *msg, ULONG i )
{
    free_header( msg->header[i] );
    memmove( &msg->header[i], &msg->header[i + 1], (msg->header_count - i - 1) * sizeof(struct header *) );
    msg->header_count--;
}

/**************************************************************************
 *          WsRemoveHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveHeader( WS_MESSAGE *handle, WS_HEADER_TYPE type, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    BOOL removed = FALSE;
    HRESULT hr = S_OK;
    ULONG i;

    TRACE( "%p %u %p\n", handle, type, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else if (type < WS_ACTION_HEADER || type > WS_FAULT_TO_HEADER) hr = E_INVALIDARG;
    else
    {
        for (i = 0; i < msg->header_count; i++)
        {
            if (msg->header[i]->type == type)
            {
                remove_header( msg, i );
                removed = TRUE;
                break;
            }
        }
        if (removed) hr = write_envelope( msg, msg->init );
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

static HRESULT add_mapped_header( struct msg *msg, const WS_XML_STRING *name, WS_TYPE type,
                                  WS_WRITE_OPTION option, const void *value, ULONG size )
{
    struct header *header;
    BOOL found = FALSE;
    HRESULT hr;
    ULONG i;

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
 *          WsAddMappedHeader		[webservices.@]
 */
HRESULT WINAPI WsAddMappedHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, WS_TYPE type,
                                  WS_WRITE_OPTION option, const void *value, ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %s %u %u %p %lu %p\n", handle, debugstr_xmlstr(name), type, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !name) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else hr = add_mapped_header( msg, name, type, option, value, size );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsRemoveMappedHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveMappedHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;
    ULONG i;

    TRACE( "%p %s %p\n", handle, debugstr_xmlstr(name), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !name) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else
    {
        for (i = 0; i < msg->header_count; i++)
        {
            if (msg->header[i]->type || !msg->header[i]->mapped) continue;
            if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK)
            {
                remove_header( msg, i );
                break;
            }
        }
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT xmlstring_to_wsz( const WS_XML_STRING *str, WS_HEAP *heap, WCHAR **ret, int *len )
{
    *len = MultiByteToWideChar( CP_UTF8, 0, (char *)str->bytes, str->length, NULL, 0 );
    if (!(*ret = ws_alloc( heap, (*len + 1) * sizeof(WCHAR) ))) return WS_E_QUOTA_EXCEEDED;
    MultiByteToWideChar( CP_UTF8, 0, (char *)str->bytes, str->length, *ret, *len );
    (*ret)[*len] = 0;
    return S_OK;
}

static HRESULT get_header_value_wsz( struct header *header, WS_READ_OPTION option, WS_HEAP *heap, WCHAR **ret,
                                     ULONG size )
{
    WCHAR *str = NULL;
    int len = 0;
    HRESULT hr;

    if (header && (hr = xmlstring_to_wsz( header->u.text, heap, &str, &len )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
        if (!str && !(str = ws_alloc_zero( heap, sizeof(*str) ))) return WS_E_QUOTA_EXCEEDED;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (size != sizeof(str))
        {
            ws_free( heap, str, (len + 1) * sizeof(WCHAR) );
            return E_INVALIDARG;
        }
        *ret = str;
        break;

    default:
        FIXME( "read option %u not supported\n", option );
        ws_free( heap, str, (len + 1) * sizeof(WCHAR) );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT get_mapped_header( struct msg *msg, const WS_XML_STRING *name, WS_TYPE type, WS_READ_OPTION option,
                                  WS_HEAP *heap, void *value, ULONG size )
{
    struct header *header = NULL;
    HRESULT hr;
    ULONG i;

    for (i = 0; i < msg->header_count; i++)
    {
        if (msg->header[i]->type || !msg->header[i]->mapped) continue;
        if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK)
        {
            header = msg->header[i];
            break;
        }
    }

    switch (type)
    {
    case WS_WSZ_TYPE:
        hr = get_header_value_wsz( header, option, heap, value, size );
        break;

    default:
        FIXME( "type %u not supported\n", option );
        return WS_E_NOT_SUPPORTED;
    }

    return hr;
}

/**************************************************************************
 *          WsGetMappedHeader		[webservices.@]
 */
HRESULT WINAPI WsGetMappedHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, WS_REPEATING_HEADER_OPTION option,
                                  ULONG index, WS_TYPE type, WS_READ_OPTION read_option, WS_HEAP *heap, void *value,
                                  ULONG size, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %s %u %lu %u %u %p %p %lu %p\n", handle, debugstr_xmlstr(name), option, index, type, read_option,
           heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (option != WS_SINGLETON_HEADER)
    {
        FIXME( "option %u not supported\n", option );
        return E_NOTIMPL;
    }

    if (!msg || !name) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else hr = get_mapped_header( msg, name, type, read_option, heap, value, size );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

static HRESULT build_custom_header( struct msg *msg, const WS_XML_STRING *name, const WS_XML_STRING *ns,
                                    WS_TYPE type, const void *desc, WS_WRITE_OPTION option, const void *value,
                                    ULONG size, struct header **ret )
{
    struct header *header;
    WS_XML_BUFFER *buf;
    HRESULT hr;

    if (!(header = alloc_header( 0, FALSE, name, ns ))) return E_OUTOFMEMORY;

    if (!msg->writer && (hr = WsCreateWriter( NULL, 0, &msg->writer, NULL )) != S_OK) goto done;
    if ((hr = WsCreateXmlBuffer( msg->heap, NULL, 0, &buf, NULL )) != S_OK) goto done;
    if ((hr = WsSetOutputToBuffer( msg->writer, buf, NULL, 0, NULL )) != S_OK) goto done;
    if ((hr = write_custom_header( msg->writer, name, ns, type, desc, option, value, size )) != S_OK) goto done;

    header->u.buf = buf;

done:
    if (hr != S_OK) free_header( header );
    else *ret = header;
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

    TRACE( "%p %p %u %p %lu %#lx %p\n", handle, desc, option, value, size, attrs, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !desc) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }

    if ((hr = grow_header_array( msg, msg->header_count + 1 )) != S_OK) goto done;
    if ((hr = build_custom_header( msg, desc->elementLocalName, desc->elementNs, desc->type,
                                   desc->typeDescription, option, value, size, &header )) != S_OK) goto done;
    msg->header[msg->header_count++] = header;
    hr = write_envelope( msg, msg->init );

done:
    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT get_custom_header( struct msg *msg, const WS_ELEMENT_DESCRIPTION *desc, WS_READ_OPTION option,
                                  WS_HEAP *heap, void *value, ULONG size )
{
    HRESULT hr;
    if (!heap) heap = msg->heap;
    if (!msg->reader && (hr = WsCreateReader( NULL, 0, &msg->reader, NULL )) != S_OK) return hr;
    if ((hr = WsSetInputToBuffer( msg->reader, msg->buf, NULL, 0, NULL )) != S_OK) return hr;

    if ((hr = find_header( msg->reader, desc->elementLocalName, desc->elementNs )) != S_OK) return hr;
    return read_header( msg->reader, desc->elementLocalName, desc->elementNs, desc->type, desc->typeDescription,
                        option, heap, value, size );
}

/**************************************************************************
 *          WsGetCustomHeader		[webservices.@]
 */
HRESULT WINAPI WsGetCustomHeader( WS_MESSAGE *handle, const WS_ELEMENT_DESCRIPTION *desc,
                                  WS_REPEATING_HEADER_OPTION repeat_option, ULONG index, WS_READ_OPTION option,
                                  WS_HEAP *heap, void *value, ULONG size, ULONG *attrs, WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    TRACE( "%p %p %u %lu %u %p %p %lu %p %p\n", handle, desc, repeat_option, index, option, heap, value,
           size, attrs, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !desc || repeat_option < WS_REPEATING_HEADER || repeat_option > WS_SINGLETON_HEADER ||
        (repeat_option == WS_SINGLETON_HEADER && index)) return E_INVALIDARG;

    if (repeat_option == WS_REPEATING_HEADER)
    {
        FIXME( "repeating header not supported\n" );
        return E_NOTIMPL;
    }
    if (attrs)
    {
        FIXME( "attributes not supported\n" );
        return E_NOTIMPL;
    }

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else hr = get_custom_header( msg, desc, option, heap, value, size );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsRemoveCustomHeader		[webservices.@]
 */
HRESULT WINAPI WsRemoveCustomHeader( WS_MESSAGE *handle, const WS_XML_STRING *name, const WS_XML_STRING *ns,
                                     WS_ERROR *error )
{
    struct msg *msg = (struct msg *)handle;
    BOOL removed = FALSE;
    HRESULT hr = S_OK;
    ULONG i;

    TRACE( "%p %s %s %p\n", handle, debugstr_xmlstr(name), debugstr_xmlstr(ns), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!msg || !name || !ns) return E_INVALIDARG;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (msg->state < WS_MESSAGE_STATE_INITIALIZED) hr = WS_E_INVALID_OPERATION;
    else
    {
        for (i = 0; i < msg->header_count; i++)
        {
            if (msg->header[i]->type || msg->header[i]->mapped) continue;
            if (WsXmlStringEquals( name, &msg->header[i]->name, NULL ) == S_OK &&
                WsXmlStringEquals( ns, &msg->header[i]->ns, NULL ) == S_OK)
            {
                remove_header( msg, i );
                removed = TRUE;
                i--;
            }
        }
        if (removed) hr = write_envelope( msg, msg->init );
    }

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static WCHAR *build_http_header( const WCHAR *name, const WCHAR *value, ULONG *ret_len )
{
    int len_name = lstrlenW( name ), len_value = lstrlenW( value );
    WCHAR *ret = malloc( (len_name + len_value + 2) * sizeof(WCHAR) );

    if (!ret) return NULL;
    memcpy( ret, name, len_name * sizeof(WCHAR) );
    ret[len_name++] = ':';
    ret[len_name++] = ' ';
    memcpy( ret + len_name, value, len_value * sizeof(WCHAR) );
    *ret_len = len_name + len_value;
    return ret;
}

static inline HRESULT insert_http_header( HINTERNET req, const WCHAR *header, ULONG len, ULONG flags )
{
    if (WinHttpAddRequestHeaders( req, header, len, flags )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

static WCHAR *from_xml_string( const WS_XML_STRING *str )
{
    WCHAR *ret;
    int len = MultiByteToWideChar( CP_UTF8, 0, (char *)str->bytes, str->length, NULL, 0 );
    if (!(ret = malloc( (len + 1) * sizeof(*ret) ))) return NULL;
    MultiByteToWideChar( CP_UTF8, 0, (char *)str->bytes, str->length, ret, len );
    ret[len] = 0;
    return ret;
}

static HRESULT insert_mapped_headers( struct msg *msg, HINTERNET req )
{
    WCHAR *name, *value, *header;
    ULONG i, len;
    HRESULT hr;

    for (i = 0; i < msg->header_count; i++)
    {
        if (!msg->header[i]->mapped) continue;

        if (!(name = from_xml_string( &msg->header[i]->name ))) return E_OUTOFMEMORY;
        if (!(value = from_xml_string( msg->header[i]->u.text )))
        {
            free( name );
            return E_OUTOFMEMORY;
        }
        header = build_http_header( name, value, &len );
        free( name );
        free( value );
        if (!header) return E_OUTOFMEMORY;

        hr = insert_http_header( req, header, len, WINHTTP_ADDREQ_FLAG_ADD|WINHTTP_ADDREQ_FLAG_REPLACE );
        free( header );
        if (hr != S_OK) return hr;
    }
    return S_OK;
}

HRESULT message_insert_http_headers( WS_MESSAGE *handle, HINTERNET req )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = E_OUTOFMEMORY;
    WCHAR *header = NULL, *buf;
    ULONG len;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    switch (msg->version_env)
    {
    case WS_ENVELOPE_VERSION_SOAP_1_1:
        header = build_http_header( L"Content-Type", L"text/xml", &len );
        break;

    case WS_ENVELOPE_VERSION_SOAP_1_2:
        header = build_http_header( L"Content-Type", L"application/soap+xml", &len );
        break;

    default:
        FIXME( "unhandled envelope version %u\n", msg->version_env );
        hr = E_NOTIMPL;
    }
    if (!header) goto done;

    if ((hr = insert_http_header( req, header, len, WINHTTP_ADDREQ_FLAG_ADD )) != S_OK) goto done;
    free( header );

    hr = E_OUTOFMEMORY;
    if (!(header = build_http_header( L"Content-Type", L"charset=utf-8", &len ))) goto done;
    if ((hr = insert_http_header( req, header, len, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON )) != S_OK)
        goto done;
    free( header );
    header = NULL;

    switch (msg->version_env)
    {
    case WS_ENVELOPE_VERSION_SOAP_1_1:
    {
        if (!(len = MultiByteToWideChar( CP_UTF8, 0, (char *)msg->action->bytes, msg->action->length, NULL, 0 )))
            break;

        hr = E_OUTOFMEMORY;
        if (!(buf = malloc( (len + 3) * sizeof(WCHAR) ))) goto done;
        buf[0] = '"';
        MultiByteToWideChar( CP_UTF8, 0, (char *)msg->action->bytes, msg->action->length, buf + 1, len );
        buf[len + 1] = '"';
        buf[len + 2] = 0;

        header = build_http_header( L"SOAPAction", buf, &len );
        free( buf );
        if (!header) goto done;

        hr = insert_http_header( req, header, len, WINHTTP_ADDREQ_FLAG_ADD );
        break;
    }
    case WS_ENVELOPE_VERSION_SOAP_1_2:
    {
        ULONG len_action = ARRAY_SIZE( L"action=\"" ) - 1;

        if (!(len = MultiByteToWideChar( CP_UTF8, 0, (char *)msg->action->bytes, msg->action->length, NULL, 0 )))
            break;

        hr = E_OUTOFMEMORY;
        if (!(buf = malloc( (len + len_action + 2) * sizeof(WCHAR) ))) goto done;
        memcpy( buf, L"action=\"", len_action * sizeof(WCHAR) );
        MultiByteToWideChar( CP_UTF8, 0, (char *)msg->action->bytes, msg->action->length, buf + len_action, len );
        len += len_action;
        buf[len++] = '"';
        buf[len] = 0;

        header = build_http_header( L"Content-Type", buf, &len );
        free( buf );
        if (!header) goto done;

        hr = insert_http_header( req, header, len, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON );
        break;
    }
    default:
        FIXME( "unhandled envelope version %u\n", msg->version_env );
        hr = E_NOTIMPL;
    }

    if (hr == S_OK) hr = insert_mapped_headers( msg, req );

done:
    free( header );
    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT map_http_response_headers( struct msg *msg, HINTERNET req, const WS_HTTP_MESSAGE_MAPPING *mapping )
{
    ULONG i;
    for (i = 0; i < mapping->responseHeaderMappingCount; i++)
    {
        WCHAR *name, *value;
        DWORD size;

        if (!(name = from_xml_string( &mapping->responseHeaderMappings[i]->headerName ))) return E_OUTOFMEMORY;

        if (!WinHttpQueryHeaders( req, WINHTTP_QUERY_CUSTOM, name, NULL, &size, NULL ) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            HRESULT hr;
            if (!(value = malloc( size )))
            {
                free( name );
                return E_OUTOFMEMORY;
            }
            if (!WinHttpQueryHeaders( req, WINHTTP_QUERY_CUSTOM, name, value, &size, NULL ))
            {
                free( name );
                return HRESULT_FROM_WIN32( GetLastError() );
            }
            hr = add_mapped_header( msg, &mapping->responseHeaderMappings[i]->headerName, WS_WSZ_TYPE,
                                    WS_WRITE_REQUIRED_POINTER, &value, sizeof(value) );
            free( value );
            if (hr != S_OK)
            {
                free( name );
                return hr;
            }
        }
        free( name );
    }
    return S_OK;
}

HRESULT message_map_http_response_headers( WS_MESSAGE *handle, HINTERNET req, const WS_HTTP_MESSAGE_MAPPING *mapping )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    hr = map_http_response_headers( msg, req, mapping );

    LeaveCriticalSection( &msg->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

void message_set_send_context( WS_MESSAGE *handle, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT *ctx )
{
    struct msg *msg = (struct msg *)handle;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return;
    }

    msg->ctx_send.callback = ctx->callback;
    msg->ctx_send.state    = ctx->state;

    LeaveCriticalSection( &msg->cs );
}

void message_set_receive_context( WS_MESSAGE *handle, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT *ctx )
{
    struct msg *msg = (struct msg *)handle;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return;
    }

    msg->ctx_receive.callback = ctx->callback;
    msg->ctx_receive.state    = ctx->state;

    LeaveCriticalSection( &msg->cs );
}

void message_do_send_callback( WS_MESSAGE *handle )
{
    struct msg *msg = (struct msg *)handle;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return;
    }

    if (msg->ctx_send.callback)
    {
        HRESULT hr;
        TRACE( "executing callback %p\n", msg->ctx_send.callback );
        hr = msg->ctx_send.callback( handle, msg->heap, msg->ctx_send.state, NULL );
        TRACE( "callback %p returned %#lx\n", msg->ctx_send.callback, hr );
    }

    LeaveCriticalSection( &msg->cs );
}

void message_do_receive_callback( WS_MESSAGE *handle )
{
    struct msg *msg = (struct msg *)handle;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return;
    }

    if (msg->ctx_receive.callback)
    {
        HRESULT hr;
        TRACE( "executing callback %p\n", msg->ctx_receive.callback );
        hr = msg->ctx_receive.callback( handle, msg->heap, msg->ctx_receive.state, NULL );
        TRACE( "callback %p returned %#lx\n", msg->ctx_receive.callback, hr );
    }

    LeaveCriticalSection( &msg->cs );
}

HRESULT message_set_action( WS_MESSAGE *handle, const WS_XML_STRING *action )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    if (!action || !action->length)
    {
        free_xml_string( msg->action );
        msg->action = NULL;
    }
    else
    {
        WS_XML_STRING *str;
        if (!(str = dup_xml_string( action, FALSE ))) hr = E_OUTOFMEMORY;
        else
        {
            free_xml_string( msg->action );
            msg->action = str;
        }
    }

    LeaveCriticalSection( &msg->cs );
    return hr;
}

HRESULT message_get_id( WS_MESSAGE *handle, GUID *id )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    *id = msg->id_req;

    LeaveCriticalSection( &msg->cs );
    return hr;
}

HRESULT message_set_request_id( WS_MESSAGE *handle, const GUID *id )
{
    struct msg *msg = (struct msg *)handle;
    HRESULT hr = S_OK;

    EnterCriticalSection( &msg->cs );

    if (msg->magic != MSG_MAGIC)
    {
        LeaveCriticalSection( &msg->cs );
        return E_INVALIDARG;
    }

    msg->id_req = *id;

    LeaveCriticalSection( &msg->cs );
    return hr;
}

/* Attempt to read a fault message. If the message is a fault, WS_E_ENDPOINT_FAULT_RECEIVED will be returned. */
HRESULT message_read_fault( WS_MESSAGE *handle, WS_HEAP *heap, WS_ERROR *error )
{
    static const WS_ELEMENT_DESCRIPTION desc = { NULL, NULL, WS_FAULT_TYPE, NULL };
    BOOL isfault;
    WS_FAULT fault = {0};
    WS_XML_STRING action;
    HRESULT hr;

    if ((hr = WsGetMessageProperty( handle, WS_MESSAGE_PROPERTY_IS_FAULT, &isfault, sizeof(isfault), NULL )) != S_OK)
        return hr;
    if (!isfault)
        return S_OK;

    if ((hr = WsReadBody( handle, &desc, WS_READ_REQUIRED_VALUE, heap, &fault, sizeof(fault), NULL )) != S_OK ||
        (hr = WsReadEnvelopeEnd( handle, NULL )) != S_OK)
        goto done;

    if (!error) goto done;

    if ((hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_FAULT, &fault, sizeof(fault) )) != S_OK)
        goto done;

    if ((hr = WsGetHeader( handle, WS_ACTION_HEADER, WS_XML_STRING_TYPE, WS_READ_REQUIRED_VALUE,
                           heap, &action, sizeof(action), NULL )) != S_OK)
    {
        if (hr == WS_E_INVALID_FORMAT)
        {
            memset( &action, 0, sizeof(action) );
            hr = S_OK;
        }
        else
            goto done;
    }
    hr = WsSetFaultErrorProperty( error, WS_FAULT_ERROR_PROPERTY_ACTION, &action, sizeof(action) );

done:
    free_fault_fields( heap, &fault );
    return hr != S_OK ? hr : WS_E_ENDPOINT_FAULT_RECEIVED;
}
