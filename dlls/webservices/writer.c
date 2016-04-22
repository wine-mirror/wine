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
    WRITER_STATE_STARTENDELEMENT,
    WRITER_STATE_STARTATTRIBUTE,
    WRITER_STATE_STARTCDATA,
    WRITER_STATE_ENDSTARTELEMENT,
    WRITER_STATE_TEXT,
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

static void write_insert_node( struct writer *writer, struct node *node )
{
    node->parent = writer->current;
    if (writer->current == writer->root)
    {
        struct list *eof = list_tail( &writer->root->children );
        list_add_before( eof, &node->entry );
    }
    else list_add_tail( &writer->current->children, &node->entry );
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

static struct node *write_find_parent_element( struct writer *writer )
{
    struct node *node = writer->current;

    if (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT) return node;
    if (node_type( node->parent ) == WS_XML_NODE_TYPE_ELEMENT) return node->parent;
    return NULL;
}

static HRESULT write_endelement( struct writer *writer )
{
    struct node *node = write_find_parent_element( writer );
    WS_XML_ELEMENT_NODE *elem = &node->hdr;
    ULONG size;
    HRESULT hr;

    if (!elem) return WS_E_INVALID_FORMAT;

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

static const WS_XML_ATTRIBUTE *find_namespace_attribute( const WS_XML_ELEMENT_NODE *elem,
                                                         const WS_XML_STRING *prefix,
                                                         const WS_XML_STRING *ns )
{
    ULONG i;
    for (i = 0; i < elem->attributeCount; i++)
    {
        if (!elem->attributes[i]->isXmlNs) continue;
        if (WsXmlStringEquals( elem->attributes[i]->prefix, prefix, NULL ) == S_OK &&
            WsXmlStringEquals( elem->attributes[i]->ns, ns, NULL ) == S_OK)
        {
            return elem->attributes[i];
        }
    }
    return NULL;
}

static HRESULT write_set_element_namespace( struct writer *writer )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!elem->ns->length || is_current_namespace( writer, elem->ns ) ||
        find_namespace_attribute( elem, elem->prefix, elem->ns )) return S_OK;

    if ((hr = write_add_namespace_attribute( writer, elem->prefix, elem->ns, FALSE )) != S_OK)
        return hr;

    return set_current_namespace( writer, elem->ns );
}

static HRESULT write_endstartelement( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_set_element_namespace( writer )) != S_OK) return hr;
    if ((hr = write_startelement( writer )) != S_OK) return hr;
    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, '>' );
    return S_OK;
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

static HRESULT write_close_element( struct writer *writer )
{
    HRESULT hr;

    if (writer->state == WRITER_STATE_STARTELEMENT)
    {
        /* '/>' */
        if ((hr = write_set_element_namespace( writer )) != S_OK) return hr;
        if ((hr = write_startelement( writer )) != S_OK) return hr;
        if ((hr = write_grow_buffer( writer, 2 )) != S_OK) return hr;
        write_char( writer, '/' );
        write_char( writer, '>' );

        writer->current = writer->current->parent;
        writer->state   = WRITER_STATE_STARTENDELEMENT;
        return S_OK;
    }
    else
    {
        struct node *node = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT );
        if (!node) return E_OUTOFMEMORY;

        /* '</prefix:localname>' */
        if ((hr = write_endelement( writer )) != S_OK)
        {
            free_node( node );
            return hr;
        }

        write_insert_node( writer, node );
        writer->current = node->parent;
        writer->state   = WRITER_STATE_ENDELEMENT;
        return S_OK;
    }
    return WS_E_INVALID_OPERATION;
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

    return write_close_element( writer );
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
    writer->state = WRITER_STATE_STARTATTRIBUTE;
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

    TRACE( "%p %s %s %s %d %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(localname),
           debugstr_xmlstr(ns), single, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !localname || !ns) return E_INVALIDARG;

    if (writer->state != WRITER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    return write_add_attribute( writer, prefix, localname, ns, single );
}

/**************************************************************************
 *          WsWriteStartCData		[webservices.@]
 */
HRESULT WINAPI WsWriteStartCData( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    /* flush current start element if necessary */
    if (writer->state == WRITER_STATE_STARTELEMENT && ((hr = write_endstartelement( writer )) != S_OK))
        return hr;

    if ((hr = write_grow_buffer( writer, 9 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"<![CDATA[", 9 );
    writer->state = WRITER_STATE_STARTCDATA;
    return S_OK;
}

/**************************************************************************
 *          WsWriteEndCData		[webservices.@]
 */
HRESULT WINAPI WsWriteEndCData( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;
    if (writer->state != WRITER_STATE_STARTCDATA) return WS_E_INVALID_OPERATION;

    if ((hr = write_grow_buffer( writer, 3 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"]]>", 3 );
    writer->state = WRITER_STATE_ENDCDATA;
    return S_OK;
}

static HRESULT write_add_element_node( struct writer *writer, const WS_XML_STRING *prefix,
                                       const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    struct node *node;
    WS_XML_ELEMENT_NODE *elem;
    HRESULT hr;

    /* flush current start element if necessary */
    if (writer->state == WRITER_STATE_STARTELEMENT && ((hr = write_endstartelement( writer )) != S_OK))
        return hr;

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
    write_insert_node( writer, node );
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

    return write_add_element_node( writer, prefix, localname, ns );
}

static inline void write_set_attribute_value( struct writer *writer, WS_XML_TEXT *text )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    elem->attributes[elem->attributeCount - 1]->value = text;
}

/**************************************************************************
 *          WsWriteText		[webservices.@]
 */
HRESULT WINAPI WsWriteText( WS_XML_WRITER *handle, const WS_XML_TEXT *text, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_XML_UTF8_TEXT *dst, *src = (WS_XML_UTF8_TEXT *)text;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, text, error );

    if (!writer || !text) return E_INVALIDARG;

    if (text->textType != WS_XML_TEXT_TYPE_UTF8)
    {
        FIXME( "text type %u not supported\n", text->textType );
        return E_NOTIMPL;
    }

    if (writer->state == WRITER_STATE_STARTATTRIBUTE)
    {
        if (!(dst = alloc_utf8_text( src->value.bytes, src->value.length )))
            return E_OUTOFMEMORY;

        write_set_attribute_value( writer, &dst->text );
    }
    else
    {
        if ((hr = write_grow_buffer( writer, src->value.length )) != S_OK) return hr;
        write_bytes( writer, src->value.bytes, src->value.length );
    }

    return S_OK;
}

static WS_XML_TEXT *widechar_to_xmltext( const WCHAR *src, WS_XML_TEXT_TYPE type )
{
    switch (type)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        WS_XML_UTF8_TEXT *text;
        int len = WideCharToMultiByte( CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL ) - 1;
        if (!(text = alloc_utf8_text( NULL, len ))) return NULL;
        WideCharToMultiByte( CP_UTF8, 0, src, -1, (char *)text->value.bytes, text->value.length, NULL, NULL );
        return &text->text;
    }
    default:
        FIXME( "unhandled type %u\n", type );
        return NULL;
    }
}

static WS_XML_UTF8_TEXT *format_bool( const BOOL *ptr )
{
    static const unsigned char bool_true[] = {'t','r','u','e'}, bool_false[] = {'f','a','l','s','e'};
    if (*ptr) return alloc_utf8_text( bool_true, sizeof(bool_true) );
    else return alloc_utf8_text( bool_false, sizeof(bool_false) );
}

static WS_XML_UTF8_TEXT *format_int8( const INT8 *ptr )
{
    char buf[5]; /* "-128" */
    int len = wsprintfA( buf, "%d", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_int16( const INT16 *ptr )
{
    char buf[7]; /* "-32768" */
    int len = wsprintfA( buf, "%d", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_int32( const INT32 *ptr )
{
    char buf[12]; /* "-2147483648" */
    int len = wsprintfA( buf, "%d", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_int64( const INT64 *ptr )
{
    char buf[21]; /* "-9223372036854775808" */
    int len = wsprintfA( buf, "%I64d", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_uint8( const UINT8 *ptr )
{
    char buf[4]; /* "255" */
    int len = wsprintfA( buf, "%u", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_uint16( const UINT16 *ptr )
{
    char buf[6]; /* "65535" */
    int len = wsprintfA( buf, "%u", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_uint32( const UINT32 *ptr )
{
    char buf[11]; /* "4294967295" */
    int len = wsprintfA( buf, "%u", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static WS_XML_UTF8_TEXT *format_uint64( const UINT64 *ptr )
{
    char buf[21]; /* "18446744073709551615" */
    int len = wsprintfA( buf, "%I64u", *ptr );
    return alloc_utf8_text( (const unsigned char *)buf, len );
}

static HRESULT write_add_text_node( struct writer *writer, WS_XML_TEXT *value )
{
    struct node *node;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;
    text->text = value;

    write_insert_node( writer, node );
    writer->state = WRITER_STATE_TEXT;
    return S_OK;
}

static HRESULT write_text_node( struct writer *writer )
{
    HRESULT hr;
    WS_XML_TEXT_NODE *node = (WS_XML_TEXT_NODE *)writer->current;
    WS_XML_UTF8_TEXT *text = (WS_XML_UTF8_TEXT *)node->text;

    if ((hr = write_grow_buffer( writer, text->value.length )) != S_OK) return hr;
    write_bytes( writer, text->value.bytes, text->value.length );
    return S_OK;
}

static HRESULT write_type_text( struct writer *writer, WS_TYPE_MAPPING mapping,
                                WS_XML_TEXT *text )
{
    HRESULT hr;

    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        if ((hr = write_endstartelement( writer )) != S_OK) return hr;
        if ((hr = write_add_text_node( writer, text )) != S_OK) return hr;
        return write_text_node( writer );

    case WS_ATTRIBUTE_TYPE_MAPPING:
        write_set_attribute_value( writer, text );
        return S_OK;

    case WS_ANY_ELEMENT_TYPE_MAPPING:
        switch (writer->state)
        {
        case WRITER_STATE_STARTATTRIBUTE:
            write_set_attribute_value( writer, text );
            return S_OK;

        case WRITER_STATE_STARTELEMENT:
            if ((hr = write_endstartelement( writer )) != S_OK) return hr;
            if ((hr = write_add_text_node( writer, text )) != S_OK) return hr;
            return write_text_node( writer );

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
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_bool( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_int8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_INT8_DESCRIPTION *desc, const INT8 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_int8( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_int16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT16_DESCRIPTION *desc, const INT16 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_int16( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_int32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT32_DESCRIPTION *desc, const INT32 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_int32( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_int64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT64_DESCRIPTION *desc, const INT64 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_int64( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_uint8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_UINT8_DESCRIPTION *desc, const UINT8 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_uint8( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_uint16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT16_DESCRIPTION *desc, const UINT16 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_uint16( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_uint32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT32_DESCRIPTION *desc, const UINT32 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_uint32( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_uint64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT64_DESCRIPTION *desc, const UINT64 *value )
{
    WS_XML_UTF8_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = format_uint64( value ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, &text->text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
}

static HRESULT write_type_wsz( struct writer *writer, WS_TYPE_MAPPING mapping,
                               const WS_WSZ_DESCRIPTION *desc, const WCHAR *value )
{
    WS_XML_TEXT *text;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if (!(text = widechar_to_xmltext( value, WS_XML_TEXT_TYPE_UTF8 ))) return E_OUTOFMEMORY;
    if ((hr = write_type_text( writer, mapping, text )) == S_OK) return S_OK;
    heap_free( text );
    return hr;
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

        mapping = WS_ATTRIBUTE_TYPE_MAPPING;
        break;

    case WS_ELEMENT_FIELD_MAPPING:
        if ((hr = write_add_element_node( writer, NULL, desc->localName, desc->ns )) != S_OK)
            return hr;

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
        if ((hr = write_close_element( writer )) != S_OK) return hr;
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
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_bool( writer, mapping, desc, ptr );
    }
    case WS_INT8_TYPE:
    {
        const INT8 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int8( writer, mapping, desc, ptr );
    }
    case WS_INT16_TYPE:
    {
        const INT16 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int16( writer, mapping, desc, ptr );
    }
    case WS_INT32_TYPE:
    {
        const INT32 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int32( writer, mapping, desc, ptr );
    }
    case WS_INT64_TYPE:
    {
        const INT64 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_int64( writer, mapping, desc, ptr );
    }
    case WS_UINT8_TYPE:
    {
        const UINT8 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint8( writer, mapping, desc, ptr );
    }
    case WS_UINT16_TYPE:
    {
        const UINT16 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint16( writer, mapping, desc, ptr );
    }
    case WS_UINT32_TYPE:
    {
        const UINT32 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
        return write_type_uint32( writer, mapping, desc, ptr );
    }
    case WS_UINT64_TYPE:
    {
        const UINT64 *ptr = value;
        if (option != WS_WRITE_REQUIRED_VALUE || size != sizeof(*ptr)) return E_INVALIDARG;
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

    if ((hr = write_add_element_node( writer, NULL, desc->elementLocalName, desc->elementNs )) != S_OK)
        return hr;

    if ((hr = write_type( writer, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription,
                          option, value, size )) != S_OK) return hr;

    return write_close_element( writer );
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

static WS_TYPE map_value_type( WS_VALUE_TYPE type )
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

    if (find_namespace_attribute( &writer->current->hdr, prefix, ns )) return S_OK;
    return write_add_namespace_attribute( writer, prefix, ns, single );
}
