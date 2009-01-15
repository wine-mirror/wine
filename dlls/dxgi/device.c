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

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryInterface(IDXGIDevice *iface, REFIID riid, void **object)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDevice))
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

static ULONG STDMETHODCALLTYPE dxgi_device_AddRef(IDXGIDevice *iface)
{
    struct dxgi_device *This = (struct dxgi_device *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_device_Release(IDXGIDevice *iface)
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

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateData(IDXGIDevice *iface, REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateDataInterface(IDXGIDevice *iface, REFGUID guid, const IUnknown *object)
{
    FIXME("iface %p, guid %s, object %p stub!\n", iface, debugstr_guid(guid), object);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetPrivateData(IDXGIDevice *iface, REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n", iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetParent(IDXGIDevice *iface, REFIID riid, void **parent)
{
    FIXME("iface %p, riid %s, parent %p stub!\n", iface, debugstr_guid(riid), parent);

    return E_NOTIMPL;
}

/* IDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_GetAdapter(IDXGIDevice *iface, IDXGIAdapter **adapter)
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

static HRESULT STDMETHODCALLTYPE dxgi_device_CreateSurface(IDXGIDevice *iface,
        const DXGI_SURFACE_DESC *desc, UINT surface_count, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IDXGISurface **surface)
{
    struct dxgi_surface *object;
    HRESULT hr;
    UINT i;

    FIXME("iface %p, desc %p, surface_count %u, usage %#x, shared_resource %p, surface %p partial stub!\n",
            iface, desc, surface_count, usage, shared_resource, surface);

    memset(surface, 0, surface_count * sizeof(*surface));
    for (i = 0; i < surface_count; ++i)
    {
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if (!object)
        {
            ERR("Failed to allocate DXGI surface object memory\n");
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        object->vtbl = &dxgi_surface_vtbl;
        object->refcount = 1;
        surface[i] = (IDXGISurface *)object;

        TRACE("Created IDXGISurface %p (%u/%u)\n", object, i + 1, surface_count);
    }

    return S_OK;

fail:
    for (i = 0; i < surface_count; ++i)
    {
        HeapFree(GetProcessHeap(), 0, surface[i]);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryResourceResidency(IDXGIDevice *iface,
        IUnknown *const *resources, DXGI_RESIDENCY *residency, UINT resource_count)
{
    FIXME("iface %p, resources %p, residency %p, resource_count %u stub!\n",
            iface, resources, residency, resource_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetGPUThreadPriority(IDXGIDevice *iface, INT priority)
{
    FIXME("iface %p, priority %d stub!\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetGPUThreadPriority(IDXGIDevice *iface, INT *priority)
{
    FIXME("iface %p, priority %p stub!\n", iface, priority);

    return E_NOTIMPL;
}

const struct IDXGIDeviceVtbl dxgi_device_vtbl =
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
};
