/*
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2006 Stefan Dösinger(For CodeWeavers)
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
#include <d3d9.h>
#include <dxerr9.h>
#include "wine/test.h"

static IDirect3D9 *(WINAPI *pDirect3DCreate9)(UINT);

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

#define CHECK_CALL(r,c,d,rc) \
    if (SUCCEEDED(r)) {\
        int tmp1 = get_refcount( (IUnknown *)d ); \
        int rc_new = rc; \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    } else {\
        trace("%s failed: %s\n", c, DXGetErrorString9(r)); \
    }
#define CHECK_RELEASE(obj,d,rc) \
    if (obj) { \
        int tmp1, rc_new = rc; \
        IUnknown_Release( obj ); \
        tmp1 = get_refcount( (IUnknown *)d ); \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    }

static void check_mipmap_levels(
    IDirect3DDevice9* device, 
    int width, int height, int count) 
{

    IDirect3DBaseTexture9* texture = NULL;
    HRESULT hr = IDirect3DDevice9_CreateTexture( device, width, height, 0, 0, 
        D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DTexture9**) &texture, NULL );
       
    if (SUCCEEDED(hr)) {
        DWORD levels = IDirect3DBaseTexture9_GetLevelCount(texture);
        ok(levels == count, "Invalid level count. Expected %d got %lu\n", count, levels);
    } else 
        trace("CreateTexture failed: %s\n", DXGetErrorString9(hr));

    if (texture) IUnknown_Release( texture );
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
    hwnd = CreateWindow( "static", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr), "Failed to create IDirect3D9Device (%s)\n", DXGetErrorString9(hr));
    if (FAILED(hr)) goto cleanup;

    check_mipmap_levels(pDevice, 32, 32, 6);
    check_mipmap_levels(pDevice, 256, 1, 9);
    check_mipmap_levels(pDevice, 1, 256, 9);
    check_mipmap_levels(pDevice, 1, 1, 1);

    cleanup:
    if (pD3d)     IUnknown_Release( pD3d );
    if (pDevice)  IUnknown_Release( pDevice );
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
    hwnd = CreateWindow( "static", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.BackBufferCount  = 0;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr), "Failed to create IDirect3D9Device (%s)\n", DXGetErrorString9(hr));
    if (FAILED(hr)) goto cleanup;

    /* Check if the back buffer count was modified */
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 0, &swapchain0);
    ok(SUCCEEDED(hr), "Failed to get the impicit swapchain (%s)\n", DXGetErrorString9(hr));
    if(swapchain0) IDirect3DSwapChain9_Release(swapchain0);

    /* Check if there is a back buffer */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
    ok(backbuffer != NULL, "The back buffer is NULL\n");
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    /* Try to get a nonexistant swapchain */
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "GetSwapChain on an non-existant swapchain returned (%s)\n", DXGetErrorString9(hr));
    ok(swapchainX == NULL, "Swapchain 1 is %p\n", swapchainX);
    if(swapchainX) IDirect3DSwapChain9_Release(swapchainX);

    /* Create a bunch of swapchains */
    d3dpp.BackBufferCount = 0;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain1);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%s)\n", DXGetErrorString9(hr));
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    d3dpp.BackBufferCount  = 1;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain2);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%s)\n", DXGetErrorString9(hr));

    /* Unsupported by wine for now */
    d3dpp.BackBufferCount  = 2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(pDevice, &d3dpp, &swapchain3);
    todo_wine ok(SUCCEEDED(hr), "Failed to create a swapchain (%s)\n", DXGetErrorString9(hr));
    if(SUCCEEDED(hr)) {
        /* Swapchain 3, created with backbuffercount 2 */
        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 0, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 1st back buffer (%s)\n", DXGetErrorString9(hr));
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 1, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 2nd back buffer (%s)\n", DXGetErrorString9(hr));
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 2, 0, &backbuffer);
        ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %s\n", DXGetErrorString9(hr));
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 3, 0, &backbuffer);
        ok(FAILED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);
    }

    /* Check the back buffers of the swapchains */
    /* Swapchain 1, created with backbuffercount 0 */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
    ok(backbuffer != NULL, "The back buffer is NULL (%s)\n", DXGetErrorString9(hr));
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 1, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Swapchain 2 - created with backbuffercount 1 */
    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 0, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
    ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 1, 0, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %s\n", DXGetErrorString9(hr));
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 2, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%s)\n", DXGetErrorString9(hr));
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Try getSwapChain on a manually created swapchain
     * it should fail, apparently GetSwapChain only returns implicit swapchains
     */
    swapchainX = (void *) 0xdeadbeef;
    hr = IDirect3DDevice9_GetSwapChain(pDevice, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "Failed to get the secound swapchain (%s)\n", DXGetErrorString9(hr));
    ok(swapchainX == NULL, "The swapchain pointer is %p\n", swapchainX);
    if(swapchainX && swapchainX != (void *) 0xdeadbeef ) IDirect3DSwapChain9_Release(swapchainX);

    cleanup:
    if(swapchain1) IDirect3DSwapChain9_Release(swapchain1);
    if(swapchain2) IDirect3DSwapChain9_Release(swapchain2);
    if(swapchain3) IDirect3DSwapChain9_Release(swapchain3);
    if(pDevice) IDirect3DDevice9_Release(pDevice);
    if(pD3d) IDirect3DDevice9_Release(pD3d);
    DestroyWindow( hwnd );
}

static void test_refcount(void)
{
    HRESULT                      hr;
    HWND                         hwnd               = NULL;
    IDirect3D9                  *pD3d               = NULL;
    IDirect3DDevice9            *pDevice            = NULL;
    IDirect3DVertexBuffer9      *pVertexBuffer      = NULL;
    IDirect3DIndexBuffer9       *pIndexBuffer       = NULL;
    IDirect3DVertexDeclaration9 *pVertexDeclaration = NULL;
    IDirect3DVertexShader9      *pVertexShader      = NULL;
    IDirect3DPixelShader9       *pPixelShader       = NULL;
    IDirect3DCubeTexture9       *pCubeTexture       = NULL;
    IDirect3DTexture9           *pTexture           = NULL;
    IDirect3DVolumeTexture9     *pVolumeTexture     = NULL;
    IDirect3DSurface9           *pStencilSurface    = NULL;
    IDirect3DSurface9           *pOffscreenSurface  = NULL;
    IDirect3DSurface9           *pRenderTarget      = NULL;
    IDirect3DSurface9           *pTextureLevel      = NULL;
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
    static DWORD simple_vs[] = {0xFFFE0101,             /* vs_1_1               */
        0x0000001F, 0x80000000, 0x900F0000,             /* dcl_position0 v0     */
        0x00000009, 0xC0010000, 0x90E40000, 0xA0E40000, /* dp4 oPos.x, v0, c0   */
        0x00000009, 0xC0020000, 0x90E40000, 0xA0E40001, /* dp4 oPos.y, v0, c1   */
        0x00000009, 0xC0040000, 0x90E40000, 0xA0E40002, /* dp4 oPos.z, v0, c2   */
        0x00000009, 0xC0080000, 0x90E40000, 0xA0E40003, /* dp4 oPos.w, v0, c3   */
        0x0000FFFF};                                    /* END                  */
    static DWORD simple_ps[] = {0xFFFF0101,                                     /* ps_1_1                       */
        0x00000051, 0xA00F0001, 0x3F800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
        0x00000042, 0xB00F0000,                                                 /* tex t0                       */
        0x00000008, 0x800F0000, 0xA0E40001, 0xA0E40000,                         /* dp3 r0, c1, c0               */
        0x00000005, 0x800F0000, 0x90E40000, 0x80E40000,                         /* mul r0, v0, r0               */
        0x00000005, 0x800F0000, 0xB0E40000, 0x80E40000,                         /* mul r0, t0, r0               */
        0x0000FFFF};                                                            /* END                          */


    pD3d = pDirect3DCreate9( D3D_SDK_VERSION );
    ok(pD3d != NULL, "Failed to create IDirect3D9 object\n");
    hwnd = CreateWindow( "static", "d3d9_test", WS_OVERLAPPEDWINDOW, 100, 100, 160, 160, NULL, NULL, NULL, NULL );
    ok(hwnd != NULL, "Failed to create window\n");
    if (!pD3d || !hwnd) goto cleanup;

    IDirect3D9_GetAdapterDisplayMode( pD3d, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D9_CreateDevice( pD3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwnd,
                                  D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice );
    ok(SUCCEEDED(hr), "Failed to create IDirect3D9Device (%s)\n", DXGetErrorString9(hr));
    if (FAILED(hr)) goto cleanup;

    refcount = get_refcount( (IUnknown *)pDevice );
    ok(refcount == 1, "Invalid device RefCount %d\n", refcount);

    /* Buffers */
    hr = IDirect3DDevice9_CreateIndexBuffer( pDevice, 16, 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &pIndexBuffer, NULL );
    CHECK_CALL( hr, "CreateIndexBuffer", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateVertexBuffer( pDevice, 16, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &pVertexBuffer, NULL );
    CHECK_CALL( hr, "CreateVertexBuffer", pDevice, ++refcount );
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
        /* This should not increment device refcount */
        hr = IDirect3DTexture9_GetSurfaceLevel( pTexture, 1, &pTextureLevel );
        CHECK_CALL( hr, "GetSurfaceLevel", pDevice, refcount );
        /* But should increment texture's refcount */
        CHECK_CALL( hr, "GetSurfaceLevel", pTexture, tmp+1 );
    }
    hr = IDirect3DDevice9_CreateCubeTexture( pDevice, 32, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pCubeTexture, NULL );
    CHECK_CALL( hr, "CreateCubeTexture", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateVolumeTexture( pDevice, 32, 32, 2, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pVolumeTexture, NULL );
    CHECK_CALL( hr, "CreateVolumeTexture", pDevice, ++refcount );
    /* Surfaces */
    hr = IDirect3DDevice9_CreateDepthStencilSurface( pDevice, 32, 32, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &pStencilSurface, NULL );
    CHECK_CALL( hr, "CreateDepthStencilSurface", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface( pDevice, 32, 32, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pOffscreenSurface, NULL );
    CHECK_CALL( hr, "CreateOffscreenPlainSurface", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateRenderTarget( pDevice, 32, 32, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRenderTarget, NULL );
    CHECK_CALL( hr, "CreateRenderTarget", pDevice, ++refcount );
    /* Misc */
    hr = IDirect3DDevice9_CreateStateBlock( pDevice, D3DSBT_ALL, &pStateBlock );
    CHECK_CALL( hr, "CreateStateBlock", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateAdditionalSwapChain( pDevice, &d3dpp, &pSwapChain );
    CHECK_CALL( hr, "CreateAdditionalSwapChain", pDevice, ++refcount );
    hr = IDirect3DDevice9_CreateQuery( pDevice, D3DQUERYTYPE_EVENT, &pQuery );
    CHECK_CALL( hr, "CreateQuery", pDevice, ++refcount );

    hr = IDirect3DDevice9_BeginStateBlock( pDevice );
    CHECK_CALL( hr, "BeginStateBlock", pDevice, refcount );
    hr = IDirect3DDevice9_EndStateBlock( pDevice, &pStateBlock1 );
    CHECK_CALL( hr, "EndStateBlock", pDevice, ++refcount );

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
    /* pTextureLevel is holding a reference to the pTexture */
    CHECK_RELEASE(pTexture,             pDevice,   refcount);
    CHECK_RELEASE(pTextureLevel,        pDevice, --refcount);
    CHECK_RELEASE(pCubeTexture,         pDevice, --refcount);
    CHECK_RELEASE(pVolumeTexture,       pDevice, --refcount);
    /* Surfaces */
    CHECK_RELEASE(pStencilSurface,      pDevice, --refcount);
    CHECK_RELEASE(pOffscreenSurface,    pDevice, --refcount);
    CHECK_RELEASE(pRenderTarget,        pDevice, --refcount);
    /* Misc */
    CHECK_RELEASE(pStateBlock,          pDevice, --refcount);
    CHECK_RELEASE(pSwapChain,           pDevice, --refcount);
    CHECK_RELEASE(pQuery,               pDevice, --refcount);
    /* This will destroy device - can not check the refcount here */
    if (pStateBlock1)         IUnknown_Release( pStateBlock1 );

    if (pD3d)                 IUnknown_Release( pD3d );

    DestroyWindow( hwnd );
}

START_TEST(device)
{
    HMODULE d3d9_handle = LoadLibraryA( "d3d9.dll" );

    pDirect3DCreate9 = (void *)GetProcAddress( d3d9_handle, "Direct3DCreate9" );
    if (pDirect3DCreate9)
    {
        test_swapchain();
        test_refcount();
        test_mipmap_levels();
    }
}
