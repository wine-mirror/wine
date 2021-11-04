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

#include "uiautomation.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

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

    TRACE("%p, refcount %d\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI uia_object_wrapper_Release(IUnknown *iface)
{
    struct uia_object_wrapper *wrapper = impl_uia_object_wrapper_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&wrapper->refcount);

    TRACE("%p, refcount %d\n", iface, refcount);
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
 *          UiaLookupId (uiautomationcore.@)
 */
int WINAPI UiaLookupId(enum AutomationIdentifierType type, const GUID *guid)
{
    FIXME("(%d, %s) stub!\n", type, debugstr_guid(guid));
    return 1;
}

/***********************************************************************
 *          UiaReturnRawElementProvider (uiautomationcore.@)
 */
LRESULT WINAPI UiaReturnRawElementProvider(HWND hwnd, WPARAM wParam,
        LPARAM lParam, IRawElementProviderSimple *elprov)
{
    FIXME("(%p, %lx, %lx, %p) stub!\n", hwnd, wParam, lParam, elprov);
    return 0;
}

/***********************************************************************
 *          UiaRaiseAutomationEvent (uiautomationcore.@)
 */
HRESULT WINAPI UiaRaiseAutomationEvent(IRawElementProviderSimple *provider, EVENTID id)
{
    FIXME("(%p, %d): stub\n", provider, id);
    return S_OK;
}

void WINAPI UiaRegisterProviderCallback(UiaProviderCallback *callback)
{
    FIXME("(%p): stub\n", callback);
}

HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple **provider)
{
    FIXME("(%p, %p): stub\n", hwnd, provider);
    return E_NOTIMPL;
}

HRESULT WINAPI UiaDisconnectProvider(IRawElementProviderSimple *provider)
{
    FIXME("(%p): stub\n", provider);
    return E_NOTIMPL;
}
