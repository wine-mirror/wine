/*
 * DirectDraw XVidMode interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 */

#include "config.h"

/* FIXME: XVidMode also includes gamma functions,
 * we could perhaps use it to implement Get/SetGammaRamp */

/* FIXME: ChangeDisplaySettings ought to be able to use this */

#ifdef HAVE_LIBXXF86VM

#include "ts_xlib.h"
#include "ts_xf86vmode.h"
#include "x11drv.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(x11drv);

LPDDHALMODEINFO xf86vm_modes;
unsigned xf86vm_mode_count;
XF86VidModeModeInfo** modes;

#define CONVERT_MODE(dotclock) \
  info->dwWidth      = mode->hdisplay; \
  info->dwHeight     = mode->vdisplay; \
  info->wRefreshRate = dotclock * 1000 / (mode->htotal * mode->vtotal); \
  TRACE(" width=%ld, height=%ld, refresh=%d\n", \
	info->dwWidth, info->dwHeight, info->wRefreshRate); \
  /* XVidMode cannot change display depths... */ \
  /* let's not bother with filling out these then... */ \
  info->lPitch         = 0; \
  info->dwBPP          = 0; \
  info->wFlags         = 0; \
  info->dwRBitMask     = 0; \
  info->dwGBitMask     = 0; \
  info->dwBBitMask     = 0; \
  info->dwAlphaBitMask = 0;

static void convert_modeinfo(XF86VidModeModeInfo *mode, LPDDHALMODEINFO info)
{
  CONVERT_MODE(mode->dotclock)
}

static void convert_modeline(int dotclock, XF86VidModeModeLine *mode, LPDDHALMODEINFO info)
{
  CONVERT_MODE(dotclock)
}

void X11DRV_XF86VM_Init(void)
{
  int nmodes, major, minor, i;

  if (xf86vm_modes) return; /* already initialized? */

  /* if in desktop mode, don't use XVidMode */
  if (X11DRV_GetXRootWindow() != DefaultRootWindow(display)) return;

  /* see if XVidMode is available */
  if (!TSXF86VidModeQueryVersion(display, &major, &minor)) return;

  /* retrieve modes */
  if (!TSXF86VidModeGetAllModeLines(display, DefaultScreen(display), &nmodes,
				    &modes))
    return;

  TRACE("XVidMode modes: count=%d\n", nmodes);

  xf86vm_mode_count = nmodes;
  xf86vm_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * nmodes);

  /* convert modes to DDHALMODEINFO format */
  for (i=0; i<nmodes; i++)
    convert_modeinfo(modes[i], &xf86vm_modes[i]);

  TRACE("Enabling XVidMode\n");
}

void X11DRV_XF86VM_Cleanup(void)
{
  TSXFree(modes);
}

int X11DRV_XF86VM_GetCurrentMode(void)
{
  XF86VidModeModeLine line;
  int dotclock, i;
  DDHALMODEINFO cmode;

  TRACE("Querying XVidMode current mode\n");
  TSXF86VidModeGetModeLine(display, DefaultScreen(display), &dotclock, &line);
  convert_modeline(dotclock, &line, &cmode);
  for (i=0; i<xf86vm_mode_count; i++)
    if (memcmp(&xf86vm_modes[i], &cmode, sizeof(cmode)) == 0) {
      TRACE("mode=%d\n", i);
      return i;
    }
  ERR("unknown mode, shouldn't happen\n");
  return 0; /* return first mode */
}

void X11DRV_XF86VM_SetCurrentMode(int mode)
{
  TSXF86VidModeSwitchToMode(display, DefaultScreen(display), modes[mode]);
  TSXF86VidModeSetViewPort(display, DefaultScreen(display), 0, 0);
  TSXSync(display, False);
}

void X11DRV_XF86VM_SetExclusiveMode(int lock)
{
  TSXF86VidModeLockModeSwitch(display, DefaultScreen(display), lock);
}

#endif /* HAVE_LIBXXF86VM */
