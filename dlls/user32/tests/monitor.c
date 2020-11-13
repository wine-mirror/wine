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
static LONG (WINAPI *pQueryDisplayConfig)(UINT32,UINT32*,DISPLAYCONFIG_PATH_INFO*,UINT32*,
                                          DISPLAYCONFIG_MODE_INFO*,DISPLAYCONFIG_TOPOLOGY_ID*);
static LONG (WINAPI *pDisplayConfigGetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER*);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static void init_function_pointers(void)
{
    hdll = GetModuleHandleA("user32.dll");

#define GET_PROC(func) \
    p ## func = (void*)GetProcAddress(hdll, #func); \
    if(!p ## func) \
      trace("GetProcAddress(%s) failed\n", #func);

    GET_PROC(GetDisplayConfigBufferSizes)
    GET_PROC(QueryDisplayConfig)
    GET_PROC(DisplayConfigGetDeviceInfo)
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
    {1024, 768, 0, 1, DM_PELSWIDTH | DM_PELSHEIGHT |                 DM_DISPLAYFREQUENCY, 0},
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

#define expect_dm(a, b, c) _expect_dm(__LINE__, a, b, c)
static void _expect_dm(INT line, const DEVMODEA *expected, const CHAR *device, DWORD test)
{
    DEVMODEA dm;
    BOOL ret;

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    SetLastError(0xdeadbeef);
    ret = EnumDisplaySettingsA(device, ENUM_CURRENT_SETTINGS, &dm);
    ok_(__FILE__, line)(ret, "Device %s test %d EnumDisplaySettingsA failed, error %#x\n", device, test, GetLastError());

    ok_(__FILE__, line)((dm.dmFields & expected->dmFields) == expected->dmFields,
            "Device %s test %d expect dmFields to contain %#x, got %#x\n", device, test, expected->dmFields, dm.dmFields);
    /* Wine doesn't support changing color depth yet */
    todo_wine_if(expected->dmFields & DM_BITSPERPEL && expected->dmBitsPerPel != 32 && expected->dmBitsPerPel != 24)
    ok_(__FILE__, line)(!(expected->dmFields & DM_BITSPERPEL) || dm.dmBitsPerPel == expected->dmBitsPerPel,
            "Device %s test %d expect dmBitsPerPel %u, got %u\n", device, test, expected->dmBitsPerPel, dm.dmBitsPerPel);
    ok_(__FILE__, line)(!(expected->dmFields & DM_PELSWIDTH) || dm.dmPelsWidth == expected->dmPelsWidth,
            "Device %s test %d expect dmPelsWidth %u, got %u\n", device, test, expected->dmPelsWidth, dm.dmPelsWidth);
    ok_(__FILE__, line)(!(expected->dmFields & DM_PELSHEIGHT) || dm.dmPelsHeight == expected->dmPelsHeight,
            "Device %s test %d expect dmPelsHeight %u, got %u\n", device, test, expected->dmPelsHeight, dm.dmPelsHeight);
    ok_(__FILE__, line)(!(expected->dmFields & DM_POSITION) || dm.dmPosition.x == expected->dmPosition.x,
            "Device %s test %d expect dmPosition.x %d, got %d\n", device, test, expected->dmPosition.x, dm.dmPosition.x);
    ok_(__FILE__, line)(!(expected->dmFields & DM_POSITION) || dm.dmPosition.y == expected->dmPosition.y,
            "Device %s test %d expect dmPosition.y %d, got %d\n", device, test, expected->dmPosition.y, dm.dmPosition.y);
    ok_(__FILE__, line)(!(expected->dmFields & DM_DISPLAYFREQUENCY) ||
            dm.dmDisplayFrequency == expected->dmDisplayFrequency,
            "Device %s test %d expect dmDisplayFrequency %u, got %u\n", device, test, expected->dmDisplayFrequency,
            dm.dmDisplayFrequency);
    ok_(__FILE__, line)(!(expected->dmFields & DM_DISPLAYORIENTATION) ||
            dm.dmDisplayOrientation == expected->dmDisplayOrientation,
            "Device %s test %d expect dmDisplayOrientation %d, got %d\n", device, test, expected->dmDisplayOrientation,
            dm.dmDisplayOrientation);
}

static void test_ChangeDisplaySettingsEx(void)
{
    static const DWORD registry_fields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
            DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    static const DWORD depths[] = {8, 16, 32};
    DPI_AWARENESS_CONTEXT context = NULL;
    UINT primary, device, test, mode;
    UINT device_size, device_count;
    struct device_info *devices;
    DEVMODEA dm, dm2, dm3;
    INT count, old_count;
    DISPLAY_DEVICEA dd;
    POINTL position;
    DEVMODEW dmW;
    LONG res;
    int i;

    /* Test invalid device names */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#x\n", GetLastError());

    res = ChangeDisplaySettingsExA("invalid", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %d\n", res);

    res = ChangeDisplaySettingsExA("\\\\.\\DISPLAY0", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %d\n", res);

    res = ChangeDisplaySettingsExA("\\\\.\\DISPLAY1\\Monitor0", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %d\n", res);

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
        memset(&devices[device_count].original_mode, 0, sizeof(devices[device_count].original_mode));
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

    /* Test changing to a mode with depth set but with zero width and height */
    for (device = 0; device < device_count; ++device)
    {
        for (test = 0; test < ARRAY_SIZE(depths); ++test)
        {
            /* Find the native resolution */
            memset(&dm, 0, sizeof(dm));
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm2, 0); ++mode)
            {
                if (dm2.dmBitsPerPel == depths[test]
                    && dm2.dmPelsWidth > dm.dmPelsWidth && dm2.dmPelsHeight > dm.dmPelsHeight)
                    dm = dm2;
            }
            if (dm.dmBitsPerPel != depths[test])
            {
                skip("Depth %u is unsupported for %s.\n", depths[test], devices[device].name);
                continue;
            }

            /* Find the second resolution */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm2, 0); ++mode)
            {
                if (dm2.dmBitsPerPel == depths[test]
                    && dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight)
                    break;
            }
            if (dm2.dmBitsPerPel != depths[test]
                || dm2.dmPelsWidth == dm.dmPelsWidth || dm2.dmPelsHeight == dm.dmPelsHeight)
            {
                skip("Failed to find the second mode for %s.\n", devices[device].name);
                continue;
            }

            /* Find the third resolution */
            memset(&dm3, 0, sizeof(dm3));
            dm3.dmSize = sizeof(dm3);
            for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm3, 0); ++mode)
            {
                if (dm3.dmBitsPerPel == depths[test]
                    && dm3.dmPelsWidth != dm.dmPelsWidth && dm3.dmPelsHeight != dm.dmPelsHeight
                    && dm3.dmPelsWidth != dm2.dmPelsWidth && dm3.dmPelsHeight != dm2.dmPelsHeight)
                    break;
            }
            if (dm3.dmBitsPerPel != depths[test]
                || dm3.dmPelsWidth == dm.dmPelsWidth || dm3.dmPelsHeight == dm.dmPelsHeight
                || dm3.dmPelsWidth == dm2.dmPelsWidth || dm3.dmPelsHeight == dm2.dmPelsHeight)
            {
                skip("Failed to find the third mode for %s.\n", devices[device].name);
                continue;
            }

            /* Change the current mode to the third mode first */
            res = ChangeDisplaySettingsExA(devices[device].name, &dm3, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL
               || broken(res == DISP_CHANGE_FAILED), /* Win8 TestBots */
               "ChangeDisplaySettingsExA %s returned unexpected %d.\n", devices[device].name, res);
            if (res != DISP_CHANGE_SUCCESSFUL)
            {
                win_skip("Failed to change display mode for %s.\n", devices[device].name);
                continue;
            }
            flush_events();
            expect_dm(&dm3, devices[device].name, test);

            /* Change the registry mode to the second mode */
            res = ChangeDisplaySettingsExA(devices[device].name, &dm2, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL
               || broken(res == DISP_CHANGE_BADFLAGS), /* Win10 32bit */
               "ChangeDisplaySettingsExA %s returned unexpected %d.\n", devices[device].name, res);

            /* Change to a mode with depth set but with zero width and height */
            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            dm.dmBitsPerPel = depths[test];
            res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d.\n",
               devices[device].name, res);
            flush_events();

            dd.cb = sizeof(dd);
            res = EnumDisplayDevicesA(NULL, devices[device].index, &dd, 0);
            ok(res, "EnumDisplayDevicesA %s failed, error %#x.\n", devices[device].name, GetLastError());
            ok(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP, "Expect %s attached.\n",
               devices[device].name);

            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
            ok(res, "Device %s EnumDisplaySettingsA failed, error %#x.\n", devices[device].name, GetLastError());
            todo_wine_if(depths[test] != 32)
            ok(dm.dmBitsPerPel == depths[test], "Device %s expect dmBitsPerPel %u, got %u.\n",
               devices[device].name, depths[test], dm.dmBitsPerPel);
            /* 2008 resets to the resolution in the registry. Newer versions of Windows doesn't
             * change the current resolution */
            ok(dm.dmPelsWidth == dm3.dmPelsWidth || broken(dm.dmPelsWidth == dm2.dmPelsWidth),
               "Device %s expect dmPelsWidth %u, got %u.\n",
               devices[device].name, dm3.dmPelsWidth, dm.dmPelsWidth);
            ok(dm.dmPelsHeight == dm3.dmPelsHeight || broken(dm.dmPelsHeight == dm2.dmPelsHeight),
               "Device %s expect dmPelsHeight %u, got %u.\n",
               devices[device].name, dm3.dmPelsHeight, dm.dmPelsHeight);
        }
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
        ok(!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP), "Expect device %s detached.\n", devices[device].name);

        count = GetSystemMetrics(SM_CMONITORS);
        ok(count == old_count - 1, "Expect monitor count %d, got %d\n", old_count - 1, count);
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
            expect_dm(&dm, devices[device].name, mode);
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
        /* Place adapter to the right */
        if (device == 0)
        {
            position.x = 0;
            position.y = 0;
        }
        else
        {
            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            res = EnumDisplaySettingsA(devices[device - 1].name, ENUM_CURRENT_SETTINGS, &dm);
            ok(res, "EnumDisplaySettingsA %s failed, error %#x.\n", devices[device - 1].name, GetLastError());
            position.x = dm.dmPosition.x + dm.dmPelsWidth;
        }

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x\n", devices[device].name, GetLastError());
        dm3 = dm;

        /* Find a mode that's different from the current mode */
        memset(&dm2, 0, sizeof(dm2));
        dm2.dmSize = sizeof(dm2);
        for (mode = 0; EnumDisplaySettingsA(devices[device].name, mode, &dm2); ++mode)
        {
            /* Use the same color depth because the win2008 TestBots are unable to change it */
            if (dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight &&
                    dm2.dmBitsPerPel == dm.dmBitsPerPel)
                break;
        }
        ok(dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight &&
                dm2.dmBitsPerPel == dm.dmBitsPerPel, "Failed to find a different mode.\n");

        /* Test normal operation */
        dm = dm2;
        dm.dmFields |= DM_POSITION;
        dm.dmPosition = position;
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
        expect_dm(&dm, devices[device].name, 0);

        /* Test specifying only position, width and height */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPosition = position;
        dm.dmPelsWidth = dm3.dmPelsWidth;
        dm.dmPelsHeight = dm3.dmPelsHeight;
        res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned %d.\n", devices[device].name, res);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_REGISTRY_SETTINGS, &dm);
        /* Win10 either returns failure here or retrieves outdated display settings until they're applied */
        if (res)
        {
            ok((dm.dmFields & registry_fields) == registry_fields, "Got unexpected dmFields %#x.\n", dm.dmFields);
            ok(dm.dmPosition.x == position.x, "Expected dmPosition.x %d, got %d.\n", position.x, dm.dmPosition.x);
            ok(dm.dmPosition.y == position.y, "Expected dmPosition.y %d, got %d.\n", position.y, dm.dmPosition.y);
            ok(dm.dmPelsWidth == dm3.dmPelsWidth || broken(dm.dmPelsWidth == dm2.dmPelsWidth), /* Win10 */
                    "Expected dmPelsWidth %u, got %u.\n", dm3.dmPelsWidth, dm.dmPelsWidth);
            ok(dm.dmPelsHeight == dm3.dmPelsHeight || broken(dm.dmPelsHeight == dm2.dmPelsHeight), /* Win10 */
                    "Expected dmPelsHeight %u, got %u.\n", dm3.dmPelsHeight, dm.dmPelsHeight);
            ok(dm.dmBitsPerPel, "Expected dmBitsPerPel not zero.\n");
            ok(dm.dmDisplayFrequency, "Expected dmDisplayFrequency not zero.\n");
        }
        else
        {
            win_skip("EnumDisplaySettingsA %s failed, error %#x.\n", devices[device].name, GetLastError());
        }

        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned %d.\n", devices[device].name, res);
        flush_events();

        res = EnumDisplaySettingsA(devices[device].name, ENUM_REGISTRY_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x.\n", devices[device].name, GetLastError());
        ok((dm.dmFields & registry_fields) == registry_fields, "Got unexpected dmFields %#x.\n", dm.dmFields);
        ok(dm.dmPosition.x == position.x, "Expected dmPosition.x %d, got %d.\n", position.x, dm.dmPosition.x);
        ok(dm.dmPosition.y == position.y, "Expected dmPosition.y %d, got %d.\n", position.y, dm.dmPosition.y);
        ok(dm.dmPelsWidth == dm3.dmPelsWidth, "Expected dmPelsWidth %u, got %u.\n", dm3.dmPelsWidth, dm.dmPelsWidth);
        ok(dm.dmPelsHeight == dm3.dmPelsHeight, "Expected dmPelsHeight %u, got %u.\n", dm3.dmPelsHeight,
                dm.dmPelsHeight);
        ok(dm.dmBitsPerPel, "Expected dmBitsPerPel not zero.\n");
        ok(dm.dmDisplayFrequency, "Expected dmDisplayFrequency not zero.\n");

        expect_dm(&dm, devices[device].name, 0);
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
            ok(dm2.dmPosition.x == dm.dmPosition.x + dm.dmPelsWidth,
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
            ok(dm2.dmPosition.x == dm.dmPosition.x - dm2.dmPelsWidth,
               "Expected dmPosition.x %d, got %d.\n", dm.dmPosition.x - dm2.dmPelsWidth,
               dm2.dmPosition.x);

            /* Test position with extra space. The extra space will be removed */
            dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth + 1;
            dm2.dmPosition.y = dm.dmPosition.y;
            res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d\n", devices[1].name, res);

            dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth;
            expect_dm(&dm2, devices[1].name, 0);

            /* Test placing the secondary adapter to all sides of the primary adapter */
            for (test = 0; test < 8; ++test)
            {
                switch (test)
                {
                /* Bottom side with x offset */
                case 0:
                    dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth / 2;
                    dm2.dmPosition.y = dm.dmPosition.y + dm.dmPelsHeight;
                    break;
                /* Left side with y offset */
                case 1:
                    dm2.dmPosition.x = dm.dmPosition.x - dm2.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y + dm.dmPelsHeight / 2;
                    break;
                /* Top side with x offset */
                case 2:
                    dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth / 2;
                    dm2.dmPosition.y = dm.dmPosition.y - dm2.dmPelsHeight;
                    break;
                /* Right side with y offset */
                case 3:
                    dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y + dm.dmPelsHeight / 2;
                    break;
                /* Bottom side with the same x */
                case 4:
                    dm2.dmPosition.x = dm.dmPosition.x;
                    dm2.dmPosition.y = dm.dmPosition.y + dm.dmPelsHeight;
                    break;
                /* Left side with the same y */
                case 5:
                    dm2.dmPosition.x = dm.dmPosition.x - dm2.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y;
                    break;
                /* Top side with the same x */
                case 6:
                    dm2.dmPosition.x = dm.dmPosition.x;
                    dm2.dmPosition.y = dm.dmPosition.y - dm2.dmPelsHeight;
                    break;
                /* Right side with the same y */
                case 7:
                    dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth;
                    dm2.dmPosition.y = dm.dmPosition.y;
                    break;
                }

                res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
                ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s test %d returned unexpected %d\n",
                        devices[1].name, test, res);
                if (res != DISP_CHANGE_SUCCESSFUL)
                {
                    win_skip("ChangeDisplaySettingsExA %s test %d returned unexpected %d.\n", devices[1].name, test, res);
                    continue;
                }

                flush_events();
                expect_dm(&dm2, devices[1].name, test);
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
            ok(dm2.dmPosition.x == dm.dmPelsWidth, "Expect dmPosition.x %d, got %d\n",
                    dm.dmPelsWidth, dm2.dmPosition.x);
        }
        else
        {
            win_skip("EnumDisplaySettingsA failed\n");
        }
    }

    /* Test changing each adapter to every supported display orientation */
    for (device = 0; device < device_count; ++device)
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#x.\n", devices[device].name, GetLastError());

        memset(&dm2, 0, sizeof(dm2));
        dm2.dmSize = sizeof(dm2);
        for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm2, EDS_ROTATEDMODE); ++mode)
        {
            if (dm2.dmBitsPerPel != dm.dmBitsPerPel || dm2.dmDisplayFrequency != dm.dmDisplayFrequency)
                continue;

            if ((dm2.dmDisplayOrientation == DMDO_DEFAULT || dm2.dmDisplayOrientation == DMDO_180)
                    && (dm2.dmPelsWidth != dm.dmPelsWidth || dm2.dmPelsHeight != dm.dmPelsHeight))
                continue;

            if ((dm2.dmDisplayOrientation == DMDO_90 || dm2.dmDisplayOrientation == DMDO_270)
                    && (dm2.dmPelsWidth != dm.dmPelsHeight || dm2.dmPelsHeight != dm.dmPelsWidth))
                continue;

            res = ChangeDisplaySettingsExA(devices[device].name, &dm2, NULL, CDS_RESET, NULL);
            if (res != DISP_CHANGE_SUCCESSFUL)
            {
                win_skip("Failed to change %s to mode %d.\n", devices[device].name, mode);
                continue;
            }
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s mode %d returned unexpected %d.\n",
                    devices[device].name, mode, res);
            flush_events();
            expect_dm(&dm2, devices[device].name, mode);

            /* EnumDisplaySettingsEx without EDS_ROTATEDMODE reports modes with current orientation */
            memset(&dm3, 0, sizeof(dm3));
            dm3.dmSize = sizeof(dm3);
            for (i = 0; EnumDisplaySettingsExA(devices[device].name, i, &dm3, 0); ++i)
            {
                ok(dm3.dmDisplayOrientation == dm2.dmDisplayOrientation,
                        "Expected %s display mode %d orientation %d, got %d.\n",
                        devices[device].name, i, dm2.dmDisplayOrientation, dm3.dmDisplayOrientation);
            }
            ok(i > 0, "Expected at least one display mode found.\n");

            if (device == 0)
            {
                ok(GetSystemMetrics(SM_CXSCREEN) == dm2.dmPelsWidth, "Expected %d, got %d.\n",
                        dm2.dmPelsWidth, GetSystemMetrics(SM_CXSCREEN));
                ok(GetSystemMetrics(SM_CYSCREEN) == dm2.dmPelsHeight, "Expected %d, got %d.\n",
                        dm2.dmPelsHeight, GetSystemMetrics(SM_CYSCREEN));
            }

            if (device_count == 1)
            {
                ok(GetSystemMetrics(SM_CXVIRTUALSCREEN) == dm2.dmPelsWidth, "Expected %d, got %d.\n",
                        dm2.dmPelsWidth, GetSystemMetrics(SM_CXVIRTUALSCREEN));
                ok(GetSystemMetrics(SM_CYVIRTUALSCREEN) == dm2.dmPelsHeight, "Expected %d, got %d.\n",
                        dm2.dmPelsHeight, GetSystemMetrics(SM_CYVIRTUALSCREEN));
            }
        }
        ok(mode > 0, "Expected at least one display mode found.\n");
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
        expect_dm(&devices[device].original_mode, devices[device].name, 0);

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

static void test_GetDisplayConfigBufferSizes(void)
{
    UINT32 paths, modes;
    LONG ret;

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

    /* Flag validation on Windows is driver-dependent */
    paths = modes = 100;
    ret = pGetDisplayConfigBufferSizes(0, &paths, &modes);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);
    ok((modes == 0 || modes == 100) && paths == 0, "got %u, %u\n", modes, paths);

    paths = modes = 100;
    ret = pGetDisplayConfigBufferSizes(0xFF, &paths, &modes);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);
    ok((modes == 0 || modes == 100) && paths == 0, "got %u, %u\n", modes, paths);

    /* Test success */
    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_DATABASE_CURRENT, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);
}

static BOOL CALLBACK test_EnumDisplayMonitors_normal_cb(HMONITOR monitor, HDC hdc, LPRECT rect,
        LPARAM lparam)
{
    MONITORINFO mi;
    LONG ret;

    mi.cbSize = sizeof(mi);
    ret = GetMonitorInfoA(monitor, &mi);
    ok(ret, "GetMonitorInfoA failed, error %#x.\n", GetLastError());
    ok(EqualRect(rect, &mi.rcMonitor), "Expected rect %s, got %s.\n",
            wine_dbgstr_rect(&mi.rcMonitor), wine_dbgstr_rect(rect));

    return TRUE;
}

static BOOL CALLBACK test_EnumDisplayMonitors_return_false_cb(HMONITOR monitor, HDC hdc,
        LPRECT rect, LPARAM lparam)
{
    return FALSE;
}

static BOOL CALLBACK test_EnumDisplayMonitors_invalid_handle_cb(HMONITOR monitor, HDC hdc,
        LPRECT rect, LPARAM lparam)
{
    MONITORINFOEXA mi, mi2;
    DEVMODEA old_dm, dm;
    DWORD error;
    INT count;
    LONG ret;

    mi.cbSize = sizeof(mi);
    ret = GetMonitorInfoA(monitor, (MONITORINFO *)&mi);
    ok(ret, "GetMonitorInfoA failed, error %#x.\n", GetLastError());

    /* Test that monitor handle is invalid after the monitor is detached */
    if (!(mi.dwFlags & MONITORINFOF_PRIMARY))
    {
        count = GetSystemMetrics(SM_CMONITORS);

        /* Save current display settings */
        memset(&old_dm, 0, sizeof(old_dm));
        old_dm.dmSize = sizeof(old_dm);
        ret = EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &old_dm);
        ok(ret, "EnumDisplaySettingsA %s failed, error %#x.\n", mi.szDevice, GetLastError());

        /* Detach monitor */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPosition.x = mi.rcMonitor.left;
        dm.dmPosition.y = mi.rcMonitor.top;
        ret = ChangeDisplaySettingsExA(mi.szDevice, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET,
                NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d.\n",
                mi.szDevice, ret);
        ret = ChangeDisplaySettingsExA(mi.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d.\n",
                mi.szDevice, ret);

        /* Check if it's really detached */
        if (GetSystemMetrics(SM_CMONITORS) != count - 1)
        {
            skip("Failed to detach %s.\n", mi.szDevice);
            SetLastError(0xdeadbeef);
            return TRUE;
        }

        /* The monitor handle should be invalid now */
        mi2.cbSize = sizeof(mi2);
        SetLastError(0xdeadbeef);
        ret = GetMonitorInfoA(monitor, (MONITORINFO *)&mi2);
        ok(!ret, "GetMonitorInfoA succeeded.\n");
        error = GetLastError();
        ok(error == ERROR_INVALID_MONITOR_HANDLE || error == ERROR_INVALID_HANDLE,
               "Expected error %#x, got %#x.\n", ERROR_INVALID_MONITOR_HANDLE, error);

        /* Restore the original display settings */
        ret = ChangeDisplaySettingsExA(mi.szDevice, &old_dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET,
                NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d.\n",
                mi.szDevice, ret);
        ret = ChangeDisplaySettingsExA(mi.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %d.\n",
                mi.szDevice, ret);
    }

    SetLastError(0xdeadbeef);
    return TRUE;
}

static void test_EnumDisplayMonitors(void)
{
    DWORD error;
    INT count;
    BOOL ret;

    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_normal_cb, 0);
    ok(ret, "EnumDisplayMonitors failed, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_return_false_cb, 0);
    error = GetLastError();
    ok(!ret, "EnumDisplayMonitors succeeded.\n");
    ok(error == 0xdeadbeef, "Expected error %#x, got %#x.\n", 0xdeadbeef, error);

    count = GetSystemMetrics(SM_CMONITORS);
    SetLastError(0xdeadbeef);
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_invalid_handle_cb, 0);
    error = GetLastError();
    if (count >= 2)
        todo_wine ok(!ret, "EnumDisplayMonitors succeeded.\n");
    else
        ok(ret, "EnumDisplayMonitors failed.\n");
    ok(error == 0xdeadbeef, "Expected error %#x, got %#x.\n", 0xdeadbeef, error);
}

static void test_QueryDisplayConfig_result(UINT32 flags,
        UINT32 paths, const DISPLAYCONFIG_PATH_INFO *pi, UINT32 modes, const DISPLAYCONFIG_MODE_INFO *mi)
{
    UINT32 i;
    LONG ret;
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name;
    DISPLAYCONFIG_TARGET_DEVICE_NAME target_name;
    DISPLAYCONFIG_TARGET_PREFERRED_MODE preferred_mode;
    DISPLAYCONFIG_ADAPTER_NAME adapter_name;

    for (i = 0; i < paths; i++)
    {
        source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        source_name.header.size = sizeof(source_name);
        source_name.header.adapterId = pi[i].sourceInfo.adapterId;
        source_name.header.id = pi[i].sourceInfo.id;
        source_name.viewGdiDeviceName[0] = '\0';
        ret = pDisplayConfigGetDeviceInfo(&source_name.header);
        ok(!ret, "Expected 0, got %d\n", ret);
        ok(source_name.viewGdiDeviceName[0] != '\0', "Expected GDI device name, got empty string\n");

        /* Test with an invalid adapter LUID */
        source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        source_name.header.size = sizeof(source_name);
        source_name.header.adapterId.LowPart = 0xFFFF;
        source_name.header.adapterId.HighPart = 0xFFFF;
        source_name.header.id = pi[i].sourceInfo.id;
        ret = pDisplayConfigGetDeviceInfo(&source_name.header);
        ok(ret == ERROR_GEN_FAILURE, "Expected GEN_FAILURE, got %d\n", ret);

        todo_wine {
        target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        target_name.header.size = sizeof(target_name);
        target_name.header.adapterId = pi[i].targetInfo.adapterId;
        target_name.header.id = pi[i].targetInfo.id;
        target_name.monitorDevicePath[0] = '\0';
        ret = pDisplayConfigGetDeviceInfo(&target_name.header);
        ok(!ret, "Expected 0, got %d\n", ret);
        ok(target_name.monitorDevicePath[0] != '\0', "Expected monitor device path, got empty string\n");
        }

        todo_wine {
        preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
        preferred_mode.header.size = sizeof(preferred_mode);
        preferred_mode.header.adapterId = pi[i].targetInfo.adapterId;
        preferred_mode.header.id = pi[i].targetInfo.id;
        preferred_mode.width = preferred_mode.height = 0;
        ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
        ok(!ret, "Expected 0, got %d\n", ret);
        ok(preferred_mode.width > 0 && preferred_mode.height > 0, "Expected non-zero height/width, got %ux%u\n",
                preferred_mode.width, preferred_mode.height);
        }

        todo_wine {
        adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapter_name.header.size = sizeof(adapter_name);
        adapter_name.header.adapterId = pi[i].sourceInfo.adapterId;
        adapter_name.adapterDevicePath[0] = '\0';
        ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
        ok(!ret, "Expected 0, got %d\n", ret);
        ok(adapter_name.adapterDevicePath[0] != '\0', "Expected adapter device path, got empty string\n");
        }

        /* Check corresponding modes */
        if (pi[i].sourceInfo.modeInfoIdx == DISPLAYCONFIG_PATH_MODE_IDX_INVALID)
        {
            skip("Path doesn't contain source modeInfoIdx");
            continue;
        }
        ok(pi[i].sourceInfo.modeInfoIdx < modes, "Expected index <%d, got %d\n", modes, pi[i].sourceInfo.modeInfoIdx);
        if (pi[i].sourceInfo.modeInfoIdx >= modes)
            continue;

        ok(mi[pi[i].sourceInfo.modeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE, "Expected infoType %d, got %d\n",
                DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE, mi[pi[i].sourceInfo.modeInfoIdx].infoType);
        ok(pi[i].sourceInfo.id == mi[pi[i].sourceInfo.modeInfoIdx].id, "Expected id %u, got %u\n",
                pi[i].sourceInfo.id, mi[pi[i].sourceInfo.modeInfoIdx].id);
        ok(pi[i].sourceInfo.adapterId.HighPart == mi[pi[i].sourceInfo.modeInfoIdx].adapterId.HighPart &&
           pi[i].sourceInfo.adapterId.LowPart == mi[pi[i].sourceInfo.modeInfoIdx].adapterId.LowPart,
                "Expected LUID %08x:%08x, got %08x:%08x\n",
                pi[i].sourceInfo.adapterId.HighPart, pi[i].sourceInfo.adapterId.LowPart,
                mi[pi[i].sourceInfo.modeInfoIdx].adapterId.HighPart, mi[pi[i].sourceInfo.modeInfoIdx].adapterId.LowPart);
        ok(mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.width > 0 && mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.height > 0,
                "Expected non-zero height/width, got %ux%u\n",
                mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.width, mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.height);


        if (pi[i].targetInfo.modeInfoIdx == DISPLAYCONFIG_PATH_MODE_IDX_INVALID)
        {
            skip("Path doesn't contain target modeInfoIdx");
            continue;
        }
        ok(pi[i].targetInfo.modeInfoIdx < modes, "Expected index <%d, got %d\n", modes, pi[i].targetInfo.modeInfoIdx);
        if (pi[i].targetInfo.modeInfoIdx >= modes)
            continue;

        ok(mi[pi[i].targetInfo.modeInfoIdx].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET, "Expected infoType %d, got %d\n",
                DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE, mi[pi[i].targetInfo.modeInfoIdx].infoType);
        ok(pi[i].targetInfo.id == mi[pi[i].targetInfo.modeInfoIdx].id, "Expected id %u, got %u\n",
                pi[i].targetInfo.id, mi[pi[i].targetInfo.modeInfoIdx].id);
        ok(pi[i].targetInfo.adapterId.HighPart == mi[pi[i].targetInfo.modeInfoIdx].adapterId.HighPart &&
           pi[i].targetInfo.adapterId.LowPart == mi[pi[i].targetInfo.modeInfoIdx].adapterId.LowPart,
                "Expected LUID %08x:%08x, got %08x:%08x\n",
                pi[i].targetInfo.adapterId.HighPart, pi[i].targetInfo.adapterId.LowPart,
                mi[pi[i].targetInfo.modeInfoIdx].adapterId.HighPart, mi[pi[i].targetInfo.modeInfoIdx].adapterId.LowPart);
        ok(mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.activeSize.cx > 0 &&
           mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.activeSize.cy > 0,
                "Expected non-zero height/width, got %ux%u\n",
                mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.activeSize.cx,
                mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.activeSize.cy);

        if (flags == QDC_DATABASE_CURRENT)
            ok(mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cx == 0 &&
               mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cy == 0,
                    "Expected zero height/width, got %ux%u\n",
                    mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cx,
                    mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cy);
        else
            ok(mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cx > 0 &&
               mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cy > 0,
                    "Expected non-zero height/width, got %ux%u\n",
                    mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cx,
                    mi[pi[i].targetInfo.modeInfoIdx].targetMode.targetVideoSignalInfo.totalSize.cy);
    }
}

static void test_QueryDisplayConfig(void)
{
    UINT32 paths, modes;
    DISPLAYCONFIG_PATH_INFO pi[10];
    DISPLAYCONFIG_MODE_INFO mi[20];
    DISPLAYCONFIG_TOPOLOGY_ID topologyid;
    LONG ret;

    ret = pQueryDisplayConfig(QDC_ALL_PATHS, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, NULL, &modes, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, NULL, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = 0;
    modes = 1;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    ok (paths == 0, "got %u\n", paths);
    ok (modes == 1, "got %u\n", modes);

    /* Crashes on Windows 10 */
    if (0)
    {
        ret = pQueryDisplayConfig(QDC_ALL_PATHS, NULL, pi, NULL, mi, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

        ret = pQueryDisplayConfig(QDC_ALL_PATHS, NULL, pi, &modes, mi, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);
    }

    paths = modes = 1;
    ret = pQueryDisplayConfig(0, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(0xFF, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(QDC_DATABASE_CURRENT, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, &topologyid);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    /* Below this point, test functionality that requires a WDDM driver on Windows */
    paths = modes = 1;
    memset(pi, 0xFF, sizeof(pi[0]));
    memset(mi, 0xFF, sizeof(mi[0]));
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, NULL);
    if (ret == ERROR_NOT_SUPPORTED)
    {
        todo_wine
        win_skip("QueryDisplayConfig() functionality is unsupported\n");
        return;
    }
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "got %d\n", ret);
    ok (paths == 1, "got %u\n", paths);
    ok (modes == 1, "got %u\n", modes);

    paths = ARRAY_SIZE(pi);
    modes = ARRAY_SIZE(mi);
    memset(pi, 0xFF, sizeof(pi));
    memset(mi, 0xFF, sizeof(mi));
    ret = pQueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths, pi, &modes, mi, NULL);
    ok(!ret, "got %d\n", ret);
    ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    if (!ret && paths > 0 && modes > 0)
        test_QueryDisplayConfig_result(QDC_ONLY_ACTIVE_PATHS, paths, pi, modes, mi);

    paths = ARRAY_SIZE(pi);
    modes = ARRAY_SIZE(mi);
    memset(pi, 0xFF, sizeof(pi));
    memset(mi, 0xFF, sizeof(mi));
    topologyid = 0xFF;
    ret = pQueryDisplayConfig(QDC_DATABASE_CURRENT, &paths, pi, &modes, mi, &topologyid);
    ok(!ret, "got %d\n", ret);
    ok(topologyid != 0xFF, "expected topologyid to be set, got %d\n", topologyid);
    if (!ret && paths > 0 && modes > 0)
        test_QueryDisplayConfig_result(QDC_DATABASE_CURRENT, paths, pi, modes, mi);
}

static void test_DisplayConfigGetDeviceInfo(void)
{
    LONG ret;
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name;
    DISPLAYCONFIG_TARGET_DEVICE_NAME target_name;
    DISPLAYCONFIG_TARGET_PREFERRED_MODE preferred_mode;
    DISPLAYCONFIG_ADAPTER_NAME adapter_name;

    ret = pDisplayConfigGetDeviceInfo(NULL);
    ok(ret == ERROR_GEN_FAILURE, "got %d\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE, "got %d\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE, "got %d\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = sizeof(source_name.header);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name.header);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = sizeof(source_name);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name);
    source_name.header.adapterId.LowPart = 0xFFFF;
    source_name.header.adapterId.HighPart = 0xFFFF;
    source_name.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target_name.header.size = sizeof(target_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&target_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target_name.header.size = sizeof(target_name);
    target_name.header.adapterId.LowPart = 0xFFFF;
    target_name.header.adapterId.HighPart = 0xFFFF;
    target_name.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&target_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
    preferred_mode.header.size = sizeof(preferred_mode) - 1;
    ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
    preferred_mode.header.size = sizeof(preferred_mode);
    preferred_mode.header.adapterId.LowPart = 0xFFFF;
    preferred_mode.header.adapterId.HighPart = 0xFFFF;
    preferred_mode.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);

    adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
    adapter_name.header.size = sizeof(adapter_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %d\n", ret);

    adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
    adapter_name.header.size = sizeof(adapter_name);
    adapter_name.header.adapterId.LowPart = 0xFFFF;
    adapter_name.header.adapterId.HighPart = 0xFFFF;
    ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %d\n", ret);
}

static void test_display_config(void)
{
    if (!pGetDisplayConfigBufferSizes ||
        !pQueryDisplayConfig ||
        !pDisplayConfigGetDeviceInfo)
    {
        win_skip("DisplayConfig APIs are not supported\n");
        return;
    }

    test_GetDisplayConfigBufferSizes();
    test_QueryDisplayConfig();
    test_DisplayConfigGetDeviceInfo();
}

START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
    test_ChangeDisplaySettingsEx();
    test_EnumDisplayMonitors();
    test_monitors();
    test_work_area();
    test_display_config();
}
