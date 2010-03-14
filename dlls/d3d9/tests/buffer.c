/*
 * Copyright (C) 2010 Stefan Dösinger(for CodeWeavers)
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
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    return CreateWindow("d3d9_test_wc", "d3d9_test",
            0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static IDirect3DDevice9 *init_d3d9(void)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion) = 0;
    IDirect3D9 *d3d9_ptr = 0;
    IDirect3DDevice9 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hr;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return NULL;

    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    if (!d3d9_ptr)
    {
        skip("could not create D3D9\n");
        return NULL;
    }

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hr))
    {
        skip("could not create device, IDirect3D9_CreateDevice returned 0x%08x\n", hr);
        return NULL;
    }

    return device_ptr;
}

static void lock_flag_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DVertexBuffer9 *buffer;
    unsigned int i;
    void *data;
    const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT win7_result;
    }
    test_data[] =
    {
        {D3DLOCK_READONLY,                          "D3DLOCK_READONLY",                         D3D_OK             },
        {D3DLOCK_DISCARD,                           "D3DLOCK_DISCARD",                          D3D_OK             },
        {D3DLOCK_NOOVERWRITE,                       "D3DLOCK_NOOVERWRITE",                      D3D_OK             },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD,     "D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD",    D3D_OK             },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY,    "D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY",   D3D_OK             },
        {D3DLOCK_READONLY    | D3DLOCK_DISCARD,     "D3DLOCK_READONLY | D3DLOCK_DISCARD",       D3DERR_INVALIDCALL },
        /* Completely bogous flags aren't an error */
        {0xdeadbeef,                                "0xdeadbeef",                               D3DERR_INVALIDCALL },
    };

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 1024, D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &buffer, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateBuffer failed, 0x%08x\n", hr);

    for(i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, test_data[i].flags);
        /* Windows XP always returns D3D_OK even with flags that don't make sense. Windows 7 returns
         * an error. At least one game(Shaiya) depends on the Windows XP result, so mark the Windows 7
         * behavior as broken()
         */
        ok(hr == D3D_OK || broken(hr == test_data[i].win7_result), "Lock flags %s returned 0x%08x, expected D3D_OK\n",
            test_data[i].debug_string, hr);

        if(SUCCEEDED(hr))
        {
            ok(data != NULL, "The data pointer returned by Lock is NULL\n");
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer9_Unlock failed, 0x%08x\n", hr);
        }
    }

    IDirect3DVertexBuffer9_Release(buffer);
}

static inline const char *debug_d3dpool(D3DPOOL pool)
{
    switch(pool)
    {
        case D3DPOOL_DEFAULT: return "D3DPOOL_DEFAULT";
        case D3DPOOL_SYSTEMMEM: return "D3DPOOL_SYSTEMMEM";
        case D3DPOOL_SCRATCH: return "D3DPOOL_SCRATCH";
        case D3DPOOL_MANAGED: return "D3DPOOL_MANAGED";
        default:
        return "unknown pool";
    }
}

static void test_vertex_buffer_alignment(IDirect3DDevice9 *device)
{
    IDirect3DVertexBuffer9 *buffer = NULL;
    HRESULT hr;
    D3DPOOL pools[] = {D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEM, D3DPOOL_SCRATCH, D3DPOOL_MANAGED};
    DWORD sizes[] = {1, 4, 16, 17, 32, 33, 64, 65, 1024, 1025, 1048576, 1048577};
    unsigned int i, j;
    void *data;

    for(i = 0; i < (sizeof(sizes) / sizeof(sizes[0])); i++)
    {
        for(j = 0; j < (sizeof(pools) / sizeof(pools[0])); j++)
        {
            hr = IDirect3DDevice9_CreateVertexBuffer(device, sizes[i], 0, 0, pools[j], &buffer, NULL);
            if(pools[j] == D3DPOOL_SCRATCH)
            {
                ok(hr == D3DERR_INVALIDCALL, "Creating a D3DPOOL_SCRATCH buffer returned (0x%08x)\n", hr);
            }
            else
            {
                ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexBuffer failed (0x%08x). Pool = %s, size %d\n", hr,
                   debug_d3dpool(pools[j]), sizes[i]);
            }
            if(FAILED(hr)) continue;

            hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, 0);
            ok(SUCCEEDED(hr), "IDirect3DVertexBuffer9_Lock failed (0x%08x)\n", hr);
            ok(((DWORD_PTR) data & 31) == 0, "Vertex buffer start address is not 32 byte aligned(size: %d, pool: %s, data: %p)\n",
               sizes[i], debug_d3dpool(pools[j]), data);
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(SUCCEEDED(hr), "IDirect3DVertexBuffer9_Unlock failed (0x%08x)\n", hr);

            if(buffer) IDirect3DVertexBuffer9_Release(buffer);
        }
    }
}

START_TEST(buffer)
{
    IDirect3DDevice9 *device_ptr;
    ULONG refcount;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr) return;

    lock_flag_test(device_ptr);
    test_vertex_buffer_alignment(device_ptr);

    refcount = IDirect3DDevice9_Release(device_ptr);
    ok(!refcount, "Device has %u references left\n", refcount);
}
