/*
 * Copyright 2005, 2007 Henri Verbeet
 * Copyright (C) 2007 Stefan Dösinger(for CodeWeavers)
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
#include <dxerr9.h>
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    HWND ret;
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    ret = CreateWindow("d3d9_test_wc", "d3d9_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);
    return ret;
}

static DWORD getPixelColor(IDirect3DDevice9 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DSurface9 *surf;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 640, 480, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if(FAILED(hr) || !surf )  /* This is not a test */
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%s\n", DXGetErrorString9(hr));
        return 0xdeadbeef;
    }

    hr = IDirect3DDevice9_GetFrontBufferData(device, 0, surf);
    if(FAILED(hr))
    {
        trace("Can't read the front buffer data, hr=%s\n", DXGetErrorString9(hr));
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DSurface9_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%s\n", DXGetErrorString9(hr));
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface9_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%s\n", DXGetErrorString9(hr));
    }

out:
    if(surf) IDirect3DSurface9_Release(surf);
    return ret;
}

static IDirect3DDevice9 *init_d3d9(void)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion) = 0;
    IDirect3D9 *d3d9_ptr = 0;
    IDirect3DDevice9 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hr;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return NULL;

    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = FALSE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D_CreateDevice returned: %s\n", DXGetErrorString9(hr));

    return device_ptr;
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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, (D3DMATRIX *)mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetFVF(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3D_OK)
    {
        /* No lights are defined... That means, lit vertices should be entirely black */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetFVF(device, nfvf);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower left quad - unlit with normals */
    ok(color == 0x000000ff, "Unlit quad with normals has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper left quad - lit with normals */
    ok(color == 0x00000000, "Lit quad with normals has color %08x\n", color);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

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
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3DERR_INVALIDCALL) invalid_clear_failed = TRUE;

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.y1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice9_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3DERR_INVALIDCALL) invalid_clear_failed = TRUE;

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

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

    /* Test how the viewport affects clears */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_GetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %s\n", DXGetErrorString9(hr));

    vp.X = 160;
    vp.Y = 120;
    vp.Width = 160;
    vp.Height = 120;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    vp.X = 320;
    vp.Y = 240;
    vp.Width = 320;
    vp.Height = 240;
    vp.MinZ = 0.0;
    vp.MaxZ = 1.0;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %s\n", DXGetErrorString9(hr));
    rect[0].x1 = 160;
    rect[0].y1 = 120;
    rect[0].x2 = 480;
    rect[0].y2 = 360;
    hr = IDirect3DDevice9_Clear(device, 1, &rect[0], D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetViewport(device, &old_vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %s\n", DXGetErrorString9(hr));

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    scissor.left = 160;
    scissor.right = 480;
    scissor.top = 120;
    scissor.bottom = 360;
    hr = IDirect3DDevice9_SetScissorRect(device, &scissor);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Clear(device, 1, &rect[1], D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice_SetScissorRect failed with %s\n", DXGetErrorString9(hr));

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_COLORWRITEENABLE, &oldColorWrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, oldColorWrite);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    /* Colorwriteenable does not affect the clear */
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Color write protected clear returned color %08x\n", color);
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

            hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
            ok(SUCCEEDED(hr), "Present failed (%08x)\n", hr);

            color = getPixelColor(device, 320, 240);
            ok(color == test_data[j][i].out, "Expected color %08x, got %08x (for input %f, instruction %s)\n",
               test_data[j][i].out, color, test_data[j][i].in[0], j == 0 ? "mov" : "mova");

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
    DWORD color;
    float start = 0.0f, end = 1.0f;
    D3DCAPS9 caps;

    /* Gets full z based fog with linear fog, no fog with specular color */
    struct sVertex unstransformed_1[] = {
        {-1,    -1,   0.1f,         0xFFFF0000,     0xFF000000  },
        {-1,     0,   0.1f,         0xFFFF0000,     0xFF000000  },
        { 0,     0,   0.1f,         0xFFFF0000,     0xFF000000  },
        { 0,    -1,   0.1f,         0xFFFF0000,     0xFF000000  },
    };
    /* Ok, I am too lazy to deal with transform matrices */
    struct sVertex unstransformed_2[] = {
        {-1,     0,   1.0f,         0xFFFF0000,     0xFF000000  },
        {-1,     1,   1.0f,         0xFFFF0000,     0xFF000000  },
        { 0,     1,   1.0f,         0xFFFF0000,     0xFF000000  },
        { 0,     0,   1.0f,         0xFFFF0000,     0xFF000000  },
    };
    /* Untransformed ones. Give them a different diffuse color to make the test look
     * nicer. It also makes making sure that they are drawn correctly easier.
     */
    struct sVertexT transformed_1[] = {
        {320,    0,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {640,    0,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {640,  240,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {320,  240,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
    };
    struct sVertexT transformed_2[] = {
        {320,  240,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {640,  240,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {640,  480,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
        {320,  480,   1.0f, 1.0f,   0xFFFFFF00,     0xFF000000  },
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString9(hr));

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK)
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_1,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        /* That makes it use the Z value */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_2,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        /* transformed verts */
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                                                     sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %s\n", DXGetErrorString9(hr));
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_2,
                                                     sizeof(transformed_2[0]));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %s\n", DXGetErrorString9(hr));
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000FF00, "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    if(caps.RasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        color = getPixelColor(device, 480, 360);
        ok(color == 0x0000FF00, "Transformed vertex with linear table fog has color %08x\n", color);
    }
    else
    {
        /* Without fog table support the vertex fog is still applied, even though table fog is turned on.
         * The settings above result in no fogging with vertex fog
         */
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
        trace("Info: Table fog not supported by this device\n");
    }

    /* Now test the special case fogstart == fogend */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK)
    {
        start = 512;
        end = 512;
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
        ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
        ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "IDirect3DDevice9_SetRenderState %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %s\n", DXGetErrorString9(hr));

        /* Untransformed vertex, z coord = 0.1, fogstart = 512, fogend = 512. Would result in
         * a completely fog-free primitive because start > zcoord, but because start == end, the primitive
         * is fully covered by fog. The same happens to the 2nd untransformed quad with z = 1.0.
         * The third transformed quad remains unfogged because the fogcoords are read from the specular
         * color and has fixed fogstart and fogend.
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_1,
                sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_2,
                sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %s\n", DXGetErrorString9(hr));
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000FF00, "Untransformed vertex with vertex fog and z = 0.1 has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000FF00, "Untransformed vertex with vertex fog and z = 1.0 has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %s\n", DXGetErrorString9(hr));
    start = 0.0;
    end = 1.0;
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog end returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    ok( hr == D3D_OK, "IDirect3DDevice9_SetRenderState %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
    ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %s\n", DXGetErrorString9(hr));
}

/* This test verifies the behaviour of cube maps wrt. texture wrapping.
 * D3D cube map wrapping always behaves like GL_CLAMP_TO_EDGE,
 * regardless of the actual addressing mode set. */
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

    hr = IDirect3DDevice9_CreateCubeTexture(device, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateCubeTexture failed (0x%08x)\n", hr);

    /* Create cube faces */
    for (face = 0; face < 6; ++face)
    {
        IDirect3DSurface9 *face_surface = NULL;

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
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

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

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        /* Due to the nature of this test, we sample essentially at the edge
         * between two faces. Because of this it's undefined from which face
         * the driver will sample. Furtunately that's not important for this
         * test, since all we care about is that it doesn't sample from the
         * other side of the surface or from the border. */
        color = getPixelColor(device, 320, 240);
        ok(color == 0x00ff0000 || color == 0x000000ff,
                "Got color 0x%08x for addressing mode %s, expected 0x00ff0000 or 0x000000ff.\n",
                color, address_modes[x].name);

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
    ok(hr == D3D_OK, "Clear failed, hr = %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
    ok(hr == D3D_OK || D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %s\n", DXGetErrorString9(hr));
    if(!offscreenTexture) {
        trace("Failed to create an X8R8G8B8 offscreen texture, trying R5G6B5\n");
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
        ok(hr == D3D_OK || D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %s\n", DXGetErrorString9(hr));
        if(!offscreenTexture) {
            skip("Cannot create an offscreen render target\n");
            goto out;
        }
    }

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %s\n", DXGetErrorString9(hr));
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %s\n", DXGetErrorString9(hr));
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "SetFVF failed, hr = %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK) {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, offscreen);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %s\n", DXGetErrorString9(hr));

        /* Draw without textures - Should resut in a white quad */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) offscreenTexture);
        ok(hr == D3D_OK, "SetTexture failed, %s\n", DXGetErrorString9(hr));

        /* This time with the texture */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed, hr = %s\n", DXGetErrorString9(hr));

        IDirect3DDevice9_EndScene(device);
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

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

out:
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);

    /* restore things */
    if(backbuffer) {
        IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
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
 *                vertex fog with foggy vertex shader
 * What's not tested: non linear fog with shader
 *                    table fog with foggy vertex shader
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
    static const DWORD vertex_shader_code1[] = {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x0000001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                  */
        0x0000ffff
    };
    /* basic vertex shader with reversed fog computation ("foggy") */
    static const DWORD vertex_shader_code2[] = {
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
    /* basic pixel shader */
    static const DWORD pixel_shader_code[] = {
        0xffff0101,                                                             /* ps_1_1     */
        0x00000001, 0x800f0000, 0x90e40000,                                     /* mov r0, vo */
        0x0000ffff
    };

    static struct vertex quad[] = {
        {-1.0f, -1.0f,  0.0f,          0xFFFF0000  },
        {-1.0f,  1.0f,  0.0f,          0xFFFF0000  },
        { 1.0f, -1.0f,  0.0f,          0xFFFF0000  },
        { 1.0f,  1.0f,  0.0f,          0xFFFF0000  },
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexShader9      *vertex_shader[3]   = {NULL, NULL, NULL};
    IDirect3DPixelShader9       *pixel_shader[2]    = {NULL, NULL};

    /* This reference data was collected on a nVidia GeForce 7600GS driver version 84.19 DirectX version 9.0c on Windows XP */
    static const struct test_data_t {
        int vshader;
        int pshader;
        D3DFOGMODE vfog;
        D3DFOGMODE tfog;
        unsigned int color[11];
    } test_data[] = {
        /* only pixel shader: */
        {0, 1, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 3, 0,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

        /* vertex shader */
        {1, 0, 0, 0,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
         0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 0, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

        /* vertex shader and pixel shader */
        {1, 1, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

#if 0  /* FIXME: these fail on GeForce 8500 */
        /* foggy vertex shader */
        {2, 0, 0, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 1, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 2, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 3, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
#endif

        /* foggy vertex shader and pixel shader */
        {2, 1, 0, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 1, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 2, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 3, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

    };

    /* NOTE: changing these values will not affect the tests with foggy vertex shader, as the values are hardcoded in the shader*/
    start.f=0.9f;
    end.f=0.1f;

    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code1, &vertex_shader[1]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code2, &vertex_shader[2]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &pixel_shader[1]);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
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

            IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

            /* As the red and green component are the result of blending use 5% tolerance on the expected value */
            color = getPixelColor(device, 128, 240);
            ok((unsigned char)(color) == ((unsigned char)test_data[i].color[j])
                    && abs( ((unsigned char)(color>>8)) - (unsigned char)(test_data[i].color[j]>>8) ) < 13
                    && abs( ((unsigned char)(color>>16)) - (unsigned char)(test_data[i].color[j]>>16) ) < 13,
                    "fog ps%i vs%i fvm%i ftm%i %d: got color %08x, expected %08x +-5%%\n", test_data[i].vshader, test_data[i].pshader, test_data[i].vfog, test_data[i].tfog, j, color, test_data[i].color[j]);
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
    IDirect3DPixelShader9_Release(pixel_shader[1]);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
}

/* test the behavior of the texbem instruction
 * with normal 2D and projective 2D textures
 */
static void texbem_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    unsigned int i, x, y;

    static const DWORD pixel_shader_code[] = {
        0xffff0101,                         /* ps_1_1*/
        0x00000042, 0xb00f0000,             /* tex t0*/
        0x00000043, 0xb00f0001, 0xb0e40000, /* texbem t1, t0*/
        0x00000001, 0x800f0000, 0xb0e40001, /* mov r0, t1*/
        0x0000ffff
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

    /* use assymetric matrix to test loading */
    float bumpenvmat[4] = {0.0,0.5,-0.5,0.0};

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DPixelShader9       *pixel_shader       = NULL;
    IDirect3DTexture9           *texture[2]         = {NULL, NULL};
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

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        color = getPixelColor(device, 320-32, 240);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320+32, 240);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240-32);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240+32);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

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
        hr = IDirect3DDevice9_SetTexture(device, i, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);
        IDirect3DCubeTexture9_Release(texture[i]);
    }
}

static void z_range_test(IDirect3DDevice9 *device)
{
    const struct vertex quad[] =
    {
        {-1.0f,  0.0f,   1.1f,                          0xffff0000},
        {-1.0f,  1.0f,   1.1f,                          0xffff0000},
        { 1.0f,  0.0f,  -1.1f,                          0xffff0000},
        { 1.0f,  1.0f,  -1.1f,                          0xffff0000},
    };
    const struct vertex quad2[] =
    {
        {-1.0f,  0.0f,   1.1f,                          0xff0000ff},
        {-1.0f,  1.0f,   1.1f,                          0xff0000ff},
        { 1.0f,  0.0f,  -1.1f,                          0xff0000ff},
        { 1.0f,  1.0f,  -1.1f,                          0xff0000ff},
    };

    const struct tvertex quad3[] =
    {
        {    0,   240,   1.1f,  1.0,                    0xffffff00},
        {    0,   480,   1.1f,  1.0,                    0xffffff00},
        {  640,   240,  -1.1f,  1.0,                    0xffffff00},
        {  640,   480,  -1.1f,  1.0,                    0xffffff00},
    };
    const struct tvertex quad4[] =
    {
        {    0,   240,   1.1f,  1.0,                    0xff00ff00},
        {    0,   480,   1.1f,  1.0,                    0xff00ff00},
        {  640,   240,  -1.1f,  1.0,                    0xff00ff00},
        {  640,   480,  -1.1f,  1.0,                    0xff00ff00},
    };
    HRESULT hr;
    DWORD color;
    IDirect3DVertexShader9 *shader;
    IDirect3DVertexDeclaration9 *decl;
    D3DCAPS9 caps;
    const DWORD shader_code[] = {
        0xfffe0101,                                     /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position v0  */
        0x00000001, 0xc00f0000, 0x90e40000,             /* mov oPos, v0     */
        0x00000001, 0xd00f0000, 0xa0e40000,             /* mov oD0, c0      */
        0x0000ffff                                      /* end              */
    };
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    /* Does the Present clear the depth stencil? Clear the depth buffer with some value != 0,
     * then call Present. Then clear the color buffer to make sure it has some defined content
     * after the Present with D3DSWAPEFFECT_DISCARD. After that draw a plane that is somewhere cut
     * by the depth value.
     */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.75, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.4, 0);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3D_OK)
    {
        /* Test the untransformed vertex path */
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad2, sizeof(quad2[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        /* Test the transformed vertex path */
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad4, sizeof(quad4[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad3, sizeof(quad3[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    /* Do not test the exact corner pixels, but go pretty close to them */

    /* Clipped because z > 1.0 */
    color = getPixelColor(device, 28, 238);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 28, 241);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    /* Not clipped, > z buffer clear value(0.75) */
    color = getPixelColor(device, 31, 238);
    ok(color == 0x00ff0000, "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 31, 241);
    ok(color == 0x00ffff00, "Z range failed: Got color 0x%08x, expected 0x00ffff00.\n", color);
    color = getPixelColor(device, 100, 238);
    ok(color == 0x00ff0000, "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 100, 241);
    ok(color == 0x00ffff00, "Z range failed: Got color 0x%08x, expected 0x00ffff00.\n", color);

    /* Not clipped, < z buffer clear value */
    color = getPixelColor(device, 104, 238);
    ok(color == 0x000000ff, "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 104, 241);
    ok(color == 0x0000ff00, "Z range failed: Got color 0x%08x, expected 0x0000ff00.\n", color);
    color = getPixelColor(device, 318, 238);
    ok(color == 0x000000ff, "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 318, 241);
    ok(color == 0x0000ff00, "Z range failed: Got color 0x%08x, expected 0x0000ff00.\n", color);

    /* Clipped because z < 0.0 */
    color = getPixelColor(device, 321, 238);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    color = getPixelColor(device, 321, 241);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    /* Test the shader path */
    IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (caps.VertexShaderVersion < D3DVS_VERSION(1, 1)) {
        skip("Vertex shaders not supported\n");
        goto out;
    }
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.4, 0);

    IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetVertexShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3D_OK)
    {
        float colorf[] = {1.0, 0.0, 0.0, 1.0};
        float colorf2[] = {0.0, 0.0, 1.0, 1.0};
        IDirect3DDevice9_SetVertexShaderConstantF(device, 0, colorf, 1);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_LESS);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetVertexShaderConstantF(device, 0, colorf2, 1);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2 /*PrimCount */, quad2, sizeof(quad2[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

    IDirect3DVertexDeclaration9_Release(decl);
    IDirect3DVertexShader9_Release(shader);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);
    /* Z < 1.0 */
    color = getPixelColor(device, 28, 238);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    /* 1.0 < z < 0.75 */
    color = getPixelColor(device, 31, 238);
    ok(color == 0x00ff0000, "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 100, 238);
    ok(color == 0x00ff0000, "Z range failed: Got color 0x%08x, expected 0x00ff0000.\n", color);

    /* 0.75 < z < 0.0 */
    color = getPixelColor(device, 104, 238);
    ok(color == 0x000000ff, "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 318, 238);
    ok(color == 0x000000ff, "Z range failed: Got color 0x%08x, expected 0x000000ff.\n", color);

    /* 0.0 < z */
    color = getPixelColor(device, 321, 238);
    ok(color == 0x00ffffff, "Z range failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

    out:
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
}

static void fill_surface(IDirect3DSurface9 *surface, DWORD color)
{
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT l;
    HRESULT hr;
    unsigned int x, y;
    DWORD *mem;

    memset(&desc, 0, sizeof(desc));
    memset(&l, 0, sizeof(l));
    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(hr == D3D_OK, "IDirect3DSurface9_GetDesc failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DSurface9_LockRect(surface, &l, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect failed with %s\n", DXGetErrorString9(hr));
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
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect failed with %s\n", DXGetErrorString9(hr));
}

static void maxmip_test(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    HRESULT hr;
    DWORD color;
    const float quads[] = {
        -1.0,   -1.0,   0.0,    0.0,    0.0,
        -1.0,    0.0,   0.0,    0.0,    1.0,
         0.0,   -1.0,   0.0,    1.0,    0.0,
         0.0,    0.0,   0.0,    1.0,    1.0,

         0.0,   -1.0,   0.0,    0.0,    0.0,
         0.0,    0.0,   0.0,    0.0,    1.0,
         1.0,   -1.0,   0.0,    1.0,    0.0,
         1.0,    0.0,   0.0,    1.0,    1.0,

         0.0,    0.0,   0.0,    0.0,    0.0,
         0.0,    1.0,   0.0,    0.0,    1.0,
         1.0,    0.0,   0.0,    1.0,    0.0,
         1.0,    1.0,   0.0,    1.0,    1.0,

        -1.0,    0.0,   0.0,    0.0,    0.0,
        -1.0,    1.0,   0.0,    0.0,    1.0,
         0.0,    0.0,   0.0,    1.0,    0.0,
         0.0,    1.0,   0.0,    1.0,    1.0,
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 3, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                        &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %s\n", DXGetErrorString9(hr));
    if(!texture)
    {
        skip("Failed to create test texture\n");
        return;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    fill_surface(surface, 0xffff0000);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 1, &surface);
    fill_surface(surface, 0xff00ff00);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 2, &surface);
    fill_surface(surface, 0xff0000ff);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[ 0], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[20], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[40], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[60], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);
    /* With mipmapping disabled, the max mip level is ignored, only level 0 is used */
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "MapMip 0, no mipfilter has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00FF0000, "MapMip 3, no mipfilter has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FF0000, "MapMip 2, no mipfilter has color %08x\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00FF0000, "MapMip 1, no mipfilter has color %08x\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[ 0], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[20], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[40], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quads[60], 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
    }

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);
    /* Max Mip level 0-2 sample from the specified texture level, Max Mip level 3(> levels in texture)
     * samples from the highest level in the texture(level 2)
     */
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "MapMip 0, point mipfilter has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x000000FF, "MapMip 3, point mipfilter has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000FF, "MapMip 2, point mipfilter has color %08x\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000FF00, "MapMip 1, point mipfilter has color %08x\n", color);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAXMIPLEVEL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
    IDirect3DTexture9_Release(texture);
}

static void release_buffer_test(IDirect3DDevice9 *device)
{
    IDirect3DVertexBuffer9 *vb = NULL;
    IDirect3DIndexBuffer9 *ib = NULL;
    HRESULT hr;
    BYTE *data;
    long ref;

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
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %s\n", DXGetErrorString9(hr));
    if(!vb) {
        skip("Failed to create a vertex buffer\n");
        return;
    }
    hr = IDirect3DDevice9_CreateIndexBuffer(device, sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateIndexBuffer failed with %s\n", DXGetErrorString9(hr));
    if(!ib) {
        skip("Failed to create an index buffer\n");
        return;
    }

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DIndexBuffer9_Lock(ib, 0, sizeof(indices), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
    memcpy(data, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(ib);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer9_Unlock failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetIndices(device, ib);
    ok(hr == D3D_OK, "IDirect3DIndexBuffer8_Unlock failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad[0]));
    ok(hr == D3D_OK, "IDirect3DIndexBuffer8_Unlock failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

    /* Now destroy the bound index buffer and draw again */
    ref = IDirect3DIndexBuffer9_Release(ib);
    ok(ref == 0, "Index Buffer reference count is %08ld\n", ref);

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
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %s\n", DXGetErrorString9(hr));
    if(!texture) {
        skip("Failed to create R32F texture\n");
        goto out;
    }

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed with %s\n", DXGetErrorString9(hr));
    data = lr.pBits;
    *data = 0.0;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 240, 320);
    ok(color == 0x0000FFFF, "R32F with value 0.0 has color %08x, expected 0x0000FFFF\n", color);

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
    DWORD color, red, green, blue;
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
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %s\n", DXGetErrorString9(hr));
    if(!texture) {
        skip("Failed to create D3DFMT_G16R16 texture\n");
        goto out;
    }

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed with %s\n", DXGetErrorString9(hr));
    data = lr.pBits;
    *data = 0x0f00f000;
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 240, 320);
    red   = (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue  = (color & 0x000000ff) >>  0;
    ok(blue == 0xff && red >= 0xef && red <= 0xf1 && green >= 0x0e && green <= 0x10,
       "D3DFMT_G16R16 with value 0x00ffff00 has color %08x, expected 0x00F00FFF\n", color);

out:
    if(texture) IDirect3DTexture9_Release(texture);
    IDirect3D9_Release(d3d);
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
    IDirect3DVertexDeclaration9 *decl, *decl2;
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

    memset(&lr, 0, sizeof(lr));
    memset(&lb, 0, sizeof(lb));
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0,
                                     D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16) == D3D_OK) {
        fmt = D3DFMT_A16B16G16R16;
    }
    IDirect3D9_Release(d3d);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements2, &decl2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_SRGBTEXTURE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_SRGBTEXTURE) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MAGFILTER) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MINFILTER) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_MIPFILTER) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSU) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSV) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState(D3DSAMP_ADDRESSW) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState(D3DRS_LIGHTING) returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateTexture(device, caps.MaxTextureWidth, caps.MaxTextureHeight, 1,
                                        0, fmt, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %s\n", DXGetErrorString9(hr));
    if(!texture) {
        skip("Failed to create the test texture\n");
        return;
    }

    /* Unfortunately there is no easy way to set up a texture coordinate passthrough
     * in d3d fixed function pipeline, so create a texture that has a gradient from 0.0 to
     * 1.0 in red and green for the x and y coords
     */
    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %s\n", DXGetErrorString9(hr));
    for(y = 0; y < caps.MaxTextureHeight; y++) {
        for(x = 0; x < caps.MaxTextureWidth; x++) {
            double r_f = (double) y / (double) caps.MaxTextureHeight;
            double g_f = (double) x / (double) caps.MaxTextureWidth;
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
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
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
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens with transforms enabled? */
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens if 4 coords are used, but only 2 given ?*/
        mat[8] = 1.0;
        mat[13] = 1.0;
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* What happens with transformed geometry? This setup lead to 0/0 coords with untransformed
         * geometry. If the same applies to transformed vertices, the quad will be black, otherwise red,
         * due to the coords in the vertices. (turns out red, indeed)
         */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_TEX1);
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FFFF00 || color == 0x00FEFE00, "quad 1 has color %08x, expected 0x00FFFF00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x0000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x0000FF00 || color == 0x0000FE00, "quad 3 has color %08x, expected 0x0000FF00\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00FF0000 || 0x00FE0000, "quad 4 has color %08x, expected 0x00FF0000\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
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

        /* What happens to the default 1 in the 3rd coordinate if it is disabled?
         */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* D3DTFF_COUNT1 does not work on Nvidia drivers. It behaves like D3DTTFF_DISABLE. On ATI drivers
         * it behaves like COUNT2 because normal textures require 2 coords
         */
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Just to be sure, the same as quad2 above */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Now, what happens to the 2nd coordinate(that is disabled in the matrix) if it is not
         * used? And what happens to the first?
         */
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000 || color == 0x00FE0000, "quad 1 has color %08x, expected 0x00FF0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x0000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff8000 || color == 0x00fe7f00 || color == 0x00000000,
       "quad 3 has color %08x, expected 0x00ff8000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0033cc00 || color == 0x0032cb00 || color == 0x00FF0000 || color == 0x00FE0000,
       "quad 4 has color %08x, expected 0x0033cc00\n", color);

    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff203040, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));
    /* Use a smaller volume texture than the biggest possible size for memory and performance reasons
     * Thus watch out if sampling from texels between 0 and 1.
     */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 32, 32, 32, 1, 0, fmt, D3DPOOL_MANAGED, &volume, 0);
    ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
       "IDirect3DDevice9_CreateVolumeTexture failed with %s\n", DXGetErrorString9(hr));
    if(!volume) {
        skip("Failed to create a volume texture\n");
        goto out;
    }

    hr = IDirect3DVolumeTexture9_LockBox(volume, 0, &lb, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_LockBox failed with %s\n", DXGetErrorString9(hr));
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
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_UnlockBox failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) volume);
    ok(hr == D3D_OK, "IDirect3DVolumeTexture9_UnlockBox failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
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
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));

        /* Draw a quad with all 3 coords enabled. Nothing fancy. v and w are swapped, but have the same
         * values
         */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* Now disable the w coordinate. Does that change the input, or the output. The coordinates
         * are swapped by the matrix. If it changes the input, the v coord will be missing(green),
         * otherwise the w will be missing(blue).
         * turns out that on nvidia cards the blue color is missing, so it is an output modification.
         * On ATI cards the COUNT2 is ignored, and it behaves in the same way as COUNT3.
         */
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* default values? Set up the identity matrix, pass in 2 vertex coords, and enable 4 */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) identity);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* D3DTTFF_COUNT1. Set a NULL matrix, and count1, pass in all values as 1.0. Nvidia has count1 ==
         * disable. ATI extends it up to the amount of values needed for the volume texture
         */
        memset(mat, 0, sizeof(mat));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ffffff, "quad 1 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00ffff00 /* NV*/ || color == 0x00ffffff /* ATI */,
       "quad 2 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "quad 3 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ffffff || color == 0x0000ff00, "quad 4 has color %08x, expected 0x00ffffff\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff303030, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
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
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));

        /* Default values? 4 coords used, 3 passed. What happens to the 4th?
         */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* None passed */
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) identity);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        /* 4 used, 1 passed */
        hr = IDirect3DDevice9_SetVertexDeclaration(device, decl2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) mat2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 4 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 160, 360);
    ok(color == 0x0000ff00, "quad 1 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00000000, "quad 2 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff0000, "quad 3 has color %08x, expected 0x00ff0000\n", color);
    /* Quad4: unused */

    IDirect3DVolumeTexture9_Release(volume);

    out:
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetTextureStageState failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *) &identity);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));
    IDirect3DVertexDeclaration9_Release(decl);
    IDirect3DVertexDeclaration9_Release(decl2);
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
    DWORD shader_code[] = {
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
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_ALWAYS);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);

    /* Fill the depth buffer with a gradient */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }

    /* Now perform the actual tests. Same geometry, but with the shader */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZFUNC, D3DCMP_GREATER);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 158, 240);
    ok(color == 0x000000ff, "Pixel 158(25%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 162, 240);
    ok(color == 0x00ffffff, "Pixel 158(25%% + 2 pixel) has color %08x, expected 0x00ffffff\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 318, 240);
    ok(color == 0x000000ff, "Pixel 318(50%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 322, 240);
    ok(color == 0x00ffff00, "Pixel 322(50%% + 2 pixel) has color %08x, expected 0x00ffff00\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data3, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 1, 240);
    ok(color == 0x00ff0000, "Pixel 1(0%% + 2 pixel) has color %08x, expected 0x00ff0000\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data4, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 318, 240);
    ok(color == 0x000000ff, "Pixel 318(50%% - 2 pixel) has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 322, 240);
    ok(color == 0x0000ff00, "Pixel 322(50%% + 2 pixel) has color %08x, expected 0x0000ff00\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data5, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 1, 240);
    ok(color == 0x00ffff00, "Pixel 1(0%% + 2 pixel) has color %08x, expected 0x00ffff00\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data6, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 638, 240);
    ok(color == 0x000000ff, "Pixel 638(100%% + 2 pixel) has color %08x, expected 0x000000ff\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, texdepth_test_data7, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 638, 240);
    ok(color == 0x000000ff, "Pixel 638(100%% + 2 pixel) has color %08x, expected 0x000000ff\n", color);

    /* Cleanup */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
    IDirect3DPixelShader9_Release(shader);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
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

    DWORD shader_code_11[] = {
    0xffff0101,                                                             /* ps_1_1                     */
    0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 1.0, 0.0, 0.0, 1.0 */
    0x00000041, 0xb00f0000,                                                 /* texkill t0                 */
    0x00000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                 */
    0x0000ffff                                                              /* end                        */
    };
    DWORD shader_code_20[] = {
    0xffff0200,                                                             /* ps_2_0                     */
    0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                     */
    0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* def c0, 0.0, 0.0, 1.0, 1.0 */
    0x01000041, 0xb00f0000,                                                 /* texkill t0                 */
    0x02000001, 0x800f0800, 0xa0e40000,                                     /* mov oC0, c0                */
    0x0000ffff                                                              /* end                        */
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 7 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
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

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    IDirect3DPixelShader9_Release(shader);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_20, &shader);
    if(FAILED(hr)) {
        skip("Failed to create 2.0 test shader, most likely not supported\n");
        return;
    }

    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, vertex, 7 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
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
    DWORD shader_code[] = {
        0xffff0101,                             /* ps_1_1       */
        0x00000042, 0xb00f0000,                 /* tex t0       */
        0x00000001, 0x800f0000, 0xb0e40000,     /* mov r0, t0   */
        0x0000ffff                              /* end          */
    };
    DWORD shader_code2[] = {
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
    };

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

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
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 578, 430);
    ok(color == 0x008262ca || color == 0x008363ca || color == 0x008362ca,
       "D3DFMT_X8L8V8U8 = 0x112131ca returns color %08x, expected 0x008262ca\n", color);

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
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 578, 430);
    ok(color == 0x00ffffff, "w component of D3DFMT_X8L8V8U8 = 0x11ca3141 returns color %08x\n", color);

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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    /* Make the mipmap big, so that a smaller mipmap is used
     */
    hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 0, D3DUSAGE_AUTOGENMIPMAP,
                                        D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %s\n", DXGetErrorString9(hr));
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
    ok(hr == D3D_OK, "IDirect3DSurface9_UnlockRect returned %s\n", DXGetErrorString9(hr));
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        const float quad[] =  {
           -0.5,   -0.5,    0.1,    0.0,    0.0,
           -0.5,    0.5,    0.1,    0.0,    1.0,
            0.5,   -0.5,    0.1,    1.0,    0.0,
            0.5,    0.5,    0.1,    1.0,    1.0
        };

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 5 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed with %s\n", DXGetErrorString9(hr));
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
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
}

static void test_constant_clamp_vs(IDirect3DDevice9 *device)
{
    IDirect3DVertexShader9 *shader_11, *shader_11_2, *shader_20, *shader_20_2;
    IDirect3DVertexDeclaration9 *decl;
    HRESULT hr;
    DWORD color;
    DWORD shader_code_11[] =  {
        0xfffe0101,                                         /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    DWORD shader_code_11_2[] =  {
        0xfffe0101,                                         /* vs_1_1           */
        0x00000051, 0xa00f0001, 0x3fa00000, 0xbf000000, 0xbfc00000, 0x3f800000, /* dcl ... */
        0x00000051, 0xa00f0002, 0xbf000000, 0x3fa00000, 0x40000000, 0x3f800000, /* dcl ... */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    DWORD shader_code_20[] =  {
        0xfffe0200,                                         /* vs_2_0           */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    DWORD shader_code_20_2[] =  {
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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_11_2, &shader_11_2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_20, &shader_20);
    if(FAILED(hr)) shader_20 = NULL;
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code_20_2, &shader_20_2);
    if(FAILED(hr)) shader_20_2 = NULL;
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetVertexShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_11_2);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        if(shader_20) {
            hr = IDirect3DDevice9_SetVertexShader(device, shader_20);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        if(shader_20_2) {
            hr = IDirect3DDevice9_SetVertexShader(device, shader_20_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00bfbf80 || color == 0x00bfbf7f || color == 0x00bfbf81,
       "quad 1 has color %08x, expected 0x00bfbf80\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00bfbf80 || color == 0x00bfbf7f || color == 0x00bfbf81,
       "quad 2 has color %08x, expected 0x00bfbf80\n", color);
    if(shader_20) {
        color = getPixelColor(device, 160, 120);
        ok(color == 0x00bfbf80 || color == 0x00bfbf7f || color == 0x00bfbf81,
           "quad 3 has color %08x, expected 0x00bfbf80\n", color);
    }
    if(shader_20_2) {
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00bfbf80 || color == 0x00bfbf7f || color == 0x00bfbf81,
           "quad 4 has color %08x, expected 0x00bfbf80\n", color);
    }

    IDirect3DVertexDeclaration9_Release(decl);
    if(shader_20_2) IDirect3DVertexShader9_Release(shader_20_2);
    if(shader_20) IDirect3DVertexShader9_Release(shader_20);
    IDirect3DVertexShader9_Release(shader_11_2);
    IDirect3DVertexShader9_Release(shader_11);
}

static void constant_clamp_ps_test(IDirect3DDevice9 *device)
{
    IDirect3DPixelShader9 *shader_11, *shader_12, *shader_14, *shader_20;
    HRESULT hr;
    DWORD color;
    DWORD shader_code_11[] =  {
        0xffff0101,                                         /* ps_1_1           */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    DWORD shader_code_12[] =  {
        0xffff0102,                                         /* ps_1_2           */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    /* Skip 1.3 shaders because we have only 4 quads(ok, could make them smaller if needed).
     * 1.2 and 1.4 shaders behave the same, so it's unlikely that 1.3 shaders are different.
     * During development of this test, 1.3 shaders were verified too
     */
    DWORD shader_code_14[] =  {
        0xffff0104,                                         /* ps_1_4           */
        /* Try to make one constant local. It gets clamped too, although the binary contains
         * the bigger numbers
         */
        0x00000051, 0xa00f0002, 0xbf000000, 0x3fa00000, 0x40000000, 0x3f800000, /* def c2, -0.5, 1.25, 2, 1 */
        0x00000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x00000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x0000ffff                                          /* end              */
    };
    DWORD shader_code_20[] =  {
        0xffff0200,                                         /* ps_2_0           */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x02000001, 0x800f0800, 0x80e40000,                 /* mov oC0, r0      */
        0x0000ffff                                          /* end              */
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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12, &shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14, &shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_20, &shader_20);
    if(FAILED(hr)) shader_20 = NULL;

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        if(shader_20) {
            hr = IDirect3DDevice9_SetPixelShader(device, shader_20);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 3 * sizeof(float));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00808000 || color == 0x007f7f00 || color == 0x00818100,
       "quad 1 has color %08x, expected 0x00808000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00808000 || color == 0x007f7f00 || color == 0x00818100,
       "quad 2 has color %08x, expected 0x00808000\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00808000 || color == 0x007f7f00 || color == 0x00818100,
       "quad 3 has color %08x, expected 0x00808000\n", color);
    if(shader_20) {
        color = getPixelColor(device, 160, 120);
        ok(color == 0x00bfbf80 || color == 0x00bfbf7f || color == 0x00bfbf81,
           "quad 4 has color %08x, expected 0x00bfbf80\n", color);
    }

    if(shader_20) IDirect3DPixelShader9_Release(shader_20);
    IDirect3DPixelShader9_Release(shader_14);
    IDirect3DPixelShader9_Release(shader_12);
    IDirect3DPixelShader9_Release(shader_11);
}

static void cnd_test(IDirect3DDevice9 *device)
{
    IDirect3DPixelShader9 *shader_11, *shader_12, *shader_13, *shader_14;
    IDirect3DPixelShader9 *shader_11_coissue, *shader_12_coissue, *shader_13_coissue, *shader_14_coissue;
    HRESULT hr;
    DWORD color;
    /* ps 1.x shaders are rather picky with writemasks and source swizzles. The dp3 is
     * used to copy r0.r to all components of r1, then copy r1.a to c0.a. Essentially it
     * does a mov r0.a, r0.r, which isn't allowed as-is in 1.x pixel shaders.
     */
    DWORD shader_code_11[] =  {
        0xffff0101,                                                                 /* ps_1_1               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, ???(t0)      */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3 r1, r0, c0       */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    DWORD shader_code_12[] =  {
        0xffff0102,                                                                 /* ps_1_2               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, t0           */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3 r1, r0, c0       */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    DWORD shader_code_13[] =  {
        0xffff0103,                                                                 /* ps_1_3               */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000,     /* def c0, 1, 0, 0, 0   */
        0x00000040, 0xb00f0000,                                                     /* texcoord t0          */
        0x00000001, 0x800f0000, 0xb0e40000,                                         /* mov r0, t0           */
        0x00000008, 0x800f0001, 0x80e40000, 0xa0e40000,                             /* dp3, r1, r0, c0      */
        0x00000001, 0x80080000, 0x80ff0001,                                         /* mov r0.a, r1.a       */
        0x00000050, 0x800f0000, 0x80ff0000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0.a, c1, c2 */
        0x0000ffff                                                                  /* end                  */
    };
    DWORD shader_code_14[] =  {
        0xffff0104,                                                                 /* ps_1_3               */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,     /* def c0, 0, 0, 0, 1   */
        0x00000040, 0x80070000, 0xb0e40000,                                         /* texcrd r0, t0        */
        0x00000001, 0x80080000, 0xa0ff0000,                                         /* mov r0.a, c0.a       */
        0x00000050, 0x800f0000, 0x80e40000, 0xa0e40001, 0xa0e40002,                 /* cnd r0, r0, c1, c2   */
        0x0000ffff                                                                  /* end                  */
    };

    /* Special fun: The coissue flag on cnd: Apparently cnd always selects the 2nd source,
     * as if the src0 comparison against 0.5 always evaluates to true. The coissue flag isn't
     * set by the compiler, it was added manually after compilation. It isn't always allowed,
     * only if there's a mov r0.a, XXXX, and the cnd instruction writes to r0.xyz, otherwise
     * native CreatePixelShader returns an error.
     *
     * The shader attempts to test the range [-1;1] against coissued cnd, which is a bit tricky.
     * The input from t0 is [0;1]. 0.5 is substracted, then we have to multiply with 2. Since
     * constants are clamped to [-1;1], a 2.0 is constructed by adding c0.r(=1.0) to c0.r into r1.r,
     * then r1(2.0, 0.0, 0.0, 0.0) is passed to dp3(explained above).
     */
    DWORD shader_code_11_coissue[] =  {
        0xffff0101,                                                             /* ps_1_1                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3, r1, r0, r1          */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        /* 0x40000000 = D3DSI_COISSUE */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* cnd r0.xyz, r0.a, c1, c2 */
        0x0000ffff                                                              /* end                      */
    };
    DWORD shader_code_12_coissue[] =  {
        0xffff0102,                                                             /* ps_1_2                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3, r1, r0, r1          */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        /* 0x40000000 = D3DSI_COISSUE */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* cnd r0.xyz, r0.a, c1, c2 */
        0x0000ffff                                                              /* end                      */
    };
    DWORD shader_code_13_coissue[] =  {
        0xffff0103,                                                             /* ps_1_3                   */
        0x00000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 1, 0, 0, 0       */
        0x00000051, 0xa00f0003, 0x3f000000, 0x3f000000, 0x3f000000, 0x00000000, /* def c3, 0.5, 0.5, 0.5, 0 */
        0x00000040, 0xb00f0000,                                                 /* texcoord t0              */
        0x00000001, 0x800f0000, 0xb0e40000,                                     /* mov r0, t0               */
        0x00000003, 0x800f0000, 0x80e40000, 0xa0e40003,                         /* sub r0, r0, c3           */
        0x00000002, 0x800f0001, 0xa0e40000, 0xa0e40000,                         /* add r1, c0, c0           */
        0x00000008, 0x800f0001, 0x80e40000, 0x80e40001,                         /* dp3, r1, r0, r1          */
        0x00000001, 0x80080000, 0x80ff0001,                                     /* mov r0.a, r1.a           */
        /* 0x40000000 = D3DSI_COISSUE */
        0x40000050, 0x80070000, 0x80ff0000, 0xa0e40001, 0xa0e40002,             /* cnd r0.xyz, r0.a, c1, c2 */
        0x0000ffff                                                              /* end                      */
    };
    /* ps_1_4 does not have a different cnd behavior, just pass the [0;1] texcrd result to cnd, it will
     * compare against 0.5
     */
    DWORD shader_code_14_coissue[] =  {
        0xffff0104,                                                             /* ps_1_4                   */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 0, 0, 0, 1       */
        0x00000040, 0x80070000, 0xb0e40000,                                     /* texcrd r0, t0            */
        0x00000001, 0x80080000, 0xa0ff0000,                                     /* mov r0.a, c0.a           */
        /* 0x40000000 = D3DSI_COISSUE */
        0x40000050, 0x80070000, 0x80e40000, 0xa0e40001, 0xa0e40002,             /* cnd r0.xyz, r0, c1, c2   */
        0x0000ffff                                                              /* end                      */
    };
    float quad1[] = {
        -1.0,   -1.0,   0.1,     0.0,    0.0,    1.0,
         0.0,   -1.0,   0.1,     1.0,    0.0,    1.0,
        -1.0,    0.0,   0.1,     0.0,    1.0,    0.0,
         0.0,    0.0,   0.1,     1.0,    1.0,    0.0
    };
    float quad2[] = {
         0.0,   -1.0,   0.1,     0.0,    0.0,    1.0,
         1.0,   -1.0,   0.1,     1.0,    0.0,    1.0,
         0.0,    0.0,   0.1,     0.0,    1.0,    0.0,
         1.0,    0.0,   0.1,     1.0,    1.0,    0.0
    };
    float quad3[] = {
         0.0,    0.0,   0.1,     0.0,    0.0,    1.0,
         1.0,    0.0,   0.1,     1.0,    0.0,    1.0,
         0.0,    1.0,   0.1,     0.0,    1.0,    0.0,
         1.0,    1.0,   0.1,     1.0,    1.0,    0.0
    };
    float quad4[] = {
        -1.0,    0.0,   0.1,     0.0,    0.0,    1.0,
         0.0,    0.0,   0.1,     1.0,    0.0,    1.0,
        -1.0,    1.0,   0.1,     0.0,    1.0,    0.0,
         0.0,    1.0,   0.1,     1.0,    1.0,    0.0
    };
    float test_data_c1[4] = {  0.0, 0.0, 0.0, 0.0};
    float test_data_c2[4] = {  1.0, 1.0, 1.0, 1.0};
    float test_data_c1_coi[4] = {  0.0, 1.0, 0.0, 0.0};
    float test_data_c2_coi[4] = {  1.0, 0.0, 1.0, 1.0};

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11, &shader_11);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12, &shader_12);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_13, &shader_13);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14, &shader_14);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_11_coissue, &shader_11_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_12_coissue, &shader_12_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_13_coissue, &shader_13_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code_14_coissue, &shader_14_coissue);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_13);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

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
    ok(color == 0x00000000, "pixel 162, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 158, 362);
    ok(color == 0x00ffffff, "pixel 158, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color == 0x00000000, "pixel 162, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 478, 358);
    ok(color == 0x00ffffff, "pixel 478, 358 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color == 0x00000000, "pixel 482, 358 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x00ffffff, "pixel 478, 362 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color == 0x00000000, "pixel 482, 362 has color %08x, expected 0x00000000\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 478, 118);
    ok(color == 0x00ffffff, "pixel 478, 118 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 118);
    ok(color == 0x00000000, "pixel 482, 118 has color %08x, expected 0x00000000\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color == 0x00ffffff, "pixel 478, 122 has color %08x, expected 0x00ffffff\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color == 0x00000000, "pixel 482, 122 has color %08x, expected 0x00000000\n", color);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff00ffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, test_data_c1_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 2, test_data_c2_coi, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShaderConstantF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, shader_11_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_12_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_13_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, shader_14_coissue);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, 6 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

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

    /* 1.1 shader. coissue flag changed the semantic of cnd, c1 is always selected */
    color = getPixelColor(device, 158, 358);
    ok(color == 0x0000ff00, "pixel 158, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 162, 358);
    ok(color == 0x0000ff00, "pixel 162, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 158, 362);
    ok(color == 0x0000ff00, "pixel 158, 362 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 162, 362);
    ok(color == 0x0000ff00, "pixel 162, 362 has color %08x, expected 0x0000ff00\n", color);

    /* 1.2 shader */
    color = getPixelColor(device, 478, 358);
    ok(color == 0x0000ff00, "pixel 478, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 358);
    ok(color == 0x0000ff00, "pixel 482, 358 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 478, 362);
    ok(color == 0x0000ff00, "pixel 478, 362 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 362);
    ok(color == 0x0000ff00, "pixel 482, 362 has color %08x, expected 0x0000ff00\n", color);

    /* 1.3 shader */
    color = getPixelColor(device, 478, 118);
    ok(color == 0x0000ff00, "pixel 478, 118 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 118);
    ok(color == 0x0000ff00, "pixel 482, 118 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 478, 122);
    ok(color == 0x0000ff00, "pixel 478, 122 has color %08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 482, 122);
    ok(color == 0x0000ff00, "pixel 482, 122 has color %08x, expected 0x0000ff00\n", color);

    IDirect3DPixelShader9_Release(shader_14_coissue);
    IDirect3DPixelShader9_Release(shader_13_coissue);
    IDirect3DPixelShader9_Release(shader_12_coissue);
    IDirect3DPixelShader9_Release(shader_11_coissue);
    IDirect3DPixelShader9_Release(shader_14);
    IDirect3DPixelShader9_Release(shader_13);
    IDirect3DPixelShader9_Release(shader_12);
    IDirect3DPixelShader9_Release(shader_11);
}

static void nested_loop_test(IDirect3DDevice9 *device) {
    const DWORD shader_code[] = {
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
    IDirect3DPixelShader9 *shader;
    HRESULT hr;
    DWORD color;
    const float quad[] = {
        -1.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
        -1.0,    1.0,   0.1,
         1.0,    1.0,   0.1
    };

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x0000ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, 3 * sizeof(float));
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 360, 240);
    ok(color == 0x007f0000 || color == 0x00800000 || color == 0x00810000,
       "Nested loop test returned color 0x%08x, expected 0x00800000\n", color);

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed with %s\n", DXGetErrorString9(hr));
    IDirect3DPixelShader9_Release(shader);
}

struct varying_test_struct
{
    const DWORD             *shader_code;
    IDirect3DPixelShader9   *shader;
    DWORD                   color, color_rhw;
    const char              *name;
    BOOL                    todo, todo_rhw;
};

struct hugeVertex
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
    DWORD diffuse, specular;
};

static void fixed_function_varying_test(IDirect3DDevice9 *device) {
    /* dcl_position: fails to compile */
    const DWORD blendweight_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000001, 0x900f0000,     /* dcl_blendweight, v0      */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD blendindices_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000002, 0x900f0000,     /* dcl_blendindices, v0     */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD normal_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000003, 0x900f0000,     /* dcl_normal, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* psize: fails? */
    const DWORD texcoord0_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000005, 0x900f0000,     /* dcl_texcoord0, v0        */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD tangent_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000006, 0x900f0000,     /* dcl_tangent, v0          */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD binormal_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x80000007, 0x900f0000,     /* dcl_binormal, v0         */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* tessfactor: fails */
    /* positiont: fails */
    const DWORD color_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000a, 0x900f0000,     /* dcl_color0, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD fog_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000b, 0x900f0000,     /* dcl_fog, v0              */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD depth_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8000000c, 0x900f0000,     /* dcl_depth, v0            */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    const DWORD specular_code[] = {
        0xffff0300,                             /* ps_3_0                   */
        0x0200001f, 0x8001000a, 0x900f0000,     /* dcl_color1, v0           */
        0x02000001, 0x800f0800, 0x90e40000,     /* mov oC0, v0              */
        0x0000ffff                              /* end                      */
    };
    /* sample: fails */

    struct varying_test_struct tests[] = {
       {blendweight_code,       NULL,       0x00000000,     0x00191919,     "blendweight"   ,   FALSE,  TRUE  },
       {blendindices_code,      NULL,       0x00000000,     0x00000000,     "blendindices"  ,   FALSE,  FALSE },
       {normal_code,            NULL,       0x00000000,     0x004c4c4c,     "normal"        ,   FALSE,  TRUE  },
       /* Why does dx not forward the texcoord? */
       {texcoord0_code,         NULL,       0x00000000,     0x00808c8c,     "texcoord0"     ,   FALSE,  FALSE },
       {tangent_code,           NULL,       0x00000000,     0x00999999,     "tangent"       ,   FALSE,  TRUE  },
       {binormal_code,          NULL,       0x00000000,     0x00b2b2b2,     "binormal"      ,   FALSE,  TRUE  },
       {color_code,             NULL,       0x00e6e6e6,     0x00e6e6e6,     "color"         ,   FALSE,  FALSE },
       {fog_code,               NULL,       0x00000000,     0x00666666,     "fog"           ,   FALSE,  TRUE  },
       {depth_code,             NULL,       0x00000000,     0x00cccccc,     "depth"         ,   FALSE,  TRUE  },
       {specular_code,          NULL,       0x004488ff,     0x004488ff,     "specular"      ,   FALSE,  FALSE }
    };
    /* Declare a monster vertex type :-) */
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,   0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,       0},
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
    static const D3DVERTEXELEMENT9 decl_elements2[] = {
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
    struct hugeVertex data[4] = {
        {
            -1.0,   -1.0,   0.1,    1.0,
             0.1,    0.1,   0.1,    0.1,
             0.2,    0.2,   0.2,    0.2,
             0.3,    0.3,   0.3,    0.3,
             0.4,    0.4,   0.4,    0.4,
             0.50,   0.55,  0.55,   0.55,
             0.6,    0.6,   0.6,    0.7,
             0.7,    0.7,   0.7,    0.6,
             0.8,    0.8,   0.8,    0.8,
             0xe6e6e6e6, /* 0.9 * 256 */
             0x224488ff  /* Nothing special */
        },
        {
             1.0,   -1.0,   0.1,    1.0,
             0.1,    0.1,   0.1,    0.1,
             0.2,    0.2,   0.2,    0.2,
             0.3,    0.3,   0.3,    0.3,
             0.4,    0.4,   0.4,    0.4,
             0.50,   0.55,  0.55,   0.55,
             0.6,    0.6,   0.6,    0.7,
             0.7,    0.7,   0.7,    0.6,
             0.8,    0.8,   0.8,    0.8,
             0xe6e6e6e6, /* 0.9 * 256 */
             0x224488ff /* Nothing special */
        },
        {
            -1.0,    1.0,   0.1,    1.0,
             0.1,    0.1,   0.1,    0.1,
             0.2,    0.2,   0.2,    0.2,
             0.3,    0.3,   0.3,    0.3,
             0.4,    0.4,   0.4,    0.4,
             0.50,   0.55,  0.55,   0.55,
             0.6,    0.6,   0.6,    0.7,
             0.7,    0.7,   0.7,    0.6,
             0.8,    0.8,   0.8,    0.8,
             0xe6e6e6e6, /* 0.9 * 256 */
             0x224488ff /* Nothing special */
        },
        {
             1.0,    1.0,   0.1,    1.0,
             0.1,    0.1,   0.1,    0.1,
             0.2,    0.2,   0.2,    0.2,
             0.3,    0.3,   0.3,    0.3,
             0.4,    0.4,   0.4,    0.4,
             0.50,   0.55,  0.55,   0.55,
             0.6,    0.6,   0.6,    0.7,
             0.7,    0.7,   0.7,    0.6,
             0.8,    0.8,   0.8,    0.8,
             0xe6e6e6e6, /* 0.9 * 256 */
             0x224488ff /* Nothing special */
        },
    };
    struct hugeVertex data2[4];
    IDirect3DVertexDeclaration9 *decl;
    IDirect3DVertexDeclaration9 *decl2;
    HRESULT hr;
    unsigned int i;
    DWORD color, r, g, b, r_e, g_e, b_e;

    memcpy(data2, data, sizeof(data2));
    data2[0].pos_x = 0;     data2[0].pos_y = 0;
    data2[1].pos_x = 640;   data2[1].pos_y = 0;
    data2[2].pos_x = 0;     data2[2].pos_y = 480;
    data2[3].pos_x = 640;   data2[3].pos_y = 480;

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements2, &decl2);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        hr = IDirect3DDevice9_CreatePixelShader(device, tests[i].shader_code, &tests[i].shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed for shader %s, hr = %s\n",
           tests[i].name, DXGetErrorString9(hr));
    }

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

        IDirect3DDevice9_SetPixelShader(device, tests[i].shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, data, sizeof(data[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
        }
        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

        color = getPixelColor(device, 360, 240);
        r = color & 0x00ff0000 >> 16;
        g = color & 0x0000ff00 >>  8;
        b = color & 0x000000ff;
        r_e = tests[i].color & 0x00ff0000 >> 16;
        g_e = tests[i].color & 0x0000ff00 >>  8;
        b_e = tests[i].color & 0x000000ff;

        if(tests[i].todo) {
            todo_wine ok(abs(r - r_e) <= 1 && abs(g - g_e) <= 1 && abs(b - b_e) <= 1,
                         "Test %s returned color 0x%08x, expected 0x%08x(todo)\n",
                         tests[i].name, color, tests[i].color);
        } else {
            ok(abs(r - r_e) <= 1 && abs(g - g_e) <= 1 && abs(b - b_e) <= 1,
               "Test %s returned color 0x%08x, expected 0x%08x\n",
               tests[i].name, color, tests[i].color);
        }
    }

    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl2);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

        IDirect3DDevice9_SetPixelShader(device, tests[i].shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, data2, sizeof(data2[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);
            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
        }
        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

        color = getPixelColor(device, 360, 240);
        r = color & 0x00ff0000 >> 16;
        g = color & 0x0000ff00 >>  8;
        b = color & 0x000000ff;
        r_e = tests[i].color_rhw & 0x00ff0000 >> 16;
        g_e = tests[i].color_rhw & 0x0000ff00 >>  8;
        b_e = tests[i].color_rhw & 0x000000ff;

        if(tests[i].todo_rhw) {
            /* This isn't a weekend's job to fix, ignore the problem for now. Needs a replacement
             * pipeline
             */
            todo_wine ok(abs(r - r_e) <= 1 && abs(g - g_e) <= 1 && abs(b - b_e) <= 1,
                         "Test %s returned color 0x%08x, expected 0x%08x(todo)\n",
                         tests[i].name, color, tests[i].color_rhw);
        } else {
            ok(abs(r - r_e) <= 1 && abs(g - g_e) <= 1 && abs(b - b_e) <= 1,
               "Test %s returned color 0x%08x, expected 0x%08x\n",
               tests[i].name, color, tests[i].color_rhw);
        }
    }

    for(i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        IDirect3DPixelShader9_Release(tests[i].shader);
    }

    IDirect3DVertexDeclaration9_Release(decl2);
    IDirect3DVertexDeclaration9_Release(decl);
}

static void vshader_version_varying_test(IDirect3DDevice9 *device) {
    static const DWORD ps_code[] = {
    0xffff0300,                                                             /* ps_3_0                       */
    0x05000030, 0xf00f0000, 0x00000003, 0x00000003, 0x00000001, 0x00000000, /* defi i0, 3, 3, 1, 0          */
    0x05000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.0, 0.0, 0.0, 0.0   */
    0x0200001f, 0x8001000a, 0x900f0003,                                     /* dcl_color1 v3                */
    0x0200001f, 0x8000000b, 0x900f0004,                                     /* dcl_fog v4                   */
    0x0200001f, 0x80030005, 0x900f0005,                                     /* dcl_texcoord3 v5             */
    0x0200001f, 0x80000003, 0x900f0006,
    0x0200001f, 0x80000006, 0x900f0007,
    0x0200001f, 0x80000001, 0x900f0008,
    0x0200001f, 0x8000000c, 0x900f0009,

    0x02000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                   */
    0x0200001b, 0xf0e40800, 0xf0e40000,                                     /* loop aL, i0                  */
    0x04000002, 0x800f0000, 0x80e40000, 0x90e42000, 0xf0e40800,             /* add r0, r0, v0[aL]           */
    0x0000001d,                                                             /* endloop                      */
    0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                  */
    0x0000ffff                                                              /* end                          */
    };
    static const DWORD vs_1_code[] = {
    0xfffe0101,                                                             /* vs_1_1                       */
    0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
    0x00000051, 0xa00f0000, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.1, 0.0, 0.0, 0.0   */
    0x00000051, 0xa00f0001, 0x00000000, 0x3e4ccccd, 0x00000000, 0x00000000, /* def c1, 0.0, 0.2, 0.0, 0.0   */
    0x00000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3ecccccd, 0x00000000, /* def c2, 0.0, 0.0, 0.4, 0.0   */
    0x00000051, 0xa00f0003, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 1.0, 1.0, 1.0, 1.0   */
    0x00000001, 0xd00f0000, 0xa0e40002,                                     /* mov oD0, c2                  */
    0x00000001, 0xd00f0001, 0xa0e40000,                                     /* mov oD1, c0                  */
    0x00000001, 0xc00f0001, 0xa0550001,                                     /* mov oFog, c1.g               */
    0x00000001, 0xe00f0000, 0xa0e40003,                                     /* mov oT0, c3                  */
    0x00000001, 0xe00f0001, 0xa0e40003,                                     /* mov oT1, c3                  */
    0x00000001, 0xe00f0002, 0xa0e40003,                                     /* mov oT2, c3                  */
    0x00000001, 0xe00f0003, 0xa0e40002,                                     /* mov oT3, c2                  */
    0x00000001, 0xe00f0004, 0xa0e40003,                                     /* mov oT4, c3                  */
    0x00000001, 0xe00f0005, 0xa0e40003,                                     /* mov oT5, c3                  */
    0x00000001, 0xe00f0006, 0xa0e40003,                                     /* mov oT6, c3                  */
    0x00000001, 0xe00f0007, 0xa0e40003,                                     /* mov oT7, c3                  */
    0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
    0x0000ffff
    };
    DWORD vs_2_code[] = {
    0xfffe0200,                                                             /* vs_2_0                       */
    0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
    0x05000051, 0xa00f0000, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.5, 0.0, 0.0, 0.0   */
    0x05000051, 0xa00f0001, 0x00000000, 0x3e4ccccd, 0x00000000, 0x00000000, /* def c1, 0.0, 0.5, 0.0, 0.0   */
    0x05000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3ecccccd, 0x00000000, /* def c2, 0.0, 0.0, 0.5, 0.0   */
    0x05000051, 0xa00f0003, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 1.0, 1.0, 1.0, 1.0   */
    0x02000001, 0xd00f0000, 0xa0e40002,                                     /* mov oD0, c2                  */
    0x02000001, 0xd00f0001, 0xa0e40000,                                     /* mov oD1, c0                  */
    0x02000001, 0xc00f0001, 0xa0550001,                                     /* mov oFog, c1.g               */
    0x02000001, 0xe00f0000, 0xa0e40003,                                     /* mov oT0, c3                  */
    0x02000001, 0xe00f0001, 0xa0e40003,                                     /* mov oT1, c3                  */
    0x02000001, 0xe00f0002, 0xa0e40003,                                     /* mov oT2, c3                  */
    0x02000001, 0xe00f0003, 0xa0e40002,                                     /* mov oT3, c2                  */
    0x02000001, 0xe00f0004, 0xa0e40003,                                     /* mov oT4, c3                  */
    0x02000001, 0xe00f0005, 0xa0e40003,                                     /* mov oT5, c3                  */
    0x02000001, 0xe00f0006, 0xa0e40003,                                     /* mov oT6, c3                  */
    0x02000001, 0xe00f0007, 0xa0e40003,                                     /* mov oT7, c3                  */
    0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
    0x0000ffff                                                              /* end                          */
    };
    /* TODO: Define normal, tangent, blendweight and depth here */
    static const DWORD vs_3_code[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
    0x0200001f, 0x8001000a, 0xe00f0009,                                     /* dcl_color1 o9                */
    0x0200001f, 0x8000000b, 0xe00f0002,                                     /* dcl_fog o2                   */
    0x0200001f, 0x80030005, 0xe00f0005,                                     /* dcl_texcoord3 o5             */
    0x0200001f, 0x80000000, 0xe00f000b,                                     /* dcl_position o11             */
    0x05000051, 0xa00f0000, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.1, 0.0, 0.0, 0.0   */
    0x05000051, 0xa00f0001, 0x00000000, 0x3e4ccccd, 0x00000000, 0x00000000, /* def c1, 0.0, 0.2, 0.0, 0.0   */
    0x05000051, 0xa00f0002, 0x00000000, 0x00000000, 0x3ecccccd, 0x00000000, /* def c2, 0.0, 0.0, 0.4, 0.0   */
    0x05000051, 0xa00f0003, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 1.0, 1.0, 1.0, 1.0   */
    0x02000001, 0xe00f0009, 0xa0e40000,                                     /* mov o9, c0                   */
    0x02000001, 0xe00f0002, 0xa0e40001,                                     /* mov o2, c1                   */
    0x02000001, 0xe00f0005, 0xa0e40002,                                     /* mov o5, c2                   */
    0x02000001, 0xe00f000b, 0x90e40000,                                     /* mov o11, v0                  */
    0x0000ffff                                                              /* end                          */
    };
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

    HRESULT hr;
    DWORD color;
    IDirect3DPixelShader9 *pixelshader = NULL;
    IDirect3DVertexShader9 *vs_1_shader = NULL;
    IDirect3DVertexShader9 *vs_2_shader = NULL;
    IDirect3DVertexShader9 *vs_3_shader = NULL;

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff110000, 0.0, 0);

    hr = IDirect3DDevice9_CreatePixelShader(device, ps_code, &pixelshader);
    ok(hr == D3D_OK, "IDirect3DDevice_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_code, &vs_1_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, vs_2_code, &vs_2_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_code, &vs_3_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, pixelshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetVertexShader(device, vs_1_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, vs_2_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,  quad2, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, vs_3_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 120);
    ok((color & 0x00ff0000) >= 0x00190000 && (color & 0x00ff0000) <= 0x00210000 &&
       (color & 0x0000ff00) >= 0x00003300 && (color & 0x0000ff00) <= 0x00003500 &&
       (color & 0x000000ff) >= 0x00000066 && (color & 0x000000ff) <= 0x00210068,
       "vs_3_0 returned color 0x%08x, expected 0x00203366\n", color);
    color = getPixelColor(device, 160, 360);
    ok((color & 0x00ff0000) >= 0x003c0000 && (color & 0x00ff0000) <= 0x004e0000 &&
       (color & 0x0000ff00) >= 0x00000000 && (color & 0x0000ff00) <= 0x00000000 &&
       (color & 0x000000ff) >= 0x00000066 && (color & 0x000000ff) <= 0x00210068,
       "vs_1_1 returned color 0x%08x, expected 0x00808080\n", color);
    color = getPixelColor(device, 480, 360);
    ok((color & 0x00ff0000) >= 0x003c0000 && (color & 0x00ff0000) <= 0x004e0000 &&
       (color & 0x0000ff00) >= 0x00000000 && (color & 0x0000ff00) <= 0x00000000 &&
       (color & 0x000000ff) >= 0x00000066 && (color & 0x000000ff) <= 0x00210068,
       "vs_2_0 returned color 0x%08x, expected 0x00000000\n", color);

    /* cleanup */
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    if(pixelshader) IDirect3DPixelShader9_Release(pixelshader);
    if(vs_1_shader) IDirect3DVertexShader9_Release(vs_1_shader);
    if(vs_2_shader) IDirect3DVertexShader9_Release(vs_2_shader);
    if(vs_3_shader) IDirect3DVertexShader9_Release(vs_3_shader);
}

static void pshader_version_varying_test(IDirect3DDevice9 *device) {
    static const DWORD vs_code[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
    0x0200001f, 0x80000000, 0xe00f0000,                                     /* dcl_position o0              */
    0x0200001f, 0x8000000a, 0xe00f0001,                                     /* dcl_color0 o1                */
    0x0200001f, 0x80000005, 0xe00f0002,                                     /* dcl_texcoord0 o2             */
    0x0200001f, 0x8000000b, 0xe00f0003,                                     /* dcl_fog o3                   */
    0x0200001f, 0x80000003, 0xe00f0004,                                     /* dcl_normal o4                */
    0x0200001f, 0x8000000c, 0xe00f0005,                                     /* dcl_depth o5                 */
    0x0200001f, 0x80000006, 0xe00f0006,                                     /* dcl_tangent o6               */
    0x0200001f, 0x80000001, 0xe00f0007,                                     /* dcl_blendweight o7           */
    0x05000051, 0xa00f0001, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, /* def c1, 0.1, 0.0, 0.0, 0.0   */
    0x05000051, 0xa00f0002, 0x00000000, 0x3e4ccccd, 0x00000000, 0x3f800000, /* def c2, 0.0, 0.2, 0.0, 1.0   */
    0x05000051, 0xa00f0003, 0x3ecccccd, 0x3f59999a, 0x3f666666, 0x00000000, /* def c3, 0.4, 0.85,0.9, 0.0   */
    0x05000051, 0xa00f0000, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c0, 1.0, 1.0, 1.0, 1.0   */

    0x02000001, 0xe00f0000, 0x90e40000,                                     /* mov o0, v0                   */
    0x02000001, 0xe00f0001, 0xa0e40001,                                     /* mov o1, c1                   */
    0x02000001, 0xe00f0002, 0xa0e40002,                                     /* mov o2, c2                   */
    0x02000001, 0xe00f0003, 0xa0e40003,                                     /* mov o3, c3                   */
    0x02000001, 0xe00f0004, 0xa0e40000,                                     /* mov o4, c0                   */
    0x02000001, 0xe00f0005, 0xa0e40000,                                     /* mov o5, c0                   */
    0x02000001, 0xe00f0006, 0xa0e40000,                                     /* mov o6, c0                   */
    0x02000001, 0xe00f0007, 0xa0e40000,                                     /* mov o7, c0                   */
    0x0000ffff                                                              /* end                          */
    };
    static const DWORD ps_1_code[] = {
    0xffff0104,                                                             /* ps_1_4                       */
    0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.0, 0.0, 0.0, 0.0   */
    0x00000040, 0x80070001, 0xb0e40000,                                     /* texcrd r1.xyz, t0            */
    0x00000001, 0x80080001, 0xa0ff0000,                                     /* mov r1.a, c0.a               */
    0x00000002, 0x800f0000, 0x90e40000, 0x80e40001,                         /* add r0, v0, r1               */
    0x0000ffff                                                              /* end                          */
    };
    static const DWORD ps_2_code[] = {
    0xffff0200,                                                             /* ps_2_0                       */
    0x0200001f, 0x80000000, 0xb00f0000,                                     /* dcl t0                       */
    0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl v0                       */
    0x0200001f, 0x80000000, 0x900f0001,                                     /* dcl v1                       */

    0x02000001, 0x800f0000, 0x90e40000,                                     /* mov r0, v0                   */
    0x03000002, 0x800f0000, 0x80e40000,0xb0e40000,                          /* add r0, r0, t0               */
    0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                  */
    0x0000ffff                                                              /* end                          */
    };
    static const DWORD ps_3_code[] = {
    0xffff0300,                                                             /* ps_3_0                       */
    0x0200001f, 0x80000005, 0x900f0000,                                     /* dcl_texcoord0 v0             */
    0x0200001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                */
    0x0200001f, 0x8000000b, 0x900f0002,                                     /* dcl_fog v2                   */

    0x02000001, 0x800f0000, 0x90e40000,                                     /* mov r0, v0                   */
    0x03000002, 0x800f0000, 0x80e40000, 0x90e40001,                         /* add r0, r0, v1               */
    0x03000002, 0x800f0000, 0x80e40000, 0x90e40002,                         /* mov r0, r0, v2               */
    0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                  */
    0x0000ffff                                                              /* end                          */
    };

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

    HRESULT hr;
    DWORD color;
    IDirect3DVertexShader9 *vertexshader = NULL;
    IDirect3DPixelShader9 *ps_1_shader = NULL;
    IDirect3DPixelShader9 *ps_2_shader = NULL;
    IDirect3DPixelShader9 *ps_3_shader = NULL;
    IDirect3DTexture9 *texture = NULL;
    D3DLOCKED_RECT lr;
    unsigned int x, y;

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);

    hr = IDirect3DDevice9_CreateTexture(device, 512,  512, 1, 0, D3DFMT_A16B16G16R16, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %s\n", DXGetErrorString9(hr));
    for(y = 0; y < 512; y++) {
        for(x = 0; x < 512; x++) {
            double r_f = (double) x / (double) 512;
            double g_f = (double) y / (double) 512;
            unsigned short *dst = (unsigned short *) (((unsigned char *) lr.pBits) + y * lr.Pitch + x * 8);
            unsigned short r = (unsigned short) (r_f * 65535.0);
            unsigned short g = (unsigned short) (g_f * 65535.0);
            dst[0] = r;
            dst[1] = g;
            dst[2] = 0;
            dst[3] = 65535;
        }
    }
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexShader(device, vs_code, &vertexshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_1_code, &ps_1_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_2_code, &ps_2_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_3_code, &ps_3_shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShader(device, vertexshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetPixelShader(device, ps_1_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, ps_2_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,  quad2, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, ps_3_shader);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 120);
    ok((color & 0x00ff0000) >= 0x00790000 && (color & 0x00ff0000) <= 0x00810000 &&
       (color & 0x0000ff00) == 0x0000ff00 &&
       (color & 0x000000ff) >= 0x000000e4 && (color & 0x000000ff) <= 0x000000e6,
       "ps_3_0 returned color 0x%08x, expected 0x0080ffe5\n", color);
    color = getPixelColor(device, 160, 360);
    ok((color & 0x00ff0000) >= 0x00190000 && (color & 0x00ff0000) <= 0x00210000 &&
       (color & 0x0000ff00) >= 0x00003300 && (color & 0x0000ff00) <= 0x00003400 &&
       (color & 0x000000ff) == 0x00000000,
       "ps_1_4 returned color 0x%08x, expected 0x00203300\n", color);
    color = getPixelColor(device, 480, 360);
    ok((color & 0x00ff0000) >= 0x00190000 && (color & 0x00ff0000) <= 0x00210000 &&
       (color & 0x0000ff00) >= 0x00003200 && (color & 0x0000ff00) <= 0x00003400 &&
       (color & 0x000000ff) == 0x00000000,
       "ps_2_0 returned color 0x%08x, expected 0x00203300\n", color);
    color = getPixelColor(device, 480, 160);
    ok((color & 0x00ff0000) >= 0x00190000 && (color & 0x00ff0000) <= 0x00210000 &&
       (color & 0x0000ff00) >= 0x00003200 && (color & 0x0000ff00) <= 0x00003400 &&
       (color & 0x000000ff) == 0x00000000,
       "fixed function fragment processing returned color 0x%08x, expected 0x00203300\n", color);

    /* cleanup */
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    if(vertexshader) IDirect3DVertexShader9_Release(vertexshader);
    if(ps_1_shader) IDirect3DPixelShader9_Release(ps_1_shader);
    if(ps_2_shader) IDirect3DPixelShader9_Release(ps_2_shader);
    if(ps_3_shader) IDirect3DPixelShader9_Release(ps_3_shader);
    if(texture) IDirect3DTexture9_Release(texture);
}

void test_compare_instructions(IDirect3DDevice9 *device)
{
    DWORD shader_sge_vec_code[] = {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000d, 0xd00f0000, 0x80e40000, 0xa0e40001,     /* sge oD0, r0, c1          */
        0x0000ffff                                          /* end                      */
    };
    DWORD shader_slt_vec_code[] = {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000c, 0xd00f0000, 0x80e40000, 0xa0e40001,     /* slt oD0, r0, c1          */
        0x0000ffff                                          /* end                      */
    };
    DWORD shader_sge_scalar_code[] = {
        0xfffe0101,                                         /* vs_1_1                   */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0          */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0             */
        0x00000001, 0x800f0000, 0xa0e40000,                 /* mov r0, c0               */
        0x0000000d, 0xd0010000, 0x80000000, 0xa0550001,     /* slt oD0.r, r0.r, c1.b    */
        0x0000000d, 0xd0020000, 0x80550000, 0xa0aa0001,     /* slt oD0.g, r0.g, c1.r    */
        0x0000000d, 0xd0040000, 0x80aa0000, 0xa0000001,     /* slt oD0.b, r0.b, c1.g    */
        0x0000ffff                                          /* end                      */
    };
    DWORD shader_slt_scalar_code[] = {
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

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_sge_vec_code, &shader_sge_vec);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_slt_vec_code, &shader_slt_vec);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_sge_scalar_code, &shader_sge_scalar);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_slt_scalar_code, &shader_slt_scalar);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, const0, 1);
    ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, const1, 1);
    ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetFVF failed (%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_SetVertexShader(device, shader_sge_vec);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_slt_vec);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2,  quad2, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_sge_scalar);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, const0, 1);
        ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);

        hr = IDirect3DDevice9_SetVertexShader(device, shader_slt_scalar);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF00FF, "Compare test: Quad 1(sge vec) returned color 0x%08x, expected 0x00FF00FF\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000FF00, "Compare test: Quad 2(slt vec) returned color 0x%08x, expected 0x0000FF00\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x00FFFFFF, "Compare test: Quad 3(sge scalar) returned color 0x%08x, expected 0x00FFFFFF\n", color);
    color = getPixelColor(device, 480, 160);
    ok(color == 0x000000ff, "Compare test: Quad 4(slt scalar) returned color 0x%08x, expected 0x000000ff\n", color);

    IDirect3DVertexShader9_Release(shader_sge_vec);
    IDirect3DVertexShader9_Release(shader_slt_vec);
    IDirect3DVertexShader9_Release(shader_sge_scalar);
    IDirect3DVertexShader9_Release(shader_slt_scalar);
}

void test_vshader_input(IDirect3DDevice9 *device)
{
    DWORD swapped_shader_code_3[] = {
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
    DWORD swapped_shader_code_1[] = {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord0 v1     */
        0x0000001f, 0x80010005, 0x900f0002,                 /* dcl_texcoord1 v2     */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x00000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x00000002, 0xd00f0000, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    DWORD swapped_shader_code_2[] = {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord0 v1     */
        0x0200001f, 0x80010005, 0x900f0002,                 /* dcl_texcoord1 v2     */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x02000001, 0x800f0001, 0x90e40001,                 /* mov r1, v1           */
        0x03000002, 0xd00f0000, 0x80e40001, 0x91e40002,     /* sub o1, r1, v2       */
        0x0000ffff                                          /* end                  */
    };
    DWORD texcoord_color_shader_code_3[] = {
        0xfffe0300,                                         /* vs_3_0               */
        0x0200001f, 0x80000000, 0xe00f0000,                 /* dcl_position o0      */
        0x0200001f, 0x8000000a, 0xe00f0001,                 /* dcl_color o1         */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x02000001, 0xe00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x02000001, 0xe00f0001, 0x90e40001,                 /* mov o1, v1           */
        0x0000ffff                                          /* end                  */
    };
    DWORD texcoord_color_shader_code_2[] = {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x02000001, 0xd00f0000, 0x90e40001,                 /* mov oD0, v1          */
        0x0000ffff                                          /* end                  */
    };
    DWORD texcoord_color_shader_code_1[] = {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x80000005, 0x900f0001,                 /* dcl_texcoord v1      */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000001, 0xd00f0000, 0x90e40001,                 /* mov oD0, v1          */
        0x0000ffff                                          /* end                  */
    };
    DWORD color_color_shader_code_3[] = {
        0xfffe0300,                                         /* vs_3_0               */
        0x0200001f, 0x80000000, 0xe00f0000,                 /* dcl_position o0      */
        0x0200001f, 0x8000000a, 0xe00f0001,                 /* dcl_color o1         */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x02000001, 0xe00f0000, 0x90e40000,                 /* mov o0, v0           */
        0x03000005, 0xe00f0001, 0xa0e40000, 0x90e40001,     /* mul o1, c0, v1       */
        0x0000ffff                                          /* end                  */
    };
    DWORD color_color_shader_code_2[] = {
        0xfffe0200,                                         /* vs_2_0               */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0200001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x03000005, 0xd00f0000, 0xa0e40000, 0x90e40001,     /* mul oD0, c0, v1       */
        0x0000ffff                                          /* end                  */
    };
    DWORD color_color_shader_code_1[] = {
        0xfffe0101,                                         /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,                 /* dcl_position v0      */
        0x0000001f, 0x8000000a, 0x900f0001,                 /* dcl_color v1         */
        0x00000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0         */
        0x00000005, 0xd00f0000, 0xa0e40000, 0x90e40001,     /* mul oD0, c0, v1       */
        0x0000ffff                                          /* end                  */
    };
    IDirect3DVertexShader9 *swapped_shader, *texcoord_color_shader, *color_color_shader;
    HRESULT hr;
    DWORD color, r, g, b;
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
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_onetexcrd, &decl_onetexcrd);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_twotexcrd_wrongidx, &decl_twotex_wrongidx);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_twotexcrd_rightorder, &decl_twotexcrd_rightorder);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_texcoord_color, &decl_texcoord_color);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_color, &decl_color_color);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_ubyte, &decl_color_ubyte);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_color_float, &decl_color_float);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexDeclaration returned %s\n", DXGetErrorString9(hr));

    for(i = 1; i <= 3; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
        if(i == 3) {
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_3, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        } else if(i == 2){
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_2, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        } else if(i == 1) {
            hr = IDirect3DDevice9_CreateVertexShader(device, swapped_shader_code_1, &swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        }

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetVertexShader(device, swapped_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_onetexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                /* Succeeds or fails, depending on SW or HW vertex processing */
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x), i = 1\n", hr);
            }

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd_rightorder);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotex_wrongidx);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = 1\n", hr);
            }

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
        }

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

        if(i == 3 || i == 2) {
            color = getPixelColor(device, 160, 360);
            ok(color == 0x00FFFF80 || color == 0x00FFFF7f || color == 0x00FFFF81,
               "Input test: Quad 1(2crd) returned color 0x%08x, expected 0x00FFFF80\n", color);

            /* The last value of the read but undefined stream is used */
            color = getPixelColor(device, 480, 360);
            ok(color == 0x00FFFF00, "Input test: Quad 2(1crd) returned color 0x%08x, expected 0x00FFFF00\n", color);
            color = getPixelColor(device, 160, 120);
            ok(color == 0x00FF0080 || color == 0x00FF007f || color == 0x00FF0081,
               "Input test: Quad 3(2crd-wrongidx) returned color 0x%08x, expected 0x00FF0080\n", color);

            color = getPixelColor(device, 480, 160);
            ok(color == 0x00000000, "Input test: Quad 4(2crd-rightorder) returned color 0x%08x, expected 0x00000000\n", color);
        } else if(i == 1) {
            color = getPixelColor(device, 160, 360);
            ok(color == 0x00FFFF80 || color == 0x00FFFF7f || color == 0x00FFFF81,
               "Input test: Quad 1(2crd) returned color 0x%08x, expected 0x00FFFF80\n", color);
            color = getPixelColor(device, 480, 360);
            /* Accept the clear color as well in this case, since SW VP returns an error */
            ok(color == 0x00FFFF00 || color == 0x00FF0000, "Input test: Quad 2(1crd) returned color 0x%08x, expected 0x00FFFF00\n", color);
            color = getPixelColor(device, 160, 120);
            ok(color == 0x00FF0080 || color == 0x00FF0000 || color == 0x00FF007f || color == 0x00FF0081,
               "Input test: Quad 3(2crd-wrongidx) returned color 0x%08x, expected 0x00FF0080\n", color);
            color = getPixelColor(device, 480, 160);
            ok(color == 0x00000000, "Input test: Quad 4(2crd-rightorder) returned color 0x%08x, expected 0x00000000\n", color);
        }

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff808080, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

        /* Now find out if the whole streams are re-read, or just the last active value for the
         * vertices is used.
         */
        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
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
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_twotexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 3, quad1_modified, sizeof(float) * 11);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_onetexcrd);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2_modified, sizeof(float) * 11);
            if(i == 3 || i == 2) {
                ok(hr == D3D_OK, "DrawPrimitiveUP returned (%08x) i = %d\n", hr, i);
            } else if(i == 1) {
                /* Succeeds or fails, depending on SW or HW vertex processing */
                ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "DrawPrimitiveUP returned (%08x), i = 1\n", hr);
            }

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
        }
        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

        color = getPixelColor(device, 480, 360);
        /* vs_1_1 may fail, accept the clear color
         *
         * NOTE: This test fails on the reference rasterizer. In the refrast, the 4 vertices have different colors,
         * i.e., the whole old stream is read, and not just the last used attribute. Some games require that this
         * does *not* happen, otherwise they can crash because of a read from a bad pointer, so do not accept the
         * refrast's result.
         *
         * A test app for this behavior is Half Life 2 Episode 2 in dxlevel 95, and related games(Portal, TF2).
         */
        ok(color == 0x000000FF || color == 0x00808080,
           "Input test: Quad 2(different colors) returned color 0x%08x, expected 0x000000FF\n", color);
        color = getPixelColor(device, 160, 120);

        IDirect3DDevice9_SetVertexShader(device, NULL);
        IDirect3DDevice9_SetVertexDeclaration(device, NULL);

        IDirect3DVertexShader9_Release(swapped_shader);
    }

    for(i = 1; i <= 3; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
        if(i == 3) {
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_3, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_3, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        } else if(i == 2){
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_2, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_2, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        } else if(i == 1) {
            hr = IDirect3DDevice9_CreateVertexShader(device, texcoord_color_shader_code_1, &texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_CreateVertexShader(device, color_color_shader_code_1, &color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
        }

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
        if(SUCCEEDED(hr))
        {
            hr = IDirect3DDevice9_SetVertexShader(device, texcoord_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_texcoord_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad1_color, sizeof(quad1_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexShader(device, color_color_shader);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));

            hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, normalize, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_ubyte);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad2_color, sizeof(quad2_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, no_normalize, 1);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad3_color, sizeof(quad3_color[0]));
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, decl_color_float);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration returned %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad4_color, sizeof(float) * 7);
            ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
        }
        IDirect3DDevice9_SetVertexShader(device, NULL);
        IDirect3DDevice9_SetVertexDeclaration(device, NULL);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));

        color = getPixelColor(device, 160, 360);
        r = (color & 0x00ff0000) >> 16;
        g = (color & 0x0000ff00) >>  8;
        b = (color & 0x000000ff) >>  0;
        ok(r >= 0xfe && r <= 0xff && g >= 0x7f && g <= 0x81 && b >= 0x3f && b <= 0x41,
           "Input test: Quad 1(color-texcoord) returned color 0x%08x, expected 0x00ff8040\n", color);
        color = getPixelColor(device, 480, 360);
        r = (color & 0x00ff0000) >> 16;
        g = (color & 0x0000ff00) >>  8;
        b = (color & 0x000000ff) >>  0;
        ok(r >= 0x3f && r <= 0x41 && g >= 0x7f && g <= 0x81 && b >= 0xfe && b <= 0xff,
           "Input test: Quad 2(color-ubyte) returned color 0x%08x, expected 0x004080ff\n", color);
        color = getPixelColor(device, 160, 120);
        r = (color & 0x00ff0000) >> 16;
        g = (color & 0x0000ff00) >>  8;
        b = (color & 0x000000ff) >>  0;
        ok(r >= 0xfe && r <= 0xff && g >= 0x7f && g <= 0x81 && b >= 0x3f && b <= 0x41,
           "Input test: Quad 3(color-color) returned color 0x%08x, expected 0x00ff8040\n", color);
        color = getPixelColor(device, 480, 160);
        r = (color & 0x00ff0000) >> 16;
        g = (color & 0x0000ff00) >>  8;
        b = (color & 0x000000ff) >>  0;
        ok(r >= 0xfe && r <= 0xff && g >= 0xfe && g <= 0xff && b <= 0x01,
           "Input test: Quad 4(color-float) returned color 0x%08x, expected 0x00FFFF00\n", color);

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
}

static void fog_srgbwrite_test(IDirect3DDevice9 *device)
{
    /* Draw a black quad, half fogged with white fog -> grey color. Enable sRGB writing.
     * if sRGB writing is applied before fogging, the 0.0 will be multiplied with ~ 12.92, so still
     * stay 0.0. After that the fog gives 0.5. If sRGB writing is applied after fogging, the
     * 0.5 will run through the alternative path(0^5 ^ 0.41666 * 1.055 - 0.055), resulting in approx.
     * 0.73
     *
     * At the time of this writing, wined3d could not apply sRGB correction to fixed function rendering,
     * so use shaders for this task
     */
    IDirect3DPixelShader9 *pshader;
    IDirect3DVertexShader9 *vshader;
    IDirect3D9 *d3d;
    DWORD vshader_code[] = {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x00000051, 0xa00f0000, 0x3f000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.5, 0.0, 0.0, 0.0   */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x00000001, 0xc00f0001, 0xa0000000,                                     /* mov oFog, c0.x               */
        0x0000ffff                                                              /* end                          */
    };
    DWORD pshader_code[] = {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, /* def c0, 0.0, 0.0, 0.0, 0.0   */
        0x00000001, 0x800f0000, 0xa0e40000,                                     /* mov r0, c0                   */
        0x0000ffff                                                              /* end                          */
    };
    const float quad[] = {
       -1.0,   -1.0,    0.1,
        1.0,   -1.0,    0.1,
       -1.0,    1.0,    0.1,
        1.0,    1.0,    0.1
    };
    HRESULT hr;
    DWORD color;

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    if(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                                    D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_SRGBWRITE,
                                    D3DRTYPE_SURFACE, D3DFMT_A8R8G8B8) != D3D_OK) {
        skip("No SRGBWRITEENABLE support on D3DFMT_X8R8G8B8\n");
        IDirect3D9_Release(d3d);
        return;
    }
    IDirect3D9_Release(d3d);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xffffffff);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRGBWRITEENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexShader(device, vshader_code, &vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreatePixelShader(device, pshader_code, &pshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexShader(device, vshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, pshader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene returned %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 3);
        ok(hr == D3D_OK, "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene returned %s\n", DXGetErrorString9(hr));
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShader returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader returned %s\n", DXGetErrorString9(hr));
    IDirect3DPixelShader9_Release(pshader);
    IDirect3DVertexShader9_Release(vshader);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SRGBWRITEENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_Present failed with %s\n", DXGetErrorString9(hr));
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00808080 || color == 0x007f7f7f || color == 0x00818181,
       "Fog with D3DRS_SRGBWRITEENABLE returned color 0x%08x, expected 0x00808080\n", color);
}

static void alpha_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *offscreenTexture;
    IDirect3DSurface9 *backbuffer = NULL, *offscreen = NULL;
    DWORD color, red, green, blue;

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

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &offscreenTexture, NULL);
    ok(hr == D3D_OK || D3DERR_INVALIDCALL, "Creating the offscreen render target failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %s\n", DXGetErrorString9(hr));
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DTexture9_GetSurfaceLevel(offscreenTexture, 0, &offscreen);
    ok(hr == D3D_OK, "Can't get offscreen surface, hr = %s\n", DXGetErrorString9(hr));
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed, hr = %#08x\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed, hr = %08x\n", hr);
    if(IDirect3DDevice9_BeginScene(device) == D3D_OK) {

        /* Draw two quads, one with src alpha blending, one with dest alpha blending. The
         * SRCALPHA / INVSRCALPHA blend doesn't give any surprises. Colors are blended based on
         * the input alpha
         *
         * The DESTALPHA / INVDESTALPHA do not "work" on the regular buffer because there is no alpha.
         * They give essentially ZERO and ONE blend factors
         */
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

        /* Switch to the offscreen buffer, and redo the testing. SRCALPHA and DESTALPHA. The offscreen buffer
         * has a alpha channel on its own. Clear the offscreen buffer with alpha = 0.5 again, then draw the
         * quads again. The SRCALPHA/INVSRCALPHA doesn't give any surprises, but the DESTALPHA/INVDESTALPHA
         * blending works as supposed now - blend factor is 0.5 in both cases, not 0.75 as from the input
         * vertices
         */
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

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0xbe && red <= 0xc0 && green >= 0x39 && green <= 0x41 && blue == 0x00,
       "SRCALPHA on frame buffer returned color %08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 160, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0x00 && green == 0x00 && blue >= 0xfe && blue <= 0xff ,
       "DSTALPHA on frame buffer returned color %08x, expected 0x00ff0000\n", color);

    color = getPixelColor(device, 480, 360);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0xbe && red <= 0xc0 && green >= 0x39 && green <= 0x41 && blue == 0x00,
       "SRCALPHA on texture returned color %08x, expected bar\n", color);

    color = getPixelColor(device, 480, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x81 && green == 0x00 && blue >= 0x7e && blue <= 0x81,
       "DSTALPHA on texture returned color %08x, expected foo\n", color);

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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_d3dcolor, &dcl_color);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_ubyte4n, &dcl_ubyte);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_short4, &dcl_short);
    ok(SUCCEEDED(hr) || hr == E_FAIL, "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_float, &dcl_float);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_ubyte4n_2streams, &dcl_ubyte_2);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_d3dcolor_2streams, &dcl_color_2);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements_positiont, &dcl_positiont);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);

    size = max(sizeof(quad1), max(sizeof(quad2), max(sizeof(quad3), max(sizeof(quad4), sizeof(quads)))));
    hr = IDirect3DDevice9_CreateVertexBuffer(device, size,
                                             0, 0, D3DPOOL_MANAGED, &vb, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %s\n", DXGetErrorString9(hr));

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

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad1), (void **) &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
            memcpy(data, quad1, sizeof(quad1));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_color);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad1[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
        }

        if(dcl_ubyte) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad2), (void **) &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
            memcpy(data, quad2, sizeof(quad2));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad2[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ub_ok = SUCCEEDED(hr);
        }

        if(dcl_short) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad3), (void **) &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
            memcpy(data, quad3, sizeof(quad3));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_short);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad3[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            s_ok = SUCCEEDED(hr);
        }

        if(dcl_float) {
            hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad4), (void **) &data, 0);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
            memcpy(data, quad4, sizeof(quad4));
            hr = IDirect3DVertexBuffer9_Unlock(vb);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_float);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);
            hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad4[0]));
            ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            f_ok = SUCCEEDED(hr);
        }

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr = %#08x\n", hr);
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed (%08x)\n", hr);

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad_transformed), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
    memcpy(data, quad_transformed, sizeof(quad_transformed));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_positiont);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(quad_transformed[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    /* This test is pointless without those two declarations: */
    if(!dcl_color_2 || !dcl_ubyte_2) goto out;

    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quads), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
    memcpy(data, quads, sizeof(quads));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(colors),
                                             0, 0, D3DPOOL_MANAGED, &vb2, NULL);
    ok(hr == D3D_OK, "CreateVertexBuffer failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DVertexBuffer9_Lock(vb2, 0, sizeof(colors), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed with %s\n", DXGetErrorString9(hr));
    memcpy(data, colors, sizeof(colors));
    hr = IDirect3DVertexBuffer9_Unlock(vb2);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed (%08x)\n", hr);

    for(i = 0; i < 2; i++) {
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
        if(i == 0) {
            hr = IDirect3DDevice9_SetStreamSource(device, 1, vb2, 0, sizeof(DWORD) * 4);
        } else {
            hr = IDirect3DDevice9_SetStreamSource(device, 1, vb2, 8, sizeof(DWORD) * 4);
        }
        ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
        ub_ok = FALSE;
        if(SUCCEEDED(hr)) {
            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ub_ok = SUCCEEDED(hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_color_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 4, 2);
            ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);

            hr = IDirect3DDevice9_SetVertexDeclaration(device, dcl_ubyte_2);
            ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed with %s\n", DXGetErrorString9(hr));
            hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 8, 2);
            ok(hr == D3D_OK || hr == D3DERR_INVALIDCALL,
               "IDirect3DDevice9_DrawPrimitive failed, hr = %#08x\n", hr);
            ok(ub_ok == SUCCEEDED(hr), "dcl_ubyte_2 returned two different results\n");

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
        }

        IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetStreamSource(device, 1, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed with %s\n", DXGetErrorString9(hr));
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
    DWORD shader_code[] = {
        0xfffe0101, 0x0000001f, 0x80000000, 0x900f0000, 0x0000001f, 0x8000000a,
        0x900f0001, 0x00000001, 0xc00f0000, 0x90e40000, 0x00000001, 0xd00f0000,
        0x90e40001, 0x0000ffff
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
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vdecl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexDeclaration failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexShader failed hr=%s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_SetVertexDeclaration(device, vdecl);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  0, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  4, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad +  8, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad + 12, sizeof(quad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%s\n", DXGetErrorString9(hr));
    }
    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff102030, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, 0,
                                             D3DPOOL_MANAGED, &buffer, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexBuffer failed, hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(quad), (void **) &data, 0);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Lock failed, hr=%s\n", DXGetErrorString9(hr));
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed, hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(quad[0]));
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed, hr=%s\n", DXGetErrorString9(hr));

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
            ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed, hr=%s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
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

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetStreamSource failed, hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed hr=%s\n", DXGetErrorString9(hr));

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
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed hr=%s\n", DXGetErrorString9(hr));
    if(!(caps.TextureCaps & D3DPTEXTURECAPS_POW2)) {
        /* NP2 conditional requires the POW2 flag. Check that while we're at it */
        ok((caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) == 0,
           "Card has conditional NP2 support without power of two restriction set\n");
        skip("Card has unconditional pow2 support, skipping conditional NP2 tests\n");
        return;
    } else if(!(caps.TextureCaps & D3DPTEXTURECAPS_POW2)) {
        skip("No conditional NP2 support, skipping conditional NP2 tests\n");
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_CreateTexture(device, 10, 10, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%s\n", DXGetErrorString9(hr));

    memset(&rect, 0, sizeof(rect));
    hr = IDirect3DTexture9_LockRect(texture, 0, &rect, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DTexture9_LockRect failed hr=%s\n", DXGetErrorString9(hr));
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
    ok(hr == D3D_OK, "IDirect3DTexture9_UnlockRect failed hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexDeclaration failed, hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

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

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed hr=%s\n", DXGetErrorString9(hr));
    IDirect3DTexture9_Release(texture);
}

static void vFace_register_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    const DWORD shader_code[] = {
        0xffff0300,                                                             /* ps_3_0                     */
        0x05000051, 0xa00f0000, 0x00000000, 0x3f800000, 0x00000000, 0x00000000, /* def c0, 0.0, 1.0, 0.0, 0.0 */
        0x05000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1, 1.0, 0.0, 0.0, 0.0 */
        0x0200001f, 0x80000000, 0x900f1001,                                     /* dcl vFace                  */
        0x02000001, 0x800f0001, 0xa0e40001,                                     /* mov r1, c1                 */
        0x04000058, 0x800f0000, 0x90e41001, 0xa0e40000, 0x80e40001,             /* cmp r0, vFace, c0, r1      */
        0x02000001, 0x800f0800, 0x80e40000,                                     /* mov oC0, r0                */
        0x0000ffff                                                              /* END                        */
    };
    IDirect3DPixelShader9 *shader;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface, *backbuffer;
    const float quad[] = {
        -1.0,   -1.0,   0.1,
         1.0,   -1.0,   0.1,
        -1.0,    0.0,   0.1,

         1.0,   -1.0,   0.1,
         1.0,    0.0,   0.1,
        -1.0,    0.0,   0.1,

        -1.0,    0.0,   0.1,
        -1.0,    1.0,   0.1,
         1.0,    0.0,   0.1,

         1.0,    0.0,   0.1,
        -1.0,    1.0,   0.1,
         1.0,    1.0,   0.1,
    };
    const float blit[] = {
         0.0,   -1.0,   0.1,    0.0,    0.0,
         1.0,   -1.0,   0.1,    1.0,    0.0,
         0.0,    1.0,   0.1,    0.0,    1.0,
         1.0,    1.0,   0.1,    1.0,    1.0,
    };

    hr = IDirect3DDevice9_CreatePixelShader(device, shader_code, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreatePixelShader failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetSurfaceLevel failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed hr=%s\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        /* First, draw to the texture and the back buffer to test both offscreen and onscreen cases */
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff0000ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 4, quad, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 4, quad, sizeof(float) * 3);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));

        /* Blit the texture ontp the back buffer to make it visible */
        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *) texture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState failed, hr=%s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed hr=%s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, blit, sizeof(float) * 5);
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawPrimitiveUP failed, hr=%s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed hr=%s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00ff0000, "vFace: Onscreen rendered front facing quad has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000ff00, "vFace: Onscreen rendered back facing quad has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000ff00, "vFace: Offscreen rendered back facing quad has color 0x%08x, expected 0x0000ff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00ff0000, "vFace: Offscreen rendered front facing quad has color 0x%08x, expected 0x00ff0000\n", color);

    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed hr=%s\n", DXGetErrorString9(hr));
    IDirect3DDevice9_SetTexture(device, 0, NULL);
    IDirect3DPixelShader9_Release(shader);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(backbuffer);
    IDirect3DTexture9_Release(texture);
}

START_TEST(visual)
{
    IDirect3DDevice9 *device_ptr;
    D3DCAPS9 caps;
    HRESULT hr;
    DWORD color;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr)
    {
        skip("Creating the device failed\n");
        return;
    }

    IDirect3DDevice9_GetDeviceCaps(device_ptr, &caps);

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 1, 1);
    if(color !=0x00ff0000)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 639, 479);
    if(color != 0x0000ddee)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now execute the real tests */
    lighting_test(device_ptr);
    clear_test(device_ptr);
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
    alpha_test(device_ptr);
    release_buffer_test(device_ptr);
    float_texture_test(device_ptr);
    g16r16_texture_test(device_ptr);
    texture_transform_flags_test(device_ptr);
    autogen_mipmap_test(device_ptr);
    fixed_function_decl_test(device_ptr);
    conditional_np2_repeat_test(device_ptr);

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        test_constant_clamp_vs(device_ptr);
        test_compare_instructions(device_ptr);
    }
    else skip("No vs_1_1 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0))
    {
        test_mova(device_ptr);
        if (caps.VertexShaderVersion >= D3DVS_VERSION(3, 0)) {
            test_vshader_input(device_ptr);
            test_vshader_float16(device_ptr);
        } else {
            skip("No vs_3_0 support\n");
        }
    }
    else skip("No vs_2_0 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1) && caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        fog_with_shader_test(device_ptr);
        fog_srgbwrite_test(device_ptr);
    }
    else skip("No vs_1_1 and ps_1_1 support\n");

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        texbem_test(device_ptr);
        texdepth_test(device_ptr);
        texkill_test(device_ptr);
        x8l8v8u8_test(device_ptr);
        if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 4)) {
            constant_clamp_ps_test(device_ptr);
            cnd_test(device_ptr);
            if (caps.PixelShaderVersion >= D3DPS_VERSION(3, 0)) {
                nested_loop_test(device_ptr);
                fixed_function_varying_test(device_ptr);
                vFace_register_test(device_ptr);
                if(caps.VertexShaderVersion >= D3DVS_VERSION(3, 0)) {
                    vshader_version_varying_test(device_ptr);
                    pshader_version_varying_test(device_ptr);
                } else {
                    skip("No vs_3_0 support\n");
                }
            } else {
                skip("No ps_3_0 support\n");
            }
        }
    }
    else skip("No ps_1_1 support\n");

cleanup:
    if(device_ptr) {
        ULONG ref;

        D3DPRESENT_PARAMETERS present_parameters;
        IDirect3DSwapChain9 *swapchain;
        IDirect3DDevice9_GetSwapChain(device_ptr, 0, &swapchain);
        IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters);
        IDirect3DSwapChain9_Release(swapchain);
        ref = IDirect3DDevice9_Release(device_ptr);
        DestroyWindow(present_parameters.hDeviceWindow);
        ok(ref == 0, "The device was not properly freed: refcount %u\n", ref);
    }
}
