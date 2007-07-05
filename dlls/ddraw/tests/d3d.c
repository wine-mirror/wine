/*
 * Some unit tests for d3d functions
 *
 * Copyright (C) 2005 Antoine Chavasse
 * Copyright (C) 2006 Stefan Dösinger for CodeWeavers
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

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

static LPDIRECTDRAW7           lpDD = NULL;
static LPDIRECT3D7             lpD3D = NULL;
static LPDIRECTDRAWSURFACE7    lpDDS = NULL;
static LPDIRECTDRAWSURFACE7    lpDDSdepth = NULL;
static LPDIRECT3DDEVICE7       lpD3DDevice = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufSrc = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufDest1 = NULL;
static LPDIRECT3DVERTEXBUFFER7 lpVBufDest2 = NULL;

/* To compare bad floating point numbers. Not the ideal way to do it,
 * but it should be enough for here */
#define comparefloat(a, b) ( (((a) - (b)) < 0.0001) && (((a) - (b)) > -0.0001) )

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

typedef struct _VERTEX
{
    float x, y, z;  /* position */
} VERTEX, *LPVERTEX;

typedef struct _TVERTEX
{
    float x, y, z;  /* position */
    float rhw;
} TVERTEX, *LPTVERTEX;


static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
}


static BOOL CreateDirect3D(void)
{
    HRESULT rc;
    DDSURFACEDESC2 ddsd;

    rc = pDirectDrawCreateEx(NULL, (void**)&lpDD,
        &IID_IDirectDraw7, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %x\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %x\n", rc);
        return FALSE;
    }

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK, "SetCooperativeLevel returned: %x\n", rc);

    rc = IDirectDraw7_QueryInterface(lpDD, &IID_IDirect3D7, (void**) &lpD3D);
    if (rc == E_NOINTERFACE) return FALSE;
    ok(rc==DD_OK, "QueryInterface returned: %x\n", rc);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
    ok(rc==DD_OK, "CreateSurface returned: %x\n", rc);
    if (!SUCCEEDED(rc))
	return FALSE;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(U4(ddsd).ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(U4(ddsd).ddpfPixelFormat).dwZBitMask = 0x0000FFFF;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDSdepth, NULL);
    ok(rc==DD_OK, "CreateSurface returned: %x\n", rc);
    if (!SUCCEEDED(rc)) {
        lpDDSdepth = NULL;
    } else {
        rc = IDirectDrawSurface_AddAttachedSurface(lpDDS, lpDDSdepth);
        ok(rc == DD_OK, "IDirectDrawSurface_AddAttachedSurface returned %x\n", rc);
        if (!SUCCEEDED(rc))
            return FALSE;
    }

    rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DTnLHalDevice, lpDDS,
        &lpD3DDevice);
    ok(rc==D3D_OK || rc==DDERR_NOPALETTEATTACHED || rc==E_OUTOFMEMORY, "CreateDevice returned: %x\n", rc);
    if (!lpD3DDevice) {
        trace("IDirect3D7::CreateDevice() for a TnL Hal device failed with an error %x, trying HAL\n", rc);
        rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DHALDevice, lpDDS,
            &lpD3DDevice);
        if (!lpD3DDevice) {
            trace("IDirect3D7::CreateDevice() for a HAL device failed with an error %x, trying RGB\n", rc);
            rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DRGBDevice, lpDDS,
                &lpD3DDevice);
            if (!lpD3DDevice) {
                trace("IDirect3D7::CreateDevice() for a RGB device failed with an error %x, giving up\n", rc);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static void ReleaseDirect3D(void)
{
    if (lpD3DDevice != NULL)
    {
        IDirect3DDevice7_Release(lpD3DDevice);
        lpD3DDevice = NULL;
    }

    if (lpDDSdepth != NULL)
    {
        IDirectDrawSurface_Release(lpDDSdepth);
        lpDDSdepth = NULL;
    }

    if (lpDDS != NULL)
    {
        IDirectDrawSurface_Release(lpDDS);
        lpDDS = NULL;
    }

    if (lpD3D != NULL)
    {
        IDirect3D7_Release(lpD3D);
        lpD3D = NULL;
    }

    if (lpDD != NULL)
    {
        IDirectDraw_Release(lpDD);
        lpDD = NULL;
    }
}

static void LightTest(void)
{
    HRESULT rc;
    D3DLIGHT7 light;
    D3DLIGHT7 defaultlight;
    BOOL bEnabled = FALSE;
    float one = 1.0f;
    float zero= 0.0f;
    D3DMATERIAL7 mat;

    /* Set a few lights with funky indices. */
    memset(&light, 0, sizeof(light));
    light.dltType = D3DLIGHT_DIRECTIONAL;
    U1(light.dcvDiffuse).r = 0.5f;
    U2(light.dcvDiffuse).g = 0.6f;
    U3(light.dcvDiffuse).b = 0.7f;
    U2(light.dvDirection).y = 1.f;

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 5, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 45, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);


    /* Try to retrieve a light beyond the indices of the lights that have
       been set. */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %x\n", rc);
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 2, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %x\n", rc);


    /* Try to retrieve one of the lights that have been set */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);


    /* Enable a light that have been previously set. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 10, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);


    /* Enable some lights that have not been previously set, and verify that
       they have been initialized with proper default values. */
    memset(&defaultlight, 0, sizeof(D3DLIGHT7));
    defaultlight.dltType = D3DLIGHT_DIRECTIONAL;
    U1(defaultlight.dcvDiffuse).r = 1.f;
    U2(defaultlight.dcvDiffuse).g = 1.f;
    U3(defaultlight.dcvDiffuse).b = 1.f;
    U3(defaultlight.dvDirection).z = 1.f;

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 20, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 50, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==D3D_OK, "GetLight returned: %x\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );


    /* Disable one of the light that have been previously enabled. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, FALSE);
    ok(rc==D3D_OK, "LightEnable returned: %x\n", rc);

    /* Try to retrieve the enable status of some lights */
    /* Light 20 is supposed to be disabled */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 20, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %x\n", rc);
    ok(!bEnabled, "GetLightEnable says the light is enabled\n");

    /* Light 10 is supposed to be enabled */
    bEnabled = FALSE;
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 10, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %x\n", rc);
    ok(bEnabled, "GetLightEnable says the light is disabled\n");

    /* Light 80 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 80, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %x\n", rc);

    /* Light 23 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 23, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %x\n", rc);

    /* Set some lights with invalid parameters */
    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 0;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 100, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = 12345;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 101, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 102, NULL);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    memset(&light, 0, sizeof(D3DLIGHT7));
    light.dltType = D3DLIGHT_SPOT;
    U1(light.dcvDiffuse).r = 1.f;
    U2(light.dcvDiffuse).g = 1.f;
    U3(light.dcvDiffuse).b = 1.f;
    U3(light.dvDirection).z = 1.f;

    light.dvAttenuation0 = -one / zero; /* -INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==DDERR_INVALIDPARAMS, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = 0.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = 1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = one / zero; /* +INFINITY */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    light.dvAttenuation0 = zero / zero; /* NaN */
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    /* Directional light ignores attenuation */
    light.dltType = D3DLIGHT_DIRECTIONAL;
    light.dvAttenuation0 = -1.0;
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 103, &light);
    ok(rc==D3D_OK, "SetLight returned: %x\n", rc);

    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial returned: %x\n", rc);

    U4(mat).power = 129.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial(power = 129.0) returned: %x\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_GetMaterial returned: %x\n", rc);
    ok(U4(mat).power == 129, "Returned power is %f\n", U4(mat).power);

    U4(mat).power = -1.0;
    rc = IDirect3DDevice7_SetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetMaterial(power = -1.0) returned: %x\n", rc);
    memset(&mat, 0, sizeof(mat));
    rc = IDirect3DDevice7_GetMaterial(lpD3DDevice, &mat);
    ok(rc == D3D_OK, "IDirect3DDevice7_GetMaterial returned: %x\n", rc);
    ok(U4(mat).power == -1, "Returned power is %f\n", U4(mat).power);
}

static void ProcessVerticesTest(void)
{
    D3DVERTEXBUFFERDESC desc;
    HRESULT rc;
    VERTEX *in;
    TVERTEX *out;
    VERTEX *out2;
    D3DVIEWPORT7 vp;
    D3DMATRIX view = {  2.0, 0.0, 0.0, 0.0,
                        0.0, -1.0, 0.0, 0.0,
                        0.0, 0.0, 1.0, 0.0,
                        0.0, 0.0, 0.0, 3.0 };

    D3DMATRIX world = { 0.0, 1.0, 0.0, 0.0,
                        1.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 1.0 };

    D3DMATRIX proj = {  1.0, 0.0, 0.0, 1.0,
                        0.0, 1.0, 1.0, 0.0,
                        0.0, 1.0, 1.0, 0.0,
                        1.0, 0.0, 0.0, 1.0 };
    /* Create some vertex buffers */

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 16;
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufSrc, 0);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufSrc)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZRHW;
    desc.dwNumVertices = 16;
    /* Msdn says that the last parameter must be 0 - check that */
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufDest1, 4);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufDest1)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = 0;
    desc.dwFVF = D3DFVF_XYZ;
    desc.dwNumVertices = 16;
    /* Msdn says that the last parameter must be 0 - check that */
    rc = IDirect3D7_CreateVertexBuffer(lpD3D, &desc, &lpVBufDest2, 12345678);
    ok(rc==D3D_OK || rc==E_OUTOFMEMORY, "CreateVertexBuffer returned: %x\n", rc);
    if (!lpVBufDest2)
    {
        trace("IDirect3D7::CreateVertexBuffer() failed with an error %x\n", rc);
        goto out;
    }

    rc = IDirect3DVertexBuffer7_Lock(lpVBufSrc, 0, (void **) &in, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!in) goto out;

    /* Check basic transformation */

    in[0].x = 0.0;
    in[0].y = 0.0;
    in[0].z = 0.0;

    in[1].x = 1.0;
    in[1].y = 1.0;
    in[1].z = 1.0;

    in[2].x = -1.0;
    in[2].y = -1.0;
    in[2].z = 0.5;

    in[3].x = 0.5;
    in[3].y = -0.5;
    in[3].z = 0.25;
    rc = IDirect3DVertexBuffer7_Unlock(lpVBufSrc);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest2, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Check the results */
    ok( comparefloat(out[0].x, 128.0 ) &&
        comparefloat(out[0].y, 128.0 ) &&
        comparefloat(out[0].z, 0.0 ) &&
        comparefloat(out[0].rhw, 1.0 ),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 0.0 ) &&
        comparefloat(out[1].z, 1.0 ) &&
        comparefloat(out[1].rhw, 1.0 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 0.0 ) &&
        comparefloat(out[2].y, 256.0 ) &&
        comparefloat(out[2].z, 0.5 ) &&
        comparefloat(out[2].rhw, 1.0 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[2].x, out[2].y, out[2].z, out[2].rhw);

    ok( comparefloat(out[3].x, 192.0 ) &&
        comparefloat(out[3].y, 192.0 ) &&
        comparefloat(out[3].z, 0.25 ) &&
        comparefloat(out[3].rhw, 1.0 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest2, 0, (void **) &out2, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out2) goto out;
    /* Small thing without much practial meaning, but I stumbled upon it,
     * so let's check for it: If the output vertex buffer has to RHW value,
     * The RHW value of the last vertex is written into the next vertex
     */
    ok( comparefloat(out2[4].x, 1.0 ) &&
        comparefloat(out2[4].y, 0.0 ) &&
        comparefloat(out2[4].z, 0.0 ),
        "Output 4 vertex is (%f , %f , %f)\n", out2[4].x, out2[4].y, out2[4].z);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest2);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    /* Try a more complicated viewport, same vertices */
    memset(&vp, 0, sizeof(vp));
    vp.dwX = 10;
    vp.dwY = 5;
    vp.dwWidth = 246;
    vp.dwHeight = 130;
    vp.dvMinZ = -2.0;
    vp.dvMaxZ = 4.0;
    rc = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetViewport failed with rc=%x\n", rc);

    /* Process again */
    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Check the results */
    ok( comparefloat(out[0].x, 133.0 ) &&
        comparefloat(out[0].y, 70.0 ) &&
        comparefloat(out[0].z, -2.0 ) &&
        comparefloat(out[0].rhw, 1.0 ),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 5.0 ) &&
        comparefloat(out[1].z, 4.0 ) &&
        comparefloat(out[1].rhw, 1.0 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 10.0 ) &&
        comparefloat(out[2].y, 135.0 ) &&
        comparefloat(out[2].z, 1.0 ) &&
        comparefloat(out[2].rhw, 1.0 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[3].x, 194.5 ) &&
        comparefloat(out[3].y, 102.5 ) &&
        comparefloat(out[3].z, -0.5 ) &&
        comparefloat(out[3].rhw, 1.0 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

    /* Play with some matrices. */

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_VIEW, &view);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_PROJECTION, &proj);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DDevice7_SetTransform(lpD3DDevice, D3DTRANSFORMSTATE_WORLD, &world);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetTransform failed\n");

    rc = IDirect3DVertexBuffer7_ProcessVertices(lpVBufDest1, D3DVOP_TRANSFORM, 0, 4, lpVBufSrc, 0, lpD3DDevice, 0);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::ProcessVertices returned: %x\n", rc);

    rc = IDirect3DVertexBuffer7_Lock(lpVBufDest1, 0, (void **) &out, NULL);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Lock returned: %x\n", rc);
    if(!out) goto out;

    /* Keep the viewport simpler, otherwise we get bad numbers to compare */
    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 100;
    vp.dwHeight = 100;
    vp.dvMinZ = 1.0;
    vp.dvMaxZ = 0.0;
    rc = IDirect3DDevice7_SetViewport(lpD3DDevice, &vp);
    ok(rc==D3D_OK, "IDirect3DDevice7_SetViewport failed\n");

    /* Check the results */
    ok( comparefloat(out[0].x, 256.0 ) &&    /* X coordinate is cut at the surface edges */
        comparefloat(out[0].y, 70.0 ) &&
        comparefloat(out[0].z, -2.0 ) &&
        comparefloat(out[0].rhw, (1.0 / 3.0)),
        "Output 0 vertex is (%f , %f , %f , %f)\n", out[0].x, out[0].y, out[0].z, out[0].rhw);

    ok( comparefloat(out[1].x, 256.0 ) &&
        comparefloat(out[1].y, 78.125000 ) &&
        comparefloat(out[1].z, -2.750000 ) &&
        comparefloat(out[1].rhw, 0.125000 ),
        "Output 1 vertex is (%f , %f , %f , %f)\n", out[1].x, out[1].y, out[1].z, out[1].rhw);

    ok( comparefloat(out[2].x, 256.0 ) &&
        comparefloat(out[2].y, 44.000000 ) &&
        comparefloat(out[2].z, 0.400000 ) &&
        comparefloat(out[2].rhw, 0.400000 ),
        "Output 2 vertex is (%f , %f , %f , %f)\n", out[2].x, out[2].y, out[2].z, out[2].rhw);

    ok( comparefloat(out[3].x, 256.0 ) &&
        comparefloat(out[3].y, 81.818184 ) &&
        comparefloat(out[3].z, -3.090909 ) &&
        comparefloat(out[3].rhw, 0.363636 ),
        "Output 3 vertex is (%f , %f , %f , %f)\n", out[3].x, out[3].y, out[3].z, out[3].rhw);

    rc = IDirect3DVertexBuffer7_Unlock(lpVBufDest1);
    ok(rc==D3D_OK , "IDirect3DVertexBuffer::Unlock returned: %x\n", rc);
    out = NULL;

out:
    IDirect3DVertexBuffer7_Release(lpVBufSrc);
    IDirect3DVertexBuffer7_Release(lpVBufDest1);
    IDirect3DVertexBuffer7_Release(lpVBufDest2);
}

static void StateTest( void )
{
    HRESULT rc;

    /* The msdn says its undocumented, does it return an error too? */
    rc = IDirect3DDevice7_SetRenderState(lpD3DDevice, D3DRENDERSTATE_ZVISIBLE, TRUE);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetRenderState(D3DRENDERSTATE_ZVISIBLE, TRUE) returned %08x\n", rc);
    rc = IDirect3DDevice7_SetRenderState(lpD3DDevice, D3DRENDERSTATE_ZVISIBLE, FALSE);
    ok(rc == D3D_OK, "IDirect3DDevice7_SetRenderState(D3DRENDERSTATE_ZVISIBLE, FALSE) returned %08x\n", rc);
}


static void SceneTest(void)
{
    HRESULT                      hr;

    /* Test an EndScene without beginscene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    if(SUCCEEDED(hr))
    {
        DDBLTFX fx;
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);

        if(lpDDSdepth) {
            hr = IDirectDrawSurface7_Blt(lpDDSdepth, NULL, NULL, NULL, DDBLT_DEPTHFILL, &fx);
            ok(hr == D3D_OK, "Depthfill failed in a BeginScene / EndScene pair\n");
        } else {
            skip("Depth stencil creation failed at startup, skipping\n");
        }
        hr = IDirect3DDevice7_EndScene(lpD3DDevice);
        ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    }

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice7_BeginScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_IN_SCENE, "IDirect3DDevice7_BeginScene returned %08x\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3D_OK, "IDirect3DDevice7_EndScene failed with %08x\n", hr);
    hr = IDirect3DDevice7_EndScene(lpD3DDevice);
    ok(hr == D3DERR_SCENE_NOT_IN_SCENE, "IDirect3DDevice7_EndScene returned %08x\n", hr);

    /* TODO: Verify that blitting works in the same way as in d3d9 */
}

static void LimitTest(void)
{
    IDirectDrawSurface7 *pTexture = NULL;
    HRESULT hr;
    int i;
    DDSURFACEDESC2 ddsd;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    hr = IDirectDraw7_CreateSurface(lpDD, &ddsd, &pTexture, NULL);
    ok(hr==DD_OK,"CreateSurface returned: %x\n",hr);
    if(!pTexture) return;

    for(i = 0; i < 8; i++) {
        hr = IDirect3DDevice7_SetTexture(lpD3DDevice, i, pTexture);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice7_SetTexture(lpD3DDevice, i, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice7_SetTextureStageState(lpD3DDevice, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetTextureStageState for texture %d failed with %08x\n", i, hr);
    }

    IDirectDrawSurface7_Release(pTexture);
}

static HRESULT WINAPI enumDevicesCallback(GUID *Guid,LPSTR DeviceDescription,LPSTR DeviceName, D3DDEVICEDESC *hal, D3DDEVICEDESC *hel, VOID *ctx)
{
    UINT ver = *((UINT *) ctx);
    if(IsEqualGUID(&IID_IDirect3DRGBDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "RGB Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "RGB Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "RGB Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "RGB Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "RGB Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "RGB Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else if(IsEqualGUID(&IID_IDirect3DHALDevice, Guid))
    {
        /* pow2 is hardware dependent */

        ok(hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "HAL Device %d hal line caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "HAL Device %d hal tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "HAL Device %d hel line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "HAL Device %d hel tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else if(IsEqualGUID(&IID_IDirect3DRefDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "REF Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "REF Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "REF Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "REF Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "REF Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "REF Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else if(IsEqualGUID(&IID_IDirect3DRampDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "Ramp Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "Ramp Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "Ramp Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "Ramp Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "Ramp Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "Ramp Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else if(IsEqualGUID(&IID_IDirect3DMMXDevice, Guid))
    {
        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "MMX Device %d hal line caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) == 0,
           "MMX Device %d hal tri caps has D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "MMX Device %d hel line caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_POW2 flag set\n", ver);

        ok((hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "MMX Device %d hal line caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok((hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) == 0,
           "MMX Device %d hal tri caps has D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
        ok(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE,
           "MMX Device %d hel tri caps does not have D3DPTEXTURECAPS_PERSPECTIVE set\n", ver);
    }
    else
    {
        ok(FALSE, "Unexpected device enumerated: \"%s\" \"%s\"\n", DeviceDescription, DeviceName);
        if(hal->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hal line has pow2 set\n");
        else trace("hal line does NOT have pow2 set\n");
        if(hal->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hal tri has pow2 set\n");
        else trace("hal tri does NOT have pow2 set\n");
        if(hel->dpcLineCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hel line has pow2 set\n");
        else trace("hel line does NOT have pow2 set\n");
        if(hel->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) trace("hel tri has pow2 set\n");
        else trace("hel tri does NOT have pow2 set\n");
    }
    return DDENUMRET_OK;
}

static void CapsTest(void)
{
    IDirect3D3 *d3d3;
    IDirect3D3 *d3d2;
    IDirectDraw *dd1;
    HRESULT hr;
    UINT ver;

    hr = DirectDrawCreate(NULL, &dd1, NULL);
    ok(hr == DD_OK, "Cannot create a DirectDraw 1 interface, hr = %08x\n", hr);
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirect3D3, (void **) &d3d3);
    ok(hr == D3D_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);
    ver = 3;
    IDirect3D3_EnumDevices(d3d3, enumDevicesCallback, &ver);

    IDirect3D3_Release(d3d3);
    IDirectDraw_Release(dd1);

    hr = DirectDrawCreate(NULL, &dd1, NULL);
    ok(hr == DD_OK, "Cannot create a DirectDraw 1 interface, hr = %08x\n", hr);
    hr = IDirectDraw_QueryInterface(dd1, &IID_IDirect3D2, (void **) &d3d2);
    ok(hr == D3D_OK, "IDirectDraw_QueryInterface returned %08x\n", hr);
    ver = 2;
    IDirect3D2_EnumDevices(d3d2, enumDevicesCallback, &ver);

    IDirect3D2_Release(d3d2);
    IDirectDraw_Release(dd1);
}

struct v_in {
    float x, y, z;
};
struct v_out {
    float x, y, z, rhw;
};

static void Direct3D1Test(void)
{
    IDirect3DDevice *dev1 = NULL;
    IDirectDraw *dd;
    IDirect3D *d3d;
    IDirectDrawSurface *dds;
    IDirect3DExecuteBuffer *exebuf;
    IDirect3DViewport *vp;
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DEXECUTEBUFFERDESC desc;
    D3DVIEWPORT vp_data;
    D3DINSTRUCTION *instr;
    D3DBRANCH *branch;
    unsigned int idx = 0;
    static struct v_in testverts[] = {
        {0.0, 0.0, 0.0},  { 1.0,  1.0,  1.0}, {-1.0, -1.0, -1.0},
        {0.5, 0.5, 0.5},  {-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.0},
    };
    static struct v_in cliptest[] = {
        {25.59, 25.59, 1.0},  {-25.59, -25.59,  0.0},
        {25.61, 25.61, 1.01}, {-25.60, -25.60, -0.01},
    };
    static struct v_in offscreentest[] = {
        {128.1, 0.0, 0.0},
    };
    struct v_out out[sizeof(testverts) / sizeof(testverts[0])];
    D3DHVERTEX outH[sizeof(testverts) / sizeof(testverts[0])];
    D3DTRANSFORMDATA transformdata;
    DWORD i = FALSE;

    /* An IDirect3DDevice cannot be queryInterfaced from an IDirect3DDevice7 on windows */
    hr = DirectDrawCreate(NULL, &dd, NULL);
    ok(hr==DD_OK || hr==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreate returned: %x\n", hr);
    if (!dd) {
        trace("DirectDrawCreate() failed with an error %x\n", hr);
        return;
    }

    hr = IDirectDraw_SetCooperativeLevel(dd, NULL, DDSCL_NORMAL);
    ok(hr==DD_OK, "SetCooperativeLevel returned: %x\n", hr);

    hr = IDirectDraw_QueryInterface(dd, &IID_IDirect3D, (void**) &d3d);
    ok(hr==DD_OK, "QueryInterface returned: %x\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    hr = IDirectDraw_CreateSurface(dd, &ddsd, &dds, NULL);
    ok(hr==DD_OK, "CreateSurface returned: %x\n", hr);

    dev1 = NULL;
    hr = IDirectDrawSurface_QueryInterface(dds, &IID_IDirect3DRGBDevice, (void **) &dev1);
    ok(hr==D3D_OK || hr==DDERR_NOPALETTEATTACHED || hr==E_OUTOFMEMORY, "CreateDevice returned: %x\n", hr);
    if(!dev1) return;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    desc.dwCaps = D3DDEBCAPS_VIDEOMEMORY;
    desc.dwBufferSize = 128;
    desc.lpData = NULL;
    hr = IDirect3DDevice_CreateExecuteBuffer(dev1, &desc, &exebuf, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice_CreateExecuteBuffer failed: %08x\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);

    hr = IDirect3DExecuteBuffer_Lock(exebuf, &desc);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Lock failed: %08x\n", hr);
    instr = desc.lpData;
    instr[idx].bOpcode = D3DOP_BRANCHFORWARD;
    instr[idx].bSize = sizeof(*branch);
    instr[idx].wCount = 1;
    idx++;
    branch = (D3DBRANCH *) &instr[idx];
    branch->dwMask = 0x0;
    branch->dwValue = 1;
    branch->bNegate = TRUE;
    branch->dwOffset = 0;
    idx += (sizeof(*branch) / sizeof(*instr));
    instr[idx].bOpcode = D3DOP_EXIT;
    instr[idx].bSize = 0;
    instr[idx].bSize = 0;
    hr = IDirect3DExecuteBuffer_Unlock(exebuf);
    ok(hr == D3D_OK, "IDirect3DExecuteBuffer_Unlock failed: %08x\n", hr);

    hr = IDirect3D_CreateViewport(d3d, &vp, NULL);
    ok(hr == D3D_OK, "IDirect3D_CreateViewport failed: %08x\n", hr);
    hr = IDirect3DViewport_Initialize(vp, d3d);
    ok(hr == DDERR_ALREADYINITIALIZED, "IDirect3DViewport_Initialize returned %08x\n", hr);

    hr = IDirect3DDevice_AddViewport(dev1, vp);
    ok(hr == D3D_OK, "IDirect3DDevice_AddViewport returned %08x\n", hr);
    vp_data.dwSize = sizeof(vp_data);
    vp_data.dwX = 0;
    vp_data.dwY = 0;
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    vp_data.dvMaxX = 256;
    vp_data.dvMaxY = 256;
    vp_data.dvMinZ = 0;
    vp_data.dvMaxZ = 1;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);

    hr = IDirect3DDevice_Execute(dev1, exebuf, vp, D3DEXECUTE_CLIPPED);
    ok(hr == D3D_OK, "IDirect3DDevice_Execute returned %08x\n", hr);

    memset(&transformdata, 0, sizeof(transformdata));
    transformdata.dwSize = sizeof(transformdata);
    transformdata.lpIn = (void *) testverts;
    transformdata.dwInSize = sizeof(testverts[0]);
    transformdata.lpOut = out;
    transformdata.dwOutSize = sizeof(out[0]);

    transformdata.lpHOut = NULL;
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, D3DTRANSFORM_UNCLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);

    transformdata.lpHOut = outH;
    memset(outH, 0xaa, sizeof(outH));
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, D3DTRANSFORM_UNCLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);

    for(i = 0; i < sizeof(testverts) / sizeof(testverts[0]); i++) {
        static const struct v_out cmp[] = {
            {128.0, 128.0, 0.0, 1}, {129.0, 127.0,  1.0, 1}, {127.0, 129.0, -1, 1},
            {128.5, 127.5, 0.5, 1}, {127.5, 128.5, -0.5, 1}, {127.5, 128.5,  0, 1}
        };

        ok(cmp[i].x == out[i].x && cmp[i].y == out[i].y &&
           cmp[i].z == out[i].z && cmp[i].rhw == out[i].rhw,
           "Vertex %d differs. Got %f %f %f %f, expexted %f %f %f %f\n", i + 1,
           out[i].x, out[i].y, out[i].z, out[i].rhw,
           cmp[i].x, cmp[i].y, cmp[i].z, cmp[i].rhw);
    }
    for(i = 0; i < sizeof(outH); i++) {
        if(((unsigned char *) outH)[i] != 0xaa) {
            ok(FALSE, "Homogenous output was generated despite UNCLIPPED flag\n");
            break;
        }
    }

    vp_data.dvScaleX = 5;
    vp_data.dvScaleY = 5;
    vp_data.dvMinZ = -25;
    vp_data.dvMaxZ = 60;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, D3DTRANSFORM_UNCLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    for(i = 0; i < sizeof(testverts) / sizeof(testverts[0]); i++) {
        static const struct v_out cmp[] = {
            {128.0, 128.0, 0.0, 1}, {133.0, 123.0,  1.0, 1}, {123.0, 133.0, -1, 1},
            {130.5, 125.5, 0.5, 1}, {125.5, 130.5, -0.5, 1}, {125.5, 130.5,  0, 1}
        };
        ok(cmp[i].x == out[i].x && cmp[i].y == out[i].y &&
           cmp[i].z == out[i].z && cmp[i].rhw == out[i].rhw,
           "Vertex %d differs. Got %f %f %f %f, expexted %f %f %f %f\n", i + 1,
           out[i].x, out[i].y, out[i].z, out[i].rhw,
           cmp[i].x, cmp[i].y, cmp[i].z, cmp[i].rhw);
    }

    vp_data.dwX = 10;
    vp_data.dwY = 20;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, D3DTRANSFORM_UNCLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    for(i = 0; i < sizeof(testverts) / sizeof(testverts[0]); i++) {
        static const struct v_out cmp[] = {
            {138.0, 148.0, 0.0, 1}, {143.0, 143.0,  1.0, 1}, {133.0, 153.0, -1, 1},
            {140.5, 145.5, 0.5, 1}, {135.5, 150.5, -0.5, 1}, {135.5, 150.5,  0, 1}
        };
        ok(cmp[i].x == out[i].x && cmp[i].y == out[i].y &&
           cmp[i].z == out[i].z && cmp[i].rhw == out[i].rhw,
           "Vertex %d differs. Got %f %f %f %f, expexted %f %f %f %f\n", i + 1,
           out[i].x, out[i].y, out[i].z, out[i].rhw,
           cmp[i].x, cmp[i].y, cmp[i].z, cmp[i].rhw);
    }

    memset(out, 0xbb, sizeof(out));
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    for(i = 0; i < sizeof(testverts) / sizeof(testverts[0]); i++) {
        static const D3DHVERTEX cmpH[] = {
            {0,             { 0.0}, { 0.0}, { 0.0}}, {0, { 1.0}, { 1.0}, {1.0}},
            {D3DCLIP_FRONT, {-1.0}, {-1.0}, {-1.0}}, {0, { 0.5}, { 0.5}, {0.5}},
            {D3DCLIP_FRONT, {-0.5}, {-0.5}, {-0.5}}, {0, {-0.5}, {-0.5}, {0.0}}
        };
        ok(cmpH[i].hx == outH[i].hx && cmpH[i].hy == outH[i].hy &&
           cmpH[i].hz == outH[i].hz && cmpH[i].dwFlags == outH[i].dwFlags,
           "HVertex %d differs. Got %08x %f %f %f, expexted %08x %f %f %f\n", i + 1,
           outH[i].dwFlags, outH[i].hx, outH[i].hy, outH[i].hz,
           cmpH[i].dwFlags, cmpH[i].hx, cmpH[i].hy, cmpH[i].hz);

        /* No scheme has been found behind those return values. It seems to be
         * whatever data windows has when throwing the vertex away. Modify the
         * input test vertices to test this more. Depending on the input data
         * it can happen that the z coord gets written into y, or simmilar things
         */
        if(0)
        {
            static const struct v_out cmp[] = {
                {138.0, 148.0, 0.0, 1}, {143.0, 143.0,  1.0, 1}, { -1.0,  -1.0, 0.5, 1},
                {140.5, 145.5, 0.5, 1}, { -0.5,  -0.5, -0.5, 1}, {135.5, 150.5, 0.0, 1}
            };
            ok(cmp[i].x == out[i].x && cmp[i].y == out[i].y &&
               cmp[i].z == out[i].z && cmp[i].rhw == out[i].rhw,
                "Vertex %d differs. Got %f %f %f %f, expexted %f %f %f %f\n", i + 1,
               out[i].x, out[i].y, out[i].z, out[i].rhw,
               cmp[i].x, cmp[i].y, cmp[i].z, cmp[i].rhw);
        }
    }
    for(i = 0; i < sizeof(out) / sizeof(DWORD); i++) {
        ok(((DWORD *) out)[i] != 0xbbbbbbbb,
                "Regular output DWORD %d remained untouched\n", i);
    }

    transformdata.lpIn = (void *) cliptest;
    transformdata.dwInSize = sizeof(cliptest[0]);
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(cliptest) / sizeof(cliptest[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    for(i = 0; i < sizeof(cliptest) / sizeof(cliptest[0]); i++) {
        DWORD Flags[sizeof(cliptest) / sizeof(cliptest[0])] =
        {
            0,
            0,
            D3DCLIP_RIGHT | D3DCLIP_BACK   | D3DCLIP_TOP,
            D3DCLIP_LEFT  | D3DCLIP_BOTTOM | D3DCLIP_FRONT,
        };
        ok(Flags[i] == outH[i].dwFlags,
           "Cliptest %d differs. Got %08x expexted %08x\n", i + 1,
           outH[i].dwFlags, Flags[i]);
    }

    vp_data.dwWidth = 10;
    vp_data.dwHeight = 1000;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    i = 10;
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(cliptest) / sizeof(cliptest[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    for(i = 0; i < sizeof(cliptest) / sizeof(cliptest[0]); i++) {
        DWORD Flags[sizeof(cliptest) / sizeof(cliptest[0])] =
        {
            D3DCLIP_RIGHT,
            D3DCLIP_LEFT,
            D3DCLIP_RIGHT | D3DCLIP_BACK,
            D3DCLIP_LEFT  | D3DCLIP_FRONT,
        };
        ok(Flags[i] == outH[i].dwFlags,
           "Cliptest %d differs. Got %08x expexted %08x\n", i + 1,
           outH[i].dwFlags, Flags[i]);
    }
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(cliptest) / sizeof(cliptest[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %s\n", i ? "TRUE" : "FALSE");
    for(i = 0; i < sizeof(cliptest) / sizeof(cliptest[0]); i++) {
        DWORD Flags[sizeof(cliptest) / sizeof(cliptest[0])] =
        {
            0,
            0,
            D3DCLIP_BACK,
            D3DCLIP_FRONT,
        };
        ok(Flags[i] == outH[i].dwFlags,
           "Cliptest %d differs. Got %08x expexted %08x\n", i + 1,
           outH[i].dwFlags, Flags[i]);
    }

    /* Finally try to figure out how the DWORD dwOffscreen works.
     * Apparently no vertex is offscreen with clipping off,
     * and with clipping on the offscreen flag is set if only one vertex
     * is transformed, and this vertex is offscreen.
     */
    vp_data.dwWidth = 5;
    vp_data.dwHeight = 5;
    vp_data.dvScaleX = 10000;
    vp_data.dvScaleY = 10000;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    ok(hr == D3D_OK, "IDirect3DViewport_SetViewport returned %08x\n", hr);
    transformdata.lpIn = cliptest;
    hr = IDirect3DViewport_TransformVertices(vp, 1,
                                             &transformdata, D3DTRANSFORM_UNCLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    hr = IDirect3DViewport_TransformVertices(vp, 1,
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == (D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %d\n", i);
    hr = IDirect3DViewport_TransformVertices(vp, 2,
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    transformdata.lpIn = cliptest + 1;
    hr = IDirect3DViewport_TransformVertices(vp, 1,
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == (D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %d\n", i);

    transformdata.lpIn = (void *) offscreentest;
    transformdata.dwInSize = sizeof(offscreentest[0]);
    vp_data.dwWidth = 257;
    vp_data.dwHeight = 257;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    i = 12345;
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(offscreentest) / sizeof(offscreentest[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == 0, "Offscreen is %d\n", i);
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    hr = IDirect3DViewport_SetViewport(vp, &vp_data);
    i = 12345;
    hr = IDirect3DViewport_TransformVertices(vp, sizeof(offscreentest) / sizeof(offscreentest[0]),
                                             &transformdata, D3DTRANSFORM_CLIPPED,
                                             &i);
    ok(hr == D3D_OK, "IDirect3DViewport_TransformVertices returned %08x\n", hr);
    ok(i == D3DCLIP_RIGHT, "Offscreen is %d\n", i);

    hr = IDirect3DViewport_TransformVertices(vp, sizeof(testverts) / sizeof(testverts[0]),
                                             &transformdata, 0,
                                             &i);
    ok(hr == DDERR_INVALIDPARAMS, "IDirect3DViewport_TransformVertices returned %08x\n", hr);

    hr = IDirect3DDevice_DeleteViewport(dev1, vp);
    ok(hr == D3D_OK, "IDirect3DDevice_DeleteViewport returned %08x\n", hr);

    IDirect3DViewport_Release(vp);
    IDirect3DExecuteBuffer_Release(exebuf);
    IDirect3DDevice_Release(dev1);
    IDirectDrawSurface_Release(dds);
    IDirect3D_Release(d3d);
    IDirectDraw_Release(dd);
    return;
}

START_TEST(d3d)
{
    init_function_pointers();
    if(!pDirectDrawCreateEx) {
        skip("function DirectDrawCreateEx not available\n");
        return;
    }

    if(!CreateDirect3D()) {
        trace("Skipping tests\n");
        return;
    }
    LightTest();
    ProcessVerticesTest();
    StateTest();
    SceneTest();
    LimitTest();
    CapsTest();
    ReleaseDirect3D();
    Direct3D1Test();
}
