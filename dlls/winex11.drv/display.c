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

HANDLE get_display_device_init_mutex(void)
{
    static const WCHAR init_mutexW[] = {'d','i','s','p','l','a','y','_','d','e','v','i','c','e','_','i','n','i','t'};
    UNICODE_STRING name = { sizeof(init_mutexW), sizeof(init_mutexW), (WCHAR *)init_mutexW };
    OBJECT_ATTRIBUTES attr;
    HANDLE mutex = 0;

    InitializeObjectAttributes( &attr, &name, OBJ_OPENIF, NULL, NULL );
    NtCreateMutant( &mutex, MUTEX_ALL_ACCESS, &attr, FALSE );
    if (mutex) NtWaitForSingleObject( mutex, FALSE, NULL );
    return mutex;
}

void release_display_device_init_mutex(HANDLE mutex)
{
    NtReleaseMutant( mutex, NULL );
    NtClose( mutex );
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

void X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager,
                                  BOOL force, void *param )
{
    struct x11drv_display_device_handler *handler;
    struct gdi_adapter *adapters;
    struct gdi_monitor *monitors;
    struct gdi_gpu *gpus;
    INT gpu_count, adapter_count, monitor_count;
    INT gpu, adapter, monitor;

    if (!force && !force_display_devices_refresh) return;
    force_display_devices_refresh = FALSE;
    handler = is_virtual_desktop() ? &desktop_handler : &host_handler;

    TRACE("via %s\n", wine_dbgstr_a(handler->name));

    /* Initialize GPUs */
    if (!handler->get_gpus(&gpus, &gpu_count))
        return;
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
        }

        handler->free_adapters(adapters);
    }

    handler->free_gpus(gpus);
}

void X11DRV_DisplayDevices_Init(BOOL force)
{
    UINT32 num_path, num_mode;

    if (force) force_display_devices_refresh = TRUE;
    /* trigger refresh in win32u */
    NtUserGetDisplayConfigBufferSizes( QDC_ONLY_ACTIVE_PATHS, &num_path, &num_mode );
}
