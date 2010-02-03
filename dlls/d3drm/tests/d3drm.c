/*
 * Copyright 2010 Christian Costa
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

#include "d3drm.h"

#include "wine/test.h"

static HMODULE d3drm_handle = 0;

static HRESULT (WINAPI * pDirect3DRMCreate)(LPDIRECT3DRM* ppDirect3DRM);

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

char data_ok[] =
"xof 0302txt 0064\n"
"Mesh Object\n"
"{\n"
"0;\n"
"0;\n"
"}\n";

char data_bad[] =
"xof 0302txt 0064\n"
"Header Object\n"
"{\n"
"1; 2; 3;\n"
"}\n";

static void MeshBuilderTest(void)
{
    HRESULT hr;
    LPDIRECT3DRM pD3DRM;
    LPDIRECT3DRMMESHBUILDER pMeshBuilder;
    D3DRMLOADMEMORY info;

    hr = pDirect3DRMCreate(&pD3DRM);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRM interface (hr = %x)\n", hr);

    hr = IDirect3DRM_CreateMeshBuilder(pD3DRM, &pMeshBuilder);
    ok(hr == D3DRM_OK, "Cannot get IDirect3DRMMeshBuilder interface (hr = %x)\n", hr);

    info.lpMemory = data_bad;
    info.dSize = sizeof(data_bad);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRMERR_BADFILE, "Sould have returned D3DRMERR_BADFILE (hr = %x)\n", hr);

    info.lpMemory = data_ok;
    info.dSize = sizeof(data_ok);
    hr = IDirect3DRMMeshBuilder_Load(pMeshBuilder, &info, NULL, D3DRMLOAD_FROMMEMORY, NULL, NULL);
    ok(hr == D3DRM_OK, "Cannot load mesh data (hr = %x)\n", hr);

    IDirect3DRMMeshBuilder_Release(pMeshBuilder);

    IDirect3DRM_Release(pD3DRM);
}

START_TEST(d3drm)
{
    if (!InitFunctionPtrs())
        return;

    MeshBuilderTest();

    FreeLibrary(d3drm_handle);
}
