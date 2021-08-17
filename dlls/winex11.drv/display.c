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
#include "winuser.h"
#include "rpc.h"
#include "winreg.h"
#include "initguid.h"
#include "devguid.h"
#include "devpkey.h"
#include "setupapi.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_GPU_LUID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 1);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_OUTPUT_ID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 2);

/* Wine specific properties */
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_GPU_VULKAN_UUID, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5c, 2);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_STATEFLAGS, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 2);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_RCMONITOR, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 3);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_RCWORK, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 4);
DEFINE_DEVPROPKEY(WINE_DEVPROPKEY_MONITOR_ADAPTERNAME, 0x233a9ef3, 0xafc4, 0x4abd, 0xb5, 0x64, 0xc3, 0x2f, 0x21, 0xf1, 0x53, 0x5b, 5);

static const WCHAR driver_date_dataW[] = {'D','r','i','v','e','r','D','a','t','e','D','a','t','a',0};
static const WCHAR driver_dateW[] = {'D','r','i','v','e','r','D','a','t','e',0};
static const WCHAR driver_descW[] = {'D','r','i','v','e','r','D','e','s','c',0};
static const WCHAR displayW[] = {'D','I','S','P','L','A','Y',0};
static const WCHAR pciW[] = {'P','C','I',0};
static const WCHAR video_idW[] = {'V','i','d','e','o','I','D',0};
static const WCHAR symbolic_link_valueW[]= {'S','y','m','b','o','l','i','c','L','i','n','k','V','a','l','u','e',0};
static const WCHAR gpu_idW[] = {'G','P','U','I','D',0};
static const WCHAR monitor_id_fmtW[] = {'M','o','n','i','t','o','r','I','D','%','d',0};
static const WCHAR adapter_name_fmtW[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','%','d',0};
static const WCHAR state_flagsW[] = {'S','t','a','t','e','F','l','a','g','s',0};
static const WCHAR guid_fmtW[] = {
    '{','%','0','8','x','-','%','0','4','x','-','%','0','4','x','-','%','0','2','x','%','0','2','x','-',
    '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x','}',0};
static const WCHAR gpu_instance_fmtW[] = {
    'P','C','I','\\',
    'V','E','N','_','%','0','4','X','&',
    'D','E','V','_','%','0','4','X','&',
    'S','U','B','S','Y','S','_','%','0','8','X','&',
    'R','E','V','_','%','0','2','X','\\',
    '%','0','8','X',0};
static const WCHAR gpu_hardware_id_fmtW[] = {
    'P','C','I','\\',
    'V','E','N','_','%','0','4','X','&',
    'D','E','V','_','%','0','4','X','&',
    'S','U','B','S','Y','S','_','0','0','0','0','0','0','0','0','&',
    'R','E','V','_','0','0',0};
static const WCHAR video_keyW[] = {
    'H','A','R','D','W','A','R','E','\\',
    'D','E','V','I','C','E','M','A','P','\\',
    'V','I','D','E','O',0};
static const WCHAR adapter_key_fmtW[] = {
    'S','y','s','t','e','m','\\',
    'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
    'C','o','n','t','r','o','l','\\',
    'V','i','d','e','o','\\',
    '%','s','\\',
    '%','0','4','x',0};
static const WCHAR device_video_fmtW[] = {
    '\\','D','e','v','i','c','e','\\',
    'V','i','d','e','o','%','d',0};
static const WCHAR machine_prefixW[] = {
    '\\','R','e','g','i','s','t','r','y','\\',
    'M','a','c','h','i','n','e','\\',0};
static const WCHAR nt_classW[] = {
    '\\','R','e','g','i','s','t','r','y','\\',
    'M','a','c','h','i','n','e','\\',
    'S','y','s','t','e','m','\\',
    'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
    'C','o','n','t','r','o','l','\\',
    'C','l','a','s','s','\\',0};
static const WCHAR monitor_instance_fmtW[] = {
    'D','I','S','P','L','A','Y','\\',
    'D','e','f','a','u','l','t','_','M','o','n','i','t','o','r','\\',
    '%','0','4','X','&','%','0','4','X',0};
static const WCHAR monitor_hardware_idW[] = {
    'M','O','N','I','T','O','R','\\',
    'D','e','f','a','u','l','t','_','M','o','n','i','t','o','r',0,0};
static const WCHAR driver_date_fmtW[] = {'%','u','-','%','u','-','%','u',0};

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
    struct x11drv_gpu *gpus = NULL;
    struct x11drv_adapter *adapters = NULL;
    struct x11drv_monitor *monitors = NULL;
    RECT rect = {0};

    /* The first monitor is always primary */
    if (host_handler.get_gpus(&gpus, &gpu_count) && gpu_count &&
        host_handler.get_adapters(gpus[0].id, &adapters, &adapter_count) && adapter_count &&
        host_handler.get_monitors(adapters[0].id, &monitors, &monitor_count) && monitor_count)
        rect = monitors[0].rc_monitor;

    if (gpus) host_handler.free_gpus(gpus);
    if (adapters) host_handler.free_adapters(adapters);
    if (monitors) host_handler.free_monitors(monitors);
    return rect;
}

BOOL get_host_primary_gpu(struct x11drv_gpu *gpu)
{
    struct x11drv_gpu *gpus;
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

/* Initialize a GPU instance.
 * Return its GUID string in guid_string, driver value in driver parameter and LUID in gpu_luid */
static BOOL X11DRV_InitGpu(HDEVINFO devinfo, const struct x11drv_gpu *gpu, INT gpu_index, WCHAR *guid_string,
                           WCHAR *driver, LUID *gpu_luid)
{
    static const BOOL present = TRUE;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    WCHAR instanceW[MAX_PATH];
    DEVPROPTYPE property_type;
    SYSTEMTIME systemtime;
    WCHAR bufferW[1024];
    FILETIME filetime;
    HKEY hkey = NULL;
    GUID guid;
    LUID luid;
    INT written;
    DWORD size;
    BOOL ret = FALSE;

    TRACE("GPU id:0x%s name:%s.\n", wine_dbgstr_longlong(gpu->id), wine_dbgstr_w(gpu->name));

    sprintfW(instanceW, gpu_instance_fmtW, gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id, gpu_index);
    if (!SetupDiOpenDeviceInfoW(devinfo, instanceW, NULL, 0, &device_data))
    {
        SetupDiCreateDeviceInfoW(devinfo, instanceW, &GUID_DEVCLASS_DISPLAY, gpu->name, NULL, 0, &device_data);
        if (!SetupDiRegisterDeviceInfo(devinfo, &device_data, 0, NULL, NULL, NULL))
            goto done;
    }

    /* Write HardwareID registry property, REG_MULTI_SZ */
    written = sprintfW(bufferW, gpu_hardware_id_fmtW, gpu->vendor_id, gpu->device_id);
    bufferW[written + 1] = 0;
    if (!SetupDiSetDeviceRegistryPropertyW(devinfo, &device_data, SPDRP_HARDWAREID, (const BYTE *)bufferW,
                                           (written + 2) * sizeof(WCHAR)))
        goto done;

    /* Write DEVPKEY_Device_IsPresent property */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, DEVPROP_TYPE_BOOLEAN,
                                   (const BYTE *)&present, sizeof(present), 0))
        goto done;

    /* Write DEVPROPKEY_GPU_LUID property */
    if (!SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_GPU_LUID, &property_type,
                                   (BYTE *)&luid, sizeof(luid), NULL, 0))
    {
        if (!AllocateLocallyUniqueId(&luid))
            goto done;

        if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_GPU_LUID,
                                       DEVPROP_TYPE_UINT64, (const BYTE *)&luid, sizeof(luid), 0))
            goto done;
    }
    *gpu_luid = luid;
    TRACE("LUID:%08x:%08x.\n", luid.HighPart, luid.LowPart);

    /* Write WINE_DEVPROPKEY_GPU_VULKAN_UUID property */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_GPU_VULKAN_UUID,
                                   DEVPROP_TYPE_GUID, (const BYTE *)&gpu->vulkan_uuid,
                                   sizeof(gpu->vulkan_uuid), 0))
        goto done;
    TRACE("Vulkan UUID:%s.\n", wine_dbgstr_guid(&gpu->vulkan_uuid));

    /* Open driver key.
     * This is where HKLM\System\CurrentControlSet\Control\Video\{GPU GUID}\{Adapter Index} links to */
    hkey = SetupDiCreateDevRegKeyW(devinfo, &device_data, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);

    /* Write DriverDesc value */
    if (RegSetValueExW(hkey, driver_descW, 0, REG_SZ, (const BYTE *)gpu->name,
                       (strlenW(gpu->name) + 1) * sizeof(WCHAR)))
        goto done;
    /* Write DriverDateData value, using current time as driver date, needed by Evoland */
    GetSystemTimeAsFileTime(&filetime);
    if (RegSetValueExW(hkey, driver_date_dataW, 0, REG_BINARY, (BYTE *)&filetime, sizeof(filetime)))
        goto done;

    GetSystemTime(&systemtime);
    sprintfW(bufferW, driver_date_fmtW, systemtime.wMonth, systemtime.wDay, systemtime.wYear);
    if (RegSetValueExW(hkey, driver_dateW, 0, REG_SZ, (BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
        goto done;

    RegCloseKey(hkey);

    /* Retrieve driver value for adapters */
    if (!SetupDiGetDeviceRegistryPropertyW(devinfo, &device_data, SPDRP_DRIVER, NULL, (BYTE *)bufferW, sizeof(bufferW),
                                           NULL))
        goto done;
    lstrcpyW(driver, nt_classW);
    lstrcatW(driver, bufferW);

    /* Write GUID in VideoID in .../instance/Device Parameters, reuse the GUID if it's existent */
    hkey = SetupDiCreateDevRegKeyW(devinfo, &device_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);

    size = sizeof(bufferW);
    if (RegQueryValueExW(hkey, video_idW, 0, NULL, (BYTE *)bufferW, &size))
    {
        UuidCreate(&guid);
        sprintfW(bufferW, guid_fmtW, guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
                 guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        if (RegSetValueExW(hkey, video_idW, 0, REG_SZ, (const BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
            goto done;
    }
    lstrcpyW(guid_string, bufferW);

    ret = TRUE;
done:
    RegCloseKey(hkey);
    if (!ret)
        ERR("Failed to initialize GPU\n");
    return ret;
}

static BOOL X11DRV_InitAdapter(HKEY video_hkey, INT video_index, INT gpu_index, INT adapter_index, INT monitor_count,
                               const struct x11drv_gpu *gpu, const WCHAR *guid_string,
                               const WCHAR *gpu_driver, const struct x11drv_adapter *adapter)
{
    WCHAR adapter_keyW[MAX_PATH];
    WCHAR key_nameW[MAX_PATH];
    WCHAR bufferW[1024];
    HKEY hkey = NULL;
    BOOL ret = FALSE;
    LSTATUS ls;
    INT i;

    sprintfW(key_nameW, device_video_fmtW, video_index);
    lstrcpyW(bufferW, machine_prefixW);
    sprintfW(adapter_keyW, adapter_key_fmtW, guid_string, adapter_index);
    lstrcatW(bufferW, adapter_keyW);

    /* Write value of \Device\Video? (adapter key) in HKLM\HARDWARE\DEVICEMAP\VIDEO\ */
    if (RegSetValueExW(video_hkey, key_nameW, 0, REG_SZ, (const BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
        goto done;

    /* Create HKLM\System\CurrentControlSet\Control\Video\{GPU GUID}\{Adapter Index} link to GPU driver */
    ls = RegCreateKeyExW(HKEY_LOCAL_MACHINE, adapter_keyW, 0, NULL, REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                         KEY_ALL_ACCESS, NULL, &hkey, NULL);
    if (ls == ERROR_ALREADY_EXISTS)
        RegCreateKeyExW(HKEY_LOCAL_MACHINE, adapter_keyW, 0, NULL, REG_OPTION_VOLATILE | REG_OPTION_OPEN_LINK,
                        KEY_ALL_ACCESS, NULL, &hkey, NULL);
    if (RegSetValueExW(hkey, symbolic_link_valueW, 0, REG_LINK, (const BYTE *)gpu_driver,
                       strlenW(gpu_driver) * sizeof(WCHAR)))
        goto done;
    RegCloseKey(hkey);
    hkey = NULL;

    /* FIXME:
     * Following information is Wine specific, it doesn't really exist on Windows. It is used so that we can
     * implement EnumDisplayDevices etc by querying registry only. This information is most likely reported by the
     * device driver on Windows */
    RegCreateKeyExW(HKEY_CURRENT_CONFIG, adapter_keyW, 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);

    /* Write GPU instance path so that we can find the GPU instance via adapters quickly. Another way is trying to match
     * them via the GUID in Device Parameters/VideoID, but it would require enumerating all GPU instances */
    sprintfW(bufferW, gpu_instance_fmtW, gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id, gpu_index);
    if (RegSetValueExW(hkey, gpu_idW, 0, REG_SZ, (const BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
        goto done;

    /* Write all monitor instances paths under this adapter */
    for (i = 0; i < monitor_count; i++)
    {
        sprintfW(key_nameW, monitor_id_fmtW, i);
        sprintfW(bufferW, monitor_instance_fmtW, video_index, i);
        if (RegSetValueExW(hkey, key_nameW, 0, REG_SZ, (const BYTE *)bufferW, (strlenW(bufferW) + 1) * sizeof(WCHAR)))
            goto done;
    }

    /* Write StateFlags */
    if (RegSetValueExW(hkey, state_flagsW, 0, REG_DWORD, (const BYTE *)&adapter->state_flags,
                       sizeof(adapter->state_flags)))
        goto done;

    ret = TRUE;
done:
    RegCloseKey(hkey);
    if (!ret)
        ERR("Failed to initialize adapter\n");
    return ret;
}

static BOOL X11DRV_InitMonitor(HDEVINFO devinfo, const struct x11drv_monitor *monitor, int monitor_index,
                               int video_index, const LUID *gpu_luid, UINT output_id)
{
    SP_DEVINFO_DATA device_data = {sizeof(SP_DEVINFO_DATA)};
    WCHAR bufferW[MAX_PATH];
    DWORD length;
    HKEY hkey;
    BOOL ret = FALSE;

    /* Create GUID_DEVCLASS_MONITOR instance */
    sprintfW(bufferW, monitor_instance_fmtW, video_index, monitor_index);
    SetupDiCreateDeviceInfoW(devinfo, bufferW, &GUID_DEVCLASS_MONITOR, monitor->name, NULL, 0, &device_data);
    if (!SetupDiRegisterDeviceInfo(devinfo, &device_data, 0, NULL, NULL, NULL))
        goto done;

    /* Write HardwareID registry property */
    if (!SetupDiSetDeviceRegistryPropertyW(devinfo, &device_data, SPDRP_HARDWAREID,
                                           (const BYTE *)monitor_hardware_idW, sizeof(monitor_hardware_idW)))
        goto done;

    /* Write DEVPROPKEY_MONITOR_GPU_LUID */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID,
                                   DEVPROP_TYPE_INT64, (const BYTE *)gpu_luid, sizeof(*gpu_luid), 0))
        goto done;

    /* Write DEVPROPKEY_MONITOR_OUTPUT_ID */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPROPKEY_MONITOR_OUTPUT_ID,
                                   DEVPROP_TYPE_UINT32, (const BYTE *)&output_id, sizeof(output_id), 0))
        goto done;

    /* Create driver key */
    hkey = SetupDiCreateDevRegKeyW(devinfo, &device_data, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    RegCloseKey(hkey);

    /* FIXME:
     * Following properties are Wine specific, see comments in X11DRV_InitAdapter for details */
    /* StateFlags */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_STATEFLAGS, DEVPROP_TYPE_UINT32,
                                   (const BYTE *)&monitor->state_flags, sizeof(monitor->state_flags), 0))
        goto done;
    /* RcMonitor */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_RCMONITOR, DEVPROP_TYPE_BINARY,
                                   (const BYTE *)&monitor->rc_monitor, sizeof(monitor->rc_monitor), 0))
        goto done;
    /* RcWork */
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_RCWORK, DEVPROP_TYPE_BINARY,
                                   (const BYTE *)&monitor->rc_work, sizeof(monitor->rc_work), 0))
        goto done;
    /* Adapter name */
    length = sprintfW(bufferW, adapter_name_fmtW, video_index + 1);
    if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &WINE_DEVPROPKEY_MONITOR_ADAPTERNAME, DEVPROP_TYPE_STRING,
                                   (const BYTE *)bufferW, (length + 1) * sizeof(WCHAR), 0))
        goto done;

    ret = TRUE;
done:
    if (!ret)
        ERR("Failed to initialize monitor\n");
    return ret;
}

static void prepare_devices(HKEY video_hkey)
{
    static const BOOL not_present = FALSE;
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo;
    DWORD i = 0;

    /* Remove all monitors */
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_MONITOR, displayW, NULL, 0);
    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        if (!SetupDiRemoveDevice(devinfo, &device_data))
            ERR("Failed to remove monitor\n");
    }
    SetupDiDestroyDeviceInfoList(devinfo);

    /* Clean up old adapter keys for reinitialization */
    RegDeleteTreeW(video_hkey, NULL);

    /* FIXME:
     * Currently SetupDiGetClassDevsW with DIGCF_PRESENT is unsupported, So we need to clean up not present devices in
     * case application uses SetupDiGetClassDevsW to enumerate devices. Wrong devices could exist in registry as a result
     * of prefix copying or having devices unplugged. But then we couldn't simply delete GPUs because we need to retain
     * the same GUID for the same GPU. */
    i = 0;
    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, pciW, NULL, 0);
    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        if (!SetupDiSetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, DEVPROP_TYPE_BOOLEAN,
                                       (const BYTE *)&not_present, sizeof(not_present), 0))
            ERR("Failed to set GPU present property\n");
    }
    SetupDiDestroyDeviceInfoList(devinfo);
}

static void cleanup_devices(void)
{
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HDEVINFO devinfo;
    DWORD type;
    DWORD i = 0;
    BOOL present;

    devinfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, pciW, NULL, 0);
    while (SetupDiEnumDeviceInfo(devinfo, i++, &device_data))
    {
        present = FALSE;
        SetupDiGetDevicePropertyW(devinfo, &device_data, &DEVPKEY_Device_IsPresent, &type, (BYTE *)&present,
                                  sizeof(present), NULL, 0);
        if (!present && !SetupDiRemoveDevice(devinfo, &device_data))
            ERR("Failed to remove GPU\n");
    }
    SetupDiDestroyDeviceInfoList(devinfo);
}

void X11DRV_DisplayDevices_Init(BOOL force)
{
    HANDLE mutex;
    struct x11drv_display_device_handler *handler = is_virtual_desktop() ? &desktop_handler : &host_handler;
    struct x11drv_gpu *gpus = NULL;
    struct x11drv_adapter *adapters = NULL;
    struct x11drv_monitor *monitors = NULL;
    INT gpu_count, adapter_count, monitor_count;
    INT gpu, adapter, monitor;
    HDEVINFO gpu_devinfo = NULL, monitor_devinfo = NULL;
    HKEY video_hkey = NULL;
    INT video_index = 0;
    DWORD disposition = 0;
    WCHAR guidW[40];
    WCHAR driverW[1024];
    LUID gpu_luid;
    UINT output_id = 0;

    mutex = get_display_device_init_mutex();

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, video_keyW, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &video_hkey,
                        &disposition))
    {
        ERR("Failed to create video device key\n");
        goto done;
    }

    /* Avoid unnecessary reinit */
    if (!force && disposition != REG_CREATED_NEW_KEY)
        goto done;

    TRACE("via %s\n", wine_dbgstr_a(handler->name));

    prepare_devices(video_hkey);

    gpu_devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    monitor_devinfo = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_MONITOR, NULL);

    /* Initialize GPUs */
    if (!handler->get_gpus(&gpus, &gpu_count))
        goto done;
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = 0; gpu < gpu_count; gpu++)
    {
        if (!X11DRV_InitGpu(gpu_devinfo, &gpus[gpu], gpu, guidW, driverW, &gpu_luid))
            goto done;

        /* Initialize adapters */
        if (!handler->get_adapters(gpus[gpu].id, &adapters, &adapter_count))
            goto done;
        TRACE("GPU: %#lx %s, adapter count: %d\n", gpus[gpu].id, wine_dbgstr_w(gpus[gpu].name), adapter_count);

        for (adapter = 0; adapter < adapter_count; adapter++)
        {
            if (!handler->get_monitors(adapters[adapter].id, &monitors, &monitor_count))
                goto done;
            TRACE("adapter: %#lx, monitor count: %d\n", adapters[adapter].id, monitor_count);

            if (!X11DRV_InitAdapter(video_hkey, video_index, gpu, adapter, monitor_count,
                                    &gpus[gpu], guidW, driverW, &adapters[adapter]))
                goto done;

            /* Initialize monitors */
            for (monitor = 0; monitor < monitor_count; monitor++)
            {
                TRACE("monitor: %#x %s\n", monitor, wine_dbgstr_w(monitors[monitor].name));
                if (!X11DRV_InitMonitor(monitor_devinfo, &monitors[monitor], monitor, video_index, &gpu_luid, output_id++))
                    goto done;
            }

            handler->free_monitors(monitors);
            monitors = NULL;
            video_index++;
        }

        handler->free_adapters(adapters);
        adapters = NULL;
    }

done:
    cleanup_devices();
    SetupDiDestroyDeviceInfoList(monitor_devinfo);
    SetupDiDestroyDeviceInfoList(gpu_devinfo);
    RegCloseKey(video_hkey);
    release_display_device_init_mutex(mutex);
    if (gpus)
        handler->free_gpus(gpus);
    if (adapters)
        handler->free_adapters(adapters);
    if (monitors)
        handler->free_monitors(monitors);
}
