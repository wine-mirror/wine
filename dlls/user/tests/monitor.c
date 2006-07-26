/*
 * Unit tests for monitor APIs
 *
 * Copyright 2005 Huw Davies
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

static HMODULE hdll;
static BOOL (WINAPI *pEnumDisplayDevicesA)(LPCSTR,DWORD,LPDISPLAY_DEVICEA,DWORD);
static BOOL (WINAPI *pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
static BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR,LPMONITORINFO);

static void init_function_pointers(void)
{
    hdll = GetModuleHandleA("user32.dll");

    if(hdll)
    {
       pEnumDisplayDevicesA = (void*)GetProcAddress(hdll, "EnumDisplayDevicesA");
       pEnumDisplayMonitors = (void*)GetProcAddress(hdll, "EnumDisplayMonitors");
       pGetMonitorInfoA = (void*)GetProcAddress(hdll, "GetMonitorInfoA");
    }
}

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc,
                                       LPARAM lparam)
{
    MONITORINFOEXA mi;
    char *primary = (char *)lparam;

    mi.cbSize = sizeof(mi);

    ok(pGetMonitorInfoA(hmon, (MONITORINFO*)&mi), "GetMonitorInfo failed\n");
    if(mi.dwFlags == MONITORINFOF_PRIMARY)
        strcpy(primary, mi.szDevice);

    return TRUE;
}

static void test_enumdisplaydevices(void)
{
    DISPLAY_DEVICEA dd;
    char primary_device_name[32];
    char primary_monitor_device_name[32];
    DWORD primary_num = -1, num = 0;

    dd.cb = sizeof(dd);
    if(pEnumDisplayDevicesA == NULL) return;
    while(1)
    {
        BOOL ret;
        HDC dc;
        ret = pEnumDisplayDevicesA(NULL, num, &dd, 0);
        ok(ret || num != 0, "EnumDisplayDevices fails with num == 0\n");
        if(!ret) break;
        if(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            strcpy(primary_device_name, dd.DeviceName);
            primary_num = num;
        }
        if(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        {
            /* test creating DC */
            dc = CreateDCA(dd.DeviceName, NULL, NULL, NULL);
            ok(dc != NULL, "Failed to CreateDC(\"%s\") err=%ld\n", dd.DeviceName, GetLastError());
            DeleteDC(dc);
        }
        num++;
    }
    ok(primary_num != -1, "Didn't get the primary device\n");

    if(pEnumDisplayMonitors && pGetMonitorInfoA) {
        ok(pEnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)primary_monitor_device_name),
           "EnumDisplayMonitors failed\n");

        ok(!strcmp(primary_monitor_device_name, primary_device_name),
           "monitor device name %s, device name %s\n", primary_monitor_device_name,
           primary_device_name);
    }
}

struct vid_mode
{
    DWORD w, h, bpp, freq, fields;
    LONG res;
};

static struct vid_mode vid_modes_test[] = {
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY, DISP_CHANGE_SUCCESSFUL},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT |                 DM_DISPLAYFREQUENCY, DISP_CHANGE_SUCCESSFUL},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL                      , DISP_CHANGE_SUCCESSFUL},
    {640, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT                                      , DISP_CHANGE_SUCCESSFUL},
    {640, 480, 0, 0,                                DM_BITSPERPEL                      , DISP_CHANGE_SUCCESSFUL},
    {640, 480, 0, 0,                                                DM_DISPLAYFREQUENCY, DISP_CHANGE_SUCCESSFUL},

    {0, 0, 0, 0, DM_PELSWIDTH, DISP_CHANGE_SUCCESSFUL},
    {0, 0, 0, 0, DM_PELSHEIGHT, DISP_CHANGE_SUCCESSFUL},

    {640, 480, 0, 0, DM_PELSWIDTH, DISP_CHANGE_BADMODE},
    {640, 480, 0, 0, DM_PELSHEIGHT, DISP_CHANGE_BADMODE},
    {  0, 480, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, DISP_CHANGE_BADMODE},
    {640,   0, 0, 0, DM_PELSWIDTH | DM_PELSHEIGHT, DISP_CHANGE_BADMODE},

    {0, 0, 0, 10, DM_DISPLAYFREQUENCY, DISP_CHANGE_BADMODE},
};
#define vid_modes_cnt (sizeof(vid_modes_test) / sizeof(vid_modes_test[0]))

static void test_ChangeDisplaySettingsEx(void)
{
    DEVMODE dm;
    LONG res;
    int i;

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);

    for (i = 0; i < vid_modes_cnt; i++)
    {
        dm.dmPelsWidth        = vid_modes_test[i].w;
        dm.dmPelsHeight       = vid_modes_test[i].h;
        dm.dmBitsPerPel       = vid_modes_test[i].bpp;
        dm.dmDisplayFrequency = vid_modes_test[i].freq;
        dm.dmFields           = vid_modes_test[i].fields;
        res = ChangeDisplaySettingsEx(NULL, &dm, NULL, CDS_FULLSCREEN, NULL);
        ok(res == vid_modes_test[i].res, "Failed to change resolution[%d]: %ld\n", i, res);
    }
    res = ChangeDisplaySettingsEx(NULL, NULL, NULL, CDS_RESET, NULL);
    ok(res == DISP_CHANGE_SUCCESSFUL, "Failed to reset default resolution: %ld\n", res);
}


START_TEST(monitor)
{
    init_function_pointers();
    test_enumdisplaydevices();
    test_ChangeDisplaySettingsEx();
}
