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
#include "d3dcompiler.h"
#include "wine/test.h"

/*
 * This doesn't belong here, but for some functions it is possible to return that value,
 * see http://msdn.microsoft.com/en-us/library/bb205278%28v=VS.85%29.aspx
 * The original definition is in D3DX10core.h.
 */
#define D3DERR_INVALIDCALL 0x8876086c

#define MAKE_TAG(ch0, ch1, ch2, ch3) \
    ((DWORD)(ch0) | ((DWORD)(ch1) << 8) | \
    ((DWORD)(ch2) << 16) | ((DWORD)(ch3) << 24 ))
#define TAG_DXBC MAKE_TAG('D', 'X', 'B', 'C')
#define TAG_ISGN MAKE_TAG('I', 'S', 'G', 'N')

static void test_create_blob(void)
{
    ID3D10Blob *blob;
    HRESULT hr;
    ULONG refcount;

    hr = D3DCreateBlob(1, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DCreateBlob failed with %x\n", hr);

    hr = D3DCreateBlob(0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DCreateBlob failed with %x\n", hr);

    hr = D3DCreateBlob(0, &blob);
    ok(hr == S_OK, "D3DCreateBlob failed with %x\n", hr);

    refcount = blob->lpVtbl->Release(blob);
    ok(!refcount, "ID3DBlob has %u references left\n", refcount);
}

static const D3D_BLOB_PART parts[] =
{
   D3D_BLOB_INPUT_SIGNATURE_BLOB, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB,
   D3D_BLOB_PATCH_CONSTANT_SIGNATURE_BLOB, D3D_BLOB_ALL_SIGNATURE_BLOB, D3D_BLOB_DEBUG_INFO,
   D3D_BLOB_LEGACY_SHADER, D3D_BLOB_XNA_PREPASS_SHADER, D3D_BLOB_XNA_SHADER,
   D3D_BLOB_TEST_ALTERNATE_SHADER, D3D_BLOB_TEST_COMPILE_DETAILS, D3D_BLOB_TEST_COMPILE_PERF
};

/*
 * test_blob_part - fxc.exe /E VS /Tvs_4_0_level_9_0 /Fx
 */
#if 0
float4 VS(float4 position : POSITION, float4 pos : SV_POSITION) : SV_POSITION
{
  return position;
}
#endif
static DWORD test_blob_part[] = {
0x43425844, 0x0ef2a70f, 0x6a548011, 0x91ff9409, 0x9973a43d, 0x00000001, 0x000002e0, 0x00000008,
0x00000040, 0x0000008c, 0x000000d8, 0x0000013c, 0x00000180, 0x000001fc, 0x00000254, 0x000002ac,
0x53414e58, 0x00000044, 0x00000044, 0xfffe0200, 0x00000020, 0x00000024, 0x00240000, 0x00240000,
0x00240000, 0x00240000, 0x00240000, 0xfffe0200, 0x0200001f, 0x80000005, 0x900f0000, 0x02000001,
0xc00f0000, 0x80e40000, 0x0000ffff, 0x50414e58, 0x00000044, 0x00000044, 0xfffe0200, 0x00000020,
0x00000024, 0x00240000, 0x00240000, 0x00240000, 0x00240000, 0x00240000, 0xfffe0200, 0x0200001f,
0x80000005, 0x900f0000, 0x02000001, 0xc00f0000, 0x80e40000, 0x0000ffff, 0x396e6f41, 0x0000005c,
0x0000005c, 0xfffe0200, 0x00000034, 0x00000028, 0x00240000, 0x00240000, 0x00240000, 0x00240000,
0x00240001, 0x00000000, 0xfffe0200, 0x0200001f, 0x80000005, 0x900f0000, 0x04000004, 0xc0030000,
0x90ff0000, 0xa0e40000, 0x90e40000, 0x02000001, 0xc00c0000, 0x90e40000, 0x0000ffff, 0x52444853,
0x0000003c, 0x00010040, 0x0000000f, 0x0300005f, 0x001010f2, 0x00000000, 0x04000067, 0x001020f2,
0x00000000, 0x00000001, 0x05000036, 0x001020f2, 0x00000000, 0x00101e46, 0x00000000, 0x0100003e,
0x54415453, 0x00000074, 0x00000002, 0x00000000, 0x00000000, 0x00000002, 0x00000000, 0x00000000,
0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x46454452,
0x00000050, 0x00000000, 0x00000000, 0x00000000, 0x0000001c, 0xfffe0400, 0x00000100, 0x0000001c,
0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d,
0x39207265, 0x2e39322e, 0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000050, 0x00000002,
0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x00000041,
0x00000000, 0x00000000, 0x00000003, 0x00000001, 0x0000000f, 0x49534f50, 0x4e4f4954, 0x5f565300,
0x49534f50, 0x4e4f4954, 0xababab00, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x5449534f, 0x004e4f49,
};

static void test_get_blob_part(void)
{
    ID3DBlob *blob, *blob2;
    HRESULT hr;
    ULONG refcount;
    DWORD *dword;
    SIZE_T size;
    UINT i;

    hr = D3DCreateBlob(1, &blob2);
    ok(hr == S_OK, "D3DCreateBlob failed with %x\n", hr);
    blob = blob2;

    /* invalid cases */
    hr = D3DGetBlobPart(NULL, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(NULL, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(NULL, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);

    hr = D3DGetBlobPart(NULL, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);

    hr = D3DGetBlobPart(test_blob_part, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);

    hr = D3DGetBlobPart(test_blob_part, 0, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 1, &blob);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], 0xffffffff, 0, &blob);
    ok(hr == D3DERR_INVALIDCALL, "D3DGetBlobPart failed with %x\n", hr);
    ok(blob2 == blob, "D3DGetBlobPart failed got %p, expected %p\n", blob, blob2);

    refcount = ID3D10Blob_Release(blob2);
    ok(!refcount, "ID3DBlob has %u references left\n", refcount);

    /* D3D_BLOB_INPUT_SIGNATURE_BLOB */
    hr = D3DGetBlobPart(test_blob_part, test_blob_part[6], D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, &blob);
    ok(hr == S_OK, "D3DGetBlobPart failed, got %x, expected %x\n", hr, S_OK);

    size = ID3D10Blob_GetBufferSize(blob);
    ok(size == 124, "GetBufferSize failed, got %lu, expected %u\n", size, 124);

    dword = ((DWORD*)ID3D10Blob_GetBufferPointer(blob));
    ok(TAG_DXBC == *dword, "DXBC got %#x, expected %#x.\n", *dword, TAG_DXBC);
    ok(TAG_ISGN == *(dword+9), "ISGN got %#x, expected %#x.\n", *(dword+9), TAG_ISGN);

    for (i = 0; i < sizeof(parts) / sizeof(parts[0]); i++)
    {
        hr = D3DGetBlobPart(dword, size, parts[i], 0, &blob2);

        if (parts[i] == D3D_BLOB_INPUT_SIGNATURE_BLOB)
        {
            ok(hr == S_OK, "D3DGetBlobPart failed, got %x, expected %x\n", hr, S_OK);

            refcount = ID3D10Blob_Release(blob2);
            ok(!refcount, "ID3DBlob has %u references left\n", refcount);
        }
        else
        {
            ok(hr == E_FAIL, "D3DGetBlobPart failed, got %x, expected %x\n", hr, E_FAIL);
        }
    }

    refcount = ID3D10Blob_Release(blob);
    ok(!refcount, "ID3DBlob has %u references left\n", refcount);
}

START_TEST(blob)
{
    test_create_blob();
    test_get_blob_part();
}
