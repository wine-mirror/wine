/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

HRESULT basetexture_init(IWineD3DBaseTextureImpl *texture, const struct wined3d_texture_ops *texture_ops,
        UINT layer_count, UINT level_count, WINED3DRESOURCETYPE resource_type, IWineD3DDeviceImpl *device,
        DWORD usage, const struct wined3d_format *format, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, const struct wined3d_resource_ops *resource_ops)
{
    HRESULT hr;

    hr = resource_init(&texture->resource, device, resource_type, format,
            WINED3DMULTISAMPLE_NONE, 0, usage, pool, 0, 0, 0, 0,
            parent, parent_ops, resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        return hr;
    }

    texture->baseTexture.texture_ops = texture_ops;
    texture->baseTexture.sub_resources = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            level_count * layer_count * sizeof(*texture->baseTexture.sub_resources));
    if (!texture->baseTexture.sub_resources)
    {
        ERR("Failed to allocate sub-resource array.\n");
        resource_cleanup(&texture->resource);
        return E_OUTOFMEMORY;
    }

    texture->baseTexture.layer_count = layer_count;
    texture->baseTexture.level_count = level_count;
    texture->baseTexture.filterType = (usage & WINED3DUSAGE_AUTOGENMIPMAP) ? WINED3DTEXF_LINEAR : WINED3DTEXF_NONE;
    texture->baseTexture.LOD = 0;
    texture->baseTexture.texture_rgb.dirty = TRUE;
    texture->baseTexture.texture_srgb.dirty = TRUE;
    texture->baseTexture.is_srgb = FALSE;
    texture->baseTexture.pow2Matrix_identity = TRUE;

    if (texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING)
    {
        texture->baseTexture.minMipLookup = minMipLookup;
        texture->baseTexture.magLookup = magLookup;
    }
    else
    {
        texture->baseTexture.minMipLookup = minMipLookup_noFilter;
        texture->baseTexture.magLookup = magLookup_noFilter;
    }

    return WINED3D_OK;
}

void basetexture_cleanup(IWineD3DBaseTextureImpl *texture)
{
    basetexture_unload(texture);
    HeapFree(GetProcessHeap(), 0, texture->baseTexture.sub_resources);
    resource_cleanup(&texture->resource);
}

struct wined3d_resource *basetexture_get_sub_resource(IWineD3DBaseTextureImpl *texture, UINT sub_resource_idx)
{
    UINT sub_count = texture->baseTexture.level_count * texture->baseTexture.layer_count;

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return texture->baseTexture.sub_resources[sub_resource_idx];
}

/* A GL context is provided by the caller */
static void gltexture_delete(struct gl_texture *tex)
{
    ENTER_GL();
    glDeleteTextures(1, &tex->name);
    LEAVE_GL();
    tex->name = 0;
}

void basetexture_unload(IWineD3DBaseTextureImpl *texture)
{
    IWineD3DDeviceImpl *device = texture->resource.device;
    struct wined3d_context *context = NULL;

    if (texture->baseTexture.texture_rgb.name || texture->baseTexture.texture_srgb.name)
    {
        context = context_acquire(device, NULL);
    }

    if (texture->baseTexture.texture_rgb.name)
        gltexture_delete(&texture->baseTexture.texture_rgb);

    if (texture->baseTexture.texture_srgb.name)
        gltexture_delete(&texture->baseTexture.texture_srgb);

    if (context) context_release(context);

    basetexture_set_dirty(texture, TRUE);

    resource_unload(&texture->resource);
}

DWORD basetexture_set_lod(IWineD3DBaseTextureImpl *texture, DWORD lod)
{
    DWORD old = texture->baseTexture.LOD;

    TRACE("texture %p, lod %u.\n", texture, lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    if (texture->resource.pool != WINED3DPOOL_MANAGED)
    {
        TRACE("Ignoring SetLOD on %s texture, returning 0.\n", debug_d3dpool(texture->resource.pool));
        return 0;
    }

    if (lod >= texture->baseTexture.level_count)
        lod = texture->baseTexture.level_count - 1;

    if (texture->baseTexture.LOD != lod)
    {
        texture->baseTexture.LOD = lod;

        texture->baseTexture.texture_rgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        texture->baseTexture.texture_srgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        if (texture->baseTexture.bindCount)
            IWineD3DDeviceImpl_MarkStateDirty(texture->resource.device, STATE_SAMPLER(texture->baseTexture.sampler));
    }

    return old;
}

DWORD basetexture_get_lod(IWineD3DBaseTextureImpl *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->baseTexture.LOD);

    return texture->baseTexture.LOD;
}

DWORD basetexture_get_level_count(IWineD3DBaseTextureImpl *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->baseTexture.level_count);
    return texture->baseTexture.level_count;
}

HRESULT basetexture_set_autogen_filter_type(IWineD3DBaseTextureImpl *texture, WINED3DTEXTUREFILTERTYPE filter_type)
{
    TRACE("texture %p, filter_type %s.\n", texture, debug_d3dtexturefiltertype(filter_type));

    if (!(texture->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP))
    {
        WARN("Texture doesn't have AUTOGENMIPMAP usage.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->baseTexture.filterType != filter_type)
    {
        GLenum target = texture->baseTexture.target;
        struct wined3d_context *context;

        context = context_acquire(texture->resource.device, NULL);

        ENTER_GL();
        glBindTexture(target, texture->baseTexture.texture_rgb.name);
        checkGLcall("glBindTexture");
        switch (filter_type)
        {
            case WINED3DTEXF_NONE:
            case WINED3DTEXF_POINT:
                glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_FASTEST);
                checkGLcall("glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_FASTEST)");
                break;

            case WINED3DTEXF_LINEAR:
                glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
                checkGLcall("glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST)");
                break;

            default:
                WARN("Unexpected filter type %#x, setting to GL_NICEST.\n", filter_type);
                glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
                checkGLcall("glTexParameteri(target, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST)");
                break;
        }
        LEAVE_GL();

        context_release(context);
    }
    texture->baseTexture.filterType = filter_type;

    return WINED3D_OK;
}

WINED3DTEXTUREFILTERTYPE basetexture_get_autogen_filter_type(IWineD3DBaseTextureImpl *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->baseTexture.filterType;
}

void basetexture_generate_mipmaps(IWineD3DBaseTextureImpl *texture)
{
    /* TODO: Implement filters using GL_SGI_generate_mipmaps. */
    FIXME("texture %p stub!\n", texture);
}

void basetexture_set_dirty(IWineD3DBaseTextureImpl *texture, BOOL dirty)
{
    texture->baseTexture.texture_rgb.dirty = dirty;
    texture->baseTexture.texture_srgb.dirty = dirty;
}

/* Context activation is done by the caller. */
HRESULT basetexture_bind(IWineD3DBaseTextureImpl *texture,
        const struct wined3d_gl_info *gl_info, BOOL srgb, BOOL *set_surface_desc)
{
    HRESULT hr = WINED3D_OK;
    GLenum textureDimensions;
    BOOL isNewTexture = FALSE;
    struct gl_texture *gl_tex;

    TRACE("texture %p, srgb %#x, set_surface_desc %p.\n", texture, srgb, set_surface_desc);

    texture->baseTexture.is_srgb = srgb; /* SRGB mode cache for PreLoad calls outside drawprim */
    gl_tex = basetexture_get_gl_texture(texture, gl_info, srgb);

    textureDimensions = texture->baseTexture.target;

    ENTER_GL();
    /* Generate a texture name if we don't already have one */
    if (!gl_tex->name)
    {
        *set_surface_desc = TRUE;
        glGenTextures(1, &gl_tex->name);
        checkGLcall("glGenTextures");
        TRACE("Generated texture %d\n", gl_tex->name);
        if (texture->resource.pool == WINED3DPOOL_DEFAULT)
        {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            glPrioritizeTextures(1, &gl_tex->name, &tmp);

        }
        /* Initialise the state of the texture object
        to the openGL defaults, not the directx defaults */
        gl_tex->states[WINED3DTEXSTA_ADDRESSU]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_ADDRESSV]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_ADDRESSW]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_BORDERCOLOR]   = 0;
        gl_tex->states[WINED3DTEXSTA_MAGFILTER]     = WINED3DTEXF_LINEAR;
        gl_tex->states[WINED3DTEXSTA_MINFILTER]     = WINED3DTEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
        gl_tex->states[WINED3DTEXSTA_MIPFILTER]     = WINED3DTEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
        gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL]   = 0;
        gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = 1;
        if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
            gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE] = TRUE;
        else
            gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE] = srgb;
        gl_tex->states[WINED3DTEXSTA_SHADOW]        = FALSE;
        basetexture_set_dirty(texture, TRUE);
        isNewTexture = TRUE;

        if (texture->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP)
        {
            /* This means double binding the texture at creation, but keeps the code simpler all
             * in all, and the run-time path free from additional checks
             */
            glBindTexture(textureDimensions, gl_tex->name);
            checkGLcall("glBindTexture");
            glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
            checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE)");
        }
    } else {
        *set_surface_desc = FALSE;
    }

    /* Bind the texture */
    if (gl_tex->name)
    {
        glBindTexture(textureDimensions, gl_tex->name);
        checkGLcall("glBindTexture");
        if (isNewTexture) {
            /* For a new texture we have to set the textures levels after binding the texture.
             * In theory this is all we should ever have to do, but because ATI's drivers are broken, we
             * also need to set the texture dimensions before the texture is set
             * Beware that texture rectangles do not support mipmapping, but set the maxmiplevel if we're
             * relying on the partial GL_ARB_texture_non_power_of_two emulation with texture rectangles
             * (ie, do not care for cond_np2 here, just look for GL_TEXTURE_RECTANGLE_ARB)
             */
            if (textureDimensions != GL_TEXTURE_RECTANGLE_ARB)
            {
                TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", texture->baseTexture.level_count - 1);
                glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, texture->baseTexture.level_count - 1);
                checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, texture->baseTexture.level_count)");
            }
            if(textureDimensions==GL_TEXTURE_CUBE_MAP_ARB) {
                /* Cubemaps are always set to clamp, regardless of the sampler state. */
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            }
        }
    } else { /* this only happened if we've run out of openGL textures */
        WARN("This texture doesn't have an openGL texture assigned to it\n");
        hr =  WINED3DERR_INVALIDCALL;
    }

    LEAVE_GL();
    return hr;
}

/* GL locking is done by the caller */
static void apply_wrap(const struct wined3d_gl_info *gl_info, GLenum target,
        WINED3DTEXTUREADDRESS d3d_wrap, GLenum param, BOOL cond_np2)
{
    GLint gl_wrap;

    if (d3d_wrap < WINED3DTADDRESS_WRAP || d3d_wrap > WINED3DTADDRESS_MIRRORONCE)
    {
        FIXME("Unrecognized or unsupported WINED3DTEXTUREADDRESS %#x.\n", d3d_wrap);
        return;
    }

    if (target == GL_TEXTURE_CUBE_MAP_ARB
            || (cond_np2 && d3d_wrap == WINED3DTADDRESS_WRAP))
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_wrap = GL_CLAMP_TO_EDGE;
    }
    else
    {
        gl_wrap = gl_info->wrap_lookup[d3d_wrap - WINED3DTADDRESS_WRAP];
    }

    TRACE("Setting param %#x to %#x for target %#x.\n", param, gl_wrap, target);
    glTexParameteri(target, param, gl_wrap);
    checkGLcall("glTexParameteri(target, param, gl_wrap)");
}

/* GL locking is done by the caller (state handler) */
void basetexture_apply_state_changes(IWineD3DBaseTextureImpl *texture,
        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1],
        const struct wined3d_gl_info *gl_info)
{
    BOOL cond_np2 = texture->baseTexture.cond_np2;
    GLenum textureDimensions = texture->baseTexture.target;
    DWORD state;
    DWORD aniso;
    struct gl_texture *gl_tex;

    TRACE("texture %p, samplerStates %p\n", texture, samplerStates);

    gl_tex = basetexture_get_gl_texture(texture, gl_info, texture->baseTexture.is_srgb);

    /* This function relies on the correct texture being bound and loaded. */

    if(samplerStates[WINED3DSAMP_ADDRESSU]      != gl_tex->states[WINED3DTEXSTA_ADDRESSU]) {
        state = samplerStates[WINED3DSAMP_ADDRESSU];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_S, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSU] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSV]      != gl_tex->states[WINED3DTEXSTA_ADDRESSV]) {
        state = samplerStates[WINED3DSAMP_ADDRESSV];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_T, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSV] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSW]      != gl_tex->states[WINED3DTEXSTA_ADDRESSW]) {
        state = samplerStates[WINED3DSAMP_ADDRESSW];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_R, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSW] = state;
    }

    if(samplerStates[WINED3DSAMP_BORDERCOLOR]   != gl_tex->states[WINED3DTEXSTA_BORDERCOLOR]) {
        float col[4];

        state = samplerStates[WINED3DSAMP_BORDERCOLOR];
        D3DCOLORTOGLFLOAT4(state, col);
        TRACE("Setting border color for %u to %x\n", textureDimensions, state);
        glTexParameterfv(textureDimensions, GL_TEXTURE_BORDER_COLOR, &col[0]);
        checkGLcall("glTexParameterfv(..., GL_TEXTURE_BORDER_COLOR, ...)");
        gl_tex->states[WINED3DTEXSTA_BORDERCOLOR] = state;
    }

    if(samplerStates[WINED3DSAMP_MAGFILTER]     != gl_tex->states[WINED3DTEXSTA_MAGFILTER]) {
        GLint glValue;
        state = samplerStates[WINED3DSAMP_MAGFILTER];
        if (state > WINED3DTEXF_ANISOTROPIC) {
            FIXME("Unrecognized or unsupported MAGFILTER* value %d\n", state);
        }

        glValue = wined3d_gl_mag_filter(texture->baseTexture.magLookup,
                min(max(state, WINED3DTEXF_POINT), WINED3DTEXF_LINEAR));
        TRACE("ValueMAG=%d setting MAGFILTER to %x\n", state, glValue);
        glTexParameteri(textureDimensions, GL_TEXTURE_MAG_FILTER, glValue);

        gl_tex->states[WINED3DTEXSTA_MAGFILTER] = state;
    }

    if((samplerStates[WINED3DSAMP_MINFILTER]     != gl_tex->states[WINED3DTEXSTA_MINFILTER] ||
        samplerStates[WINED3DSAMP_MIPFILTER]     != gl_tex->states[WINED3DTEXSTA_MIPFILTER] ||
        samplerStates[WINED3DSAMP_MAXMIPLEVEL]   != gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL])) {
        GLint glValue;

        gl_tex->states[WINED3DTEXSTA_MIPFILTER] = samplerStates[WINED3DSAMP_MIPFILTER];
        gl_tex->states[WINED3DTEXSTA_MINFILTER] = samplerStates[WINED3DSAMP_MINFILTER];
        gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] = samplerStates[WINED3DSAMP_MAXMIPLEVEL];

        if (gl_tex->states[WINED3DTEXSTA_MINFILTER] > WINED3DTEXF_ANISOTROPIC
            || gl_tex->states[WINED3DTEXSTA_MIPFILTER] > WINED3DTEXF_ANISOTROPIC)
        {

            FIXME("Unrecognized or unsupported D3DSAMP_MINFILTER value %d D3DSAMP_MIPFILTER value %d\n",
                  gl_tex->states[WINED3DTEXSTA_MINFILTER],
                  gl_tex->states[WINED3DTEXSTA_MIPFILTER]);
        }
        glValue = wined3d_gl_min_mip_filter(texture->baseTexture.minMipLookup,
                min(max(samplerStates[WINED3DSAMP_MINFILTER], WINED3DTEXF_POINT), WINED3DTEXF_LINEAR),
                min(max(samplerStates[WINED3DSAMP_MIPFILTER], WINED3DTEXF_NONE), WINED3DTEXF_LINEAR));

        TRACE("ValueMIN=%d, ValueMIP=%d, setting MINFILTER to %x\n",
              samplerStates[WINED3DSAMP_MINFILTER],
              samplerStates[WINED3DSAMP_MIPFILTER], glValue);
        glTexParameteri(textureDimensions, GL_TEXTURE_MIN_FILTER, glValue);
        checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");

        if (!cond_np2)
        {
            if (gl_tex->states[WINED3DTEXSTA_MIPFILTER] == WINED3DTEXF_NONE)
                glValue = texture->baseTexture.LOD;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] >= texture->baseTexture.level_count)
                glValue = texture->baseTexture.level_count - 1;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] < texture->baseTexture.LOD)
                /* baseTexture.LOD is already clamped in the setter */
                glValue = texture->baseTexture.LOD;
            else
                glValue = gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL];

            /* Note that D3DSAMP_MAXMIPLEVEL specifies the biggest mipmap(default 0), while
             * GL_TEXTURE_MAX_LEVEL specifies the smallest mimap used(default 1000).
             * So D3DSAMP_MAXMIPLEVEL is the same as GL_TEXTURE_BASE_LEVEL.
             */
            glTexParameteri(textureDimensions, GL_TEXTURE_BASE_LEVEL, glValue);
        }
    }

    if ((gl_tex->states[WINED3DTEXSTA_MAGFILTER] != WINED3DTEXF_ANISOTROPIC
         && gl_tex->states[WINED3DTEXSTA_MINFILTER] != WINED3DTEXF_ANISOTROPIC
         && gl_tex->states[WINED3DTEXSTA_MIPFILTER] != WINED3DTEXF_ANISOTROPIC)
            || cond_np2)
    {
        aniso = 1;
    }
    else
    {
        aniso = samplerStates[WINED3DSAMP_MAXANISOTROPY];
    }

    if (gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] != aniso)
    {
        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
        {
            glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            checkGLcall("glTexParameteri(GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso)");
        }
        else
        {
            WARN("Anisotropic filtering not supported.\n");
        }
        gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = aniso;
    }

    /* These should always be the same unless EXT_texture_sRGB_decode is supported. */
    if (samplerStates[WINED3DSAMP_SRGBTEXTURE] != gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE])
    {
        glTexParameteri(textureDimensions, GL_TEXTURE_SRGB_DECODE_EXT,
                samplerStates[WINED3DSAMP_SRGBTEXTURE] ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT);
        checkGLcall("glTexParameteri(GL_TEXTURE_SRGB_DECODE_EXT)");
    }

    if (!(texture->resource.format->flags & WINED3DFMT_FLAG_SHADOW)
            != !gl_tex->states[WINED3DTEXSTA_SHADOW])
    {
        if (texture->resource.format->flags & WINED3DFMT_FLAG_SHADOW)
        {
            glTexParameteri(textureDimensions, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
            checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = TRUE;
        }
        else
        {
            glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
            checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = FALSE;
        }
    }
}
