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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

static BOOL CALLBACK monitor_enum_proc(HMONITOR hmon, HDC hdc, LPRECT lprc,
                                       LPARAM lparam)
{
    MONITORINFOEXA mi;
    char *primary = (char *)lparam;

    mi.cbSize = sizeof(mi);

    ok(GetMonitorInfoA(hmon, (MONITORINFO*)&mi), "GetMonitorInfo failed\n");
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
    while(1)
    {
        BOOL ret;
        ret = EnumDisplayDevicesA(NULL, num, &dd, 0), "EnumDisplayDevices fails\n";
        ok(ret || num != 0, "EnumDisplayDevices fails with num == 0\n");
        if(!ret) break;
        if(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            strcpy(primary_device_name, dd.DeviceName);
            primary_num = num;
        }
        num++;
    }
    ok(primary_num != -1, "Didn't get the primary device\n");

    ok(EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM)primary_monitor_device_name),
       "EnumDisplayMonitors failed\n");

    ok(!strcmp(primary_monitor_device_name, primary_device_name),
       "monitor device name %s, device name %s\n", primary_monitor_device_name,
       primary_device_name);
}



START_TEST(monitor)
{
    test_enumdisplaydevices();
}
