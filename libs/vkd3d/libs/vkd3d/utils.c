/*
 * Copyright 2016 Józef Kucia for CodeWeavers
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

#include "vkd3d_private.h"

#include <errno.h>

#define COLOR         (VK_IMAGE_ASPECT_COLOR_BIT)
#define DEPTH         (VK_IMAGE_ASPECT_DEPTH_BIT)
#define STENCIL       (VK_IMAGE_ASPECT_STENCIL_BIT)
#define DEPTH_STENCIL (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)
#define TYPELESS      VKD3D_FORMAT_TYPE_TYPELESS
#define SINT          VKD3D_FORMAT_TYPE_SINT
#define UINT          VKD3D_FORMAT_TYPE_UINT
static const struct vkd3d_format vkd3d_formats[] =
{
    {DXGI_FORMAT_UNKNOWN,               VK_FORMAT_UNDEFINED,                1,  1, 1,  1, 0,     1},
    {DXGI_FORMAT_R32G32B32A32_TYPELESS, VK_FORMAT_R32G32B32A32_SFLOAT,      16, 1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R32G32B32A32_FLOAT,    VK_FORMAT_R32G32B32A32_SFLOAT,      16, 1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R32G32B32A32_UINT,     VK_FORMAT_R32G32B32A32_UINT,        16, 1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R32G32B32A32_SINT,     VK_FORMAT_R32G32B32A32_SINT,        16, 1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R32G32B32_TYPELESS,    VK_FORMAT_R32G32B32_SFLOAT,         12, 1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R32G32B32_FLOAT,       VK_FORMAT_R32G32B32_SFLOAT,         12, 1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R32G32B32_UINT,        VK_FORMAT_R32G32B32_UINT,           12, 1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R32G32B32_SINT,        VK_FORMAT_R32G32B32_SINT,           12, 1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, VK_FORMAT_R16G16B16A16_SFLOAT,      8,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R16G16B16A16_FLOAT,    VK_FORMAT_R16G16B16A16_SFLOAT,      8,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16B16A16_UNORM,    VK_FORMAT_R16G16B16A16_UNORM,       8,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16B16A16_UINT,     VK_FORMAT_R16G16B16A16_UINT,        8,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R16G16B16A16_SNORM,    VK_FORMAT_R16G16B16A16_SNORM,       8,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16B16A16_SINT,     VK_FORMAT_R16G16B16A16_SINT,        8,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R32G32_TYPELESS,       VK_FORMAT_R32G32_SFLOAT,            8,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R32G32_FLOAT,          VK_FORMAT_R32G32_SFLOAT,            8,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R32G32_UINT,           VK_FORMAT_R32G32_UINT,              8,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R32G32_SINT,           VK_FORMAT_R32G32_SINT,              8,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R10G10B10A2_TYPELESS,  VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R10G10B10A2_UNORM,     VK_FORMAT_A2B10G10R10_UNORM_PACK32, 4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R10G10B10A2_UINT,      VK_FORMAT_A2B10G10R10_UINT_PACK32,  4,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R11G11B10_FLOAT,       VK_FORMAT_B10G11R11_UFLOAT_PACK32,  4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8_TYPELESS,         VK_FORMAT_R8G8_UNORM,               2,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R8G8_UNORM,            VK_FORMAT_R8G8_UNORM,               2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8_UINT,             VK_FORMAT_R8G8_UINT,                2,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R8G8_SNORM,            VK_FORMAT_R8G8_SNORM,               2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8_SINT,             VK_FORMAT_R8G8_SINT,                2,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R8G8B8A8_TYPELESS,     VK_FORMAT_R8G8B8A8_UNORM,           4,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R8G8B8A8_UNORM,        VK_FORMAT_R8G8B8A8_UNORM,           4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,   VK_FORMAT_R8G8B8A8_SRGB,            4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8B8A8_UINT,         VK_FORMAT_R8G8B8A8_UINT,            4,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R8G8B8A8_SNORM,        VK_FORMAT_R8G8B8A8_SNORM,           4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8G8B8A8_SINT,         VK_FORMAT_R8G8B8A8_SINT,            4,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R16G16_TYPELESS,       VK_FORMAT_R16G16_SFLOAT,            4,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R16G16_FLOAT,          VK_FORMAT_R16G16_SFLOAT,            4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16_UNORM,          VK_FORMAT_R16G16_UNORM,             4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16_UINT,           VK_FORMAT_R16G16_UINT,              4,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R16G16_SNORM,          VK_FORMAT_R16G16_SNORM,             4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16G16_SINT,           VK_FORMAT_R16G16_SINT,              4,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R32_TYPELESS,          VK_FORMAT_R32_UINT,                 4,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_D32_FLOAT,             VK_FORMAT_D32_SFLOAT,               4,  1, 1,  1, DEPTH, 1},
    {DXGI_FORMAT_R32_FLOAT,             VK_FORMAT_R32_SFLOAT,               4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R32_UINT,              VK_FORMAT_R32_UINT,                 4,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R32_SINT,              VK_FORMAT_R32_SINT,                 4,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R16_TYPELESS,          VK_FORMAT_R16_UINT,                 2,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R16_FLOAT,             VK_FORMAT_R16_SFLOAT,               2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_D16_UNORM,             VK_FORMAT_D16_UNORM,                2,  1, 1,  1, DEPTH, 1},
    {DXGI_FORMAT_R16_UNORM,             VK_FORMAT_R16_UNORM,                2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16_UINT,              VK_FORMAT_R16_UINT,                 2,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R16_SNORM,             VK_FORMAT_R16_SNORM,                2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R16_SINT,              VK_FORMAT_R16_SINT,                 2,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_R8_TYPELESS,           VK_FORMAT_R8_UNORM,                 1,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_R8_UNORM,              VK_FORMAT_R8_UNORM,                 1,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8_UINT,               VK_FORMAT_R8_UINT,                  1,  1, 1,  1, COLOR, 1, UINT},
    {DXGI_FORMAT_R8_SNORM,              VK_FORMAT_R8_SNORM,                 1,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_R8_SINT,               VK_FORMAT_R8_SINT,                  1,  1, 1,  1, COLOR, 1, SINT},
    {DXGI_FORMAT_A8_UNORM,              VK_FORMAT_R8_UNORM,                 1,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B5G6R5_UNORM,          VK_FORMAT_R5G6B5_UNORM_PACK16,      2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B5G5R5A1_UNORM,        VK_FORMAT_A1R5G5B5_UNORM_PACK16,    2,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B8G8R8A8_UNORM,        VK_FORMAT_B8G8R8A8_UNORM,           4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B8G8R8X8_UNORM,        VK_FORMAT_B8G8R8A8_UNORM,           4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B8G8R8A8_TYPELESS,     VK_FORMAT_B8G8R8A8_UNORM,           4,  1, 1,  1, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,   VK_FORMAT_B8G8R8A8_SRGB,            4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B8G8R8X8_TYPELESS,     VK_FORMAT_B8G8R8A8_UNORM,           4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,   VK_FORMAT_B8G8R8A8_SRGB,            4,  1, 1,  1, COLOR, 1},
    {DXGI_FORMAT_BC1_TYPELESS,          VK_FORMAT_BC1_RGBA_UNORM_BLOCK,     1,  4, 4,  8, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC1_UNORM,             VK_FORMAT_BC1_RGBA_UNORM_BLOCK,     1,  4, 4,  8, COLOR, 1},
    {DXGI_FORMAT_BC1_UNORM_SRGB,        VK_FORMAT_BC1_RGBA_SRGB_BLOCK,      1,  4, 4,  8, COLOR, 1},
    {DXGI_FORMAT_BC2_TYPELESS,          VK_FORMAT_BC2_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC2_UNORM,             VK_FORMAT_BC2_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC2_UNORM_SRGB,        VK_FORMAT_BC2_SRGB_BLOCK,           1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC3_TYPELESS,          VK_FORMAT_BC3_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC3_UNORM,             VK_FORMAT_BC3_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC3_UNORM_SRGB,        VK_FORMAT_BC3_SRGB_BLOCK,           1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC4_TYPELESS,          VK_FORMAT_BC4_UNORM_BLOCK,          1,  4, 4,  8, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC4_UNORM,             VK_FORMAT_BC4_UNORM_BLOCK,          1,  4, 4,  8, COLOR, 1},
    {DXGI_FORMAT_BC4_SNORM,             VK_FORMAT_BC4_SNORM_BLOCK,          1,  4, 4,  8, COLOR, 1},
    {DXGI_FORMAT_BC5_TYPELESS,          VK_FORMAT_BC5_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC5_UNORM,             VK_FORMAT_BC5_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC5_SNORM,             VK_FORMAT_BC5_SNORM_BLOCK,          1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC6H_TYPELESS,         VK_FORMAT_BC6H_UFLOAT_BLOCK,        1,  4, 4, 16, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC6H_UF16,             VK_FORMAT_BC6H_UFLOAT_BLOCK,        1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC6H_SF16,             VK_FORMAT_BC6H_SFLOAT_BLOCK,        1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC7_TYPELESS,          VK_FORMAT_BC7_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1, TYPELESS},
    {DXGI_FORMAT_BC7_UNORM,             VK_FORMAT_BC7_UNORM_BLOCK,          1,  4, 4, 16, COLOR, 1},
    {DXGI_FORMAT_BC7_UNORM_SRGB,        VK_FORMAT_BC7_SRGB_BLOCK,           1,  4, 4, 16, COLOR, 1},
};

static const struct vkd3d_format format_b4g4r4a4 =
    {DXGI_FORMAT_B4G4R4A4_UNORM,        VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT, 2,  1, 1,  1, COLOR, 1};

/* Each depth/stencil format is only compatible with itself in Vulkan. */
static const struct vkd3d_format vkd3d_depth_stencil_formats[] =
{
    {DXGI_FORMAT_R32G8X24_TYPELESS,        VK_FORMAT_D32_SFLOAT_S8_UINT, 8,  1, 1, 1, DEPTH_STENCIL, 2, TYPELESS},
    {DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     VK_FORMAT_D32_SFLOAT_S8_UINT, 8,  1, 1, 1, DEPTH_STENCIL, 2},
    {DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, VK_FORMAT_D32_SFLOAT_S8_UINT, 8,  1, 1, 1, DEPTH,         2},
    {DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  VK_FORMAT_D32_SFLOAT_S8_UINT, 8,  1, 1, 1, STENCIL,       2},
    {DXGI_FORMAT_R32_TYPELESS,             VK_FORMAT_D32_SFLOAT,         4,  1, 1, 1, DEPTH,         1, TYPELESS},
    {DXGI_FORMAT_R32_FLOAT,                VK_FORMAT_D32_SFLOAT,         4,  1, 1, 1, DEPTH,         1},
    {DXGI_FORMAT_R24G8_TYPELESS,           VK_FORMAT_D24_UNORM_S8_UINT,  4,  1, 1, 1, DEPTH_STENCIL, 2, TYPELESS},
    {DXGI_FORMAT_D24_UNORM_S8_UINT,        VK_FORMAT_D24_UNORM_S8_UINT,  4,  1, 1, 1, DEPTH_STENCIL, 2},
    {DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    VK_FORMAT_D24_UNORM_S8_UINT,  4,  1, 1, 1, DEPTH,         2},
    {DXGI_FORMAT_X24_TYPELESS_G8_UINT,     VK_FORMAT_D24_UNORM_S8_UINT,  4,  1, 1, 1, STENCIL,       2},
    {DXGI_FORMAT_R16_TYPELESS,             VK_FORMAT_D16_UNORM,          2,  1, 1, 1, DEPTH,         1, TYPELESS},
    {DXGI_FORMAT_R16_UNORM,                VK_FORMAT_D16_UNORM,          2,  1, 1, 1, DEPTH,         1},
};
#undef COLOR
#undef DEPTH
#undef STENCIL
#undef DEPTH_STENCIL
#undef TYPELESS
#undef SINT
#undef UINT

static const struct vkd3d_format_compatibility_info
{
    DXGI_FORMAT format;
    DXGI_FORMAT typeless_format;
}
vkd3d_format_compatibility_info[] =
{
    /* DXGI_FORMAT_R32G32B32A32_TYPELESS */
    {DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_TYPELESS},
    {DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_TYPELESS},
    {DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_TYPELESS},
    /* DXGI_FORMAT_R32G32B32_TYPELESS */
    {DXGI_FORMAT_R32G32B32_UINT,           DXGI_FORMAT_R32G32B32_TYPELESS},
    {DXGI_FORMAT_R32G32B32_SINT,           DXGI_FORMAT_R32G32B32_TYPELESS},
    {DXGI_FORMAT_R32G32B32_FLOAT,          DXGI_FORMAT_R32G32B32_TYPELESS},
    /* DXGI_FORMAT_R16G16B16A16_TYPELESS */
    {DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_TYPELESS},
    {DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_TYPELESS},
    {DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_TYPELESS},
    {DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_TYPELESS},
    {DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_TYPELESS},
    /* DXGI_FORMAT_R32G32_TYPELESS */
    {DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_TYPELESS},
    {DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_TYPELESS},
    {DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_TYPELESS},
    /* DXGI_FORMAT_R32G8X24_TYPELESS */
    {DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_R32G8X24_TYPELESS},
    {DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  DXGI_FORMAT_R32G8X24_TYPELESS},
    {DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     DXGI_FORMAT_R32G8X24_TYPELESS},
    /* DXGI_FORMAT_R10G10B10A2_TYPELESS */
    {DXGI_FORMAT_R10G10B10A2_UINT,         DXGI_FORMAT_R10G10B10A2_TYPELESS},
    {DXGI_FORMAT_R10G10B10A2_UNORM,        DXGI_FORMAT_R10G10B10A2_TYPELESS},
    /* DXGI_FORMAT_R8G8B8A8_TYPELESS */
    {DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_TYPELESS},
    {DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_TYPELESS},
    {DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_TYPELESS},
    {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      DXGI_FORMAT_R8G8B8A8_TYPELESS},
    {DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_TYPELESS},
    /* DXGI_FORMAT_R16G16_TYPELESS */
    {DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_TYPELESS},
    {DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_TYPELESS},
    {DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_TYPELESS},
    {DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_TYPELESS},
    {DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_TYPELESS},
    /* DXGI_FORMAT_R32_TYPELESS */
    {DXGI_FORMAT_D32_FLOAT,                DXGI_FORMAT_R32_TYPELESS},
    {DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_TYPELESS},
    {DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_TYPELESS},
    {DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_TYPELESS},
    /* DXGI_FORMAT_R24G8_TYPELESS */
    {DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_R24G8_TYPELESS},
    {DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_R24G8_TYPELESS},
    {DXGI_FORMAT_D24_UNORM_S8_UINT,        DXGI_FORMAT_R24G8_TYPELESS},
    /* DXGI_FORMAT_R8G8_TYPELESS */
    {DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_TYPELESS},
    {DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_TYPELESS},
    {DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_TYPELESS},
    {DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_TYPELESS},
    /* DXGI_FORMAT_R16_TYPELESS */
    {DXGI_FORMAT_D16_UNORM,                DXGI_FORMAT_R16_TYPELESS},
    {DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_TYPELESS},
    {DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_TYPELESS},
    {DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_TYPELESS},
    {DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_TYPELESS},
    {DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_TYPELESS},
    /* DXGI_FORMAT_R8_TYPELESS */
    {DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_TYPELESS},
    {DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_TYPELESS},
    {DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_TYPELESS},
    {DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_TYPELESS},
    /* DXGI_FORMAT_BC1_TYPELESS */
    {DXGI_FORMAT_BC1_UNORM_SRGB,           DXGI_FORMAT_BC1_TYPELESS},
    {DXGI_FORMAT_BC1_UNORM,                DXGI_FORMAT_BC1_TYPELESS},
    /* DXGI_FORMAT_BC2_TYPELESS */
    {DXGI_FORMAT_BC2_UNORM_SRGB,           DXGI_FORMAT_BC2_TYPELESS},
    {DXGI_FORMAT_BC2_UNORM,                DXGI_FORMAT_BC2_TYPELESS},
    /* DXGI_FORMAT_BC3_TYPELESS */
    {DXGI_FORMAT_BC3_UNORM_SRGB,           DXGI_FORMAT_BC3_TYPELESS},
    {DXGI_FORMAT_BC3_UNORM,                DXGI_FORMAT_BC3_TYPELESS},
    /* DXGI_FORMAT_BC4_TYPELESS */
    {DXGI_FORMAT_BC4_UNORM,                DXGI_FORMAT_BC4_TYPELESS},
    {DXGI_FORMAT_BC4_SNORM,                DXGI_FORMAT_BC4_TYPELESS},
    /* DXGI_FORMAT_BC5_TYPELESS */
    {DXGI_FORMAT_BC5_UNORM,                DXGI_FORMAT_BC5_TYPELESS},
    {DXGI_FORMAT_BC5_SNORM,                DXGI_FORMAT_BC5_TYPELESS},
    /* DXGI_FORMAT_BC6H_TYPELESS */
    {DXGI_FORMAT_BC6H_UF16,                DXGI_FORMAT_BC6H_TYPELESS},
    {DXGI_FORMAT_BC6H_SF16,                DXGI_FORMAT_BC6H_TYPELESS},
    /* DXGI_FORMAT_BC7_TYPELESS */
    {DXGI_FORMAT_BC7_UNORM_SRGB,           DXGI_FORMAT_BC7_TYPELESS},
    {DXGI_FORMAT_BC7_UNORM,                DXGI_FORMAT_BC7_TYPELESS},
    /* DXGI_FORMAT_B8G8R8A8_TYPELESS */
    {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8A8_TYPELESS},
    {DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_TYPELESS},
    /* DXGI_FORMAT_B8G8R8X8_TYPELESS */
    {DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8X8_TYPELESS},
    {DXGI_FORMAT_B8G8R8X8_UNORM,           DXGI_FORMAT_B8G8R8X8_TYPELESS},
};

static bool dxgi_format_is_depth_stencil(DXGI_FORMAT dxgi_format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_formats); ++i)
    {
        const struct vkd3d_format *current = &vkd3d_formats[i];

        if (current->dxgi_format == dxgi_format)
            return current->vk_aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
    }

    for (i = 0; i < ARRAY_SIZE(vkd3d_depth_stencil_formats); ++i)
    {
        if (vkd3d_depth_stencil_formats[i].dxgi_format == dxgi_format)
            return true;
    }

    return false;
}

/* FIXME: This table should be generated at compile-time. */
static HRESULT vkd3d_init_format_compatibility_lists(struct d3d12_device *device)
{
    struct vkd3d_format_compatibility_list *lists, *current_list;
    const struct vkd3d_format_compatibility_info *current;
    DXGI_FORMAT dxgi_format;
    VkFormat vk_format;
    unsigned int count;
    unsigned int i, j;

    device->format_compatibility_list_count = 0;
    device->format_compatibility_lists = NULL;

    if (!device->vk_info.KHR_image_format_list)
        return S_OK;

    count = 1;
    dxgi_format = vkd3d_format_compatibility_info[0].typeless_format;
    for (i = 0; i < ARRAY_SIZE(vkd3d_format_compatibility_info); ++i)
    {
        DXGI_FORMAT typeless_format = vkd3d_format_compatibility_info[i].typeless_format;

        if (dxgi_format != typeless_format)
        {
            ++count;
            dxgi_format = typeless_format;
        }
    }

    if (!(lists = vkd3d_calloc(count, sizeof(*lists))))
        return E_OUTOFMEMORY;

    count = 0;
    current_list = lists;
    current_list->typeless_format = vkd3d_format_compatibility_info[0].typeless_format;
    for (i = 0; i < ARRAY_SIZE(vkd3d_format_compatibility_info); ++i)
    {
        current = &vkd3d_format_compatibility_info[i];

        if (current_list->typeless_format != current->typeless_format)
        {
            /* Avoid empty format lists. */
            if (current_list->format_count)
            {
                ++current_list;
                ++count;
            }

            current_list->typeless_format = current->typeless_format;
        }

        /* In Vulkan, each depth-stencil format is only compatible with itself. */
        if (dxgi_format_is_depth_stencil(current->format))
            continue;

        if (!(vk_format = vkd3d_get_vk_format(current->format)))
            continue;

        for (j = 0; j < current_list->format_count; ++j)
        {
            if (current_list->vk_formats[j] == vk_format)
                break;
        }

        if (j >= current_list->format_count)
        {
            VKD3D_ASSERT(current_list->format_count < VKD3D_MAX_COMPATIBLE_FORMAT_COUNT);
            current_list->vk_formats[current_list->format_count++] = vk_format;
        }
    }
    if (current_list->format_count)
        ++count;

    device->format_compatibility_list_count = count;
    device->format_compatibility_lists = lists;
    return S_OK;
}

static void vkd3d_cleanup_format_compatibility_lists(struct d3d12_device *device)
{
    vkd3d_free((void *)device->format_compatibility_lists);

    device->format_compatibility_lists = NULL;
    device->format_compatibility_list_count = 0;
}

static HRESULT vkd3d_init_depth_stencil_formats(struct d3d12_device *device)
{
    const unsigned int count = ARRAY_SIZE(vkd3d_depth_stencil_formats);
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkFormatProperties properties;
    struct vkd3d_format *formats;
    unsigned int i;

    VK_CALL(vkGetPhysicalDeviceFormatProperties(device->vk_physical_device,
            VK_FORMAT_D24_UNORM_S8_UINT, &properties));

    if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        device->depth_stencil_formats = vkd3d_depth_stencil_formats;
    }
    else
    {
        /* AMD doesn't support VK_FORMAT_D24_UNORM_S8_UINT. */
        WARN("Mapping VK_FORMAT_D24_UNORM_S8_UINT to VK_FORMAT_D32_SFLOAT_S8_UINT.\n");

        if (!(formats = vkd3d_calloc(count, sizeof(*formats))))
            return E_OUTOFMEMORY;

        memcpy(formats, vkd3d_depth_stencil_formats, sizeof(vkd3d_depth_stencil_formats));
        for (i = 0; i < count; ++i)
        {
            if (formats[i].vk_format == VK_FORMAT_D24_UNORM_S8_UINT)
            {
                formats[i].vk_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                formats[i].is_emulated = true;
            }
        }

        device->depth_stencil_formats = formats;
    }

    return S_OK;
}

static void vkd3d_cleanup_depth_stencil_formats(struct d3d12_device *device)
{
    if (vkd3d_depth_stencil_formats != device->depth_stencil_formats)
        vkd3d_free((void *)device->depth_stencil_formats);

    device->depth_stencil_formats = NULL;
}

HRESULT vkd3d_init_format_info(struct d3d12_device *device)
{
    HRESULT hr;

    if (FAILED(hr = vkd3d_init_depth_stencil_formats(device)))
        return hr;

    if (FAILED(hr = vkd3d_init_format_compatibility_lists(device)))
        vkd3d_cleanup_depth_stencil_formats(device);

    return hr;
}

void vkd3d_cleanup_format_info(struct d3d12_device *device)
{
    vkd3d_cleanup_depth_stencil_formats(device);
    vkd3d_cleanup_format_compatibility_lists(device);
}

/* We use overrides for depth/stencil formats. This is required in order to
 * properly support typeless formats because depth/stencil formats are only
 * compatible with themselves in Vulkan.
 */
static const struct vkd3d_format *vkd3d_get_depth_stencil_format(const struct d3d12_device *device,
        DXGI_FORMAT dxgi_format)
{
    const struct vkd3d_format *formats;
    unsigned int i;

    VKD3D_ASSERT(device);
    formats = device->depth_stencil_formats;

    for (i = 0; i < ARRAY_SIZE(vkd3d_depth_stencil_formats); ++i)
    {
        if (formats[i].dxgi_format == dxgi_format)
            return &formats[i];
    }

    return NULL;
}

const struct vkd3d_format *vkd3d_get_format(const struct d3d12_device *device,
        DXGI_FORMAT dxgi_format, bool depth_stencil)
{
    const struct vkd3d_format *format;
    unsigned int i;

    if (depth_stencil && (format = vkd3d_get_depth_stencil_format(device, dxgi_format)))
        return format;

    for (i = 0; i < ARRAY_SIZE(vkd3d_formats); ++i)
    {
        if (vkd3d_formats[i].dxgi_format == dxgi_format)
            return &vkd3d_formats[i];
    }

    /* Combined depth/stencil formats can be used without
     * D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL as well. In some ways it may be
     * nicer to add entries for these to vkd3d_formats[], much like we do with
     * e.g. DXGI_FORMAT_R32_TYPELESS, but we can't handle the override for the
     * VK_FORMAT_D24_UNORM_S8_UINT formats there. */
    if ((format = vkd3d_get_depth_stencil_format(device, dxgi_format)))
        return format;

    /* Do not check VkPhysicalDevice4444FormatsFeaturesEXT because apps
     * should query format support, which returns more detailed info. */
    if (dxgi_format == format_b4g4r4a4.dxgi_format && device->vk_info.EXT_4444_formats)
        return &format_b4g4r4a4;

    return NULL;
}

const struct vkd3d_format *vkd3d_find_uint_format(const struct d3d12_device *device, DXGI_FORMAT dxgi_format)
{
    DXGI_FORMAT typeless_format = DXGI_FORMAT_UNKNOWN;
    const struct vkd3d_format *vkd3d_format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_format_compatibility_info); ++i)
    {
        if (vkd3d_format_compatibility_info[i].format == dxgi_format)
        {
            typeless_format = vkd3d_format_compatibility_info[i].typeless_format;
            break;
        }
    }

    if (!typeless_format)
        return NULL;

    for (i = 0; i < ARRAY_SIZE(vkd3d_format_compatibility_info); ++i)
    {
        if (vkd3d_format_compatibility_info[i].typeless_format != typeless_format)
            continue;

        vkd3d_format = vkd3d_get_format(device, vkd3d_format_compatibility_info[i].format, false);
        if (vkd3d_format->type == VKD3D_FORMAT_TYPE_UINT)
            return vkd3d_format;
    }

    return NULL;
}

void vkd3d_format_copy_data(const struct vkd3d_format *format, const uint8_t *src,
        unsigned int src_row_pitch, unsigned int src_slice_pitch, uint8_t *dst, unsigned int dst_row_pitch,
        unsigned int dst_slice_pitch, unsigned int w, unsigned int h, unsigned int d)
{
    unsigned int row_block_count, row_count, row_size, slice, row;
    unsigned int slice_count = d;
    const uint8_t *src_row;
    uint8_t *dst_row;

    row_block_count = (w + format->block_width - 1) / format->block_width;
    row_count = (h + format->block_height - 1) / format->block_height;
    row_size = row_block_count * format->byte_count * format->block_byte_count;

    for (slice = 0; slice < slice_count; ++slice)
    {
        for (row = 0; row < row_count; ++row)
        {
            src_row = &src[slice * src_slice_pitch + row * src_row_pitch];
            dst_row = &dst[slice * dst_slice_pitch + row * dst_row_pitch];
            memcpy(dst_row, src_row, row_size);
        }
    }
}

VkFormat vkd3d_get_vk_format(DXGI_FORMAT format)
{
    const struct vkd3d_format *vkd3d_format;

    if (!(vkd3d_format = vkd3d_get_format(NULL, format, false)))
        return VK_FORMAT_UNDEFINED;

    return vkd3d_format->vk_format;
}

DXGI_FORMAT vkd3d_get_dxgi_format(VkFormat format)
{
    DXGI_FORMAT dxgi_format;
    VkFormat vk_format;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(vkd3d_formats); ++i)
    {
        vk_format = vkd3d_formats[i].vk_format;
        dxgi_format = vkd3d_formats[i].dxgi_format;
        if (vk_format == format && vkd3d_formats[i].type != VKD3D_FORMAT_TYPE_TYPELESS)
            return dxgi_format;
    }

    FIXME("Unhandled Vulkan format %#x.\n", format);
    return DXGI_FORMAT_UNKNOWN;
}

bool is_valid_feature_level(D3D_FEATURE_LEVEL feature_level)
{
    static const D3D_FEATURE_LEVEL valid_feature_levels[] =
    {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
        D3D_FEATURE_LEVEL_1_0_CORE,
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(valid_feature_levels); ++i)
    {
        if (valid_feature_levels[i] == feature_level)
            return true;
    }

    return false;
}

bool is_write_resource_state(D3D12_RESOURCE_STATES state)
{
    return state & (D3D12_RESOURCE_STATE_RENDER_TARGET
            | D3D12_RESOURCE_STATE_UNORDERED_ACCESS
            | D3D12_RESOURCE_STATE_DEPTH_WRITE
            | D3D12_RESOURCE_STATE_STREAM_OUT
            | D3D12_RESOURCE_STATE_COPY_DEST
            | D3D12_RESOURCE_STATE_RESOLVE_DEST);
}

static bool is_power_of_two(unsigned int x)
{
    return x && !(x & (x -1));
}

bool is_valid_resource_state(D3D12_RESOURCE_STATES state)
{
    const D3D12_RESOURCE_STATES valid_states =
            D3D12_RESOURCE_STATE_COMMON |
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
            D3D12_RESOURCE_STATE_INDEX_BUFFER |
            D3D12_RESOURCE_STATE_RENDER_TARGET |
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
            D3D12_RESOURCE_STATE_DEPTH_WRITE |
            D3D12_RESOURCE_STATE_DEPTH_READ |
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_STREAM_OUT |
            D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
            D3D12_RESOURCE_STATE_COPY_DEST |
            D3D12_RESOURCE_STATE_COPY_SOURCE |
            D3D12_RESOURCE_STATE_RESOLVE_DEST |
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
            D3D12_RESOURCE_STATE_GENERIC_READ |
            D3D12_RESOURCE_STATE_PRESENT |
            D3D12_RESOURCE_STATE_PREDICATION;

    if (state & ~valid_states)
    {
        WARN("Invalid resource states %#x.\n", state & ~valid_states);
        return false;
    }

    /* Exactly one bit must be set for write states. */
    if (is_write_resource_state(state) && !is_power_of_two(state))
    {
        WARN("Write state cannot be mixed with other states: %#x.\n", state);
        return false;
    }

    return true;
}

HRESULT return_interface(void *iface, REFIID iface_iid,
        REFIID requested_iid, void **object)
{
    IUnknown *unknown = iface;
    HRESULT hr;

    if (IsEqualGUID(iface_iid, requested_iid))
    {
        *object = unknown;
        return S_OK;
    }

    hr = IUnknown_QueryInterface(unknown, requested_iid, object);
    IUnknown_Release(unknown);
    return hr;
}

struct debug_buffer
{
    struct vkd3d_string_buffer b;
};

static void debug_buffer_cleanup(struct debug_buffer *buffer)
{
    vkd3d_string_buffer_cleanup(&buffer->b);
}

static void debug_buffer_init(struct debug_buffer *buffer)
{
    vkd3d_string_buffer_init(&buffer->b);
}

static void debug_buffer_append(struct debug_buffer *buffer, const char *format, ...)
{
    va_list args;

    if (buffer->b.content_size)
        vkd3d_string_buffer_printf(&buffer->b, " | ");

    va_start(args, format);
    vkd3d_string_buffer_vprintf(&buffer->b, format, args);
    va_end(args);
}

static void debug_buffer_append_flag(struct debug_buffer *buffer, uint32_t *flags, uint32_t flag, const char *name)
{
    if (!(*flags & flag))
        return;
    debug_buffer_append(buffer, "%s", name);
    *flags &= ~flag;
}

static const char *debug_buffer_ret(struct debug_buffer *buffer, uint32_t flags, const char *default_string)
{
    const char *ret = default_string;

    if (flags)
        debug_buffer_append(buffer, "%#x", flags);
    if (buffer->b.content_size)
        ret = vkd3d_dbg_sprintf("%s", buffer->b.buffer);
    debug_buffer_cleanup(buffer);

    return ret;
}

const char *debug_cpu_handle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return vkd3d_dbg_sprintf("{%#"PRIxPTR"}", (uintptr_t)handle.ptr);
}

const char *debug_d3d12_box(const D3D12_BOX *box)
{
    if (!box)
        return "(null)";

    return vkd3d_dbg_sprintf("(%u, %u, %u)-(%u, %u, %u)",
            box->left, box->top, box->front,
            box->right, box->bottom, box->back);
}

const char *debug_d3d12_comparison_func(D3D12_COMPARISON_FUNC f)
{
    switch (f)
    {
#define TO_STR(f) case D3D12_COMPARISON_FUNC_##f: return #f;
        TO_STR(NEVER);
        TO_STR(LESS);
        TO_STR(EQUAL);
        TO_STR(LESS_EQUAL);
        TO_STR(GREATER);
        TO_STR(NOT_EQUAL);
        TO_STR(GREATER_EQUAL);
        TO_STR(ALWAYS);
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", f);
}

const char *debug_d3d12_descriptor_range(const D3D12_DESCRIPTOR_RANGE *r)
{
    unsigned int first, last;
    const char *type = NULL;

    switch (r->RangeType)
    {
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            type = "srv";
            break;

        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            type = "uav";
            break;

        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            type = "cbv";
            break;

        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            type = "sampler";
            break;
    }

    first = r->BaseShaderRegister;
    last = first + r->NumDescriptors - 1;

    if (type)
        return vkd3d_dbg_sprintf("%s[%u:%u], space %u, offset %u",
                type, first, last, r->RegisterSpace, r->OffsetInDescriptorsFromTableStart);

    return vkd3d_dbg_sprintf("unknown<%#x>[%u:%u], space %u, offset %u",
            r->RangeType, first, last, r->RegisterSpace,
            r->OffsetInDescriptorsFromTableStart);
}

const char *debug_d3d12_feature(D3D12_FEATURE f)
{
    switch (f)
    {
#define TO_STR(f) case D3D12_FEATURE_##f: return #f;
        TO_STR(D3D12_OPTIONS)
        TO_STR(ARCHITECTURE)
        TO_STR(FEATURE_LEVELS)
        TO_STR(FORMAT_SUPPORT)
        TO_STR(MULTISAMPLE_QUALITY_LEVELS)
        TO_STR(FORMAT_INFO)
        TO_STR(GPU_VIRTUAL_ADDRESS_SUPPORT)
        TO_STR(SHADER_MODEL)
        TO_STR(D3D12_OPTIONS1)
        TO_STR(PROTECTED_RESOURCE_SESSION_SUPPORT)
        TO_STR(ROOT_SIGNATURE)
        TO_STR(ARCHITECTURE1)
        TO_STR(D3D12_OPTIONS2)
        TO_STR(SHADER_CACHE)
        TO_STR(COMMAND_QUEUE_PRIORITY)
        TO_STR(D3D12_OPTIONS3)
        TO_STR(EXISTING_HEAPS)
        TO_STR(D3D12_OPTIONS4)
        TO_STR(SERIALIZATION)
        TO_STR(CROSS_NODE)
        TO_STR(D3D12_OPTIONS5)
        TO_STR(DISPLAYABLE)
        TO_STR(D3D12_OPTIONS6)
        TO_STR(QUERY_META_COMMAND)
        TO_STR(D3D12_OPTIONS7)
        TO_STR(PROTECTED_RESOURCE_SESSION_TYPE_COUNT)
        TO_STR(PROTECTED_RESOURCE_SESSION_TYPES)
        TO_STR(D3D12_OPTIONS8)
        TO_STR(D3D12_OPTIONS9)
        TO_STR(D3D12_OPTIONS10)
        TO_STR(D3D12_OPTIONS11)
        TO_STR(D3D12_OPTIONS12)
        TO_STR(D3D12_OPTIONS13)
        TO_STR(D3D12_OPTIONS14)
        TO_STR(D3D12_OPTIONS15)
        TO_STR(D3D12_OPTIONS16)
        TO_STR(D3D12_OPTIONS17)
        TO_STR(D3D12_OPTIONS18)
        TO_STR(D3D12_OPTIONS19)
        TO_STR(D3D12_OPTIONS20)
        TO_STR(PREDICATION)
        TO_STR(PLACED_RESOURCE_SUPPORT_INFO)
        TO_STR(HARDWARE_COPY)
        TO_STR(D3D12_OPTIONS21)
        TO_STR(D3D12_TIGHT_ALIGNMENT)
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", f);
}

const char *debug_d3d12_filter(D3D12_FILTER f)
{
    switch (f)
    {
#define TO_STR(f) case D3D12_FILTER_##f: return #f;
        TO_STR(MIN_MAG_MIP_POINT);
        TO_STR(MIN_MAG_POINT_MIP_LINEAR);
        TO_STR(MIN_POINT_MAG_LINEAR_MIP_POINT);
        TO_STR(MIN_POINT_MAG_MIP_LINEAR);
        TO_STR(MIN_LINEAR_MAG_MIP_POINT);
        TO_STR(MIN_LINEAR_MAG_POINT_MIP_LINEAR);
        TO_STR(MIN_MAG_LINEAR_MIP_POINT);
        TO_STR(MIN_MAG_MIP_LINEAR);
        TO_STR(ANISOTROPIC);
        TO_STR(COMPARISON_MIN_MAG_MIP_POINT);
        TO_STR(COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
        TO_STR(COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT);
        TO_STR(COMPARISON_MIN_POINT_MAG_MIP_LINEAR);
        TO_STR(COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
        TO_STR(COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
        TO_STR(COMPARISON_MIN_MAG_LINEAR_MIP_POINT);
        TO_STR(COMPARISON_MIN_MAG_MIP_LINEAR);
        TO_STR(COMPARISON_ANISOTROPIC);
        TO_STR(MINIMUM_MIN_MAG_MIP_POINT);
        TO_STR(MINIMUM_MIN_MAG_POINT_MIP_LINEAR);
        TO_STR(MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
        TO_STR(MINIMUM_MIN_POINT_MAG_MIP_LINEAR);
        TO_STR(MINIMUM_MIN_LINEAR_MAG_MIP_POINT);
        TO_STR(MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
        TO_STR(MINIMUM_MIN_MAG_LINEAR_MIP_POINT);
        TO_STR(MINIMUM_MIN_MAG_MIP_LINEAR);
        TO_STR(MINIMUM_ANISOTROPIC);
        TO_STR(MAXIMUM_MIN_MAG_MIP_POINT);
        TO_STR(MAXIMUM_MIN_MAG_POINT_MIP_LINEAR);
        TO_STR(MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
        TO_STR(MAXIMUM_MIN_POINT_MAG_MIP_LINEAR);
        TO_STR(MAXIMUM_MIN_LINEAR_MAG_MIP_POINT);
        TO_STR(MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
        TO_STR(MAXIMUM_MIN_MAG_LINEAR_MIP_POINT);
        TO_STR(MAXIMUM_MIN_MAG_MIP_LINEAR);
        TO_STR(MAXIMUM_ANISOTROPIC);
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", f);
}

const char *debug_d3d12_root_signature_flags(D3D12_ROOT_SIGNATURE_FLAGS flags)
{
    struct debug_buffer buffer;

    debug_buffer_init(&buffer);
#define FLAG_TO_STR(f) debug_buffer_append_flag(&buffer, (uint32_t *)&flags, D3D12_ROOT_SIGNATURE_FLAG_##f, #f)
    FLAG_TO_STR(NONE);
    FLAG_TO_STR(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    FLAG_TO_STR(DENY_VERTEX_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(DENY_HULL_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(DENY_DOMAIN_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(DENY_GEOMETRY_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(DENY_PIXEL_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(ALLOW_STREAM_OUTPUT);
    FLAG_TO_STR(LOCAL_ROOT_SIGNATURE);
    FLAG_TO_STR(DENY_AMPLIFICATION_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(DENY_MESH_SHADER_ROOT_ACCESS);
    FLAG_TO_STR(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED);
    FLAG_TO_STR(SAMPLER_HEAP_DIRECTLY_INDEXED);
#undef FLAG_TO_STR

    return debug_buffer_ret(&buffer, flags, "NONE");
}

static const char *debug_d3d12_shader_component(D3D12_SHADER_COMPONENT_MAPPING component)
{
    switch (component)
    {
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0:
            return "r";
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1:
            return "g";
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2:
            return "b";
        case D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3:
            return "a";
        case D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0:
            return "0";
        case D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1:
            return "1";
    }

    FIXME("Invalid component mapping %#x.\n", component);
    return "invalid";
}

const char *debug_d3d12_shader_component_mapping(unsigned int mapping)
{
    return vkd3d_dbg_sprintf("{%s, %s, %s, %s}",
            debug_d3d12_shader_component(D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(0, mapping)),
            debug_d3d12_shader_component(D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(1, mapping)),
            debug_d3d12_shader_component(D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(2, mapping)),
            debug_d3d12_shader_component(D3D12_DECODE_SHADER_4_COMPONENT_MAPPING(3, mapping)));
}

const char *debug_d3d12_shader_visibility(D3D12_SHADER_VISIBILITY v)
{
    switch (v)
    {
#define TO_STR(v) case D3D12_SHADER_VISIBILITY_##v: return #v;
        TO_STR(ALL);
        TO_STR(VERTEX);
        TO_STR(HULL);
        TO_STR(DOMAIN);
        TO_STR(GEOMETRY);
        TO_STR(PIXEL);
        TO_STR(AMPLIFICATION);
        TO_STR(MESH);
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", v);
}

const char *debug_d3d12_static_border_color(D3D12_STATIC_BORDER_COLOR c)
{
    switch (c)
    {
#define TO_STR(c) case D3D12_STATIC_BORDER_COLOR_##c: return #c;
        TO_STR(TRANSPARENT_BLACK);
        TO_STR(OPAQUE_BLACK);
        TO_STR(OPAQUE_WHITE);
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", c);
}

const char *debug_d3d12_texture_address_mode(D3D12_TEXTURE_ADDRESS_MODE m)
{
    switch (m)
    {
#define TO_STR(m) case D3D12_TEXTURE_ADDRESS_MODE_##m: return #m;
        TO_STR(WRAP);
        TO_STR(MIRROR);
        TO_STR(CLAMP);
        TO_STR(BORDER);
        TO_STR(MIRROR_ONCE);
#undef TO_STR
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", m);
}

const char *debug_denormal_mode(enum vkd3d_shader_denormal_mode m)
{
    switch (m)
    {
#define TO_STR(u) case VKD3D_SHADER_DENORMAL_MODE_##u: return #u;
        TO_STR(ANY);
        TO_STR(PRESERVE);
        TO_STR(FLUSH_TO_ZERO);
#undef TO_STR
        case VKD3D_SHADER_DENORMAL_MODE_FORCE_32BIT:
            break;
    }

    return vkd3d_dbg_sprintf("UNKNOWN(%#x)", m);
}

const char *debug_gpu_handle(D3D12_GPU_DESCRIPTOR_HANDLE handle)
{
    return vkd3d_dbg_sprintf("{%#"PRIx64"}", handle.ptr);
}

const char *debug_vk_extent_3d(VkExtent3D extent)
{
    return vkd3d_dbg_sprintf("(%u, %u, %u)",
            (unsigned int)extent.width,
            (unsigned int)extent.height,
            (unsigned int)extent.depth);
}

const char *debug_vk_queue_flags(VkQueueFlags flags)
{
    struct debug_buffer buffer;

    debug_buffer_init(&buffer);
#define FLAG_TO_STR(f) debug_buffer_append_flag(&buffer, (uint32_t *)&flags, f, #f)
    FLAG_TO_STR(VK_QUEUE_GRAPHICS_BIT);
    FLAG_TO_STR(VK_QUEUE_COMPUTE_BIT);
    FLAG_TO_STR(VK_QUEUE_TRANSFER_BIT);
    FLAG_TO_STR(VK_QUEUE_SPARSE_BINDING_BIT);
    FLAG_TO_STR(VK_QUEUE_PROTECTED_BIT);
#undef FLAG_TO_STR
#define FLAG_TO_STR(f, n) debug_buffer_append_flag(&buffer, (uint32_t *)&flags, f, #n)
    FLAG_TO_STR(0x20, VK_QUEUE_VIDEO_DECODE_BIT_KHR);
    FLAG_TO_STR(0x40, VK_QUEUE_VIDEO_ENCODE_BIT_KHR);
    FLAG_TO_STR(0x100, VK_QUEUE_OPTICAL_FLOW_BIT_NV);
#undef FLAG_TO_STR

    return debug_buffer_ret(&buffer, flags, "0");
}

const char *debug_vk_memory_heap_flags(VkMemoryHeapFlags flags)
{
    struct debug_buffer buffer;

    debug_buffer_init(&buffer);
#define FLAG_TO_STR(f) debug_buffer_append_flag(&buffer, (uint32_t *)&flags, f, #f)
    FLAG_TO_STR(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
    FLAG_TO_STR(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
#undef FLAG_TO_STR

    return debug_buffer_ret(&buffer, flags, "0");
}

const char *debug_vk_memory_property_flags(VkMemoryPropertyFlags flags)
{
    struct debug_buffer buffer;

    debug_buffer_init(&buffer);
#define FLAG_TO_STR(f) debug_buffer_append_flag(&buffer, (uint32_t *)&flags, f, #f)
    FLAG_TO_STR(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_PROTECTED_BIT);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
    FLAG_TO_STR(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
#undef FLAG_TO_STR

    return debug_buffer_ret(&buffer, flags, "0");
}

HRESULT hresult_from_errno(int rc)
{
    switch (rc)
    {
        case 0:
            return S_OK;
        case ENOMEM:
            return E_OUTOFMEMORY;
        case EINVAL:
            return E_INVALIDARG;
        default:
            FIXME("Unhandled errno %d.\n", rc);
            return E_FAIL;
    }
}

HRESULT hresult_from_vk_result(VkResult vr)
{
    switch (vr)
    {
        case VK_SUCCESS:
            return S_OK;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            WARN("Out of device memory.\n");
            /* fall-through */
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return E_OUTOFMEMORY;
        default:
            FIXME("Unhandled VkResult %d.\n", vr);
            /* fall-through */
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return E_FAIL;
    }
}

#define LOAD_GLOBAL_PFN(name) \
    if (!(procs->name = (void *)vkGetInstanceProcAddr(NULL, #name))) \
    { \
        ERR("Could not get global proc addr for '" #name "'.\n"); \
        return E_FAIL; \
    }

HRESULT vkd3d_load_vk_global_procs(struct vkd3d_vk_global_procs *procs,
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr)
{
    memset(procs, 0, sizeof(*procs));

    procs->vkGetInstanceProcAddr = vkGetInstanceProcAddr;

    LOAD_GLOBAL_PFN(vkCreateInstance)
    LOAD_GLOBAL_PFN(vkEnumerateInstanceExtensionProperties)

    TRACE("Loaded global Vulkan procs.\n");
    return S_OK;
}

#define LOAD_INSTANCE_PFN(name) \
    if (!(procs->name = (void *)global_procs->vkGetInstanceProcAddr(instance, #name))) \
    { \
        ERR("Could not get instance proc addr for '" #name "'.\n"); \
        return E_FAIL; \
    }
#define LOAD_INSTANCE_OPTIONAL_PFN(name) \
    procs->name = (void *)global_procs->vkGetInstanceProcAddr(instance, #name);

HRESULT vkd3d_load_vk_instance_procs(struct vkd3d_vk_instance_procs *procs,
        const struct vkd3d_vk_global_procs *global_procs, VkInstance instance)
{
    memset(procs, 0, sizeof(*procs));

#define VK_INSTANCE_PFN     LOAD_INSTANCE_PFN
#define VK_INSTANCE_EXT_PFN LOAD_INSTANCE_OPTIONAL_PFN
#include "vulkan_procs.h"

    TRACE("Loaded procs for VkInstance %p.\n", instance);
    return S_OK;
}

#define COPY_PARENT_PFN(name) procs->name = parent_procs->name;
#define LOAD_DEVICE_PFN(name) \
    if (!(procs->name = (void *)procs->vkGetDeviceProcAddr(device, #name))) \
    { \
        ERR("Could not get device proc addr for '" #name "'.\n"); \
        return E_FAIL; \
    }
#define LOAD_DEVICE_OPTIONAL_PFN(name) \
    procs->name = (void *)procs->vkGetDeviceProcAddr(device, #name);

HRESULT vkd3d_load_vk_device_procs(struct vkd3d_vk_device_procs *procs,
        const struct vkd3d_vk_instance_procs *parent_procs, VkDevice device)
{
    memset(procs, 0, sizeof(*procs));

#define VK_INSTANCE_PFN   COPY_PARENT_PFN
#define VK_DEVICE_PFN     LOAD_DEVICE_PFN
#define VK_DEVICE_EXT_PFN LOAD_DEVICE_OPTIONAL_PFN
#include "vulkan_procs.h"

    TRACE("Loaded procs for VkDevice %p.\n", device);
    return S_OK;
}

#if HAVE_DECL_PROGRAM_INVOCATION_NAME

bool vkd3d_get_program_name(char program_name[PATH_MAX])
{
    char *name, *p, *real_path = NULL;

    if ((name = strrchr(program_invocation_name, '/')))
    {
        real_path = realpath("/proc/self/exe", NULL);

        /* Try to strip command line arguments. */
        if (real_path && (p = strrchr(real_path, '/'))
                && !strncmp(real_path, program_invocation_name, strlen(real_path)))
        {
            name = p;
        }

        ++name;
    }
    else if ((name = strrchr(program_invocation_name, '\\')))
    {
        ++name;
    }
    else
    {
        name = program_invocation_name;
    }

    strncpy(program_name, name, PATH_MAX);
    program_name[PATH_MAX - 1] = '\0';
    free(real_path);
    return true;
}

#elif defined(WIN32)

bool vkd3d_get_program_name(char program_name[PATH_MAX])
{
    char buffer[MAX_PATH];
    char *p, *name;
    size_t len;

    *program_name = '\0';
    len = GetModuleFileNameA(NULL, buffer, ARRAY_SIZE(buffer));
    if (!(len && len < MAX_PATH))
        return false;

    name = buffer;
    if ((p = strrchr(name, '/')))
        name = p + 1;
    if ((p = strrchr(name, '\\')))
        name = p + 1;

    len = strlen(name) + 1;
    memcpy(program_name, name, len);
    return true;
}

#else

bool vkd3d_get_program_name(char program_name[PATH_MAX])
{
    *program_name = '\0';
    return false;
}

#endif  /* HAVE_DECL_PROGRAM_INVOCATION_NAME */

static struct vkd3d_private_data *vkd3d_private_store_get_private_data(
        const struct vkd3d_private_store *store, const GUID *tag)
{
    struct vkd3d_private_data *data;

    LIST_FOR_EACH_ENTRY(data, &store->content, struct vkd3d_private_data, entry)
    {
        if (IsEqualGUID(&data->tag, tag))
            return data;
    }

    return NULL;
}

static HRESULT vkd3d_private_store_set_private_data(struct vkd3d_private_store *store,
        const GUID *tag, const void *data, unsigned int data_size, bool is_object)
{
    struct vkd3d_private_data *d, *old_data;
    const void *ptr = data;

    if (!data)
    {
        if ((d = vkd3d_private_store_get_private_data(store, tag)))
        {
            vkd3d_private_data_destroy(d);
            return S_OK;
        }

        return S_FALSE;
    }

    if (is_object)
    {
        if (data_size != sizeof(IUnknown *))
            return E_INVALIDARG;
        ptr = &data;
    }

    if (!(d = vkd3d_malloc(offsetof(struct vkd3d_private_data, u.data[data_size]))))
        return E_OUTOFMEMORY;

    d->tag = *tag;
    d->size = data_size;
    d->is_object = is_object;
    memcpy(d->u.data, ptr, data_size);
    if (is_object)
        IUnknown_AddRef(d->u.object);

    if ((old_data = vkd3d_private_store_get_private_data(store, tag)))
        vkd3d_private_data_destroy(old_data);
    list_add_tail(&store->content, &d->entry);

    return S_OK;
}

HRESULT vkd3d_get_private_data(struct vkd3d_private_store *store,
        const GUID *tag, unsigned int *out_size, void *out)
{
    const struct vkd3d_private_data *data;
    HRESULT hr = S_OK;
    unsigned int size;

    if (!out_size)
        return E_INVALIDARG;

    vkd3d_mutex_lock(&store->mutex);

    if (!(data = vkd3d_private_store_get_private_data(store, tag)))
    {
        *out_size = 0;
        hr = DXGI_ERROR_NOT_FOUND;
        goto done;
    }

    size = *out_size;
    *out_size = data->size;
    if (!out)
        goto done;

    if (size < data->size)
    {
        hr = DXGI_ERROR_MORE_DATA;
        goto done;
    }

    if (data->is_object)
        IUnknown_AddRef(data->u.object);
    memcpy(out, data->u.data, data->size);

done:
    vkd3d_mutex_unlock(&store->mutex);
    return hr;
}

HRESULT vkd3d_set_private_data(struct vkd3d_private_store *store,
        const GUID *tag, unsigned int data_size, const void *data)
{
    HRESULT hr;

    vkd3d_mutex_lock(&store->mutex);

    hr = vkd3d_private_store_set_private_data(store, tag, data, data_size, false);

    vkd3d_mutex_unlock(&store->mutex);
    return hr;
}

HRESULT vkd3d_set_private_data_interface(struct vkd3d_private_store *store,
        const GUID *tag, const IUnknown *object)
{
    const void *data = object ? object : (void *)&object;
    HRESULT hr;

    vkd3d_mutex_lock(&store->mutex);

    hr = vkd3d_private_store_set_private_data(store, tag, data, sizeof(object), !!object);

    vkd3d_mutex_unlock(&store->mutex);
    return hr;
}

VkResult vkd3d_set_vk_object_name_utf8(struct d3d12_device *device, uint64_t vk_object,
        VkDebugReportObjectTypeEXT vk_object_type, const char *name)
{
    const struct vkd3d_vk_device_procs *vk_procs = &device->vk_procs;
    VkDebugMarkerObjectNameInfoEXT info;

    if (!device->vk_info.EXT_debug_marker)
        return VK_SUCCESS;

    info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
    info.pNext = NULL;
    info.objectType = vk_object_type;
    info.object = vk_object;
    info.pObjectName = name;
    return VK_CALL(vkDebugMarkerSetObjectNameEXT(device->vk_device, &info));
}

HRESULT vkd3d_set_vk_object_name(struct d3d12_device *device, uint64_t vk_object,
        VkDebugReportObjectTypeEXT vk_object_type, const WCHAR *name)
{
    char *name_utf8;
    VkResult vr;

    if (!name)
        return E_INVALIDARG;

    if (!device->vk_info.EXT_debug_marker)
        return S_OK;

    if (!(name_utf8 = vkd3d_strdup_w_utf8(name, device->wchar_size)))
        return E_OUTOFMEMORY;

    vr = vkd3d_set_vk_object_name_utf8(device, vk_object, vk_object_type, name_utf8);

    vkd3d_free(name_utf8);

    return hresult_from_vk_result(vr);
}
