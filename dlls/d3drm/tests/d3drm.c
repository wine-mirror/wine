/*
 * Copyright 2010, 2012 Christian Costa
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
#include <d3drmwin.h>

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

static D3DRMMATRIX4D identity = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

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

static char data_full[] =
"xof 0302txt 0064\n"
"Header { 1; 0; 1; }\n"
"Mesh {\n"
" 3;\n"
" 0.1; 0.2; 0.3;,\n"
" 0.4; 0.5; 0.6;,\n"
" 0.7; 0.8; 0.9;;\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
" MeshMaterialList {\n"
"  1; 1; 0;\n"
"  Material {\n"
"   0.0; 1.0; 0.0; 1.0;;\n"
"   30.0;\n"
"   1.0; 0.0; 0.0;;\n"
"   0.5; 0.5; 0.5;;\n"
"   TextureFileName {\n"
"    \"Texture.bmp\";\n"
"   }\n"
"  }\n"
" }\n"
" MeshNormals {\n"
"  3;\n"
"  1.1; 1.2; 1.3;,\n"
"  1.4; 1.5; 1.6;,\n"
"  1.7; 1.8; 1.9;;\n"
"  1;"
"  3; 0, 1, 2;;\n"
" }\n"
" MeshTextureCoords {\n"
"  3;\n"
"  0.13; 0.17;,\n"
"  0.23; 0.27;,\n"
"  0.33; 0.37;;\n"
" }\n"
"}\n";

static char data_d3drm_load[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 0; 1;\n"
"}\n"
"Mesh Object1\n"
"{\n"
" 1;\n"
" 0.1; 0.2; 0.3;,\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
"}\n"
"Mesh Object2\n"
"{\n"
" 1;\n"
" 0.1; 0.2; 0.3;,\n"
" 1;\n"
" 3; 0, 1, 2;;\n"
"}\n"
"Frame Scene\n"
"{\n"
" {Object1}\n"
" {Object2}\n"
"}\n"
"Material\n"
"{\n"
" 0.1, 0.2, 0.3, 0.4;;\n"
" 0.5;\n"
" 0.6, 0.7, 0.8;;\n"
" 0.9, 1.0, 1.1;;\n"
"}\n";

static void test_MeshBuilder(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMMESHBUILDER pMeshBuilder;
    LPDIRECT3DRMMESH mesh;
    D3DRMLOADMEMORY info;
    int val;
    DWORD val1, val2, val3;
    D3DVALUE valu, valv;
    D3DVECTOR v[3];
    D3DVECTOR n[3];
    DWORD f[8];
    char name[10];
    DWORD size;
    D3DCOLOR color;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMeshBuilder(pD3DRM, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMeshBuilder_GetClassName(pMeshBuilder, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Builder"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Builder"), "Expected cname to be \"Builder\", but got \"%s\"\n", cname);

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

    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(!strcmp(name, "Object"), "Retrieved name '%s' instead of 'Object'\n", name);
    size = strlen("Object"); /* No space for null character */
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == E_INVALIDARG, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    hr = IDirect3DRMMeshBuilder_SetName(pMeshBuilder, NULL);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_SetName returned hr = %x\n", hr);
    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(size == 0, "Size should be 0 instead of %u\n", size);
    hr = IDirect3DRMMeshBuilder_SetName(pMeshBuilder, "");
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_SetName returned hr = %x\n", hr);
    size = sizeof(name);
    hr = IDirect3DRMMeshBuilder_GetName(pMeshBuilder, &size, name);
    ok(hr == D3DRM_OK, "IDirect3DRMMeshBuilder_GetName returned hr = %x\n", hr);
    ok(!strcmp(name, ""), "Retrieved name '%s' instead of ''\n", name);

    val = IDirect3DRMMeshBuilder_GetVertexCount(pMeshBuilder);
    ok(val == 4, "Wrong number of vertices %d (must be 4)\n", val);

    val = IDirect3DRMMeshBuilder_GetFaceCount(pMeshBuilder);
    ok(val == 3, "Wrong number of faces %d (must be 3)\n", val);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, NULL, &val2, NULL, &val3, NULL);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 4, "Wrong number of vertices %d (must be 4)\n", val1);
    ok(val2 == 4, "Wrong number of normals %d (must be 4)\n", val2);
    ok(val3 == 22, "Wrong number of face data bytes %d (must be 22)\n", val3);

    /* Check that Load method generated default texture coordinates (0.0f, 0.0f) for each vertex */
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 1, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 2, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 3, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 4, &valu, &valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder_SetTextureCoordinates(pMeshBuilder, 0, valu, valv);
    ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);
    hr = IDirect3DRMMeshBuilder_SetTextureCoordinates(pMeshBuilder, 4, valu, valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder_GetTextureCoordinates(pMeshBuilder, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    hr = IDirect3DRM_CreateMeshBuilder(pD3DRM, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    /* No group in mesh when mesh builder is not loaded */
    hr = IDirect3DRMMeshBuilder_CreateMesh(pMeshBuilder, &mesh);
    ok(hr == D3DRM_OK, "CreateMesh failed returning hr = %x\n", hr);
    if (hr == D3DRM_OK)
    {
        DWORD nb_groups;

        nb_groups = IDirect3DRMMesh_GetGroupCount(mesh);
        ok(nb_groups == 0, "GetCroupCount returned %u\n", nb_groups);

        IDirect3DRMMesh_Release(mesh);
    }

    info.lpMemory = data_full;
    info.dSize = strlen(data_full);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    val = IDirect3DRMMeshBuilder_GetVertexCount(pMeshBuilder);
    ok(val == 3, "Wrong number of vertices %d (must be 3)\n", val);

    val = IDirect3DRMMeshBuilder_GetFaceCount(pMeshBuilder);
    ok(val == 1, "Wrong number of faces %d (must be 1)\n", val);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val1 == 3, "Wrong number of vertices %d (must be 3)\n", val1);
    ok(val2 == 3, "Wrong number of normals %d (must be 3)\n", val2);
    ok(val3 == 8, "Wrong number of face data bytes %d (must be 8)\n", val3);
    ok(U1(v[0]).x == 0.1f, "Wrong component v[0].x = %f (expected 0.1)\n", U1(v[0]).x);
    ok(U2(v[0]).y == 0.2f, "Wrong component v[0].y = %f (expected 0.2)\n", U2(v[0]).y);
    ok(U3(v[0]).z == 0.3f, "Wrong component v[0].z = %f (expected 0.3)\n", U3(v[0]).z);
    ok(U1(v[1]).x == 0.4f, "Wrong component v[1].x = %f (expected 0.4)\n", U1(v[1]).x);
    ok(U2(v[1]).y == 0.5f, "Wrong component v[1].y = %f (expected 0.5)\n", U2(v[1]).y);
    ok(U3(v[1]).z == 0.6f, "Wrong component v[1].z = %f (expected 0.6)\n", U3(v[1]).z);
    ok(U1(v[2]).x == 0.7f, "Wrong component v[2].x = %f (expected 0.7)\n", U1(v[2]).x);
    ok(U2(v[2]).y == 0.8f, "Wrong component v[2].y = %f (expected 0.8)\n", U2(v[2]).y);
    ok(U3(v[2]).z == 0.9f, "Wrong component v[2].z = %f (expected 0.9)\n", U3(v[2]).z);
    ok(U1(n[0]).x == 1.1f, "Wrong component n[0].x = %f (expected 1.1)\n", U1(n[0]).x);
    ok(U2(n[0]).y == 1.2f, "Wrong component n[0].y = %f (expected 1.2)\n", U2(n[0]).y);
    ok(U3(n[0]).z == 1.3f, "Wrong component n[0].z = %f (expected 1.3)\n", U3(n[0]).z);
    ok(U1(n[1]).x == 1.4f, "Wrong component n[1].x = %f (expected 1.4)\n", U1(n[1]).x);
    ok(U2(n[1]).y == 1.5f, "Wrong component n[1].y = %f (expected 1.5)\n", U2(n[1]).y);
    ok(U3(n[1]).z == 1.6f, "Wrong component n[1].z = %f (expected 1.6)\n", U3(n[1]).z);
    ok(U1(n[2]).x == 1.7f, "Wrong component n[2].x = %f (expected 1.7)\n", U1(n[2]).x);
    ok(U2(n[2]).y == 1.8f, "Wrong component n[2].y = %f (expected 1.8)\n", U2(n[2]).y);
    ok(U3(n[2]).z == 1.9f, "Wrong component n[2].z = %f (expected 1.9)\n", U3(n[2]).z);
    ok(f[0] == 3 , "Wrong component f[0] = %d (expected 3)\n", f[0]);
    ok(f[1] == 0 , "Wrong component f[1] = %d (expected 0)\n", f[1]);
    ok(f[2] == 0 , "Wrong component f[2] = %d (expected 0)\n", f[2]);
    ok(f[3] == 1 , "Wrong component f[3] = %d (expected 1)\n", f[3]);
    ok(f[4] == 1 , "Wrong component f[4] = %d (expected 1)\n", f[4]);
    ok(f[5] == 2 , "Wrong component f[5] = %d (expected 2)\n", f[5]);
    ok(f[6] == 2 , "Wrong component f[6] = %d (expected 2)\n", f[6]);
    ok(f[7] == 0 , "Wrong component f[7] = %d (expected 0)\n", f[7]);

    hr = IDirect3DRMMeshBuilder_CreateMesh(pMeshBuilder, &mesh);
    ok(hr == D3DRM_OK, "CreateMesh failed returning hr = %x\n", hr);
    if (hr == D3DRM_OK)
    {
        DWORD nb_groups;
        unsigned nb_vertices, nb_faces, nb_face_vertices;
        DWORD data_size;
        LPDIRECT3DRMMATERIAL material = (LPDIRECT3DRMMATERIAL)0xdeadbeef;
        LPDIRECT3DRMTEXTURE texture = (LPDIRECT3DRMTEXTURE)0xdeadbeef;
        D3DVALUE values[3];

        nb_groups = IDirect3DRMMesh_GetGroupCount(mesh);
        ok(nb_groups == 1, "GetCroupCount returned %u\n", nb_groups);
        hr = IDirect3DRMMesh_GetGroup(mesh, 1, &nb_vertices, &nb_faces, &nb_face_vertices, &data_size, NULL);
        ok(hr == D3DRMERR_BADVALUE, "GetCroup returned hr = %x\n", hr);
        hr = IDirect3DRMMesh_GetGroup(mesh, 0, &nb_vertices, &nb_faces, &nb_face_vertices, &data_size, NULL);
        ok(hr == D3DRM_OK, "GetCroup failed returning hr = %x\n", hr);
        ok(nb_vertices == 3, "Wrong number of vertices %u (must be 3)\n", nb_vertices);
        ok(nb_faces == 1, "Wrong number of faces %u (must be 1)\n", nb_faces);
        todo_wine ok(nb_face_vertices == 3, "Wrong number of vertices per face %u (must be 3)\n", nb_face_vertices);
        todo_wine ok(data_size == 3, "Wrong number of face data bytes %u (must be 3)\n", data_size);
        color = IDirect3DRMMesh_GetGroupColor(mesh, 0);
        ok(color == 0xff00ff00, "Wrong color returned %#x instead of %#x\n", color, 0xff00ff00);
        hr = IDirect3DRMMesh_GetGroupTexture(mesh, 0, &texture);
        ok(hr == D3DRM_OK, "GetCroupTexture failed returning hr = %x\n", hr);
        ok(texture == NULL, "No texture should be present\n");
        hr = IDirect3DRMMesh_GetGroupMaterial(mesh, 0, &material);
        ok(hr == D3DRM_OK, "GetCroupMaterial failed returning hr = %x\n", hr);
        ok(material != NULL, "No material present\n");
        if ((hr == D3DRM_OK) && material)
        {
            hr = IDirect3DRMMaterial_GetEmissive(material, &values[0], &values[1], &values[2]);
            ok(hr == D3DRM_OK, "GetMaterialEmissive failed returning hr = %x\n", hr);
            ok(values[0] == 0.5f, "Emissive red component should be %f instead of %f\n", 0.5f, values[0]);
            ok(values[1] == 0.5f, "Emissive green component should be %f instead of %f\n", 0.5f, values[1]);
            ok(values[2] == 0.5f, "Emissive blue component should be %f instead of %f\n", 0.5f, values[2]);
            hr = IDirect3DRMMaterial_GetSpecular(material, &values[0], &values[1], &values[2]);
            ok(hr == D3DRM_OK, "GetMaterialEmissive failed returning hr = %x\n", hr);
            ok(values[0] == 1.0f, "Specular red component should be %f instead of %f\n", 1.0f, values[0]);
            ok(values[1] == 0.0f, "Specular green component should be %f instead of %f\n", 0.0f, values[1]);
            ok(values[2] == 0.0f, "Specular blue component should be %f instead of %f\n", 0.0f, values[2]);
            values[0] = IDirect3DRMMaterial_GetPower(material);
            ok(values[0] == 30.0f, "Power value should be %f instead of %f\n", 30.0f, values[0]);
        }

        IDirect3DRMMesh_Release(mesh);
    }

    hr = IDirect3DRMMeshBuilder_Scale(pMeshBuilder, 2, 3 ,4);
    ok(hr == D3DRM_OK, "Scale failed returning hr = %x\n", hr);

    hr = IDirect3DRMMeshBuilder_GetVertices(pMeshBuilder, &val1, v, &val2, n, &val3, f);
    ok(hr == D3DRM_OK, "Cannot get vertices information (hr = %x)\n", hr);
    ok(val2 == 3, "Wrong number of normals %d (must be 3)\n", val2);
    ok(val1 == 3, "Wrong number of vertices %d (must be 3)\n", val1);
    ok(U1(v[0]).x == 0.1f*2, "Wrong component v[0].x = %f (expected %f)\n", U1(v[0]).x, 0.1f*2);
    ok(U2(v[0]).y == 0.2f*3, "Wrong component v[0].y = %f (expected %f)\n", U2(v[0]).y, 0.2f*3);
    ok(U3(v[0]).z == 0.3f*4, "Wrong component v[0].z = %f (expected %f)\n", U3(v[0]).z, 0.3f*4);
    ok(U1(v[1]).x == 0.4f*2, "Wrong component v[1].x = %f (expected %f)\n", U1(v[1]).x, 0.4f*2);
    ok(U2(v[1]).y == 0.5f*3, "Wrong component v[1].y = %f (expected %f)\n", U2(v[1]).y, 0.5f*3);
    ok(U3(v[1]).z == 0.6f*4, "Wrong component v[1].z = %f (expected %f)\n", U3(v[1]).z, 0.6f*4);
    ok(U1(v[2]).x == 0.7f*2, "Wrong component v[2].x = %f (expected %f)\n", U1(v[2]).x, 0.7f*2);
    ok(U2(v[2]).y == 0.8f*3, "Wrong component v[2].y = %f (expected %f)\n", U2(v[2]).y, 0.8f*3);
    ok(U3(v[2]).z == 0.9f*4, "Wrong component v[2].z = %f (expected %f)\n", U3(v[2]).z, 0.9f*4);
    /* Normals are not affected by Scale */
    ok(U1(n[0]).x == 1.1f, "Wrong component n[0].x = %f (expected %f)\n", U1(n[0]).x, 1.1f);
    ok(U2(n[0]).y == 1.2f, "Wrong component n[0].y = %f (expected %f)\n", U2(n[0]).y, 1.2f);
    ok(U3(n[0]).z == 1.3f, "Wrong component n[0].z = %f (expected %f)\n", U3(n[0]).z, 1.3f);
    ok(U1(n[1]).x == 1.4f, "Wrong component n[1].x = %f (expected %f)\n", U1(n[1]).x, 1.4f);
    ok(U2(n[1]).y == 1.5f, "Wrong component n[1].y = %f (expected %f)\n", U2(n[1]).y, 1.5f);
    ok(U3(n[1]).z == 1.6f, "Wrong component n[1].z = %f (expected %f)\n", U3(n[1]).z, 1.6f);
    ok(U1(n[2]).x == 1.7f, "Wrong component n[2].x = %f (expected %f)\n", U1(n[2]).x, 1.7f);
    ok(U2(n[2]).y == 1.8f, "Wrong component n[2].y = %f (expected %f)\n", U2(n[2]).y, 1.8f);
    ok(U3(n[2]).z == 1.9f, "Wrong component n[2].z = %f (expected %f)\n", U3(n[2]).z, 1.9f);

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
    DWORD size;
    CHAR cname[64] = {0};

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

    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMeshBuilder3_GetClassName(pMeshBuilder3, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Builder"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Builder"), "Expected cname to be \"Builder\", but got \"%s\"\n", cname);

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

    /* Check that Load method generated default texture coordinates (0.0f, 0.0f) for each vertex */
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 1, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 2, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 3, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valu);
    ok(valv == 0.0f, "Wrong coordinate %f (must be 0.0)\n", valv);
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 4, &valu, &valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 1.23f;
    valv = 3.21f;
    hr = IDirect3DRMMeshBuilder3_SetTextureCoordinates(pMeshBuilder3, 0, valu, valv);
    ok(hr == D3DRM_OK, "Cannot set texture coordinates (hr = %x)\n", hr);
    hr = IDirect3DRMMeshBuilder3_SetTextureCoordinates(pMeshBuilder3, 4, valu, valv);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);

    valu = 0.0f;
    valv = 0.0f;
    hr = IDirect3DRMMeshBuilder3_GetTextureCoordinates(pMeshBuilder3, 0, &valu, &valv);
    ok(hr == D3DRM_OK, "Cannot get texture coordinates (hr = %x)\n", hr);
    ok(valu == 1.23f, "Wrong coordinate %f (must be 1.23)\n", valu);
    ok(valv == 3.21f, "Wrong coordinate %f (must be 3.21)\n", valv);

    IDirect3DRMMeshBuilder3_Release(pMeshBuilder3);
    IDirect3DRM3_Release(pD3DRM3);
    IDirect3DRM_Release(pD3DRM);
}

static void test_Mesh(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMMESH pMesh;
    DWORD size;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMesh(pD3DRM, &pMesh);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMesh interface (hr = %x)\n", hr);

    hr = IDirect3DRMMesh_GetClassName(pMesh, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMesh_GetClassName(pMesh, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMesh_GetClassName(pMesh, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMesh_GetClassName(pMesh, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Mesh"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Mesh"), "Expected cname to be \"Mesh\", but got \"%s\"\n", cname);

    IDirect3DRMMesh_Release(pMesh);

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
    LPDIRECT3DRMMESHBUILDER pMeshBuilder;
    LPDIRECT3DRMVISUAL pVisual1;
    LPDIRECT3DRMVISUAL pVisualTmp;
    LPDIRECT3DRMVISUALARRAY pVisualArray;
    LPDIRECT3DRMLIGHT pLight1;
    LPDIRECT3DRMLIGHT pLightTmp;
    LPDIRECT3DRMLIGHTARRAY pLightArray;
    DWORD count;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrameC);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    hr = IDirect3DRMFrame_GetClassName(pFrameC, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMFrame_GetClassName(pFrameC, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = 1;
    hr = IDirect3DRMFrame_GetClassName(pFrameC, &count, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    count = sizeof(cname);
    hr = IDirect3DRMFrame_GetClassName(pFrameC, &count, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(count == sizeof("Frame"), "wrong size: %u\n", count);
    ok(!strcmp(cname, "Frame"), "Expected cname to be \"Frame\", but got \"%s\"\n", cname);

    hr = IDirect3DRMFrame_GetParent(pFrameC, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Should fail and return D3DRM_BADVALUE (hr = %x)\n", hr);
    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameC, 1);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameC, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    ok(pArray != NULL, "pArray = %p\n", pArray);
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

    /* GetParent with NULL pointer */
    hr = IDirect3DRMFrame_GetParent(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* [Add/Delete]Child with NULL pointer */
    hr = IDirect3DRMFrame_AddChild(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    hr = IDirect3DRMFrame_DeleteChild(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);

    /* Add child to first parent */
    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameP1, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    hr = IDirect3DRMFrame_AddChild(pFrameP1, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 1);
    CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP1, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    /* In some older version of d3drm, creating IDirect3DRMFrameArray object with GetChildren does not increment refcount of children frames */
    ok((get_refcount((IUnknown*)pFrameC) == 3) || broken(get_refcount((IUnknown*)pFrameC) == 2),
            "Invalid refcount. Expected 3 (or 2) got %d\n", get_refcount((IUnknown*)pFrameC));
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pFrameTmp == pFrameC, "pFrameTmp = %p\n", pFrameTmp);
        ok((get_refcount((IUnknown*)pFrameC) == 4) || broken(get_refcount((IUnknown*)pFrameC) == 3),
                "Invalid refcount. Expected 4 (or 3) got %d\n", get_refcount((IUnknown*)pFrameC));
        IDirect3DRMFrame_Release(pFrameTmp);
        ok((get_refcount((IUnknown*)pFrameC) == 3) || broken(get_refcount((IUnknown*)pFrameC) == 2),
                "Invalid refcount. Expected 3 (or 2) got %d\n", get_refcount((IUnknown*)pFrameC));
        IDirect3DRMFrameArray_Release(pArray);
        CHECK_REFCOUNT(pFrameC, 2);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == pFrameP1, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameP1, 2);

    /* Add child to second parent */
    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrameP2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
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
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 0, "count = %u\n", count);
        pFrameTmp = (void*)0xdeadbeef;
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == pFrameP2, "pFrameTmp = %p\n", pFrameTmp);
    CHECK_REFCOUNT(pFrameP2, 2);
    CHECK_REFCOUNT(pFrameC, 2);

    /* Add child again */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
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
    ok(hr == D3DRM_OK, "Cannot delete child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 1);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
    if (pArray)
    {
        count = IDirect3DRMFrameArray_GetSize(pArray);
        ok(count == 0, "count = %u\n", count);
        pFrameTmp = (void*)0xdeadbeef;
        hr = IDirect3DRMFrameArray_GetElement(pArray, 0, &pFrameTmp);
        ok(hr == D3DRMERR_BADVALUE, "Should have returned D3DRMERR_BADVALUE (hr = %x)\n", hr);
        ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);
        IDirect3DRMFrameArray_Release(pArray);
    }

    pFrameTmp = (void*)0xdeadbeef;
    hr = IDirect3DRMFrame_GetParent(pFrameC, &pFrameTmp);
    ok(hr == D3DRM_OK, "Cannot get parent frame (hr = %x)\n", hr);
    ok(pFrameTmp == NULL, "pFrameTmp = %p\n", pFrameTmp);

    /* Add two children */
    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameC);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameC, 2);

    hr = IDirect3DRMFrame_AddChild(pFrameP2, pFrameP1);
    ok(hr == D3DRM_OK, "Cannot add child frame (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    pArray = NULL;
    hr = IDirect3DRMFrame_GetChildren(pFrameP2, &pArray);
    ok(hr == D3DRM_OK, "Cannot get children (hr = %x)\n", hr);
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

    /* [Add/Delete]Visual with NULL pointer */
    hr = IDirect3DRMFrame_AddVisual(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    hr = IDirect3DRMFrame_DeleteVisual(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    /* Create Visual */
    hr = IDirect3DRM_CreateMeshBuilder(pD3DRM, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);
    pVisual1 = (LPDIRECT3DRMVISUAL)pMeshBuilder;

    /* Add Visual to first parent */
    hr = IDirect3DRMFrame_AddVisual(pFrameP1, pVisual1);
    ok(hr == D3DRM_OK, "Cannot add visual (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
    CHECK_REFCOUNT(pVisual1, 2);

    pVisualArray = NULL;
    hr = IDirect3DRMFrame_GetVisuals(pFrameP1, &pVisualArray);
    ok(hr == D3DRM_OK, "Cannot get visuals (hr = %x)\n", hr);
    if (pVisualArray)
    {
        count = IDirect3DRMVisualArray_GetSize(pVisualArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMVisualArray_GetElement(pVisualArray, 0, &pVisualTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pVisualTmp == pVisual1, "pVisualTmp = %p\n", pVisualTmp);
        IDirect3DRMVisual_Release(pVisualTmp);
        IDirect3DRMVisualArray_Release(pVisualArray);
    }

    /* Delete Visual */
    hr = IDirect3DRMFrame_DeleteVisual(pFrameP1, pVisual1);
    ok(hr == D3DRM_OK, "Cannot delete visual (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    /* [Add/Delete]Light with NULL pointer */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    hr = IDirect3DRMFrame_DeleteLight(pFrameP1, NULL);
    ok(hr == D3DRMERR_BADOBJECT, "Should have returned D3DRMERR_BADOBJECT (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);

    /* Create Light */
    hr = IDirect3DRM_CreateLightRGB(pD3DRM, D3DRMLIGHT_SPOT, 0.1, 0.2, 0.3, &pLight1);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    /* Add Light to first parent */
    hr = IDirect3DRMFrame_AddLight(pFrameP1, pLight1);
    ok(hr == D3DRM_OK, "Cannot add light (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
    CHECK_REFCOUNT(pLight1, 2);

    pLightArray = NULL;
    hr = IDirect3DRMFrame_GetLights(pFrameP1, &pLightArray);
    ok(hr == D3DRM_OK, "Cannot get lights (hr = %x)\n", hr);
    if (pLightArray)
    {
        count = IDirect3DRMLightArray_GetSize(pLightArray);
        ok(count == 1, "count = %u\n", count);
        hr = IDirect3DRMLightArray_GetElement(pLightArray, 0, &pLightTmp);
        ok(hr == D3DRM_OK, "Cannot get element (hr = %x)\n", hr);
        ok(pLightTmp == pLight1, "pLightTmp = %p\n", pLightTmp);
        IDirect3DRMLight_Release(pLightTmp);
        IDirect3DRMLightArray_Release(pLightArray);
    }

    /* Delete Light */
    hr = IDirect3DRMFrame_DeleteLight(pFrameP1, pLight1);
    ok(hr == D3DRM_OK, "Cannot delete light (hr = %x)\n", hr);
    CHECK_REFCOUNT(pFrameP1, 3);
    IDirect3DRMLight_Release(pLight1);

    /* Cleanup */
    IDirect3DRMFrame_Release(pFrameP2);
    CHECK_REFCOUNT(pFrameC, 2);
    CHECK_REFCOUNT(pFrameP1, 3);

    IDirect3DRMFrame_Release(pFrameC);
    IDirect3DRMFrame_Release(pFrameP1);

    IDirect3DRM_Release(pD3DRM);
}

static void test_Viewport(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECTDRAWCLIPPER pClipper;
    LPDIRECT3DRMDEVICE pDevice;
    LPDIRECT3DRMFRAME pFrame;
    LPDIRECT3DRMVIEWPORT pViewport;
    GUID driver;
    HWND window;
    RECT rc;
    DWORD size;
    CHAR cname[64] = {0};

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
    GetClientRect(window, &rc);

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = DirectDrawCreateClipper(0, &pClipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x)\n", hr);

    hr = IDirectDrawClipper_SetHWnd(pClipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x)\n", hr);

    memcpy(&driver, &IID_IDirect3DRGBDevice, sizeof(GUID));
    hr = IDirect3DRM3_CreateDeviceFromClipper(pD3DRM, pClipper, &driver, rc.right, rc.bottom, &pDevice);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(pD3DRM, NULL, &pFrame);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateViewport(pD3DRM, pDevice, pFrame, rc.left, rc.top, rc.right, rc.bottom, &pViewport);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMViewport interface (hr = %x)\n", hr);

    hr = IDirect3DRMViewport_GetClassName(pViewport, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMViewport_GetClassName(pViewport, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMViewport_GetClassName(pViewport, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMViewport_GetClassName(pViewport, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Viewport"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Viewport"), "Expected cname to be \"Viewport\", but got \"%s\"\n", cname);

    IDirect3DRMViewport_Release(pViewport);
    IDirect3DRMFrame_Release(pFrame);
    IDirect3DRMDevice_Release(pDevice);
    IDirectDrawClipper_Release(pClipper);

    IDirect3DRM_Release(pD3DRM);
    DestroyWindow(window);
}

static void test_Light(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMLIGHT pLight;
    D3DRMLIGHTTYPE type;
    D3DCOLOR color;
    DWORD size;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateLightRGB(pD3DRM, D3DRMLIGHT_SPOT, 0.5, 0.5, 0.5, &pLight);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMLight interface (hr = %x)\n", hr);

    hr = IDirect3DRMLight_GetClassName(pLight, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMLight_GetClassName(pLight, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMLight_GetClassName(pLight, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMLight_GetClassName(pLight, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Light"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Light"), "Expected cname to be \"Light\", but got \"%s\"\n", cname);

    type = IDirect3DRMLight_GetType(pLight);
    ok(type == D3DRMLIGHT_SPOT, "wrong type (%u)\n", type);

    color = IDirect3DRMLight_GetColor(pLight);
    ok(color == 0xff7f7f7f, "wrong color (%x)\n", color);

    hr = IDirect3DRMLight_SetType(pLight, D3DRMLIGHT_POINT);
    ok(hr == D3DRM_OK, "Cannot set type (hr = %x)\n", hr);
    type = IDirect3DRMLight_GetType(pLight);
    ok(type == D3DRMLIGHT_POINT, "wrong type (%u)\n", type);

    hr = IDirect3DRMLight_SetColor(pLight, 0xff180587);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMLight_GetColor(pLight);
    ok(color == 0xff180587, "wrong color (%x)\n", color);

    hr = IDirect3DRMLight_SetColorRGB(pLight, 0.5, 0.5, 0.5);
    ok(hr == D3DRM_OK, "Cannot set color (hr = %x)\n", hr);
    color = IDirect3DRMLight_GetColor(pLight);
    ok(color == 0xff7f7f7f, "wrong color (%x)\n", color);

    IDirect3DRMLight_Release(pLight);

    IDirect3DRM_Release(pD3DRM);
}

static void test_Material2(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRM3 pD3DRM3;
    LPDIRECT3DRMMATERIAL2 pMaterial2;
    D3DVALUE r, g, b;
    DWORD size;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_QueryInterface(pD3DRM, &IID_IDirect3DRM3, (LPVOID*)&pD3DRM3);
    if (FAILED(hr))
    {
        win_skip("Cannot get IDirect3DRM3 interface (hr = %x), skipping tests\n", hr);
        IDirect3DRM_Release(pD3DRM);
        return;
    }

    hr = IDirect3DRM3_CreateMaterial(pD3DRM3, 18.5f, &pMaterial2);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMaterial2 interface (hr = %x)\n", hr);

    hr = IDirect3DRMMaterial2_GetClassName(pMaterial2, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMMaterial2_GetClassName(pMaterial2, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMMaterial2_GetClassName(pMaterial2, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMMaterial2_GetClassName(pMaterial2, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Material"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Material"), "Expected cname to be \"Material\", but got \"%s\"\n", cname);

    r = IDirect3DRMMaterial2_GetPower(pMaterial2);
    ok(r == 18.5f, "wrong power (%f)\n", r);

    hr = IDirect3DRMMaterial2_GetEmissive(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.0f && g == 0.0f && b == 0.0f, "wrong emissive r=%f g=%f b=%f, expected r=0.0 g=0.0 b=0.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_GetSpecular(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 1.0f && g == 1.0f && b == 1.0f, "wrong specular r=%f g=%f b=%f, expected r=1.0 g=1.0 b=1.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_GetAmbient(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.0f && g == 0.0f && b == 0.0f, "wrong ambient r=%f g=%f b=%f, expected r=0.0 g=0.0 b=0.0\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetPower(pMaterial2, 5.87f);
    ok(hr == D3DRM_OK, "Cannot set power (hr = %x)\n", hr);
    r = IDirect3DRMMaterial2_GetPower(pMaterial2);
    ok(r == 5.87f, "wrong power (%f)\n", r);

    hr = IDirect3DRMMaterial2_SetEmissive(pMaterial2, 0.5f, 0.5f, 0.5f);
    ok(hr == D3DRM_OK, "Cannot set emissive (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetEmissive(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get emissive (hr = %x)\n", hr);
    ok(r == 0.5f && g == 0.5f && b == 0.5f, "wrong emissive r=%f g=%f b=%f, expected r=0.5 g=0.5 b=0.5\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetSpecular(pMaterial2, 0.6f, 0.6f, 0.6f);
    ok(hr == D3DRM_OK, "Cannot set specular (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetSpecular(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get specular (hr = %x)\n", hr);
    ok(r == 0.6f && g == 0.6f && b == 0.6f, "wrong specular r=%f g=%f b=%f, expected r=0.6 g=0.6 b=0.6\n", r, g, b);

    hr = IDirect3DRMMaterial2_SetAmbient(pMaterial2, 0.7f, 0.7f, 0.7f);
    ok(hr == D3DRM_OK, "Cannot set ambient (hr = %x)\n", hr);
    hr = IDirect3DRMMaterial2_GetAmbient(pMaterial2, &r, &g, &b);
    ok(hr == D3DRM_OK, "Cannot get ambient (hr = %x)\n", hr);
    ok(r == 0.7f && g == 0.7f && b == 0.7f, "wrong ambient r=%f g=%f b=%f, expected r=0.7 g=0.7 b=0.7\n", r, g, b);

    IDirect3DRMMaterial2_Release(pMaterial2);

    IDirect3DRM3_Release(pD3DRM3);
    IDirect3DRM_Release(pD3DRM);
}

static void test_Texture(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMTEXTURE pTexture;
    D3DRMIMAGE initimg = {
        2, 2, 1, 1, 32,
        TRUE, 2 * sizeof(DWORD), NULL, NULL,
        0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, 0, NULL
    };
    DWORD pixel[4] = { 20000, 30000, 10000, 0 };
    DWORD size;
    CHAR cname[64] = {0};

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    initimg.buffer1 = &pixel;
    hr = IDirect3DRM_CreateTexture(pD3DRM, &initimg, &pTexture);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMTexture interface (hr = %x)\n", hr);

    hr = IDirect3DRMTexture_GetClassName(pTexture, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMTexture_GetClassName(pTexture, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMTexture_GetClassName(pTexture, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMTexture_GetClassName(pTexture, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Texture"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Texture"), "Expected cname to be \"Texture\", but got \"%s\"\n", cname);

    IDirect3DRMTexture_Release(pTexture);

    IDirect3DRM_Release(pD3DRM);
}

static void test_Device(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECTDRAWCLIPPER pClipper;
    LPDIRECT3DRMDEVICE pDevice;
    LPDIRECT3DRMWINDEVICE pWinDevice;
    GUID driver;
    HWND window;
    RECT rc;
    DWORD size;
    CHAR cname[64] = {0};

    window = CreateWindowA("static", "d3drm_test", WS_OVERLAPPEDWINDOW, 0, 0, 300, 200, 0, 0, 0, 0);
    GetClientRect(window, &rc);

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = DirectDrawCreateClipper(0, &pClipper, NULL);
    ok(hr == DD_OK, "Cannot get IDirectDrawClipper interface (hr = %x)\n", hr);

    hr = IDirectDrawClipper_SetHWnd(pClipper, 0, window);
    ok(hr == DD_OK, "Cannot set HWnd to Clipper (hr = %x)\n", hr);

    memcpy(&driver, &IID_IDirect3DRGBDevice, sizeof(GUID));
    hr = IDirect3DRM3_CreateDeviceFromClipper(pD3DRM, pClipper, &driver, rc.right, rc.bottom, &pDevice);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMDevice interface (hr = %x)\n", hr);

    hr = IDirect3DRMDevice_GetClassName(pDevice, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMDevice_GetClassName(pDevice, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMDevice_GetClassName(pDevice, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMDevice_GetClassName(pDevice, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Device"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Device"), "Expected cname to be \"Device\", but got \"%s\"\n", cname);

    /* WinDevice */
    hr = IDirect3DRMDevice_QueryInterface(pDevice, &IID_IDirect3DRMWinDevice, (LPVOID*)&pWinDevice);
    if (FAILED(hr))
    {
        win_skip("Cannot get IDirect3DRMWinDevice interface (hr = %x), skipping tests\n", hr);
        goto cleanup;
    }

    hr = IDirect3DRMWinDevice_GetClassName(pWinDevice, NULL, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    hr = IDirect3DRMWinDevice_GetClassName(pWinDevice, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = 1;
    hr = IDirect3DRMWinDevice_GetClassName(pWinDevice, &size, cname);
    ok(hr == E_INVALIDARG, "GetClassName failed with %x\n", hr);
    size = sizeof(cname);
    hr = IDirect3DRMWinDevice_GetClassName(pWinDevice, &size, cname);
    ok(hr == D3DRM_OK, "Cannot get classname (hr = %x)\n", hr);
    ok(size == sizeof("Device"), "wrong size: %u\n", size);
    ok(!strcmp(cname, "Device"), "Expected cname to be \"Device\", but got \"%s\"\n", cname);

    IDirect3DRMWinDevice_Release(pWinDevice);

cleanup:
    IDirect3DRMDevice_Release(pDevice);
    IDirectDrawClipper_Release(pClipper);

    IDirect3DRM_Release(pD3DRM);
    DestroyWindow(window);
}

static void test_frame_transform(void)
{
    HRESULT hr;
    LPDIRECT3DRM d3drm;
    LPDIRECT3DRMFRAME frame;
    D3DRMMATRIX4D matrix;

    hr = pDirect3DRMCreate(&d3drm);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateFrame(d3drm, NULL, &frame);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMFrame interface (hr = %x)\n", hr);

    hr = IDirect3DRMFrame_GetTransform(frame, matrix);
    ok(hr == D3DRM_OK, "IDirect3DRMFrame_GetTransform returned hr = %x\n", hr);
    ok(!memcmp(matrix, identity, sizeof(D3DRMMATRIX4D)), "Returned matrix is not identity\n");

    IDirect3DRM_Release(d3drm);
}

static int nb_objects = 0;
static const GUID* refiids[] =
{
    &IID_IDirect3DRMMeshBuilder,
    &IID_IDirect3DRMMeshBuilder,
    &IID_IDirect3DRMFrame,
    &IID_IDirect3DRMMaterial /* Not taken into account and not notified */
};

static void __cdecl object_load_callback(LPDIRECT3DRMOBJECT object, REFIID objectguid, LPVOID arg)
{
    ok(object != NULL, "Arg 1 should not be null\n");
    ok(IsEqualGUID(objectguid, refiids[nb_objects]), "Arg 2 is incorrect\n");
    ok(arg == (LPVOID)0xdeadbeef, "Arg 3 should be 0xdeadbeef (got %p)\n", arg);
    nb_objects++;
}

static void test_d3drm_load(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    D3DRMLOADMEMORY info;
    const GUID* req_refiids[] = { &IID_IDirect3DRMMeshBuilder, &IID_IDirect3DRMFrame, &IID_IDirect3DRMMaterial };

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    info.lpMemory = data_d3drm_load;
    info.dSize = strlen(data_d3drm_load);
    hr = IDirect3DRM_Load(pD3DRM, &info, NULL, (GUID**)req_refiids, 3, D3DRMLOAD_FROMMEMORY, object_load_callback, (LPVOID)0xdeadbeef, NULL, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load data (hr = %x)\n", hr);
    ok(nb_objects == 3, "Should have loaded 3 objects (got %d)\n", nb_objects);

    IDirect3DRM_Release(pD3DRM);
}

START_TEST(d3drm)
{
    if (!InitFunctionPtrs())
        return;

    test_MeshBuilder();
    test_MeshBuilder3();
    test_Mesh();
    test_Frame();
    test_Device();
    test_Viewport();
    test_Light();
    test_Material2();
    test_Texture();
    test_frame_transform();
    test_d3drm_load();

    FreeLibrary(d3drm_handle);
}
