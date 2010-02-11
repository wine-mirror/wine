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

static ID3D10Device *create_device(void)
{
    ID3D10Device *device;

    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a HW device\n");
        return device;
    }

    trace("Failed to create a HW device, trying REF\n");
    if (SUCCEEDED(D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL, 0, D3D10_SDK_VERSION, &device)))
    {
        trace("Created a REF device\n");
        return device;
    }

    trace("Failed to create a device, returning NULL\n");
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

static void test_effect_constant_buffer_type(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type, *type2, *null_type;
    D3D10_EFFECT_TYPE_DESC type_desc;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

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

static void test_effect_variable_type(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type, *type2, *type3;
    D3D10_EFFECT_TYPE_DESC type_desc;
    HRESULT hr;
    LPCSTR string;
    unsigned int i;

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

static void test_effect_variable_member(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectVariable *variable, *variable2, *variable3, *null_variable;
    D3D10_EFFECT_VARIABLE_DESC desc;
    HRESULT hr;

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

static void test_effect_variable_element(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *parent;
    ID3D10EffectVariable *variable, *variable2, *variable3, *variable4, *variable5;
    ID3D10EffectType *type, *type2;
    D3D10_EFFECT_VARIABLE_DESC desc;
    D3D10_EFFECT_TYPE_DESC type_desc;
    HRESULT hr;

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

static void test_effect_variable_type_class(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer, *null_buffer, *parent;
    ID3D10EffectVariable *variable;
    ID3D10EffectType *type;
    D3D10_EFFECT_VARIABLE_DESC vd;
    D3D10_EFFECT_TYPE_DESC td;
    HRESULT hr;
    unsigned int variable_nr = 0;

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

    check_as((ID3D10EffectVariable *)variable);

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

static void test_effect_constant_buffer_stride(ID3D10Device *device)
{
    ID3D10Effect *effect;
    ID3D10EffectConstantBuffer *constantbuffer;
    ID3D10EffectType *type;
    D3D10_EFFECT_TYPE_DESC tdesc;
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

    hr = create_effect(fx_test_ecbs, 0, device, NULL, &effect);
    ok(SUCCEEDED(hr), "D3D10CreateEffectFromMemory failed (%x)\n", hr);

    for (i=0; i<sizeof(tv_ecbs)/sizeof(tv_ecbs[0]); i++)
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
}

START_TEST(effect)
{
    ID3D10Device *device;
    ULONG refcount;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests\n");
        return;
    }

    test_effect_constant_buffer_type(device);
    test_effect_variable_type(device);
    test_effect_variable_member(device);
    test_effect_variable_element(device);
    test_effect_variable_type_class(device);
    test_effect_constant_buffer_stride(device);

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
