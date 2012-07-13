/*
 * Wine X11drv Xrandr interface
 *
 * Copyright 2003 Alexander James Pasadyn
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
#undef MAKE_FUNCPTR

extern int usexrandr;

static struct x11drv_mode_info *dd_modes;
static SizeID *xrandr_modes;
static unsigned int xrandr_mode_count;

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

#undef LOAD_FUNCPTR

        r = 1;   /* success */

sym_not_found:
        if (!r)  TRACE("Unable to load function ptrs from XRandR library\n");
    }
    return r;
}

static int XRandRErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}

static int X11DRV_XRandR_GetCurrentMode(void)
{
    SizeID size;
    Rotation rot;
    Window root;
    XRRScreenConfiguration *sc;
    short rate;
    unsigned int i;
    int res = -1;
    
    wine_tsx11_lock();
    root = RootWindow (gdi_display, DefaultScreen(gdi_display));
    sc = pXRRGetScreenInfo (gdi_display, root);
    size = pXRRConfigCurrentConfiguration (sc, &rot);
    rate = pXRRConfigCurrentRate (sc);
    pXRRFreeScreenConfigInfo(sc);
    wine_tsx11_unlock();
    for (i = 0; i < xrandr_mode_count; ++i)
    {
        if (xrandr_modes[i] == size && dd_modes[i].refresh_rate == rate)
        {
            res = i;
            break;
        }
    }
    if (res == -1)
    {
        ERR("In unknown mode, returning default\n");
        res = 0;
    }
    return res;
}

static LONG X11DRV_XRandR_SetCurrentMode(int mode)
{
    SizeID size;
    Rotation rot;
    Window root;
    XRRScreenConfiguration *sc;
    Status stat;
    short rate;

    wine_tsx11_lock();
    root = RootWindow (gdi_display, DefaultScreen(gdi_display));
    sc = pXRRGetScreenInfo (gdi_display, root);
    size = pXRRConfigCurrentConfiguration (sc, &rot);
    mode = mode % xrandr_mode_count;

    TRACE("Changing Resolution to %dx%d @%d Hz\n",
          dd_modes[mode].width,
          dd_modes[mode].height,
          dd_modes[mode].refresh_rate);

    size = xrandr_modes[mode];
    rate = dd_modes[mode].refresh_rate;

    if (rate)
        stat = pXRRSetScreenConfigAndRate( gdi_display, sc, root, size, rot, rate, CurrentTime );
    else
        stat = pXRRSetScreenConfig( gdi_display, sc, root, size, rot, CurrentTime );

    pXRRFreeScreenConfigInfo(sc);
    wine_tsx11_unlock();
    if (stat == RRSetConfigSuccess)
    {
        X11DRV_resize_desktop( dd_modes[mode].width, dd_modes[mode].height );
        return DISP_CHANGE_SUCCESSFUL;
    }

    ERR("Resolution change not successful -- perhaps display has changed?\n");
    return DISP_CHANGE_FAILED;
}

static void xrandr_init_modes(void)
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

    if (!(xrandr_modes = HeapAlloc( GetProcessHeap(), 0, sizeof(*xrandr_modes) * nmodes )))
    {
        ERR("Failed to allocate xrandr mode info array.\n");
        return;
    }

    dd_modes = X11DRV_Settings_SetHandlers( "XRandR",
                                            X11DRV_XRandR_GetCurrentMode,
                                            X11DRV_XRandR_SetCurrentMode,
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
                xrandr_modes[xrandr_mode_count++] = i;
            }
        }
        else
        {
            X11DRV_Settings_AddOneMode( sizes[i].width, sizes[i].height, 0, 0 );
            xrandr_modes[xrandr_mode_count++] = i;
        }
    }

    X11DRV_Settings_AddDepthModes();
    nmodes = X11DRV_Settings_GetModeCount();

    TRACE("Available DD modes: count=%d\n", nmodes);
    TRACE("Enabling XRandR\n");
}

void X11DRV_XRandR_Init(void)
{
    int event_base, error_base, minor;
    static int major;
    Bool ok;

    if (major) return; /* already initialized? */
    if (!usexrandr) return; /* disabled in config */
    if (root_window != DefaultRootWindow( gdi_display )) return;
    if (!load_xrandr()) return;  /* can't load the Xrandr library */

    /* see if Xrandr is available */
    wine_tsx11_lock();
    if (!pXRRQueryExtension( gdi_display, &event_base, &error_base )) goto done;
    X11DRV_expect_error( gdi_display, XRandRErrorHandler, NULL );
    ok = pXRRQueryVersion( gdi_display, &major, &minor );
    if (X11DRV_check_error() || !ok) goto done;

    TRACE("Found XRandR %d.%d.\n", major, minor);
    xrandr_init_modes();

done:
    wine_tsx11_unlock();
}

#else /* SONAME_LIBXRANDR */

void X11DRV_XRandR_Init(void)
{
    TRACE("XRandR support not compiled in.\n");
}

#endif /* SONAME_LIBXRANDR */
