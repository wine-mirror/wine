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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"
#include "wine/unicode.h"

#include "shdocvw.h"
#include "mshtml.h"
#include "exdispid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static void dump_BINDINFO(BINDINFO *bi)
{
    static const char *BINDINFOF_str[] = {
        "#0",
        "BINDINFOF_URLENCODESTGMEDDATA",
        "BINDINFOF_URLENCODEDEXTRAINFO"
    };

    static const char *BINDVERB_str[] = {
        "BINDVERB_GET",
        "BINDVERB_POST",
        "BINDVERB_PUT",
        "BINDVERB_CUSTOM"
    };

    TRACE("\n"
            "BINDINFO = {\n"
            "    %ld, %s,\n"
            "    {%ld, %p, %p},\n"
            "    %s,\n"
            "    %s,\n"
            "    %s,\n"
            "    %ld, %08lx, %ld, %ld\n"
            "    {%ld %p %x},\n"
            "    %s\n"
            "    %p, %ld\n"
            "}\n",

            bi->cbSize, debugstr_w(bi->szExtraInfo),
            bi->stgmedData.tymed, bi->stgmedData.u.hGlobal, bi->stgmedData.pUnkForRelease,
            bi->grfBindInfoF > BINDINFOF_URLENCODEDEXTRAINFO
                ? "unknown" : BINDINFOF_str[bi->grfBindInfoF],
            bi->dwBindVerb > BINDVERB_CUSTOM
                ? "unknown" : BINDVERB_str[bi->dwBindVerb],
            debugstr_w(bi->szCustomVerb),
            bi->cbStgmedData, bi->dwOptions, bi->dwOptionsFlags, bi->dwCodePage,
            bi->securityAttributes.nLength,
            bi->securityAttributes.lpSecurityDescriptor,
            bi->securityAttributes.bInheritHandle,
            debugstr_guid(&bi->iid),
            bi->pUnk, bi->dwReserved
            );
}


static void on_before_navigate2(WebBrowser *This, PBYTE post_data, ULONG post_data_len,
                                LPWSTR headers, VARIANT_BOOL *cancel)
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
    V_BSTR(&var_url) = SysAllocString(This->url);

    V_VT(params+6) = (VT_DISPATCH);
    V_DISPATCH(params+6) = (IDispatch*)WEBBROWSER2(This);

    call_sink(This->cp_wbe2, DISPID_BEFORENAVIGATE2, &dispparams);

    if(post_data_len)
        SafeArrayDestroy(V_ARRAY(&var_post_data));

}

static HRESULT navigate(WebBrowser *This, IMoniker *mon, IBindCtx *bindctx,
                        IBindStatusCallback *callback)
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
        WARN("Could not create HTMLDocument: %08lx\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IPersistMoniker, (void**)&persist);
    if(FAILED(hres))
        return hres;

    if(FAILED(hres)) {
        IPersistMoniker_Release(persist);
        return hres;
    }

    hres = IPersistMoniker_Load(persist, FALSE, mon, bindctx, 0);
    IPersistMoniker_Release(persist);
    if(FAILED(hres)) {
        WARN("Load failed: %08lx\n", hres);
        return hres;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(FAILED(hres))
        return hres;

    hres = IOleObject_SetClientSite(oleobj, CLIENTSITE(This));
    IOleObject_Release(oleobj);

    PostMessageW(This->doc_view_hwnd, WB_WM_NAVIGATE2, 0, 0);

    return hres;
}

HRESULT navigate_hlink(WebBrowser *This, IMoniker *mon, IBindCtx *bindctx,
                       IBindStatusCallback *callback)
{
    IHttpNegotiate *http_negotiate;
    PBYTE post_data = NULL;
    ULONG post_data_len = 0;
    LPWSTR headers = NULL;
    VARIANT_BOOL cancel = VARIANT_FALSE;
    BINDINFO bindinfo;
    DWORD bindf = 0;
    HRESULT hres;

    IMoniker_GetDisplayName(mon, NULL, NULL, &This->url);
    TRACE("navigating to %s\n", debugstr_w(This->url));

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
    if(bindinfo.dwBindVerb == BINDVERB_POST) {
        post_data_len = bindinfo.cbStgmedData;
        if(post_data_len)
            post_data = bindinfo.stgmedData.u.hGlobal;
    }

    on_before_navigate2(This, post_data, post_data_len, headers, &cancel);

    CoTaskMemFree(headers);
    ReleaseBindInfo(&bindinfo);

    if(cancel) {
        FIXME("navigation canceled\n");
        return S_OK;
    }

    return navigate(This, mon, bindctx, callback);
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
    BINDINFO bi = {0};
    DWORD bindf = 0;

    TRACE("(%p)->(%08lx %p %p %p)\n", This, grfHLNF, pbc, pibsc, pihlNavigate);

    if(grfHLNF)
        FIXME("unsupported grfHLNF=%08lx\n", grfHLNF);

    /* Windows calls GetTargetFrameName here. */

    memset(&bi, 0, sizeof(bi));
    bi.cbSize = sizeof(bi);

    IBindStatusCallback_GetBindInfo(pibsc, &bindf, &bi);
    dump_BINDINFO(&bi);

    IHlink_GetMonikerReference(pihlNavigate, 1, &mon, &location);

    if(location) {
        FIXME("location = %s\n", debugstr_w(location));
        CoTaskMemFree(location);
    }

    /* Windows calls GetHlinkSite here */

    return navigate_hlink(This, mon, pbc, pibsc);
}

static HRESULT WINAPI HlinkFrame_OnNavigate(IHlinkFrame *iface, DWORD grfHLNF,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%08lx %p %s %s %ld)\n", This, grfHLNF, pimkTarget, debugstr_w(pwzLocation),
          debugstr_w(pwzFriendlyName), dwreserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI HlinkFrame_UpdateHlink(IHlinkFrame *iface, ULONG uHLID,
        IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
{
    WebBrowser *This = HLINKFRAME_THIS(iface);
    FIXME("(%p)->(%lu %p %s %s)\n", This, uHLID, pimkTarget, debugstr_w(pwzLocation),
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
