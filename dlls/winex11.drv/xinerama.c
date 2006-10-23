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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#include <X11/extensions/Xinerama.h>
#endif
#include "wine/library.h"
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

static MONITORINFOEXW *monitors;
static int nb_monitors;

static inline MONITORINFOEXW *get_primary(void)
{
    /* default to 0 if specified primary is invalid */
    int idx = primary_monitor;
    if (idx >= nb_monitors) idx = 0;
    return &monitors[idx];
}

#ifdef HAVE_LIBXINERAMA

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

MAKE_FUNCPTR(XineramaQueryExtension);
MAKE_FUNCPTR(XineramaQueryScreens);

static void load_xinerama(void)
{
    void *handle;

    if (!(handle = wine_dlopen(SONAME_LIBXINERAMA, RTLD_NOW, NULL, 0)))
    {
        WARN( "failed to open %s\n", SONAME_LIBXINERAMA );
        return;
    }
    pXineramaQueryExtension = wine_dlsym( handle, "XineramaQueryExtension", NULL, 0 );
    if (!pXineramaQueryExtension) WARN( "XineramaQueryScreens not found\n" );
    pXineramaQueryScreens = wine_dlsym( handle, "XineramaQueryScreens", NULL, 0 );
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

    if (monitors != &default_monitor) HeapFree( GetProcessHeap(), 0, monitors );
    if ((monitors = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*monitors) )))
    {
        nb_monitors = count;
        for (i = 0; i < nb_monitors; i++)
        {
            /* FIXME: for now, force primary to be the screen that starts at (0,0) origin */
            if (!screens[i].x_org && !screens[i].y_org) primary_monitor = i;

            monitors[i].cbSize = sizeof( monitors[i] );
            monitors[i].rcMonitor.left   = screens[i].x_org;
            monitors[i].rcMonitor.top    = screens[i].y_org;
            monitors[i].rcMonitor.right  = screens[i].x_org + screens[i].width;
            monitors[i].rcMonitor.bottom = screens[i].y_org + screens[i].height;
            monitors[i].rcWork           = monitors[i].rcMonitor;
            monitors[i].dwFlags          = 0;
            /* FIXME: using the same device name for all monitors for now */
            lstrcpyW( monitors[i].szDevice, default_monitor.szDevice );
        }

        get_primary()->dwFlags |= MONITORINFOF_PRIMARY;

        for (i = 0; i < nb_monitors; i++)
            TRACE( "monitor %d: %s%s\n",
                   i, wine_dbgstr_rect(&monitors[i].rcMonitor),
                   (monitors[i].dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "" );
    }
    else count = 0;

    XFree( screens );
    return count;
}

#else  /* HAVE_LIBXINERAMA */

static inline int query_screens(void)
{
    return 0;
}

#endif  /* HAVE_LIBXINERAMA */

void xinerama_init(void)
{
    MONITORINFOEXW *primary;

    wine_tsx11_lock();

    SetRect( &virtual_screen_rect, 0, 0, screen_width, screen_height );

    if (root_window != DefaultRootWindow( gdi_display ) || !query_screens())
    {
        default_monitor.rcWork = default_monitor.rcMonitor = virtual_screen_rect;
        nb_monitors = 1;
        monitors = &default_monitor;
    }

    primary = get_primary();
    /* coordinates (0,0) have to point to the primary monitor origin */
    OffsetRect( &virtual_screen_rect, -primary->rcMonitor.left, -primary->rcMonitor.top );
    screen_width = primary->rcMonitor.right - primary->rcMonitor.left;
    screen_height = primary->rcMonitor.bottom - primary->rcMonitor.top;
    TRACE( "virtual size: %s primary size: %dx%d\n",
           wine_dbgstr_rect(&virtual_screen_rect), screen_width, screen_height );

    wine_tsx11_unlock();
}
