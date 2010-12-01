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

static const DWORD ctab_basic[] = {                  /* 0123456789ABCDEF */
    0xfffe0200, 0x0052fffe, 0x42415443, 0x0000001c,  /* ......R.CTAB.... */
    0x00000113, 0xfffe0200, 0x00000006, 0x0000001c,  /* ................ */
    0x00000100, 0x0000010c, 0x00000094, 0x00080002,  /* ................ */
    0x00000001, 0x00000098, 0x00000000, 0x000000a8,  /* ................ */
    0x00060002, 0x00000001, 0x000000ac, 0x00000000,  /* ................ */
    0x000000bc, 0x00070002, 0x00000001, 0x000000c0,  /* ................ */
    0x00000000, 0x000000d0, 0x00040002, 0x00000001,  /* ................ */
    0x000000d4, 0x00000000, 0x000000e4, 0x00050002,  /* ................ */
    0x00000001, 0x000000e8, 0x00000000, 0x000000f8,  /* ................ */
    0x00000002, 0x00000004, 0x000000fc, 0x00000000,  /* ................ */
    0xabab0062, 0x00010000, 0x00010001, 0x00000001,  /* b............... */
    0x00000000, 0xabab0066, 0x00030000, 0x00010001,  /* ....f........... */
    0x00000001, 0x00000000, 0xab003466, 0x00030001,  /* ........f4...... */
    0x00040001, 0x00000001, 0x00000000, 0xabab0069,  /* ............i... */
    0x00020000, 0x00010001, 0x00000001, 0x00000000,  /* ................ */
    0xab003469, 0x00020001, 0x00040001, 0x00000001,  /* i4.............. */
    0x00000000, 0x0070766d, 0x00030003, 0x00040004,  /* ....mvp......... */
    0x00000001, 0x00000000, 0x325f7376, 0x4d00305f,  /* ........vs_2_0.M */
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,  /* icrosoft.(R).HLS */
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,  /* L.Shader.Compile */
    0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,  /* r.9.29.952.3111. */
    0x0000ffff};                                     /* END              */

static const D3DXCONSTANT_DESC ctab_basic_expected[] = {
    {"mvp", D3DXRS_FLOAT4, 0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, 0},
    {"i", D3DXRS_FLOAT4, 4, 1, D3DXPC_SCALAR, D3DXPT_INT, 1, 1, 1, 0, 4, 0},
    {"i4", D3DXRS_FLOAT4, 5, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 4, 1, 0, 16, 0},
    {"f", D3DXRS_FLOAT4, 6, 1, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 1, 0, 4, 0},
    {"f4", D3DXRS_FLOAT4, 7, 1, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 1, 0, 16, 0}};

static const DWORD ctab_matrices[] = {               /* 0123456789ABCDEF */
    0xfffe0300, 0x003afffe, 0x42415443, 0x0000001c,  /* ......:.CTAB.... */
    0x000000b3, 0xfffe0300, 0x00000003, 0x0000001c,  /* ................ */
    0x00000100, 0x000000ac, 0x00000058, 0x00070002,  /* ........X....... */
    0x00000001, 0x00000064, 0x00000000, 0x00000074,  /* ....d.......t... */
    0x00000002, 0x00000004, 0x00000080, 0x00000000,  /* ................ */
    0x00000090, 0x00040002, 0x00000003, 0x0000009c,  /* ................ */
    0x00000000, 0x74616d66, 0x33786972, 0xab003178,  /* ....fmatrix3x1.. */
    0x00030003, 0x00010003, 0x00000001, 0x00000000,  /* ................ */
    0x74616d66, 0x34786972, 0xab003478, 0x00030003,  /* fmatrix4x4...... */
    0x00040004, 0x00000001, 0x00000000, 0x74616d69,  /* ............imat */
    0x32786972, 0xab003378, 0x00020003, 0x00030002,  /* rix2x3.......... */
    0x00000001, 0x00000000, 0x335f7376, 0x4d00305f,  /* ........vs_3_0.M */
    0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,  /* icrosoft.(R).HLS */
    0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970,  /* L.Shader.Compile */
    0x2e392072, 0x392e3932, 0x332e3235, 0x00313131,  /* r.9.29.952.3111. */
    0x0000ffff};                                     /* END              */

static const D3DXCONSTANT_DESC ctab_matrices_expected[] = {
    {"fmatrix4x4", D3DXRS_FLOAT4, 0, 4, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 1, 0, 64, 0},
    {"imatrix2x3", D3DXRS_FLOAT4, 4, 3, D3DXPC_MATRIX_COLUMNS, D3DXPT_INT, 2, 3, 1, 0, 24, 0},
    {"fmatrix3x1", D3DXRS_FLOAT4, 7, 1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 3, 1, 1, 0, 12, 0}};

static const DWORD ctab_arrays[] = {                 /* 0123456789ABCDEF */
    0xfffe0200, 0x005cfffe, 0x42415443, 0x0000001c,  /* ......\.CTAB.... */
    0x0000013b, 0xfffe0200, 0x00000006, 0x0000001c,  /* ;............... */
    0x00000100, 0x00000134, 0x00000094, 0x000e0002,  /* ....4........... */
    0x00000002, 0x0000009c, 0x00000000, 0x000000ac,  /* ................ */
    0x00100002, 0x00000002, 0x000000b8, 0x00000000,  /* ................ */
    0x000000c8, 0x00080002, 0x00000004, 0x000000d0,  /* ................ */
    0x00000000, 0x000000e0, 0x00000002, 0x00000008,  /* ................ */
    0x000000ec, 0x00000000, 0x000000fc, 0x000c0002,  /* ................ */
    0x00000002, 0x00000108, 0x00000000, 0x00000118,  /* ................ */
    0x00120002, 0x00000001, 0x00000124, 0x00000000,  /* ........$....... */
    0x72726162, 0xab007961, 0x00010000, 0x00010001,  /* barray.......... */
    0x00000002, 0x00000000, 0x63657662, 0x61727261,  /* ........bvecarra */
    0xabab0079, 0x00010001, 0x00030001, 0x00000003,  /* y............... */
    0x00000000, 0x72726166, 0xab007961, 0x00030000,  /* ....farray...... */
    0x00010001, 0x00000004, 0x00000000, 0x78746d66,  /* ............fmtx */
    0x61727261, 0xabab0079, 0x00030003, 0x00040004,  /* array........... */
    0x00000002, 0x00000000, 0x63657666, 0x61727261,  /* ........fvecarra */
    0xabab0079, 0x00030001, 0x00040001, 0x00000002,  /* y............... */
    0x00000000, 0x63657669, 0x61727261, 0xabab0079,  /* ....ivecarray... */
    0x00020001, 0x00040001, 0x00000001, 0x00000000,  /* ................ */
    0x325f7376, 0x4d00305f, 0x6f726369, 0x74666f73,  /* vs_2_0.Microsoft */
    0x29522820, 0x534c4820, 0x6853204c, 0x72656461,  /* .(R).HLSL.Shader */
    0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932,  /* .Compiler.9.29.9 */
    0x332e3235, 0x00313131, 0x0000ffff};             /* 52.3111.  END    */

static const D3DXCONSTANT_DESC ctab_arrays_expected[] = {
    {"fmtxarray", D3DXRS_FLOAT4, 0, 8, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4, 2, 0, 128, 0},
    {"farray", D3DXRS_FLOAT4, 8, 4, D3DXPC_SCALAR, D3DXPT_FLOAT, 1, 1, 4, 0, 16, 0},
    {"fvecarray", D3DXRS_FLOAT4, 12, 2, D3DXPC_VECTOR, D3DXPT_FLOAT, 1, 4, 2, 0, 32, 0},
    {"barray", D3DXRS_FLOAT4, 14, 2, D3DXPC_SCALAR, D3DXPT_BOOL, 1, 1, 2, 0, 8, 0},
    {"bvecarray", D3DXRS_FLOAT4, 16, 2, D3DXPC_VECTOR, D3DXPT_BOOL, 1, 3, 3, 0, 36, 0},
    {"ivecarray", D3DXRS_FLOAT4, 18, 1, D3DXPC_VECTOR, D3DXPT_INT, 1, 4, 1, 0, 16, 0}};

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

static void test_constant_table(const char *test_name, const DWORD *ctable_fn,
        const D3DXCONSTANT_DESC *expecteds, UINT count)
{
    UINT i;
    ID3DXConstantTable *ctable;

    HRESULT res;

    /* Get the constant table from the shader itself */
    res = D3DXGetShaderConstantTable(ctable_fn, &ctable);
    ok(res == D3D_OK, "D3DXGetShaderConstantTable failed on %p: got %08x\n", test_name, res);

    for (i = 0; i < count; i++)
    {
        const D3DXCONSTANT_DESC *expected = &expecteds[i];
        D3DXHANDLE const_handle;
        D3DXCONSTANT_DESC actual;
        UINT pCount = 1;

        const_handle = ID3DXConstantTable_GetConstantByName(ctable, NULL, expected->Name);

        ID3DXConstantTable_GetConstantDesc(ctable, const_handle, &actual, &pCount);
        ok(pCount == 1, "Got more or less descriptions: %d\n", pCount);

        ok(strcmp(actual.Name, expected->Name) == 0,
           "%s in %s: Got different names: Got %s, expected %s\n", expected->Name,
           test_name, actual.Name, expected->Name);
        ok(actual.RegisterSet == expected->RegisterSet,
           "%s in %s: Got different register sets: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterSet, expected->RegisterSet);
        ok(actual.RegisterIndex == expected->RegisterIndex,
           "%s in %s: Got different register indices: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterIndex, expected->RegisterIndex);
        ok(actual.RegisterCount == expected->RegisterCount,
           "%s in %s: Got different register counts: Got %d, expected %d\n",
           expected->Name, test_name, actual.RegisterCount, expected->RegisterCount);
        ok(actual.Class == expected->Class,
           "%s in %s: Got different classes: Got %d, expected %d\n", expected->Name,
           test_name, actual.Class, expected->Class);
        ok(actual.Type == expected->Type,
           "%s in %s: Got different types: Got %d, expected %d\n", expected->Name,
           test_name, actual.Type, expected->Type);
        ok(actual.Rows == expected->Rows && actual.Columns == expected->Columns,
           "%s in %s: Got different dimensions: Got (%d, %d), expected (%d, %d)\n",
           expected->Name, test_name, actual.Rows, actual.Columns, expected->Rows,
           expected->Columns);
        ok(actual.Elements == expected->Elements,
           "%s in %s: Got different element count: Got %d, expected %d\n",
           expected->Name, test_name, actual.Elements, expected->Elements);
        ok(actual.StructMembers == expected->StructMembers,
           "%s in %s: Got different struct member count: Got %d, expected %d\n",
           expected->Name, test_name, actual.StructMembers, expected->StructMembers);
        ok(actual.Bytes == expected->Bytes,
           "%s in %s: Got different byte count: Got %d, expected %d\n",
           expected->Name, test_name, actual.Bytes, expected->Bytes);
    }

    /* Finally, release the constant table */
    ID3DXConstantTable_Release(ctable);
}

static void test_constant_tables(void)
{
    test_constant_table("test_basic", ctab_basic, ctab_basic_expected,
            sizeof(ctab_basic_expected)/sizeof(*ctab_basic_expected));
    test_constant_table("test_matrices", ctab_matrices, ctab_matrices_expected,
            sizeof(ctab_matrices_expected)/sizeof(*ctab_matrices_expected));
    test_constant_table("test_arrays", ctab_arrays, ctab_arrays_expected,
            sizeof(ctab_arrays_expected)/sizeof(*ctab_arrays_expected));
}

START_TEST(shader)
{
    test_get_shader_size();
    test_get_shader_version();
    test_find_shader_comment();
    test_get_shader_constant_table_ex();
    test_constant_tables();
}
