/*
 * Copyright 2006-2007 Henri Verbeet
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
    wc.lpfnWndProc = &DefWindowProc;
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
    ok(d3d8_ptr != NULL, "Failed to create IDirect3D8 object\n");
    if (!d3d8_ptr) return NULL;

    IDirect3D8_GetAdapterDisplayMode(d3d8_ptr, D3DADAPTER_DEFAULT, &d3ddm );
    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferFormat = d3ddm.Format;

    hr = IDirect3D8_CreateDevice(d3d8_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hr))
    {
        trace("could not create device, IDirect3D8_CreateDevice returned %#x\n", hr);
        return NULL;
    }

    return device_ptr;
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

    IDirect3DSurface8_Release(surface);
}

static unsigned long getref(IUnknown *iface)
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
    /* Object is NOT beein addrefed */
    ok(ptr == (IUnknown *) device, "Returned interface pointer is %p, expected %p\n", ptr, device);
    ok(ref2 == ref + 2, "Object reference is %d, expected %d. ptr at %p, orig at %p\n", ref2, ref + 2, ptr, device);
    IUnknown_Release(ptr);

    IDirect3DSurface8_Release(surface);

    /* Destroying the surface frees the held reference */
    ref2 = getref((IUnknown *) device);
    /* -1 because the surface was released and held a reference before */
    ok(ref2 == (ref - 1), "Object reference is %d, expected %d\n", ref2, ref - 1);
}

START_TEST(surface)
{
    HMODULE d3d8_handle;
    IDirect3DDevice8 *device_ptr;

    d3d8_handle = LoadLibraryA("d3d8.dll");
    if (!d3d8_handle)
    {
        skip("Could not load d3d8.dll\n");
        return;
    }

    device_ptr = init_d3d8(d3d8_handle);
    if (!device_ptr) return;

    test_surface_get_container(device_ptr);
    test_lockrect_invalid(device_ptr);
    test_private_data(device_ptr);
}
