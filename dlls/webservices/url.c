/*
 * Copyright 2016 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static WS_URL_SCHEME_TYPE scheme_type( const WCHAR *str, ULONG len )
{
    if (len == ARRAY_SIZE( L"http" ) - 1 && !wcsnicmp( str, L"http", ARRAY_SIZE( L"http" ) - 1 ))
        return WS_URL_HTTP_SCHEME_TYPE;

    if (len == ARRAY_SIZE( L"https" ) - 1 && !wcsnicmp( str, L"https", ARRAY_SIZE( L"https" ) - 1 ))
        return WS_URL_HTTPS_SCHEME_TYPE;

    if (len == ARRAY_SIZE( L"net.tcp" ) - 1 && !wcsnicmp( str, L"net.tcp", ARRAY_SIZE( L"net.tcp" ) - 1 ))
        return WS_URL_NETTCP_SCHEME_TYPE;

    if (len == ARRAY_SIZE( L"soap.udp" ) - 1 && !wcsnicmp( str, L"soap.udp", ARRAY_SIZE( L"soap.udp" ) - 1 ))
        return WS_URL_SOAPUDP_SCHEME_TYPE;

    if (len == ARRAY_SIZE( L"net.pipe" ) - 1 && !wcsnicmp( str, L"net.pipe", ARRAY_SIZE( L"net.pipe" ) - 1 ))
        return WS_URL_NETPIPE_SCHEME_TYPE;

    return ~0u;
}

static USHORT default_port( WS_URL_SCHEME_TYPE scheme )
{
    switch (scheme)
    {
    case WS_URL_HTTP_SCHEME_TYPE:    return 80;
    case WS_URL_HTTPS_SCHEME_TYPE:   return 443;
    case WS_URL_NETTCP_SCHEME_TYPE:  return 808;
    case WS_URL_SOAPUDP_SCHEME_TYPE:
    case WS_URL_NETPIPE_SCHEME_TYPE: return 65535;
    default:
        ERR( "unhandled scheme %u\n", scheme );
        return 0;
    }
}

static unsigned char *strdup_utf8( const WCHAR *str, ULONG len, ULONG *ret_len )
{
    unsigned char *ret;
    *ret_len = WideCharToMultiByte( CP_UTF8, 0, str, len, NULL, 0, NULL, NULL );
    if ((ret = malloc( *ret_len )))
        WideCharToMultiByte( CP_UTF8, 0, str, len, (char *)ret, *ret_len, NULL, NULL );
    return ret;
}

static inline int url_decode_byte( char c1, char c2 )
{
    int ret;

    if (c1 >= '0' && c1 <= '9') ret = (c1 - '0') * 16;
    else if (c1 >= 'a' && c1 <= 'f') ret = (c1 - 'a' + 10) * 16;
    else if (c1 >= 'A' && c1 <= 'F') ret = (c1 - 'A' + 10) * 16;
    else return -1;

    if (c2 >= '0' && c2 <= '9') ret += c2 - '0';
    else if (c2 >= 'a' && c2 <= 'f') ret += c2 - 'a' + 10;
    else if (c2 >= 'A' && c2 <= 'F') ret += c2 - 'A' + 10;
    else return -1;

    return ret;
}

static WCHAR *url_decode( WCHAR *str, ULONG len, WS_HEAP *heap, ULONG *ret_len )
{
    WCHAR *p = str, *q, *ret;
    BOOL decode = FALSE, convert = FALSE;
    ULONG i, len_utf8, len_left;
    unsigned char *utf8, *r;
    int b;

    *ret_len = len;
    for (i = 0; i < len; i++, p++)
    {
        if ((len - i) < 3) break;
        if (p[0] == '%' && (b = url_decode_byte( p[1], p[2] )) != -1)
        {
            decode = TRUE;
            if (b > 159)
            {
                convert = TRUE;
                break;
            }
            *ret_len -= 2;
        }
    }
    if (!decode) return str;
    if (!convert)
    {
        if (!(q = ret = ws_alloc( heap, *ret_len * sizeof(WCHAR) ))) return NULL;
        p = str;
        while (len)
        {
            if (len >= 3 && p[0] == '%' && (b = url_decode_byte( p[1], p[2] )) != -1)
            {
                *q++ = b;
                p += 3;
                len -= 3;
            }
            else
            {
                *q++ = *p++;
                len -= 1;
            }
        }
        return ret;
    }

    if (!(r = utf8 = strdup_utf8( str, len, &len_utf8 ))) return NULL;
    len_left = len_utf8;
    while (len_left)
    {
        if (len_left >= 3 && r[0] == '%' && (b = url_decode_byte( r[1], r[2] )) != -1)
        {
            r[0] = b;
            len_left -= 3;
            memmove( r + 1, r + 3, len_left );
            len_utf8 -= 2;
        }
        else len_left -= 1;
        r++;
    }

    if (!(*ret_len = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, (char *)utf8,
                                          len_utf8, NULL, 0 )))
    {
        WARN( "invalid UTF-8 sequence\n" );
        free( utf8 );
        return NULL;
    }
    if ((ret = ws_alloc( heap, *ret_len * sizeof(WCHAR) )))
        MultiByteToWideChar( CP_UTF8, 0, (char *)utf8, len_utf8, ret, *ret_len );

    free( utf8 );
    return ret;
}

/**************************************************************************
 *          WsDecodeUrl		[webservices.@]
 */
HRESULT WINAPI WsDecodeUrl( const WS_STRING *str, ULONG flags, WS_HEAP *heap, WS_URL **ret, WS_ERROR *error )
{
    HRESULT hr = WS_E_QUOTA_EXCEEDED;
    WCHAR *p, *q, *decoded = NULL;
    WS_HTTP_URL *url = NULL;
    ULONG len, len_decoded, port = 0;

    TRACE( "%s %#lx %p %p %p\n", str ? debugstr_wn(str->chars, str->length) : "null", flags, heap, ret, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!str || !heap) return E_INVALIDARG;
    if (!str->length) return WS_E_INVALID_FORMAT;
    if (flags)
    {
        FIXME( "unimplemented flags %#lx\n", flags );
        return E_NOTIMPL;
    }
    if (!(decoded = url_decode( str->chars, str->length, heap, &len_decoded )) ||
        !(url = ws_alloc( heap, sizeof(*url) ))) goto done;

    hr = WS_E_INVALID_FORMAT;

    p = q = decoded;
    len = len_decoded;
    while (len && *q != ':') { q++; len--; };
    if (*q != ':') goto done;
    if ((url->url.scheme = scheme_type( p, q - p )) == ~0u) goto done;

    if (!--len || *++q != '/') goto done;
    if (!--len || *++q != '/') goto done;

    p = ++q; len--;
    if (*q == '[')
    {
        while (len && *q != ']') { q++; len--; };
        if (*q++ != ']') goto done;
    }
    else
    {
        while (len && *q != '/' && *q != ':' && *q != '?' && *q != '#') { q++; len--; };
    }
    if (q == p) goto done;
    url->host.length = q - p;
    url->host.chars  = p;

    if (len && *q == ':')
    {
        p = ++q; len--;
        while (len && '0' <= *q && *q <= '9')
        {
            if ((port = port * 10 + *q - '0') > 65535) goto done;
            q++; len--;
        };
        url->port = port;
        url->portAsString.length = q - p;
        url->portAsString.chars  = p;
    }
    if (!port)
    {
        url->port = default_port( url->url.scheme );
        url->portAsString.length = 0;
        url->portAsString.chars  = NULL;
    }

    if (len && *q == '/')
    {
        p = q;
        while (len && *q != '?') { q++; len--; };
        url->path.length = q - p;
        url->path.chars  = p;
    }
    else url->path.length = 0;

    if (len && *q == '?')
    {
        p = ++q; len--;
        while (len && *q != '#') { q++; len--; };
        url->query.length = q - p;
        url->query.chars  = p;
    }
    else url->query.length = 0;

    if (len && *q == '#')
    {
        p = ++q; len--;
        while (len && *q != '#') { q++; len--; };
        url->fragment.length = q - p;
        url->fragment.chars  = p;
    }
    else url->fragment.length = 0;

    *ret = (WS_URL *)url;
    hr = S_OK;

done:
    if (hr != S_OK)
    {
        if (decoded != str->chars) ws_free( heap, decoded, len_decoded );
        ws_free( heap, url, sizeof(*url) );
    }
    TRACE( "returning %#lx\n", hr );
    return hr;
}

static const WCHAR *scheme_str( WS_URL_SCHEME_TYPE scheme, ULONG *len )
{
    switch (scheme)
    {
    case WS_URL_HTTP_SCHEME_TYPE:
        *len = ARRAY_SIZE( L"http" ) - 1;
        return L"http";

    case WS_URL_HTTPS_SCHEME_TYPE:
        *len = ARRAY_SIZE( L"https" ) - 1;
        return L"https";

    case WS_URL_NETTCP_SCHEME_TYPE:
        *len = ARRAY_SIZE( L"net.tcp" ) - 1;
        return L"net.tcp";

    case WS_URL_SOAPUDP_SCHEME_TYPE:
        *len = ARRAY_SIZE( L"soap.udp" ) - 1;
        return L"soap.udp";

    case WS_URL_NETPIPE_SCHEME_TYPE:
        *len = ARRAY_SIZE( L"net.pipe" ) - 1;
        return L"net.pipe";

    default:
        ERR( "unhandled scheme %u\n", scheme );
        return NULL;
    }
}

static inline ULONG escape_size( unsigned char ch, const char *except )
{
    const char *p = except;
    while (*p)
    {
        if (*p == ch) return 1;
        p++;
    }
    if ((ch >= 'a' && ch <= 'z' ) || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) return 1;
    if (ch < 33 || ch > 126) return 3;
    switch (ch)
    {
    case '/':
    case '?':
    case '"':
    case '#':
    case '%':
    case '<':
    case '>':
    case '\\':
    case '[':
    case ']':
    case '^':
    case '`':
    case '{':
    case '|':
    case '}':
        return 3;
    default:
        return 1;
    }
}

static HRESULT url_encode_size( const WCHAR *str, ULONG len, const char *except, ULONG *ret_len )
{
    ULONG i, len_utf8;
    BOOL convert = FALSE;
    unsigned char *utf8;

    *ret_len = 0;
    for (i = 0; i < len; i++)
    {
        if (str[i] > 159)
        {
            convert = TRUE;
            break;
        }
        *ret_len += escape_size( str[i], except );
    }
    if (!convert) return S_OK;

    *ret_len = 0;
    if (!(utf8 = strdup_utf8( str, len, &len_utf8 ))) return E_OUTOFMEMORY;
    for (i = 0; i < len_utf8; i++) *ret_len += escape_size( utf8[i], except );
    free( utf8 );

    return S_OK;
}

static ULONG url_encode_byte( unsigned char byte, const char *except, WCHAR *buf )
{
    static const WCHAR hex[] = L"0123456789ABCDEF";
    switch (escape_size( byte, except ))
    {
    case 3:
        buf[0] = '%';
        buf[1] = hex[(byte >> 4) & 0xf];
        buf[2] = hex[byte & 0xf];
        return 3;

    case 1:
        buf[0] = byte;
        return 1;

    default:
        ERR( "unhandled escape size\n" );
        return 0;
    }

}

static HRESULT url_encode( const WCHAR *str, ULONG len, WCHAR *buf, const char *except, ULONG *ret_len )
{
    HRESULT hr = S_OK;
    ULONG i, len_utf8, len_enc;
    BOOL convert = FALSE;
    WCHAR *p = buf;
    unsigned char *utf8;

    *ret_len = 0;
    for (i = 0; i < len; i++)
    {
        if (str[i] > 159)
        {
            convert = TRUE;
            break;
        }
        len_enc = url_encode_byte( str[i], except, p );
        *ret_len += len_enc;
        p += len_enc;
    }
    if (!convert) return S_OK;

    p = buf;
    *ret_len = 0;
    if (!(utf8 = strdup_utf8( str, len, &len_utf8 ))) return E_OUTOFMEMORY;
    for (i = 0; i < len_utf8; i++)
    {
        len_enc = url_encode_byte( utf8[i], except, p );
        *ret_len += len_enc;
        p += len_enc;
    }

    free( utf8 );
    return hr;
}

/**************************************************************************
 *          WsEncodeUrl		[webservices.@]
 */
HRESULT WINAPI WsEncodeUrl( const WS_URL *base, ULONG flags, WS_HEAP *heap, WS_STRING *ret,
                            WS_ERROR *error )
{
    ULONG len = 0, len_scheme, len_enc, ret_size = 0;
    const WS_HTTP_URL *url = (const WS_HTTP_URL *)base;
    const WCHAR *scheme;
    WCHAR *str = NULL, *p, *q;
    ULONG port = 0;
    HRESULT hr = WS_E_INVALID_FORMAT;

    TRACE( "%p %#lx %p %p %p\n", base, flags, heap, ret, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!url || !heap || !ret) return E_INVALIDARG;
    if (flags)
    {
        FIXME( "unimplemented flags %#lx\n", flags );
        return E_NOTIMPL;
    }
    if (!(scheme = scheme_str( url->url.scheme, &len_scheme ))) goto done;
    len = len_scheme + 3; /* '://' */
    len += 6; /* ':65535' */

    if ((hr = url_encode_size( url->host.chars, url->host.length, "", &len_enc )) != S_OK)
        goto done;
    len += len_enc;

    if ((hr = url_encode_size( url->path.chars, url->path.length, "/", &len_enc )) != S_OK)
        goto done;
    len += len_enc;

    if ((hr = url_encode_size( url->query.chars, url->query.length, "/?", &len_enc )) != S_OK)
        goto done;
    len += len_enc + 1; /* '?' */

    if ((hr = url_encode_size( url->fragment.chars, url->fragment.length, "/?", &len_enc )) != S_OK)
        goto done;
    len += len_enc + 1; /* '#' */

    ret_size = len * sizeof(WCHAR);
    if (!(str = ws_alloc( heap, ret_size )))
    {
        hr = WS_E_QUOTA_EXCEEDED;
        goto done;
    }

    memcpy( str, scheme, len_scheme * sizeof(WCHAR) );
    p = str + len_scheme;
    p[0] = ':';
    p[1] = p[2] = '/';
    p += 3;

    if ((hr = url_encode( url->host.chars, url->host.length, p, "", &len_enc )) != S_OK)
        goto done;
    p += len_enc;

    if (url->portAsString.length)
    {
        q = url->portAsString.chars;
        len = url->portAsString.length;
        while (len && '0' <= *q && *q <= '9')
        {
            if ((port = port * 10 + *q - '0') > 65535)
            {
                hr = WS_E_INVALID_FORMAT;
                goto done;
            }
            q++; len--;
        }
        if (url->port && port != url->port)
        {
            hr = E_INVALIDARG;
            goto done;
        }
    } else port = url->port;

    if (port == default_port( url->url.scheme )) port = 0;
    if (port)
    {
        WCHAR buf[7];
        len = swprintf( buf, ARRAY_SIZE(buf), L":%u", port );
        memcpy( p, buf, len * sizeof(WCHAR) );
        p += len;
    }

    if ((hr = url_encode( url->path.chars, url->path.length, p, "/", &len_enc )) != S_OK)
        goto done;
    p += len_enc;

    if (url->query.length)
    {
        *p++ = '?';
        if ((hr = url_encode( url->query.chars, url->query.length, p, "/?", &len_enc )) != S_OK)
            goto done;
        p += len_enc;
    }

    if (url->fragment.length)
    {
        *p++ = '#';
        if ((hr = url_encode( url->fragment.chars, url->fragment.length, p, "/?", &len_enc )) != S_OK)
            goto done;
        p += len_enc;
    }

    ret->length = p - str;
    ret->chars  = str;
    hr = S_OK;

done:
    if (hr != S_OK) ws_free( heap, str, ret_size );
    TRACE( "returning %#lx\n", hr );
    return hr;
}
