/*
 * Unit tests to document InternetShortcut's behaviour
 *
 * Copyright 2008 Damjan Jovanovic
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"

#include "shlobj.h"
#include "shobjidl.h"
#include "shlguid.h"
#include "ole2.h"
#include "mshtml.h"
#include "initguid.h"
#include "isguids.h"
#include "intshcut.h"

#include "wine/test.h"

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *pUnknown, REFIID riid, void **ppvObject)
{
    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppvObject = pUnknown;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *pUnknown)
{
    return 2;
}

static ULONG WINAPI Unknown_Release(IUnknown *pUnknown)
{
    return 1;
}

static IUnknownVtbl unknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

static IUnknown unknown = {
    &unknownVtbl
};

static const char *printGUID(const GUID *guid)
{
    static char guidSTR[39];

    if (!guid) return NULL;

    sprintf(guidSTR, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
     guid->Data1, guid->Data2, guid->Data3,
     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return guidSTR;
}

static void test_Aggregability(void)
{
    HRESULT hr;
    IUnknown *pUnknown = NULL;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(SUCCEEDED(hr), "could not create instance of CLSID_InternetShortcut with IID_IUnknown, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&pUnknown);
    ok(SUCCEEDED(hr), "could not create instance of CLSID_InternetShortcut with IID_IUniformResourceLocatorA, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);

    hr = CoCreateInstance(&CLSID_InternetShortcut, &unknown, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    ok(FAILED(hr), "aggregation didn't fail like it should, hr = 0x%x\n", hr);
    if (pUnknown)
        IUnknown_Release(pUnknown);
}

static void can_query_interface(IUnknown *pUnknown, REFIID riid)
{
    HRESULT hr;
    IUnknown *newInterface;
    hr = IUnknown_QueryInterface(pUnknown, riid, (void**)&newInterface);
    ok(SUCCEEDED(hr), "interface %s could not be queried\n", printGUID(riid));
    if (SUCCEEDED(hr))
        IUnknown_Release(newInterface);
}

static void test_QueryInterface(void)
{
    HRESULT hr;
    IUnknown *pUnknown;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUnknown, (void**)&pUnknown);
    if (SUCCEEDED(hr))
    {
        can_query_interface(pUnknown, &IID_IUniformResourceLocatorA);
        can_query_interface(pUnknown, &IID_IUniformResourceLocatorW);
        can_query_interface(pUnknown, &IID_IPersistFile);
        IUnknown_Release(pUnknown);
    }
    else
        skip("could not create a CLSID_InternetShortcut for QueryInterface tests, hr=0x%x\n", hr);
}

static CHAR *set_and_get_url(IUniformResourceLocatorA *urlA, LPCSTR input, DWORD flags)
{
    HRESULT hr;
    hr = urlA->lpVtbl->SetURL(urlA, input, flags);
    if (SUCCEEDED(hr))
    {
        CHAR *output;
        hr = urlA->lpVtbl->GetURL(urlA, &output);
        if (SUCCEEDED(hr))
            return output;
        else
            skip("GetUrl failed, hr=0x%x\n", hr);
    }
    else
        skip("SetUrl (%s, 0x%x) failed, hr=0x%x\n", input, flags, hr);
    return NULL;
}

static void check_string_transform(IUniformResourceLocatorA *urlA, LPCSTR input, DWORD flags, LPCSTR expectedOutput)
{
    CHAR *output = set_and_get_url(urlA, input, flags);
    if (output != NULL)
    {
        ok(lstrcmpA(output, expectedOutput) == 0, "unexpected URL change %s -> %s (expected %s)\n",
            input, output, expectedOutput);
        CoTaskMemFree(output);
    }
}

static BOOL check_ie(void)
{
    IHTMLDocument5 *doc;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &IID_IHTMLDocument5, (void**)&doc);
    if(FAILED(hres))
        return FALSE;

    IHTMLDocument5_Release(doc);
    return TRUE;
}

static void test_ReadAndWriteProperties(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;
    IUniformResourceLocatorA *urlAFromFile;
    WCHAR fileNameW[MAX_PATH];
    static const WCHAR shortcutW[] = {'t','e','s','t','s','h','o','r','t','c','u','t','.','u','r','l',0};
    WCHAR iconPath[] = {'f','i','l','e',':','/','/','/','C',':','/','a','r','b','i','t','r','a','r','y','/','i','c','o','n','/','p','a','t','h',0};
    int iconIndex = 7;
    char testurl[] = "http://some/bogus/url.html";
    PROPSPEC ps[2];
    ps[0].ulKind = PRSPEC_PROPID;
    ps[0].propid = PID_IS_ICONFILE;
    ps[1].ulKind = PRSPEC_PROPID;
    ps[1].propid = PID_IS_ICONINDEX;

    /* Make sure we have a valid temporary directory */
    GetTempPathW(MAX_PATH, fileNameW);
    lstrcatW(fileNameW, shortcutW);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    if (hr == S_OK)
    {
        IPersistFile *pf;
        IPropertyStorage *pPropStgWrite;
        IPropertySetStorage *pPropSetStg;
        PROPVARIANT pv[2];

        /* We need to set a URL -- IPersistFile refuses to save without one. */
        hr = urlA->lpVtbl->SetURL(urlA, testurl, 0);
        ok(hr == S_OK, "Failed to set a URL.  hr=0x%x\n", hr);

        /* Write this shortcut out to a file so that we can test reading it in again. */
        hr = urlA->lpVtbl->QueryInterface(urlA, &IID_IPersistFile, (void **) &pf);
        ok(hr == S_OK, "Failed to get the IPersistFile for writing.  hr=0x%x\n", hr);

        hr = IPersistFile_Save(pf, fileNameW, TRUE);
        ok(hr == S_OK, "Failed to save via IPersistFile. hr=0x%x\n", hr);

        IPersistFile_Release(pf);

        pv[0].vt = VT_LPWSTR;
        pv[0].pwszVal = (void *) iconPath;
        pv[1].vt = VT_I4;
        pv[1].iVal = iconIndex;
        hr = urlA->lpVtbl->QueryInterface(urlA, &IID_IPropertySetStorage, (void **) &pPropSetStg);
        ok(hr == S_OK, "Unable to get an IPropertySetStorage, hr=0x%x\n", hr);

        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &pPropStgWrite);
        ok(hr == S_OK, "Unable to get an IPropertyStorage for writing, hr=0x%x\n", hr);

        hr = IPropertyStorage_WriteMultiple(pPropStgWrite, 2, ps, pv, 0);
        ok(hr == S_OK, "Unable to set properties, hr=0x%x\n", hr);

        hr = IPropertyStorage_Commit(pPropStgWrite, STGC_DEFAULT);
        ok(hr == S_OK, "Failed to commit properties, hr=0x%x\n", hr);

        pPropStgWrite->lpVtbl->Release(pPropStgWrite);
        urlA->lpVtbl->Release(urlA);
        IPropertySetStorage_Release(pPropSetStg);
    }
    else
        skip("could not create a CLSID_InternetShortcut for property tests, hr=0x%x\n", hr);

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlAFromFile);
    if (hr == S_OK)
    {
        IPropertySetStorage *pPropSetStg;
        IPropertyStorage *pPropStgRead;
        PROPVARIANT pvread[2];
        IPersistFile *pf;
        LPSTR url = NULL;

        /* Now read that .url file back in. */
        hr = urlAFromFile->lpVtbl->QueryInterface(urlAFromFile, &IID_IPersistFile, (void **) &pf);
        ok(hr == S_OK, "Failed to get the IPersistFile for reading.  hr=0x%x\n", hr);

        hr = IPersistFile_Load(pf, fileNameW, 0);
        ok(hr == S_OK, "Failed to load via IPersistFile. hr=0x%x\n", hr);
        IPersistFile_Release(pf);


        hr = urlAFromFile->lpVtbl->GetURL(urlAFromFile, &url);
        ok(lstrcmp(url, testurl) == 0, "Wrong url read from file: %s\n",url);


        hr = urlAFromFile->lpVtbl->QueryInterface(urlAFromFile, &IID_IPropertySetStorage, (void **) &pPropSetStg);
        ok(hr == S_OK, "Unable to get an IPropertySetStorage, hr=0x%x\n", hr);

        hr = IPropertySetStorage_Open(pPropSetStg, &FMTID_Intshcut, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStgRead);
        ok(hr == S_OK, "Unable to get an IPropertyStorage for reading, hr=0x%x\n", hr);

        hr = IPropertyStorage_ReadMultiple(pPropStgRead, 2, ps, pvread);
        ok(hr == S_OK, "Unable to read properties, hr=0x%x\n", hr);

        todo_wine /* Wine doesn't yet support setting properties after save */
        {
            ok(pvread[1].iVal == iconIndex, "Read wrong icon index: %d\n", pvread[1].iVal);

            ok(lstrcmpW(pvread[0].pwszVal, iconPath) == 0, "Wrong icon path read: %s\n",wine_dbgstr_w(pvread[0].pwszVal));
        }

        PropVariantClear(&pvread[0]);
        PropVariantClear(&pvread[1]);
        IPropertyStorage_Release(pPropStgRead);
        IPropertySetStorage_Release(pPropSetStg);
        urlAFromFile->lpVtbl->Release(urlAFromFile);
        DeleteFileW(fileNameW);
    }
    else
        skip("could not create a CLSID_InternetShortcut for property tests, hr=0x%x\n", hr);
}

static void test_NullURLs(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    if (SUCCEEDED(hr))
    {
        LPSTR url = NULL;

        hr = urlA->lpVtbl->GetURL(urlA, &url);
        ok(SUCCEEDED(hr), "getting uninitialized URL unexpectedly failed, hr=0x%x\n", hr);
        ok(url == NULL, "uninitialized URL is not NULL but %s\n", url);

        hr = urlA->lpVtbl->SetURL(urlA, NULL, 0);
        ok(SUCCEEDED(hr), "setting NULL URL unexpectedly failed, hr=0x%x\n", hr);

        hr = urlA->lpVtbl->GetURL(urlA, &url);
        ok(SUCCEEDED(hr), "getting NULL URL unexpectedly failed, hr=0x%x\n", hr);
        ok(url == NULL, "URL unexpectedly not NULL but %s\n", url);

        urlA->lpVtbl->Release(urlA);
    }
    else
        skip("could not create a CLSID_InternetShortcut for NullURL tests, hr=0x%x\n", hr);
}

static void test_SetURLFlags(void)
{
    HRESULT hr;
    IUniformResourceLocatorA *urlA;

    hr = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_ALL, &IID_IUniformResourceLocatorA, (void**)&urlA);
    if (SUCCEEDED(hr))
    {
        check_string_transform(urlA, "somerandomstring", 0, "somerandomstring");
        check_string_transform(urlA, "www.winehq.org", 0, "www.winehq.org");

        todo_wine
        {
            check_string_transform(urlA, "www.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "http://www.winehq.org/");
            check_string_transform(urlA, "ftp.winehq.org", IURL_SETURL_FL_GUESS_PROTOCOL, "ftp://ftp.winehq.org/");
        }

        urlA->lpVtbl->Release(urlA);
    }
    else
        skip("could not create a CLSID_InternetShortcut for SetUrl tests, hr=0x%x\n", hr);
}

static void test_InternetShortcut(void)
{
    if (check_ie())
    {
        test_Aggregability();
        test_QueryInterface();
        test_NullURLs();
        test_SetURLFlags();
        test_ReadAndWriteProperties();
    }
}

START_TEST(intshcut)
{
    OleInitialize(NULL);
    test_InternetShortcut();
    OleUninitialize();
}
