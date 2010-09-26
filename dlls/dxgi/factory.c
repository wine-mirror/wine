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

static HRESULT STDMETHODCALLTYPE dxgi_factory_QueryInterface(IWineDXGIFactory *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIFactory)
            || IsEqualGUID(riid, &IID_IWineDXGIFactory))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_AddRef(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_factory_Release(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        UINT i;

        for (i = 0; i < This->adapter_count; ++i)
        {
            IDXGIAdapter_Release(This->adapters[i]);
        }
        HeapFree(GetProcessHeap(), 0, This->adapters);

        EnterCriticalSection(&dxgi_cs);
        IWineD3D_Release(This->wined3d);
        LeaveCriticalSection(&dxgi_cs);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateData(IWineDXGIFactory *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_SetPrivateDataInterface(IWineDXGIFactory *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetPrivateData(IWineDXGIFactory *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetParent(IWineDXGIFactory *iface, REFIID riid, void **parent)
{
    WARN("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    *parent = NULL;

    return E_NOINTERFACE;
}

/* IDXGIFactory methods */

static HRESULT STDMETHODCALLTYPE dxgi_factory_EnumAdapters(IWineDXGIFactory *iface,
        UINT adapter_idx, IDXGIAdapter **adapter)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;

    TRACE("iface %p, adapter_idx %u, adapter %p\n", iface, adapter_idx, adapter);

    if (!adapter) return DXGI_ERROR_INVALID_CALL;

    if (adapter_idx >= This->adapter_count)
    {
        *adapter = NULL;
        return DXGI_ERROR_NOT_FOUND;
    }

    *adapter = This->adapters[adapter_idx];
    IDXGIAdapter_AddRef(*adapter);

    TRACE("Returning adapter %p\n", *adapter);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_MakeWindowAssociation(IWineDXGIFactory *iface, HWND window, UINT flags)
{
    FIXME("iface %p, window %p, flags %#x stub!\n\n", iface, window, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_GetWindowAssociation(IWineDXGIFactory *iface, HWND *window)
{
    FIXME("iface %p, window %p stub!\n", iface, window);

    return E_NOTIMPL;
}

/* TODO: The DXGI swapchain desc is a bit nicer than WINED3DPRESENT_PARAMETERS,
 * change wined3d to use a structure more similar to DXGI. */
static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSwapChain(IWineDXGIFactory *iface,
        IUnknown *device, DXGI_SWAP_CHAIN_DESC *desc, IDXGISwapChain **swapchain)
{
    WINED3DPRESENT_PARAMETERS present_parameters;
    IWineD3DSwapChain *wined3d_swapchain;
    IWineD3DDevice *wined3d_device;
    IWineDXGIDevice *dxgi_device;
    UINT count;
    HRESULT hr;

    FIXME("iface %p, device %p, desc %p, swapchain %p partial stub!\n", iface, device, desc, swapchain);

    hr = IUnknown_QueryInterface(device, &IID_IWineDXGIDevice, (void **)&dxgi_device);
    if (FAILED(hr))
    {
        ERR("This is not the device we're looking for\n");
        return hr;
    }

    wined3d_device = IWineDXGIDevice_get_wined3d_device(dxgi_device);
    IWineDXGIDevice_Release(dxgi_device);

    count = IWineD3DDevice_GetNumberOfSwapChains(wined3d_device);
    if (count)
    {
        FIXME("Only a single swapchain supported.\n");
        IWineD3DDevice_Release(wined3d_device);
        return E_FAIL;
    }

    if (!desc->OutputWindow)
    {
        FIXME("No output window, should use factory output window\n");
    }

    FIXME("Ignoring SwapEffect and Flags\n");

    present_parameters.BackBufferWidth = desc->BufferDesc.Width;
    present_parameters.BackBufferHeight = desc->BufferDesc.Height;
    present_parameters.BackBufferFormat = wined3dformat_from_dxgi_format(desc->BufferDesc.Format);
    present_parameters.BackBufferCount = desc->BufferCount;
    if (desc->SampleDesc.Count > 1)
    {
        present_parameters.MultiSampleType = desc->SampleDesc.Count;
        present_parameters.MultiSampleQuality = desc->SampleDesc.Quality;
    }
    else
    {
        present_parameters.MultiSampleType = WINED3DMULTISAMPLE_NONE;
        present_parameters.MultiSampleQuality = 0;
    }
    present_parameters.SwapEffect = WINED3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = desc->OutputWindow;
    present_parameters.Windowed = desc->Windowed;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = 0;
    present_parameters.Flags = 0; /* WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL? */
    present_parameters.FullScreen_RefreshRateInHz =
            desc->BufferDesc.RefreshRate.Numerator / desc->BufferDesc.RefreshRate.Denominator;
    present_parameters.PresentationInterval = WINED3DPRESENT_INTERVAL_DEFAULT;

    hr = IWineD3DDevice_Init3D(wined3d_device, &present_parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize 3D, returning %#x\n", hr);
        IWineD3DDevice_Release(wined3d_device);
        return hr;
    }

    hr = IWineD3DDevice_GetSwapChain(wined3d_device, 0, &wined3d_swapchain);
    IWineD3DDevice_Release(wined3d_device);
    if (FAILED(hr))
    {
        WARN("Failed to get swapchain, returning %#x\n", hr);
        return hr;
    }

    *swapchain = IWineD3DSwapChain_GetParent(wined3d_swapchain);
    IUnknown_Release(wined3d_swapchain);

    /* FIXME? The swapchain is created with refcount 1 by the wined3d device,
     * but the wined3d device can't hold a real reference. */

    TRACE("Created IDXGISwapChain %p\n", *swapchain);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_factory_CreateSoftwareAdapter(IWineDXGIFactory *iface,
        HMODULE swrast, IDXGIAdapter **adapter)
{
    FIXME("iface %p, swrast %p, adapter %p stub!\n", iface, swrast, adapter);

    return E_NOTIMPL;
}

/* IWineDXGIFactory methods */

static IWineD3D * STDMETHODCALLTYPE dxgi_factory_get_wined3d(IWineDXGIFactory *iface)
{
    struct dxgi_factory *This = (struct dxgi_factory *)iface;

    TRACE("iface %p\n", iface);

    EnterCriticalSection(&dxgi_cs);
    IWineD3D_AddRef(This->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    return This->wined3d;
}

static const struct IWineDXGIFactoryVtbl dxgi_factory_vtbl =
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
    /* IWineDXGIFactory methods */
    dxgi_factory_get_wined3d,
};

HRESULT dxgi_factory_init(struct dxgi_factory *factory)
{
    HRESULT hr;
    UINT i;

    factory->vtbl = &dxgi_factory_vtbl;
    factory->refcount = 1;

    EnterCriticalSection(&dxgi_cs);
    factory->wined3d = WineDirect3DCreate(10, (IUnknown *)factory);
    if (!factory->wined3d)
    {
        LeaveCriticalSection(&dxgi_cs);
        return DXGI_ERROR_UNSUPPORTED;
    }

    factory->adapter_count = IWineD3D_GetAdapterCount(factory->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    factory->adapters = HeapAlloc(GetProcessHeap(), 0, factory->adapter_count * sizeof(*factory->adapters));
    if (!factory->adapters)
    {
        ERR("Failed to allocate DXGI adapter array memory.\n");
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    for (i = 0; i < factory->adapter_count; ++i)
    {
        struct dxgi_adapter *adapter = HeapAlloc(GetProcessHeap(), 0, sizeof(*adapter));
        if (!adapter)
        {
            UINT j;

            ERR("Failed to allocate DXGI adapter memory.\n");

            for (j = 0; j < i; ++j)
            {
                IDXGIAdapter_Release(factory->adapters[j]);
            }
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        hr = dxgi_adapter_init(adapter, (IWineDXGIFactory *)factory, i);
        if (FAILED(hr))
        {
            UINT j;

            ERR("Failed to initialize adapter, hr %#x.\n", hr);

            HeapFree(GetProcessHeap(), 0, adapter);
            for (j = 0; j < i; ++j)
            {
                IDXGIAdapter_Release(factory->adapters[j]);
            }
            goto fail;
        }

        factory->adapters[i] = (IDXGIAdapter *)adapter;
    }

    return S_OK;

fail:
    HeapFree(GetProcessHeap(), 0, factory->adapters);
    EnterCriticalSection(&dxgi_cs);
    IWineD3D_Release(factory->wined3d);
    LeaveCriticalSection(&dxgi_cs);
    return hr;
}
