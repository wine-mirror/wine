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

static inline struct dxgi_device *impl_from_IWineDXGIDevice(IWineDXGIDevice *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_device, IWineDXGIDevice_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_QueryInterface(IWineDXGIDevice *iface, REFIID riid, void **object)
{
    struct dxgi_device *This = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IDXGIDevice)
            || IsEqualGUID(riid, &IID_IDXGIDevice1)
            || IsEqualGUID(riid, &IID_IWineDXGIDevice))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (This->child_layer)
    {
        TRACE("forwarding to child layer %p.\n", This->child_layer);
        return IUnknown_QueryInterface(This->child_layer, riid, object);
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_device_AddRef(IWineDXGIDevice *iface)
{
    struct dxgi_device *This = impl_from_IWineDXGIDevice(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_device_Release(IWineDXGIDevice *iface)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        if (device->child_layer)
            IUnknown_Release(device->child_layer);
        wined3d_mutex_lock();
        wined3d_device_uninit_3d(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        IWineDXGIAdapter_Release(device->adapter);
        wined3d_private_store_cleanup(&device->private_store);
        heap_free(device);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_SetPrivateDataInterface(IWineDXGIDevice *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&device->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetPrivateData(IWineDXGIDevice *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&device->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetParent(IWineDXGIDevice *iface, REFIID riid, void **parent)
{
    IDXGIAdapter *adapter;
    HRESULT hr;

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    hr = IWineDXGIDevice_GetAdapter(iface, &adapter);
    if (FAILED(hr))
    {
        ERR("Failed to get adapter, hr %#x.\n", hr);
        return hr;
    }

    hr = IDXGIAdapter_QueryInterface(adapter, riid, parent);
    IDXGIAdapter_Release(adapter);

    return hr;
}

/* IDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_GetAdapter(IWineDXGIDevice *iface, IDXGIAdapter **adapter)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);

    TRACE("iface %p, adapter %p.\n", iface, adapter);

    *adapter = (IDXGIAdapter *)device->adapter;
    IDXGIAdapter_AddRef(*adapter);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_CreateSurface(IWineDXGIDevice *iface,
        const DXGI_SURFACE_DESC *desc, UINT surface_count, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IDXGISurface **surface)
{
    struct wined3d_device_parent *device_parent;
    struct wined3d_resource_desc surface_desc;
    IWineDXGIDeviceParent *dxgi_device_parent;
    HRESULT hr;
    UINT i;
    UINT j;

    TRACE("iface %p, desc %p, surface_count %u, usage %#x, shared_resource %p, surface %p.\n",
            iface, desc, surface_count, usage, shared_resource, surface);

    hr = IWineDXGIDevice_QueryInterface(iface, &IID_IWineDXGIDeviceParent, (void **)&dxgi_device_parent);
    if (FAILED(hr))
    {
        ERR("Device should implement IWineDXGIDeviceParent.\n");
        return E_FAIL;
    }

    device_parent = IWineDXGIDeviceParent_get_wined3d_device_parent(dxgi_device_parent);

    surface_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    surface_desc.format = wined3dformat_from_dxgi_format(desc->Format);
    wined3d_sample_desc_from_dxgi(&surface_desc.multisample_type,
            &surface_desc.multisample_quality, &desc->SampleDesc);
    surface_desc.usage = wined3d_usage_from_dxgi_usage(usage);
    surface_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    surface_desc.width = desc->Width;
    surface_desc.height = desc->Height;
    surface_desc.depth = 1;
    surface_desc.size = 0;

    wined3d_mutex_lock();
    memset(surface, 0, surface_count * sizeof(*surface));
    for (i = 0; i < surface_count; ++i)
    {
        struct wined3d_texture *wined3d_texture;
        IUnknown *parent;

        if (FAILED(hr = device_parent->ops->create_swapchain_texture(device_parent,
                NULL, &surface_desc, 0, &wined3d_texture)))
        {
            ERR("Failed to create surface, hr %#x.\n", hr);
            goto fail;
        }

        parent = wined3d_texture_get_parent(wined3d_texture);
        hr = IUnknown_QueryInterface(parent, &IID_IDXGISurface, (void **)&surface[i]);
        wined3d_texture_decref(wined3d_texture);
        if (FAILED(hr))
        {
            ERR("Surface should implement IDXGISurface.\n");
            goto fail;
        }

        TRACE("Created IDXGISurface %p (%u/%u).\n", surface[i], i + 1, surface_count);
    }
    wined3d_mutex_unlock();
    IWineDXGIDeviceParent_Release(dxgi_device_parent);

    return S_OK;

fail:
    wined3d_mutex_unlock();
    for (j = 0; j < i; ++j)
    {
        IDXGISurface_Release(surface[i]);
    }
    IWineDXGIDeviceParent_Release(dxgi_device_parent);
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

static HRESULT STDMETHODCALLTYPE dxgi_device_SetMaximumFrameLatency(IWineDXGIDevice *iface, UINT max_latency)
{
    FIXME("iface %p, max_latency %u stub!\n", iface, max_latency);

    if (max_latency > DXGI_FRAME_LATENCY_MAX)
        return DXGI_ERROR_INVALID_CALL;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_GetMaximumFrameLatency(IWineDXGIDevice *iface, UINT *max_latency)
{
    FIXME("iface %p, max_latency %p stub!\n", iface, max_latency);

    if (max_latency)
        *max_latency = DXGI_FRAME_LATENCY_DEFAULT;

    return E_NOTIMPL;
}

/* IWineDXGIDevice methods */

static HRESULT STDMETHODCALLTYPE dxgi_device_create_surface(IWineDXGIDevice *iface,
        struct wined3d_texture *wined3d_texture, DXGI_USAGE usage,
        const DXGI_SHARED_RESOURCE *shared_resource, IUnknown *outer, void **surface)
{
    struct dxgi_surface *object;
    HRESULT hr;

    TRACE("iface %p, wined3d_texture %p, usage %#x, shared_resource %p, outer %p, surface %p.\n",
            iface, wined3d_texture, usage, shared_resource, outer, surface);

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        ERR("Failed to allocate DXGI surface object memory\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = dxgi_surface_init(object, (IDXGIDevice *)iface, outer, wined3d_texture)))
    {
        WARN("Failed to initialize surface, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created IDXGISurface %p\n", object);
    *surface = outer ? &object->IUnknown_iface : (IUnknown *)&object->IDXGISurface1_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_device_create_swapchain(IWineDXGIDevice *iface,
        struct wined3d_swapchain_desc *desc, BOOL implicit, struct wined3d_swapchain **wined3d_swapchain)
{
    struct dxgi_device *device = impl_from_IWineDXGIDevice(iface);
    struct dxgi_swapchain *object;
    HRESULT hr;

    TRACE("iface %p, desc %p, wined3d_swapchain %p.\n",
            iface, desc, wined3d_swapchain);

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        ERR("Failed to allocate DXGI swapchain object memory\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = dxgi_swapchain_init(object, device, desc, implicit)))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created IDXGISwapChain %p.\n", object);
    *wined3d_swapchain = object->wined3d_swapchain;

    return S_OK;
}

static const struct IWineDXGIDeviceVtbl dxgi_device_vtbl =
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
    /* IDXGIDevice1 methods */
    dxgi_device_SetMaximumFrameLatency,
    dxgi_device_GetMaximumFrameLatency,
    /* IWineDXGIDevice methods */
    dxgi_device_create_surface,
    dxgi_device_create_swapchain,
};

HRESULT dxgi_device_init(struct dxgi_device *device, struct dxgi_device_layer *layer,
        IDXGIFactory *factory, IDXGIAdapter *adapter,
        const D3D_FEATURE_LEVEL *feature_levels, unsigned int level_count)
{
    struct wined3d_device_parent *wined3d_device_parent;
    struct wined3d_swapchain_desc swapchain_desc;
    IWineDXGIDeviceParent *dxgi_device_parent;
    struct dxgi_adapter *dxgi_adapter;
    struct dxgi_factory *dxgi_factory;
    D3D_FEATURE_LEVEL feature_level;
    void *layer_base;
    HRESULT hr;

    if (!(dxgi_factory = unsafe_impl_from_IDXGIFactory(factory)))
    {
        WARN("This is not the factory we're looking for.\n");
        return E_FAIL;
    }

    if (!(dxgi_adapter = unsafe_impl_from_IDXGIAdapter(adapter)))
    {
        WARN("This is not the adapter we're looking for.\n");
        return E_FAIL;
    }

    device->IWineDXGIDevice_iface.lpVtbl = &dxgi_device_vtbl;
    device->refcount = 1;
    wined3d_mutex_lock();
    wined3d_private_store_init(&device->private_store);

    layer_base = device + 1;

    if (FAILED(hr = layer->create(layer->id, &layer_base, 0,
            device, &IID_IUnknown, (void **)&device->child_layer)))
    {
        WARN("Failed to create device, returning %#x.\n", hr);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    if (FAILED(hr = IWineDXGIDevice_QueryInterface(&device->IWineDXGIDevice_iface,
            &IID_IWineDXGIDeviceParent, (void **)&dxgi_device_parent)))
    {
        ERR("DXGI device should implement IWineDXGIDeviceParent.\n");
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_device_parent = IWineDXGIDeviceParent_get_wined3d_device_parent(dxgi_device_parent);
    IWineDXGIDeviceParent_Release(dxgi_device_parent);

    if (!(feature_level = dxgi_check_feature_level_support(dxgi_factory, dxgi_adapter,
            feature_levels, level_count)))
    {
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return E_FAIL;
    }

    FIXME("Ignoring adapter type.\n");

    hr = wined3d_device_create(dxgi_factory->wined3d, dxgi_adapter->ordinal, WINED3D_DEVICE_TYPE_HAL,
            NULL, 0, 4, wined3d_device_parent, &device->wined3d_device);
    if (FAILED(hr))
    {
        WARN("Failed to create a wined3d device, returning %#x.\n", hr);
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }

    layer->set_feature_level(layer->id, device->child_layer, feature_level);

    memset(&swapchain_desc, 0, sizeof(swapchain_desc));
    swapchain_desc.swap_effect = WINED3D_SWAP_EFFECT_DISCARD;
    swapchain_desc.device_window = dxgi_factory_get_device_window(dxgi_factory);
    swapchain_desc.windowed = TRUE;
    if (FAILED(hr = wined3d_device_init_3d(device->wined3d_device, &swapchain_desc)))
    {
        ERR("Failed to initialize 3D, hr %#x.\n", hr);
        wined3d_device_decref(device->wined3d_device);
        IUnknown_Release(device->child_layer);
        wined3d_private_store_cleanup(&device->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    device->adapter = &dxgi_adapter->IWineDXGIAdapter_iface;
    IWineDXGIAdapter_AddRef(device->adapter);

    return S_OK;
}
