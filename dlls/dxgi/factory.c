/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IDXGIFactory *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIFactory))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IDXGIFactory *iface)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IDXGIFactory *iface)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IDXGIFactory *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IDXGIFactory *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IDXGIFactory *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IDXGIFactory *iface, REFIID riid, void **parent)
{
    FIXME("iface %p, riid %s, parent %p stub!\n", iface, debugstr_guid(riid), parent);

    return E_NOTIMPL;
}

/* IDXGIFactory methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IDXGIFactory *iface, UINT adapter_idx, IDXGIAdapter **adapter)
{
    FIXME("iface %p, adapter_idx %u, adapter %p stub!\n", iface, adapter_idx, adapter);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IDXGIFactory *iface, HWND window, UINT flags)
{
    FIXME("iface %p, window %p, flags %#x stub!\n\n", iface, window, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IDXGIFactory *iface, HWND *window)
{
    FIXME("iface %p, window %p stub!\n", iface, window);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IDXGIFactory *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    struct dxgi_swapchain *object;

    FIXME("iface %p, device %p, desc %p, swapchain %p partial stub!\n", iface, device, desc, swapchain);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI swapchain object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &dxgi_swapchain_vtbl;
    object->refcount = 1;
    *swapchain = (IDXGISwapChain *)object;

    TRACE("Created IDXGISwapChain %p\n", object);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IDXGIFactory *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    FIXME("iface %p, swrast %p, adapter %p stub!\n", iface, swrast, adapter);

    return E_NOTIMPL;
}

const struct IDXGIFactoryVtbl dxgi_factory_vtbl =
{
    /* IUnknown methods */
    dxgi_factory_QueryInterface,
    dxgi_factory_AddRef,
    dxgi_factory_Release,
    /* IDXGIObject methods */
    dxgi_factory_SetPrivateData,
    dxgi_factory_SetPrivateDataInterface,
    dxgi_factory_GetPrivateData,
    dxgi_factory_GetParent,
    /* IDXGIFactory methods */
    dxgi_factory_EnumAdapters,
    dxgi_factory_MakeWindowAssociation,
    dxgi_factory_GetWindowAssociation,
    dxgi_factory_CreateSwapChain,
    dxgi_factory_CreateSoftwareAdapter,
};
