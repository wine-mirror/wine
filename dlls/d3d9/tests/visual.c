/*
 * Copyright 2005, 2007-2008 Henri Verbeet
 * Copyright (C) 2007-2013 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2008 Jason Green(for TransGaming)
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

/* This test framework allows limited testing of rendering results. Things are rendered, shown on
 * the framebuffer, read back from there and compared to expected colors.
 *
 * However, neither d3d nor opengl is guaranteed to be pixel exact, and thus the capability of this test
 * is rather limited. As a general guideline for adding tests, do not rely on corner pixels. Draw a big enough
 * area which shows specific behavior(like a quad on the whole screen), and try to get resulting colors with
 * all bits set or unset in all channels(like pure red, green, blue, white, black). Hopefully everything that
 * causes visible results in games can be tested in a way that does not depend on pixel exactness
 */

#define COBJMACROS
#include <d3d9.h>
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

static HWND create_window(void)
{
    WNDCLASSA wc = {0};
    HWND ret;
    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    ret = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_SYSMENU | WS_POPUP,
            0, 0, 640, 480, 0, 0, 0, 0);
    ShowWindow(ret, SW_SHOW);
    return ret;
}

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

/* Locks a given surface and returns the color at (x,y).  It's the caller's
 * responsibility to only pass in lockable surfaces and valid x,y coordinates */
static DWORD getPixelColorFromSurface(IDirect3DSurface9 *surface, UINT x, UINT y)
{
    DWORD color;
    HRESULT hr;
    D3DSURFACE_DESC desc;
    RECT rectToLock = {x, y, x+1, y+1};
    D3DLOCKED_RECT lockedRect;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    if(FAILED(hr))  /* This is not a test */
    {
        trace("Can't get the surface description, hr=%08x\n", hr);
        return 0xdeadbeef;
    }

    hr = IDirect3DSurface9_LockRect(surface, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))  /* This is not a test */
    {
        trace("Can't lock the surface, hr=%08x\n", hr);
        return 0xdeadbeef;
    }
    switch(desc.Format) {
        case D3DFMT_A8R8G8B8:
        {
            color = ((DWORD *) lockedRect.pBits)[0] & 0xffffffff;
            break;
        }
        default:
            trace("Error: unknown surface format: %d\n", desc.Format);
            color = 0xdeadbeef;
            break;
    }
    hr = IDirect3DSurface9_UnlockRect(surface);
    if(FAILED(hr))
    {
        trace("Can't unlock the surface, hr=%08x\n", hr);
    }
    return color;
}

static DWORD getPixelColor(IDirect3DDevice9 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DSurface9 *surf = NULL, *target = NULL;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 640, 480,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if (FAILED(hr) || !surf)
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%08x\n", hr);
        return 0xdeadbeef;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &target);
    if(FAILED(hr))
    {
        trace("Can't get the render target, hr=%08x\n", hr);
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DDevice9_GetRenderTargetData(device, target, surf);
    if (FAILED(hr))
    {
        trace("Can't read the render target data, hr=%08x\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    hr = IDirect3DSurface9_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%08x\n", hr);
        ret = 0xdeadbeeb;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface9_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%08x\n", hr);
    }

out:
    if(target) IDirect3DSurface9_Release(target);
    if(surf) IDirect3DSurface9_Release(surf);
    return ret;
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d, HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device;

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device)))
        return device;

    return NULL;
}

static IDirect3DDevice9 *init_d3d9(void)
{
    D3DADAPTER_IDENTIFIER9 identifier;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;

    if (!(d3d9 = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        win_skip("could not create D3D9\n");
        return NULL;
    }

    memset(&identifier, 0, sizeof(identifier));
    hr = IDirect3D9_GetAdapterIdentifier(d3d9, 0, 0, &identifier);
    ok(hr == D3D_OK, "Failed to get adapter identifier description\n");
    trace("Driver string: \"%s\"\n", identifier.Driver);
    trace("Description string: \"%s\"\n", identifier.Description);
    /* Only Windows XP's default VGA driver should have an empty description */
    ok(identifier.Description[0] != '\0' ||
       broken(strcmp(identifier.Driver, "vga.dll") == 0),
       "Empty driver description\n");
    trace("Device name string: \"%s\"\n", identifier.DeviceName);
    ok(identifier.DeviceName[0]  != '\0', "Empty device name\n");
    trace("Driver version %d.%d.%d.%d\n",
          HIWORD(U(identifier.DriverVersion).HighPart), LOWORD(U(identifier.DriverVersion).HighPart),
          HIWORD(U(identifier.DriverVersion).LowPart), LOWORD(U(identifier.DriverVersion).LowPart));

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    if ((device = create_device(d3d9, window, window, TRUE)))
        return device;

    DestroyWindow(window);
    return NULL;
}

static void cleanup_device(IDirect3DDevice9 *device)
{
    if (device)
    {
        D3DPRESENT_PARAMETERS present_parameters;
        IDirect3DSwapChain9 *swapchain;
        ULONG ref;

        IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
        IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters);
        IDirect3DSwapChain9_Release(swapchain);
        ref = IDirect3DDevice9_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);
        DestroyWindow(present_parameters.hDeviceWindow);
    }
}

struct vertex
{
    float x, y, z;
    DWORD diffuse;
};

struct tvertex
{
    float x, y, z, rhw;
    DWORD diffuse;
};

struct nvertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
};

static void lighting_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    DWORD color;
    D3DMATERIAL9 material, old_material;
    DWORD cop, carg;
    DWORD old_colorwrite;

    float mat[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                      0.0f, 1.0f, 0.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f };

    struct vertex unlitquad[] =
    {
        {-1.0f, -1.0f,   0.1f,                          0xffff0000},
        {-1.0f,  0.0f,   0.1f,                          0xffff0000},
        { 0.0f,  0.0f,   0.1f,                          0xffff0000},
        { 0.0f, -1.0f,   0.1f,                          0xffff0000},
    };
    struct vertex litquad[] =
    {
        {-1.0f,  0.0f,   0.1f,                          0xff00ff00},
        {-1.0f,  1.0f,   0.1f,                          0xff00ff00},
        { 0.0f,  1.0f,   0.1f,                          0xff00ff00},
        { 0.0f,  0.0f,   0.1f,                          0xff00ff00},
    };
    struct nvertex unlitnquad[] =
    {
        { 0.0f, -1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 0.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 1.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 1.0f, -1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
    };
    struct nvertex litnquad[] =
    {
        { 0.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xffffff00},
        { 0.0f,  1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xffffff00},
        { 1.0f,  1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xffffff00},
        { 1.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xffffff00},
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, (D3DMATRIX *)mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_COLORWRITEENABLE, &old_colorwrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(hr == D3D_OK)
    {
        /* No lights are defined... That means, lit vertices should be entirely black */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetFVF(device, nfvf);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360); /* Lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 160, 120); /* Upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 480, 360); /* Lower left quad - unlit with normals */
    ok(color == 0x000000ff, "Unlit quad with normals has color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 480, 120); /* Upper left quad - lit with normals */
    ok(color == 0x00000000, "Lit quad with normals has color 0x%08x, expected 0x00000000.\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_GetMaterial(device, &old_material);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetMaterial returned %08x\n", hr);
    memset(&material, 0, sizeof(material));
    material.Diffuse.r = 0.0;
    material.Diffuse.g = 0.0;
    material.Diffuse.b = 0.0;
    material.Diffuse.a = 1.0;
    material.Ambient.r = 0.0;
    material.Ambient.g = 0.0;
    material.Ambient.b = 0.0;
    material.Ambient.a = 0.0;
    material.Specular.r = 0.0;
    material.Specular.g = 0.0;
    material.Specular.b = 0.0;
    material.Specular.a = 0.0;
    material.Emissive.r = 0.0;
    material.Emissive.g = 0.0;
    material.Emissive.b = 0.0;
    material.Emissive.a = 0.0;
    material.Power = 0.0;
    hr = IDirect3DDevice9_SetMaterial(device, &material);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetMaterial returned %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_COLOROP, &cop);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_COLORARG1, &carg);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr)) {
        struct vertex lighting_test[] = {
            {-1.0,   -1.0,   0.1,    0x8000ff00},
            { 1.0,   -1.0,   0.1,    0x80000000},
            {-1.0,    1.0,   0.1,    0x8000ff00},
            { 1.0,    1.0,   0.1,    0x80000000}
        };
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, lighting_test, sizeof(lighting_test[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Lit vertex alpha test returned color %08x, expected 0x00ffffff\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, cop);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, old_colorwrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, carg);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetMaterial(device, &old_material);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetMaterial returned %08x\n", hr);
}

static void clear_test(IDirect3DDevice9 *device)
{
    /* Tests the correctness of clearing parameters */
    HRESULT hr;
    D3DRECT rect[2];
    D3DRECT rect_negneg;
    DWORD color;
    D3DVIEWPORT9 old_vp, vp;
    RECT scissor;
    DWORD oldColorWrite;
    BOOL invalid_clear_failed = FALSE;

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

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
    /* Clear 2 rectangles with one call. The refrast returns an error in this case, every real driver tested so far
     * returns D3D_OK, but ignores the rectangle silently
     */
    hr = IDirect3DDevice9_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    if(hr == D3DERR_INVALIDCALL) invalid_clear_failed = TRUE;

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.y1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice9_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    if(hr == D3DERR_INVALIDCALL) invalid_clear_failed = TRUE;

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    if(invalid_clear_failed) {
        /* If the negative rectangle was refused, the other rectangles in the list shouldn't be cleared either */
        ok(color == 0x00ffffff, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    } else {
        /* If the negative rectangle was dropped silently, the correct ones are cleared */
        ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    }
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* Hack to work around a nvidia windows driver bug. The clear below is supposed to
     * clear the red quad in the top left part of the render target. For some reason it
     * doesn't work if the clear color is 0xffffffff on some versions of the Nvidia Windows
     * driver(tested on 8.17.12.5896, Win7). A clear with a different color works around
     * this bug and fixes the clear with the white color. Even 0xfeffffff works, but let's
     * pick some obvious value
     */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xdeadbabe, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    /* Test how the viewport affects clears */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);

    vp.X = 160;
    vp.Y = 120;
    vp.Width = 160;
    vp.Height = 120;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    vp.X = 320;
    vp.Y = 240;
    vp.Width = 320;
    vp.Height = 240;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);
    rect[0].x1 = 160;
    rect[0].y1 = 120;
    rect[0].x2 = 480;
    rect[0].y2 = 360;
    hr = IDirect3DDevice9_Clear(device, 1, &rect[0], D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);

    color = getPixelColor(device, 158, 118);
    ok(color == 0x00ffffff, "(158,118) has color %08x\n", color);
    color = getPixelColor(device, 162, 118);
    ok(color == 0x00ffffff, "(162,118) has color %08x\n", color);
    color = getPixelColor(device, 158, 122);
    ok(color == 0x00ffffff, "(158,122) has color %08x\n", color);
    color = getPixelColor(device, 162, 122);
    ok(color == 0x000000ff, "(162,122) has color %08x\n", color);

    color = getPixelColor(device, 318, 238);
    ok(color == 0x000000ff, "(318,238) has color %08x\n", color);
    color = getPixelColor(device, 322, 238);
    ok(color == 0x00ffffff, "(322,328) has color %08x\n", color);
    color = getPixelColor(device, 318, 242);
    ok(color == 0x00ffffff, "(318,242) has color %08x\n", color);
    color = getPixelColor(device, 322, 242);
    ok(color == 0x0000ff00, "(322,242) has color %08x\n", color);

    color = getPixelColor(device, 478, 358);
    ok(color == 0x0000ff00, "(478,358 has color %08x\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color == 0x00ffffff, "(482,358) has color %08x\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x00ffffff, "(478,362) has color %08x\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color == 0x00ffffff, "(482,362) has color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    scissor.left = 160;
    scissor.right = 480;
    scissor.top = 120;
    scissor.bottom = 360;
    hr = IDirect3DDevice9_SetScissorRect(device, &scissor);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 1, &rect[1], D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %08x\n", hr);

    color = getPixelColor(device, 158, 118);
    ok(color == 0x00ffffff, "Pixel 158/118 has color %08x\n", color);
    color = getPixelColor(device, 162, 118);
    ok(color == 0x00ffffff, "Pixel 162/118 has color %08x\n", color);
    color = getPixelColor(device, 158, 122);
    ok(color == 0x00ffffff, "Pixel 158/122 has color %08x\n", color);
    color = getPixelColor(device, 162, 122);
    ok(color == 0x00ff0000, "Pixel 162/122 has color %08x\n", color);

    color = getPixelColor(device, 158, 358);
    ok(color == 0x00ffffff, "Pixel 158/358 has color %08x\n", color);
    color = getPixelColor(device, 162, 358);
    ok(color == 0x0000ff00, "Pixel 162/358 has color %08x\n", color);
    color = getPixelColor(device, 158, 358);
    ok(color == 0x00ffffff, "Pixel 158/358 has color %08x\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color == 0x00ffffff, "Pixel 162/362 has color %08x\n", color);

    color = getPixelColor(device, 478, 118);
    ok(color == 0x00ffffff, "Pixel 158/118 has color %08x\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color == 0x0000ff00, "Pixel 162/118 has color %08x\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color == 0x00ffffff, "Pixel 158/122 has color %08x\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color == 0x00ffffff, "Pixel 162/122 has color %08x\n", color);

    color = getPixelColor(device, 478, 358);
    ok(color == 0x0000ff00, "Pixel 478/358 has color %08x\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x00ffffff, "Pixel 478/118 has color %08x\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color == 0x00ffffff, "Pixel 482/122 has color %08x\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color == 0x00ffffff, "Pixel 482/122 has color %08x\n", color);

    color = getPixelColor(device, 318, 238);
    ok(color == 0x00ff0000, "Pixel 318/238 has color %08x\n", color);
    color = getPixelColor(device, 318, 242);
    ok(color == 0x0000ff00, "Pixel 318/242 has color %08x\n", color);
    color = getPixelColor(device, 322, 238);
    ok(color == 0x0000ff00, "Pixel 322/238 has color %08x\n", color);
    color = getPixelColor(device, 322, 242);
    ok(color == 0x0000ff00, "Pixel 322/242 has color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_COLORWRITEENABLE, &oldColorWrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);

    /* Same nvidia windows driver trouble with white clears as earlier in the same test */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xdeadbeef, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, oldColorWrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);

    /* Colorwriteenable does not affect the clear */
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Color write protected clear returned color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00ffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with %08x\n", hr);

    rect[0].x1 = 0;
    rect[0].y1 = 0;
    rect[0].x2 = 640;
    rect[0].y2 = 480;
    hr = IDirect3DDevice9_Clear(device, 0, rect, D3DCLEAR_TARGET, 0x00ff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with %08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff), 1),
            "Clear with count = 0, rect != NULL has color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 1, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with %08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
            "Clear with count = 1, rect = NULL has color %08x\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
}

static void color_fill_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DSurface9 *backbuffer = NULL;
    IDirect3DSurface9 *rt_surface = NULL;
    IDirect3DSurface9 *offscreen_surface = NULL;
    DWORD fill_color, color;

    /* Test ColorFill on a the backbuffer (should pass) */
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(backbuffer)
    {
        fill_color = 0x112233;
        hr = IDirect3DDevice9_ColorFill(device, backbuffer, NULL, fill_color);
        ok(SUCCEEDED(hr), "Color fill failed, hr %#x.\n", hr);

        color = getPixelColor(device, 0, 0);
        ok(color == fill_color, "Expected color %08x, got %08x\n", fill_color, color);

        IDirect3DSurface9_Release(backbuffer);
    }

    /* Test ColorFill on a render target surface (should pass) */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &rt_surface, NULL );
    ok(hr == D3D_OK, "Unable to create render target surface, hr = %08x\n", hr);
    if(rt_surface)
    {
        fill_color = 0x445566;
        hr = IDirect3DDevice9_ColorFill(device, rt_surface, NULL, fill_color);
        ok(SUCCEEDED(hr), "Color fill failed, hr %#x.\n", hr);

        color = getPixelColorFromSurface(rt_surface, 0, 0);
        ok(color == fill_color, "Expected color %08x, got %08x\n", fill_color, color);

        IDirect3DSurface9_Release(rt_surface);
    }

    /* Test ColorFill on an offscreen plain surface in D3DPOOL_DEFAULT (should pass) */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &offscreen_surface, NULL);
    ok(hr == D3D_OK, "Unable to create offscreen plain surface, hr = %08x\n", hr);
    if(offscreen_surface)
    {
        fill_color = 0x778899;
        hr = IDirect3DDevice9_ColorFill(device, offscreen_surface, NULL, fill_color);
        ok(SUCCEEDED(hr), "Color fill failed, hr %#x.\n", hr);

        color = getPixelColorFromSurface(offscreen_surface, 0, 0);
        ok(color == fill_color, "Expected color %08x, got %08x\n", fill_color, color);

        IDirect3DSurface9_Release(offscreen_surface);
    }

    /* Try ColorFill on an offscreen surface in sysmem (should fail) */
    offscreen_surface = NULL;
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreen_surface, NULL);
    ok(hr == D3D_OK, "Unable to create offscreen plain surface, hr = %08x\n", hr);
    if(offscreen_surface)
    {
        hr = IDirect3DDevice9_ColorFill(device, offscreen_surface, NULL, 0);
        ok(hr == D3DERR_INVALIDCALL, "ColorFill on offscreen sysmem surface failed with hr = %08x\n", hr);

        IDirect3DSurface9_Release(offscreen_surface);
    }
}

typedef struct {
    float in[4];
    DWORD out;
} test_data_t;

/*
 *  c7      mova    ARGB            mov     ARGB
 * -2.4     -2      0x00ffff00      -3      0x00ff0000
 * -1.6     -2      0x00ffff00      -2      0x00ffff00
 * -0.4      0      0x0000ffff      -1      0x0000ff00
 *  0.4      0      0x0000ffff       0      0x0000ffff
 *  1.6      2      0x00ff00ff       1      0x000000ff
 *  2.4      2      0x00ff00ff       2      0x00ff00ff
 */
static void test_mova(IDirect3DDevice9 *device)
{
    static const DWORD mova_test[] = {
        0xfffe0200,                                                             /* vs_2_0                       */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x05000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 1.0, 0.0, 0.0, 1.0   */
        0x05000051, 0xa00f0001, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, /* def c1, 1.0, 1.0, 0.0, 1.0   */
        0x05000051, 0xa00f0002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, /* def c2, 0.0, 1.0, 0.0, 1.0   */
        0x05000051, 0xa00f0003, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 0.0, 1.0, 1.0, 1.0   */
        0x05000051, 0xa00f0004, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* def c4, 0.0, 0.0, 1.0, 1.0   */
        0x05000051, 0xa00f0005, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, /* def c5, 1.0, 0.0, 1.0, 1.0   */
        0x05000051, 0xa00f0006, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c6, 1.0, 1.0, 1.0, 1.0   */
        0x0200002e, 0xb0010000, 0xa0000007,                                     /* mova a0.x, c7.x              */
        0x03000001, 0xd00f0000, 0xa0e42003, 0xb0000000,                         /* mov oD0, c[a0.x + 3]         */
        0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x0000ffff                                                              /* END                          */
    };
    static const DWORD mov_test[] = {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 1.0, 0.0, 0.0, 1.0   */
        0x00000051, 0xa00f0001, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, /* def c1, 1.0, 1.0, 0.0, 1.0   */
        0x00000051, 0xa00f0002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, /* def c2, 0.0, 1.0, 0.0, 1.0   */
        0x00000051, 0xa00f0003, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 0.0, 1.0, 1.0, 1.0   */
        0x00000051, 0xa00f0004, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* def c4, 0.0, 0.0, 1.0, 1.0   */
        0x00000051, 0xa00f0005, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, /* def c5, 1.0, 0.0, 1.0, 1.0   */
        0x00000051, 0xa00f0006, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c6, 1.0, 1.0, 1.0, 1.0   */
        0x00000001, 0xb0010000, 0xa0000007,                                     /* mov a0.x, c7.x               */
        0x00000001, 0xd00f0000, 0xa0e42003,                                     /* mov oD0, c[a0.x + 3]         */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x0000ffff                                                              /* END                          */
    };

    static const test_data_t test_data[2][6] = {
        {
            {{-2.4f, 0.0f, 0.0f, 0.0f}, 0x00ff0000},
            {{-1.6f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
            {{-0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ff00},
            {{ 0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
            {{ 1.6f, 0.0f, 0.0f, 0.0f}, 0x000000ff},
            {{ 2.4f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff}
        },
        {
            {{-2.4f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
            {{-1.6f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
            {{-0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
            {{ 0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
            {{ 1.6f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff},
            {{ 2.4f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff}
        }
    };

    static const float quad[][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexShader9 *mova_shader = NULL;
    IDirect3DVertexShader9 *mov_shader = NULL;
    HRESULT hr;
    UINT i, j;

    hr = IDirect3DDevice9_CreateVertexShader(device, mova_test, &mova_shader);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, mov_test, &mov_shader);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, mov_shader);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
    for(j = 0; j < 2; ++j)
    {
        for (i = 0; i < (sizeof(test_data[0]) / sizeof(test_data_t)); ++i)
        {
            DWORD color;

            hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 7, test_data[j][i].in, 1);
            ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);

            hr = IDirect3DDevice9_BeginScene(device);
            ok(SUCCEEDED(hr), "BeginScene failed (%08x)\n", hr);

            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], 3 * sizeof(float));
            ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(SUCCEEDED(hr), "EndScene failed (%08x)\n", hr);

            color = getPixelColor(device, 320, 240);
            ok(color == test_data[j][i].out, "Expected color %08x, got %08x (for input %f, instruction %s)\n",
               test_data[j][i].out, color, test_data[j][i].in[0], j == 0 ? "mov" : "mova");

            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "Present failed (%08x)\n", hr);

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
            ok(SUCCEEDED(hr), "Clear failed (%08x)\n", hr);
        }
        hr = IDirect3DDevice9_SetVertexShader(device, mova_shader);
        ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DVertexShader9_Release(mova_shader);
    IDirect3DVertexShader9_Release(mov_shader);
}

struct sVertex {
    float x, y, z;
    DWORD diffuse;
    DWORD specular;
};

struct sVertexT {
    float x, y, z, rhw;
    DWORD diffuse;
    DWORD specular;
};

static void fog_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    D3DCOLOR color;
    float start = 0.0f, end = 1.0f;
    D3DCAPS9 caps;
    int i;

    /* Gets full z based fog with linear fog, no fog with specular color */
    struct sVertex untransformed_1[] = {
        {-1,    -1,   0.1f,         0xffff0000,     0xff000000  },
        {-1,     0,   0.1f,         0xffff0000,     0xff000000  },
        { 0,     0,   0.1f,         0xffff0000,     0xff000000  },
        { 0,    -1,   0.1f,         0xffff0000,     0xff000000  },
    };
    /* Ok, I am too lazy to deal with transform matrices */
    struct sVertex untransformed_2[] = {
        {-1,     0,   1.0f,         0xffff0000,     0xff000000  },
        {-1,     1,   1.0f,         0xffff0000,     0xff000000  },
        { 0,     1,   1.0f,         0xffff0000,     0xff000000  },
        { 0,     0,   1.0f,         0xffff0000,     0xff000000  },
    };
    /* Untransformed ones. Give them a different diffuse color to make the test look
     * nicer. It also makes making sure that they are drawn correctly easier.
     */
    struct sVertexT transformed_1[] = {
        {320,    0,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {640,    0,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {640,  240,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {320,  240,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
    };
    struct sVertexT transformed_2[] = {
        {320,  240,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {640,  240,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {640,  480,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
        {320,  480,   1.0f, 1.0f,   0xffffff00,     0xff000000  },
    };
    struct vertex rev_fog_quads[] = {
       {-1.0,   -1.0,   0.1,    0x000000ff},
       {-1.0,    0.0,   0.1,    0x000000ff},
       { 0.0,    0.0,   0.1,    0x000000ff},
       { 0.0,   -1.0,   0.1,    0x000000ff},

       { 0.0,   -1.0,   0.9,    0x000000ff},
       { 0.0,    0.0,   0.9,    0x000000ff},
       { 1.0,    0.0,   0.9,    0x000000ff},
       { 1.0,   -1.0,   0.9,    0x000000ff},

       { 0.0,    0.0,   0.4,    0x000000ff},
       { 0.0,    1.0,   0.4,    0x000000ff},
       { 1.0,    1.0,   0.4,    0x000000ff},
       { 1.0,    0.0,   0.4,    0x000000ff},

       {-1.0,    0.0,   0.7,    0x000000ff},
       {-1.0,    1.0,   0.7,    0x000000ff},
       { 0.0,    1.0,   0.7,    0x000000ff},
       { 0.0,    0.0,   0.7,    0x000000ff},
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    const float ident_mat[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    const float world_mat1[16] =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -0.5f, 1.0f
    };
    const float world_mat2[16] =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f
    };
    const float proj_mat[16] =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 1.0f
    };

    const struct sVertex far_quad1[] =
    {
        {-1.0f, -1.0f, 0.5f, 0xffff0000, 0xff000000},
        {-1.0f,  0.0f, 0.5f, 0xffff0000, 0xff000000},
        { 0.0f,  0.0f, 0.5f, 0xffff0000, 0xff000000},
        { 0.0f, -1.0f, 0.5f, 0xffff0000, 0xff000000},
    };
    const struct sVertex far_quad2[] =
    {
        {-1.0f, 0.0f, 1.5f, 0xffff0000, 0xff000000},
        {-1.0f, 1.0f, 1.5f, 0xffff0000, 0xff000000},
        { 0.0f, 1.0f, 1.5f, 0xffff0000, 0xff000000},
        { 0.0f, 0.0f, 1.5f, 0xffff0000, 0xff000000},
    };

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps returned %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xff00ff00 /* A nice green */);
    ok(hr == D3D_OK, "Setting fog color returned %#08x\n", hr);

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog returned %08x\n", hr);

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog end returned %08x\n", hr);

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK)
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %08x\n", hr);
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, untransformed_1,
                                                     sizeof(untransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);

        /* That makes it use the Z value */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR returned %#08x\n", hr);
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, untransformed_2,
                                                     sizeof(untransformed_2[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);

        /* transformed verts */
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %08x\n", hr);
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                                                     sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %08x\n", hr);
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_2,
                                                     sizeof(transformed_2[0]));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %08x\n", hr);
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x0000ff00, 1), "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ffff00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    if(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        color = getPixelColor(device, 480, 360);
        ok(color_match(color, 0x0000ff00, 1), "Transformed vertex with linear table fog has color %08x\n", color);
    }
    else
    {
        /* Without fog table support the vertex fog is still applied, even though table fog is turned on.
         * The settings above result in no fogging with vertex fog
         */
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00ffff00, "Transformed vertex with linear vertex fog has color %08x\n", color);
        trace("Info: Table fog not supported by this device\n");
    }
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* Now test the special case fogstart == fogend */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK)
    {
        start = 512;
        end = 512;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
        ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
        ok(hr == D3D_OK, "Setting fog end returned %08x\n", hr);

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %08x\n", hr);

        /* Untransformed vertex, z coord = 0.1, fogstart = 512, fogend = 512. Would result in
         * a completely fog-free primitive because start > zcoord, but because start == end, the primitive
         * is fully covered by fog. The same happens to the 2nd untransformed quad with z = 1.0.
         * The third transformed quad remains unfogged because the fogcoords are read from the specular
         * color and has fixed fogstart and fogend.
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, untransformed_1,
                sizeof(untransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, untransformed_2,
                sizeof(untransformed_2[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %08x\n", hr);
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %08x\n", hr);
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x0000ff00, 1), "Untransformed vertex with vertex fog and z = 0.1 has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x0000ff00, 1), "Untransformed vertex with vertex fog and z = 1.0 has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ffff00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* Test "reversed" fog without shaders. With shaders this fails on a few Windows D3D implementations,
     * but without shaders it seems to work everywhere
     */
    end = 0.2;
    start = 0.8;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog end returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok( hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    /* Test reversed fog without shaders. ATI cards have problems with reversed fog and shaders, so
     * it doesn't seem very important for games. ATI cards also have problems with reversed table fog,
     * so skip this for now
     */
    for(i = 0; i < 1 /*2 - Table fog test disabled, fails on ATI */; i++) {
        const char *mode = (i ? "table" : "vertex");
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, i == 0 ? D3DFOG_LINEAR : D3DFOG_NONE);
        ok( hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, i == 0 ? D3DFOG_NONE : D3DFOG_LINEAR);
        ok( hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_BeginScene(device);
        ok( hr == D3D_OK, "IDirect3DDDevice9_BeginScene returned %08x\n", hr);
        if(SUCCEEDED(hr)) {
            WORD Indices2[] = { 0,  1,  2,  2,  3, 0,
                                4,  5,  6,  6,  7, 4,
                                8,  9, 10, 10, 11, 8,
                            12, 13, 14, 14, 15, 12};

            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */,
                    16 /* NumVerts */, 8 /*PrimCount */, Indices2, D3DFMT_INDEX16, rev_fog_quads,
                    sizeof(rev_fog_quads[0]));
            ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok( hr == D3D_OK, "IDirect3DDDevice9_EndScene returned %08x\n", hr);
        }
        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x0000ff00, 1),
                "Reversed %s fog: z=0.1 has color 0x%08x, expected 0x0000ff00 or 0x0000fe00\n", mode, color);

        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0x2b, 0xd4), 2),
                "Reversed %s fog: z=0.7 has color 0x%08x\n", mode, color);

        color = getPixelColor(device, 480, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xaa, 0x55), 2),
                "Reversed %s fog: z=0.4 has color 0x%08x\n", mode, color);

        color = getPixelColor(device, 480, 360);
        ok(color == 0x000000ff, "Reversed %s fog: z=0.9 has color 0x%08x, expected 0x000000ff\n", mode, color);

        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

        if(!(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE)) {
            skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping reversed table fog test\n");
            break;
        }
    }

    if (caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        /* A simple fog + non-identity world matrix test */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (const D3DMATRIX *)world_mat1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %#08x\n", hr);

        start = 0.0;
        end = 1.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *)&start));
        ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *)&end));
        ok(hr == D3D_OK, "Setting fog end returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %#08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
        ok(hr == D3D_OK, "Turning off vertex fog returned %#08x\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %#08x\n", hr);

        if (IDirect3DDevice9_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
            ok(hr == D3D_OK, "SetVertexShader returned %#08x\n", hr);

            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                    2, Indices, D3DFMT_INDEX16, far_quad1, sizeof(far_quad1[0]));
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                    2, Indices, D3DFMT_INDEX16, far_quad2, sizeof(far_quad2[0]));
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "EndScene returned %#08x\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00), 4),
                "Unfogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Fogged out quad has color %08x\n", color);

        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

        /* Test fog behavior with an orthogonal (but non-identity) projection matrix */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (const D3DMATRIX *)world_mat2);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, (const D3DMATRIX *)proj_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear returned %#08x\n", hr);

        if (IDirect3DDevice9_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
            ok(hr == D3D_OK, "SetVertexShader returned %#08x\n", hr);

            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                    2, Indices, D3DFMT_INDEX16, untransformed_1, sizeof(untransformed_1[0]));
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4,
                    2, Indices, D3DFMT_INDEX16, untransformed_2, sizeof(untransformed_2[0]));
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "EndScene returned %#08x\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00e51900, 4), "Partially fogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Fogged out quad has color %08x\n", color);

        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

        hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (const D3DMATRIX *)ident_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, (const D3DMATRIX *)ident_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
    }
    else
    {
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests\n");
    }

    /* Test RANGEFOG vs FOGTABLEMODE */
    if ((caps.RasterCaps & (D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGRANGE)) ==
            (D3DPRASTERCAPS_FOGTABLE | D3DPRASTERCAPS_FOGRANGE))
    {
        struct sVertex untransformed_3[] =
        {
            {-1.0,-1.0,   0.4999f,      0xffff0000,     0xff000000  },
            {-1.0, 1.0,   0.4999f,      0xffff0000,     0xff000000  },
            { 1.0,-1.0,   0.4999f,      0xffff0000,     0xff000000  },
            { 1.0, 1.0,   0.4999f,      0xffff0000,     0xff000000  },
        };

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_RANGEFOGENABLE, TRUE);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);

        /* z=0.4999, set the fogstart to 0.5 and fogend slightly higher. If range fog
         * is not used, the fog coordinate will be equal to fogstart and the quad not
         * fogged. If range fog is used the fog coordinate will be slightly higher and
         * the fog coordinate will be > fogend, so we get a fully fogged quad. The fog
         * is calculated per vertex and interpolated, so even the center of the screen
         * where the difference doesn't matter will be fogged, but check the corners in
         * case a d3d/gl implementation decides to calculate the fog factor per fragment */
        start = 0.5f;
        end = 0.50001f;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);

        /* Table fog: Range fog is not used */
        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_BeginScene failed, hr %#x.\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, untransformed_3, sizeof(*untransformed_3));
            ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);
        }
        color = getPixelColor(device, 10, 10);
        ok(color == 0x00ff0000, "Rangefog with table fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 630, 10);
        ok(color == 0x00ff0000, "Rangefog with table fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 10, 470);
        ok(color == 0x00ff0000, "Rangefog with table fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 630, 470);
        ok(color == 0x00ff0000, "Rangefog with table fog returned color 0x%08x\n", color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed, hr %#x.\n", hr);

        /* Vertex fog: Rangefog is used */
        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP returned %#08x\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, untransformed_3, sizeof(*untransformed_3));
            ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);
        }
        color = getPixelColor(device, 10, 10);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Rangefog with vertex fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 630, 10);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Rangefog with vertex fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 10, 470);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Rangefog with vertex fog returned color 0x%08x\n", color);
        color = getPixelColor(device, 630, 470);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 1),
                "Rangefog with vertex fog returned color 0x%08x\n", color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_RANGEFOGENABLE, FALSE);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    }
    else
    {
        skip("Range fog or table fog not supported, skipping range fog tests\n");
    }

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    ok( hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %08x\n", hr);
}

/* This test verifies the behaviour of cube maps wrt. texture wrapping.
 * D3D cube map wrapping always behaves like GL_CLAMP_TO_EDGE,
 * regardless of the actual addressing mode set. The way this test works is
 * that we sample in one of the corners of the cubemap with filtering enabled,
 * and check the interpolated color. There are essentially two reasonable
 * things an implementation can do: Either pick one of the faces and
 * interpolate the edge texel with itself (i.e., clamp within the face), or
 * interpolate between the edge texels of the three involved faces. It should
 * never involve the border color or the other side (texcoord wrapping) of a
 * face in the interpolation. */
static void test_cube_wrap(IDirect3DDevice9 *device)
{
    static const float quad[][6] = {
        {-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    static const struct {
        D3DTEXTUREADDRESS mode;
        const char *name;
    } address_modes[] = {
        {D3DTADDRESS_WRAP, "D3DTADDRESS_WRAP"},
        {D3DTADDRESS_MIRROR, "D3DTADDRESS_MIRROR"},
        {D3DTADDRESS_CLAMP, "D3DTADDRESS_CLAMP"},
        {D3DTADDRESS_BORDER, "D3DTADDRESS_BORDER"},
        {D3DTADDRESS_MIRRORONCE, "D3DTADDRESS_MIRRORONCE"},
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DCubeTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DSurface9 *face_surface;
    D3DLOCKED_RECT locked_rect;
    HRESULT hr;
    UINT x;
    INT y, face;

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "CreateOffscreenPlainSurface failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateCubeTexture(device, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateCubeTexture failed (0x%08x)\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    for (y = 0; y < 128; ++y)
    {
        DWORD *ptr = (DWORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
        for (x = 0; x < 64; ++x)
        {
            *ptr++ = 0xff0000ff;
        }
        for (x = 64; x < 128; ++x)
        {
            *ptr++ = 0xffff0000;
        }
    }

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    hr= IDirect3DCubeTexture9_GetCubeMapSurface(texture, 0, 0, &face_surface);
    ok(SUCCEEDED(hr), "GetCubeMapSurface failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_UpdateSurface(device, surface, NULL, face_surface, NULL);
    ok(SUCCEEDED(hr), "UpdateSurface failed (0x%08x)\n", hr);

    IDirect3DSurface9_Release(face_surface);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    for (y = 0; y < 128; ++y)
    {
        DWORD *ptr = (DWORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
        for (x = 0; x < 64; ++x)
        {
            *ptr++ = 0xffff0000;
        }
        for (x = 64; x < 128; ++x)
        {
            *ptr++ = 0xff0000ff;
        }
    }

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    /* Create cube faces */
    for (face = 1; face < 6; ++face)
    {
        hr= IDirect3DCubeTexture9_GetCubeMapSurface(texture, face, 0, &face_surface);
        ok(SUCCEEDED(hr), "GetCubeMapSurface failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_UpdateSurface(device, surface, NULL, face_surface, NULL);
        ok(SUCCEEDED(hr), "UpdateSurface failed (0x%08x)\n", hr);

        IDirect3DSurface9_Release(face_surface);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_BORDERCOLOR, 0xff00ff00);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_BORDERCOLOR failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    for (x = 0; x < (sizeof(address_modes) / sizeof(*address_modes)); ++x)
    {
        DWORD color;

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, address_modes[x].mode);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSU (%s) failed (0x%08x)\n", address_modes[x].name, hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, address_modes[x].mode);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSV (%s) failed (0x%08x)\n", address_modes[x].name, hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0xff), 1),
                "Got color 0x%08x for addressing mode %s, expected 0x000000ff.\n",
                color, address_modes[x].name);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed (0x%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DCubeTexture9_Release(texture);
    IDirect3DSurface9_Release(surface);
}

static void offscreen_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *offscreenTexture = NULL;
    IDirect3DSurface9 *backbuffer = NULL, *offscreen = NULL;
    DWORD color;

    static const float quad[][5] = {
        {-0.5f, -0.5f, 0.1f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.1f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.1f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.1f, 1.0f, 1.0f},
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %08x\n", hr);
    if(!offscreenTexture) {
        trace("Failed to create an X8R8G8B8 offscreen texture, trying R5G6B5\n");
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
        ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %08x\n", hr);
        if(!offscreenTexture) {
            skip("Cannot create an offscreen render target\n");
            goto out;
        }
    }

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %08x\n", hr);
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "SetFVF failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK) {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        /* Draw without textures - Should result in a white quad */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) offscreenTexture);
        ok(hr == D3D_OK, "SetTexture failed, %08x\n", hr);

        /* This time with the texture */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %08x\n", hr);

        IDirect3DDevice9_EndScene(device);
    }

    /* Center quad - should be white */
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Offscreen failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    /* Some quad in the cleared part of the texture */
    color = getPixelColor(device, 170, 240);
    ok(color == 0x00ff00ff, "Offscreen failed: Got color 0x%08x, expected 0x00ff00ff.\n", color);
    /* Part of the originally cleared back buffer */
    color = getPixelColor(device, 10, 10);
    ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    if(0) {
        /* Lower left corner of the screen, where back buffer offscreen rendering draws the offscreen texture.
         * It should be red, but the offscreen texture may leave some junk there. Not tested yet. Depending on
         * the offscreen rendering mode this test would succeed or fail
         */
        color = getPixelColor(device, 10, 470);
        ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

out:
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture returned %#x.\n", hr);

    /* restore things */
    if (backbuffer)
    {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderTarget returned %#x.\n", hr);
        IDirect3DSurface9_Release(backbuffer);
    }
    if(offscreenTexture) {
        IDirect3DTexture9_Release(offscreenTexture);
    }
    if(offscreen) {
        IDirect3DSurface9_Release(offscreen);
    }
}

/* This test tests fog in combination with shaders.
 * What's tested: linear fog (vertex and table) with pixel shader
 *                linear table fog with non foggy vertex shader
 *                vertex fog with foggy vertex shader, non-linear
 *                fog with shader, non-linear fog with foggy shader,
 *                linear table fog with foggy shader
 */
static void fog_with_shader_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    union {
        float f;
        DWORD i;
    } start, end;
    unsigned int i, j;

    /* basic vertex shader without fog computation ("non foggy") */
    static const DWORD vertex_shader_code1[] =
    {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x0000001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                  */
        0x0000ffff
    };
    /* basic vertex shader with reversed fog computation ("foggy") */
    static const DWORD vertex_shader_code2[] =
    {
        0xfffe0101,                                                             /* vs_1_1                        */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0               */
        0x0000001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                 */
        0x00000051, 0xa00f0000, 0xbfa00000, 0x00000000, 0xbf666666, 0x00000000, /* def c0, -1.25, 0.0, -0.9, 0.0 */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                  */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                   */
        0x00000002, 0x800f0000, 0x90aa0000, 0xa0aa0000,                         /* add r0, v0.z, c0.z            */
        0x00000005, 0xc00f0001, 0x80000000, 0xa0000000,                         /* mul oFog, r0.x, c0.x          */
        0x0000ffff
    };
    /* basic vertex shader with reversed fog computation ("foggy"), vs_2_0 */
    static const DWORD vertex_shader_code3[] =
    {
        0xfffe0200,                                                             /* vs_2_0                        */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0               */
        0x0200001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                 */
        0x05000051, 0xa00f0000, 0xbfa00000, 0x00000000, 0xbf666666, 0x00000000, /* def c0, -1.25, 0.0, -0.9, 0.0 */
        0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                  */
        0x02000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                   */
        0x03000002, 0x800f0000, 0x90aa0000, 0xa0aa0000,                         /* add r0, v0.z, c0.z            */
        0x03000005, 0xc00f0001, 0x80000000, 0xa0000000,                         /* mul oFog, r0.x, c0.x          */
        0x0000ffff
    };
    /* basic pixel shader */
    static const DWORD pixel_shader_code[] =
    {
        0xffff0101,                                                             /* ps_1_1     */
        0x00000001, 0x800f0000, 0x90e40000,                                     /* mov r0, v0 */
        0x0000ffff
    };
    static const DWORD pixel_shader_code2[] =
    {
        0xffff0200,                                                             /* ps_2_0     */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl v0 */
        0x02000001, 0x800f0800, 0x90e40000,                                     /* mov oC0, v0 */
        0x0000ffff
    };

    static struct vertex quad[] = {
        {-1.0f, -1.0f,  0.0f,          0xffff0000  },
        {-1.0f,  1.0f,  0.0f,          0xffff0000  },
        { 1.0f, -1.0f,  0.0f,          0xffff0000  },
        { 1.0f,  1.0f,  0.0f,          0xffff0000  },
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexShader9      *vertex_shader[4]   = {NULL, NULL, NULL, NULL};
    IDirect3DPixelShader9       *pixel_shader[3]    = {NULL, NULL, NULL};

    /* This reference data was collected on a nVidia GeForce 7600GS driver version 84.19 DirectX version 9.0c on Windows XP */
    static const struct test_data_t {
        int vshader;
        int pshader;
        D3DFOGMODE vfog;
        D3DFOGMODE tfog;
        unsigned int color[11];
    } test_data[] = {
        /* only pixel shader: */
        {0, 1, D3DFOG_NONE, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_EXP, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_EXP2, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {0, 1, D3DFOG_LINEAR, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* vertex shader */
        {1, 0, D3DFOG_NONE, D3DFOG_NONE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
         0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_NONE, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_EXP, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        {1, 0, D3DFOG_EXP2, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 0, D3DFOG_LINEAR, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* vertex shader and pixel shader */
        /* The next 4 tests would read the fog coord output, but it isn't available.
         * The result is a fully fogged quad, no matter what the Z coord is. This is on
         * a geforce 7400, 97.52 driver, Windows Vista, but probably hardware dependent.
         * These tests should be disabled if some other hardware behaves differently
         */
        {1, 1, D3DFOG_NONE, D3DFOG_NONE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP, D3DFOG_NONE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP2, D3DFOG_NONE,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},

        /* These use the Z coordinate with linear table fog */
        {1, 1, D3DFOG_NONE, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_EXP2, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
        {1, 1, D3DFOG_LINEAR, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},

        /* Non-linear table fog without fog coord */
        {1, 1, D3DFOG_NONE, D3DFOG_EXP,
        {0x00ff0000, 0x00e71800, 0x00d12e00, 0x00bd4200, 0x00ab5400, 0x009b6400,
        0x008d7200, 0x007f8000, 0x00738c00, 0x00689700, 0x005ea100}},
        {1, 1, D3DFOG_NONE, D3DFOG_EXP2,
        {0x00fd0200, 0x00f50200, 0x00f50a00, 0x00e91600, 0x00d92600, 0x00c73800,
        0x00b24d00, 0x009c6300, 0x00867900, 0x00728d00, 0x005ea100}},

        /* These tests fail on older Nvidia drivers */
        /* foggy vertex shader */
        {2, 0, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        {3, 0, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 0, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 0, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 0, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* foggy vertex shader and pixel shader. First 4 tests with vertex fog,
         * all using the fixed fog-coord linear fog
         */
        /* vs_1_1 with ps_1_1 */
        {2, 1, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* vs_2_0 with ps_1_1 */
        {3, 1, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 1, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 1, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 1, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* vs_1_1 with ps_2_0 */
        {2, 2, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 2, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 2, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 2, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* vs_2_0 with ps_2_0 */
        {3, 2, D3DFOG_NONE, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 2, D3DFOG_EXP, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 2, D3DFOG_EXP2, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {3, 2, D3DFOG_LINEAR, D3DFOG_NONE,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* These use table fog. Here the shader-provided fog coordinate is
         * ignored and the z coordinate used instead
         */
        {2, 1, D3DFOG_NONE, D3DFOG_EXP,
        {0x00ff0000, 0x00e71800, 0x00d12e00, 0x00bd4200, 0x00ab5400, 0x009b6400,
        0x008d7200, 0x007f8000, 0x00738c00, 0x00689700, 0x005ea100}},
        {2, 1, D3DFOG_NONE, D3DFOG_EXP2,
        {0x00fd0200, 0x00f50200, 0x00f50a00, 0x00e91600, 0x00d92600, 0x00c73800,
        0x00b24d00, 0x009c6300, 0x00867900, 0x00728d00, 0x005ea100}},
        {2, 1, D3DFOG_NONE, D3DFOG_LINEAR,
        {0x00ff0000, 0x00ff0000, 0x00df2000, 0x00bf4000, 0x009f6000, 0x007f8000,
        0x005fa000, 0x0040bf00, 0x0020df00, 0x0000ff00, 0x0000ff00}},
    };

    /* NOTE: changing these values will not affect the tests with foggy vertex shader, as the values are hardcoded in the shader*/
    start.f=0.1f;
    end.f=0.9f;

    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code1, &vertex_shader[1]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code2, &vertex_shader[2]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code3, &vertex_shader[3]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &pixel_shader[1]);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code2, &pixel_shader[2]);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xff00ff00 /* A nice green */);
    ok(hr == D3D_OK, "Setting fog color failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog failed (%08x)\n", hr);

    /* Use fogtart = 0.1 and end = 0.9 to test behavior outside the fog transition phase, too*/
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, start.i);
    ok(hr == D3D_OK, "Setting fog start failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, end.i);
    ok(hr == D3D_OK, "Setting fog end failed (%08x)\n", hr);

    for (i = 0; i < sizeof(test_data)/sizeof(test_data[0]); i++)
    {
        hr = IDirect3DDevice9_SetVertexShader(device, vertex_shader[test_data[i].vshader]);
        ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, pixel_shader[test_data[i].pshader]);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, test_data[i].vfog);
        ok( hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, test_data[i].tfog);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR failed (%08x)\n", hr);

        for(j=0; j < 11; j++)
        {
            /* Don't use the whole zrange to prevent rounding errors */
            quad[0].z = 0.001f + (float)j / 10.02f;
            quad[1].z = 0.001f + (float)j / 10.02f;
            quad[2].z = 0.001f + (float)j / 10.02f;
            quad[3].z = 0.001f + (float)j / 10.02f;

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);

            hr = IDirect3DDevice9_BeginScene(device);
            ok( hr == D3D_OK, "BeginScene returned failed (%08x)\n", hr);

            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
            ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "EndScene failed (%08x)\n", hr);

            /* As the red and green component are the result of blending use 5% tolerance on the expected value */
            color = getPixelColor(device, 128, 240);
            ok(color_match(color, test_data[i].color[j], 13),
                "fog vs%i ps%i fvm%i ftm%i %d: got color %08x, expected %08x +-5%%\n",
                test_data[i].vshader, test_data[i].pshader, test_data[i].vfog, test_data[i].tfog, j, color, test_data[i].color[j]);

            IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        }
    }

    /* reset states */
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations failed (%08x)\n", hr);

    IDirect3DVertexShader9_Release(vertex_shader[1]);
    IDirect3DVertexShader9_Release(vertex_shader[2]);
    IDirect3DVertexShader9_Release(vertex_shader[3]);
    IDirect3DPixelShader9_Release(pixel_shader[1]);
    IDirect3DPixelShader9_Release(pixel_shader[2]);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
}

static void generate_bumpmap_textures(IDirect3DDevice9 *device) {
    unsigned int i, x, y;
    HRESULT hr;
    IDirect3DTexture9 *texture[2] = {NULL, NULL};
    D3DLOCKED_RECT locked_rect;

    /* Generate the textures */
    for(i=0; i<2; i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0, i?D3DFMT_A8R8G8B8:D3DFMT_V8U8,
                                            D3DPOOL_MANAGED, &texture[i], NULL);
        ok(SUCCEEDED(hr), "CreateTexture failed (0x%08x)\n", hr);

        hr = IDirect3DTexture9_LockRect(texture[i], 0, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
        for (y = 0; y < 128; ++y)
        {
            if(i)
            { /* Set up black texture with 2x2 texel white spot in the middle */
                DWORD *ptr = (DWORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
                for (x = 0; x < 128; ++x)
                {
                    if(y>62 && y<66 && x>62 && x<66)
                        *ptr++ = 0xffffffff;
                    else
                        *ptr++ = 0xff000000;
                }
            }
            else
            { /* Set up a displacement map which points away from the center parallel to the closest axis.
               * (if multiplied with bumpenvmat)
              */
                WORD *ptr = (WORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
                for (x = 0; x < 128; ++x)
                {
                    if(abs(x-64)>abs(y-64))
                    {
                        if(x < 64)
                            *ptr++ = 0xc000;
                        else
                            *ptr++ = 0x4000;
                    }
                    else
                    {
                        if(y < 64)
                            *ptr++ = 0x0040;
                        else
                            *ptr++ = 0x00c0;
                    }
                }
            }
        }
        hr = IDirect3DTexture9_UnlockRect(texture[i], 0);
        ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, i, (IDirect3DBaseTexture9 *)texture[i]);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

        /* Disable texture filtering */
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSU failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSV failed (0x%08x)\n", hr);
    }
}

/* test the behavior of the texbem instruction
 * with normal 2D and projective 2D textures
 */
static void texbem_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    int i;

    static const DWORD pixel_shader_code[] = {
        0xffff0101,                         /* ps_1_1*/
        0x00000042, 0xb00f0000,             /* tex t0*/
        0x00000043, 0xb00f0001, 0xb0e40000, /* texbem t1, t0*/
        0x00000001, 0x800f0000, 0xb0e40001, /* mov r0, t1*/
        0x0000ffff
    };
    static const DWORD double_texbem_code[] =  {
        0xffff0103,                                         /* ps_1_3           */
        0x00000042, 0xb00f0000,                             /* tex t0           */
        0x00000043, 0xb00f0001, 0xb0e40000,                 /* texbem t1, t0    */
        0x00000042, 0xb00f0002,                             /* tex t2           */
        0x00000043, 0xb00f0003, 0xb0e40002,                 /* texbem t3, t2    */
        0x00000002, 0x800f0000, 0xb0e40001, 0xb0e40003,     /* add r0, t1, t3   */
        0x0000ffff                                          /* end              */
    };


    static const float quad[][7] = {
        {-128.0f/640.0f, -128.0f/480.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f},
        {-128.0f/640.0f,  128.0f/480.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f},
        { 128.0f/640.0f, -128.0f/480.0f, 0.1f, 1.0f, 0.0f, 1.0f, 0.0f},
        { 128.0f/640.0f,  128.0f/480.0f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f},
    };
    static const float quad_proj[][9] = {
        {-128.0f/640.0f, -128.0f/480.0f, 0.1f, 0.0f, 0.0f,   0.0f,   0.0f, 0.0f, 128.0f},
        {-128.0f/640.0f,  128.0f/480.0f, 0.1f, 0.0f, 1.0f,   0.0f, 128.0f, 0.0f, 128.0f},
        { 128.0f/640.0f, -128.0f/480.0f, 0.1f, 1.0f, 0.0f, 128.0f,   0.0f, 0.0f, 128.0f},
        { 128.0f/640.0f,  128.0f/480.0f, 0.1f, 1.0f, 1.0f, 128.0f, 128.0f, 0.0f, 128.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[][4] = { {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    },{
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    } };

    /* use asymmetric matrix to test loading */
    float bumpenvmat[4] = {0.0,0.5,-0.5,0.0};

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DPixelShader9       *pixel_shader       = NULL;
    IDirect3DTexture9           *texture            = NULL, *texture1, *texture2;
    D3DLOCKED_RECT locked_rect;

    generate_bumpmap_textures(device);

    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT00, *(LPDWORD)&bumpenvmat[0]);
    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT01, *(LPDWORD)&bumpenvmat[1]);
    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT10, *(LPDWORD)&bumpenvmat[2]);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT11, *(LPDWORD)&bumpenvmat[3]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);

    for(i=0; i<2; i++)
    {
        if(i)
        {
            hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4|D3DTTFF_PROJECTED);
            ok(SUCCEEDED(hr), "SetTextureStageState D3DTSS_TEXTURETRANSFORMFLAGS failed (0x%08x)\n", hr);
        }

        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements[i], &vertex_declaration);
        ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
        ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &pixel_shader);
        ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, pixel_shader);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

        if(!i)
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        else
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad_proj[0], sizeof(quad_proj[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

        color = getPixelColor(device, 320-32, 240);
        ok(color_match(color, 0x00ffffff, 4), "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320+32, 240);
        ok(color_match(color, 0x00ffffff, 4), "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240-32);
        ok(color_match(color, 0x00ffffff, 4), "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240+32);
        ok(color_match(color, 0x00ffffff, 4), "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
        IDirect3DPixelShader9_Release(pixel_shader);

        hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
        ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
    }

    /* clean up */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DTSS_TEXTURETRANSFORMFLAGS failed (0x%08x)\n", hr);

    for(i=0; i<2; i++)
    {
        hr = IDirect3DDevice9_GetTexture(device, i, (IDirect3DBaseTexture9 **) &texture);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_GetTexture failed (0x%08x)\n", hr);
        IDirect3DTexture9_Release(texture); /* For the GetTexture */
        hr = IDirect3DDevice9_SetTexture(device, i, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);
        IDirect3DTexture9_Release(texture);
    }

    /* Test double texbem */
    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_V8U8, D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_V8U8, D3DPOOL_MANAGED, &texture1, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 8, 8, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture2, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, double_texbem_code, &pixel_shader);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);

    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
    ((signed char *) locked_rect.pBits)[0] = (-1.0 / 8.0) * 127;
    ((signed char *) locked_rect.pBits)[1] = ( 1.0 / 8.0) * 127;

    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    hr = IDirect3DTexture9_LockRect(texture1, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
    ((signed char *) locked_rect.pBits)[0] = (-2.0 / 8.0) * 127;
    ((signed char *) locked_rect.pBits)[1] = (-4.0 / 8.0) * 127;
    hr = IDirect3DTexture9_UnlockRect(texture1, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    {
        /* Some data without any meaning, just to have an 8x8 array to see which element is picked */
#define tex  0x00ff0000
#define tex1 0x0000ff00
#define origin 0x000000ff
        static const DWORD pixel_data[] = {
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, tex1      , 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, origin,     0x000000ff, tex       , 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
            0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff,
        };
#undef tex1
#undef tex2
#undef origin

        hr = IDirect3DTexture9_LockRect(texture2, 0, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
        for(i = 0; i < 8; i++) {
            memcpy(((char *) locked_rect.pBits) + i * locked_rect.Pitch, pixel_data + 8 * i, 8 * sizeof(DWORD));
        }
        hr = IDirect3DTexture9_UnlockRect(texture2, 0);
        ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, (IDirect3DBaseTexture9 *) texture2);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 2, (IDirect3DBaseTexture9 *) texture1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 3, (IDirect3DBaseTexture9 *) texture2);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, pixel_shader);
    ok(SUCCEEDED(hr), "Direct3DDevice9_SetPixelShader failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX4);
    ok(SUCCEEDED(hr), "Direct3DDevice9_SetPixelShader failed (0x%08x)\n", hr);

    bumpenvmat[0] =-1.0;  bumpenvmat[2] =  2.0;
    bumpenvmat[1] = 0.0;  bumpenvmat[3] =  0.0;
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT00, *(LPDWORD)&bumpenvmat[0]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT01, *(LPDWORD)&bumpenvmat[1]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT10, *(LPDWORD)&bumpenvmat[2]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT11, *(LPDWORD)&bumpenvmat[3]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);

    bumpenvmat[0] = 1.5; bumpenvmat[2] =  0.0;
    bumpenvmat[1] = 0.0; bumpenvmat[3] =  0.5;
    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_BUMPENVMAT00, *(LPDWORD)&bumpenvmat[0]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_BUMPENVMAT01, *(LPDWORD)&bumpenvmat[1]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_BUMPENVMAT10, *(LPDWORD)&bumpenvmat[2]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_BUMPENVMAT11, *(LPDWORD)&bumpenvmat[3]);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 3, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 3, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetSamplerState returned %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        static const float double_quad[] = {
            -1.0,   -1.0,   0.0,    0.0,    0.0,    0.5,    0.5,    0.0,    0.0,    0.5,    0.5,
             1.0,   -1.0,   0.0,    0.0,    0.0,    0.5,    0.5,    0.0,    0.0,    0.5,    0.5,
            -1.0,    1.0,   0.0,    0.0,    0.0,    0.5,    0.5,    0.0,    0.0,    0.5,    0.5,
             1.0,    1.0,   0.0,    0.0,    0.0,    0.5,    0.5,    0.0,    0.0,    0.5,    0.5,
        };

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, double_quad, sizeof(float) * 11);
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffff00, "double texbem failed: Got color 0x%08x, expected 0x00ffff00.\n", color);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 2, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 3, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "Direct3DDevice9_SetPixelShader failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DPixelShader9_Release(pixel_shader);
    IDirect3DTexture9_Release(texture);
    IDirect3DTexture9_Release(texture1);
    IDirect3DTexture9_Release(texture2);
}

static void z_range_test(IDirect3DDevice9 *device)
{
    static const struct vertex quad[] =
    {
        {-1.0f, 0.0f,  1.1f, 0xffff0000},
        {-1.0f, 1.0f,  1.1f, 0xffff0000},
        { 1.0f, 0.0f, -1.1f, 0xffff0000},
        { 1.0f, 1.0f, -1.1f, 0xffff0000},
    };
    static const struct vertex quad2[] =
    {
        {-1.0f, 0.0f,  1.1f, 0xff0000ff},
        {-1.0f, 1.0f,  1.1f, 0xff0000ff},
        { 1.0f, 0.0f, -1.1f, 0xff0000ff},
        { 1.0f, 1.0f, -1.1f, 0xff0000ff},
    };
    static const struct tvertex quad3[] =
    {
        {  0.0f, 240.0f,  1.1f, 1.0f, 0xffffff00},
        {  0.0f, 480.0f,  1.1f, 1.0f, 0xffffff00},
        {640.0f, 240.0f, -1.1f, 1.0f, 0xffffff00},
        {640.0f, 480.0f, -1.1f, 1.0f, 0xffffff00},
    };
    static const struct tvertex quad4[] =
    {
        {  0.0f, 240.0f,  1.1f, 1.0f, 0xff00ff00},
        {  0.0f, 480.0f,  1.1f, 1.0f, 0xff00ff00},
        {640.0f, 240.0f, -1.1f, 1.0f, 0xff00ff00},
        {640.0f, 480.0f, -1.1f, 1.0f, 0xff00ff00},
    };
    HRESULT hr;
    DWORD color;
    IDirect3DVertexShader9 *shader;
    D3DCAPS9 caps;
    static const DWORD shader_code[] =
    {
        0xfffe0101,                                     /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position v0  */
        0x00000001, 0xc00f0000, 0x90e40000,             /* mov oPos, v0     */
        0x00000001, 0xd00f0000, 0xa0e40000,             /* mov oD0, c0      */
        0x0000ffff                                      /* end              */
    };
    static const float color_const_1[] = {1.0f, 0.0f, 0.0f, 1.0f};
    static const float color_const_2[] = {0.0f, 0.0f, 1.0f, 1.0f};

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    /* Does the Present clear the depth stencil? Clear the depth buffer with some value != 0,
     * then call Present. Then clear the color buffer to make sure it has some defined content
     * after the Present with D3DSWAPEFFECT_DISCARD. After that draw a plane that is somewhere cut
     * by the depth value. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.75f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z test, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z writes, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* Test the untransformed vertex path */
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    /* Test the transformed vertex path */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(quad4[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(quad3[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Test the shader path */
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1))
    {
        skip("Vertex shaders not supported\n");
        goto out;
    }
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, color_const_1, 1);
    ok(SUCCEEDED(hr), "Failed to set vs constant 0, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
    ok(SUCCEEDED(hr), "Failed to set z function, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, color_const_2, 1);
    ok(SUCCEEDED(hr), "Failed to set vs constant 0, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    IDirect3DVertexShader9_Release(shader);

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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

out:
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z test, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z writes, hr %#x.\n", hr);
}

static void fill_surface(IDirect3DSurface9 *surface, DWORD color, DWORD flags)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT l;
    HRESULT hr;
    unsigned int x, y;
    DWORD *mem;

    memset(&desc, 0, sizeof(desc));
    memset(&l, 0, sizeof(l));
    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(hr == D3D_OK, "IDirect3DSurface9_GetDesc failed with %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &l, NULL, flags);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect failed with %08x\n", hr);
    if(FAILED(hr)) return;

    for(y = 0; y < desc.Height; y++)
    {
        mem = (DWORD *) ((BYTE *) l.pBits + y * l.Pitch);
        for(x = 0; x < l.Pitch / sizeof(DWORD); x++)
        {
            mem[x] = color;
        }
    }
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect failed with %08x\n", hr);
}

/* This tests a variety of possible StretchRect() situations */
static void stretchrect_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *tex_rt32 = NULL, *tex_rt64 = NULL, *tex_rt_dest64 = NULL, *tex_rt_dest640_480 = NULL;
    IDirect3DSurface9 *surf_tex_rt32 = NULL, *surf_tex_rt64 = NULL, *surf_tex_rt_dest64 = NULL, *surf_tex_rt_dest640_480 = NULL;
    IDirect3DTexture9 *tex32 = NULL, *tex64 = NULL, *tex_dest64 = NULL;
    IDirect3DSurface9 *surf_tex32 = NULL, *surf_tex64 = NULL, *surf_tex_dest64 = NULL;
    IDirect3DSurface9 *surf_rt32 = NULL, *surf_rt64 = NULL, *surf_rt_dest64 = NULL;
    IDirect3DSurface9 *surf_offscreen32 = NULL, *surf_offscreen64 = NULL, *surf_offscreen_dest64 = NULL;
    IDirect3DSurface9 *surf_temp32 = NULL, *surf_temp64 = NULL;
    IDirect3DSurface9 *orig_rt = NULL;
    IDirect3DSurface9 *backbuffer = NULL;
    DWORD color;

    RECT src_rect64 = {0, 0, 64, 64};
    RECT src_rect64_flipy = {0, 64, 64, 0};
    RECT dst_rect64 = {0, 0, 64, 64};
    RECT dst_rect64_flipy = {0, 64, 64, 0};

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &orig_rt);
    ok(hr == D3D_OK, "Can't get render target, hr = %08x\n", hr);
    if(!orig_rt) {
        goto out;
    }

    /* Create our temporary surfaces in system memory */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf_temp32, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf_temp64, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);

    /* Create offscreen plain surfaces in D3DPOOL_DEFAULT */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf_offscreen32, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf_offscreen64, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf_offscreen_dest64, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);

    /* Create render target surfaces */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surf_rt32, NULL );
    ok(hr == D3D_OK, "Creating the render target surface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surf_rt64, NULL );
    ok(hr == D3D_OK, "Creating the render target surface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &surf_rt_dest64, NULL );
    ok(hr == D3D_OK, "Creating the render target surface failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);

    /* Create render target textures */
    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt32, NULL);
    ok(hr == D3D_OK, "Creating the render target texture failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt64, NULL);
    ok(hr == D3D_OK, "Creating the render target texture failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt_dest64, NULL);
    ok(hr == D3D_OK, "Creating the render target texture failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt_dest640_480, NULL);
    ok(hr == D3D_OK, "Creating the render target texture failed with %08x\n", hr);
    if (tex_rt32) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex_rt32, 0, &surf_tex_rt32);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }
    if (tex_rt64) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex_rt64, 0, &surf_tex_rt64);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }
    if (tex_rt_dest64) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex_rt_dest64, 0, &surf_tex_rt_dest64);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }
    if (tex_rt_dest640_480) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex_rt_dest640_480, 0, &surf_tex_rt_dest640_480);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }

    /* Create regular textures in D3DPOOL_DEFAULT */
    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex32, NULL);
    ok(hr == D3D_OK, "Creating the regular texture failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex64, NULL);
    ok(hr == D3D_OK, "Creating the regular texture failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_dest64, NULL);
    ok(hr == D3D_OK, "Creating the regular texture failed with %08x\n", hr);
    if (tex32) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex32, 0, &surf_tex32);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }
    if (tex64) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex64, 0, &surf_tex64);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }
    if (tex_dest64) {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex_dest64, 0, &surf_tex_dest64);
        ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);
    }

    /*********************************************************************
     * Tests for when the source parameter is an offscreen plain surface *
     *********************************************************************/

    /* Fill the offscreen 64x64 surface with green */
    if (surf_offscreen64)
        fill_surface(surf_offscreen64, 0xff00ff00, 0);

    /* offscreenplain ==> offscreenplain, same size */
    if(surf_offscreen64 && surf_offscreen_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, NULL, surf_offscreen_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_offscreen_dest64, 32, 32);
            ok(color == 0xff00ff00, "StretchRect offscreen ==> offscreen same size failed: Got color 0x%08x, expected 0xff00ff00.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_offscreen_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64_flipy, surf_offscreen_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_offscreen_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* offscreenplain ==> rendertarget texture, same size */
    if(surf_offscreen64 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 32, 32);
            ok(color == 0xff00ff00, "StretchRect offscreen ==> rendertarget texture same size failed: Got color 0x%08x, expected 0xff00ff00.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64_flipy, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_tex_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* offscreenplain ==> rendertarget surface, same size */
    if(surf_offscreen64 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_rt_dest64, 32, 32);
            ok(color == 0xff00ff00, "StretchRect offscreen ==> rendertarget surface same size failed: Got color 0x%08x, expected 0xff00ff00.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64_flipy, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, &src_rect64, surf_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* offscreenplain ==> texture, same size (should fail) */
    if(surf_offscreen64 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen64, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* Fill the smaller offscreen surface with red */
    fill_surface(surf_offscreen32, 0xffff0000, 0);

    /* offscreenplain ==> offscreenplain, scaling (should fail) */
    if(surf_offscreen32 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen32, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* offscreenplain ==> rendertarget texture, scaling */
    if(surf_offscreen32 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen32, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 48, 48);
            ok(color == 0xffff0000, "StretchRect offscreen ==> rendertarget texture same size failed: Got color 0x%08x, expected 0xffff0000.\n", color);
        }
    }

    /* offscreenplain ==> rendertarget surface, scaling */
    if(surf_offscreen32 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen32, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        color = getPixelColorFromSurface(surf_rt_dest64, 48, 48);
        ok(color == 0xffff0000, "StretchRect offscreen ==> rendertarget surface scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
    }

    /* offscreenplain ==> texture, scaling (should fail) */
    if(surf_offscreen32 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_offscreen32, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /************************************************************
     * Tests for when the source parameter is a regular texture *
     ************************************************************/

    /* Fill the surface of the regular texture with blue */
    if (surf_tex64 && surf_temp64) {
        /* Can't fill the surf_tex directly because it's created in D3DPOOL_DEFAULT */
        fill_surface(surf_temp64, 0xff0000ff, 0);
        hr = IDirect3DDevice9_UpdateSurface(device, surf_temp64, NULL, surf_tex64, NULL);
        ok( hr == D3D_OK, "IDirect3DDevice9_UpdateSurface failed with %08x\n", hr);
    }

    /* texture ==> offscreenplain, same size */
    if(surf_tex64 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* texture ==> rendertarget texture, same size */
    if(surf_tex64 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 32, 32);
            ok(color == 0xff0000ff, "StretchRect texture ==> rendertarget texture same size failed: Got color 0x%08x, expected 0xff0000ff.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64_flipy, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64, surf_tex_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* texture ==> rendertarget surface, same size */
    if(surf_tex64 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_rt_dest64, 32, 32);
            ok(color == 0xff0000ff, "StretchRect texture ==> rendertarget surface same size failed: Got color 0x%08x, expected 0xff0000ff.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64_flipy, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, &src_rect64, surf_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* texture ==> texture, same size (should fail) */
    if(surf_tex64 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex64, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* Fill the surface of the smaller regular texture with red */
    if (surf_tex32 && surf_temp32) {
        /* Can't fill the surf_tex directly because it's created in D3DPOOL_DEFAULT */
        fill_surface(surf_temp32, 0xffff0000, 0);
        hr = IDirect3DDevice9_UpdateSurface(device, surf_temp32, NULL, surf_tex32, NULL);
        ok( hr == D3D_OK, "IDirect3DDevice9_UpdateSurface failed with %08x\n", hr);
    }

    /* texture ==> offscreenplain, scaling (should fail) */
    if(surf_tex32 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex32, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* texture ==> rendertarget texture, scaling */
    if(surf_tex32 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex32, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 48, 48);
            ok(color == 0xffff0000, "StretchRect texture ==> rendertarget texture scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
        }
    }

    /* texture ==> rendertarget surface, scaling */
    if(surf_tex32 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex32, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        color = getPixelColorFromSurface(surf_rt_dest64, 48, 48);
        ok(color == 0xffff0000, "StretchRect texture ==> rendertarget surface scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
    }

    /* texture ==> texture, scaling (should fail) */
    if(surf_tex32 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex32, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /*****************************************************************
     * Tests for when the source parameter is a rendertarget texture *
     *****************************************************************/

    /* Fill the surface of the rendertarget texture with white */
    if (surf_tex_rt64 && surf_temp64) {
        /* Can't fill the surf_tex_rt directly because it's created in D3DPOOL_DEFAULT */
        fill_surface(surf_temp64, 0xffffffff, 0);
        hr = IDirect3DDevice9_UpdateSurface(device, surf_temp64, NULL, surf_tex_rt64, NULL);
        ok( hr == D3D_OK, "IDirect3DDevice9_UpdateSurface failed with %08x\n", hr);
    }

    /* rendertarget texture ==> offscreenplain, same size */
    if(surf_tex_rt64 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* rendertarget texture ==> rendertarget texture, same size */
    if(surf_tex_rt64 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 32, 32);
            ok(color == 0xffffffff, "StretchRect rendertarget texture ==> rendertarget texture same size failed: Got color 0x%08x, expected 0xffffffff.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64_flipy, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64, surf_tex_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* rendertarget texture ==> rendertarget surface, same size */
    if(surf_tex_rt64 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_rt_dest64, 32, 32);
            ok(color == 0xffffffff, "StretchRect rendertarget texture ==> rendertarget surface same size failed: Got color 0x%08x, expected 0xffffffff.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64_flipy, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, &src_rect64, surf_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* rendertarget texture ==> texture, same size (should fail) */
    if(surf_tex_rt64 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt64, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* Fill the surface of the smaller rendertarget texture with red */
    if (surf_tex_rt32 && surf_temp32) {
        /* Can't fill the surf_tex_rt directly because it's created in D3DPOOL_DEFAULT */
        fill_surface(surf_temp32, 0xffff0000, 0);
        hr = IDirect3DDevice9_UpdateSurface(device, surf_temp32, NULL, surf_tex_rt32, NULL);
        ok( hr == D3D_OK, "IDirect3DDevice9_UpdateSurface failed with %08x\n", hr);
    }

    /* rendertarget texture ==> offscreenplain, scaling (should fail) */
    if(surf_tex_rt32 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt32, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* rendertarget texture ==> rendertarget texture, scaling */
    if(surf_tex_rt32 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt32, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 48, 48);
            ok(color == 0xffff0000, "StretchRect rendertarget texture ==> rendertarget texture scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
        }
    }

    /* rendertarget texture ==> rendertarget surface, scaling */
    if(surf_tex_rt32 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt32, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        color = getPixelColorFromSurface(surf_rt_dest64, 48, 48);
        ok(color == 0xffff0000, "StretchRect rendertarget texture ==> rendertarget surface scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
    }

    /* rendertarget texture ==> texture, scaling (should fail) */
    if(surf_tex_rt32 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_tex_rt32, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /*****************************************************************
     * Tests for when the source parameter is a rendertarget surface *
     *****************************************************************/

    /* Fill the surface of the rendertarget surface with black */
    if (surf_rt64)
        fill_surface(surf_rt64, 0xff000000, 0);

    /* rendertarget texture ==> offscreenplain, same size */
    if(surf_rt64 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* rendertarget surface ==> rendertarget texture, same size */
    if(surf_rt64 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 32, 32);
            ok(color == 0xff000000, "StretchRect rendertarget surface ==> rendertarget texture same size failed: Got color 0x%08x, expected 0xff000000.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64_flipy, surf_tex_rt_dest64, &dst_rect64, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64, surf_tex_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* rendertarget surface ==> rendertarget surface, same size */
    if(surf_rt64 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_rt_dest64, 32, 32);
            ok(color == 0xff000000, "StretchRect rendertarget surface ==> rendertarget surface same size failed: Got color 0x%08x, expected 0xff000000.\n", color);
        }

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64, surf_rt_dest64, &dst_rect64, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64_flipy, surf_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, &src_rect64, surf_rt_dest64, &dst_rect64_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* rendertarget surface ==> texture, same size (should fail) */
    if(surf_rt64 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt64, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* Fill the surface of the smaller rendertarget texture with red */
    if (surf_rt32)
        fill_surface(surf_rt32, 0xffff0000, 0);

    /* rendertarget surface ==> offscreenplain, scaling (should fail) */
    if(surf_rt32 && surf_offscreen64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt32, NULL, surf_offscreen64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* rendertarget surface ==> rendertarget texture, scaling */
    if(surf_rt32 && surf_tex_rt_dest64 && surf_temp64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt32, NULL, surf_tex_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* We can't lock rendertarget textures, so copy to our temp surface first */
        if (hr == D3D_OK) {
            hr = IDirect3DDevice9_GetRenderTargetData(device, surf_tex_rt_dest64, surf_temp64);
            ok( hr == D3D_OK, "IDirect3DDevice9_GetRenderTargetData failed with %08x\n", hr);
        }

        if (hr == D3D_OK) {
            color = getPixelColorFromSurface(surf_temp64, 48, 48);
            ok(color == 0xffff0000, "StretchRect rendertarget surface ==> rendertarget texture scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
        }
    }

    /* rendertarget surface ==> rendertarget surface, scaling */
    if(surf_rt32 && surf_rt_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt32, NULL, surf_rt_dest64, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        color = getPixelColorFromSurface(surf_rt_dest64, 48, 48);
        ok(color == 0xffff0000, "StretchRect rendertarget surface ==> rendertarget surface scaling failed: Got color 0x%08x, expected 0xffff0000.\n", color);
    }

    /* rendertarget surface ==> texture, scaling (should fail) */
    if(surf_rt32 && surf_tex_dest64) {
        hr = IDirect3DDevice9_StretchRect(device, surf_rt32, NULL, surf_tex_dest64, NULL, 0);
        todo_wine ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");
    }

    /* backbuffer ==> surface tests (no scaling) */
    if(backbuffer && surf_tex_rt_dest640_480)
    {
        RECT src_rect = {0, 0, 640, 480};
        RECT src_rect_flipy = {0, 480, 640, 0};
        RECT dst_rect = {0, 0, 640, 480};
        RECT dst_rect_flipy = {0, 480, 640, 0};

        /* Blit with NULL rectangles */
        hr = IDirect3DDevice9_StretchRect(device, backbuffer, NULL, surf_tex_rt_dest640_480, NULL, 0);
        ok( hr == D3D_OK, "StretchRect backbuffer ==> texture same size failed:\n");

        /* Blit without scaling */
        hr = IDirect3DDevice9_StretchRect(device, backbuffer, &src_rect, surf_tex_rt_dest640_480, &dst_rect, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect succeeded, shouldn't happen (todo)\n");

        /* Flipping in y-direction through src_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, backbuffer, &src_rect_flipy, surf_tex_rt_dest640_480, &dst_rect, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);

        /* Flipping in y-direction through dst_rect, no scaling (not allowed) */
        hr = IDirect3DDevice9_StretchRect(device, backbuffer, &src_rect, surf_tex_rt_dest640_480, &dst_rect_flipy, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }

    /* TODO: Test format conversions */


out:
    /* Clean up */
    if (backbuffer)
        IDirect3DSurface9_Release(backbuffer);
    if (surf_rt32)
        IDirect3DSurface9_Release(surf_rt32);
    if (surf_rt64)
        IDirect3DSurface9_Release(surf_rt64);
    if (surf_rt_dest64)
        IDirect3DSurface9_Release(surf_rt_dest64);
    if (surf_temp32)
        IDirect3DSurface9_Release(surf_temp32);
    if (surf_temp64)
        IDirect3DSurface9_Release(surf_temp64);
    if (surf_offscreen32)
        IDirect3DSurface9_Release(surf_offscreen32);
    if (surf_offscreen64)
        IDirect3DSurface9_Release(surf_offscreen64);
    if (surf_offscreen_dest64)
        IDirect3DSurface9_Release(surf_offscreen_dest64);

    if (tex_rt32) {
        if (surf_tex_rt32)
            IDirect3DSurface9_Release(surf_tex_rt32);
        IDirect3DTexture9_Release(tex_rt32);
    }
    if (tex_rt64) {
        if (surf_tex_rt64)
            IDirect3DSurface9_Release(surf_tex_rt64);
        IDirect3DTexture9_Release(tex_rt64);
    }
    if (tex_rt_dest64) {
        if (surf_tex_rt_dest64)
            IDirect3DSurface9_Release(surf_tex_rt_dest64);
        IDirect3DTexture9_Release(tex_rt_dest64);
    }
    if (tex_rt_dest640_480) {
        if (surf_tex_rt_dest640_480)
            IDirect3DSurface9_Release(surf_tex_rt_dest640_480);
        IDirect3DTexture9_Release(tex_rt_dest640_480);
    }
    if (tex32) {
        if (surf_tex32)
            IDirect3DSurface9_Release(surf_tex32);
        IDirect3DTexture9_Release(tex32);
    }
    if (tex64) {
        if (surf_tex64)
            IDirect3DSurface9_Release(surf_tex64);
        IDirect3DTexture9_Release(tex64);
    }
    if (tex_dest64) {
        if (surf_tex_dest64)
            IDirect3DSurface9_Release(surf_tex_dest64);
        IDirect3DTexture9_Release(tex_dest64);
    }

    if (orig_rt) {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, orig_rt);
        ok(hr == D3D_OK, "IDirect3DSetRenderTarget failed with %08x\n", hr);
        IDirect3DSurface9_Release(orig_rt);
    }
}

static void maxmip_test(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    HRESULT hr;
    DWORD color;
    static const struct
    {
        struct
        {
            float x, y, z;
            float s, t;
        }
        v[4];
    }
    quads[] =
    {
        {{
            {-1.0, -1.0,  0.0,  0.0,  0.0},
            {-1.0,  0.0,  0.0,  0.0,  1.0},
            { 0.0, -1.0,  0.0,  1.0,  0.0},
            { 0.0,  0.0,  0.0,  1.0,  1.0},
        }},
        {{
            { 0.0, -1.0,  0.0,  0.0,  0.0},
            { 0.0,  0.0,  0.0,  0.0,  1.0},
            { 1.0, -1.0,  0.0,  1.0,  0.0},
            { 1.0,  0.0,  0.0,  1.0,  1.0},
        }},
        {{
            { 0.0,  0.0,  0.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0,  0.0,  1.0},
            { 1.0,  0.0,  0.0,  1.0,  0.0},
            { 1.0,  1.0,  0.0,  1.0,  1.0},
        }},
        {{
            {-1.0,  0.0,  0.0,  0.0,  0.0},
            {-1.0,  1.0,  0.0,  0.0,  1.0},
            { 0.0,  0.0,  0.0,  1.0,  0.0},
            { 0.0,  1.0,  0.0,  1.0,  1.0},
        }},
    };

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 3, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                        &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);
    if(!texture)
    {
        skip("Failed to create test texture\n");
        return;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "IDirect3DTexture9_GetSurfaceLevel returned %#x.\n", hr);
    fill_surface(surface, 0xffff0000, 0);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 1, &surface);
    ok(SUCCEEDED(hr), "IDirect3DTexture9_GetSurfaceLevel returned %#x.\n", hr);
    fill_surface(surface, 0xff00ff00, 0);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 2, &surface);
    ok(SUCCEEDED(hr), "IDirect3DTexture9_GetSurfaceLevel returned %#x.\n", hr);
    fill_surface(surface, 0xff0000ff, 0);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[1], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[2], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[3], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (%08x)\n", hr);
    }

    /* With mipmapping disabled, the max mip level is ignored, only level 0 is used */
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "MaxMip 0, no mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ff0000, "MaxMip 1, no mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff0000, "MaxMip 2, no mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00ff0000, "MaxMip 3, no mipfilter has color 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[1], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[2], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[3], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene returned %#x.\n", hr);
    }

    /* Max Mip level 0-2 sample from the specified texture level, Max Mip
     * level 3 (> levels in texture) samples from the highest level in the
     * texture (level 2). */
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "MaxMip 0, point mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000ff00, "MaxMip 1, point mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "MaxMip 2, point mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x000000ff, "MaxMip 3, point mipfilter has color 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr))
    {
        DWORD ret;

        /* Mipmapping OFF, LOD level smaller than MAXMIPLEVEL. LOD level limits */
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        ret = IDirect3DTexture9_SetLOD(texture, 1);
        ok(ret == 0, "IDirect3DTexture9_SetLOD returned %u, expected 0\n", ret);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Mipmapping ON, LOD level smaller than max mip level. LOD level limits */
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        ret = IDirect3DTexture9_SetLOD(texture, 2);
        ok(ret == 1, "IDirect3DTexture9_SetLOD returned %u, expected 1\n", ret);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[1], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Mipmapping ON, LOD level bigger than max mip level. MAXMIPLEVEL limits */
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        ret = IDirect3DTexture9_SetLOD(texture, 1);
        ok(ret == 2, "IDirect3DTexture9_SetLOD returned %u, expected 2\n", ret);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[2], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Mipmapping OFF, LOD level bigger than max mip level. LOD level limits */
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
        ret = IDirect3DTexture9_SetLOD(texture, 1);
        ok(ret == 1, "IDirect3DTexture9_SetLOD returned %u, expected 1\n", ret);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[3], sizeof(*quads->v));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);
    }

    /* Max Mip level 0-2 sample from the specified texture level, Max Mip
     * level 3 (> levels in texture) samples from the highest level in the
     * texture (level 2). */
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000ff00, "MaxMip 0, LOD 1, none mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x000000ff, "MaxMip 1, LOD 2, point mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "MaxMip 2, LOD 1, point mipfilter has color 0x%08x.\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000ff00, "MaxMip 2, LOD 1, none mipfilter has color 0x%08x.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
    IDirect3DTexture9_Release(texture);
}

static void release_buffer_test(IDirect3DDevice9 *device)
{
    IDirect3DVertexBuffer9 *vb = NULL;
    IDirect3DIndexBuffer9 *ib = NULL;
    HRESULT hr;
    BYTE *data;
    LONG ref;

    static const struct vertex quad[] = {
        {-1.0,      -1.0,       0.1,        0xffff0000},
        {-1.0,       1.0,       0.1,        0xffff0000},
        { 1.0,       1.0,       0.1,        0xffff0000},

        {-1.0,      -1.0,       0.1,        0xff00ff00},
        {-1.0,       1.0,       0.1,        0xff00ff00},
        { 1.0,       1.0,       0.1,        0xff00ff00}
    };
    short indices[] = {3, 4, 5};

    /* Index and vertex buffers should always be creatable */
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, D3DFVF_XYZ | D3DFVF_DIFFUSE,
                                              D3DPOOL_MANAGED, &vb, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if(!vb) {
        skip("Failed to create a vertex buffer\n");
        return;
    }
    hr = IDirect3DDevice9_CreateIndexBuffer(device, sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateIndexBuffer failed with %08x\n", hr);
    if(!ib) {
        skip("Failed to create an index buffer\n");
        return;
    }

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);

    hr = IDirect3DIndexBuffer9_Lock(ib, 0, sizeof(indices), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(ib);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Unlock failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetIndices(device, ib);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetIndices failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad[0]));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

    /* Now destroy the bound index buffer and draw again */
    ref = IDirect3DIndexBuffer9_Release(ib);
    ok(ref == 0, "Index Buffer reference count is %08d\n", ref);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        /* Deliberately using minvertexindex = 0 and numVertices = 6 to prevent d3d from
         * making assumptions about the indices or vertices
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 3, 3, 0, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitive failed with %08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetIndices(device, NULL);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Unlock failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Unlock failed with %08x\n", hr);

    /* Index buffer was already destroyed as part of the test */
    IDirect3DVertexBuffer9_Release(vb);
}

static void float_texture_test(IDirect3DDevice9 *device)
{
    IDirect3D9 *d3d = NULL;
    HRESULT hr;
    IDirect3DTexture9 *texture = NULL;
    D3DLOCKED_RECT lr;
    float *data;
    DWORD color;
    float quad[] = {
        -1.0,      -1.0,       0.1,     0.0,    0.0,
        -1.0,       1.0,       0.1,     0.0,    1.0,
         1.0,      -1.0,       0.1,     1.0,    0.0,
         1.0,       1.0,       0.1,     1.0,    1.0,
    };

    memset(&lr, 0, sizeof(lr));
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                                     D3DRTYPE_TEXTURE, D3DFMT_R32F) != D3D_OK) {
        skip("D3DFMT_R32F textures not supported\n");
        goto out;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_R32F,
                                        D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);
    if(!texture) {
        skip("Failed to create R32F texture\n");
        goto out;
    }

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed with %08x\n", hr);
    data = lr.pBits;
    *data = 0.0;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

    color = getPixelColor(device, 240, 320);
    ok(color == 0x0000ffff, "R32F with value 0.0 has color %08x, expected 0x0000ffff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

out:
    if(texture) IDirect3DTexture9_Release(texture);
    IDirect3D9_Release(d3d);
}

static void g16r16_texture_test(IDirect3DDevice9 *device)
{
    IDirect3D9 *d3d = NULL;
    HRESULT hr;
    IDirect3DTexture9 *texture = NULL;
    D3DLOCKED_RECT lr;
    DWORD *data;
    DWORD color;
    float quad[] = {
       -1.0,      -1.0,       0.1,     0.0,    0.0,
       -1.0,       1.0,       0.1,     0.0,    1.0,
        1.0,      -1.0,       0.1,     1.0,    0.0,
        1.0,       1.0,       0.1,     1.0,    1.0,
    };

    memset(&lr, 0, sizeof(lr));
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
       D3DRTYPE_TEXTURE, D3DFMT_G16R16) != D3D_OK) {
           skip("D3DFMT_G16R16 textures not supported\n");
           goto out;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_G16R16,
                                        D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);
    if(!texture) {
        skip("Failed to create D3DFMT_G16R16 texture\n");
        goto out;
    }

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed with %08x\n", hr);
    data = lr.pBits;
    *data = 0x0f00f000;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

    color = getPixelColor(device, 240, 320);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xf0, 0x0f, 0xff), 1),
       "D3DFMT_G16R16 with value 0x00ffff00 has color %08x, expected 0x00f00fff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

out:
    if(texture) IDirect3DTexture9_Release(texture);
    IDirect3D9_Release(d3d);
}

static void check_rect(IDirect3DDevice9 *device, RECT r, const char *message)
{
    LONG x_coords[2][2] =
    {
        {r.left - 1, r.left + 1},
        {r.right + 1, r.right - 1},
    };
    LONG y_coords[2][2] =
    {
        {r.top - 1, r.top + 1},
        {r.bottom + 1, r.bottom - 1}
    };
    unsigned int i, j, x_side, y_side;

    for (i = 0; i < 2; ++i)
    {
        for (j = 0; j < 2; ++j)
        {
            for (x_side = 0; x_side < 2; ++x_side)
            {
                for (y_side = 0; y_side < 2; ++y_side)
                {
                    unsigned int x = x_coords[i][x_side], y = y_coords[j][y_side];
                    DWORD color;
                    DWORD expected = (x_side == 1 && y_side == 1) ? 0x00ffffff : 0;

                    color = getPixelColor(device, x, y);
                    ok(color == expected, "%s: Pixel (%d, %d) has color %08x, expected %08x\n",
                            message, x, y, color, expected);
                }
            }
        }
    }
}

struct projected_textures_test_run
{
    const char *message;
    DWORD flags;
    IDirect3DVertexDeclaration9 *decl;
    BOOL vs, ps;
    RECT rect;
};

static void projected_textures_test(IDirect3DDevice9 *device,
        struct projected_textures_test_run tests[4])
{
    unsigned int i;

    static const DWORD vertex_shader[] =
    {
        0xfffe0101,                                     /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position v0  */
        0x0000001f, 0x80000005, 0x900f0001,             /* dcl_texcoord0 v1 */
        0x00000001, 0xc00f0000, 0x90e40000,             /* mov oPos, v0     */
        0x00000001, 0xe00f0000, 0x90e40001,             /* mov oT0, v1      */
        0x0000ffff                                      /* end              */
    };
    static const DWORD pixel_shader[] =
    {
        0xffff0103,                                     /* ps_1_3           */
        0x00000042, 0xb00f0000,                         /* tex t0           */
        0x00000001, 0x800f0000, 0xb0e40000,             /* mov r0, t0       */
        0x0000ffff                                      /* end              */
    };
    IDirect3DVertexShader9 *vs = NULL;
    IDirect3DPixelShader9 *ps = NULL;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HRESULT hr;

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed (%08x)\n", hr);
    IDirect3D9_Release(d3d);

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader, &vs);
        ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    }
    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 3))
    {
        hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader, &ps);
        ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff203040, 0.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if (FAILED(hr))
        return;

    for (i = 0; i < 4; ++i)
    {
        DWORD value = 0xdeadbeef;
        static const float proj_quads[] =
        {
            -1.0,   -1.0,    0.1,    0.0,    0.0,    4.0,    6.0,
             0.0,   -1.0,    0.1,    4.0,    0.0,    4.0,    6.0,
            -1.0,    0.0,    0.1,    0.0,    4.0,    4.0,    6.0,
             0.0,    0.0,    0.1,    4.0,    4.0,    4.0,    6.0,

             0.0,   -1.0,    0.1,    0.0,    0.0,    4.0,    6.0,
             1.0,   -1.0,    0.1,    4.0,    0.0,    4.0,    6.0,
             0.0,    0.0,    0.1,    0.0,    4.0,    4.0,    6.0,
             1.0,    0.0,    0.1,    4.0,    4.0,    4.0,    6.0,

            -1.0,    0.0,    0.1,    0.0,    0.0,    4.0,    6.0,
             0.0,    0.0,    0.1,    4.0,    0.0,    4.0,    6.0,
            -1.0,    1.0,    0.1,    0.0,    4.0,    4.0,    6.0,
             0.0,    1.0,    0.1,    4.0,    4.0,    4.0,    6.0,

             0.0,    0.0,    0.1,    0.0,    0.0,    4.0,    6.0,
             1.0,    0.0,    0.1,    4.0,    0.0,    4.0,    6.0,
             0.0,    1.0,    0.1,    0.0,    4.0,    4.0,    6.0,
             1.0,    1.0,    0.1,    4.0,    4.0,    4.0,    6.0,
        };

        if (tests[i].vs)
        {
            if (!vs)
            {
                skip("Vertex shaders not supported, skipping\n");
                continue;
            }
            hr = IDirect3DDevice9_SetVertexShader(device, vs);
        }
        else
            hr = IDirect3DDevice9_SetVertexShader(device, NULL);
        ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
        if (tests[i].ps)
        {
            if (!ps)
            {
                skip("Pixel shaders not supported, skipping\n");
                continue;
            }
            hr = IDirect3DDevice9_SetPixelShader(device, ps);
        }
        else
            hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexDeclaration(device, tests[i].decl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, tests[i].flags);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, &value);
        ok(SUCCEEDED(hr) && value == tests[i].flags,
                "GetTextureStageState returned: hr %08x, value %08x.\n", hr, value);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,
                &proj_quads[i * 4 * 7], 7 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
    }

    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    if (vs) IDirect3DVertexShader9_Release(vs);
    if (ps) IDirect3DPixelShader9_Release(ps);

    for (i = 0; i < 4; ++i)
    {
        if ((!tests[i].vs || vs) && (!tests[i].ps || ps))
            check_rect(device, tests[i].rect, tests[i].message);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);
}

static void texture_transform_flags_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3D9 *d3d;
    D3DFORMAT fmt = D3DFMT_X8R8G8B8;
    D3DCAPS9 caps;
    IDirect3DTexture9 *texture = NULL;
    IDirect3DVolumeTexture9 *volume = NULL;
    unsigned int x, y, z;
    D3DLOCKED_RECT lr;
    D3DLOCKED_BOX lb;
    DWORD color;
    UINT w, h;
    IDirect3DVertexDeclaration9 *decl, *decl2, *decl3, *decl4;
    float identity[16] = {1.0, 0.0, 0.0, 0.0,
                           0.0, 1.0, 0.0, 0.0,
                           0.0, 0.0, 1.0, 0.0,
                           0.0, 0.0, 0.0, 1.0};
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements2[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements3[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements4[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };
    static const unsigned char proj_texdata[] = {0x00, 0x00, 0x00, 0x00,
                                                 0x00, 0xff, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00};

    memset(&lr, 0, sizeof(lr));
    memset(&lb, 0, sizeof(lb));
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                                     D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16) == D3D_OK) {
        fmt = D3DFMT_A16B16G16R16;
    }
    IDirect3D9_Release(d3d);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements2, &decl2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements3, &decl3);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements4, &decl4);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_SRGBTEXTURE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_SRGBTEXTURE) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MAGFILTER) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MINFILTER) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MIPFILTER) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSU) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSV) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSW) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState(D3DTSS_COLOROP) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState(D3DTSS_COLORARG1) returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState(D3DRS_LIGHTING) returned %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps returned %08x\n", hr);
    w = min(1024, caps.MaxTextureWidth);
    h = min(1024, caps.MaxTextureHeight);
    hr = IDirect3DDevice9_CreateTexture(device, w, h, 1,
                                        0, fmt, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %08x\n", hr);
    if(!texture) {
        skip("Failed to create the test texture\n");
        return;
    }

    /* Unfortunately there is no easy way to set up a texture coordinate passthrough
     * in d3d fixed function pipeline, so create a texture that has a gradient from 0.0 to
     * 1.0 in red and green for the x and y coords
     */
    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %08x\n", hr);
    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            double r_f = (double) y / (double) h;
            double g_f = (double) x / (double) w;
            if(fmt == D3DFMT_A16B16G16R16) {
                unsigned short r, g;
                unsigned short *dst = (unsigned short *) (((char *) lr.pBits) + y * lr.Pitch + x * 8);
                r = (unsigned short) (r_f * 65536.0);
                g = (unsigned short) (g_f * 65536.0);
                dst[0] = r;
                dst[1] = g;
                dst[2] = 0;
                dst[3] = 65535;
            } else {
                unsigned char *dst = ((unsigned char *) lr.pBits) + y * lr.Pitch + x * 4;
                unsigned char r = (unsigned char) (r_f * 255.0);
                unsigned char g = (unsigned char) (g_f * 255.0);
                dst[0] = 0;
                dst[1] = g;
                dst[2] = r;
                dst[3] = 255;
            }
        }
    }
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        float quad1[] = {
            -1.0,      -1.0,       0.1,     1.0,    1.0,
            -1.0,       0.0,       0.1,     1.0,    1.0,
             0.0,      -1.0,       0.1,     1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,
        };
        float quad2[] = {
            -1.0,       0.0,       0.1,     1.0,    1.0,
            -1.0,       1.0,       0.1,     1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,
             0.0,       1.0,       0.1,     1.0,    1.0,
        };
        float quad3[] = {
             0.0,       0.0,       0.1,     0.5,    0.5,
             0.0,       1.0,       0.1,     0.5,    0.5,
             1.0,       0.0,       0.1,     0.5,    0.5,
             1.0,       1.0,       0.1,     0.5,    0.5,
        };
        float quad4[] = {
             320,       480,       0.1,     1.0,    0.0,    1.0,
             320,       240,       0.1,     1.0,    0.0,    1.0,
             640,       480,       0.1,     1.0,    0.0,    1.0,
             640,       240,       0.1,     1.0,    0.0,    1.0,
        };
        float mat[16] = {0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0};

        /* What happens with the texture matrix if D3DTSS_TEXTURETRANSFORMFLAGS is disabled? */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens with transforms enabled? */
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens if 4 coords are used, but only 2 given ?*/
        mat[8] = 1.0;
        mat[13] = 1.0;
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens with transformed geometry? This setup lead to 0/0 coords with untransformed
         * geometry. If the same applies to transformed vertices, the quad will be black, otherwise red,
         * due to the coords in the vertices. (turns out red, indeed)
         */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_TEX1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00ffff00, 1), "quad 1 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x0000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x0000ff00, 1), "quad 3 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, 0x00ff0000, 1), "quad 4 has color %08x, expected 0x00ff0000\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        float quad1[] = {
            -1.0,      -1.0,       0.1,     0.8,    0.2,
            -1.0,       0.0,       0.1,     0.8,    0.2,
             0.0,      -1.0,       0.1,     0.8,    0.2,
             0.0,       0.0,       0.1,     0.8,    0.2,
        };
        float quad2[] = {
            -1.0,       0.0,       0.1,     0.5,    1.0,
            -1.0,       1.0,       0.1,     0.5,    1.0,
             0.0,       0.0,       0.1,     0.5,    1.0,
             0.0,       1.0,       0.1,     0.5,    1.0,
        };
        float quad3[] = {
             0.0,       0.0,       0.1,     0.5,    1.0,
             0.0,       1.0,       0.1,     0.5,    1.0,
             1.0,       0.0,       0.1,     0.5,    1.0,
             1.0,       1.0,       0.1,     0.5,    1.0,
        };
        float quad4[] = {
             0.0,      -1.0,       0.1,     0.8,    0.2,
             0.0,       0.0,       0.1,     0.8,    0.2,
             1.0,      -1.0,       0.1,     0.8,    0.2,
             1.0,       0.0,       0.1,     0.8,    0.2,
        };
        float mat[16] = {0.0, 0.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0,
                          0.0, 1.0, 0.0, 0.0,
                          0.0, 0.0, 0.0, 0.0};

        /* What happens to the default 1 in the 3rd coordinate if it is disabled? */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* D3DTFF_COUNT1 does not work on Nvidia drivers. It behaves like D3DTTFF_DISABLE. On ATI drivers
         * it behaves like COUNT2 because normal textures require 2 coords. */
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Just to be sure, the same as quad2 above */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Now, what happens to the 2nd coordinate(that is disabled in the matrix) if it is not
         * used? And what happens to the first? */
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }
    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00ff0000, 1), "quad 1 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x0000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x00ff8000, 1) || color == 0x00000000,
       "quad 3 has color %08x, expected 0x00ff8000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, 0x0033cc00, 1) || color_match(color, 0x00ff0000, 1),
       "quad 4 has color %08x, expected 0x0033cc00\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DTexture9_Release(texture);

    /* Test projected textures, without any fancy matrices */
    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %08x\n", hr);
    if (SUCCEEDED(hr))
    {
        struct projected_textures_test_run projected_tests_1[4] =
        {
            {
                "D3DTTFF_COUNT4 | D3DTTFF_PROJECTED - bottom left",
                D3DTTFF_COUNT4 | D3DTTFF_PROJECTED,
                decl3,
                FALSE, TRUE,
                {120, 300, 240, 390},
            },
            {
                "D3DTTFF_COUNT3 | D3DTTFF_PROJECTED - bottom right",
                D3DTTFF_COUNT3 | D3DTTFF_PROJECTED,
                decl3,
                FALSE, TRUE,
                {400, 360, 480, 420},
            },
            /* Try with some invalid values */
            {
                "0xffffffff (draws like COUNT4 | PROJECTED) - top left",
                0xffffffff,
                decl3,
                FALSE, TRUE,
                {120, 60, 240, 150}
            },
            {
                "D3DTTFF_COUNT3 | D3DTTFF_PROJECTED (draws non-projected) - top right",
                D3DTTFF_COUNT3 | D3DTTFF_PROJECTED,
                decl4,
                FALSE, TRUE,
                {340, 210, 360, 225},
            }
        };
        struct projected_textures_test_run projected_tests_2[4] =
        {
            {
                "D3DTTFF_PROJECTED (like COUNT4 | PROJECTED, texcoord has 4 components) - bottom left",
                D3DTTFF_PROJECTED,
                decl3,
                FALSE, TRUE,
                {120, 300, 240, 390},
            },
            {
                "D3DTTFF_PROJECTED (like COUNT3 | PROJECTED, texcoord has only 3 components) - bottom right",
                D3DTTFF_PROJECTED,
                decl,
                FALSE, TRUE,
                {400, 360, 480, 420},
            },
            {
                "0xffffffff (like COUNT3 | PROJECTED, texcoord has only 3 components) - top left",
                0xffffffff,
                decl,
                FALSE, TRUE,
                {80, 120, 160, 180},
            },
            {
                "D3DTTFF_COUNT1 (draws non-projected) - top right",
                D3DTTFF_COUNT1,
                decl4,
                FALSE, TRUE,
                {340, 210, 360, 225},
            }
        };
        struct projected_textures_test_run projected_tests_3[4] =
        {
            {
                "D3DTTFF_COUNT3 | D3DTTFF_PROJECTED (like COUNT4 | PROJECTED) - bottom left",
                D3DTTFF_PROJECTED,
                decl3,
                TRUE, FALSE,
                {120, 300, 240, 390},
            },
            {
                "D3DTTFF_COUNT3 | D3DTTFF_PROJECTED (like COUNT4 | PROJECTED) - bottom right",
                D3DTTFF_COUNT3 | D3DTTFF_PROJECTED,
                decl3,
                TRUE, TRUE,
                {440, 300, 560, 390},
            },
            {
                "0xffffffff (like COUNT4 | PROJECTED) - top left",
                0xffffffff,
                decl3,
                TRUE, TRUE,
                {120, 60, 240, 150},
            },
            {
                "D3DTTFF_PROJECTED (like COUNT4 | PROJECTED) - top right",
                D3DTTFF_PROJECTED,
                decl3,
                FALSE, FALSE,
                {440, 60, 560, 150},
            },
        };

        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &identity);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);

        hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
        ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed with %08x\n", hr);
        for(x = 0; x < 4; x++) {
            memcpy(((BYTE *) lr.pBits) + lr.Pitch * x, proj_texdata + 4 * x, 4 * sizeof(proj_texdata[0]));
        }
        hr = IDirect3DTexture9_UnlockRect(texture, 0);
        ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

        projected_textures_test(device, projected_tests_1);
        projected_textures_test(device, projected_tests_2);
        projected_textures_test(device, projected_tests_3);

        hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
        IDirect3DTexture9_Release(texture);
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff203040, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    /* Use a smaller volume texture than the biggest possible size for memory and performance reasons
     * Thus watch out if sampling from texels between 0 and 1.
     */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 32, 32, 32, 1, 0, fmt, D3DPOOL_MANAGED, &volume, 0);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
       "IDirect3DDevice9_CreateVolumeTexture failed with %08x\n", hr);
    if(!volume) {
        skip("Failed to create a volume texture\n");
        goto out;
    }

    hr = IDirect3DVolumeTexture9_LockBox(volume, 0, &lb, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_LockBox failed with %08x\n", hr);
    for(z = 0; z < 32; z++) {
        for(y = 0; y < 32; y++) {
            for(x = 0; x < 32; x++) {
                char size = (fmt == D3DFMT_A16B16G16R16 ? 8 : 4);
                void *mem = ((char *)  lb.pBits) + y * lb.RowPitch + z * lb.SlicePitch + x * size;
                float r_f = (float) x / 31.0;
                float g_f = (float) y / 31.0;
                float b_f = (float) z / 31.0;

                if(fmt == D3DFMT_A16B16G16R16) {
                    unsigned short *mem_s = mem;
                    mem_s[0]  = r_f * 65535.0;
                    mem_s[1]  = g_f * 65535.0;
                    mem_s[2]  = b_f * 65535.0;
                    mem_s[3]  = 65535;
                } else {
                    unsigned char *mem_c = mem;
                    mem_c[0]  = b_f * 255.0;
                    mem_c[1]  = g_f * 255.0;
                    mem_c[2]  = r_f * 255.0;
                    mem_c[3]  = 255;
                }
            }
        }
    }
    hr = IDirect3DVolumeTexture9_UnlockBox(volume, 0);
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_UnlockBox failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) volume);
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_UnlockBox failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        float quad1[] = {
            -1.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
            -1.0,       0.0,       0.1,     1.0,    1.0,    1.0,
             0.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,    1.0
        };
        float quad2[] = {
            -1.0,       0.0,       0.1,     1.0,    1.0,    1.0,
            -1.0,       1.0,       0.1,     1.0,    1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,    1.0,
             0.0,       1.0,       0.1,     1.0,    1.0,    1.0
        };
        float quad3[] = {
             0.0,       0.0,       0.1,     0.0,    0.0,
             0.0,       1.0,       0.1,     0.0,    0.0,
             1.0,       0.0,       0.1,     0.0,    0.0,
             1.0,       1.0,       0.1,     0.0,    0.0
        };
        float quad4[] = {
             0.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,    1.0,
             1.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
             1.0,       0.0,       0.1,     1.0,    1.0,    1.0
        };
        float mat[16] = {1.0, 0.0, 0.0, 0.0,
                         0.0, 0.0, 1.0, 0.0,
                         0.0, 1.0, 0.0, 0.0,
                         0.0, 0.0, 0.0, 1.0};
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);

        /* Draw a quad with all 3 coords enabled. Nothing fancy. v and w are swapped, but have the same
         * values
         */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Now disable the w coordinate. Does that change the input, or the output. The coordinates
         * are swapped by the matrix. If it changes the input, the v coord will be missing(green),
         * otherwise the w will be missing(blue).
         * turns out that on nvidia cards the blue color is missing, so it is an output modification.
         * On ATI cards the COUNT2 is ignored, and it behaves in the same way as COUNT3. */
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* default values? Set up the identity matrix, pass in 2 vertex coords, and enable 3 */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) identity);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* D3DTTFF_COUNT1. Set a NULL matrix, and count1, pass in all values as 1.0. Nvidia has count1 ==
         * disable. ATI extends it up to the amount of values needed for the volume texture
         */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ffffff, "quad 1 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00ffff00 /* NV*/ || color == 0x00ffffff /* ATI */,
       "quad 2 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "quad 3 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ffffff || color == 0x0000ff00, "quad 4 has color %08x, expected 0x00ffffff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff303030, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        float quad1[] = {
            -1.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
            -1.0,       0.0,       0.1,     1.0,    1.0,    1.0,
             0.0,      -1.0,       0.1,     1.0,    1.0,    1.0,
             0.0,       0.0,       0.1,     1.0,    1.0,    1.0
        };
        float quad2[] = {
            -1.0,       0.0,       0.1,
            -1.0,       1.0,       0.1,
             0.0,       0.0,       0.1,
             0.0,       1.0,       0.1,
        };
        float quad3[] = {
             0.0,       0.0,       0.1,     1.0,
             0.0,       1.0,       0.1,     1.0,
             1.0,       0.0,       0.1,     1.0,
             1.0,       1.0,       0.1,     1.0
        };
        float mat[16] =  {0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0,
                           0.0, 0.0, 0.0, 0.0,
                           0.0, 1.0, 0.0, 0.0};
        float mat2[16] = {0.0, 0.0, 0.0, 1.0,
                           1.0, 0.0, 0.0, 0.0,
                           0.0, 1.0, 0.0, 0.0,
                           0.0, 0.0, 1.0, 0.0};
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);

        /* Default values? 4 coords used, 3 passed. What happens to the 4th?
         * Use COUNT3 because newer Nvidia drivers return black when there are more (output) coords
         * than being used by the texture(volume tex -> 3). Again, as shown in earlier test the COUNTx
         * affects the post-transformation output, so COUNT3 plus the matrix above is OK for testing the
         * 4th *input* coordinate.
         */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* None passed */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) identity);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* 4 used, 1 passed */
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 4 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000ff00, "quad 1 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff0000, "quad 3 has color %08x, expected 0x00ff0000\n", color);
    /* Quad4: unused */

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DVolumeTexture9_Release(volume);

    out:
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &identity);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %08x\n", hr);
    IDirect3DVertexDeclaration9_Release(decl);
    IDirect3DVertexDeclaration9_Release(decl2);
    IDirect3DVertexDeclaration9_Release(decl3);
    IDirect3DVertexDeclaration9_Release(decl4);
}

static void texdepth_test(IDirect3DDevice9 *device)
{
    IDirect3DPixelShader9 *shader;
    HRESULT hr;
    const float texdepth_test_data1[] = { 0.25,  2.0, 0.0, 0.0};
    const float texdepth_test_data2[] = { 0.25,  0.5, 0.0, 0.0};
    const float texdepth_test_data3[] = {-1.00,  0.1, 0.0, 0.0};
    const float texdepth_test_data4[] = {-0.25, -0.5, 0.0, 0.0};
    const float texdepth_test_data5[] = { 1.00, -0.1, 0.0, 0.0};
    const float texdepth_test_data6[] = { 1.00,  0.5, 0.0, 0.0};
    const float texdepth_test_data7[] = { 0.50,  0.0, 0.0, 0.0};
    static const DWORD shader_code[] =
    {
        0xffff0104,                                                                 /* ps_1_4               */
        0x00000051, 0xa00f0001, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000,     /* def c1, 0, 0, 1, 1   */
        0x00000001, 0x800f0005, 0xa0e40000,                                         /* mov r5, c0           */
        0x0000fffd,                                                                 /* phase                */
        0x00000057, 0x800f0005,                                                     /* texdepth r5          */
        0x00000001, 0x800f0000, 0xa0e40001,                                         /* mov r0, c1           */
        0x0000ffff                                                                  /* end                  */
    };
    DWORD color;
    float vertex[] = {
       -1.0,   -1.0,    0.0,
        1.0,   -1.0,    1.0,
       -1.0,    1.0,    0.0,
        1.0,    1.0,    1.0
    };

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF returned %#x.\n", hr);

    /* Fill the depth buffer with a gradient */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    /* Now perform the actual tests. Same geometry, but with the shader */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 158, 240);
    ok(color == 0x000000ff, "Pixel 158(25%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 162, 240);
    ok(color == 0x00ffffff, "Pixel 158(25%% + 2 pixel) has color %08x, expected 0x00ffffff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 318, 240);
    ok(color == 0x000000ff, "Pixel 318(50%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 322, 240);
    ok(color == 0x00ffff00, "Pixel 322(50%% + 2 pixel) has color %08x, expected 0x00ffff00\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data3, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 1, 240);
    ok(color == 0x00ff0000, "Pixel 1(0%% + 2 pixel) has color %08x, expected 0x00ff0000\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data4, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }
    color = getPixelColor(device, 318, 240);
    ok(color == 0x000000ff, "Pixel 318(50%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 322, 240);
    ok(color == 0x0000ff00, "Pixel 322(50%% + 2 pixel) has color %08x, expected 0x0000ff00\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data5, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 1, 240);
    ok(color == 0x00ffff00, "Pixel 1(0%% + 2 pixel) has color %08x, expected 0x00ffff00\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data6, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 638, 240);
    ok(color == 0x000000ff, "Pixel 638(100%% + 2 pixel) has color %08x, expected 0x000000ff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data7, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 638, 240);
    ok(color == 0x000000ff, "Pixel 638(100%% + 2 pixel) has color %08x, expected 0x000000ff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    /* Cleanup */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
    IDirect3DPixelShader9_Release(shader);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
}

static void texkill_test(IDirect3DDevice9 *device)
{
    IDirect3DPixelShader9 *shader;
    HRESULT hr;
    DWORD color;

    const float vertex[] = {
    /*                          bottom  top    right    left */
        -1.0,   -1.0,   1.0,   -0.1,    0.9,    0.9,   -0.1,
         1.0,   -1.0,   0.0,    0.9,   -0.1,    0.9,   -0.1,
        -1.0,    1.0,   1.0,   -0.1,    0.9,   -0.1,    0.9,
         1.0,    1.0,   0.0,    0.9,   -0.1,   -0.1,    0.9,
    };

    static const DWORD shader_code_11[] =
    {
        0xffff0101,                                                             /* ps_1_1                     */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 1.0, 0.0, 0.0, 1.0 */
        0x00000041, 0xb00f0000,                                                 /* texkill t0                 */
        0x00000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                 */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD shader_code_20[] =
    {
        0xffff0200,                                                             /* ps_2_0                     */
        0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* def c0, 0.0, 0.0, 1.0, 1.0 */
        0x01000041, 0xb00f0000,                                                 /* texkill t0                 */
        0x02000001, 0x800f0800, 0xa0e40000,                                     /* mov oC0, c0                */
        0x0000ffff                                                              /* end                        */
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 7 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }
    color = getPixelColor(device, 63, 46);
    ok(color == 0x0000ff00, "Pixel 63/46 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 66, 46);
    ok(color == 0x0000ff00, "Pixel 66/64 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 63, 49);
    ok(color == 0x0000ff00, "Pixel 63/49 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 66, 49);
    ok(color == 0x00ff0000, "Pixel 66/49 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 578, 46);
    ok(color == 0x0000ff00, "Pixel 578/46 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 575, 46);
    ok(color == 0x0000ff00, "Pixel 575/64 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 578, 49);
    ok(color == 0x0000ff00, "Pixel 578/49 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 575, 49);
    ok(color == 0x00ff0000, "Pixel 575/49 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 63, 430);
    ok(color == 0x0000ff00, "Pixel 578/46 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 63, 433);
    ok(color == 0x0000ff00, "Pixel 575/64 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 66, 433);
    ok(color == 0x00ff0000, "Pixel 578/49 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 66, 430);
    ok(color == 0x00ff0000, "Pixel 575/49 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 578, 430);
    ok(color == 0x0000ff00, "Pixel 578/46 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 578, 433);
    ok(color == 0x0000ff00, "Pixel 575/64 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 575, 433);
    ok(color == 0x00ff0000, "Pixel 578/49 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 575, 430);
    ok(color == 0x00ff0000, "Pixel 575/49 has color %08x, expected 0x00ff0000\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
    IDirect3DPixelShader9_Release(shader);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_20, &shader);
    if(FAILED(hr)) {
        skip("Failed to create 2.0 test shader, most likely not supported\n");
        return;
    }

    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 7 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 63, 46);
    ok(color == 0x00ffff00, "Pixel 63/46 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 66, 46);
    ok(color == 0x00ffff00, "Pixel 66/64 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 63, 49);
    ok(color == 0x00ffff00, "Pixel 63/49 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 66, 49);
    ok(color == 0x000000ff, "Pixel 66/49 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 578, 46);
    ok(color == 0x00ffff00, "Pixel 578/46 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 575, 46);
    ok(color == 0x00ffff00, "Pixel 575/64 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 578, 49);
    ok(color == 0x00ffff00, "Pixel 578/49 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 575, 49);
    ok(color == 0x000000ff, "Pixel 575/49 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 63, 430);
    ok(color == 0x00ffff00, "Pixel 578/46 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 63, 433);
    ok(color == 0x00ffff00, "Pixel 575/64 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 66, 433);
    ok(color == 0x00ffff00, "Pixel 578/49 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 66, 430);
    ok(color == 0x000000ff, "Pixel 575/49 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 578, 430);
    ok(color == 0x00ffff00, "Pixel 578/46 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 578, 433);
    ok(color == 0x00ffff00, "Pixel 575/64 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 575, 433);
    ok(color == 0x00ffff00, "Pixel 578/49 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 575, 430);
    ok(color == 0x000000ff, "Pixel 575/49 has color %08x, expected 0x000000ff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    /* Cleanup */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
    IDirect3DPixelShader9_Release(shader);
}

static void x8l8v8u8_test(IDirect3DDevice9 *device)
{
    IDirect3D9 *d3d9;
    HRESULT hr;
    IDirect3DTexture9 *texture;
    IDirect3DPixelShader9 *shader;
    IDirect3DPixelShader9 *shader2;
    D3DLOCKED_RECT lr;
    DWORD color;
    static const DWORD shader_code[] =
    {
        0xffff0101,                             /* ps_1_1       */
        0x00000042, 0xb00f0000,                 /* tex t0       */
        0x00000001, 0x800f0000, 0xb0e40000,     /* mov r0, t0   */
        0x0000ffff                              /* end          */
    };
    static const DWORD shader_code2[] =
    {
        0xffff0101,                             /* ps_1_1       */
        0x00000042, 0xb00f0000,                 /* tex t0       */
        0x00000001, 0x800f0000, 0xb0ff0000,     /* mov r0, t0.w */
        0x0000ffff                              /* end          */
    };

    float quad[] = {
       -1.0,   -1.0,   0.1,     0.5,    0.5,
        1.0,   -1.0,   0.1,     0.5,    0.5,
       -1.0,    1.0,   0.1,     0.5,    0.5,
        1.0,    1.0,   0.1,     0.5,    0.5,
    };

    memset(&lr, 0, sizeof(lr));
    IDirect3DDevice9_GetDirect3D(device, &d3d9);
    hr = IDirect3D9_CheckDeviceFormat(d3d9, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                      0,  D3DRTYPE_TEXTURE, D3DFMT_X8L8V8U8);
    IDirect3D9_Release(d3d9);
    if(FAILED(hr)) {
        skip("No D3DFMT_X8L8V8U8 support\n");
        return;
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_X8L8V8U8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed (%08x)\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed (%08x)\n", hr);
    *((DWORD *) lr.pBits) = 0x11ca3141;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed (%08x)\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code2, &shader2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    }
    color = getPixelColor(device, 578, 430);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x82, 0x62, 0xca), 1),
       "D3DFMT_X8L8V8U8 = 0x112131ca returns color %08x, expected 0x008262ca\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, shader2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    }
    color = getPixelColor(device, 578, 430);
    ok(color == 0x00ffffff, "w component of D3DFMT_X8L8V8U8 = 0x11ca3141 returns color %08x\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);
    IDirect3DPixelShader9_Release(shader);
    IDirect3DPixelShader9_Release(shader2);
    IDirect3DTexture9_Release(texture);
}

static void autogen_mipmap_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3D9 *d3d;
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface;
    DWORD color;
    const RECT r1 = {256, 256, 512, 512};
    const RECT r2 = {512, 256, 768, 512};
    const RECT r3 = {256, 512, 512, 768};
    const RECT r4 = {512, 512, 768, 768};
    unsigned int x, y;
    D3DLOCKED_RECT lr;
    memset(&lr, 0, sizeof(lr));

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
       D3DUSAGE_AUTOGENMIPMAP,  D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8) != D3D_OK) {
        skip("No autogenmipmap support\n");
        IDirect3D9_Release(d3d);
        return;
    }
    IDirect3D9_Release(d3d);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    /* Make the mipmap big, so that a smaller mipmap is used
     */
    hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 0, D3DUSAGE_AUTOGENMIPMAP,
                                        D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %08x\n", hr);

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel returned %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
    for(y = 0; y < 1024; y++) {
        for(x = 0; x < 1024; x++) {
            DWORD *dst = (DWORD *) (((BYTE *) lr.pBits) + y * lr.Pitch + x * 4);
            POINT pt;

            pt.x = x;
            pt.y = y;
            if(PtInRect(&r1, pt)) {
                *dst = 0xffff0000;
            } else if(PtInRect(&r2, pt)) {
                *dst = 0xff00ff00;
            } else if(PtInRect(&r3, pt)) {
                *dst = 0xff0000ff;
            } else if(PtInRect(&r4, pt)) {
                *dst = 0xff000000;
            } else {
                *dst = 0xffffffff;
            }
        }
    }
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect returned %08x\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr)) {
        const float quad[] =  {
           -0.5,   -0.5,    0.1,    0.0,    0.0,
           -0.5,    0.5,    0.1,    0.0,    1.0,
            0.5,   -0.5,    0.1,    1.0,    0.0,
            0.5,    0.5,    0.1,    1.0,    1.0
        };

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);
    IDirect3DTexture9_Release(texture);

    color = getPixelColor(device, 200, 200);
    ok(color == 0x00ffffff, "pixel 200/200 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 280, 200);
    ok(color == 0x000000ff, "pixel 280/200 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 360, 200);
    ok(color == 0x00000000, "pixel 360/200 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 440, 200);
    ok(color == 0x00ffffff, "pixel 440/200 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 200, 270);
    ok(color == 0x00ffffff, "pixel 200/270 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 280, 270);
    ok(color == 0x00ff0000, "pixel 280/270 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 360, 270);
    ok(color == 0x0000ff00, "pixel 360/270 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 440, 270);
    ok(color == 0x00ffffff, "pixel 440/200 has color %08x, expected 0x00ffffff\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);
}

static void test_constant_clamp_vs(IDirect3DDevice9 *device)
{
    IDirect3DVertexShader9 *shader_11, *shader_11_2, *shader_20, *shader_20_2;
    IDirect3DVertexDeclaration9 *decl;
    HRESULT hr;
    DWORD color;
    static const DWORD shader_code_11[] =
    {
        0xfffe0101,                                         /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    static const DWORD shader_code_11_2[] =
    {
        0xfffe0101,                                         /* vs_1_1           */
        0x00000051, 0xa00f0001, 0x3fa00000, 0xbf000000, 0xbfc00000, 0x3f800000, /* dcl ... */
        0x00000051, 0xa00f0002, 0xbf000000, 0x3fa00000, 0x40000000, 0x3f800000, /* dcl ... */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    static const DWORD shader_code_20[] =
    {
        0xfffe0200,                                         /* vs_2_0           */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    static const DWORD shader_code_20_2[] =
    {
        0xfffe0200,                                         /* vs_2_0           */
        0x05000051, 0xa00f0001, 0x3fa00000, 0xbf000000, 0xbfc00000, 0x3f800000,
        0x05000051, 0xa00f0002, 0xbf000000, 0x3fa00000, 0x40000000, 0x3f800000,
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    float quad1[] = {
        -1.0,   -1.0,   0.1,
         0.0,   -1.0,   0.1,
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1
    };
    float quad2[] = {
         0.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
         0.0,    0.0,   0.1,
         1.0,    0.0,   0.1
    };
    float quad3[] = {
         0.0,    0.0,   0.1,
         1.0,    0.0,   0.1,
         0.0,    1.0,   0.1,
         1.0,    1.0,   0.1
    };
    float quad4[] = {
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1,
        -1.0,    1.0,   0.1,
         0.0,    1.0,   0.1
    };
    float test_data_c1[4] = {  1.25, -0.50, -1.50, 1.0};
    float test_data_c2[4] = { -0.50,  1.25,  2.00, 1.0};

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_11_2, &shader_11_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_20, &shader_20);
    if(FAILED(hr)) shader_20 = NULL;
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_20_2, &shader_20_2);
    if(FAILED(hr)) shader_20_2 = NULL;
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetVertexShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_11_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        if(shader_20) {
            hr = IDirect3DDevice9_SetVertexShader(device, shader_20);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        if(shader_20_2) {
            hr = IDirect3DDevice9_SetVertexShader(device, shader_20_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0xbf, 0x80), 1),
       "quad 1 has color %08x, expected 0x00bfbf80\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0xbf, 0x80), 1),
       "quad 2 has color %08x, expected 0x00bfbf80\n", color);
    if(shader_20) {
        color = getPixelColor(device, 480, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0xbf, 0x80), 1),
           "quad 3 has color %08x, expected 0x00bfbf80\n", color);
    }
    if(shader_20_2) {
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0xbf, 0x80), 1),
           "quad 4 has color %08x, expected 0x00bfbf80\n", color);
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DVertexDeclaration9_Release(decl);
    if(shader_20_2) IDirect3DVertexShader9_Release(shader_20_2);
    if(shader_20) IDirect3DVertexShader9_Release(shader_20);
    IDirect3DVertexShader9_Release(shader_11_2);
    IDirect3DVertexShader9_Release(shader_11);
}

static void constant_clamp_ps_test(void)
{
    IDirect3DPixelShader9 *shader_11, *shader_12, *shader_14, *shader_20;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const DWORD shader_code_11[] =
    {
        0xffff0101,                                         /* ps_1_1           */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    static const DWORD shader_code_12[] =
    {
        0xffff0102,                                         /* ps_1_2           */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    /* Skip 1.3 shaders because we have only 4 quads (ok, could make them
     * smaller if needed). 1.2 and 1.4 shaders behave the same, so it's
     * unlikely that 1.3 shaders are different. During development of this
     * test, 1.3 shaders were verified too. */
    static const DWORD shader_code_14[] =
    {
        0xffff0104,                                         /* ps_1_4           */
        /* Try to make one constant local. It gets clamped too, although the
         * binary contains the bigger numbers. */
        0x00000051, 0xa00f0002, 0xbf000000, 0x3fa00000, 0x40000000, 0x3f800000, /* def c2, -0.5, 1.25, 2, 1 */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    static const DWORD shader_code_20[] =
    {
        0xffff0200,                                         /* ps_2_0           */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x02000001, 0x800f0800, 0x80e40000,                 /* mov oC0, r0      */
        0x0000ffff                                          /* end              */
    };
    static const float quad1[] =
    {
        -1.0f, -1.0f, 0.1f,
        -1.0f,  0.0f, 0.1f,
         0.0f, -1.0f, 0.1f,
         0.0f,  0.0f, 0.1f,
    };
    static const float quad2[] =
    {
         0.0f, -1.0f, 0.1f,
         0.0f,  0.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
         1.0f,  0.0f, 0.1f,
    };
    static const float quad3[] =
    {
         0.0f,  0.0f, 0.1f,
         0.0f,  1.0f, 0.1f,
         1.0f,  0.0f, 0.1f,
         1.0f,  1.0f, 0.1f,
    };
    static const float quad4[] =
    {
        -1.0f,  0.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         0.0f,  0.0f, 0.1f,
         0.0f,  1.0f, 0.1f,
    };
    static const float test_data_c1[4] = { 1.25f, -0.50f, -1.50f, 1.0f};
    static const float test_data_c2[4] = {-0.50f,  1.25f,  2.00f, 1.0f};

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 4))
    {
        skip("No ps_1_4 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12, &shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14, &shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_20, &shader_20);
    if(FAILED(hr)) shader_20 = NULL;

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        if(shader_20) {
            hr = IDirect3DDevice9_SetPixelShader(device, shader_20);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x80, 0x80, 0x00), 1),
       "quad 1 has color %08x, expected 0x00808000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x80, 0x80, 0x00), 1),
       "quad 2 has color %08x, expected 0x00808000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x80, 0x80, 0x00), 1),
       "quad 3 has color %08x, expected 0x00808000\n", color);
    if(shader_20) {
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xbf, 0xbf, 0x80), 1),
           "quad 4 has color %08x, expected 0x00bfbf80\n", color);
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    if(shader_20) IDirect3DPixelShader9_Release(shader_20);
    IDirect3DPixelShader9_Release(shader_14);
    IDirect3DPixelShader9_Release(shader_12);
    IDirect3DPixelShader9_Release(shader_11);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void dp2add_ps_test(void)
{
    IDirect3DPixelShader9 *shader_dp2add_sat;
    IDirect3DPixelShader9 *shader_dp2add;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    /* DP2ADD is defined as:  (src0.r * src1.r) + (src0.g * src1.g) + src2.
     * One D3D restriction of all shader instructions except SINCOS is that no more than 2
     * source tokens can be constants.  So, for this exercise, we move contents of c0 to
     * r0 first.
     * The result here for the r,g,b components should be roughly 0.5:
     *   (0.5 * 0.5) + (0.5 * 0.5) + 0.0 = 0.5 */
    static const DWORD shader_code_dp2add[] =  {
        0xffff0200,                                                             /* ps_2_0                       */
        0x05000051, 0xa00f0000, 0x3f000000, 0x3f000000, 0x3f800000, 0x00000000, /* def c0, 0.5, 0.5, 1.0, 0     */

        0x02000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                   */
        0x0400005a, 0x80070000, 0x80000000, 0x80000000, 0x80ff0000,             /* dp2add r0.rgb, r0, r0, r0.a  */

        0x02000001, 0x80080000, 0xa0aa0000,                                     /* mov r0.a, c0.b               */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                  */
        0x0000ffff                                                              /* end                          */
    };

    /* Test the _sat modifier, too.  Result here should be:
     *   DP2: (-0.5 * -0.5) + (-0.5 * -0.5) + 2.0 = 2.5
     *      _SAT: ==> 1.0
     *   ADD: (1.0 + -0.5) = 0.5
     */
    static const DWORD shader_code_dp2add_sat[] =  {
        0xffff0200,                                                             /* ps_2_0                           */
        0x05000051, 0xa00f0000, 0xbf000000, 0xbf000000, 0x3f800000, 0x40000000, /* def c0, -0.5, -0.5, 1.0, 2.0     */

        0x02000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                       */
        0x0400005a, 0x80170000, 0x80000000, 0x80000000, 0x80ff0000,             /* dp2add_sat r0.rgb, r0, r0, r0.a  */
        0x03000002, 0x80070000, 0x80e40000, 0xa0000000,                         /* add r0.rgb, r0, c0.r             */

        0x02000001, 0x80080000, 0xa0aa0000,                                     /* mov r0.a, c0.b                   */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                      */
        0x0000ffff                                                              /* end                              */
    };
    static const float quad[] =
    {
        -1.0f,   -1.0f,   0.1f,
        -1.0f,    1.0f,   0.1f,
         1.0f,   -1.0f,   0.1f,
         1.0f,    1.0f,   0.1f,
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        skip("No ps_2_0 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_dp2add, &shader_dp2add);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_dp2add_sat, &shader_dp2add_sat);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, shader_dp2add);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw primitive, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 360, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x7f, 0x7f, 0x7f), 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present frame, hr %#x.\n", hr);
    IDirect3DPixelShader9_Release(shader_dp2add);

    hr = IDirect3DDevice9_SetPixelShader(device, shader_dp2add_sat);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw primitive, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = getPixelColor(device, 360, 240);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x7f, 0x7f, 0x7f), 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present frame, hr %#x.\n", hr);
    IDirect3DPixelShader9_Release(shader_dp2add_sat);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void cnd_test(void)
{
    IDirect3DPixelShader9 *shader_11_coissue_2, *shader_12_coissue_2, *shader_13_coissue_2, *shader_14_coissue_2;
    IDirect3DPixelShader9 *shader_11_coissue, *shader_12_coissue, *shader_13_coissue, *shader_14_coissue;
    IDirect3DPixelShader9 *shader_11, *shader_12, *shader_13, *shader_14;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    DWORD color;
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
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, ???(t0)      */
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
        -1.0f, -1.0f, 0.1f, 0.0f, 0.0f, 1.0f,
        -1.0f,  0.0f, 0.1f, 0.0f, 1.0f, 0.0f,
         0.0f, -1.0f, 0.1f, 1.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.1f, 1.0f, 1.0f, 0.0f,
    };
    static const float quad2[] =
    {
         0.0f, -1.0f, 0.1f, 0.0f, 0.0f, 1.0f,
         0.0f,  0.0f, 0.1f, 0.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 0.1f, 1.0f, 0.0f, 1.0f,
         1.0f,  0.0f, 0.1f, 1.0f, 1.0f, 0.0f,
    };
    static const float quad3[] =
    {
         0.0f,  0.0f, 0.1f, 0.0f, 0.0f, 1.0f,
         0.0f,  1.0f, 0.1f, 0.0f, 1.0f, 0.0f,
         1.0f,  0.0f, 0.1f, 1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.1f, 1.0f, 1.0f, 0.0f,
    };
    static const float quad4[] =
    {
        -1.0f,  0.0f, 0.1f, 0.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.1f, 0.0f, 1.0f, 0.0f,
         0.0f,  0.0f, 0.1f, 1.0f, 0.0f, 1.0f,
         0.0f,  1.0f, 0.1f, 1.0f, 1.0f, 0.0f,
    };
    static const float test_data_c1[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    static const float test_data_c2[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    static const float test_data_c1_coi[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    static const float test_data_c2_coi[4] = {1.0f, 0.0f, 1.0f, 1.0f};

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 4))
    {
        skip("No ps_1_4 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12, &shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_13, &shader_13);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14, &shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11_coissue, &shader_11_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12_coissue, &shader_12_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_13_coissue, &shader_13_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14_coissue, &shader_14_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11_coissue_2, &shader_11_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12_coissue_2, &shader_12_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_13_coissue_2, &shader_13_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14_coissue_2, &shader_14_coissue_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_13);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

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
    ok(color_match(color, 0x00000000, 1), "pixel 162, 358 has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 158, 362);
    ok(color == 0x00ffffff, "pixel 158, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color_match(color, 0x00000000, 1), "pixel 162, 362 has color 0x%08x, expected 0x00000000.\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 478, 358);
    ok(color == 0x00ffffff, "pixel 478, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color_match(color, 0x00000000, 1), "pixel 482, 358 has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x00ffffff, "pixel 478, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color_match(color, 0x00000000, 1), "pixel 482, 362 has color 0x%08x, expected 0x00000000.\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 478, 118);
    ok(color == 0x00ffffff, "pixel 478, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 118);
    ok(color_match(color, 0x00000000, 1), "pixel 482, 118 has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color == 0x00ffffff, "pixel 478, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color_match(color, 0x00000000, 1), "pixel 482, 122 has color 0x%08x, expected 0x00000000.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_13_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);

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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    /* Retest with the coissue flag on the alpha instruction instead. This works "as expected". */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11_coissue_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12_coissue_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_13_coissue_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14_coissue_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

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
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 238, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 242, 358);
    ok(color_match(color, 0x00000000, 1),
        "pixel 242, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 238, 362);
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 238, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 242, 362);
    ok(color_match(color, 0x00000000, 1),
        "pixel 242, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 558, 358);
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 558, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 358);
    ok(color_match(color, 0x00000000, 1),
        "pixel 562, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 558, 362);
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 558, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 362);
    ok(color_match(color, 0x00000000, 1),
        "pixel 562, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 558, 118);
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 558, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 118);
    ok(color_match(color, 0x00000000, 1),
        "pixel 562, 118 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 558, 122);
    ok(color_match(color, 0x00ffffff, 1),
        "pixel 558, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 562, 122);
    ok(color_match(color, 0x00000000, 1),
        "pixel 562, 122 has color %08x, expected 0x00000000\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DPixelShader9_Release(shader_14_coissue_2);
    IDirect3DPixelShader9_Release(shader_13_coissue_2);
    IDirect3DPixelShader9_Release(shader_12_coissue_2);
    IDirect3DPixelShader9_Release(shader_11_coissue_2);
    IDirect3DPixelShader9_Release(shader_14_coissue);
    IDirect3DPixelShader9_Release(shader_13_coissue);
    IDirect3DPixelShader9_Release(shader_12_coissue);
    IDirect3DPixelShader9_Release(shader_11_coissue);
    IDirect3DPixelShader9_Release(shader_14);
    IDirect3DPixelShader9_Release(shader_13);
    IDirect3DPixelShader9_Release(shader_12);
    IDirect3DPixelShader9_Release(shader_11);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void nested_loop_test(void)
{
    IDirect3DVertexShader9 *vshader;
    IDirect3DPixelShader9 *shader;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const DWORD shader_code[] =
    {
        0xffff0300,                                                             /* ps_3_0               */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0, 0, 0, 1   */
        0x05000051, 0xa00f0001, 0x3d000000, 0x00000000, 0x00000000, 0x00000000, /* def c1, 1/32, 0, 0, 0*/
        0x05000030, 0xf00f0000, 0x00000004, 0x00000000, 0x00000002, 0x00000000, /* defi i0, 4, 0, 2, 0  */
        0x02000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0           */
        0x0200001b, 0xf0e40800, 0xf0e40000,                                     /* loop aL, i0          */
        0x0200001b, 0xf0e40800, 0xf0e40000,                                     /* loop aL, i0          */
        0x03000002, 0x800f0000, 0x80e40000, 0xa0e40001,                         /* add r0, r0, c1       */
        0x0000001d,                                                             /* endloop              */
        0x0000001d,                                                             /* endloop              */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0          */
        0x0000ffff                                                              /* end                  */
    };
    static const DWORD vshader_code[] =
    {
        0xfffe0300,                                                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,                                     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,                                     /* mov o0, v0           */
        0x0000ffff                                                              /* end                  */
    };
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
         1.0f,  1.0f, 0.1f,
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        skip("No shader model 3 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vshader_code, &vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x0000ff00, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    color = getPixelColor(device, 360, 240);
    ok(color_match(color, 0x00800000, 1),
            "Nested loop test returned color 0x%08x, expected 0x00800000.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DPixelShader9_Release(shader);
    IDirect3DVertexShader9_Release(vshader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void pretransformed_varying_test(void)
{
    /* dcl_position: fails to compile */
    static const DWORD blendweight_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000001, 0x900f0000,     /* dcl_blendweight, v0      */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD blendindices_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000002, 0x900f0000,     /* dcl_blendindices, v0     */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD normal_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000003, 0x900f0000,     /* dcl_normal, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* psize: fails? */
    static const DWORD texcoord0_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000005, 0x900f0000,     /* dcl_texcoord0, v0        */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD tangent_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000006, 0x900f0000,     /* dcl_tangent, v0          */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD binormal_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000007, 0x900f0000,     /* dcl_binormal, v0         */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* tessfactor: fails */
    /* positiont: fails */
    static const DWORD color_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000a, 0x900f0000,     /* dcl_color0, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD fog_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000b, 0x900f0000,     /* dcl_fog, v0              */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD depth_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000c, 0x900f0000,     /* dcl_depth, v0            */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    static const DWORD specular_code[] =
    {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8001000a, 0x900f0000,     /* dcl_color1, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* sample: fails */

    static const struct
    {
        const char *name;
        const DWORD *shader_code;
        DWORD color;
        BOOL todo;
    }
    tests[] =
    {
        {"blendweight",     blendweight_code,   0x00191919, TRUE },
        {"blendindices",    blendindices_code,  0x00333333, TRUE },
        {"normal",          normal_code,        0x004c4c4c, TRUE },
        {"texcoord0",       texcoord0_code,     0x00808c8c, FALSE},
        {"tangent",         tangent_code,       0x00999999, TRUE },
        {"binormal",        binormal_code,      0x00b2b2b2, TRUE },
        {"color",           color_code,         0x00e6e6e6, FALSE},
        {"fog",             fog_code,           0x00666666, TRUE },
        {"depth",           depth_code,         0x00cccccc, TRUE },
        {"specular",        specular_code,      0x004488ff, FALSE},
    };
    /* Declare a monster vertex type :-) */
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,   0,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT,      0},
        {0,  16,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,    0},
        {0,  32,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES,   0},
        {0,  48,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,         0},
        {0,  64,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_FOG,            0},
        {0,  80,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       0},
        {0,  96,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,        0},
        {0, 112,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,       0},
        {0, 128,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_DEPTH,          0},
        {0, 144,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        {0, 148,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          1},
        D3DDECL_END()
    };

    static const struct
    {
        float pos_x,        pos_y,      pos_z,      rhw;
        float weight_1,     weight_2,   weight_3,   weight_4;
        float index_1,      index_2,    index_3,    index_4;
        float normal_1,     normal_2,   normal_3,   normal_4;
        float fog_1,        fog_2,      fog_3,      fog_4;
        float texcoord_1,   texcoord_2, texcoord_3, texcoord_4;
        float tangent_1,    tangent_2,  tangent_3,  tangent_4;
        float binormal_1,   binormal_2, binormal_3, binormal_4;
        float depth_1,      depth_2,    depth_3,    depth_4;
        D3DCOLOR diffuse;
        D3DCOLOR specular;
    }
    data[] =
    {
        {
            0.0f,   0.0f,   0.1f,   1.0f,
            0.1f,   0.1f,   0.1f,   0.1f,
            0.2f,   0.2f,   0.2f,   0.2f,
            0.3f,   0.3f,   0.3f,   0.3f,
            0.4f,   0.4f,   0.4f,   0.4f,
            0.5f,   0.55f,  0.55f,  0.55f,
            0.6f,   0.6f,   0.6f,   0.7f,
            0.7f,   0.7f,   0.7f,   0.6f,
            0.8f,   0.8f,   0.8f,   0.8f,
            0xe6e6e6e6, /* 0.9 * 256 */
            0x224488ff, /* Nothing special */
        },
        {
            640.0f, 0.0f,   0.1f,   1.0f,
            0.1f,   0.1f,   0.1f,   0.1f,
            0.2f,   0.2f,   0.2f,   0.2f,
            0.3f,   0.3f,   0.3f,   0.3f,
            0.4f,   0.4f,   0.4f,   0.4f,
            0.5f,   0.55f,  0.55f,  0.55f,
            0.6f,   0.6f,   0.6f,   0.7f,
            0.7f,   0.7f,   0.7f,   0.6f,
            0.8f,   0.8f,   0.8f,   0.8f,
            0xe6e6e6e6, /* 0.9 * 256 */
            0x224488ff, /* Nothing special */
        },
        {
            0.0f,   480.0f, 0.1f,   1.0f,
            0.1f,   0.1f,   0.1f,   0.1f,
            0.2f,   0.2f,   0.2f,   0.2f,
            0.3f,   0.3f,   0.3f,   0.3f,
            0.4f,   0.4f,   0.4f,   0.4f,
            0.5f,   0.55f,  0.55f,  0.55f,
            0.6f,   0.6f,   0.6f,   0.7f,
            0.7f,   0.7f,   0.7f,   0.6f,
            0.8f,   0.8f,   0.8f,   0.8f,
            0xe6e6e6e6, /* 0.9 * 256 */
            0x224488ff, /* Nothing special */
        },
        {
           640.0f,  480.0f, 0.1f,   1.0f,
           0.1f,    0.1f,   0.1f,   0.1f,
           0.2f,    0.2f,   0.2f,   0.2f,
           0.3f,    0.3f,   0.3f,   0.3f,
           0.4f,    0.4f,   0.4f,   0.4f,
           0.5f,    0.55f,  0.55f,  0.55f,
           0.6f,    0.6f,   0.6f,   0.7f,
           0.7f,    0.7f,   0.7f,   0.6f,
           0.8f,    0.8f,   0.8f,   0.8f,
           0xe6e6e6e6, /* 0.9 * 256 */
           0x224488ff, /* Nothing special */
        },
    };
    IDirect3DVertexDeclaration9 *decl;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        skip("No shader model 3 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
    for (i = 0; i < sizeof(tests) / sizeof(*tests); ++i)
    {
        IDirect3DPixelShader9 *shader;

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

        hr = IDirect3DDevice9_CreatePixelShader(device, tests[i].shader_code, &shader);
        ok(SUCCEEDED(hr), "Failed to create pixel shader for test %s, hr %#x.\n", tests[i].name, hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, data, sizeof(*data));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
        }

        /* This isn't a weekend's job to fix, ignore the problem for now.
         * Needs a replacement pipeline. */
        color = getPixelColor(device, 360, 240);
        if (tests[i].todo)
            todo_wine ok(color_match(color, tests[i].color, 1)
                    || broken(color_match(color, 0x00000000, 1)
                    && tests[i].shader_code == blendindices_code),
                    "Test %s returned color 0x%08x, expected 0x%08x (todo).\n",
                    tests[i].name, color, tests[i].color);
        else
            ok(color_match(color, tests[i].color, 1),
                    "Test %s returned color 0x%08x, expected 0x%08x.\n",
                    tests[i].name, color, tests[i].color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "Failed to set pixel shader for test %s, hr %#x.\n", tests[i].name, hr);
        IDirect3DPixelShader9_Release(shader);
    }

    IDirect3DVertexDeclaration9_Release(decl);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_compare_instructions(IDirect3DDevice9 *device)
{
    static const DWORD shader_sge_vec_code[] =
    {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000d, 0xd00f0000, 0x80e40000, 0xa0e40001,     /* sge oD0, r0, c1          */
        0x0000ffff                                          /* end                      */
    };
    static const DWORD shader_slt_vec_code[] =
    {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000c, 0xd00f0000, 0x80e40000, 0xa0e40001,     /* slt oD0, r0, c1          */
        0x0000ffff                                          /* end                      */
    };
    static const DWORD shader_sge_scalar_code[] =
    {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000d, 0xd0010000, 0x80000000, 0xa0550001,     /* slt oD0.r, r0.r, c1.b    */
        0x0000000d, 0xd0020000, 0x80550000, 0xa0aa0001,     /* slt oD0.g, r0.g, c1.r    */
        0x0000000d, 0xd0040000, 0x80aa0000, 0xa0000001,     /* slt oD0.b, r0.b, c1.g    */
        0x0000ffff                                          /* end                      */
    };
    static const DWORD shader_slt_scalar_code[] =
    {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000c, 0xd0010000, 0x80000000, 0xa0aa0001,     /* slt oD0.r, r0.r, c1.b    */
        0x0000000c, 0xd0020000, 0x80550000, 0xa0000001,     /* slt oD0.g, r0.g, c1.r    */
        0x0000000c, 0xd0040000, 0x80aa0000, 0xa0550001,     /* slt oD0.b, r0.b, c1.g    */
        0x0000ffff                                          /* end                      */
    };
    IDirect3DVertexShader9 *shader_sge_vec;
    IDirect3DVertexShader9 *shader_slt_vec;
    IDirect3DVertexShader9 *shader_sge_scalar;
    IDirect3DVertexShader9 *shader_slt_scalar;
    HRESULT hr, color;
    float quad1[] =  {
        -1.0,   -1.0,   0.1,
         0.0,   -1.0,   0.1,
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1
    };
    float quad2[] =  {
         0.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
         0.0,    0.0,   0.1,
         1.0,    0.0,   0.1
    };
    float quad3[] =  {
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1,
        -1.0,    1.0,   0.1,
         0.0,    1.0,   0.1
    };
    float quad4[] =  {
         0.0,    0.0,   0.1,
         1.0,    0.0,   0.1,
         0.0,    1.0,   0.1,
         1.0,    1.0,   0.1
    };
    const float const0[4] = {0.8, 0.2, 0.2, 0.2};
    const float const1[4] = {0.2, 0.8, 0.2, 0.2};

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_sge_vec_code, &shader_sge_vec);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_slt_vec_code, &shader_slt_vec);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_sge_scalar_code, &shader_sge_scalar);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_slt_scalar_code, &shader_slt_scalar);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, const0, 1);
    ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, const1, 1);
    ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetVertexShader(device, shader_sge_vec);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_slt_vec);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,  quad2, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_sge_scalar);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, const0, 1);
        ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_slt_scalar);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff00ff, "Compare test: Quad 1(sge vec) returned color 0x%08x, expected 0x00ff00ff\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000ff00, "Compare test: Quad 2(slt vec) returned color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00ffffff, "Compare test: Quad 3(sge scalar) returned color 0x%08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 480, 160);
    ok(color == 0x000000ff, "Compare test: Quad 4(slt scalar) returned color 0x%08x, expected 0x000000ff\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    IDirect3DVertexShader9_Release(shader_sge_vec);
    IDirect3DVertexShader9_Release(shader_slt_vec);
    IDirect3DVertexShader9_Release(shader_sge_scalar);
    IDirect3DVertexShader9_Release(shader_slt_scalar);
}

static void test_vshader_input(IDirect3DDevice9 *device)
{
    static const DWORD swapped_shader_code_3[] =
    {
        0xfffe0300,                                         /* vs_3_0               */
        0x0200001f, 0x80000000, 0xe00f0000,                 /* dcl_position o0      */
        0x0200001f, 0x8000000a, 0xe00f0001,                 /* dcl_color o1         */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord0 v1     */
        0x0200001f, 0x80010005, 0x900f0002,                 /* dcl_texcoord1 v2     */
        0x02000001, 0xe00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x02000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x03000002, 0xe00f0001, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD swapped_shader_code_1[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord0 v1     */
        0x0000001f, 0x80010005, 0x900f0002,                 /* dcl_texcoord1 v2     */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x00000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x00000002, 0xd00f0000, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD swapped_shader_code_2[] =
    {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord0 v1     */
        0x0200001f, 0x80010005, 0x900f0002,                 /* dcl_texcoord1 v2     */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x02000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x03000002, 0xd00f0000, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD texcoord_color_shader_code_3[] =
    {
        0xfffe0300,                                         /* vs_3_0               */
        0x0200001f, 0x80000000, 0xe00f0000,                 /* dcl_position o0      */
        0x0200001f, 0x8000000a, 0xe00f0001,                 /* dcl_color o1         */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x02000001, 0xe00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x02000001, 0xe00f0001, 0x90e40001,                 /* mov o1, v1           */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD texcoord_color_shader_code_2[] =
    {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x02000001, 0xd00f0000, 0x90e40001,                 /* mov oD0, v1          */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD texcoord_color_shader_code_1[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000001, 0xd00f0000, 0x90e40001,                 /* mov oD0, v1          */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD color_color_shader_code_3[] =
    {
        0xfffe0300,                                         /* vs_3_0               */
        0x0200001f, 0x80000000, 0xe00f0000,                 /* dcl_position o0      */
        0x0200001f, 0x8000000a, 0xe00f0001,                 /* dcl_color o1         */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x02000001, 0xe00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x03000005, 0xe00f0001, 0xa0e40000, 0x90e40001,     /* mul o1, c0, v1       */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD color_color_shader_code_2[] =
    {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x03000005, 0xd00f0000, 0xa0e40000, 0x90e40001,     /* mul oD0, c0, v1      */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD color_color_shader_code_1[] =
    {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000005, 0xd00f0000, 0xa0e40000, 0x90e40001,     /* mul oD0, c0, v1      */
        0x0000ffff                                          /* end                  */
    };
    static const DWORD ps3_code[] =
    {
        0xffff0300,                                         /* ps_3_0               */
        0x0200001f, 0x8000000a, 0x900f0000,                 /* dcl_color0 v0        */
        0x02000001, 0x800f0800, 0x90e40000,                 /* mov oC0, v0          */
        0x0000ffff                                          /* end                  */
    };
    IDirect3DVertexShader9 *swapped_shader, *texcoord_color_shader, *color_color_shader;
    IDirect3DPixelShader9 *ps;
    HRESULT hr;
    DWORD color;
    float quad1[] =  {
        -1.0,   -1.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
         0.0,   -1.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
        -1.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
         0.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
    };
    float quad2[] =  {
         0.0,   -1.0,   0.1,    1.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
         1.0,   -1.0,   0.1,    1.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
         0.0,    0.0,   0.1,    1.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
         1.0,    0.0,   0.1,    1.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
    };
    float quad3[] =  {
        -1.0,    0.0,   0.1,   -1.0,    0.0,    0.0,    0.0,    1.0,    -1.0,   0.0,    0.0,
         0.0,    0.0,   0.1,   -1.0,    0.0,    0.0,    0.0,    1.0,     0.0,   0.0,    0.0,
        -1.0,    1.0,   0.1,   -1.0,    0.0,    0.0,    0.0,    0.0,    -1.0,   1.0,    0.0,
         0.0,    1.0,   0.1,   -1.0,    0.0,    0.0,    0.0,   -1.0,     0.0,   0.0,    0.0,
    };
    float quad4[] =  {
         0.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
         1.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
         0.0,    1.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
         1.0,    1.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.5,    0.0,
    };
    static const D3DVERTEXELEMENT9 decl_elements_twotexcrd[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       0},
        {0,  28,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       1},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_twotexcrd_rightorder[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       1},
        {0,  28,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_onetexcrd[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_twotexcrd_wrongidx[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       1},
        {0,  28,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       2},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_texcoord_color[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,       0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_color_color[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_color_ubyte[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_color_float[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    IDirect3DVertexDeclaration9 *decl_twotexcrd, *decl_onetexcrd, *decl_twotex_wrongidx, *decl_twotexcrd_rightorder;
    IDirect3DVertexDeclaration9 *decl_texcoord_color, *decl_color_color, *decl_color_ubyte, *decl_color_float;
    unsigned int i;
    float normalize[4] = {1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0};
    float no_normalize[4] = {1.0, 1.0, 1.0, 1.0};

    struct vertex quad1_color[] =  {
       {-1.0,   -1.0,   0.1,    0x00ff8040},
       { 0.0,   -1.0,   0.1,    0x00ff8040},
       {-1.0,    0.0,   0.1,    0x00ff8040},
       { 0.0,    0.0,   0.1,    0x00ff8040}
    };
    struct vertex quad2_color[] =  {
       { 0.0,   -1.0,   0.1,    0x00ff8040},
       { 1.0,   -1.0,   0.1,    0x00ff8040},
       { 0.0,    0.0,   0.1,    0x00ff8040},
       { 1.0,    0.0,   0.1,    0x00ff8040}
    };
    struct vertex quad3_color[] =  {
       {-1.0,    0.0,   0.1,    0x00ff8040},
       { 0.0,    0.0,   0.1,    0x00ff8040},
       {-1.0,    1.0,   0.1,    0x00ff8040},
       { 0.0,    1.0,   0.1,    0x00ff8040}
    };
    float quad4_color[] =  {
         0.0,    0.0,   0.1,    1.0,    1.0,    0.0,    0.0,
         1.0,    0.0,   0.1,    1.0,    1.0,    0.0,    1.0,
         0.0,    1.0,   0.1,    1.0,    1.0,    0.0,    0.0,
         1.0,    1.0,   0.1,    1.0,    1.0,    0.0,    1.0,
    };

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_twotexcrd, &decl_twotexcrd);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_onetexcrd, &decl_onetexcrd);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_twotexcrd_wrongidx, &decl_twotex_wrongidx);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_twotexcrd_rightorder, &decl_twotexcrd_rightorder);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_texcoord_color, &decl_texcoord_color);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_color, &decl_color_color);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_ubyte, &decl_color_ubyte);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_float, &decl_color_float);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %08x\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, ps3_code, &ps);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %08x\n", hr);

    for(i = 1; i <= 3; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);
        if(i == 3) {
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_3, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_SetPixelShader(device, ps);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        } else if(i == 2){
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_2, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
        } else if(i == 1) {
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_1, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
        }

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetVertexShader(device, swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_onetexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                /* Succeeds or fails, depending on SW or HW vertex processing */
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x), i = 1\n", hr);
            }

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd_rightorder);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotex_wrongidx);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = 1\n", hr);
            }

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
        }

        if(i == 3 || i == 2) {
            color = getPixelColor(device, 160, 360);
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x80), 1),
               "Input test: Quad 1(2crd) returned color 0x%08x, expected 0x00ffff80\n", color);

            /* The last value of the read but undefined stream is used, it is 0x00. The defined input is vec4(1, 0, 0, 0) */
            color = getPixelColor(device, 480, 360);
            ok(color == 0x00ffff00 || color ==0x00ff0000,
               "Input test: Quad 2(1crd) returned color 0x%08x, expected 0x00ffff00\n", color);
            color = getPixelColor(device, 160, 120);
            /* Same as above, accept both the last used value and 0.0 for the undefined streams */
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x80), 1) || color == D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00),
               "Input test: Quad 3(2crd-wrongidx) returned color 0x%08x, expected 0x00ff0080\n", color);

            color = getPixelColor(device, 480, 160);
            ok(color == 0x00000000, "Input test: Quad 4(2crd-rightorder) returned color 0x%08x, expected 0x00000000\n", color);
        } else if(i == 1) {
            color = getPixelColor(device, 160, 360);
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x80), 1),
               "Input test: Quad 1(2crd) returned color 0x%08x, expected 0x00ffff80\n", color);
            color = getPixelColor(device, 480, 360);
            /* Accept the clear color as well in this case, since SW VP returns an error */
            ok(color == 0x00ffff00 || color == 0x00ff0000, "Input test: Quad 2(1crd) returned color 0x%08x, expected 0x00ffff00\n", color);
            color = getPixelColor(device, 160, 120);
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x80), 1) || color == D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00),
               "Input test: Quad 3(2crd-wrongidx) returned color 0x%08x, expected 0x00ff0080\n", color);
            color = getPixelColor(device, 480, 160);
            ok(color == 0x00000000, "Input test: Quad 4(2crd-rightorder) returned color 0x%08x, expected 0x00000000\n", color);
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff808080, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

        /* Now find out if the whole streams are re-read, or just the last active value for the
         * vertices is used.
         */
        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            float quad1_modified[] =  {
                -1.0,   -1.0,   0.1,    1.0,    0.0,    1.0,    0.0,   -1.0,     0.0,   0.0,    0.0,
                 0.0,   -1.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,    -1.0,   0.0,    0.0,
                -1.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,    0.0,     0.0,  -1.0,    0.0,
                 0.0,    0.0,   0.1,    1.0,    0.0,    1.0,    0.0,   -1.0,    -1.0,  -1.0,    0.0,
            };
            float quad2_modified[] =  {
                 0.0,   -1.0,   0.1,    0.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
                 1.0,   -1.0,   0.1,    0.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
                 0.0,    0.0,   0.1,    0.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
                 1.0,    0.0,   0.1,    0.0,    0.0,    0.0,    0.0,    0.0,     0.0,   0.0,    0.0,
            };

            hr = IDirect3DDevice9_SetVertexShader(device, swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 3, quad1_modified, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_onetexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2_modified, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                /* Succeeds or fails, depending on SW or HW vertex processing */
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x), i = 1\n", hr);
            }

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
        }

        color = getPixelColor(device, 480, 350);
        /* vs_1_1 may fail, accept the clear color. Some drivers also set the undefined streams to 0, accept that
         * as well.
         *
         * NOTE: This test fails on the reference rasterizer. In the refrast, the 4 vertices have different colors,
         * i.e., the whole old stream is read, and not just the last used attribute. Some games require that this
         * does *not* happen, otherwise they can crash because of a read from a bad pointer, so do not accept the
         * refrast's result.
         *
         * A test app for this behavior is Half Life 2 Episode 2 in dxlevel 95, and related games(Portal, TF2).
         */
        ok(color == 0x000000ff || color == 0x00808080 || color == 0x00000000,
           "Input test: Quad 2(different colors) returned color 0x%08x, expected 0x000000ff, 0x00808080 or 0x00000000\n", color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

        IDirect3DDevice9_SetVertexShader(device, NULL);
        IDirect3DDevice9_SetPixelShader(device, NULL);
        IDirect3DDevice9_SetVertexDeclaration(device, NULL);

        IDirect3DVertexShader9_Release(swapped_shader);
    }

    for(i = 1; i <= 3; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear returned %#x.\n", hr);
        if(i == 3) {
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_3, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_3, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_SetPixelShader(device, ps);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %08x\n", hr);
        } else if(i == 2){
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_2, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_2, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
        } else if(i == 1) {
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_1, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_1, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
        }

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetVertexShader(device, texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_texcoord_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1_color, sizeof(quad1_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexShader(device, color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

            hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, normalize, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_ubyte);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2_color, sizeof(quad2_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, no_normalize, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3_color, sizeof(quad3_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_float);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4_color, sizeof(float) * 7);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
        }
        IDirect3DDevice9_SetVertexShader(device, NULL);
        IDirect3DDevice9_SetVertexDeclaration(device, NULL);
        IDirect3DDevice9_SetPixelShader(device, NULL);

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

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

        IDirect3DVertexShader9_Release(texcoord_color_shader);
        IDirect3DVertexShader9_Release(color_color_shader);
    }

    IDirect3DVertexDeclaration9_Release(decl_twotexcrd);
    IDirect3DVertexDeclaration9_Release(decl_onetexcrd);
    IDirect3DVertexDeclaration9_Release(decl_twotex_wrongidx);
    IDirect3DVertexDeclaration9_Release(decl_twotexcrd_rightorder);

    IDirect3DVertexDeclaration9_Release(decl_texcoord_color);
    IDirect3DVertexDeclaration9_Release(decl_color_color);
    IDirect3DVertexDeclaration9_Release(decl_color_ubyte);
    IDirect3DVertexDeclaration9_Release(decl_color_float);

    IDirect3DPixelShader9_Release(ps);
}

static void srgbtexture_test(IDirect3DDevice9 *device)
{
    /* Fill a texture with 0x7f (~ .5), and then turn on the D3DSAMP_SRGBTEXTURE
     * texture stage state to render a quad using that texture.  The resulting
     * color components should be 0x36 (~ 0.21), per this formula:
     *    linear_color = ((srgb_color + 0.055) / 1.055) ^ 2.4
     * This is true where srgb_color > 0.04045. */
    struct IDirect3DTexture9 *texture = NULL;
    struct IDirect3DSurface9 *surface = NULL;
    IDirect3D9 *d3d = NULL;
    HRESULT hr;
    D3DLOCKED_RECT lr;
    DWORD color;
    float quad[] = {
        -1.0,       1.0,       0.0,     0.0,    0.0,
         1.0,       1.0,       0.0,     1.0,    0.0,
        -1.0,      -1.0,       0.0,     0.0,    1.0,
         1.0,      -1.0,       0.0,     1.0,    1.0,
    };


    memset(&lr, 0, sizeof(lr));
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                    D3DUSAGE_QUERY_SRGBREAD, D3DRTYPE_TEXTURE,
                                    D3DFMT_A8R8G8B8) != D3D_OK) {
        skip("D3DFMT_A8R8G8B8 textures with SRGBREAD not supported\n");
        goto out;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1, 0,
                                        D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                        &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);
    if(!texture) {
        skip("Failed to create A8R8G8B8 texture with SRGBREAD\n");
        goto out;
    }
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);

    fill_surface(surface, 0xff7f7f7f, 0);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_SRGBTEXTURE, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);


        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_SRGBTEXTURE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00363636, 1), "sRGB quad has color 0x%08x, expected 0x00363636.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

out:
    if(texture) IDirect3DTexture9_Release(texture);
    IDirect3D9_Release(d3d);
}

static void shademode_test(IDirect3DDevice9 *device)
{
    /* Render a quad and try all of the different fixed function shading models. */
    struct IDirect3DVertexBuffer9 *vb_strip = NULL;
    struct IDirect3DVertexBuffer9 *vb_list = NULL;
    HRESULT hr;
    DWORD color0, color1;
    DWORD color0_gouraud = 0, color1_gouraud = 0;
    DWORD shademode = D3DSHADE_FLAT;
    DWORD primtype = D3DPT_TRIANGLESTRIP;
    void *data = NULL;
    UINT i, j;
    struct vertex quad_strip[] =
    {
        {-1.0f, -1.0f,  0.0f, 0xffff0000  },
        {-1.0f,  1.0f,  0.0f, 0xff00ff00  },
        { 1.0f, -1.0f,  0.0f, 0xff0000ff  },
        { 1.0f,  1.0f,  0.0f, 0xffffffff  }
    };
    struct vertex quad_list[] =
    {
        {-1.0f, -1.0f,  0.0f, 0xffff0000  },
        {-1.0f,  1.0f,  0.0f, 0xff00ff00  },
        { 1.0f, -1.0f,  0.0f, 0xff0000ff  },

        {-1.0f,  1.0f,  0.0f, 0xff00ff00  },
        { 1.0f, -1.0f,  0.0f, 0xff0000ff  },
        { 1.0f,  1.0f,  0.0f, 0xffffffff  }
    };

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad_strip),
                                             0, 0, D3DPOOL_MANAGED, &vb_strip, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if (FAILED(hr)) goto bail;

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad_list),
                                             0, 0, D3DPOOL_MANAGED, &vb_list, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if (FAILED(hr)) goto bail;

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(vb_strip, 0, sizeof(quad_strip), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quad_strip, sizeof(quad_strip));
    hr = IDirect3DVertexBuffer9_Unlock(vb_strip);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(vb_list, 0, sizeof(quad_list), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quad_list, sizeof(quad_list));
    hr = IDirect3DVertexBuffer9_Unlock(vb_list);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);

    /* Try it first with a TRIANGLESTRIP.  Do it with different geometry because
     * the color fixups we have to do for FLAT shading will be dependent on that. */
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb_strip, 0, sizeof(quad_strip[0]));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);

    /* First loop uses a TRIANGLESTRIP geometry, 2nd uses a TRIANGLELIST */
    for (j=0; j<2; j++) {

        /* Inner loop just changes the D3DRS_SHADEMODE */
        for (i=0; i<3; i++) {
            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SHADEMODE, shademode);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

            hr = IDirect3DDevice9_BeginScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
            if(SUCCEEDED(hr))
            {
                hr = IDirect3DDevice9_DrawPrimitive(device, primtype, 0, 2);
                ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed with %08x\n", hr);

                hr = IDirect3DDevice9_EndScene(device);
                ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
            }

            /* Sample two spots from the output */
            color0 = getPixelColor(device, 100, 100); /* Inside first triangle */
            color1 = getPixelColor(device, 500, 350); /* Inside second triangle */
            switch(shademode) {
                case D3DSHADE_FLAT:
                    /* Should take the color of the first vertex of each triangle */
                    if (0)
                    {
                        /* This test depends on EXT_provoking_vertex being
                         * available. This extension is currently (20090810)
                         * not common enough to let the test fail if it isn't
                         * present. */
                        ok(color0 == 0x00ff0000, "FLAT shading has color0 %08x, expected 0x00ff0000\n", color0);
                        ok(color1 == 0x0000ff00, "FLAT shading has color1 %08x, expected 0x0000ff00\n", color1);
                    }
                    shademode = D3DSHADE_GOURAUD;
                    break;
                case D3DSHADE_GOURAUD:
                    /* Should be an interpolated blend */

                    ok(color_match(color0, D3DCOLOR_ARGB(0x00, 0x0d, 0xca, 0x28), 2),
                       "GOURAUD shading has color0 %08x, expected 0x00dca28\n", color0);
                    ok(color_match(color1, D3DCOLOR_ARGB(0x00, 0x0d, 0x45, 0xc7), 2),
                       "GOURAUD shading has color1 %08x, expected 0x000d45c7\n", color1);

                    color0_gouraud = color0;
                    color1_gouraud = color1;

                    shademode = D3DSHADE_PHONG;
                    break;
                case D3DSHADE_PHONG:
                    /* Should be the same as GOURAUD, since no hardware implements this */
                    ok(color_match(color0, D3DCOLOR_ARGB(0x00, 0x0d, 0xca, 0x28), 2),
                       "PHONG shading has color0 %08x, expected 0x000dca28\n", color0);
                    ok(color_match(color1, D3DCOLOR_ARGB(0x00, 0x0d, 0x45, 0xc7), 2),
                       "PHONG shading has color1 %08x, expected 0x000d45c7\n", color1);

                    ok(color0 == color0_gouraud, "difference between GOURAUD and PHONG shading detected: %08x %08x\n",
                            color0_gouraud, color0);
                    ok(color1 == color1_gouraud, "difference between GOURAUD and PHONG shading detected: %08x %08x\n",
                            color1_gouraud, color1);
                    break;
            }
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

        /* Now, do it all over again with a TRIANGLELIST */
        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb_list, 0, sizeof(quad_list[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
        primtype = D3DPT_TRIANGLELIST;
        shademode = D3DSHADE_FLAT;
    }

bail:
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    if (vb_strip)
        IDirect3DVertexBuffer9_Release(vb_strip);
    if (vb_list)
        IDirect3DVertexBuffer9_Release(vb_list);
}

static void alpha_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *offscreenTexture;
    IDirect3DSurface9 *backbuffer = NULL, *offscreen = NULL;
    DWORD color;

    struct vertex quad1[] =
    {
        {-1.0f, -1.0f,   0.1f,                          0x4000ff00},
        {-1.0f,  0.0f,   0.1f,                          0x4000ff00},
        { 1.0f, -1.0f,   0.1f,                          0x4000ff00},
        { 1.0f,  0.0f,   0.1f,                          0x4000ff00},
    };
    struct vertex quad2[] =
    {
        {-1.0f,  0.0f,   0.1f,                          0xc00000ff},
        {-1.0f,  1.0f,   0.1f,                          0xc00000ff},
        { 1.0f,  0.0f,   0.1f,                          0xc00000ff},
        { 1.0f,  1.0f,   0.1f,                          0xc00000ff},
    };
    static const float composite_quad[][5] = {
        { 0.0f, -1.0f, 0.1f, 0.0f, 1.0f},
        { 0.0f,  1.0f, 0.1f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.1f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.1f, 1.0f, 0.0f},
    };

    /* Clear the render target with alpha = 0.5 */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %08x\n", hr);
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
    if(IDirect3DDevice9_BeginScene(device) == D3D_OK) {

        /* Draw two quads, one with src alpha blending, one with dest alpha blending. */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

        /* Switch to the offscreen buffer, and redo the testing. The offscreen render target
         * doesn't have an alpha channel. DESTALPHA and INVDESTALPHA "don't work" on render
         * targets without alpha channel, they give essentially ZERO and ONE blend factors. */
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen);
        ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);

        /* Render the offscreen texture onto the frame buffer to be able to compare it regularly.
         * Disable alpha blending for the final composition
         */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) offscreenTexture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, composite_quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);
        hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr = %08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed, hr = %08x\n", hr);
    }

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

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    out:
    /* restore things */
    if(backbuffer) {
        IDirect3DSurface9_Release(backbuffer);
    }
    if(offscreenTexture) {
        IDirect3DTexture9_Release(offscreenTexture);
    }
    if(offscreen) {
        IDirect3DSurface9_Release(offscreen);
    }
}

struct vertex_shortcolor {
    float x, y, z;
    unsigned short r, g, b, a;
};
struct vertex_floatcolor {
    float x, y, z;
    float r, g, b, a;
};

static void fixed_function_decl_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    BOOL s_ok, ub_ok, f_ok;
    DWORD color, size, i;
    void *data;
    static const D3DVERTEXELEMENT9 decl_elements_d3dcolor[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_d3dcolor_2streams[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {1,   0,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_ubyte4n[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_ubyte4n_2streams[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {1,   0,  D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_short4[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_USHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_float[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    static const D3DVERTEXELEMENT9 decl_elements_positiont[] = {
        {0,   0,  D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT,      0},
        {0,  16,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    IDirect3DVertexDeclaration9 *dcl_float = NULL, *dcl_short = NULL, *dcl_ubyte = NULL, *dcl_color = NULL;
    IDirect3DVertexDeclaration9 *dcl_color_2 = NULL, *dcl_ubyte_2 = NULL, *dcl_positiont;
    IDirect3DVertexBuffer9 *vb, *vb2;
    struct vertex quad1[] =                             /* D3DCOLOR */
    {
        {-1.0f, -1.0f,   0.1f,                          0x00ffff00},
        {-1.0f,  0.0f,   0.1f,                          0x00ffff00},
        { 0.0f, -1.0f,   0.1f,                          0x00ffff00},
        { 0.0f,  0.0f,   0.1f,                          0x00ffff00},
    };
    struct vertex quad2[] =                             /* UBYTE4N */
    {
        {-1.0f,  0.0f,   0.1f,                          0x00ffff00},
        {-1.0f,  1.0f,   0.1f,                          0x00ffff00},
        { 0.0f,  0.0f,   0.1f,                          0x00ffff00},
        { 0.0f,  1.0f,   0.1f,                          0x00ffff00},
    };
    struct vertex_shortcolor quad3[] =                  /* short */
    {
        { 0.0f, -1.0f,   0.1f,                          0x0000, 0x0000, 0xffff, 0xffff},
        { 0.0f,  0.0f,   0.1f,                          0x0000, 0x0000, 0xffff, 0xffff},
        { 1.0f, -1.0f,   0.1f,                          0x0000, 0x0000, 0xffff, 0xffff},
        { 1.0f,  0.0f,   0.1f,                          0x0000, 0x0000, 0xffff, 0xffff},
    };
    struct vertex_floatcolor quad4[] =
    {
        { 0.0f,  0.0f,   0.1f,                          1.0, 0.0, 0.0, 0.0},
        { 0.0f,  1.0f,   0.1f,                          1.0, 0.0, 0.0, 0.0},
        { 1.0f,  0.0f,   0.1f,                          1.0, 0.0, 0.0, 0.0},
        { 1.0f,  1.0f,   0.1f,                          1.0, 0.0, 0.0, 0.0},
    };
    DWORD colors[] = {
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
        0x00ff0000,   0x0000ff00,     0x000000ff,   0x00ffffff,
    };
    float quads[] = {
        -1.0,   -1.0,     0.1,
        -1.0,    0.0,     0.1,
         0.0,   -1.0,     0.1,
         0.0,    0.0,     0.1,

         0.0,   -1.0,     0.1,
         0.0,    0.0,     0.1,
         1.0,   -1.0,     0.1,
         1.0,    0.0,     0.1,

         0.0,    0.0,     0.1,
         0.0,    1.0,     0.1,
         1.0,    0.0,     0.1,
         1.0,    1.0,     0.1,

        -1.0,    0.0,     0.1,
        -1.0,    1.0,     0.1,
         0.0,    0.0,     0.1,
         0.0,    1.0,     0.1
    };
    struct tvertex quad_transformed[] = {
       {  90,    110,     0.1,      2.0,        0x00ffff00},
       { 570,    110,     0.1,      2.0,        0x00ffff00},
       {  90,    300,     0.1,      2.0,        0x00ffff00},
       { 570,    300,     0.1,      2.0,        0x00ffff00}
    };
    D3DCAPS9 caps;

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "GetDeviceCaps failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_d3dcolor, &dcl_color);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_short4, &dcl_short);
    ok(SUCCEEDED(hr) || hr == E_FAIL, "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_float, &dcl_float);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    if(caps.DeclTypes & D3DDTCAPS_UBYTE4N) {
        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_ubyte4n_2streams, &dcl_ubyte_2);
        ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_ubyte4n, &dcl_ubyte);
        ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    } else {
        trace("D3DDTCAPS_UBYTE4N not supported\n");
        dcl_ubyte_2 = NULL;
        dcl_ubyte = NULL;
    }
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_d3dcolor_2streams, &dcl_color_2);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_positiont, &dcl_positiont);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);

    size = max(sizeof(quad1), max(sizeof(quad2), max(sizeof(quad3), max(sizeof(quad4), sizeof(quads)))));
    hr = IDirect3DDevice9_CreateVertexBuffer(device, size,
                                             0, 0, D3DPOOL_MANAGED, &vb, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    f_ok = FALSE; s_ok = FALSE; ub_ok = FALSE;
    if(SUCCEEDED(hr)) {
        if(dcl_color) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);
        }

        /* Tests with non-standard fixed function types fail on the refrast. The ATI driver partially
         * accepts them, the nvidia driver accepts them all. All those differences even though we're
         * using software vertex processing. Doh!
         */
        if(dcl_ubyte) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "DrawPrimitiveUP failed, hr = %#08x\n", hr);
            ub_ok = SUCCEEDED(hr);
        }

        if(dcl_short) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_short);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(quad3[0]));
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "DrawPrimitiveUP failed, hr = %#08x\n", hr);
            s_ok = SUCCEEDED(hr);
        }

        if(dcl_float) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_float);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(quad4[0]));
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "DrawPrimitiveUP failed, hr = %#08x\n", hr);
            f_ok = SUCCEEDED(hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr = %#08x\n", hr);
    }

    if(dcl_short) {
        color = getPixelColor(device, 480, 360);
        ok(color == 0x000000ff || !s_ok,
           "D3DDECLTYPE_USHORT4N returned color %08x, expected 0x000000ff\n", color);
    }
    if(dcl_ubyte) {
        color = getPixelColor(device, 160, 120);
        ok(color == 0x0000ffff || !ub_ok,
           "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x0000ffff\n", color);
    }
    if(dcl_color) {
        color = getPixelColor(device, 160, 360);
        ok(color == 0x00ffff00,
           "D3DDECLTYPE_D3DCOLOR returned color %08x, expected 0x00ffff00\n", color);
    }
    if(dcl_float) {
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00ff0000 || !f_ok,
           "D3DDECLTYPE_FLOAT4 returned color %08x, expected 0x00ff0000\n", color);
    }
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* The following test with vertex buffers doesn't serve to find out new information from windows.
     * It is a plain regression test because wined3d uses different codepaths for attribute conversion
     * with vertex buffers. It makes sure that the vertex buffer one works, while the above tests
     * whether the immediate mode code works
     */
    f_ok = FALSE; s_ok = FALSE; ub_ok = FALSE;
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        if(dcl_color) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad1), &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
            memcpy(data, quad1, sizeof(quad1));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad1[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
        }

        if(dcl_ubyte) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad2), &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
            memcpy(data, quad2, sizeof(quad2));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad2[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ub_ok = SUCCEEDED(hr);
        }

        if(dcl_short) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad3), &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
            memcpy(data, quad3, sizeof(quad3));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_short);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad3[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            s_ok = SUCCEEDED(hr);
        }

        if(dcl_float) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad4), &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
            memcpy(data, quad4, sizeof(quad4));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_float);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad4[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            f_ok = SUCCEEDED(hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr = %#08x\n", hr);
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);

    if(dcl_short) {
        color = getPixelColor(device, 480, 360);
        ok(color == 0x000000ff || !s_ok,
           "D3DDECLTYPE_USHORT4N returned color %08x, expected 0x000000ff\n", color);
    }
    if(dcl_ubyte) {
        color = getPixelColor(device, 160, 120);
        ok(color == 0x0000ffff || !ub_ok,
           "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x0000ffff\n", color);
    }
    if(dcl_color) {
        color = getPixelColor(device, 160, 360);
        ok(color == 0x00ffff00,
           "D3DDECLTYPE_D3DCOLOR returned color %08x, expected 0x00ffff00\n", color);
    }
    if(dcl_float) {
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00ff0000 || !f_ok,
           "D3DDECLTYPE_FLOAT4 returned color %08x, expected 0x00ff0000\n", color);
    }
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad_transformed), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quad_transformed, sizeof(quad_transformed));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_positiont);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad_transformed[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
    }

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

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* This test is pointless without those two declarations: */
    if((!dcl_color_2) || (!dcl_ubyte_2)) {
        skip("color-ubyte switching test declarations aren't supported\n");
        goto out;
    }

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quads), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quads, sizeof(quads));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(colors),
                                             0, 0, D3DPOOL_MANAGED, &vb2, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb2, 0, sizeof(colors), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, colors, sizeof(colors));
    hr = IDirect3DVertexBuffer9_Unlock(vb2);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);

    for(i = 0; i < 2; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
        if(i == 0) {
            hr = IDirect3DDevice9_SetStreamSource(device, 1, vb2, 0, sizeof(DWORD) * 4);
        } else {
            hr = IDirect3DDevice9_SetStreamSource(device, 1, vb2, 8, sizeof(DWORD) * 4);
        }
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
        ub_ok = FALSE;
        if(SUCCEEDED(hr)) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ub_ok = SUCCEEDED(hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_color_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 4, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 8, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ub_ok = (SUCCEEDED(hr) && ub_ok);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
        }

        if(i == 0) {
            color = getPixelColor(device, 480, 360);
            ok(color == 0x00ff0000,
               "D3DDECLTYPE_D3DCOLOR returned color %08x, expected 0x00ff0000\n", color);
            color = getPixelColor(device, 160, 120);
            ok(color == 0x00ffffff,
                "Unused quad returned color %08x, expected 0x00ffffff\n", color);
            color = getPixelColor(device, 160, 360);
            ok(color == 0x000000ff || !ub_ok,
               "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x000000ff\n", color);
            color = getPixelColor(device, 480, 120);
            ok(color == 0x000000ff || !ub_ok,
               "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x000000ff\n", color);
        } else {
            color = getPixelColor(device, 480, 360);
            ok(color == 0x000000ff,
               "D3DDECLTYPE_D3DCOLOR returned color %08x, expected 0x000000ff\n", color);
            color = getPixelColor(device, 160, 120);
            ok(color == 0x00ffffff,
               "Unused quad returned color %08x, expected 0x00ffffff\n", color);
            color = getPixelColor(device, 160, 360);
            ok(color == 0x00ff0000 || !ub_ok,
               "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x00ff0000\n", color);
            color = getPixelColor(device, 480, 120);
            ok(color == 0x00ff0000 || !ub_ok,
               "D3DDECLTYPE_UBYTE4N returned color %08x, expected 0x00ff0000\n", color);
        }
        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 1, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x\n", hr);
    IDirect3DVertexBuffer9_Release(vb2);

    out:
    IDirect3DVertexBuffer9_Release(vb);
    if(dcl_float) IDirect3DVertexDeclaration9_Release(dcl_float);
    if(dcl_short) IDirect3DVertexDeclaration9_Release(dcl_short);
    if(dcl_ubyte) IDirect3DVertexDeclaration9_Release(dcl_ubyte);
    if(dcl_color) IDirect3DVertexDeclaration9_Release(dcl_color);
    if(dcl_color_2) IDirect3DVertexDeclaration9_Release(dcl_color_2);
    if(dcl_ubyte_2) IDirect3DVertexDeclaration9_Release(dcl_ubyte_2);
    if(dcl_positiont) IDirect3DVertexDeclaration9_Release(dcl_positiont);
}

struct vertex_float16color {
    float x, y, z;
    DWORD c1, c2;
};

static void test_vshader_float16(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    void *data;
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
        {0,  12,  D3DDECLTYPE_FLOAT16_4,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,          0},
        D3DDECL_END()
    };
    IDirect3DVertexDeclaration9 *vdecl = NULL;
    IDirect3DVertexBuffer9 *buffer = NULL;
    IDirect3DVertexShader9 *shader;
    static const DWORD shader_code[] =
    {
        0xfffe0101,                             /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,     /* dcl_position v0  */
        0x0000001f, 0x8000000a, 0x900f0001,     /* dcl_color v1     */
        0x00000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0     */
        0x00000001, 0xd00f0000, 0x90e40001,     /* mov oD0, v1      */
        0x0000ffff,
    };
    struct vertex_float16color quad[] = {
        { -1.0,   -1.0,     0.1,        0x3c000000, 0x00000000 }, /* green */
        { -1.0,    0.0,     0.1,        0x3c000000, 0x00000000 },
        {  0.0,   -1.0,     0.1,        0x3c000000, 0x00000000 },
        {  0.0,    0.0,     0.1,        0x3c000000, 0x00000000 },

        {  0.0,   -1.0,     0.1,        0x00003c00, 0x00000000 }, /* red */
        {  0.0,    0.0,     0.1,        0x00003c00, 0x00000000 },
        {  1.0,   -1.0,     0.1,        0x00003c00, 0x00000000 },
        {  1.0,    0.0,     0.1,        0x00003c00, 0x00000000 },

        {  0.0,    0.0,     0.1,        0x00000000, 0x00003c00 }, /* blue */
        {  0.0,    1.0,     0.1,        0x00000000, 0x00003c00 },
        {  1.0,    0.0,     0.1,        0x00000000, 0x00003c00 },
        {  1.0,    1.0,     0.1,        0x00000000, 0x00003c00 },

        { -1.0,    0.0,     0.1,        0x00000000, 0x3c000000 }, /* alpha */
        { -1.0,    1.0,     0.1,        0x00000000, 0x3c000000 },
        {  0.0,    0.0,     0.1,        0x00000000, 0x3c000000 },
        {  0.0,    1.0,     0.1,        0x00000000, 0x3c000000 },
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff102030, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vdecl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexDeclaration failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetVertexDeclaration(device, vdecl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  0, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  4, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  8, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad + 12, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%08x\n", hr);
    }
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ff0000,
       "Input 0x00003c00, 0x00000000 returned color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000,
       "Input 0x00000000, 0x3c000000 returned color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000ff00,
       "Input 0x3c000000, 0x00000000 returned color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff,
       "Input 0x00000000, 0x00003c00 returned color %08x, expected 0x000000ff\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff102030, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, 0,
                                             D3DPOOL_MANAGED, &buffer, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexBuffer failed, hr=%08x\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(quad), &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed, hr=%08x\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(quad[0]));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (%08x)\n", hr);
    if(SUCCEEDED(hr)) {
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,  0, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,  4, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,  8, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 12, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%08x\n", hr);
    }

    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ff0000,
       "Input 0x00003c00, 0x00000000 returned color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000,
       "Input 0x00000000, 0x3c000000 returned color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000ff00,
       "Input 0x3c000000, 0x00000000 returned color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff,
       "Input 0x00000000, 0x00003c00 returned color %08x, expected 0x000000ff\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);

    IDirect3DVertexDeclaration9_Release(vdecl);
    IDirect3DVertexShader9_Release(shader);
    IDirect3DVertexBuffer9_Release(buffer);
}

static void conditional_np2_repeat_test(IDirect3DDevice9 *device)
{
    D3DCAPS9 caps;
    IDirect3DTexture9 *texture;
    HRESULT hr;
    D3DLOCKED_RECT rect;
    unsigned int x, y;
    DWORD *dst, color;
    const float quad[] = {
        -1.0,   -1.0,   0.1,   -0.2,   -0.2,
         1.0,   -1.0,   0.1,    1.2,   -0.2,
        -1.0,    1.0,   0.1,   -0.2,    1.2,
         1.0,    1.0,   0.1,    1.2,    1.2
    };
    memset(&caps, 0, sizeof(caps));

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed hr=%08x\n", hr);
    if (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
    {
        /* NP2 conditional requires the POW2 flag. Check that while we're at it */
        ok(caps.TextureCaps & D3DPTEXTURECAPS_POW2,
                "Card has conditional NP2 support without power of two restriction set\n");
    }
    else if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No conditional NP2 support, skipping conditional NP2 tests\n");
        return;
    }
    else
    {
        skip("Card has unconditional NP2 support, skipping conditional NP2 tests\n");
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 10, 10, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%08x\n", hr);

    memset(&rect, 0, sizeof(rect));
    hr = IDirect3DTexture9_LockRect(texture, 0, &rect, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed hr=%08x\n", hr);
    for(y = 0; y < 10; y++) {
        for(x = 0; x < 10; x++) {
            dst = (DWORD *) ((BYTE *) rect.pBits + y * rect.Pitch + x * sizeof(DWORD));
            if(x == 0 || x == 9 || y == 0 || y == 9) {
                *dst = 0x00ff0000;
            } else {
                *dst = 0x000000ff;
            }
        }
    }
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }

    color = getPixelColor(device,    1,  1);
    ok(color == 0x00ff0000, "NP2: Pixel   1,  1 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 639, 479);
    ok(color == 0x00ff0000, "NP2: Pixel 639, 479 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 135, 101);
    ok(color == 0x00ff0000, "NP2: Pixel 135, 101 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 140, 101);
    ok(color == 0x00ff0000, "NP2: Pixel 140, 101 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 135, 105);
    ok(color == 0x00ff0000, "NP2: Pixel 135, 105 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 140, 105);
    ok(color == 0x000000ff, "NP2: Pixel 140, 105 has color %08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 135, 376);
    ok(color == 0x00ff0000, "NP2: Pixel 135, 376 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 140, 376);
    ok(color == 0x000000ff, "NP2: Pixel 140, 376 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 135, 379);
    ok(color == 0x00ff0000, "NP2: Pixel 135, 379 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 140, 379);
    ok(color == 0x00ff0000, "NP2: Pixel 140, 379 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 500, 101);
    ok(color == 0x00ff0000, "NP2: Pixel 500, 101 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 504, 101);
    ok(color == 0x00ff0000, "NP2: Pixel 504, 101 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 500, 105);
    ok(color == 0x000000ff, "NP2: Pixel 500, 105 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 504, 105);
    ok(color == 0x00ff0000, "NP2: Pixel 504, 105 has color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 500, 376);
    ok(color == 0x000000ff, "NP2: Pixel 500, 376 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 504, 376);
    ok(color == 0x00ff0000, "NP2: Pixel 504, 376 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 500, 380);
    ok(color == 0x00ff0000, "NP2: Pixel 500, 380 has color %08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 504, 380);
    ok(color == 0x00ff0000, "NP2: Pixel 504, 380 has color %08x, expected 0x00ff0000\n", color);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    IDirect3DTexture9_Release(texture);
}

static void vface_register_test(void)
{
    IDirect3DSurface9 *surface, *backbuffer;
    IDirect3DVertexShader9 *vshader;
    IDirect3DPixelShader9 *shader;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const DWORD shader_code[] =
    {
        0xffff0300,                                                             /* ps_3_0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c0, 0.0, 1.0, 0.0, 0.0 */
        0x05000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1, 1.0, 0.0, 0.0, 0.0 */
        0x0200001f, 0x80000000, 0x900f1001,                                     /* dcl vFace                  */
        0x02000001, 0x800f0001, 0xa0e40001,                                     /* mov r1, c1                 */
        0x04000058, 0x800f0000, 0x90e41001, 0xa0e40000, 0x80e40001,             /* cmp r0, vFace, c0, r1      */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                */
        0x0000ffff                                                              /* END                        */
    };
    static const DWORD vshader_code[] =
    {
        0xfffe0300,                                                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,                                     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,                                     /* mov o0, v0           */
        0x0000ffff                                                              /* end                  */
    };
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
        -1.0f,  0.0f, 0.1f,

         1.0f, -1.0f, 0.1f,
         1.0f,  0.0f, 0.1f,
        -1.0f,  0.0f, 0.1f,

        -1.0f,  0.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f,  0.0f, 0.1f,

         1.0f,  0.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f,  1.0f, 0.1f,
    };
    static const float blit[] =
    {
         0.0f, -1.0f, 0.1f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.1f, 1.0f, 0.0f,
         0.0f,  1.0f, 0.1f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.1f, 1.0f, 1.0f,
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        skip("No shader model 3 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device, vshader_code, &vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%08x\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Failed to set cull mode, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        /* First, draw to the texture and the back buffer to test both offscreen and onscreen cases */
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 4, quad, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 4, quad, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        /* Blit the texture onto the back buffer to make it visible */
        hr = IDirect3DDevice9_SetVertexShader(device, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%08x\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, blit, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "vFace: Onscreen rendered front facing quad has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000ff00, "vFace: Onscreen rendered back facing quad has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000ff00, "vFace: Offscreen rendered back facing quad has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff0000, "vFace: Offscreen rendered front facing quad has color 0x%08x, expected 0x00ff0000\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    IDirect3DPixelShader9_Release(shader);
    IDirect3DVertexShader9_Release(vshader);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(backbuffer);
    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void fixed_function_bumpmap_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    int i;
    D3DCAPS9 caps;
    BOOL L6V5U5_supported = FALSE;
    IDirect3DTexture9 *tex1, *tex2;
    D3DLOCKED_RECT locked_rect;

    static const float quad[][7] = {
        {-128.0f/640.0f, -128.0f/480.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f},
        {-128.0f/640.0f,  128.0f/480.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f},
        { 128.0f/640.0f, -128.0f/480.0f, 0.1f, 1.0f, 0.0f, 1.0f, 0.0f},
        { 128.0f/640.0f,  128.0f/480.0f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    };

    /* use asymmetric matrix to test loading */
    float bumpenvmat[4] = {0.0,0.5,-0.5,0.0};
    float scale, offset;

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DTexture9           *texture            = NULL;

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed hr=%08x\n", hr);
    if(!(caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAP)) {
        skip("D3DTEXOPCAPS_BUMPENVMAP not set, skipping bumpmap tests\n");
        return;
    } else {
        /* This check is disabled, some Windows drivers do not handle D3DUSAGE_QUERY_LEGACYBUMPMAP properly.
         * They report that it is not supported, but after that bump mapping works properly. So just test
         * if the format is generally supported, and check the BUMPENVMAP flag
         */
        IDirect3D9 *d3d9;

        IDirect3DDevice9_GetDirect3D(device, &d3d9);
        hr = IDirect3D9_CheckDeviceFormat(d3d9, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                                          D3DRTYPE_TEXTURE, D3DFMT_L6V5U5);
        L6V5U5_supported = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d9, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                                          D3DRTYPE_TEXTURE, D3DFMT_V8U8);
        IDirect3D9_Release(d3d9);
        if(FAILED(hr)) {
            skip("D3DFMT_V8U8 not supported for legacy bump mapping\n");
            return;
        }
    }

    /* Generate the textures */
    generate_bumpmap_textures(device);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVMAT00, *(LPDWORD)&bumpenvmat[0]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVMAT01, *(LPDWORD)&bumpenvmat[1]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVMAT10, *(LPDWORD)&bumpenvmat[2]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVMAT11, *(LPDWORD)&bumpenvmat[3]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_BUMPENVMAP);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT );
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);


    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

    /* on MacOS(10.5.4, radeon X1600), the white dots are have color 0x00fbfbfb rather than 0x00ffffff. This is
     * kinda strange since no calculations are done on the sampled colors, only on the texture coordinates.
     * But since testing the color match is not the purpose of the test don't be too picky
     */
    color = getPixelColor(device, 320-32, 240);
    ok(color_match(color, 0x00ffffff, 4), "bumpmap failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 320+32, 240);
    ok(color_match(color, 0x00ffffff, 4), "bumpmap failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 320, 240-32);
    ok(color_match(color, 0x00ffffff, 4), "bumpmap failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 320, 240+32);
    ok(color_match(color, 0x00ffffff, 4), "bumpmap failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00000000, 4), "bumpmap failed: Got color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 320+32, 240+32);
    ok(color_match(color, 0x00000000, 4), "bumpmap failed: Got color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 320-32, 240+32);
    ok(color_match(color, 0x00000000, 4), "bumpmap failed: Got color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 320+32, 240-32);
    ok(color_match(color, 0x00000000, 4), "bumpmap failed: Got color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 320-32, 240-32);
    ok(color_match(color, 0x00000000, 4), "bumpmap failed: Got color 0x%08x, expected 0x00000000.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    for(i = 0; i < 2; i++) {
        hr = IDirect3DDevice9_GetTexture(device, i, (IDirect3DBaseTexture9 **) &texture);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_GetTexture failed (0x%08x)\n", hr);
        IDirect3DTexture9_Release(texture); /* For the GetTexture */
        hr = IDirect3DDevice9_SetTexture(device, i, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);
        IDirect3DTexture9_Release(texture); /* To destroy it */
    }

    if(!(caps.TextureOpCaps & D3DTEXOPCAPS_BUMPENVMAPLUMINANCE)) {
        skip("D3DTOP_BUMPENVMAPLUMINANCE not supported, skipping\n");
        goto cleanup;
    }
    if(L6V5U5_supported == FALSE) {
        skip("L6V5U5_supported not supported, skipping D3DTOP_BUMPENVMAPLUMINANCE test\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 0.0, 0x8);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    /* This test only tests the luminance part. The bumpmapping part was already tested above and
     * would only make this test more complicated
     */
    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_L6V5U5, D3DPOOL_MANAGED, &tex1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &tex2, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed, hr=%08x\n", hr);

    memset(&locked_rect, 0, sizeof(locked_rect));
    hr = IDirect3DTexture9_LockRect(tex1, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    *((DWORD *)locked_rect.pBits) = 0x4000; /* L = 0.25, V = 0.0, U = 0.0 */
    hr = IDirect3DTexture9_UnlockRect(tex1, 0);
    ok(SUCCEEDED(hr), "UnlockRect failed with 0x%08x\n", hr);

    memset(&locked_rect, 0, sizeof(locked_rect));
    hr = IDirect3DTexture9_LockRect(tex2, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    *((DWORD *)locked_rect.pBits) = 0x00ff80c0;
    hr = IDirect3DTexture9_UnlockRect(tex2, 0);
    ok(SUCCEEDED(hr), "UnlockRect failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) tex1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, (IDirect3DBaseTexture9 *) tex2);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_BUMPENVMAPLUMINANCE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    scale = 2.0;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLSCALE, *((DWORD *)&scale));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    offset = 0.1;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLOFFSET, *((DWORD *)&offset));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);
    }

    color = getPixelColor(device, 320, 240);
    /* red:   1.0  * (0.25 * 2.0 + 0.1) = 1.0  * 0.6 = 0.6  = 0x99
     * green: 0.5  * (0.25 * 2.0 + 0.1) = 0.5  * 0.6 = 0.3  = 0x4c
     * green: 0.75 * (0.25 * 2.0 + 0.1) = 0.75 * 0.6 = 0.45 = 0x72
     */
    ok(color_match(color, 0x00994c72, 5), "bumpmap failed: Got color 0x%08x, expected 0x00994c72.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Check a result scale factor > 1.0 */
    scale = 10;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLSCALE, *((DWORD *)&scale));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    offset = 10;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLOFFSET, *((DWORD *)&offset));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff80c0, 1), "bumpmap failed: Got color 0x%08x, expected 0x00ff80c0.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Check clamping in the scale factor calculation */
    scale = 1000;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLSCALE, *((DWORD *)&scale));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    offset = -1;
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_BUMPENVLOFFSET, *((DWORD *)&offset));
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff80c0, 1), "bumpmap failed: Got color 0x%08x, expected 0x00ff80c0.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTexture failed (%08x)\n", hr);

    IDirect3DTexture9_Release(tex1);
    IDirect3DTexture9_Release(tex2);

cleanup:
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
}

static void stencil_cull_test(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    static const float quad1[] =
    {
        -1.0,   -1.0,   0.1,
         0.0,   -1.0,   0.1,
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1,
    };
    static const float quad2[] =
    {
         0.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
         0.0,    0.0,   0.1,
         1.0,    0.0,   0.1,
    };
    static const float quad3[] =
    {
        0.0,    0.0,   0.1,
        1.0,    0.0,   0.1,
        0.0,    1.0,   0.1,
        1.0,    1.0,   0.1,
    };
    static const float quad4[] =
    {
        -1.0,    0.0,   0.1,
         0.0,    0.0,   0.1,
        -1.0,    1.0,   0.1,
         0.0,    1.0,   0.1,
    };
    struct vertex painter[] =
    {
       {-1.0,   -1.0,   0.0,    0x00000000},
       { 1.0,   -1.0,   0.0,    0x00000000},
       {-1.0,    1.0,   0.0,    0x00000000},
       { 1.0,    1.0,   0.0,    0x00000000},
    };
    static const WORD indices_cw[]  = {0, 1, 3};
    static const WORD indices_ccw[] = {0, 2, 3};
    unsigned int i;
    DWORD color;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Cannot create a device with a D24S8 stencil buffer.\n");
        DestroyWindow(window);
        IDirect3D9_Release(d3d);
        return;
    }
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.StencilCaps & D3DSTENCILCAPS_TWOSIDED))
    {
        skip("No two sided stencil support\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_STENCIL, 0x00ff0000, 0.0, 0x8);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF,hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "Failed to disable Z test, %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Failed to disable lighting, %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILFAIL, D3DSTENCILOP_INCR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILREF, 0x3);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_REPLACE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_DECR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CCW_STENCILPASS, D3DSTENCILOP_INCR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TWOSIDEDSTENCILMODE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    /* First pass: Fill the stencil buffer with some values... */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CW);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_cw, D3DFMT_INDEX16, quad1, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_ccw, D3DFMT_INDEX16, quad1, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TWOSIDEDSTENCILMODE, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_cw, D3DFMT_INDEX16, quad2, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_ccw, D3DFMT_INDEX16, quad2, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CW);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_cw, D3DFMT_INDEX16, quad3, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_ccw, D3DFMT_INDEX16, quad3, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_CCW);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_cw, D3DFMT_INDEX16, quad4, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                1 /*PrimCount */, indices_ccw, D3DFMT_INDEX16, quad4, sizeof(float) * 3);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawIndexedPrimitiveUP returned %#x.\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TWOSIDEDSTENCILMODE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    /* 2nd pass: Make the stencil values visible */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
        ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
        for (i = 0; i < 16; ++i)
        {
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILREF, i);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

            painter[0].diffuse = (i * 16); /* Creates shades of blue */
            painter[1].diffuse = (i * 16);
            painter[2].diffuse = (i * 16);
            painter[3].diffuse = (i * 16);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, painter, sizeof(painter[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    color = getPixelColor(device, 160, 420);
    ok(color == 0x00000030, "CCW triangle, twoside FALSE, cull cw, replace, has color 0x%08x, expected 0x00000030\n", color);
    color = getPixelColor(device, 160, 300);
    ok(color == 0x00000080, "CW triangle, twoside FALSE, cull cw, culled, has color 0x%08x, expected 0x00000080\n", color);

    color = getPixelColor(device, 480, 420);
    ok(color == 0x00000090, "CCW triangle, twoside TRUE, cull off, incr, has color 0x%08x, expected 0x00000090\n", color);
    color = getPixelColor(device, 480, 300);
    ok(color == 0x00000030, "CW triangle, twoside TRUE, cull off, replace, has color 0x%08x, expected 0x00000030\n", color);

    color = getPixelColor(device, 160, 180);
    ok(color == 0x00000080, "CCW triangle, twoside TRUE, cull ccw, culled, has color 0x%08x, expected 0x00000080\n", color);
    color = getPixelColor(device, 160, 60);
    ok(color == 0x00000030, "CW triangle, twoside TRUE, cull ccw, replace, has color 0x%08x, expected 0x00000030\n", color);

    color = getPixelColor(device, 480, 180);
    ok(color == 0x00000090, "CCW triangle, twoside TRUE, cull cw, incr, has color 0x%08x, expected 0x00000090\n", color);
    color = getPixelColor(device, 480, 60);
    ok(color == 0x00000080, "CW triangle, twoside TRUE, cull cw, culled, has color 0x%08x, expected 0x00000080\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

cleanup:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void vpos_register_test(void)
{
    IDirect3DSurface9 *surface = NULL, *backbuffer;
    IDirect3DPixelShader9 *shader, *shader_frac;
    IDirect3DVertexShader9 *vshader;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;
    DWORD *pos;

    static const DWORD shader_code[] =
    {
        0xffff0300,                                                             /* ps_3_0                     */
        0x0200001f, 0x80000000, 0x90031000,                                     /* dcl vPos.xy                */
        0x03000002, 0x80030000, 0x90541000, 0xa1fe0000,                         /* sub r0.xy, vPos.xy, c0.zw  */
        0x02000001, 0x800f0001, 0xa0e40000,                                     /* mov r1, c0                 */
        0x02000001, 0x80080002, 0xa0550000,                                     /* mov r2.a, c0.y             */
        0x02000001, 0x80010002, 0xa0550000,                                     /* mov r2.r, c0.y             */
        0x04000058, 0x80020002, 0x80000000, 0x80000001, 0x80550001,             /* cmp r2.g, r0.x, r1.x, r1.y */
        0x04000058, 0x80040002, 0x80550000, 0x80000001, 0x80550001,             /* cmp r2.b, r0.y, r1.x, r1.y */
        0x02000001, 0x800f0800, 0x80e40002,                                     /* mov oC0, r2                */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD shader_frac_code[] =
    {
        0xffff0300,                                                             /* ps_3_0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.0, 0.0, 0.0, 0.0 */
        0x0200001f, 0x80000000, 0x90031000,                                     /* dcl vPos.xy                */
        0x02000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                 */
        0x02000013, 0x80030000, 0x90541000,                                     /* frc r0.xy, vPos.xy         */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD vshader_code[] =
    {
        0xfffe0300,                                                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,                                     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,                                     /* mov o0, v0           */
        0x0000ffff                                                              /* end                  */
    };
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.1f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.1f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.1f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.1f, 1.0f, 1.0f,
    };
    float constant[4] = {1.0, 0.0, 320, 240};

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        skip("No shader model 3 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vshader_code, &vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_frac_code, &shader_frac);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, constant, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF failed hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }

    /* This has to be pixel exact */
    color = getPixelColor(device, 319, 239);
    ok(color == 0x00000000, "vPos: Pixel 319,239 has color 0x%08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 320, 239);
    ok(color == 0x0000ff00, "vPos: Pixel 320,239 has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 319, 240);
    ok(color == 0x000000ff, "vPos: Pixel 319,240 has color 0x%08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 320, 240);
    ok(color == 0x0000ffff, "vPos: Pixel 320,240 has color 0x%08x, expected 0x0000ffff\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32, D3DFMT_X8R8G8B8, 0, 0, TRUE,
                                             &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateRenderTarget failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        constant[2] = 16; constant[3] = 16;
        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, constant, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF failed hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }
    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect failed, hr=%08x\n", hr);

    pos = (DWORD *) (((BYTE *) lr.pBits) + 14 * lr.Pitch + 14 * sizeof(DWORD));
    color = *pos & 0x00ffffff;
    ok(color == 0x00000000, "Pixel 14/14 has color 0x%08x, expected 0x00000000\n", color);
    pos = (DWORD *) (((BYTE *) lr.pBits) + 14 * lr.Pitch + 18 * sizeof(DWORD));
    color = *pos & 0x00ffffff;
    ok(color == 0x0000ff00, "Pixel 14/18 has color 0x%08x, expected 0x0000ff00\n", color);
    pos = (DWORD *) (((BYTE *) lr.pBits) + 18 * lr.Pitch + 14 * sizeof(DWORD));
    color = *pos & 0x00ffffff;
    ok(color == 0x000000ff, "Pixel 18/14 has color 0x%08x, expected 0x000000ff\n", color);
    pos = (DWORD *) (((BYTE *) lr.pBits) + 18 * lr.Pitch + 18 * sizeof(DWORD));
    color = *pos & 0x00ffffff;
    ok(color == 0x0000ffff, "Pixel 18/18 has color 0x%08x, expected 0x0000ffff\n", color);

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect failed, hr=%08x\n", hr);

    /* Test the fraction value of vPos. This is tested with the offscreen target and not the backbuffer to
     * have full control over the multisampling setting inside this test
     */
    hr = IDirect3DDevice9_SetPixelShader(device, shader_frac);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect failed, hr=%08x\n", hr);

    pos = (DWORD *) (((BYTE *) lr.pBits) + 14 * lr.Pitch + 14 * sizeof(DWORD));
    color = *pos & 0x00ffffff;
    ok(color == 0x00000000, "vPos fraction test has color 0x%08x, expected 0x00000000\n", color);

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);
    IDirect3DPixelShader9_Release(shader);
    IDirect3DPixelShader9_Release(shader_frac);
    IDirect3DVertexShader9_Release(vshader);
    if(surface) IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(backbuffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static BOOL point_match(IDirect3DDevice9 *device, UINT x, UINT y, UINT r)
{
    D3DCOLOR color;

    color = D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff);
    if (!color_match(getPixelColor(device, x + r, y), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x - r, y), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x, y + r), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x, y - r), color, 1)) return FALSE;

    ++r;
    color = D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0xff);
    if (!color_match(getPixelColor(device, x + r, y), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x - r, y), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x, y + r), color, 1)) return FALSE;
    if (!color_match(getPixelColor(device, x, y - r), color, 1)) return FALSE;

    return TRUE;
}

static void pointsize_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    D3DCAPS9 caps;
    D3DMATRIX matrix;
    D3DMATRIX identity;
    float ptsize, ptsize_orig, ptsizemax_orig, ptsizemin_orig;
    DWORD color;
    IDirect3DSurface9 *rt, *backbuffer;
    IDirect3DTexture9 *tex1, *tex2;
    RECT rect = {0, 0, 128, 128};
    D3DLOCKED_RECT lr;
    const DWORD tex1_data[4] = {0x00ff0000, 0x00ff0000,
                                0x00000000, 0x00000000};
    const DWORD tex2_data[4] = {0x00000000, 0x0000ff00,
                                0x00000000, 0x0000ff00};

    const float vertices[] = {
        64,     64,     0.1,
        128,    64,     0.1,
        192,    64,     0.1,
        256,    64,     0.1,
        320,    64,     0.1,
        384,    64,     0.1,
        448,    64,     0.1,
        512,    64,     0.1,
    };

    /* Transforms the coordinate system [-1.0;1.0]x[-1.0;1.0] to [0.0;0.0]x[640.0;480.0]. Z is untouched */
    U(matrix).m[0][0] = 2.0/640.0; U(matrix).m[1][0] = 0.0;       U(matrix).m[2][0] = 0.0;   U(matrix).m[3][0] =-1.0;
    U(matrix).m[0][1] = 0.0;       U(matrix).m[1][1] =-2.0/480.0; U(matrix).m[2][1] = 0.0;   U(matrix).m[3][1] = 1.0;
    U(matrix).m[0][2] = 0.0;       U(matrix).m[1][2] = 0.0;       U(matrix).m[2][2] = 1.0;   U(matrix).m[3][2] = 0.0;
    U(matrix).m[0][3] = 0.0;       U(matrix).m[1][3] = 0.0;       U(matrix).m[2][3] = 0.0;   U(matrix).m[3][3] = 1.0;

    U(identity).m[0][0] = 1.0;     U(identity).m[1][0] = 0.0;     U(identity).m[2][0] = 0.0; U(identity).m[3][0] = 0.0;
    U(identity).m[0][1] = 0.0;     U(identity).m[1][1] = 1.0;     U(identity).m[2][1] = 0.0; U(identity).m[3][1] = 0.0;
    U(identity).m[0][2] = 0.0;     U(identity).m[1][2] = 0.0;     U(identity).m[2][2] = 1.0; U(identity).m[3][2] = 0.0;
    U(identity).m[0][3] = 0.0;     U(identity).m[1][3] = 0.0;     U(identity).m[2][3] = 0.0; U(identity).m[3][3] = 1.0;

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed hr=%08x\n", hr);
    if(caps.MaxPointSize < 32.0) {
        skip("MaxPointSize < 32.0, skipping(MaxPointsize = %f)\n", caps.MaxPointSize);
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &matrix);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_POINTSIZE, (DWORD *) &ptsize_orig);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%08x\n", hr);
    if (SUCCEEDED(hr))
    {
        ptsize = 15.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[0], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        ptsize = 31.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[3], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        ptsize = 30.75;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[6], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        if (caps.MaxPointSize >= 63.0)
        {
            ptsize = 63.0;
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[9], sizeof(float) * 3);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

            ptsize = 62.75;
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[15], sizeof(float) * 3);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        }

        ptsize = 1.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[12], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_GetRenderState(device, D3DRS_POINTSIZE_MAX, (DWORD *) (&ptsizemax_orig));
        ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_GetRenderState(device, D3DRS_POINTSIZE_MIN, (DWORD *) (&ptsizemin_orig));
        ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed, hr=%08x\n", hr);

        /* What happens if point scaling is disabled, and POINTSIZE_MAX < POINTSIZE? */
        ptsize = 15.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        ptsize = 1.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE_MAX, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[18], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE_MAX, *((DWORD *) (&ptsizemax_orig)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);

        /* pointsize < pointsize_min < pointsize_max?
         * pointsize = 1.0, pointsize_min = 15.0, pointsize_max = default(usually 64.0) */
        ptsize = 1.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        ptsize = 15.0;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE_MIN, *((DWORD *) (&ptsize)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[21], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE_MIN, *((DWORD *) (&ptsizemin_orig)));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%08x\n", hr);
    }

    ok(point_match(device, 64, 64, 7), "point_match(64, 64, 7) failed, expected point size 15.\n");
    ok(point_match(device, 128, 64, 15), "point_match(128, 64, 15) failed, expected point size 31.\n");
    ok(point_match(device, 192, 64, 15), "point_match(192, 64, 15) failed, expected point size 31.\n");

    if (caps.MaxPointSize >= 63.0)
    {
        ok(point_match(device, 256, 64, 31), "point_match(256, 64, 31) failed, expected point size 63.\n");
        ok(point_match(device, 384, 64, 31), "point_match(384, 64, 31) failed, expected point size 63.\n");
    }

    ok(point_match(device, 320, 64, 0), "point_match(320, 64, 0) failed, expected point size 1.\n");
    /* ptsize = 15, ptsize_max = 1 --> point has size 1 */
    ok(point_match(device, 448, 64, 0), "point_match(448, 64, 0) failed, expected point size 1.\n");
    /* ptsize = 1, ptsize_max = default(64), ptsize_min = 15 --> point has size 15 */
    ok(point_match(device, 512, 64, 7), "point_match(512, 64, 7) failed, expected point size 15.\n");

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* The following code tests point sprites with two textures, to see if each texture coordinate unit
     * generates texture coordinates for the point(result: Yes, it does)
     *
     * However, not all GL implementations support point sprites(they need GL_ARB_point_sprite), but there
     * is no point sprite cap bit in d3d because native d3d software emulates point sprites. Until the
     * SW emulation is implemented in wined3d, this test will fail on GL drivers that does not support them.
     */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex2, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%08x\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture9_LockRect(tex1, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed hr=%08x\n", hr);
    memcpy(lr.pBits, tex1_data, sizeof(tex1_data));
    hr = IDirect3DTexture9_UnlockRect(tex1, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed hr=%08x\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture9_LockRect(tex2, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed hr=%08x\n", hr);
    memcpy(lr.pBits, tex2_data, sizeof(tex2_data));
    hr = IDirect3DTexture9_UnlockRect(tex2, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) tex1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, (IDirect3DBaseTexture9 *) tex2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSPRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed hr=%08x\n", hr);
    ptsize = 32.0;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize)));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed, hr=%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[0], sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%08x\n", hr);
    }

    color = getPixelColor(device, 64-4, 64-4);
    ok(color == 0x00ff0000, "pSprite: Pixel (64-4),(64-4) has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 64-4, 64+4);
    ok(color == 0x00000000, "pSprite: Pixel (64-4),(64+4) has color 0x%08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 64+4, 64+4);
    ok(color == 0x0000ff00, "pSprite: Pixel (64+4),(64+4) has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 64+4, 64-4);
    ok(color == 0x00ffff00, "pSprite: Pixel (64+4),(64-4) has color 0x%08x, expected 0x00ffff00\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    U(matrix).m[0][0] =  1.0f / 64.0f;
    U(matrix).m[1][1] = -1.0f / 64.0f;
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &matrix);
    ok(SUCCEEDED(hr), "SetTransform failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &rt, NULL );
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, &vertices[0], sizeof(float) * 3);
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, rt, &rect, backbuffer, &rect, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    IDirect3DSurface9_Release(backbuffer);
    IDirect3DSurface9_Release(rt);

    color = getPixelColor(device, 64-4, 64-4);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00), 0),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    color = getPixelColor(device, 64+4, 64-4);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x00), 0),
            "Expected color 0x00ffff00, got 0x%08x.\n", color);
    color = getPixelColor(device, 64-4, 64+4);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00), 0),
            "Expected color 0x00000000, got 0x%08x.\n", color);
    color = getPixelColor(device, 64+4, 64+4);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 0),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%08x\n", hr);
    IDirect3DTexture9_Release(tex1);
    IDirect3DTexture9_Release(tex2);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSPRITEENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, *((DWORD *) (&ptsize_orig)));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &identity);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed, hr=%08x\n", hr);
}

static void multiple_rendertargets_test(void)
{
    IDirect3DSurface9 *surf1, *surf2, *backbuf, *readback;
    IDirect3DPixelShader9 *ps1, *ps2;
    IDirect3DTexture9 *tex1, *tex2;
    IDirect3DVertexShader9 *vs;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;
    UINT i, j;

    static const DWORD vshader_code[] =
    {
        0xfffe0300,                                                             /* vs_3_0                     */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0            */
        0x0200001f, 0x80000000, 0xe00f0000,                                     /* dcl_position o0            */
        0x02000001, 0xe00f0000, 0x90e40000,                                     /* mov o0, v0                 */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD pshader_code1[] =
    {
        0xffff0300,                                                             /* ps_3_0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c0, 0.0, 1.0, 0.0, 0.0 */
        0x02000001, 0x800f0800, 0xa0e40000,                                     /* mov oC0, c0                */
        0x0000ffff                                                              /* end                        */
    };
    static const DWORD pshader_code2[] =
    {
        0xffff0300,                                                             /* ps_3_0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c0, 0.0, 1.0, 0.0, 0.0 */
        0x05000051, 0xa00f0001, 0x00000000, 0x00000000, 0x3f800000, 0x00000000, /* def c1, 0.0, 0.0, 1.0, 0.0 */
        0x02000001, 0x800f0800, 0xa0e40000,                                     /* mov oC0, c0                */
        0x02000001, 0x800f0801, 0xa0e40001,                                     /* mov oC1, c1                */
        0x0000ffff                                                              /* end                        */
    };
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
         1.0f,  1.0f, 0.1f,
    };
    static const float texquad[] =
    {
        -1.0f, -1.0f, 0.1f, 0.0f, 0.0f,
        -1.0f,  1.0f, 0.1f, 0.0f, 1.0f,
         0.0f, -1.0f, 0.1f, 1.0f, 0.0f,
         0.0f,  1.0f, 0.1f, 1.0f, 1.0f,

         0.0f, -1.0f, 0.1f, 0.0f, 0.0f,
         0.0f,  1.0f, 0.1f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.1f, 1.0f, 0.0f,
         1.0f,  1.0f, 0.1f, 1.0f, 1.0f,
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.NumSimultaneousRTs < 2)
    {
        skip("Only 1 simultaneous render target supported, skipping MRT test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }
    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0) || caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        skip("No shader model 3 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, NULL);
    ok(SUCCEEDED(hr), "CreateOffscreenPlainSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex2, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vshader_code, &vs);
    ok(SUCCEEDED(hr), "CreateVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pshader_code1, &ps1);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pshader_code2, &ps2);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuf);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderTarget failed, hr=%08x\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(tex1, 0, &surf1);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed, hr=%08x\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(tex2, 0, &surf2);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, vs);
    ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, surf1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 1, surf2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr=%08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x,\n", hr);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf1, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    color = getPixelColorFromSurface(readback, 8, 8);
    ok(color_match(color, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff), 0),
            "Expected color 0x000000ff, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf2, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    color = getPixelColorFromSurface(readback, 8, 8);
    ok(color_match(color, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff), 0),
            "Expected color 0x000000ff, got 0x%08x.\n", color);

    /* Render targets not written by the pixel shader should be unmodified. */
    hr = IDirect3DDevice9_SetPixelShader(device, ps1);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf1, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    color = getPixelColorFromSurface(readback, 8, 8);
    ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 0),
            "Expected color 0xff00ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf2, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    for (i = 6; i < 10; ++i)
    {
        for (j = 6; j < 10; ++j)
        {
            color = getPixelColorFromSurface(readback, j, i);
            ok(color_match(color, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff), 0),
                    "Expected color 0xff0000ff, got 0x%08x at %u, %u.\n", color, j, i);
        }
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x,\n", hr);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf1, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    color = getPixelColorFromSurface(readback, 8, 8);
    ok(color_match(color, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00), 0),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_GetRenderTargetData(device, surf2, readback);
    ok(SUCCEEDED(hr), "GetRenderTargetData failed, hr %#x.\n", hr);
    color = getPixelColorFromSurface(readback, 8, 8);
    ok(color_match(color, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00), 0),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);

    hr = IDirect3DDevice9_SetPixelShader(device, ps2);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed, hr=%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetVertexShader(device, NULL);
        ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuf);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetRenderTarget(device, 1, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) tex1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &texquad[0], 5 * sizeof(float));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) tex2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr=%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &texquad[20], 5 * sizeof(float));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%08x\n", hr);
    }

    color = getPixelColor(device, 160, 240);
    ok(color == 0x0000ff00, "Texture 1(output color 1) has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 240);
    ok(color == 0x000000ff, "Texture 2(output color 2) has color 0x%08x, expected 0x000000ff\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    IDirect3DPixelShader9_Release(ps2);
    IDirect3DPixelShader9_Release(ps1);
    IDirect3DVertexShader9_Release(vs);
    IDirect3DTexture9_Release(tex1);
    IDirect3DTexture9_Release(tex2);
    IDirect3DSurface9_Release(surf1);
    IDirect3DSurface9_Release(surf2);
    IDirect3DSurface9_Release(backbuf);
    IDirect3DSurface9_Release(readback);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

struct formats {
    const char *fmtName;
    D3DFORMAT textureFormat;
    DWORD resultColorBlending;
    DWORD resultColorNoBlending;
};

static const struct formats test_formats[] = {
  { "D3DFMT_G16R16", D3DFMT_G16R16, 0x001818ff, 0x002010ff},
  { "D3DFMT_R16F", D3DFMT_R16F, 0x0018ffff, 0x0020ffff },
  { "D3DFMT_G16R16F", D3DFMT_G16R16F, 0x001818ff, 0x002010ff },
  { "D3DFMT_A16B16G16R16F", D3DFMT_A16B16G16R16F, 0x00181800, 0x00201000 },
  { "D3DFMT_R32F", D3DFMT_R32F, 0x0018ffff, 0x0020ffff },
  { "D3DFMT_G32R32F", D3DFMT_G32R32F, 0x001818ff, 0x002010ff },
  { "D3DFMT_A32B32G32R32F", D3DFMT_A32B32G32R32F, 0x00181800, 0x00201000 },
  { NULL, 0 }
};

static void pixelshader_blending_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *offscreenTexture = NULL;
    IDirect3DSurface9 *backbuffer = NULL, *offscreen = NULL;
    IDirect3D9 *d3d = NULL;
    DWORD color;
    int fmt_index;

    static const float quad[][5] = {
        {-0.5f, -0.5f, 0.1f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.1f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.1f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.1f, 1.0f, 1.0f},
    };

    /* Quad with R=0x10, G=0x20 */
    static const struct vertex quad1[] = {
        {-1.0f, -1.0f, 0.1f, 0x80102000},
        {-1.0f,  1.0f, 0.1f, 0x80102000},
        { 1.0f, -1.0f, 0.1f, 0x80102000},
        { 1.0f,  1.0f, 0.1f, 0x80102000},
    };

    /* Quad with R=0x20, G=0x10 */
    static const struct vertex quad2[] = {
        {-1.0f, -1.0f, 0.1f, 0x80201000},
        {-1.0f,  1.0f, 0.1f, 0x80201000},
        { 1.0f, -1.0f, 0.1f, 0x80201000},
        { 1.0f,  1.0f, 0.1f, 0x80201000},
    };

    IDirect3DDevice9_GetDirect3D(device, &d3d);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(!backbuffer) {
        goto out;
    }

    for(fmt_index=0; test_formats[fmt_index].textureFormat != 0; fmt_index++)
    {
        D3DFORMAT fmt = test_formats[fmt_index].textureFormat;

        if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, fmt) != D3D_OK)
        {
            skip("%s textures not supported as render targets.\n", test_formats[fmt_index].fmtName);
            continue;
        }

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
        ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %08x\n", hr);
        if(!offscreenTexture) {
            continue;
        }

        hr = IDirect3DTexture9_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
        ok(hr == D3D_OK, "Can't get offscreen surface, hr = %08x\n", hr);
        if(!offscreen) {
            continue;
        }

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
        ok(hr == D3D_OK, "SetFVF failed, hr = %08x\n", hr);

        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

        /* Below we will draw two quads with different colors and try to blend them together.
         * The result color is compared with the expected outcome.
         */
        if(IDirect3DDevice9_BeginScene(device) == D3D_OK) {
            hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen);
            ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00ffffff, 0.0, 0);
            ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);

            /* Draw a quad using color 0x0010200 */
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_ONE);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_ZERO);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(quad1[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

            /* Draw a quad using color 0x0020100 */
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(quad2[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %#08x\n", hr);

            /* We don't want to blend the result on the backbuffer */
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);

            /* Prepare rendering the 'blended' texture quad to the backbuffer */
            hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
            ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) offscreenTexture);
            ok(hr == D3D_OK, "SetTexture failed, %08x\n", hr);

            hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
            ok(hr == D3D_OK, "SetFVF failed, hr = %08x\n", hr);

            /* This time with the texture */
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %08x\n", hr);

            IDirect3DDevice9_EndScene(device);
        }

        if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, fmt) == D3D_OK)
        {
            /* Compare the color of the center quad with our expectation. */
            color = getPixelColor(device, 320, 240);
            ok(color_match(color, test_formats[fmt_index].resultColorBlending, 1),
                    "Offscreen failed for %s: Got color 0x%08x, expected 0x%08x.\n",
                    test_formats[fmt_index].fmtName, color, test_formats[fmt_index].resultColorBlending);
        }
        else
        {
            /* No pixel shader blending is supported so expect garbage. The
             * type of 'garbage' depends on the driver version and OS. E.g. on
             * G16R16 ATI reports (on old r9600 drivers) 0x00ffffff and on
             * modern ones 0x002010ff which is also what NVIDIA reports. On
             * Vista NVIDIA seems to report 0x00ffffff on Geforce7 cards. */
            color = getPixelColor(device, 320, 240);
            ok((color == 0x00ffffff) || (color == test_formats[fmt_index].resultColorNoBlending),
                    "Offscreen failed for %s: Got unexpected color 0x%08x, expected no color blending.\n",
                    test_formats[fmt_index].fmtName, color);
        }
        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

        IDirect3DDevice9_SetTexture(device, 0, NULL);
        if(offscreenTexture) {
            IDirect3DTexture9_Release(offscreenTexture);
        }
        if(offscreen) {
            IDirect3DSurface9_Release(offscreen);
        }
    }

out:
    /* restore things */
    if(backbuffer) {
        IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        IDirect3DSurface9_Release(backbuffer);
    }
}

static void tssargtemp_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    static const struct vertex quad[] = {
        {-1.0,     -1.0,    0.1,    0x00ff0000},
        { 1.0,     -1.0,    0.1,    0x00ff0000},
        {-1.0,      1.0,    0.1,    0x00ff0000},
        { 1.0,      1.0,    0.1,    0x00ff0000}
    };
    D3DCAPS9 caps;

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed with %08x\n", hr);
    if(!(caps.PrimitiveMiscCaps & D3DPMISCCAPS_TSSARGTEMP)) {
        skip("D3DPMISCCAPS_TSSARGTEMP not supported\n");
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_RESULTARG, D3DTA_TEMP);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 2, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 2, D3DTSS_COLORARG1, D3DTA_CURRENT);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 2, D3DTSS_COLORARG2, D3DTA_TEMP);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0x0000ff00);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed, hr = %08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed with %08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr = %08x\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffff00, "TSSARGTEMP test returned color 0x%08x, expected 0x00ffff00\n", color);
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* Set stage 1 back to default */
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_RESULTARG, D3DTA_CURRENT);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 3, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
}

struct testdata
{
    DWORD idxVertex; /* number of instances in the first stream */
    DWORD idxColor; /* number of instances in the second stream */
    DWORD idxInstance; /* should be 1 ?? */
    DWORD color1; /* color 1 instance */
    DWORD color2; /* color 2 instance */
    DWORD color3; /* color 3 instance */
    DWORD color4; /* color 4 instance */
    WORD strVertex; /* specify which stream to use 0-2*/
    WORD strColor;
    WORD strInstance;
};

static const struct testdata testcases[]=
{
    {4, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 1, 2}, /*  0 */
    {3, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ffffff, 0, 1, 2}, /*  1 */
    {2, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ffffff, 0x00ffffff, 0, 1, 2}, /*  2 */
    {1, 4, 1, 0x00ff0000, 0x00ffffff, 0x00ffffff, 0x00ffffff, 0, 1, 2}, /*  3 */
    {4, 3, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 1, 2}, /*  4 */
    {4, 2, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 1, 2}, /*  5 */
    {4, 1, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 1, 2}, /*  6 */
    {4, 0, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 1, 2}, /*  7 */
    {3, 3, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ffffff, 0, 1, 2}, /*  8 */
    {4, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 1, 0, 2}, /*  9 */
    {4, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0, 2, 1}, /* 10 */
    {4, 4, 1, 0x00ff0000, 0x00ffffff, 0x00ffffff, 0x00ffffff, 2, 3, 1}, /* 11 */
    {4, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 2, 0, 1}, /* 12 */
    {4, 4, 1, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 1, 2, 3}, /* 13 */
/*
    This draws one instance on some machines, no instance on others
    {0, 4, 1, 0x00ff0000, 0x00ffffff, 0x00ffffff, 0x00ffffff, 0, 1, 2},
*/
/*
    This case is handled in a stand alone test, SetStreamSourceFreq(0,(D3DSTREAMSOURCE_INSTANCEDATA | 1))  has to return D3DERR_INVALIDCALL!
    {4, 4, 1, 0x00ffffff, 0x00ffffff, 0x00ffffff, 0x00ffffff, 2, 1, 0, D3DERR_INVALIDCALL},
*/
};

/* Drawing Indexed Geometry with instances*/
static void stream_test(IDirect3DDevice9 *device)
{
    IDirect3DVertexBuffer9 *vb = NULL;
    IDirect3DVertexBuffer9 *vb2 = NULL;
    IDirect3DVertexBuffer9 *vb3 = NULL;
    IDirect3DIndexBuffer9 *ib = NULL;
    IDirect3DVertexDeclaration9 *pDecl = NULL;
    IDirect3DVertexShader9 *shader = NULL;
    HRESULT hr;
    BYTE *data;
    DWORD color;
    DWORD ind;
    unsigned i;

    static const DWORD shader_code[] =
    {
        0xfffe0101,                                     /* vs_1_1 */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position v0 */
        0x0000001f, 0x8000000a, 0x900f0001,             /* dcl_color0 v1 */
        0x0000001f, 0x80000005, 0x900f0002,             /* dcl_texcoord v2 */
        0x00000001, 0x800f0000, 0x90e40000,             /* mov r0, v0 */
        0x00000002, 0xc00f0000, 0x80e40000, 0x90e40002, /* add oPos, r0, v2 */
        0x00000001, 0xd00f0000, 0x90e40001,             /* mov oD0, v1 */
        0x0000ffff
    };

    const float quad[][3] =
    {
        {-0.5f, -0.5f,  1.1f}, /*0 */
        {-0.5f,  0.5f,  1.1f}, /*1 */
        { 0.5f, -0.5f,  1.1f}, /*2 */
        { 0.5f,  0.5f,  1.1f}, /*3 */
    };

    const float vertcolor[][4] =
    {
        {1.0f, 0.0f, 0.0f, 1.0f}, /*0 */
        {1.0f, 0.0f, 0.0f, 1.0f}, /*1 */
        {1.0f, 0.0f, 0.0f, 1.0f}, /*2 */
        {1.0f, 0.0f, 0.0f, 1.0f}, /*3 */
    };

    /* 4 position for 4 instances */
    const float instancepos[][3] =
    {
        {-0.6f,-0.6f, 0.0f},
        { 0.6f,-0.6f, 0.0f},
        { 0.6f, 0.6f, 0.0f},
        {-0.6f, 0.6f, 0.0f},
    };

    short indices[] = {0, 1, 2, 1, 2, 3};

    D3DVERTEXELEMENT9 decl[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        {2, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    /* set the default value because it isn't done in wine? */
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);

    /* check for D3DSTREAMSOURCE_INDEXEDDATA at stream0 */
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 0, (D3DSTREAMSOURCE_INSTANCEDATA | 1));
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);

    /* check wrong cases */
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetStreamSourceFreq(device, 1, &ind);
    ok(hr == D3D_OK && ind == 1, "IDirect3DDevice9_GetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, 2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetStreamSourceFreq(device, 1, &ind);
    ok(hr == D3D_OK && ind == 2, "IDirect3DDevice9_GetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, (D3DSTREAMSOURCE_INDEXEDDATA | 0));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetStreamSourceFreq(device, 1, &ind);
    ok(hr == D3D_OK && ind == (D3DSTREAMSOURCE_INDEXEDDATA | 0), "IDirect3DDevice9_GetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, (D3DSTREAMSOURCE_INSTANCEDATA | 0));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetStreamSourceFreq(device, 1, &ind);
    ok(hr == D3D_OK && ind == (0U | D3DSTREAMSOURCE_INSTANCEDATA), "IDirect3DDevice9_GetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, (D3DSTREAMSOURCE_INSTANCEDATA | D3DSTREAMSOURCE_INDEXEDDATA | 0));
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetStreamSourceFreq(device, 1, &ind);
    ok(hr == D3D_OK && ind == (0U | D3DSTREAMSOURCE_INSTANCEDATA), "IDirect3DDevice9_GetStreamSourceFreq failed with %08x\n", hr);

    /* set the default value back */
    hr = IDirect3DDevice9_SetStreamSourceFreq(device, 1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x\n", hr);

    /* create all VertexBuffers*/
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, 0, D3DPOOL_MANAGED, &vb, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if(!vb) {
        skip("Failed to create a vertex buffer\n");
        return;
    }
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(vertcolor), 0, 0, D3DPOOL_MANAGED, &vb2, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if(!vb2) {
        skip("Failed to create a vertex buffer\n");
        goto out;
    }
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(instancepos), 0, 0, D3DPOOL_MANAGED, &vb3, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %08x\n", hr);
    if(!vb3) {
        skip("Failed to create a vertex buffer\n");
        goto out;
    }

    /* create IndexBuffer*/
    hr = IDirect3DDevice9_CreateIndexBuffer(device, sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateIndexBuffer failed with %08x\n", hr);
    if(!ib) {
        skip("Failed to create an index buffer\n");
        goto out;
    }

    /* copy all Buffers (Vertex + Index)*/
    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb2, 0, sizeof(vertcolor), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, vertcolor, sizeof(vertcolor));
    hr = IDirect3DVertexBuffer9_Unlock(vb2);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb3, 0, sizeof(instancepos), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, instancepos, sizeof(instancepos));
    hr = IDirect3DVertexBuffer9_Unlock(vb3);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %08x\n", hr);
    hr = IDirect3DIndexBuffer9_Lock(ib, 0, sizeof(indices), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Lock failed with %08x\n", hr);
    memcpy(data, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(ib);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Unlock failed with %08x\n", hr);

    /* create VertexShader */
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexShader failed hr=%08x\n", hr);
    if(!shader) {
        skip("Failed to create a vetex shader\n");
        goto out;
    }

    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed hr=%08x\n", hr);

    hr = IDirect3DDevice9_SetIndices(device, ib);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetIndices failed with %08x\n", hr);

    /* run all tests */
    for( i = 0; i < sizeof(testcases)/sizeof(testcases[0]); ++i)
    {
        struct testdata act = testcases[i];
        decl[0].Stream = act.strVertex;
        decl[1].Stream = act.strColor;
        decl[2].Stream = act.strInstance;
        /* create VertexDeclarations */
        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl, &pDecl);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexDeclaration failed hr=%08x (case %i)\n", hr, i);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x (case %i)\n", hr, i);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x (case %i)\n", hr, i);
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, pDecl);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %08x (case %i)\n", hr, i);

            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.strVertex, (D3DSTREAMSOURCE_INDEXEDDATA | act.idxVertex));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.strVertex, vb, 0, sizeof(quad[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);

            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.strColor, (D3DSTREAMSOURCE_INDEXEDDATA | act.idxColor));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.strColor, vb2, 0, sizeof(vertcolor[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);

            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.strInstance, (D3DSTREAMSOURCE_INSTANCEDATA | act.idxInstance));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.strInstance, vb3, 0, sizeof(instancepos[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);

            hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitive failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x (case %i)\n", hr, i);

            /* set all StreamSource && StreamSourceFreq back to default */
            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.strVertex, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.strVertex, NULL, 0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.idxColor, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.idxColor, NULL, 0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSourceFreq(device, act.idxInstance, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSourceFreq failed with %08x (case %i)\n", hr, i);
            hr = IDirect3DDevice9_SetStreamSource(device, act.idxInstance, NULL, 0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %08x (case %i)\n", hr, i);
        }

        hr = IDirect3DVertexDeclaration9_Release(pDecl);
        ok(hr == D3D_OK, "IDirect3DVertexDeclaration9_Release failed with %08x (case %i)\n", hr, i);

        color = getPixelColor(device, 160, 360);
        ok(color == act.color1, "has color 0x%08x, expected 0x%08x (case %i)\n", color, act.color1, i);
        color = getPixelColor(device, 480, 360);
        ok(color == act.color2, "has color 0x%08x, expected 0x%08x (case %i)\n", color, act.color2, i);
        color = getPixelColor(device, 480, 120);
        ok(color == act.color3, "has color 0x%08x, expected 0x%08x (case %i)\n", color, act.color3, i);
        color = getPixelColor(device, 160, 120);
        ok(color == act.color4, "has color 0x%08x, expected 0x%08x (case %i)\n", color, act.color4, i);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x (case %i)\n", hr, i);
    }

    hr = IDirect3DDevice9_SetIndices(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetIndices failed with %08x\n", hr);

out:
    if(vb) IDirect3DVertexBuffer9_Release(vb);
    if(vb2)IDirect3DVertexBuffer9_Release(vb2);
    if(vb3)IDirect3DVertexBuffer9_Release(vb3);
    if(ib)IDirect3DIndexBuffer9_Release(ib);
    if(shader)IDirect3DVertexShader9_Release(shader);
}

static void np2_stretch_rect_test(IDirect3DDevice9 *device) {
    IDirect3DSurface9 *src = NULL, *dst = NULL, *backbuffer = NULL;
    IDirect3DTexture9 *dsttex = NULL;
    HRESULT hr;
    DWORD color;
    D3DRECT r1 = {0,  0,  50,  50 };
    D3DRECT r2 = {50, 0,  100, 50 };
    D3DRECT r3 = {50, 50, 100, 100};
    D3DRECT r4 = {0,  50,  50, 100};
    const float quad[] = {
        -1.0,   -1.0,   0.1,    0.0,    0.0,
         1.0,   -1.0,   0.1,    1.0,    0.0,
        -1.0,    1.0,   0.1,    0.0,    1.0,
         1.0,    1.0,   0.1,    1.0,    1.0,
    };

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed with %08x\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 100, 100, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &src, NULL );
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_CreateRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 25, 25, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &dsttex, NULL);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);

    if(!src || !dsttex) {
        skip("One or more test resources could not be created\n");
        goto cleanup;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(dsttex, 0, &dst);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    /* Clear the StretchRect destination for debugging */
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, dst);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, src);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 1, &r1, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 1, &r2, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 1, &r3, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 1, &r4, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    /* Stretchrect before setting the render target back to the backbuffer. This will make Wine use
     * the target -> texture GL blit path
     */
    hr = IDirect3DDevice9_StretchRect(device, src, NULL, dst, NULL, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    IDirect3DSurface9_Release(dst);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) dsttex);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "stretchrect: Pixel 160,360 has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000ff00, "stretchrect: Pixel 480,360 has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "stretchrect: Pixel 480,120 has color 0x%08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "stretchrect: Pixel 160,120 has color 0x%08x, expected 0x00000000\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %08x\n", hr);

cleanup:
    if(src) IDirect3DSurface9_Release(src);
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);
    if(dsttex) IDirect3DTexture9_Release(dsttex);
}

static void texop_test(void)
{
    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DTexture9 *texture = NULL;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    unsigned i;

    static const struct {
        float x, y, z;
        float s, t;
        D3DCOLOR diffuse;
    } quad[] = {
        {-1.0f, -1.0f, 0.1f, -1.0f, -1.0f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00)},
        {-1.0f,  1.0f, 0.1f, -1.0f,  1.0f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00)},
        { 1.0f, -1.0f, 0.1f,  1.0f, -1.0f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00)},
        { 1.0f,  1.0f, 0.1f,  1.0f,  1.0f, D3DCOLOR_ARGB(0x55, 0xff, 0x00, 0x00)}
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
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
        {D3DTOP_DOTPRODUCT3,               "DOTPRODUCT3",               D3DTEXOPCAPS_DOTPRODUCT3,               D3DCOLOR_ARGB(0x00, 0x99, 0x99, 0x99)},
        {D3DTOP_MULTIPLYADD,               "MULTIPLYADD",               D3DTEXOPCAPS_MULTIPLYADD,               D3DCOLOR_ARGB(0x00, 0xff, 0x33, 0x00)},
        {D3DTOP_LERP,                      "LERP",                      D3DTEXOPCAPS_LERP,                      D3DCOLOR_ARGB(0x00, 0x00, 0x33, 0x33)},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed with 0x%08x\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    *((DWORD *)locked_rect.pBits) = D3DCOLOR_ARGB(0x99, 0x00, 0xff, 0x00);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "UnlockRect failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG0, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0xdd333333);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); ++i)
    {
        if (!(caps.TextureOpCaps & test_data[i].caps_flag))
        {
            skip("tex operation %s not supported\n", test_data[i].name);
            continue;
        }

        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, test_data[i].op);
        ok(SUCCEEDED(hr), "SetTextureStageState (%s) failed with 0x%08x\n", test_data[i].name, hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed with 0x%08x\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed with 0x%08x\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, test_data[i].result, 3), "Operation %s returned color 0x%08x, expected 0x%08x\n",
                test_data[i].name, color, test_data[i].result);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed with 0x%08x\n", hr);
    }

    IDirect3DTexture9_Release(texture);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void yuv_color_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DSurface9 *surface, *target;
    unsigned int i;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    D3DFORMAT skip_once = D3DFMT_UNKNOWN;
    D3DSURFACE_DESC desc;

    static const struct
    {
        DWORD in;
        D3DFORMAT format;
        const char *fmt_string;
        D3DCOLOR left, right;
    }
    test_data[] =
    {
        {0x00000000, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00008700, 0x00008700},
        {0xff000000, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00008700, 0x004bff1c},
        {0x00ff0000, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00b30000, 0x00b30000},
        {0x0000ff00, D3DFMT_UYVY, "D3DFMT_UYVY", 0x004bff1c, 0x00008700},
        {0x000000ff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x000030e1, 0x000030e1},
        {0xffff0000, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00b30000, 0x00ffd01c},
        {0xff00ff00, D3DFMT_UYVY, "D3DFMT_UYVY", 0x004bff1c, 0x004bff1c},
        {0xff0000ff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x000030e1, 0x004bffff},
        {0x00ffff00, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00ffd01c, 0x00b30000},
        {0x00ff00ff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00b300e1, 0x00b300e1},
        {0x0000ffff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x004bffff, 0x001030e1},
        {0xffffff00, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00ffd01c, 0x00ffd01c},
        {0xffff00ff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00b300e1, 0x00ff79ff},
        {0xffffffff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00ff79ff, 0x00ff79ff},
        {0x4cff4c54, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00ff0000, 0x00ff0000},
        {0x00800080, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00000000, 0x00000000},
        {0xff80ff80, D3DFMT_UYVY, "D3DFMT_UYVY", 0x00ffffff, 0x00ffffff},
        {0x1c6b1cff, D3DFMT_UYVY, "D3DFMT_UYVY", 0x000000fd, 0x000000fd},

        {0x00000000, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00008700, 0x00008700},
        {0xff000000, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00b30000, 0x00b30000},
        {0x00ff0000, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00008700, 0x004bff1c},
        {0x0000ff00, D3DFMT_YUY2, "D3DFMT_YUY2", 0x000030e1, 0x000030e1},
        {0x000000ff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x004bff1c, 0x00008700},
        {0xffff0000, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00b30000, 0x00ffd01c},
        {0xff00ff00, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00b300e1, 0x00b300e1},
        {0xff0000ff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00ffd01c, 0x00b30000},
        {0x00ffff00, D3DFMT_YUY2, "D3DFMT_YUY2", 0x000030e1, 0x004bffff},
        {0x00ff00ff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x004bff1c, 0x004bff1c},
        {0x0000ffff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x004bffff, 0x000030e1},
        {0xffffff00, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00b300e1, 0x00ff79ff},
        {0xffff00ff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00ffd01c, 0x00ffd01c},
        {0xffffffff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00ff79ff, 0x00ff79ff},
        {0x4cff4c54, D3DFMT_YUY2, "D3DFMT_YUY2", 0x000b8b00, 0x00b6ffa3},
        {0x00800080, D3DFMT_YUY2, "D3DFMT_YUY2", 0x0000ff00, 0x0000ff00},
        {0xff80ff80, D3DFMT_YUY2, "D3DFMT_YUY2", 0x00ff00ff, 0x00ff00ff},
        {0x1c6b1cff, D3DFMT_YUY2, "D3DFMT_YUY2", 0x006dff45, 0x0000d500},
    };

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d9 interface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &target);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(target, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);

    for (i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
    {
        /* Some(all?) Windows drivers do not support YUV 3D textures, only 2D surfaces in StretchRect.
         * Thus use StretchRect to draw the YUV surface onto the screen instead of drawPrimitive. */
        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, test_data[i].format)))
        {
            if (skip_once != test_data[i].format)
            {
                skip("%s is not supported.\n", test_data[i].fmt_string);
                skip_once = test_data[i].format;
            }
            continue;
        }
        if (FAILED(IDirect3D9_CheckDeviceFormatConversion(d3d, 0,
                D3DDEVTYPE_HAL, test_data[i].format, desc.Format)))
        {
            if (skip_once != test_data[i].format)
            {
                skip("Driver cannot blit %s surfaces.\n", test_data[i].fmt_string);
                skip_once = test_data[i].format;
            }
            continue;
        }

        /* A pixel is effectively 16 bit large, but two pixels are stored together, so the minimum size is 2x1.
         * However, Nvidia Windows drivers have problems with 2x1 YUY2/UYVY surfaces, so use a 4x1 surface and
         * fill the second block with dummy data. If the surface has a size of 2x1, those drivers ignore the
         * second luminance value, resulting in an incorrect color in the right pixel. */
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 1, test_data[i].format,
                D3DPOOL_DEFAULT, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);


        hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        ((DWORD *)lr.pBits)[0] = test_data[i].in;
        ((DWORD *)lr.pBits)[1] = 0x00800080;
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
        hr = IDirect3DDevice9_StretchRect(device, surface, NULL, target, NULL, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "Failed to draw surface onto backbuffer, hr %#x.\n", hr);

        /* Some Windows drivers (mostly Nvidia, but also some VM drivers) insist on doing linear filtering
         * although we asked for point filtering. Be careful when reading the results and use the pixel
         * centers. In the future we may want to add tests for the filtered pixels as well.
         *
         * Unfortunately different implementations(Windows-Nvidia and Mac-AMD tested) interpret some colors
         * vastly differently, so we need a max diff of 18. */
        color = getPixelColor(device, 1, 240);
        ok(color_match(color, test_data[i].left, 18),
                "Input 0x%08x: Got color 0x%08x for pixel 1/1, expected 0x%08x, format %s.\n",
                test_data[i].in, color, test_data[i].left, test_data[i].fmt_string);
        color = getPixelColor(device, 318, 240);
        ok(color_match(color, test_data[i].right, 18),
                "Input 0x%08x: Got color 0x%08x for pixel 2/1, expected 0x%08x, format %s.\n",
                test_data[i].in, color, test_data[i].right, test_data[i].fmt_string);
        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
        IDirect3DSurface9_Release(surface);
    }

    IDirect3DSurface9_Release(target);
    IDirect3D9_Release(d3d);
}

static void yuv_layout_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DSurface9 *surface, *target;
    unsigned int fmt, i, x, y;
    D3DFORMAT format;
    const char *fmt_string;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    DWORD ref_color;
    BYTE *buf, *chroma_buf, *u_buf, *v_buf;
    UINT width = 20, height = 16;
    D3DCAPS9 caps;
    D3DSURFACE_DESC desc;

    static const struct
    {
        DWORD color1, color2;
        DWORD rgb1, rgb2;
    }
    test_data[] =
    {
        { 0x000000, 0xffffff, 0x00008800, 0x00ff7dff },
        { 0xff0000, 0x00ffff, 0x004aff14, 0x00b800ee },
        { 0x00ff00, 0xff00ff, 0x000024ee, 0x00ffe114 },
        { 0x0000ff, 0xffff00, 0x00b80000, 0x004affff },
        { 0xffff00, 0x0000ff, 0x004affff, 0x00b80000 },
        { 0xff00ff, 0x00ff00, 0x00ffe114, 0x000024ee },
        { 0x00ffff, 0xff0000, 0x00b800ee, 0x004aff14 },
        { 0xffffff, 0x000000, 0x00ff7dff, 0x00008800 },
    };

    static const struct
    {
        D3DFORMAT format;
        const char *str;
    }
    formats[] =
    {
        { D3DFMT_UYVY, "D3DFMT_UYVY", },
        { D3DFMT_YUY2, "D3DFMT_YUY2", },
        { MAKEFOURCC('Y','V','1','2'), "D3DFMT_YV12", },
        { MAKEFOURCC('N','V','1','2'), "D3DFMT_NV12", },
    };

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2
            && !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
    {
        skip("No NP2 texture support, skipping YUV texture layout test.\n");
        return;
    }

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDirect3D failed, hr = %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &target);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderTarget failed, hr = %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(target, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);

    for (fmt = 0; fmt < sizeof(formats) / sizeof(formats[0]); fmt++)
    {
        format = formats[fmt].format;
        fmt_string = formats[fmt].str;

        /* Some (all?) Windows drivers do not support YUV 3D textures, only 2D surfaces in
         * StretchRect. Thus use StretchRect to draw the YUV surface onto the screen instead
         * of drawPrimitive. */
        if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                D3DRTYPE_SURFACE, format) != D3D_OK)
        {
            skip("%s is not supported.\n", fmt_string);
            continue;
        }
        if (FAILED(IDirect3D9_CheckDeviceFormatConversion(d3d, 0,
                D3DDEVTYPE_HAL, format, desc.Format)))
        {
            skip("Driver cannot blit %s surfaces.\n", fmt_string);
            continue;
        }

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, format, D3DPOOL_DEFAULT, &surface, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed, hr = %#x.\n", hr);

        for (i = 0; i < sizeof(test_data) / sizeof(test_data[0]); i++)
        {
            hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
            ok(hr == D3D_OK, "IDirect3DSurface9_LockRect failed, hr = %#x.\n", hr);
            buf = lr.pBits;
            chroma_buf = buf + lr.Pitch * height;
            if (format == MAKEFOURCC('Y','V','1','2'))
            {
                v_buf = chroma_buf;
                u_buf = chroma_buf + height / 2 * lr.Pitch/2;
            }
            /* Draw the top left quarter of the screen with color1, the rest with color2 */
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x += 2)
                {
                    DWORD color = (x < width / 2 && y < height / 2) ? test_data[i].color1 : test_data[i].color2;
                    BYTE Y = (color >> 16) & 0xff;
                    BYTE U = (color >>  8) & 0xff;
                    BYTE V = (color >>  0) & 0xff;
                    if (format == D3DFMT_UYVY)
                    {
                        buf[y * lr.Pitch + 2 * x + 0] = U;
                        buf[y * lr.Pitch + 2 * x + 1] = Y;
                        buf[y * lr.Pitch + 2 * x + 2] = V;
                        buf[y * lr.Pitch + 2 * x + 3] = Y;
                    }
                    else if (format == D3DFMT_YUY2)
                    {
                        buf[y * lr.Pitch + 2 * x + 0] = Y;
                        buf[y * lr.Pitch + 2 * x + 1] = U;
                        buf[y * lr.Pitch + 2 * x + 2] = Y;
                        buf[y * lr.Pitch + 2 * x + 3] = V;
                    }
                    else if (format == MAKEFOURCC('Y','V','1','2'))
                    {
                        buf[y * lr.Pitch + x + 0] = Y;
                        buf[y * lr.Pitch + x + 1] = Y;
                        u_buf[(y / 2) * (lr.Pitch / 2) + (x / 2)] = U;
                        v_buf[(y / 2) * (lr.Pitch / 2) + (x / 2)] = V;
                    }
                    else if (format == MAKEFOURCC('N','V','1','2'))
                    {
                        buf[y * lr.Pitch + x + 0] = Y;
                        buf[y * lr.Pitch + x + 1] = Y;
                        chroma_buf[(y / 2) * lr.Pitch + 2 * (x / 2) + 0] = U;
                        chroma_buf[(y / 2) * lr.Pitch + 2 * (x / 2) + 1] = V;
                    }
                }
            }
            hr = IDirect3DSurface9_UnlockRect(surface);
            ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect failed, hr = %#x.\n", hr);

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %#x.\n", hr);
            hr = IDirect3DDevice9_StretchRect(device, surface, NULL, target, NULL, D3DTEXF_POINT);
            ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %#x.\n", hr);

            /* Some Windows drivers (mostly Nvidia, but also some VM drivers) insist on doing linear filtering
             * although we asked for point filtering. To prevent running into precision problems, read at points
             * with some margin within each quadrant.
             *
             * Unfortunately different implementations(Windows-Nvidia and Mac-AMD tested) interpret some colors
             * vastly differently, so we need a max diff of 18. */
            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < 4; x++)
                {
                    UINT xcoord = (1 + 2 * x) * 640 / 8;
                    UINT ycoord = (1 + 2 * y) * 480 / 8;
                    ref_color = (y < 2 && x < 2) ? test_data[i].rgb1 : test_data[i].rgb2;
                    color = getPixelColor(device, xcoord, ycoord);
                    ok(color_match(color, ref_color, 18),
                            "Format %s: Got color %#x for pixel (%d/%d)/(%d/%d), pixel %d %d, expected %#x.\n",
                            fmt_string, color, x, 4, y, 4, xcoord, ycoord, ref_color);
                }
            }
            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

            ok(SUCCEEDED(hr), "Present failed with %#x.\n", hr);
        }
        IDirect3DSurface9_Release(surface);
    }

    IDirect3DSurface9_Release(target);
    IDirect3D9_Release(d3d);
}

static void texop_range_test(void)
{
    IDirect3DTexture9 *texture;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const struct
    {
        float x, y, z;
        D3DCOLOR diffuse;
    }
    quad[] =
    {
        {-1.0f, -1.0f, 0.1f, D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff)},
        {-1.0f,  1.0f, 0.1f, D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff)},
        { 1.0f, -1.0f, 0.1f, D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff)},
        { 1.0f,  1.0f, 0.1f, D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff)}
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    /* We need ADD and SUBTRACT operations */
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed with 0x%08x\n", hr);
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_ADD))
    {
        skip("D3DTOP_ADD is not supported, skipping value range test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_SUBTRACT))
    {
        skip("D3DTEXOPCAPS_SUBTRACT is not supported, skipping value range test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    /* Stage 1: result = diffuse(=1.0) + diffuse
     * stage 2: result = result - tfactor(= 0.5)
     */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0x80808080);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_SUBTRACT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear device, hr %#x.\n\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed with 0x%08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00808080, 1), "texop Range > 1.0 returned 0x%08x, expected 0x00808080\n",
       color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture failed with 0x%08x\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed with 0x%08x\n", hr);
    *((DWORD *)locked_rect.pBits) = D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0x00);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "UnlockRect failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed with 0x%08x\n", hr);

    /* Stage 1: result = texture(=0.0) - tfactor(= 0.5)
     * stage 2: result = result + diffuse(1.0)
     */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0x80808080);
    ok(SUCCEEDED(hr), "SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SUBTRACT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed with 0x%08x\n", hr);

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ffffff, 1), "texop Range < 0.0 returned 0x%08x, expected 0x00ffffff\n",
       color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed with 0x%08x\n", hr);

    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void alphareplicate_test(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const struct vertex quad[] =
    {
        {-1.0f, -1.0f, 0.1f, 0x80ff00ff},
        {-1.0f,  1.0f, 0.1f, 0x80ff00ff},
        { 1.0f, -1.0f, 0.1f, 0x80ff00ff},
        { 1.0f,  1.0f, 0.1f, 0x80ff00ff},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE | D3DTA_ALPHAREPLICATE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
    }

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00808080, 1), "alphareplicate test 0x%08x, expected 0x00808080\n",
       color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed with 0x%08x\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void dp3_alpha_test(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const struct vertex quad[] =
    {
        {-1.0f, -1.0f, 0.1f, 0x408080c0},
        {-1.0f,  1.0f, 0.1f, 0x408080c0},
        { 1.0f, -1.0f, 0.1f, 0x408080c0},
        { 1.0f,  1.0f, 0.1f, 0x408080c0},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureOpCaps & D3DTEXOPCAPS_DOTPRODUCT3))
    {
        skip("D3DTOP_DOTPRODUCT3 not supported\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with 0x%08x\n", hr);

    /* dp3_x4 r0, diffuse_bias, tfactor_bias
     * mov r0.a, diffuse.a
     * mov r0, r0.a
     *
     * It turns out that the 2nd line is ignored, and the dp3 result written into r0.a instead
     * thus with input vec4(0.5, 0.5, 0.75, 0.25) and vec4(1.0, 1.0, 1.0, 1.0) the result is
     * (0.0 * 0.5 + 0.0 * 0.5 + 0.25 * 0.5) * 4 = 0.125 * 4 = 0.5, with a bunch of inprecision.
     */
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DOTPRODUCT3);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_CURRENT | D3DTA_ALPHAREPLICATE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0xffffffff);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
    }

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00808080, 4), "dp3 alpha test 0x%08x, expected 0x00808080\n",
       color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed with 0x%08x\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void zwriteenable_test(IDirect3DDevice9 *device) {
    HRESULT hr;
    DWORD color;
    struct vertex quad1[] = {
        { -1.0,  -1.0,  0.1,    0x00ff0000},
        { -1.0,   1.0,  0.1,    0x00ff0000},
        {  1.0,  -1.0,  0.1,    0x00ff0000},
        {  1.0,   1.0,  0.1,    0x00ff0000},
    };
    struct vertex quad2[] = {
        { -1.0,  -1.0,  0.9,    0x0000ff00},
        { -1.0,   1.0,  0.9,    0x0000ff00},
        {  1.0,  -1.0,  0.9,    0x0000ff00},
        {  1.0,   1.0,  0.9,    0x0000ff00},
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x000000ff, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
    if(SUCCEEDED(hr)) {
        /* The Z buffer is filled with 1.0. Draw a red quad with z = 0.1, zenable = D3DZB_FALSE, zwriteenable = TRUE.
         * The red color is written because the z test is disabled. The question is whether the z = 0.1 values
         * are written into the Z buffer. After the draw, set zenable = TRUE and draw a green quad at z = 0.9.
         * If the values are written, the z test will fail(0.9 > 0.1) and the red color remains. If the values
         * are not written, the z test succeeds(0.9 < 1.0) and the green color is written. It turns out that
         * the screen is green, so zenable = D3DZB_FALSE and zwriteenable  = TRUE does NOT write to the z buffer.
         */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
    }

    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1), "zwriteenable test returned 0x%08x, expected 0x0000ff00\n",
       color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
}

static void alphatest_test(IDirect3DDevice9 *device) {
#define ALPHATEST_PASSED 0x0000ff00
#define ALPHATEST_FAILED 0x00ff0000
    struct {
        D3DCMPFUNC  func;
        DWORD       color_less;
        DWORD       color_equal;
        DWORD       color_greater;
    } testdata[] = {
        {   D3DCMP_NEVER,           ALPHATEST_FAILED, ALPHATEST_FAILED, ALPHATEST_FAILED },
        {   D3DCMP_LESS,            ALPHATEST_PASSED, ALPHATEST_FAILED, ALPHATEST_FAILED },
        {   D3DCMP_EQUAL,           ALPHATEST_FAILED, ALPHATEST_PASSED, ALPHATEST_FAILED },
        {   D3DCMP_LESSEQUAL,       ALPHATEST_PASSED, ALPHATEST_PASSED, ALPHATEST_FAILED },
        {   D3DCMP_GREATER,         ALPHATEST_FAILED, ALPHATEST_FAILED, ALPHATEST_PASSED },
        {   D3DCMP_NOTEQUAL,        ALPHATEST_PASSED, ALPHATEST_FAILED, ALPHATEST_PASSED },
        {   D3DCMP_GREATEREQUAL,    ALPHATEST_FAILED, ALPHATEST_PASSED, ALPHATEST_PASSED },
        {   D3DCMP_ALWAYS,          ALPHATEST_PASSED, ALPHATEST_PASSED, ALPHATEST_PASSED },
    };
    unsigned int i, j;
    HRESULT hr;
    DWORD color;
    struct vertex quad[] = {
        {   -1.0,    -1.0,   0.1,    ALPHATEST_PASSED | 0x80000000  },
        {    1.0,    -1.0,   0.1,    ALPHATEST_PASSED | 0x80000000  },
        {   -1.0,     1.0,   0.1,    ALPHATEST_PASSED | 0x80000000  },
        {    1.0,     1.0,   0.1,    ALPHATEST_PASSED | 0x80000000  },
    };
    D3DCAPS9 caps;

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with 0x%08x\n", hr);

    for (j = 0; j < 2; ++j)
    {
        if (j == 1)
        {
            /* Try a pixel shader instead of fixed function. The wined3d code
             * may emulate the alpha test either for performance reasons
             * (floating point RTs) or to work around driver bugs (GeForce
             * 7x00 cards on MacOS). There may be a different codepath for ffp
             * and shader in this case, and the test should cover both. */
            IDirect3DPixelShader9 *ps;
            static const DWORD shader_code[] =
            {
                0xffff0101,                                 /* ps_1_1           */
                0x00000001, 0x800f0000, 0x90e40000,         /* mov r0, v0       */
                0x0000ffff                                  /* end              */
            };
            memset(&caps, 0, sizeof(caps));
            hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
            ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed with 0x%08x\n", hr);
            if(caps.PixelShaderVersion < D3DPS_VERSION(1, 1)) {
                break;
            }

            hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &ps);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_SetPixelShader(device, ps);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed with 0x%08x\n", hr);
            IDirect3DPixelShader9_Release(ps);
        }

        for(i = 0; i < (sizeof(testdata)/sizeof(testdata[0])); i++) {
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAFUNC, testdata[i].func);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, ALPHATEST_FAILED, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAREF, 0x90);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_BeginScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
            if(SUCCEEDED(hr)) {
                hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
                ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
                hr = IDirect3DDevice9_EndScene(device);
                ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
            }
            color = getPixelColor(device, 320, 240);
            ok(color_match(color, testdata[i].color_less, 1), "Alphatest failed. Got color 0x%08x, expected 0x%08x. alpha < ref, func %u\n",
            color, testdata[i].color_less, testdata[i].func);
            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed with 0x%08x\n", hr);

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, ALPHATEST_FAILED, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAREF, 0x80);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_BeginScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
            if(SUCCEEDED(hr)) {
                hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
                ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
                hr = IDirect3DDevice9_EndScene(device);
                ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
            }
            color = getPixelColor(device, 320, 240);
            ok(color_match(color, testdata[i].color_equal, 1), "Alphatest failed. Got color 0x%08x, expected 0x%08x. alpha == ref, func %u\n",
            color, testdata[i].color_equal, testdata[i].func);
            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed with 0x%08x\n", hr);

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, ALPHATEST_FAILED, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHAREF, 0x70);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
            hr = IDirect3DDevice9_BeginScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with 0x%08x\n", hr);
            if(SUCCEEDED(hr)) {
                hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
                ok(SUCCEEDED(hr), "DrawPrimitiveUP failed with 0x%08x\n", hr);
                hr = IDirect3DDevice9_EndScene(device);
                ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
            }
            color = getPixelColor(device, 320, 240);
            ok(color_match(color, testdata[i].color_greater, 1), "Alphatest failed. Got color 0x%08x, expected 0x%08x. alpha > ref, func %u\n",
            color, testdata[i].color_greater, testdata[i].func);
            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "IDirect3DDevice9_Present failed with 0x%08x\n", hr);
        }
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed with 0x%08x\n", hr);
}

static void sincos_test(IDirect3DDevice9 *device)
{
    static const DWORD sin_shader_code[] =
    {
        0xfffe0200,                                                                 /* vs_2_0                       */
        0x0200001f, 0x80000000, 0x900f0000,                                         /* dcl_position v0              */
        0x05000051, 0xa00f0002, 0x40490fdb, 0x3f800000, 0x00000000, 0x3f59999a,     /* def c2, 3.14159, 1, 0, 0.85  */
        0x03000005, 0x80010001, 0x90000000, 0xa0000002,                             /* mul r1.x, v0.x, c2.x         */
        0x04000025, 0x80020000, 0x80000001, 0xa0e40000, 0xa0e40001,                 /* sincos r0.y, r1.x, c0, c1    */
        0x02000001, 0xc00d0000, 0x90e40000,                                         /* mov oPos.xzw, v0             */
        0x03000005, 0xc0020000, 0x80550000, 0xa0ff0002,                             /* mul oPos.y, r0.y, c2.w       */
        0x02000001, 0xd00f0000, 0xa0a60002,                                         /* mov oD0, c2.zyzz             */
        0x0000ffff                                                                  /* end                          */
    };
    static const DWORD cos_shader_code[] =
    {
        0xfffe0200,                                                                 /* vs_2_0                       */
        0x0200001f, 0x80000000, 0x900f0000,                                         /* dcl_position v0              */
        0x05000051, 0xa00f0002, 0x40490fdb, 0x3f800000, 0x00000000, 0x3f59999a,     /* def c2, 3.14159, 1, 0, 0.85  */
        0x03000005, 0x80010001, 0x90000000, 0xa0000002,                             /* mul r1.x, v0.x, c2.x         */
        0x04000025, 0x80010000, 0x80000001, 0xa0e40000, 0xa0e40001,                 /* sincos r0.x, r1.x, c0, c1    */
        0x02000001, 0xc00d0000, 0x90e40000,                                         /* mov oPos.xzw, v0             */
        0x03000005, 0xc0020000, 0x80000000, 0xa0ff0002,                             /* mul oPos.y, r0.x, c2.w       */
        0x02000001, 0xd00f0000, 0xa0a90002,                                         /* mov oD0, c2.yzzz             */
        0x0000ffff                                                                  /* end                          */
    };
    IDirect3DVertexShader9 *sin_shader, *cos_shader;
    HRESULT hr;
    struct {
        float x, y, z;
    } data[1280];
    unsigned int i;
    float sincosc1[4] = {D3DSINCOSCONST1};
    float sincosc2[4] = {D3DSINCOSCONST2};

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexShader(device, sin_shader_code, &sin_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, cos_shader_code, &cos_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, sincosc1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF failed with 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, sincosc2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF failed with 0x%08x\n", hr);

    /* Generate a point from -1 to 1 every 0.5 pixels */
    for(i = 0; i < 1280; i++) {
        data[i].x = (-640.0 + i) / 640.0;
        data[i].y = 0.0;
        data[i].z = 0.1;
    }

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetVertexShader(device, sin_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1280, data, sizeof(*data));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed with 0x%08x\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, cos_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with 0x%08x\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1280, data, sizeof(*data));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed with 0x%08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with 0x%08x\n", hr);
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Present returned %#x.\n", hr);
    /* TODO: Find a way to properly validate the lines. Precicion issues make this a kinda nasty task */

    IDirect3DDevice9_SetVertexShader(device, NULL);
    IDirect3DVertexShader9_Release(sin_shader);
    IDirect3DVertexShader9_Release(cos_shader);
}

static void loop_index_test(IDirect3DDevice9 *device)
{
    static const DWORD shader_code[] =
    {
        0xfffe0200,                                                 /* vs_2_0                   */
        0x0200001f, 0x80000000, 0x900f0000,                         /* dcl_position v0          */
        0x02000001, 0x800f0000, 0xa0e40000,                         /* mov r0, c0               */
        0x0200001b, 0xf0e40800, 0xf0e40000,                         /* loop aL, i0              */
        0x04000002, 0x800f0000, 0x80e40000, 0xa0e42001, 0xf0e40800, /* add r0, r0, c[aL + 1]    */
        0x0000001d,                                                 /* endloop                  */
        0x02000001, 0xc00f0000, 0x90e40000,                         /* mov oPos, v0             */
        0x02000001, 0xd00f0000, 0x80e40000,                         /* mov oD0, r0              */
        0x0000ffff                                                  /* END                      */
    };
    IDirect3DVertexShader9 *shader;
    HRESULT hr;
    DWORD color;
    const float quad[] = {
        -1.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
        -1.0,    1.0,   0.1,
         1.0,    1.0,   0.1
    };
    const float zero[4] = {0, 0, 0, 0};
    const float one[4] = {1, 1, 1, 1};
    int i0[4] = {2, 10, -3, 0};
    float values[4];

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00ff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, zero, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 2, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 3, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 4, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 5, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 6, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 7, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    values[0] = 1.0;
    values[1] = 1.0;
    values[2] = 0.0;
    values[3] = 0.0;
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 8, values, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 9, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 10, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    values[0] = -1.0;
    values[1] = 0.0;
    values[2] = 0.0;
    values[3] = 0.0;
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 11, values, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 12, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 13, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 14, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 15, one, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantF returned %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexShaderConstantI(device, 0, i0, 1);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShaderConstantI returned %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
       "aL indexing test returned color 0x%08x, expected 0x0000ff00\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with %08x\n", hr);
    IDirect3DVertexShader9_Release(shader);
}

static void sgn_test(IDirect3DDevice9 *device)
{
    static const DWORD shader_code[] =
    {
        0xfffe0200,                                                             /* vs_2_0                       */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position o0              */
        0x05000051, 0xa00f0000, 0xbf000000, 0x00000000, 0x3f000000, 0x41400000, /* def c0, -0.5, 0.0, 0.5, 12.0 */
        0x05000051, 0xa00f0001, 0x3fc00000, 0x00000000, 0x00000000, 0x00000000, /* def c1, 1.5, 0.0, 0.0, 0.0   */
        0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x04000022, 0x800f0000, 0xa0e40000, 0x80e40001, 0x80e40002,             /* sgn r0, c0, r1, r2           */
        0x03000002, 0xd00f0000, 0x80e40000, 0xa0e40001,                         /* add oD0, r0, c1              */
        0x0000ffff                                                              /* end                          */
    };
    IDirect3DVertexShader9 *shader;
    HRESULT hr;
    DWORD color;
    const float quad[] = {
        -1.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
        -1.0,    1.0,   0.1,
         1.0,    1.0,   0.1
    };

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %08x\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00ff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x008000ff, 1),
       "sgn test returned color 0x%08x, expected 0x008000ff\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed with %08x\n", hr);
    IDirect3DVertexShader9_Release(shader);
}

static void viewport_test(IDirect3DDevice9 *device) {
    HRESULT hr;
    DWORD color;
    D3DVIEWPORT9 vp, old_vp;
    BOOL draw_failed = TRUE;
    const float quad[] =
    {
        -0.5,   -0.5,   0.1,
         0.5,   -0.5,   0.1,
        -0.5,    0.5,   0.1,
         0.5,    0.5,   0.1
    };

    memset(&old_vp, 0, sizeof(old_vp));
    hr = IDirect3DDevice9_GetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00ff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %08x\n", hr);

    /* Test a viewport with Width and Height bigger than the surface dimensions
     *
     * TODO: Test Width < surface.width, but X + Width > surface.width
     * TODO: Test Width < surface.width, what happens with the height?
     *
     * The expected behavior is that the viewport behaves like the "default"
     * viewport with X = Y = 0, Width = surface_width, Height = surface_height,
     * MinZ = 0.0, MaxZ = 1.0.
     *
     * Starting with Windows 7 the behavior among driver versions is not
     * consistent. The SetViewport call is accepted on all drivers. Some
     * drivers(older nvidia ones) refuse to draw and return an error. Newer
     * nvidia drivers draw, but use the actual values in the viewport and only
     * display the upper left part on the surface.
     */
    memset(&vp, 0, sizeof(vp));
    vp.X = 0;
    vp.Y = 0;
    vp.Width = 10000;
    vp.Height = 10000;
    vp.MinZ = 0.0;
    vp.MaxZ = 0.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "DrawPrimitiveUP failed (%08x)\n", hr);
        draw_failed = FAILED(hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %08x\n", hr);
    }

    if(!draw_failed)
    {
        color = getPixelColor(device, 158, 118);
        ok(color == 0x00ff0000, "viewport test: (158,118) has color %08x\n", color);
        color = getPixelColor(device, 162, 118);
        ok(color == 0x00ff0000, "viewport test: (162,118) has color %08x\n", color);
        color = getPixelColor(device, 158, 122);
        ok(color == 0x00ff0000, "viewport test: (158,122) has color %08x\n", color);
        color = getPixelColor(device, 162, 122);
        ok(color == 0x00ffffff || broken(color == 0x00ff0000), "viewport test: (162,122) has color %08x\n", color);

        color = getPixelColor(device, 478, 358);
        ok(color == 0x00ffffff || broken(color == 0x00ff0000), "viewport test: (478,358 has color %08x\n", color);
        color = getPixelColor(device, 482, 358);
        ok(color == 0x00ff0000, "viewport test: (482,358) has color %08x\n", color);
        color = getPixelColor(device, 478, 362);
        ok(color == 0x00ff0000, "viewport test: (478,362) has color %08x\n", color);
        color = getPixelColor(device, 482, 362);
        ok(color == 0x00ff0000, "viewport test: (482,362) has color %08x\n", color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);
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
static void depth_clamp_test(IDirect3DDevice9 *device)
{
    const struct tvertex quad1[] =
    {
        {  0.0f,   0.0f,  5.0f, 1.0f, 0xff002b7f},
        {640.0f,   0.0f,  5.0f, 1.0f, 0xff002b7f},
        {  0.0f, 480.0f,  5.0f, 1.0f, 0xff002b7f},
        {640.0f, 480.0f,  5.0f, 1.0f, 0xff002b7f},
    };
    const struct tvertex quad2[] =
    {
        {  0.0f, 300.0f, 10.0f, 1.0f, 0xfff9e814},
        {640.0f, 300.0f, 10.0f, 1.0f, 0xfff9e814},
        {  0.0f, 360.0f, 10.0f, 1.0f, 0xfff9e814},
        {640.0f, 360.0f, 10.0f, 1.0f, 0xfff9e814},
    };
    const struct tvertex quad3[] =
    {
        {112.0f, 108.0f,  5.0f, 1.0f, 0xffffffff},
        {208.0f, 108.0f,  5.0f, 1.0f, 0xffffffff},
        {112.0f, 204.0f,  5.0f, 1.0f, 0xffffffff},
        {208.0f, 204.0f,  5.0f, 1.0f, 0xffffffff},
    };
    const struct tvertex quad4[] =
    {
        { 42.0f,  41.0f, 10.0f, 1.0f, 0xffffffff},
        {112.0f,  41.0f, 10.0f, 1.0f, 0xffffffff},
        { 42.0f, 108.0f, 10.0f, 1.0f, 0xffffffff},
        {112.0f, 108.0f, 10.0f, 1.0f, 0xffffffff},
    };
    const struct vertex quad5[] =
    {
        { -0.5f,   0.5f, 10.0f,       0xff14f914},
        {  0.5f,   0.5f, 10.0f,       0xff14f914},
        { -0.5f,  -0.5f, 10.0f,       0xff14f914},
        {  0.5f,  -0.5f, 10.0f,       0xff14f914},
    };
    const struct vertex quad6[] =
    {
        { -1.0f,   0.5f, 10.0f,      0xfff91414},
        {  1.0f,   0.5f, 10.0f,      0xfff91414},
        { -1.0f,  0.25f, 10.0f,      0xfff91414},
        {  1.0f,  0.25f, 10.0f,      0xfff91414},
    };

    D3DVIEWPORT9 vp;
    D3DCOLOR color;
    D3DCAPS9 caps;
    HRESULT hr;

    vp.X = 0;
    vp.Y = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0;
    vp.MaxZ = 7.5;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetViewport(device, &vp);
    if(FAILED(hr))
    {
        /* Windows 7 rejects MaxZ > 1.0, Windows XP allows it. This doesn't break
         * the tests because the 7.5 is just intended to show that it doesn't have
         * any influence on the drawing or D3DRS_CLIPPING = FALSE. Set an accepted
         * viewport and continue.
         */
        ok(broken(hr == D3DERR_INVALIDCALL), "D3D rejected maxZ > 1.0\n");
        vp.MaxZ = 1.0;
        hr = IDirect3DDevice9_SetViewport(device, &vp);
    }
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(*quad3));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(*quad4));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad5, sizeof(*quad5));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad6, sizeof(*quad6));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);
}

static void depth_bounds_test(void)
{
    const struct tvertex quad1[] =
    {
        {    0,    0, 0.0f, 1, 0xfff9e814},
        {  640,    0, 0.0f, 1, 0xfff9e814},
        {    0,  480, 1.0f, 1, 0xfff9e814},
        {  640,  480, 1.0f, 1, 0xfff9e814},
    };
    const struct tvertex quad2[] =
    {
        {    0,    0,  0.6f, 1, 0xff002b7f},
        {  640,    0,  0.6f, 1, 0xff002b7f},
        {    0,  480,  0.6f, 1, 0xff002b7f},
        {  640,  480,  0.6f, 1, 0xff002b7f},
    };
    const struct tvertex quad3[] =
    {
        {    0,  100, 0.6f, 1, 0xfff91414},
        {  640,  100, 0.6f, 1, 0xfff91414},
        {    0,  160, 0.6f, 1, 0xfff91414},
        {  640,  160, 0.6f, 1, 0xfff91414},
    };

    union {
        DWORD d;
        float f;
    } tmpvalue;

    IDirect3DSurface9 *offscreen_surface = NULL;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0,  D3DRTYPE_SURFACE, MAKEFOURCC('N','V','D','B')) != D3D_OK)
    {
        skip("No NVDB (depth bounds test) support, skipping tests.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            MAKEFOURCC('N','V','D','B'), D3DPOOL_DEFAULT, &offscreen_surface, NULL);
    ok(FAILED(hr), "Able to create surface, hr %#x.\n", hr);
    if (offscreen_surface) IDirect3DSurface9_Release(offscreen_surface);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);


    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ADAPTIVETESS_X, MAKEFOURCC('N','V','D','B'));
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    tmpvalue.f = 0.625;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ADAPTIVETESS_Z, tmpvalue.d);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    tmpvalue.f = 0.75;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ADAPTIVETESS_W, tmpvalue.d);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    tmpvalue.f = 0.75;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ADAPTIVETESS_Z, tmpvalue.d);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(*quad3));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ADAPTIVETESS_X, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    color = getPixelColor(device, 150, 130);
    ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 200);
    ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 300-5);
    ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 300+5);
    ok(color_match(color, 0x00002b7f, 1), "color 0x%08x.\n", color);/**/
    color = getPixelColor(device, 150, 330);
    ok(color_match(color, 0x00002b7f, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 360-5);
    ok(color_match(color, 0x00002b7f, 1), "color 0x%08x.\n", color);/**/
    color = getPixelColor(device, 150, 360+5);
    ok(color_match(color, 0x00f9e814, 1), "color 0x%08x.\n", color);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void depth_buffer_test(void)
{
    static const struct vertex quad1[] =
    {
        { -1.0,  1.0, 0.33f, 0xff00ff00},
        {  1.0,  1.0, 0.33f, 0xff00ff00},
        { -1.0, -1.0, 0.33f, 0xff00ff00},
        {  1.0, -1.0, 0.33f, 0xff00ff00},
    };
    static const struct vertex quad2[] =
    {
        { -1.0,  1.0, 0.50f, 0xffff00ff},
        {  1.0,  1.0, 0.50f, 0xffff00ff},
        { -1.0, -1.0, 0.50f, 0xffff00ff},
        {  1.0, -1.0, 0.50f, 0xffff00ff},
    };
    static const struct vertex quad3[] =
    {
        { -1.0,  1.0, 0.66f, 0xffff0000},
        {  1.0,  1.0, 0.66f, 0xffff0000},
        { -1.0, -1.0, 0.66f, 0xffff0000},
        {  1.0, -1.0, 0.66f, 0xffff0000},
    };
    static const DWORD expected_colors[4][4] =
    {
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x00ff0000},
        {0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000},
    };

    IDirect3DSurface9 *backbuffer, *rt1, *rt2, *rt3;
    IDirect3DDevice9 *device;
    unsigned int i, j;
    D3DVIEWPORT9 vp;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
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

    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 320, 240, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt1, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 480, 360, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt2, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt3, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt3);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt1);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt2);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(*quad3));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface9_Release(backbuffer);
    IDirect3DSurface9_Release(rt3);
    IDirect3DSurface9_Release(rt2);
    IDirect3DSurface9_Release(rt1);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Test that partial depth copies work the way they're supposed to. The clear
 * on rt2 only needs a partial copy of the onscreen depth/stencil buffer, and
 * the following draw should only copy back the part that was modified. */
static void depth_buffer2_test(void)
{
    static const struct vertex quad[] =
    {
        { -1.0,  1.0, 0.66f, 0xffff0000},
        {  1.0,  1.0, 0.66f, 0xffff0000},
        { -1.0, -1.0, 0.66f, 0xffff0000},
        {  1.0, -1.0, 0.66f, 0xffff0000},
    };

    IDirect3DSurface9 *backbuffer, *rt1, *rt2;
    IDirect3DDevice9 *device;
    unsigned int i, j;
    D3DVIEWPORT9 vp;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
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

    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt1, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 480, 360, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt2, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt1);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 0.5f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt2);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = getPixelColor(device, x, y);
            ok(color_match(color, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00), 0),
                    "Expected color 0x0000ff00 at %u,%u, got 0x%08x.\n", x, y, color);
        }
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface9_Release(backbuffer);
    IDirect3DSurface9_Release(rt2);
    IDirect3DSurface9_Release(rt1);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void depth_blit_test(void)
{
    static const struct vertex quad1[] =
    {
        { -1.0,  1.0, 0.33f, 0xff00ff00},
        {  1.0,  1.0, 0.33f, 0xff00ff00},
        { -1.0, -1.0, 0.33f, 0xff00ff00},
        {  1.0, -1.0, 0.33f, 0xff00ff00},
    };
    static const struct vertex quad2[] =
    {
        { -1.0,  1.0, 0.66f, 0xff0000ff},
        {  1.0,  1.0, 0.66f, 0xff0000ff},
        { -1.0, -1.0, 0.66f, 0xff0000ff},
        {  1.0, -1.0, 0.66f, 0xff0000ff},
    };
    static const DWORD expected_colors[4][4] =
    {
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x000000ff, 0x000000ff, 0x0000ff00, 0x00ff0000},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x00ff0000},
        {0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000},
    };

    IDirect3DSurface9 *backbuffer, *ds1, *ds2, *ds3;
    IDirect3DDevice9 *device;
    RECT src_rect, dst_rect;
    unsigned int i, j;
    D3DVIEWPORT9 vp;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
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

    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds1);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8, 0, 0, FALSE, &ds2, NULL);
    ok(SUCCEEDED(hr), "CreateDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds2);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 320, 240, D3DFMT_D24S8, 0, 0, FALSE, &ds3, NULL);
    ok(SUCCEEDED(hr), "CreateDepthStencilSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);
    SetRect(&dst_rect, 0, 0, 480, 360);
    hr = IDirect3DDevice9_Clear(device, 1, (D3DRECT *)&dst_rect, D3DCLEAR_ZBUFFER, 0, 0.5f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirect3DDevice9_Clear(device, 1, (D3DRECT *)&dst_rect, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    /* Partial blit. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirect3DDevice9_StretchRect(device, ds2, &src_rect, ds1, &dst_rect, D3DTEXF_POINT);
    ok(hr == D3DERR_INVALIDCALL, "StretchRect returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Flipped. */
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 480, 640, 0);
    hr = IDirect3DDevice9_StretchRect(device, ds2, &src_rect, ds1, &dst_rect, D3DTEXF_POINT);
    ok(hr == D3DERR_INVALIDCALL, "StretchRect returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Full, explicit. */
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirect3DDevice9_StretchRect(device, ds2, &src_rect, ds1, &dst_rect, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    /* Filtered blit. */
    hr = IDirect3DDevice9_StretchRect(device, ds2, NULL, ds1, NULL, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    /* Depth -> color blit.*/
    hr = IDirect3DDevice9_StretchRect(device, ds2, NULL, backbuffer, NULL, D3DTEXF_POINT);
    ok(hr == D3DERR_INVALIDCALL, "StretchRect returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    IDirect3DSurface9_Release(backbuffer);
    /* Full surface, different sizes */
    hr = IDirect3DDevice9_StretchRect(device, ds3, NULL, ds1, NULL, D3DTEXF_POINT);
    ok(hr == D3DERR_INVALIDCALL, "StretchRect returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = IDirect3DDevice9_StretchRect(device, ds1, NULL, ds3, NULL, D3DTEXF_POINT);
    ok(hr == D3DERR_INVALIDCALL, "StretchRect returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds1);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_StretchRect(device, ds2, NULL, ds1, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
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

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface9_Release(ds3);
    IDirect3DSurface9_Release(ds2);
    IDirect3DSurface9_Release(ds1);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void intz_test(void)
{
    static const DWORD ps_code[] =
    {
        0xffff0200,                                                             /* ps_2_0                       */
        0x0200001f, 0x90000000, 0xa00f0800,                                     /* dcl_2d s0                    */
        0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                       */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0.0, 0.0, 0.0, 1.0   */
        0x02000001, 0x800f0001, 0xa0e40000,                                     /* mov r1, c0                   */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texld r0, t0, s0             */
        0x02000001, 0x80010001, 0x80e40000,                                     /* mov r1.x, r0                 */
        0x03010042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texldp r0, t0, s0            */
        0x02000001, 0x80020001, 0x80000000,                                     /* mov r1.y, r0.x               */
        0x02000001, 0x800f0800, 0x80e40001,                                     /* mov oC0, r1                  */
        0x0000ffff,                                                             /* end                          */
    };
    struct
    {
        float x, y, z;
        float s, t, p, q;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    },
    half_quad_1[] =
    {
        { -1.0f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    },
    half_quad_2[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    };
    struct
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

    IDirect3DSurface9 *original_rt, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DPixelShader9 *ps;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *ds;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    UINT i;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, MAKEFOURCC('I','N','T','Z'))))
    {
        skip("No INTZ support, skipping INTZ test.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        skip("No pixel shader 2.0 support, skipping INTZ test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No unconditional NP2 texture support, skipping INTZ test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4(0));
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);

    /* Render offscreen, using the INTZ texture as depth buffer */
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Setup the depth/stencil surface. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    /* Render onscreen while using the INTZ texture as depth buffer */
    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture, NULL);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Setup the depth/stencil surface. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    /* Render offscreen, then onscreen, and finally check the INTZ texture in both areas */
    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture, NULL);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Setup the depth/stencil surface. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, half_quad_1, sizeof(*half_quad_1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, half_quad_2, sizeof(*half_quad_2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    IDirect3DTexture9_Release(texture);
    IDirect3DPixelShader9_Release(ps);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(rt);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void shadow_test(void)
{
    static const DWORD ps_code[] =
    {
        0xffff0200,                                                             /* ps_2_0                       */
        0x0200001f, 0x90000000, 0xa00f0800,                                     /* dcl_2d s0                    */
        0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                       */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0.0, 0.0, 0.0, 1.0   */
        0x02000001, 0x800f0001, 0xa0e40000,                                     /* mov r1, c0                   */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texld r0, t0, s0             */
        0x02000001, 0x80010001, 0x80e40000,                                     /* mov r1.x, r0                 */
        0x03010042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texldp r0, t0, s0            */
        0x02000001, 0x80020001, 0x80000000,                                     /* mov r1.y, r0.x               */
        0x02000001, 0x800f0800, 0x80e40001,                                     /* mov 0C0, r1                  */
        0x0000ffff,                                                             /* end                          */
    };
    struct
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
        {D3DFMT_D32F_LOCKABLE,  "D3DFMT_D32F_LOCKABLE"},
        {D3DFMT_D24FS8,         "D3DFMT_D24FS8"},
    };
    struct
    {
        float x, y, z;
        float s, t, p, q;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
    };
    struct
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

    IDirect3DSurface9 *original_ds, *original_rt, *rt;
    IDirect3DPixelShader9 *ps;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    UINT i;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        skip("No pixel shader 2.0 support, skipping shadow test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &original_ds);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 1024, 1024, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4(0));
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); ++i)
    {
        D3DFORMAT format = formats[i].format;
        IDirect3DTexture9 *texture;
        IDirect3DSurface9 *ds;
        unsigned int j;

        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, format)))
            continue;

        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1,
                D3DUSAGE_DEPTHSTENCIL, format, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);

        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &ds);
        ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
        ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
        ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

        /* Setup the depth/stencil surface. */
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
        ok(SUCCEEDED(hr), "SetDepthStencilSurface failed, hr %#x.\n", hr);
        IDirect3DSurface9_Release(ds);

        hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
        ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, ps);
        ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

        /* Do the actual shadow mapping. */
        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
        IDirect3DTexture9_Release(texture);

        for (j = 0; j < sizeof(expected_colors) / sizeof(*expected_colors); ++j)
        {
            D3DCOLOR color = getPixelColor(device, expected_colors[j].x, expected_colors[j].y);
            ok(color_match(color, expected_colors[j].color, 0),
                    "Expected color 0x%08x at (%u, %u) for format %s, got 0x%08x.\n",
                    expected_colors[j].color, expected_colors[j].x, expected_colors[j].y,
                    formats[i].name, color);
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);
    }

    IDirect3DPixelShader9_Release(ps);
    IDirect3DSurface9_Release(original_ds);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(rt);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void clip_planes(IDirect3DDevice9 *device, const char *test_name)
{
    const struct vertex quad1[] =
    {
        {-1.0f, -1.0f, 0.0f, 0xfff9e814},
        { 1.0f, -1.0f, 0.0f, 0xfff9e814},
        {-1.0f,  1.0f, 0.0f, 0xfff9e814},
        { 1.0f,  1.0f, 0.0f, 0xfff9e814},
    };
    const struct vertex quad2[] =
    {
        {-1.0f, -1.0f, 0.0f, 0xff002b7f},
        { 1.0f, -1.0f, 0.0f, 0xff002b7f},
        {-1.0f,  1.0f, 0.0f, 0xff002b7f},
        { 1.0f,  1.0f, 0.0f, 0xff002b7f},
    };
    D3DCOLOR color;
    HRESULT hr;

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 1.0, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPLANEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(*quad1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPLANEENABLE, 0x1);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(*quad2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    color = getPixelColor(device, 1, 240);
    ok(color_match(color, 0x00002b7f, 1), "%s test: color 0x%08x.\n", test_name, color);
    color = getPixelColor(device, 638, 240);
    ok(color_match(color, 0x00002b7f, 1), "%s test: color 0x%08x.\n", test_name, color);

    color = getPixelColor(device, 1, 241);
    ok(color_match(color, 0x00f9e814, 1), "%s test: color 0x%08x.\n", test_name, color);
    color = getPixelColor(device, 638, 241);
    ok(color_match(color, 0x00f9e814, 1), "%s test: color 0x%08x.\n", test_name, color);
}

static void clip_planes_test(IDirect3DDevice9 *device)
{
    const float plane0[4] = {0.0f, 1.0f, 0.0f, 0.5f / 480.0f}; /* a quarter-pixel offset */

    static const DWORD shader_code[] =
    {
        0xfffe0200,                             /* vs_2_0 */
        0x0200001f, 0x80000000, 0x900f0000,     /* dcl_position v0 */
        0x0200001f, 0x8000000a, 0x900f0001,     /* dcl_color0 v1 */
        0x02000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0 */
        0x02000001, 0xd00f0000, 0x90e40001,     /* mov oD0, v1 */
        0x0000ffff                              /* end */
    };
    IDirect3DVertexShader9 *shader;

    IDirect3DTexture9 *offscreen = NULL;
    IDirect3DSurface9 *offscreen_surface, *original_rt;
    HRESULT hr;

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed, hr=%08x\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed, hr=%08x\n", hr);

    IDirect3DDevice9_SetClipPlane(device, 0, plane0);

    clip_planes(device, "Onscreen FFP");

    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &offscreen, NULL);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(offscreen, 0, &offscreen_surface);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen_surface);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);

    clip_planes(device, "Offscreen FFP");

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);

    clip_planes(device, "Onscreen vertex shader");

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen_surface);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);

    clip_planes(device, "Offscreen vertex shader");

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPLANEENABLE, 0);
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader failed, hr=%08x\n", hr);
    IDirect3DVertexShader9_Release(shader);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%08x\n", hr);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(offscreen_surface);
    IDirect3DTexture9_Release(offscreen);
}

static void fp_special_test(void)
{
    /* Microsoft's assembler generates nan and inf with "1.#QNAN" and "1.#INF." respectively */
    static const DWORD vs_header[] =
    {
        0xfffe0200,                                                             /* vs_2_0                       */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f000000, 0x3f800000, 0x40000000, /* def c0, 0.0, 0.5, 1.0, 2.0   */
        0x05000051, 0xa00f0001, 0x7fc00000, 0xff800000, 0x7f800000, 0x00000000, /* def c1, nan, -inf, inf, 0    */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x0200001f, 0x80000005, 0x900f0001,                                     /* dcl_texcoord0 v1             */
    };

    static const DWORD vs_log[] = {0x0200000f, 0x80010000, 0x90000001};         /* log r0.x, v1.x               */
    static const DWORD vs_pow[] =
            {0x03000020, 0x80010000, 0x90000001, 0x90000001};                   /* pow r0.x, v1.x, v1.x         */
    static const DWORD vs_nrm[] = {0x02000024, 0x80070000, 0x90000001};         /* nrm r0.xyz, v1.x             */
    static const DWORD vs_rcp1[] = {0x02000006, 0x80010000, 0x90000001};        /* rcp r0.x, v1.x               */
    static const DWORD vs_rcp2[] = {0x02000006, 0x80010000, 0x91000001};        /* rcp r0.x, -v1.x              */
    static const DWORD vs_rsq1[] = {0x02000007, 0x80010000, 0x90000001};        /* rsq r0.x, v1.x               */
    static const DWORD vs_rsq2[] = {0x02000007, 0x80010000, 0x91000001};        /* rsq r0.x, -v1.x              */
    static const DWORD vs_lit[] = {0x02000010, 0x800f0000, 0x90000001,          /* lit r0, v1.xxxx              */
            0x02000001, 0x80010000, 0x80aa0000};                                /* mov r0.x, v0.z               */
    static const DWORD vs_def1[] = {0x02000001, 0x80010000, 0xa0000001};        /* mov r0.x, c1.x               */
    static const DWORD vs_def2[] = {0x02000001, 0x80010000, 0xa0550001};        /* mov r0.x, c1.y               */
    static const DWORD vs_def3[] = {0x02000001, 0x80010000, 0xa0aa0001};        /* mov r0.x, c1.z               */

    static const DWORD vs_footer[] =
    {
        0x03000005, 0x80020000, 0x80000000, 0xa0ff0000,                         /* mul r0.y, r0.x, c0.w         */
        0x0300000d, 0x80040000, 0x80000000, 0x80550000,                         /* sge r0.z, r0.x, r0.y         */
        0x0300000d, 0x80020000, 0x80e40000, 0x80000000,                         /* sge r0.y, r0, r0.x           */
        0x03000005, 0x80040000, 0x80550000, 0x80e40000,                         /* mul r0.z, r0.y, r0           */
        0x0300000b, 0x80080000, 0x81aa0000, 0x80aa0000,                         /* max r0.w, -r0.z, r0.z        */
        0x0300000c, 0x80020000, 0x80000000, 0x80000000,                         /* slt r0.y, r0.x, r0.x         */
        0x03000002, 0x80040000, 0x80550000, 0x80550000,                         /* add r0.z, r0.y, r0.y         */
        0x0300000c, 0x80020000, 0xa0000000, 0x80ff0000,                         /* slt r0.y, c0.x, r0.w         */
        0x0300000b, 0x80080000, 0x81aa0000, 0x80aa0000,                         /* max r0.w, -r0.z, r0.z        */
        0x03000002, 0x80040000, 0x81550000, 0xa0e40000,                         /* add r0.z, -r0.y, c0          */
        0x0300000c, 0x80080000, 0xa0000000, 0x80e40000,                         /* slt r0.w, c0.x, r0           */
        0x03000005, 0x80040000, 0x80ff0000, 0x80e40000,                         /* mul r0.z, r0.w, r0           */
        0x04000004, 0x80020000, 0x80aa0000, 0xa0e40000, 0x80e40000,             /* mad r0.y, r0.z, c0, r0       */
        0x02000001, 0xe0030000, 0x80e40000,                                     /* mov oT0.xy, r0               */
        0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x0000ffff,                                                             /* end                          */
    };

    static const struct
    {
        const char *name;
        const DWORD *ops;
        DWORD size;
        D3DCOLOR r500;
        D3DCOLOR r600;
        D3DCOLOR nv40;
        D3DCOLOR nv50;
    }
    vs_body[] =
    {
        /* The basic ideas here are:
         *     2.0 * +/-INF == +/-INF
         *     NAN != NAN
         *
         * The vertex shader value is written to the red component, with 0.0
         * and +/-INF mapping to 0xff, and NAN to 0x7f. Anything else should
         * result in 0x00. The pixel shader value is written to the green
         * component, but here 0.0 also results in 0x00. The actual value is
         * written to the blue component.
         *
         * There are considerable differences between graphics cards in how
         * these are handled, but pow and nrm never generate INF or NAN. */
        {"log",     vs_log,     sizeof(vs_log),     0x00000000, 0x00000000, 0x00ff0000, 0x00ff7f00},
        {"pow",     vs_pow,     sizeof(vs_pow),     0x000000ff, 0x000000ff, 0x0000ff00, 0x000000ff},
        {"nrm",     vs_nrm,     sizeof(vs_nrm),     0x00ff0000, 0x00ff0000, 0x0000ff00, 0x00ff0000},
        {"rcp1",    vs_rcp1,    sizeof(vs_rcp1),    0x000000ff, 0x000000ff, 0x00ff00ff, 0x00ff7f00},
        {"rcp2",    vs_rcp2,    sizeof(vs_rcp2),    0x000000ff, 0x00000000, 0x00ff0000, 0x00ff7f00},
        {"rsq1",    vs_rsq1,    sizeof(vs_rsq1),    0x000000ff, 0x000000ff, 0x00ff00ff, 0x00ff7f00},
        {"rsq2",    vs_rsq2,    sizeof(vs_rsq2),    0x000000ff, 0x000000ff, 0x00ff00ff, 0x00ff7f00},
        {"lit",     vs_lit,     sizeof(vs_lit),     0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000},
        {"def1",    vs_def1,    sizeof(vs_def1),    0x000000ff, 0x00007f00, 0x0000ff00, 0x00007f00},
        {"def2",    vs_def2,    sizeof(vs_def2),    0x00ff0000, 0x00ff7f00, 0x00ff0000, 0x00ff7f00},
        {"def3",    vs_def3,    sizeof(vs_def3),    0x00ff00ff, 0x00ff7f00, 0x00ff00ff, 0x00ff7f00},
    };

    static const DWORD ps_code[] =
    {
        0xffff0200,                                                             /* ps_2_0                       */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f000000, 0x3f800000, 0x40000000, /* def c0, 0.0, 0.5, 1.0, 2.0   */
        0x0200001f, 0x80000000, 0xb0030000,                                     /* dcl t0.xy                    */
        0x0300000b, 0x80010001, 0xb0e40000, 0xa0e40000,                         /* max r1.x, t0, c0             */
        0x0300000a, 0x80010000, 0xb0e40000, 0xa0e40000,                         /* min r0.x, t0, c0             */
        0x03000002, 0x80010000, 0x80e40000, 0x81e40001,                         /* add r0.x, r0, -r1            */
        0x04000004, 0x80010001, 0xb0e40000, 0xa0ff0000, 0xb1e40000,             /* mad r1.x, t0, c0.w. -t0      */
        0x02000023, 0x80010002, 0x80e40001,                                     /* abs r2.x, r1                 */
        0x02000023, 0x80010000, 0x80e40000,                                     /* abs r0.x, r0                 */
        0x02000023, 0x80010001, 0xb0e40000,                                     /* abs r1.x, t0                 */
        0x04000058, 0x80010002, 0x81e40002, 0xa0aa0000, 0xa0e40000,             /* cmp r2.x, -r2, c0.z, c0      */
        0x02000023, 0x80010002, 0x80e40002,                                     /* abs r2.x, r2                 */
        0x04000058, 0x80010001, 0x81e40001, 0xa0aa0000, 0xa0e40000,             /* cmp r1.x, -r1, c0.z, c0      */
        0x02000023, 0x80010001, 0x80e40001,                                     /* abs r1.x, r1                 */
        0x04000058, 0x80010003, 0x81e40002, 0xa0aa0000, 0xa0e40000,             /* cmp r3.x, -r2, c0.z, c0      */
        0x04000058, 0x80010002, 0x81e40001, 0xa0aa0000, 0xa0e40000,             /* cmp r2.x, -r1, c0.z, c0      */
        0x04000058, 0x80010000, 0x81e40000, 0xa0550000, 0xa0e40000,             /* cmp r0.x, -r0, c0.y, c0      */
        0x03000005, 0x80010002, 0x80e40002, 0x80e40003,                         /* mul r2.x, r2, r3             */
        0x04000058, 0x80010000, 0x81e40002, 0xa0aa0000, 0x80e40000,             /* cmp r0.x, -r2, c0.z, r0      */
        0x04000058, 0x80020000, 0x81000001, 0x80000000, 0xa0000000,             /* cmp r0.y, -r1.x, r0.x, c0.x  */
        0x02000001, 0x80050000, 0xb0c90000,                                     /* mov r0.xz, t0.yzxw           */
        0x02000001, 0x80080000, 0xa0aa0000,                                     /* mov r0.w, c0.z               */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                  */
        0x0000ffff,                                                             /* end                          */
    };

    struct
    {
        float x, y, z;
        float s;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f},
        {  1.0f,  1.0f, 1.0f, 0.0f},
        { -1.0f, -1.0f, 0.0f, 0.0f},
        {  1.0f, -1.0f, 1.0f, 0.0f},
    };

    IDirect3DPixelShader9 *ps;
    IDirect3DDevice9 *device;
    UINT body_size = 0;
    IDirect3D9 *d3d;
    DWORD *vs_code;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    UINT i;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0) || caps.VertexShaderVersion < D3DVS_VERSION(2, 0))
    {
        skip("No shader model 2.0 support, skipping floating point specials test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE1(0));
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(vs_body) / sizeof(*vs_body); ++i)
    {
        if (vs_body[i].size > body_size) body_size = vs_body[i].size;
    }

    vs_code = HeapAlloc(GetProcessHeap(), 0, sizeof(vs_header) + body_size + sizeof(vs_footer));
    memcpy(vs_code, vs_header, sizeof(vs_header));

    for (i = 0; i < sizeof(vs_body) / sizeof(*vs_body); ++i)
    {
        DWORD offset = sizeof(vs_header) / sizeof(*vs_header);
        IDirect3DVertexShader9 *vs;
        D3DCOLOR color;

        memcpy(vs_code + offset, vs_body[i].ops, vs_body[i].size);
        offset += vs_body[i].size / sizeof(*vs_body[i].ops);
        memcpy(vs_code + offset, vs_footer, sizeof(vs_footer));

        hr = IDirect3DDevice9_CreateVertexShader(device, vs_code, &vs);
        ok(SUCCEEDED(hr), "CreateVertexShader failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetVertexShader(device, vs);
        ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color_match(color, vs_body[i].r500, 1)
                || color_match(color, vs_body[i].r600, 1)
                || color_match(color, vs_body[i].nv40, 1)
                || color_match(color, vs_body[i].nv50, 1),
                "Expected color 0x%08x, 0x%08x, 0x%08x or 0x%08x for instruction \"%s\", got 0x%08x.\n",
                vs_body[i].r500, vs_body[i].r600, vs_body[i].nv40, vs_body[i].nv50, vs_body[i].name, color);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, NULL);
        ok(SUCCEEDED(hr), "SetVertexShader failed, hr %#x.\n", hr);
        IDirect3DVertexShader9_Release(vs);
    }

    HeapFree(GetProcessHeap(), 0, vs_code);

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);
    IDirect3DPixelShader9_Release(ps);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void srgbwrite_format_test(void)
{
    IDirect3D9 *d3d;
    IDirect3DSurface9 *rt, *backbuffer;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    int i;
    DWORD color_rgb = 0x00808080, color_srgb = 0x00bcbcbc, color;
    static const struct
    {
        D3DFORMAT fmt;
        const char *name;
    }
    formats[] =
    {
        { D3DFMT_R5G6B5, "D3DFMT_R5G6B5" },
        { D3DFMT_X8R8G8B8, "D3DFMT_X8R8G8B8" },
        { D3DFMT_A8R8G8B8, "D3DFMT_A8R8G8B8" },
        { D3DFMT_A16B16G16R16F, "D3DFMT_A16B16G16R16F" },
        { D3DFMT_A32B32G32R32F, "D3DFMT_A32B32G32R32F" },
    };
    static const struct
    {
        float x, y, z;
        float u, v;
    }
    quad[] =
    {
        {-1.0f,  -1.0f,  0.1f,   0.0f,   0.0f},
        {-1.0f,   1.0f,  0.1f,   1.0f,   0.0f},
        { 1.0f,  -1.0f,  0.1f,   0.0f,   1.0f},
        { 1.0f,   1.0f,  0.1f,   1.0f,   1.0f}
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "GetBackBuffer failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_TEXTUREFACTOR, 0x80808080);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    for(i = 0; i < (sizeof(formats) / sizeof(*formats)); i++)
    {
        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, formats[i].fmt)))
        {
            skip("Format %s not supported as render target, skipping test.\n",
                    formats[i].name);
            continue;
        }

        hr = IDirect3DDevice9_CreateTexture(device, 8, 8, 1, D3DUSAGE_RENDERTARGET,
                formats[i].fmt, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff0000, 1.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &rt);
        ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
        ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRGBWRITEENABLE, TRUE);
            ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
            ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
            ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

            hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRGBWRITEENABLE, FALSE);
            ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
            ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
            ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
            ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
            ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);
            hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
            ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);
        }

        IDirect3DSurface9_Release(rt);
        IDirect3DTexture9_Release(texture);

        color = getPixelColor(device, 360, 240);
        if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                    D3DUSAGE_QUERY_SRGBWRITE,
                                    D3DRTYPE_TEXTURE, formats[i].fmt) == D3D_OK)
        {
            /* Big slop for R5G6B5 */
            ok(color_match(color, color_srgb, 5), "Format %s supports srgb, expected color 0x%08x, got 0x%08x\n",
                formats[i].name, color_srgb, color);
        }
        else
        {
            /* Big slop for R5G6B5 */
            ok(color_match(color, color_rgb, 5), "Format %s does not support srgb, expected color 0x%08x, got 0x%08x\n",
                formats[i].name, color_rgb, color);
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);
    }

    IDirect3DSurface9_Release(backbuffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void ds_size_test(IDirect3DDevice9 *device)
{
    IDirect3DSurface9 *ds, *rt, *old_rt, *old_ds, *readback;
    HRESULT hr;
    DWORD num_passes;
    struct
    {
        float x, y, z;
    }
    quad[] =
    {
        {-1.0,  -1.0,   0.0 },
        {-1.0,   1.0,   0.0 },
        { 1.0,  -1.0,   0.0 },
        { 1.0,   1.0,   0.0 }
    };

    hr = IDirect3DDevice9_CreateRenderTarget(device, 64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 32, 32, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, 0, TRUE, &ds, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &readback, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateOffscreenPlainSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_ValidateDevice(device, &num_passes);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_ValidateDevice failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &old_rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &old_ds);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_GetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_ValidateDevice(device, &num_passes);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_ValidateDevice failed, hr %#x.\n", hr);

    /* The D3DCLEAR_TARGET clear works. D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER returns OK,
     * but does not change the surface's contents. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x000000FF, 0.0f, 0);
    ok(SUCCEEDED(hr), "Target clear failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 0.2f, 0);
    ok(SUCCEEDED(hr), "Z Buffer clear failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff0000, 0.5f, 0);
    ok(SUCCEEDED(hr), "Target and Z Buffer clear failed, hr %#x.\n", hr);

    /* Nvidia does not clear the surface(The color is still 0x000000ff), AMD does(the color is 0x00ff0000) */

    /* Turning on any depth-related state results in a ValidateDevice failure */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_ValidateDevice(device, &num_passes);
    ok(hr == D3DERR_CONFLICTINGRENDERSTATE || hr == D3D_OK, "IDirect3DDevice9_ValidateDevice returned %#x, expected "
        "D3DERR_CONFLICTINGRENDERSTATE.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_ValidateDevice(device, &num_passes);
    ok(hr == D3DERR_CONFLICTINGRENDERSTATE || hr == D3D_OK, "IDirect3DDevice9_ValidateDevice returned %#x, expected "
        "D3DERR_CONFLICTINGRENDERSTATE.\n", hr);

    /* Try to draw with the device in an invalid state */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_BeginScene failed, hr %#x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);

        /* Don't check the resulting draw unless we find an app that needs it. On nvidia ValidateDevice
         * returns CONFLICTINGRENDERSTATE, so the result is undefined. On AMD d3d seems to assume the
         * stored Z buffer value is 0.0 for all pixels, even those that are covered by the depth buffer */
    }

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, old_rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, old_ds);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_ValidateDevice(device, &num_passes);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_ValidateDevice failed, hr %#x.\n", hr);

    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(ds);
    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(old_rt);
    IDirect3DSurface9_Release(old_ds);
}

static void unbound_sampler_test(void)
{
    IDirect3DPixelShader9 *ps, *ps_cube, *ps_volume;
    IDirect3DSurface9 *rt, *old_rt;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD color;
    HWND window;
    HRESULT hr;

    static const DWORD ps_code[] =
    {
        0xffff0200,                                     /* ps_2_0           */
        0x0200001f, 0x90000000, 0xa00f0800,             /* dcl_2d s0        */
        0x0200001f, 0x80000000, 0xb00f0000,             /* dcl t0           */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800, /* texld r0, t0, s0 */
        0x02000001, 0x800f0800, 0x80e40000,             /* mov oC0, r0      */
        0x0000ffff,                                     /* end              */
    };
    static const DWORD ps_code_cube[] =
    {
        0xffff0200,                                     /* ps_2_0           */
        0x0200001f, 0x98000000, 0xa00f0800,             /* dcl_cube s0      */
        0x0200001f, 0x80000000, 0xb00f0000,             /* dcl t0           */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800, /* texld r0, t0, s0 */
        0x02000001, 0x800f0800, 0x80e40000,             /* mov oC0, r0      */
        0x0000ffff,                                     /* end              */
    };
    static const DWORD ps_code_volume[] =
    {
        0xffff0200,                                     /* ps_2_0           */
        0x0200001f, 0xa0000000, 0xa00f0800,             /* dcl_volume s0    */
        0x0200001f, 0x80000000, 0xb00f0000,             /* dcl t0           */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800, /* texld r0, t0, s0 */
        0x02000001, 0x800f0800, 0x80e40000,             /* mov oC0, r0      */
        0x0000ffff,                                     /* end              */
    };

    static const struct
    {
        float x, y, z;
        float u, v;
    }
    quad[] =
    {
        {-1.0f,  -1.0f,  0.1f,   0.0f,   0.0f},
        {-1.0f,   1.0f,  0.1f,   1.0f,   0.0f},
        { 1.0f,  -1.0f,  0.1f,   0.0f,   1.0f},
        { 1.0f,   1.0f,  0.1f,   1.0f,   1.0f}
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        skip("No ps_2_0 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP) || !(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No cube / volume texture support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStage failed, %#x.\n", hr);

    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreatePixelShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code_cube, &ps_cube);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreatePixelShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code_volume, &ps_volume);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &rt, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &old_rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_GetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 );
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x56ffffff, 1.0f, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_BeginScene failed, hr %#x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);
    }

    color = getPixelColorFromSurface(rt, 32, 32);
    ok(color == 0xff000000, "Unbound sampler color is %#x.\n", color);

    /* Now try with a cube texture */
    hr = IDirect3DDevice9_SetPixelShader(device, ps_cube);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_BeginScene failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);
    }

    color = getPixelColorFromSurface(rt, 32, 32);
    ok(color == 0xff000000, "Unbound sampler color is %#x.\n", color);

    /* And then with a volume texture */
    hr = IDirect3DDevice9_SetPixelShader(device, ps_volume);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetPixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_BeginScene failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitiveUP failed, hr %#x.\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_EndScene failed, hr %#x.\n", hr);
    }

    color = getPixelColorFromSurface(rt, 32, 32);
    ok(color == 0xff000000, "Unbound sampler color is %#x.\n", color);

    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(old_rt);
    IDirect3DPixelShader9_Release(ps);
    IDirect3DPixelShader9_Release(ps_cube);
    IDirect3DPixelShader9_Release(ps_volume);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void update_surface_test(void)
{
    static const BYTE blocks[][8] =
    {
        {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00}, /* White */
        {0x00, 0xf8, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00}, /* Red */
        {0xe0, 0xff, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00}, /* Yellow */
        {0xe0, 0x07, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00}, /* Green */
        {0xff, 0x07, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00}, /* Cyan */
        {0x1f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00}, /* Blue */
        {0x1f, 0xf8, 0x1f, 0xf8, 0x00, 0x00, 0x00, 0x00}, /* Magenta */
    };
    static const struct
    {
        UINT x, y;
        D3DCOLOR color;
    }
    expected_colors[] =
    {
        { 18, 240, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0xff)},
        { 57, 240, D3DCOLOR_ARGB(0x00, 0x00, 0x00, 0xff)},
        {109, 240, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0xff)},
        {184, 240, D3DCOLOR_ARGB(0x00, 0x00, 0xff, 0x00)},
        {290, 240, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0x00)},
        {440, 240, D3DCOLOR_ARGB(0x00, 0xff, 0x00, 0x00)},
        {584, 240, D3DCOLOR_ARGB(0x00, 0xff, 0xff, 0xff)},
    };
    static const struct
    {
        float x, y, z, w;
        float u, v;
    }
    tri[] =
    {
        {  0.0f, 480.0f, 0.0f,  1.0f,   0.0f, 0.0f},
        {  0.0f,   0.0f, 0.0f,  1.0f,   0.0f, 1.0f},
        {640.0f, 240.0f, 0.0f, 10.0f, 100.0f, 0.5f},
    };
    static const RECT rect_2x2 = {0, 0, 2, 2};
    static const struct
    {
        UINT src_level;
        UINT dst_level;
        const RECT *r;
        HRESULT hr;
    }
    block_size_tests[] =
    {
        {1, 0, NULL,      D3D_OK},
        {0, 1, NULL,      D3DERR_INVALIDCALL},
        {5, 4, NULL,      D3DERR_INVALIDCALL},
        {4, 5, NULL,      D3DERR_INVALIDCALL},
        {4, 5, &rect_2x2, D3DERR_INVALIDCALL},
        {5, 5, &rect_2x2, D3D_OK},
    };

    IDirect3DSurface9 *src_surface, *dst_surface;
    IDirect3DTexture9 *src_tex, *dst_tex;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    UINT count, i;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1)))
    {
        skip("DXT1 not supported, skipping tests.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, 0, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &src_tex, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, 0, D3DFMT_DXT1, D3DPOOL_DEFAULT, &dst_tex, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    count = IDirect3DTexture9_GetLevelCount(src_tex);
    ok(count == 7, "Got level count %u, expected 7.\n", count);

    for (i = 0; i < count; ++i)
    {
        UINT row_count, block_count, x, y;
        D3DSURFACE_DESC desc;
        BYTE *row, *block;
        D3DLOCKED_RECT r;

        hr = IDirect3DTexture9_GetLevelDesc(src_tex, i, &desc);
        ok(SUCCEEDED(hr), "Failed to get level desc, hr %#x.\n", hr);

        hr = IDirect3DTexture9_LockRect(src_tex, i, &r, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);

        row_count = ((desc.Height + 3) & ~3) / 4;
        block_count = ((desc.Width + 3) & ~3) / 4;
        row = r.pBits;

        for (y = 0; y < row_count; ++y)
        {
            block = row;
            for (x = 0; x < block_count; ++x)
            {
                memcpy(block, blocks[i], sizeof(blocks[i]));
                block += sizeof(blocks[i]);
            }
            row += r.Pitch;
        }

        hr = IDirect3DTexture9_UnlockRect(src_tex, i);
        ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    }

    for (i = 0; i < sizeof(block_size_tests) / sizeof(*block_size_tests); ++i)
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(src_tex, block_size_tests[i].src_level, &src_surface);
        ok(SUCCEEDED(hr), "Failed to get texture surface, hr %#x.\n", hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(dst_tex, block_size_tests[i].dst_level, &dst_surface);
        ok(SUCCEEDED(hr), "Failed to get texture surface, hr %#x.\n", hr);

        hr = IDirect3DDevice9_UpdateSurface(device, src_surface, block_size_tests[i].r, dst_surface, NULL);
        ok(hr == block_size_tests[i].hr, "Update surface returned %#x for test %u, expected %#x.\n",
                hr, i, block_size_tests[i].hr);

        IDirect3DSurface9_Release(dst_surface);
        IDirect3DSurface9_Release(src_surface);
    }

    for (i = 0; i < count; ++i)
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(src_tex, i, &src_surface);
        ok(SUCCEEDED(hr), "Failed to get texture surface, hr %#x.\n", hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(dst_tex, i, &dst_surface);
        ok(SUCCEEDED(hr), "Failed to get texture surface, hr %#x.\n", hr);

        hr = IDirect3DDevice9_UpdateSurface(device, src_surface, NULL, dst_surface, NULL);
        ok(SUCCEEDED(hr), "Failed to update surface at level %u, hr %#x.\n", i, hr);

        IDirect3DSurface9_Release(dst_surface);
        IDirect3DSurface9_Release(src_surface);
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)dst_tex);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "SetTextureStageState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 1, tri, sizeof(*tri));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 0),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    IDirect3DTexture9_Release(dst_tex);
    IDirect3DTexture9_Release(src_tex);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void multisample_get_rtdata_test(void)
{
    IDirect3DSurface9 *original_ds, *original_rt, *rt, *readback;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping tests.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &readback, NULL);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &original_ds);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTargetData(device, rt, readback);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, original_ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to restore original render target, hr %#x.\n", hr);

    IDirect3DSurface9_Release(original_ds);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(rt);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void multisampled_depth_buffer_test(void)
{
    IDirect3DDevice9 *device = 0;
    IDirect3DSurface9 *original_rt, *rt, *readback, *ds, *original_ds;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HRESULT hr;
    D3DPRESENT_PARAMETERS present_parameters;
    unsigned int i;
    static const struct
    {
        float x, y, z;
        D3DCOLOR color;
    }
    quad_1[] =
    {
        { -1.0f,  1.0f, 0.0f, 0xffff0000},
        {  1.0f,  1.0f, 1.0f, 0xffff0000},
        { -1.0f, -1.0f, 0.0f, 0xffff0000},
        {  1.0f, -1.0f, 1.0f, 0xffff0000},
    },
    quad_2[] =
    {
        { -1.0f,  1.0f, 1.0f, 0xff0000ff},
        {  1.0f,  1.0f, 0.0f, 0xff0000ff},
        { -1.0f, -1.0f, 1.0f, 0xff0000ff},
        {  1.0f, -1.0f, 0.0f, 0xff0000ff},
    };
    static const struct
    {
        UINT x, y;
        D3DCOLOR color;
    }
    expected_colors[] =
    {
        { 80, 100, D3DCOLOR_ARGB(0xff, 0xff, 0x00, 0x00)},
        {240, 100, D3DCOLOR_ARGB(0xff, 0xff, 0x00, 0x00)},
        {400, 100, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff)},
        {560, 100, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff)},
        { 80, 450, D3DCOLOR_ARGB(0xff, 0xff, 0x00, 0x00)},
        {240, 450, D3DCOLOR_ARGB(0xff, 0xff, 0x00, 0x00)},
        {400, 450, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff)},
        {560, 450, D3DCOLOR_ARGB(0xff, 0x00, 0x00, 0xff)},
    };

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (FAILED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping multisampled depth buffer test.\n");
        IDirect3D9_Release(d3d);
        return;
    }
    if (FAILED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_D24S8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL)))
    {
        skip("Multisampling not supported for D3DFMT_D24S8, skipping multisampled depth buffer test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &present_parameters, &device);
    ok(hr == D3D_OK, "Failed to create a device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No unconditional NP2 texture support, skipping multisampled depth buffer test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &readback, NULL);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    /* Render onscreen and then offscreen */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_1, sizeof(*quad_1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, original_rt, NULL, rt, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_2, sizeof(*quad_2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, rt, NULL, readback, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColorFromSurface(readback, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_StretchRect(device, rt, NULL, original_rt, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Render offscreen and then onscreen */
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_2_SAMPLES, 0, TRUE, &ds, NULL);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_1, sizeof(*quad_1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, rt, NULL, original_rt, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_2, sizeof(*quad_2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, original_rt, NULL, readback, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColorFromSurface(readback, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DSurface9_Release(ds);
    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(original_rt);
    cleanup_device(device);

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &present_parameters, &device);
    ok(hr == D3D_OK, "Failed to create a device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ffff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear depth buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &readback, NULL);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &ds, NULL);
    ok(SUCCEEDED(hr), "CreateDepthStencilSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &original_ds);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);

    /* Render to a multisampled offscreen frame buffer and then blit to
     * the onscreen (not multisampled) frame buffer. */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_1, sizeof(*quad_1));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, rt, NULL, original_rt, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_StretchRect(device, ds, NULL, original_ds, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, original_ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad_2, sizeof(*quad_2));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, original_rt, NULL, readback, NULL, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "StretchRect failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColorFromSurface(readback, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to restore original render target, hr %#x.\n", hr);

    IDirect3DSurface9_Release(original_ds);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(ds);
    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(rt);
cleanup:
    cleanup_device(device);
    IDirect3D9_Release(d3d);
}

static void resz_test(void)
{
    IDirect3DDevice9 *device = 0;
    IDirect3DSurface9 *rt, *original_rt, *ds, *readback, *intz_ds;
    D3DCAPS9 caps;
    HRESULT hr;
    D3DPRESENT_PARAMETERS present_parameters;
    unsigned int i;
    static const DWORD ps_code[] =
    {
        0xffff0200,                                                             /* ps_2_0                       */
        0x0200001f, 0x90000000, 0xa00f0800,                                     /* dcl_2d s0                    */
        0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                       */
        0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0.0, 0.0, 0.0, 1.0   */
        0x02000001, 0x800f0001, 0xa0e40000,                                     /* mov r1, c0                   */
        0x03000042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texld r0, t0, s0             */
        0x02000001, 0x80010001, 0x80e40000,                                     /* mov r1.x, r0                 */
        0x03010042, 0x800f0000, 0xb0e40000, 0xa0e40800,                         /* texldp r0, t0, s0            */
        0x02000001, 0x80020001, 0x80000000,                                     /* mov r1.y, r0.x               */
        0x02000001, 0x800f0800, 0x80e40001,                                     /* mov oC0, r1                  */
        0x0000ffff,                                                             /* end                          */
    };
    struct
    {
        float x, y, z;
        float s, t, p, q;
    }
    quad[] =
    {
        { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f},
        {  1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.5f},
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f},
        {  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f},
    };
    struct
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
    IDirect3DTexture9 *texture;
    IDirect3DPixelShader9 *ps;
    IDirect3D9 *d3d;
    DWORD value;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (FAILED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL)))
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8, skipping RESZ test.\n");
        IDirect3D9_Release(d3d);
        return;
    }
    if (FAILED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_D24S8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL)))
    {
        skip("Multisampling not supported for D3DFMT_D24S8, skipping RESZ test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, MAKEFOURCC('I','N','T','Z'))))
    {
        skip("No INTZ support, skipping RESZ test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, MAKEFOURCC('R','E','S','Z'))))
    {
        skip("No RESZ support, skipping RESZ test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    ok(hr == D3D_OK, "Failed to create a device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        skip("No pixel shader 2.0 support, skipping INTZ test.\n");
        cleanup_device(device);
        IDirect3D9_Release(d3d);
        return;
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
    {
        skip("No unconditional NP2 texture support, skipping INTZ test.\n");
        cleanup_device(device);
        IDirect3D9_Release(d3d);
        return;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_2_SAMPLES, 0, TRUE, &ds, NULL);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &readback, NULL);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &intz_ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, intz_ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear depth/stencil, hr %#x.\n", hr);
    IDirect3DSurface9_Release(intz_ds);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4(0));
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);

    /* Render offscreen (multisampled), blit the depth buffer
     * into the INTZ texture and then check its contents */
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    /* The destination depth texture has to be bound to sampler 0 */
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    /* the ATI "spec" says you have to do a dummy draw to ensure correct commands ordering */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0xf);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    /* The actual multisampled depth buffer resolve happens here */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_POINTSIZE, &value);
    ok(SUCCEEDED(hr) && value == 0x7fa05000, "GetRenderState failed, hr %#x, value %#x.\n", hr, value);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back */
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);
    IDirect3DPixelShader9_Release(ps);
    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(original_rt);
    IDirect3DSurface9_Release(rt);
    cleanup_device(device);


    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    ok(hr == D3D_OK, "Failed to create a device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &original_rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 640, 480, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &readback, NULL);
    ok(SUCCEEDED(hr), "Failed to create readback surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 640, 480, 1,
            D3DUSAGE_DEPTHSTENCIL, MAKEFOURCC('I','N','T','Z'), D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &intz_ds);
    ok(SUCCEEDED(hr), "GetSurfaceLevel failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, readback);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, intz_ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear depth/stencil, hr %#x.\n", hr);
    IDirect3DSurface9_Release(intz_ds);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
    ok(SUCCEEDED(hr), "CreatePixelShader failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4(0));
    ok(SUCCEEDED(hr), "SetFVF failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState failed, hr %#x.\n", hr);

    /* Render onscreen, blit the depth buffer into the INTZ texture
     * and then check its contents */
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0xf);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    /* The actual multisampled depth buffer resolve happens here */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_POINTSIZE, &value);
    ok(SUCCEEDED(hr) && value == 0x7fa05000, "GetRenderState failed, hr %#x, value %#x.\n", hr, value);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, readback);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);


    /* Test edge cases - try with no texture at all */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);

    /* With a non-multisampled depth buffer */
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &ds, NULL);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, readback);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, 0xf);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetPixelShader(device, ps);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);

    /* Read the depth values back. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    for (i = 0; i < sizeof(expected_colors) / sizeof(*expected_colors); ++i)
    {
        D3DCOLOR color = getPixelColor(device, expected_colors[i].x, expected_colors[i].y);
        ok(color_match(color, expected_colors[i].color, 1),
                "Expected color 0x%08x at (%u, %u), got 0x%08x.\n",
                expected_colors[i].color, expected_colors[i].x, expected_colors[i].y, color);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Without a current depth-stencil buffer set */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_POINTSIZE, 0x7fa05000);
    ok(SUCCEEDED(hr), "SetRenderState (multisampled depth buffer resolve) failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);
    IDirect3DSurface9_Release(ds);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, original_rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);
    IDirect3DPixelShader9_Release(ps);
    IDirect3DSurface9_Release(readback);
    IDirect3DSurface9_Release(original_rt);
    cleanup_device(device);
    IDirect3D9_Release(d3d);
}

static void zenable_test(void)
{
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
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    UINT x, y;
    UINT i, j;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, tquad, sizeof(*tquad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = getPixelColor(device, x, y);
            ok(color_match(color, 0x0000ff00, 1),
                    "Expected color 0x0000ff00 at %u, %u, got 0x%08x.\n", x, y, color);
        }
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1)
            && caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        static const DWORD vs_code[] =
        {
            0xfffe0101,                                 /* vs_1_1           */
            0x0000001f, 0x80000000, 0x900f0000,         /* dcl_position v0  */
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

        IDirect3DVertexShader9 *vs;
        IDirect3DPixelShader9 *ps;

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
        ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_code, &vs);
        ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &ps);
        ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetVertexShader(device, vs);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, ps);
        ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        for (i = 0; i < 4; ++i)
        {
            for (j = 0; j < 4; ++j)
            {
                x = 80 * ((2 * j) + 1);
                y = 60 * ((2 * i) + 1);
                color = getPixelColor(device, x, y);
                ok(color_match(color, expected[i * 4 + j], 1),
                        "Expected color 0x%08x at %u, %u, got 0x%08x.\n", expected[i * 4 + j], x, y, color);
            }
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetVertexShader(device, NULL);
        ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        IDirect3DPixelShader9_Release(ps);
        IDirect3DVertexShader9_Release(vs);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void fog_special_test(void)
{
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
        0xffff0101,                                 /* ps_1_1           */
        0x00000001, 0x800f0000, 0x90e40000,         /* mov r0, v0       */
        0x0000ffff
    };
    static const DWORD vertex_shader_code[] =
    {
        0xfffe0101,                                 /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,         /* dcl_position v0  */
        0x0000001f, 0x8000000a, 0x900f0001,         /* dcl_color0 v1    */
        0x00000001, 0xc00f0000, 0x90e40000,         /* mov oPos, v0     */
        0x00000001, 0xd00f0000, 0x90e40001,         /* mov oD0, v1      */
        0x0000ffff
    };
    static const D3DMATRIX identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};
    union
    {
        float f;
        DWORD d;
    } conv;
    DWORD color;
    HRESULT hr;
    unsigned int i;
    IDirect3DPixelShader9 *ps;
    IDirect3DVertexShader9 *vs;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code, &vs);
        ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#x.\n", hr);
    }
    else
    {
        skip("Vertex Shaders not supported, skipping some fog tests.\n");
        vs = NULL;
    }
    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &ps);
        ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    }
    else
    {
        skip("Pixel Shaders not supported, skipping some fog tests.\n");
        ps = NULL;
    }

    /* The table fog tests seem to depend on the projection matrix explicitly
     * being set to an identity matrix, even though that's the default.
     * (AMD Radeon HD 6310, Windows 7) */
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable fog, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xffff0000);
    ok(SUCCEEDED(hr), "Failed to set fog color, hr %#x.\n", hr);

    conv.f = 0.5f;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, conv.d);
    ok(SUCCEEDED(hr), "Failed to set fog start, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, conv.d);
    ok(SUCCEEDED(hr), "Failed to set fog end, hr %#x.\n", hr);

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

        if (!tests[i].vs)
        {
            hr = IDirect3DDevice9_SetVertexShader(device, NULL);
            ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        }
        else if (vs)
        {
            hr = IDirect3DDevice9_SetVertexShader(device, vs);
            ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
        }
        else
        {
            continue;
        }

        if (!tests[i].ps)
        {
            hr = IDirect3DDevice9_SetPixelShader(device, NULL);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
        }
        else if (ps)
        {
            hr = IDirect3DDevice9_SetPixelShader(device, ps);
            ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
        }
        else
        {
            continue;
        }

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, tests[i].vertexmode);
        ok(SUCCEEDED(hr), "Failed to set fogvertexmode, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, tests[i].tablemode);
        ok(SUCCEEDED(hr), "Failed to set fogtablemode, hr %#x.\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = getPixelColor(device, 310, 240);
        ok(color_match(color, tests[i].color_left, 1),
                "Expected left color 0x%08x, got 0x%08x, case %u.\n", tests[i].color_left, color, i);
        color = getPixelColor(device, 330, 240);
        ok(color_match(color, tests[i].color_right, 1),
                "Expected right color 0x%08x, got 0x%08x, case %u.\n", tests[i].color_right, color, i);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);
    }

    if (vs)
        IDirect3DVertexShader9_Release(vs);
    if (ps)
        IDirect3DPixelShader9_Release(ps);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void volume_srgb_test(void)
{
    HRESULT hr;
    unsigned int i, j;
    IDirect3DVolumeTexture9 *tex1, *tex2;
    D3DPOOL pool;
    D3DLOCKED_BOX locked_box;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;

    static const struct
    {
        BOOL srgb;
        DWORD color;
    }
    tests[] =
    {
        /* Try toggling on and off */
        { FALSE,    0x007f7f7f },
        { TRUE,     0x00363636 },
        { FALSE,    0x007f7f7f },
    };
    static const struct
    {
        struct vec3 pos;
        struct vec3 texcrd;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_QUERY_SRGBREAD, D3DRTYPE_VOLUMETEXTURE, D3DFMT_A8R8G8B8) != D3D_OK)
    {
        skip("D3DFMT_A8R8G8B8 volume textures with SRGBREAD not supported.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op 0, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op 0, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);

    for (i = 0; i < 2; i++)
    {
        if (!i)
            pool = D3DPOOL_SYSTEMMEM;
        else
            pool = D3DPOOL_MANAGED;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, 0, D3DFMT_A8R8G8B8, pool, &tex1, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DVolumeTexture9_LockBox(tex1, 0, &locked_box, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        *((DWORD *)locked_box.pBits) = 0x7f7f7f7f;
        hr = IDirect3DVolumeTexture9_UnlockBox(tex1, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);

        if (!i)
        {
            hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, 0,
                    D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex2, NULL);
            ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
            hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex1, (IDirect3DBaseTexture9 *)tex2);
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
            IDirect3DVolumeTexture9_Release(tex1);

            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex2);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
            IDirect3DVolumeTexture9_Release(tex2);
        }
        else
        {
            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex1);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
            IDirect3DVolumeTexture9_Release(tex1);
        }

        for (j = 0; j < sizeof(tests) / sizeof(*tests); j++)
        {
            hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_SRGBTEXTURE, tests[j].srgb);
            ok(SUCCEEDED(hr), "Failed to set srgb state, hr %#x.\n", hr);

            hr = IDirect3DDevice9_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            color = getPixelColor(device, 320, 240);
            ok(color_match(color, tests[j].color, 2),
                    "Expected color 0x%08x, got 0x%08x, i = %u, j = %u.\n", tests[j].color, color, i, j);

            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void volume_dxt5_test(void)
{
    IDirect3DVolumeTexture9 *texture;
    IDirect3DDevice9 *device;
    D3DLOCKED_BOX box;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    DWORD color;
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

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_VOLUMETEXTURE, D3DFMT_DXT5)))
    {
        skip("DXT5 volume textures are not supported, skipping test.\n");
        goto done;
    }
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 8, 4, 2, 1, 0, D3DFMT_DXT5,
            D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    memcpy(box.pBits, texture_data, sizeof(texture_data));
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "Failed to set mag filter, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff00ff, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[4], sizeof(*quads));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    for (i = 0; i < 4; i++)
    {
        color = getPixelColor(device, 80 + 160 * i, 240);
        ok (color_match(color, expected_colors[i], 1),
                "Expected color 0x%08x, got 0x%08x, case %u.\n", expected_colors[i], color, i);
    }

    IDirect3DVolumeTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void volume_v16u16_test(void)
{
    IDirect3DVolumeTexture9 *texture;
    IDirect3DPixelShader9 *shader;
    IDirect3DDevice9 *device;
    D3DLOCKED_BOX box;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    D3DCAPS9 caps;
    SHORT *texel;
    DWORD color;
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

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.PixelShaderVersion < D3DPS_VERSION(1, 1))
    {
        skip("No ps_1_1 support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }
    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_VOLUMETEXTURE, D3DFMT_V16U16)))
    {
        skip("Volume V16U16 textures are not supported, skipping test.\n");
        IDirect3DDevice9_Release(device);
        goto done;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "Failed to create pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "Failed to set filter, hr %#x.\n", hr);

    for (i = 0; i < 2; i++)
    {
        D3DPOOL pool;

        if (i)
            pool = D3DPOOL_SYSTEMMEM;
        else
            pool = D3DPOOL_MANAGED;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 2, 2, 1, 0, D3DFMT_V16U16,
                pool, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &box, NULL, 0);
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

        hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        if (i)
        {
            IDirect3DVolumeTexture9 *texture2;

            hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 2, 2, 1, 0, D3DFMT_V16U16,
                    D3DPOOL_DEFAULT, &texture2, NULL);
            ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

            hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)texture,
                    (IDirect3DBaseTexture9 *)texture2);
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

            IDirect3DVolumeTexture9_Release(texture);
            texture = texture2;
        }

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
        ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00ff00ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[0], sizeof(*quads));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[4], sizeof(*quads));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
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

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

        IDirect3DVolumeTexture9_Release(texture);
    }

    IDirect3DPixelShader9_Release(shader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void add_dirty_rect_test_draw(IDirect3DDevice9 *device)
{
    HRESULT hr;
    static const struct
    {
        struct vec3 position;
        struct vec2 texcoord;
    }
    quad[] =
    {
        {{-1.0, -1.0, 0.0}, {0.0, 0.0}},
        {{-1.0,  1.0, 0.0}, {0.0, 1.0}},
        {{ 1.0, -1.0, 0.0}, {1.0, 0.0}},
        {{ 1.0,  1.0, 0.0}, {1.0, 1.0}},
    };

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
}

static void add_dirty_rect_test(void)
{
    HRESULT hr;
    IDirect3DTexture9 *tex_dst1, *tex_dst2, *tex_src_red, *tex_src_green, *tex_managed;
    IDirect3DSurface9 *surface_dst2, *surface_src_green, *surface_src_red, *surface_managed;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    DWORD *texel;
    HWND window;
    D3DLOCKED_RECT locked_rect;
    static const RECT part_rect = {96, 96, 160, 160};
    DWORD color;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &tex_dst1, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &tex_dst2, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &tex_src_red, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &tex_src_green, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_MANAGED, &tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    hr = IDirect3DTexture9_GetSurfaceLevel(tex_dst2, 0, &surface_dst2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(tex_src_green, 0, &surface_src_green);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(tex_src_red, 0, &surface_src_red);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(tex_managed, 0, &surface_managed);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    fill_surface(surface_src_red, 0x00ff0000, 0);
    fill_surface(surface_src_green, 0x0000ff00, 0);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);

    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    /* The second UpdateTexture call writing to tex_dst2 is ignored because tex_src_green is not dirty. */
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_red,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on the destination is ignored. */
    hr = IDirect3DTexture9_AddDirtyRect(tex_dst2, &part_rect);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture9_AddDirtyRect(tex_dst2, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on the source makes UpdateTexture work. Partial rectangle
     * tracking is supported. */
    hr = IDirect3DTexture9_AddDirtyRect(tex_src_green, &part_rect);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    color = getPixelColor(device, 1, 1);
    todo_wine ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture9_AddDirtyRect(tex_src_green, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 1, 1);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);

    /* Locks with NO_DIRTY_UPDATE are ignored. */
    fill_surface(surface_src_green, 0x00000080, D3DLOCK_NO_DIRTY_UPDATE);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Readonly maps write to D3DPOOL_SYSTEMMEM, but don't record a dirty rectangle. */
    fill_surface(surface_src_green, 0x000000ff, D3DLOCK_READONLY);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    todo_wine ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    hr = IDirect3DTexture9_AddDirtyRect(tex_src_green, NULL);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x000000ff, 1),
            "Expected color 0x000000ff, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Maps without either of these flags record a dirty rectangle. */
    fill_surface(surface_src_green, 0x00ffffff, 0);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ffffff, 1),
            "Expected color 0x00ffffff, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Partial LockRect works just like a partial AddDirtyRect call. */
    hr = IDirect3DTexture9_LockRect(tex_src_green, 0, &locked_rect, &part_rect, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    texel = locked_rect.pBits;
    for (i = 0; i < 64; i++)
        texel[i] = 0x00ff00ff;
    for (i = 1; i < 64; i++)
        memcpy((BYTE *)locked_rect.pBits + i * locked_rect.Pitch, locked_rect.pBits, locked_rect.Pitch);
    hr = IDirect3DTexture9_UnlockRect(tex_src_green, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff00ff, 1),
            "Expected color 0x00ff00ff, got 0x%08x.\n", color);
    color = getPixelColor(device, 1, 1);
    ok(color_match(color, 0x00ffffff, 1),
            "Expected color 0x00ffffff, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    fill_surface(surface_src_red, 0x00ff0000, 0);
    fill_surface(surface_src_green, 0x0000ff00, 0);

    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_green,
            (IDirect3DBaseTexture9 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex_dst1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* UpdateSurface ignores the missing dirty marker. */
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)tex_src_red,
            (IDirect3DBaseTexture9 *)tex_dst2);
    hr = IDirect3DDevice9_UpdateSurface(device, surface_src_green, NULL, surface_dst2, NULL);
    ok(SUCCEEDED(hr), "Failed to update surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex_dst2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    fill_surface(surface_managed, 0x00ff0000, 0);
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)tex_managed);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* Managed textures also honor D3DLOCK_NO_DIRTY_UPDATE. */
    fill_surface(surface_managed, 0x0000ff00, D3DLOCK_NO_DIRTY_UPDATE);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00ff0000, 1),
            "Expected color 0x00ff0000, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect uploads the new contents.
     * Side note, not tested in the test: Partial surface updates work, and two separate
     * dirty rectangles are tracked individually. Tested on Nvidia Kepler, other drivers
     * untested. */
    hr = IDirect3DTexture9_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x0000ff00, 1),
            "Expected color 0x0000ff00, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* So does EvictManagedResources. */
    fill_surface(surface_managed, 0x000000ff, D3DLOCK_NO_DIRTY_UPDATE);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EvictManagedResources(device);
    ok(SUCCEEDED(hr), "Failed to evict managed resources, hr %#x.\n", hr);
    add_dirty_rect_test_draw(device);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x000000ff, 1),
            "Expected color 0x000000ff, got 0x%08x.\n", color);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    /* AddDirtyRect on a locked texture is allowed. */
    hr = IDirect3DTexture9_LockRect(tex_src_red, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_AddDirtyRect(tex_src_red, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DTexture9_UnlockRect(tex_src_red, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

    /* Redundant AddDirtyRect calls are ok. */
    hr = IDirect3DTexture9_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);
    hr = IDirect3DTexture9_AddDirtyRect(tex_managed, NULL);
    ok(SUCCEEDED(hr), "Failed to add dirty rect, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface_dst2);
    IDirect3DSurface9_Release(surface_managed);
    IDirect3DSurface9_Release(surface_src_red);
    IDirect3DSurface9_Release(surface_src_green);
    IDirect3DTexture9_Release(tex_src_red);
    IDirect3DTexture9_Release(tex_src_green);
    IDirect3DTexture9_Release(tex_dst1);
    IDirect3DTexture9_Release(tex_dst2);
    IDirect3DTexture9_Release(tex_managed);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(visual)
{
    IDirect3DDevice9 *device_ptr;
    D3DCAPS9 caps;
    HRESULT hr;
    DWORD color;

    if (!(device_ptr = init_d3d9()))
    {
        skip("Creating the device failed\n");
        return;
    }

    IDirect3DDevice9_GetDeviceCaps(device_ptr, &caps);

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(device_ptr, 1, 1);
    if(color !=0x00ff0000)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(device_ptr, 639, 479);
    if(color != 0x0000ddee)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    /* Now execute the real tests */
    depth_clamp_test(device_ptr);
    stretchrect_test(device_ptr);
    lighting_test(device_ptr);
    clear_test(device_ptr);
    color_fill_test(device_ptr);
    fog_test(device_ptr);
    if(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        test_cube_wrap(device_ptr);
    } else {
        skip("No cube texture support\n");
    }
    z_range_test(device_ptr);
    if(caps.TextureCaps & D3DPTEXTURECAPS_MIPMAP)
    {
        maxmip_test(device_ptr);
    }
    else
    {
        skip("No mipmap support\n");
    }

    offscreen_test(device_ptr);
    ds_size_test(device_ptr);
    alpha_test(device_ptr);
    shademode_test(device_ptr);
    srgbtexture_test(device_ptr);
    release_buffer_test(device_ptr);
    float_texture_test(device_ptr);
    g16r16_texture_test(device_ptr);
    pixelshader_blending_test(device_ptr);
    texture_transform_flags_test(device_ptr);
    autogen_mipmap_test(device_ptr);
    fixed_function_decl_test(device_ptr);
    conditional_np2_repeat_test(device_ptr);
    fixed_function_bumpmap_test(device_ptr);
    pointsize_test(device_ptr);
    tssargtemp_test(device_ptr);
    np2_stretch_rect_test(device_ptr);
    yuv_color_test(device_ptr);
    yuv_layout_test(device_ptr);
    zwriteenable_test(device_ptr);
    alphatest_test(device_ptr);
    viewport_test(device_ptr);

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        test_constant_clamp_vs(device_ptr);
        test_compare_instructions(device_ptr);
    }
    else skip("No vs_1_1 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0))
    {
        test_mova(device_ptr);
        loop_index_test(device_ptr);
        sincos_test(device_ptr);
        sgn_test(device_ptr);
        clip_planes_test(device_ptr);
        if (caps.VertexShaderVersion >= D3DVS_VERSION(3, 0)) {
            test_vshader_input(device_ptr);
            test_vshader_float16(device_ptr);
            stream_test(device_ptr);
        } else {
            skip("No vs_3_0 support\n");
        }
    }
    else skip("No vs_2_0 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0) && caps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
    {
        fog_with_shader_test(device_ptr);
    }
    else skip("No vs_2_0 and ps_2_0 support\n");

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        texbem_test(device_ptr);
        texdepth_test(device_ptr);
        texkill_test(device_ptr);
        x8l8v8u8_test(device_ptr);
    }
    else skip("No ps_1_1 support\n");

    cleanup_device(device_ptr);
    device_ptr = NULL;

    volume_v16u16_test();
    constant_clamp_ps_test();
    cnd_test();
    dp2add_ps_test();
    unbound_sampler_test();
    nested_loop_test();
    pretransformed_varying_test();
    vface_register_test();
    vpos_register_test();
    multiple_rendertargets_test();
    texop_test();
    texop_range_test();
    alphareplicate_test();
    dp3_alpha_test();
    depth_buffer_test();
    depth_buffer2_test();
    depth_blit_test();
    intz_test();
    shadow_test();
    fp_special_test();
    depth_bounds_test();
    srgbwrite_format_test();
    update_surface_test();
    multisample_get_rtdata_test();
    zenable_test();
    fog_special_test();
    volume_srgb_test();
    volume_dxt5_test();
    add_dirty_rect_test();
    multisampled_depth_buffer_test();
    resz_test();
    stencil_cull_test();

cleanup:
    cleanup_device(device_ptr);
}
