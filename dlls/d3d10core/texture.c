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

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_QueryInterface(ID3D10Texture2D *iface, REFIID riid, void **object)
{
    struct d3d10_texture2d *This = (struct d3d10_texture2d *)iface;

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
    struct d3d10_texture2d *This = (struct d3d10_texture2d *)iface;
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    if (refcount == 1 && This->wined3d_surface) IWineD3DSurface_AddRef(This->wined3d_surface);

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
    struct d3d10_texture2d *This = (struct d3d10_texture2d *)iface;
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        if (This->wined3d_surface) IWineD3DSurface_Release(This->wined3d_surface);
        else d3d10_texture2d_wined3d_object_released(This);
    }

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

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_Map(ID3D10Texture2D *iface, UINT sub_resource,
        D3D10_MAP map_type, UINT map_flags, D3D10_MAPPED_TEXTURE2D *mapped_texture)
{
    FIXME("iface %p, sub_resource %u, map_type %u, map_flags %#x, mapped_texture %p stub!\n",
            iface, sub_resource, map_type, map_flags, mapped_texture);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d3d10_texture2d_Unmap(ID3D10Texture2D *iface, UINT sub_resource)
{
    FIXME("iface %p, sub_resource %u stub!\n", iface, sub_resource);
}

static void STDMETHODCALLTYPE d3d10_texture2d_GetDesc(ID3D10Texture2D *iface, D3D10_TEXTURE2D_DESC *desc)
{
    struct d3d10_texture2d *This = (struct d3d10_texture2d *)iface;

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

    texture->vtbl = &d3d10_texture2d_vtbl;
    texture->refcount = 1;
    texture->desc = *desc;

    if (desc->MipLevels == 1 && desc->ArraySize == 1)
    {
        IWineDXGIDevice *wine_device;

        hr = ID3D10Device_QueryInterface((ID3D10Device *)device, &IID_IWineDXGIDevice, (void **)&wine_device);
        if (FAILED(hr))
        {
            ERR("Device should implement IWineDXGIDevice\n");
            return E_FAIL;
        }

        hr = IWineDXGIDevice_create_surface(wine_device, NULL, 0, NULL,
                (IUnknown *)texture, (void **)&texture->dxgi_surface);
        IWineDXGIDevice_Release(wine_device);
        if (FAILED(hr))
        {
            ERR("Failed to create DXGI surface, returning %#x\n", hr);
            return hr;
        }

        FIXME("Implement DXGI<->wined3d usage conversion\n");

        hr = IWineD3DDevice_CreateSurface(device->wined3d_device, desc->Width, desc->Height,
                wined3dformat_from_dxgi_format(desc->Format), FALSE, FALSE, 0, desc->Usage, WINED3DPOOL_DEFAULT,
                desc->SampleDesc.Count > 1 ? desc->SampleDesc.Count : WINED3DMULTISAMPLE_NONE,
                desc->SampleDesc.Quality, SURFACE_OPENGL, texture, &d3d10_texture2d_wined3d_parent_ops,
                &texture->wined3d_surface);
        if (FAILED(hr))
        {
            ERR("CreateSurface failed, returning %#x\n", hr);
            IDXGISurface_Release(texture->dxgi_surface);
            return hr;
        }
    }

    return S_OK;
}
