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
    if(FAILED(hr) || !tex )  /* This is not a test */
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

struct vertex
{
    float x, y, z;
    DWORD diffuse;
};

struct tvertex
{
    float x, y, z, w;
    DWORD diffuse;
};

struct nvertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
};

static void test_sanity(IDirect3DDevice8 *device)
{
    D3DCOLOR color;
    HRESULT hr;

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
}

static void lighting_test(IDirect3DDevice8 *device)
{
    HRESULT hr;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    DWORD color;

    static const struct vertex unlitquad[] =
    {
        {-1.0f, -1.0f, 0.1f, 0xffff0000},
        {-1.0f,  0.0f, 0.1f, 0xffff0000},
        { 0.0f,  0.0f, 0.1f, 0xffff0000},
        { 0.0f, -1.0f, 0.1f, 0xffff0000},
    };
    static const struct vertex litquad[] =
    {
        {-1.0f,  0.0f, 0.1f, 0xff00ff00},
        {-1.0f,  1.0f, 0.1f, 0xff00ff00},
        { 0.0f,  1.0f, 0.1f, 0xff00ff00},
        { 0.0f,  0.0f, 0.1f, 0xff00ff00},
    };
    static const struct nvertex unlitnquad[] =
    {
        { 0.0f, -1.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xff0000ff},
        { 0.0f,  0.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xff0000ff},
        { 1.0f,  0.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xff0000ff},
        { 1.0f, -1.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xff0000ff},
    };
    static const struct nvertex litnquad[] =
    {
        { 0.0f,  0.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xffffff00},
        { 0.0f,  1.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xffffff00},
        { 1.0f,  1.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xffffff00},
        { 1.0f,  0.0f, 0.1f, 1.0f, 1.0f, 1.0f, 0xffffff00},
    };
    static const WORD Indices[] = {0, 1, 2, 2, 3, 0};
    static const D3DMATRIX mat =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %#08x\n", hr);

    /* Setup some states that may cause issues */
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
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed with %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed with %#08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* No lights are defined... That means, lit vertices should be entirely black. */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, nfvf);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
            2 /* PrimCount */, Indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
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

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %#08x\n", hr);
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
    static struct vertex quad[] =
    {
        {-1.0f, -1.0f,  0.0f, 0xffff0000},
        {-1.0f,  1.0f,  0.0f, 0xffff0000},
        { 1.0f, -1.0f,  0.0f, 0xffff0000},
        { 1.0f,  1.0f,  0.0f, 0xffff0000},
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
        unsigned int color[11];
    }
    test_data[] =
    {
        /* Only pixel shader */
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

        /* Vertex shader */
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

        /* Vertex shader and pixel shader */
        /* The next 4 tests would read the fog coord output, but it isn't available.
         * The result is a fully fogged quad, no matter what the Z coord is. */
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
        /* Foggy vertex shader */
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

        /* Foggy vertex shader and pixel shader. First 4 tests with vertex fog,
         * all using the fixed fog-coord linear fog */
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

        /* These use table fog. Here the shader-provided fog coordinate is
         * ignored and the z coordinate used instead */
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
            quad[0].z = 0.001f + j / 10.02f;
            quad[1].z = 0.001f + j / 10.02f;
            quad[2].z = 0.001f + j / 10.02f;
            quad[3].z = 0.001f + j / 10.02f;

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
            ok(color_match(color, test_data[i].color[j], 13),
                    "fog vs%i ps%i fvm%i ftm%i %d: got color %08x, expected %08x +-5%%\n",
                    test_data[i].vshader, test_data[i].pshader,
                    test_data[i].vfog, test_data[i].tfog, j, color, test_data[i].color[j]);

            IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
        }
    }

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

    /* Retest with the coissue flag on the alpha instruction instead. This works "as expected". */
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
        {640.0f, 240.0f, -1.1f, 1.0f, 0xffffff00},
        {640.0f, 480.0f, -1.1f, 1.0f, 0xffffff00},
        {  0.0f, 240.0f,  1.1f, 1.0f, 0xffffff00},
        {  0.0f, 480.0f,  1.1f, 1.0f, 0xffffff00},
    };
    static const struct tvertex quad4[] =
    {
        {640.0f, 240.0f, -1.1f, 1.0f, 0xff00ff00},
        {640.0f, 480.0f, -1.1f, 1.0f, 0xff00ff00},
        {  0.0f, 240.0f,  1.1f, 1.0f, 0xff00ff00},
        {  0.0f, 480.0f,  1.1f, 1.0f, 0xff00ff00},
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

static void alpha_test(void)
{
    IDirect3DSurface8 *backbuffer, *offscreen, *depthstencil;
    IDirect3DTexture8 *offscreenTexture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct vertex quad1[] =
    {
        {-1.0f, -1.0f, 0.1f, 0x4000ff00},
        {-1.0f,  0.0f, 0.1f, 0x4000ff00},
        { 1.0f, -1.0f, 0.1f, 0x4000ff00},
        { 1.0f,  0.0f, 0.1f, 0x4000ff00},
    };
    static const struct vertex quad2[] =
    {
        {-1.0f,  0.0f, 0.1f, 0xc00000ff},
        {-1.0f,  1.0f, 0.1f, 0xc00000ff},
        { 1.0f,  0.0f, 0.1f, 0xc00000ff},
        { 1.0f,  1.0f, 0.1f, 0xc00000ff},
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
static void depth_clamp_test(IDirect3DDevice8 *device)
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
        { -1.0f,   0.5f, 10.0f,       0xfff91414},
        {  1.0f,   0.5f, 10.0f,       0xfff91414},
        { -1.0f,  0.25f, 10.0f,       0xfff91414},
        {  1.0f,  0.25f, 10.0f,       0xfff91414},
    };

    D3DVIEWPORT8 vp;
    D3DCOLOR color;
    D3DCAPS8 caps;
    HRESULT hr;

    vp.X = 0;
    vp.Y = 0;
    vp.Width = 640;
    vp.Height = 480;
    vp.MinZ = 0.0;
    vp.MaxZ = 7.5;

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

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

    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice8_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);
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

    static const struct vertex quad[] =
    {
        { -1.0,  1.0, 0.66f, 0xffff0000},
        {  1.0,  1.0, 0.66f, 0xffff0000},
        { -1.0, -1.0, 0.66f, 0xffff0000},
        {  1.0, -1.0, 0.66f, 0xffff0000},
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

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
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
            ok(color_match(color, 0x0000ff00, 1),
                    "Expected color 0x0000ff00 at %u, %u, got 0x%08x.\n", x, y, color);
        }
    }

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present backbuffer, hr %#x.\n", hr);

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
                ok(color_match(color, expected[i * 4 + j], 1),
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

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);
    for (i = 0; i < 4; i++)
    {
        color = getPixelColor(device, 80 + 160 * i, 240);
        ok (color_match(color, expected_colors[i], 1),
                "Expected color 0x%08x, got 0x%08x, case %u.\n", expected_colors[i], color, i);
    }

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
    ok(color_match(color, 0x00ff0000, 1),
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

START_TEST(visual)
{
    D3DADAPTER_IDENTIFIER8 identifier;
    IDirect3DDevice8 *device_ptr;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
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

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    if (!(device_ptr = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto cleanup;
    }

    test_sanity(device_ptr);
    depth_clamp_test(device_ptr);
    lighting_test(device_ptr);

    refcount = IDirect3DDevice8_Release(device_ptr);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);

    clear_test();
    fog_test();
    z_range_test();
    offscreen_test();
    alpha_test();
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
}
