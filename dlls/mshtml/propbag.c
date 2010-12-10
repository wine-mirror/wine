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

HRESULT create_param_prop_bag(IPropertyBag **ret)
{
    PropertyBag *prop_bag;

    prop_bag = heap_alloc(sizeof(*prop_bag));
    if(!prop_bag)
        return E_OUTOFMEMORY;

    prop_bag->IPropertyBag_iface.lpVtbl  = &PropertyBagVtbl;
    prop_bag->ref = 1;

    *ret = &prop_bag->IPropertyBag_iface;
    return S_OK;
}
