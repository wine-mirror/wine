/*	XF86DGA2 primary surface driver
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include "ts_xlib.h"
#include "ts_xf86dga2.h"
#include "x11drv.h"
#include "winerror.h"

#include <assert.h>
#include <stdlib.h>

#include "debugtools.h"
#include "ddraw_private.h"
#include "ddraw/user.h"
#include "ddraw/dga2.h"
#include "dsurface/main.h"
#include "dsurface/dib.h"
#include "dsurface/dga2.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

static ICOM_VTABLE(IDirectDrawSurface7) XF86DGA2_IDirectDrawSurface7_VTable;

HRESULT
XF86DGA2_DirectDrawSurface_Construct(IDirectDrawSurfaceImpl* This,
				     IDirectDrawImpl* pDD,
				     const DDSURFACEDESC2* pDDSD)
{
    XF86DGA2_PRIV_VAR(priv, This);
    XF86DGA2_DDRAW_PRIV_VAR(ddpriv, pDD);
    HRESULT hr;
    XDGADevice* mode;

    TRACE("(%p,%p,%p)\n",This,pDD,pDDSD);
    if (!ddpriv->xf86dga2.current_mode) {
	/* we need a mode! */
	hr = XF86DGA2_DirectDraw_SetDisplayMode(ICOM_INTERFACE(pDD, IDirectDraw7),
						pDD->width, pDD->height,
						pDD->pixelformat.u1.dwRGBBitCount,
						0, 0);
	if (FAILED(hr)) return hr;
    }

    /* grab framebuffer data from current_mode */
    mode = ddpriv->xf86dga2.current_mode;
    priv->xf86dga2.fb_pitch = mode->mode.bytesPerScanline;
    priv->xf86dga2.fb_vofs  = ddpriv->xf86dga2.next_vofs;
    priv->xf86dga2.fb_addr  = mode->data +
			      priv->xf86dga2.fb_pitch * priv->xf86dga2.fb_vofs;
    TRACE("vofs=%ld, addr=%p\n", priv->xf86dga2.fb_vofs, priv->xf86dga2.fb_addr);

    /* fill in surface_desc before we construct DIB from it */
    This->surface_desc = *pDDSD;
    This->surface_desc.lpSurface = priv->xf86dga2.fb_addr;
    This->surface_desc.u1.lPitch = priv->xf86dga2.fb_pitch;
    This->surface_desc.dwFlags |= DDSD_LPSURFACE | DDSD_PITCH;

    hr = DIB_DirectDrawSurface_Construct(This, pDD, &This->surface_desc);
    if (FAILED(hr)) return hr;

    if (This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
	priv->xf86dga2.pal = TSXDGACreateColormap(display, DefaultScreen(display), mode, AllocAll);
	TSXDGAInstallColormap(display, DefaultScreen(display), priv->xf86dga2.pal);
    }

    ddpriv->xf86dga2.next_vofs += pDDSD->dwHeight;

    ICOM_INIT_INTERFACE(This, IDirectDrawSurface7,
			XF86DGA2_IDirectDrawSurface7_VTable);

    This->final_release = XF86DGA2_DirectDrawSurface_final_release;
    This->duplicate_surface = XF86DGA2_DirectDrawSurface_duplicate_surface;

    This->flip_data   = XF86DGA2_DirectDrawSurface_flip_data;
    This->flip_update = XF86DGA2_DirectDrawSurface_flip_update;

    This->set_palette    = XF86DGA2_DirectDrawSurface_set_palette;
    This->update_palette = XF86DGA2_DirectDrawSurface_update_palette;

    This->get_display_window = XF86DGA2_DirectDrawSurface_get_display_window;

    return DD_OK;
}

HRESULT
XF86DGA2_DirectDrawSurface_Create(IDirectDrawImpl *pDD,
			      const DDSURFACEDESC2 *pDDSD,
			      LPDIRECTDRAWSURFACE7 *ppSurf,
			      IUnknown *pUnkOuter)
{
    IDirectDrawSurfaceImpl* This;
    HRESULT hr;
    assert(pUnkOuter == NULL);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		     sizeof(*This) + sizeof(XF86DGA2_DirectDrawSurfaceImpl));
    if (This == NULL) return E_OUTOFMEMORY;

    This->private = (XF86DGA2_DirectDrawSurfaceImpl*)(This+1);

    hr = XF86DGA2_DirectDrawSurface_Construct(This, pDD, pDDSD);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppSurf = ICOM_INTERFACE(This, IDirectDrawSurface7);

    return hr;
}

void XF86DGA2_DirectDrawSurface_final_release(IDirectDrawSurfaceImpl* This)
{
    XF86DGA2_PRIV_VAR(priv, This);

    DIB_DirectDrawSurface_final_release(This);
    if (priv->xf86dga2.pal)
	TSXFreeColormap(display, priv->xf86dga2.pal);
}

void XF86DGA2_DirectDrawSurface_set_palette(IDirectDrawSurfaceImpl* This,
					IDirectDrawPaletteImpl* pal) 
{
    DIB_DirectDrawSurface_set_palette(This, pal);
}

void XF86DGA2_DirectDrawSurface_update_palette(IDirectDrawSurfaceImpl* This,
					   IDirectDrawPaletteImpl* pal,
					   DWORD dwStart, DWORD dwCount,
					   LPPALETTEENTRY palent)
{
    XF86DGA2_PRIV_VAR(priv, This);

    if (This->surface_desc.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
	XColor c;
	int n;

	c.flags = DoRed|DoGreen|DoBlue;
	c.pixel = dwStart;
	for (n=0; n<dwCount; n++,c.pixel++) {
	    c.red   = palent[n].peRed   << 8;
	    c.green = palent[n].peGreen << 8;
	    c.blue  = palent[n].peBlue  << 8;
	    TSXStoreColor(display, priv->xf86dga2.pal, &c);
	}
	TSXFlush(display);
    }
}

HRESULT XF86DGA2_DirectDrawSurface_duplicate_surface(IDirectDrawSurfaceImpl* This,
						 LPDIRECTDRAWSURFACE7* ppDup)
{
    return XF86DGA2_DirectDrawSurface_Create(This->ddraw_owner,
					     &This->surface_desc, ppDup, NULL);
}

void XF86DGA2_DirectDrawSurface_flip_data(IDirectDrawSurfaceImpl* front,
				      IDirectDrawSurfaceImpl* back)
{
    XF86DGA2_PRIV_VAR(front_priv, front);
    XF86DGA2_PRIV_VAR(back_priv, back);

    {
	DWORD tmp;
	tmp = front_priv->xf86dga2.fb_vofs;
	front_priv->xf86dga2.fb_vofs = back_priv->xf86dga2.fb_vofs;
	back_priv->xf86dga2.fb_vofs = tmp;
    }
    {
	LPVOID tmp;
	tmp = front_priv->xf86dga2.fb_addr;
	front_priv->xf86dga2.fb_addr = back_priv->xf86dga2.fb_addr;
	back_priv->xf86dga2.fb_addr = tmp;
    }

    DIB_DirectDrawSurface_flip_data(front, back);
}

void XF86DGA2_DirectDrawSurface_flip_update(IDirectDrawSurfaceImpl* This)
{
    XF86DGA2_PRIV_VAR(priv, This);

    /* XXX having the Flip's dwFlags would be nice here */
    TSXDGASetViewport(display, DefaultScreen(display),
		      0, priv->xf86dga2.fb_vofs, XDGAFlipImmediate);
}

HWND XF86DGA2_DirectDrawSurface_get_display_window(IDirectDrawSurfaceImpl* This)
{
    /* there's a potential drawable in the ddraw object's current_mode->pixmap...
     * perhaps it's possible to use it for the Direct3D rendering as well? */
    return 0;
}

static ICOM_VTABLE(IDirectDrawSurface7) XF86DGA2_IDirectDrawSurface7_VTable =
{
    Main_DirectDrawSurface_QueryInterface,
    Main_DirectDrawSurface_AddRef,
    Main_DirectDrawSurface_Release,
    Main_DirectDrawSurface_AddAttachedSurface,
    Main_DirectDrawSurface_AddOverlayDirtyRect,
    DIB_DirectDrawSurface_Blt,
    Main_DirectDrawSurface_BltBatch,
    DIB_DirectDrawSurface_BltFast,
    Main_DirectDrawSurface_DeleteAttachedSurface,
    Main_DirectDrawSurface_EnumAttachedSurfaces,
    Main_DirectDrawSurface_EnumOverlayZOrders,
    Main_DirectDrawSurface_Flip,
    Main_DirectDrawSurface_GetAttachedSurface,
    Main_DirectDrawSurface_GetBltStatus,
    Main_DirectDrawSurface_GetCaps,
    Main_DirectDrawSurface_GetClipper,
    Main_DirectDrawSurface_GetColorKey,
    Main_DirectDrawSurface_GetDC,
    Main_DirectDrawSurface_GetFlipStatus,
    Main_DirectDrawSurface_GetOverlayPosition,
    Main_DirectDrawSurface_GetPalette,
    Main_DirectDrawSurface_GetPixelFormat,
    Main_DirectDrawSurface_GetSurfaceDesc,
    Main_DirectDrawSurface_Initialize,
    Main_DirectDrawSurface_IsLost,
    Main_DirectDrawSurface_Lock,
    Main_DirectDrawSurface_ReleaseDC,
    DIB_DirectDrawSurface_Restore,
    Main_DirectDrawSurface_SetClipper,
    Main_DirectDrawSurface_SetColorKey,
    Main_DirectDrawSurface_SetOverlayPosition,
    Main_DirectDrawSurface_SetPalette,
    Main_DirectDrawSurface_Unlock,
    Main_DirectDrawSurface_UpdateOverlay,
    Main_DirectDrawSurface_UpdateOverlayDisplay,
    Main_DirectDrawSurface_UpdateOverlayZOrder,
    Main_DirectDrawSurface_GetDDInterface,
    Main_DirectDrawSurface_PageLock,
    Main_DirectDrawSurface_PageUnlock,
    DIB_DirectDrawSurface_SetSurfaceDesc,
    Main_DirectDrawSurface_SetPrivateData,
    Main_DirectDrawSurface_GetPrivateData,
    Main_DirectDrawSurface_FreePrivateData,
    Main_DirectDrawSurface_GetUniquenessValue,
    Main_DirectDrawSurface_ChangeUniquenessValue,
    Main_DirectDrawSurface_SetPriority,
    Main_DirectDrawSurface_GetPriority,
    Main_DirectDrawSurface_SetLOD,
    Main_DirectDrawSurface_GetLOD
};

#endif /* HAVE_LIBXXF86DGA2 */
