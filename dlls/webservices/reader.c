/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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

static const char *debugstr_xmlstr( const WS_XML_STRING *str )
{
    if (!str) return "(null)";
    return debugstr_an( (const char *)str->bytes, str->length );
}

static const struct
{
    ULONG size;
    BOOL  readonly;
}
error_props[] =
{
    { sizeof(ULONG), TRUE },    /* WS_ERROR_PROPERTY_STRING_COUNT */
    { sizeof(ULONG), FALSE },   /* WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE */
    { sizeof(LANGID), FALSE }   /* WS_ERROR_PROPERTY_LANGID */
};

struct error
{
    ULONG             prop_count;
    WS_ERROR_PROPERTY prop[sizeof(error_props)/sizeof(error_props[0])];
};

static struct error *alloc_error(void)
{
    static const ULONG count = sizeof(error_props)/sizeof(error_props[0]);
    struct error *ret;
    ULONG i, size = sizeof(*ret) + count * sizeof(WS_ERROR_PROPERTY);
    char *ptr;

    for (i = 0; i < count; i++) size += error_props[i].size;
    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ptr = (char *)&ret->prop[count];
    for (i = 0; i < count; i++)
    {
        ret->prop[i].value = ptr;
        ret->prop[i].valueSize = error_props[i].size;
        ptr += ret->prop[i].valueSize;
    }
    ret->prop_count = count;
    return ret;
}

static HRESULT set_error_prop( struct error *error, WS_ERROR_PROPERTY_ID id, const void *value, ULONG size )
{
    if (id >= error->prop_count || size != error_props[id].size || error_props[id].readonly)
        return E_INVALIDARG;

    memcpy( error->prop[id].value, value, size );
    return S_OK;
}

static HRESULT get_error_prop( struct error *error, WS_ERROR_PROPERTY_ID id, void *buf, ULONG size )
{
    if (id >= error->prop_count || size != error_props[id].size)
        return E_INVALIDARG;

    memcpy( buf, error->prop[id].value, error->prop[id].valueSize );
    return S_OK;
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

    set_error_prop( error, WS_ERROR_PROPERTY_LANGID, &langid, sizeof(langid) );
    for (i = 0; i < count; i++)
    {
        if (properties[i].id == WS_ERROR_PROPERTY_ORIGINAL_ERROR_CODE)
        {
            heap_free( error );
            return E_INVALIDARG;
        }
        hr = set_error_prop( error, properties[i].id, properties[i].value, properties[i].valueSize );
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

static const struct
{
    ULONG size;
    BOOL  readonly;
}
heap_props[] =
{
    { sizeof(SIZE_T), FALSE }, /* WS_HEAP_PROPERTY_MAX_SIZE */
    { sizeof(SIZE_T), FALSE }, /* WS_HEAP_PROPERTY_TRIM_SIZE */
    { sizeof(SIZE_T), TRUE },  /* WS_HEAP_PROPERTY_REQUESTED_SIZE */
    { sizeof(SIZE_T), TRUE }   /* WS_HEAP_PROPERTY_ACTUAL_SIZE */
};

struct heap
{
    HANDLE           handle;
    ULONG            prop_count;
    WS_HEAP_PROPERTY prop[sizeof(heap_props)/sizeof(heap_props[0])];
};

static struct heap *alloc_heap(void)
{
    static const ULONG count = sizeof(heap_props)/sizeof(heap_props[0]);
    struct heap *ret;
    ULONG i, size = sizeof(*ret) + count * sizeof(WS_HEAP_PROPERTY);
    char *ptr;

    for (i = 0; i < count; i++) size += heap_props[i].size;
    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ptr = (char *)&ret->prop[count];
    for (i = 0; i < count; i++)
    {
        ret->prop[i].value = ptr;
        ret->prop[i].valueSize = heap_props[i].size;
        ptr += ret->prop[i].valueSize;
    }
    ret->prop_count = count;
    return ret;
}

static HRESULT set_heap_prop( struct heap *heap, WS_HEAP_PROPERTY_ID id, const void *value, ULONG size )
{
    if (id >= heap->prop_count || size != heap_props[id].size || heap_props[id].readonly)
        return E_INVALIDARG;

    memcpy( heap->prop[id].value, value, size );
    return S_OK;
}

static HRESULT get_heap_prop( struct heap *heap, WS_HEAP_PROPERTY_ID id, void *buf, ULONG size )
{
    if (id >= heap->prop_count || size != heap_props[id].size)
        return E_INVALIDARG;

    memcpy( buf, heap->prop[id].value, heap->prop[id].valueSize );
    return S_OK;
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

    set_heap_prop( heap, WS_HEAP_PROPERTY_MAX_SIZE, &max_size, sizeof(max_size) );
    set_heap_prop( heap, WS_HEAP_PROPERTY_TRIM_SIZE, &trim_size, sizeof(trim_size) );

    if (!(heap->handle = HeapCreate( 0, 0, max_size )))
    {
        heap_free( heap );
        return E_OUTOFMEMORY;
    }

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

struct node
{
    WS_XML_ELEMENT_NODE hdr;
    struct list entry;
};

static struct node *alloc_node( WS_XML_NODE_TYPE type )
{
    struct node *ret;

    if (!(ret = heap_alloc_zero( sizeof(*ret) ))) return NULL;
    ret->hdr.node.nodeType = type;
    list_init( &ret->entry );
    return ret;
}

static void free_attribute( WS_XML_ATTRIBUTE *attr )
{
    if (!attr) return;
    heap_free( attr->prefix );
    heap_free( attr->localName );
    heap_free( attr->ns );
    heap_free( attr->value );
    heap_free( attr );
}

static void free_node( struct node *node )
{
    if (!node) return;
    switch (node->hdr.node.nodeType)
    {
    case WS_XML_NODE_TYPE_ELEMENT:
    {
        WS_XML_ELEMENT_NODE *elem = (WS_XML_ELEMENT_NODE *)node;
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
    }
    case WS_XML_NODE_TYPE_END_ELEMENT:
    case WS_XML_NODE_TYPE_EOF:
    case WS_XML_NODE_TYPE_BOF:
        break;

    default:
        ERR( "unhandled type %u\n", node->hdr.node.nodeType );
        break;
    }
    heap_free( node );
}

static void destroy_nodes( struct list *list )
{
    struct list *ptr;

    while ((ptr = list_head( list )))
    {
        struct node *node = LIST_ENTRY( ptr, struct node, entry );
        list_remove( &node->entry );
        free_node( node );
    }
}

static const struct
{
    ULONG size;
    BOOL  readonly;
}
reader_props[] =
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
    READER_STATE_TEXT,
    READER_STATE_ENDELEMENT,
    READER_STATE_COMMENT,
    READER_STATE_EOF
};

struct reader
{
    ULONG                   read_size;
    ULONG                   read_pos;
    const char             *read_bufptr;
    enum reader_state       state;
    struct list             nodes;
    struct node            *current;
    const char             *input_data;
    ULONG                   input_size;
    ULONG                   prop_count;
    WS_XML_READER_PROPERTY  prop[sizeof(reader_props)/sizeof(reader_props[0])];
};

static struct reader *alloc_reader(void)
{
    static const ULONG count = sizeof(reader_props)/sizeof(reader_props[0]);
    struct reader *ret;
    ULONG i, size = sizeof(*ret) + count * sizeof(WS_XML_READER_PROPERTY);
    char *ptr;

    for (i = 0; i < count; i++) size += reader_props[i].size;
    if (!(ret = heap_alloc_zero( size ))) return NULL;

    ptr = (char *)&ret->prop[count];
    for (i = 0; i < count; i++)
    {
        ret->prop[i].value = ptr;
        ret->prop[i].valueSize = reader_props[i].size;
        ptr += ret->prop[i].valueSize;
    }
    ret->prop_count = count;
    return ret;
}

static HRESULT set_reader_prop( struct reader *reader, WS_XML_READER_PROPERTY_ID id, const void *value, ULONG size )
{
    if (id >= reader->prop_count || size != reader_props[id].size || reader_props[id].readonly)
        return E_INVALIDARG;

    memcpy( reader->prop[id].value, value, size );
    return S_OK;
}

static HRESULT get_reader_prop( struct reader *reader, WS_XML_READER_PROPERTY_ID id, void *buf, ULONG size )
{
    if (id >= reader->prop_count || size != reader_props[id].size)
        return E_INVALIDARG;

    memcpy( buf, reader->prop[id].value, reader->prop[id].valueSize );
    return S_OK;
}

/**************************************************************************
 *          WsCreateReader		[webservices.@]
 */
HRESULT WINAPI WsCreateReader( const WS_XML_READER_PROPERTY *properties, ULONG count,
                               WS_XML_READER **handle, WS_ERROR *error )
{
    struct reader *reader;
    struct node *node;
    ULONG i, max_depth = 32, max_attrs = 128, max_ns = 32;
    WS_CHARSET charset = WS_CHARSET_UTF8;
    BOOL read_decl = TRUE;
    HRESULT hr;

    TRACE( "%p %u %p %p\n", properties, count, handle, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!handle) return E_INVALIDARG;
    if (!(reader = alloc_reader())) return E_OUTOFMEMORY;

    set_reader_prop( reader, WS_XML_READER_PROPERTY_MAX_DEPTH, &max_depth, sizeof(max_depth) );
    set_reader_prop( reader, WS_XML_READER_PROPERTY_MAX_ATTRIBUTES, &max_attrs, sizeof(max_attrs) );
    set_reader_prop( reader, WS_XML_READER_PROPERTY_READ_DECLARATION, &read_decl, sizeof(read_decl) );
    set_reader_prop( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, sizeof(charset) );
    set_reader_prop( reader, WS_XML_READER_PROPERTY_MAX_NAMESPACES, &max_ns, sizeof(max_ns) );

    for (i = 0; i < count; i++)
    {
        hr = set_reader_prop( reader, properties[i].id, properties[i].value, properties[i].valueSize );
        if (hr != S_OK)
        {
            heap_free( reader );
            return hr;
        }
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_EOF )))
    {
        heap_free( reader );
        return E_OUTOFMEMORY;
    }
    list_init( &reader->nodes );
    list_add_tail( &reader->nodes, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_INITIAL;

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
    destroy_nodes( &reader->nodes );
    heap_free( reader );
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
    return get_error_prop( error, id, buf, size );
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

    return get_heap_prop( heap, id, buf, size );
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

    if (!reader->input_data) return WS_E_INVALID_OPERATION;

    if (id == WS_XML_READER_PROPERTY_CHARSET)
    {
        WS_CHARSET charset;
        HRESULT hr;

        if ((hr = get_reader_prop( reader, id, &charset, size )) != S_OK) return hr;
        if (!charset) return WS_E_INVALID_FORMAT;
        *(WS_CHARSET *)buf = charset;
        return S_OK;
    }
    return get_reader_prop( reader, id, buf, size );
}

/**************************************************************************
 *          WsGetXmlAttribute		[webservices.@]
 */
HRESULT WINAPI WsGetXmlAttribute( WS_XML_READER *handle, const WS_XML_STRING *attr,
                                  WS_HEAP *heap, WCHAR **str, ULONG *len, WS_ERROR *error )
{
    FIXME( "%p %p %p %p %p %p: stub\n", handle, debugstr_xmlstr(attr), heap, str, len, error );
    return E_NOTIMPL;
}

static WS_XML_STRING *alloc_xml_string( const char *data, ULONG len )
{
    WS_XML_STRING *ret;

    if (!(ret = heap_alloc( sizeof(*ret) + len ))) return NULL;
    ret->length     = len;
    ret->bytes      = (BYTE *)(ret + 1);
    ret->dictionary = NULL;
    ret->id         = 0;
    if (data) memcpy( ret->bytes, data, len );
    return ret;
}

static WS_XML_UTF8_TEXT *alloc_utf8_text( const char *data, ULONG len )
{
    WS_XML_UTF8_TEXT *ret;

    if (!(ret = heap_alloc( sizeof(*ret) + len ))) return NULL;
    ret->text.textType    = WS_XML_TEXT_TYPE_UTF8;
    ret->value.length     = len;
    ret->value.bytes      = (BYTE *)(ret + 1);
    ret->value.dictionary = NULL;
    ret->value.id         = 0;
    if (data) memcpy( ret->value.bytes, data, len );
    return ret;
}

static inline BOOL read_end_of_data( struct reader *reader )
{
    return reader->read_pos >= reader->read_size;
}

static inline const char *read_current_ptr( struct reader *reader )
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
    const char *end;

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
    while (reader->read_pos < reader->read_size && count)
    {
        reader->read_pos++;
        count--;
    }
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
    const char *ptr = read_current_ptr( reader );

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

static HRESULT append_attribute( WS_XML_ELEMENT_NODE *elem, WS_XML_ATTRIBUTE *attr )
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

static HRESULT read_attribute( struct reader *reader, WS_XML_ATTRIBUTE **ret )
{
    WS_XML_ATTRIBUTE *attr;
    WS_XML_UTF8_TEXT *text;
    unsigned int len = 0, ch, skip, quote;
    const char *start;
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

    hr = E_OUTOFMEMORY;
    if (!(attr->localName = alloc_xml_string( start, len ))) goto error;
    if (!(attr->prefix = alloc_xml_string( NULL, 0 ))) goto error;
    attr->prefix->bytes = NULL;
    if (!(attr->ns = alloc_xml_string( NULL, 0 ))) goto error;
    attr->ns->bytes = NULL;

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

    if (!(text = alloc_utf8_text( start, len )))
    {
        free_attribute( attr );
        return E_OUTOFMEMORY;
    }
    attr->value = (WS_XML_TEXT *)text;
    attr->singleQuote = (quote == '\'');

    *ret = attr;
    return S_OK;

error:
    free_attribute( attr );
    return hr;
}

static HRESULT read_element( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const char *start;
    struct node *node;
    WS_XML_ELEMENT_NODE *elem;
    WS_XML_ATTRIBUTE *attr;
    HRESULT hr = WS_E_INVALID_FORMAT;

    if (read_end_of_data( reader ))
    {
        reader->current = LIST_ENTRY( list_tail( &reader->nodes ), struct node, entry );
        reader->state   = READER_STATE_EOF;
        return S_OK;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_ELEMENT ))) return E_OUTOFMEMORY;
    elem = (WS_XML_ELEMENT_NODE *)node;

    if (read_cmp( reader, "<", 1 )) goto error;
    read_skip( reader, 1 );

    start = read_current_ptr( reader );
    for (;;)
    {
        if (!(ch = read_utf8_char( reader, &skip ))) goto error;
        if (!read_isnamechar( ch )) break;
        read_skip( reader, skip );
        len += skip;
    }
    if (!len) goto error;

    hr = E_OUTOFMEMORY;
    if (!(elem->localName = alloc_xml_string( start, len ))) goto error;
    if (!(elem->prefix = alloc_xml_string( NULL, 0 ))) goto error;
    elem->prefix->bytes = NULL;
    if (!(elem->ns = alloc_xml_string( NULL, 0 ))) goto error;

    for (;;)
    {
        read_skip_whitespace( reader );
        if (!read_cmp( reader, ">", 1 )) break;
        if ((hr = read_attribute( reader, &attr )) != S_OK) goto error;
        if ((hr = append_attribute( elem, attr )) != S_OK)
        {
            free_attribute( attr );
            goto error;
        }
    }

    read_skip_whitespace( reader );
    if (read_cmp( reader, ">", 1 ))
    {
        hr = WS_E_INVALID_FORMAT;
        goto error;
    }
    read_skip( reader, 1 );

    list_add_after( &reader->current->entry, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_STARTELEMENT;
    return S_OK;

error:
    free_node( node );
    return hr;
}

static HRESULT read_text( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const char *start;
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
    text->text = (WS_XML_TEXT *)utf8;

    list_add_after( &reader->current->entry, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_TEXT;
    return S_OK;
}

static HRESULT read_startelement( struct reader *reader )
{
    if (!read_cmp( reader, "<", 1 )) return read_element( reader );
    return read_text( reader );
}

static HRESULT read_to_startelement( struct reader *reader, BOOL *found )
{
    HRESULT hr;

    switch (reader->state)
    {
    case READER_STATE_INITIAL:
    {
        if ((hr = read_xmldecl( reader )) != S_OK) return hr;
        break;
    }
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
    struct node *node;
    unsigned int ch, skip;

    if (reader->state != READER_STATE_TEXT) return WS_E_INVALID_FORMAT;

    if (read_cmp( reader, "</", 2 )) return WS_E_INVALID_FORMAT;
    read_skip( reader, 2 );

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
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_END_ELEMENT ))) return E_OUTOFMEMORY;
    list_add_after( &reader->current->entry, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_ENDELEMENT;
    return S_OK;
}

static HRESULT read_comment( struct reader *reader )
{
    unsigned int len = 0, ch, skip;
    const char *start;
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

    list_add_after( &reader->current->entry, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_COMMENT;
    return S_OK;
}

static HRESULT read_node( struct reader *reader )
{
    HRESULT hr;

    for (;;)
    {
        if (read_end_of_data( reader ))
        {
            reader->current = LIST_ENTRY( list_tail( &reader->nodes ), struct node, entry );
            reader->state   = READER_STATE_EOF;
            return S_OK;
        }
        if (!read_cmp( reader, "<?", 2 ))
        {
            hr = read_xmldecl( reader );
            if (FAILED( hr )) return hr;
        }
        else if (!read_cmp( reader, "</", 2 )) return read_endelement( reader );
        else if (!read_cmp( reader, "<!", 2 )) return read_comment( reader );
        else if (!read_cmp( reader, "<", 1 )) return read_startelement( reader );
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

static void *read_alloc( WS_HEAP *handle, SIZE_T size )
{
    struct heap *heap = (struct heap *)handle;
    return HeapAlloc( heap->handle, 0, size );
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
        if (!(ret = read_alloc( heap, (len + 1) * sizeof(WCHAR) ))) return NULL;
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

static HRESULT str_to_int64( const char *str, ULONG len, INT64 min, INT64 max, INT64 *ret )
{
    BOOL negative = FALSE;
    const char *ptr = str;

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

static HRESULT str_to_uint64( const char *str, ULONG len, UINT64 max, UINT64 *ret )
{
    const char *ptr = str;

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

/**************************************************************************
 *          WsReadType		[webservices.@]
 */
HRESULT WINAPI WsReadType( WS_XML_READER *handle, WS_TYPE_MAPPING mapping, WS_TYPE type,
                           const void *desc, WS_READ_OPTION option, WS_HEAP *heap, void *value,
                           ULONG value_size, WS_ERROR *error )
{
    struct reader *reader = (struct reader *)handle;
    WS_XML_TEXT_NODE *text;

    TRACE( "%p %u %u %p %u %p %p %u %p\n", handle, mapping, type, desc, option, heap, value,
           value_size, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!reader || !value) return E_INVALIDARG;

    if (reader->current->hdr.node.nodeType != WS_XML_NODE_TYPE_TEXT)
    {
        FIXME( "only text nodes are supported\n" );
        return E_NOTIMPL;
    }
    text = (WS_XML_TEXT_NODE *)&reader->current->hdr.node;
    if (text->text->textType != WS_XML_TEXT_TYPE_UTF8)
    {
        FIXME( "text type %u not supported\n", text->text->textType );
        return E_NOTIMPL;
    }

    switch (mapping)
    {
    case WS_ELEMENT_CONTENT_TYPE_MAPPING:
        break;
    default:
        FIXME( "mapping %u not supported\n", mapping );
        return E_NOTIMPL;
    }

    switch (type)
    {
    case WS_BOOL_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        ULONG len = utf8->value.length;
        BOOL *ret = value;

        if (value_size != sizeof(BOOL)) return E_INVALIDARG;

        if (len == 4 && !memcmp( utf8->value.bytes, "true", 4 )) *ret = TRUE;
        else if (len == 1 && !memcmp( utf8->value.bytes, "1", 1 )) *ret = TRUE;
        else if (len == 5 && !memcmp( utf8->value.bytes, "false", 5 )) *ret = FALSE;
        else if (len == 1 && !memcmp( utf8->value.bytes, "0", 1 )) *ret = FALSE;
        else return WS_E_INVALID_FORMAT;
        break;
    }
    case WS_INT8_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        INT8 *ret = value;
        HRESULT hr;
        INT64 val;

        if (value_size != sizeof(INT8)) return E_INVALIDARG;
        hr = str_to_int64( (const char *)utf8->value.bytes, utf8->value.length, MIN_INT8, MAX_INT8, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_INT16_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        INT16 *ret = value;
        HRESULT hr;
        INT64 val;

        if (value_size != sizeof(INT16)) return E_INVALIDARG;
        hr = str_to_int64( (const char *)utf8->value.bytes, utf8->value.length, MIN_INT16, MAX_INT16, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_INT32_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        INT32 *ret = value;
        HRESULT hr;
        INT64 val;

        if (value_size != sizeof(INT32)) return E_INVALIDARG;
        hr = str_to_int64( (const char *)utf8->value.bytes, utf8->value.length, MIN_INT32, MAX_INT32, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_INT64_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        INT64 val, *ret = value;
        HRESULT hr;

        if (value_size != sizeof(INT64)) return E_INVALIDARG;
        hr = str_to_int64( (const char *)utf8->value.bytes, utf8->value.length, MIN_INT64, MAX_INT64, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_UINT8_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        UINT8 *ret = value;
        HRESULT hr;
        UINT64 val;

        if (value_size != sizeof(UINT8)) return E_INVALIDARG;
        hr = str_to_uint64( (const char *)utf8->value.bytes, utf8->value.length, MAX_UINT8, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_UINT16_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        UINT16 *ret = value;
        HRESULT hr;
        UINT64 val;

        if (value_size != sizeof(UINT16)) return E_INVALIDARG;
        hr = str_to_uint64( (const char *)utf8->value.bytes, utf8->value.length, MAX_UINT16, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_UINT32_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        UINT32 *ret = value;
        HRESULT hr;
        UINT64 val;

        if (value_size != sizeof(UINT32)) return E_INVALIDARG;
        hr = str_to_uint64( (const char *)utf8->value.bytes, utf8->value.length, MAX_UINT32, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_UINT64_TYPE:
    {
        WS_XML_UTF8_TEXT *utf8 = (WS_XML_UTF8_TEXT *)text->text;
        UINT64 val, *ret = value;
        HRESULT hr;

        if (value_size != sizeof(UINT64)) return E_INVALIDARG;
        hr = str_to_uint64( (const char *)utf8->value.bytes, utf8->value.length, MAX_UINT64, &val );
        if (hr != S_OK) return hr;
        *ret = val;
        break;
    }
    case WS_WSZ_TYPE:
    {
        WCHAR *str, **ret = value;

        if (value_size != sizeof(WCHAR *)) return E_INVALIDARG;
        if (!(str = xmltext_to_widechar( heap, text->text ))) return E_OUTOFMEMORY;
        *ret = str;
        break;
    }
    default:
        FIXME( "type %u not supported\n", type );
        return E_NOTIMPL;
    }

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
    return set_error_prop( error, id, value, size );
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

static HRESULT detect_charset( const WS_XML_READER_INPUT *input, WS_CHARSET *charset, ULONG *offset )
{
    const WS_XML_READER_BUFFER_INPUT *buf = (const WS_XML_READER_BUFFER_INPUT *)input;

    if (input->inputType != WS_XML_READER_INPUT_TYPE_BUFFER)
    {
        FIXME( "charset detection on input type %u not supported\n", input->inputType );
        return E_NOTIMPL;
    }

    /* FIXME: parse xml declaration */

    if (is_utf16le( buf->encodedData, buf->encodedDataSize, offset ))
    {
        *charset = WS_CHARSET_UTF16LE;
    }
    else if (is_utf8( buf->encodedData, buf->encodedDataSize, offset ))
    {
        *charset = WS_CHARSET_UTF8;
    }
    else
    {
        FIXME( "charset not recognized\n" );
        *charset = 0;
    }

    TRACE( "detected charset %u\n", *charset );
    return S_OK;
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

    switch (encoding->encodingType)
    {
        case WS_XML_READER_ENCODING_TYPE_TEXT:
        {
            WS_XML_READER_TEXT_ENCODING *text = (WS_XML_READER_TEXT_ENCODING *)encoding;
            WS_CHARSET charset = text->charSet;

            if (charset == WS_CHARSET_AUTO && (hr = detect_charset( input, &charset, &offset )) != S_OK)
                return hr;

            hr = set_reader_prop( reader, WS_XML_READER_PROPERTY_CHARSET, &charset, sizeof(charset) );
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
            reader->input_data = (const char *)buf->encodedData + offset;
            reader->input_size = buf->encodedDataSize - offset;
            reader->read_bufptr = reader->input_data;
            break;
        }
        default:
            FIXME( "input type %u not supported\n", input->inputType );
            return E_NOTIMPL;
    }

    for (i = 0; i < count; i++)
    {
        hr = set_reader_prop( reader, properties[i].id, properties[i].value, properties[i].valueSize );
        if (hr != S_OK) return hr;
    }

    if (!(node = alloc_node( WS_XML_NODE_TYPE_BOF ))) return E_OUTOFMEMORY;
    list_add_head( &reader->nodes, &node->entry );
    reader->current = node;
    reader->state   = READER_STATE_INITIAL;

    return S_OK;
}
