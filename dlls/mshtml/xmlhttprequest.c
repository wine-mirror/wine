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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLXMLHttpRequestFactory_iface,
            &HTMLXMLHttpRequestFactory_dispex);

    *ret_ptr = ret;
    return S_OK;
}
