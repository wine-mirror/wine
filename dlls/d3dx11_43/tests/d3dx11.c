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
#include "wine/wined3d.h"
#include "wine/test.h"
#include <stdint.h>

static const D3DX11_IMAGE_LOAD_INFO d3dx11_default_load_info =
{
    D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,
    D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, NULL
};

#define D3DERR_INVALIDCALL 0x8876086c

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

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

static bool wined3d_opengl;

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
    uint32_t dxgi_format;
    uint32_t resource_dimension;
    uint32_t misc_flag;
    uint32_t array_size;
    uint32_t misc_flags2;
};

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

/* 1x1 1bpp bmp image */
static const uint8_t test_bmp_1bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};
static const uint8_t test_bmp_1bpp_data[] =
{
    0xf3,0xf2,0xf1,0xff
};

/* 1x1 2bpp bmp image */
static const uint8_t test_bmp_2bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 4bpp bmp image */
static const uint8_t test_bmp_4bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};
static const uint8_t test_bmp_4bpp_data[] =
{
    0xf3,0xf2,0xf1,0xff
};

/* 1x1 8bpp bmp image */
static const uint8_t test_bmp_8bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};
static const uint8_t test_bmp_8bpp_data[] =
{
    0xf3,0xf2,0xf1,0xff
};

/* 1x1 16bpp bmp image */
static const uint8_t test_bmp_16bpp[] =
{
    0x42,0x4d,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x10,0x00,0x00,0x00,
    0x00,0x00,0x06,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x42,0x00,0x00,0x00,0x00
};
static const uint8_t test_bmp_16bpp_data[] =
{
    0x84,0x84,0x73,0xff
};

/* 1x1 24bpp bmp image */
static const uint8_t test_bmp_24bpp[] =
{
    0x42,0x4d,0x3c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x18,0x00,0x00,0x00,
    0x00,0x00,0x06,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x73,0x84,0x84,0x00,0x00,0x00
};
static const uint8_t test_bmp_24bpp_data[] =
{
    0x84,0x84,0x73,0xff
};

/* 2x2 32bpp XRGB bmp image */
static const uint8_t test_bmp_32bpp_xrgb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x00
};
static const uint8_t test_bmp_32bpp_xrgb_data[] =
{
    0xc2,0xb2,0xa2,0xff,0xc3,0xb3,0xa3,0xff,0xc0,0xb0,0xa0,0xff,0xc1,0xb1,0xa1,0xff
};

/* 2x2 32bpp ARGB bmp image */
static const uint8_t test_bmp_32bpp_argb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x01
};
static const uint8_t test_bmp_32bpp_argb_data[] =
{
    0xc2,0xb2,0xa2,0xff,0xc3,0xb3,0xa3,0xff,0xc0,0xb0,0xa0,0xff,0xc1,0xb1,0xa1,0xff
};

/* 1x1 8bpp gray png image */
static const uint8_t test_png_8bpp_gray[] =
{
    0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,
    0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x00,0x00,0x00,0x00,0x3a,0x7e,0x9b,
    0x55,0x00,0x00,0x00,0x0a,0x49,0x44,0x41,0x54,0x08,0xd7,0x63,0xf8,0x0f,0x00,0x01,
    0x01,0x01,0x00,0x1b,0xb6,0xee,0x56,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,
    0x42,0x60,0x82
};
static const uint8_t test_png_8bpp_gray_data[] =
{
    0xff,0xff,0xff,0xff
};

/* 1x1 jpg image */
static const uint8_t test_jpg[] =
{
    0xff,0xd8,0xff,0xe0,0x00,0x10,0x4a,0x46,0x49,0x46,0x00,0x01,0x01,0x01,0x01,0x2c,
    0x01,0x2c,0x00,0x00,0xff,0xdb,0x00,0x43,0x00,0x05,0x03,0x04,0x04,0x04,0x03,0x05,
    0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x07,0x0c,0x08,0x07,0x07,0x07,0x07,0x0f,0x0b,
    0x0b,0x09,0x0c,0x11,0x0f,0x12,0x12,0x11,0x0f,0x11,0x11,0x13,0x16,0x1c,0x17,0x13,
    0x14,0x1a,0x15,0x11,0x11,0x18,0x21,0x18,0x1a,0x1d,0x1d,0x1f,0x1f,0x1f,0x13,0x17,
    0x22,0x24,0x22,0x1e,0x24,0x1c,0x1e,0x1f,0x1e,0xff,0xdb,0x00,0x43,0x01,0x05,0x05,
    0x05,0x07,0x06,0x07,0x0e,0x08,0x08,0x0e,0x1e,0x14,0x11,0x14,0x1e,0x1e,0x1e,0x1e,
    0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
    0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,
    0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0xff,0xc0,
    0x00,0x11,0x08,0x00,0x01,0x00,0x01,0x03,0x01,0x22,0x00,0x02,0x11,0x01,0x03,0x11,
    0x01,0xff,0xc4,0x00,0x15,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0xc4,0x00,0x14,0x10,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xc4,
    0x00,0x14,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xff,0xc4,0x00,0x14,0x11,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xda,0x00,0x0c,0x03,0x01,
    0x00,0x02,0x11,0x03,0x11,0x00,0x3f,0x00,0xb2,0xc0,0x07,0xff,0xd9
};
static const uint8_t test_jpg_data[] =
{
    0xff,0xff,0xff,0xff
};

/* 1x1 gif image */
static const uint8_t test_gif[] =
{
    0x47,0x49,0x46,0x38,0x37,0x61,0x01,0x00,0x01,0x00,0x80,0x00,0x00,0xff,0xff,0xff,
    0xff,0xff,0xff,0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x02,0x02,0x44,
    0x01,0x00,0x3b
};
static const uint8_t test_gif_data[] =
{
    0xff,0xff,0xff,0xff
};

/* 1x1 tiff image */
static const uint8_t test_tiff[] =
{
    0x49,0x49,0x2a,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0xfe,0x00,
    0x04,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x01,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
    0x00,0x00,0x02,0x01,0x03,0x00,0x03,0x00,0x00,0x00,0xd2,0x00,0x00,0x00,0x03,0x01,
    0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x06,0x01,0x03,0x00,0x01,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x0d,0x01,0x02,0x00,0x1b,0x00,0x00,0x00,0xd8,0x00,
    0x00,0x00,0x11,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x12,0x01,
    0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x15,0x01,0x03,0x00,0x01,0x00,
    0x00,0x00,0x03,0x00,0x00,0x00,0x16,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x40,0x00,
    0x00,0x00,0x17,0x01,0x04,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x1a,0x01,
    0x05,0x00,0x01,0x00,0x00,0x00,0xf4,0x00,0x00,0x00,0x1b,0x01,0x05,0x00,0x01,0x00,
    0x00,0x00,0xfc,0x00,0x00,0x00,0x1c,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
    0x00,0x00,0x28,0x01,0x03,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x08,0x00,0x08,0x00,0x08,0x00,0x2f,0x68,0x6f,0x6d,0x65,0x2f,0x6d,0x65,
    0x68,0x2f,0x44,0x65,0x73,0x6b,0x74,0x6f,0x70,0x2f,0x74,0x65,0x73,0x74,0x2e,0x74,
    0x69,0x66,0x00,0x00,0x00,0x00,0x00,0x48,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x48,
    0x00,0x00,0x00,0x01
};
static const uint8_t test_tiff_data[] =
{
    0x00,0x00,0x00,0xff
};

/* 1x1 alpha dds image */
static const uint8_t test_dds_alpha[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff
};
static const uint8_t test_dds_alpha_data[] =
{
    0xff
};

/* 1x1 luminance dds image */
static const uint8_t test_dds_luminance[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x82
};
static const uint8_t test_dds_luminance_data[] =
{
    0x82,0x82,0x82,0xff
};

/* 1x1 16bpp dds image */
static const uint8_t test_dds_16bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,
    0xe0,0x03,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0e,0x42
};
static const uint8_t test_dds_16bpp_data[] =
{
    0x84,0x84,0x73,0xff
};

/* 1x1 24bpp dds image */
static const uint8_t test_dds_24bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x70,0x81,0x83
};
static const uint8_t test_dds_24bpp_data[] =
{
    0x83,0x81,0x70,0xff
};

/* 1x1 32bpp dds image */
static const uint8_t test_dds_32bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x70,0x81,0x83,0xff
};
static const uint8_t test_dds_32bpp_data[] =
{
    0x83,0x81,0x70,0xff
};

/* 1x1 64bpp dds image */
static const uint8_t test_dds_64bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x02,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x83,0x83,0x81,0x81,0x70,0x70,0xff,0xff
};
static const uint8_t test_dds_64bpp_data[] =
{
    0x83,0x83,0x81,0x81,0x70,0x70,0xff,0xff
};

/* 1x1 96bpp dds image */
static const uint8_t test_dds_96bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x02,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x31,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x06,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x84,0x83,0x03,0x3f,0x82,0x81,0x01,0x3f,0xe2,0xe0,0xe0,0x3e
};
static const uint8_t test_dds_96bpp_data[] =
{
    0x84,0x83,0x03,0x3f,0x82,0x81,0x01,0x3f,0xe2,0xe0,0xe0,0x3e
};

/* 1x1 128bpp dds image */
static const uint8_t test_dds_128bpp[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x02,0x00,0x01,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x74,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x84,0x83,0x03,0x3f,0x82,0x81,0x01,0x3f,0xe2,0xe0,0xe0,0x3e,0x00,0x00,0x80,0x3f
};
static const uint8_t test_dds_128bpp_data[] =
{
    0x84,0x83,0x03,0x3f,0x82,0x81,0x01,0x3f,0xe2,0xe0,0xe0,0x3e,0x00,0x00,0x80,0x3f
};

/* 4x4 DXT1 dds image */
static const uint8_t test_dds_dxt1[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x2a,0x31,0xf5,0xbc,0xe3,0x6e,0x2a,0x3a
};
static const uint8_t test_dds_dxt1_data[] =
{
    0x2a,0x31,0xf5,0xbc,0xe3,0x6e,0x2a,0x3a
};

/* 4x8 DXT1 dds image */
static const uint8_t test_dds_dxt1_4x8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x08,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x92,0xce,0x09,0x7a,0x5d,0xdd,0xa7,0x26,0x55,0xde,0xaf,0x52,0xbc,0xf8,0x6c,0x44,
    0x53,0xbd,0x8b,0x72,0x55,0x33,0x88,0xaa,0xb2,0x9c,0x6c,0x93,0x55,0x00,0x55,0x00,
    0x0f,0x9c,0x0f,0x9c,0x00,0x00,0x00,0x00,
};
static const uint8_t test_dds_dxt1_4x8_data[] =
{
    0x92,0xce,0x09,0x7a,0x5d,0xdd,0xa7,0x26,0x55,0xde,0xaf,0x52,0xbc,0xf8,0x6c,0x44,
};

/* 4x4 DXT2 dds image */
static const uint8_t test_dds_dxt2[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x02,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0xde,0xc4,0x10,0x2f,0xbf,0xff,0x7b,
    0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x57,0x53,0x00,0x00,0x52,0x52,0x55,0x55,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xce,0x59,0x00,0x00,0x54,0x55,0x55,0x55
};
static const uint8_t test_dds_dxt2_data[] =
{
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfd,0xde,0xc4,0x10,0x2f,0xbf,0xff,0x7b
};

/* 1x3 DXT3 dds image */
static const uint8_t test_dds_dxt3[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x03,0x00,0x00,0x00,
    0x01,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0c,0x92,0x38,0x84,0x00,0xff,0x55,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x53,0x8b,0x53,0x8b,0x00,0x00,0x00,0x00
};
static const uint8_t test_dds_dxt3_data[] =
{
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x4e,0x92,0xd6,0x83,0x00,0xaa,0x55,0x55
};

/* 4x4 DXT4 dds image */
static const uint8_t test_dds_dxt4[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x02,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x34,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xfd,0xde,0xc4,0x10,0x2f,0xbf,0xff,0x7b,
    0xff,0x00,0x40,0x02,0x24,0x49,0x92,0x24,0x57,0x53,0x00,0x00,0x52,0x52,0x55,0x55,
    0xff,0x00,0x48,0x92,0x24,0x49,0x92,0x24,0xce,0x59,0x00,0x00,0x54,0x55,0x55,0x55
};
static const uint8_t test_dds_dxt4_data[] =
{
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xfd,0xde,0xc4,0x10,0x2f,0xbf,0xff,0x7b
};

/* 4x2 DXT5 dds image */
static const uint8_t test_dds_dxt5[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50
};
static const uint8_t test_dds_dxt5_data[] =
{
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x05,0x05
};

/* 8x8 DXT5 dds image */
static const uint8_t test_dds_dxt5_8x8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x08,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x4b,0x8a,0x72,0x39,0x5e,0x5e,0xfa,0xa8,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1c,0xd7,0xd5,0x4a,0x2d,0x2d,0xad,0xfd,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x47,0x9a,0x73,0x83,0xa0,0xf0,0x78,0x78,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x5b,0x06,0x19,0x00,0xe8,0x78,0x58,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x39,0xbe,0x8c,0x49,0x35,0xb5,0xff,0x7f,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x96,0x84,0xab,0x59,0x11,0xff,0x11,0xff,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x6a,0xf0,0x6a,0x00,0x00,0x00,0x00,
};
static const uint8_t test_dds_dxt5_8x8_data[] =
{
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x4b,0x8a,0x72,0x39,0x5e,0x5e,0xfa,0xa8,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1c,0xd7,0xd5,0x4a,0x2d,0x2d,0xad,0xfd,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x47,0x9a,0x73,0x83,0xa0,0xf0,0x78,0x78,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x5b,0x06,0x19,0x00,0xe8,0x78,0x58,
};

/* 4x4 BC4 dds image */
static const uint8_t test_dds_bc4[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x42,0x43,0x34,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xd9,0x15,0xbc,0x41,0x5b,0xa3,0x3d,0x3a,0x8f,0x3d,0x45,0x81,0x20,0x45,0x81,0x20,
    0x6f,0x6f,0x00,0x00,0x00,0x00,0x00,0x00
};
static const uint8_t test_dds_bc4_data[] =
{
    0xd9,0x15,0xbc,0x41,0x5b,0xa3,0x3d,0x3a
};

/* 6x3 BC5 dds image */
static const uint8_t test_dds_bc5[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x03,0x00,0x00,0x00,
    0x06,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x42,0x43,0x35,0x55,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x9f,0x28,0x73,0xac,0xd5,0x80,0xaa,0xd5,0x70,0x2c,0x4e,0xd6,0x76,0x1d,0xd6,0x76,
    0xd5,0x0f,0xc3,0x50,0x96,0xcf,0x53,0x96,0xdf,0x16,0xc3,0x50,0x96,0xcf,0x53,0x96,
    0x83,0x55,0x08,0x83,0x30,0x08,0x83,0x30,0x79,0x46,0x31,0x1c,0xc3,0x31,0x1c,0xc3,
    0x6d,0x6d,0x00,0x00,0x00,0x00,0x00,0x00,0x5c,0x5c,0x00,0x00,0x00,0x00,0x00,0x00
};
static const uint8_t test_dds_bc5_data[] =
{
    0x95,0x35,0xe2,0xa3,0xf5,0xd2,0x28,0x68,0x65,0x32,0x7c,0x4e,0xdb,0xe4,0x56,0x0a,
    0xb9,0x33,0xaf,0xf0,0x52,0xbe,0xed,0x27,0xb4,0x2e,0xa6,0x60,0x4e,0xb6,0x5d,0x3f
};

/* 4x4 DXT1 cube map */
static const uint8_t test_dds_cube[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x31,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,
    0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,
    0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00,
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,
    0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,
    0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00,
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,
    0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,
    0x32,0x96,0x0b,0x7b,0xcc,0x55,0xcc,0x55,0x0e,0x84,0x0e,0x84,0x00,0x00,0x00,0x00
};
static const uint8_t test_dds_cube_data[] =
{
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,
    0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7,0xf5,0xa7,0x08,0x69,0x74,0xc0,0xbf,0xd7
};

/* 4x4x2 DXT3 volume dds,2 mipmaps */
static const uint8_t test_dds_volume[] =
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
static const uint8_t test_dds_volume_data[] =
{
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
};

/*
 * 8x8 24-bit dds, 4 mipmaps. Level 0 is red, level 1 is green, level 2 is
 * blue, and level 3 is black.
 */
static const uint8_t dds_24bit_8_8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x08,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00
};

/* 4x4 cube map DDS file with 2 mips. */
static const uint8_t dds_cube_map_4_4[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x02,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0x00,0xff,0x00,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0xff,0x08,0x10,0x40,0x00,
    0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,
    0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,
    0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,
    0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,
    0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,
    0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,
    0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,
    0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,
    0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,
    0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,
    0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,
    0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,
    0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,
    0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,
    0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
    0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
    0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
    0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
    0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,
};

/* 1x1 wmp image */
static const uint8_t test_wmp[] =
{
    0x49,0x49,0xbc,0x01,0x20,0x00,0x00,0x00,0x24,0xc3,0xdd,0x6f,0x03,0x4e,0xfe,0x4b,
    0xb1,0x85,0x3d,0x77,0x76,0x8d,0xc9,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x08,0x00,0x01,0xbc,0x01,0x00,0x10,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x02,0xbc,
    0x04,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xbc,0x04,0x00,0x01,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x81,0xbc,0x04,0x00,0x01,0x00,0x00,0x00,0x01,0x00,
    0x00,0x00,0x82,0xbc,0x0b,0x00,0x01,0x00,0x00,0x00,0x25,0x06,0xc0,0x42,0x83,0xbc,
    0x0b,0x00,0x01,0x00,0x00,0x00,0x25,0x06,0xc0,0x42,0xc0,0xbc,0x04,0x00,0x01,0x00,
    0x00,0x00,0x86,0x00,0x00,0x00,0xc1,0xbc,0x04,0x00,0x01,0x00,0x00,0x00,0x92,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x57,0x4d,0x50,0x48,0x4f,0x54,0x4f,0x00,0x11,0x45,
    0xc0,0x71,0x00,0x00,0x00,0x00,0x60,0x00,0xc0,0x00,0x00,0x0c,0x00,0x00,0x00,0xc0,
    0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x08,0x00,0x25,0xff,0xff,0x00,0x00,0x01,
    0x01,0xc8,0xf0,0x00,0x00,0x00,0x00,0x01,0x02,0x04,0x10,0x10,0xa6,0x18,0x8c,0x21,
    0x00,0xc4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x03,0x4e,0x0f,0x3a,0x4c,0x94,0x9d,0xba,0x79,0xe7,0x38,
    0x4c,0xcf,0x14,0xc3,0x43,0x91,0x88,0xfb,0xdc,0xe0,0x7c,0x34,0x70,0x9b,0x28,0xa9,
    0x18,0x74,0x62,0x87,0x8e,0xe4,0x68,0x5f,0xb9,0xcc,0x0e,0xe1,0x8c,0x76,0x3a,0x9b,
    0x82,0x76,0x71,0x13,0xde,0x50,0xd4,0x2d,0xc2,0xda,0x1e,0x3b,0xa6,0xa1,0x62,0x7b,
    0xca,0x1a,0x85,0x4b,0x6e,0x74,0xec,0x60
};
static const uint8_t test_wmp_data[] =
{
    0xff,0xff,0xff,0xff
};

static const struct test_image
{
    const uint8_t *data;
    unsigned int size;
    const uint8_t *expected_data;
    D3DX11_IMAGE_INFO expected_info;
}
test_image[] =
{
    {
        test_bmp_1bpp,       sizeof(test_bmp_1bpp),          test_bmp_1bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_4bpp,       sizeof(test_bmp_4bpp),          test_bmp_4bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_8bpp,       sizeof(test_bmp_8bpp),          test_bmp_8bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_16bpp,      sizeof(test_bmp_16bpp),         test_bmp_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_24bpp,      sizeof(test_bmp_24bpp),         test_bmp_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_32bpp_xrgb, sizeof(test_bmp_32bpp_xrgb),    test_bmp_32bpp_xrgb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_bmp_32bpp_argb, sizeof(test_bmp_32bpp_argb),    test_bmp_32bpp_argb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP}
    },
    {
        test_png_8bpp_gray,  sizeof(test_png_8bpp_gray),     test_png_8bpp_gray_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_PNG}
    },
    {
        test_jpg,            sizeof(test_jpg),               test_jpg_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_JPG}
    },
    {
        test_gif,            sizeof(test_gif),               test_gif_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_GIF}
    },
    {
        test_tiff,           sizeof(test_tiff),              test_tiff_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_TIFF}
    },
    {
        test_dds_alpha,      sizeof(test_dds_alpha),         test_dds_alpha_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_A8_UNORM,           D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_luminance,  sizeof(test_dds_luminance),     test_dds_luminance_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_16bpp,      sizeof(test_dds_16bpp),         test_dds_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_24bpp,      sizeof(test_dds_24bpp),         test_dds_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_32bpp,      sizeof(test_dds_32bpp),         test_dds_32bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_64bpp,      sizeof(test_dds_64bpp),         test_dds_64bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R16G16B16A16_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_96bpp,      sizeof(test_dds_96bpp),         test_dds_96bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32_FLOAT,    D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_128bpp,     sizeof(test_dds_128bpp),        test_dds_128bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt1,       sizeof(test_dds_dxt1),          test_dds_dxt1_data,
        {4, 4, 1, 1, 1, 0,   DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt1_4x8,   sizeof(test_dds_dxt1_4x8),      test_dds_dxt1_4x8_data,
        {4, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt2,       sizeof(test_dds_dxt2),          test_dds_dxt2_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt3,       sizeof(test_dds_dxt3),          test_dds_dxt3_data,
        {1, 3, 1, 1, 2, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt4,       sizeof(test_dds_dxt4),          test_dds_dxt4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt5,       sizeof(test_dds_dxt5),          test_dds_dxt5_data,
        {4, 2, 1, 1, 1, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_dxt5_8x8,   sizeof(test_dds_dxt5_8x8),      test_dds_dxt5_8x8_data,
        {8, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_bc4,        sizeof(test_dds_bc4),           test_dds_bc4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC4_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_bc5,        sizeof(test_dds_bc5),           test_dds_bc5_data,
        {6, 3, 1, 1, 3, 0,   DXGI_FORMAT_BC5_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_cube,       sizeof(test_dds_cube),          test_dds_cube_data,
        {4, 4, 1, 6, 3, 0x4, DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS}
    },
    {
        test_dds_volume,     sizeof(test_dds_volume),        test_dds_volume_data,
        {4, 4, 2, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE3D, D3DX11_IFF_DDS}
    },
    {
        test_wmp,            sizeof(test_wmp),               test_wmp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_WMP}
    },
};

static const struct test_image_load_info
{
    const uint8_t *data;
    uint32_t size;
    D3DX11_IMAGE_LOAD_INFO load_info;
    HRESULT expected_hr;

    D3D11_RESOURCE_DIMENSION expected_type;
    union
    {
        D3D11_TEXTURE2D_DESC desc_2d;
        D3D11_TEXTURE3D_DESC desc_3d;
    } expected_resource_desc;
    BOOL todo_resource_desc;
} test_image_load_info[] =
{
    /*
     * Autogen mips misc flag specified. In the case of a cube texture image,
     * the autogen mips flag is OR'd against D3D11_RESOURCE_MISC_TEXTURECUBE,
     * even if it isn't specified in the load info structure.
     */
    {
        dds_cube_map_4_4, sizeof(dds_cube_map_4_4),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
            (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET),        D3DX11_DEFAULT, D3D11_RESOURCE_MISC_GENERATE_MIPS,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        S_OK, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
        {
            .desc_2d =
            {
                4, 4, 3, 6, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT,
                (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET), 0,
                (D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE)
            }
        }
    },
    /*
     * Pass in different dimensions and texture format than the source image.
     */
    {
        test_dds_dxt1, sizeof(test_dds_dxt1),
        {
            8,              8,              D3DX11_DEFAULT, D3DX11_DEFAULT,             1,  (D3D11_USAGE)D3DX11_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, DXGI_FORMAT_R8G8B8A8_UNORM, D3DX11_DEFAULT,
            D3DX11_DEFAULT
        },
        S_OK, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
        {
            .desc_2d =
            {
                8, 8, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0
            }
        }
    },
};

static const struct test_invalid_image_load_info
{
    const uint8_t *data;
    uint32_t size;
    D3DX11_IMAGE_LOAD_INFO load_info;
    HRESULT expected_hr;
    HRESULT expected_process_hr;
    HRESULT expected_create_device_object_hr;
    BOOL todo_hr;
    BOOL todo_process_hr;
    BOOL todo_create_device_object_hr;
} test_invalid_image_load_info[] =
{
    /*
     * A depth value that isn't D3DX11_FROM_FILE/D3DX11_DEFAULT/0 on a 2D
     * texture results in failure.
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, 2,              D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        E_FAIL, E_FAIL
    },
    /* Invalid filter value. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7,              D3DX11_DEFAULT
        },
        D3DERR_INVALIDCALL, D3DERR_INVALIDCALL
    },
    /* Invalid mipfilter value, only validated if mips are generated. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7
        },
        S_OK, S_OK, S_OK
    },
    /* Invalid mipfilter value. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            2,              2,              D3DX11_DEFAULT, D3DX11_DEFAULT, 2,              (D3D11_USAGE)D3DX11_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7
        },
        D3DERR_INVALIDCALL, D3DERR_INVALIDCALL
    },
    /*
     * Usage/BindFlags/CpuAccessFlags are validated in the call to
     * CreateDeviceObject().
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_CPU_ACCESS_READ, D3D11_USAGE_DYNAMIC,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        E_INVALIDARG, S_OK, E_INVALIDARG
    },
    /* 5. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT,           D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
            D3D11_BIND_DEPTH_STENCIL, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        E_INVALIDARG, S_OK, E_INVALIDARG
    },
    /*
     * D3D11_RESOURCE_MISC_GENERATE_MIPS requires binding as a shader resource
     * and a render target.
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,    D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_RESOURCE_MISC_GENERATE_MIPS, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        E_INVALIDARG, S_OK, E_INVALIDARG
    },
    /* Can't set the cube texture flag if the image isn't a cube texture. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        {
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,  D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
            D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_RESOURCE_MISC_TEXTURECUBE, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT
        },
        E_INVALIDARG, S_OK, E_INVALIDARG
    },
};

static WCHAR temp_dir[MAX_PATH];

static DXGI_FORMAT block_compressed_formats[] =
{
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static BOOL is_block_compressed(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(block_compressed_formats); ++i)
        if (format == block_compressed_formats[i])
            return TRUE;

    return FALSE;
}

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

static HRESULT WINAPI D3DX11ThreadPump_QueryInterface(ID3DX11ThreadPump *iface, REFIID riid, void **out)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI D3DX11ThreadPump_AddRef(ID3DX11ThreadPump *iface)
{
    return 2;
}

static ULONG WINAPI D3DX11ThreadPump_Release(ID3DX11ThreadPump *iface)
{
    return 1;
}

static int add_work_item_count = 1;

static HRESULT WINAPI D3DX11ThreadPump_AddWorkItem(ID3DX11ThreadPump *iface, ID3DX11DataLoader *loader,
        ID3DX11DataProcessor *processor, HRESULT *result, void **object)
{
    SIZE_T size;
    void *data;
    HRESULT hr;

    ok(!add_work_item_count++, "unexpected call\n");

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataLoader_Decompress(loader, &data, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(processor, data, size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_CreateDeviceObject(processor, object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Destroy(processor);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (result) *result = S_OK;
    return S_OK;
}

static UINT WINAPI D3DX11ThreadPump_GetWorkItemCount(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return 0;
}

static HRESULT WINAPI D3DX11ThreadPump_WaitForAllItems(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_ProcessDeviceWorkItems(ID3DX11ThreadPump *iface, UINT count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_PurgeAllItems(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_GetQueueStatus(ID3DX11ThreadPump *iface, UINT *queue,
        UINT *processqueue, UINT *devicequeue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ID3DX11ThreadPumpVtbl D3DX11ThreadPumpVtbl =
{
    D3DX11ThreadPump_QueryInterface,
    D3DX11ThreadPump_AddRef,
    D3DX11ThreadPump_Release,
    D3DX11ThreadPump_AddWorkItem,
    D3DX11ThreadPump_GetWorkItemCount,
    D3DX11ThreadPump_WaitForAllItems,
    D3DX11ThreadPump_ProcessDeviceWorkItems,
    D3DX11ThreadPump_PurgeAllItems,
    D3DX11ThreadPump_GetQueueStatus
};
static ID3DX11ThreadPump thread_pump = { &D3DX11ThreadPumpVtbl };

static char *get_str_a(const WCHAR *wstr)
{
    static char buffer[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, sizeof(buffer), NULL, NULL);
    return buffer;
}

static BOOL create_file(const WCHAR *filename, const void *data, unsigned int size, WCHAR *out_path)
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

static BOOL delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    return DeleteFileW(path);
}

static ID3D11Device *create_device(void)
{
    HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *,
                                         UINT, UINT, ID3D11Device **,  D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
    HMODULE d3d11_mod = LoadLibraryA("d3d11.dll");
    ID3D11Device *device;


    if (!d3d11_mod)
    {
        win_skip("d3d11.dll not present\n");
        return NULL;
    }

    pD3D11CreateDevice = (void *)GetProcAddress(d3d11_mod, "D3D11CreateDevice");
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static HMODULE create_resource_module(const WCHAR *filename, const void *data, unsigned int size)
{
    WCHAR resource_module_path[MAX_PATH], current_module_path[MAX_PATH];
    HANDLE resource;
    HMODULE module;
    BOOL ret;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(resource_module_path, temp_dir);
    lstrcatW(resource_module_path, filename);

    GetModuleFileNameW(NULL, current_module_path, ARRAY_SIZE(current_module_path));
    ret = CopyFileW(current_module_path, resource_module_path, FALSE);
    ok(ret, "CopyFileW failed, error %lu.\n", GetLastError());
    SetFileAttributesW(resource_module_path, FILE_ATTRIBUTE_NORMAL);

    resource = BeginUpdateResourceW(resource_module_path, TRUE);
    UpdateResourceW(resource, (LPCWSTR)RT_RCDATA, filename, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (void *)data, size);
    EndUpdateResourceW(resource, FALSE);

    module = LoadLibraryExW(resource_module_path, NULL, LOAD_LIBRARY_AS_DATAFILE);

    return module;
}

static void delete_resource_module(const WCHAR *filename, HMODULE module)
{
    WCHAR path[MAX_PATH];

    FreeLibrary(module);

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static void check_image_info(D3DX11_IMAGE_INFO *image_info, const struct test_image *image, unsigned int line)
{
    ok_(__FILE__, line)(image_info->Width == image->expected_info.Width,
            "Got unexpected Width %u, expected %u.\n",
            image_info->Width, image->expected_info.Width);
    ok_(__FILE__, line)(image_info->Height == image->expected_info.Height,
            "Got unexpected Height %u, expected %u.\n",
            image_info->Height, image->expected_info.Height);
    ok_(__FILE__, line)(image_info->Depth == image->expected_info.Depth,
            "Got unexpected Depth %u, expected %u.\n",
            image_info->Depth, image->expected_info.Depth);
    ok_(__FILE__, line)(image_info->ArraySize == image->expected_info.ArraySize,
            "Got unexpected ArraySize %u, expected %u.\n",
            image_info->ArraySize, image->expected_info.ArraySize);
    ok_(__FILE__, line)(image_info->MipLevels == image->expected_info.MipLevels,
            "Got unexpected MipLevels %u, expected %u.\n",
            image_info->MipLevels, image->expected_info.MipLevels);
    ok_(__FILE__, line)(image_info->MiscFlags == image->expected_info.MiscFlags,
            "Got unexpected MiscFlags %#x, expected %#x.\n",
            image_info->MiscFlags, image->expected_info.MiscFlags);
    ok_(__FILE__, line)(image_info->Format == image->expected_info.Format,
            "Got unexpected Format %#x, expected %#x.\n",
            image_info->Format, image->expected_info.Format);
    ok_(__FILE__, line)(image_info->ResourceDimension == image->expected_info.ResourceDimension,
            "Got unexpected ResourceDimension %u, expected %u.\n",
            image_info->ResourceDimension, image->expected_info.ResourceDimension);
    ok_(__FILE__, line)(image_info->ImageFileFormat == image->expected_info.ImageFileFormat,
            "Got unexpected ImageFileFormat %u, expected %u.\n",
            image_info->ImageFileFormat, image->expected_info.ImageFileFormat);
}

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

#define check_texture2d_desc(desc, expected, wine_todo) check_texture2d_desc_(__LINE__, desc, expected, wine_todo)
static inline void check_texture2d_desc_(uint32_t line, const D3D11_TEXTURE2D_DESC *desc, const D3D11_TEXTURE2D_DESC *expected,
        BOOL wine_todo)
{
    BOOL matched;

    matched = !memcmp(expected, desc, sizeof(*desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected 2D texture desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc->Width != expected->Width)
        ok_(__FILE__, line)(desc->Width == expected->Width, "Expected width %u, got %u.\n", expected->Width,
                desc->Width);
    todo_wine_if(wine_todo && desc->Height != expected->Height)
        ok_(__FILE__, line)(desc->Height == expected->Height, "Expected height %u, got %u.\n", expected->Height,
                desc->Height);
    todo_wine_if(wine_todo && desc->ArraySize != expected->ArraySize)
        ok_(__FILE__, line)(desc->ArraySize == expected->ArraySize, "Expected array size %u, got %u.\n",
                expected->ArraySize, desc->ArraySize);
    todo_wine_if(wine_todo && desc->MipLevels != expected->MipLevels)
        ok_(__FILE__, line)(desc->MipLevels == expected->MipLevels, "Expected mip levels %u, got %u.\n",
                expected->MipLevels, desc->MipLevels);
    todo_wine_if(wine_todo && desc->Format != expected->Format)
        ok_(__FILE__, line)(desc->Format == expected->Format, "Expected texture format %#x, got %#x.\n",
                expected->Format, desc->Format);
    todo_wine_if(wine_todo && desc->SampleDesc.Count != expected->SampleDesc.Count)
        ok_(__FILE__, line)(desc->SampleDesc.Count == expected->SampleDesc.Count, "Expected sample count %u, got %u.\n",
                expected->SampleDesc.Count, desc->SampleDesc.Count);
    todo_wine_if(wine_todo && desc->SampleDesc.Quality != expected->SampleDesc.Quality)
        ok_(__FILE__, line)(desc->SampleDesc.Quality == expected->SampleDesc.Quality,
                "Expected sample quality %u, got %u.\n", expected->SampleDesc.Quality, desc->SampleDesc.Quality);
    todo_wine_if(wine_todo && desc->Usage != expected->Usage)
        ok_(__FILE__, line)(desc->Usage == expected->Usage, "Expected usage %u, got %u.\n", expected->Usage,
                desc->Usage);
    todo_wine_if(wine_todo && desc->BindFlags != expected->BindFlags)
        ok_(__FILE__, line)(desc->BindFlags == expected->BindFlags, "Expected bind flags %#x, got %#x.\n",
                expected->BindFlags, desc->BindFlags);
    todo_wine_if(wine_todo && desc->CPUAccessFlags != expected->CPUAccessFlags)
        ok_(__FILE__, line)(desc->CPUAccessFlags == expected->CPUAccessFlags,
                "Expected CPU access flags %#x, got %#x.\n", expected->CPUAccessFlags, desc->CPUAccessFlags);
    todo_wine_if(wine_todo && desc->MiscFlags != expected->MiscFlags)
        ok_(__FILE__, line)(desc->MiscFlags == expected->MiscFlags, "Expected misc flags %#x, got %#x.\n",
                expected->MiscFlags, desc->MiscFlags);
}

#define check_texture2d_desc_values(desc, width, height, mip_levels, array_size, format, sample_count, sample_quality, \
                                usage, bind_flags, cpu_access_flags, misc_flags, wine_todo) \
    check_texture2d_desc_values_(__LINE__, desc, width, height, mip_levels, array_size, format, sample_count, sample_quality, \
                                usage, bind_flags, cpu_access_flags, misc_flags, wine_todo)
static inline void check_texture2d_desc_values_(uint32_t line, const D3D11_TEXTURE2D_DESC *desc, uint32_t width,
        uint32_t height, uint32_t mip_levels, uint32_t array_size, DXGI_FORMAT format, uint32_t sample_count,
        uint32_t sample_quality, D3D11_USAGE usage, uint32_t bind_flags, uint32_t cpu_access_flags, uint32_t misc_flags,
        BOOL wine_todo)
{
    const D3D11_TEXTURE2D_DESC expected_desc = { width, height, mip_levels, array_size, format, { sample_count, sample_quality },
                                                 usage, bind_flags, cpu_access_flags, misc_flags };

    check_texture2d_desc_(line, desc, &expected_desc, wine_todo);
}

#define check_texture3d_desc(desc, expected, wine_todo) check_texture3d_desc_(__LINE__, desc, expected, wine_todo)
static inline void check_texture3d_desc_(uint32_t line, const D3D11_TEXTURE3D_DESC *desc,
        const D3D11_TEXTURE3D_DESC *expected, BOOL wine_todo)
{
    BOOL matched;

    matched = !memcmp(expected, desc, sizeof(*desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected 3D texture desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc->Width != expected->Width)
        ok_(__FILE__, line)(desc->Width == expected->Width, "Expected width %u, got %u.\n", expected->Width,
                desc->Width);
    todo_wine_if(wine_todo && desc->Height != expected->Height)
        ok_(__FILE__, line)(desc->Height == expected->Height, "Expected height %u, got %u.\n", expected->Height,
                desc->Height);
    todo_wine_if(wine_todo && desc->Depth != expected->Depth)
        ok_(__FILE__, line)(desc->Depth == expected->Depth, "Expected depth %u, got %u.\n", expected->Depth,
                desc->Depth);
    todo_wine_if(wine_todo && desc->MipLevels != expected->MipLevels)
        ok_(__FILE__, line)(desc->MipLevels == expected->MipLevels, "Expected mip levels %u, got %u.\n",
                expected->MipLevels, desc->MipLevels);
    todo_wine_if(wine_todo && desc->Format != expected->Format)
        ok_(__FILE__, line)(desc->Format == expected->Format, "Expected texture format %#x, got %#x.\n",
                expected->Format, desc->Format);
    todo_wine_if(wine_todo && desc->Usage != expected->Usage)
        ok_(__FILE__, line)(desc->Usage == expected->Usage, "Expected usage %u, got %u.\n", expected->Usage,
                desc->Usage);
    todo_wine_if(wine_todo && desc->BindFlags != expected->BindFlags)
        ok_(__FILE__, line)(desc->BindFlags == expected->BindFlags, "Expected bind flags %#x, got %#x.\n",
                expected->BindFlags, desc->BindFlags);
    todo_wine_if(wine_todo && desc->CPUAccessFlags != expected->CPUAccessFlags)
        ok_(__FILE__, line)(desc->CPUAccessFlags == expected->CPUAccessFlags,
                "Expected CPU access flags %#x, got %#x.\n", expected->CPUAccessFlags, desc->CPUAccessFlags);
    todo_wine_if(wine_todo && desc->MiscFlags != expected->MiscFlags)
        ok_(__FILE__, line)(desc->MiscFlags == expected->MiscFlags, "Expected misc flags %#x, got %#x.\n",
                expected->MiscFlags, desc->MiscFlags);
}

/*
 * Taken from the d3d11 tests. If there's a missing resource type or
 * texture format checking function, check to see if it exists there first.
 */
struct resource_readback
{
    ID3D11Resource *resource;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *immediate_context;
    unsigned int width, height, depth, sub_resource_idx;
};

static void init_resource_readback(ID3D11Resource *resource, ID3D11Resource *readback_resource,
        unsigned int width, unsigned int height, unsigned int depth, unsigned int sub_resource_idx,
        ID3D11Device *device, struct resource_readback *rb)
{
    HRESULT hr;

    rb->resource = readback_resource;
    rb->width = width;
    rb->height = height;
    rb->depth = depth;
    rb->sub_resource_idx = sub_resource_idx;

    ID3D11Device_GetImmediateContext(device, &rb->immediate_context);

    ID3D11DeviceContext_CopyResource(rb->immediate_context, rb->resource, resource);
    if (FAILED(hr = ID3D11DeviceContext_Map(rb->immediate_context,
            rb->resource, sub_resource_idx, D3D11_MAP_READ, 0, &rb->map_desc)))
    {
        trace("Failed to map resource, hr %#lx.\n", hr);
        ID3D11Resource_Release(rb->resource);
        rb->resource = NULL;
        ID3D11DeviceContext_Release(rb->immediate_context);
        rb->immediate_context = NULL;
    }
}

static void get_texture_readback(ID3D11Texture2D *texture, unsigned int sub_resource_idx,
        struct resource_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11Resource *rb_texture;
    unsigned int miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture2D_GetDevice(texture, &device);

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb_texture)))
    {
        trace("Failed to create texture, hr %#lx.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    init_resource_readback((ID3D11Resource *)texture, rb_texture,
            max(1, texture_desc.Width >> miplevel),
            max(1, texture_desc.Height >> miplevel),
            1, sub_resource_idx, device, rb);

    ID3D11Device_Release(device);
}

static void get_texture3d_readback(ID3D11Texture3D *texture, unsigned int sub_resource_idx,
        struct resource_readback *rb)
{
    D3D11_TEXTURE3D_DESC texture_desc;
    ID3D11Resource *rb_texture;
    unsigned int miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture3D_GetDevice(texture, &device);

    ID3D11Texture3D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture3D(device, &texture_desc, NULL, (ID3D11Texture3D **)&rb_texture)))
    {
        trace("Failed to create texture, hr %#lx.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    init_resource_readback((ID3D11Resource *)texture, rb_texture,
            max(1, texture_desc.Width >> miplevel),
            max(1, texture_desc.Height >> miplevel),
            max(1, texture_desc.Depth >> miplevel),
            sub_resource_idx, device, rb);

    ID3D11Device_Release(device);
}

static void *get_readback_data(struct resource_readback *rb, uint32_t x, uint32_t y, uint32_t z, unsigned byte_width)
{
    return (uint8_t *)rb->map_desc.pData + z * rb->map_desc.DepthPitch + y * rb->map_desc.RowPitch + x * byte_width;
}

static uint32_t get_readback_u32(struct resource_readback *rb, uint32_t x, uint32_t y)
{
    return *(uint32_t *)get_readback_data(rb, x, y, 0, sizeof(uint32_t));
}

static void release_resource_readback(struct resource_readback *rb)
{
    ID3D11DeviceContext_Unmap(rb->immediate_context, rb->resource, rb->sub_resource_idx);
    ID3D11Resource_Release(rb->resource);
    ID3D11DeviceContext_Release(rb->immediate_context);
}

#define check_readback_data_u32(a, b, c) check_readback_data_u32_(__LINE__, a, b, c)
static void check_readback_data_u32_(unsigned int line, struct resource_readback *rb,
        const RECT *rect, uint32_t expected)
{
    unsigned int x = 0, y = 0;
    BOOL all_match = FALSE;
    RECT default_rect;
    uint32_t got = 0;

    if (!rect)
    {
        SetRect(&default_rect, 0, 0, rb->width, rb->height);
        rect = &default_rect;
    }

    for (y = rect->top; y < rect->bottom; ++y)
    {
        for (x = rect->left; x < rect->right; ++x)
        {
            if ((got = get_readback_u32(rb, x, y)) != expected)
                goto done;
        }
    }
    all_match = TRUE;

done:
    ok_(__FILE__, line)(all_match,
            "Got 0x%08x, expected 0x%08x at (%u, %u), sub-resource %u.\n",
            got, expected, x, y, rb->sub_resource_idx);
}

#define check_texture_sub_resource_u32(a, b, c, d) check_texture_sub_resource_u32_(__LINE__, a, b, c, d)
static void check_texture_sub_resource_u32_(uint32_t line, ID3D11Texture2D *texture,
        uint32_t sub_resource_idx, const RECT *rect, uint32_t expected)
{
    struct resource_readback rb;

    get_texture_readback(texture, sub_resource_idx, &rb);
    check_readback_data_u32_(line, &rb, rect, expected);
    release_resource_readback(&rb);
}

#define check_test_image_load_info_resource(resource, image_load_info) \
    check_test_image_load_info_resource_(__LINE__, resource, image_load_info)
static void check_test_image_load_info_resource_(uint32_t line, ID3D11Resource *resource,
        const struct test_image_load_info *image_load_info)
{
    D3D11_RESOURCE_DIMENSION resource_dimension;
    HRESULT hr;

    ID3D11Resource_GetType(resource, &resource_dimension);
    ok(resource_dimension == image_load_info->expected_type, "Got unexpected ResourceDimension %u, expected %u.\n",
         resource_dimension, image_load_info->expected_type);
    switch (resource_dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            const D3D11_TEXTURE2D_DESC *expected_desc_2d = &image_load_info->expected_resource_desc.desc_2d;
            D3D11_TEXTURE2D_DESC desc_2d;
            ID3D11Texture2D *tex_2d;

            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture2D_GetDesc(tex_2d, &desc_2d);
            check_texture2d_desc_(line, &desc_2d, expected_desc_2d, image_load_info->todo_resource_desc);
            ID3D11Texture2D_Release(tex_2d);
            break;
        }

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            const D3D11_TEXTURE3D_DESC *expected_desc_3d = &image_load_info->expected_resource_desc.desc_3d;
            D3D11_TEXTURE3D_DESC desc_3d;
            ID3D11Texture3D *tex_3d;

            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&tex_3d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture3D_GetDesc(tex_3d, &desc_3d);
            check_texture3d_desc_(line, &desc_3d, expected_desc_3d, image_load_info->todo_resource_desc);
            ID3D11Texture3D_Release(tex_3d);
            break;
        }

        default:
            break;
    }
}

static void check_resource_info(ID3D11Resource *resource, const struct test_image *image, unsigned int line)
{
    unsigned int expected_mip_levels, expected_width, expected_height, max_dimension;
    D3D11_RESOURCE_DIMENSION resource_dimension;
    D3D11_TEXTURE2D_DESC desc_2d;
    D3D11_TEXTURE3D_DESC desc_3d;
    ID3D11Texture2D *texture_2d;
    ID3D11Texture3D *texture_3d;
    HRESULT hr;

    expected_width = image->expected_info.Width;
    expected_height = image->expected_info.Height;
    if (is_block_compressed(image->expected_info.Format))
    {
        expected_width = (expected_width + 3) & ~3;
        expected_height = (expected_height + 3) & ~3;
    }
    expected_mip_levels = 0;
    max_dimension = max(expected_width, expected_height);
    while (max_dimension)
    {
        ++expected_mip_levels;
        max_dimension >>= 1;
    }

    ID3D11Resource_GetType(resource, &resource_dimension);
    ok(resource_dimension == image->expected_info.ResourceDimension,
            "Got unexpected ResourceDimension %u, expected %u.\n",
             resource_dimension, image->expected_info.ResourceDimension);

    switch (resource_dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture_2d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture2D_GetDesc(texture_2d, &desc_2d);
            ok_(__FILE__, line)(desc_2d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_2d.Width, expected_width);
            ok_(__FILE__, line)(desc_2d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_2d.Height, expected_height);
            ok_(__FILE__, line)(desc_2d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_2d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_2d.ArraySize == image->expected_info.ArraySize,
                    "Got unexpected ArraySize %u, expected %u.\n",
                     desc_2d.ArraySize, image->expected_info.ArraySize);
            ok_(__FILE__, line)(desc_2d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_2d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Count == 1,
                    "Got unexpected SampleDesc.Count %u, expected %u\n",
                     desc_2d.SampleDesc.Count, 1);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Quality == 0,
                    "Got unexpected SampleDesc.Quality %u, expected %u\n",
                     desc_2d.SampleDesc.Quality, 0);
            ok_(__FILE__, line)(desc_2d.Usage == D3D11_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_2d.Usage, D3D11_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_2d.BindFlags == D3D11_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_2d.BindFlags, D3D11_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_2d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_2d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_2d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_2d.MiscFlags, image->expected_info.MiscFlags);

            ID3D11Texture2D_Release(texture_2d);
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture_3d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture3D_GetDesc(texture_3d, &desc_3d);
            ok_(__FILE__, line)(desc_3d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_3d.Width, expected_width);
            ok_(__FILE__, line)(desc_3d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_3d.Height, expected_height);
            ok_(__FILE__, line)(desc_3d.Depth == image->expected_info.Depth,
                    "Got unexpected Depth %u, expected %u.\n",
                     desc_3d.Depth, image->expected_info.Depth);
            ok_(__FILE__, line)(desc_3d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_3d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_3d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_3d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_3d.Usage == D3D11_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_3d.Usage, D3D11_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_3d.BindFlags == D3D11_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_3d.BindFlags, D3D11_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_3d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_3d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_3d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_3d.MiscFlags, image->expected_info.MiscFlags);
            ID3D11Texture3D_Release(texture_3d);
            break;

        default:
            break;
    }
}

static void check_texture2d_data(ID3D11Texture2D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, stride, i, array_slice, bpp;
    struct resource_readback rb;
    D3D11_TEXTURE2D_DESC desc;
    const BYTE *expected_data;
    BOOL line_match;

    ID3D11Texture2D_GetDesc(texture, &desc);
    width = desc.Width;
    height = desc.Height;
    bpp = get_bpp_from_format(desc.Format);
    stride = (width * bpp + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    for (array_slice = 0; array_slice < desc.ArraySize; ++array_slice)
    {
        get_texture_readback(texture, array_slice * desc.MipLevels, &rb);
        ok_(__FILE__, line)(!!rb.resource, "Failed to create texture readback.\n");
        if (!rb.resource)
            return;

        for (i = 0; i < height; ++i)
        {
            line_match = !memcmp(expected_data + stride * i, get_readback_data(&rb, 0, i, 0, bpp), stride);
            todo_wine_if(is_block_compressed(image->expected_info.Format) && image->data != test_dds_dxt5
                    && (image->expected_info.Width % 4 != 0 || image->expected_info.Height % 4 != 0))
                ok_(__FILE__, line)(line_match, "Data mismatch for line %u, array slice %u.\n", i, array_slice);
            if (!line_match)
                break;
        }
        expected_data += stride * height;

        release_resource_readback(&rb);
    }
}

static void check_texture3d_data(ID3D11Texture3D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, depth, stride, i, bpp;
    struct resource_readback rb;
    D3D11_TEXTURE3D_DESC desc;
    const BYTE *expected_data;
    BOOL line_match;

    ID3D11Texture3D_GetDesc(texture, &desc);
    width = desc.Width;
    height = desc.Height;
    depth = desc.Depth;
    bpp = get_bpp_from_format(desc.Format);
    stride = (width * bpp + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    get_texture3d_readback(texture, 0, &rb);
    ok_(__FILE__, line)(!!rb.resource, "Failed to create texture readback.\n");
    if (!rb.resource)
        return;

    for (i = 0; i < height * depth; ++i)
    {
        line_match = !memcmp(expected_data + stride * i, get_readback_data(&rb, 0, i, 0, bpp), stride);
        ok_(__FILE__, line)(line_match, "Data mismatch for line %u.\n", i);
        if (!line_match)
        {
            for (unsigned int j = 0; j < stride; ++j)
                trace("%02x\n", *((BYTE *)get_readback_data(&rb, j, i, 0, 1)));
            break;
        }
    }

    release_resource_readback(&rb);
}

static void check_resource_data(ID3D11Resource *resource, const struct test_image *image, unsigned int line)
{
    ID3D11Texture3D *texture3d;
    ID3D11Texture2D *texture2d;

    if (SUCCEEDED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture3d)))
    {
        if (wined3d_opengl && is_block_compressed(image->expected_info.Format))
            skip("Skipping compressed format 3D texture readback test.\n");
        else
            check_texture3d_data(texture3d, image, line);
        ID3D11Texture3D_Release(texture3d);
    }
    else if (SUCCEEDED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture2d)))
    {
        check_texture2d_data(texture2d, image, line);
        ID3D11Texture2D_Release(texture2d);
    }
    else
    {
        ok(0, "Failed to get 2D or 3D texture interface.\n");
    }
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

static void test_D3DX11CreateAsyncFileLoader(void)
{
    static const WCHAR test_filename[] = L"asyncloader.data";
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
    create_file(test_filename, test_data1, sizeof(test_data1), path);

    hr = D3DX11CreateAsyncFileLoaderW(path, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = delete_file(test_filename);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Create it again. */
    create_file(test_filename, test_data1, sizeof(test_data1), path);
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
    create_file(test_filename, test_data2, sizeof(test_data2), NULL);

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

    ret = delete_file(test_filename);
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

static void test_get_image_info(void)
{
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    char buffer[sizeof(test_bmp_1bpp) * 2];
    D3DX11_IMAGE_INFO image_info;
    HMODULE resource_module;
    WCHAR path[MAX_PATH];
    unsigned int i;
    DWORD dword;
    HRESULT hr, hr2;

    CoInitialize(NULL);

    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromMemory(test_image[0].data, 0, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromMemory(NULL, test_image[0].size, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    dword = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromMemory(&dword, sizeof(dword), NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    /* NULL hr2 is valid. */
    hr = D3DX11GetImageInfoFromMemory(test_image[0].data, test_image[0].size, NULL, &image_info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test a too-large size. */
    memset(buffer, 0xcc, sizeof(buffer));
    memcpy(buffer, test_bmp_1bpp, sizeof(test_bmp_1bpp));
    hr = D3DX11GetImageInfoFromMemory(buffer, sizeof(buffer), NULL, &image_info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info(&image_info, &test_image[0], __LINE__);

    /* Test a too-small size. */
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromMemory(test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, &image_info, &hr2);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    /* 2 bpp is not a valid bit count. */
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromMemory(test_bmp_2bpp, sizeof(test_bmp_2bpp), NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromMemory(test_image[i].data, test_image[i].size, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        winetest_pop_context();
    }

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX11GetImageInfoFromMemory(test_image[0].data, test_image[0].size, &thread_pump, &image_info, &hr2);
    todo_wine ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_image_info(&image_info, test_image, __LINE__);

    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileW(NULL, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileW(L"deadbeaf", NULL, &image_info, &hr2);
    todo_wine ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileA(NULL, NULL, &image_info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileA("deadbeaf", NULL, &image_info, &hr2);
    todo_wine ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

todo_wine {
    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromFileW(path, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromFileA(get_str_a(path), NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        delete_file(test_filename);
        winetest_pop_context();
    }
}

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
    /* ATI1 is unsupported, but ATI2 is supported. */
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, MAKEFOURCC('A','T','I','1'), 0, 0, 0, 0, 0, E_FAIL);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('A','T','I','2'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC5_UNORM);

    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B5G6R5_UNORM, E_FAIL);
    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B5G5R5A1_UNORM, E_FAIL);
    check_dds_dxt10_format_unsupported(DXGI_FORMAT_B4G4R4A4_UNORM, E_FAIL);
    /* Formats unsupported on d3dx10, but now supported on d3dx11. */
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_UF16, FALSE);
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC6H_SF16, DXGI_FORMAT_BC6H_SF16, FALSE);
    todo_wine check_dds_dxt10_format(DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM, FALSE);

    /*
     * These formats should map 1:1 from the DXT10 header, unlike legacy DDS
     * file equivalents.
     */
    check_dds_dxt10_format(DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, FALSE);
    check_dds_dxt10_format(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE);

    if (!strcmp(winetest_platform, "wine"))
    {
        skip("Skipping D3DX11GetImageInfoFromResource() tests.\n");
        CoUninitialize();
        return;
    }

    /* D3DX11GetImageInfoFromResource tests */

    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceW(NULL, NULL, NULL, &image_info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceW(NULL, L"deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceA(NULL, NULL, NULL, &image_info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceA(NULL, "deadbeaf", NULL, &image_info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceW(resource_module, L"deadbeef", NULL, &image_info, &hr2);
        ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceW(resource_module, test_resource_name, NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceA(resource_module, get_str_a(test_resource_name), NULL, &image_info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();
}

static void test_create_texture(void)
{
    static const uint32_t dds_24bit_8_8_mip_level_expected[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xff000000 };
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    D3D11_TEXTURE2D_DESC tex_2d_desc;
    D3DX11_IMAGE_LOAD_INFO load_info;
    D3DX11_IMAGE_INFO img_info;
    unsigned int i, mip_level;
    ID3D11Resource *resource;
    ID3D11Texture2D *tex_2d;
    HMODULE resource_module;
    ID3D11Device *device;
    WCHAR path[MAX_PATH];
    HRESULT hr, hr2;

    if (!strcmp(winetest_platform, "wine"))
    {
        skip("Skipping texture creation tests.\n");
        return;
    }

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    /* D3DX11CreateTextureFromMemory tests */

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(NULL, test_bmp_1bpp, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, NULL, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, NULL, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, test_bmp_1bpp, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test_image[i].data, test_image[i].size, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        /* Test behavior with load_info structure values set to defaults. */
        load_info = d3dx11_default_load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test_image[i].data, test_image[i].size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        /* Check that passed in D3DX11_IMAGE_INFO structure is filled. */
        memset(&img_info, 0, sizeof(img_info));
        load_info = d3dx11_default_load_info;
        load_info.pSrcInfo = &img_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test_image[i].data, test_image[i].size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ok(!memcmp(&test_image[i].expected_info, &img_info, sizeof(img_info)), "Unexpected image info.\n");
            ID3D11Resource_Release(resource);
        }

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        load_info = test->load_info;
        hr = D3DX11CreateTextureFromMemory(device, test->data, test->size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test->todo_hr) ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_image_load_info); ++i)
    {
        const struct test_image_load_info *test = &test_image_load_info[i];

        winetest_push_context("Test %u", i);

        load_info = test->load_info;
        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test->data, test->size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test);
            ID3D11Resource_Release(resource);
        }

        winetest_pop_context();
    }

    /*
     * Test behavior of the FirstMipLevel argument, including checks of the
     * values loaded into each level of the created texture.
     */
    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("FirstMipLevel %u", i);
        memset(&img_info, 0, sizeof(img_info));
        load_info = d3dx11_default_load_info;
        load_info.FirstMipLevel = i;
        load_info.MipLevels = D3DX11_FROM_FILE;
        load_info.pSrcInfo = &img_info;

        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        /*
         * The FirstMipLevel behavior does not exactly match D3DX_SKIP_DDS_MIP_LEVEL
         * from d3dx9, image info values always represent mip level 0 regardless of
         * what FirstMipLevel is set to. Further, texture dimensions are
         * always based on mip level 0, and the number of mip levels is always the same
         * as the number in the image info structure.
         */
        ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
        check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
        check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM,
                D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

        /*
         * Image data is loaded starting from the mip level provided by
         * FirstMipLevel. This behavior does match d3dx9's D3DX_SKIP_DDS_MIP_LEVEL.
         */
        for (mip_level = 0; mip_level < 4; ++mip_level)
        {
            winetest_push_context("MipLevel %u", mip_level);
            check_texture_sub_resource_u32(tex_2d, mip_level, NULL,
                    dds_24bit_8_8_mip_level_expected[min(3, mip_level + i)]);
            winetest_pop_context();
        }

        ID3D11Texture2D_Release(tex_2d);
        ID3D11Resource_Release(resource);
        winetest_pop_context();
    }

    /*
     * If FirstMipLevel is set to a value that is larger than the total number
     * of mip levels in the image, it falls back to 0.
     */
    memset(&img_info, 0, sizeof(img_info));
    load_info = d3dx11_default_load_info;
    load_info.FirstMipLevel = 5;
    load_info.MipLevels = D3DX11_FROM_FILE;
    load_info.pSrcInfo = &img_info;

    resource = NULL;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &resource, &hr2);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM,
            D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

    hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
    check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
    for (mip_level = 0; mip_level < 4; ++mip_level)
    {
        winetest_push_context("MipLevel %u", mip_level);
        check_texture_sub_resource_u32(tex_2d, mip_level, NULL, dds_24bit_8_8_mip_level_expected[mip_level]);
        winetest_pop_context();
    }

    ID3D11Texture2D_Release(tex_2d);
    ID3D11Resource_Release(resource);

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX11CreateTextureFromMemory(device, test_image[0].data, test_image[0].size,
            NULL, &thread_pump, &resource, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_resource_info(resource, test_image, __LINE__);
    check_resource_data(resource, test_image, __LINE__);
    ID3D11Resource_Release(resource);

    /* D3DX11CreateTextureFromFile tests */

    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileW(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileW(device, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileA(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileA(device, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileW(device, path, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileA(device, get_str_a(path), NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        delete_file(test_filename);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        create_file(test_filename, test->data, test->size, path);
        load_info = test->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileW(device, path, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test->todo_hr) ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        hr = D3DX11CreateTextureFromFileA(device, get_str_a(path), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test->todo_hr) ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        delete_file(test_filename);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_image_load_info); ++i)
    {
        const struct test_image_load_info *test = &test_image_load_info[i];

        winetest_push_context("Test %u", i);
        create_file(test_filename, test->data, test->size, path);
        load_info = test->load_info;

        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileW(device, path, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileA(device, get_str_a(path), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test);
            ID3D11Resource_Release(resource);
        }

        delete_file(test_filename);
        winetest_pop_context();
    }

    /* D3DX11CreateTextureFromResource tests */

    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceW(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceW(device, NULL, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceA(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceA(device, NULL, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module, L"deadbeef", NULL, NULL, &resource, &hr2);
        ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module,
                test_resource_name, NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test->data, test->size);
        load_info = test->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module,
                test_resource_name, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test->todo_hr) ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test->todo_hr) ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_image_load_info); ++i)
    {
        const struct test_image_load_info *test = &test_image_load_info[i];

        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test->data, test->size);
        load_info = test->load_info;

        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module,
                test_resource_name, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test);
            ID3D11Resource_Release(resource);
        }

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
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

START_TEST(d3dx11)
{
    HMODULE wined3d;

    if ((wined3d = GetModuleHandleA("wined3d.dll")))
    {
        enum wined3d_renderer (CDECL *p_wined3d_get_renderer)(void);

        if ((p_wined3d_get_renderer = (void *)GetProcAddress(wined3d, "wined3d_get_renderer"))
                && p_wined3d_get_renderer() == WINED3D_RENDERER_OPENGL)
            wined3d_opengl = true;
    }

    test_D3DX11CreateAsyncMemoryLoader();
    test_D3DX11CreateAsyncFileLoader();
    test_D3DX11CreateAsyncResourceLoader();
    test_D3DX11CompileFromFile();
    test_get_image_info();
    test_create_texture();
    test_legacy_dds_header_image_info();
    test_dxt10_dds_header_image_info();
}
