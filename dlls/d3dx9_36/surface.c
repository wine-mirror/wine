/*
 * Copyright (C) 2009-2010 Tony Wasserka
 * Copyright (C) 2012 Józef Kucia
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
 *
 */


#include "d3dx9_private.h"

#include "initguid.h"
#include "ole2.h"
#include "wincodec.h"

#define BCDEC_IMPLEMENTATION
#define BCDEC_STATIC
#include "bcdec.h"
#define STB_DXT_IMPLEMENTATION
#define STB_DXT_STATIC
#include "stb_dxt.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT, IWICImagingFactory**);

static const struct
{
    const GUID *wic_guid;
    enum d3dx_pixel_format_id d3dx_pixel_format;
} wic_pixel_formats[] =
{
    { &GUID_WICPixelFormat8bppIndexed, D3DX_PIXEL_FORMAT_P8_UINT },
    { &GUID_WICPixelFormat1bppIndexed, D3DX_PIXEL_FORMAT_P1_UINT },
    { &GUID_WICPixelFormat2bppIndexed, D3DX_PIXEL_FORMAT_P2_UINT },
    { &GUID_WICPixelFormat4bppIndexed, D3DX_PIXEL_FORMAT_P4_UINT },
    { &GUID_WICPixelFormat8bppGray,    D3DX_PIXEL_FORMAT_L8_UNORM },
    { &GUID_WICPixelFormat16bppBGR555, D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM },
    { &GUID_WICPixelFormat16bppBGR565, D3DX_PIXEL_FORMAT_B5G6R5_UNORM },
    { &GUID_WICPixelFormat24bppBGR,    D3DX_PIXEL_FORMAT_B8G8R8_UNORM },
    { &GUID_WICPixelFormat32bppBGR,    D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM },
    { &GUID_WICPixelFormat32bppBGRA,   D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM },
    { &GUID_WICPixelFormat48bppRGB,    D3DX_PIXEL_FORMAT_R16G16B16_UNORM },
    { &GUID_WICPixelFormat64bppRGBA,   D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM },
};

static enum d3dx_pixel_format_id d3dx_pixel_format_id_from_wic_pixel_format(const GUID *guid)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(wic_pixel_formats); i++)
    {
        if (IsEqualGUID(wic_pixel_formats[i].wic_guid, guid))
            return wic_pixel_formats[i].d3dx_pixel_format;
    }

    return D3DX_PIXEL_FORMAT_COUNT;

}

static const GUID *wic_guid_from_d3dx_pixel_format_id(enum d3dx_pixel_format_id d3dx_pixel_format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(wic_pixel_formats); i++)
    {
        if (wic_pixel_formats[i].d3dx_pixel_format == d3dx_pixel_format)
            return wic_pixel_formats[i].wic_guid;
    }

    return NULL;
}

#define TGA_IMAGETYPE_COLORMAPPED 1
#define TGA_IMAGETYPE_TRUECOLOR 2
#define TGA_IMAGETYPE_GRAYSCALE 3
#define TGA_IMAGETYPE_MASK 0x07
#define TGA_IMAGETYPE_RLE 8

#define TGA_IMAGE_RIGHTTOLEFT 0x10
#define TGA_IMAGE_TOPTOBOTTOM 0x20

#pragma pack(push,1)
struct tga_header
{
    uint8_t  id_length;
    uint8_t  color_map_type;
    uint8_t  image_type;
    uint16_t color_map_firstentry;
    uint16_t color_map_length;
    uint8_t  color_map_entrysize;
    uint16_t xorigin;
    uint16_t yorigin;
    uint16_t width;
    uint16_t height;
    uint8_t  depth;
    uint8_t  image_descriptor;
};
#pragma pack(pop)

HRESULT lock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect, D3DLOCKED_RECT *lock,
        IDirect3DSurface9 **temp_surface, BOOL write)
{
    unsigned int width, height;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    DWORD lock_flag;
    HRESULT hr;

    lock_flag = write ? 0 : D3DLOCK_READONLY;
    *temp_surface = NULL;
    if (FAILED(hr = IDirect3DSurface9_LockRect(surface, lock, surface_rect, lock_flag)))
    {
        IDirect3DSurface9_GetDevice(surface, &device);
        IDirect3DSurface9_GetDesc(surface, &desc);

        if (surface_rect)
        {
            width = surface_rect->right - surface_rect->left;
            height = surface_rect->bottom - surface_rect->top;
        }
        else
        {
            width = desc.Width;
            height = desc.Height;
        }

        hr = write ? IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height,
                desc.Format, D3DPOOL_SYSTEMMEM, temp_surface, NULL)
                : IDirect3DDevice9_CreateRenderTarget(device, width, height,
                desc.Format, D3DMULTISAMPLE_NONE, 0, TRUE, temp_surface, NULL);
        if (FAILED(hr))
        {
            WARN("Failed to create temporary surface, surface %p, format %#x, "
                    "usage %#lx, pool %#x, write %#x, width %u, height %u.\n",
                    surface, desc.Format, desc.Usage, desc.Pool, write, width, height);
            IDirect3DDevice9_Release(device);
            return hr;
        }

        if (write || SUCCEEDED(hr = IDirect3DDevice9_StretchRect(device, surface, surface_rect,
                *temp_surface, NULL, D3DTEXF_NONE)))
            hr = IDirect3DSurface9_LockRect(*temp_surface, lock, NULL, lock_flag);

        IDirect3DDevice9_Release(device);
        if (FAILED(hr))
        {
            WARN("Failed to lock surface %p, write %#x, usage %#lx, pool %#x.\n",
                    surface, write, desc.Usage, desc.Pool);
            IDirect3DSurface9_Release(*temp_surface);
            *temp_surface = NULL;
            return hr;
        }
        TRACE("Created temporary surface %p.\n", surface);
    }
    return hr;
}

HRESULT unlock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect,
        IDirect3DSurface9 *temp_surface, BOOL update)
{
    IDirect3DDevice9 *device;
    POINT surface_point;
    HRESULT hr;

    if (!temp_surface)
    {
        hr = IDirect3DSurface9_UnlockRect(surface);
        return hr;
    }

    hr = IDirect3DSurface9_UnlockRect(temp_surface);
    if (update)
    {
        if (surface_rect)
        {
            surface_point.x = surface_rect->left;
            surface_point.y = surface_rect->top;
        }
        else
        {
            surface_point.x = 0;
            surface_point.y = 0;
        }
        IDirect3DSurface9_GetDevice(surface, &device);
        if (FAILED(hr = IDirect3DDevice9_UpdateSurface(device, temp_surface, NULL, surface, &surface_point)))
            WARN("Updating surface failed, hr %#lx, surface %p, temp_surface %p.\n",
                    hr, surface, temp_surface);
        IDirect3DDevice9_Release(device);
    }
    IDirect3DSurface9_Release(temp_surface);
    return hr;
}

static const struct
{
    struct dds_pixel_format dds_pixel_format;
    enum d3dx_pixel_format_id d3dx_pixel_format;
} dds_pixel_formats[] =
{
    /* DDS_PF_FOURCC. */
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('U','Y','V','Y') }, D3DX_PIXEL_FORMAT_UYVY },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('Y','U','Y','2') }, D3DX_PIXEL_FORMAT_YUY2 },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('R','G','B','G') }, D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('G','R','G','B') }, D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('D','X','T','1') }, D3DX_PIXEL_FORMAT_DXT1_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('D','X','T','2') }, D3DX_PIXEL_FORMAT_DXT2_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('D','X','T','3') }, D3DX_PIXEL_FORMAT_DXT3_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('D','X','T','4') }, D3DX_PIXEL_FORMAT_DXT4_UNORM },
    { { 32, DDS_PF_FOURCC, MAKEFOURCC('D','X','T','5') }, D3DX_PIXEL_FORMAT_DXT5_UNORM },
    /* These aren't actually fourcc values, they're just D3DFMT values. */
    { { 32, DDS_PF_FOURCC, 0x24 }, D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM },
    { { 32, DDS_PF_FOURCC, 0x6e }, D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM },
    { { 32, DDS_PF_FOURCC, 0x6f }, D3DX_PIXEL_FORMAT_R16_FLOAT },
    { { 32, DDS_PF_FOURCC, 0x70 }, D3DX_PIXEL_FORMAT_R16G16_FLOAT },
    { { 32, DDS_PF_FOURCC, 0x71 }, D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT },
    { { 32, DDS_PF_FOURCC, 0x72 }, D3DX_PIXEL_FORMAT_R32_FLOAT },
    { { 32, DDS_PF_FOURCC, 0x73 }, D3DX_PIXEL_FORMAT_R32G32_FLOAT },
    { { 32, DDS_PF_FOURCC, 0x74 }, D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT },
    /* DDS_PF_RGB. */
    { { 32, DDS_PF_RGB,  0, 8,  0xe0,       0x1c,       0x03,       0x00       }, D3DX_PIXEL_FORMAT_B2G3R3_UNORM },
    { { 32, DDS_PF_RGB,  0, 16, 0xf800,     0x07e0,     0x001f,     0x0000     }, D3DX_PIXEL_FORMAT_B5G6R5_UNORM },
    { { 32, DDS_PF_RGB,  0, 16, 0x7c00,     0x03e0,     0x001f,     0x0000     }, D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM },
    { { 32, DDS_PF_RGB,  0, 16, 0x0f00,     0x00f0,     0x000f,     0x0000     }, D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM },
    { { 32, DDS_PF_RGB,  0, 24, 0xff0000,   0x00ff00,   0x0000ff,   0x000000   }, D3DX_PIXEL_FORMAT_B8G8R8_UNORM },
    { { 32, DDS_PF_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 }, D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM },
    { { 32, DDS_PF_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 }, D3DX_PIXEL_FORMAT_R16G16_UNORM },
    { { 32, DDS_PF_RGB,  0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 }, D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0,     0x001c,     0x0003,     0xff00     }, D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00,     0x03e0,     0x001f,     0x8000     }, D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00,     0x00f0,     0x000f,     0xf000     }, D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }, D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 }, D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM },
    { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 }, D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM },
    /* DDS_PF_INDEXED. */
    { { 32, DDS_PF_INDEXED, 0, 8 },                                   D3DX_PIXEL_FORMAT_P8_UINT },
    { { 32, DDS_PF_INDEXED | DDS_PF_ALPHA, 0, 16, 0, 0, 0, 0xff00, }, D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM },
    /* DDS_PF_LUMINANCE. */
    { { 32, DDS_PF_LUMINANCE, 0,  8, 0x00ff },                              D3DX_PIXEL_FORMAT_L8_UNORM },
    { { 32, DDS_PF_LUMINANCE, 0, 16, 0xffff },                              D3DX_PIXEL_FORMAT_L16_UNORM },
    { { 32, DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0,  8, 0x000f, 0, 0, 0x00f0 }, D3DX_PIXEL_FORMAT_L4A4_UNORM },
    { { 32, DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00 }, D3DX_PIXEL_FORMAT_L8A8_UNORM },
    /* Exceptional case, A8L8 can also have 8bpp. */
    { { 32, DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8,  0x00ff, 0, 0, 0xff00 }, D3DX_PIXEL_FORMAT_L8A8_UNORM },
    /* DDS_PF_ALPHA_ONLY. */
    { { 32, DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff }, D3DX_PIXEL_FORMAT_A8_UNORM },
    /* DDS_PF_BUMPDUDV. */
    { { 32, DDS_PF_BUMPDUDV, 0, 16, 0x000000ff, 0x0000ff00, 0x00000000, 0x00000000 }, D3DX_PIXEL_FORMAT_U8V8_SNORM },
    { { 32, DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 }, D3DX_PIXEL_FORMAT_U16V16_SNORM },
    { { 32, DDS_PF_BUMPDUDV, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 }, D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM },
    { { 32, DDS_PF_BUMPDUDV | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 }, D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM },
    /* DDS_PF_BUMPLUMINANCE. */
    { { 32, DDS_PF_BUMPLUMINANCE, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 }, D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM },
};

static BOOL dds_pixel_format_compare(const struct dds_pixel_format *pf_a, const struct dds_pixel_format *pf_b,
        BOOL check_rmask, BOOL check_gmask, BOOL check_bmask, BOOL check_amask)
{
    return pf_a->bpp == pf_b->bpp && !((check_rmask && pf_a->rmask != pf_b->rmask)
            || (check_gmask && pf_a->gmask != pf_b->gmask) || (check_bmask && pf_a->bmask != pf_b->bmask)
            || (check_amask && pf_a->amask != pf_b->amask));
}

static enum d3dx_pixel_format_id d3dx_pixel_format_id_from_dds_pixel_format(const struct dds_pixel_format *pixel_format)
{
    uint32_t i;

    TRACE("pixel_format: size %lu, flags %#lx, fourcc %#lx, bpp %lu.\n", pixel_format->size,
            pixel_format->flags, pixel_format->fourcc, pixel_format->bpp);
    TRACE("rmask %#lx, gmask %#lx, bmask %#lx, amask %#lx.\n", pixel_format->rmask, pixel_format->gmask,
            pixel_format->bmask, pixel_format->amask);

    for (i = 0; i < ARRAY_SIZE(dds_pixel_formats); ++i)
    {
        const struct dds_pixel_format *dds_pf = &dds_pixel_formats[i].dds_pixel_format;

        if (pixel_format->flags != dds_pf->flags)
            continue;

        switch (pixel_format->flags & ~DDS_PF_ALPHA)
        {
            case DDS_PF_ALPHA_ONLY:
                if (dds_pixel_format_compare(pixel_format, dds_pf, FALSE, FALSE, FALSE, TRUE))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_FOURCC:
                if (pixel_format->fourcc == dds_pf->fourcc)
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_INDEXED:
                if (dds_pixel_format_compare(pixel_format, dds_pf, FALSE, FALSE, FALSE, pixel_format->flags & DDS_PF_ALPHA))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_RGB:
                if (dds_pixel_format_compare(pixel_format, dds_pf, TRUE, TRUE, TRUE, pixel_format->flags & DDS_PF_ALPHA))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_LUMINANCE:
                if (dds_pixel_format_compare(pixel_format, dds_pf, TRUE, FALSE, FALSE, pixel_format->flags & DDS_PF_ALPHA))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_BUMPLUMINANCE:
                if (dds_pixel_format_compare(pixel_format, dds_pf, TRUE, TRUE, TRUE, FALSE))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            case DDS_PF_BUMPDUDV:
                if (dds_pixel_format_compare(pixel_format, dds_pf, TRUE, TRUE, TRUE, TRUE))
                    return dds_pixel_formats[i].d3dx_pixel_format;
                break;

            default:
                assert(0); /* Should not happen. */
                break;
        }
    }

    WARN("Unknown pixel format (flags %#lx, fourcc %#lx, bpp %lu, r %#lx, g %#lx, b %#lx, a %#lx).\n",
        pixel_format->flags, pixel_format->fourcc, pixel_format->bpp,
        pixel_format->rmask, pixel_format->gmask, pixel_format->bmask, pixel_format->amask);
    return D3DX_PIXEL_FORMAT_COUNT;
}

static HRESULT dds_pixel_format_from_d3dx_pixel_format_id(struct dds_pixel_format *pixel_format,
        enum d3dx_pixel_format_id d3dx_pixel_format)
{
    const struct dds_pixel_format *pf = NULL;
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(dds_pixel_formats); ++i)
    {
        if (dds_pixel_formats[i].d3dx_pixel_format == d3dx_pixel_format)
        {
            pf = &dds_pixel_formats[i].dds_pixel_format;
            break;
        }
    }

    if (!pf)
    {
        WARN("Unhandled format %#x.\n", d3dx_pixel_format);
        return E_NOTIMPL;
    }

    if (pixel_format)
        *pixel_format = *pf;

    return D3D_OK;
}

static void d3dx_get_next_mip_level_size(struct volume *size)
{
    size->width  = max(size->width  / 2, 1);
    size->height = max(size->height / 2, 1);
    size->depth  = max(size->depth  / 2, 1);
}

static const char *debug_volume(const struct volume *volume)
{
    if (!volume)
        return "(null)";
    return wine_dbg_sprintf("(%ux%ux%u)", volume->width, volume->height, volume->depth);
}

static HRESULT d3dx_calculate_pixels_size(enum d3dx_pixel_format_id format, uint32_t width, uint32_t height,
    uint32_t *pitch, uint32_t *size)
{
    const struct pixel_format_desc *format_desc = get_d3dx_pixel_format_info(format);

    if (is_unknown_format(format_desc))
        return E_NOTIMPL;

    if (format_desc->block_width != 1 || format_desc->block_height != 1)
    {
        *pitch = format_desc->block_byte_count
            * max(1, (width + format_desc->block_width - 1) / format_desc->block_width);
        *size = *pitch
            * max(1, (height + format_desc->block_height - 1) / format_desc->block_height);
    }
    else
    {
        *pitch = width * format_desc->bytes_per_pixel;
        *size = *pitch * height;
    }

    return D3D_OK;
}

uint32_t d3dx_calculate_layer_pixels_size(enum d3dx_pixel_format_id format, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t mip_levels)
{
    uint32_t layer_size, row_pitch, slice_pitch, i;
    struct volume dims = { width, height, depth };

    layer_size = 0;
    for (i = 0; i < mip_levels; ++i)
    {
        if (FAILED(d3dx_calculate_pixels_size(format, dims.width, dims.height, &row_pitch, &slice_pitch)))
            return 0;
        layer_size += slice_pitch * dims.depth;
        d3dx_get_next_mip_level_size(&dims);
    }

    return layer_size;
}

HRESULT d3dx_init_dds_header(struct dds_header *header, D3DRESOURCETYPE resource_type,
        enum d3dx_pixel_format_id format, const struct volume *size, uint32_t mip_levels)
{
    HRESULT hr;

    memset(header, 0, sizeof(*header));
    header->signature = MAKEFOURCC('D','D','S',' ');
    /* The signature is not really part of the DDS header. */
    header->size = sizeof(*header) - FIELD_OFFSET(struct dds_header, size);
    hr = dds_pixel_format_from_d3dx_pixel_format_id(&header->pixel_format, format);
    if (FAILED(hr))
        return hr;

    header->flags = DDS_CAPS | DDS_HEIGHT | DDS_WIDTH | DDS_PIXELFORMAT;
    header->height = size->height;
    header->width = size->width;
    header->caps = DDS_CAPS_TEXTURE;
    if (size->depth > 1)
    {
        header->flags |= DDS_DEPTH;
        header->depth = size->depth;
        header->caps2 |= DDS_CAPS2_VOLUME;
    }

    if (mip_levels > 1)
    {
        header->flags |= DDS_MIPMAPCOUNT;
        header->caps |= (DDS_CAPS_MIPMAP | DDS_CAPS_COMPLEX);
        header->miplevels = mip_levels;
    }

    if (resource_type == D3DRTYPE_CUBETEXTURE)
    {
        header->caps |= DDS_CAPS_COMPLEX;
        header->caps2 |= (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES);
    }

    if (header->pixel_format.flags & DDS_PF_ALPHA || header->pixel_format.flags & DDS_PF_ALPHA_ONLY)
        header->caps |= DDSCAPS_ALPHA;
    if (header->pixel_format.flags & DDS_PF_INDEXED)
        header->caps |= DDSCAPS_PALETTE;

    return D3D_OK;
}

static const GUID *wic_container_guid_from_d3dx_file_format(D3DXIMAGE_FILEFORMAT iff)
{
    switch (iff)
    {
        case D3DXIFF_DIB:
        case D3DXIFF_BMP: return &GUID_ContainerFormatBmp;
        case D3DXIFF_JPG: return &GUID_ContainerFormatJpeg;
        case D3DXIFF_PNG: return &GUID_ContainerFormatPng;
        default:
            assert(0 && "Unexpected file format.");
            return NULL;
    }
}

static HRESULT d3dx_pixels_save_wic(struct d3dx_pixels *pixels, const struct pixel_format_desc *fmt_desc,
        D3DXIMAGE_FILEFORMAT image_file_format, IStream **wic_file, uint32_t *wic_file_size)
{
    const GUID *container_format = wic_container_guid_from_d3dx_file_format(image_file_format);
    const GUID *pixel_format_guid = wic_guid_from_d3dx_pixel_format_id(fmt_desc->format);
    IWICBitmapFrameEncode *wic_frame = NULL;
    IPropertyBag2 *encoder_options = NULL;
    IWICBitmapEncoder *wic_encoder = NULL;
    WICPixelFormatGUID wic_pixel_format;
    const LARGE_INTEGER seek = { 0 };
    IWICImagingFactory *wic_factory;
    IWICPalette *wic_palette = NULL;
    IStream *stream = NULL;
    STATSTG stream_stats;
    HRESULT hr;

    assert(container_format && pixel_format_guid);
    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &wic_factory);
    if (FAILED(hr))
        return D3DERR_INVALIDCALL;

    hr = IWICImagingFactory_CreateEncoder(wic_factory, container_format, NULL, &wic_encoder);
    if (FAILED(hr))
    {
        hr = D3DERR_INVALIDCALL;
        goto exit;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapEncoder_Initialize(wic_encoder, stream, WICBitmapEncoderNoCache);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapEncoder_CreateNewFrame(wic_encoder, &wic_frame, &encoder_options);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapFrameEncode_Initialize(wic_frame, encoder_options);
    if (FAILED(hr))
        goto exit;

    if (image_file_format == D3DXIFF_JPG)
        FIXME("JPEG saving quality adjustment currently unimplemented, expect lower quality JPEG.\n");

    hr = IWICBitmapFrameEncode_SetSize(wic_frame, pixels->size.width, pixels->size.height);
    if (FAILED(hr))
        goto exit;

    if (pixels->palette)
    {
        WICColor tmp_palette[256];
        unsigned int i;

        hr = IWICImagingFactory_CreatePalette(wic_factory, &wic_palette);
        if (FAILED(hr))
            goto exit;

        for (i = 0; i < ARRAY_SIZE(tmp_palette); ++i)
        {
            const PALETTEENTRY *pe = &pixels->palette[i];

            tmp_palette[i] = (pe->peFlags << 24) | (pe->peRed << 16) | (pe->peGreen << 8) | (pe->peBlue);
        }

        hr = IWICPalette_InitializeCustom(wic_palette, tmp_palette, ARRAY_SIZE(tmp_palette));
        if (FAILED(hr))
            goto exit;

        hr = IWICBitmapFrameEncode_SetPalette(wic_frame, wic_palette);
        if (FAILED(hr))
            goto exit;
    }

    /*
     * Encode 32bpp BGRA format surfaces as 32bpp BGRX for BMP.
     * This matches the behavior of native.
     */
    if (IsEqualGUID(&GUID_ContainerFormatBmp, container_format) && (fmt_desc->format == D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM))
        pixel_format_guid = wic_guid_from_d3dx_pixel_format_id(D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM);

    wic_pixel_format = *pixel_format_guid;
    hr = IWICBitmapFrameEncode_SetPixelFormat(wic_frame, &wic_pixel_format);
    if (FAILED(hr))
        goto exit;

    if (!IsEqualGUID(pixel_format_guid, &wic_pixel_format))
    {
        ERR("SetPixelFormat returned a different pixel format.\n");
        hr = E_FAIL;
        goto exit;
    }

    hr = IWICBitmapFrameEncode_WritePixels(wic_frame, pixels->size.height, pixels->row_pitch, pixels->slice_pitch,
            (BYTE *)pixels->data);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapFrameEncode_Commit(wic_frame);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapEncoder_Commit(wic_encoder);
    if (FAILED(hr))
        goto exit;

    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto exit;

    hr = IStream_Stat(stream, &stream_stats, STATFLAG_NONAME);
    if (FAILED(hr))
        goto exit;

    if (!stream_stats.cbSize.u.HighPart && !!stream_stats.cbSize.u.LowPart)
    {
        *wic_file = stream;
        *wic_file_size = stream_stats.cbSize.u.LowPart;
    }
    else
    {
        hr = D3DXERR_INVALIDDATA;
    }

exit:
    if (wic_factory)
        IWICImagingFactory_Release(wic_factory);
    if (stream && (*wic_file != stream))
        IStream_Release(stream);
    if (wic_frame)
        IWICBitmapFrameEncode_Release(wic_frame);
    if (wic_palette)
        IWICPalette_Release(wic_palette);
    if (encoder_options)
        IPropertyBag2_Release(encoder_options);
    if (wic_encoder)
        IWICBitmapEncoder_Release(wic_encoder);

    return hr;
}

static const enum d3dx_pixel_format_id tga_save_pixel_formats[] =
{
    D3DX_PIXEL_FORMAT_B8G8R8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM
};

static const enum d3dx_pixel_format_id png_save_pixel_formats[] =
{
    D3DX_PIXEL_FORMAT_B8G8R8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM,
    D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM
};

static const enum d3dx_pixel_format_id jpg_save_pixel_formats[] =
{
    D3DX_PIXEL_FORMAT_B8G8R8_UNORM,
};

static const enum d3dx_pixel_format_id bmp_save_pixel_formats[] =
{
    D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM,
    D3DX_PIXEL_FORMAT_B5G6R5_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM,
    D3DX_PIXEL_FORMAT_P8_UINT,
};

static const enum d3dx_pixel_format_id unimplemented_bmp_save_pixel_formats[] =
{
    D3DX_PIXEL_FORMAT_A8_UNORM,
    D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM,
    D3DX_PIXEL_FORMAT_L8A8_UNORM,
    D3DX_PIXEL_FORMAT_L16_UNORM,
    D3DX_PIXEL_FORMAT_B2G3R3_UNORM,
    D3DX_PIXEL_FORMAT_R16_FLOAT,
    D3DX_PIXEL_FORMAT_R16G16_FLOAT,
    D3DX_PIXEL_FORMAT_R16G16_UNORM,
    D3DX_PIXEL_FORMAT_R32_FLOAT,
    D3DX_PIXEL_FORMAT_R32G32_FLOAT,
    D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM,
    D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM,
    D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM,
    D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM,
    D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM,
    D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM,
    D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM,
    D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM,
};

static enum d3dx_pixel_format_id d3dx_get_closest_d3dx_pixel_format_id(const enum d3dx_pixel_format_id *format_ids,
        uint32_t format_ids_size, enum d3dx_pixel_format_id format_id)
{
    const struct pixel_format_desc *fmt, *curfmt, *bestfmt = NULL;
    int bestscore = INT_MIN, rgb_channels, a_channel, i, j;
    BOOL alpha_only, rgb_only;

    for (i = 0; i < format_ids_size; ++i)
    {
        if (format_ids[i] == format_id)
            return format_id;
    }

    TRACE("Requested format is not directly supported, looking for the best alternative.\n");
    switch (format_id)
    {
        case D3DX_PIXEL_FORMAT_P8_UINT:
        case D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM:
        case D3DX_PIXEL_FORMAT_DXT1_UNORM:
        case D3DX_PIXEL_FORMAT_DXT2_UNORM:
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:
        case D3DX_PIXEL_FORMAT_DXT4_UNORM:
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:
            fmt = get_d3dx_pixel_format_info(D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM);
            break;

        default:
            fmt = get_d3dx_pixel_format_info(format_id);
            break;
    }

    alpha_only = rgb_only = FALSE;
    if (fmt->a_type != CTYPE_EMPTY && fmt->rgb_type == CTYPE_EMPTY)
        alpha_only = TRUE;
    else if (fmt->a_type == CTYPE_EMPTY && fmt->rgb_type != CTYPE_EMPTY)
        rgb_only = TRUE;

    if (fmt->rgb_type == CTYPE_LUMA)
        rgb_channels = 3;
    else
        rgb_channels = !!fmt->bits[1] + !!fmt->bits[2] + !!fmt->bits[3];
    a_channel = !!fmt->bits[0];
    for (i = 0; i < format_ids_size; ++i)
    {
        int cur_rgb_channels, cur_a_channel, score;

        curfmt = get_d3dx_pixel_format_info(format_ids[i]);
        if (!is_conversion_to_supported(curfmt))
            continue;
        if (alpha_only && curfmt->a_type == CTYPE_EMPTY)
            continue;
        if (rgb_only && curfmt->rgb_type == CTYPE_EMPTY)
            continue;
        if (fmt->rgb_type == CTYPE_SNORM && curfmt->rgb_type != CTYPE_SNORM)
            continue;

        cur_rgb_channels = !!curfmt->bits[1] + !!curfmt->bits[2] + !!curfmt->bits[3];
        cur_a_channel = !!curfmt->bits[0];
        /* Calculate a score for this format. */
        score = 512 * (format_types_match(curfmt, fmt));
        score -= 32 * abs(cur_a_channel - a_channel);
        score -= 32 * abs(cur_rgb_channels - rgb_channels);
        for (j = 0; j < 4; ++j)
        {
            int diff = curfmt->bits[j] - fmt->bits[j];

            score -= (diff < 0 ? -diff * 8 : diff) * (j == 0 ? 1 : 2);
        }

        if (score > bestscore)
        {
            bestscore = score;
            bestfmt = curfmt;
        }
    }

    return (bestfmt) ? bestfmt->format : D3DX_PIXEL_FORMAT_COUNT;
}

HRESULT d3dx_save_pixels_to_memory(struct d3dx_pixels *src_pixels, const struct pixel_format_desc *src_fmt_desc,
        D3DXIMAGE_FILEFORMAT file_format, ID3DXBuffer **dst_buffer)
{
    enum d3dx_pixel_format_id dst_format = src_fmt_desc->format;
    const struct pixel_format_desc *dst_fmt_desc;
    uint32_t dst_row_pitch, dst_slice_pitch;
    struct d3dx_pixels dst_pixels;
    uint8_t *pixels, *tmp_buf;
    ID3DXBuffer *buffer;
    HRESULT hr;

    *dst_buffer = buffer = NULL;
    pixels = tmp_buf = NULL;
    switch (file_format)
    {
        case D3DXIFF_DDS:
            hr = dds_pixel_format_from_d3dx_pixel_format_id(NULL, dst_format);
            if (FAILED(hr))
                return hr;
            break;

        case D3DXIFF_TGA:
            dst_format = d3dx_get_closest_d3dx_pixel_format_id(tga_save_pixel_formats, ARRAY_SIZE(tga_save_pixel_formats),
                    dst_format);
            break;

        case D3DXIFF_PNG:
            dst_format = d3dx_get_closest_d3dx_pixel_format_id(png_save_pixel_formats, ARRAY_SIZE(png_save_pixel_formats),
                    dst_format);
            break;

        case D3DXIFF_JPG:
            dst_format = d3dx_get_closest_d3dx_pixel_format_id(jpg_save_pixel_formats, ARRAY_SIZE(jpg_save_pixel_formats),
                    dst_format);
            break;

        case D3DXIFF_BMP:
        case D3DXIFF_DIB:
        {
            unsigned int i;

            for (i = 0; i < ARRAY_SIZE(unimplemented_bmp_save_pixel_formats); ++i)
            {
                if (unimplemented_bmp_save_pixel_formats[i] == dst_format)
                {
                    FIXME("Saving pixel format %d to BMP files is currently unsupported.\n", dst_format);
                    return E_NOTIMPL;
                }
            }
            dst_format = d3dx_get_closest_d3dx_pixel_format_id(bmp_save_pixel_formats, ARRAY_SIZE(bmp_save_pixel_formats),
                    dst_format);
            break;
        }

        default:
            assert(0 && "Unexpected file format.");
            return E_FAIL;
    }

    if (dst_format == D3DX_PIXEL_FORMAT_COUNT)
    {
        WARN("Failed to find adequate replacement format for saving.\n");
        return D3DERR_INVALIDCALL;
    }

    if (dst_format != src_fmt_desc->format && !is_conversion_from_supported(src_fmt_desc))
    {
        FIXME("Cannot convert d3dx pixel format %d, can't save.\n", src_fmt_desc->format);
        return E_NOTIMPL;
    }

    dst_fmt_desc = get_d3dx_pixel_format_info(dst_format);
    src_pixels->size.depth = (file_format == D3DXIFF_DDS) ? src_pixels->size.depth : 1;
    hr = d3dx_calculate_pixels_size(dst_format, src_pixels->size.width, src_pixels->size.height, &dst_row_pitch,
            &dst_slice_pitch);
    if (FAILED(hr))
        return hr;

    switch (file_format)
    {
        case D3DXIFF_DDS:
        {
            struct dds_header *header;
            uint32_t header_size;

            header_size = is_index_format(dst_fmt_desc) ? sizeof(*header) + DDS_PALETTE_SIZE : sizeof(*header);
            hr = D3DXCreateBuffer((dst_slice_pitch * src_pixels->size.depth) + header_size, &buffer);
            if (FAILED(hr))
                return hr;

            header = ID3DXBuffer_GetBufferPointer(buffer);
            pixels = (uint8_t *)ID3DXBuffer_GetBufferPointer(buffer) + header_size;
            hr = d3dx_init_dds_header(header, D3DRTYPE_TEXTURE, dst_format, &src_pixels->size, 1);
            if (FAILED(hr))
                goto exit;
            if (is_index_format(dst_fmt_desc))
                memcpy((uint8_t *)ID3DXBuffer_GetBufferPointer(buffer) + sizeof(*header), src_pixels->palette,
                        DDS_PALETTE_SIZE);
            break;
        }

        case D3DXIFF_TGA:
        {
            struct tga_header *header;

            hr = D3DXCreateBuffer(dst_slice_pitch + sizeof(*header), &buffer);
            if (FAILED(hr))
                return hr;

            header = ID3DXBuffer_GetBufferPointer(buffer);
            pixels = (uint8_t *)ID3DXBuffer_GetBufferPointer(buffer) + sizeof(*header);

            memset(header, 0, sizeof(*header));
            header->image_type = TGA_IMAGETYPE_TRUECOLOR;
            header->width = src_pixels->size.width;
            header->height = src_pixels->size.height;
            header->image_descriptor = TGA_IMAGE_TOPTOBOTTOM;
            header->depth = dst_fmt_desc->bytes_per_pixel * 8;
            if (dst_fmt_desc->format == D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM)
                header->image_descriptor |= 0x08;
             break;
        }

        case D3DXIFF_PNG:
        case D3DXIFF_JPG:
        case D3DXIFF_BMP:
        case D3DXIFF_DIB:
            if (src_fmt_desc == dst_fmt_desc)
                dst_pixels = *src_pixels;
            else
                pixels = tmp_buf = malloc(dst_slice_pitch);
            break;

        default:
            break;
    }

    if (src_pixels->size.width != 0 && src_pixels->size.height != 0)
    {
        if (pixels)
        {
            const PALETTEENTRY *dst_palette = is_index_format(dst_fmt_desc) ? src_pixels->palette : NULL;
            const RECT dst_rect = { 0, 0, src_pixels->size.width, src_pixels->size.height };

            set_d3dx_pixels(&dst_pixels, pixels, dst_row_pitch, dst_slice_pitch, dst_palette,
                    src_pixels->size.width, src_pixels->size.height, src_pixels->size.depth, &dst_rect);

            hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_fmt_desc, src_pixels, src_fmt_desc, D3DX_FILTER_NONE, 0);
            if (FAILED(hr))
                goto exit;
        }

        /* WIC path, encode the image. */
        if (!buffer)
        {
            IStream *wic_file = NULL;
            uint32_t buf_size = 0;

            hr = d3dx_pixels_save_wic(&dst_pixels, dst_fmt_desc, file_format, &wic_file, &buf_size);
            if (FAILED(hr))
                goto exit;
            hr = D3DXCreateBuffer(buf_size, &buffer);
            if (FAILED(hr))
            {
                IStream_Release(wic_file);
                goto exit;
            }

            hr = IStream_Read(wic_file, ID3DXBuffer_GetBufferPointer(buffer), buf_size, NULL);
            IStream_Release(wic_file);
            if (FAILED(hr))
                goto exit;
        }
    }
    /* Return an empty buffer for size 0 images via WIC. */
    else if (!buffer)
    {
        FIXME("Returning empty buffer for size 0 image.\n");
        hr = D3DXCreateBuffer(64, &buffer);
        if (FAILED(hr))
            goto exit;
    }

    *dst_buffer = buffer;
exit:
    free(tmp_buf);
    if (*dst_buffer != buffer)
        ID3DXBuffer_Release(buffer);
    return hr;
}

static const uint8_t bmp_file_signature[] =       { 'B', 'M' };
static const uint8_t jpg_file_signature[] =       { 0xff, 0xd8 };
static const uint8_t png_file_signature[] =       { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
static const uint8_t dds_file_signature[] =       { 'D', 'D', 'S', ' ' };
static const uint8_t ppm_plain_file_signature[] = { 'P', '3' };
static const uint8_t ppm_raw_file_signature[] =   { 'P', '6' };
static const uint8_t hdr_file_signature[] =       { '#', '?', 'R', 'A', 'D', 'I', 'A', 'N', 'C', 'E', '\n' };
static const uint8_t pfm_color_file_signature[] = { 'P', 'F' };
static const uint8_t pfm_gray_file_signature[] =  { 'P', 'f' };

/*
 * If none of these match, the file is either DIB, TGA, or something we don't
 * support.
 */
struct d3dx_file_format_signature
{
    const uint8_t *file_signature;
    uint32_t file_signature_len;
    D3DXIMAGE_FILEFORMAT image_file_format;
};

static const struct d3dx_file_format_signature file_format_signatures[] =
{
    { bmp_file_signature,       sizeof(bmp_file_signature),       D3DXIFF_BMP },
    { jpg_file_signature,       sizeof(jpg_file_signature),       D3DXIFF_JPG },
    { png_file_signature,       sizeof(png_file_signature),       D3DXIFF_PNG },
    { dds_file_signature,       sizeof(dds_file_signature),       D3DXIFF_DDS },
    { ppm_plain_file_signature, sizeof(ppm_plain_file_signature), D3DXIFF_PPM },
    { ppm_raw_file_signature,   sizeof(ppm_raw_file_signature),   D3DXIFF_PPM },
    { hdr_file_signature,       sizeof(hdr_file_signature),       D3DXIFF_HDR },
    { pfm_color_file_signature, sizeof(pfm_color_file_signature), D3DXIFF_PFM },
    { pfm_gray_file_signature,  sizeof(pfm_gray_file_signature),  D3DXIFF_PFM },
};

static BOOL d3dx_get_image_file_format_from_file_signature(const void *src_data, uint32_t src_data_size,
        D3DXIMAGE_FILEFORMAT *out_iff)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(file_format_signatures); ++i)
    {
        const struct d3dx_file_format_signature *signature = &file_format_signatures[i];

        if ((src_data_size >= signature->file_signature_len)
                && !memcmp(src_data, signature->file_signature, signature->file_signature_len))
        {
            *out_iff = signature->image_file_format;
            return TRUE;
        }
    }

    return FALSE;
}

static HRESULT d3dx_initialize_image_from_dds(const void *src_data, uint32_t src_data_size,
        struct d3dx_image *image, uint32_t starting_mip_level)
{
    uint32_t expected_src_data_size, header_size;
    const struct dds_header *header = src_data;
    BOOL is_indexed_fmt;
    HRESULT hr;

    if (src_data_size < sizeof(*header) || header->pixel_format.size != sizeof(header->pixel_format))
        return D3DXERR_INVALIDDATA;

    TRACE("File type is DDS.\n");
    is_indexed_fmt = !!(header->pixel_format.flags & DDS_PF_INDEXED);
    header_size = is_indexed_fmt ? sizeof(*header) + DDS_PALETTE_SIZE : sizeof(*header);

    set_volume_struct(&image->size, header->width, header->height, 1);
    image->mip_levels = header->miplevels ? header->miplevels : 1;
    image->format = d3dx_pixel_format_id_from_dds_pixel_format(&header->pixel_format);
    image->layer_count = 1;

    if (image->format == D3DX_PIXEL_FORMAT_COUNT)
        return D3DXERR_INVALIDDATA;

    TRACE("Pixel format is %#x.\n", image->format);
    if (header->flags & DDS_DEPTH)
    {
        image->size.depth = max(header->depth, 1);
        image->resource_type = D3DRTYPE_VOLUMETEXTURE;
    }
    else if (header->caps2 & DDS_CAPS2_CUBEMAP)
    {
        if ((header->caps2 & DDS_CAPS2_CUBEMAP_ALL_FACES) != DDS_CAPS2_CUBEMAP_ALL_FACES)
        {
            WARN("Tried to load a partial cubemap DDS file.\n");
            return D3DXERR_INVALIDDATA;
        }

        image->layer_count = 6;
        image->resource_type = D3DRTYPE_CUBETEXTURE;
    }
    else
        image->resource_type = D3DRTYPE_TEXTURE;

    image->layer_pitch = d3dx_calculate_layer_pixels_size(image->format, image->size.width, image->size.height,
            image->size.depth, image->mip_levels);
    if (!image->layer_pitch)
        return D3DXERR_INVALIDDATA;
    expected_src_data_size = (image->layer_pitch * image->layer_count) + header_size;
    if (src_data_size < expected_src_data_size)
    {
        WARN("File is too short %u, expected at least %u bytes.\n", src_data_size, expected_src_data_size);
        return D3DXERR_INVALIDDATA;
    }

    image->palette = (is_indexed_fmt) ? (PALETTEENTRY *)(((uint8_t *)src_data) + sizeof(*header)) : NULL;
    image->pixels = ((BYTE *)src_data) + header_size;
    image->image_file_format = D3DXIFF_DDS;
    if (starting_mip_level && (image->mip_levels > 1))
    {
        uint32_t i, row_pitch, slice_pitch, initial_mip_levels;
        const struct volume initial_size = image->size;

        initial_mip_levels = image->mip_levels;
        for (i = 0; i < starting_mip_level; i++)
        {
            hr = d3dx_calculate_pixels_size(image->format, image->size.width, image->size.height, &row_pitch, &slice_pitch);
            if (FAILED(hr))
                return hr;

            image->pixels += slice_pitch * image->size.depth;
            d3dx_get_next_mip_level_size(&image->size);
            if (--image->mip_levels == 1)
                break;
        }

        TRACE("Requested starting mip level %u, actual starting mip level is %u (of %u total in image).\n",
                starting_mip_level, (initial_mip_levels - image->mip_levels), initial_mip_levels);
        TRACE("Original dimensions %s, new dimensions %s.\n", debug_volume(&initial_size), debug_volume(&image->size));
    }

    return D3D_OK;
}

static BOOL convert_dib_to_bmp(const void **data, unsigned int *size)
{
    ULONG header_size;
    ULONG count = 0;
    ULONG offset;
    BITMAPFILEHEADER *header;
    BYTE *new_data;
    UINT new_size;

    if ((*size < 4) || (*size < (header_size = *(ULONG*)*data)))
        return FALSE;

    if ((header_size == sizeof(BITMAPINFOHEADER)) ||
        (header_size == sizeof(BITMAPV4HEADER)) ||
        (header_size == sizeof(BITMAPV5HEADER)) ||
        (header_size == 64 /* sizeof(BITMAPCOREHEADER2) */))
    {
        /* All structures begin with the same memory layout as BITMAPINFOHEADER */
        BITMAPINFOHEADER *info_header = (BITMAPINFOHEADER*)*data;
        count = info_header->biClrUsed;

        if (!count && info_header->biBitCount <= 8)
            count = 1 << info_header->biBitCount;

        offset = sizeof(BITMAPFILEHEADER) + header_size + sizeof(RGBQUAD) * count;

        /* For BITMAPINFOHEADER with BI_BITFIELDS compression, there are 3 additional color masks after header */
        if ((info_header->biSize == sizeof(BITMAPINFOHEADER)) && (info_header->biCompression == BI_BITFIELDS))
            offset += 3 * sizeof(DWORD);
    }
    else if (header_size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core_header = (BITMAPCOREHEADER*)*data;

        if (core_header->bcBitCount <= 8)
            count = 1 << core_header->bcBitCount;

        offset = sizeof(BITMAPFILEHEADER) + header_size + sizeof(RGBTRIPLE) * count;
    }
    else
    {
        return FALSE;
    }

    TRACE("Converting DIB file to BMP\n");

    new_size = *size + sizeof(BITMAPFILEHEADER);
    new_data = malloc(new_size);
    CopyMemory(new_data + sizeof(BITMAPFILEHEADER), *data, *size);

    /* Add BMP header */
    header = (BITMAPFILEHEADER*)new_data;
    header->bfType = 0x4d42; /* BM */
    header->bfSize = new_size;
    header->bfReserved1 = 0;
    header->bfReserved2 = 0;
    header->bfOffBits = offset;

    /* Update input data */
    *data = new_data;
    *size = new_size;

    return TRUE;
}

/* windowscodecs always returns xRGB, but we should return ARGB if and only if
 * at least one pixel has a non-zero alpha component. */
static BOOL image_is_argb(IWICBitmapFrameDecode *frame, struct d3dx_image *image)
{
    unsigned int size, i;
    BYTE *buffer;
    HRESULT hr;

    if (image->format != D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM || image->image_file_format != D3DXIFF_BMP)
        return FALSE;

    size = image->size.width * image->size.height * 4;
    if (!(buffer = malloc(size)))
        return FALSE;

    if (FAILED(hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, image->size.width * 4, size, buffer)))
    {
        ERR("Failed to copy pixels, hr %#lx.\n", hr);
        free(buffer);
        return FALSE;
    }

    for (i = 0; i < image->size.width * image->size.height; ++i)
    {
        if (buffer[i * 4 + 3])
        {
            free(buffer);
            return TRUE;
        }
    }

    free(buffer);
    return FALSE;
}

const char *debug_d3dx_image_file_format(D3DXIMAGE_FILEFORMAT format)
{
    switch (format)
    {
#define FMT_TO_STR(format) case format: return #format
        FMT_TO_STR(D3DXIFF_BMP);
        FMT_TO_STR(D3DXIFF_JPG);
        FMT_TO_STR(D3DXIFF_TGA);
        FMT_TO_STR(D3DXIFF_PNG);
        FMT_TO_STR(D3DXIFF_DDS);
        FMT_TO_STR(D3DXIFF_PPM);
        FMT_TO_STR(D3DXIFF_DIB);
        FMT_TO_STR(D3DXIFF_HDR);
        FMT_TO_STR(D3DXIFF_PFM);
#undef FMT_TO_STR
        default:
            return "unrecognized";
    }
}

static HRESULT d3dx_image_wic_frame_decode(struct d3dx_image *image,
        IWICImagingFactory *wic_factory, IWICBitmapFrameDecode *bitmap_frame)
{
    const struct pixel_format_desc *fmt_desc;
    uint32_t row_pitch, slice_pitch;
    IWICPalette *wic_palette = NULL;
    PALETTEENTRY *palette = NULL;
    WICColor *colors = NULL;
    BYTE *buffer = NULL;
    HRESULT hr;

    fmt_desc = get_d3dx_pixel_format_info(image->format);
    hr = d3dx_calculate_pixels_size(image->format, image->size.width, image->size.height, &row_pitch, &slice_pitch);
    if (FAILED(hr))
        return hr;

    /* Allocate a buffer for our image. */
    if (!(buffer = malloc(slice_pitch)))
        return E_OUTOFMEMORY;

    hr = IWICBitmapFrameDecode_CopyPixels(bitmap_frame, NULL, row_pitch, slice_pitch, buffer);
    if (FAILED(hr))
    {
        free(buffer);
        return hr;
    }

    if (is_index_format(fmt_desc))
    {
        uint32_t nb_colors, i;

        hr = IWICImagingFactory_CreatePalette(wic_factory, &wic_palette);
        if (FAILED(hr))
            goto exit;

        hr = IWICBitmapFrameDecode_CopyPalette(bitmap_frame, wic_palette);
        if (FAILED(hr))
            goto exit;

        hr = IWICPalette_GetColorCount(wic_palette, &nb_colors);
        if (FAILED(hr))
            goto exit;

        colors = malloc(nb_colors * sizeof(colors[0]));
        palette = malloc(256 * sizeof(palette[0]));
        if (!colors || !palette)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        hr = IWICPalette_GetColors(wic_palette, nb_colors, colors, &nb_colors);
        if (FAILED(hr))
            goto exit;

        /* Convert colors from WICColor (ARGB) to PALETTEENTRY (ABGR) */
        for (i = 0; i < nb_colors; i++)
        {
            palette[i].peRed   = (colors[i] >> 16) & 0xff;
            palette[i].peGreen = (colors[i] >> 8) & 0xff;
            palette[i].peBlue  = colors[i] & 0xff;
            palette[i].peFlags = (colors[i] >> 24) & 0xff; /* peFlags is the alpha component in DX8 and higher */
        }
        if (nb_colors < 256)
            memset(&palette[nb_colors], 0xff, sizeof(*palette) * (256 - nb_colors));
    }

    image->image_buf = image->pixels = buffer;
    image->image_palette = image->palette = palette;

exit:
    free(colors);
    if (image->image_buf != buffer)
        free(buffer);
    if (image->image_palette != palette)
        free(palette);
    if (wic_palette)
        IWICPalette_Release(wic_palette);

    return hr;
}

static HRESULT d3dx_initialize_image_from_wic(const void *src_data, uint32_t src_data_size,
        struct d3dx_image *image, D3DXIMAGE_FILEFORMAT d3dx_file_format, uint32_t flags)
{
    const GUID *container_format_guid = wic_container_guid_from_d3dx_file_format(d3dx_file_format);
    IWICBitmapFrameDecode *bitmap_frame = NULL;
    IWICBitmapDecoder *bitmap_decoder = NULL;
    IWICImagingFactory *wic_factory;
    WICPixelFormatGUID pixel_format;
    IWICStream *wic_stream = NULL;
    uint32_t frame_count = 0;
    HRESULT hr;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &wic_factory);
    if (FAILED(hr))
        return hr;

    hr = IWICImagingFactory_CreateDecoder(wic_factory, container_format_guid, NULL, &bitmap_decoder);
    if (FAILED(hr))
        goto exit;

    hr = IWICImagingFactory_CreateStream(wic_factory, &wic_stream);
    if (FAILED(hr))
        goto exit;

    hr = IWICStream_InitializeFromMemory(wic_stream, (BYTE *)src_data, src_data_size);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapDecoder_Initialize(bitmap_decoder, (IStream *)wic_stream, 0);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapDecoder_GetFrameCount(bitmap_decoder, &frame_count);
    if (FAILED(hr) || (SUCCEEDED(hr) && !frame_count))
    {
        hr = D3DXERR_INVALIDDATA;
        goto exit;
    }

    image->image_file_format = d3dx_file_format;
    hr = IWICBitmapDecoder_GetFrame(bitmap_decoder, 0, &bitmap_frame);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapFrameDecode_GetSize(bitmap_frame, &image->size.width, &image->size.height);
    if (FAILED(hr))
        goto exit;

    hr = IWICBitmapFrameDecode_GetPixelFormat(bitmap_frame, &pixel_format);
    if (FAILED(hr))
        goto exit;

    image->format = d3dx_pixel_format_id_from_wic_pixel_format(&pixel_format);
    switch (image->format)
    {
        case D3DX_PIXEL_FORMAT_P2_UINT:
            if (image->image_file_format != D3DXIFF_BMP)
                break;
            /* Fall through. */

        case D3DX_PIXEL_FORMAT_COUNT:
            WARN("Unsupported pixel format %s.\n", debugstr_guid(&pixel_format));
            hr = D3DXERR_INVALIDDATA;
            goto exit;

        default:
            break;
    }

    if (image_is_argb(bitmap_frame, image))
        image->format = D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM;

    if (!(flags & D3DX_IMAGE_INFO_ONLY))
    {
        hr = d3dx_image_wic_frame_decode(image, wic_factory, bitmap_frame);
        if (FAILED(hr))
            goto exit;
    }

    image->size.depth = 1;
    image->mip_levels = 1;
    image->layer_count = 1;
    image->resource_type = D3DRTYPE_TEXTURE;

exit:
    if (bitmap_frame)
        IWICBitmapFrameDecode_Release(bitmap_frame);
    if (bitmap_decoder)
        IWICBitmapDecoder_Release(bitmap_decoder);
    if (wic_stream)
        IWICStream_Release(wic_stream);
    IWICImagingFactory_Release(wic_factory);

    return hr;
}

static enum d3dx_pixel_format_id d3dx_get_tga_format_for_bpp(uint8_t bpp)
{
    switch (bpp)
    {
        case 15: return D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM;
        case 16: return D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM;
        case 24: return D3DX_PIXEL_FORMAT_B8G8R8_UNORM;
        case 32: return D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM;
        default:
            WARN("Unhandled bpp %u for targa.\n", bpp);
            return D3DX_PIXEL_FORMAT_COUNT;
    }
}

static HRESULT d3dx_image_tga_rle_decode_row(const uint8_t **src, uint32_t src_bytes_left, uint32_t row_width,
        uint32_t bytes_per_pixel, uint8_t *dst_row)
{
    const uint8_t *src_ptr = *src;
    uint32_t pixel_count = 0;

    while (pixel_count != row_width)
    {
        uint32_t rle_count = (src_ptr[0] & 0x7f) + 1;
        uint32_t rle_packet_size = 1;

        rle_packet_size += (src_ptr[0] & 0x80) ? bytes_per_pixel : (bytes_per_pixel * rle_count);
        if ((rle_packet_size > src_bytes_left) || (pixel_count + rle_count) > row_width)
            return D3DXERR_INVALIDDATA;

        if (src_ptr[0] & 0x80)
        {
            uint32_t i;

            for (i = 0; i < rle_count; ++i)
                memcpy(&dst_row[(pixel_count + i) * bytes_per_pixel], src_ptr + 1, bytes_per_pixel);
        }
        else
        {
            memcpy(&dst_row[pixel_count * bytes_per_pixel], src_ptr + 1, rle_packet_size - 1);
        }

        src_ptr += rle_packet_size;
        src_bytes_left -= rle_packet_size;
        pixel_count += rle_count;
        if (!src_bytes_left && pixel_count != row_width)
            return D3DXERR_INVALIDDATA;
    }

    *src = src_ptr;
    return D3D_OK;
}

struct d3dx_color_key;
static void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
        const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
        const struct volume *dst_size, const struct pixel_format_desc *dst_format, const struct d3dx_color_key *color_key,
        const PALETTEENTRY *palette);
static HRESULT d3dx_image_tga_decode(const void *src_data, uint32_t src_data_size, uint32_t src_header_size,
        struct d3dx_image *image)
{
    const struct pixel_format_desc *fmt_desc = get_d3dx_pixel_format_info(image->format);
    const struct tga_header *header = (const struct tga_header *)src_data;
    const BOOL right_to_left = !!(header->image_descriptor & TGA_IMAGE_RIGHTTOLEFT);
    const BOOL bottom_to_top = !(header->image_descriptor & TGA_IMAGE_TOPTOBOTTOM);
    const BOOL is_rle = !!(header->image_type & TGA_IMAGETYPE_RLE);
    uint8_t *img_buf = NULL, *src_row = NULL;
    uint32_t row_pitch, slice_pitch, i;
    PALETTEENTRY *palette = NULL;
    const uint8_t *src_pos;
    HRESULT hr;

    hr = d3dx_calculate_pixels_size(image->format, image->size.width, image->size.height, &row_pitch, &slice_pitch);
    if (FAILED(hr))
        return hr;

    /* File is too small. */
    if (!is_rle && (src_header_size + slice_pitch) > src_data_size)
        return D3DXERR_INVALIDDATA;

    if (image->format == D3DX_PIXEL_FORMAT_P8_UINT)
    {
        const uint8_t *src_palette = ((const uint8_t *)src_data) + sizeof(*header) + header->id_length;
        const struct volume image_map_size = { header->color_map_length, 1, 1 };
        uint32_t src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch;
        const struct pixel_format_desc *src_desc, *dst_desc;

        if (!(palette = malloc(sizeof(*palette) * 256)))
            return E_OUTOFMEMORY;

        /*
         * Convert from a TGA colormap to PALETTEENTRY. TGA is BGRA,
         * PALETTEENTRY is RGBA.
         */
        src_desc = get_d3dx_pixel_format_info(d3dx_get_tga_format_for_bpp(header->color_map_entrysize));
        hr = d3dx_calculate_pixels_size(src_desc->format, header->color_map_length, 1, &src_row_pitch, &src_slice_pitch);
        if (FAILED(hr))
            goto exit;

        dst_desc = get_format_info(D3DFMT_A8B8G8R8);
        d3dx_calculate_pixels_size(dst_desc->format, 256, 1, &dst_row_pitch, &dst_slice_pitch);
        convert_argb_pixels(src_palette, src_row_pitch, src_slice_pitch, &image_map_size, src_desc, (BYTE *)palette,
                dst_row_pitch, dst_slice_pitch, &image_map_size, dst_desc, NULL, NULL);

        /* Initialize unused palette entries to 0xff. */
        if (header->color_map_length < 256)
            memset(&palette[header->color_map_length], 0xff, sizeof(*palette) * (256 - header->color_map_length));
    }

    if (!is_rle && !bottom_to_top && !right_to_left)
    {
        image->pixels = (uint8_t *)src_data + src_header_size;
        image->image_palette = image->palette = palette;
        return D3D_OK;
    }

    if (!(img_buf = malloc(slice_pitch)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    /* Allocate an extra row to use as a temporary buffer. */
    if (is_rle)
    {
        if (!(src_row = malloc(row_pitch)))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }

    src_pos = (const uint8_t *)src_data + src_header_size;
    for (i = 0; i < image->size.height; ++i)
    {
        const uint32_t dst_row_idx = bottom_to_top ? (image->size.height - i - 1) : i;
        uint8_t *dst_row = img_buf + (dst_row_idx * row_pitch);

        if (is_rle)
        {
            hr = d3dx_image_tga_rle_decode_row(&src_pos, src_data_size - (src_pos - (const uint8_t *)src_data),
                    image->size.width, fmt_desc->bytes_per_pixel, src_row);
            if (FAILED(hr))
                goto exit;
        }
        else
        {
            src_row = (uint8_t *)src_pos;
            src_pos += row_pitch;
        }

        if (right_to_left)
        {
            const uint8_t *src_pixel = &src_row[((image->size.width - 1)) * fmt_desc->bytes_per_pixel];
            uint8_t *dst_pixel = dst_row;
            uint32_t j;

            for (j = 0; j < image->size.width; ++j)
            {
                memcpy(dst_pixel, src_pixel, fmt_desc->bytes_per_pixel);
                src_pixel -= fmt_desc->bytes_per_pixel;
                dst_pixel += fmt_desc->bytes_per_pixel;
            }
        }
        else
        {
            memcpy(dst_row, src_row, row_pitch);
        }
    }

    image->image_buf = image->pixels = img_buf;
    image->image_palette = image->palette = palette;

exit:
    if (is_rle)
        free(src_row);
    if (img_buf && (image->image_buf != img_buf))
        free(img_buf);
    if (palette && (image->image_palette != palette))
        free(palette);

    return hr;
}

static HRESULT d3dx_initialize_image_from_tga(const void *src_data, uint32_t src_data_size, struct d3dx_image *image,
        uint32_t flags)
{
    const struct tga_header *header = (const struct tga_header *)src_data;
    uint32_t expected_header_size = sizeof(*header);

    if (src_data_size < sizeof(*header))
        return D3DXERR_INVALIDDATA;

    expected_header_size += header->id_length;
    expected_header_size += header->color_map_length * ((header->color_map_entrysize + 7) / CHAR_BIT);
    if (src_data_size < expected_header_size)
        return D3DXERR_INVALIDDATA;

    if (header->color_map_type && ((header->color_map_type > 1) || (!header->color_map_length)
                || (d3dx_get_tga_format_for_bpp(header->color_map_entrysize) == D3DX_PIXEL_FORMAT_COUNT)))
        return D3DXERR_INVALIDDATA;

    switch (header->image_type & TGA_IMAGETYPE_MASK)
    {
        case TGA_IMAGETYPE_COLORMAPPED:
            if (header->depth != 8 || !header->color_map_type)
                return D3DXERR_INVALIDDATA;
            image->format = D3DX_PIXEL_FORMAT_P8_UINT;
            break;

        case TGA_IMAGETYPE_TRUECOLOR:
            if ((image->format = d3dx_get_tga_format_for_bpp(header->depth)) == D3DX_PIXEL_FORMAT_COUNT)
                return D3DXERR_INVALIDDATA;
            break;

        case TGA_IMAGETYPE_GRAYSCALE:
            if (header->depth != 8)
                return D3DXERR_INVALIDDATA;
            image->format = D3DX_PIXEL_FORMAT_L8_UNORM;
            break;

        default:
            return D3DXERR_INVALIDDATA;
    }

    set_volume_struct(&image->size, header->width, header->height, 1);
    image->mip_levels = 1;
    image->layer_count = 1;
    image->resource_type = D3DRTYPE_TEXTURE;
    image->image_file_format = D3DXIFF_TGA;

    if (!(flags & D3DX_IMAGE_INFO_ONLY))
        return d3dx_image_tga_decode(src_data, src_data_size, expected_header_size, image);

    return D3D_OK;
}

HRESULT d3dx_image_init(const void *src_data, uint32_t src_data_size, struct d3dx_image *image,
        uint32_t starting_mip_level, uint32_t flags)
{
    D3DXIMAGE_FILEFORMAT iff = D3DXIFF_FORCE_DWORD;
    HRESULT hr;

    if (!src_data || !src_data_size || !image)
        return D3DERR_INVALIDCALL;

    memset(image, 0, sizeof(*image));
    if (!d3dx_get_image_file_format_from_file_signature(src_data, src_data_size, &iff))
    {
        uint32_t src_image_size = src_data_size;
        const void *src_image = src_data;

        if (convert_dib_to_bmp(&src_image, &src_image_size))
        {
            hr = d3dx_image_init(src_image, src_image_size, image, starting_mip_level, flags);
            free((void *)src_image);
            if (SUCCEEDED(hr))
                image->image_file_format = D3DXIFF_DIB;
            return hr;
        }

        /* Last resort, try TGA. */
        return d3dx_initialize_image_from_tga(src_data, src_data_size, image, flags);
    }

    switch (iff)
    {
        case D3DXIFF_BMP:
        case D3DXIFF_JPG:
        case D3DXIFF_PNG:
            hr = d3dx_initialize_image_from_wic(src_data, src_data_size, image, iff, flags);
            break;

        case D3DXIFF_DDS:
            hr = d3dx_initialize_image_from_dds(src_data, src_data_size, image, starting_mip_level);
            break;

        case D3DXIFF_PPM:
        case D3DXIFF_HDR:
        case D3DXIFF_PFM:
            WARN("Unsupported file format %s.\n", debug_d3dx_image_file_format(iff));
            hr = E_NOTIMPL;
            break;

        case D3DXIFF_FORCE_DWORD:
            ERR("Unrecognized file format.\n");
            hr = D3DXERR_INVALIDDATA;
            break;

        default:
            assert(0);
            return E_FAIL;
    }

    return hr;
}

void d3dx_image_cleanup(struct d3dx_image *image)
{
    free(image->image_buf);
    free(image->image_palette);
}

HRESULT d3dx_image_get_pixels(struct d3dx_image *image, uint32_t layer, uint32_t mip_level,
        struct d3dx_pixels *pixels)
{
    struct volume mip_level_size = image->size;
    const BYTE *pixels_ptr = image->pixels;
    uint32_t row_pitch, slice_pitch, i;
    RECT unaligned_rect;
    HRESULT hr = S_OK;

    if (mip_level >= image->mip_levels)
    {
        ERR("Tried to retrieve mip level %u, but image only has %u mip levels.\n", mip_level, image->mip_levels);
        return E_FAIL;
    }

    if (layer >= image->layer_count)
    {
        ERR("Tried to retrieve layer %u, but image only has %u layers.\n", layer, image->layer_count);
        return E_FAIL;
    }

    slice_pitch = row_pitch = 0;
    for (i = 0; i < image->mip_levels; i++)
    {
        hr = d3dx_calculate_pixels_size(image->format, mip_level_size.width, mip_level_size.height, &row_pitch, &slice_pitch);
        if (FAILED(hr))
            return hr;

        if (i == mip_level)
            break;

        pixels_ptr += slice_pitch * mip_level_size.depth;
        d3dx_get_next_mip_level_size(&mip_level_size);
    }

    pixels_ptr += (layer * image->layer_pitch);
    SetRect(&unaligned_rect, 0, 0, mip_level_size.width, mip_level_size.height);
    set_d3dx_pixels(pixels, pixels_ptr, row_pitch, slice_pitch, image->palette, mip_level_size.width,
            mip_level_size.height, mip_level_size.depth, &unaligned_rect);

    return D3D_OK;
}

void d3dximage_info_from_d3dx_image(D3DXIMAGE_INFO *info, struct d3dx_image *image)
{
    info->ImageFileFormat = image->image_file_format;
    info->Width = image->size.width;
    info->Height = image->size.height;
    info->Depth = image->size.depth;
    info->MipLevels = image->mip_levels;
    switch (image->format)
    {
        case D3DX_PIXEL_FORMAT_P1_UINT:
        case D3DX_PIXEL_FORMAT_P2_UINT:
        case D3DX_PIXEL_FORMAT_P4_UINT:
            info->Format = D3DFMT_P8;
            break;

        case D3DX_PIXEL_FORMAT_R16G16B16_UNORM:
            info->Format = D3DFMT_A16B16G16R16;
            break;

        case D3DX_PIXEL_FORMAT_B8G8R8_UNORM:
            if (info->ImageFileFormat == D3DXIFF_PNG || info->ImageFileFormat == D3DXIFF_JPG)
                info->Format = D3DFMT_X8R8G8B8;
            else
                info->Format = d3dformat_from_d3dx_pixel_format_id(image->format);
            break;

        default:
            info->Format = d3dformat_from_d3dx_pixel_format_id(image->format);
            break;
    }
    info->ResourceType = image->resource_type;
}

/************************************************************
 * D3DXGetImageInfoFromFileInMemory
 *
 * Fills a D3DXIMAGE_INFO structure with info about an image
 *
 * PARAMS
 *   data     [I] pointer to the image file data
 *   datasize [I] size of the passed data
 *   info     [O] pointer to the destination structure
 *
 * RETURNS
 *   Success: D3D_OK, if info is not NULL and data and datasize make up a valid image file or
 *                    if info is NULL and data and datasize are not NULL
 *   Failure: D3DXERR_INVALIDDATA, if data is no valid image file and datasize and info are not NULL
 *            D3DERR_INVALIDCALL, if data is NULL or
 *                                if datasize is 0
 *
 * NOTES
 *   datasize may be bigger than the actual file size
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileInMemory(const void *data, UINT datasize, D3DXIMAGE_INFO *info)
{
    struct d3dx_image image;
    HRESULT hr;

    TRACE("(%p, %d, %p)\n", data, datasize, info);

    if (!data || !datasize)
        return D3DERR_INVALIDCALL;

    if (!info)
        return D3D_OK;

    hr = d3dx_image_init(data, datasize, &image, 0, D3DX_IMAGE_INFO_ONLY);
    if (FAILED(hr)) {
        TRACE("Invalid or unsupported image file\n");
        return D3DXERR_INVALIDDATA;
    }

    d3dximage_info_from_d3dx_image(info, &image);
    return D3D_OK;
}

/************************************************************
 * D3DXGetImageInfoFromFile
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load a valid image file or
 *                    if we successfully load a file which is no valid image and info is NULL
 *   Failure: D3DXERR_INVALIDDATA, if we fail to load file or
 *                                 if file is not a valid image file and info is not NULL
 *            D3DERR_INVALIDCALL, if file is NULL
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromFileA(const char *file, D3DXIMAGE_INFO *info)
{
    WCHAR *widename;
    HRESULT hr;
    int strlength;

    TRACE("file %s, info %p.\n", debugstr_a(file), info);

    if( !file ) return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, file, -1, NULL, 0);
    widename = malloc(strlength * sizeof(*widename));
    MultiByteToWideChar(CP_ACP, 0, file, -1, widename, strlength);

    hr = D3DXGetImageInfoFromFileW(widename, info);
    free(widename);

    return hr;
}

HRESULT WINAPI D3DXGetImageInfoFromFileW(const WCHAR *file, D3DXIMAGE_INFO *info)
{
    void *buffer;
    HRESULT hr;
    DWORD size;

    TRACE("file %s, info %p.\n", debugstr_w(file), info);

    if (!file)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(file, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXGetImageInfoFromFileInMemory(buffer, size, info);
    UnmapViewOfFile(buffer);

    return hr;
}

/************************************************************
 * D3DXGetImageInfoFromResource
 *
 * RETURNS
 *   Success: D3D_OK, if resource is a valid image file
 *   Failure: D3DXERR_INVALIDDATA, if resource is no valid image file or NULL or
 *                                 if we fail to load resource
 *
 */
HRESULT WINAPI D3DXGetImageInfoFromResourceA(HMODULE module, const char *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_a(resource), info);

    if (!(resinfo = FindResourceA(module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

HRESULT WINAPI D3DXGetImageInfoFromResourceW(HMODULE module, const WCHAR *resource, D3DXIMAGE_INFO *info)
{
    HRSRC resinfo;
    void *buffer;
    DWORD size;

    TRACE("module %p, resource %s, info %p.\n", module, debugstr_w(resource), info);

    if (!(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(module, resinfo, &buffer, &size)))
        return D3DXERR_INVALIDDATA;

    return D3DXGetImageInfoFromFileInMemory(buffer, size, info);
}

static HRESULT d3dx_load_surface_from_memory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        enum d3dx_pixel_format_id src_format, uint32_t src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_desc, *dst_desc;
    struct d3dx_pixels src_pixels, dst_pixels;
    RECT dst_rect_tmp, dst_rect_aligned;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT lock_rect;
    D3DSURFACE_DESC desc;
    HRESULT hr;

    IDirect3DSurface9_GetDesc(dst_surface, &desc);
    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        TRACE("Multisampled destination surface, doing nothing.\n");
        return D3D_OK;
    }

    dst_desc = get_format_info(desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_tmp, 0, 0, desc.Width, desc.Height);
        dst_rect = &dst_rect_tmp;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > desc.Width
                || dst_rect->top > dst_rect->bottom || dst_rect->bottom > desc.Height
                || dst_rect->left < 0 || dst_rect->top < 0)
        {
            WARN("Invalid dst_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
        if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
        {
            WARN("Empty dst_rect specified.\n");
            return D3D_OK;
        }
    }

    src_desc = get_d3dx_pixel_format_info(src_format);
    hr = d3dx_pixels_init(src_memory, src_pitch, 0, src_palette, src_desc->format,
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1, &src_pixels);
    if (FAILED(hr))
        return hr;

    get_aligned_rect(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, desc.Width, desc.Height,
        dst_desc, &dst_rect_aligned);
    if (FAILED(hr = lock_surface(dst_surface, &dst_rect_aligned, &lock_rect, &surface, TRUE)))
        return hr;

    set_d3dx_pixels(&dst_pixels, lock_rect.pBits, lock_rect.Pitch, 0, dst_palette,
            (dst_rect_aligned.right - dst_rect_aligned.left), (dst_rect_aligned.bottom - dst_rect_aligned.top), 1,
            dst_rect);
    OffsetRect(&dst_pixels.unaligned_rect, -dst_rect_aligned.left, -dst_rect_aligned.top);

    if (FAILED(hr = d3dx_load_pixels_from_pixels(&dst_pixels, dst_desc, &src_pixels, src_desc, filter, color_key)))
    {
        unlock_surface(dst_surface, &dst_rect_aligned, surface, FALSE);
        return hr;
    }

    return unlock_surface(dst_surface, &dst_rect_aligned, surface, TRUE);
}

/************************************************************
 * D3DXLoadSurfaceFromFileInMemory
 *
 * Loads data from a given buffer into a surface and fills a given
 * D3DXIMAGE_INFO structure with info about the source data.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcData     [I] pointer to the source data
 *   SrcDataSize  [I] size of the source data in bytes
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *   pSrcInfo     [O] pointer to a D3DXIMAGE_INFO structure
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, pSrcData or SrcDataSize is NULL
 *            D3DXERR_INVALIDDATA, if pSrcData is no valid image file
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromFileInMemory(IDirect3DSurface9 *pDestSurface,
        const PALETTEENTRY *pDestPalette, const RECT *pDestRect, const void *pSrcData, UINT SrcDataSize,
        const RECT *pSrcRect, DWORD dwFilter, D3DCOLOR Colorkey, D3DXIMAGE_INFO *pSrcInfo)
{
    struct d3dx_pixels pixels = { 0 };
    struct d3dx_image image;
    D3DXIMAGE_INFO img_info;
    RECT src_rect;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_data %p, src_data_size %u, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            pDestSurface, pDestPalette, wine_dbgstr_rect(pDestRect), pSrcData, SrcDataSize,
            wine_dbgstr_rect(pSrcRect), dwFilter, Colorkey, pSrcInfo);

    if (!pDestSurface || !pSrcData || !SrcDataSize)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&dwFilter)))
        return hr;

    hr = d3dx_image_init(pSrcData, SrcDataSize, &image, 0, 0);
    if (FAILED(hr))
        return D3DXERR_INVALIDDATA;

    d3dximage_info_from_d3dx_image(&img_info, &image);
    if (pSrcRect)
        src_rect = *pSrcRect;
    else
        SetRect(&src_rect, 0, 0, img_info.Width, img_info.Height);

    hr = d3dx_image_get_pixels(&image, 0, 0, &pixels);
    if (FAILED(hr))
        goto exit;

    hr = d3dx_load_surface_from_memory(pDestSurface, pDestPalette, pDestRect, pixels.data, image.format, pixels.row_pitch,
            pixels.palette, &src_rect, dwFilter, Colorkey);
    if (SUCCEEDED(hr) && pSrcInfo)
        *pSrcInfo = img_info;

exit:
    d3dx_image_cleanup(&image);
    return FAILED(hr) ? D3DXERR_INVALIDDATA : D3D_OK;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const char *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    WCHAR *src_file_w;
    HRESULT hr;
    int strlength;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_a(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    strlength = MultiByteToWideChar(CP_ACP, 0, src_file, -1, NULL, 0);
    src_file_w = malloc(strlength * sizeof(*src_file_w));
    MultiByteToWideChar(CP_ACP, 0, src_file, -1, src_file_w, strlength);

    hr = D3DXLoadSurfaceFromFileW(dst_surface, dst_palette, dst_rect,
            src_file_w, src_rect, filter, color_key, src_info);
    free(src_file_w);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromFileW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const WCHAR *src_file,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    void *data;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_file %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), debugstr_w(src_file),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!src_file || !dst_surface)
        return D3DERR_INVALIDCALL;

    if (FAILED(map_view_of_file(src_file, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
    UnmapViewOfFile(data);

    return hr;
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceA(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const char *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_a(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceA(src_module, resource, (const char *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceA(src_module, resource, (const char *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}

HRESULT WINAPI D3DXLoadSurfaceFromResourceW(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, HMODULE src_module, const WCHAR *resource,
        const RECT *src_rect, DWORD filter, D3DCOLOR color_key, D3DXIMAGE_INFO *src_info)
{
    DWORD data_size;
    HRSRC resinfo;
    void *data;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_module %p, resource %s, "
            "src_rect %s, filter %#lx, color_key 0x%08lx, src_info %p.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_module, debugstr_w(resource),
            wine_dbgstr_rect(src_rect), filter, color_key, src_info);

    if (!dst_surface)
        return D3DERR_INVALIDCALL;

    if (!(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_RCDATA))
            /* Try loading the resource as bitmap data (which is in DIB format D3DXIFF_DIB) */
            && !(resinfo = FindResourceW(src_module, resource, (const WCHAR *)RT_BITMAP)))
        return D3DXERR_INVALIDDATA;

    if (FAILED(load_resource_into_memory(src_module, resinfo, &data, &data_size)))
        return D3DXERR_INVALIDDATA;

    return D3DXLoadSurfaceFromFileInMemory(dst_surface, dst_palette, dst_rect,
            data, data_size, src_rect, filter, color_key, src_info);
}


/************************************************************
 * helper functions for D3DXLoadSurfaceFromMemory
 */
struct argb_conversion_info
{
    const struct pixel_format_desc *srcformat;
    const struct pixel_format_desc *destformat;
    DWORD srcshift[4], destshift[4];
    DWORD srcmask[4], destmask[4];
    BOOL process_channel[4];
    DWORD channelmask;
};

static void init_argb_conversion_info(const struct pixel_format_desc *srcformat, const struct pixel_format_desc *destformat, struct argb_conversion_info *info)
{
    UINT i;
    ZeroMemory(info->process_channel, 4 * sizeof(BOOL));
    info->channelmask = 0;

    info->srcformat  =  srcformat;
    info->destformat = destformat;

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        info->srcshift[i]  =  srcformat->shift[i] + max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        info->destshift[i] = destformat->shift[i] + max(destformat->bits[i] -  srcformat->bits[i], 0);

        info->srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        info->destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) info->process_channel[i] = TRUE;
            else info->channelmask |= info->destmask[i];
        }
    }
}

/************************************************************
 * get_relevant_argb_components
 *
 * Extracts the relevant components from the source color and
 * drops the less significant bits if they aren't used by the destination format.
 */
static void get_relevant_argb_components(const struct argb_conversion_info *info, const BYTE *col, DWORD *out)
{
    unsigned int i, j;
    unsigned int component, mask;

    for (i = 0; i < 4; ++i)
    {
        if (!info->process_channel[i])
            continue;

        component = 0;
        mask = info->srcmask[i];
        for (j = 0; j < 4 && mask; ++j)
        {
            if (info->srcshift[i] < j * 8)
                component |= (col[j] & mask) << (j * 8 - info->srcshift[i]);
            else
                component |= (col[j] & mask) >> (info->srcshift[i] - j * 8);
            mask >>= 8;
        }
        out[i] = component;
    }
}

static float d3dx_clamp(float value, float min_value, float max_value)
{
    if (isnan(value))
        return max_value;
    return value < min_value ? min_value : value > max_value ? max_value : value;
}

/************************************************************
 * make_argb_color
 *
 * Recombines the output of get_relevant_argb_components and converts
 * it to the destination format.
 */
static DWORD make_argb_color(const struct argb_conversion_info *info, const DWORD *in)
{
    UINT i;
    DWORD val = 0;

    for(i = 0;i < 4;i++) {
        if(info->process_channel[i]) {
            /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
            signed int shift;
            for(shift = info->destshift[i]; shift > info->destformat->shift[i]; shift -= info->srcformat->bits[i]) val |= in[i] << shift;
            val |= (in[i] >> (info->destformat->shift[i] - shift)) << info->destformat->shift[i];
        }
    }
    val |= info->channelmask;   /* new channels are set to their maximal value */
    return val;
}

static enum range get_range_for_component_type(enum component_type type)
{
    switch (type)
    {
        case CTYPE_SNORM:
            return RANGE_SNORM;

        case CTYPE_LUMA:
        case CTYPE_INDEX:
        case CTYPE_UNORM:
            return RANGE_UNORM;

        case CTYPE_EMPTY:
        case CTYPE_FLOAT:
            return RANGE_FULL;

        default:
            assert(0);
            return RANGE_FULL;
    }
}

/* It doesn't work for components bigger than 32 bits (or somewhat smaller but unaligned). */
void format_to_d3dx_color(const struct pixel_format_desc *format, const BYTE *src, const PALETTEENTRY *palette,
        struct d3dx_color *dst)
{
    DWORD mask, tmp;
    unsigned int c;

    dst->rgb_range = get_range_for_component_type(format->rgb_type);
    dst->a_range = get_range_for_component_type(format->a_type);
    for (c = 0; c < 4; ++c)
    {
        const enum component_type dst_ctype = !c ? format->a_type : format->rgb_type;
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        float *dst_component = &dst->value.x + component_offsets[c];

        if (format->bits[c])
        {
            mask = ~0u >> (32 - format->bits[c]);

            memcpy(&tmp, src + format->shift[c] / 8,
                    min(sizeof(DWORD), (format->shift[c] % 8 + format->bits[c] + 7) / 8));
            tmp = (tmp >> (format->shift[c] % 8)) & mask;

            switch (dst_ctype)
            {
            case CTYPE_FLOAT:
                if (format->bits[c] == 16)
                    *dst_component = float_16_to_32(tmp);
                else
                    *dst_component = *(float *)&tmp;
                break;

            case CTYPE_INDEX:
                *dst_component = (&palette[tmp].peRed)[component_offsets[c]] / 255.0f;
                break;

            case CTYPE_LUMA:
            case CTYPE_UNORM:
                *dst_component = (float)tmp / mask;
                break;

            case CTYPE_SNORM:
            {
                const uint32_t sign_bit = (1u << (format->bits[c] - 1));
                uint32_t tmp_extended = (tmp & sign_bit) ? (tmp | ~(sign_bit - 1)) : tmp;

                /*
                 * In order to clamp to an even range, we need to ignore
                 * the maximum negative value.
                 */
                if (tmp == sign_bit)
                    tmp_extended |= 1;

                *dst_component = (float)(((int32_t)tmp_extended)) / (sign_bit - 1);
                break;
            }

            default:
                break;
            }
        }
        else if (dst_ctype == CTYPE_LUMA)
        {
            assert(format->bits[1]);
            *dst_component = dst->value.x;
        }
        else
        {
            *dst_component = 1.0f;
        }
    }
}

/* It doesn't work for components bigger than 32 bits. */
void format_from_d3dx_color(const struct pixel_format_desc *format, const struct d3dx_color *src, BYTE *dst)
{
    DWORD v, mask32;
    unsigned int c, i;

    memset(dst, 0, format->bytes_per_pixel);

    for (c = 0; c < 4; ++c)
    {
        const enum component_type dst_ctype = !c ? format->a_type : format->rgb_type;
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        const float src_component = *(&src->value.x + component_offsets[c]);
        const enum range src_range = !c ? src->a_range : src->rgb_range;

        if (!format->bits[c])
            continue;

        mask32 = ~0u >> (32 - format->bits[c]);

        switch (dst_ctype)
        {
        case CTYPE_FLOAT:
            if (format->bits[c] == 16)
                v = float_32_to_16(src_component);
            else
                v = *(DWORD *)&src_component;
            break;

        case CTYPE_LUMA:
        {
            float val = src->value.x * 0.2125f + src->value.y * 0.7154f + src->value.z * 0.0721f;

            if (src_range == RANGE_SNORM)
                val = (val + 1.0f) / 2.0f;

            v = d3dx_clamp(val, 0.0f, 1.0f) * ((1u << format->bits[c]) - 1) + 0.5f;
            break;
        }

        case CTYPE_UNORM:
        {
            float val = src_component;

            if (src_range == RANGE_SNORM)
                val = (val + 1.0f) / 2.0f;

            v = d3dx_clamp(val, 0.0f, 1.0f) * ((1u << format->bits[c]) - 1) + 0.5f;
            break;
        }

        case CTYPE_SNORM:
        {
            const uint32_t max_value = (1u << (format->bits[c] - 1)) - 1;
            float val = src_component;

            if (src_range == RANGE_UNORM)
                val = (val * 2.0f) - 1.0f;

            v = d3dx_clamp(val, -1.0f, 1.0f) * max_value + 0.5f;
            break;
        }

        /* We shouldn't be trying to output to CTYPE_INDEX. */
        case CTYPE_INDEX:
            assert(0);
            break;

        default:
            v = 0;
            break;
        }

        for (i = format->shift[c] / 8 * 8; i < format->shift[c] + format->bits[c]; i += 8)
        {
            BYTE mask, byte;

            if (format->shift[c] > i)
            {
                mask = mask32 << (format->shift[c] - i);
                byte = (v << (format->shift[c] - i)) & mask;
            }
            else
            {
                mask = mask32 >> (i - format->shift[c]);
                byte = (v >> (i - format->shift[c])) & mask;
            }
            dst[i / 8] |= byte;
        }
    }
}

struct d3dx_color_key
{
    uint8_t color_key_min[4];
    uint8_t color_key_max[4];
};

static const char *debug_d3dx_color_key(const struct d3dx_color_key *color_key)
{
    if (!color_key)
        return "(null)";
    return wine_dbg_sprintf("(0x%02x->0x%02x)-(0x%02x->0x%02x)-(0x%02x->0x%02x)-(0x%02x->0x%02x)",
            color_key->color_key_min[0], color_key->color_key_max[0],
            color_key->color_key_min[1], color_key->color_key_max[1],
            color_key->color_key_min[2], color_key->color_key_max[2],
            color_key->color_key_min[3], color_key->color_key_max[3]);
}

static void d3dx_init_color_key(const struct pixel_format_desc *src_fmt, uint32_t color_key,
        struct d3dx_color_key *color_key_out)
{
    unsigned int i;

    for (i = 0; i < 4; ++i)
    {
        const enum component_type src_ctype = !i ? src_fmt->a_type : src_fmt->rgb_type;
        const uint8_t channel_bits = (src_ctype == CTYPE_LUMA) ? src_fmt->bits[1] : src_fmt->bits[i];
        const uint8_t ck_channel = (color_key >> (24 - (i * 8))) & 0xff;
        float slop, channel_conv, unique_values;

        if (!channel_bits)
        {
            color_key_out->color_key_min[i] = 0x00;
            color_key_out->color_key_max[i] = 0xff;
            continue;
        }

        /*
         * If the source format channel can represent all unique channel
         * values in the color key, no extra processing is necessary.
         */
        if (src_ctype == CTYPE_FLOAT || (src_ctype == CTYPE_SNORM && channel_bits > 8)
                || (src_ctype != CTYPE_SNORM && channel_bits >= 8))
        {
            color_key_out->color_key_min[i] = color_key_out->color_key_max[i] = ck_channel;
            continue;
        }

        channel_conv = ck_channel / 255.0f;
        if (src_ctype == CTYPE_SNORM)
        {
            const uint32_t max_value = (1u << (channel_bits - 1)) - 1;

            unique_values = (1u << channel_bits) - 2;
            channel_conv = (channel_conv * 2.0f) - 1.0f;
            channel_conv = rintf(channel_conv * max_value) / max_value;
            channel_conv = (channel_conv + 1.0f) / 2.0f;
        }
        else
        {
            unique_values = (1u << channel_bits) - 1;
            channel_conv = rintf(channel_conv * unique_values) / unique_values;
        }

        channel_conv = channel_conv * 255.0f;
        slop = (255.0f - unique_values) / unique_values / 2.0f;
        color_key_out->color_key_min[i] = rintf(d3dx_clamp(channel_conv - slop, 0.0f, 255.0f));
        color_key_out->color_key_max[i] = rintf(d3dx_clamp(channel_conv + slop, 0.0f, 255.0f));
    }
}

/************************************************************
 * copy_pixels
 *
 * Copies the source buffer to the destination buffer.
 * Works for any pixel format.
 * The source and the destination must be block-aligned.
 */
static void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
        BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
        const struct pixel_format_desc *format)
{
    UINT row, slice;
    BYTE *dst_addr;
    const BYTE *src_addr;
    UINT row_block_count = (size->width + format->block_width - 1) / format->block_width;
    UINT row_count = (size->height + format->block_height - 1) / format->block_height;

    for (slice = 0; slice < size->depth; slice++)
    {
        src_addr = src + slice * src_slice_pitch;
        dst_addr = dst + slice * dst_slice_pitch;

        for (row = 0; row < row_count; row++)
        {
            memcpy(dst_addr, src_addr, row_block_count * format->block_byte_count);
            src_addr += src_row_pitch;
            dst_addr += dst_row_pitch;
        }
    }
}

static void convert_argb_pixel(const uint8_t *src_ptr, const struct pixel_format_desc *src_fmt,
        uint8_t *dst_ptr, const struct pixel_format_desc *dst_fmt, const PALETTEENTRY *palette,
        struct argb_conversion_info *conv_info, const struct d3dx_color_key *color_key,
        const struct pixel_format_desc *ck_format, struct argb_conversion_info *ck_conv_info)
{
    unsigned int i;

    if (format_types_match(src_fmt, dst_fmt) && src_fmt->bytes_per_pixel <= 4 && dst_fmt->bytes_per_pixel <= 4)
    {
        DWORD channels[4];
        DWORD val;

        get_relevant_argb_components(conv_info, src_ptr, channels);
        val = make_argb_color(conv_info, channels);

        if (color_key)
        {
            DWORD ck_pixel;

            get_relevant_argb_components(ck_conv_info, src_ptr, channels);
            ck_pixel = make_argb_color(ck_conv_info, channels);
            for (i = 0; i < 4; ++i)
            {
                const uint8_t ck_channel = (ck_pixel >> (24 - (i * 8))) & 0xff;

                if ((ck_channel < color_key->color_key_min[i]) || (ck_channel > color_key->color_key_max[i]))
                    break;
            }
            if (i == 4)
                val = 0;
        }
        memcpy(dst_ptr, &val, dst_fmt->bytes_per_pixel);
    }
    else
    {
        struct d3dx_color color, tmp;

        format_to_d3dx_color(src_fmt, src_ptr, palette, &color);
        tmp = color;

        if (color_key)
        {
            DWORD ck_pixel = 0;

            format_from_d3dx_color(ck_format, &tmp, (BYTE *)&ck_pixel);
            for (i = 0; i < 4; ++i)
            {
                const uint8_t ck_channel = (ck_pixel >> (24 - (i * 8))) & 0xff;

                if ((ck_channel < color_key->color_key_min[i]) || (ck_channel > color_key->color_key_max[i]))
                    break;
            }
            if (i == 4)
                tmp.value.x = tmp.value.y = tmp.value.z = tmp.value.w = 0.0f;
        }

        color = tmp;
        format_from_d3dx_color(dst_fmt, &color, dst_ptr);
    }
}

/************************************************************
 * convert_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Pixels outsize the source rect are blacked out.
 */
static void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
        const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
        const struct volume *dst_size, const struct pixel_format_desc *dst_format, const struct d3dx_color_key *color_key,
        const PALETTEENTRY *palette)
{
    /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
    const struct pixel_format_desc *ck_format = color_key ? get_format_info(D3DFMT_A8R8G8B8) : NULL;
    struct argb_conversion_info conv_info, ck_conv_info;
    UINT min_width, min_height, min_depth;
    UINT x, y, z;

    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
            "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key %s, palette %p.\n",
            src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
            dst_format, debug_d3dx_color_key(color_key), palette);

    init_argb_conversion_info(src_format, dst_format, &conv_info);

    min_width = min(src_size->width, dst_size->width);
    min_height = min(src_size->height, dst_size->height);
    min_depth = min(src_size->depth, dst_size->depth);

    if (color_key)
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);

    for (z = 0; z < min_depth; z++) {
        const BYTE *src_slice_ptr = src + z * src_slice_pitch;
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;

        for (y = 0; y < min_height; y++) {
            const BYTE *src_ptr = src_slice_ptr + y * src_row_pitch;
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;

            for (x = 0; x < min_width; x++) {
                convert_argb_pixel(src_ptr, src_format, dst_ptr, dst_format, palette,
                        &conv_info, color_key, ck_format, &ck_conv_info);

                src_ptr += src_format->bytes_per_pixel;
                dst_ptr += dst_format->bytes_per_pixel;
            }

            if (src_size->width < dst_size->width) /* black out remaining pixels */
                memset(dst_ptr, 0, dst_format->bytes_per_pixel * (dst_size->width - src_size->width));
        }

        if (src_size->height < dst_size->height) /* black out remaining pixels */
            memset(dst + src_size->height * dst_row_pitch, 0, dst_row_pitch * (dst_size->height - src_size->height));
    }
    if (src_size->depth < dst_size->depth) /* black out remaining pixels */
        memset(dst + src_size->depth * dst_slice_pitch, 0, dst_slice_pitch * (dst_size->depth - src_size->depth));
}

/************************************************************
 * point_filter_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion, color keying and stretching
 * using a point filter.
 */
static void point_filter_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
        const struct volume *src_size, const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch,
        UINT dst_slice_pitch, const struct volume *dst_size, const struct pixel_format_desc *dst_format,
        const struct d3dx_color_key *color_key, const PALETTEENTRY *palette)
{
    /* Color keys are always represented in D3DFMT_A8R8G8B8 format. */
    const struct pixel_format_desc *ck_format = color_key ? get_format_info(D3DFMT_A8R8G8B8) : NULL;
    struct argb_conversion_info conv_info, ck_conv_info;
    UINT x, y, z;

    TRACE("src %p, src_row_pitch %u, src_slice_pitch %u, src_size %p, src_format %p, dst %p, "
            "dst_row_pitch %u, dst_slice_pitch %u, dst_size %p, dst_format %p, color_key %s, palette %p.\n",
            src, src_row_pitch, src_slice_pitch, src_size, src_format, dst, dst_row_pitch, dst_slice_pitch, dst_size,
            dst_format, debug_d3dx_color_key(color_key), palette);

    init_argb_conversion_info(src_format, dst_format, &conv_info);

    if (color_key)
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);

    for (z = 0; z < dst_size->depth; z++)
    {
        BYTE *dst_slice_ptr = dst + z * dst_slice_pitch;
        const BYTE *src_slice_ptr = src + src_slice_pitch * (z * src_size->depth / dst_size->depth);

        for (y = 0; y < dst_size->height; y++)
        {
            BYTE *dst_ptr = dst_slice_ptr + y * dst_row_pitch;
            const BYTE *src_row_ptr = src_slice_ptr + src_row_pitch * (y * src_size->height / dst_size->height);

            for (x = 0; x < dst_size->width; x++)
            {
                const BYTE *src_ptr = src_row_ptr + (x * src_size->width / dst_size->width) * src_format->bytes_per_pixel;

                convert_argb_pixel(src_ptr, src_format, dst_ptr, dst_format, palette,
                        &conv_info, color_key, ck_format, &ck_conv_info);
                dst_ptr += dst_format->bytes_per_pixel;
            }
        }
    }
}

static HRESULT d3dx_pixels_decompress(struct d3dx_pixels *pixels, const struct pixel_format_desc *desc,
        BOOL is_dst, void **out_memory, uint32_t *out_row_pitch, uint32_t *out_slice_pitch,
        const struct pixel_format_desc **out_desc)
{
    uint32_t uncompressed_slice_pitch, uncompressed_row_pitch, block_buf_row_pitch, block_width_mask, block_height_mask;
    void (*decompress_bcn_block)(const void *src, void *dst, int dst_row_pitch);
    const struct pixel_format_desc *uncompressed_desc = NULL;
    const struct volume *size = &pixels->size;
    BYTE *uncompressed_mem;
    uint8_t block_buf[64];
    unsigned int x, y, z;
    RECT aligned_rect;

    switch (desc->format)
    {
        case D3DX_PIXEL_FORMAT_DXT1_UNORM:
            uncompressed_desc = get_format_info(D3DFMT_A8B8G8R8);
            decompress_bcn_block = bcdec_bc1;
            break;

        case D3DX_PIXEL_FORMAT_DXT2_UNORM:
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:
            uncompressed_desc = get_format_info(D3DFMT_A8B8G8R8);
            decompress_bcn_block = bcdec_bc2;
            break;

        case D3DX_PIXEL_FORMAT_DXT4_UNORM:
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:
            uncompressed_desc = get_format_info(D3DFMT_A8B8G8R8);
            decompress_bcn_block = bcdec_bc3;
            break;

        default:
            FIXME("Unexpected compressed texture format %u.\n", desc->format);
            return E_NOTIMPL;
    }

    block_width_mask = desc->block_width - 1;
    block_height_mask = desc->block_height - 1;
    block_buf_row_pitch = desc->block_width * uncompressed_desc->bytes_per_pixel;
    uncompressed_row_pitch = size->width * uncompressed_desc->bytes_per_pixel;
    uncompressed_slice_pitch = uncompressed_row_pitch * size->height;
    if (!(uncompressed_mem = malloc(size->depth * uncompressed_slice_pitch)))
        return E_OUTOFMEMORY;

    /*
     * For compressed destination pixels, width/height will represent
     * the entire set of compressed blocks our destination rectangle touches.
     * If we're only updating a sub-area of any blocks, we need to decompress
     * the pixels outside of the sub-area.
     */
    if (is_dst)
    {
        SetRect(&aligned_rect, 0, 0, size->width, size->height);

        /*
         * If our destination covers the entire set of blocks, no
         * decompression needs to be done, just return the allocated memory.
         */
        if (EqualRect(&aligned_rect, &pixels->unaligned_rect))
            goto exit;
    }
    /*
     * For compressed source pixels, width/height will represent the size of
     * the unaligned rectangle. I.e, if we have an 8x8 source with an
     * unaligned rect of (2,2)-(6,6) our width/height will be 4.
     */
    else
    {
        SetRect(&aligned_rect, 0, 0, (pixels->unaligned_rect.right + block_width_mask) & ~block_width_mask,
                (pixels->unaligned_rect.bottom + block_height_mask) & ~block_height_mask);
    }

    TRACE("Decompressing pixels.\n");
    for (z = 0; z < size->depth; ++z)
    {
        const uint8_t *src_slice = &((const uint8_t *)pixels->data)[z * pixels->slice_pitch];
        uint8_t *dst_slice = &uncompressed_mem[z * uncompressed_slice_pitch];

        for (y = 0; y < aligned_rect.bottom; y += desc->block_height)
        {
            const uint8_t *src_ptr = &src_slice[(y / desc->block_height) * pixels->row_pitch];

            for (x = 0; x < aligned_rect.right; x += desc->block_width)
            {
                struct volume dst_block_size;
                RECT src_rect, dst_rect;
                uint8_t *dst_ptr;

                SetRect(&src_rect, x, y, x + desc->block_width, y + desc->block_height);
                IntersectRect(&src_rect, &src_rect, &pixels->unaligned_rect);
                dst_rect = src_rect;
                OffsetRect(&dst_rect, -pixels->unaligned_rect.left, -pixels->unaligned_rect.top);

                set_volume_struct(&dst_block_size, dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top, 1);
                dst_ptr = &dst_slice[(dst_rect.top * uncompressed_row_pitch)];
                dst_ptr += dst_rect.left * uncompressed_desc->bytes_per_pixel;

                if (dst_block_size.width != desc->block_width || dst_block_size.height != desc->block_height)
                {
                    if (!is_dst)
                    {
                        unsigned int block_buf_offset;

                        decompress_bcn_block(src_ptr, block_buf, block_buf_row_pitch);
                        block_buf_offset = (src_rect.top - y) * block_buf_row_pitch;
                        block_buf_offset += uncompressed_desc->bytes_per_pixel * (src_rect.left - x);
                        copy_pixels(&block_buf[block_buf_offset], block_buf_row_pitch, 0, dst_ptr,
                                uncompressed_row_pitch, 0, &dst_block_size, uncompressed_desc);
                    }
                    /*
                     * If this is the destination, we can just copy the whole
                     * block. It will be partially overwritten later.
                     */
                    else
                    {
                        dst_ptr = &dst_slice[y * uncompressed_row_pitch + x * uncompressed_desc->bytes_per_pixel];
                        decompress_bcn_block(src_ptr, dst_ptr, uncompressed_row_pitch);
                    }

                }
                /* Full block copy. */
                else if (!is_dst)
                {
                    decompress_bcn_block(src_ptr, dst_ptr, uncompressed_row_pitch);
                }
                src_ptr += desc->block_byte_count;
            }
        }
    }

exit:
    *out_memory = uncompressed_mem;
    *out_row_pitch = uncompressed_row_pitch;
    *out_slice_pitch = uncompressed_slice_pitch;
    *out_desc = uncompressed_desc;

    return S_OK;
}

static HRESULT d3dx_pixels_unpack_index(struct d3dx_pixels *pixels, const struct pixel_format_desc *desc,
        void **out_memory, uint32_t *out_row_pitch, uint32_t *out_slice_pitch, const struct pixel_format_desc **out_desc)
{
    uint32_t x, y, z, unpacked_slice_pitch, unpacked_row_pitch;
    const struct pixel_format_desc *unpacked_desc = NULL;
    const struct volume *size = &pixels->size;
    uint8_t *unpacked_mem;
    uint8_t mask, shift;

    switch (desc->format)
    {
        case D3DX_PIXEL_FORMAT_P1_UINT:
        case D3DX_PIXEL_FORMAT_P2_UINT:
        case D3DX_PIXEL_FORMAT_P4_UINT:
            unpacked_desc = get_d3dx_pixel_format_info(D3DX_PIXEL_FORMAT_P8_UINT);
            break;

        default:
            FIXME("Unexpected format %u.\n", desc->format);
            return E_NOTIMPL;
    }

    unpacked_row_pitch = size->width * unpacked_desc->bytes_per_pixel;
    unpacked_slice_pitch = unpacked_row_pitch * size->height;
    if (!(unpacked_mem = malloc(size->depth * unpacked_slice_pitch)))
        return E_OUTOFMEMORY;

    shift = 8 / desc->block_width;
    mask = (1u << shift) - 1;

    TRACE("Unpacking pixels.\n");
    for (z = 0; z < size->depth; ++z)
    {
        const uint8_t *slice_data = (const uint8_t *)pixels->data + (pixels->slice_pitch * z);

        for (y = 0; y < size->height; ++y)
        {
            uint8_t *ptr = &unpacked_mem[(z * unpacked_slice_pitch) + (y * unpacked_row_pitch)];
            const uint8_t *row_data = slice_data + (pixels->row_pitch * y);

            for (x = 0; x < size->width; x += desc->block_width)
            {
                const uint8_t packed_data = *row_data;
                unsigned int i;

                for (i = 0; i < desc->block_width; ++i)
                {
                    const uint8_t cur_shift = ((desc->block_width - 1) - i) * shift;

                    if (x + i >= size->width)
                        break;
                    ptr[i] = (packed_data >> cur_shift) & mask;
                }
                ptr += unpacked_desc->bytes_per_pixel * desc->block_width;
                row_data++;
            }
        }
    }

    *out_memory = unpacked_mem;
    *out_row_pitch = unpacked_row_pitch;
    *out_slice_pitch = unpacked_slice_pitch;
    *out_desc = unpacked_desc;

    return S_OK;
}

static void d3dx_compress_block(enum d3dx_pixel_format_id fmt, uint8_t *block_buf, void *dst_buf)
{
    switch (fmt)
    {
        case D3DX_PIXEL_FORMAT_DXT1_UNORM:
            stb_compress_dxt_block(dst_buf, block_buf, FALSE, 0);
            break;

        case D3DX_PIXEL_FORMAT_DXT2_UNORM:
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:
        {
            uint8_t *dst_data_offset = dst_buf;
            unsigned int y;

            /* STB doesn't do DXT2/DXT3, we'll do the alpha part ourselves. */
            for (y = 0; y < 4; ++y)
            {
                uint8_t *tmp_row = &block_buf[y * 4 * 4];

                dst_data_offset[0]  = (tmp_row[7] & 0xf0);
                dst_data_offset[0] |= (tmp_row[3] >> 4);
                dst_data_offset[1]  = (tmp_row[15] & 0xf0);
                dst_data_offset[1] |= (tmp_row[11] >> 4);

                /*
                 * Set all alpha values to 0xff so they aren't considered during
                 * compression. This modifies the source data being passed in.
                 */
                tmp_row[3] = tmp_row[7] = tmp_row[11] = tmp_row[15] = 0xff;
                dst_data_offset += 2;
            }
            stb_compress_dxt_block(dst_data_offset, block_buf, FALSE, 0);
            break;
        }

        case D3DX_PIXEL_FORMAT_DXT4_UNORM:
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:
            stb_compress_dxt_block(dst_buf, block_buf, TRUE, 0);
            break;

        default:
            assert(0);
            break;
    }
}

/*
 * Source data passed into this function is potentially modified (currently
 * only in the case of DXT2/DXT3). As of now we only pass temporary buffers
 * into this function, but this should be taken to account if used elsewhere
 * outside of d3dx_load_pixels_from_pixels() in the future.
 */
static HRESULT d3dx_pixels_compress(struct d3dx_pixels *src_pixels,
        const struct pixel_format_desc *src_desc, struct d3dx_pixels *dst_pixels,
        const struct pixel_format_desc *dst_desc)
{
    unsigned int x, y, z, block_buf_row_pitch;
    uint8_t block_buf[64];

    switch (dst_desc->format)
    {
        case D3DX_PIXEL_FORMAT_DXT1_UNORM:
        case D3DX_PIXEL_FORMAT_DXT2_UNORM:
        case D3DX_PIXEL_FORMAT_DXT3_UNORM:
        case D3DX_PIXEL_FORMAT_DXT4_UNORM:
        case D3DX_PIXEL_FORMAT_DXT5_UNORM:
            assert(src_desc->format == D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM);
            break;

        default:
            FIXME("Unexpected compressed texture format %u.\n", dst_desc->format);
            return E_NOTIMPL;
    }

    TRACE("Compressing pixels.\n");
    block_buf_row_pitch = src_desc->bytes_per_pixel * dst_desc->block_width;
    for (z = 0; z < src_pixels->size.depth; ++z)
    {
        const uint8_t *src_slice = &((const uint8_t *)src_pixels->data)[z * src_pixels->slice_pitch];
        uint8_t *dst_slice = &((uint8_t *)dst_pixels->data)[z * dst_pixels->slice_pitch];

        for (y = 0; y < src_pixels->size.height; y += dst_desc->block_height)
        {
            const unsigned int tmp_src_height = min(dst_desc->block_height, src_pixels->size.height - y);
            uint8_t *dst_ptr = &dst_slice[(y / dst_desc->block_height) * dst_pixels->row_pitch];
            const uint8_t *src_ptr = &src_slice[y * src_pixels->row_pitch];

            for (x = 0; x < src_pixels->size.width; x += dst_desc->block_width)
            {
                const unsigned int tmp_src_width = min(dst_desc->block_width, src_pixels->size.width - x);
                struct volume block_buf_size = { tmp_src_width, tmp_src_height, 1 };

                if (tmp_src_width != dst_desc->block_width || tmp_src_height != dst_desc->block_height)
                    memset(block_buf, 0, sizeof(block_buf));
                copy_pixels(src_ptr, src_pixels->row_pitch, src_pixels->slice_pitch, block_buf, block_buf_row_pitch, 0,
                        &block_buf_size, src_desc);
                d3dx_compress_block(dst_desc->format, block_buf, dst_ptr);
                src_ptr += (src_desc->bytes_per_pixel * dst_desc->block_width);
                dst_ptr += dst_desc->block_byte_count;
            }
        }
    }

    return S_OK;
}

HRESULT d3dx_pixels_init(const void *data, uint32_t row_pitch, uint32_t slice_pitch,
        const PALETTEENTRY *palette, enum d3dx_pixel_format_id format, uint32_t left, uint32_t top, uint32_t right,
        uint32_t bottom, uint32_t front, uint32_t back, struct d3dx_pixels *pixels)
{
    const struct pixel_format_desc *fmt_desc = get_d3dx_pixel_format_info(format);
    const BYTE *ptr = data;
    RECT unaligned_rect;

    memset(pixels, 0, sizeof(*pixels));
    if (is_unknown_format(fmt_desc))
    {
        FIXME("Unsupported format %#x.\n", format);
        return E_NOTIMPL;
    }

    ptr += front * slice_pitch;
    ptr += (top / fmt_desc->block_height) * row_pitch;
    ptr += (left / fmt_desc->block_width) * fmt_desc->block_byte_count;

    if (is_compressed_format(fmt_desc))
    {
        uint32_t left_aligned, top_aligned;

        top_aligned = top & ~(fmt_desc->block_height - 1);
        left_aligned = left & ~(fmt_desc->block_width - 1);
        SetRect(&unaligned_rect, left, top, right, bottom);
        OffsetRect(&unaligned_rect, -left_aligned, -top_aligned);
    }
    else
    {
        SetRect(&unaligned_rect, 0, 0, (right - left), (bottom - top));
    }

    if (!slice_pitch)
        slice_pitch = row_pitch * (bottom - top);
    set_d3dx_pixels(pixels, ptr, row_pitch, slice_pitch, palette, (right - left), (bottom - top), (back - front),
            &unaligned_rect);

    return S_OK;
}

static const char *debug_d3dx_pixels(struct d3dx_pixels *pixels)
{
    if (!pixels)
        return "(null)";
    return wine_dbg_sprintf("(data %p, row_pitch %d, slice_pitch %d, palette %p, width %d, height %d, depth %d, "
            "unaligned_rect %s)", pixels->data, pixels->row_pitch, pixels->slice_pitch, pixels->palette,
            pixels->size.width, pixels->size.height, pixels->size.depth, wine_dbgstr_rect(&pixels->unaligned_rect));
}

HRESULT d3dx_load_pixels_from_pixels(struct d3dx_pixels *dst_pixels,
       const struct pixel_format_desc *dst_desc, struct d3dx_pixels *src_pixels,
       const struct pixel_format_desc *src_desc, uint32_t filter_flags, uint32_t color_key)
{
    struct volume src_size, dst_size, dst_size_aligned;
    const struct d3dx_color_key *d3dx_ck = NULL;
    struct d3dx_color_key d3dx_color_key;
    HRESULT hr = S_OK;

    TRACE("dst_pixels %s, dst_desc %p, src_pixels %s, src_desc %p, filter_flags %#x, color_key %#x.\n",
            debug_d3dx_pixels(dst_pixels), dst_desc, debug_d3dx_pixels(src_pixels), src_desc,
            filter_flags, color_key);

    if (is_compressed_format(src_desc))
        set_volume_struct(&src_size, (src_pixels->unaligned_rect.right - src_pixels->unaligned_rect.left),
                (src_pixels->unaligned_rect.bottom - src_pixels->unaligned_rect.top), src_pixels->size.depth);
    else
        src_size = src_pixels->size;

    dst_size_aligned = dst_pixels->size;
    if (is_compressed_format(dst_desc))
        set_volume_struct(&dst_size, (dst_pixels->unaligned_rect.right - dst_pixels->unaligned_rect.left),
                (dst_pixels->unaligned_rect.bottom - dst_pixels->unaligned_rect.top), dst_pixels->size.depth);
    else
        dst_size = dst_size_aligned;

    /* Everything matches, simply copy the pixels. */
    if (src_desc->format == dst_desc->format
            && (dst_size.width == src_size.width && !(dst_size.width % dst_desc->block_width))
            && (dst_size.height == src_size.height && !(dst_size.height % dst_desc->block_height))
            && (dst_size.depth == src_size.depth)
            && color_key == 0
            && !(src_pixels->unaligned_rect.left & (src_desc->block_width - 1))
            && !(src_pixels->unaligned_rect.top & (src_desc->block_height - 1))
            && !(dst_pixels->unaligned_rect.left & (dst_desc->block_width - 1))
            && !(dst_pixels->unaligned_rect.top & (dst_desc->block_height - 1)))
    {
        TRACE("Simple copy.\n");
        copy_pixels(src_pixels->data, src_pixels->row_pitch, src_pixels->slice_pitch, (void *)dst_pixels->data,
                dst_pixels->row_pitch, dst_pixels->slice_pitch, &src_size, src_desc);
        return S_OK;
    }

    /* Stretching or format conversion. */
    if (!is_conversion_from_supported(src_desc)
            || !is_conversion_to_supported(dst_desc))
    {
        FIXME("Unsupported format conversion %#x -> %#x.\n", src_desc->format, dst_desc->format);
        return E_NOTIMPL;
    }

    if (is_index_format(src_desc) && (src_desc->block_width > 1))
    {
        uint32_t unpacked_row_pitch, unpacked_slice_pitch;
        const struct pixel_format_desc *unpacked_desc;
        void *unpacked_mem = NULL;

        hr = d3dx_pixels_unpack_index(src_pixels, src_desc, &unpacked_mem, &unpacked_row_pitch,
                &unpacked_slice_pitch, &unpacked_desc);
        if (SUCCEEDED(hr))
        {
            struct d3dx_pixels unpacked_pixels;

            d3dx_pixels_init(unpacked_mem, unpacked_row_pitch, unpacked_slice_pitch, src_pixels->palette,
                    unpacked_desc->format, 0, 0, src_pixels->size.width, src_pixels->size.height,
                    0, src_pixels->size.depth, &unpacked_pixels);

            hr = d3dx_load_pixels_from_pixels(dst_pixels, dst_desc, &unpacked_pixels, unpacked_desc,
                    filter_flags, color_key);
        }
        free(unpacked_mem);
        goto exit;
    }

    /*
     * If the source is a compressed image, we need to decompress it first
     * before doing any modifications.
     */
    if (is_compressed_format(src_desc))
    {
        uint32_t uncompressed_row_pitch, uncompressed_slice_pitch;
        const struct pixel_format_desc *uncompressed_desc;
        void *uncompressed_mem = NULL;

        hr = d3dx_pixels_decompress(src_pixels, src_desc, FALSE, &uncompressed_mem, &uncompressed_row_pitch,
                &uncompressed_slice_pitch, &uncompressed_desc);
        if (SUCCEEDED(hr))
        {
            struct d3dx_pixels uncompressed_pixels;

            d3dx_pixels_init(uncompressed_mem, uncompressed_row_pitch, uncompressed_slice_pitch, NULL,
                    uncompressed_desc->format, 0, 0, src_pixels->size.width, src_pixels->size.height,
                    0, src_pixels->size.depth, &uncompressed_pixels);

            if (sizeof(void *) == 4 && color_key)
            {
                TRACE("Clearing color key value on compressed source pixels.\n");
                color_key = 0;
            }
            hr = d3dx_load_pixels_from_pixels(dst_pixels, dst_desc, &uncompressed_pixels, uncompressed_desc,
                    filter_flags, color_key);
        }
        free(uncompressed_mem);
        goto exit;
    }

    /* Same as the above, need to decompress the destination prior to modifying. */
    if (is_compressed_format(dst_desc))
    {
        uint32_t uncompressed_row_pitch, uncompressed_slice_pitch;
        const struct pixel_format_desc *uncompressed_desc;
        struct d3dx_pixels uncompressed_pixels;
        void *uncompressed_mem = NULL;

        hr = d3dx_pixels_decompress(dst_pixels, dst_desc, TRUE, &uncompressed_mem, &uncompressed_row_pitch,
                &uncompressed_slice_pitch, &uncompressed_desc);
        if (FAILED(hr))
            goto exit;

        d3dx_pixels_init(uncompressed_mem, uncompressed_row_pitch, uncompressed_slice_pitch, NULL,
                uncompressed_desc->format, dst_pixels->unaligned_rect.left, dst_pixels->unaligned_rect.top,
                dst_pixels->unaligned_rect.right, dst_pixels->unaligned_rect.bottom, 0, dst_pixels->size.depth,
                &uncompressed_pixels);

        hr = d3dx_load_pixels_from_pixels(&uncompressed_pixels, uncompressed_desc, src_pixels, src_desc, filter_flags,
                color_key);
        if (SUCCEEDED(hr))
        {
            d3dx_pixels_init(uncompressed_mem, uncompressed_row_pitch, uncompressed_slice_pitch, NULL,
                    uncompressed_desc->format, 0, 0, dst_size_aligned.width, dst_size_aligned.height, 0,
                    dst_pixels->size.depth, &uncompressed_pixels);

            hr = d3dx_pixels_compress(&uncompressed_pixels, uncompressed_desc, dst_pixels, dst_desc);
            if (FAILED(hr))
                WARN("Failed to compress pixels, hr %#lx.\n", hr);
        }
        free(uncompressed_mem);
        goto exit;
    }

    if (color_key)
    {
        d3dx_init_color_key(src_desc, color_key, &d3dx_color_key);
        d3dx_ck = &d3dx_color_key;
    }

    if ((filter_flags & 0xf) == D3DX_FILTER_NONE)
    {
        convert_argb_pixels(src_pixels->data, src_pixels->row_pitch, src_pixels->slice_pitch, &src_size, src_desc,
                (BYTE *)dst_pixels->data, dst_pixels->row_pitch, dst_pixels->slice_pitch, &dst_size, dst_desc,
                d3dx_ck, src_pixels->palette);
    }
    else /* if ((filter & 0xf) == D3DX_FILTER_POINT) */
    {
        if ((filter_flags & 0xf) != D3DX_FILTER_POINT)
            FIXME("Unhandled filter %#x.\n", filter_flags);

        /* Always apply a point filter until D3DX_FILTER_LINEAR,
         * D3DX_FILTER_TRIANGLE and D3DX_FILTER_BOX are implemented. */
        point_filter_argb_pixels(src_pixels->data, src_pixels->row_pitch, src_pixels->slice_pitch, &src_size,
                src_desc, (BYTE *)dst_pixels->data, dst_pixels->row_pitch, dst_pixels->slice_pitch, &dst_size,
                dst_desc, d3dx_ck, src_pixels->palette);
    }

exit:
    if (FAILED(hr))
        WARN("Failed to load pixels, hr %#lx.\n", hr);
    return hr;
}

void get_aligned_rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t width, uint32_t height,
        const struct pixel_format_desc *fmt_desc, RECT *aligned_rect)
{
    SetRect(aligned_rect, left, top, right, bottom);
    if (aligned_rect->left & (fmt_desc->block_width - 1))
        aligned_rect->left = aligned_rect->left & ~(fmt_desc->block_width - 1);
    if (aligned_rect->top & (fmt_desc->block_height - 1))
        aligned_rect->top = aligned_rect->top & ~(fmt_desc->block_height - 1);
    if (aligned_rect->right & (fmt_desc->block_width - 1) && aligned_rect->right != width)
        aligned_rect->right = min((aligned_rect->right + fmt_desc->block_width - 1)
                & ~(fmt_desc->block_width - 1), width);
    if (aligned_rect->bottom & (fmt_desc->block_height - 1) && aligned_rect->bottom != height)
        aligned_rect->bottom = min((aligned_rect->bottom + fmt_desc->block_height - 1)
                & ~(fmt_desc->block_height - 1), height);
}

/************************************************************
 * D3DXLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: D3D_OK, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than D3DFMT_UNKNOWN) or
 *                                if DestRect is invalid
 *            D3DXERR_INVALIDDATA, if we fail to lock pDestSurface
 *            E_FAIL, if SrcFormat is D3DFMT_UNKNOWN or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromMemory(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, const void *src_memory,
        D3DFORMAT src_format, UINT src_pitch, const PALETTEENTRY *src_palette, const RECT *src_rect,
        DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_desc;
    HRESULT hr;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_memory %p, src_format %#x, "
            "src_pitch %u, src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_memory, src_format,
            src_pitch, src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_memory || !src_rect)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    if (src_format == D3DFMT_UNKNOWN
            || src_rect->left >= src_rect->right
            || src_rect->top >= src_rect->bottom)
    {
        WARN("Invalid src_format or src_rect.\n");
        return E_FAIL;
    }

    src_desc = get_format_info(src_format);
    if (is_unknown_format(src_desc))
    {
        FIXME("Unsupported format %#x.\n", src_format);
        return E_NOTIMPL;
    }

    return d3dx_load_surface_from_memory(dst_surface, dst_palette, dst_rect, src_memory, src_desc->format, src_pitch,
            src_palette, src_rect, filter, color_key);
}

/************************************************************
 * D3DXLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: D3D_OK
 *   Failure: D3DERR_INVALIDCALL, if pDestSurface or pSrcSurface is NULL
 *            D3DXERR_INVALIDDATA, if one of the surfaces is not lockable
 *
 */
HRESULT WINAPI D3DXLoadSurfaceFromSurface(IDirect3DSurface9 *dst_surface,
        const PALETTEENTRY *dst_palette, const RECT *dst_rect, IDirect3DSurface9 *src_surface,
        const PALETTEENTRY *src_palette, const RECT *src_rect, DWORD filter, D3DCOLOR color_key)
{
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;
    D3DSURFACE_DESC src_desc, dst_desc;
    struct volume src_size, dst_size;
    IDirect3DSurface9 *temp_surface;
    D3DTEXTUREFILTERTYPE d3d_filter;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lock;
    RECT dst_rect_temp;
    HRESULT hr;
    RECT s;

    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
            "src_palette %p, src_rect %s, filter %#lx, color_key 0x%08lx.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
            src_palette, wine_dbgstr_rect(src_rect), filter, color_key);

    if (!dst_surface || !src_surface)
        return D3DERR_INVALIDCALL;

    if (FAILED(hr = d3dx9_handle_load_filter(&filter)))
        return hr;

    IDirect3DSurface9_GetDesc(src_surface, &src_desc);
    src_format_desc = get_format_info(src_desc.Format);
    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }
    else if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
    {
        WARN("Empty src_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (src_rect->left > src_rect->right || src_rect->right > src_desc.Width
            || src_rect->left < 0 || src_rect->left > src_desc.Width
            || src_rect->top > src_rect->bottom || src_rect->bottom > src_desc.Height
            || src_rect->top < 0 || src_rect->top > src_desc.Height)
    {
        WARN("Invalid src_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    IDirect3DSurface9_GetDesc(dst_surface, &dst_desc);
    dst_format_desc = get_format_info(dst_desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_temp, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &dst_rect_temp;
    }
    else if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
    {
        WARN("Empty dst_rect specified.\n");
        return filter == D3DX_FILTER_NONE ? D3D_OK : E_FAIL;
    }
    else if (dst_rect->left > dst_rect->right || dst_rect->right > dst_desc.Width
            || dst_rect->left < 0 || dst_rect->left > dst_desc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > dst_desc.Height
            || dst_rect->top < 0 || dst_rect->top > dst_desc.Height)
    {
        WARN("Invalid dst_rect specified.\n");
        return D3DERR_INVALIDCALL;
    }

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;

    if (!dst_palette && !src_palette && !color_key)
    {
        if (src_desc.Format == dst_desc.Format
                && dst_size.width == src_size.width
                && dst_size.height == src_size.height
                && color_key == 0
                && !(src_rect->left & (src_format_desc->block_width - 1))
                && !(src_rect->top & (src_format_desc->block_height - 1))
                && !(dst_rect->left & (dst_format_desc->block_width - 1))
                && !(dst_rect->top & (dst_format_desc->block_height - 1)))
        {
            d3d_filter = D3DTEXF_NONE;
        }
        else
        {
            switch (filter)
            {
                case D3DX_FILTER_NONE:
                    d3d_filter = D3DTEXF_NONE;
                    break;

                case D3DX_FILTER_POINT:
                    d3d_filter = D3DTEXF_POINT;
                    break;

                case D3DX_FILTER_LINEAR:
                    d3d_filter = D3DTEXF_LINEAR;
                    break;

                default:
                    d3d_filter = D3DTEXF_FORCE_DWORD;
                    break;
            }
        }

        if (d3d_filter != D3DTEXF_FORCE_DWORD)
        {
            IDirect3DSurface9_GetDevice(src_surface, &device);
            hr = IDirect3DDevice9_StretchRect(device, src_surface, src_rect, dst_surface, dst_rect, d3d_filter);
            IDirect3DDevice9_Release(device);
            if (SUCCEEDED(hr))
                return D3D_OK;
        }
    }

    if (FAILED(lock_surface(src_surface, NULL, &lock, &temp_surface, FALSE)))
        return D3DXERR_INVALIDDATA;

    hr = D3DXLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
            src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (FAILED(unlock_surface(src_surface, NULL, temp_surface, FALSE)))
        return D3DXERR_INVALIDDATA;

    return hr;
}


HRESULT WINAPI D3DXSaveSurfaceToFileA(const char *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    int len;
    WCHAR *filename;
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
            wine_dbgstr_a(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    len = MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, NULL, 0);
    filename = malloc(len * sizeof(WCHAR));
    if (!filename) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, dst_filename, -1, filename, len);

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    free(filename);
    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileW(const WCHAR *dst_filename, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    HRESULT hr;
    ID3DXBuffer *buffer;

    TRACE("(%s, %#x, %p, %p, %s): relay\n",
        wine_dbgstr_w(dst_filename), file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_filename) return D3DERR_INVALIDCALL;

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, file_format, src_surface, src_palette, src_rect);
    if (SUCCEEDED(hr))
    {
        hr = write_buffer_to_file(dst_filename, buffer);
        ID3DXBuffer_Release(buffer);
    }

    return hr;
}

HRESULT WINAPI D3DXSaveSurfaceToFileInMemory(ID3DXBuffer **dst_buffer, D3DXIMAGE_FILEFORMAT file_format,
        IDirect3DSurface9 *src_surface, const PALETTEENTRY *src_palette, const RECT *src_rect)
{
    const struct pixel_format_desc *src_fmt_desc;
    D3DSURFACE_DESC src_surface_desc;
    IDirect3DSurface9 *temp_surface;
    struct d3dx_pixels src_pixels;
    D3DLOCKED_RECT locked_rect;
    ID3DXBuffer *buffer;
    RECT src_rect_temp;
    HRESULT hr;

    TRACE("dst_buffer %p, file_format %#x, src_surface %p, src_palette %p, src_rect %s.\n",
        dst_buffer, file_format, src_surface, src_palette, wine_dbgstr_rect(src_rect));

    if (!dst_buffer || !src_surface || file_format > D3DXIFF_PFM)
        return D3DERR_INVALIDCALL;

    switch (file_format)
    {
        case D3DXIFF_HDR:
        case D3DXIFF_PFM:
        case D3DXIFF_PPM:
            FIXME("File format %#x is not supported yet.\n", file_format);
            return E_NOTIMPL;

        default:
            break;
    }

    IDirect3DSurface9_GetDesc(src_surface, &src_surface_desc);
    src_fmt_desc = get_format_info(src_surface_desc.Format);
    if (is_unknown_format(src_fmt_desc))
        return E_NOTIMPL;

    if (!src_palette && is_index_format(src_fmt_desc))
    {
        FIXME("Default palette unimplemented.\n");
        return E_NOTIMPL;
    }

    if (src_rect)
    {
        if (src_rect->left > src_rect->right || src_rect->right > src_surface_desc.Width
                || src_rect->top > src_rect->bottom || src_rect->bottom > src_surface_desc.Height
                || src_rect->left < 0 || src_rect->top < 0)
        {
            WARN("Invalid src_rect specified.\n");
            return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        SetRect(&src_rect_temp, 0, 0, src_surface_desc.Width, src_surface_desc.Height);
        src_rect = &src_rect_temp;
    }

    hr = lock_surface(src_surface, NULL, &locked_rect, &temp_surface, FALSE);
    if (FAILED(hr))
        return hr;

    hr = d3dx_pixels_init(locked_rect.pBits, locked_rect.Pitch, 0, src_palette, src_fmt_desc->format,
            src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1, &src_pixels);
    if (FAILED(hr))
    {
        unlock_surface(src_surface, NULL, temp_surface, FALSE);
        return hr;
    }

    hr = d3dx_save_pixels_to_memory(&src_pixels, src_fmt_desc, file_format, &buffer);
    if (FAILED(hr))
    {
        unlock_surface(src_surface, NULL, temp_surface, FALSE);
        return hr;
    }

    hr = unlock_surface(src_surface, NULL, temp_surface, FALSE);
    if (FAILED(hr))
    {
        ID3DXBuffer_Release(buffer);
        return hr;
    }

    *dst_buffer = buffer;
    return D3D_OK;
}
