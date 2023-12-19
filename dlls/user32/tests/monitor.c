/*
 * Unit tests for monitor APIs
 *
 * Copyright 2005 Huw Davies
 * Copyright 2008 Dmitry Timoshkov
 * Copyright 2019-2022 Zhiyi Zhang for CodeWeavers
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

#include "ntstatus.h"
#define WIN32_NO_STATUS

#include "initguid.h"

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"
#include "setupapi.h"
#include "ntddvdeo.h"
#include <stdio.h>

DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_GPU_LUID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 1);
DEFINE_DEVPROPKEY(DEVPROPKEY_MONITOR_OUTPUT_ID, 0xca085853, 0x16ce, 0x48aa, 0xb1, 0x14, 0xde, 0x9c, 0x72, 0x33, 0x42, 0x23, 2);

static LONG (WINAPI *pGetDisplayConfigBufferSizes)(UINT32,UINT32*,UINT32*);
static BOOL (WINAPI *pGetDpiForMonitorInternal)(HMONITOR,UINT,UINT*,UINT*);
static LONG (WINAPI *pQueryDisplayConfig)(UINT32,UINT32*,DISPLAYCONFIG_PATH_INFO*,UINT32*,
                                          DISPLAYCONFIG_MODE_INFO*,DISPLAYCONFIG_TOPOLOGY_ID*);
static LONG (WINAPI *pDisplayConfigGetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER*);
static LONG (WINAPI *pDisplayConfigSetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER*);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER*);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME*);

static void init_function_pointers(void)
{
    HMODULE user32 = GetModuleHandleA("user32.dll");
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");

#define GET_PROC(module, func)                       \
    p##func = (void *)GetProcAddress(module, #func); \
    if (!p##func)                                    \
        trace("GetProcAddress(%s, %s) failed.\n", #module, #func);

    GET_PROC(user32, GetDisplayConfigBufferSizes)
    GET_PROC(user32, GetDpiForMonitorInternal)
    GET_PROC(user32, QueryDisplayConfig)
    GET_PROC(user32, DisplayConfigGetDeviceInfo)
    GET_PROC(user32, DisplayConfigSetDeviceInfo)
    GET_PROC(user32, SetThreadDpiAwarenessContext)

    GET_PROC(gdi32, D3DKMTCloseAdapter)
    GET_PROC(gdi32, D3DKMTOpenAdapterFromGdiDisplayName)

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

static unsigned int get_primary_dpi(void)
{
    DPI_AWARENESS_CONTEXT old_context;
    UINT dpi_x = 0, dpi_y = 0;
    POINT point = {0, 0};
    HMONITOR monitor;

    if (!pSetThreadDpiAwarenessContext || !pGetDpiForMonitorInternal)
        return 0;

    old_context = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
    pGetDpiForMonitorInternal(monitor, 0, &dpi_x, &dpi_y);
    pSetThreadDpiAwarenessContext(old_context);
    return dpi_y;
}

static int get_bitmap_stride(int width, int bpp)
{
    return ((width * bpp + 15) >> 3) & ~1;
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
    if (device->StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
    {
        /* Test creating DC */
        hdc = CreateDCA(device->DeviceName, NULL, NULL, NULL);
        ok(hdc != NULL, "#%d: failed to CreateDC(\"%s\") err=%ld\n", index, device->DeviceName, GetLastError());
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

static void test_enumdisplaydevices_monitor(int monitor_index, const char *adapter_name,
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
    ok(device->StateFlags <= (DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE),
       "#%d wrong state %#lx\n", monitor_index, device->StateFlags);

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
                test_enumdisplaydevices_monitor(monitor_index, adapter_name, &dd, flags[flag_index]);
        }

    ok(adapter_count > 0, "Expect at least one adapter found\n");
    /* XP on Testbot doesn't report a monitor, whereas XP on real machine does */
    ok(broken(monitor_count == 0) || monitor_count > 0, "Expect at least one monitor found\n");
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
    ok_(__FILE__, line)(ret, "Device %s test %ld EnumDisplaySettingsA failed, error %#lx\n", device, test, GetLastError());

    ok_(__FILE__, line)((dm.dmFields & expected->dmFields) == expected->dmFields,
            "Device %s test %ld expect dmFields to contain %#lx, got %#lx\n", device, test, expected->dmFields, dm.dmFields);
    ok_(__FILE__, line)(!(expected->dmFields & DM_BITSPERPEL) || dm.dmBitsPerPel == expected->dmBitsPerPel,
            "Device %s test %ld expect dmBitsPerPel %lu, got %lu\n", device, test, expected->dmBitsPerPel, dm.dmBitsPerPel);
    ok_(__FILE__, line)(!(expected->dmFields & DM_PELSWIDTH) || dm.dmPelsWidth == expected->dmPelsWidth,
            "Device %s test %ld expect dmPelsWidth %lu, got %lu\n", device, test, expected->dmPelsWidth, dm.dmPelsWidth);
    ok_(__FILE__, line)(!(expected->dmFields & DM_PELSHEIGHT) || dm.dmPelsHeight == expected->dmPelsHeight,
            "Device %s test %ld expect dmPelsHeight %lu, got %lu\n", device, test, expected->dmPelsHeight, dm.dmPelsHeight);
    ok_(__FILE__, line)(!(expected->dmFields & DM_POSITION) || dm.dmPosition.x == expected->dmPosition.x,
            "Device %s test %ld expect dmPosition.x %ld, got %ld\n", device, test, expected->dmPosition.x, dm.dmPosition.x);
    ok_(__FILE__, line)(!(expected->dmFields & DM_POSITION) || dm.dmPosition.y == expected->dmPosition.y,
            "Device %s test %ld expect dmPosition.y %ld, got %ld\n", device, test, expected->dmPosition.y, dm.dmPosition.y);
    ok_(__FILE__, line)(!(expected->dmFields & DM_DISPLAYFREQUENCY) ||
            dm.dmDisplayFrequency == expected->dmDisplayFrequency,
            "Device %s test %ld expect dmDisplayFrequency %lu, got %lu\n", device, test, expected->dmDisplayFrequency,
            dm.dmDisplayFrequency);
    ok_(__FILE__, line)(!(expected->dmFields & DM_DISPLAYORIENTATION) ||
            dm.dmDisplayOrientation == expected->dmDisplayOrientation,
            "Device %s test %ld expect dmDisplayOrientation %ld, got %ld\n", device, test, expected->dmDisplayOrientation,
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
    BOOL found;
    LONG res;
    int i;

    /* Test invalid device names */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());

    res = ChangeDisplaySettingsExA("invalid", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %ld\n", res);

    res = ChangeDisplaySettingsExA("\\\\.\\DISPLAY0", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %ld\n", res);

    res = ChangeDisplaySettingsExA("\\\\.\\DISPLAY1\\Monitor0", &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADPARAM, "ChangeDisplaySettingsA returned unexpected %ld\n", res);

    /* Test dmDriverExtra */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    res = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(res, "EnumDisplaySettingsW failed, error %#lx\n", GetLastError());

    /* ChangeDisplaySettingsA/W reset dmDriverExtra to 0 */
    dm.dmDriverExtra = 1;
    res = ChangeDisplaySettingsA(&dm, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsA returned unexpected %ld\n", res);
    ok(dm.dmDriverExtra == 0, "ChangeDisplaySettingsA didn't reset dmDriverExtra to 0\n");

    dmW.dmDriverExtra = 1;
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW returned unexpected %ld\n", res);
    ok(dmW.dmDriverExtra == 0, "ChangeDisplaySettingsW didn't reset dmDriverExtra to 0\n");

    /* ChangeDisplaySettingsExA/W do not modify dmDriverExtra */
    dm.dmDriverExtra = 1;
    res = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld\n", res);
    ok(dm.dmDriverExtra == 1, "ChangeDisplaySettingsExA shouldn't change dmDriverExtra\n");

    dmW.dmDriverExtra = 1;
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW returned unexpected %ld\n", res);
    ok(dmW.dmDriverExtra == 1, "ChangeDisplaySettingsExW shouldn't change dmDriverExtra\n");

    /* Test dmSize */
    /* ChangeDisplaySettingsA/ExA report success even if dmSize is 0 */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    res = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(res, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());

    dm.dmSize = 0;
    res = ChangeDisplaySettingsA(&dm, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsA returned unexpected %ld\n", res);

    dm.dmSize = 0;
    res = ChangeDisplaySettingsExA(NULL, &dm, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld\n", res);

    /* dmSize for ChangeDisplaySettingsW/ExW needs to be at least FIELD_OFFSET(DEVMODEW, dmICMMethod) */
    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    res = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(res, "EnumDisplaySettingsW failed, error %#lx\n", GetLastError());

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod) - 1;
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_BADMODE, "ChangeDisplaySettingsW returned %ld, expect DISP_CHANGE_BADMODE\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod) - 1;
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_BADMODE, "ChangeDisplaySettingsExW returned %ld, expect DISP_CHANGE_BADMODE\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    res = ChangeDisplaySettingsW(&dmW, CDS_TEST);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW returned unexpected %ld\n", res);

    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    res = ChangeDisplaySettingsExW(NULL, &dmW, NULL, CDS_TEST, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW returned unexpected %ld\n", res);

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
           "Unexpected ChangeDisplaySettingsExA() return code for vid_modes_test[%d]: %ld\n", i, res);

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

    /* Save the original mode for all devices so that they can be restored at the end of tests */
    device_count = 0;
    device_size = 2;
    devices = calloc(device_size, sizeof(*devices));
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
            devices = realloc(devices, device_size * sizeof(*devices));
            ok(devices != NULL, "Failed to reallocate memory.\n");
        }

        devices[device_count].index = device;
        lstrcpyA(devices[device_count].name, dd.DeviceName);
        memset(&devices[device_count].original_mode, 0, sizeof(devices[device_count].original_mode));
        devices[device_count].original_mode.dmSize = sizeof(devices[device_count].original_mode);
        res = EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &devices[device_count].original_mode);
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", dd.DeviceName, GetLastError());
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
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[0].name, res);
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[0].name, res);

        /* Check that the adapter is still attached */
        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[0].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#lx\n", devices[0].name, GetLastError());
        ok(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP, "Expect device %s still attached.\n", devices[0].name);

        count = GetSystemMetrics(SM_CMONITORS);
        ok(count == old_count, "Expect monitor count %d, got %d\n", old_count, count);

        /* Restore registry settings */
        res = ChangeDisplaySettingsExA(devices[0].name, &devices[0].original_mode, NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_BADPARAM) || /* win10 */
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[0].name, res);
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
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[1].name, res);
        res = ChangeDisplaySettingsExA(devices[1].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[1].name, res);

        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[1].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#lx\n", devices[1].name, GetLastError());
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
                skip("Depth %lu is unsupported for %s.\n", depths[test], devices[device].name);
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
               "ChangeDisplaySettingsExA %s returned unexpected %ld.\n", devices[device].name, res);
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
               "ChangeDisplaySettingsExA %s returned unexpected %ld.\n", devices[device].name, res);

            /* Change to a mode with depth set but with zero width and height */
            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            dm.dmBitsPerPel = depths[test];
            res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
               devices[device].name, res);
            flush_events();

            dd.cb = sizeof(dd);
            res = EnumDisplayDevicesA(NULL, devices[device].index, &dd, 0);
            ok(res, "EnumDisplayDevicesA %s failed, error %#lx.\n", devices[device].name, GetLastError());
            ok(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP, "Expect %s attached.\n",
               devices[device].name);

            memset(&dm, 0, sizeof(dm));
            dm.dmSize = sizeof(dm);
            res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
            ok(res, "Device %s EnumDisplaySettingsA failed, error %#lx.\n", devices[device].name, GetLastError());
            ok(dm.dmBitsPerPel == depths[test], "Device %s expect dmBitsPerPel %lu, got %lu.\n",
               devices[device].name, depths[test], dm.dmBitsPerPel);
            /* 2008 resets to the resolution in the registry. Newer versions of Windows doesn't
             * change the current resolution */
            ok(dm.dmPelsWidth == dm3.dmPelsWidth || broken(dm.dmPelsWidth == dm2.dmPelsWidth),
               "Device %s expect dmPelsWidth %lu, got %lu.\n",
               devices[device].name, dm3.dmPelsWidth, dm.dmPelsWidth);
            ok(dm.dmPelsHeight == dm3.dmPelsHeight || broken(dm.dmPelsHeight == dm2.dmPelsHeight),
               "Device %s expect dmPelsHeight %lu, got %lu.\n",
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
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);
        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);

        dd.cb = sizeof(dd);
        res = EnumDisplayDevicesA(NULL, devices[device].index, &dd, 0);
        ok(res, "EnumDisplayDevicesA %s failed, error %#lx\n", devices[device].name, GetLastError());
        ok(!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP), "Expect device %s detached.\n", devices[device].name);

        count = GetSystemMetrics(SM_CMONITORS);
        ok(count == old_count - 1, "Expect monitor count %d, got %d\n", old_count - 1, count);
    }

    /* Test changing each adapter to different width, height, frequency and depth */
    position.x = 0;
    position.y = 0;
    for (device = 0; device < device_count; ++device)
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        for (mode = 0; EnumDisplaySettingsExA(devices[device].name, mode, &dm, 0); ++mode)
        {
            if (mode == 0)
            {
                dm2 = dm;
            }
            else
            {
                found = FALSE;
                if (dm2.dmPelsWidth && dm.dmPelsWidth != dm2.dmPelsWidth)
                {
                    dm2.dmPelsWidth = 0;
                    found = TRUE;
                }
                if (dm2.dmPelsHeight && dm.dmPelsHeight != dm2.dmPelsHeight)
                {
                    dm2.dmPelsHeight = 0;
                    found = TRUE;
                }
                if (dm2.dmDisplayFrequency && dm.dmDisplayFrequency != dm2.dmDisplayFrequency)
                {
                    dm2.dmDisplayFrequency = 0;
                    found = TRUE;
                }
                if (dm2.dmBitsPerPel && dm.dmBitsPerPel != dm2.dmBitsPerPel)
                {
                    dm2.dmBitsPerPel = 0;
                    found = TRUE;
                }

                if (!dm2.dmPelsWidth && !dm2.dmPelsHeight && !dm2.dmDisplayFrequency
                        && !dm2.dmBitsPerPel)
                    break;

                if (!found)
                    continue;
            }

            dm.dmPosition = position;
            dm.dmFields |= DM_POSITION;
            /* Reattach detached non-primary adapters, otherwise ChangeDisplaySettingsExA with only CDS_RESET fails */
            if (mode == 0 && device)
            {
                res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
                ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s mode %d returned unexpected %ld\n",
                        devices[device].name, mode, res);
                res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
            }
            else
            {
                res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_RESET, NULL);
            }

            ok(res == DISP_CHANGE_SUCCESSFUL ||
                    broken(res == DISP_CHANGE_FAILED), /* TestBots using VGA driver can't change to some modes */
                    "ChangeDisplaySettingsExA %s mode %d returned unexpected %ld\n", devices[device].name, mode, res);
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
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);

        /* Place the next adapter to the right so that there is no position conflict */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[device].name, GetLastError());
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
            ok(res, "EnumDisplaySettingsA %s failed, error %#lx.\n", devices[device - 1].name, GetLastError());
            position.x = dm.dmPosition.x + dm.dmPelsWidth;
        }

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[device].name, GetLastError());
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
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);
        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL ||
                broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);
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
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned %ld.\n", devices[device].name, res);
        res = EnumDisplaySettingsA(devices[device].name, ENUM_REGISTRY_SETTINGS, &dm);
        /* Win10 either returns failure here or retrieves outdated display settings until they're applied */
        if (res)
        {
            ok((dm.dmFields & registry_fields) == registry_fields, "Got unexpected dmFields %#lx.\n", dm.dmFields);
            ok(dm.dmPosition.x == position.x, "Expected dmPosition.x %ld, got %ld.\n", position.x, dm.dmPosition.x);
            ok(dm.dmPosition.y == position.y, "Expected dmPosition.y %ld, got %ld.\n", position.y, dm.dmPosition.y);
            ok(dm.dmPelsWidth == dm3.dmPelsWidth || broken(dm.dmPelsWidth == dm2.dmPelsWidth), /* Win10 */
                    "Expected dmPelsWidth %lu, got %lu.\n", dm3.dmPelsWidth, dm.dmPelsWidth);
            ok(dm.dmPelsHeight == dm3.dmPelsHeight || broken(dm.dmPelsHeight == dm2.dmPelsHeight), /* Win10 */
                    "Expected dmPelsHeight %lu, got %lu.\n", dm3.dmPelsHeight, dm.dmPelsHeight);
            ok(dm.dmBitsPerPel, "Expected dmBitsPerPel not zero.\n");
            ok(dm.dmDisplayFrequency, "Expected dmDisplayFrequency not zero.\n");
        }
        else
        {
            win_skip("EnumDisplaySettingsA %s failed, error %#lx.\n", devices[device].name, GetLastError());
        }

        res = ChangeDisplaySettingsExA(devices[device].name, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned %ld.\n", devices[device].name, res);
        flush_events();

        res = EnumDisplaySettingsA(devices[device].name, ENUM_REGISTRY_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx.\n", devices[device].name, GetLastError());
        ok((dm.dmFields & registry_fields) == registry_fields, "Got unexpected dmFields %#lx.\n", dm.dmFields);
        ok(dm.dmPosition.x == position.x, "Expected dmPosition.x %ld, got %ld.\n", position.x, dm.dmPosition.x);
        ok(dm.dmPosition.y == position.y, "Expected dmPosition.y %ld, got %ld.\n", position.y, dm.dmPosition.y);
        ok(dm.dmPelsWidth == dm3.dmPelsWidth, "Expected dmPelsWidth %lu, got %lu.\n", dm3.dmPelsWidth, dm.dmPelsWidth);
        ok(dm.dmPelsHeight == dm3.dmPelsHeight, "Expected dmPelsHeight %lu, got %lu.\n", dm3.dmPelsHeight,
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
            ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[device].name, GetLastError());

            dm.dmPelsWidth = 0;
            dm.dmPelsHeight = 0;
            dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
            res = ChangeDisplaySettingsExA(devices[device].name, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);
        }
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld\n", res);
    }

    if (device_count >= 2)
    {
        /* Query the primary adapter settings */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        res = EnumDisplaySettingsA(devices[0].name, ENUM_CURRENT_SETTINGS, &dm);
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[0].name, GetLastError());

        if (res)
        {
            /* Query the secondary adapter settings */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[1].name, GetLastError());
        }

        if (res)
        {
            /* The secondary adapter should be to the right of the primary adapter */
            ok(dm2.dmPosition.x == dm.dmPosition.x + dm.dmPelsWidth,
               "Expected dm2.dmPosition.x %ld, got %ld.\n", dm.dmPosition.x + dm.dmPelsWidth,
               dm2.dmPosition.x);
            ok(dm2.dmPosition.y == dm.dmPosition.y, "Expected dm2.dmPosition.y %ld, got %ld.\n",
               dm.dmPosition.y, dm2.dmPosition.y);

            /* Test position conflict */
            dm2.dmPosition.x = dm.dmPosition.x - dm2.dmPelsWidth + 1;
            dm2.dmPosition.y = dm.dmPosition.y;
            res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[1].name, res);

            /* Position is changed and automatically moved */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[1].name, GetLastError());
            ok(dm2.dmPosition.x == dm.dmPosition.x - dm2.dmPelsWidth,
               "Expected dmPosition.x %ld, got %ld.\n", dm.dmPosition.x - dm2.dmPelsWidth,
               dm2.dmPosition.x);

            /* Test position with extra space. The extra space will be removed */
            dm2.dmPosition.x = dm.dmPosition.x + dm.dmPelsWidth + 1;
            dm2.dmPosition.y = dm.dmPosition.y;
            res = ChangeDisplaySettingsExA(devices[1].name, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[1].name, res);

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
                ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s test %d returned unexpected %ld\n",
                        devices[1].name, test, res);
                if (res != DISP_CHANGE_SUCCESSFUL)
                {
                    win_skip("ChangeDisplaySettingsExA %s test %d returned unexpected %ld.\n", devices[1].name, test, res);
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
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[0].name, res);

            /* Now the position of the second adapter should be changed */
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            res = EnumDisplaySettingsA(devices[1].name, ENUM_CURRENT_SETTINGS, &dm2);
            ok(res, "EnumDisplaySettingsA %s failed, error %#lx\n", devices[1].name, GetLastError());
            ok(dm2.dmPosition.x == dm.dmPelsWidth, "Expect dmPosition.x %ld, got %ld\n",
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
        ok(res, "EnumDisplaySettingsA %s failed, error %#lx.\n", devices[device].name, GetLastError());

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
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s mode %d returned unexpected %ld.\n",
                    devices[device].name, mode, res);
            flush_events();
            expect_dm(&dm2, devices[device].name, mode);

            /* EnumDisplaySettingsEx without EDS_ROTATEDMODE reports modes with current orientation */
            memset(&dm3, 0, sizeof(dm3));
            dm3.dmSize = sizeof(dm3);
            for (i = 0; EnumDisplaySettingsExA(devices[device].name, i, &dm3, 0); ++i)
            {
                ok(dm3.dmDisplayOrientation == dm2.dmDisplayOrientation,
                        "Expected %s display mode %d orientation %ld, got %ld.\n",
                        devices[device].name, i, dm2.dmDisplayOrientation, dm3.dmDisplayOrientation);
            }
            ok(i > 0, "Expected at least one display mode found.\n");

            if (device == 0)
            {
                ok(GetSystemMetrics(SM_CXSCREEN) == dm2.dmPelsWidth, "Expected %ld, got %d.\n",
                        dm2.dmPelsWidth, GetSystemMetrics(SM_CXSCREEN));
                ok(GetSystemMetrics(SM_CYSCREEN) == dm2.dmPelsHeight, "Expected %ld, got %d.\n",
                        dm2.dmPelsHeight, GetSystemMetrics(SM_CYSCREEN));
            }

            if (device_count == 1)
            {
                ok(GetSystemMetrics(SM_CXVIRTUALSCREEN) == dm2.dmPelsWidth, "Expected %ld, got %d.\n",
                        dm2.dmPelsWidth, GetSystemMetrics(SM_CXVIRTUALSCREEN));
                ok(GetSystemMetrics(SM_CYVIRTUALSCREEN) == dm2.dmPelsHeight, "Expected %ld, got %d.\n",
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
                "ChangeDisplaySettingsExA %s returned unexpected %ld\n", devices[device].name, res);
    }
    res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL ||
            broken(res == DISP_CHANGE_FAILED), /* win8 TestBot */
            "ChangeDisplaySettingsExA returned unexpected %ld\n", res);
    for (device = 0; device < device_count; ++device)
        expect_dm(&devices[device].original_mode, devices[device].name, 0);

    free(devices);
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
    ok(ret, "GetMonitorInfo error %lu\n", GetLastError());
    ok(mi.dwFlags & MONITORINFOF_PRIMARY, "not a primary monitor\n");
    trace("primary monitor %s\n", wine_dbgstr_rect(&mi.rcMonitor));

    SetLastError(0xdeadbeef);
    ret = SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc_work, 0);
    ok(ret, "SystemParametersInfo error %lu\n", GetLastError());
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
    trace("min: %ld,%ld max %ld,%ld normal %s\n", wp.ptMinPosition.x, wp.ptMinPosition.y,
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
    trace("min: %ld,%ld max %ld,%ld normal %s\n", wp.ptMinPosition.x, wp.ptMinPosition.y,
          wp.ptMaxPosition.x, wp.ptMaxPosition.y, wine_dbgstr_rect(&wp.rcNormalPosition));
    ok(EqualRect(&rc_normal, &wp.rcNormalPosition), "normal pos is different\n");

    DestroyWindow(hwnd);
}

static void test_GetDisplayConfigBufferSizes(void)
{
    UINT32 paths, modes;
    LONG ret;

    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = 100;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, &paths, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    ok(paths == 100, "got %u\n", paths);

    modes = 100;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, NULL, &modes);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    ok(modes == 100, "got %u\n", modes);

    ret = pGetDisplayConfigBufferSizes(0, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = 100;
    ret = pGetDisplayConfigBufferSizes(0, &paths, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    ok(paths == 100, "got %u\n", paths);

    modes = 100;
    ret = pGetDisplayConfigBufferSizes(0, NULL, &modes);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    ok(modes == 100, "got %u\n", modes);

    /* Flag validation on Windows is driver-dependent */
    paths = modes = 100;
    ret = pGetDisplayConfigBufferSizes(0, &paths, &modes);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);
    ok((modes == 0 || modes == 100) && paths == 0, "got %u, %u\n", modes, paths);

    paths = modes = 100;
    ret = pGetDisplayConfigBufferSizes(0xFF, &paths, &modes);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);
    ok((modes == 0 || modes == 100) && paths == 0, "got %u, %u\n", modes, paths);

    /* Test success */
    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_ALL_PATHS, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);

    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);

    paths = modes = 0;
    ret = pGetDisplayConfigBufferSizes(QDC_DATABASE_CURRENT, &paths, &modes);
    if (!ret)
        ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    else
        ok(ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);
}

static BOOL CALLBACK test_EnumDisplayMonitors_normal_cb(HMONITOR monitor, HDC hdc, LPRECT rect,
        LPARAM lparam)
{
    MONITORINFO mi;
    LONG ret;

    mi.cbSize = sizeof(mi);
    ret = GetMonitorInfoA(monitor, &mi);
    ok(ret, "GetMonitorInfoA failed, error %#lx.\n", GetLastError());
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
    ok(ret, "GetMonitorInfoA failed, error %#lx.\n", GetLastError());

    /* Test that monitor handle is invalid after the monitor is detached */
    if (!(mi.dwFlags & MONITORINFOF_PRIMARY))
    {
        count = GetSystemMetrics(SM_CMONITORS);

        /* Save current display settings */
        memset(&old_dm, 0, sizeof(old_dm));
        old_dm.dmSize = sizeof(old_dm);
        ret = EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &old_dm);
        ok(ret, "EnumDisplaySettingsA %s failed, error %#lx.\n", mi.szDevice, GetLastError());

        /* Detach monitor */
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        dm.dmPosition.x = mi.rcMonitor.left;
        dm.dmPosition.y = mi.rcMonitor.top;
        ret = ChangeDisplaySettingsExA(mi.szDevice, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET,
                NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                mi.szDevice, ret);
        ret = ChangeDisplaySettingsExA(mi.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
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
               "Expected error %#x, got %#lx.\n", ERROR_INVALID_MONITOR_HANDLE, error);

        /* Restore the original display settings */
        ret = ChangeDisplaySettingsExA(mi.szDevice, &old_dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET,
                NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                mi.szDevice, ret);
        ret = ChangeDisplaySettingsExA(mi.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                mi.szDevice, ret);
    }

    SetLastError(0xdeadbeef);
    return TRUE;
}

static BOOL CALLBACK test_EnumDisplayMonitors_count(HMONITOR monitor, HDC hdc, LPRECT rect,
        LPARAM lparam)
{
    INT *count = (INT *)lparam;
    ++(*count);
    return TRUE;
}

static void test_EnumDisplayMonitors(void)
{
    static const DWORD DESKTOP_ALL_ACCESS = 0x01ff;
    HWINSTA winstation, old_winstation;
    HDESK desktop, old_desktop;
    USEROBJECTFLAGS flags;
    INT count, old_count;
    DWORD error;
    BOOL ret;

    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_normal_cb, 0);
    ok(ret, "EnumDisplayMonitors failed, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_return_false_cb, 0);
    error = GetLastError();
    ok(!ret, "EnumDisplayMonitors succeeded.\n");
    ok(error == 0xdeadbeef, "Expected error %#x, got %#lx.\n", 0xdeadbeef, error);

    count = GetSystemMetrics(SM_CMONITORS);
    SetLastError(0xdeadbeef);
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_invalid_handle_cb, 0);
    error = GetLastError();
    if (count >= 2)
        todo_wine ok(!ret, "EnumDisplayMonitors succeeded.\n");
    else
        ok(ret, "EnumDisplayMonitors failed.\n");
    ok(error == 0xdeadbeef, "Expected error %#x, got %#lx.\n", 0xdeadbeef, error);

    /* Test that monitor enumeration is not affected by window stations and desktops */
    old_winstation = GetProcessWindowStation();
    old_desktop = GetThreadDesktop(GetCurrentThreadId());
    old_count = GetSystemMetrics(SM_CMONITORS);

    count = 0;
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_count, (LPARAM)&count);
    ok(ret, "EnumDisplayMonitors failed, error %#lx.\n", GetLastError());
    ok(count == old_count, "Expected %d, got %d.\n", old_count, count);

    winstation = CreateWindowStationW(NULL, 0, WINSTA_ALL_ACCESS, NULL);
    ok(!!winstation && winstation != old_winstation, "CreateWindowStationW failed, error %#lx.\n", GetLastError());
    ret = SetProcessWindowStation(winstation);
    ok(ret, "SetProcessWindowStation failed, error %#lx.\n", GetLastError());
    ok(winstation == GetProcessWindowStation(), "Expected %p, got %p.\n", GetProcessWindowStation(), winstation);

    flags.fInherit = FALSE;
    flags.fReserved = FALSE;
    flags.dwFlags = WSF_VISIBLE;
    ret = SetUserObjectInformationW(winstation, UOI_FLAGS, &flags, sizeof(flags));
    ok(ret, "SetUserObjectInformationW failed, error %#lx.\n", GetLastError());

    desktop = CreateDesktopW(L"test_desktop", NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL);
    ok(!!desktop && desktop != old_desktop, "CreateDesktopW failed, error %#lx.\n", GetLastError());
    ret = SetThreadDesktop(desktop);
    ok(ret, "SetThreadDesktop failed, error %#lx.\n", GetLastError());
    ok(desktop == GetThreadDesktop(GetCurrentThreadId()), "Expected %p, got %p.\n",
            GetThreadDesktop(GetCurrentThreadId()), desktop);

    count = GetSystemMetrics(SM_CMONITORS);
    ok(count == old_count, "Expected %d, got %d.\n", old_count, count);
    count = 0;
    ret = EnumDisplayMonitors(NULL, NULL, test_EnumDisplayMonitors_count, (LPARAM)&count);
    ok(ret, "EnumDisplayMonitors failed, error %#lx.\n", GetLastError());
    ok(count == old_count, "Expected %d, got %d.\n", old_count, count);

    ret = SetProcessWindowStation(old_winstation);
    ok(ret, "SetProcessWindowStation failed, error %#lx.\n", GetLastError());
    ret = SetThreadDesktop(old_desktop);
    ok(ret, "SetThreadDesktop failed, error %#lx.\n", GetLastError());
    ret = CloseDesktop(desktop);
    ok(ret, "CloseDesktop failed, error %#lx.\n", GetLastError());
    ret = CloseWindowStation(winstation);
    ok(ret, "CloseWindowStation failed, error %#lx.\n", GetLastError());
}

static void check_device_path(const WCHAR *device_path, const LUID *adapter_id, DWORD id)
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    BOOL ret, found = FALSE;
    DEVPROPTYPE type;
    DWORD output_id;
    unsigned int i;
    HDEVINFO set;
    LUID luid;

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);

    set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_MONITOR, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "Got error %lu.\n", GetLastError());

    i = 0;
    while (SetupDiEnumDeviceInterfaces(set, NULL, &GUID_DEVINTERFACE_MONITOR, i, &iface))
    {
        ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data,
                sizeof(iface_detail_buffer), NULL, &device_data);
        ok(ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError());

        ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPROPKEY_MONITOR_GPU_LUID, &type,
                (BYTE *)&luid, sizeof(luid), NULL, 0);
        ok(ret || broken(GetLastError() == ERROR_NOT_FOUND) /* before Win10 1809 */,
                "Got error %lu.\n", GetLastError());
        if (!ret)
        {
            win_skip("DEVPROPKEY_MONITOR_GPU_LUID is not found, skipping device path check.\n");
            SetupDiDestroyDeviceInfoList(set);
            return;
        }
        ret = SetupDiGetDevicePropertyW(set, &device_data, &DEVPROPKEY_MONITOR_OUTPUT_ID,
                &type, (BYTE *)&output_id, sizeof(output_id), NULL, 0);
        ok(ret, "Got error %lu.\n", GetLastError());

        if (output_id == id && RtlEqualLuid(&luid, adapter_id) && !wcsicmp(device_path, iface_data->DevicePath))
        {
            found = TRUE;
            break;
        }
        ++i;
    }
    ok(found, "device_path %s not found, luid %04lx:%04lx.\n", debugstr_w(device_path), adapter_id->HighPart,
            adapter_id->LowPart);
    SetupDiDestroyDeviceInfoList(set);
}

static void check_preferred_mode(const DISPLAYCONFIG_TARGET_PREFERRED_MODE *mode, const WCHAR *gdi_device_name)
{
    DISPLAYCONFIG_TARGET_PREFERRED_MODE mode2;
    DEVMODEW dm, dm2;
    LONG lret;
    BOOL bret;

    dm.dmSize = sizeof(dm);
    bret = EnumDisplaySettingsW(gdi_device_name, ENUM_CURRENT_SETTINGS, &dm);
    ok(bret, "got error %lu.\n", GetLastError());

    if (dm.dmPelsWidth == 1024 && dm.dmPelsHeight == 768)
    {
        skip("Current display mode is already 1024x768, skipping test.\n");
        return;
    }
    if (mode->width == 1024 && mode->height == 768)
    {
        skip("Preferred display mode is 1024x768, skipping test.\n");
        return;
    }

    memset(&dm2, 0, sizeof(dm2));
    dm2.dmSize = sizeof(dm2);
    dm2.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    dm2.dmPelsWidth = 1024;
    dm2.dmPelsHeight = 768;
    lret = ChangeDisplaySettingsExW(gdi_device_name, &dm2, NULL, 0, 0);
    if (lret != DISP_CHANGE_SUCCESSFUL)
    {
        skip("Can't change display settings, skipping test.\n");
        return;
    }

    memset(&mode2, 0, sizeof(mode2));
    mode2.header = mode->header;

    lret = pDisplayConfigGetDeviceInfo(&mode2.header);
    ok(!lret, "got %ld\n", lret);
    ok(mode2.width == mode->width, "got %u, expected %u.\n", mode2.width, mode->width);
    ok(mode2.height == mode->height, "got %u, expected %u.\n", mode2.height, mode->height);

    lret = ChangeDisplaySettingsExW(gdi_device_name, &dm, NULL, 0, 0);
    ok(lret == DISP_CHANGE_SUCCESSFUL, "got %ld.\n", lret);
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
        ok(!ret, "Expected 0, got %ld\n", ret);
        ok(source_name.viewGdiDeviceName[0] != '\0', "Expected GDI device name, got empty string\n");

        /* Test with an invalid adapter LUID */
        source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        source_name.header.size = sizeof(source_name);
        source_name.header.adapterId.LowPart = 0xFFFF;
        source_name.header.adapterId.HighPart = 0xFFFF;
        source_name.header.id = pi[i].sourceInfo.id;
        ret = pDisplayConfigGetDeviceInfo(&source_name.header);
        ok(ret == ERROR_GEN_FAILURE, "Expected GEN_FAILURE, got %ld\n", ret);

        target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        target_name.header.size = sizeof(target_name);
        target_name.header.adapterId = pi[i].targetInfo.adapterId;
        target_name.header.id = pi[i].targetInfo.id;
        target_name.monitorDevicePath[0] = '\0';
        ret = pDisplayConfigGetDeviceInfo(&target_name.header);
        ok(!ret, "Expected 0, got %ld\n", ret);
        check_device_path(target_name.monitorDevicePath, &target_name.header.adapterId, target_name.header.id);

        preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
        preferred_mode.header.size = sizeof(preferred_mode);
        preferred_mode.header.adapterId = pi[i].targetInfo.adapterId;
        preferred_mode.header.id = pi[i].targetInfo.id;
        preferred_mode.width = preferred_mode.height = 0;
        ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
        ok(!ret, "Expected 0, got %ld\n", ret);
        ok(preferred_mode.width > 0 && preferred_mode.height > 0, "Expected non-zero height/width, got %ux%u\n",
                preferred_mode.width, preferred_mode.height);
        check_preferred_mode(&preferred_mode, source_name.viewGdiDeviceName);

        todo_wine {
        adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapter_name.header.size = sizeof(adapter_name);
        adapter_name.header.adapterId = pi[i].sourceInfo.adapterId;
        adapter_name.adapterDevicePath[0] = '\0';
        ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
        ok(!ret, "Expected 0, got %ld\n", ret);
        ok(adapter_name.adapterDevicePath[0] != '\0', "Expected adapter device path, got empty string\n");
        }

        /* Check corresponding modes */
        if (pi[i].sourceInfo.modeInfoIdx == DISPLAYCONFIG_PATH_MODE_IDX_INVALID)
        {
            skip("Path doesn't contain source modeInfoIdx\n");
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
                "Expected LUID %08lx:%08lx, got %08lx:%08lx\n",
                pi[i].sourceInfo.adapterId.HighPart, pi[i].sourceInfo.adapterId.LowPart,
                mi[pi[i].sourceInfo.modeInfoIdx].adapterId.HighPart, mi[pi[i].sourceInfo.modeInfoIdx].adapterId.LowPart);
        ok(mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.width > 0 && mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.height > 0,
                "Expected non-zero height/width, got %ux%u\n",
                mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.width, mi[pi[i].sourceInfo.modeInfoIdx].sourceMode.height);


        if (pi[i].targetInfo.modeInfoIdx == DISPLAYCONFIG_PATH_MODE_IDX_INVALID)
        {
            skip("Path doesn't contain target modeInfoIdx\n");
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
                "Expected LUID %08lx:%08lx, got %08lx:%08lx\n",
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
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, NULL, &modes, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, NULL, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 0;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = 0;
    modes = 1;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    ok (paths == 0, "got %u\n", paths);
    ok (modes == 1, "got %u\n", modes);

    /* Crashes on Windows 10 */
    if (0)
    {
        ret = pQueryDisplayConfig(QDC_ALL_PATHS, NULL, pi, NULL, mi, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

        ret = pQueryDisplayConfig(QDC_ALL_PATHS, NULL, pi, &modes, mi, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);
    }

    paths = modes = 1;
    ret = pQueryDisplayConfig(0, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(0xFF, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(QDC_DATABASE_CURRENT, &paths, pi, &modes, mi, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    paths = modes = 1;
    ret = pQueryDisplayConfig(QDC_ALL_PATHS, &paths, pi, &modes, mi, &topologyid);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

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
    ok(ret == ERROR_INSUFFICIENT_BUFFER, "got %ld\n", ret);
    ok (paths == 1, "got %u\n", paths);
    ok (modes == 1, "got %u\n", modes);

    paths = ARRAY_SIZE(pi);
    modes = ARRAY_SIZE(mi);
    memset(pi, 0xFF, sizeof(pi));
    memset(mi, 0xFF, sizeof(mi));
    ret = pQueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &paths, pi, &modes, mi, NULL);
    ok(!ret, "got %ld\n", ret);
    ok(paths > 0 && modes > 0, "got %u, %u\n", paths, modes);
    if (!ret && paths > 0 && modes > 0)
        test_QueryDisplayConfig_result(QDC_ONLY_ACTIVE_PATHS, paths, pi, modes, mi);

    paths = ARRAY_SIZE(pi);
    modes = ARRAY_SIZE(mi);
    memset(pi, 0xFF, sizeof(pi));
    memset(mi, 0xFF, sizeof(mi));
    topologyid = 0xFF;
    ret = pQueryDisplayConfig(QDC_DATABASE_CURRENT, &paths, pi, &modes, mi, &topologyid);
    ok(!ret, "got %ld\n", ret);
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
    ok(ret == ERROR_GEN_FAILURE, "got %ld\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE, "got %ld\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE, "got %ld\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = sizeof(source_name.header);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name.header);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    source_name.header.type = 0xFFFF;
    source_name.header.size = sizeof(source_name);
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(source_name);
    source_name.header.adapterId.LowPart = 0xFFFF;
    source_name.header.adapterId.HighPart = 0xFFFF;
    source_name.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&source_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);

    target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target_name.header.size = sizeof(target_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&target_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    target_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target_name.header.size = sizeof(target_name);
    target_name.header.adapterId.LowPart = 0xFFFF;
    target_name.header.adapterId.HighPart = 0xFFFF;
    target_name.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&target_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);

    preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
    preferred_mode.header.size = sizeof(preferred_mode) - 1;
    ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    preferred_mode.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
    preferred_mode.header.size = sizeof(preferred_mode);
    preferred_mode.header.adapterId.LowPart = 0xFFFF;
    preferred_mode.header.adapterId.HighPart = 0xFFFF;
    preferred_mode.header.id = 0;
    ret = pDisplayConfigGetDeviceInfo(&preferred_mode.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);

    adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
    adapter_name.header.size = sizeof(adapter_name) - 1;
    ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
    ok(ret == ERROR_INVALID_PARAMETER, "got %ld\n", ret);

    adapter_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
    adapter_name.header.size = sizeof(adapter_name);
    adapter_name.header.adapterId.LowPart = 0xFFFF;
    adapter_name.header.adapterId.HighPart = 0xFFFF;
    ret = pDisplayConfigGetDeviceInfo(&adapter_name.header);
    ok(ret == ERROR_GEN_FAILURE || ret == ERROR_INVALID_PARAMETER || ret == ERROR_NOT_SUPPORTED, "got %ld\n", ret);
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

static void test_DisplayConfigSetDeviceInfo(void)
{
    static const unsigned int scales[] = {100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500};
    static const DWORD enabled = 1;
    int current_scale, current_scale_idx, recommended_scale_idx, step, dpi, old_dpi;
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    DISPLAYCONFIG_GET_SOURCE_DPI_SCALE get_scale_req;
    DISPLAYCONFIG_SET_SOURCE_DPI_SCALE set_scale_req;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;
    HKEY key;
    LONG ret;

#define CHECK_FUNC(func)                           \
    if (!p##func)                                  \
    {                                              \
        win_skip("%s() is unavailable.\n", #func); \
        ret = TRUE;                                \
    }

    ret = FALSE;
    CHECK_FUNC(D3DKMTCloseAdapter)
    CHECK_FUNC(D3DKMTOpenAdapterFromGdiDisplayName)
    CHECK_FUNC(DisplayConfigGetDeviceInfo)
    todo_wine CHECK_FUNC(DisplayConfigSetDeviceInfo)
    CHECK_FUNC(GetDpiForMonitorInternal)
    CHECK_FUNC(SetThreadDpiAwarenessContext)
    if (ret) return;

#undef CHECK_FUNC

    lstrcpyW(open_adapter_gdi_desc.DeviceName, L"\\\\.\\DISPLAY1");
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_SUCCESS, "D3DKMTOpenAdapterFromGdiDisplayName failed, status %#lx.\n", status);

    get_scale_req.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_DPI_SCALE;
    get_scale_req.header.size = sizeof(get_scale_req);
    get_scale_req.header.adapterId = open_adapter_gdi_desc.AdapterLuid;
    get_scale_req.header.id = open_adapter_gdi_desc.VidPnSourceId;
    ret = pDisplayConfigGetDeviceInfo(&get_scale_req.header);
    if (ret != NO_ERROR)
    {
        skip("DisplayConfigGetDeviceInfo failed, returned %ld.\n", ret);
        goto failed;
    }

    /* Set IgnorePerProcessSystemDPIToast to 1 to disable "fix blurry apps popup" on Windows 10,
     * which may interfere other tests because it steals focus */
    RegOpenKeyA(HKEY_CURRENT_USER, "Control Panel\\Desktop", &key);
    RegSetValueExA(key, "IgnorePerProcessSystemDPIToast", 0, REG_DWORD, (const BYTE *)&enabled,
                   sizeof(enabled));

    dpi = get_primary_dpi();
    old_dpi = dpi;
    current_scale = dpi * 100 / 96;
    for (current_scale_idx = 0; current_scale_idx < ARRAY_SIZE(scales); ++current_scale_idx)
    {
        if (scales[current_scale_idx] == current_scale)
            break;
    }
    ok(scales[current_scale_idx] == current_scale, "Failed to find current scale.\n");
    recommended_scale_idx = current_scale_idx - get_scale_req.curRelativeScaleStep;

    set_scale_req.header.type = DISPLAYCONFIG_DEVICE_INFO_SET_SOURCE_DPI_SCALE;
    set_scale_req.header.size = sizeof(set_scale_req);
    set_scale_req.header.adapterId = open_adapter_gdi_desc.AdapterLuid;
    set_scale_req.header.id = open_adapter_gdi_desc.VidPnSourceId;
    for (step = get_scale_req.minRelativeScaleStep; step <= get_scale_req.maxRelativeScaleStep; ++step)
    {
        set_scale_req.relativeScaleStep = step;
        ret = pDisplayConfigSetDeviceInfo(&set_scale_req.header);
        ok(ret == NO_ERROR, "DisplayConfigSetDeviceInfo failed, returned %ld.\n", ret);

        dpi = scales[step + recommended_scale_idx] * 96 / 100;
        ok(dpi == get_primary_dpi(), "Expected %d, got %d.\n", get_primary_dpi(), dpi);
    }

    /* Restore to the original scale */
    set_scale_req.relativeScaleStep = get_scale_req.curRelativeScaleStep;
    ret = pDisplayConfigSetDeviceInfo(&set_scale_req.header);
    ok(ret == NO_ERROR, "DisplayConfigSetDeviceInfo failed, returned %ld.\n", ret);
    ok(old_dpi == get_primary_dpi(), "Expected %d, got %d.\n", get_primary_dpi(), old_dpi);

    /* Remove IgnorePerProcessSystemDPIToast registry value */
    RegDeleteValueA(key, "IgnorePerProcessSystemDPIToast");
    RegCloseKey(key);

failed:
    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status);
}

static BOOL CALLBACK test_handle_proc(HMONITOR full_monitor, HDC hdc, LPRECT rect, LPARAM lparam)
{
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    HMONITOR monitor;
    BOOL ret;

#ifdef _WIN64
    if ((ULONG_PTR)full_monitor >> 32)
        monitor = full_monitor;
    else
        monitor = (HMONITOR)((ULONG_PTR)full_monitor | ((ULONG_PTR)~0u << 32));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(ret, "GetMonitorInfoW failed, error %#lx.\n", GetLastError());

    monitor = (HMONITOR)((ULONG_PTR)full_monitor & 0xffffffff);
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(ret, "GetMonitorInfoW failed, error %#lx.\n", GetLastError());

    monitor = (HMONITOR)(((ULONG_PTR)full_monitor & 0xffffffff) | ((ULONG_PTR)0x1234 << 32));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(ret, "GetMonitorInfoW failed, error %#lx.\n", GetLastError());

    monitor = (HMONITOR)((ULONG_PTR)full_monitor & 0xffff);
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    todo_wine ok(!ret, "GetMonitorInfoW succeeded.\n");
    todo_wine ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Expected error code %#x, got %#lx.\n",
            ERROR_INVALID_MONITOR_HANDLE, GetLastError());

    monitor = (HMONITOR)(((ULONG_PTR)full_monitor & 0xffff) | ((ULONG_PTR)0x9876 << 16));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(!ret, "GetMonitorInfoW succeeded.\n");
    ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Expected error code %#x, got %#lx.\n",
            ERROR_INVALID_MONITOR_HANDLE, GetLastError());

    monitor = (HMONITOR)(((ULONG_PTR)full_monitor & 0xffff) | ((ULONG_PTR)0x12345678 << 16));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(!ret, "GetMonitorInfoW succeeded.\n");
    ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Expected error code %#x, got %#lx.\n",
            ERROR_INVALID_MONITOR_HANDLE, GetLastError());
#else
    if ((ULONG_PTR)full_monitor >> 16)
        monitor = full_monitor;
    else
        monitor = (HMONITOR)((ULONG_PTR)full_monitor | ((ULONG_PTR)~0u << 16));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    todo_wine_if(((ULONG_PTR)full_monitor >> 16) == 0)
    ok(ret, "GetMonitorInfoW failed, error %#lx.\n", GetLastError());

    monitor = (HMONITOR)((ULONG_PTR)full_monitor & 0xffff);
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(ret, "GetMonitorInfoW failed, error %#lx.\n", GetLastError());

    monitor = (HMONITOR)(((ULONG_PTR)full_monitor & 0xffff) | ((ULONG_PTR)0x1234 << 16));
    SetLastError(0xdeadbeef);
    ret = GetMonitorInfoW(monitor, &monitor_info);
    ok(!ret, "GetMonitorInfoW succeeded.\n");
    ok(GetLastError() == ERROR_INVALID_MONITOR_HANDLE, "Expected error code %#x, got %#lx.\n",
            ERROR_INVALID_MONITOR_HANDLE, GetLastError());
#endif

    return TRUE;
}

static void test_handles(void)
{
    BOOL ret;

    /* Test that monitor handles are user32 handles */
    ret = EnumDisplayMonitors(NULL, NULL, test_handle_proc, 0);
    ok(ret, "EnumDisplayMonitors failed, error %#lx.\n", GetLastError());
}

#define check_display_dc(a, b, c) _check_display_dc(__LINE__, a, b, c)
static void _check_display_dc(INT line, HDC hdc, const DEVMODEA *dm, BOOL allow_todo)
{
    unsigned char buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])] = {0};
    BITMAPINFO *bmi = (BITMAPINFO *)buffer;
    DIBSECTION dib;
    BITMAP bitmap;
    HBITMAP hbmp;
    INT value;
    BOOL ret;

    value = GetDeviceCaps(hdc, HORZRES);
    todo_wine_if(allow_todo && dm->dmPelsWidth != GetSystemMetrics(SM_CXSCREEN))
    ok_(__FILE__, line)(value == dm->dmPelsWidth, "Expected HORZRES %ld, got %d.\n",
            dm->dmPelsWidth, value);

    value = GetDeviceCaps(hdc, VERTRES);
    todo_wine_if(allow_todo && dm->dmPelsHeight != GetSystemMetrics(SM_CYSCREEN))
    ok_(__FILE__, line)(value == dm->dmPelsHeight, "Expected VERTRES %ld, got %d.\n",
            dm->dmPelsHeight, value);

    value = GetDeviceCaps(hdc, DESKTOPHORZRES);
    todo_wine_if(dm->dmPelsWidth != GetSystemMetrics(SM_CXVIRTUALSCREEN)
            && value == GetSystemMetrics(SM_CXVIRTUALSCREEN))
    ok_(__FILE__, line)(value == dm->dmPelsWidth, "Expected DESKTOPHORZRES %ld, got %d.\n",
            dm->dmPelsWidth, value);

    value = GetDeviceCaps(hdc, DESKTOPVERTRES);
    todo_wine_if(dm->dmPelsHeight != GetSystemMetrics(SM_CYVIRTUALSCREEN)
            && value == GetSystemMetrics(SM_CYVIRTUALSCREEN))
    ok_(__FILE__, line)(value == dm->dmPelsHeight, "Expected DESKTOPVERTRES %ld, got %d.\n",
            dm->dmPelsHeight, value);

    value = GetDeviceCaps(hdc, VREFRESH);
    todo_wine_if(allow_todo)
    ok_(__FILE__, line)(value == dm->dmDisplayFrequency, "Expected VREFRESH %ld, got %d.\n",
            dm->dmDisplayFrequency, value);

    value = GetDeviceCaps(hdc, BITSPIXEL);
    ok_(__FILE__, line)(value == dm->dmBitsPerPel, "Expected BITSPIXEL %ld, got %d.\n",
            dm->dmBitsPerPel, value);

    hbmp = GetCurrentObject(hdc, OBJ_BITMAP);
    ok_(__FILE__, line)(!!hbmp, "GetCurrentObject failed, error %#lx.\n", GetLastError());

    /* Expect hbmp to be a bitmap, not a DIB when GetObjectA() succeeds */
    value = GetObjectA(hbmp, sizeof(dib), &dib);
    ok(!value || value == sizeof(BITMAP), "GetObjectA failed, value %d.\n", value);

    /* Expect GetDIBits() to succeed */
    bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    value = GetDIBits(hdc, hbmp, 0, 0, NULL, (LPBITMAPINFO)bmi, DIB_RGB_COLORS);
    ok(value, "GetDIBits failed, error %#lx.\n", GetLastError());
    ok(bmi->bmiHeader.biCompression == BI_BITFIELDS, "Got unexpected biCompression %lu.\n", bmi->bmiHeader.biCompression);

    ret = GetObjectA(hbmp, sizeof(bitmap), &bitmap);
    /* GetObjectA fails on Win7 and older */
    if (ret)
    {
        ok_(__FILE__, line)(bitmap.bmType == 0, "Expected bmType %d, got %d.\n", 0, bitmap.bmType);
        todo_wine
        ok_(__FILE__, line)(bitmap.bmWidth == GetSystemMetrics(SM_CXVIRTUALSCREEN),
                "Expected bmWidth %d, got %d.\n", GetSystemMetrics(SM_CXVIRTUALSCREEN), bitmap.bmWidth);
        todo_wine
        ok_(__FILE__, line)(bitmap.bmHeight == GetSystemMetrics(SM_CYVIRTUALSCREEN),
                "Expected bmHeight %d, got %d.\n", GetSystemMetrics(SM_CYVIRTUALSCREEN), bitmap.bmHeight);
        ok_(__FILE__, line)(bitmap.bmBitsPixel == 32, "Expected bmBitsPixel %d, got %d.\n", 32,
                bitmap.bmBitsPixel);
        ok_(__FILE__, line)(bitmap.bmWidthBytes == get_bitmap_stride(bitmap.bmWidth, bitmap.bmBitsPixel),
                "Expected bmWidthBytes %d, got %d.\n", get_bitmap_stride(bitmap.bmWidth, bitmap.bmBitsPixel),
                bitmap.bmWidthBytes);
        ok_(__FILE__, line)(bitmap.bmPlanes == 1, "Expected bmPlanes %d, got %d.\n", 1, bitmap.bmPlanes);
        ok_(__FILE__, line)(bitmap.bmBits == NULL, "Expected bmBits %p, got %p.\n", NULL, bitmap.bmBits);
    }
}

static void test_display_dc(void)
{
    static const INT bpps[] = {1, 4, 8, 16, 24, 32};
    unsigned char buffer[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bmi = (BITMAPINFO *)buffer;
    INT count, old_count, i, bpp, value;
    HBITMAP hbitmap, old_hbitmap;
    DWORD device_idx, mode_idx;
    DEVMODEA dm, dm2, dm3;
    DISPLAY_DEVICEA dd;
    HDC hdc, mem_dc;
    DIBSECTION dib;
    BITMAP bitmap;
    BOOL ret;
    LONG res;

    /* Test DCs covering the entire virtual screen */
    hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    ok(!!hdc, "CreateDCA failed.\n");

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed.\n");

    check_display_dc(hdc, &dm, FALSE);

    /* Test that CreateCompatibleBitmap() for display DCs creates DDBs */
    hbitmap = CreateCompatibleBitmap(hdc, dm.dmPelsWidth, dm.dmPelsHeight);
    ok(!!hbitmap, "CreateCompatibleBitmap failed, error %ld.\n", GetLastError());
    count = GetObjectW(hbitmap, sizeof(dib), &dib);
    ok(count == sizeof(BITMAP), "GetObject failed, count %d.\n", count);
    count = GetObjectW(hbitmap, sizeof(bitmap), &bitmap);
    ok(count == sizeof(BITMAP), "GetObject failed, count %d.\n", count);
    ok(bitmap.bmBitsPixel == dm.dmBitsPerPel, "Expected %ld, got %d.\n", dm.dmBitsPerPel,
       bitmap.bmBitsPixel);
    DeleteObject(hbitmap);

    /* Test selecting a DDB of a different depth into a display compatible DC */
    for (i = 0; i < ARRAY_SIZE(bpps); ++i)
    {
        winetest_push_context("bpp %d", bpps[i]);

        mem_dc = CreateCompatibleDC(hdc);
        hbitmap = CreateBitmap(dm.dmPelsWidth, dm.dmPelsHeight, 1, bpps[i], NULL);
        ok(!!hbitmap, "CreateBitmap failed, error %ld.\n", GetLastError());
        old_hbitmap = SelectObject(mem_dc, hbitmap);
        if (bpps[i] != 1 && bpps[i] != 32)
            ok(!old_hbitmap, "Selecting bitmap succeeded.\n");
        else
            ok(!!old_hbitmap, "Failed to select bitmap.\n");

        SelectObject(mem_dc, old_hbitmap);
        DeleteObject(hbitmap);
        DeleteDC(mem_dc);

        winetest_pop_context();
    }

    /* Test selecting a DIB of a different depth into a display compatible DC */
    for (i = 0; i < ARRAY_SIZE(bpps); ++i)
    {
        winetest_push_context("bpp %d", bpps[i]);

        mem_dc = CreateCompatibleDC(hdc);
        memset(buffer, 0, sizeof(buffer));
        bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
        bmi->bmiHeader.biWidth = dm.dmPelsWidth;
        bmi->bmiHeader.biHeight = dm.dmPelsHeight;
        bmi->bmiHeader.biBitCount = bpps[i];
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biCompression = BI_RGB;
        hbitmap = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, NULL, NULL, 0);
        ok(!!hbitmap, "CreateDIBSection failed, error %ld.\n", GetLastError());
        old_hbitmap = SelectObject(mem_dc, hbitmap);
        ok(!!old_hbitmap, "Failed to select bitmap.\n");

        value = GetDeviceCaps(mem_dc, BITSPIXEL);
        ok(value == 32, "Expected 32, got %d.\n", value);
        value = GetDeviceCaps(mem_dc, NUMCOLORS);
        ok(value == -1, "Expected -1, got %d.\n", value);

        SelectObject(mem_dc, old_hbitmap);
        DeleteObject(hbitmap);
        DeleteDC(mem_dc);

        winetest_pop_context();
    }

    /* Test NUMCOLORS for display DCs */
    for (i = ARRAY_SIZE(bpps) - 1; i >= 0; --i)
    {
        winetest_push_context("bpp %d", bpps[i]);

        ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        ok(ret, "EnumDisplaySettingsA failed.\n");
        if (bpps[i] == dm.dmBitsPerPel)
        {
            res = DISP_CHANGE_SUCCESSFUL;
        }
        else
        {
            memset(&dm2, 0, sizeof(dm2));
            dm2.dmSize = sizeof(dm2);
            for (mode_idx = 0; EnumDisplaySettingsA(NULL, mode_idx, &dm2); ++mode_idx)
            {
                if (dm2.dmBitsPerPel == bpps[i])
                    break;
            }
            if (dm2.dmBitsPerPel != bpps[i])
            {
                skip("%d-bit display mode not found.\n", bpps[i]);
                winetest_pop_context();
                continue;
            }

            res = ChangeDisplaySettingsExA(NULL, &dm2, NULL, CDS_RESET, NULL);
            /* Win8 TestBots */
            ok(res == DISP_CHANGE_SUCCESSFUL || broken(res == DISP_CHANGE_FAILED),
               "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);
        }

        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            value = GetDeviceCaps(hdc, BITSPIXEL);
            ok(value == (bpps[i] == 4 ? 1 : bpps[i]), "Expected %d, got %d.\n",
                    (bpps[i] == 4 ? 1 : bpps[i]), value);

            value = GetDeviceCaps(hdc, NUMCOLORS);
            if (bpps[i] > 8 || (bpps[i] == 8 && LOBYTE(LOWORD(GetVersion())) >= 6))
                ok(value == -1, "Expected -1, got %d.\n", value);
            else if (bpps[i] == 8 && LOBYTE(LOWORD(GetVersion())) < 6)
                ok(value > 16 && value <= 256, "Got %d.\n", value);
            else
                ok(value == 1 << bpps[i], "Expected %d, got %d.\n", 1 << bpps[i], value);
        }
        winetest_pop_context();
    }
    res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
    /* Win8 TestBots */
    ok(res == DISP_CHANGE_SUCCESSFUL || broken(res == DISP_CHANGE_FAILED),
       "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);

    /* Test selecting a DDB of the same color depth into a display compatible DC */
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed.\n");

    mem_dc = CreateCompatibleDC(hdc);
    bpp = GetDeviceCaps(mem_dc, BITSPIXEL);
    ok(bpp == dm.dmBitsPerPel, "Expected bpp %ld, got %d.\n", dm.dmBitsPerPel, bpp);
    hbitmap = CreateCompatibleBitmap(hdc, dm.dmPelsWidth, dm.dmPelsHeight);
    count = GetObjectW(hbitmap, sizeof(bitmap), &bitmap);
    ok(count == sizeof(BITMAP), "GetObject failed, count %d.\n", count);
    ok(bitmap.bmBitsPixel == dm.dmBitsPerPel, "Expected %ld, got %d.\n", dm.dmBitsPerPel, bitmap.bmBitsPixel);
    old_hbitmap = SelectObject(mem_dc, hbitmap);
    ok(!!old_hbitmap, "Failed to select bitmap.\n");
    SelectObject(mem_dc, old_hbitmap);
    DeleteDC(mem_dc);

    /* Tests after mode changes to a mode with different resolution and color depth */
    memset(&dm2, 0, sizeof(dm2));
    dm2.dmSize = sizeof(dm2);
    for (mode_idx = 0; EnumDisplaySettingsA(NULL, mode_idx, &dm2); ++mode_idx)
    {
        if (dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight &&
            dm2.dmBitsPerPel != dm.dmBitsPerPel && dm2.dmBitsPerPel != 1)
            break;
    }
    if (dm2.dmBitsPerPel && dm2.dmBitsPerPel != dm.dmBitsPerPel)
    {
        res = ChangeDisplaySettingsExA(NULL, &dm2, NULL, CDS_RESET, NULL);
        /* Win8 TestBots */
        ok(res == DISP_CHANGE_SUCCESSFUL || broken(res == DISP_CHANGE_FAILED),
                "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);
        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            check_display_dc(hdc, &dm2, FALSE);

            /* Test selecting a compatible bitmap of a different color depth previously created for
             * a display DC into a new compatible DC */
            count = GetObjectW(hbitmap, sizeof(bitmap), &bitmap);
            ok(count == sizeof(BITMAP), "GetObject failed, count %d.\n", count);
            ok(bitmap.bmBitsPixel == dm.dmBitsPerPel, "Expected %ld, got %d.\n", dm.dmBitsPerPel,
               bitmap.bmBitsPixel);

            /* Note that hbitmap is of a different color depth and it can be successfully selected
             * into the new compatible DC */
            mem_dc = CreateCompatibleDC(hdc);
            bpp = GetDeviceCaps(mem_dc, BITSPIXEL);
            ok(bpp == dm2.dmBitsPerPel, "Expected bpp %ld, got %d.\n", dm2.dmBitsPerPel, bpp);
            old_hbitmap = SelectObject(mem_dc, hbitmap);
            ok(!!old_hbitmap, "Failed to select bitmap.\n");
            bpp = GetDeviceCaps(mem_dc, BITSPIXEL);
            ok(bpp == dm2.dmBitsPerPel, "Expected bpp %ld, got %d.\n", dm2.dmBitsPerPel, bpp);
            SelectObject(mem_dc, old_hbitmap);
            DeleteDC(mem_dc);
            DeleteObject(hbitmap);

            /* Test selecting a DDB of a different color depth into a display compatible DC */
            for (i = 0; i < ARRAY_SIZE(bpps); ++i)
            {
                winetest_push_context("bpp %d", bpps[i]);

                mem_dc = CreateCompatibleDC(hdc);
                hbitmap = CreateBitmap(dm2.dmPelsWidth, dm2.dmPelsHeight, 1, bpps[i], NULL);
                ok(!!hbitmap, "CreateBitmap failed, error %ld.\n", GetLastError());
                old_hbitmap = SelectObject(mem_dc, hbitmap);
                /* On Win7 dual-QXL test bot and XP, only 1-bit DDBs and DDBs with the same color
                 * depth can be selected to the compatible DC. On newer versions of Windows, only
                 * 1-bit and 32-bit DDBs can be selected into the compatible DC even if
                 * GetDeviceCaps(BITSPIXEL) reports a different color depth, for example, 16-bit.
                 * It's most likely due to the fact that lower color depth are emulated on newer
                 * versions of Windows as the real color depth is still 32-bit */
                if (bpps[i] != 1 && bpps[i] != 32)
                    todo_wine_if(bpps[i] == dm2.dmBitsPerPel)
                    ok(!old_hbitmap || broken(!!old_hbitmap) /* Win7 dual-QXL test bot and XP */,
                       "Selecting bitmap succeeded.\n");
                else
                    ok(!!old_hbitmap || broken(!old_hbitmap) /* Win7 dual-QXL test bot and XP */,
                       "Failed to select bitmap.\n");

                SelectObject(mem_dc, old_hbitmap);
                DeleteObject(hbitmap);
                DeleteDC(mem_dc);

                winetest_pop_context();
            }
            hbitmap = NULL;

            res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);
        }
    }
    else
    {
        win_skip("Failed to find a required display mode.\n");
    }

    if (hbitmap)
        DeleteObject(hbitmap);
    DeleteDC(hdc);

    /* Test DCs covering a specific monitor */
    dd.cb = sizeof(dd);
    for (device_idx = 0; EnumDisplayDevicesA(NULL, device_idx, &dd, 0); ++device_idx)
    {
        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        ret = EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);
        ok(ret, "EnumDisplaySettingsA %s failed.\n", dd.DeviceName);

        hdc = CreateDCA(dd.DeviceName, NULL, NULL, NULL);
        ok(!!hdc, "CreateDCA %s failed.\n", dd.DeviceName);

        check_display_dc(hdc, &dm, FALSE);

        /* Tests after mode changes to a different resolution */
        memset(&dm2, 0, sizeof(dm2));
        dm2.dmSize = sizeof(dm2);
        for (mode_idx = 0; EnumDisplaySettingsA(dd.DeviceName, mode_idx, &dm2); ++mode_idx)
        {
            if (dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight)
                break;
        }
        ok(dm2.dmPelsWidth && dm2.dmPelsWidth != dm.dmPelsWidth && dm2.dmPelsHeight != dm.dmPelsHeight,
                "Failed to find a different resolution for %s.\n", dd.DeviceName);

        res = ChangeDisplaySettingsExA(dd.DeviceName, &dm2, NULL, CDS_RESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL || broken(res == DISP_CHANGE_FAILED), /* Win8 TestBots */
                "ChangeDisplaySettingsExA %s returned unexpected %ld.\n", dd.DeviceName, res);
        if (res != DISP_CHANGE_SUCCESSFUL)
        {
            win_skip("Failed to change display mode for %s.\n", dd.DeviceName);
            DeleteDC(hdc);
            continue;
        }

        check_display_dc(hdc, &dm2, FALSE);

        /* Tests after mode changes to a different color depth */
        memset(&dm2, 0, sizeof(dm2));
        dm2.dmSize = sizeof(dm2);
        for (mode_idx = 0; EnumDisplaySettingsA(dd.DeviceName, mode_idx, &dm2); ++mode_idx)
        {
            if (dm2.dmBitsPerPel != dm.dmBitsPerPel)
                break;
        }
        if (dm2.dmBitsPerPel && dm2.dmBitsPerPel != dm.dmBitsPerPel)
        {
            res = ChangeDisplaySettingsExA(dd.DeviceName, &dm2, NULL, CDS_RESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);

            check_display_dc(hdc, &dm2, FALSE);

            res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA returned unexpected %ld.\n", res);
        }
        else
        {
            win_skip("Failed to find a different color depth other than %lu.\n", dm.dmBitsPerPel);
        }

        /* Tests after monitor detach */
        ret = EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm2);
        ok(ret, "EnumDisplaySettingsA %s failed.\n", dd.DeviceName);

        if (!(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
        {
            old_count = GetSystemMetrics(SM_CMONITORS);

            memset(&dm3, 0, sizeof(dm3));
            dm3.dmSize = sizeof(dm3);
            ret = EnumDisplaySettingsA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm3);
            ok(ret, "EnumDisplaySettingsA %s failed.\n", dd.DeviceName);

            dm3.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
            dm3.dmPelsWidth = 0;
            dm3.dmPelsHeight = 0;
            res = ChangeDisplaySettingsExA(dd.DeviceName, &dm3, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                    dd.DeviceName, res);
            res = ChangeDisplaySettingsExA(dd.DeviceName, NULL, NULL, 0, NULL);
            ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                    dd.DeviceName, res);

            count = GetSystemMetrics(SM_CMONITORS);
            ok(count == old_count - 1, "Expected monitor count %d, got %d.\n", old_count - 1, count);

            /* Should report the same values before detach */
            check_display_dc(hdc, &dm2, TRUE);
        }

        res = ChangeDisplaySettingsExA(dd.DeviceName, &dm, NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                dd.DeviceName, res);
        res = ChangeDisplaySettingsExA(NULL, NULL, NULL, 0, NULL);
        ok(res == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExA %s returned unexpected %ld.\n",
                dd.DeviceName, res);
        DeleteDC(hdc);
    }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData)
{
    MONITORINFOEXW info;
    static int index;

    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(hMonitor, (MONITORINFO*)&info))
    {
        printf("Monitor %d %7s [%02lx] %s %s\n", index,
              (info.dwFlags & MONITORINFOF_PRIMARY) ? "primary" : "",
               info.dwFlags, wine_dbgstr_rect(&info.rcMonitor),
               wine_dbgstr_w(info.szDevice));
    }
    index++;
    return TRUE;
}

START_TEST(monitor)
{
    char** myARGV;
    int myARGC = winetest_get_mainargs(&myARGV);

    init_function_pointers();

    if (myARGC >= 3 && strcmp(myARGV[2], "info") == 0)
    {
        printf("Monitor information:\n");
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
        return;
    }

    test_enumdisplaydevices();
    test_ChangeDisplaySettingsEx();
    test_DisplayConfigSetDeviceInfo();
    test_EnumDisplayMonitors();
    test_monitors();
    test_work_area();
    test_display_config();
    test_handles();
    test_display_dc();
}
