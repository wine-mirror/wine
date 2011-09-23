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
    HRESULT hres;

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

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hres))
    {
        skip("could not create device, IDirect3D9_CreateDevice returned %#x\n", hres);
        return NULL;
    }

    return device_ptr;
}

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef(object);
    return IUnknown_Release(object);
}

static void test_get_set_vertex_shader(IDirect3DDevice9 *device_ptr)
{

    static DWORD simple_vs[] = {0xFFFE0101,             /* vs_1_1               */
        0x0000001F, 0x80000000, 0x900F0000,             /* dcl_position0 v0     */
        0x00000009, 0xC0010000, 0x90E40000, 0xA0E40000, /* dp4 oPos.x, v0, c0   */
        0x00000009, 0xC0020000, 0x90E40000, 0xA0E40001, /* dp4 oPos.y, v0, c1   */
        0x00000009, 0xC0040000, 0x90E40000, 0xA0E40002, /* dp4 oPos.z, v0, c2   */
        0x00000009, 0xC0080000, 0x90E40000, 0xA0E40003, /* dp4 oPos.w, v0, c3   */
        0x0000FFFF};                                    /* END                  */

    IDirect3DVertexShader9 *shader_ptr = 0;
    IDirect3DVertexShader9 *current_shader_ptr = 0;
    HRESULT hret = 0;
    int shader_refcount = 0;
    int i = 0;

    hret = IDirect3DDevice9_CreateVertexShader(device_ptr, simple_vs, &shader_ptr);
    ok(hret == D3D_OK && shader_ptr != NULL, "CreateVertexShader returned: hret 0x%x, shader_ptr %p. "
        "Expected hret 0x%x, shader_ptr != %p. Aborting.\n", hret, shader_ptr, D3D_OK, NULL);
    if (hret != D3D_OK || shader_ptr == NULL) return;

    /* SetVertexShader should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader_ptr);
    hret = IDirect3DDevice9_SetVertexShader(device_ptr, shader_ptr);
    shader_refcount = get_refcount((IUnknown *)shader_ptr);
    ok(hret == D3D_OK && shader_refcount == i, "SetVertexShader returned: hret 0x%x, refcount %d. "
        "Expected hret 0x%x, refcount %d.\n", hret, shader_refcount, D3D_OK, i);

    /* GetVertexShader should increase the shader's refcount by one. */
    i = shader_refcount+1;
    hret = IDirect3DDevice9_GetVertexShader(device_ptr, &current_shader_ptr);
    shader_refcount = get_refcount((IUnknown *)shader_ptr);
    ok(hret == D3D_OK && shader_refcount == i && current_shader_ptr == shader_ptr,
        "GetVertexShader returned: hret 0x%x, current_shader_ptr %p refcount %d. "
        "Expected hret 0x%x, current_shader_ptr %p, refcount %d.\n", hret, current_shader_ptr, shader_refcount, D3D_OK, shader_ptr, i);
    IDirect3DVertexShader9_Release(current_shader_ptr);

    IDirect3DVertexShader9_Release(shader_ptr);
}

static void test_vertex_shader_constant(IDirect3DDevice9 *device_ptr, DWORD consts)
{
    float c[4] = { 0.0, 0.0, 0.0, 0.0 };
    float d[16] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    HRESULT hr;

    /* A simple check that the stuff works at all */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, 0, c, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);

    /* Test corner cases: Write to const MAX - 1, MAX, MAX + 1, and writing 4 consts from
     * MAX - 1
     */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, consts - 1, c, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, consts + 0, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, consts + 1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);

    /* Constant -1 */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device_ptr, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);
}

static void test_get_set_pixel_shader(IDirect3DDevice9 *device_ptr)
{
    static DWORD simple_ps[] = {0xFFFF0101,                                     /* ps_1_1                       */
        0x00000051, 0xA00F0001, 0x3F800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
        0x00000042, 0xB00F0000,                                                 /* tex t0                       */
        0x00000008, 0x800F0000, 0xA0E40001, 0xA0E40000,                         /* dp3 r0, c1, c0               */
        0x00000005, 0x800F0000, 0x90E40000, 0x80E40000,                         /* mul r0, v0, r0               */
        0x00000005, 0x800F0000, 0xB0E40000, 0x80E40000,                         /* mul r0, t0, r0               */
        0x0000FFFF};                                                            /* END                          */

    IDirect3DPixelShader9 *shader_ptr = 0;
    IDirect3DPixelShader9 *current_shader_ptr = 0;
    HRESULT hret = 0;
    int shader_refcount = 0;
    int i = 0;

    hret = IDirect3DDevice9_CreatePixelShader(device_ptr, simple_ps, &shader_ptr);
    ok(hret == D3D_OK && shader_ptr != NULL, "CreatePixelShader returned: hret 0x%x, shader_ptr %p. "
        "Expected hret 0x%x, shader_ptr != %p. Aborting.\n", hret, shader_ptr, D3D_OK, NULL);
    if (hret != D3D_OK || shader_ptr == NULL) return;

    /* SetPixelsShader should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader_ptr);
    hret = IDirect3DDevice9_SetPixelShader(device_ptr, shader_ptr);
    shader_refcount = get_refcount((IUnknown *)shader_ptr);
    ok(hret == D3D_OK && shader_refcount == i, "SetPixelShader returned: hret 0x%x, refcount %d. "
        "Expected hret 0x%x, refcount %d.\n", hret, shader_refcount, D3D_OK, i);

    /* GetPixelShader should increase the shader's refcount by one. */
    i = shader_refcount+1;
    hret = IDirect3DDevice9_GetPixelShader(device_ptr, &current_shader_ptr);
    shader_refcount = get_refcount((IUnknown *)shader_ptr);
    ok(hret == D3D_OK && shader_refcount == i && current_shader_ptr == shader_ptr,
        "GetPixelShader returned: hret 0x%x, current_shader_ptr %p refcount %d. "
        "Expected hret 0x%x, current_shader_ptr %p, refcount %d.\n", hret, current_shader_ptr, shader_refcount, D3D_OK, shader_ptr, i);
    IDirect3DPixelShader9_Release(current_shader_ptr);

    IDirect3DPixelShader9_Release(shader_ptr);
}

static void test_pixel_shader_constant(IDirect3DDevice9 *device_ptr)
{
    float c[4] = { 0.0, 0.0, 0.0, 0.0 };
    float d[16] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    HRESULT hr;
    DWORD consts = 0;

    /* A simple check that the stuff works at all */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device_ptr, 0, c, 1);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetVertexShaderConstantF returned 0x%08x\n", hr);

    /* Is there really no max pixel shader constant value??? Test how far I can go */
    while(SUCCEEDED(IDirect3DDevice9_SetPixelShaderConstantF(device_ptr, consts++, c, 1)));
    consts = consts - 1;
    trace("SetPixelShaderConstantF was able to set %d shader constants\n", consts);

    /* Test corner cases: writing 4 consts from MAX - 1, everything else is pointless
     * given the way the constant limit was found out
     */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device_ptr, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetPixelShaderConstantF returned 0x%08x\n", hr);

    /* Constant -1 */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device_ptr, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetPixelShaderConstantF returned 0x%08x\n", hr);
}

static void test_wrong_shader(IDirect3DDevice9 *device_ptr)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                                     /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position0 v0     */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000, /* dp4 oPos.x, v0, c0   */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001, /* dp4 oPos.y, v0, c1   */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002, /* dp4 oPos.z, v0, c2   */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003, /* dp4 oPos.w, v0, c3   */
        0x0000ffff                                      /* END                  */
    };

    static const DWORD simple_ps[] =
    {
        0xffff0101,                                                             /* ps_1_1                       */
        0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
        0x00000042, 0xb00f0000,                                                 /* tex t0                       */
        0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
        0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
        0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
        0x0000ffff                                                              /* END                          */
    };

#if 0
float4 main(const float4 color : COLOR) : SV_TARGET
{
    float4 o;

    o = color;

    return o;
}
#endif
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac,
        0x00000005, 0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452,
        0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100,
        0x0000001c, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168,
        0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00,
        0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000,
        0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f, 0x0000002c,
        0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040,
        0x0000000e, 0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000,
        0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453,
        0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000,
    };

    IDirect3DVertexShader9 *vs;
    IDirect3DPixelShader9 *ps;
    HRESULT hret;

    hret = IDirect3DDevice9_CreateVertexShader(device_ptr, simple_ps, &vs);
    ok(hret == D3DERR_INVALIDCALL, "CreateVertexShader returned: hret 0x%x, shader_ptr %p.\n", hret, vs);
    hret = IDirect3DDevice9_CreatePixelShader(device_ptr, simple_vs, &ps);
    ok(hret == D3DERR_INVALIDCALL, "CreatePixelShader returned: hret 0x%x, shader_ptr %p.\n", hret, ps);

    hret = IDirect3DDevice9_CreatePixelShader(device_ptr, ps_4_0, &ps);
    ok(hret == D3DERR_INVALIDCALL, "CreatePixelShader returned: hret 0x%x, shader_ptr %p.\n", hret, ps);
}

START_TEST(shader)
{
    D3DCAPS9 caps;
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

    IDirect3DDevice9_GetDeviceCaps(device_ptr, &caps);

    if (caps.VertexShaderVersion & 0xffff)
    {
        test_get_set_vertex_shader(device_ptr);
        test_vertex_shader_constant(device_ptr, caps.MaxVertexShaderConst);
    }
    else skip("No vertex shader support\n");

    if (caps.PixelShaderVersion & 0xffff)
    {
        test_get_set_pixel_shader(device_ptr);
        /* No max pixel shader constant value??? */
        test_pixel_shader_constant(device_ptr);
        if (caps.VertexShaderVersion & 0xffff)
            test_wrong_shader(device_ptr);
    }
    else skip("No pixel shader support\n");

    refcount = IDirect3DDevice9_Release(device_ptr);
    ok(!refcount, "Device has %u references left\n", refcount);
}
