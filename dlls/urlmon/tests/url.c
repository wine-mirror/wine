/*
 * UrlMon URL tests
 *
 * Copyright 2004 Kevin Koltzau
 * Copyright 2004 Jacek Caban
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

#define COBJMACROS
#define CONST_VTABLE

#include "windef.h"
#include "winbase.h"
#include "urlmon.h"
#include "wininet.h"

#include "wine/test.h"

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
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func = TRUE; \
    }while(0)

#define CHECK_CALLED(func) \
    do { \
        ok(called_ ## func, "expected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CHECK_NOT_CALLED(func) \
    do { \
        ok(!called_ ## func, "unexpected " #func "\n"); \
        expect_ ## func = called_ ## func = FALSE; \
    }while(0)

#define CLEAR_CALLED(func) \
    expect_ ## func = called_ ## func = FALSE

DEFINE_EXPECT(QueryInterface_IServiceProvider);
DEFINE_EXPECT(QueryInterface_IHttpNegotiate);
DEFINE_EXPECT(BeginningTransaction);
DEFINE_EXPECT(OnResponse);
DEFINE_EXPECT(QueryInterface_IHttpNegotiate2);
DEFINE_EXPECT(GetRootSecurityId);
DEFINE_EXPECT(GetBindInfo);
DEFINE_EXPECT(OnStartBinding);
DEFINE_EXPECT(OnProgress_FINDINGRESOURCE);
DEFINE_EXPECT(OnProgress_CONNECTING);
DEFINE_EXPECT(OnProgress_SENDINGREQUEST);
DEFINE_EXPECT(OnProgress_MIMETYPEAVAILABLE);
DEFINE_EXPECT(OnProgress_BEGINDOWNLOADDATA);
DEFINE_EXPECT(OnProgress_DOWNLOADINGDATA);
DEFINE_EXPECT(OnProgress_ENDDOWNLOADDATA);
DEFINE_EXPECT(OnStopBinding);
DEFINE_EXPECT(OnDataAvailable);
DEFINE_EXPECT(Start);
DEFINE_EXPECT(Read);
DEFINE_EXPECT(LockRequest);
DEFINE_EXPECT(Terminate);
DEFINE_EXPECT(UnlockRequest);

static const WCHAR TEST_URL_1[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.','o','r','g','/','\0'};
static const WCHAR TEST_PART_URL_1[] = {'/','t','e','s','t','/','\0'};

static const WCHAR WINE_ABOUT_URL[] = {'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q','.',
                                       'o','r','g','/','s','i','t','e','/','a','b','o','u','t',0};
static const WCHAR SHORT_RESPONSE_URL[] =
        {'h','t','t','p',':','/','/','c','r','o','s','s','o','v','e','r','.',
         'c','o','d','e','w','e','a','v','e','r','s','.','c','o','m','/',
         'p','o','s','t','t','e','s','t','.','p','h','p',0};
static const WCHAR ABOUT_BLANK[] = {'a','b','o','u','t',':','b','l','a','n','k',0};
static WCHAR INDEX_HTML[MAX_PATH];
static const WCHAR ITS_URL[] =
    {'i','t','s',':','t','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};
static const WCHAR MK_URL[] = {'m','k',':','@','M','S','I','T','S','t','o','r','e',':',
    't','e','s','t','.','c','h','m',':',':','/','b','l','a','n','k','.','h','t','m','l',0};



static const WCHAR wszIndexHtml[] = {'i','n','d','e','x','.','h','t','m','l',0};

static BOOL stopped_binding = FALSE, emulate_protocol = FALSE,
    data_available = FALSE, http_is_first = TRUE;
static DWORD read = 0, bindf = 0;
static CHAR mime_type[512];

static LPCWSTR urls[] = {
    WINE_ABOUT_URL,
    ABOUT_BLANK,
    INDEX_HTML,
    ITS_URL,
    MK_URL
};

static enum {
    HTTP_TEST,
    ABOUT_TEST,
    FILE_TEST,
    ITS_TEST,
    MK_TEST
} test_protocol;

static enum {
    BEFORE_DOWNLOAD,
    DOWNLOADING,
    END_DOWNLOAD
} download_state;

static void test_CreateURLMoniker(LPCWSTR url1, LPCWSTR url2)
{
    HRESULT hr;
    IMoniker *mon1 = NULL;
    IMoniker *mon2 = NULL;

    hr = CreateURLMoniker(NULL, url1, &mon1);
    ok(SUCCEEDED(hr), "failed to create moniker: 0x%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = CreateURLMoniker(mon1, url2, &mon2);
        ok(SUCCEEDED(hr), "failed to create moniker: 0x%08x\n", hr);
    }
    if(mon1) IMoniker_Release(mon1);
    if(mon2) IMoniker_Release(mon2);
}

static void test_create(void)
{
    test_CreateURLMoniker(TEST_URL_1, TEST_PART_URL_1);
}

static HRESULT WINAPI Protocol_QueryInterface(IInternetProtocol *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IInternetProtocol, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Protocol_AddRef(IInternetProtocol *iface)
{
    return 2;
}

static ULONG WINAPI Protocol_Release(IInternetProtocol *iface)
{
    return 1;
}

static HRESULT WINAPI Protocol_Start(IInternetProtocol *iface, LPCWSTR szUrl,
        IInternetProtocolSink *pOIProtSink, IInternetBindInfo *pOIBindInfo,
        DWORD grfPI, DWORD dwReserved)
{
    BINDINFO bindinfo, bi = {sizeof(bi), 0};
    DWORD bindf, bscf = BSCF_FIRSTDATANOTIFICATION | BSCF_LASTDATANOTIFICATION;
    WCHAR null_char = 0;
    HRESULT hres;

    static const WCHAR wszTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};

    CHECK_EXPECT(Start);

    read = 0;

    ok(szUrl && !lstrcmpW(szUrl, urls[test_protocol]), "wrong url\n");
    ok(pOIProtSink != NULL, "pOIProtSink == NULL\n");
    ok(pOIBindInfo != NULL, "pOIBindInfo == NULL\n");
    ok(grfPI == 0, "grfPI=%d, expected 0\n", grfPI);
    ok(dwReserved == 0, "dwReserved=%d, expected 0\n", dwReserved);

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);
    hres = IInternetBindInfo_GetBindInfo(pOIBindInfo, &bindf, &bindinfo);
    ok(hres == S_OK, "GetBindInfo failed: %08x\n", hres);

    if(test_protocol == FILE_TEST || test_protocol == MK_TEST) {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA
                     |BINDF_FROMURLMON),
           "bindf=%08x\n", bindf);
    }else {
        ok(bindf == (BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA|
                     BINDF_FROMURLMON|BINDF_NEEDFILE),
           "bindf=%08x\n", bindf);
    }

    ok(!memcmp(&bindinfo, &bi, sizeof(bindinfo)), "wrong bindinfo\n");

    switch(test_protocol) {
    case MK_TEST:
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_DIRECTBIND, NULL);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);

    case FILE_TEST:
    case ITS_TEST:
        SET_EXPECT(OnProgress_SENDINGREQUEST);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_SENDINGREQUEST, &null_char);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_SENDINGREQUEST) failed: %08x\n", hres);
        CHECK_CALLED(OnProgress_SENDINGREQUEST);
    default:
        break;
    }

    if(test_protocol == FILE_TEST) {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_CACHEFILENAMEAVAILABLE, &null_char);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_CACHEFILENAMEAVAILABLE) failed: %08x\n", hres);

        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE) failed: %08x\n", hres);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    }else {
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink,
                BINDSTATUS_MIMETYPEAVAILABLE, wszTextHtml);
        ok(hres == S_OK,
           "ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE) failed: %08x\n", hres);
    }

    if(test_protocol == ABOUT_TEST)
        bscf |= BSCF_DATAFULLYAVAILABLE;
    if(test_protocol == ITS_TEST)
        bscf = BSCF_FIRSTDATANOTIFICATION|BSCF_DATAFULLYAVAILABLE;

    SET_EXPECT(Read);
    if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
    SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
    SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
    SET_EXPECT(LockRequest);
    SET_EXPECT(OnDataAvailable);
    SET_EXPECT(OnStopBinding);

    hres = IInternetProtocolSink_ReportData(pOIProtSink, bscf, 13, 13);
    ok(hres == S_OK, "ReportData failed: %08x\n", hres);

    CHECK_CALLED(Read);
    if(test_protocol != FILE_TEST && test_protocol != MK_TEST)
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
    CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
    CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
    CHECK_CALLED(LockRequest);
    CHECK_CALLED(OnDataAvailable);
    CHECK_CALLED(OnStopBinding);

    if(test_protocol == ITS_TEST) {
        SET_EXPECT(Read);
        hres = IInternetProtocolSink_ReportProgress(pOIProtSink, BINDSTATUS_BEGINDOWNLOADDATA, NULL);
        ok(hres == S_OK, "ReportProgress(BINDSTATUS_BEGINDOWNLOADDATA) failed: %08x\n", hres);
        CHECK_CALLED(Read);
    }

    SET_EXPECT(Terminate);
    hres = IInternetProtocolSink_ReportResult(pOIProtSink, S_OK, 0, NULL);
    ok(hres == S_OK, "ReportResult failed: %08x\n", hres);
    CHECK_CALLED(Terminate);

    return S_OK;
}

static HRESULT WINAPI Protocol_Continue(IInternetProtocol *iface,
        PROTOCOLDATA *pProtocolData)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Abort(IInternetProtocol *iface, HRESULT hrReason,
        DWORD dwOptions)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Terminate(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(Terminate);
    ok(dwOptions == 0, "dwOptions=%d, expected 0\n", dwOptions);
    return S_OK;
}

static HRESULT WINAPI Protocol_Suspend(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Resume(IInternetProtocol *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_Read(IInternetProtocol *iface, void *pv,
        ULONG cb, ULONG *pcbRead)
{
    static const char data[] = "<HTML></HTML>";

    CHECK_EXPECT2(Read);

    if(read) {
        *pcbRead = 0;
        return S_FALSE;
    }

    ok(pv != NULL, "pv == NULL\n");
    ok(cb != 0, "cb == 0\n");
    ok(pcbRead != NULL, "pcbRead == NULL\n");
    if(pcbRead) {
        ok(*pcbRead == 0, "*pcbRead=%d, expected 0\n", *pcbRead);
        read += *pcbRead = sizeof(data)-1;
    }
    if(pv)
        memcpy(pv, data, sizeof(data));

    return S_OK;
}

static HRESULT WINAPI Protocol_Seek(IInternetProtocol *iface,
        LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI Protocol_LockRequest(IInternetProtocol *iface, DWORD dwOptions)
{
    CHECK_EXPECT(LockRequest);
    return S_OK;
}

static HRESULT WINAPI Protocol_UnlockRequest(IInternetProtocol *iface)
{
    CHECK_EXPECT(UnlockRequest);
    return S_OK;
}

static const IInternetProtocolVtbl ProtocolVtbl = {
    Protocol_QueryInterface,
    Protocol_AddRef,
    Protocol_Release,
    Protocol_Start,
    Protocol_Continue,
    Protocol_Abort,
    Protocol_Terminate,
    Protocol_Suspend,
    Protocol_Resume,
    Protocol_Read,
    Protocol_Seek,
    Protocol_LockRequest,
    Protocol_UnlockRequest
};

static IInternetProtocol Protocol = { &ProtocolVtbl };

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

    ok(!lstrcmpW(szURL, urls[test_protocol]), "szURL != urls[test_protocol]\n");
    ok(!dwReserved, "dwReserved=%d, expected 0\n", dwReserved);
    ok(pszAdditionalHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalHeaders)
        ok(*pszAdditionalHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    CHECK_EXPECT(OnResponse);

    ok(dwResponseCode == 200, "dwResponseCode=%d, expected 200\n", dwResponseCode);
    ok(szResponseHeaders != NULL, "szResponseHeaders == NULL\n");
    ok(szRequestHeaders == NULL, "szRequestHeaders != NULL\n");
    /* Note: in protocol.c tests, OnResponse pszAdditionalRequestHeaders _is_ NULL */
    ok(pszAdditionalRequestHeaders != NULL, "pszAdditionalHeaders == NULL\n");
    if(pszAdditionalRequestHeaders)
        ok(*pszAdditionalRequestHeaders == NULL, "*pszAdditionalHeaders != NULL\n");

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
        ok(*pcbSecurityId == 512, "*pcbSecurityId=%d, expected 512\n", *pcbSecurityId);
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

static IHttpNegotiate2 HttpNegotiate = { &HttpNegotiateVtbl };

static HRESULT WINAPI statusclb_QueryInterface(IBindStatusCallback *iface, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IInternetProtocol, riid)) {
        if(emulate_protocol) {
            *ppv = &Protocol;
            return S_OK;
        }else {
            return E_NOINTERFACE;
        }
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid))
    {
        CHECK_EXPECT(QueryInterface_IServiceProvider);
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate, riid))
    {
        CHECK_EXPECT(QueryInterface_IHttpNegotiate);
        *ppv = &HttpNegotiate;
        return S_OK;
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate2, riid))
    {
        CHECK_EXPECT(QueryInterface_IHttpNegotiate2);
        *ppv = &HttpNegotiate;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI statusclb_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI statusclb_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI statusclb_OnStartBinding(IBindStatusCallback *iface, DWORD dwReserved,
        IBinding *pib)
{
    HRESULT hres;
    IMoniker *mon;

    CHECK_EXPECT(OnStartBinding);

    ok(pib != NULL, "pib should not be NULL\n");

    hres = IBinding_QueryInterface(pib, &IID_IMoniker, (void**)&mon);
    ok(hres == E_NOINTERFACE, "IBinding should not have IMoniker interface\n");
    if(SUCCEEDED(hres))
        IMoniker_Release(mon);

    return S_OK;
}

static HRESULT WINAPI statusclb_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI statusclb_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    switch(ulStatusCode) {
    case BINDSTATUS_FINDINGRESOURCE:
        CHECK_EXPECT(OnProgress_FINDINGRESOURCE);
        break;
    case BINDSTATUS_CONNECTING:
        CHECK_EXPECT(OnProgress_CONNECTING);
        break;
    case BINDSTATUS_SENDINGREQUEST:
        CHECK_EXPECT(OnProgress_SENDINGREQUEST);
        break;
    case BINDSTATUS_MIMETYPEAVAILABLE:
        CHECK_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
           download_state);
        WideCharToMultiByte(CP_ACP, 0, szStatusText, -1, mime_type, sizeof(mime_type)-1, NULL, NULL);
        break;
    case BINDSTATUS_BEGINDOWNLOADDATA:
        CHECK_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText\n");
        ok(download_state == BEFORE_DOWNLOAD, "Download state was %d, expected BEFORE_DOWNLOAD\n",
           download_state);
        download_state = DOWNLOADING;
        break;
    case BINDSTATUS_DOWNLOADINGDATA:
        CHECK_EXPECT2(OnProgress_DOWNLOADINGDATA);
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        break;
    case BINDSTATUS_ENDDOWNLOADDATA:
        CHECK_EXPECT(OnProgress_ENDDOWNLOADDATA);
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText)
            ok(!lstrcmpW(szStatusText, urls[test_protocol]), "wrong szStatusText\n");
        ok(download_state == DOWNLOADING, "Download state was %d, expected DOWNLOADING\n",
           download_state);
        download_state = END_DOWNLOAD;
        break;
    case BINDSTATUS_CACHEFILENAMEAVAILABLE:
        ok(szStatusText != NULL, "szStatusText == NULL\n");
        if(szStatusText && test_protocol == FILE_TEST)
            ok(!lstrcmpW(INDEX_HTML+7, szStatusText), "wrong szStatusText\n");
        break;
    default:
        todo_wine { ok(0, "unexpexted code %d\n", ulStatusCode); }
    };
    return S_OK;
}

static HRESULT WINAPI statusclb_OnStopBinding(IBindStatusCallback *iface, HRESULT hresult, LPCWSTR szError)
{
    CHECK_EXPECT(OnStopBinding);

    /* ignore DNS failure */
    if (hresult != HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        ok(SUCCEEDED(hresult), "Download failed: %08x\n", hresult);
        ok(szError == NULL, "szError should be NULL\n");
    }
    stopped_binding = TRUE;

    return S_OK;
}

static HRESULT WINAPI statusclb_GetBindInfo(IBindStatusCallback *iface, DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    DWORD cbSize;

    CHECK_EXPECT(GetBindInfo);

    *grfBINDF = bindf;
    cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize);
    pbindinfo->cbSize = cbSize;

    return S_OK;
}

static HRESULT WINAPI statusclb_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    HRESULT hres;
    DWORD readed;
    BYTE buf[512];
    CHAR clipfmt[512];

    CHECK_EXPECT2(OnDataAvailable);
    ok(download_state == DOWNLOADING || download_state == END_DOWNLOAD,
       "Download state was %d, expected DOWNLOADING or END_DOWNLOAD\n",
       download_state);
    data_available = TRUE;

    ok(pformatetc != NULL, "pformatetx == NULL\n");
    if(pformatetc) {
        if (mime_type[0]) todo_wine {
            clipfmt[0] = 0;
            ok(GetClipboardFormatName(pformatetc->cfFormat, clipfmt, sizeof(clipfmt)-1),
               "GetClipboardFormatName failed, error %d\n", GetLastError());
            ok(!lstrcmp(clipfmt, mime_type), "clipformat != mime_type, \"%s\" != \"%s\"\n",
               clipfmt, mime_type);
        } else {
            ok(pformatetc->cfFormat == 0, "clipformat=%x\n", pformatetc->cfFormat);
        }
        ok(pformatetc->ptd == NULL, "ptd = %p\n", pformatetc->ptd);
        ok(pformatetc->dwAspect == 1, "dwAspect=%u\n", pformatetc->dwAspect);
        ok(pformatetc->lindex == -1, "lindex=%d\n", pformatetc->lindex);
        ok(pformatetc->tymed == TYMED_ISTREAM, "tymed=%u\n", pformatetc->tymed);
    }

    ok(pstgmed != NULL, "stgmeg == NULL\n");
    if(pstgmed) {
        ok(pstgmed->tymed == TYMED_ISTREAM, "tymed=%u\n", pstgmed->tymed);
        ok(U(*pstgmed).pstm != NULL, "pstm == NULL\n");
        ok(pstgmed->pUnkForRelease != NULL, "pUnkForRelease == NULL\n");
    }

    if(U(*pstgmed).pstm) {
        do hres = IStream_Read(U(*pstgmed).pstm, buf, 512, &readed);
        while(hres == S_OK);
        ok(hres == S_FALSE || hres == E_PENDING, "IStream_Read returned %08x\n", hres);
    }

    return S_OK;
}

static HRESULT WINAPI statusclb_OnObjectAvailable(IBindStatusCallback *iface, REFIID riid, IUnknown *punk)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    statusclb_QueryInterface,
    statusclb_AddRef,
    statusclb_Release,
    statusclb_OnStartBinding,
    statusclb_GetPriority,
    statusclb_OnLowResource,
    statusclb_OnProgress,
    statusclb_OnStopBinding,
    statusclb_GetBindInfo,
    statusclb_OnDataAvailable,
    statusclb_OnObjectAvailable
};

static IBindStatusCallback bsc = { &BindStatusCallbackVtbl };

static void test_CreateAsyncBindCtx(void)
{
    IBindCtx *bctx = (IBindCtx*)0x0ff00ff0;
    IUnknown *unk;
    HRESULT hres;
    ULONG ref;
    BIND_OPTS bindopts;

    hres = CreateAsyncBindCtx(0, NULL, NULL, &bctx);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08x\n", hres);
    ok(bctx == (IBindCtx*)0x0ff00ff0, "bctx should not be changed\n");

    hres = CreateAsyncBindCtx(0, NULL, NULL, NULL);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed. expected: E_INVALIDARG, got: %08x\n", hres);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(SUCCEEDED(hres), "CreateAsyncBindCtx failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    bindopts.cbStruct = sizeof(bindopts);
    hres = IBindCtx_GetBindOptions(bctx, &bindopts);
    ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
    ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
    ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
    ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == E_NOINTERFACE, "QueryInterface(IID_IAsyncBindCtx) failed: %08x, expected E_NOINTERFACE\n", hres);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk);

    ref = IBindCtx_Release(bctx);
    ok(ref == 0, "bctx should be destroyed here\n");
}

static void test_CreateAsyncBindCtxEx(void)
{
    IBindCtx *bctx = NULL, *bctx_arg = NULL;
    IUnknown *unk;
    BIND_OPTS bindopts;
    HRESULT hres;

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, NULL, 0);
    ok(hres == E_INVALIDARG, "CreateAsyncBindCtx failed: %08x, expected E_INVALIDARG\n", hres);

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    CreateBindCtx(0, &bctx_arg);
    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    if(SUCCEEDED(hres)) {
        bindopts.cbStruct = sizeof(bindopts);
        hres = IBindCtx_GetBindOptions(bctx, &bindopts);
        ok(SUCCEEDED(hres), "IBindCtx_GetBindOptions failed: %08x\n", hres);
        ok(bindopts.grfFlags == BIND_MAYBOTHERUSER,
                "bindopts.grfFlags = %08x, expected: BIND_MAYBOTHERUSER\n", bindopts.grfFlags);
        ok(bindopts.grfMode == (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                "bindopts.grfMode = %08x, expected: STGM_READWRITE | STGM_SHARE_EXCLUSIVE\n",
                bindopts.grfMode);
        ok(bindopts.dwTickCountDeadline == 0,
                "bindopts.dwTickCountDeadline = %08x, expected: 0\n", bindopts.dwTickCountDeadline);

        IBindCtx_Release(bctx);
    }

    IBindCtx_Release(bctx_arg);

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtxEx(NULL, 0, &bsc, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);

    hres = IBindCtx_QueryInterface(bctx, &IID_IAsyncBindCtx, (void**)&unk);
    ok(hres == S_OK, "QueryInterface(IID_IAsyncBindCtx) failed: %08x\n", hres);
    if(SUCCEEDED(hres))
        IUnknown_Release(unk);

    if(SUCCEEDED(hres))
        IBindCtx_Release(bctx);
}

static void test_BindToStorage(int protocol, BOOL emul)
{
    IMoniker *mon;
    HRESULT hres;
    LPOLESTR display_name;
    IBindCtx *bctx;
    MSG msg;
    IBindStatusCallback *previousclb;
    IUnknown *unk = (IUnknown*)0x00ff00ff;
    IBinding *bind;

    test_protocol = protocol;
    emulate_protocol = emul;
    download_state = BEFORE_DOWNLOAD;
    stopped_binding = FALSE;
    data_available = FALSE;
    mime_type[0] = 0;

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = CreateAsyncBindCtx(0, &bsc, NULL, &bctx);
    ok(hres == S_OK, "CreateAsyncBindCtx failed: %08x\n\n", hres);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(FAILED(hres))
        return;

    SET_EXPECT(QueryInterface_IServiceProvider);
    hres = RegisterBindStatusCallback(bctx, &bsc, &previousclb, 0);
    ok(hres == S_OK, "RegisterBindStatusCallback failed: %08x\n", hres);
    ok(previousclb == &bsc, "previousclb(%p) != sclb(%p)\n", previousclb, &bsc);
    CHECK_CALLED(QueryInterface_IServiceProvider);
    if(previousclb)
        IBindStatusCallback_Release(previousclb);

    hres = CreateURLMoniker(NULL, urls[test_protocol], &mon);
    ok(SUCCEEDED(hres), "failed to create moniker: %08x\n", hres);
    if(FAILED(hres)) {
        IBindCtx_Release(bctx);
        return;
    }

    if(test_protocol == FILE_TEST && INDEX_HTML[7] == '/')
        memmove(INDEX_HTML+7, INDEX_HTML+8, lstrlenW(INDEX_HTML+7)*sizeof(WCHAR));

    hres = IMoniker_QueryInterface(mon, &IID_IBinding, (void**)&bind);
    ok(hres == E_NOINTERFACE, "IMoniker should not have IBinding interface\n");
    if(SUCCEEDED(hres))
        IBinding_Release(bind);

    hres = IMoniker_GetDisplayName(mon, bctx, NULL, &display_name);
    ok(hres == S_OK, "GetDisplayName failed %08x\n", hres);
    ok(!lstrcmpW(display_name, urls[test_protocol]), "GetDisplayName got wrong name\n");

    SET_EXPECT(QueryInterface_IServiceProvider);
    SET_EXPECT(GetBindInfo);
    SET_EXPECT(OnStartBinding);
    if(emulate_protocol) {
        SET_EXPECT(Start);
        SET_EXPECT(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            SET_EXPECT(QueryInterface_IHttpNegotiate);
            SET_EXPECT(BeginningTransaction);
            SET_EXPECT(QueryInterface_IHttpNegotiate2);
            SET_EXPECT(GetRootSecurityId);
            SET_EXPECT(OnProgress_FINDINGRESOURCE);
            SET_EXPECT(OnProgress_CONNECTING);
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            SET_EXPECT(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnResponse);
        SET_EXPECT(OnProgress_MIMETYPEAVAILABLE);
        SET_EXPECT(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            SET_EXPECT(OnProgress_DOWNLOADINGDATA);
        SET_EXPECT(OnProgress_ENDDOWNLOADDATA);
        SET_EXPECT(OnDataAvailable);
        SET_EXPECT(OnStopBinding);
    }

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    if (test_protocol == HTTP_TEST && hres == HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED))
    {
        trace( "Network unreachable, skipping tests\n" );
        return;
    }
    if (!SUCCEEDED(hres)) return;

    if((bindf & BINDF_ASYNCHRONOUS) && !data_available) {
        ok(hres == MK_S_ASYNCHRONOUS, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk == NULL, "istr should be NULL\n");
    }else {
        ok(hres == S_OK, "IMoniker_BindToStorage failed: %08x\n", hres);
        ok(unk != NULL, "unk == NULL\n");
    }
    if(unk)
        IUnknown_Release(unk);

    while((bindf & BINDF_ASYNCHRONOUS) &&
          !stopped_binding && GetMessage(&msg,NULL,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CHECK_NOT_CALLED(QueryInterface_IServiceProvider);
    CHECK_CALLED(GetBindInfo);
    CHECK_CALLED(OnStartBinding);
    if(emulate_protocol) {
        CHECK_CALLED(Start);
        CHECK_CALLED(UnlockRequest);
    }else {
        if(test_protocol == HTTP_TEST) {
            CHECK_CALLED(QueryInterface_IHttpNegotiate);
            CHECK_CALLED(BeginningTransaction);
            /* QueryInterface_IHttpNegotiate2 and GetRootSecurityId
             * called on WinXP but not on Win98 */
            CLEAR_CALLED(QueryInterface_IHttpNegotiate2);
            CLEAR_CALLED(GetRootSecurityId);
            if(http_is_first) {
                CHECK_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_CALLED(OnProgress_CONNECTING);
            }else todo_wine {
                CHECK_NOT_CALLED(OnProgress_FINDINGRESOURCE);
                CHECK_NOT_CALLED(OnProgress_CONNECTING);
            }
        }
        if(test_protocol == HTTP_TEST || test_protocol == FILE_TEST)
            CHECK_CALLED(OnProgress_SENDINGREQUEST);
        if(test_protocol == HTTP_TEST)
            CHECK_CALLED(OnResponse);
        CHECK_CALLED(OnProgress_MIMETYPEAVAILABLE);
        CHECK_CALLED(OnProgress_BEGINDOWNLOADDATA);
        if(test_protocol == HTTP_TEST)
            CLEAR_CALLED(OnProgress_DOWNLOADINGDATA);
        CHECK_CALLED(OnProgress_ENDDOWNLOADDATA);
        CHECK_CALLED(OnDataAvailable);
        CHECK_CALLED(OnStopBinding);
    }

    ok(IMoniker_Release(mon) == 0, "mon should be destroyed here\n");
    ok(IBindCtx_Release(bctx) == 0, "bctx should be destroyed here\n");

    if(test_protocol == HTTP_TEST)
        http_is_first = FALSE;
}

static void set_file_url(void)
{
    int len;

    static const WCHAR wszFile[] = {'f','i','l','e',':','/','/'};

    memcpy(INDEX_HTML, wszFile, sizeof(wszFile));
    len = sizeof(wszFile)/sizeof(WCHAR);
    INDEX_HTML[len++] = '/';
    len += GetCurrentDirectoryW(sizeof(INDEX_HTML)/sizeof(WCHAR)-len, INDEX_HTML+len);
    INDEX_HTML[len++] = '\\';
    memcpy(INDEX_HTML+len, wszIndexHtml, sizeof(wszIndexHtml));
}

static void create_file(void)
{
    HANDLE file;
    DWORD size;

    static const char html_doc[] = "<HTML></HTML>";

    file = CreateFileW(wszIndexHtml, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed\n");
    if(file == INVALID_HANDLE_VALUE)
        return;

    WriteFile(file, html_doc, sizeof(html_doc)-1, &size, NULL);
    CloseHandle(file);

    set_file_url();
}

static void test_BindToStorage_fail(void)
{
    IMoniker *mon = NULL;
    IBindCtx *bctx = NULL;
    IUnknown *unk;
    HRESULT hres;

    hres = CreateURLMoniker(NULL, ABOUT_BLANK, &mon);
    ok(hres == S_OK, "CreateURLMoniker failed: %08x\n", hres);
    if(FAILED(hres))
        return;

    hres = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &bctx, 0);
    ok(hres == S_OK, "CreateAsyncBindCtxEx failed: %08x\n", hres);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&unk);
    ok(hres == MK_E_SYNTAX, "hres=%08x, expected INET_E_SYNTAX\n", hres);

    IBindCtx_Release(bctx);

    IMoniker_Release(mon);
}

START_TEST(url)
{
    test_create();
    test_CreateAsyncBindCtx();
    test_CreateAsyncBindCtxEx();

    trace("synchronous http test...\n");
    test_BindToStorage(HTTP_TEST, FALSE);

    trace("synchronous file test...\n");
    create_file();
    test_BindToStorage(FILE_TEST, FALSE);
    DeleteFileW(wszIndexHtml);

    bindf = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    trace("http test...\n");
    test_BindToStorage(HTTP_TEST, FALSE);

    trace("http test (short response)...\n");
    http_is_first = TRUE;
    urls[HTTP_TEST] = SHORT_RESPONSE_URL;
    test_BindToStorage(HTTP_TEST, FALSE);

    trace("about test...\n");
    CoInitialize(NULL);
    test_BindToStorage(ABOUT_TEST, FALSE);
    CoUninitialize();

    trace("emulated about test...\n");
    test_BindToStorage(ABOUT_TEST, TRUE);

    trace("file test...\n");
    create_file();
    test_BindToStorage(FILE_TEST, FALSE);
    DeleteFileW(wszIndexHtml);

    trace("emulated file test...\n");
    set_file_url();
    test_BindToStorage(FILE_TEST, TRUE);

    trace("emulated its test...\n");
    test_BindToStorage(ITS_TEST, TRUE);

    trace("emulated mk test...\n");
    test_BindToStorage(MK_TEST, TRUE);

    test_BindToStorage_fail();
}
