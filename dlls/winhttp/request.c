/*
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2006 Rob Shearman for CodeWeavers
 * Copyright 2008, 2011 Hans Leidekker for CodeWeavers
 * Copyright 2009 Juan Lang
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

#include <assert.h>
#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "ws2tcpip.h"
#include "ole2.h"
#include "initguid.h"
#include "httprequest.h"
#include "httprequestid.h"
#include "schannel.h"
#include "winhttp.h"
#include "ntsecapi.h"
#include "winternl.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

#define DEFAULT_KEEP_ALIVE_TIMEOUT 30000

static const WCHAR *attribute_table[] =
{
    L"Mime-Version",                /* WINHTTP_QUERY_MIME_VERSION               = 0  */
    L"Content-Type"  ,              /* WINHTTP_QUERY_CONTENT_TYPE               = 1  */
    L"Content-Transfer-Encoding",   /* WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING  = 2  */
    L"Content-ID",                  /* WINHTTP_QUERY_CONTENT_ID                 = 3  */
    NULL,                           /* WINHTTP_QUERY_CONTENT_DESCRIPTION        = 4  */
    L"Content-Length",              /* WINHTTP_QUERY_CONTENT_LENGTH             = 5  */
    L"Content-Language",            /* WINHTTP_QUERY_CONTENT_LANGUAGE           = 6  */
    L"Allow",                       /* WINHTTP_QUERY_ALLOW                      = 7  */
    L"Public",                      /* WINHTTP_QUERY_PUBLIC                     = 8  */
    L"Date",                        /* WINHTTP_QUERY_DATE                       = 9  */
    L"Expires",                     /* WINHTTP_QUERY_EXPIRES                    = 10 */
    L"Last-Modified",               /* WINHTTP_QUERY_LAST_MODIFIEDcw            = 11 */
    NULL,                           /* WINHTTP_QUERY_MESSAGE_ID                 = 12 */
    L"URI",                         /* WINHTTP_QUERY_URI                        = 13 */
    L"From",                        /* WINHTTP_QUERY_DERIVED_FROM               = 14 */
    NULL,                           /* WINHTTP_QUERY_COST                       = 15 */
    NULL,                           /* WINHTTP_QUERY_LINK                       = 16 */
    L"Pragma",                      /* WINHTTP_QUERY_PRAGMA                     = 17 */
    NULL,                           /* WINHTTP_QUERY_VERSION                    = 18 */
    L"Status",                      /* WINHTTP_QUERY_STATUS_CODE                = 19 */
    NULL,                           /* WINHTTP_QUERY_STATUS_TEXT                = 20 */
    NULL,                           /* WINHTTP_QUERY_RAW_HEADERS                = 21 */
    NULL,                           /* WINHTTP_QUERY_RAW_HEADERS_CRLF           = 22 */
    L"Connection",                  /* WINHTTP_QUERY_CONNECTION                 = 23 */
    L"Accept",                      /* WINHTTP_QUERY_ACCEPT                     = 24 */
    L"Accept-Charset",              /* WINHTTP_QUERY_ACCEPT_CHARSET             = 25 */
    L"Accept-Encoding",             /* WINHTTP_QUERY_ACCEPT_ENCODING            = 26 */
    L"Accept-Language",             /* WINHTTP_QUERY_ACCEPT_LANGUAGE            = 27 */
    L"Authorization",               /* WINHTTP_QUERY_AUTHORIZATION              = 28 */
    L"Content-Encoding",            /* WINHTTP_QUERY_CONTENT_ENCODING           = 29 */
    NULL,                           /* WINHTTP_QUERY_FORWARDED                  = 30 */
    NULL,                           /* WINHTTP_QUERY_FROM                       = 31 */
    L"If-Modified-Since",           /* WINHTTP_QUERY_IF_MODIFIED_SINCE          = 32 */
    L"Location",                    /* WINHTTP_QUERY_LOCATION                   = 33 */
    NULL,                           /* WINHTTP_QUERY_ORIG_URI                   = 34 */
    L"Referer",                     /* WINHTTP_QUERY_REFERER                    = 35 */
    L"Retry-After",                 /* WINHTTP_QUERY_RETRY_AFTER                = 36 */
    L"Server",                      /* WINHTTP_QUERY_SERVER                     = 37 */
    NULL,                           /* WINHTTP_TITLE                            = 38 */
    L"User-Agent",                  /* WINHTTP_QUERY_USER_AGENT                 = 39 */
    L"WWW-Authenticate",            /* WINHTTP_QUERY_WWW_AUTHENTICATE           = 40 */
    L"Proxy-Authenticate",          /* WINHTTP_QUERY_PROXY_AUTHENTICATE         = 41 */
    L"Accept-Ranges",               /* WINHTTP_QUERY_ACCEPT_RANGES              = 42 */
    L"Set-Cookie",                  /* WINHTTP_QUERY_SET_COOKIE                 = 43 */
    L"Cookie",                      /* WINHTTP_QUERY_COOKIE                     = 44 */
    NULL,                           /* WINHTTP_QUERY_REQUEST_METHOD             = 45 */
    NULL,                           /* WINHTTP_QUERY_REFRESH                    = 46 */
    NULL,                           /* WINHTTP_QUERY_CONTENT_DISPOSITION        = 47 */
    L"Age",                         /* WINHTTP_QUERY_AGE                        = 48 */
    L"Cache-Control",               /* WINHTTP_QUERY_CACHE_CONTROL              = 49 */
    L"Content-Base",                /* WINHTTP_QUERY_CONTENT_BASE               = 50 */
    L"Content-Location",            /* WINHTTP_QUERY_CONTENT_LOCATION           = 51 */
    L"Content-MD5",                 /* WINHTTP_QUERY_CONTENT_MD5                = 52 */
    L"Content-Range",               /* WINHTTP_QUERY_CONTENT_RANGE              = 53 */
    L"ETag",                        /* WINHTTP_QUERY_ETAG                       = 54 */
    L"Host",                        /* WINHTTP_QUERY_HOST                       = 55 */
    L"If-Match",                    /* WINHTTP_QUERY_IF_MATCH                   = 56 */
    L"If-None-Match",               /* WINHTTP_QUERY_IF_NONE_MATCH              = 57 */
    L"If-Range",                    /* WINHTTP_QUERY_IF_RANGE                   = 58 */
    L"If-Unmodified-Since",         /* WINHTTP_QUERY_IF_UNMODIFIED_SINCE        = 59 */
    L"Max-Forwards",                /* WINHTTP_QUERY_MAX_FORWARDS               = 60 */
    L"Proxy-Authorization",         /* WINHTTP_QUERY_PROXY_AUTHORIZATION        = 61 */
    L"Range",                       /* WINHTTP_QUERY_RANGE                      = 62 */
    L"Transfer-Encoding",           /* WINHTTP_QUERY_TRANSFER_ENCODING          = 63 */
    L"Upgrade",                     /* WINHTTP_QUERY_UPGRADE                    = 64 */
    L"Vary",                        /* WINHTTP_QUERY_VARY                       = 65 */
    L"Via",                         /* WINHTTP_QUERY_VIA                        = 66 */
    L"Warning",                     /* WINHTTP_QUERY_WARNING                    = 67 */
    L"Expect",                      /* WINHTTP_QUERY_EXPECT                     = 68 */
    L"Proxy-Connection",            /* WINHTTP_QUERY_PROXY_CONNECTION           = 69 */
    L"Unless-Modified-Since",       /* WINHTTP_QUERY_UNLESS_MODIFIED_SINCE      = 70 */
    NULL,                           /* WINHTTP_QUERY_PROXY_SUPPORT              = 75 */
    NULL,                           /* WINHTTP_QUERY_AUTHENTICATION_INFO        = 76 */
    NULL,                           /* WINHTTP_QUERY_PASSPORT_URLS              = 77 */
    NULL                            /* WINHTTP_QUERY_PASSPORT_CONFIG            = 78 */
};

static DWORD start_queue( struct queue *queue )
{
    if (queue->pool) return ERROR_SUCCESS;

    if (!(queue->pool = CreateThreadpool( NULL ))) return GetLastError();
    SetThreadpoolThreadMinimum( queue->pool, 1 );
    SetThreadpoolThreadMaximum( queue->pool, 1 );

    memset( &queue->env, 0, sizeof(queue->env) );
    queue->env.Version = 1;
    queue->env.Pool = queue->pool;

    TRACE("started %p\n", queue);
    return ERROR_SUCCESS;
}

void stop_queue( struct queue *queue )
{
    if (!queue->pool) return;
    CloseThreadpool( queue->pool );
    queue->pool = NULL;
    TRACE("stopped %p\n", queue);
}

static DWORD queue_task( struct queue *queue, PTP_WORK_CALLBACK task, void *ctx )
{
    TP_WORK *work;
    DWORD ret;

    if ((ret = start_queue( queue ))) return ret;

    if (!(work = CreateThreadpoolWork( task, ctx, &queue->env ))) return GetLastError();
    TRACE("queueing %p in %p\n", work, queue);
    SubmitThreadpoolWork( work );
    CloseThreadpoolWork( work );

    return ERROR_SUCCESS;
}

static void free_header( struct header *header )
{
    free( header->field );
    free( header->value );
    free( header );
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

static struct header *parse_header( const WCHAR *string )
{
    const WCHAR *p, *q;
    struct header *header;
    int len;

    p = string;
    if (!(q = wcschr( p, ':' )))
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
    if (!(header = calloc( 1, sizeof(*header) ))) return NULL;
    if (!(header->field = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        free( header );
        return NULL;
    }
    memcpy( header->field, string, len * sizeof(WCHAR) );
    header->field[len] = 0;

    q++; /* skip past colon */
    while (*q == ' ') q++;
    len = lstrlenW( q );

    if (!(header->value = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        free_header( header );
        return NULL;
    }
    memcpy( header->value, q, len * sizeof(WCHAR) );
    header->value[len] = 0;

    return header;
}

static int get_header_index( struct request *request, const WCHAR *field, int requested_index, BOOL request_only )
{
    int index;

    TRACE("%s\n", debugstr_w(field));

    for (index = 0; index < request->num_headers; index++)
    {
        if (wcsicmp( request->headers[index].field, field )) continue;
        if (request_only && !request->headers[index].is_request) continue;
        if (!request_only && request->headers[index].is_request) continue;

        if (!requested_index) break;
        requested_index--;
    }
    if (index >= request->num_headers) index = -1;
    TRACE("returning %d\n", index);
    return index;
}

static DWORD insert_header( struct request *request, struct header *header )
{
    DWORD count = request->num_headers + 1;
    struct header *hdrs;

    if (request->headers)
    {
        if ((hdrs = realloc( request->headers, sizeof(*header) * count )))
            memset( &hdrs[count - 1], 0, sizeof(*header) );
    }
    else hdrs = calloc( 1, sizeof(*header) );
    if (!hdrs) return ERROR_OUTOFMEMORY;

    request->headers = hdrs;
    request->headers[count - 1].field = strdupW( header->field );
    request->headers[count - 1].value = strdupW( header->value );
    request->headers[count - 1].is_request = header->is_request;
    request->num_headers = count;
    return ERROR_SUCCESS;
}

static void delete_header( struct request *request, DWORD index )
{
    if (!request->num_headers || index >= request->num_headers) return;
    request->num_headers--;

    free( request->headers[index].field );
    free( request->headers[index].value );

    memmove( &request->headers[index], &request->headers[index + 1],
             (request->num_headers - index) * sizeof(struct header) );
    memset( &request->headers[request->num_headers], 0, sizeof(struct header) );
}

DWORD process_header( struct request *request, const WCHAR *field, const WCHAR *value, DWORD flags, BOOL request_only )
{
    int index;
    struct header hdr;

    TRACE("%s: %s 0x%08x\n", debugstr_w(field), debugstr_w(value), flags);

    if ((index = get_header_index( request, field, 0, request_only )) >= 0)
    {
        if (flags & WINHTTP_ADDREQ_FLAG_ADD_IF_NEW) return ERROR_WINHTTP_HEADER_ALREADY_EXISTS;
    }

    if (flags & WINHTTP_ADDREQ_FLAG_REPLACE)
    {
        if (index >= 0)
        {
            delete_header( request, index );
            if (!value || !value[0]) return ERROR_SUCCESS;
        }
        else if (!(flags & WINHTTP_ADDREQ_FLAG_ADD)) return ERROR_WINHTTP_HEADER_NOT_FOUND;

        hdr.field = (LPWSTR)field;
        hdr.value = (LPWSTR)value;
        hdr.is_request = request_only;
        return insert_header( request, &hdr );
    }
    else if (value)
    {

        if ((flags & (WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA | WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON)) &&
            index >= 0)
        {
            WCHAR *tmp;
            int len, len_orig, len_value;
            struct header *header = &request->headers[index];

            len_orig = lstrlenW( header->value );
            len_value = lstrlenW( value );

            len = len_orig + len_value + 2;
            if (!(tmp = realloc( header->value, (len + 1) * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
            header->value = tmp;
            header->value[len_orig++] = (flags & WINHTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA) ? ',' : ';';
            header->value[len_orig++] = ' ';

            memcpy( &header->value[len_orig], value, len_value * sizeof(WCHAR) );
            header->value[len] = 0;
            return ERROR_SUCCESS;
        }
        else
        {
            hdr.field = (LPWSTR)field;
            hdr.value = (LPWSTR)value;
            hdr.is_request = request_only;
            return insert_header( request, &hdr );
        }
    }

    return ERROR_SUCCESS;
}

DWORD add_request_headers( struct request *request, const WCHAR *headers, DWORD len, DWORD flags )
{
    DWORD ret = ERROR_WINHTTP_INVALID_HEADER;
    WCHAR *buffer, *p, *q;
    struct header *header;

    if (len == ~0u) len = lstrlenW( headers );
    if (!len) return ERROR_SUCCESS;
    if (!(buffer = malloc( (len + 1) * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    memcpy( buffer, headers, len * sizeof(WCHAR) );
    buffer[len] = 0;

    p = buffer;
    do
    {
        q = p;
        while (*q)
        {
            if (q[0] == '\n' && q[1] == '\r')
            {
                q[0] = '\r';
                q[1] = '\n';
            }
            if (q[0] == '\r') break;
            q++;
        }
        if (!*p) break;
        if (*q == '\r')
        {
            *q = 0;
            if (q[1] == '\n')
                q += 2; /* jump over \r\n */
            else
                q++; /* jump over \r */
        }
        if ((header = parse_header( p )))
        {
            ret = process_header( request, header->field, header->value, flags, TRUE );
            free_header( header );
        }
        p = q;
    } while (!ret);

    free( buffer );
    return ret;
}

/***********************************************************************
 *          WinHttpAddRequestHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpAddRequestHeaders( HINTERNET hrequest, LPCWSTR headers, DWORD len, DWORD flags )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %s, %u, 0x%08x\n", hrequest, debugstr_wn(headers, len), len, flags);

    if (!headers || !len)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = add_request_headers( request, headers, len, flags );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static WCHAR *build_absolute_request_path( struct request *request, const WCHAR **path )
{
    const WCHAR *scheme;
    WCHAR *ret;
    int len, offset;

    scheme = (request->netconn ? request->netconn->secure : (request->hdr.flags & WINHTTP_FLAG_SECURE)) ? L"https" : L"http";

    len = lstrlenW( scheme ) + lstrlenW( request->connect->hostname ) + 4; /* '://' + nul */
    if (request->connect->hostport) len += 6; /* ':' between host and port, up to 5 for port */

    len += lstrlenW( request->path );
    if ((ret = malloc( len * sizeof(WCHAR) )))
    {
        offset = swprintf( ret, len, L"%s://%s", scheme, request->connect->hostname );
        if (request->connect->hostport)
        {
            offset += swprintf( ret + offset, len - offset, L":%u", request->connect->hostport );
        }
        lstrcpyW( ret + offset, request->path );
        if (path) *path = ret + offset;
    }

    return ret;
}

static WCHAR *build_request_string( struct request *request )
{
    WCHAR *path, *ret;
    unsigned int i, len;

    if (!wcsicmp( request->connect->hostname, request->connect->servername )) path = request->path;
    else if (!(path = build_absolute_request_path( request, NULL ))) return NULL;

    len = lstrlenW( request->verb ) + 1 /* ' ' */;
    len += lstrlenW( path ) + 1 /* ' ' */;
    len += lstrlenW( request->version );

    for (i = 0; i < request->num_headers; i++)
    {
        if (request->headers[i].is_request)
            len += lstrlenW( request->headers[i].field ) + lstrlenW( request->headers[i].value ) + 4; /* '\r\n: ' */
    }
    len += 4; /* '\r\n\r\n' */

    if ((ret = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        lstrcpyW( ret, request->verb );
        lstrcatW( ret, L" " );
        lstrcatW( ret, path );
        lstrcatW( ret, L" " );
        lstrcatW( ret, request->version );

        for (i = 0; i < request->num_headers; i++)
        {
            if (request->headers[i].is_request)
            {
                lstrcatW( ret, L"\r\n" );
                lstrcatW( ret, request->headers[i].field );
                lstrcatW( ret, L": " );
                lstrcatW( ret, request->headers[i].value );
            }
        }
        lstrcatW( ret, L"\r\n\r\n" );
    }

    if (path != request->path) free( path );
    return ret;
}

#define QUERY_MODIFIER_MASK (WINHTTP_QUERY_FLAG_REQUEST_HEADERS | WINHTTP_QUERY_FLAG_SYSTEMTIME | WINHTTP_QUERY_FLAG_NUMBER)

static DWORD query_headers( struct request *request, DWORD level, const WCHAR *name, void *buffer, DWORD *buflen,
                            DWORD *index )
{
    struct header *header = NULL;
    BOOL request_only;
    int requested_index, header_index = -1;
    DWORD attr, len, ret = ERROR_WINHTTP_HEADER_NOT_FOUND;

    request_only = level & WINHTTP_QUERY_FLAG_REQUEST_HEADERS;
    requested_index = index ? *index : 0;

    attr = level & ~QUERY_MODIFIER_MASK;
    switch (attr)
    {
    case WINHTTP_QUERY_CUSTOM:
    {
        header_index = get_header_index( request, name, requested_index, request_only );
        break;
    }
    case WINHTTP_QUERY_RAW_HEADERS:
    {
        WCHAR *headers, *p, *q;

        if (request_only)
            headers = build_request_string( request );
        else
            headers = request->raw_headers;

        if (!(p = headers)) return ERROR_OUTOFMEMORY;
        for (len = 0; *p; p++) if (*p != '\r') len++;

        if (!buffer || len * sizeof(WCHAR) > *buflen) ret = ERROR_INSUFFICIENT_BUFFER;
        else
        {
            for (p = headers, q = buffer; *p; p++, q++)
            {
                if (*p != '\r') *q = *p;
                else
                {
                    *q = 0;
                    p++; /* skip '\n' */
                }
            }
            TRACE("returning data: %s\n", debugstr_wn(buffer, len));
            if (len) len--;
            ret = ERROR_SUCCESS;
        }
        *buflen = len * sizeof(WCHAR);
        if (request_only) free( headers );
        return ret;
    }
    case WINHTTP_QUERY_RAW_HEADERS_CRLF:
    {
        WCHAR *headers;

        if (request_only)
            headers = build_request_string( request );
        else
            headers = request->raw_headers;

        if (!headers) return ERROR_OUTOFMEMORY;
        len = lstrlenW( headers ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            memcpy( buffer, headers, len + sizeof(WCHAR) );
            TRACE("returning data: %s\n", debugstr_wn(buffer, len / sizeof(WCHAR)));
            ret = ERROR_SUCCESS;
        }
        *buflen = len;
        if (request_only) free( headers );
        return ret;
    }
    case WINHTTP_QUERY_VERSION:
        len = lstrlenW( request->version ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            lstrcpyW( buffer, request->version );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = ERROR_SUCCESS;
        }
        *buflen = len;
        return ret;

    case WINHTTP_QUERY_STATUS_TEXT:
        len = lstrlenW( request->status_text ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            lstrcpyW( buffer, request->status_text );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = ERROR_SUCCESS;
        }
        *buflen = len;
        return ret;

    case WINHTTP_QUERY_REQUEST_METHOD:
        len = lstrlenW( request->verb ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            lstrcpyW( buffer, request->verb );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = ERROR_SUCCESS;
        }
        *buflen = len;
        return ret;

    default:
        if (attr >= ARRAY_SIZE(attribute_table)) return ERROR_INVALID_PARAMETER;
        if (!attribute_table[attr])
        {
            FIXME("attribute %u not implemented\n", attr);
            return ERROR_WINHTTP_HEADER_NOT_FOUND;
        }
        TRACE("attribute %s\n", debugstr_w(attribute_table[attr]));
        header_index = get_header_index( request, attribute_table[attr], requested_index, request_only );
        break;
    }

    if (header_index >= 0)
    {
        header = &request->headers[header_index];
    }
    if (!header || (request_only && !header->is_request)) return ERROR_WINHTTP_HEADER_NOT_FOUND;
    if (level & WINHTTP_QUERY_FLAG_NUMBER)
    {
        if (!buffer || sizeof(int) > *buflen) ret = ERROR_INSUFFICIENT_BUFFER;
        else
        {
            int *number = buffer;
            *number = wcstol( header->value, NULL, 10 );
            TRACE("returning number: %d\n", *number);
            ret = ERROR_SUCCESS;
        }
        *buflen = sizeof(int);
    }
    else if (level & WINHTTP_QUERY_FLAG_SYSTEMTIME)
    {
        SYSTEMTIME *st = buffer;
        if (!buffer || sizeof(SYSTEMTIME) > *buflen) ret = ERROR_INSUFFICIENT_BUFFER;
        else if (WinHttpTimeToSystemTime( header->value, st ))
        {
            TRACE("returning time: %04d/%02d/%02d - %d - %02d:%02d:%02d.%02d\n",
                  st->wYear, st->wMonth, st->wDay, st->wDayOfWeek,
                  st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);
            ret = ERROR_SUCCESS;
        }
        *buflen = sizeof(SYSTEMTIME);
    }
    else if (header->value)
    {
        len = lstrlenW( header->value ) * sizeof(WCHAR);
        if (!buffer || len + sizeof(WCHAR) > *buflen)
        {
            len += sizeof(WCHAR);
            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            lstrcpyW( buffer, header->value );
            TRACE("returning string: %s\n", debugstr_w(buffer));
            ret = ERROR_SUCCESS;
        }
        *buflen = len;
    }
    if (!ret && index) *index += 1;
    return ret;
}

/***********************************************************************
 *          WinHttpQueryHeaders (winhttp.@)
 */
BOOL WINAPI WinHttpQueryHeaders( HINTERNET hrequest, DWORD level, LPCWSTR name, LPVOID buffer, LPDWORD buflen, LPDWORD index )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, 0x%08x, %s, %p, %p, %p\n", hrequest, level, debugstr_w(name), buffer, buflen, index);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = query_headers( request, level, name, buffer, buflen, index );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static const struct
{
    const WCHAR *str;
    unsigned int len;
    DWORD scheme;
}
auth_schemes[] =
{
    { L"Basic",     ARRAY_SIZE(L"Basic") - 1,     WINHTTP_AUTH_SCHEME_BASIC },
    { L"NTLM",      ARRAY_SIZE(L"NTLM") - 1,      WINHTTP_AUTH_SCHEME_NTLM },
    { L"Passport",  ARRAY_SIZE(L"Passport") - 1,  WINHTTP_AUTH_SCHEME_PASSPORT },
    { L"Digest",    ARRAY_SIZE(L"Digest") - 1,    WINHTTP_AUTH_SCHEME_DIGEST },
    { L"Negotiate", ARRAY_SIZE(L"Negotiate") - 1, WINHTTP_AUTH_SCHEME_NEGOTIATE }
};

static enum auth_scheme scheme_from_flag( DWORD flag )
{
    int i;

    for (i = 0; i < ARRAY_SIZE( auth_schemes ); i++) if (flag == auth_schemes[i].scheme) return i;
    return SCHEME_INVALID;
}

static DWORD auth_scheme_from_header( const WCHAR *header )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE( auth_schemes ); i++)
    {
        if (!wcsnicmp( header, auth_schemes[i].str, auth_schemes[i].len ) &&
            (header[auth_schemes[i].len] == ' ' || !header[auth_schemes[i].len])) return auth_schemes[i].scheme;
    }
    return 0;
}

static DWORD query_auth_schemes( struct request *request, DWORD level, DWORD *supported, DWORD *first )
{
    DWORD ret, index = 0, supported_schemes = 0, first_scheme = 0;

    for (;;)
    {
        WCHAR *buffer;
        DWORD size, scheme;

        size = 0;
        ret = query_headers( request, level, NULL, NULL, &size, &index );
        if (ret != ERROR_INSUFFICIENT_BUFFER)
        {
            if (index) ret = ERROR_SUCCESS;
            break;
        }

        if (!(buffer = malloc( size ))) return ERROR_OUTOFMEMORY;
        if ((ret = query_headers( request, level, NULL, buffer, &size, &index )))
        {
            free( buffer );
            return ret;
        }
        scheme = auth_scheme_from_header( buffer );
        free( buffer );
        if (!scheme) continue;

        if (!first_scheme) first_scheme = scheme;
        supported_schemes |= scheme;
    }

    if (!ret)
    {
        *supported = supported_schemes;
        *first = first_scheme;
    }
    return ret;
}

/***********************************************************************
 *          WinHttpQueryAuthSchemes (winhttp.@)
 */
BOOL WINAPI WinHttpQueryAuthSchemes( HINTERNET hrequest, LPDWORD supported, LPDWORD first, LPDWORD target )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %p, %p, %p\n", hrequest, supported, first, target);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }
    if (!supported || !first || !target)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;

    }

    if (!(ret = query_auth_schemes( request, WINHTTP_QUERY_WWW_AUTHENTICATE, supported, first )))
    {
        *target = WINHTTP_AUTH_TARGET_SERVER;
    }
    else if (!(ret = query_auth_schemes( request, WINHTTP_QUERY_PROXY_AUTHENTICATE, supported, first )))
    {
        *target = WINHTTP_AUTH_TARGET_PROXY;
    }
    else ret = ERROR_INVALID_OPERATION;

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static UINT encode_base64( const char *bin, unsigned int len, WCHAR *base64 )
{
    UINT n = 0, x;
    static const char base64enc[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (len > 0)
    {
        /* first 6 bits, all from bin[0] */
        base64[n++] = base64enc[(bin[0] & 0xfc) >> 2];
        x = (bin[0] & 3) << 4;

        /* next 6 bits, 2 from bin[0] and 4 from bin[1] */
        if (len == 1)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[1] & 0xf0) >> 4)];
        x = (bin[1] & 0x0f) << 2;

        /* next 6 bits 4 from bin[1] and 2 from bin[2] */
        if (len == 2)
        {
            base64[n++] = base64enc[x];
            base64[n++] = '=';
            break;
        }
        base64[n++] = base64enc[x | ((bin[2] & 0xc0) >> 6)];

        /* last 6 bits, all from bin [2] */
        base64[n++] = base64enc[bin[2] & 0x3f];
        bin += 3;
        len -= 3;
    }
    base64[n] = 0;
    return n;
}

static inline char decode_char( WCHAR c )
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 64;
}

static unsigned int decode_base64( const WCHAR *base64, unsigned int len, char *buf )
{
    unsigned int i = 0;
    char c0, c1, c2, c3;
    const WCHAR *p = base64;

    while (len > 4)
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        len -= 4;
        i += 3;
        p += 4;
    }
    if (p[2] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;

        if (buf) buf[i] = (c0 << 2) | (c1 >> 4);
        i++;
    }
    else if (p[3] == '=')
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
        }
        i += 2;
    }
    else
    {
        if ((c0 = decode_char( p[0] )) > 63) return 0;
        if ((c1 = decode_char( p[1] )) > 63) return 0;
        if ((c2 = decode_char( p[2] )) > 63) return 0;
        if ((c3 = decode_char( p[3] )) > 63) return 0;

        if (buf)
        {
            buf[i + 0] = (c0 << 2) | (c1 >> 4);
            buf[i + 1] = (c1 << 4) | (c2 >> 2);
            buf[i + 2] = (c2 << 6) |  c3;
        }
        i += 3;
    }
    return i;
}

static struct authinfo *alloc_authinfo(void)
{
    struct authinfo *ret;

    if (!(ret = malloc( sizeof(*ret) ))) return NULL;

    SecInvalidateHandle( &ret->cred );
    SecInvalidateHandle( &ret->ctx );
    memset( &ret->exp, 0, sizeof(ret->exp) );
    ret->scheme    = 0;
    ret->attr      = 0;
    ret->max_token = 0;
    ret->data      = NULL;
    ret->data_len  = 0;
    ret->finished  = FALSE;
    return ret;
}

void destroy_authinfo( struct authinfo *authinfo )
{
    if (!authinfo) return;

    if (SecIsValidHandle( &authinfo->ctx ))
        DeleteSecurityContext( &authinfo->ctx );
    if (SecIsValidHandle( &authinfo->cred ))
        FreeCredentialsHandle( &authinfo->cred );

    free( authinfo->data );
    free( authinfo );
}

static BOOL get_authvalue( struct request *request, DWORD level, DWORD scheme, WCHAR *buffer, DWORD len )
{
    DWORD size, index = 0;
    for (;;)
    {
        size = len;
        if (query_headers( request, level, NULL, buffer, &size, &index )) return FALSE;
        if (auth_scheme_from_header( buffer ) == scheme) break;
    }
    return TRUE;
}

static BOOL do_authorization( struct request *request, DWORD target, DWORD scheme_flag )
{
    struct authinfo *authinfo, **auth_ptr;
    enum auth_scheme scheme = scheme_from_flag( scheme_flag );
    const WCHAR *auth_target, *username, *password;
    WCHAR auth_value[2048], *auth_reply;
    DWORD len = sizeof(auth_value), len_scheme, flags;
    BOOL ret, has_auth_value;

    if (scheme == SCHEME_INVALID) return FALSE;

    switch (target)
    {
    case WINHTTP_AUTH_TARGET_SERVER:
        has_auth_value = get_authvalue( request, WINHTTP_QUERY_WWW_AUTHENTICATE, scheme_flag, auth_value, len );
        auth_ptr = &request->authinfo;
        auth_target = L"Authorization";
        if (request->creds[TARGET_SERVER][scheme].username)
        {
            if (scheme != SCHEME_BASIC && !has_auth_value) return FALSE;
            username = request->creds[TARGET_SERVER][scheme].username;
            password = request->creds[TARGET_SERVER][scheme].password;
        }
        else
        {
            if (!has_auth_value) return FALSE;
            username = request->connect->username;
            password = request->connect->password;
        }
        break;

    case WINHTTP_AUTH_TARGET_PROXY:
        if (!get_authvalue( request, WINHTTP_QUERY_PROXY_AUTHENTICATE, scheme_flag, auth_value, len ))
            return FALSE;
        auth_ptr = &request->proxy_authinfo;
        auth_target = L"Proxy-Authorization";
        if (request->creds[TARGET_PROXY][scheme].username)
        {
            username = request->creds[TARGET_PROXY][scheme].username;
            password = request->creds[TARGET_PROXY][scheme].password;
        }
        else
        {
            username = request->connect->session->proxy_username;
            password = request->connect->session->proxy_password;
        }
        break;

    default:
        WARN("unknown target %x\n", target);
        return FALSE;
    }
    authinfo = *auth_ptr;

    switch (scheme)
    {
    case SCHEME_BASIC:
    {
        int userlen, passlen;

        if (!username || !password) return FALSE;
        if ((!authinfo && !(authinfo = alloc_authinfo())) || authinfo->finished) return FALSE;

        userlen = WideCharToMultiByte( CP_UTF8, 0, username, lstrlenW( username ), NULL, 0, NULL, NULL );
        passlen = WideCharToMultiByte( CP_UTF8, 0, password, lstrlenW( password ), NULL, 0, NULL, NULL );

        authinfo->data_len = userlen + 1 + passlen;
        if (!(authinfo->data = malloc( authinfo->data_len ))) return FALSE;

        WideCharToMultiByte( CP_UTF8, 0, username, -1, authinfo->data, userlen, NULL, NULL );
        authinfo->data[userlen] = ':';
        WideCharToMultiByte( CP_UTF8, 0, password, -1, authinfo->data + userlen + 1, passlen, NULL, NULL );

        authinfo->scheme   = SCHEME_BASIC;
        authinfo->finished = TRUE;
        break;
    }
    case SCHEME_NTLM:
    case SCHEME_NEGOTIATE:
    {
        SECURITY_STATUS status;
        SecBufferDesc out_desc, in_desc;
        SecBuffer out, in;
        ULONG flags = ISC_REQ_CONNECTION|ISC_REQ_USE_DCE_STYLE|ISC_REQ_MUTUAL_AUTH|ISC_REQ_DELEGATE;
        const WCHAR *p;
        BOOL first = FALSE;

        if (!authinfo)
        {
            TimeStamp exp;
            SEC_WINNT_AUTH_IDENTITY_W id;
            WCHAR *domain, *user;

            if (!username || !password || !(authinfo = alloc_authinfo())) return FALSE;

            first = TRUE;
            domain = (WCHAR *)username;
            user = wcschr( username, '\\' );

            if (user) user++;
            else
            {
                user = (WCHAR *)username;
                domain = NULL;
            }
            id.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;
            id.User           = user;
            id.UserLength     = lstrlenW( user );
            id.Domain         = domain;
            id.DomainLength   = domain ? user - domain - 1 : 0;
            id.Password       = (WCHAR *)password;
            id.PasswordLength = lstrlenW( password );

            status = AcquireCredentialsHandleW( NULL, (SEC_WCHAR *)auth_schemes[scheme].str,
                                                SECPKG_CRED_OUTBOUND, NULL, &id, NULL, NULL,
                                                &authinfo->cred, &exp );
            if (status == SEC_E_OK)
            {
                PSecPkgInfoW info;
                status = QuerySecurityPackageInfoW( (SEC_WCHAR *)auth_schemes[scheme].str, &info );
                if (status == SEC_E_OK)
                {
                    authinfo->max_token = info->cbMaxToken;
                    FreeContextBuffer( info );
                }
            }
            if (status != SEC_E_OK)
            {
                WARN("AcquireCredentialsHandleW for scheme %s failed with error 0x%08x\n",
                     debugstr_w(auth_schemes[scheme].str), status);
                free( authinfo );
                return FALSE;
            }
            authinfo->scheme = scheme;
        }
        else if (authinfo->finished) return FALSE;

        if ((lstrlenW( auth_value ) < auth_schemes[authinfo->scheme].len ||
            wcsnicmp( auth_value, auth_schemes[authinfo->scheme].str, auth_schemes[authinfo->scheme].len )))
        {
            ERR("authentication scheme changed from %s to %s\n",
                debugstr_w(auth_schemes[authinfo->scheme].str), debugstr_w(auth_value));
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        in.BufferType = SECBUFFER_TOKEN;
        in.cbBuffer   = 0;
        in.pvBuffer   = NULL;

        in_desc.ulVersion = 0;
        in_desc.cBuffers  = 1;
        in_desc.pBuffers  = &in;

        p = auth_value + auth_schemes[scheme].len;
        if (*p == ' ')
        {
            int len = lstrlenW( ++p );
            in.cbBuffer = decode_base64( p, len, NULL );
            if (!(in.pvBuffer = malloc( in.cbBuffer ))) {
                destroy_authinfo( authinfo );
                *auth_ptr = NULL;
                return FALSE;
            }
            decode_base64( p, len, in.pvBuffer );
        }
        out.BufferType = SECBUFFER_TOKEN;
        out.cbBuffer   = authinfo->max_token;
        if (!(out.pvBuffer = malloc( authinfo->max_token )))
        {
            free( in.pvBuffer );
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        out_desc.ulVersion = 0;
        out_desc.cBuffers  = 1;
        out_desc.pBuffers  = &out;

        status = InitializeSecurityContextW( first ? &authinfo->cred : NULL, first ? NULL : &authinfo->ctx,
                                             first ? request->connect->servername : NULL, flags, 0,
                                             SECURITY_NETWORK_DREP, in.pvBuffer ? &in_desc : NULL, 0,
                                             &authinfo->ctx, &out_desc, &authinfo->attr, &authinfo->exp );
        free( in.pvBuffer );
        if (status == SEC_E_OK)
        {
            free( authinfo->data );
            authinfo->data     = out.pvBuffer;
            authinfo->data_len = out.cbBuffer;
            authinfo->finished = TRUE;
            TRACE("sending last auth packet\n");
        }
        else if (status == SEC_I_CONTINUE_NEEDED)
        {
            free( authinfo->data );
            authinfo->data     = out.pvBuffer;
            authinfo->data_len = out.cbBuffer;
            TRACE("sending next auth packet\n");
        }
        else
        {
            ERR("InitializeSecurityContextW failed with error 0x%08x\n", status);
            free( out.pvBuffer );
            destroy_authinfo( authinfo );
            *auth_ptr = NULL;
            return FALSE;
        }
        break;
    }
    default:
        ERR("invalid scheme %u\n", scheme);
        return FALSE;
    }
    *auth_ptr = authinfo;

    len_scheme = auth_schemes[authinfo->scheme].len;
    len = len_scheme + 1 + ((authinfo->data_len + 2) * 4) / 3;
    if (!(auth_reply = malloc( (len + 1) * sizeof(WCHAR) ))) return FALSE;

    memcpy( auth_reply, auth_schemes[authinfo->scheme].str, len_scheme * sizeof(WCHAR) );
    auth_reply[len_scheme] = ' ';
    encode_base64( authinfo->data, authinfo->data_len, auth_reply + len_scheme + 1 );

    flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
    ret = !process_header( request, auth_target, auth_reply, flags, TRUE );
    free( auth_reply );
    return ret;
}

static WCHAR *build_proxy_connect_string( struct request *request )
{
    WCHAR *ret, *host;
    unsigned int i;
    int len = lstrlenW( request->connect->hostname ) + 7;

    if (!(host = malloc( len * sizeof(WCHAR) ))) return NULL;
    len = swprintf( host, len, L"%s:%u", request->connect->hostname, request->connect->hostport );

    len += ARRAY_SIZE(L"CONNECT");
    len += ARRAY_SIZE(L"HTTP/1.1");

    for (i = 0; i < request->num_headers; i++)
    {
        if (request->headers[i].is_request)
            len += lstrlenW( request->headers[i].field ) + lstrlenW( request->headers[i].value ) + 4; /* '\r\n: ' */
    }
    len += 4; /* '\r\n\r\n' */

    if ((ret = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        lstrcpyW( ret, L"CONNECT" );
        lstrcatW( ret, L" " );
        lstrcatW( ret, host );
        lstrcatW( ret, L" " );
        lstrcatW( ret, L"HTTP/1.1" );

        for (i = 0; i < request->num_headers; i++)
        {
            if (request->headers[i].is_request)
            {
                lstrcatW( ret, L"\r\n" );
                lstrcatW( ret, request->headers[i].field );
                lstrcatW( ret, L": " );
                lstrcatW( ret, request->headers[i].value );
            }
        }
        lstrcatW( ret, L"\r\n\r\n" );
    }

    free( host );
    return ret;
}

static DWORD read_reply( struct request *request );

static DWORD secure_proxy_connect( struct request *request )
{
    WCHAR *str;
    char *strA;
    int len, bytes_sent;
    DWORD ret;

    if (!(str = build_proxy_connect_string( request ))) return ERROR_OUTOFMEMORY;
    strA = strdupWA( str );
    free( str );
    if (!strA) return ERROR_OUTOFMEMORY;

    len = strlen( strA );
    ret = netconn_send( request->netconn, strA, len, &bytes_sent );
    free( strA );
    if (!ret) ret = read_reply( request );

    return ret;
}

static WCHAR *addr_to_str( struct sockaddr_storage *addr )
{
    char buf[INET6_ADDRSTRLEN];
    void *src;

    switch (addr->ss_family)
    {
    case AF_INET:
        src = &((struct sockaddr_in *)addr)->sin_addr;
        break;
    case AF_INET6:
        src = &((struct sockaddr_in6 *)addr)->sin6_addr;
        break;
    default:
        WARN("unsupported address family %d\n", addr->ss_family);
        return NULL;
    }
    if (!inet_ntop( addr->ss_family, src, buf, sizeof(buf) )) return NULL;
    return strdupAW( buf );
}

static CRITICAL_SECTION connection_pool_cs;
static CRITICAL_SECTION_DEBUG connection_pool_debug =
{
    0, 0, &connection_pool_cs,
    { &connection_pool_debug.ProcessLocksList, &connection_pool_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": connection_pool_cs") }
};
static CRITICAL_SECTION connection_pool_cs = { &connection_pool_debug, -1, 0, 0, 0, 0 };

static struct list connection_pool = LIST_INIT( connection_pool );

void release_host( struct hostdata *host )
{
    LONG ref;

    EnterCriticalSection( &connection_pool_cs );
    if (!(ref = --host->ref)) list_remove( &host->entry );
    LeaveCriticalSection( &connection_pool_cs );
    if (ref) return;

    assert( list_empty( &host->connections ) );
    free( host->hostname );
    free( host );
}

static BOOL connection_collector_running;

static void CALLBACK connection_collector( TP_CALLBACK_INSTANCE *instance, void *ctx )
{
    unsigned int remaining_connections;
    struct netconn *netconn, *next_netconn;
    struct hostdata *host, *next_host;
    ULONGLONG now;

    do
    {
        /* FIXME: Use more sophisticated method */
        Sleep(5000);
        remaining_connections = 0;
        now = GetTickCount64();

        EnterCriticalSection(&connection_pool_cs);

        LIST_FOR_EACH_ENTRY_SAFE(host, next_host, &connection_pool, struct hostdata, entry)
        {
            LIST_FOR_EACH_ENTRY_SAFE(netconn, next_netconn, &host->connections, struct netconn, entry)
            {
                if (netconn->keep_until < now)
                {
                    TRACE("freeing %p\n", netconn);
                    list_remove(&netconn->entry);
                    netconn_close(netconn);
                }
                else remaining_connections++;
            }
        }

        if (!remaining_connections) connection_collector_running = FALSE;

        LeaveCriticalSection(&connection_pool_cs);
    } while(remaining_connections);

    FreeLibraryWhenCallbackReturns( instance, winhttp_instance );
}

static void cache_connection( struct netconn *netconn )
{
    TRACE( "caching connection %p\n", netconn );

    EnterCriticalSection( &connection_pool_cs );

    netconn->keep_until = GetTickCount64() + DEFAULT_KEEP_ALIVE_TIMEOUT;
    list_add_head( &netconn->host->connections, &netconn->entry );

    if (!connection_collector_running)
    {
        HMODULE module;

        GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (const WCHAR *)winhttp_instance, &module );

        if (TrySubmitThreadpoolCallback( connection_collector, NULL, NULL )) connection_collector_running = TRUE;
        else FreeLibrary( winhttp_instance );
    }

    LeaveCriticalSection( &connection_pool_cs );
}

static DWORD map_secure_protocols( DWORD mask )
{
    DWORD ret = 0;
    if (mask & WINHTTP_FLAG_SECURE_PROTOCOL_SSL2) ret |= SP_PROT_SSL2_CLIENT;
    if (mask & WINHTTP_FLAG_SECURE_PROTOCOL_SSL3) ret |= SP_PROT_SSL3_CLIENT;
    if (mask & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1) ret |= SP_PROT_TLS1_CLIENT;
    if (mask & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1) ret |= SP_PROT_TLS1_1_CLIENT;
    if (mask & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2) ret |= SP_PROT_TLS1_2_CLIENT;
    return ret;
}

static DWORD ensure_cred_handle( struct request *request )
{
    SECURITY_STATUS status = SEC_E_OK;

    if (request->cred_handle_initialized) return ERROR_SUCCESS;

    if (!request->cred_handle_initialized)
    {
        SCHANNEL_CRED cred;
        memset( &cred, 0, sizeof(cred) );
        cred.dwVersion             = SCHANNEL_CRED_VERSION;
        cred.grbitEnabledProtocols = map_secure_protocols( request->connect->session->secure_protocols );
        if (request->client_cert)
        {
            cred.paCred = &request->client_cert;
            cred.cCreds = 1;
        }
        status = AcquireCredentialsHandleW( NULL, (WCHAR *)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL,
                                            &cred, NULL, NULL, &request->cred_handle, NULL );
        if (status == SEC_E_OK)
            request->cred_handle_initialized = TRUE;
    }

    if (status != SEC_E_OK)
    {
        WARN( "AcquireCredentialsHandleW failed: 0x%08x\n", status );
        return status;
    }
    return ERROR_SUCCESS;
}

static DWORD open_connection( struct request *request )
{
    BOOL is_secure = request->hdr.flags & WINHTTP_FLAG_SECURE;
    struct hostdata *host = NULL, *iter;
    struct netconn *netconn = NULL;
    struct connect *connect;
    WCHAR *addressW = NULL;
    INTERNET_PORT port;
    DWORD ret, len;

    if (request->netconn) goto done;

    connect = request->connect;
    port = connect->serverport ? connect->serverport : (request->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    EnterCriticalSection( &connection_pool_cs );

    LIST_FOR_EACH_ENTRY( iter, &connection_pool, struct hostdata, entry )
    {
        if (iter->port == port && !wcscmp( connect->servername, iter->hostname ) && !is_secure == !iter->secure)
        {
            host = iter;
            host->ref++;
            break;
        }
    }

    if (!host)
    {
        if ((host = malloc( sizeof(*host) )))
        {
            host->ref = 1;
            host->secure = is_secure;
            host->port = port;
            list_init( &host->connections );
            if ((host->hostname = strdupW( connect->servername )))
            {
                list_add_head( &connection_pool, &host->entry );
            }
            else
            {
                free( host );
                host = NULL;
            }
        }
    }

    LeaveCriticalSection( &connection_pool_cs );

    if (!host) return ERROR_OUTOFMEMORY;

    for (;;)
    {
        EnterCriticalSection( &connection_pool_cs );
        if (!list_empty( &host->connections ))
        {
            netconn = LIST_ENTRY( list_head( &host->connections ), struct netconn, entry );
            list_remove( &netconn->entry );
        }
        LeaveCriticalSection( &connection_pool_cs );
        if (!netconn) break;

        if (netconn_is_alive( netconn )) break;
        TRACE("connection %p no longer alive, closing\n", netconn);
        netconn_close( netconn );
        netconn = NULL;
    }

    if (!connect->resolved && netconn)
    {
        connect->sockaddr = netconn->sockaddr;
        connect->resolved = TRUE;
    }

    if (!connect->resolved)
    {
        len = lstrlenW( host->hostname ) + 1;
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESOLVING_NAME, host->hostname, len );

        if ((ret = netconn_resolve( host->hostname, port, &connect->sockaddr, request->resolve_timeout )))
        {
            release_host( host );
            return ret;
        }
        connect->resolved = TRUE;

        if (!(addressW = addr_to_str( &connect->sockaddr )))
        {
            release_host( host );
            return ERROR_OUTOFMEMORY;
        }
        len = lstrlenW( addressW ) + 1;
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_NAME_RESOLVED, addressW, len );
    }

    if (!netconn)
    {
        if (!addressW && !(addressW = addr_to_str( &connect->sockaddr )))
        {
            release_host( host );
            return ERROR_OUTOFMEMORY;
        }

        TRACE("connecting to %s:%u\n", debugstr_w(addressW), port);

        len = lstrlenW( addressW ) + 1;
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER, addressW, len );

        if ((ret = netconn_create( host, &connect->sockaddr, request->connect_timeout, &netconn )))
        {
            free( addressW );
            release_host( host );
            return ret;
        }
        netconn_set_timeout( netconn, TRUE, request->send_timeout );
        netconn_set_timeout( netconn, FALSE, request->receive_response_timeout );

        request->netconn = netconn;

        if (is_secure)
        {
            if (connect->session->proxy_server && wcsicmp( connect->hostname, connect->servername ))
            {
                if ((ret = secure_proxy_connect( request )))
                {
                    request->netconn = NULL;
                    free( addressW );
                    netconn_close( netconn );
                    return ret;
                }
            }

            CertFreeCertificateContext( request->server_cert );
            request->server_cert = NULL;

            if ((ret = ensure_cred_handle( request )) ||
                (ret = netconn_secure_connect( netconn, connect->hostname, request->security_flags,
                                               &request->cred_handle, request->check_revocation )))
            {
                request->netconn = NULL;
                free( addressW );
                netconn_close( netconn );
                return ret;
            }
        }

        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER, addressW, lstrlenW(addressW) + 1 );
    }
    else
    {
        TRACE("using connection %p\n", netconn);

        netconn_set_timeout( netconn, TRUE, request->send_timeout );
        netconn_set_timeout( netconn, FALSE, request->receive_response_timeout );
        request->netconn = netconn;
    }

    if (netconn->secure && !(request->server_cert = netconn_get_certificate( netconn )))
    {
        free( addressW );
        netconn_close( netconn );
        return ERROR_WINHTTP_SECURE_FAILURE;
    }

done:
    request->read_pos = request->read_size = 0;
    request->read_chunked = FALSE;
    request->read_chunked_size = ~0u;
    request->read_chunked_eof = FALSE;
    free( addressW );
    return ERROR_SUCCESS;
}

void close_connection( struct request *request )
{
    if (!request->netconn) return;

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION, 0, 0 );
    netconn_close( request->netconn );
    request->netconn = NULL;
    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED, 0, 0 );
}

static DWORD add_host_header( struct request *request, DWORD modifier )
{
    DWORD ret, len;
    WCHAR *host;
    struct connect *connect = request->connect;
    INTERNET_PORT port;

    port = connect->hostport ? connect->hostport : (request->hdr.flags & WINHTTP_FLAG_SECURE ? 443 : 80);

    if (port == INTERNET_DEFAULT_HTTP_PORT || port == INTERNET_DEFAULT_HTTPS_PORT)
    {
        return process_header( request, L"Host", connect->hostname, modifier, TRUE );
    }
    len = lstrlenW( connect->hostname ) + 7; /* sizeof(":65335") */
    if (!(host = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    swprintf( host, len, L"%s:%u", connect->hostname, port );
    ret = process_header( request, L"Host", host, modifier, TRUE );
    free( host );
    return ret;
}

static void clear_response_headers( struct request *request )
{
    unsigned int i;

    for (i = 0; i < request->num_headers; i++)
    {
        if (!request->headers[i].field) continue;
        if (!request->headers[i].value) continue;
        if (request->headers[i].is_request) continue;
        delete_header( request, i );
        i--;
    }
}

/* remove some amount of data from the read buffer */
static void remove_data( struct request *request, int count )
{
    if (!(request->read_size -= count)) request->read_pos = 0;
    else request->read_pos += count;
}

/* read some more data into the read buffer */
static DWORD read_more_data( struct request *request, int maxlen, BOOL notify )
{
    int len;
    DWORD ret;

    if (request->read_chunked_eof) return ERROR_INSUFFICIENT_BUFFER;

    if (request->read_size && request->read_pos)
    {
        /* move existing data to the start of the buffer */
        memmove( request->read_buf, request->read_buf + request->read_pos, request->read_size );
        request->read_pos = 0;
    }
    if (maxlen == -1) maxlen = sizeof(request->read_buf);

    if (notify) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE, NULL, 0 );

    ret = netconn_recv( request->netconn, request->read_buf + request->read_size,
                        maxlen - request->read_size, 0, &len );

    if (notify) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED, &len, sizeof(len) );

    request->read_size += len;
    return ret;
}

/* discard data contents until we reach end of line */
static DWORD discard_eol( struct request *request, BOOL notify )
{
    DWORD ret;
    do
    {
        char *eol = memchr( request->read_buf + request->read_pos, '\n', request->read_size );
        if (eol)
        {
            remove_data( request, (eol + 1) - (request->read_buf + request->read_pos) );
            break;
        }
        request->read_pos = request->read_size = 0;  /* discard everything */
        if ((ret = read_more_data( request, -1, notify ))) return ret;
    } while (request->read_size);
    return ERROR_SUCCESS;
}

/* read the size of the next chunk */
static DWORD start_next_chunk( struct request *request, BOOL notify )
{
    DWORD ret, chunk_size = 0;

    assert(!request->read_chunked_size || request->read_chunked_size == ~0u);

    if (request->read_chunked_eof) return ERROR_INSUFFICIENT_BUFFER;

    /* read terminator for the previous chunk */
    if (!request->read_chunked_size && (ret = discard_eol( request, notify ))) return ret;

    for (;;)
    {
        while (request->read_size)
        {
            char ch = request->read_buf[request->read_pos];
            if (ch >= '0' && ch <= '9') chunk_size = chunk_size * 16 + ch - '0';
            else if (ch >= 'a' && ch <= 'f') chunk_size = chunk_size * 16 + ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F') chunk_size = chunk_size * 16 + ch - 'A' + 10;
            else if (ch == ';' || ch == '\r' || ch == '\n')
            {
                TRACE("reading %u byte chunk\n", chunk_size);

                if (request->content_length == ~0u) request->content_length = chunk_size;
                else request->content_length += chunk_size;

                request->read_chunked_size = chunk_size;
                if (!chunk_size) request->read_chunked_eof = TRUE;

                return discard_eol( request, notify );
            }
            remove_data( request, 1 );
        }
        if ((ret = read_more_data( request, -1, notify ))) return ret;
        if (!request->read_size)
        {
            request->content_length = request->content_read = 0;
            request->read_chunked_size = 0;
            return ERROR_SUCCESS;
        }
    }
}

static DWORD refill_buffer( struct request *request, BOOL notify )
{
    int len = sizeof(request->read_buf);
    DWORD ret;

    if (request->read_chunked)
    {
        if (request->read_chunked_eof) return ERROR_INSUFFICIENT_BUFFER;
        if (request->read_chunked_size == ~0u || !request->read_chunked_size)
        {
            if ((ret = start_next_chunk( request, notify ))) return ret;
        }
        len = min( len, request->read_chunked_size );
    }
    else if (request->content_length != ~0u)
    {
        len = min( len, request->content_length - request->content_read );
    }

    if (len <= request->read_size) return ERROR_SUCCESS;
    if ((ret = read_more_data( request, len, notify ))) return ret;
    if (!request->read_size) request->content_length = request->content_read = 0;
    return ERROR_SUCCESS;
}

static void finished_reading( struct request *request )
{
    BOOL close = FALSE;
    WCHAR connection[20];
    DWORD size = sizeof(connection);

    if (!request->netconn) return;

    if (request->hdr.disable_flags & WINHTTP_DISABLE_KEEP_ALIVE) close = TRUE;
    else if (!query_headers( request, WINHTTP_QUERY_CONNECTION, NULL, connection, &size, NULL ) ||
             !query_headers( request, WINHTTP_QUERY_PROXY_CONNECTION, NULL, connection, &size, NULL ))
    {
        if (!wcsicmp( connection, L"close" )) close = TRUE;
    }
    else if (!wcscmp( request->version, L"HTTP/1.0" )) close = TRUE;
    if (close)
    {
        close_connection( request );
        return;
    }

    cache_connection( request->netconn );
    request->netconn = NULL;
}

/* return the size of data available to be read immediately */
static DWORD get_available_data( struct request *request )
{
    if (request->read_chunked) return min( request->read_chunked_size, request->read_size );
    return request->read_size;
}

/* check if we have reached the end of the data to read */
static BOOL end_of_read_data( struct request *request )
{
    if (!request->content_length) return TRUE;
    if (request->read_chunked) return request->read_chunked_eof;
    if (request->content_length == ~0u) return FALSE;
    return (request->content_length == request->content_read);
}

static DWORD read_data( struct request *request, void *buffer, DWORD size, DWORD *read, BOOL async )
{
    int count, bytes_read = 0;
    DWORD ret = ERROR_SUCCESS;

    if (end_of_read_data( request )) goto done;

    while (size)
    {
        if (!(count = get_available_data( request )))
        {
            if ((ret = refill_buffer( request, async ))) goto done;
            if (!(count = get_available_data( request ))) goto done;
        }
        count = min( count, size );
        memcpy( (char *)buffer + bytes_read, request->read_buf + request->read_pos, count );
        remove_data( request, count );
        if (request->read_chunked) request->read_chunked_size -= count;
        size -= count;
        bytes_read += count;
        request->content_read += count;
        if (end_of_read_data( request )) goto done;
    }
    if (request->read_chunked && !request->read_chunked_size) ret = refill_buffer( request, async );

done:
    TRACE( "retrieved %u bytes (%u/%u)\n", bytes_read, request->content_read, request->content_length );
    if (async)
    {
        if (!ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, buffer, bytes_read );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_READ_DATA;
            result.dwError  = ret;
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }

    if (!ret && read) *read = bytes_read;
    if (end_of_read_data( request )) finished_reading( request );
    return ret;
}

/* read any content returned by the server so that the connection can be reused */
static void drain_content( struct request *request )
{
    DWORD size, bytes_read, bytes_total = 0, bytes_left = request->content_length - request->content_read;
    char buffer[2048];

    refill_buffer( request, FALSE );
    for (;;)
    {
        if (request->read_chunked) size = sizeof(buffer);
        else
        {
            if (bytes_total >= bytes_left) return;
            size = min( sizeof(buffer), bytes_left - bytes_total );
        }
        if (read_data( request, buffer, size, &bytes_read, FALSE ) || !bytes_read) return;
        bytes_total += bytes_read;
    }
}

enum escape_flags
{
    ESCAPE_FLAG_NON_PRINTABLE = 0x01,
    ESCAPE_FLAG_SPACE         = 0x02,
    ESCAPE_FLAG_PERCENT       = 0x04,
    ESCAPE_FLAG_UNSAFE        = 0x08,
    ESCAPE_FLAG_DEL           = 0x10,
    ESCAPE_FLAG_8BIT          = 0x20,
    ESCAPE_FLAG_STRIP_CRLF    = 0x40,
};

#define ESCAPE_MASK_DEFAULT (ESCAPE_FLAG_NON_PRINTABLE | ESCAPE_FLAG_SPACE | ESCAPE_FLAG_UNSAFE |\
                             ESCAPE_FLAG_DEL | ESCAPE_FLAG_8BIT)
#define ESCAPE_MASK_PERCENT (ESCAPE_FLAG_PERCENT | ESCAPE_MASK_DEFAULT)
#define ESCAPE_MASK_DISABLE (ESCAPE_FLAG_SPACE | ESCAPE_FLAG_8BIT | ESCAPE_FLAG_STRIP_CRLF)

static inline BOOL need_escape( char ch, enum escape_flags flags )
{
    static const char unsafe[] = "\"#<>[\\]^`{|}";
    const char *ptr = unsafe;

    if ((flags & ESCAPE_FLAG_SPACE) && ch == ' ') return TRUE;
    if ((flags & ESCAPE_FLAG_PERCENT) && ch == '%') return TRUE;
    if ((flags & ESCAPE_FLAG_NON_PRINTABLE) && ch < 0x20) return TRUE;
    if ((flags & ESCAPE_FLAG_DEL) && ch == 0x7f) return TRUE;
    if ((flags & ESCAPE_FLAG_8BIT) && (ch & 0x80)) return TRUE;
    if ((flags & ESCAPE_FLAG_UNSAFE)) while (*ptr) { if (ch == *ptr++) return TRUE; }
    return FALSE;
}

static DWORD escape_string( const char *src, DWORD len, char *dst, enum escape_flags flags )
{
    static const char hex[] = "0123456789ABCDEF";
    DWORD i, ret = len;
    char *ptr = dst;

    for (i = 0; i < len; i++)
    {
        if ((flags & ESCAPE_FLAG_STRIP_CRLF) && (src[i] == '\r' || src[i] == '\n'))
        {
            ret--;
            continue;
        }
        if (need_escape( src[i], flags ))
        {
            if (dst)
            {
                ptr[0] = '%';
                ptr[1] = hex[(src[i] >> 4) & 0xf];
                ptr[2] = hex[src[i] & 0xf];
                ptr += 3;
            }
            ret += 2;
        }
        else if (dst) *ptr++ = src[i];
    }

    if (dst) dst[ret] = 0;
    return ret;
}

static DWORD str_to_wire( const WCHAR *src, int src_len, char *dst, enum escape_flags flags )
{
    DWORD len;
    char *utf8;

    if (src_len < 0) src_len = lstrlenW( src );
    len = WideCharToMultiByte( CP_UTF8, 0, src, src_len, NULL, 0, NULL, NULL );
    if (!(utf8 = malloc( len ))) return 0;

    WideCharToMultiByte( CP_UTF8, 0, src, -1, utf8, len, NULL, NULL );
    len = escape_string( utf8, len, dst, flags );
    free( utf8 );

    return len;
}

static char *build_wire_path( struct request *request, DWORD *ret_len )
{
    WCHAR *full_path;
    const WCHAR *start, *path, *query = NULL;
    DWORD len, len_path = 0, len_query = 0;
    enum escape_flags path_flags, query_flags;
    char *ret;

    if (!wcsicmp( request->connect->hostname, request->connect->servername )) start = full_path = request->path;
    else if (!(full_path = build_absolute_request_path( request, &start ))) return NULL;

    len = lstrlenW( full_path );
    if ((path = wcschr( start, '/' )))
    {
        len_path = lstrlenW( path );
        if ((query = wcschr( path, '?' )))
        {
            len_query = lstrlenW( query );
            len_path -= len_query;
        }
    }

    if (request->hdr.flags & WINHTTP_FLAG_ESCAPE_DISABLE) path_flags = ESCAPE_MASK_DISABLE;
    else if (request->hdr.flags & WINHTTP_FLAG_ESCAPE_PERCENT) path_flags = ESCAPE_MASK_PERCENT;
    else path_flags = ESCAPE_MASK_DEFAULT;

    if (request->hdr.flags & WINHTTP_FLAG_ESCAPE_DISABLE_QUERY) query_flags = ESCAPE_MASK_DISABLE;
    else query_flags = path_flags;

    *ret_len = str_to_wire( full_path, len - len_path - len_query, NULL, 0 );
    if (path) *ret_len += str_to_wire( path, len_path, NULL, path_flags );
    if (query) *ret_len += str_to_wire( query, len_query, NULL, query_flags );

    if ((ret = malloc( *ret_len + 1 )))
    {
        len = str_to_wire( full_path, len - len_path - len_query, ret, 0 );
        if (path) len += str_to_wire( path, len_path, ret + len, path_flags );
        if (query) str_to_wire( query, len_query, ret + len, query_flags );
    }

    if (full_path != request->path) free( full_path );
    return ret;
}

static char *build_wire_request( struct request *request, DWORD *len )
{
    char *path, *ptr, *ret;
    DWORD i, len_path;

    if (!(path = build_wire_path( request, &len_path ))) return NULL;

    *len = str_to_wire( request->verb, -1, NULL, 0 ) + 1; /* ' ' */
    *len += len_path + 1; /* ' ' */
    *len += str_to_wire( request->version, -1, NULL, 0 );

    for (i = 0; i < request->num_headers; i++)
    {
        if (request->headers[i].is_request)
        {
            *len += str_to_wire( request->headers[i].field, -1, NULL, 0 ) + 2; /* ': ' */
            *len += str_to_wire( request->headers[i].value, -1, NULL, 0 ) + 2; /* '\r\n' */
        }
    }
    *len += 4; /* '\r\n\r\n' */

    if ((ret = ptr = malloc( *len + 1 )))
    {
        ptr += str_to_wire( request->verb, -1, ptr, 0 );
        *ptr++ = ' ';
        memcpy( ptr, path, len_path );
        ptr += len_path;
        *ptr++ = ' ';
        ptr += str_to_wire( request->version, -1, ptr, 0 );

        for (i = 0; i < request->num_headers; i++)
        {
            if (request->headers[i].is_request)
            {
                *ptr++ = '\r';
                *ptr++ = '\n';
                ptr += str_to_wire( request->headers[i].field, -1, ptr, 0 );
                *ptr++ = ':';
                *ptr++ = ' ';
                ptr += str_to_wire( request->headers[i].value, -1, ptr, 0 );
            }
        }
        memcpy( ptr, "\r\n\r\n", sizeof("\r\n\r\n") );
    }

    free( path );
    return ret;
}

static WCHAR *create_websocket_key(void)
{
    WCHAR *ret;
    char buf[16];
    DWORD base64_len = ((sizeof(buf) + 2) * 4) / 3;
    if (!RtlGenRandom( buf, sizeof(buf) )) return NULL;
    if ((ret = malloc( (base64_len + 1) * sizeof(WCHAR) ))) encode_base64( buf, sizeof(buf), ret );
    return ret;
}

static DWORD add_websocket_key_header( struct request *request )
{
    WCHAR *key = create_websocket_key();
    if (!key) return ERROR_OUTOFMEMORY;
    process_header( request, L"Sec-WebSocket-Key", key, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE, TRUE );
    free( key );
    return ERROR_SUCCESS;
}

static DWORD send_request( struct request *request, const WCHAR *headers, DWORD headers_len, void *optional,
                           DWORD optional_len, DWORD total_len, DWORD_PTR context, BOOL async )
{
    struct connect *connect = request->connect;
    struct session *session = connect->session;
    char *wire_req;
    int bytes_sent;
    DWORD ret, len;

    clear_response_headers( request );
    drain_content( request );

    if (session->agent)
        process_header( request, L"User-Agent", session->agent, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );

    if (connect->hostname)
        add_host_header( request, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW );

    if (request->creds[TARGET_SERVER][SCHEME_BASIC].username)
        do_authorization( request, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC );

    if (total_len || (request->verb && !wcscmp( request->verb, L"POST" )))
    {
        WCHAR length[21]; /* decimal long int + null */
        swprintf( length, ARRAY_SIZE(length), L"%ld", total_len );
        process_header( request, L"Content-Length", length, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (request->flags & REQUEST_FLAG_WEBSOCKET_UPGRADE)
    {
        process_header( request, L"Upgrade", L"websocket", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
        process_header( request, L"Connection", L"Upgrade", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
        process_header( request, L"Sec-WebSocket-Version", L"13", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
        if ((ret = add_websocket_key_header( request ))) return ret;
    }
    else if (!(request->hdr.disable_flags & WINHTTP_DISABLE_KEEP_ALIVE))
    {
        process_header( request, L"Connection", L"Keep-Alive", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (request->hdr.flags & WINHTTP_FLAG_REFRESH)
    {
        process_header( request, L"Pragma", L"no-cache", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
        process_header( request, L"Cache-Control", L"no-cache", WINHTTP_ADDREQ_FLAG_ADD_IF_NEW, TRUE );
    }
    if (headers && (ret = add_request_headers( request, headers, headers_len,
                                               WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE )))
    {
        TRACE("failed to add request headers: %u\n", ret);
        return ret;
    }
    if (!(request->hdr.disable_flags & WINHTTP_DISABLE_COOKIES) && (ret = add_cookie_headers( request )))
    {
        WARN("failed to add cookie headers: %u\n", ret);
        return ret;
    }

    if (context) request->hdr.context = context;

    if ((ret = open_connection( request ))) goto end;
    if (!(wire_req = build_wire_request( request, &len )))
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }
    TRACE("full request: %s\n", debugstr_a(wire_req));

    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_SENDING_REQUEST, NULL, 0 );

    ret = netconn_send( request->netconn, wire_req, len, &bytes_sent );
    free( wire_req );
    if (ret) goto end;

    if (optional_len)
    {
        if ((ret = netconn_send( request->netconn, optional, optional_len, &bytes_sent ))) goto end;
        request->optional = optional;
        request->optional_len = optional_len;
        len += optional_len;
    }
    send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_SENT, &len, sizeof(len) );

end:
    if (async)
    {
        if (!ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE, NULL, 0 );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_SEND_REQUEST;
            result.dwError  = ret;
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_send_request( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct send_request *s = ctx;

    TRACE("running %p\n", work);
    send_request( s->request, s->headers, s->headers_len, s->optional, s->optional_len, s->total_len, s->context, TRUE );

    release_object( &s->request->hdr );
    free( s->headers );
    free( s );
}

/***********************************************************************
 *          WinHttpSendRequest (winhttp.@)
 */
BOOL WINAPI WinHttpSendRequest( HINTERNET hrequest, LPCWSTR headers, DWORD headers_len,
                                LPVOID optional, DWORD optional_len, DWORD total_len, DWORD_PTR context )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %s, %u, %p, %u, %u, %lx\n", hrequest, debugstr_wn(headers, headers_len), headers_len, optional,
          optional_len, total_len, context);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (headers && !headers_len) headers_len = lstrlenW( headers );

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct send_request *s;

        if (!(s = malloc( sizeof(*s) ))) return FALSE;
        s->request      = request;
        s->headers      = strdupW( headers );
        s->headers_len  = headers_len;
        s->optional     = optional;
        s->optional_len = optional_len;
        s->total_len    = total_len;
        s->context      = context;

        addref_object( &request->hdr );
        if ((ret = queue_task( &request->queue, task_send_request, s )))
        {
            release_object( &request->hdr );
            free( s->headers );
            free( s );
        }
    }
    else ret = send_request( request, headers, headers_len, optional, optional_len, total_len, context, FALSE );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static DWORD set_credentials( struct request *request, DWORD target, DWORD scheme_flag, const WCHAR *username,
                              const WCHAR *password )
{
    enum auth_scheme scheme = scheme_from_flag( scheme_flag );

    if (scheme == SCHEME_INVALID || ((scheme == SCHEME_BASIC || scheme == SCHEME_DIGEST) && (!username || !password)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    switch (target)
    {
    case WINHTTP_AUTH_TARGET_SERVER:
    {
        free( request->creds[TARGET_SERVER][scheme].username );
        if (!username) request->creds[TARGET_SERVER][scheme].username = NULL;
        else if (!(request->creds[TARGET_SERVER][scheme].username = strdupW( username ))) return ERROR_OUTOFMEMORY;

        free( request->creds[TARGET_SERVER][scheme].password );
        if (!password) request->creds[TARGET_SERVER][scheme].password = NULL;
        else if (!(request->creds[TARGET_SERVER][scheme].password = strdupW( password ))) return ERROR_OUTOFMEMORY;
        break;
    }
    case WINHTTP_AUTH_TARGET_PROXY:
    {
        free( request->creds[TARGET_PROXY][scheme].username );
        if (!username) request->creds[TARGET_PROXY][scheme].username = NULL;
        else if (!(request->creds[TARGET_PROXY][scheme].username = strdupW( username ))) return ERROR_OUTOFMEMORY;

        free( request->creds[TARGET_PROXY][scheme].password );
        if (!password) request->creds[TARGET_PROXY][scheme].password = NULL;
        else if (!(request->creds[TARGET_PROXY][scheme].password = strdupW( password ))) return ERROR_OUTOFMEMORY;
        break;
    }
    default:
        WARN("unknown target %u\n", target);
        return ERROR_INVALID_PARAMETER;
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 *          WinHttpSetCredentials (winhttp.@)
 */
BOOL WINAPI WinHttpSetCredentials( HINTERNET hrequest, DWORD target, DWORD scheme, LPCWSTR username,
                                   LPCWSTR password, LPVOID params )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %x, 0x%08x, %s, %p, %p\n", hrequest, target, scheme, debugstr_w(username), password, params);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    ret = set_credentials( request, target, scheme, username, password );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static DWORD handle_authorization( struct request *request, DWORD status )
{
    DWORD ret, i, schemes, first, level, target;

    switch (status)
    {
    case HTTP_STATUS_DENIED:
        target = WINHTTP_AUTH_TARGET_SERVER;
        level  = WINHTTP_QUERY_WWW_AUTHENTICATE;
        break;

    case HTTP_STATUS_PROXY_AUTH_REQ:
        target = WINHTTP_AUTH_TARGET_PROXY;
        level  = WINHTTP_QUERY_PROXY_AUTHENTICATE;
        break;

    default:
        ERR("unhandled status %u\n", status);
        return ERROR_WINHTTP_INTERNAL_ERROR;
    }

    if ((ret = query_auth_schemes( request, level, &schemes, &first ))) return ret;
    if (do_authorization( request, target, first )) return ERROR_SUCCESS;

    schemes &= ~first;
    for (i = 0; i < ARRAY_SIZE( auth_schemes ); i++)
    {
        if (!(schemes & auth_schemes[i].scheme)) continue;
        if (do_authorization( request, target, auth_schemes[i].scheme )) return ERROR_SUCCESS;
    }
    return ERROR_WINHTTP_LOGIN_FAILURE;
}

/* set the request content length based on the headers */
static void set_content_length( struct request *request, DWORD status )
{
    WCHAR encoding[20];
    DWORD buflen = sizeof(request->content_length);

    if (status == HTTP_STATUS_NO_CONTENT || status == HTTP_STATUS_NOT_MODIFIED ||
        status == HTTP_STATUS_SWITCH_PROTOCOLS || !wcscmp( request->verb, L"HEAD" ))
    {
        request->content_length = 0;
    }
    else
    {
        if (query_headers( request, WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER,
                           NULL, &request->content_length, &buflen, NULL ))
            request->content_length = ~0u;

        buflen = sizeof(encoding);
        if (!query_headers( request, WINHTTP_QUERY_TRANSFER_ENCODING, NULL, encoding, &buflen, NULL ) &&
            !wcsicmp( encoding, L"chunked" ))
        {
            request->content_length = ~0u;
            request->read_chunked = TRUE;
            request->read_chunked_size = ~0u;
            request->read_chunked_eof = FALSE;
        }
    }
    request->content_read = 0;
}

static DWORD read_line( struct request *request, char *buffer, DWORD *len )
{
    int count, bytes_read, pos = 0;
    DWORD ret;

    for (;;)
    {
        char *eol = memchr( request->read_buf + request->read_pos, '\n', request->read_size );
        if (eol)
        {
            count = eol - (request->read_buf + request->read_pos);
            bytes_read = count + 1;
        }
        else count = bytes_read = request->read_size;

        count = min( count, *len - pos );
        memcpy( buffer + pos, request->read_buf + request->read_pos, count );
        pos += count;
        remove_data( request, bytes_read );
        if (eol) break;

        if ((ret = read_more_data( request, -1, TRUE ))) return ret;
        if (!request->read_size)
        {
            *len = 0;
            TRACE("returning empty string\n");
            return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        }
    }
    if (pos < *len)
    {
        if (pos && buffer[pos - 1] == '\r') pos--;
        *len = pos + 1;
    }
    buffer[*len - 1] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return ERROR_SUCCESS;
}

#define MAX_REPLY_LEN   1460
#define INITIAL_HEADER_BUFFER_LEN  512

static DWORD read_reply( struct request *request )
{
    char buffer[MAX_REPLY_LEN];
    DWORD ret, buflen, len, offset, crlf_len = 2; /* lstrlenW(crlf) */
    char *status_code, *status_text;
    WCHAR *versionW, *status_textW, *raw_headers;
    WCHAR status_codeW[4]; /* sizeof("nnn") */

    if (!request->netconn) return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;

    do
    {
        buflen = MAX_REPLY_LEN;
        if ((ret = read_line( request, buffer, &buflen ))) return ret;

        /* first line should look like 'HTTP/1.x nnn OK' where nnn is the status code */
        if (!(status_code = strchr( buffer, ' ' ))) return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        status_code++;
        if (!(status_text = strchr( status_code, ' ' ))) return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        if ((len = status_text - status_code) != sizeof("nnn") - 1) return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        status_text++;

        TRACE("version [%s] status code [%s] status text [%s]\n",
              debugstr_an(buffer, status_code - buffer - 1),
              debugstr_an(status_code, len),
              debugstr_a(status_text));

    } while (!memcmp( status_code, "100", len )); /* ignore "100 Continue" responses */

    /*  we rely on the fact that the protocol is ascii */
    MultiByteToWideChar( CP_ACP, 0, status_code, len, status_codeW, len );
    status_codeW[len] = 0;
    if ((ret = process_header( request, L"Status", status_codeW,
                               WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE, FALSE ))) return ret;

    len = status_code - buffer;
    if (!(versionW = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    MultiByteToWideChar( CP_ACP, 0, buffer, len - 1, versionW, len -1 );
    versionW[len - 1] = 0;

    free( request->version );
    request->version = versionW;

    len = buflen - (status_text - buffer);
    if (!(status_textW = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    MultiByteToWideChar( CP_ACP, 0, status_text, len, status_textW, len );

    free( request->status_text );
    request->status_text = status_textW;

    len = max( buflen + crlf_len, INITIAL_HEADER_BUFFER_LEN );
    if (!(raw_headers = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
    MultiByteToWideChar( CP_ACP, 0, buffer, buflen, raw_headers, buflen );
    memcpy( raw_headers + buflen - 1, L"\r\n", sizeof(L"\r\n") );

    free( request->raw_headers );
    request->raw_headers = raw_headers;

    offset = buflen + crlf_len - 1;
    for (;;)
    {
        struct header *header;

        buflen = MAX_REPLY_LEN;
        if (read_line( request, buffer, &buflen )) return ERROR_SUCCESS;
        if (!*buffer) buflen = 1;

        while (len - offset < buflen + crlf_len)
        {
            WCHAR *tmp;
            len *= 2;
            if (!(tmp = realloc( raw_headers, len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
            request->raw_headers = raw_headers = tmp;
        }
        if (!*buffer)
        {
            memcpy( raw_headers + offset, L"\r\n", sizeof(L"\r\n") );
            break;
        }
        MultiByteToWideChar( CP_ACP, 0, buffer, buflen, raw_headers + offset, buflen );

        if (!(header = parse_header( raw_headers + offset ))) break;
        if ((ret = process_header( request, header->field, header->value, WINHTTP_ADDREQ_FLAG_ADD, FALSE )))
        {
            free_header( header );
            break;
        }
        free_header( header );
        memcpy( raw_headers + offset + buflen - 1, L"\r\n", sizeof(L"\r\n") );
        offset += buflen + crlf_len - 1;
    }

    TRACE("raw headers: %s\n", debugstr_w(raw_headers));
    return ret;
}

static void record_cookies( struct request *request )
{
    unsigned int i;

    for (i = 0; i < request->num_headers; i++)
    {
        struct header *set_cookie = &request->headers[i];
        if (!wcsicmp( set_cookie->field, L"Set-Cookie" ) && !set_cookie->is_request)
        {
            set_cookies( request, set_cookie->value );
        }
    }
}

static DWORD get_redirect_url( struct request *request, WCHAR **ret_url, DWORD *ret_len )
{
    DWORD size, ret;
    WCHAR *url;

    ret = query_headers( request, WINHTTP_QUERY_LOCATION, NULL, NULL, &size, NULL );
    if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;
    if (!(url = malloc( size ))) return ERROR_OUTOFMEMORY;
    if ((ret = query_headers( request, WINHTTP_QUERY_LOCATION, NULL, url, &size, NULL )))
    {
        free( url );
        return ret;
    }
    *ret_url = url;
    *ret_len = size / sizeof(WCHAR);
    return ERROR_SUCCESS;
}

static DWORD handle_redirect( struct request *request, DWORD status )
{
    DWORD ret, len, len_loc = 0;
    URL_COMPONENTS uc;
    struct connect *connect = request->connect;
    INTERNET_PORT port;
    WCHAR *hostname = NULL, *location = NULL;
    int index;

    if ((ret = get_redirect_url( request, &location, &len_loc ))) return ret;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength = uc.dwHostNameLength = uc.dwUrlPathLength = uc.dwExtraInfoLength = ~0u;

    if (!WinHttpCrackUrl( location, len_loc, 0, &uc )) /* assume relative redirect */
    {
        WCHAR *path, *p;

        ret = ERROR_OUTOFMEMORY;
        if (location[0] == '/')
        {
            if (!(path = malloc( (len_loc + 1) * sizeof(WCHAR) ))) goto end;
            memcpy( path, location, len_loc * sizeof(WCHAR) );
            path[len_loc] = 0;
        }
        else
        {
            if ((p = wcsrchr( request->path, '/' ))) *p = 0;
            len = lstrlenW( request->path ) + 1 + len_loc;
            if (!(path = malloc( (len + 1) * sizeof(WCHAR) ))) goto end;
            lstrcpyW( path, request->path );
            lstrcatW( path, L"/" );
            memcpy( path + lstrlenW(path), location, len_loc * sizeof(WCHAR) );
            path[len_loc] = 0;
        }
        free( request->path );
        request->path = path;

        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REDIRECT, location, len_loc + 1 );
    }
    else
    {
        if (uc.nScheme == INTERNET_SCHEME_HTTP && request->hdr.flags & WINHTTP_FLAG_SECURE)
        {
            if (request->hdr.redirect_policy == WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP)
            {
                ret = ERROR_WINHTTP_REDIRECT_FAILED;
                goto end;
            }
            TRACE("redirect from secure page to non-secure page\n");
            request->hdr.flags &= ~WINHTTP_FLAG_SECURE;
        }
        else if (uc.nScheme == INTERNET_SCHEME_HTTPS && !(request->hdr.flags & WINHTTP_FLAG_SECURE))
        {
            TRACE("redirect from non-secure page to secure page\n");
            request->hdr.flags |= WINHTTP_FLAG_SECURE;
        }

        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REDIRECT, location, len_loc + 1 );

        len = uc.dwHostNameLength;
        if (!(hostname = malloc( (len + 1) * sizeof(WCHAR) )))
        {
            ret = ERROR_OUTOFMEMORY;
            goto end;
        }
        memcpy( hostname, uc.lpszHostName, len * sizeof(WCHAR) );
        hostname[len] = 0;

        port = uc.nPort ? uc.nPort : (uc.nScheme == INTERNET_SCHEME_HTTPS ? 443 : 80);
        if (wcsicmp( connect->hostname, hostname ) || connect->serverport != port)
        {
            free( connect->hostname );
            connect->hostname = hostname;
            connect->hostport = port;
            if (!set_server_for_hostname( connect, hostname, port ))
            {
                ret = ERROR_OUTOFMEMORY;
                goto end;
            }

            netconn_close( request->netconn );
            request->netconn = NULL;
            request->content_length = request->content_read = 0;
            request->read_pos = request->read_size = 0;
            request->read_chunked = request->read_chunked_eof = FALSE;
        }
        else free( hostname );

        if ((ret = add_host_header( request, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ))) goto end;
        if ((ret = open_connection( request ))) goto end;

        free( request->path );
        request->path = NULL;
        if (uc.dwUrlPathLength)
        {
            len = uc.dwUrlPathLength + uc.dwExtraInfoLength;
            if (!(request->path = malloc( (len + 1) * sizeof(WCHAR) ))) goto end;
            memcpy( request->path, uc.lpszUrlPath, (len + 1) * sizeof(WCHAR) );
            request->path[len] = 0;
        }
        else request->path = strdupW( L"/" );
    }

    /* remove content-type/length headers */
    if ((index = get_header_index( request, L"Content-Type", 0, TRUE )) >= 0) delete_header( request, index );
    if ((index = get_header_index( request, L"Content-Length", 0, TRUE )) >= 0 ) delete_header( request, index );

    if (status != HTTP_STATUS_REDIRECT_KEEP_VERB && !wcscmp( request->verb, L"POST" ))
    {
        free( request->verb );
        request->verb = strdupW( L"GET" );
        request->optional = NULL;
        request->optional_len = 0;
    }

end:
    free( location );
    return ret;
}

static BOOL is_passport_request( struct request *request )
{
    static const WCHAR passportW[] = {'P','a','s','s','p','o','r','t','1','.','4'};
    WCHAR buf[1024];
    DWORD len = ARRAY_SIZE(buf);

    if (!(request->connect->session->passport_flags & WINHTTP_ENABLE_PASSPORT_AUTH) ||
        query_headers( request, WINHTTP_QUERY_WWW_AUTHENTICATE, NULL, buf, &len, NULL )) return FALSE;

    if (!wcsnicmp( buf, passportW, ARRAY_SIZE(passportW) ) &&
        (buf[ARRAY_SIZE(passportW)] == ' ' || !buf[ARRAY_SIZE(passportW)])) return TRUE;

    return FALSE;
}

static DWORD handle_passport_redirect( struct request *request )
{
    DWORD ret, flags = WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE;
    int i, len = lstrlenW( request->raw_headers );
    WCHAR *p = request->raw_headers;

    if ((ret = process_header( request, L"Status", L"401", flags, FALSE ))) return ret;

    for (i = 0; i < len; i++)
    {
        if (i <= len - 3 && p[i] == '3' && p[i + 1] == '0' && p[i + 2] == '2')
        {
            p[i] = '4';
            p[i + 2] = '1';
            break;
        }
    }
    return ERROR_SUCCESS;
}

static DWORD receive_response( struct request *request, BOOL async )
{
    DWORD ret, size, query, status;

    if (!request->netconn) return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;

    netconn_set_timeout( request->netconn, FALSE, request->receive_response_timeout );
    for (;;)
    {
        if ((ret = read_reply( request ))) break;

        size = sizeof(DWORD);
        query = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
        if ((ret = query_headers( request, query, NULL, &status, &size, NULL ))) break;

        set_content_length( request, status );

        if (!(request->hdr.disable_flags & WINHTTP_DISABLE_COOKIES)) record_cookies( request );

        if (status == HTTP_STATUS_REDIRECT && is_passport_request( request ))
        {
            ret = handle_passport_redirect( request );
        }
        else if (status == HTTP_STATUS_MOVED || status == HTTP_STATUS_REDIRECT || status == HTTP_STATUS_REDIRECT_KEEP_VERB)
        {
            if (request->hdr.disable_flags & WINHTTP_DISABLE_REDIRECTS ||
                request->hdr.redirect_policy == WINHTTP_OPTION_REDIRECT_POLICY_NEVER) break;

            if (++request->redirect_count > request->max_redirects) return ERROR_WINHTTP_REDIRECT_FAILED;

            if ((ret = handle_redirect( request, status ))) break;

            /* recurse synchronously */
            if (!(ret = send_request( request, NULL, 0, request->optional, request->optional_len, 0, 0, FALSE ))) continue;
        }
        else if (status == HTTP_STATUS_DENIED || status == HTTP_STATUS_PROXY_AUTH_REQ)
        {
            if (request->hdr.disable_flags & WINHTTP_DISABLE_AUTHENTICATION) break;

            if ((ret = handle_authorization( request, status ))) break;

            /* recurse synchronously */
            if (!(ret = send_request( request, NULL, 0, request->optional, request->optional_len, 0, 0, FALSE ))) continue;
        }
        break;
    }

    if (request->netconn) netconn_set_timeout( request->netconn, FALSE, request->receive_timeout );
    if (request->content_length) ret = refill_buffer( request, FALSE );

    if (async)
    {
        if (!ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE, NULL, 0 );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_RECEIVE_RESPONSE;
            result.dwError  = ret;
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_receive_response( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct receive_response *r = ctx;

    TRACE("running %p\n", work);
    receive_response( r->request, TRUE );

    release_object( &r->request->hdr );
    free( r );
}

/***********************************************************************
 *          WinHttpReceiveResponse (winhttp.@)
 */
BOOL WINAPI WinHttpReceiveResponse( HINTERNET hrequest, LPVOID reserved )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %p\n", hrequest, reserved);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct receive_response *r;

        if (!(r = malloc( sizeof(*r) ))) return FALSE;
        r->request = request;

        addref_object( &request->hdr );
        if ((ret = queue_task( &request->queue, task_receive_response, r )))
        {
            release_object( &request->hdr );
            free( r );
        }
    }
    else ret = receive_response( request, FALSE );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static DWORD query_data_available( struct request *request, DWORD *available, BOOL async )
{
    DWORD ret = ERROR_SUCCESS, count = 0;

    if (end_of_read_data( request )) goto done;

    count = get_available_data( request );
    if (!request->read_chunked && request->netconn) count += netconn_query_data_available( request->netconn );
    if (!count)
    {
        if ((ret = refill_buffer( request, async ))) goto done;
        count = get_available_data( request );
        if (!request->read_chunked && request->netconn) count += netconn_query_data_available( request->netconn );
    }

done:
    TRACE("%u bytes available\n", count);
    if (async)
    {
        if (!ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE, &count, sizeof(count) );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_QUERY_DATA_AVAILABLE;
            result.dwError  = ret;
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }

    if (!ret && available) *available = count;
    return ret;
}

static void CALLBACK task_query_data_available( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct query_data *q = ctx;

    TRACE("running %p\n", work);
    query_data_available( q->request, q->available, TRUE );

    release_object( &q->request->hdr );
    free( q );
}

/***********************************************************************
 *          WinHttpQueryDataAvailable (winhttp.@)
 */
BOOL WINAPI WinHttpQueryDataAvailable( HINTERNET hrequest, LPDWORD available )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %p\n", hrequest, available);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct query_data *q;

        if (!(q = malloc( sizeof(*q) ))) return FALSE;
        q->request   = request;
        q->available = available;

        addref_object( &request->hdr );
        if ((ret = queue_task( &request->queue, task_query_data_available, q )))
        {
            release_object( &request->hdr );
            free( q );
        }
    }
    else ret = query_data_available( request, available, FALSE );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static void CALLBACK task_read_data( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct read_data *r = ctx;

    TRACE("running %p\n", work);
    read_data( r->request, r->buffer, r->to_read, r->read, TRUE );

    release_object( &r->request->hdr );
    free( r );
}

/***********************************************************************
 *          WinHttpReadData (winhttp.@)
 */
BOOL WINAPI WinHttpReadData( HINTERNET hrequest, LPVOID buffer, DWORD to_read, LPDWORD read )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %p, %d, %p\n", hrequest, buffer, to_read, read);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct read_data *r;

        if (!(r = malloc( sizeof(*r) ))) return FALSE;
        r->request = request;
        r->buffer  = buffer;
        r->to_read = to_read;
        r->read    = read;

        addref_object( &request->hdr );
        if ((ret = queue_task( &request->queue, task_read_data, r )))
        {
            release_object( &request->hdr );
            free( r );
        }
    }
    else ret = read_data( request, buffer, to_read, read, FALSE );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static DWORD write_data( struct request *request, const void *buffer, DWORD to_write, DWORD *written, BOOL async )
{
    DWORD ret;
    int num_bytes;

    ret = netconn_send( request->netconn, buffer, to_write, &num_bytes );

    if (async)
    {
        if (!ret) send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, &num_bytes, sizeof(num_bytes) );
        else
        {
            WINHTTP_ASYNC_RESULT result;
            result.dwResult = API_WRITE_DATA;
            result.dwError  = ret;
            send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    if (!ret && written) *written = num_bytes;
    return ret;
}

static void CALLBACK task_write_data( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct write_data *w = ctx;

    TRACE("running %p\n", work);
    write_data( w->request, w->buffer, w->to_write, w->written, TRUE );

    release_object( &w->request->hdr );
    free( w );
}

/***********************************************************************
 *          WinHttpWriteData (winhttp.@)
 */
BOOL WINAPI WinHttpWriteData( HINTERNET hrequest, LPCVOID buffer, DWORD to_write, LPDWORD written )
{
    DWORD ret;
    struct request *request;

    TRACE("%p, %p, %d, %p\n", hrequest, buffer, to_write, written);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return FALSE;
    }

    if (request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct write_data *w;

        if (!(w = malloc( sizeof(*w) ))) return FALSE;
        w->request  = request;
        w->buffer   = buffer;
        w->to_write = to_write;
        w->written  = written;

        addref_object( &request->hdr );
        if ((ret = queue_task( &request->queue, task_write_data, w )))
        {
            release_object( &request->hdr );
            free( w );
        }
    }
    else ret = write_data( request, buffer, to_write, written, FALSE );

    release_object( &request->hdr );
    SetLastError( ret );
    return !ret;
}

static BOOL socket_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    FIXME("unimplemented option %u\n", option);
    SetLastError( ERROR_WINHTTP_INVALID_OPTION );
    return FALSE;
}

static void socket_destroy( struct object_header *hdr )
{
    struct socket *socket = (struct socket *)hdr;

    TRACE("%p\n", socket);

    stop_queue( &socket->send_q );
    stop_queue( &socket->recv_q );

    release_object( &socket->request->hdr );
    free( socket );
}

static BOOL socket_set_option( struct object_header *hdr, DWORD option, void *buffer, DWORD buflen )
{
    FIXME("unimplemented option %u\n", option);
    SetLastError( ERROR_WINHTTP_INVALID_OPTION );
    return FALSE;
}

static const struct object_vtbl socket_vtbl =
{
    socket_destroy,
    socket_query_option,
    socket_set_option,
};

HINTERNET WINAPI WinHttpWebSocketCompleteUpgrade( HINTERNET hrequest, DWORD_PTR context )
{
    struct socket *socket;
    struct request *request;
    HINTERNET hsocket = NULL;

    TRACE("%p, %08lx\n", hrequest, context);

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(socket = calloc( 1, sizeof(*socket) )))
    {
        release_object( &request->hdr );
        return NULL;
    }
    socket->hdr.type = WINHTTP_HANDLE_TYPE_SOCKET;
    socket->hdr.vtbl = &socket_vtbl;
    socket->hdr.refs = 1;
    socket->hdr.callback = request->hdr.callback;
    socket->hdr.notify_mask = request->hdr.notify_mask;
    socket->hdr.context = context;

    addref_object( &request->hdr );
    socket->request = request;

    if ((hsocket = alloc_handle( &socket->hdr )))
    {
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hsocket, sizeof(hsocket) );
    }

    release_object( &socket->hdr );
    release_object( &request->hdr );
    TRACE("returning %p\n", hsocket);
    if (hsocket) SetLastError( ERROR_SUCCESS );
    return hsocket;
}

static DWORD send_bytes( struct netconn *netconn, char *bytes, int len )
{
    int count;
    DWORD err;
    if ((err = netconn_send( netconn, bytes, len, &count ))) return err;
    return (count == len) ? ERROR_SUCCESS : ERROR_INTERNAL_ERROR;
}

#define FIN_BIT (1 << 7)
#define MASK_BIT (1 << 7)
#define RESERVED_BIT (7 << 4)
#define CONTROL_BIT (1 << 3)

static DWORD send_frame( struct netconn *netconn, enum socket_opcode opcode, USHORT status, const char *buf,
                         DWORD buflen, BOOL final )
{
    DWORD i = 0, j, ret, offset = 2, len = buflen;
    char hdr[14], byte, *mask = NULL;

    TRACE("sending %02x frame\n", opcode);

    if (opcode == SOCKET_OPCODE_CLOSE) len += sizeof(status);

    hdr[0] = final ? (char)FIN_BIT : 0;
    hdr[0] |= opcode;
    hdr[1] = (char)MASK_BIT;
    if (len < 126) hdr[1] |= len;
    else if (len < 65536)
    {
        hdr[1] |= 126;
        hdr[2] = len >> 8;
        hdr[3] = len & 0xff;
        offset += 2;
    }
    else
    {
        hdr[1] |= 127;
        hdr[2] = hdr[3] = hdr[4] = hdr[5] = 0;
        hdr[6] = len >> 24;
        hdr[7] = (len >> 16) & 0xff;
        hdr[8] = (len >> 8) & 0xff;
        hdr[9] = len & 0xff;
        offset += 8;
    }

    if ((ret = send_bytes( netconn, hdr, offset ))) return ret;
    if (len)
    {
        mask = &hdr[offset];
        RtlGenRandom( mask, 4 );
        if ((ret = send_bytes( netconn, mask, 4 ))) return ret;
    }

    if (opcode == SOCKET_OPCODE_CLOSE) /* prepend status code */
    {
        byte = (status >> 8) ^ mask[i++ % 4];
        if ((ret = send_bytes( netconn, &byte, 1 ))) return ret;

        byte = (status & 0xff) ^ mask[i++ % 4];
        if ((ret = send_bytes( netconn, &byte, 1 ))) return ret;
    }

    for (j = 0; j < buflen; j++)
    {
        byte = buf[j] ^ mask[i++ % 4];
        if ((ret = send_bytes( netconn, &byte, 1 ))) return ret;
    }

    return ERROR_SUCCESS;
}

static enum socket_opcode map_buffer_type( WINHTTP_WEB_SOCKET_BUFFER_TYPE type )
{
    switch (type)
    {
    case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:   return SOCKET_OPCODE_TEXT;
    case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE: return SOCKET_OPCODE_BINARY;
    case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:          return SOCKET_OPCODE_CLOSE;
    default:
        FIXME("buffer type %u not supported\n", type);
        return SOCKET_OPCODE_INVALID;
    }
}

static DWORD socket_send( struct socket *socket, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, const void *buf, DWORD len,
                          BOOL async )
{
    enum socket_opcode opcode = map_buffer_type( type );
    DWORD ret;

    ret = send_frame( socket->request->netconn, opcode, 0, buf, len, TRUE );
    if (async)
    {
        if (!ret)
        {
            WINHTTP_WEB_SOCKET_STATUS status;
            status.dwBytesTransferred = len;
            status.eBufferType        = type;
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, &status, sizeof(status) );
        }
        else
        {
            WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
            result.AsyncResult.dwResult = API_WRITE_DATA;
            result.AsyncResult.dwError  = ret;
            result.Operation = WINHTTP_WEB_SOCKET_SEND_OPERATION;
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_socket_send( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct socket_send *s = ctx;

    TRACE("running %p\n", work);
    socket_send( s->socket, s->type, s->buf, s->len, TRUE );

    release_object( &s->socket->hdr );
    free( s );
}

DWORD WINAPI WinHttpWebSocketSend( HINTERNET hsocket, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, void *buf, DWORD len )
{
    struct socket *socket;
    DWORD ret;

    TRACE("%p, %u, %p, %u\n", hsocket, type, buf, len);

    if (len && !buf) return ERROR_INVALID_PARAMETER;
    if (type != WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE && type != WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
    {
        FIXME("buffer type %u not supported\n", type);
        return ERROR_NOT_SUPPORTED;
    }

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_SOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state != SOCKET_STATE_OPEN)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
    }

    if (socket->request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_send *s;

        if (!(s = malloc( sizeof(*s) ))) return FALSE;
        s->socket = socket;
        s->type   = type;
        s->buf    = buf;
        s->len    = len;

        addref_object( &socket->hdr );
        if ((ret = queue_task( &socket->send_q, task_socket_send, s )))
        {
            release_object( &socket->hdr );
            free( s );
        }
    }
    else ret = socket_send( socket, type, buf, len, FALSE );

    release_object( &socket->hdr );
    return ret;
}

static DWORD receive_bytes( struct netconn *netconn, char *buf, DWORD len, DWORD *ret_len )
{
    DWORD err;
    if ((err = netconn_recv( netconn, buf, len, 0, (int *)ret_len ))) return err;
    if (*ret_len != len) return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    return ERROR_SUCCESS;
}

static BOOL is_supported_opcode( enum socket_opcode opcode )
{
    switch (opcode)
    {
    case SOCKET_OPCODE_TEXT:
    case SOCKET_OPCODE_BINARY:
    case SOCKET_OPCODE_CLOSE:
    case SOCKET_OPCODE_PING:
    case SOCKET_OPCODE_PONG:
        return TRUE;
    default:
        FIXME( "opcode %02x not handled\n", opcode );
        return FALSE;
    }
}

static DWORD receive_frame( struct netconn *netconn, DWORD *ret_len, enum socket_opcode *opcode )
{
    DWORD ret, len, count;
    char hdr[2];

    if ((ret = receive_bytes( netconn, hdr, sizeof(hdr), &count ))) return ret;
    if ((hdr[0] & RESERVED_BIT) || (hdr[1] & MASK_BIT) || !is_supported_opcode( hdr[0] & 0xf ))
    {
        return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    }
    *opcode = hdr[0] & 0xf;
    TRACE("received %02x frame\n", *opcode);

    len = hdr[1] & ~MASK_BIT;
    if (len == 126)
    {
        USHORT len16;
        if ((ret = receive_bytes( netconn, (char *)&len16, sizeof(len16), &count ))) return ret;
        len = RtlUshortByteSwap( len16 );
    }
    else if (len == 127)
    {
        ULONGLONG len64;
        if ((ret = receive_bytes( netconn, (char *)&len64, sizeof(len64), &count ))) return ret;
        if ((len64 = RtlUlonglongByteSwap( len64 )) > ~0u) return ERROR_NOT_SUPPORTED;
        len = len64;
    }

    *ret_len = len;
    return ERROR_SUCCESS;
}

static void CALLBACK task_socket_send_pong( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct socket_send *s = ctx;

    TRACE("running %p\n", work);
    send_frame( s->socket->request->netconn, SOCKET_OPCODE_PONG, 0, NULL, 0, TRUE );

    release_object( &s->socket->hdr );
    free( s );
}

static DWORD socket_send_pong( struct socket *socket )
{
    if (socket->request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_send *s;
        DWORD ret;

        if (!(s = malloc( sizeof(*s) ))) return ERROR_OUTOFMEMORY;
        s->socket = socket;

        addref_object( &socket->hdr );
        if ((ret = queue_task( &socket->send_q, task_socket_send_pong, s )))
        {
            release_object( &socket->hdr );
            free( s );
        }
        return ret;
    }
    return send_frame( socket->request->netconn, SOCKET_OPCODE_PONG, 0, NULL, 0, TRUE );
}

static DWORD socket_drain( struct socket *socket )
{
    struct netconn *netconn = socket->request->netconn;
    DWORD ret, count;

    while (socket->read_size)
    {
        char buf[1024];
        if ((ret = receive_bytes( netconn, buf, min(socket->read_size, sizeof(buf)), &count ))) return ret;
        socket->read_size -= count;
    }
    return ERROR_SUCCESS;
}

static DWORD handle_control_frame( struct socket *socket )
{
    switch (socket->opcode)
    {
    case SOCKET_OPCODE_PING:
        return socket_send_pong( socket );

    case SOCKET_OPCODE_PONG:
        return socket_drain( socket );

    default:
        ERR("unhandled control opcode %02x\n", socket->opcode);
        return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    }

    return ERROR_SUCCESS;
}

static WINHTTP_WEB_SOCKET_BUFFER_TYPE map_opcode( enum socket_opcode opcode, BOOL fragment )
{
    switch (opcode)
    {
    case SOCKET_OPCODE_TEXT:
        if (fragment) return WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
        return WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

    case SOCKET_OPCODE_BINARY:
        if (fragment) return WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
        return WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE;

    case SOCKET_OPCODE_CLOSE:
        return WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE;

    default:
        FIXME("opcode %02x not handled\n", opcode);
        return ~0u;
    }
}

static DWORD socket_receive( struct socket *socket, void *buf, DWORD len, DWORD *ret_len,
                             WINHTTP_WEB_SOCKET_BUFFER_TYPE *ret_type, BOOL async )
{
    struct netconn *netconn = socket->request->netconn;
    DWORD count, ret = ERROR_SUCCESS;

    if (!socket->read_size)
    {
        for (;;)
        {
            if (!(ret = receive_frame( netconn, &socket->read_size, &socket->opcode )))
            {
                if (!(socket->opcode & CONTROL_BIT) || (ret = handle_control_frame( socket ))) break;
            }
            else if (ret == WSAETIMEDOUT) ret = socket_send_pong( socket );
            if (ret) break;
        }
    }
    if (!ret) ret = receive_bytes( netconn, buf, min(len, socket->read_size), &count );
    if (!ret)
    {
        socket->read_size -= count;
        if (!async)
        {
            *ret_len = count;
            *ret_type = map_opcode( socket->opcode, socket->read_size != 0 );
        }
    }
    if (async)
    {
        if (!ret)
        {
            WINHTTP_WEB_SOCKET_STATUS status;
            status.dwBytesTransferred = count;
            status.eBufferType        = map_opcode( socket->opcode, socket->read_size != 0 );
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, &status, sizeof(status) );
        }
        else
        {
            WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
            result.AsyncResult.dwResult = API_READ_DATA;
            result.AsyncResult.dwError  = ret;
            result.Operation = WINHTTP_WEB_SOCKET_RECEIVE_OPERATION;
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_socket_receive( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct socket_receive *r = ctx;

    TRACE("running %p\n", work);
    socket_receive( r->socket, r->buf, r->len, NULL, NULL, TRUE );

    release_object( &r->socket->hdr );
    free( r );
}

DWORD WINAPI WinHttpWebSocketReceive( HINTERNET hsocket, void *buf, DWORD len, DWORD *ret_len,
                                      WINHTTP_WEB_SOCKET_BUFFER_TYPE *ret_type )
{
    struct socket *socket;
    DWORD ret;

    TRACE("%p, %p, %u, %p, %p\n", hsocket, buf, len, ret_len, ret_type);

    if (!buf || !len) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_SOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state != SOCKET_STATE_OPEN)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
    }

    if (socket->request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_receive *r;

        if (!(r = malloc( sizeof(*r) ))) return FALSE;
        r->socket = socket;
        r->buf    = buf;
        r->len    = len;

        addref_object( &socket->hdr );
        if ((ret = queue_task( &socket->recv_q, task_socket_receive, r )))
        {
            release_object( &socket->hdr );
            free( r );
        }
    }
    else ret = socket_receive( socket, buf, len, ret_len, ret_type, FALSE );

    release_object( &socket->hdr );
    return ret;
}

static DWORD socket_shutdown( struct socket *socket, USHORT status, const void *reason, DWORD len, BOOL async )
{
    struct netconn *netconn = socket->request->netconn;
    DWORD ret;

    stop_queue( &socket->send_q );
    if (!(ret = send_frame( netconn, SOCKET_OPCODE_CLOSE, status, reason, len, TRUE )))
    {
        socket->state = SOCKET_STATE_SHUTDOWN;
    }
    if (async)
    {
        if (!ret) send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE, NULL, 0 );
        else
        {
            WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
            result.AsyncResult.dwResult = API_WRITE_DATA;
            result.AsyncResult.dwError  = ret;
            result.Operation = WINHTTP_WEB_SOCKET_SHUTDOWN_OPERATION;
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_socket_shutdown( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct socket_shutdown *s = ctx;

    socket_shutdown( s->socket, s->status, s->reason, s->len, TRUE );

    TRACE("running %p\n", work);
    release_object( &s->socket->hdr );
    free( s );
}

DWORD WINAPI WinHttpWebSocketShutdown( HINTERNET hsocket, USHORT status, void *reason, DWORD len )
{
    struct socket *socket;
    DWORD ret;

    TRACE("%p, %u, %p, %u\n", hsocket, status, reason, len);

    if ((len && !reason) || len > sizeof(socket->reason)) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_SOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state >= SOCKET_STATE_SHUTDOWN)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
    }

    if (socket->request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_shutdown *s;

        if (!(s = malloc( sizeof(*s) ))) return FALSE;
        s->socket = socket;
        s->status = status;
        memcpy( s->reason, reason, len );
        s->len    = len;

        addref_object( &socket->hdr );
        if ((ret = queue_task( &socket->send_q, task_socket_shutdown, s )))
        {
            release_object( &socket->hdr );
            free( s );
        }
    }
    else ret = socket_shutdown( socket, status, reason, len, FALSE );

    release_object( &socket->hdr );
    return ret;
}

static DWORD socket_close( struct socket *socket, USHORT status, const void *reason, DWORD len, BOOL async )
{
    struct netconn *netconn = socket->request->netconn;
    DWORD ret, count;

    if ((ret = socket_drain( socket ))) goto done;

    if (socket->state < SOCKET_STATE_SHUTDOWN)
    {
        stop_queue( &socket->send_q );
        if ((ret = send_frame( netconn, SOCKET_OPCODE_CLOSE, status, reason, len, TRUE ))) goto done;
        socket->state = SOCKET_STATE_SHUTDOWN;
    }

    if ((ret = receive_frame( netconn, &count, &socket->opcode ))) goto done;
    if (socket->opcode != SOCKET_OPCODE_CLOSE ||
        (count && (count < sizeof(socket->status) || count > sizeof(socket->status) + sizeof(socket->reason))))
    {
        ret = ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        goto done;
    }

    if (count)
    {
        DWORD reason_len = count - sizeof(socket->status);
        if ((ret = receive_bytes( netconn, (char *)&socket->status, sizeof(socket->status), &count ))) goto done;
        socket->status = RtlUshortByteSwap( socket->status );
        if ((ret = receive_bytes( netconn, socket->reason, reason_len, &socket->reason_len ))) goto done;
    }
    socket->state = SOCKET_STATE_CLOSED;

done:
    if (async)
    {
        if (!ret) send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE, NULL, 0 );
        else
        {
            WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
            result.AsyncResult.dwResult = API_READ_DATA; /* FIXME */
            result.AsyncResult.dwError  = ret;
            result.Operation = WINHTTP_WEB_SOCKET_CLOSE_OPERATION;
            send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
        }
    }
    return ret;
}

static void CALLBACK task_socket_close( TP_CALLBACK_INSTANCE *instance, void *ctx, TP_WORK *work )
{
    struct socket_shutdown *s = ctx;

    socket_close( s->socket, s->status, s->reason, s->len, TRUE );

    TRACE("running %p\n", work);
    release_object( &s->socket->hdr );
    free( s );
}

DWORD WINAPI WinHttpWebSocketClose( HINTERNET hsocket, USHORT status, void *reason, DWORD len )
{
    struct socket *socket;
    DWORD ret;

    TRACE("%p, %u, %p, %u\n", hsocket, status, reason, len);

    if ((len && !reason) || len > sizeof(socket->reason)) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_SOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state >= SOCKET_STATE_CLOSED)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
    }

    if (socket->request->connect->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_shutdown *s;

        if (!(s = malloc( sizeof(*s) ))) return FALSE;
        s->socket = socket;
        s->status = status;
        memcpy( s->reason, reason, len );
        s->len    = len;

        addref_object( &socket->hdr );
        if ((ret = queue_task( &socket->recv_q, task_socket_close, s )))
        {
            release_object( &socket->hdr );
            free( s );
        }
    }
    else ret = socket_close( socket, status, reason, len, FALSE );

    release_object( &socket->hdr );
    return ret;
}

DWORD WINAPI WinHttpWebSocketQueryCloseStatus( HINTERNET hsocket, USHORT *status, void *reason, DWORD len,
                                               DWORD *ret_len )
{
    struct socket *socket;
    DWORD ret;

    TRACE("%p, %p, %p, %u, %p\n", hsocket, status, reason, len, ret_len);

    if (!status || (len && !reason) || !ret_len) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_SOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state < SOCKET_STATE_CLOSED)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
    }

    *status = socket->status;
    *ret_len = socket->reason_len;
    if (socket->reason_len > len) ret = ERROR_INSUFFICIENT_BUFFER;
    else
    {
        memcpy( reason, socket->reason, socket->reason_len );
        ret = ERROR_SUCCESS;
    }

    release_object( &socket->hdr );
    return ret;
}

enum request_state
{
    REQUEST_STATE_INITIALIZED,
    REQUEST_STATE_CANCELLED,
    REQUEST_STATE_OPEN,
    REQUEST_STATE_SENT,
    REQUEST_STATE_RESPONSE_RECEIVED
};

struct winhttp_request
{
    IWinHttpRequest IWinHttpRequest_iface;
    LONG refs;
    CRITICAL_SECTION cs;
    enum request_state state;
    HINTERNET hsession;
    HINTERNET hconnect;
    HINTERNET hrequest;
    VARIANT data;
    WCHAR *verb;
    HANDLE done;
    HANDLE wait;
    HANDLE cancel;
    BOOL proc_running;
    char *buffer;
    DWORD offset;
    DWORD bytes_available;
    DWORD bytes_read;
    DWORD error;
    DWORD logon_policy;
    DWORD disable_feature;
    LONG resolve_timeout;
    LONG connect_timeout;
    LONG send_timeout;
    LONG receive_timeout;
    WINHTTP_PROXY_INFO proxy;
    BOOL async;
    UINT url_codepage;
};

static inline struct winhttp_request *impl_from_IWinHttpRequest( IWinHttpRequest *iface )
{
    return CONTAINING_RECORD( iface, struct winhttp_request, IWinHttpRequest_iface );
}

static ULONG WINAPI winhttp_request_AddRef(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    return InterlockedIncrement( &request->refs );
}

/* critical section must be held */
static void cancel_request( struct winhttp_request *request )
{
    if (request->state <= REQUEST_STATE_CANCELLED) return;

    if (request->proc_running)
    {
        SetEvent( request->cancel );
        LeaveCriticalSection( &request->cs );

        WaitForSingleObject( request->done, INFINITE );

        EnterCriticalSection( &request->cs );
    }
    request->state = REQUEST_STATE_CANCELLED;
}

/* critical section must be held */
static void free_request( struct winhttp_request *request )
{
    if (request->state < REQUEST_STATE_INITIALIZED) return;
    WinHttpCloseHandle( request->hrequest );
    WinHttpCloseHandle( request->hconnect );
    WinHttpCloseHandle( request->hsession );
    CloseHandle( request->done );
    CloseHandle( request->wait );
    CloseHandle( request->cancel );
    free( request->proxy.lpszProxy );
    free( request->proxy.lpszProxyBypass );
    free( request->buffer );
    free( request->verb );
    VariantClear( &request->data );
}

static ULONG WINAPI winhttp_request_Release(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    LONG refs = InterlockedDecrement( &request->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", request);

        EnterCriticalSection( &request->cs );
        cancel_request( request );
        free_request( request );
        LeaveCriticalSection( &request->cs );
        request->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &request->cs );
        free( request );
    }
    return refs;
}

static HRESULT WINAPI winhttp_request_QueryInterface(
    IWinHttpRequest *iface,
    REFIID riid,
    void **obj )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %s, %p\n", request, debugstr_guid(riid), obj );

    if (IsEqualGUID( riid, &IID_IWinHttpRequest ) ||
        IsEqualGUID( riid, &IID_IDispatch ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWinHttpRequest_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_GetTypeInfoCount(
    IWinHttpRequest *iface,
    UINT *count )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %p\n", request, count);
    *count = 1;
    return S_OK;
}

enum type_id
{
    IWinHttpRequest_tid,
    last_tid
};

static ITypeLib *winhttp_typelib;
static ITypeInfo *winhttp_typeinfo[last_tid];

static REFIID winhttp_tid_id[] =
{
    &IID_IWinHttpRequest
};

static HRESULT get_typeinfo( enum type_id tid, ITypeInfo **ret )
{
    HRESULT hr;

    if (!winhttp_typelib)
    {
        ITypeLib *typelib;

        hr = LoadRegTypeLib( &LIBID_WinHttp, 5, 1, LOCALE_SYSTEM_DEFAULT, &typelib );
        if (FAILED(hr))
        {
            ERR("LoadRegTypeLib failed: %08x\n", hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)&winhttp_typelib, typelib, NULL ))
            ITypeLib_Release( typelib );
    }
    if (!winhttp_typeinfo[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid( winhttp_typelib, winhttp_tid_id[tid], &typeinfo );
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(winhttp_tid_id[tid]), hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer( (void **)(winhttp_typeinfo + tid), typeinfo, NULL ))
            ITypeInfo_Release( typeinfo );
    }
    *ret = winhttp_typeinfo[tid];
    ITypeInfo_AddRef(winhttp_typeinfo[tid]);
    return S_OK;
}

void release_typelib(void)
{
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(winhttp_typeinfo); i++)
        if (winhttp_typeinfo[i])
            ITypeInfo_Release(winhttp_typeinfo[i]);

    if (winhttp_typelib)
        ITypeLib_Release(winhttp_typelib);
}

static HRESULT WINAPI winhttp_request_GetTypeInfo(
    IWinHttpRequest *iface,
    UINT index,
    LCID lcid,
    ITypeInfo **info )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    TRACE("%p, %u, %u, %p\n", request, index, lcid, info);

    return get_typeinfo( IWinHttpRequest_tid, info );
}

static HRESULT WINAPI winhttp_request_GetIDsOfNames(
    IWinHttpRequest *iface,
    REFIID riid,
    LPOLESTR *names,
    UINT count,
    LCID lcid,
    DISPID *dispid )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %u, %p\n", request, debugstr_guid(riid), names, count, lcid, dispid);

    if (!names || !count || !dispid) return E_INVALIDARG;

    hr = get_typeinfo( IWinHttpRequest_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, names, count, dispid );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI winhttp_request_Invoke(
    IWinHttpRequest *iface,
    DISPID member,
    REFIID riid,
    LCID lcid,
    WORD flags,
    DISPPARAMS *params,
    VARIANT *result,
    EXCEPINFO *excep_info,
    UINT *arg_err )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %d, %s, %d, %d, %p, %p, %p, %p\n", request, member, debugstr_guid(riid),
          lcid, flags, params, result, excep_info, arg_err);

    if (!IsEqualIID( riid, &IID_NULL )) return DISP_E_UNKNOWNINTERFACE;

    if (member == DISPID_HTTPREQUEST_OPTION)
    {
        VARIANT ret_value, option;
        UINT err_pos;

        if (!result) result = &ret_value;
        if (!arg_err) arg_err = &err_pos;

        VariantInit( &option );
        VariantInit( result );

        if (!flags) return S_OK;

        if (flags == DISPATCH_PROPERTYPUT)
        {
            hr = DispGetParam( params, 0, VT_I4, &option, arg_err );
            if (FAILED(hr)) return hr;

            hr = IWinHttpRequest_put_Option( &request->IWinHttpRequest_iface, V_I4( &option ), params->rgvarg[0] );
            if (FAILED(hr))
                WARN("put_Option(%d) failed: %x\n", V_I4( &option ), hr);
            return hr;
        }
        else if (flags & (DISPATCH_PROPERTYGET | DISPATCH_METHOD))
        {
            hr = DispGetParam( params, 0, VT_I4, &option, arg_err );
            if (FAILED(hr)) return hr;

            hr = IWinHttpRequest_get_Option( &request->IWinHttpRequest_iface, V_I4( &option ), result );
            if (FAILED(hr))
                WARN("get_Option(%d) failed: %x\n", V_I4( &option ), hr);
            return hr;
        }

        FIXME("unsupported flags %x\n", flags);
        return E_NOTIMPL;
    }

    /* fallback to standard implementation */

    hr = get_typeinfo( IWinHttpRequest_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &request->IWinHttpRequest_iface, member, flags,
                               params, result, excep_info, arg_err );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI winhttp_request_SetProxy(
    IWinHttpRequest *iface,
    HTTPREQUEST_PROXY_SETTING proxy_setting,
    VARIANT proxy_server,
    VARIANT bypass_list )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS;

    TRACE("%p, %u, %s, %s\n", request, proxy_setting, debugstr_variant(&proxy_server),
          debugstr_variant(&bypass_list));

    EnterCriticalSection( &request->cs );
    switch (proxy_setting)
    {
    case HTTPREQUEST_PROXYSETTING_DEFAULT:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
        free( request->proxy.lpszProxy );
        free( request->proxy.lpszProxyBypass );
        request->proxy.lpszProxy = NULL;
        request->proxy.lpszProxyBypass = NULL;
        break;

    case HTTPREQUEST_PROXYSETTING_DIRECT:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
        free( request->proxy.lpszProxy );
        free( request->proxy.lpszProxyBypass );
        request->proxy.lpszProxy = NULL;
        request->proxy.lpszProxyBypass = NULL;
        break;

    case HTTPREQUEST_PROXYSETTING_PROXY:
        request->proxy.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        if (V_VT( &proxy_server ) == VT_BSTR)
        {
            free( request->proxy.lpszProxy );
            request->proxy.lpszProxy = strdupW( V_BSTR( &proxy_server ) );
        }
        if (V_VT( &bypass_list ) == VT_BSTR)
        {
            free( request->proxy.lpszProxyBypass );
            request->proxy.lpszProxyBypass = strdupW( V_BSTR( &bypass_list ) );
        }
        break;

    default:
        err = ERROR_INVALID_PARAMETER;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_SetCredentials(
    IWinHttpRequest *iface,
    BSTR username,
    BSTR password,
    HTTPREQUEST_SETCREDENTIALS_FLAGS flags )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD target, scheme = WINHTTP_AUTH_SCHEME_BASIC; /* FIXME: query supported schemes */
    DWORD err = ERROR_SUCCESS;

    TRACE("%p, %s, %p, 0x%08x\n", request, debugstr_w(username), password, flags);

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN;
        goto done;
    }
    switch (flags)
    {
    case HTTPREQUEST_SETCREDENTIALS_FOR_SERVER:
        target = WINHTTP_AUTH_TARGET_SERVER;
        break;
    case HTTPREQUEST_SETCREDENTIALS_FOR_PROXY:
        target = WINHTTP_AUTH_TARGET_PROXY;
        break;
    default:
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    if (!WinHttpSetCredentials( request->hrequest, target, scheme, username, password, NULL ))
    {
        err = GetLastError();
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static void initialize_request( struct winhttp_request *request )
{
    request->wait   = CreateEventW( NULL, FALSE, FALSE, NULL );
    request->cancel = CreateEventW( NULL, FALSE, FALSE, NULL );
    request->done   = CreateEventW( NULL, FALSE, FALSE, NULL );
    request->connect_timeout = 60000;
    request->send_timeout    = 30000;
    request->receive_timeout = 30000;
    request->url_codepage = CP_UTF8;
    VariantInit( &request->data );
    request->state = REQUEST_STATE_INITIALIZED;
}

static void reset_request( struct winhttp_request *request )
{
    cancel_request( request );
    WinHttpCloseHandle( request->hrequest );
    request->hrequest = NULL;
    WinHttpCloseHandle( request->hconnect );
    request->hconnect = NULL;
    free( request->buffer );
    request->buffer   = NULL;
    free( request->verb );
    request->verb     = NULL;
    request->offset   = 0;
    request->bytes_available = 0;
    request->bytes_read = 0;
    request->error    = ERROR_SUCCESS;
    request->logon_policy = 0;
    request->disable_feature = 0;
    request->async    = FALSE;
    request->connect_timeout = 60000;
    request->send_timeout    = 30000;
    request->receive_timeout = 30000;
    request->url_codepage = CP_UTF8;
    free( request->proxy.lpszProxy );
    request->proxy.lpszProxy = NULL;
    free( request->proxy.lpszProxyBypass );
    request->proxy.lpszProxyBypass = NULL;
    VariantClear( &request->data );
    request->state = REQUEST_STATE_INITIALIZED;
}

static HRESULT WINAPI winhttp_request_Open(
    IWinHttpRequest *iface,
    BSTR method,
    BSTR url,
    VARIANT async )
{
    static const WCHAR httpsW[] = {'h','t','t','p','s'};
    static const WCHAR *acceptW[] = {L"*/*", NULL};
    static const WCHAR user_agentW[] = L"Mozilla/4.0 (compatible; Win32; WinHttp.WinHttpRequest.5)";
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    URL_COMPONENTS uc;
    WCHAR *hostname, *path = NULL, *verb = NULL;
    DWORD err = ERROR_OUTOFMEMORY, len, flags = 0;

    TRACE("%p, %s, %s, %s\n", request, debugstr_w(method), debugstr_w(url),
          debugstr_variant(&async));

    if (!method || !url) return E_INVALIDARG;

    memset( &uc, 0, sizeof(uc) );
    uc.dwStructSize = sizeof(uc);
    uc.dwSchemeLength   = ~0u;
    uc.dwHostNameLength = ~0u;
    uc.dwUrlPathLength  = ~0u;
    uc.dwExtraInfoLength = ~0u;
    if (!WinHttpCrackUrl( url, 0, 0, &uc )) return HRESULT_FROM_WIN32( GetLastError() );

    EnterCriticalSection( &request->cs );
    reset_request( request );

    if (!(hostname = malloc( (uc.dwHostNameLength + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( hostname, uc.lpszHostName, uc.dwHostNameLength * sizeof(WCHAR) );
    hostname[uc.dwHostNameLength] = 0;

    if (!(path = malloc( (uc.dwUrlPathLength + uc.dwExtraInfoLength + 1) * sizeof(WCHAR) ))) goto error;
    memcpy( path, uc.lpszUrlPath, (uc.dwUrlPathLength + uc.dwExtraInfoLength) * sizeof(WCHAR) );
    path[uc.dwUrlPathLength + uc.dwExtraInfoLength] = 0;

    if (!(verb = strdupW( method ))) goto error;
    if (SUCCEEDED( VariantChangeType( &async, &async, 0, VT_BOOL )) && V_BOOL( &async )) request->async = TRUE;
    else request->async = FALSE;

    if (!request->hsession)
    {
        if (!(request->hsession = WinHttpOpen( user_agentW, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL,
                                               WINHTTP_FLAG_ASYNC )))
        {
            err = GetLastError();
            goto error;
        }
        if (!(request->hconnect = WinHttpConnect( request->hsession, hostname, uc.nPort, 0 )))
        {
            WinHttpCloseHandle( request->hsession );
            request->hsession = NULL;
            err = GetLastError();
            goto error;
        }
    }
    else if (!(request->hconnect = WinHttpConnect( request->hsession, hostname, uc.nPort, 0 )))
    {
        err = GetLastError();
        goto error;
    }

    len = ARRAY_SIZE( httpsW );
    if (uc.dwSchemeLength == len && !memcmp( uc.lpszScheme, httpsW, len * sizeof(WCHAR) ))
    {
        flags |= WINHTTP_FLAG_SECURE;
    }
    if (!(request->hrequest = WinHttpOpenRequest( request->hconnect, method, path, NULL, NULL, acceptW, flags )))
    {
        err = GetLastError();
        goto error;
    }
    WinHttpSetOption( request->hrequest, WINHTTP_OPTION_CONTEXT_VALUE, &request, sizeof(request) );

    request->state = REQUEST_STATE_OPEN;
    request->verb = verb;
    free( hostname );
    free( path );
    LeaveCriticalSection( &request->cs );
    return S_OK;

error:
    WinHttpCloseHandle( request->hconnect );
    request->hconnect = NULL;
    free( hostname );
    free( path );
    free( verb );
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_SetRequestHeader(
    IWinHttpRequest *iface,
    BSTR header,
    BSTR value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD len, err = ERROR_SUCCESS;
    WCHAR *str;

    TRACE("%p, %s, %s\n", request, debugstr_w(header), debugstr_w(value));

    if (!header) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN;
        goto done;
    }
    if (request->state >= REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND;
        goto done;
    }
    len = lstrlenW( header ) + 4;
    if (value) len += lstrlenW( value );
    if (!(str = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    swprintf( str, len + 1, L"%s: %s\r\n", header, value ? value : L"" );
    if (!WinHttpAddRequestHeaders( request->hrequest, str, len,
                                   WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE ))
    {
        err = GetLastError();
    }
    free( str );

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_GetResponseHeader(
    IWinHttpRequest *iface,
    BSTR header,
    BSTR *value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD size, err = ERROR_SUCCESS;

    TRACE("%p, %p\n", request, header);

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!header || !value)
    {
        err = ERROR_INVALID_PARAMETER;
        goto done;
    }
    size = 0;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CUSTOM, header, NULL, &size, NULL ))
    {
        err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*value = SysAllocStringLen( NULL, size / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CUSTOM, header, *value, &size, NULL ))
    {
        err = GetLastError();
        SysFreeString( *value );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_GetAllResponseHeaders(
    IWinHttpRequest *iface,
    BSTR *headers )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD size, err = ERROR_SUCCESS;

    TRACE("%p, %p\n", request, headers);

    if (!headers) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    size = 0;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &size, NULL ))
    {
        err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*headers = SysAllocStringLen( NULL, size / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, *headers, &size, NULL ))
    {
        err = GetLastError();
        SysFreeString( *headers );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static void CALLBACK wait_status_callback( HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID buffer, DWORD size )
{
    struct winhttp_request *request = (struct winhttp_request *)context;

    switch (status)
    {
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        request->bytes_available = *(DWORD *)buffer;
        request->error = ERROR_SUCCESS;
        break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        request->bytes_read = size;
        request->error = ERROR_SUCCESS;
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    {
        WINHTTP_ASYNC_RESULT *result = (WINHTTP_ASYNC_RESULT *)buffer;
        request->error = result->dwError;
        break;
    }
    default:
        request->error = ERROR_SUCCESS;
        break;
    }
    SetEvent( request->wait );
}

static void wait_set_status_callback( struct winhttp_request *request, DWORD status )
{
    status |= WINHTTP_CALLBACK_STATUS_REQUEST_ERROR;
    WinHttpSetStatusCallback( request->hrequest, wait_status_callback, status, 0 );
}

static DWORD wait_for_completion( struct winhttp_request *request )
{
    HANDLE handles[2] = { request->wait, request->cancel };
    DWORD ret;

    switch (WaitForMultipleObjects( 2, handles, FALSE, INFINITE ))
    {
    case WAIT_OBJECT_0:
        ret = request->error;
        break;
    case WAIT_OBJECT_0 + 1:
        ret = request->error = ERROR_CANCELLED;
        SetEvent( request->done );
        break;
    default:
        ret = request->error = GetLastError();
        break;
    }
    return ret;
}

static HRESULT request_receive( struct winhttp_request *request )
{
    DWORD err, size, buflen = 4096;

    wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE );
    if (!WinHttpReceiveResponse( request->hrequest, NULL ))
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    if ((err = wait_for_completion( request ))) return HRESULT_FROM_WIN32( err );
    if (!wcscmp( request->verb, L"HEAD" ))
    {
        request->state = REQUEST_STATE_RESPONSE_RECEIVED;
        return S_OK;
    }
    if (!(request->buffer = malloc( buflen ))) return E_OUTOFMEMORY;
    request->buffer[0] = 0;
    size = 0;
    do
    {
        wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE );
        if (!WinHttpQueryDataAvailable( request->hrequest, &request->bytes_available ))
        {
            err = GetLastError();
            goto error;
        }
        if ((err = wait_for_completion( request ))) goto error;
        if (!request->bytes_available) break;
        size += request->bytes_available;
        if (buflen < size)
        {
            char *tmp;
            while (buflen < size) buflen *= 2;
            if (!(tmp = realloc( request->buffer, buflen )))
            {
                err = ERROR_OUTOFMEMORY;
                goto error;
            }
            request->buffer = tmp;
        }
        wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_READ_COMPLETE );
        if (!WinHttpReadData( request->hrequest, request->buffer + request->offset,
                              request->bytes_available, &request->bytes_read ))
        {
            err = GetLastError();
            goto error;
        }
        if ((err = wait_for_completion( request ))) goto error;
        request->offset += request->bytes_read;
    } while (request->bytes_read);

    request->state = REQUEST_STATE_RESPONSE_RECEIVED;
    return S_OK;

error:
    free( request->buffer );
    request->buffer = NULL;
    return HRESULT_FROM_WIN32( err );
}

static DWORD request_set_parameters( struct winhttp_request *request )
{
    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_PROXY, &request->proxy,
                           sizeof(request->proxy) )) return GetLastError();

    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_AUTOLOGON_POLICY, &request->logon_policy,
                           sizeof(request->logon_policy) )) return GetLastError();

    if (!WinHttpSetOption( request->hrequest, WINHTTP_OPTION_DISABLE_FEATURE, &request->disable_feature,
                           sizeof(request->disable_feature) )) return GetLastError();

    if (!WinHttpSetTimeouts( request->hrequest,
                             request->resolve_timeout,
                             request->connect_timeout,
                             request->send_timeout,
                             request->receive_timeout )) return GetLastError();
    return ERROR_SUCCESS;
}

static void request_set_utf8_content_type( struct winhttp_request *request )
{
    WCHAR headerW[64];
    int len;

    len = swprintf( headerW, ARRAY_SIZE(headerW), L"%s: %s", L"Content-Type", L"text/plain" );
    WinHttpAddRequestHeaders( request->hrequest, headerW, len, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW );

    len = swprintf( headerW, ARRAY_SIZE(headerW), L"%s: %s", L"Content-Type", L"charset=utf-8" );
    WinHttpAddRequestHeaders( request->hrequest, headerW, len, WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON );
}

static HRESULT request_send( struct winhttp_request *request )
{
    SAFEARRAY *sa = NULL;
    VARIANT data;
    char *ptr = NULL;
    LONG size = 0;
    HRESULT hr;
    DWORD err;

    if ((err = request_set_parameters( request ))) return HRESULT_FROM_WIN32( err );
    if (wcscmp( request->verb, L"GET" ))
    {
        VariantInit( &data );
        if (V_VT( &request->data ) == VT_BSTR)
        {
            UINT cp = CP_ACP;
            const WCHAR *str = V_BSTR( &request->data );
            int i, len = lstrlenW( str );

            for (i = 0; i < len; i++)
            {
                if (str[i] > 127)
                {
                    cp = CP_UTF8;
                    break;
                }
            }
            size = WideCharToMultiByte( cp, 0, str, len, NULL, 0, NULL, NULL );
            if (!(ptr = malloc( size ))) return E_OUTOFMEMORY;
            WideCharToMultiByte( cp, 0, str, len, ptr, size, NULL, NULL );
            if (cp == CP_UTF8) request_set_utf8_content_type( request );
        }
        else if (VariantChangeType( &data, &request->data, 0, VT_ARRAY|VT_UI1 ) == S_OK)
        {
            sa = V_ARRAY( &data );
            if ((hr = SafeArrayAccessData( sa, (void **)&ptr )) != S_OK) return hr;
            if ((hr = SafeArrayGetUBound( sa, 1, &size )) != S_OK)
            {
                SafeArrayUnaccessData( sa );
                return hr;
            }
            size++;
        }
    }
    wait_set_status_callback( request, WINHTTP_CALLBACK_STATUS_REQUEST_SENT );
    if (!WinHttpSendRequest( request->hrequest, NULL, 0, ptr, size, size, 0 ))
    {
        err = GetLastError();
        goto error;
    }
    if ((err = wait_for_completion( request ))) goto error;
    if (sa) SafeArrayUnaccessData( sa );
    else free( ptr );
    request->state = REQUEST_STATE_SENT;
    return S_OK;

error:
    if (sa) SafeArrayUnaccessData( sa );
    else free( ptr );
    return HRESULT_FROM_WIN32( err );
}

static void CALLBACK send_and_receive_proc( TP_CALLBACK_INSTANCE *instance, void *ctx )
{
    struct winhttp_request *request = (struct winhttp_request *)ctx;
    if (request_send( request ) == S_OK) request_receive( request );
    SetEvent( request->done );
}

/* critical section must be held */
static DWORD request_wait( struct winhttp_request *request, DWORD timeout )
{
    HANDLE done = request->done;
    DWORD err, ret;

    LeaveCriticalSection( &request->cs );
    while ((err = MsgWaitForMultipleObjects( 1, &done, FALSE, timeout, QS_ALLINPUT )) == WAIT_OBJECT_0 + 1)
    {
        MSG msg;
        while (PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    }
    switch (err)
    {
    case WAIT_OBJECT_0:
        ret = request->error;
        break;
    case WAIT_TIMEOUT:
        ret = ERROR_TIMEOUT;
        break;
    default:
        ret = GetLastError();
        break;
    }
    EnterCriticalSection( &request->cs );
    if (err == WAIT_OBJECT_0) request->proc_running = FALSE;
    return ret;
}

static HRESULT WINAPI winhttp_request_Send(
    IWinHttpRequest *iface,
    VARIANT body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr;

    TRACE("%p, %s\n", request, debugstr_variant(&body));

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_OPEN)
    {
        LeaveCriticalSection( &request->cs );
        return HRESULT_FROM_WIN32( ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN );
    }
    if (request->state >= REQUEST_STATE_SENT)
    {
        LeaveCriticalSection( &request->cs );
        return S_OK;
    }
    VariantClear( &request->data );
    if ((hr = VariantCopyInd( &request->data, &body )) != S_OK)
    {
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    if (!TrySubmitThreadpoolCallback( send_and_receive_proc, request, NULL ))
    {
        LeaveCriticalSection( &request->cs );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    request->proc_running = TRUE;
    if (!request->async)
    {
        hr = HRESULT_FROM_WIN32( request_wait( request, INFINITE ) );
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_get_Status(
    IWinHttpRequest *iface,
    LONG *status )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS, flags, status_code, len = sizeof(status_code), index = 0;

    TRACE("%p, %p\n", request, status);

    if (!status) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    flags = WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER;
    if (!WinHttpQueryHeaders( request->hrequest, flags, NULL, &status_code, &len, &index ))
    {
        err = GetLastError();
        goto done;
    }
    *status = status_code;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_StatusText(
    IWinHttpRequest *iface,
    BSTR *status )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS, len = 0, index = 0;

    TRACE("%p, %p\n", request, status);

    if (!status) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_STATUS_TEXT, NULL, NULL, &len, &index ))
    {
        err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER) goto done;
    }
    if (!(*status = SysAllocStringLen( NULL, len / sizeof(WCHAR) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    index = 0;
    err = ERROR_SUCCESS;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_STATUS_TEXT, NULL, *status, &len, &index ))
    {
        err = GetLastError();
        SysFreeString( *status );
    }
done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static DWORD request_get_codepage( struct winhttp_request *request, UINT *codepage )
{
    WCHAR *buffer, *p;
    DWORD size;

    *codepage = CP_ACP;
    if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CONTENT_TYPE, NULL, NULL, &size, NULL ) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        if (!(buffer = malloc( size ))) return ERROR_OUTOFMEMORY;
        if (!WinHttpQueryHeaders( request->hrequest, WINHTTP_QUERY_CONTENT_TYPE, NULL, buffer, &size, NULL ))
        {
            return GetLastError();
        }
        if ((p = wcsstr( buffer, L"charset" )))
        {
            p += lstrlenW( L"charset" );
            while (*p == ' ') p++;
            if (*p++ == '=')
            {
                while (*p == ' ') p++;
                if (!wcsicmp( p, L"utf-8" )) *codepage = CP_UTF8;
            }
        }
        free( buffer );
    }
    return ERROR_SUCCESS;
}

static HRESULT WINAPI winhttp_request_get_ResponseText(
    IWinHttpRequest *iface,
    BSTR *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    UINT codepage;
    DWORD err = ERROR_SUCCESS;
    int len;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if ((err = request_get_codepage( request, &codepage ))) goto done;
    len = MultiByteToWideChar( codepage, 0, request->buffer, request->offset, NULL, 0 );
    if (!(*body = SysAllocStringLen( NULL, len )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    MultiByteToWideChar( codepage, 0, request->buffer, request->offset, *body, len );
    (*body)[len] = 0;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_ResponseBody(
    IWinHttpRequest *iface,
    VARIANT *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    SAFEARRAY *sa;
    HRESULT hr;
    DWORD err = ERROR_SUCCESS;
    char *ptr;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!(sa = SafeArrayCreateVector( VT_UI1, 0, request->offset )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    if ((hr = SafeArrayAccessData( sa, (void **)&ptr )) != S_OK)
    {
        SafeArrayDestroy( sa );
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    memcpy( ptr, request->buffer, request->offset );
    if ((hr = SafeArrayUnaccessData( sa )) != S_OK)
    {
        SafeArrayDestroy( sa );
        LeaveCriticalSection( &request->cs );
        return hr;
    }
    V_VT( body ) =  VT_ARRAY|VT_UI1;
    V_ARRAY( body ) = sa;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

struct stream
{
    IStream IStream_iface;
    LONG    refs;
    char   *data;
    ULARGE_INTEGER pos, size;
};

static inline struct stream *impl_from_IStream( IStream *iface )
{
    return CONTAINING_RECORD( iface, struct stream, IStream_iface );
}

static HRESULT WINAPI stream_QueryInterface( IStream *iface, REFIID riid, void **obj )
{
    struct stream *stream = impl_from_IStream( iface );

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), obj);

    if (IsEqualGUID( riid, &IID_IStream ) || IsEqualGUID( riid, &IID_IUnknown ))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IStream_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI stream_AddRef( IStream *iface )
{
    struct stream *stream = impl_from_IStream( iface );
    return InterlockedIncrement( &stream->refs );
}

static ULONG WINAPI stream_Release( IStream *iface )
{
    struct stream *stream = impl_from_IStream( iface );
    LONG refs = InterlockedDecrement( &stream->refs );
    if (!refs)
    {
        free( stream->data );
        free( stream );
    }
    return refs;
}

static HRESULT WINAPI stream_Read( IStream *iface, void *buf, ULONG len, ULONG *read )
{
    struct stream *stream = impl_from_IStream( iface );
    ULONG size;

    if (stream->pos.QuadPart >= stream->size.QuadPart)
    {
        *read = 0;
        return S_FALSE;
    }

    size = min( stream->size.QuadPart - stream->pos.QuadPart, len );
    memcpy( buf, stream->data + stream->pos.QuadPart, size );
    stream->pos.QuadPart += size;
    *read = size;

    return S_OK;
}

static HRESULT WINAPI stream_Write( IStream *iface, const void *buf, ULONG len, ULONG *written )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Seek( IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *newpos )
{
    struct stream *stream = impl_from_IStream( iface );

    if (origin == STREAM_SEEK_SET)
        stream->pos.QuadPart = move.QuadPart;
    else if (origin == STREAM_SEEK_CUR)
        stream->pos.QuadPart += move.QuadPart;
    else if (origin == STREAM_SEEK_END)
        stream->pos.QuadPart = stream->size.QuadPart - move.QuadPart;

    if (newpos) newpos->QuadPart = stream->pos.QuadPart;
    return S_OK;
}

static HRESULT WINAPI stream_SetSize( IStream *iface, ULARGE_INTEGER newsize )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_CopyTo( IStream *iface, IStream *stream, ULARGE_INTEGER len, ULARGE_INTEGER *read,
                                     ULARGE_INTEGER *written )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Commit( IStream *iface, DWORD flags )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Revert( IStream *iface )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_LockRegion( IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_UnlockRegion( IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER len, DWORD locktype )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Stat( IStream *iface, STATSTG *stg, DWORD flag )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stream_Clone( IStream *iface, IStream **stream )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const IStreamVtbl stream_vtbl =
{
    stream_QueryInterface,
    stream_AddRef,
    stream_Release,
    stream_Read,
    stream_Write,
    stream_Seek,
    stream_SetSize,
    stream_CopyTo,
    stream_Commit,
    stream_Revert,
    stream_LockRegion,
    stream_UnlockRegion,
    stream_Stat,
    stream_Clone
};

static HRESULT WINAPI winhttp_request_get_ResponseStream(
    IWinHttpRequest *iface,
    VARIANT *body )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err = ERROR_SUCCESS;
    struct stream *stream;

    TRACE("%p, %p\n", request, body);

    if (!body) return E_INVALIDARG;

    EnterCriticalSection( &request->cs );
    if (request->state < REQUEST_STATE_SENT)
    {
        err = ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND;
        goto done;
    }
    if (!(stream = malloc( sizeof(*stream) )))
    {
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    stream->IStream_iface.lpVtbl = &stream_vtbl;
    stream->refs = 1;
    if (!(stream->data = malloc( request->offset )))
    {
        free( stream );
        err = ERROR_OUTOFMEMORY;
        goto done;
    }
    memcpy( stream->data, request->buffer, request->offset );
    stream->pos.QuadPart = 0;
    stream->size.QuadPart = request->offset;
    V_VT( body ) = VT_UNKNOWN;
    V_UNKNOWN( body ) = (IUnknown *)&stream->IStream_iface;

done:
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_get_Option(
    IWinHttpRequest *iface,
    WinHttpRequestOption option,
    VARIANT *value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u, %p\n", request, option, value);

    EnterCriticalSection( &request->cs );
    switch (option)
    {
    case WinHttpRequestOption_URLCodePage:
        V_VT( value ) = VT_I4;
        V_I4( value ) = request->url_codepage;
        break;
    default:
        FIXME("unimplemented option %u\n", option);
        hr = E_NOTIMPL;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_put_Option(
    IWinHttpRequest *iface,
    WinHttpRequestOption option,
    VARIANT value )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u, %s\n", request, option, debugstr_variant(&value));

    EnterCriticalSection( &request->cs );
    switch (option)
    {
    case WinHttpRequestOption_EnableRedirects:
    {
        if (V_BOOL( &value ))
            request->disable_feature &= ~WINHTTP_DISABLE_REDIRECTS;
        else
            request->disable_feature |= WINHTTP_DISABLE_REDIRECTS;
        break;
    }
    case WinHttpRequestOption_URLCodePage:
    {
        VARIANT cp;
        VariantInit( &cp );
        hr = VariantChangeType( &cp, &value, 0, VT_UI4 );
        if (SUCCEEDED( hr ))
        {
            request->url_codepage = V_UI4( &cp );
            TRACE("URL codepage: %u\n", request->url_codepage);
        }
        else if (V_VT( &value ) == VT_BSTR && !wcsicmp( V_BSTR( &value ), L"utf-8" ))
        {
            TRACE("URL codepage: UTF-8\n");
            request->url_codepage = CP_UTF8;
            hr = S_OK;
        }
        else
            FIXME("URL codepage %s is not recognized\n", debugstr_variant( &value ));
        break;
    }
    default:
        FIXME("unimplemented option %u\n", option);
        hr = E_NOTIMPL;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static HRESULT WINAPI winhttp_request_WaitForResponse(
    IWinHttpRequest *iface,
    VARIANT timeout,
    VARIANT_BOOL *succeeded )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    DWORD err, msecs = (V_I4(&timeout) == -1) ? INFINITE : V_I4(&timeout) * 1000;

    TRACE("%p, %s, %p\n", request, debugstr_variant(&timeout), succeeded);

    EnterCriticalSection( &request->cs );
    if (request->state >= REQUEST_STATE_RESPONSE_RECEIVED)
    {
        LeaveCriticalSection( &request->cs );
        return S_OK;
    }
    switch ((err = request_wait( request, msecs )))
    {
    case ERROR_TIMEOUT:
        if (succeeded) *succeeded = VARIANT_FALSE;
        err = ERROR_SUCCESS;
        break;

    default:
        if (succeeded) *succeeded = VARIANT_TRUE;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return HRESULT_FROM_WIN32( err );
}

static HRESULT WINAPI winhttp_request_Abort(
    IWinHttpRequest *iface )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p\n", request);

    EnterCriticalSection( &request->cs );
    cancel_request( request );
    LeaveCriticalSection( &request->cs );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_SetTimeouts(
    IWinHttpRequest *iface,
    LONG resolve_timeout,
    LONG connect_timeout,
    LONG send_timeout,
    LONG receive_timeout )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );

    TRACE("%p, %d, %d, %d, %d\n", request, resolve_timeout, connect_timeout, send_timeout, receive_timeout);

    EnterCriticalSection( &request->cs );
    request->resolve_timeout = resolve_timeout;
    request->connect_timeout = connect_timeout;
    request->send_timeout    = send_timeout;
    request->receive_timeout = receive_timeout;
    LeaveCriticalSection( &request->cs );
    return S_OK;
}

static HRESULT WINAPI winhttp_request_SetClientCertificate(
    IWinHttpRequest *iface,
    BSTR certificate )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI winhttp_request_SetAutoLogonPolicy(
    IWinHttpRequest *iface,
    WinHttpRequestAutoLogonPolicy policy )
{
    struct winhttp_request *request = impl_from_IWinHttpRequest( iface );
    HRESULT hr = S_OK;

    TRACE("%p, %u\n", request, policy );

    EnterCriticalSection( &request->cs );
    switch (policy)
    {
    case AutoLogonPolicy_Always:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
        break;
    case AutoLogonPolicy_OnlyIfBypassProxy:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
        break;
    case AutoLogonPolicy_Never:
        request->logon_policy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
        break;
    default: hr = E_INVALIDARG;
        break;
    }
    LeaveCriticalSection( &request->cs );
    return hr;
}

static const struct IWinHttpRequestVtbl winhttp_request_vtbl =
{
    winhttp_request_QueryInterface,
    winhttp_request_AddRef,
    winhttp_request_Release,
    winhttp_request_GetTypeInfoCount,
    winhttp_request_GetTypeInfo,
    winhttp_request_GetIDsOfNames,
    winhttp_request_Invoke,
    winhttp_request_SetProxy,
    winhttp_request_SetCredentials,
    winhttp_request_Open,
    winhttp_request_SetRequestHeader,
    winhttp_request_GetResponseHeader,
    winhttp_request_GetAllResponseHeaders,
    winhttp_request_Send,
    winhttp_request_get_Status,
    winhttp_request_get_StatusText,
    winhttp_request_get_ResponseText,
    winhttp_request_get_ResponseBody,
    winhttp_request_get_ResponseStream,
    winhttp_request_get_Option,
    winhttp_request_put_Option,
    winhttp_request_WaitForResponse,
    winhttp_request_Abort,
    winhttp_request_SetTimeouts,
    winhttp_request_SetClientCertificate,
    winhttp_request_SetAutoLogonPolicy
};

HRESULT WinHttpRequest_create( void **obj )
{
    struct winhttp_request *request;

    TRACE("%p\n", obj);

    if (!(request = calloc( 1, sizeof(*request) ))) return E_OUTOFMEMORY;
    request->IWinHttpRequest_iface.lpVtbl = &winhttp_request_vtbl;
    request->refs = 1;
    InitializeCriticalSection( &request->cs );
    request->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": winhttp_request.cs");
    initialize_request( request );

    *obj = &request->IWinHttpRequest_iface;
    TRACE("returning iface %p\n", *obj);
    return S_OK;
}
