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
    struct dxgi_swapchain *This = (struct dxgi_swapchain *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE destroy_swapchain(IWineD3DSwapChain *swapchain)
{
    TRACE("swapchain %p\n", swapchain);

    return IWineD3DSwapChain_Release(swapchain);
}

static ULONG STDMETHODCALLTYPE dxgi_swapchain_Release(IDXGISwapChain *iface)
{
    struct dxgi_swapchain *This = (struct dxgi_swapchain *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        IWineD3DDevice *wined3d_device;
        HRESULT hr;

        FIXME("Only a single swapchain is supported\n");

        hr = IWineD3DSwapChain_GetDevice(This->wined3d_swapchain, &wined3d_device);
        if (FAILED(hr))
        {
            ERR("Failed to get the wined3d device, hr %#x\n", hr);
        }
        else
        {
            hr = IWineD3DDevice_Uninit3D(wined3d_device, destroy_swapchain);
            IWineD3DDevice_Release(wined3d_device);
            if (FAILED(hr))
            {
                ERR("Uninit3D failed, hr %#x\n", hr);
            }
        }

        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetPrivateDataInterface(IDXGISwapChain *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetPrivateData(IDXGISwapChain *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetParent(IDXGISwapChain *iface, REFIID riid, void **parent)
{
    FIXME("iface %p, riid %s, parent %p stub!\n", iface, debugstr_guid(riid), parent);

    return E_NOTIMPL;
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDevice(IDXGISwapChain *iface, REFIID riid, void **device)
{
    FIXME("iface %p, riid %s, device %p stub!\n", iface, debugstr_guid(riid), device);

    return E_NOTIMPL;
}

/* IDXGISwapChain methods */

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_Present(IDXGISwapChain *iface, UINT sync_interval, UINT flags)
{
    struct dxgi_swapchain *This = (struct dxgi_swapchain *)iface;

    TRACE("iface %p, sync_interval %u, flags %#x\n", iface, sync_interval, flags);

    if (sync_interval) FIXME("Unimplemented sync interval %u\n", sync_interval);
    if (flags) FIXME("Unimplemented flags %#x\n", flags);

    return IWineD3DSwapChain_Present(This->wined3d_swapchain, NULL, NULL, NULL, NULL, 0);
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetBuffer(IDXGISwapChain *iface,
        UINT buffer_idx, REFIID riid, void **surface)
{
    struct dxgi_swapchain *This = (struct dxgi_swapchain *)iface;
    IWineD3DSurface *backbuffer;
    IUnknown *parent;
    HRESULT hr;

    TRACE("iface %p, buffer_idx %u, riid %s, surface %p\n",
            iface, buffer_idx, debugstr_guid(riid), surface);

    EnterCriticalSection(&dxgi_cs);

    hr = IWineD3DSwapChain_GetBackBuffer(This->wined3d_swapchain, buffer_idx, WINED3DBACKBUFFER_TYPE_MONO, &backbuffer);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&dxgi_cs);
        return hr;
    }

    parent = IWineD3DSurface_GetParent(backbuffer);
    hr = IUnknown_QueryInterface(parent, riid, surface);
    IWineD3DSurface_Release(backbuffer);
    LeaveCriticalSection(&dxgi_cs);

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_SetFullscreenState(IDXGISwapChain *iface,
        BOOL fullscreen, IDXGIOutput *target)
{
    FIXME("iface %p, fullscreen %u, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetFullscreenState(IDXGISwapChain *iface,
        BOOL *fullscreen, IDXGIOutput **target)
{
    FIXME("iface %p, fullscreen %p, target %p stub!\n", iface, fullscreen, target);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetDesc(IDXGISwapChain *iface, DXGI_SWAP_CHAIN_DESC *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeBuffers(IDXGISwapChain *iface,
        UINT buffer_count, UINT width, UINT height, DXGI_FORMAT format, UINT flags)
{
    FIXME("iface %p, buffer_count %u, width %u, height %u, format %s, flags %#x stub!\n",
            iface, buffer_count, width, height, debug_dxgi_format(format), flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_ResizeTarget(IDXGISwapChain *iface,
        const DXGI_MODE_DESC *target_mode_desc)
{
    FIXME("iface %p, target_mode_desc %p stub!\n", iface, target_mode_desc);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_swapchain_GetContainingOutput(IDXGISwapChain *iface, IDXGIOutput **output)
{
    FIXME("iface %p, output %p stub!\n", iface, output);

    return E_NOTIMPL;
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

HRESULT dxgi_swapchain_init(struct dxgi_swapchain *swapchain, struct dxgi_device *device,
        WINED3DPRESENT_PARAMETERS *present_parameters)
{
    HRESULT hr;

    swapchain->vtbl = &dxgi_swapchain_vtbl;
    swapchain->refcount = 1;

    hr = IWineD3DDevice_CreateSwapChain(device->wined3d_device, present_parameters,
            SURFACE_OPENGL, swapchain, &swapchain->wined3d_swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    return S_OK;
}
