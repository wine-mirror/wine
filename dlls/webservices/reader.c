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
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

ULONG prop_size( const struct prop_desc *desc, ULONG count )
{
    ULONG i, ret = count * sizeof(struct prop);
    for (i = 0; i < count; i++) ret += desc[i].size;
    return ret;
}

void prop_init( const struct prop_desc *desc, ULONG count, struct prop *prop, void *data )
{
    ULONG i;
    char *ptr = data;
    for (i = 0; i < count; i++)
    {
        prop[i].value     = ptr;
        prop[i].size      = desc[i].size;
        prop[i].readonly  = desc[i].readonly;
        prop[i].writeonly = desc[i].writeonly;
        ptr += prop[i].size;
    }
}

HRESULT prop_set( const struct prop *prop, ULONG count, ULONG id, const void *value, ULONG size )
{
    if (id >= count || size != prop[id].size || prop[id].readonly) return E_INVALIDARG;
    memcpy( prop[id].value, value, size );
    return S_OK;
}

HRESULT prop_get( const struct prop *prop, ULONG count, ULONG id, void *buf, ULONG size )
{
    if (id >= count || size != prop[id].size || prop[id].writeonly) return E_INVALIDARG;
    memcpy( buf, prop[id].value, prop[id].size );
    return S_OK;
}

struct node *alloc_node( WS_XML_NODE_TYPE type )
{
    struct node *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    ret->hdr.node.nodeType = type;
    list_init( &ret->entry );
    list_init( &ret->children );
    return ret;
}

void free_attribute( WS_XML_ATTRIBUTE *attr )
{
    if (!attr) return;
    free_xml_string( attr->prefix );
    free_xml_string( attr->localName );
    free_xml_string( attr->ns );
    free( attr->value );
    free( attr );
}

void free_node( struct node *node )
{
    if (!node) return;
    switch (node_type( node ))
    {
    case WS_XML_NODE_TYPE_ELEMENT:
    {
        WS_XML_ELEMENT_NODE *elem = &node->hdr;
        ULONG i;

        for (i = 0; i < elem->attributeCount; i++) free_attribute( elem->attributes[i] );
        free( elem->attributes );
        free_xml_string( elem->prefix );
        free_xml_string( elem->localName );
        free_xml_string( elem->ns );
        break;
    }
    case WS_XML_NODE_TYPE_TEXT:
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        free( text->text );
        break;
    }
    case WS_XML_NODE_TYPE_COMMENT:
    {
        WS_XML_COMMENT_NODE *comment = (WS_XML_COMMENT_NODE *)node;
        free( comment->value.bytes );
        break;
    }
    case WS_XML_NODE_TYPE_CDATA:
    case WS_XML_NODE_TYPE_END_CDATA:
    case WS_XML_NODE_TYPE_END_ELEMENT:
    case WS_XML_NODE_TYPE_EOF:
    case WS_XML_NODE_TYPE_BOF:
        break;

    default:
        ERR( "unhandled type %u\n", node_type( node ) );
        break;
    }
    free( node );
}

void destroy_nodes( struct node *node )
{
    struct list *ptr;

    if (!node) return;
    while ((ptr = list_head( &node->children )))
    {
        struct node *child = LIST_ENTRY( ptr, struct node, entry );
        list_remove( &child->entry );
        destroy_nodes( child );
    }
    free_node( node );
}

static WS_XML_ATTRIBUTE *dup_attribute( const WS_XML_ATTRIBUTE *src, WS_XML_WRITER_ENCODING_TYPE enc )
{
    WS_XML_ATTRIBUTE *dst;
    HRESULT hr;

    if (!(dst = calloc( 1, sizeof(*dst) ))) return NULL;
    dst->singleQuote = src->singleQuote;
    dst->isXmlNs     = src->isXmlNs;

    if (src->prefix && !(dst->prefix = dup_xml_string( src->prefix, FALSE ))) goto error;
    if (src->localName && !(dst->localName = dup_xml_string( src->localName, FALSE ))) goto error;
    if (src->ns && !(dst->ns = dup_xml_string( src->ns, FALSE ))) goto error;

    if (src->value)
    {
        switch (enc)
        {
        case WS_XML_WRITER_ENCODING_TYPE_BINARY:
            if ((hr = text_to_text( src->value, NULL, NULL, &dst->value )) != S_OK) goto error;
            break;

        case WS_XML_WRITER_ENCODING_TYPE_TEXT:
            if ((hr = text_to_utf8text( src->value, NULL, NULL, (WS_XML_UTF8_TEXT **)&dst->value )) != S_OK)
                goto error;
            break;

        default:
            ERR( "unhandled encoding %u\n", enc );
            goto error;
        }
    }

    return dst;

error:
    free_attribute( dst );
    return NULL;
}

static WS_XML_ATTRIBUTE **dup_attributes( WS_XML_ATTRIBUTE * const *src, ULONG count,
                                          WS_XML_WRITER_ENCODING_TYPE enc )
{
    WS_XML_ATTRIBUTE **dst;
    ULONG i;

    if (!(dst = malloc( sizeof(*dst) * count ))) return NULL;
    for (i = 0; i < count; i++)
    {
        if (!(dst[i] = dup_attribute( src[i], enc )))
        {
            for (; i > 0; i--) free_attribute( dst[i - 1] );
            free( dst );
            return NULL;
        }
    }
    return dst;
}

static struct node *dup_element_node( const WS_XML_ELEMENT_NODE *src, WS_XML_WRITER_ENCODING_TYPE enc )
{
    struct node *node;
    WS_XML_ELEMENT_NODE *dst;
    ULONG count = src->attributeCount;
    WS_XML_ATTRIBUTE **attrs = src->attributes;
    const WS_XML_STRING *prefix = (src->prefix && src->prefix->length) ? src->prefix : NULL;
    const WS_XML_STRING *localname = src->localName;
    const WS_XML_STRING *ns = src->ns;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) return NULL;
    dst = &node->hdr;

    if (count && !(dst->attributes = dup_attributes( attrs, count, enc ))) goto error;
    dst->attributeCount = count;

    if (prefix && !(dst->prefix = dup_xml_string( prefix, FALSE ))) goto error;
    if (localname && !(dst->localName = dup_xml_string( localname, FALSE ))) goto error;
    if (ns && !(dst->ns = dup_xml_string( ns, FALSE ))) goto error;
    return node;

error:
    free_node( node );
    return NULL;
}

static struct node *dup_text_node( const WS_XML_TEXT_NODE *src, WS_XML_WRITER_ENCODING_TYPE enc )
{
    struct node *node;
    WS_XML_TEXT_NODE *dst;
    HRESULT hr;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    dst = (WS_XML_TEXT_NODE *)node;
    if (!src->text) return node;

    switch (enc)
    {
    case WS_XML_WRITER_ENCODING_TYPE_BINARY:
        hr = text_to_text( src->text, NULL, NULL, &dst->text );
        break;

    case WS_XML_WRITER_ENCODING_TYPE_TEXT:
        hr = text_to_utf8text( src->text, NULL, NULL, (WS_XML_UTF8_TEXT **)&dst->text );
        break;

    default:
        ERR( "unhandled encoding %u\n", enc );
        free_node( node );
        return NULL;
    }

    if (hr != S_OK)
    {
        free_node( node );
        return NULL;
    }

    return node;
}

static struct node *dup_comment_node( const WS_XML_COMMENT_NODE *src )
{
    struct node *node;
    WS_XML_COMMENT_NODE *dst;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return NULL;
    dst = (WS_XML_COMMENT_NODE *)node;

    if (src->value.length && !(dst->value.bytes = malloc( src->value.length )))
    {
        free_node( node );
        return NULL;
    }
    memcpy( dst->value.bytes, src->value.bytes, src->value.length );
    dst->value.length = src->value.length;
    return node;
}

static struct node *dup_node( const struct node *src, WS_XML_WRITER_ENCODING_TYPE enc )
{
    switch (node_type( src ))
    {
    case WS_XML_NODE_TYPE_ELEMENT:
        return dup_element_node( &src->hdr, enc );

    case WS_XML_NODE_TYPE_TEXT:
        return dup_text_node( (const WS_XML_TEXT_NODE *)src, enc );

    case WS_XML_NODE_TYPE_COMMENT:
        return dup_comment_node( (const WS_XML_COMMENT_NODE *)src );

    case WS_XML_NODE_TYPE_CDATA:
    case WS_XML_NODE_TYPE_END_CDATA:
    case WS_XML_NODE_TYPE_END_ELEMENT:
    case WS_XML_NODE_TYPE_EOF:
    case WS_XML_NODE_TYPE_BOF:
        return alloc_node( node_type( src ) );

    default:
        ERR( "unhandled type %u\n", node_type( src ) );
        break;
    }
    return NULL;
}

static HRESULT dup_tree( const struct node *src, WS_XML_WRITER_ENCODING_TYPE enc, struct node **dst )
{
    struct node *parent;
    const struct node *child;

    if (!*dst && !(*dst = dup_node( src, enc ))) return E_OUTOFMEMORY;
    parent = *dst;

    LIST_FOR_EACH_ENTRY( child, &src->children, struct node, entry )
    {
        HRESULT hr = E_OUTOFMEMORY;
        struct node *new_child;

        if (!(new_child = dup_node( child, enc )) || (hr = dup_tree( child, enc, &new_child )) != S_OK)
        {
            destroy_nodes( *dst );
            return hr;
        }
        new_child->parent = parent;
        list_add_tail( &parent->children, &new_child->entry );
    }
    return S_OK;
}

static const struct prop_desc reader_props[] =
{
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_MAX_DEPTH */
    { sizeof(BOOL), FALSE },       /* WS_XML_READER_PROPERTY_ALLOW_FRAGMENT */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_MAX_ATTRIBUTES */
    { sizeof(BOOL), FALSE },       /* WS_XML_READER_PROPERTY_READ_DECLARATION */
    { sizeof(WS_CHARSET), FALSE }, /* WS_XML_READER_PROPERTY_CHARSET */
    { sizeof(ULONGLONG), TRUE },   /* WS_XML_READER_PROPERTY_ROW */
    { sizeof(ULONGLONG), TRUE },   /* WS_XML_READER_PROPERTY_COLUMN */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_UTF8_TRIM_SIZE */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_STREAM_BUFFER_SIZE */
    { sizeof(BOOL), TRUE },        /* WS_XML_READER_PROPERTY_IN_ATTRIBUTE */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_STREAM_MAX_ROOT_MIME_PART_SIZE */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_STREAM_MAX_MIME_HEADERS_SIZE */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_MAX_MIME_PARTS */
    { sizeof(BOOL), FALSE },       /* WS_XML_READER_PROPERTY_ALLOW_INVALID_CHARACTER_REFERENCES */
    { sizeof(ULONG), FALSE },      /* WS_XML_READER_PROPERTY_MAX_NAMESPACES */
};

enum reader_state
{
    READER_STATE_INITIAL,
    READER_STATE_BOF,
    READER_STATE_STARTELEMENT,
    READER_STATE_STARTATTRIBUTE,
    READER_STATE_STARTCDATA,
    READER_STATE_CDATA,
    READER_STATE_TEXT,
    READER_STATE_ENDELEMENT,
    READER_STATE_ENDCDATA,
    READER_STATE_COMMENT,
    READER_STATE_EOF
};

struct prefix
{
    WS_XML_STRING *str;
    WS_XML_STRING *ns;
};

struct reader
{
    ULONG                        magic;
    CRITICAL_SECTION             cs;
    ULONG                        read_size;
    ULONG                        read_pos;
    const unsigned char         *read_bufptr;
    enum reader_state            state;
    struct node                 *root;
    struct node                 *current;
    ULONG                        current_attr;
    struct node                 *last;
    struct prefix               *prefixes;
    ULONG                        nb_prefixes;
    ULONG                        nb_prefixes_allocated;
    WS_XML_READER_ENCODING_TYPE  input_enc;
    WS_CHARSET                   input_charset;
    WS_XML_READER_INPUT_TYPE     input_type;
    WS_READ_CALLBACK             input_cb;
    void                        *input_cb_state;
    struct xmlbuf               *input_buf;
    unsigned char               *input_conv;
    ULONG                        input_size;
    ULONG                        text_conv_offset;
    unsigned char               *stream_buf;
    const WS_XML_DICTIONARY     *dict_static;
    WS_XML_DICTIONARY           *dict;
    ULONG                        prop_count;
    struct prop                  prop[ARRAY_SIZE( reader_props )];
};

#define READER_MAGIC (('R' << 24) | ('E' << 16) | ('A' << 8) | 'D')

static struct reader *alloc_reader(void)
{
    static const ULONG count = ARRAY_SIZE( reader_props );
    struct reader *ret;
    ULONG size = sizeof(*ret) + prop_size( reader_props, count );

    if (!(ret = calloc( 1, size ))) return NULL;
    if (!(ret->prefixes = calloc( 1, sizeof(*ret->prefixes) )))
    {
        free( ret );
        return NULL;
    }
    ret->nb_prefixes = ret->nb_prefixes_allocated = 1;

    ret->magic       = READER_MAGIC;
    InitializeCriticalSection( &ret->cs );
    ret->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": reader.cs");

    prop_init( reader_props, count, ret->prop, &ret[1] );
    ret->prop_count  = count;
    return ret;
}

static void clear_prefixes( struct prefix *prefixes, ULONG count )
{
    ULONG i;
    for (i = 0; i < count; i++)
    {
        free_xml_string( prefixes[i].str );
        prefixes[i].str = NULL;
        free_xml_string( prefixes[i].ns );
        prefixes[i].ns  = NULL;
    }
}

static HRESULT set_prefix( struct prefix *prefix, const WS_XML_STRING *str, const WS_XML_STRING *ns )
{
    if (str)
    {
        free_xml_string( prefix->str );
        if (!(prefix->str = dup_xml_string( str, FALSE ))) return E_OUTOFMEMORY;
    }
    if (prefix->ns) free_xml_string( prefix->ns );
    if (!(prefix->ns = dup_xml_string( ns, FALSE ))) return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT bind_prefix( struct reader *reader, const WS_XML_STRING *prefix, const WS_XML_STRING *ns )
{
    ULONG i;
    HRESULT hr;

    for (i = 0; i < reader->nb_prefixes; i++)
    {
        if (WsXmlStringEquals( prefix, reader->prefixes[i].str, NULL ) == S_OK)
            return set_prefix( &reader->prefixes[i], NULL, ns );
    }
    if (i >= reader->nb_prefixes_allocated)
    {
        ULONG new_size = reader->nb_prefixes_allocated * 2;
        struct prefix *tmp = realloc( reader->prefixes, new_size * sizeof(*tmp)  );
        if (!tmp) return E_OUTOFMEMORY;
        memset( tmp + reader->nb_prefixes_allocated, 0, (new_size - reader->nb_prefixes_allocated) * sizeof(*tmp) );
        reader->prefixes = tmp;
        reader->nb_prefixes_allocated = new_size;
    }
    if ((hr = set_prefix( &reader->prefixes[i], prefix, ns )) != S_OK) return hr;
    reader->nb_prefixes++;
    return S_OK;
}

static const WS_XML_STRING *get_namespace( struct reader *reader, const WS_XML_STRING *prefix )
{
    ULONG i;
    for (i = 0; i < reader->nb_prefixes; i++)
    {
        if (WsXmlStringEquals( prefix, reader->prefixes[i].str, NULL ) == S_OK)
            return reader->prefixes[i].ns;
    }
    return NULL;
}

static void read_insert_eof( struct reader *reader, struct node *eof )
{
    if (!reader->root) reader->root = eof;
    else
    {
        eof->parent = reader->root;
        list_add_tail( &reader->root->children, &eof->entry );
    }
    reader->current = reader->last = eof;
}

static void read_insert_bof( struct reader *reader, struct node *bof )
{
    reader->root->parent = bof;
    list_add_tail( &bof->children, &reader->root->entry );
    reader->current = reader->last = reader->root = bof;
}

static void read_insert_node( struct reader *reader, struct node *parent, struct node *node )
{
    node->parent = parent;
    list_add_before( list_tail( &parent->children ), &node->entry );
    reader->current = reader->last = node;
}

static void free_reader( struct reader *reader )
{
    destroy_nodes( reader->root );
    clear_prefixes( reader->prefixes, reader->nb_prefixes );
    free( reader->prefixes );
    free( reader->stream_buf );
    free( reader->input_conv );

    reader->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &reader->cs );
    free( reader );
}

static HRESULT init_reader( struct reader *reader )
{
    static const WS_XML_STRING empty = {0, NULL};
    struct node *node;
    HRESULT hr;

    reader->state        = READER_STATE_INITIAL;
    destroy_nodes( reader->root );
    reader->root         = reader->current = NULL;
    reader->current_attr = 0;
    clear_prefixes( reader->prefixes, reader->nb_prefixes );
    reader->nb_prefixes  = 1;
    if ((hr = bind_prefix( reader, &empty, &empty )) != S_OK) return hr;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_EOF ))) return E_OUTOFMEMORY;
    read_insert_eof( reader, node );
    reader->input_enc     = WS_XML_READER_ENCODING_TYPE_TEXT;
    reader->input_charset = WS_CHARSET_UTF8;
    reader->dict_static   = NULL;
    reader->dict          = NULL;
    return S_OK;
}

/**************************************************************************
 *          WsCreateReader		[webservices.@]
 */
HRESULT WINAPI WsCreateReader( const WS_XML_READER_PROPERTY *properties, ULONG count,
                               WS_XML_READER **handle, WS_ERROR *error )
{
    struct reader *reader;
    ULONG i, max_depth = 32, max_attrs = 128, max_ns = 32;
    BOOL read_decl = TRUE;
    HRESULT hr;

    TRACE( "%p %lu %p %p\n", properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (!(reader = alloc_reader())) return E_OUTOFMEMORY;

    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, sizeof(max_depth) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, sizeof(max_attrs) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_READ_DECLARATION, &read_decl, sizeof(read_decl) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_MAX_NAMESPACES, &max_ns, sizeof(max_ns) );

    for (i = 0; i < count; i++)
    {
        hr = prop_set( reader->prop, reader->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            free_reader( reader );
            return hr;
        }
    }

    if ((hr = init_reader( reader )) != S_OK)
    {
        free_reader( reader );
        return hr;
    }

    TRACE( "created %p\n", reader );
    *handle = (WS_XML_READER *)reader;
    return S_OK;
}

/**************************************************************************
 *          WsFreeReader		[webservices.@]
 */
void WINAPI WsFreeReader( WS_XML_READER *handle )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p\n", handle );

    if (!reader) return;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return;
    }

    reader->magic = 0;

    LeaveCriticalSection( &reader->cs );
    free_reader( reader );
}

static HRESULT read_more_data( struct reader *reader, ULONG min_size, const WS_ASYNC_CONTEXT *ctx,
                               WS_ERROR *error )
{
    ULONG size = 0, max_size;

    if (reader->read_size - reader->read_pos >= min_size) return S_OK;
    if (reader->input_type != WS_XML_READER_INPUT_TYPE_STREAM) return WS_E_INVALID_FORMAT;
    if (min_size > reader->input_size) return WS_E_QUOTA_EXCEEDED;

    if (reader->read_pos)
    {
        memmove( reader->stream_buf, reader->stream_buf + reader->read_pos, reader->read_size - reader->read_pos );
        reader->read_size -= reader->read_pos;
        reader->read_pos = 0;
    }
    max_size = reader->input_size - reader->read_size;

    reader->input_cb( reader->input_cb_state, reader->stream_buf + reader->read_size, max_size, &size, ctx, error );
    if (size < min_size) return WS_E_QUOTA_EXCEEDED;
    reader->read_size += size;
    return S_OK;
}

/**************************************************************************
 *          WsFillReader		[webservices.@]
 */
HRESULT WINAPI WsFillReader( WS_XML_READER *handle, ULONG min_size, const WS_ASYNC_CONTEXT *ctx,
                             WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %lu %p %p\n", handle, min_size, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );
    if (ctx) FIXME( "ignoring ctx parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (reader->input_type == WS_XML_READER_INPUT_TYPE_STREAM)
    {
        hr = read_more_data( reader, min_size, ctx, error );
    }
    else
    {
        reader->read_size = min( min_size, reader->input_size );
        reader->read_pos = 0;
        hr = S_OK;
    }

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetNamespaceFromPrefix		[webservices.@]
 */
HRESULT WINAPI WsGetNamespaceFromPrefix( WS_XML_READER *handle, const WS_XML_STRING *prefix,
                                         BOOL required, const WS_XML_STRING **ns, WS_ERROR *error )
{
    static const WS_XML_STRING xml = {3, (BYTE *)"xml"};
    static const WS_XML_STRING xmlns = {5, (BYTE *)"xmlns"};
    static const WS_XML_STRING empty_ns = {0, NULL};
    static const WS_XML_STRING xml_ns = {36, (BYTE *)"http://www.w3.org/XML/1998/namespace"};
    static const WS_XML_STRING xmlns_ns = {29, (BYTE *)"http://www.w3.org/2000/xmlns/"};
    struct reader *reader = (struct reader *)handle;
    BOOL found = FALSE;
    HRESULT hr = S_OK;

    TRACE( "%p %s %d %p %p\n", handle, debugstr_xmlstr(prefix), required, ns, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !prefix || !ns) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (reader->state != READER_STATE_STARTELEMENT) hr = WS_E_INVALID_OPERATION;
    else if (!prefix->length)
    {
        *ns = &empty_ns;
        found = TRUE;
    }
    else if (WsXmlStringEquals( prefix, &xml, NULL ) == S_OK)
    {
        *ns = &xml_ns;
        found = TRUE;
    }
    else if (WsXmlStringEquals( prefix, &xmlns, NULL ) == S_OK)
    {
        *ns = &xmlns_ns;
        found = TRUE;
    }
    else
    {
        WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;
        ULONG i;

        for (i = 0; i < elem->attributeCount; i++)
        {
            if (!elem->attributes[i]->isXmlNs) continue;
            if (WsXmlStringEquals( prefix, elem->attributes[i]->prefix, NULL ) == S_OK)
            {
                *ns = elem->attributes[i]->ns;
                found = TRUE;
                break;
            }
        }
    }

    LeaveCriticalSection( &reader->cs );

    if (hr == S_OK && !found)
    {
        if (required) hr = WS_E_INVALID_FORMAT;
        else
        {
            *ns = NULL;
            hr = S_FALSE;
        }
    }

    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetReaderNode		[webservices.@]
 */
HRESULT WINAPI WsGetReaderNode( WS_XML_READER *handle, const WS_XML_NODE **node,
                                WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, node, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !node) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    *node = &reader->current->hdr.node;

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return S_OK;
}

static HRESULT get_charset( struct reader *reader, void *buf, ULONG size )
{
    if (!buf || size != sizeof(reader->input_charset)) return E_INVALIDARG;
    if (!reader->input_charset) return WS_E_INVALID_FORMAT;
    *(WS_CHARSET *)buf = reader->input_charset;
    return S_OK;
}

/**************************************************************************
 *          WsGetReaderProperty		[webservices.@]
 */
HRESULT WINAPI WsGetReaderProperty( WS_XML_READER *handle, WS_XML_READER_PROPERTY_ID id,
                                    void *buf, ULONG size, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %lu %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type) hr = WS_E_INVALID_OPERATION;
    else if (id == WS_XML_READER_PROPERTY_CHARSET) hr = get_charset( reader, buf, size );
    else hr = prop_get( reader->prop, reader->prop_count, id, buf, size );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetXmlAttribute		[webservices.@]
 */
HRESULT WINAPI WsGetXmlAttribute( WS_XML_READER *handle, const WS_XML_STRING *attr,
                                  WS_HEAP *heap, WCHAR **str, ULONG *len, WS_ERROR *error )
{
    FIXME( "%p %s %p %p %p %p: stub\n", handle, debugstr_xmlstr(attr), heap, str, len, error );
    return E_NOTIMPL;
}

WS_XML_UTF8_TEXT *alloc_utf8_text( const BYTE *data, ULONG len )
{
    WS_XML_UTF8_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) + len ))) return NULL;
    ret->text.textType    = WS_XML_TEXT_TYPE_UTF8;
    ret->value.length     = len;
    ret->value.bytes      = len ? (BYTE *)(ret + 1) : NULL;
    ret->value.dictionary = NULL;
    ret->value.id         = 0;
    if (data) memcpy( ret->value.bytes, data, len );
    return ret;
}

WS_XML_UTF16_TEXT *alloc_utf16_text( const BYTE *data, ULONG len )
{
    WS_XML_UTF16_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) + len ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_UTF16;
    ret->byteCount     = len;
    ret->bytes         = len ? (BYTE *)(ret + 1) : NULL;
    if (data) memcpy( ret->bytes, data, len );
    return ret;
}

WS_XML_BASE64_TEXT *alloc_base64_text( const BYTE *data, ULONG len )
{
    WS_XML_BASE64_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) + len ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_BASE64;
    ret->length        = len;
    ret->bytes         = len ? (BYTE *)(ret + 1) : NULL;
    if (data) memcpy( ret->bytes, data, len );
    return ret;
}

WS_XML_BOOL_TEXT *alloc_bool_text( BOOL value )
{
    WS_XML_BOOL_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_BOOL;
    ret->value         = value;
    return ret;
}

WS_XML_INT32_TEXT *alloc_int32_text( INT32 value )
{
    WS_XML_INT32_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_INT32;
    ret->value         = value;
    return ret;
}

WS_XML_INT64_TEXT *alloc_int64_text( INT64 value )
{
    WS_XML_INT64_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_INT64;
    ret->value         = value;
    return ret;
}

WS_XML_UINT64_TEXT *alloc_uint64_text( UINT64 value )
{
    WS_XML_UINT64_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_UINT64;
    ret->value         = value;
    return ret;
}

static WS_XML_FLOAT_TEXT *alloc_float_text( float value )
{
    WS_XML_FLOAT_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_FLOAT;
    ret->value         = value;
    return ret;
}

WS_XML_DOUBLE_TEXT *alloc_double_text( double value )
{
    WS_XML_DOUBLE_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_DOUBLE;
    ret->value         = value;
    return ret;
}

WS_XML_GUID_TEXT *alloc_guid_text( const GUID *value )
{
    WS_XML_GUID_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_GUID;
    ret->value         = *value;
    return ret;
}

WS_XML_UNIQUE_ID_TEXT *alloc_unique_id_text( const GUID *value )
{
    WS_XML_UNIQUE_ID_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_UNIQUE_ID;
    ret->value         = *value;
    return ret;
}

WS_XML_DATETIME_TEXT *alloc_datetime_text( const WS_DATETIME *value )
{
    WS_XML_DATETIME_TEXT *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;
    ret->text.textType = WS_XML_TEXT_TYPE_DATETIME;
    ret->value         = *value;
    return ret;
}

static inline BOOL read_end_of_data( struct reader *reader )
{
    return (read_more_data( reader, 1, NULL, NULL ) != S_OK);
}

static inline const unsigned char *read_current_ptr( struct reader *reader )
{
    return &reader->read_bufptr[reader->read_pos];
}

static inline void read_skip( struct reader *reader, unsigned int count )
{
    assert( reader->read_pos + count <= reader->read_size );
    reader->read_pos += count;
}

static inline HRESULT read_peek( struct reader *reader, unsigned char *bytes, unsigned int len )
{
    HRESULT hr;
    if ((hr = read_more_data( reader, len, NULL, NULL )) != S_OK) return hr;
    memcpy( bytes, read_current_ptr( reader ), len );
    return S_OK;
}

static inline HRESULT read_byte( struct reader *reader, unsigned char *byte )
{
    HRESULT hr;
    if ((hr = read_more_data( reader, 1, NULL, NULL )) != S_OK) return hr;
    *byte = *read_current_ptr( reader );
    read_skip( reader, 1 );
    return S_OK;
}

static inline HRESULT read_bytes( struct reader *reader, unsigned char *bytes, unsigned int len )
{
    HRESULT hr;
    if ((hr = read_more_data( reader, len, NULL, NULL )) != S_OK) return hr;
    memcpy( bytes, read_current_ptr( reader ), len );
    read_skip( reader, len );
    return S_OK;
}

/* UTF-8 support based on libs/wine/utf8.c */

/* number of following bytes in sequence based on first byte value (for bytes above 0x7f) */
static const char utf8_length[128] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9f */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xa0-0xaf */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xb0-0xbf */
    0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xc0-0xcf */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xd0-0xdf */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xe0-0xef */
    3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0  /* 0xf0-0xff */
};

/* first byte mask depending on UTF-8 sequence length */
static const unsigned char utf8_mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };

/* minimum Unicode value depending on UTF-8 sequence length */
static const unsigned int utf8_minval[4] = { 0x0, 0x80, 0x800, 0x10000 };

static inline HRESULT read_utf8_char( struct reader *reader, unsigned int *ret, unsigned int *skip )
{
    unsigned int len;
    unsigned char ch;
    const unsigned char *end;
    HRESULT hr;

    if ((hr = read_more_data( reader, 1, NULL, NULL )) != S_OK) return hr;
    ch = *read_current_ptr( reader );
    if (ch < 0x80)
    {
        *ret = ch;
        *skip = 1;
        return S_OK;
    }

    len = utf8_length[ch - 0x80];
    if ((hr = read_more_data( reader, len, NULL, NULL )) != S_OK) return hr;
    end = read_current_ptr( reader ) + len + 1;
    *ret = ch & utf8_mask[len];

    switch (len)
    {
    case 3:
        if ((ch = end[-3] ^ 0x80) >= 0x40) break;
        *ret = (*ret << 6) | ch;
    case 2:
        if ((ch = end[-2] ^ 0x80) >= 0x40) break;
        *ret = (*ret << 6) | ch;
    case 1:
        if ((ch = end[-1] ^ 0x80) >= 0x40) break;
        *ret = (*ret << 6) | ch;
        if (*ret < utf8_minval[len]) break;
        *skip = len + 1;
        return S_OK;
    }

    return WS_E_INVALID_FORMAT;
}

static inline BOOL read_isnamechar( unsigned int ch )
{
    /* FIXME: incomplete */
    return (ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9') ||
           ch == '_' || ch == '-' || ch == '.' || ch == ':';
}

static inline BOOL read_isspace( unsigned int ch )
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static inline void read_skip_whitespace( struct reader *reader )
{
    for (;;)
    {
        if (read_more_data( reader, 1, NULL, NULL ) != S_OK || !read_isspace( *read_current_ptr( reader ) )) break;
        read_skip( reader, 1 );
    }
}

static inline HRESULT read_cmp( struct reader *reader, const char *str, int len )
{
    const unsigned char *ptr;
    HRESULT hr;

    if (len < 0) len = strlen( str );
    if ((hr = read_more_data( reader, len, NULL, NULL )) != S_OK) return hr;

    ptr = read_current_ptr( reader );
    while (len--)
    {
        if (*str != *ptr) return WS_E_INVALID_FORMAT;
        str++; ptr++;
    }
    return S_OK;
}

static HRESULT read_xmldecl( struct reader *reader )
{
    HRESULT hr;

    if ((hr = read_more_data( reader, 1, NULL, NULL )) != S_OK) return hr;
    if (*read_current_ptr( reader ) != '<' || (hr = read_cmp( reader, "<?", 2 )) != S_OK)
    {
        reader->state = READER_STATE_BOF;
        return S_OK;
    }
    if ((hr = read_cmp( reader, "<?xml ", 6 )) != S_OK) return hr;
    read_skip( reader, 6 );

    /* FIXME: parse attributes */
    for (;;)
    {
        if (read_more_data( reader, 1, NULL, NULL ) != S_OK || *read_current_ptr( reader ) == '?' ) break;
        read_skip( reader, 1 );
    }

    if ((hr = read_cmp( reader, "?>", 2 )) != S_OK) return hr;
    read_skip( reader, 2 );

    reader->state = READER_STATE_BOF;
    return S_OK;
}

HRESULT append_attribute( WS_XML_ELEMENT_NODE *elem, WS_XML_ATTRIBUTE *attr )
{
    if (elem->attributeCount)
    {
        WS_XML_ATTRIBUTE **tmp;
        if (!(tmp = realloc( elem->attributes, (elem->attributeCount + 1) * sizeof(attr) ))) return E_OUTOFMEMORY;
        elem->attributes = tmp;
    }
    else if (!(elem->attributes = malloc( sizeof(attr) ))) return E_OUTOFMEMORY;
    elem->attributes[elem->attributeCount++] = attr;
    return S_OK;
}

static inline void init_xml_string( BYTE *bytes, ULONG len, WS_XML_STRING *str )
{
    str->length     = len;
    str->bytes      = bytes;
    str->dictionary = NULL;
    str->id         = 0;
}

static HRESULT split_qname( const BYTE *str, ULONG len, WS_XML_STRING *prefix, WS_XML_STRING *localname )
{
    BYTE *prefix_bytes = NULL, *localname_bytes = (BYTE *)str, *ptr = (BYTE *)str;
    ULONG prefix_len = 0, localname_len = len;

    while (len--)
    {
        if (*ptr == ':')
        {
            if (ptr == str) return WS_E_INVALID_FORMAT;
            prefix_bytes = (BYTE *)str;
            prefix_len   = ptr - str;
            localname_bytes = ptr + 1;
            localname_len   = len;
            break;
        }
        ptr++;
    }
    if (!localname_len) return WS_E_INVALID_FORMAT;

    init_xml_string( prefix_bytes, prefix_len, prefix );
    init_xml_string( localname_bytes, localname_len, localname );
    return S_OK;
}

static HRESULT parse_qname( const BYTE *str, ULONG len, WS_XML_STRING **prefix_ret, WS_XML_STRING **localname_ret )
{
    WS_XML_STRING prefix, localname;
    HRESULT hr;

    if ((hr = split_qname( str, len, &prefix, &localname )) != S_OK) return hr;
    if (!(*prefix_ret = alloc_xml_string( NULL, prefix.length ))) return E_OUTOFMEMORY;
    if (!(*localname_ret = dup_xml_string( &localname, FALSE )))
    {
        free_xml_string( *prefix_ret );
        return E_OUTOFMEMORY;
    }
    memcpy( (*prefix_ret)->bytes, prefix.bytes, prefix.length );
    if (prefix.length && add_xml_string( *prefix_ret ) != S_OK) WARN( "prefix not added to dictionary\n" );
    return S_OK;
}

static int codepoint_to_utf8( int cp, unsigned char *dst )
{
    if (!cp) return -1;
    if (cp < 0x80)
    {
        *dst = cp;
        return 1;
    }
    if (cp < 0x800)
    {
        dst[1] = 0x80 | (cp & 0x3f);
        cp >>= 6;
        dst[0] = 0xc0 | cp;
        return 2;
    }
    if ((cp >= 0xd800 && cp <= 0xdfff) || cp == 0xfffe || cp == 0xffff) return -1;
    if (cp < 0x10000)
    {
        dst[2] = 0x80 | (cp & 0x3f);
        cp >>= 6;
        dst[1] = 0x80 | (cp & 0x3f);
        cp >>= 6;
        dst[0] = 0xe0 | cp;
        return 3;
    }
    if (cp >= 0x110000) return -1;
    dst[3] = 0x80 | (cp & 0x3f);
    cp >>= 6;
    dst[2] = 0x80 | (cp & 0x3f);
    cp >>= 6;
    dst[1] = 0x80 | (cp & 0x3f);
    cp >>= 6;
    dst[0] = 0xf0 | cp;
    return 4;
}

static HRESULT decode_text( const unsigned char *str, ULONG len, unsigned char *ret, ULONG *ret_len  )
{
    const unsigned char *p = str;
    unsigned char *q = ret;

    *ret_len = 0;
    while (len)
    {
        if (*p == '&')
        {
            p++; len--;
            if (!len) return WS_E_INVALID_FORMAT;

            if (len >= 3 && !memcmp( p, "lt;", 3 ))
            {
                *q++ = '<';
                p += 3;
                len -= 3;
            }
            else if (len >= 3 && !memcmp( p, "gt;", 3 ))
            {
                *q++ = '>';
                p += 3;
                len -= 3;
            }
            else if (len >= 5 && !memcmp( p, "quot;", 5 ))
            {
                *q++ = '"';
                p += 5;
                len -= 5;
            }
            else if (len >= 4 && !memcmp( p, "amp;", 4 ))
            {
                *q++ = '&';
                p += 4;
                len -= 4;
            }
            else if (len >= 5 && !memcmp( p, "apos;", 5 ))
            {
                *q++ = '\'';
                p += 5;
                len -= 5;
            }
            else if (*p == '#')
            {
                ULONG start, nb_digits, i;
                int len_utf8, cp = 0;

                p++; len--;
                if (!len) return WS_E_INVALID_FORMAT;
                if (*p == 'x')
                {
                    p++; len--;

                    start = len;
                    while (len && isxdigit( *p )) { p++; len--; };
                    if (!len) return WS_E_INVALID_FORMAT;

                    p -= nb_digits = start - len;
                    if (!nb_digits || nb_digits > 6 || p[nb_digits] != ';') return WS_E_INVALID_FORMAT;
                    for (i = 0; i < nb_digits; i++)
                    {
                        cp *= 16;
                        if (*p >= '0' && *p <= '9') cp += *p - '0';
                        else if (*p >= 'a' && *p <= 'f') cp += *p - 'a' + 10;
                        else cp += *p - 'A' + 10;
                        p++;
                    }
                }
                else if (isdigit( *p ))
                {
                    while (len && *p == '0') { p++; len--; };
                    if (!len) return WS_E_INVALID_FORMAT;

                    start = len;
                    while (len && isdigit( *p )) { p++; len--; };
                    if (!len) return WS_E_INVALID_FORMAT;

                    p -= nb_digits = start - len;
                    if (!nb_digits || nb_digits > 7 || p[nb_digits] != ';') return WS_E_INVALID_FORMAT;
                    for (i = 0; i < nb_digits; i++)
                    {
                        cp *= 10;
                        cp += *p - '0';
                        p++;
                    }
                }
                else return WS_E_INVALID_FORMAT;
                p++; len--;
                if ((len_utf8 = codepoint_to_utf8( cp, q )) < 0) return WS_E_INVALID_FORMAT;
                *ret_len += len_utf8;
                q += len_utf8;
                continue;
            }
            else return WS_E_INVALID_FORMAT;
        }
        else
        {
            *q++ = *p++;
            len--;
        }
        *ret_len += 1;
    }
    return S_OK;
}

static HRESULT read_attribute_value_text( struct reader *reader, WS_XML_ATTRIBUTE *attr )
{
    WS_XML_UTF8_TEXT *utf8;
    unsigned int len, ch, skip, quote;
    const unsigned char *start;
    HRESULT hr;

    read_skip_whitespace( reader );
    if ((hr = read_cmp( reader, "=", 1 )) != S_OK) return hr;
    read_skip( reader, 1 );

    read_skip_whitespace( reader );
    if ((hr = read_cmp( reader, "\"", 1 )) != S_OK && (hr = read_cmp( reader, "'", 1 )) != S_OK) return hr;
    if ((hr = read_utf8_char( reader, &quote, &skip )) != S_OK) return hr;
    read_skip( reader, 1 );

    len = 0;
    start = read_current_ptr( reader );
    for (;;)
    {
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) return hr;
        if (ch == quote) break;
        read_skip( reader, skip );
        len += skip;
    }
    read_skip( reader, 1 );

    if (attr->isXmlNs)
    {
        if (!(attr->ns = alloc_xml_string( start, len ))) return E_OUTOFMEMORY;
        if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) return hr;
        if (!(utf8 = alloc_utf8_text( NULL, 0 ))) return E_OUTOFMEMORY;
    }
    else
    {
        if (!(utf8 = alloc_utf8_text( NULL, len ))) return E_OUTOFMEMORY;
        if ((hr = decode_text( start, len, utf8->value.bytes, &utf8->value.length )) != S_OK)
        {
            free( utf8 );
            return hr;
        }
    }

    attr->value = &utf8->text;
    attr->singleQuote = (quote == '\'');
    return S_OK;
}

static inline BOOL is_text_type( unsigned char type )
{
    return (type >= RECORD_ZERO_TEXT && type <= RECORD_QNAME_DICTIONARY_TEXT_WITH_ENDELEMENT);
}

static HRESULT read_int31( struct reader *reader, ULONG *len )
{
    unsigned char byte;
    HRESULT hr;

    if ((hr = read_byte( reader, &byte )) != S_OK) return hr;
    *len = byte & 0x7f;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = read_byte( reader, &byte )) != S_OK) return hr;
    *len += (byte & 0x7f) << 7;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = read_byte( reader, &byte )) != S_OK) return hr;
    *len += (byte & 0x7f) << 14;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = read_byte( reader, &byte )) != S_OK) return hr;
    *len += (byte & 0x7f) << 21;
    if (!(byte & 0x80)) return S_OK;

    if ((hr = read_byte( reader, &byte )) != S_OK) return hr;
    *len += (byte & 0x07) << 28;
    return S_OK;
}

static HRESULT read_string( struct reader *reader, WS_XML_STRING **str )
{
    ULONG len;
    HRESULT hr;
    if ((hr = read_int31( reader, &len )) != S_OK) return hr;
    if (!(*str = alloc_xml_string( NULL, len ))) return E_OUTOFMEMORY;
    if ((hr = read_bytes( reader, (*str)->bytes, len )) == S_OK)
    {
        if (add_xml_string( *str ) != S_OK) WARN( "string not added to dictionary\n" );
        return S_OK;
    }
    free_xml_string( *str );
    return hr;
}

static HRESULT read_dict_string( struct reader *reader, WS_XML_STRING **str )
{
    const WS_XML_DICTIONARY *dict;
    HRESULT hr;
    ULONG id;

    if ((hr = read_int31( reader, &id )) != S_OK) return hr;
    dict = (id & 1) ? reader->dict : reader->dict_static;
    if (!dict || (id >>= 1) >= dict->stringCount) return WS_E_INVALID_FORMAT;
    if (!(*str = alloc_xml_string( NULL, 0 ))) return E_OUTOFMEMORY;
    *(*str) = dict->strings[id];
    return S_OK;
}

static HRESULT read_datetime( struct reader *reader, WS_DATETIME *ret )
{
    UINT64 val;
    HRESULT hr;

    if ((hr = read_bytes( reader, (unsigned char *)&val, sizeof(val) )) != S_OK) return hr;

    if ((val & 0x03) == 1) ret->format = WS_DATETIME_FORMAT_UTC;
    else if ((val & 0x03) == 2) ret->format = WS_DATETIME_FORMAT_LOCAL;
    else ret->format = WS_DATETIME_FORMAT_NONE;

    if ((ret->ticks = val >> 2) > TICKS_MAX) return WS_E_INVALID_FORMAT;
    return S_OK;
}

static HRESULT lookup_string( struct reader *reader, ULONG id, const WS_XML_STRING **ret )
{
    const WS_XML_DICTIONARY *dict = (id & 1) ? reader->dict : reader->dict_static;
    if (!dict || (id >>= 1) >= dict->stringCount) return WS_E_INVALID_FORMAT;
    *ret = &dict->strings[id];
    return S_OK;
}

static HRESULT read_attribute_value_bin( struct reader *reader, WS_XML_ATTRIBUTE *attr )
{
    WS_XML_UTF8_TEXT *text_utf8 = NULL;
    WS_XML_BASE64_TEXT *text_base64 = NULL;
    WS_XML_INT32_TEXT *text_int32;
    WS_XML_INT64_TEXT *text_int64;
    WS_XML_BOOL_TEXT *text_bool;
    const WS_XML_STRING *str;
    unsigned char type;
    UINT8 val_uint8;
    UINT16 val_uint16;
    INT32 val_int32;
    ULONG len = 0, id;
    GUID guid;
    HRESULT hr;

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (!is_text_type( type )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 1 );

    switch (type)
    {
    case RECORD_ZERO_TEXT:
    {
        if (!(text_int32 = alloc_int32_text( 0 ))) return E_OUTOFMEMORY;
        attr->value = &text_int32->text;
        return S_OK;
    }
    case RECORD_ONE_TEXT:
    {
        if (!(text_int32 = alloc_int32_text( 1 ))) return E_OUTOFMEMORY;
        attr->value = &text_int32->text;
        return S_OK;
    }
    case RECORD_FALSE_TEXT:
    {
        if (!(text_bool = alloc_bool_text( FALSE ))) return E_OUTOFMEMORY;
        attr->value = &text_bool->text;
        return S_OK;
    }
    case RECORD_TRUE_TEXT:
    {
        if (!(text_bool = alloc_bool_text( TRUE ))) return E_OUTOFMEMORY;
        attr->value = &text_bool->text;
        return S_OK;
    }
    case RECORD_INT8_TEXT:
    {
        INT8 val_int8;
        if ((hr = read_byte( reader, (unsigned char *)&val_int8 )) != S_OK) return hr;
        if (!(text_int64 = alloc_int64_text( val_int8 ))) return E_OUTOFMEMORY;
        attr->value = &text_int64->text;
        return S_OK;
    }
    case RECORD_INT16_TEXT:
    {
        INT16 val_int16;
        if ((hr = read_bytes( reader, (unsigned char *)&val_int16, sizeof(val_int16) )) != S_OK) return hr;
        if (!(text_int64 = alloc_int64_text( val_int16 ))) return E_OUTOFMEMORY;
        attr->value = &text_int64->text;
        return S_OK;
    }
    case RECORD_INT32_TEXT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_int32, sizeof(val_int32) )) != S_OK) return hr;
        if (!(text_int64 = alloc_int64_text( val_int32 ))) return E_OUTOFMEMORY;
        attr->value = &text_int64->text;
        return S_OK;

    case RECORD_INT64_TEXT:
    {
        INT64 val_int64;
        if ((hr = read_bytes( reader, (unsigned char *)&val_int64, sizeof(val_int64) )) != S_OK) return hr;
        if (!(text_int64 = alloc_int64_text( val_int64 ))) return E_OUTOFMEMORY;
        attr->value = &text_int64->text;
        return S_OK;
    }
    case RECORD_FLOAT_TEXT:
    {
        WS_XML_FLOAT_TEXT *text_float;
        float val_float;

        if ((hr = read_bytes( reader, (unsigned char *)&val_float, sizeof(val_float) )) != S_OK) return hr;
        if (!(text_float = alloc_float_text( val_float ))) return E_OUTOFMEMORY;
        attr->value = &text_float->text;
        return S_OK;
    }
    case RECORD_DOUBLE_TEXT:
    {
        WS_XML_DOUBLE_TEXT *text_double;
        double val_double;

        if ((hr = read_bytes( reader, (unsigned char *)&val_double, sizeof(val_double) )) != S_OK) return hr;
        if (!(text_double = alloc_double_text( val_double ))) return E_OUTOFMEMORY;
        attr->value = &text_double->text;
        return S_OK;
    }
    case RECORD_DATETIME_TEXT:
    {
        WS_XML_DATETIME_TEXT *text_datetime;
        WS_DATETIME datetime;

        if ((hr = read_datetime( reader, &datetime )) != S_OK) return hr;
        if (!(text_datetime = alloc_datetime_text( &datetime ))) return E_OUTOFMEMORY;
        attr->value = &text_datetime->text;
        return S_OK;
    }
    case RECORD_CHARS8_TEXT:
        if ((hr = read_byte( reader, (unsigned char *)&val_uint8 )) != S_OK) return hr;
        len = val_uint8;
        break;

    case RECORD_CHARS16_TEXT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_uint16, sizeof(val_uint16) )) != S_OK) return hr;
        len = val_uint16;
        break;

    case RECORD_CHARS32_TEXT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_int32, sizeof(val_int32) )) != S_OK) return hr;
        if (val_int32 < 0) return WS_E_INVALID_FORMAT;
        len = val_int32;
        break;

    case RECORD_BYTES8_TEXT:
        if ((hr = read_byte( reader, (unsigned char *)&val_uint8 )) != S_OK) return hr;
        if (!(text_base64 = alloc_base64_text( NULL, val_uint8 ))) return E_OUTOFMEMORY;
        if ((hr = read_bytes( reader, text_base64->bytes, val_uint8 )) != S_OK)
        {
            free( text_base64 );
            return hr;
        }
        break;

    case RECORD_BYTES16_TEXT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_uint16, sizeof(val_uint16) )) != S_OK) return hr;
        if (!(text_base64 = alloc_base64_text( NULL, val_uint16 ))) return E_OUTOFMEMORY;
        if ((hr = read_bytes( reader, text_base64->bytes, val_uint16 )) != S_OK)
        {
            free( text_base64 );
            return hr;
        }
        break;

    case RECORD_BYTES32_TEXT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_int32, sizeof(val_int32) )) != S_OK) return hr;
        if (val_int32 < 0) return WS_E_INVALID_FORMAT;
        if (!(text_base64 = alloc_base64_text( NULL, val_int32 ))) return E_OUTOFMEMORY;
        if ((hr = read_bytes( reader, text_base64->bytes, val_int32 )) != S_OK)
        {
            free( text_base64 );
            return hr;
        }
        break;

    case RECORD_EMPTY_TEXT:
        break;

    case RECORD_DICTIONARY_TEXT:
        if ((hr = read_int31( reader, &id )) != S_OK) return hr;
        if ((hr = lookup_string( reader, id, &str )) != S_OK) return hr;
        if (!(text_utf8 = alloc_utf8_text( str->bytes, str->length ))) return E_OUTOFMEMORY;
        break;

    case RECORD_UNIQUE_ID_TEXT:
    {
        WS_XML_UNIQUE_ID_TEXT *text_unique_id;
        if ((hr = read_bytes( reader, (unsigned char *)&guid, sizeof(guid) )) != S_OK) return hr;
        if (!(text_unique_id = alloc_unique_id_text( &guid ))) return E_OUTOFMEMORY;
        attr->value = &text_unique_id->text;
        return S_OK;
    }
    case RECORD_GUID_TEXT:
    {
        WS_XML_GUID_TEXT *guid_text;
        if ((hr = read_bytes( reader, (unsigned char *)&guid, sizeof(guid) )) != S_OK) return hr;
        if (!(guid_text = alloc_guid_text( &guid ))) return E_OUTOFMEMORY;
        attr->value = &guid_text->text;
        return S_OK;
    }
    case RECORD_UINT64_TEXT:
    {
        WS_XML_UINT64_TEXT *text_uint64;
        UINT64 val_uint64;

        if ((hr = read_bytes( reader, (unsigned char *)&val_uint64, sizeof(val_uint64) )) != S_OK) return hr;
        if (!(text_uint64 = alloc_uint64_text( val_uint64 ))) return E_OUTOFMEMORY;
        attr->value = &text_uint64->text;
        return S_OK;
    }
    case RECORD_BOOL_TEXT:
    {
        WS_XML_BOOL_TEXT *text_bool;
        BOOL val_bool;

        if ((hr = read_bytes( reader, (unsigned char *)&val_bool, sizeof(val_bool) )) != S_OK) return hr;
        if (!(text_bool = alloc_bool_text( !!val_bool ))) return E_OUTOFMEMORY;
        attr->value = &text_bool->text;
        return S_OK;
    }
    default:
        ERR( "unhandled record type %02x\n", type );
        return WS_E_NOT_SUPPORTED;
    }

    if (type >= RECORD_BYTES8_TEXT && type <= RECORD_BYTES32_TEXT)
    {
        attr->value = &text_base64->text;
        return S_OK;
    }

    if (!text_utf8)
    {
        if (!(text_utf8 = alloc_utf8_text( NULL, len ))) return E_OUTOFMEMORY;
        if (!len) text_utf8->value.bytes = (BYTE *)(text_utf8 + 1); /* quirk */
        if ((hr = read_bytes( reader, text_utf8->value.bytes, len )) != S_OK)
        {
            free( text_utf8 );
            return hr;
        }
    }

    attr->value = &text_utf8->text;
    return S_OK;
}

static HRESULT read_attribute_text( struct reader *reader, WS_XML_ATTRIBUTE **ret )
{
    static const WS_XML_STRING xmlns = {5, (BYTE *)"xmlns"};
    WS_XML_ATTRIBUTE *attr;
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    WS_XML_STRING *prefix, *localname;
    HRESULT hr;

    if (!(attr = calloc( 1, sizeof(*attr) ))) return E_OUTOFMEMORY;

    start = read_current_ptr( reader );
    for (;;)
    {
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) goto error;
        if (!read_isnamechar( ch )) break;
        read_skip( reader, skip );
        len += skip;
    }
    if (!len)
    {
        hr = WS_E_INVALID_FORMAT;
        goto error;
    }

    if ((hr = parse_qname( start, len, &prefix, &localname )) != S_OK) goto error;
    if (WsXmlStringEquals( prefix, &xmlns, NULL ) == S_OK)
    {
        free_xml_string( prefix );
        attr->isXmlNs   = 1;
        if (!(attr->prefix = alloc_xml_string( localname->bytes, localname->length )))
        {
            free_xml_string( localname );
            hr = E_OUTOFMEMORY;
            goto error;
        }
        attr->localName = localname;
    }
    else if (!prefix->length && WsXmlStringEquals( localname, &xmlns, NULL ) == S_OK)
    {
        attr->isXmlNs   = 1;
        attr->prefix    = prefix;
        attr->localName = localname;
    }
    else
    {
        attr->prefix    = prefix;
        attr->localName = localname;
    }

    if ((hr = read_attribute_value_text( reader, attr )) != S_OK) goto error;

    *ret = attr;
    return S_OK;

error:
    free_attribute( attr );
    return hr;
}

static inline BOOL is_attribute_type( unsigned char type )
{
    return (type >= RECORD_SHORT_ATTRIBUTE && type <= RECORD_PREFIX_ATTRIBUTE_Z);
}

static WS_XML_STRING *get_xmlns_localname( struct reader *reader, const WS_XML_STRING *prefix )
{
    if (!get_namespace( reader, prefix )) return alloc_xml_string( NULL, 0 );
    return alloc_xml_string( prefix->bytes, prefix->length );
}

static HRESULT read_attribute_bin( struct reader *reader, WS_XML_ATTRIBUTE **ret )
{
    WS_XML_UTF8_TEXT *utf8;
    WS_XML_ATTRIBUTE *attr;
    unsigned char type = 0;
    HRESULT hr;

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (!is_attribute_type( type )) return WS_E_INVALID_FORMAT;
    if (!(attr = calloc( 1, sizeof(*attr) ))) return E_OUTOFMEMORY;
    read_skip( reader, 1 );

    if (type >= RECORD_PREFIX_ATTRIBUTE_A && type <= RECORD_PREFIX_ATTRIBUTE_Z)
    {
        unsigned char ch = type - RECORD_PREFIX_ATTRIBUTE_A + 'a';
        if (!(attr->prefix = alloc_xml_string( &ch, 1 )))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        if ((hr = read_string( reader, &attr->localName )) != S_OK) goto error;
        if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
    }
    else if (type >= RECORD_PREFIX_DICTIONARY_ATTRIBUTE_A && type <= RECORD_PREFIX_DICTIONARY_ATTRIBUTE_Z)
    {
        unsigned char ch = type - RECORD_PREFIX_DICTIONARY_ATTRIBUTE_A + 'a';
        if (!(attr->prefix = alloc_xml_string( &ch, 1 )))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        if ((hr = read_dict_string( reader, &attr->localName )) != S_OK) goto error;
        if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
    }
    else
    {
        switch (type)
        {
        case RECORD_SHORT_ATTRIBUTE:
            if (!(attr->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_string( reader, &attr->localName )) != S_OK) goto error;
            if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
            break;

        case RECORD_ATTRIBUTE:
            if ((hr = read_string( reader, &attr->prefix )) != S_OK) goto error;
            if ((hr = read_string( reader, &attr->localName )) != S_OK) goto error;
            if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
            break;

        case RECORD_SHORT_DICTIONARY_ATTRIBUTE:
            if (!(attr->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_dict_string( reader, &attr->localName )) != S_OK) goto error;
            if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
            break;

        case RECORD_DICTIONARY_ATTRIBUTE:
            if ((hr = read_string( reader, &attr->prefix )) != S_OK) goto error;
            if ((hr = read_dict_string( reader, &attr->localName )) != S_OK) goto error;
            if ((hr = read_attribute_value_bin( reader, attr )) != S_OK) goto error;
            break;

        case RECORD_SHORT_XMLNS_ATTRIBUTE:
            if (!(attr->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if (!(attr->localName = get_xmlns_localname( reader, attr->prefix )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_string( reader, &attr->ns )) != S_OK) goto error;
            if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) goto error;
            attr->isXmlNs = 1;
            break;

        case RECORD_XMLNS_ATTRIBUTE:
            if ((hr = read_string( reader, &attr->prefix )) != S_OK) goto error;
            if (!(attr->localName = get_xmlns_localname( reader, attr->prefix )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_string( reader, &attr->ns )) != S_OK) goto error;
            if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) goto error;
            attr->isXmlNs = 1;
            break;

        case RECORD_SHORT_DICTIONARY_XMLNS_ATTRIBUTE:
            if (!(attr->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if (!(attr->localName = get_xmlns_localname( reader, attr->prefix )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_dict_string( reader, &attr->ns )) != S_OK) goto error;
            if (!(utf8 = alloc_utf8_text( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            attr->value = &utf8->text;
            if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) goto error;
            attr->isXmlNs = 1;
            break;

        case RECORD_DICTIONARY_XMLNS_ATTRIBUTE:
            if ((hr = read_string( reader, &attr->prefix )) != S_OK) goto error;
            if (!(attr->localName = get_xmlns_localname( reader, attr->prefix )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_dict_string( reader, &attr->ns )) != S_OK) goto error;
            if (!(utf8 = alloc_utf8_text( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            attr->value = &utf8->text;
            if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) goto error;
            attr->isXmlNs = 1;
            break;

        default:
            ERR( "unhandled record type %02x\n", type );
            return WS_E_NOT_SUPPORTED;
        }
    }

    *ret = attr;
    return S_OK;

error:
    free_attribute( attr );
    return hr;
}

static inline struct node *find_parent( struct reader *reader )
{
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_END_ELEMENT)
    {
        if (is_valid_parent( reader->current->parent->parent )) return reader->current->parent->parent;
        return NULL;
    }
    if (is_valid_parent( reader->current )) return reader->current;
    if (is_valid_parent( reader->current->parent )) return reader->current->parent;
    return NULL;
}

static HRESULT set_namespaces( struct reader *reader, WS_XML_ELEMENT_NODE *elem )
{
    static const WS_XML_STRING xml = {3, (BYTE *)"xml"};
    const WS_XML_STRING *ns;
    ULONG i;

    if (!(ns = get_namespace( reader, elem->prefix ))) return WS_E_INVALID_FORMAT;
    if (!(elem->ns = dup_xml_string( ns, FALSE ))) return E_OUTOFMEMORY;

    for (i = 0; i < elem->attributeCount; i++)
    {
        WS_XML_ATTRIBUTE *attr = elem->attributes[i];
        if (attr->isXmlNs || WsXmlStringEquals( attr->prefix, &xml, NULL ) == S_OK) continue;
        if (!(ns = get_namespace( reader, attr->prefix ))) return WS_E_INVALID_FORMAT;
        if (!(attr->ns = alloc_xml_string( NULL, ns->length ))) return E_OUTOFMEMORY;
        if (attr->ns->length) memcpy( attr->ns->bytes, ns->bytes, ns->length );
    }
    return S_OK;
}

static WS_XML_ELEMENT_NODE *alloc_element_pair(void)
{
    struct node *node, *end;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) return NULL;
    if (!(end = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT )))
    {
        free_node( node );
        return NULL;
    }
    list_add_tail( &node->children, &end->entry );
    end->parent = node;
    return &node->hdr;
}

static HRESULT read_attributes_text( struct reader *reader, WS_XML_ELEMENT_NODE *elem )
{
    WS_XML_ATTRIBUTE *attr;
    HRESULT hr;

    reader->current_attr = 0;
    for (;;)
    {
        read_skip_whitespace( reader );
        if (read_cmp( reader, ">", 1 ) == S_OK || read_cmp( reader, "/>", 2 ) == S_OK) break;
        if ((hr = read_attribute_text( reader, &attr )) != S_OK) return hr;
        if ((hr = append_attribute( elem, attr )) != S_OK)
        {
            free_attribute( attr );
            return hr;
        }
        reader->current_attr++;
    }
    return S_OK;
}

static HRESULT read_element_text( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    unsigned char buf[2];
    struct node *node = NULL, *parent;
    WS_XML_ELEMENT_NODE *elem;
    HRESULT hr;

    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    if ((hr = read_peek( reader, buf, 2 )) != S_OK) return hr;
    if (buf[0] != '<' || !read_isnamechar( buf[1] )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 1 );

    if (!(elem = alloc_element_pair())) return E_OUTOFMEMORY;
    node = (struct node *)elem;

    start = read_current_ptr( reader );
    for (;;)
    {
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) goto error;
        if (!read_isnamechar( ch )) break;
        read_skip( reader, skip );
        len += skip;
    }
    if (!len)
    {
        hr = WS_E_INVALID_FORMAT;
        goto error;
    }

    if (!(parent = find_parent( reader ))) goto error;
    if ((hr = parse_qname( start, len, &elem->prefix, &elem->localName )) != S_OK) goto error;
    if ((hr = read_attributes_text( reader, elem )) != S_OK) goto error;
    if ((hr = set_namespaces( reader, elem )) != S_OK) goto error;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_STARTELEMENT;
    return S_OK;

error:
    destroy_nodes( node );
    return hr;
}

static inline BOOL is_element_type( unsigned char type )
{
    return (type >= RECORD_SHORT_ELEMENT && type <= RECORD_PREFIX_ELEMENT_Z);
}

static HRESULT read_attributes_bin( struct reader *reader, WS_XML_ELEMENT_NODE *elem )
{
    WS_XML_ATTRIBUTE *attr;
    unsigned char type;
    HRESULT hr;

    reader->current_attr = 0;
    for (;;)
    {
        if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
        if (!is_attribute_type( type )) break;
        if ((hr = read_attribute_bin( reader, &attr )) != S_OK) return hr;
        if ((hr = append_attribute( elem, attr )) != S_OK)
        {
            free_attribute( attr );
            return hr;
        }
        reader->current_attr++;
    }
    return S_OK;
}

static HRESULT read_element_bin( struct reader *reader )
{
    struct node *node = NULL, *parent;
    WS_XML_ELEMENT_NODE *elem;
    unsigned char type;
    HRESULT hr;

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (!is_element_type( type )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 1 );

    if (!(elem = alloc_element_pair())) return E_OUTOFMEMORY;
    node = (struct node *)elem;

    if (type >= RECORD_PREFIX_ELEMENT_A && type <= RECORD_PREFIX_ELEMENT_Z)
    {
        unsigned char ch = type - RECORD_PREFIX_ELEMENT_A + 'a';
        if (!(elem->prefix = alloc_xml_string( &ch, 1 )))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        if ((hr = read_string( reader, &elem->localName )) != S_OK) goto error;
    }
    else if (type >= RECORD_PREFIX_DICTIONARY_ELEMENT_A && type <= RECORD_PREFIX_DICTIONARY_ELEMENT_Z)
    {
        unsigned char ch = type - RECORD_PREFIX_DICTIONARY_ELEMENT_A + 'a';
        if (!(elem->prefix = alloc_xml_string( &ch, 1 )))
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        if ((hr = read_dict_string( reader, &elem->localName )) != S_OK) goto error;
    }
    else
    {
        switch (type)
        {
        case RECORD_SHORT_ELEMENT:
            if (!(elem->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_string( reader, &elem->localName )) != S_OK) goto error;
            break;

        case RECORD_ELEMENT:
            if ((hr = read_string( reader, &elem->prefix )) != S_OK) goto error;
            if ((hr = read_string( reader, &elem->localName )) != S_OK) goto error;
            break;

        case RECORD_SHORT_DICTIONARY_ELEMENT:
            if (!(elem->prefix = alloc_xml_string( NULL, 0 )))
            {
                hr = E_OUTOFMEMORY;
                goto error;
            }
            if ((hr = read_dict_string( reader, &elem->localName )) != S_OK) goto error;
            break;

        case RECORD_DICTIONARY_ELEMENT:
            if ((hr = read_string( reader, &elem->prefix )) != S_OK) goto error;
            if ((hr = read_dict_string( reader, &elem->localName )) != S_OK) goto error;
            break;

        default:
            ERR( "unhandled record type %02x\n", type );
            return WS_E_NOT_SUPPORTED;
        }
    }

    if (!(parent = find_parent( reader )))
    {
        hr = WS_E_INVALID_FORMAT;
        goto error;
    }

    if ((hr = read_attributes_bin( reader, elem )) != S_OK) goto error;
    if ((hr = set_namespaces( reader, elem )) != S_OK) goto error;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_STARTELEMENT;
    return S_OK;

error:
    destroy_nodes( node );
    return hr;
}

static HRESULT read_text_text( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node, *parent;
    WS_XML_TEXT_NODE *text;
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;

    start = read_current_ptr( reader );
    for (;;)
    {
        if (read_end_of_data( reader )) break;
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) return hr;
        if (ch == '<') break;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;
    if (!(utf8 = alloc_utf8_text( NULL, len )))
    {
        free( node );
        return E_OUTOFMEMORY;
    }
    if ((hr = decode_text( start, len, utf8->value.bytes, &utf8->value.length )) != S_OK)
    {
        free( utf8 );
        free( node );
        return hr;
    }
    text->text = &utf8->text;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_TEXT;
    reader->text_conv_offset = 0;
    return S_OK;
}

static struct node *alloc_utf8_text_node( const BYTE *data, ULONG len, WS_XML_UTF8_TEXT **ret )
{
    struct node *node;
    WS_XML_UTF8_TEXT *utf8;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(utf8 = alloc_utf8_text( data, len )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &utf8->text;
    if (ret) *ret = utf8;
    return node;
}

static struct node *alloc_base64_text_node( const BYTE *data, ULONG len, WS_XML_BASE64_TEXT **ret )
{
    struct node *node;
    WS_XML_BASE64_TEXT *base64;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(base64 = alloc_base64_text( data, len )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &base64->text;
    if (ret) *ret = base64;
    return node;
}

static struct node *alloc_bool_text_node( BOOL value )
{
    struct node *node;
    WS_XML_BOOL_TEXT *text_bool;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_bool = alloc_bool_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_bool->text;
    return node;
}

static struct node *alloc_int32_text_node( INT32 value )
{
    struct node *node;
    WS_XML_INT32_TEXT *text_int32;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_int32 = alloc_int32_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_int32->text;
    return node;
}

static struct node *alloc_int64_text_node( INT64 value )
{
    struct node *node;
    WS_XML_INT64_TEXT *text_int64;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_int64 = alloc_int64_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_int64->text;
    return node;
}

static struct node *alloc_float_text_node( float value )
{
    struct node *node;
    WS_XML_FLOAT_TEXT *text_float;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_float = alloc_float_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_float->text;
    return node;
}

static struct node *alloc_double_text_node( double value )
{
    struct node *node;
    WS_XML_DOUBLE_TEXT *text_double;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_double = alloc_double_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_double->text;
    return node;
}

static struct node *alloc_datetime_text_node( const WS_DATETIME *value )
{
    struct node *node;
    WS_XML_DATETIME_TEXT *text_datetime;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_datetime = alloc_datetime_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_datetime->text;
    return node;
}

static struct node *alloc_unique_id_text_node( const GUID *value )
{
    struct node *node;
    WS_XML_UNIQUE_ID_TEXT *text_unique_id;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_unique_id = alloc_unique_id_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_unique_id->text;
    return node;
}

static struct node *alloc_guid_text_node( const GUID *value )
{
    struct node *node;
    WS_XML_GUID_TEXT *text_guid;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_guid = alloc_guid_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_guid->text;
    return node;
}

static struct node *alloc_uint64_text_node( UINT64 value )
{
    struct node *node;
    WS_XML_UINT64_TEXT *text_uint64;
    WS_XML_TEXT_NODE *text;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    if (!(text_uint64 = alloc_uint64_text( value )))
    {
        free( node );
        return NULL;
    }
    text = (WS_XML_TEXT_NODE *)node;
    text->text = &text_uint64->text;
    return node;
}

static HRESULT append_text_bytes( struct reader *reader, WS_XML_TEXT_NODE *node, ULONG len )
{
    WS_XML_BASE64_TEXT *new, *old = (WS_XML_BASE64_TEXT *)node->text;
    HRESULT hr;

    if (!(new = alloc_base64_text( NULL, old->length + len ))) return E_OUTOFMEMORY;
    memcpy( new->bytes, old->bytes, old->length );
    if ((hr = read_bytes( reader, new->bytes + old->length, len )) != S_OK) return hr;
    free( old );
    node->text = &new->text;
    return S_OK;
}

static HRESULT read_text_bytes( struct reader *reader, unsigned char type )
{
    struct node *node = NULL, *parent;
    WS_XML_BASE64_TEXT *base64;
    HRESULT hr;
    ULONG len;

    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;
    for (;;)
    {
        switch (type)
        {
        case RECORD_BYTES8_TEXT:
        case RECORD_BYTES8_TEXT_WITH_ENDELEMENT:
        {
            UINT8 len_uint8;
            if ((hr = read_byte( reader, (unsigned char *)&len_uint8 )) != S_OK) goto error;
            len = len_uint8;
            break;
        }
        case RECORD_BYTES16_TEXT:
        case RECORD_BYTES16_TEXT_WITH_ENDELEMENT:
        {
            UINT16 len_uint16;
            if ((hr = read_bytes( reader, (unsigned char *)&len_uint16, sizeof(len_uint16) )) != S_OK) goto error;
            len = len_uint16;
            break;
        }
        case RECORD_BYTES32_TEXT:
        case RECORD_BYTES32_TEXT_WITH_ENDELEMENT:
        {
            INT32 len_int32;
            if ((hr = read_bytes( reader, (unsigned char *)&len_int32, sizeof(len_int32) )) != S_OK) goto error;
            if (len_int32 < 0)
            {
                hr = WS_E_INVALID_FORMAT;
                goto error;
            }
            len = len_int32;
            break;
        }
        default:
            ERR( "unexpected type %u\n", type );
            hr = E_INVALIDARG;
            goto error;
        }

        if (!node)
        {
            if (!(node = alloc_base64_text_node( NULL, len, &base64 ))) return E_OUTOFMEMORY;
            if ((hr = read_bytes( reader, base64->bytes, len )) != S_OK) goto error;
        }
        else if ((hr = append_text_bytes( reader, (WS_XML_TEXT_NODE *)node, len )) != S_OK) goto error;

        if (type & 1)
        {
            node->flags |= NODE_FLAG_TEXT_WITH_IMPLICIT_END_ELEMENT;
            break;
        }
        if ((hr = read_peek( reader, &type, 1 )) != S_OK) goto error;
        if (type < RECORD_BYTES8_TEXT || type > RECORD_BYTES32_TEXT_WITH_ENDELEMENT) break;
        read_skip( reader, 1 );
    }

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_TEXT;
    reader->text_conv_offset = 0;
    return S_OK;

error:
    free_node( node );
    return hr;
}

static HRESULT read_text_bin( struct reader *reader )
{
    struct node *node = NULL, *parent;
    unsigned char type;
    WS_XML_UTF8_TEXT *utf8;
    INT32 val_int32;
    UINT8 val_uint8;
    UINT16 val_uint16;
    ULONG len, id;
    GUID uuid;
    HRESULT hr;

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (!is_text_type( type ) || !(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;
    read_skip( reader, 1 );

    switch (type)
    {
    case RECORD_ZERO_TEXT:
    case RECORD_ZERO_TEXT_WITH_ENDELEMENT:
        if (!(node = alloc_int32_text_node( 0 ))) return E_OUTOFMEMORY;
        break;

    case RECORD_ONE_TEXT:
    case RECORD_ONE_TEXT_WITH_ENDELEMENT:
        if (!(node = alloc_int32_text_node( 1 ))) return E_OUTOFMEMORY;
        break;

    case RECORD_FALSE_TEXT:
    case RECORD_FALSE_TEXT_WITH_ENDELEMENT:
        if (!(node = alloc_bool_text_node( FALSE ))) return E_OUTOFMEMORY;
        break;

    case RECORD_TRUE_TEXT:
    case RECORD_TRUE_TEXT_WITH_ENDELEMENT:
        if (!(node = alloc_bool_text_node( TRUE ))) return E_OUTOFMEMORY;
        break;

    case RECORD_INT8_TEXT:
    case RECORD_INT8_TEXT_WITH_ENDELEMENT:
    {
        INT8 val_int8;
        if ((hr = read_byte( reader, (unsigned char *)&val_int8 )) != S_OK) return hr;
        if (!(node = alloc_int32_text_node( val_int8 ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_INT16_TEXT:
    case RECORD_INT16_TEXT_WITH_ENDELEMENT:
    {
        INT16 val_int16;
        if ((hr = read_bytes( reader, (unsigned char *)&val_int16, sizeof(val_int16) )) != S_OK) return hr;
        if (!(node = alloc_int32_text_node( val_int16 ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_INT32_TEXT:
    case RECORD_INT32_TEXT_WITH_ENDELEMENT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_int32, sizeof(val_int32) )) != S_OK) return hr;
        if (!(node = alloc_int32_text_node( val_int32 ))) return E_OUTOFMEMORY;
        break;

    case RECORD_INT64_TEXT:
    case RECORD_INT64_TEXT_WITH_ENDELEMENT:
    {
        INT64 val_int64;
        if ((hr = read_bytes( reader, (unsigned char *)&val_int64, sizeof(val_int64) )) != S_OK) return hr;
        if (!(node = alloc_int64_text_node( val_int64 ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_FLOAT_TEXT:
    case RECORD_FLOAT_TEXT_WITH_ENDELEMENT:
    {
        float val_float;
        if ((hr = read_bytes( reader, (unsigned char *)&val_float, sizeof(val_float) )) != S_OK) return hr;
        if (!(node = alloc_float_text_node( val_float ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_DOUBLE_TEXT:
    case RECORD_DOUBLE_TEXT_WITH_ENDELEMENT:
    {
        double val_double;
        if ((hr = read_bytes( reader, (unsigned char *)&val_double, sizeof(val_double) )) != S_OK) return hr;
        if (!(node = alloc_double_text_node( val_double ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_DATETIME_TEXT:
    case RECORD_DATETIME_TEXT_WITH_ENDELEMENT:
    {
        WS_DATETIME datetime;
        if ((hr = read_datetime( reader, &datetime )) != S_OK) return hr;
        if (!(node = alloc_datetime_text_node( &datetime ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_CHARS8_TEXT:
    case RECORD_CHARS8_TEXT_WITH_ENDELEMENT:
        if ((hr = read_byte( reader, (unsigned char *)&val_uint8 )) != S_OK) return hr;
        len = val_uint8;
        break;

    case RECORD_CHARS16_TEXT:
    case RECORD_CHARS16_TEXT_WITH_ENDELEMENT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_uint16, sizeof(val_uint16) )) != S_OK) return hr;
        len = val_uint16;
        break;

    case RECORD_CHARS32_TEXT:
    case RECORD_CHARS32_TEXT_WITH_ENDELEMENT:
        if ((hr = read_bytes( reader, (unsigned char *)&val_int32, sizeof(val_int32) )) != S_OK) return hr;
        if (val_int32 < 0) return WS_E_INVALID_FORMAT;
        len = val_int32;
        break;

    case RECORD_BYTES8_TEXT:
    case RECORD_BYTES8_TEXT_WITH_ENDELEMENT:
    case RECORD_BYTES16_TEXT:
    case RECORD_BYTES16_TEXT_WITH_ENDELEMENT:
    case RECORD_BYTES32_TEXT:
    case RECORD_BYTES32_TEXT_WITH_ENDELEMENT:
        return read_text_bytes( reader, type );

    case RECORD_EMPTY_TEXT:
    case RECORD_EMPTY_TEXT_WITH_ENDELEMENT:
        len = 0;
        break;

    case RECORD_DICTIONARY_TEXT:
    case RECORD_DICTIONARY_TEXT_WITH_ENDELEMENT:
    {
        const WS_XML_STRING *str;
        if ((hr = read_int31( reader, &id )) != S_OK) return hr;
        if ((hr = lookup_string( reader, id, &str )) != S_OK) return hr;
        if (!(node = alloc_utf8_text_node( str->bytes, str->length, NULL ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_UNIQUE_ID_TEXT:
    case RECORD_UNIQUE_ID_TEXT_WITH_ENDELEMENT:
        if ((hr = read_bytes( reader, (unsigned char *)&uuid, sizeof(uuid) )) != S_OK) return hr;
        if (!(node = alloc_unique_id_text_node( &uuid ))) return E_OUTOFMEMORY;
        break;

    case RECORD_GUID_TEXT:
    case RECORD_GUID_TEXT_WITH_ENDELEMENT:
        if ((hr = read_bytes( reader, (unsigned char *)&uuid, sizeof(uuid) )) != S_OK) return hr;
        if (!(node = alloc_guid_text_node( &uuid ))) return E_OUTOFMEMORY;
        break;

    case RECORD_UINT64_TEXT:
    case RECORD_UINT64_TEXT_WITH_ENDELEMENT:
    {
        UINT64 val_uint64;
        if ((hr = read_bytes( reader, (unsigned char *)&val_uint64, sizeof(val_uint64) )) != S_OK) return hr;
        if (!(node = alloc_uint64_text_node( val_uint64 ))) return E_OUTOFMEMORY;
        break;
    }
    case RECORD_BOOL_TEXT:
    case RECORD_BOOL_TEXT_WITH_ENDELEMENT:
    {
        BOOL val_bool;
        if ((hr = read_bytes( reader, (unsigned char *)&val_bool, sizeof(val_bool) )) != S_OK) return hr;
        if (!(node = alloc_bool_text_node( !!val_bool ))) return E_OUTOFMEMORY;
        break;
    }
    default:
        ERR( "unhandled record type %02x\n", type );
        return WS_E_NOT_SUPPORTED;
    }

    if (!node)
    {
        if (!(node = alloc_utf8_text_node( NULL, len, &utf8 ))) return E_OUTOFMEMORY;
        if (!len) utf8->value.bytes = (BYTE *)(utf8 + 1); /* quirk */
        if ((hr = read_bytes( reader, utf8->value.bytes, len )) != S_OK)
        {
            free_node( node );
            return hr;
        }
    }

    if (type & 1) node->flags |= NODE_FLAG_TEXT_WITH_IMPLICIT_END_ELEMENT;
    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_TEXT;
    reader->text_conv_offset = 0;
    return S_OK;
}

static HRESULT read_node_text( struct reader * );

static HRESULT read_startelement_text( struct reader *reader )
{
    HRESULT hr;

    if (read_cmp( reader, "<?", 2 ) == S_OK)
    {
        if ((hr = read_xmldecl( reader )) != S_OK) return hr;
    }
    read_skip_whitespace( reader );
    if (read_cmp( reader, "<", 1 ) == S_OK)
    {
        if ((hr = read_element_text( reader )) != S_OK) return hr;
    }
    if (read_cmp( reader, "/>", 2 ) == S_OK)
    {
        read_skip( reader, 2 );
        reader->current = LIST_ENTRY( list_tail( &reader->current->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_ENDELEMENT;
        return S_OK;
    }
    else if (read_cmp( reader, ">", 1 ) == S_OK)
    {
        read_skip( reader, 1 );
        return read_node_text( reader );
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT read_node_bin( struct reader * );

static HRESULT read_startelement_bin( struct reader *reader )
{
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT) return WS_E_INVALID_FORMAT;
    return read_node_bin( reader );
}

static HRESULT read_startelement( struct reader *reader )
{
    switch (reader->input_enc)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:   return read_startelement_text( reader );
    case WS_XML_READER_ENCODING_TYPE_BINARY: return read_startelement_bin( reader );
    default:
        ERR( "unhandled encoding %u\n", reader->input_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static HRESULT read_to_startelement_text( struct reader *reader, BOOL *found )
{
    HRESULT hr;

    switch (reader->state)
    {
    case READER_STATE_INITIAL:
        if ((hr = read_xmldecl( reader )) != S_OK) return hr;
        break;

    case READER_STATE_STARTELEMENT:
        if (found) *found = TRUE;
        return S_OK;

    default:
        break;
    }

    read_skip_whitespace( reader );
    if ((hr = read_element_text( reader )) == S_OK && found)
    {
        if (reader->state == READER_STATE_STARTELEMENT)
            *found = TRUE;
        else
            *found = FALSE;
    }

    return hr;
}

static HRESULT read_to_startelement_bin( struct reader *reader, BOOL *found )
{
    HRESULT hr;

    if (reader->state == READER_STATE_STARTELEMENT)
    {
        if (found) *found = TRUE;
        return S_OK;
    }

    if ((hr = read_element_bin( reader )) == S_OK && found)
    {
        if (reader->state == READER_STATE_STARTELEMENT)
            *found = TRUE;
        else
            *found = FALSE;
    }

    return hr;
}

static HRESULT read_to_startelement( struct reader *reader, BOOL *found )
{
    switch (reader->input_enc)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:   return read_to_startelement_text( reader, found );
    case WS_XML_READER_ENCODING_TYPE_BINARY: return read_to_startelement_bin( reader, found );
    default:
        ERR( "unhandled encoding %u\n", reader->input_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static int cmp_name( const unsigned char *name1, ULONG len1, const unsigned char *name2, ULONG len2 )
{
    ULONG i;
    if (len1 != len2) return 1;
    for (i = 0; i < len1; i++) { if (toupper( name1[i] ) != toupper( name2[i] )) return 1; }
    return 0;
}

static struct node *find_startelement( struct reader *reader, const WS_XML_STRING *prefix,
                                       const WS_XML_STRING *localname )
{
    struct node *parent;
    const WS_XML_STRING *str;

    for (parent = reader->current; parent; parent = parent->parent)
    {
        if (node_type( parent ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            str = parent->hdr.prefix;
            if (cmp_name( str->bytes, str->length, prefix->bytes, prefix->length )) continue;
            str = parent->hdr.localName;
            if (cmp_name( str->bytes, str->length, localname->bytes, localname->length )) continue;
            return parent;
       }
    }
    return NULL;
}

static HRESULT read_endelement_text( struct reader *reader )
{
    struct node *parent;
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    WS_XML_STRING prefix, localname;
    HRESULT hr;

    if ((hr = read_cmp( reader, "</", 2 )) != S_OK) return hr;
    read_skip( reader, 2 );

    start = read_current_ptr( reader );
    for (;;)
    {
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) return hr;
        if (ch == '>')
        {
            read_skip( reader, 1 );
            break;
        }
        if (!read_isnamechar( ch )) return WS_E_INVALID_FORMAT;
        read_skip( reader, skip );
        len += skip;
    }

    if ((hr = split_qname( start, len, &prefix, &localname )) != S_OK) return hr;
    if (!(parent = find_startelement( reader, &prefix, &localname ))) return WS_E_INVALID_FORMAT;

    reader->current = LIST_ENTRY( list_tail( &parent->children ), struct node, entry );
    reader->last    = reader->current;
    reader->state   = READER_STATE_ENDELEMENT;
    return S_OK;
}

static HRESULT read_endelement_bin( struct reader *reader )
{
    struct node *parent;
    unsigned char type;
    HRESULT hr;

    if (!(reader->current->flags & NODE_FLAG_TEXT_WITH_IMPLICIT_END_ELEMENT))
    {
        if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
        if (type != RECORD_ENDELEMENT) return WS_E_INVALID_FORMAT;
        read_skip( reader, 1 );
    }
    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;

    reader->current = LIST_ENTRY( list_tail( &parent->children ), struct node, entry );
    reader->last    = reader->current;
    reader->state   = READER_STATE_ENDELEMENT;
    return S_OK;
}

static HRESULT read_endelement( struct reader *reader )
{
    if (reader->state == READER_STATE_EOF) return WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    switch (reader->input_enc)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:   return read_endelement_text( reader );
    case WS_XML_READER_ENCODING_TYPE_BINARY: return read_endelement_bin( reader );
    default:
        ERR( "unhandled encoding %u\n", reader->input_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

static HRESULT read_comment_text( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node, *parent;
    WS_XML_COMMENT_NODE *comment;
    HRESULT hr;

    if ((hr = read_cmp( reader, "<!--", 4 )) != S_OK) return hr;
    read_skip( reader, 4 );

    start = read_current_ptr( reader );
    for (;;)
    {
        if (read_cmp( reader, "-->", 3 ) == S_OK)
        {
            read_skip( reader, 3 );
            break;
        }
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) return hr;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;
    if (!(comment->value.bytes = malloc( len )))
    {
        free( node );
        return E_OUTOFMEMORY;
    }
    memcpy( comment->value.bytes, start, len );
    comment->value.length = len;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_COMMENT;
    return S_OK;
}

static HRESULT read_comment_bin( struct reader *reader )
{
    struct node *node, *parent;
    WS_XML_COMMENT_NODE *comment;
    unsigned char type;
    ULONG len;
    HRESULT hr;

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (type != RECORD_COMMENT) return WS_E_INVALID_FORMAT;
    read_skip( reader, 1 );
    if ((hr = read_int31( reader, &len )) != S_OK) return hr;

    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;
    if (!(comment->value.bytes = malloc( len )))
    {
        free( node );
        return E_OUTOFMEMORY;
    }
    if ((hr = read_bytes( reader, comment->value.bytes, len )) != S_OK)
    {
        free_node( node );
        return E_OUTOFMEMORY;
    }
    comment->value.length = len;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_COMMENT;
    return S_OK;
}

static HRESULT read_startcdata( struct reader *reader )
{
    struct node *node, *endnode, *parent;
    HRESULT hr;

    if ((hr = read_cmp( reader, "<![CDATA[", 9 )) != S_OK) return hr;
    read_skip( reader, 9 );

    if (!(parent = find_parent( reader ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_CDATA ))) return E_OUTOFMEMORY;
    if (!(endnode = alloc_node( WS_XML_NODE_TYPE_END_CDATA )))
    {
        free( node );
        return E_OUTOFMEMORY;
    }
    list_add_tail( &node->children, &endnode->entry );
    endnode->parent = node;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_STARTCDATA;
    return S_OK;
}

static HRESULT read_cdata( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node;
    WS_XML_TEXT_NODE *text;
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;

    start = read_current_ptr( reader );
    for (;;)
    {
        if (read_cmp( reader, "]]>", 3 ) == S_OK) break;
        if ((hr = read_utf8_char( reader, &ch, &skip )) != S_OK) return hr;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;
    if (!(utf8 = alloc_utf8_text( start, len )))
    {
        free( node );
        return E_OUTOFMEMORY;
    }
    text->text = &utf8->text;

    read_insert_node( reader, reader->current, node );
    reader->state = READER_STATE_CDATA;
    return S_OK;
}

static HRESULT read_endcdata( struct reader *reader )
{
    struct node *parent;
    HRESULT hr;

    if ((hr = read_cmp( reader, "]]>", 3 )) != S_OK) return hr;
    read_skip( reader, 3 );

    if (node_type( reader->current ) == WS_XML_NODE_TYPE_TEXT) parent = reader->current->parent;
    else parent = reader->current;

    reader->current = LIST_ENTRY( list_tail( &parent->children ), struct node, entry );
    reader->last    = reader->current;
    reader->state   = READER_STATE_ENDCDATA;
    return S_OK;
}

static HRESULT read_node_text( struct reader *reader )
{
    HRESULT hr;

    for (;;)
    {
        if (read_end_of_data( reader ))
        {
            reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
            reader->last    = reader->current;
            reader->state   = READER_STATE_EOF;
            return S_OK;
        }
        if (reader->state == READER_STATE_STARTCDATA) return read_cdata( reader );
        else if (reader->state == READER_STATE_CDATA) return read_endcdata( reader );
        else if (read_cmp( reader, "<?", 2 ) == S_OK)
        {
            if ((hr = read_xmldecl( reader )) != S_OK) return hr;
        }
        else if (read_cmp( reader, "</", 2 ) == S_OK) return read_endelement_text( reader );
        else if (read_cmp( reader, "<![CDATA[", 9 ) == S_OK) return read_startcdata( reader );
        else if (read_cmp( reader, "<!--", 4 ) == S_OK) return read_comment_text( reader );
        else if (read_cmp( reader, "<", 1 ) == S_OK) return read_element_text( reader );
        else if (read_cmp( reader, "/>", 2 ) == S_OK || read_cmp( reader, ">", 1 ) == S_OK)
        {
            return read_startelement_text( reader );
        }
        else return read_text_text( reader );
    }
}

static HRESULT read_node_bin( struct reader *reader )
{
    unsigned char type;
    HRESULT hr;

    if (reader->current->flags & NODE_FLAG_TEXT_WITH_IMPLICIT_END_ELEMENT)
    {
        reader->current = LIST_ENTRY( list_tail( &reader->current->parent->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_ENDELEMENT;
        return S_OK;
    }
    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    if ((hr = read_peek( reader, &type, 1 )) != S_OK) return hr;
    if (type == RECORD_ENDELEMENT)
    {
        return read_endelement_bin( reader );
    }
    else if (type == RECORD_COMMENT)
    {
        return read_comment_bin( reader );
    }
    else if (type >= RECORD_SHORT_ELEMENT && type <= RECORD_PREFIX_ELEMENT_Z)
    {
        return read_element_bin( reader );
    }
    else if (type >= RECORD_ZERO_TEXT && type <= RECORD_QNAME_DICTIONARY_TEXT_WITH_ENDELEMENT)
    {
        return read_text_bin( reader );
    }
    FIXME( "unhandled record type %02x\n", type );
    return WS_E_NOT_SUPPORTED;
}

static HRESULT read_node( struct reader *reader )
{
    switch (reader->input_enc)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:   return read_node_text( reader );
    case WS_XML_READER_ENCODING_TYPE_BINARY: return read_node_bin( reader );
    default:
        ERR( "unhandled encoding %u\n", reader->input_enc );
        return WS_E_NOT_SUPPORTED;
    }
}

HRESULT copy_node( WS_XML_READER *handle, WS_XML_WRITER_ENCODING_TYPE enc, struct node **node )
{
    struct reader *reader = (struct reader *)handle;
    const struct list *ptr;
    const struct node *start;
    HRESULT hr;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (reader->current != reader->root) ptr = &reader->current->entry;
    else /* copy whole tree */
    {
        if (!read_end_of_data( reader ))
        {
            for (;;)
            {
                if ((hr = read_node( reader )) != S_OK) goto done;
                if (node_type( reader->current ) == WS_XML_NODE_TYPE_EOF) break;
            }
        }
        ptr = list_head( &reader->root->children );
    }

    start = LIST_ENTRY( ptr, struct node, entry );
    if (node_type( start ) == WS_XML_NODE_TYPE_EOF) hr = WS_E_INVALID_OPERATION;
    else hr = dup_tree( start, enc, node );

done:
    LeaveCriticalSection( &reader->cs );
    return hr;
}

/**************************************************************************
 *          WsReadEndElement		[webservices.@]
 */
HRESULT WINAPI WsReadEndElement( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_endelement( reader );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadNode		[webservices.@]
 */
HRESULT WINAPI WsReadNode( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_node( reader );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT skip_node( struct reader *reader )
{
    const struct node *parent;
    HRESULT hr;

    if (node_type( reader->current ) == WS_XML_NODE_TYPE_EOF) return WS_E_INVALID_OPERATION;
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_ELEMENT) parent = reader->current;
    else parent = NULL;

    for (;;)
    {
        if ((hr = read_node( reader )) != S_OK || !parent) break;
        if (node_type( reader->current ) != WS_XML_NODE_TYPE_END_ELEMENT) continue;
        if (reader->current->parent == parent) return read_node( reader );
    }

    return hr;
}

/**************************************************************************
 *          WsSkipNode		[webservices.@]
 */
HRESULT WINAPI WsSkipNode( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = skip_node( reader );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadStartElement		[webservices.@]
 */
HRESULT WINAPI WsReadStartElement( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_startelement( reader );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadToStartElement		[webservices.@]
 */
HRESULT WINAPI WsReadToStartElement( WS_XML_READER *handle, const WS_XML_STRING *localname,
                                     const WS_XML_STRING *ns, BOOL *found, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %s %s %p %p\n", handle, debugstr_xmlstr(localname), debugstr_xmlstr(ns), found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    if (localname || ns) FIXME( "name and/or namespace not verified\n" );

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_to_startelement( reader, found );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

BOOL move_to_root_element( struct node *root, struct node **current )
{
    struct list *ptr;
    struct node *node;

    if (!(ptr = list_head( &root->children ))) return FALSE;
    node = LIST_ENTRY( ptr, struct node, entry );
    if (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        *current = node;
        return TRUE;
    }
    while ((ptr = list_next( &root->children, &node->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            *current = next;
            return TRUE;
        }
        node = next;
    }
    return FALSE;
}

BOOL move_to_next_element( struct node **current )
{
    struct list *ptr;
    struct node *node = *current, *parent = (*current)->parent;

    if (!parent) return FALSE;
    while ((ptr = list_next( &parent->children, &node->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            *current = next;
            return TRUE;
        }
        node = next;
    }
    return FALSE;
}

BOOL move_to_prev_element( struct node **current )
{
    struct list *ptr;
    struct node *node = *current, *parent = (*current)->parent;

    if (!parent) return FALSE;
    while ((ptr = list_prev( &parent->children, &node->entry )))
    {
        struct node *prev = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( prev ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            *current = prev;
            return TRUE;
        }
        node = prev;
    }
    return FALSE;
}

BOOL move_to_child_element( struct node **current )
{
    struct list *ptr;
    struct node *child, *node = *current;

    if (!(ptr = list_head( &node->children ))) return FALSE;
    child = LIST_ENTRY( ptr, struct node, entry );
    if (node_type( child ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        *current = child;
        return TRUE;
    }
    while ((ptr = list_next( &node->children, &child->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            *current = next;
            return TRUE;
        }
        child = next;
    }
    return FALSE;
}

BOOL move_to_end_element( struct node **current )
{
    struct list *ptr;
    struct node *node = *current;

    if (node_type( node ) != WS_XML_NODE_TYPE_ELEMENT) return FALSE;

    if ((ptr = list_tail( &node->children )))
    {
        struct node *tail = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( tail ) == WS_XML_NODE_TYPE_END_ELEMENT)
        {
            *current = tail;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL move_to_parent_element( struct node **current )
{
    struct node *parent = (*current)->parent;

    if (parent && (node_type( parent ) == WS_XML_NODE_TYPE_ELEMENT ||
                   node_type( parent ) == WS_XML_NODE_TYPE_BOF))
    {
        *current = parent;
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_first_node( struct node **current )
{
    struct list *ptr;
    struct node *node = *current;

    if ((ptr = list_head( &node->parent->children )))
    {
        *current = LIST_ENTRY( ptr, struct node, entry );
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_next_node( struct node **current )
{
    struct list *ptr;
    struct node *node = *current;

    if ((ptr = list_next( &node->parent->children, &node->entry )))
    {
        *current = LIST_ENTRY( ptr, struct node, entry );
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_prev_node( struct node **current )
{
    struct list *ptr;
    struct node *node = *current;

    if ((ptr = list_prev( &node->parent->children, &node->entry )))
    {
        *current = LIST_ENTRY( ptr, struct node, entry );
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_bof( struct node *root, struct node **current )
{
    *current = root;
    return TRUE;
}

BOOL move_to_eof( struct node *root, struct node **current )
{
    struct list *ptr;
    if ((ptr = list_tail( &root->children )))
    {
        *current = LIST_ENTRY( ptr, struct node, entry );
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_child_node( struct node **current )
{
    struct list *ptr;
    struct node *node = *current;

    if ((ptr = list_head( &node->children )))
    {
        *current = LIST_ENTRY( ptr, struct node, entry );
        return TRUE;
    }
    return FALSE;
}

BOOL move_to_parent_node( struct node **current )
{
    struct node *parent = (*current)->parent;
    if (!parent) return FALSE;
    *current = parent;
    return TRUE;
}

static HRESULT read_move_to( struct reader *reader, WS_MOVE_TO move, BOOL *found )
{
    BOOL success = FALSE;
    HRESULT hr = S_OK;

    if (!read_end_of_data( reader ))
    {
        struct node *saved_current = reader->current;
        while (reader->state != READER_STATE_EOF && (hr = read_node( reader )) == S_OK) { /* nothing */ };
        if (hr != S_OK) return hr;
        reader->current = saved_current;
    }
    switch (move)
    {
    case WS_MOVE_TO_ROOT_ELEMENT:
        success = move_to_root_element( reader->root, &reader->current );
        break;

    case WS_MOVE_TO_NEXT_ELEMENT:
        success = move_to_next_element( &reader->current );
        break;

    case WS_MOVE_TO_PREVIOUS_ELEMENT:
        success = move_to_prev_element( &reader->current );
        break;

    case WS_MOVE_TO_CHILD_ELEMENT:
        success = move_to_child_element( &reader->current );
        break;

    case WS_MOVE_TO_END_ELEMENT:
        success = move_to_end_element( &reader->current );
        break;

    case WS_MOVE_TO_PARENT_ELEMENT:
        success = move_to_parent_element( &reader->current );
        break;

    case WS_MOVE_TO_FIRST_NODE:
        success = move_to_first_node( &reader->current );
        break;

    case WS_MOVE_TO_NEXT_NODE:
        success = move_to_next_node( &reader->current );
        break;

    case WS_MOVE_TO_PREVIOUS_NODE:
        success = move_to_prev_node( &reader->current );
        break;

    case WS_MOVE_TO_CHILD_NODE:
        success = move_to_child_node( &reader->current );
        break;

    case WS_MOVE_TO_BOF:
        success = move_to_bof( reader->root, &reader->current );
        break;

    case WS_MOVE_TO_EOF:
        success = move_to_eof( reader->root, &reader->current );
        break;

    default:
        FIXME( "unhandled move %u\n", move );
        return E_NOTIMPL;
    }

    if (found)
    {
        *found = success;
        return S_OK;
    }
    return success ? S_OK : WS_E_INVALID_FORMAT;
}

/**************************************************************************
 *          WsMoveReader		[webservices.@]
 */
HRESULT WINAPI WsMoveReader( WS_XML_READER *handle, WS_MOVE_TO move, BOOL *found, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", handle, move, found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (reader->input_type != WS_XML_READER_INPUT_TYPE_BUFFER) hr = WS_E_INVALID_OPERATION;
    else hr = read_move_to( reader, move, found );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadStartAttribute		[webservices.@]
 */
HRESULT WINAPI WsReadStartAttribute( WS_XML_READER *handle, ULONG index, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    const WS_XML_ELEMENT_NODE *elem;
    HRESULT hr = S_OK;

    TRACE( "%p %lu %p\n", handle, index, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    elem = &reader->current->hdr;
    if (reader->state != READER_STATE_STARTELEMENT || index >= elem->attributeCount) hr = WS_E_INVALID_FORMAT;
    else
    {
        reader->current_attr = index;
        reader->state        = READER_STATE_STARTATTRIBUTE;
    }

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return S_OK;
}

/**************************************************************************
 *          WsReadEndAttribute		[webservices.@]
 */
HRESULT WINAPI WsReadEndAttribute( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (reader->state != READER_STATE_STARTATTRIBUTE) hr = WS_E_INVALID_FORMAT;
    else reader->state = READER_STATE_STARTELEMENT;

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT str_to_bool( const unsigned char *str, ULONG len, BOOL *ret )
{
    if (len == 4 && !memcmp( str, "true", 4 )) *ret = TRUE;
    else if (len == 1 && !memcmp( str, "1", 1 )) *ret = TRUE;
    else if (len == 5 && !memcmp( str, "false", 5 )) *ret = FALSE;
    else if (len == 1 && !memcmp( str, "0", 1 )) *ret = FALSE;
    else return WS_E_INVALID_FORMAT;
    return S_OK;
}

static HRESULT str_to_int64( const unsigned char *str, ULONG len, INT64 min, INT64 max, INT64 *ret )
{
    BOOL negative = FALSE;
    const unsigned char *ptr = str;

    *ret = 0;
    while (len && read_isspace( *ptr )) { ptr++; len--; }
    while (len && read_isspace( ptr[len - 1] )) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    if (*ptr == '-')
    {
        negative = TRUE;
        ptr++;
        len--;
    }
    if (!len) return WS_E_INVALID_FORMAT;

    while (len--)
    {
        int val;

        if (!isdigit( *ptr )) return WS_E_INVALID_FORMAT;
        val = *ptr - '0';
        if (negative) val = -val;

        if ((!negative && (*ret > max / 10 || *ret * 10 > max - val)) ||
            (negative && (*ret < min / 10 || *ret * 10 < min - val)))
        {
            return WS_E_NUMERIC_OVERFLOW;
        }
        *ret = *ret * 10 + val;
        ptr++;
    }

    return S_OK;
}

static HRESULT str_to_uint64( const unsigned char *str, ULONG len, UINT64 max, UINT64 *ret )
{
    const unsigned char *ptr = str;

    *ret = 0;
    while (len && read_isspace( *ptr )) { ptr++; len--; }
    while (len && read_isspace( ptr[len - 1] )) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    while (len--)
    {
        unsigned int val;

        if (!isdigit( *ptr )) return WS_E_INVALID_FORMAT;
        val = *ptr - '0';

        if ((*ret > max / 10 || *ret * 10 > max - val)) return WS_E_NUMERIC_OVERFLOW;
        *ret = *ret * 10 + val;
        ptr++;
    }

    return S_OK;
}

static HRESULT str_to_double( const unsigned char *str, ULONG len, double *ret )
{
    BOOL found_sign = FALSE, found_exponent = FALSE, found_digit = FALSE, found_decimal = FALSE;
    static const unsigned __int64 nan = 0xfff8000000000000;
    static const unsigned __int64 inf = 0x7ff0000000000000;
    static const unsigned __int64 inf_min = 0xfff0000000000000;
    const char *p = (const char *)str;
    double tmp;
    ULONG i;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    if (len == 3 && !memcmp( p, "NaN", 3 ))
    {
        *(unsigned __int64 *)ret = nan;
        return S_OK;
    }
    if (len == 3 && !memcmp( p, "INF", 3 ))
    {
        *(unsigned __int64 *)ret = inf;
        return S_OK;
    }
    if (len == 4 && !memcmp( p, "-INF", 4 ))
    {
        *(unsigned __int64 *)ret = inf_min;
        return S_OK;
    }

    for (i = 0; i < len; i++)
    {
        if (p[i] >= '0' && p[i] <= '9')
        {
            found_digit = TRUE;
            continue;
        }
        if (!found_sign && !found_digit && (p[i] == '+' || p[i] == '-'))
        {
            found_sign = TRUE;
            continue;
        }
        if (!found_exponent && found_digit && (p[i] == 'e' || p[i] == 'E'))
        {
            found_exponent = found_decimal = TRUE;
            found_digit = found_sign = FALSE;
            continue;
        }
        if (!found_decimal && p[i] == '.')
        {
            found_decimal = TRUE;
            continue;
        }
        return WS_E_INVALID_FORMAT;
    }
    if (!found_digit && !found_exponent)
    {
        *ret = 0;
        return S_OK;
    }

    if (_snscanf_l( p, len, "%lf", c_locale, &tmp ) != 1) return WS_E_INVALID_FORMAT;
    *ret = tmp;
    return S_OK;
}

static HRESULT str_to_float( const unsigned char *str, ULONG len, float *ret )
{
    static const unsigned int inf = 0x7f800000;
    static const unsigned int inf_min = 0xff800000;
    const unsigned char *p = str;
    double val;
    HRESULT hr;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    if (len == 3 && !memcmp( p, "INF", 3 ))
    {
        *(unsigned int *)ret = inf;
        return S_OK;
    }
    if (len == 4 && !memcmp( p, "-INF", 4 ))
    {
        *(unsigned int *)ret = inf_min;
        return S_OK;
    }

    if ((hr = str_to_double( p, len, &val )) != S_OK) return hr;
    *ret = val;
    return S_OK;
}

HRESULT str_to_guid( const unsigned char *str, ULONG len, GUID *ret )
{
    static const unsigned char hex[] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x00 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x10 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x20 */
        0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,        /* 0x30 */
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,  /* 0x40 */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x50 */
        0,10,11,12,13,14,15                     /* 0x60 */
    };
    const unsigned char *p = str;
    ULONG i;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }
    if (len != 36) return WS_E_INVALID_FORMAT;

    if (p[8] != '-' || p[13] != '-' || p[18] != '-' || p[23] != '-')
        return WS_E_INVALID_FORMAT;

    for (i = 0; i < 36; i++)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23) continue;
        if (p[i] > 'f' || (!hex[p[i]] && p[i] != '0')) return WS_E_INVALID_FORMAT;
    }

    ret->Data1 = hex[p[0]] << 28 | hex[p[1]] << 24 | hex[p[2]] << 20 | hex[p[3]] << 16 |
                 hex[p[4]] << 12 | hex[p[5]] << 8  | hex[p[6]] << 4  | hex[p[7]];

    ret->Data2 = hex[p[9]]  << 12 | hex[p[10]] << 8 | hex[p[11]] << 4 | hex[p[12]];
    ret->Data3 = hex[p[14]] << 12 | hex[p[15]] << 8 | hex[p[16]] << 4 | hex[p[17]];

    ret->Data4[0] = hex[p[19]] << 4 | hex[p[20]];
    ret->Data4[1] = hex[p[21]] << 4 | hex[p[22]];
    ret->Data4[2] = hex[p[24]] << 4 | hex[p[25]];
    ret->Data4[3] = hex[p[26]] << 4 | hex[p[27]];
    ret->Data4[4] = hex[p[28]] << 4 | hex[p[29]];
    ret->Data4[5] = hex[p[30]] << 4 | hex[p[31]];
    ret->Data4[6] = hex[p[32]] << 4 | hex[p[33]];
    ret->Data4[7] = hex[p[34]] << 4 | hex[p[35]];

    return S_OK;
}

static HRESULT str_to_string( const unsigned char *str, ULONG len, WS_HEAP *heap, WS_STRING *ret )
{
    int len_utf16 = MultiByteToWideChar( CP_UTF8, 0, (const char *)str, len, NULL, 0 );
    if (!(ret->chars = ws_alloc( heap, len_utf16 * sizeof(WCHAR) ))) return WS_E_QUOTA_EXCEEDED;
    MultiByteToWideChar( CP_UTF8, 0, (const char *)str, len, ret->chars, len_utf16 );
    ret->length = len_utf16;
    return S_OK;
}

static HRESULT str_to_unique_id( const unsigned char *str, ULONG len, WS_HEAP *heap, WS_UNIQUE_ID *ret )
{
    if (len == 45 && !memcmp( str, "urn:uuid:", 9 ))
    {
        ret->uri.length = 0;
        ret->uri.chars  = NULL;
        return str_to_guid( str + 9, len - 9, &ret->guid );
    }

    memset( &ret->guid, 0, sizeof(ret->guid) );
    return str_to_string( str, len, heap, &ret->uri );
}

static inline unsigned char decode_char( unsigned char c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 64;
}

static ULONG decode_base64( const unsigned char *base64, ULONG len, unsigned char *buf )
{
    ULONG i = 0;
    unsigned char c0, c1, c2, c3;
    const unsigned char *p = base64;

    while (len > 4)
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;
        buf[i + 0] = (c0 << 2) | (c1 >> 4);
        buf[i + 1] = (c1 << 4) | (c2 >> 2);
        buf[i + 2] = (c2 << 6) |  c3;
        len -= 4;
        i += 3;
        p += 4;
    }
    if (p[2] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        buf[i] = (c0 << 2) | (c1 >> 4);
        i++;
    }
    else if (p[3] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        buf[i + 0] = (c0 << 2) | (c1 >> 4);
        buf[i + 1] = (c1 << 4) | (c2 >> 2);
        i += 2;
    }
    else
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;
        buf[i + 0] = (c0 << 2) | (c1 >> 4);
        buf[i + 1] = (c1 << 4) | (c2 >> 2);
        buf[i + 2] = (c2 << 6) |  c3;
        i += 3;
    }
    return i;
}

static HRESULT str_to_bytes( const unsigned char *str, ULONG len, WS_HEAP *heap, WS_BYTES *ret )
{
    const unsigned char *p = str;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }

    if (len % 4) return WS_E_INVALID_FORMAT;
    if (!(ret->bytes = ws_alloc( heap, len * 3 / 4 ))) return WS_E_QUOTA_EXCEEDED;
    ret->length = decode_base64( p, len, ret->bytes );
    return S_OK;
}

static HRESULT str_to_xml_string( const unsigned char *str, ULONG len, WS_HEAP *heap, WS_XML_STRING *ret )
{
    if (!(ret->bytes = ws_alloc( heap, len ))) return WS_E_QUOTA_EXCEEDED;
    memcpy( ret->bytes, str, len );
    ret->length     = len;
    ret->dictionary = NULL;
    ret->id         = 0;
    return S_OK;
}

static HRESULT copy_xml_string( WS_HEAP *heap, const WS_XML_STRING *src, WS_XML_STRING *dst )
{
    if (!(dst->bytes = ws_alloc( heap, src->length ))) return WS_E_QUOTA_EXCEEDED;
    memcpy( dst->bytes, src->bytes, src->length );
    dst->length = src->length;
    return S_OK;
}

static HRESULT str_to_qname( struct reader *reader, const unsigned char *str, ULONG len, WS_HEAP *heap,
                             WS_XML_STRING *prefix_ret, WS_XML_STRING *localname_ret, WS_XML_STRING *ns_ret )
{
    const unsigned char *p = str;
    WS_XML_STRING prefix, localname;
    const WS_XML_STRING *ns;
    HRESULT hr;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }

    if ((hr = split_qname( p, len, &prefix, &localname )) != S_OK) return hr;
    if (!(ns = get_namespace( reader, &prefix ))) return WS_E_INVALID_FORMAT;

    if (prefix_ret && (hr = copy_xml_string( heap, &prefix, prefix_ret )) != S_OK) return hr;
    if ((hr = copy_xml_string( heap, &localname, localname_ret )) != S_OK)
    {
        ws_free( heap, prefix_ret->bytes, prefix_ret->length );
        return hr;
    }
    if ((hr = copy_xml_string( heap, ns, ns_ret )) != S_OK)
    {
        ws_free( heap, prefix_ret->bytes, prefix_ret->length );
        ws_free( heap, localname_ret->bytes, localname_ret->length );
        return hr;
    }
    return S_OK;
}

static HRESULT read_qualified_name( struct reader *reader, WS_HEAP *heap, WS_XML_STRING *prefix,
                                    WS_XML_STRING *localname, WS_XML_STRING *ns )
{
    const WS_XML_TEXT_NODE *node = (const WS_XML_TEXT_NODE *)reader->current;
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)node->text;
    return str_to_qname( reader, utf8->value.bytes, utf8->value.length, heap, prefix, localname, ns );
}

/**************************************************************************
 *          WsReadQualifiedName		[webservices.@]
 */
HRESULT WINAPI WsReadQualifiedName( WS_XML_READER *handle, WS_HEAP *heap, WS_XML_STRING *prefix,
                                    WS_XML_STRING *localname, WS_XML_STRING *ns,
                                    WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %p %p %p %p %p\n", handle, heap, prefix, localname, ns, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !heap) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type) hr = WS_E_INVALID_OPERATION;
    else if (!localname) hr = E_INVALIDARG;
    else if (reader->state != READER_STATE_TEXT) hr = WS_E_INVALID_FORMAT;
    else hr = read_qualified_name( reader, heap, prefix, localname, ns );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static const int month_offsets[2][12] =
{
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static inline int valid_day( int year, int month, int day )
{
    return day > 0 && day <= month_days[leap_year( year )][month - 1];
}

static inline int leap_days_before( int year )
{
    return (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400;
}

static HRESULT str_to_datetime( const unsigned char *bytes, ULONG len, WS_DATETIME *ret )
{
    const unsigned char *p = bytes, *q;
    int year, month, day, hour, min, sec, sec_frac = 0, tz_hour, tz_min, tz_neg;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }

    q = p;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 4 || !len || *q != '-') return WS_E_INVALID_FORMAT;
    year = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + p[3] - '0';
    if (year < 1) return WS_E_INVALID_FORMAT;

    p = ++q; len--;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != '-') return WS_E_INVALID_FORMAT;
    month = (p[0] - '0') * 10 + p[1] - '0';
    if (month < 1 || month > 12) return WS_E_INVALID_FORMAT;

    p = ++q; len--;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != 'T') return WS_E_INVALID_FORMAT;
    day = (p[0] - '0') * 10 + p[1] - '0';
    if (!valid_day( year, month, day )) return WS_E_INVALID_FORMAT;

    p = ++q; len--;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != ':') return WS_E_INVALID_FORMAT;
    hour = (p[0] - '0') * 10 + p[1] - '0';
    if (hour > 24) return WS_E_INVALID_FORMAT;

    p = ++q; len--;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != ':') return WS_E_INVALID_FORMAT;
    min = (p[0] - '0') * 10 + p[1] - '0';
    if (min > 59 || (min > 0 && hour == 24)) return WS_E_INVALID_FORMAT;

    p = ++q; len--;
    while (len && isdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len) return WS_E_INVALID_FORMAT;
    sec = (p[0] - '0') * 10 + p[1] - '0';
    if (sec > 59 || (sec > 0 && hour == 24)) return WS_E_INVALID_FORMAT;

    if (*q == '.')
    {
        unsigned int i, nb_digits, mul = TICKS_PER_SEC / 10;
        p = ++q; len--;
        while (len && isdigit( *q )) { q++; len--; };
        nb_digits = q - p;
        if (nb_digits < 1 || nb_digits > 7) return WS_E_INVALID_FORMAT;
        for (i = 0; i < nb_digits; i++)
        {
            sec_frac += (p[i] - '0') * mul;
            mul /= 10;
        }
    }
    if (*q == 'Z')
    {
        if (--len) return WS_E_INVALID_FORMAT;
        tz_hour = tz_min = tz_neg = 0;
        ret->format = WS_DATETIME_FORMAT_UTC;
    }
    else if (*q == '+' || *q == '-')
    {
        tz_neg = (*q == '-') ? 1 : 0;

        p = ++q; len--;
        while (len && isdigit( *q )) { q++; len--; };
        if (q - p != 2 || !len || *q != ':') return WS_E_INVALID_FORMAT;
        tz_hour = (p[0] - '0') * 10 + p[1] - '0';
        if (tz_hour > 14) return WS_E_INVALID_FORMAT;

        p = ++q; len--;
        while (len && isdigit( *q )) { q++; len--; };
        if (q - p != 2 || len) return WS_E_INVALID_FORMAT;
        tz_min = (p[0] - '0') * 10 + p[1] - '0';
        if (tz_min > 59 || (tz_min > 0 && tz_hour == 14)) return WS_E_INVALID_FORMAT;

        ret->format = WS_DATETIME_FORMAT_LOCAL;
    }
    else return WS_E_INVALID_FORMAT;

    ret->ticks = ((year - 1) * 365 + leap_days_before( year )) * TICKS_PER_DAY;
    ret->ticks += month_offsets[leap_year( year )][month - 1] * TICKS_PER_DAY;
    ret->ticks += (day - 1) * TICKS_PER_DAY;
    ret->ticks += hour * TICKS_PER_HOUR;
    ret->ticks += min * TICKS_PER_MIN;
    ret->ticks += sec * TICKS_PER_SEC;
    ret->ticks += sec_frac;

    if (tz_neg)
    {
        if (tz_hour * TICKS_PER_HOUR + tz_min * TICKS_PER_MIN + ret->ticks > TICKS_MAX)
            return WS_E_INVALID_FORMAT;
        ret->ticks += tz_hour * TICKS_PER_HOUR;
        ret->ticks += tz_min * TICKS_PER_MIN;
    }
    else
    {
        if (tz_hour * TICKS_PER_HOUR + tz_min * TICKS_PER_MIN > ret->ticks)
            return WS_E_INVALID_FORMAT;
        ret->ticks -= tz_hour * TICKS_PER_HOUR;
        ret->ticks -= tz_min * TICKS_PER_MIN;
    }

    return S_OK;
}

/**************************************************************************
 *          WsDateTimeToFileTime               [webservices.@]
 */
HRESULT WINAPI WsDateTimeToFileTime( const WS_DATETIME *dt, FILETIME *ft, WS_ERROR *error )
{
    unsigned __int64 ticks;

    TRACE( "%p %p %p\n", dt, ft, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!dt || !ft) return E_INVALIDARG;

    if (dt->ticks < TICKS_1601_01_01) return WS_E_INVALID_FORMAT;
    ticks = dt->ticks - TICKS_1601_01_01;
    ft->dwHighDateTime = ticks >> 32;
    ft->dwLowDateTime  = (DWORD)ticks;
    return S_OK;
}

/**************************************************************************
 *          WsFileTimeToDateTime               [webservices.@]
 */
HRESULT WINAPI WsFileTimeToDateTime( const FILETIME *ft, WS_DATETIME *dt, WS_ERROR *error )
{
    unsigned __int64 ticks;

    TRACE( "%p %p %p\n", ft, dt, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!dt || !ft) return E_INVALIDARG;

    ticks = ((unsigned __int64)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    if (ticks > MAX_UINT64 - TICKS_1601_01_01) return WS_E_NUMERIC_OVERFLOW;
    if (ticks + TICKS_1601_01_01 > TICKS_MAX) return WS_E_INVALID_FORMAT;
    dt->ticks  = ticks + TICKS_1601_01_01;
    dt->format = WS_DATETIME_FORMAT_UTC;
    return S_OK;
}

static BOOL find_attribute( struct reader *reader, const WS_XML_STRING *localname,
                            const WS_XML_STRING *ns, ULONG *index )
{
    ULONG i;
    WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;

    if (!localname)
    {
        *index = reader->current_attr;
        return TRUE;
    }
    for (i = 0; i < elem->attributeCount; i++)
    {
        const WS_XML_STRING *localname2 = elem->attributes[i]->localName;
        const WS_XML_STRING *ns2 = elem->attributes[i]->ns;

        if (!cmp_name( localname->bytes, localname->length, localname2->bytes, localname2->length ) &&
            !cmp_name( ns->bytes, ns->length, ns2->bytes, ns2->length ))
        {
            *index = i;
            return TRUE;
        }
    }
    return FALSE;
}

/**************************************************************************
 *          WsFindAttribute		[webservices.@]
 */
HRESULT WINAPI WsFindAttribute( WS_XML_READER *handle, const WS_XML_STRING *localname,
                                const WS_XML_STRING *ns, BOOL required, ULONG *index,
                                WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %s %s %d %p %p\n", handle, debugstr_xmlstr(localname), debugstr_xmlstr(ns),
           required, index, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !localname || !ns || !index) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT) hr = WS_E_INVALID_OPERATION;
    else if (!find_attribute( reader, localname, ns, index ))
    {
        if (required) hr = WS_E_INVALID_FORMAT;
        else
        {
            *index = ~0u;
            hr = S_FALSE;
        }
    }

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT get_node_text( struct reader *reader, const WS_XML_TEXT **ret )
{
    WS_XML_TEXT_NODE *node = (WS_XML_TEXT_NODE *)&reader->current->hdr.node;
    *ret = node->text;
    return S_OK;
}

static HRESULT get_attribute_text( struct reader *reader, ULONG index, const WS_XML_TEXT **ret )
{
    WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;
    *ret = elem->attributes[index]->value;
    return S_OK;
}

static BOOL match_element( const struct node *node, const WS_XML_STRING *localname, const WS_XML_STRING *ns )
{
    const WS_XML_ELEMENT_NODE *elem = (const WS_XML_ELEMENT_NODE *)node;
    if (node_type( node ) != WS_XML_NODE_TYPE_ELEMENT) return FALSE;
    return WsXmlStringEquals( localname, elem->localName, NULL ) == S_OK &&
          (!ns || WsXmlStringEquals( ns, elem->ns, NULL ) == S_OK);
}

static HRESULT read_next_node( struct reader *reader )
{
    if (reader->current == reader->last) return read_node( reader );
    if (move_to_child_node( &reader->current )) return S_OK;
    if (move_to_next_node( &reader->current )) return S_OK;
    if (!move_to_parent_node( &reader->current )) return WS_E_INVALID_FORMAT;
    if (move_to_next_node( &reader->current )) return S_OK;
    return WS_E_INVALID_FORMAT;
}

struct reader_pos
{
    struct node *node;
    ULONG        attr;
};

static void save_reader_position( const struct reader *reader, struct reader_pos *pos )
{
    pos->node = reader->current;
    pos->attr = reader->current_attr;
}

static void restore_reader_position( struct reader *reader, const struct reader_pos *pos )
{
    reader->current      = pos->node;
    reader->current_attr = pos->attr;
}

static HRESULT get_text( struct reader *reader, WS_TYPE_MAPPING mapping, const WS_XML_STRING *localname,
                         const WS_XML_STRING *ns, const WS_XML_TEXT **ret, BOOL *found )
{
    switch (mapping)
    {
    case WS_ATTRIBUTE_TYPE_MAPPING:
    {
        ULONG i;
        WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;

        *found = FALSE;
        for (i = 0; i < elem->attributeCount; i++)
        {
            const WS_XML_STRING *localname2 = elem->attributes[i]->localName;
            const WS_XML_STRING *ns2 = elem->attributes[i]->ns;

            if (cmp_name( localname->bytes, localname->length, localname2->bytes, localname2->length )) continue;
            if (!ns->length || !cmp_name( ns->bytes, ns->length, ns2->bytes, ns2->length ))
            {
                *found = TRUE;
                break;
            }
        }
        if (!*found) return S_OK;
        return get_attribute_text( reader, i, ret );
    }
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
    case WS_ANY_ELEMENT_TYPE_MAPPING:
    {
        *found = TRUE;
        if (localname)
        {
            struct reader_pos pos;
            HRESULT hr;

            if (!match_element( reader->current, localname, ns ))
            {
                *found = FALSE;
                return S_OK;
            }
            save_reader_position( reader, &pos );
            if ((hr = read_next_node( reader )) != S_OK) return hr;
            if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT)
            {
                restore_reader_position( reader, &pos );
                *found = FALSE;
                return S_OK;
            }
        }
        if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT)
        {
            *found = FALSE;
            return S_OK;
        }
        return get_node_text( reader, ret );
    }
    default:
        FIXME( "mapping %u not supported\n", mapping );
        return E_NOTIMPL;
    }
}

static HRESULT text_to_bool( const WS_XML_TEXT *text, BOOL *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_bool( text_utf8->value.bytes, text_utf8->value.length, val );
        break;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *text_bool = (const WS_XML_BOOL_TEXT *)text;
        *val = text_bool->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_bool( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_BOOL_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    BOOL val = FALSE;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_bool( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(BOOL *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        BOOL *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(BOOL **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_int8( const WS_XML_TEXT *text, INT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_int64( text_utf8->value.bytes, text_utf8->value.length, MIN_INT8, MAX_INT8, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (text_int32->value < MIN_INT8 || text_int32->value > MAX_INT8) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_int8( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_INT8_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    INT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_int8( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT8)) return E_INVALIDARG;
        *(INT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT8 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(INT8 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_int16( const WS_XML_TEXT *text, INT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_int64( text_utf8->value.bytes, text_utf8->value.length, MIN_INT16, MAX_INT16, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (text_int32->value < MIN_INT16 || text_int32->value > MAX_INT16) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_int16( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT16_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    INT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_int16( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT16)) return E_INVALIDARG;
        *(INT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT16 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(INT16 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_int32( const WS_XML_TEXT *text, INT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_int64( text_utf8->value.bytes, text_utf8->value.length, MIN_INT32, MAX_INT32, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_int32( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT32_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    INT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_int32( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT32)) return E_INVALIDARG;
        *(INT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT32 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(INT32 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_int64( const WS_XML_TEXT *text, INT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_int64( text_utf8->value.bytes, text_utf8->value.length, MIN_INT64, MAX_INT64, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        *val = text_int64->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_int64( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT64_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    INT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_int64( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(INT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT64 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(INT64 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_uint8( const WS_XML_TEXT *text, UINT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_uint64( text_utf8->value.bytes, text_utf8->value.length, MAX_UINT8, val );
        break;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        if (text_uint64->value > MAX_UINT8) return WS_E_NUMERIC_OVERFLOW;
        *val = text_uint64->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_uint8( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_UINT8_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    UINT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_uint8( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT8)) return E_INVALIDARG;
        *(UINT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT8 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(UINT8 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_uint16( const WS_XML_TEXT *text, UINT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_uint64( text_utf8->value.bytes, text_utf8->value.length, MAX_UINT16, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (text_int32->value < 0 || text_int32->value > MAX_UINT16) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        if (text_uint64->value > MAX_UINT16) return WS_E_NUMERIC_OVERFLOW;
        *val = text_uint64->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_uint16( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT16_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    UINT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_uint16( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT16)) return E_INVALIDARG;
        *(UINT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT16 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(UINT16 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_uint32( const WS_XML_TEXT *text, UINT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_uint64( text_utf8->value.bytes, text_utf8->value.length, MAX_UINT32, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (text_int32->value < 0) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        if (text_int64->value < 0 || text_int64->value > MAX_UINT32) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int64->value;
        hr = S_OK;
        break;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        if (text_uint64->value > MAX_UINT32) return WS_E_NUMERIC_OVERFLOW;
        *val = text_uint64->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_uint32( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT32_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    UINT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_uint32( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT32)) return E_INVALIDARG;
        *(UINT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT32 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(UINT32 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_uint64( const WS_XML_TEXT *text, UINT64 *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_uint64( text_utf8->value.bytes, text_utf8->value.length, MAX_UINT64, val );
        break;
    }
    case WS_XML_TEXT_TYPE_INT32:
    {
        const WS_XML_INT32_TEXT *text_int32 = (const WS_XML_INT32_TEXT *)text;
        if (text_int32->value < 0) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int32->value;
        hr = S_OK;
        break;
    }
    case WS_XML_TEXT_TYPE_INT64:
    {
        const WS_XML_INT64_TEXT *text_int64 = (const WS_XML_INT64_TEXT *)text;
        if (text_int64->value < 0) return WS_E_NUMERIC_OVERFLOW;
        *val = text_int64->value;
        hr = S_OK;
        break;
    }
    case WS_XML_TEXT_TYPE_UINT64:
    {
        const WS_XML_UINT64_TEXT *text_uint64 = (const WS_XML_UINT64_TEXT *)text;
        *val = text_uint64->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_uint64( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT64_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    UINT64 val = 0;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_uint64( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(UINT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT64 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(UINT64 **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_float( const WS_XML_TEXT *text, float *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_float( text_utf8->value.bytes, text_utf8->value.length, val );
        break;
    }
    case WS_XML_TEXT_TYPE_FLOAT:
    {
        const WS_XML_FLOAT_TEXT *text_float = (const WS_XML_FLOAT_TEXT *)text;
        *val = text_float->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_float( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_FLOAT_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    float val = 0.0;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_float( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(float *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        float *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(float **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_double( const WS_XML_TEXT *text, double *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_double( text_utf8->value.bytes, text_utf8->value.length, val );
        break;
    }
    case WS_XML_TEXT_TYPE_DOUBLE:
    {
        const WS_XML_DOUBLE_TEXT *text_double = (const WS_XML_DOUBLE_TEXT *)text;
        *val = text_double->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_double( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_DOUBLE_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    double val = 0.0;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_double( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(double *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        double *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(double **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_wsz( const WS_XML_TEXT *text, WS_HEAP *heap, WCHAR **ret )
{
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
    int len;

    assert( text->textType == WS_XML_TEXT_TYPE_UTF8 );
    len = MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, NULL, 0 );
    if (!(*ret = ws_alloc( heap, (len + 1) * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, *ret, len );
    (*ret)[len] = 0;
    return S_OK;
}

static HRESULT read_type_wsz( struct reader *reader, WS_TYPE_MAPPING mapping,
                              const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                              const WS_WSZ_DESCRIPTION *desc, WS_READ_OPTION option,
                              WS_HEAP *heap, WCHAR **ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    WCHAR *str = NULL;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_wsz( text, heap, &str )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
        if (!str && !(str = ws_alloc_zero( heap, sizeof(*str) ))) return WS_E_QUOTA_EXCEEDED;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (size != sizeof(str)) return E_INVALIDARG;
        *ret = str;
        break;

    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT get_enum_value( const WS_XML_TEXT *text, const WS_ENUM_DESCRIPTION *desc, int *ret )
{
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
    ULONG i;

    assert( text->textType == WS_XML_TEXT_TYPE_UTF8 );
    for (i = 0; i < desc->valueCount; i++)
    {
        if (WsXmlStringEquals( &utf8->value, desc->values[i].name, NULL ) == S_OK)
        {
            *ret = desc->values[i].value;
            return S_OK;
        }
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT read_type_enum( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_ENUM_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    int val = 0;

    if (!desc) return E_INVALIDARG;

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = get_enum_value( text, desc, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(int *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        int *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(int **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_datetime( const WS_XML_TEXT *text, WS_DATETIME *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_datetime( text_utf8->value.bytes, text_utf8->value.length, val );
        break;
    }
    case WS_XML_TEXT_TYPE_DATETIME:
    {
        const WS_XML_DATETIME_TEXT *text_datetime = (const WS_XML_DATETIME_TEXT *)text;
        *val = text_datetime->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_datetime( struct reader *reader, WS_TYPE_MAPPING mapping,
                                   const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                   const WS_DATETIME_DESCRIPTION *desc, WS_READ_OPTION option,
                                   WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    HRESULT hr;
    WS_DATETIME val = {0, WS_DATETIME_FORMAT_UTC};

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_datetime( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_DATETIME *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_DATETIME *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_DATETIME **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_guid( const WS_XML_TEXT *text, GUID *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_guid( text_utf8->value.bytes, text_utf8->value.length, val );
        break;
    }
    case WS_XML_TEXT_TYPE_GUID:
    {
        const WS_XML_GUID_TEXT *text_guid = (const WS_XML_GUID_TEXT *)text;
        *val = text_guid->value;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_guid( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_GUID_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    GUID val = {0};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_guid( text, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(GUID *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        GUID *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(GUID **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_unique_id( const WS_XML_TEXT *text, WS_HEAP *heap, WS_UNIQUE_ID *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_unique_id( text_utf8->value.bytes, text_utf8->value.length, heap, val );
        break;
    }
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    {
        const WS_XML_UNIQUE_ID_TEXT *text_unique_id = (const WS_XML_UNIQUE_ID_TEXT *)text;
        val->guid       = text_unique_id->value;
        val->uri.length = 0;
        val->uri.chars  = NULL;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_unique_id( struct reader *reader, WS_TYPE_MAPPING mapping,
                                    const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                    const WS_UNIQUE_ID_DESCRIPTION *desc, WS_READ_OPTION option,
                                    WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    WS_UNIQUE_ID val = {{0}};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_unique_id( text, heap, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_UNIQUE_ID *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_UNIQUE_ID *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_UNIQUE_ID **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_string( const WS_XML_TEXT *text, WS_HEAP *heap, WS_STRING *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_string( text_utf8->value.bytes, text_utf8->value.length, heap, val );
        break;
    }
    case WS_XML_TEXT_TYPE_UTF16:
    {
        const WS_XML_UTF16_TEXT *text_utf16 = (const WS_XML_UTF16_TEXT *)text;
        if (!(val->chars = ws_alloc( heap, text_utf16->byteCount ))) return WS_E_QUOTA_EXCEEDED;
        memcpy( val->chars, text_utf16->bytes, text_utf16->byteCount );
        val->length = text_utf16->byteCount / sizeof(WCHAR);
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_string( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_STRING_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    WS_STRING val = {0};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_string( text, heap, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_STRING *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
    {
        WS_STRING *heap_val;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
        *heap_val = val;
        *(WS_STRING **)ret = heap_val;
        break;
    }
    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_STRING *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_STRING **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_bytes( const WS_XML_TEXT *text, WS_HEAP *heap, WS_BYTES *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_bytes( text_utf8->value.bytes, text_utf8->value.length, heap, val );
        break;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *text_base64 = (const WS_XML_BASE64_TEXT *)text;
        if (!(val->bytes = ws_alloc( heap, text_base64->length ))) return WS_E_QUOTA_EXCEEDED;
        memcpy( val->bytes, text_base64->bytes, text_base64->length );
        val->length = text_base64->length;
        hr = S_OK;
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_bytes( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_BYTES_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    WS_BYTES val = {0};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_bytes( text, heap, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_BYTES *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
    {
        WS_BYTES *heap_val;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
        *heap_val = val;
        *(WS_BYTES **)ret = heap_val;
        break;
    }
    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_BYTES *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_BYTES **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_xml_string( const WS_XML_TEXT *text, WS_HEAP *heap, WS_XML_STRING *val )
{
    const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
    assert( text->textType == WS_XML_TEXT_TYPE_UTF8 );
    return str_to_xml_string( utf8->value.bytes, utf8->value.length, heap, val );
}

static HRESULT read_type_xml_string( struct reader *reader, WS_TYPE_MAPPING mapping,
                                     const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                     const WS_XML_STRING_DESCRIPTION *desc, WS_READ_OPTION option,
                                     WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    WS_XML_STRING val = {0};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_xml_string( text, heap, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_XML_STRING *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
    {
        WS_XML_STRING *heap_val;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
        *heap_val = val;
        *(WS_XML_STRING **)ret = heap_val;
        break;
    }
    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_XML_STRING *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_XML_STRING **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT text_to_qname( struct reader *reader, const WS_XML_TEXT *text, WS_HEAP *heap, WS_XML_QNAME *val )
{
    HRESULT hr;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        hr = str_to_qname( reader, text_utf8->value.bytes, text_utf8->value.length, heap, NULL,
                           &val->localName, &val->ns );
        break;
    }
    case WS_XML_TEXT_TYPE_QNAME:
    {
        const WS_XML_QNAME_TEXT *text_qname = (const WS_XML_QNAME_TEXT *)text;
        if ((hr = copy_xml_string( heap, text_qname->localName, &val->localName )) != S_OK) return hr;
        if ((hr = copy_xml_string( heap, text_qname->ns, &val->ns )) != S_OK)
        {
            ws_free( heap, val->localName.bytes, val->localName.length );
            return hr;
        }
        break;
    }
    default:
        FIXME( "unhandled text type %u\n", text->textType );
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT read_type_qname( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_XML_QNAME_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    const WS_XML_TEXT *text;
    WS_XML_QNAME val = {{0}};
    HRESULT hr;

    if (desc) FIXME( "ignoring description\n" );

    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT) return WS_E_INVALID_FORMAT;
    if ((hr = read_startelement( reader )) != S_OK) return hr;
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT) return WS_E_INVALID_FORMAT;

    if ((hr = get_text( reader, mapping, localname, ns, &text, found )) != S_OK) return hr;
    if (*found && (hr = text_to_qname( reader, text, heap, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(val)) return E_INVALIDARG;
        *(WS_XML_QNAME *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!*found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_XML_QNAME *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (*found)
        {
            if (!(heap_val = ws_alloc( heap, sizeof(*heap_val) ))) return WS_E_QUOTA_EXCEEDED;
            *heap_val = val;
        }
        *(WS_XML_QNAME **)ret = heap_val;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT read_type_description( struct reader *reader, WS_TYPE_MAPPING mapping,
                                      const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                      const WS_STRUCT_DESCRIPTION *desc, WS_READ_OPTION option,
                                      WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
    case WS_READ_OPTIONAL_POINTER:
    {
        if (size != sizeof(desc)) return E_INVALIDARG;
        *(const WS_STRUCT_DESCRIPTION **)ret = desc;
        *found = TRUE;
        break;
    }
    default:
        FIXME( "read option %u not supported\n", option );
        return E_NOTIMPL;
    }

    return S_OK;
}

static BOOL is_empty_text_node( const struct node *node )
{
    const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)node;

    if (node_type( node ) != WS_XML_NODE_TYPE_TEXT) return FALSE;
    switch (text->text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        ULONG i;
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text->text;
        for (i = 0; i < utf8->value.length; i++) if (!read_isspace( utf8->value.bytes[i] )) return FALSE;
        return TRUE;
    }
    case WS_XML_TEXT_TYPE_BASE64:
    {
        const WS_XML_BASE64_TEXT *base64 = (const WS_XML_BASE64_TEXT *)text->text;
        return !base64->length;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    case WS_XML_TEXT_TYPE_INT32:
    case WS_XML_TEXT_TYPE_INT64:
    case WS_XML_TEXT_TYPE_UINT64:
    case WS_XML_TEXT_TYPE_FLOAT:
    case WS_XML_TEXT_TYPE_DOUBLE:
    case WS_XML_TEXT_TYPE_DECIMAL:
    case WS_XML_TEXT_TYPE_GUID:
    case WS_XML_TEXT_TYPE_UNIQUE_ID:
    case WS_XML_TEXT_TYPE_DATETIME:
        return FALSE;

    default:
        ERR( "unhandled text type %u\n", text->text->textType );
        return FALSE;
    }
}

/* skips comment and empty text nodes */
static HRESULT read_type_next_node( struct reader *reader )
{
    for (;;)
    {
        HRESULT hr;
        WS_XML_NODE_TYPE type;

        if ((hr = read_next_node( reader )) != S_OK) return hr;
        type = node_type( reader->current );
        if (type == WS_XML_NODE_TYPE_COMMENT ||
            (type == WS_XML_NODE_TYPE_TEXT && is_empty_text_node( reader->current ))) continue;
        return S_OK;
    }
}

static HRESULT read_type_next_element_node( struct reader *reader, const WS_XML_STRING *localname,
                                            const WS_XML_STRING *ns )
{
    struct reader_pos pos;
    HRESULT hr;

    if (!localname) return S_OK; /* assume reader is already correctly positioned */
    if (reader->current == reader->last)
    {
        BOOL found;
        if ((hr = read_to_startelement( reader, &found )) != S_OK) return hr;
        if (!found) return WS_E_INVALID_FORMAT;
    }
    if (match_element( reader->current, localname, ns )) return S_OK;

    save_reader_position( reader, &pos );
    if ((hr = read_type_next_node( reader )) != S_OK) return hr;
    if (match_element( reader->current, localname, ns )) return S_OK;
    restore_reader_position( reader, &pos );

    return WS_E_INVALID_FORMAT;
}

ULONG get_type_size( WS_TYPE type, const void *desc )
{
    switch (type)
    {
    case WS_INT8_TYPE:
    case WS_UINT8_TYPE:
        return sizeof(INT8);

    case WS_INT16_TYPE:
    case WS_UINT16_TYPE:
        return sizeof(INT16);

    case WS_BOOL_TYPE:
    case WS_INT32_TYPE:
    case WS_UINT32_TYPE:
    case WS_ENUM_TYPE:
        return sizeof(INT32);

    case WS_INT64_TYPE:
    case WS_UINT64_TYPE:
        return sizeof(INT64);

    case WS_FLOAT_TYPE:
        return sizeof(float);

    case WS_DOUBLE_TYPE:
        return sizeof(double);

    case WS_DATETIME_TYPE:
        return sizeof(WS_DATETIME);

    case WS_GUID_TYPE:
        return sizeof(GUID);

    case WS_UNIQUE_ID_TYPE:
        return sizeof(WS_UNIQUE_ID);

    case WS_STRING_TYPE:
        return sizeof(WS_STRING);

    case WS_WSZ_TYPE:
        return sizeof(WCHAR *);

    case WS_BYTES_TYPE:
        return sizeof(WS_BYTES);

    case WS_XML_STRING_TYPE:
        return sizeof(WS_XML_STRING);

    case WS_XML_QNAME_TYPE:
        return sizeof(WS_XML_QNAME);

    case WS_DESCRIPTION_TYPE:
        return sizeof(WS_STRUCT_DESCRIPTION *);

    case WS_STRUCT_TYPE:
    {
        const WS_STRUCT_DESCRIPTION *desc_struct = desc;
        return desc_struct->size;
    }
    case WS_UNION_TYPE:
    {
        const WS_UNION_DESCRIPTION *desc_union = desc;
        return desc_union->size;
    }
    case WS_ANY_ATTRIBUTES_TYPE:
        return 0;

    default:
        ERR( "unhandled type %u\n", type );
        return 0;
    }
}

static WS_READ_OPTION get_field_read_option( WS_TYPE type, ULONG options )
{
    if (options & WS_FIELD_POINTER)
    {
        if (options & WS_FIELD_NILLABLE) return WS_READ_NILLABLE_POINTER;
        if (options & WS_FIELD_OPTIONAL) return WS_READ_OPTIONAL_POINTER;
        return WS_READ_REQUIRED_POINTER;
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
    case WS_FLOAT_TYPE:
    case WS_DOUBLE_TYPE:
    case WS_DATETIME_TYPE:
    case WS_GUID_TYPE:
    case WS_UNIQUE_ID_TYPE:
    case WS_STRING_TYPE:
    case WS_BYTES_TYPE:
    case WS_XML_STRING_TYPE:
    case WS_XML_QNAME_TYPE:
    case WS_XML_BUFFER_TYPE:
    case WS_STRUCT_TYPE:
    case WS_ENUM_TYPE:
    case WS_UNION_TYPE:
        if (options & (WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE)) return WS_READ_NILLABLE_VALUE;
        return WS_READ_REQUIRED_VALUE;

    case WS_WSZ_TYPE:
    case WS_DESCRIPTION_TYPE:
        if (options & WS_FIELD_NILLABLE) return WS_READ_NILLABLE_POINTER;
        if (options & WS_FIELD_OPTIONAL) return WS_READ_OPTIONAL_POINTER;
        return WS_READ_REQUIRED_POINTER;

    default:
        FIXME( "unhandled type %u\n", type );
        return 0;
    }
}

static HRESULT read_type_field( struct reader *, const WS_STRUCT_DESCRIPTION *, const WS_FIELD_DESCRIPTION *,
                                WS_HEAP *, char *, ULONG );

static HRESULT read_type_union( struct reader *reader, const WS_UNION_DESCRIPTION *desc, WS_HEAP *heap, void *ret,
                                ULONG size, BOOL *found )
{
    struct reader_pos pos;
    HRESULT hr;
    ULONG i;

    if (size != desc->size) return E_INVALIDARG;

    save_reader_position( reader, &pos );
    if ((hr = read_type_next_node( reader )) != S_OK) return hr;

    for (i = 0; i < desc->fieldCount; i++)
    {
        if ((*found = match_element( reader->current, desc->fields[i]->field.localName, desc->fields[i]->field.ns )))
            break;
    }

    if (!*found)
    {
        *(int *)((char *)ret + desc->enumOffset) = desc->noneEnumValue;
        restore_reader_position( reader, &pos );
    }
    else
    {
        ULONG offset = desc->fields[i]->field.offset;
        if ((hr = read_type_field( reader, NULL, &desc->fields[i]->field, heap, ret, offset )) != S_OK) return hr;
        *(int *)((char *)ret + desc->enumOffset) = desc->fields[i]->value;
    }

    return S_OK;
}

static HRESULT read_type( struct reader *, WS_TYPE_MAPPING, WS_TYPE, const WS_XML_STRING *,
                          const WS_XML_STRING *, const void *, WS_READ_OPTION, WS_HEAP *,
                          void *, ULONG, BOOL * );

static HRESULT read_type_array( struct reader *reader, const WS_FIELD_DESCRIPTION *desc, WS_HEAP *heap,
                                void **ret, ULONG *count )
{
    HRESULT hr;
    ULONG item_size, nb_items = 0, nb_allocated = 1, offset = 0;
    WS_READ_OPTION option;
    BOOL found;
    char *buf;

    if (!(option = get_field_read_option( desc->type, desc->options ))) return E_INVALIDARG;

    /* wrapper element */
    if (desc->localName && ((hr = read_type_next_element_node( reader, desc->localName, desc->ns )) != S_OK))
        return hr;

    if (option == WS_READ_REQUIRED_VALUE || option == WS_READ_NILLABLE_VALUE)
        item_size = get_type_size( desc->type, desc->typeDescription );
    else
        item_size = sizeof(void *);

    if (!(buf = ws_alloc_zero( heap, item_size ))) return WS_E_QUOTA_EXCEEDED;
    for (;;)
    {
        if (nb_items >= nb_allocated)
        {
            SIZE_T old_size = nb_allocated * item_size, new_size = old_size * 2;
            if (!(buf = ws_realloc_zero( heap, buf, old_size, new_size ))) return WS_E_QUOTA_EXCEEDED;
            nb_allocated *= 2;
        }

        if (desc->type == WS_UNION_TYPE)
        {
            hr = read_type_union( reader, desc->typeDescription, heap, buf + offset, item_size, &found );
            if (hr != S_OK)
            {
                ws_free( heap, buf, nb_allocated * item_size );
                return hr;
            }
            if (!found) break;
        }
        else
        {
            hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->itemLocalName, desc->itemNs,
                            desc->typeDescription, option, heap, buf + offset, item_size, &found );
            if (hr == WS_E_INVALID_FORMAT) break;
            if (hr != S_OK)
            {
                ws_free( heap, buf, nb_allocated * item_size );
                return hr;
            }
        }

        offset += item_size;
        nb_items++;
    }

    if (desc->localName && ((hr = read_type_next_node( reader )) != S_OK)) return hr;

    if (desc->itemRange && (nb_items < desc->itemRange->minItemCount || nb_items > desc->itemRange->maxItemCount))
    {
        TRACE( "number of items %lu out of range (%lu-%lu)\n", nb_items, desc->itemRange->minItemCount,
               desc->itemRange->maxItemCount );
        ws_free( heap, buf, nb_allocated * item_size );
        return WS_E_INVALID_FORMAT;
    }

    *count = nb_items;
    *ret = buf;

    return S_OK;
}

static HRESULT read_type_text( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                               WS_READ_OPTION option, WS_HEAP *heap, void *ret, ULONG size )
{
    struct reader_pos pos;
    BOOL found;
    HRESULT hr;

    if (reader->current == reader->last)
    {
        if ((hr = read_to_startelement( reader, &found )) != S_OK) return hr;
        if (!found) return WS_E_INVALID_FORMAT;
    }

    save_reader_position( reader, &pos );
    if ((hr = read_next_node( reader )) != S_OK) return hr;
    hr = read_type( reader, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, NULL, NULL,
                    desc->typeDescription, option, heap, ret, size, &found );
    if (hr == S_OK && !found) restore_reader_position( reader, &pos );
    return hr;
}

static HRESULT read_type_field( struct reader *reader, const WS_STRUCT_DESCRIPTION *desc_struct,
                                const WS_FIELD_DESCRIPTION *desc, WS_HEAP *heap, char *buf, ULONG offset )
{
    char *ptr;
    WS_READ_OPTION option;
    ULONG size;
    HRESULT hr;
    BOOL found;

    if (!desc) return E_INVALIDARG;
    if (desc->options & ~(WS_FIELD_POINTER|WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM))
    {
        FIXME( "options %#lx not supported\n", desc->options );
        return E_NOTIMPL;
    }
    if (!(option = get_field_read_option( desc->type, desc->options ))) return E_INVALIDARG;

    if (option == WS_READ_REQUIRED_VALUE || option == WS_READ_NILLABLE_VALUE)
        size = get_type_size( desc->type, desc->typeDescription );
    else
        size = sizeof(void *);

    ptr = buf + offset;
    switch (desc->mapping)
    {
    case WS_TYPE_ATTRIBUTE_FIELD_MAPPING:
        hr = read_type( reader, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->localName, desc->ns,
                        desc_struct, option, heap, ptr, size, &found );
        break;

    case WS_ATTRIBUTE_FIELD_MAPPING:
        hr = read_type( reader, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->localName, desc->ns,
                        desc->typeDescription, option, heap, ptr, size, &found );
        break;

    case WS_ELEMENT_FIELD_MAPPING:
        hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->localName, desc->ns,
                        desc->typeDescription, option, heap, ptr, size, &found );
        break;

    case WS_ELEMENT_CHOICE_FIELD_MAPPING:
    {
        if (desc->type != WS_UNION_TYPE || !desc->typeDescription ||
            (desc->options & (WS_FIELD_POINTER|WS_FIELD_NILLABLE))) return E_INVALIDARG;
        hr = read_type_union( reader, desc->typeDescription, heap, ptr, size, &found );
        break;
    }
    case WS_REPEATING_ELEMENT_FIELD_MAPPING:
    case WS_REPEATING_ELEMENT_CHOICE_FIELD_MAPPING:
    {
        ULONG count;
        hr = read_type_array( reader, desc, heap, (void **)ptr, &count );
        if (hr == S_OK) *(ULONG *)(buf + desc->countOffset) = count;
        break;
    }
    case WS_TEXT_FIELD_MAPPING:
        hr = read_type_text( reader, desc, option, heap, ptr, size );
        break;

    default:
        FIXME( "unhandled field mapping %u\n", desc->mapping );
        return E_NOTIMPL;
    }

    if (hr == WS_E_INVALID_FORMAT)
    {
        switch (option)
        {
        case WS_READ_REQUIRED_VALUE:
        case WS_READ_REQUIRED_POINTER:
            return WS_E_INVALID_FORMAT;

        case WS_READ_NILLABLE_VALUE:
            if (desc->defaultValue) memcpy( ptr, desc->defaultValue->value, desc->defaultValue->valueSize );
            return S_OK;

        case WS_READ_OPTIONAL_POINTER:
        case WS_READ_NILLABLE_POINTER:
            *(void **)ptr = NULL;
            return S_OK;

        default:
            ERR( "unhandled option %u\n", option );
            return E_NOTIMPL;
        }
    }

    return hr;
}

static HRESULT read_type_struct( struct reader *reader, WS_TYPE_MAPPING mapping, const WS_XML_STRING *localname,
                                 const WS_XML_STRING *ns, const WS_STRUCT_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    ULONG i, offset;
    HRESULT hr;
    char *buf;

    if (!desc) return E_INVALIDARG;
    if (desc->structOptions & ~WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT)
    {
        FIXME( "struct options %#lx not supported\n",
               desc->structOptions & ~WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT );
    }

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (size != sizeof(void *)) return E_INVALIDARG;
        if (!(buf = ws_alloc_zero( heap, desc->size ))) return WS_E_QUOTA_EXCEEDED;
        break;

    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        if (size != desc->size) return E_INVALIDARG;
        buf = ret;
        break;

    default:
        FIXME( "unhandled read option %u\n", option );
        return E_NOTIMPL;
    }

    for (i = 0; i < desc->fieldCount; i++)
    {
        offset = desc->fields[i]->offset;
        if ((hr = read_type_field( reader, desc, desc->fields[i], heap, buf, offset )) != S_OK) break;
    }

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
        if (hr != S_OK)
        {
            ws_free( heap, buf, desc->size );
            return hr;
        }
        *(char **)ret = buf;
        break;

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (is_nil_value( buf, desc->size ))
        {
            ws_free( heap, buf, desc->size );
            buf = NULL;
        }
        *(char **)ret = buf;
        break;

    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        if (hr != S_OK) return hr;
        break;

    default:
        ERR( "unhandled read option %u\n", option );
        return E_NOTIMPL;
    }

    if (desc->structOptions & WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT)
    {
        struct node *parent = find_parent( reader );
        parent->flags |= NODE_FLAG_IGNORE_TRAILING_ELEMENT_CONTENT;
    }

    *found = TRUE;
    return S_OK;
}

static HRESULT read_type_fault( struct reader *reader, WS_TYPE_MAPPING mapping, const WS_XML_STRING *localname,
                                const WS_XML_STRING *ns, const WS_FAULT_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size, BOOL *found )
{
    static const WS_XML_STRING faultcode = {9, (BYTE *)"faultcode"}, faultstring = {11, (BYTE *)"faultstring"};
    static const WS_XML_STRING faultactor = {10, (BYTE *)"faultactor"}, detail = {6, (BYTE *)"detail"};

    const struct node *root = reader->current;
    WS_FAULT *fault;
    HRESULT hr = S_OK;

    if (mapping != WS_ELEMENT_TYPE_MAPPING || !desc || !ret)
        return E_INVALIDARG;
    if (desc->envelopeVersion < WS_ENVELOPE_VERSION_SOAP_1_1 || desc->envelopeVersion >= WS_ENVELOPE_VERSION_NONE)
        return E_INVALIDARG;
    else if (desc->envelopeVersion != WS_ENVELOPE_VERSION_SOAP_1_1)
    {
        FIXME( "unhandled envelopeVersion %u\n", desc->envelopeVersion );
        return E_NOTIMPL;
    }

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (size != sizeof(*fault))
            return E_INVALIDARG;
        fault = ret;
        memset( fault, 0, sizeof(*fault) );
        break;

    case WS_READ_REQUIRED_POINTER:
    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (size != sizeof(void *))
            return E_INVALIDARG;
        if (!(fault = ws_alloc_zero( heap, sizeof(*fault) )))
            return WS_E_QUOTA_EXCEEDED;
        break;

    case WS_READ_NILLABLE_VALUE:
        return E_INVALIDARG;

    default:
        FIXME( "unhandled read option %u\n", option );
        return E_NOTIMPL;
    }

    if ((hr = read_type_next_node( reader )) != S_OK) goto done;
    for (;;)
    {
        if (node_type( reader->current ) == WS_XML_NODE_TYPE_END_ELEMENT && reader->current->parent == root)
            break;

        if (match_element( reader->current, &faultcode, ns ))
        {
            if (fault->code)
            {
                hr = WS_E_INVALID_FORMAT;
                break;
            }
            if (!(fault->code = ws_alloc_zero( heap, sizeof(*fault->code) )))
            {
                hr = WS_E_QUOTA_EXCEEDED;
                break;
            }
            if ((hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, WS_XML_QNAME_TYPE, NULL, NULL,
                                 NULL, WS_READ_REQUIRED_VALUE, heap, &fault->code->value,
                                 sizeof(fault->code->value), found )) != S_OK)
                break;

        }
        else if (match_element( reader->current, &faultstring, ns ))
        {
            if (fault->reasons)
            {
                hr = WS_E_INVALID_FORMAT;
                break;
            }
            if (!(fault->reasons = ws_alloc_zero( heap, sizeof(*fault->reasons) )))
            {
                hr = WS_E_QUOTA_EXCEEDED;
                break;
            }
            fault->reasonCount = 1;
            /* FIXME: parse optional xml:lang attribute */
            if ((hr = read_next_node( reader )) != S_OK ||
                (hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRING_TYPE, NULL, NULL,
                                 NULL, WS_READ_REQUIRED_VALUE, heap, &fault->reasons->text,
                                 sizeof(fault->reasons->text), found )) != S_OK)
                break;
        }
        else if (match_element( reader->current, &faultactor, ns ))
        {
            if (fault->actor.length > 0)
            {
                hr = WS_E_INVALID_FORMAT;
                break;
            }
            if ((hr = read_next_node( reader )) != S_OK ||
                (hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, WS_STRING_TYPE, NULL, NULL,
                                 NULL, WS_READ_REQUIRED_VALUE, heap, &fault->actor,
                                 sizeof(fault->actor), found )) != S_OK)
                break;
        }
        else if (match_element( reader->current, &detail, ns ))
        {
            if (fault->detail)
            {
                hr = WS_E_INVALID_FORMAT;
                break;
            }
            if ((hr = WsReadXmlBuffer( (WS_XML_READER *)reader, heap, &fault->detail, NULL )) != S_OK)
                break;
        }
        else if ((hr = read_type_next_node( reader )) != S_OK)
            break;
    }

done:
    if ((!fault->code || !fault->reasons) && hr == S_OK)
        hr = WS_E_INVALID_FORMAT;

    if (hr != S_OK)
    {
        free_fault_fields( heap, fault );

        switch (option)
        {
        case WS_READ_REQUIRED_VALUE:
        case WS_READ_REQUIRED_POINTER:
            memset( fault, 0, sizeof(*fault) );
            break;

        case WS_READ_OPTIONAL_POINTER:
        case WS_READ_NILLABLE_POINTER:
            if (hr == WS_E_INVALID_FORMAT && is_nil_value( (const char *)fault, sizeof(*fault) ))
            {
                ws_free( heap, fault, sizeof(*fault) );
                fault = NULL;
                hr = S_OK;
            }
            else
                memset( fault, 0, sizeof(*fault) );
            break;

        default:
            ERR( "unhandled option %u\n", option );
            return E_NOTIMPL;
        }
    }

    if (option != WS_READ_REQUIRED_VALUE)
        *(WS_FAULT **)ret = fault;

    *found = TRUE;
    return hr;
}

static HRESULT start_mapping( struct reader *reader, WS_TYPE_MAPPING mapping, const WS_XML_STRING *localname,
                              const WS_XML_STRING *ns )
{
    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        return read_type_next_element_node( reader, localname, ns );

    case WS_ANY_ELEMENT_TYPE_MAPPING:
    case WS_ATTRIBUTE_TYPE_MAPPING:
        return S_OK;

    default:
        FIXME( "unhandled mapping %u\n", mapping );
        return E_NOTIMPL;
    }
}

static HRESULT read_type_endelement_node( struct reader *reader )
{
    const struct node *parent = find_parent( reader );
    HRESULT hr;

    for (;;)
    {
        if ((hr = read_type_next_node( reader )) != S_OK) return hr;
        if (node_type( reader->current ) == WS_XML_NODE_TYPE_END_ELEMENT && reader->current->parent == parent)
        {
            return S_OK;
        }
        if (read_end_of_data( reader ) || !(parent->flags & NODE_FLAG_IGNORE_TRAILING_ELEMENT_CONTENT)) break;
    }

    return WS_E_INVALID_FORMAT;
}

static HRESULT end_mapping( struct reader *reader, WS_TYPE_MAPPING mapping )
{
    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
        return read_type_endelement_node( reader );

    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        return read_type_next_node( reader );

    case WS_ATTRIBUTE_TYPE_MAPPING:
    default:
        return S_OK;
    }
}

static BOOL is_true_text( const WS_XML_TEXT *text )
{
    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *text_utf8 = (const WS_XML_UTF8_TEXT *)text;
        if (text_utf8->value.length == 4 && !memcmp( text_utf8->value.bytes, "true", 4 )) return TRUE;
        return FALSE;
    }
    case WS_XML_TEXT_TYPE_BOOL:
    {
        const WS_XML_BOOL_TEXT *text_bool = (const WS_XML_BOOL_TEXT *)text;
        return text_bool->value;
    }
    default:
        ERR( "unhandled text type %u\n", text->textType );
        return FALSE;
    }
}

static HRESULT is_nil_element( const WS_XML_ELEMENT_NODE *elem )
{
    static const WS_XML_STRING localname = {3, (BYTE *)"nil"};
    static const WS_XML_STRING ns = {41, (BYTE *)"http://www.w3.org/2001/XMLSchema-instance"};
    ULONG i;

    for (i = 0; i < elem->attributeCount; i++)
    {
        if (elem->attributes[i]->isXmlNs) continue;
        if (WsXmlStringEquals( elem->attributes[i]->localName, &localname, NULL ) == S_OK &&
            WsXmlStringEquals( elem->attributes[i]->ns, &ns, NULL ) == S_OK &&
            is_true_text( elem->attributes[i]->value )) return TRUE;
    }
    return FALSE;
}

static HRESULT read_type( struct reader *reader, WS_TYPE_MAPPING mapping, WS_TYPE type,
                          const WS_XML_STRING *localname, const WS_XML_STRING *ns, const void *desc,
                          WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size, BOOL *found )
{
    HRESULT hr;

    if ((hr = start_mapping( reader, mapping, localname, ns )) != S_OK) return hr;

    if (mapping == WS_ELEMENT_TYPE_MAPPING && is_nil_element( &reader->current->hdr ))
    {
        if (option != WS_READ_NILLABLE_POINTER && option != WS_READ_NILLABLE_VALUE) return WS_E_INVALID_FORMAT;
        *found = TRUE;
        return end_mapping( reader, mapping );
    }

    switch (type)
    {
    case WS_BOOL_TYPE:
        hr = read_type_bool( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_INT8_TYPE:
        hr = read_type_int8( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_INT16_TYPE:
        hr = read_type_int16( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_INT32_TYPE:
        hr = read_type_int32( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_INT64_TYPE:
        hr = read_type_int64( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_UINT8_TYPE:
        hr = read_type_uint8( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_UINT16_TYPE:
        hr = read_type_uint16( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_UINT32_TYPE:
        hr = read_type_uint32( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_UINT64_TYPE:
        hr = read_type_uint64( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_FLOAT_TYPE:
        hr = read_type_float( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_DOUBLE_TYPE:
        hr = read_type_double( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_DATETIME_TYPE:
        hr = read_type_datetime( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_GUID_TYPE:
        hr = read_type_guid( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_UNIQUE_ID_TYPE:
        hr = read_type_unique_id( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_STRING_TYPE:
        hr = read_type_string( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_WSZ_TYPE:
        hr = read_type_wsz( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_BYTES_TYPE:
        hr = read_type_bytes( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_XML_STRING_TYPE:
        hr = read_type_xml_string( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_XML_QNAME_TYPE:
        hr = read_type_qname( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_DESCRIPTION_TYPE:
        hr = read_type_description( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_STRUCT_TYPE:
        hr = read_type_struct( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_FAULT_TYPE:
        hr = read_type_fault( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    case WS_ENUM_TYPE:
        hr = read_type_enum( reader, mapping, localname, ns, desc, option, heap, value, size, found );
        break;

    default:
        FIXME( "type %u not supported\n", type );
        return E_NOTIMPL;
    }

    if (hr != S_OK) return hr;
    return end_mapping( reader, mapping );
}

/**************************************************************************
 *          WsReadType		[webservices.@]
 */
HRESULT WINAPI WsReadType( WS_XML_READER *handle, WS_TYPE_MAPPING mapping, WS_TYPE type,
                           const void *desc, WS_READ_OPTION option, WS_HEAP *heap, void *value,
                           ULONG size, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    BOOL found;
    HRESULT hr;

    TRACE( "%p %u %u %p %#x %p %p %lu %p\n", handle, mapping, type, desc, option, heap, value,
           size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !value) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if ((hr = read_type( reader, mapping, type, NULL, NULL, desc, option, heap, value, size, &found )) == S_OK)
    {
        switch (mapping)
        {
        case WS_ELEMENT_TYPE_MAPPING:
            hr = read_node( reader );
            break;

        default:
            break;
        }
        if (hr == S_OK && !read_end_of_data( reader )) hr = WS_E_INVALID_FORMAT;
    }

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

HRESULT read_header( WS_XML_READER *handle, const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                     WS_TYPE type, const void *desc, WS_READ_OPTION option, WS_HEAP *heap, void *value,
                     ULONG size )
{
    struct reader *reader = (struct reader *)handle;
    BOOL found;
    HRESULT hr;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_type( reader, WS_ELEMENT_CONTENT_TYPE_MAPPING, type, localname, ns, desc, option, heap,
                    value, size, &found );

    LeaveCriticalSection( &reader->cs );
    return hr;
}

/**************************************************************************
 *          WsReadElement		[webservices.@]
 */
HRESULT WINAPI WsReadElement( WS_XML_READER *handle, const WS_ELEMENT_DESCRIPTION *desc,
                              WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size,
                              WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    BOOL found;
    HRESULT hr;

    TRACE( "%p %p %#x %p %p %lu %p\n", handle, desc, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !desc || !value) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->elementLocalName,
                    desc->elementNs, desc->typeDescription, option, heap, value, size, &found );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadValue		[webservices.@]
 */
HRESULT WINAPI WsReadValue( WS_XML_READER *handle, WS_VALUE_TYPE value_type, void *value, ULONG size,
                            WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    WS_TYPE type = map_value_type( value_type );
    BOOL found;
    HRESULT hr;

    TRACE( "%p %u %p %lu %p\n", handle, type, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !value || type == ~0u) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, type, NULL, NULL, NULL, WS_READ_REQUIRED_VALUE,
                    NULL, value, size, &found );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadAttribute		[webservices.@]
 */
HRESULT WINAPI WsReadAttribute( WS_XML_READER *handle, const WS_ATTRIBUTE_DESCRIPTION *desc,
                                WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size,
                                WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    BOOL found;
    HRESULT hr;

    TRACE( "%p %p %#x %p %p %lu %p\n", handle, desc, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !desc || !value) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type) hr = WS_E_INVALID_OPERATION;
    else hr = read_type( reader, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->attributeLocalName,
                         desc->attributeNs, desc->typeDescription, option, heap, value, size, &found );

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static inline BOOL is_utf8( const unsigned char *data, ULONG size, ULONG *offset )
{
    static const char bom[] = {0xef,0xbb,0xbf};
    return (size >= sizeof(bom) && !memcmp( data, bom, sizeof(bom) ) && (*offset = sizeof(bom))) ||
           (size > 2 && !(*offset = 0));
}

static inline BOOL is_utf16le( const unsigned char *data, ULONG size, ULONG *offset )
{
    static const char bom[] = {0xff,0xfe};
    return (size >= sizeof(bom) && !memcmp( data, bom, sizeof(bom) ) && (*offset = sizeof(bom))) ||
           (size >= 4 && data[0] == '<' && !data[1] && !(*offset = 0));
}

static WS_CHARSET detect_charset( const unsigned char *data, ULONG size, ULONG *offset )
{
    WS_CHARSET ret = 0;

    /* FIXME: parse xml declaration */

    if (is_utf16le( data, size, offset )) ret = WS_CHARSET_UTF16LE;
    else if (is_utf8( data, size, offset )) ret = WS_CHARSET_UTF8;
    else
    {
        FIXME( "charset not recognized\n" );
        return 0;
    }

    TRACE( "detected charset %u\n", ret );
    return ret;
}

static HRESULT utf16le_to_utf8( const unsigned char *data, ULONG size, unsigned char **buf, ULONG *buflen )
{
    if (size % sizeof(WCHAR)) return E_INVALIDARG;
    *buflen = WideCharToMultiByte( CP_UTF8, 0, (const WCHAR *)data, size / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (!(*buf = malloc( *buflen ))) return E_OUTOFMEMORY;
    WideCharToMultiByte( CP_UTF8, 0, (const WCHAR *)data, size / sizeof(WCHAR), (char *)*buf, *buflen, NULL, NULL );
    return S_OK;
}

static HRESULT set_input_buffer( struct reader *reader, const unsigned char *data, ULONG size )
{
    reader->input_type  = WS_XML_READER_INPUT_TYPE_BUFFER;
    reader->input_buf   = NULL;

    if (reader->input_enc == WS_XML_READER_ENCODING_TYPE_TEXT && reader->input_charset == WS_CHARSET_UTF16LE)
    {
        unsigned char *buf;
        ULONG buflen;
        HRESULT hr;

        if ((hr = utf16le_to_utf8( data, size, &buf, &buflen )) != S_OK) return hr;
        free( reader->input_conv );
        reader->read_bufptr = reader->input_conv = buf;
        reader->read_size   = reader->input_size = buflen;
    }
    else
    {
        reader->read_bufptr = data;
        reader->read_size   = reader->input_size = size;
    }

    reader->read_pos         = 0;
    reader->text_conv_offset = 0;
    return S_OK;
}

static void set_input_stream( struct reader *reader, WS_READ_CALLBACK callback, void *state )
{
    reader->input_type     = WS_XML_READER_INPUT_TYPE_STREAM;
    reader->input_cb       = callback;
    reader->input_cb_state = state;
    reader->input_buf      = NULL;
    reader->input_size     = STREAM_BUFSIZE;

    if (reader->read_pos >= reader->read_size) reader->read_size = 0;
    else
    {
        memmove( reader->stream_buf, reader->stream_buf + reader->read_pos, reader->read_size - reader->read_pos );
        reader->read_size -= reader->read_pos;
    }
    reader->read_pos         = 0;
    reader->read_bufptr      = reader->stream_buf;
    reader->text_conv_offset = 0;
}

/**************************************************************************
 *          WsSetInput		[webservices.@]
 */
HRESULT WINAPI WsSetInput( WS_XML_READER *handle, const WS_XML_READER_ENCODING *encoding,
                           const WS_XML_READER_INPUT *input, const WS_XML_READER_PROPERTY *properties,
                           ULONG count, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    struct node *node;
    ULONG i, offset = 0;
    HRESULT hr;

    TRACE( "%p %p %p %p %lu %p\n", handle, encoding, input, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( reader->prop, reader->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) goto done;
    }

    if ((hr = init_reader( reader )) != S_OK) goto done;

    switch (encoding->encodingType)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:
    {
        if (input->inputType == WS_XML_READER_INPUT_TYPE_BUFFER)
        {
            const WS_XML_READER_TEXT_ENCODING *text = (const WS_XML_READER_TEXT_ENCODING *)encoding;
            const WS_XML_READER_BUFFER_INPUT *buf = (const WS_XML_READER_BUFFER_INPUT *)input;
            if (text->charSet != WS_CHARSET_AUTO) reader->input_charset = text->charSet;
            else reader->input_charset = detect_charset( buf->encodedData, buf->encodedDataSize, &offset );
        }

        reader->input_enc = WS_XML_READER_ENCODING_TYPE_TEXT;
        break;
    }
    case WS_XML_READER_ENCODING_TYPE_BINARY:
    {
        const WS_XML_READER_BINARY_ENCODING *bin = (const WS_XML_READER_BINARY_ENCODING *)encoding;
        reader->input_enc     = WS_XML_READER_ENCODING_TYPE_BINARY;
        reader->input_charset = 0;
        reader->dict_static   = bin->staticDictionary ? bin->staticDictionary : &dict_builtin_static.dict;
        reader->dict          = bin->dynamicDictionary ? bin->dynamicDictionary : &dict_builtin.dict;
        break;
    }
    default:
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        hr = E_NOTIMPL;
        goto done;
    }

    switch (input->inputType)
    {
    case WS_XML_READER_INPUT_TYPE_BUFFER:
    {
        const WS_XML_READER_BUFFER_INPUT *buf = (const WS_XML_READER_BUFFER_INPUT *)input;
        hr = set_input_buffer( reader, (const unsigned char *)buf->encodedData + offset, buf->encodedDataSize - offset );
        if (hr != S_OK) goto done;
        break;
    }
    case WS_XML_READER_INPUT_TYPE_STREAM:
    {
        const WS_XML_READER_STREAM_INPUT *stream = (const WS_XML_READER_STREAM_INPUT *)input;
        if (!reader->stream_buf && !(reader->stream_buf = malloc( STREAM_BUFSIZE )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        set_input_stream( reader, stream->readCallback, stream->readCallbackState );
        break;
    }
    default:
        FIXME( "input type %u not supported\n", input->inputType );
        hr = E_NOTIMPL;
        goto done;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) hr = E_OUTOFMEMORY;
    else read_insert_bof( reader, node );

done:
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT set_input_xml_buffer( struct reader *reader, struct xmlbuf *xmlbuf )
{
    reader->input_type    = WS_XML_READER_INPUT_TYPE_BUFFER;
    reader->input_buf     = xmlbuf;
    reader->input_enc     = xmlbuf->encoding;
    reader->input_charset = xmlbuf->charset;
    reader->dict_static   = xmlbuf->dict_static;
    reader->dict          = xmlbuf->dict;

    if (reader->input_enc == WS_XML_READER_ENCODING_TYPE_TEXT && reader->input_charset == WS_CHARSET_UTF16LE)
    {
        unsigned char *buf;
        ULONG buflen;
        HRESULT hr;

        if ((hr = utf16le_to_utf8( xmlbuf->bytes.bytes, xmlbuf->bytes.length, &buf, &buflen )) != S_OK) return hr;
        free( reader->input_conv );
        reader->read_bufptr = reader->input_conv = buf;
        reader->read_size   = reader->input_size = buflen;
    }
    else
    {
        reader->read_bufptr = xmlbuf->bytes.bytes;
        reader->read_size   = reader->input_size = xmlbuf->bytes.length;
    }

    reader->read_pos         = 0;
    reader->text_conv_offset = 0;
    return S_OK;
}

/**************************************************************************
 *          WsSetInputToBuffer		[webservices.@]
 */
HRESULT WINAPI WsSetInputToBuffer( WS_XML_READER *handle, WS_XML_BUFFER *buffer,
                                   const WS_XML_READER_PROPERTY *properties, ULONG count,
                                   WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    struct xmlbuf *xmlbuf = (struct xmlbuf *)buffer;
    struct node *node;
    HRESULT hr;
    ULONG i;

    TRACE( "%p %p %p %lu %p\n", handle, buffer, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !xmlbuf) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    for (i = 0; i < count; i++)
    {
        hr = prop_set( reader->prop, reader->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) goto done;
    }

    if ((hr = init_reader( reader )) != S_OK) goto done;
    if ((hr = set_input_xml_buffer( reader, xmlbuf )) != S_OK) goto done;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) hr = E_OUTOFMEMORY;
    else read_insert_bof( reader, node );

done:
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsGetReaderPosition		[webservices.@]
 */
HRESULT WINAPI WsGetReaderPosition( WS_XML_READER *handle, WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !pos) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_buf) hr = WS_E_INVALID_OPERATION;
    else
    {
        pos->buffer = (WS_XML_BUFFER *)reader->input_buf;
        pos->node   = reader->current;
    }

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsSetReaderPosition		[webservices.@]
 */
HRESULT WINAPI WsSetReaderPosition( WS_XML_READER *handle, const WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !pos) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC || (struct xmlbuf *)pos->buffer != reader->input_buf)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_buf) hr = WS_E_INVALID_OPERATION;
    else reader->current = pos->node;

    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT utf8_to_base64( const WS_XML_UTF8_TEXT *utf8, WS_XML_BASE64_TEXT *base64 )
{
    if (utf8->value.length % 4) return WS_E_INVALID_FORMAT;
    if (!(base64->bytes = malloc( utf8->value.length * 3 / 4 ))) return E_OUTOFMEMORY;
    base64->length = decode_base64( utf8->value.bytes, utf8->value.length, base64->bytes );
    return S_OK;
}

/**************************************************************************
 *          WsReadBytes		[webservices.@]
 */
HRESULT WINAPI WsReadBytes( WS_XML_READER *handle, void *bytes, ULONG max_count, ULONG *count, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %lu %p %p\n", handle, bytes, max_count, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }
    if (!count)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *count = 0;
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_TEXT && bytes)
    {
        const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)reader->current;
        WS_XML_BASE64_TEXT base64;

        if ((hr = utf8_to_base64( (const WS_XML_UTF8_TEXT *)text->text, &base64 )) != S_OK) goto done;
        if (reader->text_conv_offset == base64.length)
        {
            free( base64.bytes );
            hr = read_node( reader );
            goto done;
        }
        *count = min( base64.length - reader->text_conv_offset, max_count );
        memcpy( bytes, base64.bytes + reader->text_conv_offset, *count );
        reader->text_conv_offset += *count;
        free( base64.bytes );
    }

done:
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT utf8_to_utf16( const WS_XML_UTF8_TEXT *utf8, WS_XML_UTF16_TEXT *utf16 )
{
    int len = MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, NULL, 0 );
    if (!(utf16->bytes = malloc( len * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, (WCHAR *)utf16->bytes, len );
    utf16->byteCount = len * sizeof(WCHAR);
    return S_OK;
}

/**************************************************************************
 *          WsReadChars		[webservices.@]
 */
HRESULT WINAPI WsReadChars( WS_XML_READER *handle, WCHAR *chars, ULONG max_count, ULONG *count, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %lu %p %p\n", handle, chars, max_count, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }
    if (!count)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *count = 0;
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_TEXT && chars)
    {
        const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)reader->current;
        WS_XML_UTF16_TEXT utf16;
        HRESULT hr;

        if ((hr = utf8_to_utf16( (const WS_XML_UTF8_TEXT *)text->text, &utf16 )) != S_OK) goto done;
        if (reader->text_conv_offset == utf16.byteCount / sizeof(WCHAR))
        {
            free( utf16.bytes );
            hr = read_node( reader );
            goto done;
        }
        *count = min( utf16.byteCount / sizeof(WCHAR) - reader->text_conv_offset, max_count );
        memcpy( chars, utf16.bytes + reader->text_conv_offset * sizeof(WCHAR), *count * sizeof(WCHAR) );
        reader->text_conv_offset += *count;
        free( utf16.bytes );
    }

done:
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

/**************************************************************************
 *          WsReadCharsUtf8		[webservices.@]
 */
HRESULT WINAPI WsReadCharsUtf8( WS_XML_READER *handle, BYTE *bytes, ULONG max_count, ULONG *count, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = S_OK;

    TRACE( "%p %p %lu %p %p\n", handle, bytes, max_count, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type)
    {
        hr = WS_E_INVALID_OPERATION;
        goto done;
    }
    if (!count)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *count = 0;
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_TEXT && bytes)
    {
        const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)reader->current;
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text->text;

        if (reader->text_conv_offset == utf8->value.length)
        {
            hr = read_node( reader );
            goto done;
        }
        *count = min( utf8->value.length - reader->text_conv_offset, max_count );
        memcpy( bytes, utf8->value.bytes + reader->text_conv_offset, *count );
        reader->text_conv_offset += *count;
    }

done:
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static HRESULT move_to_element( struct reader *reader )
{
    HRESULT hr;
    if (node_type( reader->current ) == WS_XML_NODE_TYPE_BOF &&
        (hr = read_move_to( reader, WS_MOVE_TO_CHILD_NODE, NULL )) != S_OK) return hr;
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT) return E_FAIL;
    return S_OK;
}

static HRESULT copy_tree( struct reader *reader, WS_XML_WRITER *writer )
{
    const struct node *node, *parent;
    BOOL done = FALSE;
    HRESULT hr;

    if ((hr = move_to_element( reader )) != S_OK) return hr;
    parent = reader->current;
    for (;;)
    {
        node = reader->current;
        if ((hr = WsWriteNode( writer, (const WS_XML_NODE *)node, NULL )) != S_OK) break;
        if (node_type( node ) == WS_XML_NODE_TYPE_END_ELEMENT && node->parent == parent) done = TRUE;
        if ((hr = read_next_node( reader )) != S_OK || done) break;
    }
    return hr;
}

/**************************************************************************
 *          WsReadXmlBuffer		[webservices.@]
 */
HRESULT WINAPI WsReadXmlBuffer( WS_XML_READER *handle, WS_HEAP *heap, WS_XML_BUFFER **ret, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    WS_XML_WRITER *writer = NULL;
    WS_XML_BUFFER *buffer = NULL;
    HRESULT hr;

    TRACE( "%p %p %p %p\n", handle, heap, ret, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !heap) return E_INVALIDARG;
    if (!ret) return E_FAIL;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if (!reader->input_type) hr = WS_E_INVALID_OPERATION;
    else
    {
        if ((hr = WsCreateWriter( NULL, 0, &writer, NULL )) != S_OK) goto done;
        if ((hr = WsCreateXmlBuffer( heap, NULL, 0, &buffer, NULL )) != S_OK) goto done;
        if ((hr = WsSetOutputToBuffer( writer, buffer, NULL, 0, NULL )) != S_OK) goto done;
        if ((hr = copy_tree( reader, writer )) == S_OK) *ret = buffer;
    }

done:
    if (hr != S_OK) free_xmlbuf( (struct xmlbuf *)buffer );
    WsFreeWriter( writer );
    LeaveCriticalSection( &reader->cs );
    TRACE( "returning %#lx\n", hr );
    return hr;
}

HRESULT create_header_buffer( WS_XML_READER *handle, WS_HEAP *heap, WS_XML_BUFFER **ret )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr = WS_E_QUOTA_EXCEEDED;
    struct xmlbuf *xmlbuf;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if ((xmlbuf = alloc_xmlbuf( heap, reader->read_pos, reader->input_enc, reader->input_charset,
                                reader->dict_static, reader->dict )))
    {
        memcpy( xmlbuf->bytes.bytes, reader->read_bufptr, reader->read_pos );
        xmlbuf->bytes.length = reader->read_pos;
        *ret = (WS_XML_BUFFER *)xmlbuf;
        hr = S_OK;
    }

    LeaveCriticalSection( &reader->cs );
    return hr;
}

HRESULT get_param_desc( const WS_STRUCT_DESCRIPTION *desc, USHORT index, const WS_FIELD_DESCRIPTION **ret )
{
    if (index >= desc->fieldCount) return E_INVALIDARG;
    *ret = desc->fields[index];
    return S_OK;
}

static ULONG get_field_size( const WS_FIELD_DESCRIPTION *desc )
{
    if (desc->options & WS_FIELD_POINTER) return sizeof(void *);
    return get_type_size( desc->type, desc->typeDescription );
}

static HRESULT read_param( struct reader *reader, const WS_FIELD_DESCRIPTION *desc, WS_HEAP *heap, void *ret )
{
    if (!ret && !(ret = ws_alloc_zero( heap, get_field_size(desc) ))) return WS_E_QUOTA_EXCEEDED;
    return read_type_field( reader, NULL, desc, heap, ret, 0 );
}

static HRESULT read_param_array( struct reader *reader, const WS_FIELD_DESCRIPTION *desc, WS_HEAP *heap,
                                 void **ret, ULONG *count )
{
    if (!ret && !(ret = ws_alloc_zero( heap, sizeof(void **) ))) return WS_E_QUOTA_EXCEEDED;
    return read_type_array( reader, desc, heap, ret, count );
}

static void set_array_len( const WS_PARAMETER_DESCRIPTION *params, ULONG count, ULONG index, ULONG len,
                           const void **args )
{
    ULONG i, *ptr;
    for (i = 0; i < count; i++)
    {
        if (params[i].outputMessageIndex != index || params[i].parameterType != WS_PARAMETER_TYPE_ARRAY_COUNT)
            continue;
        if ((ptr = *(ULONG **)args[i])) *ptr = len;
        break;
    }
}

HRESULT read_output_params( WS_XML_READER *handle, WS_HEAP *heap, const WS_ELEMENT_DESCRIPTION *desc,
                            const WS_PARAMETER_DESCRIPTION *params, ULONG count, const void **args )
{
    struct reader *reader = (struct reader *)handle;
    const WS_STRUCT_DESCRIPTION *desc_struct;
    const WS_FIELD_DESCRIPTION *desc_field;
    ULONG i, len;
    HRESULT hr;

    if (desc->type != WS_STRUCT_TYPE || !(desc_struct = desc->typeDescription)) return E_INVALIDARG;

    EnterCriticalSection( &reader->cs );

    if (reader->magic != READER_MAGIC)
    {
        LeaveCriticalSection( &reader->cs );
        return E_INVALIDARG;
    }

    if ((hr = start_mapping( reader, WS_ELEMENT_TYPE_MAPPING, desc->elementLocalName, desc->elementNs )) != S_OK)
        goto done;

    for (i = 0; i < count; i++)
    {
        if (params[i].outputMessageIndex == INVALID_PARAMETER_INDEX) continue;
        if (params[i].parameterType == WS_PARAMETER_TYPE_MESSAGES)
        {
            FIXME( "messages type not supported\n" );
            hr = E_NOTIMPL;
            goto done;
        }
        if ((hr = get_param_desc( desc_struct, params[i].outputMessageIndex, &desc_field )) != S_OK) goto done;
        if (params[i].parameterType == WS_PARAMETER_TYPE_NORMAL)
        {
            void *ptr = *(void **)args[i];
            if ((hr = read_param( reader, desc_field, heap, ptr )) != S_OK) goto done;
        }
        else if (params[i].parameterType == WS_PARAMETER_TYPE_ARRAY)
        {
            void **ptr = *(void ***)args[i];
            if ((hr = read_param_array( reader, desc_field, heap, ptr, &len )) != S_OK) goto done;
            set_array_len( params, count, params[i].outputMessageIndex, len, args );
        }
    }

    if (desc_struct->structOptions & WS_STRUCT_IGNORE_TRAILING_ELEMENT_CONTENT)
    {
        struct node *parent = find_parent( reader );
        parent->flags |= NODE_FLAG_IGNORE_TRAILING_ELEMENT_CONTENT;
    }

    hr = end_mapping( reader, WS_ELEMENT_TYPE_MAPPING );

done:
    LeaveCriticalSection( &reader->cs );
    return hr;
}
