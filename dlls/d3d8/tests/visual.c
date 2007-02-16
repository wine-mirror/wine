/*
 * Copyright (C) 2005 Henri Verbeet
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

#define COBJMACROS
#include <d3d8.h>
#include <dxerr8.h>
#include "wine/test.h"

static HMODULE d3d8_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    HWND ret;
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d8_test_wc";
    RegisterClass(&wc);

    ret = CreateWindow("d3d8_test_wc", "d3d8_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);
    return ret;
}

static DWORD getPixelColor(IDirect3DDevice8 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DSurface8 *surf;
    IDirect3DTexture8 *tex;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1 /* Levels */, 0 /* usage */, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex);
    if(FAILED(hr) || !tex )  /* This is not a test */
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%s\n", DXGetErrorString8(hr));
        return 0xdeadbeef;
    }
    hr = IDirect3DTexture8_GetSurfaceLevel(tex, 0, &surf);
    if(FAILED(hr) || !tex )  /* This is not a test */
    {
        trace("Can't get surface from texture, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeee;
        goto out;
    }

    hr = IDirect3DDevice8_GetFrontBuffer(device, surf);
    if(FAILED(hr))
    {
        trace("Can't read the front buffer data, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DSurface8_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeec;
        goto out;
    }
    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface8_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%s\n", DXGetErrorString8(hr));
    }

out:
    if(surf) IDirect3DSurface8_Release(surf);
    if(tex) IDirect3DTexture8_Release(tex);
    return ret;
}

static IDirect3DDevice8 *init_d3d8(void)
{
    IDirect3D8 * (__stdcall * d3d8_create)(UINT SDKVersion) = 0;
    IDirect3D8 *d3d8_ptr = 0;
    IDirect3DDevice8 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hr;

    d3d8_create = (void *)GetProcAddress(d3d8_handle, "Direct3DCreate8");
    ok(d3d8_create != NULL, "Failed to get address of Direct3DCreate8\n");
    if (!d3d8_create) return NULL;

    d3d8_ptr = d3d8_create(D3D_SDK_VERSION);
    ok(d3d8_ptr != NULL, "Failed to create IDirect3D8 object\n");
    if (!d3d8_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = FALSE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

    hr = IDirect3D8_CreateDevice(d3d8_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hr == D3D_OK, "IDirect3D_CreateDevice returned: %s\n", DXGetErrorString8(hr));

    return device_ptr;
}

START_TEST(visual)
{
    IDirect3DDevice8 *device_ptr;
    HRESULT hr;
    DWORD color;

    d3d8_handle = LoadLibraryA("d3d8.dll");
    if (!d3d8_handle)
    {
        trace("Could not load d3d8.dll, skipping tests\n");
        return;
    }

    device_ptr = init_d3d8();
    if (!device_ptr) return;

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice8_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice8_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 1, 1);
    if(color !=0x00ff0000)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice8_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice8_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 639, 479);
    if(color != 0x0000ddee)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

cleanup:
    if(device_ptr) IDirect3DDevice8_Release(device_ptr);
}
