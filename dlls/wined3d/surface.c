/*
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2011, 2013-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2007-2008 Henri Verbeet
 * Copyright 2006-2008 Roderick Colenbrander
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static const DWORD surface_simple_locations = WINED3D_LOCATION_SYSMEM
        | WINED3D_LOCATION_USER_MEMORY | WINED3D_LOCATION_BUFFER;

void surface_get_drawable_size(const struct wined3d_surface *surface, const struct wined3d_context *context,
        unsigned int *width, unsigned int *height)
{
    if (surface->container->swapchain)
    {
        /* The drawable size of an onscreen drawable is the surface size.
         * (Actually: The window size, but the surface is created in window
         * size.) */
        *width = context->current_rt.texture->resource.width;
        *height = context->current_rt.texture->resource.height;
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_swapchain *swapchain = context->swapchain;

        /* The drawable size of a backbuffer / aux buffer offscreen target is
         * the size of the current context's drawable, which is the size of
         * the back buffer of the swapchain the active context belongs to. */
        *width = swapchain->desc.backbuffer_width;
        *height = swapchain->desc.backbuffer_height;
    }
    else
    {
        struct wined3d_surface *rt;

        /* The drawable size of an FBO target is the OpenGL texture size,
         * which is the power of two size. */
        rt = context->current_rt.texture->sub_resources[context->current_rt.sub_resource_idx].u.surface;
        *width = wined3d_texture_get_level_pow2_width(rt->container, rt->texture_level);
        *height = wined3d_texture_get_level_pow2_height(rt->container, rt->texture_level);
    }
}

struct blt_info
{
    GLenum binding;
    GLenum bind_target;
    enum wined3d_gl_resource_type tex_type;
    GLfloat coords[4][3];
};

struct float_rect
{
    float l;
    float t;
    float r;
    float b;
};

static inline void cube_coords_float(const RECT *r, UINT w, UINT h, struct float_rect *f)
{
    f->l = ((r->left * 2.0f) / w) - 1.0f;
    f->t = ((r->top * 2.0f) / h) - 1.0f;
    f->r = ((r->right * 2.0f) / w) - 1.0f;
    f->b = ((r->bottom * 2.0f) / h) - 1.0f;
}

static void surface_get_blt_info(GLenum target, const RECT *rect, GLsizei w, GLsizei h, struct blt_info *info)
{
    GLfloat (*coords)[3] = info->coords;
    struct float_rect f;

    switch (target)
    {
        default:
            FIXME("Unsupported texture target %#x\n", target);
            /* Fall back to GL_TEXTURE_2D */
        case GL_TEXTURE_2D:
            info->binding = GL_TEXTURE_BINDING_2D;
            info->bind_target = GL_TEXTURE_2D;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_2D;
            coords[0][0] = (float)rect->left / w;
            coords[0][1] = (float)rect->top / h;
            coords[0][2] = 0.0f;

            coords[1][0] = (float)rect->right / w;
            coords[1][1] = (float)rect->top / h;
            coords[1][2] = 0.0f;

            coords[2][0] = (float)rect->left / w;
            coords[2][1] = (float)rect->bottom / h;
            coords[2][2] = 0.0f;

            coords[3][0] = (float)rect->right / w;
            coords[3][1] = (float)rect->bottom / h;
            coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            info->binding = GL_TEXTURE_BINDING_RECTANGLE_ARB;
            info->bind_target = GL_TEXTURE_RECTANGLE_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_RECT;
            coords[0][0] = rect->left;  coords[0][1] = rect->top;       coords[0][2] = 0.0f;
            coords[1][0] = rect->right; coords[1][1] = rect->top;       coords[1][2] = 0.0f;
            coords[2][0] = rect->left;  coords[2][1] = rect->bottom;    coords[2][2] = 0.0f;
            coords[3][0] = rect->right; coords[3][1] = rect->bottom;    coords[3][2] = 0.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] =  1.0f;   coords[0][1] = -f.t;   coords[0][2] = -f.l;
            coords[1][0] =  1.0f;   coords[1][1] = -f.t;   coords[1][2] = -f.r;
            coords[2][0] =  1.0f;   coords[2][1] = -f.b;   coords[2][2] = -f.l;
            coords[3][0] =  1.0f;   coords[3][1] = -f.b;   coords[3][2] = -f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = -1.0f;   coords[0][1] = -f.t;   coords[0][2] = f.l;
            coords[1][0] = -1.0f;   coords[1][1] = -f.t;   coords[1][2] = f.r;
            coords[2][0] = -1.0f;   coords[2][1] = -f.b;   coords[2][2] = f.l;
            coords[3][0] = -1.0f;   coords[3][1] = -f.b;   coords[3][2] = f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] =  1.0f;   coords[0][2] = f.t;
            coords[1][0] = f.r;   coords[1][1] =  1.0f;   coords[1][2] = f.t;
            coords[2][0] = f.l;   coords[2][1] =  1.0f;   coords[2][2] = f.b;
            coords[3][0] = f.r;   coords[3][1] =  1.0f;   coords[3][2] = f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] = -1.0f;   coords[0][2] = -f.t;
            coords[1][0] = f.r;   coords[1][1] = -1.0f;   coords[1][2] = -f.t;
            coords[2][0] = f.l;   coords[2][1] = -1.0f;   coords[2][2] = -f.b;
            coords[3][0] = f.r;   coords[3][1] = -1.0f;   coords[3][2] = -f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = f.l;   coords[0][1] = -f.t;   coords[0][2] =  1.0f;
            coords[1][0] = f.r;   coords[1][1] = -f.t;   coords[1][2] =  1.0f;
            coords[2][0] = f.l;   coords[2][1] = -f.b;   coords[2][2] =  1.0f;
            coords[3][0] = f.r;   coords[3][1] = -f.b;   coords[3][2] =  1.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            info->binding = GL_TEXTURE_BINDING_CUBE_MAP_ARB;
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            info->tex_type = WINED3D_GL_RES_TYPE_TEX_CUBE;
            cube_coords_float(rect, w, h, &f);

            coords[0][0] = -f.l;   coords[0][1] = -f.t;   coords[0][2] = -1.0f;
            coords[1][0] = -f.r;   coords[1][1] = -f.t;   coords[1][2] = -1.0f;
            coords[2][0] = -f.l;   coords[2][1] = -f.b;   coords[2][2] = -1.0f;
            coords[3][0] = -f.r;   coords[3][1] = -f.b;   coords[3][2] = -1.0f;
            break;
    }
}

static void surface_get_rect(const struct wined3d_surface *surface, const RECT *rect_in, RECT *rect_out)
{
    if (rect_in)
        *rect_out = *rect_in;
    else
    {
        const struct wined3d_texture *texture = surface->container;

        SetRect(rect_out, 0, 0, wined3d_texture_get_level_width(texture, surface->texture_level),
                wined3d_texture_get_level_height(texture, surface->texture_level));
    }
}

/* Context activation is done by the caller. */
void draw_textured_quad(const struct wined3d_surface *src_surface, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect, enum wined3d_texture_filter_type filter)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture *texture = src_surface->container;
    struct blt_info info;

    surface_get_blt_info(src_surface->texture_target, src_rect,
            wined3d_texture_get_level_pow2_width(texture, src_surface->texture_level),
            wined3d_texture_get_level_pow2_height(texture, src_surface->texture_level), &info);

    gl_info->gl_ops.gl.p_glEnable(info.bind_target);
    checkGLcall("glEnable(bind_target)");

    context_bind_texture(context, info.bind_target, texture->texture_rgb.name);

    /* Filtering for StretchRect */
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (context->gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    checkGLcall("glTexEnvi");

    /* Draw a quad */
    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[0]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->left, dst_rect->top);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[1]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->right, dst_rect->top);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[2]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->left, dst_rect->bottom);

    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[3]);
    gl_info->gl_ops.gl.p_glVertex2i(dst_rect->right, dst_rect->bottom);
    gl_info->gl_ops.gl.p_glEnd();

    /* Unbind the texture */
    context_bind_texture(context, info.bind_target, 0);

    /* We changed the filtering settings on the texture. Inform the
     * container about this to get the filters reset properly next draw. */
    texture->texture_rgb.sampler_desc.mag_filter = WINED3D_TEXF_POINT;
    texture->texture_rgb.sampler_desc.min_filter = WINED3D_TEXF_POINT;
    texture->texture_rgb.sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    texture->texture_rgb.sampler_desc.srgb_decode = FALSE;
}

/* Works correctly only for <= 4 bpp formats. */
static void get_color_masks(const struct wined3d_format *format, DWORD *masks)
{
    masks[0] = ((1u << format->red_size) - 1) << format->red_offset;
    masks[1] = ((1u << format->green_size) - 1) << format->green_offset;
    masks[2] = ((1u << format->blue_size) - 1) << format->blue_offset;
}

void wined3d_surface_destroy_dc(struct wined3d_surface *surface)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = NULL;
    D3DKMT_DESTROYDCFROMMEMORY destroy_desc;
    struct wined3d_context *context = NULL;
    struct wined3d_bo_address data;
    NTSTATUS status;

    if (!surface->dc)
    {
        ERR("Surface %p has no DC.\n", surface);
        return;
    }

    TRACE("dc %p, bitmap %p.\n", surface->dc, surface->bitmap);

    destroy_desc.hDc = surface->dc;
    destroy_desc.hBitmap = surface->bitmap;
    if ((status = D3DKMTDestroyDCFromMemory(&destroy_desc)))
        ERR("Failed to destroy dc, status %#x.\n", status);
    surface->dc = NULL;
    surface->bitmap = NULL;

    if (device->d3d_initialized)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
    }

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    wined3d_texture_unmap_bo_address(&data, gl_info, GL_PIXEL_UNPACK_BUFFER);

    if (context)
        context_release(context);
}

HRESULT wined3d_surface_create_dc(struct wined3d_surface *surface)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    const struct wined3d_format *format = texture->resource.format;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = NULL;
    struct wined3d_context *context = NULL;
    unsigned int row_pitch, slice_pitch;
    struct wined3d_bo_address data;
    D3DKMT_CREATEDCFROMMEMORY desc;
    NTSTATUS status;

    TRACE("surface %p.\n", surface);

    if (!format->ddi_format)
    {
        WARN("Cannot create a DC for format %s.\n", debug_d3dformat(format->id));
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_texture_get_pitch(texture, surface->texture_level, &row_pitch, &slice_pitch);

    if (device->d3d_initialized)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
    }

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    desc.pMemory = wined3d_texture_map_bo_address(&data, texture->sub_resources[sub_resource_idx].size,
            gl_info, GL_PIXEL_UNPACK_BUFFER, 0);

    if (context)
        context_release(context);

    desc.Format = format->ddi_format;
    desc.Width = wined3d_texture_get_level_width(texture, surface->texture_level);
    desc.Height = wined3d_texture_get_level_height(texture, surface->texture_level);
    desc.Pitch = row_pitch;
    desc.hDeviceDc = CreateCompatibleDC(NULL);
    desc.pColorTable = NULL;

    status = D3DKMTCreateDCFromMemory(&desc);
    DeleteDC(desc.hDeviceDc);
    if (status)
    {
        WARN("Failed to create DC, status %#x.\n", status);
        return WINED3DERR_INVALIDCALL;
    }

    surface->dc = desc.hDc;
    surface->bitmap = desc.hBitmap;

    TRACE("Created DC %p, bitmap %p for surface %p.\n", surface->dc, surface->bitmap, surface);

    return WINED3D_OK;
}

static BOOL surface_is_full_rect(const struct wined3d_surface *surface, const RECT *r)
{
    unsigned int t;

    t = wined3d_texture_get_level_width(surface->container, surface->texture_level);
    if ((r->left && r->right) || abs(r->right - r->left) != t)
        return FALSE;
    t = wined3d_texture_get_level_height(surface->container, surface->texture_level);
    if ((r->top && r->bottom) || abs(r->bottom - r->top) != t)
        return FALSE;
    return TRUE;
}

static void surface_depth_blt_fbo(const struct wined3d_device *device,
        struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect)
{
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    unsigned int src_sub_resource_idx = surface_get_sub_resource_idx(src_surface);
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_texture *src_texture = src_surface->container;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    DWORD src_mask, dst_mask;
    GLbitfield gl_mask;

    TRACE("device %p\n", device);
    TRACE("src_surface %p, src_location %s, src_rect %s,\n",
            src_surface, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect));
    TRACE("dst_surface %p, dst_location %s, dst_rect %s.\n",
            dst_surface, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect));

    src_mask = src_texture->resource.format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    dst_mask = dst_texture->resource.format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);

    if (src_mask != dst_mask)
    {
        ERR("Incompatible formats %s and %s.\n",
                debug_d3dformat(src_texture->resource.format->id),
                debug_d3dformat(dst_texture->resource.format->id));
        return;
    }

    if (!src_mask)
    {
        ERR("Not a depth / stencil format: %s.\n",
                debug_d3dformat(src_texture->resource.format->id));
        return;
    }

    gl_mask = 0;
    if (src_mask & WINED3DFMT_FLAG_DEPTH)
        gl_mask |= GL_DEPTH_BUFFER_BIT;
    if (src_mask & WINED3DFMT_FLAG_STENCIL)
        gl_mask |= GL_STENCIL_BUFFER_BIT;

    context = context_acquire(device, NULL);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, src_location);
    if (!surface_is_full_rect(dst_surface, dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    gl_info = context->gl_info;

    context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, NULL, src_surface, src_location);
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

    context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, NULL, dst_surface, dst_location);
    context_set_draw_buffer(context, GL_NONE);
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    if (gl_mask & GL_DEPTH_BUFFER_BIT)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZWRITEENABLE));
    }
    if (gl_mask & GL_STENCIL_BUFFER_BIT)
    {
        if (context->gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE));
        }
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
    }

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, gl_mask, GL_NEAREST);
    checkGLcall("glBlitFramebuffer()");

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

/* Blit between surface locations. Onscreen on different swapchains is not supported.
 * Depth / stencil is not supported. Context activation is done by the caller. */
static void surface_blt_fbo(const struct wined3d_device *device,
        struct wined3d_context *old_ctx, enum wined3d_texture_filter_type filter,
        struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect_in)
{
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    unsigned int src_sub_resource_idx = surface_get_sub_resource_idx(src_surface);
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_texture *src_texture = src_surface->container;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context = old_ctx;
    struct wined3d_surface *required_rt, *restore_rt = NULL;
    RECT src_rect, dst_rect;
    GLenum gl_filter;
    GLenum buffer;

    TRACE("device %p, filter %s,\n", device, debug_d3dtexturefiltertype(filter));
    TRACE("src_surface %p, src_location %s, src_rect %s,\n",
            src_surface, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect_in));
    TRACE("dst_surface %p, dst_location %s, dst_rect %s.\n",
            dst_surface, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect_in));

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    switch (filter)
    {
        case WINED3D_TEXF_LINEAR:
            gl_filter = GL_LINEAR;
            break;

        default:
            FIXME("Unsupported filter mode %s (%#x).\n", debug_d3dtexturefiltertype(filter), filter);
        case WINED3D_TEXF_NONE:
        case WINED3D_TEXF_POINT:
            gl_filter = GL_NEAREST;
            break;
    }

    /* Resolve the source surface first if needed. */
    if (src_location == WINED3D_LOCATION_RB_MULTISAMPLE
            && (src_texture->resource.format->id != dst_texture->resource.format->id
                || abs(src_rect.bottom - src_rect.top) != abs(dst_rect.bottom - dst_rect.top)
                || abs(src_rect.right - src_rect.left) != abs(dst_rect.right - dst_rect.left)))
        src_location = WINED3D_LOCATION_RB_RESOLVED;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. (And is
     * in fact harmful if we're being called by surface_load_location() with
     * the purpose of loading the destination surface.) */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, old_ctx, src_location);
    if (!surface_is_full_rect(dst_surface, &dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, old_ctx, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, old_ctx, dst_location);


    if (src_location == WINED3D_LOCATION_DRAWABLE) required_rt = src_surface;
    else if (dst_location == WINED3D_LOCATION_DRAWABLE) required_rt = dst_surface;
    else required_rt = NULL;

    restore_rt = context_get_rt_surface(old_ctx);
    if (restore_rt != required_rt)
        context = context_acquire(device, required_rt);
    else
        restore_rt = NULL;

    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        return;
    }

    gl_info = context->gl_info;

    if (src_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Source surface %p is onscreen.\n", src_surface);
        buffer = wined3d_texture_get_gl_buffer(src_texture);
        surface_translate_drawable_coords(src_surface, context->win_handle, &src_rect);
    }
    else
    {
        TRACE("Source surface %p is offscreen.\n", src_surface);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, src_surface, NULL, src_location);
    gl_info->gl_ops.gl.p_glReadBuffer(buffer);
    checkGLcall("glReadBuffer()");
    context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Destination surface %p is onscreen.\n", dst_surface);
        buffer = wined3d_texture_get_gl_buffer(dst_texture);
        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);
    }
    else
    {
        TRACE("Destination surface %p is offscreen.\n", dst_surface);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    context_apply_fbo_state_blit(context, GL_DRAW_FRAMEBUFFER, dst_surface, NULL, dst_location);
    context_set_draw_buffer(context, buffer);
    context_check_fbo_status(context, GL_DRAW_FRAMEBUFFER);
    context_invalidate_state(context, STATE_FRAMEBUFFER);

    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));

    gl_info->fbo_ops.glBlitFramebuffer(src_rect.left, src_rect.top, src_rect.right, src_rect.bottom,
            dst_rect.left, dst_rect.top, dst_rect.right, dst_rect.bottom, GL_COLOR_BUFFER_BIT, gl_filter);
    checkGLcall("glBlitFramebuffer()");

    if (wined3d_settings.strict_draw_ordering || (dst_location == WINED3D_LOCATION_DRAWABLE
            && dst_texture->swapchain->front_buffer == dst_texture))
        gl_info->gl_ops.gl.p_glFlush();

    if (restore_rt)
        context_restore(context, restore_rt);
}

static BOOL fbo_blit_supported(const struct wined3d_gl_info *gl_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    if ((wined3d_settings.offscreen_rendering_mode != ORM_FBO) || !gl_info->fbo_ops.glBlitFramebuffer)
        return FALSE;

    /* Source and/or destination need to be on the GL side */
    if (src_pool == WINED3D_POOL_SYSTEM_MEM || dst_pool == WINED3D_POOL_SYSTEM_MEM)
        return FALSE;

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            if (!((src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                    || (src_usage & WINED3DUSAGE_RENDERTARGET)))
                return FALSE;
            if (!((dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                    || (dst_usage & WINED3DUSAGE_RENDERTARGET)))
                return FALSE;
            if (!(src_format->id == dst_format->id
                    || (is_identity_fixup(src_format->color_fixup)
                    && is_identity_fixup(dst_format->color_fixup))))
                return FALSE;
            break;

        case WINED3D_BLIT_OP_DEPTH_BLIT:
            if (!(src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            if (!(dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
                return FALSE;
            /* Accept pure swizzle fixups for depth formats. In general we
             * ignore the stencil component (if present) at the moment and the
             * swizzle is not relevant with just the depth component. */
            if (is_complex_fixup(src_format->color_fixup) || is_complex_fixup(dst_format->color_fixup)
                    || is_scaling_fixup(src_format->color_fixup) || is_scaling_fixup(dst_format->color_fixup))
                return FALSE;
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

static BOOL surface_convert_depth_to_float(const struct wined3d_surface *surface, DWORD depth, float *float_depth)
{
    const struct wined3d_format *format = surface->container->resource.format;

    switch (format->id)
    {
        case WINED3DFMT_S1_UINT_D15_UNORM:
            *float_depth = depth / (float)0x00007fff;
            break;

        case WINED3DFMT_D16_UNORM:
            *float_depth = depth / (float)0x0000ffff;
            break;

        case WINED3DFMT_D24_UNORM_S8_UINT:
        case WINED3DFMT_X8D24_UNORM:
            *float_depth = depth / (float)0x00ffffff;
            break;

        case WINED3DFMT_D32_UNORM:
            *float_depth = depth / (float)0xffffffff;
            break;

        default:
            ERR("Unhandled conversion from %s to floating point.\n", debug_d3dformat(format->id));
            return FALSE;
    }

    return TRUE;
}

static HRESULT wined3d_surface_depth_fill(struct wined3d_surface *surface, const RECT *rect, float depth)
{
    struct wined3d_resource *resource = &surface->container->resource;
    struct wined3d_device *device = resource->device;
    struct wined3d_rendertarget_view_desc view_desc;
    struct wined3d_rendertarget_view *view;
    const struct blit_shader *blitter;
    HRESULT hr;

    if (!(blitter = wined3d_select_blitter(&device->adapter->gl_info, &device->adapter->d3d_info,
            WINED3D_BLIT_OP_DEPTH_FILL, NULL, 0, 0, NULL, rect, resource->usage, resource->pool, resource->format)))
    {
        FIXME("No blitter is capable of performing the requested depth fill operation.\n");
        return WINED3DERR_INVALIDCALL;
    }

    view_desc.format_id = resource->format->id;
    view_desc.u.texture.level_idx = surface->texture_level;
    view_desc.u.texture.layer_idx = surface->texture_layer;
    view_desc.u.texture.layer_count = 1;
    if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc,
            resource, NULL, &wined3d_null_parent_ops, &view)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    hr = blitter->depth_fill(device, view, rect, WINED3DCLEAR_ZBUFFER, depth, 0);
    wined3d_rendertarget_view_decref(view);

    return hr;
}

static HRESULT wined3d_surface_depth_blt(struct wined3d_surface *src_surface, DWORD src_location, const RECT *src_rect,
        struct wined3d_surface *dst_surface, DWORD dst_location, const RECT *dst_rect)
{
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_device *device = src_texture->resource.device;

    if (!fbo_blit_supported(&device->adapter->gl_info, WINED3D_BLIT_OP_DEPTH_BLIT,
            src_rect, src_texture->resource.usage, src_texture->resource.pool, src_texture->resource.format,
            dst_rect, dst_texture->resource.usage, dst_texture->resource.pool, dst_texture->resource.format))
        return WINED3DERR_INVALIDCALL;

    surface_depth_blt_fbo(device, src_surface, src_location, src_rect, dst_surface, dst_location, dst_rect);

    surface_modify_ds_location(dst_surface, dst_location,
            dst_surface->ds_current_size.cx, dst_surface->ds_current_size.cy);

    return WINED3D_OK;
}

/* This call just downloads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void surface_download_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        DWORD dst_location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    const struct wined3d_format *format = texture->resource.format;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int dst_row_pitch, dst_slice_pitch;
    unsigned int src_row_pitch, src_slice_pitch;
    struct wined3d_bo_address data;
    BYTE *temporary_mem = NULL;
    void *mem;

    /* Only support read back of converted P8 surfaces. */
    if (texture->flags & WINED3D_TEXTURE_CONVERTED && format->id != WINED3DFMT_P8_UINT)
    {
        ERR("Trying to read back converted surface %p with format %s.\n", surface, debug_d3dformat(format->id));
        return;
    }

    sub_resource = &texture->sub_resources[sub_resource_idx];

    if (surface->texture_target == GL_TEXTURE_2D_ARRAY)
    {
        /* NP2 emulation is not allowed on array textures. */
        if (texture->flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
            ERR("Array texture %p uses NP2 emulation.\n", texture);

        WARN_(d3d_perf)("Downloading all miplevel layers to get the surface data for a single sub-resource.\n");

        if (!(temporary_mem = wined3d_calloc(texture->layer_count, sub_resource->size)))
        {
            ERR("Out of memory.\n");
            return;
        }
    }

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, dst_location);

    if (texture->flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
    {
        wined3d_texture_get_pitch(texture, surface->texture_level, &dst_row_pitch, &dst_slice_pitch);
        wined3d_format_calculate_pitch(format, texture->resource.device->surface_alignment,
                wined3d_texture_get_level_pow2_width(texture, surface->texture_level),
                wined3d_texture_get_level_pow2_height(texture, surface->texture_level),
                &src_row_pitch, &src_slice_pitch);
        if (!(temporary_mem = HeapAlloc(GetProcessHeap(), 0, src_slice_pitch)))
        {
            ERR("Out of memory.\n");
            return;
        }

        if (data.buffer_object)
            ERR("NP2 emulated texture uses PBO unexpectedly.\n");
        if (texture->resource.format_flags & WINED3DFMT_FLAG_COMPRESSED)
            ERR("Unexpected compressed format for NP2 emulated texture.\n");
    }

    if (temporary_mem)
    {
        mem = temporary_mem;
    }
    else if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
        checkGLcall("glBindBuffer");
        mem = data.addr;
    }
    else
    {
        mem = data.addr;
    }

    if (texture->resource.format_flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("Downloading compressed surface %p, level %u, format %#x, type %#x, data %p.\n",
                surface, surface->texture_level, format->glFormat, format->glType, mem);

        GL_EXTCALL(glGetCompressedTexImage(surface->texture_target, surface->texture_level, mem));
        checkGLcall("glGetCompressedTexImage");
    }
    else
    {
        TRACE("Downloading surface %p, level %u, format %#x, type %#x, data %p.\n",
                surface, surface->texture_level, format->glFormat, format->glType, mem);

        gl_info->gl_ops.gl.p_glGetTexImage(surface->texture_target, surface->texture_level,
                format->glFormat, format->glType, mem);
        checkGLcall("glGetTexImage");
    }

    if (texture->flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
    {
        const BYTE *src_data;
        unsigned int h, y;
        BYTE *dst_data;
        /*
         * Some games (e.g. warhammer 40k) don't work properly with the odd pitches, preventing
         * the surface pitch from being used to box non-power2 textures. Instead we have to use a hack to
         * repack the texture so that the bpp * width pitch can be used instead of bpp * pow2width.
         *
         * We're doing this...
         *
         * instead of boxing the texture :
         * |<-texture width ->|  -->pow2width|   /\
         * |111111111111111111|              |   |
         * |222 Texture 222222| boxed empty  | texture height
         * |3333 Data 33333333|              |   |
         * |444444444444444444|              |   \/
         * -----------------------------------   |
         * |     boxed  empty | boxed empty  | pow2height
         * |                  |              |   \/
         * -----------------------------------
         *
         *
         * we're repacking the data to the expected texture width
         *
         * |<-texture width ->|  -->pow2width|   /\
         * |111111111111111111222222222222222|   |
         * |222333333333333333333444444444444| texture height
         * |444444                           |   |
         * |                                 |   \/
         * |                                 |   |
         * |            empty                | pow2height
         * |                                 |   \/
         * -----------------------------------
         *
         * == is the same as
         *
         * |<-texture width ->|    /\
         * |111111111111111111|
         * |222222222222222222|texture height
         * |333333333333333333|
         * |444444444444444444|    \/
         * --------------------
         *
         * This also means that any references to surface memory should work with the data as if it were a
         * standard texture with a non-power2 width instead of a texture boxed up to be a power2 texture.
         *
         * internally the texture is still stored in a boxed format so any references to textureName will
         * get a boxed texture with width pow2width and not a texture of width resource.width. */
        src_data = mem;
        dst_data = data.addr;
        TRACE("Repacking the surface data from pitch %u to pitch %u.\n", src_row_pitch, dst_row_pitch);
        h = wined3d_texture_get_level_height(texture, surface->texture_level);
        for (y = 0; y < h; ++y)
        {
            memcpy(dst_data, src_data, dst_row_pitch);
            src_data += src_row_pitch;
            dst_data += dst_row_pitch;
        }
    }
    else if (temporary_mem)
    {
        void *src_data = temporary_mem + surface->texture_layer * sub_resource->size;
        if (data.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
            checkGLcall("glBindBuffer");
            GL_EXTCALL(glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, sub_resource->size, src_data));
            checkGLcall("glBufferSubData");
        }
        else
        {
            memcpy(data.addr, src_data, sub_resource->size);
        }
    }

    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    HeapFree(GetProcessHeap(), 0, temporary_mem);
}

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
void wined3d_surface_upload_data(struct wined3d_surface *surface, const struct wined3d_gl_info *gl_info,
        const struct wined3d_format *format, const RECT *src_rect, UINT src_pitch, const POINT *dst_point,
        BOOL srgb, const struct wined3d_const_bo_address *data)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    UINT update_w = src_rect->right - src_rect->left;
    UINT update_h = src_rect->bottom - src_rect->top;

    TRACE("surface %p, gl_info %p, format %s, src_rect %s, src_pitch %u, dst_point %s, srgb %#x, data {%#x:%p}.\n",
            surface, gl_info, debug_d3dformat(format->id), wine_dbgstr_rect(src_rect), src_pitch,
            wine_dbgstr_point(dst_point), srgb, data->buffer_object, data->addr);

    if (texture->sub_resources[sub_resource_idx].map_count)
    {
        WARN("Uploading a surface that is currently mapped, setting WINED3D_TEXTURE_PIN_SYSMEM.\n");
        texture->flags |= WINED3D_TEXTURE_PIN_SYSMEM;
    }

    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_HEIGHT_SCALE)
    {
        update_h *= format->height_scale.numerator;
        update_h /= format->height_scale.denominator;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
    {
        unsigned int dst_row_pitch, dst_slice_pitch;
        const BYTE *addr = data->addr;
        GLenum internal;

        addr += (src_rect->top / format->block_height) * src_pitch;
        addr += (src_rect->left / format->block_width) * format->block_byte_count;

        if (srgb)
            internal = format->glGammaInternal;
        else if (texture->resource.usage & WINED3DUSAGE_RENDERTARGET
                && wined3d_resource_is_offscreen(&texture->resource))
            internal = format->rtInternal;
        else
            internal = format->glInternal;

        wined3d_format_calculate_pitch(format, 1, update_w, update_h, &dst_row_pitch, &dst_slice_pitch);

        TRACE("Uploading compressed data, target %#x, level %u, layer %u, x %d, y %d, w %u, h %u, "
                "format %#x, image_size %#x, addr %p.\n",
                surface->texture_target, surface->texture_level, surface->texture_layer,
                dst_point->x, dst_point->y, update_w, update_h, internal, dst_slice_pitch, addr);

        if (dst_row_pitch == src_pitch)
        {
            if (surface->texture_target == GL_TEXTURE_2D_ARRAY)
            {
                GL_EXTCALL(glCompressedTexSubImage3D(surface->texture_target, surface->texture_level,
                            dst_point->x, dst_point->y, surface->texture_layer, update_w, update_h, 1,
                            internal, dst_slice_pitch, addr));
            }
            else
            {
                GL_EXTCALL(glCompressedTexSubImage2D(surface->texture_target, surface->texture_level,
                            dst_point->x, dst_point->y, update_w, update_h,
                            internal, dst_slice_pitch, addr));
            }
        }
        else
        {
            UINT row_count = (update_h + format->block_height - 1) / format->block_height;
            UINT row, y;

            /* glCompressedTexSubImage2D() ignores pixel store state, so we
             * can't use the unpack row length like for glTexSubImage2D. */
            for (row = 0, y = dst_point->y; row < row_count; ++row)
            {
                if (surface->texture_target == GL_TEXTURE_2D_ARRAY)
                {
                    GL_EXTCALL(glCompressedTexSubImage3D(surface->texture_target, surface->texture_level,
                            dst_point->x, y, surface->texture_layer, update_w, format->block_height, 1,
                            internal, dst_row_pitch, addr));
                }
                else
                {
                    GL_EXTCALL(glCompressedTexSubImage2D(surface->texture_target, surface->texture_level,
                            dst_point->x, y, update_w, format->block_height, internal, dst_row_pitch, addr));
                }

                y += format->block_height;
                addr += src_pitch;
            }
        }
        checkGLcall("Upload compressed surface data");
    }
    else
    {
        const BYTE *addr = data->addr;

        addr += src_rect->top * src_pitch;
        addr += src_rect->left * format->byte_count;

        TRACE("Uploading data, target %#x, level %u, layer %u, x %d, y %d, w %u, h %u, "
                "format %#x, type %#x, addr %p.\n",
                surface->texture_target, surface->texture_level, surface->texture_layer,
                dst_point->x, dst_point->y, update_w, update_h, format->glFormat, format->glType, addr);

        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, src_pitch / format->byte_count);
        if (surface->texture_target == GL_TEXTURE_2D_ARRAY)
        {
            GL_EXTCALL(glTexSubImage3D(surface->texture_target, surface->texture_level,
                    dst_point->x, dst_point->y, surface->texture_layer, update_w, update_h, 1,
                    format->glFormat, format->glType, addr));
        }
        else
        {
            gl_info->gl_ops.gl.p_glTexSubImage2D(surface->texture_target, surface->texture_level,
                    dst_point->x, dst_point->y, update_w, update_h, format->glFormat, format->glType, addr);
        }
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        checkGLcall("Upload surface data");
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush();

    if (gl_info->quirks & WINED3D_QUIRK_FBO_TEX_UPDATE)
    {
        struct wined3d_device *device = texture->resource.device;
        unsigned int i;

        for (i = 0; i < device->context_count; ++i)
        {
            context_surface_update(device->contexts[i], surface);
        }
    }
}

static BOOL surface_check_block_align_rect(struct wined3d_surface *surface, const RECT *rect)
{
    struct wined3d_box box = {rect->left, rect->top, rect->right, rect->bottom, 0, 1};

    return wined3d_texture_check_block_align(surface->container, surface->texture_level, &box);
}

HRESULT surface_upload_from_surface(struct wined3d_surface *dst_surface, const POINT *dst_point,
        struct wined3d_surface *src_surface, const RECT *src_rect)
{
    unsigned int src_sub_resource_idx = surface_get_sub_resource_idx(src_surface);
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_texture *dst_texture = dst_surface->container;
    unsigned int src_row_pitch, src_slice_pitch;
    const struct wined3d_format *src_format;
    const struct wined3d_format *dst_format;
    unsigned int src_fmt_flags, dst_fmt_flags;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_bo_address data;
    UINT update_w, update_h;
    UINT dst_w, dst_h;
    RECT r, dst_rect;
    POINT p;

    TRACE("dst_surface %p, dst_point %s, src_surface %p, src_rect %s.\n",
            dst_surface, wine_dbgstr_point(dst_point),
            src_surface, wine_dbgstr_rect(src_rect));

    src_format = src_texture->resource.format;
    dst_format = dst_texture->resource.format;
    src_fmt_flags = src_texture->resource.format_flags;
    dst_fmt_flags = dst_texture->resource.format_flags;

    if (src_format->id != dst_format->id)
    {
        WARN("Source and destination surfaces should have the same format.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!dst_point)
    {
        p.x = 0;
        p.y = 0;
        dst_point = &p;
    }
    else if (dst_point->x < 0 || dst_point->y < 0)
    {
        WARN("Invalid destination point.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!src_rect)
    {
        SetRect(&r, 0, 0, wined3d_texture_get_level_width(src_texture, src_surface->texture_level),
                wined3d_texture_get_level_height(src_texture, src_surface->texture_level));
        src_rect = &r;
    }
    else if (src_rect->left < 0 || src_rect->top < 0 || IsRectEmpty(src_rect))
    {
        WARN("Invalid source rectangle.\n");
        return WINED3DERR_INVALIDCALL;
    }

    dst_w = wined3d_texture_get_level_width(dst_texture, dst_surface->texture_level);
    dst_h = wined3d_texture_get_level_height(dst_texture, dst_surface->texture_level);

    update_w = src_rect->right - src_rect->left;
    update_h = src_rect->bottom - src_rect->top;

    if (update_w > dst_w || dst_point->x > dst_w - update_w
            || update_h > dst_h || dst_point->y > dst_h - update_h)
    {
        WARN("Destination out of bounds.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((src_fmt_flags & WINED3DFMT_FLAG_BLOCKS) && !surface_check_block_align_rect(src_surface, src_rect))
    {
        WARN("Source rectangle not block-aligned.\n");
        return WINED3DERR_INVALIDCALL;
    }

    SetRect(&dst_rect, dst_point->x, dst_point->y, dst_point->x + update_w, dst_point->y + update_h);
    if ((dst_fmt_flags & WINED3DFMT_FLAG_BLOCKS) && !surface_check_block_align_rect(dst_surface, &dst_rect))
    {
        WARN("Destination rectangle not block-aligned.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Use wined3d_surface_blt() instead of uploading directly if we need conversion. */
    if (dst_format->convert || wined3d_format_get_color_key_conversion(dst_texture, FALSE))
        return wined3d_surface_blt(dst_surface, &dst_rect, src_surface, src_rect, 0, NULL, WINED3D_TEXF_POINT);

    context = context_acquire(dst_texture->resource.device, NULL);
    gl_info = context->gl_info;

    /* Only load the surface for partial updates. For newly allocated texture
     * the texture wouldn't be the current location, and we'd upload zeroes
     * just to overwrite them again. */
    if (update_w == dst_w && update_h == dst_h)
        wined3d_texture_prepare_texture(dst_texture, context, FALSE);
    else
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_bind_and_dirtify(dst_texture, context, FALSE);

    wined3d_texture_get_memory(src_texture, src_sub_resource_idx, &data,
            src_texture->sub_resources[src_sub_resource_idx].locations);
    wined3d_texture_get_pitch(src_texture, src_surface->texture_level, &src_row_pitch, &src_slice_pitch);

    wined3d_surface_upload_data(dst_surface, gl_info, src_format, src_rect,
            src_row_pitch, dst_point, FALSE, wined3d_const_bo_address(&data));

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);

    return WINED3D_OK;
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
/* Context activation is done by the caller. */
void surface_set_compatible_renderbuffer(struct wined3d_surface *surface, const struct wined3d_surface *rt)
{
    const struct wined3d_gl_info *gl_info = &surface->container->resource.device->adapter->gl_info;
    struct wined3d_renderbuffer_entry *entry;
    GLuint renderbuffer = 0;
    unsigned int src_width, src_height;
    unsigned int width, height;

    if (rt && rt->container->resource.format->id != WINED3DFMT_NULL)
    {
        width = wined3d_texture_get_level_pow2_width(rt->container, rt->texture_level);
        height = wined3d_texture_get_level_pow2_height(rt->container, rt->texture_level);
    }
    else
    {
        width = wined3d_texture_get_level_pow2_width(surface->container, surface->texture_level);
        height = wined3d_texture_get_level_pow2_height(surface->container, surface->texture_level);
    }

    src_width = wined3d_texture_get_level_pow2_width(surface->container, surface->texture_level);
    src_height = wined3d_texture_get_level_pow2_height(surface->container, surface->texture_level);

    /* A depth stencil smaller than the render target is not valid */
    if (width > src_width || height > src_height) return;

    /* Remove any renderbuffer set if the sizes match */
    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT]
            || (width == src_width && height == src_height))
    {
        surface->current_renderbuffer = NULL;
        return;
    }

    /* Look if we've already got a renderbuffer of the correct dimensions */
    LIST_FOR_EACH_ENTRY(entry, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        if (entry->width == width && entry->height == height)
        {
            renderbuffer = entry->id;
            surface->current_renderbuffer = entry;
            break;
        }
    }

    if (!renderbuffer)
    {
        gl_info->fbo_ops.glGenRenderbuffers(1, &renderbuffer);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER,
                surface->container->resource.format->glInternal, width, height);

        entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
        entry->width = width;
        entry->height = height;
        entry->id = renderbuffer;
        list_add_head(&surface->renderbuffers, &entry->entry);

        surface->current_renderbuffer = entry;
    }

    checkGLcall("set_compatible_renderbuffer");
}

/* See also float_16_to_32() in wined3d_private.h */
static inline unsigned short float_32_to_16(const float *in)
{
    int exp = 0;
    float tmp = fabsf(*in);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (*in == 0.0f)
        return 0x0000;
    if (isnan(*in))
        return 0x7c01;
    if (isinf(*in))
        return (*in < 0.0f ? 0xfc00 : 0x7c00);

    if (tmp < (float)(1u << 10))
    {
        do
        {
            tmp = tmp * 2.0f;
            exp--;
        } while (tmp < (float)(1u << 10));
    }
    else if (tmp >= (float)(1u << 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= (float)(1u << 11));
    }

    mantissa = (unsigned int)tmp;
    if (tmp - mantissa >= 0.5f)
        ++mantissa; /* Round to nearest, away from zero. */

    exp += 10;  /* Normalize the mantissa. */
    exp += 15;  /* Exponent is encoded with excess 15. */

    if (exp > 30) /* too big */
    {
        ret = 0x7c00; /* INF */
    }
    else if (exp <= 0)
    {
        /* exp == 0: Non-normalized mantissa. Returns 0x0000 (=0.0) for too small numbers. */
        while (exp <= 0)
        {
            mantissa = mantissa >> 1;
            ++exp;
        }
        ret = mantissa & 0x3ff;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((*in < 0.0f ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

static void convert_r32_float_r16_float(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned short *dst_s;
    const float *src_f;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        src_f = (const float *)(src + y * pitch_in);
        dst_s = (unsigned short *) (dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            dst_s[x] = float_32_to_16(src_f + x);
        }
    }
}

static void convert_r5g6b5_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    static const unsigned char convert_6to8[] =
    {
        0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
        0x20, 0x24, 0x28, 0x2d, 0x31, 0x35, 0x39, 0x3d,
        0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
        0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
        0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
        0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
        0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd7, 0xdb, 0xdf,
        0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
    };
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const WORD *src_line = (const WORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            WORD pixel = src_line[x];
            dst_line[x] = 0xff000000u
                    | convert_5to8[(pixel & 0xf800u) >> 11] << 16
                    | convert_6to8[(pixel & 0x07e0u) >> 5] << 8
                    | convert_5to8[(pixel & 0x001fu)];
        }
    }
}

/* We use this for both B8G8R8A8 -> B8G8R8X8 and B8G8R8X8 -> B8G8R8A8, since
 * in both cases we're just setting the X / Alpha channel to 0xff. */
static void convert_a8r8g8b8_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const DWORD *src_line = (const DWORD *)(src + y * pitch_in);
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);

        for (x = 0; x < w; ++x)
        {
            dst_line[x] = 0xff000000 | (src_line[x] & 0xffffff);
        }
    }
}

static inline BYTE cliptobyte(int x)
{
    return (BYTE)((x < 0) ? 0 : ((x > 255) ? 255 : x));
}

static void convert_yuy2_x8r8g8b8(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;
    unsigned int x, y;

    TRACE("Converting %ux%u pixels, pitches %u %u.\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        DWORD *dst_line = (DWORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = 0xff000000
                | cliptobyte((c2 + r2) >> 8) << 16    /* red   */
                | cliptobyte((c2 + g2) >> 8) << 8     /* green */
                | cliptobyte((c2 + b2) >> 8);         /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

static void convert_yuy2_r5g6b5(const BYTE *src, BYTE *dst,
        DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h)
{
    unsigned int x, y;
    int c2, d, e, r2 = 0, g2 = 0, b2 = 0;

    TRACE("Converting %ux%u pixels, pitches %u %u\n", w, h, pitch_in, pitch_out);

    for (y = 0; y < h; ++y)
    {
        const BYTE *src_line = src + y * pitch_in;
        WORD *dst_line = (WORD *)(dst + y * pitch_out);
        for (x = 0; x < w; ++x)
        {
            /* YUV to RGB conversion formulas from http://en.wikipedia.org/wiki/YUV:
             *     C = Y - 16; D = U - 128; E = V - 128;
             *     R = cliptobyte((298 * C + 409 * E + 128) >> 8);
             *     G = cliptobyte((298 * C - 100 * D - 208 * E + 128) >> 8);
             *     B = cliptobyte((298 * C + 516 * D + 128) >> 8);
             * Two adjacent YUY2 pixels are stored as four bytes: Y0 U Y1 V .
             * U and V are shared between the pixels. */
            if (!(x & 1)) /* For every even pixel, read new U and V. */
            {
                d = (int) src_line[1] - 128;
                e = (int) src_line[3] - 128;
                r2 = 409 * e + 128;
                g2 = - 100 * d - 208 * e + 128;
                b2 = 516 * d + 128;
            }
            c2 = 298 * ((int) src_line[0] - 16);
            dst_line[x] = (cliptobyte((c2 + r2) >> 8) >> 3) << 11   /* red   */
                | (cliptobyte((c2 + g2) >> 8) >> 2) << 5            /* green */
                | (cliptobyte((c2 + b2) >> 8) >> 3);                /* blue  */
                /* Scale RGB values to 0..255 range,
                 * then clip them if still not in range (may be negative),
                 * then shift them within DWORD if necessary. */
            src_line += 2;
        }
    }
}

struct d3dfmt_converter_desc
{
    enum wined3d_format_id from, to;
    void (*convert)(const BYTE *src, BYTE *dst, DWORD pitch_in, DWORD pitch_out, unsigned int w, unsigned int h);
};

static const struct d3dfmt_converter_desc converters[] =
{
    {WINED3DFMT_R32_FLOAT,      WINED3DFMT_R16_FLOAT,       convert_r32_float_r16_float},
    {WINED3DFMT_B5G6R5_UNORM,   WINED3DFMT_B8G8R8X8_UNORM,  convert_r5g6b5_x8r8g8b8},
    {WINED3DFMT_B8G8R8A8_UNORM, WINED3DFMT_B8G8R8X8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_B8G8R8A8_UNORM,  convert_a8r8g8b8_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B8G8R8X8_UNORM,  convert_yuy2_x8r8g8b8},
    {WINED3DFMT_YUY2,           WINED3DFMT_B5G6R5_UNORM,    convert_yuy2_r5g6b5},
};

static inline const struct d3dfmt_converter_desc *find_converter(enum wined3d_format_id from,
        enum wined3d_format_id to)
{
    unsigned int i;

    for (i = 0; i < (sizeof(converters) / sizeof(*converters)); ++i)
    {
        if (converters[i].from == from && converters[i].to == to)
            return &converters[i];
    }

    return NULL;
}

static struct wined3d_texture *surface_convert_format(struct wined3d_texture *src_texture,
        unsigned int sub_resource_idx, const struct wined3d_format *dst_format)
{
    unsigned int texture_level = sub_resource_idx % src_texture->level_count;
    const struct wined3d_format *src_format = src_texture->resource.format;
    struct wined3d_device *device = src_texture->resource.device;
    const struct d3dfmt_converter_desc *conv = NULL;
    struct wined3d_texture *dst_texture;
    struct wined3d_resource_desc desc;
    struct wined3d_map_desc src_map;

    if (!(conv = find_converter(src_format->id, dst_format->id)) && (!device->d3d_initialized
            || !is_identity_fixup(src_format->color_fixup) || src_format->convert
            || !is_identity_fixup(dst_format->color_fixup) || dst_format->convert))
    {
        FIXME("Cannot find a conversion function from format %s to %s.\n",
                debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));
        return NULL;
    }

    /* FIXME: Multisampled conversion? */
    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = dst_format->id;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = 0;
    desc.pool = WINED3D_POOL_SCRATCH;
    desc.width = wined3d_texture_get_level_width(src_texture, texture_level);
    desc.height = wined3d_texture_get_level_height(src_texture, texture_level);
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(wined3d_texture_create(device, &desc, 1, 1,
            WINED3D_TEXTURE_CREATE_MAPPABLE | WINED3D_TEXTURE_CREATE_DISCARD,
            NULL, NULL, &wined3d_null_parent_ops, &dst_texture)))
    {
        ERR("Failed to create a destination texture for conversion.\n");
        return NULL;
    }

    memset(&src_map, 0, sizeof(src_map));
    if (FAILED(wined3d_resource_map(&src_texture->resource, sub_resource_idx,
            &src_map, NULL, WINED3D_MAP_READONLY)))
    {
        ERR("Failed to map the source texture.\n");
        wined3d_texture_decref(dst_texture);
        return NULL;
    }
    if (conv)
    {
        struct wined3d_map_desc dst_map;

        memset(&dst_map, 0, sizeof(dst_map));
        if (FAILED(wined3d_resource_map(&dst_texture->resource, 0, &dst_map, NULL, 0)))
        {
            ERR("Failed to map the destination texture.\n");
            wined3d_resource_unmap(&src_texture->resource, sub_resource_idx);
            wined3d_texture_decref(dst_texture);
            return NULL;
        }

        conv->convert(src_map.data, dst_map.data, src_map.row_pitch, dst_map.row_pitch, desc.width, desc.height);

        wined3d_resource_unmap(&dst_texture->resource, 0);
    }
    else
    {
        struct wined3d_bo_address data = {0, src_map.data};
        RECT src_rect = {0, 0, desc.width, desc.height};
        const struct wined3d_gl_info *gl_info;
        struct wined3d_context *context;
        POINT dst_point = {0, 0};

        TRACE("Using upload conversion.\n");
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;

        wined3d_texture_prepare_texture(dst_texture, context, FALSE);
        wined3d_texture_bind_and_dirtify(dst_texture, context, FALSE);
        wined3d_surface_upload_data(dst_texture->sub_resources[0].u.surface, gl_info, src_format,
                &src_rect, src_map.row_pitch, &dst_point, FALSE, wined3d_const_bo_address(&data));

        context_release(context);

        wined3d_texture_validate_location(dst_texture, 0, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(dst_texture, 0, ~WINED3D_LOCATION_TEXTURE_RGB);
    }
    wined3d_resource_unmap(&src_texture->resource, sub_resource_idx);

    return dst_texture;
}

static HRESULT _Blt_ColorFill(BYTE *buf, unsigned int width, unsigned int height,
        unsigned int bpp, UINT pitch, DWORD color)
{
    BYTE *first;
    unsigned int x, y;

    /* Do first row */

#define COLORFILL_ROW(type) \
do { \
    type *d = (type *)buf; \
    for (x = 0; x < width; ++x) \
        d[x] = (type)color; \
} while(0)

    switch (bpp)
    {
        case 1:
            COLORFILL_ROW(BYTE);
            break;

        case 2:
            COLORFILL_ROW(WORD);
            break;

        case 3:
        {
            BYTE *d = buf;
            for (x = 0; x < width; ++x, d += 3)
            {
                d[0] = (color      ) & 0xff;
                d[1] = (color >>  8) & 0xff;
                d[2] = (color >> 16) & 0xff;
            }
            break;
        }
        case 4:
            COLORFILL_ROW(DWORD);
            break;

        default:
            FIXME("Color fill not implemented for bpp %u!\n", bpp * 8);
            return WINED3DERR_NOTAVAILABLE;
    }

#undef COLORFILL_ROW

    /* Now copy first row. */
    first = buf;
    for (y = 1; y < height; ++y)
    {
        buf += pitch;
        memcpy(buf, first, width * bpp);
    }

    return WINED3D_OK;
}

static void read_from_framebuffer(struct wined3d_surface *surface,
        struct wined3d_context *old_ctx, DWORD dst_location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context = old_ctx;
    struct wined3d_surface *restore_rt = NULL;
    unsigned int row_pitch, slice_pitch;
    unsigned int width, height;
    BYTE *mem;
    BYTE *row, *top, *bottom;
    int i;
    BOOL srcIsUpsideDown;
    struct wined3d_bo_address data;

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, dst_location);

    restore_rt = context_get_rt_surface(old_ctx);
    if (restore_rt != surface)
        context = context_acquire(device, surface);
    else
        restore_rt = NULL;

    context_apply_blit_state(context, device);
    gl_info = context->gl_info;

    /* Select the correct read buffer, and give some debug output.
     * There is no need to keep track of the current read buffer or reset it, every part of the code
     * that reads sets the read buffer as desired.
     */
    if (wined3d_resource_is_offscreen(&texture->resource))
    {
        /* Mapping the primary render target which is not on a swapchain.
         * Read from the back buffer. */
        TRACE("Mapping offscreen render target.\n");
        gl_info->gl_ops.gl.p_glReadBuffer(context_get_offscreen_gl_buffer(context));
        srcIsUpsideDown = TRUE;
    }
    else
    {
        /* Onscreen surfaces are always part of a swapchain */
        GLenum buffer = wined3d_texture_get_gl_buffer(texture);
        TRACE("Mapping %#x buffer.\n", buffer);
        gl_info->gl_ops.gl.p_glReadBuffer(buffer);
        checkGLcall("glReadBuffer");
        srcIsUpsideDown = FALSE;
    }

    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data.buffer_object));
        checkGLcall("glBindBuffer");
    }

    wined3d_texture_get_pitch(texture, surface->texture_level, &row_pitch, &slice_pitch);

    /* Setup pixel store pack state -- to glReadPixels into the correct place */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, row_pitch / texture->resource.format->byte_count);
    checkGLcall("glPixelStorei");

    width = wined3d_texture_get_level_width(texture, surface->texture_level);
    height = wined3d_texture_get_level_height(texture, surface->texture_level);
    gl_info->gl_ops.gl.p_glReadPixels(0, 0, width, height,
            texture->resource.format->glFormat,
            texture->resource.format->glType, data.addr);
    checkGLcall("glReadPixels");

    /* Reset previous pixel store pack state */
    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    checkGLcall("glPixelStorei");

    if (!srcIsUpsideDown)
    {
        /* glReadPixels returns the image upside down, and there is no way to
         * prevent this. Flip the lines in software. */

        if (!(row = HeapAlloc(GetProcessHeap(), 0, row_pitch)))
            goto error;

        if (data.buffer_object)
        {
            mem = GL_EXTCALL(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE));
            checkGLcall("glMapBuffer");
        }
        else
            mem = data.addr;

        top = mem;
        bottom = mem + row_pitch * (height - 1);
        for (i = 0; i < height / 2; i++)
        {
            memcpy(row, top, row_pitch);
            memcpy(top, bottom, row_pitch);
            memcpy(bottom, row, row_pitch);
            top += row_pitch;
            bottom -= row_pitch;
        }
        HeapFree(GetProcessHeap(), 0, row);

        if (data.buffer_object)
            GL_EXTCALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }

error:
    if (data.buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    if (restore_rt)
        context_restore(context, restore_rt);
}

/* Read the framebuffer contents into a texture. Note that this function
 * doesn't do any kind of flipping. Using this on an onscreen surface will
 * result in a flipped D3D texture.
 *
 * Context activation is done by the caller. This function may temporarily
 * switch to a different context and restore the original one before return. */
void surface_load_fb_texture(struct wined3d_surface *surface, BOOL srgb, struct wined3d_context *old_ctx)
{
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context = old_ctx;
    struct wined3d_surface *restore_rt = NULL;

    restore_rt = context_get_rt_surface(old_ctx);
    if (restore_rt != surface)
        context = context_acquire(device, surface);
    else
        restore_rt = NULL;

    gl_info = context->gl_info;
    device_invalidate_state(device, STATE_FRAMEBUFFER);

    wined3d_texture_prepare_texture(texture, context, srgb);
    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    TRACE("Reading back offscreen render target %p.\n", surface);

    if (wined3d_resource_is_offscreen(&texture->resource))
        gl_info->gl_ops.gl.p_glReadBuffer(context_get_offscreen_gl_buffer(context));
    else
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(texture));
    checkGLcall("glReadBuffer");

    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(surface->texture_target, surface->texture_level,
            0, 0, 0, 0, wined3d_texture_get_level_width(texture, surface->texture_level),
            wined3d_texture_get_level_height(texture, surface->texture_level));
    checkGLcall("glCopyTexSubImage2D");

    if (restore_rt)
        context_restore(context, restore_rt);
}

/* Does a direct frame buffer -> texture copy. Stretching is done with single
 * pixel copy calls. */
static void fb_copy_to_texture_direct(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, enum wined3d_texture_filter_type filter)
{
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_device *device = dst_texture->resource.device;
    const struct wined3d_gl_info *gl_info;
    float xrel, yrel;
    struct wined3d_context *context;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;
    unsigned int src_height;

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    context = context_acquire(device, src_surface);
    gl_info = context->gl_info;
    context_apply_blit_state(context, device);
    wined3d_texture_load(dst_texture, context, FALSE);

    /* Bind the target texture */
    context_bind_texture(context, dst_texture->target, dst_texture->texture_rgb.name);
    if (wined3d_resource_is_offscreen(&src_texture->resource))
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(context_get_offscreen_gl_buffer(context));
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(src_texture));
    }
    checkGLcall("glReadBuffer");

    xrel = (float) (src_rect->right - src_rect->left) / (float) (dst_rect.right - dst_rect.left);
    yrel = (float) (src_rect->bottom - src_rect->top) / (float) (dst_rect.bottom - dst_rect.top);

    if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
    {
        FIXME_(d3d_perf)("Doing a pixel by pixel copy from the framebuffer to a texture.\n");

        if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            ERR("Texture filtering not supported in direct blit.\n");
    }
    else if ((filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT)
            && ((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        ERR("Texture filtering not supported in direct blit\n");
    }

    src_height = wined3d_texture_get_level_height(src_texture, src_surface->texture_level);
    if (upsidedown
            && !((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            && !((yrel - 1.0f < -eps) || (yrel - 1.0f > eps)))
    {
        /* Upside down copy without stretching is nice, one glCopyTexSubImage call will do. */
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                dst_rect.left /*xoffset */, dst_rect.top /* y offset */,
                src_rect->left, src_height - src_rect->bottom,
                dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    }
    else
    {
        LONG row;
        UINT yoffset = src_height - src_rect->top + dst_rect.top - 1;
        /* I have to process this row by row to swap the image,
         * otherwise it would be upside down, so stretching in y direction
         * doesn't cost extra time
         *
         * However, stretching in x direction can be avoided if not necessary
         */
        for(row = dst_rect.top; row < dst_rect.bottom; row++) {
            if ((xrel - 1.0f < -eps) || (xrel - 1.0f > eps))
            {
                /* Well, that stuff works, but it's very slow.
                 * find a better way instead
                 */
                LONG col;

                for (col = dst_rect.left; col < dst_rect.right; ++col)
                {
                    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                            dst_rect.left + col /* x offset */, row /* y offset */,
                            src_rect->left + col * xrel, yoffset - (int) (row * yrel), 1, 1);
                }
            }
            else
            {
                gl_info->gl_ops.gl.p_glCopyTexSubImage2D(dst_surface->texture_target, dst_surface->texture_level,
                        dst_rect.left /* x offset */, row /* y offset */,
                        src_rect->left, yoffset - (int) (row * yrel), dst_rect.right - dst_rect.left, 1);
            }
        }
    }
    checkGLcall("glCopyTexSubImage2D");

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Uses the hardware to stretch and flip the image */
static void fb_copy_to_texture_hwstretch(struct wined3d_surface *dst_surface, struct wined3d_surface *src_surface,
        const RECT *src_rect, const RECT *dst_rect_in, enum wined3d_texture_filter_type filter)
{
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    unsigned int src_width, src_height, src_pow2_width, src_pow2_height;
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_device *device = dst_texture->resource.device;
    GLuint src, backup = 0;
    float left, right, top, bottom; /* Texture coordinates */
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    GLenum drawBuffer = GL_BACK;
    GLenum offscreen_buffer;
    GLenum texture_target;
    BOOL noBackBufferBackup;
    BOOL src_offscreen;
    BOOL upsidedown = FALSE;
    RECT dst_rect = *dst_rect_in;

    TRACE("Using hwstretch blit\n");
    /* Activate the Proper context for reading from the source surface, set it up for blitting */
    context = context_acquire(device, src_surface);
    gl_info = context->gl_info;
    context_apply_blit_state(context, device);
    wined3d_texture_load(dst_texture, context, FALSE);

    offscreen_buffer = context_get_offscreen_gl_buffer(context);
    src_width = wined3d_texture_get_level_width(src_texture, src_surface->texture_level);
    src_height = wined3d_texture_get_level_height(src_texture, src_surface->texture_level);
    src_pow2_width = wined3d_texture_get_level_pow2_width(src_texture, src_surface->texture_level);
    src_pow2_height = wined3d_texture_get_level_pow2_height(src_texture, src_surface->texture_level);

    src_offscreen = wined3d_resource_is_offscreen(&src_texture->resource);
    noBackBufferBackup = src_offscreen && wined3d_settings.offscreen_rendering_mode == ORM_FBO;
    if (!noBackBufferBackup && !src_texture->texture_rgb.name)
    {
        /* Get it a description */
        wined3d_texture_load(src_texture, context, FALSE);
    }

    /* Try to use an aux buffer for drawing the rectangle. This way it doesn't need restoring.
     * This way we don't have to wait for the 2nd readback to finish to leave this function.
     */
    if (context->aux_buffers >= 2)
    {
        /* Got more than one aux buffer? Use the 2nd aux buffer */
        drawBuffer = GL_AUX1;
    }
    else if ((!src_offscreen || offscreen_buffer == GL_BACK) && context->aux_buffers >= 1)
    {
        /* Only one aux buffer, but it isn't used (Onscreen rendering, or non-aux orm)? Use it! */
        drawBuffer = GL_AUX0;
    }

    if (noBackBufferBackup)
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &backup);
        checkGLcall("glGenTextures");
        context_bind_texture(context, GL_TEXTURE_2D, backup);
        texture_target = GL_TEXTURE_2D;
    }
    else
    {
        /* Backup the back buffer and copy the source buffer into a texture to draw an upside down stretched quad. If
         * we are reading from the back buffer, the backup can be used as source texture
         */
        texture_target = src_surface->texture_target;
        context_bind_texture(context, texture_target, src_texture->texture_rgb.name);
        gl_info->gl_ops.gl.p_glEnable(texture_target);
        checkGLcall("glEnable(texture_target)");

        /* For now invalidate the texture copy of the back buffer. Drawable and sysmem copy are untouched */
        surface_get_sub_resource(src_surface)->locations &= ~WINED3D_LOCATION_TEXTURE_RGB;
    }

    /* Make sure that the top pixel is always above the bottom pixel, and keep a separate upside down flag
     * glCopyTexSubImage is a bit picky about the parameters we pass to it
     */
    if(dst_rect.top > dst_rect.bottom) {
        UINT tmp = dst_rect.bottom;
        dst_rect.bottom = dst_rect.top;
        dst_rect.top = tmp;
        upsidedown = TRUE;
    }

    if (src_offscreen)
    {
        TRACE("Reading from an offscreen target\n");
        upsidedown = !upsidedown;
        gl_info->gl_ops.gl.p_glReadBuffer(offscreen_buffer);
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(wined3d_texture_get_gl_buffer(src_texture));
    }

    /* TODO: Only back up the part that will be overwritten */
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target, 0, 0, 0, 0, 0, src_width, src_height);

    checkGLcall("glCopyTexSubImage2D");

    /* No issue with overriding these - the sampler is dirty due to blit usage */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    checkGLcall("glTexParameteri");
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    checkGLcall("glTexParameteri");

    if (!src_texture->swapchain || src_texture == src_texture->swapchain->back_buffers[0])
    {
        src = backup ? backup : src_texture->texture_rgb.name;
    }
    else
    {
        gl_info->gl_ops.gl.p_glReadBuffer(GL_FRONT);
        checkGLcall("glReadBuffer(GL_FRONT)");

        gl_info->gl_ops.gl.p_glGenTextures(1, &src);
        checkGLcall("glGenTextures(1, &src)");
        context_bind_texture(context, GL_TEXTURE_2D, src);

        /* TODO: Only copy the part that will be read. Use src_rect->left,
         * src_rect->bottom as origin, but with the width watch out for power
         * of 2 sizes. */
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, src_pow2_width,
                src_pow2_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        checkGLcall("glTexImage2D");
        gl_info->gl_ops.gl.p_glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, src_width, src_height);

        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_info->gl_ops.gl.p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");

        gl_info->gl_ops.gl.p_glReadBuffer(GL_BACK);
        checkGLcall("glReadBuffer(GL_BACK)");

        if (texture_target != GL_TEXTURE_2D)
        {
            gl_info->gl_ops.gl.p_glDisable(texture_target);
            gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
            texture_target = GL_TEXTURE_2D;
        }
    }
    checkGLcall("glEnd and previous");

    left = src_rect->left;
    right = src_rect->right;

    if (!upsidedown)
    {
        top = src_height - src_rect->top;
        bottom = src_height - src_rect->bottom;
    }
    else
    {
        top = src_height - src_rect->bottom;
        bottom = src_height - src_rect->top;
    }

    if (src_texture->flags & WINED3D_TEXTURE_NORMALIZED_COORDS)
    {
        left /= src_pow2_width;
        right /= src_pow2_width;
        top /= src_pow2_height;
        bottom /= src_pow2_height;
    }

    /* draw the source texture stretched and upside down. The correct surface is bound already */
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    context_set_draw_buffer(context, drawBuffer);
    gl_info->gl_ops.gl.p_glReadBuffer(drawBuffer);

    gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
        /* bottom left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(0, 0);

        /* top left */
        gl_info->gl_ops.gl.p_glTexCoord2f(left, top);
        gl_info->gl_ops.gl.p_glVertex2i(0, dst_rect.bottom - dst_rect.top);

        /* top right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, top);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);

        /* bottom right */
        gl_info->gl_ops.gl.p_glTexCoord2f(right, bottom);
        gl_info->gl_ops.gl.p_glVertex2i(dst_rect.right - dst_rect.left, 0);
    gl_info->gl_ops.gl.p_glEnd();
    checkGLcall("glEnd and previous");

    if (texture_target != dst_surface->texture_target)
    {
        gl_info->gl_ops.gl.p_glDisable(texture_target);
        gl_info->gl_ops.gl.p_glEnable(dst_surface->texture_target);
        texture_target = dst_surface->texture_target;
    }

    /* Now read the stretched and upside down image into the destination texture */
    context_bind_texture(context, texture_target, dst_texture->texture_rgb.name);
    gl_info->gl_ops.gl.p_glCopyTexSubImage2D(texture_target,
                        0,
                        dst_rect.left, dst_rect.top, /* xoffset, yoffset */
                        0, 0, /* We blitted the image to the origin */
                        dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top);
    checkGLcall("glCopyTexSubImage2D");

    if (drawBuffer == GL_BACK)
    {
        /* Write the back buffer backup back. */
        if (backup)
        {
            if (texture_target != GL_TEXTURE_2D)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_2D);
                texture_target = GL_TEXTURE_2D;
            }
            context_bind_texture(context, GL_TEXTURE_2D, backup);
        }
        else
        {
            if (texture_target != src_surface->texture_target)
            {
                gl_info->gl_ops.gl.p_glDisable(texture_target);
                gl_info->gl_ops.gl.p_glEnable(src_surface->texture_target);
                texture_target = src_surface->texture_target;
            }
            context_bind_texture(context, src_surface->texture_target, src_texture->texture_rgb.name);
        }

        gl_info->gl_ops.gl.p_glBegin(GL_QUADS);
            /* top left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(0, src_height);

            /* bottom left */
            gl_info->gl_ops.gl.p_glTexCoord2f(0.0f, (float)src_height / (float)src_pow2_height);
            gl_info->gl_ops.gl.p_glVertex2i(0, 0);

            /* bottom right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)src_width / (float)src_pow2_width,
                    (float)src_height / (float)src_pow2_height);
            gl_info->gl_ops.gl.p_glVertex2i(src_width, 0);

            /* top right */
            gl_info->gl_ops.gl.p_glTexCoord2f((float)src_width / (float)src_pow2_width, 0.0f);
            gl_info->gl_ops.gl.p_glVertex2i(src_width, src_height);
        gl_info->gl_ops.gl.p_glEnd();
    }
    gl_info->gl_ops.gl.p_glDisable(texture_target);
    checkGLcall("glDisable(texture_target)");

    /* Cleanup */
    if (src != src_texture->texture_rgb.name && src != backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &src);
        checkGLcall("glDeleteTextures(1, &src)");
    }
    if (backup)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &backup);
        checkGLcall("glDeleteTextures(1, &backup)");
    }

    if (wined3d_settings.strict_draw_ordering)
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);

    /* The texture is now most up to date - If the surface is a render target
     * and has a drawable, this path is never entered. */
    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Front buffer coordinates are always full screen coordinates, but our GL
 * drawable is limited to the window's client area. The sysmem and texture
 * copies do have the full screen size. Note that GL has a bottom-left
 * origin, while D3D has a top-left origin. */
void surface_translate_drawable_coords(const struct wined3d_surface *surface, HWND window, RECT *rect)
{
    struct wined3d_texture *texture = surface->container;
    UINT drawable_height;

    if (texture->swapchain && texture == texture->swapchain->front_buffer)
    {
        POINT offset = {0, 0};
        RECT windowsize;

        ScreenToClient(window, &offset);
        OffsetRect(rect, offset.x, offset.y);

        GetClientRect(window, &windowsize);
        drawable_height = windowsize.bottom - windowsize.top;
    }
    else
    {
        drawable_height = wined3d_texture_get_level_height(texture, surface->texture_level);
    }

    rect->top = drawable_height - rect->top;
    rect->bottom = drawable_height - rect->bottom;
}

/* Context activation is done by the caller. */
static void surface_blt_to_drawable(const struct wined3d_device *device,
        struct wined3d_context *old_ctx,
        enum wined3d_texture_filter_type filter, BOOL alpha_test,
        struct wined3d_surface *src_surface, const RECT *src_rect_in,
        struct wined3d_surface *dst_surface, const RECT *dst_rect_in)
{
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_texture *dst_texture = dst_surface->container;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context = old_ctx;
    struct wined3d_surface *restore_rt = NULL;
    RECT src_rect, dst_rect;

    src_rect = *src_rect_in;
    dst_rect = *dst_rect_in;

    restore_rt = context_get_rt_surface(old_ctx);
    if (restore_rt != dst_surface)
        context = context_acquire(device, dst_surface);
    else
        restore_rt = NULL;

    gl_info = context->gl_info;

    /* Make sure the surface is up-to-date. This should probably use
     * surface_load_location() and worry about the destination surface too,
     * unless we're overwriting it completely. */
    wined3d_texture_load(src_texture, context, FALSE);

    /* Activate the destination context, set it up for blitting */
    context_apply_blit_state(context, device);

    if (!wined3d_resource_is_offscreen(&dst_texture->resource))
        surface_translate_drawable_coords(dst_surface, context->win_handle, &dst_rect);

    device->blitter->set_shader(device->blit_priv, context, src_surface, NULL);

    if (alpha_test)
    {
        gl_info->gl_ops.gl.p_glEnable(GL_ALPHA_TEST);
        checkGLcall("glEnable(GL_ALPHA_TEST)");

        /* For P8 surfaces, the alpha component contains the palette index.
         * Which means that the colorkey is one of the palette entries. In
         * other cases pixels that should be masked away have alpha set to 0. */
        if (src_texture->resource.format->id == WINED3DFMT_P8_UINT)
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL,
                    (float)src_texture->async.src_blt_color_key.color_space_low_value / 255.0f);
        else
            gl_info->gl_ops.gl.p_glAlphaFunc(GL_NOTEQUAL, 0.0f);
        checkGLcall("glAlphaFunc");
    }
    else
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    draw_textured_quad(src_surface, context, &src_rect, &dst_rect, filter);

    if (alpha_test)
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        checkGLcall("glDisable(GL_ALPHA_TEST)");
    }

    /* Leave the opengl state valid for blitting */
    device->blitter->unset_shader(context->gl_info);

    if (wined3d_settings.strict_draw_ordering
            || (dst_texture->swapchain && dst_texture->swapchain->front_buffer == dst_texture))
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    if (restore_rt)
        context_restore(context, restore_rt);
}

HRESULT surface_color_fill(struct wined3d_surface *s, const RECT *rect, const struct wined3d_color *color)
{
    struct wined3d_resource *resource = &s->container->resource;
    struct wined3d_device *device = resource->device;
    struct wined3d_rendertarget_view_desc view_desc;
    struct wined3d_rendertarget_view *view;
    const struct blit_shader *blitter;
    HRESULT hr;

    if (!(blitter = wined3d_select_blitter(&device->adapter->gl_info, &device->adapter->d3d_info,
            WINED3D_BLIT_OP_COLOR_FILL, NULL, 0, 0, NULL, rect, resource->usage, resource->pool, resource->format)))
    {
        FIXME("No blitter is capable of performing the requested color fill operation.\n");
        return WINED3DERR_INVALIDCALL;
    }

    view_desc.format_id = resource->format->id;
    view_desc.u.texture.level_idx = s->texture_level;
    view_desc.u.texture.layer_idx = s->texture_layer;
    view_desc.u.texture.layer_count = 1;
    if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc,
            resource, NULL, &wined3d_null_parent_ops, &view)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    hr = blitter->color_fill(device, view, rect, color);
    wined3d_rendertarget_view_decref(view);

    return hr;
}

static HRESULT surface_blt_special(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_device *device = dst_texture->resource.device;
    const struct wined3d_surface *rt = wined3d_rendertarget_view_get_surface(device->fb.render_targets[0]);
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    struct wined3d_texture *src_texture;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));

    /* Get the swapchain. One of the surfaces has to be a primary surface */
    if (dst_texture->resource.pool == WINED3D_POOL_SYSTEM_MEM)
    {
        WARN("Destination is in sysmem, rejecting gl blt\n");
        return WINED3DERR_INVALIDCALL;
    }

    dst_swapchain = dst_texture->swapchain;

    if (src_surface)
    {
        src_texture = src_surface->container;
        if (src_texture->resource.pool == WINED3D_POOL_SYSTEM_MEM)
        {
            WARN("Src is in sysmem, rejecting gl blt\n");
            return WINED3DERR_INVALIDCALL;
        }

        src_swapchain = src_texture->swapchain;
    }
    else
    {
        src_texture = NULL;
        src_swapchain = NULL;
    }

    /* Early sort out of cases where no render target is used */
    if (!dst_swapchain && !src_swapchain && src_surface != rt && dst_surface != rt)
    {
        TRACE("No surface is render target, not using hardware blit.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* No destination color keying supported */
    if (flags & (WINED3D_BLT_DST_CKEY | WINED3D_BLT_DST_CKEY_OVERRIDE))
    {
        /* Can we support that with glBlendFunc if blitting to the frame buffer? */
        TRACE("Destination color key not supported in accelerated Blit, falling back to software\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && dst_swapchain == src_swapchain)
    {
        FIXME("Implement hardware blit between two surfaces on the same swapchain\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain && src_swapchain)
    {
        FIXME("Implement hardware blit between two different swapchains\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_swapchain)
    {
        /* Handled with regular texture -> swapchain blit */
        if (src_surface == rt)
            TRACE("Blit from active render target to a swapchain\n");
    }
    else if (src_swapchain && dst_surface == rt)
    {
        FIXME("Implement blit from a swapchain to the active render target\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((src_swapchain || src_surface == rt) && !dst_swapchain)
    {
        unsigned int src_width, src_height;
        /* Blit from render target to texture */
        BOOL stretchx;

        /* P8 read back is not implemented */
        if (src_texture->resource.format->id == WINED3DFMT_P8_UINT
                || dst_texture->resource.format->id == WINED3DFMT_P8_UINT)
        {
            TRACE("P8 read back not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (flags & (WINED3D_BLT_SRC_CKEY | WINED3D_BLT_SRC_CKEY_OVERRIDE))
        {
            TRACE("Color keying not supported by frame buffer to texture blit\n");
            return WINED3DERR_INVALIDCALL;
            /* Destination color key is checked above */
        }

        if (dst_rect->right - dst_rect->left != src_rect->right - src_rect->left)
            stretchx = TRUE;
        else
            stretchx = FALSE;

        /* Blt is a pretty powerful call, while glCopyTexSubImage2D is not. glCopyTexSubImage cannot
         * flip the image nor scale it.
         *
         * -> If the app asks for an unscaled, upside down copy, just perform one glCopyTexSubImage2D call
         * -> If the app wants an image width an unscaled width, copy it line per line
         * -> If the app wants an image that is scaled on the x axis, and the destination rectangle is smaller
         *    than the frame buffer, draw an upside down scaled image onto the fb, read it back and restore the
         *    back buffer. This is slower than reading line per line, thus not used for flipping
         * -> If the app wants a scaled image with a dest rect that is bigger than the fb, it has to be copied
         *    pixel by pixel. */
        src_width = wined3d_texture_get_level_width(src_texture, src_surface->texture_level);
        src_height = wined3d_texture_get_level_height(src_texture, src_surface->texture_level);
        if (!stretchx || dst_rect->right - dst_rect->left > src_width
                || dst_rect->bottom - dst_rect->top > src_height)
        {
            TRACE("No stretching in x direction, using direct framebuffer -> texture copy.\n");
            fb_copy_to_texture_direct(dst_surface, src_surface, src_rect, dst_rect, filter);
        }
        else
        {
            TRACE("Using hardware stretching to flip / stretch the texture.\n");
            fb_copy_to_texture_hwstretch(dst_surface, src_surface, src_rect, dst_rect, filter);
        }

        return WINED3D_OK;
    }

    /* Default: Fall back to the generic blt. Not an error, a TRACE is enough */
    TRACE("Didn't find any usable render target setup for hw blit, falling back to software\n");
    return WINED3DERR_INVALIDCALL;
}

/* Context activation is done by the caller. */
static void surface_depth_blt(const struct wined3d_surface *surface, struct wined3d_context *context,
        GLuint texture, GLint x, GLint y, GLsizei w, GLsizei h, GLenum target)
{
    struct wined3d_device *device = surface->container->resource.device;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLint compare_mode = GL_NONE;
    struct blt_info info;
    GLint old_binding = 0;
    RECT rect;

    gl_info->gl_ops.gl.p_glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);

    gl_info->gl_ops.gl.p_glDisable(GL_CULL_FACE);
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
    gl_info->gl_ops.gl.p_glEnable(GL_DEPTH_TEST);
    gl_info->gl_ops.gl.p_glDepthFunc(GL_ALWAYS);
    gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
    gl_info->gl_ops.gl.p_glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    gl_info->gl_ops.gl.p_glViewport(x, y, w, h);
    gl_info->gl_ops.gl.p_glDepthRange(0.0, 1.0);

    SetRect(&rect, 0, h, w, 0);
    surface_get_blt_info(target, &rect,
            wined3d_texture_get_level_pow2_width(surface->container, surface->texture_level),
            wined3d_texture_get_level_pow2_height(surface->container, surface->texture_level), &info);
    context_active_texture(context, context->gl_info, 0);
    gl_info->gl_ops.gl.p_glGetIntegerv(info.binding, &old_binding);
    gl_info->gl_ops.gl.p_glBindTexture(info.bind_target, texture);
    if (gl_info->supported[ARB_SHADOW])
    {
        gl_info->gl_ops.gl.p_glGetTexParameteriv(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, &compare_mode);
        if (compare_mode != GL_NONE)
            gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
    }

    device->shader_backend->shader_select_depth_blt(device->shader_priv,
            gl_info, info.tex_type, &surface->ds_current_size);

    gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[0]);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[1]);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, -1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[2]);
    gl_info->gl_ops.gl.p_glVertex2f(-1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glTexCoord3fv(info.coords[3]);
    gl_info->gl_ops.gl.p_glVertex2f(1.0f, 1.0f);
    gl_info->gl_ops.gl.p_glEnd();

    if (compare_mode != GL_NONE)
        gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_COMPARE_MODE_ARB, compare_mode);
    gl_info->gl_ops.gl.p_glBindTexture(info.bind_target, old_binding);

    gl_info->gl_ops.gl.p_glPopAttrib();

    device->shader_backend->shader_deselect_depth_blt(device->shader_priv, gl_info);
}

void surface_modify_ds_location(struct wined3d_surface *surface,
        DWORD location, UINT w, UINT h)
{
    struct wined3d_texture *texture = surface->container;
    unsigned int sub_resource_idx;

    TRACE("surface %p, new location %#x, w %u, h %u.\n", surface, location, w, h);

    sub_resource_idx = surface_get_sub_resource_idx(surface);
    surface->ds_current_size.cx = w;
    surface->ds_current_size.cy = h;
    wined3d_texture_validate_location(texture, sub_resource_idx, location);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~location);
}

/* Context activation is done by the caller. */
static void surface_load_ds_location(struct wined3d_surface *surface, struct wined3d_context *context, DWORD location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLsizei w, h;

    TRACE("surface %p, context %p, new location %#x.\n", surface, context, location);

    /* TODO: Make this work for modes other than FBO */
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return;

    if (!(texture->sub_resources[sub_resource_idx].locations & location))
    {
        w = surface->ds_current_size.cx;
        h = surface->ds_current_size.cy;
        surface->ds_current_size.cx = 0;
        surface->ds_current_size.cy = 0;
    }
    else
    {
        w = wined3d_texture_get_level_width(texture, surface->texture_level);
        h = wined3d_texture_get_level_height(texture, surface->texture_level);
    }

    if (surface->current_renderbuffer)
    {
        FIXME("Not supported with fixed up depth stencil.\n");
        return;
    }

    wined3d_texture_prepare_location(texture, sub_resource_idx, context, location);

    if (location == WINED3D_LOCATION_TEXTURE_RGB)
    {
        GLint old_binding = 0;
        GLenum bind_target;

        /* The render target is allowed to be smaller than the depth/stencil
         * buffer, so the onscreen depth/stencil buffer is potentially smaller
         * than the offscreen surface. Don't overwrite the offscreen surface
         * with undefined data. */
        w = min(w, context->swapchain->desc.backbuffer_width);
        h = min(h, context->swapchain->desc.backbuffer_height);

        TRACE("Copying onscreen depth buffer to depth texture.\n");

        if (!device->depth_blt_texture)
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->depth_blt_texture);

        /* Note that we use depth_blt here as well, rather than glCopyTexImage2D
         * directly on the FBO texture. That's because we need to flip. */
        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                context->swapchain->front_buffer->sub_resources[0].u.surface,
                NULL, WINED3D_LOCATION_DRAWABLE);
        if (surface->texture_target == GL_TEXTURE_RECTANGLE_ARB)
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &old_binding);
            bind_target = GL_TEXTURE_RECTANGLE_ARB;
        }
        else
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
            bind_target = GL_TEXTURE_2D;
        }
        gl_info->gl_ops.gl.p_glBindTexture(bind_target, device->depth_blt_texture);
        /* We use GL_DEPTH_COMPONENT instead of the surface's specific
         * internal format, because the internal format might include stencil
         * data. In principle we should copy stencil data as well, but unless
         * the driver supports stencil export it's hard to do, and doesn't
         * seem to be needed in practice. If the hardware doesn't support
         * writing stencil data, the glCopyTexImage2D() call might trigger
         * software fallbacks. */
        gl_info->gl_ops.gl.p_glCopyTexImage2D(bind_target, 0, GL_DEPTH_COMPONENT, 0, 0, w, h, 0);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(bind_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glBindTexture(bind_target, old_binding);

        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                NULL, surface, WINED3D_LOCATION_TEXTURE_RGB);
        context_set_draw_buffer(context, GL_NONE);

        /* Do the actual blit */
        surface_depth_blt(surface, context, device->depth_blt_texture, 0, 0, w, h, bind_target);
        checkGLcall("depth_blt");

        context_invalidate_state(context, STATE_FRAMEBUFFER);

        if (wined3d_settings.strict_draw_ordering)
            gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */
    }
    else if (location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Copying depth texture to onscreen depth buffer.\n");

        context_apply_fbo_state_blit(context, GL_FRAMEBUFFER,
                context->swapchain->front_buffer->sub_resources[0].u.surface,
                NULL, WINED3D_LOCATION_DRAWABLE);
        surface_depth_blt(surface, context, texture->texture_rgb.name, 0,
                wined3d_texture_get_level_pow2_height(texture, surface->texture_level) - h,
                w, h, surface->texture_target);
        checkGLcall("depth_blt");

        context_invalidate_state(context, STATE_FRAMEBUFFER);

        if (wined3d_settings.strict_draw_ordering)
            gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */
    }
    else
    {
        ERR("Invalid location (%#x) specified.\n", location);
    }
}

static DWORD resource_access_from_location(DWORD location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_BUFFER:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_LOCATION_DRAWABLE:
        case WINED3D_LOCATION_TEXTURE_SRGB:
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
        case WINED3D_LOCATION_RB_RESOLVED:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

static void surface_copy_simple_location(struct wined3d_surface *surface, DWORD location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_context *context;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_address dst, src;

    sub_resource = &texture->sub_resources[sub_resource_idx];
    wined3d_texture_get_memory(texture, sub_resource_idx, &dst, location);
    wined3d_texture_get_memory(texture, sub_resource_idx, &src, sub_resource->locations);

    if (dst.buffer_object)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, dst.buffer_object));
        GL_EXTCALL(glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, sub_resource->size, src.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("Upload PBO");
        context_release(context);
        return;
    }
    if (src.buffer_object)
    {
        context = context_acquire(device, NULL);
        gl_info = context->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, src.buffer_object));
        GL_EXTCALL(glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, sub_resource->size, dst.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("Download PBO");
        context_release(context);
        return;
    }
    memcpy(dst.addr, src.addr, sub_resource->size);
}

/* Context activation is done by the caller. */
static void surface_load_sysmem(struct wined3d_surface *surface,
        struct wined3d_context *context, DWORD dst_location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture *texture = surface->container;
    struct wined3d_texture_sub_resource *sub_resource;

    wined3d_texture_prepare_location(texture, sub_resource_idx, context, dst_location);

    sub_resource = &texture->sub_resources[sub_resource_idx];
    if (sub_resource->locations & surface_simple_locations)
    {
        surface_copy_simple_location(surface, dst_location);
        return;
    }

    if (sub_resource->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED))
        wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);

    /* Download the surface to system memory. */
    if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
    {
        wined3d_texture_bind_and_dirtify(texture, context,
                !(sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB));
        surface_download_data(surface, gl_info, dst_location);
        ++texture->download_count;

        return;
    }

    if (sub_resource->locations & WINED3D_LOCATION_DRAWABLE)
    {
        read_from_framebuffer(surface, context, dst_location);
        return;
    }

    FIXME("Can't load surface %p with location flags %s into sysmem.\n",
            surface, wined3d_debug_location(sub_resource->locations));
}

/* Context activation is done by the caller. */
static HRESULT surface_load_drawable(struct wined3d_surface *surface,
        struct wined3d_context *context)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    RECT r;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && wined3d_resource_is_offscreen(&texture->resource))
    {
        ERR("Trying to load offscreen surface into WINED3D_LOCATION_DRAWABLE.\n");
        return WINED3DERR_INVALIDCALL;
    }

    surface_get_rect(surface, NULL, &r);
    wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    surface_blt_to_drawable(texture->resource.device, context,
            WINED3D_TEXF_POINT, FALSE, surface, &r, surface, &r);

    return WINED3D_OK;
}

static HRESULT surface_load_texture(struct wined3d_surface *surface,
        struct wined3d_context *context, BOOL srgb)
{
    unsigned int width, height, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch;
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture *texture = surface->container;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_color_key_conversion *conversion;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_bo_address data;
    BYTE *src_mem, *dst_mem = NULL;
    struct wined3d_format format;
    POINT dst_point = {0, 0};
    RECT src_rect;

    sub_resource = surface_get_sub_resource(surface);
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
            && wined3d_resource_is_offscreen(&texture->resource)
            && (sub_resource->locations & WINED3D_LOCATION_DRAWABLE))
    {
        surface_load_fb_texture(surface, srgb, context);

        return WINED3D_OK;
    }

    width = wined3d_texture_get_level_width(texture, surface->texture_level);
    height = wined3d_texture_get_level_height(texture, surface->texture_level);
    SetRect(&src_rect, 0, 0, width, height);

    if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | WINED3D_LOCATION_TEXTURE_RGB)
            && (texture->resource.format_flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB)
            && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                NULL, texture->resource.usage, texture->resource.pool, texture->resource.format,
                NULL, texture->resource.usage, texture->resource.pool, texture->resource.format))
    {
        if (srgb)
            surface_blt_fbo(device, context, WINED3D_TEXF_POINT, surface, WINED3D_LOCATION_TEXTURE_RGB,
                    &src_rect, surface, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect);
        else
            surface_blt_fbo(device, context, WINED3D_TEXF_POINT, surface, WINED3D_LOCATION_TEXTURE_SRGB,
                    &src_rect, surface, WINED3D_LOCATION_TEXTURE_RGB, &src_rect);

        return WINED3D_OK;
    }

    if (sub_resource->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED)
            && (!srgb || (texture->resource.format_flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB))
            && fbo_blit_supported(gl_info, WINED3D_BLIT_OP_COLOR_BLIT,
                NULL, texture->resource.usage, texture->resource.pool, texture->resource.format,
                NULL, texture->resource.usage, texture->resource.pool, texture->resource.format))
    {
        DWORD src_location = sub_resource->locations & WINED3D_LOCATION_RB_RESOLVED ?
                WINED3D_LOCATION_RB_RESOLVED : WINED3D_LOCATION_RB_MULTISAMPLE;
        DWORD dst_location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;

        surface_blt_fbo(device, context, WINED3D_TEXF_POINT, surface, src_location,
                &src_rect, surface, dst_location, &src_rect);

        return WINED3D_OK;
    }

    /* Upload from system memory */

    if (srgb)
    {
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | texture->resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_RGB)
        {
            FIXME_(d3d_perf)("Downloading RGB surface %p to reload it as sRGB.\n", surface);
            wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding);
        }
    }
    else
    {
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | texture->resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_SRGB)
        {
            FIXME_(d3d_perf)("Downloading sRGB surface %p to reload it as RGB.\n", surface);
            wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding);
        }
    }

    if (!(sub_resource->locations & surface_simple_locations))
    {
        WARN("Trying to load a texture from sysmem, but no simple location is valid.\n");
        /* Lets hope we get it from somewhere... */
        wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_SYSMEM);
    }

    wined3d_texture_prepare_texture(texture, context, srgb);
    wined3d_texture_bind_and_dirtify(texture, context, srgb);
    wined3d_texture_get_pitch(texture, surface->texture_level, &src_row_pitch, &src_slice_pitch);

    format = *texture->resource.format;
    if ((conversion = wined3d_format_get_color_key_conversion(texture, TRUE)))
        format = *wined3d_get_format(gl_info, conversion->dst_format);

    /* Don't use PBOs for converted surfaces. During PBO conversion we look at
     * WINED3D_TEXTURE_CONVERTED but it isn't set (yet) in all cases it is
     * getting called. */
    if ((format.convert || conversion) && texture->sub_resources[sub_resource_idx].buffer_object)
    {
        TRACE("Removing the pbo attached to surface %p.\n", surface);

        wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_SYSMEM);
        wined3d_texture_set_map_binding(texture, WINED3D_LOCATION_SYSMEM);
    }

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, sub_resource->locations);
    if (format.convert)
    {
        /* This code is entered for texture formats which need a fixup. */
        format.byte_count = format.conv_byte_count;
        wined3d_format_calculate_pitch(&format, 1, width, height, &dst_row_pitch, &dst_slice_pitch);

        src_mem = wined3d_texture_map_bo_address(&data, src_slice_pitch,
                gl_info, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READONLY);
        if (!(dst_mem = HeapAlloc(GetProcessHeap(), 0, dst_slice_pitch)))
        {
            ERR("Out of memory (%u).\n", dst_slice_pitch);
            context_release(context);
            return E_OUTOFMEMORY;
        }
        format.convert(src_mem, dst_mem, src_row_pitch, src_slice_pitch,
                dst_row_pitch, dst_slice_pitch, width, height, 1);
        src_row_pitch = dst_row_pitch;
        wined3d_texture_unmap_bo_address(&data, gl_info, GL_PIXEL_UNPACK_BUFFER);

        data.buffer_object = 0;
        data.addr = dst_mem;
    }
    else if (conversion)
    {
        /* This code is only entered for color keying fixups */
        struct wined3d_palette *palette = NULL;

        wined3d_format_calculate_pitch(&format, device->surface_alignment,
                width, height, &dst_row_pitch, &dst_slice_pitch);

        src_mem = wined3d_texture_map_bo_address(&data, src_slice_pitch,
                gl_info, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READONLY);
        if (!(dst_mem = HeapAlloc(GetProcessHeap(), 0, dst_slice_pitch)))
        {
            ERR("Out of memory (%u).\n", dst_slice_pitch);
            context_release(context);
            return E_OUTOFMEMORY;
        }
        if (texture->swapchain && texture->swapchain->palette)
            palette = texture->swapchain->palette;
        conversion->convert(src_mem, src_row_pitch, dst_mem, dst_row_pitch,
                width, height, palette, &texture->async.gl_color_key);
        src_row_pitch = dst_row_pitch;
        wined3d_texture_unmap_bo_address(&data, gl_info, GL_PIXEL_UNPACK_BUFFER);

        data.buffer_object = 0;
        data.addr = dst_mem;
    }

    wined3d_surface_upload_data(surface, gl_info, &format, &src_rect,
            src_row_pitch, &dst_point, srgb, wined3d_const_bo_address(&data));

    HeapFree(GetProcessHeap(), 0, dst_mem);

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void surface_load_renderbuffer(struct wined3d_surface *surface, struct wined3d_context *context,
        DWORD dst_location)
{
    struct wined3d_texture *texture = surface->container;
    const RECT rect = {0, 0,
            wined3d_texture_get_level_width(texture, surface->texture_level),
            wined3d_texture_get_level_height(texture, surface->texture_level)};
    DWORD locations = surface_get_sub_resource(surface)->locations;
    DWORD src_location;

    if (locations & WINED3D_LOCATION_RB_MULTISAMPLE)
        src_location = WINED3D_LOCATION_RB_MULTISAMPLE;
    else if (locations & WINED3D_LOCATION_RB_RESOLVED)
        src_location = WINED3D_LOCATION_RB_RESOLVED;
    else if (locations & WINED3D_LOCATION_TEXTURE_SRGB)
        src_location = WINED3D_LOCATION_TEXTURE_SRGB;
    else /* surface_blt_fbo will load the source location if necessary. */
        src_location = WINED3D_LOCATION_TEXTURE_RGB;

    surface_blt_fbo(texture->resource.device, context, WINED3D_TEXF_POINT,
            surface, src_location, &rect, surface, dst_location, &rect);
}

/* Context activation is done by the caller. Context may be NULL in ddraw-only mode. */
HRESULT surface_load_location(struct wined3d_surface *surface, struct wined3d_context *context, DWORD location)
{
    unsigned int sub_resource_idx = surface_get_sub_resource_idx(surface);
    struct wined3d_texture *texture = surface->container;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int surface_w, surface_h;
    HRESULT hr;

    TRACE("surface %p, location %s.\n", surface, wined3d_debug_location(location));

    surface_w = wined3d_texture_get_level_width(texture, surface->texture_level);
    surface_h = wined3d_texture_get_level_height(texture, surface->texture_level);

    sub_resource = &texture->sub_resources[sub_resource_idx];
    if (sub_resource->locations & location && (!(texture->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
            || (surface->ds_current_size.cx == surface_w && surface->ds_current_size.cy == surface_h)))
    {
        TRACE("Location (%#x) is already up to date.\n", location);
        return WINED3D_OK;
    }

    if (WARN_ON(d3d))
    {
        DWORD required_access = resource_access_from_location(location);
        if ((texture->resource.access_flags & required_access) != required_access)
            WARN("Operation requires %#x access, but surface only has %#x.\n",
                    required_access, texture->resource.access_flags);
    }

    if (sub_resource->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Surface previously discarded, nothing to do.\n");
        wined3d_texture_prepare_location(texture, sub_resource_idx, context, location);
        wined3d_texture_validate_location(texture, sub_resource_idx, location);
        wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        goto done;
    }

    if (!sub_resource->locations)
    {
        ERR("Surface %p does not have any up to date location.\n", surface);
        wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        return surface_load_location(surface, context, location);
    }

    if (texture->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
    {
        if ((location == WINED3D_LOCATION_TEXTURE_RGB && sub_resource->locations & WINED3D_LOCATION_DRAWABLE)
                || (location == WINED3D_LOCATION_DRAWABLE && sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB))
        {
            surface_load_ds_location(surface, context, location);
            goto done;
        }

        FIXME("Unimplemented copy from %s to %s for depth/stencil buffers.\n",
                wined3d_debug_location(sub_resource->locations), wined3d_debug_location(location));
        return WINED3DERR_INVALIDCALL;
    }

    switch (location)
    {
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_BUFFER:
            surface_load_sysmem(surface, context, location);
            break;

        case WINED3D_LOCATION_DRAWABLE:
            if (FAILED(hr = surface_load_drawable(surface, context)))
                return hr;
            break;

        case WINED3D_LOCATION_RB_RESOLVED:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
            surface_load_renderbuffer(surface, context, location);
            break;

        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (FAILED(hr = surface_load_texture(surface, context,
                    location == WINED3D_LOCATION_TEXTURE_SRGB)))
                return hr;
            break;

        default:
            ERR("Don't know how to handle location %#x.\n", location);
            break;
    }

done:
    wined3d_texture_validate_location(texture, sub_resource_idx, location);

    if (texture->resource.usage & WINED3DUSAGE_DEPTHSTENCIL)
    {
        surface->ds_current_size.cx = surface_w;
        surface->ds_current_size.cy = surface_h;
    }

    return WINED3D_OK;
}

static HRESULT ffp_blit_alloc(struct wined3d_device *device) { return WINED3D_OK; }
/* Context activation is done by the caller. */
static void ffp_blit_free(struct wined3d_device *device) { }

/* Context activation is done by the caller. */
static HRESULT ffp_blit_set(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface,
        const struct wined3d_color_key *color_key)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    gl_info->gl_ops.gl.p_glEnable(surface->container->target);
    checkGLcall("glEnable(target)");

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void ffp_blit_unset(const struct wined3d_gl_info *gl_info)
{
    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
}

static BOOL ffp_blit_supported(const struct wined3d_gl_info *gl_info,
        const struct wined3d_d3d_info *d3d_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    if (src_pool == WINED3D_POOL_SYSTEM_MEM || dst_pool == WINED3D_POOL_SYSTEM_MEM)
    {
        TRACE("Source or destination is in system memory.\n");
        return FALSE;
    }

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT_CKEY:
            if (d3d_info->shader_color_key)
            {
                TRACE("Color keying requires converted textures.\n");
                return FALSE;
            }
        case WINED3D_BLIT_OP_COLOR_BLIT:
        case WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST:
            if (TRACE_ON(d3d))
            {
                TRACE("Checking support for fixup:\n");
                dump_color_fixup_desc(src_format->color_fixup);
            }

            /* We only support identity conversions. */
            if (!is_identity_fixup(src_format->color_fixup)
                    || !is_identity_fixup(dst_format->color_fixup))
            {
                TRACE("Fixups are not supported.\n");
                return FALSE;
            }

            if (!(dst_usage & WINED3DUSAGE_RENDERTARGET))
            {
                TRACE("Can only blit to render targets.\n");
                return FALSE;
            }
            return TRUE;

        case WINED3D_BLIT_OP_COLOR_FILL:
            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
            {
                if (!((dst_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FBO_ATTACHABLE)
                        || (dst_usage & WINED3DUSAGE_RENDERTARGET)))
                    return FALSE;
            }
            else if (!(dst_usage & WINED3DUSAGE_RENDERTARGET))
            {
                TRACE("Color fill not supported\n");
                return FALSE;
            }

            /* FIXME: We should reject color fills on formats with fixups,
             * but this would break P8 color fills for example. */

            return TRUE;

        case WINED3D_BLIT_OP_DEPTH_FILL:
            return TRUE;

        default:
            TRACE("Unsupported blit_op=%d\n", blit_op);
            return FALSE;
    }
}

static HRESULT ffp_blit_color_fill(struct wined3d_device *device, struct wined3d_rendertarget_view *view,
        const RECT *rect, const struct wined3d_color *color)
{
    const RECT draw_rect = {0, 0, view->width, view->height};
    struct wined3d_fb_state fb = {&view, NULL};

    device_clear_render_targets(device, 1, &fb, 1, rect, &draw_rect, WINED3DCLEAR_TARGET, color, 0.0f, 0);

    return WINED3D_OK;
}

static HRESULT ffp_blit_depth_fill(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view, const RECT *rect, DWORD clear_flags,
        float depth, DWORD stencil)
{
    const RECT draw_rect = {0, 0, view->width, view->height};
    struct wined3d_fb_state fb = {NULL, view};

    device_clear_render_targets(device, 0, &fb, 1, rect, &draw_rect, clear_flags, NULL, depth, stencil);

    return WINED3D_OK;
}

static void ffp_blit_blit_surface(struct wined3d_device *device, enum wined3d_blit_op op, DWORD filter,
        struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect,
        const struct wined3d_color_key *color_key)
{
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_texture *src_texture = src_surface->container;
    struct wined3d_context *context;

    /* Blit from offscreen surface to render target */
    struct wined3d_color_key old_blt_key = src_texture->async.src_blt_color_key;
    DWORD old_color_key_flags = src_texture->async.color_key_flags;

    TRACE("Blt from surface %p to rendertarget %p\n", src_surface, dst_surface);

    wined3d_texture_set_color_key(src_texture, WINED3D_CKEY_SRC_BLT, color_key);

    context = context_acquire(device, dst_surface);

    if (op == WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST)
        glEnable(GL_ALPHA_TEST);

    surface_blt_to_drawable(device, context, filter,
            !!color_key, src_surface, src_rect, dst_surface, dst_rect);

    if (op == WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST)
        glDisable(GL_ALPHA_TEST);

    context_release(context);

    /* Restore the color key parameters */
    wined3d_texture_set_color_key(src_texture, WINED3D_CKEY_SRC_BLT,
            (old_color_key_flags & WINED3D_CKEY_SRC_BLT) ? &old_blt_key : NULL);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, dst_texture->resource.draw_binding);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~dst_texture->resource.draw_binding);
}

const struct blit_shader ffp_blit =  {
    ffp_blit_alloc,
    ffp_blit_free,
    ffp_blit_set,
    ffp_blit_unset,
    ffp_blit_supported,
    ffp_blit_color_fill,
    ffp_blit_depth_fill,
    ffp_blit_blit_surface,
};

static HRESULT cpu_blit_alloc(struct wined3d_device *device)
{
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void cpu_blit_free(struct wined3d_device *device)
{
}

/* Context activation is done by the caller. */
static HRESULT cpu_blit_set(void *blit_priv, struct wined3d_context *context, const struct wined3d_surface *surface,
        const struct wined3d_color_key *color_key)
{
    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void cpu_blit_unset(const struct wined3d_gl_info *gl_info)
{
}

static BOOL cpu_blit_supported(const struct wined3d_gl_info *gl_info,
        const struct wined3d_d3d_info *d3d_info, enum wined3d_blit_op blit_op,
        const RECT *src_rect, DWORD src_usage, enum wined3d_pool src_pool, const struct wined3d_format *src_format,
        const RECT *dst_rect, DWORD dst_usage, enum wined3d_pool dst_pool, const struct wined3d_format *dst_format)
{
    if (blit_op == WINED3D_BLIT_OP_COLOR_FILL)
    {
        return TRUE;
    }

    return FALSE;
}

static HRESULT surface_cpu_blt_compressed(const BYTE *src_data, BYTE *dst_data,
        UINT src_pitch, UINT dst_pitch, UINT update_w, UINT update_h,
        const struct wined3d_format *format, DWORD flags, const struct wined3d_blt_fx *fx)
{
    UINT row_block_count;
    const BYTE *src_row;
    BYTE *dst_row;
    UINT x, y;

    src_row = src_data;
    dst_row = dst_data;

    row_block_count = (update_w + format->block_width - 1) / format->block_width;

    if (!flags)
    {
        for (y = 0; y < update_h; y += format->block_height)
        {
            memcpy(dst_row, src_row, row_block_count * format->block_byte_count);
            src_row += src_pitch;
            dst_row += dst_pitch;
        }

        return WINED3D_OK;
    }

    if (flags == WINED3D_BLT_FX && fx->fx == WINEDDBLTFX_MIRRORUPDOWN)
    {
        src_row += (((update_h / format->block_height) - 1) * src_pitch);

        switch (format->id)
        {
            case WINED3DFMT_DXT1:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            case WINED3DFMT_DXT2:
            case WINED3DFMT_DXT3:
                for (y = 0; y < update_h; y += format->block_height)
                {
                    struct block
                    {
                        WORD alpha_row[4];
                        WORD color[2];
                        BYTE control_row[4];
                    };

                    const struct block *s = (const struct block *)src_row;
                    struct block *d = (struct block *)dst_row;

                    for (x = 0; x < row_block_count; ++x)
                    {
                        d[x].alpha_row[0] = s[x].alpha_row[3];
                        d[x].alpha_row[1] = s[x].alpha_row[2];
                        d[x].alpha_row[2] = s[x].alpha_row[1];
                        d[x].alpha_row[3] = s[x].alpha_row[0];
                        d[x].color[0] = s[x].color[0];
                        d[x].color[1] = s[x].color[1];
                        d[x].control_row[0] = s[x].control_row[3];
                        d[x].control_row[1] = s[x].control_row[2];
                        d[x].control_row[2] = s[x].control_row[1];
                        d[x].control_row[3] = s[x].control_row[0];
                    }
                    src_row -= src_pitch;
                    dst_row += dst_pitch;
                }
                return WINED3D_OK;

            default:
                FIXME("Compressed flip not implemented for format %s.\n",
                        debug_d3dformat(format->id));
                return E_NOTIMPL;
        }
    }

    FIXME("Unsupported blit on compressed surface (format %s, flags %#x, DDFX %#x).\n",
            debug_d3dformat(format->id), flags, flags & WINED3D_BLT_FX ? fx->fx : 0);

    return E_NOTIMPL;
}

static HRESULT surface_cpu_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const struct wined3d_box *dst_box, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, DWORD flags, const struct wined3d_blt_fx *fx,
        enum wined3d_texture_filter_type filter)
{
    unsigned int bpp, src_height, src_width, dst_height, dst_width, row_byte_count;
    const struct wined3d_format *src_format, *dst_format;
    struct wined3d_texture *converted_texture = NULL;
    unsigned int src_fmt_flags, dst_fmt_flags;
    struct wined3d_map_desc dst_map, src_map;
    const BYTE *sbase = NULL;
    HRESULT hr = WINED3D_OK;
    BOOL same_sub_resource;
    const BYTE *sbuf;
    BYTE *dbuf;
    int x, y;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_box %s, src_texture %p, "
            "src_sub_resource_idx %u, src_box %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, debug_box(dst_box), src_texture,
            src_sub_resource_idx, debug_box(src_box), flags, fx, debug_d3dtexturefiltertype(filter));

    if (src_texture == dst_texture && src_sub_resource_idx == dst_sub_resource_idx)
    {
        same_sub_resource = TRUE;
        wined3d_resource_map(&dst_texture->resource, dst_sub_resource_idx, &dst_map, NULL, 0);
        src_map = dst_map;
        src_format = dst_texture->resource.format;
        dst_format = src_format;
        dst_fmt_flags = dst_texture->resource.format_flags;
        src_fmt_flags = dst_fmt_flags;
    }
    else
    {
        same_sub_resource = FALSE;
        dst_format = dst_texture->resource.format;
        dst_fmt_flags = dst_texture->resource.format_flags;
        if (src_texture)
        {
            if (dst_texture->resource.format->id != src_texture->resource.format->id)
            {
                if (!(converted_texture = surface_convert_format(src_texture, src_sub_resource_idx, dst_format)))
                {
                    FIXME("Cannot convert %s to %s.\n", debug_d3dformat(src_texture->resource.format->id),
                            debug_d3dformat(dst_texture->resource.format->id));
                    return WINED3DERR_NOTAVAILABLE;
                }
                src_texture = converted_texture;
                src_sub_resource_idx = 0;
            }
            wined3d_resource_map(&src_texture->resource, src_sub_resource_idx, &src_map, NULL, WINED3D_MAP_READONLY);
            src_format = src_texture->resource.format;
            src_fmt_flags = src_texture->resource.format_flags;
        }
        else
        {
            src_format = dst_format;
            src_fmt_flags = dst_fmt_flags;
        }

        wined3d_resource_map(&dst_texture->resource, dst_sub_resource_idx, &dst_map, dst_box, 0);
    }

    bpp = dst_format->byte_count;
    src_height = src_box->bottom - src_box->top;
    src_width = src_box->right - src_box->left;
    dst_height = dst_box->bottom - dst_box->top;
    dst_width = dst_box->right - dst_box->left;
    row_byte_count = dst_width * bpp;

    if (src_texture)
        sbase = (BYTE *)src_map.data
                + ((src_box->top / src_format->block_height) * src_map.row_pitch)
                + ((src_box->left / src_format->block_width) * src_format->block_byte_count);
    if (same_sub_resource)
        dbuf = (BYTE *)dst_map.data
                + ((dst_box->top / dst_format->block_height) * dst_map.row_pitch)
                + ((dst_box->left / dst_format->block_width) * dst_format->block_byte_count);
    else
        dbuf = dst_map.data;

    if (src_fmt_flags & dst_fmt_flags & WINED3DFMT_FLAG_BLOCKS)
    {
        TRACE("%s -> %s copy.\n", debug_d3dformat(src_format->id), debug_d3dformat(dst_format->id));

        if (same_sub_resource)
        {
            FIXME("Only plain blits supported on compressed surfaces.\n");
            hr = E_NOTIMPL;
            goto release;
        }

        if (src_height != dst_height || src_width != dst_width)
        {
            WARN("Stretching not supported on compressed surfaces.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        if (!wined3d_texture_check_block_align(src_texture,
                src_sub_resource_idx % src_texture->level_count, src_box))
        {
            WARN("Source rectangle not block-aligned.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        if (!wined3d_texture_check_block_align(dst_texture,
                dst_sub_resource_idx % dst_texture->level_count, dst_box))
        {
            WARN("Destination rectangle not block-aligned.\n");
            hr = WINED3DERR_INVALIDCALL;
            goto release;
        }

        hr = surface_cpu_blt_compressed(sbase, dbuf,
                src_map.row_pitch, dst_map.row_pitch, dst_width, dst_height,
                src_format, flags, fx);
        goto release;
    }

    /* First, all the 'source-less' blits */
    if (flags & WINED3D_BLT_COLOR_FILL)
    {
        hr = _Blt_ColorFill(dbuf, dst_width, dst_height, bpp, dst_map.row_pitch, fx->fill_color);
        flags &= ~WINED3D_BLT_COLOR_FILL;
    }

    if (flags & WINED3D_BLT_DEPTH_FILL)
        FIXME("WINED3D_BLT_DEPTH_FILL needs to be implemented!\n");

    /* Now the 'with source' blits. */
    if (src_texture)
    {
        int sx, xinc, sy, yinc;

        if (!dst_width || !dst_height) /* Hmm... stupid program? */
            goto release;

        if (filter != WINED3D_TEXF_NONE && filter != WINED3D_TEXF_POINT
                && (src_width != dst_width || src_height != dst_height))
        {
            /* Can happen when d3d9 apps do a StretchRect() call which isn't handled in GL. */
            FIXME("Filter %s not supported in software blit.\n", debug_d3dtexturefiltertype(filter));
        }

        xinc = (src_width << 16) / dst_width;
        yinc = (src_height << 16) / dst_height;

        if (!flags)
        {
            /* No effects, we can cheat here. */
            if (dst_width == src_width)
            {
                if (dst_height == src_height)
                {
                    /* No stretching in either direction. This needs to be as
                     * fast as possible. */
                    sbuf = sbase;

                    /* Check for overlapping surfaces. */
                    if (!same_sub_resource || dst_box->top < src_box->top
                            || dst_box->right <= src_box->left || src_box->right <= dst_box->left)
                    {
                        /* No overlap, or dst above src, so copy from top downwards. */
                        for (y = 0; y < dst_height; ++y)
                        {
                            memcpy(dbuf, sbuf, row_byte_count);
                            sbuf += src_map.row_pitch;
                            dbuf += dst_map.row_pitch;
                        }
                    }
                    else if (dst_box->top > src_box->top)
                    {
                        /* Copy from bottom upwards. */
                        sbuf += src_map.row_pitch * dst_height;
                        dbuf += dst_map.row_pitch * dst_height;
                        for (y = 0; y < dst_height; ++y)
                        {
                            sbuf -= src_map.row_pitch;
                            dbuf -= dst_map.row_pitch;
                            memcpy(dbuf, sbuf, row_byte_count);
                        }
                    }
                    else
                    {
                        /* Src and dst overlapping on the same line, use memmove. */
                        for (y = 0; y < dst_height; ++y)
                        {
                            memmove(dbuf, sbuf, row_byte_count);
                            sbuf += src_map.row_pitch;
                            dbuf += dst_map.row_pitch;
                        }
                    }
                }
                else
                {
                    /* Stretching in y direction only. */
                    for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                        memcpy(dbuf, sbuf, row_byte_count);
                        dbuf += dst_map.row_pitch;
                    }
                }
            }
            else
            {
                /* Stretching in X direction. */
                int last_sy = -1;
                for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                {
                    sbuf = sbase + (sy >> 16) * src_map.row_pitch;

                    if ((sy >> 16) == (last_sy >> 16))
                    {
                        /* This source row is the same as last source row -
                         * Copy the already stretched row. */
                        memcpy(dbuf, dbuf - dst_map.row_pitch, row_byte_count);
                    }
                    else
                    {
#define STRETCH_ROW(type) \
do { \
    const type *s = (const type *)sbuf; \
    type *d = (type *)dbuf; \
    for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        d[x] = s[sx >> 16]; \
} while(0)

                        switch(bpp)
                        {
                            case 1:
                                STRETCH_ROW(BYTE);
                                break;
                            case 2:
                                STRETCH_ROW(WORD);
                                break;
                            case 4:
                                STRETCH_ROW(DWORD);
                                break;
                            case 3:
                            {
                                const BYTE *s;
                                BYTE *d = dbuf;
                                for (x = sx = 0; x < dst_width; x++, sx+= xinc)
                                {
                                    DWORD pixel;

                                    s = sbuf + 3 * (sx >> 16);
                                    pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                                    d[0] = (pixel      ) & 0xff;
                                    d[1] = (pixel >>  8) & 0xff;
                                    d[2] = (pixel >> 16) & 0xff;
                                    d += 3;
                                }
                                break;
                            }
                            default:
                                FIXME("Stretched blit not implemented for bpp %u!\n", bpp * 8);
                                hr = WINED3DERR_NOTAVAILABLE;
                                goto error;
                        }
#undef STRETCH_ROW
                    }
                    dbuf += dst_map.row_pitch;
                    last_sy = sy;
                }
            }
        }
        else
        {
            LONG dstyinc = dst_map.row_pitch, dstxinc = bpp;
            DWORD keylow = 0xffffffff, keyhigh = 0, keymask = 0xffffffff;
            DWORD destkeylow = 0x0, destkeyhigh = 0xffffffff, destkeymask = 0xffffffff;
            if (flags & (WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                    | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE))
            {
                /* The color keying flags are checked for correctness in ddraw */
                if (flags & WINED3D_BLT_SRC_CKEY)
                {
                    keylow  = src_texture->async.src_blt_color_key.color_space_low_value;
                    keyhigh = src_texture->async.src_blt_color_key.color_space_high_value;
                }
                else if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
                {
                    keylow = fx->src_color_key.color_space_low_value;
                    keyhigh = fx->src_color_key.color_space_high_value;
                }

                if (flags & WINED3D_BLT_DST_CKEY)
                {
                    /* Destination color keys are taken from the source surface! */
                    destkeylow = src_texture->async.dst_blt_color_key.color_space_low_value;
                    destkeyhigh = src_texture->async.dst_blt_color_key.color_space_high_value;
                }
                else if (flags & WINED3D_BLT_DST_CKEY_OVERRIDE)
                {
                    destkeylow = fx->dst_color_key.color_space_low_value;
                    destkeyhigh = fx->dst_color_key.color_space_high_value;
                }

                if (bpp == 1)
                {
                    keymask = 0xff;
                }
                else
                {
                    DWORD masks[3];
                    get_color_masks(src_format, masks);
                    keymask = masks[0]
                            | masks[1]
                            | masks[2];
                }
                flags &= ~(WINED3D_BLT_SRC_CKEY | WINED3D_BLT_DST_CKEY
                        | WINED3D_BLT_SRC_CKEY_OVERRIDE | WINED3D_BLT_DST_CKEY_OVERRIDE);
            }

            if (flags & WINED3D_BLT_FX)
            {
                BYTE *dTopLeft, *dTopRight, *dBottomLeft, *dBottomRight, *tmp;
                LONG tmpxy;
                dTopLeft     = dbuf;
                dTopRight    = dbuf + ((dst_width - 1) * bpp);
                dBottomLeft  = dTopLeft + ((dst_height - 1) * dst_map.row_pitch);
                dBottomRight = dBottomLeft + ((dst_width - 1) * bpp);

                if (fx->fx & WINEDDBLTFX_ARITHSTRETCHY)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Nothing done for WINEDDBLTFX_ARITHSTRETCHY.\n");
                }
                if (fx->fx & WINEDDBLTFX_MIRRORLEFTRIGHT)
                {
                    tmp          = dTopRight;
                    dTopRight    = dTopLeft;
                    dTopLeft     = tmp;
                    tmp          = dBottomRight;
                    dBottomRight = dBottomLeft;
                    dBottomLeft  = tmp;
                    dstxinc = dstxinc * -1;
                }
                if (fx->fx & WINEDDBLTFX_MIRRORUPDOWN)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dBottomLeft;
                    dBottomLeft  = tmp;
                    tmp          = dTopRight;
                    dTopRight    = dBottomRight;
                    dBottomRight = tmp;
                    dstyinc = dstyinc * -1;
                }
                if (fx->fx & WINEDDBLTFX_NOTEARING)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Nothing done for WINEDDBLTFX_NOTEARING.\n");
                }
                if (fx->fx & WINEDDBLTFX_ROTATE180)
                {
                    tmp          = dBottomRight;
                    dBottomRight = dTopLeft;
                    dTopLeft     = tmp;
                    tmp          = dBottomLeft;
                    dBottomLeft  = dTopRight;
                    dTopRight    = tmp;
                    dstxinc = dstxinc * -1;
                    dstyinc = dstyinc * -1;
                }
                if (fx->fx & WINEDDBLTFX_ROTATE270)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dBottomLeft;
                    dBottomLeft  = dBottomRight;
                    dBottomRight = dTopRight;
                    dTopRight    = tmp;
                    tmpxy   = dstxinc;
                    dstxinc = dstyinc;
                    dstyinc = tmpxy;
                    dstxinc = dstxinc * -1;
                }
                if (fx->fx & WINEDDBLTFX_ROTATE90)
                {
                    tmp          = dTopLeft;
                    dTopLeft     = dTopRight;
                    dTopRight    = dBottomRight;
                    dBottomRight = dBottomLeft;
                    dBottomLeft  = tmp;
                    tmpxy   = dstxinc;
                    dstxinc = dstyinc;
                    dstyinc = tmpxy;
                    dstyinc = dstyinc * -1;
                }
                if (fx->fx & WINEDDBLTFX_ZBUFFERBASEDEST)
                {
                    /* I don't think we need to do anything about this flag */
                    WARN("Nothing done for WINEDDBLTFX_ZBUFFERBASEDEST.\n");
                }
                dbuf = dTopLeft;
                flags &= ~(WINED3D_BLT_FX);
            }

#define COPY_COLORKEY_FX(type) \
do { \
    const type *s; \
    type *d = (type *)dbuf, *dx, tmp; \
    for (y = sy = 0; y < dst_height; ++y, sy += yinc) \
    { \
        s = (const type *)(sbase + (sy >> 16) * src_map.row_pitch); \
        dx = d; \
        for (x = sx = 0; x < dst_width; ++x, sx += xinc) \
        { \
            tmp = s[sx >> 16]; \
            if (((tmp & keymask) < keylow || (tmp & keymask) > keyhigh) \
                    && ((dx[0] & destkeymask) >= destkeylow && (dx[0] & destkeymask) <= destkeyhigh)) \
            { \
                dx[0] = tmp; \
            } \
            dx = (type *)(((BYTE *)dx) + dstxinc); \
        } \
        d = (type *)(((BYTE *)d) + dstyinc); \
    } \
} while(0)

            switch (bpp)
            {
                case 1:
                    COPY_COLORKEY_FX(BYTE);
                    break;
                case 2:
                    COPY_COLORKEY_FX(WORD);
                    break;
                case 4:
                    COPY_COLORKEY_FX(DWORD);
                    break;
                case 3:
                {
                    const BYTE *s;
                    BYTE *d = dbuf, *dx;
                    for (y = sy = 0; y < dst_height; ++y, sy += yinc)
                    {
                        sbuf = sbase + (sy >> 16) * src_map.row_pitch;
                        dx = d;
                        for (x = sx = 0; x < dst_width; ++x, sx+= xinc)
                        {
                            DWORD pixel, dpixel = 0;
                            s = sbuf + 3 * (sx>>16);
                            pixel = s[0] | (s[1] << 8) | (s[2] << 16);
                            dpixel = dx[0] | (dx[1] << 8 ) | (dx[2] << 16);
                            if (((pixel & keymask) < keylow || (pixel & keymask) > keyhigh)
                                    && ((dpixel & keymask) >= destkeylow || (dpixel & keymask) <= keyhigh))
                            {
                                dx[0] = (pixel      ) & 0xff;
                                dx[1] = (pixel >>  8) & 0xff;
                                dx[2] = (pixel >> 16) & 0xff;
                            }
                            dx += dstxinc;
                        }
                        d += dstyinc;
                    }
                    break;
                }
                default:
                    FIXME("%s color-keyed blit not implemented for bpp %u!\n",
                          (flags & WINED3D_BLT_SRC_CKEY) ? "Source" : "Destination", bpp * 8);
                    hr = WINED3DERR_NOTAVAILABLE;
                    goto error;
#undef COPY_COLORKEY_FX
            }
        }
    }

error:
    if (flags)
        FIXME("    Unsupported flags %#x.\n", flags);

release:
    wined3d_resource_unmap(&dst_texture->resource, dst_sub_resource_idx);
    if (src_texture && !same_sub_resource)
        wined3d_resource_unmap(&src_texture->resource, src_sub_resource_idx);
    if (converted_texture)
        wined3d_texture_decref(converted_texture);

    return hr;
}

static HRESULT cpu_blit_color_fill(struct wined3d_device *device, struct wined3d_rendertarget_view *view,
        const RECT *rect, const struct wined3d_color *color)
{
    const struct wined3d_box box = {rect->left, rect->top, rect->right, rect->bottom, 0, 1};
    static const struct wined3d_box src_box;
    struct wined3d_blt_fx fx;

    fx.fill_color = wined3d_format_convert_from_float(view->format, color);
    return surface_cpu_blt(texture_from_resource(view->resource), view->sub_resource_idx,
            &box, NULL, 0, &src_box, WINED3D_BLT_COLOR_FILL, &fx, WINED3D_TEXF_POINT);
}

static HRESULT cpu_blit_depth_fill(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view,  const RECT *rect, DWORD clear_flags,
        float depth, DWORD stencil)
{
    FIXME("Depth/stencil filling not implemented by cpu_blit.\n");
    return WINED3DERR_INVALIDCALL;
}

static void cpu_blit_blit_surface(struct wined3d_device *device, enum wined3d_blit_op op, DWORD filter,
        struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const RECT *dst_rect,
        const struct wined3d_color_key *color_key)
{
    /* FIXME: Remove error returns from surface_blt_cpu. */
    ERR("Blit method not implemented by cpu_blit.\n");
}

const struct blit_shader cpu_blit =  {
    cpu_blit_alloc,
    cpu_blit_free,
    cpu_blit_set,
    cpu_blit_unset,
    cpu_blit_supported,
    cpu_blit_color_fill,
    cpu_blit_depth_fill,
    cpu_blit_blit_surface,
};

HRESULT wined3d_surface_blt(struct wined3d_surface *dst_surface, const RECT *dst_rect,
        struct wined3d_surface *src_surface, const RECT *src_rect, DWORD flags,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_box dst_box = {dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, 0, 1};
    struct wined3d_box src_box = {src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1};
    unsigned int dst_sub_resource_idx = surface_get_sub_resource_idx(dst_surface);
    struct wined3d_texture *dst_texture = dst_surface->container;
    struct wined3d_device *device = dst_texture->resource.device;
    struct wined3d_swapchain *src_swapchain, *dst_swapchain;
    struct wined3d_texture *src_texture = NULL;
    unsigned int dst_w, dst_h, src_w, src_h;
    unsigned int src_sub_resource_idx = 0;
    DWORD src_ds_flags, dst_ds_flags;
    BOOL scale, convert;

    static const DWORD simple_blit = WINED3D_BLT_ASYNC
            | WINED3D_BLT_COLOR_FILL
            | WINED3D_BLT_SRC_CKEY
            | WINED3D_BLT_SRC_CKEY_OVERRIDE
            | WINED3D_BLT_WAIT
            | WINED3D_BLT_DEPTH_FILL
            | WINED3D_BLT_DO_NOT_WAIT
            | WINED3D_BLT_ALPHA_TEST;

    TRACE("dst_surface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_surface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect),
            flags, fx, debug_d3dtexturefiltertype(filter));
    TRACE("Usage is %s.\n", debug_d3dusage(dst_texture->resource.usage));

    if (fx)
    {
        TRACE("fx %#x.\n", fx->fx);
        TRACE("fill_color 0x%08x.\n", fx->fill_color);
        TRACE("dst_color_key {0x%08x, 0x%08x}.\n",
                fx->dst_color_key.color_space_low_value,
                fx->dst_color_key.color_space_high_value);
        TRACE("src_color_key {0x%08x, 0x%08x}.\n",
                fx->src_color_key.color_space_low_value,
                fx->src_color_key.color_space_high_value);
    }

    if (src_surface)
    {
        src_texture = src_surface->container;
        src_sub_resource_idx = surface_get_sub_resource_idx(src_surface);
    }

    if (dst_texture->sub_resources[dst_sub_resource_idx].map_count
            || (src_texture && src_texture->sub_resources[src_sub_resource_idx].map_count))
    {
        WARN("Surface is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
    }

    dst_w = wined3d_texture_get_level_width(dst_texture, dst_surface->texture_level);
    dst_h = wined3d_texture_get_level_height(dst_texture, dst_surface->texture_level);
    if (IsRectEmpty(dst_rect) || dst_rect->left > dst_w || dst_rect->left < 0
            || dst_rect->top > dst_h || dst_rect->top < 0
            || dst_rect->right > dst_w || dst_rect->right < 0
            || dst_rect->bottom > dst_h || dst_rect->bottom < 0)
    {
        WARN("The application gave us a bad destination rectangle.\n");
        return WINEDDERR_INVALIDRECT;
    }

    if (src_texture)
    {
        src_w = wined3d_texture_get_level_width(src_texture, src_surface->texture_level);
        src_h = wined3d_texture_get_level_height(src_texture, src_surface->texture_level);
        if (IsRectEmpty(src_rect) || src_rect->left > src_w || src_rect->left < 0
                || src_rect->top > src_h || src_rect->top < 0
                || src_rect->right > src_w || src_rect->right < 0
                || src_rect->bottom > src_h || src_rect->bottom < 0)
        {
            WARN("The application gave us a bad source rectangle.\n");
            return WINEDDERR_INVALIDRECT;
        }
    }

    if (!fx || !(fx->fx))
        flags &= ~WINED3D_BLT_FX;

    if (flags & WINED3D_BLT_WAIT)
        flags &= ~WINED3D_BLT_WAIT;

    if (flags & WINED3D_BLT_ASYNC)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINED3D_BLT_ASYNC flag.\n");
        flags &= ~WINED3D_BLT_ASYNC;
    }

    /* WINED3D_BLT_DO_NOT_WAIT appeared in DX7. */
    if (flags & WINED3D_BLT_DO_NOT_WAIT)
    {
        static unsigned int once;

        if (!once++)
            FIXME("Can't handle WINED3D_BLT_DO_NOT_WAIT flag.\n");
        flags &= ~WINED3D_BLT_DO_NOT_WAIT;
    }

    if (!device->d3d_initialized)
    {
        WARN("D3D not initialized, using fallback.\n");
        goto cpu;
    }

    /* We want to avoid invalidating the sysmem location for converted
     * surfaces, since otherwise we'd have to convert the data back when
     * locking them. */
    if (dst_texture->flags & WINED3D_TEXTURE_CONVERTED || dst_texture->resource.format->convert
            || wined3d_format_get_color_key_conversion(dst_texture, TRUE))
    {
        WARN_(d3d_perf)("Converted surface, using CPU blit.\n");
        goto cpu;
    }

    if (flags & ~simple_blit)
    {
        WARN_(d3d_perf)("Using fallback for complex blit (%#x).\n", flags);
        goto fallback;
    }

    if (src_texture)
        src_swapchain = src_texture->swapchain;
    else
        src_swapchain = NULL;

    dst_swapchain = dst_texture->swapchain;

    /* This isn't strictly needed. FBO blits for example could deal with
     * cross-swapchain blits by first downloading the source to a texture
     * before switching to the destination context. We just have this here to
     * not have to deal with the issue, since cross-swapchain blits should be
     * rare. */
    if (src_swapchain && dst_swapchain && src_swapchain != dst_swapchain)
    {
        FIXME("Using fallback for cross-swapchain blit.\n");
        goto fallback;
    }

    scale = src_texture
            && (src_rect->right - src_rect->left != dst_rect->right - dst_rect->left
            || src_rect->bottom - src_rect->top != dst_rect->bottom - dst_rect->top);
    convert = src_texture && src_texture->resource.format->id != dst_texture->resource.format->id;

    dst_ds_flags = dst_texture->resource.format_flags
            & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    if (src_texture)
        src_ds_flags = src_texture->resource.format_flags
                & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL);
    else
        src_ds_flags = 0;

    if (src_ds_flags || dst_ds_flags)
    {
        if (flags & WINED3D_BLT_DEPTH_FILL)
        {
            float depth;

            TRACE("Depth fill.\n");

            if (!surface_convert_depth_to_float(dst_surface, fx->fill_color, &depth))
                return WINED3DERR_INVALIDCALL;

            if (SUCCEEDED(wined3d_surface_depth_fill(dst_surface, dst_rect, depth)))
                return WINED3D_OK;
        }
        else
        {
            if (src_ds_flags != dst_ds_flags)
            {
                WARN("Rejecting depth / stencil blit between incompatible formats.\n");
                return WINED3DERR_INVALIDCALL;
            }

            if (SUCCEEDED(wined3d_surface_depth_blt(src_surface, src_texture->resource.draw_binding,
                    src_rect, dst_surface, dst_texture->resource.draw_binding, dst_rect)))
                return WINED3D_OK;
        }
    }
    else
    {
        struct wined3d_texture_sub_resource *src_sub_resource, *dst_sub_resource;
        const struct blit_shader *blitter;

        dst_sub_resource = surface_get_sub_resource(dst_surface);
        src_sub_resource = src_texture ? &src_texture->sub_resources[src_sub_resource_idx] : NULL;

        /* In principle this would apply to depth blits as well, but we don't
         * implement those in the CPU blitter at the moment. */
        if ((dst_sub_resource->locations & dst_texture->resource.map_binding)
                && (!src_texture || (src_sub_resource->locations & src_texture->resource.map_binding)))
        {
            if (scale)
                TRACE("Not doing sysmem blit because of scaling.\n");
            else if (convert)
                TRACE("Not doing sysmem blit because of format conversion.\n");
            else
                goto cpu;
        }

        if (flags & WINED3D_BLT_COLOR_FILL)
        {
            struct wined3d_color color;
            const struct wined3d_palette *palette = dst_swapchain ? dst_swapchain->palette : NULL;

            TRACE("Color fill.\n");

            if (!wined3d_format_convert_color_to_float(dst_texture->resource.format,
                    palette, fx->fill_color, &color))
                goto fallback;

            if (SUCCEEDED(surface_color_fill(dst_surface, dst_rect, &color)))
                return WINED3D_OK;
        }
        else
        {
            enum wined3d_blit_op blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
            const struct wined3d_color_key *color_key = NULL;

            TRACE("Color blit.\n");
            if (flags & WINED3D_BLT_SRC_CKEY_OVERRIDE)
            {
                color_key = &fx->src_color_key;
                blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
            }
            else if (flags & WINED3D_BLT_SRC_CKEY)
            {
                color_key = &src_texture->async.src_blt_color_key;
                blit_op = WINED3D_BLIT_OP_COLOR_BLIT_CKEY;
            }
            else if (flags & WINED3D_BLT_ALPHA_TEST)
            {
                blit_op = WINED3D_BLIT_OP_COLOR_BLIT_ALPHATEST;
            }
            else if ((src_sub_resource->locations & WINED3D_LOCATION_SYSMEM)
                    && !(dst_sub_resource->locations & WINED3D_LOCATION_SYSMEM))
            {
                /* Upload */
                if (scale)
                    TRACE("Not doing upload because of scaling.\n");
                else if (convert)
                    TRACE("Not doing upload because of format conversion.\n");
                else
                {
                    POINT dst_point = {dst_rect->left, dst_rect->top};

                    if (SUCCEEDED(surface_upload_from_surface(dst_surface, &dst_point, src_surface, src_rect)))
                    {
                        if (!wined3d_resource_is_offscreen(&dst_texture->resource))
                        {
                            struct wined3d_context *context = context_acquire(device, dst_surface);
                            wined3d_texture_load_location(dst_texture, dst_sub_resource_idx,
                                    context, dst_texture->resource.draw_binding);
                            context_release(context);
                        }
                        return WINED3D_OK;
                    }
                }
            }
            else if (dst_swapchain && dst_swapchain->back_buffers
                    && dst_texture == dst_swapchain->front_buffer
                    && src_texture == dst_swapchain->back_buffers[0])
            {
                /* Use present for back -> front blits. The idea behind this is
                 * that present is potentially faster than a blit, in particular
                 * when FBO blits aren't available. Some ddraw applications like
                 * Half-Life and Prince of Persia 3D use Blt() from the backbuffer
                 * to the frontbuffer instead of doing a Flip(). D3D8 and D3D9
                 * applications can't blit directly to the frontbuffer. */
                enum wined3d_swap_effect swap_effect = dst_swapchain->desc.swap_effect;

                TRACE("Using present for backbuffer -> frontbuffer blit.\n");

                /* Set the swap effect to COPY, we don't want the backbuffer
                 * to become undefined. */
                dst_swapchain->desc.swap_effect = WINED3D_SWAP_EFFECT_COPY;
                wined3d_swapchain_present(dst_swapchain, NULL, NULL, dst_swapchain->win_handle, 0);
                dst_swapchain->desc.swap_effect = swap_effect;

                return WINED3D_OK;
            }

            if (fbo_blit_supported(&device->adapter->gl_info, blit_op,
                    src_rect, src_texture->resource.usage, src_texture->resource.pool, src_texture->resource.format,
                    dst_rect, dst_texture->resource.usage, dst_texture->resource.pool, dst_texture->resource.format))
            {
                struct wined3d_context *context;
                TRACE("Using FBO blit.\n");

                context = context_acquire(device, NULL);
                surface_blt_fbo(device, context, filter,
                        src_surface, src_texture->resource.draw_binding, src_rect,
                        dst_surface, dst_texture->resource.draw_binding, dst_rect);
                context_release(context);

                wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx,
                        dst_texture->resource.draw_binding);
                wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx,
                        ~dst_texture->resource.draw_binding);

                return WINED3D_OK;
            }

            blitter = wined3d_select_blitter(&device->adapter->gl_info, &device->adapter->d3d_info, blit_op,
                    src_rect, src_texture->resource.usage, src_texture->resource.pool, src_texture->resource.format,
                    dst_rect, dst_texture->resource.usage, dst_texture->resource.pool, dst_texture->resource.format);
            if (blitter)
            {
                blitter->blit_surface(device, blit_op, filter, src_surface,
                        src_rect, dst_surface, dst_rect, color_key);
                return WINED3D_OK;
            }
        }
    }

fallback:
    /* Special cases for render targets. */
    if (SUCCEEDED(surface_blt_special(dst_surface, dst_rect, src_surface, src_rect, flags, fx, filter)))
        return WINED3D_OK;

cpu:
    return surface_cpu_blt(dst_texture, dst_sub_resource_idx, &dst_box,
            src_texture, src_sub_resource_idx, &src_box, flags, fx, filter);
}
