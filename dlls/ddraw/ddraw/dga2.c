/*		DirectDraw IDirectDraw XF86DGA interface
 *
 *  DGA2's specific IDirectDraw functions...
 */

#include "config.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#include "dga2_private.h"

struct ICOM_VTABLE(IDirectDraw)		dga2_ddvt;
struct ICOM_VTABLE(IDirectDraw2)	dga2_dd2vt;
struct ICOM_VTABLE(IDirectDraw4)	dga2_dd4vt;

#define DDPRIVATE(x) dga2_dd_private *ddpriv = ((dga2_dd_private*)(x)->private)
#define DPPRIVATE(x) dga2_dp_private *dppriv = ((dga2_dp_private*)(x)->private)

/*******************************************************************************
 *				IDirectDraw
 */
static HRESULT WINAPI DGA2_IDirectDraw2Impl_CreateSurface(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,
    IUnknown *lpunk
) {
  HRESULT ret;
  IDirectDrawSurfaceImpl* dsurf;

  ret = DGA_IDirectDraw2Impl_CreateSurface_no_VT(iface, lpddsd, lpdsf, lpunk);

  dsurf = *(IDirectDrawSurfaceImpl**)lpdsf;
  ICOM_VTBL(dsurf) = (ICOM_VTABLE(IDirectDrawSurface)*)&dga2_dds4vt;

  return ret;
}

static HRESULT WINAPI DGA2_IDirectDraw2Impl_SetCooperativeLevel(
    LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
) {
    /* ICOM_THIS(IDirectDraw2Impl,iface); */
    HRESULT ret;
    int evbase, erbase;

    ret = IDirectDraw2Impl_SetCooperativeLevel(iface, hwnd, cooplevel);

    if (ret != DD_OK)
      return ret;

    TSXDGAQueryExtension(display, &evbase, &erbase);
    
    /* Now, start handling of DGA events giving the handle to the DDraw window
       as the window for which the event will be reported */
    TSXDGASelectInput(display, DefaultScreen(display),
		      KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask );
    X11DRV_EVENT_SetDGAStatus(hwnd, evbase);
    return DD_OK;
}

void _DGA2_Initialize_FrameBuffer(IDirectDrawImpl *This, int mode) {
    DDPIXELFORMAT *pf = &(This->d.directdraw_pixelformat);
    DDPRIVATE(This);

    /* Now, get the device / mode description */
    ddpriv->dev = TSXDGASetMode(display, DefaultScreen(display), mode);

    ddpriv->DGA.fb_width = ddpriv->dev->mode.imageWidth;
    TSXDGASetViewport(display,DefaultScreen(display),0,0, XDGAFlipImmediate);
    ddpriv->DGA.fb_height = ddpriv->dev->mode.viewportHeight;
    TRACE("video framebuffer: begin %p, width %d, memsize %d\n",
	ddpriv->dev->data,
	ddpriv->dev->mode.imageWidth,
	(ddpriv->dev->mode.imageWidth *
	 ddpriv->dev->mode.imageHeight *
	 (ddpriv->dev->mode.bitsPerPixel / 8))
    );
    TRACE("viewport height: %d\n", ddpriv->dev->mode.viewportHeight);
    /* Get the screen dimensions as seen by Wine.
     * In that case, it may be better to ignore the -desktop mode and return the
     * real screen size => print a warning */
    This->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    This->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
    ddpriv->DGA.fb_addr = ddpriv->dev->data;
    ddpriv->DGA.fb_memsize = (ddpriv->dev->mode.imageWidth *
			      ddpriv->dev->mode.imageHeight *
			      (ddpriv->dev->mode.bitsPerPixel / 8));
    ddpriv->DGA.vpmask = 0;

    /* Fill the screen pixelformat */
    pf->dwSize = sizeof(DDPIXELFORMAT);
    pf->dwFourCC = 0;
    pf->u.dwRGBBitCount = ddpriv->dev->mode.bitsPerPixel;
    if (ddpriv->dev->mode.depth == 8) {
	pf->dwFlags = DDPF_PALETTEINDEXED8|DDPF_RGB;
	pf->u1.dwRBitMask = 0;
	pf->u2.dwGBitMask = 0;
	pf->u3.dwBBitMask = 0;
    } else {
	pf->dwFlags = DDPF_RGB;
	pf->u1.dwRBitMask = ddpriv->dev->mode.redMask;
	pf->u2.dwGBitMask = ddpriv->dev->mode.greenMask;
	pf->u3.dwBBitMask = ddpriv->dev->mode.blueMask;
    }
    pf->u4.dwRGBAlphaBitMask= 0;
    This->d.screen_pixelformat = *pf; 
}

static HRESULT WINAPI DGA2_IDirectDrawImpl_SetDisplayMode(
    LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
    ICOM_THIS(IDirectDrawImpl,iface);
    DDPRIVATE(This);
    int	i;
    XDGAMode *modes = ddpriv->modes;
    int mode_to_use = -1;

    TRACE("(%p)->(%ld,%ld,%ld)\n", This, width, height, depth);

    /* Search in the list a display mode that corresponds to what is requested */
    for (i = 0; i < ddpriv->num_modes; i++) {
      if ((height == modes[i].viewportHeight) &&
	  (width == modes[i].viewportWidth) &&
	  (depth == modes[i].depth)
	  )
	mode_to_use = modes[i].num;
    }
    
    if (mode_to_use < 0) {
      ERR("Could not find matching mode !!!\n");
      return DDERR_UNSUPPORTEDMODE;
    } else {
      TRACE("Using mode number %d\n", mode_to_use);
      
      VirtualFree(ddpriv->DGA.fb_addr, 0, MEM_RELEASE);
      TSXDGACloseFramebuffer(display, DefaultScreen(display));
      
      if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	ERR("Error opening the frame buffer !!!\n");
	return DDERR_GENERIC;
      }
      
      /* Initialize the frame buffer */
      _DGA2_Initialize_FrameBuffer(This, mode_to_use);
      VirtualAlloc(ddpriv->DGA.fb_addr, ddpriv->DGA.fb_memsize, MEM_RESERVE|MEM_SYSTEM, PAGE_READWRITE);
      
      /* Re-get (if necessary) the DGA events */
      TSXDGASelectInput(display, DefaultScreen(display),
			KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask );
    }
    
    return DD_OK;
}

static HRESULT WINAPI DGA2_IDirectDraw2Impl_CreatePalette(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawPaletteImpl*	ddpal;
    dga_dp_private		*dppriv;
    HRESULT			res;
    int 			xsize = 0,i;
    DDPRIVATE(This);

    TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,lpddpal,lpunk);
    res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,(IDirectDrawPaletteImpl**)lpddpal,lpunk,&xsize);
    if (res != 0)
	return res;
    ddpal = *(IDirectDrawPaletteImpl**)lpddpal;
    ddpal->private = HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(dga_dp_private)
    );
    dppriv = (dga_dp_private*)ddpal->private;

    ICOM_VTBL(ddpal)= &dga_ddpalvt;
    if (This->d.directdraw_pixelformat.u.dwRGBBitCount<=8) {
	dppriv->cm = TSXDGACreateColormap(display,DefaultScreen(display), ddpriv->dev, AllocAll);
    } else {
	ERR("why are we doing CreatePalette in hi/truecolor?\n");
	dppriv->cm = 0;
    }
    if (dppriv->cm && xsize) {
	for (i=0;i<xsize;i++) {
	    XColor xc;

	    xc.red = ddpal->palents[i].peRed<<8;
	    xc.blue = ddpal->palents[i].peBlue<<8;
	    xc.green = ddpal->palents[i].peGreen<<8;
	    xc.flags = DoRed|DoBlue|DoGreen;
	    xc.pixel = i;
	    TSXStoreColor(display,dppriv->cm,&xc);
	}
    }
    return DD_OK;
}

static HRESULT WINAPI DGA2_IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->()\n",This);
    Sleep(1000);
    
    XDGACloseFramebuffer(display,DefaultScreen(display));
    XDGASetMode(display,DefaultScreen(display), 0);

    return DD_OK;
}

static ULONG WINAPI DGA2_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
      TRACE("Closing access to the FrameBuffer\n");
      VirtualFree(ddpriv->DGA.fb_addr, 0, MEM_RELEASE);
      TSXDGACloseFramebuffer(display, DefaultScreen(display));
      TRACE("Going back to normal X mode of operation\n");
      TSXDGASetMode(display, DefaultScreen(display), 0);

      /* Set the input handling back to absolute */
      X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_ABSOLUTE);
      
      /* Remove the handling of DGA2 events */
      X11DRV_EVENT_SetDGAStatus(0, -1);
      
      /* Free the modes list */
      TSXFree(ddpriv->modes);

      HeapFree(GetProcessHeap(),0,This);
      return S_OK;
    }
    return This->ref;
}

static HRESULT WINAPI DGA2_IDirectDraw2Impl_EnumDisplayModes(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDSURFACEDESC	ddsfd;
    DDPRIVATE(This);
    int	i;
    XDGAMode *modes = ddpriv->modes;

    TRACE("(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
    ddsfd.dwSize = sizeof(ddsfd);
    ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
    if (dwFlags & DDEDM_REFRESHRATES) {
	    ddsfd.dwFlags |= DDSD_REFRESHRATE;
	    ddsfd.u.dwRefreshRate = 60;
    }
    ddsfd.ddsCaps.dwCaps = 0;
    ddsfd.dwBackBufferCount = 1;


    ddsfd.dwFlags |= DDSD_PITCH;
    for (i = 0; i < ddpriv->num_modes; i++) {
      if (TRACE_ON(ddraw)) {
	DPRINTF("  Enumerating mode %d : %s (FB: %dx%d / VP: %dx%d) - depth %d -",
		modes[i].num,
		modes[i].name, modes[i].imageWidth, modes[i].imageHeight,
		modes[i].viewportWidth, modes[i].viewportHeight,
		modes[i].depth);
	if (modes[i].flags & XDGAConcurrentAccess) DPRINTF(" XDGAConcurrentAccess ");
	if (modes[i].flags & XDGASolidFillRect) DPRINTF(" XDGASolidFillRect ");
	if (modes[i].flags & XDGABlitRect) DPRINTF(" XDGABlitRect ");
	if (modes[i].flags & XDGABlitTransRect) DPRINTF(" XDGABlitTransRect ");
	if (modes[i].flags & XDGAPixmap) DPRINTF(" XDGAPixmap ");
	DPRINTF("\n");
      }
      /* Fill the pixel format */
      ddsfd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
      ddsfd.ddpfPixelFormat.dwFourCC = 0;
      ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;
      ddsfd.ddpfPixelFormat.u.dwRGBBitCount = modes[i].bitsPerPixel;
      if (modes[i].depth == 8) {
	ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB|DDPF_PALETTEINDEXED8;
	ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0;
	ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0;
	ddsfd.ddpfPixelFormat.u3.dwBBitMask = 0;
	ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;
      } else {
	ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsfd.ddpfPixelFormat.u1.dwRBitMask = modes[i].redMask;
	ddsfd.ddpfPixelFormat.u2.dwGBitMask = modes[i].greenMask;
	ddsfd.ddpfPixelFormat.u3.dwBBitMask = modes[i].blueMask;
      }
      
      ddsfd.dwWidth = modes[i].viewportWidth;
      ddsfd.dwHeight = modes[i].viewportHeight;
      ddsfd.lPitch = modes[i].imageWidth;
      
      /* Send mode to the application */
      if (!modescb(&ddsfd,context)) return DD_OK;
    }
    
    return DD_OK;
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga2_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

struct ICOM_VTABLE(IDirectDraw) dga2_ddvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)DGA_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)DGA2_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)DGA2_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)DGA2_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)DGA2_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)DGA_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)DGA_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)DGA2_IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)DGA2_IDirectDraw2Impl_SetCooperativeLevel,
    DGA2_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
};
#undef XCAST

/*****************************************************************************
 * 	IDirectDraw2
 *
 */

static HRESULT WINAPI DGA2_IDirectDraw2Impl_SetDisplayMode(
    LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD dwRefreshRate, DWORD dwFlags
) {
    FIXME( "Ignored parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
    return DGA2_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

ICOM_VTABLE(IDirectDraw2) dga2_dd2vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DGA_IDirectDraw2Impl_QueryInterface,
    IDirectDraw2Impl_AddRef,
    DGA2_IDirectDraw2Impl_Release,
    IDirectDraw2Impl_Compact,
    IDirectDraw2Impl_CreateClipper,
    DGA2_IDirectDraw2Impl_CreatePalette,
    DGA2_IDirectDraw2Impl_CreateSurface,
    IDirectDraw2Impl_DuplicateSurface,
    DGA2_IDirectDraw2Impl_EnumDisplayModes,
    IDirectDraw2Impl_EnumSurfaces,
    IDirectDraw2Impl_FlipToGDISurface,
    DGA_IDirectDraw2Impl_GetCaps,
    DGA_IDirectDraw2Impl_GetDisplayMode,
    IDirectDraw2Impl_GetFourCCCodes,
    IDirectDraw2Impl_GetGDISurface,
    IDirectDraw2Impl_GetMonitorFrequency,
    IDirectDraw2Impl_GetScanLine,
    IDirectDraw2Impl_GetVerticalBlankStatus,
    IDirectDraw2Impl_Initialize,
    DGA2_IDirectDraw2Impl_RestoreDisplayMode,
    DGA2_IDirectDraw2Impl_SetCooperativeLevel,
    DGA2_IDirectDraw2Impl_SetDisplayMode,
    IDirectDraw2Impl_WaitForVerticalBlank,
    DGA_IDirectDraw2Impl_GetAvailableVidMem
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga2_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

ICOM_VTABLE(IDirectDraw4) dga2_dd4vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)DGA_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)DGA2_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)DGA2_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)DGA2_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)DGA2_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)DGA_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)DGA_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)DGA2_IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)DGA2_IDirectDraw2Impl_SetCooperativeLevel,
    XCAST(SetDisplayMode)DGA2_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
    XCAST(GetAvailableVidMem)DGA_IDirectDraw2Impl_GetAvailableVidMem,
    IDirectDraw4Impl_GetSurfaceFromDC,
    IDirectDraw4Impl_RestoreAllSurfaces,
    IDirectDraw4Impl_TestCooperativeLevel,
    IDirectDraw4Impl_GetDeviceIdentifier
};
#undef XCAST
