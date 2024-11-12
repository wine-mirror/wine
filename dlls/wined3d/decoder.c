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

static void wined3d_null_decoder_get_profiles(struct wined3d_adapter *adapter, unsigned int *count, GUID *profiles)
{
    *count = 0;
}

const struct wined3d_decoder_ops wined3d_null_decoder_ops =
{
    .get_profiles = wined3d_null_decoder_get_profiles,
};

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

const struct wined3d_decoder_ops wined3d_decoder_vk_ops =
{
    .get_profiles = wined3d_decoder_vk_get_profiles,
};
