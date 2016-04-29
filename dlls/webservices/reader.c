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
    struct prefix           *prefixes;
    ULONG                    nb_prefixes;
    ULONG                    nb_prefixes_allocated;
    WS_XML_READER_INPUT_TYPE input_type;
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
    reader->current = eof;
}

static void read_insert_bof( struct reader *reader, struct node *bof )
{
    reader->root->parent = bof;
    list_add_tail( &bof->children, &reader->root->entry );
    reader->current = reader->root = bof;
}

static void read_insert_node( struct reader *reader, struct node *parent, struct node *node )
{
    node->parent = parent;
    if (node->parent == reader->root)
    {
        struct list *eof = list_tail( &reader->root->children );
        list_add_before( eof, &node->entry );
    }
    else list_add_tail( &parent->children, &node->entry );
    reader->current = node;
}

static HRESULT read_init_state( struct reader *reader )
{
    struct node *node;

    destroy_nodes( reader->root );
    reader->root = NULL;
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

static HRESULT read_attribute( struct reader *reader, WS_XML_ATTRIBUTE **ret )
{
    static const WS_XML_STRING xmlns = {5, (BYTE *)"xmlns"};
    WS_XML_ATTRIBUTE *attr;
    WS_XML_UTF8_TEXT *text;
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
    else if (!(text = alloc_utf8_text( start, len ))) goto error;

    attr->value = &text->text;
    attr->singleQuote = (quote == '\'');

    *ret = attr;
    return S_OK;

error:
    free_attribute( attr );
    return hr;
}

static int cmp_name( const unsigned char *name1, ULONG len1, const unsigned char *name2, ULONG len2 )
{
    ULONG i;
    if (len1 != len2) return 1;
    for (i = 0; i < len1; i++) { if (toupper( name1[i] ) != toupper( name2[i] )) return 1; }
    return 0;
}

static struct node *read_find_parent( struct reader *reader, const WS_XML_STRING *prefix,
                                      const WS_XML_STRING *localname )
{
    struct node *parent;
    const WS_XML_STRING *str;

    for (parent = reader->current; parent; parent = parent->parent)
    {
        if (node_type( parent ) == WS_XML_NODE_TYPE_BOF)
        {
            if (!localname) return parent;
            return NULL;
        }
        else if (node_type( parent ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            if (!localname) return parent;

            str = parent->hdr.prefix;
            if (cmp_name( str->bytes, str->length, prefix->bytes, prefix->length )) continue;
            str = parent->hdr.localName;
            if (cmp_name( str->bytes, str->length, localname->bytes, localname->length )) continue;
            return parent;
       }
    }

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
    struct node *node = NULL, *parent;
    WS_XML_ELEMENT_NODE *elem;
    WS_XML_ATTRIBUTE *attr = NULL;
    HRESULT hr = WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        struct list *eof = list_tail( &reader->root->children );
        reader->current = LIST_ENTRY( eof, struct node, entry );
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

    if (!(parent = read_find_parent( reader, NULL, NULL ))) goto error;

    hr = E_OUTOFMEMORY;
    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) goto error;
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
    free_node( node );
    return hr;
}

static HRESULT read_text( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node;
    WS_XML_TEXT_NODE *text;
    WS_XML_UTF8_TEXT *utf8;

    start = read_current_ptr( reader );
    for (;;)
    {
        if (read_end_of_data( reader )) break;
        if (!(ch = read_utf8_char( reader, &skip ))) return WS_E_INVALID_FORMAT;
        if (ch == '<') break;
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
    reader->state = READER_STATE_TEXT;
    return S_OK;
}

static HRESULT read_node( struct reader * );

static HRESULT read_startelement( struct reader *reader )
{
    struct node *node;

    read_skip_whitespace( reader );
    if (!read_cmp( reader, "/>", 2 ))
    {
        read_skip( reader, 2 );
        if (!(node = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT ))) return E_OUTOFMEMORY;
        read_insert_node( reader, reader->current, node );
        reader->state = READER_STATE_ENDELEMENT;
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

static HRESULT read_endelement( struct reader *reader )
{
    struct node *node, *parent;
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    WS_XML_STRING *prefix, *localname;
    HRESULT hr;

    if (reader->state == READER_STATE_EOF) return WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        struct list *eof = list_tail( &reader->root->children );
        reader->current = LIST_ENTRY( eof, struct node, entry );
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
    parent = read_find_parent( reader, prefix, localname );
    heap_free( prefix );
    heap_free( localname );
    if (!parent) return WS_E_INVALID_FORMAT;

    if (!(node = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT ))) return E_OUTOFMEMORY;
    read_insert_node( reader, parent, node );
    reader->state = READER_STATE_ENDELEMENT;
    return S_OK;
}

static HRESULT read_comment( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const unsigned char *start;
    struct node *node;
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

    if (!(node = alloc_node( WS_XML_NODE_TYPE_COMMENT ))) return E_OUTOFMEMORY;
    comment = (WS_XML_COMMENT_NODE *)node;
    if (!(comment->value.bytes = heap_alloc( len )))
    {
        heap_free( node );
        return E_OUTOFMEMORY;
    }
    memcpy( comment->value.bytes, start, len );
    comment->value.length = len;

    read_insert_node( reader, reader->current, node );
    reader->state = READER_STATE_COMMENT;
    return S_OK;
}

static HRESULT read_startcdata( struct reader *reader )
{
    struct node *node;

    if (read_cmp( reader, "<![CDATA[", 9 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 9 );

    if (!(node = alloc_node( WS_XML_NODE_TYPE_CDATA ))) return E_OUTOFMEMORY;
    read_insert_node( reader, reader->current, node );
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
    struct node *node;

    if (read_cmp( reader, "]]>", 3 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 3 );

    if (!(node = alloc_node( WS_XML_NODE_TYPE_END_CDATA ))) return E_OUTOFMEMORY;
    read_insert_node( reader, reader->current->parent, node );
    reader->state = READER_STATE_ENDCDATA;
    return S_OK;
}

static HRESULT read_node( struct reader *reader )
{
    HRESULT hr;

    for (;;)
    {
        if (read_end_of_data( reader ))
        {
            struct list *eof = list_tail( &reader->root->children );
            reader->current = LIST_ENTRY( eof, struct node, entry );
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

static BOOL move_to_root_element( struct reader *reader )
{
    struct list *ptr;
    struct node *node;

    if (!(ptr = list_head( &reader->root->children ))) return FALSE;
    node = LIST_ENTRY( ptr, struct node, entry );
    if (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        reader->current = node;
        return TRUE;
    }
    while ((ptr = list_next( &reader->root->children, &node->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            reader->current = next;
            return TRUE;
        }
        node = next;
    }
    return FALSE;
}

static BOOL move_to_next_element( struct reader *reader )
{
    struct list *ptr;
    struct node *node = reader->current;

    while ((ptr = list_next( &node->parent->children, &node->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            reader->current = next;
            return TRUE;
        }
        node = next;
    }
    return FALSE;
}

static BOOL move_to_prev_element( struct reader *reader )
{
    struct list *ptr;
    struct node *node = reader->current;

    while ((ptr = list_prev( &node->parent->children, &node->entry )))
    {
        struct node *prev = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( prev ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            reader->current = prev;
            return TRUE;
        }
        node = prev;
    }
    return FALSE;
}

static BOOL move_to_child_element( struct reader *reader )
{
    struct list *ptr;
    struct node *node;

    if (!(ptr = list_head( &reader->current->children ))) return FALSE;
    node = LIST_ENTRY( ptr, struct node, entry );
    if (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT)
    {
        reader->current = node;
        return TRUE;
    }
    while ((ptr = list_next( &reader->current->children, &node->entry )))
    {
        struct node *next = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( next ) == WS_XML_NODE_TYPE_ELEMENT)
        {
            reader->current = next;
            return TRUE;
        }
        node = next;
    }
    return FALSE;
}

static BOOL move_to_end_element( struct reader *reader )
{
    struct list *ptr;
    struct node *node = reader->current;

    if (node_type( node ) != WS_XML_NODE_TYPE_ELEMENT) return FALSE;

    if ((ptr = list_tail( &node->children )))
    {
        struct node *tail = LIST_ENTRY( ptr, struct node, entry );
        if (node_type( tail ) == WS_XML_NODE_TYPE_END_ELEMENT)
        {
            reader->current = tail;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL move_to_parent_element( struct reader *reader )
{
    struct node *parent = reader->current->parent;

    if (parent && (node_type( parent ) == WS_XML_NODE_TYPE_ELEMENT ||
                   node_type( parent ) == WS_XML_NODE_TYPE_BOF))
    {
        reader->current = parent;
        return TRUE;
    }
    return FALSE;
}

static HRESULT read_move_to( struct reader *reader, WS_MOVE_TO move, BOOL *found )
{
    struct list *ptr;
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
        success = move_to_root_element( reader );
        break;

    case WS_MOVE_TO_NEXT_ELEMENT:
        success = move_to_next_element( reader );
        break;

    case WS_MOVE_TO_PREVIOUS_ELEMENT:
        success = move_to_prev_element( reader );
        break;

    case WS_MOVE_TO_CHILD_ELEMENT:
        success = move_to_child_element( reader );
        break;

    case WS_MOVE_TO_END_ELEMENT:
        success = move_to_end_element( reader );
        break;

    case WS_MOVE_TO_PARENT_ELEMENT:
        success = move_to_parent_element( reader );
        break;

    case WS_MOVE_TO_FIRST_NODE:
        if ((ptr = list_head( &reader->current->parent->children )))
        {
            reader->current = LIST_ENTRY( ptr, struct node, entry );
            success = TRUE;
        }
        break;

    case WS_MOVE_TO_NEXT_NODE:
        if ((ptr = list_next( &reader->current->parent->children, &reader->current->entry )))
        {
            reader->current = LIST_ENTRY( ptr, struct node, entry );
            success = TRUE;
        }
        break;

    case WS_MOVE_TO_PREVIOUS_NODE:
        if ((ptr = list_prev( &reader->current->parent->children, &reader->current->entry )))
        {
            reader->current = LIST_ENTRY( ptr, struct node, entry );
            success = TRUE;
        }
        break;

    case WS_MOVE_TO_CHILD_NODE:
        if ((ptr = list_head( &reader->current->children )))
        {
            reader->current = LIST_ENTRY( ptr, struct node, entry );
            success = TRUE;
        }
        break;

    case WS_MOVE_TO_BOF:
        reader->current = reader->root;
        success = TRUE;
        break;

    case WS_MOVE_TO_EOF:
        if ((ptr = list_tail( &reader->root->children )))
        {
            reader->current = LIST_ENTRY( ptr, struct node, entry );
            success = TRUE;
        }
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

#define TICKS_PER_SEC   10000000
#define TICKS_PER_MIN   (60 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_PER_HOUR  (3600 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_PER_DAY   (86400 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_MAX       3155378975999999999

static const int month_offsets[2][12] =
{
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
};

static const int month_days[2][12] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static inline int is_leap_year( int year )
{
    return !(year % 4) && (year % 100 || !(year % 400));
}

static inline int valid_day( int year, int month, int day )
{
    return day > 0 && day <= month_days[is_leap_year( year )][month - 1];
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
    ret->ticks += month_offsets[is_leap_year( year )][month - 1] * TICKS_PER_DAY;
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

#define TICKS_1601_01_01    504911232000000000

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
        if (size != sizeof(BOOL)) return E_INVALIDARG;
        *(BOOL *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    INT64 val;
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
        if (size != sizeof(INT8)) return E_INVALIDARG;
        *(INT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    INT64 val;
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
        if (size != sizeof(INT16)) return E_INVALIDARG;
        *(INT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    INT64 val;
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
        if (size != sizeof(INT32)) return E_INVALIDARG;
        *(INT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    INT64 val;
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
        if (size != sizeof(INT64)) return E_INVALIDARG;
        *(INT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    UINT64 val;
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
        if (size != sizeof(UINT8)) return E_INVALIDARG;
        *(UINT8 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    UINT64 val;
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
        if (size != sizeof(UINT16)) return E_INVALIDARG;
        *(UINT16 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    UINT64 val;
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
        if (size != sizeof(UINT32)) return E_INVALIDARG;
        *(UINT32 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
    UINT64 val;
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
        if (size != sizeof(UINT64)) return E_INVALIDARG;
        *(UINT64 *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
        if (size != sizeof(int)) return E_INVALIDARG;
        *(int *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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
        if (size != sizeof(WS_DATETIME)) return E_INVALIDARG;
        *(WS_DATETIME *)ret = val;
        break;

    case WS_READ_REQUIRED_POINTER:
        if (!found) return WS_E_INVALID_FORMAT;
        /* fall through */

    case WS_READ_OPTIONAL_POINTER:
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

/* skips comment and empty text nodes */
static HRESULT read_type_next_node( struct reader *reader )
{
    for (;;)
    {
        HRESULT hr;
        WS_XML_NODE_TYPE type;

        if ((hr = read_node( reader )) != S_OK) return hr;
        type = node_type( reader->current );
        if (type == WS_XML_NODE_TYPE_COMMENT ||
            (type == WS_XML_NODE_TYPE_TEXT && is_empty_text_node( reader->current ))) continue;
        return S_OK;
    }
}

static HRESULT read_type_next_element_node( struct reader *reader, const WS_XML_STRING *localname,
                                            const WS_XML_STRING *ns )
{
    const WS_XML_ELEMENT_NODE *elem;
    HRESULT hr;
    BOOL found;

    if (!localname) return S_OK; /* assume reader is already correctly positioned */
    if ((hr = read_to_startelement( reader, &found )) != S_OK) return hr;
    if (!found) return WS_E_INVALID_FORMAT;

    elem = &reader->current->hdr;
    if (WsXmlStringEquals( localname, elem->localName, NULL ) == S_OK &&
        WsXmlStringEquals( ns, elem->ns, NULL ) == S_OK) return S_OK;

    return read_type_next_node( reader );
}

static ULONG get_type_size( WS_TYPE type, const WS_STRUCT_DESCRIPTION *desc )
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

    case WS_DATETIME_TYPE:
        return sizeof(WS_DATETIME);

    case WS_WSZ_TYPE:
        return sizeof(WCHAR *);

    case WS_STRUCT_TYPE:
        return desc->size;

    default:
        ERR( "unhandled type %u\n", type );
        return 0;
    }
}

static HRESULT read_type( struct reader *, WS_TYPE_MAPPING, WS_TYPE, const WS_XML_STRING *,
                          const WS_XML_STRING *, const void *, WS_READ_OPTION, WS_HEAP *,
                          void *, ULONG );

static HRESULT read_type_repeating_element( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                                            WS_READ_OPTION option, WS_HEAP *heap, void **ret,
                                            ULONG size, ULONG *count )
{
    HRESULT hr;
    ULONG item_size, nb_items = 0, nb_allocated = 1, offset = 0;
    char *buf;

    if (size != sizeof(void *)) return E_INVALIDARG;

    /* wrapper element */
    if (desc->localName && ((hr = read_type_next_element_node( reader, desc->localName, desc->ns )) != S_OK))
        return hr;

    item_size = get_type_size( desc->type, desc->typeDescription );
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
                        desc->typeDescription, WS_READ_REQUIRED_VALUE, heap, buf + offset, item_size );
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
    BOOL found;

    if ((hr = read_to_startelement( reader, &found )) != S_OK) return S_OK;
    if (!found) return WS_E_INVALID_FORMAT;
    if ((hr = read_node( reader )) != S_OK) return hr;
    if (node_type( reader->current ) != WS_XML_NODE_TYPE_TEXT) return WS_E_INVALID_FORMAT;

    return read_type( reader, WS_ANY_ELEMENT_TYPE_MAPPING, desc->type, NULL, NULL,
                      desc->typeDescription, option, heap, ret, size );
}

static WS_READ_OPTION map_field_options( WS_TYPE type, ULONG options )
{
    if (options & ~(WS_FIELD_POINTER | WS_FIELD_OPTIONAL))
    {
        FIXME( "options %08x not supported\n", options );
        return 0;
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
    case WS_ENUM_TYPE:
    case WS_DATETIME_TYPE:
        return WS_READ_REQUIRED_VALUE;

    case WS_WSZ_TYPE:
    case WS_STRUCT_TYPE:
        return WS_READ_REQUIRED_POINTER;

    default:
        FIXME( "unhandled type %u\n", type );
        return 0;
    }
}

static HRESULT read_type_struct_field( struct reader *reader, const WS_FIELD_DESCRIPTION *desc,
                                       WS_HEAP *heap, char *buf )
{
    char *ptr;
    WS_READ_OPTION option;
    ULONG size;
    HRESULT hr;

    if (!desc || !(option = map_field_options( desc->type, desc->options ))) return E_INVALIDARG;

    if (option == WS_READ_REQUIRED_VALUE)
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
        hr = read_type_repeating_element( reader, desc, option, heap, (void **)ptr, size, &count );
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

    if (hr == WS_E_INVALID_FORMAT && desc->options & WS_FIELD_OPTIONAL)
    {
        switch (option)
        {
        case WS_READ_REQUIRED_VALUE:
            if (desc->defaultValue) memcpy( ptr, desc->defaultValue->value, desc->defaultValue->valueSize );
            else memset( ptr, 0, size );
            return S_OK;

        case WS_READ_REQUIRED_POINTER:
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

    if (desc->structOptions)
    {
        FIXME( "struct options %08x not supported\n", desc->structOptions );
        return E_NOTIMPL;
    }

    switch (option)
    {
    case WS_READ_REQUIRED_POINTER:
    case WS_READ_OPTIONAL_POINTER:
        if (size != sizeof(void *)) return E_INVALIDARG;
        if (!(buf = ws_alloc_zero( heap, desc->size ))) return WS_E_QUOTA_EXCEEDED;
        break;

    case WS_READ_REQUIRED_VALUE:
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
        if (hr != S_OK)
        {
            ws_free( heap, buf );
            buf = NULL;
        }
        *(char **)ret = buf;
        return S_OK;

    case WS_READ_REQUIRED_VALUE:
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
    case WS_STRUCT_TYPE:
        if ((hr = read_type_struct( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

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

    case WS_WSZ_TYPE:
        if ((hr = read_type_wsz( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_ENUM_TYPE:
        if ((hr = read_type_enum( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
            return hr;
        break;

    case WS_DATETIME_TYPE:
        if ((hr = read_type_datetime( reader, mapping, localname, ns, desc, option, heap, value, size )) != S_OK)
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

static void set_input_buffer( struct reader *reader, const unsigned char *data, ULONG size )
{
    reader->input_type  = WS_XML_READER_INPUT_TYPE_BUFFER;
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
        set_input_buffer( reader, (const unsigned char *)buf->encodedData + offset, buf->encodedDataSize - offset );
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

    set_input_buffer( reader, (const unsigned char *)xmlbuf->ptr + offset, xmlbuf->size - offset );
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
