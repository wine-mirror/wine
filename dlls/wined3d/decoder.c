/*
 * Copyright 2024 Elizabeth Figura for CodeWeavers
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

struct wined3d_decoder
{
    LONG ref;
    struct wined3d_device *device;
    struct wined3d_decoder_desc desc;
    struct wined3d_buffer *bitstream, *parameters, *matrix, *slice_control;
    struct wined3d_decoder_output_view *output_view;
};

static void wined3d_decoder_cleanup(struct wined3d_decoder *decoder)
{
    wined3d_buffer_decref(decoder->bitstream);
    wined3d_buffer_decref(decoder->parameters);
    wined3d_buffer_decref(decoder->matrix);
    wined3d_buffer_decref(decoder->slice_control);
}

ULONG CDECL wined3d_decoder_decref(struct wined3d_decoder *decoder)
{
    unsigned int refcount = InterlockedDecrement(&decoder->ref);

    TRACE("%p decreasing refcount to %u.\n", decoder, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        decoder->device->adapter->decoder_ops->destroy(decoder);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static bool is_supported_codec(struct wined3d_adapter *adapter, const GUID *codec)
{
    GUID profiles[WINED3D_DECODER_MAX_PROFILE_COUNT];
    unsigned int count;

    adapter->decoder_ops->get_profiles(adapter, &count, profiles);

    for (unsigned int i = 0; i < count; ++i)
    {
        if (IsEqualGUID(&profiles[i], codec))
            return true;
    }
    return false;
}

static HRESULT wined3d_decoder_init(struct wined3d_decoder *decoder,
        struct wined3d_device *device, const struct wined3d_decoder_desc *desc)
{
    HRESULT hr;

    struct wined3d_buffer_desc buffer_desc =
    {
        .access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W,
    };

    decoder->ref = 1;
    decoder->device = device;
    decoder->desc = *desc;

    buffer_desc.byte_width = sizeof(DXVA_PicParams_H264);
    if (FAILED(hr = wined3d_buffer_create(device, &buffer_desc,
            NULL, NULL, &wined3d_null_parent_ops, &decoder->parameters)))
        return hr;

    buffer_desc.byte_width = sizeof(DXVA_Qmatrix_H264);
    if (FAILED(hr = wined3d_buffer_create(device, &buffer_desc,
            NULL, NULL, &wined3d_null_parent_ops, &decoder->matrix)))
    {
        wined3d_buffer_decref(decoder->parameters);
        return hr;
    }

    /* NVidia gives 64 * sizeof(DXVA_Slice_H264_Long).
     * AMD gives 4096 bytes. Pick the smaller one. */
    buffer_desc.byte_width = 4096;
    if (FAILED(hr = wined3d_buffer_create(device, &buffer_desc,
            NULL, NULL, &wined3d_null_parent_ops, &decoder->slice_control)))
    {
        wined3d_buffer_decref(decoder->matrix);
        wined3d_buffer_decref(decoder->parameters);
        return hr;
    }

    /* NVidia makes this buffer as large as width * height (as if each pixel
     * is at most 1 byte). AMD makes it larger than that.
     * Go with the smaller of the two. */
    buffer_desc.byte_width = desc->width * desc->height;
    buffer_desc.bind_flags = WINED3D_BIND_DECODER_SRC;
    buffer_desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_W;
    buffer_desc.usage = WINED3DUSAGE_DYNAMIC;

    if (FAILED(hr = wined3d_buffer_create(device, &buffer_desc,
            NULL, NULL, &wined3d_null_parent_ops, &decoder->bitstream)))
    {
        wined3d_buffer_decref(decoder->matrix);
        wined3d_buffer_decref(decoder->parameters);
        wined3d_buffer_decref(decoder->slice_control);
        return hr;
    }

    return S_OK;
}

HRESULT CDECL wined3d_decoder_create(struct wined3d_device *device,
        const struct wined3d_decoder_desc *desc, struct wined3d_decoder **decoder)
{
    TRACE("device %p, codec %s, size %ux%u, output_format %s, decoder %p.\n", device,
            debugstr_guid(&desc->codec), desc->width, desc->height, debug_d3dformat(desc->output_format), decoder);

    if (!is_supported_codec(device->adapter, &desc->codec))
    {
        WARN("Codec %s is not supported; returning E_INVALIDARG.\n", debugstr_guid(&desc->codec));
        return E_INVALIDARG;
    }

    return device->adapter->decoder_ops->create(device, desc, decoder);
}

static void wined3d_null_decoder_get_profiles(struct wined3d_adapter *adapter, unsigned int *count, GUID *profiles)
{
    *count = 0;
}

const struct wined3d_decoder_ops wined3d_null_decoder_ops =
{
    .get_profiles = wined3d_null_decoder_get_profiles,
};

/* DXVA_PicParams_H264 only allows for 16 reference frames. */
#define MAX_VK_DECODE_REFERENCE_SLOTS 16

struct wined3d_decoder_vk
{
    struct wined3d_decoder d;
    VkVideoSessionKHR vk_session;
    uint64_t command_buffer_id;
    struct wined3d_allocator_block *session_memory;
    VkDeviceMemory vk_session_memory;

    bool distinct_dpb, layered_dpb;

    bool initialized;

    bool needs_wait_semaphore;
    struct wined3d_aux_command_buffer_vk command_buffer;

    VkDeviceSize bitstream_alignment;

    struct wined3d_decoder_image_vk
    {
        uint8_t dxva_index;
        bool used;
        struct wined3d_image_vk output_image, dpb_image;
        VkImageView output_view, dpb_view;
    } images[MAX_VK_DECODE_REFERENCE_SLOTS + 1];
    struct wined3d_image_vk layered_output_image, layered_dpb_image;
};

static struct wined3d_decoder_vk *wined3d_decoder_vk(struct wined3d_decoder *decoder)
{
    return CONTAINING_RECORD(decoder, struct wined3d_decoder_vk, d);
}

static void fill_vk_profile_info(VkVideoProfileInfoKHR *profile, const GUID *codec, enum wined3d_format_id format)
{
    profile->sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;

    if (format == WINED3DFMT_NV12_PLANAR)
    {
        profile->chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;
        profile->lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
        profile->chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
    }
    else
    {
        FIXME("Unhandled output format %s.\n", debug_d3dformat(format));
    }

    if (IsEqualGUID(codec, &DXVA_ModeH264_VLD_NoFGT))
    {
        static const VkVideoDecodeH264ProfileInfoKHR h264_profile =
        {
            .sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR,
            /* DirectX does not pass profile information anywhere.
             * Instead, the DXVA H.264 specification states that streams must
             * conform to the High profile.
             *
             * The actual stream we'll get might be lower profile than that,
             * but we have no way of knowing. Even delaying until we get the first
             * sample doesn't help us; the profile isn't actually passed in DXVA's
             * marshalled PPS/SPS structure either. */
            .stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH,
            .pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_PROGRESSIVE_KHR,
        };

        profile->pNext = &h264_profile;
        profile->videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
    }
    else
    {
        FIXME("Unhandled codec %s.\n", debugstr_guid(codec));
    }
}

static bool wined3d_decoder_vk_is_h264_decode_supported(const struct wined3d_adapter_vk *adapter_vk)
{
    VkVideoDecodeH264CapabilitiesKHR h264_caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR};
    VkVideoDecodeCapabilitiesKHR decode_caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR};
    VkVideoProfileInfoKHR profile = {.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    VkVideoCapabilitiesKHR caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    const struct wined3d_vk_info *vk_info = &adapter_vk->vk_info;
    VkResult vr;

    if (!vk_info->supported[WINED3D_VK_KHR_VIDEO_DECODE_H264])
        return false;

    /* Only NV12 is required to be supported. */
    fill_vk_profile_info(&profile, &DXVA_ModeH264_VLD_NoFGT, WINED3DFMT_NV12_PLANAR);

    caps.pNext = &decode_caps;
    decode_caps.pNext = &h264_caps;
    if ((vr = VK_CALL(vkGetPhysicalDeviceVideoCapabilitiesKHR(
            adapter_vk->physical_device, &profile, &caps))) != VK_SUCCESS)
    {
        ERR("Failed to query video capabilities, vr %s.\n", wined3d_debug_vkresult(vr));
        return false;
    }

    return true;
}

static void wined3d_decoder_vk_get_profiles(struct wined3d_adapter *adapter, unsigned int *count, GUID *profiles)
{
    const struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(adapter);

    *count = 0;

    if (!adapter_vk->vk_info.supported[WINED3D_VK_KHR_VIDEO_QUEUE])
        return;

    if (wined3d_decoder_vk_is_h264_decode_supported(adapter_vk))
    {
        profiles[(*count)++] = DXVA_ModeH264_VLD_NoFGT;
        /* FIXME: Native GPUs also support DXVA2_ModeH264_VLD_Stereo_NoFGT
         * and DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT. */
    }
}

static void wined3d_decoder_vk_destroy_object(void *object)
{
    struct wined3d_decoder_vk *decoder_vk = object;
    struct wined3d_device_vk *device_vk = wined3d_device_vk(decoder_vk->d.device);
    struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    struct wined3d_context_vk *context_vk;

    TRACE("decoder_vk %p.\n", decoder_vk);

    context_vk = wined3d_context_vk(context_acquire(decoder_vk->d.device, NULL, 0));

    if (decoder_vk->session_memory)
        wined3d_context_vk_free_memory(context_vk, decoder_vk->session_memory);
    else
        VK_CALL(vkFreeMemory(device_vk->vk_device, decoder_vk->vk_session_memory, NULL));

    for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
    {
        struct wined3d_decoder_image_vk *image = &decoder_vk->images[i];

        if (image->output_view)
        {
            wined3d_context_vk_destroy_image(context_vk, &image->output_image);
            wined3d_context_vk_destroy_vk_image_view(context_vk, image->output_view, decoder_vk->command_buffer_id);
        }
        if (decoder_vk->distinct_dpb && image->dpb_view)
        {
            wined3d_context_vk_destroy_image(context_vk, &image->dpb_image);
            wined3d_context_vk_destroy_vk_image_view(context_vk, image->dpb_view, decoder_vk->command_buffer_id);
        }
    }

    if (decoder_vk->layered_dpb)
    {
        wined3d_context_vk_destroy_image(context_vk, &decoder_vk->layered_output_image);
        if (decoder_vk->distinct_dpb)
            wined3d_context_vk_destroy_image(context_vk, &decoder_vk->layered_dpb_image);
    }
    else
    {
        for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
        {
            struct wined3d_decoder_image_vk *image = &decoder_vk->images[i];

            if (image->output_image.vk_image)
                wined3d_context_vk_destroy_image(context_vk, &image->output_image);
            if (decoder_vk->distinct_dpb && image->dpb_image.vk_image)
                wined3d_context_vk_destroy_image(context_vk, &image->dpb_image);
        }
    }

    wined3d_context_vk_destroy_vk_video_session(context_vk, decoder_vk->vk_session, decoder_vk->command_buffer_id);

    free(decoder_vk);
}

static void wined3d_decoder_vk_destroy(struct wined3d_decoder *decoder)
{
    struct wined3d_decoder_vk *decoder_vk = wined3d_decoder_vk(decoder);

    wined3d_decoder_cleanup(decoder);
    wined3d_cs_destroy_object(decoder->device->cs, wined3d_decoder_vk_destroy_object, decoder_vk);
}

static bool wined3d_decoder_vk_create_image(struct wined3d_decoder_vk *decoder_vk,
        struct wined3d_context_vk *context_vk, VkImageUsageFlags usage, VkImageLayout layout,
        struct wined3d_image_vk *image, VkImageView *view)
{
    const struct wined3d_format *output_format = wined3d_get_format(
            decoder_vk->d.device->adapter, decoder_vk->d.desc.output_format, 0);
    VkVideoProfileListInfoKHR profile_list = {.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR};
    unsigned int layer_count = decoder_vk->layered_dpb ? ARRAY_SIZE(decoder_vk->images) : 1;
    VkImageViewCreateInfo view_desc = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkVideoProfileInfoKHR profile = {.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    struct wined3d_device_vk *device_vk = wined3d_device_vk(decoder_vk->d.device);
    VkFormat vk_format = wined3d_format_vk(output_format)->vk_format;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageSubresourceRange vk_range = {0};
    VkResult vr;

    if (!decoder_vk->distinct_dpb)
        usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;

    profile_list.profileCount = 1;
    profile_list.pProfiles = &profile;
    fill_vk_profile_info(&profile, &decoder_vk->d.desc.codec, decoder_vk->d.desc.output_format);

    if (!wined3d_context_vk_create_image(context_vk, VK_IMAGE_TYPE_2D, usage, vk_format,
            decoder_vk->d.desc.width, decoder_vk->d.desc.height, 1, 1, 1, layer_count, 0, &profile_list, image))
    {
        ERR("Failed to create output image.\n");
        return false;
    }

    vk_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vk_range.levelCount = 1;
    vk_range.layerCount = layer_count;

    wined3d_context_vk_image_barrier(context_vk, decoder_vk->command_buffer.vk_command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0,
            VK_IMAGE_LAYOUT_UNDEFINED, layout, image->vk_image, &vk_range);

    if (!view)
        return false;

    view_desc.image = image->vk_image;
    view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_desc.format = vk_format;
    view_desc.subresourceRange = vk_range;

    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, view))))
    {
        ERR("Failed to create image view, vr %s.\n", wined3d_debug_vkresult(vr));
        wined3d_context_vk_destroy_image(context_vk, image);
        return false;
    }

    return true;
}

static void bind_video_session_memory(struct wined3d_decoder_vk *decoder_vk)
{
    struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(decoder_vk->d.device->adapter);
    struct wined3d_device_vk *device_vk = wined3d_device_vk(decoder_vk->d.device);
    const struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    VkVideoSessionMemoryRequirementsKHR *requirements;
    VkBindVideoSessionMemoryInfoKHR *memory;
    struct wined3d_context_vk *context_vk;
    uint32_t count;
    VkResult vr;

    context_vk = wined3d_context_vk(context_acquire(&device_vk->d, NULL, 0));

    VK_CALL(vkGetVideoSessionMemoryRequirementsKHR(device_vk->vk_device, decoder_vk->vk_session, &count, NULL));

    if (!(requirements = calloc(count, sizeof(*requirements))))
    {
        context_release(&context_vk->c);
        return;
    }

    for (uint32_t i = 0; i < count; ++i)
        requirements[i].sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;

    VK_CALL(vkGetVideoSessionMemoryRequirementsKHR(device_vk->vk_device, decoder_vk->vk_session, &count, requirements));

    if (!(memory = calloc(count, sizeof(*memory))))
    {
        free(requirements);
        context_release(&context_vk->c);
        return;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        unsigned int memory_type_idx;

        /* It's not at all clear what memory properties we should be passing
         * here. The spec doesn't say, and it doesn't give a hint as to what's
         * most performant either.
         *
         * Of course, this is a terrible, terrible API generally speaking, and
         * there is no reason for it to exist. */
        memory_type_idx = wined3d_adapter_vk_get_memory_type_index(adapter_vk,
                requirements[i].memoryRequirements.memoryTypeBits, 0);
        if (memory_type_idx == ~0u)
        {
            ERR("Failed to find suitable memory type.\n");
            goto out;
        }
        if (requirements[i].memoryRequirements.alignment > WINED3D_ALLOCATOR_MIN_BLOCK_SIZE)
            ERR("Required alignment is %I64u, but we only support %u.\n",
                    requirements[i].memoryRequirements.alignment, WINED3D_ALLOCATOR_MIN_BLOCK_SIZE);
        decoder_vk->session_memory = wined3d_context_vk_allocate_memory(context_vk,
                memory_type_idx, requirements[i].memoryRequirements.size, &decoder_vk->vk_session_memory);

        memory[i].sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
        memory[i].memoryBindIndex = requirements[i].memoryBindIndex;
        memory[i].memory = decoder_vk->vk_session_memory;
        memory[i].memoryOffset = decoder_vk->session_memory ? decoder_vk->session_memory->offset : 0;
        memory[i].memorySize = requirements[i].memoryRequirements.size;
    }

    if ((vr = VK_CALL(vkBindVideoSessionMemoryKHR(device_vk->vk_device,
            decoder_vk->vk_session, count, memory))) != VK_SUCCESS)
        ERR("Failed to bind memory, vr %s.\n", wined3d_debug_vkresult(vr));

out:
    free(requirements);
    free(memory);
    context_release(&context_vk->c);
}

static void wined3d_decoder_vk_cs_init(void *object)
{
    VkVideoDecodeH264CapabilitiesKHR h264_caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR};
    VkVideoDecodeCapabilitiesKHR decode_caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR};
    VkVideoSessionCreateInfoKHR session_desc = {.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR};
    VkVideoProfileInfoKHR profile = {.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR};
    VkVideoCapabilitiesKHR caps = {.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR};
    struct wined3d_decoder_vk *decoder_vk = object;
    struct wined3d_adapter_vk *adapter_vk = wined3d_adapter_vk(decoder_vk->d.device->adapter);
    struct wined3d_device_vk *device_vk = wined3d_device_vk(decoder_vk->d.device);
    const struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    const struct wined3d_format_vk *output_format;
    VkResult vr;

    output_format = wined3d_format_vk(wined3d_get_format(&adapter_vk->a, decoder_vk->d.desc.output_format, 0));

    session_desc.queueFamilyIndex = device_vk->decode_queue.vk_queue_family_index;
    session_desc.pVideoProfile = &profile;
    session_desc.pictureFormat = output_format->vk_format;
    session_desc.referencePictureFormat = output_format->vk_format;

    fill_vk_profile_info(&profile, &decoder_vk->d.desc.codec, decoder_vk->d.desc.output_format);

    if (IsEqualGUID(&decoder_vk->d.desc.codec, &DXVA_ModeH264_VLD_NoFGT))
    {
        caps.pNext = &decode_caps;
        decode_caps.pNext = &h264_caps;

        vr = VK_CALL(vkGetPhysicalDeviceVideoCapabilitiesKHR(adapter_vk->physical_device, &profile, &caps));
        if (vr != VK_SUCCESS)
        {
            ERR("Device does not support the requested caps, vr %s.\n", wined3d_debug_vkresult(vr));
            return;
        }

        session_desc.maxCodedExtent = caps.maxCodedExtent;
        session_desc.maxDpbSlots = caps.maxDpbSlots;
        session_desc.maxActiveReferencePictures = caps.maxActiveReferencePictures;
        session_desc.pStdHeaderVersion = &caps.stdHeaderVersion;

        if (decode_caps.flags & VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_DISTINCT_BIT_KHR)
            decoder_vk->distinct_dpb = true;

        if (!(caps.flags & VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR))
            decoder_vk->layered_dpb = true;
    }
    else
    {
        ERR("Unsupported codec %s.\n", debugstr_guid(&decoder_vk->d.desc.codec));
        return;
    }

    if ((vr = VK_CALL(vkCreateVideoSessionKHR(device_vk->vk_device,
            &session_desc, NULL, &decoder_vk->vk_session))))
    {
        ERR("Failed to create video session, vr %s.\n", wined3d_debug_vkresult(vr));
        return;
    }

    TRACE("Created video session 0x%s.\n", wine_dbgstr_longlong(decoder_vk->vk_session));

    decoder_vk->bitstream_alignment = caps.minBitstreamBufferSizeAlignment;

    bind_video_session_memory(decoder_vk);

    if (decoder_vk->layered_dpb)
    {
        VkImageUsageFlags usage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        struct wined3d_context_vk *context_vk = &device_vk->context_vk;

        if (!decoder_vk->distinct_dpb)
            usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;

        if (!wined3d_decoder_vk_create_image(decoder_vk, context_vk, usage,
                VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR, &decoder_vk->layered_output_image, NULL))
            return;

        if (decoder_vk->distinct_dpb && !wined3d_decoder_vk_create_image(decoder_vk,
                context_vk, VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR,
                VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, &decoder_vk->layered_dpb_image, NULL))
            return;

        for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
        {
            VkImageViewCreateInfo view_desc = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            struct wined3d_decoder_image_vk *image = &decoder_vk->images[i];

            view_desc.image = decoder_vk->layered_output_image.vk_image;
            view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_desc.format = output_format->vk_format;
            view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_desc.subresourceRange.baseArrayLayer = i;
            view_desc.subresourceRange.layerCount = 1;
            view_desc.subresourceRange.levelCount = 1;
            if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &image->output_view))))
                ERR("Failed to create image view, vr %s.\n", wined3d_debug_vkresult(vr));

            if (decoder_vk->distinct_dpb)
            {
                view_desc.image = decoder_vk->layered_dpb_image.vk_image;
                if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &view_desc, NULL, &image->dpb_view))))
                    ERR("Failed to create image view, vr %s.\n", wined3d_debug_vkresult(vr));
            }
            else
            {
                image->dpb_view = image->output_view;
            }
        }
    }
}

static HRESULT wined3d_decoder_vk_create(struct wined3d_device *device,
        const struct wined3d_decoder_desc *desc, struct wined3d_decoder **decoder)
{
    struct wined3d_decoder_vk *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_decoder_init(&object->d, device, desc)))
    {
        free(object);
        return hr;
    }

    wined3d_cs_init_object(device->cs, wined3d_decoder_vk_cs_init, object);

    TRACE("Created decoder %p.\n", object);
    *decoder = &object->d;

    return WINED3D_OK;
}

static bool get_decode_command_buffer(struct wined3d_decoder_vk *decoder_vk,
        struct wined3d_context_vk *context_vk, struct wined3d_decoder_output_view *view)
{
    const struct wined3d_texture_vk *texture_vk = wined3d_texture_vk(view->texture);

    if (!wined3d_aux_command_pool_vk_get_buffer(context_vk, &context_vk->decode_pool, &decoder_vk->command_buffer))
        return false;

    /* If the output texture in question is in use by the current main CB,
     * we will need this ACB to wait for the main CB to complete.
     *
     * We check this by comparing IDs.
     * Note that if view_vk->command_buffer_id == current_command_buffer.id
     * then the current CB must be active, otherwise the view should not have
     * been referenced to it. */
    if (texture_vk->image.command_buffer_id == context_vk->current_command_buffer.id)
    {
        wined3d_context_vk_submit_command_buffer(context_vk, 0, NULL, NULL,
                1, &decoder_vk->command_buffer.wait_semaphore);
        decoder_vk->needs_wait_semaphore = true;
    }
    else
    {
        /* Submit the main CB anyway. We don't strictly need to do this
         * immediately, but we need to do it before the resource will be used.
         * We also need to do this because resources we're tracking (session,
         * session parameters, reference frames) need to be tied to the next
         * main CB rather than the current one.
         * Submitting now saves us the work of tracking that information,
         * and the resource will probably be used almost immediately anyway. */
        wined3d_context_vk_submit_command_buffer(context_vk, 0, NULL, NULL, 0, NULL);
        decoder_vk->needs_wait_semaphore = false;
    }

    return true;
}

static void submit_decode_command_buffer(struct wined3d_decoder_vk *decoder_vk,
        struct wined3d_context_vk *context_vk)
{
    static const VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    struct wined3d_device_vk *device_vk = wined3d_device_vk(context_vk->c.device);
    VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkResult vr;

    /* We don't strictly need to submit the ACB here. But ffmpeg and gstreamer
     * do, so it's probably the right thing to do.
     *
     * We don't strictly need to submit the main CB here either; we could delay
     * until we use the output resource. However that's a bit more complex to
     * track, and I'm not sure that there's a performance reason *not* to
     * submit early? */

    VK_CALL(vkEndCommandBuffer(decoder_vk->command_buffer.vk_command_buffer));

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &decoder_vk->command_buffer.vk_command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &decoder_vk->command_buffer.signal_semaphore;
    if (decoder_vk->needs_wait_semaphore)
    {
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &decoder_vk->command_buffer.wait_semaphore;
        submit_info.pWaitDstStageMask = &stage_mask;
    }

    if ((vr = VK_CALL(vkQueueSubmit(device_vk->decode_queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE))) < 0)
        ERR("Failed to submit, vr %d.\n", vr);

    /* Mark that the next CB needs to wait on our semaphore. */
    wined3d_array_reserve((void **)&context_vk->wait_semaphores, &context_vk->wait_semaphores_size,
            context_vk->wait_semaphore_count + 1, sizeof(*context_vk->wait_semaphores));
    context_vk->wait_semaphores[context_vk->wait_semaphore_count] = decoder_vk->command_buffer.signal_semaphore;
    wined3d_array_reserve((void **)&context_vk->wait_stages, &context_vk->wait_stages_size,
            context_vk->wait_semaphore_count + 1, sizeof(*context_vk->wait_stages));
    context_vk->wait_stages[context_vk->wait_semaphore_count] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    ++context_vk->wait_semaphore_count;

    /* Retire this buffer. */
    wined3d_aux_command_pool_vk_retire_buffer(context_vk, &context_vk->decode_pool,
            &decoder_vk->command_buffer, context_vk->current_command_buffer.id);
}

static void wined3d_decoder_vk_initialize(struct wined3d_decoder_vk *decoder_vk,
        const struct wined3d_vk_info *vk_info)
{
    static const VkVideoCodingControlInfoKHR control_info =
    {
        .sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR,
        .flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR,
    };

    VK_CALL(vkCmdControlVideoCodingKHR(decoder_vk->command_buffer.vk_command_buffer, &control_info));

    decoder_vk->initialized = true;
}

static StdVideoH264LevelIdc get_vk_h264_level(unsigned int mb_count)
{
    static const struct
    {
        StdVideoH264LevelIdc level;
        unsigned int max_mb_count;
    }
    levels[] =
    {
        {STD_VIDEO_H264_LEVEL_IDC_6_0, 696320},
        {STD_VIDEO_H264_LEVEL_IDC_5_1, 184320},
        {STD_VIDEO_H264_LEVEL_IDC_5_0, 110400},
        {STD_VIDEO_H264_LEVEL_IDC_4_2,  34816},
        {STD_VIDEO_H264_LEVEL_IDC_4_0,  32768},
        {STD_VIDEO_H264_LEVEL_IDC_3_2,  20480},
        {STD_VIDEO_H264_LEVEL_IDC_3_1,  18000},
        {STD_VIDEO_H264_LEVEL_IDC_2_2,   8100},
        {STD_VIDEO_H264_LEVEL_IDC_2_1,   4752},
        {STD_VIDEO_H264_LEVEL_IDC_1_2,   2376},
        {STD_VIDEO_H264_LEVEL_IDC_1_1,    900},
        {STD_VIDEO_H264_LEVEL_IDC_1_0,    396},
    };

    if (mb_count > levels[0].max_mb_count)
    {
        ERR("Macroblock count %u exceeds the limit for any known level!\n", mb_count);
        return STD_VIDEO_H264_LEVEL_IDC_6_2;
    }

    for (unsigned int i = 0; i < ARRAY_SIZE(levels) - 1; ++i)
    {
        if (mb_count > levels[i + 1].max_mb_count)
            return levels[i].level;
    }
    return STD_VIDEO_H264_LEVEL_IDC_1_0;
}

static VkVideoSessionParametersKHR create_h264_params(struct wined3d_decoder_vk *decoder_vk,
        struct wined3d_context_vk *context_vk)
{
    VkVideoDecodeH264SessionParametersCreateInfoKHR h264_create_info =
            {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR};
    VkVideoDecodeH264SessionParametersAddInfoKHR h264_add_info =
            {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR};
    VkVideoSessionParametersCreateInfoKHR create_info =
            {.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR};
    struct wined3d_device_vk *device_vk = wined3d_device_vk(decoder_vk->d.device);
    const struct wined3d_vk_info *vk_info = &device_vk->vk_info;
    const DXVA_PicParams_H264 *h264_params;
    StdVideoH264ScalingLists scaling_lists;
    VkVideoSessionParametersKHR vk_params;
    StdVideoH264SequenceParameterSet sps;
    StdVideoH264PictureParameterSet pps;
    const DXVA_Qmatrix_H264 *matrices;
    VkResult vr;

    h264_params = wined3d_buffer_load_sysmem(decoder_vk->d.parameters, &context_vk->c);
    matrices = wined3d_buffer_load_sysmem(decoder_vk->d.matrix, &context_vk->c);

    create_info.pNext = &h264_create_info;
    create_info.videoSession = decoder_vk->vk_session;

    h264_create_info.maxStdPPSCount = 1;
    h264_create_info.maxStdSPSCount = 1;
    h264_create_info.pParametersAddInfo = &h264_add_info;

    h264_add_info.stdPPSCount = 1;
    h264_add_info.pStdPPSs = &pps;
    h264_add_info.stdSPSCount = 1;
    h264_add_info.pStdSPSs = &sps;

    /* DXVA doesn't pass constraint set information.
     * Since we don't know whether the frame conforms to any given constraint
     * set, we must set all constraint set flags to zero. */
    sps.flags.constraint_set0_flag = 0;
    sps.flags.constraint_set1_flag = 0;
    sps.flags.constraint_set2_flag = 0;
    sps.flags.constraint_set3_flag = 0;
    /* Since we set the profile to High, constraint_set4_flag can be set if
     * frame_mbs_only_flag is 1. */
    sps.flags.constraint_set4_flag = h264_params->frame_mbs_only_flag;
    sps.flags.constraint_set5_flag = 0;
    sps.flags.direct_8x8_inference_flag = h264_params->direct_8x8_inference_flag;
    /* We don't have mb_adaptive_frame_field_flag, but we do have MbaffFrameFlag
     * which is (mb_adaptive_frame_field_flag && !field_pic_flag).
     * If field_pic_flag is 1, we don't know, so we set it to 1, which is the
     * less constrained option. */
    if (!h264_params->field_pic_flag)
        sps.flags.mb_adaptive_frame_field_flag = h264_params->MbaffFrameFlag;
    else
        sps.flags.mb_adaptive_frame_field_flag = 1;
    sps.flags.frame_mbs_only_flag = h264_params->frame_mbs_only_flag;
    sps.flags.delta_pic_order_always_zero_flag = h264_params->delta_pic_order_always_zero_flag;
    /* separate_colour_plane_flag is only relevant to 4:4:4, and DXVA does not
     * support 4:4:4. */
    sps.flags.separate_colour_plane_flag = 0;
    /* We don't have this value, so we have to say it's allowed. */
    sps.flags.gaps_in_frame_num_value_allowed_flag = 1;
    /* The High profile requires this value to be zero. */
    sps.flags.qpprime_y_zero_transform_bypass_flag = 0;
    /* As far as I can tell, frame cropping is just something DXVA defers to
     * the application. Report zero here. */
    sps.flags.frame_cropping_flag = 0;
    /* FIXME: What on earth do we put here? */
    sps.flags.seq_scaling_matrix_present_flag = 0;
    /* We don't have VUI parameters. They are not necessary to construct the
     * actual output image, so reporting 0 here should be okay. */
    sps.flags.vui_parameters_present_flag = 0;
    /* DXVA does not encode profiles. The specification does however state that
     * all video must conform to the High profile. */
    sps.profile_idc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
    sps.level_idc = get_vk_h264_level((h264_params->wFrameWidthInMbsMinus1 + 1)
            * (h264_params->wFrameHeightInMbsMinus1 + 1) * h264_params->num_ref_frames);
    sps.chroma_format_idc = h264_params->chroma_format_idc;
    /* As far as I can tell, the point here is that we can specify multiple
     * SPS / PPS structures in a single frame and then specify which one we
     * actually want to use when calling vkCmdDecodeVideoKHR().
     * This seems pointless when vkCmdDecodeVideoKHR() is only ever called
     * once per frame anyway, and it's not clear that there's any reason to try
     * to batch multiple decode calls per frame, especially when the DXVA API
     * doesn't do this explicitly.
     * Hence it doesn't matter what we set the ID to here as long as it's
     * unique and we use the same ID later. */
    sps.seq_parameter_set_id = 0;
    sps.bit_depth_luma_minus8 = h264_params->bit_depth_luma_minus8;
    sps.bit_depth_chroma_minus8 = h264_params->bit_depth_chroma_minus8;
    sps.log2_max_frame_num_minus4 = h264_params->log2_max_frame_num_minus4;
    sps.pic_order_cnt_type = h264_params->pic_order_cnt_type;
    /* FIXME: What on earth do we put here?
     * Mesa source code suggests drivers don't care. */
    sps.offset_for_non_ref_pic = 0;
    sps.offset_for_top_to_bottom_field = 0;
    sps.log2_max_pic_order_cnt_lsb_minus4 = h264_params->log2_max_pic_order_cnt_lsb_minus4;
    /* FIXME: What on earth do we put here? */
    sps.num_ref_frames_in_pic_order_cnt_cycle = 0;
    /* This was renamed in the spec. */
    sps.max_num_ref_frames = h264_params->num_ref_frames;
    sps.reserved1 = 0;
    sps.pic_width_in_mbs_minus1 = h264_params->wFrameWidthInMbsMinus1;
    if (h264_params->frame_mbs_only_flag)
        sps.pic_height_in_map_units_minus1 = h264_params->wFrameHeightInMbsMinus1;
    else
        sps.pic_height_in_map_units_minus1 = ((h264_params->wFrameHeightInMbsMinus1 + 1) >> 1) - 1;
    /* No frame cropping; see above. */
    sps.frame_crop_left_offset = 0;
    sps.frame_crop_right_offset = 0;
    sps.frame_crop_top_offset = 0;
    sps.frame_crop_bottom_offset = 0;
    sps.reserved2 = 0;
    /* We're setting num_ref_frames_in_pic_order_cnt_cycle = 0, whether that's
     * correct or not, so this array may as well be NULL. */
    sps.pOffsetForRefFrame = NULL;
    /* No scaling lists; see above. */
    sps.pScalingLists = NULL;
    /* No VUI; see above. */
    sps.pSequenceParameterSetVui = NULL;

    pps.flags.transform_8x8_mode_flag = h264_params->transform_8x8_mode_flag;
    pps.flags.redundant_pic_cnt_present_flag = h264_params->redundant_pic_cnt_present_flag;
    pps.flags.constrained_intra_pred_flag = h264_params->constrained_intra_pred_flag;
    pps.flags.deblocking_filter_control_present_flag = h264_params->deblocking_filter_control_present_flag;
    pps.flags.weighted_pred_flag = h264_params->weighted_pred_flag;
    /* This was renamed in the spec. */
    pps.flags.bottom_field_pic_order_in_frame_present_flag = h264_params->pic_order_present_flag;
    pps.flags.entropy_coding_mode_flag = h264_params->entropy_coding_mode_flag;
    /* FIXME: What on earth do we put here? */
    pps.flags.pic_scaling_matrix_present_flag = 1;
    /* See sps.seq_parameter_set_id. */
    pps.seq_parameter_set_id = 0;
    pps.pic_parameter_set_id = 0;
    /* This is an odd one. The Vulkan API doesn't seem to have a way to specify
     * num_ref_idx_l*_active_minus1 or num_ref_idx_active_override_flag.
     * GStreamer and ffmpeg both treat these two fields as being identical. */
    pps.num_ref_idx_l0_default_active_minus1 = h264_params->num_ref_idx_l0_active_minus1;
    pps.num_ref_idx_l1_default_active_minus1 = h264_params->num_ref_idx_l1_active_minus1;
    pps.weighted_bipred_idc = h264_params->weighted_bipred_idc;
    pps.pic_init_qp_minus26 = h264_params->pic_init_qp_minus26;
    pps.pic_init_qs_minus26 = h264_params->pic_init_qs_minus26;
    pps.chroma_qp_index_offset = h264_params->chroma_qp_index_offset;
    pps.second_chroma_qp_index_offset = h264_params->second_chroma_qp_index_offset;
    /* No scaling lists; see above. */
    pps.pScalingLists = &scaling_lists;

    /* We supply all six 4x4 matrices, and the first two 8x8 matrices. */
    scaling_lists.scaling_list_present_mask = wined3d_mask_from_size(8);
    /* FIXME: Should this be the inverse? The spec is hard to read. */
    scaling_lists.use_default_scaling_matrix_mask = 0;
    memcpy(scaling_lists.ScalingList4x4, matrices->bScalingLists4x4, sizeof(matrices->bScalingLists4x4));
    memcpy(scaling_lists.ScalingList8x8, matrices->bScalingLists8x8, sizeof(matrices->bScalingLists8x8));

    if ((vr = VK_CALL(vkCreateVideoSessionParametersKHR(device_vk->vk_device,
            &create_info, NULL, &vk_params))) == VK_SUCCESS)
        return vk_params;
    ERR("Failed to create parameters, vr %d.\n", vr);
    return VK_NULL_HANDLE;
}

struct h264_reference_info
{
    VkVideoPictureResourceInfoKHR picture_info;
    VkVideoDecodeH264DpbSlotInfoKHR h264_dpb_slot;
    StdVideoDecodeH264ReferenceInfo h264_reference;
};

static void init_h264_reference_info(VkVideoReferenceSlotInfoKHR *reference_slot,
        struct h264_reference_info *info, struct wined3d_decoder_vk *decoder_vk, unsigned int slot_index)
{
    reference_slot->sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
    reference_slot->pNext = &info->h264_dpb_slot;
    reference_slot->slotIndex = slot_index;
    reference_slot->pPictureResource = &info->picture_info;

    info->picture_info.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    info->picture_info.codedExtent.width = decoder_vk->d.desc.width;
    info->picture_info.codedExtent.height = decoder_vk->d.desc.height;
    info->picture_info.baseArrayLayer = 0;
    info->picture_info.imageViewBinding = decoder_vk->images[slot_index].dpb_view;

    info->h264_dpb_slot.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
    info->h264_dpb_slot.pStdReferenceInfo = &info->h264_reference;
}

static bool find_reference_slot(struct wined3d_decoder_vk *decoder_vk,
        uint8_t dxva_index, unsigned int *vulkan_index)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
    {
        if (decoder_vk->images[i].dpb_view && decoder_vk->images[i].dxva_index == dxva_index)
        {
            *vulkan_index = i;
            return true;
        }
    }

    ERR("Reference index %u was never written.\n", dxva_index);
    return false;
}

static bool find_unused_slot(struct wined3d_decoder_vk *decoder_vk, unsigned int *vulkan_index)
{
    for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
    {
        if (!decoder_vk->images[i].used)
        {
            *vulkan_index = i;
            return true;
        }
    }

    return false;
}

static void wined3d_decoder_vk_blit_output(struct wined3d_decoder_vk *decoder_vk, struct wined3d_context_vk *context_vk,
        struct wined3d_decoder_output_view_vk *output_view_vk, unsigned int slot_index)
{
    struct wined3d_texture_vk *texture_vk = wined3d_texture_vk(output_view_vk->v.texture);
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkCommandBuffer command_buffer;
    VkImageCopy regions[2] = {0};
    VkImageLayout dst_layout;
    VkImage src_image;

    command_buffer = wined3d_context_vk_get_command_buffer(context_vk);

    if (texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        dst_layout = VK_IMAGE_LAYOUT_GENERAL;
    else
        dst_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    regions[0].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    regions[0].srcSubresource.layerCount = 1;
    regions[0].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
    regions[0].dstSubresource.baseArrayLayer = output_view_vk->v.desc.u.texture.layer_idx;
    regions[0].dstSubresource.layerCount = 1;
    regions[0].extent.width = texture_vk->t.resource.width;
    regions[0].extent.height = texture_vk->t.resource.height;
    regions[0].extent.depth = 1;

    if (decoder_vk->layered_dpb)
    {
        src_image = decoder_vk->layered_output_image.vk_image;
        regions[0].srcSubresource.baseArrayLayer = slot_index;
    }
    else
    {
        src_image = decoder_vk->images[slot_index].output_image.vk_image;
        regions[0].srcSubresource.baseArrayLayer = 0;
    }

    regions[1] = regions[0];
    regions[1].srcSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    regions[1].dstSubresource.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
    regions[1].extent.width /= 2;
    regions[1].extent.height /= 2;

    VK_CALL(vkCmdCopyImage(command_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture_vk->image.vk_image, dst_layout, 2, regions));
}

static void wined3d_decoder_vk_decode_h264(struct wined3d_decoder_vk *decoder_vk, struct wined3d_context_vk *context_vk,
        struct wined3d_decoder_output_view_vk *output_view_vk, VkVideoDecodeInfoKHR *decode_info,
        const DXVA_PicParams_H264 *h264_params, const void *slice_control, unsigned int slice_control_size)
{
    VkVideoDecodeH264PictureInfoKHR vk_h264_picture = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR};
    VkVideoReferenceSlotInfoKHR setup_reference_slot = {.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR};
    VkVideoBeginCodingInfoKHR begin_info = {.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR};
    VkVideoEndCodingInfoKHR end_info = {.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR};
    VkVideoReferenceSlotInfoKHR reference_slots[MAX_VK_DECODE_REFERENCE_SLOTS + 1] = {0};
    struct h264_reference_info references[MAX_VK_DECODE_REFERENCE_SLOTS] = {0};
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    struct h264_reference_info setup_reference = {0};
    StdVideoDecodeH264ReferenceInfo *h264_reference;
    StdVideoDecodeH264PictureInfo h264_picture;
    struct wined3d_decoder_image_vk *image;
    VkVideoSessionParametersKHR vk_params;
    unsigned int slot_count = 0;
    unsigned int slot_index;
    uint32_t *slice_offsets;
    size_t slice_count;

    if (decoder_vk->d.desc.long_slice_info)
    {
        const DXVA_Slice_H264_Long *slices = slice_control;

        slice_count = slice_control_size / sizeof(*slices);
        if (!(slice_offsets = malloc(slice_count * sizeof(*slice_offsets))))
            return;

        for (size_t i = 0; i < slice_count; ++i)
            slice_offsets[i] = slices[i].BSNALunitDataLocation;
    }
    else
    {
        const DXVA_Slice_H264_Short *slices = slice_control;

        slice_count = slice_control_size / sizeof(*slices);
        if (!(slice_offsets = malloc(slice_count * sizeof(*slice_offsets))))
            return;

        for (size_t i = 0; i < slice_count; ++i)
            slice_offsets[i] = slices[i].BSNALunitDataLocation;
    }

    if (!(vk_params = create_h264_params(decoder_vk, context_vk)))
    {
        free(slice_offsets);
        return;
    }

    /* We cannot use the DXVA index or the frame number as an reference slot
     * index. Vulkan requires that reference slot indices be less than the
     * total number of reference images, and drivers impose a maximum of 16
     * reference images for H.264. However, the DXVA index and frame number may
     * both exceed 16.
     *
     * Fortunately, DXVA specifies that references must be provided if they will
     * be used for decoding this or any subsequent frames. That is, if an frame
     * is not listed in the DXVA references, we can use it as the slot index for
     * this output image.
     *
     * Therefore we mark all images as "unused" at the beginning of this
     * function, then mark images as "used" when enumerating references.
     * Afterward we pick the first unused slot, which will be used for this
     * image. */
    for (unsigned int i = 0; i < ARRAY_SIZE(decoder_vk->images); ++i)
        decoder_vk->images[i].used = false;

    begin_info.videoSession = decoder_vk->vk_session;
    begin_info.videoSessionParameters = vk_params;

    TRACE("Decoding frame %02x/%02x, RefPicFlag %#x, reference frames",
            h264_params->CurrPic.bPicEntry, h264_params->frame_num, h264_params->RefPicFlag);

    for (unsigned int i = 0; i < ARRAY_SIZE(h264_params->RefFrameList); ++i)
    {
        unsigned int field_flags = ((h264_params->UsedForReferenceFlags >> (2 * i)) & 3u);

        if (h264_params->RefFrameList[i].bPicEntry == 0xff)
            continue;

        TRACE(" %02x/%02x", h264_params->RefFrameList[i].bPicEntry, h264_params->FrameNumList[i]);

        /* NVidia's DXVA implementation apparently expects each frame to appear
         * in its own references list. Vulkan does not expect or need this. */
        if (h264_params->RefFrameList[i].Index7Bits == h264_params->CurrPic.Index7Bits)
            continue;

        if (!find_reference_slot(decoder_vk, h264_params->RefFrameList[i].Index7Bits, &slot_index))
            goto out;
        image = &decoder_vk->images[slot_index];

        image->used = true;

        if (decoder_vk->layered_dpb)
        {
            if (decoder_vk->distinct_dpb)
                wined3d_context_vk_reference_image(context_vk, &decoder_vk->layered_dpb_image);
            else
                wined3d_context_vk_reference_image(context_vk, &decoder_vk->layered_output_image);
        }
        else
        {
            if (decoder_vk->distinct_dpb)
                wined3d_context_vk_reference_image(context_vk, &image->dpb_image);
            else
                wined3d_context_vk_reference_image(context_vk, &image->output_image);
        }

        init_h264_reference_info(&reference_slots[slot_count], &references[slot_count], decoder_vk, slot_index);

        h264_reference = &references[slot_count].h264_reference;

        /* If it's a frame reference, DXVA sets both flags, but Vulkan
         * is supposed to set neither flag. */
        h264_reference->flags.top_field_flag = (field_flags == 1);
        h264_reference->flags.bottom_field_flag = (field_flags == 2);
        h264_reference->flags.used_for_long_term_reference = h264_params->RefFrameList[i].AssociatedFlag;
        h264_reference->flags.is_non_existing = !!(h264_params->NonExistingFrameFlags & (1u << i));
        /* Vulkan is underspecified here; FrameNum is only defined for
         * short-term references. Microsoft's DXVA H.264 specification actually
         * says this is FrameNum *or* LongTermFrameIdx.
         * GStreamer and ffmpeg seem to broadly agree that the Vulkan field is
         * overloaded in the same way.
         * [GStreamer however puts PicNum / LongTermPicNum here instead.] */
        h264_reference->FrameNum = h264_params->FrameNumList[i];
        h264_reference->PicOrderCnt[0] = h264_params->FieldOrderCntList[i][0];
        h264_reference->PicOrderCnt[1] = h264_params->FieldOrderCntList[i][1];

        ++slot_count;
    }

    TRACE(".\n");

    /* Current decoding reference slot. */

    if (!find_unused_slot(decoder_vk, &slot_index))
    {
        ERR("No unused reference slot.\n");
        goto out;
    }
    image = &decoder_vk->images[slot_index];

    image->dxva_index = h264_params->CurrPic.Index7Bits;

    if (!image->output_view)
    {
        VkImageUsageFlags usage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        if (!decoder_vk->distinct_dpb)
            usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;

        if (!wined3d_decoder_vk_create_image(decoder_vk, context_vk, usage,
                VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR, &image->output_image, &image->output_view))
            goto out;

        if (decoder_vk->distinct_dpb)
        {
            if (!wined3d_decoder_vk_create_image(decoder_vk, context_vk, VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR,
                    VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR, &image->dpb_image, &image->dpb_view))
                goto out;
            wined3d_context_vk_reference_image(context_vk, &image->dpb_image);
        }
        else
        {
            image->dpb_view = image->output_view;
        }
    }
    if (decoder_vk->layered_dpb)
        wined3d_context_vk_reference_image(context_vk, &decoder_vk->layered_output_image);
    else
        wined3d_context_vk_reference_image(context_vk, &image->output_image);

    init_h264_reference_info(&setup_reference_slot, &setup_reference, decoder_vk, slot_index);

    h264_reference = &setup_reference.h264_reference;
    /* FIXME: What on earth do we put here? For some reason DXVA supplies these
     * flags for reference frames, but not for the current frame.
     * Mesa source code suggests that drivers don't care about anything in
     * pSetupReferenceSlot other than the slot index and image view,
     * and in fact don't even need any of VkVideoBeginCodingInfoKHR at all
     * for decoding, so just fill these as zero for now... */
    h264_reference->flags.top_field_flag = 0;
    h264_reference->flags.bottom_field_flag = 0;
    h264_reference->flags.used_for_long_term_reference = 0;
    h264_reference->flags.is_non_existing = 0;
    /* See above s.v. FrameNum.
     * Yes, this information is duplicated. */
    h264_reference->FrameNum = h264_params->frame_num;
    h264_reference->PicOrderCnt[0] = h264_params->CurrFieldOrderCnt[0];
    h264_reference->PicOrderCnt[1] = h264_params->CurrFieldOrderCnt[1];

    /* We have to duplicate this information into the reference slot array
     * for vkCmdBeginVideoCodingKHR, but marked as in inactive reference. */
    reference_slots[slot_count] = setup_reference_slot;
    reference_slots[slot_count].slotIndex = -1;

    begin_info.referenceSlotCount = slot_count + 1;
    begin_info.pReferenceSlots = reference_slots;

    vk_h264_picture.pStdPictureInfo = &h264_picture;
    vk_h264_picture.sliceCount = slice_count;
    vk_h264_picture.pSliceOffsets = slice_offsets;

    decode_info->pNext = &vk_h264_picture;
    decode_info->pSetupReferenceSlot = &setup_reference_slot;
    decode_info->pReferenceSlots = reference_slots;
    decode_info->referenceSlotCount = slot_count;
    decode_info->dstPictureResource.imageViewBinding = image->output_view;

    h264_picture.flags.field_pic_flag = h264_params->field_pic_flag;
    /* ffmpeg treats these two as identical. */
    h264_picture.flags.is_intra = h264_params->IntraPicFlag;
    /* FIXME: What on earth do we put here?
     * Mesa source code suggests drivers don't care. */
    h264_picture.flags.IdrPicFlag = 0;
    h264_picture.flags.bottom_field_flag = h264_params->CurrPic.AssociatedFlag;
    /* This is not documented very well, but GStreamer and ffmpeg seem to agree
     * that this is what this means. */
    h264_picture.flags.is_reference = h264_params->RefPicFlag;
    /* FIXME: What on earth do we put here?
     * Mesa source code suggests drivers don't care. */
    h264_picture.flags.complementary_field_pair = 0;
    /* See above s.v. seq_parameter_set_id. */
    h264_picture.seq_parameter_set_id = 0;
    h264_picture.pic_parameter_set_id = 0;
    h264_picture.reserved1 = 0;
    h264_picture.reserved2 = 0;
    h264_picture.frame_num = h264_params->frame_num;
    /* See above s.v. IdrPicFlag. */
    h264_picture.idr_pic_id = 0;
    h264_picture.PicOrderCnt[0] = h264_params->CurrFieldOrderCnt[0];
    h264_picture.PicOrderCnt[1] = h264_params->CurrFieldOrderCnt[1];

    VK_CALL(vkCmdBeginVideoCodingKHR(decoder_vk->command_buffer.vk_command_buffer, &begin_info));
    if (!decoder_vk->initialized)
        wined3d_decoder_vk_initialize(decoder_vk, vk_info);
    VK_CALL(vkCmdDecodeVideoKHR(decoder_vk->command_buffer.vk_command_buffer, decode_info));
    VK_CALL(vkCmdEndVideoCodingKHR(decoder_vk->command_buffer.vk_command_buffer, &end_info));

    submit_decode_command_buffer(decoder_vk, context_vk);

    wined3d_decoder_vk_blit_output(decoder_vk, context_vk, output_view_vk, slot_index);

out:
    wined3d_context_vk_destroy_vk_video_parameters(context_vk, vk_params, context_vk->current_command_buffer.id);
    free(slice_offsets);
}

static void wined3d_decoder_vk_decode(struct wined3d_context *context, struct wined3d_decoder *decoder,
        struct wined3d_decoder_output_view *output_view,
        unsigned int bitstream_size, unsigned int slice_control_size)
{
    struct wined3d_decoder_output_view_vk *output_view_vk = wined3d_decoder_output_view_vk(output_view);
    VkVideoDecodeInfoKHR decode_info = {.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR};
    unsigned int sub_resource_idx = output_view_vk->v.desc.u.texture.layer_idx;
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct wined3d_decoder_vk *decoder_vk = wined3d_decoder_vk(decoder);
    struct wined3d_texture *texture = output_view_vk->v.texture;
    const void *parameters, *slice_control;
    struct wined3d_bo_vk *bitstream_bo;

    wined3d_buffer_load_location(decoder_vk->d.bitstream, &context_vk->c, WINED3D_LOCATION_BUFFER);
    bitstream_bo = wined3d_bo_vk(decoder_vk->d.bitstream->buffer_object);

    parameters = wined3d_buffer_load_sysmem(decoder_vk->d.parameters, &context_vk->c);
    slice_control = wined3d_buffer_load_sysmem(decoder_vk->d.slice_control, &context_vk->c);

    wined3d_texture_prepare_location(texture, sub_resource_idx, &context_vk->c, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);

    if (!get_decode_command_buffer(decoder_vk, context_vk, output_view))
        return;

    decode_info.srcBuffer = bitstream_bo->vk_buffer;
    decode_info.srcBufferOffset = bitstream_bo->b.buffer_offset;
    decode_info.srcBufferRange = align(bitstream_size, decoder_vk->bitstream_alignment);
    decode_info.dstPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
    decode_info.dstPictureResource.codedExtent.width = decoder_vk->d.desc.width;
    decode_info.dstPictureResource.codedExtent.height = decoder_vk->d.desc.height;
    decode_info.dstPictureResource.baseArrayLayer = 0;

    wined3d_decoder_vk_decode_h264(decoder_vk, context_vk, output_view_vk,
            &decode_info, parameters, slice_control, slice_control_size);

    wined3d_context_vk_reference_bo(context_vk, bitstream_bo);
    wined3d_context_vk_reference_texture(context_vk, wined3d_texture_vk(texture));
    decoder_vk->command_buffer_id = context_vk->current_command_buffer.id;
}

const struct wined3d_decoder_ops wined3d_decoder_vk_ops =
{
    .get_profiles = wined3d_decoder_vk_get_profiles,
    .create = wined3d_decoder_vk_create,
    .destroy = wined3d_decoder_vk_destroy,
    .decode = wined3d_decoder_vk_decode,
};

struct wined3d_resource * CDECL wined3d_decoder_get_buffer(
        struct wined3d_decoder *decoder, enum wined3d_decoder_buffer_type type)
{
    switch (type)
    {
        case WINED3D_DECODER_BUFFER_BITSTREAM:
            return &decoder->bitstream->resource;

        case WINED3D_DECODER_BUFFER_INVERSE_QUANTIZATION_MATRIX:
            return &decoder->matrix->resource;

        case WINED3D_DECODER_BUFFER_PICTURE_PARAMETERS:
            return &decoder->parameters->resource;

        case WINED3D_DECODER_BUFFER_SLICE_CONTROL:
            return &decoder->slice_control->resource;
    }

    FIXME("Unhandled buffer type %#x.\n", type);
    return NULL;
}

HRESULT CDECL wined3d_decoder_begin_frame(struct wined3d_decoder *decoder,
        struct wined3d_decoder_output_view *view)
{
    TRACE("decoder %p, view %p.\n", decoder, view);

    if (decoder->output_view)
    {
        ERR("Already in frame.\n");
        return E_INVALIDARG;
    }

    wined3d_decoder_output_view_incref(view);
    decoder->output_view = view;

    return S_OK;
}

HRESULT CDECL wined3d_decoder_end_frame(struct wined3d_decoder *decoder)
{
    TRACE("decoder %p.\n", decoder);

    if (!decoder->output_view)
    {
        ERR("Not in frame.\n");
        return E_INVALIDARG;
    }

    wined3d_decoder_output_view_decref(decoder->output_view);
    decoder->output_view = NULL;

    return S_OK;
}

HRESULT CDECL wined3d_decoder_decode(struct wined3d_decoder *decoder,
        unsigned int bitstream_size, unsigned int slice_control_size)
{
    TRACE("decoder %p, bitstream_size %u, slice_control_size %u.\n", decoder, bitstream_size, slice_control_size);

    if (!decoder->output_view)
    {
        ERR("Not in frame.\n");
        return E_INVALIDARG;
    }

    wined3d_cs_emit_decode(decoder, decoder->output_view, bitstream_size, slice_control_size);
    return S_OK;
}
