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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

static struct x11drv_display_device_handler host_handler;
static struct x11drv_settings_handler settings_handler;

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

/* All Windows drivers seen so far either support 32 bit depths, or 24 bit depths, but never both. So if we have
 * a 32 bit framebuffer, report 32 bit bpps, otherwise 24 bit ones.
 */
static const unsigned int depths_24[]  = {8, 16, 24};
static const unsigned int depths_32[]  = {8, 16, 32};
const unsigned int *depths;

static const char *debugstr_devmodew( const DEVMODEW *devmode )
{
    char position[32] = {0};
    if (devmode->dmFields & DM_POSITION) snprintf( position, sizeof(position), " at %s", wine_dbgstr_point( (POINT *)&devmode->dmPosition ) );
    return wine_dbg_sprintf( "%ux%u %ubits %uHz rotated %u degrees %sstretched %sinterlaced%s",
                             devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel,
                             devmode->dmDisplayFrequency, devmode->dmDisplayOrientation * 90,
                             devmode->dmDisplayFixedOutput == DMDFO_STRETCH ? "" : "un",
                             devmode->dmDisplayFlags & DM_INTERLACED ? "" : "non-",
                             position );
}

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
static BOOL nores_get_id(const WCHAR *device_name, BOOL is_primary, x11drv_settings_id *id)
{
    id->id = is_primary ? 1 : 0;
    return TRUE;
}

static BOOL nores_get_modes( x11drv_settings_id id, DWORD flags, struct x11drv_mode **new_modes, UINT *mode_count )
{
    RECT primary = get_host_primary_monitor_rect();
    struct x11drv_mode *modes;

    modes = calloc(1, sizeof(*modes));
    if (!modes)
    {
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    modes[0].mode.dmSize = sizeof(*modes);
    modes[0].mode.dmDriverExtra = 0;
    modes[0].mode.dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                             DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    modes[0].mode.dmDisplayOrientation = DMDO_DEFAULT;
    modes[0].mode.dmBitsPerPel = screen_bpp;
    modes[0].mode.dmPelsWidth = primary.right;
    modes[0].mode.dmPelsHeight = primary.bottom;
    modes[0].mode.dmDisplayFlags = 0;
    modes[0].mode.dmDisplayFrequency = 60;

    *new_modes = modes;
    *mode_count = 1;
    return TRUE;
}

static BOOL nores_get_current_mode(x11drv_settings_id id, DEVMODEW *mode)
{
    RECT primary = get_host_primary_monitor_rect();

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->dmDisplayOrientation = DMDO_DEFAULT;
    mode->dmDisplayFlags = 0;
    mode->dmPosition.x = 0;
    mode->dmPosition.y = 0;

    if (id.id != 1)
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

static LONG nores_set_current_mode( x11drv_settings_id id, const struct x11drv_mode *mode )
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
    nores_handler.get_current_mode = nores_get_current_mode;
    nores_handler.set_current_mode = nores_set_current_mode;
    X11DRV_Settings_SetHandler(&nores_handler);
}

static void strip_driver_extra( DEVMODEW *modes, UINT count )
{
    DEVMODEW *mode, *next;
    UINT i;

    for (i = 0, mode = modes; i < count; i++, mode = next)
    {
        next = NEXT_DEVMODEW(mode);
        mode->dmDriverExtra = 0;
        memcpy( modes + i, mode, sizeof(*mode) );
    }
}

BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

static BOOL is_same_devmode( const DEVMODEW *a, const DEVMODEW *b )
{
    return a->dmDisplayOrientation == b->dmDisplayOrientation &&
           a->dmBitsPerPel == b->dmBitsPerPel &&
           a->dmPelsWidth == b->dmPelsWidth &&
           a->dmPelsHeight == b->dmPelsHeight &&
           a->dmDisplayFrequency == b->dmDisplayFrequency;
}

static DWORD x11drv_mode_from_devmode( x11drv_settings_id id, DEVMODEW *mode, struct x11drv_mode *full )
{
    struct x11drv_mode *modes;
    UINT count, i;

    full->mode = *mode;
    full->mode.dmDriverExtra = sizeof(*full) - sizeof(full->mode);
    if (is_detached_mode( mode )) return DISP_CHANGE_SUCCESSFUL;

    if (!settings_handler.get_modes( id, EDS_ROTATEDMODE, &modes, &count )) return DISP_CHANGE_BADMODE;
    for (i = 0; i < count; i++) if (is_same_devmode( &modes[i].mode, mode )) break;
    if (i < count)
    {
        *full = modes[i];
        full->mode.dmFields |= DM_POSITION;
        full->mode.dmPosition = mode->dmPosition;
        memcpy( full->mode.dmDeviceName, mode->dmDeviceName, sizeof(mode->dmDeviceName) );
    }
    free( modes );

    return i < count ? DISP_CHANGE_SUCCESSFUL : DISP_CHANGE_BADMODE;
}

/***********************************************************************
 *      ChangeDisplaySettings  (X11DRV.@)
 *
 */
LONG X11DRV_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    INT left_most = INT_MAX, top_most = INT_MAX;
    LONG count, ret = DISP_CHANGE_FAILED;
    struct x11drv_mode *modes;
    x11drv_settings_id *ids;
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
    if (!(modes = calloc( count, sizeof(*modes) ))) goto done;

    for (count = 0, mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode), count++)
    {
        BOOL is_primary = !wcsicmp( mode->dmDeviceName, primary_name );
        if (!settings_handler.get_id( mode->dmDeviceName, is_primary, ids + count )) goto done;
        mode->dmPosition.x -= left_most;
        mode->dmPosition.y -= top_most;
        if ((ret = x11drv_mode_from_devmode( ids[count], mode, modes + count ))) goto done;
    }

    XGrabServer( gdi_display );

    /* Detach displays first to free up CRTCs */
    TRACE( "Using %s\n", settings_handler.name );
    for (UINT i = 0; !ret && i < count; i++)
    {
        if (!is_detached_mode( &modes[i].mode )) continue;
        TRACE( "  setting %s mode %s\n", debugstr_w(modes[i].mode.dmDeviceName), debugstr_devmodew(&modes[i].mode) );
        ret = settings_handler.set_current_mode( ids[i], modes + i );
    }
    for (UINT i = 0; !ret && i < count; i++)
    {
        if (is_detached_mode( &modes[i].mode )) continue;
        TRACE( "  setting %s mode %s\n", debugstr_w(modes[i].mode.dmDeviceName), debugstr_devmodew(&modes[i].mode) );
        ret = settings_handler.set_current_mode( ids[i], modes + i );
    }

    XUngrabServer( gdi_display );
    XFlush( gdi_display );

done:
    free( modes );
    free( ids );
    return ret;
}

POINT virtual_screen_to_root(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect( MDT_RAW_DPI );
    POINT pt;

    pt.x = x - virtual.left;
    pt.y = y - virtual.top;
    return pt;
}

POINT root_to_virtual_screen(INT x, INT y)
{
    RECT virtual = NtUserGetVirtualScreenRect( MDT_RAW_DPI );
    POINT pt;

    pt.x = x + virtual.left;
    pt.y = y + virtual.top;
    return pt;
}

/* Get the primary monitor rect from the host system */
RECT get_host_primary_monitor_rect(void)
{
    INT gpu_count, adapter_count, monitor_count;
    struct x11drv_gpu *gpus = NULL;
    struct x11drv_adapter *adapters = NULL;
    struct gdi_monitor *monitors = NULL;
    RECT rect = {0};

    /* The first monitor is always primary */
    if (host_handler.get_gpus(&gpus, &gpu_count, FALSE) && gpu_count &&
        host_handler.get_adapters(gpus[0].id, &adapters, &adapter_count) && adapter_count &&
        host_handler.get_monitors(adapters[0].id, &monitors, &monitor_count) && monitor_count)
        rect = monitors[0].rc_monitor;

    if (gpus) host_handler.free_gpus( gpus, gpu_count );
    if (adapters) host_handler.free_adapters(adapters);
    if (monitors) host_handler.free_monitors(monitors, monitor_count);
    return rect;
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
    if (host_handler.register_event_handlers) host_handler.register_event_handlers();
}

/* Report whether a display device handler supports detecting dynamic device changes */
BOOL X11DRV_DisplayDevices_SupportEventHandlers(void)
{
    return !!host_handler.register_event_handlers;
}

UINT X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, void *param )
{
    INT gpu_count, adapter_count, monitor_count, current_adapter_count = 0;
    struct x11drv_adapter *adapters;
    struct gdi_monitor *monitors;
    struct x11drv_gpu *gpus;
    INT gpu, adapter, monitor;
    struct x11drv_mode *modes;
    UINT mode_count;

    TRACE( "via %s\n", debugstr_a(host_handler.name) );

    /* Initialize GPUs */
    if (!host_handler.get_gpus( &gpus, &gpu_count, TRUE )) return STATUS_UNSUCCESSFUL;
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = 0; gpu < gpu_count; gpu++)
    {
        device_manager->add_gpu( gpus[gpu].name, &gpus[gpu].pci_id, &gpus[gpu].vulkan_uuid, param );

        /* Initialize adapters */
        if (!host_handler.get_adapters( gpus[gpu].id, &adapters, &adapter_count )) break;
        TRACE( "GPU: %#lx %s, adapter count: %d\n", gpus[gpu].id, debugstr_a( gpus[gpu].name ), adapter_count );

        for (adapter = 0; adapter < adapter_count; adapter++)
        {
            DEVMODEW current_mode = {.dmSize = sizeof(current_mode)};
            WCHAR devname[32];
            char buffer[32];
            x11drv_settings_id settings_id;
            BOOL is_primary = adapters[adapter].state_flags & DISPLAY_DEVICE_PRIMARY_DEVICE;
            UINT dpi = NtUserGetSystemDpiForProcess( NULL );

            sprintf( buffer, "%04lx", adapters[adapter].id );
            device_manager->add_source( buffer, adapters[adapter].state_flags, dpi, param );

            if (!host_handler.get_monitors( adapters[adapter].id, &monitors, &monitor_count )) break;
            TRACE("adapter: %#lx, monitor count: %d\n", adapters[adapter].id, monitor_count);

            /* Initialize monitors */
            for (monitor = 0; monitor < monitor_count; monitor++)
                device_manager->add_monitor( &monitors[monitor], param );

            host_handler.free_monitors( monitors, monitor_count );

            /* Get the settings handler id for the adapter */
            snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d", current_adapter_count + adapter + 1 );
            asciiz_to_unicode( devname, buffer );
            if (!settings_handler.get_id( devname, is_primary, &settings_id )) break;

            settings_handler.get_current_mode( settings_id, &current_mode );
            if (settings_handler.get_modes( settings_id, EDS_ROTATEDMODE, &modes, &mode_count ))
            {
                strip_driver_extra( &modes->mode, mode_count );
                device_manager->add_modes( &current_mode, mode_count, &modes->mode, param );
                free( modes );
            }
        }

        current_adapter_count += adapter_count;
        host_handler.free_adapters( adapters );
    }

    host_handler.free_gpus( gpus, gpu_count );
    return STATUS_SUCCESS;
}
