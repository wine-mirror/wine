/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2009 Rico Schüller
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
#include "d3d10.h"
#include "wine/test.h"

#include <float.h>

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_WARP, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
        return device;

    return NULL;
}

static inline HRESULT create_effect(DWORD *data, UINT flags, ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3D10Effect **effect)
{
    /*
     * Don't use sizeof(data), use data[6] as size,
     * because the DWORD data[] has only complete DWORDs and
     * so it could happen that there are padded bytes at the end.
     *
     * The fx size (data[6]) could be up to 3 BYTEs smaller
     * than the sizeof(data).
     */
    return D3D10CreateEffectFromMemory(data, data[6], flags, device, effect_pool, effect);
}

/*
 * test_effect_constant_buffer_type
 */
#if 0
cbuffer cb
{
    float   f1 : SV_POSITION;
    float   f2 : COLOR0;
};
#endif
static DWORD fx_test_ecbt[] = {
0x43425844, 0xc92a4732, 0xbd0d68c0, 0x877f71ee,
0x871fc277, 0x00000001, 0x0000010a, 0x00000001,
0x00000024, 0x30315846, 0x000000de, 0xfeff1001,
0x00000001, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000042,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x66006263,
0x74616f6c, 0x00000700, 0x00000100, 0x00000000,
0x00000400, 0x00001000, 0x00000400, 0x00090900,
0x00316600, 0x505f5653, 0x5449534f, 0x004e4f49,
0x43003266, 0x524f4c4f, 0x00040030, 0x00100000,
0x00000000, 0x00020000, 0xffff0000, 0x0000ffff,
0x00290000, 0x000d0000, 0x002c0000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00380000,
0x000d0000, 0x003b0000, 0x00040000, 0x00000000,
0x00000000, 0x00000000, 0x52590000,
};

static void test_effect_constant_buffer_type(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type, *type2, *null_type;
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_ecbt, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    type = constantbuffer->lpVtbl->GetType(constantbuffer);

    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_OBJECT);
    ok(type_desc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_CBUFFER);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 2, "Members is %u, expected 2\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x10, "UnpackedSize is %#x, expected 0x10\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    string = type->lpVtbl->GetMemberName(type, 0);
    ok(strcmp(string, "f1") == 0, "GetMemberName is \"%s\", expected \"f1\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 0);
    ok(strcmp(string, "SV_POSITION") == 0, "GetMemberSemantic is \"%s\", expected \"SV_POSITION\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 1);
    ok(strcmp(string, "f2") == 0, "GetMemberName is \"%s\", expected \"f2\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 1);
    ok(strcmp(string, "COLOR0") == 0, "GetMemberSemantic is \"%s\", expected \"COLOR0\"\n", string);

    for (i = 0; i < 3; ++i)
    {
        if (i == 0) type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
        else if (i == 1) type2 = type->lpVtbl->GetMemberTypeByName(type, "f1");
        else type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "SV_POSITION");

        hr = type2->lpVtbl->GetDesc(type2, &type_desc);
        ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

        if (i == 0) type2 = type->lpVtbl->GetMemberTypeByIndex(type, 1);
        else if (i == 1) type2 = type->lpVtbl->GetMemberTypeByName(type, "f2");
        else type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "COLOR0");

        hr = type2->lpVtbl->GetDesc(type2, &type_desc);
        ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);
    }

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, NULL);
    ok(hr == E_INVALIDARG, "GetDesc got %x, expected %x\n", hr, E_INVALIDARG);

    null_type = type->lpVtbl->GetMemberTypeByIndex(type, 3);
    hr = null_type->lpVtbl->GetDesc(null_type, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_type->lpVtbl->GetDesc(null_type, NULL);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    type2 = type->lpVtbl->GetMemberTypeByName(type, "invalid");
    ok(type2 == null_type, "GetMemberTypeByName got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeByName(type, NULL);
    ok(type2 == null_type, "GetMemberTypeByName got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "invalid");
    ok(type2 == null_type, "GetMemberTypeBySemantic got wrong type %p, expected %p\n", type2, null_type);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, NULL);
    ok(type2 == null_type, "GetMemberTypeBySemantic got wrong type %p, expected %p\n", type2, null_type);

    string = type->lpVtbl->GetMemberName(type, 3);
    ok(string == NULL, "GetMemberName is \"%s\", expected \"NULL\"\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 3);
    ok(string == NULL, "GetMemberSemantic is \"%s\", expected \"NULL\"\n", string);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_variable_type
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
};
struct test1
{
    float   f1;
    float   f2;
    test    t;
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_evt[] = {
0x43425844, 0xe079efed, 0x90bda0f2, 0xa6e2d0b4,
0xd2d6c200, 0x00000001, 0x0000018c, 0x00000001,
0x00000024, 0x30315846, 0x00000160, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x000000e0,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x74007400,
0x00747365, 0x53003366, 0x4f505f56, 0x49544953,
0x66004e4f, 0x4f430034, 0x30524f4c, 0x00003700,
0x00000300, 0x00000000, 0x00000800, 0x00001000,
0x00000800, 0x00000200, 0x00003c00, 0x00003f00,
0x00000000, 0x00001600, 0x00004b00, 0x00004e00,
0x00000400, 0x00001600, 0x00000700, 0x00000300,
0x00000000, 0x00001800, 0x00002000, 0x00001000,
0x00000300, 0x00000d00, 0x00000000, 0x00000000,
0x00001600, 0x00003200, 0x00000000, 0x00000400,
0x00001600, 0x00003500, 0x00000000, 0x00001000,
0x00005500, 0x00317400, 0x00000004, 0x00000020,
0x00000000, 0x00000001, 0xffffffff, 0x00000000,
0x000000dd, 0x00000091, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000,
};

static void test_effect_variable_type(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type, *type2, *type3;
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evt, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    type = constantbuffer->lpVtbl->GetType(constantbuffer);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_OBJECT);
    ok(type_desc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_CBUFFER);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 1, "Members is %u, expected 1\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x10, "PackedSize is %#x, expected 0x10\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x20, "UnpackedSize is %#x, expected 0x20\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x20, "Stride is %#x, expected 0x20\n", type_desc.Stride);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "test1") == 0, "TypeName is \"%s\", expected \"test1\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x10, "PackedSize is %#x, expected 0x10\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x18, "UnpackedSize is %#x, expected 0x18\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x20, "Stride is %#x, expected 0x20\n", type_desc.Stride);

    string = type->lpVtbl->GetMemberName(type, 0);
    ok(strcmp(string, "f1") == 0, "GetMemberName is \"%s\", expected \"f1\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 1);
    ok(strcmp(string, "f2") == 0, "GetMemberName is \"%s\", expected \"f2\"\n", string);

    string = type->lpVtbl->GetMemberName(type, 2);
    ok(strcmp(string, "t") == 0, "GetMemberName is \"%s\", expected \"t\"\n", string);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 1);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 2);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 2, "Members is %u, expected 2\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x8, "UnpackedSize is %#x, expected 0x8\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %x, expected 0x10\n", type_desc.Stride);

    for (i = 0; i < 3; ++i)
    {
        if (i == 0) type3 = type2->lpVtbl->GetMemberTypeByIndex(type2, 0);
        else if (i == 1) type3 = type2->lpVtbl->GetMemberTypeByName(type2, "f3");
        else type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "SV_POSITION");

        hr = type3->lpVtbl->GetDesc(type3, &type_desc);
        ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n",
            type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

        if (i == 0) type3 = type2->lpVtbl->GetMemberTypeByIndex(type2, 1);
        else if (i == 1) type3 = type2->lpVtbl->GetMemberTypeByName(type2, "f4");
        else type3 = type2->lpVtbl->GetMemberTypeBySemantic(type2, "COLOR0");

        hr = type3->lpVtbl->GetDesc(type3, &type_desc);
        ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

        ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n",
            type_desc.TypeName);
        ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
        ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
        ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
        ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
        ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
        ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
        ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
        ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
        ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);
    }

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 0);
    hr = type2->lpVtbl->GetDesc(type2, NULL);
    ok(hr == E_INVALIDARG, "GetDesc got %x, expected %x\n", hr, E_INVALIDARG);

    type2 = type->lpVtbl->GetMemberTypeByIndex(type, 4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    type2 = type->lpVtbl->GetMemberTypeByName(type, "invalid");
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    type2 = type->lpVtbl->GetMemberTypeByName(type, NULL);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, "invalid");
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    type2 = type->lpVtbl->GetMemberTypeBySemantic(type, NULL);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    string = type->lpVtbl->GetMemberName(type, 4);
    ok(string == NULL, "GetMemberName is \"%s\", expected NULL\n", string);

    string = type->lpVtbl->GetMemberSemantic(type, 4);
    ok(string == NULL, "GetMemberSemantic is \"%s\", expected NULL\n", string);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_variable_member
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
};
struct test1
{
    float   f1;
    float   f2;
    test    t;
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_evm[] = {
0x43425844, 0xe079efed, 0x90bda0f2, 0xa6e2d0b4,
0xd2d6c200, 0x00000001, 0x0000018c, 0x00000001,
0x00000024, 0x30315846, 0x00000160, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x000000e0,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x74007400,
0x00747365, 0x53003366, 0x4f505f56, 0x49544953,
0x66004e4f, 0x4f430034, 0x30524f4c, 0x00003700,
0x00000300, 0x00000000, 0x00000800, 0x00001000,
0x00000800, 0x00000200, 0x00003c00, 0x00003f00,
0x00000000, 0x00001600, 0x00004b00, 0x00004e00,
0x00000400, 0x00001600, 0x00000700, 0x00000300,
0x00000000, 0x00001800, 0x00002000, 0x00001000,
0x00000300, 0x00000d00, 0x00000000, 0x00000000,
0x00001600, 0x00003200, 0x00000000, 0x00000400,
0x00001600, 0x00003500, 0x00000000, 0x00001000,
0x00005500, 0x00317400, 0x00000004, 0x00000020,
0x00000000, 0x00000001, 0xffffffff, 0x00000000,
0x000000dd, 0x00000091, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000,
};

static void test_effect_variable_member(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable, *variable2, *variable3, *null_variable;
    D3D10_EFFECT_VARIABLE_DESC desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evm, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Type is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Elements is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "Members is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    null_variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 1);
    hr = null_variable->lpVtbl->GetDesc(null_variable, &desc);
    ok(hr == E_FAIL, "GetDesc got %x, expected %x\n", hr, E_FAIL);

    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "t1");
    ok(variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, variable);
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "t1") == 0, "Name is \"%s\", expected \"t1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "invalid");
    ok(null_variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, NULL);
    ok(null_variable == variable2, "GetMemberByName got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberBySemantic(constantbuffer, "invalid");
    ok(null_variable == variable2, "GetMemberBySemantic got %p, expected %p\n", variable2, null_variable);

    variable2 = constantbuffer->lpVtbl->GetMemberBySemantic(constantbuffer, NULL);
    ok(null_variable == variable2, "GetMemberBySemantic got %p, expected %p\n", variable2, null_variable);

    /* check members of "t1" */
    variable2 = variable->lpVtbl->GetMemberByName(variable, "f1");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f1") == 0, "Name is \"%s\", expected \"f1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 0);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    variable2 = variable->lpVtbl->GetMemberByName(variable, "f2");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 4, "BufferOffset is %u, expected 4\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 1);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    variable2 = variable->lpVtbl->GetMemberByName(variable, "t");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable3 = variable->lpVtbl->GetMemberByIndex(variable, 2);
    ok(variable2 == variable3, "GetMemberByIndex got %p, expected %p\n", variable3, variable2);

    /* check members of "t" */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f3");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "SV_POSITION");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberByIndex(variable2, 0);
    ok(variable == variable3, "GetMemberByIndex got %p, expected %p\n", variable, variable3);

    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f4");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 20, "BufferOffset is %u, expected 20\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = variable2->lpVtbl->GetMemberBySemantic(variable2, "COLOR0");
    ok(variable == variable3, "GetMemberBySemantic got %p, expected %p\n", variable, variable3);

    variable = variable2->lpVtbl->GetMemberByIndex(variable2, 1);
    ok(variable == variable3, "GetMemberByIndex got %p, expected %p\n", variable, variable3);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_variable_element
 */
#if 0
struct test
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR0;
    float   f5 : COLOR1;
};
struct test1
{
    float   f1;
    float   f2[3];
    test    t[2];
};
cbuffer cb
{
    test1 t1;
};
#endif
static DWORD fx_test_eve[] = {
0x43425844, 0x6ea69fd9, 0x9b4e6390, 0x006f9f71,
0x57ad58f4, 0x00000001, 0x000001c2, 0x00000001,
0x00000024, 0x30315846, 0x00000196, 0xfeff1001,
0x00000001, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000116,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x74006263,
0x31747365, 0x00316600, 0x616f6c66, 0x00100074,
0x00010000, 0x00000000, 0x00040000, 0x00100000,
0x00040000, 0x09090000, 0x32660000, 0x00001000,
0x00000100, 0x00000300, 0x00002400, 0x00001000,
0x00000c00, 0x00090900, 0x74007400, 0x00747365,
0x53003366, 0x4f505f56, 0x49544953, 0x66004e4f,
0x4f430034, 0x30524f4c, 0x00356600, 0x4f4c4f43,
0x53003152, 0x03000000, 0x02000000, 0x1c000000,
0x10000000, 0x18000000, 0x03000000, 0x58000000,
0x5b000000, 0x00000000, 0x16000000, 0x67000000,
0x6a000000, 0x04000000, 0x16000000, 0x71000000,
0x74000000, 0x08000000, 0x16000000, 0x07000000,
0x03000000, 0x00000000, 0x5c000000, 0x60000000,
0x28000000, 0x03000000, 0x0d000000, 0x00000000,
0x00000000, 0x16000000, 0x32000000, 0x00000000,
0x10000000, 0x35000000, 0x51000000, 0x00000000,
0x40000000, 0x7b000000, 0x74000000, 0x00040031,
0x00600000, 0x00000000, 0x00010000, 0xffff0000,
0x0000ffff, 0x01130000, 0x00c70000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000,
};

static void test_effect_variable_element(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *parent;
    ID3D10EffectVariable *variable, *variable2, *variable3, *variable4, *variable5;
    ID3D10EffectType *type, *type2;
    D3D10_EFFECT_VARIABLE_DESC desc;
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_eve, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Type is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Elements is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "Members is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    variable2 = constantbuffer->lpVtbl->GetMemberByName(constantbuffer, "t1");
    hr = variable2->lpVtbl->GetDesc(variable2, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    parent = variable2->lpVtbl->GetParentConstantBuffer(variable2);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    /* check variable f1 */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f1");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f1") == 0, "Name is \"%s\", expected \"f1\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    /* check variable f2 */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "f2");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)!\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 3, "Elements is %u, expected 3\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x24, "UnpackedSize is %#x, expected 0x24\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 0);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 16, "BufferOffset is %u, expected 16\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 1);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 32, "BufferOffset is %u, expected 32\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type2 = variable4->lpVtbl->GetType(variable4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);
    ok(type == type2, "type(%p) != type2(%p)\n", type, type2);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 2);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f2") == 0, "Name is \"%s\", expected \"f2\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 48, "BufferOffset is %u, expected 48\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type2 = variable4->lpVtbl->GetType(variable4);
    hr = type2->lpVtbl->GetDesc(type2, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);
    ok(type == type2, "type(%p) != type2(%p)\n", type, type2);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    /* check variable t */
    variable3 = variable2->lpVtbl->GetMemberByName(variable2, "t");
    hr = variable3->lpVtbl->GetDesc(variable3, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)!\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable3->lpVtbl->GetParentConstantBuffer(variable3);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable3->lpVtbl->GetType(variable3);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 2, "Elements is %u, expected 2\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x1c, "UnpackedSize is %#x, expected 0x1c\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 0);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0xc, "UnpackedSize is %#x, expected 0xc\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 0);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 64, "BufferOffset is %u, expected 64\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 1);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 68, "BufferOffset is %u, expected 68\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 2);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f5") == 0, "Name is \"%s\", expected \"f5\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR1") == 0, "Semantic is \"%s\", expected \"COLOR1\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 72, "BufferOffset is %u, expected 72\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable4 = variable3->lpVtbl->GetElement(variable3, 1);
    hr = variable4->lpVtbl->GetDesc(variable4, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "t") == 0, "Name is \"%s\", expected \"t\"\n", desc.Name);
    ok(desc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 80, "BufferOffset is %u, expected 80\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable4->lpVtbl->GetParentConstantBuffer(variable4);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n",
        parent, constantbuffer);

    type = variable4->lpVtbl->GetType(variable4);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "test") == 0, "TypeName is \"%s\", expected \"test\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_STRUCT, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_STRUCT);
    ok(type_desc.Type == D3D10_SVT_VOID, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_VOID);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 3, "Members is %u, expected 3\n", type_desc.Members);
    ok(type_desc.Rows == 0, "Rows is %u, expected 0\n", type_desc.Rows);
    ok(type_desc.Columns == 0, "Columns is %u, expected 0\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0xc, "PackedSize is %#x, expected 0xc\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0xc, "UnpackedSize is %#x, expected 0xc\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 0);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f3") == 0, "Name is \"%s\", expected \"f3\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "SV_POSITION") == 0, "Semantic is \"%s\", expected \"SV_POSITION\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 80, "BufferOffset is %u, expected 80\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 1);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f4") == 0, "Name is \"%s\", expected \"f4\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR0") == 0, "Semantic is \"%s\", expected \"COLOR0\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 84, "BufferOffset is %u, expected 84\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    variable5 = variable4->lpVtbl->GetMemberByIndex(variable4, 2);
    hr = variable5->lpVtbl->GetDesc(variable5, &desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(desc.Name, "f5") == 0, "Name is \"%s\", expected \"f5\"\n", desc.Name);
    ok(strcmp(desc.Semantic, "COLOR1") == 0, "Semantic is \"%s\", expected \"COLOR1\"\n", desc.Semantic);
    ok(desc.Flags == 0, "Flags is %u, expected 0\n", desc.Flags);
    ok(desc.Annotations == 0, "Annotations is %u, expected 0\n", desc.Annotations);
    ok(desc.BufferOffset == 88, "BufferOffset is %u, expected 88\n", desc.BufferOffset);
    ok(desc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", desc.ExplicitBindPoint);

    parent = variable5->lpVtbl->GetParentConstantBuffer(variable5);
    ok(parent == constantbuffer, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable5->lpVtbl->GetType(variable5);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(type_desc.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", type_desc.TypeName);
    ok(type_desc.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", type_desc.Class, D3D10_SVC_SCALAR);
    ok(type_desc.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", type_desc.Type, D3D10_SVT_FLOAT);
    ok(type_desc.Elements == 0, "Elements is %u, expected 0\n", type_desc.Elements);
    ok(type_desc.Members == 0, "Members is %u, expected 0\n", type_desc.Members);
    ok(type_desc.Rows == 1, "Rows is %u, expected 1\n", type_desc.Rows);
    ok(type_desc.Columns == 1, "Columns is %u, expected 1\n", type_desc.Columns);
    ok(type_desc.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", type_desc.PackedSize);
    ok(type_desc.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", type_desc.UnpackedSize);
    ok(type_desc.Stride == 0x10, "Stride is %#x, expected 0x10\n", type_desc.Stride);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_variable_type_class
 */
#if 0
cbuffer cb <String s = "STRING";>
{
    float f;
    vector <int, 2> i;
    matrix <uint, 2, 3> u;
    row_major matrix <bool, 2, 3> b;
};
BlendState blend;
DepthStencilState depthstencil;
RasterizerState rast;
SamplerState sam;
RenderTargetView rtv;
DepthStencilView dsv;
Texture1D t1;
Texture1DArray t1a;
Texture2D t2;
Texture2DMS <float4, 4> t2dms;
Texture2DArray t2a;
Texture2DMSArray <float4, 4> t2dmsa;
Texture3D t3;
TextureCube tq;
GeometryShader gs[2];
PixelShader ps;
VertexShader vs[1];
#endif
static DWORD fx_test_evtc[] = {
0x43425844, 0xc04c50cb, 0x0afeb4ef, 0xbb93f346,
0x97a29499, 0x00000001, 0x00000659, 0x00000001,
0x00000024, 0x30315846, 0x0000062d, 0xfeff1001,
0x00000001, 0x00000004, 0x00000011, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x000003d9,
0x00000000, 0x00000008, 0x00000001, 0x00000001,
0x00000001, 0x00000001, 0x00000001, 0x00000001,
0x00000004, 0x00000000, 0x00000000, 0x53006263,
0x6e697274, 0x00070067, 0x00020000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00010000,
0x00730000, 0x49525453, 0x6600474e, 0x74616f6c,
0x00003300, 0x00000100, 0x00000000, 0x00000400,
0x00001000, 0x00000400, 0x00090900, 0x69006600,
0x0032746e, 0x00000057, 0x00000001, 0x00000000,
0x00000008, 0x00000010, 0x00000008, 0x00001112,
0x69750069, 0x7832746e, 0x007a0033, 0x00010000,
0x00000000, 0x00280000, 0x00300000, 0x00180000,
0x5a1b0000, 0x00750000, 0x6c6f6f62, 0x00337832,
0x000000a0, 0x00000001, 0x00000000, 0x0000001c,
0x00000020, 0x00000018, 0x00001a23, 0x6c420062,
0x53646e65, 0x65746174, 0x0000c600, 0x00000200,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000200, 0x656c6200, 0x4400646e, 0x68747065,
0x6e657453, 0x536c6963, 0x65746174, 0x0000f300,
0x00000200, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000300, 0x70656400, 0x74736874,
0x69636e65, 0x6152006c, 0x72657473, 0x72657a69,
0x74617453, 0x012e0065, 0x00020000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00040000,
0x61720000, 0x53007473, 0x6c706d61, 0x74537265,
0x00657461, 0x0000015f, 0x00000002, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000015,
0x006d6173, 0x646e6552, 0x61547265, 0x74656772,
0x77656956, 0x00018c00, 0x00000200, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00001300,
0x76747200, 0x70654400, 0x74536874, 0x69636e65,
0x6569566c, 0x01bd0077, 0x00020000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00140000,
0x73640000, 0x65540076, 0x72757478, 0x00443165,
0x000001ee, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x0000000a, 0x54003174,
0x75747865, 0x44316572, 0x61727241, 0x02170079,
0x00020000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x000b0000, 0x31740000, 0x65540061,
0x72757478, 0x00443265, 0x00000246, 0x00000002,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x0000000c, 0x54003274, 0x75747865, 0x44326572,
0x6f00534d, 0x02000002, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x0e000000, 0x74000000,
0x736d6432, 0x78655400, 0x65727574, 0x72414432,
0x00796172, 0x0000029d, 0x00000002, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x0000000d,
0x00613274, 0x74786554, 0x32657275, 0x41534d44,
0x79617272, 0x0002cc00, 0x00000200, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000f00,
0x64327400, 0x0061736d, 0x74786554, 0x33657275,
0x03000044, 0x00020000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00100000, 0x33740000,
0x78655400, 0x65727574, 0x65627543, 0x00032900,
0x00000200, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00001100, 0x00717400, 0x6d6f6547,
0x79727465, 0x64616853, 0x54007265, 0x02000003,
0x02000000, 0x00000000, 0x00000000, 0x00000000,
0x07000000, 0x67000000, 0x69500073, 0x536c6578,
0x65646168, 0x03820072, 0x00020000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00050000,
0x73700000, 0x72655600, 0x53786574, 0x65646168,
0x03ad0072, 0x00020000, 0x00010000, 0x00000000,
0x00000000, 0x00000000, 0x00060000, 0x73760000,
0x00000400, 0x00006000, 0x00000000, 0x00000400,
0xffffff00, 0x000001ff, 0x00002a00, 0x00000e00,
0x00002c00, 0x00005500, 0x00003900, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00007800, 0x00005c00, 0x00000000, 0x00000400,
0x00000000, 0x00000000, 0x00000000, 0x00009e00,
0x00008200, 0x00000000, 0x00001000, 0x00000000,
0x00000000, 0x00000000, 0x0000c400, 0x0000a800,
0x00000000, 0x00004000, 0x00000000, 0x00000000,
0x00000000, 0x0000ed00, 0x0000d100, 0x00000000,
0xffffff00, 0x000000ff, 0x00000000, 0x00012100,
0x00010500, 0x00000000, 0xffffff00, 0x000000ff,
0x00000000, 0x00015a00, 0x00013e00, 0x00000000,
0xffffff00, 0x000000ff, 0x00000000, 0x00018800,
0x00016c00, 0x00000000, 0xffffff00, 0x000000ff,
0x00000000, 0x0001b900, 0x00019d00, 0x00000000,
0xffffff00, 0x000000ff, 0x0001ea00, 0x0001ce00,
0x00000000, 0xffffff00, 0x000000ff, 0x00021400,
0x0001f800, 0x00000000, 0xffffff00, 0x000000ff,
0x00024200, 0x00022600, 0x00000000, 0xffffff00,
0x000000ff, 0x00026c00, 0x00025000, 0x00000000,
0xffffff00, 0x000000ff, 0x00029700, 0x00027b00,
0x00000000, 0xffffff00, 0x000000ff, 0x0002c800,
0x0002ac00, 0x00000000, 0xffffff00, 0x000000ff,
0x0002f900, 0x0002dd00, 0x00000000, 0xffffff00,
0x000000ff, 0x00032600, 0x00030a00, 0x00000000,
0xffffff00, 0x000000ff, 0x00035100, 0x00033500,
0x00000000, 0xffffff00, 0x000000ff, 0x00037f00,
0x00036300, 0x00000000, 0xffffff00, 0x000000ff,
0x00000000, 0x00000000, 0x0003aa00, 0x00038e00,
0x00000000, 0xffffff00, 0x000000ff, 0x00000000,
0x0003d600, 0x0003ba00, 0x00000000, 0xffffff00,
0x000000ff, 0x00000000, 0x00000000,
};

static BOOL is_valid_check(BOOL a, BOOL b)
{
    return (a && b) || (!a && !b);
}

static void check_as(ID3D10EffectVariable *variable)
{
    ID3D10EffectVariable *variable2;
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC td;
    BOOL ret, is_valid;
    HRESULT hr;

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsConstantBuffer(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_CBUFFER);
    ok(ret, "AsConstantBuffer valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsString(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_STRING);
    ok(ret, "AsString valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsScalar(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_SCALAR);
    ok(ret, "AsScalar valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsVector(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_VECTOR);
    ok(ret, "AsVector valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsMatrix(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Class == D3D10_SVC_MATRIX_ROWS
        || td.Class == D3D10_SVC_MATRIX_COLUMNS);
    ok(ret, "AsMatrix valid check failed (Class is %x)\n", td.Class);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsBlend(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_BLEND);
    ok(ret, "AsBlend valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsDepthStencil(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_DEPTHSTENCIL);
    ok(ret, "AsDepthStencil valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsRasterizer(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_RASTERIZER);
    ok(ret, "AsRasterizer valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsSampler(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_SAMPLER);
    ok(ret, "AsSampler valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsDepthStencilView(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_DEPTHSTENCILVIEW);
    ok(ret, "AsDepthStencilView valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsRenderTargetView(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_RENDERTARGETVIEW);
    ok(ret, "AsRenderTargetView valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsShaderResource(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_TEXTURE1D
        || td.Type == D3D10_SVT_TEXTURE1DARRAY || td.Type == D3D10_SVT_TEXTURE2D
        || td.Type == D3D10_SVT_TEXTURE2DMS || td.Type == D3D10_SVT_TEXTURE2DARRAY
        || td.Type == D3D10_SVT_TEXTURE2DMSARRAY || td.Type == D3D10_SVT_TEXTURE3D
        || td.Type == D3D10_SVT_TEXTURECUBE);
    ok(ret, "AsShaderResource valid check failed (Type is %x)\n", td.Type);

    variable2 = (ID3D10EffectVariable *)variable->lpVtbl->AsShader(variable);
    is_valid = variable2->lpVtbl->IsValid(variable2);
    ret = is_valid_check(is_valid, td.Type == D3D10_SVT_GEOMETRYSHADER
        || td.Type == D3D10_SVT_PIXELSHADER || td.Type == D3D10_SVT_VERTEXSHADER);
    ok(ret, "AsShader valid check failed (Type is %x)\n", td.Type);
}

static void test_effect_variable_type_class(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *null_buffer, *parent;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type;
    D3D10_EFFECT_VARIABLE_DESC vd;
    D3D10_EFFECT_TYPE_DESC td;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    unsigned int variable_nr = 0;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_evtc, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    /* get the null_constantbuffer, so that we can compare it to variables->GetParentConstantBuffer */
    null_buffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 1);

    /* check constantbuffer cb */
    constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, 0);
    hr = constantbuffer->lpVtbl->GetDesc(constantbuffer, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "cb") == 0, "Name is \"%s\", expected \"cb\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Type is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 1, "Elements is %u, expected 1\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "Members is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as((ID3D10EffectVariable *)constantbuffer);

    parent = constantbuffer->lpVtbl->GetParentConstantBuffer(constantbuffer);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = constantbuffer->lpVtbl->GetType(constantbuffer);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_CBUFFER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 4, "Members is %u, expected 4\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x3c, "PackedSize is %#x, expected 0x3c\n", td.PackedSize);
    ok(td.UnpackedSize == 0x60, "UnpackedSize is %#x, expected 0x60\n", td.UnpackedSize);
    ok(td.Stride == 0x60, "Stride is %#x, expected 0x60\n", td.Stride);

    /* check annotation a */
    variable = constantbuffer->lpVtbl->GetAnnotationByIndex(constantbuffer, 0);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "s") == 0, "Name is \"%s\", expected \"s\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 2, "Flags is %u, expected 2\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "String") == 0, "TypeName is \"%s\", expected \"String\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_STRING, "Type is %x, expected %x\n", td.Type, D3D10_SVT_STRING);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check float f */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "f") == 0, "Name is \"%s\", expected \"f\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "float") == 0, "TypeName is \"%s\", expected \"float\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_SCALAR, "Class is %x, expected %x\n", td.Class, D3D10_SVC_SCALAR);
    ok(td.Type == D3D10_SVT_FLOAT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_FLOAT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 1, "Rows is %u, expected 1\n", td.Rows);
    ok(td.Columns == 1, "Columns is %u, expected 1\n", td.Columns);
    ok(td.PackedSize == 0x4, "PackedSize is %#x, expected 0x4\n", td.PackedSize);
    ok(td.UnpackedSize == 0x4, "UnpackedSize is %#x, expected 0x4\n", td.UnpackedSize);
    ok(td.Stride == 0x10, "Stride is %#x, expected 0x10\n", td.Stride);

    /* check int2 i */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "i") == 0, "Name is \"%s\", expected \"i\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 4, "BufferOffset is %u, expected 4\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "int2") == 0, "TypeName is \"%s\", expected \"int2\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_VECTOR, "Class is %x, expected %x\n", td.Class, D3D10_SVC_VECTOR);
    ok(td.Type == D3D10_SVT_INT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_INT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 1, "Rows is %u, expected 1\n", td.Rows);
    ok(td.Columns == 2, "Columns is %u, expected 2\n", td.Columns);
    ok(td.PackedSize == 0x8, "PackedSize is %#x, expected 0x8\n", td.PackedSize);
    ok(td.UnpackedSize == 0x8, "UnpackedSize is %#x, expected 0x8\n", td.UnpackedSize);
    ok(td.Stride == 0x10, "Stride is %#x, expected 0x10\n", td.Stride);

    /* check uint2x3 u */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "u") == 0, "Name is \"%s\", expected \"u\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 16, "BufferOffset is %u, expected 16\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "uint2x3") == 0, "TypeName is \"%s\", expected \"uint2x3\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_MATRIX_COLUMNS, "Class is %x, expected %x\n", td.Class, D3D10_SVC_MATRIX_COLUMNS);
    ok(td.Type == D3D10_SVT_UINT, "Type is %x, expected %x\n", td.Type, D3D10_SVT_UINT);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 2, "Rows is %u, expected 2\n", td.Rows);
    ok(td.Columns == 3, "Columns is %u, expected 3\n", td.Columns);
    ok(td.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", td.PackedSize);
    ok(td.UnpackedSize == 0x28, "UnpackedSize is %#x, expected 0x28\n", td.UnpackedSize);
    ok(td.Stride == 0x30, "Stride is %#x, expected 0x30\n", td.Stride);

    /* check bool2x3 b */
    variable = constantbuffer->lpVtbl->GetMemberByIndex(constantbuffer, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "b") == 0, "Name is \"%s\", expected \"b\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 64, "BufferOffset is %u, expected 64\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(constantbuffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, constantbuffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "bool2x3") == 0, "TypeName is \"%s\", expected \"bool2x3\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_MATRIX_ROWS, "Class is %x, expected %x\n", td.Class, D3D10_SVC_MATRIX_ROWS);
    ok(td.Type == D3D10_SVT_BOOL, "Type is %x, expected %x\n", td.Type, D3D10_SVT_BOOL);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 2, "Rows is %u, expected 2\n", td.Rows);
    ok(td.Columns == 3, "Columns is %u, expected 3\n", td.Columns);
    ok(td.PackedSize == 0x18, "PackedSize is %#x, expected 0x18\n", td.PackedSize);
    ok(td.UnpackedSize == 0x1c, "UnpackedSize is %#x, expected 0x1c\n", td.UnpackedSize);
    ok(td.Stride == 0x20, "Stride is %#x, expected 0x20\n", td.Stride);

    /* check BlendState blend */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "blend") == 0, "Name is \"%s\", expected \"blend\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "BlendState") == 0, "TypeName is \"%s\", expected \"BlendState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_BLEND, "Type is %x, expected %x\n", td.Type, D3D10_SVT_BLEND);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check DepthStencilState depthstencil */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "depthstencil") == 0, "Name is \"%s\", expected \"depthstencil\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "DepthStencilState") == 0, "TypeName is \"%s\", expected \"DepthStencilState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_DEPTHSTENCIL, "Type is %x, expected %x\n", td.Type, D3D10_SVT_DEPTHSTENCIL);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check RasterizerState rast */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "rast") == 0, "Name is \"%s\", expected \"rast\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "RasterizerState") == 0, "TypeName is \"%s\", expected \"RasterizerState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_RASTERIZER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_RASTERIZER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check SamplerState sam */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "sam") == 0, "Name is \"%s\", expected \"sam\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "SamplerState") == 0, "TypeName is \"%s\", expected \"SamplerState\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_SAMPLER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_SAMPLER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check RenderTargetView rtv */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "rtv") == 0, "Name is \"%s\", expected \"rtv\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "RenderTargetView") == 0, "TypeName is \"%s\", expected \"RenderTargetView\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_RENDERTARGETVIEW, "Type is %x, expected %x\n", td.Type, D3D10_SVT_RENDERTARGETVIEW);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check DepthStencilView dsv */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "dsv") == 0, "Name is \"%s\", expected \"dsv\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "DepthStencilView") == 0, "TypeName is \"%s\", expected \"DepthStencilView\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_DEPTHSTENCILVIEW, "Type is %x, expected %x\n", td.Type, D3D10_SVT_DEPTHSTENCILVIEW);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture1D t1 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t1") == 0, "Name is \"%s\", expected \"t1\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture1D") == 0, "TypeName is \"%s\", expected \"Texture1D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE1D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE1D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture1DArray t1a */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t1a") == 0, "Name is \"%s\", expected \"t1a\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture1DArray") == 0, "TypeName is \"%s\", expected \"Texture1DArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE1DARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE1DARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2D t2 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t2") == 0, "Name is \"%s\", expected \"t2\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture2D") == 0, "TypeName is \"%s\", expected \"Texture2D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DMS t2dms */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t2dms") == 0, "Name is \"%s\", expected \"t2dms\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture2DMS") == 0, "TypeName is \"%s\", expected \"Texture2DMS\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DMS, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DMS);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DArray t2a */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t2a") == 0, "Name is \"%s\", expected \"t2a\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture2DArray") == 0, "TypeName is \"%s\", expected \"Texture2DArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture2DMSArray t2dmsa */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t2dmsa") == 0, "Name is \"%s\", expected \"t2dmsa\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture2DMSArray") == 0, "TypeName is \"%s\", expected \"TTexture2DMSArray\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE2DMSARRAY, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE2DMSARRAY);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check Texture3D t3 */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "t3") == 0, "Name is \"%s\", expected \"t3\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "Texture3D") == 0, "TypeName is \"%s\", expected \"Texture3D\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURE3D, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURE3D);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check TextureCube tq */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "tq") == 0, "Name is \"%s\", expected \"tq\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "TextureCube") == 0, "TypeName is \"%s\", expected \"TextureCube\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_TEXTURECUBE, "Type is %x, expected %x\n", td.Type, D3D10_SVT_TEXTURECUBE);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check GeometryShader gs[2] */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "gs") == 0, "Name is \"%s\", expected \"gs\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(td.Elements == 2, "Elements is %u, expected 2\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check PixelShader ps */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "ps") == 0, "Name is \"%s\", expected \"ps\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_PIXELSHADER);
    ok(td.Elements == 0, "Elements is %u, expected 0\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    /* check VertexShader vs[1] */
    variable = effect->lpVtbl->GetVariableByIndex(effect, variable_nr++);
    hr = variable->lpVtbl->GetDesc(variable, &vd);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vd.Name, "vs") == 0, "Name is \"%s\", expected \"vs\"\n", vd.Name);
    ok(vd.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vd.Semantic);
    ok(vd.Flags == 0, "Flags is %u, expected 0\n", vd.Flags);
    ok(vd.Annotations == 0, "Annotations is %u, expected 0\n", vd.Annotations);
    ok(vd.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vd.BufferOffset);
    ok(vd.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vd.ExplicitBindPoint);

    check_as(variable);

    parent = variable->lpVtbl->GetParentConstantBuffer(variable);
    ok(null_buffer == parent, "GetParentConstantBuffer got %p, expected %p\n", parent, null_buffer);

    type = variable->lpVtbl->GetType(variable);
    hr = type->lpVtbl->GetDesc(type, &td);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(td.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", td.TypeName);
    ok(td.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", td.Class, D3D10_SVC_OBJECT);
    ok(td.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", td.Type, D3D10_SVT_VERTEXSHADER);
    ok(td.Elements == 1, "Elements is %u, expected 1\n", td.Elements);
    ok(td.Members == 0, "Members is %u, expected 0\n", td.Members);
    ok(td.Rows == 0, "Rows is %u, expected 0\n", td.Rows);
    ok(td.Columns == 0, "Columns is %u, expected 0\n", td.Columns);
    ok(td.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", td.PackedSize);
    ok(td.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", td.UnpackedSize);
    ok(td.Stride == 0x0, "Stride is %#x, expected 0x0\n", td.Stride);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_constant_buffer_stride
 */
#if 0
cbuffer cb1
{
    float   a1;
    float   b1;
    float   c1;
    float   d1;
};
cbuffer cb2
{
    float   a2;
    float2  b2;
};
cbuffer cb3
{
    float2  a3;
    float3  b3;
};
cbuffer cb4
{
    float2  a4;
    float3  b4;
    float2  c4;
};
cbuffer cb5
{
    float2  a5;
    float2  b5;
    float3  c5;
};
cbuffer cb6
{
    float2  a6 : packoffset(c0);
    float3  b6 : packoffset(c1);
    float2  c6 : packoffset(c0.z);
};
cbuffer cb7
{
    float2  a7 : packoffset(c0);
    float3  b7 : packoffset(c1);
    float2  c7 : packoffset(c2);
};
cbuffer cb8
{
    float2  a8 : packoffset(c0);
    float3  b8 : packoffset(c0.y);
    float4  c8 : packoffset(c2);
};
cbuffer cb9
{
    float2  a9 : packoffset(c0);
    float2  b9 : packoffset(c0.y);
    float2  c9 : packoffset(c0.z);
};
cbuffer cb10
{
    float4  a10 : packoffset(c2);
};
cbuffer cb11
{
    struct {
        float4 a11;
        float  b11;
    } s11;
    float  c11;
};
cbuffer cb12
{
    float  c12;
    struct {
        float  b12;
        float4 a12;
    } s12;
};
cbuffer cb13
{
    float  a13;
    struct {
        float  b13;
    } s13;
};
cbuffer cb14
{
    struct {
        float  a14;
    } s14;
    struct {
        float  b14;
    } t14;
};
cbuffer cb15
{
    float2  a15[2] : packoffset(c0);
};
#endif
static DWORD fx_test_ecbs[] = {
0x43425844, 0x615d7d77, 0x21289d92, 0xe9e8d98e,
0xcae7b74e, 0x00000001, 0x00000855, 0x00000001,
0x00000024, 0x30315846, 0x00000829, 0xfeff1001,
0x0000000f, 0x00000024, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000285,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00316263,
0x616f6c66, 0x00080074, 0x00010000, 0x00000000,
0x00040000, 0x00100000, 0x00040000, 0x09090000,
0x31610000, 0x00316200, 0x64003163, 0x62630031,
0x32610032, 0x6f6c6600, 0x00327461, 0x0000003d,
0x00000001, 0x00000000, 0x00000008, 0x00000010,
0x00000008, 0x0000110a, 0x63003262, 0x61003362,
0x6c660033, 0x3374616f, 0x00006a00, 0x00000100,
0x00000000, 0x00000c00, 0x00001000, 0x00000c00,
0x00190a00, 0x00336200, 0x00346263, 0x62003461,
0x34630034, 0x35626300, 0x00356100, 0x63003562,
0x62630035, 0x36610036, 0x00366200, 0x63003663,
0x61003762, 0x37620037, 0x00376300, 0x00386263,
0x62003861, 0x6c660038, 0x3474616f, 0x0000ce00,
0x00000100, 0x00000000, 0x00001000, 0x00001000,
0x00001000, 0x00210a00, 0x00386300, 0x00396263,
0x62003961, 0x39630039, 0x31626300, 0x31610030,
0x62630030, 0x3c003131, 0x616e6e75, 0x3e64656d,
0x31316100, 0x31316200, 0x00010f00, 0x00000300,
0x00000000, 0x00001400, 0x00002000, 0x00001400,
0x00000200, 0x00011900, 0x00000000, 0x00000000,
0x0000d500, 0x00011d00, 0x00000000, 0x00001000,
0x00000e00, 0x31317300, 0x31316300, 0x31626300,
0x31630032, 0x31620032, 0x31610032, 0x010f0032,
0x00030000, 0x00000000, 0x00200000, 0x00200000,
0x00140000, 0x00020000, 0x016e0000, 0x00000000,
0x00000000, 0x000e0000, 0x01720000, 0x00000000,
0x00100000, 0x00d50000, 0x31730000, 0x62630032,
0x61003331, 0x62003331, 0x0f003331, 0x03000001,
0x00000000, 0x04000000, 0x10000000, 0x04000000,
0x01000000, 0xbf000000, 0x00000001, 0x00000000,
0x0e000000, 0x73000000, 0x63003331, 0x00343162,
0x00343161, 0x0000010f, 0x00000003, 0x00000000,
0x00000004, 0x00000010, 0x00000004, 0x00000001,
0x000001f8, 0x00000000, 0x00000000, 0x0000000e,
0x00343173, 0x00343162, 0x0000010f, 0x00000003,
0x00000000, 0x00000004, 0x00000010, 0x00000004,
0x00000001, 0x0000022c, 0x00000000, 0x00000000,
0x0000000e, 0x00343174, 0x35316263, 0x00003d00,
0x00000100, 0x00000200, 0x00001800, 0x00001000,
0x00001000, 0x00110a00, 0x35316100, 0x00000400,
0x00001000, 0x00000000, 0x00000400, 0xffffff00,
0x000000ff, 0x00002a00, 0x00000e00, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00002d00, 0x00000e00, 0x00000000, 0x00000400,
0x00000000, 0x00000000, 0x00000000, 0x00003000,
0x00000e00, 0x00000000, 0x00000800, 0x00000000,
0x00000000, 0x00000000, 0x00003300, 0x00000e00,
0x00000000, 0x00000c00, 0x00000000, 0x00000000,
0x00000000, 0x00003600, 0x00001000, 0x00000000,
0x00000200, 0xffffff00, 0x000000ff, 0x00003a00,
0x00000e00, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00006000, 0x00004400,
0x00000000, 0x00000400, 0x00000000, 0x00000000,
0x00000000, 0x00006300, 0x00002000, 0x00000000,
0x00000200, 0xffffff00, 0x000000ff, 0x00006700,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00008d00, 0x00007100,
0x00000000, 0x00001000, 0x00000000, 0x00000000,
0x00000000, 0x00009000, 0x00003000, 0x00000000,
0x00000300, 0xffffff00, 0x000000ff, 0x00009400,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00009700, 0x00007100,
0x00000000, 0x00001000, 0x00000000, 0x00000000,
0x00000000, 0x00009a00, 0x00004400, 0x00000000,
0x00002000, 0x00000000, 0x00000000, 0x00000000,
0x00009d00, 0x00002000, 0x00000000, 0x00000300,
0xffffff00, 0x000000ff, 0x0000a100, 0x00004400,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0000a400, 0x00004400, 0x00000000,
0x00000800, 0x00000000, 0x00000000, 0x00000000,
0x0000a700, 0x00007100, 0x00000000, 0x00001000,
0x00000000, 0x00000000, 0x00000000, 0x0000aa00,
0x00002000, 0x00000000, 0x00000300, 0xffffff00,
0x000000ff, 0x0000ae00, 0x00004400, 0x00000000,
0x00000000, 0x00000000, 0x00000400, 0x00000000,
0x0000b100, 0x00007100, 0x00000000, 0x00001000,
0x00000000, 0x00000400, 0x00000000, 0x0000b400,
0x00004400, 0x00000000, 0x00000800, 0x00000000,
0x00000400, 0x00000000, 0x0000b700, 0x00003000,
0x00000000, 0x00000300, 0xffffff00, 0x000000ff,
0x0000bb00, 0x00004400, 0x00000000, 0x00000000,
0x00000000, 0x00000400, 0x00000000, 0x0000be00,
0x00007100, 0x00000000, 0x00001000, 0x00000000,
0x00000400, 0x00000000, 0x0000c100, 0x00004400,
0x00000000, 0x00002000, 0x00000000, 0x00000400,
0x00000000, 0x0000c400, 0x00003000, 0x00000000,
0x00000300, 0xffffff00, 0x000000ff, 0x0000c800,
0x00004400, 0x00000000, 0x00000000, 0x00000000,
0x00000400, 0x00000000, 0x0000cb00, 0x00007100,
0x00000000, 0x00000400, 0x00000000, 0x00000400,
0x00000000, 0x0000f100, 0x0000d500, 0x00000000,
0x00002000, 0x00000000, 0x00000400, 0x00000000,
0x0000f400, 0x00001000, 0x00000000, 0x00000300,
0xffffff00, 0x000000ff, 0x0000f800, 0x00004400,
0x00000000, 0x00000000, 0x00000000, 0x00000400,
0x00000000, 0x0000fb00, 0x00004400, 0x00000000,
0x00000400, 0x00000000, 0x00000400, 0x00000000,
0x0000fe00, 0x00004400, 0x00000000, 0x00000800,
0x00000000, 0x00000400, 0x00000000, 0x00010100,
0x00003000, 0x00000000, 0x00000100, 0xffffff00,
0x000000ff, 0x00010600, 0x0000d500, 0x00000000,
0x00002000, 0x00000000, 0x00000400, 0x00000000,
0x00010a00, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00015d00, 0x00012100,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00016100, 0x00000e00, 0x00000000,
0x00001400, 0x00000000, 0x00000000, 0x00000000,
0x00016500, 0x00003000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00016a00, 0x00000e00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0001b200, 0x00017600, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x0001b600, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x0001bb00, 0x00000e00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x0001ef00, 0x0001c300, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x0001f300, 0x00002000, 0x00000000, 0x00000200,
0xffffff00, 0x000000ff, 0x00022800, 0x0001fc00,
0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00025c00, 0x00023000, 0x00000000,
0x00001000, 0x00000000, 0x00000000, 0x00000000,
0x00026000, 0x00002000, 0x00000000, 0x00000100,
0xffffff00, 0x000000ff, 0x00028100, 0x00026500,
0x00000000, 0x00000000, 0x00000000, 0x00000400,
0x00000000, 0x00000000,
};

static void test_effect_constant_buffer_stride(void)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC tdesc;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;
    unsigned int i;

    static const struct {
        unsigned int m; /* members */
        unsigned int p; /* packed size */
        unsigned int u; /* unpacked size */
        unsigned int s; /* stride */
    } tv_ecbs[] = {
        {4, 0x10,  0x10,  0x10},
        {2,  0xc,  0x10,  0x10},
        {2, 0x14,  0x20,  0x20},
        {3, 0x1c,  0x30,  0x30},
        {3, 0x1c,  0x20,  0x20},
        {3, 0x1c,  0x20,  0x20},
        {3, 0x1c,  0x30,  0x30},
        {3, 0x24,  0x30,  0x30},
        {3, 0x18,  0x10,  0x10},
        {1, 0x10,  0x30,  0x30},
        {2, 0x18,  0x20,  0x20},
        {2, 0x18,  0x30,  0x30},
        {2,  0x8,  0x20,  0x20},
        {2,  0x8,  0x20,  0x20},
        {1, 0x10,  0x20,  0x20},
    };

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_ecbs, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    for (i=0; i<ARRAY_SIZE(tv_ecbs); i++)
    {
        constantbuffer = effect->lpVtbl->GetConstantBufferByIndex(effect, i);
        type = constantbuffer->lpVtbl->GetType(constantbuffer);

        hr = type->lpVtbl->GetDesc(type, &tdesc);
        ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

        ok(strcmp(tdesc.TypeName, "cbuffer") == 0, "TypeName is \"%s\", expected \"cbuffer\"\n", tdesc.TypeName);
        ok(tdesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", tdesc.Class, D3D10_SVC_OBJECT);
        ok(tdesc.Type == D3D10_SVT_CBUFFER, "Type is %x, expected %x\n", tdesc.Type, D3D10_SVT_CBUFFER);
        ok(tdesc.Elements == 0, "Elements is %u, expected 0\n", tdesc.Elements);
        ok(tdesc.Members == tv_ecbs[i].m, "Members is %u, expected %u\n", tdesc.Members, tv_ecbs[i].m);
        ok(tdesc.Rows == 0, "Rows is %u, expected 0\n", tdesc.Rows);
        ok(tdesc.Columns == 0, "Columns is %u, expected 0\n", tdesc.Columns);
        ok(tdesc.PackedSize == tv_ecbs[i].p, "PackedSize is %#x, expected %#x\n", tdesc.PackedSize, tv_ecbs[i].p);
        ok(tdesc.UnpackedSize == tv_ecbs[i].u, "UnpackedSize is %#x, expected %#x\n", tdesc.UnpackedSize, tv_ecbs[i].u);
        ok(tdesc.Stride == tv_ecbs[i].s, "Stride is %#x, expected %#x\n", tdesc.Stride, tv_ecbs[i].s);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

#if 0
float4 VS( float4 Pos : POSITION ) : SV_POSITION { return Pos; }
float4 VS2( int4 Pos : POSITION ) : SV_POSITION { return Pos; }
float4 PS( float4 Pos : SV_POSITION ) : SV_Target { return float4( 1.0f, 1.0f, 0.0f, 1.0f ); }
struct GS_OUT { float4 Pos : SV_POSITION; };
[maxvertexcount(3)]
void GS( triangle float4 Pos[3] : SV_POSITION, inout TriangleStream<GS_OUT> TriStream )
{
     GS_OUT out1;
     out1.Pos = Pos[0];
     TriStream.Append( out1 );
     out1.Pos = Pos[1];
     TriStream.Append( out1 );
     out1.Pos = Pos[2];
     TriStream.Append( out1 );
     TriStream.RestartStrip();
}
VertexShader v0 = NULL;
PixelShader p0 = NULL;
GeometryShader g0 = NULL;
VertexShader v[2] = { CompileShader( vs_4_0, VS() ), CompileShader( vs_4_0, VS2() ) };
PixelShader p = CompileShader( ps_4_0, PS() );
GeometryShader g = CompileShader( gs_4_0, GS() );
technique10 Render
{
    pass P0 {}
    pass P1
    {
        SetPixelShader( NULL );
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
    }
    pass P2
    {
        SetPixelShader( NULL );
        SetVertexShader( NULL );
        SetGeometryShader( NULL );
    }
    pass P3
    {
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
    }
    pass P4
    {
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
        SetVertexShader( CompileShader( vs_4_0, VS2() ) );
        SetGeometryShader( CompileShader( gs_4_0, GS() ) );
    }
    pass P5
    {
        SetPixelShader( p0 );
        SetVertexShader( v0 );
        SetGeometryShader( g0 );
    }
    pass P6
    {
        SetPixelShader( p );
        SetVertexShader( v[0] );
        SetGeometryShader( g );
    }
    pass P7
    {
        SetPixelShader( p );
        SetVertexShader( v[1] );
        SetGeometryShader( g );
    }
}
#endif
static DWORD fx_local_shader[] = {
0x43425844, 0x95577e13, 0xab5facae, 0xd06d9eab, 0x8b127be0, 0x00000001, 0x00001652, 0x00000001,
0x00000024, 0x30315846, 0x00001626, 0xfeff1001, 0x00000000, 0x00000000, 0x00000006, 0x00000000,
0x00000000, 0x00000000, 0x00000001, 0x0000138a, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000000d, 0x00000006, 0x00000000, 0x74726556,
0x68537865, 0x72656461, 0x00000400, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000600, 0x00307600, 0x65786950, 0x6168536c, 0x00726564, 0x00000030, 0x00000002, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000005, 0x47003070, 0x656d6f65, 0x53797274, 0x65646168,
0x005b0072, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00070000, 0x30670000,
0x00000400, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000600, 0xb4007600,
0x44000001, 0x02434258, 0x5f11b96d, 0x31cdd883, 0xade81d9f, 0x014d6a2d, 0xb4000000, 0x05000001,
0x34000000, 0x8c000000, 0xc0000000, 0xf4000000, 0x38000000, 0x52000001, 0x50464544, 0x00000000,
0x00000000, 0x00000000, 0x1c000000, 0x00000000, 0x00fffe04, 0x1c000001, 0x4d000000, 0x6f726369,
0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072,
0x392e3632, 0x322e3235, 0x00343438, 0x49ababab, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000,
0x00000000, 0x00000000, 0x03000000, 0x00000000, 0x0f000000, 0x5000000f, 0x5449534f, 0x004e4f49,
0x4fababab, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000,
0x00000000, 0x0f000000, 0x53000000, 0x4f505f56, 0x49544953, 0x53004e4f, 0x3c524448, 0x40000000,
0x0f000100, 0x5f000000, 0xf2030000, 0x00001010, 0x67000000, 0xf2040000, 0x00001020, 0x01000000,
0x36000000, 0xf2050000, 0x00001020, 0x46000000, 0x0000101e, 0x3e000000, 0x53010000, 0x74544154,
0x02000000, 0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xb4000000, 0x44000001, 0xa4434258,
0x42e88ad3, 0xcc4b1ab5, 0x5f89bf37, 0x0139edfb, 0xb4000000, 0x05000001, 0x34000000, 0x8c000000,
0xc0000000, 0xf4000000, 0x38000000, 0x52000001, 0x50464544, 0x00000000, 0x00000000, 0x00000000,
0x1c000000, 0x00000000, 0x00fffe04, 0x1c000001, 0x4d000000, 0x6f726369, 0x74666f73, 0x29522820,
0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3632, 0x322e3235,
0x00343438, 0x49ababab, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x00000000,
0x02000000, 0x00000000, 0x0f000000, 0x5000000f, 0x5449534f, 0x004e4f49, 0x4fababab, 0x2c4e4753,
0x01000000, 0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000, 0x00000000, 0x0f000000,
0x53000000, 0x4f505f56, 0x49544953, 0x53004e4f, 0x3c524448, 0x40000000, 0x0f000100, 0x5f000000,
0xf2030000, 0x00001010, 0x67000000, 0xf2040000, 0x00001020, 0x01000000, 0x2b000000, 0xf2050000,
0x00001020, 0x46000000, 0x0000101e, 0x3e000000, 0x53010000, 0x74544154, 0x02000000, 0x00000000,
0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x70000000, 0x0001b000, 0x42584400, 0xf9269a43, 0xf17ba57f,
0x6d728d8a, 0x599e9d79, 0x000001ff, 0x0001b000, 0x00000500, 0x00003400, 0x00008c00, 0x0000c000,
0x0000f400, 0x00013400, 0x45445200, 0x00005046, 0x00000000, 0x00000000, 0x00000000, 0x00001c00,
0xff040000, 0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c,
0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x36322e39, 0x3235392e, 0x3438322e, 0xabab0034,
0x475349ab, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300,
0x00000000, 0x00000f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100,
0x00000800, 0x00002000, 0x00000000, 0x00000000, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300,
0x67726154, 0xab007465, 0x444853ab, 0x00003852, 0x00004000, 0x00000e00, 0x00006500, 0x1020f203,
0x00000000, 0x00003600, 0x1020f208, 0x00000000, 0x00400200, 0x80000000, 0x8000003f, 0x0000003f,
0x80000000, 0x00003e3f, 0x41545301, 0x00007454, 0x00000200, 0x00000000, 0x00000000, 0x00000100,
0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x10006700, 0x44000002, 0x5c434258, 0x7482cd04, 0xb6d82978, 0xe48192f2, 0x01eec6be,
0x10000000, 0x05000002, 0x34000000, 0x8c000000, 0xc0000000, 0xf4000000, 0x94000000, 0x52000001,
0x50464544, 0x00000000, 0x00000000, 0x00000000, 0x1c000000, 0x00000000, 0x00475304, 0x1c000001,
0x4d000000, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c, 0x72656461, 0x6d6f4320,
0x656c6970, 0x2e392072, 0x392e3632, 0x322e3235, 0x00343438, 0x49ababab, 0x2c4e4753, 0x01000000,
0x08000000, 0x20000000, 0x00000000, 0x01000000, 0x03000000, 0x00000000, 0x0f000000, 0x5300000f,
0x4f505f56, 0x49544953, 0x4f004e4f, 0x2c4e4753, 0x01000000, 0x08000000, 0x20000000, 0x00000000,
0x01000000, 0x03000000, 0x00000000, 0x0f000000, 0x53000000, 0x4f505f56, 0x49544953, 0x53004e4f,
0x98524448, 0x40000000, 0x26000200, 0x61000000, 0xf2050000, 0x03002010, 0x00000000, 0x01000000,
0x5d000000, 0x5c010018, 0x67010028, 0xf2040000, 0x00001020, 0x01000000, 0x5e000000, 0x03020000,
0x36000000, 0xf2060000, 0x00001020, 0x46000000, 0x0000201e, 0x00000000, 0x13000000, 0x36010000,
0xf2060000, 0x00001020, 0x46000000, 0x0100201e, 0x00000000, 0x13000000, 0x36010000, 0xf2060000,
0x00001020, 0x46000000, 0x0200201e, 0x00000000, 0x13000000, 0x09010000, 0x3e010000, 0x53010000,
0x74544154, 0x08000000, 0x00000000, 0x00000000, 0x02000000, 0x00000000, 0x00000000, 0x00000000,
0x01000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01000000, 0x03000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x03000000, 0x05000000, 0x03000000, 0x00000000, 0x00000000, 0x00000000, 0x52000000, 0x65646e65,
0x30500072, 0x00315000, 0x00000001, 0x00000002, 0x00000000, 0x00000001, 0x00000002, 0x00000000,
0x00000001, 0x00000002, 0x00000000, 0x01003250, 0x02000000, 0x00000000, 0x01000000, 0x02000000,
0x00000000, 0x01000000, 0x02000000, 0x00000000, 0x50000000, 0x01b00033, 0x58440000, 0x269a4342,
0x7ba57ff9, 0x728d8af1, 0x9e9d796d, 0x0001ff59, 0x01b00000, 0x00050000, 0x00340000, 0x008c0000,
0x00c00000, 0x00f40000, 0x01340000, 0x44520000, 0x00504645, 0x00000000, 0x00000000, 0x00000000,
0x001c0000, 0x04000000, 0x0100ffff, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228,
0x4c534c48, 0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e36, 0x38322e32,
0xab003434, 0x5349abab, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000,
0x00030000, 0x00000000, 0x000f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47,
0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000, 0x00000000, 0x000f0000,
0x56530000, 0x7261545f, 0x00746567, 0x4853abab, 0x00385244, 0x00400000, 0x000e0000, 0x00650000,
0x20f20300, 0x00000010, 0x00360000, 0x20f20800, 0x00000010, 0x40020000, 0x00000000, 0x00003f80,
0x00003f80, 0x00000000, 0x003e3f80, 0x54530100, 0x00745441, 0x00020000, 0x00000000, 0x00000000,
0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x083e0000, 0x00000000, 0x01b40000, 0x58440000, 0x6d024342, 0x835f11b9,
0x9f31cdd8, 0x2dade81d, 0x00014d6a, 0x01b40000, 0x00050000, 0x00340000, 0x008c0000, 0x00c00000,
0x00f40000, 0x01380000, 0x44520000, 0x00504645, 0x00000000, 0x00000000, 0x00000000, 0x001c0000,
0x04000000, 0x0100fffe, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48,
0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e36, 0x38322e32, 0xab003434,
0x5349abab, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000,
0x00000000, 0x0f0f0000, 0x4f500000, 0x49544953, 0xab004e4f, 0x534fabab, 0x002c4e47, 0x00010000,
0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000,
0x534f505f, 0x4f495449, 0x4853004e, 0x003c5244, 0x00400000, 0x000f0001, 0x005f0000, 0x10f20300,
0x00000010, 0x00670000, 0x20f20400, 0x00000010, 0x00010000, 0x00360000, 0x20f20500, 0x00000010,
0x1e460000, 0x00000010, 0x003e0000, 0x54530100, 0x00745441, 0x00020000, 0x00000000, 0x00000000,
0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x09fa0000, 0x00000000, 0x02100000, 0x58440000, 0x045c4342, 0x787482cd,
0xf2b6d829, 0xbee48192, 0x0001eec6, 0x02100000, 0x00050000, 0x00340000, 0x008c0000, 0x00c00000,
0x00f40000, 0x01940000, 0x44520000, 0x00504645, 0x00000000, 0x00000000, 0x00000000, 0x001c0000,
0x04000000, 0x01004753, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48,
0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e36, 0x38322e32, 0xab003434,
0x5349abab, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000,
0x00000000, 0x0f0f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47, 0x00010000,
0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000,
0x534f505f, 0x4f495449, 0x4853004e, 0x00985244, 0x00400000, 0x00260002, 0x00610000, 0x10f20500,
0x00030020, 0x00000000, 0x00010000, 0x185d0000, 0x285c0100, 0x00670100, 0x20f20400, 0x00000010,
0x00010000, 0x005e0000, 0x00030200, 0x00360000, 0x20f20600, 0x00000010, 0x1e460000, 0x00000020,
0x00000000, 0x00130000, 0x00360100, 0x20f20600, 0x00000010, 0x1e460000, 0x00010020, 0x00000000,
0x00130000, 0x00360100, 0x20f20600, 0x00000010, 0x1e460000, 0x00020020, 0x00000000, 0x00130000,
0x00090100, 0x003e0100, 0x54530100, 0x00745441, 0x00080000, 0x00000000, 0x00000000, 0x00020000,
0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00010000, 0x00030000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00030000, 0x00050000, 0x00030000, 0x00000000, 0x00000000,
0x00000000, 0x0bba0000, 0x00000000, 0x34500000, 0x0001b000, 0x42584400, 0xf9269a43, 0xf17ba57f,
0x6d728d8a, 0x599e9d79, 0x000001ff, 0x0001b000, 0x00000500, 0x00003400, 0x00008c00, 0x0000c000,
0x0000f400, 0x00013400, 0x45445200, 0x00005046, 0x00000000, 0x00000000, 0x00000000, 0x00001c00,
0xff040000, 0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c,
0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x36322e39, 0x3235392e, 0x3438322e, 0xabab0034,
0x475349ab, 0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300,
0x00000000, 0x00000f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100,
0x00000800, 0x00002000, 0x00000000, 0x00000000, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300,
0x67726154, 0xab007465, 0x444853ab, 0x00003852, 0x00004000, 0x00000e00, 0x00006500, 0x1020f203,
0x00000000, 0x00003600, 0x1020f208, 0x00000000, 0x00400200, 0x80000000, 0x8000003f, 0x0000003f,
0x80000000, 0x00003e3f, 0x41545301, 0x00007454, 0x00000200, 0x00000000, 0x00000000, 0x00000100,
0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x000dd900, 0x00000000, 0x0001b400, 0x42584400, 0x8ad3a443, 0x1ab542e8, 0xbf37cc4b,
0xedfb5f89, 0x00000139, 0x0001b400, 0x00000500, 0x00003400, 0x00008c00, 0x0000c000, 0x0000f400,
0x00013800, 0x45445200, 0x00005046, 0x00000000, 0x00000000, 0x00000000, 0x00001c00, 0xfe040000,
0x000100ff, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853,
0x43207265, 0x69706d6f, 0x2072656c, 0x36322e39, 0x3235392e, 0x3438322e, 0xabab0034, 0x475349ab,
0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000000, 0x00000200, 0x00000000,
0x000f0f00, 0x534f5000, 0x4f495449, 0xabab004e, 0x47534fab, 0x00002c4e, 0x00000100, 0x00000800,
0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x49534f50,
0x4e4f4954, 0x44485300, 0x00003c52, 0x01004000, 0x00000f00, 0x00005f00, 0x1010f203, 0x00000000,
0x00006700, 0x1020f204, 0x00000000, 0x00000100, 0x00002b00, 0x1020f205, 0x00000000, 0x101e4600,
0x00000000, 0x00003e00, 0x41545301, 0x00007454, 0x00000200, 0x00000000, 0x00000000, 0x00000200,
0x00000000, 0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x000f9500, 0x00000000, 0x00021000, 0x42584400, 0xcd045c43, 0x29787482, 0x92f2b6d8,
0xc6bee481, 0x000001ee, 0x00021000, 0x00000500, 0x00003400, 0x00008c00, 0x0000c000, 0x0000f400,
0x00019400, 0x45445200, 0x00005046, 0x00000000, 0x00000000, 0x00000000, 0x00001c00, 0x53040000,
0x00010047, 0x00001c00, 0x63694d00, 0x6f736f72, 0x28207466, 0x48202952, 0x204c534c, 0x64616853,
0x43207265, 0x69706d6f, 0x2072656c, 0x36322e39, 0x3235392e, 0x3438322e, 0xabab0034, 0x475349ab,
0x00002c4e, 0x00000100, 0x00000800, 0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000,
0x000f0f00, 0x5f565300, 0x49534f50, 0x4e4f4954, 0x47534f00, 0x00002c4e, 0x00000100, 0x00000800,
0x00002000, 0x00000000, 0x00000100, 0x00000300, 0x00000000, 0x00000f00, 0x5f565300, 0x49534f50,
0x4e4f4954, 0x44485300, 0x00009852, 0x02004000, 0x00002600, 0x00006100, 0x2010f205, 0x00000300,
0x00000000, 0x00000100, 0x00185d00, 0x00285c01, 0x00006701, 0x1020f204, 0x00000000, 0x00000100,
0x00005e00, 0x00000302, 0x00003600, 0x1020f206, 0x00000000, 0x201e4600, 0x00000000, 0x00000000,
0x00001300, 0x00003601, 0x1020f206, 0x00000000, 0x201e4600, 0x00000100, 0x00000000, 0x00001300,
0x00003601, 0x1020f206, 0x00000000, 0x201e4600, 0x00000200, 0x00000000, 0x00001300, 0x00000901,
0x00003e01, 0x41545301, 0x00007454, 0x00000800, 0x00000000, 0x00000000, 0x00000200, 0x00000000,
0x00000000, 0x00000000, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100,
0x00000300, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000300, 0x00000500, 0x00000300, 0x00000000, 0x00000000, 0x00000000,
0x00115500, 0x00000000, 0x00355000, 0xa5003650, 0x00000000, 0x50000000, 0x00a50037, 0x00010000,
0x002d0000, 0x00110000, 0x00000000, 0xffff0000, 0x0000ffff, 0x00000000, 0x00580000, 0x003c0000,
0x00000000, 0xffff0000, 0x0000ffff, 0x00000000, 0x00860000, 0x006a0000, 0x00000000, 0xffff0000,
0x0000ffff, 0x00000000, 0x00a50000, 0x00890000, 0x00000000, 0xffff0000, 0x00a7ffff, 0x025f0000,
0x00000000, 0x04170000, 0x003c0000, 0x00000000, 0xffff0000, 0x0419ffff, 0x00000000, 0x05cd0000,
0x006a0000, 0x00000000, 0xffff0000, 0x05cfffff, 0x00000000, 0x07e30000, 0x00080000, 0x00000000,
0x07ea0000, 0x00000000, 0x00000000, 0x07ed0000, 0x00030000, 0x00000000, 0x00070000, 0x00000000,
0x00010000, 0x07f00000, 0x00060000, 0x00000000, 0x00010000, 0x07fc0000, 0x00080000, 0x00000000,
0x00010000, 0x08080000, 0x08140000, 0x00030000, 0x00000000, 0x00070000, 0x00000000, 0x00010000,
0x08170000, 0x00060000, 0x00000000, 0x00010000, 0x08230000, 0x00080000, 0x00000000, 0x00010000,
0x082f0000, 0x083b0000, 0x00030000, 0x00000000, 0x00070000, 0x00000000, 0x00070000, 0x09f20000,
0x00060000, 0x00000000, 0x00070000, 0x0bb20000, 0x00080000, 0x00000000, 0x00070000, 0x0dce0000,
0x0dd60000, 0x00030000, 0x00000000, 0x00070000, 0x00000000, 0x00070000, 0x0f8d0000, 0x00060000,
0x00000000, 0x00070000, 0x114d0000, 0x00080000, 0x00000000, 0x00070000, 0x13690000, 0x13710000,
0x00030000, 0x00000000, 0x00070000, 0x00000000, 0x00020000, 0x00580000, 0x00060000, 0x00000000,
0x00020000, 0x002d0000, 0x00080000, 0x00000000, 0x00020000, 0x00860000, 0x13740000, 0x00030000,
0x00000000, 0x00070000, 0x00000000, 0x00020000, 0x04170000, 0x00060000, 0x00000000, 0x00030000,
0x13770000, 0x00080000, 0x00000000, 0x00020000, 0x05cd0000, 0x137f0000, 0x00030000, 0x00000000,
0x00070000, 0x00000000, 0x00020000, 0x04170000, 0x00060000, 0x00000000, 0x00030000, 0x13820000,
0x00080000, 0x00000000, 0x00020000, 0x05cd0000, 0x00000000,
};

static void test_effect_local_shader(void)
{
    HRESULT hr;
    BOOL ret;
    ID3D10Effect* effect;
    ID3D10EffectVariable* v;
    ID3D10EffectPass *p, *p2, *null_pass;
    ID3D10EffectTechnique *t, *t2, *null_technique;
    D3D10_PASS_SHADER_DESC pdesc = {0};
    D3D10_EFFECT_VARIABLE_DESC vdesc = {0};
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC typedesc;
    ID3D10EffectShaderVariable *null_shader, *null_anon_vs, *null_anon_ps, *null_anon_gs,
        *p3_anon_vs, *p3_anon_ps, *p3_anon_gs, *p6_vs, *p6_ps, *p6_gs;
    ID3D10Device *device;
    ULONG refcount;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_local_shader, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed!\n");

    null_technique = effect->lpVtbl->GetTechniqueByIndex(effect, 0xffffffff);
    null_pass = null_technique->lpVtbl->GetPassByIndex(null_technique, 0xffffffff);

    /* check technique */
    t = effect->lpVtbl->GetTechniqueByName(effect, NULL);
    ok(null_technique == t, "GetTechniqueByName got %p, expected %p\n", t, null_technique);

    t = effect->lpVtbl->GetTechniqueByName(effect, "invalid");
    ok(null_technique == t, "GetTechniqueByName got %p, expected %p\n", t, null_technique);

    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    ok(null_technique != t, "GetTechniqueByIndex failed %p\n", t);

    t2 = effect->lpVtbl->GetTechniqueByName(effect, "Render");
    ok(t2 == t, "GetTechniqueByName got %p, expected %p\n", t2, t);

    /* check invalid pass arguments */
    p = null_technique->lpVtbl->GetPassByName(null_technique, NULL);
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);

    p = null_technique->lpVtbl->GetPassByName(null_technique, "invalid");
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);

if (0)
{
    /* This crashes on W7/DX10, if t is a valid technique and name=NULL. */
    p = t->lpVtbl->GetPassByName(t, NULL);
    ok(null_pass == p, "GetPassByName got %p, expected %p\n", p, null_pass);
}

    p = t->lpVtbl->GetPassByIndex(t, 0xffffffff);
    ok(null_pass == p, "GetPassByIndex got %p, expected %p\n", p, null_pass);

    /* check for invalid arguments */
    hr = null_pass->lpVtbl->GetVertexShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "GetVertexShaderDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_pass->lpVtbl->GetVertexShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "GetVertexShaderDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_pass->lpVtbl->GetPixelShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "GetPixelShaderDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_pass->lpVtbl->GetPixelShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "GetPixelShaderDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_pass->lpVtbl->GetGeometryShaderDesc(null_pass, NULL);
    ok(hr == E_FAIL, "GetGeometryShaderDesc got %x, expected %x\n", hr, E_FAIL);

    hr = null_pass->lpVtbl->GetGeometryShaderDesc(null_pass, &pdesc);
    ok(hr == E_FAIL, "GetGeometryShaderDesc got %x, expected %x\n", hr, E_FAIL);

    /* check valid pass arguments */
    t = effect->lpVtbl->GetTechniqueByIndex(effect, 0);
    p = t->lpVtbl->GetPassByIndex(t, 0);

    p2 = t->lpVtbl->GetPassByName(t, "P0");
    ok(p2 == p, "GetPassByName got %p, expected %p\n", p2, p);

    hr = p->lpVtbl->GetVertexShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "GetVertexShaderDesc got %x, expected %x\n", hr, E_INVALIDARG);

    hr = p->lpVtbl->GetPixelShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "GetPixelShaderDesc got %x, expected %x\n", hr, E_INVALIDARG);

    hr = p->lpVtbl->GetGeometryShaderDesc(p, NULL);
    ok(hr == E_INVALIDARG, "GetGeometryShaderDesc got %x, expected %x\n", hr, E_INVALIDARG);

    /* get the null_shader_variable */
    v = effect->lpVtbl->GetVariableByIndex(effect, 10000);
    null_shader = v->lpVtbl->AsShader(v);

    /* pass 0 */
    p = t->lpVtbl->GetPassByIndex(t, 0);
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "GetDesc failed (%x)\n", hr);

    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "GetDesc failed (%x)\n", hr);

    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_shader, "Got %p, expected %p\n", pdesc.pShaderVariable, null_shader);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == E_FAIL, "GetDesc failed (%x)\n", hr);

    /* pass 1 */
    p = t->lpVtbl->GetPassByIndex(t, 1);

    /* pass 1 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    null_anon_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexhader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 1 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    null_anon_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 1 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    null_anon_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "GetDesc failed (%x) expected %x\n", hr, S_OK);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 2 */
    p = t->lpVtbl->GetPassByIndex(t, 2);

    /* pass 2 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_anon_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_vs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 2 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_anon_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 2 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == null_anon_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, null_anon_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(hr == S_OK, "GetDesc failed (%x) expected %x\n", hr, S_OK);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    /* pass 3 */
    p = t->lpVtbl->GetPassByIndex(t, 3);

    /* pass 3 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    p3_anon_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 3 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    p3_anon_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 3 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    p3_anon_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 */
    p = t->lpVtbl->GetPassByIndex(t, 4);

    /* pass 4 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable != p3_anon_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_vs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "vertexshader") == 0, "TypeName is \"%s\", expected \"vertexshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable != p3_anon_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "pixelshader") == 0, "TypeName is \"%s\", expected \"pixelshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 4 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable != p3_anon_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, p3_anon_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %x\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "$Anonymous") == 0, "Name is \"%s\", expected \"$Anonymous\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected 0\n", vdesc.ExplicitBindPoint);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "geometryshader") == 0, "TypeName is \"%s\", expected \"geometryshader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 */
    p = t->lpVtbl->GetPassByIndex(t, 5);

    /* pass 5 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "v0") == 0, "Name is \"%s\", expected \"v0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "p0") == 0, "Name is \"%s\", expected \"p0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 5 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "g0") == 0, "Name is \"%s\", expected \"g0\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 6 */
    p = t->lpVtbl->GetPassByIndex(t, 6);

    /* pass 6 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    p6_vs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "v") == 0, "Name is \"%s\", expected \"v\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 2, "Elements is %u, expected 2\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 6 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    p6_ps = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "p") == 0, "Name is \"%s\", expected \"p\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 6 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    p6_gs = pdesc.pShaderVariable;
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "g") == 0, "Name is \"%s\", expected \"g\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 */
    p = t->lpVtbl->GetPassByIndex(t, 7);

    /* pass 7 vertexshader */
    hr = p->lpVtbl->GetVertexShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetVertexShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == p6_vs, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_vs);
    ok(pdesc.ShaderIndex == 1, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 1);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "v") == 0, "Name is \"%s\", expected \"v\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "VertexShader") == 0, "TypeName is \"%s\", expected \"VertexShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_VERTEXSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_VERTEXSHADER);
    ok(typedesc.Elements == 2, "Elements is %u, expected 2\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 pixelshader */
    hr = p->lpVtbl->GetPixelShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetPixelShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == p6_ps, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_ps);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "p") == 0, "Name is \"%s\", expected \"p\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "PixelShader") == 0, "TypeName is \"%s\", expected \"PixelShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_PIXELSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_PIXELSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    /* pass 7 geometryshader */
    hr = p->lpVtbl->GetGeometryShaderDesc(p, &pdesc);
    ok(hr == S_OK, "GetGeometryShaderDesc got %x, expected %x\n", hr, S_OK);
    ok(pdesc.pShaderVariable == p6_gs, "Got %p, expected %p\n", pdesc.pShaderVariable, p6_gs);
    ok(pdesc.ShaderIndex == 0, "ShaderIndex is %u, expected %u\n", pdesc.ShaderIndex, 0);

    hr = pdesc.pShaderVariable->lpVtbl->GetDesc(pdesc.pShaderVariable, &vdesc);
    ok(SUCCEEDED(hr), "GetDesc failed (%x)\n", hr);

    ok(strcmp(vdesc.Name, "g") == 0, "Name is \"%s\", expected \"g\"\n", vdesc.Name);
    ok(vdesc.Semantic == NULL, "Semantic is \"%s\", expected NULL\n", vdesc.Semantic);
    ok(vdesc.Flags == 0, "Flags is %u, expected 0\n", vdesc.Flags);
    ok(vdesc.Annotations == 0, "Annotations is %u, expected 0\n", vdesc.Annotations);
    ok(vdesc.BufferOffset == 0, "BufferOffset is %u, expected 0\n", vdesc.BufferOffset);
    ok(vdesc.ExplicitBindPoint == 0, "ExplicitBindPoint is %u, expected %u\n", vdesc.ExplicitBindPoint, 0);

    ret = pdesc.pShaderVariable->lpVtbl->IsValid(pdesc.pShaderVariable);
    ok(ret, "IsValid() failed\n");

    type = pdesc.pShaderVariable->lpVtbl->GetType(pdesc.pShaderVariable);
    ret = type->lpVtbl->IsValid(type);
    ok(ret, "IsValid() failed\n");

    hr = type->lpVtbl->GetDesc(type, &typedesc);
    ok(hr == S_OK, "GetDesc failed (%x)\n", hr);
    ok(strcmp(typedesc.TypeName, "GeometryShader") == 0, "TypeName is \"%s\", expected \"GeometryShader\"\n", typedesc.TypeName);
    ok(typedesc.Class == D3D10_SVC_OBJECT, "Class is %x, expected %x\n", typedesc.Class, D3D10_SVC_OBJECT);
    ok(typedesc.Type == D3D10_SVT_GEOMETRYSHADER, "Type is %x, expected %x\n", typedesc.Type, D3D10_SVT_GEOMETRYSHADER);
    ok(typedesc.Elements == 0, "Elements is %u, expected 0\n", typedesc.Elements);
    ok(typedesc.Members == 0, "Members is %u, expected 0\n", typedesc.Members);
    ok(typedesc.Rows == 0, "Rows is %u, expected 0\n", typedesc.Rows);
    ok(typedesc.Columns == 0, "Columns is %u, expected 0\n", typedesc.Columns);
    ok(typedesc.PackedSize == 0x0, "PackedSize is %#x, expected 0x0\n", typedesc.PackedSize);
    ok(typedesc.UnpackedSize == 0x0, "UnpackedSize is %#x, expected 0x0\n", typedesc.UnpackedSize);
    ok(typedesc.Stride == 0x0, "Stride is %#x, expected 0x0\n", typedesc.Stride);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_get_variable_by
 */
#if 0
cbuffer cb
{
    float   f1 : SV_POSITION;
    float   f2 : COLOR0;
};
cbuffer cb2
{
    float   f3 : SV_POSITION;
    float   f4 : COLOR1;
};
Texture1D tex1 : COLOR2;
Texture1D tex2 : COLOR1;
#endif
static DWORD fx_test_egvb[] = {
0x43425844, 0x63d60ede, 0xf75a09d1, 0x47da5604, 0x7ef6e331, 0x00000001, 0x000001ca, 0x00000001,
0x00000024, 0x30315846, 0x0000019e, 0xfeff1001, 0x00000002, 0x00000004, 0x00000002, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x0000008a, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x66006263,
0x74616f6c, 0x00000700, 0x00000100, 0x00000000, 0x00000400, 0x00001000, 0x00000400, 0x00090900,
0x00316600, 0x505f5653, 0x5449534f, 0x004e4f49, 0x43003266, 0x524f4c4f, 0x62630030, 0x33660032,
0x00346600, 0x4f4c4f43, 0x54003152, 0x75747865, 0x44316572, 0x00005300, 0x00000200, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000a00, 0x78657400, 0x4f430031, 0x32524f4c, 0x78657400,
0x00040032, 0x00100000, 0x00000000, 0x00020000, 0xffff0000, 0x0000ffff, 0x00290000, 0x000d0000,
0x002c0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00380000, 0x000d0000, 0x003b0000,
0x00040000, 0x00000000, 0x00000000, 0x00000000, 0x00420000, 0x00100000, 0x00000000, 0x00020000,
0xffff0000, 0x0000ffff, 0x00460000, 0x000d0000, 0x002c0000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00490000, 0x000d0000, 0x004c0000, 0x00040000, 0x00000000, 0x00000000, 0x00000000,
0x00790000, 0x005d0000, 0x007e0000, 0xffff0000, 0x0000ffff, 0x00850000, 0x005d0000, 0x004c0000,
0xffff0000, 0x0000ffff, 0x00000000,
};

static void test_effect_get_variable_by(void)
{
    ID3D10Effect *effect;
    ID3D10EffectVariable *variable_by_index, *variable, *null_variable;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_egvb, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    /* get the null variable */
    null_variable = effect->lpVtbl->GetVariableByIndex(effect, 0xffffffff);

    /* check for invalid arguments */
    variable = effect->lpVtbl->GetVariableByName(effect, NULL);
    ok(null_variable == variable, "GetVariableByName got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, NULL);
    ok(null_variable == variable, "GetVariableBySemantic got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableByName(effect, "invalid");
    ok(null_variable == variable, "GetVariableByName got %p, expected %p\n", variable, null_variable);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "invalid");
    ok(null_variable == variable, "GetVariableBySemantic got %p, expected %p\n", variable, null_variable);

    /* variable f1 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 0);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f1");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "SV_POSITION");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable f2 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 1);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f2");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR0");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable f3 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 2);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f3");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "SV_POSITION");
    ok(variable != null_variable, "GetVariableBySemantic failed %p\n", variable);
    ok(variable != variable_by_index, "GetVariableBySemantic failed %p\n", variable);

    /* variable f4 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 3);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "f4");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR1");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable tex1 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 4);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "tex1");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR2");
    ok(variable_by_index == variable, "GetVariableBySemantic got %p, expected %p\n", variable, variable_by_index);

    /* variable tex2 */
    variable_by_index = effect->lpVtbl->GetVariableByIndex(effect, 5);
    ok(null_variable != variable_by_index, "GetVariableByIndex failed %p\n", variable_by_index);

    variable = effect->lpVtbl->GetVariableByName(effect, "tex2");
    ok(variable_by_index == variable, "GetVariableByName got %p, expected %p\n", variable, variable_by_index);

    variable = effect->lpVtbl->GetVariableBySemantic(effect, "COLOR1");
    ok(variable != null_variable, "GetVariableBySemantic failed %p\n", variable);
    ok(variable != variable_by_index, "GetVariableBySemantic failed %p\n", variable);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

#if 0
RasterizerState rast_state
{
    FillMode = wireframe;                       /* 0x0c */
    CullMode = front;                           /* 0x0d */
    FrontCounterClockwise = true;               /* 0x0e */
    DepthBias = -4;                             /* 0x0f */
    DepthBiasClamp = 0.5f;                      /* 0x10 */
    SlopeScaledDepthBias = 0.25f;               /* 0x11 */
    DepthClipEnable = false;                    /* 0x12 */
    ScissorEnable = true;                       /* 0x13 */
    MultisampleEnable = true;                   /* 0x14 */
    AntialiasedLineEnable = true;               /* 0x15 */
};

DepthStencilState ds_state
{
    DepthEnable = true;                         /* 0x16 */
    DepthWriteMask = zero;                      /* 0x17 */
    DepthFunc = equal;                          /* 0x18 */
    StencilEnable = true;                       /* 0x19 */
    StencilReadMask = 0x4;                      /* 0x1a */
    StencilWriteMask = 0x5;                     /* 0x1b */
    FrontFaceStencilFail = invert;              /* 0x1c */
    FrontFaceStencilDepthFail = incr;           /* 0x1d */
    FrontFaceStencilPass = decr;                /* 0x1e */
    FrontFaceStencilFunc = less_equal;          /* 0x1f */
    BackFaceStencilFail = replace;              /* 0x20 */
    BackFaceStencilDepthFail = incr_sat;        /* 0x21 */
    BackFaceStencilPass = decr_sat;             /* 0x22 */
    BackFaceStencilFunc = greater_equal;        /* 0x23 */
};

BlendState blend_state
{
    AlphaToCoverageEnable = false;              /* 0x24 */
    BlendEnable[0] = true;                      /* 0x25[0] */
    BlendEnable[7] = false;                     /* 0x25[7] */
    SrcBlend = one;                             /* 0x26 */
    DestBlend = src_color;                      /* 0x27 */
    BlendOp = min;                              /* 0x28 */
    SrcBlendAlpha = src_alpha;                  /* 0x29 */
    DestBlendAlpha = inv_src_alpha;             /* 0x2a */
    BlendOpAlpha = max;                         /* 0x2b */
    RenderTargetWriteMask[0] = 0x8;             /* 0x2c[0] */
    RenderTargetWriteMask[7] = 0x7;             /* 0x2c[7] */
};

SamplerState sampler0
{
    Filter = min_mag_mip_linear;                /* 0x2d */
    AddressU = wrap;                            /* 0x2e */
    AddressV = mirror;                          /* 0x2f */
    AddressW = clamp;                           /* 0x30 */
    MipMapLODBias = -1;                         /* 0x31 */
    MaxAnisotropy = 4u;                         /* 0x32 */
    ComparisonFunc = always;                    /* 0x33 */
    BorderColor = float4(1.0, 2.0, 3.0, 4.0);   /* 0x34 */
    MinLOD = 6u;                                /* 0x35 */
    MaxLOD = 5u;                                /* 0x36 */
};

technique10 tech0
{
    pass pass0
    {
        SetBlendState(blend_state, float4(0.5f, 0.6f, 0.7f, 0.8f), 0xffff);
        SetDepthStencilState(ds_state, 1.0f);
        SetRasterizerState(rast_state);
    }
};
#endif
static DWORD fx_test_state_groups[] =
{
    0x43425844, 0xbf7e3418, 0xd2838ea5, 0x8012c315,
    0x7dd76ca7, 0x00000001, 0x00000794, 0x00000001,
    0x00000024, 0x30315846, 0x00000768, 0xfeff1001,
    0x00000001, 0x00000000, 0x00000004, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x0000035c,
    0x00000000, 0x00000000, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x73615200, 0x69726574, 0x5372657a,
    0x65746174, 0x00000d00, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000400,
    0x73617200, 0x74735f74, 0x00657461, 0x00000001,
    0x00000002, 0x00000002, 0x00000001, 0x00000002,
    0x00000002, 0x00000001, 0x00000004, 0x00000001,
    0x00000001, 0x00000002, 0xfffffffc, 0x00000001,
    0x00000001, 0x3f000000, 0x00000001, 0x00000001,
    0x3e800000, 0x00000001, 0x00000004, 0x00000000,
    0x00000001, 0x00000004, 0x00000001, 0x00000001,
    0x00000004, 0x00000001, 0x00000001, 0x00000004,
    0x00000001, 0x74706544, 0x65745368, 0x6c69636e,
    0x74617453, 0x00bc0065, 0x00020000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00030000,
    0x73640000, 0x6174735f, 0x01006574, 0x04000000,
    0x01000000, 0x01000000, 0x02000000, 0x00000000,
    0x01000000, 0x02000000, 0x03000000, 0x01000000,
    0x04000000, 0x01000000, 0x01000000, 0x02000000,
    0x04000000, 0x01000000, 0x02000000, 0x05000000,
    0x01000000, 0x02000000, 0x06000000, 0x01000000,
    0x02000000, 0x07000000, 0x01000000, 0x02000000,
    0x08000000, 0x01000000, 0x02000000, 0x04000000,
    0x01000000, 0x02000000, 0x03000000, 0x01000000,
    0x02000000, 0x04000000, 0x01000000, 0x02000000,
    0x05000000, 0x01000000, 0x02000000, 0x07000000,
    0x42000000, 0x646e656c, 0x74617453, 0x019b0065,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00020000, 0x6c620000, 0x5f646e65,
    0x74617473, 0x00010065, 0x00040000, 0x00000000,
    0x00010000, 0x00040000, 0x00010000, 0x00010000,
    0x00040000, 0x00000000, 0x00010000, 0x00020000,
    0x00020000, 0x00010000, 0x00020000, 0x00030000,
    0x00010000, 0x00020000, 0x00040000, 0x00010000,
    0x00020000, 0x00050000, 0x00010000, 0x00020000,
    0x00060000, 0x00010000, 0x00020000, 0x00050000,
    0x00010000, 0x00020000, 0x00080000, 0x00010000,
    0x00020000, 0x00070000, 0x61530000, 0x656c706d,
    0x61745372, 0x52006574, 0x02000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x15000000,
    0x73000000, 0x6c706d61, 0x00307265, 0x00000001,
    0x00000002, 0x00000015, 0x00000001, 0x00000002,
    0x00000001, 0x00000001, 0x00000002, 0x00000002,
    0x00000001, 0x00000002, 0x00000003, 0x00000001,
    0x00000002, 0xffffffff, 0x00000001, 0x00000002,
    0x00000004, 0x00000001, 0x00000002, 0x00000008,
    0x00000004, 0x00000001, 0x3f800000, 0x00000001,
    0x40000000, 0x00000001, 0x40400000, 0x00000001,
    0x40800000, 0x00000001, 0x00000002, 0x00000006,
    0x00000001, 0x00000002, 0x00000005, 0x68636574,
    0x61700030, 0x00307373, 0x00000004, 0x00000001,
    0x3f000000, 0x00000001, 0x3f19999a, 0x00000001,
    0x3f333333, 0x00000001, 0x3f4ccccd, 0x00000001,
    0x00000002, 0x0000ffff, 0x00000001, 0x00000001,
    0x3f800000, 0x00000004, 0x00000000, 0x00000000,
    0x00000000, 0xffffffff, 0x00000000, 0x00000039,
    0x0000001d, 0x00000000, 0xffffffff, 0x0000000a,
    0x0000000c, 0x00000000, 0x00000001, 0x00000044,
    0x0000000d, 0x00000000, 0x00000001, 0x00000050,
    0x0000000e, 0x00000000, 0x00000001, 0x0000005c,
    0x0000000f, 0x00000000, 0x00000001, 0x00000068,
    0x00000010, 0x00000000, 0x00000001, 0x00000074,
    0x00000011, 0x00000000, 0x00000001, 0x00000080,
    0x00000012, 0x00000000, 0x00000001, 0x0000008c,
    0x00000013, 0x00000000, 0x00000001, 0x00000098,
    0x00000014, 0x00000000, 0x00000001, 0x000000a4,
    0x00000015, 0x00000000, 0x00000001, 0x000000b0,
    0x00000000, 0x000000ea, 0x000000ce, 0x00000000,
    0xffffffff, 0x0000000e, 0x00000016, 0x00000000,
    0x00000001, 0x000000f3, 0x00000017, 0x00000000,
    0x00000001, 0x000000ff, 0x00000018, 0x00000000,
    0x00000001, 0x0000010b, 0x00000019, 0x00000000,
    0x00000001, 0x00000117, 0x0000001a, 0x00000000,
    0x00000001, 0x00000123, 0x0000001b, 0x00000000,
    0x00000001, 0x0000012f, 0x0000001c, 0x00000000,
    0x00000001, 0x0000013b, 0x0000001d, 0x00000000,
    0x00000001, 0x00000147, 0x0000001e, 0x00000000,
    0x00000001, 0x00000153, 0x0000001f, 0x00000000,
    0x00000001, 0x0000015f, 0x00000020, 0x00000000,
    0x00000001, 0x0000016b, 0x00000021, 0x00000000,
    0x00000001, 0x00000177, 0x00000022, 0x00000000,
    0x00000001, 0x00000183, 0x00000023, 0x00000000,
    0x00000001, 0x0000018f, 0x00000000, 0x000001c2,
    0x000001a6, 0x00000000, 0xffffffff, 0x0000000b,
    0x00000024, 0x00000000, 0x00000001, 0x000001ce,
    0x00000025, 0x00000000, 0x00000001, 0x000001da,
    0x00000025, 0x00000007, 0x00000001, 0x000001e6,
    0x00000026, 0x00000000, 0x00000001, 0x000001f2,
    0x00000027, 0x00000000, 0x00000001, 0x000001fe,
    0x00000028, 0x00000000, 0x00000001, 0x0000020a,
    0x00000029, 0x00000000, 0x00000001, 0x00000216,
    0x0000002a, 0x00000000, 0x00000001, 0x00000222,
    0x0000002b, 0x00000000, 0x00000001, 0x0000022e,
    0x0000002c, 0x00000000, 0x00000001, 0x0000023a,
    0x0000002c, 0x00000007, 0x00000001, 0x00000246,
    0x00000000, 0x0000027b, 0x0000025f, 0x00000000,
    0xffffffff, 0x0000000a, 0x0000002d, 0x00000000,
    0x00000001, 0x00000284, 0x0000002e, 0x00000000,
    0x00000001, 0x00000290, 0x0000002f, 0x00000000,
    0x00000001, 0x0000029c, 0x00000030, 0x00000000,
    0x00000001, 0x000002a8, 0x00000031, 0x00000000,
    0x00000001, 0x000002b4, 0x00000032, 0x00000000,
    0x00000001, 0x000002c0, 0x00000033, 0x00000000,
    0x00000001, 0x000002cc, 0x00000034, 0x00000000,
    0x00000001, 0x000002d8, 0x00000035, 0x00000000,
    0x00000001, 0x000002fc, 0x00000036, 0x00000000,
    0x00000001, 0x00000308, 0x00000000, 0x00000314,
    0x00000001, 0x00000000, 0x0000031a, 0x00000006,
    0x00000000, 0x0000000a, 0x00000000, 0x00000001,
    0x00000320, 0x0000000b, 0x00000000, 0x00000001,
    0x00000344, 0x00000002, 0x00000000, 0x00000002,
    0x000001c2, 0x00000009, 0x00000000, 0x00000001,
    0x00000350, 0x00000001, 0x00000000, 0x00000002,
    0x000000ea, 0x00000000, 0x00000000, 0x00000002,
    0x00000039,
};

static void test_effect_state_groups(void)
{
    ID3D10EffectDepthStencilVariable *d;
    ID3D10EffectRasterizerVariable *r;
    ID3D10DepthStencilState *ds_state;
    ID3D10RasterizerState *rast_state;
    ID3D10EffectTechnique *technique;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    D3D10_RASTERIZER_DESC rast_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10EffectSamplerVariable *s;
    ID3D10BlendState *blend_state;
    UINT sample_mask, stencil_ref;
    ID3D10EffectBlendVariable *b;
    D3D10_BLEND_DESC blend_desc;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    float blend_factor[4];
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_state_groups, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "Failed to create effect, hr %#x.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "sampler0");
    s = v->lpVtbl->AsSampler(v);
    s->lpVtbl->GetBackingStore(s, 0, &sampler_desc);
    ok(sampler_desc.Filter == D3D10_FILTER_MIN_MAG_MIP_LINEAR, "Got unexpected Filter %#x.\n", sampler_desc.Filter);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    ok(sampler_desc.AddressV == D3D10_TEXTURE_ADDRESS_MIRROR, "Got unexpected AddressV %#x.\n", sampler_desc.AddressV);
    ok(sampler_desc.AddressW == D3D10_TEXTURE_ADDRESS_CLAMP, "Got unexpected AddressW %#x.\n", sampler_desc.AddressW);
    ok(sampler_desc.MipLODBias == -1.0f, "Got unexpected MipLODBias %.8e.\n", sampler_desc.MipLODBias);
    ok(sampler_desc.MaxAnisotropy == 4, "Got unexpected MaxAnisotropy %#x.\n", sampler_desc.MaxAnisotropy);
    ok(sampler_desc.ComparisonFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected ComparisonFunc %#x.\n",
            sampler_desc.ComparisonFunc);
    ok(sampler_desc.BorderColor[0] == 1.0f, "Got unexpected BorderColor[0] %.8e.\n", sampler_desc.BorderColor[0]);
    ok(sampler_desc.BorderColor[1] == 2.0f, "Got unexpected BorderColor[1] %.8e.\n", sampler_desc.BorderColor[1]);
    ok(sampler_desc.BorderColor[2] == 3.0f, "Got unexpected BorderColor[2] %.8e.\n", sampler_desc.BorderColor[2]);
    ok(sampler_desc.BorderColor[3] == 4.0f, "Got unexpected BorderColor[3] %.8e.\n", sampler_desc.BorderColor[3]);
    ok(sampler_desc.MinLOD == 6.0f, "Got unexpected MinLOD %.8e.\n", sampler_desc.MinLOD);
    ok(sampler_desc.MaxLOD == 5.0f, "Got unexpected MaxLOD %.8e.\n", sampler_desc.MaxLOD);

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    b = v->lpVtbl->AsBlend(v);
    b->lpVtbl->GetBackingStore(b, 0, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_ONE, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_SRC_COLOR, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_MIN, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_MAX, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0x8, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0x7, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);

    v = effect->lpVtbl->GetVariableByName(effect, "ds_state");
    d = v->lpVtbl->AsDepthStencil(v);
    d->lpVtbl->GetBackingStore(d, 0, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ZERO, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_EQUAL, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0x4, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0x5, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_INVERT, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_DECR, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_LESS_EQUAL, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_REPLACE, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR_SAT,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_DECR_SAT, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_GREATER_EQUAL, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);

    v = effect->lpVtbl->GetVariableByName(effect, "rast_state");
    r = v->lpVtbl->AsRasterizer(v);
    r->lpVtbl->GetBackingStore(r, 0, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_WIREFRAME, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_FRONT, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == -4, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.5f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.25f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(!rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);

    technique = effect->lpVtbl->GetTechniqueByName(effect, "tech0");
    ok(!!technique, "Failed to get technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "pass0");
    ok(!!pass, "Failed to get pass.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(SUCCEEDED(hr), "Failed to get pass desc, hr %#x.\n", hr);
    ok(!strcmp(pass_desc.Name, "pass0"), "Got unexpected Name \"%s\".\n", pass_desc.Name);
    ok(!pass_desc.Annotations, "Got unexpected Annotations %#x.\n", pass_desc.Annotations);
    ok(!pass_desc.pIAInputSignature, "Got unexpected pIAInputSignature %p.\n", pass_desc.pIAInputSignature);
    ok(pass_desc.StencilRef == 1, "Got unexpected StencilRef %#x.\n", pass_desc.StencilRef);
    ok(pass_desc.SampleMask == 0xffff, "Got unexpected SampleMask %#x.\n", pass_desc.SampleMask);
    ok(pass_desc.BlendFactor[0] == 0.5f, "Got unexpected BlendFactor[0] %.8e.\n", pass_desc.BlendFactor[0]);
    ok(pass_desc.BlendFactor[1] == 0.6f, "Got unexpected BlendFactor[1] %.8e.\n", pass_desc.BlendFactor[1]);
    ok(pass_desc.BlendFactor[2] == 0.7f, "Got unexpected BlendFactor[2] %.8e.\n", pass_desc.BlendFactor[2]);
    ok(pass_desc.BlendFactor[3] == 0.8f, "Got unexpected BlendFactor[3] %.8e.\n", pass_desc.BlendFactor[3]);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(SUCCEEDED(hr), "Failed to apply pass, hr %#x.\n", hr);

    ID3D10Device_OMGetBlendState(device, &blend_state, blend_factor, &sample_mask);
    ID3D10BlendState_GetDesc(blend_state, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_ONE, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_SRC_COLOR, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_MIN, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_MAX, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0x8, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0x7, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);
    ok(blend_factor[0] == 0.5f, "Got unexpected blend_factor[0] %.8e.\n", blend_factor[0]);
    ok(blend_factor[1] == 0.6f, "Got unexpected blend_factor[1] %.8e.\n", blend_factor[1]);
    ok(blend_factor[2] == 0.7f, "Got unexpected blend_factor[2] %.8e.\n", blend_factor[2]);
    ok(blend_factor[3] == 0.8f, "Got unexpected blend_factor[3] %.8e.\n", blend_factor[3]);
    ok(sample_mask == 0xffff, "Got unexpected sample_mask %#x.\n", sample_mask);

    ID3D10Device_OMGetDepthStencilState(device, &ds_state, &stencil_ref);
    ID3D10DepthStencilState_GetDesc(ds_state, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ZERO, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_EQUAL, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0x4, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0x5, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_INVERT, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_DECR, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_LESS_EQUAL, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_REPLACE, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_INCR_SAT,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_DECR_SAT, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_GREATER_EQUAL, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);
    ok(stencil_ref == 1, "Got unexpected stencil_ref %#x.\n", stencil_ref);

    ID3D10Device_RSGetState(device, &rast_state);
    ID3D10RasterizerState_GetDesc(rast_state, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_WIREFRAME, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_FRONT, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == -4, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.5f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.25f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(!rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);

    ID3D10RasterizerState_Release(rast_state);
    ID3D10DepthStencilState_Release(ds_state);
    ID3D10BlendState_Release(blend_state);
    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

#if 0
RasterizerState rast_state {};
DepthStencilState ds_state {};
BlendState blend_state {};
SamplerState sampler0 {};

technique10 tech0
{
    pass pass0
    {
    }
};
#endif
static DWORD fx_test_state_group_defaults[] =
{
    0x43425844, 0x920e6905, 0x58225fcd, 0x3b65b423,
    0x67e96b6c, 0x00000001, 0x000001f4, 0x00000001,
    0x00000024, 0x30315846, 0x000001c8, 0xfeff1001,
    0x00000001, 0x00000000, 0x00000004, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x000000ec,
    0x00000000, 0x00000000, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x6f6c4724,
    0x736c6162, 0x73615200, 0x69726574, 0x5372657a,
    0x65746174, 0x00000d00, 0x00000200, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000400,
    0x73617200, 0x74735f74, 0x00657461, 0x74706544,
    0x65745368, 0x6c69636e, 0x74617453, 0x00440065,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00030000, 0x73640000, 0x6174735f,
    0x42006574, 0x646e656c, 0x74617453, 0x007b0065,
    0x00020000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00020000, 0x6c620000, 0x5f646e65,
    0x74617473, 0x61530065, 0x656c706d, 0x61745372,
    0xae006574, 0x02000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x15000000, 0x73000000,
    0x6c706d61, 0x00307265, 0x68636574, 0x61700030,
    0x00307373, 0x00000004, 0x00000000, 0x00000000,
    0x00000000, 0xffffffff, 0x00000000, 0x00000039,
    0x0000001d, 0x00000000, 0xffffffff, 0x00000000,
    0x00000000, 0x00000072, 0x00000056, 0x00000000,
    0xffffffff, 0x00000000, 0x00000000, 0x000000a2,
    0x00000086, 0x00000000, 0xffffffff, 0x00000000,
    0x00000000, 0x000000d7, 0x000000bb, 0x00000000,
    0xffffffff, 0x00000000, 0x00000000, 0x000000e0,
    0x00000001, 0x00000000, 0x000000e6, 0x00000000,
    0x00000000,
};

static void test_effect_state_group_defaults(void)
{
    ID3D10EffectDepthStencilVariable *d;
    ID3D10EffectRasterizerVariable *r;
    ID3D10EffectTechnique *technique;
    D3D10_DEPTH_STENCIL_DESC ds_desc;
    D3D10_RASTERIZER_DESC rast_desc;
    D3D10_SAMPLER_DESC sampler_desc;
    ID3D10EffectSamplerVariable *s;
    ID3D10EffectBlendVariable *b;
    D3D10_BLEND_DESC blend_desc;
    D3D10_PASS_DESC pass_desc;
    ID3D10EffectVariable *v;
    ID3D10EffectPass *pass;
    ID3D10Effect *effect;
    ID3D10Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_state_group_defaults, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "Failed to create effect, hr %#x.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "sampler0");
    s = v->lpVtbl->AsSampler(v);
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    s->lpVtbl->GetBackingStore(s, 0, &sampler_desc);
    ok(sampler_desc.Filter == D3D10_FILTER_MIN_MAG_MIP_POINT, "Got unexpected Filter %#x.\n", sampler_desc.Filter);
    ok(sampler_desc.AddressU == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressU %#x.\n", sampler_desc.AddressU);
    ok(sampler_desc.AddressV == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressV %#x.\n", sampler_desc.AddressV);
    ok(sampler_desc.AddressW == D3D10_TEXTURE_ADDRESS_WRAP, "Got unexpected AddressW %#x.\n", sampler_desc.AddressW);
    ok(sampler_desc.MipLODBias == 0.0f, "Got unexpected MipLODBias %.8e.\n", sampler_desc.MipLODBias);
    ok(sampler_desc.MaxAnisotropy == 16, "Got unexpected MaxAnisotropy %#x.\n", sampler_desc.MaxAnisotropy);
    ok(sampler_desc.ComparisonFunc == D3D10_COMPARISON_NEVER, "Got unexpected ComparisonFunc %#x.\n",
            sampler_desc.ComparisonFunc);
    ok(sampler_desc.BorderColor[0] == 0.0f, "Got unexpected BorderColor[0] %.8e.\n", sampler_desc.BorderColor[0]);
    ok(sampler_desc.BorderColor[1] == 0.0f, "Got unexpected BorderColor[1] %.8e.\n", sampler_desc.BorderColor[1]);
    ok(sampler_desc.BorderColor[2] == 0.0f, "Got unexpected BorderColor[2] %.8e.\n", sampler_desc.BorderColor[2]);
    ok(sampler_desc.BorderColor[3] == 0.0f, "Got unexpected BorderColor[3] %.8e.\n", sampler_desc.BorderColor[3]);
    ok(sampler_desc.MinLOD == 0.0f, "Got unexpected MinLOD %.8e.\n", sampler_desc.MinLOD);
    ok(sampler_desc.MaxLOD == FLT_MAX, "Got unexpected MaxLOD %.8e.\n", sampler_desc.MaxLOD);

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    b = v->lpVtbl->AsBlend(v);
    memset(&blend_desc, 0, sizeof(blend_desc));
    b->lpVtbl->GetBackingStore(b, 0, &blend_desc);
    ok(!blend_desc.AlphaToCoverageEnable, "Got unexpected AlphaToCoverageEnable %#x.\n",
            blend_desc.AlphaToCoverageEnable);
    ok(!blend_desc.BlendEnable[0], "Got unexpected BlendEnable[0] %#x.\n", blend_desc.BlendEnable[0]);
    ok(!blend_desc.BlendEnable[7], "Got unexpected BlendEnable[7] %#x.\n", blend_desc.BlendEnable[7]);
    ok(blend_desc.SrcBlend == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlend %#x.\n", blend_desc.SrcBlend);
    ok(blend_desc.DestBlend == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlend %#x.\n", blend_desc.DestBlend);
    ok(blend_desc.BlendOp == D3D10_BLEND_OP_ADD, "Got unexpected BlendOp %#x.\n", blend_desc.BlendOp);
    ok(blend_desc.SrcBlendAlpha == D3D10_BLEND_SRC_ALPHA, "Got unexpected SrcBlendAlpha %#x.\n",
            blend_desc.SrcBlendAlpha);
    ok(blend_desc.DestBlendAlpha == D3D10_BLEND_INV_SRC_ALPHA, "Got unexpected DestBlendAlpha %#x.\n",
            blend_desc.DestBlendAlpha);
    ok(blend_desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Got unexpected BlendOpAlpha %#x.\n", blend_desc.BlendOpAlpha);
    ok(blend_desc.RenderTargetWriteMask[0] == 0xf, "Got unexpected RenderTargetWriteMask[0] %#x.\n",
            blend_desc.RenderTargetWriteMask[0]);
    ok(blend_desc.RenderTargetWriteMask[7] == 0xf, "Got unexpected RenderTargetWriteMask[7] %#x.\n",
            blend_desc.RenderTargetWriteMask[7]);

    v = effect->lpVtbl->GetVariableByName(effect, "ds_state");
    d = v->lpVtbl->AsDepthStencil(v);
    d->lpVtbl->GetBackingStore(d, 0, &ds_desc);
    ok(ds_desc.DepthEnable, "Got unexpected DepthEnable %#x.\n", ds_desc.DepthEnable);
    ok(ds_desc.DepthWriteMask == D3D10_DEPTH_WRITE_MASK_ALL, "Got unexpected DepthWriteMask %#x.\n",
            ds_desc.DepthWriteMask);
    ok(ds_desc.DepthFunc == D3D10_COMPARISON_LESS, "Got unexpected DepthFunc %#x.\n", ds_desc.DepthFunc);
    ok(!ds_desc.StencilEnable, "Got unexpected StencilEnable %#x.\n", ds_desc.StencilEnable);
    ok(ds_desc.StencilReadMask == 0xff, "Got unexpected StencilReadMask %#x.\n", ds_desc.StencilReadMask);
    ok(ds_desc.StencilWriteMask == 0xff, "Got unexpected StencilWriteMask %#x.\n", ds_desc.StencilWriteMask);
    ok(ds_desc.FrontFace.StencilFailOp == D3D10_STENCIL_OP_KEEP, "Got unexpected FrontFaceStencilFail %#x.\n",
            ds_desc.FrontFace.StencilFailOp);
    ok(ds_desc.FrontFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected FrontFaceStencilDepthFail %#x.\n", ds_desc.FrontFace.StencilDepthFailOp);
    ok(ds_desc.FrontFace.StencilPassOp == D3D10_STENCIL_OP_KEEP, "Got unexpected FrontFaceStencilPass %#x.\n",
            ds_desc.FrontFace.StencilPassOp);
    ok(ds_desc.FrontFace.StencilFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected FrontFaceStencilFunc %#x.\n",
            ds_desc.FrontFace.StencilFunc);
    ok(ds_desc.BackFace.StencilFailOp == D3D10_STENCIL_OP_KEEP, "Got unexpected BackFaceStencilFail %#x.\n",
            ds_desc.BackFace.StencilFailOp);
    ok(ds_desc.BackFace.StencilDepthFailOp == D3D10_STENCIL_OP_KEEP,
            "Got unexpected BackFaceStencilDepthFail %#x.\n", ds_desc.BackFace.StencilDepthFailOp);
    ok(ds_desc.BackFace.StencilPassOp == D3D10_STENCIL_OP_KEEP, "Got unexpected BackFaceStencilPass %#x.\n",
            ds_desc.BackFace.StencilPassOp);
    ok(ds_desc.BackFace.StencilFunc == D3D10_COMPARISON_ALWAYS, "Got unexpected BackFaceStencilFunc %#x.\n",
            ds_desc.BackFace.StencilFunc);

    v = effect->lpVtbl->GetVariableByName(effect, "rast_state");
    r = v->lpVtbl->AsRasterizer(v);
    r->lpVtbl->GetBackingStore(r, 0, &rast_desc);
    ok(rast_desc.FillMode == D3D10_FILL_SOLID, "Got unexpected FillMode %#x.\n", rast_desc.FillMode);
    ok(rast_desc.CullMode == D3D10_CULL_BACK, "Got unexpected CullMode %#x.\n", rast_desc.CullMode);
    ok(!rast_desc.FrontCounterClockwise, "Got unexpected FrontCounterClockwise %#x.\n",
            rast_desc.FrontCounterClockwise);
    ok(rast_desc.DepthBias == 0, "Got unexpected DepthBias %#x.\n", rast_desc.DepthBias);
    ok(rast_desc.DepthBiasClamp == 0.0f, "Got unexpected DepthBiasClamp %.8e.\n", rast_desc.DepthBiasClamp);
    ok(rast_desc.SlopeScaledDepthBias == 0.0f, "Got unexpected SlopeScaledDepthBias %.8e.\n",
            rast_desc.SlopeScaledDepthBias);
    ok(rast_desc.DepthClipEnable, "Got unexpected DepthClipEnable %#x.\n", rast_desc.DepthClipEnable);
    ok(!rast_desc.ScissorEnable, "Got unexpected ScissorEnable %#x.\n", rast_desc.ScissorEnable);
    ok(!rast_desc.MultisampleEnable, "Got unexpected MultisampleEnable %#x.\n", rast_desc.MultisampleEnable);
    ok(!rast_desc.AntialiasedLineEnable, "Got unexpected AntialiasedLineEnable %#x.\n",
            rast_desc.AntialiasedLineEnable);

    technique = effect->lpVtbl->GetTechniqueByName(effect, "tech0");
    ok(!!technique, "Failed to get technique.\n");
    pass = technique->lpVtbl->GetPassByName(technique, "pass0");
    ok(!!pass, "Failed to get pass.\n");
    hr = pass->lpVtbl->GetDesc(pass, &pass_desc);
    ok(SUCCEEDED(hr), "Failed to get pass desc, hr %#x.\n", hr);
    ok(!strcmp(pass_desc.Name, "pass0"), "Got unexpected Name \"%s\".\n", pass_desc.Name);
    ok(!pass_desc.Annotations, "Got unexpected Annotations %#x.\n", pass_desc.Annotations);
    ok(!pass_desc.pIAInputSignature, "Got unexpected pIAInputSignature %p.\n", pass_desc.pIAInputSignature);
    ok(pass_desc.StencilRef == 0, "Got unexpected StencilRef %#x.\n", pass_desc.StencilRef);
    ok(pass_desc.SampleMask == 0, "Got unexpected SampleMask %#x.\n", pass_desc.SampleMask);
    ok(pass_desc.BlendFactor[0] == 0.0f, "Got unexpected BlendFactor[0] %.8e.\n", pass_desc.BlendFactor[0]);
    ok(pass_desc.BlendFactor[1] == 0.0f, "Got unexpected BlendFactor[1] %.8e.\n", pass_desc.BlendFactor[1]);
    ok(pass_desc.BlendFactor[2] == 0.0f, "Got unexpected BlendFactor[2] %.8e.\n", pass_desc.BlendFactor[2]);
    ok(pass_desc.BlendFactor[3] == 0.0f, "Got unexpected BlendFactor[3] %.8e.\n", pass_desc.BlendFactor[3]);

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_scalar_variable
 */
#if 0
cbuffer cb
{
    float f0, f_a[2];
    int i0, i_a[2];
    bool b0, b_a[2];
};
#endif
static DWORD fx_test_scalar_variable[] =
{
    0x43425844, 0xe4da4aa6, 0x1380ddc5, 0x445edad5,
    0x08581666, 0x00000001, 0x0000020b, 0x00000001,
    0x00000024, 0x30315846, 0x000001df, 0xfeff1001,
    0x00000001, 0x00000006, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000d3,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00000700, 0x00000100, 0x00000000,
    0x00000400, 0x00001000, 0x00000400, 0x00090900,
    0x00306600, 0x00000007, 0x00000001, 0x00000002,
    0x00000014, 0x00000010, 0x00000008, 0x00000909,
    0x00615f66, 0x00746e69, 0x0000004c, 0x00000001,
    0x00000000, 0x00000004, 0x00000010, 0x00000004,
    0x00000911, 0x4c003069, 0x01000000, 0x02000000,
    0x14000000, 0x10000000, 0x08000000, 0x11000000,
    0x69000009, 0x6200615f, 0x006c6f6f, 0x0000008f,
    0x00000001, 0x00000000, 0x00000004, 0x00000010,
    0x00000004, 0x00000921, 0x8f003062, 0x01000000,
    0x02000000, 0x14000000, 0x10000000, 0x08000000,
    0x21000000, 0x62000009, 0x0400615f, 0x70000000,
    0x00000000, 0x06000000, 0xff000000, 0x00ffffff,
    0x29000000, 0x0d000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x48000000,
    0x2c000000, 0x00000000, 0x10000000, 0x00000000,
    0x00000000, 0x00000000, 0x6c000000, 0x50000000,
    0x00000000, 0x24000000, 0x00000000, 0x00000000,
    0x00000000, 0x8b000000, 0x6f000000, 0x00000000,
    0x30000000, 0x00000000, 0x00000000, 0x00000000,
    0xb0000000, 0x94000000, 0x00000000, 0x44000000,
    0x00000000, 0x00000000, 0x00000000, 0xcf000000,
    0xb3000000, 0x00000000, 0x50000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};

static void test_scalar_methods(ID3D10EffectScalarVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name)
{
    float ret_f, expected_f;
    int ret_i, expected_i;
    BOOL ret_b, expected_b;
    HRESULT hr;

    hr = var->lpVtbl->SetFloat(var, 5.0f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_f = type == D3D10_SVT_BOOL ? -1.0f : 5.0f;
    ok(ret_f == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 5;
    ok(ret_i == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_b == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetInt(var, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_f = type == D3D10_SVT_BOOL ? -1.0f : 2.0f;
    ok(ret_f == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 2;
    ok(ret_i == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_b == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetBool(var, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_f == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_i == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
    ok(ret_b == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b);

    hr = var->lpVtbl->SetBool(var, 32);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloat(var, &ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_f == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f);

    hr = var->lpVtbl->GetInt(var, &ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    ok(ret_i == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i);

    hr = var->lpVtbl->GetBool(var, &ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_b = type == D3D10_SVT_BOOL ? 32 : -1;
    ok(ret_b == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b);
}

static void test_scalar_array_methods(ID3D10EffectScalarVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name)
{
    float set_f[2], ret_f[2], expected_f;
    int set_i[6], ret_i[6], expected_i, expected_i_a[2];
    BOOL set_b[2], ret_b[2], expected_b, expected_b_a[6];
    unsigned int i;
    HRESULT hr;

    set_f[0] = 10.0f; set_f[1] = 20.0f;
    hr = var->lpVtbl->SetFloatArray(var, set_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_i[0] = 5; set_i[1] = 6;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 1;
    hr = var->lpVtbl->SetBoolArray(var, set_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 10; set_b[1] = 20;
    hr = var->lpVtbl->SetBoolArray(var, set_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatArray(var, ret_f, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolArray(var, ret_b, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < 2; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    /* Array offset tests. Offset argument goes unused for scalar arrays. */
    set_i[0] = 0; set_i[1] = 0;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    /* After this, if offset is in use, return should be { 0, 5 }. */
    set_i[0] = 5;
    hr = var->lpVtbl->SetIntArray(var, set_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_b_a[0] = -1; expected_b_a[1] = 0;
    expected_i_a[0] =  5; expected_i_a[1] = 0;
    for (i = 0; i < 2; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? expected_b_a[i] : expected_i_a[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    /* Test the offset on GetArray methods. If offset was in use, we'd get
     * back 5 given that the variable was previously set to { 0, 5 }. */
    hr = var->lpVtbl->GetIntArray(var, ret_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 5;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    /* Try setting offset larger than number of elements. */
    set_i[0] = 0; set_i[1] = 0;
    hr = var->lpVtbl->SetIntArray(var, set_i, 0, 2);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    set_i[0] = 1;
    hr = var->lpVtbl->SetIntArray(var, set_i, 6, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    /* Since offset goes unused, a larger offset than the number of elements
     * in the array should have no effect. */
    hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 1;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    memset(ret_i, 0, sizeof(ret_i));
    hr = var->lpVtbl->GetIntArray(var, ret_i, 6, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i = type == D3D10_SVT_BOOL ? -1 : 1;
    ok(ret_i[0] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[0]);

    if (0)
    {
        /* Windows array setting function has no bounds checking, so this test
         * ends up writing over into the adjacent variables in the local buffer. */
        set_i[0] = 1; set_i[1] = 2; set_i[2] = 3; set_i[3] = 4; set_i[4] = 5; set_i[5] = 6;
        hr = var->lpVtbl->SetIntArray(var, set_i, 0, 6);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

        memset(ret_i, 0, sizeof(ret_i));
        hr = var->lpVtbl->GetIntArray(var, ret_i, 0, 6);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

        expected_i_a[0] = 1; expected_i_a[1] = 2; expected_i_a[2] = 0; expected_i_a[3] = 0;
        expected_i_a[4] = 0; expected_i_a[5] = 0;
        expected_b_a[0] = -1; expected_b_a[1] = -1; expected_b_a[2] = 0; expected_b_a[3] = 0;
        expected_b_a[4] = 0; expected_b_a[5] = 0;
        for (i = 0; i < 6; ++i)
        {
            expected_i = type == D3D10_SVT_BOOL ? expected_b_a[i] : expected_i_a[i];
            ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
        }
    }
}

static void test_effect_scalar_variable(void)
{
    static const struct
    {
        const char *name;
        D3D_SHADER_VARIABLE_TYPE type;
        BOOL array;
    }
    tests[] =
    {
        {"f0", D3D10_SVT_FLOAT},
        {"i0", D3D10_SVT_INT},
        {"b0", D3D10_SVT_BOOL},
        {"f_a", D3D10_SVT_FLOAT, TRUE},
        {"i_a", D3D10_SVT_INT, TRUE},
        {"b_a", D3D10_SVT_BOOL, TRUE},
    };
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10EffectScalarVariable *s_v;
    ID3D10EffectVariable *var;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_scalar_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Check each different scalar type, make sure the variable returned is
     * valid, set it to a value, and make sure what we get back is the same
     * as what we set it to. */
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", tests[i].name, hr);
        ok(type_desc.Type == tests[i].type, "Variable %s, got unexpected type %#x.\n",
                tests[i].name, type_desc.Type);
        s_v = var->lpVtbl->AsScalar(var);
        test_scalar_methods(s_v, tests[i].type, tests[i].name);
        if (tests[i].array)
            test_scalar_array_methods(s_v, tests[i].type, tests[i].name);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_vector_variable
 */
#if 0
cbuffer cb
{
    float4 v_f0, v_f_a[2];
    int3 v_i0, v_i_a[3];
    bool2 v_b0, v_b_a[4];
};
#endif
static DWORD fx_test_vector_variable[] =
{
    0x43425844, 0x581ae0ae, 0xa906b020, 0x26bba03e,
    0x5d7dfba2, 0x00000001, 0x0000021a, 0x00000001,
    0x00000024, 0x30315846, 0x000001ee, 0xfeff1001,
    0x00000001, 0x00000006, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000e2,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00070034, 0x00010000, 0x00000000,
    0x00100000, 0x00100000, 0x00100000, 0x210a0000,
    0x5f760000, 0x07003066, 0x01000000, 0x02000000,
    0x20000000, 0x10000000, 0x20000000, 0x0a000000,
    0x76000021, 0x615f665f, 0x746e6900, 0x00510033,
    0x00010000, 0x00000000, 0x000c0000, 0x00100000,
    0x000c0000, 0x19120000, 0x5f760000, 0x51003069,
    0x01000000, 0x03000000, 0x2c000000, 0x10000000,
    0x24000000, 0x12000000, 0x76000019, 0x615f695f,
    0x6f6f6200, 0x9900326c, 0x01000000, 0x00000000,
    0x08000000, 0x10000000, 0x08000000, 0x22000000,
    0x76000011, 0x0030625f, 0x00000099, 0x00000001,
    0x00000004, 0x00000038, 0x00000010, 0x00000020,
    0x00001122, 0x5f625f76, 0x00040061, 0x00c00000,
    0x00000000, 0x00060000, 0xffff0000, 0x0000ffff,
    0x002a0000, 0x000e0000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x004b0000,
    0x002f0000, 0x00000000, 0x00100000, 0x00000000,
    0x00000000, 0x00000000, 0x00720000, 0x00560000,
    0x00000000, 0x00300000, 0x00000000, 0x00000000,
    0x00000000, 0x00930000, 0x00770000, 0x00000000,
    0x00400000, 0x00000000, 0x00000000, 0x00000000,
    0x00bb0000, 0x009f0000, 0x00000000, 0x00700000,
    0x00000000, 0x00000000, 0x00000000, 0x00dc0000,
    0x00c00000, 0x00000000, 0x00800000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,
};

static void test_vector_methods(ID3D10EffectVectorVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int components)
{
    float set_f[4], ret_f[4], expected_f, expected_f_v[4];
    int set_i[4], ret_i[4], expected_i, expected_i_v[4];
    BOOL set_b[4], ret_b[4], expected_b;
    unsigned int i;
    HRESULT hr;

    set_f[0] = 1.0f; set_f[1] = 2.0f; set_f[2] = 3.0f; set_f[3] = 4.0f;
    hr = var->lpVtbl->SetFloatVector(var, set_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_i[0] = 5; set_i[1] = 6; set_i[2] = 7; set_i[3] = 8;
    hr = var->lpVtbl->SetIntVector(var, set_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 0; set_b[2] = 1; set_b[3] = 0;
    hr = var->lpVtbl->SetBoolVector(var, set_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_f_v[0] = -1.0f; expected_f_v[1] = 0.0f; expected_f_v[2] = -1.0f; expected_f_v[3] = 0.0f;
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == expected_f_v[i], "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i_v[0] = -1; expected_i_v[1] = 0; expected_i_v[2] = -1; expected_i_v[3] = 0;
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == expected_i_v[i], "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : expected_i_v[i];
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 5; set_b[1] = 10; set_b[2] = 15; set_b[3] = 20;
    hr = var->lpVtbl->SetBoolVector(var, set_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVector(var, ret_f);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVector(var, ret_i);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVector(var, ret_b);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }
}

static void test_vector_array_methods(ID3D10EffectVectorVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int components, unsigned int elements)
{
    float set_f[9], ret_f[9], expected_f, expected_f_a[9];
    int set_i[9], ret_i[9], expected_i, expected_i_a[9];
    BOOL set_b[9], ret_b[9], expected_b;
    unsigned int i;
    HRESULT hr;

    set_f[0] = 1.0f; set_f[1] = 2.0f; set_f[2] = 3.0f; set_f[3] = 4.0f;
    set_f[4] = 5.0f; set_f[5] = 6.0f; set_f[6] = 7.0f; set_f[7] = 8.0f;
    set_f[8] = 9.0f;
    hr = var->lpVtbl->SetFloatVectorArray(var, set_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : set_f[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : (int)set_f[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_i[0] = 10; set_i[1] = 11; set_i[2] = 12; set_i[3] = 13;
    set_i[4] = 14; set_i[5] = 15; set_i[6] = 16; set_i[7] = 17;
    set_i[8] = 18;
    hr = var->lpVtbl->SetIntVectorArray(var, set_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_f = type == D3D10_SVT_BOOL ? -1.0f : (float)set_i[i];
        ok(ret_f[i] == expected_f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);
    }

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_i = type == D3D10_SVT_BOOL ? -1 : set_i[i];
        ok(ret_i[i] == expected_i, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);
    }

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_b[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    set_b[0] = 1; set_b[1] = 0; set_b[2] = 1; set_b[3] = 1;
    set_b[4] = 1; set_b[5] = 0; set_b[6] = 0; set_b[7] = 1;
    set_b[8] = 1;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_f_a[0] = -1.0f; expected_f_a[1] = 0.0f; expected_f_a[2] = -1.0f; expected_f_a[3] = -1.0f;
    expected_f_a[4] = -1.0f; expected_f_a[5] = 0.0f; expected_f_a[6] =  0.0f; expected_f_a[7] = -1.0f;
    expected_f_a[8] = -1.0f;
    for (i = 0; i < components * elements; ++i)
        ok(ret_f[i] == expected_f_a[i], "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    expected_i_a[0] = -1; expected_i_a[1] = 0; expected_i_a[2] = -1; expected_i_a[3] = -1;
    expected_i_a[4] = -1; expected_i_a[5] = 0; expected_i_a[6] =  0; expected_i_a[7] = -1;
    expected_i_a[8] = -1;
    for (i = 0; i < components * elements; ++i)
        ok(ret_i[i] == expected_i_a[i], "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : expected_i_a[i];
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    set_b[0] = 5;  set_b[1] = 10; set_b[2] = 15; set_b[3] = 20;
    set_b[4] = 25; set_b[5] = 30; set_b[6] = 35; set_b[7] = 40;
    set_b[8] = 45;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components * elements; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? set_b[i] : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    /* According to MSDN, the offset argument goes unused for VectorArray
     * methods, same as the ScalarArray methods. This test shows that's not
     * the case. */
    set_b[0] = 0; set_b[1] = 0; set_b[2] = 0; set_b[3] = 0;
    set_b[4] = 0; set_b[5] = 0; set_b[6] = 0; set_b[7] = 0;
    set_b[8] = 0;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    set_b[0] = 1; set_b[1] = 1; set_b[2] = 1; set_b[3] = 1;
    hr = var->lpVtbl->SetBoolVectorArray(var, set_b, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    /* If the previous offset of 1 worked, then the first vector value of the
     * array should still be false. */
    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == 0, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == 0, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 0, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(!ret_b[i], "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);

    /* Test the GetFloatVectorArray offset argument. If it works, we should
     * get a vector with all values set to true. */
    hr = var->lpVtbl->GetFloatVectorArray(var, ret_f, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_f[i] == -1.0f, "Variable %s, got unexpected value %.8e.\n", name, ret_f[i]);

    hr = var->lpVtbl->GetIntVectorArray(var, ret_i, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
        ok(ret_i[i] == -1, "Variable %s, got unexpected value %#x.\n", name, ret_i[i]);

    hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < components; ++i)
    {
        expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
        ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
    }

    if (0)
    {
        /* Windows array setting function has no bounds checking on offset values
         * either, so this ends up writing into adjacent variables. */
        hr = var->lpVtbl->SetBoolVectorArray(var, set_b, elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

        hr = var->lpVtbl->GetBoolVectorArray(var, ret_b, elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
        for (i = 0; i < components; ++i)
        {
            expected_b = type == D3D10_SVT_BOOL ? 1 : -1;
            ok(ret_b[i] == expected_b, "Variable %s, got unexpected value %#x.\n", name, ret_b[i]);
        }
    }
}

static void test_effect_vector_variable(void)
{
    static const struct
    {
        const char *name;
        D3D_SHADER_VARIABLE_TYPE type;
        unsigned int components;
        unsigned int elements;
    }
    tests[] =
    {
        {"v_f0", D3D10_SVT_FLOAT, 4, 1},
        {"v_i0", D3D10_SVT_INT, 3, 1},
        {"v_b0", D3D10_SVT_BOOL, 2, 1},
        {"v_f_a", D3D10_SVT_FLOAT, 4, 2},
        {"v_i_a", D3D10_SVT_INT, 3, 3},
        {"v_b_a", D3D10_SVT_BOOL, 2, 4},
    };
    ID3D10EffectVectorVariable *v_var;
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10EffectVariable *var;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_vector_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", tests[i].name, hr);
        ok(type_desc.Type == tests[i].type, "Variable %s, got unexpected type %#x.\n",
                tests[i].name, type_desc.Type);
        v_var = var->lpVtbl->AsVector(var);
        test_vector_methods(v_var, tests[i].type, tests[i].name, tests[i].components);
        if (tests[i].elements > 1)
            test_vector_array_methods(v_var, tests[i].type, tests[i].name, tests[i].components, tests[i].elements);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_matrix_variable
 */
#if 0
cbuffer cb
{
    float4x4 m_f0;
    float4x4 m_f_a[2];

    row_major int2x3 m_i0;

    bool3x2 m_b0;
    bool3x2 m_b_a[2];
};
#endif

static DWORD fx_test_matrix_variable[] =
{
    0x43425844, 0xc95a5c42, 0xa138d3cb, 0x8a4ef493,
    0x3515b7ee, 0x00000001, 0x000001e2, 0x00000001,
    0x00000024, 0x30315846, 0x000001b6, 0xfeff1001,
    0x00000001, 0x00000005, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x000000c6,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x66006263,
    0x74616f6c, 0x00347834, 0x00000007, 0x00000001,
    0x00000000, 0x00000040, 0x00000040, 0x00000040,
    0x0000640b, 0x30665f6d, 0x00000700, 0x00000100,
    0x00000200, 0x00008000, 0x00004000, 0x00008000,
    0x00640b00, 0x665f6d00, 0x6900615f, 0x7832746e,
    0x00530033, 0x00010000, 0x00000000, 0x001c0000,
    0x00200000, 0x00180000, 0x1a130000, 0x5f6d0000,
    0x62003069, 0x336c6f6f, 0x7b003278, 0x01000000,
    0x00000000, 0x1c000000, 0x20000000, 0x18000000,
    0x23000000, 0x6d000053, 0x0030625f, 0x0000007b,
    0x00000001, 0x00000002, 0x0000003c, 0x00000020,
    0x00000030, 0x00005323, 0x5f625f6d, 0x00040061,
    0x01400000, 0x00000000, 0x00050000, 0xffff0000,
    0x0000ffff, 0x002c0000, 0x00100000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x004d0000, 0x00310000, 0x00000000, 0x00400000,
    0x00000000, 0x00000000, 0x00000000, 0x00760000,
    0x005a0000, 0x00000000, 0x00c00000, 0x00000000,
    0x00000000, 0x00000000, 0x009f0000, 0x00830000,
    0x00000000, 0x00e00000, 0x00000000, 0x00000000,
    0x00000000, 0x00c00000, 0x00a40000, 0x00000000,
    0x01000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
};

struct d3d10_matrix
{
    float m[4][4];
};

static void set_test_matrix(struct d3d10_matrix *m, D3D10_SHADER_VARIABLE_TYPE type,
        unsigned int row_count, unsigned int col_count, unsigned int elements)
{
    unsigned int row, col, elem;
    float tmp_f;
    int tmp_i;
    BOOL tmp_b;

    memset(m, 0, elements * sizeof(*m));
    switch (type)
    {
        case D3D10_SVT_FLOAT:
            tmp_f = 1.0f;
            for (elem = 0; elem < elements; ++elem)
            {
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = tmp_f;
                        ++tmp_f;
                    }
                }
            }
            break;

        case D3D10_SVT_INT:
            tmp_i = 1;
            for (elem = 0; elem < elements; ++elem)
            {
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = *(float *)&tmp_i;
                        ++tmp_i;
                    }
                }
            }
            break;

        case D3D10_SVT_BOOL:
            tmp_b = FALSE;
            for (elem = 0; elem < elements; ++elem)
            {
                tmp_b = !tmp_b;
                for (row = 0; row < row_count; ++row)
                {
                    for (col = 0; col < col_count; ++col)
                    {
                        m[elem].m[row][col] = *(float *)&tmp_b;
                        tmp_b = !tmp_b;
                    }
                }
            }
            break;

        default:
            break;
    }
}

static void transpose_matrix(struct d3d10_matrix *src, struct d3d10_matrix *dst,
        unsigned int row_count, unsigned int col_count)
{
    unsigned int row, col;

    for (row = 0; row < col_count; ++row)
    {
        for (col = 0; col < row_count; ++col)
            dst->m[row][col] = src->m[col][row];
    }
}

static void compare_matrix(const char *name, unsigned int line, struct d3d10_matrix *a,
        struct d3d10_matrix *b, unsigned int row_count, unsigned int col_count, BOOL transpose)
{
    unsigned int row, col;
    float tmp;

    for (row = 0; row < row_count; ++row)
    {
        for (col = 0; col < col_count; ++col)
        {
            tmp = !transpose ? b->m[row][col] : b->m[col][row];
            ok_(__FILE__, line)(a->m[row][col] == tmp,
                    "Variable %s (%u, %u), got unexpected value 0x%08x.\n", name, row, col,
                    *(unsigned int *)&tmp);
        }
    }
}

static void test_matrix_methods(ID3D10EffectMatrixVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int row_count, unsigned int col_count)
{
    struct d3d10_matrix m_set, m_ret, m_expected;
    HRESULT hr;

    set_test_matrix(&m_set, type, row_count, col_count, 1);

    hr = var->lpVtbl->SetMatrix(var, &m_set.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrix(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    compare_matrix(name, __LINE__, &m_set, &m_ret, row_count, col_count, FALSE);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixTranspose(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    compare_matrix(name, __LINE__, &m_set, &m_ret, row_count, col_count, TRUE);

    hr = var->lpVtbl->SetMatrixTranspose(var, &m_set.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrix(var, &m_ret.m[0][0]);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    compare_matrix(name, __LINE__, &m_ret, &m_set, row_count, col_count, TRUE);

    memset(&m_ret.m[0][0], 0, sizeof(m_ret));
    memset(&m_expected.m[0][0], 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTranspose(var, &m_ret.m[0][0]);
    transpose_matrix(&m_set, &m_expected, row_count, col_count);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    compare_matrix(name, __LINE__, &m_expected, &m_ret, row_count, col_count, TRUE);
}

static void test_matrix_array_methods(ID3D10EffectMatrixVariable *var, D3D10_SHADER_VARIABLE_TYPE type,
        const char *name, unsigned int row_count, unsigned int col_count, unsigned int elements)
{
    struct d3d10_matrix m_set[2], m_ret[2], m_expected;
    unsigned int i;
    HRESULT hr;

    set_test_matrix(&m_set[0], type, row_count, col_count, elements);

    hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_set[i], &m_ret[i], row_count, col_count, FALSE);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_set[i], &m_ret[i], row_count, col_count, TRUE);

    hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < elements; ++i)
        compare_matrix(name, __LINE__, &m_ret[i], &m_set[i], row_count, col_count, TRUE);

    memset(m_ret, 0, sizeof(m_ret));
    memset(&m_expected, 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 0, elements);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    for (i = 0; i < elements; ++i)
    {
        memset(&m_expected, 0, sizeof(m_expected));
        transpose_matrix(&m_set[i], &m_expected, row_count, col_count);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[i], row_count, col_count, TRUE);
    }

    /* Offset tests. */
    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->SetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);

    hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    compare_matrix(name, __LINE__, &m_ret[0], &m_set[0], row_count, col_count, FALSE);

    memset(m_ret, 0, sizeof(m_ret));
    hr = var->lpVtbl->SetMatrixArray(var, &m_ret[0].m[0][0], 0, elements);

    hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

    memset(&m_expected, 0, sizeof(m_expected));
    hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], 1, 1);
    ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
    transpose_matrix(&m_set[0], &m_expected, row_count, col_count);
    compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);

    if (0)
    {
        /* Like vector array functions, matrix array functions will allow for
         * writing out of bounds into adjacent memory. */
        hr = var->lpVtbl->SetMatrixArray(var, &m_set[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

        memset(m_ret, 0, sizeof(m_ret));
        hr = var->lpVtbl->GetMatrixArray(var, &m_ret[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);

        hr = var->lpVtbl->SetMatrixTransposeArray(var, &m_set[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);

        memset(&m_expected, 0, sizeof(m_expected));
        hr = var->lpVtbl->GetMatrixTransposeArray(var, &m_ret[0].m[0][0], elements + 1, 1);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", name, hr);
        transpose_matrix(&m_set[0], &m_expected, row_count, col_count);
        compare_matrix(name, __LINE__, &m_expected, &m_ret[0], row_count, col_count, TRUE);
    }
}

static void test_effect_matrix_variable(void)
{
    static const struct
    {
        const char *name;
        D3D_SHADER_VARIABLE_TYPE type;
        unsigned int rows;
        unsigned int columns;
        unsigned int elements;
    }
    tests[] =
    {
        {"m_f0", D3D10_SVT_FLOAT, 4, 4, 1},
        {"m_i0", D3D10_SVT_INT, 2, 3, 1},
        {"m_b0", D3D10_SVT_BOOL, 3, 2, 1},
        {"m_f_a", D3D10_SVT_FLOAT, 4, 4, 2},
        {"m_b_a", D3D10_SVT_BOOL, 3, 2, 2},
    };
    ID3D10EffectMatrixVariable *m_var;
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10EffectVariable *var;
    ID3D10EffectType *type;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_test_matrix_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        var = effect->lpVtbl->GetVariableByName(effect, tests[i].name);
        type = var->lpVtbl->GetType(var);
        hr = type->lpVtbl->GetDesc(type, &type_desc);
        ok(hr == S_OK, "Variable %s, got unexpected hr %#x.\n", tests[i].name, hr);
        ok(type_desc.Type == tests[i].type, "Variable %s, got unexpected type %#x.\n",
                tests[i].name, type_desc.Type);
        m_var = var->lpVtbl->AsMatrix(var);
        test_matrix_methods(m_var, tests[i].type, tests[i].name, tests[i].rows, tests[i].columns);
        if (tests[i].elements > 1)
            test_matrix_array_methods(m_var, tests[i].type, tests[i].name, tests[i].rows, tests[i].columns,
                    tests[i].elements);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

/*
 * test_effect_resource_variable
 */
#if 0
Texture2D t0;
Texture2D t_a[2];

float4 VS( float4 Pos : POSITION ) : SV_POSITION { return float4(1.0f, 1.0f, 1.0f, 1.0f); }

float4 PS( float4 Pos : SV_POSITION ) : SV_Target
{
    uint4 tmp;

    tmp = t0.Load(int3(0, 0, 0));
    tmp = t_a[0].Load(int4(0, 0, 0, 0));
    tmp = t_a[1].Load(int4(0, 0, 0, 0));
    return float4(1.0f, 1.0f, 0.0f, 1.0f) + tmp;
}

technique10 rsrc_test
{
    pass p0
    {
        SetVertexShader(CompileShader(vs_4_0, VS()));
        SetPixelShader(CompileShader(ps_4_0, PS()));
    }
}
#endif
static DWORD fx_test_resource_variable[] =
{
    0x43425844, 0x767a8421, 0xdcbfffe6, 0x83df123d, 0x189ce72a, 0x00000001, 0x0000065a, 0x00000001,
    0x00000024, 0x30315846, 0x0000062e, 0xfeff1001, 0x00000000, 0x00000000, 0x00000002, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0x00000582, 0x00000000, 0x00000003, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000002, 0x00000000, 0x74786554,
    0x32657275, 0x00040044, 0x00020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000c0000,
    0x30740000, 0x00000400, 0x00000200, 0x00000200, 0x00000000, 0x00000000, 0x00000000, 0x00000c00,
    0x615f7400, 0x72737200, 0x65745f63, 0x70007473, 0x01b40030, 0x58440000, 0x338d4342, 0xc5a69f46,
    0x56883ae5, 0xa98fccc2, 0x00018ead, 0x01b40000, 0x00050000, 0x00340000, 0x008c0000, 0x00c00000,
    0x00f40000, 0x01380000, 0x44520000, 0x00504645, 0x00000000, 0x00000000, 0x00000000, 0x001c0000,
    0x04000000, 0x0100fffe, 0x001c0000, 0x694d0000, 0x736f7263, 0x2074666f, 0x20295228, 0x4c534c48,
    0x61685320, 0x20726564, 0x706d6f43, 0x72656c69, 0x322e3920, 0x35392e39, 0x31332e32, 0xab003131,
    0x5349abab, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000,
    0x00000000, 0x000f0000, 0x4f500000, 0x49544953, 0xab004e4f, 0x534fabab, 0x002c4e47, 0x00010000,
    0x00080000, 0x00200000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x000f0000, 0x56530000,
    0x534f505f, 0x4f495449, 0x4853004e, 0x003c5244, 0x00400000, 0x000f0001, 0x00670000, 0x20f20400,
    0x00000010, 0x00010000, 0x00360000, 0x20f20800, 0x00000010, 0x40020000, 0x00000000, 0x00003f80,
    0x00003f80, 0x00003f80, 0x003e3f80, 0x54530100, 0x00745441, 0x00020000, 0x00000000, 0x00000000,
    0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00010000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00010000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x005a0000, 0x00000000, 0x035c0000, 0x58440000, 0xe3754342, 0x3e477f40,
    0xed6f143f, 0xf16d26ce, 0x00010c3a, 0x035c0000, 0x00050000, 0x00340000, 0x00d00000, 0x01040000,
    0x01380000, 0x02e00000, 0x44520000, 0x00944645, 0x00000000, 0x00000000, 0x00020000, 0x001c0000,
    0x04000000, 0x0100ffff, 0x00630000, 0x005c0000, 0x00020000, 0x00050000, 0x00040000, 0xffff0000,
    0x0000ffff, 0x00010000, 0x000c0000, 0x005f0000, 0x00020000, 0x00050000, 0x00040000, 0xffff0000,
    0x0001ffff, 0x00020000, 0x000c0000, 0x30740000, 0x615f7400, 0x63694d00, 0x6f736f72, 0x28207466,
    0x48202952, 0x204c534c, 0x64616853, 0x43207265, 0x69706d6f, 0x2072656c, 0x39322e39, 0x3235392e,
    0x3131332e, 0x53490031, 0x002c4e47, 0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00010000,
    0x00030000, 0x00000000, 0x000f0000, 0x56530000, 0x534f505f, 0x4f495449, 0x534f004e, 0x002c4e47,
    0x00010000, 0x00080000, 0x00200000, 0x00000000, 0x00000000, 0x00030000, 0x00000000, 0x000f0000,
    0x56530000, 0x7261545f, 0x00746567, 0x4853abab, 0x01a05244, 0x00400000, 0x00680000, 0x18580000,
    0x70000400, 0x00000010, 0x55550000, 0x18580000, 0x70000400, 0x00010010, 0x55550000, 0x18580000,
    0x70000400, 0x00020010, 0x55550000, 0x00650000, 0x20f20300, 0x00000010, 0x00680000, 0x00020200,
    0x002d0000, 0x00f20a00, 0x00000010, 0x40020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x7e460000, 0x00000010, 0x001c0000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x00560000,
    0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x002d0000, 0x00f20a00, 0x00010010, 0x40020000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7e460000, 0x00010010, 0x00000000, 0x00f20700,
    0x00000010, 0x0e460000, 0x00000010, 0x0e460000, 0x00010010, 0x001c0000, 0x00f20500, 0x00000010,
    0x0e460000, 0x00000010, 0x00560000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x002d0000,
    0x00f20a00, 0x00010010, 0x40020000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x7e460000,
    0x00020010, 0x00000000, 0x00f20700, 0x00000010, 0x0e460000, 0x00000010, 0x0e460000, 0x00010010,
    0x001c0000, 0x00f20500, 0x00000010, 0x0e460000, 0x00000010, 0x00560000, 0x00f20500, 0x00000010,
    0x0e460000, 0x00000010, 0x00000000, 0x20f20a00, 0x00000010, 0x0e460000, 0x00000010, 0x40020000,
    0x00000000, 0x00003f80, 0x00003f80, 0x00000000, 0x003e3f80, 0x54530100, 0x00745441, 0x000d0000,
    0x00020000, 0x00000000, 0x00010000, 0x00030000, 0x00000000, 0x00000000, 0x00010000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00060000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x021a0000, 0x00000000, 0x002a0000, 0x000e0000,
    0x00000000, 0xffff0000, 0x0000ffff, 0x00490000, 0x002d0000, 0x00000000, 0xffff0000, 0x0000ffff,
    0x004d0000, 0x00010000, 0x00000000, 0x00570000, 0x00020000, 0x00000000, 0x00060000, 0x00000000,
    0x00070000, 0x02120000, 0x00070000, 0x00000000, 0x00070000, 0x057a0000, 0x00000000,
};

static void create_effect_texture_resource(ID3D10Device *device, ID3D10ShaderResourceView **srv,
        ID3D10Texture2D **tex)
{
    D3D10_TEXTURE2D_DESC tex_desc;
    HRESULT hr;

    tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    tex_desc.Width  = 8;
    tex_desc.Height = 8;
    tex_desc.ArraySize = 1;
    tex_desc.MipLevels = 0;
    tex_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    tex_desc.Usage = D3D10_USAGE_DEFAULT;
    tex_desc.CPUAccessFlags = 0;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.MiscFlags = 0;

    hr = ID3D10Device_CreateTexture2D(device, &tex_desc, NULL, tex);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = ID3D10Device_CreateShaderResourceView(device, (ID3D10Resource *)*tex, NULL, srv);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

#define get_effect_shader_resource_variable(a) get_effect_shader_resource_variable_(__LINE__, a)
static ID3D10EffectShaderResourceVariable *get_effect_shader_resource_variable_(unsigned int line,
        ID3D10EffectVariable *var)
{
    D3D10_EFFECT_TYPE_DESC type_desc;
    ID3D10EffectType *type;
    HRESULT hr;

    type = var->lpVtbl->GetType(var);
    ok_(__FILE__, line)(!!type, "Got unexpected type %p.\n", type);
    hr = type->lpVtbl->GetDesc(type, &type_desc);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok_(__FILE__, line)(type_desc.Type == D3D10_SVT_TEXTURE2D, "Type is %x, expected %x.\n",
            type_desc.Type, D3D10_SVT_TEXTURE2D);
    return var->lpVtbl->AsShaderResource(var);
}

static void test_effect_resource_variable(void)
{
    ID3D10ShaderResourceView *srv0, *srv_a[2], *srv_tmp[2];
    ID3D10EffectShaderResourceVariable *t0, *t_a, *t_a_0;
    ID3D10EffectTechnique *technique;
    ID3D10Texture2D *tex0, *tex_a[2];
    ID3D10EffectVariable *var;
    ID3D10EffectPass *pass;
    ID3D10Device *device;
    ID3D10Effect *effect;
    unsigned int i;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = create_effect(fx_test_resource_variable, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    var = effect->lpVtbl->GetVariableByName(effect, "t0");
    t0 = get_effect_shader_resource_variable(var);
    create_effect_texture_resource(device, &srv0, &tex0);
    hr = t0->lpVtbl->SetResource(t0, srv0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    var = effect->lpVtbl->GetVariableByName(effect, "t_a");
    t_a = get_effect_shader_resource_variable(var);
    for (i = 0; i < 2; ++i)
        create_effect_texture_resource(device, &srv_a[i], &tex_a[i]);
    hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 0, 2);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Apply the pass to bind the resource to the shader. */
    technique = effect->lpVtbl->GetTechniqueByName(effect, "rsrc_test");
    ok(!!technique, "Got unexpected technique %p.\n", technique);
    pass = technique->lpVtbl->GetPassByName(technique, "p0");
    ok(!!pass, "Got unexpected pass %p.\n", pass);
    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 0, 1, &srv_tmp[0]);
    ok(srv_tmp[0] == srv0, "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ID3D10ShaderResourceView_Release(srv_tmp[0]);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    for (i = 0; i < 2; ++i)
    {
        ok(srv_tmp[i] == srv_a[i], "Got unexpected shader resource view %p.\n", srv_tmp[i]);
        ID3D10ShaderResourceView_Release(srv_tmp[i]);
    }

    /* Test individual array element variable SetResource. */
    var = t_a->lpVtbl->GetElement(t_a, 1);
    t_a_0 = get_effect_shader_resource_variable(var);
    hr = t_a_0->lpVtbl->SetResource(t_a_0, srv0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    ok(srv_tmp[0] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ok(srv_tmp[1] == srv0, "Got unexpected shader resource view %p.\n", srv_tmp[1]);
    for (i = 0; i < 2; ++i)
        ID3D10ShaderResourceView_Release(srv_tmp[i]);

    /* Test offset. */
    hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 1, 1);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = pass->lpVtbl->Apply(pass, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ID3D10Device_PSGetShaderResources(device, 1, 2, srv_tmp);
    ok(srv_tmp[0] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[0]);
    ok(srv_tmp[1] == srv_a[0], "Got unexpected shader resource view %p.\n", srv_tmp[1]);
    for (i = 0; i < 2; ++i)
        ID3D10ShaderResourceView_Release(srv_tmp[i]);

    if (0)
    {
        /* This crashes on Windows. */
        hr = t_a->lpVtbl->SetResourceArray(t_a, srv_a, 2, 2);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }

    ID3D10ShaderResourceView_Release(srv0);
    ID3D10Texture2D_Release(tex0);
    for (i = 0; i < 2; ++i)
    {
        ID3D10ShaderResourceView_Release(srv_a[i]);
        ID3D10Texture2D_Release(tex_a[i]);
    }

    effect->lpVtbl->Release(effect);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

START_TEST(effect)
{
    test_effect_constant_buffer_type();
    test_effect_variable_type();
    test_effect_variable_member();
    test_effect_variable_element();
    test_effect_variable_type_class();
    test_effect_constant_buffer_stride();
    test_effect_local_shader();
    test_effect_get_variable_by();
    test_effect_state_groups();
    test_effect_state_group_defaults();
    test_effect_scalar_variable();
    test_effect_vector_variable();
    test_effect_matrix_variable();
    test_effect_resource_variable();
}
