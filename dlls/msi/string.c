/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2004, Mike McCormack for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct _msistring
{
    UINT hash;
    UINT refcount;
    LPWSTR str;
} msistring;

struct string_table
{
    UINT count;         /* the number of strings */
    UINT freeslot;
    UINT codepage;
    msistring *strings; /* an array of strings (in the tree) */
};

static int msistring_makehash( const WCHAR *str )
{
    int hash = 0;

    while( *str )
    {
        hash ^= *str++;
        hash *= 53;
        hash = (hash<<5) || (hash>>27);
    }
    return hash;
}

string_table *msi_init_stringtable( int entries, UINT codepage )
{
    string_table *st;

    st = HeapAlloc( GetProcessHeap(), 0, sizeof (string_table) );
    if( !st )
        return NULL;    
    st->strings = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              sizeof (msistring) * entries );
    if( !st )
    {
        HeapFree( GetProcessHeap(), 0, st );
        return NULL;    
    }
    st->count = entries;
    st->freeslot = 1;
    st->codepage = codepage;

    return st;
}

VOID msi_destroy_stringtable( string_table *st )
{
    UINT i;

    for( i=0; i<st->count; i++ )
    {
        if( st->strings[i].refcount )
            HeapFree( GetProcessHeap(), 0, st->strings[i].str );
    }
    HeapFree( GetProcessHeap(), 0, st->strings );
    HeapFree( GetProcessHeap(), 0, st );
}

static int st_find_free_entry( string_table *st )
{
    int i, sz;
    msistring *p;

    TRACE("%p\n", st);

    for( i = st->freeslot; i < st->count; i++ )
        if( !st->strings[i].refcount )
            return i;
    for( i = 1; i < st->freeslot; i++ )
        if( !st->strings[i].refcount )
            return i;

    /* dynamically resize */
    sz = st->count + 1 + st->count/2;
    p = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                     st->strings, sz*sizeof(msistring) );
    if( !p )
        return -1;
    st->strings = p;
    st->freeslot = st->count;
    st->count = sz;
    if( st->strings[st->freeslot].refcount )
        ERR("oops. expected freeslot to be free...\n");
    return st->freeslot;
}

static void st_mark_entry_used( string_table *st, int n )
{
    if( n >= st->count )
        return;
    st->freeslot = n + 1;
}

int msi_addstring( string_table *st, UINT n, const CHAR *data, int len, UINT refcount )
{
    int sz;

    if( !data[0] )
        return 0;
    if( n > 0 )
    {
        if( st->strings[n].refcount )
            return -1;
    }
    else
    {
        if( ERROR_SUCCESS == msi_string2idA( st, data, &n ) )
        {
            st->strings[n].refcount++;
            return n;
        }
        n = st_find_free_entry( st );
        if( n < 0 )
            return -1;
    }

    /* allocate a new string */
    if( len < 0 )
        len = strlen(data);
    sz = MultiByteToWideChar( st->codepage, 0, data, len, NULL, 0 );
    st->strings[n].str = HeapAlloc( GetProcessHeap(), 0, (sz+1)*sizeof(WCHAR) );
    if( !st->strings[n].str )
        return -1;
    MultiByteToWideChar( st->codepage, 0, data, len, st->strings[n].str, sz );
    st->strings[n].str[sz] = 0;
    st->strings[n].refcount = 1;
    st->strings[n].hash = msistring_makehash( st->strings[n].str );

    st_mark_entry_used( st, n );

    return n;
}

int msi_addstringW( string_table *st, UINT n, const WCHAR *data, int len, UINT refcount )
{
    /* TRACE("[%2d] = %s\n", string_no, debugstr_an(data,len) ); */

    if( !data[0] )
        return 0;
    if( n > 0 )
    {
        if( st->strings[n].refcount )
            return -1;
    }
    else
    {
        if( ERROR_SUCCESS == msi_string2idW( st, data, &n ) )
        {
            st->strings[n].refcount++;
            return n;
        }
        n = st_find_free_entry( st );
        if( n < 0 )
            return -1;
    }

    /* allocate a new string */
    if(len<0)
        len = strlenW(data);
    TRACE("%s, n = %d len = %d\n", debugstr_w(data), n, len );

    st->strings[n].str = HeapAlloc( GetProcessHeap(), 0, (len+1)*sizeof(WCHAR) );
    if( !st->strings[n].str )
        return -1;
    TRACE("%d\n",__LINE__);
    memcpy( st->strings[n].str, data, len*sizeof(WCHAR) );
    st->strings[n].str[len] = 0;
    st->strings[n].refcount = 1;
    st->strings[n].hash = msistring_makehash( st->strings[n].str );

    st_mark_entry_used( st, n );

    return n;
}

/* find the string identified by an id - return null if there's none */
const WCHAR *msi_string_lookup_id( string_table *st, UINT id )
{
    static const WCHAR zero[] = { 0 };
    if( id == 0 )
        return zero;

    if( id >= st->count )
        return NULL;

    if( id && !st->strings[id].refcount )
        return NULL;

    return st->strings[id].str;
}

/*
 *  msi_id2stringW
 *
 *  [in] st         - pointer to the string table
 *  [in] id  - id of the string to retrieve
 *  [out] buffer    - destination of the string
 *  [in/out] sz     - number of bytes available in the buffer on input
 *                    number of bytes used on output
 *
 *   The size includes the terminating nul character.  Short buffers
 *  will be filled, but not nul terminated.
 */
UINT msi_id2stringW( string_table *st, UINT id, LPWSTR buffer, UINT *sz )
{
    UINT len;
    const WCHAR *str;

    TRACE("Finding string %d of %d\n", id, st->count);

    str = msi_string_lookup_id( st, id );
    if( !str )
        return ERROR_FUNCTION_FAILED;

    len = strlenW( str ) + 1;

    if( !buffer )
    {
        *sz = len;
        return ERROR_SUCCESS;
    }

    if( *sz < len )
        *sz = len;
    memcpy( buffer, str, (*sz)*sizeof(WCHAR) ); 
    *sz = len;

    return ERROR_SUCCESS;
}

/*
 *  msi_id2stringA
 *
 *  [in] st         - pointer to the string table
 *  [in] id         - id of the string to retrieve
 *  [out] buffer    - destination of the UTF8 string
 *  [in/out] sz     - number of bytes available in the buffer on input
 *                    number of bytes used on output
 *
 *   The size includes the terminating nul character.  Short buffers
 *  will be filled, but not nul terminated.
 */
UINT msi_id2stringA( string_table *st, UINT id, LPSTR buffer, UINT *sz )
{
    UINT len;
    const WCHAR *str;

    TRACE("Finding string %d of %d\n", id, st->count);

    str = msi_string_lookup_id( st, id );
    if( !str )
        return ERROR_FUNCTION_FAILED;

    len = WideCharToMultiByte( st->codepage, 0, str, -1, NULL, 0, NULL, NULL );

    if( !buffer )
    {
        *sz = len;
        return ERROR_SUCCESS;
    }

    *sz = WideCharToMultiByte( st->codepage, 0, str, -1, buffer, *sz, NULL, NULL );

    return ERROR_SUCCESS;
}

/*
 *  msi_string2idW
 *
 *  [in] st         - pointer to the string table
 *  [in] str        - string to find in the string table
 *  [out] id        - id of the string, if found
 */
UINT msi_string2idW( string_table *st, LPCWSTR str, UINT *id )
{
    int hash;
    UINT i, r = ERROR_INVALID_PARAMETER;

    hash = msistring_makehash( str );
    for( i=0; i<st->count; i++ )
    {
        if( ( st->strings[i].hash == hash ) &&
            !strcmpW( st->strings[i].str, str ) )
        {
            r = ERROR_SUCCESS;
            *id = i;
            break;
        }
    }

    return r;
}

UINT msi_string2idA( string_table *st, LPCSTR buffer, UINT *id )
{
    DWORD sz;
    UINT r = ERROR_INVALID_PARAMETER;
    LPWSTR str;

    TRACE("Finding string %s in string table\n", debugstr_a(buffer) );

    if( buffer[0] == 0 )
    {
        *id = 0;
        return ERROR_SUCCESS;
    }

    sz = MultiByteToWideChar( st->codepage, 0, buffer, -1, NULL, 0 );
    if( sz <= 0 )
        return r;
    str = HeapAlloc( GetProcessHeap(), 0, sz*sizeof(WCHAR) );
    if( !str )
        return ERROR_NOT_ENOUGH_MEMORY;
    MultiByteToWideChar( st->codepage, 0, buffer, -1, str, sz );

    r = msi_string2idW( st, str, id );
    if( str )
        HeapFree( GetProcessHeap(), 0, str );

    return r;
}

UINT msi_strcmp( string_table *st, UINT lval, UINT rval, UINT *res )
{
    const WCHAR *l_str, *r_str;

    l_str = msi_string_lookup_id( st, lval );
    if( !l_str )
        return ERROR_INVALID_PARAMETER;
    
    r_str = msi_string_lookup_id( st, rval );
    if( !r_str )
        return ERROR_INVALID_PARAMETER;

    /* does this do the right thing for all UTF-8 strings? */
    *res = strcmpW( l_str, r_str );

    return ERROR_SUCCESS;
}

UINT msi_string_count( string_table *st )
{
    return st->count;
}

UINT msi_id_refcount( string_table *st, UINT i )
{
    if( i >= st->count )
        return 0;
    return st->strings[i].refcount;
}

UINT msi_string_totalsize( string_table *st )
{
    UINT size = 0, i, len;

    for( i=0; i<st->count; i++)
    {
        if( st->strings[i].str )
        {
            len = WideCharToMultiByte( st->codepage, 0,
                     st->strings[i].str, -1, NULL, 0, NULL, NULL);
            if( len )
                len--;
            size += len;
        }
    }
    return size;
}
