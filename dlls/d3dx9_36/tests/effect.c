/*
 * Copyright 2010 Christian Costa
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
#include "initguid.h"
#include "wine/test.h"
#include "d3dx9.h"

static const char effect_desc[] =
"Technique\n"
"{\n"
"}\n";

static void test_create_effect_and_pool(IDirect3DDevice9 *device)
{
    HRESULT hr;
    ID3DXEffect *effect;
    ID3DXBaseEffect *base;
    ULONG count;
    IDirect3DDevice9 *device2;
    ID3DXEffectPool *pool = (ID3DXEffectPool *)0xdeadbeef, *pool2;

    hr = D3DXCreateEffect(NULL, effect_desc, sizeof(effect_desc), NULL, NULL, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffect(device, NULL, 0, NULL, NULL, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffect(device, effect_desc, 0, NULL, NULL, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, E_FAIL);

    hr = D3DXCreateEffect(device, effect_desc, sizeof(effect_desc), NULL, NULL, 0, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = D3DXCreateEffect(device, effect_desc, sizeof(effect_desc), NULL, NULL, 0, NULL, &effect, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = effect->lpVtbl->QueryInterface(effect, &IID_ID3DXBaseEffect, (void **)&base);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, got %x, expected %x (E_NOINTERFACE)\n", hr, E_NOINTERFACE);

    hr = effect->lpVtbl->GetPool(effect, &pool);
    ok(hr == D3D_OK, "GetPool failed, got %x, expected 0 (D3D_OK)\n", hr);
    ok(!pool, "GetPool failed, got %p\n", pool);

    hr = effect->lpVtbl->GetPool(effect, NULL);
    ok(hr == D3DERR_INVALIDCALL, "GetPool failed, got %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = effect->lpVtbl->GetDevice(effect, &device2);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = effect->lpVtbl->GetDevice(effect, NULL);
    ok(hr == D3DERR_INVALIDCALL, "GetDevice failed, got %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    count = IDirect3DDevice9_Release(device2);
    ok(count == 2, "Release failed, got %u, expected 2\n", count);

    count = effect->lpVtbl->Release(effect);
    ok(count == 0, "Release failed %u\n", count);

    hr = D3DXCreateEffectPool(NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectPool(&pool);
    ok(hr == S_OK, "Got result %x, expected 0 (S_OK)\n", hr);

    count = pool->lpVtbl->Release(pool);
    ok(count == 0, "Release failed %u\n", count);

    hr = D3DXCreateEffectPool(&pool);
    ok(hr == S_OK, "Got result %x, expected 0 (S_OK)\n", hr);

    hr = D3DXCreateEffect(device, effect_desc, sizeof(effect_desc), NULL, NULL, 0, pool, NULL, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = pool->lpVtbl->QueryInterface(pool, &IID_ID3DXEffectPool, (void **)&pool2);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(pool == pool2, "Release failed, got %p, expected %p\n", pool2, pool);

    count = pool2->lpVtbl->Release(pool2);
    ok(count == 1, "Release failed, got %u, expected 1\n", count);

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9, (void **)&device2);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    count = IDirect3DDevice9_Release(device2);
    ok(count == 1, "Release failed, got %u, expected 1\n", count);

    hr = D3DXCreateEffect(device, effect_desc, sizeof(effect_desc), NULL, NULL, 0, pool, &effect, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = effect->lpVtbl->GetPool(effect, &pool);
    ok(hr == D3D_OK, "GetPool failed, got %x, expected 0 (D3D_OK)\n", hr);
    ok(pool == pool2, "GetPool failed, got %p, expected %p\n", pool2, pool);

    count = pool2->lpVtbl->Release(pool2);
    ok(count == 2, "Release failed, got %u, expected 2\n", count);

    count = effect->lpVtbl->Release(effect);
    ok(count == 0, "Release failed %u\n", count);

    count = pool->lpVtbl->Release(pool);
    ok(count == 0, "Release failed %u\n", count);
}

START_TEST(effect)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;
    ULONG count;

    wnd = CreateWindow("static", "d3dx9_test", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!wnd) {
        skip("Couldn't create application window\n");
        return;
    }
    if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_create_effect_and_pool(device);

    count = IDirect3DDevice9_Release(device);
    ok(count == 0, "The device was not properly freed: refcount %u\n", count);

    count = IDirect3D9_Release(d3d);
    ok(count == 0, "Release failed %u\n", count);

    DestroyWindow(wnd);
}
