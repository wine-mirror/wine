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

static WCHAR *build_request_string( request_t *request, LPCWSTR verb, LPCWSTR path, LPCWSTR version )
{
    static const WCHAR space[]   = {' ',0};
    static const WCHAR crlf[]    = {'\r','\n',0};
    static const WCHAR colon[]   = {':',' ',0};
    static const WCHAR twocrlf[] = {'\r','\n','\r','\n',0};

    WCHAR *ret;
    const WCHAR **headers, **p;
    unsigned int len, i = 0, j;

    /* allocate space for an array of all the string pointers to be added */
    len = request->num_headers * 4 + 7;
    if (!(headers = heap_alloc( len * sizeof(LPCWSTR) ))) return NULL;

    headers[i++] = verb;
    headers[i++] = space;
    headers[i++] = path;
    headers[i++] = space;
    headers[i++] = version;

    for (j = 0; j < request->num_headers; j++)
    {
        if (request->headers[j].is_request)
        {
            headers[i++] = crlf;
            headers[i++] = request->headers[j].field;
            headers[i++] = colon;
            headers[i++] = request->headers[j].value;

            TRACE("adding header %s (%s)\n", debugstr_w(request->headers[j].field),
                  debugstr_w(request->headers[j].value));
        }
    }
    headers[i++] = twocrlf;
    headers[i] = NULL;

    len = 0;
    for (p = headers; *p; p++) len += strlenW( *p );
    len++;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) )))
    {
        heap_free( headers );
        return NULL;
    }
    *ret = 0;
    for (p = headers; *p; p++) strcatW( ret, *p );

    heap_free( headers );
    return ret;
}

#define QUERY_MODIFIER_MASK (WINHTTP_QUERY_FLAG_REQUEST_HEADERS | WINHTTP_QUERY_FLAG_SYSTEMTIME | WINHTTP_QUERY_FLAG_NUMBER)

static BOOL query_headers( request_t *request, DWORD level, LPCWSTR name, LPVOID buffer, LPDWORD buflen, LPDWORD index )
{
    header_t *header = NULL;
    BOOL request_only, ret = FALSE;
    int requested_index, header_index = -1;
    DWORD attribute;

    request_only = level & WINHTTP_QUERY_FLAG_REQUEST_HEADERS;
    requested_index = index ? *index : 0;

    attribute = level & ~QUERY_MODIFIER_MASK;
    switch (attribute)
    {
    case WINHTTP_QUERY_CUSTOM:
    {
        header_index = get_header_index( request, name, requested_index, request_only );
        break;
    }
    case WINHTTP_QUERY_RAW_HEADERS_CRLF:
    {
        WCHAR *headers;
        DWORD len;

        if (request_only)
            headers = build_request_string( request, request->verb, request->path, request->version );
        else
            headers = request->raw_headers;

        len = strlenW( headers ) * sizeof(WCHAR);
        if (len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (buffer)
        {
            memcpy( buffer, headers, len + sizeof(WCHAR) );
            TRACE("returning data: %s\n", debugstr_wn(buffer, len / sizeof(WCHAR)));
            ret = TRUE;
        }
        *buflen = len;
        if (request_only) heap_free( headers );
        return ret;
    }
    default:
    {
        FIXME("attribute %u not implemented\n", attribute);
        return FALSE;
    }
    }

    if (header_index >= 0)
    {
        header = &request->headers[header_index];
    }
    if (!header || (request_only && !header->is_request))
    {
        set_last_error( ERROR_WINHTTP_HEADER_NOT_FOUND );
        return FALSE;
    }
    if (index) *index += 1;
    if (level & WINHTTP_QUERY_FLAG_NUMBER)
    {
        int *number = buffer;
        if (sizeof(int) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (number)
        {
            *number = atoiW( header->value );
            TRACE("returning number: %d\n", *number);
            ret = TRUE;
        }
        *buflen = sizeof(int);
    }
    else if (level & WINHTTP_QUERY_FLAG_SYSTEMTIME)
    {
        SYSTEMTIME *st = buffer;
        if (sizeof(SYSTEMTIME) > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
        }
        else if (st && (ret = WinHttpTimeToSystemTime( header->value, st )))
        {
            TRACE("returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
                  st->wYear, st->wMonth, st->wDay, st->wDayOfWeek,
                  st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
        }
        *buflen = sizeof(SYSTEMTIME);
    }
    else if (header->value)
    {
        WCHAR *string = buffer;
        DWORD len = (strlenW( header->value ) + 1) * sizeof(WCHAR);
        if (len > *buflen)
        {
            set_last_error( ERROR_INSUFFICIENT_BUFFER );
            *buflen = len;
            return FALSE;
        }
        else if (string)
        {
            strcpyW( string, header->value );
            TRACE("returning string: %s\n", debugstr_w(string));
            ret = TRUE;
        }
        *buflen = len - sizeof(WCHAR);
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpQueryHeaders( HINTERNET hrequest, DWORD level, LPCWSTR name, LPVOID buffer, LPDWORD buflen, LPDWORD index )
{
    BOOL ret;
    request_t *request;

    TRACE("%p, 0x%08x, %s, %p, %p, %p\n", hrequest, level, debugstr_w(name), buffer, buflen, index);

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

    ret = query_headers( request, level, name, buffer, buflen, index );

    release_object( &request->hdr );
    return ret;
}
