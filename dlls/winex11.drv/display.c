/*
 * X11DRV display device functions
 *
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static struct x11drv_display_device_handler host_handler;
struct x11drv_display_device_handler desktop_handler;
static struct x11drv_settings_handler settings_handler;

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

struct x11drv_display_depth
{
    struct list entry;
    ULONG_PTR display_id;
    DWORD depth;
};

/* Display device emulated depth list, protected by modes_section */
static struct list x11drv_display_depth_list = LIST_INIT(x11drv_display_depth_list);

/* All Windows drivers seen so far either support 32 bit depths, or 24 bit depths, but never both. So if we have
 * a 32 bit framebuffer, report 32 bit bpps, otherwise 24 bit ones.
 */
static const unsigned int depths_24[]  = {8, 16, 24};
static const unsigned int depths_32[]  = {8, 16, 32};
const unsigned int *depths;

/* Cached display modes for a device, protected by modes_section */
static WCHAR cached_device_name[CCHDEVICENAME];
static DWORD cached_flags;
static DEVMODEW *cached_modes;
static UINT cached_mode_count;

static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;

void X11DRV_Settings_SetHandler(const struct x11drv_settings_handler *new_handler)
{
    if (new_handler->priority > settings_handler.priority)
    {
        settings_handler = *new_handler;
        TRACE("Display settings are now handled by: %s.\n", settings_handler.name);
    }
}

/***********************************************************************
 * Default handlers if resolution switching is not enabled
 *
 */
static BOOL nores_get_id(const WCHAR *device_name, ULONG_PTR *id)
{
    WCHAR primary_adapter[CCHDEVICENAME];

    if (!get_primary_adapter( primary_adapter ))
        return FALSE;

    *id = !wcsicmp( device_name, primary_adapter ) ? 1 : 0;
    return TRUE;
}

static BOOL nores_get_modes(ULONG_PTR id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count)
{
    RECT primary = get_host_primary_monitor_rect();
    DEVMODEW *modes;

    modes = calloc(1, sizeof(*modes));
    if (!modes)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    modes[0].dmSize = sizeof(*modes);
    modes[0].dmDriverExtra = 0;
    modes[0].dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                        DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    modes[0].dmDisplayOrientation = DMDO_DEFAULT;
    modes[0].dmBitsPerPel = screen_bpp;
    modes[0].dmPelsWidth = primary.right;
    modes[0].dmPelsHeight = primary.bottom;
    modes[0].dmDisplayFlags = 0;
    modes[0].dmDisplayFrequency = 60;

    *new_modes = modes;
    *mode_count = 1;
    return TRUE;
}

static void nores_free_modes(DEVMODEW *modes)
{
    free(modes);
}

static BOOL nores_get_current_mode(ULONG_PTR id, DEVMODEW *mode)
{
    RECT primary = get_host_primary_monitor_rect();

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmDisplayFlags = 0;
    mode->dmPosition.x = 0;
    mode->dmPosition.y = 0;

    if (id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        mode->dmBitsPerPel = 0;
        mode->dmPelsWidth = 0;
        mode->dmPelsHeight = 0;
        mode->dmDisplayFrequency = 0;
        return TRUE;
    }

    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = primary.right;
    mode->dmPelsHeight = primary.bottom;
    mode->dmDisplayFrequency = 60;
    return TRUE;
}

static LONG nores_set_current_mode(ULONG_PTR id, const DEVMODEW *mode)
{
    WARN("NoRes settings handler, ignoring mode change request.\n");
    return DISP_CHANGE_SUCCESSFUL;
}

/* default handler only gets the current X desktop resolution */
void X11DRV_Settings_Init(void)
{
    struct x11drv_settings_handler nores_handler;

    depths = screen_bpp == 32 ? depths_32 : depths_24;

    nores_handler.name = "NoRes";
    nores_handler.priority = 1;
    nores_handler.get_id = nores_get_id;
    nores_handler.get_modes = nores_get_modes;
    nores_handler.free_modes = nores_free_modes;
    nores_handler.get_current_mode = nores_get_current_mode;
    nores_handler.set_current_mode = nores_set_current_mode;
    X11DRV_Settings_SetHandler(&nores_handler);
}

/* Initialize registry display settings when new display devices are added */
void init_registry_display_settings(void)
{
    DEVMODEW dm = {.dmSize = sizeof(dm)};
    DISPLAY_DEVICEW dd = {sizeof(dd)};
    UNICODE_STRING device_name;
    DWORD i = 0;
    LONG ret;

    while (!NtUserEnumDisplayDevices( NULL, i++, &dd, 0 ))
    {
        RtlInitUnicodeString( &device_name, dd.DeviceName );

        /* Skip if the device already has registry display settings */
        if (NtUserEnumDisplaySettings( &device_name, ENUM_REGISTRY_SETTINGS, &dm, 0 ))
            continue;

        if (!NtUserEnumDisplaySettings( &device_name, ENUM_CURRENT_SETTINGS, &dm, 0 ))
        {
            ERR("Failed to query current display settings for %s.\n", wine_dbgstr_w(dd.DeviceName));
            continue;
        }

        TRACE("Device %s current display mode %ux%u %ubits %uHz at %d,%d.\n",
              wine_dbgstr_w(dd.DeviceName), dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              dm.dmDisplayFrequency, dm.dmPosition.x, dm.dmPosition.y);

        ret = NtUserChangeDisplaySettings( &device_name, &dm, NULL,
                                           CDS_GLOBAL | CDS_NORESET | CDS_UPDATEREGISTRY, NULL );
        if (ret != DISP_CHANGE_SUCCESSFUL)
            ERR("Failed to save registry display settings for %s, returned %d.\n",
                wine_dbgstr_w(dd.DeviceName), ret);
    }
}

BOOL get_primary_adapter(WCHAR *name)
{
    DISPLAY_DEVICEW dd;
    DWORD i;

    dd.cb = sizeof(dd);
    for (i = 0; !NtUserEnumDisplayDevices( NULL, i, &dd, 0 ); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static int mode_compare(const void *p1, const void *p2)
{
    DWORD a_width, a_height, b_width, b_height;
    const DEVMODEW *a = p1, *b = p2;

    /* Use the width and height in landscape mode for comparison */
    if (a->dmDisplayOrientation == DMDO_DEFAULT || a->dmDisplayOrientation == DMDO_180)
    {
        a_width = a->dmPelsWidth;
        a_height = a->dmPelsHeight;
    }
    else
    {
        a_width = a->dmPelsHeight;
        a_height = a->dmPelsWidth;
    }

    if (b->dmDisplayOrientation == DMDO_DEFAULT || b->dmDisplayOrientation == DMDO_180)
    {
        b_width = b->dmPelsWidth;
        b_height = b->dmPelsHeight;
    }
    else
    {
        b_width = b->dmPelsHeight;
        b_height = b->dmPelsWidth;
    }

    /* Depth in descending order */
    if (a->dmBitsPerPel != b->dmBitsPerPel)
        return b->dmBitsPerPel - a->dmBitsPerPel;

    /* Width in ascending order */
    if (a_width != b_width)
        return a_width - b_width;

    /* Height in ascending order */
    if (a_height != b_height)
        return a_height - b_height;

    /* Frequency in descending order */
    if (a->dmDisplayFrequency != b->dmDisplayFrequency)
        return b->dmDisplayFrequency - a->dmDisplayFrequency;

    /* Orientation in ascending order */
    return a->dmDisplayOrientation - b->dmDisplayOrientation;
}

static void set_display_depth(ULONG_PTR display_id, DWORD depth)
{
    struct x11drv_display_depth *display_depth;

    pthread_mutex_lock( &settings_mutex );
    LIST_FOR_EACH_ENTRY(display_depth, &x11drv_display_depth_list, struct x11drv_display_depth, entry)
    {
        if (display_depth->display_id == display_id)
        {
            display_depth->depth = depth;
            pthread_mutex_unlock( &settings_mutex );
            return;
        }
    }

    display_depth = malloc(sizeof(*display_depth));
    if (!display_depth)
    {
        ERR("Failed to allocate memory.\n");
        pthread_mutex_unlock( &settings_mutex );
        return;
    }

    display_depth->display_id = display_id;
    display_depth->depth = depth;
    list_add_head(&x11drv_display_depth_list, &display_depth->entry);
    pthread_mutex_unlock( &settings_mutex );
}

static DWORD get_display_depth(ULONG_PTR display_id)
{
    struct x11drv_display_depth *display_depth;
    DWORD depth;

    pthread_mutex_lock( &settings_mutex );
    LIST_FOR_EACH_ENTRY(display_depth, &x11drv_display_depth_list, struct x11drv_display_depth, entry)
    {
        if (display_depth->display_id == display_id)
        {
            depth = display_depth->depth;
            pthread_mutex_unlock( &settings_mutex );
            return depth;
        }
    }
    pthread_mutex_unlock( &settings_mutex );
    return screen_bpp;
}

/***********************************************************************
 *      EnumDisplaySettingsEx  (X11DRV.@)
 *
 */
BOOL X11DRV_EnumDisplaySettingsEx( LPCWSTR name, DWORD n, LPDEVMODEW devmode, DWORD flags)
{
    DEVMODEW *modes, mode;
    UINT mode_count;
    ULONG_PTR id;

    pthread_mutex_lock( &settings_mutex );
    if (n == 0 || wcsicmp(cached_device_name, name) || cached_flags != flags)
    {
        if (!settings_handler.get_id(name, &id) || !settings_handler.get_modes(id, flags, &modes, &mode_count))
        {
            ERR("Failed to get %s supported display modes.\n", wine_dbgstr_w(name));
            pthread_mutex_unlock( &settings_mutex );
            return FALSE;
        }

        qsort(modes, mode_count, sizeof(*modes) + modes[0].dmDriverExtra, mode_compare);

        if (cached_modes)
            settings_handler.free_modes(cached_modes);
        lstrcpyW(cached_device_name, name);
        cached_flags = flags;
        cached_modes = modes;
        cached_mode_count = mode_count;
    }

    if (n >= cached_mode_count)
    {
        pthread_mutex_unlock( &settings_mutex );
        WARN("handler:%s device:%s mode index:%#x not found.\n", settings_handler.name, wine_dbgstr_w(name), n);
        RtlSetLastWin32Error( ERROR_NO_MORE_FILES );
        return FALSE;
    }

    mode = *(DEVMODEW *)((BYTE *)cached_modes + (sizeof(*cached_modes) + cached_modes[0].dmDriverExtra) * n);
    pthread_mutex_unlock( &settings_mutex );

    memcpy( &devmode->dmFields, &mode.dmFields, devmode->dmSize - offsetof(DEVMODEW, dmFields) );
    return TRUE;
}

/***********************************************************************
 *      GetCurrentDisplaySettings  (X11DRV.@)
 *
 */
BOOL X11DRV_GetCurrentDisplaySettings( LPCWSTR name, LPDEVMODEW devmode )
{
    DEVMODEW mode;
    ULONG_PTR id;

    if (!settings_handler.get_id( name, &id ) || !settings_handler.get_current_mode( id, &mode ))
    {
        ERR("Failed to get %s current display settings.\n", wine_dbgstr_w(name));
        return FALSE;
    }

    memcpy( &devmode->dmFields, &mode.dmFields, devmode->dmSize - offsetof(DEVMODEW, dmFields) );
    if (!is_detached_mode( devmode )) devmode->dmBitsPerPel = get_display_depth( id );
    return TRUE;
}

BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

/* Get the full display mode with all the necessary fields set.
 * Return NULL on failure. Caller should call free_full_mode() to free the returned mode. */
static DEVMODEW *get_full_mode(ULONG_PTR id, DEVMODEW *dev_mode)
{
    DEVMODEW *modes, *full_mode, *found_mode = NULL;
    UINT mode_count, mode_idx;

    if (is_detached_mode(dev_mode))
        return dev_mode;

    if (!settings_handler.get_modes(id, EDS_ROTATEDMODE, &modes, &mode_count))
        return NULL;

    qsort(modes, mode_count, sizeof(*modes) + modes[0].dmDriverExtra, mode_compare);
    for (mode_idx = 0; mode_idx < mode_count; ++mode_idx)
    {
        found_mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*modes) + modes[0].dmDriverExtra) * mode_idx);

        if (dev_mode->dmFields & DM_BITSPERPEL &&
            dev_mode->dmBitsPerPel &&
            found_mode->dmBitsPerPel != dev_mode->dmBitsPerPel)
            continue;
        if (dev_mode->dmFields & DM_PELSWIDTH && found_mode->dmPelsWidth != dev_mode->dmPelsWidth)
            continue;
        if (dev_mode->dmFields & DM_PELSHEIGHT && found_mode->dmPelsHeight != dev_mode->dmPelsHeight)
            continue;
        if (dev_mode->dmFields & DM_DISPLAYFREQUENCY &&
            dev_mode->dmDisplayFrequency &&
            found_mode->dmDisplayFrequency &&
            dev_mode->dmDisplayFrequency != 1 &&
            dev_mode->dmDisplayFrequency != found_mode->dmDisplayFrequency)
            continue;
        if (dev_mode->dmFields & DM_DISPLAYORIENTATION &&
            found_mode->dmDisplayOrientation != dev_mode->dmDisplayOrientation)
            continue;

        break;
    }

    if (!found_mode || mode_idx == mode_count)
    {
        settings_handler.free_modes(modes);
        return NULL;
    }

    if (!(full_mode = malloc(sizeof(*found_mode) + found_mode->dmDriverExtra)))
    {
        settings_handler.free_modes(modes);
        return NULL;
    }

    memcpy(full_mode, found_mode, sizeof(*found_mode) + found_mode->dmDriverExtra);
    settings_handler.free_modes(modes);

    full_mode->dmFields |= DM_POSITION;
    full_mode->dmPosition = dev_mode->dmPosition;
    return full_mode;
}

static void free_full_mode(DEVMODEW *mode)
{
    if (!is_detached_mode(mode))
        free(mode);
}

static LONG get_display_settings(DEVMODEW **new_displays, const WCHAR *dev_name, DEVMODEW *dev_mode)
{
    DEVMODEW registry_mode, current_mode, *mode, *displays;
    INT display_idx, display_count = 0;
    DISPLAY_DEVICEW display_device;
    LONG ret = DISP_CHANGE_FAILED;
    UNICODE_STRING device_name;

    display_device.cb = sizeof(display_device);
    for (display_idx = 0; !NtUserEnumDisplayDevices( NULL, display_idx, &display_device, 0 ); ++display_idx)
        ++display_count;

    /* use driver extra data to store an ULONG_PTR adapter id after each mode,
     * and allocate an extra mode to make iteration easier */
    if (!(displays = calloc(display_count + 1, sizeof(DEVMODEW) + sizeof(ULONG_PTR)))) goto done;
    mode = displays;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        ULONG_PTR *id = (ULONG_PTR *)(mode + 1);

        if (NtUserEnumDisplayDevices( NULL, display_idx, &display_device, 0 ))
            goto done;

        if (!settings_handler.get_id(display_device.DeviceName, id))
        {
            ret = DISP_CHANGE_BADPARAM;
            goto done;
        }

        RtlInitUnicodeString( &device_name, display_device.DeviceName );

        if (!dev_mode)
        {
            memset(&registry_mode, 0, sizeof(registry_mode));
            registry_mode.dmSize = sizeof(registry_mode);
            if (!NtUserEnumDisplaySettings( &device_name, ENUM_REGISTRY_SETTINGS, &registry_mode, 0 ))
                goto done;
            *mode = registry_mode;
        }
        else if (!wcsicmp(dev_name, display_device.DeviceName))
        {
            *mode = *dev_mode;
        }
        else
        {
            memset(&current_mode, 0, sizeof(current_mode));
            current_mode.dmSize = sizeof(current_mode);
            if (!NtUserEnumDisplaySettings( &device_name, ENUM_CURRENT_SETTINGS, &current_mode, 0 ))
                goto done;
            *mode = current_mode;
        }

        mode->dmSize = sizeof(DEVMODEW);
        lstrcpyW(mode->dmDeviceName, display_device.DeviceName);
        mode->dmDriverExtra = sizeof(ULONG_PTR);
        mode = NEXT_DEVMODEW(mode);
    }

    *new_displays = displays;
    return DISP_CHANGE_SUCCESSFUL;

done:
    free(displays);
    return ret;
}

static INT offset_length(POINT offset)
{
    return offset.x * offset.x + offset.y * offset.y;
}

static void set_rect_from_devmode(RECT *rect, const DEVMODEW *mode)
{
    SetRect(rect, mode->dmPosition.x, mode->dmPosition.y, mode->dmPosition.x + mode->dmPelsWidth, mode->dmPosition.y + mode->dmPelsHeight);
}

/* Check if a rect overlaps with placed display rects */
static BOOL overlap_placed_displays(const RECT *rect, const DEVMODEW *displays)
{
    const DEVMODEW *mode;
    RECT intersect;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode(&intersect, mode);
        if ((mode->dmFields & DM_POSITION) && intersect_rect(&intersect, &intersect, rect))
            return TRUE;
    }
    return FALSE;
}

/* Get the offset with minimum length to place a display next to the placed displays with no spacing and overlaps */
static POINT get_placement_offset(const DEVMODEW *displays, const DEVMODEW *placing)
{
    POINT points[8], left_top, offset, min_offset = {0, 0};
    INT point_idx, point_count, vertex_idx;
    BOOL has_placed = FALSE, first = TRUE;
    RECT desired_rect, rect;
    const DEVMODEW *mode;
    INT width, height;

    set_rect_from_devmode(&desired_rect, placing);

    /* If the display to be placed is detached, no offset is needed to place it */
    if (IsRectEmpty(&desired_rect))
        return min_offset;

    /* If there is no placed and attached display, place this display as it is */
    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode(&rect, mode);
        if ((mode->dmFields & DM_POSITION) && !IsRectEmpty(&rect))
        {
            has_placed = TRUE;
            break;
        }
    }

    if (!has_placed)
        return min_offset;

    /* Try to place this display with each of its four vertices at every vertex of the placed
     * displays and see which combination has the minimum offset length */
    width = desired_rect.right - desired_rect.left;
    height = desired_rect.bottom - desired_rect.top;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode(&rect, mode);
        if (!(mode->dmFields & DM_POSITION) || IsRectEmpty(&rect))
            continue;

        /* Get four vertices of the placed display rectangle */
        points[0].x = rect.left;
        points[0].y = rect.top;
        points[1].x = rect.left;
        points[1].y = rect.bottom;
        points[2].x = rect.right;
        points[2].y = rect.top;
        points[3].x = rect.right;
        points[3].y = rect.bottom;
        point_count = 4;

        /* Intersected points when moving the display to be placed horizontally */
        if (desired_rect.bottom >= rect.top &&
            desired_rect.top <= rect.bottom)
        {
            points[point_count].x = rect.left;
            points[point_count++].y = desired_rect.top;
            points[point_count].x = rect.right;
            points[point_count++].y = desired_rect.top;
        }
        /* Intersected points when moving the display to be placed vertically */
        if (desired_rect.left <= rect.right &&
            desired_rect.right >= rect.left)
        {
            points[point_count].x = desired_rect.left;
            points[point_count++].y = rect.top;
            points[point_count].x = desired_rect.left;
            points[point_count++].y = rect.bottom;
        }

        /* Try moving each vertex of the display rectangle to each points */
        for (point_idx = 0; point_idx < point_count; ++point_idx)
        {
            for (vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
            {
                switch (vertex_idx)
                {
                /* Move the bottom right vertex to the point */
                case 0:
                    left_top.x = points[point_idx].x - width;
                    left_top.y = points[point_idx].y - height;
                    break;
                /* Move the bottom left vertex to the point */
                case 1:
                    left_top.x = points[point_idx].x;
                    left_top.y = points[point_idx].y - height;
                    break;
                /* Move the top right vertex to the point */
                case 2:
                    left_top.x = points[point_idx].x - width;
                    left_top.y = points[point_idx].y;
                    break;
                /* Move the top left vertex to the point */
                case 3:
                    left_top.x = points[point_idx].x;
                    left_top.y = points[point_idx].y;
                    break;
                }

                offset.x = left_top.x - desired_rect.left;
                offset.y = left_top.y - desired_rect.top;
                rect = desired_rect;
                OffsetRect(&rect, offset.x, offset.y);
                if (!overlap_placed_displays(&rect, displays))
                {
                    if (first)
                    {
                        min_offset = offset;
                        first = FALSE;
                        continue;
                    }

                    if (offset_length(offset) < offset_length(min_offset))
                        min_offset = offset;
                }
            }
        }
    }

    return min_offset;
}

static void place_all_displays(DEVMODEW *displays)
{
    INT left_most = INT_MAX, top_most = INT_MAX;
    POINT min_offset, offset;
    DEVMODEW *mode, *placing;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        mode->dmFields &= ~DM_POSITION;

    /* Place all displays with no extra space between them and no overlapping */
    while (1)
    {
        /* Place the unplaced display with the minimum offset length first */
        placing = NULL;
        for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        {
            if (mode->dmFields & DM_POSITION)
                continue;

            offset = get_placement_offset(displays, mode);
            if (!placing || offset_length(offset) < offset_length(min_offset))
            {
                min_offset = offset;
                placing = mode;
            }
        }

        /* If all displays are placed */
        if (!placing)
            break;

        placing->dmPosition.x += min_offset.x;
        placing->dmPosition.y += min_offset.y;
        placing->dmFields |= DM_POSITION;

        left_most = min(left_most, placing->dmPosition.x);
        top_most = min(top_most, placing->dmPosition.y);
    }

    /* Convert virtual screen coordinates to root coordinates */
    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        mode->dmPosition.x -= left_most;
        mode->dmPosition.y -= top_most;
    }
}

static LONG apply_display_settings(DEVMODEW *displays, BOOL do_attach)
{
    DEVMODEW *full_mode;
    BOOL attached_mode;
    DEVMODEW *mode;
    LONG ret;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        ULONG_PTR *id = (ULONG_PTR *)(mode + 1);

        attached_mode = !is_detached_mode(mode);
        if ((attached_mode && !do_attach) || (!attached_mode && do_attach))
            continue;

        full_mode = get_full_mode(*id, mode);
        if (!full_mode)
            return DISP_CHANGE_BADMODE;

        TRACE("handler:%s changing %s to position:(%d,%d) resolution:%ux%u frequency:%uHz "
              "depth:%ubits orientation:%#x.\n", settings_handler.name,
              wine_dbgstr_w(mode->dmDeviceName),
              full_mode->dmPosition.x, full_mode->dmPosition.y, full_mode->dmPelsWidth,
              full_mode->dmPelsHeight, full_mode->dmDisplayFrequency, full_mode->dmBitsPerPel,
              full_mode->dmDisplayOrientation);

        ret = settings_handler.set_current_mode(*id, full_mode);
        if (attached_mode && ret == DISP_CHANGE_SUCCESSFUL)
            set_display_depth(*id, full_mode->dmBitsPerPel);
        free_full_mode(full_mode);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return ret;
    }

    return DISP_CHANGE_SUCCESSFUL;
}

static BOOL all_detached_settings(const DEVMODEW *displays)
{
    const DEVMODEW *mode;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        if (!is_detached_mode(mode))
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *      ChangeDisplaySettingsEx  (X11DRV.@)
 *
 */
LONG X11DRV_ChangeDisplaySettingsEx( LPCWSTR devname, LPDEVMODEW devmode,
                                     HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    DEVMODEW *displays, *mode, *full_mode;
    LONG ret;

    ret = get_display_settings( &displays, devname, devmode );
    if (ret != DISP_CHANGE_SUCCESSFUL)
        return ret;

    if (flags & CDS_UPDATEREGISTRY && devname && devmode)
    {
        for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        {
            ULONG_PTR *id = (ULONG_PTR *)(mode + 1);

            if (!wcsicmp(mode->dmDeviceName, devname))
            {
                full_mode = get_full_mode(*id, mode);
                if (!full_mode)
                {
                    free(displays);
                    return DISP_CHANGE_BADMODE;
                }

                memcpy( &devmode->dmFields, &full_mode->dmFields, devmode->dmSize - offsetof(DEVMODEW, dmFields) );
                free_full_mode(full_mode);
                break;
            }
        }
    }

    if (flags & (CDS_TEST | CDS_NORESET))
    {
        free(displays);
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (all_detached_settings( displays ))
    {
        WARN("Detaching all displays is not permitted.\n");
        free(displays);
        return DISP_CHANGE_SUCCESSFUL;
    }

    place_all_displays( displays );

    /* Detach displays first to free up CRTCs */
    ret = apply_display_settings( displays, FALSE );
    if (ret == DISP_CHANGE_SUCCESSFUL)
        ret = apply_display_settings( displays, TRUE );
    if (ret == DISP_CHANGE_SUCCESSFUL)
        X11DRV_DisplayDevices_Update(TRUE);
    free(displays);
    return ret;
}

POINT virtual_screen_to_root(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect();
    POINT pt;

    pt.x = x - virtual.left;
    pt.y = y - virtual.top;
    return pt;
}

POINT root_to_virtual_screen(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect();
    POINT pt;

    pt.x = x + virtual.left;
    pt.y = y + virtual.top;
    return pt;
}

/* Get the primary monitor rect from the host system */
RECT get_host_primary_monitor_rect(void)
{
    INT gpu_count, adapter_count, monitor_count;
    struct gdi_gpu *gpus = NULL;
    struct gdi_adapter *adapters = NULL;
    struct gdi_monitor *monitors = NULL;
    RECT rect = {0};

    /* The first monitor is always primary */
    if (host_handler.get_gpus(&gpus, &gpu_count) && gpu_count &&
        host_handler.get_adapters(gpus[0].id, &adapters, &adapter_count) && adapter_count &&
        host_handler.get_monitors(adapters[0].id, &monitors, &monitor_count) && monitor_count)
        rect = monitors[0].rc_monitor;

    if (gpus) host_handler.free_gpus(gpus);
    if (adapters) host_handler.free_adapters(adapters);
    if (monitors) host_handler.free_monitors(monitors, monitor_count);
    return rect;
}

BOOL get_host_primary_gpu(struct gdi_gpu *gpu)
{
    struct gdi_gpu *gpus;
    INT gpu_count;

    if (host_handler.get_gpus(&gpus, &gpu_count) && gpu_count)
    {
        *gpu = gpus[0];
        host_handler.free_gpus(gpus);
        return TRUE;
    }

    return FALSE;
}

RECT get_work_area(const RECT *monitor_rect)
{
    Atom type;
    int format;
    unsigned long count, remaining, i;
    long *work_area;
    RECT work_rect;

    /* Try _GTK_WORKAREAS first as _NET_WORKAREA may be incorrect on multi-monitor systems */
    if (!XGetWindowProperty(gdi_display, DefaultRootWindow(gdi_display),
                            x11drv_atom(_GTK_WORKAREAS_D0), 0, ~0, False, XA_CARDINAL, &type,
                            &format, &count, &remaining, (unsigned char **)&work_area))
    {
        if (type == XA_CARDINAL && format == 32)
        {
            for (i = 0; i < count / 4; ++i)
            {
                work_rect.left = work_area[i * 4];
                work_rect.top = work_area[i * 4 + 1];
                work_rect.right = work_rect.left + work_area[i * 4 + 2];
                work_rect.bottom = work_rect.top + work_area[i * 4 + 3];

                if (intersect_rect( &work_rect, &work_rect, monitor_rect ))
                {
                    TRACE("work_rect:%s.\n", wine_dbgstr_rect(&work_rect));
                    XFree(work_area);
                    return work_rect;
                }
            }
        }
        XFree(work_area);
    }

    WARN("_GTK_WORKAREAS is not supported, fallback to _NET_WORKAREA. "
         "Work areas may be incorrect on multi-monitor systems.\n");
    if (!XGetWindowProperty(gdi_display, DefaultRootWindow(gdi_display), x11drv_atom(_NET_WORKAREA),
                            0, ~0, False, XA_CARDINAL, &type, &format, &count, &remaining,
                            (unsigned char **)&work_area))
    {
        if (type == XA_CARDINAL && format == 32 && count >= 4)
        {
            SetRect(&work_rect, work_area[0], work_area[1], work_area[0] + work_area[2],
                    work_area[1] + work_area[3]);

            if (intersect_rect( &work_rect, &work_rect, monitor_rect ))
            {
                TRACE("work_rect:%s.\n", wine_dbgstr_rect(&work_rect));
                XFree(work_area);
                return work_rect;
            }
        }
        XFree(work_area);
    }

    WARN("_NET_WORKAREA is not supported, Work areas may be incorrect.\n");
    TRACE("work_rect:%s.\n", wine_dbgstr_rect(monitor_rect));
    return *monitor_rect;
}

void X11DRV_DisplayDevices_SetHandler(const struct x11drv_display_device_handler *new_handler)
{
    if (new_handler->priority > host_handler.priority)
    {
        host_handler = *new_handler;
        TRACE("Display device functions are now handled by: %s\n", host_handler.name);
    }
}

void X11DRV_DisplayDevices_RegisterEventHandlers(void)
{
    struct x11drv_display_device_handler *handler = is_virtual_desktop() ? &desktop_handler : &host_handler;

    if (handler->register_event_handlers)
        handler->register_event_handlers();
}

void X11DRV_DisplayDevices_Update(BOOL send_display_change)
{
    RECT old_virtual_rect, new_virtual_rect;
    DWORD tid, pid;
    HWND foreground;
    UINT mask = 0, i;
    HWND *list;

    old_virtual_rect = NtUserGetVirtualScreenRect();
    X11DRV_DisplayDevices_Init(TRUE);
    new_virtual_rect = NtUserGetVirtualScreenRect();

    /* Calculate XReconfigureWMWindow() mask */
    if (old_virtual_rect.left != new_virtual_rect.left)
        mask |= CWX;
    if (old_virtual_rect.top != new_virtual_rect.top)
        mask |= CWY;

    X11DRV_resize_desktop(send_display_change);

    list = build_hwnd_list();
    for (i = 0; list && list[i] != HWND_BOTTOM; i++)
    {
        struct x11drv_win_data *data;

        if (!(data = get_win_data( list[i] ))) continue;

        /* update the full screen state */
        update_net_wm_states(data);

        if (mask && data->whole_window)
        {
            POINT pos = virtual_screen_to_root(data->whole_rect.left, data->whole_rect.top);
            XWindowChanges changes;
            changes.x = pos.x;
            changes.y = pos.y;
            XReconfigureWMWindow(data->display, data->whole_window, data->vis.screen, mask, &changes);
        }
        release_win_data(data);
    }

    free( list );

    /* forward clip_fullscreen_window request to the foreground window */
    if ((foreground = NtUserGetForegroundWindow()) &&
        (tid = NtUserGetWindowThread( foreground, &pid )) && pid == GetCurrentProcessId())
    {
        if (tid == GetCurrentThreadId()) clip_fullscreen_window( foreground, TRUE );
        else send_notify_message( foreground, WM_X11DRV_CLIP_CURSOR_REQUEST, TRUE, TRUE );
    }
}

static BOOL force_display_devices_refresh;

BOOL X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, BOOL force, void *param )
{
    struct x11drv_display_device_handler *handler;
    struct gdi_adapter *adapters;
    struct gdi_monitor *monitors;
    struct gdi_gpu *gpus;
    INT gpu_count, adapter_count, monitor_count;
    INT gpu, adapter, monitor;
    DEVMODEW *modes, *mode;
    DWORD mode_count;

    if (!force && !force_display_devices_refresh) return TRUE;
    force_display_devices_refresh = FALSE;
    handler = is_virtual_desktop() ? &desktop_handler : &host_handler;

    TRACE("via %s\n", wine_dbgstr_a(handler->name));

    /* Initialize GPUs */
    if (!handler->get_gpus( &gpus, &gpu_count )) return FALSE;
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = 0; gpu < gpu_count; gpu++)
    {
        device_manager->add_gpu( &gpus[gpu], param );

        /* Initialize adapters */
        if (!handler->get_adapters(gpus[gpu].id, &adapters, &adapter_count)) break;
        TRACE("GPU: %#lx %s, adapter count: %d\n", gpus[gpu].id, wine_dbgstr_w(gpus[gpu].name), adapter_count);

        for (adapter = 0; adapter < adapter_count; adapter++)
        {
            device_manager->add_adapter( &adapters[adapter], param );

            if (!handler->get_monitors(adapters[adapter].id, &monitors, &monitor_count)) break;
            TRACE("adapter: %#lx, monitor count: %d\n", adapters[adapter].id, monitor_count);

            /* Initialize monitors */
            for (monitor = 0; monitor < monitor_count; monitor++)
            {
                TRACE("monitor: %#x %s\n", monitor, wine_dbgstr_w(monitors[monitor].name));
                device_manager->add_monitor( &monitors[monitor], param );
            }

            handler->free_monitors(monitors, monitor_count);

            if (!settings_handler.get_modes( adapters[adapter].id, EDS_ROTATEDMODE, &modes, &mode_count ))
                continue;

            qsort( modes, mode_count, sizeof(*modes) + modes[0].dmDriverExtra, mode_compare );

            for (mode = modes; mode_count; mode_count--)
            {
                TRACE( "mode: %p\n", mode );
                device_manager->add_mode( mode, param );
                mode = (DEVMODEW *)((char *)mode + sizeof(*modes) + modes[0].dmDriverExtra);
            }

            settings_handler.free_modes( modes );
        }

        handler->free_adapters(adapters);
    }

    handler->free_gpus(gpus);
    return TRUE;
}

void X11DRV_DisplayDevices_Init(BOOL force)
{
    UINT32 num_path, num_mode;

    if (force) force_display_devices_refresh = TRUE;
    /* trigger refresh in win32u */
    NtUserGetDisplayConfigBufferSizes( QDC_ONLY_ACTIVE_PATHS, &num_path, &num_mode );
}
