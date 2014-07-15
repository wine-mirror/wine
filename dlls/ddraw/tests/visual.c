/*
 * Copyright (C) 2007 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2008 Alexander Dorofeyev
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

#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

static HWND window;
static IDirectDraw7        *DirectDraw;
static IDirectDrawSurface7 *Surface;
static IDirectDrawSurface7 *depth_buffer;
static IDirect3D7          *Direct3D;
static IDirect3DDevice7    *Direct3DDevice;

static IDirectDraw *DirectDraw1;
static IDirectDrawSurface *Surface1;
static IDirect3D *Direct3D1;
static IDirect3DDevice *Direct3DDevice1;
static IDirect3DExecuteBuffer *ExecuteBuffer;
static IDirect3DViewport *Viewport;

static BOOL refdevice = FALSE;

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *driver_guid,
        void **ddraw, REFIID interface_iid, IUnknown *outer);

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

static HRESULT WINAPI enum_z_fmt(DDPIXELFORMAT *fmt, void *ctx)
{
    DDPIXELFORMAT *zfmt = ctx;

    if(U1(*fmt).dwZBufferBitDepth > U1(*zfmt).dwZBufferBitDepth)
    {
        *zfmt = *fmt;
    }
    return DDENUMRET_OK;
}

static HRESULT WINAPI enum_devtype_cb(char *desc_str, char *name, D3DDEVICEDESC7 *desc, void *ctx)
{
    BOOL *hal_ok = ctx;
    if (IsEqualGUID(&desc->deviceGUID, &IID_IDirect3DTnLHalDevice))
    {
        *hal_ok = TRUE;
        return DDENUMRET_CANCEL;
    }
    return DDENUMRET_OK;
}

static BOOL createObjects(void)
{
    HRESULT hr;
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    DDPIXELFORMAT zfmt;
    BOOL hal_ok = FALSE;
    const GUID *devtype = &IID_IDirect3DHALDevice;

    if(!hmod) return FALSE;
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    if(!pDirectDrawCreateEx) return FALSE;

    hr = pDirectDrawCreateEx(NULL, (void **) &DirectDraw, &IID_IDirectDraw7, NULL);
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", hr);
    if(!DirectDraw) goto err;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d7_test_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("d3d7_test_wc", "d3d7_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
            0, 0, 640, 480, 0, 0, 0, 0);

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
    if(FAILED(hr)) goto err;

    hr = IDirect3D7_EnumDevices(Direct3D, enum_devtype_cb, &hal_ok);
    ok(SUCCEEDED(hr), "Failed to enumerate devices, hr %#x.\n", hr);
    if (hal_ok) devtype = &IID_IDirect3DTnLHalDevice;

    memset(&zfmt, 0, sizeof(zfmt));
    hr = IDirect3D7_EnumZBufferFormats(Direct3D, devtype, enum_z_fmt, &zfmt);
    if (FAILED(hr)) goto err;
    if (zfmt.dwSize == 0) goto err;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(ddsd).ddpfPixelFormat = zfmt;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &depth_buffer, NULL);
    ok(SUCCEEDED(hr), "CreateSurface failed, hr %#x.\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirectDrawSurface_AddAttachedSurface(Surface, depth_buffer);
    ok(SUCCEEDED(hr), "AddAttachedSurface failed, hr %#x.\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirect3D7_CreateDevice(Direct3D, devtype, Surface, &Direct3DDevice);
    if (FAILED(hr) || !Direct3DDevice) goto err;
    return TRUE;

    err:
    if(DirectDraw) IDirectDraw7_Release(DirectDraw);
    if (depth_buffer) IDirectDrawSurface7_Release(depth_buffer);
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
    IDirectDrawSurface7_Release(depth_buffer);
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

static void set_viewport_size(IDirect3DDevice7 *device)
{
    D3DVIEWPORT7 vp = {0};
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    IDirectDrawSurface7 *target;

    hr = IDirect3DDevice7_GetRenderTarget(device, &target);
    ok(hr == D3D_OK, "IDirect3DDevice7_GetRenderTarget returned %08x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_GetSurfaceDesc(target, &ddsd);
    ok(hr == D3D_OK, "IDirectDrawSurface7_GetSurfaceDesc returned %08x\n", hr);
    IDirectDrawSurface7_Release(target);

    vp.dwWidth = ddsd.dwWidth;
    vp.dwHeight = ddsd.dwHeight;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetViewport returned %08x\n", hr);
    return;
}

static void lighting_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    DWORD color;

    D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    struct
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
    struct
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
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice7_Clear failed with %08x\n", hr);

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
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

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    }

    color = getPixelColor(device, 160, 360); /* Lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color 0x%08x, expected 0x00ff0000.\n", color);
    color = getPixelColor(device, 160, 120); /* Upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color 0x%08x, expected 0x00000000.\n", color);
    color = getPixelColor(device, 480, 360); /* Lower right quad - unlit with normals */
    ok(color == 0x000000ff, "Unlit quad with normals has color 0x%08x, expected 0x000000ff.\n", color);
    color = getPixelColor(device, 480, 120); /* Upper right quad - lit with normals */
    ok(color == 0x00000000, "Lit quad with normals has color 0x%08x, expected 0x00000000.\n", color);

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

    struct
    {
        struct vec3 position;
        DWORD diffuse;
        DWORD specular;
    }
    /* Gets full z based fog with linear fog, no fog with specular color */
    untransformed_1[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{-1.0f,  0.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{ 0.0f, -1.0f, 0.1f}, 0xffff0000, 0xff000000},
    },
    /* Ok, I am too lazy to deal with transform matrices */
    untransformed_2[] =
    {
        {{-1.0f,  0.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{-1.0f,  1.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  1.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 1.0f}, 0xffff0000, 0xff000000},
    },
    far_quad1[] =
    {
        {{-1.0f, -1.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{-1.0f,  0.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f, -1.0f, 0.5f}, 0xffff0000, 0xff000000},
    },
    far_quad2[] =
    {
        {{-1.0f,  0.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{-1.0f,  1.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  1.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 1.5f}, 0xffff0000, 0xff000000},
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
    D3DMATRIX ident_mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    D3DMATRIX world_mat1 =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -0.5f, 1.0f,
    };
    D3DMATRIX world_mat2 =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
    };
    D3DMATRIX proj_mat =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 1.0f,
    };

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
    ok(hr == D3D_OK, "Setting fog color returned %08x\n", hr);

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog returned %08x\n", hr);

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %08x\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog end returned %08x\n", hr);

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK)
    {
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   untransformed_1, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

        /* That makes it use the Z value */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR returned %08x\n", hr);
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   untransformed_2, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "DrawIndexedPrimitive returned %08x\n", hr);

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
    ok(color_match(color, 0x00FF0000, 1), "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x0000FF00, 1), "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x00FFFF00, 1), "Transformed vertex with linear vertex fog has color %08x\n", color);
    if(caps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        color = getPixelColor(device, 480, 360);
        ok(color_match(color, 0x0000FF00, 1), "Transformed vertex with linear table fog has color %08x\n", color);
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

    if (caps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        /* A simple fog + non-identity world matrix test */
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &world_mat1);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetTransform returned %#08x\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %#08x\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
        ok(hr == D3D_OK, "Turning off vertex fog returned %#08x\n", hr);

        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "IDirect3DDevice7_Clear returned %#08x\n", hr);

        if (IDirect3DDevice7_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, far_quad1, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "DrawIndexedPrimitive returned %#08x\n", hr);

            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, far_quad2, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "DrawIndexedPrimitive returned %#08x\n", hr);

            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == D3D_OK, "EndScene returned %#08x\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00ff0000, 4), "Unfogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, 0x0000ff00, 1), "Fogged out quad has color %08x\n", color);

        /* Test fog behavior with an orthogonal (but not identity) projection matrix */
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &world_mat2);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &proj_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);

        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear returned %#08x\n", hr);

        if (IDirect3DDevice7_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, untransformed_1, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, untransformed_2, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %#08x\n", hr);

            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == D3D_OK, "EndScene returned %#08x\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00e51900, 4), "Partially fogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, 0x0000ff00, 1), "Fogged out quad has color %08x\n", color);

        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &ident_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &ident_mat);
        ok(hr == D3D_OK, "SetTransform returned %#08x\n", hr);
    }
    else
    {
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests\n");
    }

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %08x\n", hr);
}

static void blt_test(IDirect3DDevice7 *device)
{
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &offscreen, NULL);
    ok(hr == D3D_OK, "Creating the offscreen render target failed, hr = %08x\n", hr);

    /* Offscreen blits with the same source as destination */
    if(SUCCEEDED(hr))
    {
        RECT src_rect, dst_rect;

        /* Blit the whole surface to itself */
        hr = IDirectDrawSurface_Blt(offscreen, NULL, offscreen, NULL, 0, NULL);
        ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

        /* Overlapped blit */
        dst_rect.left = 0; dst_rect.right = 480;
        dst_rect.top = 0; dst_rect.bottom = 480;
        src_rect.left = 160; src_rect.right = 640;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, offscreen, &src_rect, 0, NULL);
        ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

        /* Overlapped blit, flip-y through source rectangle (not allowed) */
        dst_rect.left = 0; dst_rect.right = 480;
        dst_rect.top = 0; dst_rect.bottom = 480;
        src_rect.left = 160; src_rect.right = 640;
        src_rect.top = 480; src_rect.bottom = 0;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, offscreen, &src_rect, 0, NULL);
        ok(hr == DDERR_INVALIDRECT, "IDirectDrawSurface7_Blt returned %08x\n", hr);

        /* Overlapped blit, with shrinking in x */
        dst_rect.left = 0; dst_rect.right = 480;
        dst_rect.top = 0; dst_rect.bottom = 480;
        src_rect.left = 160; src_rect.right = 480;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, offscreen, &src_rect, 0, NULL);
        ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);
    }

    hr = IDirect3DDevice7_GetRenderTarget(device, &backbuffer);
    ok(hr == D3D_OK, "Unable to obtain a surface pointer to the backbuffer, hr = %08x\n", hr);

    /* backbuffer ==> texture blits */
    if(SUCCEEDED(hr) && offscreen)
    {
        RECT src_rect, dst_rect;

        /* backbuffer ==> texture, src_rect=NULL, dst_rect=NULL, no scaling */
        hr = IDirectDrawSurface_Blt(offscreen, NULL, backbuffer, NULL, 0, NULL);
        ok(hr == DD_OK, "fullscreen Blt from backbuffer => texture failed with hr = %08x\n", hr);

        /* backbuffer ==> texture, full surface blits, no scaling */
        dst_rect.left = 0; dst_rect.right = 640;
        dst_rect.top = 0; dst_rect.bottom = 480;
        src_rect.left = 0; src_rect.right = 640;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, backbuffer, &src_rect, 0, NULL);
        ok(hr == DD_OK, "fullscreen Blt from backbuffer => texture failed with hr = %08x\n", hr);

        /* backbuffer ==> texture, flip in y-direction through source rectangle, no scaling (allowed) */
        dst_rect.left = 0; dst_rect.right = 640;
        dst_rect.top = 480; dst_rect.top = 0;
        src_rect.left = 0; src_rect.right = 640;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, backbuffer, &src_rect, 0, NULL);
        ok(hr == DD_OK, "backbuffer => texture flip-y src_rect failed with hr = %08x\n", hr);

        /* backbuffer ==> texture, flip in x-direction through source rectangle, no scaling (not allowed) */
        dst_rect.left = 640; dst_rect.right = 0;
        dst_rect.top = 0; dst_rect.top = 480;
        src_rect.left = 0; src_rect.right = 640;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, backbuffer, &src_rect, 0, NULL);
        ok(hr == DDERR_INVALIDRECT, "backbuffer => texture flip-x src_rect failed with hr = %08x\n", hr);

        /* backbuffer ==> texture, flip in y-direction through destination rectangle (not allowed) */
        dst_rect.left = 0; dst_rect.right = 640;
        dst_rect.top = 0; dst_rect.top = 480;
        src_rect.left = 0; src_rect.right = 640;
        src_rect.top = 480; src_rect.bottom = 0;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, backbuffer, &src_rect, 0, NULL);
        ok(hr == DDERR_INVALIDRECT, "backbuffer => texture flip-y dst_rect failed with hr = %08x\n", hr);

        /* backbuffer ==> texture, flip in x-direction through destination rectangle, no scaling (not allowed) */
        dst_rect.left = 0; dst_rect.right = 640;
        dst_rect.top = 0; dst_rect.top = 480;
        src_rect.left = 640; src_rect.right = 0;
        src_rect.top = 0; src_rect.bottom = 480;
        hr = IDirectDrawSurface_Blt(offscreen, &dst_rect, backbuffer, &src_rect, 0, NULL);
        ok(hr == DDERR_INVALIDRECT, "backbuffer => texture flip-x dst_rect failed with hr = %08x\n", hr);
    }

    if(offscreen) IDirectDrawSurface7_Release(offscreen);
    if(backbuffer) IDirectDrawSurface7_Release(backbuffer);
}

static void offscreen_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    DWORD color;
    DDSURFACEDESC2 ddsd;

    static float quad[][5] = {
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

    if (refdevice) {
        win_skip("Tests would crash on W2K with a refdevice\n");
        goto out;
    }

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK) {
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        set_viewport_size(device);
        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Clear failed, hr = %08x\n", hr);

        /* Draw without textures - Should result in a white quad */
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
        ok(hr == D3D_OK, "DrawPrimitive failed, hr = %08x\n", hr);

        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(hr == D3D_OK, "SetRenderTarget failed, hr = %08x\n", hr);
        set_viewport_size(device);

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
    ok(SUCCEEDED(hr), "IDirect3DDevice7_SetTexture returned %#x.\n", hr);

    /* restore things */
    if(backbuffer) {
        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(SUCCEEDED(hr), "IDirect3DDevice7_SetRenderTarget returned %#x.\n", hr);
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

    struct
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
    static float composite_quad[][5] = {
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

    if (refdevice) {
        win_skip("Tests would crash on W2K with a refdevice\n");
        goto out;
    }

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
         * has an alpha channel on its own. Clear the offscreen buffer with alpha = 0.5 again, then draw the
         * quads again. The SRCALPHA/INVSRCALPHA doesn't give any surprises, but the DESTALPHA/INVDESTALPHA
         * blending works as supposed now - blend factor is 0.5 in both cases, not 0.75 as from the input
         * vertices
         */
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderTarget failed, hr = %08x\n", hr);
        set_viewport_size(device);
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
        ok(hr == D3D_OK, "IDirect3DDevice7_SetRenderTarget failed, hr = %08x\n", hr);
        set_viewport_size(device);

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
       "SRCALPHA on frame buffer returned color 0x%08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 160, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0x00 && green == 0x00 && blue >= 0xfe && blue <= 0xff ,
       "DSTALPHA on frame buffer returned color 0x%08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 480, 360);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0xbe && red <= 0xc0 && green >= 0x39 && green <= 0x41 && blue == 0x00,
       "SRCALPHA on texture returned color 0x%08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 480, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x81 && green == 0x00 && blue >= 0x7e && blue <= 0x81,
       "DSTALPHA on texture returned color 0x%08x, expected 0x00800080\n", color);

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
    ok(color == 0xffffff ||
       broken(color == 0), /* VMware */
       "Got color %08x, expected 00ffffff\n", color);

    color = getPixelColor(device, 105, 105);
    ok(color == 0, "Got color %08x, expected 00000000\n", color);
}

static BOOL D3D1_createObjects(void)
{
    WNDCLASSA wc = {0};
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

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "texturemapblend_test_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("texturemapblend_test_wc", "texturemapblend_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

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

static HRESULT CALLBACK TextureFormatEnumCallback(DDSURFACEDESC *lpDDSD, void *lpContext)
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

    exe_buffer_ptr = sizeof(test1_quads) + (char*)exdesc.lpData;

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

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - sizeof(test1_quads);

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(exdata));
    exdata.dwSize = sizeof(exdata);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = sizeof(test1_quads);
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
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

    exe_buffer_ptr = sizeof(test1_quads) + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(1, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - sizeof(test1_quads);

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(exdata));
    exdata.dwSize = sizeof(exdata);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = sizeof(test1_quads);
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
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

    exe_buffer_ptr = sizeof(test2_quads) + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - sizeof(test2_quads);

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(exdata));
    exdata.dwSize = sizeof(exdata);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = sizeof(test2_quads);
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
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

    exe_buffer_ptr = sizeof(test1_quads) + (char*)exdesc.lpData;

    EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

    EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE, exe_buffer_ptr);
    hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
    ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
    EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

    EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
    EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

    EXEBUF_END(exe_buffer_ptr);

    exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - sizeof(test1_quads);

    hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
    if (FAILED(hr)) {
        trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
    }

    memset(&exdata, 0, sizeof(exdata));
    exdata.dwSize = sizeof(exdata);
    exdata.dwVertexCount = 8;
    exdata.dwInstructionOffset = sizeof(test1_quads);
    exdata.dwInstructionLength = exe_length;
    hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
        ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
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

        exe_buffer_ptr = sizeof(test1_quads) + (char*)exdesc.lpData;

        EXEBUF_PUT_PROCESSVERTICES(8, exe_buffer_ptr);

        EXEBUF_START_RENDER_STATES(2, exe_buffer_ptr);
        EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE, exe_buffer_ptr);
        hr = IDirect3DTexture_GetHandle(Texture, Direct3DDevice1, &htex);
        ok(hr == D3D_OK, "IDirect3DTexture_GetHandle failed with %08x\n", hr);
        EXEBUF_PUT_RENDER_STATE(D3DRENDERSTATE_TEXTUREHANDLE,  htex, exe_buffer_ptr);

        EXEBUF_PUT_QUAD(0, exe_buffer_ptr);
        EXEBUF_PUT_QUAD(4, exe_buffer_ptr);

        EXEBUF_END(exe_buffer_ptr);

        exe_length = ((char*)exe_buffer_ptr - (char*)exdesc.lpData) - sizeof(test1_quads);

        hr = IDirect3DExecuteBuffer_Unlock(ExecuteBuffer);
        if (FAILED(hr)) {
            trace("IDirect3DExecuteBuffer_Unlock failed with %08x\n", hr);
        }

        memset(&exdata, 0, sizeof(exdata));
        exdata.dwSize = sizeof(exdata);
        exdata.dwVertexCount = 8;
        exdata.dwInstructionOffset = sizeof(test1_quads);
        exdata.dwInstructionLength = exe_length;
        hr = IDirect3DExecuteBuffer_SetExecuteData(ExecuteBuffer, &exdata);
        ok(hr == D3D_OK, "IDirect3DExecuteBuffer_SetExecuteData failed with %08x\n", hr);

        hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

        if (SUCCEEDED(hr)) {
            hr = IDirect3DDevice_Execute(Direct3DDevice1, ExecuteBuffer, Viewport, D3DEXECUTE_UNCLIPPED);
            ok(hr == D3D_OK, "IDirect3DDevice_Execute failed, hr = %08x\n", hr);
            hr = IDirect3DDevice_EndScene(Direct3DDevice1);
            ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
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

static void D3D1_ViewportClearTest(void)
{
    HRESULT hr;
    IDirect3DMaterial *bgMaterial = NULL;
    D3DMATERIAL mat;
    D3DMATERIALHANDLE hMat;
    D3DVIEWPORT vp_data;
    IDirect3DViewport *Viewport2 = NULL;
    DWORD color, red, green, blue;

    hr = IDirect3D_CreateMaterial(Direct3D1, &bgMaterial, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateMaterial failed: %08x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    hr = IDirect3D_CreateViewport(Direct3D1, &Viewport2, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateViewport failed: %08x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    hr = IDirect3DViewport_Initialize(Viewport2, Direct3D1);
    ok(hr == D3D_OK || hr == DDERR_ALREADYINITIALIZED, "IDirect3DViewport_Initialize returned %08x\n", hr);
    hr = IDirect3DDevice_AddViewport(Direct3DDevice1, Viewport2);
    ok(hr == D3D_OK, "IDirect3DDevice_AddViewport returned %08x\n", hr);
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwX = 200;
    vp_data.dwY = 200;
    vp_data.dwWidth = 100;
    vp_data.dwHeight = 100;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    vp_data.dvMaxX = 100;
    vp_data.dvMaxY = 100;
    vp_data.dvMinZ = 0;
    vp_data.dvMaxZ = 1;
    hr = IDirect3DViewport_SetViewport(Viewport2, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U(mat).diffuse).r = 1.0f;
    hr = IDirect3DMaterial_SetMaterial(bgMaterial, &mat);
    ok(hr == D3D_OK, "IDirect3DMaterial_SetMaterial failed: %08x\n", hr);

    hr = IDirect3DMaterial_GetHandle(bgMaterial, Direct3DDevice1, &hMat);
    ok(hr == D3D_OK, "IDirect3DMaterial_GetHandle failed: %08x\n", hr);

    hr = IDirect3DViewport_SetBackground(Viewport, hMat);
    ok(hr == D3D_OK, "IDirect3DViewport_SetBackground failed: %08x\n", hr);
    hr = IDirect3DViewport_SetBackground(Viewport2, hMat);
    ok(hr == D3D_OK, "IDirect3DViewport_SetBackground failed: %08x\n", hr);

    hr = IDirect3DDevice_BeginScene(Direct3DDevice1);
    ok(hr == D3D_OK, "IDirect3DDevice_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        D3DRECT rect;

        U1(rect).x1 = U2(rect).y1 = 0;
        U3(rect).x2 = 640;
        U4(rect).y2 = 480;

        hr = IDirect3DViewport_Clear(Viewport,  1,  &rect, D3DCLEAR_TARGET);
        ok(hr == D3D_OK, "IDirect3DViewport_Clear failed: %08x\n", hr);

        memset(&mat, 0, sizeof(mat));
        mat.dwSize = sizeof(mat);
        U3(U(mat).diffuse).b = 1.0f;
        hr = IDirect3DMaterial_SetMaterial(bgMaterial, &mat);
        ok(hr == D3D_OK, "IDirect3DMaterial_SetMaterial failed: %08x\n", hr);

        hr = IDirect3DViewport_Clear(Viewport2,  1,  &rect, D3DCLEAR_TARGET);
        ok(hr == D3D_OK, "IDirect3DViewport_Clear failed: %08x\n", hr);

        hr = IDirect3DDevice_EndScene(Direct3DDevice1);
        ok(hr == D3D_OK, "IDirect3DDevice_EndScene failed, hr = %08x\n", hr);
        }

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok((red == 0xff && green == 0 && blue == 0) ||
       broken(red == 0 && green == 0 && blue == 0xff), /* VMware and some native boxes */
       "Got color %08x, expected 00ff0000\n", color);

    color = D3D1_getPixelColor(DirectDraw1, Surface1, 205, 205);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 && green == 0 && blue == 0xff, "Got color %08x, expected 000000ff\n", color);

    out:

    if (bgMaterial) IDirect3DMaterial_Release(bgMaterial);
    if (Viewport2) IDirect3DViewport_Release(Viewport2);
}

static DWORD D3D3_getPixelColor(IDirectDraw4 *DirectDraw, IDirectDrawSurface4 *Surface, UINT x, UINT y)
{
    DWORD ret;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    RECT rectToLock = {x, y, x+1, y+1};
    IDirectDrawSurface4 *surf = NULL;

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
    hr = IDirectDraw4_CreateSurface(DirectDraw, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface failed with %08x\n", hr);
    if(!surf)
    {
        trace("cannot create helper surface\n");
        return 0xdeadbeef;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);

    hr = IDirectDrawSurface4_BltFast(surf, 0, 0, Surface, NULL, 0);
    ok(hr == DD_OK, "IDirectDrawSurface_BltFast returned %08x\n", hr);
    if(FAILED(hr))
    {
        trace("Cannot blit\n");
        ret = 0xdeadbee;
        goto out;
    }

    hr = IDirectDrawSurface4_Lock(surf, &rectToLock, &ddsd, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface4_Unlock(surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%08x\n", hr);
    }

out:
    IDirectDrawSurface4_Release(surf);
    return ret;
}

static void D3D3_ViewportClearTest(void)
{
    HRESULT hr;
    IDirectDraw *DirectDraw1 = NULL;
    IDirectDraw4 *DirectDraw4 = NULL;
    IDirectDrawSurface4 *Primary = NULL;
    IDirect3D3 *Direct3D3 = NULL;
    IDirect3DViewport3 *Viewport3 = NULL;
    IDirect3DViewport3 *SmallViewport3 = NULL;
    IDirect3DDevice3 *Direct3DDevice3 = NULL;
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    D3DVIEWPORT2 vp_data;
    DWORD color, red, green, blue;
    D3DRECT rect;
    D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffffffff},
        {{-1.0f,  1.0f, 0.1f}, 0xffffffff},
        {{ 1.0f,  1.0f, 0.1f}, 0xffffffff},
        {{ 1.0f, -1.0f, 0.1f}, 0xffffffff},
    };

    WORD Indices[] = {0, 1, 2, 2, 3, 0};
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "D3D3_ViewportClearTest_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("D3D3_ViewportClearTest_wc", "D3D3_ViewportClearTest",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

    hr = DirectDrawCreate( NULL, &DirectDraw1, NULL );
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 32);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 24);
    }
    ok(hr==DD_OK || hr == DDERR_UNSUPPORTED, "SetDisplayMode returned: %x\n", hr);
    if (FAILED(hr)) goto out;

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw4, (void**)&DirectDraw4);
    ok(hr==DD_OK, "QueryInterface returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags    = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;

    hr = IDirectDraw_CreateSurface(DirectDraw4, &ddsd, &Primary, NULL);
    ok(hr==DD_OK, "IDirectDraw_CreateSurface returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw4_QueryInterface(DirectDraw4, &IID_IDirect3D3, (void**)&Direct3D3);
    ok(hr==DD_OK, "IDirectDraw4_QueryInterface returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3D3_CreateDevice(Direct3D3, &IID_IDirect3DHALDevice, Primary, &Direct3DDevice3, NULL);
    if(FAILED(hr)) {
        trace("Creating a HAL device failed, trying Ref\n");
        hr = IDirect3D3_CreateDevice(Direct3D3, &IID_IDirect3DRefDevice, Primary, &Direct3DDevice3, NULL);
    }
    ok(hr==D3D_OK, "Creating 3D device returned: %x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3D3_CreateViewport(Direct3D3, &Viewport3, NULL);
    ok(hr==DD_OK, "IDirect3D3_CreateViewport returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3DDevice3_AddViewport(Direct3DDevice3, Viewport3);
    ok(hr==DD_OK, "IDirect3DDevice3_AddViewport returned: %08x\n", hr);

    memset(&vp_data, 0, sizeof(D3DVIEWPORT2));
    vp_data.dwSize = sizeof(D3DVIEWPORT2);
    vp_data.dwWidth = 640;
    vp_data.dwHeight = 480;
    vp_data.dvClipX = -1.0f;
    vp_data.dvClipWidth = 2.0f;
    vp_data.dvClipY = 1.0f;
    vp_data.dvClipHeight = 2.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(Viewport3, &vp_data);
    ok(hr==DD_OK, "IDirect3DViewport3_SetViewport2 returned: %08x\n", hr);

    hr = IDirect3D3_CreateViewport(Direct3D3, &SmallViewport3, NULL);
    ok(hr==DD_OK, "IDirect3D3_CreateViewport returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3DDevice3_AddViewport(Direct3DDevice3, SmallViewport3);
    ok(hr==DD_OK, "IDirect3DDevice3_AddViewport returned: %08x\n", hr);

    memset(&vp_data, 0, sizeof(D3DVIEWPORT2));
    vp_data.dwSize = sizeof(D3DVIEWPORT2);
    vp_data.dwX = 400;
    vp_data.dwY = 100;
    vp_data.dwWidth = 100;
    vp_data.dwHeight = 100;
    vp_data.dvClipX = -1.0f;
    vp_data.dvClipWidth = 2.0f;
    vp_data.dvClipY = 1.0f;
    vp_data.dvClipHeight = 2.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(SmallViewport3, &vp_data);
    ok(hr==DD_OK, "IDirect3DViewport3_SetViewport2 returned: %08x\n", hr);

    hr = IDirect3DDevice3_BeginScene(Direct3DDevice3);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetTransform returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState failed with %08x\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice3_SetRenderState returned %08x\n", hr);

    if (SUCCEEDED(hr)) {
        U1(rect).x1 = U2(rect).y1 = 0;
        U3(rect).x2 = 640;
        U4(rect).y2 = 480;

        hr = IDirect3DViewport3_Clear2(Viewport3, 1, &rect, D3DCLEAR_TARGET, 0x00ff00, 0.0f, 0);
        ok(hr == D3D_OK, "IDirect3DViewport3_Clear2 failed, hr = %08x\n", hr);

        hr = IDirect3DViewport3_Clear2(SmallViewport3, 1, &rect, D3DCLEAR_TARGET, 0xff0000, 0.0f, 0);
        ok(hr == D3D_OK, "IDirect3DViewport3_Clear2 failed, hr = %08x\n", hr);

        hr = IDirect3DDevice3_EndScene(Direct3DDevice3);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
        }

    color = D3D3_getPixelColor(DirectDraw4, Primary, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 && green == 0xff && blue == 0, "Got color %08x, expected 0000ff00\n", color);

    color = D3D3_getPixelColor(DirectDraw4, Primary, 405, 105);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff && green == 0 && blue == 0, "Got color %08x, expected 00ff0000\n", color);

    /* Test that clearing viewport doesn't interfere with rendering to previously active viewport. */
    hr = IDirect3DDevice3_BeginScene(Direct3DDevice3);
    ok(hr == D3D_OK, "IDirect3DDevice3_BeginScene failed with %08x\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice3_SetCurrentViewport(Direct3DDevice3, SmallViewport3);
        ok(hr == D3D_OK, "IDirect3DDevice3_SetCurrentViewport failed with %08x\n", hr);

        hr = IDirect3DViewport3_Clear2(Viewport3, 1, &rect, D3DCLEAR_TARGET, 0x000000, 0.0f, 0);
        ok(hr == D3D_OK, "IDirect3DViewport3_Clear2 failed, hr = %08x\n", hr);

        hr = IDirect3DDevice3_DrawIndexedPrimitive(Direct3DDevice3, D3DPT_TRIANGLELIST, fvf, quad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "IDirect3DDevice3_DrawIndexedPrimitive failed with %08x\n", hr);

        hr = IDirect3DDevice3_EndScene(Direct3DDevice3);
        ok(hr == D3D_OK, "IDirect3DDevice3_EndScene failed, hr = %08x\n", hr);
        }

    color = D3D3_getPixelColor(DirectDraw4, Primary, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 && green == 0 && blue == 0, "Got color %08x, expected 00000000\n", color);

    color = D3D3_getPixelColor(DirectDraw4, Primary, 405, 105);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff && green == 0xff && blue == 0xff, "Got color %08x, expected 00ffffff\n", color);

    out:

    if (SmallViewport3) IDirect3DViewport3_Release(SmallViewport3);
    if (Viewport3) IDirect3DViewport3_Release(Viewport3);
    if (Direct3DDevice3) IDirect3DDevice3_Release(Direct3DDevice3);
    if (Direct3D3) IDirect3D3_Release(Direct3D3);
    if (Primary) IDirectDrawSurface4_Release(Primary);
    if (DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if (DirectDraw4) IDirectDraw4_Release(DirectDraw4);
    if(window) DestroyWindow(window);
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

    p = (BYTE *)ddsd.lpSurface + U1(ddsd).lPitch * y + x;

    for (i = 0; i < h; i++) {
        for (i1 = 0; i1 < w; i1++) {
            p[i1] = colorindex;
        }
        p += U1(ddsd).lPitch;
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

static void p8_primary_test(void)
{
    /* Test 8bit mode used by games like StarCraft, C&C Red Alert I etc */
    DDSURFACEDESC ddsd;
    HDC hdc;
    HRESULT hr;
    PALETTEENTRY entries[256];
    RGBQUAD coltable[256];
    UINT i, i1, i2;
    IDirectDrawPalette *ddprimpal = NULL;
    IDirectDrawSurface *offscreen = NULL;
    WNDCLASSA wc = {0};
    DDBLTFX ddbltfx;
    COLORREF color;
    RECT rect;
    DDCOLORKEY clrKey;
    unsigned differences;

    /* An IDirect3DDevice cannot be queryInterfaced from an IDirect3DDevice7 on windows */
    hr = DirectDrawCreate(NULL, &DirectDraw1, NULL);

    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "p8_primary_test_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("p8_primary_test_wc", "p8_primary_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

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

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = 1;
    hr = IDirectDrawSurface_Blt(Surface1, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt failed with %08x\n", hr);

    color = getPixelColor_GDI(Surface1, 10, 10);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0xFF && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R 00 G FF B 00\n",
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
    ok(hr == DD_OK ||
       broken(hr == DDERR_INVALIDPIXELFORMAT) || /* VMware */
       broken(hr == DDERR_NODIRECTDRAWHW), /* VMware */
       "IDirectDraw_CreateSurface returned %08x\n", hr);
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

    p8_surface_fill_rect(offscreen, 0, 0, 16, 16, 2);

    memset(entries, 0, sizeof(entries));
    entries[0].peRed = 0xff;
    entries[1].peGreen = 0xff;
    entries[2].peBlue = 0xff;
    entries[3].peRed = 0x80;
    hr = IDirectDrawPalette_SetEntries(ddprimpal, 0, 0, 256, entries);
    ok(hr == DD_OK, "IDirectDrawPalette_SetEntries failed with %08x\n", hr);

    hr = IDirectDrawSurface_BltFast(Surface1, 0, 0, offscreen, NULL, 0);
    ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

    color = getPixelColor_GDI(Surface1, 1, 1);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0x00 && GetBValue(color) == 0xFF,
            "got R %02X G %02X B %02X, expected R 00 G 00 B FF\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    /* Color keyed blit. */
    p8_surface_fill_rect(offscreen, 0, 0, 8, 8, 3);
    clrKey.dwColorSpaceLowValue = 3;
    clrKey.dwColorSpaceHighValue = 3;
    hr = IDirectDrawSurface_SetColorKey(offscreen, DDCKEY_SRCBLT, &clrKey);
    ok(hr==D3D_OK, "IDirectDrawSurfac_SetColorKey returned: %x\n", hr);

    hr = IDirectDrawSurface_BltFast(Surface1, 100, 100, offscreen, NULL, DDBLTFAST_SRCCOLORKEY);
    ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

    color = getPixelColor_GDI(Surface1, 105, 105);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0xFF && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R 00 G FF B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    color = getPixelColor_GDI(Surface1, 112, 112);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0x00 && GetBValue(color) == 0xFF,
            "got R %02X G %02X B %02X, expected R 00 G 00 B FF\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    rect.left = 100;
    rect.top = 200;
    rect.right = 116;
    rect.bottom = 216;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.ddckSrcColorkey.dwColorSpaceLowValue = ddbltfx.ddckSrcColorkey.dwColorSpaceHighValue = 2;
    hr = IDirectDrawSurface_Blt(Surface1, &rect, offscreen, NULL,
        DDBLT_WAIT | DDBLT_KEYSRC | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
    ok(hr==DDERR_INVALIDPARAMS, "IDirectDrawSurface_Blt returned: %x\n", hr);
    hr = IDirectDrawSurface_Blt(Surface1, &rect, offscreen, NULL,
        DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &ddbltfx);
    ok(hr==DD_OK, "IDirectDrawSurface_Blt returned: %x\n", hr);

    color = getPixelColor_GDI(Surface1, 105, 205);
    ok(GetRValue(color) == 0x80 && GetGValue(color) == 0 && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R 80 G 00 B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    color = getPixelColor_GDI(Surface1, 112, 212);
    ok(GetRValue(color) == 0 && GetGValue(color) == 0xFF && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R 00 G FF B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    /* Test blitting and locking patterns that are likely to trigger bugs in opengl renderer (p8
       surface conversion and uploading/downloading to/from opengl texture). Similar patterns (
       blitting front buffer areas to/from an offscreen surface mixed with locking) are used by C&C
       Red Alert I. */
    IDirectDrawSurface_Release(offscreen);

    memset (&ddsd, 0, sizeof (ddsd));
    ddsd.dwSize = sizeof (ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 8;
    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &offscreen, NULL);
    ok(hr == DD_OK, "IDirectDraw_CreateSurface returned %08x\n", hr);

    if (FAILED(hr)) goto out;

    /* Test two times, first time front buffer has a palette and second time front buffer
       has no palette; the latter is somewhat contrived example, but an app could set
       front buffer palette later. */
    for (i2 = 0; i2 < 2; i2++) {
        if (i2 == 1) {
            hr = IDirectDrawSurface_SetPalette(Surface1, NULL);
            ok(hr==DD_OK, "IDirectDrawSurface_SetPalette returned: %x\n", hr);
        }

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(Surface1, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_Lock returned: %x\n", hr);

        for (i = 0; i < 256; i++) {
            unsigned x = (i % 128) * 4;
            unsigned y = (i / 128) * 4;
            BYTE *p = (BYTE *)ddsd.lpSurface + U1(ddsd).lPitch * y + x;

            for (i1 = 0; i1 < 4; i1++) {
                p[0] = p[1] = p[2] = p[3] = i;
                p += U1(ddsd).lPitch;
            }
        }

        hr = IDirectDrawSurface_Unlock(Surface1, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_UnLock returned: %x\n", hr);

        hr = IDirectDrawSurface_BltFast(offscreen, 0, 0, Surface1, NULL, 0);
        ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

        /* This ensures offscreen surface contents will be downloaded to system memory. */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(offscreen, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_Lock returned: %x\n", hr);
        hr = IDirectDrawSurface_Unlock(offscreen, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_UnLock returned: %x\n", hr);

        /* Offscreen surface data will have to be converted and uploaded to texture. */
        rect.left = 0;
        rect.top = 0;
        rect.right = 16;
        rect.bottom = 16;
        hr = IDirectDrawSurface_BltFast(offscreen, 600, 400, Surface1, &rect, 0);
        ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

        /* This ensures offscreen surface contents will be downloaded to system memory. */
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(offscreen, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_Lock returned: %x\n", hr);
        hr = IDirectDrawSurface_Unlock(offscreen, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_UnLock returned: %x\n", hr);

        hr = IDirectDrawSurface_BltFast(Surface1, 0, 0, offscreen, NULL, 0);
        ok(hr==DD_OK, "IDirectDrawSurface_BltFast returned: %x\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(Surface1, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_Lock returned: %x\n", hr);

        differences = 0;

        for (i = 0; i < 256; i++) {
            unsigned x = (i % 128) * 4 + 1;
            unsigned y = (i / 128) * 4 + 1;
            BYTE *p = (BYTE *)ddsd.lpSurface + U1(ddsd).lPitch * y + x;

            if (*p != i) differences++;
        }

        hr = IDirectDrawSurface_Unlock(Surface1, NULL);
        ok(hr==DD_OK, "IDirectDrawSurface_UnLock returned: %x\n", hr);

        ok(differences == 0, i2 == 0 ? "Pass 1. Unexpected front buffer contents after blit (%u differences)\n" :
                "Pass 2 (with NULL front buffer palette). Unexpected front buffer contents after blit (%u differences)\n",
                differences);
    }

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
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES | DDSCAPS2_TEXTUREMANAGE;
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00FF0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000FF00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000FF;

    hr = IDirectDraw7_CreateSurface(ddraw, &ddsd, &cubemap, NULL);
    ok(hr == DD_OK, "IDirectDraw7_CreateSurface returned %08x\n", hr);
    IDirectDraw7_Release(ddraw);

    /* Positive X */
    U5(DDBltFx).dwFillColor = 0x00ff0000;
    hr = IDirectDrawSurface7_Blt(cubemap, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_TEXTURE;
    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    U5(DDBltFx).dwFillColor = 0x0000ffff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    U5(DDBltFx).dwFillColor = 0x0000ff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    U5(DDBltFx).dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    U5(DDBltFx).dwFillColor = 0x00ffff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "IDirectDrawSurface7_Blt returned %08x\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "IDirectDrawSurface7_Lock returned %08x\n", hr);
    U5(DDBltFx).dwFillColor = 0x00ff00ff;
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
        if (hr == DDERR_UNSUPPORTED || hr == DDERR_NODIRECTDRAWHW)
        {
            /* VMware */
            win_skip("IDirect3DDevice7_DrawPrimitive is not completely implemented, colors won't be tested\n");
            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == DD_OK, "IDirect3DDevice7_EndScene returned %08x\n", hr);
            goto out;
        }
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

out:
    hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
    ok(hr == DD_OK, "IDirect3DDevice7_SetTexture returned %08x\n", hr);
    IDirectDrawSurface7_Release(cubemap);
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
static void depth_clamp_test(IDirect3DDevice7 *device)
{
    struct
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
    struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad5[] =
    {
        {{-0.5f,  0.5f, 10.0f}, 0xff14f914},
        {{ 0.5f,  0.5f, 10.0f}, 0xff14f914},
        {{-0.5f, -0.5f, 10.0f}, 0xff14f914},
        {{ 0.5f, -0.5f, 10.0f}, 0xff14f914},
    },
    quad6[] =
    {
        {{-1.0f, 0.5f,  10.0f}, 0xfff91414},
        {{ 1.0f, 0.5f,  10.0f}, 0xfff91414},
        {{-1.0f, 0.25f, 10.0f}, 0xfff91414},
        {{ 1.0f, 0.25f, 10.0f}, 0xfff91414},
    };

    D3DVIEWPORT7 vp;
    D3DCOLOR color;
    HRESULT hr;

    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 640;
    vp.dwHeight = 480;
    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 7.5;

    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad1, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);
    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad2, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad3, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);
    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad4, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad5, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "SetRenderState failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad6, 4, 0);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice7_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    color = getPixelColor(device, 75, 75);
    ok(color_match(color, 0x00ffffff, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 150);
    ok(color_match(color, 0x00ffffff, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00002b7f, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 330);
    ok(color_match(color, 0x00f9e814, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 330);
    ok(color_match(color, 0x00f9e814, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);

    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 1.0;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);
}

static void DX1_BackBufferFlipTest(void)
{
    HRESULT hr;
    IDirectDraw *DirectDraw1 = NULL;
    IDirectDrawSurface *Primary = NULL;
    IDirectDrawSurface *Backbuffer = NULL;
    WNDCLASSA wc = {0};
    DDSURFACEDESC ddsd;
    DDBLTFX ddbltfx;
    COLORREF color;
    const DWORD white = 0xffffff;
    const DWORD red = 0xff0000;
    BOOL attached = FALSE;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "DX1_BackBufferFlipTest_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("DX1_BackBufferFlipTest_wc", "DX1_BackBufferFlipTest",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

    hr = DirectDrawCreate( NULL, &DirectDraw1, NULL );
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 32);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 24);
    }
    ok(hr==DD_OK || hr == DDERR_UNSUPPORTED, "SetDisplayMode returned: %x\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Primary, NULL);
    ok(hr==DD_OK, "IDirectDraw_CreateSurface returned: %08x\n", hr);

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount      = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask         = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask         = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask         = 0x000000ff;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Backbuffer, NULL);
    ok(hr==DD_OK, "IDirectDraw_CreateSurface returned: %08x\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDrawSurface_AddAttachedSurface(Primary, Backbuffer);
    todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE),
       "Attaching a back buffer to a front buffer returned %08x\n", hr);
    if (FAILED(hr)) goto out;

    attached = TRUE;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    U5(ddbltfx).dwFillColor = red;
    hr = IDirectDrawSurface_Blt(Backbuffer, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt returned: %x\n", hr);

    U5(ddbltfx).dwFillColor = white;
    hr = IDirectDrawSurface_Blt(Primary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "IDirectDrawSurface_Blt returned: %x\n", hr);

    /* Check it out */
    color = getPixelColor_GDI(Primary, 5, 5);
    ok(GetRValue(color) == 0xFF && GetGValue(color) == 0xFF && GetBValue(color) == 0xFF,
            "got R %02X G %02X B %02X, expected R FF G FF B FF\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    color = getPixelColor_GDI(Backbuffer, 5, 5);
    ok(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R FF G 00 B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    hr = IDirectDrawSurface_Flip(Primary, NULL, DDFLIP_WAIT);
    todo_wine ok(hr == DD_OK, "IDirectDrawSurface_Flip returned 0x%08x\n", hr);

    if (hr == DD_OK)
    {
        color = getPixelColor_GDI(Primary, 5, 5);
        ok(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0,
                "got R %02X G %02X B %02X, expected R FF G 00 B 00\n",
                GetRValue(color), GetGValue(color), GetBValue(color));

        color = getPixelColor_GDI(Backbuffer, 5, 5);
        ok((GetRValue(color) == 0xFF && GetGValue(color) == 0xFF && GetBValue(color) == 0xFF) ||
           broken(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0),  /* broken driver */
                "got R %02X G %02X B %02X, expected R FF G FF B FF\n",
                GetRValue(color), GetGValue(color), GetBValue(color));
    }

    out:

    if (Backbuffer)
    {
        if (attached)
            IDirectDrawSurface_DeleteAttachedSurface(Primary, 0, Backbuffer);
        IDirectDrawSurface_Release(Backbuffer);
    }
    if (Primary) IDirectDrawSurface_Release(Primary);
    if (DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if (window) DestroyWindow(window);
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
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 1, 1);
    if(color !=0x00ff0000)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice7_Clear(Direct3DDevice, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 639, 479);
    if(color != 0x0000ddee)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now run the tests */
    blt_test(Direct3DDevice);
    depth_clamp_test(Direct3DDevice);
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
        D3D1_ViewportClearTest();
    }
    D3D1_releaseObjects();

    D3D3_ViewportClearTest();
    p8_primary_test();
    DX1_BackBufferFlipTest();

    return ;

cleanup:
    releaseObjects();
}
