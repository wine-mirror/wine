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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "wine/heap.h"

static inline LPSTR dns_strdup_a( LPCSTR src )
{
    LPSTR dst;

    if (!src) return NULL;
    dst = heap_alloc( (lstrlenA( src ) + 1) * sizeof(char) );
    if (dst) lstrcpyA( dst, src );
    return dst;
}

static inline char *dns_strdup_u( const char *src )
{
    char *dst;

    if (!src) return NULL;
    dst = heap_alloc( (strlen( src ) + 1) * sizeof(char) );
    if (dst) strcpy( dst, src );
    return dst;
}

static inline LPWSTR dns_strdup_w( LPCWSTR src )
{
    LPWSTR dst;

    if (!src) return NULL;
    dst = heap_alloc( (lstrlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) lstrcpyW( dst, src );
    return dst;
}

static inline LPWSTR dns_strdup_aw( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPWSTR dns_strdup_uw( const char *str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPSTR dns_strdup_wa( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = heap_alloc( len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *dns_strdup_wu( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = heap_alloc( len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *dns_strdup_au( LPCSTR src )
{
    char *dst = NULL;
    LPWSTR ret = dns_strdup_aw( src );

    if (ret)
    {
        dst = dns_strdup_wu( ret );
        heap_free( ret );
    }
    return dst;
}

static inline LPSTR dns_strdup_ua( const char *src )
{
    LPSTR dst = NULL;
    LPWSTR ret = dns_strdup_uw( src );

    if (ret)
    {
        dst = dns_strdup_wa( ret );
        heap_free( ret );
    }
    return dst;
}

const char *dns_type_to_str( unsigned short ) DECLSPEC_HIDDEN;

#ifdef HAVE_RESOLV
int dns_ns_initparse( const u_char *, int, ns_msg * ) DECLSPEC_HIDDEN;
int dns_ns_parserr( ns_msg *, ns_sect, int, ns_rr * ) DECLSPEC_HIDDEN;
int dns_ns_name_skip( const u_char **, const u_char * ) DECLSPEC_HIDDEN;
int dns_ns_name_uncompress( const u_char *, const u_char *, const u_char *, char *, size_t ) DECLSPEC_HIDDEN;
#endif
