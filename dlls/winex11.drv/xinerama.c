/*
 * Xinerama support
 *
 * Copyright 2006 Alexandre Julliard
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

#include <stdarg.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#include <X11/extensions/Xinerama.h>
#endif
#include <dlfcn.h>
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static MONITORINFOEXW default_monitor =
{
    sizeof(default_monitor),    /* cbSize */
    { 0, 0, 0, 0 },             /* rcMonitor */
    { 0, 0, 0, 0 },             /* rcWork */
    MONITORINFOF_PRIMARY,       /* dwFlags */
    { '\\','\\','.','\\','D','I','S','P','L','A','Y','1',0 }   /* szDevice */
};

static pthread_mutex_t xinerama_mutex = PTHREAD_MUTEX_INITIALIZER;
static MONITORINFOEXW *monitors;
static int nb_monitors;

static inline MONITORINFOEXW *get_primary(void)
{
    /* default to 0 if specified primary is invalid */
    int idx = primary_monitor;
    if (idx >= nb_monitors) idx = 0;
    return &monitors[idx];
}

#ifdef SONAME_LIBXINERAMA

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

MAKE_FUNCPTR(XineramaQueryExtension);
MAKE_FUNCPTR(XineramaQueryScreens);

static void load_xinerama(void)
{
    void *handle;

    if (!(handle = dlopen(SONAME_LIBXINERAMA, RTLD_NOW)))
    {
        WARN( "failed to open %s\n", SONAME_LIBXINERAMA );
        return;
    }
    pXineramaQueryExtension = dlsym( handle, "XineramaQueryExtension" );
    if (!pXineramaQueryExtension) WARN( "XineramaQueryScreens not found\n" );
    pXineramaQueryScreens = dlsym( handle, "XineramaQueryScreens" );
    if (!pXineramaQueryScreens) WARN( "XineramaQueryScreens not found\n" );
}

static int query_screens(void)
{
    int i, count, event_base, error_base;
    XineramaScreenInfo *screens;

    if (!monitors)  /* first time around */
        load_xinerama();

    if (!pXineramaQueryExtension || !pXineramaQueryScreens ||
        !pXineramaQueryExtension( gdi_display, &event_base, &error_base ) ||
        !(screens = pXineramaQueryScreens( gdi_display, &count ))) return 0;

    if (monitors != &default_monitor) free( monitors );
    if ((monitors = malloc( count * sizeof(*monitors) )))
    {
        nb_monitors = count;
        for (i = 0; i < nb_monitors; i++)
        {
            monitors[i].cbSize = sizeof( monitors[i] );
            monitors[i].rcMonitor.left   = screens[i].x_org;
            monitors[i].rcMonitor.top    = screens[i].y_org;
            monitors[i].rcMonitor.right  = screens[i].x_org + screens[i].width;
            monitors[i].rcMonitor.bottom = screens[i].y_org + screens[i].height;
            monitors[i].dwFlags          = 0;
            monitors[i].rcWork           = get_work_area( &monitors[i].rcMonitor );
        }

        get_primary()->dwFlags |= MONITORINFOF_PRIMARY;
    }
    else count = 0;

    XFree( screens );
    return count;
}

#else  /* SONAME_LIBXINERAMA */

static inline int query_screens(void)
{
    return 0;
}

#endif  /* SONAME_LIBXINERAMA */

/* Get xinerama monitor indices required for _NET_WM_FULLSCREEN_MONITORS */
BOOL xinerama_get_fullscreen_monitors( const RECT *rect, long *indices )
{
    RECT window_rect, intersected_rect, monitor_rect;
    BOOL ret = FALSE;
    POINT offset;
    INT i;

    pthread_mutex_lock( &xinerama_mutex );
    if (nb_monitors == 1)
    {
        memset( indices, 0, sizeof(*indices) * 4 );
        ret = TRUE;
        goto done;
    }

    /* Convert window rectangle to root coordinates */
    offset = virtual_screen_to_root( rect->left, rect->top );
    window_rect.left = offset.x;
    window_rect.top = offset.y;
    window_rect.right = window_rect.left + rect->right - rect->left;
    window_rect.bottom = window_rect.top + rect->bottom - rect->top;

    /* Compare to xinerama monitor rectangles in root coordinates */
    offset.x = INT_MAX;
    offset.y = INT_MAX;
    for (i = 0; i < nb_monitors; ++i)
    {
        offset.x = min( offset.x, monitors[i].rcMonitor.left );
        offset.y = min( offset.y, monitors[i].rcMonitor.top );
    }

    indices[0] = -1;
    indices[1] = -1;
    indices[2] = -1;
    indices[3] = -1;
    for (i = 0; i < nb_monitors; ++i)
    {
        SetRect( &monitor_rect, monitors[i].rcMonitor.left - offset.x,
                 monitors[i].rcMonitor.top - offset.y, monitors[i].rcMonitor.right - offset.x,
                 monitors[i].rcMonitor.bottom - offset.y );
        intersect_rect( &intersected_rect, &window_rect, &monitor_rect );
        if (EqualRect( &intersected_rect, &monitor_rect ))
        {
            if (indices[0] == -1 || monitors[i].rcMonitor.top < monitors[indices[0]].rcMonitor.top)
                indices[0] = i;
            if (indices[1] == -1 || monitors[i].rcMonitor.bottom > monitors[indices[1]].rcMonitor.bottom)
                indices[1] = i;
            if (indices[2] == -1 || monitors[i].rcMonitor.left < monitors[indices[2]].rcMonitor.left)
                indices[2] = i;
            if (indices[3] == -1 || monitors[i].rcMonitor.right > monitors[indices[3]].rcMonitor.right)
                indices[3] = i;
        }
    }

    if (indices[0] == -1 || indices[1] == -1 || indices[2] == -1 || indices[3] == -1)
        ERR("Failed to get xinerama fullscreen monitor indices.\n");
    else
        ret = TRUE;

done:
    pthread_mutex_unlock( &xinerama_mutex );
    if (ret)
        TRACE( "fullscreen monitors: %ld,%ld,%ld,%ld.\n", indices[0], indices[1], indices[2], indices[3] );
    return ret;
}

static BOOL xinerama_get_gpus( struct gdi_gpu **new_gpus, int *count, BOOL get_properties )
{
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
    struct gdi_gpu *gpus;

    /* Xinerama has no support for GPU, faking one */
    gpus = calloc( 1, sizeof(*gpus) );
    if (!gpus)
        return FALSE;

    lstrcpyW( gpus[0].name, wine_adapterW );

    *new_gpus = gpus;
    *count = 1;

    return TRUE;
}

static void xinerama_free_gpus( struct gdi_gpu *gpus )
{
    free( gpus );
}

static BOOL xinerama_get_adapters( ULONG_PTR gpu_id, struct gdi_adapter **new_adapters, int *count )
{
    struct gdi_adapter *adapters = NULL;
    INT index = 0;
    INT i, j;
    INT primary_index;
    BOOL mirrored;

    if (gpu_id)
        return FALSE;

    /* Being lazy, actual adapter count may be less */
    pthread_mutex_lock( &xinerama_mutex );
    adapters = calloc( nb_monitors, sizeof(*adapters) );
    if (!adapters)
    {
        pthread_mutex_unlock( &xinerama_mutex );
        return FALSE;
    }

    primary_index = primary_monitor;
    if (primary_index >= nb_monitors)
        primary_index = 0;

    for (i = 0; i < nb_monitors; i++)
    {
        mirrored = FALSE;
        for (j = 0; j < i; j++)
        {
            if (EqualRect( &monitors[i].rcMonitor, &monitors[j].rcMonitor) && !IsRectEmpty( &monitors[j].rcMonitor ))
            {
                mirrored = TRUE;
                break;
            }
        }

        /* Mirrored monitors share the same adapter */
        if (mirrored)
            continue;

        /* Use monitor index as id */
        adapters[index].id = (ULONG_PTR)i;

        if (i == primary_index)
            adapters[index].state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;

        if (!IsRectEmpty( &monitors[i].rcMonitor ))
            adapters[index].state_flags |= DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

        index++;
    }

    /* Primary adapter has to be first */
    if (primary_index)
    {
        struct gdi_adapter tmp;
        tmp = adapters[primary_index];
        adapters[primary_index] = adapters[0];
        adapters[0] = tmp;
    }

    *new_adapters = adapters;
    *count = index;
    pthread_mutex_unlock( &xinerama_mutex );
    return TRUE;
}

static void xinerama_free_adapters( struct gdi_adapter *adapters )
{
    free( adapters );
}

static BOOL xinerama_get_monitors( ULONG_PTR adapter_id, struct gdi_monitor **new_monitors, int *count )
{
    struct gdi_monitor *monitor;
    INT first = (INT)adapter_id;
    INT monitor_count = 0;
    INT index = 0;
    INT i;

    pthread_mutex_lock( &xinerama_mutex );

    for (i = first; i < nb_monitors; i++)
    {
        if (i == first
            || (EqualRect( &monitors[i].rcMonitor, &monitors[first].rcMonitor )
                && !IsRectEmpty( &monitors[first].rcMonitor )))
            monitor_count++;
    }

    monitor = calloc( monitor_count, sizeof(*monitor) );
    if (!monitor)
    {
        pthread_mutex_unlock( &xinerama_mutex );
        return FALSE;
    }

    for (i = first; i < nb_monitors; i++)
    {
        if (i == first
            || (EqualRect( &monitors[i].rcMonitor, &monitors[first].rcMonitor )
                && !IsRectEmpty( &monitors[first].rcMonitor )))
        {
            monitor[index].rc_monitor = monitors[i].rcMonitor;
            monitor[index].rc_work = monitors[i].rcWork;
            /* Xinerama only reports monitors already attached */
            monitor[index].state_flags = DISPLAY_DEVICE_ATTACHED;
            monitor[index].edid_len = 0;
            monitor[index].edid = NULL;
            if (!IsRectEmpty( &monitors[i].rcMonitor ))
                monitor[index].state_flags |= DISPLAY_DEVICE_ACTIVE;

            index++;
        }
    }

    *new_monitors = monitor;
    *count = monitor_count;
    pthread_mutex_unlock( &xinerama_mutex );
    return TRUE;
}

static void xinerama_free_monitors( struct gdi_monitor *monitors, int count )
{
    free( monitors );
}

void xinerama_init( unsigned int width, unsigned int height )
{
    struct x11drv_display_device_handler handler;
    MONITORINFOEXW *primary;
    int i;
    RECT rect;

    if (is_virtual_desktop())
        return;

    pthread_mutex_lock( &xinerama_mutex );

    SetRect( &rect, 0, 0, width, height );
    if (!query_screens())
    {
        default_monitor.rcMonitor = rect;
        default_monitor.rcWork = get_work_area( &default_monitor.rcMonitor );
        nb_monitors = 1;
        monitors = &default_monitor;
    }

    primary = get_primary();

    /* coordinates (0,0) have to point to the primary monitor origin */
    OffsetRect( &rect, -primary->rcMonitor.left, -primary->rcMonitor.top );
    for (i = 0; i < nb_monitors; i++)
    {
        OffsetRect( &monitors[i].rcMonitor, rect.left, rect.top );
        OffsetRect( &monitors[i].rcWork, rect.left, rect.top );
        TRACE( "monitor 0x%x: %s work %s%s\n",
               i, wine_dbgstr_rect(&monitors[i].rcMonitor),
               wine_dbgstr_rect(&monitors[i].rcWork),
               (monitors[i].dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "" );
    }

    pthread_mutex_unlock( &xinerama_mutex );

    handler.name = "Xinerama";
    handler.priority = 100;
    handler.get_gpus = xinerama_get_gpus;
    handler.get_adapters = xinerama_get_adapters;
    handler.get_monitors = xinerama_get_monitors;
    handler.free_gpus = xinerama_free_gpus;
    handler.free_adapters = xinerama_free_adapters;
    handler.free_monitors = xinerama_free_monitors;
    handler.register_event_handlers = NULL;
    X11DRV_DisplayDevices_SetHandler( &handler );
}
