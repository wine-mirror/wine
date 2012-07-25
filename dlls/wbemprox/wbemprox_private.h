/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include "wine/list.h"
#include "wine/unicode.h"

IClientSecurity client_security;
struct list *table_list;

#define SIZEOF(array) (sizeof(array)/sizeof((array)[0]))

enum param_direction
{
    PARAM_OUT   = -1,
    PARAM_INOUT = 0,
    PARAM_IN    = 1
};

#define COL_TYPE_MASK    0x0000ffff
#define COL_FLAG_DYNAMIC 0x00010000
#define COL_FLAG_KEY     0x00020000
#define COL_FLAG_METHOD  0x00040000

typedef HRESULT (class_method)(IWbemClassObject *, IWbemClassObject **);

struct column
{
    const WCHAR *name;
    UINT type;
    VARTYPE vartype; /* 0 for default mapping */
};

#define TABLE_FLAG_DYNAMIC 0x00000001

struct table
{
    const WCHAR *name;
    UINT num_cols;
    const struct column *columns;
    UINT num_rows;
    BYTE *data;
    void (*fill)(struct table *);
    UINT flags;
    struct list entry;
};

struct property
{
    const WCHAR *name;
    const WCHAR *class;
    const struct property *next;
};

enum operator
{
    OP_EQ      = 1,
    OP_AND     = 2,
    OP_OR      = 3,
    OP_GT      = 4,
    OP_LT      = 5,
    OP_LE      = 6,
    OP_GE      = 7,
    OP_NE      = 8,
    OP_ISNULL  = 9,
    OP_NOTNULL = 10,
    OP_LIKE    = 11
};

struct expr;
struct complex_expr
{
    enum operator op;
    struct expr *left;
    struct expr *right;
};

enum expr_type
{
    EXPR_COMPLEX = 1,
    EXPR_UNARY   = 2,
    EXPR_PROPVAL = 3,
    EXPR_SVAL    = 4,
    EXPR_IVAL    = 5,
    EXPR_BVAL    = 6
};

struct expr
{
    enum expr_type type;
    union
    {
        struct complex_expr expr;
        const struct property *propval;
        const WCHAR *sval;
        int ival;
    } u;
};

struct view
{
    const struct property *proplist;
    struct table *table;
    const struct expr *cond;
    UINT *result;
    UINT  count;
};

struct query
{
    LONG refs;
    struct view *view;
    struct list mem;
};

void addref_query( struct query * ) DECLSPEC_HIDDEN;
void release_query( struct query *query ) DECLSPEC_HIDDEN;
HRESULT exec_query( const WCHAR *, IEnumWbemClassObject ** ) DECLSPEC_HIDDEN;
HRESULT parse_query( const WCHAR *, struct view **, struct list * ) DECLSPEC_HIDDEN;
HRESULT create_view( const struct property *, const WCHAR *, const struct expr *,
                     struct view ** ) DECLSPEC_HIDDEN;
void destroy_view( struct view * ) DECLSPEC_HIDDEN;
void init_table_list( void ) DECLSPEC_HIDDEN;
struct table *get_table( const WCHAR * ) DECLSPEC_HIDDEN;
struct table *create_table( const WCHAR *, UINT, const struct column *, UINT,
                            BYTE *, void (*)(struct table *)) DECLSPEC_HIDDEN;
BOOL add_table( struct table * ) DECLSPEC_HIDDEN;
void free_columns( struct column *, UINT ) DECLSPEC_HIDDEN;
void free_table( struct table * ) DECLSPEC_HIDDEN;
UINT get_type_size( CIMTYPE ) DECLSPEC_HIDDEN;
HRESULT get_column_index( const struct table *, const WCHAR *, UINT * ) DECLSPEC_HIDDEN;
HRESULT get_value( const struct table *, UINT, UINT, LONGLONG * ) DECLSPEC_HIDDEN;
BSTR get_value_bstr( const struct table *, UINT, UINT ) DECLSPEC_HIDDEN;
HRESULT set_value( const struct table *, UINT, UINT, LONGLONG, CIMTYPE ) DECLSPEC_HIDDEN;
HRESULT get_propval( const struct view *, UINT, const WCHAR *, VARIANT *,
                     CIMTYPE *, LONG * ) DECLSPEC_HIDDEN;
HRESULT put_propval( const struct view *, UINT, const WCHAR *, VARIANT *, CIMTYPE ) DECLSPEC_HIDDEN;
HRESULT get_properties( const struct view *, SAFEARRAY ** ) DECLSPEC_HIDDEN;
HRESULT get_object( const WCHAR *, IWbemClassObject ** ) DECLSPEC_HIDDEN;

HRESULT WbemLocator_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT WbemServices_create(IUnknown *, const WCHAR *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT WbemClassObject_create(IUnknown *, IEnumWbemClassObject *, UINT, LPVOID *) DECLSPEC_HIDDEN;
HRESULT EnumWbemClassObject_create(IUnknown *, struct query *, LPVOID *) DECLSPEC_HIDDEN;

static void *heap_alloc( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static void *heap_realloc( void *mem, size_t len ) __WINE_ALLOC_SIZE(2);
static inline void *heap_realloc( void *mem, size_t len )
{
    return HeapReAlloc( GetProcessHeap(), 0, mem, len );
}

static void *heap_alloc_zero( size_t len ) __WINE_ALLOC_SIZE(1);
static inline void *heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *heap_strdupW( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    if ((dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) ))) strcpyW( dst, src );
    return dst;
}
