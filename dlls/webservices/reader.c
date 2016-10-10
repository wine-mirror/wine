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
#include "winnls.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

const char *debugstr_xmlstr( const WS_XML_STRING *str )
{
    if (!str) return "(null)";
    return debugstr_an( (const char *)str->bytes, str->length );
}

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

static const struct prop_desc error_props[] =
{
    { sizeof(ULONG), TRUE },    /* WS_ERROR_PROPERTY_STRING_COUNT */
    { sizeof(ULONG), FALSE },   /* WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE */
    { sizeof(LANGID), FALSE }   /* WS_ERROR_PROPERTY_LANGID */
};

struct error
{
    ULONG       prop_count;
    struct prop prop[sizeof(error_props)/sizeof(error_props[0])];
};

static struct error *alloc_error(void)
{
    static const ULONG count = sizeof(error_props)/sizeof(error_props[0]);
    struct error *ret;
    ULONG size = sizeof(*ret) + prop_size( error_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    prop_init( error_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

/**************************************************************************
 *          WsCreateError		[webservices.@]
 */
HRESULT WINAPI WsCreateError( const WS_ERROR_PROPERTY *properties, ULONG count, WS_ERROR **handle )
{
    struct error *error;
    LANGID langid = GetUserDefaultUILanguage();
    HRESULT hr;
    ULONG i;

    TRACE( "%p %u %p\n", properties, count, handle );

    if (!handle) return E_INVALIDARG;
    if (!(error = alloc_error())) return E_OUTOFMEMORY;

    prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_LANGID, &langid, sizeof(langid) );
    for (i = 0; i < count; i++)
    {
        if (properties[i].id == WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE)
        {
            heap_free( error );
            return E_INVALIDARG;
        }
        hr = prop_set( error->prop, error->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK)
        {
            heap_free( error );
            return hr;
        }
    }

    *handle = (WS_ERROR *)error;
    return S_OK;
}

/**************************************************************************
 *          WsFreeError		[webservices.@]
 */
void WINAPI WsFreeError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;

    TRACE( "%p\n", handle );
    heap_free( error );
}

/**************************************************************************
 *          WsResetError	[webservices.@]
 */
HRESULT WINAPI WsResetError( WS_ERROR *handle )
{
    struct error *error = (struct error *)handle;
    ULONG code;

    TRACE( "%p\n", handle );

    if (!handle) return E_INVALIDARG;

    /* FIXME: release strings added with WsAddErrorString when it's implemented, reset string count */
    code = 0;
    return prop_set( error->prop, error->prop_count, WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE, &code, sizeof(code) );
}

static const struct prop_desc heap_props[] =
{
    { sizeof(SIZE_T), FALSE }, /* WS_HEAP_PROPERTY_MAX_SIZE */
    { sizeof(SIZE_T), FALSE }, /* WS_HEAP_PROPERTY_TRIM_SIZE */
    { sizeof(SIZE_T), TRUE },  /* WS_HEAP_PROPERTY_REQUESTED_SIZE */
    { sizeof(SIZE_T), TRUE }   /* WS_HEAP_PROPERTY_ACTUAL_SIZE */
};

struct heap
{
    HANDLE      handle;
    ULONG       prop_count;
    struct prop prop[sizeof(heap_props)/sizeof(heap_props[0])];
};

static BOOL ensure_heap( struct heap *heap )
{
    SIZE_T size;
    if (heap->handle) return TRUE;
    if (prop_get( heap->prop, heap->prop_count, WS_HEAP_PROPERTY_MAX_SIZE, &size, sizeof(size) ) != S_OK)
        return FALSE;
    if (!(heap->handle = HeapCreate( 0, 0, size ))) return FALSE;
    return TRUE;
}

void *ws_alloc( WS_HEAP *handle, SIZE_T size )
{
    struct heap *heap = (struct heap *)handle;
    if (!ensure_heap( heap )) return NULL;
    return HeapAlloc( heap->handle, 0, size );
}

static void *ws_alloc_zero( WS_HEAP *handle, SIZE_T size )
{
    struct heap *heap = (struct heap *)handle;
    if (!ensure_heap( heap )) return NULL;
    return HeapAlloc( heap->handle, HEAP_ZERO_MEMORY, size );
}

void *ws_realloc( WS_HEAP *handle, void *ptr, SIZE_T size )
{
    struct heap *heap = (struct heap *)handle;
    if (!ensure_heap( heap )) return NULL;
    return HeapReAlloc( heap->handle, 0, ptr, size );
}

static void *ws_realloc_zero( WS_HEAP *handle, void *ptr, SIZE_T size )
{
    struct heap *heap = (struct heap *)handle;
    if (!ensure_heap( heap )) return NULL;
    return HeapReAlloc( heap->handle, HEAP_ZERO_MEMORY, ptr, size );
}

void ws_free( WS_HEAP *handle, void *ptr )
{
    struct heap *heap = (struct heap *)handle;
    if (!heap->handle) return;
    HeapFree( heap->handle, 0, ptr );
}

/**************************************************************************
 *          WsAlloc		[webservices.@]
 */
HRESULT WINAPI WsAlloc( WS_HEAP *handle, SIZE_T size, void **ptr, WS_ERROR *error )
{
    void *mem;

    TRACE( "%p %u %p %p\n", handle, (ULONG)size, ptr, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || !ptr) return E_INVALIDARG;

    if (!(mem = ws_alloc( handle, size ))) return E_OUTOFMEMORY;
    *ptr = mem;
    return S_OK;
}

static struct heap *alloc_heap(void)
{
    static const ULONG count = sizeof(heap_props)/sizeof(heap_props[0]);
    struct heap *ret;
    ULONG size = sizeof(*ret) + prop_size( heap_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    prop_init( heap_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

/**************************************************************************
 *          WsCreateHeap		[webservices.@]
 */
HRESULT WINAPI WsCreateHeap( SIZE_T max_size, SIZE_T trim_size, const WS_HEAP_PROPERTY *properties,
                             ULONG count, WS_HEAP **handle, WS_ERROR *error )
{
    struct heap *heap;

    TRACE( "%u %u %p %u %p %p\n", (ULONG)max_size, (ULONG)trim_size, properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle || count) return E_INVALIDARG;
    if (!(heap = alloc_heap())) return E_OUTOFMEMORY;

    prop_set( heap->prop, heap->prop_count, WS_HEAP_PROPERTY_MAX_SIZE, &max_size, sizeof(max_size) );
    prop_set( heap->prop, heap->prop_count, WS_HEAP_PROPERTY_TRIM_SIZE, &trim_size, sizeof(trim_size) );

    *handle = (WS_HEAP *)heap;
    return S_OK;
}

/**************************************************************************
 *          WsFreeHeap		[webservices.@]
 */
void WINAPI WsFreeHeap( WS_HEAP *handle )
{
    struct heap *heap = (struct heap *)handle;

    TRACE( "%p\n", handle );

    if (!heap) return;
    HeapDestroy( heap->handle );
    heap_free( heap );
}

/**************************************************************************
 *          WsResetHeap		[webservices.@]
 */
HRESULT WINAPI WsResetHeap( WS_HEAP *handle, WS_ERROR *error )
{
    struct heap *heap = (struct heap *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!heap) return E_INVALIDARG;

    HeapDestroy( heap->handle );
    heap->handle = NULL;
    return S_OK;
}

struct node *alloc_node( WS_XML_NODE_TYPE type )
{
    struct node *ret;

    if (!(ret = heap_alloc_zero( sizeof(*ret) ))) return NULL;
    ret->hdr.node.nodeType = type;
    list_init( &ret->entry );
    list_init( &ret->children );
    return ret;
}

void free_attribute( WS_XML_ATTRIBUTE *attr )
{
    if (!attr) return;
    heap_free( attr->prefix );
    heap_free( attr->localName );
    heap_free( attr->ns );
    heap_free( attr->value );
    heap_free( attr );
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
        heap_free( elem->attributes );
        heap_free( elem->prefix );
        heap_free( elem->localName );
        heap_free( elem->ns );
        break;
    }
    case WS_XML_NODE_TYPE_TEXT:
    {
        WS_XML_TEXT_NODE *text = (WS_XML_TEXT_NODE *)node;
        heap_free( text->text );
        break;
    }
    case WS_XML_NODE_TYPE_COMMENT:
    {
        WS_XML_COMMENT_NODE *comment = (WS_XML_COMMENT_NODE *)node;
        heap_free( comment->value.bytes );
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
    heap_free( node );
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

static WS_XML_ATTRIBUTE *dup_attribute( const WS_XML_ATTRIBUTE *src )
{
    WS_XML_ATTRIBUTE *dst;
    const WS_XML_STRING *prefix = (src->prefix && src->prefix->length) ? src->prefix : NULL;
    const WS_XML_STRING *localname = src->localName;
    const WS_XML_STRING *ns = src->localName;

    if (!(dst = heap_alloc( sizeof(*dst) ))) return NULL;
    dst->singleQuote = src->singleQuote;
    dst->isXmlNs     = src->isXmlNs;
    if (prefix && !(dst->prefix = alloc_xml_string( prefix->bytes, prefix->length ))) goto error;
    if (localname && !(dst->localName = alloc_xml_string( localname->bytes, localname->length ))) goto error;
    if (ns && !(dst->ns = alloc_xml_string( ns->bytes, ns->length ))) goto error;
    return dst;

error:
    free_attribute( dst );
    return NULL;
}

static WS_XML_ATTRIBUTE **dup_attributes( WS_XML_ATTRIBUTE * const *src, ULONG count )
{
    WS_XML_ATTRIBUTE **dst;
    ULONG i;

    if (!(dst = heap_alloc( sizeof(*dst) * count ))) return NULL;
    for (i = 0; i < count; i++)
    {
        if (!(dst[i] = dup_attribute( src[i] )))
        {
            for (; i > 0; i--) free_attribute( dst[i - 1] );
            heap_free( dst );
            return NULL;
        }
    }
    return dst;
}

static struct node *dup_element_node( const WS_XML_ELEMENT_NODE *src )
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

    if (count && !(dst->attributes = dup_attributes( attrs, count ))) goto error;
    dst->attributeCount = count;

    if (prefix && !(dst->prefix = alloc_xml_string( prefix->bytes, prefix->length ))) goto error;
    if (localname && !(dst->localName = alloc_xml_string( localname->bytes, localname->length ))) goto error;
    if (ns && !(dst->ns = alloc_xml_string( ns->bytes, ns->length ))) goto error;
    return node;

error:
    free_node( node );
    return NULL;
}

static struct node *dup_text_node( const WS_XML_TEXT_NODE *src )
{
    struct node *node;
    WS_XML_TEXT_NODE *dst;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return NULL;
    dst = (WS_XML_TEXT_NODE *)node;

    if (src->text)
    {
        WS_XML_UTF8_TEXT *utf8;
        const WS_XML_UTF8_TEXT *utf8_src = (WS_XML_UTF8_TEXT *)src->text;
        if (!(utf8 = alloc_utf8_text( utf8_src->value.bytes, utf8_src->value.length )))
        {
            free_node( node );
            return NULL;
        }
        dst->text = &utf8->text;
    }
    return node;
}

static struct node *dup_comment_node( const WS_XML_COMMENT_NODE *src )
{
    struct node *node;
    WS_XML_COMMENT_NODE *dst;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return NULL;
    dst = (WS_XML_COMMENT_NODE *)node;

    if (src->value.length && !(dst->value.bytes = heap_alloc( src->value.length )))
    {
        free_node( node );
        return NULL;
    }
    memcpy( dst->value.bytes, src->value.bytes, src->value.length );
    dst->value.length = src->value.length;
    return node;
}

static struct node *dup_node( const struct node *src )
{
    switch (node_type( src ))
    {
    case WS_XML_NODE_TYPE_ELEMENT:
        return dup_element_node( &src->hdr );

    case WS_XML_NODE_TYPE_TEXT:
        return dup_text_node( (const WS_XML_TEXT_NODE *)src );

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

static HRESULT dup_tree( struct node **dst, const struct node *src )
{
    struct node *parent;
    const struct node *child;

    if (!*dst && !(*dst = dup_node( src ))) return E_OUTOFMEMORY;
    parent = *dst;

    LIST_FOR_EACH_ENTRY( child, &src->children, struct node, entry )
    {
        HRESULT hr = E_OUTOFMEMORY;
        struct node *new_child;

        if (!(new_child = dup_node( child )) || (hr = dup_tree( &new_child, child )) != S_OK)
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
    WS_XML_STRING str;
    WS_XML_STRING ns;
};

struct reader
{
    ULONG                    read_size;
    ULONG                    read_pos;
    const unsigned char     *read_bufptr;
    enum reader_state        state;
    struct node             *root;
    struct node             *current;
    ULONG                    current_attr;
    struct node             *last;
    struct prefix           *prefixes;
    ULONG                    nb_prefixes;
    ULONG                    nb_prefixes_allocated;
    WS_XML_READER_INPUT_TYPE input_type;
    struct xmlbuf           *input_buf;
    const unsigned char     *input_data;
    ULONG                    input_size;
    ULONG                    prop_count;
    struct prop              prop[sizeof(reader_props)/sizeof(reader_props[0])];
};

static struct reader *alloc_reader(void)
{
    static const ULONG count = sizeof(reader_props)/sizeof(reader_props[0]);
    struct reader *ret;
    ULONG size = sizeof(*ret) + prop_size( reader_props, count );

    if (!(ret = heap_alloc_zero( size ))) return NULL;
    if (!(ret->prefixes = heap_alloc_zero( sizeof(*ret->prefixes) )))
    {
        heap_free( ret );
        return NULL;
    }
    ret->nb_prefixes = ret->nb_prefixes_allocated = 1;

    prop_init( reader_props, count, ret->prop, &ret[1] );
    ret->prop_count = count;
    return ret;
}

static void clear_prefixes( struct prefix *prefixes, ULONG count )
{
    ULONG i;
    for (i = 0; i < count; i++)
    {
        heap_free( prefixes[i].str.bytes );
        prefixes[i].str.bytes  = NULL;
        prefixes[i].str.length = 0;

        heap_free( prefixes[i].ns.bytes );
        prefixes[i].ns.bytes  = NULL;
        prefixes[i].ns.length = 0;
    }
}

static void free_reader( struct reader *reader )
{
    if (!reader) return;
    destroy_nodes( reader->root );
    clear_prefixes( reader->prefixes, reader->nb_prefixes );
    heap_free( reader->prefixes );
    heap_free( reader );
}

HRESULT copy_node( WS_XML_READER *handle, struct node **node )
{
    struct reader *reader = (struct reader *)handle;
    return dup_tree( node, reader->current );
}

static HRESULT set_prefix( struct prefix *prefix, const WS_XML_STRING *str, const WS_XML_STRING *ns )
{
    if (str)
    {
        heap_free( prefix->str.bytes );
        if (!(prefix->str.bytes = heap_alloc( str->length ))) return E_OUTOFMEMORY;
        memcpy( prefix->str.bytes, str->bytes, str->length );
        prefix->str.length = str->length;
    }

    heap_free( prefix->ns.bytes );
    if (!(prefix->ns.bytes = heap_alloc( ns->length ))) return E_OUTOFMEMORY;
    memcpy( prefix->ns.bytes, ns->bytes, ns->length );
    prefix->ns.length = ns->length;

    return S_OK;
}

static HRESULT bind_prefix( struct reader *reader, const WS_XML_STRING *prefix, const WS_XML_STRING *ns )
{
    ULONG i;
    HRESULT hr;

    for (i = 0; i < reader->nb_prefixes; i++)
    {
        if (WsXmlStringEquals( prefix, &reader->prefixes[i].str, NULL ) == S_OK)
            return set_prefix( &reader->prefixes[i], NULL, ns );
    }
    if (i >= reader->nb_prefixes_allocated)
    {
        ULONG new_size = reader->nb_prefixes_allocated * sizeof(*reader->prefixes) * 2;
        struct prefix *tmp = heap_realloc_zero( reader->prefixes, new_size  );
        if (!tmp) return E_OUTOFMEMORY;
        reader->prefixes = tmp;
        reader->nb_prefixes_allocated *= 2;
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
        if (WsXmlStringEquals( prefix, &reader->prefixes[i].str, NULL ) == S_OK)
            return &reader->prefixes[i].ns;
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

static HRESULT read_init_state( struct reader *reader )
{
    struct node *node;

    destroy_nodes( reader->root );
    reader->root = NULL;
    reader->input_buf = NULL;
    clear_prefixes( reader->prefixes, reader->nb_prefixes );
    reader->nb_prefixes = 1;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_EOF ))) return E_OUTOFMEMORY;
    read_insert_eof( reader, node );
    reader->state = READER_STATE_INITIAL;
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
    WS_CHARSET charset = WS_CHARSET_UTF8;
    BOOL read_decl = TRUE;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (!(reader = alloc_reader())) return E_OUTOFMEMORY;

    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, sizeof(max_depth) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, sizeof(max_attrs) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_READ_DECLARATION, &read_decl, sizeof(read_decl) );
    prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_CHARSET, &charset, sizeof(charset) );
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

    if ((hr = read_init_state( reader )) != S_OK)
    {
        free_reader( reader );
        return hr;
    }

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
    free_reader( reader );
}

/**************************************************************************
 *          WsFillReader		[webservices.@]
 */
HRESULT WINAPI WsFillReader( WS_XML_READER *handle, ULONG min_size, const WS_ASYNC_CONTEXT *ctx,
                             WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %u %p %p\n", handle, min_size, ctx, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    /* FIXME: add support for stream input */
    reader->read_size = min( min_size, reader->input_size );
    reader->read_pos  = 0;

    return S_OK;
}

/**************************************************************************
 *          WsGetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsGetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, void *buf,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;

    TRACE( "%p %u %p %u\n", handle, id, buf, size );
    return prop_get( error->prop, error->prop_count, id, buf, size );
}

/**************************************************************************
 *          WsGetErrorString		[webservices.@]
 */
HRESULT WINAPI WsGetErrorString( WS_ERROR *handle, ULONG index, WS_STRING *str )
{
    FIXME( "%p %u %p: stub\n", handle, index, str );
    return E_NOTIMPL;
}

/**************************************************************************
 *          WsGetHeapProperty		[webservices.@]
 */
HRESULT WINAPI WsGetHeapProperty( WS_HEAP *handle, WS_HEAP_PROPERTY_ID id, void *buf,
                                  ULONG size, WS_ERROR *error )
{
    struct heap *heap = (struct heap *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    return prop_get( heap->prop, heap->prop_count, id, buf, size );
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

    TRACE( "%p %s %d %p %p\n", handle, debugstr_xmlstr(prefix), required, ns, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !prefix || !ns) return E_INVALIDARG;
    if (reader->state != READER_STATE_STARTELEMENT) return WS_E_INVALID_OPERATION;

    if (!prefix->length)
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

    if (!found)
    {
        if (required) return WS_E_INVALID_FORMAT;
        *ns = NULL;
        return S_FALSE;
    }
    return S_OK;
}

/**************************************************************************
 *          WsGetReaderNode		[webservices.@]
 */
HRESULT WINAPI WsGetReaderNode( WS_XML_READER *handle, const WS_XML_NODE **node,
                                WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p %p\n", handle, node, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !node) return E_INVALIDARG;

    *node = &reader->current->hdr.node;
    return S_OK;
}

/**************************************************************************
 *          WsGetReaderProperty		[webservices.@]
 */
HRESULT WINAPI WsGetReaderProperty( WS_XML_READER *handle, WS_XML_READER_PROPERTY_ID id,
                                    void *buf, ULONG size, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %u %p %u %p\n", handle, id, buf, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader->input_type) return WS_E_INVALID_OPERATION;

    if (id == WS_XML_READER_PROPERTY_CHARSET)
    {
        WS_CHARSET charset;
        HRESULT hr;

        if ((hr = prop_get( reader->prop, reader->prop_count, id, &charset, size )) != S_OK) return hr;
        if (!charset) return WS_E_INVALID_FORMAT;
        *(WS_CHARSET *)buf = charset;
        return S_OK;
    }
    return prop_get( reader->prop, reader->prop_count, id, buf, size );
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

WS_XML_STRING *alloc_xml_string( const unsigned char *data, ULONG len )
{
    WS_XML_STRING *ret;

    if (!(ret = heap_alloc( sizeof(*ret) + len ))) return NULL;
    ret->length     = len;
    ret->bytes      = len ? (BYTE *)(ret + 1) : NULL;
    ret->dictionary = NULL;
    ret->id         = 0;
    if (data) memcpy( ret->bytes, data, len );
    return ret;
}

WS_XML_UTF8_TEXT *alloc_utf8_text( const unsigned char *data, ULONG len )
{
    WS_XML_UTF8_TEXT *ret;

    if (!(ret = heap_alloc( sizeof(*ret) + len ))) return NULL;
    ret->text.textType    = WS_XML_TEXT_TYPE_UTF8;
    ret->value.length     = len;
    ret->value.bytes      = len ? (BYTE *)(ret + 1) : NULL;
    ret->value.dictionary = NULL;
    ret->value.id         = 0;
    if (data) memcpy( ret->value.bytes, data, len );
    return ret;
}

static inline BOOL read_end_of_data( struct reader *reader )
{
    return reader->read_pos >= reader->read_size;
}

static inline const unsigned char *read_current_ptr( struct reader *reader )
{
    return &reader->read_bufptr[reader->read_pos];
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

static inline unsigned int read_utf8_char( struct reader *reader, unsigned int *skip )
{
    unsigned int len, res;
    unsigned char ch = reader->read_bufptr[reader->read_pos];
    const unsigned char *end;

    if (reader->read_pos >= reader->read_size) return 0;

    if (ch < 0x80)
    {
        *skip = 1;
        return ch;
    }
    len = utf8_length[ch - 0x80];
    if (reader->read_pos + len >= reader->read_size) return 0;
    end = reader->read_bufptr + reader->read_pos + len;
    res = ch & utf8_mask[len];

    switch (len)
    {
    case 3:
        if ((ch = end[-3] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
    case 2:
        if ((ch = end[-2] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
    case 1:
        if ((ch = end[-1] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        if (res < utf8_minval[len]) break;
        *skip = len + 1;
        return res;
    }

    return 0;
}

static inline void read_skip( struct reader *reader, unsigned int count )
{
    if (reader->read_pos + count > reader->read_size) return;
    reader->read_pos += count;
}

static inline void read_rewind( struct reader *reader, unsigned int count )
{
    reader->read_pos -= count;
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
    while (reader->read_pos < reader->read_size && read_isspace( reader->read_bufptr[reader->read_pos] ))
        reader->read_pos++;
}

static inline int read_cmp( struct reader *reader, const char *str, int len )
{
    const unsigned char *ptr = read_current_ptr( reader );

    if (len < 0) len = strlen( str );
    if (reader->read_pos + len > reader->read_size) return -1;
    while (len--)
    {
        if (*str != *ptr) return *ptr - *str;
        str++; ptr++;
    }
    return 0;
}

static HRESULT read_xmldecl( struct reader *reader )
{
    if (!reader->read_size) return WS_E_INVALID_FORMAT;

    if (read_cmp( reader, "<", 1 ) || read_cmp( reader, "<?", 2 ))
    {
        reader->state = READER_STATE_BOF;
        return S_OK;
    }
    if (read_cmp( reader, "<?xml ", 6 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 6 );

    /* FIXME: parse attributes */
    while (reader->read_pos < reader->read_size && reader->read_bufptr[reader->read_pos] != '?')
        reader->read_pos++;

    if (read_cmp( reader, "?>", 2 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 2 );

    reader->state = READER_STATE_BOF;
    return S_OK;
}

HRESULT append_attribute( WS_XML_ELEMENT_NODE *elem, WS_XML_ATTRIBUTE *attr )
{
    if (elem->attributeCount)
    {
        WS_XML_ATTRIBUTE **tmp;
        if (!(tmp = heap_realloc( elem->attributes, (elem->attributeCount + 1) * sizeof(attr) )))
            return E_OUTOFMEMORY;
        elem->attributes = tmp;
    }
    else if (!(elem->attributes = heap_alloc( sizeof(attr) ))) return E_OUTOFMEMORY;
    elem->attributes[elem->attributeCount++] = attr;
    return S_OK;
}

static HRESULT parse_name( const unsigned char *str, unsigned int len,
                           WS_XML_STRING **prefix, WS_XML_STRING **localname )
{
    const unsigned char *name_ptr = str, *prefix_ptr = NULL;
    unsigned int i, name_len = len, prefix_len = 0;

    for (i = 0; i < len; i++)
    {
        if (str[i] == ':')
        {
            prefix_ptr = str;
            prefix_len = i;
            name_ptr = &str[i + 1];
            name_len -= i + 1;
            break;
        }
    }
    if (!(*prefix = alloc_xml_string( prefix_ptr, prefix_len ))) return E_OUTOFMEMORY;
    if (!(*localname = alloc_xml_string( name_ptr, name_len )))
    {
        heap_free( *prefix );
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

static int codepoint_to_utf8( int cp, unsigned char *dst )
{
    if (!cp)
        return -1;
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
                else if (*p == 'x')
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

static HRESULT read_attribute( struct reader *reader, WS_XML_ATTRIBUTE **ret )
{
    static const WS_XML_STRING xmlns = {5, (BYTE *)"xmlns"};
    WS_XML_ATTRIBUTE *attr;
    WS_XML_UTF8_TEXT *text = NULL;
    unsigned int len = 0, ch, skip, quote;
    const unsigned char *start;
    WS_XML_STRING *prefix, *localname;
    HRESULT hr = WS_E_INVALID_FORMAT;

    if (!(attr = heap_alloc_zero( sizeof(*attr) ))) return E_OUTOFMEMORY;

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!(ch = read_utf8_char( reader, &skip ))) goto error;
        if (!read_isnamechar( ch )) break;
        read_skip( reader, skip );
        len += skip;
    }
    if (!len) goto error;

    if ((hr = parse_name( start, len, &prefix, &localname )) != S_OK) goto error;
    hr = E_OUTOFMEMORY;
    if (WsXmlStringEquals( prefix, &xmlns, NULL ) == S_OK)
    {
        heap_free( prefix );
        attr->isXmlNs   = 1;
        if (!(attr->prefix = alloc_xml_string( localname->bytes, localname->length )))
        {
            heap_free( localname );
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

    hr = WS_E_INVALID_FORMAT;
    read_skip_whitespace( reader );
    if (read_cmp( reader, "=", 1 )) goto error;
    read_skip( reader, 1 );

    read_skip_whitespace( reader );
    if (read_cmp( reader, "\"", 1 ) && read_cmp( reader, "'", 1 )) goto error;
    quote = read_utf8_char( reader, &skip );
    read_skip( reader, 1 );

    len = 0;
    start = read_current_ptr( reader );
    for (;;)
    {
        if (!(ch = read_utf8_char( reader, &skip ))) goto error;
        if (ch == quote) break;
        read_skip( reader, skip );
        len += skip;
    }
    read_skip( reader, 1 );

    hr = E_OUTOFMEMORY;
    if (attr->isXmlNs)
    {
        if (!(attr->ns = alloc_xml_string( start, len ))) goto error;
        if ((hr = bind_prefix( reader, attr->prefix, attr->ns )) != S_OK) goto error;
        if (!(text = alloc_utf8_text( NULL, 0 ))) goto error;
    }
    else
    {
        if (!(text = alloc_utf8_text( NULL, len ))) goto error;
        if ((hr = decode_text( start, len, text->value.bytes, &text->value.length )) != S_OK) goto error;
    }

    attr->value = &text->text;
    attr->singleQuote = (quote == '\'');

    *ret = attr;
    return S_OK;

error:
    heap_free( text );
    free_attribute( attr );
    return hr;
}

static inline BOOL is_valid_parent( const struct node *node )
{
    if (!node) return FALSE;
    return node_type( node ) == WS_XML_NODE_TYPE_ELEMENT || node_type( node ) == WS_XML_NODE_TYPE_BOF;
}

struct node *find_parent( struct node *node )
{
    if (is_valid_parent( node )) return node;
    if (is_valid_parent( node->parent )) return node->parent;
    return NULL;
}

static HRESULT set_namespaces( struct reader *reader, WS_XML_ELEMENT_NODE *elem )
{
    static const WS_XML_STRING xml = {3, (BYTE *)"xml"};
    const WS_XML_STRING *ns;
    ULONG i;

    if (!(ns = get_namespace( reader, elem->prefix ))) return WS_E_INVALID_FORMAT;
    if (!(elem->ns = alloc_xml_string( ns->bytes, ns->length ))) return E_OUTOFMEMORY;
    if (!elem->ns->length) elem->ns->bytes = (BYTE *)(elem->ns + 1); /* quirk */

    for (i = 0; i < elem->attributeCount; i++)
    {
        WS_XML_ATTRIBUTE *attr = elem->attributes[i];
        if (attr->isXmlNs || WsXmlStringEquals( attr->prefix, &xml, NULL ) == S_OK) continue;
        if (!(ns = get_namespace( reader, attr->prefix ))) return WS_E_INVALID_FORMAT;
        if (!(attr->ns = alloc_xml_string( ns->bytes, ns->length ))) return E_OUTOFMEMORY;
    }
    return S_OK;
}

static HRESULT read_element( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node = NULL, *endnode, *parent;
    WS_XML_ELEMENT_NODE *elem;
    WS_XML_ATTRIBUTE *attr = NULL;
    HRESULT hr = WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    if (read_cmp( reader, "<", 1 )) goto error;
    read_skip( reader, 1 );
    if (!read_isnamechar( read_utf8_char( reader, &skip )))
    {
        read_rewind( reader, 1 );
        goto error;
    }

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!(ch = read_utf8_char( reader, &skip ))) goto error;
        if (!read_isnamechar( ch )) break;
        read_skip( reader, skip );
        len += skip;
    }
    if (!len) goto error;

    if (!(parent = find_parent( reader->current ))) goto error;

    hr = E_OUTOFMEMORY;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) goto error;
    if (!(endnode = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT ))) goto error;
    list_add_tail( &node->children, &endnode->entry );
    endnode->parent = node;

    elem = (WS_XML_ELEMENT_NODE *)node;
    if ((hr = parse_name( start, len, &elem->prefix, &elem->localName )) != S_OK) goto error;

    reader->current_attr = 0;
    for (;;)
    {
        read_skip_whitespace( reader );
        if (!read_cmp( reader, ">", 1 ) || !read_cmp( reader, "/>", 2 )) break;
        if ((hr = read_attribute( reader, &attr )) != S_OK) goto error;
        if ((hr = append_attribute( elem, attr )) != S_OK)
        {
            free_attribute( attr );
            goto error;
        }
        reader->current_attr++;
    }
    if ((hr = set_namespaces( reader, elem )) != S_OK) goto error;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_STARTELEMENT;
    return S_OK;

error:
    destroy_nodes( node );
    return hr;
}

static HRESULT read_text( struct reader *reader )
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
        if (!(ch = read_utf8_char( reader, &skip ))) return WS_E_INVALID_FORMAT;
        if (ch == '<') break;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(parent = find_parent( reader->current ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;
    if (!(utf8 = alloc_utf8_text( NULL, len )))
    {
        heap_free( node );
        return E_OUTOFMEMORY;
    }
    if ((hr = decode_text( start, len, utf8->value.bytes, &utf8->value.length )) != S_OK)
    {
        heap_free( utf8 );
        heap_free( node );
        return hr;
    }
    text->text = &utf8->text;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_TEXT;
    return S_OK;
}

static HRESULT read_node( struct reader * );

static HRESULT read_startelement( struct reader *reader )
{
    read_skip_whitespace( reader );
    if (!read_cmp( reader, "/>", 2 ))
    {
        read_skip( reader, 2 );
        reader->current = LIST_ENTRY( list_tail( &reader->current->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_ENDELEMENT;
        return S_OK;
    }
    else if (!read_cmp( reader, ">", 1 ))
    {
        read_skip( reader, 1 );
        return read_node( reader );
    }
    return WS_E_INVALID_FORMAT;
}

static HRESULT read_to_startelement( struct reader *reader, BOOL *found )
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
    if ((hr = read_element( reader )) == S_OK && found)
    {
        if (reader->state == READER_STATE_STARTELEMENT)
            *found = TRUE;
        else
            *found = FALSE;
    }

    return hr;
}

static int cmp_name( const unsigned char *name1, ULONG len1, const unsigned char *name2, ULONG len2 )
{
    ULONG i;
    if (len1 != len2) return 1;
    for (i = 0; i < len1; i++) { if (toupper( name1[i] ) != toupper( name2[i] )) return 1; }
    return 0;
}

static struct node *read_find_startelement( struct reader *reader, const WS_XML_STRING *prefix,
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

static HRESULT read_endelement( struct reader *reader )
{
    struct node *parent;
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    WS_XML_STRING *prefix, *localname;
    HRESULT hr;

    if (reader->state == READER_STATE_EOF) return WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->root->children ), struct node, entry );
        reader->last    = reader->current;
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    if (read_cmp( reader, "</", 2 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 2 );

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!(ch = read_utf8_char( reader, &skip ))) return WS_E_INVALID_FORMAT;
        if (ch == '>')
        {
            read_skip( reader, 1 );
            break;
        }
        if (!read_isnamechar( ch )) return WS_E_INVALID_FORMAT;
        read_skip( reader, skip );
        len += skip;
    }

    if ((hr = parse_name( start, len, &prefix, &localname )) != S_OK) return hr;
    parent = read_find_startelement( reader, prefix, localname );
    heap_free( prefix );
    heap_free( localname );
    if (!parent) return WS_E_INVALID_FORMAT;

    reader->current = LIST_ENTRY( list_tail( &parent->children ), struct node, entry );
    reader->last    = reader->current;
    reader->state   = READER_STATE_ENDELEMENT;
    return S_OK;
}

static HRESULT read_comment( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node, *parent;
    WS_XML_COMMENT_NODE *comment;

    if (read_cmp( reader, "<!--", 4 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 4 );

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!read_cmp( reader, "-->", 3 ))
        {
            read_skip( reader, 3 );
            break;
        }
        if (!(ch = read_utf8_char( reader, &skip ))) return WS_E_INVALID_FORMAT;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(parent = find_parent( reader->current ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;
    if (!(comment->value.bytes = heap_alloc( len )))
    {
        heap_free( node );
        return E_OUTOFMEMORY;
    }
    memcpy( comment->value.bytes, start, len );
    comment->value.length = len;

    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_COMMENT;
    return S_OK;
}

static HRESULT read_startcdata( struct reader *reader )
{
    struct node *node, *endnode, *parent;

    if (read_cmp( reader, "<![CDATA[", 9 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 9 );

    if (!(parent = find_parent( reader->current ))) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_CDATA ))) return E_OUTOFMEMORY;
    if (!(endnode = alloc_node( WS_XML_NODE_TYPE_END_CDATA )))
    {
        heap_free( node );
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

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!read_cmp( reader, "]]>", 3 )) break;
        if (!(ch = read_utf8_char( reader, &skip ))) return WS_E_INVALID_FORMAT;
        read_skip( reader, skip );
        len += skip;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_TEXT ))) return E_OUTOFMEMORY;
    text = (WS_XML_TEXT_NODE *)node;
    if (!(utf8 = alloc_utf8_text( start, len )))
    {
        heap_free( node );
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

    if (read_cmp( reader, "]]>", 3 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 3 );

    if (node_type( reader->current ) == WS_XML_NODE_TYPE_TEXT) parent = reader->current->parent;
    else parent = reader->current;

    reader->current = LIST_ENTRY( list_tail( &parent->children ), struct node, entry );
    reader->last    = reader->current;
    reader->state   = READER_STATE_ENDCDATA;
    return S_OK;
}

static HRESULT read_node( struct reader *reader )
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
        else if (!read_cmp( reader, "<?", 2 ))
        {
            hr = read_xmldecl( reader );
            if (FAILED( hr )) return hr;
        }
        else if (!read_cmp( reader, "</", 2 )) return read_endelement( reader );
        else if (!read_cmp( reader, "<![CDATA[", 9 )) return read_startcdata( reader );
        else if (!read_cmp( reader, "<!--", 4 )) return read_comment( reader );
        else if (!read_cmp( reader, "<", 1 )) return read_element( reader );
        else if (!read_cmp( reader, "/>", 2 ) || !read_cmp( reader, ">", 1 )) return read_startelement( reader );
        else return read_text( reader );
    }
}

/**************************************************************************
 *          WsReadEndElement		[webservices.@]
 */
HRESULT WINAPI WsReadEndElement( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    return read_endelement( reader );
}

/**************************************************************************
 *          WsReadNode		[webservices.@]
 */
HRESULT WINAPI WsReadNode( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    return read_node( reader );
}

/**************************************************************************
 *          WsReadStartElement		[webservices.@]
 */
HRESULT WINAPI WsReadStartElement( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    return read_startelement( reader );
}

/**************************************************************************
 *          WsReadToStartElement		[webservices.@]
 */
HRESULT WINAPI WsReadToStartElement( WS_XML_READER *handle, const WS_XML_STRING *localname,
                                     const WS_XML_STRING *ns, BOOL *found, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %s %s %p %p\n", handle, debugstr_xmlstr(localname), debugstr_xmlstr(ns), found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    if (localname || ns) FIXME( "name and/or namespace not verified\n" );

    return read_to_startelement( reader, found );
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
        while (reader->state != READER_STATE_EOF && (hr = read_node( reader )) == S_OK) { /* nothing */ };
        if (hr != S_OK) return hr;
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

    TRACE( "%p %u %p %p\n", handle, move, found, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;
    if (!reader->input_type) return WS_E_INVALID_OPERATION;

    return read_move_to( reader, move, found );
}

/**************************************************************************
 *          WsReadStartAttribute		[webservices.@]
 */
HRESULT WINAPI WsReadStartAttribute( WS_XML_READER *handle, ULONG index, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    WS_XML_ELEMENT_NODE *elem;

    TRACE( "%p %u %p\n", handle, index, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    elem = &reader->current->hdr;
    if (reader->state != READER_STATE_STARTELEMENT || index >= elem->attributeCount)
        return WS_E_INVALID_FORMAT;

    reader->current_attr = index;
    reader->state = READER_STATE_STARTATTRIBUTE;
    return S_OK;
}

/**************************************************************************
 *          WsReadEndAttribute		[webservices.@]
 */
HRESULT WINAPI WsReadEndAttribute( WS_XML_READER *handle, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p\n", handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    if (reader->state != READER_STATE_STARTATTRIBUTE)
        return WS_E_INVALID_FORMAT;

    reader->state = READER_STATE_STARTELEMENT;
    return S_OK;
}

static WCHAR *xmltext_to_widechar( WS_HEAP *heap, const WS_XML_TEXT *text )
{
    WCHAR *ret;

    switch (text->textType)
    {
    case WS_XML_TEXT_TYPE_UTF8:
    {
        const WS_XML_UTF8_TEXT *utf8 = (const WS_XML_UTF8_TEXT *)text;
        int len = MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, NULL, 0 );
        if (!(ret = ws_alloc( heap, (len + 1) * sizeof(WCHAR) ))) return NULL;
        MultiByteToWideChar( CP_UTF8, 0, (char *)utf8->value.bytes, utf8->value.length, ret, len );
        ret[len] = 0;
        break;
    }
    default:
        FIXME( "unhandled type %u\n", text->textType );
        return NULL;
    }

    return ret;
}

#define MAX_INT8    0x7f
#define MIN_INT8    (-MAX_INT8 - 1)
#define MAX_INT16   0x7fff
#define MIN_INT16   (-MAX_INT16 - 1)
#define MAX_INT32   0x7fffffff
#define MIN_INT32   (-MAX_INT32 - 1)
#define MAX_INT64   (((INT64)0x7fffffff << 32) | 0xffffffff)
#define MIN_INT64   (-MAX_INT64 - 1)
#define MAX_UINT8   0xff
#define MAX_UINT16  0xffff
#define MAX_UINT32  0xffffffff
#define MAX_UINT64  (((UINT64)0xffffffff << 32) | 0xffffffff)

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

#if defined(__i386__) || defined(__x86_64__)

#define RC_DOWN 0x100;
BOOL set_fp_rounding( unsigned short *save )
{
#ifdef __GNUC__
    unsigned short fpword;

    __asm__ __volatile__( "fstcw %0" : "=m" (fpword) );
    *save = fpword;
    fpword |= RC_DOWN;
    __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );
    return TRUE;
#else
    FIXME( "not implemented\n" );
    return FALSE;
#endif
}
void restore_fp_rounding( unsigned short fpword )
{
#ifdef __GNUC__
    __asm__ __volatile__( "fldcw %0" : : "m" (fpword) );
#else
    FIXME( "not implemented\n" );
#endif
}
#else
BOOL set_fp_rounding( unsigned short *save )
{
    FIXME( "not implemented\n" );
    return FALSE;
}
void restore_fp_rounding( unsigned short fpword )
{
    FIXME( "not implemented\n" );
}
#endif

static HRESULT str_to_double( const unsigned char *str, ULONG len, double *ret )
{
    static const unsigned __int64 nan = 0xfff8000000000000;
    static const unsigned __int64 inf = 0x7ff0000000000000;
    static const unsigned __int64 inf_min = 0xfff0000000000000;
    HRESULT hr = WS_E_INVALID_FORMAT;
    const unsigned char *p = str, *q;
    int sign = 1, exp_sign = 1, exp = 0, exp_tmp = 0, neg_exp, i, nb_digits, have_digits;
    unsigned __int64 val = 0, tmp;
    long double exp_val = 1.0, exp_mul = 10.0;
    unsigned short fpword;

    while (len && read_isspace( *p )) { p++; len--; }
    while (len && read_isspace( p[len - 1] )) { len--; }
    if (!len) return WS_E_INVALID_FORMAT;

    if (len == 3 && !memcmp( p, "NaN", 3 ))
    {
        *(unsigned __int64 *)ret = nan;
        return S_OK;
    }
    else if (len == 3 && !memcmp( p, "INF", 3 ))
    {
        *(unsigned __int64 *)ret = inf;
        return S_OK;
    }
    else if (len == 4 && !memcmp( p, "-INF", 4 ))
    {
        *(unsigned __int64 *)ret = inf_min;
        return S_OK;
    }

    *ret = 0.0;
    if (*p == '-')
    {
        sign = -1;
        p++; len--;
    }
    else if (*p == '+') { p++; len--; };
    if (!len) return S_OK;

    if (!set_fp_rounding( &fpword )) return E_NOTIMPL;

    q = p;
    while (len && isdigit( *q )) { q++; len--; }
    have_digits = nb_digits = q - p;
    for (i = 0; i < nb_digits; i++)
    {
        tmp = val * 10 + p[i] - '0';
        if (val > MAX_UINT64 / 10 || tmp < val)
        {
            for (; i < nb_digits; i++) exp++;
            break;
        }
        val = tmp;
    }

    if (len)
    {
        if (*q == '.')
        {
            p = ++q; len--;
            while (len && isdigit( *q )) { q++; len--; };
            have_digits |= nb_digits = q - p;
            for (i = 0; i < nb_digits; i++)
            {
                tmp = val * 10 + p[i] - '0';
                if (val > MAX_UINT64 / 10 || tmp < val) break;
                val = tmp;
                exp--;
            }
        }
        if (len > 1 && tolower(*q) == 'e')
        {
            if (!have_digits) goto done;
            p = ++q; len--;
            if (*p == '-')
            {
                exp_sign = -1;
                p++; len--;
            }
            else if (*p == '+') { p++; len--; };

            q = p;
            while (len && isdigit( *q )) { q++; len--; };
            nb_digits = q - p;
            if (!nb_digits || len) goto done;
            for (i = 0; i < nb_digits; i++)
            {
                if (exp_tmp > MAX_INT32 / 10 || (exp_tmp = exp_tmp * 10 + p[i] - '0') < 0)
                    exp_tmp = MAX_INT32;
            }
            exp_tmp *= exp_sign;

            if (exp < 0 && exp_tmp < 0 && exp + exp_tmp >= 0) exp = MIN_INT32;
            else if (exp > 0 && exp_tmp > 0 && exp + exp_tmp < 0) exp = MAX_INT32;
            else exp += exp_tmp;
        }
    }
    if (!have_digits || len) goto done;

    if ((neg_exp = exp < 0)) exp = -exp;
    for (; exp; exp >>= 1)
    {
        if (exp & 1) exp_val *= exp_mul;
        exp_mul *= exp_mul;
    }

    *ret = sign * (neg_exp ? val / exp_val : val * exp_val);
    hr = S_OK;

done:
    restore_fp_rounding( fpword );
    return hr;
}

static HRESULT str_to_guid( const unsigned char *str, ULONG len, GUID *ret )
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

static HRESULT read_get_node_text( struct reader *reader, WS_XML_UTF8_TEXT **ret )
{
    WS_XML_TEXT_NODE *text;

    if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT)
        return WS_E_INVALID_FORMAT;

    text = (WS_XML_TEXT_NODE *)&reader->current->hdr.node;
    if (text->text->textType != WS_XML_TEXT_TYPE_UTF8)
    {
        FIXME( "text type %u not supported\n", text->text->textType );
        return E_NOTIMPL;
    }
    *ret = (WS_XML_UTF8_TEXT *)text->text;
    return S_OK;
}

static HRESULT read_get_attribute_text( struct reader *reader, ULONG index, WS_XML_UTF8_TEXT **ret )
{
    WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;
    WS_XML_ATTRIBUTE *attr;

    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT)
        return WS_E_INVALID_FORMAT;

    attr = elem->attributes[index];
    if (attr->value->textType != WS_XML_TEXT_TYPE_UTF8)
    {
        FIXME( "text type %u not supported\n", attr->value->textType );
        return E_NOTIMPL;
    }
    *ret = (WS_XML_UTF8_TEXT *)attr->value;
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

    TRACE( "%p %s %s %d %p %p\n", handle, debugstr_xmlstr(localname), debugstr_xmlstr(ns),
           required, index, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !localname || !ns || !index) return E_INVALIDARG;

    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT)
        return WS_E_INVALID_OPERATION;

    if (!find_attribute( reader, localname, ns, index ))
    {
        if (required) return WS_E_INVALID_FORMAT;
        *index = ~0u;
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT read_get_text( struct reader *reader, WS_TYPE_MAPPING mapping,
                              const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                              WS_XML_UTF8_TEXT **ret, BOOL *found )
{
    switch (mapping)
    {
    case WS_ATTRIBUTE_TYPE_MAPPING:
    {
        ULONG index;
        if (!(*found = find_attribute( reader, localname, ns, &index ))) return S_OK;
        return read_get_attribute_text( reader, index, ret );
    }
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
    case WS_ANY_ELEMENT_TYPE_MAPPING:
    {
        HRESULT hr;
        *found = TRUE;
        if (localname)
        {
            const WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;

            if (WsXmlStringEquals( localname, elem->localName, NULL ) != S_OK ||
                WsXmlStringEquals( ns, elem->ns, NULL ) != S_OK)
            {
                *found = FALSE;
                return S_OK;
            }
            if ((hr = read_startelement( reader )) != S_OK) return hr;
        }
        return read_get_node_text( reader, ret );
    }
    default:
        FIXME( "mapping %u not supported\n", mapping );
        return E_NOTIMPL;
    }
}

static HRESULT read_type_bool( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_BOOL_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    BOOL found, val = FALSE;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found)
    {
        ULONG len = utf8->value.length;
        if (len == 4 && !memcmp( utf8->value.bytes, "true", 4 )) val = TRUE;
        else if (len == 1 && !memcmp( utf8->value.bytes, "1", 1 )) val = TRUE;
        else if (len == 5 && !memcmp( utf8->value.bytes, "false", 5 )) val = FALSE;
        else if (len == 1 && !memcmp( utf8->value.bytes, "0", 1 )) val = FALSE;
        else return WS_E_INVALID_FORMAT;
    }

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(BOOL)) return E_INVALIDARG;
        *(BOOL *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        BOOL *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_int8( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_INT8_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    INT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_int64( utf8->value.bytes, utf8->value.length, MIN_INT8, MAX_INT8, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT8)) return E_INVALIDARG;
        *(INT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT8 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_int16( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT16_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    INT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_int64( utf8->value.bytes, utf8->value.length, MIN_INT16, MAX_INT16, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT16)) return E_INVALIDARG;
        *(INT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT16 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_int32( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT32_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    INT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_int64( utf8->value.bytes, utf8->value.length, MIN_INT32, MAX_INT32, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT32)) return E_INVALIDARG;
        *(INT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT32 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_int64( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_INT64_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    INT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_int64( utf8->value.bytes, utf8->value.length, MIN_INT64, MAX_INT64, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(INT64)) return E_INVALIDARG;
        *(INT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        INT64 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_uint8( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_UINT8_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    UINT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_uint64( utf8->value.bytes, utf8->value.length, MAX_UINT8, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT8)) return E_INVALIDARG;
        *(UINT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT8 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_uint16( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT16_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    UINT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_uint64( utf8->value.bytes, utf8->value.length, MAX_UINT16, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT16)) return E_INVALIDARG;
        *(UINT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT16 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_uint32( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT32_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    UINT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_uint64( utf8->value.bytes, utf8->value.length, MAX_UINT32, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT32)) return E_INVALIDARG;
        *(UINT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT32 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_uint64( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_UINT64_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    UINT64 val = 0;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_uint64( utf8->value.bytes, utf8->value.length, MAX_UINT64, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(UINT64)) return E_INVALIDARG;
        *(UINT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        UINT64 *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_double( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_DOUBLE_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    double val = 0.0;
    BOOL found;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_double( utf8->value.bytes, utf8->value.length, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(double)) return E_INVALIDARG;
        *(double *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        double *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_wsz( struct reader *reader, WS_TYPE_MAPPING mapping,
                              const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                              const WS_WSZ_DESCRIPTION *desc, WS_READ_OPTION option,
                              WS_HEAP *heap, WCHAR **ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    WCHAR *str = NULL;
    BOOL found;

    if (desc)
    {
        FIXME( "description not supported\n" );
        return E_NOTIMPL;
    }
    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && !(str = xmltext_to_widechar( heap, &utf8->text ))) return WS_E_QUOTA_EXCEEDED;

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
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

static HRESULT get_enum_value( const WS_XML_UTF8_TEXT *text, const WS_ENUM_DESCRIPTION *desc, int *ret )
{
    ULONG i;
    for (i = 0; i < desc->valueCount; i++)
    {
        if (WsXmlStringEquals( &text->value, desc->values[i].name, NULL ) == S_OK)
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
                               WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    int val = 0;
    BOOL found;

    if (!desc) return E_INVALIDARG;

    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = get_enum_value( utf8, desc, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(int)) return E_INVALIDARG;
        *(int *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        int *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_datetime( struct reader *reader, WS_TYPE_MAPPING mapping,
                                   const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                   const WS_DATETIME_DESCRIPTION *desc, WS_READ_OPTION option,
                                   WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    HRESULT hr;
    WS_DATETIME val = {0, WS_DATETIME_FORMAT_UTC};
    BOOL found;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_datetime( utf8->value.bytes, utf8->value.length, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(WS_DATETIME)) return E_INVALIDARG;
        *(WS_DATETIME *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_DATETIME *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_guid( struct reader *reader, WS_TYPE_MAPPING mapping,
                               const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                               const WS_GUID_DESCRIPTION *desc, WS_READ_OPTION option,
                               WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    GUID val = {0};
    HRESULT hr;
    BOOL found;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_guid( utf8->value.bytes, utf8->value.length, &val )) != S_OK) return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(GUID)) return E_INVALIDARG;
        *(GUID *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        GUID *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static HRESULT read_type_bytes( struct reader *reader, WS_TYPE_MAPPING mapping,
                                const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                const WS_BYTES_DESCRIPTION *desc, WS_READ_OPTION option,
                                WS_HEAP *heap, void *ret, ULONG size )
{
    WS_XML_UTF8_TEXT *utf8;
    WS_BYTES val = {0};
    HRESULT hr;
    BOOL found;

    if (desc) FIXME( "ignoring description\n" );

    if ((hr = read_get_text( reader, mapping, localname, ns, &utf8, &found )) != S_OK) return hr;
    if (found && (hr = str_to_bytes( utf8->value.bytes, utf8->value.length, heap, &val )) != S_OK)
        return hr;

    switch (option)
    {
    case WS_READ_REQUIRED_VALUE:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_NILLABLE_VALUE:
        if (size != sizeof(WS_BYTES)) return E_INVALIDARG;
        *(WS_BYTES *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
    {
        WS_BYTES *heap_val = NULL;
        if (size != sizeof(heap_val)) return E_INVALIDARG;
        if (found)
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

static BOOL is_empty_text_node( const struct node *node )
{
    const WS_XML_TEXT_NODE *text = (const WS_XML_TEXT_NODE *)node;
    const WS_XML_UTF8_TEXT *utf8;
    ULONG i;

    if (node_type( node ) != WS_XML_NODE_TYPE_TEXT) return FALSE;
    if (text->text->textType != WS_XML_TEXT_TYPE_UTF8)
    {
        ERR( "unhandled text type %u\n", text->text->textType );
        return FALSE;
    }
    utf8 = (const WS_XML_UTF8_TEXT *)text->text;
    for (i = 0; i < utf8->value.length; i++) if (!read_isspace( utf8->value.bytes[i] )) return FALSE;
    return TRUE;
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

static BOOL match_current_element( struct reader *reader, const WS_XML_STRING *localname,
                                   const WS_XML_STRING *ns )
{
    const WS_XML_ELEMENT_NODE *elem = &reader->current->hdr;
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_ELEMENT) return FALSE;
    return WsXmlStringEquals( localname, elem->localName, NULL ) == S_OK &&
           WsXmlStringEquals( ns, elem->ns, NULL ) == S_OK;
}

static HRESULT read_type_next_element_node( struct reader *reader, const WS_XML_STRING *localname,
                                            const WS_XML_STRING *ns )
{
    struct node *node;
    ULONG attr;
    HRESULT hr;

    if (!localname) return S_OK; /* assume reader is already correctly positioned */
    if (reader->current == reader->last)
    {
        BOOL found;
        if ((hr = read_to_startelement( reader, &found )) != S_OK) return hr;
        if (!found) return WS_E_INVALID_FORMAT;
    }
    if (match_current_element( reader, localname, ns )) return S_OK;

    node = reader->current;
    attr = reader->current_attr;

    if ((hr = read_type_next_node( reader )) != S_OK) return hr;
    if (match_current_element( reader, localname, ns )) return S_OK;

    reader->current = node;
    reader->current_attr = attr;

    return WS_E_INVALID_FORMAT;
}

ULONG get_type_size( WS_TYPE type, const WS_STRUCT_DESCRIPTION *desc )
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

    case WS_DOUBLE_TYPE:
        return sizeof(double);

    case WS_DATETIME_TYPE:
        return sizeof(WS_DATETIME);

    case WS_GUID_TYPE:
        return sizeof(GUID);

    case WS_WSZ_TYPE:
        return sizeof(WCHAR *);

    case WS_BYTES_TYPE:
        return sizeof(WS_BYTES);

    case WS_STRUCT_TYPE:
        return desc->size;

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
    case WS_DOUBLE_TYPE:
    case WS_DATETIME_TYPE:
    case WS_GUID_TYPE:
    case WS_BYTES_TYPE:
    case WS_STRUCT_TYPE:
    case WS_ENUM_TYPE:
        if (options & (WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE)) return WS_READ_NILLABLE_VALUE;
        return WS_READ_REQUIRED_VALUE;

    case WS_WSZ_TYPE:
        if (options & WS_FIELD_NILLABLE) return WS_READ_NILLABLE_POINTER;
        if (options & WS_FIELD_OPTIONAL) return WS_READ_OPTIONAL_POINTER;
        return WS_READ_REQUIRED_POINTER;

    default:
        FIXME( "unhandled type %u\n", type );
        return 0;
    }
}

static HRESULT read_type( struct reader *, WS_TYPE_MAPPING, WS_TYPE, const WS_XML_STRING *,
                          const WS_XML_STRING *, const void *, WS_READ_OPTION, WS_HEAP *,
                          void *, ULONG );

static HRESULT read_type_repeating_element( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                                            WS_HEAP *heap, void **ret, ULONG *count )
{
    HRESULT hr;
    ULONG item_size, nb_items = 0, nb_allocated = 1, offset = 0;
    WS_READ_OPTION option;
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
            if (!(buf = ws_realloc_zero( heap, buf, nb_allocated * 2 * item_size )))
                return WS_E_QUOTA_EXCEEDED;
            nb_allocated *= 2;
        }
        hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->itemLocalName, desc->itemNs,
                        desc->typeDescription, option, heap, buf + offset, item_size );
        if (hr == WS_E_INVALID_FORMAT) break;
        if (hr != S_OK)
        {
            ws_free( heap, buf );
            return hr;
        }
        offset += item_size;
        nb_items++;
    }

    if (desc->localName && ((hr = read_type_next_node( reader )) != S_OK)) return hr;

    if (desc->itemRange && (nb_items < desc->itemRange->minItemCount || nb_items > desc->itemRange->maxItemCount))
    {
        TRACE( "number of items %u out of range (%u-%u)\n", nb_items, desc->itemRange->minItemCount,
               desc->itemRange->maxItemCount );
        ws_free( heap, buf );
        return WS_E_INVALID_FORMAT;
    }

    *count = nb_items;
    *ret = buf;

    return S_OK;
}

static HRESULT read_type_text( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                               WS_READ_OPTION option, WS_HEAP *heap, void *ret, ULONG size )
{
    HRESULT hr;
    if (reader->current == reader->last)
    {
        BOOL found;
        if ((hr = read_to_startelement( reader, &found )) != S_OK) return S_OK;
        if (!found) return WS_E_INVALID_FORMAT;
    }
    if ((hr = read_next_node( reader )) != S_OK) return hr;
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT) return WS_E_INVALID_FORMAT;

    return read_type( reader, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, NULL, NULL,
                      desc->typeDescription, option, heap, ret, size );
}

static HRESULT read_type_struct_field( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                                       WS_HEAP *heap, char *buf )
{
    char *ptr;
    WS_READ_OPTION option;
    ULONG size;
    HRESULT hr;

    if (!desc) return E_INVALIDARG;
    if (desc->options & ~(WS_FIELD_POINTER|WS_FIELD_OPTIONAL|WS_FIELD_NILLABLE|WS_FIELD_NILLABLE_ITEM))
    {
        FIXME( "options %08x not supported\n", desc->options );
        return E_NOTIMPL;
    }
    if (!(option = get_field_read_option( desc->type, desc->options ))) return E_INVALIDARG;

    if (option == WS_READ_REQUIRED_VALUE || option == WS_READ_NILLABLE_VALUE)
        size = get_type_size( desc->type, desc->typeDescription );
    else
        size = sizeof(void *);

    ptr = buf + desc->offset;
    switch (desc->mapping)
    {
    case WS_ATTRIBUTE_FIELD_MAPPING:
        hr = read_type( reader, WS_ATTRIBUTE_TYPE_MAPPING, desc->type, desc->localName, desc->ns,
                        desc->typeDescription, option, heap, ptr, size );
        break;

    case WS_ELEMENT_FIELD_MAPPING:
        hr = read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->localName, desc->ns,
                        desc->typeDescription, option, heap, ptr, size );
        break;

    case WS_REPEATING_ELEMENT_FIELD_MAPPING:
    {
        ULONG count;
        hr = read_type_repeating_element( reader, desc, heap, (void **)ptr, &count );
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

static HRESULT read_type_struct( struct reader *reader, WS_TYPE_MAPPING mapping,
                                 const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                                 const WS_STRUCT_DESCRIPTION *desc, WS_READ_OPTION option,
                                 WS_HEAP *heap, void *ret, ULONG size )
{
    ULONG i;
    HRESULT hr;
    char *buf;

    if (!desc) return E_INVALIDARG;
    if (desc->structOptions) FIXME( "struct options %08x not supported\n", desc->structOptions );

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
        if ((hr = read_type_struct_field( reader, desc->fields[i], heap, buf )) != S_OK)
            break;
    }

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
        if (hr != S_OK)
        {
            ws_free( heap, buf );
            return hr;
        }
        *(char **)ret = buf;
        return S_OK;

    case WS_READ_OPTIONAL_POINTER:
    case WS_READ_NILLABLE_POINTER:
        if (is_nil_value( buf, desc->size ))
        {
            ws_free( heap, buf );
            buf = NULL;
        }
        *(char **)ret = buf;
        return S_OK;

    case WS_READ_REQUIRED_VALUE:
    case WS_READ_NILLABLE_VALUE:
        return hr;

    default:
        ERR( "unhandled read option %u\n", option );
        return E_NOTIMPL;
    }
}

static HRESULT read_type( struct reader *reader, WS_TYPE_MAPPING mapping, WS_TYPE type,
                          const WS_XML_STRING *localname, const WS_XML_STRING *ns,
                          const void *desc, WS_READ_OPTION option, WS_HEAP *heap,
                          void *value, ULONG size )
{
    HRESULT hr;

    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        if ((hr = read_type_next_element_node( reader, localname, ns )) != S_OK) return hr;
        break;

    case WS_ANY_ELEMENT_TYPE_MAPPING:
    case WS_ATTRIBUTE_TYPE_MAPPING:
        break;

    default:
        FIXME( "unhandled mapping %u\n", mapping );
        return E_NOTIMPL;
    }

    switch (type)
    {
    case WS_BOOL_TYPE:
        if ((hr = read_type_bool( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_INT8_TYPE:
        if ((hr = read_type_int8( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_INT16_TYPE:
        if ((hr = read_type_int16( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_INT32_TYPE:
        if ((hr = read_type_int32( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_INT64_TYPE:
        if ((hr = read_type_int64( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_UINT8_TYPE:
        if ((hr = read_type_uint8( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_UINT16_TYPE:
        if ((hr = read_type_uint16( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_UINT32_TYPE:
        if ((hr = read_type_uint32( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_UINT64_TYPE:
        if ((hr = read_type_uint64( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_DOUBLE_TYPE:
        if ((hr = read_type_double( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_DATETIME_TYPE:
        if ((hr = read_type_datetime( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_GUID_TYPE:
        if ((hr = read_type_guid( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_WSZ_TYPE:
        if ((hr = read_type_wsz( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_BYTES_TYPE:
        if ((hr = read_type_bytes( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_STRUCT_TYPE:
        if ((hr = read_type_struct( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_ENUM_TYPE:
        if ((hr = read_type_enum( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    default:
        FIXME( "type %u not supported\n", type );
        return E_NOTIMPL;
    }

    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        return read_type_next_node( reader );

    case WS_ATTRIBUTE_TYPE_MAPPING:
    default:
        return S_OK;
    }
}

/**************************************************************************
 *          WsReadType		[webservices.@]
 */
HRESULT WINAPI WsReadType( WS_XML_READER *handle, WS_TYPE_MAPPING mapping, WS_TYPE type,
                           const void *desc, WS_READ_OPTION option, WS_HEAP *heap, void *value,
                           ULONG size, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    HRESULT hr;

    TRACE( "%p %u %u %p %u %p %p %u %p\n", handle, mapping, type, desc, option, heap, value,
           size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !value) return E_INVALIDARG;

    if ((hr = read_type( reader, mapping, type, NULL, NULL, desc, option, heap, value, size )) != S_OK)
        return hr;

    switch (mapping)
    {
    case WS_ELEMENT_TYPE_MAPPING:
        if ((hr = read_node( reader )) != S_OK) return hr;
        break;

    default:
        break;
    }

    if (!read_end_of_data( reader )) return WS_E_INVALID_FORMAT;
    return S_OK;
}

/**************************************************************************
 *          WsReadElement		[webservices.@]
 */
HRESULT WINAPI WsReadElement( WS_XML_READER *handle, const WS_ELEMENT_DESCRIPTION *desc,
                              WS_READ_OPTION option, WS_HEAP *heap, void *value, ULONG size,
                              WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p %u %p %p %u %p\n", handle, desc, option, heap, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !desc || !value) return E_INVALIDARG;

    return read_type( reader, WS_ELEMENT_TYPE_MAPPING, desc->type, desc->elementLocalName,
                      desc->elementNs, desc->typeDescription, option, heap, value, size );
}

/**************************************************************************
 *          WsReadValue		[webservices.@]
 */
HRESULT WINAPI WsReadValue( WS_XML_READER *handle, WS_VALUE_TYPE value_type, void *value, ULONG size,
                            WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    WS_TYPE type = map_value_type( value_type );

    TRACE( "%p %u %p %u %p\n", handle, type, value, size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !value || type == ~0u) return E_INVALIDARG;

    return read_type( reader, WS_ELEMENT_TYPE_MAPPING, type, NULL, NULL, NULL, WS_READ_REQUIRED_VALUE,
                      NULL, value, size );
}

/**************************************************************************
 *          WsSetErrorProperty		[webservices.@]
 */
HRESULT WINAPI WsSetErrorProperty( WS_ERROR *handle, WS_ERROR_PROPERTY_ID id, const void *value,
                                   ULONG size )
{
    struct error *error = (struct error *)handle;

    TRACE( "%p %u %p %u\n", handle, id, value, size );

    if (id == WS_ERROR_PROPERTY_LANGID) return WS_E_INVALID_OPERATION;
    return prop_set( error->prop, error->prop_count, id, value, size );
}

static inline BOOL is_utf8( const unsigned char *data, ULONG size, ULONG *offset )
{
    static const char bom[] = {0xef,0xbb,0xbf};
    const unsigned char *p = data;

    return (size >= sizeof(bom) && !memcmp( p, bom, sizeof(bom) ) && (*offset = sizeof(bom))) ||
           (size > 2 && !(*offset = 0));
}

static inline BOOL is_utf16le( const unsigned char *data, ULONG size, ULONG *offset )
{
    static const char bom[] = {0xff,0xfe};
    const unsigned char *p = data;

    return (size >= sizeof(bom) && !memcmp( p, bom, sizeof(bom) ) && (*offset = sizeof(bom))) ||
           (size >= 4 && p[0] == '<' && !p[1] && !(*offset = 0));
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

static void set_input_buffer( struct reader *reader, struct xmlbuf *buf, const unsigned char *data, ULONG size )
{
    reader->input_type  = WS_XML_READER_INPUT_TYPE_BUFFER;
    reader->input_buf   = buf;
    reader->input_data  = data;
    reader->input_size  = size;

    reader->read_size   = reader->input_size;
    reader->read_pos    = 0;
    reader->read_bufptr = reader->input_data;
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
    HRESULT hr;
    ULONG i, offset = 0;

    TRACE( "%p %p %p %p %u %p\n", handle, encoding, input, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader) return E_INVALIDARG;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( reader->prop, reader->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if ((hr = read_init_state( reader )) != S_OK) return hr;

    switch (encoding->encodingType)
    {
    case WS_XML_READER_ENCODING_TYPE_TEXT:
    {
        WS_XML_READER_TEXT_ENCODING *text = (WS_XML_READER_TEXT_ENCODING *)encoding;
        WS_XML_READER_BUFFER_INPUT *buf = (WS_XML_READER_BUFFER_INPUT *)input;
        WS_CHARSET charset = text->charSet;

        if (input->inputType != WS_XML_READER_INPUT_TYPE_BUFFER)
        {
            FIXME( "charset detection on input type %u not supported\n", input->inputType );
            return E_NOTIMPL;
        }

        if (charset == WS_CHARSET_AUTO)
            charset = detect_charset( buf->encodedData, buf->encodedDataSize, &offset );

        hr = prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_CHARSET,
                       &charset, sizeof(charset) );
        if (hr != S_OK) return hr;
        break;
    }
    default:
        FIXME( "encoding type %u not supported\n", encoding->encodingType );
        return E_NOTIMPL;
    }
    switch (input->inputType)
    {
    case WS_XML_READER_INPUT_TYPE_BUFFER:
    {
        WS_XML_READER_BUFFER_INPUT *buf = (WS_XML_READER_BUFFER_INPUT *)input;
        set_input_buffer( reader, NULL, (const unsigned char *)buf->encodedData + offset,
                          buf->encodedDataSize - offset );
        break;
    }
    default:
        FIXME( "input type %u not supported\n", input->inputType );
        return E_NOTIMPL;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) return E_OUTOFMEMORY;
    read_insert_bof( reader, node );
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
    WS_CHARSET charset;
    struct node *node;
    HRESULT hr;
    ULONG i, offset = 0;

    TRACE( "%p %p %p %u %p\n", handle, buffer, properties, count, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !xmlbuf) return E_INVALIDARG;

    for (i = 0; i < count; i++)
    {
        hr = prop_set( reader->prop, reader->prop_count, properties[i].id, properties[i].value,
                       properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if ((hr = read_init_state( reader )) != S_OK) return hr;

    charset = detect_charset( xmlbuf->ptr, xmlbuf->size, &offset );
    hr = prop_set( reader->prop, reader->prop_count, WS_XML_READER_PROPERTY_CHARSET,
                   &charset, sizeof(charset) );
    if (hr != S_OK) return hr;

    set_input_buffer( reader, xmlbuf, (const unsigned char *)xmlbuf->ptr + offset, xmlbuf->size - offset );
    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) return E_OUTOFMEMORY;
    read_insert_bof( reader, node );
    return S_OK;
}

/**************************************************************************
 *          WsXmlStringEquals		[webservices.@]
 */
HRESULT WINAPI WsXmlStringEquals( const WS_XML_STRING *str1, const WS_XML_STRING *str2, WS_ERROR *error )
{
    TRACE( "%s %s %p\n", debugstr_xmlstr(str1), debugstr_xmlstr(str2), error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!str1 || !str2) return E_INVALIDARG;

    if (str1->length != str2->length) return S_FALSE;
    if (!memcmp( str1->bytes, str2->bytes, str1->length )) return S_OK;
    return S_FALSE;
}

/**************************************************************************
 *          WsGetReaderPosition		[webservices.@]
 */
HRESULT WINAPI WsGetReaderPosition( WS_XML_READER *handle, WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !pos) return E_INVALIDARG;
    if (!reader->input_buf) return WS_E_INVALID_OPERATION;

    pos->buffer = (WS_XML_BUFFER *)reader->input_buf;
    pos->node   = reader->current;
    return S_OK;
}

/**************************************************************************
 *          WsSetReaderPosition		[webservices.@]
 */
HRESULT WINAPI WsSetReaderPosition( WS_XML_READER *handle, const WS_XML_NODE_POSITION *pos, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;

    TRACE( "%p %p %p\n", handle, pos, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !pos || (struct xmlbuf *)pos->buffer != reader->input_buf) return E_INVALIDARG;
    if (!reader->input_buf) return WS_E_INVALID_OPERATION;

    reader->current = pos->node;
    return S_OK;
}
