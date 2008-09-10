/*
 * MimeInternational tests
 *
 * Copyright 2008 Huw Davies
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windows.h"
#include "ole2.h"
#include "ocidl.h"

#include "mimeole.h"

#include "initguid.h"
#include "mlang.h"

#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

static void test_create(void)
{
    IMimeInternational *internat, *internat2;
    HRESULT hr;
    ULONG ref;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = MimeOleGetInternat(&internat2);
    ok(hr == S_OK, "ret %08x\n", hr);

    /* test to show that the object is a singleton with
       a reference held by the dll. */
    ok(internat == internat2, "instances differ\n");
    ref = IMimeInternational_Release(internat2);
    ok(ref == 2, "got %d\n", ref);

    IMimeInternational_Release(internat);
}

static inline HRESULT get_mlang(IMultiLanguage **ml)
{
    return CoCreateInstance(&CLSID_CMultiLanguage, NULL,  CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                            &IID_IMultiLanguage, (void **)ml);
}

static HRESULT mlang_getcsetinfo(const char *charset, MIMECSETINFO *mlang_info)
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, charset, -1, NULL, 0);
    BSTR bstr = SysAllocStringLen(NULL, len - 1);
    HRESULT hr;
    IMultiLanguage *ml;

    MultiByteToWideChar(CP_ACP, 0, charset, -1, bstr, len);

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCharsetInfo(ml, bstr, mlang_info);
        IMultiLanguage_Release(ml);
    }
    SysFreeString(bstr);
    if(FAILED(hr)) hr = MIME_E_NOT_FOUND;
    return hr;
}

static HRESULT mlang_getcodepageinfo(UINT cp, MIMECPINFO *mlang_cp_info)
{
    HRESULT hr;
    IMultiLanguage *ml;

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCodePageInfo(ml, cp, mlang_cp_info);
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static HRESULT mlang_getcsetinfo_from_cp(UINT cp, CHARSETTYPE charset_type, MIMECSETINFO *mlang_info)
{
    MIMECPINFO mlang_cp_info;
    WCHAR *charset_name;
    HRESULT hr;
    IMultiLanguage *ml;

    hr = mlang_getcodepageinfo(cp, &mlang_cp_info);
    if(FAILED(hr)) return hr;

    switch(charset_type)
    {
    case CHARSET_BODY:
        charset_name = mlang_cp_info.wszBodyCharset;
        break;
    case CHARSET_HEADER:
        charset_name = mlang_cp_info.wszHeaderCharset;
        break;
    case CHARSET_WEB:
        charset_name = mlang_cp_info.wszWebCharset;
        break;
    }

    hr = get_mlang(&ml);

    if(SUCCEEDED(hr))
    {
        hr = IMultiLanguage_GetCharsetInfo(ml, charset_name, mlang_info);
        IMultiLanguage_Release(ml);
    }
    return hr;
}

static void test_charset(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    HCHARSET hcs, hcs_windows_1252, hcs_windows_1251;
    INETCSETINFO cs_info;
    MIMECSETINFO mlang_cs_info;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeInternational_FindCharset(internat, "non-existent", &hcs);
    ok(hr == MIME_E_NOT_FOUND, "got %08x\n", hr);

    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs_windows_1252);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(hcs_windows_1252 == hcs, "got different hcharsets for the same name\n");

    hr = IMimeInternational_FindCharset(internat, "windows-1251", &hcs_windows_1251);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(hcs_windows_1252 != hcs_windows_1251, "got the same hcharset for the different names\n");

    hr = IMimeInternational_GetCharsetInfo(internat, hcs_windows_1252, &cs_info);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = mlang_getcsetinfo("windows-1252", &mlang_cs_info);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(cs_info.cpiWindows == mlang_cs_info.uiCodePage, "cpiWindows %d while mlang uiCodePage %d\n",
       cs_info.cpiWindows, mlang_cs_info.uiCodePage);
    ok(cs_info.cpiInternet == mlang_cs_info.uiInternetEncoding, "cpiInternet %d while mlang uiInternetEncoding %d\n",
       cs_info.cpiInternet, mlang_cs_info.uiInternetEncoding);
    ok(cs_info.hCharset == hcs_windows_1252, "hCharset doesn't match requested\n");
    ok(!strcmp(cs_info.szName, "windows-1252"), "szName doesn't match requested\n");

    hr = IMimeInternational_GetCodePageCharset(internat, 1252, CHARSET_BODY, &hcs);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IMimeInternational_GetCharsetInfo(internat, hcs, &cs_info);
    ok(hr == S_OK, "got %08x\n", hr);

    hr = mlang_getcsetinfo_from_cp(1252, CHARSET_BODY, &mlang_cs_info);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(cs_info.cpiWindows == mlang_cs_info.uiCodePage, "cpiWindows %d while mlang uiCodePage %d\n",
       cs_info.cpiWindows, mlang_cs_info.uiCodePage);
    ok(cs_info.cpiInternet == mlang_cs_info.uiInternetEncoding, "cpiInternet %d while mlang uiInternetEncoding %d\n",
       cs_info.cpiInternet, mlang_cs_info.uiInternetEncoding);

    IMimeInternational_Release(internat);
}

static void test_defaultcharset(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    HCHARSET hcs_default, hcs;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeInternational_GetDefaultCharset(internat, &hcs_default);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = IMimeInternational_GetCodePageCharset(internat, GetACP(), CHARSET_BODY, &hcs);
    ok(hr == S_OK, "ret %08x\n", hr);
    ok(hcs_default == hcs, "Unexpected default charset\n");

    IMimeInternational_Release(internat);
}

START_TEST(mimeintl)
{
    OleInitialize(NULL);
    test_create();
    test_charset();
    test_defaultcharset();
    OleUninitialize();
}
