/*
 * Copyright 2008 Luis Busquets
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

#include "wine/test.h"
#include "d3dx9.h"

static const DWORD simple_vs[] = {
    0xfffe0101,                                                             /* vs_1_1                       */
    0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position0 v0             */
    0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000,                         /* dp4 oPos.x, v0, c0           */
    0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001,                         /* dp4 oPos.y, v0, c1           */
    0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002,                         /* dp4 oPos.z, v0, c2           */
    0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003,                         /* dp4 oPos.w, v0, c3           */
    0x0000ffff};                                                            /* END                          */

static const DWORD simple_ps[] = {
    0xffff0101,                                                             /* ps_1_1                       */
    0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
    0x00000042, 0xb00f0000,                                                 /* tex t0                       */
    0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
    0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
    0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
    0x0000ffff};                                                            /* END                          */

#define FCC_TEXT MAKEFOURCC('T','E','X','T')
#define FCC_CTAB MAKEFOURCC('C','T','A','B')

static const DWORD shader_with_ctab[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0002fffe, FCC_TEXT,   0x00000000,                                     /* TEXT comment                 */
    0x0008fffe, FCC_CTAB,   0x0000001c, 0x00000010, 0xfffe0300, 0x00000000, /* CTAB comment                 */
                0x00000000, 0x00000000, 0x00000000,
    0x0004fffe, FCC_TEXT,   0x00000000, 0x00000000, 0x00000000,             /* TEXT comment                 */
    0x0000ffff};                                                            /* END                          */

static const DWORD shader_with_invalid_ctab[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x0005fffe, FCC_CTAB,                                                   /* CTAB comment                 */
                0x0000001c, 0x000000a9, 0xfffe0300,
                0x00000000, 0x00000000,
    0x0000ffff};                                                            /* END                          */

static const DWORD shader_with_ctab_constants[] = {
    0xfffe0300,                                                             /* vs_3_0                       */
    0x002efffe, FCC_CTAB,                                                   /* CTAB comment                 */
    0x0000001c, 0x000000a4, 0xfffe0300, 0x00000003, 0x0000001c, 0x20008100, /* Header                       */
    0x0000009c,
    0x00000058, 0x00070002, 0x00000001, 0x00000064, 0x00000000,             /* Constant 1 desc              */
    0x00000074, 0x00000002, 0x00000004, 0x00000080, 0x00000000,             /* Constant 2 desc              */
    0x00000090, 0x00040002, 0x00000003, 0x00000080, 0x00000000,             /* Constant 3 desc              */
    0x736e6f43, 0x746e6174, 0xabab0031,                                     /* Constant 1 name string       */
    0x00030001, 0x00040001, 0x00000001, 0x00000000,                         /* Constant 1 type desc         */
    0x736e6f43, 0x746e6174, 0xabab0032,                                     /* Constant 2 name string       */
    0x00030003, 0x00040004, 0x00000001, 0x00000000,                         /* Constant 2 & 3 type desc     */
    0x736e6f43, 0x746e6174, 0xabab0033,                                     /* Constant 3 name string       */
    0x335f7376, 0xab00305f,                                                 /* Target name string           */
    0x656e6957, 0x6f727020, 0x7463656a, 0xababab00,                         /* Creator name string          */
    0x0000ffff};                                                            /* END                          */

static void test_get_shader_size(void)
{
    UINT shader_size, expected;

    shader_size = D3DXGetShaderSize(simple_vs);
    expected = sizeof(simple_vs);
    ok(shader_size == expected, "Got shader size %u, expected %u\n", shader_size, expected);

    shader_size = D3DXGetShaderSize(simple_ps);
    expected = sizeof(simple_ps);
    ok(shader_size == expected, "Got shader size %u, expected %u\n", shader_size, expected);

    shader_size = D3DXGetShaderSize(NULL);
    ok(shader_size == 0, "Got shader size %u, expected 0\n", shader_size);
}

static void test_get_shader_version(void)
{
    DWORD shader_version;

    shader_version = D3DXGetShaderVersion(simple_vs);
    ok(shader_version == D3DVS_VERSION(1, 1), "Got shader version 0x%08x, expected 0x%08x\n",
            shader_version, D3DVS_VERSION(1, 1));

    shader_version = D3DXGetShaderVersion(simple_ps);
    ok(shader_version == D3DPS_VERSION(1, 1), "Got shader version 0x%08x, expected 0x%08x\n",
            shader_version, D3DPS_VERSION(1, 1));

    shader_version = D3DXGetShaderVersion(NULL);
    ok(shader_version == 0, "Got shader version 0x%08x, expected 0\n", shader_version);
}

static void test_find_shader_comment(void)
{
    HRESULT hr;
    LPCVOID data;
    UINT size;

    hr = D3DXFindShaderComment(NULL, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), NULL, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), &data, NULL);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    hr = D3DXFindShaderComment(shader_with_ctab, 0, &data, &size);
    ok(hr == S_FALSE, "Got result %x, expected 1 (S_FALSE)\n", hr);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('X','X','X','X'), &data, &size);
    ok(hr == S_FALSE, "Got result %x, expected 1 (S_FALSE)\n", hr);

    hr = D3DXFindShaderComment(shader_with_ctab, MAKEFOURCC('C','T','A','B'), &data, &size);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
    ok(data == (LPCVOID)(shader_with_ctab + 6), "Got result %p, expected %p\n", data, shader_with_ctab + 6);
    ok(size == 28, "Got result %d, expected 28\n", size);
}

static void test_get_shader_constant_table_ex(void)
{
    LPD3DXCONSTANTTABLE constant_table = NULL;
    HRESULT hr;
    LPVOID data;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;

    hr = D3DXGetShaderConstantTableEx(NULL, 0, &constant_table);
    ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    /* No CTAB data */
    hr = D3DXGetShaderConstantTableEx(simple_ps, 0, &constant_table);
    ok(hr == D3DXERR_INVALIDDATA, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DXERR_INVALIDDATA);

    /* With invalid CTAB data */
    hr = D3DXGetShaderConstantTableEx(shader_with_invalid_ctab, 0, &constant_table);
    ok(hr == D3DXERR_INVALIDDATA || broken(hr == D3D_OK), /* winxp 64-bit, w2k3 64-bit */
       "Got result %x, expected %x (D3DXERR_INVALIDDATA)\n", hr, D3DXERR_INVALIDDATA);
    if (constant_table) ID3DXConstantTable_Release(constant_table);

    hr = D3DXGetShaderConstantTableEx(shader_with_ctab, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    if (constant_table)
    {
        size = ID3DXConstantTable_GetBufferSize(constant_table);
        ok(size == 28, "Got result %x, expected 28\n", size);

        data = ID3DXConstantTable_GetBufferPointer(constant_table);
        ok(!memcmp(data, shader_with_ctab + 6, size), "Retrieved wrong CTAB data\n");

        hr = ID3DXConstantTable_GetDesc(constant_table, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

        hr = ID3DXConstantTable_GetDesc(constant_table, &desc);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(desc.Creator == (LPCSTR)data + 0x10, "Got result %p, expected %p\n", desc.Creator, (LPCSTR)data + 0x10);
        ok(desc.Version == D3DVS_VERSION(3, 0), "Got result %x, expected %x\n", desc.Version, D3DVS_VERSION(3, 0));
        ok(desc.Constants == 0, "Got result %x, expected 0\n", desc.Constants);

        ID3DXConstantTable_Release(constant_table);
    }

    hr = D3DXGetShaderConstantTableEx(shader_with_ctab_constants, 0, &constant_table);
    ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);

    if (SUCCEEDED(hr))
    {
        D3DXHANDLE constant;
        D3DXCONSTANT_DESC constant_desc;
        D3DXCONSTANT_DESC constant_desc_save;
        UINT nb;

        /* Test GetDesc */
        hr = ID3DXConstantTable_GetDesc(constant_table, &desc);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(desc.Creator, "Wine project"), "Got result '%s', expected 'Wine project'\n", desc.Creator);
        ok(desc.Version == D3DVS_VERSION(3, 0), "Got result %x, expected %x\n", desc.Version, D3DVS_VERSION(3, 0));
        ok(desc.Constants == 3, "Got result %x, expected 3\n", desc.Constants);

        /* Test GetConstant */
        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 0);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant1"), "Got result '%s', expected 'Constant1'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_VECTOR, "Got result %x, expected %u (D3DXPC_VECTOR)\n",
            constant_desc.Class, D3DXPC_VECTOR);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 1, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 1);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant2"), "Got result '%s', expected 'Constant2'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_MATRIX_COLUMNS, "Got result %x, expected %u (D3DXPC_MATRIX_COLUMNS)\n",
            constant_desc.Class, D3DXPC_MATRIX_COLUMNS);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 4, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 2);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!strcmp(constant_desc.Name, "Constant3"), "Got result '%s', expected 'Constant3'\n",
            constant_desc.Name);
        ok(constant_desc.Class == D3DXPC_MATRIX_COLUMNS, "Got result %x, expected %u (D3DXPC_MATRIX_COLUMNS)\n",
            constant_desc.Class, D3DXPC_MATRIX_COLUMNS);
        ok(constant_desc.Type == D3DXPT_FLOAT, "Got result %x, expected %u (D3DXPT_FLOAT)\n",
            constant_desc.Type, D3DXPT_FLOAT);
        ok(constant_desc.Rows == 4, "Got result %x, expected 1\n", constant_desc.Rows);
        ok(constant_desc.Columns == 4, "Got result %x, expected 4\n", constant_desc.Columns);
        constant_desc_save = constant_desc; /* For GetConstantDesc test */

        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 3);
        ok(constant == NULL, "Got result %p, expected NULL\n", constant);

        /* Test GetConstantByName */
        constant = ID3DXConstantTable_GetConstantByName(constant_table, NULL, "Constant unknown");
        ok(constant == NULL, "Got result %p, expected NULL\n", constant);
        constant = ID3DXConstantTable_GetConstantByName(constant_table, NULL, "Constant3");
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!memcmp(&constant_desc, &constant_desc_save, sizeof(D3DXCONSTANT_DESC)), "Got different constant data\n");

        /* Test GetConstantDesc */
        constant = ID3DXConstantTable_GetConstant(constant_table, NULL, 0);
        ok(constant != NULL, "No constant found\n");
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, NULL, &constant_desc, &nb);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, NULL, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, constant, &constant_desc, NULL);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, "Constant unknow", &constant_desc, &nb);
        ok(hr == D3DERR_INVALIDCALL, "Got result %x, expected %x (D3DERR_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);
        hr = ID3DXConstantTable_GetConstantDesc(constant_table, "Constant3", &constant_desc, &nb);
        ok(hr == D3D_OK, "Got result %x, expected 0 (D3D_OK)\n", hr);
        ok(!memcmp(&constant_desc, &constant_desc_save, sizeof(D3DXCONSTANT_DESC)), "Got different constant data\n");

        ID3DXConstantTable_Release(constant_table);
    }
}

START_TEST(shader)
{
    test_get_shader_size();
    test_get_shader_version();
    test_find_shader_comment();
    test_get_shader_constant_table_ex();
}
