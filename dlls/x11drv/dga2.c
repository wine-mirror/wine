/*
 * DirectDraw DGA2 interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */

#include "config.h"

#ifdef HAVE_LIBXXF86DGA2

#include "ts_xlib.h"
#include "ts_xf86dga2.h"
#include "x11drv.h"
#include "x11ddraw.h"
#include "dga2.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

extern int usedga;

LPDDHALMODEINFO xf86dga2_modes;
unsigned xf86dga2_mode_count;
static XDGAMode* modes;
static int dga_event, dga_error;

static void convert_mode(XDGAMode *mode, LPDDHALMODEINFO info)
{
  info->dwWidth        = mode->viewportWidth;
  info->dwHeight       = mode->viewportHeight;
  info->wRefreshRate   = mode->verticalRefresh;
  info->lPitch         = mode->bytesPerScanline;
  info->dwBPP          = (mode->depth < 24) ? mode->depth : mode->bitsPerPixel;
  info->wFlags         = (mode->depth == 8) ? DDMODEINFO_PALETTIZED : 0;
  info->dwRBitMask     = mode->redMask;
  info->dwGBitMask     = mode->greenMask;
  info->dwBBitMask     = mode->blueMask;
  info->dwAlphaBitMask = 0;
  TRACE(" width=%ld, height=%ld, bpp=%ld, refresh=%d\n", \
        info->dwWidth, info->dwHeight, info->dwBPP, info->wRefreshRate); \
}

void X11DRV_XF86DGA2_Init(void)
{
  int nmodes, major, minor, i;

  if (xf86dga2_modes) return; /* already initialized? */

  /* if in desktop mode, don't use DGA */
  if (root_window != DefaultRootWindow(gdi_display)) return;

  if (!usedga) return;

  if (!TSXDGAQueryExtension(gdi_display, &dga_event, &dga_error)) return;

  if (!TSXDGAQueryVersion(gdi_display, &major, &minor)) return;
  
  if (major < 2) return; /* only bother with DGA 2+ */

  /* test that it works */
  if (!TSXDGAOpenFramebuffer(gdi_display, DefaultScreen(gdi_display))) {
    TRACE("disabling XF86DGA2 (insufficient permissions?)\n");
    return;
  }
  TSXDGACloseFramebuffer(gdi_display, DefaultScreen(gdi_display));

  /* retrieve modes */
  modes = TSXDGAQueryModes(gdi_display, DefaultScreen(gdi_display), &nmodes);
  if (!modes) return;

  TRACE("DGA modes: count=%d\n", nmodes);

  xf86dga2_mode_count = nmodes+1;
  xf86dga2_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * (nmodes+1));

  /* make dummy mode for exiting DGA */
  memset(&xf86dga2_modes[0], 0, sizeof(xf86dga2_modes[0]));

  /* convert modes to DDHALMODEINFO format */
  for (i=0; i<nmodes; i++)
    convert_mode(&modes[i], &xf86dga2_modes[i+1]);

  TRACE("Enabling DGA\n");
}

void X11DRV_XF86DGA2_Cleanup(void)
{
  if (modes) TSXFree(modes);
}

static XDGADevice *dga_dev;

static VIDMEM dga_mem = {
  VIDMEM_ISRECTANGULAR | VIDMEM_ISHEAP
};

static DWORD PASCAL X11DRV_XF86DGA2_SetMode(LPDDHAL_SETMODEDATA data)
{
  LPDDRAWI_DIRECTDRAW_LCL ddlocal = data->lpDD->lpExclusiveOwner;
  DWORD vram;
  Display *display = thread_display();

  data->ddRVal = DD_OK;
  if (data->dwModeIndex) {
    /* enter DGA */
    XDGADevice *new_dev = NULL;
    if (dga_dev || TSXDGAOpenFramebuffer(display, DefaultScreen(display)))
      new_dev = TSXDGASetMode(display, DefaultScreen(display), modes[data->dwModeIndex-1].num);
    if (new_dev) {
      TSXDGASetViewport(display, DefaultScreen(display), 0, 0, XDGAFlipImmediate);
      if (dga_dev) {
	VirtualFree(dga_dev->data, 0, MEM_RELEASE);
	TSXFree(dga_dev);
      } else {
	TSXDGASelectInput(display, DefaultScreen(display),
			  KeyPressMask|KeyReleaseMask|
			  ButtonPressMask|ButtonReleaseMask|
			  PointerMotionMask);
	X11DRV_EVENT_SetDGAStatus(ddlocal->hWnd, dga_event);
	X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_RELATIVE);
      }
      dga_dev = new_dev;
      vram = dga_dev->mode.bytesPerScanline * dga_dev->mode.imageHeight;
      VirtualAlloc(dga_dev->data, vram, MEM_RESERVE|MEM_SYSTEM, PAGE_READWRITE);
      dga_mem.fpStart = (FLATPTR)dga_dev->data;
      dga_mem.u1.dwWidth = dga_dev->mode.bytesPerScanline;
      dga_mem.u2.dwHeight = dga_dev->mode.imageHeight;
      X11DRV_DDHAL_SwitchMode(data->dwModeIndex, dga_dev->data, &dga_mem);
      X11DRV_DD_IsDirect = TRUE;
    }
    else {
      ERR("failed\n");
      if (!dga_dev) TSXDGACloseFramebuffer(display, DefaultScreen(display));
      data->ddRVal = DDERR_GENERIC;
    }
  }
  else if (dga_dev) {
    /* exit DGA */
    X11DRV_DD_IsDirect = FALSE;
    X11DRV_DDHAL_SwitchMode(0, NULL, NULL);
    TSXDGASetMode(display, DefaultScreen(display), 0);
    VirtualFree(dga_dev->data, 0, MEM_RELEASE);
    X11DRV_EVENT_SetInputMethod(X11DRV_INPUT_ABSOLUTE);
    X11DRV_EVENT_SetDGAStatus(0, -1);
    TSXFree(dga_dev);
    TSXDGACloseFramebuffer(display, DefaultScreen(display));
    dga_dev = NULL;
  }
  return DDHAL_DRIVER_HANDLED;
}

static LPDDHAL_CREATESURFACE X11DRV_XF86DGA2_old_create_surface;

static DWORD PASCAL X11DRV_XF86DGA2_CreateSurface(LPDDHAL_CREATESURFACEDATA data)
{
  LPDDRAWI_DDRAWSURFACE_LCL lcl = *data->lplpSList;
  LPDDRAWI_DDRAWSURFACE_GBL gbl = lcl->lpGbl;
  LPDDSURFACEDESC2 desc = (LPDDSURFACEDESC2)data->lpDDSurfaceDesc;
  HRESULT hr = DDHAL_DRIVER_NOTHANDLED;

  if (X11DRV_XF86DGA2_old_create_surface)
    hr = X11DRV_XF86DGA2_old_create_surface(data);

  if (desc->ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER)) {
    gbl->fpVidMem = 0; /* tell ddraw to allocate the memory */
    hr = DDHAL_DRIVER_HANDLED;
  }
  return hr;
}

static DWORD PASCAL X11DRV_XF86DGA2_CreatePalette(LPDDHAL_CREATEPALETTEDATA data)
{
  Display *display = thread_display();
  data->lpDDPalette->u1.dwReserved1 = TSXDGACreateColormap(display, DefaultScreen(display), dga_dev, AllocAll);
  if (data->lpColorTable)
    X11DRV_DDHAL_SetPalEntries(data->lpDDPalette->u1.dwReserved1, 0, 256,
			       data->lpColorTable);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_XF86DGA2_Flip(LPDDHAL_FLIPDATA data)
{
  Display *display = thread_display();
  if (data->lpSurfCurr == X11DRV_DD_Primary) {
    DWORD ofs = data->lpSurfCurr->lpGbl->fpVidMem - dga_mem.fpStart;
    TSXDGASetViewport(display, DefaultScreen(display),
                      (ofs % dga_dev->mode.bytesPerScanline)*8/dga_dev->mode.bitsPerPixel,
                      ofs / dga_dev->mode.bytesPerScanline,
                      XDGAFlipImmediate);
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_XF86DGA2_SetPalette(LPDDHAL_SETPALETTEDATA data)
{
  Display *display = thread_display();
  if ((data->lpDDSurface == X11DRV_DD_Primary) &&
      data->lpDDPalette && data->lpDDPalette->u1.dwReserved1) {
    TSXDGAInstallColormap(display, DefaultScreen(display), data->lpDDPalette->u1.dwReserved1);
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

int X11DRV_XF86DGA2_CreateDriver(LPDDHALINFO info)
{
  if (!xf86dga2_mode_count) return 0; /* no DGA */

  info->dwNumModes = xf86dga2_mode_count;
  info->lpModeInfo = xf86dga2_modes;
  info->dwModeIndex = 0;

  X11DRV_XF86DGA2_old_create_surface = info->lpDDCallbacks->CreateSurface;
  info->lpDDCallbacks->SetMode = X11DRV_XF86DGA2_SetMode;
  info->lpDDCallbacks->CreateSurface = X11DRV_XF86DGA2_CreateSurface;
  info->lpDDCallbacks->CreatePalette = X11DRV_XF86DGA2_CreatePalette;
  info->lpDDSurfaceCallbacks->Flip = X11DRV_XF86DGA2_Flip;
  info->lpDDSurfaceCallbacks->SetPalette = X11DRV_XF86DGA2_SetPalette;
  return TRUE;
}

#endif /* HAVE_LIBXXF86DGA2 */
