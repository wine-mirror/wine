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
#include <limits.h>
#include "wine/test.h"
#include "d3dx9.h"

/* helper functions */
static BOOL compare_float(FLOAT f, FLOAT g, UINT ulps)
{
    INT x = *(INT *)&f;
    INT y = *(INT *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static inline INT get_int(D3DXPARAMETER_TYPE type, LPCVOID data)
{
    INT i;

    switch (type)
    {
        case D3DXPT_FLOAT:
            i = *(FLOAT *)data;
            break;

        case D3DXPT_INT:
            i = *(INT *)data;
            break;

        case D3DXPT_BOOL:
            i = *(BOOL *)data;
            break;

        default:
            i = 0;
            ok(0, "Unhandled type %x.\n", type);
            break;
    }

    return i;
}

static inline FLOAT get_float(D3DXPARAMETER_TYPE type, LPCVOID data)
{
    FLOAT f;

    switch (type)
    {
        case D3DXPT_FLOAT:
            f = *(FLOAT *)data;
            break;

        case D3DXPT_INT:
            f = *(INT *)data;
            break;

        case D3DXPT_BOOL:
            f = *(BOOL *)data;
            break;

        default:
            f = 0.0f;
            ok(0, "Unhandled type %x.\n", type);
            break;
    }

    return f;
}

static inline BOOL get_bool(LPCVOID data)
{
    return (*(BOOL *)data) ? TRUE : FALSE;
}

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
    LPD3DXEFFECTSTATEMANAGER manager = (LPD3DXEFFECTSTATEMANAGER)0xdeadbeef;
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

    hr = effect->lpVtbl->GetStateManager(effect, NULL);
    ok(hr == D3DERR_INVALIDCALL, "GetStateManager failed, got %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = effect->lpVtbl->GetStateManager(effect, &manager);
    ok(hr == D3D_OK, "GetStateManager failed, got %x, expected 0 (D3D_OK)\n", hr);
    ok(!manager, "GetStateManager failed, got %p\n", manager);

    /* this works, but it is not recommended! */
    hr = effect->lpVtbl->SetStateManager(effect, (LPD3DXEFFECTSTATEMANAGER) device);
    ok(hr == D3D_OK, "SetStateManager failed, got %x, expected 0 (D3D_OK)\n", hr);

    hr = effect->lpVtbl->GetStateManager(effect, &manager);
    ok(hr == D3D_OK, "GetStateManager failed, got %x, expected 0 (D3D_OK)\n", hr);
    ok(manager != NULL, "GetStateManager failed\n");

    IDirect3DDevice9_AddRef(device);
    count = IDirect3DDevice9_Release(device);
    ok(count == 4, "Release failed, got %u, expected 4\n", count);

    count = IUnknown_Release(manager);
    ok(count == 3, "Release failed, got %u, expected 3\n", count);

    hr = effect->lpVtbl->SetStateManager(effect, NULL);
    ok(hr == D3D_OK, "SetStateManager failed, got %x, expected 0 (D3D_OK)\n", hr);

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

static void test_create_effect_compiler(void)
{
    HRESULT hr;
    ID3DXEffectCompiler *compiler, *compiler2;
    ID3DXBaseEffect *base;
    IUnknown *unknown;
    ULONG count;

    hr = D3DXCreateEffectCompiler(NULL, 0, NULL, NULL, 0, &compiler, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(NULL, 0, NULL, NULL, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(effect_desc, 0, NULL, NULL, 0, &compiler, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    count = compiler->lpVtbl->Release(compiler);
    ok(count == 0, "Release failed %u\n", count);

    hr = D3DXCreateEffectCompiler(effect_desc, 0, NULL, NULL, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(NULL, sizeof(effect_desc), NULL, NULL, 0, &compiler, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(NULL, sizeof(effect_desc), NULL, NULL, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(effect_desc, sizeof(effect_desc), NULL, NULL, 0, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateEffectCompiler(effect_desc, sizeof(effect_desc), NULL, NULL, 0, &compiler, NULL);
    ok(hr == D3D_OK, "Got result %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    hr = compiler->lpVtbl->QueryInterface(compiler, &IID_ID3DXBaseEffect, (void **)&base);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, got %x, expected %x (E_NOINTERFACE)\n", hr, E_NOINTERFACE);

    hr = compiler->lpVtbl->QueryInterface(compiler, &IID_ID3DXEffectCompiler, (void **)&compiler2);
    ok(hr == D3D_OK, "QueryInterface failed, got %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    hr = compiler->lpVtbl->QueryInterface(compiler, &IID_IUnknown, (void **)&unknown);
    ok(hr == D3D_OK, "QueryInterface failed, got %x, expected %x (D3D_OK)\n", hr, D3D_OK);

    count = unknown->lpVtbl->Release(unknown);
    ok(count == 2, "Release failed, got %u, expected %u\n", count, 2);

    count = compiler2->lpVtbl->Release(compiler2);
    ok(count == 1, "Release failed, got %u, expected %u\n", count, 1);

    count = compiler->lpVtbl->Release(compiler);
    ok(count == 0, "Release failed %u\n", count);
}

/*
 * Parameter value test
 */
struct test_effect_parameter_value_result
{
    LPCSTR full_name;
    D3DXPARAMETER_DESC desc;
    UINT value_offset; /* start position for the value in the blob */
};

/*
 * fxc.exe /Tfx_2_0
 */
#if 0
float f = 0.1;
float1 f1 = {1.1};
float2 f2 = {2.1, 2.2};
float3 f3 = {3.1, 3.2, 3.3};
float4 f4 = {4.1, 4.2, 4.3, 4.4};
float1x1 f11 = {11.1};
float1x2 f12 = {12.1, 12.2};
float1x3 f13 = {13.1, 13.2, 13.3};
float1x4 f14 = {14.1, 14.2, 14.3, 14.4};
float2x1 f21 = {{21.11, 21.21}};
float2x2 f22 = {{22.11, 22.21}, {22.12, 22.22}};
float2x3 f23 = {{23.11, 23.21}, {23.12, 23.22}, {23.13, 23.23}};
float2x4 f24 = {{24.11, 24.21}, {24.12, 24.22}, {24.13, 24.23}, {24.14, 24.24}};
float3x1 f31 = {{31.11, 31.21, 31.31}};
float3x2 f32 = {{32.11, 32.21, 32.31}, {32.12, 32.22, 32.32}};
float3x3 f33 = {{33.11, 33.21, 33.31}, {33.12, 33.22, 33.32},
        {33.13, 33.23, 33.33}};
float3x4 f34 = {{34.11, 34.21, 34.31}, {34.12, 34.22, 34.32},
        {34.13, 34.23, 34.33}, {34.14, 34.24, 34.34}};
float4x1 f41 = {{41.11, 41.21, 41.31, 41.41}};
float4x2 f42 = {{42.11, 42.21, 42.31, 42.41}, {42.12, 42.22, 42.32, 42.42}};
float4x3 f43 = {{43.11, 43.21, 43.31, 43.41}, {43.12, 43.22, 43.32, 43.42},
        {43.13, 43.23, 43.33, 43.43}};
float4x4 f44 = {{44.11, 44.21, 44.31, 44.41}, {44.12, 44.22, 44.32, 44.42},
        {44.13, 44.23, 44.33, 44.43}, {44.14, 44.24, 44.34, 44.44}};
float f_2[2] = {0.101, 0.102};
float1 f1_2[2] = {{1.101}, {1.102}};
float2 f2_2[2] = {{2.101, 2.201}, {2.102, 2.202}};
float3 f3_2[2] = {{3.101, 3.201, 3.301}, {3.102, 3.202, 3.302}};
float4 f4_2[2] = {{4.101, 4.201, 4.301, 4.401}, {4.102, 4.202, 4.302, 4.402}};
float1x1 f11_2[2] = {{11.101}, {11.102}};
float1x2 f12_2[2] = {{12.101, 12.201}, {12.102, 12.202}};
float1x3 f13_2[2] = {{13.101, 13.201, 13.301}, {13.102, 13.202, 13.302}};
float1x4 f14_2[2] = {{14.101, 14.201, 14.301, 14.401}, {14.102, 14.202, 14.302, 14.402}};
float2x1 f21_2[2] = {{{21.1101, 21.2101}}, {{21.1102, 21.2102}}};
float2x2 f22_2[2] = {{{22.1101, 22.2101}, {22.1201, 22.2201}}, {{22.1102, 22.2102}, {22.1202, 22.2202}}};
float2x3 f23_2[2] = {{{23.1101, 23.2101}, {23.1201, 23.2201}, {23.1301, 23.2301}}, {{23.1102, 23.2102},
        {23.1202, 23.2202}, {23.1302, 23.2302}}};
float2x4 f24_2[2] = {{{24.1101, 24.2101}, {24.1201, 24.2201}, {24.1301, 24.2301}, {24.1401, 24.2401}},
        {{24.1102, 24.2102}, {24.1202, 24.2202}, {24.1302, 24.2302}, {24.1402, 24.2402}}};
float3x1 f31_2[2] = {{{31.1101, 31.2101, 31.3101}}, {{31.1102, 31.2102, 31.3102}}};
float3x2 f32_2[2] = {{{32.1101, 32.2101, 32.3101}, {32.1201, 32.2201, 32.3201}},
        {{32.1102, 32.2102, 32.3102}, {32.1202, 32.2202, 32.3202}}};
float3x3 f33_2[2] = {{{33.1101, 33.2101, 33.3101}, {33.1201, 33.2201, 33.3201},
        {33.1301, 33.2301, 33.3301}}, {{33.1102, 33.2102, 33.3102}, {33.1202, 33.2202, 33.3202},
        {33.1302, 33.2302, 33.3302}}};
float3x4 f34_2[2] = {{{34.1101, 34.2101, 34.3101}, {34.1201, 34.2201, 34.3201},
        {34.1301, 34.2301, 34.3301}, {34.1401, 34.2401, 34.3401}}, {{34.1102, 34.2102, 34.3102},
        {34.1202, 34.2202, 34.3202}, {34.1302, 34.2302, 34.3302}, {34.1402, 34.2402, 34.3402}}};
float4x1 f41_2[2] = {{{41.1101, 41.2101, 41.3101, 41.4101}}, {{41.1102, 41.2102, 41.3102, 41.4102}}};
float4x2 f42_2[2] = {{{42.1101, 42.2101, 42.3101, 42.4101}, {42.1201, 42.2201, 42.3201, 42.4201}},
        {{42.1102, 42.2102, 42.3102, 42.4102}, {42.1202, 42.2202, 42.3202, 42.4202}}};
float4x3 f43_2[2] = {{{43.1101, 43.2101, 43.3101, 43.4101}, {43.1201, 43.2201, 43.3201, 43.4201},
        {43.1301, 43.2301, 43.3301, 43.4301}}, {{43.1102, 43.2102, 43.3102, 43.4102},
        {43.1202, 43.2202, 43.3202, 43.4202}, {43.1302, 43.2302, 43.3302, 43.4302}}};
float4x4 f44_2[2] = {{{44.1101, 44.2101, 44.3101, 44.4101}, {44.1201, 44.2201, 44.3201, 44.4201},
        {44.1301, 44.2301, 44.3301, 44.4301}, {44.1401, 44.2401, 44.3401, 44.4401}},
        {{44.1102, 44.2102, 44.3102, 44.4102}, {44.1202, 44.2202, 44.3202, 44.4202},
        {44.1302, 44.2302, 44.3302, 44.4302}, {44.1402, 44.2402, 44.3402, 44.4402}}};
technique t { pass p { } }
#endif
static const DWORD test_effect_parameter_value_blob_float[] =
{
0xfeff0901, 0x00000b80, 0x00000000, 0x00000003, 0x00000000, 0x00000024, 0x00000000, 0x00000000,
0x00000001, 0x00000001, 0x3dcccccd, 0x00000002, 0x00000066, 0x00000003, 0x00000001, 0x0000004c,
0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x3f8ccccd, 0x00000003, 0x00003166, 0x00000003,
0x00000001, 0x00000078, 0x00000000, 0x00000000, 0x00000002, 0x00000001, 0x40066666, 0x400ccccd,
0x00000003, 0x00003266, 0x00000003, 0x00000001, 0x000000a8, 0x00000000, 0x00000000, 0x00000003,
0x00000001, 0x40466666, 0x404ccccd, 0x40533333, 0x00000003, 0x00003366, 0x00000003, 0x00000001,
0x000000dc, 0x00000000, 0x00000000, 0x00000004, 0x00000001, 0x40833333, 0x40866666, 0x4089999a,
0x408ccccd, 0x00000003, 0x00003466, 0x00000003, 0x00000002, 0x00000104, 0x00000000, 0x00000000,
0x00000001, 0x00000001, 0x4131999a, 0x00000004, 0x00313166, 0x00000003, 0x00000002, 0x00000130,
0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x4141999a, 0x41433333, 0x00000004, 0x00323166,
0x00000003, 0x00000002, 0x00000160, 0x00000000, 0x00000000, 0x00000001, 0x00000003, 0x4151999a,
0x41533333, 0x4154cccd, 0x00000004, 0x00333166, 0x00000003, 0x00000002, 0x00000194, 0x00000000,
0x00000000, 0x00000001, 0x00000004, 0x4161999a, 0x41633333, 0x4164cccd, 0x41666666, 0x00000004,
0x00343166, 0x00000003, 0x00000002, 0x000001c0, 0x00000000, 0x00000000, 0x00000002, 0x00000001,
0x41a8e148, 0x41a9ae14, 0x00000004, 0x00313266, 0x00000003, 0x00000002, 0x000001f4, 0x00000000,
0x00000000, 0x00000002, 0x00000002, 0x41b0e148, 0x41b1ae14, 0x41b0f5c3, 0x41b1c28f, 0x00000004,
0x00323266, 0x00000003, 0x00000002, 0x00000230, 0x00000000, 0x00000000, 0x00000002, 0x00000003,
0x41b8e148, 0x41b9ae14, 0x41b8f5c3, 0x41b9c28f, 0x41b90a3d, 0x41b9d70a, 0x00000004, 0x00333266,
0x00000003, 0x00000002, 0x00000274, 0x00000000, 0x00000000, 0x00000002, 0x00000004, 0x41c0e148,
0x41c1ae14, 0x41c0f5c3, 0x41c1c28f, 0x41c10a3d, 0x41c1d70a, 0x41c11eb8, 0x41c1eb85, 0x00000004,
0x00343266, 0x00000003, 0x00000002, 0x000002a4, 0x00000000, 0x00000000, 0x00000003, 0x00000001,
0x41f8e148, 0x41f9ae14, 0x41fa7ae1, 0x00000004, 0x00313366, 0x00000003, 0x00000002, 0x000002e0,
0x00000000, 0x00000000, 0x00000003, 0x00000002, 0x420070a4, 0x4200d70a, 0x42013d71, 0x42007ae1,
0x4200e148, 0x420147ae, 0x00000004, 0x00323366, 0x00000003, 0x00000002, 0x00000328, 0x00000000,
0x00000000, 0x00000003, 0x00000003, 0x420470a4, 0x4204d70a, 0x42053d71, 0x42047ae1, 0x4204e148,
0x420547ae, 0x4204851f, 0x4204eb85, 0x420551ec, 0x00000004, 0x00333366, 0x00000003, 0x00000002,
0x0000037c, 0x00000000, 0x00000000, 0x00000003, 0x00000004, 0x420870a4, 0x4208d70a, 0x42093d71,
0x42087ae1, 0x4208e148, 0x420947ae, 0x4208851f, 0x4208eb85, 0x420951ec, 0x42088f5c, 0x4208f5c3,
0x42095c29, 0x00000004, 0x00343366, 0x00000003, 0x00000002, 0x000003b0, 0x00000000, 0x00000000,
0x00000004, 0x00000001, 0x422470a4, 0x4224d70a, 0x42253d71, 0x4225a3d7, 0x00000004, 0x00313466,
0x00000003, 0x00000002, 0x000003f4, 0x00000000, 0x00000000, 0x00000004, 0x00000002, 0x422870a4,
0x4228d70a, 0x42293d71, 0x4229a3d7, 0x42287ae1, 0x4228e148, 0x422947ae, 0x4229ae14, 0x00000004,
0x00323466, 0x00000003, 0x00000002, 0x00000448, 0x00000000, 0x00000000, 0x00000004, 0x00000003,
0x422c70a4, 0x422cd70a, 0x422d3d71, 0x422da3d7, 0x422c7ae1, 0x422ce148, 0x422d47ae, 0x422dae14,
0x422c851f, 0x422ceb85, 0x422d51ec, 0x422db852, 0x00000004, 0x00333466, 0x00000003, 0x00000002,
0x000004ac, 0x00000000, 0x00000000, 0x00000004, 0x00000004, 0x423070a4, 0x4230d70a, 0x42313d71,
0x4231a3d7, 0x42307ae1, 0x4230e148, 0x423147ae, 0x4231ae14, 0x4230851f, 0x4230eb85, 0x423151ec,
0x4231b852, 0x42308f5c, 0x4230f5c3, 0x42315c29, 0x4231c28f, 0x00000004, 0x00343466, 0x00000003,
0x00000000, 0x000004d8, 0x00000000, 0x00000002, 0x00000001, 0x00000001, 0x3dced917, 0x3dd0e560,
0x00000004, 0x00325f66, 0x00000003, 0x00000001, 0x00000504, 0x00000000, 0x00000002, 0x00000001,
0x00000001, 0x3f8ced91, 0x3f8d0e56, 0x00000005, 0x325f3166, 0x00000000, 0x00000003, 0x00000001,
0x0000053c, 0x00000000, 0x00000002, 0x00000002, 0x00000001, 0x400676c9, 0x400cdd2f, 0x4006872b,
0x400ced91, 0x00000005, 0x325f3266, 0x00000000, 0x00000003, 0x00000001, 0x0000057c, 0x00000000,
0x00000002, 0x00000003, 0x00000001, 0x404676c9, 0x404cdd2f, 0x40534396, 0x4046872b, 0x404ced91,
0x405353f8, 0x00000005, 0x325f3366, 0x00000000, 0x00000003, 0x00000001, 0x000005c4, 0x00000000,
0x00000002, 0x00000004, 0x00000001, 0x40833b64, 0x40866e98, 0x4089a1cb, 0x408cd4fe, 0x40834396,
0x408676c9, 0x4089a9fc, 0x408cdd2f, 0x00000005, 0x325f3466, 0x00000000, 0x00000003, 0x00000002,
0x000005f4, 0x00000000, 0x00000002, 0x00000001, 0x00000001, 0x41319db2, 0x4131a1cb, 0x00000006,
0x5f313166, 0x00000032, 0x00000003, 0x00000002, 0x0000062c, 0x00000000, 0x00000002, 0x00000001,
0x00000002, 0x41419db2, 0x4143374c, 0x4141a1cb, 0x41433b64, 0x00000006, 0x5f323166, 0x00000032,
0x00000003, 0x00000002, 0x0000066c, 0x00000000, 0x00000002, 0x00000001, 0x00000003, 0x41519db2,
0x4153374c, 0x4154d0e5, 0x4151a1cb, 0x41533b64, 0x4154d4fe, 0x00000006, 0x5f333166, 0x00000032,
0x00000003, 0x00000002, 0x000006b4, 0x00000000, 0x00000002, 0x00000001, 0x00000004, 0x41619db2,
0x4163374c, 0x4164d0e5, 0x41666a7f, 0x4161a1cb, 0x41633b64, 0x4164d4fe, 0x41666e98, 0x00000006,
0x5f343166, 0x00000032, 0x00000003, 0x00000002, 0x000006ec, 0x00000000, 0x00000002, 0x00000002,
0x00000001, 0x41a8e17c, 0x41a9ae49, 0x41a8e1b1, 0x41a9ae7d, 0x00000006, 0x5f313266, 0x00000032,
0x00000003, 0x00000002, 0x00000734, 0x00000000, 0x00000002, 0x00000002, 0x00000002, 0x41b0e17c,
0x41b1ae49, 0x41b0f5f7, 0x41b1c2c4, 0x41b0e1b1, 0x41b1ae7d, 0x41b0f62b, 0x41b1c2f8, 0x00000006,
0x5f323266, 0x00000032, 0x00000003, 0x00000002, 0x0000078c, 0x00000000, 0x00000002, 0x00000002,
0x00000003, 0x41b8e17c, 0x41b9ae49, 0x41b8f5f7, 0x41b9c2c4, 0x41b90a72, 0x41b9d73f, 0x41b8e1b1,
0x41b9ae7d, 0x41b8f62b, 0x41b9c2f8, 0x41b90aa6, 0x41b9d773, 0x00000006, 0x5f333266, 0x00000032,
0x00000003, 0x00000002, 0x000007f4, 0x00000000, 0x00000002, 0x00000002, 0x00000004, 0x41c0e17c,
0x41c1ae49, 0x41c0f5f7, 0x41c1c2c4, 0x41c10a72, 0x41c1d73f, 0x41c11eed, 0x41c1ebba, 0x41c0e1b1,
0x41c1ae7d, 0x41c0f62b, 0x41c1c2f8, 0x41c10aa6, 0x41c1d773, 0x41c11f21, 0x41c1ebee, 0x00000006,
0x5f343266, 0x00000032, 0x00000003, 0x00000002, 0x00000834, 0x00000000, 0x00000002, 0x00000003,
0x00000001, 0x41f8e17c, 0x41f9ae49, 0x41fa7b16, 0x41f8e1b1, 0x41f9ae7d, 0x41fa7b4a, 0x00000006,
0x5f313366, 0x00000032, 0x00000003, 0x00000002, 0x0000088c, 0x00000000, 0x00000002, 0x00000003,
0x00000002, 0x420070be, 0x4200d724, 0x42013d8b, 0x42007afb, 0x4200e162, 0x420147c8, 0x420070d8,
0x4200d73f, 0x42013da5, 0x42007b16, 0x4200e17c, 0x420147e3, 0x00000006, 0x5f323366, 0x00000032,
0x00000003, 0x00000002, 0x000008fc, 0x00000000, 0x00000002, 0x00000003, 0x00000003, 0x420470be,
0x4204d724, 0x42053d8b, 0x42047afb, 0x4204e162, 0x420547c8, 0x42048539, 0x4204eb9f, 0x42055206,
0x420470d8, 0x4204d73f, 0x42053da5, 0x42047b16, 0x4204e17c, 0x420547e3, 0x42048553, 0x4204ebba,
0x42055220, 0x00000006, 0x5f333366, 0x00000032, 0x00000003, 0x00000002, 0x00000984, 0x00000000,
0x00000002, 0x00000003, 0x00000004, 0x420870be, 0x4208d724, 0x42093d8b, 0x42087afb, 0x4208e162,
0x420947c8, 0x42088539, 0x4208eb9f, 0x42095206, 0x42088f76, 0x4208f5dd, 0x42095c43, 0x420870d8,
0x4208d73f, 0x42093da5, 0x42087b16, 0x4208e17c, 0x420947e3, 0x42088553, 0x4208ebba, 0x42095220,
0x42088f91, 0x4208f5f7, 0x42095c5d, 0x00000006, 0x5f343366, 0x00000032, 0x00000003, 0x00000002,
0x000009cc, 0x00000000, 0x00000002, 0x00000004, 0x00000001, 0x422470be, 0x4224d724, 0x42253d8b,
0x4225a3f1, 0x422470d8, 0x4224d73f, 0x42253da5, 0x4225a40b, 0x00000006, 0x5f313466, 0x00000032,
0x00000003, 0x00000002, 0x00000a34, 0x00000000, 0x00000002, 0x00000004, 0x00000002, 0x422870be,
0x4228d724, 0x42293d8b, 0x4229a3f1, 0x42287afb, 0x4228e162, 0x422947c8, 0x4229ae2f, 0x422870d8,
0x4228d73f, 0x42293da5, 0x4229a40b, 0x42287b16, 0x4228e17c, 0x422947e3, 0x4229ae49, 0x00000006,
0x5f323466, 0x00000032, 0x00000003, 0x00000002, 0x00000abc, 0x00000000, 0x00000002, 0x00000004,
0x00000003, 0x422c70be, 0x422cd724, 0x422d3d8b, 0x422da3f1, 0x422c7afb, 0x422ce162, 0x422d47c8,
0x422dae2f, 0x422c8539, 0x422ceb9f, 0x422d5206, 0x422db86c, 0x422c70d8, 0x422cd73f, 0x422d3da5,
0x422da40b, 0x422c7b16, 0x422ce17c, 0x422d47e3, 0x422dae49, 0x422c8553, 0x422cebba, 0x422d5220,
0x422db886, 0x00000006, 0x5f333466, 0x00000032, 0x00000003, 0x00000002, 0x00000b64, 0x00000000,
0x00000002, 0x00000004, 0x00000004, 0x423070be, 0x4230d724, 0x42313d8b, 0x4231a3f1, 0x42307afb,
0x4230e162, 0x423147c8, 0x4231ae2f, 0x42308539, 0x4230eb9f, 0x42315206, 0x4231b86c, 0x42308f76,
0x4230f5dd, 0x42315c43, 0x4231c2aa, 0x423070d8, 0x4230d73f, 0x42313da5, 0x4231a40b, 0x42307b16,
0x4230e17c, 0x423147e3, 0x4231ae49, 0x42308553, 0x4230ebba, 0x42315220, 0x4231b886, 0x42308f91,
0x4230f5f7, 0x42315c5d, 0x4231c2c4, 0x00000006, 0x5f343466, 0x00000032, 0x00000002, 0x00000070,
0x00000002, 0x00000074, 0x0000002a, 0x00000001, 0x00000001, 0x00000001, 0x00000004, 0x00000020,
0x00000000, 0x00000000, 0x0000002c, 0x00000048, 0x00000000, 0x00000000, 0x00000054, 0x00000070,
0x00000000, 0x00000000, 0x00000080, 0x0000009c, 0x00000000, 0x00000000, 0x000000b0, 0x000000cc,
0x00000000, 0x00000000, 0x000000e4, 0x00000100, 0x00000000, 0x00000000, 0x0000010c, 0x00000128,
0x00000000, 0x00000000, 0x00000138, 0x00000154, 0x00000000, 0x00000000, 0x00000168, 0x00000184,
0x00000000, 0x00000000, 0x0000019c, 0x000001b8, 0x00000000, 0x00000000, 0x000001c8, 0x000001e4,
0x00000000, 0x00000000, 0x000001fc, 0x00000218, 0x00000000, 0x00000000, 0x00000238, 0x00000254,
0x00000000, 0x00000000, 0x0000027c, 0x00000298, 0x00000000, 0x00000000, 0x000002ac, 0x000002c8,
0x00000000, 0x00000000, 0x000002e8, 0x00000304, 0x00000000, 0x00000000, 0x00000330, 0x0000034c,
0x00000000, 0x00000000, 0x00000384, 0x000003a0, 0x00000000, 0x00000000, 0x000003b8, 0x000003d4,
0x00000000, 0x00000000, 0x000003fc, 0x00000418, 0x00000000, 0x00000000, 0x00000450, 0x0000046c,
0x00000000, 0x00000000, 0x000004b4, 0x000004d0, 0x00000000, 0x00000000, 0x000004e0, 0x000004fc,
0x00000000, 0x00000000, 0x00000510, 0x0000052c, 0x00000000, 0x00000000, 0x00000548, 0x00000564,
0x00000000, 0x00000000, 0x00000588, 0x000005a4, 0x00000000, 0x00000000, 0x000005d0, 0x000005ec,
0x00000000, 0x00000000, 0x00000600, 0x0000061c, 0x00000000, 0x00000000, 0x00000638, 0x00000654,
0x00000000, 0x00000000, 0x00000678, 0x00000694, 0x00000000, 0x00000000, 0x000006c0, 0x000006dc,
0x00000000, 0x00000000, 0x000006f8, 0x00000714, 0x00000000, 0x00000000, 0x00000740, 0x0000075c,
0x00000000, 0x00000000, 0x00000798, 0x000007b4, 0x00000000, 0x00000000, 0x00000800, 0x0000081c,
0x00000000, 0x00000000, 0x00000840, 0x0000085c, 0x00000000, 0x00000000, 0x00000898, 0x000008b4,
0x00000000, 0x00000000, 0x00000908, 0x00000924, 0x00000000, 0x00000000, 0x00000990, 0x000009ac,
0x00000000, 0x00000000, 0x000009d8, 0x000009f4, 0x00000000, 0x00000000, 0x00000a40, 0x00000a5c,
0x00000000, 0x00000000, 0x00000ac8, 0x00000ae4, 0x00000000, 0x00000000, 0x00000b78, 0x00000000,
0x00000001, 0x00000b70, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

struct test_effect_parameter_value_result test_effect_parameter_value_result_float[] =
{
    {"f",     {"f",     NULL, D3DXPC_SCALAR,      D3DXPT_FLOAT, 1, 1, 0, 0, 0, 0,   4},  10},
    {"f1",    {"f1",    NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 1, 0, 0, 0, 0,   4},  20},
    {"f2",    {"f2",    NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 2, 0, 0, 0, 0,   8},  30},
    {"f3",    {"f3",    NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 3, 0, 0, 0, 0,  12},  41},
    {"f4",    {"f4",    NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 4, 0, 0, 0, 0,  16},  53},
    {"f11",   {"f11",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 1, 0, 0, 0, 0,   4},  66},
    {"f12",   {"f12",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 2, 0, 0, 0, 0,   8},  76},
    {"f13",   {"f13",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 3, 0, 0, 0, 0,  12},  87},
    {"f14",   {"f14",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 4, 0, 0, 0, 0,  16},  99},
    {"f21",   {"f21",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 1, 0, 0, 0, 0,   8}, 112},
    {"f22",   {"f22",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 2, 0, 0, 0, 0,  16}, 123},
    {"f23",   {"f23",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 3, 0, 0, 0, 0,  24}, 136},
    {"f24",   {"f24",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 4, 0, 0, 0, 0,  32}, 151},
    {"f31",   {"f31",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 1, 0, 0, 0, 0,  12}, 168},
    {"f32",   {"f32",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 0, 0, 0, 0,  24}, 180},
    {"f33",   {"f33",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 3, 0, 0, 0, 0,  36}, 195},
    {"f34",   {"f34",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 4, 0, 0, 0, 0,  48}, 213},
    {"f41",   {"f41",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 1, 0, 0, 0, 0,  16}, 234},
    {"f42",   {"f42",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 2, 0, 0, 0, 0,  32}, 247},
    {"f43",   {"f43",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 3, 0, 0, 0, 0,  48}, 264},
    {"f44",   {"f44",   NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 4, 0, 0, 0, 0,  64}, 285},
    {"f_2",   {"f_2",   NULL, D3DXPC_SCALAR,      D3DXPT_FLOAT, 1, 1, 2, 0, 0, 0,   8}, 310},
    {"f1_2",  {"f1_2",  NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 1, 2, 0, 0, 0,   8}, 321},
    {"f2_2",  {"f2_2",  NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 2, 2, 0, 0, 0,  16}, 333},
    {"f3_2",  {"f3_2",  NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 3, 2, 0, 0, 0,  24}, 347},
    {"f4_2",  {"f4_2",  NULL, D3DXPC_VECTOR,      D3DXPT_FLOAT, 1, 4, 2, 0, 0, 0,  32}, 363},
    {"f11_2", {"f11_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 1, 2, 0, 0, 0,   8}, 381},
    {"f12_2", {"f12_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 2, 2, 0, 0, 0,  16}, 393},
    {"f13_2", {"f13_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 3, 2, 0, 0, 0,  24}, 407},
    {"f14_2", {"f14_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 1, 4, 2, 0, 0, 0,  32}, 423},
    {"f21_2", {"f21_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 1, 2, 0, 0, 0,  16}, 441},
    {"f22_2", {"f22_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 2, 2, 0, 0, 0,  32}, 455},
    {"f23_2", {"f23_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 3, 2, 0, 0, 0,  48}, 473},
    {"f24_2", {"f24_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 2, 4, 2, 0, 0, 0,  64}, 495},
    {"f31_2", {"f31_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 1, 2, 0, 0, 0,  24}, 521},
    {"f32_2", {"f32_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 2, 2, 0, 0, 0,  48}, 537},
    {"f33_2", {"f33_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 3, 2, 0, 0, 0,  72}, 559},
    {"f34_2", {"f34_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 3, 4, 2, 0, 0, 0,  96}, 587},
    {"f41_2", {"f41_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 1, 2, 0, 0, 0,  32}, 621},
    {"f42_2", {"f42_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 2, 2, 0, 0, 0,  64}, 639},
    {"f43_2", {"f43_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 3, 2, 0, 0, 0,  96}, 665},
    {"f44_2", {"f44_2", NULL, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 4, 2, 0, 0, 0, 128}, 699},
};

#define ADD_PARAMETER_VALUE(x) {\
    test_effect_parameter_value_blob_ ## x,\
    sizeof(test_effect_parameter_value_blob_ ## x),\
    test_effect_parameter_value_result_ ## x,\
    sizeof(test_effect_parameter_value_result_ ## x)/sizeof(*test_effect_parameter_value_result_ ## x),\
}

static const struct
{
    const DWORD *blob;
    UINT blob_size;
    const struct test_effect_parameter_value_result *res;
    UINT res_count;
}
test_effect_parameter_value_data[] =
{
    ADD_PARAMETER_VALUE(float),
};

#undef ADD_PARAMETER_VALUE

#define EFFECT_PARAMETER_VALUE_ARRAY_SIZE 48
/* Constants for special INT/FLOAT conversation */
#define INT_FLOAT_MULTI 255.0f

static void test_effect_parameter_value_GetValue(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    DWORD value[EFFECT_PARAMETER_VALUE_ARRAY_SIZE];
    HRESULT hr;
    UINT l;

    memset(value, 0xab, sizeof(value));
    hr = effect->lpVtbl->GetValue(effect, parameter, value, res_desc->Bytes);
    if (res_desc->Class == D3DXPC_SCALAR
            || res_desc->Class == D3DXPC_VECTOR
            || res_desc->Class == D3DXPC_MATRIX_ROWS)
    {
        ok(hr == D3D_OK, "%u - %s: GetValue failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        for (l = 0; l < res_desc->Bytes / sizeof(*value); ++l)
        {
            ok(value[l] == res_value[l], "%u - %s: GetValue value[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, value[l], res_value[l]);
        }

        for (l = res_desc->Bytes / sizeof(*value); l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(value[l] == 0xabababab, "%u - %s: GetValue value[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, value[l], 0xabababab);
        }
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetValue failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);

        for (l = 0; l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(value[l] == 0xabababab, "%u - %s: GetValue value[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, value[l], 0xabababab);
        }
    }
}

static void test_effect_parameter_value_GetBool(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    BOOL bvalue = 0xabababab;
    HRESULT hr;

    hr = effect->lpVtbl->GetBool(effect, parameter, &bvalue);
    if (!res_desc->Elements && res_desc->Rows == 1 && res_desc->Columns == 1)
    {
        ok(hr == D3D_OK, "%u - %s: GetBool failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);
        ok(bvalue == get_bool(res_value), "%u - %s: GetBool bvalue failed, got %#x, expected %#x\n",
                i, res_full_name, bvalue, get_bool(res_value));
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetBool failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);
        ok(bvalue == 0xabababab, "%u - %s: GetBool bvalue failed, got %#x, expected %#x\n",
                i, res_full_name, bvalue, 0xabababab);
    }
}

static void test_effect_parameter_value_GetBoolArray(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    BOOL bavalue[EFFECT_PARAMETER_VALUE_ARRAY_SIZE];
    HRESULT hr;
    UINT l;

    memset(bavalue, 0xab, sizeof(bavalue));
    hr = effect->lpVtbl->GetBoolArray(effect, parameter, bavalue, res_desc->Bytes / sizeof(*bavalue));
    if (res_desc->Class == D3DXPC_SCALAR
            || res_desc->Class == D3DXPC_VECTOR
            || res_desc->Class == D3DXPC_MATRIX_ROWS)
    {
        ok(hr == D3D_OK, "%u - %s: GetBoolArray failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        for (l = 0; l < res_desc->Bytes / sizeof(*bavalue); ++l)
        {
            ok(bavalue[l] == get_bool(&res_value[l]), "%u - %s: GetBoolArray bavalue[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, bavalue[l], get_bool(&res_value[l]));
        }

        for (l = res_desc->Bytes / sizeof(*bavalue); l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(bavalue[l] == 0xabababab, "%u - %s: GetBoolArray bavalue[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, bavalue[l], 0xabababab);
        }
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetBoolArray failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);

        for (l = 0; l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(bavalue[l] == 0xabababab, "%u - %s: GetBoolArray bavalue[%u] failed, got %#x, expected %#x\n",
                    i, res_full_name, l, bavalue[l], 0xabababab);
        }
    }
}

static void test_effect_parameter_value_GetInt(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    INT ivalue = 0xabababab;
    HRESULT hr;

    hr = effect->lpVtbl->GetInt(effect, parameter, &ivalue);
    if (!res_desc->Elements && res_desc->Columns == 1 && res_desc->Rows == 1)
    {
        ok(hr == D3D_OK, "%u - %s: GetInt failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);
        ok(ivalue == get_int(res_desc->Type, res_value), "%u - %s: GetInt ivalue failed, got %i, expected %i\n",
                i, res_full_name, ivalue, get_int(res_desc->Type, res_value));
    }
    else if(!res_desc->Elements && res_desc->Type == D3DXPT_FLOAT &&
            ((res_desc->Class == D3DXPC_VECTOR && res_desc->Columns != 2) ||
            (res_desc->Class == D3DXPC_MATRIX_ROWS && res_desc->Rows != 2 && res_desc->Columns == 1)))
    {
        INT tmp;

        ok(hr == D3D_OK, "%u - %s: GetInt failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        tmp = (INT)(min(max(0.0f, *((FLOAT *)res_value + 2)), 1.0f) * INT_FLOAT_MULTI);
        tmp += ((INT)(min(max(0.0f, *((FLOAT *)res_value + 1)), 1.0f) * INT_FLOAT_MULTI)) << 8;
        tmp += ((INT)(min(max(0.0f, *((FLOAT *)res_value + 0)), 1.0f) * INT_FLOAT_MULTI)) << 16;
        if (res_desc->Columns * res_desc->Rows > 3)
        {
            tmp += ((INT)(min(max(0.0f, *((FLOAT *)res_value + 3)), 1.0f) * INT_FLOAT_MULTI)) << 24;
        }

        ok(ivalue == tmp, "%u - %s: GetInt ivalue failed, got %x, expected %x\n",
                i, res_full_name, ivalue, tmp);
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetInt failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);
        ok(ivalue == 0xabababab, "%u - %s: GetInt ivalue failed, got %i, expected %i\n",
                i, res_full_name, ivalue, 0xabababab);
    }
}

static void test_effect_parameter_value_GetIntArray(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    INT iavalue[EFFECT_PARAMETER_VALUE_ARRAY_SIZE];
    HRESULT hr;
    UINT l;

    memset(iavalue, 0xab, sizeof(iavalue));
    hr = effect->lpVtbl->GetIntArray(effect, parameter, iavalue, res_desc->Bytes / sizeof(*iavalue));
    if (res_desc->Class == D3DXPC_SCALAR
            || res_desc->Class == D3DXPC_VECTOR
            || res_desc->Class == D3DXPC_MATRIX_ROWS)
    {
        ok(hr == D3D_OK, "%u - %s: GetIntArray failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        for (l = 0; l < res_desc->Bytes / sizeof(*iavalue); ++l)
        {
            ok(iavalue[l] == get_int(res_desc->Type, &res_value[l]), "%u - %s: GetIntArray iavalue[%u] failed, got %i, expected %i\n",
                    i, res_full_name, l, iavalue[l], get_int(res_desc->Type, &res_value[l]));
        }

        for (l = res_desc->Bytes / sizeof(*iavalue); l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(iavalue[l] == 0xabababab, "%u - %s: GetIntArray iavalue[%u] failed, got %i, expected %i\n",
                    i, res_full_name, l, iavalue[l], 0xabababab);
        }
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetIntArray failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);

        for (l = 0; l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(iavalue[l] == 0xabababab, "%u - %s: GetIntArray iavalue[%u] failed, got %i, expected %i\n",
                    i, res_full_name, l, iavalue[l], 0xabababab);
        }
    }
}

static void test_effect_parameter_value_GetFloat(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    HRESULT hr;
    DWORD cmp = 0xabababab;
    FLOAT fvalue = *(FLOAT *)&cmp;

    hr = effect->lpVtbl->GetFloat(effect, parameter, &fvalue);
    if (!res_desc->Elements && res_desc->Columns == 1 && res_desc->Rows == 1)
    {
        ok(hr == D3D_OK, "%u - %s: GetFloat failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);
        ok(compare_float(fvalue, get_float(res_desc->Type, res_value), 512), "%u - %s: GetFloat fvalue failed, got %f, expected %f\n",
                i, res_full_name, fvalue, get_float(res_desc->Type, res_value));
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetFloat failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);
        ok(fvalue == *(FLOAT *)&cmp, "%u - %s: GetFloat fvalue failed, got %f, expected %f\n",
                i, res_full_name, fvalue, *(FLOAT *)&cmp);
    }
}

static void test_effect_parameter_value_GetFloatArray(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    FLOAT favalue[EFFECT_PARAMETER_VALUE_ARRAY_SIZE];
    HRESULT hr;
    UINT l;
    DWORD cmp = 0xabababab;

    memset(favalue, 0xab, sizeof(favalue));
    hr = effect->lpVtbl->GetFloatArray(effect, parameter, favalue, res_desc->Bytes / sizeof(*favalue));
    if (res_desc->Class == D3DXPC_SCALAR
            || res_desc->Class == D3DXPC_VECTOR
            || res_desc->Class == D3DXPC_MATRIX_ROWS)
    {
        ok(hr == D3D_OK, "%u - %s: GetFloatArray failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        for (l = 0; l < res_desc->Bytes / sizeof(*favalue); ++l)
        {
            ok(compare_float(favalue[l], get_float(res_desc->Type, &res_value[l]), 512),
                    "%u - %s: GetFloatArray favalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, favalue[l], get_float(res_desc->Type, &res_value[l]));
        }

        for (l = res_desc->Bytes / sizeof(*favalue); l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(favalue[l] == *(FLOAT *)&cmp, "%u - %s: GetFloatArray favalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, favalue[l], *(FLOAT *)&cmp);
        }
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetFloatArray failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);

        for (l = 0; l < EFFECT_PARAMETER_VALUE_ARRAY_SIZE; ++l)
        {
            ok(favalue[l] == *(FLOAT *)&cmp, "%u - %s: GetFloatArray favalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, favalue[l], *(FLOAT *)&cmp);
        }
    }
}

static void test_effect_parameter_value_GetVector(const struct test_effect_parameter_value_result *res,
        ID3DXEffect *effect, const DWORD *res_value, D3DXHANDLE parameter, UINT i)
{
    const D3DXPARAMETER_DESC *res_desc = &res->desc;
    LPCSTR res_full_name = res->full_name;
    HRESULT hr;
    DWORD cmp = 0xabababab;
    FLOAT fvalue[4];
    UINT l;

    memset(fvalue, 0xab, sizeof(fvalue));
    hr = effect->lpVtbl->GetVector(effect, parameter, (D3DXVECTOR4 *)&fvalue);
    if (!res_desc->Elements &&
            (res_desc->Class == D3DXPC_SCALAR || res_desc->Class == D3DXPC_VECTOR) &&
            res_desc->Type == D3DXPT_INT && res_desc->Bytes == 4)
    {
        DWORD tmp;

        ok(hr == D3D_OK, "%u - %s: GetVector failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        tmp = (DWORD)(*(fvalue + 2) * INT_FLOAT_MULTI);
        tmp += ((DWORD)(*(fvalue + 1) * INT_FLOAT_MULTI)) << 8;
        tmp += ((DWORD)(*fvalue * INT_FLOAT_MULTI)) << 16;
        tmp += ((DWORD)(*(fvalue + 3) * INT_FLOAT_MULTI)) << 24;

        ok(*res_value == tmp, "%u - %s: GetVector ivalue failed, got %i, expected %i\n",
                i, res_full_name, tmp, *res_value);
    }
    else if (!res_desc->Elements && (res_desc->Class == D3DXPC_SCALAR || res_desc->Class == D3DXPC_VECTOR))
    {
        ok(hr == D3D_OK, "%u - %s: GetVector failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

        for (l = 0; l < res_desc->Columns; ++l)
        {
            ok(compare_float(fvalue[l], get_float(res_desc->Type, &res_value[l]), 512),
                    "%u - %s: GetVector fvalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, fvalue[l], get_float(res_desc->Type, &res_value[l]));
        }

        for (l = res_desc->Columns; l < 4; ++l)
        {
            ok(fvalue[l] == 0.0f, "%u - %s: GetVector fvalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, fvalue[l], 0.0f);
        }
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "%u - %s: GetVector failed, got %#x, expected %#x\n",
                i, res_full_name, hr, D3DERR_INVALIDCALL);

        for (l = 0; l < 4; ++l)
        {
            ok(fvalue[l] == *(FLOAT *)&cmp, "%u - %s: GetVector fvalue[%u] failed, got %f, expected %f\n",
                    i, res_full_name, l, fvalue[l], *(FLOAT *)&cmp);
        }
    }
}

static void test_effect_parameter_value(IDirect3DDevice9 *device)
{
    UINT i;
    UINT effect_count = sizeof(test_effect_parameter_value_data) / sizeof(*test_effect_parameter_value_data);

    for (i = 0; i < effect_count; ++i)
    {
        const struct test_effect_parameter_value_result *res = test_effect_parameter_value_data[i].res;
        UINT res_count = test_effect_parameter_value_data[i].res_count;
        const DWORD *blob = test_effect_parameter_value_data[i].blob;
        UINT blob_size = test_effect_parameter_value_data[i].blob_size;
        HRESULT hr;
        ID3DXEffect *effect;
        D3DXEFFECT_DESC edesc;
        ULONG count;
        UINT k;

        hr = D3DXCreateEffect(device, blob, blob_size, NULL, NULL, 0, NULL, &effect, NULL);
        ok(hr == D3D_OK, "%u: D3DXCreateEffect failed, got %#x, expected %#x\n", i, hr, D3D_OK);

        hr = effect->lpVtbl->GetDesc(effect, &edesc);
        ok(hr == D3D_OK, "%u: GetDesc failed, got %#x, expected %#x\n", i, hr, D3D_OK);
        ok(edesc.Parameters == res_count, "%u: Parameters failed, got %u, expected %u\n",
                i, edesc.Parameters, res_count);

        for (k = 0; k < res_count; ++k)
        {
            const D3DXPARAMETER_DESC *res_desc = &res[k].desc;
            LPCSTR res_full_name = res[k].full_name;
            UINT res_value_offset = res[k].value_offset;
            D3DXHANDLE parameter;
            D3DXPARAMETER_DESC pdesc;

            parameter = effect->lpVtbl->GetParameterByName(effect, NULL, res_full_name);
            ok(parameter != NULL, "%u - %s: GetParameterByName failed\n", i, res_full_name);

            hr = effect->lpVtbl->GetParameterDesc(effect, parameter, &pdesc);
            ok(hr == D3D_OK, "%u - %s: GetParameterDesc failed, got %#x, expected %#x\n", i, res_full_name, hr, D3D_OK);

            ok(res_desc->Name ? !strcmp(pdesc.Name, res_desc->Name) : !pdesc.Name,
                    "%u - %s: GetParameterDesc Name failed, got \"%s\", expected \"%s\"\n",
                    i, res_full_name, pdesc.Name, res_desc->Name);
            ok(res_desc->Semantic ? !strcmp(pdesc.Semantic, res_desc->Semantic) : !pdesc.Semantic,
                    "%u - %s: GetParameterDesc Semantic failed, got \"%s\", expected \"%s\"\n",
                    i, res_full_name, pdesc.Semantic, res_desc->Semantic);
            ok(res_desc->Class == pdesc.Class, "%u - %s: GetParameterDesc Class failed, got %#x, expected %#x\n",
                    i, res_full_name, pdesc.Class, res_desc->Class);
            ok(res_desc->Type == pdesc.Type, "%u - %s: GetParameterDesc Type failed, got %#x, expected %#x\n",
                    i, res_full_name, pdesc.Type, res_desc->Type);
            ok(res_desc->Rows == pdesc.Rows, "%u - %s: GetParameterDesc Rows failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.Rows, res_desc->Rows);
            ok(res_desc->Columns == pdesc.Columns, "%u - %s: GetParameterDesc Columns failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.Columns, res_desc->Columns);
            ok(res_desc->Elements == pdesc.Elements, "%u - %s: GetParameterDesc Elements failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.Elements, res_desc->Elements);
            ok(res_desc->Annotations == pdesc.Annotations, "%u - %s: GetParameterDesc Annotations failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.Annotations, res_desc->Annotations);
            ok(res_desc->StructMembers == pdesc.StructMembers, "%u - %s: GetParameterDesc StructMembers failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.StructMembers, res_desc->StructMembers);
            ok(res_desc->Flags == pdesc.Flags, "%u - %s: GetParameterDesc Flags failed, got %u, expected %u\n",
                    i, res_full_name, pdesc.Flags, res_desc->Flags);
            ok(res_desc->Bytes == pdesc.Bytes, "%u - %s: GetParameterDesc Bytes, got %u, expected %u\n",
                    i, res_full_name, pdesc.Bytes, res_desc->Bytes);

            /* check size */
            ok(EFFECT_PARAMETER_VALUE_ARRAY_SIZE >= res_desc->Bytes / 4 +
                    (res_desc->Elements ? res_desc->Bytes / 4 / res_desc->Elements : 0),
                    "%u - %s: Warning: Array size to small\n", i, res_full_name);

            test_effect_parameter_value_GetValue(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetBool(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetBoolArray(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetInt(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetIntArray(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetFloat(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetFloatArray(&res[k], effect, &blob[res_value_offset], parameter, i);
            test_effect_parameter_value_GetVector(&res[k], effect, &blob[res_value_offset], parameter, i);
        }

        count = effect->lpVtbl->Release(effect);
        ok(!count, "Release failed %u\n", count);
    }
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
    test_create_effect_compiler();
    test_effect_parameter_value(device);

    count = IDirect3DDevice9_Release(device);
    ok(count == 0, "The device was not properly freed: refcount %u\n", count);

    count = IDirect3D9_Release(d3d);
    ok(count == 0, "Release failed %u\n", count);

    DestroyWindow(wnd);
}
