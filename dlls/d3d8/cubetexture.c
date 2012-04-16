/*
 * IDirect3DCubeTexture8 implementation
 *
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline struct d3d8_texture *impl_from_IDirect3DCubeTexture8(IDirect3DCubeTexture8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_texture, IDirect3DBaseTexture8_iface);
}

static HRESULT WINAPI d3d8_texture_cube_QueryInterface(IDirect3DCubeTexture8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DCubeTexture8)
            || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
            || IsEqualGUID(riid, &IID_IDirect3DResource8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_texture_cube_AddRef(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    ULONG ref = InterlockedIncrement(&texture->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IUnknown_AddRef(texture->parent_device);
        wined3d_mutex_lock();
        wined3d_texture_incref(texture->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI d3d8_texture_cube_Release(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    ULONG ref = InterlockedDecrement(&texture->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice8 *parent_device = texture->parent_device;

        TRACE("Releasing child %p.\n", texture->wined3d_texture);

        wined3d_mutex_lock();
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice8_Release(parent_device);
    }
    return ref;
}

static HRESULT WINAPI d3d8_texture_cube_GetDevice(IDirect3DCubeTexture8 *iface, IDirect3DDevice8 **device)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = texture->parent_device;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_texture_cube_SetPrivateData(IDirect3DCubeTexture8 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    hr = wined3d_resource_set_private_data(resource, guid, data, data_size, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_texture_cube_GetPrivateData(IDirect3DCubeTexture8 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    hr = wined3d_resource_get_private_data(resource, guid, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_texture_cube_FreePrivateData(IDirect3DCubeTexture8 *iface, REFGUID guid)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI d3d8_texture_cube_SetPriority(IDirect3DCubeTexture8 *iface, DWORD priority)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_priority(texture->wined3d_texture, priority);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d8_texture_cube_GetPriority(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret =  wined3d_texture_get_priority(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d8_texture_cube_PreLoad(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d8_texture_cube_GetType(IDirect3DCubeTexture8 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_CUBETEXTURE;
}

static DWORD WINAPI d3d8_texture_cube_SetLOD(IDirect3DCubeTexture8 *iface, DWORD lod)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(texture->wined3d_texture, lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d8_texture_cube_GetLOD(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d8_texture_cube_GetLevelCount(IDirect3DCubeTexture8 *iface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d8_texture_cube_GetLevelDesc(IDirect3DCubeTexture8 *iface, UINT level, D3DSURFACE_DESC *desc)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        struct wined3d_resource_desc wined3d_desc;

        wined3d_resource_get_desc(sub_resource, &wined3d_desc);
        desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
        desc->Type = wined3d_desc.resource_type;
        desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
        desc->Pool = wined3d_desc.pool;
        desc->Size = wined3d_desc.size;
        desc->MultiSampleType = wined3d_desc.multisample_type;
        desc->Width = wined3d_desc.width;
        desc->Height = wined3d_desc.height;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_texture_cube_GetCubeMapSurface(IDirect3DCubeTexture8 *iface,
        D3DCUBEMAP_FACES face, UINT level, IDirect3DSurface8 **surface)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *sub_resource;
    IDirect3DSurface8Impl *surface_impl;
    UINT sub_resource_idx;

    TRACE("iface %p, face %#x, level %u, surface %p.\n", iface, face, level, surface);

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    surface_impl = wined3d_resource_get_parent(sub_resource);
    *surface = &surface_impl->IDirect3DSurface8_iface;
    IDirect3DSurface8_AddRef(*surface);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_texture_cube_LockRect(IDirect3DCubeTexture8 *iface,
        D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect,
        DWORD flags)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *sub_resource;
    IDirect3DSurface8Impl *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, face, level, locked_rect, rect, flags);

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface8_LockRect(&surface_impl->IDirect3DSurface8_iface, locked_rect, rect, flags);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_texture_cube_UnlockRect(IDirect3DCubeTexture8 *iface,
        D3DCUBEMAP_FACES face, UINT level)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    struct wined3d_resource *sub_resource;
    IDirect3DSurface8Impl *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u.\n", iface, face, level);

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface8_UnlockRect(&surface_impl->IDirect3DSurface8_iface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_texture_cube_AddDirtyRect(IDirect3DCubeTexture8 *iface,
        D3DCUBEMAP_FACES face, const RECT *dirty_rect)
{
    struct d3d8_texture *texture = impl_from_IDirect3DCubeTexture8(iface);
    HRESULT hr;

    TRACE("iface %p, face %#x, dirty_rect %s.\n",
            iface, face, wine_dbgstr_rect(dirty_rect));

    wined3d_mutex_lock();
    if (!dirty_rect)
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, face, NULL);
    else
    {
        struct wined3d_box dirty_region;

        dirty_region.left = dirty_rect->left;
        dirty_region.top = dirty_rect->top;
        dirty_region.right = dirty_rect->right;
        dirty_region.bottom = dirty_rect->bottom;
        dirty_region.front = 0;
        dirty_region.back = 1;
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, face, &dirty_region);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DCubeTexture8Vtbl Direct3DCubeTexture8_Vtbl =
{
    /* IUnknown */
    d3d8_texture_cube_QueryInterface,
    d3d8_texture_cube_AddRef,
    d3d8_texture_cube_Release,
    /* IDirect3DResource8 */
    d3d8_texture_cube_GetDevice,
    d3d8_texture_cube_SetPrivateData,
    d3d8_texture_cube_GetPrivateData,
    d3d8_texture_cube_FreePrivateData,
    d3d8_texture_cube_SetPriority,
    d3d8_texture_cube_GetPriority,
    d3d8_texture_cube_PreLoad,
    d3d8_texture_cube_GetType,
    /* IDirect3DBaseTexture8 */
    d3d8_texture_cube_SetLOD,
    d3d8_texture_cube_GetLOD,
    d3d8_texture_cube_GetLevelCount,
    /* IDirect3DCubeTexture8 */
    d3d8_texture_cube_GetLevelDesc,
    d3d8_texture_cube_GetCubeMapSurface,
    d3d8_texture_cube_LockRect,
    d3d8_texture_cube_UnlockRect,
    d3d8_texture_cube_AddDirtyRect,
};

static void STDMETHODCALLTYPE d3d8_cubetexture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_cubetexture_wined3d_parent_ops =
{
    d3d8_cubetexture_wined3d_object_destroyed,
};

HRESULT cubetexture_init(struct d3d8_texture *texture, IDirect3DDevice8Impl *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->IDirect3DBaseTexture8_iface.lpVtbl = (const IDirect3DBaseTexture8Vtbl *)&Direct3DCubeTexture8_Vtbl;
    texture->refcount = 1;

    wined3d_mutex_lock();
    hr = wined3d_texture_create_cube(device->wined3d_device, edge_length, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool, texture,
            &d3d8_cubetexture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d cube texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parent_device = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(texture->parent_device);

    return D3D_OK;
}
