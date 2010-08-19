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

START_TEST(blob)
{
    test_create_blob();
}
