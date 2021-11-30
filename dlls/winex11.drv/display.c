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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "rpc.h"
#include "winreg.h"
#include "cfgmgr32.h"
#include "initguid.h"
#include "devguid.h"
#include "devpkey.h"
#include "ntddvdeo.h"
#include "setupapi.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

/* Wine specific properties */
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_RCMONITOR, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 3);

static const WCHAR displayW[] = {'D','I','S','P','L','A','Y',0};
static const WCHAR video_keyW[] = {
    'H','A','R','D','W','A','R','E','\\',
    'D','E','V','I','C','E','M','A','P','\\',
    'V','I','D','E','O',0};

static struct x11drv_display_device_handler host_handler;
struct x11drv_display_device_handler desktop_handler;

/* Cached screen information, protected by screen_section */
static HKEY video_key;
static RECT virtual_screen_rect;
static RECT primary_monitor_rect;
static FILETIME last_query_screen_time;
static CRITICAL_SECTION screen_section;
static CRITICAL_SECTION_DEBUG screen_critsect_debug =
{
    0, 0, &screen_section,
    {&screen_critsect_debug.ProcessLocksList, &screen_critsect_debug.ProcessLocksList},
     0, 0, {(DWORD_PTR)(__FILE__ ": screen_section")}
};
static CRITICAL_SECTION screen_section = {&screen_critsect_debug, -1, 0, 0, 0, 0};

HANDLE get_display_device_init_mutex(void)
{
    static const WCHAR init_mutexW[] = {'d','i','s','p','l','a','y','_','d','e','v','i','c','e','_','i','n','i','t',0};
    HANDLE mutex = CreateMutexW(NULL, FALSE, init_mutexW);

    WaitForSingleObject(mutex, INFINITE);
    return mutex;
}

void release_display_device_init_mutex(HANDLE mutex)
{
    ReleaseMutex(mutex);
    CloseHandle(mutex);
}

/* Update screen rectangle cache from SetupAPI if it's outdated, return FALSE on failure and TRUE on success */
static BOOL update_screen_cache(void)
{
    RECT virtual_rect = {0}, primary_rect = {0}, monitor_rect;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo = INVALID_HANDLE_VALUE;
    FILETIME filetime = {0};
    HANDLE mutex = NULL;
    DWORD i = 0;
    INT result;
    DWORD type;
    BOOL ret = FALSE;

    EnterCriticalSection(&screen_section);
    if ((!video_key && RegOpenKeyW(HKEY_LOCAL_MACHINE, video_keyW, &video_key))
        || RegQueryInfoKeyW(video_key, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, &filetime))
    {
        LeaveCriticalSection(&screen_section);
        return FALSE;
    }
    result = CompareFileTime(&filetime, &last_query_screen_time);
    LeaveCriticalSection(&screen_section);
    if (result < 1)
        return TRUE;

    mutex = get_display_device_init_mutex();

    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, displayW, NULL, DIGCF_PRESENT);
    if (devinfo == INVALID_HANDLE_VALUE)
        goto fail;

    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_RCMONITOR, &type,
                                       (BYTE *)&monitor_rect, sizeof(monitor_rect), NULL, 0))
            goto fail;

        UnionRect(&virtual_rect, &virtual_rect, &monitor_rect);
        if (i == 1)
            primary_rect = monitor_rect;
    }

    EnterCriticalSection(&screen_section);
    virtual_screen_rect = virtual_rect;
    primary_monitor_rect = primary_rect;
    last_query_screen_time = filetime;
    LeaveCriticalSection(&screen_section);
    ret = TRUE;
fail:
    SetupDiDestroyDeviceInfoList(devinfo);
    release_display_device_init_mutex(mutex);
    if (!ret)
        WARN("Update screen cache failed!\n");
    return ret;
}

POINT virtual_screen_to_root(INT x, INT y)
{
    RECT virtual = get_virtual_screen_rect();
    POINT pt;

    pt.x = x - virtual.left;
    pt.y = y - virtual.top;
    return pt;
}

POINT root_to_virtual_screen(INT x, INT y)
{
    RECT virtual = get_virtual_screen_rect();
    POINT pt;

    pt.x = x + virtual.left;
    pt.y = y + virtual.top;
    return pt;
}

RECT get_virtual_screen_rect(void)
{
    RECT virtual;

    update_screen_cache();
    EnterCriticalSection(&screen_section);
    virtual = virtual_screen_rect;
    LeaveCriticalSection(&screen_section);
    return virtual;
}

RECT get_primary_monitor_rect(void)
{
    RECT primary;

    update_screen_cache();
    EnterCriticalSection(&screen_section);
    primary = primary_monitor_rect;
    LeaveCriticalSection(&screen_section);
    return primary;
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

                if (IntersectRect(&work_rect, &work_rect, monitor_rect))
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

            if (IntersectRect(&work_rect, &work_rect, monitor_rect))
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

static BOOL CALLBACK update_windows_on_display_change(HWND hwnd, LPARAM lparam)
{
    struct x11drv_win_data *data;
    UINT mask = (UINT)lparam;

    if (!(data = get_win_data(hwnd)))
        return TRUE;

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
    return TRUE;
}

void X11DRV_DisplayDevices_Update(BOOL send_display_change)
{
    RECT old_virtual_rect, new_virtual_rect;
    DWORD tid, pid;
    HWND foreground;
    UINT mask = 0;

    old_virtual_rect = get_virtual_screen_rect();
    X11DRV_DisplayDevices_Init(TRUE);
    new_virtual_rect = get_virtual_screen_rect();

    /* Calculate XReconfigureWMWindow() mask */
    if (old_virtual_rect.left != new_virtual_rect.left)
        mask |= CWX;
    if (old_virtual_rect.top != new_virtual_rect.top)
        mask |= CWY;

    X11DRV_resize_desktop(send_display_change);
    EnumWindows(update_windows_on_display_change, (LPARAM)mask);

    /* forward clip_fullscreen_window request to the foreground window */
    if ((foreground = GetForegroundWindow()) && (tid = GetWindowThreadProcessId( foreground, &pid )) && pid == GetCurrentProcessId())
    {
        if (tid == GetCurrentThreadId()) clip_fullscreen_window( foreground, TRUE );
        else SendNotifyMessageW( foreground, WM_X11DRV_CLIP_CURSOR_REQUEST, TRUE, TRUE );
    }
}

static BOOL force_display_devices_refresh;

void CDECL X11DRV_UpdateDisplayDevices( const struct gdi_device_manager *device_manager,
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
