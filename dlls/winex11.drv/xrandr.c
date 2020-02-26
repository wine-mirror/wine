/*
 * Wine X11drv Xrandr interface
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2012 Henri Verbeet for CodeWeavers
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
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

#include "wine/heap.h"
#include "wine/library.h"
#include "wine/unicode.h"

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
MAKE_FUNCPTR(XRRSetScreenSize)
static typeof(XRRGetScreenResources) *pXRRGetScreenResourcesCurrent;
static RRMode *xrandr12_modes;
static int primary_crtc;
#endif

#ifdef HAVE_XRRGETPROVIDERRESOURCES
MAKE_FUNCPTR(XRRSelectInput)
MAKE_FUNCPTR(XRRGetOutputPrimary)
MAKE_FUNCPTR(XRRGetProviderResources)
MAKE_FUNCPTR(XRRFreeProviderResources)
MAKE_FUNCPTR(XRRGetProviderInfo)
MAKE_FUNCPTR(XRRFreeProviderInfo)
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
        LOAD_FUNCPTR(XRRSetScreenSize)
        r = 2;
#endif

#ifdef HAVE_XRRGETPROVIDERRESOURCES
        LOAD_FUNCPTR(XRRSelectInput)
        LOAD_FUNCPTR(XRRGetOutputPrimary)
        LOAD_FUNCPTR(XRRGetProviderResources)
        LOAD_FUNCPTR(XRRFreeProviderResources)
        LOAD_FUNCPTR(XRRGetProviderInfo)
        LOAD_FUNCPTR(XRRFreeProviderInfo)
        r = 4;
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

static XRRScreenResources *xrandr_get_screen_resources(void)
{
    XRRScreenResources *resources = pXRRGetScreenResourcesCurrent( gdi_display, root_window );
    if (resources && !resources->ncrtc)
    {
        pXRRFreeScreenResources( resources );
        resources = pXRRGetScreenResources( gdi_display, root_window );
    }

    if (!resources)
        ERR("Failed to get screen resources.\n");
    return resources;
}

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

static void get_screen_size( XRRScreenResources *resources, unsigned int *width, unsigned int *height )
{
    XRRCrtcInfo *crtc_info;
    int i;
    *width = *height = 0;

    for (i = 0; i < resources->ncrtc; ++i)
    {
        if (!(crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[i] )))
            continue;

        if (crtc_info->mode != None)
        {
            *width = max(*width, crtc_info->x + crtc_info->width);
            *height = max(*height, crtc_info->y + crtc_info->height);
        }

        pXRRFreeCrtcInfo( crtc_info );
    }
}

static LONG xrandr12_set_current_mode( int mode )
{
    unsigned int screen_width, screen_height;
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

    /* According to the RandR spec, the entire CRTC must fit inside the screen.
     * Since we use the union of all enabled CRTCs to determine the necessary
     * screen size, this might involve shrinking the screen, so we must disable
     * the CRTC in question first. */

    XGrabServer( gdi_display );

    status = pXRRSetCrtcConfig( gdi_display, resources, resources->crtcs[primary_crtc],
                                CurrentTime, crtc_info->x, crtc_info->y, None,
                                crtc_info->rotation, NULL, 0 );
    if (status != RRSetConfigSuccess)
    {
        XUngrabServer( gdi_display );
        XFlush( gdi_display );
        ERR("Failed to disable CRTC.\n");
        pXRRFreeCrtcInfo( crtc_info );
        pXRRFreeScreenResources( resources );
        return DISP_CHANGE_FAILED;
    }

    get_screen_size( resources, &screen_width, &screen_height );
    screen_width = max( screen_width, crtc_info->x + dd_modes[mode].width );
    screen_height = max( screen_height, crtc_info->y + dd_modes[mode].height );

    pXRRSetScreenSize( gdi_display, root_window, screen_width, screen_height,
            screen_width * DisplayWidthMM( gdi_display, default_visual.screen )
                         / DisplayWidth( gdi_display, default_visual.screen ),
            screen_height * DisplayHeightMM( gdi_display, default_visual.screen )
                         / DisplayHeight( gdi_display, default_visual.screen ));

    status = pXRRSetCrtcConfig( gdi_display, resources, resources->crtcs[primary_crtc],
                                CurrentTime, crtc_info->x, crtc_info->y, xrandr12_modes[mode],
                                crtc_info->rotation, crtc_info->outputs, crtc_info->noutput );

    XUngrabServer( gdi_display );
    XFlush( gdi_display );

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

    if (!(resources = xrandr_get_screen_resources()))
        return ret;

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

#ifdef HAVE_XRRGETPROVIDERRESOURCES

static RECT get_primary_rect( XRRScreenResources *resources )
{
    XRROutputInfo *output_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    RROutput primary_output;
    RECT primary_rect = {0};
    RECT first_rect = {0};
    INT i;

    primary_output = pXRRGetOutputPrimary( gdi_display, root_window );
    if (!primary_output)
        goto fallback;

    output_info = pXRRGetOutputInfo( gdi_display, resources, primary_output );
    if (!output_info || output_info->connection != RR_Connected || !output_info->crtc)
        goto fallback;

    crtc_info = pXRRGetCrtcInfo( gdi_display, resources, output_info->crtc );
    if (!crtc_info || !crtc_info->mode)
        goto fallback;

    SetRect( &primary_rect, crtc_info->x, crtc_info->y, crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );
    pXRRFreeCrtcInfo( crtc_info );
    pXRRFreeOutputInfo( output_info );
    return primary_rect;

/* Fallback when XRandR primary output is a disconnected output.
 * Try to find a crtc with (x, y) being (0, 0). If it's found then get the primary rect from that crtc,
 * otherwise use the first active crtc to get the primary rect */
fallback:
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );

    WARN("Primary is set to a disconnected XRandR output.\n");
    for (i = 0; i < resources->ncrtc; ++i)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, resources, resources->crtcs[i] );
        if (!crtc_info)
            continue;

        if (!crtc_info->mode)
        {
            pXRRFreeCrtcInfo( crtc_info );
            continue;
        }

        if (!crtc_info->x && !crtc_info->y)
        {
            SetRect( &primary_rect, 0, 0, crtc_info->width, crtc_info->height );
            pXRRFreeCrtcInfo( crtc_info );
            break;
        }

        if (IsRectEmpty( &first_rect ))
            SetRect( &first_rect, crtc_info->x, crtc_info->y,
                     crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );

        pXRRFreeCrtcInfo( crtc_info );
    }

    return IsRectEmpty( &primary_rect ) ? first_rect : primary_rect;
}

static BOOL is_crtc_primary( RECT primary, const XRRCrtcInfo *crtc )
{
    return crtc &&
           crtc->mode &&
           crtc->x == primary.left &&
           crtc->y == primary.top &&
           crtc->x + crtc->width == primary.right &&
           crtc->y + crtc->height == primary.bottom;
}

static BOOL xrandr14_get_gpus( struct x11drv_gpu **new_gpus, int *count )
{
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
    struct x11drv_gpu *gpus = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRRProviderResources *provider_resources = NULL;
    XRRProviderInfo *provider_info = NULL;
    XRRCrtcInfo *crtc_info = NULL;
    INT primary_provider = -1;
    RECT primary_rect;
    BOOL ret = FALSE;
    INT i, j;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    provider_resources = pXRRGetProviderResources( gdi_display, root_window );
    if (!provider_resources)
        goto done;

    gpus = heap_calloc( provider_resources->nproviders ? provider_resources->nproviders : 1, sizeof(*gpus) );
    if (!gpus)
        goto done;

    /* Some XRandR implementations don't support providers.
     * In this case, report a fake one to try searching adapters in screen resources */
    if (!provider_resources->nproviders)
    {
        WARN("XRandR implementation doesn't report any providers, faking one.\n");
        lstrcpyW( gpus[0].name, wine_adapterW );
        *new_gpus = gpus;
        *count = 1;
        ret = TRUE;
        goto done;
    }

    primary_rect = get_primary_rect( screen_resources );
    for (i = 0; i < provider_resources->nproviders; ++i)
    {
        provider_info = pXRRGetProviderInfo( gdi_display, screen_resources, provider_resources->providers[i] );
        if (!provider_info)
            goto done;

        /* Find primary provider */
        for (j = 0; primary_provider == -1 && j < provider_info->ncrtcs; ++j)
        {
            crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, provider_info->crtcs[j] );
            if (!crtc_info)
                continue;

            if (is_crtc_primary( primary_rect, crtc_info ))
            {
                primary_provider = i;
                pXRRFreeCrtcInfo( crtc_info );
                break;
            }

            pXRRFreeCrtcInfo( crtc_info );
        }

        gpus[i].id = provider_resources->providers[i];
        MultiByteToWideChar( CP_UTF8, 0, provider_info->name, -1, gpus[i].name, ARRAY_SIZE(gpus[i].name) );
        /* PCI IDs are all zero because there is currently no portable way to get it via XRandR. Some AMD drivers report
         * their PCI address in the name but many others don't */
        pXRRFreeProviderInfo( provider_info );
    }

    /* Make primary GPU the first */
    if (primary_provider > 0)
    {
        struct x11drv_gpu tmp = gpus[0];
        gpus[0] = gpus[primary_provider];
        gpus[primary_provider] = tmp;
    }

    *new_gpus = gpus;
    *count = provider_resources->nproviders;
    ret = TRUE;
done:
    if (provider_resources)
        pXRRFreeProviderResources( provider_resources );
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (!ret)
    {
        heap_free( gpus );
        ERR("Failed to get gpus\n");
    }
    return ret;
}

static void xrandr14_free_gpus( struct x11drv_gpu *gpus )
{
    heap_free( gpus );
}

static BOOL xrandr14_get_adapters( ULONG_PTR gpu_id, struct x11drv_adapter **new_adapters, int *count )
{
    struct x11drv_adapter *adapters = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRRProviderInfo *provider_info = NULL;
    XRRCrtcInfo *enum_crtc_info, *crtc_info = NULL;
    XRROutputInfo *output_info = NULL;
    RROutput *outputs;
    INT crtc_count, output_count;
    INT primary_adapter = 0;
    INT adapter_count = 0;
    BOOL mirrored, detached;
    RECT primary_rect;
    BOOL ret = FALSE;
    INT i, j;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    if (gpu_id)
    {
        provider_info = pXRRGetProviderInfo( gdi_display, screen_resources, gpu_id );
        if (!provider_info)
            goto done;

        crtc_count = provider_info->ncrtcs;
        output_count = provider_info->noutputs;
        outputs = provider_info->outputs;
    }
    /* Fake provider id, search adapters in screen resources */
    else
    {
        crtc_count = screen_resources->ncrtc;
        output_count = screen_resources->noutput;
        outputs = screen_resources->outputs;
    }

    /* Actual adapter count could be less */
    adapters = heap_calloc( crtc_count, sizeof(*adapters) );
    if (!adapters)
        goto done;

    primary_rect = get_primary_rect( screen_resources );
    for (i = 0; i < output_count; ++i)
    {
        output_info = pXRRGetOutputInfo( gdi_display, screen_resources, outputs[i] );
        if (!output_info)
            goto done;

        /* Only connected output are considered as monitors */
        if (output_info->connection != RR_Connected)
        {
            pXRRFreeOutputInfo( output_info );
            output_info = NULL;
            continue;
        }

        /* Connected output doesn't mean the output is attached to a crtc */
        detached = FALSE;
        if (output_info->crtc)
        {
            crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, output_info->crtc );
            if (!crtc_info)
                goto done;
        }

        if (!output_info->crtc || !crtc_info->mode)
            detached = TRUE;

        /* Ignore crtc mirroring slaves because mirrored monitors are under the same adapter */
        mirrored = FALSE;
        if (!detached)
        {
            for (j = 0; j < screen_resources->ncrtc; ++j)
            {
                enum_crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, screen_resources->crtcs[j] );
                if (!enum_crtc_info)
                    goto done;

                /* Some crtcs on different providers may have the same coordinates, aka mirrored.
                 * Choose the crtc with the lowest value as primary and the rest will then be slaves
                 * in a mirroring set */
                if (crtc_info->x == enum_crtc_info->x &&
                    crtc_info->y == enum_crtc_info->y &&
                    crtc_info->width == enum_crtc_info->width &&
                    crtc_info->height == enum_crtc_info->height &&
                    output_info->crtc > screen_resources->crtcs[j])
                {
                    mirrored = TRUE;
                    pXRRFreeCrtcInfo( enum_crtc_info );
                    break;
                }

                pXRRFreeCrtcInfo( enum_crtc_info );
            }
        }

        if (!mirrored || detached)
        {
            /* Use RROutput as adapter id. The reason of not using RRCrtc is that we need to detect inactive but
             * attached monitors */
            adapters[adapter_count].id = outputs[i];
            if (!detached)
                adapters[adapter_count].state_flags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
            if (is_crtc_primary( primary_rect, crtc_info ))
            {
                adapters[adapter_count].state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
                primary_adapter = adapter_count;
            }

            ++adapter_count;
        }

        pXRRFreeOutputInfo( output_info );
        output_info = NULL;
        if (crtc_info)
        {
            pXRRFreeCrtcInfo( crtc_info );
            crtc_info = NULL;
        }
    }

    /* Make primary adapter the first */
    if (primary_adapter)
    {
        struct x11drv_adapter tmp = adapters[0];
        adapters[0] = adapters[primary_adapter];
        adapters[primary_adapter] = tmp;
    }

    *new_adapters = adapters;
    *count = adapter_count;
    ret = TRUE;
done:
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (provider_info)
        pXRRFreeProviderInfo( provider_info );
    if (output_info)
        pXRRFreeOutputInfo( output_info );
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (!ret)
    {
        heap_free( adapters );
        ERR("Failed to get adapters\n");
    }
    return ret;
}

static void xrandr14_free_adapters( struct x11drv_adapter *adapters )
{
    heap_free( adapters );
}

static BOOL xrandr14_get_monitors( ULONG_PTR adapter_id, struct x11drv_monitor **new_monitors, int *count )
{
    static const WCHAR generic_nonpnp_monitorW[] = {
        'G','e','n','e','r','i','c',' ',
        'N','o','n','-','P','n','P',' ','M','o','n','i','t','o','r',0};
    struct x11drv_monitor *realloc_monitors, *monitors = NULL;
    XRRScreenResources *screen_resources = NULL;
    XRROutputInfo *output_info = NULL, *enum_output_info = NULL;
    XRRCrtcInfo *crtc_info = NULL, *enum_crtc_info;
    INT primary_index = 0, monitor_count = 0, capacity;
    RECT work_rect, primary_rect;
    BOOL ret = FALSE;
    INT i;

    screen_resources = xrandr_get_screen_resources();
    if (!screen_resources)
        goto done;

    /* First start with a 2 monitors, should be enough for most cases */
    capacity = 2;
    monitors = heap_calloc( capacity, sizeof(*monitors) );
    if (!monitors)
        goto done;

    output_info = pXRRGetOutputInfo( gdi_display, screen_resources, adapter_id );
    if (!output_info)
        goto done;

    if (output_info->crtc)
    {
        crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, output_info->crtc );
        if (!crtc_info)
            goto done;
    }

    /* Inactive but attached monitor, no need to check for mirrored/slave monitors */
    if (!output_info->crtc || !crtc_info->mode)
    {
        lstrcpyW( monitors[monitor_count].name, generic_nonpnp_monitorW );
        monitors[monitor_count].state_flags = DISPLAY_DEVICE_ATTACHED;
        monitor_count = 1;
    }
    /* Active monitors, need to find other monitors with the same coordinates as mirrored */
    else
    {
        query_work_area( &work_rect );
        primary_rect = get_primary_rect( screen_resources );

        for (i = 0; i < screen_resources->noutput; ++i)
        {
            enum_output_info = pXRRGetOutputInfo( gdi_display, screen_resources, screen_resources->outputs[i] );
            if (!enum_output_info)
                goto done;

            /* Detached outputs don't count */
            if (enum_output_info->connection != RR_Connected)
            {
                pXRRFreeOutputInfo( enum_output_info );
                enum_output_info = NULL;
                continue;
            }

            /* Allocate more space if needed */
            if (monitor_count >= capacity)
            {
                capacity *= 2;
                realloc_monitors = heap_realloc( monitors, capacity * sizeof(*monitors) );
                if (!realloc_monitors)
                    goto done;
                monitors = realloc_monitors;
            }

            if (enum_output_info->crtc)
            {
                enum_crtc_info = pXRRGetCrtcInfo( gdi_display, screen_resources, enum_output_info->crtc );
                if (!enum_crtc_info)
                    goto done;

                if (enum_crtc_info->x == crtc_info->x &&
                    enum_crtc_info->y == crtc_info->y &&
                    enum_crtc_info->width == crtc_info->width &&
                    enum_crtc_info->height == crtc_info->height)
                {
                    /* FIXME: Read output EDID property and parse the data to get the correct name */
                    lstrcpyW( monitors[monitor_count].name, generic_nonpnp_monitorW );

                    SetRect( &monitors[monitor_count].rc_monitor, crtc_info->x, crtc_info->y,
                             crtc_info->x + crtc_info->width, crtc_info->y + crtc_info->height );
                    if (!IntersectRect( &monitors[monitor_count].rc_work, &work_rect, &monitors[monitor_count].rc_monitor ))
                        monitors[monitor_count].rc_work = monitors[monitor_count].rc_monitor;

                    monitors[monitor_count].state_flags = DISPLAY_DEVICE_ATTACHED;
                    if (!IsRectEmpty( &monitors[monitor_count].rc_monitor ))
                        monitors[monitor_count].state_flags |= DISPLAY_DEVICE_ACTIVE;

                    if (is_crtc_primary( primary_rect, crtc_info ))
                        primary_index = monitor_count;
                    monitor_count++;
                }

                pXRRFreeCrtcInfo( enum_crtc_info );
            }

            pXRRFreeOutputInfo( enum_output_info );
            enum_output_info = NULL;
        }

        /* Make sure the first monitor is the primary */
        if (primary_index)
        {
            struct x11drv_monitor tmp = monitors[0];
            monitors[0] = monitors[primary_index];
            monitors[primary_index] = tmp;
        }

        /* Make sure the primary monitor origin is at (0, 0) */
        for (i = 0; i < monitor_count; i++)
        {
            OffsetRect( &monitors[i].rc_monitor, -primary_rect.left, -primary_rect.top );
            OffsetRect( &monitors[i].rc_work, -primary_rect.left, -primary_rect.top );
        }
    }

    *new_monitors = monitors;
    *count = monitor_count;
    ret = TRUE;
done:
    if (screen_resources)
        pXRRFreeScreenResources( screen_resources );
    if (output_info)
        pXRRFreeOutputInfo( output_info);
    if (crtc_info)
        pXRRFreeCrtcInfo( crtc_info );
    if (enum_output_info)
        pXRRFreeOutputInfo( enum_output_info );
    if (!ret)
    {
        heap_free( monitors );
        ERR("Failed to get monitors\n");
    }
    return ret;
}

static void xrandr14_free_monitors( struct x11drv_monitor *monitors )
{
    heap_free( monitors );
}

static BOOL xrandr14_device_change_handler( HWND hwnd, XEvent *event )
{
    if (hwnd == GetDesktopWindow() && GetWindowThreadProcessId( hwnd, NULL ) == GetCurrentThreadId())
        X11DRV_DisplayDevices_Init( TRUE );
    return FALSE;
}

static void xrandr14_register_event_handlers(void)
{
    Display *display = thread_init_display();
    int event_base, error_base;

    if (!pXRRQueryExtension( display, &event_base, &error_base ))
        return;

    pXRRSelectInput( display, root_window,
                     RRCrtcChangeNotifyMask | RROutputChangeNotifyMask | RRProviderChangeNotifyMask );
    X11DRV_register_event_handler( event_base + RRNotify_CrtcChange, xrandr14_device_change_handler,
                                   "XRandR CrtcChange" );
    X11DRV_register_event_handler( event_base + RRNotify_OutputChange, xrandr14_device_change_handler,
                                   "XRandR OutputChange" );
    X11DRV_register_event_handler( event_base + RRNotify_ProviderChange, xrandr14_device_change_handler,
                                   "XRandR ProviderChange" );
}

#endif

void X11DRV_XRandR_Init(void)
{
    struct x11drv_display_device_handler handler;
    int event_base, error_base, minor, ret;
    static int major;
    Bool ok;

    if (major) return; /* already initialized? */
    if (!usexrandr) return; /* disabled in config */
    if (is_virtual_desktop()) return;
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

#ifdef HAVE_XRRGETPROVIDERRESOURCES
    if (ret >= 4 && (major > 1 || (major == 1 && minor >= 4)))
    {
        handler.name = "XRandR 1.4";
        handler.priority = 200;
        handler.get_gpus = xrandr14_get_gpus;
        handler.get_adapters = xrandr14_get_adapters;
        handler.get_monitors = xrandr14_get_monitors;
        handler.free_gpus = xrandr14_free_gpus;
        handler.free_adapters = xrandr14_free_adapters;
        handler.free_monitors = xrandr14_free_monitors;
        handler.register_event_handlers = xrandr14_register_event_handlers;
        X11DRV_DisplayDevices_SetHandler( &handler );
    }
#endif
}

#else /* SONAME_LIBXRANDR */

void X11DRV_XRandR_Init(void)
{
    TRACE("XRandR support not compiled in.\n");
}

#endif /* SONAME_LIBXRANDR */
