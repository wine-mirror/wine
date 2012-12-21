/*
 * MACDRV display settings
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers Inc.
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

#include "macdrv.h"
#include "winuser.h"

WINE_DEFAULT_DEBUG_CHANNEL(display);


static inline HMONITOR display_id_to_monitor(CGDirectDisplayID display_id)
{
    return (HMONITOR)(UINT_PTR)display_id;
}

static inline CGDirectDisplayID monitor_to_display_id(HMONITOR handle)
{
    return (CGDirectDisplayID)(UINT_PTR)handle;
}


/***********************************************************************
 *              EnumDisplayMonitors  (MACDRV.@)
 */
BOOL CDECL macdrv_EnumDisplayMonitors(HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lparam)
{
    struct macdrv_display *displays;
    int num_displays;
    int i;
    BOOL ret = TRUE;

    TRACE("%p, %s, %p, %#lx\n", hdc, wine_dbgstr_rect(rect), proc, lparam);

    if (hdc)
    {
        POINT origin;
        RECT limit;

        if (!GetDCOrgEx(hdc, &origin)) return FALSE;
        if (GetClipBox(hdc, &limit) == ERROR) return FALSE;

        if (rect && !IntersectRect(&limit, &limit, rect)) return TRUE;

        if (macdrv_get_displays(&displays, &num_displays))
            return FALSE;

        for (i = 0; i < num_displays; i++)
        {
            RECT monrect = rect_from_cgrect(displays[i].frame);
            OffsetRect(&monrect, -origin.x, -origin.y);
            if (IntersectRect(&monrect, &monrect, &limit))
            {
                HMONITOR monitor = display_id_to_monitor(displays[i].displayID);
                TRACE("monitor %d handle %p @ %s\n", i, monitor, wine_dbgstr_rect(&monrect));
                if (!proc(monitor, hdc, &monrect, lparam))
                {
                    ret = FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        if (macdrv_get_displays(&displays, &num_displays))
            return FALSE;

        for (i = 0; i < num_displays; i++)
        {
            RECT monrect = rect_from_cgrect(displays[i].frame);
            RECT unused;
            if (!rect || IntersectRect(&unused, &monrect, rect))
            {
                HMONITOR monitor = display_id_to_monitor(displays[i].displayID);
                TRACE("monitor %d handle %p @ %s\n", i, monitor, wine_dbgstr_rect(&monrect));
                if (!proc(monitor, 0, &monrect, lparam))
                {
                    ret = FALSE;
                    break;
                }
            }
        }
    }

    macdrv_free_displays(displays);

    return ret;
}
