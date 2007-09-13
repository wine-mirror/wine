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
 * area which shows specific behavior(like a quad on the whole screen), and try to get resulting colos with
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
    color = getPixelColor(device, 480, 360); /* lower left quad - unlit width normals */
    ok(color == 0x000000ff, "Unlit quad width normals has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper left quad - lit width normals */
    ok(color == 0x00000000, "Lit quad width normals has color %08x\n", color);

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
    /* Clear 2 rectangles with one call. Shows that a positive value is returned, but the negative rectangle
     * is ignored, the positive is still cleared afterwards
     */
    hr = IDirect3DDevice9_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.x1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice9_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);
}

typedef struct {
    float in[4];
    DWORD out;
} test_data_t;

/*
 *  c7      rounded     ARGB
 * -2.4     -2          0x00ffff00
 * -1.6     -2          0x00ffff00
 * -0.4      0          0x0000ffff
 *  0.4      0          0x0000ffff
 *  1.6      2          0x00ff00ff
 *  2.4      2          0x00ff00ff
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

    static const test_data_t test_data[] = {
        {{-2.4f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
        {{-1.6f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
        {{-0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
        {{ 0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
        {{ 1.6f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff},
        {{ 2.4f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff}
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
    HRESULT hr;
    int i;

    hr = IDirect3DDevice9_CreateVertexShader(device, mova_test, &mova_shader);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, mova_shader);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(test_data_t)); ++i)
    {
        DWORD color;

        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 7, test_data[i].in, 1);
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
        ok(color == test_data[i].out, "Expected color %08x, got %08x (for input %f)\n", test_data[i].out, color, test_data[i].in[0]);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed (%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DVertexShader9_Release(mova_shader);
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
    INT x, y, face;

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "CreateOffscreenPlainSurface failed (0x%08x)\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, D3DLOCK_DISCARD);
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
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_NONE);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_NONE);
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

        hr = IDirect3DTexture9_LockRect(texture[i], 0, &locked_rect, NULL, D3DLOCK_DISCARD);
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
    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    if(FAILED(hr)) {
        skip("Can't create test vertex shader, most likely shaders not supported\n");
        goto out;
    }
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);

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
    hr = IDirect3DSurface9_LockRect(surface, &l, NULL, D3DLOCK_DISCARD);
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

        /* What does this mean? Not sure... */
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
    ok(color == 0x00ff8000 || color == 0x00fe7f00, "quad 3 has color %08x, expected 0x00ff8000\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0033cc00 || color == 0x0032cb00, "quad 4 has color %08x, expected 0x0033cc00\n", color);

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
         * turns out that the blue color is missing, so it is an output modification
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

        /* D3DTTFF_COUNT1. Set a NULL matrix, and count1, pass in all values as 1.0 */
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
    ok(color == 0x00ffff00, "quad 2 has color %08x, expected 0x00ffff00\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x000000ff, "quad 3 has color %08x, expected 0x000000ff\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x00ffffff, "quad 4 has color %08x, expected 0x00ffffff\n", color);

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

    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetPixelShader failed (%08x)\n", hr);
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
    ok(color == 0x008262ca, "D3DFMT_X8L8V8U8 = 0x112131ca returns color %08x, expected 0x008262ca\n", color);

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
    release_buffer_test(device_ptr);
    float_texture_test(device_ptr);
    texture_transform_flags_test(device_ptr);
    autogen_mipmap_test(device_ptr);


    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1))
    {
        test_constant_clamp_vs(device_ptr);
    }
    else skip("No vs_1_1 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0))
    {
        test_mova(device_ptr);
    }
    else skip("No vs_2_0 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1) && caps.PixelShaderVersion >= D3DPS_VERSION(1, 1))
    {
        fog_with_shader_test(device_ptr);
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
        }
    }
    else skip("No ps_1_1 support\n");

cleanup:
    if(device_ptr) IDirect3DDevice9_Release(device_ptr);
}
