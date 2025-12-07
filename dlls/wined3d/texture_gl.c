/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Roderick Colenbrander
 * Copyright 2006-2009, 2013-2016 Stefan Dösinger for CodeWeavers
 * Copyright 2007-2008 Henri Verbeet
 * Copyright 2008-2020 Henri Verbeet for CodeWeavers
 * Copyright 2016-2017 Matteo Bruni for CodeWeavers
 * Copyright 2016-2018 Józef Kucia for CodeWeavers
 * Copyright 2018 Andrew Wesie
 * Copyright 2018 Connor McAdams
 * Copyright 2018 Sven Hesse for CodeWeavers
 * Copyright 2019 Paul Gofman
 * Copyright 2022 Elizabeth Figura for CodeWeavers
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

#include "wined3d_private.h"
#include "wined3d_gl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

GLenum wined3d_texture_get_gl_buffer(const struct wined3d_texture *texture)
{
    const struct wined3d_swapchain *swapchain = texture->swapchain;

    TRACE("texture %p.\n", texture);

    if (!swapchain)
    {
        ERR("Texture %p is not part of a swapchain.\n", texture);
        return GL_NONE;
    }

    if (texture == swapchain->front_buffer)
    {
        TRACE("Returning GL_FRONT.\n");
        return GL_FRONT;
    }

    if (texture == swapchain->back_buffers[0])
    {
        TRACE("Returning GL_BACK.\n");
        return GL_BACK;
    }

    FIXME("Higher back buffer, returning GL_BACK.\n");
    return GL_BACK;
}

static void ffp_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    free(blitter);
}

bool wined3d_rendertarget_view_is_full_clear(const struct wined3d_rendertarget_view *rtv,
        const RECT *draw_rect, const RECT *clear_rect)
{
    unsigned int height = rtv->height;
    unsigned int width = rtv->width;

    /* partial draw rect */
    if (draw_rect->left || draw_rect->top || draw_rect->right < width || draw_rect->bottom < height)
        return false;

    /* partial clear rect */
    if (clear_rect && (clear_rect->left > 0 || clear_rect->top > 0
            || clear_rect->right < width || clear_rect->bottom < height))
        return false;

    return true;
}

static void ffp_blitter_clear_rendertargets(struct wined3d_device *device, unsigned int rt_count,
        const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rect, const RECT *draw_rect,
        uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_rendertarget_view *rtv = rt_count ? fb->render_targets[0] : NULL;
    struct wined3d_rendertarget_view *dsv = fb->depth_stencil;
    const struct wined3d_state *state = &device->cs->state;
    struct wined3d_texture *depth_stencil = NULL;
    unsigned int drawable_width, drawable_height;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    struct wined3d_texture *target = NULL;
    struct wined3d_color colour_srgb;
    struct wined3d_context *context;
    GLbitfield clear_mask = 0;
    bool render_offscreen;
    unsigned int i;

    if (rtv && rtv->resource->type != WINED3D_RTYPE_BUFFER)
    {
        target = texture_from_resource(rtv->resource);
        context = context_acquire(device, target, rtv->sub_resource_idx);
    }
    else
    {
        context = context_acquire(device, NULL, 0);
    }
    context_gl = wined3d_context_gl(context);

    if (dsv && dsv->resource->type != WINED3D_RTYPE_BUFFER)
        depth_stencil = texture_from_resource(dsv->resource);

    if (!context_gl->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping clear.\n");
        return;
    }
    gl_info = context_gl->gl_info;

    /* When we're clearing parts of the drawable, make sure that the target
     * surface is well up to date in the drawable. After the clear we'll mark
     * the drawable up to date, so we have to make sure that this is true for
     * the cleared parts, and the untouched parts.
     *
     * If we're clearing the whole target there is no need to copy it into the
     * drawable, it will be overwritten anyway. If we're not clearing the
     * colour buffer we don't have to copy either since we're not going to set
     * the drawable up to date. We have to check all settings that limit the
     * clear area though. Do not bother checking all this if the destination
     * surface is in the drawable anyway. */
    for (i = 0; i < rt_count; ++i)
    {
        struct wined3d_rendertarget_view *rtv = fb->render_targets[i];

        if (rtv && rtv->format->id != WINED3DFMT_NULL)
        {
            if ((flags & WINED3DCLEAR_TARGET)
                    && !wined3d_rendertarget_view_is_full_clear(rtv, draw_rect, rect_count ? clear_rect : NULL))
                wined3d_rendertarget_view_load_location(rtv, context, rtv->resource->draw_binding);
            else
                wined3d_rendertarget_view_prepare_location(rtv, context, rtv->resource->draw_binding);
        }
    }

    if (target)
    {
        render_offscreen = wined3d_resource_is_offscreen(&target->resource);
        wined3d_rendertarget_view_get_drawable_size(rtv, context, &drawable_width, &drawable_height);
    }
    else
    {
        unsigned int ds_level = dsv->sub_resource_idx % depth_stencil->level_count;

        render_offscreen = true;
        drawable_width = wined3d_texture_get_level_width(depth_stencil, ds_level);
        drawable_height = wined3d_texture_get_level_height(depth_stencil, ds_level);
    }

    if (depth_stencil)
    {
        DWORD ds_location = render_offscreen ? dsv->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;

        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)
                && !wined3d_rendertarget_view_is_full_clear(dsv, draw_rect, rect_count ? clear_rect : NULL))
            wined3d_rendertarget_view_load_location(dsv, context, ds_location);
        else
            wined3d_rendertarget_view_prepare_location(dsv, context, ds_location);

        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
        {
            wined3d_rendertarget_view_validate_location(dsv, ds_location);
            wined3d_rendertarget_view_invalidate_location(dsv, ~ds_location);
        }
    }

    if (!wined3d_context_gl_apply_clear_state(context_gl, state, rt_count, fb))
    {
        context_release(context);
        WARN("Failed to apply clear state, skipping clear.\n");
        return;
    }

    /* Only set the values up once, as they are not changing. */
    if (flags & WINED3DCLEAR_STENCIL)
    {
        if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        gl_info->gl_ops.gl.p_glStencilMask(~0u);
        context_invalidate_state(context, STATE_DEPTH_STENCIL);
        gl_info->gl_ops.gl.p_glClearStencil(stencil);
        checkGLcall("glClearStencil");
        clear_mask = clear_mask | GL_STENCIL_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_ZBUFFER)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_DEPTH_STENCIL);
        if (gl_info->supported[ARB_ES2_COMPATIBILITY])
            GL_EXTCALL(glClearDepthf(depth));
        else
            gl_info->gl_ops.gl.p_glClearDepth(depth);
        checkGLcall("glClearDepth");
        clear_mask = clear_mask | GL_DEPTH_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            struct wined3d_rendertarget_view *rtv = fb->render_targets[i];

            if (!rtv)
                continue;

            if (rtv->resource->type == WINED3D_RTYPE_BUFFER)
            {
                FIXME("Not supported on buffer resources.\n");
                continue;
            }

            wined3d_rendertarget_view_validate_location(rtv, rtv->resource->draw_binding);
            wined3d_rendertarget_view_invalidate_location(rtv, ~rtv->resource->draw_binding);
        }

        if (!gl_info->supported[ARB_FRAMEBUFFER_SRGB] && needs_srgb_write(context->d3d_info, state, fb))
        {
            if (rt_count > 1)
                WARN("Clearing multiple sRGB render targets without GL_ARB_framebuffer_sRGB "
                        "support, this might cause graphical issues.\n");

            wined3d_colour_srgb_from_linear(&colour_srgb, colour);
            colour = &colour_srgb;
        }

        gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        context_invalidate_state(context, STATE_BLEND);
        gl_info->gl_ops.gl.p_glClearColor(colour->r, colour->g, colour->b, colour->a);
        checkGLcall("glClearColor");
        clear_mask = clear_mask | GL_COLOR_BUFFER_BIT;
    }

    if (!rect_count)
    {
        if (render_offscreen)
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, draw_rect->top,
                    draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        else
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, drawable_height - draw_rect->bottom,
                        draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        gl_info->gl_ops.gl.p_glClear(clear_mask);
    }
    else
    {
        RECT current_rect;

        /* Now process each rect in turn. */
        for (i = 0; i < rect_count; ++i)
        {
            /* Note that GL uses lower left, width/height. */
            IntersectRect(&current_rect, draw_rect, &clear_rect[i]);

            TRACE("clear_rect[%u] %s, current_rect %s.\n", i,
                    wine_dbgstr_rect(&clear_rect[i]),
                    wine_dbgstr_rect(&current_rect));

            /* Tests show that rectangles where x1 > x2 or y1 > y2 are ignored
             * silently. The rectangle is not cleared, no error is returned,
             * but further rectangles are still cleared if they are valid. */
            if (current_rect.left > current_rect.right || current_rect.top > current_rect.bottom)
            {
                TRACE("Rectangle with negative dimensions, ignoring.\n");
                continue;
            }

            if (render_offscreen)
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, current_rect.top,
                        current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            else
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, drawable_height - current_rect.bottom,
                          current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            gl_info->gl_ops.gl.p_glClear(clear_mask);
        }
    }
    context->scissor_rect_count = WINED3D_MAX_VIEWPORTS;
    checkGLcall("clear");

    if (flags & WINED3DCLEAR_TARGET && target->swapchain && target->swapchain->front_buffer == target)
        gl_info->gl_ops.gl.p_glFlush();

    context_release(context);
}

static void ffp_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_rendertarget_view *view, *previous = NULL;
    bool have_identical_size = TRUE;
    struct wined3d_fb_state tmp_fb;
    unsigned int next_rt_count = 0;
    struct wined3d_blitter *next;
    DWORD next_flags = 0;
    unsigned int i;

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (!(view = fb->render_targets[i]))
                continue;

            if (wined3d_rendertarget_view_use_cpu_clear(view)
                    || (!(view->resource->bind_flags & WINED3D_BIND_RENDER_TARGET)
                    && !(view->format_caps & WINED3D_FORMAT_CAP_FBO_ATTACHABLE)))
            {
                next_flags |= WINED3DCLEAR_TARGET;
                flags &= ~WINED3DCLEAR_TARGET;
                next_rt_count = rt_count;
                rt_count = 0;
                break;
            }

            /* FIXME: We should reject colour fills on formats with fixups,
             * but this would break P8 colour fills for example. */
        }
    }

    if ((flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)) && (view = fb->depth_stencil)
            && (!view->format->depth_size || (flags & WINED3DCLEAR_ZBUFFER))
            && (!view->format->stencil_size || (flags & WINED3DCLEAR_STENCIL))
            && wined3d_rendertarget_view_use_cpu_clear(view))
    {
        next_flags |= flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
        flags &= ~(WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    }

    if (flags)
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (!(view = fb->render_targets[i]))
                continue;

            if (previous && (previous->width != view->width || previous->height != view->height))
                have_identical_size = false;
            previous = view;
        }
        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
        {
            view = fb->depth_stencil;

            if (previous && (previous->width != view->width || previous->height != view->height))
                have_identical_size = false;
        }

        if (have_identical_size)
        {
            ffp_blitter_clear_rendertargets(device, rt_count, fb, rect_count,
                    clear_rects, draw_rect, flags, colour, depth, stencil);
        }
        else
        {
            for (i = 0; i < rt_count; ++i)
            {
                if (!(view = fb->render_targets[i]))
                    continue;

                tmp_fb.render_targets[0] = view;
                tmp_fb.depth_stencil = NULL;
                ffp_blitter_clear_rendertargets(device, 1, &tmp_fb, rect_count,
                        clear_rects, draw_rect, WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
            if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
            {
                tmp_fb.render_targets[0] = NULL;
                tmp_fb.depth_stencil = fb->depth_stencil;
                ffp_blitter_clear_rendertargets(device, 0, &tmp_fb, rect_count,
                        clear_rects, draw_rect, flags & ~WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
        }
    }

    if (next_flags && (next = blitter->next))
        next->ops->blitter_clear(next, device, next_rt_count, fb, rect_count,
                clear_rects, draw_rect, next_flags, colour, depth, stencil);
}

static DWORD ffp_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter,
        const struct wined3d_format *resolve_format)
{
    struct wined3d_blitter *next;

    if (!(next = blitter->next))
    {
        ERR("No blitter to handle blit op %#x.\n", op);
        return dst_location;
    }

    return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
            src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter,
            resolve_format);
}

static const struct wined3d_blitter_ops ffp_blitter_ops =
{
    ffp_blitter_destroy,
    ffp_blitter_clear,
    ffp_blitter_blit,
};

void wined3d_ffp_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = malloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &ffp_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

static void raw_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    free(blitter);
}

static void raw_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_blitter *next;

    if (!(next = blitter->next))
    {
        ERR("No blitter to handle clear.\n");
        return;
    }

    TRACE("Forwarding to blitter %p.\n", next);
    next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
            clear_rects, draw_rect, flags, colour, depth, stencil);
}

static bool gl_formats_compatible(struct wined3d_texture *src_texture, DWORD src_location,
        struct wined3d_texture *dst_texture, DWORD dst_location)
{
    GLuint src_internal, dst_internal;
    bool src_ds, dst_ds;

    src_ds = src_texture->resource.format->depth_size || src_texture->resource.format->stencil_size;
    dst_ds = dst_texture->resource.format->depth_size || dst_texture->resource.format->stencil_size;
    if (src_ds == dst_ds)
        return true;
    /* Also check the internal format because, e.g. WINED3DFMT_D24_UNORM_S8_UINT has nonzero depth and stencil
     * sizes as does WINED3DFMT_R24G8_TYPELESS when bound with flag WINED3D_BIND_DEPTH_STENCIL, but these share
     * the same internal format with WINED3DFMT_R24_UNORM_X8_TYPELESS. */
    src_internal = wined3d_gl_get_internal_format(&src_texture->resource,
            wined3d_format_gl(src_texture->resource.format), src_location == WINED3D_LOCATION_TEXTURE_SRGB);
    dst_internal = wined3d_gl_get_internal_format(&dst_texture->resource,
            wined3d_format_gl(dst_texture->resource.format), dst_location == WINED3D_LOCATION_TEXTURE_SRGB);
    return src_internal == dst_internal;
}

static DWORD raw_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter,
        const struct wined3d_format *resolve_format)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_texture_gl *dst_texture_gl = wined3d_texture_gl(dst_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int src_level, src_layer, dst_level, dst_layer;
    struct wined3d_blitter *next;
    GLuint src_name, dst_name;
    DWORD location;

    /* If we would need to copy from a renderbuffer or drawable, we'd probably
     * be better off using the FBO blitter directly, since we'd need to use it
     * to copy the resource contents to the texture anyway.
     *
     * We also can't copy between depth/stencil and colour resources, since
     * the formats are considered incompatible in OpenGL. */
    if (op != WINED3D_BLIT_OP_RAW_BLIT || !gl_formats_compatible(src_texture, src_location, dst_texture, dst_location)
            || ((src_texture->resource.format_attrs | dst_texture->resource.format_attrs)
                    & WINED3D_FORMAT_ATTR_HEIGHT_SCALE)
            || (src_texture->resource.format->id == dst_texture->resource.format->id
            && (!(src_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            || !(dst_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB)))))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter,
                resolve_format);
    }

    TRACE("Blit using ARB_copy_image.\n");

    src_level = src_sub_resource_idx % src_texture->level_count;
    src_layer = src_sub_resource_idx / src_texture->level_count;

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    dst_layer = dst_sub_resource_idx / dst_texture->level_count;

    location = src_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    if (!location)
        location = src_texture->flags & WINED3D_TEXTURE_IS_SRGB
                ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    if (!wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, location))
        ERR("Failed to load the source sub-resource into %s.\n", wined3d_debug_location(location));
    src_name = wined3d_texture_gl_get_texture_name(src_texture_gl,
            context, location == WINED3D_LOCATION_TEXTURE_SRGB);

    location = dst_location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    if (!location)
        location = dst_texture->flags & WINED3D_TEXTURE_IS_SRGB
                ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    if (wined3d_texture_is_full_rect(dst_texture, dst_level, dst_rect))
    {
        if (!wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, location))
            ERR("Failed to prepare the destination sub-resource into %s.\n", wined3d_debug_location(location));
    }
    else
    {
        if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, location))
            ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(location));
    }
    dst_name = wined3d_texture_gl_get_texture_name(dst_texture_gl,
            context, location == WINED3D_LOCATION_TEXTURE_SRGB);

    GL_EXTCALL(glCopyImageSubData(src_name, src_texture_gl->target, src_level,
            src_rect->left, src_rect->top, src_layer, dst_name, dst_texture_gl->target, dst_level,
            dst_rect->left, dst_rect->top, dst_layer, src_rect->right - src_rect->left,
            src_rect->bottom - src_rect->top, 1));
    checkGLcall("copy image data");

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, location);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~location);
    if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location))
        ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(dst_location));

    return dst_location | location;
}

static const struct wined3d_blitter_ops raw_blitter_ops =
{
    raw_blitter_destroy,
    raw_blitter_clear,
    raw_blitter_blit,
};

void wined3d_raw_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if (!gl_info->supported[ARB_COPY_IMAGE])
        return;

    if (!(blitter = malloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &raw_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

static bool fbo_blitter_supported(enum wined3d_blit_op blit_op, const struct wined3d_gl_info *gl_info,
        const struct wined3d_resource *src_resource, DWORD src_location,
        const struct wined3d_resource *dst_resource, DWORD dst_location)
{
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;
    bool src_ds, dst_ds;

    if ((src_resource->format_attrs | dst_resource->format_attrs) & WINED3D_FORMAT_ATTR_HEIGHT_SCALE)
        return false;

    /* Source and/or destination need to be on the GL side. */
    if (!(src_resource->access & dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
        return false;

    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return false;

    /* We can't copy between depth/stencil and colour attachments. One notable
     * way we can end up here is when copying between typeless resources with
     * formats like R16_TYPELESS, which can end up using either a
     * depth/stencil or a colour format on the OpenGL side, depending on the
     * resource's bind flags. */
    src_ds = src_format->depth_size || src_format->stencil_size;
    dst_ds = dst_format->depth_size || dst_format->stencil_size;
    if (src_ds != dst_ds)
        return false;

    switch (blit_op)
    {
        case WINED3D_BLIT_OP_COLOR_BLIT:
            if (!((src_format->caps[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3D_FORMAT_CAP_FBO_ATTACHABLE)
                    || (src_resource->bind_flags & WINED3D_BIND_RENDER_TARGET)))
                return false;
            if (!((dst_format->caps[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3D_FORMAT_CAP_FBO_ATTACHABLE)
                    || (dst_resource->bind_flags & WINED3D_BIND_RENDER_TARGET)))
                return false;
            if ((src_format->id != dst_format->id || dst_location == WINED3D_LOCATION_DRAWABLE)
                    && (!is_identity_fixup(src_format->color_fixup) || !is_identity_fixup(dst_format->color_fixup)))
                return false;
            break;

        case WINED3D_BLIT_OP_DEPTH_BLIT:
            if (!(src_format->caps[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3D_FORMAT_CAP_DEPTH_STENCIL))
                return false;
            if (!(dst_format->caps[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3D_FORMAT_CAP_DEPTH_STENCIL))
                return false;
            /* Accept pure swizzle fixups for depth formats. In general we
             * ignore the stencil component (if present) at the moment and the
             * swizzle is not relevant with just the depth component. */
            if (is_complex_fixup(src_format->color_fixup) || is_complex_fixup(dst_format->color_fixup)
                    || is_scaling_fixup(src_format->color_fixup) || is_scaling_fixup(dst_format->color_fixup))
                return false;
            break;

        default:
            return false;
    }

    return true;
}

/* Blit between surface locations. Onscreen on different swapchains is not supported.
 * Depth / stencil is not supported. Context activation is done by the caller. */
static void texture2d_blt_fbo(struct wined3d_device *device, struct wined3d_context *context,
        enum wined3d_texture_filter_type filter, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, DWORD src_location, const RECT *src_rect,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, DWORD dst_location,
        const RECT *dst_rect, const struct wined3d_format *resolve_format)
{
    struct wined3d_texture *required_texture, *restore_texture, *dst_save_texture = dst_texture;
    unsigned int restore_idx, dst_save_sub_resource_idx = dst_sub_resource_idx;
    bool resolve, scaled_resolve, restore_context = false;
    struct wined3d_texture *src_staging_texture = NULL;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    GLenum gl_filter;
    GLenum buffer;
    RECT s, d;

    TRACE("device %p, context %p, filter %s, src_texture %p, src_sub_resource_idx %u, src_location %s, "
            "src_rect %s, dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, resolve format %p.\n",
            device, context, debug_d3dtexturefiltertype(filter), src_texture, src_sub_resource_idx,
            wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect), resolve_format);

    resolve = wined3d_texture_gl_is_multisample_location(wined3d_texture_gl(src_texture), src_location);
    scaled_resolve = resolve
            && (abs(src_rect->bottom - src_rect->top) != abs(dst_rect->bottom - dst_rect->top)
            || abs(src_rect->right - src_rect->left) != abs(dst_rect->right - dst_rect->left));

    if (filter == WINED3D_TEXF_LINEAR)
        gl_filter = scaled_resolve ? GL_SCALED_RESOLVE_NICEST_EXT : GL_LINEAR;
    else
        gl_filter = scaled_resolve ? GL_SCALED_RESOLVE_FASTEST_EXT : GL_NEAREST;

    if (resolve)
    {
        GLint resolve_internal, src_internal, dst_internal;
        enum wined3d_format_id resolve_format_id;

        src_internal = wined3d_gl_get_internal_format(&src_texture->resource,
                wined3d_format_gl(src_texture->resource.format), src_location == WINED3D_LOCATION_TEXTURE_SRGB);
        dst_internal = wined3d_gl_get_internal_format(&dst_texture->resource,
                wined3d_format_gl(dst_texture->resource.format), dst_location == WINED3D_LOCATION_TEXTURE_SRGB);

        if (resolve_format)
        {
            resolve_internal = wined3d_format_gl(resolve_format)->internal;
            resolve_format_id = resolve_format->id;
        }
        else if (!wined3d_format_is_typeless(src_texture->resource.format))
        {
            resolve_internal = src_internal;
            resolve_format_id = src_texture->resource.format->id;
        }
        else
        {
            resolve_internal = dst_internal;
            resolve_format_id = dst_texture->resource.format->id;
        }

        /* In case of typeless resolve the texture type may not match the resolve type.
         * To handle that, allocate intermediate texture(s) to resolve from/to.
         * A possible performance improvement would be to resolve using a shader instead. */
        if (src_internal != resolve_internal)
        {
            struct wined3d_resource_desc desc;
            unsigned src_level;
            HRESULT hr;

            src_level = src_sub_resource_idx % src_texture->level_count;
            desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
            desc.format = resolve_format_id;
            desc.multisample_type = src_texture->resource.multisample_type;
            desc.multisample_quality = src_texture->resource.multisample_quality;
            desc.usage = WINED3DUSAGE_CS;
            desc.bind_flags = 0;
            desc.access = WINED3D_RESOURCE_ACCESS_GPU;
            desc.width = wined3d_texture_get_level_width(src_texture, src_level);
            desc.height = wined3d_texture_get_level_height(src_texture, src_level);
            desc.depth = 1;
            desc.size = 0;

            hr = wined3d_texture_create(device, &desc, 1, 1, 0, NULL, NULL, &wined3d_null_parent_ops,
                    &src_staging_texture);
            if (FAILED(hr))
            {
                ERR("Failed to create staging texture, hr %#lx.\n", hr);
                goto done;
            }

            if (src_location == WINED3D_LOCATION_DRAWABLE)
                FIXME("WINED3D_LOCATION_DRAWABLE not supported for the source of a typeless resolve.\n");

            device->blitter->ops->blitter_blit(device->blitter, WINED3D_BLIT_OP_RAW_BLIT, context,
                    src_texture, src_sub_resource_idx, src_location, src_rect,
                    src_staging_texture, 0, src_location, src_rect,
                    NULL, WINED3D_TEXF_NONE, NULL);

            src_texture = src_staging_texture;
            src_sub_resource_idx = 0;
        }

        if (dst_internal != resolve_internal)
        {
            struct wined3d_resource_desc desc;
            unsigned dst_level;
            HRESULT hr;

            dst_level = dst_sub_resource_idx % dst_texture->level_count;
            desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
            desc.format = resolve_format_id;
            desc.multisample_type = dst_texture->resource.multisample_type;
            desc.multisample_quality = dst_texture->resource.multisample_quality;
            desc.usage = WINED3DUSAGE_CS;
            desc.bind_flags = 0;
            desc.access = WINED3D_RESOURCE_ACCESS_GPU;
            desc.width = wined3d_texture_get_level_width(dst_texture, dst_level);
            desc.height = wined3d_texture_get_level_height(dst_texture, dst_level);
            desc.depth = 1;
            desc.size = 0;

            hr = wined3d_texture_create(device, &desc, 1, 1, 0, NULL, NULL, &wined3d_null_parent_ops,
                    &dst_texture);
            if (FAILED(hr))
            {
                ERR("Failed to create staging texture, hr %#lx.\n", hr);
                goto done;
            }

            wined3d_texture_load_location(dst_texture, 0, context, dst_location);
            dst_sub_resource_idx = 0;
        }
    }

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. (And is
     * in fact harmful if we're being called by surface_load_location() with
     * the purpose of loading the destination surface.) */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, src_location);
    if (!wined3d_texture_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    /* Acquire a context for the front-buffer, even though we may be blitting
     * to/from a back-buffer. Since context_acquire() doesn't take the
     * resource location into account, it may consider the back-buffer to be
     * offscreen. */
    if (src_location == WINED3D_LOCATION_DRAWABLE)
        required_texture = src_texture->swapchain->front_buffer;
    else if (dst_location == WINED3D_LOCATION_DRAWABLE)
        required_texture = dst_texture->swapchain->front_buffer;
    else
        required_texture = NULL;

    restore_texture = context->current_rt.texture;
    restore_idx = context->current_rt.sub_resource_idx;
    if (restore_texture != required_texture)
    {
        context = context_acquire(device, required_texture, 0);
        restore_context = true;
    }

    context_gl = wined3d_context_gl(context);
    if (!context_gl->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping blit.\n");
        restore_context = false;
        goto done;
    }

    gl_info = context_gl->gl_info;

    if (src_location == WINED3D_LOCATION_DRAWABLE)
    {
        TRACE("Source texture %p is onscreen.\n", src_texture);
        buffer = wined3d_texture_get_gl_buffer(src_texture);
        s = *src_rect;
        wined3d_texture_translate_drawable_coords(src_texture, context_gl->window, &s);
        src_rect = &s;
    }
    else
    {
        TRACE("Source texture %p is offscreen.\n", src_texture);
        buffer = GL_COLOR_ATTACHMENT0;
    }

    wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_READ_FRAMEBUFFER,
            &src_texture->resource, src_sub_resource_idx, NULL, 0, src_location);
    gl_info->gl_ops.gl.p_glReadBuffer(buffer);
    checkGLcall("glReadBuffer()");
    wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);

    context_gl_apply_texture_draw_state(context_gl, dst_texture, dst_sub_resource_idx, dst_location);

    if (dst_location == WINED3D_LOCATION_DRAWABLE)
    {
        d = *dst_rect;
        wined3d_texture_translate_drawable_coords(dst_texture, context_gl->window, &d);
        dst_rect = &d;
    }

    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_invalidate_state(context, STATE_BLEND);

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RASTERIZER);

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, GL_COLOR_BUFFER_BIT, gl_filter);
    checkGLcall("glBlitFramebuffer()");

    if (dst_location == WINED3D_LOCATION_DRAWABLE && dst_texture->swapchain->front_buffer == dst_texture)
        gl_info->gl_ops.gl.p_glFlush();

    if (dst_texture != dst_save_texture)
    {
        if (dst_location == WINED3D_LOCATION_DRAWABLE)
            FIXME("WINED3D_LOCATION_DRAWABLE not supported for the destination of a typeless resolve.\n");

        device->blitter->ops->blitter_blit(device->blitter, WINED3D_BLIT_OP_RAW_BLIT, context,
                dst_texture, 0, dst_location, dst_rect,
                dst_save_texture, dst_save_sub_resource_idx, dst_location, dst_rect,
                NULL, WINED3D_TEXF_NONE, NULL);
    }

done:
    if (dst_texture != dst_save_texture)
        wined3d_texture_decref(dst_texture);

    if (src_staging_texture)
        wined3d_texture_decref(src_staging_texture);

    if (restore_context)
        context_restore(context, restore_texture, restore_idx);
}

static void texture2d_depth_blt_fbo(const struct wined3d_device *device, struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, DWORD src_location,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        DWORD dst_location, const RECT *dst_rect)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLbitfield src_mask, dst_mask;
    GLbitfield gl_mask;

    TRACE("device %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_rect %s, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s.\n", device,
            src_texture, src_sub_resource_idx, wined3d_debug_location(src_location), wine_dbgstr_rect(src_rect),
            dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location), wine_dbgstr_rect(dst_rect));

    src_mask = 0;
    if (src_texture->resource.format->depth_size)
        src_mask |= GL_DEPTH_BUFFER_BIT;
    if (src_texture->resource.format->stencil_size)
        src_mask |= GL_STENCIL_BUFFER_BIT;

    dst_mask = 0;
    if (dst_texture->resource.format->depth_size)
        dst_mask |= GL_DEPTH_BUFFER_BIT;
    if (dst_texture->resource.format->stencil_size)
        dst_mask |= GL_STENCIL_BUFFER_BIT;

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
    gl_mask = src_mask;

    /* Make sure the locations are up-to-date. Loading the destination
     * surface isn't required if the entire surface is overwritten. */
    wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, src_location);
    if (!wined3d_texture_is_full_rect(dst_texture, dst_sub_resource_idx % dst_texture->level_count, dst_rect))
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    else
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);

    wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_READ_FRAMEBUFFER, NULL, 0,
            &src_texture->resource, src_sub_resource_idx, src_location);
    wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);

    context_gl_apply_texture_draw_state(context_gl, dst_texture, dst_sub_resource_idx, dst_location);

    if (gl_mask & GL_DEPTH_BUFFER_BIT)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_DEPTH_STENCIL);
    }
    if (gl_mask & GL_STENCIL_BUFFER_BIT)
    {
        if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_DEPTH_STENCIL);
    }

    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RASTERIZER);

    gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
            dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, gl_mask, GL_NEAREST);
    checkGLcall("glBlitFramebuffer()");
}

static void fbo_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    free(blitter);
}

static void fbo_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_blitter *next;

    if ((next = blitter->next))
        next->ops->blitter_clear(next, device, rt_count, fb, rect_count,
                clear_rects, draw_rect, flags, colour, depth, stencil);
}

static DWORD fbo_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter,
        const struct wined3d_format *resolve_format)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_resource *src_resource, *dst_resource;
    enum wined3d_blit_op blit_op = op;
    struct wined3d_device *device;
    struct wined3d_blitter *next;

    TRACE("blitter %p, op %#x, context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, "
            "src_rect %s, dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, "
            "colour_key %p, filter %s, resolve_format %p.\n",
            blitter, op, context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            wine_dbgstr_rect(src_rect), dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location),
            wine_dbgstr_rect(dst_rect), colour_key, debug_d3dtexturefiltertype(filter), resolve_format);

    src_resource = &src_texture->resource;
    dst_resource = &dst_texture->resource;

    device = dst_resource->device;

    if (blit_op == WINED3D_BLIT_OP_RAW_BLIT && dst_resource->format->id == src_resource->format->id)
    {
        if (dst_resource->format->depth_size || dst_resource->format->stencil_size)
            blit_op = WINED3D_BLIT_OP_DEPTH_BLIT;
        else
            blit_op = WINED3D_BLIT_OP_COLOR_BLIT;
    }

    if (!fbo_blitter_supported(blit_op, context_gl->gl_info,
            src_resource, src_location, dst_resource, dst_location))
    {
        if (!(next = blitter->next))
        {
            ERR("No blitter to handle blit op %#x.\n", op);
            return dst_location;
        }

        TRACE("Forwarding to blitter %p.\n", next);
        return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter,
                resolve_format);
    }

    if (blit_op == WINED3D_BLIT_OP_COLOR_BLIT)
    {
        TRACE("Colour blit.\n");
        texture2d_blt_fbo(device, context, filter, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, resolve_format);
        return dst_location;
    }

    if (blit_op == WINED3D_BLIT_OP_DEPTH_BLIT)
    {
        TRACE("Depth/stencil blit.\n");
        texture2d_depth_blt_fbo(device, context, src_texture, src_sub_resource_idx, src_location,
                src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect);
        return dst_location;
    }

    ERR("This blitter does not implement blit op %#x.\n", blit_op);
    return dst_location;
}

static const struct wined3d_blitter_ops fbo_blitter_ops =
{
    fbo_blitter_destroy,
    fbo_blitter_clear,
    fbo_blitter_blit,
};

void wined3d_fbo_blitter_create(struct wined3d_blitter **next, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = malloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &fbo_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}

struct wined3d_rect_f
{
    float l;
    float t;
    float r;
    float b;
};

static inline void cube_coords_float(const RECT *r, unsigned int w, unsigned int h, struct wined3d_rect_f *f)
{
    f->l = ((r->left * 2.0f) / w) - 1.0f;
    f->t = ((r->top * 2.0f) / h) - 1.0f;
    f->r = ((r->right * 2.0f) / w) - 1.0f;
    f->b = ((r->bottom * 2.0f) / h) - 1.0f;
}

void texture2d_get_blt_info(const struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, const RECT *rect, struct wined3d_blt_info *info)
{
    struct wined3d_vec3 *coords = info->texcoords;
    struct wined3d_rect_f f;
    unsigned int level;
    GLenum target;
    GLsizei w, h;

    level = sub_resource_idx % texture_gl->t.level_count;
    w = wined3d_texture_get_level_width(&texture_gl->t, level);
    h = wined3d_texture_get_level_height(&texture_gl->t, level);
    target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);

    switch (target)
    {
        default:
            FIXME("Unsupported texture target %#x.\n", target);
            /* Fall back to GL_TEXTURE_2D */
        case GL_TEXTURE_2D:
            info->bind_target = GL_TEXTURE_2D;
            coords[0].x = (float)rect->left / w;
            coords[0].y = (float)rect->top / h;
            coords[0].z = 0.0f;

            coords[1].x = (float)rect->right / w;
            coords[1].y = (float)rect->top / h;
            coords[1].z = 0.0f;

            coords[2].x = (float)rect->left / w;
            coords[2].y = (float)rect->bottom / h;
            coords[2].z = 0.0f;

            coords[3].x = (float)rect->right / w;
            coords[3].y = (float)rect->bottom / h;
            coords[3].z = 0.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x =  1.0f;   coords[0].y = -f.t;   coords[0].z = -f.l;
            coords[1].x =  1.0f;   coords[1].y = -f.t;   coords[1].z = -f.r;
            coords[2].x =  1.0f;   coords[2].y = -f.b;   coords[2].z = -f.l;
            coords[3].x =  1.0f;   coords[3].y = -f.b;   coords[3].z = -f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = -1.0f;   coords[0].y = -f.t;   coords[0].z = f.l;
            coords[1].x = -1.0f;   coords[1].y = -f.t;   coords[1].z = f.r;
            coords[2].x = -1.0f;   coords[2].y = -f.b;   coords[2].z = f.l;
            coords[3].x = -1.0f;   coords[3].y = -f.b;   coords[3].z = f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y =  1.0f;   coords[0].z = f.t;
            coords[1].x = f.r;   coords[1].y =  1.0f;   coords[1].z = f.t;
            coords[2].x = f.l;   coords[2].y =  1.0f;   coords[2].z = f.b;
            coords[3].x = f.r;   coords[3].y =  1.0f;   coords[3].z = f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y = -1.0f;   coords[0].z = -f.t;
            coords[1].x = f.r;   coords[1].y = -1.0f;   coords[1].z = -f.t;
            coords[2].x = f.l;   coords[2].y = -1.0f;   coords[2].z = -f.b;
            coords[3].x = f.r;   coords[3].y = -1.0f;   coords[3].z = -f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y = -f.t;   coords[0].z =  1.0f;
            coords[1].x = f.r;   coords[1].y = -f.t;   coords[1].z =  1.0f;
            coords[2].x = f.l;   coords[2].y = -f.b;   coords[2].z =  1.0f;
            coords[3].x = f.r;   coords[3].y = -f.b;   coords[3].z =  1.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = -f.l;   coords[0].y = -f.t;   coords[0].z = -1.0f;
            coords[1].x = -f.r;   coords[1].y = -f.t;   coords[1].z = -1.0f;
            coords[2].x = -f.l;   coords[2].y = -f.b;   coords[2].z = -1.0f;
            coords[3].x = -f.r;   coords[3].y = -f.b;   coords[3].z = -1.0f;
            break;
    }
}

static void gltexture_delete(struct wined3d_device *device, const struct wined3d_gl_info *gl_info,
        struct gl_texture *tex)
{
    context_gl_resource_released(device, tex->name, FALSE);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex->name);
    tex->name = 0;
}

/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_gl_allocate_mutable_storage(struct wined3d_texture_gl *texture_gl,
        GLenum gl_internal_format, const struct wined3d_format_gl *format,
        const struct wined3d_gl_info *gl_info)
{
    unsigned int level_count, layer_count;
    GLsizei width, height, depth;
    GLenum target;

    level_count = texture_gl->t.level_count;
    if (texture_gl->target == GL_TEXTURE_1D_ARRAY || texture_gl->target == GL_TEXTURE_2D_ARRAY)
        layer_count = 1;
    else
        layer_count = texture_gl->t.layer_count;

    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    checkGLcall("glBindBuffer");

    for (unsigned int layer = 0; layer < layer_count; ++layer)
    {
        target = wined3d_texture_gl_get_sub_resource_target(texture_gl, layer * level_count);

        for (unsigned int level = 0; level < level_count; ++level)
        {
            width = wined3d_texture_get_level_width(&texture_gl->t, level);
            height = wined3d_texture_get_level_height(&texture_gl->t, level);
            if (texture_gl->t.resource.format_attrs & WINED3D_FORMAT_ATTR_HEIGHT_SCALE)
            {
                height *= format->f.height_scale.numerator;
                height /= format->f.height_scale.denominator;
            }

            TRACE("texture_gl %p, layer %u, level %u, target %#x, width %u, height %u.\n",
                    texture_gl, layer, level, target, width, height);

            if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY)
            {
                depth = wined3d_texture_get_level_depth(&texture_gl->t, level);
                GL_EXTCALL(glTexImage3D(target, level, gl_internal_format, width, height,
                        target == GL_TEXTURE_2D_ARRAY ? texture_gl->t.layer_count : depth, 0,
                        format->format, format->type, NULL));
                checkGLcall("glTexImage3D");
            }
            else if (target == GL_TEXTURE_1D)
            {
                gl_info->gl_ops.gl.p_glTexImage1D(target, level, gl_internal_format,
                        width, 0, format->format, format->type, NULL);
            }
            else
            {
                gl_info->gl_ops.gl.p_glTexImage2D(target, level, gl_internal_format, width,
                        target == GL_TEXTURE_1D_ARRAY ? texture_gl->t.layer_count : height, 0,
                        format->format, format->type, NULL);
                checkGLcall("glTexImage2D");
            }
        }
    }
}

/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_gl_allocate_immutable_storage(struct wined3d_texture_gl *texture_gl,
        GLenum gl_internal_format, const struct wined3d_gl_info *gl_info)
{
    unsigned int samples = wined3d_resource_get_sample_count(&texture_gl->t.resource);
    GLboolean standard_pattern = texture_gl->t.resource.multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
            && texture_gl->t.resource.multisample_quality == WINED3D_STANDARD_MULTISAMPLE_PATTERN;
    GLsizei height = wined3d_texture_get_level_height(&texture_gl->t, 0);
    GLsizei width = wined3d_texture_get_level_width(&texture_gl->t, 0);

    switch (texture_gl->target)
    {
        case GL_TEXTURE_3D:
            GL_EXTCALL(glTexStorage3D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height, wined3d_texture_get_level_depth(&texture_gl->t, 0)));
            break;
        case GL_TEXTURE_2D_ARRAY:
            GL_EXTCALL(glTexStorage3D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height, texture_gl->t.layer_count));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
            GL_EXTCALL(glTexStorage2DMultisample(texture_gl->target, samples,
                    gl_internal_format, width, height, standard_pattern));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            GL_EXTCALL(glTexStorage3DMultisample(texture_gl->target, samples,
                    gl_internal_format, width, height, texture_gl->t.layer_count, standard_pattern));
            break;
        case GL_TEXTURE_1D_ARRAY:
            GL_EXTCALL(glTexStorage2D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, texture_gl->t.layer_count));
            break;
        case GL_TEXTURE_1D:
            GL_EXTCALL(glTexStorage1D(texture_gl->target, texture_gl->t.level_count, gl_internal_format, width));
            break;
        default:
            GL_EXTCALL(glTexStorage2D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height));
            break;
    }

    checkGLcall("allocate immutable storage");
}

static void wined3d_texture_gl_upload_bo(const struct wined3d_format *src_format, GLenum target,
        unsigned int level, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z, unsigned int update_w,
        unsigned int update_h, unsigned int update_d, const BYTE *addr, bool srgb,
        struct wined3d_texture *dst_texture, const struct wined3d_gl_info *gl_info)
{
    const struct wined3d_format_gl *format_gl = wined3d_format_gl(src_format);

    if (src_format->attrs & WINED3D_FORMAT_ATTR_COMPRESSED)
    {
        GLenum internal = wined3d_gl_get_internal_format(&dst_texture->resource, format_gl, srgb);
        unsigned int dst_row_pitch, dst_slice_pitch;

        wined3d_format_calculate_pitch(src_format, 1, update_w, update_h, &dst_row_pitch, &dst_slice_pitch);

        TRACE("Uploading compressed data, target %#x, level %u, x %u, y %u, z %u, "
                "w %u, h %u, d %u, format %#x, image_size %#x, addr %p.\n",
                target, level, dst_x, dst_y, dst_z, update_w, update_h,
                update_d, internal, dst_slice_pitch, addr);

        if (target == GL_TEXTURE_1D)
        {
            GL_EXTCALL(glCompressedTexSubImage1D(target, level, dst_x,
                    update_w, internal, dst_row_pitch, addr));
        }
        else
        {
            unsigned int slice_count = 1, row_count = 1;

            /* glCompressedTexSubImage2D() ignores pixel store state, so we
             * can't use the unpack row length like for glTexSubImage2D. */
            if (dst_row_pitch != src_row_pitch)
            {
                row_count = (update_h + src_format->block_height - 1) / src_format->block_height;
                update_h = src_format->block_height;
                wined3d_format_calculate_pitch(src_format, 1, update_w, update_h,
                        &dst_row_pitch, &dst_slice_pitch);
            }

            if (dst_slice_pitch != src_slice_pitch)
            {
                slice_count = update_d;
                update_d = 1;
            }

            for (unsigned int slice = 0; slice < slice_count; ++slice)
            {
                for (unsigned int row = 0, y = dst_y; row < row_count; ++row)
                {
                    const BYTE *upload_addr = &addr[slice * src_slice_pitch + row * src_row_pitch];

                    if (target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_3D)
                    {
                        GL_EXTCALL(glCompressedTexSubImage3D(target, level, dst_x, y, dst_z + slice, update_w,
                                update_h, update_d, internal, update_d * dst_slice_pitch, upload_addr));
                    }
                    else
                    {
                        GL_EXTCALL(glCompressedTexSubImage2D(target, level, dst_x, y, update_w,
                                update_h, internal, dst_slice_pitch, upload_addr));
                    }

                    y += src_format->block_height;
                }
            }
        }
        checkGLcall("Upload compressed texture data");
    }
    else
    {
        unsigned int y_count, z_count;
        bool unpacking_rows = false;

        TRACE("Uploading data, target %#x, level %u, x %u, y %u, z %u, "
                "w %u, h %u, d %u, format %#x, type %#x, addr %p.\n",
                target, level, dst_x, dst_y, dst_z, update_w, update_h,
                update_d, format_gl->format, format_gl->type, addr);

        if (src_row_pitch && !(src_row_pitch % src_format->byte_count))
        {
            gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, src_row_pitch / src_format->byte_count);
            y_count = 1;
            unpacking_rows = true;
        }
        else
        {
            y_count = update_h;
            update_h = 1;
        }

        if (src_slice_pitch && unpacking_rows && !(src_slice_pitch % src_row_pitch))
        {
            gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, src_slice_pitch / src_row_pitch);
            z_count = 1;
        }
        else if (src_slice_pitch && !unpacking_rows && !(src_slice_pitch % (update_w * src_format->byte_count)))
        {
            gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,
                    src_slice_pitch / (update_w * src_format->byte_count));
            z_count = 1;
        }
        else
        {
            z_count = update_d;
            update_d = 1;
        }

        for (unsigned int z = 0; z < z_count; ++z)
        {
            for (unsigned int y = 0; y < y_count; ++y)
            {
                const BYTE *upload_addr = &addr[z * src_slice_pitch + y * src_row_pitch];
                if (target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_3D)
                {
                    GL_EXTCALL(glTexSubImage3D(target, level, dst_x, dst_y + y, dst_z + z, update_w,
                            update_h, update_d, format_gl->format, format_gl->type, upload_addr));
                }
                else if (target == GL_TEXTURE_1D)
                {
                    gl_info->gl_ops.gl.p_glTexSubImage1D(target, level, dst_x,
                            update_w, format_gl->format, format_gl->type, upload_addr);
                }
                else
                {
                    gl_info->gl_ops.gl.p_glTexSubImage2D(target, level, dst_x, dst_y + y,
                            update_w, update_h, format_gl->format, format_gl->type, upload_addr);
                }
            }
        }
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
        checkGLcall("Upload texture data");
    }
}

static const struct d3dfmt_alpha_fixup
{
    enum wined3d_format_id format_id, conv_format_id;
}
formats_src_alpha_fixup[] =
{
    {WINED3DFMT_B8G8R8X8_UNORM, WINED3DFMT_B8G8R8A8_UNORM},
    {WINED3DFMT_B5G5R5X1_UNORM, WINED3DFMT_B5G5R5A1_UNORM},
    {WINED3DFMT_B4G4R4X4_UNORM, WINED3DFMT_B4G4R4A4_UNORM},
};

static enum wined3d_format_id wined3d_get_alpha_fixup_format(enum wined3d_format_id format_id,
        const struct wined3d_format *dst_format)
{
    if (!(dst_format->attrs & WINED3D_FORMAT_ATTR_COMPRESSED) && !dst_format->alpha_size)
        return WINED3DFMT_UNKNOWN;

    for (unsigned int i = 0; i < ARRAY_SIZE(formats_src_alpha_fixup); ++i)
    {
        if (formats_src_alpha_fixup[i].format_id == format_id)
            return formats_src_alpha_fixup[i].conv_format_id;
    }

    return WINED3DFMT_UNKNOWN;
}

static void wined3d_fixup_alpha(const struct wined3d_format *format, const uint8_t *src,
        unsigned int src_row_pitch, uint8_t *dst, unsigned int dst_row_pitch,
        unsigned int width, unsigned int height)
{
    unsigned int byte_count, alpha_mask;

    byte_count = format->byte_count;
    alpha_mask = wined3d_mask_from_size(format->alpha_size) << format->alpha_offset;

    switch (byte_count)
    {
        case 2:
            for (unsigned int y = 0; y < height; ++y)
            {
                const uint16_t *src_row = (const uint16_t *)&src[y * src_row_pitch];
                uint16_t *dst_row = (uint16_t *)&dst[y * dst_row_pitch];

                for (unsigned int x = 0; x < width; ++x)
                    dst_row[x] = src_row[x] | alpha_mask;
            }
            break;

        case 4:
            for (unsigned int y = 0; y < height; ++y)
            {
                const uint32_t *src_row = (const uint32_t *)&src[y * src_row_pitch];
                uint32_t *dst_row = (uint32_t *)&dst[y * dst_row_pitch];

                for (unsigned int x = 0; x < width; ++x)
                    dst_row[x] = src_row[x] | alpha_mask;
            }
            break;

        default:
            ERR("Unsupported byte count %u.\n", byte_count);
            break;
    }
}

static void wined3d_texture_gl_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, uint32_t dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    enum wined3d_format_id alpha_fixup_format_id = WINED3DFMT_UNKNOWN;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int update_w = src_box->right - src_box->left;
    unsigned int update_h = src_box->bottom - src_box->top;
    unsigned int update_d = src_box->back - src_box->front;
    struct wined3d_bo_address bo;
    unsigned int level;
    bool srgb = false;
    BOOL decompress;
    GLenum target;

    TRACE("context %p, src_bo_addr %s, src_format %s, src_box %s, src_row_pitch %u, src_slice_pitch %u, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_x %u, dst_y %u, dst_z %u.\n",
            context, debug_const_bo_address(src_bo_addr), debug_d3dformat(src_format->id), debug_box(src_box),
            src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx,
            wined3d_debug_location(dst_location), dst_x, dst_y, dst_z);

    if (dst_location == WINED3D_LOCATION_TEXTURE_SRGB)
    {
        srgb = true;
    }
    else if (dst_location != WINED3D_LOCATION_TEXTURE_RGB)
    {
        FIXME("Unhandled location %s.\n", wined3d_debug_location(dst_location));
        return;
    }

    wined3d_texture_gl_bind_and_dirtify(wined3d_texture_gl(dst_texture), wined3d_context_gl(context), srgb);

    if (dst_texture->sub_resources[dst_sub_resource_idx].map_count)
    {
        WARN("Uploading a texture that is currently mapped, pinning sysmem.\n");
        dst_texture->resource.pin_sysmem = 1;
    }

    if (src_format->attrs & WINED3D_FORMAT_ATTR_HEIGHT_SCALE)
    {
        update_h *= src_format->height_scale.numerator;
        update_h /= src_format->height_scale.denominator;
    }

    target = wined3d_texture_gl_get_sub_resource_target(wined3d_texture_gl(dst_texture), dst_sub_resource_idx);
    level = dst_sub_resource_idx % dst_texture->level_count;

    switch (target)
    {
        case GL_TEXTURE_1D_ARRAY:
            dst_y = dst_sub_resource_idx / dst_texture->level_count;
            update_h = 1;
            break;
        case GL_TEXTURE_2D_ARRAY:
            dst_z = dst_sub_resource_idx / dst_texture->level_count;
            update_d = 1;
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            FIXME("Not supported for multisample textures.\n");
            return;
    }

    bo.buffer_object = src_bo_addr->buffer_object;
    bo.addr = (BYTE *)src_bo_addr->addr + src_box->front * src_slice_pitch;
    if (dst_texture->resource.format_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
    {
        bo.addr += (src_box->top / src_format->block_height) * src_row_pitch;
        bo.addr += (src_box->left / src_format->block_width) * src_format->block_byte_count;
    }
    else
    {
        bo.addr += src_box->top * src_row_pitch;
        bo.addr += src_box->left * src_format->byte_count;
    }

    decompress = (dst_texture->resource.format_caps & WINED3D_FORMAT_CAP_DECOMPRESS)
            || (src_format->decompress && src_format->id != dst_texture->resource.format->id);

    if (src_format->upload || decompress
            || (alpha_fixup_format_id = wined3d_get_alpha_fixup_format(src_format->id,
            dst_texture->resource.format)) != WINED3DFMT_UNKNOWN)
    {
        const struct wined3d_format *compressed_format = src_format;
        unsigned int dst_row_pitch, dst_slice_pitch;
        struct wined3d_format_gl f;
        void *converted_mem;
        BYTE *src_mem;

        if (decompress)
        {
            src_format = wined3d_resource_get_decompress_format(&dst_texture->resource);
        }
        else if (alpha_fixup_format_id != WINED3DFMT_UNKNOWN)
        {
            src_format = wined3d_get_format(context->device->adapter, alpha_fixup_format_id, 0);
            assert(!!src_format);
        }
        else
        {
            if (dst_texture->resource.format_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
                ERR("Converting a block-based format.\n");

            f = *wined3d_format_gl(src_format);
            f.f.byte_count = src_format->conv_byte_count;
            src_format = &f.f;
        }

        wined3d_format_calculate_pitch(src_format, 1, update_w, update_h, &dst_row_pitch, &dst_slice_pitch);

        if (!(converted_mem = malloc(dst_slice_pitch)))
        {
            ERR("Failed to allocate upload buffer.\n");
            return;
        }

        src_mem = wined3d_context_gl_map_bo_address(context_gl, &bo, src_slice_pitch * update_d, WINED3D_MAP_READ);

        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");

        for (unsigned int z = 0; z < update_d; ++z, src_mem += src_slice_pitch)
        {
            if (decompress)
                compressed_format->decompress(src_mem, converted_mem, src_row_pitch, src_slice_pitch,
                        dst_row_pitch, dst_slice_pitch, update_w, update_h, 1);
            else if (alpha_fixup_format_id != WINED3DFMT_UNKNOWN)
                wined3d_fixup_alpha(src_format, src_mem, src_row_pitch, converted_mem, dst_row_pitch,
                        update_w, update_h);
            else
                src_format->upload(src_mem, converted_mem, src_row_pitch, src_slice_pitch,
                        dst_row_pitch, dst_slice_pitch, update_w, update_h, 1);

            wined3d_texture_gl_upload_bo(src_format, target, level, dst_row_pitch, dst_slice_pitch, dst_x,
                    dst_y, dst_z + z, update_w, update_h, 1, converted_mem, srgb, dst_texture, gl_info);
        }

        wined3d_context_gl_unmap_bo_address(context_gl, &bo, 0, NULL);
        free(converted_mem);
    }
    else
    {
        const uint8_t *offset = bo.addr;

        if (bo.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, wined3d_bo_gl(bo.buffer_object)->id));
            checkGLcall("glBindBuffer");
            offset += bo.buffer_object->buffer_offset;
        }
        else
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            checkGLcall("glBindBuffer");
        }

        wined3d_texture_gl_upload_bo(src_format, target, level, src_row_pitch, src_slice_pitch, dst_x,
                dst_y, dst_z, update_w, update_h, update_d, offset, srgb, dst_texture, gl_info);

        if (bo.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            wined3d_context_gl_reference_bo(context_gl, wined3d_bo_gl(bo.buffer_object));
            checkGLcall("glBindBuffer");
        }
    }

    if (gl_info->quirks & WINED3D_QUIRK_FBO_TEX_UPDATE)
    {
        struct wined3d_device *device = dst_texture->resource.device;

        for (unsigned int i = 0; i < device->context_count; ++i)
            wined3d_context_gl_texture_update(wined3d_context_gl(device->contexts[i]), wined3d_texture_gl(dst_texture));
    }
}

static void wined3d_texture_gl_download_data_slow_path(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, const struct wined3d_bo_address *data)
{
    struct wined3d_bo_gl *bo = wined3d_bo_gl(data->buffer_object);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int dst_row_pitch, dst_slice_pitch;
    unsigned int src_row_pitch, src_slice_pitch;
    const struct wined3d_format_gl *format_gl;
    BYTE *temporary_mem = NULL;
    unsigned int level;
    GLenum target;
    void *mem;

    format_gl = wined3d_format_gl(texture_gl->t.resource.format);

    /* Only support read back of converted P8 textures. */
    if (texture_gl->t.flags & WINED3D_TEXTURE_CONVERTED && format_gl->f.id != WINED3DFMT_P8_UINT
            && !format_gl->f.download)
    {
        ERR("Trying to read back converted texture %p, %u with format %s.\n",
                texture_gl, sub_resource_idx, debug_d3dformat(format_gl->f.id));
        return;
    }

    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];
    target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);
    level = sub_resource_idx % texture_gl->t.level_count;

    if (target == GL_TEXTURE_1D_ARRAY || target == GL_TEXTURE_2D_ARRAY)
    {
        if (format_gl->f.download)
        {
            FIXME("Reading back converted array texture %p is not supported.\n", texture_gl);
            return;
        }

        WARN_(d3d_perf)("Downloading all miplevel layers to get the data for a single sub-resource.\n");

        if (!(temporary_mem = calloc(texture_gl->t.layer_count, sub_resource->size)))
        {
            ERR("Out of memory.\n");
            return;
        }
    }

    if (format_gl->f.download)
    {
        struct wined3d_format f;

        if (bo)
            ERR("Converted texture %p uses PBO unexpectedly.\n", texture_gl);

        WARN_(d3d_perf)("Downloading converted texture %p, %u with format %s.\n",
                texture_gl, sub_resource_idx, debug_d3dformat(format_gl->f.id));

        f = format_gl->f;
        f.byte_count = format_gl->f.conv_byte_count;
        wined3d_texture_get_pitch(&texture_gl->t, level, &dst_row_pitch, &dst_slice_pitch);
        wined3d_format_calculate_pitch(&f, texture_gl->t.resource.device->surface_alignment,
                wined3d_texture_get_level_width(&texture_gl->t, level),
                wined3d_texture_get_level_height(&texture_gl->t, level),
                &src_row_pitch, &src_slice_pitch);

        if (!(temporary_mem = malloc(src_slice_pitch)))
        {
            ERR("Failed to allocate memory.\n");
            return;
        }
    }

    if (temporary_mem)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
        mem = temporary_mem;
    }
    else if (bo)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, bo->id));
        checkGLcall("glBindBuffer");
        mem = (uint8_t *)data->addr + bo->b.buffer_offset;
    }
    else
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
        mem = data->addr;
    }

    if (texture_gl->t.resource.format_attrs & WINED3D_FORMAT_ATTR_COMPRESSED)
    {
        TRACE("Downloading compressed texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                texture_gl, sub_resource_idx, level, format_gl->format, format_gl->type, mem);

        GL_EXTCALL(glGetCompressedTexImage(target, level, mem));
        checkGLcall("glGetCompressedTexImage");
    }
    else
    {
        TRACE("Downloading texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                texture_gl, sub_resource_idx, level, format_gl->format, format_gl->type, mem);

        gl_info->gl_ops.gl.p_glGetTexImage(target, level, format_gl->format, format_gl->type, mem);
        checkGLcall("glGetTexImage");
    }

    if (format_gl->f.download)
    {
        format_gl->f.download(mem, data->addr, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
                wined3d_texture_get_level_width(&texture_gl->t, level),
                wined3d_texture_get_level_height(&texture_gl->t, level), 1);
    }
    else if (temporary_mem)
    {
        unsigned int layer = sub_resource_idx / texture_gl->t.level_count;
        void *src_data = temporary_mem + layer * sub_resource->size;
        if (bo)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, bo->id));
            checkGLcall("glBindBuffer");
            GL_EXTCALL(glBufferSubData(GL_PIXEL_PACK_BUFFER,
                    (GLintptr)data->addr + bo->b.buffer_offset, sub_resource->size, src_data));
            checkGLcall("glBufferSubData");
        }
        else
        {
            memcpy(data->addr, src_data, sub_resource->size);
        }
    }

    if (bo)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        wined3d_context_gl_reference_bo(context_gl, bo);
        checkGLcall("glBindBuffer");
    }

    free(temporary_mem);
}

static void wined3d_texture_gl_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int src_level, src_width, src_height, src_depth;
    unsigned int src_row_pitch, src_slice_pitch;
    const struct wined3d_format_gl *format_gl;
    uint8_t *offset = dst_bo_addr->addr;
    struct wined3d_bo *dst_bo;
    bool srgb = false;
    GLenum target;

    TRACE("context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_box %s, dst_bo_addr %s, "
            "dst_format %s, dst_x %u, dst_y %u, dst_z %u, dst_row_pitch %u, dst_slice_pitch %u.\n",
            context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            debug_box(src_box), debug_bo_address(dst_bo_addr), debug_d3dformat(dst_format->id),
            dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);

    if (src_location == WINED3D_LOCATION_TEXTURE_SRGB)
    {
        srgb = true;
    }
    else if (src_location != WINED3D_LOCATION_TEXTURE_RGB)
    {
        FIXME("Unhandled location %s.\n", wined3d_debug_location(src_location));
        return;
    }

    src_level = src_sub_resource_idx % src_texture->level_count;
    src_width = wined3d_texture_get_level_width(src_texture, src_level);
    src_height = wined3d_texture_get_level_height(src_texture, src_level);
    src_depth = wined3d_texture_get_level_depth(src_texture, src_level);
    if (src_box->left || src_box->top || src_box->right != src_width || src_box->bottom != src_height
            || src_box->front || src_box->back != src_depth)
    {
        FIXME("Unhandled source box %s.\n", debug_box(src_box));
        return;
    }

    if (dst_x || dst_y || dst_z)
    {
        FIXME("Unhandled destination (%u, %u, %u).\n", dst_x, dst_y, dst_z);
        return;
    }

    if (dst_format->id != src_texture->resource.format->id)
    {
        FIXME("Unhandled format conversion (%s -> %s).\n",
                debug_d3dformat(src_texture->resource.format->id),
                debug_d3dformat(dst_format->id));
        return;
    }

    wined3d_texture_get_pitch(src_texture, src_level, &src_row_pitch, &src_slice_pitch);
    if (src_row_pitch != dst_row_pitch || src_slice_pitch != dst_slice_pitch)
    {
        FIXME("Unhandled destination pitches %u/%u (source pitches %u/%u).\n",
                dst_row_pitch, dst_slice_pitch, src_row_pitch, src_slice_pitch);
        return;
    }

    wined3d_texture_gl_bind_and_dirtify(src_texture_gl, context_gl, srgb);

    format_gl = wined3d_format_gl(src_texture->resource.format);
    target = wined3d_texture_gl_get_sub_resource_target(src_texture_gl, src_sub_resource_idx);

    if ((src_texture->resource.type == WINED3D_RTYPE_TEXTURE_2D
            && (target == GL_TEXTURE_2D_ARRAY || format_gl->f.conv_byte_count
            || (src_texture->flags & WINED3D_TEXTURE_CONVERTED)))
            || target == GL_TEXTURE_1D_ARRAY)
    {
        wined3d_texture_gl_download_data_slow_path(src_texture_gl, src_sub_resource_idx, context_gl, dst_bo_addr);
        return;
    }

    if (format_gl->f.conv_byte_count)
    {
        FIXME("Attempting to download a converted texture, type %s format %s.\n",
                debug_d3dresourcetype(src_texture->resource.type),
                debug_d3dformat(format_gl->f.id));
        return;
    }

    if ((dst_bo = dst_bo_addr->buffer_object))
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, wined3d_bo_gl(dst_bo)->id));
        checkGLcall("glBindBuffer");
        offset += dst_bo->buffer_offset;
    }
    else
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    if (src_texture->resource.format_attrs & WINED3D_FORMAT_ATTR_COMPRESSED)
    {
        TRACE("Downloading compressed texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                src_texture, src_sub_resource_idx, src_level, format_gl->format, format_gl->type, offset);

        GL_EXTCALL(glGetCompressedTexImage(target, src_level, offset));
        checkGLcall("glGetCompressedTexImage");
    }
    else
    {
        TRACE("Downloading texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                src_texture, src_sub_resource_idx, src_level, format_gl->format, format_gl->type, offset);

        gl_info->gl_ops.gl.p_glGetTexImage(target, src_level, format_gl->format, format_gl->type, offset);
        checkGLcall("glGetTexImage");
    }

    if (dst_bo)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        wined3d_context_gl_reference_bo(context_gl, wined3d_bo_gl(dst_bo));
        checkGLcall("glBindBuffer");
    }
}

/* Context activation is done by the caller. */
static BOOL wined3d_texture_gl_load_sysmem(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, uint32_t dst_location)
{
    struct wined3d_texture_sub_resource *sub_resource;

    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];

    /* We cannot download data from multisample textures directly. */
    if (wined3d_texture_gl_is_multisample_location(texture_gl, WINED3D_LOCATION_TEXTURE_RGB)
            || sub_resource->locations & WINED3D_LOCATION_RB_MULTISAMPLE)
        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_RB_RESOLVED);

    if (sub_resource->locations & WINED3D_LOCATION_RB_RESOLVED)
    {
        texture2d_read_from_framebuffer(&texture_gl->t, sub_resource_idx, &context_gl->c,
                WINED3D_LOCATION_RB_RESOLVED, dst_location);
        return TRUE;
    }

    if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
    {
        unsigned int row_pitch, slice_pitch, level;
        struct wined3d_bo_address data;
        struct wined3d_box src_box;
        uint32_t src_location;

        level = sub_resource_idx % texture_gl->t.level_count;
        wined3d_texture_get_bo_address(&texture_gl->t, sub_resource_idx, &data, dst_location);
        src_location = sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB
                ? WINED3D_LOCATION_TEXTURE_RGB : WINED3D_LOCATION_TEXTURE_SRGB;
        wined3d_texture_get_level_box(&texture_gl->t, level, &src_box);
        wined3d_texture_get_pitch(&texture_gl->t, level, &row_pitch, &slice_pitch);
        wined3d_texture_gl_download_data(&context_gl->c, &texture_gl->t, sub_resource_idx, src_location,
                &src_box, &data, texture_gl->t.resource.format, 0, 0, 0, row_pitch, slice_pitch);

        ++texture_gl->t.download_count;
        return TRUE;
    }

    if (sub_resource->locations & WINED3D_LOCATION_DRAWABLE)
    {
        texture2d_read_from_framebuffer(&texture_gl->t, sub_resource_idx, &context_gl->c,
                texture_gl->t.resource.draw_binding, dst_location);
        return TRUE;
    }

    FIXME("Can't load texture %p, %u with location flags %s into sysmem.\n",
            texture_gl, sub_resource_idx, wined3d_debug_location(sub_resource->locations));

    return FALSE;
}

static BOOL wined3d_texture_load_drawable(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context)
{
    struct wined3d_device *device;
    unsigned int level;
    RECT r;

    if (texture->resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        uint32_t current = texture->sub_resources[sub_resource_idx].locations;
        FIXME("Unimplemented copy from %s for depth/stencil buffers.\n",
                wined3d_debug_location(current));
        return FALSE;
    }

    if (wined3d_resource_is_offscreen(&texture->resource))
    {
        ERR("Trying to load offscreen texture into WINED3D_LOCATION_DRAWABLE.\n");
        return FALSE;
    }

    device = texture->resource.device;
    level = sub_resource_idx % texture->level_count;
    SetRect(&r, 0, 0, wined3d_texture_get_level_width(texture, level),
            wined3d_texture_get_level_height(texture, level));
    wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    device->blitter->ops->blitter_blit(device->blitter, WINED3D_BLIT_OP_COLOR_BLIT, context,
            texture, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &r,
            texture, sub_resource_idx, WINED3D_LOCATION_DRAWABLE, &r,
            NULL, WINED3D_TEXF_POINT, NULL);

    return TRUE;
}

static BOOL wined3d_texture_load_renderbuffer(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t dst_location)
{
    unsigned int level = sub_resource_idx % texture->level_count;
    const RECT rect = {0, 0,
            wined3d_texture_get_level_width(texture, level),
            wined3d_texture_get_level_height(texture, level)};
    struct wined3d_texture_sub_resource *sub_resource;
    uint32_t src_location, locations;

    sub_resource = &texture->sub_resources[sub_resource_idx];
    locations = sub_resource->locations;
    if (texture->resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        FIXME("Unimplemented copy from %s for depth/stencil buffers.\n",
                wined3d_debug_location(locations));
        return FALSE;
    }

    if (locations & WINED3D_LOCATION_RB_MULTISAMPLE)
        src_location = WINED3D_LOCATION_RB_MULTISAMPLE;
    else if (locations & WINED3D_LOCATION_RB_RESOLVED)
        src_location = WINED3D_LOCATION_RB_RESOLVED;
    else if (locations & WINED3D_LOCATION_TEXTURE_SRGB)
        src_location = WINED3D_LOCATION_TEXTURE_SRGB;
    else if (locations & WINED3D_LOCATION_TEXTURE_RGB)
        src_location = WINED3D_LOCATION_TEXTURE_RGB;
    else if (locations & WINED3D_LOCATION_DRAWABLE)
        src_location = WINED3D_LOCATION_DRAWABLE;
    else /* texture2d_blt_fbo() will load the source location if necessary. */
        src_location = WINED3D_LOCATION_TEXTURE_RGB;

    texture2d_blt_fbo(texture->resource.device, context, WINED3D_TEXF_POINT, texture,
            sub_resource_idx, src_location, &rect, texture, sub_resource_idx, dst_location, &rect, NULL);

    return TRUE;
}

static BOOL wined3d_texture_gl_load_texture(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, bool srgb)
{
    struct wined3d_device *device = texture_gl->t.resource.device;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int level, src_row_pitch, src_slice_pitch;
    struct wined3d_texture_sub_resource *sub_resource;
    const struct wined3d_format *format;
    struct wined3d_bo_address data;
    struct wined3d_box src_box;
    uint32_t dst_location;
    bool depth;

    depth = texture_gl->t.resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL;
    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];
    level = sub_resource_idx % texture_gl->t.level_count;
    wined3d_texture_get_level_box(&texture_gl->t, level, &src_box);

    if (!depth && sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | WINED3D_LOCATION_TEXTURE_RGB)
            && (texture_gl->t.resource.format_caps & WINED3D_FORMAT_CAP_FBO_ATTACHABLE_SRGB)
            && fbo_blitter_supported(WINED3D_BLIT_OP_COLOR_BLIT, gl_info,
                    &texture_gl->t.resource, WINED3D_LOCATION_TEXTURE_RGB,
                    &texture_gl->t.resource, WINED3D_LOCATION_TEXTURE_SRGB))
    {
        RECT src_rect;

        SetRect(&src_rect, src_box.left, src_box.top, src_box.right, src_box.bottom);
        if (srgb)
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &src_rect,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect, NULL);
        else
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &src_rect, NULL);

        return TRUE;
    }

    if (!depth && sub_resource->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED)
            && (!srgb || (texture_gl->t.resource.format_caps & WINED3D_FORMAT_CAP_FBO_ATTACHABLE_SRGB)))
    {
        uint32_t src_location = sub_resource->locations & WINED3D_LOCATION_RB_RESOLVED ?
                WINED3D_LOCATION_RB_RESOLVED : WINED3D_LOCATION_RB_MULTISAMPLE;
        RECT src_rect;

        SetRect(&src_rect, src_box.left, src_box.top, src_box.right, src_box.bottom);
        dst_location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
        if (fbo_blitter_supported(WINED3D_BLIT_OP_COLOR_BLIT, gl_info,
                &texture_gl->t.resource, src_location, &texture_gl->t.resource, dst_location))
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT, &texture_gl->t, sub_resource_idx,
                    src_location, &src_rect, &texture_gl->t, sub_resource_idx, dst_location, &src_rect, NULL);

        return TRUE;
    }

    /* Upload from system memory */

    if (srgb)
    {
        dst_location = WINED3D_LOCATION_TEXTURE_SRGB;
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | texture_gl->t.resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_RGB)
        {
            FIXME_(d3d_perf)("Downloading RGB texture %p, %u to reload it as sRGB.\n", texture_gl, sub_resource_idx);
            wined3d_texture_load_location(&texture_gl->t, sub_resource_idx,
                    &context_gl->c, texture_gl->t.resource.map_binding);
        }
    }
    else
    {
        dst_location = WINED3D_LOCATION_TEXTURE_RGB;
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | texture_gl->t.resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_SRGB)
        {
            FIXME_(d3d_perf)("Downloading sRGB texture %p, %u to reload it as RGB.\n", texture_gl, sub_resource_idx);
            wined3d_texture_load_location(&texture_gl->t, sub_resource_idx,
                    &context_gl->c, texture_gl->t.resource.map_binding);
        }
    }

    if (!(sub_resource->locations & wined3d_texture_sysmem_locations))
    {
        WARN("Trying to load a texture from sysmem, but no simple location is valid.\n");
        /* Lets hope we get it from somewhere... */
        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_SYSMEM);
    }

    wined3d_texture_get_pitch(&texture_gl->t, level, &src_row_pitch, &src_slice_pitch);

    format = texture_gl->t.resource.format;

    wined3d_texture_get_memory(&texture_gl->t, sub_resource_idx, &context_gl->c, &data);
    wined3d_texture_gl_upload_data(&context_gl->c, wined3d_const_bo_address(&data), format, &src_box,
            src_row_pitch, src_slice_pitch, &texture_gl->t, sub_resource_idx, dst_location, 0, 0, 0);
    return TRUE;
}

static void wined3d_texture_gl_prepare_buffer_object(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_bo_gl *bo;

    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];

    if (sub_resource->bo)
        return;

    if (!(bo = malloc(sizeof(*bo))))
        return;

    if (!wined3d_device_gl_create_bo(wined3d_device_gl(texture_gl->t.resource.device),
            context_gl, sub_resource->size, GL_PIXEL_UNPACK_BUFFER, GL_STREAM_DRAW, true,
            GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT, bo))
    {
        free(bo);
        return;
    }

    TRACE("Created buffer object %u for texture %p, sub-resource %u.\n", bo->id, texture_gl, sub_resource_idx);
    sub_resource->bo = &bo->b;
}

static bool wined3d_texture_use_immutable_storage(const struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info)
{
    /* We don't expect to create texture views for textures with height-scaled formats.
     * Besides, ARB_texture_storage doesn't allow specifying exact sizes for all levels. */
    return gl_info->supported[ARB_TEXTURE_STORAGE]
            && !(texture->resource.format_attrs & WINED3D_FORMAT_ATTR_HEIGHT_SCALE);
}

void wined3d_texture_gl_prepare_texture(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, bool srgb)
{
    uint32_t alloc_flag = srgb ? WINED3D_TEXTURE_SRGB_ALLOCATED : WINED3D_TEXTURE_RGB_ALLOCATED;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_resource *resource = &texture_gl->t.resource;
    const struct wined3d_format *format = resource->format;
    const struct wined3d_format_gl *format_gl;
    GLenum internal;

    TRACE("texture_gl %p, context_gl %p, srgb %d, format %s.\n",
            texture_gl, context_gl, srgb, debug_d3dformat(format->id));

    if (texture_gl->t.flags & alloc_flag)
        return;

    if (resource->format_caps & WINED3D_FORMAT_CAP_DECOMPRESS)
    {
        TRACE("WINED3D_FORMAT_CAP_DECOMPRESS set.\n");
        texture_gl->t.flags |= WINED3D_TEXTURE_CONVERTED;
        format = wined3d_resource_get_decompress_format(resource);
    }
    else if (format->conv_byte_count)
    {
        texture_gl->t.flags |= WINED3D_TEXTURE_CONVERTED;
    }
    format_gl = wined3d_format_gl(format);

    wined3d_texture_gl_bind_and_dirtify(texture_gl, context_gl, srgb);

    internal = wined3d_gl_get_internal_format(resource, format_gl, srgb);
    if (!internal)
        FIXME("No GL internal format for format %s.\n", debug_d3dformat(format->id));

    TRACE("internal %#x, format %#x, type %#x.\n", internal, format_gl->format, format_gl->type);

    if (wined3d_texture_use_immutable_storage(&texture_gl->t, gl_info))
        wined3d_texture_gl_allocate_immutable_storage(texture_gl, internal, gl_info);
    else
        wined3d_texture_gl_allocate_mutable_storage(texture_gl, internal, format_gl, gl_info);
    texture_gl->t.flags |= alloc_flag;
}

static void wined3d_texture_gl_prepare_rb(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_gl_info *gl_info, bool multisample)
{
    const struct wined3d_format_gl *format_gl;

    format_gl = wined3d_format_gl(texture_gl->t.resource.format);
    if (multisample)
    {
        if (texture_gl->rb_multisample)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture_gl->rb_multisample);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture_gl->rb_multisample);
        gl_info->fbo_ops.glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                wined3d_resource_get_sample_count(&texture_gl->t.resource),
                format_gl->internal, texture_gl->t.resource.width, texture_gl->t.resource.height);
        checkGLcall("glRenderbufferStorageMultisample()");
        TRACE("Created multisample rb %u.\n", texture_gl->rb_multisample);
    }
    else
    {
        if (texture_gl->rb_resolved)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture_gl->rb_resolved);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture_gl->rb_resolved);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format_gl->internal,
                texture_gl->t.resource.width, texture_gl->t.resource.height);
        checkGLcall("glRenderbufferStorage()");
        TRACE("Created resolved rb %u.\n", texture_gl->rb_resolved);
    }
}

static BOOL wined3d_texture_gl_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return texture->sub_resources[sub_resource_idx].user_memory ? TRUE
                    : wined3d_resource_prepare_sysmem(&texture->resource);

        case WINED3D_LOCATION_BUFFER:
            wined3d_texture_gl_prepare_buffer_object(texture_gl, sub_resource_idx, context_gl);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_RGB:
            wined3d_texture_gl_prepare_texture(texture_gl, context_gl, false);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            wined3d_texture_gl_prepare_texture(texture_gl, context_gl, true);
            return TRUE;

        case WINED3D_LOCATION_DRAWABLE:
            if (!texture->swapchain)
                ERR("Texture %p does not have a drawable.\n", texture);
            return TRUE;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            wined3d_texture_gl_prepare_rb(texture_gl, context_gl->gl_info, true);
            return TRUE;

        case WINED3D_LOCATION_RB_RESOLVED:
            wined3d_texture_gl_prepare_rb(texture_gl, context_gl->gl_info, false);
            return TRUE;

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static bool use_ffp_clear(const struct wined3d_texture *texture, unsigned int location)
{
    if (location == WINED3D_LOCATION_DRAWABLE)
        return true;

    if (location == WINED3D_LOCATION_TEXTURE_RGB
            && !(texture->resource.format_caps & WINED3D_FORMAT_CAP_FBO_ATTACHABLE))
        return false;
    if (location == WINED3D_LOCATION_TEXTURE_SRGB
            && !(texture->resource.format_caps & WINED3D_FORMAT_CAP_FBO_ATTACHABLE_SRGB))
        return false;

    return location & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED
            | WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
}

static bool wined3d_texture_gl_clear(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, unsigned int location)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_bo_address addr;

    /* The code that delays clears is Vulkan-specific, so here we should only
     * encounter WINED3D_LOCATION_CLEARED on newly created resources and thus
     * a zero clear value. */
    if (!format->depth_size && !format->stencil_size)
    {
        if (sub_resource->clear_value.colour.r || sub_resource->clear_value.colour.g
                || sub_resource->clear_value.colour.b || sub_resource->clear_value.colour.a)
        {
            ERR("Unexpected color clear value r=%08e, g=%08e, b=%08e, a=%08e.\n",
                    sub_resource->clear_value.colour.r, sub_resource->clear_value.colour.g,
                    sub_resource->clear_value.colour.b, sub_resource->clear_value.colour.a);
        }
    }
    else
    {
        if (format->depth_size && sub_resource->clear_value.depth)
            ERR("Unexpected depth clear value %08e.\n", sub_resource->clear_value.depth);
        if (format->stencil_size && sub_resource->clear_value.stencil)
            ERR("Unexpected stencil clear value %x.\n", sub_resource->clear_value.stencil);
    }

    if (use_ffp_clear(texture, location))
    {
        GLbitfield clear_mask = 0;

        context_gl_apply_texture_draw_state(context_gl, texture, sub_resource_idx, location);

        gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
        context_invalidate_state(&context_gl->c, STATE_RASTERIZER);

        if (format->depth_size)
        {
            gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
            context_invalidate_state(&context_gl->c, STATE_DEPTH_STENCIL);

            if (gl_info->supported[ARB_ES2_COMPATIBILITY])
                GL_EXTCALL(glClearDepthf(0.0f));
            else
                gl_info->gl_ops.gl.p_glClearDepth(0.0);
            clear_mask |= GL_DEPTH_BUFFER_BIT;
        }

        if (format->stencil_size)
        {
            if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
                gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            gl_info->gl_ops.gl.p_glStencilMask(~0u);
            context_invalidate_state(&context_gl->c, STATE_DEPTH_STENCIL);
            gl_info->gl_ops.gl.p_glClearStencil(0);
            clear_mask |= GL_STENCIL_BUFFER_BIT;
        }

        if (!format->depth_size && !format->stencil_size)
        {
            gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            context_invalidate_state(&context_gl->c, STATE_BLEND);
            gl_info->gl_ops.gl.p_glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            clear_mask |= GL_COLOR_BUFFER_BIT;
        }

        gl_info->gl_ops.gl.p_glClear(clear_mask);
        checkGLcall("clear texture");

        wined3d_texture_validate_location(texture, sub_resource_idx, location);
        return true;
    }

    if (!wined3d_texture_prepare_location(texture, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_SYSMEM))
        return false;
    wined3d_texture_get_bo_address(texture, sub_resource_idx, &addr, WINED3D_LOCATION_SYSMEM);
    memset(addr.addr, 0, sub_resource->size);
    wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_SYSMEM);
    return true;
}

/* Context activation is done by the caller. */
static BOOL wined3d_texture_gl_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t location)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);

    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    if (!wined3d_texture_gl_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    if (sub_resource->locations & WINED3D_LOCATION_CLEARED)
    {
        if (!wined3d_texture_gl_clear(texture, sub_resource_idx, context_gl, location))
            return FALSE;

        if (sub_resource->locations & location)
            return TRUE;
    }

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_BUFFER:
            return wined3d_texture_gl_load_sysmem(texture_gl, sub_resource_idx, context_gl, location);

        case WINED3D_LOCATION_DRAWABLE:
            return wined3d_texture_load_drawable(texture, sub_resource_idx, context);

        case WINED3D_LOCATION_RB_RESOLVED:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
            return wined3d_texture_load_renderbuffer(texture, sub_resource_idx, context, location);

        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            return wined3d_texture_gl_load_texture(texture_gl, sub_resource_idx,
                    context_gl, location == WINED3D_LOCATION_TEXTURE_SRGB);

        default:
            FIXME("Unhandled %s load from %s.\n", wined3d_debug_location(location),
                    wined3d_debug_location(texture->sub_resources[sub_resource_idx].locations));
            return FALSE;
    }
}

static void wined3d_texture_remove_buffer_object(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    struct wined3d_bo_gl *bo_gl = wined3d_bo_gl(sub_resource->bo);

    TRACE("texture %p, sub_resource_idx %u, context_gl %p.\n", texture, sub_resource_idx, context_gl);

    wined3d_context_gl_destroy_bo(context_gl, bo_gl);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_BUFFER);
    sub_resource->bo = NULL;
    free(bo_gl);
}

static void wined3d_texture_gl_unload_location(struct wined3d_texture *texture,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_renderbuffer_entry *entry, *entry2;
    unsigned int sub_count;

    TRACE("texture %p, context %p, location %s.\n", texture, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_BUFFER:
            sub_count = texture->level_count * texture->layer_count;
            for (unsigned int i = 0; i < sub_count; ++i)
            {
                if (texture_gl->t.sub_resources[i].bo)
                    wined3d_texture_remove_buffer_object(&texture_gl->t, i, context_gl);
            }
            break;

        case WINED3D_LOCATION_TEXTURE_RGB:
            if (texture_gl->texture_rgb.name)
                gltexture_delete(texture_gl->t.resource.device, context_gl->gl_info, &texture_gl->texture_rgb);
            break;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (texture_gl->texture_srgb.name)
                gltexture_delete(texture_gl->t.resource.device, context_gl->gl_info, &texture_gl->texture_srgb);
            break;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            if (texture_gl->rb_multisample)
            {
                TRACE("Deleting multisample renderbuffer %u.\n", texture_gl->rb_multisample);
                context_gl_resource_released(texture_gl->t.resource.device, texture_gl->rb_multisample, TRUE);
                context_gl->gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture_gl->rb_multisample);
                texture_gl->rb_multisample = 0;
            }
            break;

        case WINED3D_LOCATION_RB_RESOLVED:
            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &texture_gl->renderbuffers,
                    struct wined3d_renderbuffer_entry, entry)
            {
                context_gl_resource_released(texture_gl->t.resource.device, entry->id, TRUE);
                context_gl->gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
                list_remove(&entry->entry);
                free(entry);
            }
            list_init(&texture_gl->renderbuffers);
            texture_gl->current_renderbuffer = NULL;

            if (texture_gl->rb_resolved)
            {
                TRACE("Deleting resolved renderbuffer %u.\n", texture_gl->rb_resolved);
                context_gl_resource_released(texture_gl->t.resource.device, texture_gl->rb_resolved, TRUE);
                context_gl->gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture_gl->rb_resolved);
                texture_gl->rb_resolved = 0;
            }
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

static const struct wined3d_texture_ops texture_gl_ops =
{
    wined3d_texture_gl_prepare_location,
    wined3d_texture_gl_load_location,
    wined3d_texture_gl_unload_location,
    wined3d_texture_gl_upload_data,
    wined3d_texture_gl_download_data,
};

HRESULT wined3d_texture_gl_init(struct wined3d_texture_gl *texture_gl, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &wined3d_adapter_gl(device->adapter)->gl_info;
    HRESULT hr;

    TRACE("texture_gl %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_gl, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    if (!(desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP) && layer_count > 1
            && !gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        WARN("OpenGL implementation does not support array textures.\n");
        return WINED3DERR_INVALIDCALL;
    }

    switch (desc->resource_type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
            if (layer_count > 1)
                texture_gl->target = GL_TEXTURE_1D_ARRAY;
            else
                texture_gl->target = GL_TEXTURE_1D;
            break;

        case WINED3D_RTYPE_TEXTURE_2D:
            if (desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            {
                texture_gl->target = GL_TEXTURE_CUBE_MAP_ARB;
            }
            else if (desc->multisample_type && gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
            {
                if (layer_count > 1)
                    texture_gl->target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                else
                    texture_gl->target = GL_TEXTURE_2D_MULTISAMPLE;
            }
            else
            {
                if (layer_count > 1)
                    texture_gl->target = GL_TEXTURE_2D_ARRAY;
                else
                    texture_gl->target = GL_TEXTURE_2D;
            }
            break;

        case WINED3D_RTYPE_TEXTURE_3D:
            if (!gl_info->supported[EXT_TEXTURE3D])
            {
                WARN("OpenGL implementation does not support 3D textures.\n");
                return WINED3DERR_INVALIDCALL;
            }
            texture_gl->target = GL_TEXTURE_3D;
            break;

        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(desc->resource_type));
            return WINED3DERR_INVALIDCALL;
    }

    list_init(&texture_gl->renderbuffers);

    if (FAILED(hr = wined3d_texture_init(&texture_gl->t, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_gl[1], &texture_gl_ops)))
        return hr;

    if (texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE)
        texture_gl->t.flags &= ~WINED3D_TEXTURE_DOWNLOADABLE;

    return WINED3D_OK;
}

void wined3d_gl_texture_swizzle_from_color_fixup(GLint swizzle[4], struct color_fixup_desc fixup)
{
    static const GLenum swizzle_source[] =
    {
        GL_ZERO,  /* CHANNEL_SOURCE_ZERO */
        GL_ONE,   /* CHANNEL_SOURCE_ONE */
        GL_RED,   /* CHANNEL_SOURCE_X */
        GL_GREEN, /* CHANNEL_SOURCE_Y */
        GL_BLUE,  /* CHANNEL_SOURCE_Z */
        GL_ALPHA, /* CHANNEL_SOURCE_W */
    };

    swizzle[0] = swizzle_source[fixup.x_source];
    swizzle[1] = swizzle_source[fixup.y_source];
    swizzle[2] = swizzle_source[fixup.z_source];
    swizzle[3] = swizzle_source[fixup.w_source];
}

/* Context activation is done by the caller. */
void wined3d_texture_gl_bind(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb)
{
    const struct wined3d_format *format = texture_gl->t.resource.format;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct color_fixup_desc fixup = format->color_fixup;
    struct gl_texture *gl_tex;
    GLenum target;

    TRACE("texture_gl %p, context_gl %p, srgb %#x.\n", texture_gl, context_gl, srgb);

    if (!needs_separate_srgb_gl_texture(&context_gl->c, &texture_gl->t))
        srgb = FALSE;

    /* sRGB mode cache for preload() calls outside drawprim. */
    if (srgb)
        texture_gl->t.flags |= WINED3D_TEXTURE_IS_SRGB;
    else
        texture_gl->t.flags &= ~WINED3D_TEXTURE_IS_SRGB;

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, srgb);
    target = texture_gl->target;

    if (gl_tex->name)
    {
        wined3d_context_gl_bind_texture(context_gl, target, gl_tex->name);
        return;
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &gl_tex->name);
    checkGLcall("glGenTextures");
    TRACE("Generated texture %d.\n", gl_tex->name);

    if (!gl_tex->name)
    {
        ERR("Failed to generate a texture name.\n");
        return;
    }

    /* Initialise the state of the texture object to the OpenGL defaults, not
     * the wined3d defaults. */
    gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_w = WINED3D_TADDRESS_WRAP;
    memset(gl_tex->sampler_desc.border_color, 0, sizeof(gl_tex->sampler_desc.border_color));
    gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_LINEAR;
    gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.lod_bias = 0.0f;
    gl_tex->sampler_desc.min_lod = -1000.0f;
    gl_tex->sampler_desc.max_lod = 1000.0f;
    gl_tex->sampler_desc.max_anisotropy = 1;
    gl_tex->sampler_desc.reduction_mode = WINED3D_FILTER_REDUCTION_WEIGHTED_AVERAGE;
    gl_tex->sampler_desc.comparison_func = WINED3D_CMP_LESSEQUAL;
    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_tex->sampler_desc.srgb_decode = TRUE;
    else
        gl_tex->sampler_desc.srgb_decode = srgb;
    gl_tex->sampler_desc.mip_base_level = 0;
    wined3d_texture_set_dirty(&texture_gl->t);

    wined3d_context_gl_bind_texture(context_gl, target, gl_tex->name);

    /* For a new texture we have to set the texture levels after binding the
     * texture. */
    TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", texture_gl->t.level_count - 1);
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture_gl->t.level_count - 1);
    checkGLcall("glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count)");

    if (target == GL_TEXTURE_CUBE_MAP_ARB)
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2 && target != GL_TEXTURE_2D_MULTISAMPLE
            && target != GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
    {
        /* Conditional non power of two textures use a different clamping
         * default. If we're using the GL_WINE_normalized_texrect partial
         * driver emulation, we're dealing with a GL_TEXTURE_2D texture which
         * has the address mode set to repeat - something that prevents us
         * from hitting the accelerated codepath. Thus manually set the GL
         * state. The same applies to filtering. Even if the texture has only
         * one mip level, the default LINEAR_MIPMAP_LINEAR filter causes a SW
         * fallback on macos. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    }

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] && gl_info->supported[ARB_DEPTH_TEXTURE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
        checkGLcall("glTexParameteri(GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY)");
    }

    if (!is_identity_fixup(fixup) && can_use_texture_swizzle(context_gl->c.d3d_info, format))
    {
        GLint swizzle[4];

        wined3d_gl_texture_swizzle_from_color_fixup(swizzle, fixup);
        gl_info->gl_ops.gl.p_glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        checkGLcall("set format swizzle");
    }
}

/* Context activation is done by the caller. */
void wined3d_texture_gl_bind_and_dirtify(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb)
{
    /* FIXME: Ideally we'd only do this when touching a binding that's used by
     * a shader. */
    context_invalidate_compute_state(&context_gl->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    wined3d_texture_gl_bind(texture_gl, context_gl, srgb);
}

/* Context activation is done by the caller (state handler). */
/* This function relies on the correct texture being bound and loaded. */
void wined3d_texture_gl_apply_sampler_desc(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum target = texture_gl->target;
    struct gl_texture *gl_tex;
    unsigned int state;

    TRACE("texture_gl %p, sampler_desc %p, context_gl %p.\n", texture_gl, sampler_desc, context_gl);

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);

    state = sampler_desc->address_u;
    if (state != gl_tex->sampler_desc.address_u)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_u = state;
    }

    state = sampler_desc->address_v;
    if (state != gl_tex->sampler_desc.address_v)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_v = state;
    }

    state = sampler_desc->address_w;
    if (state != gl_tex->sampler_desc.address_w)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_w = state;
    }

    if (memcmp(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
            sizeof(gl_tex->sampler_desc.border_color)))
    {
        gl_info->gl_ops.gl.p_glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &sampler_desc->border_color[0]);
        memcpy(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
                sizeof(gl_tex->sampler_desc.border_color));
    }

    state = sampler_desc->mag_filter;
    if (state != gl_tex->sampler_desc.mag_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(state));
        gl_tex->sampler_desc.mag_filter = state;
    }

    if (sampler_desc->min_filter != gl_tex->sampler_desc.min_filter
            || sampler_desc->mip_filter != gl_tex->sampler_desc.mip_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                wined3d_gl_min_mip_filter(sampler_desc->min_filter, sampler_desc->mip_filter));
        gl_tex->sampler_desc.min_filter = sampler_desc->min_filter;
        gl_tex->sampler_desc.mip_filter = sampler_desc->mip_filter;
    }

    state = sampler_desc->max_anisotropy;
    if (state != gl_tex->sampler_desc.max_anisotropy)
    {
        if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY, state);
        else
            WARN("Anisotropic filtering not supported.\n");
        gl_tex->sampler_desc.max_anisotropy = state;
    }

    if (!sampler_desc->srgb_decode != !gl_tex->sampler_desc.srgb_decode
            && (context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                sampler_desc->srgb_decode ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT);
        gl_tex->sampler_desc.srgb_decode = sampler_desc->srgb_decode;
    }

    if (sampler_desc->reduction_mode != gl_tex->sampler_desc.reduction_mode)
    {
        GLenum reduction_mode = wined3d_gl_filter_reduction_mode(sampler_desc->reduction_mode);

        if (sampler_desc->reduction_mode == WINED3D_FILTER_REDUCTION_COMPARISON)
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        else
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);

        if (gl_info->supported[ARB_TEXTURE_FILTER_MINMAX])
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_REDUCTION_MODE_ARB, reduction_mode);
        else if (reduction_mode != GL_WEIGHTED_AVERAGE_ARB)
            WARN("Sampler min/max reduction filtering is not supported.\n");

        gl_tex->sampler_desc.reduction_mode = sampler_desc->reduction_mode;
    }

    checkGLcall("Texture parameter application");

    if (gl_info->supported[EXT_TEXTURE_LOD_BIAS])
    {
        gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                GL_TEXTURE_LOD_BIAS_EXT, sampler_desc->lod_bias);
        checkGLcall("glTexEnvf(GL_TEXTURE_LOD_BIAS_EXT, ...)");
    }
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
/* Context activation is done by the caller. */
void wined3d_texture_gl_set_compatible_renderbuffer(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, unsigned int level, const struct wined3d_rendertarget_info *rt)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_renderbuffer_entry *entry;
    unsigned int src_width, src_height;
    unsigned int width, height;
    GLuint renderbuffer = 0;

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
        return;

    if (rt && rt->resource->format->id != WINED3DFMT_NULL)
    {
        struct wined3d_texture *rt_texture;
        unsigned int rt_level;

        if (rt->resource->type == WINED3D_RTYPE_BUFFER)
        {
            FIXME("Unsupported resource type %s.\n", debug_d3dresourcetype(rt->resource->type));
            return;
        }
        rt_texture = wined3d_texture_from_resource(rt->resource);
        rt_level = rt->sub_resource_idx % rt_texture->level_count;

        width = wined3d_texture_get_level_width(rt_texture, rt_level);
        height = wined3d_texture_get_level_height(rt_texture, rt_level);
    }
    else
    {
        width = wined3d_texture_get_level_width(&texture_gl->t, level);
        height = wined3d_texture_get_level_height(&texture_gl->t, level);
    }

    src_width = wined3d_texture_get_level_width(&texture_gl->t, level);
    src_height = wined3d_texture_get_level_height(&texture_gl->t, level);

    /* A depth stencil smaller than the render target is not valid */
    if (width > src_width || height > src_height)
        return;

    /* Remove any renderbuffer set if the sizes match */
    if (width == src_width && height == src_height)
    {
        texture_gl->current_renderbuffer = NULL;
        return;
    }

    /* Look if we've already got a renderbuffer of the correct dimensions */
    LIST_FOR_EACH_ENTRY(entry, &texture_gl->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        if (entry->width == width && entry->height == height)
        {
            renderbuffer = entry->id;
            texture_gl->current_renderbuffer = entry;
            break;
        }
    }

    if (!renderbuffer)
    {
        const struct wined3d_format_gl *format_gl;

        format_gl = wined3d_format_gl(texture_gl->t.resource.format);
        gl_info->fbo_ops.glGenRenderbuffers(1, &renderbuffer);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format_gl->internal, width, height);

        entry = malloc(sizeof(*entry));
        entry->width = width;
        entry->height = height;
        entry->id = renderbuffer;
        list_add_head(&texture_gl->renderbuffers, &entry->entry);

        texture_gl->current_renderbuffer = entry;
    }

    checkGLcall("set compatible renderbuffer");
}
