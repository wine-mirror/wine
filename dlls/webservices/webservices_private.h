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

struct xmlbuf
{
    WS_HEAP *heap;
    void    *ptr;
    SIZE_T   size_allocated;
    SIZE_T   size;
};

void *ws_alloc( WS_HEAP *, SIZE_T ) DECLSPEC_HIDDEN;
void *ws_realloc( WS_HEAP *, void *, SIZE_T ) DECLSPEC_HIDDEN;
void ws_free( WS_HEAP *, void * ) DECLSPEC_HIDDEN;
const char *debugstr_xmlstr( const WS_XML_STRING * ) DECLSPEC_HIDDEN;
WS_XML_STRING *alloc_xml_string( const unsigned char *, ULONG ) DECLSPEC_HIDDEN;
WS_XML_UTF8_TEXT *alloc_utf8_text( const unsigned char *, ULONG ) DECLSPEC_HIDDEN;
HRESULT append_attribute( WS_XML_ELEMENT_NODE *, WS_XML_ATTRIBUTE * ) DECLSPEC_HIDDEN;
void free_attribute( WS_XML_ATTRIBUTE * ) DECLSPEC_HIDDEN;

struct node
{
    WS_XML_ELEMENT_NODE hdr;
    struct list         entry;
    struct node        *parent;
    struct list         children;
};

struct node *alloc_node( WS_XML_NODE_TYPE ) DECLSPEC_HIDDEN;
void free_node( struct node * ) DECLSPEC_HIDDEN;
void destroy_nodes( struct node * ) DECLSPEC_HIDDEN;

static inline WS_XML_NODE_TYPE node_type( const struct node *node )
{
    return node->hdr.node.nodeType;
}

struct prop_desc
{
    ULONG size;
    BOOL  readonly;
    BOOL  writeonly;
};

struct prop
{
    void  *value;
    ULONG  size;
    BOOL   readonly;
    BOOL   writeonly;
};

ULONG prop_size( const struct prop_desc *, ULONG ) DECLSPEC_HIDDEN;
void prop_init( const struct prop_desc *, ULONG, struct prop *, void * ) DECLSPEC_HIDDEN;
HRESULT prop_set( const struct prop *, ULONG, ULONG, const void *, ULONG ) DECLSPEC_HIDDEN;
HRESULT prop_get( const struct prop *, ULONG, ULONG, void *, ULONG ) DECLSPEC_HIDDEN;

struct channel
{
    WS_CHANNEL_TYPE         type;
    WS_CHANNEL_BINDING      binding;
    WS_CHANNEL_STATE        state;
    ULONG                   prop_count;
    struct prop             prop[9];
};

HRESULT create_channel( WS_CHANNEL_TYPE, WS_CHANNEL_BINDING, const WS_CHANNEL_PROPERTY *,
                        ULONG, struct channel ** ) DECLSPEC_HIDDEN;
void free_channel( struct channel * ) DECLSPEC_HIDDEN;
HRESULT open_channel( struct channel *, const WS_ENDPOINT_ADDRESS * ) DECLSPEC_HIDDEN;
HRESULT close_channel( struct channel * ) DECLSPEC_HIDDEN;

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline void *heap_alloc_zero( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
}

static inline void *heap_realloc( void *mem, SIZE_T size )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, size );
}

static inline void *heap_realloc_zero( void *mem, SIZE_T size )
{
    return HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}
