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

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

static BOOL createObjects()
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
    ok(hr == DD_OK, "IDirectDraw7_SetDisplayMode failed with %08x\n", hr);
    if(FAILED(hr)) goto err;

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

static void releaseObjects()
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
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
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
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);

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
    hr = IDirectDrawSurface7_Unlock(surf, &rectToLock);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%08x\n", hr);
    }

out:
    IDirectDrawSurface7_Release(surf);
    return ret;
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

cleanup:
    releaseObjects();
}
