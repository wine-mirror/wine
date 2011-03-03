/*
 * IWineD3DTexture implementation
 *
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
static HRESULT texture_bind(IWineD3DBaseTextureImpl *texture,
        const struct wined3d_gl_info *gl_info, BOOL srgb)
{
    BOOL set_gl_texture_desc;
    HRESULT hr;

    TRACE("texture %p, gl_info %p, srgb %#x.\n", texture, gl_info, srgb);

    hr = basetexture_bind(texture, gl_info, srgb, &set_gl_texture_desc);
    if (set_gl_texture_desc && SUCCEEDED(hr))
    {
        BOOL srgb_tex = !gl_info->supported[EXT_TEXTURE_SRGB_DECODE] && texture->baseTexture.is_srgb;
        struct gl_texture *gl_tex;
        UINT i;

        gl_tex = basetexture_get_gl_texture(texture, gl_info, srgb_tex);

        for (i = 0; i < texture->baseTexture.level_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = surface_from_resource(texture->baseTexture.sub_resources[i]);
            surface_set_texture_name(surface, gl_tex->name, srgb_tex);
        }

        /* Conditinal non power of two textures use a different clamping
         * default. If we're using the GL_WINE_normalized_texrect partial
         * driver emulation, we're dealing with a GL_TEXTURE_2D texture which
         * has the address mode set to repeat - something that prevents us
         * from hitting the accelerated codepath. Thus manually set the GL
         * state. The same applies to filtering. Even if the texture has only
         * one mip level, the default LINEAR_MIPMAP_LINEAR filter causes a SW
         * fallback on macos. */
        if (IWineD3DBaseTexture_IsCondNP2((IWineD3DBaseTexture *)texture))
        {
            GLenum target = texture->baseTexture.target;

            ENTER_GL();
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)");
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)");
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST)");
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST)");
            LEAVE_GL();
            gl_tex->states[WINED3DTEXSTA_ADDRESSU]      = WINED3DTADDRESS_CLAMP;
            gl_tex->states[WINED3DTEXSTA_ADDRESSV]      = WINED3DTADDRESS_CLAMP;
            gl_tex->states[WINED3DTEXSTA_MAGFILTER]     = WINED3DTEXF_POINT;
            gl_tex->states[WINED3DTEXSTA_MINFILTER]     = WINED3DTEXF_POINT;
            gl_tex->states[WINED3DTEXSTA_MIPFILTER]     = WINED3DTEXF_NONE;
        }
    }

    return hr;
}

/* Do not call while under the GL lock. */
static void texture_preload(IWineD3DBaseTextureImpl *texture, enum WINED3DSRGB srgb)
{
    IWineD3DDeviceImpl *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_context *context = NULL;
    struct gl_texture *gl_tex;
    unsigned int i;
    BOOL srgb_mode;

    TRACE("texture %p, srgb %#x.\n", texture, srgb);

    switch (srgb)
    {
        case SRGB_RGB:
            srgb_mode = FALSE;
            break;

        case SRGB_BOTH:
            texture_preload(texture, SRGB_RGB);
            /* Fallthrough */

        case SRGB_SRGB:
            srgb_mode = TRUE;
            break;

        default:
            srgb_mode = texture->baseTexture.is_srgb;
            break;
    }

    gl_tex = basetexture_get_gl_texture(texture, gl_info, srgb_mode);

    if (!device->isInDraw)
    {
        /* context_acquire() sets isInDraw to TRUE when loading a pbuffer into a texture,
         * thus no danger of recursive calls. */
        context = context_acquire(device, NULL);
    }

    if (texture->resource.format->id == WINED3DFMT_P8_UINT
            || texture->resource.format->id == WINED3DFMT_P8_UINT_A8_UNORM)
    {
        for (i = 0; i < texture->baseTexture.level_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = surface_from_resource(texture->baseTexture.sub_resources[i]);
            if (palette9_changed(surface))
            {
                TRACE("Reloading surface because the d3d8/9 palette was changed.\n");
                /* TODO: This is not necessarily needed with hw palettized texture support. */
                surface_load_location(surface, SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change, this kills performance though :( */
                surface_modify_location(surface, SFLAG_INTEXTURE, FALSE);
            }
        }
    }

    /* If the texture is marked dirty or the srgb sampler setting has changed
     * since the last load then reload the surfaces. */
    if (gl_tex->dirty)
    {
        for (i = 0; i < texture->baseTexture.level_count; ++i)
        {
            surface_load(surface_from_resource(texture->baseTexture.sub_resources[i]), srgb_mode);
        }
    }
    else
    {
        TRACE("Texture %p not dirty, nothing to do.\n", texture);
    }

    if (context) context_release(context);

    /* No longer dirty. */
    gl_tex->dirty = FALSE;
}

/* Do not call while under the GL lock. */
static void texture_unload(struct wined3d_resource *resource)
{
    IWineD3DBaseTextureImpl *texture = basetexture_from_resource(resource);
    unsigned int i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < texture->baseTexture.level_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->baseTexture.sub_resources[i];
        IWineD3DSurfaceImpl *surface = surface_from_resource(sub_resource);

        sub_resource->resource_ops->resource_unload(sub_resource);
        surface_set_texture_name(surface, 0, FALSE); /* Delete rgb name */
        surface_set_texture_name(surface, 0, TRUE); /* delete srgb name */
    }

    basetexture_unload(texture);
}

static const struct wined3d_texture_ops texture_ops =
{
    texture_bind,
    texture_preload,
};

static const struct wined3d_resource_ops texture_resource_ops =
{
    texture_unload,
};

static void texture_cleanup(IWineD3DTextureImpl *This)
{
    unsigned int i;

    TRACE("(%p) : Cleaning up\n", This);

    for (i = 0; i < This->baseTexture.level_count; ++i)
    {
        IWineD3DSurfaceImpl *surface = surface_from_resource(This->baseTexture.sub_resources[i]);
        if (surface)
        {
            /* Clean out the texture name we gave to the surface so that the
             * surface doesn't try and release it */
            surface_set_texture_name(surface, 0, TRUE);
            surface_set_texture_name(surface, 0, FALSE);
            surface_set_texture_target(surface, 0);
            surface_set_container(surface, WINED3D_CONTAINER_NONE, NULL);
            IWineD3DSurface_Release((IWineD3DSurface *)surface);
        }
    }

    TRACE("(%p) : Cleaning up base texture\n", This);
    basetexture_cleanup((IWineD3DBaseTextureImpl *)This);
}

/* *******************************************
   IWineD3DTexture IUnknown parts follow
   ******************************************* */

static HRESULT WINAPI IWineD3DTextureImpl_QueryInterface(IWineD3DTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DTexture)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return WINED3D_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DTextureImpl_AddRef(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

/* Do not call while under the GL lock. */
static ULONG WINAPI IWineD3DTextureImpl_Release(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (!ref)
    {
        texture_cleanup(This);
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}


/* ****************************************************
   IWineD3DTexture IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DTextureImpl_SetPrivateData(IWineD3DTexture *iface,
        REFGUID riid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data(&((IWineD3DTextureImpl *)iface)->resource, riid, data, data_size, flags);
}

static HRESULT WINAPI IWineD3DTextureImpl_GetPrivateData(IWineD3DTexture *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data(&((IWineD3DTextureImpl *)iface)->resource, guid, data, data_size);
}

static HRESULT WINAPI IWineD3DTextureImpl_FreePrivateData(IWineD3DTexture *iface, REFGUID refguid)
{
    return resource_free_private_data(&((IWineD3DTextureImpl *)iface)->resource, refguid);
}

static DWORD WINAPI IWineD3DTextureImpl_SetPriority(IWineD3DTexture *iface, DWORD priority)
{
    return resource_set_priority(&((IWineD3DTextureImpl *)iface)->resource, priority);
}

static DWORD WINAPI IWineD3DTextureImpl_GetPriority(IWineD3DTexture *iface)
{
    return resource_get_priority(&((IWineD3DTextureImpl *)iface)->resource);
}

/* Do not call while under the GL lock. */
static void WINAPI IWineD3DTextureImpl_PreLoad(IWineD3DTexture *iface)
{
    texture_preload((IWineD3DBaseTextureImpl *)iface, SRGB_ANY);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DTextureImpl_GetType(IWineD3DTexture *iface)
{
    return resource_get_type(&((IWineD3DTextureImpl *)iface)->resource);
}

static void * WINAPI IWineD3DTextureImpl_GetParent(IWineD3DTexture *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DTextureImpl *)iface)->resource.parent;
}

/* ******************************************************
   IWineD3DTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
static DWORD WINAPI IWineD3DTextureImpl_SetLOD(IWineD3DTexture *iface, DWORD LODNew) {
    return basetexture_set_lod((IWineD3DBaseTextureImpl *)iface, LODNew);
}

static DWORD WINAPI IWineD3DTextureImpl_GetLOD(IWineD3DTexture *iface) {
    return basetexture_get_lod((IWineD3DBaseTextureImpl *)iface);
}

static DWORD WINAPI IWineD3DTextureImpl_GetLevelCount(IWineD3DTexture *iface)
{
    return basetexture_get_level_count((IWineD3DBaseTextureImpl *)iface);
}

static HRESULT WINAPI IWineD3DTextureImpl_SetAutoGenFilterType(IWineD3DTexture *iface,
        WINED3DTEXTUREFILTERTYPE FilterType)
{
  return basetexture_set_autogen_filter_type((IWineD3DBaseTextureImpl *)iface, FilterType);
}

static WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DTextureImpl_GetAutoGenFilterType(IWineD3DTexture *iface)
{
  return basetexture_get_autogen_filter_type((IWineD3DBaseTextureImpl *)iface);
}

static void WINAPI IWineD3DTextureImpl_GenerateMipSubLevels(IWineD3DTexture *iface)
{
    basetexture_generate_mipmaps((IWineD3DBaseTextureImpl *)iface);
}

static BOOL WINAPI IWineD3DTextureImpl_IsCondNP2(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return This->cond_np2;
}

static HRESULT WINAPI IWineD3DTextureImpl_GetLevelDesc(IWineD3DTexture *iface,
        UINT sub_resource_idx, WINED3DSURFACE_DESC *desc)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u, desc %p.\n", iface, sub_resource_idx, desc);

    if (!(sub_resource = basetexture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    IWineD3DSurface_GetDesc((IWineD3DSurface *)surface_from_resource(sub_resource), desc);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DTextureImpl_GetSurfaceLevel(IWineD3DTexture *iface,
        UINT sub_resource_idx, IWineD3DSurface **surface)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u, surface %p.\n", iface, sub_resource_idx, surface);

    if (!(sub_resource = basetexture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    *surface = (IWineD3DSurface *)surface_from_resource(sub_resource);
    IWineD3DSurface_AddRef(*surface);

    TRACE("Returning surface %p.\n", *surface);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DTextureImpl_Map(IWineD3DTexture *iface,
        UINT sub_resource_idx, WINED3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u, locked_rect %p, rect %s, flags %#x.\n",
            iface, sub_resource_idx, locked_rect, wine_dbgstr_rect(rect), flags);

    if (!(sub_resource = basetexture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DSurface_Map((IWineD3DSurface *)surface_from_resource(sub_resource), locked_rect, rect, flags);
}

static HRESULT WINAPI IWineD3DTextureImpl_Unmap(IWineD3DTexture *iface, UINT sub_resource_idx)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    if (!(sub_resource = basetexture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DSurface_Unmap((IWineD3DSurface *)surface_from_resource(sub_resource));
}

static HRESULT WINAPI IWineD3DTextureImpl_AddDirtyRect(IWineD3DTexture *iface, const RECT *dirty_rect)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, dirty_rect %s.\n", iface, wine_dbgstr_rect(dirty_rect));

    if (!(sub_resource = basetexture_get_sub_resource(texture, 0)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    basetexture_set_dirty(texture, TRUE);
    surface_add_dirty_rect(surface_from_resource(sub_resource), dirty_rect);

    return WINED3D_OK;
}

static const IWineD3DTextureVtbl IWineD3DTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DTextureImpl_QueryInterface,
    IWineD3DTextureImpl_AddRef,
    IWineD3DTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DTextureImpl_GetParent,
    IWineD3DTextureImpl_SetPrivateData,
    IWineD3DTextureImpl_GetPrivateData,
    IWineD3DTextureImpl_FreePrivateData,
    IWineD3DTextureImpl_SetPriority,
    IWineD3DTextureImpl_GetPriority,
    IWineD3DTextureImpl_PreLoad,
    IWineD3DTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DTextureImpl_SetLOD,
    IWineD3DTextureImpl_GetLOD,
    IWineD3DTextureImpl_GetLevelCount,
    IWineD3DTextureImpl_SetAutoGenFilterType,
    IWineD3DTextureImpl_GetAutoGenFilterType,
    IWineD3DTextureImpl_GenerateMipSubLevels,
    IWineD3DTextureImpl_IsCondNP2,
    /* IWineD3DTexture */
    IWineD3DTextureImpl_GetLevelDesc,
    IWineD3DTextureImpl_GetSurfaceLevel,
    IWineD3DTextureImpl_Map,
    IWineD3DTextureImpl_Unmap,
    IWineD3DTextureImpl_AddDirtyRect
};

HRESULT texture_init(IWineD3DTextureImpl *texture, UINT width, UINT height, UINT levels,
        IWineD3DDeviceImpl *device, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
    UINT pow2_width, pow2_height;
    UINT tmp_w, tmp_h;
    unsigned int i;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= format_id)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Non-power2 support. */
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        pow2_width = width;
        pow2_height = height;
    }
    else
    {
        /* Find the nearest pow2 match. */
        pow2_width = pow2_height = 1;
        while (pow2_width < width) pow2_width <<= 1;
        while (pow2_height < height) pow2_height <<= 1;

        if (pow2_width != width || pow2_height != height)
        {
            if (levels > 1)
            {
                WARN("Attempted to create a mipmapped np2 texture without unconditional np2 support.\n");
                return WINED3DERR_INVALIDCALL;
            }
            levels = 1;
        }
    }

    /* Calculate levels for mip mapping. */
    if (usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
        {
            WARN("No mipmap generation support, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        levels = 1;
    }
    else if (!levels)
    {
        levels = wined3d_log2i(max(width, height)) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    texture->lpVtbl = &IWineD3DTexture_Vtbl;

    hr = basetexture_init((IWineD3DBaseTextureImpl *)texture, &texture_ops,
            1, levels, WINED3DRTYPE_TEXTURE, device, usage, format, pool,
            parent, parent_ops, &texture_resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize basetexture, returning %#x.\n", hr);
        return hr;
    }

    /* Precalculated scaling for 'faked' non power of two texture coords.
     * Second also don't use ARB_TEXTURE_RECTANGLE in case the surface format is P8 and EXT_PALETTED_TEXTURE
     * is used in combination with texture uploads (RTL_READTEX). The reason is that EXT_PALETTED_TEXTURE
     * doesn't work in combination with ARB_TEXTURE_RECTANGLE. */
    if (gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT] && (width != pow2_width || height != pow2_height))
    {
        texture->baseTexture.pow2Matrix[0] = 1.0f;
        texture->baseTexture.pow2Matrix[5] = 1.0f;
        texture->baseTexture.pow2Matrix[10] = 1.0f;
        texture->baseTexture.pow2Matrix[15] = 1.0f;
        texture->baseTexture.target = GL_TEXTURE_2D;
        texture->cond_np2 = TRUE;
        texture->baseTexture.minMipLookup = minMipLookup_noFilter;
    }
    else if (gl_info->supported[ARB_TEXTURE_RECTANGLE] && (width != pow2_width || height != pow2_height)
            && !(format->id == WINED3DFMT_P8_UINT && gl_info->supported[EXT_PALETTED_TEXTURE]
            && wined3d_settings.rendertargetlock_mode == RTL_READTEX))
    {
        if ((width != 1) || (height != 1)) texture->baseTexture.pow2Matrix_identity = FALSE;

        texture->baseTexture.pow2Matrix[0] = (float)width;
        texture->baseTexture.pow2Matrix[5] = (float)height;
        texture->baseTexture.pow2Matrix[10] = 1.0f;
        texture->baseTexture.pow2Matrix[15] = 1.0f;
        texture->baseTexture.target = GL_TEXTURE_RECTANGLE_ARB;
        texture->cond_np2 = TRUE;

        if (texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING)
        {
            texture->baseTexture.minMipLookup = minMipLookup_noMip;
        }
        else
        {
            texture->baseTexture.minMipLookup = minMipLookup_noFilter;
        }
    }
    else
    {
        if ((width != pow2_width) || (height != pow2_height))
        {
            texture->baseTexture.pow2Matrix_identity = FALSE;
            texture->baseTexture.pow2Matrix[0] = (((float)width) / ((float)pow2_width));
            texture->baseTexture.pow2Matrix[5] = (((float)height) / ((float)pow2_height));
        }
        else
        {
            texture->baseTexture.pow2Matrix[0] = 1.0f;
            texture->baseTexture.pow2Matrix[5] = 1.0f;
        }

        texture->baseTexture.pow2Matrix[10] = 1.0f;
        texture->baseTexture.pow2Matrix[15] = 1.0f;
        texture->baseTexture.target = GL_TEXTURE_2D;
        texture->cond_np2 = FALSE;
    }
    TRACE("xf(%f) yf(%f)\n", texture->baseTexture.pow2Matrix[0], texture->baseTexture.pow2Matrix[5]);

    /* Generate all the surfaces. */
    tmp_w = width;
    tmp_h = height;
    for (i = 0; i < texture->baseTexture.level_count; ++i)
    {
        IWineD3DSurface *surface;

        /* Use the callback to create the texture surface. */
        hr = IWineD3DDeviceParent_CreateSurface(device->device_parent, parent, tmp_w, tmp_h,
                format->id, usage, pool, i, 0, &surface);
        if (FAILED(hr))
        {
            FIXME("Failed to create surface %p, hr %#x\n", texture, hr);
            texture_cleanup(texture);
            return hr;
        }

        surface_set_container((IWineD3DSurfaceImpl *)surface, WINED3D_CONTAINER_TEXTURE, (IWineD3DBase *)texture);
        surface_set_texture_target((IWineD3DSurfaceImpl *)surface, texture->baseTexture.target);
        texture->baseTexture.sub_resources[i] = &((IWineD3DSurfaceImpl *)surface)->resource;
        TRACE("Created surface level %u @ %p.\n", i, surface);
        /* Calculate the next mipmap level. */
        tmp_w = max(1, tmp_w >> 1);
        tmp_h = max(1, tmp_h >> 1);
    }

    return WINED3D_OK;
}
