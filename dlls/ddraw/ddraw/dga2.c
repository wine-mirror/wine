/*	DirectDraw driver for XF86DGA2 primary surface
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include "debugtools.h"
#include "ts_xlib.h"
#include "ts_xf86dga2.h"
#include "x11drv.h"
#include <ddraw.h>

#include <assert.h>
#include <stdlib.h>

#include "ddraw_private.h"
#include "ddraw/main.h"
#include "ddraw/user.h"
#include "ddraw/dga2.h"
#include "dclipper/main.h"
#include "dpalette/main.h"
#include "dsurface/main.h"
#include "dsurface/dib.h"
#include "dsurface/user.h"
#include "dsurface/dga2.h"

#include "options.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirectDraw7) XF86DGA2_DirectDraw_VTable;

static const DDDEVICEIDENTIFIER2 xf86dga2_device = 
{
    "XF86DGA2 Driver",
    "WINE DirectDraw on XF86DGA2",
    { { 0x00010001, 0x00010001 } },
    0, 0, 0, 0,
    /* e2dcb020-dc60-11d1-8407-9714f5d50803 */
    {0xe2dcb020,0xdc60,0x11d1,{0x84,0x07,0x97,0x14,0xf5,0xd5,0x08,0x03}},
    0
};

HRESULT XF86DGA2_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);
HRESULT XF86DGA2_DirectDraw_Initialize(IDirectDrawImpl*, const GUID*);

static const ddraw_driver xf86dga2_driver =
{
    &xf86dga2_device,
    20, /* XVidMode is 11 */
    XF86DGA2_DirectDraw_Create,
    XF86DGA2_DirectDraw_Initialize
};

static XDGAMode* modes;
static DWORD num_modes;
static int dga_event, dga_error;

/* Called from DllInit, which is synchronised so there are no threading
 * concerns. */
static BOOL initialize(void)
{
    int nmodes;
    int major, minor;

    if (X11DRV_GetXRootWindow() != DefaultRootWindow(display)) return FALSE;

    /* FIXME: don't use PROFILE calls */
    if (!PROFILE_GetWineIniBool("x11drv", "UseDGA", 1)) return FALSE;

    if (!TSXDGAQueryExtension(display, &dga_event, &dga_error)) return FALSE;

    if (!TSXDGAQueryVersion(display, &major, &minor)) return FALSE;

    if (major < 2) return FALSE; /* only bother with DGA2 */

    /* test that it works */
    if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	TRACE("disabling XF86DGA2 (insufficient permissions?)\n");
	return FALSE;
    }
    TSXDGACloseFramebuffer(display, DefaultScreen(display));
    
    TRACE("getting XF86DGA2 mode list\n");
    modes = TSXDGAQueryModes(display, DefaultScreen(display), &nmodes);
    if (!modes) return FALSE;
    num_modes = nmodes;

    TRACE("enabling XF86DGA2\n");

    return TRUE;
}

static void cleanup(void)
{
    TSXFree(modes);
}

static XDGAMode* choose_mode(DWORD dwWidth, DWORD dwHeight,
		       DWORD dwRefreshRate, DWORD dwFlags)
{
    XDGAMode* best = NULL;
    int i;

    /* Choose the smallest mode that is large enough. */
    for (i=0; i < num_modes; i++)
    {
	if (modes[i].viewportWidth >= dwWidth && modes[i].viewportHeight >= dwHeight)
	{
	    if (best == NULL) best = &modes[i];
	    else
	    {
		if (modes[i].viewportWidth < best->viewportWidth
		    || modes[i].viewportHeight < best->viewportHeight)
		    best = &modes[i];
	    }
	}
    }

    /* all modes were too small, use the largest */
    if (!best)
    {
	TRACE("all modes too small\n");

	for (i=1; i < num_modes; i++)
	{
	    if (best == NULL) best = &modes[i];
	    else
	    {
		if (modes[i].viewportWidth > best->viewportWidth
		    || modes[i].viewportHeight > best->viewportHeight)
		    best = &modes[i];
	    }
	}
    }

    TRACE("using %d %d for %lu %lu\n", best->viewportWidth, best->viewportHeight,
	  dwWidth, dwHeight);

    return best;
}

BOOL DDRAW_XF86DGA2_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
	if (initialize())
	    DDRAW_register_driver(&xf86dga2_driver);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
	cleanup();
    }

    return TRUE;
}

/* Not called from the vtable. */
HRESULT XF86DGA2_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    HRESULT hr;

    TRACE("\n");

    hr = User_DirectDraw_Construct(This, ex);
    if (FAILED(hr)) return hr;

    This->final_release = XF86DGA2_DirectDraw_final_release;

    This->create_primary    = XF86DGA2_DirectDraw_create_primary;
    This->create_backbuffer = XF86DGA2_DirectDraw_create_backbuffer;

    ICOM_INIT_INTERFACE(This, IDirectDraw7, XF86DGA2_DirectDraw_VTable);

    return S_OK;
}

/* This function is called from DirectDrawCreate(Ex) on the most-derived
 * class to start construction.
 * Not called from the vtable. */
HRESULT XF86DGA2_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex)
{
    HRESULT hr;
    IDirectDrawImpl* This;

    TRACE("\n");

    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawImpl)
		     + sizeof(XF86DGA2_DirectDrawImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    /* Note that this relation does *not* hold true if the DD object was
     * CoCreateInstanced then Initialized. */
    This->private = (XF86DGA2_DirectDrawImpl *)(This+1);

    hr = XF86DGA2_DirectDraw_Construct(This, ex);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return hr;
}

/* This function is called from Uninit_DirectDraw_Initialize on the
 * most-derived-class to start initialization.
 * Not called from the vtable. */
HRESULT XF86DGA2_DirectDraw_Initialize(IDirectDrawImpl *This, const GUID* guid)
{
    HRESULT hr;

    TRACE("\n");

    This->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      sizeof(XF86DGA2_DirectDrawImpl));
    if (This->private == NULL) return E_OUTOFMEMORY;

    hr = XF86DGA2_DirectDraw_Construct(This, TRUE); /* XXX ex? */
    if (FAILED(hr))
    {
	HeapFree(GetProcessHeap(), 0, This->private);
	return hr;
    }

    return DD_OK;
}

/* Called from an internal function pointer. */
void XF86DGA2_DirectDraw_final_release(IDirectDrawImpl *This)
{
    XF86DGA2_DDRAW_PRIV_VAR(priv, This);

    if (priv->xf86dga2.current_mode) {
	TSXDGASetMode(display, DefaultScreen(display), 0);
	VirtualFree(priv->xf86dga2.current_mode->data, 0, MEM_RELEASE);
	X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_ABSOLUTE);
	X11DRV_EVENT_SetDGAStatus(0, -1);
	TSXFree(priv->xf86dga2.current_mode);
	TSXDGACloseFramebuffer(display, DefaultScreen(display));
	priv->xf86dga2.current_mode = NULL;
    }

    User_DirectDraw_final_release(This);
}

HRESULT XF86DGA2_DirectDraw_create_primary(IDirectDrawImpl* This,
					   const DDSURFACEDESC2* pDDSD,
					   LPDIRECTDRAWSURFACE7* ppSurf,
					   IUnknown* pUnkOuter)
{
    if (This->cooperative_level & DDSCL_EXCLUSIVE)
	return XF86DGA2_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
    else
	return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

HRESULT XF86DGA2_DirectDraw_create_backbuffer(IDirectDrawImpl* This,
					      const DDSURFACEDESC2* pDDSD,
					      LPDIRECTDRAWSURFACE7* ppSurf,
					      IUnknown* pUnkOuter,
					      IDirectDrawSurfaceImpl* primary)
{
    if (This->cooperative_level & DDSCL_EXCLUSIVE)
	return XF86DGA2_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
    else
	return User_DirectDrawSurface_Create(This, pDDSD, ppSurf, pUnkOuter);
}

HRESULT WINAPI
XF86DGA2_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags)
{
    *pDDDI = xf86dga2_device;
    return DD_OK;
}

HRESULT WINAPI
XF86DGA2_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    ICOM_THIS(IDirectDrawImpl, iface);
    HRESULT hr;

    TRACE("\n");

    hr = Main_DirectDraw_RestoreDisplayMode(iface);
    if (SUCCEEDED(hr))
    {
	XF86DGA2_DDRAW_PRIV_VAR(priv, This);

	if (priv->xf86dga2.current_mode)
	{
	    TSXDGASetMode(display, DefaultScreen(display), 0);
	    VirtualFree(priv->xf86dga2.current_mode->data, 0, MEM_RELEASE);
	    X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_ABSOLUTE);
	    X11DRV_EVENT_SetDGAStatus(0, -1);
	    TSXFree(priv->xf86dga2.current_mode);
	    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    priv->xf86dga2.current_mode = NULL;
	}
    }

    return hr;
}

HRESULT WINAPI
XF86DGA2_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
				   DWORD dwHeight, DWORD dwBPP,
				   DWORD dwRefreshRate, DWORD dwFlags)
{
    ICOM_THIS(IDirectDrawImpl, iface);

    HRESULT hr;

    TRACE("(%p)->(%ldx%ldx%ld,%ld Hz,%08lx)\n",This,dwWidth,dwHeight,dwBPP,dwRefreshRate,dwFlags);
    hr = User_DirectDraw_SetDisplayMode(iface, dwWidth, dwHeight, dwBPP,
					dwRefreshRate, dwFlags);

    if (SUCCEEDED(hr))
    {
	XF86DGA2_DDRAW_PRIV_VAR(priv, This);
	XDGADevice* old_mode = priv->xf86dga2.current_mode;
	XDGAMode* new_mode;
	int old_mode_num = old_mode ? old_mode->mode.num : 0;

	new_mode = choose_mode(dwWidth, dwHeight, dwRefreshRate, dwFlags);

	if (new_mode && new_mode->num != old_mode_num)
	{
	    XDGADevice * nm = NULL;
	    if (old_mode || TSXDGAOpenFramebuffer(display, DefaultScreen(display)))
		nm = TSXDGASetMode(display, DefaultScreen(display), new_mode->num);
	    if (nm) {
		TSXDGASetViewport(display, DefaultScreen(display), 0, 0, XDGAFlipImmediate);
		if (old_mode) {
		    VirtualFree(old_mode->data, 0, MEM_RELEASE);
		    TSXFree(old_mode);
		} else {
		    TSXDGASelectInput(display, DefaultScreen(display),
				      KeyPressMask|KeyReleaseMask|
				      ButtonPressMask|ButtonReleaseMask|
				      PointerMotionMask);
		    X11DRV_EVENT_SetDGAStatus(This->window, dga_event);
		    X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_RELATIVE);
		}
		priv->xf86dga2.current_mode = nm;
		priv->xf86dga2.next_vofs = 0;
		TRACE("frame buffer at %p, pitch=%d, width=%d, height=%d\n", nm->data,
		      nm->mode.bytesPerScanline, nm->mode.imageWidth, nm->mode.imageHeight);
		VirtualAlloc(nm->data, nm->mode.bytesPerScanline * nm->mode.imageHeight,
			     MEM_RESERVE|MEM_SYSTEM, PAGE_READWRITE);
	    } else {
		/* argh */
		ERR("failed\n");
		/* XXX revert size data to previous mode */
		if (!old_mode)
		    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    }
	}
    }

    return hr;
}

static ICOM_VTABLE(IDirectDraw7) XF86DGA2_DirectDraw_VTable =
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
    User_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    XF86DGA2_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    XF86DGA2_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    XF86DGA2_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};

#endif  /* HAVE_LIBXXF86DGA2 */
