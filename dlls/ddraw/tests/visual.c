/*
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

/* See comment in dlls/d3d9/tests/visual.c for general guidelines */

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

HWND window;
IDirectDraw7        *DirectDraw = NULL;
IDirectDrawSurface7 *Surface;
IDirect3D7          *Direct3D = NULL;
IDirect3DDevice7    *Direct3DDevice = NULL;

IDirectDraw *DirectDraw1 = NULL;
IDirectDrawSurface *Surface1 = NULL;
IDirect3D *Direct3D1 = NULL;
IDirect3DDevice *Direct3DDevice1 = NULL;
IDirect3DExecuteBuffer *ExecuteBuffer = NULL;
IDirect3DViewport *Viewport = NULL;

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

static BOOL createObjects(void)
{
    HRESULT hr;
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    WNDCLASS wc = {0};
    DDSURFACEDESC2 ddsd;


    if(!hmod) return FALSE;
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    if(!pDirectDrawCreateEx) return FALSE;

    hr = pDirectDrawCreateEx(NULL, (void **) &DirectDraw, &IID_IDirectDraw7, NULL);
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", hr);
    if(!DirectDraw) goto err;

    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d7_test_wc";
    RegisterClass(&wc);
    window = CreateWindow("d3d7_test_wc", "d3d7_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDirectDraw7_SetCooperativeLevel(DirectDraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "IDirectDraw7_SetCooperativeLevel failed with %08x\n", hr);
    if(FAILED(hr)) goto err;
    hr = IDirectDraw7_SetDisplayMode(DirectDraw, 640, 480, 32, 0, 0);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw7_SetDisplayMode(DirectDraw, 640, 480, 24, 0, 0);

    }
    ok(hr == DD_OK || hr == DDERR_UNSUPPORTED, "IDirectDraw7_SetDisplayMode failed with %08x\n", hr);
    if(FAILED(hr)) {
        /* use trace, the caller calls skip() */
        trace("SetDisplayMode failed\n");
        goto err;
    }

    hr = IDirectDraw7_QueryInterface(DirectDraw, &IID_IDirect3D7, (void**) &Direct3D);
    if (hr == E_NOINTERFACE) goto err;
    ok(hr==DD_OK, "QueryInterface returned: %08x\n", hr);

    /* DirectDraw Flipping behavior doesn't seem that well-defined. The reference rasterizer behaves differently
     * than hardware implementations. Request single buffering, that seems to work everywhere
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    ddsd.dwBackBufferCount = 1;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &Surface, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %08x\n", hr);
    if(!Surface) goto err;

    hr = IDirect3D7_CreateDevice(Direct3D, &IID_IDirect3DTnLHalDevice, Surface, &Direct3DDevice);
    if(FAILED(hr))
    {
        trace("Creating a TnLHal Device failed, trying HAL\n");
        hr = IDirect3D7_CreateDevice(Direct3D, &IID_IDirect3DHALDevice, Surface, &Direct3DDevice);
        if(FAILED(hr))
        {
            trace("Creating a HAL device failed, trying Ref\n");
            hr = IDirect3D7_CreateDevice(Direct3D, &IID_IDirect3DRefDevice, Surface, &Direct3DDevice);
        }
    }
    ok(hr == D3D_OK, "IDirect3D7_CreateDevice failed with %08x\n", hr);
    if(!Direct3DDevice) goto err;
    return TRUE;

    err:
    if(DirectDraw) IDirectDraw7_Release(DirectDraw);
    if(Surface) IDirectDrawSurface7_Release(Surface);
    if(Direct3D) IDirect3D7_Release(Direct3D);
    if(Direct3DDevice) IDirect3DDevice7_Release(Direct3DDevice);
    if(window) DestroyWindow(window);
    return FALSE;
}

static void releaseObjects(void)
{
    IDirect3DDevice7_Release(Direct3DDevice);
    IDirect3D7_Release(Direct3D);
    IDirectDrawSurface7_Release(Surface);
    IDirectDraw7_Release(DirectDraw);
    DestroyWindow(window);
}

static DWORD getPixelColor(IDirect3DDevice7 *device, UINT x, UINT y)
{
    DWORD ret;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    RECT rectToLock = {x, y, x+1, y+1};
    IDirectDrawSurface7 *surf = NULL;

    /* Some implementations seem to dislike direct locking on the front buffer. Thus copy the front buffer
     * to an offscreen surface and lock it instead of the front buffer
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface failed with %08x\n", hr);
    if(!surf)
    {
        trace("cannot create helper surface\n");
        return 0xdeadbeef;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);

    hr = IDirectDrawSurface_BltFast(surf, 0, 0, Surface, NULL, 0);
    ok(hr == DD_OK, "IDirectDrawSurface7_BltFast returned %08x\n", hr);
    if(FAILED(hr))
    {
        trace("Cannot blit\n");
        ret = 0xdeadbee;
        goto out;
    }

    hr = IDirectDrawSurface7_Lock(surf, &rectToLock, &ddsd, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%08x\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) ddsd.lpSurface)[0] & 0x00ffffff;
    hr = IDirectDrawSurface7_Unlock(surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%08x\n", hr);
    }

out:
    IDirectDrawSurface7_Release(surf);
    return ret;
}

struct vertex
{
    float x, y, z;
    DWORD diffuse;
};

struct nvertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
};

static void lighting_test(IDirect3DDevice7 *device)
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

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_VIEW, (D3DMATRIX *)mat);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed with %08x\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    if(hr == D3D_OK)
    {
        /* No lights are defined... That means, lit vertices should be entirely black */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, fvf, unlitquad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "IDirect3DDevice7_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, fvf, litquad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "IDirect3DDevice7_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, unlitnquad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "IDirect3DDevice7_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, litnquad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "IDirect3DDevice7_DrawIndexedPrimitiveUP failed with %08x\n", hr);

        IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360); /* lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower left quad - unlit width normals */
    ok(color == 0x000000ff, "Unlit quad width normals has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper left quad - lit width normals */
    ok(color == 0x00000000, "Lit quad width normals has color %08x\n", color);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned %08x\n", hr);
}

static void clear_test(IDirect3DDevice7 *device)
{
    /* Tests the correctness of clearing parameters */
    HRESULT hr;
    D3DRECT rect[2];
    D3DRECT rect_negneg;
    DWORD color;

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    /* Positive x, negative y */
    U1(rect[0]).x1 = 0;
    U2(rect[0]).y1 = 480;
    U3(rect[0]).x2 = 320;
    U4(rect[0]).y2 = 240;

    /* Positive x, positive y */
    U1(rect[1]).x1 = 0;
    U2(rect[1]).y1 = 0;
    U3(rect[1]).x2 = 320;
    U4(rect[1]).y2 = 240;
    /* Clear 2 rectangles with one call. Shows that a positive value is returned, but the negative rectangle
     * is ignored, the positive is still cleared afterwards
     */
    hr = IDirect3DDevice7_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    /* negative x, negative y */
    U1(rect_negneg).x1 = 640;
    U2(rect_negneg).y1 = 240;
    U3(rect_negneg).x2 = 320;
    U4(rect_negneg).y2 = 0;
    hr = IDirect3DDevice7_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);
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

static void fog_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    DWORD color;
    float start = 0.0, end = 1.0;
    D3DDEVICEDESC7 caps;

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
    hr = IDirect3DDevice7_GetCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetCaps returned %08x\n", hr);
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear returned %08x\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Turning on fog calculations returned %08x\n", hr);

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %08x\n", hr);

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK)
    {
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   unstransformed_1, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

        /* That makes it use the Z value */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Turning off table fog returned %08x\n", hr);
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   unstransformed_2, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

        /* transformed verts */
        ok( hr == D3D_OK, "SetFVF returned %08x\n", hr);
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   transformed_1, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %08x\n", hr);
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   transformed_2, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %08x\n", hr);
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000FF00, "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    if(caps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
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

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %08x\n", hr);
}

static void offscreen_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    DWORD color;
    DDSURFACEDESC2 ddsd;

    static const float quad[][5] = {
        {-0.5f, -0.5f, 0.1f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.1f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.1f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.1f, 1.0f, 1.0f},
    };

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &offscreen, NULL);
    ok(hr == D3D_OK, "Creating the offscreen render target failed, hr = %08x\n", hr);
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice7_GetRenderTarget(device, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState returned hr = %08x\n", hr);

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK) {
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        /* Draw without textures - Should resut in a white quad */
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetTexture(device, 0, offscreen);
        ok(hr == D3D_OK, "SetTexture failed, %08x\n", hr);

        /* This time with the texture */
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        IDirect3DDevice7_EndScene(device);
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

out:
    hr = IDirect3DDevice7_SetTexture(device, 0, NULL);

    /* restore things */
    if(backbuffer) {
        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        IDirectDrawSurface7_Release(backbuffer);
    }
    if(offscreen) {
        IDirectDrawSurface7_Release(offscreen);
    }
}

static void alpha_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    DWORD color, red, green, blue;
    DDSURFACEDESC2 ddsd;

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
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount      = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask         = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask         = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask         = 0x000000ff;
    U5(U4(ddsd).ddpfPixelFormat).dwRGBAlphaBitMask  = 0xff000000;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &offscreen, NULL);
    ok(hr == D3D_OK, "Creating the offscreen render target failed, hr = %08x\n", hr);
    if(!offscreen) {
        goto out;
    }
    hr = IDirect3DDevice7_GetRenderTarget(device, &backbuffer);
    ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "SetTextureStageState failed, hr = %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
    if(IDirect3DDevice7_BeginScene(device) == D3D_OK) {

        /* Draw two quads, one with src alpha blending, one with dest alpha blending. The
         * SRCALPHA / INVSRCALPHA blend doesn't give any surprises. Colors are blended based on
         * the input alpha
         *
         * The DESTALPHA / INVDESTALPHA do not "work" on the regular buffer because there is no alpha.
         * They give essentially ZERO and ONE blend factors
         */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad2, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        /* Switch to the offscreen buffer, and redo the testing. SRCALPHA and DESTALPHA. The offscreen buffer
         * has a alpha channel on its own. Clear the offscreen buffer with alpha = 0.5 again, then draw the
         * quads again. The SRCALPHA/INVSRCALPHA doesn't give any surprises, but the DESTALPHA/INVDESTALPHA
         * blending works as supposed now - blend factor is 0.5 in both cases, not 0.75 as from the input
         * vertices
         */
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);
        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad2, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(hr == D3D_OK, "Can't get back buffer, hr = %08x\n", hr);

        /* Render the offscreen texture onto the frame buffer to be able to compare it regularly.
         * Disable alpha blending for the final composition
         */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderState failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetTexture(device, 0, offscreen);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetTexture failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, composite_quad, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);
        hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetTexture failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed, hr = %08x\n", hr);
    }

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
    if(offscreen) IDirectDrawSurface7_Release(offscreen);
    if(backbuffer) IDirectDrawSurface7_Release(backbuffer);
}

static void rhw_zero_test(IDirect3DDevice7 *device)
{
/* Test if it will render a quad correctly when vertex rhw = 0 */
    HRESULT hr;
    DWORD color;

    struct {
        float x, y, z;
        float rhw;
        DWORD diffuse;
        } quad1[] =
    {
        {0, 100, 0, 0, 0xffffffff},
        {0, 0, 0, 0, 0xffffffff},
        {100, 100, 0, 0, 0xffffffff},
        {100, 0, 0, 0, 0xffffffff},
    };

    /* Clear to black */
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0, 0);
    ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed, hr = %08x\n", hr);
    }

    color = getPixelColor(device, 5, 5);
    ok(color == 0xffffff, "Got color %08x, expected 00ffffff\n", color);

    color = getPixelColor(device, 105, 105);
    ok(color == 0, "Got color %08x, expected 00000000\n", color);
}

static BOOL D3D1_createObjects(void)
{
    WNDCLASS wc = {0};
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DEXECUTEBUFFERDESC exdesc;
    D3DVIEWPORT vp_data;

    /* An IDirect3DDevice cannot be queryInterfaced from an IDirect3DDevice7 on windows */
    hr = DirectDrawCreate(NULL, &DirectDraw1, NULL);

    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "texturemapblend_test_wc";
    RegisterClass(&wc);
    window = CreateWindow("texturemapblend_test_wc", "texturemapblend_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);
    if(FAILED(hr)) {
        return FALSE;
    }

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 32);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 24);
    }
    ok(hr==DD_OK || hr == DDERR_UNSUPPORTED, "SetDisplayMode returned: %x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirect3D, (void**) &Direct3D1);
    ok(hr==DD_OK, "QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Surface1, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = IDirectDrawSurface_QueryInterface(Surface1, &IID_IDirect3DHALDevice, (void **) &Direct3DDevice1);
    if(FAILED(hr)) {
        trace("Creating a HAL device failed, trying Ref\n");
        hr = IDirectDrawSurface_QueryInterface(Surface1, &IID_IDirect3DRefDevice, (void **) &Direct3DDevice1);
    }
    ok(hr==D3D_OK, "Creating 3D device returned: %x\n", hr);
    if(FAILED(hr)) {
        return FALSE;
    }

    hr = IDirect3D_CreateViewport(Direct3D1, &Viewport, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateViewport failed: %08x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = IDirect3DViewport_Initialize(Viewport, Direct3D1);
    ok(hr == D3D_OK || hr == DDERR_ALREADYINITIALIZED, "IDirect3DViewport_Initialize returned %08x\n", hr);
    hr = IDirect3DDevice_AddViewport(Direct3DDevice1, Viewport);
    ok(hr == D3D_OK, "IDirect3DDevice_AddViewport returned %08x\n", hr);
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwX = 0;
    vp_data.dwY = 0;
    vp_data.dwWidth = 640;
    vp_data.dwHeight = 480;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    vp_data.dvMaxX = 640;
    vp_data.dvMaxY = 480;
    vp_data.dvMinZ = 0;
    vp_data.dvMaxZ = 1;
    hr = IDirect3DViewport_SetViewport(Viewport, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);

    memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    exdesc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exdesc.dwBufferSize = 512;
    exdesc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(Direct3DDevice1, &exdesc, &ExecuteBuffer, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice_CreateExecuteBuffer failed with %08x\n", hr);
    if (FAILED(hr)) {
        return FALSE;
    }

    return TRUE;
}

static void D3D1_releaseObjects(void)
{
    if(ExecuteBuffer) IDirect3DExecuteBuffer_Release(ExecuteBuffer);
    if(Surface1) IDirectDrawSurface_Release(Surface1);
    if(Viewport) IDirect3DViewport_Release(Viewport);
    if(Direct3DDevice1) IDirect3DDevice_Release(Direct3DDevice1);
    if(Direct3D1) IDirect3D_Release(Direct3D1);
    if(DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if(window) DestroyWindow(window);
}

static DWORD D3D1_getPixelColor(IDirectDraw *DirectDraw1, IDirectDrawSurface *Surface, UINT x, UINT y)
{
    DWORD ret;
    HRESULT hr;
    DDSURFACEDESC ddsd;
    RECT rectToLock = {x, y, x+1, y+1};
    IDirectDrawSurface *surf = NULL;

    /* Some implementations seem to dislike direct locking on the front buffer. Thus copy the front buffer
     * to an offscreen surface and lock it instead of the front buffer
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface failed with %08x\n", hr);
    if(!surf)
    {
        trace("cannot create helper surface\n");
        return 0xdeadbeef;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);

    hr = IDirectDrawSurface_BltFast(surf, 0, 0, Surface, NULL, 0);
    ok(hr == DD_OK, "IDirectDrawSurface_BltFast returned %08x\n", hr);
    if(FAILED(hr))
    {
        trace("Cannot blit\n");
        ret = 0xdeadbee;
        goto out;
    }

    hr = IDirectDrawSurface_Lock(surf, &rectToLock, &ddsd, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%08x\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) ddsd.lpSurface)[0] & 0x00ffffff;
    hr = IDirectDrawSurface_Unlock(surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%08x\n", hr);
    }

out:
    IDirectDrawSurface_Release(surf);
    return ret;
}

#define EXEBUF_START_RENDER_STATES(count, ptr) do {\
                         ((D3DINSTRUCTION*)(ptr))->bOpcode = D3DOP_STATERENDER;\
                         ((D3DINSTRUCTION*)(ptr))->bSize = sizeof(D3DSTATE);\
                         ((D3DINSTRUCTION*)(ptr))->wCount = count;\
                         ptr = ((D3DINSTRUCTION*)(ptr))+1; } while (0)

#define EXEBUF_PUT_RENDER_STATE(state, value, ptr) do {\
                         U1(*((D3DSTATE*)(ptr))).drstRenderStateType = state; \
                         U2(*((D3DSTATE*)(ptr))).dwArg[0] = value; \
                         ptr = ((D3DSTATE*)(ptr))+1; } while (0)

#define EXEBUF_PUT_PROCESSVERTICES(nvertices, ptr) do {\
                         ((D3DINSTRUCTION*)(ptr))->bOpcode = D3DOP_PROCESSVERTICES;\
                         ((D3DINSTRUCTION*)(ptr))->bSize = sizeof(D3DPROCESSVERTICES);\
                         ((D3DINSTRUCTION*)(ptr))->wCount = 1;\
                         ptr = ((D3DINSTRUCTION*)(ptr))+1;\
                         ((D3DPROCESSVERTICES*)(ptr))->dwFlags = D3DPROCESSVERTICES_COPY;\
                         ((D3DPROCESSVERTICES*)(ptr))->wStart = 0;\
                         ((D3DPROCESSVERTICES*)(ptr))->wDest = 0;\
                         ((D3DPROCESSVERTICES*)(ptr))->dwCount = nvertices;\
                         ((D3DPROCESSVERTICES*)(ptr))->dwReserved = 0;\
                         ptr = ((D3DPROCESSVERTICES*)(ptr))+1; } while (0)

#define EXEBUF_END(ptr) do {\
                         ((D3DINSTRUCTION*)(ptr))->bOpcode = D3DOP_EXIT;\
                         ((D3DINSTRUCTION*)(ptr))->bSize = 0;\
                         ((D3DINSTRUCTION*)(ptr))->wCount = 0;\
                         ptr = ((D3DINSTRUCTION*)(ptr))+1; } while (0)

#define EXEBUF_PUT_QUAD(base_idx, ptr) do {\
                         ((D3DINSTRUCTION*)(ptr))->bOpcode = D3DOP_TRIANGLE;\
                         ((D3DINSTRUCTION*)(ptr))->bSize = sizeof(D3DTRIANGLE);\
                         ((D3DINSTRUCTION*)(ptr))->wCount = 2;\
                         ptr = ((D3DINSTRUCTION*)(ptr))+1;\
                         U1(*((D3DTRIANGLE*)(ptr))).v1 = base_idx;\
                         U2(*((D3DTRIANGLE*)(ptr))).v2 = (base_idx) + 1; \
                         U3(*((D3DTRIANGLE*)(ptr))).v3 = (base_idx) + 3; \
                         ((D3DTRIANGLE*)(ptr))->wFlags = 0;\
                         ptr = ((D3DTRIANGLE*)ptr)+1;\
                         U1(*((D3DTRIANGLE*)(ptr))).v1 = (base_idx) + 1; \
                         U2(*((D3DTRIANGLE*)(ptr))).v2 = (base_idx) + 2; \
                         U3(*((D3DTRIANGLE*)(ptr))).v3 = (base_idx) + 3; \
                         ((D3DTRIANGLE*)(ptr))->wFlags = 0;\
                         ptr = ((D3DTRIANGLE*)(ptr))+1;\
                        } while (0)

HRESULT CALLBACK TextureFormatEnumCallback(LPDDSURFACEDESC lpDDSD, LPVOID lpContext)
{
    if (lpDDSD->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        *(BOOL*)lpContext = TRUE;
    }

    return DDENUMRET_OK;
}

static void D3D1_TextureMapBlendTest(void)
{
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DEXECUTEBUFFERDESC exdesc;
    D3DEXECUTEDATA exdata;
    DDBLTFX ddbltfx;
    RECT rect = { 0, 0, 64, 128 };
    DWORD color, red, blue, green;
    void *exe_buffer_ptr;
    DWORD exe_length;
    D3DTEXTUREHANDLE htex;
    DDCOLORKEY clrKey;
    IDirectDrawSurface *TexSurface = NULL;
    IDirect3DTexture *Texture = NULL;
    IDirectDrawPalette *Palette = NULL;
    PALETTEENTRY table1[256];
    BOOL p8_textures_supported = FALSE;

    struct {
        float x, y, z;
        float rhw;
        DWORD diffuse;
        DWORD specular;
        float tu, tv;
        } test1_quads[] =
    {
          {0.0f,   0.0f,     0.0f, 1.0f, 0xffffffff, 0, 0.0f, 0.0f},
          {640.0f, 0.0f,     0.0f, 1.0f, 0xffffffff, 0, 1.0f, 0.0f},
          {640.0f, 240.0f,   0.0f, 1.0f, 0xffffffff, 0, 1.0f, 1.0f},
          {0.0f,   240.0f,   0.0f, 1.0f, 0xffffffff, 0, 0.0f, 1.0f},
          {0.0f,   240.0f,   0.0f, 1.0f, 0x80ffffff, 0, 0.0f, 0.0f},
          {640.0f, 240.0f,   0.0f, 1.0f, 0x80ffffff, 0, 1.0f, 0.0f},
          {640.0f, 480.0f,   0.0f, 1.0f, 0x80ffffff, 0, 1.0f, 1.0f},
          {0.0f,   480.0f,   0.0f, 1.0f, 0x80ffffff, 0, 0.0f, 1.0f}
    },  test2_quads[] =
          {
          {0.0f,   0.0f,     0.0f, 1.0f, 0x00ff0080, 0, 0.0f, 0.0f},
          {640.0f, 0.0f,     0.0f, 1.0f, 0x00ff0080, 0, 1.0f, 0.0f},
          {640.0f, 240.0f,   0.0f, 1.0f, 0x00ff0080, 0, 1.0f, 1.0f},
          {0.0f,   240.0f,   0.0f, 1.0f, 0x00ff0080, 0, 0.0f, 1.0f},
          {0.0f,   240.0f,   0.0f, 1.0f, 0x008000ff, 0, 0.0f, 0.0f},
          {640.0f, 240.0f,   0.0f, 1.0f, 0x008000ff, 0, 1.0f, 0.0f},
          {640.0f, 480.0f,   0.0f, 1.0f, 0x008000ff, 0, 1.0f, 1.0f},
          {0.0f,   480.0f,   0.0f, 1.0f, 0x008000ff, 0, 0.0f, 1.0f}
    };

    /* 1) Test alpha with DDPF_ALPHAPIXELS texture - should be taken from texture alpha channel*/
    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask         = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask         = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask         = 0x000000ff;
    U5(ddsd.ddpfPixelFormat).dwRGBAlphaBitMask  = 0xff000000;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto out;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto out;
    }

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    U5(ddbltfx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface_Blt(TexSurface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
    U5(ddbltfx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface_Blt(TexSurface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &exdesc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed with %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirect3DExecuteBuffer_Lock failed; skipping further tests\n");
        goto out;
    }

    memcpy(exdesc.lpData, test1_quads, sizeof(test1_quads));

    exe_buffer_ptr = 256 + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(12, exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_CULLMODE,         D3DCULL_NONE,              exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ZENABLE,          FALSE,                     exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_FOGENABLE,        FALSE,                     exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_SPECULARENABLE,   FALSE,                     exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAG,       D3DFILTER_NEAREST,         exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREMIN,       D3DFILTER_NEAREST,         exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_FILLMODE  ,       D3DFILL_SOLID,             exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_SRCBLEND,         D3DBLEND_SRCALPHA,         exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_DESTBLEND,        D3DBLEND_INVSRCALPHA,      exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE,                      exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREMAPBLEND,  D3DTBLEND_MODULATE,        exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - 256;

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(D3DEXECUTEDATA));
    exdata.dwSize = sizeof(D3DEXECUTEDATA);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = 256;
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
    }

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue >= 0x7e && blue <= 0x82, "Got color %08x, expected 00000080 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue == 0xff, "Got color %08x, expected 000000ff or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue >= 0x7e && blue <= 0x82, "Got color %08x, expected 00000080 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue == 0xff, "Got color %08x, expected 000000ff or near\n", color);

    /* 2) Test alpha with texture that has no alpha channel - alpha should be taken from diffuse color */
    if(Texture) IDirect3DTexture_Release(Texture);
    Texture = NULL;
    if(TexSurface) IDirectDrawSurface_Release(TexSurface);
    TexSurface = NULL;

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask         = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask         = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask         = 0x000000ff;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto out;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto out;
    }

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    U5(ddbltfx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface_Blt(TexSurface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
    U5(ddbltfx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface_Blt(TexSurface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &exdesc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed with %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirect3DExecuteBuffer_Lock failed; skipping further tests\n");
        goto out;
    }

    memcpy(exdesc.lpData, test1_quads, sizeof(test1_quads));

    exe_buffer_ptr = 256 + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(1, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - 256;

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(D3DEXECUTEDATA));
    exdata.dwSize = sizeof(D3DEXECUTEDATA);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = 256;
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
    }

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue == 0xff, "Got color %08x, expected 000000ff or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue == 0xff, "Got color %08x, expected 000000ff or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue >= 0x7e && blue <= 0x82, "Got color %08x, expected 00000080 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 &&  green == 0 && blue >= 0x7e && blue <= 0x82, "Got color %08x, expected 00000080 or near\n", color);

    /* 3) Test RGB - should multiply color components from diffuse color and texture */
    if(Texture) IDirect3DTexture_Release(Texture);
    Texture = NULL;
    if(TexSurface) IDirectDrawSurface_Release(TexSurface);
    TexSurface = NULL;

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask         = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask         = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask         = 0x000000ff;
    U5(ddsd.ddpfPixelFormat).dwRGBAlphaBitMask  = 0xff000000;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto out;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto out;
    }

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    U5(ddbltfx).dwFillColor = 0x00ffffff;
    hr = IDirectDrawSurface_Blt(TexSurface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
    U5(ddbltfx).dwFillColor = 0x00ffff80;
    hr = IDirectDrawSurface_Blt(TexSurface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &exdesc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed with %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirect3DExecuteBuffer_Lock failed; skipping further tests\n");
        goto out;
    }

    memcpy(exdesc.lpData, test2_quads, sizeof(test2_quads));

    exe_buffer_ptr = 256 + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - 256;

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(D3DEXECUTEDATA));
    exdata.dwSize = sizeof(D3DEXECUTEDATA);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = 256;
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
    }

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff &&  green == 0 && blue >= 0x3e && blue <= 0x42, "Got color %08x, expected 00ff0040 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff &&  green == 0 && blue == 0x80, "Got color %08x, expected 00ff0080 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x82 &&  green == 0 && blue == 0x80, "Got color %08x, expected 00800080 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x82 &&  green == 0 && blue == 0xff, "Got color %08x, expected 008000ff or near\n", color);

    /* 4) Test alpha again, now with color keyed texture (colorkey emulation in wine can interfere) */
    if(Texture) IDirect3DTexture_Release(Texture);
    Texture = NULL;
    if(TexSurface) IDirectDrawSurface_Release(TexSurface);
    TexSurface = NULL;

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 16;
    U2(ddsd.ddpfPixelFormat).dwRBitMask         = 0xf800;
    U3(ddsd.ddpfPixelFormat).dwGBitMask         = 0x07e0;
    U4(ddsd.ddpfPixelFormat).dwBBitMask         = 0x001f;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
    ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
        goto out;
    }

    hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                (void *)&Texture);
    ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
    if (FAILED(hr)) {
        skip("Can't get IDirect3DTexture interface; skipping further tests\n");
        goto out;
    }

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
    U5(ddbltfx).dwFillColor = 0xf800;
    hr = IDirectDrawSurface_Blt(TexSurface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
    U5(ddbltfx).dwFillColor = 0x001f;
    hr = IDirectDrawSurface_Blt(TexSurface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    clrKey.dwColorSpaceLowValue = 0x001f;
    clrKey.dwColorSpaceHighValue = 0x001f;
    hr = IDirectDrawSurface_SetColorKey(TexSurface, DDCKEY_SRCBLT, &clrKey);
    ok(hr==D3D_OK, "IDirectDrawSurfac_SetColorKey returned: %x\n", hr);

    memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
    exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
    hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &exdesc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed with %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirect3DExecuteBuffer_Lock failed; skipping further tests\n");
        goto out;
    }

    memcpy(exdesc.lpData, test1_quads, sizeof(test1_quads));

    exe_buffer_ptr = 256 + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - 256;

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(D3DEXECUTEDATA));
    exdata.dwSize = sizeof(D3DEXECUTEDATA);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = 256;
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
    }

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
    ok(color == 0, "Got color %08x, expected 00000000\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff &&  green == 0 && blue == 0, "Got color %08x, expected 00ff0000 or near\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 245);
    ok(color == 0, "Got color %08x, expected 00000000\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 245);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x82 &&  green == 0 && blue == 0, "Got color %08x, expected 00800000 or near\n", color);

    /* 5) Test alpha again, now with color keyed P8 texture */
    if(Texture) IDirect3DTexture_Release(Texture);
    Texture = NULL;
    if(TexSurface) IDirectDrawSurface_Release(TexSurface);
    TexSurface = NULL;

    hr = IDirect3DDevice_EnumTextureFormats(Direct3DDevice1, TextureFormatEnumCallback,
                                                &p8_textures_supported);
    ok(hr == DD_OK, "IDirect3DDevice_EnumTextureFormats returned %08x\n", hr);

    if (!p8_textures_supported) {
        skip("device has no P8 texture support, skipping test\n");
    } else {
        memset (&ddsd, 0, sizeof (ddsd));
        ddsd.dwSize = sizeof (ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.dwHeight = 128;
        ddsd.dwWidth = 128;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 8;

        hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &TexSurface, NULL);
        ok(hr==D3D_OK, "CreateSurface returned: %x\n", hr);
        if (FAILED(hr)) {
            skip("IDirectDraw_CreateSurface failed; skipping further tests\n");
            goto out;
        }

        memset(table1, 0, sizeof(table1));
        table1[0].peBlue = 0xff;
        table1[1].peRed = 0xff;

        hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, table1, &Palette, NULL);
        ok(hr == DD_OK, "CreatePalette returned %08x\n", hr);
        if (FAILED(hr)) {
            skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
            goto out;
        }

        hr = IDirectDrawSurface_SetPalette(TexSurface, Palette);
        ok(hr==D3D_OK, "IDirectDrawSurface_SetPalette returned: %x\n", hr);

        hr = IDirectDrawSurface_QueryInterface(TexSurface, &IID_IDirect3DTexture,
                    (void *)&Texture);
        ok(hr==D3D_OK, "IDirectDrawSurface_QueryInterface returned: %x\n", hr);
        if (FAILED(hr)) {
            skip("Can't get IDirect3DTexture interface; skipping further tests\n");
            goto out;
        }

        memset(&ddbltfx, 0, sizeof(ddbltfx));
        ddbltfx.dwSize = sizeof(ddbltfx);
        U5(ddbltfx).dwFillColor = 0;
        hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
        U5(ddbltfx).dwFillColor = 0;
        hr = IDirectDrawSurface_Blt(TexSurface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);
        U5(ddbltfx).dwFillColor = 1;
        hr = IDirectDrawSurface_Blt(TexSurface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
        ok(hr == D3D_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

        clrKey.dwColorSpaceLowValue = 1;
        clrKey.dwColorSpaceHighValue = 1;
        hr = IDirectDrawSurface_SetColorKey(TexSurface, DDCKEY_SRCBLT, &clrKey);
        ok(hr==D3D_OK, "IDirectDrawSurfac_SetColorKey returned: %x\n", hr);

        memset(&exdesc, 0, sizeof(D3DEXECUTEBUFFERDESC));
        exdesc.dwSize = sizeof(D3DEXECUTEBUFFERDESC);
        hr = IDirect3DExecuteBuffer_Lock(ExecuteBuffer, &exdesc);
        ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed with %08x\n", hr);
        if (FAILED(hr)) {
            skip("IDirect3DExecuteBuffer_Lock failed; skipping further tests\n");
            goto out;
        }

        memcpy(exdesc.lpData, test1_quads, sizeof(test1_quads));

        exe_buffer_ptr = 256 + (char*)exdesc.lpData;

        EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

        EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
        EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE, exe_buffer_ptr);
        hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
        ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
        EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

        EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
        EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

        EXEBUF_END(exe_buffer_ptr);

        exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - 256;

        hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
        if (FAILED(hr)) {
            trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
        }

        memset(&exdata, 0, sizeof(D3DEXECUTEDATA));
        exdata.dwSize = sizeof(D3DEXECUTEDATA);
        exdata.dwVertexCount = 8;
        exdata.dwInstructionOffset = 256;
        exdata.dwInstructionLength = exe_length;
        hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
        ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

        hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

        if (SUCCEEDED(hr)) {
            hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
            ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
            hr = IDirect3DDevice_EndScene(Direct3DDevice1);
            ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
        }

        color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
        ok(color == 0, "Got color %08x, expected 00000000\n", color);

        color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 5);
        red =   (color & 0x00ff0000) >> 16;
        green = (color & 0x0000ff00) >>  8;
        blue =  (color & 0x000000ff);
        ok(red == 0 &&  green == 0 && blue == 0xff, "Got color %08x, expected 000000ff or near\n", color);

        color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 245);
        ok(color == 0, "Got color %08x, expected 00000000\n", color);

        color = D3D1_getPixelColor(DirectDraw1, Surface1, 400, 245);
        red =   (color & 0x00ff0000) >> 16;
        green = (color & 0x0000ff00) >>  8;
        blue =  (color & 0x000000ff);
        ok(red == 0 &&  green == 0 && blue >= 0x7e && blue <= 0x82, "Got color %08x, expected 00000080 or near\n", color);
    }

    out:

    if (Palette) IDirectDrawPalette_Release(Palette);
    if (TexSurface) IDirectDrawSurface_Release(TexSurface);
    if (Texture) IDirect3DTexture_Release(Texture);
}

static void p8_surface_fill_rect(IDirectDrawSurface *dest, UINT x, UINT y, UINT w, UINT h, BYTE colorindex)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    UINT i, i1;
    BYTE *p;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    hr = IDirectDrawSurface_Lock(dest, NULL, &ddsd, DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL);
    ok(hr==DD_OK, "IDirectDrawSurface_Lock returned: %x\n", hr);

    p = (BYTE *)ddsd.lpSurface + ddsd.lPitch * y;

    for (i = 0; i < h; i++) {
        for (i1 = 0; i1 < w; i1++) {
            p[i1] = colorindex;
        }
        p += ddsd.lPitch;
    }

    hr = IDirectDrawSurface_Unlock(dest, NULL);
    ok(hr==DD_OK, "IDirectDrawSurface_UnLock returned: %x\n", hr);
}

static COLORREF getPixelColor_GDI(IDirectDrawSurface *Surface, UINT x, UINT y)
{
    COLORREF clr = CLR_INVALID;
    HDC hdc;
    HRESULT hr;

    hr = IDirectDrawSurface_GetDC(Surface, &hdc);
    ok(hr==DD_OK, "IDirectDrawSurface_GetDC returned: %x\n", hr);

    if (SUCCEEDED(hr)) {
        clr = GetPixel(hdc, x, y);

        hr = IDirectDrawSurface_ReleaseDC(Surface, hdc);
        ok(hr==DD_OK, "IDirectDrawSurface_ReleaseDC returned: %x\n", hr);
    }

    return clr;
}

static BOOL colortables_check_equality(PALETTEENTRY table1[256], RGBQUAD table2[256])
{
    int i;

    for (i = 0; i < 256; i++) {
       if (table1[i].peRed != table2[i].rgbRed || table1[i].peGreen != table2[i].rgbGreen ||
           table1[i].peBlue != table2[i].rgbBlue) return FALSE;
    }

    return TRUE;
}

static void p8_primary_test()
{
    /* Test 8bit mode used by games like StarCraft, C&C Red Alert I etc */
    DDSURFACEDESC ddsd;
    HDC hdc;
    HRESULT hr;
    PALETTEENTRY entries[256];
    RGBQUAD coltable[256];
    UINT i;
    IDirectDrawPalette *ddprimpal = NULL;
    IDirectDrawSurface *offscreen = NULL;
    WNDCLASS wc = {0};
    DDBLTFX ddbltfx;
    COLORREF color;

    /* An IDirect3DDevice cannot be queryInterfaced from an IDirect3DDevice7 on windows */
    hr = DirectDrawCreate(NULL, &DirectDraw1, NULL);

    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "p8_primary_test_wc";
    RegisterClass(&wc);
    window = CreateWindow("p8_primary_test_wc", "p8_primary_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);
    if(FAILED(hr)) {
        goto out;
    }

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 8);
    ok(hr==DD_OK || hr == DDERR_UNSUPPORTED, "SetDisplayMode returned: %x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Surface1, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    memset(entries, 0, sizeof(entries));
    entries[0].peRed = 0xff;
    entries[1].peGreen = 0xff;
    entries[2].peBlue = 0xff;

    hr = IDirectDraw_CreatePalette(DirectDraw1, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, entries, &ddprimpal, NULL);
    ok(hr == DD_OK, "CreatePalette returned %08x\n", hr);
    if (FAILED(hr)) {
        skip("IDirectDraw_CreatePalette failed; skipping further tests\n");
        goto out;
    }

    hr = IDirectDrawSurface_SetPalette(Surface1, ddprimpal);
    ok(hr==DD_OK, "IDirectDrawSurface_SetPalette returned: %x\n", hr);

    p8_surface_fill_rect(Surface1, 0, 0, 640, 480, 2);

    color = getPixelColor_GDI(Surface1, 10, 10);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0 && GetBValue(color) == 0xFF,
            "got R %02X G %02X B %02X, expected R 00 G 00 B FF\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 0;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    color = getPixelColor_GDI(Surface1, 10, 10);
    ok(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R FF G 00 B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 8;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &offscreen, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface returned %08x\n", hr);
    if (FAILED(hr)) goto out;

    memset(entries, 0, sizeof(entries));
    for (i = 0; i < 256; i++) {
        entries[i].peBlue = i;
        }
    hr = IDirectDrawPalette_SetEntries(ddprimpal, 0, 0, 256, entries);
    ok(hr == DD_OK, "IDirectDrawPalette_SetEntries failed with %08x\n", hr);

    hr = IDirectDrawSurface_GetDC(offscreen, &hdc);
    ok(hr==DD_OK, "IDirectDrawSurface_GetDC returned: %x\n", hr);
    i = GetDIBColorTable(hdc, 0, 256, coltable);
    ok(i == 256, "GetDIBColorTable returned %u, last error: %x\n", i, GetLastError());
    hr = IDirectDrawSurface_ReleaseDC(offscreen, hdc);
    ok(hr==DD_OK, "IDirectDrawSurface_ReleaseDC returned: %x\n", hr);

    ok(colortables_check_equality(entries, coltable), "unexpected colortable on offscreen surface\n");

    p8_surface_fill_rect(offscreen, 0, 0, 16, 16, 1);

    memset(entries, 0, sizeof(entries));
    entries[0].peRed = 0xff;
    entries[1].peGreen = 0xff;
    entries[2].peBlue = 0xff;
    hr = IDirectDrawPalette_SetEntries(ddprimpal, 0, 0, 256, entries);
    ok(hr == DD_OK, "IDirectDrawPalette_SetEntries failed with %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(Surface1, 0, 0, offscreen, NULL, 0);
    ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

    color = getPixelColor_GDI(Surface1, 1, 1);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0xFF && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R 00 G FF B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    out:

    if(ddprimpal) IDirectDrawPalette_Release(ddprimpal);
    if(offscreen) IDirectDrawSurface_Release(offscreen);
    if(Surface1) IDirectDrawSurface_Release(Surface1);
    if(DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if(window) DestroyWindow(window);
}

static void cubemap_test(IDirect3DDevice7 *device)
{
    IDirect3D7 *d3d;
    IDirectDraw7 *ddraw;
    IDirectDrawSurface7 *cubemap, *surface;
    D3DDEVICEDESC7 d3dcaps;
    HRESULT hr;
    DWORD color;
    DDSURFACEDESC2 ddsd;
    DDBLTFX DDBltFx;
    DDSCAPS2 caps;
    static float quad[] = {
      -1.0,   -1.0,    0.1,    1.0,    0.0,    0.0, /* Lower left */
       0.0,   -1.0,    0.1,    1.0,    0.0,    0.0,
      -1.0,    0.0,    0.1,    1.0,    0.0,    0.0,
       0.0,    0.0,    0.1,    1.0,    0.0,    0.0,

       0.0,   -1.0,    0.1,    0.0,    1.0,    0.0, /* Lower right */
       1.0,   -1.0,    0.1,    0.0,    1.0,    0.0,
       0.0,    0.0,    0.1,    0.0,    1.0,    0.0,
       1.0,    0.0,    0.1,    0.0,    1.0,    0.0,

       0.0,    0.0,    0.1,    0.0,    0.0,    1.0, /* upper right */
       1.0,    0.0,    0.1,    0.0,    0.0,    1.0,
       0.0,    1.0,    0.1,    0.0,    0.0,    1.0,
       1.0,    1.0,    0.1,    0.0,    0.0,    1.0,

      -1.0,    0.0,    0.1,   -1.0,    0.0,    0.0, /* Upper left */
       0.0,    0.0,    0.1,   -1.0,    0.0,    0.0,
      -1.0,    1.0,    0.1,   -1.0,    0.0,    0.0,
       0.0,    1.0,    0.1,   -1.0,    0.0,    0.0,
    };

    memset(&DDBltFx, 0, sizeof(DDBltFx));
    DDBltFx.dwSize = sizeof(DDBltFx);

    memset(&d3dcaps, 0, sizeof(d3dcaps));
    hr = IDirect3DDevice7_GetCaps(device, &d3dcaps);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetCaps returned %08x\n", hr);
    if(!(d3dcaps.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("No cubemap support\n");
        return;
    }

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice7_GetDirect3D(device, &d3d);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetDirect3D returned %08x\n", hr);
    hr = IDirect3D7_QueryInterface(d3d, &IID_IDirectDraw7, (void **) &ddraw);
    ok(hr == D3D_OK, "IDirect3D7_QueryInterface returned %08x\n", hr);
    IDirect3D7_Release(d3d);


    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES | DDSCAPS2_TEXTUREMANAGE;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;

    hr = IDirectDraw7_CreateSurface(ddraw, &ddsd, &cubemap, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface returned %08x\n", hr);
    IDirectDraw7_Release(ddraw);

    /* Positive X */
    DDBltFx.dwFillColor = 0x00ff0000;
    hr = IDirectDrawSurface7_Blt(cubemap, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_TEXTURE;
    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    DDBltFx.dwFillColor = 0x0000ffff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    DDBltFx.dwFillColor = 0x0000ff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    DDBltFx.dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    DDBltFx.dwFillColor = 0x00ffff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    DDBltFx.dwFillColor = 0x00ff00ff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    hr = IDirect3DDevice7_SetTexture(device, 0, cubemap);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTexture returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTextureStageState returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTextureStageState returned %08x\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(hr == DD_OK, "IDirect3DDevice7_BeginScene returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 0 * 6, 4, 0);
        ok(hr == DD_OK, "IDirect3DDevice7_DrawPrimitive returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 4 * 6, 4, 0);
        ok(hr == DD_OK, "IDirect3DDevice7_DrawPrimitive returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 8 * 6, 4, 0);
        ok(hr == DD_OK, "IDirect3DDevice7_DrawPrimitive returned %08x\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 12* 6, 4, 0);
        ok(hr == DD_OK, "IDirect3DDevice7_DrawPrimitive returned %08x\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == DD_OK, "IDirect3DDevice7_EndScene returned %08x\n", hr);
    }
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTextureStageState returned %08x\n", hr);

    color = getPixelColor(device, 160, 360); /* lower left quad - positivex */
    ok(color == 0x00ff0000, "DDSCAPS2_CUBEMAP_POSITIVEX has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - negativex */
    ok(color == 0x0000ffff, "DDSCAPS2_CUBEMAP_NEGATIVEX has color 0x%08x, expected 0x0000ffff\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad - positivey */
    ok(color == 0x00ff00ff, "DDSCAPS2_CUBEMAP_POSITIVEY has color 0x%08x, expected 0x00ff00ff\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad - positivez */
    ok(color == 0x000000ff, "DDSCAPS2_CUBEMAP_POSITIVEZ has color 0x%08x, expected 0x000000ff\n", color);
    hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTexture returned %08x\n", hr);
    IDirectDrawSurface7_Release(cubemap);
}

START_TEST(visual)
{
    HRESULT hr;
    DWORD color;
    if(!createObjects())
    {
        skip("Cannot initialize DirectDraw and Direct3D, skipping\n");
        return;
    }

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice7_Clear(Direct3DDevice, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 1, 1);
    if(color !=0x00ff0000)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice7_Clear(Direct3DDevice, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 639, 479);
    if(color != 0x0000ddee)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now run the tests */
    lighting_test(Direct3DDevice);
    clear_test(Direct3DDevice);
    fog_test(Direct3DDevice);
    offscreen_test(Direct3DDevice);
    alpha_test(Direct3DDevice);
    rhw_zero_test(Direct3DDevice);
    cubemap_test(Direct3DDevice);

    releaseObjects(); /* release DX7 interfaces to test D3D1 */

    if(!D3D1_createObjects()) {
        skip("Cannot initialize D3D1, skipping\n");
    }
    else {
        D3D1_TextureMapBlendTest();
    }
    D3D1_releaseObjects();

    p8_primary_test();

    return ;

cleanup:
    releaseObjects();
}
