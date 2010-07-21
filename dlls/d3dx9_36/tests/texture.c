/*
 * Tests for the D3DX9 texture functions
 *
 * Copyright 2009 Tony Wasserka
 * Copyright 2010 Owen Rudge for CodeWeavers
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
#include "wine/test.h"
#include "d3dx9tex.h"

static void test_D3DXCheckTextureRequirements(IDirect3DDevice9 *device)
{
    HRESULT hr;

    todo_wine {

    /* general tests */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    } /* todo_wine */
}

START_TEST(texture)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (!wnd) {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXCheckTextureRequirements(device);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}
