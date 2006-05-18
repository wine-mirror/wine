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

    hres = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hres == D3D_OK, "IDirect3D_CreateDevice returned: 0x%lx\n", hres);

    return device_ptr;
}

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef(object);
    return IUnknown_Release(object);
}

static IDirect3DVertexDeclaration9 *test_create_vertex_declaration(IDirect3DDevice9 *device_ptr, D3DVERTEXELEMENT9 *vertex_decl)
{
    IDirect3DVertexDeclaration9 *decl_ptr = 0;
    HRESULT hret = 0;

    hret = IDirect3DDevice9_CreateVertexDeclaration(device_ptr, vertex_decl, &decl_ptr);
    ok(hret == D3D_OK && decl_ptr != NULL, "CreateVertexDeclaration returned: hret 0x%lx, decl_ptr %p. "
        "Expected hret 0x%lx, decl_ptr != %p. Aborting.\n", hret, decl_ptr, D3D_OK, NULL);

    return decl_ptr;
}

static void test_get_set_vertex_declaration(IDirect3DDevice9 *device_ptr, IDirect3DVertexDeclaration9 *decl_ptr)
{
    IDirect3DVertexDeclaration9 *current_decl_ptr = 0;
    HRESULT hret = 0;
    int decl_refcount = 0;
    int i = 0;
    
    /* SetVertexDeclaration should not touch the declaration's refcount. */
    i = get_refcount((IUnknown *)decl_ptr);
    hret = IDirect3DDevice9_SetVertexDeclaration(device_ptr, decl_ptr);
    decl_refcount = get_refcount((IUnknown *)decl_ptr);
    ok(hret == D3D_OK && decl_refcount == i, "SetVertexDeclaration returned: hret 0x%lx, refcount %d. "
        "Expected hret 0x%lx, refcount %d.\n", hret, decl_refcount, D3D_OK, i);
    
    /* GetVertexDeclaration should increase the declaration's refcount by one. */
    i = decl_refcount+1;
    hret = IDirect3DDevice9_GetVertexDeclaration(device_ptr, &current_decl_ptr);
    decl_refcount = get_refcount((IUnknown *)decl_ptr);
    ok(hret == D3D_OK && decl_refcount == i && current_decl_ptr == decl_ptr, 
        "GetVertexDeclaration returned: hret 0x%lx, current_decl_ptr %p refcount %d. "
        "Expected hret 0x%lx, current_decl_ptr %p, refcount %d.\n", hret, current_decl_ptr, decl_refcount, D3D_OK, decl_ptr, i);
}

static void test_get_declaration(IDirect3DVertexDeclaration9 *decl_ptr, D3DVERTEXELEMENT9 *vertex_decl, UINT expected_num_elements)
{
    int i;
    UINT num_elements = 0;
    D3DVERTEXELEMENT9 *decl = 0;
    HRESULT hret = 0;

    /* First test only getting the number of elements */
    num_elements = 0x1337c0de;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, NULL, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%lx, num_elements %d. "
            "Expected hret 0x%lx, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);

    num_elements = 0;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, NULL, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%lx, num_elements %d. "
            "Expected hret 0x%lx, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);

    /* Also test the returned data */
    decl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DVERTEXELEMENT9) * expected_num_elements);

    num_elements = 0x1337c0de;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, decl, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%lx, num_elements %d. "
            "Expected hret 0x%lx, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);
    i = memcmp(decl, vertex_decl, sizeof(vertex_decl));
    ok (!i, "Original and returned vertexdeclarations are not the same\n");
    ZeroMemory(decl, sizeof(D3DVERTEXELEMENT9) * expected_num_elements);

    num_elements = 0;
    hret = IDirect3DVertexDeclaration9_GetDeclaration(decl_ptr, decl, &num_elements);
    ok(hret == D3D_OK && num_elements == expected_num_elements,
            "GetDeclaration returned: hret 0x%lx, num_elements %d. "
            "Expected hret 0x%lx, num_elements %d.\n", hret, num_elements, D3D_OK, expected_num_elements);
    i = memcmp(decl, vertex_decl, sizeof(vertex_decl));
    ok (!i, "Original and returned vertexdeclarations are not the same\n");

    HeapFree(GetProcessHeap(), 0, decl);
}

START_TEST(vertexdeclaration)
{
    static D3DVERTEXELEMENT9 simple_decl[] = {
        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()};
    UINT simple_decl_num_elements = sizeof(simple_decl) / sizeof(*simple_decl);
    IDirect3DDevice9 *device_ptr = 0;
    IDirect3DVertexDeclaration9 *decl_ptr = 0;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        trace("Could not load d3d9.dll, skipping tests\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr)
    {
        trace("Failed to initialise d3d9, aborting test.\n");
        return;
    }

    decl_ptr = test_create_vertex_declaration(device_ptr, simple_decl);
    if (!decl_ptr)
    {
        trace("Failed to create a vertex declaration, aborting test.\n");
        return;
    }

    test_get_set_vertex_declaration(device_ptr, decl_ptr);

    test_get_declaration(decl_ptr, simple_decl, simple_decl_num_elements);
}
