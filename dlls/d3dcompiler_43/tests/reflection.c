/*
 * Copyright 2010 Rico Schüller
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

/*
 * Nearly all compiler functions need the shader blob and the size. The size
 * is always located at DWORD #6 in the shader blob (blob[6]).
 * The functions are e.g.: D3DGet*SignatureBlob, D3DReflect
 */

#define COBJMACROS
#include "initguid.h"
#include "d3dcompiler.h"
#include "wine/test.h"

/* includes for older reflection interfaces */
#include "d3d10.h"
#include "d3d10_1shader.h"

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

/* Creator string for comparison - Version 9.29.952.3111 (43) */
static DWORD shader_creator[] = {
0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00,
};

/*
 * fxc.exe /E VS /Tvs_4_0 /Fx
 */
#if 0
float4 VS(float4 position : POSITION, float4 pos : SV_POSITION) : SV_POSITION
{
  return position;
}
#endif
static DWORD test_reflection_blob[] = {
0x43425844, 0x77c6324f, 0xfd27948a, 0xe6958d31, 0x53361cba, 0x00000001, 0x000001d8, 0x00000005,
0x00000034, 0x0000008c, 0x000000e4, 0x00000118, 0x0000015c, 0x46454452, 0x00000050, 0x00000000,
0x00000000, 0x00000000, 0x0000001c, 0xfffe0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000050, 0x00000002, 0x00000008, 0x00000038,
0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041, 0x00000000, 0x00000000,
0x00000003, 0x00000001, 0x0000000f, 0x49534f50, 0x4e4f4954, 0x5f565300, 0x49534f50, 0x4e4f4954,
0xababab00, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001,
0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49, 0x52444853, 0x0000003c,
0x00010040, 0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2, 0x00000000,
0x00000001, 0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453,
0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static void test_reflection_references(void)
{
    HRESULT hr;
    ULONG count;
    ID3D11ShaderReflection *ref11, *ref11_test;
    ID3D10ShaderReflection *ref10;
    ID3D10ShaderReflection1 *ref10_1;

    hr = D3DReflect(test_reflection_blob, test_reflection_blob[6], &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == S_OK, "D3DReflect failed, got %x, expected %x\n", hr, S_OK);

    hr = ref11->lpVtbl->QueryInterface(ref11, &IID_ID3D11ShaderReflection, (void **)&ref11_test);
    ok(hr == S_OK, "QueryInterface failed, got %x, expected %x\n", hr, S_OK);

    count = ref11_test->lpVtbl->Release(ref11_test);
    ok(count == 1, "Release failed %u\n", count);

    hr = ref11->lpVtbl->QueryInterface(ref11, &IID_ID3D10ShaderReflection, (void **)&ref10);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, got %x, expected %x\n", hr, E_NOINTERFACE);

    hr = ref11->lpVtbl->QueryInterface(ref11, &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == E_NOINTERFACE, "QueryInterface failed, got %x, expected %x\n", hr, E_NOINTERFACE);

    count = ref11->lpVtbl->Release(ref11);
    ok(count == 0, "Release failed %u\n", count);

    /* check invalid cases */
    hr = D3DReflect(test_reflection_blob, test_reflection_blob[6], &IID_ID3D10ShaderReflection, (void **)&ref10);
    ok(hr == E_NOINTERFACE, "D3DReflect failed, got %x, expected %x\n", hr, E_NOINTERFACE);

    hr = D3DReflect(test_reflection_blob, test_reflection_blob[6], &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == E_NOINTERFACE, "D3DReflect failed, got %x, expected %x\n", hr, E_NOINTERFACE);

    hr = D3DReflect(NULL, test_reflection_blob[6], &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == D3DERR_INVALIDCALL, "D3DReflect failed, got %x, expected %x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DReflect(NULL, test_reflection_blob[6], &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == D3DERR_INVALIDCALL, "D3DReflect failed, got %x, expected %x\n", hr, D3DERR_INVALIDCALL);

    /* returns different errors with different sizes */
    hr = D3DReflect(test_reflection_blob, 31, &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == D3DERR_INVALIDCALL, "D3DReflect failed, got %x, expected %x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DReflect(test_reflection_blob,  32, &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == E_FAIL, "D3DReflect failed, got %x, expected %x\n", hr, E_FAIL);

    hr = D3DReflect(test_reflection_blob, test_reflection_blob[6]-1, &IID_ID3D10ShaderReflection1, (void **)&ref10_1);
    ok(hr == E_FAIL, "D3DReflect failed, got %x, expected %x\n", hr, E_FAIL);

    hr = D3DReflect(test_reflection_blob,  31, &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == D3DERR_INVALIDCALL, "D3DReflect failed, got %x, expected %x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DReflect(test_reflection_blob,  32, &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == E_FAIL, "D3DReflect failed, got %x, expected %x\n", hr, E_FAIL);

    hr = D3DReflect(test_reflection_blob,  test_reflection_blob[6]-1, &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == E_FAIL, "D3DReflect failed, got %x, expected %x\n", hr, E_FAIL);
}

/*
 * fxc.exe /E VS /Tvs_4_1 /Fx
 */
#if 0
struct vsin
{
    float4 x : SV_position;
    float4 a : BINORMAL;
    uint b : BLENDINDICES;
    float c : BLENDWEIGHT;
    float4 d : COLOR;
    float4 d1 : COLOR1;
    float4 e : NORMAL;
    float4 f : POSITION;
    float4 g : POSITIONT;
    float h : PSIZE;
    float4 i : TANGENT;
    float4 j : TEXCOORD;
    uint k : SV_VertexID;
    uint l : SV_InstanceID;
    float m : testin;
};
struct vsout
{
    float4 x : SV_position;
    float4 a : COLOR0;
    float b : FOG;
    float4 c : POSITION0;
    float d : PSIZE;
    float e : TESSFACTOR0;
    float4 f : TEXCOORD0;
    float g : SV_ClipDistance0;
    float h : SV_CullDistance0;
    uint i : SV_InstanceID;
    float j : testout;
};
vsout VS(vsin x)
{
    vsout s;
    s.x = float4(1.6f, 0.3f, 0.1f, 0.0f);
    int y = 1;
    int p[5] = {1, 2, 3, 5, 4};
    y = y << (int) x.x.x & 0xf;
    s.x.x = p[y];
    s.a = x.d;
    s.b = x.c;
    s.c = x.f;
    s.d = x.h;
    s.e = x.h;
    s.f = x.j;
    s.g = 1.0f;
    s.h = 1.0f;
    s.i = 2;
    s.j = x.m;
    return s;
}
#endif
static DWORD test_reflection_desc_vs_blob[] = {
0x43425844, 0xb65955ac, 0xcea1cb75, 0x06c5a1ad, 0x8a555fa1, 0x00000001, 0x0000076c, 0x00000005,
0x00000034, 0x0000008c, 0x0000028c, 0x00000414, 0x000006f0, 0x46454452, 0x00000050, 0x00000000,
0x00000000, 0x00000000, 0x0000001c, 0xfffe0401, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x000001f8, 0x0000000f, 0x00000008, 0x00000170,
0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x0000010f, 0x0000017c, 0x00000000, 0x00000000,
0x00000003, 0x00000001, 0x0000000f, 0x00000185, 0x00000000, 0x00000000, 0x00000001, 0x00000002,
0x00000001, 0x00000192, 0x00000000, 0x00000000, 0x00000003, 0x00000003, 0x00000101, 0x0000019e,
0x00000000, 0x00000000, 0x00000003, 0x00000004, 0x00000f0f, 0x0000019e, 0x00000001, 0x00000000,
0x00000003, 0x00000005, 0x0000000f, 0x000001a4, 0x00000000, 0x00000000, 0x00000003, 0x00000006,
0x0000000f, 0x000001ab, 0x00000000, 0x00000000, 0x00000003, 0x00000007, 0x00000f0f, 0x000001b4,
0x00000000, 0x00000000, 0x00000003, 0x00000008, 0x0000000f, 0x000001be, 0x00000000, 0x00000000,
0x00000003, 0x00000009, 0x00000101, 0x000001c4, 0x00000000, 0x00000000, 0x00000003, 0x0000000a,
0x0000000f, 0x000001cc, 0x00000000, 0x00000000, 0x00000003, 0x0000000b, 0x00000f0f, 0x000001d5,
0x00000000, 0x00000006, 0x00000001, 0x0000000c, 0x00000001, 0x000001e1, 0x00000000, 0x00000008,
0x00000001, 0x0000000d, 0x00000001, 0x000001ef, 0x00000000, 0x00000000, 0x00000003, 0x0000000e,
0x00000101, 0x705f5653, 0x7469736f, 0x006e6f69, 0x4f4e4942, 0x4c414d52, 0x454c4200, 0x4e49444e,
0x45434944, 0x4c420053, 0x57444e45, 0x48474945, 0x4f430054, 0x00524f4c, 0x4d524f4e, 0x50004c41,
0x5449534f, 0x004e4f49, 0x49534f50, 0x4e4f4954, 0x53500054, 0x00455a49, 0x474e4154, 0x00544e45,
0x43584554, 0x44524f4f, 0x5f565300, 0x74726556, 0x44497865, 0x5f565300, 0x74736e49, 0x65636e61,
0x74004449, 0x69747365, 0xabab006e, 0x4e47534f, 0x00000180, 0x0000000b, 0x00000008, 0x00000110,
0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x0000011c, 0x00000000, 0x00000000,
0x00000003, 0x00000001, 0x0000000f, 0x00000122, 0x00000000, 0x00000000, 0x00000003, 0x00000002,
0x00000e01, 0x00000126, 0x00000000, 0x00000000, 0x00000003, 0x00000002, 0x00000d02, 0x0000012c,
0x00000000, 0x00000000, 0x00000003, 0x00000002, 0x00000b04, 0x00000137, 0x00000000, 0x00000000,
0x00000003, 0x00000002, 0x00000708, 0x0000013f, 0x00000000, 0x00000000, 0x00000003, 0x00000003,
0x0000000f, 0x00000148, 0x00000000, 0x00000000, 0x00000003, 0x00000004, 0x0000000f, 0x00000151,
0x00000000, 0x00000002, 0x00000003, 0x00000005, 0x00000e01, 0x00000161, 0x00000000, 0x00000003,
0x00000003, 0x00000005, 0x00000d02, 0x00000171, 0x00000000, 0x00000000, 0x00000001, 0x00000006,
0x00000e01, 0x705f5653, 0x7469736f, 0x006e6f69, 0x4f4c4f43, 0x4f460052, 0x53500047, 0x00455a49,
0x53534554, 0x54434146, 0x7400524f, 0x6f747365, 0x50007475, 0x5449534f, 0x004e4f49, 0x43584554,
0x44524f4f, 0x5f565300, 0x70696c43, 0x74736944, 0x65636e61, 0x5f565300, 0x6c6c7543, 0x74736944,
0x65636e61, 0x5f565300, 0x74736e49, 0x65636e61, 0xab004449, 0x52444853, 0x000002d4, 0x00010041,
0x000000b5, 0x0100086a, 0x0300005f, 0x00101012, 0x00000000, 0x0300005f, 0x00101012, 0x00000003,
0x0300005f, 0x001010f2, 0x00000004, 0x0300005f, 0x001010f2, 0x00000007, 0x0300005f, 0x00101012,
0x00000009, 0x0300005f, 0x001010f2, 0x0000000b, 0x0300005f, 0x00101012, 0x0000000e, 0x04000067,
0x001020f2, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000001, 0x03000065, 0x00102012,
0x00000002, 0x03000065, 0x00102022, 0x00000002, 0x03000065, 0x00102042, 0x00000002, 0x03000065,
0x00102082, 0x00000002, 0x03000065, 0x001020f2, 0x00000003, 0x03000065, 0x001020f2, 0x00000004,
0x04000067, 0x00102012, 0x00000005, 0x00000002, 0x04000067, 0x00102022, 0x00000005, 0x00000003,
0x03000065, 0x00102012, 0x00000006, 0x02000068, 0x00000001, 0x04000069, 0x00000000, 0x00000005,
0x00000004, 0x06000036, 0x00203012, 0x00000000, 0x00000000, 0x00004001, 0x00000001, 0x06000036,
0x00203012, 0x00000000, 0x00000001, 0x00004001, 0x00000002, 0x06000036, 0x00203012, 0x00000000,
0x00000002, 0x00004001, 0x00000003, 0x06000036, 0x00203012, 0x00000000, 0x00000003, 0x00004001,
0x00000005, 0x06000036, 0x00203012, 0x00000000, 0x00000004, 0x00004001, 0x00000004, 0x0500001b,
0x00100012, 0x00000000, 0x0010100a, 0x00000000, 0x07000029, 0x00100012, 0x00000000, 0x00004001,
0x00000001, 0x0010000a, 0x00000000, 0x07000001, 0x00100012, 0x00000000, 0x0010000a, 0x00000000,
0x00004001, 0x0000000f, 0x07000036, 0x00100012, 0x00000000, 0x0420300a, 0x00000000, 0x0010000a,
0x00000000, 0x0500002b, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x08000036, 0x001020e2,
0x00000000, 0x00004002, 0x00000000, 0x3e99999a, 0x3dcccccd, 0x00000000, 0x05000036, 0x001020f2,
0x00000001, 0x00101e46, 0x00000004, 0x05000036, 0x00102012, 0x00000002, 0x0010100a, 0x00000003,
0x05000036, 0x00102062, 0x00000002, 0x00101006, 0x00000009, 0x05000036, 0x00102082, 0x00000002,
0x0010100a, 0x0000000e, 0x05000036, 0x001020f2, 0x00000003, 0x00101e46, 0x00000007, 0x05000036,
0x001020f2, 0x00000004, 0x00101e46, 0x0000000b, 0x05000036, 0x00102012, 0x00000005, 0x00004001,
0x3f800000, 0x05000036, 0x00102022, 0x00000005, 0x00004001, 0x3f800000, 0x05000036, 0x00102012,
0x00000006, 0x00004001, 0x00000002, 0x0100003e, 0x54415453, 0x00000074, 0x00000015, 0x00000001,
0x00000000, 0x00000012, 0x00000000, 0x00000001, 0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000005, 0x00000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0000000a, 0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000,
};

static const D3D11_SIGNATURE_PARAMETER_DESC test_reflection_desc_vs_resultin[] =
{
    {"SV_position", 0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x1, 0},
    {"BINORMAL", 0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"BLENDINDICES", 0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0x0, 0},
    {"BLENDWEIGHT", 0, 3, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0x1, 0},
    {"COLOR", 0, 4, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf, 0},
    {"COLOR", 1, 5, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"NORMAL", 0, 6, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"POSITION", 0, 7, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf, 0},
    {"POSITIONT", 0, 8, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"PSIZE", 0, 9, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0x1, 0},
    {"TANGENT", 0, 10, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"TEXCOORD", 0, 11, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf, 0},
    {"SV_VertexID", 0, 12, D3D_NAME_VERTEX_ID, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0x0, 0},
    {"SV_InstanceID", 0, 13, D3D_NAME_INSTANCE_ID, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0x0, 0},
    {"testin", 0, 14, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0x1, 0},
};

static const D3D11_SIGNATURE_PARAMETER_DESC test_reflection_desc_vs_resultout[] =
{
    {"SV_position", 0, 0, D3D_NAME_POSITION, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"COLOR", 0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"FOG", 0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"PSIZE", 0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x2, 0xd, 0},
    {"TESSFACTOR", 0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x4, 0xb, 0},
    {"testout", 0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x8, 0x7, 0},
    {"POSITION", 0, 3, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"TEXCOORD", 0, 4, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"SV_ClipDistance", 0, 5, D3D_NAME_CLIP_DISTANCE, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_CullDistance", 0, 5, D3D_NAME_CULL_DISTANCE, D3D_REGISTER_COMPONENT_FLOAT32, 0x2, 0xd, 0},
    {"SV_InstanceID", 0, 6, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0xe, 0},
};

static void test_reflection_desc_vs(void)
{
    HRESULT hr;
    ULONG count;
    ID3D11ShaderReflection *ref11;
    D3D11_SHADER_DESC sdesc11 = {0};
    D3D11_SIGNATURE_PARAMETER_DESC desc = {0};
    const D3D11_SIGNATURE_PARAMETER_DESC *pdesc;
    UINT ret;
    unsigned int i;

    hr = D3DReflect(test_reflection_desc_vs_blob, test_reflection_desc_vs_blob[6], &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == S_OK, "D3DReflect failed %x\n", hr);

    hr = ref11->lpVtbl->GetDesc(ref11, NULL);
    ok(hr == E_FAIL, "GetDesc failed %x\n", hr);

    hr = ref11->lpVtbl->GetDesc(ref11, &sdesc11);
    ok(hr == S_OK, "GetDesc failed %x\n", hr);

    ok(sdesc11.Version == 65601, "GetDesc failed, got %u, expected %u\n", sdesc11.Version, 65601);
    ok(strcmp(sdesc11.Creator, (char*) shader_creator) == 0, "GetDesc failed, got \"%s\", expected \"%s\"\n", sdesc11.Creator, (char*)shader_creator);
    ok(sdesc11.Flags == 256, "GetDesc failed, got %u, expected %u\n", sdesc11.Flags, 256);
    ok(sdesc11.ConstantBuffers == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.ConstantBuffers, 0);
    ok(sdesc11.BoundResources == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.BoundResources, 0);
    ok(sdesc11.InputParameters == 15, "GetDesc failed, got %u, expected %u\n", sdesc11.InputParameters, 15);
    ok(sdesc11.OutputParameters == 11, "GetDesc failed, got %u, expected %u\n", sdesc11.OutputParameters, 11);
    ok(sdesc11.InstructionCount == 21, "GetDesc failed, got %u, expected %u\n", sdesc11.InstructionCount, 21);
    ok(sdesc11.TempRegisterCount == 1, "GetDesc failed, got %u, expected %u\n", sdesc11.TempRegisterCount, 1);
    ok(sdesc11.TempArrayCount == 5, "GetDesc failed, got %u, expected %u\n", sdesc11.TempArrayCount, 5);
    ok(sdesc11.DefCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.DefCount, 0);
    ok(sdesc11.DclCount == 18, "GetDesc failed, got %u, expected %u\n", sdesc11.DclCount, 18);
    ok(sdesc11.TextureNormalInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureNormalInstructions, 0);
    ok(sdesc11.TextureLoadInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureLoadInstructions, 0);
    ok(sdesc11.TextureCompInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureCompInstructions, 0);
    ok(sdesc11.TextureBiasInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureBiasInstructions, 0);
    ok(sdesc11.TextureGradientInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureGradientInstructions, 0);
    ok(sdesc11.FloatInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.FloatInstructionCount, 0);
    ok(sdesc11.IntInstructionCount == 1, "GetDesc failed, got %u, expected %u\n", sdesc11.IntInstructionCount, 1);
    ok(sdesc11.UintInstructionCount == 1, "GetDesc failed, got %u, expected %u\n", sdesc11.UintInstructionCount, 1);
    ok(sdesc11.StaticFlowControlCount == 1, "GetDesc failed, got %u, expected %u\n", sdesc11.StaticFlowControlCount, 1);
    ok(sdesc11.DynamicFlowControlCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.DynamicFlowControlCount, 0);
    ok(sdesc11.MacroInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.MacroInstructionCount, 0);
    ok(sdesc11.ArrayInstructionCount == 6, "GetDesc failed, got %u, expected %u\n", sdesc11.ArrayInstructionCount, 6);
    ok(sdesc11.CutInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.CutInstructionCount, 0);
    ok(sdesc11.EmitInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.EmitInstructionCount, 0);
    ok(sdesc11.GSOutputTopology == 0, "GetDesc failed, got %x, expected %x\n", sdesc11.GSOutputTopology, 0);
    ok(sdesc11.GSMaxOutputVertexCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.GSMaxOutputVertexCount, 0);
    ok(sdesc11.InputPrimitive == 0, "GetDesc failed, got %x, expected %x\n", sdesc11.InputPrimitive, 0);
    ok(sdesc11.PatchConstantParameters == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.PatchConstantParameters, 0);
    ok(sdesc11.cGSInstanceCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.cGSInstanceCount, 0);
    ok(sdesc11.cControlPoints == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.cControlPoints, 0);
    ok(sdesc11.HSOutputPrimitive == 0, "GetDesc failed, got %x, expected %x\n", sdesc11.HSOutputPrimitive, 0);
    ok(sdesc11.HSPartitioning == 0, "GetDesc failed, got %x, expected %x\n", sdesc11.HSPartitioning, 0);
    ok(sdesc11.TessellatorDomain == 0, "GetDesc failed, got %x, expected %x\n", sdesc11.TessellatorDomain, 0);
    ok(sdesc11.cBarrierInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.cBarrierInstructions, 0);
    ok(sdesc11.cInterlockedInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.cInterlockedInstructions, 0);
    ok(sdesc11.cTextureStoreInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.cTextureStoreInstructions, 0);

    ret = ref11->lpVtbl->GetBitwiseInstructionCount(ref11);
    ok(ret == 0, "GetBitwiseInstructionCount failed, got %u, expected %u\n", ret, 0);

    ret = ref11->lpVtbl->GetConversionInstructionCount(ref11);
    ok(ret == 2, "GetConversionInstructionCount failed, got %u, expected %u\n", ret, 2);

    ret = ref11->lpVtbl->GetMovInstructionCount(ref11);
    ok(ret == 10, "GetMovInstructionCount failed, got %u, expected %u\n", ret, 10);

    ret = ref11->lpVtbl->GetMovcInstructionCount(ref11);
    ok(ret == 0, "GetMovcInstructionCount failed, got %u, expected %u\n", ret, 0);

    /* GetIn/OutputParameterDesc */
    for (i = 0; i < sizeof(test_reflection_desc_vs_resultin)/sizeof(*test_reflection_desc_vs_resultin); ++i)
    {
        pdesc = &test_reflection_desc_vs_resultin[i];

        hr = ref11->lpVtbl->GetInputParameterDesc(ref11, i, &desc);
        ok(hr == S_OK, "GetInputParameterDesc(%u) failed, got %x, expected %x\n", i, hr, S_OK);

        ok(!strcmp(desc.SemanticName, pdesc->SemanticName), "GetInputParameterDesc(%u) SemanticName failed, got \"%s\", expected \"%s\"\n",
                i, desc.SemanticName, pdesc->SemanticName);
        ok(desc.SemanticIndex == pdesc->SemanticIndex, "GetInputParameterDesc(%u) SemanticIndex failed, got %u, expected %u\n",
                i, desc.SemanticIndex, pdesc->SemanticIndex);
        ok(desc.Register == pdesc->Register, "GetInputParameterDesc(%u) Register failed, got %u, expected %u\n",
                i, desc.Register, pdesc->Register);
        ok(desc.SystemValueType == pdesc->SystemValueType, "GetInputParameterDesc(%u) SystemValueType failed, got %x, expected %x\n",
                i, desc.SystemValueType, pdesc->SystemValueType);
        ok(desc.ComponentType == pdesc->ComponentType, "GetInputParameterDesc(%u) ComponentType failed, got %x, expected %x\n",
                i, desc.ComponentType, pdesc->ComponentType);
        ok(desc.Mask == pdesc->Mask, "GetInputParameterDesc(%u) Mask failed, got %x, expected %x\n",
                i, desc.Mask, pdesc->Mask);
        ok(desc.ReadWriteMask == pdesc->ReadWriteMask, "GetInputParameterDesc(%u) ReadWriteMask failed, got %x, expected %x\n",
                i, desc.ReadWriteMask, pdesc->ReadWriteMask);
        ok(desc.Stream == pdesc->Stream, "GetInputParameterDesc(%u) Stream failed, got %u, expected %u\n",
                i, desc.Stream, pdesc->ReadWriteMask);
    }

    for (i = 0; i < sizeof(test_reflection_desc_vs_resultout)/sizeof(*test_reflection_desc_vs_resultout); ++i)
    {
        pdesc = &test_reflection_desc_vs_resultout[i];

        hr = ref11->lpVtbl->GetOutputParameterDesc(ref11, i, &desc);
        ok(hr == S_OK, "GetOutputParameterDesc(%u) failed, got %x, expected %x\n", i, hr, S_OK);

        ok(!strcmp(desc.SemanticName, pdesc->SemanticName), "GetOutputParameterDesc(%u) SemanticName failed, got \"%s\", expected \"%s\"\n",
                i, desc.SemanticName, pdesc->SemanticName);
        ok(desc.SemanticIndex == pdesc->SemanticIndex, "GetOutputParameterDesc(%u) SemanticIndex failed, got %u, expected %u\n",
                i, desc.SemanticIndex, pdesc->SemanticIndex);
        ok(desc.Register == pdesc->Register, "GetOutputParameterDesc(%u) Register failed, got %u, expected %u\n",
                i, desc.Register, pdesc->Register);
        ok(desc.SystemValueType == pdesc->SystemValueType, "GetOutputParameterDesc(%u) SystemValueType failed, got %x, expected %x\n",
                i, desc.SystemValueType, pdesc->SystemValueType);
        ok(desc.ComponentType == pdesc->ComponentType, "GetOutputParameterDesc(%u) ComponentType failed, got %x, expected %x\n",
                i, desc.ComponentType, pdesc->ComponentType);
        ok(desc.Mask == pdesc->Mask, "GetOutputParameterDesc(%u) Mask failed, got %x, expected %x\n",
                i, desc.Mask, pdesc->Mask);
        ok(desc.ReadWriteMask == pdesc->ReadWriteMask, "GetOutputParameterDesc(%u) ReadWriteMask failed, got %x, expected %x\n",
                i, desc.ReadWriteMask, pdesc->ReadWriteMask);
        ok(desc.Stream == pdesc->Stream, "GetOutputParameterDesc(%u) Stream failed, got %u, expected %u\n",
                i, desc.Stream, pdesc->ReadWriteMask);
    }

    count = ref11->lpVtbl->Release(ref11);
    ok(count == 0, "Release failed %u\n", count);
}

START_TEST(reflection)
{
    test_reflection_references();
    test_reflection_desc_vs();
}
