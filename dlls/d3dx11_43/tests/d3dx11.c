/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS
#include "initguid.h"
#include "d3d11.h"
#include "d3dx11.h"
#include "wine/test.h"
#include <stdint.h>

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

static unsigned int get_bpp_from_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        default:
            return 0;
    }
}

/* 1x1 bmp (1 bpp) */
static const unsigned char bmp_1bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (2 bpp) */
static const unsigned char bmp_2bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (4 bpp) */
static const unsigned char bmp_4bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (8 bpp) */
static const unsigned char bmp_8bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 2x2 bmp (32 bpp XRGB) */
static const unsigned char bmp_32bpp_xrgb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x00
};

/* 2x2 bmp (32 bpp ARGB) */
static const unsigned char bmp_32bpp_argb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x01
};

static const unsigned char png_grayscale[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49,
    0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44,
    0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01, 0x01, 0x01, 0x00, 0x1b,
    0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
    0x60, 0x82
};

/* 2x2 24-bit dds, 2 mipmaps */
static const unsigned char dds_24bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x02,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/* 2x2 16-bit dds, no mipmaps */
static const unsigned char dds_16bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,
    0xe0,0x03,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0x7f,0xff,0x7f,0xff,0x7f,0xff,0x7f
};

/* 16x4 8-bit dds  */
static const unsigned char dds_8bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x00,0x00,0x04,0x00,0x00,0x00,
    0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x47,0x49,0x4d,0x50,0x2d,0x44,0x44,0x53,0x5a,0x09,0x03,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0x8c,0xcd,0x12,0xff,
    0x78,0x01,0x14,0xff,0x50,0xcd,0x12,0xff,0x00,0x3d,0x8c,0xff,0x02,0x00,0x00,0xff,
    0x47,0x00,0x00,0xff,0xda,0x07,0x02,0xff,0x50,0xce,0x12,0xff,0xea,0x11,0x01,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x08,0x3d,0x8c,0xff,0x08,0x01,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x60,0xcc,0x12,0xff,
    0xa1,0xb2,0xd4,0xff,0xda,0x07,0x02,0xff,0x47,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x50,0xce,0x12,0xff,0x00,0x00,0x14,0xff,0xa8,0xcc,0x12,0xff,0x3c,0xb2,0xd4,0xff,
    0xda,0x07,0x02,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x01,0xff,
    0x21,0x00,0x00,0xff,0xd8,0xcb,0x12,0xff,0x54,0xcd,0x12,0xff,0x8b,0x4f,0xd5,0xff,
    0x00,0x04,0xda,0xff,0x00,0x00,0x00,0xff,0x3d,0x04,0x91,0xff,0x70,0xce,0x18,0xff,
    0xb4,0xcc,0x12,0xff,0x6b,0x4e,0xd5,0xff,0xb0,0xcc,0x12,0xff,0x00,0x00,0x00,0xff,
    0xc8,0x05,0x91,0xff,0x98,0xc7,0xcc,0xff,0x7c,0xcd,0x12,0xff,0x51,0x05,0x91,0xff,
    0x48,0x07,0x14,0xff,0x6d,0x05,0x91,0xff,0x00,0x07,0xda,0xff,0xa0,0xc7,0xcc,0xff,
    0x00,0x07,0xda,0xff,0x3a,0x77,0xd5,0xff,0xda,0x07,0x02,0xff,0x7c,0x94,0xd4,0xff,
    0xe0,0xce,0xd6,0xff,0x0a,0x80,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x78,0x9a,0xab,0xff,0xde,0x08,0x18,0xff,0xda,0x07,0x02,0xff,0x30,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x50,0xce,0x12,0xff,0x8c,0xcd,0x12,0xff,0xd0,0xb7,0xd8,0xff,
    0x00,0x00,0x00,0xff,0x60,0x32,0xd9,0xff,0x30,0xc1,0x1a,0xff,0xa8,0xcd,0x12,0xff,
    0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,0x46,0x71,0x0e,0xff,0xc0,0x1d,0x4b,0xff,
    0x09,0x87,0xd4,0xff,0x00,0x00,0x00,0xff,0xf6,0x22,0x00,0xff,0x64,0xcd,0x12,0xff,
    0x00,0x00,0x00,0xff,0xca,0x1d,0x4b,0xff,0x09,0x87,0xd4,0xff,0xaa,0x02,0x05,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0xc0,0x1d,0x4b,0xff,
    0xcd,0xab,0xba,0xff,0x00,0x00,0x00,0xff,0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,
    0xd4,0xcd,0x12,0xff,0xa6,0x4c,0xd5,0xff,0x00,0xf0,0xfd,0xff,0xd4,0xcd,0x12,0xff,
    0xf4,0x4c,0xd5,0xff,0x90,0xcd,0x12,0xff,0xc2,0x4c,0xd5,0xff,0x82,0x00,0x00,0xff,
    0xaa,0x02,0x05,0xff,0x88,0xd4,0xba,0xff,0x14,0x00,0x00,0xff,0x01,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x0c,0x08,0x13,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xd0,0xcd,0x12,0xff,0xc6,0x84,0xf1,0xff,0x7c,0x84,0xf1,0xff,0x20,0x20,0xf5,0xff,
    0x00,0x00,0x0a,0xff,0xf0,0xb0,0x94,0xff,0x64,0x6c,0xf1,0xff,0x85,0x6c,0xf1,0xff,
    0x8b,0x4f,0xd5,0xff,0x00,0x04,0xda,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,
    0x39,0xde,0xd4,0xff,0x10,0x50,0xd5,0xff,0xaa,0x02,0x05,0xff,0x00,0x00,0x00,0xff,
    0x4f,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,
    0xaa,0x02,0x05,0xff,0x4c,0xce,0x12,0xff,0x39,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5b,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x68,0x50,0xcd,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0xe3,0xea,0x90,0xff,0x5c,0xce,0x12,0xff,
    0x18,0x00,0x00,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
};

/* 4x4 cube map dds */
static const unsigned char dds_cube_map[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50
};

/* 4x4x2 volume map dds, 2 mipmaps */
static const unsigned char dds_volume_map[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x8a,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x2f,0x7e,0xcf,0x79,0x01,0x54,0x5c,0x5c,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x84,0xef,0x7b,0xaa,0xab,0xab,0xab
};

/* invalid image file */
static const unsigned char noimage[4] =
{
    0x11,0x22,0x33,0x44
};

#define check_image_info_values(info, width, height, depth, array_size, mip_levels, misc_flags, format, resource_dimension, \
                                image_file_format, wine_todo) \
    check_image_info_values_(__LINE__, info, width, height, depth, array_size, mip_levels, misc_flags, format, resource_dimension, \
            image_file_format, wine_todo)
static inline void check_image_info_values_(unsigned int line, const D3DX11_IMAGE_INFO *info, uint32_t width,
        uint32_t height, uint32_t depth, uint32_t array_size, uint32_t mip_levels, uint32_t misc_flags,
        DXGI_FORMAT format, D3D11_RESOURCE_DIMENSION resource_dimension, D3DX11_IMAGE_FILE_FORMAT image_file_format,
        BOOL wine_todo)
{
    const D3DX11_IMAGE_INFO expected_info = { width, height, depth, array_size, mip_levels, misc_flags, format,
                                              resource_dimension, image_file_format };
    BOOL matched;

    matched = !memcmp(&expected_info, info, sizeof(*info));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected image info values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && info->Width != width)
        ok_(__FILE__, line)(info->Width == width, "Expected width %u, got %u.\n", width, info->Width);
    todo_wine_if(wine_todo && info->Height != height)
        ok_(__FILE__, line)(info->Height == height, "Expected height %u, got %u.\n", height, info->Height);
    todo_wine_if(wine_todo && info->Depth != depth)
        ok_(__FILE__, line)(info->Depth == depth, "Expected depth %u, got %u.\n", depth, info->Depth);
    todo_wine_if(wine_todo && info->ArraySize != array_size)
        ok_(__FILE__, line)(info->ArraySize == array_size, "Expected array_size %u, got %u.\n", array_size,
                info->ArraySize);
    todo_wine_if(wine_todo && info->MipLevels != mip_levels)
        ok_(__FILE__, line)(info->MipLevels == mip_levels, "Expected mip_levels %u, got %u.\n", mip_levels,
                info->MipLevels);
    todo_wine_if(wine_todo && info->MiscFlags != misc_flags)
        ok_(__FILE__, line)(info->MiscFlags == misc_flags, "Expected misc_flags %u, got %u.\n", misc_flags,
                info->MiscFlags);
    ok_(__FILE__, line)(info->Format == format, "Expected texture format %d, got %d.\n", format, info->Format);
    todo_wine_if(wine_todo && info->ResourceDimension != resource_dimension)
        ok_(__FILE__, line)(info->ResourceDimension == resource_dimension, "Expected resource_dimension %d, got %d.\n",
                resource_dimension, info->ResourceDimension);
    ok_(__FILE__, line)(info->ImageFileFormat == image_file_format, "Expected image_file_format %d, got %d.\n",
            image_file_format, info->ImageFileFormat);
}

static WCHAR temp_dir[MAX_PATH];

static BOOL create_file(const WCHAR *filename, const char *data, unsigned int size, WCHAR *out_path)
{
    WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(file, data, size, &written, NULL))
    {
        CloseHandle(file);

        if (out_path)
            lstrcpyW(out_path, path);
        return TRUE;
    }

    CloseHandle(file);
    return FALSE;
}

static void delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static BOOL create_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    return CreateDirectoryW(path, NULL);
}

static void delete_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    RemoveDirectoryW(path);
}

static void test_D3DX11CreateAsyncMemoryLoader(void)
{
    ID3DX11DataLoader *loader;
    SIZE_T size;
    DWORD data = 0;
    HRESULT hr;
    void *ptr;

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(&data, 0, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = 100;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(!size, "Got unexpected data size.\n");

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(&data, sizeof(data), &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(size == sizeof(data), "Got unexpected data size.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void create_testfile(WCHAR *path, const void *data, int data_len)
{
    DWORD written;
    HANDLE file;
    BOOL ret;

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"asyncloader.data");

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Test file creation failed, at %s, error %ld.\n",
            wine_dbgstr_w(path), GetLastError());

    ret = WriteFile(file, data, data_len, &written, NULL);
    ok(ret, "Write to test file failed.\n");

    CloseHandle(file);
}

static void test_D3DX11CreateAsyncFileLoader(void)
{
    static const char test_data1[] = "test data";
    static const char test_data2[] = "more test data";
    ID3DX11DataLoader *loader;
    WCHAR path[MAX_PATH];
    SIZE_T size;
    HRESULT hr;
    void *ptr;
    BOOL ret;

    hr = D3DX11CreateAsyncFileLoaderA(NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA(NULL, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA("nonexistentfilename", &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test file sharing using dummy empty file. */
    create_testfile(path, test_data1, sizeof(test_data1));

    hr = D3DX11CreateAsyncFileLoaderW(path, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Create it again. */
    create_testfile(path, test_data1, sizeof(test_data1));
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Already loaded. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());

    /* Already loaded, file removed. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Decompress still works. */
    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data1), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data1))
        ok(!memcmp(ptr, test_data1, size), "Got unexpected file data.\n");

    /* Create it again, with different data. */
    create_testfile(path, test_data2, sizeof(test_data2));

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data2), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data2))
        ok(!memcmp(ptr, test_data2, size), "Got unexpected file data.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());
}

static void test_D3DX11CreateAsyncResourceLoader(void)
{
    ID3DX11DataLoader *loader;
    HRESULT hr;

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, "noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, L"noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
}

static HRESULT WINAPI test_d3dinclude_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";
    char *buffer;

    trace("filename %s.\n", filename);
    trace("parent_data %p: %s.\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "include1.h"))
    {
        buffer = malloc(strlen(include1));
        memcpy(buffer, include1, strlen(include1));
        *bytes = strlen(include1);
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
        ok(!strncmp(include2, parent_data, strlen(include2)),
                "Unexpected parent_data value.\n");
    }
    else if (!strcmp(filename, "include\\include2.h"))
    {
        buffer = malloc(strlen(include2));
        memcpy(buffer, include2, strlen(include2));
        *bytes = strlen(include2);
        ok(!parent_data, "Unexpected parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
    }
    else
    {
        ok(0, "Unexpected #include for file %s.\n", filename);
        return E_INVALIDARG;
    }

    *data = buffer;
    return S_OK;
}

static HRESULT WINAPI test_d3dinclude_close(ID3DInclude *iface, const void *data)
{
    free((void *)data);
    return S_OK;
}

static const struct ID3DIncludeVtbl test_d3dinclude_vtbl =
{
    test_d3dinclude_open,
    test_d3dinclude_close
};

struct test_d3dinclude
{
    ID3DInclude ID3DInclude_iface;
};

static void test_D3DX11CompileFromFile(void)
{
    struct test_d3dinclude include = {{&test_d3dinclude_vtbl}};
    WCHAR filename[MAX_PATH], directory[MAX_PATH];
    ID3D10Blob *blob = NULL, *errors = NULL;
    CHAR filename_a[MAX_PATH];
    HRESULT hr, result;
    DWORD len;
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include1_wrong[] =
        "#define LIGHT nope\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";

    create_file(L"source.ps", ps_code, strlen(ps_code), filename);
    create_directory(L"include");
    create_file(L"include\\include1.h", include1_wrong, strlen(include1_wrong), NULL);
    create_file(L"include1.h", include1, strlen(include1), NULL);
    create_file(L"include\\include2.h", include2, strlen(include2), NULL);

    hr = D3DX11CompileFromFileW(filename, NULL, &include.ID3DInclude_iface,
            "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    /* Windows always seems to resolve includes from the initial file location
     * instead of using the immediate parent, as it would be the case for
     * standard C preprocessor includes. */
    hr = D3DX11CompileFromFileW(filename, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);
    hr = D3DX11CompileFromFileA(filename_a, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    GetCurrentDirectoryW(MAX_PATH, directory);
    SetCurrentDirectoryW(temp_dir);

    hr = D3DX11CompileFromFileW(L"source.ps", NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    ID3D10Blob_Release(blob);
    blob = NULL;

    SetCurrentDirectoryW(directory);

    delete_file(L"source.ps");
    delete_file(L"include\\include1.h");
    delete_file(L"include1.h");
    delete_file(L"include\\include2.h");
    delete_directory(L"include");
}

/* dds_header.flags */
#define DDS_CAPS 0x00000001
#define DDS_HEIGHT 0x00000002
#define DDS_WIDTH 0x00000004
#define DDS_PITCH 0x00000008
#define DDS_PIXELFORMAT 0x00001000
#define DDS_MIPMAPCOUNT 0x00020000
#define DDS_LINEARSIZE 0x00080000
#define DDS_DEPTH 0x00800000

/* dds_header.caps */
#define DDSCAPS_ALPHA    0x00000002
#define DDS_CAPS_COMPLEX 0x00000008
#define DDS_CAPS_TEXTURE 0x00001000

/* dds_header.caps2 */
#define DDS_CAPS2_VOLUME  0x00200000
#define DDS_CAPS2_CUBEMAP 0x00000200
#define DDS_CAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDS_CAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDS_CAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDS_CAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDS_CAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDS_CAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDS_CAPS2_CUBEMAP_ALL_FACES ( DDS_CAPS2_CUBEMAP_POSITIVEX | DDS_CAPS2_CUBEMAP_NEGATIVEX \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEY | DDS_CAPS2_CUBEMAP_NEGATIVEY \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEZ | DDS_CAPS2_CUBEMAP_NEGATIVEZ )

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x00000001
#define DDS_PF_ALPHA_ONLY 0x00000002
#define DDS_PF_FOURCC 0x00000004
#define DDS_PF_INDEXED 0x00000020
#define DDS_PF_RGB 0x00000040
#define DDS_PF_LUMINANCE 0x00020000
#define DDS_PF_BUMPLUMINANCE 0x00040000
#define DDS_PF_BUMPDUDV 0x00080000

struct dds_pixel_format
{
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

struct dds_header
{
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitch_or_linear_size;
    DWORD depth;
    DWORD miplevels;
    DWORD reserved[11];
    struct dds_pixel_format pixel_format;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
};

#define DDS_RESOURCE_MISC_TEXTURECUBE 0x04
struct dds_header_dxt10
{
    DWORD dxgi_format;
    DWORD resource_dimension;
    DWORD misc_flag;
    DWORD array_size;
    DWORD misc_flags2;
};

/* fills dds_header with reasonable default values */
static void fill_dds_header(struct dds_header *header)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT;
    header->height = 4;
    header->width = 4;
    header->pixel_format.size = sizeof(header->pixel_format);
    /* X8R8G8B8 */
    header->pixel_format.flags = DDS_PF_RGB;
    header->pixel_format.fourcc = 0;
    header->pixel_format.bpp = 32;
    header->pixel_format.rmask = 0xff0000;
    header->pixel_format.gmask = 0x00ff00;
    header->pixel_format.bmask = 0x0000ff;
    header->pixel_format.amask = 0;
    header->caps = DDS_CAPS_TEXTURE;
}

static void set_dxt10_dds_header(struct dds_header *header, uint32_t append_flags, uint32_t width, uint32_t height,
        uint32_t depth, uint32_t mip_levels, uint32_t pitch, uint32_t caps, uint32_t caps2)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDS_CAPS | DDS_PIXELFORMAT | append_flags;
    header->height = height;
    header->width = width;
    header->depth = depth;
    header->miplevels = mip_levels;
    header->pitch_or_linear_size = pitch;
    header->pixel_format.size = sizeof(header->pixel_format);
    header->pixel_format.flags = DDS_PF_FOURCC;
    header->pixel_format.fourcc = MAKEFOURCC('D','X','1','0');
    header->caps = caps;
    header->caps2 = caps2;
}

static void set_dds_header_dxt10(struct dds_header_dxt10 *dxt10, DXGI_FORMAT format, uint32_t resource_dimension,
        uint32_t misc_flag, uint32_t array_size, uint32_t misc_flags2)
{
    dxt10->dxgi_format = format;
    dxt10->resource_dimension = resource_dimension;
    dxt10->misc_flag = misc_flag;
    dxt10->array_size = array_size;
    dxt10->misc_flags2 = misc_flags2;
}

static void check_dds_pixel_format_image_info(unsigned int line, DWORD flags, DWORD fourcc, DWORD bpp,
        DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask, HRESULT expected_hr, DXGI_FORMAT expected_format)
{
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        PALETTEENTRY palette[256];
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);
    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    memset(dds.data, 0, sizeof(dds.data));

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr) && hr == expected_hr)
        ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x.\n", info.Format);

    /* Test again with unused fields set. */
    if (flags & DDS_PF_FOURCC)
        rmask = gmask = bmask = amask = bpp = flags = ~0u;
    else if ((flags & (DDS_PF_INDEXED | DDS_PF_ALPHA)) == (DDS_PF_INDEXED | DDS_PF_ALPHA))
        rmask = gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_INDEXED)
        rmask = gmask = bmask = amask = fourcc = ~0u;
    else if ((flags & (DDS_PF_RGB | DDS_PF_ALPHA)) == (DDS_PF_RGB | DDS_PF_ALPHA))
        fourcc = ~0u;
    else if (flags & DDS_PF_RGB)
        fourcc = amask = ~0u;
    else if ((flags & (DDS_PF_LUMINANCE | DDS_PF_ALPHA)) == (DDS_PF_LUMINANCE | DDS_PF_ALPHA))
        gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_LUMINANCE)
        gmask = bmask = amask = fourcc = ~0u;
    else if (flags & DDS_PF_ALPHA_ONLY)
        rmask = gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_BUMPDUDV)
        fourcc = ~0u;
    else if (flags & DDS_PF_BUMPLUMINANCE)
        fourcc = amask = ~0u;

    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr) && hr == expected_hr)
        ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x.\n", info.Format);
}

#define check_dds_pixel_format(flags, fourcc, bpp, rmask, gmask, bmask, amask, format) \
        check_dds_pixel_format_image_info(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, S_OK, format)

#define check_dds_pixel_format_unsupported(flags, fourcc, bpp, rmask, gmask, bmask, amask, expected_hr) \
        check_dds_pixel_format_image_info(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, expected_hr, \
                DXGI_FORMAT_UNKNOWN)

static void check_dds_dxt10_format_image_info(unsigned int line, DXGI_FORMAT format, DXGI_FORMAT expected_format,
        HRESULT expected_hr, BOOL todo_format)
{
    const unsigned int stride = (4 * get_bpp_from_format(format) + 7) / 8;
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        struct dds_header_dxt10 dxt10;
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    set_dxt10_dds_header(&dds.header, 0, 4, 4, 1, 1, stride, 0, 0);
    set_dds_header_dxt10(&dds.dxt10, format, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr) && hr == expected_hr)
        todo_wine_if(todo_format) ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x.\n",
                info.Format);
}

#define check_dds_dxt10_format(format, expected_format, todo_format) \
    check_dds_dxt10_format_image_info(__LINE__, format, expected_format, S_OK, todo_format)

#define check_dds_dxt10_format_unsupported(format, expected_hr) \
    check_dds_dxt10_format_image_info(__LINE__, format, DXGI_FORMAT_UNKNOWN, expected_hr, FALSE)

static void test_legacy_dds_header_image_info(void)
{
    struct expected
    {
        HRESULT hr;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_size;
        uint32_t mip_levels;
        uint32_t misc_flags;
        DXGI_FORMAT format;
        D3D11_RESOURCE_DIMENSION resource_dimension;
    };
    static const struct
    {
        uint32_t flags;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t row_pitch;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        uint32_t pixel_data_size;
        struct expected expected;
        BOOL todo_hr;
    } tests[] =
    {
        /*
         * Only DDS header size is validated on d3dx11, unlike d3dx9 where image pixel size
         * is as well.
         */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 1, (4 * 4), 3, 0, 0, 0,
            { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, },
        },
        /* Depth value set to 4, but no caps bits are set. Depth is ignored. */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 4, (4 * 4), 3, 0, 0, 292,
            { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, },
        },
        /* The volume texture caps2 field is ignored. */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 4, (4 * 4), 3,
            (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), DDS_CAPS2_VOLUME, 292,
            { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }
        },
        /*
         * The DDS_DEPTH flag is the only thing checked to determine if a DDS
         * file represents a 3D texture.
         */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 4, (4 * 4), 3, 0, 0, 292,
            { S_OK, 4, 4, 4, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }
        },
        /* Even if the depth field is set to 0, it's still a 3D texture. */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 0, (4 * 4), 3, 0, 0, 292,
            { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }
        },
        /*
         * 5.
         * The DDS_DEPTH flag overrides cubemap caps.
         */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 4, (4 * 4), 3,
            (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES), 292,
            { S_OK, 4, 4, 4, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }
        },
        /* Cubemap where width field does not equal height. */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 5, 1, (4 * 4), 1,
            (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES), (80 * 6),
            {
                S_OK, 4, 5, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_R8G8B8A8_UNORM,
                D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            }
        },
        /* Partial cubemaps are not supported. */
        {
            (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 1, (4 * 4), 1,
            (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_POSITIVEX), (64 * 6),
            { E_FAIL, }
        },
    };
    D3DX11_IMAGE_INFO info;
    unsigned int i;
    struct
    {
        DWORD magic;
        struct dds_header header;
    } dds;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const uint32_t file_size = sizeof(dds) + tests[i].pixel_data_size;

        winetest_push_context("Test %u", i);

        dds.magic = MAKEFOURCC('D','D','S',' ');
        fill_dds_header(&dds.header);
        dds.header.flags = tests[i].flags;
        dds.header.width = tests[i].width;
        dds.header.height = tests[i].height;
        dds.header.depth = tests[i].depth;
        dds.header.pitch_or_linear_size = tests[i].row_pitch;
        dds.header.miplevels = tests[i].mip_levels;
        dds.header.caps = tests[i].caps;
        dds.header.caps2 = tests[i].caps2;

        memset(&info, 0, sizeof(info));
        hr = D3DX11GetImageInfoFromMemory(&dds, file_size, NULL, &info, NULL);
        todo_wine_if(tests[i].todo_hr) ok(hr == tests[i].expected.hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr) && SUCCEEDED(tests[i].expected.hr))
            check_image_info_values(&info, tests[i].expected.width, tests[i].expected.height,
                    tests[i].expected.depth, tests[i].expected.array_size, tests[i].expected.mip_levels,
                    tests[i].expected.misc_flags, tests[i].expected.format,
                    tests[i].expected.resource_dimension, D3DX11_IFF_DDS, FALSE);

        winetest_pop_context();
    }

    /*
     * Image size (e.g, the size of the pixels) isn't validated, but header
     * size is.
     */
    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds) - 1, NULL, &info, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_dxt10_dds_header_image_info(void)
{
    struct expected
    {
        HRESULT hr;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_size;
        uint32_t mip_levels;
        uint32_t misc_flags;
        DXGI_FORMAT format;
        D3D11_RESOURCE_DIMENSION resource_dimension;
    };
    static const struct
    {
        uint32_t append_flags;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t row_pitch;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        struct dds_header_dxt10 dxt10;
        uint32_t pixel_data_size;
        struct expected expected;
        BOOL todo_hr;
        BOOL todo_info;
    } tests[] =
    {
        /* File size validation isn't done on d3dx11. */
        {
            0, 4, 4, 0, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, }, 0,
            { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, },
        },
        /*
         * Setting the misc_flags2 field to anything other than 0 results in
         * E_FAIL.
         */
        {
            0, 4, 4, 0, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 1, }, (4 * 4 * 4),
            { E_FAIL }
        },
        /*
         * The misc_flags field isn't passed through directly, only the
         * cube texture flag is (if it's set).
         */
        {
            0, 4, 4, 0, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0xfffffffb, 1, 0, }, (4 * 4 * 4),
            { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }
        },
        /* Resource dimension field of the header isn't validated. */
        {
            0, 4, 4, 0, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, 500, 0, 1, 0, }, (4 * 4 * 4),
            { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 500, }, .todo_hr = TRUE
        },
        /* Depth value of 2, but D3D11_RESOURCE_DIMENSION_TEXTURE2D. */
        {
            DDS_DEPTH, 4, 4, 2, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, }, (4 * 4 * 4 * 2),
            { S_OK, 4, 4, 2, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }
        },
        /*
         * 5.
         * Depth field value is ignored if DDS_DEPTH isn't set.
         */
        {
            0, 4, 4, 2, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, 0, 1, 0, }, (4 * 4 * 4 * 2),
            { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, },
        },
        /*
         * 3D texture with an array size larger than 1. Technically there's no
         * such thing as a 3D texture array, but it succeeds.
         */
        {
            DDS_DEPTH, 4, 4, 2, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, 0, 2, 0, }, (4 * 4 * 4 * 2 * 2),
            { S_OK, 4, 4, 2, 2, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }
        },
        /* Cubemap caps are ignored for DXT10 files. */
        {
            0, 4, 4, 1, (4 * 4), 1, 0, DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, }, (4 * 4 * 4 * 6),
            { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D }
        },
        /* Array size value is multiplied by 6 for cubemap files. */
        {
            0, 4, 4, 1, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
            (4 * 4 * 4 * 12),
            {
                S_OK, 4, 4, 1, 12, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_R8G8B8A8_UNORM,
                D3D11_RESOURCE_DIMENSION_TEXTURE2D
            }
        },
        /* Resource dimension is validated for cube textures. */
        {
            0, 4, 4, 1, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
            (4 * 4 * 4 * 12), { E_FAIL }
        },
        /*
         * 10.
         * 1D Texture cube, invalid.
         */
        {
            0, 4, 4, 1, (4 * 4), 1, 0, 0,
            { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE1D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
            (4 * 4 * 4 * 12), { E_FAIL }
        },
    };
    D3DX11_IMAGE_INFO info;
    unsigned int i;
    struct
    {
        DWORD magic;
        struct dds_header header;
        struct dds_header_dxt10 dxt10;
    } dds;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        const uint32_t file_size = sizeof(dds) + tests[i].pixel_data_size;

        winetest_push_context("Test %u", i);

        dds.magic = MAKEFOURCC('D','D','S',' ');
        set_dxt10_dds_header(&dds.header, tests[i].append_flags, tests[i].width, tests[i].height,
                tests[i].depth, tests[i].mip_levels, tests[i].row_pitch, tests[i].caps,
                tests[i].caps2);
        dds.dxt10 = tests[i].dxt10;

        memset(&info, 0, sizeof(info));
        hr = D3DX11GetImageInfoFromMemory(&dds, file_size, NULL, &info, NULL);
        todo_wine_if(tests[i].todo_hr) ok(hr == tests[i].expected.hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr) && SUCCEEDED(tests[i].expected.hr))
            check_image_info_values(&info, tests[i].expected.width, tests[i].expected.height,
                    tests[i].expected.depth, tests[i].expected.array_size, tests[i].expected.mip_levels,
                    tests[i].expected.misc_flags, tests[i].expected.format,
                    tests[i].expected.resource_dimension, D3DX11_IFF_DDS, tests[i].todo_info);

        winetest_pop_context();
    }

    /*
     * Image size (e.g, the size of the pixels) isn't validated, while header
     * size is. Even the latter sporadically crashes on native though.
     */
    if (0)
    {
        dds.magic = MAKEFOURCC('D','D','S',' ');
        set_dxt10_dds_header(&dds.header, tests[0].append_flags, tests[0].width, tests[0].height,
                             tests[0].depth, tests[0].mip_levels, tests[0].row_pitch, tests[0].caps, tests[0].caps2);
        dds.dxt10 = tests[0].dxt10;

        hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds) - 1, NULL, &info, NULL);
        ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

        hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    }
}

static void test_D3DX11GetImageInfoFromMemory(void)
{
    D3DX11_IMAGE_INFO info;
    HRESULT hr;

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp) + 5, NULL, &info, NULL); /* too large size */
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, sizeof(noimage), NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, sizeof(noimage), NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp) - 1, NULL, &info, NULL);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp + 1, sizeof(bmp_1bpp) - 1, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 4, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 4, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* test BMP support */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    hr = D3DX11GetImageInfoFromMemory(bmp_2bpp, sizeof(bmp_2bpp), NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_4bpp, sizeof(bmp_4bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_8bpp, sizeof(bmp_8bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_32bpp_xrgb, sizeof(bmp_32bpp_xrgb), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_32bpp_argb, sizeof(bmp_32bpp_argb), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    /* Grayscale PNG */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(png_grayscale, sizeof(png_grayscale), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_PNG, FALSE);

    /* test DDS support */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_24bit, sizeof(dds_24bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 2, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_24bit, sizeof(dds_24bit) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 2, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_16bit, sizeof(dds_16bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_16bit, sizeof(dds_16bit) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_8bit, sizeof(dds_8bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 16, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_cube_map, sizeof(dds_cube_map), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC3_UNORM,
            D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_cube_map, sizeof(dds_cube_map) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC3_UNORM,
            D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_volume_map, sizeof(dds_volume_map), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 2, 1, 3, 0, DXGI_FORMAT_BC2_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D,
            D3DX11_IFF_DDS, FALSE);

    hr = D3DX11GetImageInfoFromMemory(dds_volume_map, sizeof(dds_volume_map) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 2, 1, 3, 0, DXGI_FORMAT_BC2_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D,
            D3DX11_IFF_DDS, FALSE);

    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC1_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0, DXGI_FORMAT_R8G8_B8G8_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0, DXGI_FORMAT_G8R8_G8B8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff, DXGI_FORMAT_A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, DXGI_FORMAT_R16G16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 8, 0xff, 0, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 16, 0xffff, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x0f, 0, 0, 0xf0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_INDEXED, 0, 8, 0, 0, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_INDEXED | DDS_PF_ALPHA, 0, 16, 0, 0, 0, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, 0x24, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_UNORM); /* D3DFMT_A16B16G16R16 */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x6e, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_SNORM); /* D3DFMT_Q16W16V16U16 */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x6f, 0, 0, 0, 0, 0, DXGI_FORMAT_R16_FLOAT); /* D3DFMT_R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x70, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16_FLOAT); /* D3DFMT_G16R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x71, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_FLOAT); /* D3DFMT_A16B16G16R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x72, 0, 0, 0, 0, 0, DXGI_FORMAT_R32_FLOAT); /* D3DFMT_R32F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x73, 0, 0, 0, 0, 0, DXGI_FORMAT_R32G32_FLOAT); /* D3DFMT_G32R32F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x74, 0, 0, 0, 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT); /* D3DFMT_A32B32G32R32F */

    /* Test for DDS pixel formats that are valid on d3dx9, but not d3dx10+. */
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, 0x75, 0, 0, 0, 0, 0, E_FAIL); /* D3DFMT_CxV8U8 */
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, MAKEFOURCC('U','Y','V','Y'), 0, 0, 0, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0, E_FAIL);
    /* Bumpmap formats aren't supported. */
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 32, 0xff, 0xff00, 0x00ff0000, 0xff000000, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPLUMINANCE, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, E_FAIL);

    /* Newer fourCC formats. */
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC4_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC5_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC4_SNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC5_SNORM);

    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B5G6R5_UNORM, E_FAIL);
    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B5G5R5A1_UNORM, E_FAIL);
    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B4G4R4A4_UNORM, E_FAIL);

    /*
     * These formats should map 1:1 from the DXT10 header, unlike legacy DDS
     * file equivalents.
     */
    check_dds_dxt10_format(DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE);
    /* Formats unsupported on d3dx10, but now supported on d3dx11. */
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_UF16, FALSE);
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC6H_SF16, DXGI_FORMAT_BC6H_SF16, FALSE);
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM, FALSE);
}

START_TEST(d3dx11)
{
    test_D3DX11CreateAsyncMemoryLoader();
    test_D3DX11CreateAsyncFileLoader();
    test_D3DX11CreateAsyncResourceLoader();
    test_D3DX11CompileFromFile();
    test_D3DX11GetImageInfoFromMemory();
    test_legacy_dds_header_image_info();
    test_dxt10_dds_header_image_info();
}
