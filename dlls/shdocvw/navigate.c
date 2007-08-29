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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"
#include "wine/unicode.h"

#include "shdocvw.h"
#include "mshtml.h"
#include "exdispid.h"
#include "shellapi.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

typedef struct {
    const IBindStatusCallbackVtbl  *lpBindStatusCallbackVtbl;
    const IHttpNegotiateVtbl       *lpHttpNegotiateVtbl;

    LONG ref;

    HGLOBAL post_data;
    LPWSTR headers;
    ULONG post_data_len;
} BindStatusCallback;

#define BINDSC(x)  ((IBindStatusCallback*) &(x)->lpBindStatusCallbackVtbl)
#define HTTPNEG(x) ((IHttpNegotiate*)      &(x)->lpHttpNegotiateVtbl)

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char * const BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char * const BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %d, %s,\n"
            "    {%d, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %d, %08x, %d, %d\n"
            "    {%d %p %x},\n"
            "    %s\n"
            "    %p, %d\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbstgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}

#define BINDSC_THIS(iface) DEFINE_THIS(BindStatusCallback, BindStatusCallback, iface)

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
                                                        REFIID riid, void **ppv)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = BINDSC(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback %p)\n", This, ppv);
        *ppv = BINDSC(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(BINDSC(This));
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->post_data)
            GlobalFree(This->post_data);
        shdocvw_free(This->headers);
        shdocvw_free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
       DWORD dwReserved, IBinding *pbind)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwReserved, pbind);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface,
       LONG *pnPriority)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface,
       DWORD reserved)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%d)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface,
        ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%d %d %d %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
          debugstr_w(szStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%08x %s)\n", This, hresult, debugstr_w(szError));
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = BINDSC_THIS(iface);

    FIXME("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    memset(pbindinfo, 0, sizeof(BINDINFO));
    pbindinfo->cbSize = sizeof(BINDINFO);

    pbindinfo->cbstgmedData = This->post_data_len;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)BINDSC(This);
        IBindStatusCallback_AddRef(BINDSC(This));
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BindStatusCallback *This = BINDSC_THIS(iface);
    FIXME("(%p)->(%08x %d %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);
    return E_NOTIMPL;
}

#undef BSC_THIS

static const IBindStatusCallbackVtbl BindStatusCallbackVtbl = {
    BindStatusCallback_QueryInterface,
    BindStatusCallback_AddRef,
    BindStatusCallback_Release,
    BindStatusCallback_OnStartBinding,
    BindStatusCallback_GetPriority,
    BindStatusCallback_OnLowResource,
    BindStatusCallback_OnProgress,
    BindStatusCallback_OnStopBinding,
    BindStatusCallback_GetBindInfo,
    BindStatusCallback_OnDataAvailable
};

#define HTTPNEG_THIS(iface) DEFINE_THIS(BindStatusCallback, HttpNegotiate, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate *iface,
                                                   REFIID riid, void **ppv)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_QueryInterface(BINDSC(This), riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_AddRef(BINDSC(This));
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate *iface)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_Release(BINDSC(This));
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);

    FIXME("(%p)->(%s %s %d %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    if(This->headers) {
        int size = (strlenW(This->headers)+1)*sizeof(WCHAR);
        *pszAdditionalHeaders = CoTaskMemAlloc(size);
        memcpy(*pszAdditionalHeaders, This->headers, size);
    }

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate *iface,
        DWORD dwResponseCode, LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders,
        LPWSTR *pszAdditionalRequestHeaders)
{
    BindStatusCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%d %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return E_NOTIMPL;
}

#undef HTTPNEG_THIS

static const IHttpNegotiateVtbl HttpNegotiateVtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse
};

static IBindStatusCallback *create_callback(DocHost *This, PBYTE post_data,
        ULONG post_data_len, LPWSTR headers, VARIANT_BOOL *cancel)
{
    BindStatusCallback *ret = shdocvw_alloc(sizeof(BindStatusCallback));

    ret->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    ret->lpHttpNegotiateVtbl      = &HttpNegotiateVtbl;

    ret->ref = 1;
    ret->post_data = NULL;
    ret->post_data_len = post_data_len;
    ret->headers = NULL;

    if(post_data) {
        ret->post_data = GlobalAlloc(0, post_data_len);
        memcpy(ret->post_data, post_data, post_data_len);
    }

    if(headers) {
        int size = (strlenW(headers)+1)*sizeof(WCHAR);
        ret->headers = shdocvw_alloc(size);
        memcpy(ret->headers, headers, size);
    }

    return BINDSC(ret);
}

static void on_before_navigate2(DocHost *This, LPCWSTR url, const BYTE *post_data,
                                ULONG post_data_len, LPWSTR headers, VARIANT_BOOL *cancel)
{
    VARIANT var_url, var_flags, var_frame_name, var_post_data, var_post_data2, var_headers;
    DISPPARAMS dispparams;
    VARIANTARG params[7];

    dispparams.cArgs = 7;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = VT_BOOL|VT_BYREF;
    V_BOOLREF(params) = cancel;

    V_VT(params+1) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+1) = &var_headers;
    V_VT(&var_headers) = VT_BSTR;
    V_BSTR(&var_headers) = headers;

    V_VT(params+2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+2) = &var_post_data2;
    V_VT(&var_post_data2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(&var_post_data2) = &var_post_data;
    VariantInit(&var_post_data);

    if(post_data_len) {
        SAFEARRAYBOUND bound = {post_data_len, 0};
        void *data;

        V_VT(&var_post_data) = VT_UI1|VT_ARRAY;
        V_ARRAY(&var_post_data) = SafeArrayCreate(VT_UI1, 1, &bound);

        SafeArrayAccessData(V_ARRAY(&var_post_data), &data);
        memcpy(data, post_data, post_data_len);
        SafeArrayUnaccessData(V_ARRAY(&var_post_data));
    }

    V_VT(params+3) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+3) = &var_frame_name;
    V_VT(&var_frame_name) = VT_BSTR;
    V_BSTR(&var_frame_name) = NULL;

    V_VT(params+4) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+4) = &var_flags;
    V_VT(&var_flags) = VT_I4;
    V_I4(&var_flags) = 0;

    V_VT(params+5) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+5) = &var_url;
    V_VT(&var_url) = VT_BSTR;
    V_BSTR(&var_url) = SysAllocString(url);

    V_VT(params+6) = (VT_DISPATCH);
    V_DISPATCH(params+6) = This->disp;

    call_sink(This->cps.wbe2, DISPID_BEFORENAVIGATE2, &dispparams);

    SysFreeString(V_BSTR(&var_url));
    if(post_data_len)
        SafeArrayDestroy(V_ARRAY(&var_post_data));
}

static BOOL try_application_url(LPCWSTR url)
{
    SHELLEXECUTEINFOW exec_info;
    WCHAR app[64];
    HKEY hkey;
    DWORD res, type;
    HRESULT hres;

    static const WCHAR wszURLProtocol[] = {'U','R','L',' ','P','r','o','t','o','c','o','l',0};

    hres = CoInternetParseUrl(url, PARSE_SCHEMA, 0, app, sizeof(app)/sizeof(WCHAR), NULL, 0);
    if(FAILED(hres))
        return FALSE;

    res = RegOpenKeyW(HKEY_CLASSES_ROOT, app, &hkey);
    if(res != ERROR_SUCCESS)
        return FALSE;

    res = RegQueryValueExW(hkey, wszURLProtocol, NULL, &type, NULL, NULL);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    TRACE("openning application %s\n", debugstr_w(app));
 
    memset(&exec_info, 0, sizeof(exec_info));
    exec_info.cbSize = sizeof(exec_info);
    exec_info.lpFile = url;
    exec_info.nShow = SW_SHOW;

    return ShellExecuteExW(&exec_info);
}

static HRESULT navigate(DocHost *This, IMoniker *mon, IBindCtx *bindctx)
{
    IOleObject *oleobj;
    IPersistMoniker *persist;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    HRESULT hres;

    if(cancel) {
        FIXME("Cancel\n");
        return S_OK;
    }

    IBindCtx_RegisterObjectParam(bindctx, (LPOLESTR)SZ_HTML_CLIENTSITE_OBJECTPARAM,
                                 (IUnknown*)CLIENTSITE(This));

    /*
     * FIXME:
     * We should use URLMoniker's BindToObject instead creating HTMLDocument here.
     * This should be fixed when mshtml.dll and urlmon.dll will be good enough.
     */

    if(This->document)
        deactivate_document(This);

    hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
                            CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                            &IID_IUnknown, (void**)&This->document);

    if(FAILED(hres)) {
        ERR("Could not create HTMLDocument: %08x\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IPersistMoniker, (void**)&persist);
    if(FAILED(hres))
        return hres;

    if(This->frame)
        IOleInPlaceFrame_EnableModeless(This->frame, FALSE); /* FIXME */

    hres = IPersistMoniker_Load(persist, FALSE, mon, bindctx, 0);
    IPersistMoniker_Release(persist);

    if(This->frame) {
        static const WCHAR empty[] = {0};

        IOleInPlaceFrame_SetStatusText(This->frame, empty); /* FIXME */
        IOleInPlaceFrame_EnableModeless(This->frame, TRUE); /* FIXME */
    }

    if(FAILED(hres)) {
        WARN("Load failed: %08x\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(FAILED(hres))
        return hres;

    hres = IOleObject_SetClientSite(oleobj, CLIENTSITE(This));
    IOleObject_Release(oleobj);

    PostMessageW(This->hwnd, WB_WM_NAVIGATE2, 0, 0);

    return hres;

}

static HRESULT bind_url_to_object(DocHost *This, LPCWSTR url, PBYTE post_data, ULONG post_data_len,
                     LPWSTR headers)
{
    IBindStatusCallback *callback;
    IMoniker *mon;
    IBindCtx *bindctx;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    HRESULT hres;

    if(!This->hwnd)
        create_doc_view_hwnd(This);

    hres = CreateURLMoniker(NULL, url, &mon);
    if(FAILED(hres)) {
        WARN("CreateURLMoniker failed: %08x\n", hres);
        return hres;
    }

    IMoniker_GetDisplayName(mon, NULL, NULL, &This->url);
    TRACE("navigating to %s\n", debugstr_w(This->url));

    callback = create_callback(This, post_data, post_data_len, (LPWSTR)headers, &cancel);
    CreateAsyncBindCtx(0, callback, 0, &bindctx);
    IBindStatusCallback_Release(callback);

    hres = navigate(This, mon, bindctx);

    IBindCtx_Release(bindctx);
    IMoniker_Release(mon);

    return hres;
}

HRESULT navigate_url(DocHost *This, BSTR url, const VARIANT *Flags,
                     const VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL;
    HRESULT hres;

    TRACE("navigating to %s\n", debugstr_w(url));

    if((Flags && V_VT(Flags) != VT_EMPTY) 
       || (TargetFrameName && V_VT(TargetFrameName) != VT_EMPTY))
        FIXME("Unsupported arguments\n");

    if(PostData) {
        TRACE("PostData vt=%d\n", V_VT(PostData));

        if(V_VT(PostData) == (VT_ARRAY | VT_UI1)) {
            SafeArrayAccessData(V_ARRAY(PostData), (void**)&post_data);
            post_data_len = V_ARRAY(PostData)->rgsabound[0].cElements;
        }
    }

    if(Headers && V_VT(Headers) != VT_EMPTY && V_VT(Headers) != VT_ERROR) {
        if(V_VT(Headers) != VT_BSTR)
            return E_INVALIDARG;

        headers = V_BSTR(Headers);
        TRACE("Headers: %s\n", debugstr_w(headers));
    }

    hres = bind_url_to_object(This, url, post_data, post_data_len, headers);

    if(post_data)
        SafeArrayUnaccessData(V_ARRAY(PostData));

    return hres;
}

static HRESULT navigate_hlink(DocHost *This, IMoniker *mon, IBindCtx *bindctx,
                              IBindStatusCallback *callback)
{
    IHttpNegotiate *http_negotiate;
    LPWSTR url = NULL;
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    BINDINFO bindinfo;
    DWORD bindf = 0;
    HRESULT hres;

    IMoniker_GetDisplayName(mon, NULL, NULL, &url);
    TRACE("navigating to %s\n", debugstr_w(url));

    hres = IBindStatusCallback_QueryInterface(callback, &IID_IHttpNegotiate,
                                              (void**)&http_negotiate);
    if(SUCCEEDED(hres)) {
        static const WCHAR null_string[] = {0};

        IHttpNegotiate_BeginningTransaction(http_negotiate, null_string, null_string, 0,
                                            &headers);
        IHttpNegotiate_Release(http_negotiate);
    }

    memset(&bindinfo, 0, sizeof(bindinfo));
    bindinfo.cbSize = sizeof(bindinfo);

    hres = IBindStatusCallback_GetBindInfo(callback, &bindf, &bindinfo);
    dump_BINDINFO(&bindinfo);
    if(bindinfo.dwBindVerb == BINDVERB_POST) {
        post_data_len = bindinfo.cbstgmedData;
        if(post_data_len)
            post_data = bindinfo.stgmedData.u.hGlobal;
    }

    on_before_navigate2(This, url, post_data, post_data_len, headers, &cancel);

    CoTaskMemFree(headers);
    ReleaseBindInfo(&bindinfo);

    if(cancel) {
        FIXME("navigation canceled\n");
        CoTaskMemFree(url);
        return S_OK;
    }

    /* FIXME: We should do it after BindToObject call */
    if(try_application_url(url))
        return S_OK;

    This->url = url;

    return navigate(This, mon, bindctx);
}

#define HLINKFRAME_THIS(iface) DEFINE_THIS(WebBrowser, HlinkFrame, iface)

static HRESULT WINAPI HlinkFrame_QueryInterface(IHlinkFrame *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_QueryInterface(WEBBROWSER2(This), riid, ppv);
}

static ULONG WINAPI HlinkFrame_AddRef(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER2(This));
}

static ULONG WINAPI HlinkFrame_Release(IHlinkFrame *iface)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER2(This));
}

static HRESULT WINAPI HlinkFrame_SetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext *pihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_GetBrowseContext(IHlinkFrame *iface,
                                                  IHlinkBrowseContext **ppihlbc)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppihlbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_Navigate(IHlinkFrame *iface, DWORD grfHLNF, LPBC pbc,
                                          IBindStatusCallback *pibsc, IHlink *pihlNavigate)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    IMoniker *mon;
    LPWSTR location = NULL;

    TRACE("(%p)->(%08x %p %p %p)\n", This, grfHLNF, pbc, pibsc, pihlNavigate);

    if(grfHLNF)
        FIXME("unsupported grfHLNF=%08x\n", grfHLNF);

    /* Windows calls GetTargetFrameName here. */

    IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);

    if(location) {
        FIXME("location = %s\n", debugstr_w(location));
        CoTaskMemFree(location);
    }

    /* Windows calls GetHlinkSite here */

    if(grfHLNF & HLNF_OPENINNEWWINDOW) {
        FIXME("Not supported HLNF_OPENINNEWWINDOW\n");
        return E_NOTIMPL;
    }

    return navigate_hlink(&This->doc_host, mon, pbc, pibsc);
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%08x %p %s %s %d)\n", This, grfHLNF, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), dwreserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%u %p %s %s)\n", This, uHLID, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName));
    return E_NOTIMPL;
}

#undef HLINKFRAME_THIS

static const IHlinkFrameVtbl HlinkFrameVtbl = {
    HlinkFrame_QueryInterface,
    HlinkFrame_AddRef,
    HlinkFrame_Release,
    HlinkFrame_SetBrowseContext,
    HlinkFrame_GetBrowseContext,
    HlinkFrame_Navigate,
    HlinkFrame_OnNavigate,
    HlinkFrame_UpdateHlink
};

void WebBrowser_HlinkFrame_Init(WebBrowser *This)
{
    This->lpHlinkFrameVtbl = &HlinkFrameVtbl;
}
