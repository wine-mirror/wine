/*
 * Copyright 2010 Christian Costa
 * Copyright 2012 André Hentschel
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
#include <d3drm.h>
#include <initguid.h>

#include "wine/test.h"

static HMODULE d3drm_handle = 0;

static HRESULT (WINAPI * pDirect3DRMCreate)(LPDIRECT3DRM* ppDirect3DRM);

#define CHECK_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = get_refcount( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define D3DRM_GET_PROC(func) \
    p ## func = (void*)GetProcAddress(d3drm_handle, #func); \
    if(!p ## func) { \
      trace("GetProcAddress(%s) failed\n", #func); \
      FreeLibrary(d3drm_handle); \
      return FALSE; \
    }

static BOOL InitFunctionPtrs(void)
{
    d3drm_handle = LoadLibraryA("d3drm.dll");

    if(!d3drm_handle)
    {
        skip("Could not load d3drm.dll\n");
        return FALSE;
    }

    D3DRM_GET_PROC(Direct3DRMCreate)

    return TRUE;
}

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

static char data_bad_version[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static char data_no_mesh[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n";

static char data_ok[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n"
"Mesh Object\n"
"{\n"
"4;\n"
"1.0; 0.0; 0.0;,\n"
"0.0; 1.0; 0.0;,\n"
"0.0; 0.0; 1.0;,\n"
"1.0; 1.0; 1.0;;\n"
"3;\n"
"3; 0, 1, 2;,\n"
"3; 1, 2, 3;,\n"
"3; 3, 1, 2;;\n"
"}\n";

static void test_MeshBuilder(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMMESHBUILDER pMeshBuilder;
    D3DRMLOADMEMORY info;
    int val;
    DWORD val1, val2, val3;
    D3DVALUE valu, valv;

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMeshBuilder(pD3DRM, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    info.lpMemory = data_bad_version;
    info.dSize = strlen(data_bad_version);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_BADFILE, "Should have returned D3DRMERR_BADFILE (hr = %x)\n", hr);

    info.lpMemory = data_no_mesh;
    info.dSize = strlen(data_no_mesh);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_NOTFOUND, "Should have returned D3DRMERR_NOTFOUND (hr = %x)\n", hr);

    info.lpMemory = data_ok;
    info.dSize = strlen(data_ok);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    val = IDirect3DRMMeshBuilder_GetVertexCount(pMeshBuilder);
    ok(val == 4, "Wrong number of vertices %d (must be 4)\n", val);

    val = IDirect3DRMMeshBuilder_GetFaceCount(pMeshBuilder);
    ok(val == 3, "Wrong number of faces %d (must be 3)\n", val);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, NULL, &val2, NULL, &val3, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 4, "Wrong number of vertices %d (must be 4)\n", val1);
    todo_wine ok(val2 == 4, "Wrong number of normals %d (must be 4)\n", val2);
    todo_wine ok(val3 == 22, "Wrong number of face data bytes %d (must be 22)\n", val3);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 1, &valu, &valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    todo_wine ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    todo_wine ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_SetTextureCoordinates(pMeshBuilder, 1, valu, valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 1, &valu, &valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    todo_wine ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    todo_wine ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    IDirect3DRM_Release(pD3DRM);
}

static void test_MeshBuilder3(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRM3 pD3DRM3;
    LPDIRECT3DRMMESHBUILDER3 pMeshBuilder3;
    D3DRMLOADMEMORY info;
    int val;
    DWORD val1;
    D3DVALUE valu, valv;

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_QueryInterface(pD3DRM, &IID_IDirect3DRM3, (LPVOID*)&pD3DRM3);
    if (FAILED(hr))
    {
        win_skip("Cannot get IDirect3DRM3 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(pD3DRM);
        return;
    }

    hr = IDirect3DRM3_CreateMeshBuilder(pD3DRM3, &pMeshBuilder3);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder3 interface (hr = %x)\n", hr);

    info.lpMemory = data_bad_version;
    info.dSize = strlen(data_bad_version);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_BADFILE, "Should have returned D3DRMERR_BADFILE (hr = %x)\n", hr);

    info.lpMemory = data_no_mesh;
    info.dSize = strlen(data_no_mesh);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_NOTFOUND, "Should have returned D3DRMERR_NOTFOUND (hr = %x)\n", hr);

    info.lpMemory = data_ok;
    info.dSize = strlen(data_ok);
    hr = IDirect3DRMMeshBuilder3_Load(pMeshBuilder3, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    val = IDirect3DRMMeshBuilder3_GetVertexCount(pMeshBuilder3);
    ok(val == 4, "Wrong number of vertices %d (must be 4)\n", val);

    val = IDirect3DRMMeshBuilder3_GetFaceCount(pMeshBuilder3);
    ok(val == 3, "Wrong number of faces %d (must be 3)\n", val);

    hr = IDirect3DRMMeshBuilder3_GetVertices(pMeshBuilder3, 0, &val1, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 4, "Wrong number of vertices %d (must be 4)\n", val1);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 1, &valu, &valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    todo_wine ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    todo_wine ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_SetTextureCoordinates(pMeshBuilder3, 1, valu, valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 1, &valu, &valv);
    todo_wine ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    todo_wine ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    todo_wine ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder3_Release(pMeshBuilder3);
    IDirect3DRM3_Release(pD3DRM3);
    IDirect3DRM_Release(pD3DRM);
}

static void test_Frame(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMFRAME pFrameC;
    LPDIRECT3DRMFRAME pFrameP1;
    LPDIRECT3DRMFRAME pFrameP2;
    LPDIRECT3DRMFRAME pFrameTmp;
    LPDIRECT3DRMFRAMEARRAY pArray;
    DWORD count;

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrameC);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    todo_wine ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    todo_wine ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameC, 1);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameC, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    todo_wine ok(pArray != NULL, "pArray = %p\n", pArray);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 0, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrameP1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    /* [Add/Delete]Child with NULL pointer */
    hr = IDirect3DRMFrame_AddChild(pFrameP1, NULL);
    todo_wine ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    hr = IDirect3DRMFrame_DeleteChild(pFrameP1, NULL);
    todo_wine ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* Add child to first parent */
    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameP1, &pFrameTmp);
    todo_wine ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    todo_wine ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    hr = IDirect3DRMFrame_AddChild(pFrameP1, pFrameC);
    todo_wine ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP1, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    todo_wine ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    todo_wine ok(pFrameTmp == pFrameP1, "pFrameTmp = %p\n", pFrameTmp);
    todo_wine CHECK_REFCOUNT(pFrameP1, 2);

    /* Add child to second parent */
    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrameP2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    todo_wine ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP1, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 0, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    todo_wine ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    todo_wine ok(pFrameTmp == pFrameP2, "pFrameTmp = %p\n", pFrameTmp);
    todo_wine CHECK_REFCOUNT(pFrameP2, 2);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);

    /* Add child again */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    todo_wine ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    /* Delete child */
    hr = IDirect3DRMFrame_DeleteChild(pFrameP2, pFrameC);
    todo_wine ok(hr == D3DRM_OK, "Cannot delete child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 0, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    todo_wine ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    todo_wine ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    /* Add two children */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    todo_wine ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameP1);
    todo_wine ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    todo_wine CHECK_REFCOUNT(pFrameP1, 3);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    todo_wine ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 2, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 1, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameP1, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrame_Release(pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    IDirect3DRMMeshBuilder_Release(pFrameP2);
    todo_wine CHECK_REFCOUNT(pFrameC, 2);
    todo_wine CHECK_REFCOUNT(pFrameP1, 3);

    IDirect3DRMMeshBuilder_Release(pFrameC);
    IDirect3DRMMeshBuilder_Release(pFrameP1);

    IDirect3DRM_Release(pD3DRM);
}

START_TEST(d3drm)
{
    if (!InitFunctionPtrs())
        return;

    test_MeshBuilder();
    test_MeshBuilder3();
    test_Frame();

    FreeLibrary(d3drm_handle);
}
