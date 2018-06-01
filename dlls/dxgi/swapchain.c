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

#ifdef SONAME_LIBVKD3D
#define VK_NO_PROTOTYPES
#define VKD3D_NO_PROTOTYPES
#define VKD3D_NO_VULKAN_H
#define VKD3D_NO_WIN32_TYPES
#define WINE_VK_HOST
#include "wine/library.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include <vkd3d.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static inline struct d3d11_swapchain *d3d11_swapchain_from_IDXGISwapChain1(IDXGISwapChain1 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d11_swapchain, IDXGISwapChain1_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_QueryInterface(IDXGISwapChain1 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGISwapChain)
            || IsEqualGUID(riid, &IID_IDXGISwapChain1))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_swapchain_AddRef(IDXGISwapChain1 *iface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    ULONG refcount = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u\n", swapchain, refcount);

    if (refcount == 1)
    {
        wined3d_mutex_lock();
        wined3d_swapchain_incref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_swapchain_Release(IDXGISwapChain1 *iface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetPrivateData(IDXGISwapChain1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetPrivateDataInterface(IDXGISwapChain1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetPrivateData(IDXGISwapChain1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetParent(IDXGISwapChain1 *iface, REFIID riid, void **parent)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDevice(IDXGISwapChain1 *iface, REFIID riid, void **device)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    if (!swapchain->device)
    {
        ERR("Implicit swapchain does not store reference to device.\n");
        *device = NULL;
        return E_NOINTERFACE;
    }

    return IWineDXGIDevice_QueryInterface(swapchain->device, riid, device);
}

/* IDXGISwapChain1 methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_Present(IDXGISwapChain1 *iface, UINT sync_interval, UINT flags)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

    TRACE("iface %p, sync_interval %u, flags %#x.\n", iface, sync_interval, flags);

    return IDXGISwapChain1_Present1(&swapchain->IDXGISwapChain1_iface, sync_interval, flags, NULL);
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetBuffer(IDXGISwapChain1 *iface,
        UINT buffer_idx, REFIID riid, void **surface)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_texture *texture;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, buffer_idx %u, riid %s, surface %p\n",
            iface, buffer_idx, debugstr_guid(riid), surface);

    wined3d_mutex_lock();

    if (!(texture = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain, buffer_idx)))
    {
        wined3d_mutex_unlock();
        return DXGI_ERROR_INVALID_CALL;
    }

    parent = wined3d_texture_get_parent(texture);
    hr = IUnknown_QueryInterface(parent, riid, surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH d3d11_swapchain_SetFullscreenState(IDXGISwapChain1 *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
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
        else if (FAILED(hr = IDXGISwapChain1_GetContainingOutput(iface, &target)))
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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFullscreenState(IDXGISwapChain1 *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);

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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDesc(IDXGISwapChain1 *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

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
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc,
            wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    desc->BufferUsage = dxgi_usage_from_wined3d_usage(wined3d_desc.backbuffer_usage);
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->OutputWindow = wined3d_desc.device_window;
    desc->Windowed = wined3d_desc.windowed;
    desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc->Flags = dxgi_swapchain_flags_from_wined3d(wined3d_desc.flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_ResizeBuffers(IDXGISwapChain1 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_ResizeTarget(IDXGISwapChain1 *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;

    TRACE("iface %p, target_mode_desc %p.\n", iface, target_mode_desc);

    if (!target_mode_desc)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    TRACE("Mode: %s.\n", debug_dxgi_mode(target_mode_desc));

    if (target_mode_desc->Scaling)
        FIXME("Ignoring scaling %#x.\n", target_mode_desc->Scaling);

    wined3d_display_mode_from_dxgi(&mode, target_mode_desc);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_resize_target(swapchain->wined3d_swapchain, &mode);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetContainingOutput(IDXGISwapChain1 *iface, IDXGIOutput **output)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    IDXGIAdapter *adapter;
    IDXGIDevice *device;
    HRESULT hr;

    TRACE("iface %p, output %p.\n", iface, output);

    if (swapchain->target)
    {
        IDXGIOutput_AddRef(*output = swapchain->target);
        return S_OK;
    }

    if (FAILED(hr = d3d11_swapchain_GetDevice(iface, &IID_IDXGIDevice, (void **)&device)))
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

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFrameStatistics(IDXGISwapChain1 *iface,
        DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetLastPresentCount(IDXGISwapChain1 *iface,
        UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

/* IDXGISwapChain1 methods */

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetDesc1(IDXGISwapChain1 *iface, DXGI_SWAP_CHAIN_DESC1 *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring Stereo, Scaling, SwapEffect and AlphaMode.\n");

    desc->Width = wined3d_desc.backbuffer_width;
    desc->Height = wined3d_desc.backbuffer_height;
    desc->Format = dxgi_format_from_wined3dformat(wined3d_desc.backbuffer_format);
    desc->Stereo = FALSE;
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc,
            wined3d_desc.multisample_type, wined3d_desc.multisample_quality);
    desc->BufferUsage = dxgi_usage_from_wined3d_usage(wined3d_desc.backbuffer_usage);
    desc->BufferCount = wined3d_desc.backbuffer_count;
    desc->Scaling = DXGI_SCALING_STRETCH;
    desc->SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc->AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc->Flags = dxgi_swapchain_flags_from_wined3d(wined3d_desc.flags);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetFullscreenDesc(IDXGISwapChain1 *iface,
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC *desc)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    FIXME("Ignoring ScanlineOrdering and Scaling.\n");

    desc->RefreshRate.Numerator = wined3d_desc.refresh_rate;
    desc->RefreshRate.Denominator = 1;
    desc->ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc->Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc->Windowed = wined3d_desc.windowed;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetHwnd(IDXGISwapChain1 *iface, HWND *hwnd)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    struct wined3d_swapchain_desc wined3d_desc;

    TRACE("iface %p, hwnd %p.\n", iface, hwnd);

    if (!hwnd)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    wined3d_mutex_lock();
    wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &wined3d_desc);
    wined3d_mutex_unlock();

    *hwnd = wined3d_desc.device_window;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetCoreWindow(IDXGISwapChain1 *iface,
        REFIID iid, void **core_window)
{
    FIXME("iface %p, iid %s, core_window %p stub!\n", iface, debugstr_guid(iid), core_window);

    if (core_window)
        *core_window = NULL;

    return DXGI_ERROR_INVALID_CALL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_Present1(IDXGISwapChain1 *iface,
        UINT sync_interval, UINT flags, const DXGI_PRESENT_PARAMETERS *present_parameters)
{
    struct d3d11_swapchain *swapchain = d3d11_swapchain_from_IDXGISwapChain1(iface);
    HRESULT hr;

    TRACE("iface %p, sync_interval %u, flags %#x, present_parameters %p.\n",
            iface, sync_interval, flags, present_parameters);

    if (sync_interval > 4)
    {
        WARN("Invalid sync interval %u.\n", sync_interval);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (flags & ~DXGI_PRESENT_TEST)
        FIXME("Unimplemented flags %#x.\n", flags);
    if (flags & DXGI_PRESENT_TEST)
    {
        WARN("Returning S_OK for DXGI_PRESENT_TEST.\n");
        return S_OK;
    }

    if (present_parameters)
        FIXME("Ignored present parameters %p.\n", present_parameters);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(swapchain->wined3d_swapchain, NULL, NULL, NULL, sync_interval, 0);
    wined3d_mutex_unlock();

    return hr;
}

static BOOL STDMETHODCALLTYPE d3d11_swapchain_IsTemporaryMonoSupported(IDXGISwapChain1 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetRestrictToOutput(IDXGISwapChain1 *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    if (!output)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *output = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetBackgroundColor(IDXGISwapChain1 *iface, const DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetBackgroundColor(IDXGISwapChain1 *iface, DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_SetRotation(IDXGISwapChain1 *iface, DXGI_MODE_ROTATION rotation)
{
    FIXME("iface %p, rotation %#x stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d11_swapchain_GetRotation(IDXGISwapChain1 *iface, DXGI_MODE_ROTATION *rotation)
{
    FIXME("iface %p, rotation %p stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static const struct IDXGISwapChain1Vtbl d3d11_swapchain_vtbl =
{
    /* IUnknown methods */
    d3d11_swapchain_QueryInterface,
    d3d11_swapchain_AddRef,
    d3d11_swapchain_Release,
    /* IDXGIObject methods */
    d3d11_swapchain_SetPrivateData,
    d3d11_swapchain_SetPrivateDataInterface,
    d3d11_swapchain_GetPrivateData,
    d3d11_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    d3d11_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    d3d11_swapchain_Present,
    d3d11_swapchain_GetBuffer,
    d3d11_swapchain_SetFullscreenState,
    d3d11_swapchain_GetFullscreenState,
    d3d11_swapchain_GetDesc,
    d3d11_swapchain_ResizeBuffers,
    d3d11_swapchain_ResizeTarget,
    d3d11_swapchain_GetContainingOutput,
    d3d11_swapchain_GetFrameStatistics,
    d3d11_swapchain_GetLastPresentCount,
    /* IDXGISwapChain1 methods */
    d3d11_swapchain_GetDesc1,
    d3d11_swapchain_GetFullscreenDesc,
    d3d11_swapchain_GetHwnd,
    d3d11_swapchain_GetCoreWindow,
    d3d11_swapchain_Present1,
    d3d11_swapchain_IsTemporaryMonoSupported,
    d3d11_swapchain_GetRestrictToOutput,
    d3d11_swapchain_SetBackgroundColor,
    d3d11_swapchain_GetBackgroundColor,
    d3d11_swapchain_SetRotation,
    d3d11_swapchain_GetRotation,
};

static void STDMETHODCALLTYPE d3d11_swapchain_wined3d_object_released(void *parent)
{
    struct d3d11_swapchain *swapchain = parent;

    wined3d_private_store_cleanup(&swapchain->private_store);
    heap_free(parent);
}

static const struct wined3d_parent_ops d3d11_swapchain_wined3d_parent_ops =
{
    d3d11_swapchain_wined3d_object_released,
};

HRESULT d3d11_swapchain_init(struct d3d11_swapchain *swapchain, struct dxgi_device *device,
        struct wined3d_swapchain_desc *desc, BOOL implicit)
{
    HRESULT hr;

    /*
     * A reference to the implicit swapchain is held by the wined3d device.
     * In order to avoid circular references we do not keep a reference
     * to the device in the implicit swapchain.
     */
    if (!implicit)
    {
        if (FAILED(hr = IWineDXGIAdapter_GetParent(device->adapter, &IID_IDXGIFactory,
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

    swapchain->IDXGISwapChain1_iface.lpVtbl = &d3d11_swapchain_vtbl;
    swapchain->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&swapchain->private_store);

    if (!desc->windowed && (!desc->backbuffer_width || !desc->backbuffer_height))
        FIXME("Fullscreen swapchain with back buffer width/height equal to 0 not supported properly.\n");

    swapchain->fullscreen = !desc->windowed;
    desc->windowed = TRUE;
    if (FAILED(hr = wined3d_swapchain_create(device->wined3d_device, desc, swapchain,
            &d3d11_swapchain_wined3d_parent_ops, &swapchain->wined3d_swapchain)))
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

        if (FAILED(hr = IDXGISwapChain1_GetContainingOutput(&swapchain->IDXGISwapChain1_iface,
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

HRESULT d3d11_swapchain_create(IWineDXGIDevice *device, HWND window, const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc, IDXGISwapChain1 **swapchain)
{
    struct wined3d_swapchain *wined3d_swapchain;
    struct wined3d_swapchain_desc wined3d_desc;
    HRESULT hr;

    if (swapchain_desc->Scaling != DXGI_SCALING_STRETCH)
        FIXME("Ignoring scaling %#x.\n", swapchain_desc->Scaling);
    if (swapchain_desc->AlphaMode != DXGI_ALPHA_MODE_IGNORE)
        FIXME("Ignoring alpha mode %#x.\n", swapchain_desc->AlphaMode);
    if (fullscreen_desc && fullscreen_desc->ScanlineOrdering)
        FIXME("Unhandled scanline ordering %#x.\n", fullscreen_desc->ScanlineOrdering);
    if (fullscreen_desc && fullscreen_desc->Scaling)
        FIXME("Unhandled mode scaling %#x.\n", fullscreen_desc->Scaling);

    switch (swapchain_desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_DISCARD:
            wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
            break;
        case DXGI_SWAP_EFFECT_SEQUENTIAL:
            wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_SEQUENTIAL;
            break;
        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
            wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_FLIP_DISCARD;
            break;
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
            wined3d_desc.swap_effect = WINED3D_SWAP_EFFECT_FLIP_SEQUENTIAL;
            break;
        default:
            WARN("Invalid swap effect %#x.\n", swapchain_desc->SwapEffect);
            return DXGI_ERROR_INVALID_CALL;
    }

    wined3d_desc.backbuffer_width = swapchain_desc->Width;
    wined3d_desc.backbuffer_height = swapchain_desc->Height;
    wined3d_desc.backbuffer_format = wined3dformat_from_dxgi_format(swapchain_desc->Format);
    wined3d_desc.backbuffer_count = swapchain_desc->BufferCount;
    wined3d_desc.backbuffer_usage = wined3d_usage_from_dxgi_usage(swapchain_desc->BufferUsage);
    wined3d_sample_desc_from_dxgi(&wined3d_desc.multisample_type,
            &wined3d_desc.multisample_quality, &swapchain_desc->SampleDesc);
    wined3d_desc.device_window = window;
    wined3d_desc.windowed = fullscreen_desc ? fullscreen_desc->Windowed : TRUE;
    wined3d_desc.enable_auto_depth_stencil = FALSE;
    wined3d_desc.auto_depth_stencil_format = 0;
    wined3d_desc.flags = wined3d_swapchain_flags_from_dxgi(swapchain_desc->Flags);
    wined3d_desc.refresh_rate = fullscreen_desc ? dxgi_rational_to_uint(&fullscreen_desc->RefreshRate) : 0;
    wined3d_desc.auto_restore_display_mode = TRUE;

    if (FAILED(hr = IWineDXGIDevice_create_swapchain(device, &wined3d_desc, FALSE, &wined3d_swapchain)))
    {
        WARN("Failed to create swapchain, hr %#x.\n", hr);
        return hr;
    }

    wined3d_mutex_lock();
    *swapchain = wined3d_swapchain_get_parent(wined3d_swapchain);
    wined3d_mutex_unlock();

    return S_OK;
}

#ifdef SONAME_LIBVKD3D

static PFN_vkd3d_acquire_vk_queue vkd3d_acquire_vk_queue;
static PFN_vkd3d_create_image_resource vkd3d_create_image_resource;
static PFN_vkd3d_get_vk_device vkd3d_get_vk_device;
static PFN_vkd3d_get_vk_format vkd3d_get_vk_format;
static PFN_vkd3d_get_vk_physical_device vkd3d_get_vk_physical_device;
static PFN_vkd3d_get_vk_queue_family_index vkd3d_get_vk_queue_family_index;
static PFN_vkd3d_instance_from_device vkd3d_instance_from_device;
static PFN_vkd3d_instance_get_vk_instance vkd3d_instance_get_vk_instance;
static PFN_vkd3d_release_vk_queue vkd3d_release_vk_queue;
static PFN_vkd3d_resource_decref vkd3d_resource_decref;
static PFN_vkd3d_resource_incref vkd3d_resource_incref;

struct dxgi_vk_funcs
{
    PFN_vkAcquireNextImageKHR p_vkAcquireNextImageKHR;
    PFN_vkCreateSwapchainKHR p_vkCreateSwapchainKHR;
    PFN_vkCreateWin32SurfaceKHR p_vkCreateWin32SurfaceKHR;
    PFN_vkDestroySurfaceKHR p_vkDestroySurfaceKHR;
    PFN_vkDestroySwapchainKHR p_vkDestroySwapchainKHR;
    PFN_vkGetDeviceProcAddr p_vkGetDeviceProcAddr;
    PFN_vkGetInstanceProcAddr p_vkGetInstanceProcAddr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR p_vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR p_vkGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR p_vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR p_vkGetPhysicalDeviceWin32PresentationSupportKHR;
    PFN_vkGetSwapchainImagesKHR p_vkGetSwapchainImagesKHR;
    PFN_vkQueuePresentKHR p_vkQueuePresentKHR;
    PFN_vkCreateFence p_vkCreateFence;
    PFN_vkWaitForFences p_vkWaitForFences;
    PFN_vkResetFences p_vkResetFences;
    PFN_vkDestroyFence p_vkDestroyFence;
};

static HRESULT hresult_from_vk_result(VkResult vr)
{
    switch (vr)
    {
        case VK_SUCCESS:
            return S_OK;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return E_OUTOFMEMORY;
        default:
            FIXME("Unhandled VkResult %d.\n", vr);
            return E_FAIL;
    }
}

struct d3d12_swapchain
{
    IDXGISwapChain3 IDXGISwapChain3_iface;
    LONG refcount;
    struct wined3d_private_store private_store;

    VkSwapchainKHR vk_swapchain;
    VkSurfaceKHR vk_surface;
    VkFence vk_fence;
    VkInstance vk_instance;
    VkDevice vk_device;
    ID3D12Resource *buffers[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    unsigned int buffer_count;

    uint32_t current_buffer_index;
    struct dxgi_vk_funcs vk_funcs;

    ID3D12CommandQueue *command_queue;
    ID3D12Device *device;
    IWineDXGIFactory *factory;

    HWND window;
    DXGI_SWAP_CHAIN_DESC1 desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
};

static inline struct d3d12_swapchain *d3d12_swapchain_from_IDXGISwapChain3(IDXGISwapChain3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d12_swapchain, IDXGISwapChain3_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_QueryInterface(IDXGISwapChain3 *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(iid, &IID_IDXGISwapChain)
            || IsEqualGUID(iid, &IID_IDXGISwapChain1)
            || IsEqualGUID(iid, &IID_IDXGISwapChain2)
            || IsEqualGUID(iid, &IID_IDXGISwapChain3))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d12_swapchain_AddRef(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    ULONG refcount = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u.\n", swapchain, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d12_swapchain_Release(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    ULONG refcount = InterlockedDecrement(&swapchain->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        ID3D12CommandQueue_Release(swapchain->command_queue);
        IWineDXGIFactory_Release(swapchain->factory);

        wined3d_private_store_cleanup(&swapchain->private_store);

        for (i = 0; i < swapchain->buffer_count; ++i)
        {
            vkd3d_resource_decref(swapchain->buffers[i]);
        }

        vk_funcs->p_vkDestroyFence(swapchain->vk_device, swapchain->vk_fence, NULL);
        vk_funcs->p_vkDestroySwapchainKHR(swapchain->vk_device, swapchain->vk_swapchain, NULL);
        vk_funcs->p_vkDestroySurfaceKHR(swapchain->vk_instance, swapchain->vk_surface, NULL);

        ID3D12Device_Release(swapchain->device);

        heap_free(swapchain);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetPrivateData(IDXGISwapChain3 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetPrivateDataInterface(IDXGISwapChain3 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&swapchain->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetPrivateData(IDXGISwapChain3 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&swapchain->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetParent(IDXGISwapChain3 *iface, REFIID iid, void **parent)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, iid %s, parent %p.\n", iface, debugstr_guid(iid), parent);

    return IWineDXGIFactory_QueryInterface(swapchain->factory, iid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDevice(IDXGISwapChain3 *iface, REFIID iid, void **device)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, iid %s, device %p.\n", iface, debugstr_guid(iid), device);

    return ID3D12Device_QueryInterface(swapchain->device, iid, device);
}

/* IDXGISwapChain methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_Present(IDXGISwapChain3 *iface, UINT sync_interval, UINT flags)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, sync_interval %u, flags %#x.\n", iface, sync_interval, flags);

    return IDXGISwapChain3_Present1(&swapchain->IDXGISwapChain3_iface, sync_interval, flags, NULL);
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetBuffer(IDXGISwapChain3 *iface,
        UINT buffer_idx, REFIID iid, void **surface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, buffer_idx %u, iid %s, surface %p.\n",
            iface, buffer_idx, debugstr_guid(iid), surface);

    if (buffer_idx >= swapchain->buffer_count)
    {
        WARN("Invalid buffer index %u.\n", buffer_idx);
        return DXGI_ERROR_INVALID_CALL;
    }

    return ID3D12Resource_QueryInterface(swapchain->buffers[buffer_idx], iid, surface);
}

static HRESULT STDMETHODCALLTYPE DECLSPEC_HOTPATCH d3d12_swapchain_SetFullscreenState(IDXGISwapChain3 *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    FIXME("iface %p, fullscreen %#x, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFullscreenState(IDXGISwapChain3 *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    FIXME("iface %p, fullscreen %p, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDesc(IDXGISwapChain3 *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc = &swapchain->fullscreen_desc;
    const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc = &swapchain->desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    desc->BufferDesc.Width = swapchain_desc->Width;
    desc->BufferDesc.Height = swapchain_desc->Height;
    desc->BufferDesc.RefreshRate = fullscreen_desc->RefreshRate;
    desc->BufferDesc.Format = swapchain_desc->Format;
    desc->BufferDesc.ScanlineOrdering = fullscreen_desc->ScanlineOrdering;
    desc->BufferDesc.Scaling = fullscreen_desc->Scaling;
    desc->SampleDesc = swapchain_desc->SampleDesc;
    desc->BufferUsage = swapchain_desc->BufferUsage;
    desc->BufferCount = swapchain_desc->BufferCount;
    desc->OutputWindow = swapchain->window;
    desc->Windowed = fullscreen_desc->Windowed;
    desc->SwapEffect = swapchain_desc->SwapEffect;
    desc->Flags = swapchain_desc->Flags;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeBuffers(IDXGISwapChain3 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    FIXME("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x stub!\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeTarget(IDXGISwapChain3 *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    FIXME("iface %p, target_mode_desc %p stub!\n", iface, target_mode_desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetContainingOutput(IDXGISwapChain3 *iface,
        IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFrameStatistics(IDXGISwapChain3 *iface,
        DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetLastPresentCount(IDXGISwapChain3 *iface,
        UINT *last_present_count)
{
    FIXME("iface %p, last_present_count %p stub!\n", iface, last_present_count);

    return E_NOTIMPL;
}

/* IDXGISwapChain1 methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetDesc1(IDXGISwapChain3 *iface, DXGI_SWAP_CHAIN_DESC1 *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *desc = swapchain->desc;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetFullscreenDesc(IDXGISwapChain3 *iface,
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC *desc)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *desc = swapchain->fullscreen_desc;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetHwnd(IDXGISwapChain3 *iface, HWND *hwnd)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p, hwnd %p.\n", iface, hwnd);

    if (!hwnd)
    {
        WARN("Invalid pointer.\n");
        return DXGI_ERROR_INVALID_CALL;
    }

    *hwnd = swapchain->window;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetCoreWindow(IDXGISwapChain3 *iface,
        REFIID iid, void **core_window)
{
    FIXME("iface %p, iid %s, core_window %p stub!\n", iface, debugstr_guid(iid), core_window);

    if (core_window)
        *core_window = NULL;

    return DXGI_ERROR_INVALID_CALL;
}

static HRESULT d3d12_swapchain_acquire_next_image(struct d3d12_swapchain *swapchain)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkDevice vk_device = swapchain->vk_device;
    VkFence vk_fence = swapchain->vk_fence;
    VkResult vr;

    if ((vr = vk_funcs->p_vkAcquireNextImageKHR(vk_device, swapchain->vk_swapchain, UINT64_MAX,
            VK_NULL_HANDLE, vk_fence, &swapchain->current_buffer_index)) < 0)
    {
        ERR("Failed to acquire next Vulkan image, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    if ((vr = vk_funcs->p_vkWaitForFences(vk_device, 1, &vk_fence, VK_TRUE, UINT64_MAX)) != VK_SUCCESS)
    {
        ERR("Failed to wait for fence, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }
    if ((vr = vk_funcs->p_vkResetFences(vk_device, 1, &vk_fence)) < 0)
    {
        ERR("Failed to reset fence, vr %d.\n", vr);
        return hresult_from_vk_result(vr);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_Present1(IDXGISwapChain3 *iface,
        UINT sync_interval, UINT flags, const DXGI_PRESENT_PARAMETERS *present_parameters)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    VkPresentInfoKHR present_desc;
    VkQueue vk_queue;

    TRACE("iface %p, sync_interval %u, flags %#x, present_parameters %p.\n",
            iface, sync_interval, flags, present_parameters);

    if (sync_interval > 4)
    {
        WARN("Invalid sync interval %u.\n", sync_interval);
        return DXGI_ERROR_INVALID_CALL;
    }
    if (sync_interval != 1)
        FIXME("Ignoring sync interval %u.\n", sync_interval);

    if (flags & ~DXGI_PRESENT_TEST)
        FIXME("Unimplemented flags %#x.\n", flags);
    if (flags & DXGI_PRESENT_TEST)
    {
        WARN("Returning S_OK for DXGI_PRESENT_TEST.\n");
        return S_OK;
    }

    if (present_parameters)
        FIXME("Ignored present parameters %p.\n", present_parameters);

    present_desc.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_desc.pNext = NULL;
    present_desc.waitSemaphoreCount = 0;
    present_desc.pWaitSemaphores = NULL;
    present_desc.swapchainCount = 1;
    present_desc.pSwapchains = &swapchain->vk_swapchain;
    present_desc.pImageIndices = &swapchain->current_buffer_index;
    present_desc.pResults = NULL;

    if (!(vk_queue = vkd3d_acquire_vk_queue(swapchain->command_queue)))
    {
        ERR("Failed to acquire Vulkan queue.\n");
        return E_FAIL;
    }
    vk_funcs->p_vkQueuePresentKHR(vk_queue, &present_desc);
    vkd3d_release_vk_queue(swapchain->command_queue);

    return d3d12_swapchain_acquire_next_image(swapchain);
}

static BOOL STDMETHODCALLTYPE d3d12_swapchain_IsTemporaryMonoSupported(IDXGISwapChain3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetRestrictToOutput(IDXGISwapChain3 *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    if (!output)
    {
        WARN("Invalid pointer.\n");
        return E_INVALIDARG;
    }

    *output = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetBackgroundColor(IDXGISwapChain3 *iface, const DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetBackgroundColor(IDXGISwapChain3 *iface, DXGI_RGBA *color)
{
    FIXME("iface %p, color %p stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetRotation(IDXGISwapChain3 *iface, DXGI_MODE_ROTATION rotation)
{
    FIXME("iface %p, rotation %#x stub!\n", iface, rotation);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetRotation(IDXGISwapChain3 *iface, DXGI_MODE_ROTATION *rotation)
{
    FIXME("iface %p, rotation %p stub!\n", iface, rotation);

    return E_NOTIMPL;
}

/* IDXGISwapChain2 methods */

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetSourceSize(IDXGISwapChain3 *iface, UINT width, UINT height)
{
    FIXME("iface %p, width %u, height %u stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetSourceSize(IDXGISwapChain3 *iface, UINT *width, UINT *height)
{
    FIXME("iface %p, width %p, height %p stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetMaximumFrameLatency(IDXGISwapChain3 *iface, UINT max_latency)
{
    FIXME("iface %p, max_latency %u stub!\n", iface, max_latency);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetMaximumFrameLatency(IDXGISwapChain3 *iface, UINT *max_latency)
{
    FIXME("iface %p, max_latency %p stub!\n", iface, max_latency);

    return E_NOTIMPL;
}

static HANDLE STDMETHODCALLTYPE d3d12_swapchain_GetFrameLatencyWaitableObject(IDXGISwapChain3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetMatrixTransform(IDXGISwapChain3 *iface,
        const DXGI_MATRIX_3X2_F *matrix)
{
    FIXME("iface %p, matrix %p stub!\n", iface, matrix);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_GetMatrixTransform(IDXGISwapChain3 *iface,
        DXGI_MATRIX_3X2_F *matrix)
{
    FIXME("iface %p, matrix %p stub!\n", iface, matrix);

    return E_NOTIMPL;
}

/* IDXGISwapChain3 methods */

static UINT STDMETHODCALLTYPE d3d12_swapchain_GetCurrentBackBufferIndex(IDXGISwapChain3 *iface)
{
    struct d3d12_swapchain *swapchain = d3d12_swapchain_from_IDXGISwapChain3(iface);

    TRACE("iface %p.\n", iface);

    return swapchain->current_buffer_index;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_CheckColorSpaceSupport(IDXGISwapChain3 *iface,
        DXGI_COLOR_SPACE_TYPE colour_space, UINT *colour_space_support)
{
    FIXME("iface %p, colour_space %#x, colour_space_support %p stub!\n",
            iface, colour_space, colour_space_support);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_SetColorSpace1(IDXGISwapChain3 *iface,
        DXGI_COLOR_SPACE_TYPE colour_space)
{
    FIXME("iface %p, colour_space %#x stub!\n", iface, colour_space);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d12_swapchain_ResizeBuffers1(IDXGISwapChain3 *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags,
        const UINT *node_mask, IUnknown * const *present_queue)
{
    FIXME("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x, "
            "node_mask %p, present_queue %p stub!\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags, node_mask, present_queue);

    return E_NOTIMPL;
}

static const struct IDXGISwapChain3Vtbl d3d12_swapchain_vtbl =
{
    /* IUnknown methods */
    d3d12_swapchain_QueryInterface,
    d3d12_swapchain_AddRef,
    d3d12_swapchain_Release,
    /* IDXGIObject methods */
    d3d12_swapchain_SetPrivateData,
    d3d12_swapchain_SetPrivateDataInterface,
    d3d12_swapchain_GetPrivateData,
    d3d12_swapchain_GetParent,
    /* IDXGIDeviceSubObject methods */
    d3d12_swapchain_GetDevice,
    /* IDXGISwapChain methods */
    d3d12_swapchain_Present,
    d3d12_swapchain_GetBuffer,
    d3d12_swapchain_SetFullscreenState,
    d3d12_swapchain_GetFullscreenState,
    d3d12_swapchain_GetDesc,
    d3d12_swapchain_ResizeBuffers,
    d3d12_swapchain_ResizeTarget,
    d3d12_swapchain_GetContainingOutput,
    d3d12_swapchain_GetFrameStatistics,
    d3d12_swapchain_GetLastPresentCount,
    /* IDXGISwapChain1 methods */
    d3d12_swapchain_GetDesc1,
    d3d12_swapchain_GetFullscreenDesc,
    d3d12_swapchain_GetHwnd,
    d3d12_swapchain_GetCoreWindow,
    d3d12_swapchain_Present1,
    d3d12_swapchain_IsTemporaryMonoSupported,
    d3d12_swapchain_GetRestrictToOutput,
    d3d12_swapchain_SetBackgroundColor,
    d3d12_swapchain_GetBackgroundColor,
    d3d12_swapchain_SetRotation,
    d3d12_swapchain_GetRotation,
    /* IDXGISwapChain2 methods */
    d3d12_swapchain_SetSourceSize,
    d3d12_swapchain_GetSourceSize,
    d3d12_swapchain_SetMaximumFrameLatency,
    d3d12_swapchain_GetMaximumFrameLatency,
    d3d12_swapchain_GetFrameLatencyWaitableObject,
    d3d12_swapchain_SetMatrixTransform,
    d3d12_swapchain_GetMatrixTransform,
    /* IDXGISwapChain3 methods */
    d3d12_swapchain_GetCurrentBackBufferIndex,
    d3d12_swapchain_CheckColorSpaceSupport,
    d3d12_swapchain_SetColorSpace1,
    d3d12_swapchain_ResizeBuffers1,
};

static const struct vulkan_funcs *get_vk_funcs(void)
{
    const struct vulkan_funcs *vk_funcs;
    HDC hdc;

    hdc = GetDC(0);
    vk_funcs = __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
    ReleaseDC(0, hdc);
    return vk_funcs;
}

static BOOL load_vkd3d_functions(void *vkd3d_handle)
{
#define LOAD_FUNCPTR(f) if (!(f = wine_dlsym(vkd3d_handle, #f, NULL, 0))) return FALSE;
    LOAD_FUNCPTR(vkd3d_acquire_vk_queue)
    LOAD_FUNCPTR(vkd3d_create_image_resource)
    LOAD_FUNCPTR(vkd3d_get_vk_device)
    LOAD_FUNCPTR(vkd3d_get_vk_format)
    LOAD_FUNCPTR(vkd3d_get_vk_physical_device)
    LOAD_FUNCPTR(vkd3d_get_vk_queue_family_index)
    LOAD_FUNCPTR(vkd3d_instance_from_device)
    LOAD_FUNCPTR(vkd3d_instance_get_vk_instance)
    LOAD_FUNCPTR(vkd3d_release_vk_queue)
    LOAD_FUNCPTR(vkd3d_resource_decref)
    LOAD_FUNCPTR(vkd3d_resource_incref)
#undef LOAD_FUNCPTR

    return TRUE;
}

static void *vkd3d_handle;

static BOOL WINAPI init_vkd3d_once(INIT_ONCE *once, void *param, void **context)
{
    TRACE("Loading vkd3d %s.\n", SONAME_LIBVKD3D);

    if (!(vkd3d_handle = wine_dlopen(SONAME_LIBVKD3D, RTLD_NOW, NULL, 0)))
        return FALSE;

    if (!load_vkd3d_functions(vkd3d_handle))
    {
        ERR("Failed to load vkd3d functions.\n");
        wine_dlclose(vkd3d_handle, NULL, 0);
        vkd3d_handle = NULL;
        return FALSE;
    }

    return TRUE;
}

static BOOL init_vkd3d(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, init_vkd3d_once, NULL, NULL);
    return !!vkd3d_handle;
}

static BOOL init_vk_funcs(struct dxgi_vk_funcs *dxgi, VkDevice vk_device)
{
    const struct vulkan_funcs *vk;

    if (!(vk = get_vk_funcs()))
    {
        ERR_(winediag)("Failed to load Wine Vulkan driver.\n");
        return FALSE;
    }

    dxgi->p_vkAcquireNextImageKHR = vk->p_vkAcquireNextImageKHR;
    dxgi->p_vkCreateSwapchainKHR = vk->p_vkCreateSwapchainKHR;
    dxgi->p_vkCreateWin32SurfaceKHR = vk->p_vkCreateWin32SurfaceKHR;
    dxgi->p_vkDestroySurfaceKHR = vk->p_vkDestroySurfaceKHR;
    dxgi->p_vkDestroySwapchainKHR = vk->p_vkDestroySwapchainKHR;
    dxgi->p_vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)vk->p_vkGetDeviceProcAddr;
    dxgi->p_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vk->p_vkGetInstanceProcAddr;
    dxgi->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR = vk->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    dxgi->p_vkGetPhysicalDeviceSurfaceFormatsKHR = vk->p_vkGetPhysicalDeviceSurfaceFormatsKHR;
    dxgi->p_vkGetPhysicalDeviceSurfacePresentModesKHR = vk->p_vkGetPhysicalDeviceSurfacePresentModesKHR;
    dxgi->p_vkGetPhysicalDeviceSurfaceSupportKHR = vk->p_vkGetPhysicalDeviceSurfaceSupportKHR;
    dxgi->p_vkGetPhysicalDeviceWin32PresentationSupportKHR = vk->p_vkGetPhysicalDeviceWin32PresentationSupportKHR;
    dxgi->p_vkGetSwapchainImagesKHR = vk->p_vkGetSwapchainImagesKHR;
    dxgi->p_vkQueuePresentKHR = vk->p_vkQueuePresentKHR;

#define LOAD_DEVICE_PFN(name) \
    if (!(dxgi->p_##name = vk->p_vkGetDeviceProcAddr(vk_device, #name))) \
    { \
        ERR("Failed to get device proc "#name".\n"); \
        return FALSE; \
    }
    LOAD_DEVICE_PFN(vkCreateFence)
    LOAD_DEVICE_PFN(vkWaitForFences)
    LOAD_DEVICE_PFN(vkResetFences)
    LOAD_DEVICE_PFN(vkDestroyFence)
#undef LOAD_DEVICE_PFN

    return TRUE;
}

static HRESULT select_vk_format(const struct dxgi_vk_funcs *vk_funcs,
        VkPhysicalDevice vk_physical_device, VkSurfaceKHR vk_surface,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, VkFormat *vk_format)
{
    VkSurfaceFormatKHR *formats;
    uint32_t format_count;
    VkFormat format;
    unsigned int i;
    VkResult vr;

    *vk_format = VK_FORMAT_UNDEFINED;

    format = vkd3d_get_vk_format(swapchain_desc->Format);
    if (format == VK_FORMAT_UNDEFINED)
        return DXGI_ERROR_INVALID_CALL;

    vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, &format_count, NULL);
    if (vr < 0 || !format_count)
    {
        WARN("Failed to get supported surface formats, vr %d.\n", vr);
        return DXGI_ERROR_INVALID_CALL;
    }

    if (!(formats = heap_calloc(format_count, sizeof(*formats))))
        return E_OUTOFMEMORY;

    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,
            vk_surface, &format_count, formats)) < 0)
    {
        WARN("Failed to enumerate supported surface formats, vr %d.\n", vr);
        heap_free(formats);
        return hresult_from_vk_result(vr);
    }

    for (i = 0; i < format_count; ++i)
    {
        if (formats[i].format == format && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            break;
    }
    heap_free(formats);

    if (i == format_count)
    {
        FIXME("Failed to find suitable format for %s.\n", debug_dxgi_format(swapchain_desc->Format));
        return DXGI_ERROR_INVALID_CALL;
    }

    *vk_format = format;
    return S_OK;
}

static HRESULT d3d12_swapchain_init(struct d3d12_swapchain *swapchain, IWineDXGIFactory *factory,
        ID3D12Device *device, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc)
{
    const struct dxgi_vk_funcs *vk_funcs = &swapchain->vk_funcs;
    struct vkd3d_image_resource_create_info resource_info;
    struct VkSwapchainCreateInfoKHR vk_swapchain_desc;
    struct VkWin32SurfaceCreateInfoKHR surface_desc;
    VkImage vk_images[DXGI_MAX_SWAP_CHAIN_BUFFERS];
    VkSwapchainKHR vk_swapchain = VK_NULL_HANDLE;
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    VkSurfaceCapabilitiesKHR surface_caps;
    VkPhysicalDevice vk_physical_device;
    VkFence vk_fence = VK_NULL_HANDLE;
    unsigned int image_count, i, j;
    VkFenceCreateInfo fence_desc;
    uint32_t queue_family_index;
    VkInstance vk_instance;
    VkBool32 supported;
    VkDevice vk_device;
    VkFormat vk_format;
    VkResult vr;
    HRESULT hr;

    swapchain->IDXGISwapChain3_iface.lpVtbl = &d3d12_swapchain_vtbl;
    swapchain->refcount = 1;

    swapchain->window = window;
    swapchain->desc = *swapchain_desc;
    swapchain->fullscreen_desc = *fullscreen_desc;

    switch (swapchain_desc->SwapEffect)
    {
        case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
        case DXGI_SWAP_EFFECT_FLIP_DISCARD:
            FIXME("Ignoring swap effect %#x.\n", swapchain_desc->SwapEffect);
            break;
        default:
            WARN("Invalid swap effect %#x.\n", swapchain_desc->SwapEffect);
            return DXGI_ERROR_INVALID_CALL;
    }

    if (!init_vkd3d())
    {
        ERR_(winediag)("libvkd3d could not be loaded.\n");
        return DXGI_ERROR_UNSUPPORTED;
    }

    if (swapchain_desc->BufferUsage && swapchain_desc->BufferUsage != DXGI_USAGE_RENDER_TARGET_OUTPUT)
        FIXME("Ignoring buffer usage %#x.\n", swapchain_desc->BufferUsage);
    if (swapchain_desc->Scaling != DXGI_SCALING_STRETCH)
        FIXME("Ignoring scaling %#x.\n", swapchain_desc->Scaling);
    if (swapchain_desc->AlphaMode && swapchain_desc->AlphaMode != DXGI_ALPHA_MODE_IGNORE)
        FIXME("Ignoring alpha mode %#x.\n", swapchain_desc->AlphaMode);
    if (swapchain_desc->Flags)
        FIXME("Ignoring swapchain flags %#x.\n", swapchain_desc->Flags);

    FIXME("Ignoring refresh rate.\n");
    if (fullscreen_desc->ScanlineOrdering)
        FIXME("Unhandled scanline ordering %#x.\n", fullscreen_desc->ScanlineOrdering);
    if (fullscreen_desc->Scaling)
        FIXME("Unhandled mode scaling %#x.\n", fullscreen_desc->Scaling);
    if (!fullscreen_desc->Windowed)
        FIXME("Fullscreen not supported yet.\n");

    vk_instance = vkd3d_instance_get_vk_instance(vkd3d_instance_from_device(device));
    vk_physical_device = vkd3d_get_vk_physical_device(device);
    vk_device = vkd3d_get_vk_device(device);

    if (!init_vk_funcs(&swapchain->vk_funcs, vk_device))
        return E_FAIL;

    surface_desc.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_desc.pNext = NULL;
    surface_desc.flags = 0;
    surface_desc.hinstance = GetModuleHandleA("dxgi.dll");
    surface_desc.hwnd = window;
    if ((vr = vk_funcs->p_vkCreateWin32SurfaceKHR(vk_instance, &surface_desc, NULL, &vk_surface)) < 0)
    {
        WARN("Failed to create Vulkan surface, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    queue_family_index = vkd3d_get_vk_queue_family_index(queue);
    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device,
            queue_family_index, vk_surface, &supported)) < 0 || !supported)
    {
        FIXME("Queue family does not support presentation, vr %d.\n", vr);
        hr = DXGI_ERROR_UNSUPPORTED;
        goto fail;
    }

    if (FAILED(hr = select_vk_format(vk_funcs, vk_physical_device, vk_surface, swapchain_desc, &vk_format)))
        goto fail;

    if ((vr = vk_funcs->p_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device,
            vk_surface, &surface_caps)) < 0)
    {
        WARN("Failed to get surface capabilities, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    if (surface_caps.maxImageCount && (swapchain_desc->BufferCount > surface_caps.maxImageCount
            || swapchain_desc->BufferCount < surface_caps.minImageCount))
    {
        WARN("Buffer count %u is not supported (%u-%u).\n", swapchain_desc->BufferCount,
                surface_caps.minImageCount, surface_caps.maxImageCount);
        hr = DXGI_ERROR_UNSUPPORTED;
        goto fail;
    }

    if (swapchain_desc->Width > surface_caps.maxImageExtent.width
            || swapchain_desc->Width < surface_caps.minImageExtent.width
            || swapchain_desc->Height > surface_caps.maxImageExtent.height
            || swapchain_desc->Height < surface_caps.minImageExtent.height)
    {
        FIXME("Swapchain dimensions %ux%u are not supported (%u-%u x %u-%u).\n",
                swapchain_desc->Width, swapchain_desc->Height,
                surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width,
                surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
    }

    if (!(surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR))
    {
        FIXME("Unsupported alpha mode.\n");
        hr = DXGI_ERROR_UNSUPPORTED;
        goto fail;
    }

    vk_swapchain_desc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    vk_swapchain_desc.pNext = NULL;
    vk_swapchain_desc.flags = 0;
    vk_swapchain_desc.surface = vk_surface;
    vk_swapchain_desc.minImageCount = swapchain_desc->BufferCount;
    vk_swapchain_desc.imageFormat = vk_format;
    vk_swapchain_desc.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    vk_swapchain_desc.imageExtent.width = swapchain_desc->Width;
    vk_swapchain_desc.imageExtent.height = swapchain_desc->Height;
    vk_swapchain_desc.imageArrayLayers = 1;
    vk_swapchain_desc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vk_swapchain_desc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vk_swapchain_desc.queueFamilyIndexCount = 0;
    vk_swapchain_desc.pQueueFamilyIndices = NULL;
    vk_swapchain_desc.preTransform = surface_caps.currentTransform;
    vk_swapchain_desc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    vk_swapchain_desc.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    vk_swapchain_desc.clipped = VK_TRUE;
    vk_swapchain_desc.oldSwapchain = VK_NULL_HANDLE;
    if ((vr = vk_funcs->p_vkCreateSwapchainKHR(vk_device, &vk_swapchain_desc, NULL, &vk_swapchain)) < 0)
    {
        WARN("Failed to create Vulkan swapchain, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_desc.pNext = NULL;
    fence_desc.flags = 0;
    if ((vr = vk_funcs->p_vkCreateFence(vk_device, &fence_desc, NULL, &vk_fence)) < 0)
    {
        WARN("Failed to create Vulkan fence, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    if ((vr = vk_funcs->p_vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &image_count, NULL)) < 0)
    {
        WARN("Failed to get Vulkan swapchain images, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }
    if (image_count != swapchain_desc->BufferCount)
        FIXME("Got %u swapchain images, expected %u.\n", image_count, swapchain_desc->BufferCount);
    if (image_count > ARRAY_SIZE(vk_images))
    {
        hr = E_FAIL;
        goto fail;
    }
    if ((vr = vk_funcs->p_vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &image_count, vk_images)) < 0)
    {
        WARN("Failed to get Vulkan swapchain images, vr %d.\n", vr);
        hr = hresult_from_vk_result(vr);
        goto fail;
    }

    swapchain->vk_swapchain = vk_swapchain;
    swapchain->vk_surface = vk_surface;
    swapchain->vk_fence = vk_fence;
    swapchain->vk_instance = vk_instance;
    swapchain->vk_device = vk_device;

    if (FAILED(hr = d3d12_swapchain_acquire_next_image(swapchain)))
    {
        WARN("Failed to acquire Vulkan image, hr %#x.\n", hr);
        goto fail;
    }

    resource_info.type = VKD3D_STRUCTURE_TYPE_IMAGE_RESOURCE_CREATE_INFO;
    resource_info.next = NULL;
    resource_info.desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_info.desc.Alignment = 0;
    resource_info.desc.Width = swapchain_desc->Width;
    resource_info.desc.Height = swapchain_desc->Height;
    resource_info.desc.DepthOrArraySize = 1;
    resource_info.desc.MipLevels = 1;
    resource_info.desc.Format = swapchain_desc->Format;
    resource_info.desc.SampleDesc.Count = 1;
    resource_info.desc.SampleDesc.Quality = 0;
    resource_info.desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_info.desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    resource_info.flags = VKD3D_RESOURCE_INITIAL_STATE_TRANSITION | VKD3D_RESOURCE_PRESENT_STATE_TRANSITION;
    resource_info.present_state = D3D12_RESOURCE_STATE_PRESENT;
    for (i = 0; i < image_count; ++i)
    {
        resource_info.vk_image = vk_images[i];
        if (SUCCEEDED(hr = vkd3d_create_image_resource(device, &resource_info, &swapchain->buffers[i])))
        {
            vkd3d_resource_incref(swapchain->buffers[i]);
            ID3D12Resource_Release(swapchain->buffers[i]);
        }
        else
        {
            ERR("Failed to create vkd3d resource for Vulkan image %u, hr %#x.\n", i, hr);
            for (j = 0; j < i; ++j)
            {
                vkd3d_resource_decref(swapchain->buffers[j]);
            }
            goto fail;
        }
    }
    swapchain->buffer_count = image_count;

    wined3d_private_store_init(&swapchain->private_store);

    ID3D12CommandQueue_AddRef(swapchain->command_queue = queue);
    ID3D12Device_AddRef(swapchain->device = device);
    IWineDXGIFactory_AddRef(swapchain->factory = factory);

    return S_OK;

fail:
    vk_funcs->p_vkDestroyFence(vk_device, vk_fence, NULL);
    vk_funcs->p_vkDestroySwapchainKHR(vk_device, vk_swapchain, NULL);
    vk_funcs->p_vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
    return hr;
}

HRESULT d3d12_swapchain_create(IWineDXGIFactory *factory, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc,
        IDXGISwapChain1 **swapchain)
{
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC default_fullscreen_desc;
    struct d3d12_swapchain *object;
    ID3D12Device *device;
    HRESULT hr;

    if (!fullscreen_desc)
    {
        memset(&default_fullscreen_desc, 0, sizeof(default_fullscreen_desc));
        default_fullscreen_desc.Windowed = TRUE;
        fullscreen_desc = &default_fullscreen_desc;
    }

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = ID3D12CommandQueue_GetDevice(queue, &IID_ID3D12Device, (void **)&device)))
    {
        ERR("Failed to get D3D12 device, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    hr = d3d12_swapchain_init(object, factory, device, queue, window, swapchain_desc, fullscreen_desc);
    ID3D12Device_Release(device);
    if (FAILED(hr))
    {
        heap_free(object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);

    *swapchain = (IDXGISwapChain1 *)&object->IDXGISwapChain3_iface;

    return S_OK;
}

#else

HRESULT d3d12_swapchain_create(IWineDXGIFactory *factory, ID3D12CommandQueue *queue, HWND window,
        const DXGI_SWAP_CHAIN_DESC1 *swapchain_desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *fullscreen_desc,
        IDXGISwapChain1 **swapchain)
{
    ERR_(winediag)("Wine was built without Direct3D 12 support.\n");
    return DXGI_ERROR_UNSUPPORTED;
}

#endif  /* SONAME_LIBVKD3D */
