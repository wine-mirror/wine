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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "webservices.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "webservices_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(webservices);

static const WCHAR http[] = {'h','t','t','p'};
static const WCHAR https[] = {'h','t','t','p','s'};
static const WCHAR nettcp[] = {'n','e','t','.','t','c','p'};
static const WCHAR soapudp[] = {'s','o','a','p','.','u','d','p'};
static const WCHAR netpipe[] = {'n','e','t','.','p','i','p','e'};

static WS_URL_SCHEME_TYPE scheme_type( const WCHAR *str, ULONG len )
{
    if (len == sizeof(http)/sizeof(http[0]) && !memicmpW( str, http, sizeof(http)/sizeof(http[0]) ))
        return WS_URL_HTTP_SCHEME_TYPE;

    if (len == sizeof(https)/sizeof(https[0]) && !memicmpW( str, https, sizeof(https)/sizeof(https[0]) ))
        return WS_URL_HTTPS_SCHEME_TYPE;

    if (len == sizeof(nettcp)/sizeof(nettcp[0]) && !memicmpW( str, nettcp, sizeof(nettcp)/sizeof(nettcp[0]) ))
        return WS_URL_NETTCP_SCHEME_TYPE;

    if (len == sizeof(soapudp)/sizeof(soapudp[0]) && !memicmpW( str, soapudp, sizeof(soapudp)/sizeof(soapudp[0]) ))
        return WS_URL_SOAPUDP_SCHEME_TYPE;

    if (len == sizeof(netpipe)/sizeof(netpipe[0]) && !memicmpW( str, netpipe, sizeof(netpipe)/sizeof(netpipe[0]) ))
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

static WCHAR *url_decode( WCHAR *str, ULONG len, WS_HEAP *heap, ULONG *ret_len )
{
    WCHAR *p = str, *q, *ret;
    BOOL decode = FALSE;
    ULONG i, val;

    *ret_len = len;
    for (i = 0; i < len; i++, p++)
    {
        if ((len - i) < 3) break;
        if (p[0] == '%' && isxdigitW( p[1] ) && isxdigitW( p[2] ))
        {
            decode = TRUE;
            *ret_len -= 2;
        }
    }
    if (!decode) return str;

    if (!(q = ret = ws_alloc( heap, *ret_len * sizeof(WCHAR) ))) return NULL;
    p = str;
    while (len)
    {
        if (len >= 3 && p[0] == '%' && isxdigitW( p[1] ) && isxdigitW( p[2] ))
        {
            if (p[1] >= '0' && p[1] <= '9') val = (p[1] - '0') * 16;
            else if (p[1] >= 'a' && p[1] <= 'f') val = (p[1] - 'a') * 16;
            else val = (p[1] - 'A') * 16;

            if (p[2] >= '0' && p[2] <= '9') val += p[2] - '0';
            else if (p[1] >= 'a' && p[1] <= 'f') val += p[2] - 'a';
            else val += p[1] - 'A';

            *q++ = val;
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

/**************************************************************************
 *          WsDecodeUrl		[webservices.@]
 */
HRESULT WINAPI WsDecodeUrl( const WS_STRING *str, ULONG flags, WS_HEAP *heap, WS_URL **ret,
                            WS_ERROR *error )
{
    HRESULT hr = WS_E_QUOTA_EXCEEDED;
    WCHAR *p, *q, *decoded = NULL;
    WS_HTTP_URL *url = NULL;
    ULONG len, port = 0;

    TRACE( "%s %08x %p %p %p\n", str ? debugstr_wn(str->chars, str->length) : "null", flags,
           heap, ret, error );
    if (error) FIXME( "ignoring error parameter\n" );

    if (!str || !heap) return E_INVALIDARG;
    if (!str->length) return WS_E_INVALID_FORMAT;
    if (flags)
    {
        FIXME( "unimplemented flags %08x\n", flags );
        return E_NOTIMPL;
    }
    if (!(decoded = url_decode( str->chars, str->length, heap, &len )) ||
        !(url = ws_alloc( heap, sizeof(*url) ))) goto error;

    hr = WS_E_INVALID_FORMAT;

    p = q = decoded;
    while (len && *q != ':') { q++; len--; };
    if (*q != ':') goto error;
    if ((url->url.scheme = scheme_type( p, q - p )) == ~0u) goto error;

    if (!--len || *++q != '/') goto error;
    if (!--len || *++q != '/') goto error;

    p = ++q; len--;
    while (len && *q != '/' && *q != ':' && *q != '?' && *q != '#') { q++; len--; };
    if (q == p) goto error;
    url->host.length = q - p;
    url->host.chars  = p;

    if (len && *q == ':')
    {
        p = ++q; len--;
        while (len && isdigitW( *q ))
        {
            if ((port = port * 10 + *q - '0') > 65535) goto error;
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
    return S_OK;

error:
    if (decoded != str->chars) ws_free( heap, decoded );
    ws_free( heap, url );
    return hr;
}
