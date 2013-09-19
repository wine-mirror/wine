/*
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2006 Chris Robinson
 * Copyright 2006-2008, 2010-2011, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2005, 2006, 2007 Henri Verbeet
 * Copyright 2013 Henri Verbeet for CodeWeavers
 * Copyright (C) 2008 Rico Schüller
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
#include <windowsx.h>
#include <d3d9.h>
#include "wine/test.h"

static INT screen_width;
static INT screen_height;

static IDirect3D9 *(WINAPI *pDirect3DCreate9)(UINT);

static const DWORD simple_vs[] =
{
    0xfffe0101,                                                             /* vs_1_1                       */
    0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position0 v0             */
    0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000,                         /* dp4 oPos.x, v0, c0           */
    0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001,                         /* dp4 oPos.y, v0, c1           */
    0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002,                         /* dp4 oPos.z, v0, c2           */
    0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003,                         /* dp4 oPos.w, v0, c3           */
    0x0000ffff,                                                             /* end                          */
};

static const DWORD simple_ps[] =
{
    0xffff0101,                                                             /* ps_1_1                       */
    0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
    0x00000042, 0xb00f0000,                                                 /* tex t0                       */
    0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
    0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
    0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
    0x0000ffff,                                                             /* end                          */
};

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
        while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessage( &msg );
        diff = time - GetTickCount();
    }
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND device_window, HWND focus_window, BOOL windowed)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device;

    present_parameters.Windowed = windowed;
    present_parameters.hDeviceWindow = device_window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = screen_width;
    present_parameters.BackBufferHeight = screen_height;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device))) return device;

    return NULL;
}

static HRESULT reset_device(IDirect3DDevice9 *device, HWND device_window, BOOL windowed)
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

    return IDirect3DDevice9_Reset(device, &present_parameters);
}

#define CHECK_CALL(r,c,d,rc) \
    if (SUCCEEDED(r)) {\
        int tmp1 = get_refcount( (IUnknown *)d ); \
        int rc_new = rc; \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    } else {\
        trace("%s failed: %08x\n", c, r); \
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
        hr = IDirect3DSurface9_GetContainer(obj, &iid, &container_ptr); \
        ok(SUCCEEDED(hr) && container_ptr == expected, "GetContainer returned: hr %#x, container_ptr %p. " \
            "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, expected); \
        if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr); \
    }

static void check_mipmap_levels(IDirect3DDevice9 *device, UINT width, UINT height, UINT count)
{
    IDirect3DBaseTexture9* texture = NULL;
    HRESULT hr = IDirect3DDevice9_CreateTexture( device, width, height, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DTexture9**) &texture, NULL );

    if (SUCCEEDED(hr)) {
        DWORD levels = IDirect3DBaseTexture9_GetLevelCount(texture);
        ok(levels == count, "Invalid level count. Expected %d got %u\n", count, levels);
    } else
        trace("CreateTexture failed: %08x\n", hr);

    if (texture) IDirect3DBaseTexture9_Release( texture );
}

static void test_mipmap_levels(void)
{

    HRESULT               hr;
    HWND                  hwnd = NULL;

    IDirect3D9            *pD3d = NULL;
    IDirect3DDevice9      *pDevice = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DDISPLAYMODE        d3ddm;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr) || hr == D3DERR_NOTAVAILABLE, "Failed to create IDirect3D9Device (%08x)\n", hr);
    if (FAILED(hr)) {
        skip("failed to create a d3d device\n");
        goto cleanup;
    }

    check_mipmap_levels(pDevice, 32, 32, 6);
    check_mipmap_levels(pDevice, 256, 1, 9);
    check_mipmap_levels(pDevice, 1, 256, 9);
    check_mipmap_levels(pDevice, 1, 1, 1);

cleanup:
    if (pDevice)
    {
        UINT refcount = IDirect3DDevice9_Release( pDevice );
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release( pD3d );
    DestroyWindow( hwnd );
}

static void test_checkdevicemultisampletype(void)
{

    HRESULT               hr;
    HWND                  hwnd = NULL;

    IDirect3D9            *pD3d = NULL;
    IDirect3DDevice9      *pDevice = NULL;
    D3DPRESENT_PARAMETERS d3dpp;
    D3DDISPLAYMODE        d3ddm;
    DWORD                 qualityLevels;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr) || hr == D3DERR_NOTAVAILABLE, "Failed to create IDirect3D9Device (%08x)\n", hr);
    if (FAILED(hr)) {
        skip("failed to create a d3d device\n");
        goto cleanup;
    }

    qualityLevels = 0;

    hr = IDirect3D9_CheckDeviceMultiSampleType(pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, TRUE,
    D3DMULTISAMPLE_NONE, &qualityLevels);
    ok(SUCCEEDED(hr) || hr == D3DERR_NOTAVAILABLE, "CheckDeviceMultiSampleType failed with (%08x)\n", hr);
    if(hr == D3DERR_NOTAVAILABLE)
    {
        skip("IDirect3D9_CheckDeviceMultiSampleType not available\n");
        goto cleanup;
    }
    ok(qualityLevels == 1,"qualitylevel is not 1 but %d\n",qualityLevels);

    hr = IDirect3D9_CheckDeviceMultiSampleType(pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, FALSE,
    D3DMULTISAMPLE_NONE, &qualityLevels);
    ok(SUCCEEDED(hr), "CheckDeviceMultiSampleType failed with (%08x)\n", hr);
    ok(qualityLevels == 1,"qualitylevel is not 1 but %d\n",qualityLevels);

cleanup:
    if (pDevice)
    {
        UINT refcount = IDirect3DDevice9_Release( pDevice );
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release( pD3d );
    DestroyWindow( hwnd );
}

static void test_swapchain(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    IDirect3DSwapChain9         *swapchain0         = NULL;
    IDirect3DSwapChain9         *swapchain1         = NULL;
    IDirect3DSwapChain9         *swapchain2         = NULL;
    IDirect3DSwapChain9         *swapchain3         = NULL;
    IDirect3DSwapChain9         *swapchainX         = NULL;
    IDirect3DSurface9           *backbuffer         = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.BackBufferCount  = 0;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == S_OK || hr == D3DERR_NOTAVAILABLE,
       "Failed to create IDirect3D9Device (%08x)\n", hr);
    if (FAILED(hr)) goto cleanup;

    /* Check if the back buffer count was modified */
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 0, &swapchain0);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x)\n", hr);
    if(swapchain0) IDirect3DSwapChain9_Release(swapchain0);

    /* Check if there is a back buffer */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    /* Try to get a nonexistent swapchain */
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "GetSwapChain on an nonexistent swapchain returned (%08x)\n", hr);
    ok(swapchainX == NULL, "Swapchain 1 is %p\n", swapchainX);
    if(swapchainX) IDirect3DSwapChain9_Release(swapchainX);

    /* Create a bunch of swapchains */
    d3dpp.BackBufferCount = 0;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain1);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    d3dpp.BackBufferCount  = 1;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain2);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);

    d3dpp.BackBufferCount  = 2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain3);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        /* Swapchain 3, created with backbuffercount 2 */
        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 0, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 1st back buffer (%08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 1, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 2nd back buffer (%08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 2, 0, &backbuffer);
        ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %08x\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 3, 0, &backbuffer);
        ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);
    }

    /* Check the back buffers of the swapchains */
    /* Swapchain 1, created with backbuffercount 0 */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL (%08x)\n", hr);
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 1, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Swapchain 2 - created with backbuffercount 1 */
    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 0, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 1, 0, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %08x\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 2, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Try getSwapChain on a manually created swapchain
     * it should fail, apparently GetSwapChain only returns implicit swapchains
     */
    swapchainX = (void *) 0xdeadbeef;
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "Failed to get the second swapchain (%08x)\n", hr);
    ok(swapchainX == NULL, "The swapchain pointer is %p\n", swapchainX);
    if(swapchainX && swapchainX != (void *) 0xdeadbeef ) IDirect3DSwapChain9_Release(swapchainX);

cleanup:
    if(swapchain1) IDirect3DSwapChain9_Release(swapchain1);
    if(swapchain2) IDirect3DSwapChain9_Release(swapchain2);
    if(swapchain3) IDirect3DSwapChain9_Release(swapchain3);
    if (pDevice)
    {
        UINT refcount = IDirect3DDevice9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    DestroyWindow( hwnd );
}

static void test_refcount(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3D9                  *pD3d2              = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    IDirect3DVertexBuffer9      *pVertexBuffer      = NULL;
    IDirect3DIndexBuffer9       *pIndexBuffer       = NULL;
    IDirect3DVertexDeclaration9 *pVertexDeclaration = NULL;
    IDirect3DVertexShader9      *pVertexShader      = NULL;
    IDirect3DPixelShader9       *pPixelShader       = NULL;
    IDirect3DCubeTexture9       *pCubeTexture       = NULL;
    IDirect3DTexture9           *pTexture           = NULL;
    IDirect3DVolumeTexture9     *pVolumeTexture     = NULL;
    IDirect3DVolume9            *pVolumeLevel       = NULL;
    IDirect3DSurface9           *pStencilSurface    = NULL;
    IDirect3DSurface9           *pOffscreenSurface  = NULL;
    IDirect3DSurface9           *pRenderTarget      = NULL;
    IDirect3DSurface9           *pRenderTarget2     = NULL;
    IDirect3DSurface9           *pRenderTarget3     = NULL;
    IDirect3DSurface9           *pTextureLevel      = NULL;
    IDirect3DSurface9           *pBackBuffer        = NULL;
    IDirect3DStateBlock9        *pStateBlock        = NULL;
    IDirect3DStateBlock9        *pStateBlock1       = NULL;
    IDirect3DSwapChain9         *pSwapChain         = NULL;
    IDirect3DQuery9             *pQuery             = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    int                          refcount = 0, tmp;

    D3DVERTEXELEMENT9 decl[] =
    {
        D3DDECL_END()
    };

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    CHECK_REFCOUNT( pD3d, 1 );

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == S_OK || hr == D3DERR_NOTAVAILABLE,
       "Failed to create IDirect3D9Device (%08x)\n", hr);
    if (FAILED(hr)) goto cleanup;

    refcount = get_refcount( (IUnknown *)pDevice );
    ok(refcount == 1, "Invalid device RefCount %d\n", refcount);

    CHECK_REFCOUNT( pD3d, 2 );

    hr = IDirect3DDevice9_GetDirect3D(pDevice, &pD3d2);
    CHECK_CALL( hr, "GetDirect3D", pDevice, refcount );

    ok(pD3d2 == pD3d, "Expected IDirect3D9 pointers to be equal\n");
    CHECK_REFCOUNT( pD3d, 3 );
    CHECK_RELEASE_REFCOUNT( pD3d, 2 );

    /**
     * Check refcount of implicit surfaces and implicit swapchain. Findings:
     *   - the container is the device OR swapchain
     *   - they hold a reference to the device
     *   - they are created with a refcount of 0 (Get/Release returns original refcount)
     *   - they are not freed if refcount reaches 0.
     *   - the refcount is not forwarded to the container.
     */
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 0, &pSwapChain);
    CHECK_CALL( hr, "GetSwapChain", pDevice, ++refcount);
    if (pSwapChain)
    {
        CHECK_REFCOUNT( pSwapChain, 1);

        hr = IDirect3DDevice9_GetRenderTarget(pDevice, 0, &pRenderTarget);
        CHECK_CALL( hr, "GetRenderTarget", pDevice, ++refcount);
        CHECK_REFCOUNT( pSwapChain, 1);
        if(pRenderTarget)
        {
            CHECK_SURFACE_CONTAINER( pRenderTarget, IID_IDirect3DSwapChain9, pSwapChain);
            CHECK_REFCOUNT( pRenderTarget, 1);

            CHECK_ADDREF_REFCOUNT(pRenderTarget, 2);
            CHECK_REFCOUNT(pDevice, refcount);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 1);
            CHECK_REFCOUNT(pDevice, refcount);

            hr = IDirect3DDevice9_GetRenderTarget(pDevice, 0, &pRenderTarget);
            CHECK_CALL( hr, "GetRenderTarget", pDevice, refcount);
            CHECK_REFCOUNT( pRenderTarget, 2);
            CHECK_RELEASE_REFCOUNT( pRenderTarget, 1);
            CHECK_RELEASE_REFCOUNT( pRenderTarget, 0);
            CHECK_REFCOUNT( pDevice, --refcount);

            /* The render target is released with the device, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pRenderTarget, 1);
            CHECK_REFCOUNT(pDevice, ++refcount);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
            CHECK_REFCOUNT(pDevice, --refcount);
        }

        /* Render target and back buffer are identical. */
        hr = IDirect3DDevice9_GetBackBuffer(pDevice, 0, 0, 0, &pBackBuffer);
        CHECK_CALL( hr, "GetBackBuffer", pDevice, ++refcount);
        if(pBackBuffer)
        {
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            ok(pRenderTarget == pBackBuffer, "RenderTarget=%p and BackBuffer=%p should be the same.\n",
            pRenderTarget, pBackBuffer);
            pBackBuffer = NULL;
        }
        CHECK_REFCOUNT( pDevice, --refcount);

        hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pStencilSurface);
        CHECK_CALL( hr, "GetDepthStencilSurface", pDevice, ++refcount);
        CHECK_REFCOUNT( pSwapChain, 1);
        if(pStencilSurface)
        {
            CHECK_SURFACE_CONTAINER( pStencilSurface, IID_IDirect3DDevice9, pDevice);
            CHECK_REFCOUNT( pStencilSurface, 1);

            CHECK_ADDREF_REFCOUNT(pStencilSurface, 2);
            CHECK_REFCOUNT(pDevice, refcount);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 1);
            CHECK_REFCOUNT(pDevice, refcount);

            CHECK_RELEASE_REFCOUNT( pStencilSurface, 0);
            CHECK_REFCOUNT( pDevice, --refcount);

            /* The stencil surface is released with the device, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pStencilSurface, 1);
            CHECK_REFCOUNT(pDevice, ++refcount);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
            CHECK_REFCOUNT(pDevice, --refcount);
            pStencilSurface = NULL;
        }

        CHECK_RELEASE_REFCOUNT( pSwapChain, 0);
        CHECK_REFCOUNT( pDevice, --refcount);

        /* The implicit swapchwin is released with the device, so AddRef with refcount=0 is fine here. */
        CHECK_ADDREF_REFCOUNT(pSwapChain, 1);
        CHECK_REFCOUNT(pDevice, ++refcount);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        CHECK_REFCOUNT(pDevice, --refcount);
        pSwapChain = NULL;
    }

    /* Buffers */
    hr = IDirect3DDevice9_CreateIndexBuffer( pDevice, 16, 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &pIndexBuffer, NULL );
    CHECK_CALL( hr, "CreateIndexBuffer", pDevice, ++refcount );
    if(pIndexBuffer)
    {
        tmp = get_refcount( (IUnknown *)pIndexBuffer );

        hr = IDirect3DDevice9_SetIndices(pDevice, pIndexBuffer);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
        hr = IDirect3DDevice9_SetIndices(pDevice, NULL);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
    }

    hr = IDirect3DDevice9_CreateVertexBuffer( pDevice, 16, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &pVertexBuffer, NULL );
    CHECK_CALL( hr, "CreateVertexBuffer", pDevice, ++refcount );
    if(pVertexBuffer)
    {
        IDirect3DVertexBuffer9 *pVBuf = (void*)~0;
        UINT offset = ~0;
        UINT stride = ~0;

        tmp = get_refcount( (IUnknown *)pVertexBuffer );

        hr = IDirect3DDevice9_SetStreamSource(pDevice, 0, pVertexBuffer, 0, 3 * sizeof(float));
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);
        hr = IDirect3DDevice9_SetStreamSource(pDevice, 0, NULL, 0, 0);
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);

        hr = IDirect3DDevice9_GetStreamSource(pDevice, 0, &pVBuf, &offset, &stride);
        ok(SUCCEEDED(hr), "GetStreamSource did not succeed with NULL stream!\n");
        ok(pVBuf==NULL, "pVBuf not NULL (%p)!\n", pVBuf);
        ok(stride==3*sizeof(float), "stride not 3 floats (got %u)!\n", stride);
        ok(offset==0, "offset not 0 (got %u)!\n", offset);
    }
    /* Shaders */
    hr = IDirect3DDevice9_CreateVertexDeclaration( pDevice, decl, &pVertexDeclaration );
    CHECK_CALL( hr, "CreateVertexDeclaration", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateVertexShader( pDevice, simple_vs, &pVertexShader );
    CHECK_CALL( hr, "CreateVertexShader", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreatePixelShader( pDevice, simple_ps, &pPixelShader );
    CHECK_CALL( hr, "CreatePixelShader", pDevice, ++refcount );
    /* Textures */
    hr = IDirect3DDevice9_CreateTexture( pDevice, 32, 32, 3, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pTexture, NULL );
    CHECK_CALL( hr, "CreateTexture", pDevice, ++refcount );
    if (pTexture)
    {
        tmp = get_refcount( (IUnknown *)pTexture );

        /* SetTexture should not increase refcounts */
        hr = IDirect3DDevice9_SetTexture(pDevice, 0, (IDirect3DBaseTexture9 *) pTexture);
        CHECK_CALL( hr, "SetTexture", pTexture, tmp);
        hr = IDirect3DDevice9_SetTexture(pDevice, 0, NULL);
        CHECK_CALL( hr, "SetTexture", pTexture, tmp);

        /* This should not increment device refcount */
        hr = IDirect3DTexture9_GetSurfaceLevel( pTexture, 1, &pTextureLevel );
        CHECK_CALL( hr, "GetSurfaceLevel", pDevice, refcount );
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
    hr = IDirect3DDevice9_CreateCubeTexture( pDevice, 32, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pCubeTexture, NULL );
    CHECK_CALL( hr, "CreateCubeTexture", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateVolumeTexture( pDevice, 32, 32, 2, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pVolumeTexture, NULL );
    CHECK_CALL( hr, "CreateVolumeTexture", pDevice, ++refcount );
    if (pVolumeTexture)
    {
        tmp = get_refcount( (IUnknown *)pVolumeTexture );

        /* This should not increment device refcount */
        hr = IDirect3DVolumeTexture9_GetVolumeLevel(pVolumeTexture, 0, &pVolumeLevel);
        CHECK_CALL( hr, "GetVolumeLevel", pDevice, refcount );
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
    hr = IDirect3DDevice9_CreateDepthStencilSurface( pDevice, 32, 32, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &pStencilSurface, NULL );
    CHECK_CALL( hr, "CreateDepthStencilSurface", pDevice, ++refcount );
    CHECK_REFCOUNT( pStencilSurface, 1 );
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface( pDevice, 32, 32, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pOffscreenSurface, NULL );
    CHECK_CALL( hr, "CreateOffscreenPlainSurface", pDevice, ++refcount );
    CHECK_REFCOUNT( pOffscreenSurface, 1 );
    hr = IDirect3DDevice9_CreateRenderTarget( pDevice, 32, 32, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRenderTarget3, NULL );
    CHECK_CALL( hr, "CreateRenderTarget", pDevice, ++refcount );
    CHECK_REFCOUNT( pRenderTarget3, 1 );
    /* Misc */
    hr = IDirect3DDevice9_CreateStateBlock( pDevice, D3DSBT_ALL, &pStateBlock );
    CHECK_CALL( hr, "CreateStateBlock", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateAdditionalSwapChain( pDevice, &d3dpp, &pSwapChain );
    CHECK_CALL( hr, "CreateAdditionalSwapChain", pDevice, ++refcount );
    if(pSwapChain)
    {
        /* check implicit back buffer */
        hr = IDirect3DSwapChain9_GetBackBuffer(pSwapChain, 0, 0, &pBackBuffer);
        CHECK_CALL( hr, "GetBackBuffer", pDevice, ++refcount);
        CHECK_REFCOUNT( pSwapChain, 1);
        if(pBackBuffer)
        {
            CHECK_SURFACE_CONTAINER( pBackBuffer, IID_IDirect3DSwapChain9, pSwapChain);
            CHECK_REFCOUNT( pBackBuffer, 1);
            CHECK_RELEASE_REFCOUNT( pBackBuffer, 0);
            CHECK_REFCOUNT( pDevice, --refcount);

            /* The back buffer is released with the swapchain, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pBackBuffer, 1);
            CHECK_REFCOUNT(pDevice, ++refcount);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_REFCOUNT(pDevice, --refcount);
            pBackBuffer = NULL;
        }
        CHECK_REFCOUNT( pSwapChain, 1);
    }
    hr = IDirect3DDevice9_CreateQuery( pDevice, D3DQUERYTYPE_EVENT, &pQuery );
    CHECK_CALL( hr, "CreateQuery", pDevice, ++refcount );

    hr = IDirect3DDevice9_BeginStateBlock( pDevice );
    CHECK_CALL( hr, "BeginStateBlock", pDevice, refcount );
    hr = IDirect3DDevice9_EndStateBlock( pDevice, &pStateBlock1 );
    CHECK_CALL( hr, "EndStateBlock", pDevice, ++refcount );

    /* The implicit render target is not freed if refcount reaches 0.
     * Otherwise GetRenderTarget would re-allocate it and the pointer would change.*/
    hr = IDirect3DDevice9_GetRenderTarget(pDevice, 0, &pRenderTarget2);
    CHECK_CALL( hr, "GetRenderTarget", pDevice, ++refcount);
    if(pRenderTarget2)
    {
        CHECK_RELEASE_REFCOUNT(pRenderTarget2, 0);
        ok(pRenderTarget == pRenderTarget2, "RenderTarget=%p and RenderTarget2=%p should be the same.\n",
           pRenderTarget, pRenderTarget2);
        CHECK_REFCOUNT( pDevice, --refcount);
        pRenderTarget2 = NULL;
    }
    pRenderTarget = NULL;

cleanup:
    CHECK_RELEASE(pDevice,              pDevice, --refcount);

    /* Buffers */
    CHECK_RELEASE(pVertexBuffer,        pDevice, --refcount);
    CHECK_RELEASE(pIndexBuffer,         pDevice, --refcount);
    /* Shaders */
    CHECK_RELEASE(pVertexDeclaration,   pDevice, --refcount);
    CHECK_RELEASE(pVertexShader,        pDevice, --refcount);
    CHECK_RELEASE(pPixelShader,         pDevice, --refcount);
    /* Textures */
    CHECK_RELEASE(pTextureLevel,        pDevice, --refcount);
    CHECK_RELEASE(pCubeTexture,         pDevice, --refcount);
    CHECK_RELEASE(pVolumeTexture,       pDevice, --refcount);
    /* Surfaces */
    CHECK_RELEASE(pStencilSurface,      pDevice, --refcount);
    CHECK_RELEASE(pOffscreenSurface,    pDevice, --refcount);
    CHECK_RELEASE(pRenderTarget3,       pDevice, --refcount);
    /* Misc */
    CHECK_RELEASE(pStateBlock,          pDevice, --refcount);
    CHECK_RELEASE(pSwapChain,           pDevice, --refcount);
    CHECK_RELEASE(pQuery,               pDevice, --refcount);
    /* This will destroy device - cannot check the refcount here */
    if (pStateBlock1)         CHECK_RELEASE_REFCOUNT( pStateBlock1, 0);

    if (pD3d)                 CHECK_RELEASE_REFCOUNT( pD3d, 0);

    DestroyWindow( hwnd );
}

static void test_cursor(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    CURSORINFO                   info;
    IDirect3DSurface9 *cursor = NULL;
    HCURSOR cur;

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    cur = info.hCursor;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == S_OK || hr == D3DERR_NOTAVAILABLE,
       "Failed to create IDirect3D9Device (%08x)\n", hr);
    if (FAILED(hr)) goto cleanup;

    IDirect3DDevice9_CreateOffscreenPlainSurface(pDevice, 32, 32, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &cursor, 0);
    ok(cursor != NULL, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);

    /* Initially hidden */
    hr = IDirect3DDevice9_ShowCursor(pDevice, TRUE);
    ok(hr == FALSE, "IDirect3DDevice9_ShowCursor returned %08x\n", hr);

    /* Not enabled without a surface*/
    hr = IDirect3DDevice9_ShowCursor(pDevice, TRUE);
    ok(hr == FALSE, "IDirect3DDevice9_ShowCursor returned %08x\n", hr);

    /* Fails */
    hr = IDirect3DDevice9_SetCursorProperties(pDevice, 0, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetCursorProperties returned %08x\n", hr);

    hr = IDirect3DDevice9_SetCursorProperties(pDevice, 0, 0, cursor);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetCursorProperties returned %08x\n", hr);

    IDirect3DSurface9_Release(cursor);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & CURSOR_SHOWING, "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor == cur, "The cursor handle is %p\n", info.hCursor); /* unchanged */

    /* Still hidden */
    hr = IDirect3DDevice9_ShowCursor(pDevice, TRUE);
    ok(hr == FALSE, "IDirect3DDevice9_ShowCursor returned %08x\n", hr);

    /* Enabled now*/
    hr = IDirect3DDevice9_ShowCursor(pDevice, TRUE);
    ok(hr == TRUE, "IDirect3DDevice9_ShowCursor returned %08x\n", hr);

    /* GDI cursor unchanged */
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & CURSOR_SHOWING, "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor == cur, "The cursor handle is %p\n", info.hCursor); /* unchanged */

cleanup:
    if (pDevice)
    {
        UINT refcount = IDirect3DDevice9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    DestroyWindow( hwnd );
}

static void test_reset(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    RECT                         winrect;
    IDirect3D9                  *pD3d               = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm, d3ddm2;
    D3DVIEWPORT9                 vp;
    DWORD                        width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    DWORD                        height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    IDirect3DSwapChain9          *pSwapchain;
    IDirect3DSurface9            *surface;
    IDirect3DTexture9            *texture;
    IDirect3DVertexShader9       *shader;
    UINT                         i, adapter_mode_count;
    D3DLOCKED_RECT               lockrect;
    IDirect3DDevice9 *device1 = NULL;
    IDirect3DDevice9 *device2 = NULL;
    D3DCAPS9 caps;
    DWORD value;
    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;
    UINT mode_count = 0;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    adapter_mode_count = IDirect3D9_GetAdapterModeCount(pD3d, D3DADAPTER_DEFAULT, d3ddm.Format);
    modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*modes) * adapter_mode_count);
    for(i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;
        ZeroMemory( &d3ddm2, sizeof(d3ddm2) );
        hr = IDirect3D9_EnumAdapterModes(pD3d, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(hr == D3D_OK, "IDirect3D9_EnumAdapterModes returned %#x\n", hr);

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

        /* We use them as invalid modes */
        if((d3ddm2.Width == 801 && d3ddm2.Height == 600) ||
           (d3ddm2.Width == 32 && d3ddm2.Height == 32)) {
            skip("This system supports a screen resolution of %dx%d, not running mode tests\n",
                 d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height) ++i;

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = FALSE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D9_CreateDevice(pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device1);
    if (FAILED(hr))
    {
        skip("could not create device, IDirect3D9_CreateDevice returned %#x\n", hr);
        goto cleanup;
    }
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after creation returned %#x\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device1, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u\n", height, modes[i].h);

    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
        ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u\n", vp.Width, modes[i].w);
        ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u\n", vp.Height, modes[i].h);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);
    }

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2;
    vp.MaxZ = 3;
    hr = IDirect3DDevice9_SetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device1, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
        ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u\n", vp.Width, modes[i].w);
        ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u\n", vp.Height, modes[i].h);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);
    }

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u\n", height, modes[i].h);

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &pSwapchain);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetSwapChain returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ZeroMemory(&d3dpp, sizeof(d3dpp));
        hr = IDirect3DSwapChain9_GetPresentParameters(pSwapchain, &d3dpp);
        ok(hr == D3D_OK, "IDirect3DSwapChain9_GetPresentParameters returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            ok(d3dpp.BackBufferWidth == modes[i].w, "Back buffer width is %u, expected %u\n",
                    d3dpp.BackBufferWidth, modes[i].w);
            ok(d3dpp.BackBufferHeight == modes[i].h, "Back buffer height is %u, expected %u\n",
                    d3dpp.BackBufferHeight, modes[i].h);
        }
        IDirect3DSwapChain9_Release(pSwapchain);
    }

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Screen width is %d\n", width);
    ok(height == orig_height, "Screen height is %d\n", height);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
        ok(vp.Width == 400, "D3DVIEWPORT->Width = %d\n", vp.Width);
        ok(vp.Height == 300, "D3DVIEWPORT->Height = %d\n", vp.Height);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);
    }

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &pSwapchain);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetSwapChain returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ZeroMemory(&d3dpp, sizeof(d3dpp));
        hr = IDirect3DSwapChain9_GetPresentParameters(pSwapchain, &d3dpp);
        ok(hr == D3D_OK, "IDirect3DSwapChain9_GetPresentParameters returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            ok(d3dpp.BackBufferWidth == 400, "Back buffer width is %d\n", d3dpp.BackBufferWidth);
            ok(d3dpp.BackBufferHeight == 300, "Back buffer height is %d\n", d3dpp.BackBufferHeight);
        }
        IDirect3DSwapChain9_Release(pSwapchain);
    }

    winrect.left = 0;
    winrect.top = 0;
    winrect.right = 200;
    winrect.bottom = 150;
    ok(AdjustWindowRect(&winrect, WS_OVERLAPPEDWINDOW, FALSE), "AdjustWindowRect failed\n");
    ok(SetWindowPos(hwnd, NULL, 0, 0,
                    winrect.right-winrect.left,
                    winrect.bottom-winrect.top,
                    SWP_NOMOVE|SWP_NOZORDER),
       "SetWindowPos failed\n");

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 0;
    d3dpp.BackBufferHeight = 0;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
        ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
        ok(vp.Width == 200, "D3DVIEWPORT->Width = %d\n", vp.Width);
        ok(vp.Height == 150, "D3DVIEWPORT->Height = %d\n", vp.Height);
        ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
        ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);
    }

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &pSwapchain);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetSwapChain returned %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        ZeroMemory(&d3dpp, sizeof(d3dpp));
        hr = IDirect3DSwapChain9_GetPresentParameters(pSwapchain, &d3dpp);
        ok(hr == D3D_OK, "IDirect3DSwapChain9_GetPresentParameters returned %08x\n", hr);
        if(SUCCEEDED(hr))
        {
            ok(d3dpp.BackBufferWidth == 200, "Back buffer width is %d\n", d3dpp.BackBufferWidth);
            ok(d3dpp.BackBufferHeight == 150, "Back buffer height is %d\n", d3dpp.BackBufferHeight);
        }
        IDirect3DSwapChain9_Release(pSwapchain);
    }

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;

    /* _Reset fails if there is a resource in the default pool */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);
    /* Reset again to get the device out of the lost state */
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture;

        hr = IDirect3DDevice9_CreateVolumeTexture(device1, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "CreateVolumeTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_Reset(device1, &d3dpp);
        ok(hr == D3DERR_INVALIDCALL, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
        hr = IDirect3DDevice9_TestCooperativeLevel(device1);
        ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n",
                hr, D3DERR_DEVICENOTRESET);
        IDirect3DVolumeTexture9_Release(volume_texture);
        hr = IDirect3DDevice9_Reset(device1, &d3dpp);
        ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_TestCooperativeLevel(device1);
        ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Scratch, sysmem and managed pools are fine */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice9_SetDepthStencilSurface(device1, NULL);
    ok(hr == D3D_OK, "SetDepthStencilSurface failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface9_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    /* Will a sysmem or scratch survive while locked */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_UnlockRect(surface);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_UnlockRect(surface);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateTexture(device1, 16, 16, 0, 0, D3DFMT_R5G6B5, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DTexture9_Release(texture);

    /* A reference held to an implicit surface causes failures as well */
    hr = IDirect3DDevice9_GetBackBuffer(device1, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    /* Shaders are fine as well */
    hr = IDirect3DDevice9_CreateVertexShader(device1, simple_vs, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    IDirect3DVertexShader9_Release(shader);

    /* Try setting invalid modes */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 32;
    d3dpp.BackBufferHeight = 32;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=32, h=32, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 801;
    d3dpp.BackBufferHeight = 600;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=801, h=600, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 0;
    d3dpp.BackBufferHeight = 0;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=0, h=0, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D9_CreateDevice(pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device2);
    if (FAILED(hr))
    {
        skip("could not create device, IDirect3D9_CreateDevice returned %#x\n", hr);
        goto cleanup;
    }

    hr = IDirect3DDevice9_TestCooperativeLevel(device2);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after creation returned %#x\n", hr);

    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3DDevice9_Reset(device2, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_GetDepthStencilSurface(device2, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface9_Release(surface);

cleanup:
    HeapFree(GetProcessHeap(), 0, modes);
    if (device2)
    {
        UINT refcount = IDirect3DDevice9_Release(device2);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (device1)
    {
        UINT refcount = IDirect3DDevice9_Release(device1);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    if (hwnd) DestroyWindow(hwnd);
}

/* Test adapter display modes */
static void test_display_modes(void)
{
    D3DDISPLAYMODE dmode;
    IDirect3D9 *pD3d;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    if(!pD3d) return;

#define TEST_FMT(x,r) do { \
    HRESULT res = IDirect3D9_EnumAdapterModes(pD3d, 0, (x), 0, &dmode); \
    ok(res==(r), "EnumAdapterModes("#x") did not return "#r" (got %08x)!\n", res); \
} while(0)

    TEST_FMT(D3DFMT_R8G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8R8G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8B8G8R8, D3DERR_INVALIDCALL);
    /* D3DFMT_R5G6B5 */
    TEST_FMT(D3DFMT_X1R5G5B5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A1R5G5B5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A4R4G4B4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_R3G3B2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8R3G3B2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X4R4G4B4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A2B10G10R10, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8B8G8R8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8B8G8R8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G16R16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A16B16G16R16, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_A8P8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_P8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_L8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8L8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A4L4, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_L6V5U5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8L8V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_Q8W8V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_V16U16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A2W10V10U10, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_UYVY, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_YUY2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT1, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT3, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_MULTI2_ARGB8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G8R8_G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_R8G8_B8G8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_D16_LOCKABLE, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D32, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D15S1, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24S8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24X8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24X4S4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_L16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D32F_LOCKABLE, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24FS8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_VERTEXDATA, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_INDEX16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_INDEX32, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_Q16W16V16U16, D3DERR_INVALIDCALL);
    /* Floating point formats */
    TEST_FMT(D3DFMT_R16F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G16R16F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A16B16G16R16F, D3DERR_INVALIDCALL);

    /* IEEE formats */
    TEST_FMT(D3DFMT_R32F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G32R32F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A32B32G32R32F, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_CxV8U8, D3DERR_INVALIDCALL);

    TEST_FMT(0, D3DERR_INVALIDCALL);

    IDirect3D9_Release(pD3d);
}

static void test_scene(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    IDirect3DSurface9            *pSurface1 = NULL, *pSurface2 = NULL, *pSurface3 = NULL, *pRenderTarget = NULL;
    IDirect3DSurface9            *pBackBuffer = NULL, *pDepthStencil = NULL;
    RECT rect = {0, 0, 128, 128};
    D3DCAPS9                     caps;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    /* Get the caps, they will be needed to tell if an operation is supposed to be valid */
    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(pDevice, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetCaps failed with %08x\n", hr);
    if(FAILED(hr)) goto cleanup;

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice9_EndScene(pDevice);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice9_BeginScene(pDevice);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice9_EndScene(pDevice);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    }

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice9_EndScene(pDevice);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice9_BeginScene(pDevice);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(pDevice);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(pDevice);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(pDevice);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Create some surfaces to test stretchrect between the scenes */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(pDevice, 128, 128, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pSurface1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(pDevice, 128, 128, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pSurface2, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(pDevice, 800, 600, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE, &pSurface3, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateDepthStencilSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(pDevice, 128, 128, d3ddm.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &pRenderTarget, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateRenderTarget failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(pDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed with %08x\n", hr);

    /* First make sure a simple StretchRect call works */
    if(pSurface1 && pSurface2) {
        hr = IDirect3DDevice9_StretchRect(pDevice, pSurface1, NULL, pSurface2, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }
    if(pBackBuffer && pRenderTarget) {
        hr = IDirect3DDevice9_StretchRect(pDevice, pBackBuffer, &rect, pRenderTarget, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }
    if(pDepthStencil && pSurface3) {
        HRESULT expected;
        if(0) /* Disabled for now because it crashes in wine */ {
            expected = caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES ? D3D_OK : D3DERR_INVALIDCALL;
            hr = IDirect3DDevice9_StretchRect(pDevice, pDepthStencil, NULL, pSurface3, NULL, 0);
            ok( hr == expected, "IDirect3DDevice9_StretchRect returned %08x, expected %08x\n", hr, expected);
        }
    }

    /* Now try it in a BeginScene - EndScene pair. Seems to be allowed in a beginScene - Endscene pair
     * with normal surfaces and render targets, but not depth stencil surfaces.
     */
    hr = IDirect3DDevice9_BeginScene(pDevice);
    ok( hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);

    if(pSurface1 && pSurface2)
    {
        hr = IDirect3DDevice9_StretchRect(pDevice, pSurface1, NULL, pSurface2, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }
    if(pBackBuffer && pRenderTarget)
    {
        hr = IDirect3DDevice9_StretchRect(pDevice, pBackBuffer, &rect, pRenderTarget, NULL, 0);
        ok( hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    }
    if(pDepthStencil && pSurface3)
    {
        /* This is supposed to fail inside a BeginScene - EndScene pair. */
        hr = IDirect3DDevice9_StretchRect(pDevice, pDepthStencil, NULL, pSurface3, NULL, 0);
        ok( hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect returned %08x, expected D3DERR_INVALIDCALL\n", hr);
    }

    hr = IDirect3DDevice9_EndScene(pDevice);
    ok( hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

    /* Does a SetRenderTarget influence BeginScene / EndScene ?
     * Set a new render target, then see if it started a new scene. Flip the rt back and see if that maybe
     * ended the scene. Expected result is that the scene is not affected by SetRenderTarget
     */
    hr = IDirect3DDevice9_SetRenderTarget(pDevice, 0, pRenderTarget);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(pDevice);
    ok( hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(pDevice, 0, pBackBuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(pDevice);
    ok( hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

cleanup:
    if(pRenderTarget) IDirect3DSurface9_Release(pRenderTarget);
    if(pDepthStencil) IDirect3DSurface9_Release(pDepthStencil);
    if(pBackBuffer) IDirect3DSurface9_Release(pBackBuffer);
    if(pSurface1) IDirect3DSurface9_Release(pSurface1);
    if(pSurface2) IDirect3DSurface9_Release(pSurface2);
    if(pSurface3) IDirect3DSurface9_Release(pSurface3);
    if (pDevice)
    {
        UINT refcount = IDirect3DDevice9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    if(hwnd) DestroyWindow(hwnd);
}

static void test_limits(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    IDirect3DTexture9           *pTexture           = NULL;
    int i;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateTexture(pDevice, 16, 16, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);
    if(!pTexture) goto cleanup;

    /* There are 16 pixel samplers. We should be able to access all of them */
    for(i = 0; i < 16; i++) {
        hr = IDirect3DDevice9_SetTexture(pDevice, i, (IDirect3DBaseTexture9 *) pTexture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_SetTexture(pDevice, i, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_SetSamplerState(pDevice, i, D3DSAMP_SRGBTEXTURE, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState for sampler %d failed with %08x\n", i, hr);
    }

    /* Now test all 8 textures stage states */
    for(i = 0; i < 8; i++) {
        hr = IDirect3DDevice9_SetTextureStageState(pDevice, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState for texture %d failed with %08x\n", i, hr);
    }

    /* Investigations show that accessing higher samplers / textures stage states does not return an error either. Writing
     * to too high samplers(approximately sampler 40) causes memory corruption in windows, so there is no bounds checking
     * but how do I test that?
     */
cleanup:
    if(pTexture) IDirect3DTexture9_Release(pTexture);
    if (pDevice)
    {
        UINT refcount = IDirect3D9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    if(hwnd) DestroyWindow(hwnd);
}

static void test_depthstenciltest(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    IDirect3DSurface9           *pDepthStencil           = NULL;
    IDirect3DSurface9           *pDepthStencil2          = NULL;
    DWORD                        state;

    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3D_OK && pDepthStencil != NULL, "IDirect3DDevice9_GetDepthStencilSurface failed with %08x\n", hr);

    /* Try to clear */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(pDevice, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetDepthStencilSurface failed with %08x\n", hr);

    /* Check if the set buffer is returned on a get. WineD3D had a bug with that once, prevent it from coming back */
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil2);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil2 == NULL, "IDirect3DDevice9_GetDepthStencilSurface failed with %08x\n", hr);
    if(pDepthStencil2) IDirect3DSurface9_Release(pDepthStencil2);

    /* This left the render states untouched! */
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_TRUE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZWRITEENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == TRUE, "D3DRS_ZWRITEENABLE is %s\n", state ? "TRUE" : "FALSE");
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_STENCILENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == FALSE, "D3DRS_STENCILENABLE is %s\n", state ? "TRUE" : "FALSE");
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_STENCILWRITEMASK, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == 0xffffffff, "D3DRS_STENCILWRITEMASK is 0x%08x\n", state);

    /* This is supposed to fail now */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(pDevice, pDepthStencil);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetDepthStencilSurface failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

    /* Now it works again */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    if(pDepthStencil) IDirect3DSurface9_Release(pDepthStencil);
    if(pDevice) IDirect3D9_Release(pDevice);

    /* Now see if autodepthstencil disable is honored. First, without a format set */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    pDepthStencil = NULL;
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil == NULL, "IDirect3DDevice9_GetDepthStencilSurface returned %08x, surface = %p\n", hr, pDepthStencil);
    if(pDepthStencil) {
        IDirect3DSurface9_Release(pDepthStencil);
        pDepthStencil = NULL;
    }

    /* Check the depth test state */
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

    if(pDevice) IDirect3D9_Release(pDevice);

    /* Next, try EnableAutoDepthStencil FALSE with a depth stencil format set */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    pDepthStencil = NULL;
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil == NULL, "IDirect3DDevice9_GetDepthStencilSurface returned %08x, surface = %p\n", hr, pDepthStencil);
    if(pDepthStencil) {
        IDirect3DSurface9_Release(pDepthStencil);
        pDepthStencil = NULL;
    }

    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

cleanup:
    if(pDepthStencil) IDirect3DSurface9_Release(pDepthStencil);
    if (pDevice)
    {
        UINT refcount = IDirect3D9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (pD3d) IDirect3D9_Release(pD3d);
    if(hwnd) DestroyWindow(hwnd);
}

static void test_get_rt(void)
{
    IDirect3DSurface9 *backbuffer, *rt;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;
    UINT i;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 128, 128, 0, 0, 0, 0);
    device = create_device(d3d9, window, window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    ok(!!backbuffer, "Got a NULL backbuffer.\n");

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        rt = backbuffer;
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &rt);
        ok(hr == D3DERR_NOTFOUND, "IDirect3DDevice9_GetRenderTarget returned %#x.\n", hr);
        ok(!rt, "Got rt %p.\n", rt);
    }

    IDirect3DSurface9_Release(backbuffer);

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "The device was not properly freed: refcount %u.\n", ref);
done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

/* Test what happens when IDirect3DDevice9_DrawIndexedPrimitive is called without a valid index buffer set. */
static void test_draw_indexed(void)
{
    static const struct {
        float position[3];
        DWORD color;
    } quad[] = {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.0f}, 0xffff0000},
    };
    WORD indices[] = {0, 1, 2, 3, 0, 2};

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    IDirect3DIndexBuffer9 *index_buffer = NULL;
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    HRESULT hr;
    HWND hwnd;
    void *ptr;

    hwnd = CreateWindow("d3d9_test_wc", "d3d9_test",
            0, 0, 0, 10, 10, 0, 0, 0, 0);
    if (!hwnd)
    {
        skip("Failed to create window\n");
        return;
    }

    d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
    if (!d3d9)
    {
        skip("Failed to create IDirect3D9 object\n");
        goto cleanup;
    }

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if (FAILED(hr) || !device)
    {
        skip("Failed to create device\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, 0, D3DPOOL_DEFAULT, &vertex_buffer, NULL);
    ok(SUCCEEDED(hr), "CreateVertexBuffer failed (0x%08x)\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Lock failed (0x%08x)\n", hr);
    memcpy(ptr, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    ok(SUCCEEDED(hr), "Unlock failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vertex_buffer, 0, sizeof(*quad));
    ok(SUCCEEDED(hr), "SetStreamSource failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateIndexBuffer(device, sizeof(indices), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &index_buffer, NULL);
    ok(SUCCEEDED(hr), "CreateIndexBuffer failed (0x%08x)\n", hr);
    hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Lock failed (0x%08x)\n", hr);
    memcpy(ptr, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(index_buffer);
    ok(SUCCEEDED(hr), "Unlock failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState D3DRS_LIGHTING failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

    /* NULL index buffer. Should fail */
    hr = IDirect3DDevice9_SetIndices(device, NULL);
    ok(SUCCEEDED(hr), "SetIndices failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */, 0 /* MinIndex */,
            4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(hr == D3DERR_INVALIDCALL, "DrawIndexedPrimitive returned 0x%08x, expected D3DERR_INVALIDCALL (0x%08x)\n",
            hr, D3DERR_INVALIDCALL);

    /* Valid index buffer, NULL vertex declaration. Should fail */
    hr = IDirect3DDevice9_SetIndices(device, index_buffer);
    ok(SUCCEEDED(hr), "SetIndices failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */, 0 /* MinIndex */,
            4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(hr == D3DERR_INVALIDCALL, "DrawIndexedPrimitive returned 0x%08x, expected D3DERR_INVALIDCALL (0x%08x)\n",
            hr, D3DERR_INVALIDCALL);

    /* Valid index buffer and vertex declaration. Should succeed */
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */, 0 /* MinIndex */,
            4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(SUCCEEDED(hr), "DrawIndexedPrimitive failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

    IDirect3DVertexBuffer9_Release(vertex_buffer);
    IDirect3DIndexBuffer9_Release(index_buffer);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);

cleanup:
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (d3d9) IDirect3D9_Release(d3d9);
    if (hwnd) DestroyWindow(hwnd);
}

static void test_null_stream(void)
{
    IDirect3DVertexBuffer9 *buffer = NULL;
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    HWND hwnd;
    HRESULT hr;
    IDirect3DVertexShader9 *shader = NULL;
    IDirect3DVertexDeclaration9 *decl = NULL;
    static const DWORD shader_code[] =
    {
        0xfffe0101,                             /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,     /* dcl_position v0  */
        0x00000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0     */
        0x0000ffff                              /* end              */
    };
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    d3d9 = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(d3d9 != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!d3d9 || !hwnd) goto cleanup;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice( d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!device)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    if(FAILED(hr)) {
        skip("No vertex shader support\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexDeclaration failed (0x%08x)\n", hr);
    if (FAILED(hr)) {
        skip("Vertex declaration handling not possible.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVertexBuffer(device, 12 * sizeof(float), 0, 0, D3DPOOL_MANAGED, &buffer, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexBuffer failed (0x%08x)\n", hr);
    if (FAILED(hr)) {
        skip("Vertex buffer handling not possible.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(float) * 3);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetStreamSource failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 1, NULL, 0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetStreamSource failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed (0x%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_POINTLIST, 0, 1);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_DrawPrimitive failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed (0x%08x)\n", hr);
    }

    IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    IDirect3DDevice9_SetVertexShader(device, NULL);
    IDirect3DDevice9_SetVertexDeclaration(device, NULL);

cleanup:
    if (buffer) IDirect3DVertexBuffer9_Release(buffer);
    if(decl) IDirect3DVertexDeclaration9_Release(decl);
    if(shader) IDirect3DVertexShader9_Release(shader);
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if(d3d9) IDirect3D9_Release(d3d9);
}

static void test_lights(void)
{
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    HWND hwnd;
    HRESULT hr;
    unsigned int i;
    BOOL enabled;
    D3DCAPS9 caps;

    d3d9 = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(d3d9 != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!d3d9 || !hwnd) goto cleanup;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice( d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE || hr == D3DERR_INVALIDCALL,
       "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!device)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed with %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice9_LightEnable(device, i, TRUE);
        ok(hr == D3D_OK, "Enabling light %u failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_GetLightEnable(device, i, &enabled);
        ok(hr == D3D_OK, "GetLightEnable on light %u failed with %08x\n", i, hr);
        ok(enabled, "Light %d is %s\n", i, enabled ? "enabled" : "disabled");
    }

    /* TODO: Test the rendering results in this situation */
    hr = IDirect3DDevice9_LightEnable(device, i + 1, TRUE);
    ok(hr == D3D_OK, "Enabling one light more than supported returned %08x\n", hr);
    hr = IDirect3DDevice9_GetLightEnable(device, i + 1, &enabled);
    ok(hr == D3D_OK, "GetLightEnable on light %u failed with %08x\n", i + 1, hr);
    ok(enabled, "Light %d is %s\n", i + 1, enabled ? "enabled" : "disabled");
    hr = IDirect3DDevice9_LightEnable(device, i + 1, FALSE);
    ok(hr == D3D_OK, "Disabling the additional returned %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice9_LightEnable(device, i, FALSE);
        ok(hr == D3D_OK, "Disabling light %u failed with %08x\n", i, hr);
    }

cleanup:
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if(d3d9) IDirect3D9_Release(d3d9);
}

static void test_set_stream_source(void)
{
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    HWND hwnd;
    HRESULT hr;
    IDirect3DVertexBuffer9 *pVertexBuffer = NULL;

    d3d9 = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(d3d9 != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!d3d9 || !hwnd) goto cleanup;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice( d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device );
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE || hr == D3DERR_INVALIDCALL,
       "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!device)
    {
        hr = IDirect3D9_CreateDevice( d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hwnd,
                                      D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device );
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE || hr == D3DERR_INVALIDCALL,
           "IDirect3D9_CreateDevice failed with %08x\n", hr);
        if(!device)
        {
            skip("Failed to create a d3d device\n");
            goto cleanup;
        }
    }

    hr = IDirect3DDevice9_CreateVertexBuffer( device, 512, 0, 0, D3DPOOL_DEFAULT, &pVertexBuffer, NULL );
    ok(hr == D3D_OK, "Failed to create a vertex buffer, hr = %08x\n", hr);
    if (SUCCEEDED(hr)) {
        /* Some cards(Geforce 7400 at least) accept non-aligned offsets, others(radeon 9000 verified) reject it,
         * so accept both results. Wine currently rejects this to be able to optimize the vbo conversion, but writes
         * a WARN
         */
        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 0, 32);
        ok(hr == D3D_OK, "Failed to set the stream source, offset 0, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 1, 32);
        ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 1, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 2, 32);
        ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 2, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 3, 32);
        ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 3, hr = %08x\n", hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 4, 32);
        ok(hr == D3D_OK, "Failed to set the stream source, offset 4, hr = %08x\n", hr);
    }
    /* Try to set the NULL buffer with an offset and stride 0 */
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "Failed to set the stream source, offset 0, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 1, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 1, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 2, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 2, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 3, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Unexpected result when setting the stream source, offset 3, hr = %08x\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 4, 0);
    ok(hr == D3D_OK, "Failed to set the stream source, offset 4, hr = %08x\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "Failed to set the stream source, offset 4, hr = %08x\n", hr);

cleanup:
    if (pVertexBuffer) IDirect3DVertexBuffer9_Release(pVertexBuffer);
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if(d3d9) IDirect3D9_Release(d3d9);
}

struct formats {
    D3DFORMAT DisplayFormat;
    D3DFORMAT BackBufferFormat;
    BOOL shouldPass;
};

static const struct formats r5g6b5_format_list[] =
{
    { D3DFMT_R5G6B5, D3DFMT_R5G6B5, TRUE },
    { D3DFMT_R5G6B5, D3DFMT_X1R5G5B5, FALSE },
    { D3DFMT_R5G6B5, D3DFMT_A1R5G5B5, FALSE },
    { D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, FALSE },
    { D3DFMT_R5G6B5, D3DFMT_A8R8G8B8, FALSE },
    { 0, 0, 0}
};

static const struct formats x1r5g5b5_format_list[] =
{
    { D3DFMT_X1R5G5B5, D3DFMT_R5G6B5, FALSE },
    { D3DFMT_X1R5G5B5, D3DFMT_X1R5G5B5, TRUE },
    { D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5, TRUE },
    { D3DFMT_X1R5G5B5, D3DFMT_X8R8G8B8, FALSE },
    { D3DFMT_X1R5G5B5, D3DFMT_A8R8G8B8, FALSE },

    /* A1R5G5B5 should not be usable as a display format, it is backbuffer-only */
    { D3DFMT_A1R5G5B5, D3DFMT_R5G6B5, FALSE },
    { D3DFMT_A1R5G5B5, D3DFMT_X1R5G5B5, FALSE },
    { D3DFMT_A1R5G5B5, D3DFMT_A1R5G5B5, FALSE },
    { D3DFMT_A1R5G5B5, D3DFMT_X8R8G8B8, FALSE },
    { D3DFMT_A1R5G5B5, D3DFMT_A8R8G8B8, FALSE },
    { 0, 0, 0}
};

static const struct formats x8r8g8b8_format_list[] =
{
    { D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, FALSE },
    { D3DFMT_X8R8G8B8, D3DFMT_X1R5G5B5, FALSE },
    { D3DFMT_X8R8G8B8, D3DFMT_A1R5G5B5, FALSE },
    { D3DFMT_X8R8G8B8, D3DFMT_X8R8G8B8, TRUE },
    { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, TRUE },

    /* A1R8G8B8 should not be usable as a display format, it is backbuffer-only */
    { D3DFMT_A8R8G8B8, D3DFMT_R5G6B5, FALSE },
    { D3DFMT_A8R8G8B8, D3DFMT_X1R5G5B5, FALSE },
    { D3DFMT_A8R8G8B8, D3DFMT_A1R5G5B5, FALSE },
    { D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, FALSE },
    { D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8, FALSE },
    { 0, 0, 0}
};

static void test_display_formats(void)
{
    /* Direct3D9 offers 4 display formats R5G6B5, X1R5G5B5, X8R8G8B8 and A2R10G10B10.
     * Next to these there are 6 different backbuffer formats. Only a fixed number of
     * combinations are possible in FULLSCREEN mode. In windowed mode more combinations are
     * allowed due to depth conversion and this is likely driver dependent.
     * This test checks which combinations are possible in fullscreen mode and this should not be driver dependent.
     * TODO: handle A2R10G10B10 but what hardware supports it? Parhelia? It is very rare. */

    UINT Adapter = D3DADAPTER_DEFAULT;
    D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;
    int i, nmodes;
    HRESULT hr;

    IDirect3D9 *d3d9 = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(d3d9 != NULL, "Failed to create IDirect3D9 object\n");
    if(!d3d9) return;

    nmodes = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, D3DFMT_R5G6B5);
    if(!nmodes) {
        skip("Display format R5G6B5 not supported, skipping\n");
    } else {
        trace("Testing display format R5G6B5\n");
        for(i=0; r5g6b5_format_list[i].DisplayFormat != 0; i++)
        {
            hr = IDirect3D9_CheckDeviceType(d3d9, Adapter, DeviceType, r5g6b5_format_list[i].DisplayFormat, r5g6b5_format_list[i].BackBufferFormat, FALSE);

            if(r5g6b5_format_list[i].shouldPass)
                ok(hr == D3D_OK ||
                   broken(hr == D3DERR_NOTAVAILABLE) /* Windows VGA driver */,
                   "format %d %d didn't pass with hr=%#08x\n", r5g6b5_format_list[i].DisplayFormat, r5g6b5_format_list[i].BackBufferFormat, hr);
            else
                ok(hr != D3D_OK, "format %d %d didn't pass while it was expected to\n", r5g6b5_format_list[i].DisplayFormat, r5g6b5_format_list[i].BackBufferFormat);
        }
    }

    nmodes = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X1R5G5B5);
    if(!nmodes) {
        skip("Display format X1R5G5B5 not supported, skipping\n");
    } else {
        trace("Testing display format X1R5G5B5\n");
        for(i=0; x1r5g5b5_format_list[i].DisplayFormat != 0; i++)
        {
            hr = IDirect3D9_CheckDeviceType(d3d9, Adapter, DeviceType, x1r5g5b5_format_list[i].DisplayFormat, x1r5g5b5_format_list[i].BackBufferFormat, FALSE);

            if(x1r5g5b5_format_list[i].shouldPass)
                ok(hr == D3D_OK ||
                   broken(hr == D3DERR_NOTAVAILABLE) /* Spice QXL driver */,
                   "format %d %d didn't pass with hr=%#08x\n", x1r5g5b5_format_list[i].DisplayFormat, x1r5g5b5_format_list[i].BackBufferFormat, hr);
            else
                ok(hr != D3D_OK, "format %d %d didn't pass while it was expected to\n", x1r5g5b5_format_list[i].DisplayFormat, x1r5g5b5_format_list[i].BackBufferFormat);
        }
    }

    nmodes = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    if(!nmodes) {
        skip("Display format X8R8G8B8 not supported, skipping\n");
    } else {
        trace("Testing display format X8R8G8B8\n");
        for(i=0; x8r8g8b8_format_list[i].DisplayFormat != 0; i++)
        {
            hr = IDirect3D9_CheckDeviceType(d3d9, Adapter, DeviceType, x8r8g8b8_format_list[i].DisplayFormat, x8r8g8b8_format_list[i].BackBufferFormat, FALSE);
            trace("CheckDeviceType(%d %d) = %08x shouldPass = %d\n", x8r8g8b8_format_list[i].DisplayFormat, x8r8g8b8_format_list[i].BackBufferFormat, hr, x8r8g8b8_format_list[i].shouldPass);

            if(x8r8g8b8_format_list[i].shouldPass)
                ok(hr == D3D_OK ||
                   broken(hr == D3DERR_NOTAVAILABLE) /* Windows VGA driver */,
                   "format %d %d didn't pass with hr=%#08x\n", x8r8g8b8_format_list[i].DisplayFormat, x8r8g8b8_format_list[i].BackBufferFormat, hr);
            else
                ok(hr != D3D_OK, "format %d %d didn't pass while it was expected to\n", x8r8g8b8_format_list[i].DisplayFormat, x8r8g8b8_format_list[i].BackBufferFormat);
        }
    }

    if(d3d9) IDirect3D9_Release(d3d9);
}

static void test_scissor_size(void)
{
    IDirect3D9 *d3d9_ptr = 0;
    unsigned int i;
    static struct {
        int winx; int winy; int backx; int backy; BOOL window;
    } scts[] = { /* scissor tests */
        {800, 600, 640, 480, TRUE},
        {800, 600, 640, 480, FALSE},
        {640, 480, 800, 600, TRUE},
        {640, 480, 800, 600, FALSE},
    };

    d3d9_ptr = pDirect3DCreate9(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr){
        skip("Failed to create IDirect3D9 object\n");
        return;
    }

    for(i=0; i<sizeof(scts)/sizeof(scts[0]); i++) {
        IDirect3DDevice9 *device_ptr = 0;
        D3DPRESENT_PARAMETERS present_parameters;
        HRESULT hr;
        HWND hwnd = 0;
        RECT scissorrect;

        hwnd = CreateWindow("d3d9_test_wc", "d3d9_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, scts[i].winx, scts[i].winy, 0, 0, 0, 0);

        if (!scts[i].window)
        {
            scts[i].backx = screen_width;
            scts[i].backy = screen_height;
        }

        ZeroMemory(&present_parameters, sizeof(present_parameters));
        present_parameters.Windowed = scts[i].window;
        present_parameters.hDeviceWindow = hwnd;
        present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present_parameters.BackBufferWidth = scts[i].backx;
        present_parameters.BackBufferHeight = scts[i].backy;
        present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
        present_parameters.EnableAutoDepthStencil = TRUE;
        present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

        hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
        if(FAILED(hr)) {
            present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
            hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
            if(FAILED(hr)) {
                hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
            }
        }
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D_CreateDevice returned: %08x\n", hr);

        if (!device_ptr)
        {
            DestroyWindow(hwnd);
            skip("Creating the device failed\n");
            goto err_out;
        }

        /* Check for the default scissor rect size */
        hr = IDirect3DDevice9_GetScissorRect(device_ptr, &scissorrect);
        ok(hr == D3D_OK, "IDirect3DDevice9_GetScissorRect failed with: %08x\n", hr);
        ok(scissorrect.right == scts[i].backx && scissorrect.bottom == scts[i].backy && scissorrect.top == 0 && scissorrect.left == 0, "Scissorrect missmatch (%d, %d) should be (%d, %d)\n", scissorrect.right, scissorrect.bottom, scts[i].backx, scts[i].backy);

        /* check the scissorrect values after a reset */
        present_parameters.BackBufferWidth = screen_width;
        present_parameters.BackBufferHeight = screen_height;
        hr = IDirect3DDevice9_Reset(device_ptr, &present_parameters);
        ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
        hr = IDirect3DDevice9_TestCooperativeLevel(device_ptr);
        ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

        hr = IDirect3DDevice9_GetScissorRect(device_ptr, &scissorrect);
        ok(hr == D3D_OK, "IDirect3DDevice9_GetScissorRect failed with: %08x\n", hr);
        ok(scissorrect.right == screen_width && scissorrect.bottom == screen_height && scissorrect.top == 0 && scissorrect.left == 0, "Scissorrect missmatch (%d, %d) should be (%d, %d)\n", scissorrect.right, scissorrect.bottom, screen_width, screen_height);

        if(device_ptr) {
            ULONG ref;

            ref = IDirect3DDevice9_Release(device_ptr);
            DestroyWindow(hwnd);
            ok(ref == 0, "The device was not properly freed: refcount %u\n", ref);
        }
    }

err_out:
    if(d3d9_ptr) IDirect3D9_Release(d3d9_ptr);
    return;
}

static void test_multi_device(void)
{
    IDirect3DDevice9 *device1 = NULL, *device2 = NULL;
    D3DPRESENT_PARAMETERS present_parameters;
    HWND hwnd1 = NULL, hwnd2 = NULL;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HRESULT hr;

    d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
    ok(d3d9 != NULL, "Failed to create a d3d9 object.\n");
    if (!d3d9) goto fail;

    hwnd1 = CreateWindow("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(hwnd1 != NULL, "Failed to create a window.\n");
    if (!hwnd1) goto fail;

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd1;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd1,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device1);
    IDirect3D9_Release(d3d9);
    d3d9 = NULL;
    if (FAILED(hr)) {
        skip("Failed to create a device\n");
        goto fail;
    }

    d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
    ok(d3d9 != NULL, "Failed to create a d3d9 object.\n");
    if (!d3d9) goto fail;

    hwnd2 = CreateWindow("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(hwnd2 != NULL, "Failed to create a window.\n");
    if (!hwnd2) goto fail;

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = hwnd2;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd2,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device2);
    ok(SUCCEEDED(hr), "Failed to create a device, hr %#x\n", hr);
    IDirect3D9_Release(d3d9);
    d3d9 = NULL;
    if (FAILED(hr)) goto fail;

fail:
    if (d3d9) IDirect3D9_Release(d3d9);
    if (device1)
    {
        refcount = IDirect3DDevice9_Release(device1);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (device2)
    {
        refcount = IDirect3DDevice9_Release(device2);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (hwnd1) DestroyWindow(hwnd1);
    if (hwnd2) DestroyWindow(hwnd2);
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
            todo_wine ok( 0, "Received unexpected message %#x for window %p.\n", message, hwnd);
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

    p->dummy_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    p->running_in_foreground = SetForegroundWindow(p->dummy_window);

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessage(&msg);
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
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
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
        {0,                     0},
    };

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
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

    flush_events();

    expect_messages = messages;

    device = create_device(d3d9, device_window, focus_window, FALSE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    device = create_device(d3d9, focus_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device = create_device(d3d9, device_window, focus_window, FALSE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    proc = SetWindowLongPtrA(focus_window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);

done:
    filter_messages = NULL;
    IDirect3D9_Release(d3d9);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_wndproc_windowed(void)
{
    struct wndproc_thread_param thread_params;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    HANDLE thread;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEvent(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
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

    device = create_device(d3d9, device_window, focus_window, TRUE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    filter_messages = device_window;

    device = create_device(d3d9, focus_window, focus_window, TRUE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device = create_device(d3d9, device_window, focus_window, TRUE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    filter_messages = NULL;
    IDirect3D9_Release(d3d9);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_reset_fullscreen(void)
{
    WNDCLASSEX wc = {0};
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d = NULL;
    ATOM atom;
    static const struct message messages[] =
    {
        {WM_ACTIVATEAPP,    FOCUS_WINDOW},
        {0,                     0},
    };

    d3d = pDirect3DCreate9(D3D_SDK_VERSION);
    ok(d3d != NULL, "Failed to create an IDirect3D object.\n");
    expect_messages = messages;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "test_reset_fullscreen";

    atom = RegisterClassEx(&wc);
    ok(atom, "Failed to register a new window class. GetLastError:%d\n", GetLastError());

    device_window = focus_window = CreateWindowEx(0, wc.lpszClassName, "Test Reset Fullscreen", 0, 0, 0, screen_width, screen_height, NULL, NULL, NULL, NULL);
    ok(device_window != NULL, "Failed to create a window. GetLastError:%d\n", GetLastError());

    /*
     * Create a device in windowed mode.
     * Since the device is windowed and we haven't called any methods that
     * could show the window (such as ShowWindow or SetWindowPos) yet,
     * WM_ACTIVATEAPP will not have been sent.
     */
    device = create_device(d3d, device_window, focus_window, TRUE);
    if (!device)
    {
        skip("Unable to create device.  Skipping test.\n");
        goto cleanup;
    }

    /*
     * Switch to fullscreen mode.
     * This will force the window to be shown and will cause the WM_ACTIVATEAPP
     * message to be sent.
     */
    ok(SUCCEEDED(reset_device(device, device_window, FALSE)), "Failed to reset device.\n");

    flush_events();
    ok(expect_messages->message == 0, "Expected to receive message %#x.\n", expect_messages->message);
    expect_messages = NULL;

cleanup:
    if (device) IDirect3DDevice9_Release(device);
    if (d3d) IDirect3D9_Release(d3d);
    DestroyWindow(device_window);
    device_window = focus_window = NULL;
    UnregisterClass(wc.lpszClassName, GetModuleHandle(NULL));
}


static inline void set_fpu_cw(WORD cw)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D9_TEST_SET_FPU_CW 1
    __asm__ volatile ("fnclex");
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D9_TEST_SET_FPU_CW 1
    __asm fnclex;
    __asm fldcw cw;
#endif
}

static inline WORD get_fpu_cw(void)
{
    WORD cw = 0;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D9_TEST_GET_FPU_CW 1
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D9_TEST_GET_FPU_CW 1
    __asm fnstcw cw;
#endif
    return cw;
}

static void test_fpu_setup(void)
{
#if defined(D3D9_TEST_SET_FPU_CW) && defined(D3D9_TEST_GET_FPU_CW)
    D3DPRESENT_PARAMETERS present_parameters;
    IDirect3DDevice9 *device;
    HWND window = NULL;
    IDirect3D9 *d3d9;
    HRESULT hr;
    WORD cw;

    d3d9 = pDirect3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a d3d9 object.\n");
    if (!d3d9) return;

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_CAPTION, 0, 0, screen_width, screen_height, 0, 0, 0, 0);
    ok(!!window, "Failed to create a window.\n");
    if (!window) goto done;

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);
    if (FAILED(hr))
    {
        skip("Failed to create a device, hr %#x.\n", hr);
        set_fpu_cw(0x37f);
        goto done;
    }

    cw = get_fpu_cw();
    ok(cw == 0x7f, "cw is %#x, expected 0x7f.\n", cw);

    IDirect3DDevice9_Release(device);

    cw = get_fpu_cw();
    ok(cw == 0x7f, "cw is %#x, expected 0x7f.\n", cw);
    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_FPU_PRESERVE, &present_parameters, &device);
    ok(SUCCEEDED(hr), "CreateDevice failed, hr %#x.\n", hr);

    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);
    set_fpu_cw(0x37f);

    IDirect3DDevice9_Release(device);

done:
    if (window) DestroyWindow(window);
    if (d3d9) IDirect3D9_Release(d3d9);
#endif
}

static void test_window_style(void)
{
    RECT focus_rect, fullscreen_rect, r;
    LONG device_style, device_exstyle;
    LONG focus_style, focus_exstyle;
    LONG style, expected_style;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HRESULT hr;
    ULONG ref;


    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);

    device_style = GetWindowLongA(device_window, GWL_STYLE);
    device_exstyle = GetWindowLongA(device_window, GWL_EXSTYLE);
    focus_style = GetWindowLongA(focus_window, GWL_STYLE);
    focus_exstyle = GetWindowLongA(focus_window, GWL_EXSTYLE);

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(focus_window, &focus_rect);

    device = create_device(d3d9, device_window, focus_window, FALSE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D9_Release(d3d9);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
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
    IDirect3DSurface9 *cursor;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
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

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping cursor tests.\n");
        return;
    }

    wc.lpfnWndProc = test_cursor_proc;
    wc.lpszClassName = "d3d9_test_cursor_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    window = CreateWindow("d3d9_test_cursor_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 320, 240, NULL, NULL, NULL, NULL);
    ShowWindow(window, SW_SHOW);

    device = create_device(d3d9, window, window, TRUE);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &cursor, NULL);
    ok(SUCCEEDED(hr), "Failed to create cursor surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
    ok(SUCCEEDED(hr), "Failed to set cursor properties, hr %#x.\n", hr);
    IDirect3DSurface9_Release(cursor);
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(!ret, "Failed to show cursor, hr %#x.\n", ret);

    flush_events();
    expect_pos = points;

    ret = SetCursorPos(50, 50);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 75, 75, 0);
    flush_events();
    /* SetCursorPosition() eats duplicates. */
    IDirect3DDevice9_SetCursorPosition(device, 75, 75, 0);
    flush_events();

    ret = SetCursorPos(100, 100);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    /* Even if the position was set with SetCursorPos(). */
    IDirect3DDevice9_SetCursorPosition(device, 100, 100, 0);
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 125, 125, 0);
    flush_events();
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    IDirect3DDevice9_SetCursorPosition(device, 125, 125, 0);
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 150, 150, 0);
    flush_events();
    /* SetCursorPos() doesn't. */
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    ok(!expect_pos->x && !expect_pos->y, "Didn't receive MOUSEMOVE %u (%d, %d).\n",
       (unsigned)(expect_pos - points), expect_pos->x, expect_pos->y);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    DestroyWindow(window);
    UnregisterClassA("d3d9_test_cursor_wc", GetModuleHandleA(NULL));
    if (d3d9)
        IDirect3D9_Release(d3d9);
}

static void test_mode_change(void)
{
    RECT fullscreen_rect, focus_rect, r;
    IDirect3DSurface9 *backbuffer;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    IDirect3D9 *d3d9;
    DEVMODEW devmode;
    UINT refcount;
    HRESULT hr;
    DWORD ret;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping mode change tests.\n");
        return;
    }

    focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(focus_window, &focus_rect);

    device = create_device(d3d9, device_window, focus_window, FALSE);
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

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(backbuffer, &desc);
    ok(SUCCEEDED(hr), "Failed to get backbuffer desc, hr %#x.\n", hr);
    ok(desc.Width == screen_width, "Got unexpected backbuffer width %u.\n", desc.Width);
    ok(desc.Height == screen_height, "Got unexpected backbuffer height %u.\n", desc.Height);
    IDirect3DSurface9_Release(backbuffer);

    refcount = IDirect3DDevice9_Release(device);
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
    if (d3d9)
        IDirect3D9_Release(d3d9);

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
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, screen_width / 2, screen_height / 2, 0, 0, 0, 0);

    SetRect(&fullscreen_rect, 0, 0, screen_width, screen_height);
    GetWindowRect(device_window, &device_rect);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    device = create_device(d3d9, NULL, focus_window, FALSE);
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

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_reset_resources(void)
{
    IDirect3DSurface9 *surface, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128,
            D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create render target texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface %u, hr %#x.\n", i, hr);
        IDirect3DTexture9_Release(texture);
        hr = IDirect3DDevice9_SetRenderTarget(device, i, surface);
        ok(SUCCEEDED(hr), "Failed to set render target surface %u, hr %#x.\n", i, hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = reset_device(device, device_window, TRUE);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &surface);
        ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_set_rt_vp_scissor(void)
{
    IDirect3DStateBlock9 *stateblock;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    UINT refcount;
    HWND window;
    HRESULT hr;
    RECT rect;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == screen_width, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == screen_height, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == screen_width && rect.bottom == screen_height,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);
    IDirect3DStateBlock9_Release(stateblock);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 128 && rect.bottom == 128,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    vp.X = 10;
    vp.Y = 20;
    vp.Width = 30;
    vp.Height = 40;
    vp.MinZ = 0.25f;
    vp.MaxZ = 0.75f;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    SetRect(&rect, 50, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 128 && rect.bottom == 128,
            "Got unexpected scissor rect {%d, %d, %d, %d}.\n",
            rect.left, rect.top, rect.right, rect.bottom);

    IDirect3DSurface9_Release(rt);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_get_container(void)
{
    IDirect3DVolumeTexture9 *texture = NULL;
    IDirect3DVolume9 *volume = NULL;
    IDirect3DDevice9 *device;
    IUnknown *container;
    IDirect3D9 *d3d9;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No volume texture support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 128, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, 0);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    ok(!!volume, "Got unexpected volume %p.\n", volume);

    /* These should work... */
    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DResource9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DBaseTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DVolumeTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DVolume9, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_resource(void)
{
    IDirect3DVolumeTexture9 *texture;
    IDirect3DResource9 *resource;
    IDirect3DVolume9 *volume;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No volume texture support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 128, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, 0);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    IDirect3DVolumeTexture9_Release(texture);

    hr = IDirect3DVolume9_QueryInterface(volume, &IID_IDirect3DResource9, (void **)&resource);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);

    IDirect3DVolume9_Release(volume);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_vb_lock_flags(void)
{
    static const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT win7_result;
    }
    test_data[] =
    {
        {D3DLOCK_READONLY,                          "D3DLOCK_READONLY",                         D3D_OK            },
        {D3DLOCK_DISCARD,                           "D3DLOCK_DISCARD",                          D3D_OK            },
        {D3DLOCK_NOOVERWRITE,                       "D3DLOCK_NOOVERWRITE",                      D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD,     "D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD",    D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY,    "D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY",   D3D_OK            },
        {D3DLOCK_READONLY | D3DLOCK_DISCARD,        "D3DLOCK_READONLY | D3DLOCK_DISCARD",       D3DERR_INVALIDCALL},
        /* Completely bogus flags aren't an error. */
        {0xdeadbeef,                                "0xdeadbeef",                               D3DERR_INVALIDCALL},
    };
    IDirect3DVertexBuffer9 *buffer;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 1024, D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); ++i)
    {
        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, test_data[i].flags);
        /* Windows XP always returns D3D_OK even with flags that don't make
         * sense. Windows 7 returns an error. At least one game (Shaiya)
         * depends on the Windows XP result, so mark the Windows 7 behavior as
         * broken. */
        ok(hr == D3D_OK || broken(hr == test_data[i].win7_result), "Got unexpected hr %#x for %s.\n",
                hr, test_data[i].debug_string);
        if (SUCCEEDED(hr))
        {
            ok(!!data, "Got unexpected data %p.\n", data);
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
        }
    }

    IDirect3DVertexBuffer9_Release(buffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static const char *debug_d3dpool(D3DPOOL pool)
{
    switch (pool)
    {
        case D3DPOOL_DEFAULT:
            return "D3DPOOL_DEFAULT";
        case D3DPOOL_SYSTEMMEM:
            return "D3DPOOL_SYSTEMMEM";
        case D3DPOOL_SCRATCH:
            return "D3DPOOL_SCRATCH";
        case D3DPOOL_MANAGED:
            return "D3DPOOL_MANAGED";
        default:
            return "unknown pool";
    }
}

static void test_vertex_buffer_alignment(void)
{
    static const D3DPOOL pools[] = {D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEM, D3DPOOL_SCRATCH, D3DPOOL_MANAGED};
    static const DWORD sizes[] = {1, 4, 16, 17, 32, 33, 64, 65, 1024, 1025, 1048576, 1048577};
    IDirect3DVertexBuffer9 *buffer = NULL;
    const unsigned int align = 16;
    IDirect3DDevice9 *device;
    unsigned int i, j;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < (sizeof(sizes) / sizeof(*sizes)); ++i)
    {
        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); ++j)
        {
            hr = IDirect3DDevice9_CreateVertexBuffer(device, sizes[i], 0, 0, pools[j], &buffer, NULL);
            if (pools[j] == D3DPOOL_SCRATCH)
                ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x trying to create a D3DPOOL_SCRATCH buffer.\n", hr);
            else
                ok(SUCCEEDED(hr), "Failed to create vertex buffer in pool %s with size %u, hr %#x.\n",
                        debug_d3dpool(pools[j]), sizes[i], hr);
            if (FAILED(hr))
                continue;

            hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, 0);
            ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
            ok(!((DWORD_PTR)data & (align - 1)),
                    "Vertex buffer start address %p is not %u byte aligned (size %u, pool %s).\n",
                    data, align, sizes[i], debug_d3dpool(pools[j]));
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
            IDirect3DVertexBuffer9_Release(buffer);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_query_support(void)
{
    static const D3DQUERYTYPE queries[] =
    {
        D3DQUERYTYPE_VCACHE,
        D3DQUERYTYPE_RESOURCEMANAGER,
        D3DQUERYTYPE_VERTEXSTATS,
        D3DQUERYTYPE_EVENT,
        D3DQUERYTYPE_OCCLUSION,
        D3DQUERYTYPE_TIMESTAMP,
        D3DQUERYTYPE_TIMESTAMPDISJOINT,
        D3DQUERYTYPE_TIMESTAMPFREQ,
        D3DQUERYTYPE_PIPELINETIMINGS,
        D3DQUERYTYPE_INTERFACETIMINGS,
        D3DQUERYTYPE_VERTEXTIMINGS,
        D3DQUERYTYPE_PIXELTIMINGS,
        D3DQUERYTYPE_BANDWIDTHTIMINGS,
        D3DQUERYTYPE_CACHEUTILIZATION,
    };
    IDirect3DQuery9 *query = NULL;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    ULONG refcount;
    BOOL supported;
    HWND window;
    HRESULT hr;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < sizeof(queries) / sizeof(*queries); ++i)
    {
        hr = IDirect3DDevice9_CreateQuery(device, queries[i], NULL);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, queries[i]);

        supported = hr == D3D_OK;

        hr = IDirect3DDevice9_CreateQuery(device, queries[i], &query);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, queries[i]);

        ok(!supported || query, "Query %#x was claimed to be supported, but can't be created.\n", queries[i]);
        ok(supported || !query, "Query %#x was claimed not to be supported, but can be created.\n", queries[i]);

        if (query)
        {
            IDirect3DQuery9_Release(query);
            query = NULL;
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_occlusion_query_states(void)
{
    static const float point[3] = {0.0, 0.0, 0.0};
    IDirect3DQuery9 *query = NULL;
    unsigned int data_size, i;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BYTE *data;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create d3d9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_OCCLUSION, &query);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    if (!query)
    {
        skip("Occlusion queries are not supported, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    data_size = IDirect3DQuery9_GetDataSize(query);
    data = HeapAlloc(GetProcessHeap(), 0, data_size);

    hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    *((DWORD *)data) = 0x12345678;
    hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_FALSE || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_FALSE || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (hr == D3D_OK)
        ok(!*(DWORD *)data, "Got unexpected query result %u.\n", *(DWORD *)data);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, point, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 500; ++i)
    {
        if ((hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH)) != S_FALSE)
            break;
        Sleep(10);
    }
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DQuery9_GetData(query, data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    HeapFree(GetProcessHeap(), 0, data);
    IDirect3DQuery9_Release(query);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_get_set_vertex_shader(void)
{
    IDirect3DVertexShader9 *current_shader = NULL;
    IDirect3DVertexShader9 *shader = NULL;
    IDirect3DDevice9 *device;
    ULONG refcount, i;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.VertexShaderVersion & 0xffff))
    {
        skip("No vertex shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create shader, hr %#x.\n", hr);
    ok(!!shader, "Got unexpected shader %p.\n", shader);

    /* SetVertexShader() should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);

    /* GetVertexShader() should increase the shader's refcount by one. */
    i = refcount + 1;
    hr = IDirect3DDevice9_GetVertexShader(device, &current_shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to get vertex shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);
    ok(current_shader == shader, "Got unexpected shader %p, expected %p.\n", current_shader, shader);
    IDirect3DVertexShader9_Release(current_shader);

    IDirect3DVertexShader9_Release(shader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_vertex_shader_constant(void)
{
    static const float d[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    static const float c[4] = {0.0, 0.0, 0.0, 0.0};
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD consts;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.VertexShaderVersion & 0xffff))
    {
        skip("No vertex shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    consts = caps.MaxVertexShaderConst;

    /* A simple check that the stuff works at all. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    /* Test corner cases: Write to const MAX - 1, MAX, MAX + 1, and writing 4
     * consts from MAX - 1. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts - 1, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 0, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Constant -1. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_set_pixel_shader(void)
{
    IDirect3DPixelShader9 *current_shader = NULL;
    IDirect3DPixelShader9 *shader = NULL;
    IDirect3DDevice9 *device;
    ULONG refcount, i;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.PixelShaderVersion & 0xffff))
    {
        skip("No pixel shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreatePixelShader(device, simple_ps, &shader);
    ok(SUCCEEDED(hr), "Failed to create shader, hr %#x.\n", hr);
    ok(!!shader, "Got unexpected shader %p.\n", shader);

    /* SetPixelShader() should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);

    /* GetPixelShader() should increase the shader's refcount by one. */
    i = refcount + 1;
    hr = IDirect3DDevice9_GetPixelShader(device, &current_shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to get pixel shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);
    ok(current_shader == shader, "Got unexpected shader %p, expected %p.\n", current_shader, shader);
    IDirect3DPixelShader9_Release(current_shader);

    IDirect3DPixelShader9_Release(shader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_pixel_shader_constant(void)
{
    static const float d[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    static const float c[4] = {0.0, 0.0, 0.0, 0.0};
    IDirect3DDevice9 *device;
    DWORD consts = 0;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.PixelShaderVersion & 0xffff))
    {
        skip("No pixel shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* A simple check that the stuff works at all. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    /* Is there really no max pixel shader constant value??? Test how far I can go. */
    while (SUCCEEDED(IDirect3DDevice9_SetPixelShaderConstantF(device, consts++, c, 1)));
    consts = consts - 1;
    trace("SetPixelShaderConstantF was able to set %u shader constants.\n", consts);

    /* Test corner cases: Write 4 consts from MAX - 1, everything else is
     * pointless given the way the constant limit was determined. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Constant -1. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_wrong_shader(void)
{
    static const DWORD vs_3_0[] =
    {
        0xfffe0300,                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,     /* mov o0, v0           */
        0x0000ffff,                             /* end                  */
    };

#if 0
float4 main(const float4 color : COLOR) : SV_TARGET
{
    float4 o;

    o = color;

    return o;
}
#endif
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac, 0x00000005,
        0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452, 0x00000050, 0x00000000,
        0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
        0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
        0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
        0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    };

    IDirect3DVertexShader9 *vs = NULL;
    IDirect3DPixelShader9 *ps = NULL;
    IDirect3DDevice9 *device;
    IDirect3D9 * d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* These should always fail, regardless of supported shader version. */
    hr = IDirect3DDevice9_CreateVertexShader(device, simple_ps, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, simple_vs, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_4_0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_0, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }
    else
        skip("This GPU supports SM3, skipping unsupported shader test.\n");

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Test the default texture stage state values */
static void test_texture_stage_states(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD value;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    for (i = 0; i < caps.MaxTextureBlendStages; ++i)
    {
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLOROP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_MODULATE),
                "Got unexpected value %#x for D3DTSS_COLOROP, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_COLORARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAOP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_SELECTARG1),
                "Got unexpected value %#x for D3DTSS_ALPHAOP, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_ALPHAARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT00, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT00, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT01, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT01, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT10, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT10, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT11, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT11, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_TEXCOORDINDEX, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == i, "Got unexpected value %#x for D3DTSS_TEXCOORDINDEX, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVLSCALE, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLSCALE, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVLOFFSET, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLOFFSET, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_TEXTURETRANSFORMFLAGS, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTTFF_DISABLE,
                "Got unexpected value %#x for D3DTSS_TEXTURETRANSFORMFLAGS, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_RESULTARG, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_RESULTARG, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_CONSTANT, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_CONSTANT, stage %u.\n", value, i);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_cube_texture_mipmap_gen(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3D9 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D, hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_CUBETEXTURE, D3DFMT_X8R8G8B8);
    IDirect3D9_Release(d3d);
    if (FAILED(hr))
    {
        skip("No cube mipmap generation support, skipping tests.\n");
        return;
    }

    hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 0, (D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP),
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);
}

static void test_cube_texture_levels(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3DSurface9 *surface;
    D3DSURFACE_DESC desc;
    DWORD levels;
    HRESULT hr;
    D3DCAPS9 caps;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (FAILED(IDirect3DDevice9_CreateCubeTexture(device, 64, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL)))
    {
        skip("Failed to create cube texture, skipping tests.\n");
        return;
    }

    levels = IDirect3DCubeTexture9_GetLevelCount(texture);
    if (caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP)
        ok(levels == 7, "Got unexpected levels %u.\n", levels);
    else
        ok(levels == 1, "Got unexpected levels %u.\n", levels);

    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels - 1, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels, &desc);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels + 1, &desc);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_NEGATIVE_Z + 1, 0, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_POSITIVE_X - 1, 0, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DCubeTexture9_Release(texture);
}

static void test_cube_textures(void)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_DEFAULT cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_MANAGED cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_SYSTEMMEM cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
    }
    else
    {
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_DEFAULT cube texture.\n", hr);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_MANAGED cube texture.\n", hr);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_SYSTEMMEM cube texture.\n", hr);
    }
    hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SCRATCH, &texture, NULL);
    ok(hr == D3D_OK, "Failed to create D3DPOOL_SCRATCH cube texture, hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);

    test_cube_texture_mipmap_gen(device);
    test_cube_texture_levels(device);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_mipmap_gen(void)
{
    D3DTEXTUREFILTERTYPE filter_type;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    ULONG refcount;
    unsigned int i;
    DWORD levels;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8)))
    {
        skip("No mipmap generation support, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, (D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP),
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
    ok(filter_type == D3DTEXF_LINEAR /* || broken(filter_type == D3DTEXF_POINT)*/,
            "Got unexpected filter_type %#x.\n", filter_type);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_NONE);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_ANISOTROPIC);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
    ok(filter_type == D3DTEXF_ANISOTROPIC, "Got unexpected filter_type %#x.\n", filter_type);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_LINEAR);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    levels = IDirect3DTexture9_GetLevelCount(texture);
    ok(levels == 1, "Got unexpected levels %u.\n", levels);

    for (i = 0; i < 6 /* 64 = 2 ^ 6 */; ++i)
    {
        surface = NULL;
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, i, &surface);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);
        if (surface)
            IDirect3DSurface9_Release(surface);

        hr = IDirect3DTexture9_GetLevelDesc(texture, i, &desc);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);

        hr = IDirect3DTexture9_LockRect(texture, i, &lr, NULL, 0);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DTexture9_UnlockRect(texture, i);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
        }
    }
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 2, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 6, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    levels = IDirect3DTexture9_GetLevelCount(texture);
    ok(levels == 1, "Got unexpected levels %u.\n", levels);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_filter(void)
{
    static const struct
    {
        DWORD magfilter, minfilter, mipfilter;
        BOOL has_texture;
        HRESULT result;
    }
    tests[] =
    {
        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_NONE,   D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_NONE,   D3DTEXF_POINT,  D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_NONE,   FALSE, D3D_OK                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_POINT,  FALSE, D3D_OK                         },

        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_NONE,   TRUE,  D3D_OK                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_POINT,  TRUE,  D3D_OK                         },

        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_LINEAR, D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_LINEAR, D3DTEXF_POINT,  D3DTEXF_NONE,   TRUE,  E_FAIL                         },
        {D3DTEXF_POINT,  D3DTEXF_LINEAR, D3DTEXF_NONE,   TRUE,  E_FAIL                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_LINEAR, TRUE,  E_FAIL                         },
    };
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    DWORD passes;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)))
    {
        skip("D3DFMT_A32B32G32R32F not supported, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    if (SUCCEEDED(hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)))
    {
        skip("D3DFMT_A32B32G32R32F supports filtering, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 0, 0,
            D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    /* Needed for ValidateDevice(). */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(tests) / sizeof(*tests)); ++i)
    {
        if (tests[i].has_texture)
        {
            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
        }
        else
        {
            hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
        }

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, tests[i].magfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, tests[i].minfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, tests[i].mipfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);

        passes = 0xdeadbeef;
        hr = IDirect3DDevice9_ValidateDevice(device, &passes);
        ok(hr == tests[i].result,
                "Got unexpected hr %#x, expected %#x (mag %#x, min %#x, mip %#x, has_texture %#x).\n",
                hr, tests[i].result, tests[i].magfilter, tests[i].minfilter,
                tests[i].mipfilter, tests[i].has_texture);
        if (SUCCEEDED(hr))
            ok(!!passes, "Got unexpected passes %#x.\n", passes);
        else
            ok(passes == 0xdeadbeef, "Got unexpected passes %#x.\n", passes);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_texture(void)
{
    IDirect3DBaseTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    texture = (IDirect3DBaseTexture9 *)0xdeadbeef;
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetTexture(device, 0, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!texture, "Got unexpected texture %p.\n", texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_lod(void)
{
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DWORD ret;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 3, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    /* SetLOD() is only supported on D3DPOOL_MANAGED textures, but doesn't
     * return a HRESULT, so it can't return a normal error. Instead, the call
     * is simply ignored. */
    ret = IDirect3DTexture9_SetLOD(texture, 0);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_SetLOD(texture, 1);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_SetLOD(texture, 2);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_GetLOD(texture);
    ok(!ret, "Got unexpected ret %u.\n", ret);

    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_get_container(void)
{
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DDevice9 *device;
    IUnknown *container;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    ok(!!surface, "Got unexpected surface %p.\n", surface);

    /* These should work... */
    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DResource9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DBaseTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DSurface9, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_alignment(void)
{
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lr;
    unsigned int i, j;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* Test a sysmem surface because those aren't affected by the hardware's np2 restrictions. */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 5, 5,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(lr.Pitch & 3), "Got misaligned pitch %d.\n", lr.Pitch);
    /* Some applications also depend on the exact pitch, rather than just the
     * alignment. */
    ok(lr.Pitch == 12, "Got unexpected pitch %d.\n", lr.Pitch);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < 5; ++i)
    {
        IDirect3DTexture9 *texture;
        unsigned int level_count;
        D3DSURFACE_DESC desc;
        int expected_pitch;

        hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, 0,
                MAKEFOURCC('D', 'X', 'T', '1' + i), D3DPOOL_MANAGED, &texture, NULL);
        ok(SUCCEEDED(hr) || hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (FAILED(hr))
        {
            skip("DXT%u surfaces are not supported, skipping tests.\n", i + 1);
            continue;
        }

        level_count = IDirect3DBaseTexture9_GetLevelCount(texture);
        for (j = 0; j < level_count; ++j)
        {
            IDirect3DTexture9_GetLevelDesc(texture, j, &desc);
            hr = IDirect3DTexture9_LockRect(texture, j, &lr, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
            hr = IDirect3DTexture9_UnlockRect(texture, j);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

            expected_pitch = ((desc.Width + 3) >> 2) << 3;
            if (i > 0)
                expected_pitch <<= 1;
            ok(lr.Pitch == expected_pitch, "Got unexpected pitch %d for DXT%u level %u (%ux%u), expected %d.\n",
                    lr.Pitch, i + 1, j, desc.Width, desc.Height, expected_pitch);
        }
        IDirect3DTexture9_Release(texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Since the DXT formats are based on 4x4 blocks, locking works slightly
 * different from regular formats. This test verifies we return the correct
 * memory offsets. */
static void test_lockrect_offset(void)
{
    static const struct
    {
        D3DFORMAT format;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_size;
    }
    dxt_formats[] =
    {
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, 8},
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, 16},
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, 16},
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, 16},
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 16},
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       1, 1,  1},
    };
    unsigned int expected_offset, offset, i;
    const RECT rect = {60, 60, 68, 68};
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    int expected_pitch;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < (sizeof(dxt_formats) / sizeof(*dxt_formats)); ++i)
    {
        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, dxt_formats[i].format)))
        {
            skip("Format %s not supported, skipping lockrect offset tests.\n", dxt_formats[i].name);
            continue;
        }

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                dxt_formats[i].format, D3DPOOL_SCRATCH, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        base = locked_rect.pBits;
        expected_pitch = (128 + dxt_formats[i].block_height - 1) / dxt_formats[i].block_width
                * dxt_formats[i].block_size;
        ok(locked_rect.Pitch == expected_pitch, "Got unexpected pitch %d for format %s, expected %d.\n",
                locked_rect.Pitch, dxt_formats[i].name, expected_pitch);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = (rect.top / dxt_formats[i].block_height) * expected_pitch
                + (rect.left / dxt_formats[i].block_width) * dxt_formats[i].block_size;
        ok(offset == expected_offset, "Got unexpected offset %u for format %s, expected %u.\n",
                offset, dxt_formats[i].name, expected_offset);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        IDirect3DSurface9_Release(surface);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_lockrect_invalid(void)
{
    static const struct
    {
        RECT rect;
        HRESULT win7_result;
    }
    test_data[] =
    {
        {{60, 60, 68, 68},      D3D_OK},                /* Valid */
        {{60, 60, 60, 68},      D3DERR_INVALIDCALL},    /* 0 height */
        {{60, 60, 68, 60},      D3DERR_INVALIDCALL},    /* 0 width */
        {{68, 60, 60, 68},      D3DERR_INVALIDCALL},    /* left > right */
        {{60, 68, 68, 60},      D3DERR_INVALIDCALL},    /* top > bottom */
        {{-8, 60,  0, 68},      D3DERR_INVALIDCALL},    /* left < surface */
        {{60, -8, 68,  0},      D3DERR_INVALIDCALL},    /* top < surface */
        {{-16, 60, -8, 68},     D3DERR_INVALIDCALL},    /* right < surface */
        {{60, -16, 68, -8},     D3DERR_INVALIDCALL},    /* bottom < surface */
        {{60, 60, 136, 68},     D3DERR_INVALIDCALL},    /* right > surface */
        {{60, 60, 68, 136},     D3DERR_INVALIDCALL},    /* bottom > surface */
        {{136, 60, 144, 68},    D3DERR_INVALIDCALL},    /* left > surface */
        {{60, 136, 68, 144},    D3DERR_INVALIDCALL},    /* top > surface */
    };
    static const RECT test_rect_2 = {0, 0, 8, 8};
    IDirect3DSurface9 *surface = NULL;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    base = locked_rect.pBits;
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); ++i)
    {
        unsigned int offset, expected_offset;
        const RECT *rect = &test_data[i].rect;

        locked_rect.pBits = (BYTE *)0xdeadbeef;
        locked_rect.Pitch = 0xdeadbeef;

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, rect, 0);
        /* Windows XP accepts invalid locking rectangles, windows 7 rejects
         * them. Some games (C&C3) depend on the XP behavior, mark the Win 7
         * one broken. */
        ok(SUCCEEDED(hr) || broken(hr == test_data[i].win7_result),
                "Failed to lock surface with rect [%d, %d]->[%d, %d], hr %#x.\n",
                rect->left, rect->top, rect->right, rect->bottom, hr);
        if (FAILED(hr))
            continue;

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
        ok(offset == expected_offset,
                "Got unexpected offset %u (expected %u) for rect [%d, %d]->[%d, %d].\n",
                offset, expected_offset, rect->left, rect->top, rect->right, rect->bottom);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface with rect NULL, hr %#x.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_data[0].rect, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, test_data[0].rect.left, test_data[0].rect.top, test_data[0].rect.right, test_data[0].rect.bottom);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_data[0].rect, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, test_data[0].rect.left, test_data[0].rect.top, test_data[0].rect.right, test_data[0].rect.bottom);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_rect_2, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%d, %d]->[%d, %d].\n",
            hr, test_rect_2.left, test_rect_2.top, test_rect_2.right, test_rect_2.bottom);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    IUnknown *ptr;
    HWND window;
    HRESULT hr;
    DWORD size;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9 /* Abuse this tag */,
            device, 0, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9 /* Abuse this tag */,
            device, 5, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9 /* Abuse this tag */,
            device, sizeof(IUnknown *) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)device);
    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9 /* Abuse this tag */,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDirect3DSurface9_FreePrivateData(surface, &IID_IDirect3DSurface9);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount - 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9,
            surface, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface9_SetPrivateData(surface, &IID_IDirect3DSurface9,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr);
    hr = IDirect3DSurface9_GetPrivateData(surface, &IID_IDirect3DSurface9, &ptr, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 2;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ok(ptr == (IUnknown *)device, "Got unexpected ptr %p, expected %p.\n", ptr, device);
    IUnknown_Release(ptr);

    /* Destroying the surface frees the held reference. */
    IDirect3DSurface9_Release(surface);
    expected_refcount = refcount - 3;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_getdc(void)
{
    static const struct
    {
        const char *name;
        D3DFORMAT format;
        BOOL getdc_supported;
    }
    testdata[] =
    {
        {"D3DFMT_A8R8G8B8",    D3DFMT_A8R8G8B8,    TRUE },
        {"D3DFMT_X8R8G8B8",    D3DFMT_X8R8G8B8,    TRUE },
        {"D3DFMT_R5G6B5",      D3DFMT_R5G6B5,      TRUE },
        {"D3DFMT_X1R5G5B5",    D3DFMT_X1R5G5B5,    TRUE },
        {"D3DFMT_A1R5G5B5",    D3DFMT_A1R5G5B5,    TRUE },
        {"D3DFMT_R8G8B8",      D3DFMT_R8G8B8,      TRUE },
        {"D3DFMT_A2R10G10B10", D3DFMT_A2R10G10B10, FALSE}, /* Untested, card on windows didn't support it. */
        {"D3DFMT_V8U8",        D3DFMT_V8U8,        FALSE},
        {"D3DFMT_Q8W8V8U8",    D3DFMT_Q8W8V8U8,    FALSE},
        {"D3DFMT_A8B8G8R8",    D3DFMT_A8B8G8R8,    FALSE},
        {"D3DFMT_X8B8G8R8",    D3DFMT_A8B8G8R8,    FALSE},
        {"D3DFMT_R3G3B2",      D3DFMT_R3G3B2,      FALSE},
        {"D3DFMT_P8",          D3DFMT_P8,          FALSE},
        {"D3DFMT_L8",          D3DFMT_L8,          FALSE},
        {"D3DFMT_A8L8",        D3DFMT_A8L8,        FALSE},
        {"D3DFMT_DXT1",        D3DFMT_DXT1,        FALSE},
        {"D3DFMT_DXT2",        D3DFMT_DXT2,        FALSE},
        {"D3DFMT_DXT3",        D3DFMT_DXT3,        FALSE},
        {"D3DFMT_DXT4",        D3DFMT_DXT4,        FALSE},
        {"D3DFMT_DXT5",        D3DFMT_DXT5,        FALSE},
    };
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    HDC dc;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < (sizeof(testdata) / sizeof(*testdata)); ++i)
    {
        texture = NULL;
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64,
                testdata[i].format, D3DPOOL_SYSTEMMEM, &surface, NULL);
        if (FAILED(hr))
        {
            hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0,
                    testdata[i].format, D3DPOOL_MANAGED, &texture, NULL);
            if (FAILED(hr))
            {
                skip("Failed to create surface for format %s (hr %#x), skipping tests.\n", testdata[i].name, hr);
                continue;
            }
            hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
            ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
        }

        dc = (void *)0x1234;
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        if (testdata[i].getdc_supported)
            ok(SUCCEEDED(hr), "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        else
            ok(FAILED(hr), "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        if (SUCCEEDED(hr))
        {
            hr = IDirect3DSurface9_ReleaseDC(surface, dc);
            ok(hr == D3D_OK, "Failed to release DC, hr %#x.\n", hr);
        }
        else
        {
            ok(dc == (void *)0x1234, "Got unexpected dc %p.\n", dc);
        }

        IDirect3DSurface9_Release(surface);
        if (texture)
            IDirect3DTexture9_Release(texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_dimensions(void)
{
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 0, 1,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_format_null(void)
{
    static const D3DFORMAT D3DFMT_NULL = MAKEFOURCC('N','U','L','L');
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    IDirect3DSurface9 *rt, *ds;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, D3DFMT_NULL);
    if (hr != D3D_OK)
    {
        skip("No D3DFMT_NULL support, skipping test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_NULL);
    ok(hr == D3D_OK, "D3DFMT_NULL should be supported for render target textures, hr %#x.\n", hr);

    hr = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DFMT_NULL, D3DFMT_D24S8);
    ok(SUCCEEDED(hr), "Depth stencil match failed for D3DFMT_NULL, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_NULL,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &rt);
    ok(SUCCEEDED(hr), "Failed to get original render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get original depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, NULL);
    ok(FAILED(hr), "Succeeded in setting render target 0 to NULL, should fail.\n");

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(ds);

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(desc.Width == 128, "Expected width 128, got %u.\n", desc.Width);
    ok(desc.Height == 128, "Expected height 128, got %u.\n", desc.Height);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(locked_rect.Pitch, "Expected non-zero pitch, got %u.\n", locked_rect.Pitch);
    ok(!!locked_rect.pBits, "Expected non-NULL pBits, got %p.\n", locked_rect.pBits);

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 0, D3DUSAGE_RENDERTARGET,
            D3DFMT_NULL, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_double_unlock(void)
{
    static const D3DPOOL pools[] =
    {
        D3DPOOL_DEFAULT,
        D3DPOOL_SCRATCH,
        D3DPOOL_SYSTEMMEM,
    };
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < (sizeof(pools) / sizeof(*pools)); ++i)
    {
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64,
                D3DFMT_X8R8G8B8, pools[i], &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface in pool %#x, hr %#x.\n", pools[i], hr);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);
        hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);

        IDirect3DSurface9_Release(surface);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
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
        {D3DFMT_YUY2,                 "D3DFMT_YUY2", 2, 1, FALSE, FALSE, TRUE },
        {D3DFMT_UYVY,                 "D3DFMT_UYVY", 2, 1, FALSE, FALSE, TRUE },
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
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  TRUE },
        /* Managed offscreen plain surfaces are not supported */
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },

        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },

        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },
    };
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    unsigned int i, j, w, h;
    BOOL surface_only;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL tex_pow2, cube_pow2;
    D3DCAPS9 caps;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    tex_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_POW2);
    if (tex_pow2)
        tex_pow2 = !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
    cube_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2);

    for (i = 0; i < (sizeof(formats) / sizeof(*formats)); ++i)
    {
        BOOL tex_support, cube_support, surface_support, format_known, dynamic_tex_support;

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, formats[i].fmt);
        tex_support = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_CUBETEXTURE, formats[i].fmt);
        cube_support = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_SURFACE, formats[i].fmt);
        surface_support = SUCCEEDED(hr);

        /* Scratch pool in general allows texture creation even if the driver does
         * not support the format. If the format is an extension format that is not
         * known to the runtime, like ATI2N, some driver support is required for
         * this to work.
         *
         * It is also possible that Windows Vista and Windows 7 d3d9 runtimes know
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

                    texture = (IDirect3DTexture9 *)0xdeadbeef;
                    cube_texture = (IDirect3DCubeTexture9 *)0xdeadbeef;
                    surface = (IDirect3DSurface9 *)0xdeadbeef;

                    switch (create_tests[j].rtype)
                    {
                        case D3DRTYPE_TEXTURE:
                            check_null = (IUnknown **)&texture;
                            hr = IDirect3DDevice9_CreateTexture(device, w, h, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &texture, NULL);
                            support = tex_support;
                            pow2 = tex_pow2;
                            break;

                        case D3DRTYPE_CUBETEXTURE:
                            if (w != h)
                                continue;
                            check_null = (IUnknown **)&cube_texture;
                            hr = IDirect3DDevice9_CreateCubeTexture(device, w, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &cube_texture, NULL);
                            support = cube_support;
                            pow2 = cube_pow2;
                            break;

                        case D3DRTYPE_SURFACE:
                            check_null = (IUnknown **)&surface;
                            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, w, h,
                                    formats[i].fmt, create_tests[j].pool, &surface, NULL);
                            support = surface_support;
                            pow2 = FALSE;
                            break;

                        default:
                            check_null = NULL;
                            pow2 = FALSE;
                            support = FALSE;
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

                    /* Wine knows about ATI2N and happily creates a scratch resource even if GL
                     * does not support it. Accept scratch creation of extension formats on
                     * Windows as well if it occurs. We don't really care if e.g. a Windows 7
                     * on an r200 GPU creates scratch ATI2N texture even though the card doesn't
                     * support it. */
                    if (!formats[i].core_fmt && !format_known && FAILED(expect_hr))
                        may_succeed = TRUE;

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

        surface_only = FALSE;
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, formats[i].fmt);
        dynamic_tex_support = SUCCEEDED(hr);
        if (!dynamic_tex_support)
        {
            if (!surface_support)
            {
                skip("Format %s not supported, skipping lockrect offset tests.\n", formats[i].name);
                continue;
            }
            surface_only = TRUE;
        }

        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); ++j)
        {
            switch (pools[j].pool)
            {
                case D3DPOOL_SYSTEMMEM:
                case D3DPOOL_MANAGED:
                    if (surface_only)
                        continue;
                    /* Fall through */
                case D3DPOOL_DEFAULT:
                    if (surface_only)
                    {
                        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                                formats[i].fmt, pools[j].pool, &surface, NULL);
                        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
                    }
                    else
                    {
                        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1,
                                pools[j].pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0,
                                formats[i].fmt, pools[j].pool, &texture, NULL);
                        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
                        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
                        ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
                        IDirect3DTexture9_Release(texture);
                    }
                    break;

                case D3DPOOL_SCRATCH:
                    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                            formats[i].fmt, pools[j].pool, &surface, NULL);
                    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
                    break;

                default:
                    break;
            }

            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
            ok(SUCCEEDED(hr), "Got unexpected hr %#x for format %s, pool %s.\n", hr, formats[i].name, pools[j].name);
            hr = IDirect3DSurface9_UnlockRect(surface);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

            IDirect3DSurface9_Release(surface);
        }

        if (!dynamic_tex_support)
        {
            skip("Dynamic %s textures not supported, skipping mipmap test.\n", formats[i].name);
            continue;
        }

        if (formats[i].block_width == 1 && formats[i].block_height == 1)
            continue;
        if (!formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice9_CreateTexture(device, formats[i].block_width, formats[i].block_height, 2,
                D3DUSAGE_DYNAMIC, formats[i].fmt, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, format %s.\n", hr, formats[i].name);

        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture9_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        rect.left = 0;
        rect.top = 0;
        rect.right = formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1;
        rect.bottom = formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1;
        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture9_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        rect.right = formats[i].block_width;
        rect.bottom = formats[i].block_height;
        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, &rect, 0);
        todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (SUCCEEDED(hr))
            IDirect3DTexture9_UnlockRect(texture, 1);

        IDirect3DTexture9_Release(texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_set_palette(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY pal[256];
    unsigned int i;
    D3DCAPS9 caps;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
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
    hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
    ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
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
        hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
        ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);
    }
    else
    {
        hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
        ok(hr == D3DERR_INVALIDCALL, "SetPaletteEntries returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_swvp_buffer(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;
    IDirect3DVertexBuffer9 *buffer;
    static const unsigned int bufsize = 1024;
    D3DVERTEXBUFFER_DESC desc;
    D3DPRESENT_PARAMETERS present_parameters = {0};
    struct
    {
        float x, y, z;
    } *ptr, *ptr2;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);

    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = screen_width;
    present_parameters.BackBufferHeight = screen_height;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.EnableAutoDepthStencil = FALSE;
    if (FAILED(IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        IDirect3D9_Release(d3d9);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, bufsize * sizeof(*ptr), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
            D3DPOOL_DEFAULT, &buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer9_GetDesc(buffer, &desc);
    ok(SUCCEEDED(hr), "Failed to get desc, hr %#x.\n", hr);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Got pool %u, expected D3DPOOL_DEFAULT\n", desc.Pool);
    ok(desc.Usage == (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY),
            "Got usage %u, expected D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY\n", desc.Usage);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, bufsize * sizeof(*ptr), (void **)&ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock buffer, hr %#x.\n", hr);
    for (i = 0; i < bufsize; i++)
    {
        ptr[i].x = i * 1.0f;
        ptr[i].y = i * 2.0f;
        ptr[i].z = i * 3.0f;
    }
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(*ptr));
    ok(SUCCEEDED(hr), "Failed to set stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, bufsize * sizeof(*ptr2), (void **)&ptr2, D3DLOCK_DISCARD);
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
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer9_Release(buffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_rtpatch(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    IDirect3DVertexBuffer9 *buffer;
    IDirect3DVertexDeclaration9 *decl;
    static const unsigned int bufsize = 16;
    struct
    {
        float x, y, z;
    } *data;
    D3DRECTPATCH_INFO patch;
    static const float num_segs[] = {1.0f, 1.0f, 1.0f, 1.0f};
    UINT handle = 0x1234;
    D3DCAPS9 caps;

    /* Position input, this generates tesselated positions, but do not generate normals
     *  or texture coordinates. The d3d documentation isn't clear on how to do this */
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (caps.DevCaps & D3DDEVCAPS_RTPATCHES)
    {
        /* The draw methods return the same values, but the patch handle support
         * is different on the refrast, which is the only d3d implementation
         * known to support tri/rect patches */
        skip("Device supports patches, skipping unsupported patch test\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(SUCCEEDED(hr), "Failed to create vertex declaration, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(SUCCEEDED(hr), "Failed to set vertex declaration, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexBuffer(device, bufsize * sizeof(*data), D3DUSAGE_RTPATCHES, 0,
            D3DPOOL_MANAGED, &buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, (void **)&data, 0);
    ok(SUCCEEDED(hr), "Failed to lock buffer, hr %#x.\n", hr);
    memset(data, 0, bufsize * sizeof(*data));
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(*data));
    ok(SUCCEEDED(hr), "Failed to set stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    patch.StartVertexOffsetWidth = 0;
    patch.StartVertexOffsetHeight = 0;
    patch.Width = 4;
    patch.Height = 4;
    patch.Stride = 4;
    patch.Basis = D3DBASIS_BEZIER;
    patch.Degree = D3DDEGREE_CUBIC;
    hr = IDirect3DDevice9_DrawRectPatch(device, handle, num_segs, NULL);
    ok(SUCCEEDED(hr), "Failed to draw rect patch, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawRectPatch(device, handle, num_segs, &patch);
    ok(SUCCEEDED(hr), "Failed to draw rect patch, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawRectPatch(device, handle, num_segs, NULL);
    ok(SUCCEEDED(hr), "Failed to draw rect patch, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawRectPatch(device, 0, num_segs, NULL);
    ok(SUCCEEDED(hr), "Failed to draw rect patch, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawRectPatch(device, 0, num_segs, &patch);
    ok(SUCCEEDED(hr), "Failed to draw rect patch outside scene, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DeletePatch(device, handle);
    ok(hr == D3DERR_INVALIDCALL, "DeletePatch returned hr %#x.\n", hr);
    hr = IDirect3DDevice9_DeletePatch(device, 0);
    ok(hr == D3DERR_INVALIDCALL, "DeletePatch returned hr %#x.\n", hr);
    hr = IDirect3DDevice9_DeletePatch(device, 0x1235);
    ok(hr == D3DERR_INVALIDCALL, "DeletePatch returned hr %#x.\n", hr);

    IDirect3DVertexDeclaration9_Release(decl);
    IDirect3DVertexBuffer9_Release(buffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_npot_textures(void)
{
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window = NULL;
    HRESULT hr;
    D3DCAPS9 caps;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
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

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
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

            hr = IDirect3DDevice9_CreateTexture(device, 10, 10, levels, 0, D3DFMT_X8R8G8B8,
                    pools[i].pool, &texture, NULL);
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
                IDirect3DTexture9_Release(texture);
        }

        hr = IDirect3DDevice9_CreateCubeTexture(device, 3, 1, 0, D3DFMT_X8R8G8B8, pools[i].pool,
                &cube_texture, NULL);
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
            IDirect3DCubeTexture9_Release(cube_texture);

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 2, 2, 3, 1, 0, D3DFMT_X8R8G8B8, pools[i].pool,
                &volume_texture, NULL);
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
            IDirect3DVolumeTexture9_Release(volume_texture);
    }

done:
    if (device)
    {
        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);

}

static void test_vidmem_accounting(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr = D3D_OK;
    IDirect3DTexture9 *textures[20];
    unsigned int i;
    UINT vidmem_start, vidmem_end, diff;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    vidmem_start = IDirect3DDevice9_GetAvailableTextureMem(device);
    memset(textures, 0, sizeof(textures));
    for (i = 0; i < sizeof(textures) / sizeof(*textures) && SUCCEEDED(hr); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &textures[i], NULL);
        /* D3DERR_OUTOFVIDEOMEMORY is returned when the card runs out of video memory
         * E_FAIL is returned on address space or system memory exhaustion */
        ok(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY,
                "Failed to create texture, hr %#x.\n", hr);
    }
    vidmem_end = IDirect3DDevice9_GetAvailableTextureMem(device);

    ok(vidmem_start > vidmem_end, "Expected available texture memory to decrease during texture creation.\n");
    diff = vidmem_start - vidmem_end;
    ok(diff > 1024 * 1024 * 2 * i, "Expected a video memory difference of at least %u MB, got %u MB.\n",
            2 * i, diff / 1024 / 1024);

    for (i = 0; i < sizeof(textures) / sizeof(*textures); i++)
    {
        if (textures[i])
            IDirect3DTexture9_Release(textures[i]);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_locking(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture9 *texture;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS9 caps;
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

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("Volume textures not supported, skipping test.\n");
        goto out;
    }

    for (i = 0; i < sizeof(tests) / sizeof(*tests); i++)
    {
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 1, tests[i].usage,
                D3DFMT_A8R8G8B8, tests[i].pool, &texture, NULL);
        ok(hr == tests[i].create_hr, "Creating volume texture pool=%u, usage=%#x returned %#x, expected %#x.\n",
                tests[i].pool, tests[i].usage, hr, tests[i].create_hr);
        if (FAILED(hr))
            continue;

        locked_box.pBits = (void *)0xdeadbeef;
        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
        ok(hr == tests[i].lock_hr, "Lock returned %#x, expected %#x.\n", hr, tests[i].lock_hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        else
        {
            ok (locked_box.pBits == NULL, "Failed lock set pBits = %p, expected NULL.\n", locked_box.pBits);
        }
        IDirect3DVolumeTexture9_Release(texture);
    }

out:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_update_volumetexture(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture9 *src, *dst;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS9 caps;
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

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
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

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, src_usage,
                D3DFMT_A8R8G8B8, tests[i].src_pool, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, dst_usage,
                D3DFMT_A8R8G8B8, tests[i].dst_pool, &dst, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        hr = IDirect3DVolumeTexture9_LockBox(src, 0, &locked_box, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        *((DWORD *)locked_box.pBits) = 0x11223344;
        hr = IDirect3DVolumeTexture9_UnlockBox(src, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)src, (IDirect3DBaseTexture9 *)dst);
        ok(hr == tests[i].hr, "UpdateTexture returned %#x, expected %#x, src pool %x, dst pool %u.\n",
                hr, tests[i].hr, tests[i].src_pool, tests[i].dst_pool);

        if (SUCCEEDED(hr))
        {
            DWORD content = *((DWORD *)locked_box.pBits);
            hr = IDirect3DVolumeTexture9_LockBox(dst, 0, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
            ok(content == 0x11223344, "Dest texture contained %#x, expected 0x11223344.\n", content);
            hr = IDirect3DVolumeTexture9_UnlockBox(dst, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        IDirect3DVolumeTexture9_Release(src);
        IDirect3DVolumeTexture9_Release(dst);
    }

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
    {
        skip("Mipmapped volume maps not supported.\n");
        goto out;
    }

    for (i = 0; i < sizeof(tests2) / sizeof(*tests2); i++)
    {
        hr = IDirect3DDevice9_CreateVolumeTexture(device,
                tests2[i].src_size, tests2[i].src_size, tests2[i].src_size,
                tests2[i].src_lvl, 0, tests2[i].src_fmt, D3DPOOL_SYSTEMMEM, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);
        hr = IDirect3DDevice9_CreateVolumeTexture(device,
                tests2[i].dst_size, tests2[i].dst_size, tests2[i].dst_size,
                tests2[i].dst_lvl, 0, tests2[i].dst_fmt, D3DPOOL_DEFAULT, &dst, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);

        hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)src, (IDirect3DBaseTexture9 *)dst);
        if (FAILED(hr))
            todo_wine ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u.\n", hr, i);
        else
            ok(SUCCEEDED(hr), "Failed to update texture, hr %#x, case %u.\n", hr, i);

        IDirect3DVolumeTexture9_Release(src);
        IDirect3DVolumeTexture9_Release(dst);
    }

    /* As far as I can see, UpdateTexture on non-matching texture behaves like a memcpy. The raw data
     * stays the same in a format change, a 2x2x1 texture is copied into the first row of a 4x4x1 texture,
     * etc. I could not get it to segfault, but the nonexistent 5th pixel of a 2x2x1 texture is copied into
     * pixel 1x2x1 of a 4x4x1 texture, demonstrating a read beyond the texture's end. I suspect any bad
     * memory access is silently ignored by the runtime, in the kernel or on the GPU.
     *
     * I'm not adding tests for this behavior until an application needs it. */

out:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_create_rt_ds_fail(void)
{
    IDirect3DDevice9 *device;
    HWND window;
    HRESULT hr;
    ULONG refcount;
    IDirect3D9 *d3d9;
    IDirect3DSurface9 *surface;

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    /* Output pointer == NULL segfaults on Windows. */

    surface = (IDirect3DSurface9 *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateRenderTarget(device, 4, 4, D3DFMT_D16,
            D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Creating a D16 render target returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface9_Release(surface);

    surface = (IDirect3DSurface9 *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 4, 4, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Creating a A8R8G8B8 depth stencil returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface9_Release(surface);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_blocks(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    D3DCAPS9 caps;
    IDirect3DVolumeTexture9 *texture;
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

    if (!(d3d9 = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create IDirect3D9 object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d9, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2);

    for (i = 0; i < sizeof(formats) / sizeof(*formats); i++)
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_VOLUMETEXTURE, formats[i].fmt);
        support = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
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

                        texture = (IDirect3DVolumeTexture9 *)0xdeadbeef;
                        hr = IDirect3DDevice9_CreateVolumeTexture(device, w, h, d, 1, 0,
                                formats[i].fmt, create_tests[j].pool, &texture, NULL);

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
                            IDirect3DVolumeTexture9_Release(texture);
                    }
                }
            }
        }

        if (!support && !formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 24, 8, 8, 1, 0,
                formats[i].fmt, D3DPOOL_SCRATCH, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        /* Test lockrect offset */
        for (j = 0; j < sizeof(offset_tests) / sizeof(*offset_tests); j++)
        {
            unsigned int bytes_per_pixel;
            bytes_per_pixel = formats[i].block_size / (formats[i].block_width * formats[i].block_height);

            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
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

            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x, j %u.\n", hr, j);

            box.Left = offset_tests[j].x;
            box.Top = offset_tests[j].y;
            box.Front = offset_tests[j].z;
            box.Right = offset_tests[j].x2;
            box.Bottom = offset_tests[j].y2;
            box.Back = offset_tests[j].z2;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
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

            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
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
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        if (formats[i].block_height > 1)
        {
            box.Left = 0;
            box.Top = formats[i].block_height >> 1;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height >> 1;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        /* Test full block lock */
        box.Left = 0;
        box.Top = 0;
        box.Right = formats[i].block_width;
        box.Bottom = formats[i].block_height;
        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        IDirect3DVolumeTexture9_Release(texture);

        /* Test mipmap locks. Don't do this with ATI2N, AMD warns that the runtime
         * does not allocate surfaces smaller than the blocksize properly. */
        if ((formats[i].block_width > 1 || formats[i].block_height > 1) && formats[i].core_fmt)
        {
            hr = IDirect3DDevice9_CreateVolumeTexture(device, formats[i].block_width, formats[i].block_height,
                    2, 2, 0, formats[i].fmt, D3DPOOL_SCRATCH, &texture, NULL);

            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Left = box.Top = box.Front = 0;
            box.Right = formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1;
            box.Back = 1;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, &box, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, &box, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
            if (SUCCEEDED(hr))
                IDirect3DVolumeTexture9_UnlockBox(texture, 1);

            IDirect3DVolumeTexture9_Release(texture);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
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
    IDirect3DVolumeTexture9 *texture = NULL;
    D3DLOCKED_BOX locked_box;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    if (!(d3d = pDirect3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Failed to create D3D object, skipping tests.\n");
        return;
    }

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(d3d, window, window, TRUE)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 2, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    base = locked_box.pBits;
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(*test_data)); ++i)
    {
        unsigned int offset, expected_offset;
        const D3DBOX *box = &test_data[i].box;

        locked_box.pBits = (BYTE *)0xdeadbeef;
        locked_box.RowPitch = 0xdeadbeef;
        locked_box.SlicePitch = 0xdeadbeef;

        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, box, 0);
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

        hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    }

    /* locked_box = NULL throws an exception on Windows */
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_boxt_2, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_boxt_2.Left, test_boxt_2.Top, test_boxt_2.Front,
            test_boxt_2.Right, test_boxt_2.Bottom, test_boxt_2.Back);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    IDirect3DVolumeTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(device)
{
    HMODULE d3d9_handle = LoadLibraryA( "d3d9.dll" );
    WNDCLASS wc = {0};

    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        goto out;
    }

    pDirect3DCreate9 = (void *)GetProcAddress( d3d9_handle, "Direct3DCreate9" );
    ok(pDirect3DCreate9 != NULL, "Failed to get address of Direct3DCreate9\n");
    if (pDirect3DCreate9)
    {
        IDirect3D9 *d3d9 = pDirect3DCreate9( D3D_SDK_VERSION );
        if(!d3d9)
        {
            skip("could not create D3D9 object\n");
            goto out;
        }
        IDirect3D9_Release(d3d9);

        screen_width = GetSystemMetrics(SM_CXSCREEN);
        screen_height = GetSystemMetrics(SM_CYSCREEN);

        test_fpu_setup();
        test_multi_device();
        test_display_formats();
        test_display_modes();
        test_swapchain();
        test_refcount();
        test_mipmap_levels();
        test_checkdevicemultisampletype();
        test_cursor();
        test_cursor_pos();
        test_reset_fullscreen();
        test_reset();
        test_scene();
        test_limits();
        test_depthstenciltest();
        test_get_rt();
        test_draw_indexed();
        test_null_stream();
        test_lights();
        test_set_stream_source();
        test_scissor_size();
        test_wndproc();
        test_wndproc_windowed();
        test_window_style();
        test_mode_change();
        test_device_window_reset();
        test_reset_resources();
        test_set_rt_vp_scissor();
        test_volume_get_container();
        test_volume_resource();
        test_vb_lock_flags();
        test_vertex_buffer_alignment();
        test_query_support();
        test_occlusion_query_states();
        test_get_set_vertex_shader();
        test_vertex_shader_constant();
        test_get_set_pixel_shader();
        test_pixel_shader_constant();
        test_wrong_shader();
        test_texture_stage_states();
        test_cube_textures();
        test_mipmap_gen();
        test_filter();
        test_get_texture();
        test_lod();
        test_surface_get_container();
        test_surface_alignment();
        test_lockrect_offset();
        test_lockrect_invalid();
        test_private_data();
        test_getdc();
        test_surface_dimensions();
        test_surface_format_null();
        test_surface_double_unlock();
        test_surface_blocks();
        test_set_palette();
        test_swvp_buffer();
        test_rtpatch();
        test_npot_textures();
        test_vidmem_accounting();
        test_volume_locking();
        test_update_volumetexture();
        test_create_rt_ds_fail();
        test_volume_blocks();
        test_lockbox_invalid();
    }

out:
    UnregisterClassA("d3d9_test_wc", GetModuleHandleA(NULL));
}
