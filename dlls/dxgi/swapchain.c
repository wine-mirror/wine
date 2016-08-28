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

static inline struct dxgi_swapchain *impl_from_IDXGISwapChain(IDXGISwapChain *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_swapchain, IDXGISwapChain_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_QueryInterface(IDXGISwapChain *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGISwapChain))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_AddRef(IDXGISwapChain *iface)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    if (refcount == 1)
    {
        wined3d_mutex_lock();
        wined3d_swapchain_incref(This->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_Release(IDXGISwapChain *iface)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    ULONG refcount = InterlockedDecrement(&swapchain->refcount);

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        IWineDXGIDevice *device = swapchain->device;
        if (swapchain->target)
        {
            WARN("Releasing fullscreen swapchain.\n");
            IDXGIOutput_Release(swapchain->target);
        }
        if (swapchain->factory)
            IDXGIFactory_Release(swapchain->factory);
        wined3d_mutex_lock();
        wined3d_swapchain_decref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();
        if (device)
            IWineDXGIDevice_Release(device);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateDataInterface(IDXGISwapChain *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetParent(IDXGISwapChain *iface, REFIID riid, void **parent)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    if (!swapchain->factory)
    {
        ERR("Implicit swapchain does not store reference to parent.\n");
        *parent = NULL;
        return E_NOINTERFACE;
    }

    return IDXGIFactory_QueryInterface(swapchain->factory, riid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDevice(IDXGISwapChain *iface, REFIID riid, void **device)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    if (!swapchain->device)
    {
        ERR("Implicit swapchain does not store reference to device.\n");
        *device = NULL;
        return E_NOINTERFACE;
    }

    return IWineDXGIDevice_QueryInterface(swapchain->device, riid, device);
}

/* IDXGISwapChain methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_Present(IDXGISwapChain *iface, UINT sync_interval, UINT flags)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    HRESULT hr;

    TRACE("iface %p, sync_interval %u, flags %#x\n", iface, sync_interval, flags);

    if (sync_interval) FIXME("Unimplemented sync interval %u\n", sync_interval);
    if (flags) FIXME("Unimplemented flags %#x\n", flags);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(This->wined3d_swapchain, NULL, NULL, NULL, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetBuffer(IDXGISwapChain *iface,
        UINT buffer_idx, REFIID riid, void **surface)
{
    struct dxgi_swapchain *This = impl_from_IDXGISwapChain(iface);
    struct wined3d_texture *texture;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, buffer_idx %u, riid %s, surface %p\n",
            iface, buffer_idx, debugstr_guid(riid), surface);

    wined3d_mutex_lock();

    if (!(texture = wined3d_swapchain_get_back_buffer(This->wined3d_swapchain, buffer_idx)))
    {
        wined3d_mutex_unlock();
        return DXGI_ERROR_INVALID_CALL;
    }

    parent = wined3d_texture_get_parent(texture);
    hr = IUnknown_QueryInterface(parent, riid, surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH dxgi_swapchain_SetFullscreenState(IDXGISwapChain *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    struct wined3d_swapchain_desc swapchain_desc;
    HRESULT hr;

    TRACE("iface %p, fullscreen %#x, target %p.\n", iface, fullscreen, target);

    if (!fullscreen && target)
    {
        WARN("Invalid call.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    if (fullscreen)
    {
        if (target)
        {
            IDXGIOutput_AddRef(target);
        }
        else if (FAILED(hr = IDXGISwapChain_GetContainingOutput(iface, &target)))
        {
            WARN("Failed to get default target output for swapchain, hr %#x.\n", hr);
            return hr;
        }
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &swapchain_desc);
    swapchain_desc.windowed = !fullscreen;
    hr = wined3d_swapchain_set_fullscreen(swapchain->wined3d_swapchain, &swapchain_desc, NULL);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        swapchain->fullscreen = fullscreen;
        if (swapchain->target)
            IDXGIOutput_Release(swapchain->target);
        swapchain->target = target;
    }
    else
    {
        IDXGIOutput_Release(target);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetFullscreenState(IDXGISwapChain *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);

    TRACE("iface %p, fullscreen %p, target %p.\n", iface, fullscreen, target);

    if (fullscreen)
        *fullscreen = swapchain->fullscreen;

    if (target)
    {
        *target = swapchain->target;
        if (*target)
            IDXGIOutput_AddRef(*target);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDesc(IDXGISwapChain *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    FIXME("iface %p, desc %p partial stub!\n", iface, desc);

    if (desc == NULL)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring ScanlineOrdering, Scaling and SwapEffect.\n");

    desc->BufferDesc.Width = wined3d_desc.backbuffer_width;
    desc->BufferDesc.Height = wined3d_desc.backbuffer_height;
    desc->BufferDesc.RefreshRate.Numerator = wined3d_desc.refresh_rate;
    desc->BufferDesc.RefreshRate.Denominator = 1;
    desc->BufferDesc.Format = dxgi_format_from_wined3dformat(wined3d_desc.backbuffer_format);
    desc->BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc->BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc, wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->OutputWindow = wined3d_desc.device_window;
    desc->Windowed = wined3d_desc.windowed;
    desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc->Flags = dxgi_swapchain_flags_from_wined3d(wined3d_desc.flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeBuffers(IDXGISwapChain *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    struct wined3d_swapchain_desc wined3d_desc;
    struct wined3d_texture *texture;
    IUnknown *parent;
    unsigned int i;
    HRESULT hr;

    TRACE("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x.\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    for (i = 0; i < wined3d_desc.backbuffer_count; ++i)
    {
        texture = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain, i);
        parent = wined3d_texture_get_parent(texture);
        IUnknown_AddRef(parent);
        if (IUnknown_Release(parent))
        {
            wined3d_mutex_unlock();
            return DXGI_ERROR_INVALID_CALL;
        }
    }
    if (format != DXGI_FORMAT_UNKNOWN)
        wined3d_desc.backbuffer_format = wined3dformat_from_dxgi_format(format);
    hr = wined3d_swapchain_resize_buffers(swapchain->wined3d_swapchain, buffer_count, width, height,
            wined3d_desc.backbuffer_format, wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeTarget(IDXGISwapChain *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;

    TRACE("iface %p, target_mode_desc %p.\n", iface, target_mode_desc);

    if (!target_mode_desc)
        return DXGI_ERROR_INVALID_CALL;

    TRACE("Mode: %s.\n", debug_dxgi_mode(target_mode_desc));

    if (target_mode_desc->Scaling)
        FIXME("Ignoring scaling %#x.\n", target_mode_desc->Scaling);

    wined3d_display_mode_from_dxgi(&mode, target_mode_desc);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_resize_target(swapchain->wined3d_swapchain, &mode);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetContainingOutput(IDXGISwapChain *iface, IDXGIOutput **output)
{
    struct dxgi_swapchain *swapchain = impl_from_IDXGISwapChain(iface);
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;

    TRACE("iface %p, output %p.\n", iface, output);

    if (swapchain->target)
    {
        IDXGIOutput_AddRef(*output = swapchain->target);
        return S_OK;
    }

    if (FAILED(hr = dxgi_swapchain_GetDevice(iface, &IID_IDXGIDevice, (void **)&device)))
        return hr;

    hr = IDXGIDevice_GetAdapter(device, &adapter);
    IDXGIDevice_Release(device);
    if (FAILED(hr))
    {
        WARN("GetAdapter failed, hr %#x.\n", hr);
        return hr;
    }

    if (SUCCEEDED(IDXGIAdapter_EnumOutputs(adapter, 1, output)))
    {
        FIXME("Adapter has got multiple outputs, returning the first one.\n");
        IDXGIOutput_Release(*output);
    }

    hr = IDXGIAdapter_EnumOutputs(adapter, 0, output);
    IDXGIAdapter_Release(adapter);
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetFrameStatistics(IDXGISwapChain *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetLastPresentCount(IDXGISwapChain *iface, UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

static const struct IDXGISwapChainVtbl dxgi_swapchain_vtbl =
{
    /* IUnknown methods */
    dxgi_swapchain_QueryInterface,
    dxgi_swapchain_AddRef,
    dxgi_swapchain_Release,
    /* IDXGIObject methods */
    dxgi_swapchain_SetPrivateData,
    dxgi_swapchain_SetPrivateDataInterface,
    dxgi_swapchain_GetPrivateData,
    dxgi_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    dxgi_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    dxgi_swapchain_Present,
    dxgi_swapchain_GetBuffer,
    dxgi_swapchain_SetFullscreenState,
    dxgi_swapchain_GetFullscreenState,
    dxgi_swapchain_GetDesc,
    dxgi_swapchain_ResizeBuffers,
    dxgi_swapchain_ResizeTarget,
    dxgi_swapchain_GetContainingOutput,
    dxgi_swapchain_GetFrameStatistics,
    dxgi_swapchain_GetLastPresentCount,
};

static void STDMETHODCALLTYPE dxgi_swapchain_wined3d_object_released(void *parent)
{
    struct dxgi_swapchain *swapchain = parent;

    wined3d_private_store_cleanup(&swapchain->private_store);
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops dxgi_swapchain_wined3d_parent_ops =
{
    dxgi_swapchain_wined3d_object_released,
};

HRESULT dxgi_swapchain_init(struct dxgi_swapchain *swapchain, struct dxgi_device *device,
        struct wined3d_swapchain_desc *desc, BOOL implicit)
{
    HRESULT hr;

    /**
     * A reference to the implicit swapchain is held by the wined3d device.
     * In order to avoid circular references we do not keep a reference
     * to the device in the implicit swapchain.
     */
    if (!implicit)
    {
        if (FAILED(hr = IDXGIAdapter1_GetParent(device->adapter, &IID_IDXGIFactory,
                (void **)&swapchain->factory)))
        {
            WARN("Failed to get adapter parent, hr %#x.\n", hr);
            return hr;
        }
        IWineDXGIDevice_AddRef(swapchain->device = &device->IWineDXGIDevice_iface);
    }
    else
    {
        swapchain->device = NULL;
        swapchain->factory = NULL;
    }

    swapchain->IDXGISwapChain_iface.lpVtbl = &dxgi_swapchain_vtbl;
    swapchain->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&swapchain->private_store);

    if (!desc->windowed && (!desc->backbuffer_width || !desc->backbuffer_height))
        FIXME("Fullscreen swapchain with back buffer width/height equal to 0 not supported properly.\n");

    swapchain->fullscreen = !desc->windowed;
    desc->windowed = TRUE;
    if (FAILED(hr = wined3d_swapchain_create(device->wined3d_device, desc, swapchain,
            &dxgi_swapchain_wined3d_parent_ops, &swapchain->wined3d_swapchain)))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        goto cleanup;
    }

    swapchain->target = NULL;
    if (swapchain->fullscreen)
    {
        desc->windowed = FALSE;
        if (FAILED(hr = wined3d_swapchain_set_fullscreen(swapchain->wined3d_swapchain,
                desc, NULL)))
        {
            WARN("Failed to set fullscreen state, hr %#x.\n", hr);
            wined3d_swapchain_decref(swapchain->wined3d_swapchain);
            goto cleanup;
        }

        if (FAILED(hr = IDXGISwapChain_GetContainingOutput(&swapchain->IDXGISwapChain_iface,
                &swapchain->target)))
        {
            WARN("Failed to get target output for fullscreen swapchain, hr %#x.\n", hr);
            wined3d_swapchain_decref(swapchain->wined3d_swapchain);
            goto cleanup;
        }
    }
    wined3d_mutex_unlock();

    return S_OK;

cleanup:
    wined3d_private_store_cleanup(&swapchain->private_store);
    wined3d_mutex_unlock();
    if (swapchain->factory)
        IDXGIFactory_Release(swapchain->factory);
    if (swapchain->device)
        IWineDXGIDevice_Release(swapchain->device);
    return hr;
}
