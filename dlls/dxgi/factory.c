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

static inline struct dxgi_factory *impl_from_IDXGIFactory1(IDXGIFactory1 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_factory, IDXGIFactory1_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IDXGIFactory1 *iface, REFIID iid, void **out)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if ((factory->extended && IsEqualGUID(iid, &IID_IDXGIFactory1))
            || IsEqualGUID(iid, &IID_IDXGIFactory)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IDXGIFactory1 *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IDXGIFactory1 *iface)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        if (factory->device_window)
            DestroyWindow(factory->device_window);

        wined3d_mutex_lock();
        wined3d_decref(factory->wined3d);
        wined3d_mutex_unlock();
        wined3d_private_store_cleanup(&factory->private_store);
        HeapFree(GetProcessHeap(), 0, factory);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IDXGIFactory1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&factory->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IDXGIFactory1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&factory->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IDXGIFactory1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&factory->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IDXGIFactory1 *iface, REFIID iid, void **parent)
{
    WARN("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    *parent = NULL;

    return E_NOINTERFACE;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters1(IDXGIFactory1 *iface,
        UINT adapter_idx, IDXGIAdapter1 **adapter)
{
    struct dxgi_factory *factory = impl_from_IDXGIFactory1(iface);
    struct dxgi_adapter *adapter_object;
    UINT adapter_count;
    HRESULT hr;

    TRACE("iface %p, adapter_idx %u, adapter %p.\n", iface, adapter_idx, adapter);

    if (!adapter)
        return DXGI_ERROR_INVALID_CALL;

    wined3d_mutex_lock();
    adapter_count = wined3d_get_adapter_count(factory->wined3d);
    wined3d_mutex_unlock();

    if (adapter_idx >= adapter_count)
    {
        *adapter = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    if (FAILED(hr = dxgi_adapter_create(factory, adapter_idx, &adapter_object)))
    {
        *adapter = NULL;
        return hr;
    }

    *adapter = &adapter_object->IDXGIAdapter1_iface;

    TRACE("Returning adapter %p.\n", *adapter);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IDXGIFactory1 *iface,
        UINT adapter_idx, IDXGIAdapter **adapter)
{
    TRACE("iface %p, adapter_idx %u, adapter %p.\n", iface, adapter_idx, adapter);

    return dxgi_factory_EnumAdapters1(iface, adapter_idx, (IDXGIAdapter1 **)adapter);
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IDXGIFactory1 *iface, HWND window, UINT flags)
{
    FIXME("iface %p, window %p, flags %#x stub!\n", iface, window, flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IDXGIFactory1 *iface, HWND *window)
{
    FIXME("iface %p, window %p stub!\n", iface, window);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IDXGIFactory1 *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_desc wined3d_desc;
    unsigned int min_buffer_count;
    IWineDXGIDevice *dxgi_device;
    HRESULT hr;

    FIXME("iface %p, device %p, desc %p, swapchain %p partial stub!\n", iface, device, desc, swapchain);

    switch (desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_DISCARD:
        case DXGI_SWAP_EFFECT_SEQUENTIAL:
            min_buffer_count = 1;
            break;

        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
            min_buffer_count = 2;
            break;

        default:
            WARN("Invalid swap effect %u used, returning DXGI_ERROR_INVALID_CALL.\n", desc->SwapEffect);
            return DXGI_ERROR_INVALID_CALL;
    }

    if (desc->BufferCount < min_buffer_count || desc->BufferCount > 16)
    {
        WARN("BufferCount is %u, returning DXGI_ERROR_INVALID_CALL.\n", desc->BufferCount);
        return DXGI_ERROR_INVALID_CALL;
    }
    if (!desc->OutputWindow)
    {
        FIXME("No output window, should use factory output window.\n");
    }

    hr = IUnknown_QueryInterface(device, &IID_IWineDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        ERR("This is not the device we're looking for\n");
        return hr;
    }

    FIXME("Ignoring SwapEffect %#x.\n", desc->SwapEffect);

    wined3d_desc.backbuffer_width = desc->BufferDesc.Width;
    wined3d_desc.backbuffer_height = desc->BufferDesc.Height;
    wined3d_desc.backbuffer_format = wined3dformat_from_dxgi_format(desc->BufferDesc.Format);
    wined3d_desc.backbuffer_count = desc->BufferCount;
    wined3d_sample_desc_from_dxgi(&wined3d_desc.multisample_type,
            &wined3d_desc.multisample_quality, &desc->SampleDesc);
    wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
    wined3d_desc.device_window = desc->OutputWindow;
    wined3d_desc.windowed = desc->Windowed;
    wined3d_desc.enable_auto_depth_stencil = FALSE;
    wined3d_desc.auto_depth_stencil_format = 0;
    wined3d_desc.flags = wined3d_swapchain_flags_from_dxgi(desc->Flags);
    wined3d_desc.refresh_rate = dxgi_rational_to_uint(&desc->BufferDesc.RefreshRate);
    wined3d_desc.swap_interval = WINED3DPRESENT_INTERVAL_DEFAULT;
    wined3d_desc.auto_restore_display_mode = TRUE;

    hr = IWineDXGIDevice_create_swapchain(dxgi_device, &wined3d_desc, FALSE, &wined3d_swapchain);
    IWineDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create swapchain, hr %#x.\n", hr);
        return hr;
    }

    wined3d_mutex_lock();
    *swapchain = wined3d_swapchain_get_parent(wined3d_swapchain);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IDXGIFactory1 *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    FIXME("iface %p, swrast %p, adapter %p stub!\n", iface, swrast, adapter);

    return E_NOTIMPL;
}

static BOOL STDMETHODCALLTYPE dxgi_factory_IsCurrent(IDXGIFactory1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return TRUE;
}

static const struct IDXGIFactory1Vtbl dxgi_factory_vtbl =
{
    dxgi_factory_QueryInterface,
    dxgi_factory_AddRef,
    dxgi_factory_Release,
    dxgi_factory_SetPrivateData,
    dxgi_factory_SetPrivateDataInterface,
    dxgi_factory_GetPrivateData,
    dxgi_factory_GetParent,
    dxgi_factory_EnumAdapters,
    dxgi_factory_MakeWindowAssociation,
    dxgi_factory_GetWindowAssociation,
    dxgi_factory_CreateSwapChain,
    dxgi_factory_CreateSoftwareAdapter,
    dxgi_factory_EnumAdapters1,
    dxgi_factory_IsCurrent,
};

struct dxgi_factory *unsafe_impl_from_IDXGIFactory1(IDXGIFactory1 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &dxgi_factory_vtbl);
    return CONTAINING_RECORD(iface, struct dxgi_factory, IDXGIFactory1_iface);
}

static HRESULT dxgi_factory_init(struct dxgi_factory *factory, BOOL extended)
{
    factory->IDXGIFactory1_iface.lpVtbl = &dxgi_factory_vtbl;
    factory->refcount = 1;
    wined3d_private_store_init(&factory->private_store);

    wined3d_mutex_lock();
    factory->wined3d = wined3d_create(0);
    wined3d_mutex_unlock();
    if (!factory->wined3d)
    {
        wined3d_private_store_cleanup(&factory->private_store);
        return DXGI_ERROR_UNSUPPORTED;
    }

    factory->extended = extended;

    return S_OK;
}

HRESULT dxgi_factory_create(REFIID riid, void **factory, BOOL extended)
{
    struct dxgi_factory *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = dxgi_factory_init(object, extended)))
    {
        WARN("Failed to initialize factory, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created factory %p.\n", object);

    hr = IDXGIFactory1_QueryInterface(&object->IDXGIFactory1_iface, riid, factory);
    IDXGIFactory1_Release(&object->IDXGIFactory1_iface);

    return hr;
}

HWND dxgi_factory_get_device_window(struct dxgi_factory *factory)
{
    wined3d_mutex_lock();

    if (!factory->device_window)
    {
        if (!(factory->device_window = CreateWindowA("static", "DXGI device window",
                WS_DISABLED, 0, 0, 0, 0, NULL, NULL, NULL, NULL)))
        {
            wined3d_mutex_unlock();
            ERR("Failed to create a window.\n");
            return NULL;
        }
        TRACE("Created device window %p for factory %p.\n", factory->device_window, factory);
    }

    wined3d_mutex_unlock();

    return factory->device_window;
}
