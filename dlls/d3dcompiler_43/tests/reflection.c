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

/*
 * fxc.exe /E PS /Tps_4_1 /Fx
 */
#if 0
Texture2D tex1;
Texture2D tex2;
SamplerState sam
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};
SamplerComparisonState samc
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = w1;
    AddressV = Wrap;
    ComparisonFunc = LESS;
};
struct psin
{
    uint f : SV_RenderTargetArrayIndex;
    uint g : SV_InstanceID;
    uint h : SV_PrimitiveID;
    float2 uv : TEXCOORD;
    float4 a : COLOR3;
    float b : VFACE;
    float4 c : SV_position;
    bool d : SV_Coverage;
    bool e : SV_IsFrontFace;
};
struct psout
{
    float a : SV_Target1;
    float b : SV_Depth;
    float x : SV_Target;
    bool c : SV_Coverage;
};
psout PS(psin p)
{
    psout a;
    float4 x = tex1.Sample(sam, p.uv);
    x += tex1.SampleCmp(samc, p.uv, 0.3f);
    if (x.y < 0.1f)
        x += tex2.SampleCmp(samc, p.uv, 0.4f);
    else if (x.y < 0.2f)
        x += tex2.SampleCmp(samc, p.uv, 0.1f);
    else if (x.y < 0.3f)
        x += tex2.SampleBias(sam, p.uv, 0.1f);
    else if (x.y < 0.4f)
        x += tex2.SampleBias(sam, p.uv, 0.2f);
    else if (x.y < 0.5f)
        x += tex2.SampleBias(sam, p.uv, 0.3f);
    else
        x += tex2.SampleBias(sam, p.uv, 0.4f);
    x += tex2.SampleGrad(sam, p.uv, x.xy, x.xy);
    x += tex2.SampleGrad(sam, p.uv, x.xz, x.xz);
    x += tex2.SampleGrad(sam, p.uv, x.xz, x.zy);
    x += tex2.SampleGrad(sam, p.uv, x.xz, x.zw);
    x += tex2.SampleGrad(sam, p.uv, x.xz, x.wz);
    a.a = x.y;
    a.b = x.x;
    a.x = x.x;
    a.c = true;
    return a;
}
#endif
static DWORD test_reflection_desc_ps_blob[] = {
0x43425844, 0x19e2f325, 0xf1ec39a3, 0x3c5a8b53, 0x5bd5fb65, 0x00000001, 0x000008d0, 0x00000005,
0x00000034, 0x0000011c, 0x00000254, 0x000002e4, 0x00000854, 0x46454452, 0x000000e0, 0x00000000,
0x00000000, 0x00000004, 0x0000001c, 0xffff0401, 0x00000100, 0x000000af, 0x0000009c, 0x00000003,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x000000a0, 0x00000003,
0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000002, 0x000000a5, 0x00000002,
0x00000005, 0x00000004, 0xffffffff, 0x00000000, 0x00000001, 0x0000000c, 0x000000aa, 0x00000002,
0x00000005, 0x00000004, 0xffffffff, 0x00000001, 0x00000001, 0x0000000c, 0x006d6173, 0x636d6173,
0x78657400, 0x65740031, 0x4d003278, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x4e475349,
0x00000130, 0x00000008, 0x00000008, 0x000000c8, 0x00000000, 0x00000004, 0x00000001, 0x00000000,
0x00000001, 0x000000e2, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000002, 0x000000f0,
0x00000000, 0x00000007, 0x00000001, 0x00000000, 0x00000004, 0x000000ff, 0x00000000, 0x00000009,
0x00000001, 0x00000000, 0x00000008, 0x0000010e, 0x00000000, 0x00000000, 0x00000003, 0x00000001,
0x00000303, 0x00000117, 0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x00000004, 0x0000011d,
0x00000003, 0x00000000, 0x00000003, 0x00000002, 0x0000000f, 0x00000123, 0x00000000, 0x00000001,
0x00000003, 0x00000003, 0x0000000f, 0x525f5653, 0x65646e65, 0x72615472, 0x41746567, 0x79617272,
0x65646e49, 0x56530078, 0x736e495f, 0x636e6174, 0x00444965, 0x505f5653, 0x696d6972, 0x65766974,
0x53004449, 0x73495f56, 0x6e6f7246, 0x63614674, 0x45540065, 0x4f4f4358, 0x56004452, 0x45434146,
0x4c4f4300, 0x5300524f, 0x6f705f56, 0x69746973, 0xab006e6f, 0x4e47534f, 0x00000088, 0x00000004,
0x00000008, 0x00000068, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000e01, 0x00000068,
0x00000001, 0x00000000, 0x00000003, 0x00000001, 0x00000e01, 0x00000072, 0x00000000, 0x00000000,
0x00000001, 0xffffffff, 0x00000e01, 0x0000007e, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
0x00000e01, 0x545f5653, 0x65677261, 0x56530074, 0x766f435f, 0x67617265, 0x56530065, 0x7065445f,
0xab006874, 0x52444853, 0x00000568, 0x00000041, 0x0000015a, 0x0100086a, 0x0300005a, 0x00106000,
0x00000000, 0x0300085a, 0x00106000, 0x00000001, 0x04001858, 0x00107000, 0x00000000, 0x00005555,
0x04001858, 0x00107000, 0x00000001, 0x00005555, 0x03001062, 0x00101032, 0x00000001, 0x03000065,
0x00102012, 0x00000000, 0x03000065, 0x00102012, 0x00000001, 0x02000065, 0x0000f000, 0x02000065,
0x0000c001, 0x02000068, 0x00000003, 0x09000045, 0x001000f2, 0x00000000, 0x00101046, 0x00000001,
0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0b000046, 0x00100012, 0x00000001, 0x00101046,
0x00000001, 0x00107006, 0x00000000, 0x00106000, 0x00000001, 0x00004001, 0x3e99999a, 0x07000000,
0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00100006, 0x00000001, 0x07000031, 0x00100012,
0x00000001, 0x0010001a, 0x00000000, 0x00004001, 0x3dcccccd, 0x0304001f, 0x0010000a, 0x00000001,
0x0b000046, 0x00100012, 0x00000001, 0x00101046, 0x00000001, 0x00107006, 0x00000001, 0x00106000,
0x00000001, 0x00004001, 0x3ecccccd, 0x07000000, 0x001000f2, 0x00000001, 0x00100e46, 0x00000000,
0x00100006, 0x00000001, 0x01000012, 0x07000031, 0x00100012, 0x00000002, 0x0010001a, 0x00000000,
0x00004001, 0x3e4ccccd, 0x0304001f, 0x0010000a, 0x00000002, 0x0b000046, 0x00100012, 0x00000002,
0x00101046, 0x00000001, 0x00107006, 0x00000001, 0x00106000, 0x00000001, 0x00004001, 0x3dcccccd,
0x07000000, 0x001000f2, 0x00000001, 0x00100e46, 0x00000000, 0x00100006, 0x00000002, 0x01000012,
0x07000031, 0x00100012, 0x00000002, 0x0010001a, 0x00000000, 0x00004001, 0x3e99999a, 0x0304001f,
0x0010000a, 0x00000002, 0x0b00004a, 0x001000f2, 0x00000002, 0x00101046, 0x00000001, 0x00107e46,
0x00000001, 0x00106000, 0x00000000, 0x00004001, 0x3dcccccd, 0x07000000, 0x001000f2, 0x00000001,
0x00100e46, 0x00000000, 0x00100e46, 0x00000002, 0x01000012, 0x07000031, 0x00100012, 0x00000002,
0x0010001a, 0x00000000, 0x00004001, 0x3ecccccd, 0x0304001f, 0x0010000a, 0x00000002, 0x0b00004a,
0x001000f2, 0x00000002, 0x00101046, 0x00000001, 0x00107e46, 0x00000001, 0x00106000, 0x00000000,
0x00004001, 0x3e4ccccd, 0x07000000, 0x001000f2, 0x00000001, 0x00100e46, 0x00000000, 0x00100e46,
0x00000002, 0x01000012, 0x07000031, 0x00100012, 0x00000002, 0x0010001a, 0x00000000, 0x00004001,
0x3f000000, 0x0304001f, 0x0010000a, 0x00000002, 0x0b00004a, 0x001000f2, 0x00000002, 0x00101046,
0x00000001, 0x00107e46, 0x00000001, 0x00106000, 0x00000000, 0x00004001, 0x3e99999a, 0x07000000,
0x001000f2, 0x00000001, 0x00100e46, 0x00000000, 0x00100e46, 0x00000002, 0x01000012, 0x0b00004a,
0x001000f2, 0x00000002, 0x00101046, 0x00000001, 0x00107e46, 0x00000001, 0x00106000, 0x00000000,
0x00004001, 0x3ecccccd, 0x07000000, 0x001000f2, 0x00000001, 0x00100e46, 0x00000000, 0x00100e46,
0x00000002, 0x01000015, 0x01000015, 0x01000015, 0x01000015, 0x01000015, 0x0d000049, 0x001000f2,
0x00000000, 0x00101046, 0x00000001, 0x00107e46, 0x00000001, 0x00106000, 0x00000000, 0x00100046,
0x00000001, 0x00100046, 0x00000001, 0x07000000, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
0x00100e46, 0x00000001, 0x0d000049, 0x001000f2, 0x00000001, 0x00101046, 0x00000001, 0x00107e46,
0x00000001, 0x00106000, 0x00000000, 0x00100086, 0x00000000, 0x00100086, 0x00000000, 0x07000000,
0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00100e46, 0x00000001, 0x0d000049, 0x001000f2,
0x00000001, 0x00101046, 0x00000001, 0x00107e46, 0x00000001, 0x00106000, 0x00000000, 0x00100086,
0x00000000, 0x00100a66, 0x00000000, 0x07000000, 0x001000f2, 0x00000000, 0x00100e46, 0x00000000,
0x00100e46, 0x00000001, 0x0d000049, 0x001000f2, 0x00000001, 0x00101046, 0x00000001, 0x00107e46,
0x00000001, 0x00106000, 0x00000000, 0x00100086, 0x00000000, 0x00100ae6, 0x00000000, 0x07000000,
0x001000f2, 0x00000000, 0x00100e46, 0x00000000, 0x00100e46, 0x00000001, 0x0d000049, 0x001000c2,
0x00000000, 0x00101046, 0x00000001, 0x001074e6, 0x00000001, 0x00106000, 0x00000000, 0x00100086,
0x00000000, 0x00100fb6, 0x00000000, 0x07000000, 0x00100032, 0x00000000, 0x00100ae6, 0x00000000,
0x00100046, 0x00000000, 0x05000036, 0x00102012, 0x00000001, 0x0010001a, 0x00000000, 0x04000036,
0x0000c001, 0x0010000a, 0x00000000, 0x05000036, 0x00102012, 0x00000000, 0x0010000a, 0x00000000,
0x04000036, 0x0000f001, 0x00004001, 0xffffffff, 0x0100003e, 0x54415453, 0x00000074, 0x00000032,
0x00000003, 0x00000000, 0x00000005, 0x00000011, 0x00000000, 0x00000000, 0x00000006, 0x00000005,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000003,
0x00000004, 0x00000005, 0x00000018, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const D3D11_SIGNATURE_PARAMETER_DESC test_reflection_desc_ps_resultin[] =
{
    {"SV_RenderTargetArrayIndex", 0, 0, D3D_NAME_RENDER_TARGET_ARRAY_INDEX, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0x0, 0},
    {"SV_InstanceID", 0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x2, 0x0, 0},
    {"SV_PrimitiveID", 0, 0, D3D_NAME_PRIMITIVE_ID, D3D_REGISTER_COMPONENT_UINT32, 0x4, 0x0, 0},
    {"SV_IsFrontFace", 0, 0, D3D_NAME_IS_FRONT_FACE, D3D_REGISTER_COMPONENT_UINT32, 0x8, 0x0, 0},
    {"TEXCOORD", 0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x3, 0x3, 0},
    {"VFACE", 0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x4, 0x0, 0},
    {"COLOR", 3, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
    {"SV_position", 0, 3, D3D_NAME_POSITION, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x0, 0},
};

static const D3D11_SIGNATURE_PARAMETER_DESC test_reflection_desc_ps_resultout[] =
{
    {"SV_Target", 0, 0, D3D_NAME_TARGET, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_Target", 1, 1, D3D_NAME_TARGET, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_Coverage", 0, 0xffffffff, D3D_NAME_COVERAGE, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0xe, 0},
    {"SV_Depth", 0, 0xffffffff, D3D_NAME_DEPTH, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
};

static void test_reflection_desc_ps(void)
{
    HRESULT hr;
    ULONG count;
    ID3D11ShaderReflection *ref11;
    D3D11_SHADER_DESC sdesc11 = {0};
    D3D11_SIGNATURE_PARAMETER_DESC desc = {0};
    const D3D11_SIGNATURE_PARAMETER_DESC *pdesc;
    UINT ret;
    unsigned int i;

    hr = D3DReflect(test_reflection_desc_ps_blob, test_reflection_desc_ps_blob[6], &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == S_OK, "D3DReflect failed %x\n", hr);

    hr = ref11->lpVtbl->GetDesc(ref11, &sdesc11);
    ok(hr == S_OK, "GetDesc failed %x\n", hr);

    ok(sdesc11.Version == 65, "GetDesc failed, got %u, expected %u\n", sdesc11.Version, 65);
    ok(strcmp(sdesc11.Creator, (char*) shader_creator) == 0, "GetDesc failed, got \"%s\", expected \"%s\"\n", sdesc11.Creator, (char*)shader_creator);
    ok(sdesc11.Flags == 256, "GetDesc failed, got %u, expected %u\n", sdesc11.Flags, 256);
    ok(sdesc11.ConstantBuffers == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.ConstantBuffers, 0);
    ok(sdesc11.BoundResources == 4, "GetDesc failed, got %u, expected %u\n", sdesc11.BoundResources, 4);
    ok(sdesc11.InputParameters == 8, "GetDesc failed, got %u, expected %u\n", sdesc11.InputParameters, 8);
    ok(sdesc11.OutputParameters == 4, "GetDesc failed, got %u, expected %u\n", sdesc11.OutputParameters, 4);
    ok(sdesc11.InstructionCount == 50, "GetDesc failed, got %u, expected %u\n", sdesc11.InstructionCount, 50);
    ok(sdesc11.TempRegisterCount == 3, "GetDesc failed, got %u, expected %u\n", sdesc11.TempRegisterCount, 3);
    ok(sdesc11.TempArrayCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TempArrayCount, 0);
    ok(sdesc11.DefCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.DefCount, 0);
    ok(sdesc11.DclCount == 5, "GetDesc failed, got %u, expected %u\n", sdesc11.DclCount, 5);
    ok(sdesc11.TextureNormalInstructions == 1, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureNormalInstructions, 1);
    ok(sdesc11.TextureLoadInstructions == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureLoadInstructions, 0);
    ok(sdesc11.TextureCompInstructions == 3, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureCompInstructions, 3);
    ok(sdesc11.TextureBiasInstructions == 4, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureBiasInstructions, 4);
    ok(sdesc11.TextureGradientInstructions == 5, "GetDesc failed, got %u, expected %u\n", sdesc11.TextureGradientInstructions, 5);
    ok(sdesc11.FloatInstructionCount == 17, "GetDesc failed, got %u, expected %u\n", sdesc11.FloatInstructionCount, 17);
    ok(sdesc11.IntInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.IntInstructionCount, 0);
    ok(sdesc11.UintInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.UintInstructionCount, 0);
    ok(sdesc11.StaticFlowControlCount == 6, "GetDesc failed, got %u, expected %u\n", sdesc11.StaticFlowControlCount, 6);
    ok(sdesc11.DynamicFlowControlCount == 5, "GetDesc failed, got %u, expected %u\n", sdesc11.DynamicFlowControlCount, 5);
    ok(sdesc11.MacroInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.MacroInstructionCount, 0);
    ok(sdesc11.ArrayInstructionCount == 0, "GetDesc failed, got %u, expected %u\n", sdesc11.ArrayInstructionCount, 0);
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
    ok(ret == 0, "GetConversionInstructionCount failed, got %u, expected %u\n", ret, 0);

    ret = ref11->lpVtbl->GetMovInstructionCount(ref11);
    ok(ret == 24, "GetMovInstructionCount failed, got %u, expected %u\n", ret, 24);

    ret = ref11->lpVtbl->GetMovcInstructionCount(ref11);
    ok(ret == 0, "GetMovcInstructionCount failed, got %u, expected %u\n", ret, 0);

    /* check invalid Get*ParameterDesc cases*/
    hr = ref11->lpVtbl->GetInputParameterDesc(ref11, 0, NULL);
    ok(hr == E_INVALIDARG, "GetInputParameterDesc failed, got %x, expected %x\n", hr, E_INVALIDARG);

    hr = ref11->lpVtbl->GetInputParameterDesc(ref11, 0xffffffff, &desc);
    ok(hr == E_INVALIDARG, "GetInputParameterDesc failed, got %x, expected %x\n", hr, E_INVALIDARG);

    hr = ref11->lpVtbl->GetOutputParameterDesc(ref11, 0, NULL);
    ok(hr == E_INVALIDARG, "GetOutputParameterDesc failed, got %x, expected %x\n", hr, E_INVALIDARG);

    hr = ref11->lpVtbl->GetOutputParameterDesc(ref11, 0xffffffff, &desc);
    ok(hr == E_INVALIDARG, "GetOutputParameterDesc failed, got %x, expected %x\n", hr, E_INVALIDARG);

    hr = ref11->lpVtbl->GetPatchConstantParameterDesc(ref11, 0, &desc);
    ok(hr == E_INVALIDARG, "GetPatchConstantParameterDesc failed, got %x, expected %x\n", hr, E_INVALIDARG);

    /* GetIn/OutputParameterDesc */
    for (i = 0; i < sizeof(test_reflection_desc_ps_resultin)/sizeof(*test_reflection_desc_ps_resultin); ++i)
    {
        pdesc = &test_reflection_desc_ps_resultin[i];

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

    for (i = 0; i < sizeof(test_reflection_desc_ps_resultout)/sizeof(*test_reflection_desc_ps_resultout); ++i)
    {
        pdesc = &test_reflection_desc_ps_resultout[i];

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

/*
 * fxc.exe /E PS /Tps_5_0 /Fx
 */
#if 0
float4 PS() : SV_Target3
{
    float4 a = float4(1.2f, 1.0f, 0.2f, 0.0f);
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_0[] = {
0x43425844, 0x3e7b77e6, 0xe4e920b7, 0x9cad0533, 0x240117cc, 0x00000001, 0x0000018c, 0x00000005,
0x00000034, 0x0000008c, 0x0000009c, 0x000000d0, 0x00000110, 0x46454452, 0x00000050, 0x00000000,
0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000003, 0x00000000, 0x00000003, 0x00000003,
0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
0x03000065, 0x001020f2, 0x00000003, 0x08000036, 0x001020f2, 0x00000003, 0x00004002, 0x3f99999a,
0x3f800000, 0x3e4ccccd, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000
};

/*
 * fxc.exe /E PS /Tps_5_0 /Fx
 */
#if 0
float PS() : SV_DepthLessEqual
{
    float a = 1.2f;
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_1[] = {
0x43425844, 0xd8ead3ec, 0x61276ada, 0x70cdaa9e, 0x2cfd7f4c, 0x00000001, 0x000001c4, 0x00000005,
0x00000034, 0x000000ac, 0x000000bc, 0x000000f8, 0x00000128, 0x46454452, 0x00000070, 0x00000000,
0x00000000, 0x00000000, 0x0000003c, 0xffff0500, 0x00000100, 0x0000003c, 0x31314452, 0x0000003c,
0x00000018, 0x00000020, 0x00000028, 0x00000024, 0x0000000c, 0x00000000, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x00000034, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
0x00000e01, 0x445f5653, 0x68747065, 0x7373654c, 0x61757145, 0xabab006c, 0x58454853, 0x00000028,
0x00000050, 0x0000000a, 0x0100086a, 0x02000065, 0x00027001, 0x04000036, 0x00027001, 0x00004001,
0x3f99999a, 0x0100003e, 0x54415453, 0x00000094, 0x00000002, 0x00000000, 0x00000000, 0x00000001,
0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000,
};

/*
 * fxc.exe /E PS /Tps_5_0 /Fx
 */
#if 0
float PS() : SV_DepthGreaterEqual
{
    float a = 1.2f;
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_2[] = {
0x43425844, 0x9f61c8df, 0x612cbb1f, 0x9e1d039e, 0xf925a074, 0x00000001, 0x000001c8, 0x00000005,
0x00000034, 0x000000ac, 0x000000bc, 0x000000fc, 0x0000012c, 0x46454452, 0x00000070, 0x00000000,
0x00000000, 0x00000000, 0x0000003c, 0xffff0500, 0x00000100, 0x0000003c, 0x31314452, 0x0000003c,
0x00000018, 0x00000020, 0x00000028, 0x00000024, 0x0000000c, 0x00000000, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x00000038, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
0x00000e01, 0x445f5653, 0x68747065, 0x61657247, 0x45726574, 0x6c617571, 0xababab00, 0x58454853,
0x00000028, 0x00000050, 0x0000000a, 0x0100086a, 0x02000065, 0x00026001, 0x04000036, 0x00026001,
0x00004001, 0x3f99999a, 0x0100003e, 0x54415453, 0x00000094, 0x00000002, 0x00000000, 0x00000000,
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000,
};

/*
 * fxc.exe /E PS /Tps_5_0 /Fx
 */
#if 0
float PS() : sV_DePtH
{
    float a = 1.2f;
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_3[] = {
0x43425844, 0x32cec0e6, 0x3873ed32, 0x2e86ffd0, 0x21bb00e8, 0x00000001, 0x000001bc, 0x00000005,
0x00000034, 0x000000ac, 0x000000bc, 0x000000f0, 0x00000120, 0x46454452, 0x00000070, 0x00000000,
0x00000000, 0x00000000, 0x0000003c, 0xffff0500, 0x00000100, 0x0000003c, 0x31314452, 0x0000003c,
0x00000018, 0x00000020, 0x00000028, 0x00000024, 0x0000000c, 0x00000000, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
0x00000e01, 0x445f5673, 0x48745065, 0xababab00, 0x58454853, 0x00000028, 0x00000050, 0x0000000a,
0x0100086a, 0x02000065, 0x0000c001, 0x04000036, 0x0000c001, 0x00004001, 0x3f99999a, 0x0100003e,
0x54415453, 0x00000094, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/*
 * fxc.exe /E PS /Tps_4_0 /Fx
 */
#if 0
float PS() : SV_Depth
{
    float a = 1.2f;
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_4[] = {
0x43425844, 0x7af34874, 0x975f09ad, 0xf6e50764, 0xdfb1255f, 0x00000001, 0x00000178, 0x00000005,
0x00000034, 0x0000008c, 0x0000009c, 0x000000d0, 0x000000fc, 0x46454452, 0x00000050, 0x00000000,
0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0xffffffff,
0x00000e01, 0x445f5653, 0x68747065, 0xababab00, 0x52444853, 0x00000024, 0x00000040, 0x00000009,
0x02000065, 0x0000c001, 0x04000036, 0x0000c001, 0x00004001, 0x3f99999a, 0x0100003e, 0x54415453,
0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/*
 * fxc.exe /E PS /Tps_4_0 /Fx
 */
#if 0
bool PS() : SV_COVERAGE
{
    bool a = true;
    return a;
}
#endif
static const DWORD test_reflection_desc_ps_output_blob_5[] = {
0x43425844, 0x40ae32a7, 0xe944bb1c, 0x1a2b1923, 0xea25962d, 0x00000001, 0x000001bc, 0x00000005,
0x00000034, 0x000000ac, 0x000000bc, 0x000000f0, 0x00000120, 0x46454452, 0x00000070, 0x00000000,
0x00000000, 0x00000000, 0x0000003c, 0xffff0500, 0x00000100, 0x0000003c, 0x31314452, 0x0000003c,
0x00000018, 0x00000020, 0x00000028, 0x00000024, 0x0000000c, 0x00000000, 0x7263694d, 0x666f736f,
0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000001, 0xffffffff,
0x00000e01, 0x435f5653, 0x5245564f, 0x00454741, 0x58454853, 0x00000028, 0x00000050, 0x0000000a,
0x0100086a, 0x02000065, 0x0000f000, 0x04000036, 0x0000f001, 0x00004001, 0xffffffff, 0x0100003e,
0x54415453, 0x00000094, 0x00000002, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const DWORD *test_reflection_desc_ps_output_blob[] = {
    test_reflection_desc_ps_output_blob_0,
    test_reflection_desc_ps_output_blob_1,
    test_reflection_desc_ps_output_blob_2,
    test_reflection_desc_ps_output_blob_3,
    test_reflection_desc_ps_output_blob_4,
    test_reflection_desc_ps_output_blob_5,
};

static const D3D11_SIGNATURE_PARAMETER_DESC test_reflection_desc_ps_output_result[] =
{
    {"SV_Target", 3, 3, D3D_NAME_TARGET, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0, 0},
    {"SV_DepthLessEqual", 0, 0xffffffff, D3D_NAME_DEPTH_LESS_EQUAL, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_DepthGreaterEqual", 0, 0xffffffff, D3D11_NAME_DEPTH_GREATER_EQUAL, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"sV_DePtH", 0, 0xffffffff, D3D_NAME_DEPTH, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_Depth", 0, 0xffffffff, D3D_NAME_DEPTH, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe, 0},
    {"SV_COVERAGE", 0, 0xffffffff, D3D_NAME_COVERAGE, D3D_REGISTER_COMPONENT_UINT32, 0x1, 0xe, 0},
};

static void test_reflection_desc_ps_output(void)
{
    HRESULT hr;
    ULONG count;
    ID3D11ShaderReflection *ref11;
    D3D11_SIGNATURE_PARAMETER_DESC desc = {0};
    const D3D11_SIGNATURE_PARAMETER_DESC *pdesc;
    unsigned int i;

    for (i = 0; i < sizeof(test_reflection_desc_ps_output_result)/sizeof(*test_reflection_desc_ps_output_result); ++i)
    {
        hr = D3DReflect(test_reflection_desc_ps_output_blob[i], test_reflection_desc_ps_output_blob[i][6], &IID_ID3D11ShaderReflection, (void **)&ref11);
        ok(hr == S_OK, "(%u): D3DReflect failed %x\n", i, hr);

        pdesc = &test_reflection_desc_ps_output_result[i];

        hr = ref11->lpVtbl->GetOutputParameterDesc(ref11, 0, &desc);
        ok(hr == S_OK, "(%u): GetOutputParameterDesc failed, got %x, expected %x\n", i, hr, S_OK);

        ok(!strcmp(desc.SemanticName, pdesc->SemanticName), "(%u): GetOutputParameterDesc SemanticName failed, got \"%s\", expected \"%s\"\n",
                i, desc.SemanticName, pdesc->SemanticName);
        ok(desc.SemanticIndex == pdesc->SemanticIndex, "(%u): GetOutputParameterDesc SemanticIndex failed, got %u, expected %u\n",
                i, desc.SemanticIndex, pdesc->SemanticIndex);
        ok(desc.Register == pdesc->Register, "(%u): GetOutputParameterDesc Register failed, got %u, expected %u\n",
                i, desc.Register, pdesc->Register);
        ok(desc.SystemValueType == pdesc->SystemValueType, "(%u): GetOutputParameterDesc SystemValueType failed, got %x, expected %x\n",
                i, desc.SystemValueType, pdesc->SystemValueType);
        ok(desc.ComponentType == pdesc->ComponentType, "(%u): GetOutputParameterDesc ComponentType failed, got %x, expected %x\n",
                i, desc.ComponentType, pdesc->ComponentType);
        ok(desc.Mask == pdesc->Mask, "(%u): GetOutputParameterDesc Mask failed, got %x, expected %x\n",
                i, desc.Mask, pdesc->Mask);
        ok(desc.ReadWriteMask == pdesc->ReadWriteMask, "(%u): GetOutputParameterDesc ReadWriteMask failed, got %x, expected %x\n",
                i, desc.ReadWriteMask, pdesc->ReadWriteMask);
        ok(desc.Stream == pdesc->Stream, "(%u): GetOutputParameterDesc Stream failed, got %u, expected %u\n",
                i, desc.Stream, pdesc->ReadWriteMask);

        count = ref11->lpVtbl->Release(ref11);
        ok(count == 0, "(%u): Release failed %u\n", i, count);
    }
}

/*
 * fxc.exe /E PS /Tps_4_0 /Fx
 */
#if 0
Texture2D tex1;
SamplerState sam
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};
cbuffer c1
{
    float x;
    float y[2];
    int z;
};
cbuffer c2
{
    float t;
};

float4 PS(float2 uv : TEXCOORD0) : sv_target
{
    float4 q = tex1.Sample(sam, uv);
    q.x = q.x + x;
    q.w = q.w + y[0] + y[1] + t;
    return q;
}
#endif
static DWORD test_reflection_bound_resources_blob[] = {
0x43425844, 0xe4af0279, 0x690268fc, 0x76bf6a72, 0xe5aff43b, 0x00000001, 0x000003f4, 0x00000005,
0x00000034, 0x000001e8, 0x0000021c, 0x00000250, 0x00000378, 0x46454452, 0x000001ac, 0x00000002,
0x000000ac, 0x00000004, 0x0000001c, 0xffff0400, 0x00000100, 0x0000017a, 0x0000009c, 0x00000003,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x000000a0, 0x00000002,
0x00000005, 0x00000004, 0xffffffff, 0x00000000, 0x00000001, 0x0000000c, 0x000000a5, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x000000a8, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000001, 0x00000000, 0x006d6173, 0x31786574,
0x00316300, 0xab003263, 0x000000a5, 0x00000003, 0x000000dc, 0x00000030, 0x00000000, 0x00000000,
0x000000a8, 0x00000001, 0x00000160, 0x00000010, 0x00000000, 0x00000000, 0x00000124, 0x00000000,
0x00000004, 0x00000002, 0x00000128, 0x00000000, 0x00000138, 0x00000010, 0x00000014, 0x00000002,
0x0000013c, 0x00000000, 0x0000014c, 0x00000024, 0x00000004, 0x00000000, 0x00000150, 0x00000000,
0xabab0078, 0x00030000, 0x00010001, 0x00000000, 0x00000000, 0xabab0079, 0x00030000, 0x00010001,
0x00000002, 0x00000000, 0xabab007a, 0x00020000, 0x00010001, 0x00000000, 0x00000000, 0x00000178,
0x00000000, 0x00000004, 0x00000002, 0x00000128, 0x00000000, 0x694d0074, 0x736f7263, 0x2074666f,
0x20295228, 0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e39,
0x31332e32, 0xab003131, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000,
0x00000000, 0x00000003, 0x00000000, 0x00000303, 0x43584554, 0x44524f4f, 0xababab00, 0x4e47534f,
0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
0x0000000f, 0x745f7673, 0x65677261, 0xabab0074, 0x52444853, 0x00000120, 0x00000040, 0x00000048,
0x04000059, 0x00208e46, 0x00000000, 0x00000003, 0x04000059, 0x00208e46, 0x00000001, 0x00000001,
0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x03001062,
0x00101032, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x02000068, 0x00000001, 0x09000045,
0x001000f2, 0x00000000, 0x00101046, 0x00000000, 0x00107e46, 0x00000000, 0x00106000, 0x00000000,
0x08000000, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x0020800a, 0x00000000, 0x00000001,
0x08000000, 0x00100082, 0x00000000, 0x0010003a, 0x00000000, 0x0020800a, 0x00000000, 0x00000002,
0x08000000, 0x00102082, 0x00000000, 0x0010003a, 0x00000000, 0x0020800a, 0x00000001, 0x00000000,
0x08000000, 0x00102012, 0x00000000, 0x0010000a, 0x00000000, 0x0020800a, 0x00000000, 0x00000000,
0x05000036, 0x00102062, 0x00000000, 0x00100656, 0x00000000, 0x0100003e, 0x54415453, 0x00000074,
0x00000007, 0x00000001, 0x00000000, 0x00000002, 0x00000004, 0x00000000, 0x00000000, 0x00000001,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static const D3D11_SHADER_INPUT_BIND_DESC test_reflection_bound_resources_result[] =
{
    {"sam", D3D_SIT_SAMPLER, 0, 1, 0, 0, D3D_SRV_DIMENSION_UNKNOWN, 0},
    {"tex1", D3D_SIT_TEXTURE, 0, 1, 12, D3D_RETURN_TYPE_FLOAT, D3D_SRV_DIMENSION_TEXTURE2D, 0xffffffff},
    {"c1", D3D_SIT_CBUFFER, 0, 1, 0, 0, D3D_SRV_DIMENSION_UNKNOWN, 0},
    {"c2", D3D_SIT_CBUFFER, 1, 1, 0, 0, D3D_SRV_DIMENSION_UNKNOWN, 0},
};

static void test_reflection_bound_resources(void)
{
    HRESULT hr;
    ULONG count;
    ID3D11ShaderReflection *ref11;
    D3D11_SHADER_INPUT_BIND_DESC desc;
    const D3D11_SHADER_INPUT_BIND_DESC *pdesc;
    unsigned int i;

    hr = D3DReflect(test_reflection_bound_resources_blob, test_reflection_bound_resources_blob[6], &IID_ID3D11ShaderReflection, (void **)&ref11);
    ok(hr == S_OK, "D3DReflect failed %x\n", hr);

    /* check invalid cases */
    hr = ref11->lpVtbl->GetResourceBindingDesc(ref11, 0, NULL);
    ok(hr == E_INVALIDARG, "GetResourceBindingDesc failed %x\n", hr);

    hr = ref11->lpVtbl->GetResourceBindingDesc(ref11, 0xffffffff, &desc);
    ok(hr == E_INVALIDARG, "GetResourceBindingDesc failed %x\n", hr);

    hr = ref11->lpVtbl->GetResourceBindingDescByName(ref11, NULL, &desc);
    ok(hr == E_INVALIDARG, "GetResourceBindingDescByName failed %x\n", hr);

    hr = ref11->lpVtbl->GetResourceBindingDescByName(ref11, "sam", NULL);
    ok(hr == E_INVALIDARG, "GetResourceBindingDescByName failed %x\n", hr);

    hr = ref11->lpVtbl->GetResourceBindingDescByName(ref11, "invalid", NULL);
    ok(hr == E_INVALIDARG, "GetResourceBindingDescByName failed %x\n", hr);

    hr = ref11->lpVtbl->GetResourceBindingDescByName(ref11, "invalid", &desc);
    ok(hr == E_INVALIDARG, "GetResourceBindingDescByName failed %x\n", hr);

    /* GetResourceBindingDesc */
    for (i = 0; i < sizeof(test_reflection_bound_resources_result)/sizeof(*test_reflection_bound_resources_result); ++i)
    {
        pdesc = &test_reflection_bound_resources_result[i];

        hr = ref11->lpVtbl->GetResourceBindingDesc(ref11, i, &desc);
        ok(hr == S_OK, "GetResourceBindingDesc(%u) failed, got %x, expected %x\n", i, hr, S_OK);

        ok(!strcmp(desc.Name, pdesc->Name), "GetResourceBindingDesc(%u) Name failed, got \"%s\", expected \"%s\"\n",
                i, desc.Name, pdesc->Name);
        ok(desc.Type == pdesc->Type, "GetResourceBindingDesc(%u) Type failed, got %x, expected %x\n",
                i, desc.Type, pdesc->Type);
        ok(desc.BindPoint == pdesc->BindPoint, "GetResourceBindingDesc(%u) BindPoint failed, got %u, expected %u\n",
                i, desc.BindPoint, pdesc->BindPoint);
        ok(desc.BindCount == pdesc->BindCount, "GetResourceBindingDesc(%u) BindCount failed, got %u, expected %u\n",
                i, desc.BindCount, pdesc->BindCount);
        ok(desc.uFlags == pdesc->uFlags, "GetResourceBindingDesc(%u) uFlags failed, got %u, expected %u\n",
                i, desc.uFlags, pdesc->uFlags);
        ok(desc.ReturnType == pdesc->ReturnType, "GetResourceBindingDesc(%u) ReturnType failed, got %x, expected %x\n",
                i, desc.ReturnType, pdesc->ReturnType);
        ok(desc.Dimension == pdesc->Dimension, "GetResourceBindingDesc(%u) Dimension failed, got %x, expected %x\n",
                i, desc.Dimension, pdesc->Dimension);
        ok(desc.NumSamples == pdesc->NumSamples, "GetResourceBindingDesc(%u) NumSamples failed, got %u, expected %u\n",
                i, desc.NumSamples, pdesc->NumSamples);
    }

    /* GetResourceBindingDescByName */
    for (i = 0; i < sizeof(test_reflection_bound_resources_result)/sizeof(*test_reflection_bound_resources_result); ++i)
    {
        pdesc = &test_reflection_bound_resources_result[i];

        hr = ref11->lpVtbl->GetResourceBindingDescByName(ref11, pdesc->Name, &desc);
        ok(hr == S_OK, "GetResourceBindingDescByName(%u) failed, got %x, expected %x\n", i, hr, S_OK);

        ok(!strcmp(desc.Name, pdesc->Name), "GetResourceBindingDescByName(%u) Name failed, got \"%s\", expected \"%s\"\n",
                i, desc.Name, pdesc->Name);
        ok(desc.Type == pdesc->Type, "GetResourceBindingDescByName(%u) Type failed, got %x, expected %x\n",
                i, desc.Type, pdesc->Type);
        ok(desc.BindPoint == pdesc->BindPoint, "GetResourceBindingDescByName(%u) BindPoint failed, got %u, expected %u\n",
                i, desc.BindPoint, pdesc->BindPoint);
        ok(desc.BindCount == pdesc->BindCount, "GetResourceBindingDescByName(%u) BindCount failed, got %u, expected %u\n",
                i, desc.BindCount, pdesc->BindCount);
        ok(desc.uFlags == pdesc->uFlags, "GetResourceBindingDescByName(%u) uFlags failed, got %u, expected %u\n",
                i, desc.uFlags, pdesc->uFlags);
        ok(desc.ReturnType == pdesc->ReturnType, "GetResourceBindingDescByName(%u) ReturnType failed, got %x, expected %x\n",
                i, desc.ReturnType, pdesc->ReturnType);
        ok(desc.Dimension == pdesc->Dimension, "GetResourceBindingDescByName(%u) Dimension failed, got %x, expected %x\n",
                i, desc.Dimension, pdesc->Dimension);
        ok(desc.NumSamples == pdesc->NumSamples, "GetResourceBindingDescByName(%u) NumSamples failed, got %u, expected %u\n",
                i, desc.NumSamples, pdesc->NumSamples);
    }

    count = ref11->lpVtbl->Release(ref11);
    ok(count == 0, "Release failed %u\n", count);
}

START_TEST(reflection)
{
    test_reflection_references();
    test_reflection_desc_vs();
    test_reflection_desc_ps();
    test_reflection_desc_ps_output();
    test_reflection_bound_resources();
}
