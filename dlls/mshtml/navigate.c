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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define CONTENT_LENGTH "Content-Length"

#define STATUSCLB_THIS(iface) DEFINE_THIS(BSCallback, BindStatusCallback, iface)

typedef struct {
    const IBindStatusCallbackVtbl *lpBindStatusCallbackVtbl;
    const IServiceProviderVtbl    *lpServiceProviderVtbl;
    const IHttpNegotiate2Vtbl     *lpHttpNegotiate2Vtbl;
    const IInternetBindInfoVtbl   *lpInternetBindInfoVtbl;

    LONG ref;

    LPWSTR headers;
    HGLOBAL post_data;
    ULONG post_data_len;
} BSCallback;

#define HTTPNEG(x)   ((IHttpNegotiate2*)    &(x)->lpHttpNegotiate2Vtbl)
#define BINDINFO(x)  ((IInternetBindInfo*)  &(x)->lpInternetBindInfoVtbl);

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BSCallback *This = STATUSCLB_THIS(iface);

    *ppv = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        TRACE("(%p)->(IID_IBindStatusCallback, %p)\n", This, ppv);
        *ppv = STATUSCLB(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IHttpNegotiate2, riid)) {
        TRACE("(%p)->(IID_IHttpNegotiate2 %p)\n", This, ppv);
        *ppv = HTTPNEG(This);
    }else if(IsEqualGUID(&IID_IInternetBindInfo, riid)) {
        TRACE("(%p)->(IID_IInternetBindInfo %p)\n", This, ppv);
        *ppv = BINDINFO(This);
    }

    if(*ppv) {
        IBindStatusCallback_AddRef(STATUSCLB(This));
        return S_OK;
    }

    TRACE("Unsupported riid = %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %ld\n", This, ref);

    if(!ref) {
        if(This->post_data)
            GlobalFree(This->post_data);
        HeapFree(GetProcessHeap(), 0, This->headers);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pbind)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, pbind);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pnPriority)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pnPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    TRACE("%p)->(%lu %lu %lu %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));
    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%08lx %s)\n", This, hresult, debugstr_w(szError));
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    DWORD size;

    TRACE("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);

    *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    size = pbindinfo->cbSize;
    memset(pbindinfo, 0, size);
    pbindinfo->cbSize = size;

    pbindinfo->cbStgmedData = This->post_data_len;
    pbindinfo->dwCodePage = CP_UTF8;
    pbindinfo->dwOptions = 0x00020000;

    if(This->post_data) {
        pbindinfo->dwBindVerb = BINDVERB_POST;

        pbindinfo->stgmedData.tymed = TYMED_HGLOBAL;
        pbindinfo->stgmedData.u.hGlobal = This->post_data;
        pbindinfo->stgmedData.pUnkForRelease = (IUnknown*)STATUSCLB(This);
        IBindStatusCallback_AddRef(STATUSCLB(This));
    }

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%08lx %ld %p %p)\n", This, grfBSCF, dwSize, pformatetc, pstgmed);
    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BSCallback *This = STATUSCLB_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), punk);
    return E_NOTIMPL;
}

#undef STATUSCLB_THIS

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
    BindStatusCallback_OnDataAvailable,
    BindStatusCallback_OnObjectAvailable
};

#define HTTPNEG_THIS(iface) DEFINE_THIS(BSCallback, HttpNegotiate2, iface)

static HRESULT WINAPI HttpNegotiate_QueryInterface(IHttpNegotiate2 *iface,
                                                   REFIID riid, void **ppv)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI HttpNegotiate_AddRef(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI HttpNegotiate_Release(IHttpNegotiate2 *iface)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI HttpNegotiate_BeginningTransaction(IHttpNegotiate2 *iface,
        LPCWSTR szURL, LPCWSTR szHeaders, DWORD dwReserved, LPWSTR *pszAdditionalHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    DWORD size;

    TRACE("(%p)->(%s %s %ld %p)\n", This, debugstr_w(szURL), debugstr_w(szHeaders),
          dwReserved, pszAdditionalHeaders);

    if(!This->headers) {
        *pszAdditionalHeaders = NULL;
        return S_OK;
    }

    size = (strlenW(This->headers)+1)*sizeof(WCHAR);
    *pszAdditionalHeaders = CoTaskMemAlloc(size);
    memcpy(*pszAdditionalHeaders, This->headers, size);

    return S_OK;
}

static HRESULT WINAPI HttpNegotiate_OnResponse(IHttpNegotiate2 *iface, DWORD dwResponseCode,
        LPCWSTR szResponseHeaders, LPCWSTR szRequestHeaders, LPWSTR *pszAdditionalRequestHeaders)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%ld %s %s %p)\n", This, dwResponseCode, debugstr_w(szResponseHeaders),
          debugstr_w(szRequestHeaders), pszAdditionalRequestHeaders);
    return E_NOTIMPL;
}

static HRESULT WINAPI HttpNegotiate_GetRootSecurityId(IHttpNegotiate2 *iface,
        BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved)
{
    BSCallback *This = HTTPNEG_THIS(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pbSecurityId, pcbSecurityId, dwReserved);
    return E_NOTIMPL;
}

#undef HTTPNEG

static const IHttpNegotiate2Vtbl HttpNegotiate2Vtbl = {
    HttpNegotiate_QueryInterface,
    HttpNegotiate_AddRef,
    HttpNegotiate_Release,
    HttpNegotiate_BeginningTransaction,
    HttpNegotiate_OnResponse,
    HttpNegotiate_GetRootSecurityId
};

#define BINDINFO_THIS(iface) DEFINE_THIS(BSCallback, InternetBindInfo, iface)

static HRESULT WINAPI InternetBindInfo_QueryInterface(IInternetBindInfo *iface,
                                                      REFIID riid, void **ppv)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI InternetBindInfo_AddRef(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI InternetBindInfo_Release(IInternetBindInfo *iface)
{
    BSCallback *This = BINDINFO_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI InternetBindInfo_GetBindInfo(IInternetBindInfo *iface,
                                                   DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, grfBINDF, pbindinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetBindInfo_GetBindString(IInternetBindInfo *iface,
        ULONG ulStringType, LPOLESTR *ppwzStr, ULONG cEl, ULONG *pcElFetched)
{
    BSCallback *This = BINDINFO_THIS(iface);
    FIXME("(%p)->(%lu %p %lu %p)\n", This, ulStringType, ppwzStr, cEl, pcElFetched);
    return E_NOTIMPL;
}

#undef BINDINFO_THIS

static const IInternetBindInfoVtbl InternetBindInfoVtbl = {
    InternetBindInfo_QueryInterface,
    InternetBindInfo_AddRef,
    InternetBindInfo_Release,
    InternetBindInfo_GetBindInfo,
    InternetBindInfo_GetBindString
};

#define SERVPROV_THIS(iface) DEFINE_THIS(BSCallback, ServiceProvider, iface)

static HRESULT WINAPI BSCServiceProvider_QueryInterface(IServiceProvider *iface,
                                                        REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_QueryInterface(STATUSCLB(This), riid, ppv);
}

static ULONG WINAPI BSCServiceProvider_AddRef(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_AddRef(STATUSCLB(This));
}

static ULONG WINAPI BSCServiceProvider_Release(IServiceProvider *iface)
{
    BSCallback *This = SERVPROV_THIS(iface);
    return IBindStatusCallback_Release(STATUSCLB(This));
}

static HRESULT WINAPI BSCServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    BSCallback *This = SERVPROV_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    BSCServiceProvider_QueryInterface,
    BSCServiceProvider_AddRef,
    BSCServiceProvider_Release,
    BSCServiceProvider_QueryService
};

static IBindStatusCallback *BSCallback_Create(HTMLDocument *doc, LPCOLESTR url,
        HGLOBAL post_data, ULONG post_data_len, LPWSTR headers)
{
    BSCallback *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(BSCallback));

    ret->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    ret->lpServiceProviderVtbl    = &ServiceProviderVtbl;
    ret->lpHttpNegotiate2Vtbl     = &HttpNegotiate2Vtbl;
    ret->lpInternetBindInfoVtbl   = &InternetBindInfoVtbl;
    ret->ref = 1;
    ret->post_data = post_data;
    ret->headers = headers;
    ret->post_data_len = post_data_len;

    return STATUSCLB(ret);
}

static void parse_post_data(nsIInputStream *post_data_stream, LPWSTR *headers_ret,
                            HGLOBAL *post_data_ret, ULONG *post_data_len_ret)
{
    PRUint32 post_data_len = 0, available = 0;
    HGLOBAL post_data = NULL;
    LPWSTR headers = NULL;
    DWORD headers_len = 0, len;
    const char *ptr, *ptr2;

    nsIInputStream_Available(post_data_stream, &available);
    post_data = GlobalAlloc(0, available+1);
    nsIInputStream_Read(post_data_stream, post_data, available, &post_data_len);
    
    TRACE("post_data = %s\n", debugstr_an(post_data, post_data_len));

    ptr = ptr2 = post_data;

    while(*ptr && (*ptr != '\r' || ptr[1] != '\n')) {
        while(*ptr && (*ptr != '\r' || ptr[1] != '\n'))
            ptr++;

        if(!*ptr) {
            FIXME("*ptr = 0\n");
            return;
        }

        ptr += 2;

        if(ptr-ptr2 >= sizeof(CONTENT_LENGTH)
           && CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                             CONTENT_LENGTH, sizeof(CONTENT_LENGTH)-1,
                             ptr2, sizeof(CONTENT_LENGTH)-1) == CSTR_EQUAL) {
            ptr2 = ptr;
            continue;
        }

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, NULL, 0);

        if(headers)
            headers = HeapReAlloc(GetProcessHeap(), 0, headers,
                                  (headers_len+len+1)*sizeof(WCHAR));
        else
            headers = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));

        len = MultiByteToWideChar(CP_ACP, 0, ptr2, ptr-ptr2, headers+headers_len, -1);
        headers_len += len;

        ptr2 = ptr;
    }

    headers[headers_len] = 0;
    *headers_ret = headers;

    if(*ptr)
        ptr += 2;

    if(!*ptr || !(ptr-(const char*)post_data)) {
        GlobalFree(post_data);
        return;
    }

    if(headers_len) {
        post_data_len -= ptr-(const char*)post_data;
        memmove(post_data, ptr, post_data_len);
        post_data = GlobalReAlloc(post_data, post_data_len+1, 0);
    }

    *((PBYTE)post_data+post_data_len) = 0;

    *post_data_ret = post_data;
    *post_data_len_ret = post_data_len+1;
}

void hlink_frame_navigate(NSContainer *container, IHlinkFrame *hlink_frame,
                          LPCWSTR uri, nsIInputStream *post_data_stream)
{
    IBindStatusCallback *callback;
    IBindCtx *bindctx;
    IMoniker *mon;
    IHlink *hlink;
    PRUint32 post_data_len = 0;
    HGLOBAL post_data = NULL;
    LPWSTR headers = NULL;

    if(post_data_stream) {
        parse_post_data(post_data_stream, &headers, &post_data, &post_data_len);
        TRACE("headers = %s post_data = %s\n", debugstr_w(headers), debugstr_a(post_data));
    }

    callback = BSCallback_Create(container->doc, uri, post_data, post_data_len, headers);
    CreateAsyncBindCtx(0, callback, NULL, &bindctx);

    hlink = Hlink_Create();

    CreateURLMoniker(NULL, uri, &mon);
    IHlink_SetMonikerReference(hlink, 0, mon, NULL);

    IHlinkFrame_Navigate(hlink_frame, 0, bindctx, callback, hlink);

    IBindCtx_Release(bindctx);
    IBindStatusCallback_Release(callback);
    IMoniker_Release(mon);

}
