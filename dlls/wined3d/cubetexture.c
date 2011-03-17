/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_texture);

/* Context activation is done by the caller. */
static HRESULT cubetexture_bind(IWineD3DBaseTextureImpl *texture,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    BOOL set_gl_texture_desc;
    HRESULT hr;

    TRACE("texture %p, gl_info %p, srgb %#x.\n", texture, gl_info, srgb);

    hr = basetexture_bind(texture, gl_info, srgb, &set_gl_texture_desc);
    if (set_gl_texture_desc && SUCCEEDED(hr))
    {
        UINT sub_count = texture->baseTexture.level_count * texture->baseTexture.layer_count;
        BOOL srgb_tex = !gl_info->supported[EXT_TEXTURE_SRGB_DECODE] && texture->baseTexture.is_srgb;
        GLuint name = srgb_tex ? texture->baseTexture.texture_srgb.name : texture->baseTexture.texture_rgb.name;
        UINT i;

        for (i = 0; i < sub_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = surface_from_resource(texture->baseTexture.sub_resources[i]);
            surface_set_texture_name(surface, name, srgb_tex);
        }
    }

    return hr;
}

/* Do not call while under the GL lock. */
static void cubetexture_preload(IWineD3DBaseTextureImpl *texture, enum WINED3DSRGB srgb)
{
    UINT sub_count = texture->baseTexture.level_count * texture->baseTexture.layer_count;
    IWineD3DDeviceImpl *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context = NULL;
    struct gl_texture *gl_tex;
    BOOL srgb_mode;
    UINT i;

    TRACE("texture %p, srgb %#x.\n", texture, srgb);

    switch (srgb)
    {
        case SRGB_RGB:
            srgb_mode = FALSE;
            break;

        case SRGB_BOTH:
            cubetexture_preload(texture, SRGB_RGB);
            /* Fallthrough */

        case SRGB_SRGB:
            srgb_mode = TRUE;
            break;

        default:
            srgb_mode = texture->baseTexture.is_srgb;
            break;
    }

    gl_tex = basetexture_get_gl_texture(texture, gl_info, srgb_mode);

    /* We only have to activate a context for gl when we're not drawing.
     * In most cases PreLoad will be called during draw and a context was
     * activated at the beginning of drawPrimitive. */
    if (!device->isInDraw)
    {
        /* No danger of recursive calls, context_acquire() sets isInDraw to true
         * when loading offscreen render targets into their texture. */
        context = context_acquire(device, NULL);
    }

    if (texture->resource.format->id == WINED3DFMT_P8_UINT
            || texture->resource.format->id == WINED3DFMT_P8_UINT_A8_UNORM)
    {
        for (i = 0; i < sub_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = surface_from_resource(texture->baseTexture.sub_resources[i]);

            if (palette9_changed(surface))
            {
                TRACE("Reloading surface %p because the d3d8/9 palette was changed.\n", surface);
                /* TODO: This is not necessarily needed with hw palettized texture support. */
                surface_load_location(surface, SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change,
                 * this kills performance though :( */
                surface_modify_location(surface, SFLAG_INTEXTURE, FALSE);
            }
        }
    }

    /* If the texture is marked dirty or the srgb sampler setting has changed
     * since the last load then reload the surfaces. */
    if (gl_tex->dirty)
    {
        for (i = 0; i < sub_count; ++i)
        {
            surface_load(surface_from_resource(texture->baseTexture.sub_resources[i]), srgb_mode);
        }
    }
    else
    {
        TRACE("Texture %p not dirty, nothing to do.\n" , texture);
    }

    /* No longer dirty. */
    gl_tex->dirty = FALSE;

    if (context) context_release(context);
}

/* Do not call while under the GL lock. */
static void cubetexture_unload(struct wined3d_resource *resource)
{
    IWineD3DBaseTextureImpl *texture = basetexture_from_resource(resource);
    UINT sub_count = texture->baseTexture.level_count * texture->baseTexture.layer_count;
    UINT i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->baseTexture.sub_resources[i];
        IWineD3DSurfaceImpl *surface = surface_from_resource(sub_resource);

        sub_resource->resource_ops->resource_unload(sub_resource);
        surface_set_texture_name(surface, 0, TRUE);
        surface_set_texture_name(surface, 0, FALSE);
    }

    basetexture_unload(texture);
}

static const struct wined3d_texture_ops cubetexture_ops =
{
    cubetexture_bind,
    cubetexture_preload,
};

static const struct wined3d_resource_ops cubetexture_resource_ops =
{
    cubetexture_unload,
};

static void cubetexture_cleanup(IWineD3DBaseTextureImpl *This)
{
    UINT sub_count = This->baseTexture.level_count * This->baseTexture.layer_count;
    UINT i;

    TRACE("(%p) : Cleaning up.\n", This);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = This->baseTexture.sub_resources[i];

        if (sub_resource)
        {
            IWineD3DSurfaceImpl *surface = surface_from_resource(sub_resource);

            /* Clean out the texture name we gave to the surface so that the
             * surface doesn't try and release it. */
            surface_set_texture_name(surface, 0, TRUE);
            surface_set_texture_name(surface, 0, FALSE);
            surface_set_texture_target(surface, 0);
            surface_set_container(surface, WINED3D_CONTAINER_NONE, NULL);
            IWineD3DSurface_Release((IWineD3DSurface *)surface);
        }
    }
    basetexture_cleanup((IWineD3DBaseTextureImpl *)This);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DCubeTextureImpl_AddRef(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

/* Do not call while under the GL lock. */
static ULONG WINAPI IWineD3DCubeTextureImpl_Release(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (!ref)
    {
        cubetexture_cleanup(This);
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface,
        REFGUID riid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data(&((IWineD3DBaseTextureImpl *)iface)->resource, riid, data, data_size, flags);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data(&((IWineD3DBaseTextureImpl *)iface)->resource, guid, data, data_size);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID refguid)
{
    return resource_free_private_data(&((IWineD3DBaseTextureImpl *)iface)->resource, refguid);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD priority)
{
    return resource_set_priority(&((IWineD3DBaseTextureImpl *)iface)->resource, priority);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetPriority(IWineD3DBaseTexture *iface)
{
    return resource_get_priority(&((IWineD3DBaseTextureImpl *)iface)->resource);
}

/* Do not call while under the GL lock. */
static void WINAPI IWineD3DCubeTextureImpl_PreLoad(IWineD3DBaseTexture *iface)
{
    cubetexture_preload((IWineD3DBaseTextureImpl *)iface, SRGB_ANY);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DCubeTextureImpl_GetType(IWineD3DBaseTexture *iface)
{
    return resource_get_type(&((IWineD3DBaseTextureImpl *)iface)->resource);
}

static void * WINAPI IWineD3DCubeTextureImpl_GetParent(IWineD3DBaseTexture *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DBaseTextureImpl *)iface)->resource.parent;
}

static DWORD WINAPI IWineD3DCubeTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew)
{
    return basetexture_set_lod((IWineD3DBaseTextureImpl *)iface, LODNew);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetLOD(IWineD3DBaseTexture *iface)
{
    return basetexture_get_lod((IWineD3DBaseTextureImpl *)iface);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface)
{
    return basetexture_get_level_count((IWineD3DBaseTextureImpl *)iface);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface,
        WINED3DTEXTUREFILTERTYPE FilterType)
{
  return basetexture_set_autogen_filter_type((IWineD3DBaseTextureImpl *)iface, FilterType);
}

static WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DCubeTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface)
{
  return basetexture_get_autogen_filter_type((IWineD3DBaseTextureImpl *)iface);
}

static void WINAPI IWineD3DCubeTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface)
{
    basetexture_generate_mipmaps((IWineD3DBaseTextureImpl *)iface);
}

static struct wined3d_resource * WINAPI IWineD3DCubeTextureImpl_GetSubResource(IWineD3DBaseTexture *iface,
        UINT sub_resource_idx)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    return basetexture_get_sub_resource(texture, sub_resource_idx);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_AddDirtyRegion(IWineD3DBaseTexture *iface,
        UINT layer, const WINED3DBOX *dirty_region)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, layer %u, dirty_region %p.\n", iface, layer, dirty_region);

    if (!(sub_resource = basetexture_get_sub_resource(texture, layer * texture->baseTexture.level_count)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    basetexture_set_dirty(texture, TRUE);
    surface_add_dirty_rect(surface_from_resource(sub_resource), dirty_region);

    return WINED3D_OK;
}

static const IWineD3DBaseTextureVtbl IWineD3DCubeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DCubeTextureImpl_QueryInterface,
    IWineD3DCubeTextureImpl_AddRef,
    IWineD3DCubeTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DCubeTextureImpl_GetParent,
    IWineD3DCubeTextureImpl_SetPrivateData,
    IWineD3DCubeTextureImpl_GetPrivateData,
    IWineD3DCubeTextureImpl_FreePrivateData,
    IWineD3DCubeTextureImpl_SetPriority,
    IWineD3DCubeTextureImpl_GetPriority,
    IWineD3DCubeTextureImpl_PreLoad,
    IWineD3DCubeTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DCubeTextureImpl_SetLOD,
    IWineD3DCubeTextureImpl_GetLOD,
    IWineD3DCubeTextureImpl_GetLevelCount,
    IWineD3DCubeTextureImpl_SetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GenerateMipSubLevels,
    IWineD3DCubeTextureImpl_GetSubResource,
    IWineD3DCubeTextureImpl_AddDirtyRegion,
};

HRESULT cubetexture_init(IWineD3DBaseTextureImpl *texture, UINT edge_length, UINT levels,
        IWineD3DDeviceImpl *device, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
    UINT pow2_edge_length;
    unsigned int i, j;
    UINT tmp_w;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= format_id)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_TEXTURE_CUBE_MAP] && pool != WINED3DPOOL_SCRATCH)
    {
        WARN("(%p) : Tried to create not supported cube texture.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping */
    if (usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        levels = 1;
    }
    else if (!levels)
    {
        levels = wined3d_log2i(edge_length) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    texture->lpVtbl = &IWineD3DCubeTexture_Vtbl;

    hr = basetexture_init((IWineD3DBaseTextureImpl *)texture, &cubetexture_ops,
            6, levels, WINED3DRTYPE_CUBETEXTURE, device, usage, format, pool,
            parent, parent_ops, &cubetexture_resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize basetexture, returning %#x\n", hr);
        return hr;
    }

    /* Find the nearest pow2 match. */
    pow2_edge_length = 1;
    while (pow2_edge_length < edge_length) pow2_edge_length <<= 1;

    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] || (edge_length == pow2_edge_length))
    {
        /* Precalculated scaling for 'faked' non power of two texture coords. */
        texture->baseTexture.pow2Matrix[0] = 1.0f;
        texture->baseTexture.pow2Matrix[5] = 1.0f;
        texture->baseTexture.pow2Matrix[10] = 1.0f;
        texture->baseTexture.pow2Matrix[15] = 1.0f;
    }
    else
    {
        /* Precalculated scaling for 'faked' non power of two texture coords. */
        texture->baseTexture.pow2Matrix[0] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[5] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[10] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[15] = 1.0f;
        texture->baseTexture.pow2Matrix_identity = FALSE;
    }
    texture->baseTexture.target = GL_TEXTURE_CUBE_MAP_ARB;

    /* Generate all the surfaces. */
    tmp_w = edge_length;
    for (i = 0; i < texture->baseTexture.level_count; ++i)
    {
        /* Create the 6 faces. */
        for (j = 0; j < 6; ++j)
        {
            static const GLenum cube_targets[6] =
            {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
            };
            UINT idx = j * texture->baseTexture.level_count + i;
            IWineD3DSurface *surface;

            hr = IWineD3DDeviceParent_CreateSurface(device->device_parent, parent, tmp_w, tmp_w,
                    format_id, usage, pool, i /* Level */, j, &surface);
            if (FAILED(hr))
            {
                FIXME("(%p) Failed to create surface, hr %#x.\n", texture, hr);
                cubetexture_cleanup(texture);
                return hr;
            }

            surface_set_container((IWineD3DSurfaceImpl *)surface, WINED3D_CONTAINER_TEXTURE, (IWineD3DBase *)texture);
            surface_set_texture_target((IWineD3DSurfaceImpl *)surface, cube_targets[j]);
            texture->baseTexture.sub_resources[idx] = &((IWineD3DSurfaceImpl *)surface)->resource;
            TRACE("Created surface level %u @ %p.\n", i, surface);
        }
        tmp_w = max(1, tmp_w >> 1);
    }

    return WINED3D_OK;
}
