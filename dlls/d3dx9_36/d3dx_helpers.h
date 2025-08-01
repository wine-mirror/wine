/*
 * Copyright 2025 Connor McAdams for CodeWeavers
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

#ifndef __WINE_D3DX_HELPERS_H
#define __WINE_D3DX_HELPERS_H

#include <stdint.h>
#include "windef.h" /* For RECT. */
#include "wingdi.h" /* For PALETTEENTRY. */

#define DDS_PALETTE_SIZE (sizeof(PALETTEENTRY) * 256)

/* dds_header.flags */
#define DDS_CAPS 0x1
#define DDS_HEIGHT 0x2
#define DDS_WIDTH 0x4
#define DDS_PITCH 0x8
#define DDS_PIXELFORMAT 0x1000
#define DDS_MIPMAPCOUNT 0x20000
#define DDS_LINEARSIZE 0x80000
#define DDS_DEPTH 0x800000

/* dds_header.caps */
#define DDSCAPS_ALPHA    0x2
#define DDS_CAPS_COMPLEX 0x8
#define DDSCAPS_PALETTE  0x100
#define DDS_CAPS_TEXTURE 0x1000
#define DDS_CAPS_MIPMAP 0x400000

/* dds_header.caps2 */
#define DDS_CAPS2_CUBEMAP 0x200
#define DDS_CAPS2_CUBEMAP_POSITIVEX 0x400
#define DDS_CAPS2_CUBEMAP_NEGATIVEX 0x800
#define DDS_CAPS2_CUBEMAP_POSITIVEY 0x1000
#define DDS_CAPS2_CUBEMAP_NEGATIVEY 0x2000
#define DDS_CAPS2_CUBEMAP_POSITIVEZ 0x4000
#define DDS_CAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define DDS_CAPS2_CUBEMAP_ALL_FACES ( DDS_CAPS2_CUBEMAP_POSITIVEX | DDS_CAPS2_CUBEMAP_NEGATIVEX \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEY | DDS_CAPS2_CUBEMAP_NEGATIVEY \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEZ | DDS_CAPS2_CUBEMAP_NEGATIVEZ )
#define DDS_CAPS2_VOLUME 0x200000

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x1
#define DDS_PF_ALPHA_ONLY 0x2
#define DDS_PF_FOURCC 0x4
#define DDS_PF_INDEXED 0x20
#define DDS_PF_RGB 0x40
#define DDS_PF_YUV 0x200
#define DDS_PF_LUMINANCE 0x20000
#define DDS_PF_BUMPLUMINANCE 0x40000
#define DDS_PF_BUMPDUDV 0x80000

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
    DWORD signature;
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

struct vec4
{
    float x, y, z, w;
};

enum range {
    RANGE_FULL  = 0,
    RANGE_UNORM = 1,
    RANGE_SNORM = 2,
};

struct d3dx_color
{
    struct vec4 value;
    enum range rgb_range;
    enum range a_range;
};

static inline void set_d3dx_color(struct d3dx_color *color, const struct vec4 *value, enum range rgb_range,
        enum range a_range)
{
    color->value = *value;
    color->rgb_range = rgb_range;
    color->a_range = a_range;
}

struct volume
{
    UINT width;
    UINT height;
    UINT depth;
};

static inline void set_volume_struct(struct volume *volume, uint32_t width, uint32_t height, uint32_t depth)
{
    volume->width = width;
    volume->height = height;
    volume->depth = depth;
}

enum d3dx_image_file_format
{
    D3DX_IMAGE_FILE_FORMAT_BMP  = 0,
    D3DX_IMAGE_FILE_FORMAT_JPG  = 1,
    D3DX_IMAGE_FILE_FORMAT_TGA  = 2,
    D3DX_IMAGE_FILE_FORMAT_PNG  = 3,
    D3DX_IMAGE_FILE_FORMAT_DDS  = 4,
    D3DX_IMAGE_FILE_FORMAT_PPM  = 5,
    D3DX_IMAGE_FILE_FORMAT_DIB  = 6,
    D3DX_IMAGE_FILE_FORMAT_HDR  = 7,
    D3DX_IMAGE_FILE_FORMAT_PFM  = 8,
    /* This is a Wine only file format value. */
    D3DX_IMAGE_FILE_FORMAT_DDS_DXT10 = 100,
    D3DX_IMAGE_FILE_FORMAT_FORCE_DWORD = 0x7fffffff
};

enum d3dx_resource_type
{
    D3DX_RESOURCE_TYPE_UNKNOWN,
    D3DX_RESOURCE_TYPE_TEXTURE_1D,
    D3DX_RESOURCE_TYPE_TEXTURE_2D,
    D3DX_RESOURCE_TYPE_TEXTURE_3D,
    D3DX_RESOURCE_TYPE_CUBE_TEXTURE,
    D3DX_RESOURCE_TYPE_COUNT,
};

/* These values act as indexes into the pixel_format_desc table. */
enum d3dx_pixel_format_id
{
    D3DX_PIXEL_FORMAT_B8G8R8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8A8_UNORM,
    D3DX_PIXEL_FORMAT_B8G8R8X8_UNORM,
    D3DX_PIXEL_FORMAT_R8G8B8A8_UNORM,
    D3DX_PIXEL_FORMAT_R8G8B8X8_UNORM,
    D3DX_PIXEL_FORMAT_B5G6R5_UNORM,
    D3DX_PIXEL_FORMAT_B5G5R5X1_UNORM,
    D3DX_PIXEL_FORMAT_B5G5R5A1_UNORM,
    D3DX_PIXEL_FORMAT_B2G3R3_UNORM,
    D3DX_PIXEL_FORMAT_B2G3R3A8_UNORM,
    D3DX_PIXEL_FORMAT_B4G4R4A4_UNORM,
    D3DX_PIXEL_FORMAT_B4G4R4X4_UNORM,
    D3DX_PIXEL_FORMAT_B10G10R10A2_UNORM,
    D3DX_PIXEL_FORMAT_R10G10B10A2_UNORM,
    D3DX_PIXEL_FORMAT_R16G16B16_UNORM,
    D3DX_PIXEL_FORMAT_R16G16B16A16_UNORM,
    D3DX_PIXEL_FORMAT_R16G16_UNORM,
    D3DX_PIXEL_FORMAT_A8_UNORM,
    D3DX_PIXEL_FORMAT_L8A8_UNORM,
    D3DX_PIXEL_FORMAT_L4A4_UNORM,
    D3DX_PIXEL_FORMAT_L8_UNORM,
    D3DX_PIXEL_FORMAT_L16_UNORM,
    D3DX_PIXEL_FORMAT_DXT1_UNORM,
    D3DX_PIXEL_FORMAT_DXT2_UNORM,
    D3DX_PIXEL_FORMAT_DXT3_UNORM,
    D3DX_PIXEL_FORMAT_DXT4_UNORM,
    D3DX_PIXEL_FORMAT_DXT5_UNORM,
    D3DX_PIXEL_FORMAT_R16_FLOAT,
    D3DX_PIXEL_FORMAT_R16G16_FLOAT,
    D3DX_PIXEL_FORMAT_R16G16B16A16_FLOAT,
    D3DX_PIXEL_FORMAT_R32_FLOAT,
    D3DX_PIXEL_FORMAT_R32G32_FLOAT,
    D3DX_PIXEL_FORMAT_R32G32B32A32_FLOAT,
    D3DX_PIXEL_FORMAT_P1_UINT,
    D3DX_PIXEL_FORMAT_P2_UINT,
    D3DX_PIXEL_FORMAT_P4_UINT,
    D3DX_PIXEL_FORMAT_P8_UINT,
    D3DX_PIXEL_FORMAT_P8_UINT_A8_UNORM,
    D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM,
    D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM,
    D3DX_PIXEL_FORMAT_U8V8_SNORM,
    D3DX_PIXEL_FORMAT_U16V16_SNORM,
    D3DX_PIXEL_FORMAT_U8V8_SNORM_L8X8_UNORM,
    D3DX_PIXEL_FORMAT_U10V10W10_SNORM_A2_UNORM,
    D3DX_PIXEL_FORMAT_R8G8_B8G8_UNORM,
    D3DX_PIXEL_FORMAT_G8R8_G8B8_UNORM,
    D3DX_PIXEL_FORMAT_UYVY,
    D3DX_PIXEL_FORMAT_YUY2,
    D3DX_PIXEL_FORMAT_COUNT,
};

/* These are aliases. */
#define D3DX_PIXEL_FORMAT_R16G16B16A16_SNORM D3DX_PIXEL_FORMAT_U16V16W16Q16_SNORM
#define D3DX_PIXEL_FORMAT_R16G16_SNORM       D3DX_PIXEL_FORMAT_U16V16_SNORM
#define D3DX_PIXEL_FORMAT_R8G8B8A8_SNORM     D3DX_PIXEL_FORMAT_U8V8W8Q8_SNORM
#define D3DX_PIXEL_FORMAT_R8G8_SNORM         D3DX_PIXEL_FORMAT_U8V8_SNORM
#define D3DX_PIXEL_FORMAT_BC1_UNORM          D3DX_PIXEL_FORMAT_DXT1_UNORM
#define D3DX_PIXEL_FORMAT_BC2_UNORM          D3DX_PIXEL_FORMAT_DXT3_UNORM
#define D3DX_PIXEL_FORMAT_BC3_UNORM          D3DX_PIXEL_FORMAT_DXT5_UNORM

/* for internal use */
enum component_type
{
    CTYPE_EMPTY,
    CTYPE_UNORM,
    CTYPE_SNORM,
    CTYPE_FLOAT,
    CTYPE_LUMA,
    CTYPE_INDEX,
};

enum format_flag
{
    FMT_FLAG_DXT  = 0x01,
    FMT_FLAG_PACKED = 0x02,
    /* Internal only format, has no exact D3DFORMAT equivalent. */
    FMT_FLAG_INTERNAL = 0x04,
};

struct pixel_format_desc {
    enum d3dx_pixel_format_id format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    enum component_type a_type;
    enum component_type rgb_type;
    uint32_t flags;
};

struct d3dx_pixels
{
    const void *data;
    uint32_t row_pitch;
    uint32_t slice_pitch;
    const PALETTEENTRY *palette;

    struct volume size;
    RECT unaligned_rect;
};

static inline void set_d3dx_pixels(struct d3dx_pixels *pixels, const void *data, uint32_t row_pitch,
        uint32_t slice_pitch, const PALETTEENTRY *palette, uint32_t width, uint32_t height, uint32_t depth,
        const RECT *unaligned_rect)
{
    pixels->data = data;
    pixels->row_pitch = row_pitch;
    pixels->slice_pitch = slice_pitch;
    pixels->palette = palette;
    set_volume_struct(&pixels->size, width, height, depth);
    pixels->unaligned_rect = *unaligned_rect;
}

#define D3DX_IMAGE_INFO_ONLY 1
#define D3DX_IMAGE_SUPPORT_DXT10 2
struct d3dx_image
{
    enum d3dx_resource_type resource_type;
    enum d3dx_pixel_format_id format;

    struct volume size;
    uint32_t mip_levels;
    uint32_t layer_count;

    BYTE *pixels;
    PALETTEENTRY *palette;
    uint32_t layer_pitch;

    /*
     * image_buf and image_palette are pointers to allocated memory used to store
     * image data. If they are non-NULL, they need to be freed when no longer
     * in use.
     */
    void *image_buf;
    PALETTEENTRY *image_palette;

    enum d3dx_image_file_format image_file_format;
};

HRESULT d3dx_image_init(const void *src_data, uint32_t src_data_size, struct d3dx_image *image,
        uint32_t starting_mip_level, uint32_t flags);
void d3dx_image_cleanup(struct d3dx_image *image);
HRESULT d3dx_image_get_pixels(struct d3dx_image *image, uint32_t layer, uint32_t mip_level,
        struct d3dx_pixels *pixels);

static inline BOOL is_unknown_format(const struct pixel_format_desc *format)
{
    return (format->format == D3DX_PIXEL_FORMAT_COUNT);
}

static inline BOOL is_index_format(const struct pixel_format_desc *format)
{
    return (format->a_type == CTYPE_INDEX || format->rgb_type == CTYPE_INDEX);
}

static inline BOOL is_compressed_format(const struct pixel_format_desc *format)
{
    return !!(format->flags & FMT_FLAG_DXT);
}

static inline BOOL is_packed_format(const struct pixel_format_desc *format)
{
    return !!(format->flags & FMT_FLAG_PACKED);
}

static inline BOOL format_types_match(const struct pixel_format_desc *src, const struct pixel_format_desc *dst)
{
    if ((src->a_type && dst->a_type) && (src->a_type != dst->a_type))
        return FALSE;

    if ((src->rgb_type && dst->rgb_type) && (src->rgb_type != dst->rgb_type))
        return FALSE;

    if (src->flags != dst->flags)
        return FALSE;

    return (src->rgb_type == dst->rgb_type || src->a_type == dst->a_type);
}

static inline BOOL is_internal_format(const struct pixel_format_desc *format)
{
    return !!(format->flags & FMT_FLAG_INTERNAL);
}

static inline BOOL is_conversion_from_supported(const struct pixel_format_desc *format)
{
    return !is_packed_format(format) && !is_unknown_format(format);
}

static inline BOOL is_conversion_to_supported(const struct pixel_format_desc *format)
{
    return !is_index_format(format) && !is_packed_format(format) && !is_unknown_format(format);
}

const struct pixel_format_desc *get_d3dx_pixel_format_info(enum d3dx_pixel_format_id format);

void format_to_d3dx_color(const struct pixel_format_desc *format, const BYTE *src, const PALETTEENTRY *palette,
        struct d3dx_color *dst);
void format_from_d3dx_color(const struct pixel_format_desc *format, const struct d3dx_color *src, BYTE *dst);

uint32_t d3dx_calculate_layer_pixels_size(enum d3dx_pixel_format_id format, uint32_t width, uint32_t height,
        uint32_t depth, uint32_t mip_levels);
HRESULT d3dx_init_dds_header(struct dds_header *header, enum d3dx_resource_type resource_type,
        enum d3dx_pixel_format_id format, const struct volume *size, uint32_t mip_levels);
const char *debug_d3dx_image_file_format(enum d3dx_image_file_format format);
HRESULT d3dx_pixels_init(const void *data, uint32_t row_pitch, uint32_t slice_pitch,
        const PALETTEENTRY *palette, enum d3dx_pixel_format_id format, uint32_t left, uint32_t top, uint32_t right,
        uint32_t bottom, uint32_t front, uint32_t back, struct d3dx_pixels *pixels);
HRESULT d3dx_load_pixels_from_pixels(struct d3dx_pixels *dst_pixels,
       const struct pixel_format_desc *dst_desc, struct d3dx_pixels *src_pixels,
       const struct pixel_format_desc *src_desc, uint32_t filter_flags, uint32_t color_key);
void get_aligned_rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t width, uint32_t height,
        const struct pixel_format_desc *fmt_desc, RECT *aligned_rect);

unsigned short float_32_to_16(const float in);
float float_16_to_32(const unsigned short in);

struct d3dx_buffer
{
    void *buffer_iface;
    void *buffer_data;
};

struct d3dx_buffer_wrapper
{
    HRESULT (*d3dx_buffer_create)(unsigned int size, struct d3dx_buffer *d3dx_buffer);
    void (*d3dx_buffer_destroy)(struct d3dx_buffer *d3dx_buffer);
};

HRESULT d3dx_save_pixels_to_memory(struct d3dx_pixels *src_pixels, const struct pixel_format_desc *src_fmt_desc,
        enum d3dx_image_file_format file_format, const struct d3dx_buffer_wrapper *wrapper, struct d3dx_buffer *dst_buffer);

/* debug helpers */
const char *debug_d3dx_image_file_format(enum d3dx_image_file_format format);
#endif /* __WINE_D3DX_HELPERS_H */
