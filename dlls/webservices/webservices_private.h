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

#include "winhttp.h"

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
WS_TYPE map_value_type( WS_VALUE_TYPE ) DECLSPEC_HIDDEN;
BOOL set_fpword( unsigned short, unsigned short * ) DECLSPEC_HIDDEN;
void restore_fpword( unsigned short ) DECLSPEC_HIDDEN;
HRESULT set_output( WS_XML_WRITER * ) DECLSPEC_HIDDEN;
HRESULT set_input( WS_XML_READER *, char *, ULONG ) DECLSPEC_HIDDEN;
ULONG get_type_size( WS_TYPE, const WS_STRUCT_DESCRIPTION * ) DECLSPEC_HIDDEN;

#define INVALID_PARAMETER_INDEX 0xffff
HRESULT get_param_desc( const WS_STRUCT_DESCRIPTION *, USHORT, const WS_FIELD_DESCRIPTION ** ) DECLSPEC_HIDDEN;
HRESULT write_input_params( WS_XML_WRITER *, const WS_ELEMENT_DESCRIPTION *,
                            const WS_PARAMETER_DESCRIPTION *, ULONG, const void ** ) DECLSPEC_HIDDEN;
HRESULT read_output_params( WS_XML_READER *, WS_HEAP *, const WS_ELEMENT_DESCRIPTION *,
                            const WS_PARAMETER_DESCRIPTION *, ULONG, const void ** ) DECLSPEC_HIDDEN;

enum node_flag
{
    NODE_FLAG_IGNORE_TRAILING_ELEMENT_CONTENT   = 0x1,
};

struct node
{
    WS_XML_ELEMENT_NODE hdr;
    struct list         entry;
    struct node        *parent;
    struct list         children;
    ULONG               flags;
};

struct node *alloc_node( WS_XML_NODE_TYPE ) DECLSPEC_HIDDEN;
void free_node( struct node * ) DECLSPEC_HIDDEN;
void destroy_nodes( struct node * ) DECLSPEC_HIDDEN;
HRESULT copy_node( WS_XML_READER *, struct node ** ) DECLSPEC_HIDDEN;

static inline WS_XML_NODE_TYPE node_type( const struct node *node )
{
    return node->hdr.node.nodeType;
}

BOOL move_to_root_element( struct node *, struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_next_element( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_prev_element( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_child_element( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_end_element( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_parent_element( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_first_node( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_next_node( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_prev_node( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_bof( struct node *, struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_eof( struct node *, struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_child_node( struct node ** ) DECLSPEC_HIDDEN;
BOOL move_to_parent_node( struct node ** ) DECLSPEC_HIDDEN;

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

HRESULT message_set_action( WS_MESSAGE *, const WS_XML_STRING * ) DECLSPEC_HIDDEN;
void message_set_send_context( WS_MESSAGE *, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT * ) DECLSPEC_HIDDEN;
void message_set_receive_context( WS_MESSAGE *, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT * ) DECLSPEC_HIDDEN;
void message_do_send_callback( WS_MESSAGE * ) DECLSPEC_HIDDEN;
void message_do_receive_callback( WS_MESSAGE * ) DECLSPEC_HIDDEN;
HRESULT message_insert_http_headers( WS_MESSAGE *, HINTERNET ) DECLSPEC_HIDDEN;

HRESULT channel_send_message( WS_CHANNEL *, WS_MESSAGE * ) DECLSPEC_HIDDEN;
HRESULT channel_receive_message( WS_CHANNEL *, char **, ULONG * ) DECLSPEC_HIDDEN;

#define TICKS_PER_SEC       10000000
#define TICKS_PER_MIN       (60 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_PER_HOUR      (3600 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_PER_DAY       (86400 * (ULONGLONG)TICKS_PER_SEC)
#define TICKS_MAX           3155378975999999999
#define TICKS_1601_01_01    504911232000000000

static const int month_days[2][12] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static inline int leap_year( int year )
{
    return !(year % 4) && (year % 100 || !(year % 400));
}

static inline BOOL is_nil_value( const char *value, ULONG size )
{
    ULONG i;
    for (i = 0; i < size; i++) if (value[i]) return FALSE;
    return TRUE;
}

static inline BOOL is_valid_parent( const struct node *node )
{
    if (!node) return FALSE;
    return (node_type( node ) == WS_XML_NODE_TYPE_ELEMENT || node_type( node ) == WS_XML_NODE_TYPE_BOF);
}

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
