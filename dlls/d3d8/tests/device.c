/*
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2006 Chris Robinson
 * Copyright (C) 2006 Louis Lenders
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2006-2007, 2011-2013 Stefan Dösinger for CodeWeavers
 * Copyright 2013 Henri Verbeet for CodeWeavers
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
#include <initguid.h>
#include <d3d8.h>
#include "wine/test.h"

struct vec3
{
    float x, y, z;
};

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

static INT screen_width;
static INT screen_height;

static HRESULT (WINAPI *ValidateVertexShader)(DWORD *, DWORD *, DWORD *, int, DWORD *);
static HRESULT (WINAPI *ValidatePixelShader)(DWORD *, DWORD *, int, DWORD *);

static BOOL (WINAPI *pGetCursorInfo)(PCURSORINFO);

static const DWORD simple_vs[] = {0xFFFE0101,       /* vs_1_1               */
    0x00000009, 0xC0010000, 0x90E40000, 0xA0E40000, /* dp4 oPos.x, v0, c0   */
    0x00000009, 0xC0020000, 0x90E40000, 0xA0E40001, /* dp4 oPos.y, v0, c1   */
    0x00000009, 0xC0040000, 0x90E40000, 0xA0E40002, /* dp4 oPos.z, v0, c2   */
    0x00000009, 0xC0080000, 0x90E40000, 0xA0E40003, /* dp4 oPos.w, v0, c3   */
    0x0000FFFF};                                    /* END                  */
static const DWORD simple_ps[] = {0xFFFF0101,                               /* ps_1_1                       */
    0x00000051, 0xA00F0001, 0x3F800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
    0x00000042, 0xB00F0000,                                                 /* tex t0                       */
    0x00000008, 0x800F0000, 0xA0E40001, 0xA0E40000,                         /* dp3 r0, c1, c0               */
    0x00000005, 0x800F0000, 0x90E40000, 0x80E40000,                         /* mul r0, v0, r0               */
    0x00000005, 0x800F0000, 0xB0E40000, 0x80E40000,                         /* mul r0, t0, r0               */
    0x0000FFFF};                                                            /* END                          */

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static IDirect3DDevice8 *create_device(IDirect3D8 *d3d8, HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice8 *device;

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = screen_width;
    present_parameters.BackBufferHeight = screen_height;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (SUCCEEDED(IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    if (SUCCEEDED(IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    return NULL;
}

static HRESULT reset_device(IDirect3DDevice8 *device, HWND device_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = screen_width;
    present_parameters.BackBufferHeight = screen_height;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    return IDirect3DDevice8_Reset(device, &present_parameters);
}

#define CHECK_CALL(r,c,d,rc) \
    if (SUCCEEDED(r)) {\
        int tmp1 = get_refcount( (IUnknown *)d ); \
        int rc_new = rc; \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    } else {\
        trace("%s failed: %#08x\n", c, r); \
    }

#define CHECK_RELEASE(obj,d,rc) \
    if (obj) { \
        int tmp1, rc_new = rc; \
        IUnknown_Release( (IUnknown*)obj ); \
        tmp1 = get_refcount( (IUnknown *)d ); \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    }

#define CHECK_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = get_refcount( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_RELEASE_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = IUnknown_Release( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_ADDREF_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = IUnknown_AddRef( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_SURFACE_CONTAINER(obj,iid,expected) \
    { \
        void *container_ptr = (void *)0x1337c0d3; \
        hr = IDirect3DSurface8_GetContainer(obj, &iid, &container_ptr); \
        ok(SUCCEEDED(hr) && container_ptr == expected, "GetContainer returned: hr %#08x, container_ptr %p. " \
            "Expected hr %#08x, container_ptr %p\n", hr, container_ptr, S_OK, expected); \
        if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr); \
    }

static void check_mipmap_levels(IDirect3DDevice8 *device, UINT width, UINT height, UINT count)
{
    IDirect3DBaseTexture8* texture = NULL;
    HRESULT hr = IDirect3DDevice8_CreateTexture( device, width, height, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DTexture8**) &texture );

    if (SUCCEEDED(hr)) {
        DWORD levels = IDirect3DBaseTexture8_GetLevelCount(texture);
        ok(levels == count, "Invalid level count. Expected %d got %u\n", count, levels);
    } else
        trace("CreateTexture failed: %#08x\n", hr);

    if (texture) IDirect3DBaseTexture8_Release( texture );
}

static void test_mipmap_levels(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    check_mipmap_levels(device, 32, 32, 6);
    check_mipmap_levels(device, 256, 1, 9);
    check_mipmap_levels(device, 1, 256, 9);
    check_mipmap_levels(device, 1, 1, 1);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_swapchain(void)
{
    IDirect3DSwapChain8 *swapchain1;
    IDirect3DSwapChain8 *swapchain2;
    IDirect3DSwapChain8 *swapchain3;
    IDirect3DSurface8 *backbuffer;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;

    /* Create a bunch of swapchains */
    d3dpp.BackBufferCount = 0;
    hr = IDirect3DDevice8_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%#08x)\n", hr);
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    d3dpp.BackBufferCount  = 1;
    hr = IDirect3DDevice8_CreateAdditionalSwapChain(device, &d3dpp, &swapchain2);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%#08x)\n", hr);

    d3dpp.BackBufferCount  = 2;
    hr = IDirect3DDevice8_CreateAdditionalSwapChain(device, &d3dpp, &swapchain3);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%#08x)\n", hr);
    if(SUCCEEDED(hr)) {
        /* Swapchain 3, created with backbuffercount 2 */
        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain8_GetBackBuffer(swapchain3, 0, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 1st back buffer (%#08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain8_GetBackBuffer(swapchain3, 1, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 2nd back buffer (%#08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain8_GetBackBuffer(swapchain3, 2, 0, &backbuffer);
        ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %#08x\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain8_GetBackBuffer(swapchain3, 3, 0, &backbuffer);
        ok(FAILED(hr), "Failed to get the back buffer (%#08x)\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);
    }

    /* Check the back buffers of the swapchains */
    /* Swapchain 1, created with backbuffercount 0 */
    hr = IDirect3DSwapChain8_GetBackBuffer(swapchain1, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%#08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL (%#08x)\n", hr);
    if(backbuffer) IDirect3DSurface8_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain8_GetBackBuffer(swapchain1, 1, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%#08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

    /* Swapchain 2 - created with backbuffercount 1 */
    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain8_GetBackBuffer(swapchain2, 0, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%#08x)\n", hr);
    ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain8_GetBackBuffer(swapchain2, 1, 0, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %#08x\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain8_GetBackBuffer(swapchain2, 2, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%#08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface8_Release(backbuffer);

    IDirect3DSwapChain8_Release(swapchain3);
    IDirect3DSwapChain8_Release(swapchain2);
    IDirect3DSwapChain8_Release(swapchain1);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_refcount(void)
{
    IDirect3DVertexBuffer8      *pVertexBuffer      = NULL;
    IDirect3DIndexBuffer8       *pIndexBuffer       = NULL;
    DWORD                       dVertexShader       = -1;
    DWORD                       dPixelShader        = -1;
    IDirect3DCubeTexture8       *pCubeTexture       = NULL;
    IDirect3DTexture8           *pTexture           = NULL;
    IDirect3DVolumeTexture8     *pVolumeTexture     = NULL;
    IDirect3DVolume8            *pVolumeLevel       = NULL;
    IDirect3DSurface8           *pStencilSurface    = NULL;
    IDirect3DSurface8           *pImageSurface      = NULL;
    IDirect3DSurface8           *pRenderTarget      = NULL;
    IDirect3DSurface8           *pRenderTarget2     = NULL;
    IDirect3DSurface8           *pRenderTarget3     = NULL;
    IDirect3DSurface8           *pTextureLevel      = NULL;
    IDirect3DSurface8           *pBackBuffer        = NULL;
    DWORD                       dStateBlock         = -1;
    IDirect3DSwapChain8         *pSwapChain         = NULL;
    D3DCAPS8                    caps;
    D3DPRESENT_PARAMETERS        d3dpp;
    IDirect3DDevice8 *device = NULL;
    ULONG refcount = 0, tmp;
    IDirect3D8 *d3d, *d3d2;
    HWND window;
    HRESULT hr;

    DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),  /* D3DVSDE_POSITION, Register v0 */
        D3DVSD_REG(D3DVSDE_DIFFUSE, D3DVSDT_D3DCOLOR), /* D3DVSDE_DIFFUSE, Register v5 */
        D3DVSD_END()
    };

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    CHECK_REFCOUNT(d3d, 1);

    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    IDirect3DDevice8_GetDeviceCaps(device, &caps);

    refcount = get_refcount((IUnknown *)device);
    ok(refcount == 1, "Invalid device RefCount %d\n", refcount);

    CHECK_REFCOUNT(d3d, 2);

    hr = IDirect3DDevice8_GetDirect3D(device, &d3d2);
    CHECK_CALL(hr, "GetDirect3D", device, refcount);

    ok(d3d2 == d3d, "Expected IDirect3D8 pointers to be equal.\n");
    CHECK_REFCOUNT(d3d, 3);
    CHECK_RELEASE_REFCOUNT(d3d, 2);

    /**
     * Check refcount of implicit surfaces. Findings:
     *   - the container is the device
     *   - they hold a reference to the device
     *   - they are created with a refcount of 0 (Get/Release returns original refcount)
     *   - they are not freed if refcount reaches 0.
     *   - the refcount is not forwarded to the container.
     */
    hr = IDirect3DDevice8_GetRenderTarget(device, &pRenderTarget);
    CHECK_CALL(hr, "GetRenderTarget", device, ++refcount);
    if(pRenderTarget)
    {
        CHECK_SURFACE_CONTAINER(pRenderTarget, IID_IDirect3DDevice8, device);
        CHECK_REFCOUNT( pRenderTarget, 1);

        CHECK_ADDREF_REFCOUNT(pRenderTarget, 2);
        CHECK_REFCOUNT(device, refcount);
        CHECK_RELEASE_REFCOUNT(pRenderTarget, 1);
        CHECK_REFCOUNT(device, refcount);

        hr = IDirect3DDevice8_GetRenderTarget(device, &pRenderTarget);
        CHECK_CALL(hr, "GetRenderTarget", device, refcount);
        CHECK_REFCOUNT( pRenderTarget, 2);
        CHECK_RELEASE_REFCOUNT( pRenderTarget, 1);
        CHECK_RELEASE_REFCOUNT( pRenderTarget, 0);
        CHECK_REFCOUNT(device, --refcount);

        /* The render target is released with the device, so AddRef with refcount=0 is fine here. */
        CHECK_ADDREF_REFCOUNT(pRenderTarget, 1);
        CHECK_REFCOUNT(device, ++refcount);
        CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
        CHECK_REFCOUNT(device, --refcount);
    }

    /* Render target and back buffer are identical. */
    hr = IDirect3DDevice8_GetBackBuffer(device, 0, 0, &pBackBuffer);
    CHECK_CALL(hr, "GetBackBuffer", device, ++refcount);
    if(pBackBuffer)
    {
        CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
        ok(pRenderTarget == pBackBuffer, "RenderTarget=%p and BackBuffer=%p should be the same.\n",
           pRenderTarget, pBackBuffer);
        pBackBuffer = NULL;
    }
    CHECK_REFCOUNT(device, --refcount);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &pStencilSurface);
    CHECK_CALL(hr, "GetDepthStencilSurface", device, ++refcount);
    if(pStencilSurface)
    {
        CHECK_SURFACE_CONTAINER(pStencilSurface, IID_IDirect3DDevice8, device);
        CHECK_REFCOUNT( pStencilSurface, 1);

        CHECK_ADDREF_REFCOUNT(pStencilSurface, 2);
        CHECK_REFCOUNT(device, refcount);
        CHECK_RELEASE_REFCOUNT(pStencilSurface, 1);
        CHECK_REFCOUNT(device, refcount);

        CHECK_RELEASE_REFCOUNT( pStencilSurface, 0);
        CHECK_REFCOUNT(device, --refcount);

        /* The stencil surface is released with the device, so AddRef with refcount=0 is fine here. */
        CHECK_ADDREF_REFCOUNT(pStencilSurface, 1);
        CHECK_REFCOUNT(device, ++refcount);
        CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
        CHECK_REFCOUNT(device, --refcount);
        pStencilSurface = NULL;
    }

    /* Buffers */
    hr = IDirect3DDevice8_CreateIndexBuffer(device, 16, 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &pIndexBuffer);
    CHECK_CALL(hr, "CreateIndexBuffer", device, ++refcount);
    if(pIndexBuffer)
    {
        tmp = get_refcount( (IUnknown *)pIndexBuffer );

        hr = IDirect3DDevice8_SetIndices(device, pIndexBuffer, 0);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
        hr = IDirect3DDevice8_SetIndices(device, NULL, 0);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
    }

    hr = IDirect3DDevice8_CreateVertexBuffer(device, 16, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &pVertexBuffer);
    CHECK_CALL(hr, "CreateVertexBuffer", device, ++refcount);
    if(pVertexBuffer)
    {
        IDirect3DVertexBuffer8 *pVBuf = (void*)~0;
        UINT stride = ~0;

        tmp = get_refcount( (IUnknown *)pVertexBuffer );

        hr = IDirect3DDevice8_SetStreamSource(device, 0, pVertexBuffer, 3 * sizeof(float));
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);
        hr = IDirect3DDevice8_SetStreamSource(device, 0, NULL, 0);
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);

        hr = IDirect3DDevice8_GetStreamSource(device, 0, &pVBuf, &stride);
        ok(SUCCEEDED(hr), "GetStreamSource did not succeed with NULL stream!\n");
        ok(pVBuf==NULL, "pVBuf not NULL (%p)!\n", pVBuf);
        ok(stride==3*sizeof(float), "stride not 3 floats (got %u)!\n", stride);
    }

    /* Shaders */
    hr = IDirect3DDevice8_CreateVertexShader(device, decl, simple_vs, &dVertexShader, 0);
    CHECK_CALL(hr, "CreateVertexShader", device, refcount);
    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 0))
    {
        hr = IDirect3DDevice8_CreatePixelShader(device, simple_ps, &dPixelShader);
        CHECK_CALL(hr, "CreatePixelShader", device, refcount);
    }
    /* Textures */
    hr = IDirect3DDevice8_CreateTexture(device, 32, 32, 3, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pTexture);
    CHECK_CALL(hr, "CreateTexture", device, ++refcount);
    if (pTexture)
    {
        tmp = get_refcount( (IUnknown *)pTexture );

        /* SetTexture should not increase refcounts */
        hr = IDirect3DDevice8_SetTexture(device, 0, (IDirect3DBaseTexture8 *) pTexture);
        CHECK_CALL( hr, "SetTexture", pTexture, tmp);
        hr = IDirect3DDevice8_SetTexture(device, 0, NULL);
        CHECK_CALL( hr, "SetTexture", pTexture, tmp);

        /* This should not increment device refcount */
        hr = IDirect3DTexture8_GetSurfaceLevel( pTexture, 1, &pTextureLevel );
        CHECK_CALL(hr, "GetSurfaceLevel", device, refcount);
        /* But should increment texture's refcount */
        CHECK_REFCOUNT( pTexture, tmp+1 );
        /* Because the texture and surface refcount are identical */
        if (pTextureLevel)
        {
            CHECK_REFCOUNT        ( pTextureLevel, tmp+1 );
            CHECK_ADDREF_REFCOUNT ( pTextureLevel, tmp+2 );
            CHECK_REFCOUNT        ( pTexture     , tmp+2 );
            CHECK_RELEASE_REFCOUNT( pTextureLevel, tmp+1 );
            CHECK_REFCOUNT        ( pTexture     , tmp+1 );
            CHECK_RELEASE_REFCOUNT( pTexture     , tmp   );
            CHECK_REFCOUNT        ( pTextureLevel, tmp   );
        }
    }
    if(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice8_CreateCubeTexture(device, 32, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pCubeTexture);
        CHECK_CALL(hr, "CreateCubeTexture", device, ++refcount);
    }
    else
    {
        skip("Cube textures not supported\n");
    }
    if(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        hr = IDirect3DDevice8_CreateVolumeTexture(device, 32, 32, 2, 0, 0,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pVolumeTexture);
        CHECK_CALL(hr, "CreateVolumeTexture", device, ++refcount);
    }
    else
    {
        skip("Volume textures not supported\n");
    }

    if (pVolumeTexture)
    {
        tmp = get_refcount( (IUnknown *)pVolumeTexture );

        /* This should not increment device refcount */
        hr = IDirect3DVolumeTexture8_GetVolumeLevel(pVolumeTexture, 0, &pVolumeLevel);
        CHECK_CALL(hr, "GetVolumeLevel", device, refcount);
        /* But should increment volume texture's refcount */
        CHECK_REFCOUNT( pVolumeTexture, tmp+1 );
        /* Because the volume texture and volume refcount are identical */
        if (pVolumeLevel)
        {
            CHECK_REFCOUNT        ( pVolumeLevel  , tmp+1 );
            CHECK_ADDREF_REFCOUNT ( pVolumeLevel  , tmp+2 );
            CHECK_REFCOUNT        ( pVolumeTexture, tmp+2 );
            CHECK_RELEASE_REFCOUNT( pVolumeLevel  , tmp+1 );
            CHECK_REFCOUNT        ( pVolumeTexture, tmp+1 );
            CHECK_RELEASE_REFCOUNT( pVolumeTexture, tmp   );
            CHECK_REFCOUNT        ( pVolumeLevel  , tmp   );
        }
    }
    /* Surfaces */
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 32, 32,
            D3DFMT_D16, D3DMULTISAMPLE_NONE, &pStencilSurface);
    CHECK_CALL(hr, "CreateDepthStencilSurface", device, ++refcount);
    CHECK_REFCOUNT( pStencilSurface, 1);
    hr = IDirect3DDevice8_CreateImageSurface(device, 32, 32,
            D3DFMT_X8R8G8B8, &pImageSurface);
    CHECK_CALL(hr, "CreateImageSurface", device, ++refcount);
    CHECK_REFCOUNT( pImageSurface, 1);
    hr = IDirect3DDevice8_CreateRenderTarget(device, 32, 32,
            D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, TRUE, &pRenderTarget3);
    CHECK_CALL(hr, "CreateRenderTarget", device, ++refcount);
    CHECK_REFCOUNT( pRenderTarget3, 1);
    /* Misc */
    hr = IDirect3DDevice8_CreateStateBlock(device, D3DSBT_ALL, &dStateBlock);
    CHECK_CALL(hr, "CreateStateBlock", device, refcount);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    hr = IDirect3DDevice8_CreateAdditionalSwapChain(device, &d3dpp, &pSwapChain);
    CHECK_CALL(hr, "CreateAdditionalSwapChain", device, ++refcount);
    if(pSwapChain)
    {
        /* check implicit back buffer */
        hr = IDirect3DSwapChain8_GetBackBuffer(pSwapChain, 0, 0, &pBackBuffer);
        CHECK_CALL(hr, "GetBackBuffer", device, ++refcount);
        CHECK_REFCOUNT( pSwapChain, 1);
        if(pBackBuffer)
        {
            CHECK_SURFACE_CONTAINER(pBackBuffer, IID_IDirect3DDevice8, device);
            CHECK_REFCOUNT( pBackBuffer, 1);
            CHECK_RELEASE_REFCOUNT( pBackBuffer, 0);
            CHECK_REFCOUNT(device, --refcount);

            /* The back buffer is released with the swapchain, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pBackBuffer, 1);
            CHECK_REFCOUNT(device, ++refcount);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_REFCOUNT(device, --refcount);
            pBackBuffer = NULL;
        }
        CHECK_REFCOUNT( pSwapChain, 1);
    }

    if(pVertexBuffer)
    {
        BYTE *data;
        /* Vertex buffers can be locked multiple times */
        hr = IDirect3DVertexBuffer8_Lock(pVertexBuffer, 0, 0, &data, 0);
        ok(hr == D3D_OK, "IDirect3DVertexBuffer8::Lock failed with %#08x\n", hr);
        hr = IDirect3DVertexBuffer8_Lock(pVertexBuffer, 0, 0, &data, 0);
        ok(hr == D3D_OK, "IDirect3DVertexBuffer8::Lock failed with %#08x\n", hr);
        hr = IDirect3DVertexBuffer8_Unlock(pVertexBuffer);
        ok(hr == D3D_OK, "IDirect3DVertexBuffer8::Unlock failed with %#08x\n", hr);
        hr = IDirect3DVertexBuffer8_Unlock(pVertexBuffer);
        ok(hr == D3D_OK, "IDirect3DVertexBuffer8::Unlock failed with %#08x\n", hr);
    }

    /* The implicit render target is not freed if refcount reaches 0.
     * Otherwise GetRenderTarget would re-allocate it and the pointer would change.*/
    hr = IDirect3DDevice8_GetRenderTarget(device, &pRenderTarget2);
    CHECK_CALL(hr, "GetRenderTarget", device, ++refcount);
    if(pRenderTarget2)
    {
        CHECK_RELEASE_REFCOUNT(pRenderTarget2, 0);
        ok(pRenderTarget == pRenderTarget2, "RenderTarget=%p and RenderTarget2=%p should be the same.\n",
           pRenderTarget, pRenderTarget2);
        CHECK_REFCOUNT(device, --refcount);
        pRenderTarget2 = NULL;
    }
    pRenderTarget = NULL;

cleanup:
    CHECK_RELEASE(device,               device, --refcount);

    /* Buffers */
    CHECK_RELEASE(pVertexBuffer,        device, --refcount);
    CHECK_RELEASE(pIndexBuffer,         device, --refcount);
    /* Shaders */
    if (dVertexShader != ~0u)
        IDirect3DDevice8_DeleteVertexShader(device, dVertexShader);
    if (dPixelShader != ~0u)
        IDirect3DDevice8_DeletePixelShader(device, dPixelShader);
    /* Textures */
    CHECK_RELEASE(pTexture,             device, --refcount);
    CHECK_RELEASE(pCubeTexture,         device, --refcount);
    CHECK_RELEASE(pVolumeTexture,       device, --refcount);
    /* Surfaces */
    CHECK_RELEASE(pStencilSurface,      device, --refcount);
    CHECK_RELEASE(pImageSurface,        device, --refcount);
    CHECK_RELEASE(pRenderTarget3,       device, --refcount);
    /* Misc */
    if (dStateBlock != ~0u)
        IDirect3DDevice8_DeleteStateBlock(device, dStateBlock);
    /* This will destroy device - cannot check the refcount here */
    if (pSwapChain)
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
    CHECK_RELEASE_REFCOUNT(d3d, 0);
    DestroyWindow(window);
}

static void test_cursor(void)
{
    HMODULE user32_handle = GetModuleHandleA("user32.dll");
    IDirect3DSurface8 *cursor = NULL;
    IDirect3DDevice8 *device;
    CURSORINFO info;
    IDirect3D8 *d3d;
    ULONG refcount;
    HCURSOR cur;
    HWND window;
    HRESULT hr;

    pGetCursorInfo = (void *)GetProcAddress(user32_handle, "GetCursorInfo");
    if (!pGetCursorInfo)
    {
        win_skip("GetCursorInfo is not available\n");
        return;
    }

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(pGetCursorInfo(&info), "GetCursorInfo failed\n");
    cur = info.hCursor;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 32, 32, D3DFMT_A8R8G8B8, &cursor);
    ok(SUCCEEDED(hr), "Failed to create cursor surface, hr %#x.\n", hr);

    /* Initially hidden */
    hr = IDirect3DDevice8_ShowCursor(device, TRUE);
    ok(hr == FALSE, "IDirect3DDevice8_ShowCursor returned %#08x\n", hr);

    /* Not enabled without a surface*/
    hr = IDirect3DDevice8_ShowCursor(device, TRUE);
    ok(hr == FALSE, "IDirect3DDevice8_ShowCursor returned %#08x\n", hr);

    /* Fails */
    hr = IDirect3DDevice8_SetCursorProperties(device, 0, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_SetCursorProperties returned %#08x\n", hr);

    hr = IDirect3DDevice8_SetCursorProperties(device, 0, 0, cursor);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetCursorProperties returned %#08x\n", hr);

    IDirect3DSurface8_Release(cursor);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(pGetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & CURSOR_SHOWING, "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor == cur, "The cursor handle is %p\n", info.hCursor); /* unchanged */

    /* Still hidden */
    hr = IDirect3DDevice8_ShowCursor(device, TRUE);
    ok(hr == FALSE, "IDirect3DDevice8_ShowCursor returned %#08x\n", hr);

    /* Enabled now*/
    hr = IDirect3DDevice8_ShowCursor(device, TRUE);
    ok(hr == TRUE, "IDirect3DDevice8_ShowCursor returned %#08x\n", hr);

    /* GDI cursor unchanged */
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(pGetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & CURSOR_SHOWING, "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor == cur, "The cursor handle is %p\n", info.hCursor); /* unchanged */

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static const POINT *expect_pos;

static LRESULT CALLBACK test_cursor_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_MOUSEMOVE)
    {
        if (expect_pos && expect_pos->x && expect_pos->y)
        {
            POINT p = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

            ClientToScreen(window, &p);
            if (expect_pos->x == p.x && expect_pos->y == p.y)
                ++expect_pos;
        }
    }

    return DefWindowProcA(window, message, wparam, lparam);
}

static void test_cursor_pos(void)
{
    IDirect3DSurface8 *cursor;
    IDirect3DDevice8 *device;
    WNDCLASSA wc = {0};
    IDirect3D8 *d3d8;
    UINT refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    /* Note that we don't check for movement we're not supposed to receive.
     * That's because it's hard to distinguish from the user accidentally
     * moving the mouse. */
    static const POINT points[] =
    {
        {50, 50},
        {75, 75},
        {100, 100},
        {125, 125},
        {150, 150},
        {125, 125},
        {150, 150},
        {150, 150},
        {0, 0},
    };

    wc.lpfnWndProc = test_cursor_proc;
    wc.lpszClassName = "d3d8_test_cursor_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    window = CreateWindowA("d3d8_test_cursor_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 320, 240, NULL, NULL, NULL, NULL);
    ShowWindow(window, SW_SHOW);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    device = create_device(d3d8, window, window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 32, 32, D3DFMT_A8R8G8B8, &cursor);
    ok(SUCCEEDED(hr), "Failed to create cursor surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetCursorProperties(device, 0, 0, cursor);
    ok(SUCCEEDED(hr), "Failed to set cursor properties, hr %#x.\n", hr);
    IDirect3DSurface8_Release(cursor);
    ret = IDirect3DDevice8_ShowCursor(device, TRUE);
    ok(!ret, "Failed to show cursor, hr %#x.\n", ret);

    flush_events();
    expect_pos = points;

    ret = SetCursorPos(50, 50);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    IDirect3DDevice8_SetCursorPosition(device, 75, 75, 0);
    flush_events();
    /* SetCursorPosition() eats duplicates. */
    IDirect3DDevice8_SetCursorPosition(device, 75, 75, 0);
    flush_events();

    ret = SetCursorPos(100, 100);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    /* Even if the position was set with SetCursorPos(). */
    IDirect3DDevice8_SetCursorPosition(device, 100, 100, 0);
    flush_events();

    IDirect3DDevice8_SetCursorPosition(device, 125, 125, 0);
    flush_events();
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    IDirect3DDevice8_SetCursorPosition(device, 125, 125, 0);
    flush_events();

    IDirect3DDevice8_SetCursorPosition(device, 150, 150, 0);
    flush_events();
    /* SetCursorPos() doesn't. */
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    ok(!expect_pos->x && !expect_pos->y, "Didn't receive MOUSEMOVE %u (%d, %d).\n",
       (unsigned)(expect_pos - points), expect_pos->x, expect_pos->y);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    DestroyWindow(window);
    UnregisterClassA("d3d8_test_cursor_wc", GetModuleHandleA(NULL));
    IDirect3D8_Release(d3d8);
}

static void test_states(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZVISIBLE, TRUE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState(D3DRS_ZVISIBLE, TRUE) returned %#08x\n", hr);
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZVISIBLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState(D3DRS_ZVISIBLE, FALSE) returned %#08x\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_shader_versions(void)
{
    IDirect3D8 *d3d;
    D3DCAPS8 caps;
    HRESULT hr;

    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    hr = IDirect3D8_GetDeviceCaps(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    ok(SUCCEEDED(hr) || hr == D3DERR_NOTAVAILABLE, "Failed to get device caps, hr %#x.\n", hr);
    IDirect3D8_Release(d3d);
    if (FAILED(hr))
    {
        skip("No Direct3D support, skipping test.\n");
        return;
    }

    ok(caps.VertexShaderVersion <= D3DVS_VERSION(1,1),
            "Got unexpected VertexShaderVersion %#x.\n", caps.VertexShaderVersion);
    ok(caps.PixelShaderVersion <= D3DPS_VERSION(1,4),
            "Got unexpected PixelShaderVersion %#x.\n", caps.PixelShaderVersion);
}

static void test_display_formats(void)
{
    D3DDEVTYPE device_type = D3DDEVTYPE_HAL;
    unsigned int backbuffer, display;
    unsigned int windowed, i;
    D3DDISPLAYMODE mode;
    IDirect3D8 *d3d8;
    BOOL should_pass;
    BOOL has_modes;
    HRESULT hr;

    static const struct
    {
        const char *name;
        D3DFORMAT format;
        D3DFORMAT alpha_format;
        BOOL display;
        BOOL windowed;
    }
    formats[] =
    {
        {"D3DFMT_R5G6B5",   D3DFMT_R5G6B5,      0,                  TRUE,   TRUE},
        {"D3DFMT_X1R5G5B5", D3DFMT_X1R5G5B5,    D3DFMT_A1R5G5B5,    TRUE,   TRUE},
        {"D3DFMT_A1R5G5B5", D3DFMT_A1R5G5B5,    D3DFMT_A1R5G5B5,    FALSE,  FALSE},
        {"D3DFMT_X8R8G8B8", D3DFMT_X8R8G8B8,    D3DFMT_A8R8G8B8,    TRUE,   TRUE},
        {"D3DFMT_A8R8G8B8", D3DFMT_A8R8G8B8,    D3DFMT_A8R8G8B8,    FALSE,  FALSE},
        {"D3DFMT_UNKNOWN",  D3DFMT_UNKNOWN,     0,                  FALSE,  FALSE},
    };

    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    for (display = 0; display < sizeof(formats) / sizeof(*formats); ++display)
    {
        for (i = 0, has_modes = FALSE; SUCCEEDED(IDirect3D8_EnumAdapterModes(d3d8, D3DADAPTER_DEFAULT, i, &mode)); ++i)
        {
            if (mode.Format == formats[display].format)
            {
                has_modes = TRUE;
                break;
            }
        }

        for (windowed = 0; windowed <= 1; ++windowed)
        {
            for (backbuffer = 0; backbuffer < sizeof(formats) / sizeof(*formats); ++backbuffer)
            {
                should_pass = FALSE;

                if (formats[display].display && (formats[display].windowed || !windowed) && (has_modes || windowed))
                {
                    D3DFORMAT backbuffer_format;

                    if (windowed && formats[backbuffer].format == D3DFMT_UNKNOWN)
                        backbuffer_format = formats[display].format;
                    else
                        backbuffer_format = formats[backbuffer].format;

                    hr = IDirect3D8_CheckDeviceFormat(d3d8, D3DADAPTER_DEFAULT, device_type, formats[display].format,
                            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, backbuffer_format);
                    should_pass = (hr == D3D_OK) && (formats[display].format == formats[backbuffer].format
                            || (formats[display].alpha_format
                            && formats[display].alpha_format == formats[backbuffer].alpha_format));
                }

                hr = IDirect3D8_CheckDeviceType(d3d8, D3DADAPTER_DEFAULT, device_type,
                        formats[display].format, formats[backbuffer].format, windowed);
                ok(SUCCEEDED(hr) == should_pass || broken(SUCCEEDED(hr) && !has_modes) /* Win8 64-bit */,
                        "Got unexpected hr %#x for %s / %s, windowed %#x, should_pass %#x.\n",
                        hr, formats[display].name, formats[backbuffer].name, windowed, should_pass);
            }
        }
    }

    IDirect3D8_Release(d3d8);
}

/* Test adapter display modes */
static void test_display_modes(void)
{
    UINT max_modes, i;
    D3DDISPLAYMODE dmode;
    IDirect3D8 *d3d;
    HRESULT res;

    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    max_modes = IDirect3D8_GetAdapterModeCount(d3d, D3DADAPTER_DEFAULT);
    ok(max_modes > 0 ||
       broken(max_modes == 0), /* VMware */
       "GetAdapterModeCount(D3DADAPTER_DEFAULT) returned 0!\n");

    for (i = 0; i < max_modes; ++i)
    {
        res = IDirect3D8_EnumAdapterModes(d3d, D3DADAPTER_DEFAULT, i, &dmode);
        ok(res==D3D_OK, "EnumAdapterModes returned %#08x for mode %u!\n", res, i);
        if(res != D3D_OK)
            continue;

        ok(dmode.Format==D3DFMT_X8R8G8B8 || dmode.Format==D3DFMT_R5G6B5,
           "Unexpected display mode returned for mode %u: %#x\n", i , dmode.Format);
    }

    IDirect3D8_Release(d3d);
}

static void test_reset(void)
{
    UINT width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    UINT height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    IDirect3DDevice8 *device1 = NULL;
    IDirect3DDevice8 *device2 = NULL;
    D3DDISPLAYMODE d3ddm, d3ddm2;
    D3DSURFACE_DESC surface_desc;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DSurface8 *surface;
    IDirect3DTexture8 *texture;
    UINT adapter_mode_count;
    D3DLOCKED_RECT lockrect;
    UINT mode_count = 0;
    IDirect3D8 *d3d8;
    RECT winrect;
    D3DVIEWPORT8 vp;
    D3DCAPS8 caps;
    DWORD shader;
    DWORD value;
    HWND window;
    HRESULT hr;
    UINT i;

    static const DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT4),
        D3DVSD_END(),
    };

    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    hr = IDirect3D8_GetAdapterDisplayMode(d3d8, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    adapter_mode_count = IDirect3D8_GetAdapterModeCount(d3d8, D3DADAPTER_DEFAULT);
    modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*modes) * adapter_mode_count);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;

        memset(&d3ddm2, 0, sizeof(d3ddm2));
        hr = IDirect3D8_EnumAdapterModes(d3d8, D3DADAPTER_DEFAULT, i, &d3ddm2);
        ok(SUCCEEDED(hr), "EnumAdapterModes failed, hr %#x.\n", hr);

        if (d3ddm2.Format != d3ddm.Format)
            continue;

        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].w == d3ddm2.Width && modes[j].h == d3ddm2.Height)
                break;
        }
        if (j == mode_count)
        {
            modes[j].w = d3ddm2.Width;
            modes[j].h = d3ddm2.Height;
            ++mode_count;
        }

        /* We use them as invalid modes. */
        if ((d3ddm2.Width == 801 && d3ddm2.Height == 600)
                || (d3ddm2.Width == 32 && d3ddm2.Height == 32))
        {
            skip("This system supports a screen resolution of %dx%d, not running mode tests.\n",
                    d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests.\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height) ++i;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device1);
    if (FAILED(hr))
    {
        skip("Failed to create device, hr %#x.\n", hr);
        goto cleanup;
    }
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDeviceCaps(device1, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice8_GetViewport(device1, &vp);
    ok(SUCCEEDED(hr), "GetViewport failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %u, expected 0.\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %u, expected 0.\n", vp.Y);
        ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u.\n", vp.Width, modes[i].w);
        ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u.\n", vp.Height, modes[i].h);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %.8e, expected 0.\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %.8e, expected 1.\n", vp.MaxZ);
    }

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.Width = modes[i].w  / 2;
    vp.Height = modes[i].h / 2;
    vp.MinZ = 0.2f;
    vp.MaxZ = 0.3f;
    hr = IDirect3DDevice8_SetViewport(device1, &vp);
    ok(SUCCEEDED(hr), "SetViewport failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice8_SetRenderState(device1, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);

    memset(&vp, 0, sizeof(vp));
    hr = IDirect3DDevice8_GetViewport(device1, &vp);
    ok(SUCCEEDED(hr), "GetViewport failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %u, expected 0.\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %u, expected 0.\n", vp.Y);
        ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u.\n", vp.Width, modes[i].w);
        ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u.\n", vp.Height, modes[i].h);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %.8e, expected 0.\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %.8e, expected 1.\n", vp.MaxZ);
    }

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice8_GetRenderTarget(device1, &surface);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(hr == D3D_OK, "GetDesc failed, hr %#x.\n", hr);
    ok(surface_desc.Width == modes[i].w, "Back buffer width is %u, expected %u.\n",
            surface_desc.Width, modes[i].w);
    ok(surface_desc.Height == modes[i].h, "Back buffer height is %u, expected %u.\n",
            surface_desc.Height, modes[i].h);
    IDirect3DSurface8_Release(surface);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    memset(&vp, 0, sizeof(vp));
    hr = IDirect3DDevice8_GetViewport(device1, &vp);
    ok(SUCCEEDED(hr), "GetViewport failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %u, expected 0.\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %u, expected 0.\n", vp.Y);
        ok(vp.Width == 400, "D3DVIEWPORT->Width = %u, expected 400.\n", vp.Width);
        ok(vp.Height == 300, "D3DVIEWPORT->Height = %u, expected 300.\n", vp.Height);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %.8e, expected 0.\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %.8e, expected 1.\n", vp.MaxZ);
    }

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Screen width is %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Screen height is %u, expected %u.\n", height, orig_height);

    hr = IDirect3DDevice8_GetRenderTarget(device1, &surface);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(hr == D3D_OK, "GetDesc failed, hr %#x.\n", hr);
    ok(surface_desc.Width == 400, "Back buffer width is %u, expected 400.\n",
            surface_desc.Width);
    ok(surface_desc.Height == 300, "Back buffer height is %u, expected 300.\n",
            surface_desc.Height);
    IDirect3DSurface8_Release(surface);

    winrect.left = 0;
    winrect.top = 0;
    winrect.right = 200;
    winrect.bottom = 150;
    ok(AdjustWindowRect(&winrect, WS_OVERLAPPEDWINDOW, FALSE), "AdjustWindowRect failed\n");
    ok(SetWindowPos(window, NULL, 0, 0,
                    winrect.right-winrect.left,
                    winrect.bottom-winrect.top,
                    SWP_NOMOVE|SWP_NOZORDER),
       "SetWindowPos failed\n");

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    memset(&vp, 0, sizeof(vp));
    hr = IDirect3DDevice8_GetViewport(device1, &vp);
    ok(SUCCEEDED(hr), "GetViewport failed, hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %u, expected 0.\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %u, expected 0.\n", vp.Y);
        ok(vp.Width == 200, "D3DVIEWPORT->Width = %u, expected 200.\n", vp.Width);
        ok(vp.Height == 150, "D3DVIEWPORT->Height = %u, expected 150.\n", vp.Height);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %.8e, expected 0.\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %.8e, expected 1.\n", vp.MaxZ);
    }

    hr = IDirect3DDevice8_GetRenderTarget(device1, &surface);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(hr == D3D_OK, "GetDesc failed, hr %#x.\n", hr);
    ok(surface_desc.Width == 200, "Back buffer width is %u, expected 200.\n", surface_desc.Width);
    ok(surface_desc.Height == 150, "Back buffer height is %u, expected 150.\n", surface_desc.Height);
    IDirect3DSurface8_Release(surface);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = d3ddm.Format;

    /* Reset fails if there is a resource in the default pool. */
    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(hr == D3DERR_DEVICELOST, "Reset returned %#x, expected %#x.\n", hr, D3DERR_DEVICELOST);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n", hr, D3DERR_DEVICENOTRESET);
    IDirect3DTexture8_Release(texture);
    /* Reset again to get the device out of the lost state. */
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture8 *volume_texture;

        hr = IDirect3DDevice8_CreateVolumeTexture(device1, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture);
        ok(SUCCEEDED(hr), "CreateVolumeTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_Reset(device1, &d3dpp);
        ok(hr == D3DERR_DEVICELOST, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
        hr = IDirect3DDevice8_TestCooperativeLevel(device1);
        ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n",
                hr, D3DERR_DEVICENOTRESET);
        IDirect3DVolumeTexture8_Release(volume_texture);
        hr = IDirect3DDevice8_Reset(device1, &d3dpp);
        ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
        hr = IDirect3DDevice8_TestCooperativeLevel(device1);
        ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Scratch, sysmem and managed pool resources are fine. */
    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice8_SetRenderTarget(device1, NULL, NULL);
    ok(SUCCEEDED(hr), "SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned %#x, expected %#x.\n", hr, D3DERR_NOTFOUND);
    ok(!surface, "Depth / stencil buffer should be NULL.\n");

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device1, &surface);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    ok(!!surface, "Depth / stencil buffer should not be NULL.\n");
    if (surface) IDirect3DSurface8_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned %#x, expected %#x.\n", hr, D3DERR_NOTFOUND);
    ok(!surface, "Depth / stencil buffer should be NULL.\n");

    /* Will a sysmem or scratch resource survive while locked? */
    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture8_LockRect(texture, 0, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "LockRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    IDirect3DTexture8_UnlockRect(texture, 0);
    IDirect3DTexture8_Release(texture);

    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DTexture8_LockRect(texture, 0, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "LockRect failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    IDirect3DTexture8_UnlockRect(texture, 0);
    IDirect3DTexture8_Release(texture);

    hr = IDirect3DDevice8_CreateTexture(device1, 16, 16, 1, 0, D3DFMT_R5G6B5, D3DPOOL_MANAGED, &texture);
    ok(SUCCEEDED(hr), "CreateTexture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    /* A reference held to an implicit surface causes failures as well. */
    hr = IDirect3DDevice8_GetBackBuffer(device1, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "GetBackBuffer failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(hr == D3DERR_DEVICELOST, "Reset returned %#x, expected %#x.\n", hr, D3DERR_DEVICELOST);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n", hr, D3DERR_DEVICENOTRESET);
    IDirect3DSurface8_Release(surface);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    /* Shaders are fine as well. */
    hr = IDirect3DDevice8_CreateVertexShader(device1, decl, simple_vs, &shader, 0);
    ok(SUCCEEDED(hr), "CreateVertexShader failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DeleteVertexShader(device1, shader);
    ok(SUCCEEDED(hr), "DeleteVertexShader failed, hr %#x.\n", hr);

    /* Try setting invalid modes. */
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 32;
    d3dpp.BackBufferHeight = 32;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n", hr, D3DERR_DEVICENOTRESET);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 801;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n", hr, D3DERR_DEVICENOTRESET);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice8_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    hr = IDirect3DDevice8_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n", hr, D3DERR_DEVICENOTRESET);

    hr = IDirect3D8_GetAdapterDisplayMode(d3d8, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#x.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device2);
    if (FAILED(hr))
    {
        skip("Failed to create device, hr %#x.\n", hr);
        goto cleanup;
    }

    hr = IDirect3DDevice8_TestCooperativeLevel(device2);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);

    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3DDevice8_Reset(device2, &d3dpp);
    ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    hr = IDirect3DDevice8_GetDepthStencilSurface(device2, &surface);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    ok(!!surface, "Depth / stencil buffer should not be NULL.\n");
    if (surface)
        IDirect3DSurface8_Release(surface);

cleanup:
    HeapFree(GetProcessHeap(), 0, modes);
    if (device2)
        IDirect3DDevice8_Release(device2);
    if (device1)
        IDirect3DDevice8_Release(device1);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_scene(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_EndScene returned %#08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene failed with %#08x\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_EndScene failed with %#08x\n", hr);

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_EndScene returned %#08x\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene failed with %#08x\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_BeginScene returned %#08x\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_EndScene failed with %#08x\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_EndScene returned %#08x\n", hr);

    /* StretchRect does not exit in Direct3D8, so no equivalent to the d3d9 stretchrect tests */

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_shader(void)
{
    DWORD                        hPixelShader = 0, hVertexShader = 0;
    DWORD                        hPixelShader2 = 0, hVertexShader2 = 0;
    DWORD                        hTempHandle;
    D3DCAPS8                     caps;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    DWORD data_size;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    static DWORD dwVertexDecl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION,  D3DVSDT_FLOAT3),
        D3DVSD_END()
    };
    DWORD decl_normal_float2[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),  /* D3DVSDE_POSITION, Register v0 */
        D3DVSD_REG(D3DVSDE_NORMAL,   D3DVSDT_FLOAT2),  /* D3DVSDE_NORMAL,   Register v1 */
        D3DVSD_END()
    };
    DWORD decl_normal_float4[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),  /* D3DVSDE_POSITION, Register v0 */
        D3DVSD_REG(D3DVSDE_NORMAL,   D3DVSDT_FLOAT4),  /* D3DVSDE_NORMAL,   Register v1 */
        D3DVSD_END()
    };
    DWORD decl_normal_d3dcolor[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),  /* D3DVSDE_POSITION, Register v0 */
        D3DVSD_REG(D3DVSDE_NORMAL,   D3DVSDT_D3DCOLOR),/* D3DVSDE_NORMAL,   Register v1 */
        D3DVSD_END()
    };
    const DWORD vertex_decl_size = sizeof(dwVertexDecl);
    const DWORD simple_vs_size = sizeof(simple_vs);
    const DWORD simple_ps_size = sizeof(simple_ps);

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    IDirect3DDevice8_GetDeviceCaps(device, &caps);

    /* Test setting and retrieving a FVF */
    hr = IDirect3DDevice8_SetVertexShader(device, fvf);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_GetVertexShader(device, &hTempHandle);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_GetVertexShader returned %#08x\n", hr);
    ok(hTempHandle == fvf, "Vertex shader %#08x is set, expected %#08x\n", hTempHandle, fvf);

    /* First create a vertex shader */
    hr = IDirect3DDevice8_SetVertexShader(device, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, dwVertexDecl, simple_vs, &hVertexShader, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);
    /* Msdn says that the new vertex shader is set immediately. This is wrong, apparently */
    hr = IDirect3DDevice8_GetVertexShader(device, &hTempHandle);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShader returned %#08x\n", hr);
    ok(hTempHandle == 0, "Vertex Shader %d is set, expected shader %d\n", hTempHandle, 0);
    /* Assign the shader, then verify that GetVertexShader works */
    hr = IDirect3DDevice8_SetVertexShader(device, hVertexShader);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_GetVertexShader(device, &hTempHandle);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShader returned %#08x\n", hr);
    ok(hTempHandle == hVertexShader, "Vertex Shader %d is set, expected shader %d\n", hTempHandle, hVertexShader);
    /* Verify that we can retrieve the declaration */
    hr = IDirect3DDevice8_GetVertexShaderDeclaration(device, hVertexShader, NULL, &data_size);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShaderDeclaration returned %#08x\n", hr);
    ok(data_size == vertex_decl_size, "Got data_size %u, expected %u\n", data_size, vertex_decl_size);
    data = HeapAlloc(GetProcessHeap(), 0, vertex_decl_size);
    data_size = 1;
    hr = IDirect3DDevice8_GetVertexShaderDeclaration(device, hVertexShader, data, &data_size);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_GetVertexShaderDeclaration returned (%#08x), "
            "expected D3DERR_INVALIDCALL\n", hr);
    ok(data_size == 1, "Got data_size %u, expected 1\n", data_size);
    data_size = vertex_decl_size;
    hr = IDirect3DDevice8_GetVertexShaderDeclaration(device, hVertexShader, data, &data_size);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShaderDeclaration returned %#08x\n", hr);
    ok(data_size == vertex_decl_size, "Got data_size %u, expected %u\n", data_size, vertex_decl_size);
    ok(!memcmp(data, dwVertexDecl, vertex_decl_size), "data not equal to shader declaration\n");
    HeapFree(GetProcessHeap(), 0, data);
    /* Verify that we can retrieve the shader function */
    hr = IDirect3DDevice8_GetVertexShaderFunction(device, hVertexShader, NULL, &data_size);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShaderFunction returned %#08x\n", hr);
    ok(data_size == simple_vs_size, "Got data_size %u, expected %u\n", data_size, simple_vs_size);
    data = HeapAlloc(GetProcessHeap(), 0, simple_vs_size);
    data_size = 1;
    hr = IDirect3DDevice8_GetVertexShaderFunction(device, hVertexShader, data, &data_size);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_GetVertexShaderFunction returned (%#08x), "
            "expected D3DERR_INVALIDCALL\n", hr);
    ok(data_size == 1, "Got data_size %u, expected 1\n", data_size);
    data_size = simple_vs_size;
    hr = IDirect3DDevice8_GetVertexShaderFunction(device, hVertexShader, data, &data_size);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShaderFunction returned %#08x\n", hr);
    ok(data_size == simple_vs_size, "Got data_size %u, expected %u\n", data_size, simple_vs_size);
    ok(!memcmp(data, simple_vs, simple_vs_size), "data not equal to shader function\n");
    HeapFree(GetProcessHeap(), 0, data);
    /* Delete the assigned shader. This is supposed to work */
    hr = IDirect3DDevice8_DeleteVertexShader(device, hVertexShader);
    ok(hr == D3D_OK, "IDirect3DDevice8_DeleteVertexShader returned %#08x\n", hr);
    /* The shader should be unset now */
    hr = IDirect3DDevice8_GetVertexShader(device, &hTempHandle);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShader returned %#08x\n", hr);
    ok(hTempHandle == 0, "Vertex Shader %d is set, expected shader %d\n", hTempHandle, 0);

    /* Test a broken declaration. 3DMark2001 tries to use normals with 2 components
     * First try the fixed function shader function, then a custom one
     */
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_normal_float2, 0, &hVertexShader, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_normal_float4, 0, &hVertexShader, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, decl_normal_d3dcolor, 0, &hVertexShader, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_CreateVertexShader(device, decl_normal_float2, simple_vs, &hVertexShader, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);
    IDirect3DDevice8_DeleteVertexShader(device, hVertexShader);

    if (caps.PixelShaderVersion >= D3DPS_VERSION(1, 0))
    {
        /* The same with a pixel shader */
        hr = IDirect3DDevice8_CreatePixelShader(device, simple_ps, &hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %#08x\n", hr);
        /* Msdn says that the new pixel shader is set immediately. This is wrong, apparently */
        hr = IDirect3DDevice8_GetPixelShader(device, &hTempHandle);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShader returned %#08x\n", hr);
        ok(hTempHandle == 0, "Pixel Shader %d is set, expected shader %d\n", hTempHandle, 0);
        /* Assign the shader, then verify that GetPixelShader works */
        hr = IDirect3DDevice8_SetPixelShader(device, hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %#08x\n", hr);
        hr = IDirect3DDevice8_GetPixelShader(device, &hTempHandle);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShader returned %#08x\n", hr);
        ok(hTempHandle == hPixelShader, "Pixel Shader %d is set, expected shader %d\n", hTempHandle, hPixelShader);
        /* Verify that we can retrieve the shader function */
        hr = IDirect3DDevice8_GetPixelShaderFunction(device, hPixelShader, NULL, &data_size);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShaderFunction returned %#08x\n", hr);
        ok(data_size == simple_ps_size, "Got data_size %u, expected %u\n", data_size, simple_ps_size);
        data = HeapAlloc(GetProcessHeap(), 0, simple_ps_size);
        data_size = 1;
        hr = IDirect3DDevice8_GetPixelShaderFunction(device, hPixelShader, data, &data_size);
        ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_GetPixelShaderFunction returned (%#08x), "
                "expected D3DERR_INVALIDCALL\n", hr);
        ok(data_size == 1, "Got data_size %u, expected 1\n", data_size);
        data_size = simple_ps_size;
        hr = IDirect3DDevice8_GetPixelShaderFunction(device, hPixelShader, data, &data_size);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShaderFunction returned %#08x\n", hr);
        ok(data_size == simple_ps_size, "Got data_size %u, expected %u\n", data_size, simple_ps_size);
        ok(!memcmp(data, simple_ps, simple_ps_size), "data not equal to shader function\n");
        HeapFree(GetProcessHeap(), 0, data);
        /* Delete the assigned shader. This is supposed to work */
        hr = IDirect3DDevice8_DeletePixelShader(device, hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_DeletePixelShader returned %#08x\n", hr);
        /* The shader should be unset now */
        hr = IDirect3DDevice8_GetPixelShader(device, &hTempHandle);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShader returned %#08x\n", hr);
        ok(hTempHandle == 0, "Pixel Shader %d is set, expected shader %d\n", hTempHandle, 0);

        /* What happens if a non-bound shader is deleted? */
        hr = IDirect3DDevice8_CreatePixelShader(device, simple_ps, &hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %#08x\n", hr);
        hr = IDirect3DDevice8_CreatePixelShader(device, simple_ps, &hPixelShader2);
        ok(hr == D3D_OK, "IDirect3DDevice8_CreatePixelShader returned %#08x\n", hr);

        hr = IDirect3DDevice8_SetPixelShader(device, hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetPixelShader returned %#08x\n", hr);
        hr = IDirect3DDevice8_DeletePixelShader(device, hPixelShader2);
        ok(hr == D3D_OK, "IDirect3DDevice8_DeletePixelShader returned %#08x\n", hr);
        hr = IDirect3DDevice8_GetPixelShader(device, &hTempHandle);
        ok(hr == D3D_OK, "IDirect3DDevice8_GetPixelShader returned %#08x\n", hr);
        ok(hTempHandle == hPixelShader, "Pixel Shader %d is set, expected shader %d\n", hTempHandle, hPixelShader);
        hr = IDirect3DDevice8_DeletePixelShader(device, hPixelShader);
        ok(hr == D3D_OK, "IDirect3DDevice8_DeletePixelShader returned %#08x\n", hr);

        /* Check for double delete. */
        hr = IDirect3DDevice8_DeletePixelShader(device, hPixelShader2);
        ok(hr == D3DERR_INVALIDCALL || broken(hr == D3D_OK), "IDirect3DDevice8_DeletePixelShader returned %#08x\n", hr);
        hr = IDirect3DDevice8_DeletePixelShader(device, hPixelShader);
        ok(hr == D3DERR_INVALIDCALL || broken(hr == D3D_OK), "IDirect3DDevice8_DeletePixelShader returned %#08x\n", hr);
    }
    else
    {
        skip("Pixel shaders not supported\n");
    }

    /* What happens if a non-bound shader is deleted? */
    hr = IDirect3DDevice8_CreateVertexShader(device, dwVertexDecl, NULL, &hVertexShader, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_CreateVertexShader(device, dwVertexDecl, NULL, &hVertexShader2, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, hVertexShader);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_DeleteVertexShader(device, hVertexShader2);
    ok(hr == D3D_OK, "IDirect3DDevice8_DeleteVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_GetVertexShader(device, &hTempHandle);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetVertexShader returned %#08x\n", hr);
    ok(hTempHandle == hVertexShader, "Vertex Shader %d is set, expected shader %d\n", hTempHandle, hVertexShader);
    hr = IDirect3DDevice8_DeleteVertexShader(device, hVertexShader);
    ok(hr == D3D_OK, "IDirect3DDevice8_DeleteVertexShader returned %#08x\n", hr);

    /* Check for double delete. */
    hr = IDirect3DDevice8_DeleteVertexShader(device, hVertexShader2);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_DeleteVertexShader returned %#08x\n", hr);
    hr = IDirect3DDevice8_DeleteVertexShader(device, hVertexShader);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_DeleteVertexShader returned %#08x\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_limits(void)
{
    IDirect3DTexture8 *texture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 16, 16, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateTexture failed with %#08x\n", hr);

    /* There are 8 texture stages. We should be able to access all of them */
    for (i = 0; i < 8; ++i)
    {
        hr = IDirect3DDevice8_SetTexture(device, i, (IDirect3DBaseTexture8 *)texture);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %#08x\n", i, hr);
        hr = IDirect3DDevice8_SetTexture(device, i, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %#08x\n", i, hr);
        hr = IDirect3DDevice8_SetTextureStageState(device, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTextureStageState for texture %d failed with %#08x\n", i, hr);
    }

    /* Investigations show that accessing higher textures stage states does
     * not return an error either. Writing to too high texture stages
     * (approximately texture 40) causes memory corruption in windows, so
     * there is no bounds checking. */
    IDirect3DTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_lights(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;
    BOOL enabled;
    D3DCAPS8 caps;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetDeviceCaps failed with %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice8_LightEnable(device, i, TRUE);
        ok(hr == D3D_OK, "Enabling light %u failed with %08x\n", i, hr);
        hr = IDirect3DDevice8_GetLightEnable(device, i, &enabled);
        ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL),
            "GetLightEnable on light %u failed with %08x\n", i, hr);
        ok(enabled, "Light %d is %s\n", i, enabled ? "enabled" : "disabled");
    }

    /* TODO: Test the rendering results in this situation */
    hr = IDirect3DDevice8_LightEnable(device, i + 1, TRUE);
    ok(hr == D3D_OK, "Enabling one light more than supported returned %08x\n", hr);
    hr = IDirect3DDevice8_GetLightEnable(device, i + 1, &enabled);
    ok(hr == D3D_OK, "GetLightEnable on light %u failed with %08x\n", i + 1, hr);
    ok(enabled, "Light %d is %s\n", i + 1, enabled ? "enabled" : "disabled");
    hr = IDirect3DDevice8_LightEnable(device, i + 1, FALSE);
    ok(hr == D3D_OK, "Disabling the additional returned %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice8_LightEnable(device, i, FALSE);
        ok(hr == D3D_OK, "Disabling light %u failed with %08x\n", i, hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_render_zero_triangles(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    struct nvertex
    {
        float x, y, z;
        float nx, ny, nz;
        DWORD diffuse;
    }  quad[] =
    {
        { 0.0f, -1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 0.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 1.0f,  0.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
        { 1.0f, -1.0f,   0.1f,  1.0f,   1.0f,   1.0f,   0xff0000ff},
    };

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 0 /* NumVerts */,
            0 /* PrimCount */, NULL, D3DFMT_INDEX16, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_depth_stencil_reset(void)
{
    D3DPRESENT_PARAMETERS present_parameters;
    D3DDISPLAYMODE display_mode;
    IDirect3DSurface8 *surface, *orig_rt;
    IDirect3DDevice8 *device = NULL;
    IDirect3D8 *d3d8;
    UINT refcount;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    IDirect3D8_GetAdapterDisplayMode(d3d8, D3DADAPTER_DEFAULT, &display_mode);
    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed               = TRUE;
    present_parameters.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat       = display_mode.Format;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr))
    {
        skip("could not create device, IDirect3D8_CreateDevice returned %#x\n", hr);
        goto cleanup;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &orig_rt);
    ok(hr == D3D_OK, "GetRenderTarget failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(SUCCEEDED(hr), "TestCooperativeLevel failed with %#x\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, NULL, NULL);
    ok(hr == D3D_OK, "SetRenderTarget failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_GetRenderTarget(device, &surface);
    ok(hr == D3D_OK, "GetRenderTarget failed with 0x%08x\n", hr);
    ok(surface == orig_rt, "Render target is %p, should be %p\n", surface, orig_rt);
    if (surface) IDirect3DSurface8_Release(surface);
    IDirect3DSurface8_Release(orig_rt);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice8_Reset(device, &present_parameters);
    ok(hr == D3D_OK, "Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface8_Release(surface);

    present_parameters.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice8_Reset(device, &present_parameters);
    ok(hr == D3D_OK, "Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    device = NULL;

    IDirect3D8_GetAdapterDisplayMode( d3d8, D3DADAPTER_DEFAULT, &display_mode );

    ZeroMemory( &present_parameters, sizeof(present_parameters) );
    present_parameters.Windowed         = TRUE;
    present_parameters.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = display_mode.Format;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D8_CreateDevice( d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                    D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device );

    if(FAILED(hr))
    {
        skip("could not create device, IDirect3D8_CreateDevice returned %#x\n", hr);
        goto cleanup;
    }

    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_TestCooperativeLevel after creation returned %#x\n", hr);

    present_parameters.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    present_parameters.Windowed         = TRUE;
    present_parameters.BackBufferWidth  = 400;
    present_parameters.BackBufferHeight = 300;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3DDevice8_Reset(device, &present_parameters);
    ok(hr == D3D_OK, "IDirect3DDevice8_Reset failed with 0x%08x\n", hr);

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface8_Release(surface);

cleanup:
    if (device)
    {
        refcount = IDirect3DDevice8_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D8_Release(d3d8);
    DestroyWindow(hwnd);
}

static HWND filter_messages;

enum message_window
{
    DEVICE_WINDOW,
    FOCUS_WINDOW,
};

struct message
{
    UINT message;
    enum message_window window;
};

static const struct message *expect_messages;
static HWND device_window, focus_window;

struct wndproc_thread_param
{
    HWND dummy_window;
    HANDLE window_created;
    HANDLE test_finished;
    BOOL running_in_foreground;
};

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (filter_messages && filter_messages == hwnd)
    {
        if (message != WM_DISPLAYCHANGE && message != WM_IME_NOTIFY)
            todo_wine ok(0, "Received unexpected message %#x for window %p.\n", message, hwnd);
    }

    if (expect_messages)
    {
        HWND w;

        switch (expect_messages->window)
        {
            case DEVICE_WINDOW:
                w = device_window;
                break;

            case FOCUS_WINDOW:
                w = focus_window;
                break;

            default:
                w = NULL;
                break;
        };

        if (hwnd == w && expect_messages->message == message) ++expect_messages;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static DWORD WINAPI wndproc_thread(void *param)
{
    struct wndproc_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->dummy_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    p->running_in_foreground = SetForegroundWindow(p->dummy_window);

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        res = WaitForSingleObject(p->test_finished, 100);
        if (res == WAIT_OBJECT_0) break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->dummy_window);

    return 0;
}

static void test_wndproc(void)
{
    struct wndproc_thread_param thread_params;
    IDirect3DDevice8 *device;
    WNDCLASSA wc = {0};
    IDirect3D8 *d3d8;
    HANDLE thread;
    LONG_PTR proc;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    static const struct message messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW},
        {WM_ACTIVATE,           FOCUS_WINDOW},
        {WM_SETFOCUS,           FOCUS_WINDOW},
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW},
        {WM_MOVE,               DEVICE_WINDOW},
        {WM_SIZE,               DEVICE_WINDOW},
        {0,                     0},
    };

    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d8_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    trace("device_window %p, focus_window %p, dummy_window %p.\n",
            device_window, focus_window, thread_params.dummy_window);

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    if (thread_params.running_in_foreground)
    {
        tmp = GetForegroundWindow();
        ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                thread_params.dummy_window, tmp);
    }
    else
        skip("Not running in foreground, skip foreground window test\n");

    flush_events();

    expect_messages = messages;

    device = create_device(d3d8, device_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it.\n",
            expect_messages->message, expect_messages->window);
    expect_messages = NULL;

    if (0) /* Disabled until we can make this work in a reliable way on Wine. */
    {
        tmp = GetFocus();
        ok(tmp == focus_window, "Expected focus %p, got %p.\n", focus_window, tmp);
        tmp = GetForegroundWindow();
        ok(tmp == focus_window, "Expected foreground window %p, got %p.\n", focus_window, tmp);
    }
    SetForegroundWindow(focus_window);
    flush_events();

    filter_messages = focus_window;

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    device = create_device(d3d8, focus_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device = create_device(d3d8, device_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    proc = SetWindowLongPtrA(focus_window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);

done:
    filter_messages = NULL;
    IDirect3D8_Release(d3d8);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d8_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_wndproc_windowed(void)
{
    struct wndproc_thread_param thread_params;
    IDirect3DDevice8 *device;
    WNDCLASSA wc = {0};
    IDirect3D8 *d3d8;
    HANDLE thread;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d8_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    trace("device_window %p, focus_window %p, dummy_window %p.\n",
            device_window, focus_window, thread_params.dummy_window);

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    if (thread_params.running_in_foreground)
    {
        tmp = GetForegroundWindow();
        ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                thread_params.dummy_window, tmp);
    }
    else
        skip("Not running in foreground, skip foreground window test\n");

    filter_messages = focus_window;

    device = create_device(d3d8, device_window, focus_window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    tmp = GetForegroundWindow();
    ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
            thread_params.dummy_window, tmp);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = NULL;

    hr = reset_device(device, device_window, FALSE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    hr = reset_device(device, device_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = focus_window;

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    filter_messages = device_window;

    device = create_device(d3d8, focus_window, focus_window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    hr = reset_device(device, focus_window, FALSE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    hr = reset_device(device, focus_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device = create_device(d3d8, device_window, focus_window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    hr = reset_device(device, device_window, FALSE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    hr = reset_device(device, device_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    filter_messages = NULL;
    IDirect3D8_Release(d3d8);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d8_test_wndproc_wc", GetModuleHandleA(NULL));
}

static inline void set_fpu_cw(WORD cw)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D8_TEST_SET_FPU_CW 1
    __asm__ volatile ("fnclex");
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D8_TEST_SET_FPU_CW 1
    __asm fnclex;
    __asm fldcw cw;
#endif
}

static inline WORD get_fpu_cw(void)
{
    WORD cw = 0;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D8_TEST_GET_FPU_CW 1
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D8_TEST_GET_FPU_CW 1
    __asm fnstcw cw;
#endif
    return cw;
}

static void test_fpu_setup(void)
{
#if defined(D3D8_TEST_SET_FPU_CW) && defined(D3D8_TEST_GET_FPU_CW)
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice8 *device;
    D3DDISPLAYMODE d3ddm;
    IDirect3D8 *d3d8;
    HWND window;
    HRESULT hr;
    WORD cw;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    ok(!!window, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    hr = IDirect3D8_GetAdapterDisplayMode(d3d8, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#x.\n", hr);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = d3ddm.Format;

    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    hr = IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    if (FAILED(hr))
    {
        skip("Failed to create a device, hr %#x.\n", hr);
        set_fpu_cw(0x37f);
        goto done;
    }

    cw = get_fpu_cw();
    ok(cw == 0x7f, "cw is %#x, expected 0x7f.\n", cw);

    IDirect3DDevice8_Release(device);

    cw = get_fpu_cw();
    ok(cw == 0x7f, "cw is %#x, expected 0x7f.\n", cw);
    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    hr = IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &present_parameters, &device);
    ok(SUCCEEDED(hr), "CreateDevice failed, hr %#x.\n", hr);

    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);
    set_fpu_cw(0x37f);

    IDirect3DDevice8_Release(device);

done:
    DestroyWindow(window);
    IDirect3D8_Release(d3d8);
#endif
}

static void test_ApplyStateBlock(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    HWND window;
    HRESULT hr;
    DWORD received, token;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    IDirect3DDevice8_BeginStateBlock(device);
    IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, TRUE);
    IDirect3DDevice8_EndStateBlock(device, &token);
    ok(token, "Received zero stateblock handle.\n");
    IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);

    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_ZENABLE, &received);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#x.\n", hr);
    ok(!received, "Expected = FALSE, received TRUE.\n");

    hr = IDirect3DDevice8_ApplyStateBlock(device, 0);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_ZENABLE, &received);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#x.\n", hr);
    ok(!received, "Expected FALSE, received TRUE.\n");

    hr = IDirect3DDevice8_ApplyStateBlock(device, token);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderState(device, D3DRS_ZENABLE, &received);
    ok(hr == D3D_OK, "Expected D3D_OK, received %#x.\n", hr);
    ok(received, "Expected TRUE, received FALSE.\n");

    IDirect3DDevice8_DeleteStateBlock(device, token);
    IDirect3DDevice8_Release(device);
cleanup:
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_depth_stencil_size(void)
{
    IDirect3DDevice8 *device;
    IDirect3DSurface8 *ds, *rt, *ds_bigger, *ds_bigger2;
    IDirect3DSurface8 *surf;
    IDirect3D8 *d3d8;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    device = create_device(d3d8, hwnd, hwnd, TRUE);
    if (!device) goto cleanup;

    hr = IDirect3DDevice8_CreateRenderTarget(device, 64, 64, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, FALSE, &rt);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 32, 32, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, &ds);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, &ds_bigger);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24X8, D3DMULTISAMPLE_NONE, &ds_bigger2);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateDepthStencilSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_SetRenderTarget returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds_bigger);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_SetRenderTarget failed, hr %#x.\n", hr);

    /* try to set the small ds without changing the render target at the same time */
    hr = IDirect3DDevice8_SetRenderTarget(device, NULL, ds);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_SetRenderTarget returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    hr = IDirect3DDevice8_SetRenderTarget(device, NULL, ds_bigger2);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_SetRenderTarget failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderTarget(device, &surf);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetRenderTarget failed, hr %#x.\n", hr);
    ok(surf == rt, "The render target is %p, expected %p\n", surf, rt);
    IDirect3DSurface8_Release(surf);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surf);
    ok(hr == D3D_OK, "IDirect3DDevice8_GetDepthStencilSurface failed, hr %#x.\n", hr);
    ok(surf == ds_bigger2, "The depth stencil is %p, expected %p\n", surf, ds_bigger2);
    IDirect3DSurface8_Release(surf);

    hr = IDirect3DDevice8_SetRenderTarget(device, NULL, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_SetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &surf);
    ok(FAILED(hr), "IDirect3DDevice8_GetDepthStencilSurface should have failed, hr %#x.\n", hr);
    ok(surf == NULL, "The depth stencil is %p, expected NULL\n", surf);
    if (surf) IDirect3DSurface8_Release(surf);

    IDirect3DSurface8_Release(rt);
    IDirect3DSurface8_Release(ds);
    IDirect3DSurface8_Release(ds_bigger);
    IDirect3DSurface8_Release(ds_bigger2);

cleanup:
    IDirect3D8_Release(d3d8);
    DestroyWindow(hwnd);
}

static void test_window_style(void)
{
    RECT focus_rect, fullscreen_rect, r;
    LONG device_style, device_exstyle;
    LONG focus_style, focus_exstyle;
    LONG style, expected_style;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    HRESULT hr;
    ULONG ref;

    focus_window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    device_style = GetWindowLongA(device_window, GWL_STYLE);
    device_exstyle = GetWindowLongA(device_window, GWL_EXSTYLE);
    focus_style = GetWindowLongA(focus_window, GWL_STYLE);
    focus_exstyle = GetWindowLongA(focus_window, GWL_EXSTYLE);

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(focus_window, &focus_rect);

    device = create_device(d3d8, device_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    style = GetWindowLongA(device_window, GWL_STYLE);
    expected_style = device_style | WS_VISIBLE;
    todo_wine ok(style == expected_style, "Expected device window style %#x, got %#x.\n",
            expected_style, style);
    style = GetWindowLongA(device_window, GWL_EXSTYLE);
    expected_style = device_exstyle | WS_EX_TOPMOST;
    todo_wine ok(style == expected_style, "Expected device window extended style %#x, got %#x.\n",
            expected_style, style);

    style = GetWindowLongA(focus_window, GWL_STYLE);
    ok(style == focus_style, "Expected focus window style %#x, got %#x.\n",
            focus_style, style);
    style = GetWindowLongA(focus_window, GWL_EXSTYLE);
    ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x.\n",
            focus_exstyle, style);

    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);
    GetClientRect(device_window, &r);
    todo_wine ok(!EqualRect(&r, &fullscreen_rect), "Client rect and window rect are equal.\n");
    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &focus_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            focus_rect.left, focus_rect.top, focus_rect.right, focus_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = reset_device(device, device_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    style = GetWindowLongA(device_window, GWL_STYLE);
    expected_style = device_style | WS_VISIBLE;
    ok(style == expected_style, "Expected device window style %#x, got %#x.\n",
            expected_style, style);
    style = GetWindowLongA(device_window, GWL_EXSTYLE);
    expected_style = device_exstyle | WS_EX_TOPMOST;
    ok(style == expected_style, "Expected device window extended style %#x, got %#x.\n",
            expected_style, style);

    style = GetWindowLongA(focus_window, GWL_STYLE);
    ok(style == focus_style, "Expected focus window style %#x, got %#x.\n",
            focus_style, style);
    style = GetWindowLongA(focus_window, GWL_EXSTYLE);
    ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x.\n",
            focus_exstyle, style);


    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D8_Release(d3d8);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
}

static void test_wrong_shader(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    DWORD vs, ps;
    HWND window;
    HRESULT hr;

    static const DWORD vs_2_0[] =
    {
        0xfffe0200,                                         /* vs_2_0           */
        0x0200001f, 0x80000000, 0x900f0000,                 /* dcl_position v0  */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0xd00f0000, 0x80e40001, 0xa0e40002,     /* add oD0, r1, c2  */
        0x02000001, 0xc00f0000, 0x90e40000,                 /* mov oPos, v0     */
        0x0000ffff                                          /* end              */
    };
    static const DWORD ps_2_0[] =
    {
        0xffff0200,                                         /* ps_2_0           */
        0x02000001, 0x800f0001, 0xa0e40001,                 /* mov r1, c1       */
        0x03000002, 0x800f0000, 0x80e40001, 0xa0e40002,     /* add r0, r1, c2   */
        0x02000001, 0x800f0800, 0x80e40000,                 /* mov oC0, r0      */
        0x0000ffff                                          /* end              */
    };

    static const DWORD decl[] =
    {
        D3DVSD_STREAM(0),
        D3DVSD_REG(D3DVSDE_POSITION, D3DVSDT_FLOAT3),
        D3DVSD_END()
    };

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateVertexShader(device, decl, simple_ps, &vs, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_CreatePixelShader(device, simple_vs, &ps);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreatePixelShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_CreateVertexShader(device, decl, vs_2_0, &vs, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreateVertexShader returned %#08x\n", hr);

    hr = IDirect3DDevice8_CreatePixelShader(device, ps_2_0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice8_CreatePixelShader returned %#08x\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_mode_change(void)
{
    RECT fullscreen_rect, focus_rect, r;
    IDirect3DSurface8 *backbuffer;
    IDirect3DDevice8 *device;
    D3DSURFACE_DESC desc;
    IDirect3D8 *d3d8;
    DEVMODEW devmode;
    UINT refcount;
    HRESULT hr;
    DWORD ret;

    focus_window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(focus_window, &focus_rect);

    device = create_device(d3d8, device_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = 640;
    devmode.dmPelsHeight = 480;

    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", ret);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == 640, "Got unexpect width %u.\n", devmode.dmPelsWidth);
    ok(devmode.dmPelsHeight == 480, "Got unexpect height %u.\n", devmode.dmPelsHeight);

    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);
    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &focus_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            focus_rect.left, focus_rect.top, focus_rect.right, focus_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(backbuffer, &desc);
    ok(SUCCEEDED(hr), "Failed to get backbuffer desc, hr %#x.\n", hr);
    ok(desc.Width == screen_width, "Got unexpected backbuffer width %u.\n", desc.Width);
    ok(desc.Height == screen_height, "Got unexpected backbuffer height %u.\n", desc.Height);
    IDirect3DSurface8_Release(backbuffer);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == screen_width, "Got unexpect width %u.\n", devmode.dmPelsWidth);
    ok(devmode.dmPelsHeight == screen_height, "Got unexpect height %u.\n", devmode.dmPelsHeight);

done:
    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    IDirect3D8_Release(d3d8);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == screen_width, "Got unexpect width %u.\n", devmode.dmPelsWidth);
    ok(devmode.dmPelsHeight == screen_height, "Got unexpect height %u.\n", devmode.dmPelsHeight);
}

static void test_device_window_reset(void)
{
    RECT fullscreen_rect, device_rect, r;
    IDirect3DDevice8 *device;
    WNDCLASSA wc = {0};
    IDirect3D8 *d3d8;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d8_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    focus_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d8_test_wndproc_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(device_window, &device_rect);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    device = create_device(d3d8, NULL, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);
    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &device_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            device_rect.left, device_rect.top, device_rect.right, device_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    hr = reset_device(device, device_window, FALSE);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);
    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected {%d, %d, %d, %d}, got {%d, %d, %d, %d}.\n",
            fullscreen_rect.left, fullscreen_rect.top, fullscreen_rect.right, fullscreen_rect.bottom,
            r.left, r.top, r.right, r.bottom);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D8_Release(d3d8);
    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d8_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void depth_blit_test(void)
{
    IDirect3DDevice8 *device = NULL;
    IDirect3DSurface8 *backbuffer, *ds1, *ds2, *ds3;
    RECT src_rect;
    const POINT dst_point = {0, 0};
    IDirect3D8 *d3d8;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "Failed to create a window.\n");
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    device = create_device(d3d8, hwnd, hwnd, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetRenderTarget(device, &backbuffer);
    ok(SUCCEEDED(hr), "GetRenderTarget failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &ds1);
    ok(SUCCEEDED(hr), "GetDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, &ds2);
    ok(SUCCEEDED(hr), "CreateDepthStencilSurface failed, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 640, 480, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, &ds3);
    ok(SUCCEEDED(hr), "CreateDepthStencilSurface failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    /* Partial blit. */
    SetRect(&src_rect, 0, 0, 320, 240);
    hr = IDirect3DDevice8_CopyRects(device, ds1, &src_rect, 1, ds2, &dst_point);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Flipped. */
    SetRect(&src_rect, 0, 480, 640, 0);
    hr = IDirect3DDevice8_CopyRects(device, ds1, &src_rect, 1, ds2, &dst_point);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Full, explicit. */
    SetRect(&src_rect, 0, 0, 640, 480);
    hr = IDirect3DDevice8_CopyRects(device, ds1, &src_rect, 1, ds2, &dst_point);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Depth -> color blit.*/
    hr = IDirect3DDevice8_CopyRects(device, ds1, &src_rect, 1, backbuffer, &dst_point);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Full, NULL rects, current depth stencil -> unbound depth stencil */
    hr = IDirect3DDevice8_CopyRects(device, ds1, NULL, 0, ds2, NULL);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Full, NULL rects, unbound depth stencil -> current depth stencil */
    hr = IDirect3DDevice8_CopyRects(device, ds2, NULL, 0, ds1, NULL);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
    /* Full, NULL rects, unbound depth stencil -> unbound depth stencil */
    hr = IDirect3DDevice8_CopyRects(device, ds2, NULL, 0, ds3, NULL);
    ok(hr == D3DERR_INVALIDCALL, "CopyRects returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);

    IDirect3DSurface8_Release(backbuffer);
    IDirect3DSurface8_Release(ds3);
    IDirect3DSurface8_Release(ds2);
    IDirect3DSurface8_Release(ds1);

done:
    if (device) IDirect3DDevice8_Release(device);
    IDirect3D8_Release(d3d8);
    DestroyWindow(hwnd);
}

static void test_reset_resources(void)
{
    IDirect3DSurface8 *surface, *rt;
    IDirect3DTexture8 *texture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, &surface);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "Failed to create render target texture, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &rt);
    ok(SUCCEEDED(hr), "Failed to get surface, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, surface);
    ok(SUCCEEDED(hr), "Failed to set render target surface, hr %#x.\n", hr);
    IDirect3DSurface8_Release(rt);
    IDirect3DSurface8_Release(surface);

    hr = reset_device(device, device_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice8_GetBackBuffer(device, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice8_GetRenderTarget(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface8_Release(surface);
    IDirect3DSurface8_Release(rt);

    ref = IDirect3DDevice8_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_set_rt_vp_scissor(void)
{
    IDirect3DDevice8 *device;
    IDirect3DSurface8 *rt;
    IDirect3D8 *d3d8;
    DWORD stateblock;
    D3DVIEWPORT8 vp;
    UINT refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, FALSE, &rt);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == screen_width, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == screen_height, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice8_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, NULL);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DeleteStateBlock(device, stateblock);
    ok(SUCCEEDED(hr), "Failed to delete stateblock, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, NULL);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    vp.X = 10;
    vp.Y = 20;
    vp.Width = 30;
    vp.Height = 40;
    vp.MinZ = 0.25f;
    vp.MaxZ = 0.75f;
    hr = IDirect3DDevice8_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, NULL);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    IDirect3DSurface8_Release(rt);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_validate_vs(void)
{
    static DWORD vs[] =
    {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000,                         /* dp4 oPos.x, v0, c0           */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001,                         /* dp4 oPos.y, v0, c1           */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002,                         /* dp4 oPos.z, v0, c2           */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003,                         /* dp4 oPos.w, v0, c3           */
        0x0000ffff,                                                             /* end                          */
    };
    HRESULT hr;

    hr = ValidateVertexShader(0, 0, 0, 0, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = ValidateVertexShader(0, 0, 0, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = ValidateVertexShader(vs, 0, 0, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ValidateVertexShader(vs, 0, 0, 1, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* Seems to do some version checking. */
    *vs = 0xfffe0100;                                                           /* vs_1_0                       */
    hr = ValidateVertexShader(vs, 0, 0, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    *vs = 0xfffe0102;                                                           /* bogus version                */
    hr = ValidateVertexShader(vs, 0, 0, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    /* I've seen that applications always pass the 2nd and 3rd parameter as 0.
     * Simple test with non-zero parameters. */
    *vs = 0xfffe0101;                                                           /* vs_1_1                       */
    hr = ValidateVertexShader(vs, vs, 0, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = ValidateVertexShader(vs, 0, vs, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    /* I've seen the 4th parameter always passed as either 0 or 1, but passing
     * other values doesn't seem to hurt. */
    hr = ValidateVertexShader(vs, 0, 0, 12345, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* What is the 5th parameter? The following seems to work ok. */
    hr = ValidateVertexShader(vs, 0, 0, 1, vs);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

static void test_validate_ps(void)
{
    static DWORD ps[] =
    {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
        0x00000042, 0xb00f0000,                                                 /* tex t0                       */
        0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
        0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
        0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
        0x0000ffff,                                                             /* end                          */
    };
    HRESULT hr;

    hr = ValidatePixelShader(0, 0, 0, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = ValidatePixelShader(0, 0, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = ValidatePixelShader(ps, 0, 0, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ValidatePixelShader(ps, 0, 1, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* Seems to do some version checking. */
    *ps = 0xffff0105;                                                           /* bogus version                */
    hr = ValidatePixelShader(ps, 0, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    /* I've seen that applications always pass the 2nd parameter as 0.
     * Simple test with a non-zero parameter. */
    *ps = 0xffff0101;                                                           /* ps_1_1                       */
    hr = ValidatePixelShader(ps, ps, 1, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    /* I've seen th 3rd parameter always passed as either 0 or 1, but passing
     * other values doesn't seem to hurt. */
    hr = ValidatePixelShader(ps, 0, 12345, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    /* What is the 4th parameter? The following seems to work ok. */
    hr = ValidatePixelShader(ps, 0, 1, ps);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

static void test_volume_get_container(void)
{
    IDirect3DVolumeTexture8 *texture = NULL;
    IDirect3DVolume8 *volume = NULL;
    IDirect3DDevice8 *device;
    IUnknown *container;
    IDirect3D8 *d3d8;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No volume texture support, skipping tests.\n");
        IDirect3DDevice8_Release(device);
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateVolumeTexture(device, 128, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DVolumeTexture8_GetVolumeLevel(texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    ok(!!volume, "Got unexpected volume %p.\n", volume);

    /* These should work... */
    container = NULL;
    hr = IDirect3DVolume8_GetContainer(volume, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume8_GetContainer(volume, &IID_IDirect3DResource8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume8_GetContainer(volume, &IID_IDirect3DBaseTexture8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume8_GetContainer(volume, &IID_IDirect3DVolumeTexture8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DVolume8_GetContainer(volume, &IID_IDirect3DVolume8, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DVolume8_Release(volume);
    IDirect3DVolumeTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_vb_lock_flags(void)
{
    static const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT result;
    }
    test_data[] =
    {
        {D3DLOCK_READONLY,                          "D3DLOCK_READONLY",                         D3D_OK            },
        {D3DLOCK_DISCARD,                           "D3DLOCK_DISCARD",                          D3D_OK            },
        {D3DLOCK_NOOVERWRITE,                       "D3DLOCK_NOOVERWRITE",                      D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD,     "D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD",    D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY,    "D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY",   D3D_OK            },
        {D3DLOCK_READONLY    | D3DLOCK_DISCARD,     "D3DLOCK_READONLY | D3DLOCK_DISCARD",       D3D_OK            },
        /* Completely bogus flags aren't an error. */
        {0xdeadbeef,                                "0xdeadbeef",                               D3D_OK            },
    };
    IDirect3DVertexBuffer8 *buffer;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BYTE *data;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateVertexBuffer(device, 1024, D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &buffer);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); ++i)
    {
        hr = IDirect3DVertexBuffer8_Lock(buffer, 0, 0, &data, test_data[i].flags);
        ok(hr == test_data[i].result, "Got unexpected hr %#x for %s.\n",
                hr, test_data[i].debug_string);
        if (SUCCEEDED(hr))
        {
            ok(!!data, "Got unexpected data %p.\n", data);
            hr = IDirect3DVertexBuffer8_Unlock(buffer);
            ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
        }
    }

    IDirect3DVertexBuffer8_Release(buffer);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

/* Test the default texture stage state values */
static void test_texture_stage_states(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    unsigned int i;
    ULONG refcount;
    D3DCAPS8 caps;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    for (i = 0; i < caps.MaxTextureBlendStages; ++i)
    {
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_COLOROP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_MODULATE),
                "Got unexpected value %#x for D3DTSS_COLOROP, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_COLORARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_COLORARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_COLORARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_ALPHAOP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_SELECTARG1),
                "Got unexpected value %#x for D3DTSS_ALPHAOP, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_ALPHAARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_ALPHAARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_ALPHAARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT00, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT00, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT01, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT01, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT10, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT10, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT11, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT11, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_TEXCOORDINDEX, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == i, "Got unexpected value %#x for D3DTSS_TEXCOORDINDEX, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVLSCALE, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLSCALE, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_BUMPENVLOFFSET, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLOFFSET, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_TEXTURETRANSFORMFLAGS, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTTFF_DISABLE,
                "Got unexpected value %#x for D3DTSS_TEXTURETRANSFORMFLAGS, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_COLORARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_ALPHAARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice8_GetTextureStageState(device, i, D3DTSS_RESULTARG, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_RESULTARG, stage %u.\n", value, i);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_cube_textures(void)
{
    IDirect3DCubeTexture8 *texture;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    ULONG refcount;
    D3DCAPS8 caps;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_DEFAULT cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture8_Release(texture);
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_MANAGED cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture8_Release(texture);
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_SYSTEMMEM cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture8_Release(texture);
    }
    else
    {
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_DEFAULT cube texture.\n", hr);
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_MANAGED cube texture.\n", hr);
        hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_SYSTEMMEM cube texture.\n", hr);
    }
    hr = IDirect3DDevice8_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SCRATCH, &texture);
    ok(hr == D3D_OK, "Failed to create D3DPOOL_SCRATCH cube texture, hr %#x.\n", hr);
    IDirect3DCubeTexture8_Release(texture);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

/* Test the behaviour of the IDirect3DDevice8::CreateImageSurface() method.
 *
 * The expected behaviour (as documented in the original DX8 docs) is that the
 * call returns a surface in the SYSTEMMEM pool. Games like Max Payne 1 and 2
 * depend on this behaviour.
 *
 * A short remark in the DX9 docs however states that the pool of the returned
 * surface object is D3DPOOL_SCRATCH. This is misinformation and would result
 * in screenshots not appearing in the savegame loading menu of both games
 * mentioned above (engine tries to display a texture from the scratch pool).
 *
 * This test verifies that the behaviour described in the original d3d8 docs
 * is the correct one. For more information about this issue, see the MSDN:
 *     d3d9 docs: "Converting to Direct3D 9"
 *     d3d9 reference: "IDirect3DDevice9::CreateOffscreenPlainSurface"
 *     d3d8 reference: "IDirect3DDevice8::CreateImageSurface" */
static void test_image_surface_pool(void)
{
    IDirect3DSurface8 *surface;
    IDirect3DDevice8 *device;
    D3DSURFACE_DESC desc;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 128, 128, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(desc.Pool == D3DPOOL_SYSTEMMEM, "Got unexpected pool %#x.\n", desc.Pool);
    IDirect3DSurface8_Release(surface);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_surface_get_container(void)
{
    IDirect3DTexture8 *texture = NULL;
    IDirect3DSurface8 *surface = NULL;
    IDirect3DDevice8 *device;
    IUnknown *container;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    ok(!!surface, "Got unexpected surface %p.\n", surface);

    /* These should work... */
    container = NULL;
    hr = IDirect3DSurface8_GetContainer(surface, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface8_GetContainer(surface, &IID_IDirect3DResource8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface8_GetContainer(surface, &IID_IDirect3DBaseTexture8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface8_GetContainer(surface, &IID_IDirect3DTexture8, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DSurface8_GetContainer(surface, &IID_IDirect3DSurface8, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DSurface8_Release(surface);
    IDirect3DTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_lockrect_invalid(void)
{
    static const RECT valid[] =
    {
        {60, 60, 68, 68},
        {120, 60, 128, 68},
        {60, 120, 68, 128},
    };
    static const RECT invalid[] =
    {
        {60, 60, 60, 68},       /* 0 height */
        {60, 60, 68, 60},       /* 0 width */
        {68, 60, 60, 68},       /* left > right */
        {60, 68, 68, 60},       /* top > bottom */
        {-8, 60,  0, 68},       /* left < surface */
        {60, -8, 68,  0},       /* top < surface */
        {-16, 60, -8, 68},      /* right < surface */
        {60, -16, 68, -8},      /* bottom < surface */
        {60, 60, 136, 68},      /* right > surface */
        {60, 60, 68, 136},      /* bottom > surface */
        {136, 60, 144, 68},     /* left > surface */
        {60, 136, 68, 144},     /* top > surface */
    };
    IDirect3DSurface8 *surface = NULL;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    unsigned int i;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 128, 128, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    base = locked_rect.pBits;
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(valid) / sizeof(*valid)); ++i)
    {
        unsigned int offset, expected_offset;
        const RECT *rect = &valid[i];

        locked_rect.pBits = (BYTE *)0xdeadbeef;
        locked_rect.Pitch = 0xdeadbeef;

        hr = IDirect3DSurface8_LockRect(surface, &locked_rect, rect, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface with rect [%d, %d]->[%d, %d], hr %#x.\n",
                rect->left, rect->top, rect->right, rect->bottom, hr);

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
        ok(offset == expected_offset,
                "Got unexpected offset %u (expected %u) for rect [%d, %d]->[%d, %d].\n",
                offset, expected_offset, rect->left, rect->top, rect->right, rect->bottom);

        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    for (i = 0; i < (sizeof(invalid) / sizeof(*invalid)); ++i)
    {
        const RECT *rect = &invalid[i];

        hr = IDirect3DSurface8_LockRect(surface, &locked_rect, rect, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
                hr, rect->left, rect->top, rect->right, rect->bottom);
    }

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface with rect NULL, hr %#x.\n", hr);
    locked_rect.pBits = (void *)0xdeadbeef;
    locked_rect.Pitch = 1;
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!locked_rect.pBits, "Got unexpected pBits %p.\n", locked_rect.pBits);
    ok(!locked_rect.Pitch, "Got unexpected Pitch %u.\n", locked_rect.Pitch);
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[0], 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, valid[0].left, valid[0].top, valid[0].right, valid[0].bottom);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[0], 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, valid[0].left, valid[0].top, valid[0].right, valid[0].bottom);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[1], 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, valid[1].left, valid[1].top, valid[1].right, valid[1].bottom);
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface8_Release(surface);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDirect3DTexture8 *texture;
    IDirect3DSurface8 *surface, *surface2;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    IUnknown *ptr;
    HWND window;
    HRESULT hr;
    DWORD size;
    DWORD data[4] = {1, 2, 3, 4};
    static const GUID d3d8_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3,0x7f,0x9b,0x1d,0xf4,0x88,0xc5,0xfc}
    };
    static const GUID d3d8_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b,0x4b,0x89,0xd7,0xd1,0x12,0xe7,0x2b}
    };

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 4, 4, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, 0, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, 5, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, sizeof(IUnknown *) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* A failing SetPrivateData call does not clear the old data with the same tag. */
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid, device,
            sizeof(device), D3DSPD_IUNKNOWN);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid, device,
            sizeof(device) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr);
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    IUnknown_Release(ptr);
    hr = IDirect3DSurface8_FreePrivateData(surface, &d3d8_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)device);
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDirect3DSurface8_FreePrivateData(surface, &d3d8_private_data_test_guid);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount - 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            surface, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    size = 2 * sizeof(ptr);
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, &ptr, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    expected_refcount = refcount + 2;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ok(ptr == (IUnknown *)device, "Got unexpected ptr %p, expected %p.\n", ptr, device);
    IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, NULL, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, NULL, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    size = 1;
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, &ptr, &size);
    ok(hr == D3DERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid2, NULL, NULL);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid2, &ptr, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    ok(size == 0xdeadbabe, "Got unexpected size %u.\n", size);
    /* GetPrivateData with size = NULL causes an access violation on Windows if the
     * requested data exists. */

    /* Destroying the surface frees the held reference. */
    IDirect3DSurface8_Release(surface);
    expected_refcount = refcount - 2;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DDevice8_CreateTexture(device, 4, 4, 2, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get texture level 0, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 1, &surface2);
    ok(SUCCEEDED(hr), "Failed to get texture level 1, hr %#x.\n", hr);

    hr = IDirect3DTexture8_SetPrivateData(texture, &d3d8_private_data_test_guid, data, sizeof(data), 0);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);

    memset(data, 0, sizeof(data));
    size = sizeof(data);
    hr = IDirect3DSurface8_GetPrivateData(surface, &d3d8_private_data_test_guid, data, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetPrivateData(texture, &d3d8_private_data_test_guid, data, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    ok(data[0] == 1 && data[1] == 2 && data[2] == 3 && data[3] == 4,
            "Got unexpected private data: %u, %u, %u, %u.\n", data[0], data[1], data[2], data[3]);

    hr = IDirect3DTexture8_FreePrivateData(texture, &d3d8_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    hr = IDirect3DSurface8_SetPrivateData(surface, &d3d8_private_data_test_guid, data, sizeof(data), 0);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetPrivateData(surface2, &d3d8_private_data_test_guid, data, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface8_FreePrivateData(surface, &d3d8_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    IDirect3DSurface8_Release(surface2);
    IDirect3DSurface8_Release(surface);
    IDirect3DTexture8_Release(texture);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_surface_dimensions(void)
{
    IDirect3DSurface8 *surface;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateImageSurface(device, 0, 1, D3DFMT_A8R8G8B8, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_CreateImageSurface(device, 1, 0, D3DFMT_A8R8G8B8, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_surface_format_null(void)
{
    static const D3DFORMAT D3DFMT_NULL = MAKEFOURCC('N','U','L','L');
    IDirect3DTexture8 *texture;
    IDirect3DSurface8 *surface;
    IDirect3DSurface8 *rt, *ds;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    D3DSURFACE_DESC desc;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, D3DFMT_NULL);
    if (hr != D3D_OK)
    {
        skip("No D3DFMT_NULL support, skipping test.\n");
        IDirect3D8_Release(d3d);
        return;
    }

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_NULL);
    ok(hr == D3D_OK, "D3DFMT_NULL should be supported for render target textures, hr %#x.\n", hr);

    hr = IDirect3D8_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DFMT_NULL, D3DFMT_D24S8);
    ok(SUCCEEDED(hr), "Depth stencil match failed for D3DFMT_NULL, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateRenderTarget(device, 128, 128, D3DFMT_NULL,
            D3DMULTISAMPLE_NONE, TRUE, &surface);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get original render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get original depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, surface, NULL);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetRenderTarget(device, rt, ds);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    IDirect3DSurface8_Release(rt);
    IDirect3DSurface8_Release(ds);

    hr = IDirect3DSurface8_GetDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(desc.Width == 128, "Expected width 128, got %u.\n", desc.Width);
    ok(desc.Height == 128, "Expected height 128, got %u.\n", desc.Height);

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(locked_rect.Pitch, "Expected non-zero pitch, got %u.\n", locked_rect.Pitch);
    ok(!!locked_rect.pBits, "Expected non-NULL pBits, got %p.\n", locked_rect.pBits);

    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface8_Release(surface);

    hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 0, D3DUSAGE_RENDERTARGET,
            D3DFMT_NULL, D3DPOOL_DEFAULT, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    IDirect3DTexture8_Release(texture);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_double_unlock(void)
{
    static const D3DPOOL pools[] =
    {
        D3DPOOL_DEFAULT,
        D3DPOOL_SYSTEMMEM,
    };
    IDirect3DSurface8 *surface;
    IDirect3DDevice8 *device;
    D3DLOCKED_RECT lr;
    IDirect3D8 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < (sizeof(pools) / sizeof(*pools)); ++i)
    {
        switch (pools[i])
        {
            case D3DPOOL_DEFAULT:
                hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                        D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, D3DFMT_X8R8G8B8);
                if (FAILED(hr))
                {
                    skip("D3DFMT_X8R8G8B8 render targets not supported, skipping double unlock DEFAULT pool test.\n");
                    continue;
                }

                hr = IDirect3DDevice8_CreateRenderTarget(device, 64, 64, D3DFMT_X8R8G8B8,
                        D3DMULTISAMPLE_NONE, TRUE, &surface);
                ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
                break;

            case D3DPOOL_SYSTEMMEM:
                hr = IDirect3DDevice8_CreateImageSurface(device, 64, 64, D3DFMT_X8R8G8B8, &surface);
                ok(SUCCEEDED(hr), "Failed to create image surface, hr %#x.\n", hr);
                break;

            default:
                break;
        }

        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);
        hr = IDirect3DSurface8_LockRect(surface, &lr, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);

        IDirect3DSurface8_Release(surface);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_blocks(void)
{
    static const struct
    {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        BOOL broken;
        BOOL create_size_checked, core_fmt;
    }
    formats[] =
    {
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, FALSE, TRUE,  TRUE },
        /* ATI2N has 2x2 blocks on all AMD cards and Geforce 7 cards,
         * which doesn't match the format spec. On newer Nvidia cards
         * it has the correct 4x4 block size */
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       4, 4, TRUE,  FALSE, FALSE},
        /* Windows drivers generally enforce block-aligned locks for
         * YUY2 and UYVY. The notable exception is the AMD r500 driver
         * in d3d8. The same driver checks the sizes in d3d9. */
        {D3DFMT_YUY2,                 "D3DFMT_YUY2", 2, 1, TRUE,  FALSE, TRUE },
        {D3DFMT_UYVY,                 "D3DFMT_UYVY", 2, 1, TRUE,  FALSE, TRUE },
    };
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        /* Don't check the return value, Nvidia returns D3DERR_INVALIDCALL for some formats
         * and E_INVALIDARG/DDERR_INVALIDPARAMS for others. */
        BOOL success;
    }
    pools[] =
    {
        {D3DPOOL_DEFAULT,       "D3DPOOL_DEFAULT",  FALSE},
        {D3DPOOL_SCRATCH,       "D3DPOOL_SCRATCH",  TRUE},
        {D3DPOOL_SYSTEMMEM,     "D3DPOOL_SYSTEMMEM",TRUE},
        {D3DPOOL_MANAGED,       "D3DPOOL_MANAGED",  TRUE},
    };
    static struct
    {
        D3DRESOURCETYPE rtype;
        const char *type_name;
        D3DPOOL pool;
        const char *pool_name;
        BOOL need_driver_support, need_runtime_support;
    }
    create_tests[] =
    {
        /* D3d8 only supports sysmem surfaces, which are created via CreateImageSurface. Other tests confirm
         * that they are D3DPOOL_SYSTEMMEM surfaces, but their creation restriction behaves like the scratch
         * pool in d3d9. */
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", FALSE, TRUE },

        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE },
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE },
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE },
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE  },

        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE },
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE },
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE },
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE  },
    };
    IDirect3DTexture8 *texture;
    IDirect3DCubeTexture8 *cube_texture;
    IDirect3DSurface8 *surface;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice8 *device;
    unsigned int i, j, w, h;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL tex_pow2, cube_pow2;
    D3DCAPS8 caps;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
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
    tex_pow2 = caps.TextureCaps & D3DPTEXTURECAPS_POW2;
    if (tex_pow2)
        tex_pow2 = !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
    cube_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2);

    for (i = 0; i < (sizeof(formats) / sizeof(*formats)); ++i)
    {
        BOOL tex_support, cube_support, surface_support, format_known;

        hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, formats[i].fmt);
        tex_support = SUCCEEDED(hr);
        hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_CUBETEXTURE, formats[i].fmt);
        cube_support = SUCCEEDED(hr);
        hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_SURFACE, formats[i].fmt);
        surface_support = SUCCEEDED(hr);

        /* Scratch pool in general allows texture creation even if the driver does
         * not support the format. If the format is an extension format that is not
         * known to the runtime, like ATI2N, some driver support is required for
         * this to work.
         *
         * It is also possible that Windows Vista and Windows 7 d3d8 runtimes know
         * about ATI2N. I cannot check this because all my Vista+ machines support
         * ATI2N in hardware, but none of my WinXP machines do. */
        format_known = tex_support || cube_support || surface_support;

        for (w = 1; w <= 8; w++)
        {
            for (h = 1; h <= 8; h++)
            {
                BOOL block_aligned = TRUE;
                BOOL size_is_pow2;

                if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                    block_aligned = FALSE;

                size_is_pow2 = !(w & (w - 1) || h & (h - 1));

                for (j = 0; j < sizeof(create_tests) / sizeof(*create_tests); j++)
                {
                    BOOL support, pow2;
                    HRESULT expect_hr;
                    BOOL may_succeed = FALSE;
                    IUnknown **check_null;

                    if (!formats[i].core_fmt)
                    {
                        /* AMD warns against creating ATI2N textures smaller than
                         * the block size because the runtime cannot calculate the
                         * correct texture size. Generalize this for all extension
                         * formats. */
                        if (w < formats[i].block_width || h < formats[i].block_height)
                            continue;
                    }

                    texture = (IDirect3DTexture8 *)0xdeadbeef;
                    cube_texture = (IDirect3DCubeTexture8 *)0xdeadbeef;
                    surface = (IDirect3DSurface8 *)0xdeadbeef;

                    switch (create_tests[j].rtype)
                    {
                        case D3DRTYPE_TEXTURE:
                            check_null = (IUnknown **)&texture;
                            hr = IDirect3DDevice8_CreateTexture(device, w, h, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &texture);
                            support = tex_support;
                            pow2 = tex_pow2;
                            break;

                        case D3DRTYPE_CUBETEXTURE:
                            if (w != h)
                                continue;
                            check_null = (IUnknown **)&cube_texture;
                            hr = IDirect3DDevice8_CreateCubeTexture(device, w, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &cube_texture);
                            support = cube_support;
                            pow2 = cube_pow2;
                            break;

                        case D3DRTYPE_SURFACE:
                            check_null = (IUnknown **)&surface;
                            hr = IDirect3DDevice8_CreateImageSurface(device, w, h,
                                    formats[i].fmt, &surface);
                            support = surface_support;
                            pow2 = FALSE;
                            break;

                        default:
                            pow2 = FALSE;
                            support = FALSE;
                            check_null = NULL;
                            break;
                    }

                    if (create_tests[j].need_driver_support && !support)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (create_tests[j].need_runtime_support && !formats[i].core_fmt && !format_known)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (formats[i].create_size_checked && !block_aligned)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (pow2 && !size_is_pow2 && create_tests[j].need_driver_support)
                        expect_hr = D3DERR_INVALIDCALL;
                    else
                        expect_hr = D3D_OK;

                    if (!formats[i].core_fmt && !format_known && FAILED(expect_hr))
                        may_succeed = TRUE;

                    /* Wine knows about ATI2N and happily creates a scratch resource even if GL
                     * does not support it. Accept scratch creation of extension formats on
                     * Windows as well if it occurs. We don't really care if e.g. a Windows 7
                     * on an r200 GPU creates scratch ATI2N texture even though the card doesn't
                     * support it. */
                    ok(hr == expect_hr || ((SUCCEEDED(hr) && may_succeed)),
                            "Got unexpected hr %#x for format %s, pool %s, type %s, size %ux%u.\n",
                            hr, formats[i].name, create_tests[j].pool_name, create_tests[j].type_name, w, h);

                    if (FAILED(hr))
                        ok(*check_null == NULL, "Got object ptr %p, expected NULL.\n", *check_null);
                    else
                        IUnknown_Release(*check_null);
                }
            }
        }

        hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, formats[i].fmt);
        if (FAILED(hr))
        {
            skip("Format %s not supported, skipping lockrect offset tests.\n", formats[i].name);
            continue;
        }

        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); ++j)
        {
            hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1,
                    pools[j].pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0,
                    formats[i].fmt, pools[j].pool, &texture);
            ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
            hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
            ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
            IDirect3DTexture8_Release(texture);

            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x for format %s, pool %s.\n", hr, formats[i].name, pools[j].name);
            hr = IDirect3DSurface8_UnlockRect(surface);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

            IDirect3DSurface8_Release(surface);
        }

        if (formats[i].block_width == 1 && formats[i].block_height == 1)
            continue;
        if (!formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice8_CreateTexture(device, formats[i].block_width, formats[i].block_height, 2,
                D3DUSAGE_DYNAMIC, formats[i].fmt, D3DPOOL_DEFAULT, &texture);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, format %s.\n", hr, formats[i].name);

        hr = IDirect3DTexture8_LockRect(texture, 1, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture8_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        rect.left = 0;
        rect.top = 0;
        rect.right = formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1;
        rect.bottom = formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1;
        hr = IDirect3DTexture8_LockRect(texture, 1, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture8_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        rect.right = formats[i].block_width;
        rect.bottom = formats[i].block_height;
        hr = IDirect3DTexture8_LockRect(texture, 1, &locked_rect, &rect, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (SUCCEEDED(hr))
            IDirect3DTexture8_UnlockRect(texture, 1);

        IDirect3DTexture8_Release(texture);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_set_palette(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    UINT refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY pal[256];
    unsigned int i;
    D3DCAPS8 caps;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < sizeof(pal) / sizeof(*pal); i++)
    {
        pal[i].peRed = i;
        pal[i].peGreen = i;
        pal[i].peBlue = i;
        pal[i].peFlags = 0xff;
    }
    hr = IDirect3DDevice8_SetPaletteEntries(device, 0, pal);
    ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    for (i = 0; i < sizeof(pal) / sizeof(*pal); i++)
    {
        pal[i].peRed = i;
        pal[i].peGreen = i;
        pal[i].peBlue = i;
        pal[i].peFlags = i;
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE)
    {
        hr = IDirect3DDevice8_SetPaletteEntries(device, 0, pal);
        ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);
    }
    else
    {
        hr = IDirect3DDevice8_SetPaletteEntries(device, 0, pal);
        ok(hr == D3DERR_INVALIDCALL, "SetPaletteEntries returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_swvp_buffer(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    UINT refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;
    IDirect3DVertexBuffer8 *buffer;
    static const unsigned int bufsize = 1024;
    D3DVERTEXBUFFER_DESC desc;
    D3DPRESENT_PARAMETERS present_parameters = {0};
    struct
    {
        float x, y, z;
    } *ptr, *ptr2;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = screen_width;
    present_parameters.BackBufferHeight = screen_height;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = FALSE;
    if (FAILED(IDirect3D8_CreateDevice(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateVertexBuffer(device, bufsize * sizeof(*ptr), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
            D3DPOOL_DEFAULT, &buffer);
    ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer8_GetDesc(buffer, &desc);
    ok(SUCCEEDED(hr), "Failed to get desc, hr %#x.\n", hr);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Got pool %u, expected D3DPOOL_DEFAULT\n", desc.Pool);
    ok(desc.Usage == (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY),
            "Got usage %u, expected D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY\n", desc.Usage);

    hr = IDirect3DVertexBuffer8_Lock(buffer, 0, bufsize * sizeof(*ptr), (BYTE **)&ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock buffer, hr %#x.\n", hr);
    for (i = 0; i < bufsize; i++)
    {
        ptr[i].x = i * 1.0f;
        ptr[i].y = i * 2.0f;
        ptr[i].z = i * 3.0f;
    }
    hr = IDirect3DVertexBuffer8_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetStreamSource(device, 0, buffer, sizeof(*ptr));
    ok(SUCCEEDED(hr), "Failed to set stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice8_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer8_Lock(buffer, 0, bufsize * sizeof(*ptr2), (BYTE **)&ptr2, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock buffer, hr %#x.\n", hr);
    ok(ptr == ptr2, "Lock returned two different pointers: %p, %p\n", ptr, ptr2);
    for (i = 0; i < bufsize; i++)
    {
        if (ptr2[i].x != i * 1.0f || ptr2[i].y != i * 2.0f || ptr2[i].z != i * 3.0f)
        {
            ok(FALSE, "Vertex %u is %f,%f,%f, expected %f,%f,%f\n", i,
                    ptr2[i].x, ptr2[i].y, ptr2[i].z, i * 1.0f, i * 2.0f, i * 3.0f);
            break;
        }
    }
    hr = IDirect3DVertexBuffer8_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer8_Release(buffer);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_npot_textures(void)
{
    IDirect3DDevice8 *device = NULL;
    IDirect3D8 *d3d8;
    ULONG refcount;
    HWND window = NULL;
    HRESULT hr;
    D3DCAPS8 caps;
    IDirect3DTexture8 *texture;
    IDirect3DCubeTexture8 *cube_texture;
    IDirect3DVolumeTexture8 *volume_texture;
    struct
    {
        D3DPOOL pool;
        const char *pool_name;
        HRESULT hr;
    }
    pools[] =
    {
        { D3DPOOL_DEFAULT,    "D3DPOOL_DEFAULT",    D3DERR_INVALIDCALL },
        { D3DPOOL_MANAGED,    "D3DPOOL_MANAGED",    D3DERR_INVALIDCALL },
        { D3DPOOL_SYSTEMMEM,  "D3DPOOL_SYSTEMMEM",  D3DERR_INVALIDCALL },
        { D3DPOOL_SCRATCH,    "D3DPOOL_SCRATCH",    D3D_OK             },
    };
    unsigned int i, levels;
    BOOL tex_pow2, cube_pow2, vol_pow2;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    tex_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_POW2);
    cube_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2);
    vol_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2);
    ok(cube_pow2 == tex_pow2, "Cube texture and 2d texture pow2 restrictions mismatch.\n");
    ok(vol_pow2 == tex_pow2, "Volume texture and 2d texture pow2 restrictions mismatch.\n");

    for (i = 0; i < sizeof(pools) / sizeof(*pools); i++)
    {
        for (levels = 0; levels <= 2; levels++)
        {
            HRESULT expected;

            hr = IDirect3DDevice8_CreateTexture(device, 10, 10, levels, 0, D3DFMT_X8R8G8B8,
                    pools[i].pool, &texture);
            if (!tex_pow2)
            {
                expected = D3D_OK;
            }
            else if (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
            {
                if (levels == 1)
                    expected = D3D_OK;
                else
                    expected = pools[i].hr;
            }
            else
            {
                expected = pools[i].hr;
            }
            ok(hr == expected, "CreateTexture(w=h=10, %s, levels=%u) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, levels, hr, expected);

            if (SUCCEEDED(hr))
                IDirect3DTexture8_Release(texture);
        }

        hr = IDirect3DDevice8_CreateCubeTexture(device, 3, 1, 0, D3DFMT_X8R8G8B8,
                pools[i].pool, &cube_texture);
        if (tex_pow2)
        {
            ok(hr == pools[i].hr, "CreateCubeTexture(EdgeLength=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, pools[i].hr);
        }
        else
        {
            ok(SUCCEEDED(hr), "CreateCubeTexture(EdgeLength=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, D3D_OK);
        }

        if (SUCCEEDED(hr))
            IDirect3DCubeTexture8_Release(cube_texture);

        hr = IDirect3DDevice8_CreateVolumeTexture(device, 2, 2, 3, 1, 0, D3DFMT_X8R8G8B8,
                pools[i].pool, &volume_texture);
        if (tex_pow2)
        {
            ok(hr == pools[i].hr, "CreateVolumeTextur(Depth=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, pools[i].hr);
        }
        else
        {
            ok(SUCCEEDED(hr), "CreateVolumeTextur(Depth=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, D3D_OK);
        }

        if (SUCCEEDED(hr))
            IDirect3DVolumeTexture8_Release(volume_texture);
    }

done:
    if (device)
    {
        refcount = IDirect3DDevice8_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);

}

static void test_volume_locking(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture8 *texture;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS8 caps;
    static const struct
    {
        D3DPOOL pool;
        DWORD usage;
        HRESULT create_hr, lock_hr;
    }
    tests[] =
    {
        { D3DPOOL_DEFAULT,      0,                  D3D_OK,             D3DERR_INVALIDCALL  },
        { D3DPOOL_DEFAULT,      D3DUSAGE_DYNAMIC,   D3D_OK,             D3D_OK              },
        { D3DPOOL_SYSTEMMEM,    0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_SYSTEMMEM,    D3DUSAGE_DYNAMIC,   D3D_OK,             D3D_OK              },
        { D3DPOOL_MANAGED,      0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_MANAGED,      D3DUSAGE_DYNAMIC,   D3DERR_INVALIDCALL, D3D_OK              },
        { D3DPOOL_SCRATCH,      0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_SCRATCH,      D3DUSAGE_DYNAMIC,   D3DERR_INVALIDCALL, D3D_OK              },
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("Volume textures not supported, skipping test.\n");
        goto out;
    }

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        hr = IDirect3DDevice8_CreateVolumeTexture(device, 4, 4, 4, 1, tests[i].usage,
                D3DFMT_A8R8G8B8, tests[i].pool, &texture);
        ok(hr == tests[i].create_hr, "Creating volume texture pool=%u, usage=%#x returned %#x, expected %#x.\n",
                tests[i].pool, tests[i].usage, hr, tests[i].create_hr);
        if (FAILED(hr))
            continue;

        locked_box.pBits = (void *)0xdeadbeef;
        hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, NULL, 0);
        ok(hr == tests[i].lock_hr, "Lock returned %#x, expected %#x.\n", hr, tests[i].lock_hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        else
        {
            ok (locked_box.pBits == NULL, "Failed lock set pBits = %p, expected NULL.\n", locked_box.pBits);
        }
        IDirect3DVolumeTexture8_Release(texture);
    }

out:
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_update_volumetexture(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture8 *src, *dst;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS8 caps;
    static const struct
    {
        D3DPOOL src_pool, dst_pool;
        HRESULT hr;
    }
    tests[] =
    {
        { D3DPOOL_DEFAULT,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_DEFAULT,    D3D_OK              },
        { D3DPOOL_SCRATCH,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
    };
    static const struct
    {
        UINT src_size, dst_size;
        UINT src_lvl, dst_lvl;
        D3DFORMAT src_fmt, dst_fmt;
    }
    tests2[] =
    {
        { 8, 8, 0, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 2, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 4, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 1, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 }, /* Different level count */
        { 4, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 }, /* Different size        */
        { 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8 }, /* Different format      */
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("Volume textures not supported, skipping test.\n");
        goto out;
    }

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        DWORD src_usage = tests[i].src_pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0;
        DWORD dst_usage = tests[i].dst_pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0;

        hr = IDirect3DDevice8_CreateVolumeTexture(device, 1, 1, 1, 1, src_usage,
                D3DFMT_A8R8G8B8, tests[i].src_pool, &src);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DDevice8_CreateVolumeTexture(device, 1, 1, 1, 1, dst_usage,
                D3DFMT_A8R8G8B8, tests[i].dst_pool, &dst);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        hr = IDirect3DVolumeTexture8_LockBox(src, 0, &locked_box, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        *((DWORD *)locked_box.pBits) = 0x11223344;
        hr = IDirect3DVolumeTexture8_UnlockBox(src, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)src, (IDirect3DBaseTexture8 *)dst);
        ok(hr == tests[i].hr, "UpdateTexture returned %#x, expected %#x, src pool %x, dst pool %u.\n",
                hr, tests[i].hr, tests[i].src_pool, tests[i].dst_pool);

        if (SUCCEEDED(hr))
        {
            DWORD content = *((DWORD *)locked_box.pBits);
            hr = IDirect3DVolumeTexture8_LockBox(dst, 0, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
            ok(content == 0x11223344, "Dest texture contained %#x, expected 0x11223344.\n", content);
            hr = IDirect3DVolumeTexture8_UnlockBox(dst, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        IDirect3DVolumeTexture8_Release(src);
        IDirect3DVolumeTexture8_Release(dst);
    }

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
    {
        skip("Mipmapped volume maps not supported.\n");
        goto out;
    }

    for (i = 0; i < sizeof(tests2) / sizeof(*tests2); i++)
    {
        hr = IDirect3DDevice8_CreateVolumeTexture(device,
                tests2[i].src_size, tests2[i].src_size, tests2[i].src_size,
                tests2[i].src_lvl, 0, tests2[i].src_fmt, D3DPOOL_SYSTEMMEM, &src);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);
        hr = IDirect3DDevice8_CreateVolumeTexture(device,
                tests2[i].dst_size, tests2[i].dst_size, tests2[i].dst_size,
                tests2[i].dst_lvl, 0, tests2[i].dst_fmt, D3DPOOL_DEFAULT, &dst);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);

        hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)src, (IDirect3DBaseTexture8 *)dst);
        if (FAILED(hr))
            todo_wine ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u.\n", hr, i);
        else
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u.\n", hr, i);

        IDirect3DVolumeTexture8_Release(src);
        IDirect3DVolumeTexture8_Release(dst);
    }

    /* As far as I can see, UpdateTexture on non-matching texture behaves like a memcpy. The raw data
     * stays the same in a format change, a 2x2x1 texture is copied into the first row of a 4x4x1 texture,
     * etc. I could not get it to segfault, but the nonexistent 5th pixel of a 2x2x1 texture is copied into
     * pixel 1x2x1 of a 4x4x1 texture, demonstrating a read beyond the texture's end. I suspect any bad
     * memory access is silently ignored by the runtime, in the kernel or on the GPU.
     *
     * I'm not adding tests for this behavior until an application needs it. */

out:
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_create_rt_ds_fail(void)
{
    IDirect3DDevice8 *device;
    HWND window;
    HRESULT hr;
    ULONG refcount;
    IDirect3D8 *d3d8;
    IDirect3DSurface8 *surface;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }

    /* Output pointer == NULL segfaults on Windows. */

    surface = (IDirect3DSurface8 *)0xdeadbeef;
    hr = IDirect3DDevice8_CreateRenderTarget(device, 4, 4, D3DFMT_D16,
            D3DMULTISAMPLE_NONE, FALSE, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Creating a D16 render target returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface8_Release(surface);

    surface = (IDirect3DSurface8 *)0xdeadbeef;
    hr = IDirect3DDevice8_CreateDepthStencilSurface(device, 4, 4, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Creating a A8R8G8B8 depth stencil returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface8_Release(surface);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_volume_blocks(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d8;
    UINT refcount;
    HWND window;
    HRESULT hr;
    D3DCAPS8 caps;
    IDirect3DVolumeTexture8 *texture;
    unsigned int w, h, d, i, j;
    static const struct
    {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_depth;
        unsigned int block_size;
        BOOL broken;
        BOOL create_size_checked, core_fmt;
    }
    formats[] =
    {
        /* Scratch volumes enforce DXTn block locks, unlike their surface counterparts.
         * ATI2N and YUV blocks are not enforced on any tested card (r200, gtx 460). */
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, 1, 8,  FALSE, TRUE,  TRUE },
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, 1, 16, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, 1, 16, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, 1, 16, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 1, 16, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 1, 16, FALSE, TRUE,  TRUE },
        /* ATI2N has 2x2 blocks on all AMD cards and Geforce 7 cards,
         * which doesn't match the format spec. On newer Nvidia cards
         * it has the correct 4x4 block size */
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       4, 4, 1, 16, TRUE,  FALSE, FALSE},
        {D3DFMT_YUY2,                 "D3DFMT_YUY2", 2, 1, 1, 4,  TRUE,  FALSE, TRUE },
        {D3DFMT_UYVY,                 "D3DFMT_UYVY", 2, 1, 1, 4,  TRUE,  FALSE, TRUE },
    };
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        BOOL need_driver_support, need_runtime_support;
    }
    create_tests[] =
    {
        {D3DPOOL_DEFAULT,       "D3DPOOL_DEFAULT",  TRUE,  FALSE},
        {D3DPOOL_SCRATCH,       "D3DPOOL_SCRATCH",  FALSE, TRUE },
        {D3DPOOL_SYSTEMMEM,     "D3DPOOL_SYSTEMMEM",TRUE,  FALSE},
        {D3DPOOL_MANAGED,       "D3DPOOL_MANAGED",  TRUE,  FALSE},
    };
    static const struct
    {
        unsigned int x, y, z, x2, y2, z2;
    }
    offset_tests[] =
    {
        {0, 0, 0, 8, 8, 8},
        {0, 0, 3, 8, 8, 8},
        {0, 4, 0, 8, 8, 8},
        {0, 4, 3, 8, 8, 8},
        {4, 0, 0, 8, 8, 8},
        {4, 0, 3, 8, 8, 8},
        {4, 4, 0, 8, 8, 8},
        {4, 4, 3, 8, 8, 8},
    };
    D3DBOX box;
    D3DLOCKED_BOX locked_box;
    BYTE *base;
    INT expected_row_pitch, expected_slice_pitch;
    BOOL support, support_2d;
    BOOL pow2;
    unsigned int offset, expected_offset;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d8, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d8);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice8_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); i++)
    {
        hr = IDirect3D8_CheckDeviceFormat(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_VOLUMETEXTURE, formats[i].fmt);
        support = SUCCEEDED(hr);
        hr = IDirect3D8_CheckDeviceFormat(d3d8, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, formats[i].fmt);
        support_2d = SUCCEEDED(hr);

        /* Test creation restrictions */
        for (w = 1; w <= 8; w++)
        {
            for (h = 1; h <= 8; h++)
            {
                for (d = 1; d <= 8; d++)
                {
                    HRESULT expect_hr;
                    BOOL size_is_pow2;
                    BOOL block_aligned = TRUE;

                    if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                        block_aligned = FALSE;

                    size_is_pow2 = !((w & (w - 1)) || (h & (h - 1)) || (d & (d - 1)));

                    for (j = 0; j < sizeof(create_tests) / sizeof(*create_tests); j++)
                    {
                        BOOL may_succeed = FALSE;
                        BOOL todo = FALSE;

                        if (create_tests[j].need_runtime_support && !formats[i].core_fmt && !support)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (formats[i].create_size_checked && !block_aligned)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (pow2 && !size_is_pow2 && create_tests[j].need_driver_support)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (create_tests[j].need_driver_support && !support)
                        {
                            todo = support_2d;
                            expect_hr = D3DERR_INVALIDCALL;
                        }
                        else
                            expect_hr = D3D_OK;

                        texture = (IDirect3DVolumeTexture8 *)0xdeadbeef;
                        hr = IDirect3DDevice8_CreateVolumeTexture(device, w, h, d, 1, 0,
                                formats[i].fmt, create_tests[j].pool, &texture);

                        /* Wine knows about ATI2N and happily creates a scratch resource even if GL
                         * does not support it. Accept scratch creation of extension formats on
                         * Windows as well if it occurs. We don't really care if e.g. a Windows 7
                         * on an r200 GPU creates scratch ATI2N texture even though the card doesn't
                         * support it. */
                        if (!formats[i].core_fmt && !support && FAILED(expect_hr))
                            may_succeed = TRUE;

                        if (todo)
                        {
                            todo_wine ok(hr == expect_hr || ((SUCCEEDED(hr) && may_succeed)),
                                    "Got unexpected hr %#x for format %s, pool %s, size %ux%ux%u.\n",
                                    hr, formats[i].name, create_tests[j].name, w, h, d);
                        }
                        else
                        {
                            ok(hr == expect_hr || ((SUCCEEDED(hr) && may_succeed)),
                                    "Got unexpected hr %#x for format %s, pool %s, size %ux%ux%u.\n",
                                    hr, formats[i].name, create_tests[j].name, w, h, d);
                        }

                        if (FAILED(hr))
                            ok(texture == NULL, "Got texture ptr %p, expected NULL.\n", texture);
                        else
                            IDirect3DVolumeTexture8_Release(texture);
                    }
                }
            }
        }

        if (!support && !formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice8_CreateVolumeTexture(device, 24, 8, 8, 1, 0,
                formats[i].fmt, D3DPOOL_SCRATCH, &texture);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        /* Test lockrect offset */
        for (j = 0; j < sizeof(offset_tests) / sizeof(*offset_tests); j++)
        {
            unsigned int bytes_per_pixel;
            bytes_per_pixel = formats[i].block_size / (formats[i].block_width * formats[i].block_height);

            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            base = locked_box.pBits;
            if (formats[i].broken)
            {
                expected_row_pitch = bytes_per_pixel * 24;
            }
            else
            {
                expected_row_pitch = (24 /* tex width */ + formats[i].block_height - 1) / formats[i].block_width
                        * formats[i].block_size;
            }
            ok(locked_box.RowPitch == expected_row_pitch, "Got unexpected row pitch %d for format %s, expected %d.\n",
                    locked_box.RowPitch, formats[i].name, expected_row_pitch);

            if (formats[i].broken)
            {
                expected_slice_pitch = expected_row_pitch * 8;
            }
            else
            {
                expected_slice_pitch = (8 /* tex height */ + formats[i].block_depth - 1) / formats[i].block_height
                        * expected_row_pitch;
            }
            ok(locked_box.SlicePitch == expected_slice_pitch,
                    "Got unexpected slice pitch %d for format %s, expected %d.\n",
                    locked_box.SlicePitch, formats[i].name, expected_slice_pitch);

            hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x, j %u.\n", hr, j);

            box.Left = offset_tests[j].x;
            box.Top = offset_tests[j].y;
            box.Front = offset_tests[j].z;
            box.Right = offset_tests[j].x2;
            box.Bottom = offset_tests[j].y2;
            box.Back = offset_tests[j].z2;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x, j %u.\n", hr, j);

            offset = (BYTE *)locked_box.pBits - base;
            if (formats[i].broken)
            {
                expected_offset = box.Front * expected_slice_pitch
                        + box.Top * expected_row_pitch
                        + box.Left * bytes_per_pixel;
            }
            else
            {
                expected_offset = (box.Front / formats[i].block_depth) * expected_slice_pitch
                        + (box.Top / formats[i].block_height) * expected_row_pitch
                        + (box.Left / formats[i].block_width) * formats[i].block_size;
            }
            ok(offset == expected_offset, "Got unexpected offset %u for format %s, expected %u, box start %ux%ux%u.\n",
                    offset, formats[i].name, expected_offset, box.Left, box.Top, box.Front);

            hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }

        /* Test partial block locks */
        box.Front = 0;
        box.Back = 1;
        if (formats[i].block_width > 1)
        {
            box.Left = formats[i].block_width >> 1;
            box.Top = 0;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        if (formats[i].block_height > 1)
        {
            box.Left = 0;
            box.Top = formats[i].block_height >> 1;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height >> 1;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        /* Test full block lock */
        box.Left = 0;
        box.Top = 0;
        box.Right = formats[i].block_width;
        box.Bottom = formats[i].block_height;
        hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &box, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        IDirect3DVolumeTexture8_Release(texture);

        /* Test mipmap locks. Don't do this with ATI2N, AMD warns that the runtime
         * does not allocate surfaces smaller than the blocksize properly. */
        if ((formats[i].block_width > 1 || formats[i].block_height > 1) && formats[i].core_fmt)
        {
            hr = IDirect3DDevice8_CreateVolumeTexture(device, formats[i].block_width, formats[i].block_height,
                    2, 2, 0, formats[i].fmt, D3DPOOL_SCRATCH, &texture);

            hr = IDirect3DVolumeTexture8_LockBox(texture, 1, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture8_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Left = box.Top = box.Front = 0;
            box.Right = formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1;
            box.Back = 1;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 1, &locked_box, &box, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture8_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture8_LockBox(texture, 1, &locked_box, &box, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
            if (SUCCEEDED(hr))
                IDirect3DVolumeTexture8_UnlockBox(texture, 1);

            IDirect3DVolumeTexture8_Release(texture);
        }
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d8);
    DestroyWindow(window);
}

static void test_lockbox_invalid(void)
{
    static const struct
    {
        D3DBOX box;
        HRESULT result;
    }
    test_data[] =
    {
        {{0, 0, 2, 2, 0, 1},    D3D_OK},                /* Valid */
        {{0, 0, 4, 4, 0, 1},    D3D_OK},                /* Valid */
        {{0, 0, 0, 4, 0, 1},    D3DERR_INVALIDCALL},    /* 0 height */
        {{0, 0, 4, 0, 0, 1},    D3DERR_INVALIDCALL},    /* 0 width */
        {{0, 0, 4, 4, 1, 1},    D3DERR_INVALIDCALL},    /* 0 depth */
        {{4, 0, 0, 4, 0, 1},    D3DERR_INVALIDCALL},    /* left > right */
        {{0, 4, 4, 0, 0, 1},    D3DERR_INVALIDCALL},    /* top > bottom */
        {{0, 0, 4, 4, 1, 0},    D3DERR_INVALIDCALL},    /* back > front */
        {{0, 0, 8, 4, 0, 1},    D3DERR_INVALIDCALL},    /* right > surface */
        {{0, 0, 4, 8, 0, 1},    D3DERR_INVALIDCALL},    /* bottom > surface */
        {{0, 0, 4, 4, 0, 3},    D3DERR_INVALIDCALL},    /* back > surface */
        {{8, 0, 16, 4, 0, 1},   D3DERR_INVALIDCALL},    /* left > surface */
        {{0, 8, 4, 16, 0, 1},   D3DERR_INVALIDCALL},    /* top > surface */
        {{0, 0, 4, 4, 2, 4},    D3DERR_INVALIDCALL},    /* top > surface */
    };
    static const D3DBOX test_boxt_2 = {2, 2, 4, 4, 0, 1};
    IDirect3DVolumeTexture8 *texture = NULL;
    D3DLOCKED_BOX locked_box;
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    window = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D8_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice8_CreateVolumeTexture(device, 4, 4, 2, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &texture);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    base = locked_box.pBits;
    hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); ++i)
    {
        unsigned int offset, expected_offset;
        const D3DBOX *box = &test_data[i].box;

        locked_box.pBits = (BYTE *)0xdeadbeef;
        locked_box.RowPitch = 0xdeadbeef;
        locked_box.SlicePitch = 0xdeadbeef;

        hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, box, 0);
        /* Unlike surfaces, volumes properly check the box even in Windows XP */
        ok(hr == test_data[i].result,
                "Got unexpected hr %#x with box [%u, %u, %u]->[%u, %u, %u], expected %#x.\n",
                hr, box->Left, box->Top, box->Front, box->Right, box->Bottom, box->Back,
                test_data[i].result);
        if (FAILED(hr))
            continue;

        offset = (BYTE *)locked_box.pBits - base;
        expected_offset = box->Front * locked_box.SlicePitch + box->Top * locked_box.RowPitch + box->Left * 4;
        ok(offset == expected_offset,
                "Got unexpected offset %u (expected %u) for rect [%u, %u, %u]->[%u, %u, %u].\n",
                offset, expected_offset, box->Left, box->Top, box->Front, box->Right, box->Bottom, box->Back);

        hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    }

    /* locked_box = NULL throws an exception on Windows */
    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture8_LockBox(texture, 0, &locked_box, &test_boxt_2, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_boxt_2.Left, test_boxt_2.Top, test_boxt_2.Front,
            test_boxt_2.Right, test_boxt_2.Bottom, test_boxt_2.Back);
    hr = IDirect3DVolumeTexture8_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    IDirect3DVolumeTexture8_Release(texture);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_pixel_format(void)
{
    HWND hwnd, hwnd2 = NULL;
    HDC hdc, hdc2 = NULL;
    HMODULE gl = NULL;
    int format, test_format;
    PIXELFORMATDESCRIPTOR pfd;
    IDirect3D8 *d3d8 = NULL;
    IDirect3DDevice8 *device = NULL;
    HRESULT hr;
    static const float point[3] = {0.0, 0.0, 0.0};

    hwnd = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    if (!hwnd)
    {
        skip("Failed to create window\n");
        return;
    }

    hwnd2 = CreateWindowA("d3d8_test_wc", "d3d8_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hdc = GetDC(hwnd);
    if (!hdc)
    {
        skip("Failed to get DC\n");
        goto cleanup;
    }

    if (hwnd2)
        hdc2 = GetDC(hwnd2);

    gl = LoadLibraryA("opengl32.dll");
    ok(!!gl, "failed to load opengl32.dll; SetPixelFormat()/GetPixelFormat() may not work right\n");

    format = GetPixelFormat(hdc);
    ok(format == 0, "new window has pixel format %d\n", format);

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    format = ChoosePixelFormat(hdc, &pfd);
    if (format <= 0)
    {
        skip("no pixel format available\n");
        goto cleanup;
    }

    if (!SetPixelFormat(hdc, format, &pfd) || GetPixelFormat(hdc) != format)
    {
        skip("failed to set pixel format\n");
        goto cleanup;
    }

    if (!hdc2 || !SetPixelFormat(hdc2, format, &pfd) || GetPixelFormat(hdc2) != format)
    {
        skip("failed to set pixel format on second window\n");
        if (hdc2)
        {
            ReleaseDC(hwnd2, hdc2);
            hdc2 = NULL;
        }
    }

    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d8, "Failed to create a D3D object.\n");

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (!(device = create_device(d3d8, hwnd, hwnd, TRUE)))
    {
        skip("Failed to create device\n");
        goto cleanup;
    }

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice8_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, point, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        hr = IDirect3DDevice8_Present(device, NULL, NULL, hwnd2, NULL);
        ok(SUCCEEDED(hr), "Present failed %#x\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

cleanup:
    if (device)
    {
        UINT refcount = IDirect3DDevice8_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (d3d8) IDirect3D8_Release(d3d8);
    if (gl) FreeLibrary(gl);
    if (hdc) ReleaseDC(hwnd, hdc);
    if (hdc2) ReleaseDC(hwnd2, hdc2);
    if (hwnd) DestroyWindow(hwnd);
    if (hwnd2) DestroyWindow(hwnd2);
}

static void test_begin_end_state_block(void)
{
    IDirect3DDevice8 *device;
    DWORD stateblock;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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

    /* Should succeed. */
    hr = IDirect3DDevice8_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    /* Calling BeginStateBlock() while recording should return
     * D3DERR_INVALIDCALL. */
    hr = IDirect3DDevice8_BeginStateBlock(device);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Should succeed. */
    stateblock = 0xdeadbeef;
    hr = IDirect3DDevice8_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);
    ok(!!stateblock && stateblock != 0xdeadbeef, "Got unexpected stateblock %#x.\n", stateblock);
    IDirect3DDevice8_DeleteStateBlock(device, stateblock);

    /* Calling EndStateBlock() while not recording should return
     * D3DERR_INVALIDCALL. stateblock should not be touched. */
    stateblock = 0xdeadbeef;
    hr = IDirect3DDevice8_EndStateBlock(device, &stateblock);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(stateblock == 0xdeadbeef, "Got unexpected stateblock %#x.\n", stateblock);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_shader_constant_apply(void)
{
    static const float vs_const[] = {1.0f, 2.0f, 3.0f, 4.0f};
    static const float ps_const[] = {5.0f, 6.0f, 7.0f, 8.0f};
    static const float initial[] = {0.0f, 0.0f, 0.0f, 0.0f};
    DWORD vs_version, ps_version;
    IDirect3DDevice8 *device;
    DWORD stateblock;
    IDirect3D8 *d3d;
    ULONG refcount;
    D3DCAPS8 caps;
    float ret[4];
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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
    vs_version = caps.VertexShaderVersion & 0xffff;
    ps_version = caps.PixelShaderVersion & 0xffff;

    if (vs_version)
    {
        hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetVertexShaderConstant(device, 1, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);

        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);

        hr = IDirect3DDevice8_SetVertexShaderConstant(device, 0, vs_const, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice8_SetPixelShaderConstant(device, 0, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
        hr = IDirect3DDevice8_SetPixelShaderConstant(device, 1, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);

        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);

        hr = IDirect3DDevice8_SetPixelShaderConstant(device, 0, ps_const, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
    }

    hr = IDirect3DDevice8_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice8_SetVertexShaderConstant(device, 1, vs_const, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice8_SetPixelShaderConstant(device, 1, ps_const, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
    }

    hr = IDirect3DDevice8_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
    }

    /* Apply doesn't overwrite constants that aren't explicitly set on the
     * source stateblock. */
    hr = IDirect3DDevice8_ApplyStateBlock(device, stateblock);
    ok(SUCCEEDED(hr), "Failed to apply stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
        hr = IDirect3DDevice8_GetVertexShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
        hr = IDirect3DDevice8_GetPixelShaderConstant(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
    }

    IDirect3DDevice8_DeleteStateBlock(device, stateblock);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_resource_type(void)
{
    IDirect3DDevice8 *device;
    IDirect3DSurface8 *surface;
    IDirect3DTexture8 *texture;
    IDirect3DCubeTexture8 *cube_texture;
    IDirect3DVolume8 *volume;
    IDirect3DVolumeTexture8 *volume_texture;
    D3DSURFACE_DESC surface_desc;
    D3DVOLUME_DESC volume_desc;
    D3DRESOURCETYPE type;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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

    hr = IDirect3DDevice8_CreateImageSurface(device, 4, 4, D3DFMT_X8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    IDirect3DSurface8_Release(surface);

    hr = IDirect3DDevice8_CreateTexture(device, 2, 8, 4, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    type = IDirect3DTexture8_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "Expected type D3DRTYPE_TEXTURE, got %u.\n", type);

    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 2, "Expected width 2, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 8, "Expected height 8, got %u.\n", surface_desc.Height);
    hr = IDirect3DTexture8_GetLevelDesc(texture, 0, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 2, "Expected width 2, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 8, "Expected height 8, got %u.\n", surface_desc.Height);
    IDirect3DSurface8_Release(surface);

    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 2, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 1, "Expected width 1, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 2, "Expected height 2, got %u.\n", surface_desc.Height);
    hr = IDirect3DTexture8_GetLevelDesc(texture, 2, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 1, "Expected width 1, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 2, "Expected height 2, got %u.\n", surface_desc.Height);
    IDirect3DSurface8_Release(surface);
    IDirect3DTexture8_Release(texture);

    hr = IDirect3DDevice8_CreateCubeTexture(device, 1, 1, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &cube_texture);
    ok(SUCCEEDED(hr), "Failed to create cube texture, hr %#x.\n", hr);
    type = IDirect3DCubeTexture8_GetType(cube_texture);
    ok(type == D3DRTYPE_CUBETEXTURE, "Expected type D3DRTYPE_CUBETEXTURE, got %u.\n", type);

    hr = IDirect3DCubeTexture8_GetCubeMapSurface(cube_texture,
            D3DCUBEMAP_FACE_NEGATIVE_X, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get cube map surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    hr = IDirect3DCubeTexture8_GetLevelDesc(cube_texture, 0, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    IDirect3DSurface8_Release(surface);
    IDirect3DCubeTexture8_Release(cube_texture);

    hr = IDirect3DDevice8_CreateVolumeTexture(device, 2, 4, 8, 4, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &volume_texture);
    type = IDirect3DVolumeTexture8_GetType(volume_texture);
    ok(type == D3DRTYPE_VOLUMETEXTURE, "Expected type D3DRTYPE_VOLUMETEXTURE, got %u.\n", type);

    hr = IDirect3DVolumeTexture8_GetVolumeLevel(volume_texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    /* IDirect3DVolume8 is not an IDirect3DResource8 and has no GetType method. */
    hr = IDirect3DVolume8_GetDesc(volume, &volume_desc);
    ok(SUCCEEDED(hr), "Failed to get volume description, hr %#x.\n", hr);
    ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
            volume_desc.Type);
    ok(volume_desc.Width == 2, "Expected width 2, got %u.\n", volume_desc.Width);
    ok(volume_desc.Height == 4, "Expected height 4, got %u.\n", volume_desc.Height);
    ok(volume_desc.Depth == 8, "Expected depth 8, got %u.\n", volume_desc.Depth);
    hr = IDirect3DVolumeTexture8_GetLevelDesc(volume_texture, 0, &volume_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
            volume_desc.Type);
    ok(volume_desc.Width == 2, "Expected width 2, got %u.\n", volume_desc.Width);
    ok(volume_desc.Height == 4, "Expected height 4, got %u.\n", volume_desc.Height);
    ok(volume_desc.Depth == 8, "Expected depth 8, got %u.\n", volume_desc.Depth);
    IDirect3DVolume8_Release(volume);

    hr = IDirect3DVolumeTexture8_GetVolumeLevel(volume_texture, 2, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    hr = IDirect3DVolume8_GetDesc(volume, &volume_desc);
    ok(SUCCEEDED(hr), "Failed to get volume description, hr %#x.\n", hr);
    ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
            volume_desc.Type);
    ok(volume_desc.Width == 1, "Expected width 1, got %u.\n", volume_desc.Width);
    ok(volume_desc.Height == 1, "Expected height 1, got %u.\n", volume_desc.Height);
    ok(volume_desc.Depth == 2, "Expected depth 2, got %u.\n", volume_desc.Depth);
    hr = IDirect3DVolumeTexture8_GetLevelDesc(volume_texture, 2, &volume_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
            volume_desc.Type);
    ok(volume_desc.Width == 1, "Expected width 1, got %u.\n", volume_desc.Width);
    ok(volume_desc.Height == 1, "Expected height 1, got %u.\n", volume_desc.Height);
    ok(volume_desc.Depth == 2, "Expected depth 2, got %u.\n", volume_desc.Depth);
    IDirect3DVolume8_Release(volume);
    IDirect3DVolumeTexture8_Release(volume_texture);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_mipmap_lock(void)
{
    IDirect3DDevice8 *device;
    IDirect3DSurface8 *surface, *surface2, *surface_dst, *surface_dst2;
    IDirect3DTexture8 *texture, *texture_dst;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DLOCKED_RECT locked_rect;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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

    hr = IDirect3DDevice8_CreateTexture(device, 4, 4, 2, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &texture_dst);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture_dst, 0, &surface_dst);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture_dst, 1, &surface_dst2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CreateTexture(device, 4, 4, 2, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture8_GetSurfaceLevel(texture, 1, &surface2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_LockRect(surface2, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirect3DDevice8_CopyRects(device, surface, NULL, 0, surface_dst, NULL);
    ok(SUCCEEDED(hr), "Failed to update surface, hr %#x.\n", hr);
    hr = IDirect3DDevice8_CopyRects(device, surface2, NULL, 0, surface_dst2, NULL);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Apparently there's no validation on the container. */
    hr = IDirect3DDevice8_UpdateTexture(device, (IDirect3DBaseTexture8 *)texture,
            (IDirect3DBaseTexture8 *)texture_dst);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    hr = IDirect3DSurface8_UnlockRect(surface2);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface8_Release(surface_dst2);
    IDirect3DSurface8_Release(surface_dst);
    IDirect3DSurface8_Release(surface2);
    IDirect3DSurface8_Release(surface);
    IDirect3DTexture8_Release(texture_dst);
    IDirect3DTexture8_Release(texture);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_writeonly_resource(void)
{
    IDirect3D8 *d3d;
    IDirect3DDevice8 *device;
    IDirect3DVertexBuffer8 *buffer;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BYTE *ptr;
    static const struct
    {
        struct vec3 pos;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}}
    };

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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

    hr = IDirect3DDevice8_CreateVertexBuffer(device, sizeof(quad),
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &buffer);
    ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer8_Lock(buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(ptr, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer8_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetStreamSource(device, 0, buffer, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to set stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice8_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene %#x\n", hr);
    hr = IDirect3DDevice8_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice8_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene %#x\n", hr);

    hr = IDirect3DVertexBuffer8_Lock(buffer, 0, 0, &ptr, 0);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok (!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer8_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer8_Lock(buffer, 0, 0, &ptr, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok (!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer8_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    refcount = IDirect3DVertexBuffer8_Release(buffer);
    ok(!refcount, "Vertex buffer has %u references left.\n", refcount);
    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirect3DDevice8 *device;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate8(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, window, FALSE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    if (hr == D3DERR_DEVICELOST)
    {
        win_skip("Broken TestCooperativeLevel(), skipping test.\n");
        IDirect3DDevice8_Release(device);
        goto done;
    }
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

    ret = ShowWindow(window, SW_RESTORE);
    ok(ret, "Failed to restore window.\n");
    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3DERR_DEVICENOTRESET, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, TRUE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = ShowWindow(window, SW_RESTORE);
    ok(ret, "Failed to restore window.\n");
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, window, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_TestCooperativeLevel(device);
    todo_wine ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    todo_wine ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

static void test_resource_priority(void)
{
    IDirect3DDevice8 *device;
    IDirect3DTexture8 *texture;
    IDirect3DVertexBuffer8 *buffer;
    IDirect3D8 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        BOOL can_set_priority;
    }
    test_data[] =
    {
        {D3DPOOL_DEFAULT, "D3DPOOL_DEFAULT", FALSE},
        {D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", FALSE},
        {D3DPOOL_MANAGED, "D3DPOOL_MANAGED", TRUE},
        {D3DPOOL_SCRATCH, "D3DPOOL_SCRATCH", FALSE}
    };
    unsigned int i;
    DWORD priority;

    window = CreateWindowA("static", "d3d8_test", WS_OVERLAPPEDWINDOW,
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

    for (i = 0; i < sizeof(test_data) / sizeof(*test_data); i++)
    {
        hr = IDirect3DDevice8_CreateTexture(device, 16, 16, 0, 0, D3DFMT_X8R8G8B8,
                test_data[i].pool, &texture);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, pool %s.\n", hr, test_data[i].name);

        priority = IDirect3DTexture8_GetPriority(texture);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DTexture8_SetPriority(texture, 1);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DTexture8_GetPriority(texture);
        if (test_data[i].can_set_priority)
        {
            ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DTexture8_SetPriority(texture, 0);
            ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        }
        else
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

        IDirect3DTexture8_Release(texture);

        if (test_data[i].pool != D3DPOOL_SCRATCH)
        {
            hr = IDirect3DDevice8_CreateVertexBuffer(device, 256, 0, 0,
                    test_data[i].pool, &buffer);
            ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x, pool %s.\n", hr, test_data[i].name);

            priority = IDirect3DVertexBuffer8_GetPriority(buffer);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DVertexBuffer8_SetPriority(buffer, 1);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DVertexBuffer8_GetPriority(buffer);
            if (test_data[i].can_set_priority)
            {
                ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
                priority = IDirect3DVertexBuffer8_SetPriority(buffer, 0);
                ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            }
            else
                ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

            IDirect3DVertexBuffer8_Release(buffer);
        }
    }

    refcount = IDirect3DDevice8_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D8_Release(d3d);
    DestroyWindow(window);
}

START_TEST(device)
{
    HMODULE d3d8_handle = LoadLibraryA( "d3d8.dll" );
    WNDCLASSA wc = {0};
    IDirect3D8 *d3d8;

    if (!d3d8_handle)
    {
        skip("Could not load d3d8.dll\n");
        return;
    }

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d8_test_wc";
    RegisterClassA(&wc);

    ValidateVertexShader = (void *)GetProcAddress(d3d8_handle, "ValidateVertexShader");
    ValidatePixelShader = (void *)GetProcAddress(d3d8_handle, "ValidatePixelShader");

    if (!(d3d8 = Direct3DCreate8(D3D_SDK_VERSION)))
    {
        skip("could not create D3D8\n");
        return;
    }
    IDirect3D8_Release(d3d8);

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);

    test_fpu_setup();
    test_display_formats();
    test_display_modes();
    test_shader_versions();
    test_swapchain();
    test_refcount();
    test_mipmap_levels();
    test_cursor();
    test_cursor_pos();
    test_states();
    test_reset();
    test_scene();
    test_shader();
    test_limits();
    test_lights();
    test_ApplyStateBlock();
    test_render_zero_triangles();
    test_depth_stencil_reset();
    test_wndproc();
    test_wndproc_windowed();
    test_depth_stencil_size();
    test_window_style();
    test_wrong_shader();
    test_mode_change();
    test_device_window_reset();
    test_reset_resources();
    depth_blit_test();
    test_set_rt_vp_scissor();
    test_validate_vs();
    test_validate_ps();
    test_volume_get_container();
    test_vb_lock_flags();
    test_texture_stage_states();
    test_cube_textures();
    test_image_surface_pool();
    test_surface_get_container();
    test_lockrect_invalid();
    test_private_data();
    test_surface_dimensions();
    test_surface_format_null();
    test_surface_double_unlock();
    test_surface_blocks();
    test_set_palette();
    test_swvp_buffer();
    test_npot_textures();
    test_volume_locking();
    test_update_volumetexture();
    test_create_rt_ds_fail();
    test_volume_blocks();
    test_lockbox_invalid();
    test_pixel_format();
    test_begin_end_state_block();
    test_shader_constant_apply();
    test_resource_type();
    test_mipmap_lock();
    test_writeonly_resource();
    test_lost_device();
    test_resource_priority();

    UnregisterClassA("d3d8_test_wc", GetModuleHandleA(NULL));
}
