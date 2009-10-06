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

    refcount = ID3D10Device_Release(device);
    ok(!refcount, "Device has %u references left\n", refcount);
}
