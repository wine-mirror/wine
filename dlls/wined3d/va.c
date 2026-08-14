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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#if defined(SONAME_LIBVA) && defined(SONAME_LIBVA_DRM)

#include "initguid.h"
#include "unixlib.h"
#include "dxva.h"
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>
#include "ntgdi.h"
#include "wine/vulkan_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

struct va_context
{
    void *libva_handle, *libva_drm_handle;
#define MAKE_FUNCPTR(f) typeof(f) *f
    MAKE_FUNCPTR(vaBeginPicture);
    MAKE_FUNCPTR(vaCreateBuffer);
    MAKE_FUNCPTR(vaCreateConfig);
    MAKE_FUNCPTR(vaCreateContext);
    MAKE_FUNCPTR(vaCreateSurfaces);
    MAKE_FUNCPTR(vaDestroyBuffer);
    MAKE_FUNCPTR(vaDestroyConfig);
    MAKE_FUNCPTR(vaDestroyContext);
    MAKE_FUNCPTR(vaDestroySurfaces);
    MAKE_FUNCPTR(vaEndPicture);
    MAKE_FUNCPTR(vaExportSurfaceHandle);
    MAKE_FUNCPTR(vaGetDisplayDRM);
    MAKE_FUNCPTR(vaInitialize);
    MAKE_FUNCPTR(vaMaxNumProfiles);
    MAKE_FUNCPTR(vaQueryConfigProfiles);
    MAKE_FUNCPTR(vaRenderPicture);
    MAKE_FUNCPTR(vaSyncSurface);
    MAKE_FUNCPTR(vaTerminate);
#undef MAKE_FUNCPTR

    VADisplay display;
    int fd;
};

struct va_decoder
{
    struct va_context ctx;
    VAConfigID config;
    VAContextID context;
    VASurfaceID surfaces[VA_DECODER_SURFACE_COUNT];
    VkDeviceMemory vk_memory[VA_DECODER_SURFACE_COUNT];
    struct
    {
        bool valid;
        uint8_t dxva_index;
    } references[VA_DECODER_SURFACE_COUNT];
    bool long_slice_info;
};

static void close_va_display(struct va_context *ctx)
{
    if (ctx->fd != -1)
        close(ctx->fd);
    ctx->vaTerminate(ctx->display);
    dlclose(ctx->libva_drm_handle);
    dlclose(ctx->libva_handle);
}

static NTSTATUS open_va_display(UINT64 handle, struct va_context *ctx)
{
    VkPhysicalDeviceDrmPropertiesEXT drm_properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT};
    VkPhysicalDeviceProperties2 properties = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    const struct vulkan_physical_device *device = vulkan_physical_device_from_handle((VkPhysicalDevice)handle);
    int major, minor;
    VAStatus status;
    char *path;

    ctx->display = 0;
    ctx->fd = -1;

    if (!device->extensions.has_VK_EXT_external_memory_dma_buf
            || !device->extensions.has_VK_EXT_image_drm_format_modifier
            || !device->extensions.has_VK_EXT_physical_device_drm)
        return E_NOTIMPL;

    if (!(ctx->libva_handle = dlopen(SONAME_LIBVA, RTLD_NOW)))
        return E_NOTIMPL;
    if (!(ctx->libva_drm_handle = dlopen(SONAME_LIBVA_DRM, RTLD_NOW)))
    {
        dlclose(ctx->libva_handle);
        return E_NOTIMPL;
    }

#define LOAD_FUNCPTR(f) \
    if (!(ctx->f = dlsym(ctx->libva_handle, #f))) \
    { \
        ERR("Failed to load function %s.\n", #f); \
        goto fail; \
    }
    LOAD_FUNCPTR(vaBeginPicture);
    LOAD_FUNCPTR(vaCreateBuffer);
    LOAD_FUNCPTR(vaCreateConfig);
    LOAD_FUNCPTR(vaCreateContext);
    LOAD_FUNCPTR(vaCreateSurfaces);
    LOAD_FUNCPTR(vaDestroyBuffer);
    LOAD_FUNCPTR(vaDestroyConfig);
    LOAD_FUNCPTR(vaDestroyContext);
    LOAD_FUNCPTR(vaDestroySurfaces);
    LOAD_FUNCPTR(vaEndPicture);
    LOAD_FUNCPTR(vaExportSurfaceHandle);
    LOAD_FUNCPTR(vaInitialize);
    LOAD_FUNCPTR(vaMaxNumProfiles);
    LOAD_FUNCPTR(vaQueryConfigProfiles);
    LOAD_FUNCPTR(vaRenderPicture);
    LOAD_FUNCPTR(vaSyncSurface);
    LOAD_FUNCPTR(vaTerminate);
#undef LOAD_FUNCPTR

    if (!(ctx->vaGetDisplayDRM = dlsym(ctx->libva_drm_handle, "vaGetDisplayDRM")))
    {
        ERR("Failed to load function vaGetDisplayDRM.\n");
        goto fail;
    }

    properties.pNext = &drm_properties;
    device->instance->p_vkGetPhysicalDeviceProperties2KHR(device->host.physical_device, &properties);

    if (drm_properties.hasPrimary)
    {
        asprintf(&path, "/dev/dri/card%"PRIu64, drm_properties.primaryMinor);
    }
    else if (drm_properties.hasRender)
    {
        asprintf(&path, "/dev/dri/renderD%"PRIu64, drm_properties.primaryMinor);
    }
    else
    {
        ERR("Vulkan device has neither a primary nor render node.\n");
        goto fail;
    }

    if ((ctx->fd = open(path, O_RDWR | O_CLOEXEC)) < 0)
    {
        ERR("Failed to open %s: %s\n", path, strerror(errno));
        free(path);
        goto fail;
    }
    free(path);

    ctx->display = ctx->vaGetDisplayDRM(ctx->fd);
    if ((status = ctx->vaInitialize(ctx->display, &major, &minor)) != VA_STATUS_SUCCESS)
    {
        ERR("Failed to initialize VA, error %#x.\n", status);
        goto fail;
    }

    return S_OK;

fail:
    close_va_display(ctx);
    return E_NOTIMPL;
}

static NTSTATUS va_get_profiles_vk(void *args)
{
    struct va_get_profiles_vk_params *params = args;
    unsigned int *count = (unsigned int *)(uintptr_t)params->count;
    GUID *profiles = (GUID *)(uintptr_t)params->profiles;
    VAProfile *va_profiles;
    struct va_context ctx;
    NTSTATUS status;
    int max_count;

    if ((status = open_va_display(params->physical_device, &ctx)))
        return status;

    max_count = ctx.vaMaxNumProfiles(ctx.display);
    va_profiles = malloc(max_count * sizeof(*va_profiles));
    ctx.vaQueryConfigProfiles(ctx.display, va_profiles, &max_count);

    for (int i = 0; i < max_count; ++i)
    {
        if (va_profiles[i] == VAProfileH264High)
        {
            profiles[(*count)++] = DXVA_ModeH264_VLD_NoFGT;
            /* FIXME: Native GPUs also support DXVA2_ModeH264_VLD_Stereo_NoFGT
             * and DXVA2_ModeH264_VLD_Stereo_Progressive_NoFGT. */
        }
    }

    free(va_profiles);
    close_va_display(&ctx);
    return S_OK;
}

static NTSTATUS va_decoder_create_vk(void *args)
{
    struct va_decoder_create_vk_params *params = args;
    VkImageDrmFormatModifierExplicitCreateInfoEXT drm_format_desc = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT};
    VkExternalMemoryImageCreateInfo external_image_desc = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    VkMemoryDedicatedAllocateInfo dedicated_desc = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    VkMemoryDedicatedRequirements dedicated_reqs = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
    VkImageMemoryRequirementsInfo2 reqs_desc = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2};
    VkMemoryFdPropertiesKHR fd_properties = {.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};
    VkImportMemoryFdInfoKHR fd_desc = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    VkMemoryRequirements2 memory_reqs = {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    VkMemoryAllocateInfo memory_desc = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    struct vulkan_device *device = vulkan_device_from_handle((VkDevice)params->device);
    VkImageCreateInfo image_desc = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VADRMPRIMESurfaceDescriptor descriptor;
    VASurfaceAttrib surface_attribs[1];
    VAConfigAttrib config_attribs[1];
    unsigned int format, fourcc;
    struct va_decoder *decoder;
    struct va_context *ctx;
    VAStatus status;
    VkResult vr;

    if (params->desc.output_format == WINED3DFMT_NV12_PLANAR)
    {
        format = VA_RT_FORMAT_YUV420;
        fourcc = VA_FOURCC_NV12;
        image_desc.format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    }
    else
    {
        FIXME("Unhandled output format %#x.\n", params->desc.output_format);
        return E_NOTIMPL;
    }

    if (!(decoder = calloc(1, sizeof(*decoder))))
        return E_OUTOFMEMORY;
    ctx = &decoder->ctx;

    if ((status = open_va_display(params->physical_device, ctx)))
        return status;

    config_attribs[0].type = VAConfigAttribRTFormat;
    config_attribs[0].value = format;

    if ((status = ctx->vaCreateConfig(ctx->display, VAProfileH264High, VAEntrypointVLD,
            config_attribs, ARRAY_SIZE(config_attribs), &decoder->config)) != VA_STATUS_SUCCESS)
    {
        ERR("Failed to create config, error %#x.\n", status);
        close_va_display(ctx);
        goto fail;
    }

    surface_attribs[0].type = VASurfaceAttribPixelFormat;
    surface_attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    surface_attribs[0].value.type = VAGenericValueTypeInteger;
    surface_attribs[0].value.value.i = fourcc;

    if ((status = ctx->vaCreateSurfaces(ctx->display, format, params->width, params->height, decoder->surfaces,
            VA_DECODER_SURFACE_COUNT, surface_attribs, ARRAY_SIZE(surface_attribs))) != VA_STATUS_SUCCESS)
    {
        ERR("Failed to create surfaces, error %#x.\n", status);
        ctx->vaDestroyConfig(ctx->display, decoder->config);
        close_va_display(ctx);
        goto fail;
    }

    if ((status = ctx->vaCreateContext(ctx->display, decoder->config, params->width, params->height, VA_PROGRESSIVE,
            decoder->surfaces, ARRAY_SIZE(decoder->surfaces), &decoder->context)) != VA_STATUS_SUCCESS)
    {
        ERR("Failed to create context, error %#x.\n", status);
        ctx->vaDestroySurfaces(ctx->display, decoder->surfaces, VA_DECODER_SURFACE_COUNT);
        ctx->vaDestroyConfig(ctx->display, decoder->config);
        close_va_display(ctx);
        goto fail;
    }

    image_desc.imageType = VK_IMAGE_TYPE_2D;
    image_desc.extent.width = params->width;
    image_desc.extent.height = params->height;
    image_desc.extent.depth = 1;
    image_desc.mipLevels = 1;
    image_desc.arrayLayers = 1;
    image_desc.samples = 1;
    image_desc.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    image_desc.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_desc.pNext = &external_image_desc;
    external_image_desc.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    external_image_desc.pNext = &drm_format_desc;

    memory_desc.pNext = &fd_desc;
    fd_desc.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

    for (unsigned int i = 0; i < VA_DECODER_SURFACE_COUNT; ++i)
    {
        VkSubresourceLayout plane_layouts[4] = {0};
        VkImage image;
        DWORD index;

        if ((status = ctx->vaExportSurfaceHandle(ctx->display, decoder->surfaces[i], VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_COMPOSED_LAYERS, &descriptor)))
            ERR("Failed to export, error %#x.\n", status);

        if (descriptor.num_objects != 1)
            FIXME("Unhandled object count %u.\n", descriptor.num_objects);
        /* We passed VA_EXPORT_SURFACE_COMPOSED_LAYERS; this shouldn't happen. */
        if (descriptor.num_layers != 1)
            ERR("Unexpected layer count %u.\n", descriptor.num_layers);

        if ((vr = device->p_vkGetMemoryFdPropertiesKHR(device->host.device,
                VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, descriptor.objects[0].fd, &fd_properties)) != VK_SUCCESS)
        {
            ERR("Failed to get fd properties, vr %#x.\n", vr);
            continue;
        }

        if (!BitScanForward(&index, fd_properties.memoryTypeBits))
        {
            ERR("No valid type bits.\n");
            return E_NOTIMPL;
        }
        memory_desc.memoryTypeIndex = index;

        memory_desc.allocationSize = descriptor.objects[0].size;

        for (unsigned int j = 0; j < descriptor.layers[0].num_planes; ++j)
        {
            plane_layouts[j].offset = descriptor.layers[0].offset[j];
            plane_layouts[j].rowPitch = descriptor.layers[0].pitch[j];
        }
        drm_format_desc.drmFormatModifier = descriptor.objects[0].drm_format_modifier;
        drm_format_desc.drmFormatModifierPlaneCount = descriptor.layers[0].num_planes;
        drm_format_desc.pPlaneLayouts = plane_layouts;

        /* Images aren't wrapped; we can just call the host version. */
        if ((vr = device->p_vkCreateImage(device->host.device, &image_desc, NULL, &image)) != VK_SUCCESS)
        {
            ERR("Failed to create image, vr %#x.\n", vr);
            continue;
        }

        if (device->extensions.has_VK_KHR_dedicated_allocation)
        {
            memory_reqs.pNext = &dedicated_reqs;
            reqs_desc.image = image;
            device->p_vkGetImageMemoryRequirements2(device->host.device, &reqs_desc, &memory_reqs);

            if (dedicated_reqs.prefersDedicatedAllocation)
            {
                fd_desc.pNext = &dedicated_desc;
                dedicated_desc.image = image;
            }
        }

        fd_desc.fd = descriptor.objects[0].fd;

        if ((vr = device->p_vkAllocateMemory(device->host.device,
                &memory_desc, NULL, &decoder->vk_memory[i])) != VK_SUCCESS)
        {
            ERR("Failed to allocate memory, vr %d.\n", vr);
            device->p_vkDestroyImage(device->host.device, image, NULL);
            continue;
        }

        if ((vr = device->p_vkBindImageMemory(device->host.device, image, decoder->vk_memory[i], 0)) != VK_SUCCESS)
        {
            ERR("Failed to bind memory, vr %d.\n", vr);
            device->p_vkFreeMemory(device->host.device, decoder->vk_memory[i], NULL);
            device->p_vkDestroyImage(device->host.device, image, NULL);
            continue;
        }

        params->surfaces[i].image = image;
    }

    decoder->long_slice_info = params->desc.long_slice_info;

    TRACE("Created VA decoder %p.\n", decoder);

    params->decoder = (uintptr_t)decoder;
    return S_OK;

fail:
    free(decoder);
    return E_FAIL;
}

static NTSTATUS va_decoder_destroy_vk(void *args)
{
    struct va_decoder_destroy_vk_params *params = args;
    struct vulkan_device *device = vulkan_device_from_handle((VkDevice)params->device);
    struct va_decoder *decoder = (struct va_decoder *)(uintptr_t)params->decoder;
    struct va_context *ctx = &decoder->ctx;

    for (unsigned int i = 0; i < ARRAY_SIZE(decoder->vk_memory); ++i)
        device->p_vkFreeMemory(device->host.device, decoder->vk_memory[i], NULL);
    ctx->vaDestroyContext(ctx->display, decoder->context);
    ctx->vaDestroySurfaces(ctx->display, decoder->surfaces, VA_DECODER_SURFACE_COUNT);
    ctx->vaDestroyConfig(ctx->display, decoder->config);
    close_va_display(ctx);
    free(decoder);
    return S_OK;
}

static bool find_reference_slot(struct va_decoder *decoder, uint8_t dxva_index, unsigned int *idx)
{
    for (unsigned int i = 0; i < VA_DECODER_SURFACE_COUNT; ++i)
    {
        if (decoder->references[i].valid && decoder->references[i].dxva_index == dxva_index)
        {
            *idx = i;
            return true;
        }
    }

    ERR("Reference index %u was never written.\n", dxva_index);
    return false;
}

static NTSTATUS va_decoder_decode(void *args)
{
    struct va_decoder_decode_params *params = args;
    const DXVA_PicParams_H264 *dxva_params = (const void *)(uintptr_t)params->parameters;
    const DXVA_Qmatrix_H264 *dxva_matrix = (const void *)(uintptr_t)params->matrix;
    struct va_decoder *decoder = (struct va_decoder *)(uintptr_t)params->decoder;
    VAPictureParameterBufferH264 va_params = {0};
    struct va_context *ctx = &decoder->ctx;
    VAIQMatrixBufferH264 va_matrix;
    unsigned int ref_count = 0;
    unsigned int slice_count;
    VABufferID buffers[3];
    VAStatus status;

    if (decoder->long_slice_info)
        slice_count = params->slice_control_size / sizeof(DXVA_Slice_H264_Long);
    else
        slice_count = params->slice_control_size / sizeof(DXVA_Slice_H264_Short);

    if ((status = ctx->vaBeginPicture(ctx->display, decoder->context,
            decoder->surfaces[params->output_idx])) != VA_STATUS_SUCCESS)
        ERR("Failed to begin picture, error %#x.\n", status);

    for (unsigned int i = 0; i < slice_count; ++i)
    {
        VASliceParameterBufferH264 slice_params = {0};
        VABufferID slice_buffer;

        if (decoder->long_slice_info)
        {
            const DXVA_Slice_H264_Long *slices = (const void *)(uintptr_t)params->slice_control;

            /* VA doesn't want the start codes. */
            slice_params.slice_data_size = slices[i].SliceBytesInBuffer - 3;
            slice_params.slice_data_offset = slices[i].BSNALunitDataLocation + 3;
            slice_params.slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

            slice_params.num_ref_idx_l0_active_minus1 = slices[i].num_ref_idx_l0_active_minus1;
            slice_params.num_ref_idx_l1_active_minus1 = slices[i].num_ref_idx_l1_active_minus1;
            slice_params.slice_type = slices[i].slice_type;
        }
        else
        {
            const DXVA_Slice_H264_Short *slices = (const void *)(uintptr_t)params->slice_control;

            slice_params.slice_data_size = slices[i].SliceBytesInBuffer - 3;
            slice_params.slice_data_offset = slices[i].BSNALunitDataLocation + 3;
            slice_params.slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

            /* FIXME: We can't fill any of the other parameters.
             * Mesa doesn't care about most of them, but it does care about
             * these two. However, it also treats them as per-picture, and we
             * have the "default" values presumably from the PPS, so provide
             * them here. */
            slice_params.num_ref_idx_l0_active_minus1 = dxva_params->num_ref_idx_l0_active_minus1;
            slice_params.num_ref_idx_l1_active_minus1 = dxva_params->num_ref_idx_l1_active_minus1;
        }

        if ((status = ctx->vaCreateBuffer(ctx->display, decoder->context, VASliceParameterBufferType,
                sizeof(slice_params), 1, &slice_params, &slice_buffer)) != VA_STATUS_SUCCESS)
            ERR("Failed to create slice parameters, error %#x.\n", status);

        if ((status = ctx->vaRenderPicture(ctx->display, decoder->context,
                &slice_buffer, 1)) != VA_STATUS_SUCCESS)
            ERR("Failed to render slice parameters, error %#x.\n", status);

        ctx->vaDestroyBuffer(ctx->display, slice_buffer);
    }

    TRACE("Decoding frame %02x/%02x/%u, RefPicFlag %#x, reference frames",
            dxva_params->CurrPic.bPicEntry, dxva_params->frame_num, params->output_idx, dxva_params->RefPicFlag);

    for (unsigned int i = 0; i < ARRAY_SIZE(dxva_params->RefFrameList); ++i)
    {
        unsigned int field_flags = ((dxva_params->UsedForReferenceFlags >> (2 * i)) & 3u);
        VAPictureH264 *va_ref = &va_params.ReferenceFrames[ref_count];
        unsigned int ref_idx;

        if (dxva_params->RefFrameList[i].bPicEntry == 0xff)
            continue;

        /* NVidia's DXVA implementation apparently expects each frame to appear
         * in its own references list. VA does not expect or need this. */
        if (dxva_params->RefFrameList[i].Index7Bits == dxva_params->CurrPic.Index7Bits)
            continue;

        if (!find_reference_slot(decoder, dxva_params->RefFrameList[i].Index7Bits, &ref_idx))
            return E_FAIL;

        TRACE(" %02x/%02x/%u", dxva_params->RefFrameList[i].bPicEntry, dxva_params->FrameNumList[i], ref_idx);

        va_ref->picture_id = decoder->surfaces[ref_idx];
        va_ref->frame_idx = dxva_params->FrameNumList[i];
        if (dxva_params->RefFrameList[i].AssociatedFlag)
            va_ref->flags = VA_PICTURE_H264_LONG_TERM_REFERENCE;
        else
            va_ref->flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;

        if (field_flags == 1)
            va_ref->flags |= VA_PICTURE_H264_TOP_FIELD;
        else if (field_flags == 2)
            va_ref->flags |= VA_PICTURE_H264_BOTTOM_FIELD;

        va_ref->TopFieldOrderCnt = dxva_params->FieldOrderCntList[i][0];
        va_ref->BottomFieldOrderCnt = dxva_params->FieldOrderCntList[i][1];

        ++ref_count;
    }

    for (unsigned int i = ref_count; i < ARRAY_SIZE(va_params.ReferenceFrames); ++i)
    {
        va_params.ReferenceFrames[i].flags = VA_PICTURE_H264_INVALID;
        va_params.ReferenceFrames[i].picture_id = VA_INVALID_SURFACE;
    }

    TRACE(".\n");

    va_params.CurrPic.picture_id = decoder->surfaces[params->output_idx];
    va_params.CurrPic.frame_idx = dxva_params->frame_num;
    /* FIXME: What on earth do we put here? For some reason DXVA supplies these
     * flags for reference frames, but not for the current frame.
     * Mesa doesn't care about most of these, but it does care whether the
     * current frame is a bottom field. */
    va_params.CurrPic.flags = 0;
    va_params.CurrPic.TopFieldOrderCnt = dxva_params->CurrFieldOrderCnt[0];
    va_params.CurrPic.BottomFieldOrderCnt = dxva_params->CurrFieldOrderCnt[1];

    va_params.picture_width_in_mbs_minus1 = dxva_params->wFrameWidthInMbsMinus1;
    va_params.picture_height_in_mbs_minus1 = dxva_params->wFrameHeightInMbsMinus1;
    va_params.bit_depth_luma_minus8 = dxva_params->bit_depth_luma_minus8;
    va_params.bit_depth_chroma_minus8 = dxva_params->bit_depth_chroma_minus8;
    va_params.num_ref_frames = dxva_params->num_ref_frames;
    va_params.seq_fields.bits.chroma_format_idc = dxva_params->chroma_format_idc;
    va_params.seq_fields.bits.residual_colour_transform_flag = dxva_params->residual_colour_transform_flag;
    /* We don't have this value, so we have to say it's allowed. */
    va_params.seq_fields.bits.gaps_in_frame_num_value_allowed_flag = 1;
    va_params.seq_fields.bits.frame_mbs_only_flag = dxva_params->frame_mbs_only_flag;
    /* We don't have mb_adaptive_frame_field_flag, but we do have MbaffFrameFlag
     * which is (mb_adaptive_frame_field_flag && !field_pic_flag).
     * If field_pic_flag is 1, we don't know, so we set it to 1, which is the
     * less constrained option. */
    if (!dxva_params->field_pic_flag)
        va_params.seq_fields.bits.mb_adaptive_frame_field_flag = dxva_params->MbaffFrameFlag;
    else
        va_params.seq_fields.bits.mb_adaptive_frame_field_flag = 1;
    va_params.seq_fields.bits.direct_8x8_inference_flag = dxva_params->direct_8x8_inference_flag;
    va_params.seq_fields.bits.MinLumaBiPredSize8x8 = dxva_params->MinLumaBipredSize8x8Flag;
    va_params.seq_fields.bits.log2_max_frame_num_minus4 = dxva_params->log2_max_frame_num_minus4;
    va_params.seq_fields.bits.pic_order_cnt_type = dxva_params->pic_order_cnt_type;
    va_params.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = dxva_params->log2_max_pic_order_cnt_lsb_minus4;
    va_params.seq_fields.bits.delta_pic_order_always_zero_flag = dxva_params->delta_pic_order_always_zero_flag;
    /* The slice group fields are "va_deprecated" and GStreamer doesn't fill them. */
    va_params.pic_init_qp_minus26 = dxva_params->pic_init_qp_minus26;
    va_params.pic_init_qs_minus26 = dxva_params->pic_init_qs_minus26;
    va_params.chroma_qp_index_offset = dxva_params->chroma_qp_index_offset;
    va_params.second_chroma_qp_index_offset = dxva_params->second_chroma_qp_index_offset;
    va_params.pic_fields.bits.entropy_coding_mode_flag = dxva_params->entropy_coding_mode_flag;
    va_params.pic_fields.bits.weighted_pred_flag = dxva_params->weighted_pred_flag;
    va_params.pic_fields.bits.weighted_bipred_idc = dxva_params->weighted_bipred_idc;
    va_params.pic_fields.bits.transform_8x8_mode_flag = dxva_params->transform_8x8_mode_flag;
    va_params.pic_fields.bits.field_pic_flag = dxva_params->field_pic_flag;
    va_params.pic_fields.bits.constrained_intra_pred_flag = dxva_params->constrained_intra_pred_flag;
    va_params.pic_fields.bits.pic_order_present_flag = dxva_params->pic_order_present_flag;
    va_params.pic_fields.bits.deblocking_filter_control_present_flag = dxva_params->deblocking_filter_control_present_flag;
    va_params.pic_fields.bits.redundant_pic_cnt_present_flag = dxva_params->redundant_pic_cnt_present_flag;
    /* This seems to be equivalent, although the GStreamer code is not the easiest to follow. */
    va_params.pic_fields.bits.reference_pic_flag = dxva_params->RefPicFlag;
    /* This too. */
    va_params.frame_num = dxva_params->frame_num;

    decoder->references[params->output_idx].valid = true;
    decoder->references[params->output_idx].dxva_index = dxva_params->CurrPic.Index7Bits;

    /* The DXVA and VA matrices are byte-compatible. */
    memcpy(&va_matrix, dxva_matrix, sizeof(*dxva_matrix));
    memset(va_matrix.va_reserved, 0, sizeof(va_matrix.va_reserved));

    /* The parameters need to be submitted first, or Mesa fails. */

    if ((status = ctx->vaCreateBuffer(ctx->display, decoder->context, VAPictureParameterBufferType,
            sizeof(va_params), 1, &va_params, &buffers[0])) != VA_STATUS_SUCCESS)
        ERR("Failed to create parameters buffer, error %#x.\n", status);

    if ((status = ctx->vaCreateBuffer(ctx->display, decoder->context, VAIQMatrixBufferType,
            sizeof(va_matrix), 1, &va_matrix, &buffers[1])) != VA_STATUS_SUCCESS)
        ERR("Failed to create parameters buffer, error %#x.\n", status);

    if ((status = ctx->vaCreateBuffer(ctx->display, decoder->context, VASliceDataBufferType,
            params->bitstream_size, 1, (void *)(uintptr_t)params->bitstream, &buffers[2])) != VA_STATUS_SUCCESS)
        ERR("Failed to create bitstream buffer, error %#x.\n", status);

    if ((status = ctx->vaRenderPicture(ctx->display, decoder->context,
            buffers, ARRAY_SIZE(buffers))) != VA_STATUS_SUCCESS)
        ERR("Failed to render buffers, error %#x.\n", status);

    ctx->vaDestroyBuffer(ctx->display, buffers[0]);
    ctx->vaDestroyBuffer(ctx->display, buffers[1]);
    ctx->vaDestroyBuffer(ctx->display, buffers[2]);

    if ((status = ctx->vaEndPicture(ctx->display, decoder->context)) != VA_STATUS_SUCCESS)
        ERR("Failed to end picture, error %#x.\n", status);

    ctx->vaSyncSurface(ctx->display, decoder->surfaces[params->output_idx]);

    return S_OK;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    [unix_va_get_profiles_vk] = va_get_profiles_vk,
    [unix_va_decoder_create_vk] = va_decoder_create_vk,
    [unix_va_decoder_destroy_vk] = va_decoder_destroy_vk,
    [unix_va_decoder_decode] = va_decoder_decode,
};

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    [unix_va_get_profiles_vk] = va_get_profiles_vk,
    [unix_va_decoder_create_vk] = va_decoder_create_vk,
    [unix_va_decoder_destroy_vk] = va_decoder_destroy_vk,
    [unix_va_decoder_decode] = va_decoder_decode,
};

#endif
