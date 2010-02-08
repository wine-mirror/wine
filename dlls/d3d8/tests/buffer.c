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

    hr = IDirect3D8_CreateDevice(d3d8_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hr))
    {
        skip("could not create device, IDirect3D8_CreateDevice returned %#x\n", hr);
        return NULL;
    }

    return device_ptr;
}

static void lock_flag_test(IDirect3DDevice8 *device)
{
    HRESULT hr;
    IDirect3DVertexBuffer8 *buffer;
    unsigned int i;
    BYTE *data;
    const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT result;
    }
    test_data[] =
    {
        {D3DLOCK_READONLY,                          "D3DLOCK_READONLY",                         D3D_OK             },
        {D3DLOCK_DISCARD,                           "D3DLOCK_DISCARD",                          D3D_OK             },
        {D3DLOCK_NOOVERWRITE,                       "D3DLOCK_NOOVERWRITE",                      D3D_OK             },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD,     "D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD",    D3D_OK             },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY,    "D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY",   D3D_OK             },
        {D3DLOCK_READONLY    | D3DLOCK_DISCARD,     "D3DLOCK_READONLY | D3DLOCK_DISCARD",       D3D_OK             },
        /* Completely bogous flags aren't an error */
        {0xdeadbeef,                                "0xdeadbeef",                               D3D_OK             },
    };

    hr = IDirect3DDevice8_CreateVertexBuffer(device, 1024, D3DUSAGE_DYNAMIC, 0, D3DPOOL_DEFAULT, &buffer);
    ok(hr == D3D_OK, "IDirect3DDevice8_CreateBuffer failed, 0x%08x\n", hr);

    for(i = 0; i < (sizeof(test_data) / sizeof(*test_data)); i++)
    {
        hr = IDirect3DVertexBuffer8_Lock(buffer, 0, 0, &data, test_data[i].flags);
        ok(hr == test_data[i].result, "Lock flags %s returned 0x%08x, expected 0x%08x\n",
            test_data[i].debug_string, hr, test_data[i].result);

        if(SUCCEEDED(hr))
        {
            ok(data != NULL, "The data pointer returned by Lock is NULL\n");
            hr = IDirect3DVertexBuffer8_Unlock(buffer);
            ok(hr == D3D_OK, "IDirect3DVertexBuffer8_Unlock failed, 0x%08x\n", hr);
        }
    }

    IDirect3DVertexBuffer8_Release(buffer);
}

START_TEST(buffer)
{
    IDirect3DDevice8 *device_ptr;
    ULONG refcount;
    HMODULE d3d8_handle = 0;

    d3d8_handle = LoadLibraryA("d3d8.dll");
    if (!d3d8_handle)
    {
        skip("Could not load d3d8.dll\n");
        return;
    }

    device_ptr = init_d3d8(d3d8_handle);
    if (!device_ptr) return;

    lock_flag_test(device_ptr);

    refcount = IDirect3DDevice8_Release(device_ptr);
    ok(!refcount, "Device has %u references left\n", refcount);
}
