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

    /*
     * Don't use sizeof(fx_test_ecbt), use fx_test_ecbt[6] as size,
     * because the DWORD fx_test_ecbt[] has only complete DWORDs and
     * so it could happen that there are padded bytes at the end.
     *
     * The fx size (fx_test_ecbt[6]) could be up to 3 BYTEs smaller
     * than the sizeof(fx_test_ecbt).
     */
    hr = D3D10CreateEffectFromMemory(fx_test_ecbt, fx_test_ecbt[6], 0, device, NULL, &effect);
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

    hr = D3D10CreateEffectFromMemory(fx_test_evt, fx_test_evt[6], 0, device, NULL, &effect);
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

    hr = D3D10CreateEffectFromMemory(fx_test_evm, fx_test_evm[6], 0, device, NULL, &effect);
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

    hr = D3D10CreateEffectFromMemory(fx_test_eve, fx_test_eve[6], 0, device, NULL, &effect);
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

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
