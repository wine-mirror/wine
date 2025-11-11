/*
 * Copyright 2020-2021 Henri Verbeet for CodeWeavers
 * Copyright 2021 Jan Sikorski for CodeWeavers
 * Copyright 2022-2023 Stefan Dösinger for CodeWeavers
 * Copyright 2022,2024-2025 Elizabeth Figura for CodeWeavers
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

void wined3d_vk_swizzle_from_color_fixup(VkComponentMapping *mapping, struct color_fixup_desc fixup)
{
    static const VkComponentSwizzle swizzle_source[] =
    {
        VK_COMPONENT_SWIZZLE_ZERO,  /* CHANNEL_SOURCE_ZERO */
        VK_COMPONENT_SWIZZLE_ONE,   /* CHANNEL_SOURCE_ONE */
        VK_COMPONENT_SWIZZLE_R,     /* CHANNEL_SOURCE_X */
        VK_COMPONENT_SWIZZLE_G,     /* CHANNEL_SOURCE_Y */
        VK_COMPONENT_SWIZZLE_B,     /* CHANNEL_SOURCE_Z */
        VK_COMPONENT_SWIZZLE_A,     /* CHANNEL_SOURCE_W */
    };

    mapping->r = swizzle_source[fixup.x_source];
    mapping->g = swizzle_source[fixup.y_source];
    mapping->b = swizzle_source[fixup.z_source];
    mapping->a = swizzle_source[fixup.w_source];
}

const VkDescriptorImageInfo *wined3d_texture_vk_get_default_image_info(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk)
{
    const struct wined3d_format_vk *format_vk;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_device_vk *device_vk;
    VkImageViewCreateInfo create_info;
    struct color_fixup_desc fixup;
    uint32_t flags = 0;
    VkResult vr;

    if (texture_vk->default_image_info.imageView)
        return &texture_vk->default_image_info;

    format_vk = wined3d_format_vk(texture_vk->t.resource.format);
    device_vk = wined3d_device_vk(texture_vk->t.resource.device);
    vk_info = context_vk->vk_info;

    if (texture_vk->t.layer_count > 1)
        flags |= WINED3D_VIEW_TEXTURE_ARRAY;

    wined3d_texture_vk_prepare_texture(texture_vk, context_vk);
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.image = texture_vk->image.vk_image;
    create_info.viewType = vk_image_view_type_from_wined3d(texture_vk->t.resource.type, flags);
    create_info.format = format_vk->vk_format;
    fixup = format_vk->f.color_fixup;
    if (is_identity_fixup(fixup) || !can_use_texture_swizzle(context_vk->c.d3d_info, &format_vk->f))
    {
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    else
    {
        wined3d_vk_swizzle_from_color_fixup(&create_info.components, fixup);
    }
    create_info.subresourceRange.aspectMask = vk_aspect_mask_from_format(&format_vk->f);
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = texture_vk->t.level_count;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = texture_vk->t.layer_count;
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &create_info,
            NULL, &texture_vk->default_image_info.imageView))) < 0)
    {
        ERR("Failed to create Vulkan image view, vr %s.\n", wined3d_debug_vkresult(vr));
        return NULL;
    }

    TRACE("Created image view 0x%s.\n", wine_dbgstr_longlong(texture_vk->default_image_info.imageView));

    texture_vk->default_image_info.sampler = VK_NULL_HANDLE;

    /* The default image view is used for SRVs, UAVs and RTVs when the d3d view encompasses the entire
     * resource. Any UAV capable resource will always use VK_IMAGE_LAYOUT_GENERAL, so we can use the
     * same image info for SRVs and UAVs. For render targets wined3d_rendertarget_view_vk_get_image_view
     * only cares about the VkImageView, not entire image info. So using SHADER_READ_ONLY_OPTIMAL works,
     * but relies on what the callers of the function do and don't do with the descriptor we return.
     *
     * Note that VkWriteDescriptorSet for SRV/UAV use takes a VkDescriptorImageInfo *, so we need a
     * place to store the VkDescriptorImageInfo. So returning onlky a VkImageView from this function
     * would bring its own problems. */
    if (texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        texture_vk->default_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    else
        texture_vk->default_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return &texture_vk->default_image_info;
}

static void wined3d_texture_vk_upload_plane(struct wined3d_context *context, VkImageAspectFlags vk_aspect,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *plane_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    struct wined3d_texture_vk *dst_texture_vk = wined3d_texture_vk(dst_texture);
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    unsigned int dst_level, dst_row_pitch, dst_slice_pitch;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int src_width, src_height, src_depth;
    struct wined3d_bo_address staging_bo_addr;
    VkPipelineStageFlags bo_stage_flags = 0;
    const struct wined3d_vk_info *vk_info;
    VkCommandBuffer vk_command_buffer;
    VkBufferMemoryBarrier vk_barrier;
    VkImageSubresourceRange vk_range;
    struct wined3d_bo_vk staging_bo;
    struct wined3d_bo_vk *src_bo;
    struct wined3d_range range;
    VkBufferImageCopy region;
    size_t src_offset;
    void *map_ptr;

    TRACE("context %p, vk_aspect %#x, src_bo_addr %s, plane_format %s, src_box %s, src_row_pitch %u, src_slice_pitch %u, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_x %u, dst_y %u, dst_z %u.\n",
            context, vk_aspect, debug_const_bo_address(src_bo_addr), debug_d3dformat(plane_format->id), debug_box(src_box),
            src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z);

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    wined3d_texture_get_pitch(dst_texture, dst_level, &dst_row_pitch, &dst_slice_pitch);
    if (dst_texture->resource.type == WINED3D_RTYPE_TEXTURE_1D)
        src_row_pitch = dst_row_pitch = 0;
    if (dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_3D)
        src_slice_pitch = dst_slice_pitch = 0;

    sub_resource = &dst_texture_vk->t.sub_resources[dst_sub_resource_idx];
    vk_info = context_vk->vk_info;

    src_width = src_box->right - src_box->left;
    src_height = src_box->bottom - src_box->top;
    src_depth = src_box->back - src_box->front;

    src_offset = src_box->front * src_slice_pitch
            + (src_box->top / plane_format->block_height) * src_row_pitch
            + (src_box->left / plane_format->block_width) * plane_format->block_byte_count;

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return;
    }

    /* We need to be outside of a render pass for vkCmdPipelineBarrier() and vkCmdCopyBufferToImage() calls below. */
    wined3d_context_vk_end_current_render_pass(context_vk);
    if (!src_bo_addr->buffer_object)
    {
        unsigned int staging_row_pitch, staging_slice_pitch, staging_size;

        wined3d_format_calculate_pitch(plane_format, context->device->surface_alignment, src_width, src_height,
                &staging_row_pitch, &staging_slice_pitch);
        staging_size = staging_slice_pitch * src_depth;

        if (!wined3d_context_vk_create_bo(context_vk, staging_size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &staging_bo))
        {
            ERR("Failed to create staging bo.\n");
            return;
        }

        staging_bo_addr.buffer_object = &staging_bo.b;
        staging_bo_addr.addr = NULL;
        if (!(map_ptr = wined3d_context_map_bo_address(context, &staging_bo_addr,
                staging_size, WINED3D_MAP_DISCARD | WINED3D_MAP_WRITE)))
        {
            ERR("Failed to map staging bo.\n");
            wined3d_context_vk_destroy_bo(context_vk, &staging_bo);
            return;
        }

        wined3d_format_copy_data(plane_format, src_bo_addr->addr + src_offset, src_row_pitch, src_slice_pitch,
                map_ptr, staging_row_pitch, staging_slice_pitch, src_width, src_height, src_depth);

        range.offset = 0;
        range.size = staging_size;
        wined3d_context_unmap_bo_address(context, &staging_bo_addr, 1, &range);

        src_bo = &staging_bo;

        src_offset = 0;
        src_row_pitch = staging_row_pitch;
        src_slice_pitch = staging_slice_pitch;
    }
    else
    {
        src_bo = wined3d_bo_vk(src_bo_addr->buffer_object);

        vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vk_barrier.pNext = NULL;
        vk_barrier.srcAccessMask = vk_access_mask_from_buffer_usage(src_bo->usage) & ~WINED3D_READ_ONLY_ACCESS_FLAGS;
        vk_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.buffer = src_bo->vk_buffer;
        vk_barrier.offset = src_bo->b.buffer_offset + (size_t)src_bo_addr->addr;
        vk_barrier.size = sub_resource->size;

        src_offset += (size_t)src_bo_addr->addr;

        bo_stage_flags = vk_pipeline_stage_mask_from_buffer_usage(src_bo->usage);
        if (vk_barrier.srcAccessMask)
            VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, bo_stage_flags,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &vk_barrier, 0, NULL));
    }

    vk_range.aspectMask = vk_aspect;
    vk_range.baseMipLevel = dst_level;
    vk_range.levelCount = 1;
    vk_range.baseArrayLayer = dst_sub_resource_idx / dst_texture_vk->t.level_count;
    vk_range.layerCount = 1;

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(dst_texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_WRITE_BIT,
            dst_texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            dst_texture_vk->image.vk_image, &vk_range);

    region.bufferOffset = src_bo->b.buffer_offset + src_offset;
    region.bufferRowLength = (src_row_pitch / plane_format->block_byte_count) * plane_format->block_width;
    if (src_row_pitch)
        region.bufferImageHeight = (src_slice_pitch / src_row_pitch) * plane_format->block_height;
    else
        region.bufferImageHeight = 1;
    region.imageSubresource.aspectMask = vk_range.aspectMask;
    region.imageSubresource.mipLevel = vk_range.baseMipLevel;
    region.imageSubresource.baseArrayLayer = vk_range.baseArrayLayer;
    region.imageSubresource.layerCount = vk_range.layerCount;
    region.imageOffset.x = dst_x;
    region.imageOffset.y = dst_y;
    region.imageOffset.z = dst_z;
    region.imageExtent.width = src_width;
    region.imageExtent.height = src_height;
    region.imageExtent.depth = src_depth;

    VK_CALL(vkCmdCopyBufferToImage(vk_command_buffer, src_bo->vk_buffer,
            dst_texture_vk->image.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            vk_access_mask_from_bind_flags(dst_texture_vk->t.resource.bind_flags),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, dst_texture_vk->layout,
            dst_texture_vk->image.vk_image, &vk_range);
    wined3d_context_vk_reference_texture(context_vk, dst_texture_vk);
    wined3d_context_vk_reference_bo(context_vk, src_bo);

    if (src_bo == &staging_bo)
    {
        wined3d_context_vk_destroy_bo(context_vk, &staging_bo);
    }
    else if (vk_barrier.srcAccessMask)
    {
        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                bo_stage_flags, 0, 0, NULL, 0, NULL, 0, NULL));
    }
}

static void wined3d_texture_vk_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, unsigned int dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    VkImageAspectFlags aspect_mask;

    TRACE("context %p, src_bo_addr %s, src_format %s, src_box %s, src_row_pitch %u, src_slice_pitch %u, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_x %u, dst_y %u, dst_z %u.\n",
            context, debug_const_bo_address(src_bo_addr), debug_d3dformat(src_format->id), debug_box(src_box),
            src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx,
            wined3d_debug_location(dst_location), dst_x, dst_y, dst_z);

    if (src_format->id != dst_texture->resource.format->id)
    {
        FIXME("Unhandled format conversion (%s -> %s).\n",
                debug_d3dformat(src_format->id),
                debug_d3dformat(dst_texture->resource.format->id));
        return;
    }

    if (dst_location != WINED3D_LOCATION_TEXTURE_RGB)
    {
        FIXME("Unhandled location %s.\n", wined3d_debug_location(dst_location));
        return;
    }

    if (wined3d_resource_get_sample_count(&dst_texture->resource) > 1)
    {
        FIXME("Not supported for multisample textures.\n");
        return;
    }

    aspect_mask = vk_aspect_mask_from_format(dst_texture->resource.format);
    if (wined3d_popcount(aspect_mask) > 1)
    {
        FIXME("Unhandled multi-aspect format %s.\n", debug_d3dformat(dst_texture->resource.format->id));
        return;
    }

    if (src_format->attrs & WINED3D_FORMAT_ATTR_PLANAR)
    {
        struct wined3d_const_bo_address uv_bo_addr;
        const struct wined3d_format *plane_format;
        struct wined3d_box uv_box;

        plane_format = wined3d_get_format(context->device->adapter, src_format->plane_formats[0], 0);
        wined3d_texture_vk_upload_plane(context, VK_IMAGE_ASPECT_PLANE_0_BIT, src_bo_addr, plane_format, src_box,
                src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z);

        uv_bo_addr = *src_bo_addr;
        uv_bo_addr.addr += src_slice_pitch;
        uv_box = *src_box;
        uv_box.left /= src_format->uv_width;
        uv_box.right /= src_format->uv_width;
        uv_box.top /= src_format->uv_height;
        uv_box.bottom /= src_format->uv_height;
        dst_x /= src_format->uv_width;
        dst_y /= src_format->uv_height;
        src_row_pitch = src_row_pitch * 2 / src_format->uv_width;
        src_slice_pitch = src_slice_pitch * 2 / src_format->uv_width / src_format->uv_height;

        plane_format = wined3d_get_format(context->device->adapter, src_format->plane_formats[1], 0);
        wined3d_texture_vk_upload_plane(context, VK_IMAGE_ASPECT_PLANE_1_BIT, &uv_bo_addr, plane_format, &uv_box,
                src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z);
    }
    else
    {
        wined3d_texture_vk_upload_plane(context, aspect_mask, src_bo_addr, src_format, src_box,
                src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z);
    }
}

static void wined3d_texture_vk_download_plane(struct wined3d_context *context, VkImageAspectFlags vk_aspect,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *plane_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    struct wined3d_texture_vk *src_texture_vk = wined3d_texture_vk(src_texture);
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    unsigned int src_level, src_width, src_height, src_depth;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int src_row_pitch, src_slice_pitch;
    struct wined3d_bo_address staging_bo_addr;
    VkPipelineStageFlags bo_stage_flags = 0;
    const struct wined3d_vk_info *vk_info;
    VkCommandBuffer vk_command_buffer;
    VkImageSubresourceRange vk_range;
    VkBufferMemoryBarrier vk_barrier;
    struct wined3d_bo_vk staging_bo;
    struct wined3d_bo_vk *dst_bo;
    VkBufferImageCopy region;
    size_t dst_offset = 0;
    void *map_ptr;

    TRACE("context %p, vk_aspect %#x, src_texture %p, src_sub_resource_idx %u, src_box %s, dst_bo_addr %s, "
            "plane_format %s, dst_x %u, dst_y %u, dst_z %u, dst_row_pitch %u, dst_slice_pitch %u.\n",
            context, vk_aspect, src_texture, src_sub_resource_idx,
            debug_box(src_box), debug_bo_address(dst_bo_addr), debug_d3dformat(plane_format->id),
            dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);

    src_level = src_sub_resource_idx % src_texture->level_count;
    wined3d_texture_get_pitch(src_texture, src_level, &src_row_pitch, &src_slice_pitch);
    if (src_texture->resource.type == WINED3D_RTYPE_TEXTURE_1D)
        src_row_pitch = dst_row_pitch = 0;
    if (src_texture->resource.type != WINED3D_RTYPE_TEXTURE_3D)
        src_slice_pitch = dst_slice_pitch = 0;

    sub_resource = &src_texture_vk->t.sub_resources[src_sub_resource_idx];
    vk_info = context_vk->vk_info;

    src_width = src_box->right - src_box->left;
    src_height = src_box->bottom - src_box->top;
    src_depth = src_box->back - src_box->front;

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return;
    }

    /* We need to be outside of a render pass for vkCmdPipelineBarrier() and vkCmdCopyImageToBuffer() calls below. */
    wined3d_context_vk_end_current_render_pass(context_vk);

    if (!dst_bo_addr->buffer_object)
    {
        if (!wined3d_context_vk_create_bo(context_vk, sub_resource->size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &staging_bo))
        {
            ERR("Failed to create staging bo.\n");
            return;
        }

        dst_bo = &staging_bo;

        region.bufferRowLength = (src_row_pitch / plane_format->block_byte_count) * plane_format->block_width;
        if (src_row_pitch)
            region.bufferImageHeight = (src_slice_pitch / src_row_pitch) * plane_format->block_height;
        else
            region.bufferImageHeight = 1;

        if (src_row_pitch % plane_format->byte_count)
        {
            FIXME("Row pitch %u is not a multiple of byte count %u.\n", src_row_pitch, plane_format->byte_count);
            return;
        }
        if (src_row_pitch && src_slice_pitch % src_row_pitch)
        {
            FIXME("Slice pitch %u is not a multiple of row pitch %u.\n", src_slice_pitch, src_row_pitch);
            return;
        }
    }
    else
    {
        dst_bo = wined3d_bo_vk(dst_bo_addr->buffer_object);

        region.bufferRowLength = (dst_row_pitch / plane_format->block_byte_count) * plane_format->block_width;
        if (dst_row_pitch)
            region.bufferImageHeight = (dst_slice_pitch / dst_row_pitch) * plane_format->block_height;
        else
            region.bufferImageHeight = 1;

        if (dst_row_pitch % plane_format->byte_count)
        {
            FIXME("Row pitch %u is not a multiple of byte count %u.\n", dst_row_pitch, plane_format->byte_count);
            return;
        }
        if (dst_row_pitch && dst_slice_pitch % dst_row_pitch)
        {
            FIXME("Slice pitch %u is not a multiple of row pitch %u.\n", dst_slice_pitch, dst_row_pitch);
            return;
        }

        vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vk_barrier.pNext = NULL;
        vk_barrier.srcAccessMask = vk_access_mask_from_buffer_usage(dst_bo->usage);
        vk_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.buffer = dst_bo->vk_buffer;
        vk_barrier.offset = dst_bo->b.buffer_offset + (size_t)dst_bo_addr->addr;
        vk_barrier.size = sub_resource->size;

        bo_stage_flags = vk_pipeline_stage_mask_from_buffer_usage(dst_bo->usage);
        dst_offset = (size_t)dst_bo_addr->addr;

        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, bo_stage_flags,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &vk_barrier, 0, NULL));
    }

    vk_range.aspectMask = vk_aspect;
    vk_range.baseMipLevel = src_level;
    vk_range.levelCount = 1;
    vk_range.baseArrayLayer = src_sub_resource_idx / src_texture_vk->t.level_count;
    vk_range.layerCount = 1;

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(src_texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_READ_BIT,
            src_texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            src_texture_vk->image.vk_image, &vk_range);

    region.bufferOffset = dst_bo->b.buffer_offset + dst_offset;
    region.imageSubresource.aspectMask = vk_range.aspectMask;
    region.imageSubresource.mipLevel = vk_range.baseMipLevel;
    region.imageSubresource.baseArrayLayer = vk_range.baseArrayLayer;
    region.imageSubresource.layerCount = vk_range.layerCount;
    region.imageOffset.x = 0;
    region.imageOffset.y = 0;
    region.imageOffset.z = 0;
    region.imageExtent.width = src_width;
    region.imageExtent.height = src_height;
    region.imageExtent.depth = src_depth;

    VK_CALL(vkCmdCopyImageToBuffer(vk_command_buffer, src_texture_vk->image.vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_bo->vk_buffer, 1, &region));

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            vk_access_mask_from_bind_flags(src_texture_vk->t.resource.bind_flags),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,  src_texture_vk->layout,
            src_texture_vk->image.vk_image, &vk_range);

    wined3d_context_vk_reference_texture(context_vk, src_texture_vk);
    wined3d_context_vk_reference_bo(context_vk, dst_bo);

    if (dst_bo == &staging_bo)
    {
        wined3d_context_vk_submit_command_buffer(context_vk, 0, NULL, NULL, 0, NULL);
        wined3d_context_vk_wait_command_buffer(context_vk, src_texture_vk->image.command_buffer_id);

        staging_bo_addr.buffer_object = &staging_bo.b;
        staging_bo_addr.addr = NULL;
        if (!(map_ptr = wined3d_context_map_bo_address(context, &staging_bo_addr,
                sub_resource->size, WINED3D_MAP_READ)))
        {
            ERR("Failed to map staging bo.\n");
            wined3d_context_vk_destroy_bo(context_vk, &staging_bo);
            return;
        }

        wined3d_format_copy_data(plane_format, map_ptr, src_row_pitch, src_slice_pitch,
                dst_bo_addr->addr, dst_row_pitch, dst_slice_pitch, src_box->right - src_box->left,
                src_box->bottom - src_box->top, src_box->back - src_box->front);

        wined3d_context_unmap_bo_address(context, &staging_bo_addr, 0, NULL);
        wined3d_context_vk_destroy_bo(context_vk, &staging_bo);
    }
    else
    {
        vk_barrier.dstAccessMask = vk_barrier.srcAccessMask;
        vk_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        if (dst_bo->host_synced)
        {
            vk_barrier.dstAccessMask |= VK_ACCESS_HOST_READ_BIT;
            bo_stage_flags |= VK_PIPELINE_STAGE_HOST_BIT;
        }

        VK_CALL(vkCmdPipelineBarrier(vk_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                bo_stage_flags, 0, 0, NULL, 1, &vk_barrier, 0, NULL));
        /* Start the download so we don't stall waiting for the result. */
        wined3d_context_vk_submit_command_buffer(context_vk, 0, NULL, NULL, 0, NULL);
    }
}

static void wined3d_texture_vk_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    unsigned int src_level, src_width, src_height, src_depth;
    VkImageAspectFlags aspect_mask;

    TRACE("context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_box %s, dst_bo_addr %s, "
            "dst_format %s, dst_x %u, dst_y %u, dst_z %u, dst_row_pitch %u, dst_slice_pitch %u.\n",
            context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            debug_box(src_box), debug_bo_address(dst_bo_addr), debug_d3dformat(dst_format->id),
            dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);

    if (src_location != WINED3D_LOCATION_TEXTURE_RGB)
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

    if (dst_format->id != src_texture->resource.format->id)
    {
        FIXME("Unhandled format conversion (%s -> %s).\n",
                debug_d3dformat(src_texture->resource.format->id),
                debug_d3dformat(dst_format->id));
        return;
    }

    if (dst_x || dst_y || dst_z)
    {
        FIXME("Unhandled destination (%u, %u, %u).\n", dst_x, dst_y, dst_z);
        return;
    }

    if (wined3d_resource_get_sample_count(&src_texture->resource) > 1)
    {
        FIXME("Not supported for multisample textures.\n");
        return;
    }

    aspect_mask = vk_aspect_mask_from_format(src_texture->resource.format);
    if (wined3d_popcount(aspect_mask) > 1)
    {
        FIXME("Unhandled multi-aspect format %s.\n", debug_d3dformat(src_texture->resource.format->id));
        return;
    }

    if (dst_format->attrs & WINED3D_FORMAT_ATTR_PLANAR)
    {
        const struct wined3d_format *plane_format;
        struct wined3d_bo_address uv_bo_addr;
        struct wined3d_box uv_box;

        plane_format = wined3d_get_format(context->device->adapter, dst_format->plane_formats[0], 0);
        wined3d_texture_vk_download_plane(context, VK_IMAGE_ASPECT_PLANE_0_BIT, src_texture, src_sub_resource_idx,
                src_box, dst_bo_addr, plane_format, dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);

        uv_bo_addr = *dst_bo_addr;
        uv_bo_addr.addr += dst_slice_pitch;
        uv_box = *src_box;
        uv_box.left /= dst_format->uv_width;
        uv_box.right /= dst_format->uv_width;
        uv_box.top /= dst_format->uv_height;
        uv_box.bottom /= dst_format->uv_height;
        dst_x /= dst_format->uv_width;
        dst_y /= dst_format->uv_height;
        dst_row_pitch = dst_row_pitch * 2 / dst_format->uv_width;
        dst_slice_pitch = dst_slice_pitch * 2 / dst_format->uv_width / dst_format->uv_height;

        plane_format = wined3d_get_format(context->device->adapter, dst_format->plane_formats[1], 0);
        wined3d_texture_vk_download_plane(context, VK_IMAGE_ASPECT_PLANE_1_BIT, src_texture, src_sub_resource_idx,
                &uv_box, &uv_bo_addr, plane_format, dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);
    }
    else
    {
        wined3d_texture_vk_download_plane(context, aspect_mask, src_texture, src_sub_resource_idx,
                src_box, dst_bo_addr, dst_format, dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);
    }
}

static bool wined3d_texture_vk_clear(struct wined3d_texture_vk *texture_vk,
        unsigned int sub_resource_idx, struct wined3d_context *context)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture_vk->t.sub_resources[sub_resource_idx];
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    const struct wined3d_format *format = texture_vk->t.resource.format;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkClearDepthStencilValue depth_value;
    VkCommandBuffer vk_command_buffer;
    VkImageSubresourceRange vk_range;
    VkClearColorValue colour_value;
    VkImageAspectFlags aspect_mask;
    VkImage vk_image;

    if (texture_vk->t.resource.format_attrs & WINED3D_FORMAT_ATTR_COMPRESSED)
    {
        struct wined3d_bo_address addr;
        struct wined3d_color *c = &sub_resource->clear_value.colour;

        if (c->r || c->g || c-> b || c->a)
            FIXME("Compressed resource %p is cleared to a non-zero color.\n", &texture_vk->t);

        if (!wined3d_texture_prepare_location(&texture_vk->t, sub_resource_idx, context, WINED3D_LOCATION_SYSMEM))
            return false;
        wined3d_texture_get_bo_address(&texture_vk->t, sub_resource_idx, &addr, WINED3D_LOCATION_SYSMEM);
        memset(addr.addr, 0, sub_resource->size);
        wined3d_texture_validate_location(&texture_vk->t, sub_resource_idx, WINED3D_LOCATION_SYSMEM);
        return true;
    }

    vk_image = texture_vk->image.vk_image;

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return false;
    }

    aspect_mask = vk_aspect_mask_from_format(format);

    vk_range.aspectMask = aspect_mask;
    vk_range.baseMipLevel = sub_resource_idx % texture_vk->t.level_count;
    vk_range.levelCount = 1;
    vk_range.baseArrayLayer = sub_resource_idx / texture_vk->t.level_count;
    vk_range.layerCount = 1;

    wined3d_context_vk_end_current_render_pass(context_vk);

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags), VK_ACCESS_TRANSFER_WRITE_BIT,
            texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_image, &vk_range);

    if (format->depth_size || format->stencil_size)
    {
        depth_value.depth = sub_resource->clear_value.depth;
        depth_value.stencil = sub_resource->clear_value.stencil;
        VK_CALL(vkCmdClearDepthStencilImage(vk_command_buffer, vk_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depth_value, 1, &vk_range));
    }
    else
    {
        wined3d_format_colour_to_vk(format, &sub_resource->clear_value.colour, &colour_value);
        VK_CALL(vkCmdClearColorImage(vk_command_buffer, vk_image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colour_value, 1, &vk_range));
    }

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT, vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture_vk->layout, vk_image, &vk_range);
    wined3d_context_vk_reference_texture(context_vk, texture_vk);

    wined3d_texture_validate_location(&texture_vk->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    return true;
}

static BOOL wined3d_texture_vk_load_texture(struct wined3d_texture_vk *texture_vk,
        unsigned int sub_resource_idx, struct wined3d_context *context)
{
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int level, row_pitch, slice_pitch;
    struct wined3d_bo_address data;
    struct wined3d_box src_box;

    sub_resource = &texture_vk->t.sub_resources[sub_resource_idx];

    if (sub_resource->locations & WINED3D_LOCATION_CLEARED)
    {
        if (!wined3d_texture_vk_clear(texture_vk, sub_resource_idx, context))
            return FALSE;

        if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
            return TRUE;
    }

    if (!(sub_resource->locations & wined3d_texture_sysmem_locations))
    {
        ERR("Unimplemented load from %s.\n", wined3d_debug_location(sub_resource->locations));
        return FALSE;
    }

    level = sub_resource_idx % texture_vk->t.level_count;
    wined3d_texture_get_memory(&texture_vk->t, sub_resource_idx, context, &data);
    wined3d_texture_get_level_box(&texture_vk->t, level, &src_box);
    wined3d_texture_get_pitch(&texture_vk->t, level, &row_pitch, &slice_pitch);
    wined3d_texture_vk_upload_data(context, wined3d_const_bo_address(&data), texture_vk->t.resource.format,
            &src_box, row_pitch, slice_pitch, &texture_vk->t, sub_resource_idx,
            WINED3D_LOCATION_TEXTURE_RGB, 0, 0, 0);

    return TRUE;
}

static BOOL wined3d_texture_vk_load_sysmem(struct wined3d_texture_vk *texture_vk,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int level, row_pitch, slice_pitch;
    struct wined3d_bo_address data;
    struct wined3d_box src_box;

    sub_resource = &texture_vk->t.sub_resources[sub_resource_idx];
    if (!(sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB))
    {
        ERR("Unimplemented load from %s.\n", wined3d_debug_location(sub_resource->locations));
        return FALSE;
    }

    level = sub_resource_idx % texture_vk->t.level_count;
    wined3d_texture_get_bo_address(&texture_vk->t, sub_resource_idx, &data, location);
    wined3d_texture_get_level_box(&texture_vk->t, level, &src_box);
    wined3d_texture_get_pitch(&texture_vk->t, level, &row_pitch, &slice_pitch);
    wined3d_texture_vk_download_data(context, &texture_vk->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB,
            &src_box, &data, texture_vk->t.resource.format, 0, 0, 0, row_pitch, slice_pitch);

    return TRUE;
}

BOOL wined3d_texture_vk_prepare_texture(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk)
{
    const struct wined3d_format_vk *format_vk;
    struct wined3d_resource *resource;
    VkCommandBuffer vk_command_buffer;
    VkImageSubresourceRange vk_range;
    VkImageUsageFlags vk_usage;
    VkImageType vk_image_type;
    unsigned int flags = 0;

    if (texture_vk->t.flags & WINED3D_TEXTURE_RGB_ALLOCATED)
        return TRUE;

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return FALSE;
    }

    resource = &texture_vk->t.resource;
    format_vk = wined3d_format_vk(resource->format);

    if (wined3d_format_is_typeless(&format_vk->f) || texture_vk->t.swapchain
            || (texture_vk->t.resource.bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
            || (format_vk->f.attrs & WINED3D_FORMAT_ATTR_PLANAR))
    {
        /* For UAVs, we need this in case a clear necessitates creation of a new view
         * with a different format. */
        flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }

    switch (resource->type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
            vk_image_type = VK_IMAGE_TYPE_1D;
            break;
        case WINED3D_RTYPE_TEXTURE_2D:
            vk_image_type = VK_IMAGE_TYPE_2D;
            if (texture_vk->t.layer_count >= 6 && resource->width == resource->height)
                flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            break;
        case WINED3D_RTYPE_TEXTURE_3D:
            vk_image_type = VK_IMAGE_TYPE_3D;
            if (resource->bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_UNORDERED_ACCESS))
                flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
            break;
        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(resource->type));
            vk_image_type = VK_IMAGE_TYPE_2D;
            break;
    }

    vk_usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (resource->bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        vk_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (resource->bind_flags & WINED3D_BIND_RENDER_TARGET)
        vk_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (resource->bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
        vk_usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    if (resource->bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
        texture_vk->layout = VK_IMAGE_LAYOUT_GENERAL;
    else if (resource->bind_flags & WINED3D_BIND_RENDER_TARGET)
        texture_vk->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    else if (resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        texture_vk->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    else if (resource->bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        texture_vk->layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    else if (resource->bind_flags & WINED3D_BIND_DECODER_OUTPUT)
        texture_vk->layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    else
    {
        FIXME("unexpected bind flags %s, using VK_IMAGE_LAYOUT_GENERAL\n", wined3d_debug_bind_flags(resource->bind_flags));
        texture_vk->layout = VK_IMAGE_LAYOUT_GENERAL;
    }

    if (!wined3d_context_vk_create_image(context_vk, vk_image_type, vk_usage, format_vk->vk_format,
            resource->width, resource->height, resource->depth, max(1, wined3d_resource_get_sample_count(resource)),
            texture_vk->t.level_count, texture_vk->t.layer_count, flags, NULL, &texture_vk->image))
    {
        return FALSE;
    }

    /* We can't use a zero src access mask without synchronization2. Set the last-used bind mask to something
     * non-zero to avoid this. */
    texture_vk->bind_mask = resource->bind_flags;

    vk_range.aspectMask = vk_aspect_mask_from_format(&format_vk->f);
    vk_range.baseMipLevel = 0;
    vk_range.levelCount = VK_REMAINING_MIP_LEVELS;
    vk_range.baseArrayLayer = 0;
    vk_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    wined3d_context_vk_reference_texture(context_vk, texture_vk);
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0,
            VK_IMAGE_LAYOUT_UNDEFINED, texture_vk->layout,
            texture_vk->image.vk_image, &vk_range);

    texture_vk->t.flags |= WINED3D_TEXTURE_RGB_ALLOCATED;

    TRACE("Created image 0x%s, memory 0x%s for texture %p.\n",
            wine_dbgstr_longlong(texture_vk->image.vk_image), wine_dbgstr_longlong(texture_vk->image.vk_memory), texture_vk);

    return TRUE;
}

static BOOL wined3d_texture_vk_prepare_buffer_object(struct wined3d_texture_vk *texture_vk,
        unsigned int sub_resource_idx, struct wined3d_context_vk *context_vk)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_bo_vk *bo;

    sub_resource = &texture_vk->t.sub_resources[sub_resource_idx];
    if (sub_resource->bo)
        return TRUE;

    if (!(bo = malloc(sizeof(*bo))))
        return FALSE;

    if (!wined3d_context_vk_create_bo(context_vk, sub_resource->size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bo))
    {
        free(bo);
        return FALSE;
    }

    /* Texture buffer objects receive a barrier to HOST_READ in wined3d_texture_vk_download_data(),
     * so they don't need it when they are mapped for reading. */
    bo->host_synced = true;
    sub_resource->bo = &bo->b;
    TRACE("Created buffer object %p for texture %p, sub-resource %u.\n", bo, texture_vk, sub_resource_idx);
    return TRUE;
}

static BOOL wined3d_texture_vk_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return texture->sub_resources[sub_resource_idx].user_memory ? TRUE
                    : wined3d_resource_prepare_sysmem(&texture->resource);

        case WINED3D_LOCATION_TEXTURE_RGB:
            return wined3d_texture_vk_prepare_texture(wined3d_texture_vk(texture), wined3d_context_vk(context));

        case WINED3D_LOCATION_BUFFER:
            return wined3d_texture_vk_prepare_buffer_object(wined3d_texture_vk(texture), sub_resource_idx,
                    wined3d_context_vk(context));

        default:
            FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static BOOL wined3d_texture_vk_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, uint32_t location)
{
    if (!wined3d_texture_vk_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
            return wined3d_texture_vk_load_texture(wined3d_texture_vk(texture), sub_resource_idx, context);

        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_BUFFER:
            return wined3d_texture_vk_load_sysmem(wined3d_texture_vk(texture), sub_resource_idx, context, location);

        default:
            FIXME("Unimplemented location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static void wined3d_texture_vk_unload_location(struct wined3d_texture *texture,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_texture_vk *texture_vk = wined3d_texture_vk(texture);
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    unsigned int i, sub_count;

    TRACE("texture %p, context %p, location %s.\n", texture, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
            if (texture_vk->default_image_info.imageView)
            {
                wined3d_context_vk_destroy_vk_image_view(context_vk,
                        texture_vk->default_image_info.imageView, texture_vk->image.command_buffer_id);
                texture_vk->default_image_info.imageView = VK_NULL_HANDLE;
            }

            if (texture_vk->image.vk_image)
                wined3d_context_vk_destroy_image(context_vk, &texture_vk->image);
            break;

        case WINED3D_LOCATION_BUFFER:
            sub_count = texture->level_count * texture->layer_count;
            for (i = 0; i < sub_count; ++i)
            {
                struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[i];

                if (sub_resource->bo)
                {
                    struct wined3d_bo_vk *bo_vk = wined3d_bo_vk(sub_resource->bo);

                    wined3d_context_vk_destroy_bo(context_vk, bo_vk);
                    free(bo_vk);
                    sub_resource->bo = NULL;
                }
            }
            break;

        case WINED3D_LOCATION_TEXTURE_SRGB:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
        case WINED3D_LOCATION_RB_RESOLVED:
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

static const struct wined3d_texture_ops wined3d_texture_vk_ops =
{
    wined3d_texture_vk_prepare_location,
    wined3d_texture_vk_load_location,
    wined3d_texture_vk_unload_location,
    wined3d_texture_vk_upload_data,
    wined3d_texture_vk_download_data,
};

HRESULT wined3d_texture_vk_init(struct wined3d_texture_vk *texture_vk, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("texture_vk %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_vk, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    return wined3d_texture_init(&texture_vk->t, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_vk[1], &wined3d_texture_vk_ops);
}

enum VkImageLayout wined3d_layout_from_bind_mask(const struct wined3d_texture_vk *texture_vk, const uint32_t bind_mask)
{
    assert(wined3d_popcount(bind_mask) == 1);

    /* We want to avoid switching between LAYOUT_GENERAL and other layouts. In Radeon GPUs (and presumably
     * others), this will trigger decompressing and recompressing the texture. We also hardcode the layout
     * into views when they are created. */
    if (texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        return VK_IMAGE_LAYOUT_GENERAL;

    switch (bind_mask)
    {
        case WINED3D_BIND_RENDER_TARGET:
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        case WINED3D_BIND_DEPTH_STENCIL:
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        case WINED3D_BIND_SHADER_RESOURCE:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        default:
            ERR("Unexpected bind mask %s.\n", wined3d_debug_bind_flags(bind_mask));
            return VK_IMAGE_LAYOUT_GENERAL;
    }
}

void wined3d_texture_vk_barrier(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    enum VkImageLayout new_layout;
    uint32_t src_bind_mask = 0;

    TRACE("texture_vk %p, context_vk %p, bind_mask %s.\n",
            texture_vk, context_vk, wined3d_debug_bind_flags(bind_mask));

    new_layout = wined3d_layout_from_bind_mask(texture_vk, bind_mask);

    /* A layout transition is potentially a read-write operation, so even if we
     * prepare the texture to e.g. read only shader resource mode, we have to wait
     * for past operations to finish. */
    if (bind_mask & ~WINED3D_READ_ONLY_BIND_MASK || new_layout != texture_vk->layout)
    {
        src_bind_mask = texture_vk->bind_mask & WINED3D_READ_ONLY_BIND_MASK;
        if (!src_bind_mask)
            src_bind_mask = texture_vk->bind_mask;

        texture_vk->bind_mask = bind_mask;
    }
    else if ((texture_vk->bind_mask & bind_mask) != bind_mask)
    {
        src_bind_mask = texture_vk->bind_mask & ~WINED3D_READ_ONLY_BIND_MASK;
        texture_vk->bind_mask |= bind_mask;
    }

    if (src_bind_mask)
    {
        VkImageSubresourceRange vk_range;

        TRACE("    %s(%x) -> %s(%x).\n",
                wined3d_debug_bind_flags(src_bind_mask), texture_vk->layout,
                wined3d_debug_bind_flags(bind_mask), new_layout);

        vk_range.aspectMask = vk_aspect_mask_from_format(texture_vk->t.resource.format);
        vk_range.baseMipLevel = 0;
        vk_range.levelCount = VK_REMAINING_MIP_LEVELS;
        vk_range.baseArrayLayer = 0;
        vk_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        wined3d_context_vk_image_barrier(context_vk, wined3d_context_vk_get_command_buffer(context_vk),
                vk_pipeline_stage_mask_from_bind_flags(src_bind_mask),
                vk_pipeline_stage_mask_from_bind_flags(bind_mask),
                vk_access_mask_from_bind_flags(src_bind_mask), vk_access_mask_from_bind_flags(bind_mask),
                texture_vk->layout, new_layout, texture_vk->image.vk_image, &vk_range);

        texture_vk->layout = new_layout;
    }
}

/* This is called when a texture is used as render target and shader resource
 * or depth stencil and shader resource at the same time. This can either be
 * read-only simultaneos use as depth stencil, but also for rendering to one
 * subresource while reading from another. Without tracking of barriers and
 * layouts per subresource VK_IMAGE_LAYOUT_GENERAL is the only thing we can do. */
void wined3d_texture_vk_make_generic(struct wined3d_texture_vk *texture_vk,
        struct wined3d_context_vk *context_vk)
{
    VkImageSubresourceRange vk_range;

    if (texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        return;

    vk_range.aspectMask = vk_aspect_mask_from_format(texture_vk->t.resource.format);
    vk_range.baseMipLevel = 0;
    vk_range.levelCount = VK_REMAINING_MIP_LEVELS;
    vk_range.baseArrayLayer = 0;
    vk_range.layerCount = VK_REMAINING_ARRAY_LAYERS;

    wined3d_context_vk_image_barrier(context_vk, wined3d_context_vk_get_command_buffer(context_vk),
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
            texture_vk->layout, VK_IMAGE_LAYOUT_GENERAL, texture_vk->image.vk_image, &vk_range);

    texture_vk->layout = VK_IMAGE_LAYOUT_GENERAL;
    texture_vk->default_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

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
