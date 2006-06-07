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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
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

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, NULL,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
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

static void test_missing_light_recording(IDirect3DDevice9 *device_ptr)
{

    D3DLIGHT9 garbage_light = { 1, 
                                { 2.0, 2.0, 2.0, 2.0 }, { 3.0, 3.0, 3.0, 3.0 }, { 4.0, 4.0, 4.0, 4.0 }, 
                                { 5.0, 5.0, 5.0 }, { 6.0, 6.0, 6.0 }, 
                                7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0 };

    D3DLIGHT9 expected_light = { D3DLIGHT_DIRECTIONAL, 
                                { 1.0, 1.0, 1.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0, 0.0 },
                                { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 1.0 }, 
                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    D3DLIGHT9 result_light;
    BOOL result_light_enable;

    HRESULT hret = 0;
    IDirect3DStateBlock9 *state_block_ptr = 0;

    /* Before we make any changes, GetLightEnable should fail, and GetLight should fail */
    hret = IDirect3DDevice9_GetLightEnable(device_ptr, 0, &result_light_enable);
    ok(hret == D3DERR_INVALIDCALL, "GetLightEnable returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", 
        hret, D3DERR_INVALIDCALL);
    if (hret != D3DERR_INVALIDCALL) goto cleanup;

    hret = IDirect3DDevice9_GetLight(device_ptr, 0, &result_light);
    ok(hret == D3DERR_INVALIDCALL, "GetLight returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", 
        hret, D3DERR_INVALIDCALL);
    if (hret != D3DERR_INVALIDCALL) goto cleanup;

    /* Set states to record with garbage data (light, light_enable) */
    hret = IDirect3DDevice9_BeginStateBlock(device_ptr);
    ok(hret == D3D_OK, "BeginStateBlock returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    hret = IDirect3DDevice9_SetLight(device_ptr, 0, &garbage_light);
    ok(hret == D3D_OK, "SetLight returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    hret = IDirect3DDevice9_LightEnable(device_ptr, 0, TRUE);
    ok(hret == D3D_OK, "SetLightEnable returned: hret 0x%lx, Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    state_block_ptr = (IDirect3DStateBlock9 *)0xdeadbeef;
    hret = IDirect3DDevice9_EndStateBlock(device_ptr, &state_block_ptr);
    ok(hret == D3D_OK, "EndStateBlock returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup; 

    /* Now capture light data */
    hret = IDirect3DStateBlock9_Capture(state_block_ptr);
    ok(hret == D3D_OK, "Capture returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    /* Now apply the resultant stateblock */
    hret = IDirect3DStateBlock9_Apply(state_block_ptr);
    ok(hret == D3D_OK, "Apply returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    /* Now try to fetch the light set by the stateblock.
     * Light enable should have been set to 0, and a default light should have been
     * created as a side effect of that */
    hret = IDirect3DDevice9_GetLightEnable(device_ptr, 0, &result_light_enable);
    ok(hret == D3D_OK, "GetLightEnable returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;
    ok(result_light_enable == 0, "Light enabled status was %u, instead of 0", result_light_enable);

    hret = IDirect3DDevice9_GetLight(device_ptr, 0, &result_light);
    ok(hret == D3D_OK, "GetLight returned: hret 0x%lx. Expected hret 0x%lx. Aborting.\n", hret, D3D_OK);
    if (hret != D3D_OK) goto cleanup;

    ok (!memcmp(&expected_light, &result_light, sizeof(D3DLIGHT9)), 
        "Light returned = { %u, { %f, %f, %f, %f }, { %f, %f, %f, %f}, { %f, %f, %f, %f }, "
                         "{ %f, %f, %f }, { %f, %f, %f }, %f, %f, %f, %f, %f, %f, %f }\n",

         result_light.Type, 
         result_light.Diffuse.r, result_light.Diffuse.g, result_light.Diffuse.b, result_light.Diffuse.a,
         result_light.Specular.r, result_light.Specular.g, result_light.Specular.b, result_light.Specular.a,
         result_light.Ambient.r, result_light.Ambient.g, result_light.Ambient.b, result_light.Ambient.a,
         result_light.Position.x, result_light.Position.y, result_light.Position.z,
         result_light.Direction.x, result_light.Direction.y, result_light.Direction.z, 
         result_light.Range, result_light.Falloff,
         result_light.Attenuation0, result_light.Attenuation1, result_light.Attenuation2,
         result_light.Theta, result_light.Phi);

    cleanup:
    if (state_block_ptr) IUnknown_Release( state_block_ptr );
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
    test_missing_light_recording(device_ptr);
}
