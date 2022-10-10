/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

#include "combaseapi.h"
#include "initguid.h"
#include "uia_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

HMODULE huia_module;

struct uia_object_wrapper
{
    IUnknown IUnknown_iface;
    LONG refcount;

    IUnknown *marshaler;
    IUnknown *marshal_object;
};

static struct uia_object_wrapper *impl_uia_object_wrapper_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct uia_object_wrapper, IUnknown_iface);
}

static HRESULT WINAPI uia_object_wrapper_QueryInterface(IUnknown *iface,
        REFIID riid, void **ppv)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    return IUnknown_QueryInterface(wrapper->marshal_object, riid, ppv);
}

static ULONG WINAPI uia_object_wrapper_AddRef(IUnknown *iface)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&wrapper->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI uia_object_wrapper_Release(IUnknown *iface)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&wrapper->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);
    if (!refcount)
    {
        IUnknown_Release(wrapper->marshaler);
        heap_free(wrapper);
    }

    return refcount;
}

static const IUnknownVtbl uia_object_wrapper_vtbl = {
    uia_object_wrapper_QueryInterface,
    uia_object_wrapper_AddRef,
    uia_object_wrapper_Release,
};

/*
 * When passing the ReservedNotSupportedValue/ReservedMixedAttributeValue
 * interface pointers across apartments within the same process, create a free
 * threaded marshaler so that the pointer value is preserved.
 */
static HRESULT create_uia_object_wrapper(IUnknown *reserved, void **ppv)
{
    struct uia_object_wrapper *wrapper;
    HRESULT hr;

    TRACE("%p, %p\n", reserved, ppv);

    wrapper = heap_alloc(sizeof(*wrapper));
    if (!wrapper)
        return E_OUTOFMEMORY;

    wrapper->IUnknown_iface.lpVtbl = &uia_object_wrapper_vtbl;
    wrapper->marshal_object = reserved;
    wrapper->refcount = 1;

    if (FAILED(hr = CoCreateFreeThreadedMarshaler(&wrapper->IUnknown_iface, &wrapper->marshaler)))
    {
        heap_free(wrapper);
        return hr;
    }

    hr = IUnknown_QueryInterface(wrapper->marshaler, &IID_IMarshal, ppv);
    IUnknown_Release(&wrapper->IUnknown_iface);

    return hr;
}

/*
 * UiaReservedNotSupportedValue/UiaReservedMixedAttributeValue object.
 */
static HRESULT WINAPI uia_reserved_obj_QueryInterface(IUnknown *iface,
        REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMarshal))
        return create_uia_object_wrapper(iface, ppv);
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI uia_reserved_obj_AddRef(IUnknown *iface)
{
    return 1;
}

static ULONG WINAPI uia_reserved_obj_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl uia_reserved_obj_vtbl = {
    uia_reserved_obj_QueryInterface,
    uia_reserved_obj_AddRef,
    uia_reserved_obj_Release,
};

static IUnknown uia_reserved_ns_iface = {&uia_reserved_obj_vtbl};
static IUnknown uia_reserved_ma_iface = {&uia_reserved_obj_vtbl};

/*
 * UiaHostProviderFromHwnd IRawElementProviderSimple interface.
 */
struct hwnd_host_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    LONG refcount;

    HWND hwnd;
};

static inline struct hwnd_host_provider *impl_from_hwnd_host_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct hwnd_host_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI hwnd_host_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI hwnd_host_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);
    ULONG refcount = InterlockedIncrement(&host_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

ULONG WINAPI hwnd_host_provider_Release(IRawElementProviderSimple *iface)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);
    ULONG refcount = InterlockedDecrement(&host_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
        heap_free(host_prov);

    return refcount;
}

HRESULT WINAPI hwnd_host_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ServerSideProvider;
    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    TRACE("%p, %d, %p\n", iface, pattern_id, ret_val);
    *ret_val = NULL;
    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct hwnd_host_provider *host_prov = impl_from_hwnd_host_provider(iface);

    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    switch (prop_id)
    {
    case UIA_NativeWindowHandlePropertyId:
        V_VT(ret_val) = VT_I4;
        V_I4(ret_val) = HandleToUlong(host_prov->hwnd);
        break;

    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: HWND Provider Proxy");
        break;

    default:
        break;
    }

    return S_OK;
}

HRESULT WINAPI hwnd_host_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = NULL;
    return S_OK;
}

static const IRawElementProviderSimpleVtbl hwnd_host_provider_vtbl = {
    hwnd_host_provider_QueryInterface,
    hwnd_host_provider_AddRef,
    hwnd_host_provider_Release,
    hwnd_host_provider_get_ProviderOptions,
    hwnd_host_provider_GetPatternProvider,
    hwnd_host_provider_GetPropertyValue,
    hwnd_host_provider_get_HostRawElementProvider,
};

/***********************************************************************
 *          UiaClientsAreListening (uiautomationcore.@)
 */
BOOL WINAPI UiaClientsAreListening(void)
{
    FIXME("()\n");
    return FALSE;
}

/***********************************************************************
 *          UiaGetReservedMixedAttributeValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedMixedAttributeValue(IUnknown **value)
{
    TRACE("(%p)\n", value);

    if (!value)
        return E_INVALIDARG;

    *value = &uia_reserved_ma_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaGetReservedNotSupportedValue (uiautomationcore.@)
 */
HRESULT WINAPI UiaGetReservedNotSupportedValue(IUnknown **value)
{
    TRACE("(%p)\n", value);

    if (!value)
        return E_INVALIDARG;

    *value = &uia_reserved_ns_iface;

    return S_OK;
}

/***********************************************************************
 *          UiaRaiseAutomationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *provider, EVENTID id)
{
    FIXME("(%p, %d): stub\n", provider, id);
    return S_OK;
}

/***********************************************************************
 *          UiaRaiseAutomationPropertyChangedEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationPropertyChangedEvent(IRawElementProviderSimple *provider, PROPERTYID id, VARIANT old, VARIANT new)
{
    FIXME("(%p, %d, %s, %s): stub\n", provider, id, debugstr_variant(&old), debugstr_variant(&new));
    return S_OK;
}

void WINAPI UiaRegisterProviderCallback(UiaProviderCallback *callback)
{
    FIXME("(%p): stub\n", callback);
}

HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **provider)
{
    struct hwnd_host_provider *host_prov;

    TRACE("(%p, %p)\n", hwnd, provider);

    if (provider)
        *provider = NULL;

    if (!IsWindow(hwnd) || !provider)
        return E_INVALIDARG;

    host_prov = heap_alloc(sizeof(*host_prov));
    if (!host_prov)
        return E_OUTOFMEMORY;

    host_prov->IRawElementProviderSimple_iface.lpVtbl = &hwnd_host_provider_vtbl;
    host_prov->refcount = 1;
    host_prov->hwnd = hwnd;
    *provider = &host_prov->IRawElementProviderSimple_iface;

    return S_OK;
}

/***********************************************************************
 *          DllMain (uiautomationcore.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, void *reserved)
{
    TRACE("(%p, %ld, %p)\n", hinst, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        huia_module = hinst;
        break;

    default:
        break;
    }

    return TRUE;
}
