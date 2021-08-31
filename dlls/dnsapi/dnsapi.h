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

#include "wine/heap.h"

static inline char *strdup_a( const char *src )
{
    char *dst;
    if (!src) return NULL;
    dst = heap_alloc( (lstrlenA( src ) + 1) * sizeof(char) );
    if (dst) lstrcpyA( dst, src );
    return dst;
}

static inline char *strdup_u( const char *src )
{
    char *dst;
    if (!src) return NULL;
    dst = heap_alloc( (strlen( src ) + 1) * sizeof(char) );
    if (dst) strcpy( dst, src );
    return dst;
}

static inline WCHAR *strdup_w( const WCHAR *src )
{
    WCHAR *dst;
    if (!src) return NULL;
    dst = heap_alloc( (lstrlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) lstrcpyW( dst, src );
    return dst;
}

static inline WCHAR *strdup_aw( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
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
        if ((ret = heap_alloc( len * sizeof(WCHAR) )))
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
        if ((ret = heap_alloc( len )))
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
        if ((ret = heap_alloc( len )))
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
        heap_free( ret );
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
        heap_free( ret );
    }
    return dst;
}

extern const char *type_to_str( unsigned short ) DECLSPEC_HIDDEN;

struct resolv_funcs
{
    DNS_STATUS (CDECL *get_searchlist)( DNS_TXT_DATAW *list, DWORD *len );
    DNS_STATUS (CDECL *get_serverlist)( USHORT family, DNS_ADDR_ARRAY *addrs, DWORD *len );
    DNS_STATUS (CDECL *query)( const char *name, WORD type, DWORD options, DNS_RECORDA **result );
    DNS_STATUS (CDECL *set_serverlist)( const IP4_ARRAY *addrs );
};

extern const struct resolv_funcs *resolv_funcs;
