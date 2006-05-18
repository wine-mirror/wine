/*	DirectDraw HAL driver
 *
 * Copyright 2001 TransGaming Technologies Inc.
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

#include "config.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "ddrawi.h"
#include "d3dhal.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const IDirectDraw7Vtbl HAL_DirectDraw_VTable;

static DDVERSIONDATA hal_version;
static DD32BITDRIVERDATA hal_driverdata;
static HINSTANCE hal_instance;

static const DDDEVICEIDENTIFIER2 hal_device =
{
    "display",
    "DirectDraw HAL",
    { { 0x00010001, 0x00010001 } },
    0, 0, 0, 0,
    /* 40c1b248-9d7d-4a29-b7d7-4cd8109f3d5d */
    {0x40c1b248,0x9d7d,0x4a29,{0xd7,0xb7,0x4c,0xd8,0x10,0x9f,0x3d,0x5d}},
    0
};

HRESULT HAL_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			      IUnknown* pUnkOuter, BOOL ex);
HRESULT HAL_DirectDraw_Initialize(IDirectDrawImpl*, const GUID*);

static const ddraw_driver hal_driver =
{
    &hal_device,
    100, /* we prefer the HAL */
    HAL_DirectDraw_Create,
    HAL_DirectDraw_Initialize
};

static DDHAL_CALLBACKS dd_cbs;
static DDRAWI_DIRECTDRAW_GBL dd_gbl;

static D3DHAL_GLOBALDRIVERDATA d3d_hal_data;
static D3DHAL_D3DEXTENDEDCAPS d3d_hal_extcaps;
static D3DHAL_CALLBACKS d3d_hal_cbs1;
static D3DHAL_CALLBACKS2 d3d_hal_cbs2;

/* in real windoze, these entry points are 16-bit, but we can work in 32-bit */
static BOOL WINAPI set_hal_info(LPDDHALINFO lpDDHalInfo, BOOL reset)
{
    dd_cbs.HALDD	= *lpDDHalInfo->lpDDCallbacks;
    dd_cbs.HALDDSurface	= *lpDDHalInfo->lpDDSurfaceCallbacks;
    dd_cbs.HALDDPalette	= *lpDDHalInfo->lpDDPaletteCallbacks;
    if (lpDDHalInfo->lpDDExeBufCallbacks)
	dd_cbs.HALDDExeBuf	= *lpDDHalInfo->lpDDExeBufCallbacks;

    dd_gbl.lpDDCBtmp = &dd_cbs;

    dd_gbl.ddCaps		 = lpDDHalInfo->ddCaps;
    dd_gbl.dwMonitorFrequency	 = lpDDHalInfo->dwMonitorFrequency;
    dd_gbl.vmiData		 = lpDDHalInfo->vmiData;
    dd_gbl.dwModeIndex		 = lpDDHalInfo->dwModeIndex;
    dd_gbl.dwNumFourCC	         = lpDDHalInfo->ddCaps.dwNumFourCCCodes;
    dd_gbl.lpdwFourCC		 = lpDDHalInfo->lpdwFourCC;
    dd_gbl.dwNumModes		 = lpDDHalInfo->dwNumModes;
    dd_gbl.lpModeInfo		 = lpDDHalInfo->lpModeInfo;
    /* FIXME: dwFlags */
    dd_gbl.dwPDevice		 = (DWORD)lpDDHalInfo->lpPDevice;
    dd_gbl.hInstance		 = lpDDHalInfo->hInstance;
    /* DirectX 2 */
    if (lpDDHalInfo->lpD3DGlobalDriverData)
	memcpy(&d3d_hal_data, (LPVOID)lpDDHalInfo->lpD3DGlobalDriverData, sizeof(D3DDEVICEDESC_V1));
    else
	memset(&d3d_hal_data, 0, sizeof(D3DDEVICEDESC_V1));
    dd_gbl.lpD3DGlobalDriverData = (ULONG_PTR)&d3d_hal_data;

    if (lpDDHalInfo->lpD3DHALCallbacks)
	memcpy(&d3d_hal_cbs1, (LPVOID)lpDDHalInfo->lpD3DHALCallbacks, sizeof(D3DHAL_CALLBACKS));
    else
	memset(&d3d_hal_cbs1, 0, sizeof(D3DHAL_CALLBACKS));
    dd_gbl.lpD3DHALCallbacks	 = (ULONG_PTR)&d3d_hal_cbs1;

    if (lpDDHalInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET) {
	DDHAL_GETDRIVERINFODATA data;
	data.dwSize = sizeof(DDHAL_GETDRIVERINFODATA);
	data.dwFlags = 0; /* ? */
	data.dwContext = hal_driverdata.dwContext; /* ? */

	data.guidInfo = GUID_D3DExtendedCaps;
	data.dwExpectedSize = sizeof(D3DHAL_D3DEXTENDEDCAPS);
	data.lpvData = &d3d_hal_extcaps;
	data.dwActualSize = 0;
	data.ddRVal = 0;
	lpDDHalInfo->GetDriverInfo(&data);
	d3d_hal_extcaps.dwSize = data.dwActualSize;
	dd_gbl.lpD3DExtendedCaps = (ULONG_PTR)&d3d_hal_extcaps;

	data.guidInfo = GUID_D3DCallbacks2;
	data.dwExpectedSize = sizeof(D3DHAL_CALLBACKS2);
	data.lpvData = &d3d_hal_cbs2;
	data.dwActualSize = 0;
	data.ddRVal = 0;
	lpDDHalInfo->GetDriverInfo(&data);
	d3d_hal_cbs2.dwSize = data.dwActualSize;
	dd_gbl.lpD3DHALCallbacks2 = (ULONG_PTR)&d3d_hal_cbs2;
    }

    if( opengl_initialized && 
           (d3d_hal_data.hwCaps.dwFlags & D3DDD_WINE_OPENGL_DEVICE) ) {
        /*GL_DirectDraw_Init(&dd_gbl);*/
    }

    return FALSE;
}

static DDHALDDRAWFNS hal_funcs = {
    sizeof(DDHALDDRAWFNS),
    set_hal_info,
    NULL, /* VidMemAlloc */
    NULL  /* VidMemFree */
};

/* Called from DllInit, which is synchronised so there are no threading
 * concerns. */
static BOOL initialize(void)
{
    DCICMD cmd;
    INT ncmd = DCICOMMAND;
    BOOL ret;
    HDC dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    INT ver = Escape(dc, QUERYESCSUPPORT, sizeof(ncmd), (LPVOID)&ncmd, NULL);
    if (ver != DD_HAL_VERSION) {
	DeleteDC(dc);
	TRACE("DirectDraw HAL not available\n");
	return FALSE;
    }
    cmd.dwVersion = DD_VERSION;
    cmd.dwReserved = 0;

    /* the DDNEWCALLBACKFNS is supposed to give the 16-bit driver entry points
     * in ddraw16.dll, but since Wine doesn't have or use 16-bit display drivers,
     * we'll just work in 32-bit, who'll notice... */
    cmd.dwCommand = DDNEWCALLBACKFNS;
    cmd.dwParam1 = (DWORD)&hal_funcs;
    ExtEscape(dc, DCICOMMAND, sizeof(cmd), (LPVOID)&cmd, 0, NULL);

    /* next, exchange version information */
    cmd.dwCommand = DDVERSIONINFO;
    cmd.dwParam1 = DD_RUNTIME_VERSION; /* not sure what should *really* go here */
    ExtEscape(dc, DCICOMMAND, sizeof(cmd), (LPVOID)&cmd, sizeof(hal_version), (LPVOID)&hal_version);

    /* get 32-bit driver data (dll name and entry point) */
    cmd.dwCommand = DDGET32BITDRIVERNAME;
    ExtEscape(dc, DCICOMMAND, sizeof(cmd), (LPVOID)&cmd, sizeof(hal_driverdata), (LPVOID)&hal_driverdata);
    /* we're supposed to load the DLL in hal_driverdata.szName, then GetProcAddress
     * the hal_driverdata.szEntryPoint, and call it with hal_driverdata.dwContext
     * as a parameter... but since this is only more remains from the 16-bit world,
     * we'll ignore it */

    /* finally, initialize the driver object */
    cmd.dwCommand = DDCREATEDRIVEROBJECT;
    ret = ExtEscape(dc, DCICOMMAND, sizeof(cmd), (LPVOID)&cmd, sizeof(hal_instance), (LPVOID)&hal_instance);
    if (ret) {
	/* the driver should have called our set_hal_info now */
	if (!dd_gbl.lpDDCBtmp) ret = FALSE;
    }

    /* init done */
    DeleteDC(dc);

    TRACE("%s DirectDraw HAL\n", ret ? "enabling" : "disabling");

    return ret;
}

static void cleanup(void)
{
    DDHAL_DESTROYDRIVERDATA data;

    if (!dd_cbs.HALDD.DestroyDriver) return;

    data.lpDD = NULL;
    data.ddRVal = 0;
    data.DestroyDriver = dd_cbs.HALDD.DestroyDriver;
    data.DestroyDriver(&data);
}

static DWORD choose_mode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP,
			 DWORD dwRefreshRate, DWORD dwFlags)
{
    int best = -1;
    unsigned int i;

    if (!dd_gbl.dwNumModes) return 0;

/* let's support HALs that cannot switch depths (XVidMode),
 * these should return dwBPP == 0 for all their resolutions */
#define BPP_MATCH(dd, bpp) ((!(dd)) || ((dd) == bpp))

/* FIXME: we should try to match the refresh rate too */

    /* Choose the smallest mode that is large enough. */
    for (i=0; i < dd_gbl.dwNumModes; i++)
    {
	if (dd_gbl.lpModeInfo[i].dwWidth >= dwWidth &&
	    dd_gbl.lpModeInfo[i].dwHeight >= dwHeight &&
	    BPP_MATCH(dd_gbl.lpModeInfo[i].dwBPP, dwBPP))
	{
	    if (best == -1) best = i;
	    else
	    {
		if (dd_gbl.lpModeInfo[i].dwWidth < dd_gbl.lpModeInfo[best].dwWidth ||
		    dd_gbl.lpModeInfo[i].dwHeight < dd_gbl.lpModeInfo[best].dwHeight)
		    best = i;
	    }
	}
    }

    if (best == -1)
    {
	TRACE("all modes too small\n");
	/* ok, let's use the largest */

	for (i=0; i < dd_gbl.dwNumModes; i++)
	{
	    if (BPP_MATCH(dd_gbl.lpModeInfo[i].dwBPP, dwBPP))
	    {
		if (best == -1) best = i;
		else
		{
		    if (dd_gbl.lpModeInfo[i].dwWidth > dd_gbl.lpModeInfo[best].dwWidth ||
			dd_gbl.lpModeInfo[i].dwHeight > dd_gbl.lpModeInfo[best].dwHeight)
			best = i;
		}
	    }
	}
    }
#undef BPP_MATCH

    if (best == -1)
    {
	ERR("requested color depth (%ld) not available, try reconfiguring X server\n", dwBPP);
	return dd_gbl.dwModeIndex;
    }

    TRACE("using mode %d\n", best);

    return best;
}

static HRESULT set_mode(IDirectDrawImpl *This, DWORD dwMode)
{
    HRESULT hr = DD_OK;

    if (dwMode != dd_gbl.dwModeIndex)
    {
	DDHAL_SETMODEDATA data;
	data.lpDD = &dd_gbl;
	data.dwModeIndex = dwMode;
	data.ddRVal = 0;
	data.SetMode = dd_cbs.HALDD.SetMode;
	data.inexcl = 0;
	data.useRefreshRate = FALSE;
	if (data.SetMode)
	    data.SetMode(&data);
	hr = data.ddRVal;
	if (SUCCEEDED(hr))
	    dd_gbl.dwModeIndex = dwMode;
    }
    return hr;
}

static HRESULT set_exclusive_mode(IDirectDrawImpl *This, DWORD dwEnterExcl)
{
    DDHAL_SETEXCLUSIVEMODEDATA data;

    data.lpDD = &dd_gbl;
    data.dwEnterExcl = dwEnterExcl;
    data.dwReserved = 0;
    data.ddRVal = 0;
    data.SetExclusiveMode = dd_cbs.HALDD.SetExclusiveMode;
    if (data.SetExclusiveMode)
	data.SetExclusiveMode(&data);
    return data.ddRVal;
}

BOOL DDRAW_HAL_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
	if (initialize())
	    DDRAW_register_driver(&hal_driver);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
	cleanup();
    }

    return TRUE;
}

/* Not called from the vtable. */
HRESULT HAL_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    HRESULT hr;

    TRACE("(%p,%d)\n", This, ex);

    hr = User_DirectDraw_Construct(This, ex);
    if (FAILED(hr)) return hr;

    This->local.lpGbl = &dd_gbl;

    This->final_release = HAL_DirectDraw_final_release;
    This->set_exclusive_mode = set_exclusive_mode;

    This->create_palette = HAL_DirectDrawPalette_Create;

    This->create_primary    = HAL_DirectDraw_create_primary;
    This->create_backbuffer = HAL_DirectDraw_create_backbuffer;
    This->create_texture    = HAL_DirectDraw_create_texture;

    ICOM_INIT_INTERFACE(This, IDirectDraw7, HAL_DirectDraw_VTable);

    /* merge HAL caps */
    This->caps.dwCaps |= dd_gbl.ddCaps.dwCaps;
    This->caps.dwCaps2 |= dd_gbl.ddCaps.dwCaps2;
    This->caps.dwCKeyCaps |= dd_gbl.ddCaps.dwCKeyCaps;
    This->caps.dwFXCaps |= dd_gbl.ddCaps.dwFXCaps;
    This->caps.dwPalCaps |= dd_gbl.ddCaps.dwPalCaps;
    /* FIXME: merge more caps */
    This->caps.ddsCaps.dwCaps |= dd_gbl.ddCaps.ddsCaps.dwCaps;
    This->caps.ddsCaps.dwCaps2 |= dd_gbl.ddsCapsMore.dwCaps2;
    This->caps.ddsCaps.dwCaps3 |= dd_gbl.ddsCapsMore.dwCaps3;
    This->caps.ddsCaps.dwCaps4 |= dd_gbl.ddsCapsMore.dwCaps4;
    This->caps.ddsOldCaps.dwCaps = This->caps.ddsCaps.dwCaps;

    return S_OK;
}

/* This function is called from DirectDrawCreate(Ex) on the most-derived
 * class to start construction.
 * Not called from the vtable. */
HRESULT HAL_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
			      IUnknown* pUnkOuter, BOOL ex)
{
    HRESULT hr;
    IDirectDrawImpl* This;

    TRACE("\n");

    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawImpl)
		     + sizeof(HAL_DirectDrawImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    /* Note that this relation does *not* hold true if the DD object was
     * CoCreateInstanced then Initialized. */
    This->private = (HAL_DirectDrawImpl *)(This+1);

    /* Initialize the DDCAPS structure */
    This->caps.dwSize = sizeof(This->caps);

    hr = HAL_DirectDraw_Construct(This, ex);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return hr;
}

/* This function is called from Uninit_DirectDraw_Initialize on the
 * most-derived-class to start initialization.
 * Not called from the vtable. */
HRESULT HAL_DirectDraw_Initialize(IDirectDrawImpl *This, const GUID* guid)
{
    HRESULT hr;

    TRACE("\n");

    This->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      sizeof(HAL_DirectDrawImpl));
    if (This->private == NULL) return E_OUTOFMEMORY;

    /* Initialize the DDCAPS structure */
    This->caps.dwSize = sizeof(This->caps);

    hr = HAL_DirectDraw_Construct(This, TRUE); /* XXX ex? */
    if (FAILED(hr))
    {
	HeapFree(GetProcessHeap(), 0, This->private);
	return hr;
    }

    return DD_OK;
}

/* Called from an internal function pointer. */
void HAL_DirectDraw_final_release(IDirectDrawImpl *This)
{
    if (dd_gbl.dwFlags & DDRAWI_MODECHANGED) set_mode(This, dd_gbl.dwModeIndexOrig);
    User_DirectDraw_final_release(This);
}

HRESULT HAL_DirectDraw_create_primary(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      IUnknown* pUnkOuter)
{
    if (This->cooperative_level & DDSCL_EXCLUSIVE)
	return HAL_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
    else
	return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

HRESULT HAL_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					 const DDSURFACEDESC2* pDDSD,
					 LPDIRECTDRAWSURFACE7* ppSurf,
					 IUnknown* pUnkOuter,
					 IDirectDrawSurfaceImpl* primary)
{
    if (This->cooperative_level & DDSCL_EXCLUSIVE)
	return HAL_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
    else
	return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

HRESULT HAL_DirectDraw_create_texture(IDirectDrawImpl* This,
				      const DDSURFACEDESC2* pDDSD,
				      LPDIRECTDRAWSURFACE7* ppSurf,
				      LPUNKNOWN pOuter,
				      DWORD dwMipMapLevel)
{
    return HAL_DirectDrawSurface_Create(This, pDDSD, ppSurf, pOuter);
}

HRESULT WINAPI
HAL_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
				   LPDDDEVICEIDENTIFIER2 pDDDI,
				   DWORD dwFlags)
{
    *pDDDI = hal_device;
    return DD_OK;
}

HRESULT WINAPI
HAL_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    HRESULT hr;

    TRACE("(%p)\n", iface);

    if (!(dd_gbl.dwFlags & DDRAWI_MODECHANGED)) return DD_OK;

    hr = Main_DirectDraw_RestoreDisplayMode(iface);
    if (SUCCEEDED(hr)) {
	hr = set_mode(This, dd_gbl.dwModeIndexOrig);
	if (SUCCEEDED(hr)) dd_gbl.dwFlags &= ~DDRAWI_MODECHANGED;
    }

    return hr;
}

HRESULT WINAPI
HAL_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
			      DWORD dwHeight, DWORD dwBPP,
			      DWORD dwRefreshRate, DWORD dwFlags)
{
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    HRESULT hr;

    TRACE("(%p)->(%ldx%ldx%ld,%ld Hz,%08lx)\n",This,dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
    hr = User_DirectDraw_SetDisplayMode(iface, dwWidth, dwHeight, dwBPP,
					dwRefreshRate, dwFlags);

    if (SUCCEEDED(hr)) {
	if (!(dd_gbl.dwFlags & DDRAWI_MODECHANGED)) dd_gbl.dwModeIndexOrig = dd_gbl.dwModeIndex;
	hr = set_mode(This, choose_mode(dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags));
	if (SUCCEEDED(hr)) dd_gbl.dwFlags |= DDRAWI_MODECHANGED;
    }

    return hr;
}

HRESULT WINAPI
HAL_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes,
			       LPDWORD pCodes)
{
    unsigned int i;
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
    if (*pNumCodes)
	*pNumCodes=dd_gbl.dwNumFourCC;
    if (pCodes && dd_gbl.dwNumFourCC)
	memcpy(pCodes,dd_gbl.lpdwFourCC,sizeof(pCodes[0])*dd_gbl.dwNumFourCC);
    FIXME("(%p,%p,%p)\n",This,pNumCodes,pCodes);
    if (dd_gbl.dwNumFourCC) {
	if (pCodes && FIXME_ON(ddraw)) {
	    FIXME("returning: ");
	    for (i=0;i<dd_gbl.dwNumFourCC;i++) {
		MESSAGE("%c%c%c%c,",
			((LPBYTE)(pCodes+i))[0],
			((LPBYTE)(pCodes+i))[1],
			((LPBYTE)(pCodes+i))[2],
			((LPBYTE)(pCodes+i))[3]
		);
	    }
	    MESSAGE("\n");
	}
    }
    return DD_OK;
}


static const IDirectDraw7Vtbl HAL_DirectDraw_VTable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    User_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    HAL_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    HAL_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    HAL_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    HAL_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
