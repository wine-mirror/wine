/*	DirectDraw driver for User-based primary surfaces
 *	with XF86VidMode mode switching in full-screen mode.
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"

#ifdef HAVE_LIBXXF86VM

#include "debugtools.h"
#include "ts_xlib.h"
#include "ts_xf86vmode.h"
#include "x11drv.h"
#include <ddraw.h>

#include <assert.h>
#include <stdlib.h>

#include "ddraw_private.h"
#include "ddraw/main.h"
#include "ddraw/user.h"
#include "ddraw/xvidmode.h"
#include "dclipper/main.h"
#include "dpalette/main.h"
#include "dsurface/main.h"
#include "dsurface/dib.h"
#include "dsurface/user.h"
#include "options.h"

#include "win.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirectDraw7) XVidMode_DirectDraw_VTable;

static const DDDEVICEIDENTIFIER2 xvidmode_device = 
{
    "User/XF86VidMode Driver",
    "WINE DirectDraw on User with XF86VidMode",
    { { 0x00010001, 0x00010001 } },
    0, 0, 0, 0,
    /* 40c1b248-9d7d-4a29-b7d7-4cd8109f3d5d */
    {0x40c1b248,0x9d7d,0x4a29,{0xd7,0xb7,0x4c,0xd8,0x10,0x9f,0x3d,0x5d}},
    0
};

HRESULT XVidMode_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex);
HRESULT XVidMode_DirectDraw_Initialize(IDirectDrawImpl*, const GUID*);

static const ddraw_driver xvidmode_driver =
{
    &xvidmode_device,
    11, /* User is 10 */
    XVidMode_DirectDraw_Create,
    XVidMode_DirectDraw_Initialize
};

static XF86VidModeModeInfo** modes;
static DWORD num_modes;

/* Called from DllInit, which is synchronised so there are no threading
 * concerns. */
static BOOL initialize(void)
{
    int nmodes;
    int major, minor;

    if (X11DRV_GetXRootWindow() != DefaultRootWindow(display)) return FALSE;

    if (!TSXF86VidModeQueryVersion(display, &major, &minor)) return FALSE;

    if (!TSXF86VidModeGetAllModeLines(display, DefaultScreen(display), &nmodes,
				      &modes))
	return FALSE;

    num_modes = nmodes;

    TRACE("enabling XVidMode\n");

    return TRUE;
}

static void cleanup(void)
{
    TSXFree(modes);
}

static HRESULT set_display_mode(XF86VidModeModeInfo* mode)
{
    int screen = DefaultScreen(display);

    TRACE("%d %d\n", mode->hdisplay, mode->vdisplay);

    /* This is questionable. Programs should leave switching unlocked when
     * they exit. So the only reason the display should be locked is if
     * another really doesn't want switches to happen. Maybe it would be better
     * to detect an XF86VideModeZoomLocked error. */
    TSXF86VidModeLockModeSwitch(display, screen, False);

    TSXSync(display, False);

    TSXF86VidModeSwitchToMode(display, screen, mode);

    TSXSync(display, False);

#if 0 /* doesn't work for me */
    TSXF86VidModeSetViewPort(display, screen, 0, 0);
#else
    TSXWarpPointer(display, None, RootWindow(display, screen), 0, 0, 0, 0, 0,
		   0);
#endif

    TSXFlush(display);

    return S_OK;
}

static XF86VidModeModeInfo* choose_mode(DWORD dwWidth, DWORD dwHeight,
					DWORD dwRefreshRate, DWORD dwFlags)
{
    XF86VidModeModeInfo* best = NULL;
    int i;

    /* Choose the smallest mode that is large enough. */
    for (i=0; i < num_modes; i++)
    {
	if (modes[i]->hdisplay >= dwWidth && modes[i]->vdisplay >= dwHeight)
	{
	    if (best == NULL) best = modes[i];
	    else
	    {
		if (modes[i]->hdisplay < best->hdisplay
		    || modes[i]->vdisplay < best->vdisplay)
		    best = modes[i];
	    }
	}
    }

    /* all modes were too small, use the largest */
    if (best == NULL)
    {
	TRACE("all modes too small\n");

	for (i=1; i < num_modes; i++)
	{
	    if (best == NULL) best = modes[i];
	    else
	    {
		if (modes[i]->hdisplay > best->hdisplay
		    || modes[i]->vdisplay > best->vdisplay)
		    best = modes[i];
	    }
	}
    }

    TRACE("using %d %d for %lu %lu\n", best->hdisplay, best->vdisplay,
	  dwWidth, dwHeight);

    return best;
}

static XF86VidModeModeInfo* get_current_mode(void)
{
    XF86VidModeModeLine line;
    int dotclock;
    int i;

    TSXF86VidModeGetModeLine(display, DefaultScreen(display), &dotclock,
			     &line);

    for (i=0; i < num_modes; i++)
    {
	if (modes[i]->dotclock == dotclock
	    && modes[i]->hdisplay == line.hdisplay
	    && modes[i]->hsyncstart == line.hsyncstart
	    && modes[i]->hsyncend == line.hsyncend
	    && modes[i]->htotal == line.htotal
	    /* && modes[i]->hskew == line.hskew */
	    && modes[i]->vdisplay == line.vdisplay
	    && modes[i]->vsyncstart == line.vsyncstart
	    && modes[i]->vsyncend == line.vsyncend
	    && modes[i]->vtotal == line.vtotal
	    && modes[i]->flags == line.flags)
	    return modes[i];
    }

    WARN("this can't happen\n");
    return modes[0]; /* should be the mode that X started in */
}

BOOL DDRAW_XVidMode_Init(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
	if (initialize())
	    DDRAW_register_driver(&xvidmode_driver);
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
	cleanup();
    }

    return TRUE;
}

/* Not called from the vtable. */
HRESULT XVidMode_DirectDraw_Construct(IDirectDrawImpl *This, BOOL ex)
{
    XVIDMODE_DDRAW_PRIV_VAR(priv,This);
    HRESULT hr;

    TRACE("\n");

    hr = User_DirectDraw_Construct(This, ex);
    if (FAILED(hr)) return hr;

    This->final_release = XVidMode_DirectDraw_final_release;

    priv->xvidmode.original_mode = get_current_mode();
    priv->xvidmode.current_mode = priv->xvidmode.original_mode;

    ICOM_INIT_INTERFACE(This, IDirectDraw7, XVidMode_DirectDraw_VTable);

    return S_OK;
}

/* This function is called from DirectDrawCreate(Ex) on the most-derived
 * class to start construction.
 * Not called from the vtable. */
HRESULT XVidMode_DirectDraw_Create(const GUID* pGUID, LPDIRECTDRAW7* pIface,
				   IUnknown* pUnkOuter, BOOL ex)
{
    HRESULT hr;
    IDirectDrawImpl* This;

    TRACE("\n");

    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(IDirectDrawImpl)
		     + sizeof(XVidMode_DirectDrawImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    /* Note that this relation does *not* hold true if the DD object was
     * CoCreateInstanced then Initialized. */
    This->private = (XVidMode_DirectDrawImpl *)(This+1);

    hr = XVidMode_DirectDraw_Construct(This, ex);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*pIface = ICOM_INTERFACE(This, IDirectDraw7);

    return hr;
}

/* This function is called from Uninit_DirectDraw_Initialize on the
 * most-derived-class to start initialization.
 * Not called from the vtable. */
HRESULT XVidMode_DirectDraw_Initialize(IDirectDrawImpl *This, const GUID* guid)
{
    HRESULT hr;

    TRACE("\n");

    This->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			      sizeof(XVidMode_DirectDrawImpl));
    if (This->private == NULL) return E_OUTOFMEMORY;

    hr = XVidMode_DirectDraw_Construct(This, TRUE); /* XXX ex? */
    if (FAILED(hr))
    {
	HeapFree(GetProcessHeap(), 0, This->private);
	return hr;
    }

    return DD_OK;
}

/* Called from an internal function pointer. */
void XVidMode_DirectDraw_final_release(IDirectDrawImpl *This)
{
    XVIDMODE_DDRAW_PRIV_VAR(priv, This);

    if (priv->xvidmode.current_mode != priv->xvidmode.original_mode)
	set_display_mode(priv->xvidmode.original_mode);

    User_DirectDraw_final_release(This);
}

HRESULT WINAPI
XVidMode_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
					LPDDDEVICEIDENTIFIER2 pDDDI,
					DWORD dwFlags)
{
    *pDDDI = xvidmode_device;
    return DD_OK;
}

HRESULT WINAPI
XVidMode_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
    ICOM_THIS(IDirectDrawImpl, iface);
    HRESULT hr;

    TRACE("\n");

    hr = Main_DirectDraw_RestoreDisplayMode(iface);
    if (SUCCEEDED(hr))
    {
	XVIDMODE_DDRAW_PRIV_VAR(priv, This);

	if (priv->xvidmode.current_mode != priv->xvidmode.original_mode)
	{
	    set_display_mode(priv->xvidmode.original_mode);
	    priv->xvidmode.current_mode = priv->xvidmode.original_mode;
	}
    }

    return hr;
}

HRESULT WINAPI
XVidMode_DirectDraw_SetDisplayMode(LPDIRECTDRAW7 iface, DWORD dwWidth,
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
	XVIDMODE_DDRAW_PRIV_VAR(priv, This);
	XF86VidModeModeInfo* new_mode;
        WND *tmpWnd = WIN_FindWndPtr(This->window);
        Window x11Wnd = X11DRV_WND_GetXWindow(tmpWnd);
        WIN_ReleaseWndPtr(tmpWnd);

	new_mode = choose_mode(dwWidth, dwHeight, dwRefreshRate, dwFlags);

	if (new_mode != NULL && new_mode != priv->xvidmode.current_mode)
	{
	    priv->xvidmode.current_mode = new_mode;
	    set_display_mode(priv->xvidmode.current_mode);
	}
        if (PROFILE_GetWineIniBool( "x11drv", "DXGrab", 0)) {
           /* Confine cursor movement (risky, but the user asked for it) */
           TSXGrabPointer(display, x11Wnd, True, 0, GrabModeAsync, GrabModeAsync, x11Wnd, None, CurrentTime);
        }
    }

    return hr;
}

static ICOM_VTABLE(IDirectDraw7) XVidMode_DirectDraw_VTable =
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
    XVidMode_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    XVidMode_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    XVidMode_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};

#endif /* HAVE_LIBXXF86VM */
