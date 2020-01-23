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

extern _locale_t c_locale DECLSPEC_HIDDEN;

#define STREAM_BUFSIZE 4096

struct xmlbuf
{
    WS_HEAP                     *heap;
    WS_BYTES                     bytes;
    SIZE_T                       size;
    WS_XML_WRITER_ENCODING_TYPE  encoding;
    WS_CHARSET                   charset;
    const WS_XML_DICTIONARY     *dict_static;
    WS_XML_DICTIONARY           *dict;
};

void *ws_alloc( WS_HEAP *, SIZE_T ) DECLSPEC_HIDDEN;
void *ws_alloc_zero( WS_HEAP *, SIZE_T ) DECLSPEC_HIDDEN;
void *ws_realloc( WS_HEAP *, void *, SIZE_T, SIZE_T ) DECLSPEC_HIDDEN;
void *ws_realloc_zero( WS_HEAP *, void *, SIZE_T, SIZE_T ) DECLSPEC_HIDDEN;
void ws_free( WS_HEAP *, void *, SIZE_T ) DECLSPEC_HIDDEN;
struct xmlbuf *alloc_xmlbuf( WS_HEAP *, SIZE_T, WS_XML_WRITER_ENCODING_TYPE, WS_CHARSET,
                             const WS_XML_DICTIONARY *, WS_XML_DICTIONARY * ) DECLSPEC_HIDDEN;
void free_xmlbuf( struct xmlbuf * ) DECLSPEC_HIDDEN;

struct dictionary
{
    WS_XML_DICTIONARY  dict;
    ULONG             *sorted;
    ULONG              size;
    ULONG              current_sequence;
    ULONG             *sequence;
};
extern struct dictionary dict_builtin DECLSPEC_HIDDEN;
extern const struct dictionary dict_builtin_static DECLSPEC_HIDDEN;

int find_string( const struct dictionary *, const unsigned char *, ULONG, ULONG * ) DECLSPEC_HIDDEN;
HRESULT insert_string( struct dictionary *, unsigned char *, ULONG, int, ULONG * ) DECLSPEC_HIDDEN;
void clear_dict( struct dictionary * ) DECLSPEC_HIDDEN;
HRESULT writer_set_lookup( WS_XML_WRITER *, BOOL ) DECLSPEC_HIDDEN;
HRESULT writer_set_dict_callback( WS_XML_WRITER *, WS_DYNAMIC_STRING_CALLBACK, void * ) DECLSPEC_HIDDEN;

const char *debugstr_xmlstr( const WS_XML_STRING * ) DECLSPEC_HIDDEN;
WS_XML_STRING *alloc_xml_string( const unsigned char *, ULONG ) DECLSPEC_HIDDEN;
WS_XML_STRING *dup_xml_string( const WS_XML_STRING *, BOOL ) DECLSPEC_HIDDEN;
HRESULT add_xml_string( WS_XML_STRING * ) DECLSPEC_HIDDEN;
void free_xml_string( WS_XML_STRING * ) DECLSPEC_HIDDEN;
HRESULT append_attribute( WS_XML_ELEMENT_NODE *, WS_XML_ATTRIBUTE * ) DECLSPEC_HIDDEN;
void free_attribute( WS_XML_ATTRIBUTE * ) DECLSPEC_HIDDEN;
WS_TYPE map_value_type( WS_VALUE_TYPE ) DECLSPEC_HIDDEN;
ULONG get_type_size( WS_TYPE, const void * ) DECLSPEC_HIDDEN;
HRESULT read_header( WS_XML_READER *, const WS_XML_STRING *, const WS_XML_STRING *, WS_TYPE,
                     const void *, WS_READ_OPTION, WS_HEAP *, void *, ULONG ) DECLSPEC_HIDDEN;
HRESULT create_header_buffer( WS_XML_READER *, WS_HEAP *, WS_XML_BUFFER ** ) DECLSPEC_HIDDEN;
HRESULT text_to_text( const WS_XML_TEXT *, const WS_XML_TEXT *, ULONG *, WS_XML_TEXT ** ) DECLSPEC_HIDDEN;
HRESULT text_to_utf8text( const WS_XML_TEXT *, const WS_XML_UTF8_TEXT *, ULONG *, WS_XML_UTF8_TEXT ** ) DECLSPEC_HIDDEN;
HRESULT str_to_guid( const unsigned char *, ULONG, GUID * ) DECLSPEC_HIDDEN;
WS_XML_UTF8_TEXT *alloc_utf8_text( const BYTE *, ULONG ) DECLSPEC_HIDDEN;
WS_XML_UTF16_TEXT *alloc_utf16_text( const BYTE *, ULONG ) DECLSPEC_HIDDEN;
WS_XML_BASE64_TEXT *alloc_base64_text( const BYTE *, ULONG ) DECLSPEC_HIDDEN;
WS_XML_BOOL_TEXT *alloc_bool_text( BOOL ) DECLSPEC_HIDDEN;
WS_XML_INT32_TEXT *alloc_int32_text( INT32 ) DECLSPEC_HIDDEN;
WS_XML_INT64_TEXT *alloc_int64_text( INT64 ) DECLSPEC_HIDDEN;
WS_XML_UINT64_TEXT *alloc_uint64_text( UINT64 ) DECLSPEC_HIDDEN;
WS_XML_FLOAT_TEXT *alloc_float_text( float ) DECLSPEC_HIDDEN;
WS_XML_DOUBLE_TEXT *alloc_double_text( double ) DECLSPEC_HIDDEN;
WS_XML_GUID_TEXT *alloc_guid_text( const GUID * ) DECLSPEC_HIDDEN;
WS_XML_UNIQUE_ID_TEXT *alloc_unique_id_text( const GUID * ) DECLSPEC_HIDDEN;
WS_XML_DATETIME_TEXT *alloc_datetime_text( const WS_DATETIME * ) DECLSPEC_HIDDEN;

#define INVALID_PARAMETER_INDEX 0xffff
HRESULT get_param_desc( const WS_STRUCT_DESCRIPTION *, USHORT, const WS_FIELD_DESCRIPTION ** ) DECLSPEC_HIDDEN;
HRESULT write_input_params( WS_XML_WRITER *, const WS_ELEMENT_DESCRIPTION *,
                            const WS_PARAMETER_DESCRIPTION *, ULONG, const void ** ) DECLSPEC_HIDDEN;
HRESULT read_output_params( WS_XML_READER *, WS_HEAP *, const WS_ELEMENT_DESCRIPTION *,
                            const WS_PARAMETER_DESCRIPTION *, ULONG, const void ** ) DECLSPEC_HIDDEN;

enum node_flag
{
    NODE_FLAG_IGNORE_TRAILING_ELEMENT_CONTENT   = 0x1,
    NODE_FLAG_TEXT_WITH_IMPLICIT_END_ELEMENT    = 0x2,
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
HRESULT copy_node( WS_XML_READER *, WS_XML_WRITER_ENCODING_TYPE, struct node ** ) DECLSPEC_HIDDEN;

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
HRESULT message_get_id( WS_MESSAGE *, GUID * ) DECLSPEC_HIDDEN;
HRESULT message_set_request_id( WS_MESSAGE *, const GUID * ) DECLSPEC_HIDDEN;
void message_set_send_context( WS_MESSAGE *, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT * ) DECLSPEC_HIDDEN;
void message_set_receive_context( WS_MESSAGE *, const WS_PROXY_MESSAGE_CALLBACK_CONTEXT * ) DECLSPEC_HIDDEN;
void message_do_send_callback( WS_MESSAGE * ) DECLSPEC_HIDDEN;
void message_do_receive_callback( WS_MESSAGE * ) DECLSPEC_HIDDEN;
HRESULT message_insert_http_headers( WS_MESSAGE *, HINTERNET ) DECLSPEC_HIDDEN;
HRESULT message_map_http_response_headers( WS_MESSAGE *, HINTERNET, const WS_HTTP_MESSAGE_MAPPING * ) DECLSPEC_HIDDEN;

HRESULT channel_send_message( WS_CHANNEL *, WS_MESSAGE * ) DECLSPEC_HIDDEN;
HRESULT channel_receive_message( WS_CHANNEL *, WS_MESSAGE * ) DECLSPEC_HIDDEN;
HRESULT channel_get_reader( WS_CHANNEL *, WS_XML_READER ** ) DECLSPEC_HIDDEN;

HRESULT parse_url( const WS_STRING *, WS_URL_SCHEME_TYPE *, WCHAR **, USHORT * ) DECLSPEC_HIDDEN;

enum record_type
{
    /* 0x00 reserved */
    RECORD_ENDELEMENT                            = 0x01,
    RECORD_COMMENT                               = 0x02,
    RECORD_ARRAY                                 = 0x03,
    RECORD_SHORT_ATTRIBUTE                       = 0x04,
    RECORD_ATTRIBUTE                             = 0x05,
    RECORD_SHORT_DICTIONARY_ATTRIBUTE            = 0x06,
    RECORD_DICTIONARY_ATTRIBUTE                  = 0x07,
    RECORD_SHORT_XMLNS_ATTRIBUTE                 = 0x08,
    RECORD_XMLNS_ATTRIBUTE                       = 0x09,
    RECORD_SHORT_DICTIONARY_XMLNS_ATTRIBUTE      = 0x0a,
    RECORD_DICTIONARY_XMLNS_ATTRIBUTE            = 0x0b,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_A         = 0x0c,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_B         = 0x0d,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_C         = 0x0e,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_D         = 0x0f,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_E         = 0x10,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_F         = 0x11,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_G         = 0x12,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_H         = 0x13,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_I         = 0x14,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_J         = 0x15,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_K         = 0x16,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_L         = 0x17,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_M         = 0x18,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_N         = 0x19,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_O         = 0x1a,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_P         = 0x1b,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_Q         = 0x1c,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_R         = 0x1d,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_S         = 0x1e,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_T         = 0x1f,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_U         = 0x20,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_V         = 0x21,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_W         = 0x22,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_X         = 0x23,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_Y         = 0x24,
    RECORD_PREFIX_DICTIONARY_ATTRIBUTE_Z         = 0x25,
    RECORD_PREFIX_ATTRIBUTE_A                    = 0x26,
    RECORD_PREFIX_ATTRIBUTE_B                    = 0x27,
    RECORD_PREFIX_ATTRIBUTE_C                    = 0x28,
    RECORD_PREFIX_ATTRIBUTE_D                    = 0x29,
    RECORD_PREFIX_ATTRIBUTE_E                    = 0x2a,
    RECORD_PREFIX_ATTRIBUTE_F                    = 0x2b,
    RECORD_PREFIX_ATTRIBUTE_G                    = 0x2c,
    RECORD_PREFIX_ATTRIBUTE_H                    = 0x2d,
    RECORD_PREFIX_ATTRIBUTE_I                    = 0x2e,
    RECORD_PREFIX_ATTRIBUTE_J                    = 0x2f,
    RECORD_PREFIX_ATTRIBUTE_K                    = 0x30,
    RECORD_PREFIX_ATTRIBUTE_L                    = 0x31,
    RECORD_PREFIX_ATTRIBUTE_M                    = 0x32,
    RECORD_PREFIX_ATTRIBUTE_N                    = 0x33,
    RECORD_PREFIX_ATTRIBUTE_O                    = 0x34,
    RECORD_PREFIX_ATTRIBUTE_P                    = 0x35,
    RECORD_PREFIX_ATTRIBUTE_Q                    = 0x36,
    RECORD_PREFIX_ATTRIBUTE_R                    = 0x37,
    RECORD_PREFIX_ATTRIBUTE_S                    = 0x38,
    RECORD_PREFIX_ATTRIBUTE_T                    = 0x39,
    RECORD_PREFIX_ATTRIBUTE_U                    = 0x3a,
    RECORD_PREFIX_ATTRIBUTE_V                    = 0x3b,
    RECORD_PREFIX_ATTRIBUTE_W                    = 0x3c,
    RECORD_PREFIX_ATTRIBUTE_X                    = 0x3d,
    RECORD_PREFIX_ATTRIBUTE_Y                    = 0x3e,
    RECORD_PREFIX_ATTRIBUTE_Z                    = 0x3f,
    RECORD_SHORT_ELEMENT                         = 0x40,
    RECORD_ELEMENT                               = 0x41,
    RECORD_SHORT_DICTIONARY_ELEMENT              = 0x42,
    RECORD_DICTIONARY_ELEMENT                    = 0x43,
    RECORD_PREFIX_DICTIONARY_ELEMENT_A           = 0x44,
    RECORD_PREFIX_DICTIONARY_ELEMENT_B           = 0x45,
    RECORD_PREFIX_DICTIONARY_ELEMENT_C           = 0x46,
    RECORD_PREFIX_DICTIONARY_ELEMENT_D           = 0x47,
    RECORD_PREFIX_DICTIONARY_ELEMENT_E           = 0x48,
    RECORD_PREFIX_DICTIONARY_ELEMENT_F           = 0x49,
    RECORD_PREFIX_DICTIONARY_ELEMENT_G           = 0x4a,
    RECORD_PREFIX_DICTIONARY_ELEMENT_H           = 0x4b,
    RECORD_PREFIX_DICTIONARY_ELEMENT_I           = 0x4c,
    RECORD_PREFIX_DICTIONARY_ELEMENT_J           = 0x4d,
    RECORD_PREFIX_DICTIONARY_ELEMENT_K           = 0x4e,
    RECORD_PREFIX_DICTIONARY_ELEMENT_L           = 0x4f,
    RECORD_PREFIX_DICTIONARY_ELEMENT_M           = 0x50,
    RECORD_PREFIX_DICTIONARY_ELEMENT_N           = 0x51,
    RECORD_PREFIX_DICTIONARY_ELEMENT_O           = 0x52,
    RECORD_PREFIX_DICTIONARY_ELEMENT_P           = 0x53,
    RECORD_PREFIX_DICTIONARY_ELEMENT_Q           = 0x54,
    RECORD_PREFIX_DICTIONARY_ELEMENT_R           = 0x55,
    RECORD_PREFIX_DICTIONARY_ELEMENT_S           = 0x56,
    RECORD_PREFIX_DICTIONARY_ELEMENT_T           = 0x57,
    RECORD_PREFIX_DICTIONARY_ELEMENT_U           = 0x58,
    RECORD_PREFIX_DICTIONARY_ELEMENT_V           = 0x59,
    RECORD_PREFIX_DICTIONARY_ELEMENT_W           = 0x5a,
    RECORD_PREFIX_DICTIONARY_ELEMENT_X           = 0x5b,
    RECORD_PREFIX_DICTIONARY_ELEMENT_Y           = 0x5c,
    RECORD_PREFIX_DICTIONARY_ELEMENT_Z           = 0x5d,
    RECORD_PREFIX_ELEMENT_A                      = 0x5e,
    RECORD_PREFIX_ELEMENT_B                      = 0x5f,
    RECORD_PREFIX_ELEMENT_C                      = 0x60,
    RECORD_PREFIX_ELEMENT_D                      = 0x61,
    RECORD_PREFIX_ELEMENT_E                      = 0x62,
    RECORD_PREFIX_ELEMENT_F                      = 0x63,
    RECORD_PREFIX_ELEMENT_G                      = 0x64,
    RECORD_PREFIX_ELEMENT_H                      = 0x65,
    RECORD_PREFIX_ELEMENT_I                      = 0x66,
    RECORD_PREFIX_ELEMENT_J                      = 0x67,
    RECORD_PREFIX_ELEMENT_K                      = 0x68,
    RECORD_PREFIX_ELEMENT_L                      = 0x69,
    RECORD_PREFIX_ELEMENT_M                      = 0x6a,
    RECORD_PREFIX_ELEMENT_N                      = 0x6b,
    RECORD_PREFIX_ELEMENT_O                      = 0x6c,
    RECORD_PREFIX_ELEMENT_P                      = 0x6d,
    RECORD_PREFIX_ELEMENT_Q                      = 0x6e,
    RECORD_PREFIX_ELEMENT_R                      = 0x6f,
    RECORD_PREFIX_ELEMENT_S                      = 0x70,
    RECORD_PREFIX_ELEMENT_T                      = 0x71,
    RECORD_PREFIX_ELEMENT_U                      = 0x72,
    RECORD_PREFIX_ELEMENT_V                      = 0x73,
    RECORD_PREFIX_ELEMENT_W                      = 0x74,
    RECORD_PREFIX_ELEMENT_X                      = 0x75,
    RECORD_PREFIX_ELEMENT_Y                      = 0x76,
    RECORD_PREFIX_ELEMENT_Z                      = 0x77,
    /* 0x78 ... 0x7f reserved */
    RECORD_ZERO_TEXT                             = 0x80,
    RECORD_ZERO_TEXT_WITH_ENDELEMENT             = 0x81,
    RECORD_ONE_TEXT                              = 0x82,
    RECORD_ONE_TEXT_WITH_ENDELEMENT              = 0x83,
    RECORD_FALSE_TEXT                            = 0x84,
    RECORD_FALSE_TEXT_WITH_ENDELEMENT            = 0x85,
    RECORD_TRUE_TEXT                             = 0x86,
    RECORD_TRUE_TEXT_WITH_ENDELEMENT             = 0x87,
    RECORD_INT8_TEXT                             = 0x88,
    RECORD_INT8_TEXT_WITH_ENDELEMENT             = 0x89,
    RECORD_INT16_TEXT                            = 0x8a,
    RECORD_INT16_TEXT_WITH_ENDELEMENT            = 0x8b,
    RECORD_INT32_TEXT                            = 0x8c,
    RECORD_INT32_TEXT_WITH_ENDELEMENT            = 0x8d,
    RECORD_INT64_TEXT                            = 0x8e,
    RECORD_INT64_TEXT_WITH_ENDELEMENT            = 0x8f,
    RECORD_FLOAT_TEXT                            = 0x90,
    RECORD_FLOAT_TEXT_WITH_ENDELEMENT            = 0x91,
    RECORD_DOUBLE_TEXT                           = 0x92,
    RECORD_DOUBLE_TEXT_WITH_ENDELEMENT           = 0x93,
    RECORD_DECIMAL_TEXT                          = 0x94,
    RECORD_DECIMAL_TEXT_WITH_ENDELEMENT          = 0x95,
    RECORD_DATETIME_TEXT                         = 0x96,
    RECORD_DATETIME_TEXT_WITH_ENDELEMENT         = 0x97,
    RECORD_CHARS8_TEXT                           = 0x98,
    RECORD_CHARS8_TEXT_WITH_ENDELEMENT           = 0x99,
    RECORD_CHARS16_TEXT                          = 0x9a,
    RECORD_CHARS16_TEXT_WITH_ENDELEMENT          = 0x9b,
    RECORD_CHARS32_TEXT                          = 0x9c,
    RECORD_CHARS32_TEXT_WITH_ENDELEMENT          = 0x9d,
    RECORD_BYTES8_TEXT                           = 0x9e,
    RECORD_BYTES8_TEXT_WITH_ENDELEMENT           = 0x9f,
    RECORD_BYTES16_TEXT                          = 0xa0,
    RECORD_BYTES16_TEXT_WITH_ENDELEMENT          = 0xa1,
    RECORD_BYTES32_TEXT                          = 0xa2,
    RECORD_BYTES32_TEXT_WITH_ENDELEMENT          = 0xa3,
    RECORD_STARTLIST_TEXT                        = 0xa4,
    /* 0xa5 reserved */
    RECORD_ENDLIST_TEXT                          = 0xa6,
    /* 0xa7 reserved */
    RECORD_EMPTY_TEXT                            = 0xa8,
    RECORD_EMPTY_TEXT_WITH_ENDELEMENT            = 0xa9,
    RECORD_DICTIONARY_TEXT                       = 0xaa,
    RECORD_DICTIONARY_TEXT_WITH_ENDELEMENT       = 0xab,
    RECORD_UNIQUE_ID_TEXT                        = 0xac,
    RECORD_UNIQUE_ID_TEXT_WITH_ENDELEMENT        = 0xad,
    RECORD_TIMESPAN_TEXT                         = 0xae,
    RECORD_TIMESPAN_TEXT_WITH_ENDELEMENT         = 0xaf,
    RECORD_GUID_TEXT                             = 0xb0,
    RECORD_GUID_TEXT_WITH_ENDELEMENT             = 0xb1,
    RECORD_UINT64_TEXT                           = 0xb2,
    RECORD_UINT64_TEXT_WITH_ENDELEMENT           = 0xb3,
    RECORD_BOOL_TEXT                             = 0xb4,
    RECORD_BOOL_TEXT_WITH_ENDELEMENT             = 0xb5,
    RECORD_UNICODE_CHARS8_TEXT                   = 0xb6,
    RECORD_UNICODE_CHARS8_TEXT_WITH_ENDELEMENT   = 0xb7,
    RECORD_UNICODE_CHARS16_TEXT                  = 0xb8,
    RECORD_UNICODE_CHARS16_TEXT_WITH_ENDELEMENT  = 0xb9,
    RECORD_UNICODE_CHARS32_TEXT                  = 0xba,
    RECORD_UNICODE_CHARS32_TEXT_WITH_ENDELEMENT  = 0xbb,
    RECORD_QNAME_DICTIONARY_TEXT                 = 0xbc,
    RECORD_QNAME_DICTIONARY_TEXT_WITH_ENDELEMENT = 0xbd,
    /* 0xbe ... 0xff reserved */
};

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

static inline void* __WINE_ALLOC_SIZE(2) heap_realloc_zero(void *mem, size_t size)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
}
