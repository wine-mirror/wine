/*
 * DirectDraw XVidMode interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 * Copyright 2020 Zhiyi Zhang for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <dlfcn.h>

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

WINE_DEFAULT_DEBUG_CHANNEL(xvidmode);

#ifdef SONAME_LIBXXF86VM

extern BOOL usexvidmode;

static int xf86vm_event, xf86vm_error, xf86vm_major, xf86vm_minor;

#ifdef X_XF86VidModeSetGammaRamp
static int xf86vm_gammaramp_size;
static BOOL xf86vm_use_gammaramp;
#endif /* X_XF86VidModeSetGammaRamp */

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

static int XVidModeErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

/* XF86VidMode display settings handler */
static BOOL xf86vm_get_id(const WCHAR *device_name, BOOL is_primary, x11drv_settings_id *id)
{
    /* XVidMode only supports changing the primary adapter settings.
     * For non-primary adapters, an id is still provided but getting
     * and changing non-primary adapters' settings will be ignored. */
    id->id = is_primary ? 1 : 0;
    return TRUE;
}

static void add_xf86vm_mode( DEVMODEW *mode, DWORD depth, const XF86VidModeModeInfo *mode_info, BOOL full )
{
    mode->dmSize = sizeof(*mode);
    mode->dmDriverExtra = full ? sizeof(mode_info) : 0;
    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS;
    if (mode_info->htotal && mode_info->vtotal)
    {
        mode->dmFields |= DM_DISPLAYFREQUENCY;
        mode->dmDisplayFrequency = mode_info->dotclock * 1000 / (mode_info->htotal * mode_info->vtotal);
    }
    mode->dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmBitsPerPel = depth;
    mode->dmPelsWidth = mode_info->hdisplay;
    mode->dmPelsHeight = mode_info->vdisplay;
    mode->dmDisplayFlags = 0;
    if (full) memcpy( mode + 1, &mode_info, sizeof(mode_info) );
}

static BOOL xf86vm_get_modes( x11drv_settings_id id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count, BOOL full )
{
    INT xf86vm_mode_idx, xf86vm_mode_count;
    XF86VidModeModeInfo **xf86vm_modes;
    UINT depth_idx, mode_idx = 0;
    DEVMODEW *modes, *mode;
    SIZE_T size;
    BYTE *ptr;
    Bool ret;

    X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
    ret = pXF86VidModeGetAllModeLines(gdi_display, DefaultScreen(gdi_display), &xf86vm_mode_count, &xf86vm_modes);
    if (X11DRV_check_error() || !ret || !xf86vm_mode_count)
        return FALSE;

    /* Put a XF86VidModeModeInfo ** at the start to store the XF86VidMode modes pointer */
    size = sizeof(XF86VidModeModeInfo **);
    /* Display modes in different color depth, with a XF86VidModeModeInfo * at the end of each
     * DEVMODEW as driver private data */
    size += (xf86vm_mode_count * DEPTH_COUNT) * (sizeof(DEVMODEW) + sizeof(XF86VidModeModeInfo *));
    ptr = calloc(1, size);
    if (!ptr)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    memcpy(ptr, &xf86vm_modes, sizeof(xf86vm_modes));
    modes = (DEVMODEW *)(ptr + sizeof(xf86vm_modes));

    for (depth_idx = 0, mode = modes; depth_idx < DEPTH_COUNT; ++depth_idx)
    {
        for (xf86vm_mode_idx = 0; xf86vm_mode_idx < xf86vm_mode_count; ++xf86vm_mode_idx)
        {
            add_xf86vm_mode( mode, depths[depth_idx], xf86vm_modes[xf86vm_mode_idx], full );
            mode = NEXT_DEVMODEW( mode );
            mode_idx++;
        }
    }

    *new_modes = modes;
    *mode_count = mode_idx;
    return TRUE;
}

static void xf86vm_free_modes(DEVMODEW *modes)
{
    XF86VidModeModeInfo **xf86vm_modes;

    if (modes)
    {
        BYTE *ptr = (BYTE *)modes - sizeof(xf86vm_modes);
        memcpy(&xf86vm_modes, ptr, sizeof(xf86vm_modes));
        XFree(xf86vm_modes);
        free(ptr);
    }
}

static BOOL xf86vm_get_current_mode(x11drv_settings_id id, DEVMODEW *mode)
{
    XF86VidModeModeLine xf86vm_mode;
    INT dotclock;
    Bool ret;

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmDisplayFlags = 0;
    mode->dmPosition.x = 0;
    mode->dmPosition.y = 0;

    if (id.id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        mode->dmBitsPerPel = 0;
        mode->dmPelsWidth = 0;
        mode->dmPelsHeight = 0;
        mode->dmDisplayFrequency = 0;
        return TRUE;
    }

    X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
    ret = pXF86VidModeGetModeLine(gdi_display, DefaultScreen(gdi_display), &dotclock, &xf86vm_mode);
    if (X11DRV_check_error() || !ret)
        return FALSE;

    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = xf86vm_mode.hdisplay;
    mode->dmPelsHeight = xf86vm_mode.vdisplay;
    if (xf86vm_mode.htotal && xf86vm_mode.vtotal)
        mode->dmDisplayFrequency = dotclock * 1000 / (xf86vm_mode.htotal * xf86vm_mode.vtotal);
    else
        mode->dmDisplayFrequency = 0;

    if (xf86vm_mode.privsize)
        XFree(xf86vm_mode.private);
    return TRUE;
}

static LONG xf86vm_set_current_mode(x11drv_settings_id id, const DEVMODEW *mode)
{
    XF86VidModeModeInfo *xf86vm_mode;
    Bool ret;

    if (id.id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (is_detached_mode(mode))
    {
        FIXME("Detaching adapters is unsupported.\n");
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (mode->dmFields & DM_BITSPERPEL && mode->dmBitsPerPel != screen_bpp)
        WARN("Cannot change screen bit depth from %dbits to %dbits!\n",
             screen_bpp, mode->dmBitsPerPel);

    assert(mode->dmDriverExtra == sizeof(XF86VidModeModeInfo *));
    memcpy(&xf86vm_mode, (BYTE *)mode + sizeof(*mode), sizeof(xf86vm_mode));
    X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
    ret = pXF86VidModeSwitchToMode(gdi_display, DefaultScreen(gdi_display), xf86vm_mode);
    if (X11DRV_check_error() || !ret)
        return DISP_CHANGE_FAILED;
#if 0 /* it is said that SetViewPort causes problems with some X servers */
    pXF86VidModeSetViewPort(gdi_display, DefaultScreen(gdi_display), 0, 0);
#else
    XWarpPointer(gdi_display, None, DefaultRootWindow(gdi_display), 0, 0, 0, 0, 0, 0);
#endif
    XFlush(gdi_display);
    return DISP_CHANGE_SUCCESSFUL;
}

void X11DRV_XF86VM_Init(void)
{
  struct x11drv_settings_handler xf86vm_handler;
  void *xvidmode_handle;
  Bool ok;

  if (xf86vm_major) return; /* already initialized? */

  xvidmode_handle = dlopen(SONAME_LIBXXF86VM, RTLD_NOW);
  if (!xvidmode_handle)
  {
    TRACE("Unable to open %s, XVidMode disabled\n", SONAME_LIBXXF86VM);
    usexvidmode = FALSE;
    return;
  }

#define LOAD_FUNCPTR(f) \
    if((p##f = dlsym(xvidmode_handle, #f)) == NULL) goto sym_not_found
    LOAD_FUNCPTR(XF86VidModeGetAllModeLines);
    LOAD_FUNCPTR(XF86VidModeGetModeLine);
    LOAD_FUNCPTR(XF86VidModeLockModeSwitch);
    LOAD_FUNCPTR(XF86VidModeQueryExtension);
    LOAD_FUNCPTR(XF86VidModeQueryVersion);
    LOAD_FUNCPTR(XF86VidModeSetViewPort);
    LOAD_FUNCPTR(XF86VidModeSwitchToMode);
#ifdef X_XF86VidModeSetGamma
    LOAD_FUNCPTR(XF86VidModeGetGamma);
    LOAD_FUNCPTR(XF86VidModeSetGamma);
#endif
#ifdef X_XF86VidModeSetGammaRamp
    LOAD_FUNCPTR(XF86VidModeGetGammaRamp);
    LOAD_FUNCPTR(XF86VidModeGetGammaRampSize);
    LOAD_FUNCPTR(XF86VidModeSetGammaRamp);
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

  if (!usexvidmode)
    return;

  xf86vm_handler.name = "XF86VidMode";
  xf86vm_handler.priority = 100;
  xf86vm_handler.get_id = xf86vm_get_id;
  xf86vm_handler.get_modes = xf86vm_get_modes;
  xf86vm_handler.free_modes = xf86vm_free_modes;
  xf86vm_handler.get_current_mode = xf86vm_get_current_mode;
  xf86vm_handler.set_current_mode = xf86vm_set_current_mode;
  X11DRV_Settings_SetHandler(&xf86vm_handler);
  return;

sym_not_found:
    TRACE("Unable to load function pointers from %s, XVidMode disabled\n", SONAME_LIBXXF86VM);
    dlclose(xvidmode_handle);
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
        if (!(red = calloc(xf86vm_gammaramp_size, 3 * sizeof(*red))))
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
        free(red);
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
        if (!(red = calloc(xf86vm_gammaramp_size, 3 * sizeof(*red))))
            return FALSE;
        green = red + xf86vm_gammaramp_size;
        blue = green + xf86vm_gammaramp_size;

        interpolate_gamma_ramp(red, green, blue, xf86vm_gammaramp_size,
                               ramp->red, ramp->green, ramp->blue, GAMMA_RAMP_SIZE);
    }

    X11DRV_expect_error(gdi_display, XVidModeErrorHandler, NULL);
    ret = pXF86VidModeSetGammaRamp(gdi_display, DefaultScreen(gdi_display),
                                   xf86vm_gammaramp_size, red, green, blue);
    if (ret) XSync( gdi_display, FALSE );
    if (X11DRV_check_error()) ret = FALSE;

    if (red != ramp->red)
        free(red);
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
 *		GetDeviceGammaRamp
 */
BOOL X11DRV_GetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
#ifdef SONAME_LIBXXF86VM
  return X11DRV_XF86VM_GetGammaRamp(ramp);
#else
  return FALSE;
#endif
}

/***********************************************************************
 *		SetDeviceGammaRamp
 */
BOOL X11DRV_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
#ifdef SONAME_LIBXXF86VM
  return X11DRV_XF86VM_SetGammaRamp(ramp);
#else
  return FALSE;
#endif
}
