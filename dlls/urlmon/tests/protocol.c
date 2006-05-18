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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportProgress_DIRECTBIND);
DEFINE_EXPECT(ReportProgress_FINDINGRESOURCE);
DEFINE_EXPECT(ReportProgress_CONNECTING);
DEFINE_EXPECT(ReportProgress_SENDINGREQUEST);
DEFINE_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
DEFINE_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
DEFINE_EXPECT(ReportData);
DEFINE_EXPECT(ReportResult);
DEFINE_EXPECT(GetBindString_ACCEPT_MIMES);
DEFINE_EXPECT(GetBindString_USER_AGENT);
DEFINE_EXPECT(QueryService_HttpNegotiate);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(Switch);

static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};
static const WCHAR index_url[] =
    {'f','i','l','e',':','i','n','d','e','x','.','h','t','m','l',0};

static HRESULT expect_hrResult;
static LPCWSTR file_name, http_url;
static IInternetProtocol *http_protocol = NULL;
static BOOL first_data_notif = FALSE;
static HWND protocol_hwnd;
static int state = 0;
static DWORD bindf = 0;

static enum {
    FILE_TEST,
    HTTP_TEST
} tested_protocol;

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid)
            || IsEqualGUID(&IID_IHttpNegotiate, riid)
            || IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        *ppv = iface;
        return S_OK;
    }

    ok(0, "unexpected call\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    return 2;
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    return 1;
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface, LPCWSTR szURL,
        LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    CHECK_EXPECT(BeginningTransaction);

    ok(!lstrcmpW(szURL, http_url), "szURL != http_url\n");

    ok(szHeaders != NULL, "szHeaders == NULL\n");
    if(szHeaders) {
        static const WCHAR header[] =
            {'A','c','c','e','p','t','-','E','n','c','o','d','i','n','g',':',
             ' ','g','z','i','p',',',' ','d','e','f','l','a','t','e',0};
        ok(!lstrcmpW(header, szHeaders), "Unexpected szHeaders\n");
    }

    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(dwResponseCode == 200, "dwResponseCode=%ld, expected 200\n", dwResponseCode);
    ok(szResponseHeaders != NULL, "szResponseHeaders == NULL\n");
    ok(szRequestHeaders == NULL, "szRequestHeaders != NULL\n");
    ok(pszAdditionalRequestHeaders == NULL, "pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    static const BYTE sec_id[] = {'h','t','t','p',':','t','e','s','t',1,0,0,0};
    
    CHECK_EXPECT(GetRootSecurityId);

    ok(!dwReserved, "dwReserved=%ld, expected 0\n", dwReserved);
    ok(pbSecurityId != NULL, "pbSecurityId == NULL\n");
    ok(pcbSecurityId != NULL, "pcbSecurityId == NULL\n");

    if(pcbSecurityId) {
        ok(*pcbSecurityId == 512, "*pcbSecurityId=%ld, expected 512\n", *pcbSecurityId);
        *pcbSecurityId = sizeof(sec_id);
    }

    if(pbSecurityId)
        memcpy(pbSecurityId, sec_id, sizeof(sec_id));

    return E_FAIL;
}

static IHttpNegotiate2Vtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

static IHttpNegotiate2 http_negotiate = { &HttpNegotiateVtbl };

static HRESULT QueryInterface(REFIID,void**);

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return 2;
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return 1;
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHttpNegotiate, guidService) || IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        CHECK_EXPECT2(QueryService_HttpNegotiate);
        return IHttpNegotiate2_QueryInterface(&http_negotiate, riid, ppv);
    }

    ok(0, "unexpected call\n");
    return E_FAIL;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

static IServiceProvider service_provider = { &ServiceProviderVtbl };

static HRESULT WINAPI ProtocolSink_QueryInterface(IInternetProtocolSink *iface, REFIID riid, void **ppv)
{
    return QueryInterface(riid, ppv);
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
    CHECK_EXPECT2(Switch);
    ok(pProtocolData != NULL, "pProtocolData == NULL\n");
    SendMessageW(protocol_hwnd, WM_USER, 0, (LPARAM)pProtocolData);
    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportProgress(IInternetProtocolSink *iface, ULONG ulStatusCode,
        LPCWSTR szStatusText)
{
    static const WCHAR text_html[] = {'t','e','x','t','/','h','t','m','l',0};
    static const WCHAR host[] =
        {'w','w','w','.','w','i','n','e','h','q','.','o','r','g',0};
    static const WCHAR wszWineHQIP[] =
        {'2','0','9','.','3','2','.','1','4','1','.','3',0};
    /* I'm not sure if it's a good idea to hardcode here the IP address... */

    switch(ulStatusCode) {
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, text_html), "szStatusText != text/html\n");
        break;
    case BINDSTATUS_DIRECTBIND:
        CHECK_EXPECT2(ReportProgress_DIRECTBIND);
        ok(szStatusText == NULL, "szStatusText != NULL\n");
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        CHECK_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, file_name), "szStatusText != file_name\n");
        break;
    case BINDSTATUS_FINDINGRESOURCE:
        CHECK_EXPECT(ReportProgress_FINDINGRESOURCE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, host), "szStatustext != \"www.winehq.org\"\n");
        break;
    case BINDSTATUS_CONNECTING:
        CHECK_EXPECT(ReportProgress_CONNECTING);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, wszWineHQIP), "Unexpected szStatusText\n");
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(ReportProgress_SENDINGREQUEST);
        if(tested_protocol == FILE_TEST) {
            ok(szStatusText != NULL, "szStatusText == NULL\n");
            if(szStatusText)
                ok(!*szStatusText, "wrong szStatusText\n");
        }else {
            ok(szStatusText == NULL, "szStatusText != NULL\n");
        }
        break;
    case BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE:
        CHECK_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, text_html), "szStatusText != text/html\n");
        break;
    default:
        ok(0, "Unexpected call %ld\n", ulStatusCode);
    };

    return S_OK;
}

static HRESULT WINAPI ProtocolSink_ReportData(IInternetProtocolSink *iface, DWORD grfBSCF,
        ULONG ulProgress, ULONG ulProgressMax)
{
    if(tested_protocol == FILE_TEST) {
        CHECK_EXPECT2(ReportData);

        ok(ulProgress == ulProgressMax, "ulProgress (%ld) != ulProgressMax (%ld)\n",
           ulProgress, ulProgressMax);
        ok(ulProgressMax == 13, "ulProgressMax=%ld, expected 13\n", ulProgressMax);
        ok(grfBSCF == (BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION),
                "grcfBSCF = %08lx\n", grfBSCF);
    }else if(tested_protocol == HTTP_TEST) {
        if(!(grfBSCF & BSCF_LASTDATANOTIFICATION))
            CHECK_EXPECT(ReportData);

        ok(ulProgress, "ulProgress == 0\n");

        if(first_data_notif) {
            ok(grfBSCF == BSCF_FIRSTDATANOTIFICATION, "grcfBSCF = %08lx\n", grfBSCF);
            first_data_notif = FALSE;
        } else {
            ok(grfBSCF == BSCF_INTERMEDIATEDATANOTIFICATION
               || grfBSCF == (BSCF_LASTDATANOTIFICATION|BSCF_INTERMEDIATEDATANOTIFICATION),
               "grcfBSCF = %08lx\n", grfBSCF);
        }
    }
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

static HRESULT QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocolSink, riid))
        *ppv = &protocol_sink;
    if(IsEqualGUID(&IID_IServiceProvider, riid))
        *ppv = &service_provider;

    if(*ppv)
        return S_OK;

    return E_NOINTERFACE;
}

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
    DWORD cbSize;

    CHECK_EXPECT(GetBindInfo);

    ok(grfBINDF != NULL, "grfBINDF == NULL\n");
    ok(pbindinfo != NULL, "pbindinfo == NULL\n");
    ok(pbindinfo->cbSize == sizeof(BINDINFO), "wrong size of pbindinfo: %ld\n", pbindinfo->cbSize);

    *grfBINDF = bindf;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static HRESULT WINAPI BindInfo_GetBindString(IInternetBindInfo *iface, ULONG ulStringType,
        LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    static const WCHAR acc_mime[] = {'*','/','*',0};
    static const WCHAR user_agent[] = {'W','i','n','e',0};

    ok(ppwzStr != NULL, "ppwzStr == NULL\n");
    ok(pcElFetched != NULL, "pcElFetched == NULL\n");

    switch(ulStringType) {
    case BINDSTRING_ACCEPT_MIMES:
        CHECK_EXPECT(GetBindString_ACCEPT_MIMES);
        ok(cEl == 256, "cEl=%ld, expected 256\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 256, "*pcElFetched=%ld, expected 256\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(acc_mime));
            memcpy(*ppwzStr, acc_mime, sizeof(acc_mime));
        }
        return S_OK;
    case BINDSTRING_USER_AGENT:
        CHECK_EXPECT(GetBindString_USER_AGENT);
        ok(cEl == 1, "cEl=%ld, expected 1\n", cEl);
        if(pcElFetched) {
            ok(*pcElFetched == 0, "*pcElFetch=%ld, expectd 0\n", *pcElFetched);
            *pcElFetched = 1;
        }
        if(ppwzStr) {
            *ppwzStr = CoTaskMemAlloc(sizeof(user_agent));
            memcpy(*ppwzStr, user_agent, sizeof(user_agent));
        }
        return S_OK;
    default:
        ok(0, "unexpected call\n");
    }

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

    hres = IInternetProtocol_QueryInterface(protocol, &IID_IInternetPriority,
                                            (void**)&priority);
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
    if(!(bindf & BINDF_FROMURLMON))
       SET_EXPECT(ReportProgress_DIRECTBIND);
    if(is_first) {
        SET_EXPECT(ReportProgress_SENDINGREQUEST);
        SET_EXPECT(ReportProgress_CACHEFILENAMEAVAILABLE);
        if(bindf & BINDF_FROMURLMON)
            SET_EXPECT(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        else
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
    }
    SET_EXPECT(ReportData);
    if(is_first)
        SET_EXPECT(ReportResult);

    expect_hrResult = S_OK;

    hres = IInternetProtocol_Start(protocol, url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == S_OK, "Start failed: %08lx\n", hres);

    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
       CHECK_CALLED(ReportProgress_DIRECTBIND);
    if(is_first) {
        CHECK_CALLED(ReportProgress_SENDINGREQUEST);
        CHECK_CALLED(ReportProgress_CACHEFILENAMEAVAILABLE);
        if(bindf & BINDF_FROMURLMON)
            CHECK_CALLED(ReportProgress_VERIFIEDMIMETYPEAVAILABLE);
        else
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

static void test_file_protocol_fail(void)
{
    IInternetProtocol *protocol;
    HRESULT hres;

    static const WCHAR index_url2[] =
        {'f','i','l','e',':','/','/','i','n','d','e','x','.','h','t','m','l',0};

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
    if(!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;
    hres = IInternetProtocol_Start(protocol, index_url, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
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
    if(!(bindf & BINDF_FROMURLMON))
        SET_EXPECT(ReportProgress_DIRECTBIND);
    SET_EXPECT(ReportProgress_SENDINGREQUEST);
    SET_EXPECT(ReportResult);
    expect_hrResult = INET_E_RESOURCE_NOT_FOUND;

    hres = IInternetProtocol_Start(protocol, index_url2, &protocol_sink, &bind_info, 0, 0);
    ok(hres == INET_E_RESOURCE_NOT_FOUND,
            "Start failed: %08lx, expected INET_E_RESOURCE_NOT_FOUND\n", hres);
    CHECK_CALLED(GetBindInfo);
    if(!(bindf & BINDF_FROMURLMON))
        CHECK_CALLED(ReportProgress_DIRECTBIND);
    CHECK_CALLED(ReportProgress_SENDINGREQUEST);
    CHECK_CALLED(ReportResult);

    IInternetProtocol_Release(protocol);
}

static void test_file_protocol(void) {
    WCHAR buf[MAX_PATH];
    DWORD size;
    ULONG len;
    HANDLE file;

    static const WCHAR wszFile[] = {'f','i','l','e',':',0};
    static const WCHAR wszFile2[] = {'f','i','l','e',':','/','/',0};
    static const WCHAR wszFile3[] = {'f','i','l','e',':','/','/','/',0};
    static const char html_doc[] = "<HTML></HTML>";

    tested_protocol = FILE_TEST;

    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;
    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    file_name = wszIndexHtml;
    bindf = 0;
    test_file_protocol_url(index_url);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(index_url);

    memcpy(buf, wszFile, sizeof(wszFile));
    len = sizeof(wszFile)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile)/sizeof(WCHAR)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile2, sizeof(wszFile2));
    len = sizeof(wszFile2)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile2)/sizeof(WCHAR)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    memcpy(buf, wszFile3, sizeof(wszFile3));
    len = sizeof(wszFile3)/sizeof(WCHAR)-1;
    len += GetCurrentDirectoryW(sizeof(buf)/sizeof(WCHAR)-len, buf+len);
    buf[len++] = '\\';
    memcpy(buf+len, wszIndexHtml, sizeof(wszIndexHtml));

    file_name = buf + sizeof(wszFile3)/sizeof(WCHAR)-1;
    bindf = 0;
    test_file_protocol_url(buf);
    bindf = BINDF_FROMURLMON;
    test_file_protocol_url(buf);

    DeleteFileW(wszIndexHtml);

    bindf = 0;
    test_file_protocol_fail();
    bindf = BINDF_FROMURLMON;
    test_file_protocol_fail();
}

static BOOL http_protocol_start(LPCWSTR url, BOOL is_first)
{
    HRESULT hres;

    first_data_notif = TRUE;

    SET_EXPECT(GetBindInfo);
    SET_EXPECT(GetBindString_USER_AGENT);
    SET_EXPECT(GetBindString_ACCEPT_MIMES);
    SET_EXPECT(QueryService_HttpNegotiate);
    SET_EXPECT(BeginningTransaction);
    SET_EXPECT(GetRootSecurityId);

    hres = IInternetProtocol_Start(http_protocol, url, &protocol_sink, &bind_info, 0, 0);
    todo_wine {
        ok(hres == S_OK, "Start failed: %08lx\n", hres);
    }
    if(FAILED(hres))
        return FALSE;

    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(GetBindString_USER_AGENT);
    CHECK_CALLED(GetBindString_ACCEPT_MIMES);
    CHECK_CALLED(QueryService_HttpNegotiate);
    CHECK_CALLED(BeginningTransaction);
    CHECK_CALLED(GetRootSecurityId);

    return TRUE;
}

static void test_http_protocol_url(LPCWSTR url)
{
    IInternetProtocolInfo *protocol_info;
    IClassFactory *factory;
    IUnknown *unk;
    HRESULT hres;

    http_url = url;

    hres = CoGetClassObject(&CLSID_HttpProtocol, CLSCTX_INPROC_SERVER, NULL,
            &IID_IUnknown, (void**)&unk);
    ok(hres == S_OK, "CoGetClassObject failed: %08lx\n", hres);
    if(!SUCCEEDED(hres))
        return;

    hres = IUnknown_QueryInterface(unk, &IID_IInternetProtocolInfo, (void**)&protocol_info);
    ok(hres == E_NOINTERFACE,
        "Could not get IInternetProtocolInfo interface: %08lx, expected E_NOINTERFACE\n",
        hres);

    hres = IUnknown_QueryInterface(unk, &IID_IClassFactory, (void**)&factory);
    ok(hres == S_OK, "Could not get IClassFactory interface\n");
    IUnknown_Release(unk);
    if(FAILED(hres))
        return;

    hres = IClassFactory_CreateInstance(factory, NULL, &IID_IInternetProtocol,
                                        (void**)&http_protocol);
    ok(hres == S_OK, "Could not get IInternetProtocol: %08lx\n", hres);
    if(SUCCEEDED(hres)) {
        BYTE buf[512];
        DWORD cb;
        MSG msg;

        bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_FROMURLMON;

        test_priority(http_protocol);

        SET_EXPECT(ReportProgress_FINDINGRESOURCE);
        SET_EXPECT(ReportProgress_CONNECTING);
        SET_EXPECT(ReportProgress_SENDINGREQUEST);

        if(!http_protocol_start(url, TRUE))
            return;

        hres = IInternetProtocol_Read(http_protocol, buf, 2, &cb);
        ok(hres == E_PENDING, "Read failed: %08lx, expected E_PENDING\n", hres);
        ok(!cb, "cb=%ld, expected 0\n", cb);

        SET_EXPECT(Switch);
        SET_EXPECT(ReportResult);
        expect_hrResult = S_OK;

        GetMessageW(&msg, NULL, 0, 0);

        CHECK_CALLED(Switch);
        CHECK_CALLED(ReportResult);

        IInternetProtocol_Release(http_protocol);
    }

    IClassFactory_Release(factory);
}

static void test_http_protocol(void)
{
    static const WCHAR winehq_url[] =
        {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
            'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};

    tested_protocol = HTTP_TEST;
    test_http_protocol_url(winehq_url);

}

static LRESULT WINAPI wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_USER) {
        HRESULT hres;
        DWORD cb;
        BYTE buf[3600];

        SET_EXPECT(ReportData);
        if(!state) {
            CHECK_CALLED(ReportProgress_FINDINGRESOURCE);
            CHECK_CALLED(ReportProgress_CONNECTING);
            CHECK_CALLED(ReportProgress_SENDINGREQUEST);

            SET_EXPECT(OnResponse);
            SET_EXPECT(ReportProgress_MIMETYPEAVAILABLE);
        }

        hres = IInternetProtocol_Continue(http_protocol, (PROTOCOLDATA*)lParam);
        ok(hres == S_OK, "Continue failed: %08lx\n", hres);

        CHECK_CALLED(ReportData);
        if(!state) {
            CHECK_CALLED(OnResponse);
            CHECK_CALLED(ReportProgress_MIMETYPEAVAILABLE);
        }

        do hres = IInternetProtocol_Read(http_protocol, buf, sizeof(buf), &cb);
        while(cb);

        ok(hres == S_FALSE || hres == E_PENDING, "Read failed: %08lx\n", hres);

        if(hres == S_FALSE)
            PostMessageW(protocol_hwnd, WM_USER+1, 0, 0);

        if(!state) {
            state = 1;

            hres = IInternetProtocol_LockRequest(http_protocol, 0);
            ok(hres == S_OK, "LockRequest failed: %08lx\n", hres);

            do hres = IInternetProtocol_Read(http_protocol, buf, sizeof(buf), &cb);
            while(cb);
            ok(hres == S_FALSE || hres == E_PENDING, "Read failed: %08lx\n", hres);
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static HWND create_protocol_window(void)
{
    static const WCHAR wszProtocolWindow[] =
        {'P','r','o','t','o','c','o','l','W','i','n','d','o','w',0};
    static WNDCLASSEXW wndclass = {
        sizeof(WNDCLASSEXW),
        0,
        wnd_proc,
        0, 0, NULL, NULL, NULL, NULL, NULL,
        wszProtocolWindow,
        NULL
    };

    RegisterClassExW(&wndclass);
    return CreateWindowW(wszProtocolWindow, wszProtocolWindow,
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                         CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

START_TEST(protocol)
{
    OleInitialize(NULL);

    protocol_hwnd = create_protocol_window();

    test_file_protocol();
    test_http_protocol();

    DestroyWindow(protocol_hwnd);

    OleUninitialize();
}
