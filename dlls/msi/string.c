/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
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
    LPSTR str;
} msistring;

struct string_table
{
    UINT count;         /* the number of strings */
    UINT freeslot;
    msistring *strings; /* an array of strings (in the tree) */
};

static int msistring_makehash( char *str )
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

string_table *msi_init_stringtable( int entries )
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
    st->freeslot = 0;

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
    int i;

    for( i = st->freeslot; i < st->count; i++ )
        if( !st->strings[i].refcount )
            return i;
    for( i = 0; i < st->freeslot; i++ )
        if( !st->strings[i].refcount )
            return i;

    FIXME("dynamically resize\n");

    return -1;
}

static void st_mark_entry_used( string_table *st, int n )
{
    if( n >= st->count )
        return;
    st->freeslot = n + 1;
}

int msi_addstring( string_table *st, UINT string_no, CHAR *data, UINT len, UINT refcount )
{
    int n;

    TRACE("[%2d] = %s\n", string_no, debugstr_an(data,len) );

    n = st_find_free_entry( st );
    if( n < 0 )
        return -1;

    /* allocate a new string */
    if( len < 0 )
        len = strlen( data );
    st->strings[n].str = HeapAlloc( GetProcessHeap(), 0, len + 1 );
    if( !st->strings[n].str )
        return -1;
    memcpy( st->strings[n].str, data, len );
    st->strings[n].str[len] = 0;
    st->strings[n].refcount = 1;
    st->strings[n].hash = msistring_makehash( st->strings[n].str );

    st_mark_entry_used( st, n );

    return n;
}

UINT msi_id2string( string_table *st, UINT string_no, LPWSTR buffer, UINT *sz )
{
    UINT len;
    LPSTR str;

    TRACE("Finding string %d of %d\n", string_no, st->count);
    if( string_no >= st->count )
        return ERROR_FUNCTION_FAILED;

    if( !st->strings[string_no].refcount )
        return ERROR_FUNCTION_FAILED;

    str = st->strings[string_no].str;
    len = strlen( str );

    if( !buffer )
    {
        *sz = len;
        return ERROR_SUCCESS;
    }

    len = MultiByteToWideChar(CP_ACP,0,str,len,buffer,*sz-1); 
    buffer[len] = 0;
    *sz = len+1;

    return ERROR_SUCCESS;
}

UINT msi_string2id( string_table *st, LPCWSTR buffer, UINT *id )
{
    DWORD sz;
    UINT i, r = ERROR_INVALID_PARAMETER;
    LPSTR str;
    int hash;

    TRACE("Finding string %s in string table\n", debugstr_w(buffer) );

    sz = WideCharToMultiByte( CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL );
    if( sz <= 0 )
        return r;
    str = HeapAlloc( GetProcessHeap(), 0, sz );
    if( !str )
        return ERROR_NOT_ENOUGH_MEMORY;
    WideCharToMultiByte( CP_ACP, 0, buffer, -1, str, sz, NULL, NULL );

    hash = msistring_makehash( str );
    for( i=0; i<st->count; i++)
    {
        if( ( st->strings[i].hash == hash ) &&
            !strcmp( st->strings[i].str, str ) )
        {
            r = ERROR_SUCCESS;
            *id = i;
            break;
        }
    }

    if( str )
        HeapFree( GetProcessHeap(), 0, str );

    return r;
}
