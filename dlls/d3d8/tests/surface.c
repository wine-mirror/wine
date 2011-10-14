/*
 * Copyright 2006-2007 Henri Verbeet
 * Copyright 2011 Stefan Dösinger for CodeWeavers
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
#include <d3d8.h>
#include "wine/test.h"

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "d3d8_test_wc";
    RegisterClass(&wc);

    return CreateWindow("d3d8_test_wc", "d3d8_test",
            0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static IDirect3DDevice8 *init_d3d8(HMODULE d3d8_handle)
{
    IDirect3D8 * (__stdcall * d3d8_create)(UINT SDKVersion) = 0;
    IDirect3D8 *d3d8_ptr = 0;
    IDirect3DDevice8 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    D3DDISPLAYMODE               d3ddm;
    HRESULT hr;

    d3d8_create = (void *)GetProcAddress(d3d8_handle, "Direct3DCreate8");
    ok(d3d8_create != NULL, "Failed to get address of Direct3DCreate8\n");
    if (!d3d8_create) return NULL;

    d3d8_ptr = d3d8_create(D3D_SDK_VERSION);
    if (!d3d8_ptr)
    {
        skip("could not create D3D8\n");
        return NULL;
    }

    IDirect3D8_GetAdapterDisplayMode(d3d8_ptr, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = d3ddm.Format;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3D8_CreateDevice(d3d8_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hr))
    {
        skip("could not create device, IDirect3D8_CreateDevice returned %#x\n", hr);
        return NULL;
    }

    return device_ptr;
}

/* Test the behaviour of the IDirect3DDevice8::CreateImageSurface method.

Expected behaviour (and also documented in the original DX8 docs) is that the
call returns a surface with the SYSTEMMEM pool type. Games like Max Payne 1
and 2 which use Direct3D8 calls depend on this behaviour.

A short remark in the DX9 docs however states that the pool of the
returned surface object is of type SCRATCH. This is misinformation and results
in screenshots not appearing in the savegame loading menu of both games
mentioned above (engine tries to display a texture from the scratch pool).

This test verifies that the behaviour described in the original D3D8 docs is
the correct one. For more information about this issue, see the MSDN:

D3D9 docs: "Converting to Direct3D 9"
D3D9 reference: "IDirect3DDevice9::CreateOffscreenPlainSurface"
D3D8 reference: "IDirect3DDevice8::CreateImageSurface"
*/

static void test_image_surface_pool(IDirect3DDevice8 *device) {
    IDirect3DSurface8 *surface = 0;
    D3DSURFACE_DESC surf_desc;
    HRESULT hr;

    hr = IDirect3DDevice8_CreateImageSurface(device, 128, 128, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "CreateImageSurface failed (0x%08x)\n", hr);

    hr = IDirect3DSurface8_GetDesc(surface, &surf_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (0x%08x)\n", hr);

    ok((surf_desc.Pool == D3DPOOL_SYSTEMMEM),
        "CreateImageSurface returns surface with unexpected pool type %u (should be SYSTEMMEM = 2)\n", surf_desc.Pool);

    IDirect3DSurface8_Release(surface);
}

static void test_surface_get_container(IDirect3DDevice8 *device_ptr)
{
    IDirect3DTexture8 *texture_ptr = 0;
    IDirect3DSurface8 *surface_ptr = 0;
    void *container_ptr;
    HRESULT hr;

    hr = IDirect3DDevice8_CreateTexture(device_ptr, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture_ptr);
    ok(SUCCEEDED(hr) && texture_ptr != NULL, "CreateTexture returned: hr %#x, texture_ptr %p. "
        "Expected hr %#x, texture_ptr != %p\n", hr, texture_ptr, D3D_OK, NULL);
    if (!texture_ptr || FAILED(hr)) goto cleanup;

    hr = IDirect3DTexture8_GetSurfaceLevel(texture_ptr, 0, &surface_ptr);
    ok(SUCCEEDED(hr) && surface_ptr != NULL, "GetSurfaceLevel returned: hr %#x, surface_ptr %p. "
        "Expected hr %#x, surface_ptr != %p\n", hr, surface_ptr, D3D_OK, NULL);
    if (!surface_ptr || FAILED(hr)) goto cleanup;

    /* These should work... */
    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface8_GetContainer(surface_ptr, &IID_IUnknown, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface8_GetContainer(surface_ptr, &IID_IDirect3DResource8, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface8_GetContainer(surface_ptr, &IID_IDirect3DBaseTexture8, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface8_GetContainer(surface_ptr, &IID_IDirect3DTexture8, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container_ptr to NULL */
    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface8_GetContainer(surface_ptr, &IID_IDirect3DSurface8, &container_ptr);
    ok(hr == E_NOINTERFACE && container_ptr == NULL, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, E_NOINTERFACE, NULL);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

cleanup:
    if (texture_ptr) IDirect3DTexture8_Release(texture_ptr);
    if (surface_ptr) IDirect3DSurface8_Release(surface_ptr);
}

static void test_lockrect_invalid(IDirect3DDevice8 *device)
{
    IDirect3DSurface8 *surface = 0;
    D3DLOCKED_RECT locked_rect;
    unsigned int i;
    BYTE *base;
    HRESULT hr;

    const RECT valid[] = {
        {60, 60, 68, 68},
        {120, 60, 128, 68},
        {60, 120, 68, 128},
    };

    const RECT invalid[] = {
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

    hr = IDirect3DDevice8_CreateImageSurface(device, 128, 128, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "CreateImageSurface failed (0x%08x)\n", hr);

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    base = locked_rect.pBits;

    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    for (i = 0; i < (sizeof(valid) / sizeof(*valid)); ++i)
    {
        unsigned int offset, expected_offset;
        const RECT *rect = &valid[i];

        locked_rect.pBits = (BYTE *)0xdeadbeef;
        locked_rect.Pitch = 0xdeadbeef;

        hr = IDirect3DSurface8_LockRect(surface, &locked_rect, rect, 0);
        ok(SUCCEEDED(hr), "LockRect failed (0x%08x) for rect [%d, %d]->[%d, %d]\n",
                hr, rect->left, rect->top, rect->right, rect->bottom);

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
        ok(offset == expected_offset, "Got offset %u, expected offset %u for rect [%d, %d]->[%d, %d]\n",
                offset, expected_offset, rect->left, rect->top, rect->right, rect->bottom);

        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);
    }

    for (i = 0; i < (sizeof(invalid) / sizeof(*invalid)); ++i)
    {
        const RECT *rect = &invalid[i];

        hr = IDirect3DSurface8_LockRect(surface, &locked_rect, rect, 0);
        ok(hr == D3DERR_INVALIDCALL, "LockRect returned 0x%08x for rect [%d, %d]->[%d, %d]"
                ", expected D3DERR_INVALIDCALL (0x%08x)\n", hr, rect->left, rect->top,
                rect->right, rect->bottom, D3DERR_INVALIDCALL);
    }

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x) for rect NULL\n", hr);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Double LockRect returned 0x%08x for rect NULL\n", hr);
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[0], 0);
    ok(hr == D3D_OK, "LockRect failed (0x%08x) for rect [%d, %d]->[%d, %d]"
            ", expected D3D_OK (0x%08x)\n", hr, valid[0].left, valid[0].top,
            valid[0].right, valid[0].bottom, D3D_OK);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[0], 0);
    ok(hr == D3DERR_INVALIDCALL, "Double LockRect failed (0x%08x) for rect [%d, %d]->[%d, %d]"
            ", expected D3DERR_INVALIDCALL (0x%08x)\n", hr, valid[0].left, valid[0].top,
            valid[0].right, valid[0].bottom,D3DERR_INVALIDCALL);
    hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &valid[1], 0);
    ok(hr == D3DERR_INVALIDCALL, "Double LockRect failed (0x%08x) for rect [%d, %d]->[%d, %d]"
            ", expected D3DERR_INVALIDCALL (0x%08x)\n", hr, valid[1].left, valid[1].top,
            valid[1].right, valid[1].bottom, D3DERR_INVALIDCALL);
    hr = IDirect3DSurface8_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    IDirect3DSurface8_Release(surface);
}

static ULONG getref(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static void test_private_data(IDirect3DDevice8 *device)
{
    HRESULT hr;
    IDirect3DSurface8 *surface;
    ULONG ref, ref2;
    IUnknown *ptr;
    DWORD size = sizeof(IUnknown *);

    hr = IDirect3DDevice8_CreateImageSurface(device, 4, 4, D3DFMT_A8R8G8B8, &surface);
    ok(SUCCEEDED(hr), "CreateImageSurface failed (0x%08x)\n", hr);
    if(!surface)
    {
        return;
    }

    /* This fails */
    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8 /* Abuse this tag */, device, 0, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8 /* Abuse this tag */, device, 5, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8 /* Abuse this tag */, device, sizeof(IUnknown *) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);

    ref = getref((IUnknown *) device);
    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8 /* Abuse this tag */, device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) device);
    ok(ref2 == ref + 1, "Object reference is %d, expected %d\n", ref2, ref + 1);
    hr = IDirect3DSurface8_FreePrivateData(surface, &IID_IDirect3DSurface8);
    ok(hr == D3D_OK, "IDirect3DSurface8_FreePrivateData returned %08x\n", hr);
    ref2 = getref((IUnknown *) device);
    ok(ref2 == ref, "Object reference is %d, expected %d\n", ref2, ref);

    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8, device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8, surface, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) device);
    ok(ref2 == ref, "Object reference is %d, expected %d\n", ref2, ref);

    hr = IDirect3DSurface8_SetPrivateData(surface, &IID_IDirect3DSurface8, device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "IDirect3DSurface8_SetPrivateData failed with %08x\n", hr);
    hr = IDirect3DSurface8_GetPrivateData(surface, &IID_IDirect3DSurface8, &ptr, &size);
    ok(hr == D3D_OK, "IDirect3DSurface8_GetPrivateData failed with %08x\n", hr);
    ref2 = getref((IUnknown *) device);
    /* Object is NOT being addrefed */
    ok(ptr == (IUnknown *) device, "Returned interface pointer is %p, expected %p\n", ptr, device);
    ok(ref2 == ref + 2, "Object reference is %d, expected %d. ptr at %p, orig at %p\n", ref2, ref + 2, ptr, device);
    IUnknown_Release(ptr);

    IDirect3DSurface8_Release(surface);

    /* Destroying the surface frees the held reference */
    ref2 = getref((IUnknown *) device);
    /* -1 because the surface was released and held a reference before */
    ok(ref2 == (ref - 1), "Object reference is %d, expected %d\n", ref2, ref - 1);
}

static void test_surface_dimensions(IDirect3DDevice8 *device)
{
    IDirect3DSurface8 *surface;
    HRESULT hr;

    hr = IDirect3DDevice8_CreateImageSurface(device, 0, 1, D3DFMT_A8R8G8B8, &surface);
    ok(hr == D3DERR_INVALIDCALL, "CreateOffscreenPlainSurface returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    if (SUCCEEDED(hr)) IDirect3DSurface8_Release(surface);

    hr = IDirect3DDevice8_CreateImageSurface(device, 1, 0, D3DFMT_A8R8G8B8, &surface);
    ok(hr == D3DERR_INVALIDCALL, "CreateOffscreenPlainSurface returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    if (SUCCEEDED(hr)) IDirect3DSurface8_Release(surface);
}

static void test_surface_format_null(IDirect3DDevice8 *device)
{
    static const D3DFORMAT D3DFMT_NULL = MAKEFOURCC('N','U','L','L');
    IDirect3DTexture8 *texture;
    IDirect3DSurface8 *surface;
    IDirect3DSurface8 *rt, *ds;
    D3DLOCKED_RECT locked_rect;
    D3DSURFACE_DESC desc;
    IDirect3D8 *d3d;
    HRESULT hr;

    IDirect3DDevice8_GetDirect3D(device, &d3d);

    hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET,  D3DRTYPE_SURFACE, D3DFMT_NULL);
    if (hr != D3D_OK)
    {
        skip("No D3DFMT_NULL support, skipping test.\n");
        IDirect3D8_Release(d3d);
        return;
    }

    hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET,  D3DRTYPE_TEXTURE, D3DFMT_NULL);
    ok(hr == D3D_OK, "D3DFMT_NULL should be supported for render target textures, hr %#x.\n", hr);

    hr = IDirect3D8_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DFMT_NULL, D3DFMT_D24S8);
    ok(SUCCEEDED(hr), "Depth stencil match failed for D3DFMT_NULL, hr %#x.\n", hr);

    IDirect3D8_Release(d3d);

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
}

static void test_surface_double_unlock(IDirect3DDevice8 *device)
{
    static  struct
    {
        D3DPOOL pool;
        const char *name;
    }
    pools[] =
    {
        { D3DPOOL_DEFAULT,      "D3DPOOL_DEFAULT"   },
        { D3DPOOL_SYSTEMMEM,    "D3DPOOL_SYSTEMMEM" },
    };
    IDirect3DSurface8 *surface;
    unsigned int i;
    HRESULT hr;
    D3DLOCKED_RECT lr;
    IDirect3D8 *d3d;

    hr = IDirect3DDevice8_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_GetDirect3D failed, hr = 0x%08x\n", hr);

    for (i = 0; i < (sizeof(pools) / sizeof(*pools)); i++)
    {
        switch (pools[i].pool)
        {
            case D3DPOOL_DEFAULT:
                hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                        D3DRTYPE_SURFACE, D3DFMT_X8R8G8B8);
                if (FAILED(hr))
                {
                    skip("D3DFMT_X8R8G8B8 render targets not supported, skipping double unlock DEFAULT pool test\n");
                    continue;
                }

                hr = IDirect3DDevice8_CreateRenderTarget(device, 64, 64, D3DFMT_X8R8G8B8,
                        D3DMULTISAMPLE_NONE, TRUE, &surface);
                ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateRenderTarget failed, hr = 0x%08x, pool %s\n",
                        hr, pools[i].name);
                break;

            case D3DPOOL_SYSTEMMEM:
                hr = IDirect3DDevice8_CreateImageSurface(device, 64, 64, D3DFMT_X8R8G8B8, &surface);
                ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateImageSurface failed, hr = 0x%08x, pool %s\n",
                        hr, pools[i].name);
                break;

            default:
                break;
        }

        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Unlock without lock returned 0x%08x, expected 0x%08x, pool %s\n",
                hr, D3DERR_INVALIDCALL, pools[i].name);

        hr = IDirect3DSurface8_LockRect(surface, &lr, NULL, 0);
        ok(SUCCEEDED(hr), "IDirect3DSurface8_LockRect failed, hr = 0x%08x, pool %s\n",
                hr, pools[i].name);
        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed, hr = 0x%08x, pool %s\n",
                hr, pools[i].name);

        hr = IDirect3DSurface8_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Double unlock returned 0x%08x, expected 0x%08x, pool %s\n",
                hr, D3DERR_INVALIDCALL, pools[i].name);

        IDirect3DSurface8_Release(surface);
    }

    IDirect3D8_Release(d3d);
}

static void test_surface_lockrect_blocks(IDirect3DDevice8 *device)
{
    IDirect3DTexture8 *texture;
    IDirect3DSurface8 *surface;
    IDirect3D8 *d3d;
    D3DLOCKED_RECT locked_rect;
    unsigned int i, j;
    HRESULT hr;
    RECT rect;

    const struct
    {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        BOOL broken;
    }
    formats[] =
    {
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, FALSE},
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, FALSE},
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, FALSE},
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, FALSE},
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, FALSE},
        /* ATI2N has 2x2 blocks on all AMD cards and Geforce 7 cards,
         * which doesn't match the format spec. On newer Nvidia cards
         * it has the correct 4x4 block size */
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       4, 4, TRUE},
        /* YUY2 and UYVY are not supported in d3d8, there is no way
         * to use them with this API considering their restrictions */
    };
    const struct
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

    hr = IDirect3DDevice8_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "IDirect3DDevice8_GetDirect3D failed (%08x)\n", hr);

    for (i = 0; i < (sizeof(formats) / sizeof(*formats)); ++i) {
        hr = IDirect3D8_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC,
                D3DRTYPE_TEXTURE, formats[i].fmt);
        if (FAILED(hr))
        {
            skip("Format %s not supported, skipping lockrect offset test\n", formats[i].name);
            continue;
        }

        for (j = 0; j < (sizeof(pools) / sizeof(*pools)); j++)
        {
            hr = IDirect3DDevice8_CreateTexture(device, 128, 128, 1,
                    pools[j].pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0,
                    formats[i].fmt, pools[j].pool, &texture);
            ok(SUCCEEDED(hr), "IDirect3DDevice8_CreateTexture failed (%08x)\n", hr);
            hr = IDirect3DTexture8_GetSurfaceLevel(texture, 0, &surface);
            ok(SUCCEEDED(hr), "IDirect3DTexture8_GetSurfaceLevel failed (%08x)\n", hr);
            IDirect3DTexture8_Release(texture);

            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(!SUCCEEDED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed (%08x)\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(!SUCCEEDED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed (%08x)\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(!SUCCEEDED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed (%08x)\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
                ok(!SUCCEEDED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface8_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed (%08x)\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirect3DSurface8_LockRect(surface, &locked_rect, &rect, 0);
            ok(hr == D3D_OK, "Full block lock returned %08x, expected %08x, format %s, pool %s\n",
                    hr, D3D_OK, formats[i].name, pools[j].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DSurface8_UnlockRect(surface);
                ok(SUCCEEDED(hr), "IDirect3DSurface8_UnlockRect failed (%08x)\n", hr);
            }

            IDirect3DSurface8_Release(surface);
        }
    }
    IDirect3D8_Release(d3d);
}

START_TEST(surface)
{
    HMODULE d3d8_handle;
    IDirect3DDevice8 *device_ptr;
    ULONG refcount;

    d3d8_handle = LoadLibraryA("d3d8.dll");
    if (!d3d8_handle)
    {
        skip("Could not load d3d8.dll\n");
        return;
    }

    device_ptr = init_d3d8(d3d8_handle);
    if (!device_ptr) return;

    test_image_surface_pool(device_ptr);
    test_surface_get_container(device_ptr);
    test_lockrect_invalid(device_ptr);
    test_private_data(device_ptr);
    test_surface_dimensions(device_ptr);
    test_surface_format_null(device_ptr);
    test_surface_double_unlock(device_ptr);
    test_surface_lockrect_blocks(device_ptr);

    refcount = IDirect3DDevice8_Release(device_ptr);
    ok(!refcount, "Device has %u references left\n", refcount);
}
