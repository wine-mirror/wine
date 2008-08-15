/*
 * Copyright 2008 Hans Leidekker for CodeWeavers
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

#ifndef _WINE_WINHTTP_PRIVATE_H_
#define _WINE_WINHTTP_PRIVATE_H_

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "wine/list.h"
#include "wine/unicode.h"

#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

typedef struct _object_header_t object_header_t;

typedef struct
{
    void (*destroy)( object_header_t * );
    BOOL (*query_option)( object_header_t *, DWORD, void *, DWORD *, BOOL );
    BOOL (*set_option)( object_header_t *, DWORD, void *, DWORD );
} object_vtbl_t;

struct _object_header_t
{
    DWORD type;
    HINTERNET handle;
    const object_vtbl_t *vtbl;
    DWORD flags;
    DWORD error;
    DWORD_PTR context;
    LONG refs;
    WINHTTP_STATUS_CALLBACK callback;
    DWORD notify_mask;
    struct list entry;
    struct list children;
};

typedef struct
{
    object_header_t hdr;
    LPWSTR agent;
    DWORD access;
    LPWSTR proxy_server;
    LPWSTR proxy_bypass;
    LPWSTR proxy_username;
    LPWSTR proxy_password;
} session_t;

typedef struct
{
    object_header_t hdr;
    session_t *session;
    LPWSTR hostname;    /* final destination of the request */
    LPWSTR servername;  /* name of the server we directly connect to */
    LPWSTR username;
    LPWSTR password;
    INTERNET_PORT hostport;
    INTERNET_PORT serverport;
    struct sockaddr_in sockaddr;
} connect_t;

typedef struct
{
    int socket;
    char *peek_msg;
    char *peek_msg_mem;
    size_t peek_len;
} netconn_t;

typedef struct
{
    LPWSTR field;
    LPWSTR value;
    WORD flags;
    WORD count;
} header_t;

typedef struct
{
    object_header_t hdr;
    connect_t *connect;
    LPWSTR verb;
    LPWSTR path;
    LPWSTR version;
    LPWSTR raw_headers;
    netconn_t netconn;
    LPWSTR status_text;
    DWORD content_length; /* total number of bytes to be read (per chunk) */
    DWORD content_read;   /* bytes read so far */
    header_t *headers;
    DWORD num_headers;
} request_t;

object_header_t *addref_object( object_header_t * );
object_header_t *grab_object( HINTERNET );
void release_object( object_header_t * );
HINTERNET alloc_handle( object_header_t * );
BOOL free_handle( HINTERNET );

static inline void *heap_alloc( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), 0, size );
}

static inline void *heap_alloc_zero( SIZE_T size )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
}

static inline void *heap_realloc_zero( LPVOID mem, SIZE_T size )
{
    return HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size );
}

static inline BOOL heap_free( LPVOID mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;

    if (!src) return NULL;
    dst = heap_alloc( (strlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) strcpyW( dst, src );
    return dst;
}

#endif /* _WINE_WINHTTP_PRIVATE_H_ */
