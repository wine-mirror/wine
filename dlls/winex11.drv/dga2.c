/*
 * DirectDraw DGA2 interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
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

#ifdef HAVE_LIBXXF86DGA2

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include "x11drv.h"
#include "x11ddraw.h"
#include "dga2.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

extern int usedga;

LPDDHALMODEINFO xf86dga2_modes;
unsigned xf86dga2_mode_count;
HWND DGAhwnd = 0;
static XDGAMode* modes;

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
  TRACE(" width=%ld, height=%ld, bpp=%ld, refresh=%d\n",
        info->dwWidth, info->dwHeight, info->dwBPP, info->wRefreshRate);
}

static int DGA2ErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

static void X11DRV_DGAKeyPressEvent( HWND hwnd, XEvent *xev )
{
    /* Fill a XKeyEvent to send to EVENT_Key */
    XDGAKeyEvent *event = (XDGAKeyEvent *)xev;
    XEvent ke;

    ke.xkey.type = KeyPress;
    ke.xkey.serial = event->serial;
    ke.xkey.send_event = FALSE;
    ke.xkey.display = event->display;
    ke.xkey.window = 0;
    ke.xkey.root = 0;
    ke.xkey.subwindow = 0;
    ke.xkey.time = event->time;
    ke.xkey.x = -1;
    ke.xkey.y = -1;
    ke.xkey.x_root = -1;
    ke.xkey.y_root = -1;
    ke.xkey.state = event->state;
    ke.xkey.keycode = event->keycode;
    ke.xkey.same_screen = TRUE;
    X11DRV_KeyEvent( 0, &ke );
}

static void X11DRV_DGAKeyReleaseEvent( HWND hwnd, XEvent *xev )
{
    /* Fill a XKeyEvent to send to EVENT_Key */
    XDGAKeyEvent *event = (XDGAKeyEvent *)xev;
    XEvent ke;

    ke.xkey.type = KeyRelease;
    ke.xkey.serial = event->serial;
    ke.xkey.send_event = FALSE;
    ke.xkey.display = event->display;
    ke.xkey.window = 0;
    ke.xkey.root = 0;
    ke.xkey.subwindow = 0;
    ke.xkey.time = event->time;
    ke.xkey.x = -1;
    ke.xkey.y = -1;
    ke.xkey.x_root = -1;
    ke.xkey.y_root = -1;
    ke.xkey.state = event->state;
    ke.xkey.keycode = event->keycode;
    ke.xkey.same_screen = TRUE;
    X11DRV_KeyEvent( 0, &ke );
}


void X11DRV_XF86DGA2_Init(void)
{
  int nmodes, major, minor, i, event_base, error_base;
  Bool ok;

  TRACE("\n");

  if (xf86dga2_modes) return; /* already initialized? */

  /* if in desktop mode, don't use DGA */
  if (root_window != DefaultRootWindow(gdi_display)) return;

  if (!usedga) return;

  wine_tsx11_lock();
  ok = XDGAQueryExtension(gdi_display, &event_base, &error_base);
  if (ok)
  {
      X11DRV_expect_error(gdi_display, DGA2ErrorHandler, NULL);
      ok = XDGAQueryVersion(gdi_display, &major, &minor);
      if (X11DRV_check_error()) ok = FALSE;
  }
  wine_tsx11_unlock();
  if (!ok) return;

  if (major < 2) return; /* only bother with DGA 2+ */

  /* test that it works */
  wine_tsx11_lock();
  X11DRV_expect_error(gdi_display, DGA2ErrorHandler, NULL);
  ok = XDGAOpenFramebuffer(gdi_display, DefaultScreen(gdi_display));
  if (X11DRV_check_error()) ok = FALSE;
  if (ok)
  {
      XDGACloseFramebuffer(gdi_display, DefaultScreen(gdi_display));
      /* retrieve modes */
      modes = XDGAQueryModes(gdi_display, DefaultScreen(gdi_display), &nmodes);
      if (!modes) ok = FALSE;
  }
  else WARN("disabling XF86DGA2 (insufficient permissions?)\n");
  wine_tsx11_unlock();
  if (!ok) return;

  TRACE("DGA modes: count=%d\n", nmodes);

  xf86dga2_mode_count = nmodes+1;
  xf86dga2_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * (nmodes+1));

  /* make dummy mode for exiting DGA */
  memset(&xf86dga2_modes[0], 0, sizeof(xf86dga2_modes[0]));

  /* convert modes to DDHALMODEINFO format */
  for (i=0; i<nmodes; i++)
    convert_mode(&modes[i], &xf86dga2_modes[i+1]);

  /* register event handlers */
  X11DRV_register_event_handler( event_base + MotionNotify, X11DRV_DGAMotionEvent );
  X11DRV_register_event_handler( event_base + ButtonPress, X11DRV_DGAButtonPressEvent );
  X11DRV_register_event_handler( event_base + ButtonRelease, X11DRV_DGAButtonReleaseEvent );
  X11DRV_register_event_handler( event_base + KeyPress, X11DRV_DGAKeyPressEvent );
  X11DRV_register_event_handler( event_base + KeyRelease, X11DRV_DGAKeyReleaseEvent );

  TRACE("Enabling XF86DGA2 mode\n");
}

void X11DRV_XF86DGA2_Cleanup(void)
{
    wine_tsx11_lock();
    if (modes) XFree(modes);
    wine_tsx11_unlock();
}

static XDGADevice *dga_dev;

static VIDMEM dga_mem = {
  VIDMEM_ISRECTANGULAR | VIDMEM_ISHEAP
};

static DWORD PASCAL X11DRV_XF86DGA2_SetMode(LPDDHAL_SETMODEDATA data)
{
  LPDDRAWI_DIRECTDRAW_LCL ddlocal = data->lpDD->lpExclusiveOwner;
  DWORD vram;
  Display *display = gdi_display;

  data->ddRVal = DD_OK;
  wine_tsx11_lock();
  if (data->dwModeIndex) {
    /* enter DGA */
    XDGADevice *new_dev = NULL;
    if (dga_dev || XDGAOpenFramebuffer(display, DefaultScreen(display)))
      new_dev = XDGASetMode(display, DefaultScreen(display), modes[data->dwModeIndex-1].num);
    if (new_dev) {
      XDGASetViewport(display, DefaultScreen(display), 0, 0, XDGAFlipImmediate);
      if (dga_dev) {
	VirtualFree(dga_dev->data, 0, MEM_RELEASE);
	XFree(dga_dev);
      } else {
	XDGASelectInput(display, DefaultScreen(display),
			  KeyPressMask|KeyReleaseMask|
			  ButtonPressMask|ButtonReleaseMask|
			  PointerMotionMask);
        DGAhwnd = (HWND)ddlocal->hWnd;
      }
      dga_dev = new_dev;
      vram = dga_dev->mode.bytesPerScanline * dga_dev->mode.imageHeight;
      VirtualAlloc(dga_dev->data, vram, MEM_SYSTEM, PAGE_READWRITE);
      dga_mem.fpStart = (FLATPTR)dga_dev->data;
      dga_mem.u1.dwWidth = dga_dev->mode.bytesPerScanline;
      dga_mem.u2.dwHeight = dga_dev->mode.imageHeight;
      X11DRV_DDHAL_SwitchMode(data->dwModeIndex, dga_dev->data, &dga_mem);
      X11DRV_DD_IsDirect = TRUE;
    }
    else {
      ERR("failed\n");
      if (!dga_dev) XDGACloseFramebuffer(display, DefaultScreen(display));
      data->ddRVal = DDERR_GENERIC;
    }
  }
  else if (dga_dev) {
    /* exit DGA */
    X11DRV_DD_IsDirect = FALSE;
    X11DRV_DDHAL_SwitchMode(0, NULL, NULL);
    XDGASetMode(display, DefaultScreen(display), 0);
    VirtualFree(dga_dev->data, 0, MEM_RELEASE);
    DGAhwnd = 0;
    XFree(dga_dev);
    XDGACloseFramebuffer(display, DefaultScreen(display));
    dga_dev = NULL;
  }
  wine_tsx11_unlock();
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
  Display *display = gdi_display;
  wine_tsx11_lock();
  data->lpDDPalette->u1.dwReserved1 = XDGACreateColormap(display, DefaultScreen(display),
                                                         dga_dev, AllocAll);
  wine_tsx11_unlock();
  if (data->lpColorTable)
    X11DRV_DDHAL_SetPalEntries(data->lpDDPalette->u1.dwReserved1, 0, 256,
			       data->lpColorTable);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_XF86DGA2_Flip(LPDDHAL_FLIPDATA data)
{
  Display *display = gdi_display;
  if (data->lpSurfCurr == X11DRV_DD_Primary) {
    DWORD ofs = data->lpSurfCurr->lpGbl->fpVidMem - dga_mem.fpStart;
    wine_tsx11_lock();
    XDGASetViewport(display, DefaultScreen(display),
                    (ofs % dga_dev->mode.bytesPerScanline)*8/dga_dev->mode.bitsPerPixel,
                    ofs / dga_dev->mode.bytesPerScanline,
                    XDGAFlipImmediate);
    wine_tsx11_unlock();
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_XF86DGA2_SetPalette(LPDDHAL_SETPALETTEDATA data)
{
  Display *display = gdi_display;
  if ((data->lpDDSurface == X11DRV_DD_Primary) &&
      data->lpDDPalette && data->lpDDPalette->u1.dwReserved1)
  {
      wine_tsx11_lock();
      XDGAInstallColormap(display, DefaultScreen(display), data->lpDDPalette->u1.dwReserved1);
      wine_tsx11_unlock();
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
