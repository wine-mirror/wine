/*
 * Some unit tests for d3d functions
 *
 * Copyright (C) 2005 Antoine Chavasse
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

static LPDIRECTDRAW7        lpDD = NULL;
static LPDIRECT3D7          lpD3D = NULL;
static LPDIRECTDRAWSURFACE7 lpDDS = NULL;
static LPDIRECT3DDEVICE7    lpD3DDevice = NULL;

static HRESULT (WINAPI *pDirectDrawCreateEx)(LPGUID,LPVOID*,REFIID,LPUNKNOWN);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ddraw.dll");

    if(hmod)
    {
        pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    }
}


static BOOL CreateDirect3D(void)
{
    HRESULT rc;
    DDSURFACEDESC2 ddsd;

    rc = pDirectDrawCreateEx(NULL, (void**)&lpDD,
        &IID_IDirectDraw7, NULL);
    ok(rc==DD_OK || rc==DDERR_NODIRECTDRAWSUPPORT, "DirectDrawCreateEx returned: %lx\n", rc);
    if (!lpDD) {
        trace("DirectDrawCreateEx() failed with an error %lx\n", rc);
        return FALSE;
    }

    rc = IDirectDraw_SetCooperativeLevel(lpDD, NULL, DDSCL_NORMAL);
    ok(rc==DD_OK, "SetCooperativeLevel returned: %lx\n", rc);

    rc = IDirectDraw7_QueryInterface(lpDD, &IID_IDirect3D7, (void**) &lpD3D);
    if (rc == E_NOINTERFACE) return FALSE;
    ok(rc==DD_OK, "QueryInterface returned: %lx\n", rc);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    ddsd.dwWidth = 256;
    ddsd.dwHeight = 256;
    rc = IDirectDraw7_CreateSurface(lpDD, &ddsd, &lpDDS, NULL);
    ok(rc==DD_OK, "CreateSurface returned: %lx\n", rc);

    rc = IDirect3D7_CreateDevice(lpD3D, &IID_IDirect3DTnLHalDevice, lpDDS,
        &lpD3DDevice);
    ok(rc==D3D_OK || rc==DDERR_NOPALETTEATTACHED || rc==E_OUTOFMEMORY, "CreateDevice returned: %lx\n", rc);
    if (!lpD3DDevice) {
        trace("IDirect3D7::CreateDevice() failed with an error %lx\n", rc);
        return FALSE;
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

    /* Set a few lights with funky indices. */
    memset(&light, 0, sizeof(light));
    light.dltType = D3DLIGHT_DIRECTIONAL;
    U1(light.dcvDiffuse).r = 0.5f;
    U2(light.dcvDiffuse).g = 0.6f;
    U3(light.dcvDiffuse).b = 0.7f;
    U2(light.dvDirection).y = 1.f;

    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 5, &light);
    ok(rc==D3D_OK, "SetLight returned: %lx\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "SetLight returned: %lx\n", rc);
    rc = IDirect3DDevice7_SetLight(lpD3DDevice, 45, &light);
    ok(rc==D3D_OK, "SetLight returned: %lx\n", rc);


    /* Try to retrieve a light beyond the indices of the lights that have
       been set. */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %lx\n", rc);
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 2, &light);
    ok(rc==DDERR_INVALIDPARAMS, "GetLight returned: %lx\n", rc);


    /* Try to retrieve one of the lights that have been set */
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 10, &light);
    ok(rc==D3D_OK, "GetLight returned: %lx\n", rc);


    /* Enable a light that have been previously set. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 10, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %lx\n", rc);


    /* Enable some lights that have not been previously set, and verify that
       they have been initialized with proper default values. */
    memset(&defaultlight, 0, sizeof(D3DLIGHT7));
    defaultlight.dltType = D3DLIGHT_DIRECTIONAL;
    U1(defaultlight.dcvDiffuse).r = 1.f;
    U2(defaultlight.dcvDiffuse).g = 1.f;
    U3(defaultlight.dcvDiffuse).b = 1.f;
    U3(defaultlight.dvDirection).z = 1.f;

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %lx\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 20, &light);
    ok(rc==D3D_OK, "GetLight returned: %lx\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );

    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 50, TRUE);
    ok(rc==D3D_OK, "LightEnable returned: %lx\n", rc);
    memset(&light, 0, sizeof(D3DLIGHT7));
    rc = IDirect3DDevice7_GetLight(lpD3DDevice, 50, &light);
    ok(rc==D3D_OK, "GetLight returned: %lx\n", rc);
    ok(!memcmp(&light, &defaultlight, sizeof(D3DLIGHT7)),
        "light data doesn't match expected default values\n" );


    /* Disable one of the light that have been previously enabled. */
    rc = IDirect3DDevice7_LightEnable(lpD3DDevice, 20, FALSE);
    ok(rc==D3D_OK, "LightEnable returned: %lx\n", rc);

    /* Try to retrieve the enable status of some lights */
    /* Light 20 is supposed to be disabled */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 20, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %lx\n", rc);
    ok(!bEnabled, "GetLightEnable says the light is enabled\n");

    /* Light 10 is supposed to be enabled */
    bEnabled = FALSE;
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 10, &bEnabled );
    ok(rc==D3D_OK, "GetLightEnable returned: %lx\n", rc);
    ok(bEnabled, "GetLightEnable says the light is disabled\n");

    /* Light 80 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 80, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %lx\n", rc);

    /* Light 23 has not been set */
    rc = IDirect3DDevice7_GetLightEnable(lpD3DDevice, 23, &bEnabled );
    ok(rc==DDERR_INVALIDPARAMS, "GetLightEnable returned: %lx\n", rc);
}

START_TEST(d3d)
{
    init_function_pointers();
    if(!pDirectDrawCreateEx) {
        trace("function DirectDrawCreateEx not available, skipping tests\n");
        return;
    }

    if(!CreateDirect3D()) {
        trace("Skipping tests\n");
        return;
	}
    LightTest();
    ReleaseDirect3D();
}
