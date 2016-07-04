/*
 * Copyright 2015, 2016 Hans Leidekker for CodeWeavers
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
#include "wine/unicode.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc writer_props[] =
{
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_DEPTH */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_ALLOW_FRAGMENT */
    { sizeof(ULONG), FALSE },       /* WS_XML_READER_PROPERTY_MAX_ATTRIBUTES */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_WRITE_DECLARATION */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_INDENT */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE */
    { sizeof(WS_CHARSET), FALSE },  /* WS_XML_WRITER_PROPERTY_CHARSET */
    { sizeof(WS_BUFFERS), FALSE },  /* WS_XML_WRITER_PROPERTY_BUFFERS */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE */
    { sizeof(WS_BYTES), FALSE },    /* WS_XML_WRITER_PROPERTY_BYTES */
    { sizeof(BOOL), TRUE },         /* WS_XML_WRITER_PROPERTY_IN_ATTRIBUTE */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE */
    { sizeof(WS_BYTES), FALSE },    /* WS_XML_WRITER_PROPERTY_INITIAL_BUFFER */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_ALLOW_INVALID_CHARACTER_REFERENCES */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_NAMESPACES */
    { sizeof(ULONG), TRUE },        /* WS_XML_WRITER_PROPERTY_BYTES_WRITTEN */
    { sizeof(ULONG), TRUE },        /* WS_XML_WRITER_PROPERTY_BYTES_TO_CLOSE */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_COMPRESS_EMPTY_ELEMENTS */
    { sizeof(BOOL), FALSE }         /* WS_XML_WRITER_PROPERTY_EMIT_UNCOMPRESSED_EMPTY_ELEMENTS */
};

enum writer_state
{
    WRITER_STATE_INITIAL,
    WRITER_STATE_STARTELEMENT,
    WRITER_STATE_STARTATTRIBUTE,
    WRITER_STATE_STARTCDATA,
    WRITER_STATE_ENDSTARTELEMENT,
    WRITER_STATE_TEXT,
    WRITER_STATE_COMMENT,
    WRITER_STATE_ENDELEMENT,
    WRITER_STATE_ENDCDATA
};

struct writer
{
    ULONG                     write_pos;
    unsigned char            *write_bufptr;
    enum writer_state         state;
    struct node              *root;
    struct node              *current;
    WS_XML_STRING            *current_ns;
    WS_XML_WRITER_OUTPUT_TYPE output_type;
    struct xmlbuf            *output_buf;
    WS_HEAP                  *output_heap;
    ULONG                     prop_count;
    struct prop               prop[sizeof(writer_props)/sizeof(writer_props[0])];
};

static struct writer *alloc_writer(void)
{
    static const ULONG count = sizeof(writer_props)/sizeof(writer_props[0]);
    struct writer *ret;
    ULONG size = sizeof(*ret) + prop_size( writer_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    prop_init( writer_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_writer( struct writer *writer )
{
    destroy_nodes( writer->root );
    heap_free( writer->current_ns );
    WsFreeHeap( writer->output_heap );
    heap_free( writer );
}

static void write_insert_eof( struct writer *writer, struct node *eof )
{
    if (!writer->root) writer->root = eof;
    else
    {
        eof->parent = writer->root;
        list_add_tail( &writer->root->children, &eof->entry );
    }
    writer->current = eof;
}

static void write_insert_bof( struct writer *writer, struct node *bof )
{
    writer->root->parent = bof;
    list_add_tail( &bof->children, &writer->root->entry );
    writer->current = writer->root = bof;
}

static void write_insert_node( struct writer *writer, struct node *parent, struct node *node )
{
    node->parent = parent;
    list_add_before( list_tail( &parent->children ), &node->entry );
    writer->current = node;
}

static HRESULT write_init_state( struct writer *writer )
{
    struct node *node;

    heap_free( writer->current_ns );
    writer->current_ns = NULL;
    destroy_nodes( writer->root );
    writer->root = NULL;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_EOF ))) return E_OUTOFMEMORY;
    write_insert_eof( writer, node );
    writer->state = WRITER_STATE_INITIAL;
    return S_OK;
}

/**************************************************************************
 *          WsCreateWriter		[webservices.@]
 */
HRESULT WINAPI WsCreateWriter( const WS_XML_WRITER_PROPERTY *properties, ULONG count,
                               WS_XML_WRITER **handle, WS_ERROR *error )
{
    struct writer *writer;
    ULONG i, max_depth = 32, max_attrs = 128, trim_size = 4096, max_size = 65536, max_ns = 32;
    WS_CHARSET charset = WS_CHARSET_UTF8;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (!(writer = alloc_writer())) return E_OUTOFMEMORY;

    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_MAX_DEPTH, &max_depth, sizeof(max_depth) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, sizeof(max_attrs) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_BUFFER_TRIM_SIZE, &trim_size, sizeof(trim_size) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_CHARSET, &charset, sizeof(charset) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE, &max_size, sizeof(max_size) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_MAX_MIME_PARTS_BUFFER_SIZE, &max_size, sizeof(max_size) );
    prop_set( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_MAX_NAMESPACES, &max_ns, sizeof(max_ns) );

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_writer( writer );
            return hr;
        }
    }

    hr = prop_get( writer->prop, writer->prop_count, WS_XML_WRITER_PROPERTY_BUFFER_MAX_SIZE,
                   &max_size, sizeof(max_size) );
    if (hr != S_OK)
    {
        free_writer( writer );
        return hr;
    }

    hr = WsCreateHeap( max_size, 0, NULL, 0, &writer->output_heap, NULL );
    if (hr != S_OK)
    {
        free_writer( writer );
        return hr;
    }

    hr = write_init_state( writer );
    if (hr != S_OK)
    {
        free_writer( writer );
        return hr;
    }

    *handle = (WS_XML_WRITER *)writer;
    return S_OK;
}

/**************************************************************************
 *          WsFreeWriter		[webservices.@]
 */
void WINAPI WsFreeWriter( WS_XML_WRITER *handle )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p\n", handle );
    free_writer( writer );
}

#define XML_BUFFER_INITIAL_ALLOCATED_SIZE 256
static struct xmlbuf *alloc_xmlbuf( WS_HEAP *heap )
{
    struct xmlbuf *ret;

    if (!(ret = ws_alloc( heap, sizeof(*ret) ))) return NULL;
    if (!(ret->ptr = ws_alloc( heap, XML_BUFFER_INITIAL_ALLOCATED_SIZE )))
    {
        ws_free( heap, ret );
        return NULL;
    }
    ret->heap           = heap;
    ret->size_allocated = XML_BUFFER_INITIAL_ALLOCATED_SIZE;
    ret->size           = 0;
    return ret;
}

static void free_xmlbuf( struct xmlbuf *xmlbuf )
{
    if (!xmlbuf) return;
    ws_free( xmlbuf->heap, xmlbuf->ptr );
    ws_free( xmlbuf->heap, xmlbuf );
}

/**************************************************************************
 *          WsCreateXmlBuffer		[webservices.@]
 */
HRESULT WINAPI WsCreateXmlBuffer( WS_HEAP *heap, const WS_XML_BUFFER_PROPERTY *properties,
                                  ULONG count, WS_XML_BUFFER **handle, WS_ERROR *error )
{
    struct xmlbuf *xmlbuf;

    if (!heap || !handle) return E_INVALIDARG;
    if (count) FIXME( "properties not implemented\n" );

    if (!(xmlbuf = alloc_xmlbuf( heap ))) return E_OUTOFMEMORY;

    *handle = (WS_XML_BUFFER *)xmlbuf;
    return S_OK;
}

/**************************************************************************
 *          WsGetWriterProperty		[webservices.@]
 */
HRESULT WINAPI WsGetWriterProperty( WS_XML_WRITER *handle, WS_XML_WRITER_PROPERTY_ID id,
                                    void *buf, ULONG size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer->output_type) return WS_E_INVALID_OPERATION;

    switch (id)
    {
    case WS_XML_WRITER_PROPERTY_BYTES:
    {
        WS_BYTES *bytes = buf;
        if (size != sizeof(*bytes)) return E_INVALIDARG;
        bytes->bytes  = writer->output_buf->ptr;
        bytes->length = writer->output_buf->size;
        return S_OK;
    }
    default:
        return prop_get( writer->prop, writer->prop_count, id, buf, size );
    }
}

static void set_output_buffer( struct writer *writer, struct xmlbuf *xmlbuf )
{
    /* free current buffer if it's ours */
    if (writer->output_buf && writer->output_buf->heap == writer->output_heap)
    {
        free_xmlbuf( writer->output_buf );
    }
    writer->output_buf   = xmlbuf;
    writer->output_type  = WS_XML_WRITER_OUTPUT_TYPE_BUFFER;
    writer->write_bufptr = xmlbuf->ptr;
    writer->write_pos    = 0;
}

/**************************************************************************
 *          WsSetOutput		[webservices.@]
 */
HRESULT WINAPI WsSetOutput( WS_XML_WRITER *handle, const WS_XML_WRITER_ENCODING *encoding,
                            const WS_XML_WRITER_OUTPUT *output, const WS_XML_WRITER_PROPERTY *properties,
                            ULONG count, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    struct node *node;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %p %p %p %u %p\n", handle, encoding, output, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if ((hr = write_init_state( writer )) != S_OK) return hr;

    switch (encoding->encodingType)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:
    {
        WS_XML_WRITER_TEXT_ENCODING *text = (WS_XML_WRITER_TEXT_ENCODING *)encoding;
        if (text->charSet != WS_CHARSET_UTF8)
        {
            FIXME( "charset %u not supported\n", text->charSet );
            return E_NOTIMPL;
        }
        break;
    }
    default:
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        return E_NOTIMPL;
    }
    switch (output->outputType)
    {
    case WS_XML_WRITER_OUTPUT_TYPE_BUFFER:
    {
        struct xmlbuf *xmlbuf;

        if (!(xmlbuf = alloc_xmlbuf( writer->output_heap ))) return E_OUTOFMEMORY;
        set_output_buffer( writer, xmlbuf );
        break;
    }
    default:
        FIXME( "output type %u not supported\n", output->outputType );
        return E_NOTIMPL;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) return E_OUTOFMEMORY;
    write_insert_bof( writer, node );
    return S_OK;
}

/**************************************************************************
 *          WsSetOutputToBuffer		[webservices.@]
 */
HRESULT WINAPI WsSetOutputToBuffer( WS_XML_WRITER *handle, WS_XML_BUFFER *buffer,
                                    const WS_XML_WRITER_PROPERTY *properties, ULONG count,
                                    WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    struct xmlbuf *xmlbuf = (struct xmlbuf *)buffer;
    struct node *node;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %p %p %u %p\n", handle, buffer, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !xmlbuf) return E_INVALIDARG;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if ((hr = write_init_state( writer )) != S_OK) return hr;
    set_output_buffer( writer, xmlbuf );

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) return E_OUTOFMEMORY;
    write_insert_bof( writer, node );
    return S_OK;
}

static HRESULT write_grow_buffer( struct writer *writer, ULONG size )
{
    struct xmlbuf *buf = writer->output_buf;
    SIZE_T new_size;
    void *tmp;

    if (buf->size_allocated >= writer->write_pos + size)
    {
        buf->size = writer->write_pos + size;
        return S_OK;
    }
    new_size = max( buf->size_allocated * 2, writer->write_pos + size );
    if (!(tmp = ws_realloc( buf->heap, buf->ptr, new_size ))) return E_OUTOFMEMORY;
    writer->write_bufptr = buf->ptr = tmp;
    buf->size_allocated = new_size;
    buf->size = writer->write_pos + size;
    return S_OK;
}

static inline void write_char( struct writer *writer, unsigned char ch )
{
    writer->write_bufptr[writer->write_pos++] = ch;
}

static inline void write_bytes( struct writer *writer, const BYTE *bytes, ULONG len )
{
    memcpy( writer->write_bufptr + writer->write_pos, bytes, len );
    writer->write_pos += len;
}

static HRESULT write_attribute( struct writer *writer, WS_XML_ATTRIBUTE *attr )
{
    WS_XML_UTF8_TEXT *text = (WS_XML_UTF8_TEXT *)attr->value;
    unsigned char quote = attr->singleQuote ? '\'' : '"';
    const WS_XML_STRING *prefix;
    ULONG size;
    HRESULT hr;

    if (attr->prefix) prefix = attr->prefix;
    else prefix = writer->current->hdr.prefix;

    /* ' prefix:attr="value"' */

    size = attr->localName->length + 4 /* ' =""' */;
    if (prefix) size += prefix->length + 1 /* ':' */;
    if (text) size += text->value.length;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, ' ' );
    if (prefix)
    {
        write_bytes( writer, prefix->bytes, prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, attr->localName->bytes, attr->localName->length );
    write_char( writer, '=' );
    write_char( writer, quote );
    if (text) write_bytes( writer, text->value.bytes, text->value.length );
    write_char( writer, quote );

    return S_OK;
}

static inline BOOL is_current_namespace( struct writer *writer, const WS_XML_STRING *ns )
{
    return (WsXmlStringEquals( writer->current_ns, ns, NULL ) == S_OK);
}

/**************************************************************************
 *          WsGetPrefixFromNamespace		[webservices.@]
 */
HRESULT WINAPI WsGetPrefixFromNamespace( WS_XML_WRITER *handle, const WS_XML_STRING *ns,
                                         BOOL required, const WS_XML_STRING **prefix,
                                         WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_XML_ELEMENT_NODE *elem;
    BOOL found = FALSE;

    TRACE( "%p %s %d %p %p\n", handle, debugstr_xmlstr(ns), required, prefix, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !ns || !prefix) return E_INVALIDARG;

    elem = &writer->current->hdr;
    if (elem->prefix && is_current_namespace( writer, ns ))
    {
        *prefix = elem->prefix;
        found = TRUE;
    }
    if (!found)
    {
        if (required) return WS_E_INVALID_FORMAT;
        *prefix = NULL;
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT set_current_namespace( struct writer *writer, const WS_XML_STRING *ns )
{
    WS_XML_STRING *str;
    if (!(str = alloc_xml_string( ns->bytes, ns->length ))) return E_OUTOFMEMORY;
    heap_free( writer->current_ns );
    writer->current_ns = str;
    return S_OK;
}

static HRESULT write_namespace_attribute( struct writer *writer, WS_XML_ATTRIBUTE *attr )
{
    unsigned char quote = attr->singleQuote ? '\'' : '"';
    ULONG size;
    HRESULT hr;

    /* ' xmlns:prefix="namespace"' */

    size = attr->ns->length + 9 /* ' xmlns=""' */;
    if (attr->prefix) size += attr->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_bytes( writer, (const BYTE *)" xmlns", 6 );
    if (attr->prefix)
    {
        write_char( writer, ':' );
        write_bytes( writer, attr->prefix->bytes, attr->prefix->length );
    }
    write_char( writer, '=' );
    write_char( writer, quote );
    write_bytes( writer, attr->ns->bytes, attr->ns->length );
    write_char( writer, quote );

    return S_OK;
}

static HRESULT write_add_namespace_attribute( struct writer *writer, const WS_XML_STRING *prefix,
                                              const WS_XML_STRING *ns, BOOL single )
{
    WS_XML_ATTRIBUTE *attr;
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!(attr = heap_alloc_zero( sizeof(*attr) ))) return E_OUTOFMEMORY;

    attr->singleQuote = !!single;
    attr->isXmlNs = 1;
    if (prefix && !(attr->prefix = alloc_xml_string( prefix->bytes, prefix->length )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->ns = alloc_xml_string( ns->bytes, ns->length )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if ((hr = append_attribute( elem, attr )) != S_OK)
    {
        free_attribute( attr );
        return hr;
    }
    return S_OK;
}

static BOOL namespace_in_scope( const WS_XML_ELEMENT_NODE *elem, const WS_XML_STRING *prefix,
                                const WS_XML_STRING *ns )
{
    ULONG i;
    const struct node *node;

    for (node = (const struct node *)elem; node; node = node->parent)
    {
        if (node_type( node ) != WS_XML_NODE_TYPE_ELEMENT) break;

        elem = &node->hdr;
        for (i = 0; i < elem->attributeCount; i++)
        {
            if (!elem->attributes[i]->isXmlNs) continue;
            if (WsXmlStringEquals( elem->attributes[i]->prefix, prefix, NULL ) == S_OK &&
                WsXmlStringEquals( elem->attributes[i]->ns, ns, NULL ) == S_OK)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static HRESULT write_set_element_namespace( struct writer *writer )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!elem->ns->length || is_current_namespace( writer, elem->ns ) ||
        namespace_in_scope( elem, elem->prefix, elem->ns )) return S_OK;

    if ((hr = write_add_namespace_attribute( writer, elem->prefix, elem->ns, FALSE )) != S_OK)
        return hr;

    return set_current_namespace( writer, elem->ns );
}

/**************************************************************************
 *          WsWriteEndAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteEndAttribute( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    writer->state = WRITER_STATE_STARTELEMENT;
    return S_OK;
}

static HRESULT write_startelement( struct writer *writer )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    ULONG size, i;
    HRESULT hr;

    /* '<prefix:localname prefix:attr="value"... xmlns:prefix="ns"'... */

    size = elem->localName->length + 1 /* '<' */;
    if (elem->prefix) size += elem->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, '<' );
    if (elem->prefix)
    {
        write_bytes( writer, elem->prefix->bytes, elem->prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, elem->localName->bytes, elem->localName->length );
    for (i = 0; i < elem->attributeCount; i++)
    {
        if (elem->attributes[i]->isXmlNs) continue;
        if ((hr = write_attribute( writer, elem->attributes[i] )) != S_OK) return hr;
    }
    for (i = 0; i < elem->attributeCount; i++)
    {
        if (!elem->attributes[i]->isXmlNs || !elem->attributes[i]->prefix) continue;
        if ((hr = write_namespace_attribute( writer, elem->attributes[i] )) != S_OK) return hr;
    }
    for (i = 0; i < elem->attributeCount; i++)
    {
        if (!elem->attributes[i]->isXmlNs || elem->attributes[i]->prefix) continue;
        if ((hr = write_namespace_attribute( writer, elem->attributes[i] )) != S_OK) return hr;
    }
    return S_OK;
}

static struct node *write_find_startelement( struct writer *writer )
{
    struct node *node;
    for (node = writer->current; node; node = node->parent)
    {
        if (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT) return node;
    }
    return NULL;
}

static inline BOOL is_empty_element( const struct node *node )
{
    const struct node *head = LIST_ENTRY( list_head( &node->children ), struct node, entry );
    return node_type( head ) == WS_XML_NODE_TYPE_END_ELEMENT;
}

static HRESULT write_endelement( struct writer *writer, const WS_XML_ELEMENT_NODE *elem )
{
    ULONG size;
    HRESULT hr;

    /* '/>' */

    if (elem->isEmpty && writer->state != WRITER_STATE_ENDSTARTELEMENT)
    {
        if ((hr = write_grow_buffer( writer, 2 )) != S_OK) return hr;
        write_char( writer, '/' );
        write_char( writer, '>' );
        return S_OK;
    }

    /* '</prefix:localname>' */

    size = elem->localName->length + 3 /* '</>' */;
    if (elem->prefix) size += elem->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, '<' );
    write_char( writer, '/' );
    if (elem->prefix)
    {
        write_bytes( writer, elem->prefix->bytes, elem->prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, elem->localName->bytes, elem->localName->length );
    write_char( writer, '>' );
    return S_OK;
}

static HRESULT write_close_element( struct writer *writer, struct node *node )
{
    WS_XML_ELEMENT_NODE *elem = &node->hdr;
    elem->isEmpty = is_empty_element( node );
    return write_endelement( writer, elem );
}

static HRESULT write_endelement_node( struct writer *writer )
{
    struct node *node;
    HRESULT hr;

    if (!(node = write_find_startelement( writer ))) return WS_E_INVALID_FORMAT;
    if (writer->state == WRITER_STATE_STARTELEMENT)
    {
        if ((hr = write_set_element_namespace( writer )) != S_OK) return hr;
        if ((hr = write_startelement( writer )) != S_OK) return hr;
    }
    if ((hr = write_close_element( writer, node )) != S_OK) return hr;
    writer->current = node->parent;
    writer->state   = WRITER_STATE_ENDELEMENT;
    return S_OK;
}

/**************************************************************************
 *          WsWriteEndElement		[webservices.@]
 */
HRESULT WINAPI WsWriteEndElement( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    return write_endelement_node( writer );
}

static HRESULT write_endstartelement( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, '>' );
    return S_OK;
}

/**************************************************************************
 *          WsWriteEndStartElement		[webservices.@]
 */
HRESULT WINAPI WsWriteEndStartElement( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    if ((hr = write_set_element_namespace( writer )) != S_OK) return hr;
    if ((hr = write_startelement( writer )) != S_OK) return hr;
    if ((hr = write_endstartelement( writer )) != S_OK) return hr;

    writer->state = WRITER_STATE_ENDSTARTELEMENT;
    return S_OK;
}

static HRESULT write_add_attribute( struct writer *writer, const WS_XML_STRING *prefix,
                                    const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                    BOOL single )
{
    WS_XML_ATTRIBUTE *attr;
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!(attr = heap_alloc_zero( sizeof(*attr) ))) return E_OUTOFMEMORY;

    if (!prefix) prefix = elem->prefix;

    attr->singleQuote = !!single;
    if (prefix && !(attr->prefix = alloc_xml_string( prefix->bytes, prefix->length )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->localName = alloc_xml_string( localname->bytes, localname->length )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->ns = alloc_xml_string( ns->bytes, ns->length )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if ((hr = append_attribute( elem, attr )) != S_OK)
    {
        free_attribute( attr );
        return hr;
    }
    return S_OK;
}

/**************************************************************************
 *          WsWriteStartAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteStartAttribute( WS_XML_WRITER *handle, const WS_XML_STRING *prefix,
                                      const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                      BOOL single, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %s %s %s %d %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(localname),
           debugstr_xmlstr(ns), single, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !localname || !ns) return E_INVALIDARG;

    if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    if ((hr = write_add_attribute( writer, prefix, localname, ns, single )) != S_OK) return hr;
    writer->state = WRITER_STATE_STARTATTRIBUTE;
    return S_OK;
}

/* flush current start element if necessary */
static HRESULT write_flush( struct writer *writer )
{
    if (writer->state == WRITER_STATE_STARTELEMENT)
    {
        HRESULT hr;
        if ((hr = write_set_element_namespace( writer )) != S_OK) return hr;
        if ((hr = write_startelement( writer )) != S_OK) return hr;
        if ((hr = write_endstartelement( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_ENDSTARTELEMENT;
    }
    return S_OK;
}

static HRESULT write_add_cdata_node( struct writer *writer )
{
    struct node *node, *parent;
    if (!(parent = find_parent( writer->current ))) return WS_E_INVALID_FORMAT;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_CDATA ))) return E_OUTOFMEMORY;
    write_insert_node( writer, parent, node );
    return S_OK;
}

static HRESULT write_add_endcdata_node( struct writer *writer )
{
    struct node *node;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_END_CDATA ))) return E_OUTOFMEMORY;
    node->parent = writer->current;
    list_add_tail( &node->parent->children, &node->entry );
    return S_OK;
}

static HRESULT write_cdata( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_grow_buffer( writer, 9 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"<![CDATA[", 9 );
    return S_OK;
}

static HRESULT write_cdata_node( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_flush( writer )) != S_OK) return hr;
    if ((hr = write_add_cdata_node( writer )) != S_OK) return hr;
    if ((hr = write_add_endcdata_node( writer )) != S_OK) return hr;
    if ((hr = write_cdata( writer )) != S_OK) return hr;
    writer->state = WRITER_STATE_STARTCDATA;
    return S_OK;
}

/**************************************************************************
 *          WsWriteStartCData		[webservices.@]
 */
HRESULT WINAPI WsWriteStartCData( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    return write_cdata_node( writer );
}

static HRESULT write_endcdata( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_grow_buffer( writer, 3 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"]]>", 3 );
    return S_OK;
}

static HRESULT write_endcdata_node( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_endcdata( writer )) != S_OK) return hr;
    writer->current = writer->current->parent;
    writer->state = WRITER_STATE_ENDCDATA;
    return S_OK;
}

/**************************************************************************
 *          WsWriteEndCData		[webservices.@]
 */
HRESULT WINAPI WsWriteEndCData( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    if (writer->state != WRITER_STATE_TEXT) return WS_E_INVALID_OPERATION;

    return write_endcdata_node( writer );
}

static HRESULT write_add_element_node( struct writer *writer, const WS_XML_STRING *prefix,
                                       const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    struct node *node, *parent;
    WS_XML_ELEMENT_NODE *elem;

    if (!(parent = find_parent( writer->current ))) return WS_E_INVALID_FORMAT;

    if (!prefix && node_type( writer->current ) == WS_XML_NODE_TYPE_ELEMENT)
        prefix = writer->current->hdr.prefix;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) return E_OUTOFMEMORY;
    elem = &node->hdr;

    if (prefix && !(elem->prefix = alloc_xml_string( prefix->bytes, prefix->length )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    if (!(elem->localName = alloc_xml_string( localname->bytes, localname->length )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    if (!(elem->ns = alloc_xml_string( ns->bytes, ns->length )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    write_insert_node( writer, parent, node );
    return S_OK;
}

static HRESULT write_add_endelement_node( struct writer *writer, struct node *parent )
{
    struct node *node;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT ))) return E_OUTOFMEMORY;
    node->parent = parent;
    list_add_tail( &parent->children, &node->entry );
    return S_OK;
}

static HRESULT write_element_node( struct writer *writer, const WS_XML_STRING *prefix,
                                   const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    HRESULT hr;
    if ((hr = write_flush( writer )) != S_OK) return hr;
    if ((hr = write_add_element_node( writer, prefix, localname, ns )) != S_OK) return hr;
    if ((hr = write_add_endelement_node( writer, writer->current )) != S_OK) return hr;
    writer->state = WRITER_STATE_STARTELEMENT;
    return S_OK;
}

/**************************************************************************
 *          WsWriteStartElement		[webservices.@]
 */
HRESULT WINAPI WsWriteStartElement( WS_XML_WRITER *handle, const WS_XML_STRING *prefix,
                                    const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                    WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %s %s %s %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(localname),
           debugstr_xmlstr(ns), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !localname || !ns) return E_INVALIDARG;
    return write_element_node( writer, prefix, localname, ns );
}

static HRESULT text_to_utf8text( const WS_XML_TEXT *text, WS_XML_UTF8_TEXT **ret )
{
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *src = (const WS_XML_UTF8_TEXT *)text;
        if (!(*ret = alloc_utf8_text( src->value.bytes, src->value.length ))) return E_OUTOFMEMORY;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *src = (const WS_XML_UTF16_TEXT *)text;
        const WCHAR *str = (const WCHAR *)src->bytes;
        ULONG len = src->byteCount / sizeof(WCHAR), len_utf8;

        if (src->byteCount % sizeof(WCHAR)) return E_INVALIDARG;
        len_utf8 = WideCharToMultiByte( CP_UTF8, 0, str, len, NULL, 0, NULL, NULL );
        if (!(*ret = alloc_utf8_text( NULL, len_utf8 ))) return E_OUTOFMEMORY;
        WideCharToMultiByte( CP_UTF8, 0, str, len, (char *)(*ret)->value.bytes, (*ret)->value.length, NULL, NULL );
        return S_OK;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }
}

static HRESULT write_set_attribute_value( struct writer *writer, const WS_XML_TEXT *value )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;

    if ((hr = text_to_utf8text( value, &utf8 )) != S_OK) return hr;
    elem->attributes[elem->attributeCount - 1]->value = &utf8->text;
    return S_OK;
}

static HRESULT write_add_text_node( struct writer *writer, const WS_XML_TEXT *value )
{
    struct node *node;
    WS_XML_TEXT_NODE *text;
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;

    if (node_type( writer->current ) != WS_XML_NODE_TYPE_ELEMENT &&
        node_type( writer->current ) != WS_XML_NODE_TYPE_BOF &&
        node_type( writer->current ) != WS_XML_NODE_TYPE_CDATA) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    if ((hr = text_to_utf8text( value, &utf8 )) != S_OK)
    {
        heap_free( node );
        return hr;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &utf8->text;

    write_insert_node( writer, writer->current, node );
    return S_OK;
}

static HRESULT write_text( struct writer *writer )
{
    const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)writer->current;
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text->text;
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, utf8->value.length )) != S_OK) return hr;
    write_bytes( writer, utf8->value.bytes, utf8->value.length );
    return S_OK;
}

static HRESULT write_text_node( struct writer *writer, const WS_XML_TEXT *text )
{
    HRESULT hr;
    if ((hr = write_flush( writer )) != S_OK) return hr;
    if ((hr = write_add_text_node( writer, text )) != S_OK) return hr;
    if ((hr = write_text( writer )) != S_OK) return hr;
    writer->state = WRITER_STATE_TEXT;
    return S_OK;
}

/**************************************************************************
 *          WsWriteText		[webservices.@]
 */
HRESULT WINAPI WsWriteText( WS_XML_WRITER *handle, const WS_XML_TEXT *text, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p %p\n", handle, text, error );

    if (!writer || !text) return E_INVALIDARG;

    if (writer->state == WRITER_STATE_STARTATTRIBUTE) return write_set_attribute_value( writer, text );
    return write_text_node( writer, text );
}

static ULONG format_bool( const BOOL *ptr, unsigned char *buf )
{
    static const unsigned char bool_true[] = {'t','r','u','e'}, bool_false[] = {'f','a','l','s','e'};
    if (*ptr)
    {
        memcpy( buf, bool_true, sizeof(bool_true) );
        return sizeof(bool_true);
    }
    memcpy( buf, bool_false, sizeof(bool_false) );
    return sizeof(bool_false);
}

static ULONG format_int8( const INT8 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%d", *ptr );
}

static ULONG format_int16( const INT16 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%d", *ptr );
}

static ULONG format_int32( const INT32 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%d", *ptr );
}

static ULONG format_int64( const INT64 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%I64d", *ptr );
}

static ULONG format_uint8( const UINT8 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%u", *ptr );
}

static ULONG format_uint16( const UINT16 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%u", *ptr );
}

static ULONG format_uint32( const UINT32 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%u", *ptr );
}

static ULONG format_uint64( const UINT64 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%I64u", *ptr );
}

static HRESULT write_type_text( struct writer *writer, WS_TYPE_MAPPING mapping, const WS_XML_TEXT *text )
{
    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        return write_text_node( writer, text );

    case WS_ATTRIBUTE_TYPE_MAPPING:
        return write_set_attribute_value( writer, text );

    case WS_ANY_ELEMENT_TYPE_MAPPING:
        switch (writer->state)
        {
        case WRITER_STATE_STARTATTRIBUTE:
            return write_set_attribute_value( writer, text );

        case WRITER_STATE_STARTELEMENT:
            return write_text_node( writer, text );

        default:
            FIXME( "writer state %u not handled\n", writer->state );
            return E_NOTIMPL;
        }

    default:
        FIXME( "mapping %u not implemented\n", mapping );
        return E_NOTIMPL;
    }
}

static HRESULT write_type_bool( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_BOOL_DESCRIPTION *desc, const BOOL *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[6]; /* "false" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_bool( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_int8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_INT8_DESCRIPTION *desc, const INT8 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[5]; /* "-128" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_int8( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_int16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT16_DESCRIPTION *desc, const INT16 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[7]; /* "-32768" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_int16( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_int32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT32_DESCRIPTION *desc, const INT32 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[12]; /* "-2147483648" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_int32( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_int64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT64_DESCRIPTION *desc, const INT64 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[21]; /* "-9223372036854775808" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_int64( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_uint8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_UINT8_DESCRIPTION *desc, const UINT8 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[4]; /* "255" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_uint8( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_uint16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT16_DESCRIPTION *desc, const UINT16 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[6]; /* "65535" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_uint16( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_uint32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT32_DESCRIPTION *desc, const UINT32 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[11]; /* "4294967295" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_uint32( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_uint64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT64_DESCRIPTION *desc, const UINT64 *value )
{
    WS_XML_UTF8_TEXT utf8;
    unsigned char buf[21]; /* "18446744073709551615" */

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = buf;
    utf8.value.length  = format_uint64( value, buf );
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_wsz( struct writer *writer, WS_TYPE_MAPPING mapping,
                               const WS_WSZ_DESCRIPTION *desc, const WCHAR *value )
{
    WS_XML_UTF16_TEXT utf16;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    utf16.text.textType = WS_XML_TEXT_TYPE_UTF16;
    utf16.bytes         = (BYTE *)value;
    utf16.byteCount     = strlenW( value ) * sizeof(WCHAR);
    return write_type_text( writer, mapping, &utf16.text );
}

static HRESULT write_type( struct writer *, WS_TYPE_MAPPING, WS_TYPE, const void *, WS_WRITE_OPTION,
                           const void *, ULONG );

static HRESULT write_type_struct_field( struct writer *writer, const WS_FIELD_DESCRIPTION *desc,
                                        const void *value, ULONG size )
{
    HRESULT hr;
    WS_TYPE_MAPPING mapping;
    WS_WRITE_OPTION option;

    if (!desc->options || desc->options == WS_FIELD_OPTIONAL) option = 0;
    else if (desc->options == WS_FIELD_POINTER) option = WS_WRITE_REQUIRED_POINTER;
    else
    {
        FIXME( "options 0x%x not supported\n", desc->options );
        return E_NOTIMPL;
    }

    switch (desc->mapping)
    {
    case WS_ATTRIBUTE_FIELD_MAPPING:
        if (!desc->localName || !desc->ns) return E_INVALIDARG;
        if ((hr = write_add_attribute( writer, NULL, desc->localName, desc->ns, FALSE )) != S_OK)
            return hr;
        writer->state = WRITER_STATE_STARTATTRIBUTE;

        mapping = WS_ATTRIBUTE_TYPE_MAPPING;
        break;

    case WS_ELEMENT_FIELD_MAPPING:
        if ((hr = write_element_node( writer, NULL, desc->localName, desc->ns )) != S_OK) return hr;
        mapping = WS_ELEMENT_TYPE_MAPPING;
        break;

    case WS_TEXT_FIELD_MAPPING:
        switch (writer->state)
        {
        case WRITER_STATE_STARTELEMENT:
            mapping = WS_ELEMENT_CONTENT_TYPE_MAPPING;
            break;

        case WRITER_STATE_STARTATTRIBUTE:
            mapping = WS_ATTRIBUTE_TYPE_MAPPING;
            break;

        default:
            FIXME( "unhandled writer state %u\n", writer->state );
            return E_NOTIMPL;
        }
        break;

    default:
        FIXME( "field mapping %u not supported\n", desc->mapping );
        return E_NOTIMPL;
    }

    if ((hr = write_type( writer, mapping, desc->type, desc->typeDescription, option, value, size )) != S_OK)
        return hr;

    switch (mapping)
    {
    case WS_ATTRIBUTE_TYPE_MAPPING:
        writer->state = WRITER_STATE_STARTELEMENT;
        break;

    case WS_ELEMENT_TYPE_MAPPING:
        if ((hr = write_endelement_node( writer )) != S_OK) return hr;
        break;

    default: break;
    }

    return S_OK;
}

static ULONG get_field_size( const WS_STRUCT_DESCRIPTION *desc, ULONG index )
{
    if (index < desc->fieldCount - 1) return desc->fields[index + 1]->offset - desc->fields[index]->offset;
    return desc->size - desc->fields[index]->offset;
}

static HRESULT write_type_struct( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_STRUCT_DESCRIPTION *desc, const void *value )
{
    ULONG i, size;
    HRESULT hr;
    const char *ptr;

    if (!desc) return E_INVALIDARG;

    if (desc->structOptions)
    {
        FIXME( "struct options 0x%x not supported\n", desc->structOptions );
        return E_NOTIMPL;
    }

    for (i = 0; i < desc->fieldCount; i++)
    {
        ptr = (const char *)value + desc->fields[i]->offset;
        size = get_field_size( desc, i );
        if ((hr = write_type_struct_field( writer, desc->fields[i], ptr, size )) != S_OK)
            return hr;
    }

    return S_OK;
}

static HRESULT write_type( struct writer *writer, WS_TYPE_MAPPING mapping, WS_TYPE type,
                           const void *desc, WS_WRITE_OPTION option, const void *value,
                           ULONG size )
{
    switch (type)
    {
    case WS_STRUCT_TYPE:
    {
        const void * const *ptr = value;

        if (!desc || (option && option != WS_WRITE_REQUIRED_POINTER) || size != sizeof(*ptr))
            return E_INVALIDARG;

        return write_type_struct( writer, mapping, desc, *ptr );
    }
    case WS_BOOL_TYPE:
    {
        const BOOL *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_bool( writer, mapping, desc, ptr );
    }
    case WS_INT8_TYPE:
    {
        const INT8 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int8( writer, mapping, desc, ptr );
    }
    case WS_INT16_TYPE:
    {
        const INT16 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int16( writer, mapping, desc, ptr );
    }
    case WS_INT32_TYPE:
    {
        const INT32 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int32( writer, mapping, desc, ptr );
    }
    case WS_INT64_TYPE:
    {
        const INT64 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int64( writer, mapping, desc, ptr );
    }
    case WS_UINT8_TYPE:
    {
        const UINT8 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint8( writer, mapping, desc, ptr );
    }
    case WS_UINT16_TYPE:
    {
        const UINT16 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint16( writer, mapping, desc, ptr );
    }
    case WS_UINT32_TYPE:
    {
        const UINT32 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint32( writer, mapping, desc, ptr );
    }
    case WS_UINT64_TYPE:
    {
        const UINT64 *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_VALUE) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint64( writer, mapping, desc, ptr );
    }
    case WS_WSZ_TYPE:
    {
        const WCHAR * const *ptr = value;
        if ((option && option != WS_WRITE_REQUIRED_POINTER) || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_wsz( writer, mapping, desc, *ptr );
    }
    default:
        FIXME( "type %u not supported\n", type );
        return E_NOTIMPL;
    }
}

/**************************************************************************
 *          WsWriteAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteAttribute( WS_XML_WRITER *handle, const WS_ATTRIBUTE_DESCRIPTION *desc,
                                 WS_WRITE_OPTION option, const void *value, ULONG size,
                                 WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p %u %p %u %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !desc || !desc->attributeLocalName || !desc->attributeNs || !value)
        return E_INVALIDARG;

    if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    if ((hr = write_add_attribute( writer, NULL, desc->attributeLocalName, desc->attributeNs,
                                   FALSE )) != S_OK) return hr;
    writer->state = WRITER_STATE_STARTATTRIBUTE;

    return write_type( writer, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->typeDescription,
                       option, value, size );
}

/**************************************************************************
 *          WsWriteElement		[webservices.@]
 */
HRESULT WINAPI WsWriteElement( WS_XML_WRITER *handle, const WS_ELEMENT_DESCRIPTION *desc,
                               WS_WRITE_OPTION option, const void *value, ULONG size,
                               WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p %u %p %u %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !desc || !desc->elementLocalName || !desc->elementNs || !value)
        return E_INVALIDARG;

    if ((hr = write_element_node( writer, NULL, desc->elementLocalName, desc->elementNs )) != S_OK) return hr;

    if ((hr = write_type( writer, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription,
                          option, value, size )) != S_OK) return hr;

    return write_endelement_node( writer );
}

/**************************************************************************
 *          WsWriteType		[webservices.@]
 */
HRESULT WINAPI WsWriteType( WS_XML_WRITER *handle, WS_TYPE_MAPPING mapping, WS_TYPE type,
                            const void *desc, WS_WRITE_OPTION option, const void *value,
                            ULONG size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %u %u %p %u %p %u %p\n", handle, mapping, type, desc, option, value,
           size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !value) return E_INVALIDARG;

    switch (mapping)
    {
    case WS_ATTRIBUTE_TYPE_MAPPING:
        if (writer->state != WRITER_STATE_STARTATTRIBUTE) return WS_E_INVALID_FORMAT;
        hr = write_type( writer, mapping, type, desc, option, value, size );
        break;

    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_FORMAT;
        hr = write_type( writer, mapping, type, desc, option, value, size );
        break;

    case WS_ANY_ELEMENT_TYPE_MAPPING:
        hr = write_type( writer, mapping, type, desc, option, value, size );
        break;

    default:
        FIXME( "mapping %u not implemented\n", mapping );
        return E_NOTIMPL;
    }

    return hr;
}

WS_TYPE map_value_type( WS_VALUE_TYPE type )
{
    switch (type)
    {
    case WS_BOOL_VALUE_TYPE:     return WS_BOOL_TYPE;
    case WS_INT8_VALUE_TYPE:     return WS_INT8_TYPE;
    case WS_INT16_VALUE_TYPE:    return WS_INT16_TYPE;
    case WS_INT32_VALUE_TYPE:    return WS_INT32_TYPE;
    case WS_INT64_VALUE_TYPE:    return WS_INT64_TYPE;
    case WS_UINT8_VALUE_TYPE:    return WS_UINT8_TYPE;
    case WS_UINT16_VALUE_TYPE:   return WS_UINT16_TYPE;
    case WS_UINT32_VALUE_TYPE:   return WS_UINT32_TYPE;
    case WS_UINT64_VALUE_TYPE:   return WS_UINT64_TYPE;
    case WS_FLOAT_VALUE_TYPE:    return WS_FLOAT_TYPE;
    case WS_DOUBLE_VALUE_TYPE:   return WS_DOUBLE_TYPE;
    case WS_DECIMAL_VALUE_TYPE:  return WS_DECIMAL_TYPE;
    case WS_DATETIME_VALUE_TYPE: return WS_DATETIME_TYPE;
    case WS_TIMESPAN_VALUE_TYPE: return WS_TIMESPAN_TYPE;
    case WS_GUID_VALUE_TYPE:     return WS_GUID_TYPE;
    default:
        FIXME( "unhandled type %u\n", type );
        return ~0u;
    }
}

/**************************************************************************
 *          WsWriteValue		[webservices.@]
 */
HRESULT WINAPI WsWriteValue( WS_XML_WRITER *handle, WS_VALUE_TYPE value_type, const void *value,
                             ULONG size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_TYPE_MAPPING mapping;
    WS_TYPE type;

    TRACE( "%p %u %p %u %p\n", handle, value_type, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !value || (type = map_value_type( value_type )) == ~0u) return E_INVALIDARG;

    switch (writer->state)
    {
    case WRITER_STATE_STARTATTRIBUTE:
        mapping = WS_ATTRIBUTE_TYPE_MAPPING;
        break;

    case WRITER_STATE_STARTELEMENT:
        mapping = WS_ELEMENT_TYPE_MAPPING;
        break;

    default:
        return WS_E_INVALID_FORMAT;
    }

    return write_type( writer, mapping, type, NULL, WS_WRITE_REQUIRED_VALUE, value, size );
}

/**************************************************************************
 *          WsWriteXmlBuffer		[webservices.@]
 */
HRESULT WINAPI WsWriteXmlBuffer( WS_XML_WRITER *handle, WS_XML_BUFFER *buffer, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    struct xmlbuf *xmlbuf = (struct xmlbuf *)buffer;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, buffer, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !xmlbuf) return E_INVALIDARG;

    if ((hr = write_grow_buffer( writer, xmlbuf->size )) != S_OK) return hr;
    write_bytes( writer, xmlbuf->ptr, xmlbuf->size );
    return S_OK;
}

/**************************************************************************
 *          WsWriteXmlBufferToBytes		[webservices.@]
 */
HRESULT WINAPI WsWriteXmlBufferToBytes( WS_XML_WRITER *handle, WS_XML_BUFFER *buffer,
                                        const WS_XML_WRITER_ENCODING *encoding,
                                        const WS_XML_WRITER_PROPERTY *properties, ULONG count,
                                        WS_HEAP *heap, void **bytes, ULONG *size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    struct xmlbuf *xmlbuf = (struct xmlbuf *)buffer;
    HRESULT hr;
    char *buf;
    ULONG i;

    TRACE( "%p %p %p %p %u %p %p %p %p\n", handle, buffer, encoding, properties, count, heap,
           bytes, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !xmlbuf || !heap || !bytes) return E_INVALIDARG;

    if (encoding && encoding->encodingType != WS_XML_WRITER_ENCODING_TYPE_TEXT)
    {
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        return E_NOTIMPL;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if (!(buf = ws_alloc( heap, xmlbuf->size ))) return WS_E_QUOTA_EXCEEDED;
    memcpy( buf, xmlbuf->ptr, xmlbuf->size );
    *bytes = buf;
    return S_OK;
}

/**************************************************************************
 *          WsWriteXmlnsAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteXmlnsAttribute( WS_XML_WRITER *handle, const WS_XML_STRING *prefix,
                                      const WS_XML_STRING *ns, BOOL single, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %s %s %d %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(ns),
           single, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !ns) return E_INVALIDARG;
    if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    if (namespace_in_scope( &writer->current->hdr, prefix, ns )) return S_OK;
    return write_add_namespace_attribute( writer, prefix, ns, single );
}

static HRESULT write_move_to( struct writer *writer, WS_MOVE_TO move, BOOL *found )
{
    BOOL success = FALSE;
    struct node *node = writer->current;

    switch (move)
    {
    case WS_MOVE_TO_ROOT_ELEMENT:
        success = move_to_root_element( writer->root, &node );
        break;

    case WS_MOVE_TO_NEXT_ELEMENT:
        success = move_to_next_element( &node );
        break;

    case WS_MOVE_TO_PREVIOUS_ELEMENT:
        success = move_to_prev_element( &node );
        break;

    case WS_MOVE_TO_CHILD_ELEMENT:
        success = move_to_child_element( &node );
        break;

    case WS_MOVE_TO_END_ELEMENT:
        success = move_to_end_element( &node );
        break;

    case WS_MOVE_TO_PARENT_ELEMENT:
        success = move_to_parent_element( &node );
        break;

    case WS_MOVE_TO_FIRST_NODE:
        success = move_to_first_node( &node );
        break;

    case WS_MOVE_TO_NEXT_NODE:
        success = move_to_next_node( &node );
        break;

    case WS_MOVE_TO_PREVIOUS_NODE:
        success = move_to_prev_node( &node );
        break;

    case WS_MOVE_TO_CHILD_NODE:
        success = move_to_child_node( &node );
        break;

    case WS_MOVE_TO_BOF:
        success = move_to_bof( writer->root, &node );
        break;

    case WS_MOVE_TO_EOF:
        success = move_to_eof( writer->root, &node );
        break;

    default:
        FIXME( "unhandled move %u\n", move );
        return E_NOTIMPL;
    }

    if (success && node == writer->root) return E_INVALIDARG;
    writer->current = node;

    if (found)
    {
        *found = success;
        return S_OK;
    }
    return success ? S_OK : WS_E_INVALID_FORMAT;
}

/**************************************************************************
 *          WsMoveWriter		[webservices.@]
 */
HRESULT WINAPI WsMoveWriter( WS_XML_WRITER *handle, WS_MOVE_TO move, BOOL *found, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %u %p %p\n", handle, move, found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    if (!writer->output_type) return WS_E_INVALID_OPERATION;

    return write_move_to( writer, move, found );
}

/**************************************************************************
 *          WsGetWriterPosition		[webservices.@]
 */
HRESULT WINAPI WsGetWriterPosition( WS_XML_WRITER *handle, WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !pos) return E_INVALIDARG;
    if (!writer->output_type) return WS_E_INVALID_OPERATION;

    pos->buffer = (WS_XML_BUFFER *)writer->output_buf;
    pos->node   = writer->current;
    return S_OK;
}

/**************************************************************************
 *          WsSetWriterPosition		[webservices.@]
 */
HRESULT WINAPI WsSetWriterPosition( WS_XML_WRITER *handle, const WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !pos || (struct xmlbuf *)pos->buffer != writer->output_buf) return E_INVALIDARG;
    if (!writer->output_type) return WS_E_INVALID_OPERATION;

    writer->current = pos->node;
    return S_OK;
}

static HRESULT write_add_comment_node( struct writer *writer, const WS_XML_STRING *value )
{
    struct node *node, *parent;
    WS_XML_COMMENT_NODE *comment;

    if (!(parent = find_parent( writer->current ))) return WS_E_INVALID_FORMAT;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;

    if (value->length && !(comment->value.bytes = heap_alloc( value->length )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    memcpy( comment->value.bytes, value->bytes, value->length );
    comment->value.length = value->length;

    write_insert_node( writer, parent, node );
    return S_OK;
}

static HRESULT write_comment( struct writer *writer )
{
    const WS_XML_COMMENT_NODE *comment = (const WS_XML_COMMENT_NODE *)writer->current;
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, comment->value.length + 7 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"<!--", 4 );
    write_bytes( writer, comment->value.bytes, comment->value.length );
    write_bytes( writer, (const BYTE *)"-->", 3 );
    return S_OK;
}

static HRESULT write_comment_node( struct writer *writer, const WS_XML_STRING *value )
{
    HRESULT hr;
    if ((hr = write_flush( writer )) != S_OK) return hr;
    if ((hr = write_add_comment_node( writer, value )) != S_OK) return hr;
    if ((hr = write_comment( writer )) != S_OK) return hr;
    writer->state = WRITER_STATE_COMMENT;
    return S_OK;
}

static HRESULT write_set_attributes( struct writer *writer, WS_XML_ATTRIBUTE **attrs, ULONG count )
{
    ULONG i;
    HRESULT hr;

    for (i = 0; i < count; i++)
    {
        if ((hr = write_add_attribute( writer, attrs[i]->prefix, attrs[i]->localName, attrs[i]->ns,
                                       attrs[i]->singleQuote )) != S_OK) return hr;
        if ((hr = write_set_attribute_value( writer, attrs[i]->value )) != S_OK) return hr;
    }
    return S_OK;
}

static HRESULT write_node( struct writer *writer, const WS_XML_NODE *node )
{
    HRESULT hr;

    switch (node->nodeType)
    {
    case WS_XML_NODE_TYPE_ELEMENT:
    {
        const WS_XML_ELEMENT_NODE *elem = (const WS_XML_ELEMENT_NODE *)node;
        if ((hr = write_element_node( writer, elem->prefix, elem->localName, elem->ns )) != S_OK) return hr;
        return write_set_attributes( writer, elem->attributes, elem->attributeCount );
    }
    case WS_XML_NODE_TYPE_TEXT:
    {
        const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)node;
        return write_text_node( writer, text->text );
    }
    case WS_XML_NODE_TYPE_END_ELEMENT:
        return write_endelement_node( writer );

    case WS_XML_NODE_TYPE_COMMENT:
    {
        const WS_XML_COMMENT_NODE *comment = (const WS_XML_COMMENT_NODE *)node;
        return write_comment_node( writer, &comment->value );
    }
    case WS_XML_NODE_TYPE_CDATA:
        return write_cdata_node( writer );

    case WS_XML_NODE_TYPE_END_CDATA:
        return write_endcdata_node( writer );

    case WS_XML_NODE_TYPE_EOF:
    case WS_XML_NODE_TYPE_BOF:
        return S_OK;

    default:
        WARN( "unknown node type %u\n", node->nodeType );
        return E_INVALIDARG;
    }
}

/**************************************************************************
 *          WsWriteNode		[webservices.@]
 */
HRESULT WINAPI WsWriteNode( WS_XML_WRITER *handle, const WS_XML_NODE *node, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;

    TRACE( "%p %p %p\n", handle, node, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !node) return E_INVALIDARG;
    if (!writer->output_type) return WS_E_INVALID_OPERATION;

    return write_node( writer, node );
}

static HRESULT write_tree_node( struct writer *writer )
{
    HRESULT hr;

    switch (node_type( writer->current ))
    {
    case WS_XML_NODE_TYPE_ELEMENT:
        if (writer->state == WRITER_STATE_STARTELEMENT && (hr = write_endstartelement( writer )) != S_OK)
            return hr;
        if ((hr = write_startelement( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_STARTELEMENT;
        return S_OK;

    case WS_XML_NODE_TYPE_TEXT:
        if (writer->state == WRITER_STATE_STARTELEMENT && (hr = write_endstartelement( writer )) != S_OK)
            return hr;
        if ((hr = write_text( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_TEXT;
        return S_OK;

    case WS_XML_NODE_TYPE_END_ELEMENT:
        if ((hr = write_close_element( writer, writer->current->parent )) != S_OK) return hr;
        writer->state = WRITER_STATE_ENDELEMENT;
        return S_OK;

    case WS_XML_NODE_TYPE_COMMENT:
        if (writer->state == WRITER_STATE_STARTELEMENT && (hr = write_endstartelement( writer )) != S_OK)
            return hr;
        if ((hr = write_comment( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_COMMENT;
        return S_OK;

    case WS_XML_NODE_TYPE_CDATA:
        if (writer->state == WRITER_STATE_STARTELEMENT && (hr = write_endstartelement( writer )) != S_OK)
            return hr;
        if ((hr = write_cdata( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_STARTCDATA;
        return S_OK;

    case WS_XML_NODE_TYPE_END_CDATA:
        if ((hr = write_endcdata( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_ENDCDATA;
        return S_OK;

    case WS_XML_NODE_TYPE_EOF:
    case WS_XML_NODE_TYPE_BOF:
        return S_OK;

    default:
        ERR( "unknown node type %u\n", node_type(writer->current) );
        return E_INVALIDARG;
    }
}

static HRESULT write_tree( struct writer *writer )
{
    HRESULT hr;

    if ((hr = write_tree_node( writer )) != S_OK) return hr;
    for (;;)
    {
        if (node_type( writer->current ) == WS_XML_NODE_TYPE_EOF) break;
        if (move_to_child_node( &writer->current ))
        {
            if ((hr = write_tree_node( writer )) != S_OK) return hr;
            continue;
        }
        if (move_to_next_node( &writer->current ))
        {
            if ((hr = write_tree_node( writer )) != S_OK) return hr;
            continue;
        }
        if (!move_to_parent_node( &writer->current ) || !move_to_next_node( &writer->current ))
        {
            ERR( "invalid tree\n" );
            return WS_E_INVALID_FORMAT;
        }
        if ((hr = write_tree_node( writer )) != S_OK) return hr;
    }
    return S_OK;
}

static void write_rewind( struct writer *writer )
{
    writer->write_pos = 0;
    writer->current   = writer->root;
    writer->state     = WRITER_STATE_INITIAL;
}

/**************************************************************************
 *          WsCopyNode		[webservices.@]
 */
HRESULT WINAPI WsCopyNode( WS_XML_WRITER *handle, WS_XML_READER *reader, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    struct node *parent, *current = writer->current, *node = NULL;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, reader, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    if (!(parent = find_parent( writer->current ))) return WS_E_INVALID_FORMAT;

    if ((hr = copy_node( reader, &node )) != S_OK) return hr;
    write_insert_node( writer, parent, node );

    write_rewind( writer );
    if ((hr = write_tree( writer )) != S_OK) return hr;

    writer->current = current;
    return S_OK;
}
