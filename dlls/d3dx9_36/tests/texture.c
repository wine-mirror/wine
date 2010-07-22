/*
 * Tests for the D3DX9 texture functions
 *
 * Copyright 2009 Tony Wasserka
 * Copyright 2010 Owen Rudge for CodeWeavers
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

static void test_D3DXCheckTextureRequirements(IDirect3DDevice9 *device)
{
    UINT width, height, mipmaps;
    D3DFORMAT format;
    D3DCAPS9 caps;
    HRESULT hr;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    /* general tests */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* width & height */
    width = height = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);
    ok(height == 256, "Returned height %d, expected %d\n", height, 256);

    width = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        skip("Hardware only supports pow2 textures\n");
    else
    {
        width = 62;
        hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(width == 62, "Returned width %d, expected %d\n", width, 62);

        width = D3DX_DEFAULT; height = 63;
        hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(width == height, "Returned width %d, expected %d\n", width, height);
        ok(height == 63, "Returned height %d, expected %d\n", height, 63);
    }

    width = D3DX_DEFAULT; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);

    width = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    width = 0xFFFFFFFE;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == caps.MaxTextureWidth, "Returned width %d, expected %d\n", width, caps.MaxTextureWidth);

    width = caps.MaxTextureWidth-1;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == caps.MaxTextureWidth-1, "Returned width %d, expected %d\n", width, caps.MaxTextureWidth-1);

    /* mipmaps */
    width = 64; height = 63;
    mipmaps = 9;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    width = 284; height = 137;
    mipmaps = 20;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    width = height = 63;
    mipmaps = 9;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 6, "Returned mipmaps %d, expected %d\n", mipmaps, 6);

    mipmaps = 20;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    mipmaps = 0;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    /* usage */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_WRITEONLY, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_DONOTCLIP, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_POINTS, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RTPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_NPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");

    /* format */
    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = 0;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);
}

static void test_D3DXCreateTexture(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *texture;
    D3DSURFACE_DESC desc;
    D3DCAPS9 caps;
    UINT mipmaps;
    HRESULT hr;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    hr = D3DXCreateTexture(NULL, 0, 0, 0, 0, D3DX_DEFAULT, 0, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* width and height tests */

    hr = D3DXCreateTexture(device, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);

        ok(desc.Width == 256, "Returned width %d, expected %d\n", desc.Width, 256);
        ok(desc.Height == 256, "Returned height %d, expected %d\n", desc.Height, 256);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);

        ok(desc.Width == 1, "Returned width %d, expected %d\n", desc.Width, 1);
        ok(desc.Height == 1, "Returned height %d, expected %d\n", desc.Height, 1);

        IDirect3DTexture9_Release(texture);
    }


    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        skip("Hardware only supports pow2 textures\n");
    else
    {
        hr = D3DXCreateTexture(device, D3DX_DEFAULT, 63, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
        ok((hr == D3D_OK) ||
           /* may not work with conditional NPOT */
           ((hr != D3D_OK) && (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)),
           "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

        if (texture)
        {
            hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);

            /* Conditional NPOT may create a texture with different dimensions, so allow those
               situations instead of returning a fail */

            ok(desc.Width == 63 ||
               (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL),
               "Returned width %d, expected %d\n", desc.Width, 63);

            ok(desc.Height == 63 ||
               (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL),
               "Returned height %d, expected %d\n", desc.Height, 63);

            IDirect3DTexture9_Release(texture);
        }
    }

    /* mipmaps */

    hr = D3DXCreateTexture(device, 64, 63, 9, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 284, 137, 9, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 20, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 64, 64, 1, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);

        IDirect3DTexture9_Release(texture);
    }

    /* usage */

    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_DONOTCLIP, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_POINTS, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_RTPATCHES, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_NPATCHES, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");

    /* format */

    hr = D3DXCreateTexture(device, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }
}

static void test_D3DXFilterTexture(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *tex;
    HRESULT hr;

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 5, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_BOX + 1); /* Invalid filter */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 5, D3DX_FILTER_NONE); /* Last miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 20, D3DX_FILTER_NONE); /* Invalid miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
    }
    else
        skip("Failed to create texture\n");

    IDirect3DTexture9_Release(tex);

    hr = D3DXFilterTexture(NULL, NULL, 0, D3DX_FILTER_NONE);
    ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* Test different pools */
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");


    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");
}

START_TEST(texture)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (!wnd) {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXCheckTextureRequirements(device);
    test_D3DXCreateTexture(device);
    test_D3DXFilterTexture(device);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}
