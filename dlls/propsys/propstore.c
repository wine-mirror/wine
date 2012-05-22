/*
 * standard IPropertyStore implementation
 *
 * Copyright 2012 Vincent Povirk for CodeWeavers
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
#include "objbase.h"
#include "rpcproxy.h"
#include "propsys.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "propsys_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

typedef struct {
    IPropertyStoreCache IPropertyStoreCache_iface;
    LONG ref;
} PropertyStore;

static inline PropertyStore *impl_from_IPropertyStoreCache(IPropertyStoreCache *iface)
{
    return CONTAINING_RECORD(iface, PropertyStore, IPropertyStoreCache_iface);
}

static HRESULT WINAPI PropertyStore_QueryInterface(IPropertyStoreCache *iface, REFIID iid,
    void **ppv)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IPropertyStore, iid) ||
        IsEqualIID(&IID_IPropertyStoreCache, iid))
    {
        *ppv = &This->IPropertyStoreCache_iface;
    }
    else
    {
        FIXME("No interface for %s\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyStore_AddRef(IPropertyStoreCache *iface)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PropertyStore_Release(IPropertyStoreCache *iface)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI PropertyStore_GetCount(IPropertyStoreCache *iface,
    DWORD *cProps)
{
    FIXME("%p,%p: stub\n", iface, cProps);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_GetAt(IPropertyStoreCache *iface,
    DWORD iProp, PROPERTYKEY *pkey)
{
    FIXME("%p,%d,%p: stub\n", iface, iProp, pkey);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_GetValue(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PROPVARIANT *pv)
{
    FIXME("%p,%p,%p: stub\n", iface, key, pv);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_SetValue(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
    FIXME("%p,%p,%p: stub\n", iface, key, propvar);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_Commit(IPropertyStoreCache *iface)
{
    FIXME("%p: stub\n", iface);
    return S_OK;
}

static HRESULT WINAPI PropertyStore_GetState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PSC_STATE *pstate)
{
    FIXME("%p,%p,%p: stub\n", iface, key, pstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_GetValueAndState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PROPVARIANT *ppropvar, PSC_STATE *pstate)
{
    FIXME("%p,%p,%p,%p: stub\n", iface, key, ppropvar, pstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_SetState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PSC_STATE pstate)
{
    FIXME("%p,%p,%d: stub\n", iface, key, pstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyStore_SetValueAndState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, const PROPVARIANT *ppropvar, PSC_STATE state)
{
    FIXME("%p,%p,%p,%d: stub\n", iface, key, ppropvar, state);
    return E_NOTIMPL;
}

static const IPropertyStoreCacheVtbl PropertyStore_Vtbl = {
    PropertyStore_QueryInterface,
    PropertyStore_AddRef,
    PropertyStore_Release,
    PropertyStore_GetCount,
    PropertyStore_GetAt,
    PropertyStore_GetValue,
    PropertyStore_SetValue,
    PropertyStore_Commit,
    PropertyStore_GetState,
    PropertyStore_GetValueAndState,
    PropertyStore_SetState,
    PropertyStore_SetValueAndState
};

HRESULT PropertyStore_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    PropertyStore *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PropertyStore));
    if (!This) return E_OUTOFMEMORY;

    This->IPropertyStoreCache_iface.lpVtbl = &PropertyStore_Vtbl;
    This->ref = 1;

    ret = IPropertyStoreCache_QueryInterface(&This->IPropertyStoreCache_iface, iid, ppv);
    IPropertyStoreCache_Release(&This->IPropertyStoreCache_iface);

    return ret;
}
