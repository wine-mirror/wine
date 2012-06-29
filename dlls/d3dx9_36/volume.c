/*
 * Copyright 2010 Christian Costa
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

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI D3DXLoadVolumeFromMemory(IDirect3DVolume9 *dst_volume,
                                        const PALETTEENTRY *dst_palette,
                                        const D3DBOX *dst_box,
                                        const void *src_memory,
                                        D3DFORMAT src_format,
                                        UINT src_row_pitch,
                                        UINT src_slice_pitch,
                                        const PALETTEENTRY *src_palette,
                                        const D3DBOX *src_box,
                                        DWORD filter,
                                        D3DCOLOR color_key)
{
    HRESULT hr;
    D3DVOLUME_DESC desc;
    D3DLOCKED_BOX locked_box;
    UINT dst_width, dst_height, dst_depth;
    UINT src_width, src_height, src_depth;
    const PixelFormatDesc *src_format_desc, *dst_format_desc;

    TRACE("(%p, %p, %p, %p, %#x, %u, %u, %p, %p, %x, %x)\n", dst_volume, dst_palette, dst_box,
            src_memory, src_format, src_row_pitch, src_slice_pitch, src_palette, src_box,
            filter, color_key);

    if (!dst_volume || !src_memory || !src_box) return D3DERR_INVALIDCALL;

    if (src_format == D3DFMT_UNKNOWN
            || src_box->Left >= src_box->Right
            || src_box->Top >= src_box->Bottom
            || src_box->Front >= src_box->Back)
        return E_FAIL;

    if (filter == D3DX_DEFAULT)
        filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    IDirect3DVolume9_GetDesc(dst_volume, &desc);

    src_width = src_box->Right - src_box->Left;
    src_height = src_box->Bottom - src_box->Top;
    src_depth = src_box->Back - src_box->Front;

    if (!dst_box)
    {
        dst_width = desc.Width;
        dst_height = desc.Height;
        dst_depth = desc.Depth;
    }
    else
    {
        if (dst_box->Left >= dst_box->Right || dst_box->Right > desc.Width)
            return D3DERR_INVALIDCALL;
        if (dst_box->Top >= dst_box->Bottom || dst_box->Bottom > desc.Height)
            return D3DERR_INVALIDCALL;
        if (dst_box->Front >= dst_box->Back || dst_box->Back > desc.Depth)
            return D3DERR_INVALIDCALL;

        dst_width = dst_box->Right - dst_box->Left;
        dst_height = dst_box->Bottom - dst_box->Top;
        dst_depth = dst_box->Back - dst_box->Front;
    }

    src_format_desc = get_format_info(src_format);
    if (src_format_desc->type == FORMAT_UNKNOWN)
        return E_NOTIMPL;

    dst_format_desc = get_format_info(desc.Format);
    if (dst_format_desc->type == FORMAT_UNKNOWN)
        return E_NOTIMPL;

    if (desc.Format == src_format
            && dst_width == src_width && dst_height == src_height && dst_depth == src_depth)
    {
        UINT row, slice;
        BYTE *dst_addr;
        const BYTE *src_addr;
        UINT row_block_count = (src_width + src_format_desc->block_width - 1) / src_format_desc->block_width;
        UINT row_count = (src_height + src_format_desc->block_height - 1) / src_format_desc->block_height;

        if (src_box->Left & (src_format_desc->block_width - 1)
                || src_box->Top & (src_format_desc->block_height - 1)
                || (src_box->Right & (src_format_desc->block_width - 1)
                    && src_width != desc.Width)
                || (src_box->Bottom & (src_format_desc->block_height - 1)
                    && src_height != desc.Height))
        {
            FIXME("Source box (%u, %u, %u, %u) is misaligned\n",
                    src_box->Left, src_box->Top, src_box->Right, src_box->Bottom);
            return E_NOTIMPL;
        }

        hr = IDirect3DVolume9_LockBox(dst_volume, &locked_box, dst_box, 0);
        if (FAILED(hr)) return hr;

        for (slice = 0; slice < src_depth; slice++)
        {
            src_addr = src_memory;
            src_addr += (src_box->Front + slice) * src_slice_pitch;
            src_addr += (src_box->Top / src_format_desc->block_height) * src_row_pitch;
            src_addr += (src_box->Left / src_format_desc->block_width) * src_format_desc->block_byte_count;

            dst_addr = locked_box.pBits;
            dst_addr += slice * locked_box.SlicePitch;

            for (row = 0; row < row_count; row++)
            {
                memcpy(dst_addr, src_addr, row_block_count * src_format_desc->block_byte_count);
                src_addr += src_row_pitch;
                dst_addr += locked_box.RowPitch;
            }
        }

        IDirect3DVolume9_UnlockBox(dst_volume);
    }
    else
    {
        FIXME("Stretching or format conversion not implemented\n");
        return E_NOTIMPL;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXLoadVolumeFromVolume(IDirect3DVolume9 *dst_volume,
                                        const PALETTEENTRY *dst_palette,
                                        const D3DBOX *dst_box,
                                        IDirect3DVolume9 *src_volume,
                                        const PALETTEENTRY *src_palette,
                                        const D3DBOX *src_box,
                                        DWORD filter,
                                        D3DCOLOR color_key)
{
    HRESULT hr;
    D3DBOX box;
    D3DVOLUME_DESC desc;
    D3DLOCKED_BOX locked_box;

    TRACE("(%p, %p, %p, %p, %p, %p, %#x, %#x)\n",
            dst_volume, dst_palette, dst_box, src_volume, src_palette, src_box,
            filter, color_key);

    if (!dst_volume || !src_volume) return D3DERR_INVALIDCALL;

    IDirect3DVolume9_GetDesc(src_volume, &desc);

    if (!src_box)
    {
        box.Left = box.Top = 0;
        box.Right = desc.Width;
        box.Bottom = desc.Height;
        box.Front = 0;
        box.Back = desc.Depth;
    }
    else box = *src_box;

    hr = IDirect3DVolume9_LockBox(src_volume, &locked_box, &box, D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;

    hr = D3DXLoadVolumeFromMemory(dst_volume, dst_palette, dst_box,
            locked_box.pBits, desc.Format, locked_box.RowPitch, locked_box.SlicePitch,
            src_palette, &box, filter, color_key);

    IDirect3DVolume9_UnlockBox(src_volume);
    return hr;
}
