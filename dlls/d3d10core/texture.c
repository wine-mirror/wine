/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "d3d10core_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d10core);

static inline struct d3d10_texture2d *impl_from_ID3D10Texture2D(ID3D10Texture2D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_texture2d, ID3D10Texture2D_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_QueryInterface(ID3D10Texture2D *iface, REFIID riid, void **object)
{
    struct d3d10_texture2d *This = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Texture2D)
            || IsEqualGUID(riid, &IID_ID3D10Resource)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if (This->dxgi_surface)
    {
        TRACE("Forwarding to dxgi surface\n");
        return IDXGISurface_QueryInterface(This->dxgi_surface, riid, object);
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_texture2d_AddRef(ID3D10Texture2D *iface)
{
    struct d3d10_texture2d *This = impl_from_ID3D10Texture2D(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    if (refcount == 1)
        wined3d_texture_incref(This->wined3d_texture);

    return refcount;
}

static void STDMETHODCALLTYPE d3d10_texture2d_wined3d_object_released(void *parent)
{
    struct d3d10_texture2d *This = parent;

    if (This->dxgi_surface) IDXGISurface_Release(This->dxgi_surface);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG STDMETHODCALLTYPE d3d10_texture2d_Release(ID3D10Texture2D *iface)
{
    struct d3d10_texture2d *This = impl_from_ID3D10Texture2D(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
        wined3d_texture_decref(This->wined3d_texture);

    return refcount;
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_texture2d_GetDevice(ID3D10Texture2D *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_GetPrivateData(ID3D10Texture2D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_SetPrivateData(ID3D10Texture2D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_SetPrivateDataInterface(ID3D10Texture2D *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

/* ID3D10Resource methods */

static void STDMETHODCALLTYPE d3d10_texture2d_GetType(ID3D10Texture2D *iface,
        D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
}

static void STDMETHODCALLTYPE d3d10_texture2d_SetEvictionPriority(ID3D10Texture2D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_texture2d_GetEvictionPriority(ID3D10Texture2D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

/* ID3D10Texture2D methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_Map(ID3D10Texture2D *iface, UINT sub_resource_idx,
        D3D10_MAP map_type, UINT map_flags, D3D10_MAPPED_TEXTURE2D *mapped_texture)
{
    struct d3d10_texture2d *texture = impl_from_ID3D10Texture2D(iface);
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, sub_resource_idx %u, map_type %u, map_flags %#x, mapped_texture %p.\n",
            iface, sub_resource_idx, map_type, map_flags, mapped_texture);

    if (map_type != D3D10_MAP_READ_WRITE)
        FIXME("Ignoring map_type %#x.\n", map_type);
    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = wined3d_surface_map(wined3d_surface_from_resource(sub_resource),
            &wined3d_map_desc, NULL, 0)))
    {
        mapped_texture->pData = wined3d_map_desc.data;
        mapped_texture->RowPitch = wined3d_map_desc.row_pitch;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d10_texture2d_Unmap(ID3D10Texture2D *iface, UINT sub_resource_idx)
{
    struct d3d10_texture2d *texture = impl_from_ID3D10Texture2D(iface);
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        return;

    wined3d_surface_unmap(wined3d_surface_from_resource(sub_resource));
}

static void STDMETHODCALLTYPE d3d10_texture2d_GetDesc(ID3D10Texture2D *iface, D3D10_TEXTURE2D_DESC *desc)
{
    struct d3d10_texture2d *This = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    *desc = This->desc;
}

static const struct ID3D10Texture2DVtbl d3d10_texture2d_vtbl =
{
    /* IUnknown methods */
    d3d10_texture2d_QueryInterface,
    d3d10_texture2d_AddRef,
    d3d10_texture2d_Release,
    /* ID3D10DeviceChild methods */
    d3d10_texture2d_GetDevice,
    d3d10_texture2d_GetPrivateData,
    d3d10_texture2d_SetPrivateData,
    d3d10_texture2d_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_texture2d_GetType,
    d3d10_texture2d_SetEvictionPriority,
    d3d10_texture2d_GetEvictionPriority,
    /* ID3D10Texture2D methods */
    d3d10_texture2d_Map,
    d3d10_texture2d_Unmap,
    d3d10_texture2d_GetDesc,
};

static const struct wined3d_parent_ops d3d10_texture2d_wined3d_parent_ops =
{
    d3d10_texture2d_wined3d_object_released,
};

HRESULT d3d10_texture2d_init(struct d3d10_texture2d *texture, struct d3d10_device *device,
        const D3D10_TEXTURE2D_DESC *desc)
{
    HRESULT hr;

    texture->ID3D10Texture2D_iface.lpVtbl = &d3d10_texture2d_vtbl;
    texture->refcount = 1;
    texture->desc = *desc;

    if (desc->MipLevels == 1 && desc->ArraySize == 1)
    {
        IWineDXGIDevice *wine_device;

        hr = ID3D10Device_QueryInterface(&device->ID3D10Device_iface, &IID_IWineDXGIDevice,
                (void **)&wine_device);
        if (FAILED(hr))
        {
            ERR("Device should implement IWineDXGIDevice\n");
            return E_FAIL;
        }

        hr = IWineDXGIDevice_create_surface(wine_device, NULL, 0, NULL,
                (IUnknown *)&texture->ID3D10Texture2D_iface, (void **)&texture->dxgi_surface);
        IWineDXGIDevice_Release(wine_device);
        if (FAILED(hr))
        {
            ERR("Failed to create DXGI surface, returning %#x\n", hr);
            return hr;
        }
    }

    FIXME("Implement DXGI<->wined3d usage conversion\n");
    if (desc->ArraySize != 1)
        FIXME("Array textures not implemented.\n");
    if (desc->SampleDesc.Count > 1)
        FIXME("Multisampled textures not implemented.\n");

    hr = wined3d_texture_create_2d(device->wined3d_device, desc->Width, desc->Height,
            desc->MipLevels, desc->Usage, wined3dformat_from_dxgi_format(desc->Format), WINED3D_POOL_DEFAULT,
            texture, &d3d10_texture2d_wined3d_parent_ops, &texture->wined3d_texture);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        if (texture->dxgi_surface)
            IDXGISurface_Release(texture->dxgi_surface);
        return hr;
    }

    return S_OK;
}

static inline struct d3d10_texture3d *impl_from_ID3D10Texture3D(ID3D10Texture3D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d10_texture3d, ID3D10Texture3D_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_QueryInterface(ID3D10Texture3D *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D10Texture3D)
            || IsEqualGUID(riid, &IID_ID3D10Resource)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d10_texture3d_AddRef(ID3D10Texture3D *iface)
{
    struct d3d10_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    ULONG refcount = InterlockedIncrement(&texture->refcount);

    TRACE("%p increasing refcount to %u.\n", texture, refcount);

    if (refcount == 1)
        wined3d_texture_incref(texture->wined3d_texture);

    return refcount;
}

static void STDMETHODCALLTYPE d3d10_texture3d_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static ULONG STDMETHODCALLTYPE d3d10_texture3d_Release(ID3D10Texture3D *iface)
{
    struct d3d10_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    ULONG refcount = InterlockedDecrement(&texture->refcount);

    TRACE("%p decreasing refcount to %u.\n", texture, refcount);

    if (!refcount)
        wined3d_texture_decref(texture->wined3d_texture);

    return refcount;
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetDevice(ID3D10Texture3D *iface, ID3D10Device **device)
{
    FIXME("iface %p, device %p stub!\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_GetPrivateData(ID3D10Texture3D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    FIXME("iface %p, guid %s, data_size %p, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_SetPrivateData(ID3D10Texture3D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    FIXME("iface %p, guid %s, data_size %u, data %p stub!\n",
            iface, debugstr_guid(guid), data_size, data);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_SetPrivateDataInterface(ID3D10Texture3D *iface,
        REFGUID guid, const IUnknown *data)
{
    FIXME("iface %p, guid %s, data %p stub!\n", iface, debugstr_guid(guid), data);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetType(ID3D10Texture3D *iface,
        D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
}

static void STDMETHODCALLTYPE d3d10_texture3d_SetEvictionPriority(ID3D10Texture3D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_texture3d_GetEvictionPriority(ID3D10Texture3D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_Map(ID3D10Texture3D *iface, UINT sub_resource_idx,
        D3D10_MAP map_type, UINT map_flags, D3D10_MAPPED_TEXTURE3D *mapped_texture)
{
    struct d3d10_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, sub_resource_idx %u, map_type %u, map_flags %#x, mapped_texture %p.\n",
            iface, sub_resource_idx, map_type, map_flags, mapped_texture);

    if (map_type != D3D10_MAP_READ_WRITE)
        FIXME("Ignoring map_type %#x.\n", map_type);
    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = wined3d_volume_map(wined3d_volume_from_resource(sub_resource),
            &wined3d_map_desc, NULL, 0)))
    {
        mapped_texture->pData = wined3d_map_desc.data;
        mapped_texture->RowPitch = wined3d_map_desc.row_pitch;
        mapped_texture->DepthPitch = wined3d_map_desc.slice_pitch;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d10_texture3d_Unmap(ID3D10Texture3D *iface, UINT sub_resource_idx)
{
    struct d3d10_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        return;

    wined3d_volume_unmap(wined3d_volume_from_resource(sub_resource));
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetDesc(ID3D10Texture3D *iface, D3D10_TEXTURE3D_DESC *desc)
{
    struct d3d10_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = texture->desc;
}

static const struct ID3D10Texture3DVtbl d3d10_texture3d_vtbl =
{
    /* IUnknown methods */
    d3d10_texture3d_QueryInterface,
    d3d10_texture3d_AddRef,
    d3d10_texture3d_Release,
    /* ID3D10DeviceChild methods */
    d3d10_texture3d_GetDevice,
    d3d10_texture3d_GetPrivateData,
    d3d10_texture3d_SetPrivateData,
    d3d10_texture3d_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_texture3d_GetType,
    d3d10_texture3d_SetEvictionPriority,
    d3d10_texture3d_GetEvictionPriority,
    /* ID3D10Texture3D methods */
    d3d10_texture3d_Map,
    d3d10_texture3d_Unmap,
    d3d10_texture3d_GetDesc,
};

static const struct wined3d_parent_ops d3d10_texture3d_wined3d_parent_ops =
{
    d3d10_texture3d_wined3d_object_released,
};

HRESULT d3d10_texture3d_init(struct d3d10_texture3d *texture, struct d3d10_device *device,
        const D3D10_TEXTURE3D_DESC *desc)
{
    HRESULT hr;

    texture->ID3D10Texture3D_iface.lpVtbl = &d3d10_texture3d_vtbl;
    texture->refcount = 1;
    texture->desc = *desc;

    FIXME("Implement DXGI<->wined3d usage conversion.\n");

    hr = wined3d_texture_create_3d(device->wined3d_device, desc->Width, desc->Height, desc->Depth,
            desc->MipLevels, desc->Usage, wined3dformat_from_dxgi_format(desc->Format), WINED3D_POOL_DEFAULT,
            texture, &d3d10_texture3d_wined3d_parent_ops, &texture->wined3d_texture);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        return hr;
    }

    return S_OK;
}
