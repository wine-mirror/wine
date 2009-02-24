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

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryInterface(IWineDXGIDevice *iface, REFIID riid, void **object)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDevice)
            || IsEqualGUID(riid, &IID_IWineDXGIDevice))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (This->child_layer)
    {
        TRACE("forwarding to child layer %p\n", This->child_layer);
        return IUnknown_QueryInterface(This->child_layer, riid, object);
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_device_AddRef(IWineDXGIDevice *iface)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_device_Release(IWineDXGIDevice *iface)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        if (This->child_layer) IUnknown_Release(This->child_layer);
        EnterCriticalSection(&dxgi_cs);
        IWineD3DDevice_Release(This->wined3d_device);
        LeaveCriticalSection(&dxgi_cs);
        IWineDXGIFactory_Release(This->factory);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateDataInterface(IWineDXGIDevice *iface,
        REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetParent(IWineDXGIDevice *iface, REFIID riid, void **parent)
{
    FIXME("iface %p, riid %s, parent %p stub!\n", iface, debugstr_guid(riid), parent);

    return E_NOTIMPL;
}

/* IDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_GetAdapter(IWineDXGIDevice *iface, IDXGIAdapter **adapter)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;
    WINED3DDEVICE_CREATION_PARAMETERS create_parameters;
    HRESULT hr;

    TRACE("iface %p, adapter %p\n", iface, adapter);

    EnterCriticalSection(&dxgi_cs);

    hr = IWineD3DDevice_GetCreationParameters(This->wined3d_device, &create_parameters);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&dxgi_cs);
        return hr;
    }

    LeaveCriticalSection(&dxgi_cs);

    return IWineDXGIFactory_EnumAdapters(This->factory, create_parameters.AdapterOrdinal, adapter);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_CreateSurface(IWineDXGIDevice *iface,
        const DXGI_SURFACE_DESC *desc, UINT surface_count, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IDXGISurface **surface)
{
    IWineD3DDeviceParent *device_parent;
    HRESULT hr;
    UINT i;
    UINT j;

    TRACE("iface %p, desc %p, surface_count %u, usage %#x, shared_resource %p, surface %p\n",
            iface, desc, surface_count, usage, shared_resource, surface);

    hr = IWineDXGIDevice_QueryInterface(iface, &IID_IWineD3DDeviceParent, (void **)&device_parent);
    if (FAILED(hr))
    {
        ERR("Device should implement IWineD3DDeviceParent\n");
        return E_FAIL;
    }

    FIXME("Implement DXGI<->wined3d usage conversion\n");

    memset(surface, 0, surface_count * sizeof(*surface));
    for (i = 0; i < surface_count; ++i)
    {
        IWineD3DSurface *wined3d_surface;
        IUnknown *parent;

        hr = IWineD3DDeviceParent_CreateSurface(device_parent, NULL, desc->Width, desc->Height,
                wined3dformat_from_dxgi_format(desc->Format), usage, WINED3DPOOL_DEFAULT, 0,
                WINED3DCUBEMAP_FACE_POSITIVE_X, &wined3d_surface);
        if (FAILED(hr))
        {
            ERR("CreateSurface failed, returning %#x\n", hr);
            goto fail;
        }

        hr = IWineD3DSurface_GetParent(wined3d_surface, &parent);
        IWineD3DSurface_Release(wined3d_surface);
        if (FAILED(hr))
        {
            ERR("GetParent failed, returning %#x\n", hr);
            goto fail;
        }

        hr = IUnknown_QueryInterface(parent, &IID_IDXGISurface, (void **)&surface[i]);
        IUnknown_Release(parent);
        if (FAILED(hr))
        {
            ERR("Surface should implement IDXGISurface\n");
            goto fail;
        }

        TRACE("Created IDXGISurface %p (%u/%u)\n", surface[i], i + 1, surface_count);
    }
    IWineD3DDeviceParent_Release(device_parent);

    return S_OK;

fail:
    for (j = 0; j < i; ++j)
    {
        IDXGISurface_Release(surface[i]);
    }
    IWineD3DDeviceParent_Release(device_parent);
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryResourceResidency(IWineDXGIDevice *iface,
        IUnknown *const *resources, DXGI_RESIDENCY *residency, UINT resource_count)
{
    FIXME("iface %p, resources %p, residency %p, resource_count %u stub!\n",
            iface, resources, residency, resource_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetGPUThreadPriority(IWineDXGIDevice *iface, INT priority)
{
    FIXME("iface %p, priority %d stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetGPUThreadPriority(IWineDXGIDevice *iface, INT *priority)
{
    FIXME("iface %p, priority %p stub!\n", iface, priority);

    return E_NOTIMPL;
}

/* IWineDXGIDevice methods */

static IWineD3DDevice * STDMETHODCALLTYPE dxgi_device_get_wined3d_device(IWineDXGIDevice *iface)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;

    TRACE("iface %p\n", iface);

    EnterCriticalSection(&dxgi_cs);
    IWineD3DDevice_AddRef(This->wined3d_device);
    LeaveCriticalSection(&dxgi_cs);
    return This->wined3d_device;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_create_surface(IWineDXGIDevice *iface, const DXGI_SURFACE_DESC *desc,
        DXGI_USAGE usage, const DXGI_SHARED_RESOURCE *shared_resource, IUnknown *outer, void **surface)
{
    struct dxgi_surface *object;

    FIXME("iface %p, desc %p, usage %#x, shared_resource %p, outer %p, surface %p partial stub!\n",
            iface, desc, usage, shared_resource, outer, surface);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI surface object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &dxgi_surface_vtbl;
    object->inner_unknown_vtbl = &dxgi_surface_inner_unknown_vtbl;
    object->refcount = 1;

    if (outer)
    {
        object->outer_unknown = outer;
        *surface = &object->inner_unknown_vtbl;
    }
    else
    {
        object->outer_unknown = (IUnknown *)&object->inner_unknown_vtbl;
        *surface = object;
    }

    TRACE("Created IDXGISurface %p\n", object);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_create_swapchain(IWineDXGIDevice *iface,
        WINED3DPRESENT_PARAMETERS *present_parameters, IWineD3DSwapChain **wined3d_swapchain)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;
    struct dxgi_swapchain *object;
    HRESULT hr;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate DXGI swapchain object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &dxgi_swapchain_vtbl;
    object->refcount = 1;

    hr = IWineD3DDevice_CreateSwapChain(This->wined3d_device, present_parameters,
            &object->wined3d_swapchain, (IUnknown *)object, SURFACE_OPENGL);
    if (FAILED(hr))
    {
        WARN("Failed to create a swapchain, returning %#x\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }
    *wined3d_swapchain = object->wined3d_swapchain;

    TRACE("Created IDXGISwapChain %p\n", object);

    return S_OK;
}

const struct IWineDXGIDeviceVtbl dxgi_device_vtbl =
{
    /* IUnknown methods */
    dxgi_device_QueryInterface,
    dxgi_device_AddRef,
    dxgi_device_Release,
    /* IDXGIObject methods */
    dxgi_device_SetPrivateData,
    dxgi_device_SetPrivateDataInterface,
    dxgi_device_GetPrivateData,
    dxgi_device_GetParent,
    /* IDXGIDevice methods */
    dxgi_device_GetAdapter,
    dxgi_device_CreateSurface,
    dxgi_device_QueryResourceResidency,
    dxgi_device_SetGPUThreadPriority,
    dxgi_device_GetGPUThreadPriority,
    /* IWineDXGIAdapter methods */
    dxgi_device_get_wined3d_device,
    dxgi_device_create_surface,
    dxgi_device_create_swapchain,
};
