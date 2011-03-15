/*
 * IWineD3DVolume implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);

/* Context activation is done by the caller. */
static void volume_bind_and_dirtify(struct IWineD3DVolumeImpl *volume, const struct wined3d_gl_info *gl_info)
{
    IWineD3DBaseTextureImpl *container = (IWineD3DBaseTextureImpl *)volume->container;
    DWORD active_sampler;

    /* We don't need a specific texture unit, but after binding the texture the current unit is dirty.
     * Read the unit back instead of switching to 0, this avoids messing around with the state manager's
     * gl states. The current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be called
     * from sampler() in state.c. This means we can't touch anything other than
     * whatever happens to be the currently active texture, or we would risk
     * marking already applied sampler states dirty again.
     *
     * TODO: Track the current active texture per GL context instead of using glGet
     */
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
        GLint active_texture;
        ENTER_GL();
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
        LEAVE_GL();
        active_sampler = volume->resource.device->rev_tex_unit_map[active_texture - GL_TEXTURE0_ARB];
    } else {
        active_sampler = 0;
    }

    if (active_sampler != WINED3D_UNMAPPED_STAGE)
    {
        IWineD3DDeviceImpl_MarkStateDirty(volume->resource.device, STATE_SAMPLER(active_sampler));
    }

    container->baseTexture.texture_ops->texture_bind(container, gl_info, FALSE);
}

void volume_add_dirty_box(struct IWineD3DVolumeImpl *volume, const WINED3DBOX *dirty_box)
{
    volume->dirty = TRUE;
    if (dirty_box)
    {
        volume->lockedBox.Left = min(volume->lockedBox.Left, dirty_box->Left);
        volume->lockedBox.Top = min(volume->lockedBox.Top, dirty_box->Top);
        volume->lockedBox.Front = min(volume->lockedBox.Front, dirty_box->Front);
        volume->lockedBox.Right = max(volume->lockedBox.Right, dirty_box->Right);
        volume->lockedBox.Bottom = max(volume->lockedBox.Bottom, dirty_box->Bottom);
        volume->lockedBox.Back = max(volume->lockedBox.Back, dirty_box->Back);
    }
    else
    {
        volume->lockedBox.Left = 0;
        volume->lockedBox.Top = 0;
        volume->lockedBox.Front = 0;
        volume->lockedBox.Right = volume->resource.width;
        volume->lockedBox.Bottom = volume->resource.height;
        volume->lockedBox.Back = volume->resource.depth;
    }
}

void volume_set_container(IWineD3DVolumeImpl *volume, struct IWineD3DBaseTextureImpl *container)
{
    TRACE("volume %p, container %p.\n", volume, container);

    volume->container = container;
}

/* Context activation is done by the caller. */
void volume_load(IWineD3DVolumeImpl *volume, UINT level, BOOL srgb_mode)
{
    const struct wined3d_gl_info *gl_info = &volume->resource.device->adapter->gl_info;
    const struct wined3d_format *format = volume->resource.format;

    TRACE("volume %p, level %u, srgb %#x, format %s (%#x).\n",
            volume, level, srgb_mode, debug_d3dformat(format->id), format->id);

    volume_bind_and_dirtify(volume, gl_info);

    ENTER_GL();
    GL_EXTCALL(glTexImage3DEXT(GL_TEXTURE_3D, level, format->glInternal,
            volume->resource.width, volume->resource.height, volume->resource.depth,
            0, format->glFormat, format->glType, volume->resource.allocatedMemory));
    checkGLcall("glTexImage3D");
    LEAVE_GL();

    /* When adding code releasing volume->resource.allocatedMemory to save
     * data keep in mind that GL_UNPACK_CLIENT_STORAGE_APPLE is enabled by
     * default if supported(GL_APPLE_client_storage). Thus do not release
     * volume->resource.allocatedMemory if GL_APPLE_client_storage is
     * supported. */
}

/* Do not call while under the GL lock. */
static void volume_unload(struct wined3d_resource *resource)
{
    TRACE("texture %p.\n", resource);

    /* The whole content is shadowed on This->resource.allocatedMemory, and
     * the texture name is managed by the VolumeTexture container. */

    resource_unload(resource);
}

/* *******************************************
   IWineD3DVolume IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVolumeImpl_QueryInterface(IWineD3DVolume *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DVolume)
            || IsEqualGUID(riid, &IID_IWineD3DResource)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
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

static ULONG WINAPI IWineD3DVolumeImpl_AddRef(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

/* Do not call while under the GL lock. */
static ULONG WINAPI IWineD3DVolumeImpl_Release(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);

    if (!ref)
    {
        resource_cleanup(&This->resource);
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVolume IWineD3DResource parts follow
   **************************************************** */
static void * WINAPI IWineD3DVolumeImpl_GetParent(IWineD3DVolume *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DVolumeImpl *)iface)->resource.parent;
}

static HRESULT WINAPI IWineD3DVolumeImpl_SetPrivateData(IWineD3DVolume *iface,
        REFGUID riid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data(&((IWineD3DVolumeImpl *)iface)->resource, riid, data, data_size, flags);
}

static HRESULT WINAPI IWineD3DVolumeImpl_GetPrivateData(IWineD3DVolume *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data(&((IWineD3DVolumeImpl *)iface)->resource, guid, data, data_size);
}

static HRESULT WINAPI IWineD3DVolumeImpl_FreePrivateData(IWineD3DVolume *iface, REFGUID refguid)
{
    return resource_free_private_data(&((IWineD3DVolumeImpl *)iface)->resource, refguid);
}

static DWORD WINAPI IWineD3DVolumeImpl_SetPriority(IWineD3DVolume *iface, DWORD priority)
{
    return resource_set_priority(&((IWineD3DVolumeImpl *)iface)->resource, priority);
}

static DWORD WINAPI IWineD3DVolumeImpl_GetPriority(IWineD3DVolume *iface)
{
    return resource_get_priority(&((IWineD3DVolumeImpl *)iface)->resource);
}

/* Do not call while under the GL lock. */
static void WINAPI IWineD3DVolumeImpl_PreLoad(IWineD3DVolume *iface) {
    FIXME("iface %p stub!\n", iface);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVolumeImpl_GetType(IWineD3DVolume *iface)
{
    return resource_get_type(&((IWineD3DVolumeImpl *)iface)->resource);
}

struct wined3d_resource * WINAPI IWineD3DVolumeImpl_GetResource(IWineD3DVolume *iface)
{
    TRACE("iface %p.\n", iface);

    return &((IWineD3DVolumeImpl *)iface)->resource;
}

static HRESULT WINAPI IWineD3DVolumeImpl_Map(IWineD3DVolume *iface,
        WINED3DLOCKED_BOX *pLockedVolume, const WINED3DBOX *pBox, DWORD flags)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : pBox=%p stub\n", This, pBox);

    if(!This->resource.allocatedMemory) {
        This->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->resource.size);
    }

    /* fixme: should we really lock as such? */
    TRACE("(%p) : box=%p, output pbox=%p, allMem=%p\n", This, pBox, pLockedVolume, This->resource.allocatedMemory);

    pLockedVolume->RowPitch = This->resource.format->byte_count * This->resource.width; /* Bytes / row */
    pLockedVolume->SlicePitch = This->resource.format->byte_count
            * This->resource.width * This->resource.height; /* Bytes / slice */
    if (!pBox) {
        TRACE("No box supplied - all is ok\n");
        pLockedVolume->pBits = This->resource.allocatedMemory;
        This->lockedBox.Left   = 0;
        This->lockedBox.Top    = 0;
        This->lockedBox.Front  = 0;
        This->lockedBox.Right  = This->resource.width;
        This->lockedBox.Bottom = This->resource.height;
        This->lockedBox.Back   = This->resource.depth;
    } else {
        TRACE("Lock Box (%p) = l %d, t %d, r %d, b %d, fr %d, ba %d\n", pBox, pBox->Left, pBox->Top, pBox->Right, pBox->Bottom, pBox->Front, pBox->Back);
        pLockedVolume->pBits = This->resource.allocatedMemory
                + (pLockedVolume->SlicePitch * pBox->Front)     /* FIXME: is front < back or vica versa? */
                + (pLockedVolume->RowPitch * pBox->Top)
                + (pBox->Left * This->resource.format->byte_count);
        This->lockedBox.Left   = pBox->Left;
        This->lockedBox.Top    = pBox->Top;
        This->lockedBox.Front  = pBox->Front;
        This->lockedBox.Right  = pBox->Right;
        This->lockedBox.Bottom = pBox->Bottom;
        This->lockedBox.Back   = pBox->Back;
    }

    if (!(flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)))
    {
        volume_add_dirty_box(This, &This->lockedBox);
        basetexture_set_dirty((IWineD3DBaseTextureImpl *)This->container, TRUE);
    }

    This->locked = TRUE;
    TRACE("returning memory@%p rpitch(%d) spitch(%d)\n", pLockedVolume->pBits, pLockedVolume->RowPitch, pLockedVolume->SlicePitch);
    return WINED3D_OK;
}

IWineD3DVolume * CDECL wined3d_volume_from_resource(struct wined3d_resource *resource)
{
    return (IWineD3DVolume *)volume_from_resource(resource);
}

static HRESULT WINAPI IWineD3DVolumeImpl_Unmap(IWineD3DVolume *iface)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    if (!This->locked)
    {
        WARN("Trying to unlock unlocked volume %p.\n", iface);
        return WINED3DERR_INVALIDCALL;
    }
    TRACE("(%p) : unlocking volume\n", This);
    This->locked = FALSE;
    memset(&This->lockedBox, 0, sizeof(This->lockedBox));
    return WINED3D_OK;
}

static const IWineD3DVolumeVtbl IWineD3DVolume_Vtbl =
{
    /* IUnknown */
    IWineD3DVolumeImpl_QueryInterface,
    IWineD3DVolumeImpl_AddRef,
    IWineD3DVolumeImpl_Release,
    /* IWineD3DResource */
    IWineD3DVolumeImpl_GetParent,
    IWineD3DVolumeImpl_SetPrivateData,
    IWineD3DVolumeImpl_GetPrivateData,
    IWineD3DVolumeImpl_FreePrivateData,
    IWineD3DVolumeImpl_SetPriority,
    IWineD3DVolumeImpl_GetPriority,
    IWineD3DVolumeImpl_PreLoad,
    IWineD3DVolumeImpl_GetType,
    /* IWineD3DVolume */
    IWineD3DVolumeImpl_GetResource,
    IWineD3DVolumeImpl_Map,
    IWineD3DVolumeImpl_Unmap,
};

static const struct wined3d_resource_ops volume_resource_ops =
{
    volume_unload,
};

HRESULT volume_init(IWineD3DVolumeImpl *volume, IWineD3DDeviceImpl *device, UINT width,
        UINT height, UINT depth, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
    HRESULT hr;

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("Volume cannot be created - no volume texture support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    volume->lpVtbl = &IWineD3DVolume_Vtbl;

    hr = resource_init(&volume->resource, device, WINED3DRTYPE_VOLUME, format,
            WINED3DMULTISAMPLE_NONE, 0, usage, pool, width, height, depth,
            width * height * depth * format->byte_count, parent, parent_ops,
            &volume_resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    volume->lockable = TRUE;
    volume->locked = FALSE;
    memset(&volume->lockedBox, 0, sizeof(volume->lockedBox));
    volume->dirty = TRUE;

    volume_add_dirty_box(volume, NULL);

    return WINED3D_OK;
}
