/*
 *    IXMLHTTPRequest implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
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

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

static const WCHAR MethodGetW[] = {'G','E','T',0};
static const WCHAR MethodPutW[] = {'P','U','T',0};
static const WCHAR MethodPostW[] = {'P','O','S','T',0};

static const WCHAR colspaceW[] = {':',' ',0};
static const WCHAR crlfW[] = {'\r','\n',0};

typedef struct BindStatusCallback BindStatusCallback;

struct reqheader
{
    struct list entry;
    BSTR header;
    BSTR value;
};

typedef struct
{
    const struct IXMLHTTPRequestVtbl *lpVtbl;
    LONG ref;

    READYSTATE state;

    /* request */
    BINDVERB verb;
    BSTR url;
    BOOL async;
    struct list reqheaders;
    /* cached resulting custom request headers string length in WCHARs */
    LONG reqheader_size;

    /* credentials */
    BSTR user;
    BSTR password;

    /* bind callback */
    BindStatusCallback *bsc;
} httprequest;

static inline httprequest *impl_from_IXMLHTTPRequest( IXMLHTTPRequest *iface )
{
    return (httprequest *)((char*)iface - FIELD_OFFSET(httprequest, lpVtbl));
}

struct BindStatusCallback
{
    const IBindStatusCallbackVtbl *lpBindStatusCallbackVtbl;
    const IHttpNegotiateVtbl      *lpHttpNegotiateVtbl;
    LONG ref;

    IBinding *binding;
    const httprequest *request;
};

static inline BindStatusCallback *impl_from_IBindStatusCallback( IBindStatusCallback *iface )
{
    return (BindStatusCallback *)((char*)iface - FIELD_OFFSET(BindStatusCallback, lpBindStatusCallbackVtbl));
}

static inline BindStatusCallback *impl_from_IHttpNegotiate( IHttpNegotiate *iface )
{
    return (BindStatusCallback *)((char*)iface - FIELD_OFFSET(BindStatusCallback, lpHttpNegotiateVtbl));
}

void BindStatusCallback_Detach(BindStatusCallback *bsc)
{
    if (bsc)
    {
        if (bsc->binding) IBinding_Abort(bsc->binding);
        bsc->request = NULL;
        IBindStatusCallback_Release((IBindStatusCallback*)bsc);
    }
}

static HRESULT WINAPI BindStatusCallback_QueryInterface(IBindStatusCallback *iface,
        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    *ppv = NULL;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IBindStatusCallback, riid))
    {
        *ppv = &This->lpBindStatusCallbackVtbl;
    }
    else if (IsEqualGUID(&IID_IHttpNegotiate, riid))
    {
        *ppv = &This->lpHttpNegotiateVtbl;
    }
    else if (IsEqualGUID(&IID_IServiceProvider, riid) ||
             IsEqualGUID(&IID_IBindStatusCallbackEx, riid))
    {
        return E_NOINTERFACE;
    }

    if (*ppv)
    {
        IBindStatusCallback_AddRef(iface);
        return S_OK;
    }

    FIXME("Unsupported riid = %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI BindStatusCallback_AddRef(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI BindStatusCallback_Release(IBindStatusCallback *iface)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %d\n", This, ref);

    if (!ref)
    {
        if (This->binding) IBinding_Release(This->binding);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI BindStatusCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD reserved, IBinding *pbind)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%d %p)\n", This, reserved, pbind);

    if (!pbind) return E_INVALIDARG;

    This->binding = pbind;
    IBinding_AddRef(pbind);

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetPriority(IBindStatusCallback *iface, LONG *pPriority)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p)\n", This, pPriority);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnLowResource(IBindStatusCallback *iface, DWORD reserved)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%d)\n", This, reserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("%p)->(%u %u %u %s)\n", This, ulProgress, ulProgressMax, ulStatusCode,
            debugstr_w(szStatusText));

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hr, LPCWSTR error)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(0x%08x %s)\n", This, hr, debugstr_w(error));

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD *bind_flags, BINDINFO *pbindinfo)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    TRACE("(%p)->(%p %p)\n", This, bind_flags, pbindinfo);

    *bind_flags = 0;
    if (This->request->async) *bind_flags |= BINDF_ASYNCHRONOUS;

    if (This->request->verb != BINDVERB_GET)
    {
        FIXME("only GET verb supported. Got %d\n", This->request->verb);
        return E_FAIL;
    }

    pbindinfo->dwBindVerb = This->request->verb;

    return S_OK;
}

static HRESULT WINAPI BindStatusCallback_OnDataAvailable(IBindStatusCallback *iface,
        DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%08x %d %p %p): stub\n", This, grfBSCF, dwSize, pformatetc, pstgmed);

    return E_NOTIMPL;
}

static HRESULT WINAPI BindStatusCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown *punk)
{
    BindStatusCallback *This = impl_from_IBindStatusCallback(iface);

    FIXME("(%p)->(%s %p): stub\n", This, debugstr_guid(riid), punk);

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

static HRESULT WINAPI BSCHttpNegotiate_QueryInterface(IHttpNegotiate *iface,
        REFIID riid, void **ppv)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_QueryInterface((IBindStatusCallback*)This, riid, ppv);
}

static ULONG WINAPI BSCHttpNegotiate_AddRef(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_AddRef((IBindStatusCallback*)This);
}

static ULONG WINAPI BSCHttpNegotiate_Release(IHttpNegotiate *iface)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    return IBindStatusCallback_Release((IBindStatusCallback*)This);
}

static HRESULT WINAPI BSCHttpNegotiate_BeginningTransaction(IHttpNegotiate *iface,
        LPCWSTR url, LPCWSTR headers, DWORD reserved, LPWSTR *add_headers)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);
    const struct reqheader *entry;
    WCHAR *buff, *ptr;

    TRACE("(%p)->(%s %s %d %p)\n", This, debugstr_w(url), debugstr_w(headers), reserved, add_headers);

    *add_headers = NULL;

    if (list_empty(&This->request->reqheaders)) return S_OK;

    buff = CoTaskMemAlloc(This->request->reqheader_size*sizeof(WCHAR));
    if (!buff) return E_OUTOFMEMORY;

    ptr = buff;
    LIST_FOR_EACH_ENTRY(entry, &This->request->reqheaders, struct reqheader, entry)
    {
        lstrcpyW(ptr, entry->header);
        ptr += SysStringLen(entry->header);

        lstrcpyW(ptr, colspaceW);
        ptr += sizeof(colspaceW)/sizeof(WCHAR)-1;

        lstrcpyW(ptr, entry->value);
        ptr += SysStringLen(entry->value);

        lstrcpyW(ptr, crlfW);
        ptr += sizeof(crlfW)/sizeof(WCHAR)-1;
    }

    *add_headers = buff;

    return S_OK;
}

static HRESULT WINAPI BSCHttpNegotiate_OnResponse(IHttpNegotiate *iface, DWORD code,
        LPCWSTR resp_headers, LPCWSTR req_headers, LPWSTR *add_reqheaders)
{
    BindStatusCallback *This = impl_from_IHttpNegotiate(iface);

    TRACE("(%p)->(%d %s %s %p)\n", This, code, debugstr_w(resp_headers),
          debugstr_w(req_headers), add_reqheaders);

    return S_OK;
}

#undef HTTPNEG2_THIS

static const IHttpNegotiateVtbl BSCHttpNegotiateVtbl = {
    BSCHttpNegotiate_QueryInterface,
    BSCHttpNegotiate_AddRef,
    BSCHttpNegotiate_Release,
    BSCHttpNegotiate_BeginningTransaction,
    BSCHttpNegotiate_OnResponse
};

static HRESULT BindStatusCallback_create(const httprequest* This, BindStatusCallback **obj)
{
    BindStatusCallback *bsc;
    IBindCtx *pbc;
    HRESULT hr;

    hr = CreateBindCtx(0, &pbc);
    if (hr != S_OK) return hr;

    bsc = heap_alloc(sizeof(*bsc));
    if (!bsc)
    {
        IBindCtx_Release(pbc);
        return E_OUTOFMEMORY;
    }

    bsc->lpBindStatusCallbackVtbl = &BindStatusCallbackVtbl;
    bsc->lpHttpNegotiateVtbl = &BSCHttpNegotiateVtbl;
    bsc->ref = 1;
    bsc->request = This;
    bsc->binding = NULL;

    hr = RegisterBindStatusCallback(pbc, (IBindStatusCallback*)bsc, NULL, 0);
    if (hr == S_OK)
    {
        IMoniker *moniker;

        hr = CreateURLMoniker(NULL, This->url, &moniker);
        if (hr == S_OK)
        {
            IStream *stream;

            hr = IMoniker_BindToStorage(moniker, pbc, NULL, &IID_IStream, (void**)&stream);
            IMoniker_Release(moniker);
            if (stream) IStream_Release(stream);
        }
        IBindCtx_Release(pbc);
    }

    if (FAILED(hr))
    {
        IBindStatusCallback_Release((IBindStatusCallback*)bsc);
        bsc = NULL;
    }

    *obj = bsc;
    return hr;
}

/* TODO: process OnChange callback */
static void httprequest_setreadystate(httprequest *This, READYSTATE state)
{
    This->state = state;
}

static HRESULT WINAPI httprequest_QueryInterface(IXMLHTTPRequest *iface, REFIID riid, void **ppvObject)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLHTTPRequest) ||
         IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLHTTPRequest_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI httprequest_AddRef(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%u)\n", This, ref );
    return ref;
}

static ULONG WINAPI httprequest_Release(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%u)\n", This, ref );

    if ( ref == 0 )
    {
        struct reqheader *header, *header2;

        SysFreeString(This->url);
        SysFreeString(This->user);
        SysFreeString(This->password);

        /* request headers */
        LIST_FOR_EACH_ENTRY_SAFE(header, header2, &This->reqheaders, struct reqheader, entry)
        {
            list_remove(&header->entry);
            SysFreeString(header->header);
            SysFreeString(header->value);
        }

        /* detach callback object */
        BindStatusCallback_Detach(This->bsc);

        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI httprequest_GetTypeInfoCount(IXMLHTTPRequest *iface, UINT *pctinfo)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI httprequest_GetTypeInfo(IXMLHTTPRequest *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLHTTPRequest_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI httprequest_GetIDsOfNames(IXMLHTTPRequest *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI httprequest_Invoke(IXMLHTTPRequest *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI httprequest_open(IXMLHTTPRequest *iface, BSTR method, BSTR url,
        VARIANT async, VARIANT user, VARIANT password)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    HRESULT hr;
    VARIANT str;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(method), debugstr_w(url));

    if (!method || !url) return E_INVALIDARG;

    /* free previously set data */
    SysFreeString(This->url);
    SysFreeString(This->user);
    SysFreeString(This->password);
    This->url = This->user = This->password = NULL;

    if (lstrcmpiW(method, MethodGetW) == 0)
    {
        This->verb = BINDVERB_GET;
    }
    else if (lstrcmpiW(method, MethodPutW) == 0)
    {
        This->verb = BINDVERB_PUT;
    }
    else if (lstrcmpiW(method, MethodPostW) == 0)
    {
        This->verb = BINDVERB_POST;
    }
    else
    {
        FIXME("unsupported request type %s\n", debugstr_w(method));
        This->verb = -1;
        return E_FAIL;
    }

    This->url = SysAllocString(url);

    hr = VariantChangeType(&async, &async, 0, VT_BOOL);
    This->async = hr == S_OK && V_BOOL(&async) == VARIANT_TRUE;

    VariantInit(&str);
    hr = VariantChangeType(&str, &user, 0, VT_BSTR);
    if (hr == S_OK)
        This->user = V_BSTR(&str);

    hr = VariantChangeType(&str, &password, 0, VT_BSTR);
    if (hr == S_OK)
        This->password = V_BSTR(&str);

    httprequest_setreadystate(This, READYSTATE_LOADING);

    return S_OK;
}

static HRESULT WINAPI httprequest_setRequestHeader(IXMLHTTPRequest *iface, BSTR header, BSTR value)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    struct reqheader *entry;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(header), debugstr_w(value));

    if (!header || !*header) return E_INVALIDARG;
    if (This->state != READYSTATE_LOADING) return E_FAIL;
    if (!value) return E_INVALIDARG;

    /* replace existing header value if already added */
    LIST_FOR_EACH_ENTRY(entry, &This->reqheaders, struct reqheader, entry)
    {
        if (lstrcmpW(entry->header, header) == 0)
        {
            LONG length = SysStringLen(entry->value);
            HRESULT hr;

            hr = SysReAllocString(&entry->value, value) ? S_OK : E_OUTOFMEMORY;

            if (hr == S_OK)
                This->reqheader_size += (SysStringLen(entry->value) - length);

            return hr;
        }
    }

    entry = heap_alloc(sizeof(*entry));
    if (!entry) return E_OUTOFMEMORY;

    /* new header */
    entry->header = SysAllocString(header);
    entry->value  = SysAllocString(value);

    /* header length including null terminator */
    This->reqheader_size += SysStringLen(entry->header) + sizeof(colspaceW)/sizeof(WCHAR) +
                            SysStringLen(entry->value)  + sizeof(crlfW)/sizeof(WCHAR) - 1;

    list_add_head(&This->reqheaders, &entry->entry);

    return S_OK;
}

static HRESULT WINAPI httprequest_getResponseHeader(IXMLHTTPRequest *iface, BSTR bstrHeader, BSTR *pbstrValue)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p) %s %p\n", This, debugstr_w(bstrHeader), pbstrValue);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_getAllResponseHeaders(IXMLHTTPRequest *iface, BSTR *pbstrHeaders)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p) %p\n", This, pbstrHeaders);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_send(IXMLHTTPRequest *iface, VARIANT varBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    BindStatusCallback *bsc = NULL;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (This->state != READYSTATE_LOADING) return E_FAIL;

    hr = BindStatusCallback_create(This, &bsc);
    if (FAILED(hr)) return hr;

    BindStatusCallback_Detach(This->bsc);
    This->bsc = bsc;

    return hr;
}

static HRESULT WINAPI httprequest_abort(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_status(IXMLHTTPRequest *iface, LONG *plStatus)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, plStatus);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_statusText(IXMLHTTPRequest *iface, BSTR *pbstrStatus)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pbstrStatus);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseXML(IXMLHTTPRequest *iface, IDispatch **ppBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, ppBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseText(IXMLHTTPRequest *iface, BSTR *pbstrBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pbstrBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseBody(IXMLHTTPRequest *iface, VARIANT *pvarBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pvarBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseStream(IXMLHTTPRequest *iface, VARIANT *pvarBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pvarBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_readyState(IXMLHTTPRequest *iface, LONG *state)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    TRACE("(%p)->(%p)\n", This, state);

    if (!state) return E_INVALIDARG;

    *state = This->state;
    return S_OK;
}

static HRESULT WINAPI httprequest_put_onreadystatechange(IXMLHTTPRequest *iface, IDispatch *pReadyStateSink)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pReadyStateSink);

    return E_NOTIMPL;
}

static const struct IXMLHTTPRequestVtbl dimimpl_vtbl =
{
    httprequest_QueryInterface,
    httprequest_AddRef,
    httprequest_Release,
    httprequest_GetTypeInfoCount,
    httprequest_GetTypeInfo,
    httprequest_GetIDsOfNames,
    httprequest_Invoke,
    httprequest_open,
    httprequest_setRequestHeader,
    httprequest_getResponseHeader,
    httprequest_getAllResponseHeaders,
    httprequest_send,
    httprequest_abort,
    httprequest_get_status,
    httprequest_get_statusText,
    httprequest_get_responseXML,
    httprequest_get_responseText,
    httprequest_get_responseBody,
    httprequest_get_responseStream,
    httprequest_get_readyState,
    httprequest_put_onreadystatechange
};

HRESULT XMLHTTPRequest_create(IUnknown *pUnkOuter, void **ppObj)
{
    httprequest *req;
    HRESULT hr = S_OK;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    req = heap_alloc( sizeof (*req) );
    if( !req )
        return E_OUTOFMEMORY;

    req->lpVtbl = &dimimpl_vtbl;
    req->ref = 1;

    req->async = FALSE;
    req->verb = -1;
    req->url = req->user = req->password = NULL;
    req->state = READYSTATE_UNINITIALIZED;
    req->bsc = NULL;
    req->reqheader_size = 0;
    list_init(&req->reqheaders);

    *ppObj = &req->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);

    return hr;
}

#else

HRESULT XMLHTTPRequest_create(IUnknown *pUnkOuter, void **ppObj)
{
    MESSAGE("This program tried to use a XMLHTTPRequest object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
