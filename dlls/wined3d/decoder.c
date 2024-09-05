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

struct wined3d_decoder_vk
{
    struct wined3d_decoder d;
    VkVideoSessionKHR vk_session;
    uint64_t command_buffer_id;
    struct wined3d_allocator_block *session_memory;
    VkDeviceMemory vk_session_memory;
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

    if (!(caps.flags & VK_VIDEO_CAPABILITY_SEPARATE_REFERENCE_IMAGES_BIT_KHR))
    {
        FIXME("Implementation requires a layered DPB.\n");
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

    wined3d_context_vk_destroy_vk_video_session(context_vk, decoder_vk->vk_session, decoder_vk->command_buffer_id);

    free(decoder_vk);
}

static void wined3d_decoder_vk_destroy(struct wined3d_decoder *decoder)
{
    struct wined3d_decoder_vk *decoder_vk = wined3d_decoder_vk(decoder);

    wined3d_decoder_cleanup(decoder);
    wined3d_cs_destroy_object(decoder->device->cs, wined3d_decoder_vk_destroy_object, decoder_vk);
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

    bind_video_session_memory(decoder_vk);
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

const struct wined3d_decoder_ops wined3d_decoder_vk_ops =
{
    .get_profiles = wined3d_decoder_vk_get_profiles,
    .create = wined3d_decoder_vk_create,
    .destroy = wined3d_decoder_vk_destroy,
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
