/*
 * DirectDraw XVidMode interface
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
#include "wine/port.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "x11drv.h"

#ifdef HAVE_X11_EXTENSIONS_XF86VMODE_H
#include <X11/extensions/xf86vmode.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XF86VMPROTO_H
#include <X11/extensions/xf86vmproto.h>
#endif

#include "windef.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(xvidmode);

#ifdef SONAME_LIBXXF86VM

extern BOOL usexvidmode;

static int xf86vm_event, xf86vm_error, xf86vm_major, xf86vm_minor;

#ifdef X_XF86VidModeSetGammaRamp
static int xf86vm_gammaramp_size;
static BOOL xf86vm_use_gammaramp;
#endif /* X_XF86VidModeSetGammaRamp */

static struct x11drv_mode_info *dd_modes;
static unsigned int dd_mode_count;
static XF86VidModeModeInfo** real_xf86vm_modes;
static unsigned int real_xf86vm_mode_count;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XF86VidModeGetAllModeLines)
MAKE_FUNCPTR(XF86VidModeGetModeLine)
MAKE_FUNCPTR(XF86VidModeLockModeSwitch)
MAKE_FUNCPTR(XF86VidModeQueryExtension)
MAKE_FUNCPTR(XF86VidModeQueryVersion)
MAKE_FUNCPTR(XF86VidModeSetViewPort)
MAKE_FUNCPTR(XF86VidModeSwitchToMode)
#ifdef X_XF86VidModeSetGamma
MAKE_FUNCPTR(XF86VidModeGetGamma)
MAKE_FUNCPTR(XF86VidModeSetGamma)
#endif
#ifdef X_XF86VidModeSetGammaRamp
MAKE_FUNCPTR(XF86VidModeGetGammaRamp)
MAKE_FUNCPTR(XF86VidModeGetGammaRampSize)
MAKE_FUNCPTR(XF86VidModeSetGammaRamp)
#endif
#undef MAKE_FUNCPTR


static void convert_modeinfo( const XF86VidModeModeInfo *mode)
{
  int rate;
  if (mode->htotal!=0 && mode->vtotal!=0)
      rate = mode->dotclock * 1000 / (mode->htotal * mode->vtotal);
  else
      rate = 0;
  X11DRV_Settings_AddOneMode(mode->hdisplay, mode->vdisplay, 0, rate);
}

static void convert_modeline(int dotclock, const XF86VidModeModeLine *mode,
                             struct x11drv_mode_info *info, unsigned int bpp)
{
  info->width   = mode->hdisplay;
  info->height  = mode->vdisplay;
  if (mode->htotal!=0 && mode->vtotal!=0)
      info->refresh_rate = dotclock * 1000 / (mode->htotal * mode->vtotal);
  else
      info->refresh_rate = 0;
  TRACE(" width=%d, height=%d, refresh=%d\n",
        info->width, info->height, info->refresh_rate);
  info->bpp     = bpp;
}

static int XVidModeErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

static int X11DRV_XF86VM_GetCurrentMode(void)
{
  XF86VidModeModeLine line;
  int dotclock;
  unsigned int i;
  struct x11drv_mode_info cmode;
  DWORD dwBpp = screen_bpp;

  TRACE("Querying XVidMode current mode\n");
  pXF86VidModeGetModeLine(gdi_display, DefaultScreen(gdi_display), &dotclock, &line);
  convert_modeline(dotclock, &line, &cmode, dwBpp);
  for (i=0; i<dd_mode_count; i++)
    if (memcmp(&dd_modes[i], &cmode, sizeof(cmode)) == 0) {
      TRACE("mode=%d\n", i);
      return i;
    }
  ERR("In unknown mode, returning default\n");
  return 0;
}

static LONG X11DRV_XF86VM_SetCurrentMode(int mode)
{
  DWORD dwBpp = screen_bpp;
  /* only set modes from the original color depth */
  if (dwBpp != dd_modes[mode].bpp)
  {
      FIXME("Cannot change screen BPP from %d to %d\n", dwBpp, dd_modes[mode].bpp);
  }
  mode = mode % real_xf86vm_mode_count;

  TRACE("Resizing X display to %dx%d\n", 
        real_xf86vm_modes[mode]->hdisplay, real_xf86vm_modes[mode]->vdisplay);
  pXF86VidModeSwitchToMode(gdi_display, DefaultScreen(gdi_display), real_xf86vm_modes[mode]);
#if 0 /* it is said that SetViewPort causes problems with some X servers */
  pXF86VidModeSetViewPort(gdi_display, DefaultScreen(gdi_display), 0, 0);
#else
  XWarpPointer(gdi_display, None, DefaultRootWindow(gdi_display), 0, 0, 0, 0, 0, 0);
#endif
  XSync(gdi_display, False);
  X11DRV_resize_desktop( real_xf86vm_modes[mode]->hdisplay, real_xf86vm_modes[mode]->vdisplay );
  return DISP_CHANGE_SUCCESSFUL;
}


void X11DRV_XF86VM_Init(void)
{
  void *xvidmode_handle;
  Bool ok;
  int nmodes;
  unsigned int i;

  if (xf86vm_major) return; /* already initialized? */

  xvidmode_handle = wine_dlopen(SONAME_LIBXXF86VM, RTLD_NOW, NULL, 0);
  if (!xvidmode_handle)
  {
    TRACE("Unable to open %s, XVidMode disabled\n", SONAME_LIBXXF86VM);
    usexvidmode = FALSE;
    return;
  }

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(xvidmode_handle, #f, NULL, 0)) == NULL) \
        goto sym_not_found;
    LOAD_FUNCPTR(XF86VidModeGetAllModeLines)
    LOAD_FUNCPTR(XF86VidModeGetModeLine)
    LOAD_FUNCPTR(XF86VidModeLockModeSwitch)
    LOAD_FUNCPTR(XF86VidModeQueryExtension)
    LOAD_FUNCPTR(XF86VidModeQueryVersion)
    LOAD_FUNCPTR(XF86VidModeSetViewPort)
    LOAD_FUNCPTR(XF86VidModeSwitchToMode)
#ifdef X_XF86VidModeSetGamma
    LOAD_FUNCPTR(XF86VidModeGetGamma)
    LOAD_FUNCPTR(XF86VidModeSetGamma)
#endif
#ifdef X_XF86VidModeSetGammaRamp
    LOAD_FUNCPTR(XF86VidModeGetGammaRamp)
    LOAD_FUNCPTR(XF86VidModeGetGammaRampSize)
    LOAD_FUNCPTR(XF86VidModeSetGammaRamp)
#endif
#undef LOAD_FUNCPTR

  /* see if XVidMode is available */
  if (!pXF86VidModeQueryExtension(gdi_display, &xf86vm_event, &xf86vm_error)) return;

  X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
  ok = pXF86VidModeQueryVersion(gdi_display, &xf86vm_major, &xf86vm_minor);
  if (X11DRV_check_error() || !ok) return;

#ifdef X_XF86VidModeSetGammaRamp
  if (xf86vm_major > 2 || (xf86vm_major == 2 && xf86vm_minor >= 1))
  {
      X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
      pXF86VidModeGetGammaRampSize(gdi_display, DefaultScreen(gdi_display),
                                   &xf86vm_gammaramp_size);
      if (X11DRV_check_error()) xf86vm_gammaramp_size = 0;
      TRACE("Gamma ramp size %d.\n", xf86vm_gammaramp_size);
      if (xf86vm_gammaramp_size >= GAMMA_RAMP_SIZE)
          xf86vm_use_gammaramp = TRUE;
  }
#endif /* X_XF86VidModeSetGammaRamp */

  /* retrieve modes */
  if (usexvidmode && root_window == DefaultRootWindow( gdi_display ))
  {
      X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
      ok = pXF86VidModeGetAllModeLines(gdi_display, DefaultScreen(gdi_display), &nmodes, &real_xf86vm_modes);
      if (X11DRV_check_error() || !ok) return;
  }
  else return; /* In desktop mode, do not switch resolution... But still use the Gamma ramp stuff */

  TRACE("XVidMode modes: count=%d\n", nmodes);

  real_xf86vm_mode_count = nmodes;

  dd_modes = X11DRV_Settings_SetHandlers("XF86VidMode", 
                                         X11DRV_XF86VM_GetCurrentMode, 
                                         X11DRV_XF86VM_SetCurrentMode, 
                                         nmodes, 1);

  /* convert modes to x11drv_mode_info format */
  for (i=0; i<real_xf86vm_mode_count; i++)
  {
      convert_modeinfo(real_xf86vm_modes[i]);
  }
  /* add modes for different color depths */
  X11DRV_Settings_AddDepthModes();
  dd_mode_count = X11DRV_Settings_GetModeCount();

  TRACE("Available DD modes: count=%d\n", dd_mode_count);
  TRACE("Enabling XVidMode\n");
  return;

sym_not_found:
    TRACE("Unable to load function pointers from %s, XVidMode disabled\n", SONAME_LIBXXF86VM);
    wine_dlclose(xvidmode_handle, NULL, 0);
    xvidmode_handle = NULL;
    usexvidmode = FALSE;
}

/***** GAMMA CONTROL *****/
/* (only available in XF86VidMode 2.x) */

#ifdef X_XF86VidModeSetGamma

static void GenerateRampFromGamma(WORD ramp[GAMMA_RAMP_SIZE], float gamma)
{
  float r_gamma = 1/gamma;
  unsigned i;
  TRACE("gamma is %f\n", r_gamma);
  for (i=0; i<GAMMA_RAMP_SIZE; i++)
    ramp[i] = pow(i/255.0, r_gamma) * 65535.0;
}

static BOOL ComputeGammaFromRamp(WORD ramp[GAMMA_RAMP_SIZE], float *gamma)
{
  float r_x, r_y, r_lx, r_ly, r_d, r_v, r_e, g_avg, g_min, g_max;
  unsigned i, f, l, g_n, c;
  f = ramp[0];
  l = ramp[255];
  if (f >= l) {
    ERR("inverted or flat gamma ramp (%d->%d), rejected\n", f, l);
    return FALSE;
  }
  r_d = l - f;
  g_min = g_max = g_avg = 0.0;
  /* check gamma ramp entries to estimate the gamma */
  TRACE("analyzing gamma ramp (%d->%d)\n", f, l);
  for (i=1, g_n=0; i<255; i++) {
    if (ramp[i] < f || ramp[i] > l) {
      ERR("strange gamma ramp ([%d]=%d for %d->%d), rejected\n", i, ramp[i], f, l);
      return FALSE;
    }
    c = ramp[i] - f;
    if (!c) continue; /* avoid log(0) */

    /* normalize entry values into 0..1 range */
    r_x = i/255.0; r_y = c / r_d;
    /* compute logarithms of values */
    r_lx = log(r_x); r_ly = log(r_y);
    /* compute gamma for this entry */
    r_v = r_ly / r_lx;
    /* compute differential (error estimate) for this entry */
    /* some games use table-based logarithms that magnifies the error by 128 */
    r_e = -r_lx * 128 / (c * r_lx * r_lx);

    /* compute min & max while compensating for estimated error */
    if (!g_n || g_min > (r_v + r_e)) g_min = r_v + r_e;
    if (!g_n || g_max < (r_v - r_e)) g_max = r_v - r_e;

    /* add to average */
    g_avg += r_v;
    g_n++;
    /* TRACE("[%d]=%d, gamma=%f, error=%f\n", i, ramp[i], r_v, r_e); */
  }
  if (!g_n) {
    ERR("no gamma data, shouldn't happen\n");
    return FALSE;
  }
  g_avg /= g_n;
  TRACE("low bias is %d, high is %d, gamma is %5.3f\n", f, 65535-l, g_avg);
  /* the bias could be because the app wanted something like a "red shift"
   * like when you're hit in Quake, but XVidMode doesn't support it,
   * so we have to reject a significant bias */
  if (f && f > (pow(1/255.0, g_avg) * 65536.0)) {
    ERR("low-biased gamma ramp (%d), rejected\n", f);
    return FALSE;
  }
  /* check that the gamma is reasonably uniform across the ramp */
  if (g_max - g_min > 12.8) {
    ERR("ramp not uniform (max=%f, min=%f, avg=%f), rejected\n", g_max, g_min, g_avg);
    return FALSE;
  }
  /* check that the gamma is not too bright */
  if (g_avg < 0.2) {
    ERR("too bright gamma ( %5.3f), rejected\n", g_avg);
    return FALSE;
  }
  /* ok, now we're pretty sure we can set the desired gamma ramp,
   * so go for it */
  *gamma = 1/g_avg;
  return TRUE;
}

#endif /* X_XF86VidModeSetGamma */

/* Hmm... should gamma control be available in desktop mode or not?
 * I'll assume that it should */

#ifdef X_XF86VidModeSetGammaRamp
static void interpolate_gamma_ramp(WORD *dst_r, WORD *dst_g, WORD *dst_b, unsigned int dst_size,
        const WORD *src_r, const WORD *src_g, const WORD *src_b, unsigned int src_size)
{
    double position, distance;
    unsigned int dst_i, src_i;

    for (dst_i = 0; dst_i < dst_size; ++dst_i)
    {
        position = dst_i * (src_size - 1) / (double)(dst_size - 1);
        src_i = position;

        if (src_i + 1 < src_size)
        {
            distance = position - src_i;

            dst_r[dst_i] = (1.0 - distance) * src_r[src_i] + distance * src_r[src_i + 1] + 0.5;
            dst_g[dst_i] = (1.0 - distance) * src_g[src_i] + distance * src_g[src_i + 1] + 0.5;
            dst_b[dst_i] = (1.0 - distance) * src_b[src_i] + distance * src_b[src_i + 1] + 0.5;
        }
        else
        {
            dst_r[dst_i] = src_r[src_i];
            dst_g[dst_i] = src_g[src_i];
            dst_b[dst_i] = src_b[src_i];
        }
    }
}

static BOOL xf86vm_get_gamma_ramp(struct x11drv_gamma_ramp *ramp)
{
    WORD *red, *green, *blue;
    BOOL ret;

    if (xf86vm_gammaramp_size == GAMMA_RAMP_SIZE)
    {
        red = ramp->red;
        green = ramp->green;
        blue = ramp->blue;
    }
    else
    {
        if (!(red = heap_calloc(xf86vm_gammaramp_size, 3 * sizeof(*red))))
            return FALSE;
        green = red + xf86vm_gammaramp_size;
        blue = green + xf86vm_gammaramp_size;
    }

    ret = pXF86VidModeGetGammaRamp(gdi_display, DefaultScreen(gdi_display),
                                   xf86vm_gammaramp_size, red, green, blue);
    if (ret && red != ramp->red)
        interpolate_gamma_ramp(ramp->red, ramp->green, ramp->blue, GAMMA_RAMP_SIZE,
                               red, green, blue, xf86vm_gammaramp_size);
    if (red != ramp->red)
        heap_free(red);
    return ret;
}

static BOOL xf86vm_set_gamma_ramp(struct x11drv_gamma_ramp *ramp)
{
    WORD *red, *green, *blue;
    BOOL ret;

    if (xf86vm_gammaramp_size == GAMMA_RAMP_SIZE)
    {
        red = ramp->red;
        green = ramp->green;
        blue = ramp->blue;
    }
    else
    {
        if (!(red = heap_calloc(xf86vm_gammaramp_size, 3 * sizeof(*red))))
            return FALSE;
        green = red + xf86vm_gammaramp_size;
        blue = green + xf86vm_gammaramp_size;

        interpolate_gamma_ramp(red, green, blue, xf86vm_gammaramp_size,
                               ramp->red, ramp->green, ramp->blue, GAMMA_RAMP_SIZE);
    }

    ret = pXF86VidModeSetGammaRamp(gdi_display, DefaultScreen(gdi_display),
                                   xf86vm_gammaramp_size, red, green, blue);
    if (red != ramp->red)
        heap_free(red);
    return ret;
}
#endif

static BOOL X11DRV_XF86VM_GetGammaRamp(struct x11drv_gamma_ramp *ramp)
{
#ifdef X_XF86VidModeSetGamma
  XF86VidModeGamma gamma;

  if (xf86vm_major < 2) return FALSE; /* no gamma control */
#ifdef X_XF86VidModeSetGammaRamp
  if (xf86vm_use_gammaramp)
      return xf86vm_get_gamma_ramp(ramp);
#endif
  if (pXF86VidModeGetGamma(gdi_display, DefaultScreen(gdi_display), &gamma))
  {
      GenerateRampFromGamma(ramp->red,   gamma.red);
      GenerateRampFromGamma(ramp->green, gamma.green);
      GenerateRampFromGamma(ramp->blue,  gamma.blue);
      return TRUE;
  }
#endif /* X_XF86VidModeSetGamma */
  return FALSE;
}

static BOOL X11DRV_XF86VM_SetGammaRamp(struct x11drv_gamma_ramp *ramp)
{
#ifdef X_XF86VidModeSetGamma
  XF86VidModeGamma gamma;

  if (xf86vm_major < 2 || !usexvidmode) return FALSE; /* no gamma control */
  if (!ComputeGammaFromRamp(ramp->red,   &gamma.red) || /* ramp validation */
      !ComputeGammaFromRamp(ramp->green, &gamma.green) ||
      !ComputeGammaFromRamp(ramp->blue,  &gamma.blue)) return FALSE;
#ifdef X_XF86VidModeSetGammaRamp
  if (xf86vm_use_gammaramp)
      return xf86vm_set_gamma_ramp(ramp);
#endif
  return pXF86VidModeSetGamma(gdi_display, DefaultScreen(gdi_display), &gamma);
#else
  return FALSE;
#endif /* X_XF86VidModeSetGamma */
}

#else /* SONAME_LIBXXF86VM */

void X11DRV_XF86VM_Init(void)
{
    TRACE("XVidMode support not compiled in.\n");
}

#endif /* SONAME_LIBXXF86VM */

/***********************************************************************
 *		GetDeviceGammaRamp (X11DRV.@)
 *
 * FIXME: should move to somewhere appropriate, but probably not before
 * the stuff in graphics/x11drv/ has been moved to dlls/x11drv, so that
 * they can include xvidmode.h directly
 */
BOOL CDECL X11DRV_GetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
#ifdef SONAME_LIBXXF86VM
  return X11DRV_XF86VM_GetGammaRamp(ramp);
#else
  return FALSE;
#endif
}

/***********************************************************************
 *		SetDeviceGammaRamp (X11DRV.@)
 *
 * FIXME: should move to somewhere appropriate, but probably not before
 * the stuff in graphics/x11drv/ has been moved to dlls/x11drv, so that
 * they can include xvidmode.h directly
 */
BOOL CDECL X11DRV_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
#ifdef SONAME_LIBXXF86VM
  return X11DRV_XF86VM_SetGammaRamp(ramp);
#else
  return FALSE;
#endif
}
