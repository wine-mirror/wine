/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlobj.h"

#include "mshtml_private.h"
#include "pluginhost.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    IPropertyBag  IPropertyBag_iface;
    IPropertyBag2 IPropertyBag2_iface;

    LONG ref;
} PropertyBag;

static inline PropertyBag *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, PropertyBag, IPropertyBag_iface);
}

static HRESULT WINAPI PropertyBag_QueryInterface(IPropertyBag *iface, REFIID riid, void **ppv)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IPropertyBag_iface;
    }else if(IsEqualGUID(&IID_IPropertyBag, riid)) {
        TRACE("(%p)->(IID_IPropertyBag %p)\n", This, ppv);
        *ppv = &This->IPropertyBag_iface;
    }else if(IsEqualGUID(&IID_IPropertyBag2, riid)) {
        TRACE("(%p)->(IID_IPropertyBag2 %p)\n", This, ppv);
        *ppv = &This->IPropertyBag2_iface;
    }else {
        WARN("Unsopported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyBag_AddRef(IPropertyBag *iface)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI PropertyBag_Release(IPropertyBag *iface)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI PropertyBag_Read(IPropertyBag *iface, LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(pszPropName), pVar, pErrorLog);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag_Write(IPropertyBag *iface, LPCOLESTR pszPropName, VARIANT *pVar)
{
    PropertyBag *This = impl_from_IPropertyBag(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(pszPropName), debugstr_variant(pVar));
    return E_NOTIMPL;
}

static const IPropertyBagVtbl PropertyBagVtbl = {
    PropertyBag_QueryInterface,
    PropertyBag_AddRef,
    PropertyBag_Release,
    PropertyBag_Read,
    PropertyBag_Write
};

static inline PropertyBag *impl_from_IPropertyBag2(IPropertyBag2 *iface)
{
    return CONTAINING_RECORD(iface, PropertyBag, IPropertyBag2_iface);
}

static HRESULT WINAPI PropertyBag2_QueryInterface(IPropertyBag2 *iface, REFIID riid, void **ppv)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_QueryInterface(&This->IPropertyBag_iface, riid, ppv);
}

static ULONG WINAPI PropertyBag2_AddRef(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_AddRef(&This->IPropertyBag_iface);
}

static ULONG WINAPI PropertyBag2_Release(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    return IPropertyBag_Release(&This->IPropertyBag_iface);
}

static HRESULT WINAPI PropertyBag2_Read(IPropertyBag2 *iface, ULONG cProperties, PROPBAG2 *pPropBag,
        IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%d %p %p %p %p)\n", This, cProperties, pPropBag, pErrLog, pvarValue, phrError);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_Write(IPropertyBag2 *iface, ULONG cProperties, PROPBAG2 *pPropBag, VARIANT *pvarValue)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%d %p %s)\n", This, cProperties, pPropBag, debugstr_variant(pvarValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_CountProperties(IPropertyBag2 *iface, ULONG *pcProperties)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%p)\n", This, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_GetPropertyInfo(IPropertyBag2 *iface, ULONG iProperty, ULONG cProperties,
        PROPBAG2 *pPropBag, ULONG *pcProperties)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%u %u %p %p)\n", This, iProperty, cProperties, pPropBag, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag2_LoadObject(IPropertyBag2 *iface, LPCOLESTR pstrName, DWORD dwHint,
        IUnknown *pUnkObject, IErrorLog *pErrLog)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    FIXME("(%p)->(%s %x %p %p)\n", This, debugstr_w(pstrName), dwHint, pUnkObject, pErrLog);
    return E_NOTIMPL;
}

static const IPropertyBag2Vtbl PropertyBag2Vtbl = {
    PropertyBag2_QueryInterface,
    PropertyBag2_AddRef,
    PropertyBag2_Release,
    PropertyBag2_Read,
    PropertyBag2_Write,
    PropertyBag2_CountProperties,
    PropertyBag2_GetPropertyInfo,
    PropertyBag2_LoadObject
};

HRESULT create_param_prop_bag(IPropertyBag **ret)
{
    PropertyBag *prop_bag;

    prop_bag = heap_alloc(sizeof(*prop_bag));
    if(!prop_bag)
        return E_OUTOFMEMORY;

    prop_bag->IPropertyBag_iface.lpVtbl  = &PropertyBagVtbl;
    prop_bag->IPropertyBag2_iface.lpVtbl = &PropertyBag2Vtbl;
    prop_bag->ref = 1;

    *ret = &prop_bag->IPropertyBag_iface;
    return S_OK;
}
