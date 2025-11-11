/*
 * Copyright 2020-2021 Henri Verbeet for CodeWeavers
 * Copyright 2021 Jan Sikorski for CodeWeavers
 * Copyright 2022 Stefan Dösinger for CodeWeavers
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
#include "wined3d_vk.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static void vk_blitter_destroy(struct wined3d_blitter *blitter, struct wined3d_context *context)
{
    struct wined3d_blitter *next;

    TRACE("blitter %p, context %p.\n", blitter, context);

    if ((next = blitter->next))
        next->ops->blitter_destroy(next, context);

    free(blitter);
}

static void vk_blitter_clear_rendertargets(struct wined3d_context_vk *context_vk, unsigned int rt_count,
        const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects, const RECT *draw_rect,
        uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    unsigned int i, attachment_count, immediate_rt_count = 0, delay_count = 0;
    VkClearValue clear_values[WINED3D_MAX_RENDER_TARGETS + 1];
    VkImageView views[WINED3D_MAX_RENDER_TARGETS + 1];
    struct wined3d_rendertarget_view_vk *rtv_vk;
    struct wined3d_rendertarget_view *view;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_fb_state immediate_fb;
    struct wined3d_device_vk *device_vk;
    VkCommandBuffer vk_command_buffer;
    VkRenderPassBeginInfo begin_desc;
    VkFramebufferCreateInfo fb_desc;
    VkFramebuffer vk_framebuffer;
    VkRenderPass vk_render_pass;
    bool depth_stencil = false;
    unsigned int layer_count;
    VkClearColorValue *c;
    VkResult vr;
    RECT r;

    TRACE("context_vk %p, rt_count %u, fb %p, rect_count %u, clear_rects %p, "
            "draw_rect %s, flags %#x, colour %s, depth %.8e, stencil %#x.\n",
            context_vk, rt_count, fb, rect_count, clear_rects,
            wine_dbgstr_rect(draw_rect), flags, debug_color(colour), depth, stencil);

    device_vk = wined3d_device_vk(context_vk->c.device);
    vk_info = context_vk->vk_info;

    if (!(flags & WINED3DCLEAR_TARGET))
        rt_count = 0;

    for (i = 0, attachment_count = 0, layer_count = 1; i < rt_count; ++i)
    {
        if (!(view = fb->render_targets[i]))
            continue;

        /* Don't delay typeless clears because the data written into the resource depends on the
         * view format. Except all-zero clears, those should result in zeros in either case.
         *
         * We could store the clear format along with the clear value, but then we'd have to
         * create a matching RTV at draw time, which would need its own render pass, thus mooting
         * the point of the delayed clear. (Unless we are lucky enough that the application
         * draws with the same RTV as it clears.) */
        if (wined3d_rendertarget_view_is_full_clear(view, draw_rect, clear_rects)
                && (!wined3d_format_is_typeless(view->resource->format) || (!colour->r && !colour->g
                && !colour->b && !colour->a)))
        {
            struct wined3d_texture *texture = texture_from_resource(view->resource);
            wined3d_rendertarget_view_validate_location(view, WINED3D_LOCATION_CLEARED);
            wined3d_rendertarget_view_invalidate_location(view, ~WINED3D_LOCATION_CLEARED);
            texture->sub_resources[view->sub_resource_idx].clear_value.colour = *colour;
            delay_count++;
            continue;
        }
        else
        {
            wined3d_rendertarget_view_load_location(view, &context_vk->c, view->resource->draw_binding);
        }
        wined3d_rendertarget_view_validate_location(view, view->resource->draw_binding);
        wined3d_rendertarget_view_invalidate_location(view, ~view->resource->draw_binding);

        immediate_fb.render_targets[immediate_rt_count++] = view;
        rtv_vk = wined3d_rendertarget_view_vk(view);
        views[attachment_count] = wined3d_rendertarget_view_vk_get_image_view(rtv_vk, context_vk);
        wined3d_rendertarget_view_vk_barrier(rtv_vk, context_vk, WINED3D_BIND_RENDER_TARGET);

        c = &clear_values[attachment_count].color;
        wined3d_format_colour_to_vk(view->format, colour, c);

        if (view->layer_count > layer_count)
            layer_count = view->layer_count;

        ++attachment_count;
    }

    if (!attachment_count)
        flags &= ~WINED3DCLEAR_TARGET;

    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL) && (view = fb->depth_stencil))
    {
        DWORD full_flags = 0;

        /* Vulkan can clear only depth or stencil, but at the moment we can't put the depth and
         * stencil parts in separate locations. It isn't easy to do either, as such a half-cleared
         * texture would need to be handled not just as a DS target but also when used as a shader
         * resource or accessed on sysmem. */
        if (view->format->depth_size)
            full_flags = WINED3DCLEAR_ZBUFFER;
        if (view->format->stencil_size)
            full_flags |= WINED3DCLEAR_STENCIL;

        if (!wined3d_rendertarget_view_is_full_clear(view, draw_rect, clear_rects) || (flags & full_flags) != full_flags
                || (wined3d_format_is_typeless(view->resource->format) && (depth || stencil)))
        {
            wined3d_rendertarget_view_load_location(view, &context_vk->c, view->resource->draw_binding);
            wined3d_rendertarget_view_validate_location(view, view->resource->draw_binding);
            wined3d_rendertarget_view_invalidate_location(view, ~view->resource->draw_binding);

            immediate_fb.depth_stencil = view;
            rtv_vk = wined3d_rendertarget_view_vk(view);
            views[attachment_count] = wined3d_rendertarget_view_vk_get_image_view(rtv_vk, context_vk);
            wined3d_rendertarget_view_vk_barrier(rtv_vk, context_vk, WINED3D_BIND_DEPTH_STENCIL);

            clear_values[attachment_count].depthStencil.depth = depth;
            clear_values[attachment_count].depthStencil.stencil = stencil;

            if (view->layer_count > layer_count)
                layer_count = view->layer_count;

            depth_stencil = true;
            ++attachment_count;
        }
        else
        {
            struct wined3d_texture *texture = texture_from_resource(view->resource);
            texture->sub_resources[view->sub_resource_idx].clear_value.depth = depth;
            texture->sub_resources[view->sub_resource_idx].clear_value.stencil = stencil;
            wined3d_rendertarget_view_validate_location(view, WINED3D_LOCATION_CLEARED);
            wined3d_rendertarget_view_invalidate_location(view, ~WINED3D_LOCATION_CLEARED);
            flags &= ~(WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
            delay_count++;
        }
    }

    if (!attachment_count)
    {
        TRACE("The clear has been delayed until draw time.\n");
        return;
    }

    TRACE("Doing an immediate clear of %u attachments.\n", attachment_count);
    if (delay_count)
        TRACE_(d3d_perf)("Partial clear: %u immediate, %u delayed.\n", attachment_count, delay_count);

    if (!(vk_render_pass = wined3d_context_vk_get_render_pass(context_vk, &immediate_fb,
            immediate_rt_count, flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL), flags)))
    {
        ERR("Failed to get render pass.\n");
        return;
    }

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return;
    }

    fb_desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_desc.pNext = NULL;
    fb_desc.flags = 0;
    fb_desc.renderPass = vk_render_pass;
    fb_desc.attachmentCount = attachment_count;
    fb_desc.pAttachments = views;
    fb_desc.width = draw_rect->right - draw_rect->left;
    fb_desc.height = draw_rect->bottom - draw_rect->top;
    fb_desc.layers = layer_count;
    if ((vr = VK_CALL(vkCreateFramebuffer(device_vk->vk_device, &fb_desc, NULL, &vk_framebuffer))) < 0)
    {
        ERR("Failed to create Vulkan framebuffer, vr %s.\n", wined3d_debug_vkresult(vr));
        return;
    }

    begin_desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_desc.pNext = NULL;
    begin_desc.renderPass = vk_render_pass;
    begin_desc.framebuffer = vk_framebuffer;
    begin_desc.clearValueCount = attachment_count;
    begin_desc.pClearValues = clear_values;

    wined3d_context_vk_end_current_render_pass(context_vk);

    for (i = 0; i < rect_count; ++i)
    {
        r.left = max(clear_rects[i].left, draw_rect->left);
        r.top = max(clear_rects[i].top, draw_rect->top);
        r.right = min(clear_rects[i].right, draw_rect->right);
        r.bottom = min(clear_rects[i].bottom, draw_rect->bottom);

        if (r.left >= r.right || r.top >= r.bottom)
            continue;

        begin_desc.renderArea.offset.x = r.left;
        begin_desc.renderArea.offset.y = r.top;
        begin_desc.renderArea.extent.width = r.right - r.left;
        begin_desc.renderArea.extent.height = r.bottom - r.top;
        VK_CALL(vkCmdBeginRenderPass(vk_command_buffer, &begin_desc, VK_SUBPASS_CONTENTS_INLINE));
        VK_CALL(vkCmdEndRenderPass(vk_command_buffer));
    }

    wined3d_context_vk_destroy_vk_framebuffer(context_vk, vk_framebuffer, context_vk->current_command_buffer.id);

    for (i = 0; i < immediate_rt_count; ++i)
        wined3d_context_vk_reference_rendertarget_view(context_vk,
                wined3d_rendertarget_view_vk(immediate_fb.render_targets[i]));

    if (depth_stencil)
        wined3d_context_vk_reference_rendertarget_view(context_vk,
                wined3d_rendertarget_view_vk(immediate_fb.depth_stencil));
}

static void vk_blitter_clear(struct wined3d_blitter *blitter, struct wined3d_device *device,
        unsigned int rt_count, const struct wined3d_fb_state *fb, unsigned int rect_count, const RECT *clear_rects,
        const RECT *draw_rect, uint32_t flags, const struct wined3d_color *colour, float depth, unsigned int stencil)
{
    struct wined3d_device_vk *device_vk = wined3d_device_vk(device);
    struct wined3d_rendertarget_view *view, *previous = NULL;
    struct wined3d_context_vk *context_vk;
    bool have_identical_size = true;
    struct wined3d_fb_state tmp_fb;
    unsigned int next_rt_count = 0;
    struct wined3d_blitter *next;
    uint32_t next_flags = 0;
    unsigned int i;

    TRACE("blitter %p, device %p, rt_count %u, fb %p, rect_count %u, clear_rects %p, "
            "draw_rect %s, flags %#x, colour %s, depth %.8e, stencil %#x.\n",
            blitter, device, rt_count, fb, rect_count, clear_rects,
            wine_dbgstr_rect(draw_rect), flags, debug_color(colour), depth, stencil);

    if (!rect_count)
    {
        rect_count = 1;
        clear_rects = draw_rect;
    }

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (!(view = fb->render_targets[i]))
                continue;

            if (wined3d_rendertarget_view_use_cpu_clear(view))
            {
                next_flags |= WINED3DCLEAR_TARGET;
                flags &= ~WINED3DCLEAR_TARGET;
                next_rt_count = rt_count;
                rt_count = 0;
                break;
            }
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
        context_vk = wined3d_context_vk(context_acquire(&device_vk->d, NULL, 0));

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
            vk_blitter_clear_rendertargets(context_vk, rt_count, fb, rect_count,
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
                vk_blitter_clear_rendertargets(context_vk, 1, &tmp_fb, rect_count,
                        clear_rects, draw_rect, WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
            if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
            {
                tmp_fb.render_targets[0] = NULL;
                tmp_fb.depth_stencil = fb->depth_stencil;
                vk_blitter_clear_rendertargets(context_vk, 0, &tmp_fb, rect_count,
                        clear_rects, draw_rect, flags & ~WINED3DCLEAR_TARGET, colour, depth, stencil);
            }
        }

        context_release(&context_vk->c);
    }

    if (!next_flags)
        return;

    if (!(next = blitter->next))
    {
        ERR("No blitter to handle clear.\n");
        return;
    }

    TRACE("Forwarding to blitter %p.\n", next);
    next->ops->blitter_clear(next, device, next_rt_count, fb, rect_count,
            clear_rects, draw_rect, next_flags, colour, depth, stencil);
}

static bool vk_blitter_blit_supported(enum wined3d_blit_op op, const struct wined3d_context *context,
        const struct wined3d_resource *src_resource, const RECT *src_rect,
        const struct wined3d_resource *dst_resource, const RECT *dst_rect, const struct wined3d_format *resolve_format)
{
    const struct wined3d_format *src_format = src_resource->format;
    const struct wined3d_format *dst_format = dst_resource->format;

    if (!(dst_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        TRACE("Destination resource does not have GPU access.\n");
        return false;
    }

    if (!(src_resource->access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        TRACE("Source resource does not have GPU access.\n");
        return false;
    }

    if (dst_format->id != src_format->id)
    {
        if (!is_identity_fixup(dst_format->color_fixup))
        {
            TRACE("Destination fixups are not supported.\n");
            return false;
        }

        if (!is_identity_fixup(src_format->color_fixup))
        {
            TRACE("Source fixups are not supported.\n");
            return false;
        }

        if (op != WINED3D_BLIT_OP_RAW_BLIT
                && wined3d_format_vk(src_format)->vk_format != wined3d_format_vk(dst_format)->vk_format
                && ((!wined3d_format_is_typeless(src_format) && !wined3d_format_is_typeless(dst_format))
                        || !resolve_format))
        {
            TRACE("Format conversion not supported.\n");
            return false;
        }

        if ((src_format->attrs | dst_format->attrs) & WINED3D_FORMAT_ATTR_PLANAR)
        {
            TRACE("Planar format conversion is not supported.\n");
            return false;
        }
    }

    if (wined3d_resource_get_sample_count(dst_resource) > 1)
    {
        TRACE("Multi-sample destination resource not supported.\n");
        return false;
    }

    if (op == WINED3D_BLIT_OP_RAW_BLIT)
        return true;

    if (op != WINED3D_BLIT_OP_COLOR_BLIT)
    {
        TRACE("Unsupported blit operation %#x.\n", op);
        return false;
    }

    if ((src_rect->right - src_rect->left != dst_rect->right - dst_rect->left)
            || (src_rect->bottom - src_rect->top != dst_rect->bottom - dst_rect->top))
    {
        TRACE("Scaling not supported.\n");
        return false;
    }

    return true;
}

static DWORD vk_blitter_blit(struct wined3d_blitter *blitter, enum wined3d_blit_op op,
        struct wined3d_context *context, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        DWORD src_location, const RECT *src_rect, struct wined3d_texture *dst_texture,
        unsigned int dst_sub_resource_idx, DWORD dst_location, const RECT *dst_rect,
        const struct wined3d_color_key *colour_key, enum wined3d_texture_filter_type filter,
        const struct wined3d_format *resolve_format)
{
    struct wined3d_texture_vk *src_texture_vk = wined3d_texture_vk(src_texture);
    struct wined3d_texture_vk *dst_texture_vk = wined3d_texture_vk(dst_texture);
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageSubresourceRange vk_src_range, vk_dst_range;
    VkImageLayout src_layout, dst_layout;
    VkCommandBuffer vk_command_buffer;
    struct wined3d_blitter *next;
    unsigned src_sample_count;
    bool resolve = false;

    TRACE("blitter %p, op %#x, context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, "
            "src_rect %s, dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_rect %s, "
            "colour_key %p, filter %s, resolve format %p.\n",
            blitter, op, context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            wine_dbgstr_rect(src_rect), dst_texture, dst_sub_resource_idx, wined3d_debug_location(dst_location),
            wine_dbgstr_rect(dst_rect), colour_key, debug_d3dtexturefiltertype(filter), resolve_format);

    if (!vk_blitter_blit_supported(op, context, &src_texture->resource, src_rect, &dst_texture->resource, dst_rect,
            resolve_format))
        goto next;

    src_sample_count = wined3d_resource_get_sample_count(&src_texture_vk->t.resource);
    if (src_sample_count > 1)
        resolve = true;

    vk_src_range.aspectMask = vk_aspect_mask_from_format(src_texture_vk->t.resource.format);
    vk_src_range.baseMipLevel = src_sub_resource_idx % src_texture->level_count;
    vk_src_range.levelCount = 1;
    vk_src_range.baseArrayLayer = src_sub_resource_idx / src_texture->level_count;
    vk_src_range.layerCount = 1;

    vk_dst_range.aspectMask = vk_aspect_mask_from_format(dst_texture_vk->t.resource.format);
    vk_dst_range.baseMipLevel = dst_sub_resource_idx % dst_texture->level_count;
    vk_dst_range.levelCount = 1;
    vk_dst_range.baseArrayLayer = dst_sub_resource_idx / dst_texture->level_count;
    vk_dst_range.layerCount = 1;

    if (!wined3d_texture_load_location(src_texture, src_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB))
        ERR("Failed to load the source sub-resource.\n");

    if (wined3d_texture_is_full_rect(dst_texture, vk_dst_range.baseMipLevel, dst_rect))
    {
        if (!wined3d_texture_prepare_location(dst_texture,
                dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB))
        {
            ERR("Failed to prepare the destination sub-resource.\n");
            goto next;
        }
    }
    else
    {
        if (!wined3d_texture_load_location(dst_texture,
                dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB))
        {
            ERR("Failed to load the destination sub-resource.\n");
            goto next;
        }
    }

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        goto next;
    }

    if (src_texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        src_layout = VK_IMAGE_LAYOUT_GENERAL;
    else
        src_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (dst_texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        dst_layout = VK_IMAGE_LAYOUT_GENERAL;
    else
        dst_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(src_texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_READ_BIT, src_texture_vk->layout, src_layout,
            src_texture_vk->image.vk_image, &vk_src_range);
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(dst_texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_WRITE_BIT, dst_texture_vk->layout, dst_layout,
            dst_texture_vk->image.vk_image, &vk_dst_range);

    if (resolve)
    {
        const struct wined3d_format_vk *src_format_vk = wined3d_format_vk(src_texture->resource.format);
        const struct wined3d_format_vk *dst_format_vk = wined3d_format_vk(dst_texture->resource.format);
        const unsigned int usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VkImageLayout resolve_src_layout, resolve_dst_layout;
        VkImage src_vk_image, dst_vk_image;
        VkImageSubresourceRange vk_range;
        VkImageResolve resolve_region;
        VkImageType vk_image_type;
        VkImageCopy copy_region;
        VkFormat vk_format;

        if (resolve_format)
        {
            vk_format = wined3d_format_vk(resolve_format)->vk_format;
        }
        else if (!wined3d_format_is_typeless(src_texture->resource.format))
        {
            vk_format = src_format_vk->vk_format;
        }
        else
        {
            vk_format = dst_format_vk->vk_format;
        }

        switch (src_texture->resource.type)
        {
            case WINED3D_RTYPE_TEXTURE_1D:
                vk_image_type = VK_IMAGE_TYPE_1D;
                break;
            case WINED3D_RTYPE_TEXTURE_2D:
                vk_image_type = VK_IMAGE_TYPE_2D;
                break;
            case WINED3D_RTYPE_TEXTURE_3D:
                vk_image_type = VK_IMAGE_TYPE_3D;
                break;
            default:
                ERR("Unexpected resource type: %s\n", debug_d3dresourcetype(src_texture->resource.type));
                goto barrier_next;
        }

        vk_range.baseMipLevel = 0;
        vk_range.levelCount = 1;
        vk_range.baseArrayLayer = 0;
        vk_range.layerCount = 1;

        resolve_region.srcSubresource.aspectMask = vk_src_range.aspectMask;
        resolve_region.dstSubresource.aspectMask = vk_dst_range.aspectMask;
        resolve_region.extent.width = src_rect->right - src_rect->left;
        resolve_region.extent.height = src_rect->bottom - src_rect->top;
        resolve_region.extent.depth = 1;

        /* In case of typeless resolve the texture type may not match the resolve type.
         * To handle that, allocate intermediate texture(s) to resolve from/to.
         * A possible performance improvement would be to resolve using a shader instead. */
        if (src_format_vk->vk_format != vk_format)
        {
            struct wined3d_image_vk src_image;

            if (!wined3d_context_vk_create_image(context_vk, vk_image_type, usage, vk_format,
                    resolve_region.extent.width, resolve_region.extent.height, 1,
                    src_sample_count, 1, 1, 0, NULL, &src_image))
                goto barrier_next;

            wined3d_context_vk_reference_image(context_vk, &src_image);
            src_vk_image = src_image.vk_image;
            wined3d_context_vk_destroy_image(context_vk, &src_image);

            vk_range.aspectMask = vk_src_range.aspectMask;

            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, src_vk_image, &vk_range);

            copy_region.srcSubresource.aspectMask = vk_src_range.aspectMask;
            copy_region.srcSubresource.mipLevel = vk_src_range.baseMipLevel;
            copy_region.srcSubresource.baseArrayLayer = vk_src_range.baseArrayLayer;
            copy_region.srcSubresource.layerCount = 1;
            copy_region.srcOffset.x = src_rect->left;
            copy_region.srcOffset.y = src_rect->top;
            copy_region.srcOffset.z = 0;
            copy_region.dstSubresource.aspectMask = vk_src_range.aspectMask;
            copy_region.dstSubresource.mipLevel = 0;
            copy_region.dstSubresource.baseArrayLayer = 0;
            copy_region.dstSubresource.layerCount = 1;
            copy_region.dstOffset.x = 0;
            copy_region.dstOffset.y = 0;
            copy_region.dstOffset.z = 0;
            copy_region.extent.width = resolve_region.extent.width;
            copy_region.extent.height = resolve_region.extent.height;
            copy_region.extent.depth = 1;

            VK_CALL(vkCmdCopyImage(vk_command_buffer, src_texture_vk->image.vk_image,
                    src_layout, src_vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &copy_region));

            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    src_vk_image, &vk_range);
            resolve_src_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            resolve_region.srcSubresource.mipLevel = 0;
            resolve_region.srcSubresource.baseArrayLayer = 0;
            resolve_region.srcSubresource.layerCount = 1;
            resolve_region.srcOffset.x = 0;
            resolve_region.srcOffset.y = 0;
            resolve_region.srcOffset.z = 0;
        }
        else
        {
            src_vk_image = src_texture_vk->image.vk_image;
            resolve_src_layout = src_layout;

            resolve_region.srcSubresource.mipLevel = vk_src_range.baseMipLevel;
            resolve_region.srcSubresource.baseArrayLayer = vk_src_range.baseArrayLayer;
            resolve_region.srcSubresource.layerCount = 1;
            resolve_region.srcOffset.x = src_rect->left;
            resolve_region.srcOffset.y = src_rect->top;
            resolve_region.srcOffset.z = 0;
        }

        if (dst_format_vk->vk_format != vk_format)
        {
            struct wined3d_image_vk dst_image;

            if (!wined3d_context_vk_create_image(context_vk, vk_image_type, usage, vk_format,
                    resolve_region.extent.width, resolve_region.extent.height, 1,
                    VK_SAMPLE_COUNT_1_BIT, 1, 1, 0, NULL, &dst_image))
                goto barrier_next;

            wined3d_context_vk_reference_image(context_vk, &dst_image);
            dst_vk_image = dst_image.vk_image;
            wined3d_context_vk_destroy_image(context_vk, &dst_image);

            vk_range.aspectMask = vk_dst_range.aspectMask;
            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_vk_image, &vk_range);
            resolve_dst_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            resolve_region.dstSubresource.mipLevel = 0;
            resolve_region.dstSubresource.baseArrayLayer = 0;
            resolve_region.dstSubresource.layerCount = 1;
            resolve_region.dstOffset.x = 0;
            resolve_region.dstOffset.y = 0;
            resolve_region.dstOffset.z = 0;
        }
        else
        {
            dst_vk_image = dst_texture_vk->image.vk_image;
            resolve_dst_layout = dst_layout;

            resolve_region.dstSubresource.mipLevel = vk_dst_range.baseMipLevel;
            resolve_region.dstSubresource.baseArrayLayer = vk_dst_range.baseArrayLayer;
            resolve_region.dstSubresource.layerCount = 1;
            resolve_region.dstOffset.x = dst_rect->left;
            resolve_region.dstOffset.y = dst_rect->top;
            resolve_region.dstOffset.z = 0;
        }

        VK_CALL(vkCmdResolveImage(vk_command_buffer, src_vk_image, resolve_src_layout,
                dst_vk_image, resolve_dst_layout, 1, &resolve_region));

        if (dst_vk_image != dst_texture_vk->image.vk_image)
        {
            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    resolve_dst_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dst_vk_image, &vk_range);

            copy_region.srcSubresource.aspectMask = vk_dst_range.aspectMask;
            copy_region.srcSubresource.mipLevel = 0;
            copy_region.srcSubresource.baseArrayLayer = 0;
            copy_region.srcSubresource.layerCount = 1;
            copy_region.srcOffset.x = 0;
            copy_region.srcOffset.y = 0;
            copy_region.srcOffset.z = 0;
            copy_region.dstSubresource.aspectMask = vk_dst_range.aspectMask;
            copy_region.dstSubresource.mipLevel = vk_dst_range.baseMipLevel;
            copy_region.dstSubresource.baseArrayLayer = vk_dst_range.baseArrayLayer;
            copy_region.dstSubresource.layerCount = 1;
            copy_region.dstOffset.x = dst_rect->left;
            copy_region.dstOffset.y = dst_rect->top;
            copy_region.dstOffset.z = 0;
            copy_region.extent.width = resolve_region.extent.width;
            copy_region.extent.height = resolve_region.extent.height;
            copy_region.extent.depth = 1;

            VK_CALL(vkCmdCopyImage(vk_command_buffer, dst_vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    dst_texture_vk->image.vk_image, dst_layout, 1, &copy_region));
        }
    }
    else
    {
        const struct wined3d_format *src_format = src_texture_vk->t.resource.format;
        VkImageCopy region;

        region.srcSubresource.aspectMask = vk_src_range.aspectMask;
        region.srcSubresource.mipLevel = vk_src_range.baseMipLevel;
        region.srcSubresource.baseArrayLayer = vk_src_range.baseArrayLayer;
        region.srcSubresource.layerCount = vk_src_range.layerCount;
        region.srcOffset.x = src_rect->left;
        region.srcOffset.y = src_rect->top;
        region.srcOffset.z = 0;
        region.dstSubresource.aspectMask = vk_dst_range.aspectMask;
        region.dstSubresource.mipLevel = vk_dst_range.baseMipLevel;
        region.dstSubresource.baseArrayLayer = vk_dst_range.baseArrayLayer;
        region.dstSubresource.layerCount = vk_dst_range.layerCount;
        region.dstOffset.x = dst_rect->left;
        region.dstOffset.y = dst_rect->top;
        region.dstOffset.z = 0;
        region.extent.width = src_rect->right - src_rect->left;
        region.extent.height = src_rect->bottom - src_rect->top;
        region.extent.depth = 1;

        if (src_format->attrs & WINED3D_FORMAT_ATTR_PLANAR)
        {
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
            VK_CALL(vkCmdCopyImage(vk_command_buffer, src_texture_vk->image.vk_image, src_layout,
                    dst_texture_vk->image.vk_image, dst_layout, 1, &region));
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
            region.srcOffset.x /= src_format->uv_width;
            region.srcOffset.y /= src_format->uv_height;
            region.dstOffset.x /= src_format->uv_width;
            region.dstOffset.y /= src_format->uv_height;
            region.extent.width /= src_format->uv_width;
            region.extent.height /= src_format->uv_height;
            VK_CALL(vkCmdCopyImage(vk_command_buffer, src_texture_vk->image.vk_image, src_layout,
                    dst_texture_vk->image.vk_image, dst_layout, 1, &region));
        }
        else
        {
            VK_CALL(vkCmdCopyImage(vk_command_buffer, src_texture_vk->image.vk_image, src_layout,
                    dst_texture_vk->image.vk_image, dst_layout, 1, &region));
        }
    }

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            vk_access_mask_from_bind_flags(dst_texture_vk->t.resource.bind_flags),
            dst_layout, dst_texture_vk->layout, dst_texture_vk->image.vk_image, &vk_dst_range);
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            vk_access_mask_from_bind_flags(src_texture_vk->t.resource.bind_flags),
            src_layout, src_texture_vk->layout, src_texture_vk->image.vk_image, &vk_src_range);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
    if (!wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, dst_location))
        ERR("Failed to load the destination sub-resource into %s.\n", wined3d_debug_location(dst_location));

    wined3d_context_vk_reference_texture(context_vk, src_texture_vk);
    wined3d_context_vk_reference_texture(context_vk, dst_texture_vk);

    return dst_location | WINED3D_LOCATION_TEXTURE_RGB;

barrier_next:
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            vk_access_mask_from_bind_flags(dst_texture_vk->t.resource.bind_flags),
            dst_layout, dst_texture_vk->layout, dst_texture_vk->image.vk_image, &vk_dst_range);
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            vk_access_mask_from_bind_flags(src_texture_vk->t.resource.bind_flags),
            src_layout, src_texture_vk->layout, src_texture_vk->image.vk_image, &vk_src_range);

next:
    if (!(next = blitter->next))
    {
        ERR("No blitter to handle blit op %#x.\n", op);
        return dst_location;
    }

    TRACE("Forwarding to blitter %p.\n", next);
    return next->ops->blitter_blit(next, op, context, src_texture, src_sub_resource_idx, src_location,
            src_rect, dst_texture, dst_sub_resource_idx, dst_location, dst_rect, colour_key, filter, resolve_format);
}

static const struct wined3d_blitter_ops vk_blitter_ops =
{
    .blitter_destroy = vk_blitter_destroy,
    .blitter_clear = vk_blitter_clear,
    .blitter_blit = vk_blitter_blit,
};

void wined3d_vk_blitter_create(struct wined3d_blitter **next)
{
    struct wined3d_blitter *blitter;

    if (!(blitter = malloc(sizeof(*blitter))))
        return;

    TRACE("Created blitter %p.\n", blitter);

    blitter->ops = &vk_blitter_ops;
    blitter->next = *next;
    *next = blitter;
}
