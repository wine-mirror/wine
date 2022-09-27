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

    for (mode_idx = 0; mode_idx < mode_count; ++mode_idx)
    {
        found_mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*modes) + modes[0].dmDriverExtra) * mode_idx);

        if (found_mode->dmBitsPerPel != dev_mode->dmBitsPerPel)
            continue;
        if (found_mode->dmPelsWidth != dev_mode->dmPelsWidth)
            continue;
        if (found_mode->dmPelsHeight != dev_mode->dmPelsHeight)
            continue;
        if (found_mode->dmDisplayFrequency != dev_mode->dmDisplayFrequency)
            continue;
        if (found_mode->dmDisplayOrientation != dev_mode->dmDisplayOrientation)
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

static LONG apply_display_settings( DEVMODEW *displays, ULONG_PTR *ids, BOOL do_attach )
{
    DEVMODEW *full_mode;
    BOOL attached_mode;
    LONG count, ret;
    DEVMODEW *mode;

    for (count = 0, mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode), count++)
    {
        ULONG_PTR *id = ids + count;

        attached_mode = !is_detached_mode(mode);
        if ((attached_mode && !do_attach) || (!attached_mode && do_attach))
            continue;

        /* FIXME: get a full mode again because X11 driver extra data isn't portable */
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

/***********************************************************************
 *      ChangeDisplaySettings  (X11DRV.@)
 *
 */
LONG X11DRV_ChangeDisplaySettings( LPDEVMODEW displays, HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    INT left_most = INT_MAX, top_most = INT_MAX;
    LONG count, ret = DISP_CHANGE_BADPARAM;
    ULONG_PTR *ids;
    DEVMODEW *mode;

    /* Convert virtual screen coordinates to root coordinates, and find display ids.
     * We cannot safely get the ids while changing modes, as the backend state may be invalidated.
     */
    for (count = 0, mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode), count++)
    {
        left_most = min( left_most, mode->dmPosition.x );
        top_most = min( top_most, mode->dmPosition.y );
    }

    if (!(ids = calloc( count, sizeof(*ids) ))) return DISP_CHANGE_FAILED;
    for (count = 0, mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode), count++)
    {
        if (!settings_handler.get_id( mode->dmDeviceName, ids + count )) goto done;
        mode->dmPosition.x -= left_most;
        mode->dmPosition.y -= top_most;
    }

    /* Detach displays first to free up CRTCs */
    ret = apply_display_settings( displays, ids, FALSE );
    if (ret == DISP_CHANGE_SUCCESSFUL)
        ret = apply_display_settings( displays, ids, TRUE );
    if (ret == DISP_CHANGE_SUCCESSFUL)
        X11DRV_DisplayDevices_Init(TRUE);

done:
    free( ids );
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
