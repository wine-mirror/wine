/*
 * Unit tests for monitor APIs
 *
 * Copyright 2005 Huw Davies
 * Copyright 2008 Dmitry Timoshkov
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

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "wine/heap.h"
#include <stdio.h>

static HMODULE hdll;
static LONG (WINAPI *pGetDisplayConfigBufferSizes)(UINT32,UINT32*,UINT32*);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static void init_function_pointers(void)
{
    hdll = GetModuleHandleA("user32.dll");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hdll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(GetDisplayConfigBufferSizes)
    GET_PROC(SetThreadDpiAwarenessContext)

#undef GET_PROC
}

static void flush_events(void)
{
    int diff = 1000;
    DWORD time;
    MSG msg;

    time = GetTickCount() + diff;
    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, 200, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc,
                                       LPARAM lparam)
{
    MONITORINFOEXA mi;
    char *primary = (char *)lparam;

    mi.cbSize = sizeof(mi);

    ok(GetMonitorInfoA(hmon, (MONITORINFO*)&mi), "GetMonitorInfo failed\n");
    if (mi.dwFlags & MONITORINFOF_PRIMARY)
        strcpy(primary, mi.szDevice);

    return TRUE;
}

static int adapter_count = 0;
static int monitor_count = 0;

static void test_enumdisplaydevices_adapter(int index, const DISPLAY_DEVICEA *device, DWORD flags)
{
    char buffer[128];
    int number;
    int vendor_id;
    int device_id;
    int subsys_id;
    int revision_id;
    HDC hdc;

    adapter_count++;

    /* DeviceName */
    ok(sscanf(device->DeviceName, "\\\\.\\DISPLAY%d", &number) == 1, "#%d: wrong DeviceName %s\n", index,
       device->DeviceName);

    /* DeviceKey */
    /* \Device\Video? value in HLKM\HARDWARE\DEVICEMAP\VIDEO are not necessarily in order with adapter index.
     * Check format only */
    ok(sscanf(device->DeviceKey, "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Video\\%[^\\]\\%04d", buffer, &number) == 2,
       "#%d: wrong DeviceKey %s\n", index, device->DeviceKey);

    /* DeviceString */
    ok(broken(!*device->DeviceString) || /* XP on Testbot will return an empty string, whereas XP on real machine doesn't. Probably a bug in virtual adapter driver */
       *device->DeviceString, "#%d: expect DeviceString not empty\n", index);

    /* StateFlags */
    if (index == 0)
        ok(device->StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE, "#%d: adapter should be primary\n", index);
    else
        ok(!(device->StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE), "#%d: adapter should not be primary\n", index);

    if (device->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
    {
        /* Test creating DC */
        hdc = CreateDCA(device->DeviceName, NULL, NULL, NULL);
        ok(hdc != NULL, "#%d: failed to CreateDC(\"%s\") err=%d\n", index, device->DeviceName, GetLastError());
        DeleteDC(hdc);
    }

    /* DeviceID */
    /* DeviceID should equal to the first string of HardwareID value data in PCI GPU instance. You can verify this
     * by changing the data and rerun EnumDisplayDevices. But it's difficult to find corresponding PCI device on
     * userland. So here we check the expected format instead. */
    if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
        ok(strlen(device->DeviceID) == 0 || /* vista+ */
           sscanf(device->DeviceID, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
                  &vendor_id, &device_id, &subsys_id, &revision_id) == 4, /* XP/2003 ignores EDD_GET_DEVICE_INTERFACE_NAME */
           "#%d: got %s\n", index, device->DeviceID);
    else
    {
        ok(broken(strlen(device->DeviceID) == 0) || /* XP on Testbot returns an empty string, whereas real machine doesn't */
           sscanf(device->DeviceID, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X", &vendor_id, &device_id, &subsys_id,
                  &revision_id) == 4, "#%d: wrong DeviceID %s\n", index, device->DeviceID);
    }
}

static void test_enumdisplaydevices_monitor(int adapter_index, int monitor_index, const char *adapter_name,
                                            DISPLAY_DEVICEA *device, DWORD flags)
{
    static const char device_key_prefix[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class"
                                            "\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\";
    char monitor_name[32];
    char buffer[128];
    int number;

    monitor_count++;

    /* DeviceName */
    lstrcpyA(monitor_name, adapter_name);
    sprintf(monitor_name + strlen(monitor_name), "\\Monitor%d", monitor_index);
    ok(!strcmp(monitor_name, device->DeviceName), "#%d: expect %s, got %s\n", monitor_index, monitor_name, device->DeviceName);

    /* DeviceString */
    ok(*device->DeviceString, "#%d: expect DeviceString not empty\n", monitor_index);

    /* StateFlags */
    if (adapter_index == 0 && monitor_index == 0)
        ok(device->StateFlags & DISPLAY_DEVICE_ATTACHED, "#%d expect to have a primary monitor attached\n", monitor_index);
    else
        ok(device->StateFlags <= (DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE), "#%d wrong state %#x\n", monitor_index,
           device->StateFlags);

    /* DeviceID */
    CharLowerA(device->DeviceID);
    if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
    {   /* HKLM\SYSTEM\CurrentControlSet\Enum\DISPLAY\[monitor name]\[instance id] GUID_DEVINTERFACE_MONITOR
         *                                                  ^             ^                     ^
         * Expect format                  \\?\DISPLAY#[monitor name]#[instance id]#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7} */
        ok(strlen(device->DeviceID) == 0 || /* vista ~ win7 */
           sscanf(device->DeviceID, "\\\\?\\display#%[^#]#%[^#]#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}", buffer, buffer) == 2 || /* win8+ */
           sscanf(device->DeviceID, "monitor\\%[^\\]\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\%04d", buffer, &number) == 2, /* XP/2003 ignores EDD_GET_DEVICE_INTERFACE_NAME */
           "#%d: wrong DeviceID : %s\n", monitor_index, device->DeviceID);
    }
    else
    {
        /* Expect HarewareID value data + Driver value data in HKLM\SYSTEM\CurrentControlSet\Enum\DISPLAY\[monitor name]\{instance} */
        /* But we don't know which monitor instance this belongs to, so check format instead */
        ok(sscanf(device->DeviceID, "monitor\\%[^\\]\\{4d36e96e-e325-11ce-bfc1-08002be10318}\\%04d", buffer, &number) == 2,
           "#%d: wrong DeviceID : %s\n", monitor_index, device->DeviceID);
    }

    /* DeviceKey */
    lstrcpynA(buffer, device->DeviceKey, sizeof(device_key_prefix));
    ok(!lstrcmpiA(buffer, device_key_prefix), "#%d: wrong DeviceKey : %s\n", monitor_index, device->DeviceKey);
    ok(sscanf(device->DeviceKey + sizeof(device_key_prefix) - 1, "%04d", &number) == 1,
       "#%d wrong DeviceKey : %s\n", monitor_index, device->DeviceKey);
}

static void test_enumdisplaydevices(void)
{
    static const DWORD flags[] = {0, EDD_GET_DEVICE_INTERFACE_NAME};
    DISPLAY_DEVICEA dd;
    char primary_device_name[32];
    char primary_monitor_device_name[32];
    char adapter_name[32];
    int number;
    int flag_index;
    int adapter_index;
    int monitor_index;
    BOOL ret;

    /* Doesn't accept \\.\DISPLAY */
    dd.cb = sizeof(dd);
    ret = EnumDisplayDevicesA("\\\\.\\DISPLAY", 0, &dd, 0);
    ok(!ret, "Expect failure\n");

    /* Enumeration */
    for (flag_index = 0; flag_index < ARRAY_SIZE(flags); flag_index++)
        for (adapter_index = 0; EnumDisplayDevicesA(NULL, adapter_index, &dd, flags[flag_index]); adapter_index++)
        {
            lstrcpyA(adapter_name, dd.DeviceName);

            if (sscanf(adapter_name, "\\\\.\\DISPLAYV%d", &number) == 1)
            {
                skip("Skipping software devices %s:%s\n", adapter_name, dd.DeviceString);
                continue;
            }

            test_enumdisplaydevices_adapter(adapter_index, &dd, flags[flag_index]);

            for (monitor_index = 0; EnumDisplayDevicesA(adapter_name, monitor_index, &dd, flags[flag_index]);
                 monitor_index++)
                test_enumdisplaydevices_monitor(adapter_index, monitor_index, adapter_name, &dd, flags[flag_index]);
        }

    ok(adapter_count > 0, "Expect at least one adapter found\n");
    /* XP on Testbot doesn't report a monitor, whereas XP on real machine does */
    ok(broken(monitor_count == 0) || monitor_count > 0, "Expect at least one monitor found\n");

    ret = EnumDisplayDevicesA(NULL, 0, &dd, 0);
    ok(ret, "Expect success\n");
    lstrcpyA(primary_device_name, dd.DeviceName);

    primary_monitor_device_name[0] = 0;
    ret = EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)primary_monitor_device_name);
    ok(ret, "EnumDisplayMonitors failed\n");
    ok(!strcmp(primary_monitor_device_name, primary_device_name),
       "monitor device name %s, device name %s\n", primary_monitor_device_name,
       primary_device_name);
}

struct vid_mode
{
    DWORD w, h, bpp, freq, fields;
    BOOL must_succeed;
};

static const struct vid_mode vid_modes_test[] = {
    {1024, 768, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY, 0},
    {1024, 768, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT |                 DM_DISPLAYFREQUENCY, 1},
    {1024, 768, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL                      , 0},
    {1024, 768, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT                                      , 1},
    {1024, 768, 0, 0,                                DM_BITSPERPEL                      , 0},
    {1024, 768, 0, 0,                                                DM_DISPLAYFREQUENCY, 0},

    {0, 0, 0, 0, DM_PELSWIDTH, 0},
    {0, 0, 0, 0, DM_PELSHEIGHT, 0},

    {1024, 768, 0, 0, DM_PELSWIDTH, 0},
    {1024, 768, 0, 0, DM_PELSHEIGHT, 0},
    {   0, 768, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},
    {1024,   0, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, 0},

    /* the following test succeeds under XP SP3
    {0, 0, 0, 0, DM_DISPLAYFREQUENCY, 0}
    */
};

struct device_info
{
    DWORD index;
    CHAR name[CCHDEVICENAME];
    DEVMODEA original_mode;
};

static BOOL get_primary_adapter(CHAR *name)
{
    DISPLAY_DEVICEA dd;
    DWORD i;

    dd.cb = sizeof(dd);
    for (i = 0; EnumDisplayDevicesA(NULL, i, &dd, 0); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyA(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

#define expect_dm(a, b, c) _expect_dm(__LINE__, a, b, c)
static void _expect_dm(INT line, DEVMODEA expected, const CHAR *device, DWORD test)
{
    CHAR primary_adapter[CCHDEVICENAME];
    BOOL is_primary;
    DEVMODEA dm;
    BOOL ret;

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    SetLastError(0xdeadbeef);
    ret = EnumDisplaySettingsA(device, ENUM_CURRENT_SETTINGS, &dm);
    ok_(__FILE__, line)(ret, "Device %s test %d EnumDisplaySettingsA failed, error %#x\n", device, test, GetLastError());

    ok(get_primary_adapter(primary_adapter), "Failed to get primary adapter name.\n");
    is_primary = !lstrcmpA(primary_adapter, device);

    todo_wine_if(expected.dmFields & DM_POSITION)
    ok_(__FILE__, line)((dm.dmFields & expected.dmFields) == expected.dmFields,
            "Device %s test %d expect dmFields to contain %#x, got %#x\n", device, test, expected.dmFields, dm.dmFields);
    /* Wine doesn't support changing color depth yet */
    todo_wine_if(expected.dmBitsPerPel != 32 && expected.dmBitsPerPel != 24)
    ok_(__FILE__, line)(dm.dmBitsPerPel == expected.dmBitsPerPel, "Device %s test %d expect dmBitsPerPel %d, got %d\n",
            device, test, expected.dmBitsPerPel, dm.dmBitsPerPel);
    /* Wine currently reports primary monitor settings for non-primary monitors */
    todo_wine_if(!is_primary && dm.dmPelsWidth != expected.dmPelsWidth)
    ok_(__FILE__, line)(dm.dmPelsWidth == expected.dmPelsWidth, "Device %s test %d expect dmPelsWidth %d, got %d\n",
            device, test, expected.dmPelsWidth, dm.dmPelsWidth);
    todo_wine_if(!is_primary && dm.dmPelsHeight != expected.dmPelsHeight)
    ok_(__FILE__, line)(dm.dmPelsHeight == expected.dmPelsHeight, "Device %s test %d expect dmPelsHeight %d, got %d\n",
            device, test, expected.dmPelsHeight, dm.dmPelsHeight);
    todo_wine_if(!is_primary && dm.dmPosition.x != expected.dmPosition.x)
    ok_(__FILE__, line)(dm.dmPosition.x == expected.dmPosition.x, "Device %s test %d expect dmPosition.x %d, got %d\n",
            device, test, expected.dmPosition.x, dm.dmPosition.x);
    todo_wine_if(!is_primary && dm.dmPosition.y != expected.dmPosition.y)
    ok_(__FILE__, line)(dm.dmPosition.y == expected.dmPosition.y, "Device %s test %d expect dmPosition.y %d, got %d\n",
            device, test, expected.dmPosition.y, dm.dmPosition.y);
    todo_wine_if(!is_primary && dm.dmDisplayFrequency != expected.dmDisplayFrequency)
    ok_(__FILE__, line)(dm.dmDisplayFrequency == expected.dmDisplayFrequency,
            "Device %s test %d expect dmDisplayFrequency %d, got %d\n", device, test, expected.dmDisplayFrequency,
            dm.dmDisplayFrequency);
    todo_wine_if(!is_primary && dm.dmDisplayOrientation != expected.dmDisplayOrientation)
    ok_(__FILE__, line)(dm.dmDisplayOrientation == expected.dmDisplayOrientation,
            "Device %s test %d expect dmDisplayOrientation %d, got %d\n", device, test, expected.dmDisplayOrientation,
            dm.dmDisplayOrientation);
}

static void test_ChangeDisplaySettingsEx(void)
{
    DPI_AWARENESS_CONTEXT context = NULL;
    UINT primary, device, side, mode;
    UINT device_size, device_count;
    struct device_info *devices;
    INT count, old_count;
    DISPLAY_DEVICEA dd;
    DEVMODEA dm, dm2;
    POINTL position;
    DEVMODEW dmW;
    LONG res;
    int i;

    /* Test dmDriverExtra */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#x\n", GetLastError());

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    res = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(res, "EnumDisplaySettingsW failed, error %#x\n", GetLastError());

    /* ChangeDisplaySettingsA/W reset dmDriverExtra to 0 */
    dm.dmDriverExtra = 1;
    res = ChangeDisplaySettingsA(&dm, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsA returned unexpected %d\n", res);
    ok(dm.dmDriverExtra == 0, "ChangeDisplaySettingsA didn't reset dmDriverExtra to 0\n");

    dmW.dmDriverExtra = 1;
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW returned unexpected %d\n", res);
    ok(dmW.dmDriverExtra == 0, "ChangeDisplaySettingsW didn't reset dmDriverExtra to 0\n");

    /* ChangeDisplaySettingsExA/W do not modify dmDriverExtra */
    dm.dmDriverExtra = 1;
    res = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %d\n", res);
    ok(dm.dmDriverExtra == 1, "ChangeDisplaySettingsExA shouldn't change dmDriverExtra\n");

    dmW.dmDriverExtra = 1;
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW returned unexpected %d\n", res);
    ok(dmW.dmDriverExtra == 1, "ChangeDisplaySettingsExW shouldn't change dmDriverExtra\n");

    /* Test dmSize */
    /* ChangeDisplaySettingsA/ExA report success even if dmSize is 0 */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#x\n", GetLastError());

    dm.dmSize = 0;
    res = ChangeDisplaySettingsA(&dm, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsA returned unexpected %d\n", res);

    dm.dmSize = 0;
    res = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %d\n", res);

    /* dmSize for ChangeDisplaySettingsW/ExW needs to be at least FIELD_OFFSET(DEVMODEW, dmICMMethod) */
    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    res = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(res, "EnumDisplaySettingsW failed, error %#x\n", GetLastError());

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod) - 1;
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_BADMODE, "ChangeDisplaySettingsW returned %d, expect DISP_CHANGE_BADMODE\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod) - 1;
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADMODE, "ChangeDisplaySettingsExW returned %d, expect DISP_CHANGE_BADMODE\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW returned unexpected %d\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW returned unexpected %d\n", res);

    /* Test clip rectangle after resolution changes */
    /* GetClipCursor always returns result in physical pixels but GetSystemMetrics(SM_CX/CYVIRTUALSCREEN) are not.
     * Set per-monitor aware context so that virtual screen rectangles are in physical pixels */
    if (pSetThreadDpiAwarenessContext)
        context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);

    for (i = 0; i < ARRAY_SIZE(vid_modes_test); i++)
    {
        dm.dmPelsWidth        = vid_modes_test[i].w;
        dm.dmPelsHeight       = vid_modes_test[i].h;
        dm.dmBitsPerPel       = vid_modes_test[i].bpp;
        dm.dmDisplayFrequency = vid_modes_test[i].freq;
        dm.dmFields           = vid_modes_test[i].fields;
        res = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
        ok(vid_modes_test[i].must_succeed ?
           (res == DISP_CHANGE_SUCCESSFUL || res == DISP_CHANGE_RESTART) :
           (res == DISP_CHANGE_SUCCESSFUL || res == DISP_CHANGE_RESTART ||
            res == DISP_CHANGE_BADMODE || res == DISP_CHANGE_BADPARAM),
           "Unexpected ChangeDisplaySettingsExA() return code for vid_modes_test[%d]: %d\n", i, res);

        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            RECT r, r1, virt;

            SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
            if (IsRectEmpty(&virt))  /* NT4 doesn't have SM_CX/YVIRTUALSCREEN */
                SetRect(&virt, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            OffsetRect(&virt, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN));

            /* Resolution change resets clip rect */
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt), "Invalid clip rect: %s\n", wine_dbgstr_rect(&r));

            if (!ClipCursor(NULL)) continue;
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt), "Invalid clip rect: %s\n", wine_dbgstr_rect(&r));

            /* This should always work. Primary monitor is at (0,0) */
            SetRect(&r1, 10, 10, 20, 20);
            ok(ClipCursor(&r1), "ClipCursor() failed\n");
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &r1), "Invalid clip rect: %s\n", wine_dbgstr_rect(&r));
            SetRect(&r1, 10, 10, 10, 10);
            ok(ClipCursor(&r1), "ClipCursor() failed\n");
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &r1), "Invalid clip rect: %s\n", wine_dbgstr_rect(&r));
            SetRect(&r1, 10, 10, 10, 9);
            ok(!ClipCursor(&r1), "ClipCursor() succeeded\n");
            /* Windows bug: further clipping fails once an empty rect is set, so we have to reset it */
            ClipCursor(NULL);

            SetRect(&r1, virt.left - 10, virt.top - 10, virt.right + 20, virt.bottom + 20);
            ok(ClipCursor(&r1), "ClipCursor() failed\n");
            ok(GetClipCursor(&r), "GetClipCursor() failed\n");
            ok(EqualRect(&r, &virt) || broken(EqualRect(&r, &r1)) /* win9x */,
               "Invalid clip rect: %s\n", wine_dbgstr_rect(&r));
            ClipCursor(&virt);
        }
    }
    if (pSetThreadDpiAwarenessContext && context)
        pSetThreadDpiAwarenessContext(context);
    res = ChangeDisplaySettingsExA(NULL, NULL, NULL, CDS_RESET, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "Failed to reset default resolution: %d\n", res);

    /* Save the original mode for all devices so that they can be restored at the end of tests */
    device_count = 0;
    device_size = 2;
    devices = heap_calloc(device_size, sizeof(*devices));
    ok(devices != NULL, "Failed to allocate memory.\n");

    primary = 0;
    memset(&dd, 0, sizeof(dd));
    dd.cb = sizeof(dd);
    for (device = 0; EnumDisplayDevicesA(NULL, device, &dd, 0); ++device)
    {
        INT number;

        /* Skip software devices */
        if (sscanf(dd.DeviceName, "\\\\.\\DISPLAY%d", &number) != 1)
            continue;

        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            primary = device_count;

        if (device_count >= device_size)
        {
            device_size *= 2;
            devices = heap_realloc(devices, device_size * sizeof(*devices));
            ok(devices != NULL, "Failed to reallocate memory.\n");
        }

        devices[device_count].index = device;
        lstrcpyA(devices[device_count].name, dd.DeviceName);
        devices[device_count].original_mode.dmSize = sizeof(devices[device_count].original_mode);
        res = EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devices[device_count].original_mode);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", dd.DeviceName, GetLastError());
        ++device_count;
    }

    /* Make the primary adapter first */
    if (primary)
    {
        struct device_info tmp;
        tmp = devices[0];
        devices[0] = devices[primary];
        devices[primary] = tmp;
    }

    /* Test detaching adapters */
    /* Test that when there is only one adapter, it can't be detached */
    if (device_count == 1)
    {
        old_count = GetSystemMetrics(SM_CMONITORS);

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPosition = devices[0].original_mode.dmPosition;
        res = ChangeDisplaySettingsExA(devices[0].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[0].name, res);
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[0].name, res);

        /* Check that the adapter is still attached */
        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[0].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#x\n", devices[0].name, GetLastError());
        ok(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP, "Expect device %s still attached.\n", devices[0].name);

        count = GetSystemMetrics(SM_CMONITORS);
        ok(count == old_count, "Expect monitor count %d, got %d\n", old_count, count);

        /* Restore registry settings */
        res = ChangeDisplaySettingsExA(devices[0].name, &devices[0].original_mode, NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_BADPARAM) || /* win10 */
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[0].name, res);
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[0].name, res);
    }

    /* Test that only specifying DM_POSITION in dmFields is not enough to detach an adapter */
    if (device_count >= 2)
    {
        old_count = GetSystemMetrics(SM_CMONITORS);

        /* MSDN says set dmFields to DM_POSITION to detach, but DM_PELSWIDTH and DM_PELSHEIGHT are needed actually.
         * To successfully detach adapters, settings have to be saved to the registry first, and then call
         * ChangeDisplaySettingsExA(device, NULL, NULL, 0, NULL) to update settings. Otherwise on some older versions
         * of Windows, e.g., XP and Win7, the adapter doesn't get detached */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION;
        dm.dmPosition = devices[1].original_mode.dmPosition;
        res = ChangeDisplaySettingsExA(devices[1].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[1].name, res);
        res = ChangeDisplaySettingsExA(devices[1].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[1].name, res);

        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[1].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#x\n", devices[1].name, GetLastError());
        ok(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP, "Expect device %s still attached.\n", devices[1].name);

        count = GetSystemMetrics(SM_CMONITORS);
        ok(count == old_count, "Expect monitor count %d, got %d\n", old_count, count);
    }

    /* Detach all non-primary adapters to avoid position conflicts */
    for (device = 1; device < device_count; ++device)
    {
        old_count = GetSystemMetrics(SM_CMONITORS);

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPosition = devices[device].original_mode.dmPosition;
        res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);
        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);

        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[device].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#x\n", devices[device].name, GetLastError());
        todo_wine ok(!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP), "Expect device %s detached.\n", devices[device].name);

        count = GetSystemMetrics(SM_CMONITORS);
        todo_wine ok(count == old_count - 1, "Expect monitor count %d, got %d\n", old_count - 1, count);
    }

    /* Test changing each adapter to every available mode */
    position.x = 0;
    position.y = 0;
    for (device = 0; device < device_count; ++device)
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm, 0); ++mode)
        {
            dm.dmPosition = position;
            dm.dmFields |= DM_POSITION;
            res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_RESET, NULL);
            /* Reattach detached non-primary adapters, otherwise ChangeDisplaySettingsExA with only CDS_RESET fails */
            if (mode == 0 && device)
            {
                todo_wine ok(res == DISP_CHANGE_FAILED, "ChangeDisplaySettingsExA %s mode %d returned unexpected %d\n",
                        devices[device].name, mode, res);
                res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
                ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s mode %d returned unexpected %d\n",
                        devices[device].name, mode, res);
                res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
            }

            ok(res == DISP_CHANGE_SUCCESSFUL ||
                    broken(res == DISP_CHANGE_FAILED), /* TestBots using VGA driver can't change to some modes */
                    "ChangeDisplaySettingsExA %s mode %d returned unexpected %d\n", devices[device].name, mode, res);
            if (res != DISP_CHANGE_SUCCESSFUL)
            {
                win_skip("Failed to change %s to mode %d.\n", devices[device].name, mode);
                continue;
            }

            flush_events();
            expect_dm(dm, devices[device].name, mode);
        }

        /* Restore settings */
        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);

        /* Place the next adapter to the right so that there is no position conflict */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[device].name, GetLastError());
        position.x = dm.dmPosition.x + dm.dmPelsWidth;
    }

    /* Test changing modes by saving settings to the registry first */
    for (device = 0; device < device_count; ++device)
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[device].name, GetLastError());

        /* Find a mode that's different from the current mode */
        memset(&dm2, 0, sizeof(dm2));
        dm2.dmSize = sizeof(dm2);
        for (mode = 0; EnumDisplaySettingsA(devices[device].name, mode, &dm2); ++mode)
        {
            if (dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight)
                break;
        }
        ok(dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight, "Failed to find a different mode.\n");

        dm.dmPelsWidth = dm2.dmPelsWidth;
        dm.dmPelsHeight = dm2.dmPelsHeight;
        dm.dmDisplayFrequency = dm2.dmDisplayFrequency;
        res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);
        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);
        if (res != DISP_CHANGE_SUCCESSFUL)
        {
            win_skip("Failed to change mode for %s.\n", devices[device].name);
            continue;
        }

        flush_events();
        expect_dm(dm, devices[device].name, 0);
    }

    /* Test dmPosition */
    /* First detach all adapters except for the primary and secondary adapters to avoid position conflicts */
    if (device_count >= 3)
    {
        for (device = 2; device < device_count; ++device)
        {
            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
            ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[device].name, GetLastError());

            dm.dmPelsWidth = 0;
            dm.dmPelsHeight = 0;
            dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
            res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);
        }
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %d\n", res);
    }

    if (device_count >= 2)
    {
        /* Query the primary adapter settings */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[0].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[0].name, GetLastError());

        if (res)
        {
            /* Query the secondary adapter settings */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[1].name, GetLastError());
        }

        if (res)
        {
            /* The secondary adapter should be to the right of the primary adapter */
            todo_wine ok(dm2.dmPosition.x == dm.dmPosition.x + dm.dmPelsWidth,
               "Expected dm2.dmPosition.x %d, got %d.\n", dm.dmPosition.x + dm.dmPelsWidth,
               dm2.dmPosition.x);
            ok(dm2.dmPosition.y == dm.dmPosition.y, "Expected dm2.dmPosition.y %d, got %d.\n",
               dm.dmPosition.y, dm2.dmPosition.y);

            /* Test position conflict */
            dm2.dmPosition.x = dm.dmPosition.x - dm2.dmPelsWidth + 1;
            dm2.dmPosition.y = dm.dmPosition.y;
            res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[1].name, res);

            /* Position is changed and automatically moved */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[1].name, GetLastError());
            todo_wine ok((dm2.dmPosition.x == dm.dmPosition.x - dm2.dmPelsWidth),
               "Expected dmPosition.x %d, got %d.\n", dm.dmPosition.x - dm2.dmPelsWidth,
               dm2.dmPosition.x);

            /* Test position with extra space. The extra space will be removed */
            dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth + 1;
            dm2.dmPosition.y = dm.dmPosition.y;
            res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[1].name, res);

            dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth;
            expect_dm(dm2, devices[1].name, 0);

            /* Test placing the secondary adapter to all sides of the primary adapter */
            for (side = 0; side < 4; ++side)
            {
                switch (side)
                {
                /* Bottom */
                case 0:
                    dm2.dmPosition.x = dm.dmPosition.x;
                    dm2.dmPosition.y = dm.dmPosition.y + dm.dmPelsHeight;
                    break;
                /* Left */
                case 1:
                    dm2.dmPosition.x = dm.dmPosition.x - dm2.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y;
                    break;
                /* Top */
                case 2:
                    dm2.dmPosition.x = dm.dmPosition.x;
                    dm2.dmPosition.y = dm.dmPosition.y - dm2.dmPelsHeight;
                    break;
                /* Right */
                case 3:
                    dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y;
                    break;
                }

                res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
                ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s side %d returned unexpected %d\n",
                        devices[1].name, side, res);
                if (res != DISP_CHANGE_SUCCESSFUL)
                {
                    win_skip("ChangeDisplaySettingsExA %s side %d returned unexpected %d.\n", devices[1].name, side, res);
                    continue;
                }

                flush_events();
                expect_dm(dm2, devices[1].name, side);
            }

            /* Test automatic position update when other adapters change resolution */
            /* Find a mode that's different from the current mode */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            for (mode = 0; EnumDisplaySettingsA(devices[0].name, mode, &dm2); ++mode)
            {
                if (dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight)
                    break;
            }
            ok(dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight, "Failed to find a different mode.\n");

            /* Change the primary adapter to a different mode */
            dm = dm2;
            res = ChangeDisplaySettingsExA(devices[0].name, &dm, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[0].name, res);

            /* Now the position of the second adapter should be changed */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[1].name, GetLastError());
            todo_wine ok(dm2.dmPosition.x == dm.dmPelsWidth, "Expect dmPosition.x %d, got %d\n",
                    dm.dmPelsWidth, dm2.dmPosition.x);
        }
        else
        {
            win_skip("EnumDisplaySettingsA failed\n");
        }
    }

    /* Restore all adapters to their original settings */
    for (device = 0; device < device_count; ++device)
    {
        res = ChangeDisplaySettingsExA(devices[device].name, &devices[device].original_mode, NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[device].name, res);
    }
    res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL ||
            broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
            "ChangeDisplaySettingsExA returned unexpected %d\n", res);
    for (device = 0; device < device_count; ++device)
        expect_dm(devices[device].original_mode, devices[device].name, 0);

    heap_free(devices);
}

static void test_monitors(void)
{
    HMONITOR monitor, primary, nearest;
    POINT pt;
    RECT rc;
    MONITORINFO mi;
    MONITORINFOEXA miexa;
    MONITORINFOEXW miexw;
    BOOL ret;
    DWORD i;

    static const struct
    {
        DWORD cbSize;
        BOOL ret;
    } testdatami[] = {
        {0, FALSE},
        {sizeof(MONITORINFO)+1, FALSE},
        {sizeof(MONITORINFO)-1, FALSE},
        {sizeof(MONITORINFO), TRUE},
        {-1, FALSE},
        {0xdeadbeef, FALSE},
    },
    testdatamiexa[] = {
        {0, FALSE},
        {sizeof(MONITORINFOEXA)+1, FALSE},
        {sizeof(MONITORINFOEXA)-1, FALSE},
        {sizeof(MONITORINFOEXA), TRUE},
        {-1, FALSE},
        {0xdeadbeef, FALSE},
    },
    testdatamiexw[] = {
        {0, FALSE},
        {sizeof(MONITORINFOEXW)+1, FALSE},
        {sizeof(MONITORINFOEXW)-1, FALSE},
        {sizeof(MONITORINFOEXW), TRUE},
        {-1, FALSE},
        {0xdeadbeef, FALSE},
    };

    pt.x = pt.y = 0;
    primary = MonitorFromPoint( pt, MONITOR_DEFAULTTOPRIMARY );
    ok( primary != 0, "couldn't get primary monitor\n" );

    monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTONULL );
    ok( !monitor, "got %p, should not get a monitor for an invalid window\n", monitor );
    monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTOPRIMARY );
    ok( monitor == primary, "got %p, should get primary %p for MONITOR_DEFAULTTOPRIMARY\n", monitor, primary );
    monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTONEAREST );
    ok( monitor == primary, "got %p, should get primary %p for MONITOR_DEFAULTTONEAREST\n", monitor, primary );

    SetRect( &rc, 0, 0, 1, 1 );
    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTOPRIMARY );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    /* Empty rect at 0,0 is considered inside the primary monitor */
    SetRect( &rc, 0, 0, -1, -1 );
    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    /* Even if there is a monitor left of the primary, the primary will have the most overlapping area */
    SetRect( &rc, -1, 0, 2, 1 );
    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    /* But the width of the rect doesn't matter if it's empty. */
    SetRect( &rc, -1, 0, 2, -1 );
    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    ok( monitor != primary, "got primary %p\n", monitor );

    /* Search for a monitor that has no others equally near to (left, top-1) */
    SetRect( &rc, -1, -2, 2, 0 );
    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    nearest = primary;
    while (monitor != NULL)
    {
        ok( monitor != primary, "got primary %p\n", monitor );
        nearest = monitor;
        mi.cbSize = sizeof(mi);
        ret = GetMonitorInfoA( monitor, &mi );
        ok( ret, "GetMonitorInfo failed\n" );
        SetRect( &rc, mi.rcMonitor.left-1, mi.rcMonitor.top-2, mi.rcMonitor.left+2, mi.rcMonitor.top );
        monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    }

    /* tests for cbSize in MONITORINFO */
    monitor = MonitorFromWindow( 0, MONITOR_DEFAULTTOPRIMARY );
    for (i = 0; i < ARRAY_SIZE(testdatami); i++)
    {
        memset( &mi, 0, sizeof(mi) );
        mi.cbSize = testdatami[i].cbSize;
        ret = GetMonitorInfoA( monitor, &mi );
        ok( ret == testdatami[i].ret, "GetMonitorInfo returned wrong value\n" );
        if (ret)
            ok( (mi.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag isn't set\n" );
        else
            ok( !(mi.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag is set\n" );

        memset( &miexw, 0, sizeof(miexw) );
        miexw.cbSize = testdatamiexw[i].cbSize;
        ret = GetMonitorInfoW( monitor, (LPMONITORINFO)&miexw );
        ok( ret == testdatamiexw[i].ret, "GetMonitorInfo returned wrong value\n" );
        if (ret)
            ok( (miexw.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag isn't set\n" );
        else
            ok( !(miexw.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag is set\n" );
    }

    /* tests for cbSize in MONITORINFOEXA */
    for (i = 0; i < ARRAY_SIZE(testdatamiexa); i++)
    {
        memset( &miexa, 0, sizeof(miexa) );
        miexa.cbSize = testdatamiexa[i].cbSize;
        ret = GetMonitorInfoA( monitor, (LPMONITORINFO)&miexa );
        ok( ret == testdatamiexa[i].ret, "GetMonitorInfo returned wrong value\n" );
        if (ret)
            ok( (miexa.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag isn't set\n" );
        else
            ok( !(miexa.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag is set\n" );
    }

    /* tests for cbSize in MONITORINFOEXW */
    for (i = 0; i < ARRAY_SIZE(testdatamiexw); i++)
    {
        memset( &miexw, 0, sizeof(miexw) );
        miexw.cbSize = testdatamiexw[i].cbSize;
        ret = GetMonitorInfoW( monitor, (LPMONITORINFO)&miexw );
        ok( ret == testdatamiexw[i].ret, "GetMonitorInfo returned wrong value\n" );
        if (ret)
            ok( (miexw.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag isn't set\n" );
        else
            ok( !(miexw.dwFlags & MONITORINFOF_PRIMARY), "MONITORINFOF_PRIMARY flag is set\n" );
    }

    SetRect( &rc, rc.left+1, rc.top+1, rc.left+2, rc.top+2 );

    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONULL );
    ok( monitor == NULL, "got %p\n", monitor );

    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTOPRIMARY );
    ok( monitor == primary, "got %p, should get primary %p\n", monitor, primary );

    monitor = MonitorFromRect( &rc, MONITOR_DEFAULTTONEAREST );
    ok( monitor == nearest, "got %p, should get nearest %p\n", monitor, nearest );
}

static BOOL CALLBACK find_primary_mon(HMONITOR hmon, HDC hdc, LPRECT rc, LPARAM lp)
{
    MONITORINFO mi;
    BOOL ret;

    mi.cbSize = sizeof(mi);
    ret = GetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo failed\n");
    if (mi.dwFlags & MONITORINFOF_PRIMARY)
    {
        *(HMONITOR *)lp = hmon;
        return FALSE;
    }
    return TRUE;
}

static void test_work_area(void)
{
    HMONITOR hmon;
    MONITORINFO mi;
    RECT rc_work, rc_normal;
    HWND hwnd;
    WINDOWPLACEMENT wp;
    BOOL ret;

    hmon = 0;
    ret = EnumDisplayMonitors(NULL, NULL, find_primary_mon, (LPARAM)&hmon);
    ok(!ret && hmon != 0, "Failed to find primary monitor\n");

    mi.cbSize = sizeof(mi);
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoA(hmon, &mi);
    ok(ret, "GetMonitorInfo error %u\n", GetLastError());
    ok(mi.dwFlags & MONITORINFOF_PRIMARY, "not a primary monitor\n");
    trace("primary monitor %s\n", wine_dbgstr_rect(&mi.rcMonitor));

    SetLastError(0xdeadbeef);
    ret = SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc_work, 0);
    ok(ret, "SystemParametersInfo error %u\n", GetLastError());
    trace("work area %s\n", wine_dbgstr_rect(&rc_work));
    ok(EqualRect(&rc_work, &mi.rcWork), "work area is different\n");

    hwnd = CreateWindowExA(0, "static", NULL, WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,10,10,0,0,0,NULL);
    ok(hwnd != 0, "CreateWindowEx failed\n");

    ret = GetWindowRect(hwnd, &rc_normal);
    ok(ret, "GetWindowRect failed\n");
    trace("normal %s\n", wine_dbgstr_rect(&rc_normal));

    wp.length = sizeof(wp);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "GetWindowPlacement failed\n");
    trace("min: %d,%d max %d,%d normal %s\n", wp.ptMinPosition.x, wp.ptMinPosition.y,
          wp.ptMaxPosition.x, wp.ptMaxPosition.y, wine_dbgstr_rect(&wp.rcNormalPosition));
    OffsetRect(&wp.rcNormalPosition, rc_work.left, rc_work.top);
    todo_wine_if (mi.rcMonitor.left != mi.rcWork.left ||
        mi.rcMonitor.top != mi.rcWork.top)  /* FIXME: remove once Wine is fixed */
    {
        ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");
    }

    SetWindowLongA(hwnd, GWL_EXSTYLE, WS_EX_TOOLWINDOW);

    wp.length = sizeof(wp);
    ret = GetWindowPlacement(hwnd, &wp);
    ok(ret, "GetWindowPlacement failed\n");
    trace("min: %d,%d max %d,%d normal %s\n", wp.ptMinPosition.x, wp.ptMinPosition.y,
          wp.ptMaxPosition.x, wp.ptMaxPosition.y, wine_dbgstr_rect(&wp.rcNormalPosition));
    ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");

    DestroyWindow(hwnd);
}

static void test_display_config(void)
{
    UINT32 paths, modes;
    LONG ret;

    if (!pGetDisplayConfigBufferSizes)
    {
        win_skip("GetDisplayConfigBufferSizes is not supported\n");
        return;
    }

    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = 100;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, &paths, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    ok(paths == 100, "got %u\n", paths);

    modes = 100;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, NULL, &modes);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    ok(modes == 100, "got %u\n", modes);

    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    /* Invalid flags, non-zero invalid flags validation is version (or driver?) dependent,
       it's unreliable to use in tests. */
    ret = pGetDisplayConfigBufferSizes(0, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = 100;
    ret = pGetDisplayConfigBufferSizes(0, &paths, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    ok(paths == 100, "got %u\n", paths);

    modes = 100;
    ret = pGetDisplayConfigBufferSizes(0, NULL, &modes);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    ok(modes == 100, "got %u\n", modes);

    paths = modes = 100;
    ret = pGetDisplayConfigBufferSizes(0, &paths, &modes);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);
    ok((modes == 0 || modes == 100) && paths == 0, "got %u, %u\n", modes, paths);
}

START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
    test_ChangeDisplaySettingsEx();
    test_monitors();
    test_work_area();
    test_display_config();
}
