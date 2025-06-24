/*
 * Tests for the D3DX9 volume functions
 *
 * Copyright 2012 Józef Kucia
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
#include "wine/test.h"
#include "d3dx9tex.h"
#include "d3dx9_test_images.h"

#define check_pixel_4bpp(box, x, y, z, color) _check_pixel_4bpp(__LINE__, box, x, y, z, color)
static inline void _check_pixel_4bpp(unsigned int line, const D3DLOCKED_BOX *box, int x, int y, int z, DWORD expected_color)
{
   DWORD color = ((DWORD *)box->pBits)[x + (y * box->RowPitch + z * box->SlicePitch) / 4];
   ok_(__FILE__, line)(color == expected_color, "Got color 0x%08lx, expected 0x%08lx\n", color, expected_color);
}

#define check_pixel_8bpp(box, x, y, z, color) _check_pixel_8bpp(__LINE__, box, x, y, z, color)
static inline void _check_pixel_8bpp(unsigned int line, const D3DLOCKED_BOX *box, int x, int y, int z, uint64_t expected_color)
{
   uint64_t color = ((uint64_t *)box->pBits)[x + (y * box->RowPitch + z * box->SlicePitch) / 8];
   ok_(__FILE__, line)(color == expected_color, "Got color %#I64x, expected %#I64x.\n", color, expected_color);
}

static inline void set_box(D3DBOX *box, UINT left, UINT top, UINT right, UINT bottom, UINT front, UINT back)
{
    box->Left = left;
    box->Top = top;
    box->Right = right;
    box->Bottom = bottom;
    box->Front = front;
    box->Back = back;
}

static void test_D3DXLoadVolumeFromMemory(IDirect3DDevice9 *device)
{
    int i, x, y, z;
    HRESULT hr;
    D3DBOX src_box, dst_box;
    D3DLOCKED_BOX locked_box;
    IDirect3DVolume9 *volume;
    IDirect3DVolumeTexture9 *volume_texture;
    const DWORD pixels[] = { 0xc3394cf0, 0x235ae892, 0x09b197fd, 0x8dc32bf6,
                             0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
                             0x00000000, 0x00000000, 0x00000000, 0xffffffff,
                             0xffffffff, 0x00000000, 0xffffffff, 0x00000000 };

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 4, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture.\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);
    set_box(&dst_box, 0, 0, 4, 1, 0, 4);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    if (FAILED(hr))
    {
        win_skip("D3DXLoadVolumeFromMemory failed with error %#lx, skipping some tests.\n", hr);
        return;
    }

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, 0, i / 4, pixels[i]);
    IDirect3DVolume9_UnlockBox(volume);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_UNKNOWN, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, E_FAIL);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, NULL, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, NULL, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 1);
    set_box(&dst_box, 0, 0, 4, 4, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, sizeof(pixels), NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, i / 4, 0, pixels[i]);
    IDirect3DVolume9_UnlockBox(volume);

    hr = D3DXLoadVolumeFromMemory(volume, NULL, NULL, pixels, D3DFMT_A8R8G8B8, 16, sizeof(pixels), NULL, &src_box, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, NULL, D3DLOCK_READONLY);
    for (i = 0; i < 16; i++) check_pixel_4bpp(&locked_box, i % 4, i / 4, 0, pixels[i]);
    for (z = 0; z < 4; z++)
    {
        for (y = 0; y < 256; y++)
        {
            for (x = 0; x < 256; x++)
                if (z != 0 || y >= 4 || x >= 4) check_pixel_4bpp(&locked_box, x, y, z, 0);
        }
    }
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 2, 2, 1, 2);
    set_box(&dst_box, 0, 0, 2, 2, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 8, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 4; i++) check_pixel_4bpp(&locked_box, i % 2, i / 2, 0, pixels[i + 4]);
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 2, 2, 2, 3);
    set_box(&dst_box, 0, 0, 2, 2, 1, 2);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 8, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    IDirect3DVolume9_LockBox(volume, &locked_box, &dst_box, D3DLOCK_READONLY);
    for (i = 0; i < 4; i++) check_pixel_4bpp(&locked_box, i % 2, i / 2, 0, pixels[i + 8]);
    IDirect3DVolume9_UnlockBox(volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);

    set_box(&dst_box, -1, -1, 3, 0, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 254, 254, 258, 255, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 4, 1, 0, 0, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 0, 0, 0, 0, 0, 0);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 0, 0, 0, 0, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&dst_box, 300, 300, 300, 300, 0, 0);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 0, 0, 4, 1, 0, 4);
    set_box(&dst_box, 0, 0, 4, 1, 0, 4);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 16, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 8, 8, 1, 1, D3DUSAGE_DYNAMIC, D3DFMT_DXT1, D3DPOOL_DEFAULT, &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    set_box(&src_box, 1, 1, 5, 3, 0, 1);
    set_box(&dst_box, 1, 1, 5, 3, 0, 1);
    hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 32, NULL, &src_box, D3DX_DEFAULT, 0);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        hr = D3DXLoadVolumeFromMemory(volume, NULL, &dst_box, pixels, D3DFMT_A8R8G8B8, 16, 32, NULL, &src_box,
                test_filter_values[i].filter, 0);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);
}

static void test_D3DXLoadVolumeFromFileInMemory(IDirect3DDevice9 *device)
{
    HRESULT hr;
    D3DBOX src_box;
    uint32_t x, y, z, i;
    D3DVOLUME_DESC desc;
    D3DXIMAGE_INFO img_info;
    IDirect3DVolume9 *volume;
    D3DLOCKED_BOX locked_box;
    struct volume_readback volume_rb;
    IDirect3DVolumeTexture9 *volume_texture;
    static const uint32_t bmp_32bpp_4_4_argb_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xff000000,
    };

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_DXT3, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, NULL, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadVolumeFromFileInMemory(NULL, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 2);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    set_box(&src_box, 0, 0, 0, 0, 0, 0);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == E_FAIL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, E_FAIL);

    set_box(&src_box, 0, 0, 5, 4, 0, 2);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    set_box(&src_box, 0, 0, 4, 4, 0, 3);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_volume_map, sizeof(dds_volume_map), &src_box, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadVolumeFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    /* Load 2D DDS image into a volume. */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 8, 8, 8, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);
    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_24bit_8_8, sizeof(dds_24bit_8_8), NULL,
                test_filter_values[i].filter, 0, NULL);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, dds_24bit_8_8, sizeof(dds_24bit_8_8), NULL,
            D3DX_FILTER_POINT, 0, &img_info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&img_info, 8, 8, 1, 4, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);

    IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &desc);
    get_texture_volume_readback(device, volume_texture, 0, &volume_rb);
    for (z = 0; z < desc.Depth; ++z)
    {
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, 0xffff0000, FALSE);
            }
        }
    }
    release_volume_readback(&volume_rb);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    /* Load a BMP image into a volume. */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);
    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, bmp_32bpp_4_4_argb, sizeof(bmp_32bpp_4_4_argb), NULL,
            D3DX_FILTER_POINT, 0, &img_info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&img_info, 4, 4, 1, 1, D3DFMT_A8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_BMP, FALSE);

    IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &desc);
    get_texture_volume_readback(device, volume_texture, 0, &volume_rb);
    for (z = 0; z < desc.Depth; ++z)
    {
        for (y = 0; y < desc.Height; ++y)
        {
            const uint32_t *expected_color = &bmp_32bpp_4_4_argb_expected[(y < (desc.Height / 2)) ? 0 : 2];

            for (x = 0; x < desc.Width; ++x)
            {
                if (x < (desc.Width / 2))
                    check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color[0], FALSE);
                else
                    check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color[1], FALSE);
            }
        }
    }
    release_volume_readback(&volume_rb);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);

    /* PNG tests. */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 2, 2, 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_A16B16G16R16, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, png_2_2_48bpp_rgb, sizeof(png_2_2_48bpp_rgb), NULL,
            D3DX_FILTER_POINT, 0, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DVolume9_LockBox(volume, &locked_box, NULL, D3DLOCK_READONLY);

    for (i = 0; i < 2; ++i)
    {
        check_pixel_8bpp(&locked_box, 0, 0, i, 0xffff202010100000);
        check_pixel_8bpp(&locked_box, 1, 0, i, 0xffff505040403030);
        check_pixel_8bpp(&locked_box, 0, 1, i, 0xffff808070706060);
        check_pixel_8bpp(&locked_box, 1, 1, i, 0xffffb0b0a0a09090);
    }

    IDirect3DVolume9_UnlockBox(volume);

    hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, png_2_2_64bpp_rgba, sizeof(png_2_2_64bpp_rgba), NULL,
            D3DX_FILTER_POINT, 0, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DVolume9_LockBox(volume, &locked_box, NULL, D3DLOCK_READONLY);

    for (i = 0; i < 2; ++i)
    {
        check_pixel_8bpp(&locked_box, 0, 0, i, 0x3030202010100000);
        check_pixel_8bpp(&locked_box, 1, 0, i, 0x7070606050504040);
        check_pixel_8bpp(&locked_box, 0, 1, i, 0xb0b0a0a090908080);
        check_pixel_8bpp(&locked_box, 1, 1, i, 0xf0f0e0e0d0d0c0c0);
    }

    IDirect3DVolume9_UnlockBox(volume);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);
}

static void set_vec3(D3DXVECTOR3 *v, float x, float y, float z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

static const D3DXVECTOR4 quadrant_color[] =
{
    { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f },
};

static void WINAPI fill_func_volume(D3DXVECTOR4 *value, const D3DXVECTOR3 *coord, const D3DXVECTOR3 *size, void *data)
{
    D3DXVECTOR3 vec = *coord;
    uint32_t idx;

    if (data)
    {
        *value = *(D3DXVECTOR4 *)data;
        return;
    }

    set_vec3(&vec, (vec.x / size->x) - 0.5f, (vec.y / size->y) - 0.5f, (vec.z / size->z) - 0.5f);
    if (vec.x < 16.0f)
        idx = vec.y < 16.0f ? 0 : 2;
    else
        idx = vec.y < 16.0f ? 1 : 3;
    idx += vec.z < 1.0f ? 0 : 4;

    *value = quadrant_color[idx];
}

#define check_volume_readback_slice_quadrants(rb, width, height, slice, expected, max_diff) \
    _check_volume_readback_slice_quadrants(__LINE__, rb, width, height, slice, expected, max_diff)
static inline void _check_volume_readback_slice_quadrants(unsigned int line, struct volume_readback *rb,
        uint32_t width, uint32_t height, uint32_t slice, const uint32_t *expected, uint8_t max_diff)
{
    /* Order is: top left, top right, bottom left, bottom right. */
    _check_volume_readback_pixel_4bpp_diff(__FILE__, line, rb, 0, 0, slice, expected[0], max_diff, FALSE);
    _check_volume_readback_pixel_4bpp_diff(__FILE__, line, rb, (width - 1), 0, slice, expected[1], max_diff, FALSE);
    _check_volume_readback_pixel_4bpp_diff(__FILE__, line, rb, 0, (height - 1), slice, expected[2], max_diff, FALSE);
    _check_volume_readback_pixel_4bpp_diff(__FILE__, line, rb, (width - 1), (height - 1), slice, expected[3], max_diff,
            FALSE);
}

static void test_d3dx_save_volume_to_file(IDirect3DDevice9 *device)
{
    static const struct dds_pixel_format d3dfmt_a8r8g8b8_pf = { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32,
                                                                0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
    static const uint32_t front_expected[] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    static const uint32_t empty_expected[] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
    static const uint32_t back_expected[] = { 0xffffffff, 0xff0000ff, 0xffff0000, 0xff00ff00 };
    static const struct
    {
        const char *name;

        D3DXIMAGE_FILEFORMAT iff;
        D3DFORMAT saved_format;
        uint8_t max_diff;
        BOOL wine_todo;
    } tests[] =
    {
        { "D3DXIFF_BMP", D3DXIFF_BMP, D3DFMT_A8R8G8B8 },
        { "D3DXIFF_JPG", D3DXIFF_JPG, D3DFMT_X8R8G8B8, .max_diff = 1 },
        { "D3DXIFF_TGA", D3DXIFF_TGA, D3DFMT_A8R8G8B8 },
        { "D3DXIFF_PNG", D3DXIFF_PNG, D3DFMT_A8R8G8B8 },
        { "D3DXIFF_DDS", D3DXIFF_DDS, D3DFMT_A8R8G8B8 },
        { "D3DXIFF_HDR", D3DXIFF_HDR, D3DFMT_A32B32G32R32F, .max_diff = 1, .wine_todo = TRUE },
        { "D3DXIFF_PFM", D3DXIFF_PFM, D3DFMT_A32B32G32R32F, .wine_todo = TRUE },
        { "D3DXIFF_PPM", D3DXIFF_PPM, D3DFMT_X8R8G8B8, .wine_todo = TRUE },
    };
    struct
    {
         DWORD magic;
         struct dds_header header;
         BYTE *data;
    } *dds;
    const D3DXVECTOR4 clear_val = { 0.0f, 0.0f, 0.0f, 0.0f };
    IDirect3DVolumeTexture9 *volume_texture;
    struct volume_readback volume_rb;
    ID3DXBuffer *buffer = NULL;
    WCHAR name_buf_w[MAX_PATH];
    char name_buf_a[MAX_PATH];
    IDirect3DVolume9 *volume;
    D3DXIMAGE_INFO info;
    unsigned int i, j;
    D3DBOX box;
    HRESULT hr;

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 32, 32, 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
            &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture.\n");
        return;
    }

    hr = D3DXFillVolumeTexture(volume_texture, fill_func_volume, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);

    hr = D3DXSaveVolumeToFileInMemory(&buffer, D3DXIFF_DDS, volume, NULL, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_DEPTH | DDSD_PIXELFORMAT, 32, 32, 0,
            2, 0, &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, DDSCAPS2_VOLUME, FALSE);
    ID3DXBuffer_Release(buffer);

    /*
     * Save with a box that has a depth of 1. This results in a DDS header
     * without any depth/volume flags set, even though we're saving a volume.
     */
    set_box(&box, 0, 0, 32, 32, 1, 2);
    hr = D3DXSaveVolumeToFileInMemory(&buffer, D3DXIFF_DDS, volume, NULL, &box);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 32, 32, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    ID3DXBuffer_Release(buffer);

    GetTempPathA(ARRAY_SIZE(name_buf_a), name_buf_a);
    GetTempFileNameA(name_buf_a, "dxa", 0, name_buf_a);

    GetTempPathW(ARRAY_SIZE(name_buf_w), name_buf_w);
    GetTempFileNameW(name_buf_w, L"dxw", 0, name_buf_w);
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("File format %u (%s)", i, tests[i].name);

        /*
         * Test twice, first with a box that has a depth of 2, then second
         * with a box that has a depth of 1.
         */
        for (j = 0; j < 2; ++j)
        {
            winetest_push_context("Depth %u", 2 - j);
            hr = D3DXFillVolumeTexture(volume_texture, fill_func_volume, NULL);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

            set_box(&box, 0, 0, 32, 32, j, 2);

            hr = D3DXSaveVolumeToFileA(name_buf_a, tests[i].iff, volume, NULL, &box);
            todo_wine_if(tests[i].wine_todo) ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

            hr = D3DXSaveVolumeToFileW(name_buf_w, tests[i].iff, volume, NULL, &box);
            todo_wine_if(tests[i].wine_todo) ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

            hr = D3DXSaveVolumeToFileInMemory(&buffer, tests[i].iff, volume, NULL, &box);
            todo_wine_if(tests[i].wine_todo) ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
            if (!strcmp(winetest_platform, "wine") && tests[i].wine_todo)
            {
                winetest_pop_context();
                continue;
            }

            /* ASCII string. */
            hr = D3DXFillVolumeTexture(volume_texture, fill_func_volume, (void *)&clear_val);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

            memset(&info, 0, sizeof(info));
            hr = D3DXLoadVolumeFromFileA(volume, NULL, NULL, name_buf_a, NULL, D3DX_FILTER_NONE, 0, &info);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
            get_texture_volume_readback(device, volume_texture, 0, &volume_rb);
            if (tests[i].iff == D3DXIFF_DDS)
            {
                if (!j)
                {
                    check_image_info(&info, 32, 32, 2, 1, D3DFMT_A8R8G8B8, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, front_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, back_expected, 0);
                }
                else
                {
                    check_image_info(&info, 32, 32, 1, 1, D3DFMT_A8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, back_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, 0);
                }
            }
            else
            {
                check_image_info(&info, 32, 32, 1, 1, tests[i].saved_format, D3DRTYPE_TEXTURE, tests[i].iff, FALSE);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, !j ? front_expected : back_expected,
                        tests[i].max_diff);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, tests[i].max_diff);
            }
            release_volume_readback(&volume_rb);

            /* Wide string. */
            hr = D3DXFillVolumeTexture(volume_texture, fill_func_volume, (void *)&clear_val);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

            memset(&info, 0, sizeof(info));
            hr = D3DXLoadVolumeFromFileW(volume, NULL, NULL, name_buf_w, NULL, D3DX_FILTER_NONE, 0, &info);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
            get_texture_volume_readback(device, volume_texture, 0, &volume_rb);
            if (tests[i].iff == D3DXIFF_DDS)
            {
                if (!j)
                {
                    check_image_info(&info, 32, 32, 2, 1, D3DFMT_A8R8G8B8, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, front_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, back_expected, 0);
                }
                else
                {
                    check_image_info(&info, 32, 32, 1, 1, D3DFMT_A8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, back_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, 0);
                }
            }
            else
            {
                check_image_info(&info, 32, 32, 1, 1, tests[i].saved_format, D3DRTYPE_TEXTURE, tests[i].iff, FALSE);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, !j ? front_expected : back_expected,
                        tests[i].max_diff);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, tests[i].max_diff);
            }
            release_volume_readback(&volume_rb);

            /* File in memory. */
            memset(&info, 0, sizeof(info));
            hr = D3DXLoadVolumeFromFileInMemory(volume, NULL, NULL, ID3DXBuffer_GetBufferPointer(buffer),
                    ID3DXBuffer_GetBufferSize(buffer), NULL, D3DX_FILTER_NONE, 0, &info);
            ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
            get_texture_volume_readback(device, volume_texture, 0, &volume_rb);
            if (tests[i].iff == D3DXIFF_DDS)
            {
                if (!j)
                {
                    check_image_info(&info, 32, 32, 2, 1, D3DFMT_A8R8G8B8, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, front_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, back_expected, 0);
                }
                else
                {
                    check_image_info(&info, 32, 32, 1, 1, D3DFMT_A8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, back_expected, 0);
                    check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, 0);
                }
            }
            else
            {
                check_image_info(&info, 32, 32, 1, 1, tests[i].saved_format, D3DRTYPE_TEXTURE, tests[i].iff, FALSE);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 0, !j ? front_expected : back_expected,
                        tests[i].max_diff);
                check_volume_readback_slice_quadrants(&volume_rb, 32, 32, 1, empty_expected, tests[i].max_diff);
            }
            release_volume_readback(&volume_rb);

            ID3DXBuffer_Release(buffer);
            DeleteFileA(name_buf_a);
            DeleteFileW(name_buf_w);
            winetest_pop_context();
        }

        winetest_pop_context();
    }

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(volume_texture);
}

START_TEST(volume)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if(FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object %#lx\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXLoadVolumeFromMemory(device);
    test_D3DXLoadVolumeFromFileInMemory(device);
    test_d3dx_save_volume_to_file(device);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}
