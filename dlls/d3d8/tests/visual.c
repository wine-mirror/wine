/*
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2007, 2009, 2011-2013 Stefan Dösinger(for CodeWeavers)
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

/* See comment in dlls/d3d9/tests/visual.c for general guidelines */

#include <math.h>

#define COBJMACROS
#include <d3d8.h>
#include "wine/test.h"

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

static BOOL color_match(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    return TRUE;
}

static DWORD getPixelColor(IDirect3DDevice8 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DTexture8 *tex = NULL;
    IDirect3DSurface8 *surf = NULL, *backbuf = NULL;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1 /* Levels */, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex);
    if(FAILED(hr) || !tex )  /* This is not a test */
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%#08x\n", hr);
        return 0xdeadbeef;
    }
    hr = IDirect3DTexture8_GetSurfaceLevel(tex, 0, &surf);
    if (FAILED(hr))  /* This is not a test */
    {
        trace("Can't get surface from texture, hr=%#08x\n", hr);
        ret = 0xdeadbeee;
        goto out;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &backbuf);
    if(FAILED(hr))
    {
        trace("Can't get the render target, hr=%#08x\n", hr);
        ret = 0xdeadbeed;
        goto out;
    }
    hr = IDirect3DDevice8_CopyRects(device, backbuf, NULL, 0, surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't read the render target, hr=%#08x\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    hr = IDirect3DSurface8_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%#08x\n", hr);
        ret = 0xdeadbeeb;
        goto out;
    }
    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface8_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%#08x\n", hr);
    }

out:
    if(backbuf) IDirect3DSurface8_Release(backbuf);
    if(surf) IDirect3DSurface8_Release(surf);
    if(tex) IDirect3DTexture8_Release(tex);
    return ret;
}

static IDirect3DDevice8 *create_device(IDirect3D8 *d3d, HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice8 *device;

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (SUCCEEDED(IDirect3D8_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device)))
        return device;

    return NULL;
}

static void test_sanity(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    color = getPixelColor(device, 1, 1);
    ok(color == 0x00ff0000, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    color = getPixelColor(device, 639, 479);
    ok(color == 0x0000ddee, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void lighting_test(void)
{
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    unlitquad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffff0000},
        {{-1.0f,  0.0f, 0.1f}, 0xffff0000},
        {{ 0.0f,  0.0f, 0.1f}, 0xffff0000},
        {{ 0.0f, -1.0f, 0.1f}, 0xffff0000},
    },
    litquad[] =
    {
        {{-1.0f,  0.0f, 0.1f}, 0xff00ff00},
        {{-1.0f,  1.0f, 0.1f}, 0xff00ff00},
        {{ 0.0f,  1.0f, 0.1f}, 0xff00ff00},
        {{ 0.0f,  0.0f, 0.1f}, 0xff00ff00},
    };
    static const struct
    {
        struct vec3 position;
        struct vec3 normal;
        DWORD diffuse;
    }
    unlitnquad[] =
    {
        {{0.0f, -1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{0.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{1.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{1.0f, -1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
    },
    litnquad[] =
    {
        {{0.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{0.0f,  1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{1.0f,  1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{1.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
    },
    nquad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
    },
    rotatedquad[] =
    {
        {{-10.0f, -11.0f, 11.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f,  -9.0f, 11.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f,  -9.0f,  9.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f, -11.0f,  9.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
    },
    translatedquad[] =
    {
        {{-11.0f, -11.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{-11.0f,  -9.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ -9.0f,  -9.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ -9.0f, -11.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
    };
    static const WORD indices[] = {0, 1, 2, 2, 3, 0};
    static const D3DMATRIX mat =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}},
    mat_singular =
    {{{
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f,
    }}},
    mat_transf =
    {{{
         0.0f,  0.0f,  1.0f, 0.0f,
         0.0f,  1.0f,  0.0f, 0.0f,
        -1.0f,  0.0f,  0.0f, 0.0f,
         10.f, 10.0f, 10.0f, 1.0f,
    }}},
    mat_nonaffine =
    {{{
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f, -1.0f,
        10.f, 10.0f, 10.0f,  0.0f,
    }}};
    static const struct
    {
        const D3DMATRIX *world_matrix;
        const void *quad;
        unsigned int size;
        DWORD expected;
        const char *message;
    }
    tests[] =
    {
        {&mat, nquad, sizeof(nquad[0]), 0x000000ff, "Lit quad with light"},
        {&mat_singular, nquad, sizeof(nquad[0]), 0x000000ff, "Lit quad with singular world matrix"},
        {&mat_transf, rotatedquad, sizeof(rotatedquad[0]), 0x000000ff, "Lit quad with transformation matrix"},
        {&mat_nonaffine, translatedquad, sizeof(translatedquad[0]), 0x00000000, "Lit quad with non-affine matrix"},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLDMATRIX(0), &mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_VIEW, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed with %#08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* No lights are defined... That means, lit vertices should be entirely black. */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
            2, indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
            2, indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, nfvf);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
            2, indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
            2, indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360); /* Lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 160, 120); /* Upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 480, 360); /* Lower right quad - unlit with normals */
    ok(color == 0x000000ff, "Unlit quad with normals has color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 480, 120); /* Upper right quad - lit with normals */
    ok(color == 0x00000000, "Lit quad with normals has color 0x%08x, expected 0x00000000.\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice8_LightEnable(device, 0, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable light 0, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLD, tests[i].world_matrix);
        ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                2, indices, D3DFMT_INDEX16, tests[i].quad, tests[i].size);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color == tests[i].expected, "%s has color 0x%08x.\n", tests[i].message, color);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_specular_lighting(void)
{
    static const unsigned int vertices_side = 5;
    const unsigned int indices_count = (vertices_side - 1) * (vertices_side - 1) * 2 * 3;
    static const DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    static const D3DMATRIX mat =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};
    static const D3DLIGHT8 directional =
    {
        D3DLIGHT_DIRECTIONAL,
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
    },
    point =
    {
        D3DLIGHT_POINT,
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        100.0f,
        0.0f,
        0.0f, 0.0f, 1.0f,
    },
    spot =
    {
        D3DLIGHT_SPOT,
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        100.0f,
        1.0f,
        0.0f, 0.0f, 1.0f,
        M_PI / 12.0f, M_PI / 3.0f
    },
    /* The chosen range value makes the test fail when using a manhattan
     * distance metric vs the correct euclidean distance. */
    point_range =
    {
        D3DLIGHT_POINT,
        {0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        1.2f,
        0.0f,
        0.0f, 0.0f, 1.0f,
    };
    static const struct expected_color
    {
        unsigned int x, y;
        D3DCOLOR color;
    }
    expected_directional[] =
    {
        {160, 120, 0x00ffffff},
        {320, 120, 0x00ffffff},
        {480, 120, 0x00ffffff},
        {160, 240, 0x00ffffff},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00ffffff},
        {160, 360, 0x00ffffff},
        {320, 360, 0x00ffffff},
        {480, 360, 0x00ffffff},
    },
    expected_directional_local[] =
    {
        {160, 120, 0x003c3c3c},
        {320, 120, 0x00717171},
        {480, 120, 0x003c3c3c},
        {160, 240, 0x00717171},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00717171},
        {160, 360, 0x003c3c3c},
        {320, 360, 0x00717171},
        {480, 360, 0x003c3c3c},
    },
    expected_point[] =
    {
        {160, 120, 0x00282828},
        {320, 120, 0x005a5a5a},
        {480, 120, 0x00282828},
        {160, 240, 0x005a5a5a},
        {320, 240, 0x00ffffff},
        {480, 240, 0x005a5a5a},
        {160, 360, 0x00282828},
        {320, 360, 0x005a5a5a},
        {480, 360, 0x00282828},
    },
    expected_point_local[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00070707},
        {480, 120, 0x00000000},
        {160, 240, 0x00070707},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00070707},
        {160, 360, 0x00000000},
        {320, 360, 0x00070707},
        {480, 360, 0x00000000},
    },
    expected_spot[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00141414},
        {480, 120, 0x00000000},
        {160, 240, 0x00141414},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00141414},
        {160, 360, 0x00000000},
        {320, 360, 0x00141414},
        {480, 360, 0x00000000},
    },
    expected_spot_local[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00020202},
        {480, 120, 0x00000000},
        {160, 240, 0x00020202},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00020202},
        {160, 360, 0x00000000},
        {320, 360, 0x00020202},
        {480, 360, 0x00000000},
    },
    expected_point_range[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x005a5a5a},
        {480, 120, 0x00000000},
        {160, 240, 0x005a5a5a},
        {320, 240, 0x00ffffff},
        {480, 240, 0x005a5a5a},
        {160, 360, 0x00000000},
        {320, 360, 0x005a5a5a},
        {480, 360, 0x00000000},
    };
    static const struct
    {
        const D3DLIGHT8 *light;
        BOOL local_viewer;
        const struct expected_color *expected;
        unsigned int expected_count;
    }
    tests[] =
    {
        {&directional, FALSE, expected_directional,
                sizeof(expected_directional) / sizeof(expected_directional[0])},
        {&directional, TRUE, expected_directional_local,
                sizeof(expected_directional_local) / sizeof(expected_directional_local[0])},
        {&point, FALSE, expected_point,
                sizeof(expected_point) / sizeof(expected_point[0])},
        {&point, TRUE, expected_point_local,
                sizeof(expected_point_local) / sizeof(expected_point_local[0])},
        {&spot, FALSE, expected_spot,
                sizeof(expected_spot) / sizeof(expected_spot[0])},
        {&spot, TRUE, expected_spot_local,
                sizeof(expected_spot_local) / sizeof(expected_spot_local[0])},
        {&point_range, FALSE, expected_point_range,
                sizeof(expected_point_range) / sizeof(expected_point_range[0])},
    };
    IDirect3DDevice8 *device;
    D3DMATERIAL8 material;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i, j, x, y;
    struct
    {
        struct vec3 position;
        struct vec3 normal;
    } *quad;
    WORD *indices;

    quad = HeapAlloc(GetProcessHeap(), 0, vertices_side * vertices_side * sizeof(*quad));
    indices = HeapAlloc(GetProcessHeap(), 0, indices_count * sizeof(*indices));
    for (i = 0, y = 0; y < vertices_side; ++y)
    {
        for (x = 0; x < vertices_side; ++x)
        {
            quad[i].position.x = x * 2.0f / (vertices_side - 1) - 1.0f;
            quad[i].position.y = y * 2.0f / (vertices_side - 1) - 1.0f;
            quad[i].position.z = 1.0f;
            quad[i].normal.x = 0.0f;
            quad[i].normal.y = 0.0f;
            quad[i++].normal.z = -1.0f;
        }
    }
    for (i = 0, y = 0; y < (vertices_side - 1); ++y)
    {
        for (x = 0; x < (vertices_side - 1); ++x)
        {
            indices[i++] = y * vertices_side + x + 1;
            indices[i++] = y * vertices_side + x;
            indices[i++] = (y + 1) * vertices_side + x;
            indices[i++] = y * vertices_side + x + 1;
            indices[i++] = (y + 1) * vertices_side + x;
            indices[i++] = (y + 1) * vertices_side + x + 1;
        }
    }

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z test, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, fvf);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.Specular.r = 1.0f;
    material.Specular.g = 1.0f;
    material.Specular.b = 1.0f;
    material.Specular.a = 1.0f;
    material.Power = 30.0f;
    hr = IDirect3DDevice8_SetMaterial(device, &material);
    ok(SUCCEEDED(hr), "Failed to set material, hr %#x.\n", hr);

    hr = IDirect3DDevice8_LightEnable(device, 0, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable light 0, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SPECULARENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable specular lighting, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i)
    {
        hr = IDirect3DDevice8_SetLight(device, 0, tests[i].light);
        ok(SUCCEEDED(hr), "Failed to set light parameters, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LOCALVIEWER, tests[i].local_viewer);
        ok(SUCCEEDED(hr), "Failed to set local viewer state, hr %#x.\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST,
                0, vertices_side * vertices_side, indices_count / 3, indices,
                D3DFMT_INDEX16, quad, sizeof(quad[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        for (j = 0; j < tests[i].expected_count; ++j)
        {
            color = getPixelColor(device, tests[i].expected[j].x, tests[i].expected[j].y);
            ok(color_match(color, tests[i].expected[j].color, 1),
                    "Expected color 0x%08x at location (%u, %u), got 0x%08x, case %u.\n",
                    tests[i].expected[j].color, tests[i].expected[j].x,
                    tests[i].expected[j].y, color, i);
        }
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
    HeapFree(GetProcessHeap(), 0, indices);
    HeapFree(GetProcessHeap(), 0, quad);
}

static void clear_test(void)
{
    /* Tests the correctness of clearing parameters */
    D3DRECT rect_negneg, rect[2];
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    /* Positive x, negative y */
    rect[0].x1 = 0;
    rect[0].y1 = 480;
    rect[0].x2 = 320;
    rect[0].y2 = 240;

    /* Positive x, positive y */
    rect[1].x1 = 0;
    rect[1].y1 = 0;
    rect[1].x2 = 320;
    rect[1].y2 = 240;
    /* Clear 2 rectangles with one call. Shows that a positive value is returned, but the negative rectangle
     * is ignored, the positive is still cleared afterwards
     */
    hr = IDirect3DDevice8_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.y1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice8_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    rect[0].x1 = 0;
    rect[0].y1 = 0;
    rect[0].x2 = 640;
    rect[0].y2 = 480;
    hr = IDirect3DDevice8_Clear(device, 0, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff), 1),
            "Clear with count = 0, rect != NULL has color %#08x\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_Clear failed with %#08x\n", hr);
    hr = IDirect3DDevice8_Clear(device, 1, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
            "Clear with count = 1, rect = NULL has color %#08x\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void fog_test(void)
{
    float start = 0.0f, end = 1.0f;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    /* Gets full z based fog with linear fog, no fog with specular color. */
    static const struct
    {
        float x, y, z;
        D3DCOLOR diffuse;
        D3DCOLOR specular;
    }
    untransformed_1[] =
    {
        {-1.0f, -1.0f, 0.1f, 0xffff0000, 0xff000000},
        {-1.0f,  0.0f, 0.1f, 0xffff0000, 0xff000000},
        { 0.0f,  0.0f, 0.1f, 0xffff0000, 0xff000000},
        { 0.0f, -1.0f, 0.1f, 0xffff0000, 0xff000000},
    },
    /* Ok, I am too lazy to deal with transform matrices. */
    untransformed_2[] =
    {
        {-1.0f,  0.0f, 1.0f, 0xffff0000, 0xff000000},
        {-1.0f,  1.0f, 1.0f, 0xffff0000, 0xff000000},
        { 0.0f,  1.0f, 1.0f, 0xffff0000, 0xff000000},
        { 0.0f,  0.0f, 1.0f, 0xffff0000, 0xff000000},
    },
    far_quad1[] =
    {
        {-1.0f, -1.0f, 0.5f, 0xffff0000, 0xff000000},
        {-1.0f,  0.0f, 0.5f, 0xffff0000, 0xff000000},
        { 0.0f,  0.0f, 0.5f, 0xffff0000, 0xff000000},
        { 0.0f, -1.0f, 0.5f, 0xffff0000, 0xff000000},
    },
    far_quad2[] =
    {
        {-1.0f,  0.0f, 1.5f, 0xffff0000, 0xff000000},
        {-1.0f,  1.0f, 1.5f, 0xffff0000, 0xff000000},
        { 0.0f,  1.0f, 1.5f, 0xffff0000, 0xff000000},
        { 0.0f,  0.0f, 1.5f, 0xffff0000, 0xff000000},
    };

    /* Untransformed ones. Give them a different diffuse color to make the
     * test look nicer. It also makes making sure that they are drawn
     * correctly easier. */
    static const struct
    {
        float x, y, z, rhw;
        D3DCOLOR diffuse;
        D3DCOLOR specular;
    }
    transformed_1[] =
    {
        {320.0f,   0.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {640.0f,   0.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {640.0f, 240.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {320.0f, 240.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
    },
    transformed_2[] =
    {
        {320.0f, 240.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {640.0f, 240.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {640.0f, 480.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
        {320.0f, 480.0f, 1.0f, 1.0f, 0xffffff00, 0xff000000},
    };
    static const D3DMATRIX ident_mat =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};
    static const D3DMATRIX world_mat1 =
    {{{
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -0.5f, 1.0f,
    }}};
    static const D3DMATRIX world_mat2 =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
    }}};
    static const D3DMATRIX proj_mat =
    {{{
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 1.0f,
    }}};
    static const WORD Indices[] = {0, 1, 2, 2, 3, 0};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetDeviceCaps returned %08x\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %#08x\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable D3DRS_ZENABLE, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Setting fog color returned %#08x\n", hr);
    /* Some of the tests seem to depend on the projection matrix explicitly
     * being set to an identity matrix, even though that's the default.
     * (AMD Radeon HD 6310, Windows 7) */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &ident_mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog returned %#08x\n", hr);

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog start returned %#08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    /* Untransformed, vertex fog = NONE, table fog = NONE:
     * Read the fog weighting from the specular color. */
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, untransformed_1, sizeof(untransformed_1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    /* This makes it use the Z value. */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    /* Untransformed, vertex fog != none (or table fog != none):
     * Use the Z value as input into the equation. */
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, untransformed_2, sizeof(untransformed_2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    /* Transformed vertices. */
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    /* Transformed, vertex fog != NONE, pixel fog == NONE:
     * Use specular color alpha component. */
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, transformed_1, sizeof(transformed_1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    /* Transformed, table fog != none, vertex anything:
     * Use Z value as input to the fog equation. */
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, transformed_2, sizeof(transformed_2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xFF, 0x00, 0x00), 1),
            "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xFF, 0x00), 1),
            "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xFF, 0xFF, 0x00), 1),
            "Transformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xFF, 0x00), 1),
            "Transformed vertex with linear table fog has color %08x\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    if (caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        /* A simple fog + non-identity world matrix test */
        hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLDMATRIX(0), &world_mat1);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %#08x\n", hr);

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %#08x\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
        ok(hr == D3D_OK, "Turning off vertex fog returned %#08x\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %#08x\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                2, Indices, D3DFMT_INDEX16, far_quad1, sizeof(far_quad1[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                2, Indices, D3DFMT_INDEX16, far_quad2, sizeof(far_quad2[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00ff0000, 4), "Unfogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Fogged out quad has color %08x\n", color);

        IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

        /* Test fog behavior with an orthogonal (but not identity) projection matrix */
        hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLDMATRIX(0), &world_mat2);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
        hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &proj_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear returned %#08x\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                2, Indices, D3DFMT_INDEX16, untransformed_1, sizeof(untransformed_1[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                2, Indices, D3DFMT_INDEX16, untransformed_2, sizeof(untransformed_2[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00e51900, 4), "Partially fogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Fogged out quad has color %08x\n", color);

        IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    }
    else
    {
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests\n");
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

/* This tests fog in combination with shaders.
 * What's tested: linear fog (vertex and table) with pixel shader
 *                linear table fog with non foggy vertex shader
 *                vertex fog with foggy vertex shader, non-linear
 *                fog with shader, non-linear fog with foggy shader,
 *                linear table fog with foggy shader */
static void fog_with_shader_test(void)
{
    /* Fill the null-shader entry with the FVF (SetVertexShader is "overloaded" on d3d8...) */
    DWORD vertex_shader[3] = {D3DFVF_XYZ | D3DFVF_DIFFUSE, 0, 0};
    DWORD pixel_shader[2] = {0, 0};
    IDirect3DDevice8 *device;
    unsigned int i, j;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    union
    {
        float f;
        DWORD i;
    } start, end;

    /* Basic vertex shader without fog computation ("non foggy") */
    static const DWORD vertex_shader_code1[] =
    {
        0xfffe0100,                                                             /* vs.1.0                       */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                  */
        0x0000ffff
    };
    /* Basic vertex shader with reversed fog computation ("foggy") */
    static const DWORD vertex_shader_code2[] =
    {
        0xfffe0100,                                                             /* vs.1.0                        */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                  */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                   */
        0x00000002, 0x800f0000, 0x90aa0000, 0xa0aa0000,                         /* add r0, v0.z, c0.z            */
        0x00000005, 0xc00f0001, 0x80000000, 0xa0000000,                         /* mul oFog, r0.x, c0.x          */
        0x0000ffff
    };
    /* Basic pixel shader */
    static const DWORD pixel_shader_code[] =
    {
        0xffff0101,                                                             /* ps_1_1     */
        0x00000001, 0x800f0000, 0x90e40000,                                     /* mov r0, v0 */
        0x0000ffff
    };
    static struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.0f}, 0xffff0000},
    };
    static const DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),  /* position, v0 */
        D3DVSD_REG(1, D3DVSDT_D3DCOLOR),  /* diffuse color, v1 */
        D3DVSD_END()
    };
    static const float vs_constant[4] = {-1.25f, 0.0f, -0.9f, 0.0f};
    /* This reference data was collected on a nVidia GeForce 7600GS
     * driver version 84.19 DirectX version 9.0c on Windows XP */
    static const struct test_data_t
    {
        int vshader;
        int pshader;
        D3DFOGMODE vfog;
        D3DFOGMODE tfog;
        BOOL uninitialized_reg;
        unsigned int color[11];
    }
    test_data[] =
    {
        /* Only pixel shader */
        {0, 1, D3DFOG_NONE, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_EXP, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_EXP2, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_LINEAR, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_LINEAR, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* Vertex shader */
        {1, 0, D3DFOG_NONE, D3DFOG_NONE, TRUE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
         0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_NONE, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_EXP, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        {1, 0, D3DFOG_EXP2, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_LINEAR, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* Vertex shader and pixel shader */
        /* The next 4 tests would read the fog coord output, but it isn't available.
         * The result is a fully fogged quad, no matter what the Z coord is. */
        {1, 1, D3DFOG_NONE, D3DFOG_NONE, TRUE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_LINEAR, D3DFOG_NONE, TRUE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP, D3DFOG_NONE, TRUE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP2, D3DFOG_NONE, TRUE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},

        /* These use the Z coordinate with linear table fog */
        {1, 1, D3DFOG_NONE, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP2, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_LINEAR, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* Non-linear table fog without fog coord */
        {1, 1, D3DFOG_NONE, D3DFOG_EXP, FALSE,
        {0x00ff0000, 0x00e71800, 0x00d12e00, 0x00bd4200, 0x00ab5400, 0x009b6400,
        0x008d7200, 0x007f8000, 0x00738c00, 0x00689700, 0x005ea100}},
        {1, 1, D3DFOG_NONE, D3DFOG_EXP2, FALSE,
        {0x00fd0200, 0x00f50200, 0x00f50a00, 0x00e91600, 0x00d92600, 0x00c73800,
        0x00b24d00, 0x009c6300, 0x00867900, 0x00728d00, 0x005ea100}},

        /* These tests fail on older Nvidia drivers */
        /* Foggy vertex shader */
        {2, 0, D3DFOG_NONE, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_EXP, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_EXP2, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_LINEAR, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* Foggy vertex shader and pixel shader. First 4 tests with vertex fog,
         * all using the fixed fog-coord linear fog */
        {2, 1, D3DFOG_NONE, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_EXP, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_EXP2, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_LINEAR, D3DFOG_NONE, FALSE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* These use table fog. Here the shader-provided fog coordinate is
         * ignored and the z coordinate used instead */
        {2, 1, D3DFOG_NONE, D3DFOG_EXP, FALSE,
        {0x00ff0000, 0x00e71800, 0x00d12e00, 0x00bd4200, 0x00ab5400, 0x009b6400,
        0x008d7200, 0x007f8000, 0x00738c00, 0x00689700, 0x005ea100}},
        {2, 1, D3DFOG_NONE, D3DFOG_EXP2, FALSE,
        {0x00fd0200, 0x00f50200, 0x00f50a00, 0x00e91600, 0x00d92600, 0x00c73800,
        0x00b24d00, 0x009c6300, 0x00867900, 0x00728d00, 0x005ea100}},
        {2, 1, D3DFOG_NONE, D3DFOG_LINEAR, FALSE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
    };
    static const D3DMATRIX identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1) || caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("No vs_1_1 / ps_1_1 support, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    /* NOTE: changing these values will not affect the tests with foggy vertex
     * shader, as the values are hardcoded in the shader constant. */
    start.f = 0.1f;
    end.f = 0.9f;

    /* Some of the tests seem to depend on the projection matrix explicitly
     * being set to an identity matrix, even though that's the default.
     * (AMD Radeon HD 6310, Windows 7) */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateVertexShader(device, decl, vertex_shader_code1, &vertex_shader[1], 0);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl, vertex_shader_code2, &vertex_shader[2], 0);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, pixel_shader_code, &pixel_shader[1]);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);

    /* Set shader constant value */
    hr = IDirect3DDevice8_SetVertexShader(device, vertex_shader[2]);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, vs_constant, 1);
    ok(hr == D3D_OK, "Setting vertex shader constant failed (%08x)\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting failed (%08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations failed (%08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Setting fog color failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog failed (%08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog failed (%08x)\n", hr);

    /* Use fogtart = 0.1 and end = 0.9 to test behavior outside the fog transition phase, too */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGSTART, start.i);
    ok(hr == D3D_OK, "Setting fog start failed (%08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGEND, end.i);
    ok(hr == D3D_OK, "Setting fog end failed (%08x)\n", hr);

    for (i = 0; i < sizeof(test_data)/sizeof(test_data[0]); ++i)
    {
        hr = IDirect3DDevice8_SetVertexShader(device, vertex_shader[test_data[i].vshader]);
        ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
        hr = IDirect3DDevice8_SetPixelShader(device, pixel_shader[test_data[i].pshader]);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, test_data[i].vfog);
        ok( hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR failed (%08x)\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, test_data[i].tfog);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR failed (%08x)\n", hr);

        for(j = 0; j < 11; ++j)
        {
            /* Don't use the whole zrange to prevent rounding errors */
            quad[0].position.z = 0.001f + j / 10.02f;
            quad[1].position.z = 0.001f + j / 10.02f;
            quad[2].position.z = 0.001f + j / 10.02f;
            quad[3].position.z = 0.001f + j / 10.02f;

            hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff00ff, 1.0f, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);

            hr = IDirect3DDevice8_BeginScene(device);
            ok( hr == D3D_OK, "BeginScene returned failed (%08x)\n", hr);

            hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
            ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice8_EndScene(device);
            ok(hr == D3D_OK, "EndScene failed (%08x)\n", hr);

            /* As the red and green component are the result of blending use 5% tolerance on the expected value */
            color = getPixelColor(device, 128, 240);
            ok(color_match(color, test_data[i].color[j], 13) || broken(test_data[i].uninitialized_reg),
                    "fog vs%i ps%i fvm%i ftm%i %d: got color %08x, expected %08x +-5%%\n",
                    test_data[i].vshader, test_data[i].pshader,
                    test_data[i].vfog, test_data[i].tfog, j, color, test_data[i].color[j]);
        }
    }
    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    IDirect3DDevice8_DeleteVertexShader(device, vertex_shader[1]);
    IDirect3DDevice8_DeleteVertexShader(device, vertex_shader[2]);
    IDirect3DDevice8_DeleteVertexShader(device, pixel_shader[1]);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void cnd_test(void)
{
    DWORD shader_11_coissue_2, shader_12_coissue_2, shader_13_coissue_2, shader_14_coissue_2;
    DWORD shader_11_coissue, shader_12_coissue, shader_13_coissue, shader_14_coissue;
    DWORD shader_11, shader_12, shader_13, shader_14;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    /* ps 1.x shaders are rather picky with writemasks and source swizzles.
     * The dp3 is used to copy r0.r to all components of r1, then copy r1.a to
     * r0.a. Essentially it does a mov r0.a, r0.r, which isn't allowed as-is
     * in 1.x pixel shaders. */
    static const DWORD shader_code_11[] =
    {
        0xffff0101,                                                                 /* ps_1_1               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, t0           */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3 r1, r0, c0       */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    static const DWORD shader_code_12[] =
    {
        0xffff0102,                                                                 /* ps_1_2               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, t0           */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3 r1, r0, c0       */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    static const DWORD shader_code_13[] =
    {
        0xffff0103,                                                                 /* ps_1_3               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, t0           */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3, r1, r0, c0      */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    static const DWORD shader_code_14[] =
    {
        0xffff0104,                                                                 /* ps_1_3               */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,     /* def c0, 0, 0, 0, 1   */
        0x00000040, 0x80070000, 0xb0e40000,                                         /* texcrd r0, t0        */
        0x00000001, 0x80080000, 0xa0ff0000,                                         /* mov r0.a, c0.a       */
        0x00000050, 0x800f0000, 0x80e40000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0, c1, c2   */
        0x0000ffff                                                                  /* end                  */
    };

    /* Special fun: The coissue flag on cnd: Apparently cnd always selects the 2nd source,
     * as if the src0 comparison against 0.5 always evaluates to true. The coissue flag isn't
     * set by the compiler, it was added manually after compilation. Note that the COISSUE
     * flag on a color(.xyz) operation is only allowed after an alpha operation. DirectX doesn't
     * have proper docs, but GL_ATI_fragment_shader explains the pairing of color and alpha ops
     * well enough.
     *
     * The shader attempts to test the range [-1;1] against coissued cnd, which is a bit tricky.
     * The input from t0 is [0;1]. 0.5 is subtracted, then we have to multiply with 2. Since
     * constants are clamped to [-1;1], a 2.0 is constructed by adding c0.r(=1.0) to c0.r into r1.r,
     * then r1(2.0, 0.0, 0.0, 0.0) is passed to dp3(explained above).
     */
    static const DWORD shader_code_11_coissue[] =
    {
        0xffff0101,                                                             /* ps_1_1                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.xyz, r0.a, c1, c2*/
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_11_coissue_2[] =
    {
        0xffff0101,                                                             /* ps_1_1                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x800f0000, 0x80e40001,                                     /* mov r0, r1               */
        0x00000001, 0x80070000, 0x80ff0001,                                     /* mov r0.xyz, r1.a         */
        0x40000050, 0x80080000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.a, r0.a, c1, c2  */
        0x00000001, 0x80070000, 0x80ff0000,                                     /* mov r0.xyz, r0.a         */
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_12_coissue[] =
    {
        0xffff0102,                                                             /* ps_1_2                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.xyz, r0.a, c1, c2*/
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_12_coissue_2[] =
    {
        0xffff0102,                                                             /* ps_1_2                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x800f0000, 0x80e40001,                                     /* mov r0, r1               */
        0x00000001, 0x80070000, 0x80ff0001,                                     /* mov r0.xyz, r1.a         */
        0x40000050, 0x80080000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.a, r0.a, c1, c2  */
        0x00000001, 0x80070000, 0x80ff0000,                                     /* mov r0.xyz, r0.a         */
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_13_coissue[] =
    {
        0xffff0103,                                                             /* ps_1_3                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.xyz, r0.a, c1, c2*/
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_13_coissue_2[] =
    {
        0xffff0103,                                                             /* ps_1_3                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3 r1, r0, r1           */
        0x00000001, 0x800f0000, 0x80e40001,                                     /* mov r0, r1               */
        0x00000001, 0x80070000, 0x80ff0001,                                     /* mov r0.xyz, r1.a         */
        0x40000050, 0x80080000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.a, r0.a, c1, c2  */
        0x00000001, 0x80070000, 0x80ff0000,                                     /* mov r0.xyz, r0.a         */
        0x0000ffff                                                              /* end                      */
    };
    /* ps_1_4 does not have a different cnd behavior, just pass the [0;1]
     * texcrd result to cnd, it will compare against 0.5. */
    static const DWORD shader_code_14_coissue[] =
    {
        0xffff0104,                                                             /* ps_1_4                   */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0, 0, 0, 1       */
        0x00000040, 0x80070000, 0xb0e40000,                                     /* texcrd r0.xyz, t0        */
        0x00000001, 0x80080000, 0xa0ff0000,                                     /* mov r0.a, c0.a           */
        0x40000050, 0x80070000, 0x80e40000, 0xa0e40001, 0xa0e40002,             /* +cnd r0.xyz, r0, c1, c2  */
        0x0000ffff                                                              /* end                      */
    };
    static const DWORD shader_code_14_coissue_2[] =
    {
        0xffff0104,                                                             /* ps_1_4                   */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0, 0, 0, 1       */
        0x00000040, 0x80070000, 0xb0e40000,                                     /* texcrd r0.xyz, t0        */
        0x00000001, 0x80080000, 0x80000000,                                     /* mov r0.a, r0.x           */
        0x00000001, 0x80070001, 0xa0ff0000,                                     /* mov r1.xyz, c0.a         */
        0x40000050, 0x80080001, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* +cnd r1.a, r0.a, c1, c2  */
        0x00000001, 0x80070000, 0x80ff0001,                                     /* mov r0.xyz, r1.a         */
        0x00000001, 0x80080000, 0xa0ff0000,                                     /* mov r0.a, c0.a           */
        0x0000ffff                                                              /* end                      */
    };
    static const float quad1[] =
    {
        -1.0f,   -1.0f,   0.1f,     0.0f,    0.0f,    1.0f,
        -1.0f,    0.0f,   0.1f,     0.0f,    1.0f,    0.0f,
         0.0f,   -1.0f,   0.1f,     1.0f,    0.0f,    1.0f,
         0.0f,    0.0f,   0.1f,     1.0f,    1.0f,    0.0f
    };
    static const float quad2[] =
    {
         0.0f,   -1.0f,   0.1f,     0.0f,    0.0f,    1.0f,
         0.0f,    0.0f,   0.1f,     0.0f,    1.0f,    0.0f,
         1.0f,   -1.0f,   0.1f,     1.0f,    0.0f,    1.0f,
         1.0f,    0.0f,   0.1f,     1.0f,    1.0f,    0.0f
    };
    static const float quad3[] =
    {
         0.0f,    0.0f,   0.1f,     0.0f,    0.0f,    1.0f,
         0.0f,    1.0f,   0.1f,     0.0f,    1.0f,    0.0f,
         1.0f,    0.0f,   0.1f,     1.0f,    0.0f,    1.0f,
         1.0f,    1.0f,   0.1f,     1.0f,    1.0f,    0.0f
    };
    static const float quad4[] =
    {
        -1.0f,    0.0f,   0.1f,     0.0f,    0.0f,    1.0f,
        -1.0f,    1.0f,   0.1f,     0.0f,    1.0f,    0.0f,
         0.0f,    0.0f,   0.1f,     1.0f,    0.0f,    1.0f,
         0.0f,    1.0f,   0.1f,     1.0f,    1.0f,    0.0f
    };
    static const float test_data_c1[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    static const float test_data_c2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const float test_data_c1_coi[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    static const float test_data_c2_coi[4] = {1.0f, 0.0f, 1.0f, 1.0f};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 4))
    {
        skip("No ps_1_4 support, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %08x\n", hr);

    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_12, &shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_13, &shader_13);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_14, &shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_11_coissue, &shader_11_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_12_coissue, &shader_12_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_13_coissue, &shader_13_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_14_coissue, &shader_14_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_11_coissue_2, &shader_11_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_12_coissue_2, &shader_12_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_13_coissue_2, &shader_13_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_14_coissue_2, &shader_14_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShaderConstant(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShaderConstant returned %08x\n", hr);
    hr = IDirect3DDevice8_SetPixelShaderConstant(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShaderConstant returned %08x\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_13);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_EndScene returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);

    /* This is the 1.4 test. Each component(r, g, b) is tested separately against 0.5 */
    color = getPixelColor(device, 158, 118);
    ok(color == 0x00ff00ff, "pixel 158, 118 has color %08x, expected 0x00ff00ff\n", color);
    color = getPixelColor(device, 162, 118);
    ok(color == 0x000000ff, "pixel 162, 118 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 158, 122);
    ok(color == 0x00ffffff, "pixel 162, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 122);
    ok(color == 0x0000ffff, "pixel 162, 122 has color %08x, expected 0x0000ffff\n", color);

    /* 1.1 shader. All 3 components get set, based on the .w comparison */
    color = getPixelColor(device, 158, 358);
    ok(color == 0x00ffffff, "pixel 158, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 358);
    ok(color_match(color, 0x00000000, 1),
            "pixel 162, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 158, 362);
    ok(color == 0x00ffffff, "pixel 158, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color_match(color, 0x00000000, 1),
            "pixel 162, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 478, 358);
    ok(color == 0x00ffffff, "pixel 478, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color_match(color, 0x00000000, 1),
            "pixel 482, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x00ffffff, "pixel 478, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color_match(color, 0x00000000, 1),
            "pixel 482, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 478, 118);
    ok(color == 0x00ffffff, "pixel 478, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 118);
    ok(color_match(color, 0x00000000, 1),
            "pixel 482, 118 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color == 0x00ffffff, "pixel 478, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color_match(color, 0x00000000, 1),
            "pixel 482, 122 has color %08x, expected 0x00000000\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice8_Present failed with %08x\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %08x\n", hr);
    hr = IDirect3DDevice8_SetPixelShaderConstant(device, 1, test_data_c1_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShaderConstant returned %08x\n", hr);
    hr = IDirect3DDevice8_SetPixelShaderConstant(device, 2, test_data_c2_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShaderConstant returned %08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_11_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_12_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_13_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_14_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_EndScene returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);

    /* This is the 1.4 test. The coissue doesn't change the behavior here, but keep in mind
     * that we swapped the values in c1 and c2 to make the other tests return some color
     */
    color = getPixelColor(device, 158, 118);
    ok(color == 0x00ffffff, "pixel 158, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 118);
    ok(color == 0x0000ffff, "pixel 162, 118 has color %08x, expected 0x0000ffff\n", color);
    color = getPixelColor(device, 158, 122);
    ok(color == 0x00ff00ff, "pixel 162, 122 has color %08x, expected 0x00ff00ff\n", color);
    color = getPixelColor(device, 162, 122);
    ok(color == 0x000000ff, "pixel 162, 122 has color %08x, expected 0x000000ff\n", color);

    /* 1.1 shader. coissue flag changed the semantic of cnd, c1 is always selected
     * (The Win7 nvidia driver always selects c2)
     */
    color = getPixelColor(device, 158, 358);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 158, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 162, 358);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 162, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 158, 362);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 158, 362 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 162, 362 has color %08x, expected 0x0000ff00\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 478, 358);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 478, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 482, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 478, 362 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 482, 362 has color %08x, expected 0x0000ff00\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 478, 118);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 478, 118 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 118);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 482, 118 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 478, 122 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color_match(color, 0x0000ff00, 1) || broken(color_match(color, 0x00ff00ff, 1)),
            "pixel 482, 122 has color %08x, expected 0x0000ff00\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice8_Present failed with %08x\n", hr);

    /* Retest with the coissue flag on the alpha instruction instead. This
     * works "as expected". The Windows 8 testbot (WARP) seems to handle this
     * the same as coissue on .rgb. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene returned %08x\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_11_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_12_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_13_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_SetPixelShader(device, shader_14_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
    ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_EndScene returned %08x\n", hr);

    /* 1.4 shader */
    color = getPixelColor(device, 158, 118);
    ok(color == 0x00ffffff, "pixel 158, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 118);
    ok(color == 0x00000000, "pixel 162, 118 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 158, 122);
    ok(color == 0x00ffffff, "pixel 162, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 122);
    ok(color == 0x00000000, "pixel 162, 122 has color %08x, expected 0x00000000\n", color);

    /* 1.1 shader */
    color = getPixelColor(device, 238, 358);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 238, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 242, 358);
    ok(color_match(color, 0x00000000, 1),
            "pixel 242, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 238, 362);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 238, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 242, 362);
    ok(color_match(color, 0x00000000, 1),
            "pixel 242, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 558, 358);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 558, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 358);
    ok(color_match(color, 0x00000000, 1),
            "pixel 562, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 558, 362);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 558, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 362);
    ok(color_match(color, 0x00000000, 1),
            "pixel 562, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 558, 118);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 558, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 118);
    ok(color_match(color, 0x00000000, 1),
            "pixel 562, 118 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 558, 122);
    ok(color_match(color, 0x00ffffff, 1) || broken(color_match(color, 0x00000000, 1)),
            "pixel 558, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 122);
    ok(color_match(color, 0x00000000, 1),
            "pixel 562, 122 has color %08x, expected 0x00000000\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice8_Present failed with %08x\n", hr);

    IDirect3DDevice8_DeletePixelShader(device, shader_14_coissue_2);
    IDirect3DDevice8_DeletePixelShader(device, shader_13_coissue_2);
    IDirect3DDevice8_DeletePixelShader(device, shader_12_coissue_2);
    IDirect3DDevice8_DeletePixelShader(device, shader_11_coissue_2);
    IDirect3DDevice8_DeletePixelShader(device, shader_14_coissue);
    IDirect3DDevice8_DeletePixelShader(device, shader_13_coissue);
    IDirect3DDevice8_DeletePixelShader(device, shader_12_coissue);
    IDirect3DDevice8_DeletePixelShader(device, shader_11_coissue);
    IDirect3DDevice8_DeletePixelShader(device, shader_14);
    IDirect3DDevice8_DeletePixelShader(device, shader_13);
    IDirect3DDevice8_DeletePixelShader(device, shader_12);
    IDirect3DDevice8_DeletePixelShader(device, shader_11);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void z_range_test(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD shader;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, 0.0f,  1.1f}, 0xffff0000},
        {{-1.0f, 1.0f,  1.1f}, 0xffff0000},
        {{ 1.0f, 0.0f, -1.1f}, 0xffff0000},
        {{ 1.0f, 1.0f, -1.1f}, 0xffff0000},
    },
    quad2[] =
    {
        {{-1.0f, 0.0f,  1.1f}, 0xff0000ff},
        {{-1.0f, 1.0f,  1.1f}, 0xff0000ff},
        {{ 1.0f, 0.0f, -1.1f}, 0xff0000ff},
        {{ 1.0f, 1.0f, -1.1f}, 0xff0000ff},
    };
    static const struct
    {
        struct vec4 position;
        DWORD diffuse;
    }
    quad3[] =
    {
        {{640.0f, 240.0f, -1.1f, 1.0f}, 0xffffff00},
        {{640.0f, 480.0f, -1.1f, 1.0f}, 0xffffff00},
        {{  0.0f, 240.0f,  1.1f, 1.0f}, 0xffffff00},
        {{  0.0f, 480.0f,  1.1f, 1.0f}, 0xffffff00},
    },
    quad4[] =
    {
        {{640.0f, 240.0f, -1.1f, 1.0f}, 0xff00ff00},
        {{640.0f, 480.0f, -1.1f, 1.0f}, 0xff00ff00},
        {{  0.0f, 240.0f,  1.1f, 1.0f}, 0xff00ff00},
        {{  0.0f, 480.0f,  1.1f, 1.0f}, 0xff00ff00},
    };
    static const DWORD shader_code[] =
    {
        0xfffe0101,                                     /* vs_1_1           */
        0x00000001, 0xc00f0000, 0x90e40000,             /* mov oPos, v0     */
        0x00000001, 0xd00f0000, 0xa0e40000,             /* mov oD0, c0      */
        0x0000ffff                                      /* end              */
    };
    static const float color_const_1[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const float color_const_2[] = {0.0f, 0.0f, 1.0f, 1.0f};
    static const DWORD vertex_declaration[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),
        D3DVSD_END()
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    /* Does the Present clear the depth stencil? Clear the depth buffer with some value != 0,
     * then call Present. Then clear the color buffer to make sure it has some defined content
     * after the Present with D3DSWAPEFFECT_DISCARD. After that draw a plane that is somewhere cut
     * by the depth value. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.75f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disabled lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z test, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z writes, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* Test the untransformed vertex path */
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    /* Test the transformed vertex path */
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(quad4[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(quad3[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    /* Do not test the exact corner pixels, but go pretty close to them */

    /* Clipped because z > 1.0 */
    color = getPixelColor(device, 28, 238);
    ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 28, 241);
    if (caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS)
        ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    else
        ok(color_match(color, 0x00ffff00, 0), "Z range failed: Got color 0x%08x, expected 0x00ffff00.\n", color);

    /* Not clipped, > z buffer clear value(0.75).
     *
     * On the r500 driver on Windows D3DCMP_GREATER and D3DCMP_GREATEREQUAL are broken for depth
     * values > 0.5. The range appears to be distorted, apparently an incoming value of ~0.875 is
     * equal to a stored depth buffer value of 0.5. */
    color = getPixelColor(device, 31, 238);
    ok(color_match(color, 0x00ff0000, 0), "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 31, 241);
    ok(color_match(color, 0x00ffff00, 0), "Z range failed: Got color 0x%08x, expected 0x00ffff00.\n", color);
    color = getPixelColor(device, 100, 238);
    ok(color_match(color, 0x00ff0000, 0) || broken(color_match(color, 0x00ffffff, 0)),
            "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 100, 241);
    ok(color_match(color, 0x00ffff00, 0) || broken(color_match(color, 0x00ffffff, 0)),
            "Z range failed: Got color 0x%08x, expected 0x00ffff00.\n", color);

    /* Not clipped, < z buffer clear value */
    color = getPixelColor(device, 104, 238);
    ok(color_match(color, 0x000000ff, 0), "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 104, 241);
    ok(color_match(color, 0x0000ff00, 0), "Z range failed: Got color 0x%08x, expected 0x0000ff00.\n", color);
    color = getPixelColor(device, 318, 238);
    ok(color_match(color, 0x000000ff, 0), "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 318, 241);
    ok(color_match(color, 0x0000ff00, 0), "Z range failed: Got color 0x%08x, expected 0x0000ff00.\n", color);

    /* Clipped because z < 0.0 */
    color = getPixelColor(device, 321, 238);
    ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 321, 241);
    if (caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS)
        ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    else
        ok(color_match(color, 0x0000ff00, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Test the shader path */
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
    {
        skip("Vertex shaders not supported\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    hr = IDirect3DDevice8_CreateVertexShader(device, vertex_declaration, shader_code, &shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, color_const_1, 1);
    ok(SUCCEEDED(hr), "Failed to set vs constant 0, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, color_const_2, 1);
    ok(SUCCEEDED(hr), "Failed to set vs constant 0, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, 0);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DeleteVertexShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to delete vertex shader, hr %#x.\n", hr);

    /* Z < 1.0 */
    color = getPixelColor(device, 28, 238);
    ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    /* 1.0 < z < 0.75 */
    color = getPixelColor(device, 31, 238);
    ok(color_match(color, 0x00ff0000, 0), "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 100, 238);
    ok(color_match(color, 0x00ff0000, 0) || broken(color_match(color, 0x00ffffff, 0)),
            "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);

    /* 0.75 < z < 0.0 */
    color = getPixelColor(device, 104, 238);
    ok(color_match(color, 0x000000ff, 0), "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 318, 238);
    ok(color_match(color, 0x000000ff, 0), "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);

    /* 0.0 < z */
    color = getPixelColor(device, 321, 238);
    ok(color_match(color, 0x00ffffff, 0), "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_scalar_instructions(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD shader;
    HWND window;
    HRESULT hr;

    static const struct vec3 quad[] =
    {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };
    static const DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),                           /* dcl_position v0 */
        D3DVSD_CONST(0, 1), 0x3e800000, 0x3f000000, 0x3f800000, 0x40000000,     /* def c0, 0.25, 0.5, 1.0, 2.0 */
        D3DVSD_END()
    };
    static const DWORD rcp_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x00000006, 0xd00f0000, 0xa0e40000,                 /* rcp oD0, c0 */
        0x0000ffff                                          /* END */
    };
    static const DWORD rsq_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x00000007, 0xd00f0000, 0xa0e40000,                 /* rsq oD0, c0 */
        0x0000ffff                                          /* END */
    };
    static const DWORD exp_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x0000000e, 0x800f0000, 0xa0e40000,                 /* exp r0, c0 */
        0x00000006, 0xd00f0000, 0x80000000,                 /* rcp oD0, r0.x */
        0x0000ffff,                                         /* END */
    };
    static const DWORD expp_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x0000004e, 0x800f0000, 0xa0e40000,                 /* expp r0, c0 */
        0x00000006, 0xd00f0000, 0x80000000,                 /* rcp oD0, r0.x */
        0x0000ffff,                                         /* END */
    };
    static const DWORD log_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x0000000f, 0xd00f0000, 0xa0e40000,                 /* log oD0, c0 */
        0x0000ffff,                                         /* END */
    };
    static const DWORD logp_test[] =
    {
        0xfffe0101,                                         /* vs_1_1 */
        0x0009fffe, 0x30303030, 0x30303030,                 /* Shaders have to have a minimal size.  */
        0x30303030, 0x30303030, 0x30303030,                 /* Add a filler comment. Usually d3dx8's */
        0x30303030, 0x30303030, 0x30303030,                 /* version comment makes the shader big  */
        0x00303030,                                         /* enough to make Windows happy.         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0 */
        0x0000004f, 0xd00f0000, 0xa0e40000,                 /* logp oD0, c0 */
        0x0000ffff,                                         /* END */
    };
    static const struct
    {
        const char *name;
        const DWORD *byte_code;
        D3DCOLOR color;
        /* Some drivers, including Intel HD4000 10.18.10.3345 and VMware SVGA
         * 3D 7.14.1.5025, use the .x component instead of the .w one. */
        D3DCOLOR broken_color;
    }
    test_data[] =
    {
        {"rcp_test",    rcp_test,   D3DCOLOR_ARGB(0x00, 0x80, 0x80, 0x80), D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff)},
        {"rsq_test",    rsq_test,   D3DCOLOR_ARGB(0x00, 0xb4, 0xb4, 0xb4), D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff)},
        {"exp_test",    exp_test,   D3DCOLOR_ARGB(0x00, 0x40, 0x40, 0x40), D3DCOLOR_ARGB(0x00, 0xd6, 0xd6, 0xd6)},
        {"expp_test",   expp_test,  D3DCOLOR_ARGB(0x00, 0x40, 0x40, 0x40), D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff)},
        {"log_test",    log_test,   D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff), D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00)},
        {"logp_test",   logp_test,  D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff), D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00)},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
    {
        skip("No vs_1_1 support, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff336699, 0.0f, 0);
        ok(SUCCEEDED(hr), "%s: Failed to clear, hr %#x.\n", test_data[i].name, hr);

        hr = IDirect3DDevice8_CreateVertexShader(device, decl, test_data[i].byte_code, &shader, 0);
        ok(SUCCEEDED(hr), "%s: Failed to create vertex shader, hr %#x.\n", test_data[i].name, hr);
        hr = IDirect3DDevice8_SetVertexShader(device, shader);
        ok(SUCCEEDED(hr), "%s: Failed to set vertex shader, hr %#x.\n", test_data[i].name, hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "%s: Failed to begin scene, hr %#x.\n", test_data[i].name, hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], 3 * sizeof(float));
        ok(SUCCEEDED(hr), "%s: Failed to draw primitive, hr %#x.\n", test_data[i].name, hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "%s: Failed to end scene, hr %#x.\n", test_data[i].name, hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, test_data[i].color, 4) || broken(color_match(color, test_data[i].broken_color, 4)),
                "%s: Got unexpected color 0x%08x, expected 0x%08x.\n",
                test_data[i].name, color, test_data[i].color);

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "%s: Failed to present, hr %#x.\n", test_data[i].name, hr);

        hr = IDirect3DDevice8_SetVertexShader(device, 0);
        ok(SUCCEEDED(hr), "%s: Failed to set vertex shader, hr %#x.\n", test_data[i].name, hr);
        hr = IDirect3DDevice8_DeleteVertexShader(device, shader);
        ok(SUCCEEDED(hr), "%s: Failed to delete vertex shader, hr %#x.\n", test_data[i].name, hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void offscreen_test(void)
{
    IDirect3DSurface8 *backbuffer, *offscreen, *depthstencil;
    IDirect3DTexture8 *offscreenTexture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const float quad[][5] =
    {
        {-0.5f, -0.5f, 0.1f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.1f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.1f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.1f, 1.0f, 1.0f},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %#08x\n", hr);
    if (!offscreenTexture)
    {
        trace("Failed to create an X8R8G8B8 offscreen texture, trying R5G6B5\n");
        hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &offscreenTexture);
        ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %#08x\n", hr);
        if (!offscreenTexture)
        {
            skip("Cannot create an offscreen render target.\n");
            IDirect3DDevice8_Release(device);
            goto done;
        }
    }

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &depthstencil);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetDepthStencilSurface failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %#08x\n", hr);

    hr = IDirect3DTexture8_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "SetVertexShader failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %#08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %#08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MINFILTER failed (%#08x)\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MAGFILTER failed (%#08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, offscreen, depthstencil);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    /* Draw without textures - Should result in a white quad. */
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depthstencil);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)offscreenTexture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    /* This time with the texture .*/
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    /* Center quad - should be white */
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Offscreen failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    /* Some quad in the cleared part of the texture */
    color = getPixelColor(device, 170, 240);
    ok(color == 0x00ff00ff, "Offscreen failed: Got color 0x%08x, expected 0x00ff00ff.\n", color);
    /* Part of the originally cleared back buffer */
    color = getPixelColor(device, 10, 10);
    ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 10, 470);
    ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    IDirect3DSurface8_Release(backbuffer);
    IDirect3DTexture8_Release(offscreenTexture);
    IDirect3DSurface8_Release(offscreen);
    IDirect3DSurface8_Release(depthstencil);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_blend(void)
{
    IDirect3DSurface8 *backbuffer, *offscreen, *depthstencil;
    IDirect3DTexture8 *offscreenTexture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{-1.0f,  0.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f,  0.0f, 0.1f}, 0x4000ff00},
    },
    quad2[] =
    {
        {{-1.0f,  0.0f, 0.1f}, 0xc00000ff},
        {{-1.0f,  1.0f, 0.1f}, 0xc00000ff},
        {{ 1.0f,  0.0f, 0.1f}, 0xc00000ff},
        {{ 1.0f,  1.0f, 0.1f}, 0xc00000ff},
    };
    static const float composite_quad[][5] =
    {
        { 0.0f, -1.0f, 0.1f, 0.0f, 1.0f},
        { 0.0f,  1.0f, 0.1f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.1f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.1f, 1.0f, 0.0f},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    /* Clear the render target with alpha = 0.5 */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x80ff0000, 1.0f, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &depthstencil);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);

    hr = IDirect3DTexture8_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "SetVertexShader failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %#08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %#08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MINFILTER failed (%#08x)\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MAGFILTER failed (%#08x)\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* Draw two quads, one with src alpha blending, one with dest alpha blending. */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    /* Switch to the offscreen buffer, and redo the testing. The offscreen
     * render target doesn't have an alpha channel. DESTALPHA and INVDESTALPHA
     * "don't work" on render targets without alpha channel, they give
     * essentially ZERO and ONE blend factors. */
    hr = IDirect3DDevice8_SetRenderTarget(device, offscreen, 0);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depthstencil);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    /* Render the offscreen texture onto the frame buffer to be able to
     * compare it regularly. Disable alpha blending for the final
     * composition. */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *) offscreenTexture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, composite_quad, sizeof(float) * 5);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0x40, 0x00), 1),
       "SRCALPHA on frame buffer returned color %08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 160, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x7f, 0x00, 0x80), 2),
       "DSTALPHA on frame buffer returned color %08x, expected 0x007f0080\n", color);

    color = getPixelColor(device, 480, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0x40, 0x00), 1),
       "SRCALPHA on texture returned color %08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 480, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0xff), 1),
       "DSTALPHA on texture returned color %08x, expected 0x000000ff\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    IDirect3DSurface8_Release(backbuffer);
    IDirect3DTexture8_Release(offscreenTexture);
    IDirect3DSurface8_Release(offscreen);
    IDirect3DSurface8_Release(depthstencil);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void p8_texture_test(void)
{
    IDirect3DTexture8 *texture, *texture2;
    IDirect3DDevice8 *device;
    PALETTEENTRY table[256];
    unsigned char *data;
    D3DLOCKED_RECT lr;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    UINT i;

    static const float quad[] =
    {
        -1.0f,  0.0f, 0.1f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.1f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.1f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.1f, 1.0f, 1.0f,
    };
    static const float quad2[] =
    {
        -1.0f, -1.0f, 0.1f, 0.0f, 0.0f,
        -1.0f,  0.0f, 0.1f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.1f, 1.0f, 0.0f,
         1.0f,  0.0f, 0.1f, 1.0f, 1.0f,
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    if (IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_P8) != D3D_OK)
    {
        skip("D3DFMT_P8 textures not supported.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 1, 1, 1, 0, D3DFMT_P8, D3DPOOL_MANAGED, &texture2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture8_LockRect(texture2, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture8_LockRect failed, hr = %08x\n", hr);
    data = lr.pBits;
    *data = 1;
    hr = IDirect3DTexture8_UnlockRect(texture2, 0);
    ok(hr == D3D_OK, "IDirect3DTexture8_UnlockRect failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 1, 1, 1, 0, D3DFMT_P8, D3DPOOL_MANAGED, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture8_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture8_LockRect failed, hr = %08x\n", hr);
    data = lr.pBits;
    *data = 1;
    hr = IDirect3DTexture8_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture8_UnlockRect failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed, hr = %08x\n", hr);

    /* The first part of the test should work both with and without D3DPTEXTURECAPS_ALPHAPALETTE;
       alpha of every entry is set to 1.0, which MS says is required when there's no
       D3DPTEXTURECAPS_ALPHAPALETTE capability */
    for (i = 0; i < 256; i++) {
        table[i].peRed = table[i].peGreen = table[i].peBlue = 0;
        table[i].peFlags = 0xff;
    }
    table[1].peRed = 0xff;
    hr = IDirect3DDevice8_SetPaletteEntries(device, 0, table);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPaletteEntries failed, hr = %08x\n", hr);

    table[1].peRed = 0;
    table[1].peBlue = 0xff;
    hr = IDirect3DDevice8_SetPaletteEntries(device, 1, table);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetPaletteEntries failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetCurrentTexturePalette(device, 0);
    ok(SUCCEEDED(hr), "Failed to set texture palette, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetCurrentTexturePalette(device, 1);
    ok(SUCCEEDED(hr), "Failed to set texture palette, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 32, 32);
    ok(color_match(color, 0x00ff0000, 0), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 32, 320);
    ok(color_match(color, 0x000000ff, 0), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice8_Present failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 32, 32);
    ok(color_match(color, 0x000000ff, 0), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice8_Present failed, hr = %08x\n", hr);

    /* Test palettes with alpha */
    IDirect3DDevice8_GetDeviceCaps(device, &caps);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE)) {
        skip("no D3DPTEXTURECAPS_ALPHAPALETTE capability, tests with alpha in palette will be skipped\n");
    } else {
        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed, hr = %08x\n", hr);

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed, hr = %08x\n", hr);

        for (i = 0; i < 256; i++) {
            table[i].peRed = table[i].peGreen = table[i].peBlue = 0;
            table[i].peFlags = 0xff;
        }
        table[1].peRed = 0xff;
        table[1].peFlags = 0x80;
        hr = IDirect3DDevice8_SetPaletteEntries(device, 0, table);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetPaletteEntries failed, hr = %08x\n", hr);

        table[1].peRed = 0;
        table[1].peBlue = 0xff;
        table[1].peFlags = 0x80;
        hr = IDirect3DDevice8_SetPaletteEntries(device, 1, table);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetPaletteEntries failed, hr = %08x\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetCurrentTexturePalette(device, 0);
        ok(SUCCEEDED(hr), "Failed to set texture palette, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetCurrentTexturePalette(device, 1);
        ok(SUCCEEDED(hr), "Failed to set texture palette, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 32, 32);
        ok(color_match(color, 0x00800000, 1), "Got unexpected color 0x%08x.\n", color);
        color = getPixelColor(device, 32, 320);
        ok(color_match(color, 0x00000080, 1), "Got unexpected color 0x%08x.\n", color);

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice8_Present failed, hr = %08x\n", hr);
    }

    IDirect3DTexture8_Release(texture);
    IDirect3DTexture8_Release(texture2);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void texop_test(void)
{
    IDirect3DTexture8 *texture;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    static const struct {
        float x, y, z;
        D3DCOLOR diffuse;
        float s, t;
    } quad[] = {
        {-1.0f, -1.0f, 0.1f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00), -1.0f, -1.0f},
        {-1.0f,  1.0f, 0.1f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00), -1.0f,  1.0f},
        { 1.0f, -1.0f, 0.1f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00),  1.0f, -1.0f},
        { 1.0f,  1.0f, 0.1f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00),  1.0f,  1.0f}
    };

    static const struct {
        D3DTEXTUREOP op;
        const char *name;
        DWORD caps_flag;
        D3DCOLOR result;
    } test_data[] = {
        {D3DTOP_SELECTARG1,                "SELECTARG1",                D3DTEXOPCAPS_SELECTARG1,                D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00)},
        {D3DTOP_SELECTARG2,                "SELECTARG2",                D3DTEXOPCAPS_SELECTARG2,                D3DCOLOR_ARGB(0x00, 0x33, 0x33, 0x33)},
        {D3DTOP_MODULATE,                  "MODULATE",                  D3DTEXOPCAPS_MODULATE,                  D3DCOLOR_ARGB(0x00, 0x00, 0x33, 0x00)},
        {D3DTOP_MODULATE2X,                "MODULATE2X",                D3DTEXOPCAPS_MODULATE2X,                D3DCOLOR_ARGB(0x00, 0x00, 0x66, 0x00)},
        {D3DTOP_MODULATE4X,                "MODULATE4X",                D3DTEXOPCAPS_MODULATE4X,                D3DCOLOR_ARGB(0x00, 0x00, 0xcc, 0x00)},
        {D3DTOP_ADD,                       "ADD",                       D3DTEXOPCAPS_ADD,                       D3DCOLOR_ARGB(0x00, 0x33, 0xff, 0x33)},

        {D3DTOP_ADDSIGNED,                 "ADDSIGNED",                 D3DTEXOPCAPS_ADDSIGNED,                 D3DCOLOR_ARGB(0x00, 0x00, 0xb2, 0x00)},
        {D3DTOP_ADDSIGNED2X,               "ADDSIGNED2X",               D3DTEXOPCAPS_ADDSIGNED2X,               D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00)},

        {D3DTOP_SUBTRACT,                  "SUBTRACT",                  D3DTEXOPCAPS_SUBTRACT,                  D3DCOLOR_ARGB(0x00, 0x00, 0xcc, 0x00)},
        {D3DTOP_ADDSMOOTH,                 "ADDSMOOTH",                 D3DTEXOPCAPS_ADDSMOOTH,                 D3DCOLOR_ARGB(0x00, 0x33, 0xff, 0x33)},
        {D3DTOP_BLENDDIFFUSEALPHA,         "BLENDDIFFUSEALPHA",         D3DTEXOPCAPS_BLENDDIFFUSEALPHA,         D3DCOLOR_ARGB(0x00, 0x22, 0x77, 0x22)},
        {D3DTOP_BLENDTEXTUREALPHA,         "BLENDTEXTUREALPHA",         D3DTEXOPCAPS_BLENDTEXTUREALPHA,         D3DCOLOR_ARGB(0x00, 0x14, 0xad, 0x14)},
        {D3DTOP_BLENDFACTORALPHA,          "BLENDFACTORALPHA",          D3DTEXOPCAPS_BLENDFACTORALPHA,          D3DCOLOR_ARGB(0x00, 0x07, 0xe4, 0x07)},
        {D3DTOP_BLENDTEXTUREALPHAPM,       "BLENDTEXTUREALPHAPM",       D3DTEXOPCAPS_BLENDTEXTUREALPHAPM,       D3DCOLOR_ARGB(0x00, 0x14, 0xff, 0x14)},
        {D3DTOP_BLENDCURRENTALPHA,         "BLENDCURRENTALPHA",         D3DTEXOPCAPS_BLENDCURRENTALPHA,         D3DCOLOR_ARGB(0x00, 0x22, 0x77, 0x22)},
        {D3DTOP_MODULATEALPHA_ADDCOLOR,    "MODULATEALPHA_ADDCOLOR",    D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR,    D3DCOLOR_ARGB(0x00, 0x1f, 0xff, 0x1f)},
        {D3DTOP_MODULATECOLOR_ADDALPHA,    "MODULATECOLOR_ADDALPHA",    D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA,    D3DCOLOR_ARGB(0x00, 0x99, 0xcc, 0x99)},
        {D3DTOP_MODULATEINVALPHA_ADDCOLOR, "MODULATEINVALPHA_ADDCOLOR", D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR, D3DCOLOR_ARGB(0x00, 0x14, 0xff, 0x14)},
        {D3DTOP_MODULATEINVCOLOR_ADDALPHA, "MODULATEINVCOLOR_ADDALPHA", D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA, D3DCOLOR_ARGB(0x00, 0xcc, 0x99, 0xcc)},
        /* BUMPENVMAP & BUMPENVMAPLUMINANCE have their own tests */
        {D3DTOP_DOTPRODUCT3,               "DOTPRODUCT2",               D3DTEXOPCAPS_DOTPRODUCT3,               D3DCOLOR_ARGB(0x00, 0x99, 0x99, 0x99)},
        {D3DTOP_MULTIPLYADD,               "MULTIPLYADD",               D3DTEXOPCAPS_MULTIPLYADD,               D3DCOLOR_ARGB(0x00, 0xff, 0x33, 0x00)},
        {D3DTOP_LERP,                      "LERP",                      D3DTEXOPCAPS_LERP,                      D3DCOLOR_ARGB(0x00, 0x00, 0x33, 0x33)},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX0);
    ok(SUCCEEDED(hr), "SetVertexShader failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed with 0x%08x\n", hr);
    hr = IDirect3DTexture8_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    *((DWORD *)locked_rect.pBits) = D3DCOLOR_ARGB(0x99, 0x00, 0xff, 0x00);
    hr = IDirect3DTexture8_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG0, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0xdd333333);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        if (!(caps.TextureOpCaps & test_data[i].caps_flag))
        {
            skip("tex operation %s not supported\n", test_data[i].name);
            continue;
        }

        hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, test_data[i].op);
        ok(SUCCEEDED(hr), "SetTextureStageState (%s) failed with 0x%08x\n", test_data[i].name, hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed with 0x%08x\n", hr);

        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);

        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed with 0x%08x\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, test_data[i].result, 3), "Operation %s returned color 0x%08x, expected 0x%08x\n",
                test_data[i].name, color, test_data[i].result);

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed with 0x%08x\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
    }

    IDirect3DTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

/* This test tests depth clamping / clipping behaviour:
 *   - With software vertex processing, depth values are clamped to the
 *     minimum / maximum z value when D3DRS_CLIPPING is disabled, and clipped
 *     when D3DRS_CLIPPING is enabled. Pretransformed vertices behave the
 *     same as regular vertices here.
 *   - With hardware vertex processing, D3DRS_CLIPPING seems to be ignored.
 *     Normal vertices are always clipped. Pretransformed vertices are
 *     clipped when D3DPMISCCAPS_CLIPTLVERTS is set, clamped when it isn't.
 *   - The viewport's MinZ/MaxZ is irrelevant for this.
 */
static void depth_clamp_test(void)
{
    IDirect3DDevice8 *device;
    D3DVIEWPORT8 vp;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec4 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{  0.0f,   0.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{640.0f,   0.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{  0.0f, 480.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{640.0f, 480.0f,  5.0f, 1.0f}, 0xff002b7f},
    },
    quad2[] =
    {
        {{  0.0f, 300.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{640.0f, 300.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{  0.0f, 360.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{640.0f, 360.0f, 10.0f, 1.0f}, 0xfff9e814},
    },
    quad3[] =
    {
        {{112.0f, 108.0f,  5.0f, 1.0f}, 0xffffffff},
        {{208.0f, 108.0f,  5.0f, 1.0f}, 0xffffffff},
        {{112.0f, 204.0f,  5.0f, 1.0f}, 0xffffffff},
        {{208.0f, 204.0f,  5.0f, 1.0f}, 0xffffffff},
    },
    quad4[] =
    {
        {{ 42.0f,  41.0f, 10.0f, 1.0f}, 0xffffffff},
        {{112.0f,  41.0f, 10.0f, 1.0f}, 0xffffffff},
        {{ 42.0f, 108.0f, 10.0f, 1.0f}, 0xffffffff},
        {{112.0f, 108.0f, 10.0f, 1.0f}, 0xffffffff},
    };
    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad5[] =
    {
        {{-0.5f,  0.5f,  10.0f}, 0xff14f914},
        {{ 0.5f,  0.5f,  10.0f}, 0xff14f914},
        {{-0.5f, -0.5f,  10.0f}, 0xff14f914},
        {{ 0.5f, -0.5f,  10.0f}, 0xff14f914},
    },
    quad6[] =
    {
        {{-1.0f,  0.5f,  10.0f}, 0xfff91414},
        {{ 1.0f,  0.5f,  10.0f}, 0xfff91414},
        {{-1.0f,  0.25f, 10.0f}, 0xfff91414},
        {{ 1.0f,  0.25f, 10.0f}, 0xfff91414},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    vp.X = 0;
    vp.Y = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0;
    vp.MaxZ = 7.5;

    hr = IDirect3DDevice8_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetVertexSahder failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(*quad3));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(*quad4));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad5, sizeof(*quad5));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad6, sizeof(*quad6));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    if (caps.PrimitiveMiscCaps & D3DPMISCCAPS_CLIPTLVERTS)
    {
        color = getPixelColor(device, 75, 75);
        ok(color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 150, 150);
        ok(color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 240);
        ok(color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 330);
        ok(color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 330);
        ok(color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    }
    else
    {
        color = getPixelColor(device, 75, 75);
        ok(color_match(color, 0x00ffffff, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 150, 150);
        ok(color_match(color, 0x00ffffff, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 240);
        ok(color_match(color, 0x00002b7f, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 330);
        ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);
        color = getPixelColor(device, 320, 330);
        ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void depth_buffer_test(void)
{
    IDirect3DSurface8 *backbuffer, *rt1, *rt2, *rt3;
    IDirect3DSurface8 *depth_stencil;
    IDirect3DDevice8 *device;
    unsigned int i, j;
    D3DVIEWPORT8 vp;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{-1.0f,  1.0f, 0.33f}, 0xff00ff00},
        {{ 1.0f,  1.0f, 0.33f}, 0xff00ff00},
        {{-1.0f, -1.0f, 0.33f}, 0xff00ff00},
        {{ 1.0f, -1.0f, 0.33f}, 0xff00ff00},
    },
    quad2[] =
    {
        {{-1.0f,  1.0f, 0.50f}, 0xffff00ff},
        {{ 1.0f,  1.0f, 0.50f}, 0xffff00ff},
        {{-1.0f, -1.0f, 0.50f}, 0xffff00ff},
        {{ 1.0f, -1.0f, 0.50f}, 0xffff00ff},
    },
    quad3[] =
    {
        {{-1.0f,  1.0f, 0.66f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.66f}, 0xffff0000},
        {{-1.0f, -1.0f, 0.66f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.66f}, 0xffff0000},
    };
    static const DWORD expected_colors[4][4] =
    {
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x00ff0000},
        {0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    vp.X = 0;
    vp.Y = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;

    hr = IDirect3DDevice8_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &depth_stencil);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderTarget(device, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 320, 240, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt1);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 480, 360, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt2);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt3);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt3, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt1, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt2, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(*quad3));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = getPixelColor(device, x, y);
            ok(color_match(color, expected_colors[i][j], 0),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected_colors[i][j], x, y, color);
        }
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface8_Release(depth_stencil);
    IDirect3DSurface8_Release(backbuffer);
    IDirect3DSurface8_Release(rt3);
    IDirect3DSurface8_Release(rt2);
    IDirect3DSurface8_Release(rt1);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

/* Test that partial depth copies work the way they're supposed to. The clear
 * on rt2 only needs a partial copy of the onscreen depth/stencil buffer, and
 * the following draw should only copy back the part that was modified. */
static void depth_buffer2_test(void)
{
    IDirect3DSurface8 *backbuffer, *rt1, *rt2;
    IDirect3DSurface8 *depth_stencil;
    IDirect3DDevice8 *device;
    unsigned int i, j;
    D3DVIEWPORT8 vp;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f,  1.0f, 0.66f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.66f}, 0xffff0000},
        {{-1.0f, -1.0f, 0.66f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.66f}, 0xffff0000},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    vp.X = 0;
    vp.Y = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;

    hr = IDirect3DDevice8_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt1);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 480, 360, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt2);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &depth_stencil);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderTarget(device, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt1, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 0.5f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt2, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depth_stencil);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = getPixelColor(device, x, y);
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 0),
                    "Expected color 0x0000ff00 %u,%u, got 0x%08x.\n", x, y, color);
        }
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface8_Release(depth_stencil);
    IDirect3DSurface8_Release(backbuffer);
    IDirect3DSurface8_Release(rt2);
    IDirect3DSurface8_Release(rt1);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void intz_test(void)
{
    IDirect3DSurface8 *original_rt, *rt;
    IDirect3DTexture8 *texture;
    IDirect3DDevice8 *device;
    IDirect3DSurface8 *ds;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    DWORD ps;
    UINT i;

    static const DWORD ps_code[] =
    {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1.0, 0.0, 0.0, 0.0   */
        0x00000051, 0xa00f0001, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c1, 0.0, 1.0, 0.0, 0.0   */
        0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, /* def c2, 0.0, 0.0, 1.0, 0.0   */
        0x00000042, 0xb00f0000,                                                 /* tex t0                       */
        0x00000042, 0xb00f0001,                                                 /* tex t1                       */
        0x00000008, 0xb0070001, 0xa0e40000, 0xb0e40001,                         /* dp3 t1.xyz, c0, t1           */
        0x00000005, 0x80070000, 0xa0e40001, 0xb0e40001,                         /* mul r0.xyz, c1, t1           */
        0x00000004, 0x80070000, 0xa0e40000, 0xb0e40000, 0x80e40000,             /* mad r0.xyz, c0, t0, r0       */
        0x40000001, 0x80080000, 0xa0aa0002,                                     /* +mov r0.w, c2.z              */
        0x0000ffff,                                                             /* end                          */
    };
    static const struct
    {
        float x, y, z;
        float s0, t0, p0;
        float s1, t1, p1, q1;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    },
    half_quad_1[] =
    {
        { -1.0f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    },
    half_quad_2[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    };
    static const struct
    {
        UINT x, y;
        D3DCOLOR color;
    }
    expected_colors[] =
    {
        { 80, 100, D3DCOLOR_ARGB(0x00, 0x20, 0x40, 0x00)},
        {240, 100, D3DCOLOR_ARGB(0x00, 0x60, 0xbf, 0x00)},
        {400, 100, D3DCOLOR_ARGB(0x00, 0x9f, 0x40, 0x00)},
        {560, 100, D3DCOLOR_ARGB(0x00, 0xdf, 0xbf, 0x00)},
        { 80, 450, D3DCOLOR_ARGB(0x00, 0x20, 0x40, 0x00)},
        {240, 450, D3DCOLOR_ARGB(0x00, 0x60, 0xbf, 0x00)},
        {400, 450, D3DCOLOR_ARGB(0x00, 0x9f, 0x40, 0x00)},
        {560, 450, D3DCOLOR_ARGB(0x00, 0xdf, 0xbf, 0x00)},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("No pixel shader 1.1 support, skipping INTZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No unconditional NP2 texture support, skipping INTZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    if (FAILED(hr = IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, MAKEFOURCC('I','N','T','Z'))))
    {
        skip("No INTZ support, skipping INTZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &original_rt);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX2
            | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE4(1));
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_COUNT4 | D3DTTFF_PROJECTED);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);

    /* Render offscreen, using the INTZ texture as depth buffer */
    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Setup the depth/stencil surface. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, NULL);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(ds);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    /* Render onscreen while using the INTZ texture as depth buffer */
    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, ds);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, NULL);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(ds);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    /* Render offscreen, then onscreen, and finally check the INTZ texture in both areas */
    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, half_quad_1, sizeof(*half_quad_1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, ds);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, half_quad_2, sizeof(*half_quad_2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, NULL);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(ds);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    IDirect3DTexture8_Release(texture);
    hr = IDirect3DDevice8_DeletePixelShader(device, ps);
    ok(SUCCEEDED(hr), "DeletePixelShader failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(original_rt);
    IDirect3DSurface8_Release(rt);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void shadow_test(void)
{
    IDirect3DSurface8 *original_rt, *rt;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    DWORD ps;
    UINT i;

    static const DWORD ps_code[] =
    {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1.0, 0.0, 0.0, 0.0   */
        0x00000051, 0xa00f0001, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c1, 0.0, 1.0, 0.0, 0.0   */
        0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, /* def c2, 0.0, 0.0, 1.0, 0.0   */
        0x00000042, 0xb00f0000,                                                 /* tex t0                       */
        0x00000042, 0xb00f0001,                                                 /* tex t1                       */
        0x00000008, 0xb0070001, 0xa0e40000, 0xb0e40001,                         /* dp3 t1.xyz, c0, t1           */
        0x00000005, 0x80070000, 0xa0e40001, 0xb0e40001,                         /* mul r0.xyz, c1, t1           */
        0x00000004, 0x80070000, 0xa0e40000, 0xb0e40000, 0x80e40000,             /* mad r0.xyz, c0, t0, r0       */
        0x40000001, 0x80080000, 0xa0aa0002,                                     /* +mov r0.w, c2.z              */
        0x0000ffff,                                                             /* end                          */
    };
    static const struct
    {
        D3DFORMAT format;
        const char *name;
    }
    formats[] =
    {
        {D3DFMT_D16_LOCKABLE,   "D3DFMT_D16_LOCKABLE"},
        {D3DFMT_D32,            "D3DFMT_D32"},
        {D3DFMT_D15S1,          "D3DFMT_D15S1"},
        {D3DFMT_D24S8,          "D3DFMT_D24S8"},
        {D3DFMT_D24X8,          "D3DFMT_D24X8"},
        {D3DFMT_D24X4S4,        "D3DFMT_D24X4S4"},
        {D3DFMT_D16,            "D3DFMT_D16"},
    };
    static const struct
    {
        float x, y, z;
        float s0, t0, p0;
        float s1, t1, p1, q1;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f},
    };
    static const struct
    {
        UINT x, y;
        D3DCOLOR color;
    }
    expected_colors[] =
    {
        {400,  60, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00)},
        {560, 180, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00)},
        {560, 300, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00)},
        {400, 420, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x00)},
        {240, 420, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x00)},
        { 80, 300, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00)},
        { 80, 180, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00)},
        {240,  60, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00)},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("No pixel shader 1.1 support, skipping shadow test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &original_rt);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateRenderTarget(device, 1024, 1024, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX2
            | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE4(1));
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_COUNT4 | D3DTTFF_PROJECTED);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); ++i)
    {
        D3DFORMAT format = formats[i].format;
        IDirect3DTexture8 *texture;
        IDirect3DSurface8 *ds;
        unsigned int j;

        if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, format)))
            continue;

        hr = IDirect3DDevice8_CreateTexture(device, 1024, 1024, 1,
                D3DUSAGE_DEPTHSTENCIL, format, D3DPOOL_DEFAULT, &texture);
        ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);

        hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &ds);
        ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
        ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetPixelShader(device, 0);
        ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

        /* Setup the depth/stencil surface. */
        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, NULL);
        ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
        IDirect3DSurface8_Release(ds);

        hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetPixelShader(device, ps);
        ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

        /* Do the actual shadow mapping. */
        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetTexture(device, 0, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetTexture(device, 1, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
        IDirect3DTexture8_Release(texture);

        for (j = 0; j < sizeof(expected_colors) / sizeof(*expected_colors); ++j)
        {
            D3DCOLOR color = getPixelColor(device, expected_colors[j].x, expected_colors[j].y);
            ok(color_match(color, expected_colors[j].color, 0),
                    "Expected color 0x%08x at (%u, %u) for format %s, got 0x%08x.\n",
                    expected_colors[j].color, expected_colors[j].x, expected_colors[j].y,
                    formats[i].name, color);
        }

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);
    }

    hr = IDirect3DDevice8_DeletePixelShader(device, ps);
    ok(SUCCEEDED(hr), "DeletePixelShader failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(original_rt);
    IDirect3DSurface8_Release(rt);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void multisample_copy_rects_test(void)
{
    IDirect3DSurface8 *ds, *ds_plain, *rt, *readback;
    RECT src_rect = {64, 64, 128, 128};
    POINT dst_point = {96, 96};
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    if (FAILED(IDirect3D8_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping multisampled CopyRects test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, FALSE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 256, 256, D3DFMT_D24S8,
            D3DMULTISAMPLE_2_SAMPLES, &ds);
    ok(SUCCEEDED(hr), "Failed to create depth stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 256, 256, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, &ds_plain);
    ok(SUCCEEDED(hr), "Failed to create depth stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateImageSurface(device, 256, 256, D3DFMT_A8R8G8B8, &readback);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CopyRects(device, rt, NULL, 0, readback, NULL);
    ok(SUCCEEDED(hr), "Failed to read render target back, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CopyRects(device, ds, NULL, 0, ds_plain, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Depth buffer copy, hr %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CopyRects(device, rt, &src_rect, 1, readback, &dst_point);
    ok(SUCCEEDED(hr), "Failed to read render target back, hr %#x.\n", hr);

    hr = IDirect3DSurface8_LockRect(readback, &locked_rect, NULL, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Failed to lock readback surface, hr %#x.\n", hr);

    color = *(DWORD *)((BYTE *)locked_rect.pBits + 31 * locked_rect.Pitch + 31 * 4);
    ok(color == 0xff00ff00, "Got unexpected color 0x%08x.\n", color);

    color = *(DWORD *)((BYTE *)locked_rect.pBits + 127 * locked_rect.Pitch + 127 * 4);
    ok(color == 0xffff0000, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DSurface8_UnlockRect(readback);
    ok(SUCCEEDED(hr), "Failed to unlock readback surface, hr %#x.\n", hr);

    IDirect3DSurface8_Release(readback);
    IDirect3DSurface8_Release(ds_plain);
    IDirect3DSurface8_Release(ds);
    IDirect3DSurface8_Release(rt);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void resz_test(void)
{
    IDirect3DSurface8 *rt, *original_rt, *ds, *original_ds, *intz_ds;
    IDirect3DTexture8 *texture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    DWORD ps, value;
    unsigned int i;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1.0, 0.0, 0.0, 0.0   */
        0x00000051, 0xa00f0001, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c1, 0.0, 1.0, 0.0, 0.0   */
        0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, /* def c2, 0.0, 0.0, 1.0, 0.0   */
        0x00000042, 0xb00f0000,                                                 /* tex t0                       */
        0x00000042, 0xb00f0001,                                                 /* tex t1                       */
        0x00000008, 0xb0070001, 0xa0e40000, 0xb0e40001,                         /* dp3 t1.xyz, c0, t1           */
        0x00000005, 0x80070000, 0xa0e40001, 0xb0e40001,                         /* mul r0.xyz, c1, t1           */
        0x00000004, 0x80070000, 0xa0e40000, 0xb0e40000, 0x80e40000,             /* mad r0.xyz, c0, t0, r0       */
        0x40000001, 0x80080000, 0xa0aa0002,                                     /* +mov r0.w, c2.z              */
        0x0000ffff,                                                             /* end                          */
    };
    static const struct
    {
        float x, y, z;
        float s0, t0, p0;
        float s1, t1, p1, q1;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    };
    static const struct
    {
        UINT x, y;
        D3DCOLOR color;
    }
    expected_colors[] =
    {
        { 80, 100, D3DCOLOR_ARGB(0x00, 0x20, 0x40, 0x00)},
        {240, 100, D3DCOLOR_ARGB(0x00, 0x60, 0xbf, 0x00)},
        {400, 100, D3DCOLOR_ARGB(0x00, 0x9f, 0x40, 0x00)},
        {560, 100, D3DCOLOR_ARGB(0x00, 0xdf, 0xbf, 0x00)},
        { 80, 450, D3DCOLOR_ARGB(0x00, 0x20, 0x40, 0x00)},
        {240, 450, D3DCOLOR_ARGB(0x00, 0x60, 0xbf, 0x00)},
        {400, 450, D3DCOLOR_ARGB(0x00, 0x9f, 0x40, 0x00)},
        {560, 450, D3DCOLOR_ARGB(0x00, 0xdf, 0xbf, 0x00)},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    if (FAILED(IDirect3D8_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping RESZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    if (FAILED(IDirect3D8_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_D24S8, TRUE, D3DMULTISAMPLE_2_SAMPLES)))
    {
        skip("Multisampling not supported for D3DFMT_D24S8, skipping RESZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, MAKEFOURCC('I','N','T','Z'))))
    {
        skip("No INTZ support, skipping RESZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, MAKEFOURCC('R','E','S','Z'))))
    {
        skip("No RESZ support, skipping RESZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No unconditional NP2 texture support, skipping INTZ test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &original_ds);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, FALSE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_2_SAMPLES, &ds);

    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &intz_ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, intz_ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    IDirect3DSurface8_Release(intz_ds);
    hr = IDirect3DDevice8_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX2
            | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEXCOORDSIZE4(1));
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_COUNT4 | D3DTTFF_PROJECTED);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    /* Render offscreen (multisampled), blit the depth buffer into the INTZ texture and then check its contents. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    /* The destination depth texture has to be bound to sampler 0 */
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    /* the ATI "spec" says you have to do a dummy draw to ensure correct commands ordering */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0xf);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    /* The actual multisampled depth buffer resolve happens here */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_POINTSIZE, &value);
    ok(SUCCEEDED(hr) && value == 0x7fa05000, "GetRenderState failed, hr %#x, value %#x.\n", hr, value);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, NULL);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Test edge cases - try with no texture at all */
    hr = IDirect3DDevice8_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);

    /* With a non-multisampled depth buffer */
    IDirect3DSurface8_Release(ds);
    IDirect3DSurface8_Release(rt);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, &ds);

    hr = IDirect3DDevice8_SetRenderTarget(device, original_rt, ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, 0);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0xf);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface8_Release(ds);
    IDirect3DTexture8_Release(texture);
    hr = IDirect3DDevice8_DeletePixelShader(device, ps);
    ok(SUCCEEDED(hr), "DeletePixelShader failed, hr %#x.\n", hr);
    IDirect3DSurface8_Release(original_ds);
    IDirect3DSurface8_Release(original_rt);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void zenable_test(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    UINT x, y;
    UINT i, j;
    UINT test;
    IDirect3DSurface8 *ds, *rt;

    static const struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{  0.0f,   0.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{640.0f, 480.0f,  1.5f, 1.0f}, 0xff00ff00},
        {{640.0f,   0.0f,  1.5f, 1.0f}, 0xff00ff00},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get depth stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);

    for (test = 0; test < 2; ++test)
    {
        /* The Windows 8 testbot (WARP) appears to clip with
         * ZENABLE = D3DZB_TRUE and no depth buffer set. */
        static const D3DCOLOR expected_broken[] =
        {
            0x00ff0000, 0x0000ff00, 0x0000ff00, 0x00ff0000,
            0x00ff0000, 0x0000ff00, 0x0000ff00, 0x00ff0000,
            0x00ff0000, 0x0000ff00, 0x0000ff00, 0x00ff0000,
            0x00ff0000, 0x0000ff00, 0x0000ff00, 0x00ff0000,
        };

        if (!test)
        {
            hr = IDirect3DDevice8_SetRenderTarget(device, rt, NULL);
            ok(SUCCEEDED(hr), "Failed to set depth stencil surface, hr %#x.\n", hr);
        }
        else
        {
            hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
            ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);
            hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
            ok(SUCCEEDED(hr), "Failed to set depth stencil surface, hr %#x.\n", hr);
            hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 0.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
        }
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, tquad, sizeof(*tquad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        for (i = 0; i < 4; ++i)
        {
            for (j = 0; j < 4; ++j)
            {
                x = 80 * ((2 * j) + 1);
                y = 60 * ((2 * i) + 1);
                color = getPixelColor(device, x, y);
                ok(color_match(color, 0x0000ff00, 1)
                        || broken(color_match(color, expected_broken[i * 4 + j], 1) && !test),
                        "Expected color 0x0000ff00 at %u, %u, got 0x%08x.\n", x, y, color);
            }
        }

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);
    }

    IDirect3DSurface8_Release(ds);
    IDirect3DSurface8_Release(rt);

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1)
            && caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        static const DWORD vs_code[] =
        {
            0xfffe0101,                                 /* vs_1_1           */
            0x00000001, 0xc00f0000, 0x90e40000,         /* mov oPos, v0     */
            0x00000001, 0xd00f0000, 0x90e40000,         /* mov oD0, v0      */
            0x0000ffff
        };
        static const DWORD ps_code[] =
        {
            0xffff0101,                                 /* ps_1_1           */
            0x00000001, 0x800f0000, 0x90e40000,         /* mov r0, v0       */
            0x0000ffff                                  /* end              */
        };
        static const struct vec3 quad[] =
        {
            {-1.0f, -1.0f, -0.5f},
            {-1.0f,  1.0f, -0.5f},
            { 1.0f, -1.0f,  1.5f},
            { 1.0f,  1.0f,  1.5f},
        };
        static const D3DCOLOR expected[] =
        {
            0x00ff0000, 0x0060df60, 0x009fdf9f, 0x00ff0000,
            0x00ff0000, 0x00609f60, 0x009f9f9f, 0x00ff0000,
            0x00ff0000, 0x00606060, 0x009f609f, 0x00ff0000,
            0x00ff0000, 0x00602060, 0x009f209f, 0x00ff0000,
        };
        /* The Windows 8 testbot (WARP) appears to not clip z for regular
         * vertices either. */
        static const D3DCOLOR expected_broken[] =
        {
            0x0020df20, 0x0060df60, 0x009fdf9f, 0x00dfdfdf,
            0x00209f20, 0x00609f60, 0x009f9f9f, 0x00df9fdf,
            0x00206020, 0x00606060, 0x009f609f, 0x00df60df,
            0x00202020, 0x00602060, 0x009f209f, 0x00df20df,
        };
        static const DWORD decl[] =
        {
            D3DVSD_STREAM(0),
            D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),
            D3DVSD_END()
        };
        DWORD vs, ps;

        hr = IDirect3DDevice8_CreateVertexShader(device, decl, vs_code, &vs, 0);
        ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_CreatePixelShader(device, ps_code, &ps);
        ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShader(device, vs);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetPixelShader(device, ps);
        ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        for (i = 0; i < 4; ++i)
        {
            for (j = 0; j < 4; ++j)
            {
                x = 80 * ((2 * j) + 1);
                y = 60 * ((2 * i) + 1);
                color = getPixelColor(device, x, y);
                ok(color_match(color, expected[i * 4 + j], 1)
                        || broken(color_match(color, expected_broken[i * 4 + j], 1)),
                        "Expected color 0x%08x at %u, %u, got 0x%08x.\n", expected[i * 4 + j], x, y, color);
            }
        }

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);

        hr = IDirect3DDevice8_DeletePixelShader(device, ps);
        ok(SUCCEEDED(hr), "Failed to delete pixel shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DeleteVertexShader(device, vs);
        ok(SUCCEEDED(hr), "Failed to delete vertex shader, hr %#x.\n", hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void fog_special_test(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD ps, vs;
    HWND window;
    HRESULT hr;
    union
    {
        float f;
        DWORD d;
    } conv;

    static const struct
    {
        struct vec3 position;
        D3DCOLOR diffuse;
    }
    quad[] =
    {
        {{ -1.0f,  -1.0f,  0.0f}, 0xff00ff00},
        {{ -1.0f,   1.0f,  0.0f}, 0xff00ff00},
        {{  1.0f,  -1.0f,  1.0f}, 0xff00ff00},
        {{  1.0f,   1.0f,  1.0f}, 0xff00ff00}
    };
    static const struct
    {
        DWORD vertexmode, tablemode;
        BOOL vs, ps;
        D3DCOLOR color_left, color_right;
    }
    tests[] =
    {
        {D3DFOG_LINEAR, D3DFOG_NONE,   FALSE, FALSE, 0x00ff0000, 0x00ff0000},
        {D3DFOG_LINEAR, D3DFOG_NONE,   FALSE, TRUE,  0x00ff0000, 0x00ff0000},
        {D3DFOG_LINEAR, D3DFOG_NONE,   TRUE,  FALSE, 0x00ff0000, 0x00ff0000},
        {D3DFOG_LINEAR, D3DFOG_NONE,   TRUE,  TRUE,  0x00ff0000, 0x00ff0000},

        {D3DFOG_NONE,   D3DFOG_LINEAR, FALSE, FALSE, 0x0000ff00, 0x00ff0000},
        {D3DFOG_NONE,   D3DFOG_LINEAR, FALSE, TRUE,  0x0000ff00, 0x00ff0000},
        {D3DFOG_NONE,   D3DFOG_LINEAR, TRUE,  FALSE, 0x0000ff00, 0x00ff0000},
        {D3DFOG_NONE,   D3DFOG_LINEAR, TRUE,  TRUE,  0x0000ff00, 0x00ff0000},
    };
    static const DWORD pixel_shader_code[] =
    {
        0xffff0101,                                 /* ps.1.1               */
        0x00000001, 0x800f0000, 0x90e40000,         /* mov r0, v0           */
        0x0000ffff
    };
    static const DWORD vertex_decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),              /* position, v0         */
        D3DVSD_REG(1, D3DVSDT_D3DCOLOR),            /* diffuse color, v1    */
        D3DVSD_END()
    };
    static const DWORD vertex_shader_code[] =
    {
        0xfffe0101,                                 /* vs.1.1               */
        0x00000001, 0xc00f0000, 0x90e40000,         /* mov oPos, v0         */
        0x00000001, 0xd00f0000, 0x90e40001,         /* mov oD0, v1          */
        0x0000ffff
    };
    static const D3DMATRIX identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        hr = IDirect3DDevice8_CreateVertexShader(device, vertex_decl, vertex_shader_code, &vs, 0);
        ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    }
    else
    {
        skip("Vertex Shaders not supported, skipping some fog tests.\n");
        vs = 0;
    }
    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        hr = IDirect3DDevice8_CreatePixelShader(device, pixel_shader_code, &ps);
        ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    }
    else
    {
        skip("Pixel Shaders not supported, skipping some fog tests.\n");
        ps = 0;
    }

    /* The table fog tests seem to depend on the projection matrix explicitly
     * being set to an identity matrix, even though that's the default.
     * (AMD Radeon HD 6310, Windows 7) */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable fog, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0xffff0000);
    ok(SUCCEEDED(hr), "Failed to set fog color, hr %#x.\n", hr);

    conv.f = 0.5f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGSTART, conv.d);
    ok(SUCCEEDED(hr), "Failed to set fog start, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGEND, conv.d);
    ok(SUCCEEDED(hr), "Failed to set fog end, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

        if (!tests[i].vs)
        {
            hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
            ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
        }
        else if (vs)
        {
            hr = IDirect3DDevice8_SetVertexShader(device, vs);
            ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        }
        else
        {
            continue;
        }

        if (!tests[i].ps)
        {
            hr = IDirect3DDevice8_SetPixelShader(device, 0);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
        }
        else if (ps)
        {
            hr = IDirect3DDevice8_SetPixelShader(device, ps);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
        }
        else
        {
            continue;
        }

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, tests[i].vertexmode);
        ok(SUCCEEDED(hr), "Failed to set fogvertexmode, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, tests[i].tablemode);
        ok(SUCCEEDED(hr), "Failed to set fogtablemode, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 310, 240);
        ok(color_match(color, tests[i].color_left, 1),
                "Expected left color 0x%08x, got 0x%08x, case %u.\n", tests[i].color_left, color, i);
        color = getPixelColor(device, 330, 240);
        ok(color_match(color, tests[i].color_right, 1),
                "Expected right color 0x%08x, got 0x%08x, case %u.\n", tests[i].color_right, color, i);

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);
    }

    if (vs)
        IDirect3DDevice8_DeleteVertexShader(device, vs);
    if (ps)
        IDirect3DDevice8_DeletePixelShader(device, ps);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void volume_dxt5_test(void)
{
    IDirect3DVolumeTexture8 *texture;
    IDirect3DDevice8 *device;
    D3DLOCKED_BOX box;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const char texture_data[] =
    {
        /* A 8x4x2 texture consisting of 4 4x4 blocks. The colors of the blocks are red, green, blue and white. */
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
    };
    static const struct
    {
        struct vec3 position;
        struct vec3 texcrd;
    }
    quads[] =
    {
        {{ -1.0f,  -1.0f,  0.0f}, { 0.0f, 0.0f, 0.25f}},
        {{ -1.0f,   1.0f,  0.0f}, { 0.0f, 1.0f, 0.25f}},
        {{  0.0f,  -1.0f,  1.0f}, { 1.0f, 0.0f, 0.25f}},
        {{  0.0f,   1.0f,  1.0f}, { 1.0f, 1.0f, 0.25f}},

        {{  0.0f,  -1.0f,  0.0f}, { 0.0f, 0.0f, 0.75f}},
        {{  0.0f,   1.0f,  0.0f}, { 0.0f, 1.0f, 0.75f}},
        {{  1.0f,  -1.0f,  1.0f}, { 1.0f, 0.0f, 0.75f}},
        {{  1.0f,   1.0f,  1.0f}, { 1.0f, 1.0f, 0.75f}},
    };
    static const DWORD expected_colors[] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00ffffff};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_VOLUMETEXTURE, D3DFMT_DXT5)))
    {
        skip("Volume DXT5 textures are not supported, skipping test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_CreateVolumeTexture(device, 8, 4, 2, 1, 0, D3DFMT_DXT5,
            D3DPOOL_MANAGED, &texture);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    memcpy(box.pBits, texture_data, sizeof(texture_data));
    hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "Failed to set mag filter, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff00ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[4], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    for (i = 0; i < 4; i++)
    {
        color = getPixelColor(device, 80 + 160 * i, 240);
        ok (color_match(color, expected_colors[i], 1),
                "Expected color 0x%08x, got 0x%08x, case %u.\n", expected_colors[i], color, i);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    IDirect3DVolumeTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void volume_v16u16_test(void)
{
    IDirect3DVolumeTexture8 *texture;
    IDirect3DDevice8 *device;
    D3DLOCKED_BOX box;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD shader;
    SHORT *texel;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        struct vec3 texcrd;
    }
    quads[] =
    {
        {{ -1.0f,  -1.0f,  0.0f}, { 0.0f, 0.0f, 0.25f}},
        {{ -1.0f,   1.0f,  0.0f}, { 0.0f, 1.0f, 0.25f}},
        {{  0.0f,  -1.0f,  1.0f}, { 1.0f, 0.0f, 0.25f}},
        {{  0.0f,   1.0f,  1.0f}, { 1.0f, 1.0f, 0.25f}},

        {{  0.0f,  -1.0f,  0.0f}, { 0.0f, 0.0f, 0.75f}},
        {{  0.0f,   1.0f,  0.0f}, { 0.0f, 1.0f, 0.75f}},
        {{  1.0f,  -1.0f,  1.0f}, { 1.0f, 0.0f, 0.75f}},
        {{  1.0f,   1.0f,  1.0f}, { 1.0f, 1.0f, 0.75f}},
    };
    static const DWORD shader_code[] =
    {
        0xffff0101,                                                     /* ps_1_1               */
        0x00000051, 0xa00f0000, 0x3f000000, 0x3f000000,                 /* def c0, 0.5, 0.5,    */
        0x3f000000, 0x3f000000,                                         /*         0.5, 0.5     */
        0x00000042, 0xb00f0000,                                         /* tex t0               */
        0x00000004, 0x800f0000, 0xb0e40000, 0xa0e40000, 0xa0e40000,     /* mad r0, t0, c0, c0   */
        0x0000ffff                                                      /* end                  */
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_VOLUMETEXTURE, D3DFMT_V16U16)))
    {
        skip("Volume V16U16 textures are not supported, skipping test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("No pixel shader 1.1 support, skipping test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetPixelShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "Failed to set filter, hr %#x.\n", hr);

    for (i = 0; i < 2; i++)
    {
        D3DPOOL pool;

        if (i)
            pool = D3DPOOL_SYSTEMMEM;
        else
            pool = D3DPOOL_MANAGED;

        hr = IDirect3DDevice8_CreateVolumeTexture(device, 1, 2, 2, 1, 0, D3DFMT_V16U16,
                pool, &texture);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &box, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);

        texel = (SHORT *)((BYTE *)box.pBits + 0 * box.RowPitch + 0 * box.SlicePitch);
        texel[0] = 32767;
        texel[1] = 32767;
        texel = (SHORT *)((BYTE *)box.pBits + 1 * box.RowPitch + 0 * box.SlicePitch);
        texel[0] = -32768;
        texel[1] = 0;
        texel = (SHORT *)((BYTE *)box.pBits + 0 * box.RowPitch + 1 * box.SlicePitch);
        texel[0] = -16384;
        texel[1] =  16384;
        texel = (SHORT *)((BYTE *)box.pBits + 1 * box.RowPitch + 1 * box.SlicePitch);
        texel[0] =  0;
        texel[1] =  0;

        hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        if (i)
        {
            IDirect3DVolumeTexture8 *texture2;

            hr = IDirect3DDevice8_CreateVolumeTexture(device, 1, 2, 2, 1, 0, D3DFMT_V16U16,
                    D3DPOOL_DEFAULT, &texture2);
            ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

            hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)texture,
                    (IDirect3DBaseTexture8 *)texture2);
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

            IDirect3DVolumeTexture8_Release(texture);
            texture = texture2;
        }

        hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *) texture);
        ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff00ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[4], sizeof(*quads));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 120, 160);
        ok (color_match(color, 0x000080ff, 2),
                "Expected color 0x000080ff, got 0x%08x, V16U16 input -32768, 0.\n", color);
        color = getPixelColor(device, 120, 400);
        ok (color_match(color, 0x00ffffff, 2),
                "Expected color 0x00ffffff, got 0x%08x, V16U16 input 32767, 32767.\n", color);
        color = getPixelColor(device, 360, 160);
        ok (color_match(color, 0x007f7fff, 2),
                "Expected color 0x007f7fff, got 0x%08x, V16U16 input 0, 0.\n", color);
        color = getPixelColor(device, 360, 400);
        ok (color_match(color, 0x0040c0ff, 2),
                "Expected color 0x0040c0ff, got 0x%08x, V16U16 input -16384, 16384.\n", color);

        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

        IDirect3DVolumeTexture8_Release(texture);
    }

    hr = IDirect3DDevice8_DeletePixelShader(device, shader);
    ok(SUCCEEDED(hr), "Failed delete pixel shader, hr %#x.\n", hr);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void fill_surface(IDirect3DSurface8 *surface, DWORD color, DWORD flags)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT l;
    HRESULT hr;
    unsigned int x, y;
    DWORD *mem;

    hr = IDirect3DSurface8_GetDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    hr = IDirect3DSurface8_LockRect(surface, &l, NULL, flags);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return;

    for (y = 0; y < desc.Height; y++)
    {
        mem = (DWORD *)((BYTE *)l.pBits + y * l.Pitch);
        for (x = 0; x < l.Pitch / sizeof(DWORD); x++)
        {
            mem[x] = color;
        }
    }
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
}

static void add_dirty_rect_test_draw(IDirect3DDevice8 *device)
{
    HRESULT hr;
    static const struct
    {
        struct vec3 position;
        struct vec2 texcoord;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
    };

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
}

static void add_dirty_rect_test(void)
{
    IDirect3DSurface8 *surface_dst2, *surface_src_green, *surface_src_red, *surface_managed;
    IDirect3DTexture8 *tex_dst1, *tex_dst2, *tex_src_red, *tex_src_green, *tex_managed;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    DWORD *texel;
    HWND window;
    HRESULT hr;

    static const RECT part_rect = {96, 96, 160, 160};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &tex_dst1);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &tex_dst2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &tex_src_red);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &tex_src_green);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_MANAGED, &tex_managed);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DTexture8_GetSurfaceLevel(tex_dst2, 0, &surface_dst2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(tex_src_green, 0, &surface_src_green);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(tex_src_red, 0, &surface_src_red);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(tex_managed, 0, &surface_managed);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    fill_surface(surface_src_red, 0x00ff0000, 0);
    fill_surface(surface_src_green, 0x0000ff00, 0);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);

    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    /* The second UpdateTexture call writing to tex_dst2 is ignored because tex_src_green is not dirty. */
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_red,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on the destination is ignored. */
    hr = IDirect3DTexture8_AddDirtyRect(tex_dst2, &part_rect);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture8_AddDirtyRect(tex_dst2, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on the source makes UpdateTexture work. Partial rectangle
     * tracking is supported. */
    hr = IDirect3DTexture8_AddDirtyRect(tex_src_green, &part_rect);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    color = getPixelColor(device, 1, 1);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture8_AddDirtyRect(tex_src_green, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 1, 1);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);

    /* Locks with NO_DIRTY_UPDATE are ignored. */
    fill_surface(surface_src_green, 0x00000080, D3DLOCK_NO_DIRTY_UPDATE);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Readonly maps write to D3DPOOL_SYSTEMMEM, but don't record a dirty rectangle. */
    fill_surface(surface_src_green, 0x000000ff, D3DLOCK_READONLY);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture8_AddDirtyRect(tex_src_green, NULL);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x000000ff, 1),
            "Expected color 0x000000ff, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Maps without either of these flags record a dirty rectangle. */
    fill_surface(surface_src_green, 0x00ffffff, 0);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ffffff, 1),
            "Expected color 0x00ffffff, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Partial LockRect works just like a partial AddDirtyRect call. */
    hr = IDirect3DTexture8_LockRect(tex_src_green, 0, &locked_rect, &part_rect, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    texel = locked_rect.pBits;
    for (i = 0; i < 64; i++)
        texel[i] = 0x00ff00ff;
    for (i = 1; i < 64; i++)
        memcpy((BYTE *)locked_rect.pBits + i * locked_rect.Pitch, locked_rect.pBits, locked_rect.Pitch);
    hr = IDirect3DTexture8_UnlockRect(tex_src_green, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff00ff, 1),
            "Expected color 0x00ff00ff, got 0x%08x.\n", color);
    color = getPixelColor(device, 1, 1);
    ok(color_match(color, 0x00ffffff, 1),
            "Expected color 0x00ffffff, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    fill_surface(surface_src_red, 0x00ff0000, 0);
    fill_surface(surface_src_green, 0x0000ff00, 0);

    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_green,
            (IDirect3DBaseTexture8 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* UpdateSurface ignores the missing dirty marker. */
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)tex_src_red,
            (IDirect3DBaseTexture8 *)tex_dst2);
    hr = IDirect3DDevice8_CopyRects(device, surface_src_green, NULL, 0, surface_dst2, NULL);
    ok(SUCCEEDED(hr), "Failed to update surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    fill_surface(surface_managed, 0x00ff0000, 0);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex_managed);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Managed textures also honor D3DLOCK_NO_DIRTY_UPDATE. */
    fill_surface(surface_managed, 0x0000ff00, D3DLOCK_NO_DIRTY_UPDATE);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect uploads the new contents.
     * Side note, not tested in the test: Partial surface updates work, and two separate
     * dirty rectangles are tracked individually. Tested on Nvidia Kepler, other drivers
     * untested. */
    hr = IDirect3DTexture8_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* So does ResourceManagerDiscardBytes. */
    fill_surface(surface_managed, 0x000000ff, D3DLOCK_NO_DIRTY_UPDATE);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_ResourceManagerDiscardBytes(device, 0);
    ok(SUCCEEDED(hr), "Failed to evict managed resources, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x000000ff, 1),
            "Expected color 0x000000ff, got 0x%08x.\n", color);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on a locked texture is allowed. */
    hr = IDirect3DTexture8_LockRect(tex_src_red, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    hr = IDirect3DTexture8_AddDirtyRect(tex_src_red, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DTexture8_UnlockRect(tex_src_red, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    /* Redundant AddDirtyRect calls are ok. */
    hr = IDirect3DTexture8_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DTexture8_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);

    IDirect3DSurface8_Release(surface_dst2);
    IDirect3DSurface8_Release(surface_managed);
    IDirect3DSurface8_Release(surface_src_red);
    IDirect3DSurface8_Release(surface_src_green);
    IDirect3DTexture8_Release(tex_src_red);
    IDirect3DTexture8_Release(tex_src_green);
    IDirect3DTexture8_Release(tex_dst1);
    IDirect3DTexture8_Release(tex_dst2);
    IDirect3DTexture8_Release(tex_managed);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_3dc_formats(void)
{
    static const char ati1n_data[] =
    {
        /* A 4x4 texture with the color component at 50%. */
        0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static const char ati2n_data[] =
    {
        /* A 8x4 texture consisting of 2 4x4 blocks. The first block has 50% first color component,
         * 0% second component. Second block is the opposite. */
        0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    static const struct
    {
        struct vec3 position;
        struct vec2 texcoord;
    }
    quads[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 0.0f, -1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  1.0f, 1.0f}, {1.0f, 1.0f}},

        {{ 0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.0f,  1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 1.0f}, {1.0f, 1.0f}},
    };
    static const DWORD ati1n_fourcc = MAKEFOURCC('A','T','I','1');
    static const DWORD ati2n_fourcc = MAKEFOURCC('A','T','I','2');
    static const struct
    {
        struct vec2 position;
        D3DCOLOR amd_r500;
        D3DCOLOR amd_r600;
        D3DCOLOR nvidia_old;
        D3DCOLOR nvidia_new;
    }
    expected_colors[] =
    {
        {{ 80, 240}, 0x007fffff, 0x003f3f3f, 0x007f7f7f, 0x007f0000},
        {{240, 240}, 0x007fffff, 0x003f3f3f, 0x007f7f7f, 0x007f0000},
        {{400, 240}, 0x00007fff, 0x00007fff, 0x00007fff, 0x00007fff},
        {{560, 240}, 0x007f00ff, 0x007f00ff, 0x007f00ff, 0x007f00ff},
    };
    IDirect3D8 *d3d;
    IDirect3DDevice8 *device;
    IDirect3DTexture8 *ati1n_texture, *ati2n_texture;
    D3DCAPS8 caps;
    D3DLOCKED_RECT rect;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, ati1n_fourcc)))
    {
        skip("ATI1N textures are not supported, skipping test.\n");
        goto done;
    }
    if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, ati2n_fourcc)))
    {
        skip("ATI2N textures are not supported, skipping test.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP))
    {
        skip("D3DTA_TEMP not supported, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 4, 4, 1, 0, ati1n_fourcc,
            D3DPOOL_MANAGED, &ati1n_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DTexture8_LockRect(ati1n_texture, 0, &rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    memcpy(rect.pBits, ati1n_data, sizeof(ati1n_data));
    hr = IDirect3DTexture8_UnlockRect(ati1n_texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 8, 4, 1, 0, ati2n_fourcc,
            D3DPOOL_MANAGED, &ati2n_texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DTexture8_LockRect(ati2n_texture, 0, &rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    memcpy(rect.pBits, ati2n_data, sizeof(ati2n_data));
    hr = IDirect3DTexture8_UnlockRect(ati2n_texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_BLENDTEXTUREALPHA);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TEMP);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set alpha op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set alpha arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "Failed to set mag filter, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff00ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)ati1n_texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)ati2n_texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[4], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        color = getPixelColor(device, expected_colors[i].position.x, expected_colors[i].position.y);
        ok (color_match(color, expected_colors[i].amd_r500, 1)
                || color_match(color, expected_colors[i].amd_r600, 1)
                || color_match(color, expected_colors[i].nvidia_old, 1)
                || color_match(color, expected_colors[i].nvidia_new, 1),
                "Got unexpected color 0x%08x, case %u.\n", color, i);
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    IDirect3DTexture8_Release(ati2n_texture);
    IDirect3DTexture8_Release(ati1n_texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_fog_interpolation(void)
{
    HRESULT hr;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    D3DCOLOR color;
    static const struct
    {
        struct vec3 position;
        D3DCOLOR diffuse;
        D3DCOLOR specular;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000, 0xff000000},
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000, 0xff000000},
        {{ 1.0f, -1.0f, 1.0f}, 0xffff0000, 0x00000000},
        {{ 1.0f,  1.0f, 1.0f}, 0xffff0000, 0x00000000},
    };
    union
    {
        DWORD d;
        float f;
    } conv;
    unsigned int i;
    static const struct
    {
        D3DFOGMODE vfog, tfog;
        D3DSHADEMODE shade;
        D3DCOLOR middle_color;
        BOOL todo;
    }
    tests[] =
    {
        {D3DFOG_NONE, D3DFOG_NONE, D3DSHADE_FLAT,    0x00007f80, FALSE},
        {D3DFOG_NONE, D3DFOG_NONE, D3DSHADE_GOURAUD, 0x00007f80, FALSE},
        {D3DFOG_EXP,  D3DFOG_NONE, D3DSHADE_FLAT,    0x00007f80, TRUE},
        {D3DFOG_EXP,  D3DFOG_NONE, D3DSHADE_GOURAUD, 0x00007f80, TRUE},
        {D3DFOG_NONE, D3DFOG_EXP,  D3DSHADE_FLAT,    0x0000ea15, FALSE},
        {D3DFOG_NONE, D3DFOG_EXP,  D3DSHADE_GOURAUD, 0x0000ea15, FALSE},
        {D3DFOG_EXP,  D3DFOG_EXP,  D3DSHADE_FLAT,    0x0000ea15, FALSE},
        {D3DFOG_EXP,  D3DFOG_EXP,  D3DSHADE_GOURAUD, 0x0000ea15, FALSE},
    };
    static const D3DMATRIX ident_mat =
    {{{
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f,  0.0f, 1.0f
    }}};
    D3DCAPS8 caps;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE))
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests\n");

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0x0000ff00);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    conv.f = 5.0;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGDENSITY, conv.d);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set texture stage state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "Failed to set texture stage state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0x000000ff);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    /* Some of the tests seem to depend on the projection matrix explicitly
     * being set to an identity matrix, even though that's the default.
     * (AMD Radeon X1600, AMD Radeon HD 6310, Windows 7). Without this,
     * the drivers seem to use a static z = 1.0 input for the fog equation.
     * The input value is independent of the actual z and w component of
     * the vertex position. */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &ident_mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        if(!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) && tests[i].tfog)
            continue;

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00808080, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SHADEMODE, tests[i].shade);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, tests[i].vfog);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, tests[i].tfog);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 0, 240);
        ok(color_match(color, 0x000000ff, 2), "Got unexpected color 0x%08x, case %u.\n", color, i);
        color = getPixelColor(device, 320, 240);
        if (tests[i].todo)
            todo_wine ok(color_match(color, tests[i].middle_color, 2),
                    "Got unexpected color 0x%08x, case %u.\n", color, i);
        else
            ok(color_match(color, tests[i].middle_color, 2),
                    "Got unexpected color 0x%08x, case %u.\n", color, i);
        color = getPixelColor(device, 639, 240);
        ok(color_match(color, 0x0000fd02, 2), "Got unexpected color 0x%08x, case %u.\n", color, i);
        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_negative_fixedfunction_fog(void)
{
    HRESULT hr;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    D3DCOLOR color;
    static const struct
    {
        struct vec3 position;
        D3DCOLOR diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, -0.5f}, 0xffff0000},
        {{-1.0f,  1.0f, -0.5f}, 0xffff0000},
        {{ 1.0f, -1.0f, -0.5f}, 0xffff0000},
        {{ 1.0f,  1.0f, -0.5f}, 0xffff0000},
    };
    static const struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
    }
    tquad[] =
    {
        {{  0.0f,   0.0f, -0.5f, 1.0f}, 0xffff0000},
        {{640.0f,   0.0f, -0.5f, 1.0f}, 0xffff0000},
        {{  0.0f, 480.0f, -0.5f, 1.0f}, 0xffff0000},
        {{640.0f, 480.0f, -0.5f, 1.0f}, 0xffff0000},
    };
    unsigned int i;
    static const D3DMATRIX zero =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }}};
    /* Needed to make AMD drivers happy. Yeah, it is not supposed to
     * have an effect on RHW draws. */
    static const D3DMATRIX identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }}};
    static const struct
    {
        DWORD pos_type;
        const void *quad;
        size_t stride;
        const D3DMATRIX *matrix;
        union
        {
            float f;
            DWORD d;
        } start, end;
        D3DFOGMODE vfog, tfog;
        DWORD color, color_broken, color_broken2;
    }
    tests[] =
    {
        /* Run the XYZRHW tests first. Depth clamping is broken after RHW draws on the testbot.
         *
         * Geforce8+ GPUs on Windows abs() table fog, everything else does not. */
        {D3DFVF_XYZRHW, tquad,  sizeof(*tquad), &identity, { 0.0f}, {1.0f},
                D3DFOG_NONE,   D3DFOG_LINEAR, 0x00ff0000, 0x00808000, 0x00808000},
        /* r200 GPUs and presumably all d3d8 and older HW clamp the fog
         * parameters to 0.0 and 1.0 in the table fog case. */
        {D3DFVF_XYZRHW, tquad,  sizeof(*tquad), &identity, {-1.0f}, {0.0f},
                D3DFOG_NONE,   D3DFOG_LINEAR, 0x00808000, 0x00ff0000, 0x0000ff00},
        /* test_fog_interpolation shows that vertex fog evaluates the fog
         * equation in the vertex pipeline. Start = -1.0 && end = 0.0 shows
         * that the abs happens before the fog equation is evaluated.
         *
         * Vertex fog abs() behavior is the same on all GPUs. */
        {D3DFVF_XYZ,    quad,   sizeof(*quad),  &zero,     { 0.0f}, {1.0f},
                D3DFOG_LINEAR, D3DFOG_NONE,   0x00808000, 0x00808000, 0x00808000},
        {D3DFVF_XYZ,    quad,   sizeof(*quad),  &zero,     {-1.0f}, {0.0f},
                D3DFOG_LINEAR, D3DFOG_NONE,   0x0000ff00, 0x0000ff00, 0x0000ff00},
        {D3DFVF_XYZ,    quad,   sizeof(*quad),  &zero,     { 0.0f}, {1.0f},
                D3DFOG_EXP,    D3DFOG_NONE,   0x009b6400, 0x009b6400, 0x009b6400},
    };
    D3DCAPS8 caps;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE))
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests.\n");

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0x0000ff00);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        if (!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE) && tests[i].tfog)
            continue;

        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

        hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, tests[i].matrix);
        ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShader(device, tests[i].pos_type | D3DFVF_DIFFUSE);
        ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGSTART, tests[i].start.d);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGEND, tests[i].end.d);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, tests[i].vfog);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, tests[i].tfog);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, tests[i].quad, tests[i].stride);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, tests[i].color, 2) || broken(color_match(color, tests[i].color_broken, 2))
                || broken(color_match(color, tests[i].color_broken2, 2)),
                "Got unexpected color 0x%08x, case %u.\n", color, i);
        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_table_fog_zw(void)
{
    HRESULT hr;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    D3DCOLOR color;
    D3DCAPS8 caps;
    static struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
    }
    quad[] =
    {
        {{  0.0f,   0.0f, 0.0f, 0.0f}, 0xffff0000},
        {{640.0f,   0.0f, 0.0f, 0.0f}, 0xffff0000},
        {{  0.0f, 480.0f, 0.0f, 0.0f}, 0xffff0000},
        {{640.0f, 480.0f, 0.0f, 0.0f}, 0xffff0000},
    };
    static const D3DMATRIX identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }}};
    static const struct
    {
        float z, w;
        D3DZBUFFERTYPE z_test;
        D3DCOLOR color;
    }
    tests[] =
    {
        {0.7f,  0.0f, D3DZB_TRUE,  0x004cb200},
        {0.7f,  0.0f, D3DZB_FALSE, 0x004cb200},
        {0.7f,  0.3f, D3DZB_TRUE,  0x004cb200},
        {0.7f,  0.3f, D3DZB_FALSE, 0x004cb200},
        {0.7f,  3.0f, D3DZB_TRUE,  0x004cb200},
        {0.7f,  3.0f, D3DZB_FALSE, 0x004cb200},
        {0.3f,  0.0f, D3DZB_TRUE,  0x00b24c00},
        {0.3f,  0.0f, D3DZB_FALSE, 0x00b24c00},
    };
    unsigned int i;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE))
    {
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping POSITIONT table fog test.\n");
        goto done;
    }

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0x0000ff00);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    /* Work around an AMD Windows driver bug. Needs a proj matrix applied redundantly. */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x000000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

        quad[0].position.z = tests[i].z;
        quad[1].position.z = tests[i].z;
        quad[2].position.z = tests[i].z;
        quad[3].position.z = tests[i].z;
        quad[0].position.w = tests[i].w;
        quad[1].position.w = tests[i].w;
        quad[2].position.w = tests[i].w;
        quad[3].position.w = tests[i].w;
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, tests[i].z_test);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

        hr = IDirect3DDevice8_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice8_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, tests[i].color, 2),
                "Got unexpected color 0x%08x, expected 0x%08x, case %u.\n", color, tests[i].color, i);
        hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    }

done:
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_signed_formats(void)
{
    IDirect3DDevice8 *device;
    HWND window;
    HRESULT hr;
    unsigned int i, j, x, y;
    IDirect3DTexture8 *texture, *texture_sysmem;
    D3DLOCKED_RECT locked_rect;
    DWORD shader, shader_alpha;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    D3DCAPS8 caps;
    ULONG refcount;

    /* See comments in the d3d9 version of this test for an
     * explanation of these values. */
    static const USHORT content_v8u8[4][4] =
    {
        {0x0000, 0x7f7f, 0x8880, 0x0000},
        {0x0080, 0x8000, 0x7f00, 0x007f},
        {0x193b, 0xe8c8, 0x0808, 0xf8f8},
        {0x4444, 0xc0c0, 0xa066, 0x22e0},
    };
    static const DWORD content_v16u16[4][4] =
    {
        {0x00000000, 0x7fff7fff, 0x88008000, 0x00000000},
        {0x00008000, 0x80000000, 0x7fff0000, 0x00007fff},
        {0x19993bbb, 0xe800c800, 0x08880888, 0xf800f800},
        {0x44444444, 0xc000c000, 0xa0006666, 0x2222e000},
    };
    static const DWORD content_q8w8v8u8[4][4] =
    {
        {0x00000000, 0xff7f7f7f, 0x7f008880, 0x817f0000},
        {0x10000080, 0x20008000, 0x30007f00, 0x4000007f},
        {0x5020193b, 0x6028e8c8, 0x70020808, 0x807ff8f8},
        {0x90414444, 0xa000c0c0, 0x8261a066, 0x834922e0},
    };
    static const DWORD content_x8l8v8u8[4][4] =
    {
        {0x00000000, 0x00ff7f7f, 0x00008880, 0x00ff0000},
        {0x00000080, 0x00008000, 0x00007f00, 0x0000007f},
        {0x0041193b, 0x0051e8c8, 0x00040808, 0x00fff8f8},
        {0x00824444, 0x0000c0c0, 0x00c2a066, 0x009222e0},
    };
    static const USHORT content_l6v5u5[4][4] =
    {
        {0x0000, 0xfdef, 0x0230, 0xfc00},
        {0x0010, 0x0200, 0x01e0, 0x000f},
        {0x4067, 0x53b9, 0x0421, 0xffff},
        {0x8108, 0x0318, 0xc28c, 0x909c},
    };
    static const struct
    {
        D3DFORMAT format;
        const char *name;
        const void *content;
        SIZE_T pixel_size;
        BOOL blue, alpha;
        unsigned int slop, slop_broken, alpha_broken;
    }
    formats[] =
    {
        {D3DFMT_V8U8,     "D3DFMT_V8U8",     content_v8u8,     sizeof(WORD),  FALSE, FALSE, 1, 0, FALSE},
        {D3DFMT_V16U16,   "D3DFMT_V16U16",   content_v16u16,   sizeof(DWORD), FALSE, FALSE, 1, 0, FALSE},
        {D3DFMT_Q8W8V8U8, "D3DFMT_Q8W8V8U8", content_q8w8v8u8, sizeof(DWORD), TRUE,  TRUE,  1, 0, TRUE },
        {D3DFMT_X8L8V8U8, "D3DFMT_X8L8V8U8", content_x8l8v8u8, sizeof(DWORD), TRUE,  FALSE, 1, 0, FALSE},
        {D3DFMT_L6V5U5,   "D3DFMT_L6V5U5",   content_l6v5u5,   sizeof(WORD),  TRUE,  FALSE, 4, 7, FALSE},
    };
    static const struct
    {
        D3DPOOL pool;
        UINT width;
    }
    tests[] =
    {
        {D3DPOOL_SYSTEMMEM, 4},
        {D3DPOOL_SYSTEMMEM, 1},
        {D3DPOOL_MANAGED,   4},
        {D3DPOOL_MANAGED,   1},
    };
    static const DWORD shader_code[] =
    {
        0xffff0101,                                                             /* ps_1_1                     */
        0x00000051, 0xa00f0000, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000, /* def c0, 0.5, 0.5, 0,5, 0,5 */
        0x00000042, 0xb00f0000,                                                 /* tex t0                     */
        0x00000004, 0x800f0000, 0xb0e40000, 0xa0e40000, 0xa0e40000,             /* mad r0, t0, c0, c0         */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD shader_code_alpha[] =
    {
        /* The idea of this shader is to replicate the alpha value in .rg, and set
         * blue to 1.0 iff the alpha value is < -1.0 and 0.0 otherwise. */
        0xffff0101,                                                             /* ps_1_1                     */
        0x00000051, 0xa00f0000, 0x3f000000, 0x3f000000, 0x3f000000, 0x3f000000, /* def c0, 0.5, 0.5, 0.5, 0.5 */
        0x00000051, 0xa00f0001, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, /* def c1, 1.0, 1.0, 0.0, 1.0 */
        0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, /* def c2, 0.0, 0.0, 1.0, 0.0 */
        0x00000042, 0xb00f0000,                                                 /* tex t0                     */
        0x00000004, 0x80070000, 0xb0ff0000, 0xa0e40000, 0xa0e40000,             /* mad r0.rgb, t0.a, c0, c0   */
        0x00000003, 0x80080000, 0xb1ff0000, 0xa0e40000,                         /* sub r0.a, -t0.a, c0        */
        0x00000050, 0x80080000, 0x80ff0000, 0xa0ff0001, 0xa0ff0002,             /* cnd r0.a, r0.a, c1.a, c2.a */
        0x00000005, 0x80070001, 0xa0e40001, 0x80e40000,                         /* mul r1.rgb, c1, r0         */
        0x00000004, 0x80070000, 0x80ff0000, 0xa0e40002, 0x80e40001,             /* mad r0.rgb, r0.a, c2, r1   */
        0x0000ffff                                                              /* end                        */
    };
    static const struct
    {
        struct vec3 position;
        struct vec2 texcrd;
    }
    quad[] =
    {
        /* Flip the y coordinate to make the input and
         * output arrays easier to compare. */
        {{ -1.0f,  -1.0f,  0.0f}, { 0.0f, 1.0f}},
        {{ -1.0f,   1.0f,  0.0f}, { 0.0f, 0.0f}},
        {{  1.0f,  -1.0f,  0.0f}, { 1.0f, 1.0f}},
        {{  1.0f,   1.0f,  0.0f}, { 1.0f, 0.0f}},
    };
    static const D3DCOLOR expected_alpha[4][4] =
    {
        {0x00808000, 0x007f7f00, 0x00ffff00, 0x00000000},
        {0x00909000, 0x00a0a000, 0x00b0b000, 0x00c0c000},
        {0x00d0d000, 0x00e0e000, 0x00f0f000, 0x00000000},
        {0x00101000, 0x00202000, 0x00010100, 0x00020200},
    };
    static const BOOL alpha_broken[4][4] =
    {
        {FALSE, FALSE, FALSE, FALSE},
        {FALSE, FALSE, FALSE, FALSE},
        {FALSE, FALSE, FALSE, TRUE },
        {FALSE, FALSE, FALSE, FALSE},
    };
    static const D3DCOLOR expected_colors[4][4] =
    {
        {0x00808080, 0x00fefeff, 0x00010780, 0x008080ff},
        {0x00018080, 0x00800180, 0x0080fe80, 0x00fe8080},
        {0x00ba98a0, 0x004767a8, 0x00888881, 0x007878ff},
        {0x00c3c3c0, 0x003f3f80, 0x00e51fe1, 0x005fa2c8},
    };
    D3DCOLOR expected_color;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("Pixel shaders not supported, skipping converted format test.\n");
        goto done;
    }

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreatePixelShader(device, shader_code_alpha, &shader_alpha);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); i++)
    {
        hr = IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, formats[i].format);
        if (FAILED(hr))
        {
            skip("Format %s not supported, skipping.\n", formats[i].name);
            continue;
        }

        for (j = 0; j < sizeof(tests) / sizeof(*tests); j++)
        {
            texture_sysmem = NULL;
            hr = IDirect3DDevice8_CreateTexture(device, tests[j].width, 4, 1, 0,
                    formats[i].format, tests[j].pool, &texture);
            ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

            hr = IDirect3DTexture8_LockRect(texture, 0, &locked_rect, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
            for (y = 0; y < 4; y++)
            {
                memcpy((char *)locked_rect.pBits + y * locked_rect.Pitch,
                        (char *)formats[i].content + y * 4 * formats[i].pixel_size,
                        tests[j].width * formats[i].pixel_size);
            }
            hr = IDirect3DTexture8_UnlockRect(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

            if (tests[j].pool == D3DPOOL_SYSTEMMEM)
            {
                texture_sysmem = texture;
                hr = IDirect3DDevice8_CreateTexture(device, tests[j].width, 4, 1, 0,
                        formats[i].format, D3DPOOL_DEFAULT, &texture);
                ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

                hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)texture_sysmem,
                        (IDirect3DBaseTexture8 *)texture);
                ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
                IDirect3DTexture8_Release(texture_sysmem);
            }

            hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
            hr = IDirect3DDevice8_SetPixelShader(device, shader_alpha);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

            hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00330033, 0.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
            hr = IDirect3DDevice8_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(*quad));
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice8_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < tests[j].width; x++)
                {
                    BOOL r200_broken = formats[i].alpha_broken && alpha_broken[y][x];
                    if (formats[i].alpha)
                        expected_color = expected_alpha[y][x];
                    else
                        expected_color = 0x00ffff00;

                    color = getPixelColor(device, 80 + 160 * x, 60 + 120 * y);
                    ok(color_match(color, expected_color, 1) || broken(r200_broken),
                            "Expected color 0x%08x, got 0x%08x, format %s, location %ux%u.\n",
                            expected_color, color, formats[i].name, x, y);
                }
            }
            hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

            hr = IDirect3DDevice8_SetPixelShader(device, shader);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

            hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00330033, 0.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
            hr = IDirect3DDevice8_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(*quad));
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice8_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < tests[j].width; x++)
                {
                    expected_color = expected_colors[y][x];
                    if (!formats[i].blue)
                        expected_color |= 0x000000ff;

                    color = getPixelColor(device, 80 + 160 * x, 60 + 120 * y);
                    ok(color_match(color, expected_color, formats[i].slop)
                            || broken(color_match(color, expected_color, formats[i].slop_broken)),
                            "Expected color 0x%08x, got 0x%08x, format %s, location %ux%u.\n",
                            expected_color, color, formats[i].name, x, y);
                }
            }
            hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

            IDirect3DTexture8_Release(texture);
        }
    }

    IDirect3DDevice8_DeletePixelShader(device, shader);
    IDirect3DDevice8_DeletePixelShader(device, shader_alpha);

done:
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_updatetexture(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    HWND window;
    HRESULT hr;
    IDirect3DBaseTexture8 *src, *dst;
    unsigned int t, i, f, l, x, y, z;
    D3DLOCKED_RECT locked_rect;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS8 caps;
    D3DCOLOR color;
    BOOL ati2n_supported, do_visual_test;
    static const struct
    {
        struct vec3 pos;
        struct vec2 texcoord;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
    };
    static const struct
    {
        struct vec3 pos;
        struct vec3 texcoord;
    }
    quad_cube_tex[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, -0.5f,  0.5f}},
        {{-1.0f,  1.0f, 0.0f}, {1.0f,  0.5f,  0.5f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, -0.5f, -0.5f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f,  0.5f, -0.5f}},
    };
    static const struct
    {
        UINT src_width, src_height;
        UINT dst_width, dst_height;
        UINT src_levels, dst_levels;
        D3DFORMAT src_format, dst_format;
        BOOL broken;
    }
    tests[] =
    {
        {8, 8, 8, 8, 0, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 0 */
        {8, 8, 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 1 */
        {8, 8, 8, 8, 2, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 2 */
        {8, 8, 8, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 3 */
        {8, 8, 8, 8, 4, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 4 */
        {8, 8, 2, 2, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 5 */
        /* The WARP renderer doesn't handle these cases correctly. */
        {8, 8, 8, 8, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, TRUE}, /* 6 */
        {8, 8, 4, 4, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, TRUE}, /* 7 */
        /* Not clear what happens here on Windows, it doesn't make much sense
         * though (on Nvidia it seems to upload the 4x4 surface into the 7x7
         * one or something like that). */
        /* {8, 8, 7, 7, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, */
        {8, 8, 8, 8, 1, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 8 */
        {4, 4, 8, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 9 */
        /* This one causes weird behavior on Windows (it probably writes out
         * of the texture memory). */
        /* {8, 8, 4, 4, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, */
        {8, 4, 4, 2, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 10 */
        {8, 4, 2, 4, 4, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 11 */
        {8, 8, 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, FALSE}, /* 12 */
        {8, 8, 8, 8, 4, 4, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, FALSE}, /* 13 */
        /* The data is converted correctly on AMD, on Nvidia nothing happens
         * (it draws a black quad). */
        {8, 8, 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_R5G6B5, TRUE}, /* 14 */
        /* This one doesn't seem to give the expected results on AMD. */
        /* {8, 8, 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_Q8W8V8U8, FALSE}, */
        {8, 8, 8, 8, 4, 4, MAKEFOURCC('A','T','I','2'), MAKEFOURCC('A','T','I','2'), FALSE}, /* 15 */
        {8, 8, 8, 8, 4, 2, MAKEFOURCC('A','T','I','2'), MAKEFOURCC('A','T','I','2'), FALSE}, /* 16 */
        {8, 8, 2, 2, 4, 2, MAKEFOURCC('A','T','I','2'), MAKEFOURCC('A','T','I','2'), FALSE}, /* 17 */
    };
    static const struct
    {
        D3DRESOURCETYPE type;
        DWORD fvf;
        const void *quad;
        unsigned int vertex_size;
        DWORD cap;
        const char *name;
    }
    texture_types[] =
    {
        {D3DRTYPE_TEXTURE, D3DFVF_XYZ | D3DFVF_TEX1,
         quad, sizeof(*quad), D3DPTEXTURECAPS_MIPMAP, "2D mipmapped"},

        {D3DRTYPE_CUBETEXTURE, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0),
         quad_cube_tex, sizeof(*quad_cube_tex), D3DPTEXTURECAPS_CUBEMAP, "Cube"},

        {D3DRTYPE_VOLUMETEXTURE, D3DFVF_XYZ | D3DFVF_TEX1,
         quad, sizeof(*quad), D3DPTEXTURECAPS_VOLUMEMAP, "Volume"}
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "Failed to set texture filtering state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "Failed to set texture filtering state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "Failed to set texture filtering state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "Failed to set texture filtering state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "Failed to set texture stage state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "Failed to set texture stage state, hr %#x.\n", hr);

    for (t = 0; t < sizeof(texture_types) / sizeof(*texture_types); ++t)
    {
        if (!(caps.TextureCaps & texture_types[t].cap))
        {
            skip("%s textures not supported, skipping some tests.\n", texture_types[t].name);
            continue;
        }

        if (FAILED(IDirect3D8_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, texture_types[t].type, MAKEFOURCC('A','T','I','2'))))
        {
            skip("%s ATI2N textures are not supported, skipping some tests.\n", texture_types[t].name);
            ati2n_supported = FALSE;
        }
        else
        {
            ati2n_supported = TRUE;
        }

        hr = IDirect3DDevice8_SetVertexShader(device, texture_types[t].fvf);
        ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

        for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
        {
            if (tests[i].src_format == MAKEFOURCC('A','T','I','2') && !ati2n_supported)
                continue;

            switch (texture_types[t].type)
            {
                case D3DRTYPE_TEXTURE:
                    hr = IDirect3DDevice8_CreateTexture(device,
                            tests[i].src_width, tests[i].src_height,
                            tests[i].src_levels, 0, tests[i].src_format, D3DPOOL_SYSTEMMEM,
                            (IDirect3DTexture8 **)&src);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    hr = IDirect3DDevice8_CreateTexture(device,
                            tests[i].dst_width, tests[i].dst_height,
                            tests[i].dst_levels, 0, tests[i].dst_format, D3DPOOL_DEFAULT,
                            (IDirect3DTexture8 **)&dst);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    break;
                case D3DRTYPE_CUBETEXTURE:
                    hr = IDirect3DDevice8_CreateCubeTexture(device,
                            tests[i].src_width,
                            tests[i].src_levels, 0, tests[i].src_format, D3DPOOL_SYSTEMMEM,
                            (IDirect3DCubeTexture8 **)&src);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    hr = IDirect3DDevice8_CreateCubeTexture(device,
                            tests[i].dst_width,
                            tests[i].dst_levels, 0, tests[i].dst_format, D3DPOOL_DEFAULT,
                            (IDirect3DCubeTexture8 **)&dst);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    break;
                case D3DRTYPE_VOLUMETEXTURE:
                    hr = IDirect3DDevice8_CreateVolumeTexture(device,
                            tests[i].src_width, tests[i].src_height, tests[i].src_width,
                            tests[i].src_levels, 0, tests[i].src_format, D3DPOOL_SYSTEMMEM,
                            (IDirect3DVolumeTexture8 **)&src);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    hr = IDirect3DDevice8_CreateVolumeTexture(device,
                            tests[i].dst_width, tests[i].dst_height, tests[i].dst_width,
                            tests[i].dst_levels, 0, tests[i].dst_format, D3DPOOL_DEFAULT,
                            (IDirect3DVolumeTexture8 **)&dst);
                    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, case %u, %u.\n", hr, t, i);
                    break;
                default:
                    trace("Unexpected resource type.\n");
            }

            /* Skip the visual part of the test for ATI2N (laziness) and cases that
             * give a different (and unlikely to be useful) result. */
            do_visual_test = (tests[i].src_format == D3DFMT_A8R8G8B8 || tests[i].src_format == D3DFMT_X8R8G8B8)
                    && tests[i].src_levels != 0
                    && tests[i].src_width >= tests[i].dst_width && tests[i].src_height >= tests[i].dst_height
                    && !(tests[i].src_width > tests[i].src_height && tests[i].dst_width < tests[i].dst_height);

            if (do_visual_test)
            {
                DWORD *ptr = NULL;
                unsigned int width, height, depth, row_pitch = 0, slice_pitch = 0;

                for (f = 0; f < (texture_types[t].type == D3DRTYPE_CUBETEXTURE ? 6 : 1); ++f)
                {
                    width = tests[i].src_width;
                    height = texture_types[t].type != D3DRTYPE_CUBETEXTURE ? tests[i].src_height : tests[i].src_width;
                    depth = texture_types[t].type == D3DRTYPE_VOLUMETEXTURE ? width : 1;

                    for (l = 0; l < tests[i].src_levels; ++l)
                    {
                        switch (texture_types[t].type)
                        {
                            case D3DRTYPE_TEXTURE:
                                hr = IDirect3DTexture8_LockRect((IDirect3DTexture8 *)src,
                                        l, &locked_rect, NULL, 0);
                                ptr = locked_rect.pBits;
                                row_pitch = locked_rect.Pitch / sizeof(*ptr);
                                break;
                            case D3DRTYPE_CUBETEXTURE:
                                hr = IDirect3DCubeTexture8_LockRect((IDirect3DCubeTexture8 *)src,
                                        f, l, &locked_rect, NULL, 0);
                                ptr = locked_rect.pBits;
                                row_pitch = locked_rect.Pitch / sizeof(*ptr);
                                break;
                            case D3DRTYPE_VOLUMETEXTURE:
                                hr = IDirect3DVolumeTexture8_LockBox((IDirect3DVolumeTexture8 *)src,
                                        l, &locked_box, NULL, 0);
                                ptr = locked_box.pBits;
                                row_pitch = locked_box.RowPitch / sizeof(*ptr);
                                slice_pitch = locked_box.SlicePitch / sizeof(*ptr);
                                break;
                            default:
                                trace("Unexpected resource type.\n");
                        }
                        ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);

                        for (z = 0; z < depth; ++z)
                        {
                            for (y = 0; y < height; ++y)
                            {
                                for (x = 0; x < width; ++x)
                                {
                                    ptr[z * slice_pitch + y * row_pitch + x] = 0xff000000
                                            | (DWORD)(x / (width - 1.0f) * 255.0f) << 16
                                            | (DWORD)(y / (height - 1.0f) * 255.0f) << 8;
                                }
                            }
                        }

                        switch (texture_types[t].type)
                        {
                            case D3DRTYPE_TEXTURE:
                                hr = IDirect3DTexture8_UnlockRect((IDirect3DTexture8 *)src, l);
                                break;
                            case D3DRTYPE_CUBETEXTURE:
                                hr = IDirect3DCubeTexture8_UnlockRect((IDirect3DCubeTexture8 *)src, f, l);
                                break;
                            case D3DRTYPE_VOLUMETEXTURE:
                                hr = IDirect3DVolumeTexture8_UnlockBox((IDirect3DVolumeTexture8 *)src, l);
                                break;
                            default:
                                trace("Unexpected resource type.\n");
                        }
                        ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

                        width >>= 1;
                        if (!width)
                            width = 1;
                        height >>= 1;
                        if (!height)
                            height = 1;
                        depth >>= 1;
                        if (!depth)
                            depth = 1;
                    }
                }
            }

            hr = IDirect3DDevice8_UpdateTexture(device, src, dst);
            if (FAILED(hr))
            {
                todo_wine ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u, %u.\n", hr, t, i);
                IDirect3DBaseTexture8_Release(src);
                IDirect3DBaseTexture8_Release(dst);
                continue;
            }
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u, %u.\n", hr, t, i);

            if (do_visual_test)
            {
                hr = IDirect3DDevice8_SetTexture(device, 0, dst);
                ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

                hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 1.0f, 0);
                ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

                hr = IDirect3DDevice8_BeginScene(device);
                ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
                hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,
                        texture_types[t].quad, texture_types[t].vertex_size);
                ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
                hr = IDirect3DDevice8_EndScene(device);
                ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

                color = getPixelColor(device, 320, 240);
                ok (color_match(color, 0x007f7f00, 2) || broken(tests[i].broken)
                        || broken(color == 0xdeadbeec), /* WARP device often just breaks down. */
                        "Got unexpected color 0x%08x, case %u, %u.\n", color, t, i);
            }

            IDirect3DBaseTexture8_Release(src);
            IDirect3DBaseTexture8_Release(dst);
        }
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static BOOL point_match(IDirect3DDevice8 *device, UINT x, UINT y, UINT r)
{
    D3DCOLOR color;

    color = D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff);
    if (!color_match(getPixelColor(device, x + r, y), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x - r, y), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x, y + r), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x, y - r), color, 1))
        return FALSE;

    ++r;
    color = D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0xff);
    if (!color_match(getPixelColor(device, x + r, y), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x - r, y), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x, y + r), color, 1))
        return FALSE;
    if (!color_match(getPixelColor(device, x, y - r), color, 1))
        return FALSE;

    return TRUE;
}

static void test_pointsize(void)
{
    static const float a = 0.5f, b = 0.5f, c = 0.5f;
    float ptsize, ptsizemax_orig, ptsizemin_orig;
    IDirect3DSurface8 *rt, *backbuffer, *depthstencil;
    IDirect3DTexture8 *tex1, *tex2;
    IDirect3DDevice8 *device;
    DWORD vs, ps;
    D3DLOCKED_RECT lr;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;
    unsigned int i, j;

    static const DWORD tex1_data[4] = {0x00ff0000, 0x00ff0000, 0x00000000, 0x00000000};
    static const DWORD tex2_data[4] = {0x00000000, 0x0000ff00, 0x00000000, 0x0000ff00};
    static const float vertices[] =
    {
         64.0f, 64.0f, 0.1f,
        128.0f, 64.0f, 0.1f,
        192.0f, 64.0f, 0.1f,
        256.0f, 64.0f, 0.1f,
        320.0f, 64.0f, 0.1f,
        384.0f, 64.0f, 0.1f,
        448.0f, 64.0f, 0.1f,
        512.0f, 64.0f, 0.1f,
    };
    static const struct
    {
        float x, y, z;
        float point_size;
    }
    vertex_pointsize = {64.0f, 64.0f, 0.1f, 48.0f},
    vertex_pointsize_scaled = {64.0f, 64.0f, 0.1f, 24.0f},
    vertex_pointsize_zero = {64.0f, 64.0f, 0.1f, 0.0f};
    static const DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),  /* position */
        D3DVSD_END()
    },
    decl_psize[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),  /* position, v0 */
        D3DVSD_REG(1, D3DVSDT_FLOAT1),  /* point size, v1 */
        D3DVSD_END()
    };
    static const DWORD vshader_code[] =
    {
        0xfffe0101,                                                 /* vs_1_1                 */
        0x00000005, 0x800f0000, 0x90000000, 0xa0e40000,             /* mul r0, v0.x, c0       */
        0x00000004, 0x800f0000, 0x90550000, 0xa0e40001, 0x80e40000, /* mad r0, v0.y, c1, r0   */
        0x00000004, 0x800f0000, 0x90aa0000, 0xa0e40002, 0x80e40000, /* mad r0, v0.z, c2, r0   */
        0x00000004, 0xc00f0000, 0x90ff0000, 0xa0e40003, 0x80e40000, /* mad oPos, v0.w, c3, r0 */
        0x0000ffff
    };
    static const DWORD vshader_psize_code[] =
    {
        0xfffe0101,                                                 /* vs_1_1                 */
        0x00000005, 0x800f0000, 0x90000000, 0xa0e40000,             /* mul r0, v0.x, c0       */
        0x00000004, 0x800f0000, 0x90550000, 0xa0e40001, 0x80e40000, /* mad r0, v0.y, c1, r0   */
        0x00000004, 0x800f0000, 0x90aa0000, 0xa0e40002, 0x80e40000, /* mad r0, v0.z, c2, r0   */
        0x00000004, 0xc00f0000, 0x90ff0000, 0xa0e40003, 0x80e40000, /* mad oPos, v0.w, c3, r0 */
        0x00000001, 0xc00f0002, 0x90000001,                         /* mov oPts, v1.x */
        0x0000ffff
    };
    static const DWORD pshader_code[] =
    {
        0xffff0101,                                                 /* ps_1_1                 */
        0x00000042, 0xb00f0000,                                     /* tex t0                 */
        0x00000042, 0xb00f0001,                                     /* tex t1                 */
        0x00000002, 0x800f0000, 0xb0e40000, 0xb0e40001,             /* add r0, t0, t1         */
        0x0000ffff
    };
    static const struct test_shader
    {
        DWORD version;
        const DWORD *code;
    }
    novs = {0, NULL},
    vs1 = {D3DVS_VERSION(1, 1), vshader_code},
    vs1_psize = {D3DVS_VERSION(1, 1), vshader_psize_code},
    nops = {0, NULL},
    ps1 = {D3DPS_VERSION(1, 1), pshader_code};
    static const struct
    {
        const DWORD *decl;
        const struct test_shader *vs;
        const struct test_shader *ps;
        DWORD accepted_fvf;
        unsigned int nonscaled_size, scaled_size;
    }
    test_setups[] =
    {
        {NULL, &novs, &nops, D3DFVF_XYZ, 32, 62},
        {decl, &vs1, &ps1, D3DFVF_XYZ, 32, 32},
        {NULL, &novs, &ps1, D3DFVF_XYZ, 32, 62},
        {decl, &vs1, &nops, D3DFVF_XYZ, 32, 32},
        {NULL, &novs, &nops, D3DFVF_XYZ | D3DFVF_PSIZE, 48, 48},
        {decl_psize, &vs1_psize, &ps1, D3DFVF_XYZ | D3DFVF_PSIZE, 48, 24},
    };
    static const struct
    {
        BOOL zero_size;
        BOOL scale;
        BOOL override_min;
        DWORD fvf;
        const void *vertex_data;
        unsigned int vertex_size;
    }
    tests[] =
    {
        {FALSE, FALSE, FALSE, D3DFVF_XYZ, vertices, sizeof(float) * 3},
        {FALSE, TRUE,  FALSE, D3DFVF_XYZ, vertices, sizeof(float) * 3},
        {FALSE, FALSE, TRUE,  D3DFVF_XYZ, vertices, sizeof(float) * 3},
        {TRUE,  FALSE, FALSE, D3DFVF_XYZ, vertices, sizeof(float) * 3},
        {FALSE, FALSE, FALSE, D3DFVF_XYZ | D3DFVF_PSIZE, &vertex_pointsize, sizeof(vertex_pointsize)},
        {FALSE, TRUE,  FALSE, D3DFVF_XYZ | D3DFVF_PSIZE, &vertex_pointsize_scaled, sizeof(vertex_pointsize_scaled)},
        {FALSE, FALSE, TRUE,  D3DFVF_XYZ | D3DFVF_PSIZE, &vertex_pointsize, sizeof(vertex_pointsize)},
        {TRUE,  FALSE, FALSE, D3DFVF_XYZ | D3DFVF_PSIZE, &vertex_pointsize_zero, sizeof(vertex_pointsize_zero)},
    };
    /* Transforms the coordinate system [-1.0;1.0]x[1.0;-1.0] to
     * [0.0;0.0]x[640.0;480.0]. Z is untouched. */
    D3DMATRIX matrix =
    {{{
        2.0f / 640.0f,           0.0f, 0.0f, 0.0f,
                 0.0f, -2.0f / 480.0f, 0.0f, 0.0f,
                 0.0f,           0.0f, 1.0f, 0.0f,
                -1.0f,           1.0f, 0.0f, 1.0f,
    }}};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (caps.MaxPointSize < 32.0f)
    {
        skip("MaxPointSize %f < 32.0, skipping.\n", caps.MaxPointSize);
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &matrix);
    ok(SUCCEEDED(hr), "Failed to set projection matrix, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    ptsize = 15.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[0], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    ptsize = 31.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[3], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    ptsize = 30.75f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[6], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    if (caps.MaxPointSize >= 63.0f)
    {
        ptsize = 63.0f;
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[9], sizeof(float) * 3);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        ptsize = 62.75f;
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[15], sizeof(float) * 3);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    }

    ptsize = 1.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[12], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_POINTSIZE_MAX, (DWORD *)&ptsizemax_orig);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_POINTSIZE_MIN, (DWORD *)&ptsizemin_orig);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);

    /* What happens if point scaling is disabled, and POINTSIZE_MAX < POINTSIZE? */
    ptsize = 15.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    ptsize = 1.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE_MAX, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[18], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE_MAX, *(DWORD *)&ptsizemax_orig);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    /* pointsize < pointsize_min < pointsize_max?
     * pointsize = 1.0, pointsize_min = 15.0, pointsize_max = default(usually 64.0) */
    ptsize = 1.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    ptsize = 15.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE_MIN, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[21], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE_MIN, *(DWORD *)&ptsizemin_orig);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    ok(point_match(device, 64, 64, 7), "point_match(64, 64, 7) failed, expected point size 15.\n");
    ok(point_match(device, 128, 64, 15), "point_match(128, 64, 15) failed, expected point size 31.\n");
    ok(point_match(device, 192, 64, 15), "point_match(192, 64, 15) failed, expected point size 31.\n");

    if (caps.MaxPointSize >= 63.0f)
    {
        ok(point_match(device, 256, 64, 31), "point_match(256, 64, 31) failed, expected point size 63.\n");
        ok(point_match(device, 384, 64, 31), "point_match(384, 64, 31) failed, expected point size 63.\n");
    }

    ok(point_match(device, 320, 64, 0), "point_match(320, 64, 0) failed, expected point size 1.\n");
    /* ptsize = 15, ptsize_max = 1 --> point has size 1 */
    ok(point_match(device, 448, 64, 0), "point_match(448, 64, 0) failed, expected point size 1.\n");
    /* ptsize = 1, ptsize_max = default(64), ptsize_min = 15 --> point has size 15 */
    ok(point_match(device, 512, 64, 7), "point_match(512, 64, 7) failed, expected point size 15.\n");

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    /* The following code tests point sprites with two textures, to see if each texture coordinate unit
     * generates texture coordinates for the point(result: Yes, it does)
     *
     * However, not all GL implementations support point sprites(they need GL_ARB_point_sprite), but there
     * is no point sprite cap bit in d3d because native d3d software emulates point sprites. Until the
     * SW emulation is implemented in wined3d, this test will fail on GL drivers that does not support them.
     */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex1);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture8_LockRect(tex1, 0, &lr, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    memcpy(lr.pBits, tex1_data, sizeof(tex1_data));
    hr = IDirect3DTexture8_UnlockRect(tex1, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture8_LockRect(tex2, 0, &lr, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    memcpy(lr.pBits, tex2_data, sizeof(tex2_data));
    hr = IDirect3DTexture8_UnlockRect(tex2, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)tex1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)tex2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSPRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable point sprites, hr %#x.\n", hr);
    ptsize = 32.0f;
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
    ok(SUCCEEDED(hr), "Failed to set point size, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[0], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 64 - 4, 64 - 4);
    ok(color == 0x00ff0000, "pSprite: Pixel (64 - 4),(64 - 4) has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 64 - 4, 64 + 4);
    ok(color == 0x00000000, "pSprite: Pixel (64 - 4),(64 + 4) has color 0x%08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 64 + 4, 64 + 4);
    ok(color == 0x0000ff00, "pSprite: Pixel (64 + 4),(64 + 4) has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 64 + 4, 64 - 4);
    ok(color == 0x00ffff00, "pSprite: Pixel (64 + 4),(64 - 4) has color 0x%08x, expected 0x00ffff00\n", color);
    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    U(matrix).m[0][0] =  1.0f / 64.0f;
    U(matrix).m[1][1] = -1.0f / 64.0f;
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, &matrix);
    ok(SUCCEEDED(hr), "Failed to set projection matrix, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderTarget(device, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &depthstencil);
    ok(SUCCEEDED(hr), "Failed to get depth / stencil buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, TRUE, &rt);
    ok(SUCCEEDED(hr), "Failed to create a render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSCALE_A, *(DWORD *)&a);
    ok(SUCCEEDED(hr), "Failed setting point scale attenuation coefficient, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSCALE_B, *(DWORD *)&b);
    ok(SUCCEEDED(hr), "Failed setting point scale attenuation coefficient, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSCALE_C, *(DWORD *)&c);
    ok(SUCCEEDED(hr), "Failed setting point scale attenuation coefficient, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, &S(U(matrix))._11, 4);
    ok(SUCCEEDED(hr), "Failed to set vertex shader constants, hr %#x.\n", hr);

    if (caps.MaxPointSize < 63.0f)
    {
        skip("MaxPointSize %f < 63.0, skipping some tests.\n", caps.MaxPointSize);
        goto cleanup;
    }

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, depthstencil);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    for (i = 0; i < sizeof(test_setups) / sizeof(test_setups[0]); ++i)
    {
        if (caps.VertexShaderVersion < test_setups[i].vs->version
                || caps.PixelShaderVersion < test_setups[i].ps->version)
        {
            skip("Vertex / pixel shader version not supported, skipping test.\n");
            continue;
        }
        if (test_setups[i].vs->code)
        {
            hr = IDirect3DDevice8_CreateVertexShader(device, test_setups[i].decl, test_setups[i].vs->code, &vs, 0);
            ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x (case %u).\n", hr, i);
        }
        else
        {
            vs = 0;
        }
        if (test_setups[i].ps->code)
        {
            hr = IDirect3DDevice8_CreatePixelShader(device, test_setups[i].ps->code, &ps);
            ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x (case %u).\n", hr, i);
        }
        else
        {
            ps = 0;
        }

        hr = IDirect3DDevice8_SetVertexShader(device, vs ? vs : test_setups[i].accepted_fvf);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetPixelShader(device, ps);
        ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

        for (j = 0; j < sizeof(tests) / sizeof(tests[0]); ++j)
        {
            unsigned int size = tests[j].override_min ? 63 : tests[j].zero_size ? 0 : tests[j].scale
                    ? test_setups[i].scaled_size : test_setups[i].nonscaled_size;

            if (test_setups[i].accepted_fvf != tests[j].fvf)
                continue;

            ptsize = tests[j].zero_size ? 0.0f : 32.0f;
            hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE, *(DWORD *)&ptsize);
            ok(SUCCEEDED(hr), "Failed to set pointsize, hr %#x.\n", hr);

            ptsize = tests[j].override_min ? 63.0f : tests[j].zero_size ? 0.0f : ptsizemin_orig;
            hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSIZE_MIN, *(DWORD *)&ptsize);
            ok(SUCCEEDED(hr), "Failed to set minimum pointsize, hr %#x.\n", hr);

            hr = IDirect3DDevice8_SetRenderState(device, D3DRS_POINTSCALEENABLE, tests[j].scale);
            ok(SUCCEEDED(hr), "Failed setting point scale state, hr %#x.\n", hr);

            hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

            hr = IDirect3DDevice8_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1,
                    tests[j].vertex_data, tests[j].vertex_size);
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice8_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            if (tests[j].zero_size)
            {
                /* Technically 0 pointsize is undefined in OpenGL but in practice it seems like
                 * it does the "useful" thing on all the drivers I tried. */
                /* On WARP it does draw some pixels, most of the time. */
                color = getPixelColor(device, 64, 64);
                ok(color_match(color, 0x0000ffff, 0)
                        || broken(color_match(color, 0x00ff0000, 0))
                        || broken(color_match(color, 0x00ffff00, 0))
                        || broken(color_match(color, 0x00000000, 0))
                        || broken(color_match(color, 0x0000ff00, 0)),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
            }
            else
            {
                color = getPixelColor(device, 64 - size / 2 + 1, 64 - size / 2 + 1);
                ok(color_match(color, 0x00ff0000, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 + size / 2 - 1, 64 - size / 2 + 1);
                ok(color_match(color, 0x00ffff00, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 - size / 2 + 1, 64 + size / 2 - 1);
                ok(color_match(color, 0x00000000, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 + size / 2 - 1, 64 + size / 2 - 1);
                ok(color_match(color, 0x0000ff00, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);

                color = getPixelColor(device, 64 - size / 2 - 1, 64 - size / 2 - 1);
                ok(color_match(color, 0x0000ffff, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 + size / 2 + 1, 64 - size / 2 - 1);
                ok(color_match(color, 0x0000ffff, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 - size / 2 - 1, 64 + size / 2 + 1);
                ok(color_match(color, 0x0000ffff, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
                color = getPixelColor(device, 64 + size / 2 + 1, 64 + size / 2 + 1);
                ok(color_match(color, 0x0000ffff, 0),
                        "Got unexpected color 0x%08x (case %u, %u, size %u).\n", color, i, j, size);
            }
        }
        IDirect3DDevice8_SetVertexShader(device, 0);
        IDirect3DDevice8_SetPixelShader(device, 0);
        if (vs)
            IDirect3DDevice8_DeleteVertexShader(device, vs);
        if (ps)
            IDirect3DDevice8_DeletePixelShader(device, ps);
    }
    hr = IDirect3DDevice8_SetRenderTarget(device, backbuffer, depthstencil);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

cleanup:
    IDirect3DSurface8_Release(backbuffer);
    IDirect3DSurface8_Release(depthstencil);
    IDirect3DSurface8_Release(rt);

    IDirect3DTexture8_Release(tex1);
    IDirect3DTexture8_Release(tex2);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_multisample_mismatch(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    HWND window;
    HRESULT hr;
    ULONG refcount;
    IDirect3DSurface8 *rt_multi, *ds;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D8_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping multisample mismatch test.\n");
        IDirect3D8_Release(d3d);
        return;
    }

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, FALSE, &rt_multi);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get original depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt_multi, ds);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface8_Release(ds);
    IDirect3DSurface8_Release(rt_multi);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_texcoordindex(void)
{
    static const D3DMATRIX mat =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
    }}};
    static const struct
    {
        struct vec3 pos;
        struct vec2 texcoord1;
        struct vec2 texcoord2;
        struct vec2 texcoord3;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}},
    };
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    HWND window;
    HRESULT hr;
    IDirect3DTexture8 *texture1, *texture2;
    D3DLOCKED_RECT locked_rect;
    ULONG refcount;
    D3DCOLOR color;
    DWORD *ptr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture1);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture2);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DTexture8_LockRect(texture1, 0, &locked_rect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    ptr = locked_rect.pBits;
    ptr[0] = 0xff000000;
    ptr[1] = 0xff00ff00;
    ptr[2] = 0xff0000ff;
    ptr[3] = 0xff00ffff;
    hr = IDirect3DTexture8_UnlockRect(texture1, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    hr = IDirect3DTexture8_LockRect(texture2, 0, &locked_rect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    ptr = locked_rect.pBits;
    ptr[0] = 0xff000000;
    ptr[1] = 0xff0000ff;
    ptr[2] = 0xffff0000;
    ptr[3] = 0xffff00ff;
    hr = IDirect3DTexture8_UnlockRect(texture2, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *)texture1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTexture(device, 1, (IDirect3DBaseTexture8 *)texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_TEX3);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetTextureStageState(device, 0, D3DTSS_TEXCOORDINDEX, 1);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXCOORDINDEX, 0);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00ff0000, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, 0x00ffffff, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    ok(SUCCEEDED(hr), "Failed to set texture transform flags, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_TEXTURE1, &mat);
    ok(SUCCEEDED(hr), "Failed to set transformation matrix, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00000000, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set texture transform flags, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetTextureStageState(device, 1, D3DTSS_TEXCOORDINDEX, 2);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00ff00ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, 0x00ffff00, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    IDirect3DTexture8_Release(texture1);
    IDirect3DTexture8_Release(texture2);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_vshader_input(void)
{
    DWORD swapped_twotexcrd_shader, swapped_onetexcrd_shader = 0;
    DWORD swapped_twotex_wrongidx_shader = 0, swapped_twotexcrd_rightorder_shader;
    DWORD texcoord_color_shader, color_ubyte_shader, color_color_shader, color_float_shader;
    DWORD color_nocolor_shader = 0;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const DWORD swapped_shader_code[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x00000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x00000002, 0xd00f0000, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD texcoord_color_shader_code[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000001, 0xd00f0000, 0x90e40007,                 /* mov oD0, v7          */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD color_color_shader_code[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000005, 0xd00f0000, 0xa0e40000, 0x90e40005,     /* mul oD0, c0, v5      */
        0x0000ffff                                          /* end                  */
    };
    static const float quad1[] =
    {
        -1.0f, -1.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
        -1.0f,  0.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
         0.0f, -1.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
         0.0f,  0.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
    };
    static const float quad4[] =
    {
         0.0f,  0.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
         0.0f,  1.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
         1.0f,  0.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
         1.0f,  1.0f, 0.1f,  1.0f, 0.0f, 1.0f, 0.0f,  0.0f, -1.0f,  0.5f, 0.0f,
    };
    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad1_color[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0x00ff8040},
        {{-1.0f,  0.0f, 0.1f}, 0x00ff8040},
        {{ 0.0f, -1.0f, 0.1f}, 0x00ff8040},
        {{ 0.0f,  0.0f, 0.1f}, 0x00ff8040},
    },
    quad2_color[] =
    {
        {{ 0.0f, -1.0f, 0.1f}, 0x00ff8040},
        {{ 0.0f,  0.0f, 0.1f}, 0x00ff8040},
        {{ 1.0f, -1.0f, 0.1f}, 0x00ff8040},
        {{ 1.0f,  0.0f, 0.1f}, 0x00ff8040},
    },
    quad3_color[] =
    {
        {{-1.0f,  0.0f, 0.1f}, 0x00ff8040},
        {{-1.0f,  1.0f, 0.1f}, 0x00ff8040},
        {{ 0.0f,  0.0f, 0.1f}, 0x00ff8040},
        {{ 0.0f,  1.0f, 0.1f}, 0x00ff8040},
    };
    static const float quad4_color[] =
    {
         0.0f,  0.0f, 0.1f,  1.0f, 1.0f, 0.0f, 0.0f,
         0.0f,  1.0f, 0.1f,  1.0f, 1.0f, 0.0f, 0.0f,
         1.0f,  0.0f, 0.1f,  1.0f, 1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.1f,  1.0f, 1.0f, 0.0f, 1.0f,
    };
    static const DWORD decl_twotexcrd[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(1, D3DVSDT_FLOAT4), /* texcoord0 */
        D3DVSD_REG(2, D3DVSDT_FLOAT4), /* texcoord1 */
        D3DVSD_END()
    };
    static const DWORD decl_twotexcrd_rightorder[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(2, D3DVSDT_FLOAT4), /* texcoord0 */
        D3DVSD_REG(1, D3DVSDT_FLOAT4), /* texcoord1 */
        D3DVSD_END()
    };
    static const DWORD decl_onetexcrd[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(1, D3DVSDT_FLOAT4), /* texcoord0 */
        D3DVSD_END()
    };
    static const DWORD decl_twotexcrd_wrongidx[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(2, D3DVSDT_FLOAT4), /* texcoord1 */
        D3DVSD_REG(3, D3DVSDT_FLOAT4), /* texcoord2 */
        D3DVSD_END()
    };
    static const DWORD decl_texcoord_color[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(7, D3DVSDT_D3DCOLOR), /* texcoord0 */
        D3DVSD_END()
    };
    static const DWORD decl_color_color[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3), /* position */
        D3DVSD_REG(5, D3DVSDT_D3DCOLOR), /* diffuse */
        D3DVSD_END()
    };
    static const DWORD decl_color_ubyte[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),
        D3DVSD_REG(5, D3DVSDT_UBYTE4),
        D3DVSD_END()
    };
    static const DWORD decl_color_float[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),
        D3DVSD_REG(5, D3DVSDT_FLOAT4),
        D3DVSD_END()
    };
    static const DWORD decl_nocolor[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(0, D3DVSDT_FLOAT3),
        D3DVSD_END()
    };
    static const float normalize[4] = {1.0f / 256.0f, 1.0f / 256.0f, 1.0f / 256.0f, 1.0f / 256.0f};
    static const float no_normalize[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
    {
        skip("No vs_1_1 support, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }

    hr = IDirect3DDevice8_CreateVertexShader(device, decl_twotexcrd, swapped_shader_code, &swapped_twotexcrd_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_onetexcrd, swapped_shader_code, &swapped_onetexcrd_shader, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Unexpected error while creating vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_twotexcrd_wrongidx, swapped_shader_code, &swapped_twotex_wrongidx_shader, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Unexpected error while creating vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_twotexcrd_rightorder, swapped_shader_code, &swapped_twotexcrd_rightorder_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateVertexShader(device, decl_texcoord_color, texcoord_color_shader_code, &texcoord_color_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_color_ubyte, color_color_shader_code, &color_ubyte_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_color_color, color_color_shader_code, &color_color_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_color_float, color_color_shader_code, &color_float_shader, 0);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_nocolor, color_color_shader_code, &color_nocolor_shader, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Unexpected error while creating vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, swapped_twotexcrd_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 11);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, swapped_twotexcrd_rightorder_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 11);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00ffff80, 1), "Got unexpected color 0x%08x for quad 1 (2crd).\n", color);
    color = getPixelColor(device, 480, 160);
    ok(color == 0x00000000, "Got unexpected color 0x%08x for quad 4 (2crd-rightorder).\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, texcoord_color_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1_color, sizeof(quad1_color[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, color_ubyte_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, normalize, 1);
    ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2_color, sizeof(quad2_color[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, no_normalize, 1);
    ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, color_color_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3_color, sizeof(quad3_color[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, color_float_shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4_color, sizeof(float) * 7);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    IDirect3DDevice8_SetVertexShader(device, 0);

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x80, 0x40), 1),
            "Input test: Quad 1(color-texcoord) returned color 0x%08x, expected 0x00ff8040\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x40, 0x80, 0xff), 1),
            "Input test: Quad 2(color-ubyte) returned color 0x%08x, expected 0x004080ff\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x80, 0x40), 1),
            "Input test: Quad 3(color-color) returned color 0x%08x, expected 0x00ff8040\n", color);
    color = getPixelColor(device, 480, 160);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x00), 1),
            "Input test: Quad 4(color-float) returned color 0x%08x, expected 0x00ffff00\n", color);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    IDirect3DDevice8_DeleteVertexShader(device, swapped_twotexcrd_shader);
    IDirect3DDevice8_DeleteVertexShader(device, swapped_onetexcrd_shader);
    IDirect3DDevice8_DeleteVertexShader(device, swapped_twotex_wrongidx_shader);
    IDirect3DDevice8_DeleteVertexShader(device, swapped_twotexcrd_rightorder_shader);
    IDirect3DDevice8_DeleteVertexShader(device, texcoord_color_shader);
    IDirect3DDevice8_DeleteVertexShader(device, color_ubyte_shader);
    IDirect3DDevice8_DeleteVertexShader(device, color_color_shader);
    IDirect3DDevice8_DeleteVertexShader(device, color_float_shader);
    IDirect3DDevice8_DeleteVertexShader(device, color_nocolor_shader);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_fixed_function_fvf(void)
{
    IDirect3DDevice8 *device;
    DWORD color;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0x00ffff00},
        {{-1.0f,  0.0f, 0.1f}, 0x00ffff00},
        {{ 0.0f, -1.0f, 0.1f}, 0x00ffff00},
        {{ 0.0f,  0.0f, 0.1f}, 0x00ffff00},
    };
    static const struct vec3 quad2[] =
    {
        {-1.0f, -1.0f, 0.1f},
        {-1.0f,  0.0f, 0.1f},
        { 0.0f, -1.0f, 0.1f},
        { 0.0f,  0.0f, 0.1f},
    };
    static const struct
    {
        struct vec4 position;
        DWORD diffuse;
    }
    quad_transformed[] =
    {
        {{ 90.0f, 110.0f, 0.1f, 2.0f}, 0x00ffff00},
        {{570.0f, 110.0f, 0.1f, 2.0f}, 0x00ffff00},
        {{ 90.0f, 300.0f, 0.1f, 2.0f}, 0x00ffff00},
        {{570.0f, 300.0f, 0.1f, 2.0f}, 0x00ffff00},
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ffff00,
            "D3DDECLTYPE_D3DCOLOR returned color %08x, expected 0x00ffff00\n", color);
    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    /* Test with no diffuse color attribute. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ffffff, "Got unexpected color 0x%08x in the no diffuse attribute test.\n", color);
    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    /* Test what happens with specular lighting enabled and no specular color attribute. */
    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SPECULARENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable specular lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad1, sizeof(quad1[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_SPECULARENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable specular lighting, hr %#x.\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ffff00, "Got unexpected color 0x%08x in the no specular attribute test.\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad_transformed, sizeof(quad_transformed[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 88, 108);
    ok(color == 0x000000ff,
            "pixel 88/108 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 92, 108);
    ok(color == 0x000000ff,
            "pixel 92/108 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 88, 112);
    ok(color == 0x000000ff,
            "pixel 88/112 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 92, 112);
    ok(color == 0x00ffff00,
            "pixel 92/112 has color %08x, expected 0x00ffff00\n", color);

    color = getPixelColor(device, 568, 108);
    ok(color == 0x000000ff,
            "pixel 568/108 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 572, 108);
    ok(color == 0x000000ff,
            "pixel 572/108 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 568, 112);
    ok(color == 0x00ffff00,
            "pixel 568/112 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 572, 112);
    ok(color == 0x000000ff,
            "pixel 572/112 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 88, 298);
    ok(color == 0x000000ff,
            "pixel 88/298 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 92, 298);
    ok(color == 0x00ffff00,
            "pixel 92/298 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 88, 302);
    ok(color == 0x000000ff,
            "pixel 88/302 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 92, 302);
    ok(color == 0x000000ff,
            "pixel 92/302 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 568, 298);
    ok(color == 0x00ffff00,
            "pixel 568/298 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 572, 298);
    ok(color == 0x000000ff,
            "pixel 572/298 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 568, 302);
    ok(color == 0x000000ff,
            "pixel 568/302 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 572, 302);
    ok(color == 0x000000ff,
            "pixel 572/302 has color %08x, expected 0x000000ff\n", color);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

START_TEST(visual)
{
    D3DADAPTER_IDENTIFIER8 identifier;
    IDirect3D8 *d3d;
    HRESULT hr;

    if (!(d3d = Direct3DCreate8(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D8 object.\n");
        return;
    }

    memset(&identifier, 0, sizeof(identifier));
    hr = IDirect3D8_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &identifier);
    ok(SUCCEEDED(hr), "Failed to get adapter identifier, hr %#x.\n", hr);
    trace("Driver string: \"%s\"\n", identifier.Driver);
    trace("Description string: \"%s\"\n", identifier.Description);
    /* Only Windows XP's default VGA driver should have an empty description */
    ok(identifier.Description[0] || broken(!strcmp(identifier.Driver, "vga.dll")), "Empty driver description.\n");
    trace("Driver version %d.%d.%d.%d\n",
            HIWORD(U(identifier.DriverVersion).HighPart), LOWORD(U(identifier.DriverVersion).HighPart),
            HIWORD(U(identifier.DriverVersion).LowPart), LOWORD(U(identifier.DriverVersion).LowPart));

    IDirect3D8_Release(d3d);

    test_sanity();
    depth_clamp_test();
    lighting_test();
    test_specular_lighting();
    clear_test();
    fog_test();
    z_range_test();
    offscreen_test();
    test_blend();
    test_scalar_instructions();
    fog_with_shader_test();
    cnd_test();
    p8_texture_test();
    texop_test();
    depth_buffer_test();
    depth_buffer2_test();
    intz_test();
    shadow_test();
    multisample_copy_rects_test();
    zenable_test();
    resz_test();
    fog_special_test();
    volume_dxt5_test();
    volume_v16u16_test();
    add_dirty_rect_test();
    test_3dc_formats();
    test_fog_interpolation();
    test_negative_fixedfunction_fog();
    test_table_fog_zw();
    test_signed_formats();
    test_updatetexture();
    test_pointsize();
    test_multisample_mismatch();
    test_texcoordindex();
    test_vshader_input();
    test_fixed_function_fvf();
}
