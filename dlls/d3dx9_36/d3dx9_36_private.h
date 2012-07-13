/*
 * Copyright (C) 2002 Raphael Junqueira
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Tony Wasserka
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

#ifndef __WINE_D3DX9_36_PRIVATE_H
#define __WINE_D3DX9_36_PRIVATE_H

#include <stdarg.h>

#define COBJMACROS
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "d3dx9.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

struct vec4
{
    float x, y, z, w;
};

struct volume
{
    UINT width;
    UINT height;
    UINT depth;
};

/* for internal use */
typedef enum _FormatType {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_UNKNOWN
} FormatType;

typedef struct _PixelFormatDesc {
    D3DFORMAT format;
    BYTE bits[4];
    BYTE shift[4];
    UINT bytes_per_pixel;
    UINT block_width;
    UINT block_height;
    UINT block_byte_count;
    FormatType type;
    void (*from_rgba)(const struct vec4 *src, struct vec4 *dst);
    void (*to_rgba)(const struct vec4 *src, struct vec4 *dst);
} PixelFormatDesc;

HRESULT map_view_of_file(LPCWSTR filename, LPVOID *buffer, DWORD *length) DECLSPEC_HIDDEN;
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, LPVOID *buffer, DWORD *length) DECLSPEC_HIDDEN;

HRESULT write_buffer_to_file(const WCHAR *filename, ID3DXBuffer *buffer) DECLSPEC_HIDDEN;

const PixelFormatDesc *get_format_info(D3DFORMAT format) DECLSPEC_HIDDEN;
const PixelFormatDesc *get_format_info_idx(int idx) DECLSPEC_HIDDEN;

void copy_simple_data(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, struct volume *src_size, const PixelFormatDesc *src_format,
        BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, struct volume *dst_size, const PixelFormatDesc *dst_format,
        D3DCOLOR color_key) DECLSPEC_HIDDEN;
void point_filter_simple_data(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, struct volume *src_size, const PixelFormatDesc *src_format,
        BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, struct volume *dst_size, const PixelFormatDesc *dst_format,
        D3DCOLOR color_key) DECLSPEC_HIDDEN;

HRESULT load_texture_from_dds(IDirect3DTexture9 *texture, const void *src_data, const PALETTEENTRY *palette,
    DWORD filter, D3DCOLOR color_key, const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT load_cube_texture_from_dds(IDirect3DCubeTexture9 *cube_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, D3DCOLOR color_key, const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT load_volume_from_dds(IDirect3DVolume9 *dst_volume, const PALETTEENTRY *dst_palette,
    const D3DBOX *dst_box, const void *src_data, const D3DBOX *src_box, DWORD filter, D3DCOLOR color_key,
    const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;
HRESULT load_volume_texture_from_dds(IDirect3DVolumeTexture9 *volume_texture, const void *src_data,
    const PALETTEENTRY *palette, DWORD filter, DWORD color_key, const D3DXIMAGE_INFO *src_info) DECLSPEC_HIDDEN;

/* debug helpers */
const char *debug_d3dxparameter_class(D3DXPARAMETER_CLASS c) DECLSPEC_HIDDEN;
const char *debug_d3dxparameter_type(D3DXPARAMETER_TYPE t) DECLSPEC_HIDDEN;

#endif /* __WINE_D3DX9_36_PRIVATE_H */
