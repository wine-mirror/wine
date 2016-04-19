/*
 * Wine X11drv Xrandr interface
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2012 Henri Verbeet for CodeWeavers
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xrandr);
#ifdef HAVE_XRRGETSCREENRESOURCES
WINE_DECLARE_DEBUG_CHANNEL(winediag);
#endif

#ifdef SONAME_LIBXRANDR

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include "x11drv.h"

#include "wine/library.h"

static void *xrandr_handle;

#define MAKE_FUNCPTR(f) static typeof(f) * p##f;
MAKE_FUNCPTR(XRRConfigCurrentConfiguration)
MAKE_FUNCPTR(XRRConfigCurrentRate)
MAKE_FUNCPTR(XRRFreeScreenConfigInfo)
MAKE_FUNCPTR(XRRGetScreenInfo)
MAKE_FUNCPTR(XRRQueryExtension)
MAKE_FUNCPTR(XRRQueryVersion)
MAKE_FUNCPTR(XRRRates)
MAKE_FUNCPTR(XRRSetScreenConfig)
MAKE_FUNCPTR(XRRSetScreenConfigAndRate)
MAKE_FUNCPTR(XRRSizes)

#ifdef HAVE_XRRGETSCREENRESOURCES
MAKE_FUNCPTR(XRRFreeCrtcInfo)
MAKE_FUNCPTR(XRRFreeOutputInfo)
MAKE_FUNCPTR(XRRFreeScreenResources)
MAKE_FUNCPTR(XRRGetCrtcInfo)
MAKE_FUNCPTR(XRRGetOutputInfo)
MAKE_FUNCPTR(XRRGetScreenResources)
MAKE_FUNCPTR(XRRSetCrtcConfig)
static typeof(XRRGetScreenResources) *pXRRGetScreenResourcesCurrent;
static RRMode *xrandr12_modes;
static int primary_crtc;
#endif

#undef MAKE_FUNCPTR

static struct x11drv_mode_info *dd_modes;
static SizeID *xrandr10_modes;
static unsigned int xrandr_mode_count;
static int xrandr_current_mode = -1;

static int load_xrandr(void)
{
    int r = 0;

    if (wine_dlopen(SONAME_LIBXRENDER, RTLD_NOW|RTLD_GLOBAL, NULL, 0) &&
        (xrandr_handle = wine_dlopen(SONAME_LIBXRANDR, RTLD_NOW, NULL, 0)))
    {

#define LOAD_FUNCPTR(f) \
        if((p##f = wine_dlsym(xrandr_handle, #f, NULL, 0)) == NULL) \
            goto sym_not_found;

        LOAD_FUNCPTR(XRRConfigCurrentConfiguration)
        LOAD_FUNCPTR(XRRConfigCurrentRate)
        LOAD_FUNCPTR(XRRFreeScreenConfigInfo)
        LOAD_FUNCPTR(XRRGetScreenInfo)
        LOAD_FUNCPTR(XRRQueryExtension)
        LOAD_FUNCPTR(XRRQueryVersion)
        LOAD_FUNCPTR(XRRRates)
        LOAD_FUNCPTR(XRRSetScreenConfig)
        LOAD_FUNCPTR(XRRSetScreenConfigAndRate)
        LOAD_FUNCPTR(XRRSizes)
        r = 1;

#ifdef HAVE_XRRGETSCREENRESOURCES
        LOAD_FUNCPTR(XRRFreeCrtcInfo)
        LOAD_FUNCPTR(XRRFreeOutputInfo)
        LOAD_FUNCPTR(XRRFreeScreenResources)
        LOAD_FUNCPTR(XRRGetCrtcInfo)
        LOAD_FUNCPTR(XRRGetOutputInfo)
        LOAD_FUNCPTR(XRRGetScreenResources)
        LOAD_FUNCPTR(XRRSetCrtcConfig)
        r = 2;
#endif
#undef LOAD_FUNCPTR

sym_not_found:
        if (!r)  TRACE("Unable to load function ptrs from XRandR library\n");
    }
    return r;
}

static int XRandRErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

static int xrandr10_get_current_mode(void)
{
    SizeID size;
    Rotation rot;
    XRRScreenConfiguration *sc;
    short rate;
    unsigned int i;
    int res = -1;

    if (xrandr_current_mode != -1)
        return xrandr_current_mode;

    sc = pXRRGetScreenInfo (gdi_display, DefaultRootWindow( gdi_display ));
    size = pXRRConfigCurrentConfiguration (sc, &rot);
    rate = pXRRConfigCurrentRate (sc);
    pXRRFreeScreenConfigInfo(sc);

    for (i = 0; i < xrandr_mode_count; ++i)
    {
        if (xrandr10_modes[i] == size && dd_modes[i].refresh_rate == rate)
        {
            res = i;
            break;
        }
    }
    if (res == -1)
    {
        ERR("In unknown mode, returning default\n");
        return 0;
    }

    xrandr_current_mode = res;
    return res;
}

static LONG xrandr10_set_current_mode( int mode )
{
    SizeID size;
    Rotation rot;
    Window root;
    XRRScreenConfiguration *sc;
    Status stat;
    short rate;

    root = DefaultRootWindow( gdi_display );
    sc = pXRRGetScreenInfo (gdi_display, root);
    pXRRConfigCurrentConfiguration (sc, &rot);
    mode = mode % xrandr_mode_count;

    TRACE("Changing Resolution to %dx%d @%d Hz\n",
          dd_modes[mode].width,
          dd_modes[mode].height,
          dd_modes[mode].refresh_rate);

    size = xrandr10_modes[mode];
    rate = dd_modes[mode].refresh_rate;

    if (rate)
        stat = pXRRSetScreenConfigAndRate( gdi_display, sc, root, size, rot, rate, CurrentTime );
    else
        stat = pXRRSetScreenConfig( gdi_display, sc, root, size, rot, CurrentTime );

    pXRRFreeScreenConfigInfo(sc);

    if (stat == RRSetConfigSuccess)
    {
        xrandr_current_mode = mode;
        X11DRV_resize_desktop( dd_modes[mode].width, dd_modes[mode].height );
        return DISP_CHANGE_SUCCESSFUL;
    }

    ERR("Resolution change not successful -- perhaps display has changed?\n");
    return DISP_CHANGE_FAILED;
}

static void xrandr10_init_modes(void)
{
    XRRScreenSize *sizes;
    int sizes_count;
    int i, j, nmodes = 0;

    sizes = pXRRSizes( gdi_display, DefaultScreen(gdi_display), &sizes_count );
    if (sizes_count <= 0) return;

    TRACE("XRandR: found %d sizes.\n", sizes_count);
    for (i = 0; i < sizes_count; ++i)
    {
        int rates_count;
        short *rates;

        rates = pXRRRates( gdi_display, DefaultScreen(gdi_display), i, &rates_count );
        TRACE("- at %d: %dx%d (%d rates):", i, sizes[i].width, sizes[i].height, rates_count);
        if (rates_count)
        {
            nmodes += rates_count;
            for (j = 0; j < rates_count; ++j)
            {
                if (j > 0)
                    TRACE(",");
                TRACE("  %d", rates[j]);
            }
        }
        else
        {
            ++nmodes;
            TRACE(" <default>");
        }
        TRACE(" Hz\n");
    }

    TRACE("XRandR modes: count=%d\n", nmodes);

    if (!(xrandr10_modes = HeapAlloc( GetProcessHeap(), 0, sizeof(*xrandr10_modes) * nmodes )))
    {
        ERR("Failed to allocate xrandr mode info array.\n");
        return;
    }

    dd_modes = X11DRV_Settings_SetHandlers( "XRandR 1.0",
                                            xrandr10_get_current_mode,
                                            xrandr10_set_current_mode,
                                            nmodes, 1 );

    xrandr_mode_count = 0;
    for (i = 0; i < sizes_count; ++i)
    {
        int rates_count;
        short *rates;

        rates = pXRRRates( gdi_display, DefaultScreen(gdi_display), i, &rates_count );

        if (rates_count)
        {
            for (j = 0; j < rates_count; ++j)
            {
                X11DRV_Settings_AddOneMode( sizes[i].width, sizes[i].height, 0, rates[j] );
                xrandr10_modes[xrandr_mode_count++] = i;
            }
        }
        else
        {
            X11DRV_Settings_AddOneMode( sizes[i].width, sizes[i].height, 0, 0 );
            xrandr10_modes[xrandr_mode_count++] = i;
        }
    }

    X11DRV_Settings_AddDepthModes();
    nmodes = X11DRV_Settings_GetModeCount();

    TRACE("Available DD modes: count=%d\n", nmodes);
    TRACE("Enabling XRandR\n");
}

#ifdef HAVE_XRRGETSCREENRESOURCES

static int xrandr12_get_current_mode(void)
{
    XRRScreenResources *resources;
    XRRCrtcInfo *crtc_info;
    int i, ret = -1;

    if (xrandr_current_mode != -1)
        return xrandr_current_mode;

    if (!(resources = pXRRGetScreenResourcesCurrent( gdi_display, root_window )))
    {
        ERR("Failed to get screen resources.\n");
        return 0;
    }

    if (resources->ncrtc <= primary_crtc ||
        !(crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[primary_crtc] )))
    {
        pXRRFreeScreenResources( resources );
        ERR("Failed to get CRTC info.\n");
        return 0;
    }

    TRACE("CRTC %d: mode %#lx, %ux%u+%d+%d.\n", primary_crtc, crtc_info->mode,
          crtc_info->width, crtc_info->height, crtc_info->x, crtc_info->y);

    for (i = 0; i < xrandr_mode_count; ++i)
    {
        if (xrandr12_modes[i] == crtc_info->mode)
        {
            ret = i;
            break;
        }
    }

    pXRRFreeCrtcInfo( crtc_info );
    pXRRFreeScreenResources( resources );

    if (ret == -1)
    {
        ERR("Unknown mode, returning default.\n");
        return 0;
    }

    xrandr_current_mode = ret;
    return ret;
}

static LONG xrandr12_set_current_mode( int mode )
{
    Status status = RRSetConfigFailed;
    XRRScreenResources *resources;
    XRRCrtcInfo *crtc_info;

    mode = mode % xrandr_mode_count;

    if (!(resources = pXRRGetScreenResourcesCurrent( gdi_display, root_window )))
    {
        ERR("Failed to get screen resources.\n");
        return DISP_CHANGE_FAILED;
    }

    if (resources->ncrtc <= primary_crtc ||
        !(crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[primary_crtc] )))
    {
        pXRRFreeScreenResources( resources );
        ERR("Failed to get CRTC info.\n");
        return DISP_CHANGE_FAILED;
    }

    TRACE("CRTC %d: mode %#lx, %ux%u+%d+%d.\n", primary_crtc, crtc_info->mode,
          crtc_info->width, crtc_info->height, crtc_info->x, crtc_info->y);

    status = pXRRSetCrtcConfig( gdi_display, resources, resources->crtcs[primary_crtc],
                                CurrentTime, crtc_info->x, crtc_info->y, xrandr12_modes[mode],
                                crtc_info->rotation, crtc_info->outputs, crtc_info->noutput );

    pXRRFreeCrtcInfo( crtc_info );
    pXRRFreeScreenResources( resources );

    if (status != RRSetConfigSuccess)
    {
        ERR("Resolution change not successful -- perhaps display has changed?\n");
        return DISP_CHANGE_FAILED;
    }

    xrandr_current_mode = mode;
    X11DRV_resize_desktop( dd_modes[mode].width, dd_modes[mode].height );
    return DISP_CHANGE_SUCCESSFUL;
}

static XRRCrtcInfo *xrandr12_get_primary_crtc_info( XRRScreenResources *resources, int *crtc_idx )
{
    XRRCrtcInfo *crtc_info;
    int i;

    for (i = 0; i < resources->ncrtc; ++i)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[i] );
        if (!crtc_info || crtc_info->mode == None)
        {
            pXRRFreeCrtcInfo( crtc_info );
            continue;
        }

        *crtc_idx = i;
        return crtc_info;
    }

    return NULL;
}

static int xrandr12_init_modes(void)
{
    unsigned int only_one_resolution = 1, mode_count;
    XRRScreenResources *resources;
    XRROutputInfo *output_info;
    XRRCrtcInfo *crtc_info;
    int ret = -1;
    int i, j;

    if (!(resources = pXRRGetScreenResourcesCurrent( gdi_display, root_window )))
    {
        ERR("Failed to get screen resources.\n");
        return ret;
    }

    if (!resources->ncrtc)
    {
        pXRRFreeScreenResources( resources );
        if (!(resources = pXRRGetScreenResources( gdi_display, root_window )))
        {
            ERR("Failed to get screen resources.\n");
            return ret;
        }
    }

    if (!(crtc_info = xrandr12_get_primary_crtc_info( resources, &primary_crtc )))
    {
        pXRRFreeScreenResources( resources );
        ERR("Failed to get primary CRTC info.\n");
        return ret;
    }

    TRACE("CRTC %d: mode %#lx, %ux%u+%d+%d.\n", primary_crtc, crtc_info->mode,
          crtc_info->width, crtc_info->height, crtc_info->x, crtc_info->y);

    if (!crtc_info->noutput || !(output_info = pXRRGetOutputInfo( gdi_display, resources, crtc_info->outputs[0] )))
    {
        pXRRFreeCrtcInfo( crtc_info );
        pXRRFreeScreenResources( resources );
        ERR("Failed to get output info.\n");
        return ret;
    }

    TRACE("OUTPUT 0: name %s.\n", debugstr_a(output_info->name));

    if (!output_info->nmode)
    {
        WARN("Output has no modes.\n");
        goto done;
    }

    if (!(xrandr12_modes = HeapAlloc( GetProcessHeap(), 0, sizeof(*xrandr12_modes) * output_info->nmode )))
    {
        ERR("Failed to allocate xrandr mode info array.\n");
        goto done;
    }

    dd_modes = X11DRV_Settings_SetHandlers( "XRandR 1.2",
                                            xrandr12_get_current_mode,
                                            xrandr12_set_current_mode,
                                            output_info->nmode, 1 );

    xrandr_mode_count = 0;
    for (i = 0; i < output_info->nmode; ++i)
    {
        for (j = 0; j < resources->nmode; ++j)
        {
            XRRModeInfo *mode = &resources->modes[j];

            if (mode->id == output_info->modes[i])
            {
                unsigned int dots = mode->hTotal * mode->vTotal;
                unsigned int refresh = dots ? (mode->dotClock + dots / 2) / dots : 0;

                TRACE("Adding mode %#lx: %ux%u@%u.\n", mode->id, mode->width, mode->height, refresh);
                X11DRV_Settings_AddOneMode( mode->width, mode->height, 0, refresh );
                xrandr12_modes[xrandr_mode_count++] = mode->id;
                break;
            }
        }
    }

    mode_count = X11DRV_Settings_GetModeCount();
    for (i = 1; i < mode_count; ++i)
    {
        if (dd_modes[i].width != dd_modes[0].width || dd_modes[i].height != dd_modes[0].height)
        {
            only_one_resolution = 0;
            break;
        }
    }

    /* Recent (304.64, possibly earlier) versions of the nvidia driver only
     * report a DFP's native mode through RandR 1.2 / 1.3. Standard DMT modes
     * are only listed through RandR 1.0 / 1.1. This is completely useless,
     * but NVIDIA considers this a feature, so it's unlikely to change. The
     * best we can do is to fall back to RandR 1.0 and encourage users to
     * consider more cooperative driver vendors when we detect such a
     * configuration. */
    if (only_one_resolution && XQueryExtension( gdi_display, "NV-CONTROL", &i, &j, &ret ))
    {
        ERR_(winediag)("Broken NVIDIA RandR detected, falling back to RandR 1.0. "
                       "Please consider using the Nouveau driver instead.\n");
        ret = -1;
        HeapFree( GetProcessHeap(), 0, xrandr12_modes );
        goto done;
    }

    X11DRV_Settings_AddDepthModes();
    ret = 0;

done:
    pXRRFreeOutputInfo( output_info );
    pXRRFreeCrtcInfo( crtc_info );
    pXRRFreeScreenResources( resources );
    return ret;
}

#endif /* HAVE_XRRGETSCREENRESOURCES */

void X11DRV_XRandR_Init(void)
{
    int event_base, error_base, minor, ret;
    static int major;
    Bool ok;

    if (major) return; /* already initialized? */
    if (!usexrandr) return; /* disabled in config */
    if (root_window != DefaultRootWindow( gdi_display )) return;
    if (!(ret = load_xrandr())) return;  /* can't load the Xrandr library */

    /* see if Xrandr is available */
    if (!pXRRQueryExtension( gdi_display, &event_base, &error_base )) return;
    X11DRV_expect_error( gdi_display, XRandRErrorHandler, NULL );
    ok = pXRRQueryVersion( gdi_display, &major, &minor );
    if (X11DRV_check_error() || !ok) return;

    TRACE("Found XRandR %d.%d.\n", major, minor);

#ifdef HAVE_XRRGETSCREENRESOURCES
    if (ret >= 2 && (major > 1 || (major == 1 && minor >= 2)))
    {
        if (major > 1 || (major == 1 && minor >= 3))
            pXRRGetScreenResourcesCurrent = wine_dlsym( xrandr_handle, "XRRGetScreenResourcesCurrent", NULL, 0 );
        if (!pXRRGetScreenResourcesCurrent)
            pXRRGetScreenResourcesCurrent = pXRRGetScreenResources;
    }

    if (!pXRRGetScreenResourcesCurrent || xrandr12_init_modes() < 0)
#endif
        xrandr10_init_modes();
}

#else /* SONAME_LIBXRANDR */

void X11DRV_XRandR_Init(void)
{
    TRACE("XRandR support not compiled in.\n");
}

#endif /* SONAME_LIBXRANDR */
