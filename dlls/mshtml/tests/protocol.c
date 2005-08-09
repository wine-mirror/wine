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

#include "initguid.h"

DEFINE_GUID(CLSID_ResProtocol, 0x3050F3BC, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);
DEFINE_GUID(CLSID_AboutProtocol, 0x3050F406, 0x98B5, 0x11CF, 0xBB,0x82, 0x00,0xAA,0x00,0xBD,0xCE,0x0B);

static BOOL expect_GetBindInfo = FALSE, called_GetBindInfo = FALSE;
static BOOL expect_ReportProgress = FALSE, called_ReportProgress = FALSE;
static BOOL expect_ReportData = FALSE, called_ReportData = FALSE;
static BOOL expect_ReportResult = FALSE, called_ReportResult = FALSE;

static HRESULT expect_hrResult;
static BOOL expect_hr_win32err = FALSE;

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid)) {
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI ProtocolSink_AddRef(IInternetProtocolSink *iface)
{
    return 2;
}

static ULONG WINAPI ProtocolSink_Release(IInternetProtocolSink *iface)
{
    return 1;
}

static HRESULT WINAPI ProtocolSink_Switch(IInternetProtocolSink *iface, PROTOCOLDATA *pProtocolData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    static const WCHAR text_html[] = {'t','e','x','t','/','h','t','m','l',0};

    ok(expect_ReportProgress, "unexpected call\n");
    ok(ulStatusCode == BINDSTATUS_MIMETYPEAVAILABLE,
            "ulStatusCode=%ld expected BINDSTATUS_MIMETYPEAVAILABLE\n", ulStatusCode);
    ok(!lstrcmpW(szStatusText, text_html), "szStatusText != text/html\n");
    expect_ReportProgress = FALSE;
    called_ReportProgress = TRUE;
    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF, ULONG ulProgress,
        ULONG ulProgressMax)
{
    ok(expect_ReportData, "unexpected call\n");
    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE),
            "grcf = %08lx\n", grfBSCF);
    expect_ReportData = FALSE;
    called_ReportData = TRUE;
    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult, DWORD dwError,
        LPCWSTR szResult)
{
    ok(expect_ReportResult, "unexpected call\n");
    if(expect_hr_win32err)
        ok((hrResult&0xffff0000) == ((FACILITY_WIN32 << 16)|0x80000000),
                "expected win32 err got: %08lx\n", hrResult);
    else
        ok(hrResult == expect_hrResult, "expected: %08lx got: %08lx\n", expect_hrResult, hrResult);
    ok(dwError == 0, "dwError = %ld\n", dwError);
    ok(!szResult, "szResult != NULL\n");
    called_ReportResult = TRUE;
    expect_ReportResult = FALSE;
    return S_OK;
}

static IInternetProtocolSinkVtbl protocol_sink_vtbl = {
    ProtocolSink_QueryInterface,
    ProtocolSink_AddRef,
    ProtocolSink_Release,
    ProtocolSink_Switch,
    ProtocolSink_ReportProgress,
    ProtocolSink_ReportData,
    ProtocolSink_ReportResult
};

static IInternetProtocolSink protocol_sink = {
    &protocol_sink_vtbl
};

static HRESULT WINAPI BindInfo_QueryInterface(IInternetBindInfo *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG WINAPI BindInfo_AddRef(IInternetBindInfo *iface)
{
    return 2;
}

static ULONG WINAPI BindInfo_Release(IInternetBindInfo *iface)
{
    return 1;
}

static HRESULT WINAPI BindInfo_GetBindInfo(IInternetBindInfo *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    ok(expect_GetBindInfo, "unexpected call\n");
    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    if(grfBINDF)
        ok(!*grfBINDF, "*grfBINDF != 0\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);
    called_GetBindInfo = TRUE;
    expect_GetBindInfo = FALSE;
    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType, LPOLESTR *ppwzStr,
        ULONG cEl, ULONG *pcElFetched)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static IInternetBindInfoVtbl bind_info_vtbl = {
    BindInfo_QueryInterface,
    BindInfo_AddRef,
    BindInfo_Release,
    BindInfo_GetBindInfo,
    BindInfo_GetBindString
};

static IInternetBindInfo bind_info = {
    &bind_info_vtbl
};

static void test_protocol_fail(IInternetProtocol *protocol, LPCWSTR url, HRESULT expected_hres,
        BOOL expect_win32err)
{
    HRESULT hres;

    expect_ReportResult = TRUE;
    expect_GetBindInfo = TRUE;
    expect_hrResult = expected_hres;
    expect_hr_win32err = expect_win32err;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    if(expect_win32err)
        ok((hres&0xffff0000) == ((FACILITY_WIN32 << 16)|0x80000000), "expected win32 err got: %08lx\n", hres);
    else
        ok(hres == expected_hres, "expected: %08lx got: %08lx\n", expected_hres, hres);
    ok(called_ReportResult, "expected ReportResult\n");
    ok(called_GetBindInfo, "expected GetBindInfo\n");
    expect_ReportResult =  called_ReportResult = FALSE;
    expect_GetBindInfo = called_GetBindInfo = FALSE;
}

static void protocol_start(IInternetProtocol *protocol, LPCWSTR url, BOOL is_mime_todo)
{
    HRESULT hres;

    expect_GetBindInfo = TRUE;
    expect_ReportResult = TRUE;
    expect_ReportProgress = TRUE;
    expect_ReportData = TRUE;
    expect_hrResult = S_OK;
    expect_hr_win32err = FALSE;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);
    ok(called_GetBindInfo, "expected GetBindInfo\n");
    if(is_mime_todo) {
        todo_wine {
            ok(called_ReportProgress, "expected ReportProgress\n");
        }
    }else {
        ok(called_ReportProgress, "expected ReportProgress\n");
    }
    ok(called_ReportData, "expected ReportData\n");
    ok(called_ReportResult, "expected ReportResult\n");
    called_GetBindInfo =  expect_GetBindInfo = FALSE;
    called_ReportProgress = expect_ReportProgress = FALSE;
    called_ReportData = expect_ReportData = FALSE;
    called_ReportResult = expect_ReportResult = FALSE;
}

static void test_res_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    static const WCHAR blank_url[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};
    static const WCHAR wrong_url1[] =
        {'m','s','h','t','m','l','.','d','l','l','/','b','l','a','n','k','.','m','t','h',0};
    static const WCHAR wrong_url2[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l',0};
    static const WCHAR wrong_url3[] =
        {'r','e','s',':','/','/','m','s','h','t','m','l','.','d','l','l','/','x','x','.','h','t','m',0};
    static const WCHAR wrong_url4[] =
        {'r','e','s',':','/','/','x','x','.','d','l','l','/','b','l','a','n','k','.','h','t','m',0};


    hres = CoGetClassObject(&CLSID_ResProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        /* TODO: test IInternetProtocol interface */
        IInternetProtocol_Release(protocol_info);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;
        BYTE buf[512];
        ULONG cb;
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            test_protocol_fail(protocol, wrong_url1, MK_E_SYNTAX, FALSE);
            test_protocol_fail(protocol, wrong_url2, MK_E_SYNTAX, FALSE);
            test_protocol_fail(protocol, wrong_url3, E_FAIL, TRUE);
            test_protocol_fail(protocol, wrong_url4, E_FAIL, TRUE);

            cb = 0xdeadbeef;
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == E_FAIL, "Read returned %08lx expected E_FAIL\n", hres);
            ok(cb == 0xdeadbeef, "cb=%lu expected 0xdeadbeef\n", cb);
    
            protocol_start(protocol, blank_url, TRUE);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx expected S_FALSE\n", hres);
            ok(cb == 0, "cb=%lu expected 0\n", cb);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            protocol_start(protocol, blank_url, TRUE);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);

            protocol_start(protocol, blank_url, TRUE);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);

            protocol_start(protocol, blank_url, TRUE);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            protocol_start(protocol, blank_url, TRUE);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);

            IInternetProtocol_Release(protocol);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_about_protocol(void)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    static const WCHAR blank_url[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
    static const WCHAR test_url[] = {'a','b','o','u','t',':','t','e','s','t',0};
    static const WCHAR res_url[] = {'r','e','s',':','b','l','a','n','k',0};
    static const WCHAR blank_html[] = {0xfeff,'<','H','T','M','L','>','<','/','H','T','M','L','>',0};
    static const WCHAR test_html[] =
        {0xfeff,'<','H','T','M','L','>','t','e','s','t','<','/','H','T','M','L','>',0};

    hres = CoGetClassObject(&CLSID_AboutProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == S_OK, "Could not get IInternetProtocolInfo interface: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        /* TODO: test IInternetProtocol interface */
        IInternetProtocol_Release(protocol_info);
    }

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;
        BYTE buf[512];
        ULONG cb;
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            protocol_start(protocol, blank_url, FALSE);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == sizeof(blank_html), "cb=%ld, expected %d\n", cb, sizeof(blank_html));
            ok(!memcmp(buf, blank_html, cb), "Readed wrong data\n");
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            protocol_start(protocol, test_url, FALSE);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == sizeof(test_html), "cb=%ld, expected %d\n", cb, sizeof(test_html));
            ok(!memcmp(buf, test_html, cb), "Readed wrong data\n");
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            protocol_start(protocol, res_url, FALSE);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == sizeof(blank_html), "cb=%ld, expected %d\n", cb, sizeof(blank_html));
            ok(!memcmp(buf, blank_html, cb), "Readed wrong data\n");
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            IInternetProtocol_Release(protocol);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    test_res_protocol();
    test_about_protocol();

    OleUninitialize();
}
