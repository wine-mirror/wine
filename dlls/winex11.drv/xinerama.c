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

static inline HMONITOR index_to_monitor( int index )
{
    return (HMONITOR)(UINT_PTR)(index + 1);
}

static inline int monitor_to_index( HMONITOR handle )
{
    UINT_PTR index = (UINT_PTR)handle;
    if (index < 1 || index > nb_monitors) return -1;
    return index - 1;
}


#ifdef HAVE_LIBXINERAMA

#define MAKE_FUNCPTR(f) static typeof(f) * p##f

MAKE_FUNCPTR(XineramaQueryExtension);
MAKE_FUNCPTR(XineramaQueryScreens);

#ifndef SONAME_LIBXINERAMA
#define SONAME_LIBXINERAMA "libXinerama" SONAME_EXT
#endif

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
    int i;

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
    for (i = 0; i < nb_monitors; i++)
    {
        OffsetRect( &monitors[i].rcMonitor, virtual_screen_rect.left, virtual_screen_rect.top );
        OffsetRect( &monitors[i].rcWork, virtual_screen_rect.left, virtual_screen_rect.top );
        TRACE( "monitor %p: %s%s\n",
               index_to_monitor(i), wine_dbgstr_rect(&monitors[i].rcMonitor),
               (monitors[i].dwFlags & MONITORINFOF_PRIMARY) ? " (primary)" : "" );
    }

    screen_width = primary->rcMonitor.right - primary->rcMonitor.left;
    screen_height = primary->rcMonitor.bottom - primary->rcMonitor.top;
    TRACE( "virtual size: %s primary size: %dx%d\n",
           wine_dbgstr_rect(&virtual_screen_rect), screen_width, screen_height );

    wine_tsx11_unlock();
}


/***********************************************************************
 *		X11DRV_GetMonitorInfo  (X11DRV.@)
 */
BOOL X11DRV_GetMonitorInfo( HMONITOR handle, LPMONITORINFO info )
{
    int i = monitor_to_index( handle );

    if (i == -1)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    info->rcMonitor = monitors[i].rcMonitor;
    info->rcWork = monitors[i].rcWork;
    info->dwFlags = monitors[i].dwFlags;
    if (info->cbSize >= sizeof(MONITORINFOEXW))
        lstrcpyW( ((MONITORINFOEXW *)info)->szDevice, monitors[i].szDevice );
    return TRUE;
}


/***********************************************************************
 *		X11DRV_EnumDisplayMonitors  (X11DRV.@)
 */
BOOL X11DRV_EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    int i;

    if (hdc)
    {
        POINT origin;
        RECT limit;

        if (!GetDCOrgEx( hdc, &origin )) return FALSE;
        if (GetClipBox( hdc, &limit ) == ERROR) return FALSE;

        if (rect && !IntersectRect( &limit, &limit, rect )) return TRUE;

        for (i = 0; i < nb_monitors; i++)
        {
            RECT monrect = monitors[i].rcMonitor;
            OffsetRect( &monrect, -origin.x, -origin.y );
            if (IntersectRect( &monrect, &monrect, &limit ))
                if (!proc( index_to_monitor(i), hdc, &monrect, lp )) break;
        }
    }
    else
    {
        for (i = 0; i < nb_monitors; i++)
        {
            RECT unused;
            if (!rect || IntersectRect( &unused, &monitors[i].rcMonitor, rect ))
                if (!proc( index_to_monitor(i), 0, &monitors[i].rcMonitor, lp )) break;
        }
    }
    return TRUE;
}
