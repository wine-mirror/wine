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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winhttp.h"

#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static void free_header( header_t *header )
{
    heap_free( header->field );
    heap_free( header->value );
    heap_free( header );
}

static BOOL valid_token_char( WCHAR c )
{
    if (c < 32 || c == 127) return FALSE;
    switch (c)
    {
    case '(': case ')':
    case '<': case '>':
    case '@': case ',':
    case ';': case ':':
    case '\\': case '\"':
    case '/': case '[':
    case ']': case '?':
    case '=': case '{':
    case '}': case ' ':
    case '\t':
        return FALSE;
    default:
        return TRUE;
    }
}

static header_t *parse_header( LPCWSTR string )
{
    const WCHAR *p, *q;
    header_t *header;
    int len;

    p = string;
    if (!(q = strchrW( p, ':' )))
    {
        WARN("no ':' in line %s\n", debugstr_w(string));
        return NULL;
    }
    if (q == string)
    {
        WARN("empty field name in line %s\n", debugstr_w(string));
        return NULL;
    }
    while (*p != ':')
    {
        if (!valid_token_char( *p ))
        {
            WARN("invalid character in field name %s\n", debugstr_w(string));
            return NULL;
        }
        p++;
    }
    len = q - string;
    if (!(header = heap_alloc_zero( sizeof(header_t) ))) return NULL;
    if (!(header->field = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        heap_free( header );
        return NULL;
    }
    memcpy( header->field, string, len * sizeof(WCHAR) );
    header->field[len] = 0;

    q++; /* skip past colon */
    while (*q == ' ') q++;
    if (!*q)
    {
        WARN("no value in line %s\n", debugstr_w(string));
        return header;
    }
    len = strlenW( q );
    if (!(header->value = heap_alloc( (len + 1) * sizeof(WCHAR) )))
    {
        free_header( header );
        return NULL;
    }
    memcpy( header->value, q, len * sizeof(WCHAR) );
    header->value[len] = 0;

    return header;
}

static int get_header_index( request_t *request, LPCWSTR field, int requested_index, BOOL request_only )
{
    int index;

    TRACE("%s\n", debugstr_w(field));

    for (index = 0; index < request->num_headers; index++)
    {
        if (strcmpiW( request->headers[index].field, field )) continue;
        if (request_only && !request->headers[index].is_request) continue;
        if (!request_only && request->headers[index].is_request) continue;

        if (!requested_index) break;
        requested_index--;
    }
    if (index >= request->num_headers) index = -1;
    TRACE("returning %d\n", index);
    return index;
}

static BOOL insert_header( request_t *request, header_t *header )
{
    DWORD count;
    header_t *hdrs;

    TRACE("inserting %s: %s\n", debugstr_w(header->field), debugstr_w(header->value));

    count = request->num_headers + 1;
    if (count > 1)
        hdrs = heap_realloc_zero( request->headers, sizeof(header_t) * count );
    else
        hdrs = heap_alloc_zero( sizeof(header_t) * count );

    if (hdrs)
    {
        request->headers = hdrs;
        request->headers[count - 1].field = strdupW( header->field );
        request->headers[count - 1].value = strdupW( header->value );
        request->headers[count - 1].is_request = header->is_request;
        request->num_headers++;
        return TRUE;
    }
    return FALSE;
}

static BOOL delete_header( request_t *request, DWORD index )
{
    if (!request->num_headers) return FALSE;
    if (index >= request->num_headers) return FALSE;
    request->num_headers--;

    heap_free( request->headers[index].field );
    heap_free( request->headers[index].value );

    memmove( &request->headers[index], &request->headers[index + 1], (request->num_headers - index) * sizeof(header_t) );
    memset( &request->headers[request->num_headers], 0, sizeof(header_t) );
    return TRUE;
}

static BOOL process_header( request_t *request, LPCWSTR field, LPCWSTR value, DWORD flags, BOOL request_only )
{
    int index;
    header_t *header;

    TRACE("%s: %s 0x%08x\n", debugstr_w(field), debugstr_w(value), flags);

    /* replace wins out over add */
    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE) flags &= ~WINHTTP_ADDREQ_FLAG_ADD;

    if (flags & WINHTTP_ADDREQ_FLAG_ADD) index = -1;
    else
        index = get_header_index( request, field, 0, request_only );

    if (index >= 0)
    {
        if (flags & WINHTTP_ADDREQ_FLAG_ADD_IF_NEW) return FALSE;
        header = &request->headers[index];
    }
    else if (value)
    {
        header_t hdr;

        hdr.field = (LPWSTR)field;
        hdr.value = (LPWSTR)value;
        hdr.is_request = request_only;

        return insert_header( request, &hdr );
    }
    /* no value to delete */
    else return TRUE;

    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE)
    {
        delete_header( request, index );
        if (value)
        {
            header_t hdr;

            hdr.field = (LPWSTR)field;
            hdr.value = (LPWSTR)value;
            hdr.is_request = request_only;

            return insert_header( request, &hdr );
        }
        return TRUE;
    }
    else if (flags & (WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON))
    {
        WCHAR sep, *tmp;
        int len, orig_len, value_len;

        orig_len = strlenW( header->value );
        value_len = strlenW( value );

        if (flags & WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA) sep = ',';
        else sep = ';';

        len = orig_len + value_len + 2;
        if ((tmp = heap_realloc( header->value, (len + 1) * sizeof(WCHAR) )))
        {
            header->value = tmp;

            header->value[orig_len] = sep;
            orig_len++;
            header->value[orig_len] = ' ';
            orig_len++;

            memcpy( &header->value[orig_len], value, value_len * sizeof(WCHAR) );
            header->value[len] = 0;
            return TRUE;
        }
    }
    return TRUE;
}

static BOOL add_request_headers( request_t *request, LPCWSTR headers, DWORD len, DWORD flags )
{
    BOOL ret = FALSE;
    WCHAR *buffer, *p, *q;
    header_t *header;

    if (len == ~0UL) len = strlenW( headers );
    if (!(buffer = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;
    strcpyW( buffer, headers );

    p = buffer;
    do
    {
        q = p;
        while (*q)
        {
            if (q[0] == '\r' && q[1] == '\n') break;
            q++;
        }
        if (!*p) break;
        if (*q == '\r')
        {
            *q = 0;
            q += 2; /* jump over \r\n */
        }
        if ((header = parse_header( p )))
        {
            ret = process_header( request, header->field, header->value, flags, TRUE );
            free_header( header );
        }
        p = q;
    } while (ret);

    heap_free( buffer );
    return ret;
}

/***********************************************************************
 *          WinHttpAddRequestHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpAddRequestHeaders( HINTERNET hrequest, LPCWSTR headers, DWORD len, DWORD flags )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, %s, 0x%x, 0x%08x\n", hrequest, debugstr_w(headers), len, flags);

    if (!headers)
    {
        set_last_error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(request = (request_t *)grab_object( hrequest )))
    {
        set_last_error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        set_last_error( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = add_request_headers( request, headers, len, flags );

    release_object( &request->hdr );
    return ret;
}
