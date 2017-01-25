/*
 * Copyright (c) 2017 Aric Stewart
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
#include "windows.h"
#include "setupapi.h"
#include "ddk/hidsdi.h"

#include "wine/test.h"

typedef void (device_test)(HANDLE device);

static void test_device_info(HANDLE device)
{
    PHIDP_PREPARSED_DATA ppd;
    HIDP_CAPS Caps;
    NTSTATUS status;
    BOOL rc;
    WCHAR device_name[128];

    rc = HidD_GetPreparsedData(device, &ppd);
    ok(rc, "Failed to get preparsed data(0x%x)\n", GetLastError());
    status = HidP_GetCaps(ppd, &Caps);
    ok(status == HIDP_STATUS_SUCCESS, "Failed to get Caps(0x%x)\n", status);
    rc = HidD_GetProductString(device, device_name, sizeof(device_name));
    ok(rc, "Failed to get product string(0x%x)\n", GetLastError());
    trace("Found device %s (%02x, %02x)\n", wine_dbgstr_w(device_name), Caps.UsagePage, Caps.Usage);
    rc = HidD_FreePreparsedData(ppd);
    ok(rc, "Failed to free preparsed data(0x%x)\n", GetLastError());
}

static void run_for_each_device(device_test *test)
{
    GUID hid_guid;
    HDEVINFO info_set;
    DWORD index = 0;
    SP_DEVICE_INTERFACE_DATA interface_data;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *data;

    HidD_GetHidGuid(&hid_guid);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    data = HeapAlloc(GetProcessHeap(), 0, sizeof(*data) + detail_size);
    data->cbSize = sizeof(*data);

    info_set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    while (SetupDiEnumDeviceInterfaces(info_set, NULL, &hid_guid, index, &interface_data))
    {
        index ++;

        if (SetupDiGetDeviceInterfaceDetailW(info_set, &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
        {
            HANDLE file = CreateFileW(data->DevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
            if (file == INVALID_HANDLE_VALUE)
            {
                trace("Failed to access device %s, likely not plugged in or access is denied.\n", wine_dbgstr_w(data->DevicePath));
                continue;
            }

            test(file);

            CloseHandle(file);
        }
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(info_set);
}

START_TEST(device)
{
    run_for_each_device(test_device_info);
}
