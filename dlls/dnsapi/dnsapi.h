/*
 * DNS support
 *
 * Copyright 2006 Hans Leidekker
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

#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "wine/unixlib.h"

static inline WCHAR *strdup_aw( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline WCHAR *strdup_uw( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline char *strdup_wa( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strdup_wu( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strdup_au( const char *src )
{
    char *dst = NULL;
    WCHAR *ret = strdup_aw( src );
    if (ret)
    {
        dst = strdup_wu( ret );
        free( ret );
    }
    return dst;
}

static inline char *strdup_ua( const char *src )
{
    char *dst = NULL;
    WCHAR *ret = strdup_uw( src );
    if (ret)
    {
        dst = strdup_wa( ret );
        free( ret );
    }
    return dst;
}

extern const char *debugstr_type( unsigned short );

struct get_searchlist_params
{
    WCHAR           *list;
    DWORD           *len;
};

struct get_serverlist_params
{
    USHORT           family;
    DNS_ADDR_ARRAY  *addrs;
    DWORD           *len;
};

struct query_params
{
    const char      *name;
    WORD             type;
    DWORD            options;
    void            *buf;
    DWORD           *len;
};

enum unix_funcs
{
    unix_get_searchlist,
    unix_get_serverlist,
    unix_set_serverlist,
    unix_query,
    unix_funcs_count
};

#define RESOLV_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )
