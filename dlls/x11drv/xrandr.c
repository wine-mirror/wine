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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <string.h>
#include <stdio.h>

#ifdef HAVE_LIBXRANDR

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include "x11drv.h"

#include "x11ddraw.h"
#include "xrandr.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xrandr);

extern int usexrandr;

static int xrandr_event, xrandr_error, xrandr_major, xrandr_minor;

static LPDDHALMODEINFO dd_modes;
static unsigned int dd_mode_count;
static XRRScreenSize *real_xrandr_sizes;
static short **real_xrandr_rates;
static unsigned int real_xrandr_sizes_count;
static int *real_xrandr_rates_count;
static unsigned int real_xrandr_modes_count;

static int XRandRErrorHandler(Display *dpy, XErrorEvent *event, void *arg)
{
    return 1;
}


/* create the mode structures */
static void make_modes(void)
{
    int i, j;
    for (i=0; i<real_xrandr_sizes_count; i++)
    {
        if (real_xrandr_rates_count[i])
        {
            for (j=0; j < real_xrandr_rates_count[i]; j++)
            {
                X11DRV_Settings_AddOneMode(real_xrandr_sizes[i].width, 
                                           real_xrandr_sizes[i].height, 
                                           0, real_xrandr_rates[i][j]);
            }
        }
        else
        {
            X11DRV_Settings_AddOneMode(real_xrandr_sizes[i].width, 
                                       real_xrandr_sizes[i].height, 
                                       0, 0);
        }
    }
}

static int X11DRV_XRandR_GetCurrentMode(void)
{
    SizeID size;
    Rotation rot;
    Window root;
    XRRScreenConfiguration *sc;
    short rate;
    int i;
    int res = -1;
    
    wine_tsx11_lock();
    root = RootWindow (gdi_display, DefaultScreen(gdi_display));
    sc = XRRGetScreenInfo (gdi_display, root);
    size = XRRConfigCurrentConfiguration (sc, &rot);
    rate = XRRConfigCurrentRate (sc);
    for (i = 0; i < real_xrandr_modes_count; i++)
    {
        if ( (dd_modes[i].dwWidth      == real_xrandr_sizes[size].width ) &&
             (dd_modes[i].dwHeight     == real_xrandr_sizes[size].height) &&
             (dd_modes[i].wRefreshRate == rate                          ) )
          {
              res = i;
          }
    }
    XRRFreeScreenConfigInfo(sc);
    wine_tsx11_unlock();
    if (res == -1)
    {
        ERR("In unknown mode, returning default\n");
        res = 0;
    }
    return res;
}

static void X11DRV_XRandR_SetCurrentMode(int mode)
{
    SizeID size;
    Rotation rot;
    Window root;
    XRRScreenConfiguration *sc;
    Status stat = RRSetConfigSuccess;
    short rate;
    int i, j;
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    
    wine_tsx11_lock();
    root = RootWindow (gdi_display, DefaultScreen(gdi_display));
    sc = XRRGetScreenInfo (gdi_display, root);
    size = XRRConfigCurrentConfiguration (sc, &rot);
    if (dwBpp != dd_modes[mode].dwBPP)
    {
        FIXME("Cannot change screen BPP from %ld to %ld\n", dwBpp, dd_modes[mode].dwBPP);
    }
    mode = mode%real_xrandr_modes_count;
    for (i = 0; i < real_xrandr_sizes_count; i++)
    {
        if ( (dd_modes[mode].dwWidth  == real_xrandr_sizes[i].width ) && 
             (dd_modes[mode].dwHeight == real_xrandr_sizes[i].height) )
        {
            size = i;
            if (real_xrandr_rates_count[i])
            {
                for (j=0; j < real_xrandr_rates_count[i]; j++)
                {
                    if (dd_modes[mode].wRefreshRate == real_xrandr_rates[i][j])
                    {
                        rate = real_xrandr_rates[i][j];
                        TRACE("Resizing X display to %ldx%ld @%d Hz\n", 
                              dd_modes[mode].dwWidth, dd_modes[mode].dwHeight, rate);
                        stat = XRRSetScreenConfigAndRate (gdi_display, sc, root, 
                                                          size, rot, rate, CurrentTime);
                    }
                }
            }
            else
            {
                TRACE("Resizing X display to %ldx%ld\n", 
                      dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
                stat = XRRSetScreenConfig (gdi_display, sc, root, 
                                           size, rot, CurrentTime);
            }
        }
    }
    XRRFreeScreenConfigInfo(sc);
    wine_tsx11_unlock();
    if (stat == RRSetConfigSuccess)
        X11DRV_handle_desktop_resize( dd_modes[mode].dwWidth, dd_modes[mode].dwHeight );
    else
        ERR("Resolution change not successful -- perhaps display has changed?\n");
}

void X11DRV_XRandR_Init(void)
{
    Bool ok;
    int nmodes = 0;
    int i;

    if (xrandr_major) return; /* already initialized? */
    if (!usexrandr) return; /* disabled in config */
    if (using_wine_desktop) return; /* not compatible with desktop mode */

    /* see if Xrandr is available */
    wine_tsx11_lock();
    ok = XRRQueryExtension(gdi_display, &xrandr_event, &xrandr_error);
    if (ok)
    {
        X11DRV_expect_error(gdi_display, XRandRErrorHandler, NULL);
        ok = XRRQueryVersion(gdi_display, &xrandr_major, &xrandr_minor);
        if (X11DRV_check_error()) ok = FALSE;
    }
    if (ok)
    {
        TRACE("Found XRandR - major: %d, minor: %d\n", xrandr_major, xrandr_minor);
        /* retrieve modes */
        real_xrandr_sizes = XRRSizes(gdi_display, DefaultScreen(gdi_display), &real_xrandr_sizes_count);
        ok = (real_xrandr_sizes_count>0);
    }
    if (ok)
    {
        real_xrandr_rates = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(short *) * real_xrandr_sizes_count);
        real_xrandr_rates_count = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(int) * real_xrandr_sizes_count);
        for (i=0; i < real_xrandr_sizes_count; i++)
        {
            real_xrandr_rates[i] = XRRRates (gdi_display, DefaultScreen(gdi_display), i, &(real_xrandr_rates_count[i]));
            if (real_xrandr_rates_count[i])
            {
                nmodes += real_xrandr_rates_count[i];
            }
            else
            {
                nmodes++;
            }
        }
    }
    wine_tsx11_unlock();
    if (!ok) return;

    real_xrandr_modes_count = nmodes;
    TRACE("XRandR modes: count=%d\n", nmodes);

    dd_modes = X11DRV_Settings_SetHandlers("XRandR", 
                                           X11DRV_XRandR_GetCurrentMode, 
                                           X11DRV_XRandR_SetCurrentMode, 
                                           nmodes, 1);
    make_modes();
    X11DRV_Settings_AddDepthModes();
    dd_mode_count = X11DRV_Settings_GetModeCount();
    X11DRV_Settings_SetDefaultMode(0);

    TRACE("Available DD modes: count=%d\n", dd_mode_count);
    TRACE("Enabling XRandR\n");
}

void X11DRV_XRandR_Cleanup(void)
{
    if (real_xrandr_rates)
    {
        HeapFree(GetProcessHeap(), 0, real_xrandr_rates);
        real_xrandr_rates = NULL;
    }
    if (real_xrandr_rates_count)
    {
        HeapFree(GetProcessHeap(), 0, real_xrandr_rates_count);
        real_xrandr_rates_count = NULL;
    }
}

#endif /* HAVE_LIBXRANDR */
