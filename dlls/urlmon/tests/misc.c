/*
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define COBJMACROS

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"

static void test_CreateFormatEnum(void)
{
    IEnumFORMATETC *fenum = NULL, *fenum2 = NULL;
    FORMATETC fetc[5];
    ULONG ul;
    HRESULT hres;

    static DVTARGETDEVICE dev = {sizeof(dev),0,0,0,0,{0}};
    static FORMATETC formatetc[] = {
        {0,&dev,0,0,0},
        {0,&dev,0,1,0},
        {0,NULL,0,2,0},
        {0,NULL,0,3,0},
        {0,NULL,0,4,0}
    };

    hres = CreateFormatEnumerator(0, formatetc, &fenum);
    ok(hres == E_FAIL, "CreateFormatEnumerator failed: %08lx, expected E_FAIL\n", hres);
    hres = CreateFormatEnumerator(0, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = CreateFormatEnumerator(5, formatetc, NULL);
    ok(hres == E_INVALIDARG, "CreateFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);


    hres = CreateFormatEnumerator(5, formatetc, &fenum);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IEnumFORMATETC_Next(fenum, 2, NULL, &ul);
    ok(hres == E_INVALIDARG, "Next failed: %08lx, expected E_INVALIDARG\n", hres);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%ld, expected 0\n", fetc[0].lindex);
    ok(fetc[1].lindex == 1, "fetc[1].lindex=%ld, expected 1\n", fetc[1].lindex);
    ok(fetc[0].ptd == &dev, "fetc[0].ptd=%p, expected %p\n", fetc[0].ptd, &dev);
    ok(ul == 2, "ul=%ld, expected 2\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08lx, expected S_FALSE\n", hres);
    ok(fetc[0].lindex == 3, "fetc[0].lindex=%ld, expected 3\n", fetc[0].lindex);
    ok(fetc[1].lindex == 4, "fetc[1].lindex=%ld, expected 4\n", fetc[1].lindex);
    ok(fetc[0].ptd == NULL, "fetc[0].ptd=%p, expected NULL\n", fetc[0].ptd);
    ok(ul == 2, "ul=%ld, expected 2\n", ul);

    hres = IEnumFORMATETC_Next(fenum, 4, fetc, &ul);
    ok(hres == S_FALSE, "Next failed: %08lx, expected S_FALSE\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);
    ul = 100;
    hres = IEnumFORMATETC_Next(fenum, 0, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(ul == 0, "ul=%ld, expected 0\n", ul);

    hres = IEnumFORMATETC_Skip(fenum, 3);
    ok(hres == S_FALSE, "Skip failed: %08lx, expected S_FALSE\n", hres);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Next(fenum, 5, fetc, NULL);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 0, "fetc[0].lindex=%ld, expected 0\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Reset(fenum);
    ok(hres == S_OK, "Reset failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Skip(fenum, 2);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, NULL);
    ok(hres == E_INVALIDARG, "Clone failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = IEnumFORMATETC_Clone(fenum, &fenum2);
    ok(hres == S_OK, "Clone failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        ok(fenum != fenum2, "fenum == fenum2\n");

        hres = IEnumFORMATETC_Next(fenum2, 2, fetc, &ul);
        ok(hres == S_OK, "Next failed: %08lx\n", hres);
        ok(fetc[0].lindex == 2, "fetc[0].lindex=%ld, expected 2\n", fetc[0].lindex);

        IEnumFORMATETC_Release(fenum2);
    }

    hres = IEnumFORMATETC_Next(fenum, 2, fetc, &ul);
    ok(hres == S_OK, "Next failed: %08lx\n", hres);
    ok(fetc[0].lindex == 2, "fetc[0].lindex=%ld, expected 2\n", fetc[0].lindex);

    hres = IEnumFORMATETC_Skip(fenum, 1);
    ok(hres == S_OK, "Skip failed: %08lx\n", hres);
    
    IEnumFORMATETC_Release(fenum);
}

static void test_RegisterFormatEnumerator(void)
{
    IBindCtx *bctx = NULL;
    IEnumFORMATETC *format = NULL, *format2 = NULL;
    IUnknown *unk = NULL;
    HRESULT hres;

    static FORMATETC formatetc = {0,NULL,0,0,0};
    static WCHAR wszEnumFORMATETC[] =
        {'_','E','n','u','m','F','O','R','M','A','T','E','T','C','_',0};

    CreateBindCtx(0, &bctx);

    hres = CreateFormatEnumerator(1, &formatetc, &format);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = RegisterFormatEnumerator(NULL, format, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);
    hres = RegisterFormatEnumerator(bctx, NULL, 0);
    ok(hres == E_INVALIDARG,
            "RegisterFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == S_OK, "GetObjectParam failed: %08lx\n", hres);
    ok(unk == (IUnknown*)format, "unk != format\n");

    hres = RevokeFormatEnumerator(NULL, format);
    ok(hres == E_INVALIDARG,
            "RevokeFormatEnumerator failed: %08lx, expected E_INVALIDARG\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);

    hres = RevokeFormatEnumerator(bctx, format);
    ok(hres == E_FAIL, "RevokeFormatEnumerator failed: %08lx, expected E_FAIL\n", hres);

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);

    hres = CreateFormatEnumerator(1, &formatetc, &format2);
    ok(hres == S_OK, "CreateFormatEnumerator failed: %08lx\n", hres);

    if(SUCCEEDED(hres)) {
        hres = RevokeFormatEnumerator(bctx, format);
        ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);

        IEnumFORMATETC_Release(format2);
    }

    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    IEnumFORMATETC_Release(format);

    hres = RegisterFormatEnumerator(bctx, format, 0);
    ok(hres == S_OK, "RegisterFormatEnumerator failed: %08lx\n", hres);
    hres = RevokeFormatEnumerator(bctx, NULL);
    ok(hres == S_OK, "RevokeFormatEnumerator failed: %08lx\n", hres);
    hres = IBindCtx_GetObjectParam(bctx, wszEnumFORMATETC, &unk);
    ok(hres == E_FAIL, "GetObjectParam failed: %08lx, expected E_FAIL\n", hres);

    IBindCtx_Release(bctx);
}

static const WCHAR url1[] = {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',
        '/','b','l','a','n','k','.','h','t','m',0};
static const WCHAR url2[] = {'i','n','d','e','x','.','h','t','m',0};
static const WCHAR url3[] = {'f','i','l','e',':','c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR url4[] = {'f','i','l','e',':','s','o','m','e','%','2','0','f','i','l','e',
        '%','2','E','j','p','g',0};
static const WCHAR url5[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g',0};

static const WCHAR url4e[] = {'f','i','l','e',':','s','o','m','e',' ','f','i','l','e',
        '.','j','p','g',0};

static const WCHAR path3[] = {'c',':','\\','I','n','d','e','x','.','h','t','m',0};
static const WCHAR path4[] = {'s','o','m','e',' ','f','i','l','e','.','j','p','g',0};

static const WCHAR wszRes[] = {'r','e','s',0};
static const WCHAR wszFile[] = {'f','i','l','e',0};
static const WCHAR wszEmpty[] = {0};

struct parse_test {
    LPCWSTR url;
    HRESULT secur_hres;
    LPCWSTR encoded_url;
    HRESULT path_hres;
    LPCWSTR path;
    LPCWSTR schema;
};

static const struct parse_test parse_tests[] = {
    {url1, S_OK,   url1,  E_INVALIDARG, NULL, wszRes},
    {url2, E_FAIL, url2,  E_INVALIDARG, NULL, wszEmpty},
    {url3, E_FAIL, url3,  S_OK, path3,        wszFile},
    {url4, E_FAIL, url4e, S_OK, path4,        wszFile}
};

static void test_CoInternetParseUrl(void)
{
    HRESULT hres;
    DWORD size;
    int i;

    static WCHAR buf[4096];

    memset(buf, 0xf0, sizeof(buf));
    hres = CoInternetParseUrl(parse_tests[0].url, PARSE_SCHEMA, 0, buf,
            3, &size, 0);
    ok(hres == E_POINTER, "schema failed: %08lx, expected E_POINTER\n", hres);

    for(i=0; i < sizeof(parse_tests)/sizeof(parse_tests[0]); i++) {
        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_SECURITY_URL, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == parse_tests[i].secur_hres, "[%d] security url failed: %08lx, expected %08lx\n",
                i, hres, parse_tests[i].secur_hres);

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_ENCODE, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == S_OK, "[%d] encoding failed: %08lx\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].encoded_url), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].encoded_url, buf), "[%d] wrong encoded url\n", i);

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_PATH_FROM_URL, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == parse_tests[i].path_hres, "[%d] path failed: %08lx\n", i, hres);
        if(parse_tests[i].path) {
            ok(size == lstrlenW(parse_tests[i].path), "[%d] wrong size\n", i);
            ok(!lstrcmpW(parse_tests[i].path, buf), "[%d] wrong path\n", i);
        }

        memset(buf, 0xf0, sizeof(buf));
        hres = CoInternetParseUrl(parse_tests[i].url, PARSE_SCHEMA, 0, buf,
                sizeof(buf)/sizeof(WCHAR), &size, 0);
        ok(hres == S_OK, "[%d] schema failed: %08lx\n", i, hres);
        ok(size == lstrlenW(parse_tests[i].schema), "[%d] wrong size\n", i);
        ok(!lstrcmpW(parse_tests[i].schema, buf), "[%d] wrong schema\n", i);
    }
}

START_TEST(misc)
{
    test_CreateFormatEnum();
    test_RegisterFormatEnumerator();
    test_CoInternetParseUrl();
}
