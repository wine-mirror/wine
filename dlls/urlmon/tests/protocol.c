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

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE, called_ ## func = FALSE

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT(func) \
    ok(expect_ ##func, "unexpected call\n"); \
    expect_ ## func = FALSE; \
    called_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    ok(expect_ ##func, "unexpected call\n"); \
    called_ ## func = TRUE

#define CHECK_CALLED(func) \
    ok(called_ ## func, "expected " #func "\n"); \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportProgress_SENDINGREQUEST);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);

static HRESULT expect_hrResult;
static LPCWSTR file_name;

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

    switch(ulStatusCode) {
        case BINDSTATUS_MIMETYPEAVAILABLE:
            CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!lstrcmpW(szStatusText, text_html), "szStatusText != text/html\n");
            break;
        case BINDSTATUS_DIRECTBIND:
            CHECK_EXPECT(ReportProgress_DIRECTBIND);
            ok(szStatusText == NULL, "szStatucText != NULL\n");
            break;
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!lstrcmpW(szStatusText, file_name), "szStatusText != file_name\n");
            break;
        case BINDSTATUS_SENDINGREQUEST:
            CHECK_EXPECT(ReportProgress_SENDINGREQUEST);
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!*szStatusText, "wrong szStatusText\n");
            break;
        default:
            ok(0, "Unexpected call %ld\n", ulStatusCode);
    };

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF,
        ULONG ulProgress, ULONG ulProgressMax)
{
    CHECK_EXPECT(ReportData);

    ok(ulProgress == ulProgressMax, "ulProgress != ulProgressMax\n");
    ok(ulProgressMax == 13, "ulProgressMax=%ld, expected 13\n", ulProgressMax);
    ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION),
            "grcf = %08lx\n", grfBSCF);

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportResult(IInternetProtocolSink *iface, HRESULT hrResult,
        DWORD dwError, LPCWSTR szResult)
{
    CHECK_EXPECT(ReportResult);

    ok(hrResult == expect_hrResult, "hrResult = %08lx, expected: %08lx\n",
            hrResult, expect_hrResult);
    if(SUCCEEDED(hrResult))
        ok(dwError == ERROR_SUCCESS, "dwError = %ld, expected ERROR_SUCCESS\n", dwError);
    else
        ok(dwError != ERROR_SUCCESS, "dwError == ERROR_SUCCESS\n");
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

static IInternetProtocolSink protocol_sink = { &protocol_sink_vtbl };

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
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);

    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType,
        LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
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

static IInternetBindInfo bind_info = { &bind_info_vtbl };

static void test_priority(IInternetProtocol *protocol)
{
    IInternetPriority *priority;
    LONG pr;
    HRESULT hres;

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority, (void**)&priority);
    ok(hres == S_OK, "QueryInterface(IID_IInternetPriority) failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(pr == 0, "pr=%ld, expected 0\n", pr);

    hres = IInternetPriority_SetPriority(priority, 1);
    ok(hres == S_OK, "SetPriority failed: %08lx\n", hres);

    hres = IInternetPriority_GetPriority(priority, &pr);
    ok(hres == S_OK, "GetPriority failed: %08lx\n", hres);
    ok(pr == 1, "pr=%ld, expected 1\n", pr);

    IInternetPriority_Release(priority);
}

static void file_protocol_start(IInternetProtocol *protocol, LPCWSTR url, BOOL is_first)
{
    HRESULT hres;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_DIRECTBIND);
    if(is_first) {
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    }
    SET_EXPECT(ReportData);
    if(is_first)
        SET_EXPECT(ReportResult);
 
    expect_hrResult = S_OK;

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportProgress_DIRECTBIND);
    if(is_first) {
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);
        CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
    }
    CHECK_CALLED(ReportData);
    if(is_first)
        CHECK_CALLED(ReportResult);
}

static void test_file_protocol_url(LPCWSTR url)
{
    IInternetProtocolInfo *protocol_info;
    IUnknown *unk;
    IClassFactory *factory;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_FileProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
            "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    if(SUCCEEDED(hres)) {
        IInternetProtocol *protocol;
        BYTE buf[512];
        ULONG cb;
        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            file_protocol_start(protocol, url, TRUE);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, sizeof(buf), &cb);
            ok(hres == S_FALSE, "Read failed: %08lx expected S_FALSE\n", hres);
            ok(cb == 0, "cb=%lu expected 0\n", cb);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            file_protocol_start(protocol, url, FALSE);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_FALSE, "Read failed: %08lx\n", hres);
            hres = IInternetProtocol_LockRequest(protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);
            hres = IInternetProtocol_UnlockRequest(protocol);
            ok(hres == S_OK, "UnlockRequest failed: %08lx\n", hres);

            IInternetProtocol_Release(protocol);
        }

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            file_protocol_start(protocol, url, TRUE);
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

            IInternetProtocol_Release(protocol);
        }

        hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol, (void**)&protocol);
        ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

        if(SUCCEEDED(hres)) {
            file_protocol_start(protocol, url, TRUE);
            hres = IInternetProtocol_Terminate(protocol, 0);
            ok(hres == S_OK, "Terminate failed: %08lx\n", hres);
            hres = IInternetProtocol_Read(protocol, buf, 2, &cb);
            ok(hres == S_OK, "Read failed: %08lx\n", hres);
            ok(cb == 2, "cb=%lu expected 2\n", cb);

            IInternetProtocol_Release(protocol);
        }

        IClassFactory_Release(factory);
    }

    IUnknown_Release(unk);
}

static void test_file_protocol(void) {
    IInternetProtocol *protocol;
    WCHAR buf[MAX_PATH];
    DWORD size;
    ULONG len;
    HANDLE file;
    HRESULT hres;

    static const WCHAR index_url[] =
        {'f','i','l','e',':','i','n','d','e','x','.','h','t','m','l',0};
    static const WCHAR index_url2[] =
        {'f','i','l','e',':','/','/','i','n','d','e','x','.','h','t','m','l',0};
    static const WCHAR wszFile[] = {'f','i','l','e',':',0};
    static const WCHAR wszFile2[] = {'f','i','l','e',':','/','/','/',0};
    static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};
    static const char html_doc[] = "<HTML></HTML>";

    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;
    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    file_name = wszIndexHtml;
    test_file_protocol_url(index_url);

    memcpy(buf, wszFile, sizeof(wszFile));
    len = sizeof(wszFile)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile)/sizeof(WCHAR)-1;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile2, sizeof(wszFile2));
    len = sizeof(wszFile2)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile2)/sizeof(WCHAR)-1;
    test_file_protocol_url(buf);

    DeleteFileW(wszIndexHtml);

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetBindInfo);
    expect_hrResult = MK_E_SYNTAX;
    hres = IInternetProtocol_Start(protocol, wszIndexHtml, &protocol_sink, &bind_info, 0, 0);
    ok(hres == MK_E_SYNTAX, "Start failed: %08lx, expected MK_E_SYNTAX\n", hres);
    CHECK_CALLED(GetBindInfo);

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;
    hres = IInternetProtocol_Start(protocol, index_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);

    hres = CoCreateInstance(&CLSID_FileProtocol, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IInternetProtocol, (void**)&protocol);
    ok(hres == S_OK, "CoCreateInstance failed: %08lx\n", hres);
    if(FAILED(hres))
        return;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;
    hres = IInternetProtocol_Start(protocol, index_url2, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx, expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);
}

static void test_http_protocol(void)
{
    IInternetProtocol *protocol;
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    HRESULT hres;

    hres = CoGetClassObject(&CLSID_HttpProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n", hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
        (void**)&protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);

    test_priority(protocol);

    IInternetProtocol_Release(protocol);
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    test_file_protocol();
    test_http_protocol();

    OleUninitialize();
}
