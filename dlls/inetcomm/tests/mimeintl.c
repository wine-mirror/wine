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

    IMimeInternational_Release(internat);
}

START_TEST(mimeintl)
{
    OleInitialize(NULL);
    test_create();
    test_charset();
    OleUninitialize();
}
