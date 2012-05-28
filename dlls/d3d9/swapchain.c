/*
 * IDirect3DSwapChain9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline IDirect3DSwapChain9Impl *impl_from_IDirect3DSwapChain9(IDirect3DSwapChain9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DSwapChain9Impl, IDirect3DSwapChain9_iface);
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_QueryInterface(IDirect3DSwapChain9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DSwapChain9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DSwapChain9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSwapChain9Impl_AddRef(IDirect3DSwapChain9 *iface)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    ULONG ref = InterlockedIncrement(&swapchain->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        if (swapchain->parentDevice)
            IDirect3DDevice9Ex_AddRef(swapchain->parentDevice);

        wined3d_mutex_lock();
        wined3d_swapchain_incref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DSwapChain9Impl_Release(IDirect3DSwapChain9 *iface)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    ULONG ref = InterlockedDecrement(&swapchain->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = swapchain->parentDevice;

        wined3d_mutex_lock();
        wined3d_swapchain_decref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        if (parentDevice) IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DSwapChain9 parts follow: */
static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DSwapChain9Impl_Present(IDirect3DSwapChain9 *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#x.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(swapchain->wined3d_swapchain, src_rect,
            dst_rect, dst_window_override, dirty_region, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetFrontBufferData(IDirect3DSwapChain9 *iface,
        IDirect3DSurface9 *pDestSurface)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    IDirect3DSurface9Impl *dst = unsafe_impl_from_IDirect3DSurface9(pDestSurface);
    HRESULT hr;

    TRACE("iface %p, surface %p.\n", iface, pDestSurface);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_front_buffer_data(swapchain->wined3d_swapchain, dst->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetBackBuffer(IDirect3DSwapChain9 *iface,
        UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    struct wined3d_surface *wined3d_surface = NULL;
    IDirect3DSurface9Impl *surface_impl;
    HRESULT hr;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, iBackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain,
            iBackBuffer, (enum wined3d_backbuffer_type)Type, &wined3d_surface);
    if (SUCCEEDED(hr) && wined3d_surface)
    {
       surface_impl = wined3d_surface_get_parent(wined3d_surface);
       *ppBackBuffer = &surface_impl->IDirect3DSurface9_iface;
       IDirect3DSurface9_AddRef(*ppBackBuffer);
       wined3d_surface_decref(wined3d_surface);
    }
    wined3d_mutex_unlock();

    /* Do not touch the **ppBackBuffer pointer otherwise! (see device test) */
    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetRasterStatus(IDirect3DSwapChain9 *iface,
        D3DRASTER_STATUS *raster_status)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, raster_status);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_raster_status(swapchain->wined3d_swapchain,
            (struct wined3d_raster_status *)raster_status);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDisplayMode(IDirect3DSwapChain9 *iface, D3DDISPLAYMODE *mode)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, mode);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_display_mode(swapchain->wined3d_swapchain, (struct wined3d_display_mode *)mode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
        mode->Format = d3dformat_from_wined3dformat(mode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetDevice(IDirect3DSwapChain9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)swapchain->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSwapChain9Impl_GetPresentParameters(IDirect3DSwapChain9 *iface,
        D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    IDirect3DSwapChain9Impl *swapchain = impl_from_IDirect3DSwapChain9(iface);
    struct wined3d_swapchain_desc desc;
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pPresentationParameters);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_desc(swapchain->wined3d_swapchain, &desc);
    wined3d_mutex_unlock();

    pPresentationParameters->BackBufferWidth = desc.backbuffer_width;
    pPresentationParameters->BackBufferHeight = desc.backbuffer_height;
    pPresentationParameters->BackBufferFormat = d3dformat_from_wined3dformat(desc.backbuffer_format);
    pPresentationParameters->BackBufferCount = desc.backbuffer_count;
    pPresentationParameters->MultiSampleType = desc.multisample_type;
    pPresentationParameters->MultiSampleQuality = desc.multisample_quality;
    pPresentationParameters->SwapEffect = desc.swap_effect;
    pPresentationParameters->hDeviceWindow = desc.device_window;
    pPresentationParameters->Windowed = desc.windowed;
    pPresentationParameters->EnableAutoDepthStencil = desc.enable_auto_depth_stencil;
    pPresentationParameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc.auto_depth_stencil_format);
    pPresentationParameters->Flags = desc.flags;
    pPresentationParameters->FullScreen_RefreshRateInHz = desc.refresh_rate;
    pPresentationParameters->PresentationInterval = desc.swap_interval;

    return hr;
}


static const IDirect3DSwapChain9Vtbl Direct3DSwapChain9_Vtbl =
{
    IDirect3DSwapChain9Impl_QueryInterface,
    IDirect3DSwapChain9Impl_AddRef,
    IDirect3DSwapChain9Impl_Release,
    IDirect3DSwapChain9Impl_Present,
    IDirect3DSwapChain9Impl_GetFrontBufferData,
    IDirect3DSwapChain9Impl_GetBackBuffer,
    IDirect3DSwapChain9Impl_GetRasterStatus,
    IDirect3DSwapChain9Impl_GetDisplayMode,
    IDirect3DSwapChain9Impl_GetDevice,
    IDirect3DSwapChain9Impl_GetPresentParameters
};

static void STDMETHODCALLTYPE d3d9_swapchain_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_swapchain_wined3d_parent_ops =
{
    d3d9_swapchain_wined3d_object_released,
};

static HRESULT swapchain_init(IDirect3DSwapChain9Impl *swapchain, struct d3d9_device *device,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    struct wined3d_swapchain_desc desc;
    HRESULT hr;

    swapchain->ref = 1;
    swapchain->IDirect3DSwapChain9_iface.lpVtbl = &Direct3DSwapChain9_Vtbl;

    desc.backbuffer_width = present_parameters->BackBufferWidth;
    desc.backbuffer_height = present_parameters->BackBufferHeight;
    desc.backbuffer_format = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    desc.backbuffer_count = max(1, present_parameters->BackBufferCount);
    desc.multisample_type = present_parameters->MultiSampleType;
    desc.multisample_quality = present_parameters->MultiSampleQuality;
    desc.swap_effect = present_parameters->SwapEffect;
    desc.device_window = present_parameters->hDeviceWindow;
    desc.windowed = present_parameters->Windowed;
    desc.enable_auto_depth_stencil = present_parameters->EnableAutoDepthStencil;
    desc.auto_depth_stencil_format = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    desc.flags = present_parameters->Flags;
    desc.refresh_rate = present_parameters->FullScreen_RefreshRateInHz;
    desc.swap_interval = present_parameters->PresentationInterval;
    desc.auto_restore_display_mode = TRUE;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, &desc,
            WINED3D_SURFACE_TYPE_OPENGL, swapchain, &d3d9_swapchain_wined3d_parent_ops,
            &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    present_parameters->BackBufferWidth = desc.backbuffer_width;
    present_parameters->BackBufferHeight = desc.backbuffer_height;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(desc.backbuffer_format);
    present_parameters->BackBufferCount = desc.backbuffer_count;
    present_parameters->MultiSampleType = desc.multisample_type;
    present_parameters->MultiSampleQuality = desc.multisample_quality;
    present_parameters->SwapEffect = desc.swap_effect;
    present_parameters->hDeviceWindow = desc.device_window;
    present_parameters->Windowed = desc.windowed;
    present_parameters->EnableAutoDepthStencil = desc.enable_auto_depth_stencil;
    present_parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc.auto_depth_stencil_format);
    present_parameters->Flags = desc.flags;
    present_parameters->FullScreen_RefreshRateInHz = desc.refresh_rate;
    present_parameters->PresentationInterval = desc.swap_interval;

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(swapchain->parentDevice);

    return D3D_OK;
}

HRESULT d3d9_swapchain_create(struct d3d9_device *device, D3DPRESENT_PARAMETERS *present_parameters,
        IDirect3DSwapChain9Impl **swapchain)
{
    IDirect3DSwapChain9Impl *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object))))
    {
        ERR("Failed to allocate swapchain memory.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = swapchain_init(object, device, present_parameters)))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = object;

    return D3D_OK;
}
