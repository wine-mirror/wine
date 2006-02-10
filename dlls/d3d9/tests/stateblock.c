/*
 * Copyright (C) 2005 Henri Verbeet
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <d3d9.h>
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = &DefWindowProc;
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
    HRESULT hres;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return NULL;
    
    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hres == D3D_OK, "IDirect3D_CreateDevice returned: 0x%lx\n", hres);

    return device_ptr;
}

static void test_begin_end_state_block(IDirect3DDevice9 *device_ptr)
{
    HRESULT hret = 0;
    IDirect3DStateBlock9 *state_block_ptr = 0;

    /* Should succeed */
    hret = IDirect3DDevice9_BeginStateBlock(device_ptr);
    ok(hret == D3D_OK, "BeginStateBlock returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) return;

    /* Calling BeginStateBlock while recording should return D3DERR_INVALIDCALL */
    hret = IDirect3DDevice9_BeginStateBlock(device_ptr);
    ok(hret == D3DERR_INVALIDCALL, "BeginStateBlock returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3DERR_INVALIDCALL);
    if (hret != D3DERR_INVALIDCALL) return;

    /* Should succeed */
    state_block_ptr = (IDirect3DStateBlock9 *)0xdeadbeef;
    hret = IDirect3DDevice9_EndStateBlock(device_ptr, &state_block_ptr);
    ok(hret == D3D_OK && state_block_ptr != 0 && state_block_ptr != (IDirect3DStateBlock9 *)0xdeadbeef, 
        "EndStateBlock returned: hret 0x%lx, state_block_ptr %p. "
        "Expected hret 0x%lx, state_block_ptr != %p, state_block_ptr != 0xdeadbeef.\n", hret, state_block_ptr, D3D_OK, NULL);

    /* Calling EndStateBlock while not recording should return D3DERR_INVALIDCALL. state_block_ptr should not be touched. */
    state_block_ptr = (IDirect3DStateBlock9 *)0xdeadbeef;
    hret = IDirect3DDevice9_EndStateBlock(device_ptr, &state_block_ptr);
    ok(hret == D3DERR_INVALIDCALL && state_block_ptr == (IDirect3DStateBlock9 *)0xdeadbeef, 
        "EndStateBlock returned: hret 0x%lx, state_block_ptr %p. "
        "Expected hret 0x%lx, state_block_ptr 0xdeadbeef.\n", hret, state_block_ptr, D3DERR_INVALIDCALL);
}

START_TEST(stateblock)
{
    IDirect3DDevice9 *device_ptr;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        trace("Could not load d3d9.dll, skipping tests\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr) return;

    test_begin_end_state_block(device_ptr);
}
