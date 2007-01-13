/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "urlmon.h"
#include "shlwapi.h"

#include "initguid.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        expect_ ## func = FALSE; \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func  "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

DEFINE_GUID(CLSID_ITSProtocol,0x9d148291,0xb9c8,0x11d0,0xa4,0xcc,0x00,0x00,0xf8,0x01,0x49,0xf6);

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(ReportProgress_SENDINGREQUEST);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);

static HRESULT expect_hrResult;

static enum {
    ITS_PROTOCOL,
    MK_PROTOCOL
} test_protocol;

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
    static const WCHAR blank_html[] = {'b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR text_html[] = {'t','e','x','t','/','h','t','m','l',0};
    static const WCHAR cache_file[] =
        {'t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};

    switch(ulStatusCode) {
    case BINDSTATUS_BEGINDOWNLOADDATA:
        CHECK_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(ReportProgress_SENDINGREQUEST);
        if(test_protocol == ITS_PROTOCOL)
            ok(!lstrcmpW(szStatusText, blank_html), "unexpected szStatusText\n");
        else
            ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(!lstrcmpW(szStatusText, text_html), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
        ok(!lstrcmpW(szStatusText, cache_file), "unexpected szStatusText\n");
        break;
    case BINDSTATUS_DIRECTBIND:
        CHECK_EXPECT(ReportProgress_DIRECTBIND);
        ok(!szStatusText, "szStatusText != NULL\n");
        break;
    default:
        ok(0, "unexpected ulStatusCode %d\n", ulStatusCode);
        break;
    }

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF, ULONG ulProgress,
        ULONG ulProgressMax)
{
    CHECK_EXPECT(ReportData);

    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    if(test_protocol == ITS_PROTOCOL)
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE), "grcf = %08x\n", grfBSCF);
    else
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION), "grcf = %08x\n", grfBSCF);

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult, DWORD dwError,
        LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    ok(hrResult == expect_hrResult, "expected: %08x got: %08x\n", expect_hrResult, hrResult);
    ok(dwError == 0, "dwError = %d\n", dwError);
    ok(!szResult, "szResult != NULL\n");

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
    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    if(grfBINDF)
        ok(!*grfBINDF, "*grfBINDF != 0\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %d\n", pbindinfo->cbSize);

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

static void test_protocol_fail(IInternetProtocol *protocol, LPCWSTR url, HRESULT expected_hres)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportResult);

    expect_hrResult = expected_hres;
    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == expected_hres, "expected: %08x got: %08x\n", expected_hres, hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportResult);
}

static void protocol_start(IInternetProtocol *protocol, LPCWSTR url)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    if(test_protocol == MK_PROTOCOL)
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    if(test_protocol == MK_PROTOCOL)
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
    SET_EXPECT(ReportData);
    if(test_protocol == ITS_PROTOCOL)
        SET_EXPECT(ReportProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(ReportResult);
    expect_hrResult = S_OK;

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08x\n", hres);

    CHECK_CALLED(GetBindInfo);
    if(test_protocol == MK_PROTOCOL)
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    if(test_protocol == MK_PROTOCOL)
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAIABLE);
    CHECK_CALLED(ReportData);
    if(test_protocol == ITS_PROTOCOL)
        CHECK_CALLED(ReportProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(ReportResult);
}

static void test_protocol_url(IClassFactory *factory, LPCWSTR url)
{
    IInternetProtocol *protocol;
    BYTE buf[512];
    ULONG cb, ref;
    HRESULT hres;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 13, "cb=%u expected 13\n", cb);
    ok(!memcmp(buf, "<html></html>", 13), "unexpected data\n");
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    cb = 0xdeadbeef;
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == (test_protocol == ITS_PROTOCOL ? INET_E_DATA_NOT_AVAILABLE : E_FAIL),
       "Read returned %08x\n", hres);
    ok(cb == 0xdeadbeef, "cb=%u expected 0xdeadbeef\n", cb);

    protocol_start(protocol, url);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u expected 2\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 11, "cb=%u, expected 11\n", cb);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_FALSE, "Read failed: %08x expected S_FALSE\n", hres);
    ok(cb == 0, "cb=%u expected 0\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 11, "cb=%u, expected 11\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
    if(FAILED(hres))
        return;

    protocol_start(protocol, url);
    hres = IInternetProtocol_LockRequest(protocol, 0);
    ok(hres == S_OK, "LockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u, expected 2\n", cb);
    hres = IInternetProtocol_UnlockRequest(protocol);
    ok(hres == S_OK, "UnlockRequest failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u, expected 2\n", cb);
    hres = IInternetProtocol_Terminate(protocol, 0);
    ok(hres == S_OK, "Terminate failed: %08x\n", hres);
    hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
    ok(hres == S_OK, "Read failed: %08x\n", hres);
    ok(cb == 2, "cb=%u expected 2\n", cb);
    ref = IInternetProtocol_Release(protocol);
    ok(!ref, "protocol ref=%d\n", ref);
}

static void test_its_protocol(void)
{
    IClassFactory *factory;
    IUnknown *unk;
    ULONG ref;
    HRESULT hres;

    static const WCHAR blank_url1[] = {'i','t','s',':',
        't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR blank_url2[] = {'m','S','-','i','T','s',':',
         't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR blank_url3[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
         't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url1[] =
        {'i','t','s',':','t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','.','h','t','m','l',0};
    static const WCHAR wrong_url2[] =
        {'i','t','s',':','t','e','s','.','c','h','m',':',':','b','/','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url3[] =
        {'i','t','s',':','t','e','s','t','.','c','h','m','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url4[] = {'m','k',':','@','M','S','I','T','S','t','o','r',':',
         't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
    static const WCHAR wrong_url5[] = {'f','i','l','e',':',
        't','e','s','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};

    test_protocol = ITS_PROTOCOL;

    hres = CoGetClassObject(&CLSID_ITSProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08x\n", hres);
        if(SUCCEEDED(hres)) {
            test_protocol_fail(protocol, wrong_url1, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url2, STG_E_FILENOTFOUND);
            test_protocol_fail(protocol, wrong_url3, STG_E_FILENOTFOUND);

            hres = IInternetProtocol_Start(protocol, wrong_url4, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08x, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            hres = IInternetProtocol_Start(protocol, wrong_url5, &protocol_sink, &bind_info, 0, 0);
            ok(hres == INET_E_USE_DEFAULT_PROTOCOLHANDLER,
               "Start failed: %08x, expected INET_E_USE_DEFAULT_PROTOCOLHANDLER\n", hres);

            ref = IInternetProtocol_Release(protocol);
            ok(!ref, "protocol ref=%d\n", ref);

            test_protocol_url(factory, blank_url1);
            test_protocol_url(factory, blank_url2);
            test_protocol_url(factory, blank_url3);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_mk_protocol(void)
{
    IClassFactory *cf;
    HRESULT hres;

    static const WCHAR blank_url[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
         't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};

    test_protocol = MK_PROTOCOL;

    hres = CoGetClassObject(&CLSID_MkProtocol, CLSCTX_INPROC_SERVER, NULL, &IID_IClassFactory,
                            (void**)&cf);
    ok(hres == S_OK, "CoGetClassObject failed: %08x\n", hres);
    if(!SUCCEEDED(hres))
        return;

    test_protocol_url(cf, blank_url);

    IClassFactory_Release(cf);
}

static BOOL create_chm(void)
{
    HANDLE file;
    HRSRC src;
    DWORD size;

    file = CreateFileA("test.chm", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Could not create test.chm file\n");
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    src = FindResourceA(NULL, MAKEINTRESOURCEA(60), MAKEINTRESOURCEA(60));

    WriteFile(file, LoadResource(NULL, src), SizeofResource(NULL, src), &size, NULL);
    CloseHandle(file);

    return TRUE;
}

static void delete_chm(void)
{
    BOOL ret;

    ret = DeleteFileA("test.chm");
    ok(ret, "DeleteFileA failed: %d\n", GetLastError());
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    if(!create_chm())
        return;

    test_its_protocol();
    test_mk_protocol();

    delete_chm();
    OleUninitialize();
}
