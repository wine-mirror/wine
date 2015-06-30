/*
 * Copyright 2015 Zhenbo Li
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
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT bstr_to_nsacstr(BSTR bstr, nsACString *str)
{
    char *cstr = heap_strdupWtoU(bstr);
    if(!cstr)
        return E_OUTOFMEMORY;
    nsACString_Init(str, cstr);
    heap_free(cstr);
    return S_OK;
}

static HRESULT variant_to_nsastr(VARIANT var, nsAString *ret)
{
    switch(V_VT(&var)) {
        case VT_NULL:
        case VT_ERROR:
        case VT_EMPTY:
            nsAString_Init(ret, NULL);
            return S_OK;
        case VT_BSTR:
            nsAString_InitDepend(ret, V_BSTR(&var));
            return S_OK;
        default:
            FIXME("Unsupported VARIANT: %s\n", debugstr_variant(&var));
            return E_INVALIDARG;
    }
}

/* IHTMLXMLHttpRequest */
typedef struct {
    EventTarget event_target;
    IHTMLXMLHttpRequest IHTMLXMLHttpRequest_iface;
    LONG ref;
    nsIXMLHttpRequest *nsxhr;
} HTMLXMLHttpRequest;

static inline HTMLXMLHttpRequest *impl_from_IHTMLXMLHttpRequest(IHTMLXMLHttpRequest *iface)
{
    return CONTAINING_RECORD(iface, HTMLXMLHttpRequest, IHTMLXMLHttpRequest_iface);
}

static HRESULT WINAPI HTMLXMLHttpRequest_QueryInterface(IHTMLXMLHttpRequest *iface, REFIID riid, void **ppv)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLXMLHttpRequest_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLXMLHttpRequest_iface;
    }else if(IsEqualGUID(&IID_IHTMLXMLHttpRequest, riid)) {
        *ppv = &This->IHTMLXMLHttpRequest_iface;
    }else if(dispex_query_interface(&This->event_target.dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLXMLHttpRequest_AddRef(IHTMLXMLHttpRequest *iface)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLXMLHttpRequest_Release(IHTMLXMLHttpRequest *iface)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->event_target.dispex);
        nsIXMLHttpRequest_Release(This->nsxhr);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLXMLHttpRequest_GetTypeInfoCount(IHTMLXMLHttpRequest *iface, UINT *pctinfo)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    return IDispatchEx_GetTypeInfoCount(&This->event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLXMLHttpRequest_GetTypeInfo(IHTMLXMLHttpRequest *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    return IDispatchEx_GetTypeInfo(&This->event_target.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLXMLHttpRequest_GetIDsOfNames(IHTMLXMLHttpRequest *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    return IDispatchEx_GetIDsOfNames(&This->event_target.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLXMLHttpRequest_Invoke(IHTMLXMLHttpRequest *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    return IDispatchEx_Invoke(&This->event_target.dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_readyState(IHTMLXMLHttpRequest *iface, LONG *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_responseBody(IHTMLXMLHttpRequest *iface, VARIANT *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_responseText(IHTMLXMLHttpRequest *iface, BSTR *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_responseXML(IHTMLXMLHttpRequest *iface, IDispatch **p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_status(IHTMLXMLHttpRequest *iface, LONG *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_statusText(IHTMLXMLHttpRequest *iface, BSTR *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_put_onreadystatechange(IHTMLXMLHttpRequest *iface, VARIANT v)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_event_handler(&This->event_target, EVENTID_READYSTATECHANGE, &v);
}

static HRESULT WINAPI HTMLXMLHttpRequest_get_onreadystatechange(IHTMLXMLHttpRequest *iface, VARIANT *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_event_handler(&This->event_target, EVENTID_READYSTATECHANGE, p);
}

static HRESULT WINAPI HTMLXMLHttpRequest_abort(IHTMLXMLHttpRequest *iface)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_open(IHTMLXMLHttpRequest *iface, BSTR bstrMethod, BSTR bstrUrl, VARIANT varAsync, VARIANT varUser, VARIANT varPassword)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    nsACString method, url;
    nsAString user, password;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %s %s %s %s)\n", This, debugstr_w(bstrMethod), debugstr_w(bstrUrl), debugstr_variant(&varAsync), debugstr_variant(&varUser), debugstr_variant(&varPassword));

    if(V_VT(&varAsync) != VT_BOOL) {
        FIXME("varAsync not supported: %s\n", debugstr_variant(&varAsync));
        return E_FAIL;
    }

    /* Note: Starting with Gecko 30.0 (Firefox 30.0 / Thunderbird 30.0 / SeaMonkey 2.27),
     * synchronous requests on the main thread have been deprecated due to the negative
     * effects to the user experience.
     */
    if(!V_BOOL(&varAsync)) {
        FIXME("Synchronous request is not supported yet\n");
        return E_FAIL;
    }

    hres = variant_to_nsastr(varUser, &user);
    if(FAILED(hres))
        return hres;
    hres = variant_to_nsastr(varPassword, &password);
    if(FAILED(hres)) {
        nsAString_Finish(&user);
        return hres;
    }

    hres = bstr_to_nsacstr(bstrMethod, &method);
    if(FAILED(hres)) {
        nsAString_Finish(&user);
        nsAString_Finish(&password);
        return hres;
    }
    hres = bstr_to_nsacstr(bstrUrl, &url);
    if(FAILED(hres)) {
        nsAString_Finish(&user);
        nsAString_Finish(&password);
        nsACString_Finish(&method);
        return hres;
    }

    nsres = nsIXMLHttpRequest_Open(This->nsxhr, &method, &url, TRUE,
            &user, &password, 0);

    nsACString_Finish(&method);
    nsACString_Finish(&url);
    nsAString_Finish(&user);
    nsAString_Finish(&password);

    if(NS_FAILED(nsres)) {
        ERR("nsIXMLHttpRequest_Open failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLXMLHttpRequest_send(IHTMLXMLHttpRequest *iface, VARIANT varBody)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&varBody));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_getAllResponseHeaders(IHTMLXMLHttpRequest *iface, BSTR *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_getResponseHeader(IHTMLXMLHttpRequest *iface, BSTR bstrHeader, BSTR *p)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrHeader), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLXMLHttpRequest_setRequestHeader(IHTMLXMLHttpRequest *iface, BSTR bstrHeader, BSTR bstrValue)
{
    HTMLXMLHttpRequest *This = impl_from_IHTMLXMLHttpRequest(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(bstrHeader), debugstr_w(bstrValue));
    return E_NOTIMPL;
}

static const IHTMLXMLHttpRequestVtbl HTMLXMLHttpRequestVtbl = {
    HTMLXMLHttpRequest_QueryInterface,
    HTMLXMLHttpRequest_AddRef,
    HTMLXMLHttpRequest_Release,
    HTMLXMLHttpRequest_GetTypeInfoCount,
    HTMLXMLHttpRequest_GetTypeInfo,
    HTMLXMLHttpRequest_GetIDsOfNames,
    HTMLXMLHttpRequest_Invoke,
    HTMLXMLHttpRequest_get_readyState,
    HTMLXMLHttpRequest_get_responseBody,
    HTMLXMLHttpRequest_get_responseText,
    HTMLXMLHttpRequest_get_responseXML,
    HTMLXMLHttpRequest_get_status,
    HTMLXMLHttpRequest_get_statusText,
    HTMLXMLHttpRequest_put_onreadystatechange,
    HTMLXMLHttpRequest_get_onreadystatechange,
    HTMLXMLHttpRequest_abort,
    HTMLXMLHttpRequest_open,
    HTMLXMLHttpRequest_send,
    HTMLXMLHttpRequest_getAllResponseHeaders,
    HTMLXMLHttpRequest_getResponseHeader,
    HTMLXMLHttpRequest_setRequestHeader
};

static inline HTMLXMLHttpRequest *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLXMLHttpRequest, event_target.dispex);
}

static void HTMLXMLHttpRequest_bind_event(DispatchEx *dispex, int eid)
{
    HTMLXMLHttpRequest *This = impl_from_DispatchEx(dispex);

    FIXME("(%p)\n", This);

    assert(eid == EVENTID_READYSTATECHANGE);
}

static dispex_static_data_vtbl_t HTMLXMLHttpRequest_dispex_vtbl = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLXMLHttpRequest_bind_event
};

static const tid_t HTMLXMLHttpRequest_iface_tids[] = {
    IHTMLXMLHttpRequest_tid,
    0
};
static dispex_static_data_t HTMLXMLHttpRequest_dispex = {
    &HTMLXMLHttpRequest_dispex_vtbl,
    DispHTMLXMLHttpRequest_tid,
    NULL,
    HTMLXMLHttpRequest_iface_tids
};


/* IHTMLXMLHttpRequestFactory */
static inline HTMLXMLHttpRequestFactory *impl_from_IHTMLXMLHttpRequestFactory(IHTMLXMLHttpRequestFactory *iface)
{
    return CONTAINING_RECORD(iface, HTMLXMLHttpRequestFactory, IHTMLXMLHttpRequestFactory_iface);
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_QueryInterface(IHTMLXMLHttpRequestFactory *iface, REFIID riid, void **ppv)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLXMLHttpRequestFactory_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLXMLHttpRequestFactory_iface;
    }else if(IsEqualGUID(&IID_IHTMLXMLHttpRequestFactory, riid)) {
        *ppv = &This->IHTMLXMLHttpRequestFactory_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLXMLHttpRequestFactory_AddRef(IHTMLXMLHttpRequestFactory *iface)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLXMLHttpRequestFactory_Release(IHTMLXMLHttpRequestFactory *iface)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_GetTypeInfoCount(IHTMLXMLHttpRequestFactory *iface, UINT *pctinfo)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_GetTypeInfo(IHTMLXMLHttpRequestFactory *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_GetIDsOfNames(IHTMLXMLHttpRequestFactory *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_Invoke(IHTMLXMLHttpRequestFactory *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLXMLHttpRequestFactory_create(IHTMLXMLHttpRequestFactory *iface, IHTMLXMLHttpRequest **p)
{
    HTMLXMLHttpRequestFactory *This = impl_from_IHTMLXMLHttpRequestFactory(iface);
    HTMLXMLHttpRequest        *ret;
    nsIXMLHttpRequest         *nsxhr;

    TRACE("(%p)->(%p)\n", This, p);

    nsxhr = create_nsxhr(This->window->base.outer_window->nswindow);
    if(!nsxhr)
        return E_FAIL;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret) {
        nsIXMLHttpRequest_Release(nsxhr);
        return E_OUTOFMEMORY;
    }
    ret->nsxhr = nsxhr;

    ret->IHTMLXMLHttpRequest_iface.lpVtbl = &HTMLXMLHttpRequestVtbl;
    init_dispex(&ret->event_target.dispex, (IUnknown*)&ret->IHTMLXMLHttpRequest_iface,
            &HTMLXMLHttpRequest_dispex);
    ret->ref = 1;

    *p = &ret->IHTMLXMLHttpRequest_iface;
    return S_OK;
}

static const IHTMLXMLHttpRequestFactoryVtbl HTMLXMLHttpRequestFactoryVtbl = {
    HTMLXMLHttpRequestFactory_QueryInterface,
    HTMLXMLHttpRequestFactory_AddRef,
    HTMLXMLHttpRequestFactory_Release,
    HTMLXMLHttpRequestFactory_GetTypeInfoCount,
    HTMLXMLHttpRequestFactory_GetTypeInfo,
    HTMLXMLHttpRequestFactory_GetIDsOfNames,
    HTMLXMLHttpRequestFactory_Invoke,
    HTMLXMLHttpRequestFactory_create
};

static const tid_t HTMLXMLHttpRequestFactory_iface_tids[] = {
    IHTMLXMLHttpRequestFactory_tid,
    0
};
static dispex_static_data_t HTMLXMLHttpRequestFactory_dispex = {
    NULL,
    IHTMLXMLHttpRequestFactory_tid,
    NULL,
    HTMLXMLHttpRequestFactory_iface_tids
};

HRESULT HTMLXMLHttpRequestFactory_Create(HTMLInnerWindow* window, HTMLXMLHttpRequestFactory **ret_ptr)
{
    HTMLXMLHttpRequestFactory *ret;

    ret = heap_alloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLXMLHttpRequestFactory_iface.lpVtbl = &HTMLXMLHttpRequestFactoryVtbl;
    ret->ref = 1;
    ret->window = window;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLXMLHttpRequestFactory_iface,
            &HTMLXMLHttpRequestFactory_dispex);

    *ret_ptr = ret;
    return S_OK;
}
