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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const struct prop_desc writer_props[] =
{
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_DEPTH */
    { sizeof(BOOL), FALSE },        /* WS_XML_WRITER_PROPERTY_ALLOW_FRAGMENT */
    { sizeof(ULONG), FALSE },       /* WS_XML_WRITER_PROPERTY_MAX_ATTRIBUTES */
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
    ULONG                        magic;
    CRITICAL_SECTION             cs;
    ULONG                        write_pos;
    unsigned char               *write_bufptr;
    enum writer_state            state;
    struct node                 *root;
    struct node                 *current;
    WS_XML_STRING               *current_ns;
    WS_XML_WRITER_ENCODING_TYPE  output_enc;
    WS_CHARSET                   output_charset;
    WS_XML_WRITER_OUTPUT_TYPE    output_type;
    WS_WRITE_CALLBACK            output_cb;
    void                        *output_cb_state;
    struct xmlbuf               *output_buf;
    BOOL                         output_buf_user;
    WS_HEAP                     *output_heap;
    unsigned char               *stream_buf;
    const WS_XML_DICTIONARY     *dict;
    BOOL                         dict_do_lookup;
    WS_DYNAMIC_STRING_CALLBACK   dict_cb;
    void                        *dict_cb_state;
    ULONG                        prop_count;
    struct prop                  prop[ARRAY_SIZE( writer_props )];
};

#define WRITER_MAGIC (('W' << 24) | ('R' << 16) | ('I' << 8) | 'T')

static struct writer *alloc_writer(void)
{
    static const ULONG count = ARRAY_SIZE( writer_props );
    struct writer *ret;
    ULONG size = sizeof(*ret) + prop_size( writer_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;

    ret->magic      = WRITER_MAGIC;
    InitializeCriticalSectionEx( &ret->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": writer.cs");

    prop_init( writer_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void free_writer( struct writer *writer )
{
    destroy_nodes( writer->root );
    free_xml_string( writer->current_ns );
    WsFreeHeap( writer->output_heap );
    free( writer->stream_buf );

    writer->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &writer->cs );
    free( writer );
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

static struct node *find_parent( struct writer *writer )
{
    if (is_valid_parent( writer->current )) return writer->current;
    if (is_valid_parent( writer->current->parent )) return writer->current->parent;
    return NULL;
}

static HRESULT init_writer( struct writer *writer )
{
    struct node *node;

    writer->write_pos      = 0;
    writer->write_bufptr   = NULL;
    destroy_nodes( writer->root );
    writer->root           = writer->current = NULL;
    free_xml_string( writer->current_ns );
    writer->current_ns     = NULL;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_EOF ))) return E_OUTOFMEMORY;
    write_insert_eof( writer, node );
    writer->state          = WRITER_STATE_INITIAL;
    writer->output_enc     = WS_XML_WRITER_ENCODING_TYPE_TEXT;
    writer->output_charset = WS_CHARSET_UTF8;
    writer->dict           = NULL;
    writer->dict_do_lookup = FALSE;
    writer->dict_cb        = NULL;
    writer->dict_cb_state  = NULL;
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

    TRACE( "%p %lu %p %p\n", properties, count, handle, error );
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

    hr = WsCreateHeap( 1 << 20, 0, NULL, 0, &writer->output_heap, NULL );
    if (hr != S_OK)
    {
        free_writer( writer );
        return hr;
    }

    hr = init_writer( writer );
    if (hr != S_OK)
    {
        free_writer( writer );
        return hr;
    }

    TRACE( "created %p\n", writer );
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

    if (!writer) return;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return;
    }

    writer->magic = 0;

    LeaveCriticalSection( &writer->cs );
    free_writer( writer );
}

/**************************************************************************
 *          WsGetWriterProperty		[webservices.@]
 */
HRESULT WINAPI WsGetWriterProperty( WS_XML_WRITER *handle, WS_XML_WRITER_PROPERTY_ID id,
                                    void *buf, ULONG size, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->output_type != WS_XML_WRITER_OUTPUT_TYPE_BUFFER) hr = WS_E_INVALID_OPERATION;
    else
    {
        switch (id)
        {
        case WS_XML_WRITER_PROPERTY_BYTES:
        {
            WS_BYTES *bytes = buf;
            if (size != sizeof(*bytes)) hr = E_INVALIDARG;
            else
            {
                bytes->bytes  = writer->output_buf->bytes.bytes;
                bytes->length = writer->output_buf->bytes.length;
            }
            break;
        }
        case WS_XML_WRITER_PROPERTY_BUFFERS:
            if (writer->output_buf->bytes.length)
            {
                WS_BUFFERS *buffers = buf;
                if (size != sizeof(*buffers)) hr = E_INVALIDARG;
                else
                {
                    buffers->bufferCount = 1;
                    buffers->buffers     = &writer->output_buf->bytes;
                }
                break;
            }
            /* fall through */
        default:
            hr = prop_get( writer->prop, writer->prop_count, id, buf, size );
        }
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static void set_output_buffer( struct writer *writer, struct xmlbuf *xmlbuf )
{
    /* free current buffer if it's ours */
    if (writer->output_buf && !writer->output_buf_user)
    {
        free_xmlbuf( writer->output_buf );
    }
    writer->output_buf   = xmlbuf;
    writer->output_type  = WS_XML_WRITER_OUTPUT_TYPE_BUFFER;
    writer->write_bufptr = xmlbuf->bytes.bytes;
    writer->write_pos    = 0;
}

static void set_output_stream( struct writer *writer, WS_WRITE_CALLBACK callback, void *state )
{
    writer->output_type     = WS_XML_WRITER_OUTPUT_TYPE_STREAM;
    writer->output_cb       = callback;
    writer->output_cb_state = state;
    writer->write_bufptr    = writer->stream_buf;
    writer->write_pos       = 0;
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

    TRACE( "%p %p %p %p %lu %p\n", handle, encoding, output, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) goto done;
    }

    if ((hr = init_writer( writer )) != S_OK) goto done;

    switch (encoding->encodingType)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:
    {
        const WS_XML_WRITER_TEXT_ENCODING *text = (const WS_XML_WRITER_TEXT_ENCODING *)encoding;
        if (text->charSet != WS_CHARSET_UTF8)
        {
            FIXME( "charset %u not supported\n", text->charSet );
            hr = E_NOTIMPL;
            goto done;
        }
        writer->output_enc     = WS_XML_WRITER_ENCODING_TYPE_TEXT;
        writer->output_charset = WS_CHARSET_UTF8;
        break;
    }
    case WS_XML_WRITER_ENCODING_TYPE_BINARY:
    {
        const WS_XML_WRITER_BINARY_ENCODING *bin = (const WS_XML_WRITER_BINARY_ENCODING *)encoding;
        writer->output_enc     = WS_XML_WRITER_ENCODING_TYPE_BINARY;
        writer->output_charset = 0;
        writer->dict           = bin->staticDictionary;
        writer->dict_cb        = bin->dynamicStringCallback;
        writer->dict_cb_state  = bin->dynamicStringCallbackState;
        break;
    }
    default:
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        hr = E_NOTIMPL;
        goto done;
    }

    switch (output->outputType)
    {
    case WS_XML_WRITER_OUTPUT_TYPE_BUFFER:
    {
        struct xmlbuf *xmlbuf;
        if (!(xmlbuf = alloc_xmlbuf( writer->output_heap, 0, writer->output_enc, writer->output_charset,
                                     writer->dict, NULL )))
        {
            hr = WS_E_QUOTA_EXCEEDED;
            goto done;
        }
        set_output_buffer( writer, xmlbuf );
        writer->output_buf_user = FALSE;
        break;
    }
    case WS_XML_WRITER_OUTPUT_TYPE_STREAM:
    {
        const WS_XML_WRITER_STREAM_OUTPUT *stream = (const WS_XML_WRITER_STREAM_OUTPUT *)output;
        if (!writer->stream_buf && !(writer->stream_buf = malloc( STREAM_BUFSIZE )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        set_output_stream( writer, stream->writeCallback, stream->writeCallbackState );
        break;

    }
    default:
        FIXME( "output type %u not supported\n", output->outputType );
        hr = E_NOTIMPL;
        goto done;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) hr = E_OUTOFMEMORY;
    else write_insert_bof( writer, node );

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    TRACE( "%p %p %p %lu %p\n", handle, buffer, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !xmlbuf) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) goto done;
    }

    if ((hr = init_writer( writer )) != S_OK) goto done;
    writer->output_enc     = xmlbuf->encoding;
    writer->output_charset = xmlbuf->charset;
    set_output_buffer( writer, xmlbuf );
    writer->output_buf_user = TRUE;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) hr = E_OUTOFMEMORY;
    else write_insert_bof( writer, node );

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT flush_writer( struct writer *writer, ULONG min_size, const WS_ASYNC_CONTEXT *ctx,
                             WS_ERROR *error )
{
    WS_BYTES buf;

    if (writer->write_pos < min_size) return S_OK;

    buf.bytes  = writer->write_bufptr;
    buf.length = writer->write_pos;
    writer->output_cb( writer->output_cb_state, &buf, 1, ctx, error );
    writer->write_pos = 0;
    return S_OK;
}

/**************************************************************************
 *          WsFlushWriter		[webservices.@]
 */
HRESULT WINAPI WsFlushWriter( WS_XML_WRITER *handle, ULONG min_size, const WS_ASYNC_CONTEXT *ctx,
                              WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %lu %p %p\n", handle, min_size, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->output_type != WS_XML_WRITER_OUTPUT_TYPE_STREAM) hr = WS_E_INVALID_OPERATION;
    else hr = flush_writer( writer, min_size, ctx, error );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_grow_buffer( struct writer *writer, ULONG size )
{
    struct xmlbuf *buf = writer->output_buf;
    SIZE_T new_size;
    void *tmp;

    if (writer->output_type == WS_XML_WRITER_OUTPUT_TYPE_STREAM)
    {
        if (size > STREAM_BUFSIZE) return WS_E_QUOTA_EXCEEDED;
        return flush_writer( writer, STREAM_BUFSIZE - size, NULL, NULL );
    }

    if (buf->size >= writer->write_pos + size)
    {
        buf->bytes.length = writer->write_pos + size;
        return S_OK;
    }
    new_size = max( buf->size * 2, writer->write_pos + size );
    if (!(tmp = ws_realloc( buf->heap, buf->bytes.bytes, buf->size, new_size ))) return WS_E_QUOTA_EXCEEDED;
    writer->write_bufptr = buf->bytes.bytes = tmp;
    buf->size = new_size;
    buf->bytes.length = writer->write_pos + size;
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

struct escape
{
    char        ch;
    const char *entity;
    ULONG       len;
};
static const struct escape escape_lt = { '<', "&lt;", 4 };
static const struct escape escape_gt = { '>', "&gt;", 4 };
static const struct escape escape_amp = { '&', "&amp;", 5 };
static const struct escape escape_apos = { '\'', "&apos;", 6 };
static const struct escape escape_quot = { '"', "&quot;", 6 };

static HRESULT write_bytes_escape( struct writer *writer, const BYTE *bytes, ULONG len,
                                   const struct escape **escapes, ULONG nb_escapes )
{
    ULONG i, j, size;
    const BYTE *ptr;
    HRESULT hr;

    for (i = 0; i < len; i++)
    {
        ptr = &bytes[i];
        size = 1;
        for (j = 0; j < nb_escapes; j++)
        {
            if (bytes[i] == escapes[j]->ch)
            {
                ptr = (const BYTE *)escapes[j]->entity;
                size = escapes[j]->len;
                break;
            }
        }
        if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;
        write_bytes( writer, ptr, size );
    }

    return S_OK;
}

static HRESULT write_attribute_value_text( struct writer *writer, const WS_XML_TEXT *text, BOOL single )
{
    WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text;
    const struct escape *escapes[3];

    escapes[0] = single ? &escape_apos : &escape_quot;
    escapes[1] = &escape_lt;
    escapes[2] = &escape_amp;
    return write_bytes_escape( writer, utf8->value.bytes, utf8->value.length, escapes, 3 );
}

static HRESULT write_attribute_text( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    unsigned char quote = attr->singleQuote ? '\'' : '"';
    const WS_XML_STRING *prefix = NULL;
    ULONG size;
    HRESULT hr;

    if (attr->prefix) prefix = attr->prefix;

    /* ' prefix:attr="value"' */

    size = attr->localName->length + 4 /* ' =""' */;
    if (prefix && prefix->length) size += prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, ' ' );
    if (prefix && prefix->length)
    {
        write_bytes( writer, prefix->bytes, prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, attr->localName->bytes, attr->localName->length );
    write_char( writer, '=' );
    write_char( writer, quote );
    if (attr->value) hr = write_attribute_value_text( writer, attr->value, attr->singleQuote );
    write_char( writer, quote );

    return hr;
}

static HRESULT write_int31( struct writer *writer, ULONG len )
{
    HRESULT hr;

    if (len > 0x7fffffff) return E_INVALIDARG;

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    if (len < 0x80)
    {
        write_char( writer, len );
        return S_OK;
    }
    write_char( writer, (len & 0x7f) | 0x80 );

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    if ((len >>= 7) < 0x80)
    {
        write_char( writer, len );
        return S_OK;
    }
    write_char( writer, (len & 0x7f) | 0x80 );

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    if ((len >>= 7) < 0x80)
    {
        write_char( writer, len );
        return S_OK;
    }
    write_char( writer, (len & 0x7f) | 0x80 );

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    if ((len >>= 7) < 0x80)
    {
        write_char( writer, len );
        return S_OK;
    }
    write_char( writer, (len & 0x7f) | 0x80 );

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    if ((len >>= 7) < 0x08)
    {
        write_char( writer, len );
        return S_OK;
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT write_string( struct writer *writer, const BYTE *bytes, ULONG len )
{
    HRESULT hr;
    if ((hr = write_int31( writer, len )) != S_OK) return hr;
    if ((hr = write_grow_buffer( writer, len )) != S_OK) return hr;
    write_bytes( writer, bytes, len );
    return S_OK;
}

static HRESULT write_dict_string( struct writer *writer, ULONG id )
{
    if (id > 0x7fffffff) return E_INVALIDARG;
    return write_int31( writer, id );
}

static enum record_type get_attr_text_record_type( const WS_XML_TEXT *text, BOOL use_dict )
{
    if (!text) return RECORD_CHARS8_TEXT;
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        if (use_dict) return RECORD_DICTIONARY_TEXT;
        if (text_utf8->value.length <= MAX_UINT8) return RECORD_CHARS8_TEXT;
        if (text_utf8->value.length <= MAX_UINT16) return RECORD_CHARS16_TEXT;
        return RECORD_CHARS32_TEXT;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *text_utf16 = (const WS_XML_UTF16_TEXT *)text;
        int len = text_utf16->byteCount / sizeof(WCHAR);
        int len_utf8 = WideCharToMultiByte( CP_UTF8, 0, (const WCHAR *)text_utf16->bytes, len, NULL, 0, NULL, NULL );
        if (len_utf8 <= MAX_UINT8) return RECORD_CHARS8_TEXT;
        if (len_utf8 <= MAX_UINT16) return RECORD_CHARS16_TEXT;
        return RECORD_CHARS32_TEXT;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        if (text_base64->length <= MAX_UINT8) return RECORD_BYTES8_TEXT;
        if (text_base64->length <= MAX_UINT16) return RECORD_BYTES16_TEXT;
        return RECORD_BYTES32_TEXT;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *text_bool = (const WS_XML_BOOL_TEXT *)text;
        return text_bool->value ? RECORD_TRUE_TEXT : RECORD_FALSE_TEXT;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (!text_int32->value) return RECORD_ZERO_TEXT;
        if (text_int32->value == 1) return RECORD_ONE_TEXT;
        if (text_int32->value >= MIN_INT8 && text_int32->value <= MAX_INT8) return RECORD_INT8_TEXT;
        if (text_int32->value >= MIN_INT16 && text_int32->value <= MAX_INT16) return RECORD_INT16_TEXT;
        return RECORD_INT32_TEXT;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        if (!text_int64->value) return RECORD_ZERO_TEXT;
        if (text_int64->value == 1) return RECORD_ONE_TEXT;
        if (text_int64->value >= MIN_INT8 && text_int64->value <= MAX_INT8) return RECORD_INT8_TEXT;
        if (text_int64->value >= MIN_INT16 && text_int64->value <= MAX_INT16) return RECORD_INT16_TEXT;
        if (text_int64->value >= MIN_INT32 && text_int64->value <= MAX_INT32) return RECORD_INT32_TEXT;
        return RECORD_INT64_TEXT;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        if (!text_uint64->value) return RECORD_ZERO_TEXT;
        if (text_uint64->value == 1) return RECORD_ONE_TEXT;
        if (text_uint64->value <= MAX_INT8) return RECORD_INT8_TEXT;
        if (text_uint64->value <= MAX_INT16) return RECORD_INT16_TEXT;
        if (text_uint64->value <= MAX_INT32) return RECORD_INT32_TEXT;
        if (text_uint64->value <= MAX_INT64) return RECORD_INT64_TEXT;
        return RECORD_UINT64_TEXT;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *text_double = (const WS_XML_DOUBLE_TEXT *)text;
        if (!text_double->value) return RECORD_ZERO_TEXT;
        if (text_double->value == 1) return RECORD_ONE_TEXT;
        if (isinf( text_double->value ) || (INT64)text_double->value != text_double->value)
            return RECORD_DOUBLE_TEXT;
        if (text_double->value <= MAX_INT8) return RECORD_INT8_TEXT;
        if (text_double->value <= MAX_INT16) return RECORD_INT16_TEXT;
        if (text_double->value <= MAX_INT32) return RECORD_INT32_TEXT;
        return RECORD_INT64_TEXT;
    }
    case WS_XML_TEXT_TYPE_GUID:
        return RECORD_GUID_TEXT;

    case WS_XML_TEXT_TYPE_UNIQUE_ID:
        return RECORD_UNIQUE_ID_TEXT;

    case WS_XML_TEXT_TYPE_DATETIME:
        return RECORD_DATETIME_TEXT;

    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return 0;
    }
}

static INT64 get_text_value_int( const WS_XML_TEXT *text )
{
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        return text_int32->value;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        return text_int64->value;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        return text_uint64->value;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *text_double = (const WS_XML_DOUBLE_TEXT *)text;
        return text_double->value;
    }
    default:
        ERR( "unhandled text type %u\n", text->textType );
        assert(0);
        return 0;
    }
}

static BOOL get_string_id( struct writer *writer, const WS_XML_STRING *str, ULONG *id )
{
    if (writer->dict && str->dictionary == writer->dict)
    {
        *id = str->id << 1;
        return TRUE;
    }
    if (writer->dict_cb)
    {
        BOOL found = FALSE;
        writer->dict_cb( writer->dict_cb_state, str, &found, id, NULL );
        if (found) *id = (*id << 1) | 1;
        return found;
    }
    return FALSE;
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

static ULONG format_int32( const INT32 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%d", *ptr );
}

static ULONG format_int64( const INT64 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%I64d", *ptr );
}

static ULONG format_uint64( const UINT64 *ptr, unsigned char *buf )
{
    return wsprintfA( (char *)buf, "%I64u", *ptr );
}

static ULONG format_double( const double *ptr, unsigned char *buf )
{
    static const double precision = 0.0000000000000001;
    unsigned char *p = buf;
    double val = *ptr;
    int neg, mag, mag2 = 0, use_exp;

    if (isnan( val ))
    {
        memcpy( buf, "NaN", 3 );
        return 3;
    }
    if (isinf( val ))
    {
        if (val < 0)
        {
            memcpy( buf, "-INF", 4 );
            return 4;
        }
        memcpy( buf, "INF", 3 );
        return 3;
    }
    if (val == 0.0)
    {
        *p = '0';
        return 1;
    }

    if ((neg = val < 0))
    {
        *p++ = '-';
        val = -val;
    }

    mag = log10( val );
    use_exp = (mag >= 15 || (neg && mag >= 1) || mag <= -1);
    if (use_exp)
    {
        if (mag < 0) mag -= 1;
        val = val / pow( 10.0, mag );
        mag2 = mag;
        mag = 0;
    }
    else if (mag < 1) mag = 0;

    while (val > precision || mag >= 0)
    {
        double weight = pow( 10.0, mag );
        if (weight > 0 && !isinf( weight ))
        {
            int digit = floor( val / weight );
            val -= digit * weight;
            *(p++) = '0' + digit;
        }
        if (!mag && val > precision) *(p++) = '.';
        mag--;
    }

    if (use_exp)
    {
        int i, j;
        *(p++) = 'E';
        if (mag2 > 0) *(p++) = '+';
        else
        {
            *(p++) = '-';
            mag2 = -mag2;
        }
        mag = 0;
        while (mag2 > 0)
        {
            *(p++) = '0' + mag2 % 10;
            mag2 /= 10;
            mag++;
        }
        for (i = -mag, j = -1; i < j; i++, j--)
        {
            p[i] ^= p[j];
            p[j] ^= p[i];
            p[i] ^= p[j];
        }
    }

    return p - buf;
}

static inline int year_size( int year )
{
    return leap_year( year ) ? 366 : 365;
}

#define TZ_OFFSET 8
static ULONG format_datetime( const WS_DATETIME *ptr, unsigned char *buf )
{
    static const char fmt[] = "%04u-%02u-%02uT%02u:%02u:%02u";
    int day, hour, min, sec, sec_frac, month = 0, year = 1, tz_hour;
    unsigned __int64 ticks, day_ticks;
    ULONG len;

    if (ptr->format == WS_DATETIME_FORMAT_LOCAL &&
        ptr->ticks >= TICKS_1601_01_01 + TZ_OFFSET * TICKS_PER_HOUR)
    {
        ticks = ptr->ticks - TZ_OFFSET * TICKS_PER_HOUR;
        tz_hour = TZ_OFFSET;
    }
    else
    {
        ticks = ptr->ticks;
        tz_hour = 0;
    }
    day = ticks / TICKS_PER_DAY;
    day_ticks = ticks % TICKS_PER_DAY;
    hour = day_ticks / TICKS_PER_HOUR;
    min = (day_ticks % TICKS_PER_HOUR) / TICKS_PER_MIN;
    sec = (day_ticks % TICKS_PER_MIN) / TICKS_PER_SEC;
    sec_frac = day_ticks % TICKS_PER_SEC;

    while (day >= year_size( year ))
    {
        day -= year_size( year );
        year++;
    }
    while (day >= month_days[leap_year( year )][month])
    {
        day -= month_days[leap_year( year )][month];
        month++;
    }

    len = sprintf( (char *)buf, fmt, year, month + 1, day + 1, hour, min, sec );
    if (sec_frac)
    {
        static const char fmt_frac[] = ".%07u";
        len += sprintf( (char *)buf + len, fmt_frac, sec_frac );
        while (buf[len - 1] == '0') len--;
    }
    if (ptr->format == WS_DATETIME_FORMAT_UTC)
    {
        buf[len++] = 'Z';
    }
    else if (ptr->format == WS_DATETIME_FORMAT_LOCAL)
    {
        static const char fmt_tz[] = "%c%02u:00";
        len += sprintf( (char *)buf + len, fmt_tz, tz_hour ? '-' : '+', tz_hour );
    }

    return len;
}

static ULONG format_guid( const GUID *ptr, unsigned char *buf )
{
    static const char fmt[] = "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";
    return sprintf( (char *)buf, fmt, ptr->Data1, ptr->Data2, ptr->Data3,
                    ptr->Data4[0], ptr->Data4[1], ptr->Data4[2], ptr->Data4[3],
                    ptr->Data4[4], ptr->Data4[5], ptr->Data4[6], ptr->Data4[7] );
}

static ULONG format_urn( const GUID *ptr, unsigned char *buf )
{
    static const char fmt[] = "urn:uuid:%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x";
    return sprintf( (char *)buf, fmt, ptr->Data1, ptr->Data2, ptr->Data3,
                    ptr->Data4[0], ptr->Data4[1], ptr->Data4[2], ptr->Data4[3],
                    ptr->Data4[4], ptr->Data4[5], ptr->Data4[6], ptr->Data4[7] );
}

static ULONG format_qname( const WS_XML_STRING *prefix, const WS_XML_STRING *localname, unsigned char *buf )
{
    ULONG len = 0;
    if (prefix && prefix->length)
    {
        memcpy( buf, prefix->bytes, prefix->length );
        len += prefix->length;
        buf[len++] = ':';
    }
    memcpy( buf + len, localname->bytes, localname->length );
    return len + localname->length;
}

static ULONG encode_base64( const unsigned char *bin, ULONG len, unsigned char *buf )
{
    static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    ULONG i = 0, x;

    while (len > 0)
    {
        buf[i++] = base64[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;
        if (len == 1)
        {
            buf[i++] = base64[x];
            buf[i++] = '=';
            buf[i++] = '=';
            break;
        }
        buf[i++] = base64[x | ((bin[1] & 0xf0) >> 4)];
        x = (bin[1] & 0x0f) << 2;
        if (len == 2)
        {
            buf[i++] = base64[x];
            buf[i++] = '=';
            break;
        }
        buf[i++] = base64[x | ((bin[2] & 0xc0) >> 6)];
        buf[i++] = base64[bin[2] & 0x3f];
        bin += 3;
        len -= 3;
    }
    return i;
}

HRESULT text_to_utf8text( const WS_XML_TEXT *text, const WS_XML_UTF8_TEXT *old, ULONG *offset,
                          WS_XML_UTF8_TEXT **ret )
{
    ULONG len_old = old ? old->value.length : 0;
    if (offset) *offset = len_old;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *src = (const WS_XML_UTF8_TEXT *)text;

        if (!(*ret = alloc_utf8_text( NULL, len_old + src->value.length ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        memcpy( (*ret)->value.bytes + len_old, src->value.bytes, src->value.length );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *src = (const WS_XML_UTF16_TEXT *)text;
        const WCHAR *str = (const WCHAR *)src->bytes;
        ULONG len = src->byteCount / sizeof(WCHAR), len_utf8;

        if (src->byteCount % sizeof(WCHAR)) return E_INVALIDARG;
        len_utf8 = WideCharToMultiByte( CP_UTF8, 0, str, len, NULL, 0, NULL, NULL );
        if (!(*ret = alloc_utf8_text( NULL, len_old + len_utf8 ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        WideCharToMultiByte( CP_UTF8, 0, str, len, (char *)(*ret)->value.bytes + len_old, len_utf8, NULL, NULL );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *base64 = (const WS_XML_BASE64_TEXT *)text;
        ULONG len = ((4 * base64->length / 3) + 3) & ~3;

        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = encode_base64( base64->bytes, base64->length, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *bool_text = (const WS_XML_BOOL_TEXT *)text;

        if (!(*ret = alloc_utf8_text( NULL, len_old + 5 ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = format_bool( &bool_text->value, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *int32_text = (const WS_XML_INT32_TEXT *)text;
        unsigned char buf[12]; /* "-2147483648" */
        ULONG len = format_int32( &int32_text->value, buf );

        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        memcpy( (*ret)->value.bytes + len_old, buf, len );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *int64_text = (const WS_XML_INT64_TEXT *)text;
        unsigned char buf[21]; /* "-9223372036854775808" */
        ULONG len = format_int64( &int64_text->value, buf );

        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        memcpy( (*ret)->value.bytes + len_old, buf, len );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *uint64_text = (const WS_XML_UINT64_TEXT *)text;
        unsigned char buf[21]; /* "18446744073709551615" */
        ULONG len = format_uint64( &uint64_text->value, buf );

        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        memcpy( (*ret)->value.bytes + len_old, buf, len );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *double_text = (const WS_XML_DOUBLE_TEXT *)text;
        unsigned char buf[32]; /* "-1.1111111111111111E-308", oversized to address Valgrind limitations */
        unsigned int fpword = _control87( 0, 0 );
        ULONG len;

        _control87( _MCW_EM | _RC_NEAR | _PC_64, _MCW_EM | _MCW_RC | _MCW_PC );
        len = format_double( &double_text->value, buf );
        _control87( fpword, _MCW_EM | _MCW_RC | _MCW_PC );
        if (!len) return E_NOTIMPL;

        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        memcpy( (*ret)->value.bytes + len_old, buf, len );
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_GUID:
    {
        const WS_XML_GUID_TEXT *id = (const WS_XML_GUID_TEXT *)text;

        if (!(*ret = alloc_utf8_text( NULL, len_old + 37 ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = format_guid( &id->value, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    {
        const WS_XML_UNIQUE_ID_TEXT *id = (const WS_XML_UNIQUE_ID_TEXT *)text;

        if (!(*ret = alloc_utf8_text( NULL, len_old + 46 ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = format_urn( &id->value, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_DATETIME:
    {
        const WS_XML_DATETIME_TEXT *dt = (const WS_XML_DATETIME_TEXT *)text;

        if (!(*ret = alloc_utf8_text( NULL, len_old + 34 ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = format_datetime( &dt->value, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_QNAME:
    {
        const WS_XML_QNAME_TEXT *qn = (const WS_XML_QNAME_TEXT *)text;
        ULONG len = qn->localName->length;

        if (qn->prefix && qn->prefix->length) len += qn->prefix->length + 1;
        if (!(*ret = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (old) memcpy( (*ret)->value.bytes, old->value.bytes, len_old );
        (*ret)->value.length = format_qname( qn->prefix, qn->localName, (*ret)->value.bytes + len_old ) + len_old;
        return S_OK;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }
}

static HRESULT write_attribute_value_bin( struct writer *writer, const WS_XML_TEXT *text )
{
    enum record_type type;
    BOOL use_dict = FALSE;
    HRESULT hr;
    ULONG id;

    if (text && text->textType == WS_XML_TEXT_TYPE_UTF8)
    {
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
        use_dict = get_string_id( writer, &utf8->value, &id );
    }
    type = get_attr_text_record_type( text, use_dict );

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, type );

    switch (type)
    {
    case RECORD_CHARS8_TEXT:
    {
        const WS_XML_UTF8_TEXT *text_utf8;
        WS_XML_UTF8_TEXT *new = NULL;
        UINT8 len;

        if (!text)
        {
            if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
            write_char( writer, 0 );
            return S_OK;
        }
        if (text->textType == WS_XML_TEXT_TYPE_UTF8) text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        else
        {
            if ((hr = text_to_utf8text( text, NULL, NULL, &new )) != S_OK) return hr;
            text_utf8 = new;
        }
        len = text_utf8->value.length;
        if ((hr = write_grow_buffer( writer, sizeof(len) + len )) != S_OK)
        {
            free( new );
            return hr;
        }
        write_char( writer, len );
        write_bytes( writer, text_utf8->value.bytes, len );
        free( new );
        return S_OK;
    }
    case RECORD_CHARS16_TEXT:
    {
        const WS_XML_UTF8_TEXT *text_utf8;
        WS_XML_UTF8_TEXT *new = NULL;
        UINT16 len;

        if (text->textType == WS_XML_TEXT_TYPE_UTF8) text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        else
        {
            if ((hr = text_to_utf8text( text, NULL, NULL, &new )) != S_OK) return hr;
            text_utf8 = new;
        }
        len = text_utf8->value.length;
        if ((hr = write_grow_buffer( writer, sizeof(len) + len )) != S_OK)
        {
            free( new );
            return hr;
        }
        write_bytes( writer, (const BYTE *)&len, sizeof(len) );
        write_bytes( writer, text_utf8->value.bytes, len );
        free( new );
        return S_OK;
    }
    case RECORD_BYTES8_TEXT:
    {
        WS_XML_BASE64_TEXT *text_base64 = (WS_XML_BASE64_TEXT *)text;
        if ((hr = write_grow_buffer( writer, 1 + text_base64->length )) != S_OK) return hr;
        write_char( writer, text_base64->length );
        write_bytes( writer, text_base64->bytes, text_base64->length );
        return S_OK;
    }
    case RECORD_BYTES16_TEXT:
    {
        WS_XML_BASE64_TEXT *text_base64 = (WS_XML_BASE64_TEXT *)text;
        UINT16 len = text_base64->length;
        if ((hr = write_grow_buffer( writer, sizeof(len) + len )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&len, sizeof(len) );
        write_bytes( writer, text_base64->bytes, len );
        return S_OK;
    }
    case RECORD_ZERO_TEXT:
    case RECORD_ONE_TEXT:
    case RECORD_FALSE_TEXT:
    case RECORD_TRUE_TEXT:
        return S_OK;

    case RECORD_INT8_TEXT:
    {
        INT8 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, sizeof(val) )) != S_OK) return hr;
        write_char( writer, val );
        return S_OK;
    }
    case RECORD_INT16_TEXT:
    {
        INT16 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, sizeof(val) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_INT32_TEXT:
    {
        INT32 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, sizeof(val) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_INT64_TEXT:
    {
        INT64 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, sizeof(val) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_UINT64_TEXT:
    {
        WS_XML_UINT64_TEXT *text_uint64 = (WS_XML_UINT64_TEXT *)text;
        if ((hr = write_grow_buffer( writer, sizeof(text_uint64->value) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&text_uint64->value, sizeof(text_uint64->value) );
        return S_OK;
    }
    case RECORD_DOUBLE_TEXT:
    {
        WS_XML_DOUBLE_TEXT *text_double = (WS_XML_DOUBLE_TEXT *)text;
        if ((hr = write_grow_buffer( writer, sizeof(text_double->value) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&text_double->value, sizeof(text_double->value) );
        return S_OK;
    }
    case RECORD_GUID_TEXT:
    {
        WS_XML_GUID_TEXT *text_guid = (WS_XML_GUID_TEXT *)text;
        if ((hr = write_grow_buffer( writer, sizeof(text_guid->value) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&text_guid->value, sizeof(text_guid->value) );
        return S_OK;
    }
    case RECORD_UNIQUE_ID_TEXT:
    {
        WS_XML_UNIQUE_ID_TEXT *text_unique_id = (WS_XML_UNIQUE_ID_TEXT *)text;
        if ((hr = write_grow_buffer( writer, sizeof(text_unique_id->value) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&text_unique_id->value, sizeof(text_unique_id->value) );
        return S_OK;
    }
    case RECORD_DATETIME_TEXT:
    {
        WS_XML_DATETIME_TEXT *text_datetime = (WS_XML_DATETIME_TEXT *)text;
        UINT64 val = text_datetime->value.ticks;

        assert( val <= TICKS_MAX );
        if (text_datetime->value.format == WS_DATETIME_FORMAT_UTC) val |= (UINT64)1 << 62;
        else if (text_datetime->value.format == WS_DATETIME_FORMAT_LOCAL) val |= (UINT64)1 << 63;

        if ((hr = write_grow_buffer( writer, sizeof(val) )) != S_OK) return hr;
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    default:
        FIXME( "unhandled record type %02x\n", type );
        return E_NOTIMPL;
    }
}

static enum record_type get_attr_record_type( const WS_XML_ATTRIBUTE *attr, BOOL use_dict )
{
    if (!attr->prefix || !attr->prefix->length)
    {
        if (use_dict) return RECORD_SHORT_DICTIONARY_ATTRIBUTE;
        return RECORD_SHORT_ATTRIBUTE;
    }
    if (attr->prefix->length == 1 && attr->prefix->bytes[0] >= 'a' && attr->prefix->bytes[0] <= 'z')
    {
        if (use_dict) return RECORD_PREFIX_DICTIONARY_ATTRIBUTE_A + attr->prefix->bytes[0] - 'a';
        return RECORD_PREFIX_ATTRIBUTE_A + attr->prefix->bytes[0] - 'a';
    }
    if (use_dict) return RECORD_DICTIONARY_ATTRIBUTE;
    return RECORD_ATTRIBUTE;
};

static HRESULT write_attribute_bin( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    ULONG id;
    enum record_type type = get_attr_record_type( attr, get_string_id(writer, attr->localName, &id) );
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, type );

    if (type >= RECORD_PREFIX_ATTRIBUTE_A && type <= RECORD_PREFIX_ATTRIBUTE_Z)
    {
        if ((hr = write_string( writer, attr->localName->bytes, attr->localName->length )) != S_OK) return hr;
        return write_attribute_value_bin( writer, attr->value );
    }
    if (type >= RECORD_PREFIX_DICTIONARY_ATTRIBUTE_A && type <= RECORD_PREFIX_DICTIONARY_ATTRIBUTE_Z)
    {
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        return write_attribute_value_bin( writer, attr->value );
    }

    switch (type)
    {
    case RECORD_SHORT_ATTRIBUTE:
        if ((hr = write_string( writer, attr->localName->bytes, attr->localName->length )) != S_OK) return hr;
        break;

    case RECORD_ATTRIBUTE:
        if ((hr = write_string( writer, attr->prefix->bytes, attr->prefix->length )) != S_OK) return hr;
        if ((hr = write_string( writer, attr->localName->bytes, attr->localName->length )) != S_OK) return hr;
        break;

    case RECORD_SHORT_DICTIONARY_ATTRIBUTE:
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        break;

    case RECORD_DICTIONARY_ATTRIBUTE:
        if ((hr = write_string( writer, attr->prefix->bytes, attr->prefix->length )) != S_OK) return hr;
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        break;

    default:
        ERR( "unhandled record type %02x\n", type );
        return WS_E_NOT_SUPPORTED;
    }

    return write_attribute_value_bin( writer, attr->value );
}

static HRESULT write_attribute( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_attribute_text( writer, attr );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_attribute_bin( writer, attr );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
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
    HRESULT hr = S_OK;

    TRACE( "%p %s %d %p %p\n", handle, debugstr_xmlstr(ns), required, prefix, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !ns || !prefix) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    elem = &writer->current->hdr;
    if (elem->prefix && is_current_namespace( writer, ns ))
    {
        *prefix = elem->prefix;
        found = TRUE;
    }

    if (!found)
    {
        if (required) hr = WS_E_INVALID_FORMAT;
        else
        {
            *prefix = NULL;
            hr = S_FALSE;
        }
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_namespace_attribute_text( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    unsigned char quote = attr->singleQuote ? '\'' : '"';
    ULONG size;
    HRESULT hr;

    /* ' xmlns:prefix="namespace"' */

    size = attr->ns->length + 9 /* ' xmlns=""' */;
    if (attr->prefix && attr->prefix->length) size += attr->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_bytes( writer, (const BYTE *)" xmlns", 6 );
    if (attr->prefix && attr->prefix->length)
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

static enum record_type get_xmlns_record_type( const WS_XML_ATTRIBUTE *attr, BOOL use_dict )
{
    if (!attr->prefix || !attr->prefix->length)
    {
        if (use_dict) return RECORD_SHORT_DICTIONARY_XMLNS_ATTRIBUTE;
        return RECORD_SHORT_XMLNS_ATTRIBUTE;
    }
    if (use_dict) return RECORD_DICTIONARY_XMLNS_ATTRIBUTE;
    return RECORD_XMLNS_ATTRIBUTE;
};

static HRESULT write_namespace_attribute_bin( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    ULONG id;
    enum record_type type = get_xmlns_record_type( attr, get_string_id(writer, attr->ns, &id) );
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, type );

    switch (type)
    {
    case RECORD_SHORT_XMLNS_ATTRIBUTE:
        break;

    case RECORD_XMLNS_ATTRIBUTE:
        if ((hr = write_string( writer, attr->prefix->bytes, attr->prefix->length )) != S_OK) return hr;
        break;

    case RECORD_SHORT_DICTIONARY_XMLNS_ATTRIBUTE:
        return write_dict_string( writer, id );

    case RECORD_DICTIONARY_XMLNS_ATTRIBUTE:
        if ((hr = write_string( writer, attr->prefix->bytes, attr->prefix->length )) != S_OK) return hr;
        return write_dict_string( writer, id );

    default:
        ERR( "unhandled record type %02x\n", type );
        return WS_E_NOT_SUPPORTED;
    }

    return write_string( writer, attr->ns->bytes, attr->ns->length );
}

static HRESULT write_namespace_attribute( struct writer *writer, const WS_XML_ATTRIBUTE *attr )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_namespace_attribute_text( writer, attr );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_namespace_attribute_bin( writer, attr );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static HRESULT add_namespace_attribute( struct writer *writer, const WS_XML_STRING *prefix,
                                        const WS_XML_STRING *ns, BOOL single )
{
    WS_XML_ATTRIBUTE *attr;
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!(attr = calloc( 1, sizeof(*attr) ))) return E_OUTOFMEMORY;

    attr->singleQuote = !!single;
    attr->isXmlNs = 1;
    if (prefix && !(attr->prefix = dup_xml_string( prefix, writer->dict_do_lookup )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->ns = dup_xml_string( ns, writer->dict_do_lookup )))
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

static inline BOOL str_equal( const WS_XML_STRING *str1, const WS_XML_STRING *str2 )
{
    if (!str1 && !str2) return TRUE;
    return WsXmlStringEquals( str1, str2, NULL ) == S_OK;
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
            if (str_equal( elem->attributes[i]->prefix, prefix ) &&
                str_equal( elem->attributes[i]->ns, ns )) return TRUE;
        }
    }
    return FALSE;
}

static HRESULT set_current_namespace( struct writer *writer, const WS_XML_STRING *ns )
{
    WS_XML_STRING *str;
    if (!(str = dup_xml_string( ns, writer->dict_do_lookup ))) return E_OUTOFMEMORY;
    free_xml_string( writer->current_ns );
    writer->current_ns = str;
    return S_OK;
}

static HRESULT set_namespaces( struct writer *writer )
{
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;
    ULONG i;

    if (elem->ns->length && !namespace_in_scope( elem, elem->prefix, elem->ns ))
    {
        if ((hr = add_namespace_attribute( writer, elem->prefix, elem->ns, FALSE )) != S_OK) return hr;
        if ((hr = set_current_namespace( writer, elem->ns )) != S_OK) return hr;
    }

    for (i = 0; i < elem->attributeCount; i++)
    {
        const WS_XML_ATTRIBUTE *attr = elem->attributes[i];
        if (!attr->ns->length || namespace_in_scope( elem, attr->prefix, attr->ns )) continue;
        if ((hr = add_namespace_attribute( writer, attr->prefix, attr->ns, FALSE )) != S_OK) return hr;
    }

    return S_OK;
}

/**************************************************************************
 *          WsWriteEndAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteEndAttribute( WS_XML_WRITER *handle, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    writer->state = WRITER_STATE_STARTELEMENT;

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_attributes( struct writer *writer, const WS_XML_ELEMENT_NODE *elem )
{
    ULONG i;
    HRESULT hr;
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

static HRESULT write_startelement_text( struct writer *writer )
{
    const WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    ULONG size;
    HRESULT hr;

    /* '<prefix:localname prefix:attr="value"... xmlns:prefix="ns"'... */

    size = elem->localName->length + 1 /* '<' */;
    if (elem->prefix && elem->prefix->length) size += elem->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, '<' );
    if (elem->prefix && elem->prefix->length)
    {
        write_bytes( writer, elem->prefix->bytes, elem->prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, elem->localName->bytes, elem->localName->length );
    return write_attributes( writer, elem );
}

static enum record_type get_elem_record_type( const WS_XML_ELEMENT_NODE *elem, BOOL use_dict )
{
    if (!elem->prefix || !elem->prefix->length)
    {
        if (use_dict) return RECORD_SHORT_DICTIONARY_ELEMENT;
        return RECORD_SHORT_ELEMENT;
    }
    if (elem->prefix->length == 1 && elem->prefix->bytes[0] >= 'a' && elem->prefix->bytes[0] <= 'z')
    {
        if (use_dict) return RECORD_PREFIX_DICTIONARY_ELEMENT_A + elem->prefix->bytes[0] - 'a';
        return RECORD_PREFIX_ELEMENT_A + elem->prefix->bytes[0] - 'a';
    }
    if (use_dict) return RECORD_DICTIONARY_ELEMENT;
    return RECORD_ELEMENT;
};

static HRESULT write_startelement_bin( struct writer *writer )
{
    const WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    ULONG id;
    enum record_type type = get_elem_record_type( elem, get_string_id(writer, elem->localName, &id) );
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, type );

    if (type >= RECORD_PREFIX_ELEMENT_A && type <= RECORD_PREFIX_ELEMENT_Z)
    {
        if ((hr = write_string( writer, elem->localName->bytes, elem->localName->length )) != S_OK) return hr;
        return write_attributes( writer, elem );
    }
    if (type >= RECORD_PREFIX_DICTIONARY_ELEMENT_A && type <= RECORD_PREFIX_DICTIONARY_ELEMENT_Z)
    {
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        return write_attributes( writer, elem );
    }

    switch (type)
    {
    case RECORD_SHORT_ELEMENT:
        if ((hr = write_string( writer, elem->localName->bytes, elem->localName->length )) != S_OK) return hr;
        break;

    case RECORD_ELEMENT:
        if ((hr = write_string( writer, elem->prefix->bytes, elem->prefix->length )) != S_OK) return hr;
        if ((hr = write_string( writer, elem->localName->bytes, elem->localName->length )) != S_OK) return hr;
        break;

    case RECORD_SHORT_DICTIONARY_ELEMENT:
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        break;

    case RECORD_DICTIONARY_ELEMENT:
        if ((hr = write_string( writer, elem->prefix->bytes, elem->prefix->length )) != S_OK) return hr;
        if ((hr = write_dict_string( writer, id )) != S_OK) return hr;
        break;

    default:
        ERR( "unhandled record type %02x\n", type );
        return WS_E_NOT_SUPPORTED;
    }

    return write_attributes( writer, elem );
}

static HRESULT write_startelement( struct writer *writer )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_startelement_text( writer );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_startelement_bin( writer );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
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

static HRESULT write_endelement_text( struct writer *writer, const WS_XML_ELEMENT_NODE *elem )
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
    if (elem->prefix && elem->prefix->length) size += elem->prefix->length + 1 /* ':' */;
    if ((hr = write_grow_buffer( writer, size )) != S_OK) return hr;

    write_char( writer, '<' );
    write_char( writer, '/' );
    if (elem->prefix && elem->prefix->length)
    {
        write_bytes( writer, elem->prefix->bytes, elem->prefix->length );
        write_char( writer, ':' );
    }
    write_bytes( writer, elem->localName->bytes, elem->localName->length );
    write_char( writer, '>' );
    return S_OK;
}

static HRESULT write_endelement_bin( struct writer *writer )
{
    HRESULT hr;
    if (node_type( writer->current ) == WS_XML_NODE_TYPE_TEXT) return S_OK;
    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, RECORD_ENDELEMENT );
    return S_OK;
}

static HRESULT write_endelement( struct writer *writer, const WS_XML_ELEMENT_NODE *elem )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_endelement_text( writer, elem );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_endelement_bin( writer );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
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
        if ((hr = set_namespaces( writer )) != S_OK) return hr;
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
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    hr = write_endelement_node( writer );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_endstartelement_text( struct writer *writer )
{
    HRESULT hr;
    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, '>' );
    return S_OK;
}

static HRESULT write_endstartelement( struct writer *writer )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_endstartelement_text( writer );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return S_OK;
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
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

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state != WRITER_STATE_STARTELEMENT) hr = WS_E_INVALID_OPERATION;
    else
    {
        if ((hr = set_namespaces( writer )) != S_OK) goto done;
        if ((hr = write_startelement( writer )) != S_OK) goto done;
        if ((hr = write_endstartelement( writer )) != S_OK) goto done;
        writer->state = WRITER_STATE_ENDSTARTELEMENT;
    }

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_add_attribute( struct writer *writer, const WS_XML_STRING *prefix,
                                    const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                    BOOL single )
{
    WS_XML_ATTRIBUTE *attr;
    WS_XML_ELEMENT_NODE *elem = &writer->current->hdr;
    HRESULT hr;

    if (!(attr = calloc( 1, sizeof(*attr) ))) return E_OUTOFMEMORY;

    if (!prefix && ns->length) prefix = elem->prefix;

    attr->singleQuote = !!single;
    if (prefix && !(attr->prefix = dup_xml_string( prefix, writer->dict_do_lookup )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->localName = dup_xml_string( localname, writer->dict_do_lookup )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    if (!(attr->ns = dup_xml_string( ns, writer->dict_do_lookup )))
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

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state != WRITER_STATE_STARTELEMENT) hr = WS_E_INVALID_OPERATION;
    else if ((hr = write_add_attribute( writer, prefix, localname, ns, single )) == S_OK)
        writer->state = WRITER_STATE_STARTATTRIBUTE;

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/* flush current start element if necessary */
static HRESULT write_commit( struct writer *writer )
{
    if (writer->state == WRITER_STATE_STARTELEMENT)
    {
        HRESULT hr;
        if ((hr = set_namespaces( writer )) != S_OK) return hr;
        if ((hr = write_startelement( writer )) != S_OK) return hr;
        if ((hr = write_endstartelement( writer )) != S_OK) return hr;
        writer->state = WRITER_STATE_ENDSTARTELEMENT;
    }
    return S_OK;
}

static HRESULT write_add_cdata_node( struct writer *writer )
{
    struct node *node, *parent;
    if (!(parent = find_parent( writer ))) return WS_E_INVALID_FORMAT;
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
    if ((hr = write_commit( writer )) != S_OK) return hr;
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
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    hr = write_cdata_node( writer );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state != WRITER_STATE_TEXT) hr = WS_E_INVALID_OPERATION;
    else hr = write_endcdata_node( writer );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_add_element_node( struct writer *writer, const WS_XML_STRING *prefix,
                                       const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    struct node *node, *parent;
    WS_XML_ELEMENT_NODE *elem;

    if (!(parent = find_parent( writer ))) return WS_E_INVALID_FORMAT;

    if (!prefix && node_type( parent ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        elem = &parent->hdr;
        if (WsXmlStringEquals( ns, elem->ns, NULL ) == S_OK) prefix = elem->prefix;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) return E_OUTOFMEMORY;
    elem = &node->hdr;

    if (prefix && !(elem->prefix = dup_xml_string( prefix, writer->dict_do_lookup )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    if (!(elem->localName = dup_xml_string( localname, writer->dict_do_lookup )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    if (!(elem->ns = dup_xml_string( ns, writer->dict_do_lookup )))
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
    if ((hr = write_commit( writer )) != S_OK) return hr;
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
    HRESULT hr;

    TRACE( "%p %s %s %s %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(localname),
           debugstr_xmlstr(ns), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !localname || !ns) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    hr = write_element_node( writer, prefix, localname, ns );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

HRESULT text_to_text( const WS_XML_TEXT *text, const WS_XML_TEXT *old, ULONG *offset, WS_XML_TEXT **ret )
{
    if (offset) *offset = 0;
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
        const WS_XML_UTF8_TEXT *utf8_old = (const WS_XML_UTF8_TEXT *)old;
        WS_XML_UTF8_TEXT *new;
        ULONG len = utf8->value.length, len_old = utf8_old ? utf8_old->value.length : 0;

        if (!(new = alloc_utf8_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (utf8_old) memcpy( new->value.bytes, utf8_old->value.bytes, len_old );
        memcpy( new->value.bytes + len_old, utf8->value.bytes, len );
        if (offset) *offset = len_old;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *utf16 = (const WS_XML_UTF16_TEXT *)text;
        const WS_XML_UTF16_TEXT *utf16_old = (const WS_XML_UTF16_TEXT *)old;
        WS_XML_UTF16_TEXT *new;
        ULONG len = utf16->byteCount, len_old = utf16_old ? utf16_old->byteCount : 0;

        if (utf16->byteCount % sizeof(WCHAR)) return E_INVALIDARG;
        if (!(new = alloc_utf16_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (utf16_old) memcpy( new->bytes, utf16_old->bytes, len_old );
        memcpy( new->bytes + len_old, utf16->bytes, len );
        if (offset) *offset = len_old;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *base64 = (const WS_XML_BASE64_TEXT *)text;
        const WS_XML_BASE64_TEXT *base64_old = (const WS_XML_BASE64_TEXT *)old;
        WS_XML_BASE64_TEXT *new;
        ULONG len = base64->length, len_old = base64_old ? base64_old->length : 0;

        if (!(new = alloc_base64_text( NULL, len_old + len ))) return E_OUTOFMEMORY;
        if (base64_old) memcpy( new->bytes, base64_old->bytes, len_old );
        memcpy( new->bytes + len_old, base64->bytes, len );
        if (offset) *offset = len_old;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *bool_text = (const WS_XML_BOOL_TEXT *)text;
        WS_XML_BOOL_TEXT *new;

        if (!(new = alloc_bool_text( bool_text->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *int32_text = (const WS_XML_INT32_TEXT *)text;
        WS_XML_INT32_TEXT *new;

        if (!(new = alloc_int32_text( int32_text->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *int64_text = (const WS_XML_INT64_TEXT *)text;
        WS_XML_INT64_TEXT *new;

        if (!(new = alloc_int64_text( int64_text->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *uint64_text = (const WS_XML_UINT64_TEXT *)text;
        WS_XML_UINT64_TEXT *new;

        if (!(new = alloc_uint64_text( uint64_text->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *double_text = (const WS_XML_DOUBLE_TEXT *)text;
        WS_XML_DOUBLE_TEXT *new;

        if (!(new = alloc_double_text( double_text->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_GUID:
    {
        const WS_XML_GUID_TEXT *id = (const WS_XML_GUID_TEXT *)text;
        WS_XML_GUID_TEXT *new;

        if (!(new = alloc_guid_text( &id->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    {
        const WS_XML_UNIQUE_ID_TEXT *id = (const WS_XML_UNIQUE_ID_TEXT *)text;
        WS_XML_UNIQUE_ID_TEXT *new;

        if (!(new = alloc_unique_id_text( &id->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
        return S_OK;
    }
    case WS_XML_TEXT_TYPE_DATETIME:
    {
        const WS_XML_DATETIME_TEXT *dt = (const WS_XML_DATETIME_TEXT *)text;
        WS_XML_DATETIME_TEXT *new;

        if (!(new = alloc_datetime_text( &dt->value ))) return E_OUTOFMEMORY;
        *ret = &new->text;
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
    HRESULT hr;

    switch (value->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    case WS_XML_TEXT_TYPE_UTF16:
    case WS_XML_TEXT_TYPE_BASE64:
        break;

    case WS_XML_TEXT_TYPE_BOOL:
    case WS_XML_TEXT_TYPE_INT32:
    case WS_XML_TEXT_TYPE_INT64:
    case WS_XML_TEXT_TYPE_UINT64:
    case WS_XML_TEXT_TYPE_DOUBLE:
    case WS_XML_TEXT_TYPE_GUID:
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    case WS_XML_TEXT_TYPE_DATETIME:
        if (elem->attributes[elem->attributeCount - 1]->value) return WS_E_INVALID_OPERATION;
        break;

    default:
        FIXME( "unhandled text type %u\n", value->textType );
        return E_NOTIMPL;
    }

    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:
    {
        WS_XML_UTF8_TEXT *new, *old = (WS_XML_UTF8_TEXT *)elem->attributes[elem->attributeCount - 1]->value;
        if ((hr = text_to_utf8text( value, old, NULL, &new )) != S_OK) return hr;
        free( old );
        elem->attributes[elem->attributeCount - 1]->value = &new->text;
        break;
    }
    case WS_XML_WRITER_ENCODING_TYPE_BINARY:
    {
        WS_XML_TEXT *new, *old = elem->attributes[elem->attributeCount - 1]->value;
        if ((hr = text_to_text( value, old, NULL, &new )) != S_OK) return hr;
        free( old );
        elem->attributes[elem->attributeCount - 1]->value = new;
        break;
    }
    default:
        FIXME( "unhandled output encoding %u\n", writer->output_enc );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT write_add_text_node( struct writer *writer, const WS_XML_TEXT *value )
{
    struct node *node;
    WS_XML_TEXT_NODE *text;
    HRESULT hr;

    if (node_type( writer->current ) != WS_XML_NODE_TYPE_ELEMENT &&
        node_type( writer->current ) != WS_XML_NODE_TYPE_BOF &&
        node_type( writer->current ) != WS_XML_NODE_TYPE_CDATA) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;

    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:
    {
        WS_XML_UTF8_TEXT *new;
        if ((hr = text_to_utf8text( value, NULL, NULL, &new )) != S_OK)
        {
            free( node );
            return hr;
        }
        text->text = &new->text;
        break;
    }
    case WS_XML_WRITER_ENCODING_TYPE_BINARY:
    {
        WS_XML_TEXT *new;
        if ((hr = text_to_text( value, NULL, NULL, &new )) != S_OK)
        {
            free( node );
            return hr;
        }
        text->text = new;
        break;
    }
    default:
        FIXME( "unhandled output encoding %u\n", writer->output_enc );
        free( node );
        return E_NOTIMPL;
    }

    write_insert_node( writer, writer->current, node );
    return S_OK;
}

static HRESULT write_text_text( struct writer *writer, const WS_XML_TEXT *text, ULONG offset )
{
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
    HRESULT hr;

    if (node_type( writer->current->parent ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        const struct escape *escapes[3] = { &escape_lt, &escape_gt, &escape_amp };
        return write_bytes_escape( writer, utf8->value.bytes + offset, utf8->value.length - offset, escapes, 3 );
    }
    else if (node_type( writer->current->parent ) == WS_XML_NODE_TYPE_CDATA)
    {
        if ((hr = write_grow_buffer( writer, utf8->value.length - offset )) != S_OK) return hr;
        write_bytes( writer, utf8->value.bytes + offset, utf8->value.length - offset );
        return S_OK;
    }

    return WS_E_INVALID_FORMAT;
}

static enum record_type get_text_record_type( const WS_XML_TEXT *text, BOOL use_dict )
{
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        if (use_dict) return RECORD_DICTIONARY_TEXT_WITH_ENDELEMENT;
        if (text_utf8->value.length <= MAX_UINT8) return RECORD_CHARS8_TEXT_WITH_ENDELEMENT;
        if (text_utf8->value.length <= MAX_UINT16) return RECORD_CHARS16_TEXT_WITH_ENDELEMENT;
        return RECORD_CHARS32_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *text_utf16 = (const WS_XML_UTF16_TEXT *)text;
        int len = text_utf16->byteCount / sizeof(WCHAR);
        int len_utf8 = WideCharToMultiByte( CP_UTF8, 0, (const WCHAR *)text_utf16->bytes, len, NULL, 0, NULL, NULL );
        if (len_utf8 <= MAX_UINT8) return RECORD_CHARS8_TEXT_WITH_ENDELEMENT;
        if (len_utf8 <= MAX_UINT16) return RECORD_CHARS16_TEXT_WITH_ENDELEMENT;
        return RECORD_CHARS32_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        ULONG rem = text_base64->length % 3, len = text_base64->length - rem;
        if (len <= MAX_UINT8) return RECORD_BYTES8_TEXT;
        if (len <= MAX_UINT16) return RECORD_BYTES16_TEXT;
        return RECORD_BYTES32_TEXT;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *text_bool = (const WS_XML_BOOL_TEXT *)text;
        return text_bool->value ? RECORD_TRUE_TEXT_WITH_ENDELEMENT : RECORD_FALSE_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (!text_int32->value) return RECORD_ZERO_TEXT_WITH_ENDELEMENT;
        if (text_int32->value == 1) return RECORD_ONE_TEXT_WITH_ENDELEMENT;
        if (text_int32->value >= MIN_INT8 && text_int32->value <= MAX_INT8) return RECORD_INT8_TEXT_WITH_ENDELEMENT;
        if (text_int32->value >= MIN_INT16 && text_int32->value <= MAX_INT16) return RECORD_INT16_TEXT_WITH_ENDELEMENT;
        return RECORD_INT32_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        if (!text_int64->value) return RECORD_ZERO_TEXT_WITH_ENDELEMENT;
        if (text_int64->value == 1) return RECORD_ONE_TEXT_WITH_ENDELEMENT;
        if (text_int64->value >= MIN_INT8 && text_int64->value <= MAX_INT8) return RECORD_INT8_TEXT_WITH_ENDELEMENT;
        if (text_int64->value >= MIN_INT16 && text_int64->value <= MAX_INT16) return RECORD_INT16_TEXT_WITH_ENDELEMENT;
        if (text_int64->value >= MIN_INT32 && text_int64->value <= MAX_INT32) return RECORD_INT32_TEXT_WITH_ENDELEMENT;
        return RECORD_INT64_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        if (!text_uint64->value) return RECORD_ZERO_TEXT_WITH_ENDELEMENT;
        if (text_uint64->value == 1) return RECORD_ONE_TEXT_WITH_ENDELEMENT;
        if (text_uint64->value <= MAX_INT8) return RECORD_INT8_TEXT_WITH_ENDELEMENT;
        if (text_uint64->value <= MAX_INT16) return RECORD_INT16_TEXT_WITH_ENDELEMENT;
        if (text_uint64->value <= MAX_INT32) return RECORD_INT32_TEXT_WITH_ENDELEMENT;
        if (text_uint64->value <= MAX_INT64) return RECORD_INT64_TEXT_WITH_ENDELEMENT;
        return RECORD_UINT64_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *text_double = (const WS_XML_DOUBLE_TEXT *)text;
        if (!text_double->value) return RECORD_ZERO_TEXT_WITH_ENDELEMENT;
        if (text_double->value == 1) return RECORD_ONE_TEXT_WITH_ENDELEMENT;
        if (isinf( text_double->value ) || (INT64)text_double->value != text_double->value)
            return RECORD_DOUBLE_TEXT_WITH_ENDELEMENT;
        if (text_double->value <= MAX_INT8) return RECORD_INT8_TEXT_WITH_ENDELEMENT;
        if (text_double->value <= MAX_INT16) return RECORD_INT16_TEXT_WITH_ENDELEMENT;
        if (text_double->value <= MAX_INT32) return RECORD_INT32_TEXT_WITH_ENDELEMENT;
        return RECORD_INT64_TEXT_WITH_ENDELEMENT;
    }
    case WS_XML_TEXT_TYPE_GUID:
        return RECORD_GUID_TEXT_WITH_ENDELEMENT;

    case WS_XML_TEXT_TYPE_UNIQUE_ID:
        return RECORD_UNIQUE_ID_TEXT_WITH_ENDELEMENT;

    case WS_XML_TEXT_TYPE_DATETIME:
        return RECORD_DATETIME_TEXT_WITH_ENDELEMENT;

    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return 0;
    }
}

static HRESULT write_text_bin( struct writer *writer, const WS_XML_TEXT *text, ULONG offset )
{
    enum record_type type;
    BOOL use_dict = FALSE;
    HRESULT hr;
    ULONG id;

    if (offset)
    {
        FIXME( "no support for appending text in binary mode\n" );
        return E_NOTIMPL;
    }

    if (text->textType == WS_XML_TEXT_TYPE_UTF8)
    {
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
        use_dict = get_string_id( writer, &utf8->value, &id );
    }

    switch ((type = get_text_record_type( text, use_dict )))
    {
    case RECORD_CHARS8_TEXT_WITH_ENDELEMENT:
    {
        const WS_XML_UTF8_TEXT *text_utf8;
        WS_XML_UTF8_TEXT *new = NULL;
        UINT8 len;

        if (text->textType == WS_XML_TEXT_TYPE_UTF8) text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        else
        {
            if ((hr = text_to_utf8text( text, NULL, NULL, &new )) != S_OK) return hr;
            text_utf8 = new;
        }
        len = text_utf8->value.length;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(len) + len )) != S_OK)
        {
            free( new );
            return hr;
        }
        write_char( writer, type );
        write_char( writer, len );
        write_bytes( writer, text_utf8->value.bytes, len );
        free( new );
        return S_OK;
    }
    case RECORD_CHARS16_TEXT_WITH_ENDELEMENT:
    {
        const WS_XML_UTF8_TEXT *text_utf8;
        WS_XML_UTF8_TEXT *new = NULL;
        UINT16 len;

        if (text->textType == WS_XML_TEXT_TYPE_UTF8) text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        else
        {
            if ((hr = text_to_utf8text( text, NULL, NULL, &new )) != S_OK) return hr;
            text_utf8 = new;
        }
        len = text_utf8->value.length;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(len) + len )) != S_OK)
        {
            free( new );
            return hr;
        }
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&len, sizeof(len) );
        write_bytes( writer, text_utf8->value.bytes, len );
        free( new );
        return S_OK;
    }
    case RECORD_BYTES8_TEXT:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        UINT8 rem = text_base64->length % 3, len = text_base64->length - rem;

        if (len)
        {
            if ((hr = write_grow_buffer( writer, 1 + sizeof(len) + len )) != S_OK) return hr;
            write_char( writer, rem ? RECORD_BYTES8_TEXT : RECORD_BYTES8_TEXT_WITH_ENDELEMENT );
            write_char( writer, len );
            write_bytes( writer, text_base64->bytes, len );
        }
        if (rem)
        {
            if ((hr = write_grow_buffer( writer, 3 )) != S_OK) return hr;
            write_char( writer, RECORD_BYTES8_TEXT_WITH_ENDELEMENT );
            write_char( writer, rem );
            write_bytes( writer, (const BYTE *)text_base64->bytes + len, rem );
        }
        return S_OK;
    }
    case RECORD_BYTES16_TEXT:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        UINT16 rem = text_base64->length % 3, len = text_base64->length - rem;

        if (len)
        {
            if ((hr = write_grow_buffer( writer, 1 + sizeof(len) + len )) != S_OK) return hr;
            write_char( writer, rem ? RECORD_BYTES16_TEXT : RECORD_BYTES16_TEXT_WITH_ENDELEMENT );
            write_bytes( writer, (const BYTE *)&len, sizeof(len) );
            write_bytes( writer, text_base64->bytes, len );
        }
        if (rem)
        {
            if ((hr = write_grow_buffer( writer, 3 )) != S_OK) return hr;
            write_char( writer, RECORD_BYTES8_TEXT_WITH_ENDELEMENT );
            write_char( writer, rem );
            write_bytes( writer, (const BYTE *)text_base64->bytes + len, rem );
        }
        return S_OK;
    }
    case RECORD_BYTES32_TEXT:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        UINT32 rem = text_base64->length % 3, len = text_base64->length - rem;
        if (len)
        {
            if ((hr = write_grow_buffer( writer, 1 + sizeof(len) + len )) != S_OK) return hr;
            write_char( writer, rem ? RECORD_BYTES32_TEXT : RECORD_BYTES32_TEXT_WITH_ENDELEMENT );
            write_bytes( writer, (const BYTE *)&len, sizeof(len) );
            write_bytes( writer, text_base64->bytes, len );
        }
        if (rem)
        {
            if ((hr = write_grow_buffer( writer, 3 )) != S_OK) return hr;
            write_char( writer, RECORD_BYTES8_TEXT_WITH_ENDELEMENT );
            write_char( writer, rem );
            write_bytes( writer, (const BYTE *)text_base64->bytes + len, rem );
        }
        return S_OK;
    }
    case RECORD_ZERO_TEXT_WITH_ENDELEMENT:
    case RECORD_ONE_TEXT_WITH_ENDELEMENT:
    case RECORD_FALSE_TEXT_WITH_ENDELEMENT:
    case RECORD_TRUE_TEXT_WITH_ENDELEMENT:
    {
        if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
        write_char( writer, type );
        return S_OK;
    }
    case RECORD_INT8_TEXT_WITH_ENDELEMENT:
    {
        INT8 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, 1 + sizeof(val) )) != S_OK) return hr;
        write_char( writer, type );
        write_char( writer, val );
        return S_OK;
    }
    case RECORD_INT16_TEXT_WITH_ENDELEMENT:
    {
        INT16 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, 1 + sizeof(val) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_INT32_TEXT_WITH_ENDELEMENT:
    {
        INT32 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, 1 + sizeof(val) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_INT64_TEXT_WITH_ENDELEMENT:
    {
        INT64 val = get_text_value_int( text );
        if ((hr = write_grow_buffer( writer, 1 + sizeof(val) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_UINT64_TEXT_WITH_ENDELEMENT:
    {
        WS_XML_UINT64_TEXT *text_uint64 = (WS_XML_UINT64_TEXT *)text;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(text_uint64->value) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&text_uint64->value, sizeof(text_uint64->value) );
        return S_OK;
    }
    case RECORD_DOUBLE_TEXT_WITH_ENDELEMENT:
    {
        WS_XML_DOUBLE_TEXT *text_double = (WS_XML_DOUBLE_TEXT *)text;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(text_double->value) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&text_double->value, sizeof(text_double->value) );
        return S_OK;
    }
    case RECORD_GUID_TEXT_WITH_ENDELEMENT:
    {
        WS_XML_GUID_TEXT *text_guid = (WS_XML_GUID_TEXT *)text;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(text_guid->value) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&text_guid->value, sizeof(text_guid->value) );
        return S_OK;
    }
    case RECORD_UNIQUE_ID_TEXT_WITH_ENDELEMENT:
    {
        WS_XML_UNIQUE_ID_TEXT *text_unique_id = (WS_XML_UNIQUE_ID_TEXT *)text;
        if ((hr = write_grow_buffer( writer, 1 + sizeof(text_unique_id->value) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&text_unique_id->value, sizeof(text_unique_id->value) );
        return S_OK;
    }
    case RECORD_DATETIME_TEXT_WITH_ENDELEMENT:
    {
        WS_XML_DATETIME_TEXT *text_datetime = (WS_XML_DATETIME_TEXT *)text;
        UINT64 val = text_datetime->value.ticks;

        assert( val <= TICKS_MAX );
        if (text_datetime->value.format == WS_DATETIME_FORMAT_UTC) val |= (UINT64)1 << 62;
        else if (text_datetime->value.format == WS_DATETIME_FORMAT_LOCAL) val |= (UINT64)1 << 63;

        if ((hr = write_grow_buffer( writer, 1 + sizeof(val) )) != S_OK) return hr;
        write_char( writer, type );
        write_bytes( writer, (const BYTE *)&val, sizeof(val) );
        return S_OK;
    }
    case RECORD_DICTIONARY_TEXT_WITH_ENDELEMENT:
    {
        if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
        write_char( writer, type );
        return write_dict_string( writer, id );
    }
    default:
        FIXME( "unhandled record type %02x\n", type );
        return E_NOTIMPL;
    }
}

static HRESULT write_text( struct writer *writer, const WS_XML_TEXT *text, ULONG offset )
{
    if (!writer->current->parent) return WS_E_INVALID_FORMAT;

    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_text_text( writer, text, offset );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_text_bin( writer, text, offset );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static HRESULT write_text_node( struct writer *writer, const WS_XML_TEXT *text )
{
    WS_XML_TEXT_NODE *node = (WS_XML_TEXT_NODE *)writer->current;
    ULONG offset = 0;
    HRESULT hr;

    if ((hr = write_commit( writer )) != S_OK) return hr;
    if (node_type( writer->current ) != WS_XML_NODE_TYPE_TEXT)
    {
        if ((hr = write_add_text_node( writer, text )) != S_OK) return hr;
        node = (WS_XML_TEXT_NODE *)writer->current;
    }
    else
    {
        switch (writer->output_enc)
        {
        case WS_XML_WRITER_ENCODING_TYPE_TEXT:
        {
            WS_XML_UTF8_TEXT *new, *old = (WS_XML_UTF8_TEXT *)node->text;
            offset = old->value.length;
            if ((hr = text_to_utf8text( text, old, &offset, &new )) != S_OK) return hr;
            free( old );
            node->text = &new->text;
            break;
        }
        case WS_XML_WRITER_ENCODING_TYPE_BINARY:
        {
            WS_XML_TEXT *new, *old = node->text;
            if ((hr = text_to_text( text, old, &offset, &new )) != S_OK) return hr;
            free( old );
            node->text = new;
            break;
        }
        default:
            FIXME( "unhandled output encoding %u\n", writer->output_enc );
            return E_NOTIMPL;
        }
    }

    if ((hr = write_text( writer, node->text, offset )) != S_OK) return hr;

    writer->state = WRITER_STATE_TEXT;
    return S_OK;
}

/**************************************************************************
 *          WsWriteText		[webservices.@]
 */
HRESULT WINAPI WsWriteText( WS_XML_WRITER *handle, const WS_XML_TEXT *text, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, text, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !text) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state == WRITER_STATE_STARTATTRIBUTE) hr = write_set_attribute_value( writer, text );
    else hr = write_text_node( writer, text );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteBytes		[webservices.@]
 */
HRESULT WINAPI WsWriteBytes( WS_XML_WRITER *handle, const void *bytes, ULONG count, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_XML_BASE64_TEXT base64;
    HRESULT hr;

    TRACE( "%p %p %lu %p\n", handle, bytes, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else
    {
        base64.text.textType = WS_XML_TEXT_TYPE_BASE64;
        base64.bytes         = (BYTE *)bytes;
        base64.length        = count;

        if (writer->state == WRITER_STATE_STARTATTRIBUTE) hr = write_set_attribute_value( writer, &base64.text );
        else hr = write_text_node( writer, &base64.text );
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteChars		[webservices.@]
 */
HRESULT WINAPI WsWriteChars( WS_XML_WRITER *handle, const WCHAR *chars, ULONG count, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_XML_UTF16_TEXT utf16;
    HRESULT hr;

    TRACE( "%p %s %lu %p\n", handle, debugstr_wn(chars, count), count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else
    {
        utf16.text.textType = WS_XML_TEXT_TYPE_UTF16;
        utf16.bytes         = (BYTE *)chars;
        utf16.byteCount     = count * sizeof(WCHAR);

        if (writer->state == WRITER_STATE_STARTATTRIBUTE) hr = write_set_attribute_value( writer, &utf16.text );
        else hr = write_text_node( writer, &utf16.text );
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteCharsUtf8		[webservices.@]
 */
HRESULT WINAPI WsWriteCharsUtf8( WS_XML_WRITER *handle, const BYTE *bytes, ULONG count, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_XML_UTF8_TEXT utf8;
    HRESULT hr;

    TRACE( "%p %s %lu %p\n", handle, debugstr_an((const char *)bytes, count), count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else
    {
        utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
        utf8.value.bytes   = (BYTE *)bytes;
        utf8.value.length  = count;

        if (writer->state == WRITER_STATE_STARTATTRIBUTE) hr = write_set_attribute_value( writer, &utf8.text );
        else hr = write_text_node( writer, &utf8.text );
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

static HRESULT write_add_nil_attribute( struct writer *writer )
{
    static const WS_XML_STRING prefix = {1, (BYTE *)"a"};
    static const WS_XML_STRING localname = {3, (BYTE *)"nil"};
    static const WS_XML_STRING ns = {41, (BYTE *)"http://www.w3.org/2001/XMLSchema-instance"};
    static const WS_XML_UTF8_TEXT value = {{WS_XML_TEXT_TYPE_UTF8}, {4, (BYTE *)"true"}};
    HRESULT hr;

    if ((hr = write_add_attribute( writer, &prefix, &localname, &ns, FALSE )) != S_OK) return hr;
    if ((hr = write_set_attribute_value( writer, &value.text )) != S_OK) return hr;
    return add_namespace_attribute( writer, &prefix, &ns, FALSE );
}

static HRESULT get_value_ptr( WS_WRITE_OPTION option, const void *value, ULONG size, ULONG expected_size,
                              const void **ptr )
{
    switch (option)
    {
    case WS_WRITE_REQUIRED_VALUE:
    case WS_WRITE_NILLABLE_VALUE:
        if (!value || size != expected_size) return E_INVALIDARG;
        *ptr = value;
        return S_OK;

    case WS_WRITE_REQUIRED_POINTER:
        if (size != sizeof(const void *) || !(*ptr = *(const void **)value)) return E_INVALIDARG;
        return S_OK;

    case WS_WRITE_NILLABLE_POINTER:
        if (size != sizeof(const void *)) return E_INVALIDARG;
        *ptr = *(const void **)value;
        return S_OK;

    default:
        return E_INVALIDARG;
    }
}

static HRESULT write_type_bool( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_BOOL_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                const BOOL *value, ULONG size )
{
    WS_XML_BOOL_TEXT text_bool;
    const BOOL *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(BOOL), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_bool.text.textType = WS_XML_TEXT_TYPE_BOOL;
    text_bool.value         = *ptr;
    return write_type_text( writer, mapping, &text_bool.text );
}

static HRESULT write_type_int8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_INT8_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                const BOOL *value, ULONG size )
{
    WS_XML_INT32_TEXT text_int32;
    const INT8 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(INT8), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_int32.text.textType = WS_XML_TEXT_TYPE_INT32;
    text_int32.value         = *ptr;
    return write_type_text( writer, mapping, &text_int32.text );
}

static HRESULT write_type_int16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT16_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const BOOL *value, ULONG size )
{
    WS_XML_INT32_TEXT text_int32;
    const INT16 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(INT16), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_int32.text.textType = WS_XML_TEXT_TYPE_INT32;
    text_int32.value         = *ptr;
    return write_type_text( writer, mapping, &text_int32.text );
}

static HRESULT write_type_int32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT32_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    WS_XML_INT32_TEXT text_int32;
    const INT32 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(INT32), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_int32.text.textType = WS_XML_TEXT_TYPE_INT32;
    text_int32.value         = *ptr;
    return write_type_text( writer, mapping, &text_int32.text );
}

static HRESULT write_type_int64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_INT64_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    WS_XML_INT64_TEXT text_int64;
    const INT64 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(INT64), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_int64.text.textType = WS_XML_TEXT_TYPE_INT64;
    text_int64.value         = *ptr;
    return write_type_text( writer, mapping, &text_int64.text );
}

static HRESULT write_type_uint8( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_UINT8_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    WS_XML_UINT64_TEXT text_uint64;
    const UINT8 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(UINT8), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_uint64.text.textType = WS_XML_TEXT_TYPE_UINT64;
    text_uint64.value         = *ptr;
    return write_type_text( writer, mapping, &text_uint64.text );
}

static HRESULT write_type_uint16( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT16_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    WS_XML_UINT64_TEXT text_uint64;
    const UINT16 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(UINT16), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_uint64.text.textType = WS_XML_TEXT_TYPE_UINT64;
    text_uint64.value         = *ptr;
    return write_type_text( writer, mapping, &text_uint64.text );
}

static HRESULT write_type_uint32( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT32_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    WS_XML_UINT64_TEXT text_uint64;
    const UINT32 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(UINT32), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_uint64.text.textType = WS_XML_TEXT_TYPE_UINT64;
    text_uint64.value         = *ptr;
    return write_type_text( writer, mapping, &text_uint64.text );
}

static HRESULT write_type_uint64( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_UINT64_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    WS_XML_UINT64_TEXT text_uint64;
    const UINT64 *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(UINT64), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_uint64.text.textType = WS_XML_TEXT_TYPE_UINT64;
    text_uint64.value         = *ptr;
    return write_type_text( writer, mapping, &text_uint64.text );
}

static HRESULT write_type_double( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_DOUBLE_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    WS_XML_DOUBLE_TEXT text_double;
    const double *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(double), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_double.text.textType = WS_XML_TEXT_TYPE_DOUBLE;
    text_double.value         = *ptr;
    return write_type_text( writer, mapping, &text_double.text );
}

static HRESULT write_type_datetime( struct writer *writer, WS_TYPE_MAPPING mapping,
                                    const WS_DATETIME_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                    const void *value, ULONG size )
{
    WS_XML_DATETIME_TEXT text_datetime;
    const WS_DATETIME *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(WS_DATETIME), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );
    if (ptr->ticks > TICKS_MAX || ptr->format > WS_DATETIME_FORMAT_NONE) return WS_E_INVALID_FORMAT;

    text_datetime.text.textType = WS_XML_TEXT_TYPE_DATETIME;
    text_datetime.value         = *ptr;
    return write_type_text( writer, mapping, &text_datetime.text );
}

static HRESULT write_type_guid( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_GUID_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                const void *value, ULONG size )
{
    WS_XML_GUID_TEXT text_guid;
    const GUID *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(GUID), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    text_guid.text.textType = WS_XML_TEXT_TYPE_GUID;
    text_guid.value         = *ptr;
    return write_type_text( writer, mapping, &text_guid.text );
}

static HRESULT write_type_unique_id( struct writer *writer, WS_TYPE_MAPPING mapping,
                                     const WS_UNIQUE_ID_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                     const void *value, ULONG size )
{
    WS_XML_UNIQUE_ID_TEXT text_unique_id;
    WS_XML_UTF16_TEXT text_utf16;
    const WS_UNIQUE_ID *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(*ptr), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    if (ptr->uri.length)
    {
        text_utf16.text.textType = WS_XML_TEXT_TYPE_UTF16;
        text_utf16.bytes         = (BYTE *)ptr->uri.chars;
        text_utf16.byteCount     = ptr->uri.length * sizeof(WCHAR);
        return write_type_text( writer, mapping, &text_utf16.text );
    }

    text_unique_id.text.textType = WS_XML_TEXT_TYPE_UNIQUE_ID;
    text_unique_id.value         = ptr->guid;
    return write_type_text( writer, mapping, &text_unique_id.text );
}

static HRESULT write_type_string( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_STRING_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    WS_XML_UTF16_TEXT utf16;
    const WS_STRING *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(WS_STRING), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );
    if (!ptr->length) return S_OK;

    utf16.text.textType = WS_XML_TEXT_TYPE_UTF16;
    utf16.bytes         = (BYTE *)ptr->chars;
    utf16.byteCount     = ptr->length * sizeof(WCHAR);
    return write_type_text( writer, mapping, &utf16.text );
}

static HRESULT write_type_wsz( struct writer *writer, WS_TYPE_MAPPING mapping,
                               const WS_WSZ_DESCRIPTION *desc, WS_WRITE_OPTION option,
                               const void *value, ULONG size )
{
    WS_XML_UTF16_TEXT utf16;
    const WCHAR *ptr;
    HRESULT hr;
    int len;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option || option == WS_WRITE_REQUIRED_VALUE || option == WS_WRITE_NILLABLE_VALUE) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, 0, (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );
    if (!(len = lstrlenW( ptr ))) return S_OK;

    utf16.text.textType = WS_XML_TEXT_TYPE_UTF16;
    utf16.bytes         = (BYTE *)ptr;
    utf16.byteCount     = len * sizeof(WCHAR);
    return write_type_text( writer, mapping, &utf16.text );
}

static HRESULT write_type_bytes( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_BYTES_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    WS_XML_BASE64_TEXT base64;
    const WS_BYTES *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(WS_BYTES), (const void **)&ptr )) != S_OK) return hr;
    if ((option == WS_WRITE_NILLABLE_VALUE && is_nil_value( value, size )) ||
        (option == WS_WRITE_NILLABLE_POINTER && !ptr)) return write_add_nil_attribute( writer );
    if (!ptr->length) return S_OK;

    base64.text.textType = WS_XML_TEXT_TYPE_BASE64;
    base64.bytes         = ptr->bytes;
    base64.length        = ptr->length;
    return write_type_text( writer, mapping, &base64.text );
}

static HRESULT write_type_xml_string( struct writer *writer, WS_TYPE_MAPPING mapping,
                                      const WS_XML_STRING_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                      const void *value, ULONG size )
{
    WS_XML_UTF8_TEXT utf8;
    const WS_XML_STRING *ptr;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(WS_XML_STRING), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );
    if (option == WS_WRITE_NILLABLE_VALUE && is_nil_value( value, size )) return write_add_nil_attribute( writer );
    if (!ptr->length) return S_OK;

    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = ptr->bytes;
    utf8.value.length  = ptr->length;
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT find_prefix( struct writer *writer, const WS_XML_STRING *ns, const WS_XML_STRING **prefix )
{
    const struct node *node;
    for (node = writer->current; node_type( node ) == WS_XML_NODE_TYPE_ELEMENT; node = node->parent)
    {
        const WS_XML_ELEMENT_NODE *elem = &node->hdr;
        ULONG i;
        for (i = 0; i < elem->attributeCount; i++)
        {
            if (!elem->attributes[i]->isXmlNs) continue;
            if (WsXmlStringEquals( elem->attributes[i]->ns, ns, NULL ) != S_OK) continue;
            *prefix = elem->attributes[i]->prefix;
            return S_OK;
        }
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT write_type_qname( struct writer *writer, WS_TYPE_MAPPING mapping,
                                 const WS_XML_QNAME_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    WS_XML_QNAME_TEXT qname;
    const WS_XML_QNAME *ptr;
    const WS_XML_STRING *prefix;
    HRESULT hr;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }

    if (!option) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(*ptr), (const void **)&ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );
    if (option == WS_WRITE_NILLABLE_VALUE && is_nil_value( value, size )) return write_add_nil_attribute( writer );

    if (((hr = find_prefix( writer, &ptr->ns, &prefix )) != S_OK)) return hr;

    qname.text.textType = WS_XML_TEXT_TYPE_QNAME;
    qname.prefix        = (WS_XML_STRING *)prefix;
    qname.localName     = (WS_XML_STRING *)&ptr->localName;
    qname.ns            = (WS_XML_STRING *)&ptr->ns;
    return write_type_text( writer, mapping, &qname.text );
}

static WS_WRITE_OPTION get_field_write_option( WS_TYPE type, ULONG options )
{
    if (options & WS_FIELD_POINTER)
    {
        if (options & (WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM)) return WS_WRITE_NILLABLE_POINTER;
        return WS_WRITE_REQUIRED_POINTER;
    }

    switch (type)
    {
    case WS_BOOL_TYPE:
    case WS_INT8_TYPE:
    case WS_INT16_TYPE:
    case WS_INT32_TYPE:
    case WS_INT64_TYPE:
    case WS_UINT8_TYPE:
    case WS_UINT16_TYPE:
    case WS_UINT32_TYPE:
    case WS_UINT64_TYPE:
    case WS_DOUBLE_TYPE:
    case WS_DATETIME_TYPE:
    case WS_GUID_TYPE:
    case WS_UNIQUE_ID_TYPE:
    case WS_STRING_TYPE:
    case WS_BYTES_TYPE:
    case WS_XML_STRING_TYPE:
    case WS_XML_QNAME_TYPE:
    case WS_STRUCT_TYPE:
    case WS_ENUM_TYPE:
    case WS_UNION_TYPE:
        if (options & (WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE)) return WS_WRITE_NILLABLE_VALUE;
        return WS_WRITE_REQUIRED_VALUE;

    case WS_WSZ_TYPE:
    case WS_DESCRIPTION_TYPE:
        if (options & (WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE)) return WS_WRITE_NILLABLE_POINTER;
        return WS_WRITE_REQUIRED_POINTER;

    default:
        FIXME( "unhandled type %u\n", type );
        return 0;
    }
}

static HRESULT find_index( const WS_UNION_DESCRIPTION *desc, int value, ULONG *idx )
{
    ULONG i;

    if (desc->valueIndices)
    {
        int c, min = 0, max = desc->fieldCount - 1;
        while (min <= max)
        {
            i = (min + max) / 2;
            c = value - desc->fields[desc->valueIndices[i]]->value;
            if (c < 0)
                max = i - 1;
            else if (c > 0)
                min = i + 1;
            else
            {
                *idx = desc->valueIndices[i];
                return S_OK;
            }
        }
        return WS_E_INVALID_FORMAT;
    }

    /* fall back to linear search */
    for (i = 0; i < desc->fieldCount; i++)
    {
        if (desc->fields[i]->value == value)
        {
            *idx = i;
            return S_OK;
        }
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT write_type_field( struct writer *, const WS_FIELD_DESCRIPTION *, const char *, ULONG );

static HRESULT write_type_union( struct writer *writer, const WS_UNION_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                 const void *value, ULONG size )
{
    ULONG i;
    const void *ptr;
    int enum_value;
    HRESULT hr;

    if (size < sizeof(enum_value)) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, desc->size, &ptr )) != S_OK) return hr;

    enum_value = *(int *)(char *)ptr + desc->enumOffset;
    if (enum_value == desc->noneEnumValue && option == WS_WRITE_NILLABLE_VALUE) return S_OK;

    if ((hr = find_index( desc, enum_value, &i )) != S_OK) return hr;
    return write_type_field( writer, &desc->fields[i]->field, ptr, desc->fields[i]->field.offset );
}

static HRESULT write_type( struct writer *, WS_TYPE_MAPPING, WS_TYPE, const void *, WS_WRITE_OPTION,
                           const void *, ULONG );

static HRESULT write_type_array( struct writer *writer, const WS_FIELD_DESCRIPTION *desc, const char *buf,
                                 ULONG count )
{
    HRESULT hr = S_OK;
    ULONG i, size, offset = 0;
    WS_WRITE_OPTION option;

    if (!(option = get_field_write_option( desc->type, desc->options ))) return E_INVALIDARG;

    /* wrapper element */
    if (desc->localName && ((hr = write_element_node( writer, NULL, desc->localName, desc->ns )) != S_OK))
        return hr;

    if (option == WS_WRITE_REQUIRED_VALUE || option == WS_WRITE_NILLABLE_VALUE)
        size = get_type_size( desc->type, desc->typeDescription );
    else
        size = sizeof(const void *);

    for (i = 0; i < count; i++)
    {
        if (desc->type == WS_UNION_TYPE)
        {
            if ((hr = write_type_union( writer, desc->typeDescription, option, buf + offset, size )) != S_OK)
                return hr;
        }
        else
        {
            if ((hr = write_element_node( writer, NULL, desc->itemLocalName, desc->itemNs )) != S_OK) return hr;
            if ((hr = write_type( writer, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription, option,
                                  buf + offset, size )) != S_OK) return hr;
            if ((hr = write_endelement_node( writer )) != S_OK) return hr;
        }
        offset += size;
    }

    if (desc->localName) hr = write_endelement_node( writer );
    return hr;
}

static HRESULT write_type_field( struct writer *writer, const WS_FIELD_DESCRIPTION *desc, const char *buf,
                                 ULONG offset )
{
    HRESULT hr;
    WS_TYPE_MAPPING mapping;
    WS_WRITE_OPTION option;
    ULONG count, size, field_options = desc->options;
    const char *ptr = buf + offset;

    if (field_options & ~(WS_FIELD_POINTER|WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM))
    {
        FIXME( "options %#lx not supported\n", desc->options );
        return E_NOTIMPL;
    }

    /* zero-terminated strings and descriptions are always pointers */
    if (desc->type == WS_WSZ_TYPE || desc->type == WS_DESCRIPTION_TYPE) field_options |= WS_FIELD_POINTER;

    if (field_options & WS_FIELD_POINTER)
        size = sizeof(const void *);
    else
        size = get_type_size( desc->type, desc->typeDescription );

    if (is_nil_value( ptr, size ))
    {
        if (field_options & WS_FIELD_OPTIONAL) return S_OK;
        if (field_options & (WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM))
        {
            if (field_options & WS_FIELD_POINTER) option = WS_WRITE_NILLABLE_POINTER;
            else option = WS_WRITE_NILLABLE_VALUE;
        }
        else
        {
            if (field_options & WS_FIELD_POINTER) option = WS_WRITE_REQUIRED_POINTER;
            else option = WS_WRITE_REQUIRED_VALUE;
        }
    }
    else
    {
        if (field_options & WS_FIELD_POINTER) option = WS_WRITE_REQUIRED_POINTER;
        else option = WS_WRITE_REQUIRED_VALUE;
    }

    switch (desc->mapping)
    {
    case WS_TYPE_ATTRIBUTE_FIELD_MAPPING:
        mapping = WS_ATTRIBUTE_TYPE_MAPPING;
        break;

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

    case WS_ELEMENT_CHOICE_FIELD_MAPPING:
        if (desc->type != WS_UNION_TYPE || !desc->typeDescription) return E_INVALIDARG;
        option = (field_options & WS_FIELD_OPTIONAL) ? WS_WRITE_NILLABLE_VALUE : WS_WRITE_REQUIRED_VALUE;
        return write_type_union( writer, desc->typeDescription, option, ptr, size );

    case WS_REPEATING_ELEMENT_FIELD_MAPPING:
    case WS_REPEATING_ELEMENT_CHOICE_FIELD_MAPPING:
        count = *(const ULONG *)(buf + desc->countOffset);
        return write_type_array( writer, desc, *(const char **)ptr, count );

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

    case WS_ANY_ATTRIBUTES_FIELD_MAPPING:
        return S_OK;

    default:
        FIXME( "field mapping %u not supported\n", desc->mapping );
        return E_NOTIMPL;
    }

    if ((hr = write_type( writer, mapping, desc->type, desc->typeDescription, option, ptr, size )) != S_OK)
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

static HRESULT write_type_struct( struct writer *writer, WS_TYPE_MAPPING mapping,
                                  const WS_STRUCT_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                  const void *value, ULONG size )
{
    ULONG i, offset;
    const void *ptr;
    HRESULT hr;

    if (!desc) return E_INVALIDARG;
    if (desc->structOptions) FIXME( "struct options %#lx not supported\n", desc->structOptions );

    if ((hr = get_value_ptr( option, value, size, desc->size, &ptr )) != S_OK) return hr;
    if (option == WS_WRITE_NILLABLE_POINTER && !ptr) return write_add_nil_attribute( writer );

    for (i = 0; i < desc->fieldCount; i++)
    {
        offset = desc->fields[i]->offset;
        if ((hr = write_type_field( writer, desc->fields[i], ptr, offset )) != S_OK) return hr;
    }

    return S_OK;
}

static const WS_XML_STRING *get_enum_value_name( const WS_ENUM_DESCRIPTION *desc, int value )
{
    ULONG i;
    for (i = 0; i < desc->valueCount; i++)
    {
        if (desc->values[i].value == value) return desc->values[i].name;
    }
    return NULL;
}

static HRESULT write_type_enum( struct writer *writer, WS_TYPE_MAPPING mapping,
                                const WS_ENUM_DESCRIPTION *desc, WS_WRITE_OPTION option,
                                const void *value, ULONG size )
{
    const WS_XML_STRING *name;
    WS_XML_UTF8_TEXT utf8;
    const int *ptr;
    HRESULT hr;

    if (!desc) return E_INVALIDARG;
    if ((hr = get_value_ptr( option, value, size, sizeof(*ptr), (const void **)&ptr )) != S_OK) return hr;
    if (!(name = get_enum_value_name( desc, *ptr ))) return E_INVALIDARG;

    utf8.text.textType = WS_XML_TEXT_TYPE_UTF8;
    utf8.value.bytes   = name->bytes;
    utf8.value.length  = name->length;
    return write_type_text( writer, mapping, &utf8.text );
}

static HRESULT write_type_description( struct writer *writer, WS_TYPE_MAPPING mapping,
                                       WS_WRITE_OPTION option, const void *value, ULONG size )
{
    const WS_STRUCT_DESCRIPTION *ptr;
    HRESULT hr;

    if ((hr = get_value_ptr( option, value, size, sizeof(*ptr), (const void **)&ptr )) != S_OK) return hr;
    if (ptr) FIXME( "ignoring type description %p\n", ptr );
    return S_OK;
}

static HRESULT write_type( struct writer *writer, WS_TYPE_MAPPING mapping, WS_TYPE type,
                           const void *desc, WS_WRITE_OPTION option, const void *value,
                           ULONG size )
{
    switch (type)
    {
    case WS_BOOL_TYPE:
        return write_type_bool( writer, mapping, desc, option, value, size );

    case WS_INT8_TYPE:
        return write_type_int8( writer, mapping, desc, option, value, size );

    case WS_INT16_TYPE:
        return write_type_int16( writer, mapping, desc, option, value, size );

    case WS_INT32_TYPE:
        return write_type_int32( writer, mapping, desc, option, value, size );

    case WS_INT64_TYPE:
        return write_type_int64( writer, mapping, desc, option, value, size );

    case WS_UINT8_TYPE:
        return write_type_uint8( writer, mapping, desc, option, value, size );

    case WS_UINT16_TYPE:
        return write_type_uint16( writer, mapping, desc, option, value, size );

    case WS_UINT32_TYPE:
        return write_type_uint32( writer, mapping, desc, option, value, size );

    case WS_UINT64_TYPE:
        return write_type_uint64( writer, mapping, desc, option, value, size );

    case WS_DOUBLE_TYPE:
        return write_type_double( writer, mapping, desc, option, value, size );

    case WS_DATETIME_TYPE:
        return write_type_datetime( writer, mapping, desc, option, value, size );

    case WS_GUID_TYPE:
        return write_type_guid( writer, mapping, desc, option, value, size );

    case WS_UNIQUE_ID_TYPE:
        return write_type_unique_id( writer, mapping, desc, option, value, size );

    case WS_STRING_TYPE:
        return write_type_string( writer, mapping, desc, option, value, size );

    case WS_WSZ_TYPE:
        return write_type_wsz( writer, mapping, desc, option, value, size );

    case WS_BYTES_TYPE:
        return write_type_bytes( writer, mapping, desc, option, value, size );

    case WS_XML_STRING_TYPE:
        return write_type_xml_string( writer, mapping, desc, option, value, size );

    case WS_XML_QNAME_TYPE:
        return write_type_qname( writer, mapping, desc, option, value, size );

    case WS_DESCRIPTION_TYPE:
        return write_type_description( writer, mapping, option, value, size );

    case WS_STRUCT_TYPE:
        return write_type_struct( writer, mapping, desc, option, value, size );

    case WS_ENUM_TYPE:
        return write_type_enum( writer, mapping, desc, option, value, size );

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

    TRACE( "%p %p %u %p %lu %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !desc || !desc->attributeLocalName || !desc->attributeNs || !value)
        return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state != WRITER_STATE_STARTELEMENT) hr = WS_E_INVALID_OPERATION;
    else if ((hr = write_add_attribute( writer, NULL, desc->attributeLocalName, desc->attributeNs, FALSE )) == S_OK)
    {
        writer->state = WRITER_STATE_STARTATTRIBUTE;
        hr = write_type( writer, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->typeDescription, option, value, size );
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    TRACE( "%p %p %u %p %lu %p\n", handle, desc, option, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !desc || !desc->elementLocalName || !desc->elementNs || !value)
        return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if ((hr = write_element_node( writer, NULL, desc->elementLocalName, desc->elementNs )) != S_OK) goto done;

    if ((hr = write_type( writer, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, desc->typeDescription,
                          option, value, size )) != S_OK) goto done;

    hr = write_endelement_node( writer );

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    TRACE( "%p %u %u %p %u %p %lu %p\n", handle, mapping, type, desc, option, value,
           size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !value) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    switch (mapping)
    {
    case WS_ATTRIBUTE_TYPE_MAPPING:
        if (writer->state != WRITER_STATE_STARTATTRIBUTE) hr = WS_E_INVALID_FORMAT;
        else hr = write_type( writer, mapping, type, desc, option, value, size );
        break;

    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
    case WS_ANY_ELEMENT_TYPE_MAPPING:
        hr = write_type( writer, mapping, type, desc, option, value, size );
        break;

    default:
        FIXME( "mapping %u not implemented\n", mapping );
        hr = E_NOTIMPL;
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
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
    HRESULT hr = S_OK;
    WS_TYPE type;

    TRACE( "%p %u %p %lu %p\n", handle, value_type, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !value || (type = map_value_type( value_type )) == ~0u) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    switch (writer->state)
    {
    case WRITER_STATE_STARTATTRIBUTE:
        mapping = WS_ATTRIBUTE_TYPE_MAPPING;
        break;

    case WRITER_STATE_STARTELEMENT:
        mapping = WS_ELEMENT_TYPE_MAPPING;
        break;

    default:
        hr = WS_E_INVALID_FORMAT;
    }

    if (hr == S_OK) hr = write_type( writer, mapping, type, NULL, WS_WRITE_REQUIRED_VALUE, value, size );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteArray		[webservices.@]
 */
HRESULT WINAPI WsWriteArray( WS_XML_WRITER *handle, const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                             WS_VALUE_TYPE value_type, const void *array, ULONG size, ULONG offset,
                             ULONG count, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    WS_TYPE type;
    ULONG type_size, i;
    HRESULT hr = S_OK;

    TRACE( "%p %s %s %u %p %lu %lu %lu %p\n", handle, debugstr_xmlstr(localname), debugstr_xmlstr(ns),
           value_type, array, size, offset, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }

    if (!localname || !ns || (type = map_value_type( value_type )) == ~0u)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    type_size = get_type_size( type, NULL );
    if (size % type_size || (offset + count) * type_size > size || (count && !array))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    for (i = offset; i < count; i++)
    {
        const char *ptr = (const char *)array + (offset + i) * type_size;
        if ((hr = write_element_node( writer, NULL, localname, ns )) != S_OK) goto done;
        if ((hr = write_type( writer, WS_ELEMENT_TYPE_MAPPING, type, NULL, WS_WRITE_REQUIRED_POINTER,
                              &ptr, sizeof(ptr) )) != S_OK) goto done;
        if ((hr = write_endelement_node( writer )) != S_OK) goto done;
    }

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (xmlbuf->encoding != writer->output_enc || xmlbuf->charset != writer->output_charset)
    {
        FIXME( "no support for different encoding and/or charset\n" );
        hr = E_NOTIMPL;
        goto done;
    }

    if ((hr = write_commit( writer )) != S_OK) goto done;
    if ((hr = write_grow_buffer( writer, xmlbuf->bytes.length )) != S_OK) goto done;
    write_bytes( writer, xmlbuf->bytes.bytes, xmlbuf->bytes.length );

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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
    HRESULT hr = S_OK;
    char *buf;
    ULONG i;

    TRACE( "%p %p %p %p %lu %p %p %p %p\n", handle, buffer, encoding, properties, count, heap,
           bytes, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !xmlbuf || !heap || !bytes) return E_INVALIDARG;

    if (encoding && encoding->encodingType != WS_XML_WRITER_ENCODING_TYPE_TEXT)
    {
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        return E_NOTIMPL;
    }

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( writer->prop, writer->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) goto done;
    }

    if (!(buf = ws_alloc( heap, xmlbuf->bytes.length ))) hr = WS_E_QUOTA_EXCEEDED;
    else
    {
        memcpy( buf, xmlbuf->bytes.bytes, xmlbuf->bytes.length );
        *bytes = buf;
        *size = xmlbuf->bytes.length;
    }

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsWriteXmlnsAttribute		[webservices.@]
 */
HRESULT WINAPI WsWriteXmlnsAttribute( WS_XML_WRITER *handle, const WS_XML_STRING *prefix,
                                      const WS_XML_STRING *ns, BOOL single, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %s %s %d %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(ns),
           single, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !ns) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->state != WRITER_STATE_STARTELEMENT) hr = WS_E_INVALID_OPERATION;
    else if (!namespace_in_scope( &writer->current->hdr, prefix, ns ))
        hr = add_namespace_attribute( writer, prefix, ns, single );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_qualified_name( struct writer *writer, const WS_XML_STRING *prefix,
                                     const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    WS_XML_QNAME_TEXT qname = {{WS_XML_TEXT_TYPE_QNAME}};
    HRESULT hr;

    if ((hr = write_commit( writer )) != S_OK) return hr;
    if (!prefix && ((hr = find_prefix( writer, ns, &prefix )) != S_OK)) return hr;

    qname.prefix    = (WS_XML_STRING *)prefix;
    qname.localName = (WS_XML_STRING *)localname;
    qname.ns        = (WS_XML_STRING *)ns;

    if ((hr = write_add_text_node( writer, &qname.text )) != S_OK) return hr;
    return write_text( writer, ((const WS_XML_TEXT_NODE *)writer->current)->text, 0 );
}

/**************************************************************************
 *          WsWriteQualifiedName		[webservices.@]
 */
HRESULT WINAPI WsWriteQualifiedName( WS_XML_WRITER *handle, const WS_XML_STRING *prefix,
                                     const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                     WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr;

    TRACE( "%p %s %s %s %p\n", handle, debugstr_xmlstr(prefix), debugstr_xmlstr(localname),
           debugstr_xmlstr(ns), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else if (writer->state != WRITER_STATE_STARTELEMENT) hr = WS_E_INVALID_FORMAT;
    else if (!localname || (!prefix && !ns)) hr = E_INVALIDARG;
    else hr = write_qualified_name( writer, prefix, localname, ns );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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
    HRESULT hr;

    TRACE( "%p %u %p %p\n", handle, move, found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (writer->output_type != WS_XML_WRITER_OUTPUT_TYPE_BUFFER) hr = WS_E_INVALID_OPERATION;
    else hr = write_move_to( writer, move, found );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetWriterPosition		[webservices.@]
 */
HRESULT WINAPI WsGetWriterPosition( WS_XML_WRITER *handle, WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !pos) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else
    {
        pos->buffer = (WS_XML_BUFFER *)writer->output_buf;
        pos->node   = writer->current;
    }

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetWriterPosition		[webservices.@]
 */
HRESULT WINAPI WsSetWriterPosition( WS_XML_WRITER *handle, const WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct writer *writer = (struct writer *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !pos) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC || (struct xmlbuf *)pos->buffer != writer->output_buf)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else writer->current = pos->node;

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_add_comment_node( struct writer *writer, const WS_XML_STRING *value )
{
    struct node *node, *parent;
    WS_XML_COMMENT_NODE *comment;

    if (!(parent = find_parent( writer ))) return WS_E_INVALID_FORMAT;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;

    if (value->length && !(comment->value.bytes = malloc( value->length )))
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    memcpy( comment->value.bytes, value->bytes, value->length );
    comment->value.length = value->length;

    write_insert_node( writer, parent, node );
    return S_OK;
}

static HRESULT write_comment_text( struct writer *writer )
{
    const WS_XML_COMMENT_NODE *comment = (const WS_XML_COMMENT_NODE *)writer->current;
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, comment->value.length + 7 )) != S_OK) return hr;
    write_bytes( writer, (const BYTE *)"<!--", 4 );
    write_bytes( writer, comment->value.bytes, comment->value.length );
    write_bytes( writer, (const BYTE *)"-->", 3 );
    return S_OK;
}

static HRESULT write_comment_bin( struct writer *writer )
{
    const WS_XML_COMMENT_NODE *comment = (const WS_XML_COMMENT_NODE *)writer->current;
    HRESULT hr;

    if ((hr = write_grow_buffer( writer, 1 )) != S_OK) return hr;
    write_char( writer, RECORD_COMMENT );
    return write_string( writer, comment->value.bytes, comment->value.length );
}

static HRESULT write_comment( struct writer *writer )
{
    switch (writer->output_enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_TEXT:   return write_comment_text( writer );
    case WS_XML_WRITER_ENCODING_TYPE_BINARY: return write_comment_bin( writer );
    default:
        ERR( "unhandled encoding %u\n", writer->output_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static HRESULT write_comment_node( struct writer *writer, const WS_XML_STRING *value )
{
    HRESULT hr;
    if ((hr = write_commit( writer )) != S_OK) return hr;
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
        const WS_XML_STRING *prefix = attrs[i]->prefix;
        const WS_XML_STRING *localname = attrs[i]->localName;
        const WS_XML_STRING *ns = attrs[i]->ns;
        BOOL single = attrs[i]->singleQuote;

        if (attrs[i]->isXmlNs)
        {
            if ((hr = add_namespace_attribute( writer, prefix, ns, single )) != S_OK) return hr;
        }
        else
        {
            if ((hr = write_add_attribute( writer, prefix, localname, ns, single )) != S_OK) return hr;
            if ((hr = write_set_attribute_value( writer, attrs[i]->value )) != S_OK) return hr;
        }
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
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, node, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer || !node) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!writer->output_type) hr = WS_E_INVALID_OPERATION;
    else hr = write_node( writer, node );

    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
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
        if ((hr = write_text( writer, ((const WS_XML_TEXT_NODE *)writer->current)->text, 0 )) != S_OK) return hr;
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
    struct node *parent, *current, *node = NULL;
    HRESULT hr;

    TRACE( "%p %p %p\n", handle, reader, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!writer) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if (!(parent = find_parent( writer ))) hr = WS_E_INVALID_FORMAT;
    else
    {
        if ((hr = copy_node( reader, writer->output_enc, &node )) != S_OK) goto done;
        current = writer->current;
        write_insert_node( writer, parent, node );

        write_rewind( writer );
        if ((hr = write_tree( writer )) != S_OK) goto done;
        writer->current = current;

        WsMoveReader( reader, WS_MOVE_TO_NEXT_NODE, NULL, NULL );
    }

done:
    LeaveCriticalSection( &writer->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT write_param( struct writer *writer, const WS_FIELD_DESCRIPTION *desc, const void *value )
{
    return write_type_field( writer, desc, value, 0 );
}

static ULONG get_array_len( const WS_PARAMETER_DESCRIPTION *params, ULONG count, ULONG index, const void **args )
{
    ULONG i, ret = 0;
    for (i = 0; i < count; i++)
    {
        if (params[i].inputMessageIndex != index || params[i].parameterType != WS_PARAMETER_TYPE_ARRAY_COUNT)
            continue;
        if (params[i].outputMessageIndex != INVALID_PARAMETER_INDEX)
            ret = **(const ULONG **)args[i];
        else
            ret = *(const ULONG *)args[i];
        break;
    }
    return ret;
}

static HRESULT write_param_array( struct writer *writer, const WS_FIELD_DESCRIPTION *desc, const void *value,
                                  ULONG len )
{
    return write_type_array( writer, desc, value, len );
}

HRESULT write_input_params( WS_XML_WRITER *handle, const WS_ELEMENT_DESCRIPTION *desc,
                            const WS_PARAMETER_DESCRIPTION *params, ULONG count, const void **args )
{
    struct writer *writer = (struct writer *)handle;
    const WS_STRUCT_DESCRIPTION *desc_struct;
    const WS_FIELD_DESCRIPTION *desc_field;
    HRESULT hr;
    ULONG i;

    if (desc->type != WS_STRUCT_TYPE || !(desc_struct = desc->typeDescription)) return E_INVALIDARG;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    if ((hr = write_element_node( writer, NULL, desc->elementLocalName, desc->elementNs )) != S_OK) goto done;

    for (i = 0; i < count; i++)
    {
        if (params[i].inputMessageIndex == INVALID_PARAMETER_INDEX) continue;
        if (params[i].parameterType == WS_PARAMETER_TYPE_MESSAGES)
        {
            FIXME( "messages type not supported\n" );
            hr = E_NOTIMPL;
            goto done;
        }
        if ((hr = get_param_desc( desc_struct, params[i].inputMessageIndex, &desc_field )) != S_OK) goto done;
        if (params[i].parameterType == WS_PARAMETER_TYPE_NORMAL)
        {
            const void *ptr;
            if (params[i].outputMessageIndex != INVALID_PARAMETER_INDEX)
                ptr = *(const void **)args[i];
            else
                ptr = args[i];
            if ((hr = write_param( writer, desc_field, ptr )) != S_OK) goto done;
        }
        else if (params[i].parameterType == WS_PARAMETER_TYPE_ARRAY)
        {
            const void *ptr;
            ULONG len;
            if (params[i].outputMessageIndex != INVALID_PARAMETER_INDEX)
                ptr = **(const void ***)args[i];
            else
                ptr = *(const void **)args[i];
            len = get_array_len( params, count, params[i].inputMessageIndex, args );
            if ((hr = write_param_array( writer, desc_field, ptr, len )) != S_OK) goto done;
        }
    }

    hr = write_endelement_node( writer );

done:
    LeaveCriticalSection( &writer->cs );
    return hr;
}

HRESULT writer_set_lookup( WS_XML_WRITER *handle, BOOL enable )
{
    struct writer *writer = (struct writer *)handle;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    writer->dict_do_lookup = enable;

    LeaveCriticalSection( &writer->cs );
    return S_OK;
}

HRESULT writer_set_dict_callback( WS_XML_WRITER *handle, WS_DYNAMIC_STRING_CALLBACK cb, void *state )
{
    struct writer *writer = (struct writer *)handle;

    EnterCriticalSection( &writer->cs );

    if (writer->magic != WRITER_MAGIC)
    {
        LeaveCriticalSection( &writer->cs );
        return E_INVALIDARG;
    }

    writer->dict_cb        = cb;
    writer->dict_cb_state  = state;

    LeaveCriticalSection( &writer->cs );
    return S_OK;
}
