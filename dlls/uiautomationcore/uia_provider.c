/*
 * Copyright 2022 Connor McAdams for CodeWeavers
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

#include "uiautomation.h"
#include "ocidl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static void variant_init_i4(VARIANT *v, int val)
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

/*
 * UiaProviderFromIAccessible IRawElementProviderSimple interface.
 */
struct msaa_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    LONG refcount;

    IAccessible *acc;
    VARIANT cid;
    HWND hwnd;
};

static inline struct msaa_provider *impl_from_msaa_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI msaa_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI msaa_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedIncrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

ULONG WINAPI msaa_provider_Release(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedDecrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
    {
        IAccessible_Release(msaa_prov->acc);
        heap_free(msaa_prov);
    }

    return refcount;
}

HRESULT WINAPI msaa_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

HRESULT WINAPI msaa_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    FIXME("%p, %d, %p: stub!\n", iface, pattern_id, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI msaa_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: MSAA Proxy");
        break;

    default:
        FIXME("Unimplemented propertyId %d\n", prop_id);
        break;
    }

    return S_OK;
}

HRESULT WINAPI msaa_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static const IRawElementProviderSimpleVtbl msaa_provider_vtbl = {
    msaa_provider_QueryInterface,
    msaa_provider_AddRef,
    msaa_provider_Release,
    msaa_provider_get_ProviderOptions,
    msaa_provider_GetPatternProvider,
    msaa_provider_GetPropertyValue,
    msaa_provider_get_HostRawElementProvider,
};

/***********************************************************************
 *          UiaProviderFromIAccessible (uiautomationcore.@)
 */
HRESULT WINAPI UiaProviderFromIAccessible(IAccessible *acc, long child_id, DWORD flags,
        IRawElementProviderSimple **elprov)
{
    struct msaa_provider *msaa_prov;
    IServiceProvider *serv_prov;
    HWND hwnd = NULL;
    IOleWindow *win;
    HRESULT hr;

    TRACE("(%p, %ld, %#lx, %p)\n", acc, child_id, flags, elprov);

    if (elprov)
        *elprov = NULL;

    if (!elprov)
        return E_POINTER;
    if (!acc)
        return E_INVALIDARG;

    if (flags != UIA_PFIA_DEFAULT)
    {
        FIXME("unsupported flags %#lx\n", flags);
        return E_NOTIMPL;
    }

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&serv_prov);
    if (SUCCEEDED(hr))
    {
        IUnknown *unk;

        hr = IServiceProvider_QueryService(serv_prov, &IIS_IsOleaccProxy, &IID_IUnknown, (void **)&unk);
        if (SUCCEEDED(hr))
        {
            WARN("Cannot wrap an oleacc proxy IAccessible!\n");
            IUnknown_Release(unk);
            IServiceProvider_Release(serv_prov);
            return E_INVALIDARG;
        }

        IServiceProvider_Release(serv_prov);
    }

    hr = IAccessible_QueryInterface(acc, &IID_IOleWindow, (void **)&win);
    if (SUCCEEDED(hr))
    {
        hr = IOleWindow_GetWindow(win, &hwnd);
        if (FAILED(hr))
            hwnd = NULL;
        IOleWindow_Release(win);
    }

    if (!IsWindow(hwnd))
    {
        VARIANT v, cid;

        VariantInit(&v);
        variant_init_i4(&cid, CHILDID_SELF);
        hr = IAccessible_accNavigate(acc, 10, cid, &v);
        if (SUCCEEDED(hr) && V_VT(&v) == VT_I4)
            hwnd = ULongToHandle(V_I4(&v));

        if (!IsWindow(hwnd))
            return E_FAIL;
    }

    msaa_prov = heap_alloc(sizeof(*msaa_prov));
    if (!msaa_prov)
        return E_OUTOFMEMORY;

    msaa_prov->IRawElementProviderSimple_iface.lpVtbl = &msaa_provider_vtbl;
    msaa_prov->refcount = 1;
    msaa_prov->hwnd = hwnd;
    variant_init_i4(&msaa_prov->cid, child_id);
    msaa_prov->acc = acc;
    IAccessible_AddRef(acc);
    *elprov = &msaa_prov->IRawElementProviderSimple_iface;

    return S_OK;
}
