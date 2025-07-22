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

#include <stdio.h>
#include "windows.h"
#include "webservices.h"
#include "wine/test.h"

static void test_WsDecodeUrl(void)
{
    static const WCHAR url1[] = L"http://host";
    static const WCHAR url2[] = L"https://host";
    static const WCHAR url3[] = L"http://host:80";
    static const WCHAR url4[] = L"https://host:80";
    static const WCHAR url5[] = L"http://host/path";
    static const WCHAR url6[] = L"http://host/path?query";
    static const WCHAR url7[] = L"http://host/path?query#frag";
    static const WCHAR url8[] = L"HTTP://host";
    static const WCHAR url9[] = L"httq://host";
    static const WCHAR url10[] = L"http:";
    static const WCHAR url11[] = L"http";
    static const WCHAR url12[] = L"net.tcp://host";
    static const WCHAR url13[] = L"soap.udp://host";
    static const WCHAR url14[] = L"net.pipe://host";
    static const WCHAR url15[] = L"http:/host";
    static const WCHAR url16[] = L"http:host";
    static const WCHAR url17[] = L"http:///host";
    static const WCHAR url18[] = L"http://host/";
    static const WCHAR url19[] = L"http://host:/";
    static const WCHAR url20[] = L"http://host:65536";
    static const WCHAR url21[] = L"http://host?query";
    static const WCHAR url22[] = L"http://host#frag";
    static const WCHAR url23[] = L"http://host%202";
    static const WCHAR url24[] = L"http://host/path%202";
    static const WCHAR url25[] = L"http://host?query%202";
    static const WCHAR url26[] = L"http://host#frag%202";
    static const WCHAR url27[] = L"http://host/%c3%ab/";
    static const WCHAR url28[] = L"net.tcp://[::1]";
    static const WCHAR url29[] = L"net.tcp://[::1]:1111";
    static const struct
    {
        const WCHAR        *str;
        HRESULT             hr;
        WS_URL_SCHEME_TYPE  scheme;
        const WCHAR        *host;
        ULONG               host_len;
        USHORT              port;
        const WCHAR        *port_str;
        ULONG               port_len;
        const WCHAR        *path;
        ULONG               path_len;
        const WCHAR        *query;
        ULONG               query_len;
        const WCHAR        *fragment;
        ULONG               fragment_len;
    }
    tests[] =
    {
        { url1, S_OK, WS_URL_HTTP_SCHEME_TYPE, url1 + 7, 4, 80 },
        { url2, S_OK, WS_URL_HTTPS_SCHEME_TYPE, url2 + 8, 4, 443 },
        { url3, S_OK, WS_URL_HTTP_SCHEME_TYPE, url3 + 7, 4, 80, url3 + 12, 2 },
        { url4, S_OK, WS_URL_HTTPS_SCHEME_TYPE, url4 + 8, 4, 80, url4 + 13, 2 },
        { url5, S_OK, WS_URL_HTTP_SCHEME_TYPE, url5 + 7, 4, 80, NULL, 0, url5 + 11, 5 },
        { url6, S_OK, WS_URL_HTTP_SCHEME_TYPE, url5 + 7, 4, 80, NULL, 0, url6 + 11, 5, url6 + 17, 5 },
        { url7, S_OK, WS_URL_HTTP_SCHEME_TYPE, url5 + 7, 4, 80, NULL, 0, url7 + 11, 5, url7 + 17, 5, url7 + 23, 4 },
        { url8, S_OK, WS_URL_HTTP_SCHEME_TYPE, url1 + 7, 4, 80 },
        { url9, WS_E_INVALID_FORMAT },
        { url10, WS_E_INVALID_FORMAT },
        { url11, WS_E_INVALID_FORMAT },
        { url12, S_OK, WS_URL_NETTCP_SCHEME_TYPE, url12 + 10, 4, 808 },
        { url13, S_OK, WS_URL_SOAPUDP_SCHEME_TYPE, url13 + 11, 4, 65535 },
        { url14, S_OK, WS_URL_NETPIPE_SCHEME_TYPE, url14 + 11, 4, 65535 },
        { url15, WS_E_INVALID_FORMAT },
        { url16, WS_E_INVALID_FORMAT },
        { url17, WS_E_INVALID_FORMAT },
        { url18, S_OK, WS_URL_HTTP_SCHEME_TYPE, url18 + 7, 4, 80, NULL, 0, url18 + 11, 1 },
        { url19, S_OK, WS_URL_HTTP_SCHEME_TYPE, url19 + 7, 4, 80, NULL, 0, url19 + 12, 1 },
        { url20, WS_E_INVALID_FORMAT },
        { url21, S_OK, WS_URL_HTTP_SCHEME_TYPE, url21 + 7, 4, 80, NULL, 0, NULL, 0, url21 + 12, 5 },
        { url22, S_OK, WS_URL_HTTP_SCHEME_TYPE, url22 + 7, 4, 80, NULL, 0, NULL, 0, NULL, 0,
          url22 + 12, 4  },
        { url23, S_OK, WS_URL_HTTP_SCHEME_TYPE, L"host 2", 6, 80 },
        { url24, S_OK, WS_URL_HTTP_SCHEME_TYPE, url24 + 7, 4, 80, NULL, 0, L"/path 2", 7 },
        { url25, S_OK, WS_URL_HTTP_SCHEME_TYPE, url25 + 7, 4, 80, NULL, 0, NULL, 0, L"query 2", 7 },
        { url26, S_OK, WS_URL_HTTP_SCHEME_TYPE, url26 + 7, 4, 80, NULL, 0, NULL, 0, NULL, 0, L"frag 2", 6 },
        { url27, S_OK, WS_URL_HTTP_SCHEME_TYPE, url27 + 7, 4, 80, NULL, 0, L"/\x00eb/", 3 },
        { url28, S_OK, WS_URL_NETTCP_SCHEME_TYPE, url28 + 10, 5, 808, NULL, 0, NULL, 0 },
        { url29, S_OK, WS_URL_NETTCP_SCHEME_TYPE, url29 + 10, 5, 1111, url29 + 16, 4, NULL, 0 },
    };
    WS_HEAP *heap;
    WS_STRING str;
    WS_HTTP_URL *url;
    HRESULT hr;
    UINT i;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsDecodeUrl( NULL, 0, heap, (WS_URL **)&url, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    str.chars  = NULL;
    str.length = 0;
    hr = WsDecodeUrl( &str, 0, heap, (WS_URL **)&url, NULL );
    ok( hr == WS_E_INVALID_FORMAT, "got %#lx\n", hr );

    str.chars  = (WCHAR *)url1;
    str.length = lstrlenW( url1 );
    hr = WsDecodeUrl( &str, 0, NULL, (WS_URL **)&url, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        str.length = lstrlenW( tests[i].str );
        str.chars  = (WCHAR *)tests[i].str;
        url = NULL;
        hr = WsDecodeUrl( &str, 0, heap, (WS_URL **)&url, NULL );
        ok( hr == tests[i].hr ||
            broken(hr == WS_E_INVALID_FORMAT && str.length >= 8 && !memcmp(L"net.pipe", str.chars, 8)),
            "%u: got %#lx\n", i, hr );
        if (hr != S_OK) continue;

        ok( url->url.scheme == tests[i].scheme, "%u: got %u\n", i, url->url.scheme );
        ok( url->port == tests[i].port, "%u: got %u\n", i, url->port );

        if (tests[i].host)
        {
            ok( url->host.length == tests[i].host_len, "%u: got %lu\n", i, url->host.length );
            ok( !memcmp( url->host.chars, tests[i].host, url->host.length * sizeof(WCHAR) ),
                "%u: got %s\n", i, wine_dbgstr_wn(url->host.chars, url->host.length) );
        }
        else ok( !url->host.length, "%u: got %lu\n", i, url->host.length );

        if (tests[i].port_str)
        {
            ok( url->portAsString.length == tests[i].port_len, "%u: got %lu\n", i, url->portAsString.length );
            ok( !memcmp( url->portAsString.chars, tests[i].port_str, url->portAsString.length * sizeof(WCHAR) ),
                "%u: got %s\n", i, wine_dbgstr_wn(url->portAsString.chars, url->portAsString.length) );
        }
        else ok( !url->portAsString.length, "%u: got %lu\n", i, url->portAsString.length );

        if (tests[i].path)
        {
            ok( url->path.length == tests[i].path_len, "%u: got %lu\n", i, url->path.length );
            ok( !memcmp( url->path.chars, tests[i].path, url->path.length * sizeof(WCHAR) ),
                "%u: got %s\n", i, wine_dbgstr_wn(url->path.chars, url->path.length) );
        }
        else ok( !url->path.length, "%u: got %lu\n", i, url->path.length );

        if (tests[i].query)
        {
            ok( url->query.length == tests[i].query_len, "%u: got %lu\n", i, url->query.length );
            ok( !memcmp( url->query.chars, tests[i].query, url->query.length * sizeof(WCHAR) ),
                "%u: got %s\n", i, wine_dbgstr_wn(url->query.chars, url->query.length) );
        }
        else ok( !url->query.length, "%u: got %lu\n", i, url->query.length );

        if (tests[i].fragment)
        {
            ok( url->fragment.length == tests[i].fragment_len, "%u: got %lu\n", i, url->fragment.length );
            ok( !memcmp( url->fragment.chars, tests[i].fragment, url->fragment.length * sizeof(WCHAR) ),
                "%u: got %s\n", i, wine_dbgstr_wn(url->fragment.chars, url->fragment.length) );
        }
        else ok( !url->fragment.length, "%u: got %lu\n", i, url->fragment.length );
    }

    WsFreeHeap( heap );
}

static void test_WsEncodeUrl(void)
{
    static const WCHAR url1[] = L"http://host";
    static const WCHAR url2[] = L"http://";
    static const WCHAR url3[] = L"http:///path";
    static const WCHAR url4[] = L"http://?query";
    static const WCHAR url5[] = L"http://#frag";
    static const WCHAR url6[] = L"http://host:8080/path?query#frag";
    static const WCHAR url7[] = L"http://:8080";
    static const WCHAR url8[] = L"http://";
    static const WCHAR url9[] = L"http:///path%202";
    static const WCHAR url10[] = L"http://?query%202";
    static const WCHAR url11[] = L"http://#frag%202";
    static const WCHAR url12[] = L"http://host%202";
    static const struct
    {
        WS_URL_SCHEME_TYPE  scheme;
        const WCHAR        *host;
        ULONG               host_len;
        USHORT              port;
        const WCHAR        *port_str;
        ULONG               port_len;
        const WCHAR        *path;
        ULONG               path_len;
        const WCHAR        *query;
        ULONG               query_len;
        const WCHAR        *fragment;
        ULONG               fragment_len;
        HRESULT             hr;
        ULONG               len;
        const WCHAR        *chars;
    }
    tests[] =
    {
        { WS_URL_HTTP_SCHEME_TYPE, L"host", 4, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, S_OK, 11, url1 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, S_OK, 7, url2 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, L"/path", 5, NULL, 0, NULL, 0, S_OK, 12, url3 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, NULL, 0, L"query", 5, NULL, 0, S_OK, 13, url4 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, NULL, 0, NULL, 0, L"frag", 4, S_OK, 12, url5 },
        { WS_URL_HTTP_SCHEME_TYPE, L"host", 4, 0, L"8080", 4, L"/path", 5, L"query", 5, L"frag", 4, S_OK, 32, url6 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 8080, NULL, 0, NULL, 0, NULL, 0, NULL, 0, S_OK, 12, url7 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, L"8080", 4, NULL, 0, NULL, 0, NULL, 0, S_OK, 12, url7 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 8080, L"8080", 4, NULL, 0, NULL, 0, NULL, 0, S_OK, 12, url7 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 8181, L"8080", 4, NULL, 0, NULL, 0, NULL, 0, E_INVALIDARG, 0, NULL },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, L"65536", 5, NULL, 0, NULL, 0, NULL, 0, WS_E_INVALID_FORMAT, 0, NULL },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 80, NULL, 0, NULL, 0, NULL, 0, NULL, 0, S_OK, 7, url8 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, L"/path 2", 7, NULL, 0, NULL, 0, S_OK, 16, url9 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, NULL, 0, L"query 2", 7, NULL, 0, S_OK, 17, url10 },
        { WS_URL_HTTP_SCHEME_TYPE, NULL, 0, 0, NULL, 0, NULL, 0, NULL, 0, L"frag 2", 6, S_OK, 16, url11 },
        { WS_URL_HTTP_SCHEME_TYPE, L"host 2", 6, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, S_OK, 15, url12 },
    };
    WS_HEAP *heap;
    WS_STRING str;
    WS_HTTP_URL url;
    HRESULT hr;
    UINT i;

    hr = WsCreateHeap( 1 << 16, 0, NULL, 0, &heap, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = WsEncodeUrl( NULL, 0, heap, &str, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    memset( &url, 0, sizeof(url) );
    url.url.scheme  = WS_URL_HTTP_SCHEME_TYPE;
    url.host.chars  = (WCHAR *)L"host";
    url.host.length = 4;
    hr = WsEncodeUrl( (const WS_URL *)&url, 0, NULL, &str, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    hr = WsEncodeUrl( (const WS_URL *)&url, 0, heap, NULL, NULL );
    ok( hr == E_INVALIDARG, "got %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( tests ); i++)
    {
        memset( &url, 0, sizeof(url) );
        url.url.scheme          = tests[i].scheme;
        url.host.chars          = (WCHAR *)tests[i].host;
        url.host.length         = tests[i].host_len;
        url.port                = tests[i].port;
        url.portAsString.chars  = (WCHAR *)tests[i].port_str;
        url.portAsString.length = tests[i].port_len;
        url.path.chars          = (WCHAR *)tests[i].path;
        url.path.length         = tests[i].path_len;
        url.query.chars         = (WCHAR *)tests[i].query;
        url.query.length        = tests[i].query_len;
        url.fragment.chars      = (WCHAR *)tests[i].fragment;
        url.fragment.length     = tests[i].fragment_len;

        memset( &str, 0, sizeof(str) );
        hr = WsEncodeUrl( (const WS_URL *)&url, 0, heap, &str, NULL );
        ok( hr == tests[i].hr, "%u: got %#lx\n", i, hr );
        if (hr != S_OK) continue;

        ok( str.length == tests[i].len, "%u: got %lu\n", i, str.length );
        ok( !memcmp( str.chars, tests[i].chars, tests[i].len * sizeof(WCHAR) ),
            "%u: wrong url %s\n", i, wine_dbgstr_wn(str.chars, str.length) );
    }

    WsFreeHeap( heap );
}

START_TEST(url)
{
    test_WsDecodeUrl();
    test_WsEncodeUrl();
}
