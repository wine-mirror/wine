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

#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "setupapi.h"
#include "hidusage.h"
#include "ddk/hidsdi.h"

#include "wine/test.h"

#define READ_MAX_TIME 5000

typedef void (device_test)(HANDLE device);

static void test_device_info(HANDLE device)
{
    PHIDP_PREPARSED_DATA ppd;
    HIDP_CAPS Caps;
    HIDD_ATTRIBUTES attributes;
    HIDP_LINK_COLLECTION_NODE nodes[16];
    ULONG nodes_count;
    NTSTATUS status;
    BOOL rc;
    WCHAR device_name[128];
    int i;

    rc = HidD_GetPreparsedData(device, &ppd);
    ok(rc, "Failed to get preparsed data(0x%lx)\n", GetLastError());
    status = HidP_GetCaps(ppd, &Caps);
    ok(status == HIDP_STATUS_SUCCESS, "Failed to get Caps(0x%lx)\n", status);
    rc = HidD_GetProductString(device, device_name, sizeof(device_name));
    ok(rc, "Failed to get product string(0x%lx)\n", GetLastError());
    trace("Found device %s (%02x, %02x)\n", wine_dbgstr_w(device_name), Caps.UsagePage, Caps.Usage);

    trace("LinkCollectionNodes: (%d)\n", Caps.NumberLinkCollectionNodes);
    ok(Caps.NumberLinkCollectionNodes > 0, "Expected at least one link collection\n");

    nodes_count = 0;
    status = HidP_GetLinkCollectionNodes(nodes, &nodes_count, ppd);
    ok(status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetLinkCollectionNodes succeeded:%lx\n", status);

    nodes_count = ARRAY_SIZE(nodes);
    status = HidP_GetLinkCollectionNodes(nodes, &nodes_count, ppd);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetLinkCollectionNodes failed:%lx\n", status);

    for (i = 0; i < min(nodes_count, ARRAY_SIZE(nodes)); ++i)
    {
        trace("  [%d] LinkUsage: %x LinkUsagePage: %x Parent: %x "
              "NumberOfChildren: %x NextSibling: %x FirstChild: %x "
              "CollectionType: %x IsAlias: %x UserContext: %p\n",
              i, nodes[i].LinkUsage, nodes[i].LinkUsagePage, nodes[i].Parent,
              nodes[i].NumberOfChildren, nodes[i].NextSibling, nodes[i].FirstChild,
              nodes[i].CollectionType, nodes[i].IsAlias, nodes[i].UserContext);
    }

    ok(nodes_count > 0, "Unexpected number of link collection nodes:%lu.\n", nodes_count);
    ok(nodes[0].LinkUsagePage == Caps.UsagePage, "Unexpected top collection usage page:%x\n", nodes[0].LinkUsagePage);
    ok(nodes[0].LinkUsage == Caps.Usage, "Unexpected top collection usage:%x\n", nodes[0].LinkUsage);
    ok(nodes[0].CollectionType == 1, "Unexpected top collection type:%x\n", nodes[0].CollectionType);

    rc = HidD_FreePreparsedData(ppd);
    ok(rc, "Failed to free preparsed data(0x%lx)\n", GetLastError());
    rc = HidD_GetAttributes(device, &attributes);
    ok(rc, "Failed to get device attributes (0x%lx)\n", GetLastError());
    ok(attributes.Size == sizeof(attributes), "Unexpected HIDD_ATTRIBUTES size: %ld\n", attributes.Size);
    trace("Device attributes: vid:%04x pid:%04x ver:%04x\n", attributes.VendorID, attributes.ProductID, attributes.VersionNumber);
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

    info_set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    while (SetupDiEnumDeviceInterfaces(info_set, NULL, &hid_guid, index, &interface_data))
    {
        index ++;

        if (SetupDiGetDeviceInterfaceDetailW(info_set, &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
        {
            HANDLE file = CreateFileW(data->DevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
            if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED)
            {
                trace("Not enough permissions to read device %s.\n", wine_dbgstr_w(data->DevicePath));
                continue;
            }
            if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION)
            {
                trace("Device is busy: %s.\n", wine_dbgstr_w(data->DevicePath));
                continue;
            }

            ok(file != INVALID_HANDLE_VALUE, "Failed to open %s, error %lu.\n",
                wine_dbgstr_w(data->DevicePath), GetLastError());

            if (file != INVALID_HANDLE_VALUE)
                test(file);

            CloseHandle(file);
        }
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(info_set);
}

static HANDLE get_device(USHORT page, USHORT usages[], UINT usage_count, DWORD access)
{
    GUID hid_guid;
    HDEVINFO info_set;
    DWORD index = 0;
    SP_DEVICE_INTERFACE_DATA interface_data;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *data;
    NTSTATUS status;
    BOOL rc;

    HidD_GetHidGuid(&hid_guid);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    data = HeapAlloc(GetProcessHeap(), 0 , sizeof(*data) + detail_size);
    data->cbSize = sizeof(*data);

    info_set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    while (SetupDiEnumDeviceInterfaces(info_set, NULL, &hid_guid, index, &interface_data))
    {
        index ++;

        if (SetupDiGetDeviceInterfaceDetailW(info_set, &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
        {
            PHIDP_PREPARSED_DATA ppd;
            HIDP_CAPS Caps;
            HANDLE file = CreateFileW(data->DevicePath, access, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
            if (file == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED)
            {
                trace("Not enough permissions to read device %s.\n", wine_dbgstr_w(data->DevicePath));
                continue;
            }
            ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

            rc = HidD_GetPreparsedData(file, &ppd);
            ok(rc, "Failed to get preparsed data(0x%lx)\n", GetLastError());
            status = HidP_GetCaps(ppd, &Caps);
            ok(status == HIDP_STATUS_SUCCESS, "Failed to get Caps(0x%lx)\n", status);
            rc = HidD_FreePreparsedData(ppd);
            ok(rc, "Failed to free preparsed data(0x%lx)\n", GetLastError());
            if (!page || page == Caps.UsagePage)
            {
                int j;
                if (!usage_count)
                {
                    HeapFree(GetProcessHeap(), 0, data);
                    SetupDiDestroyDeviceInfoList(info_set);
                    return file;
                }
                for (j = 0; j < usage_count; j++)
                    if (!usages[j] || usages[j] == Caps.Usage)
                    {
                        HeapFree(GetProcessHeap(), 0, data);
                        SetupDiDestroyDeviceInfoList(info_set);
                        return file;
                    }
            }
            CloseHandle(file);
        }
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(info_set);
    return NULL;
}

static void process_data(HIDP_CAPS Caps, PHIDP_PREPARSED_DATA ppd, CHAR *data, DWORD data_length)
{
    INT i;
    NTSTATUS status;

    if (Caps.NumberInputButtonCaps)
    {
        USAGE button_pages[100];

        for (i = 1; i < 0xff; i++)
        {
            ULONG usage_length = 100;
            status = HidP_GetUsages(HidP_Input, i, 0, button_pages, &usage_length, ppd, data, data_length);
            ok (status == HIDP_STATUS_SUCCESS || usage_length == 0,
                "HidP_GetUsages failed (%lx) but usage length still %li\n", status, usage_length);
            if (status == HIDP_STATUS_SUCCESS && usage_length)
            {
                CHAR report[50];
                int count;
                int j;

                count = usage_length;
                j = 0;
                report[0] = 0;
                trace("\tButtons [0x%x: %li buttons]:\n", i, usage_length);
                for (count = 0; count < usage_length; count += 15)
                {
                    for (j=count; j < count+15 && j < usage_length; j++)
                    {
                        CHAR btn[7];
                        sprintf(btn, "%i ", button_pages[j]);
                        strcat(report, btn);
                    }
                    trace("\t\t%s\n", report);
                }
            }
        }
    }

    if (Caps.NumberInputValueCaps)
    {
        ULONG value;
        USHORT length;
        HIDP_VALUE_CAPS *values = NULL;

        values = HeapAlloc(GetProcessHeap(), 0, sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps);
        length = Caps.NumberInputValueCaps;
        status = HidP_GetValueCaps(HidP_Input, values, &length, ppd);
        ok(status == HIDP_STATUS_SUCCESS, "Failed to get value caps (%lx)\n",status);

        trace("\tValues:\n");
        for (i = 0; i < length; i++)
        {
            ok(values[i].ReportCount, "Zero ReportCount for [%i,%i]\n", values[i].UsagePage, values[i].NotRange.Usage);
            if (values[i].IsRange || values[i].ReportCount <= 1)
            {
                status = HidP_GetUsageValue(HidP_Input, values[i].UsagePage, 0,
                    values[i].Range.UsageMin, &value, ppd, data, data_length);
                ok(status == HIDP_STATUS_SUCCESS, "Failed to get value [%i,%i] (%lx)\n",
                    values[i].UsagePage, values[i].Range.UsageMin, status);
                trace("[%02x, %02x]: %lu\n", values[i].UsagePage, values[i].Range.UsageMin, value);
            }
            else
            {
                USHORT k, array_size = (values[i].BitSize * values[i].ReportCount + 7) / 8;
                PCHAR array = HeapAlloc(GetProcessHeap(), 0, array_size);
                char *dump = HeapAlloc(GetProcessHeap(), 0, array_size * 3 + 1);

                status = HidP_GetUsageValueArray(HidP_Input, values[i].UsagePage, 0,
                    values[i].NotRange.Usage, array, array_size, ppd, data, data_length);
                ok(status == HIDP_STATUS_SUCCESS, "Failed to get value array [%i,%i] (%lx)\n",
                    values[i].UsagePage, values[i].NotRange.Usage, status);
                dump[0] = 0;
                for (k = 0; k < array_size; k++)
                {
                    char bytestr[5];
                    sprintf(bytestr, " %02x", (BYTE)array[k]);
                    strcat(dump, bytestr);
                }
                trace("[%02x, %02x] element bit size %u num elements %u:%s\n", values[i].UsagePage,
                    values[i].NotRange.Usage, values[i].BitSize, values[i].ReportCount, dump);

                HeapFree(GetProcessHeap(), 0, dump);
                HeapFree(GetProcessHeap(), 0, array);
            }
        }

        HeapFree(GetProcessHeap(), 0, values);
    }
}

static void test_read_device(void)
{
    PHIDP_PREPARSED_DATA ppd;
    HIDP_CAPS Caps;
    OVERLAPPED overlapped;
    WCHAR device_name[128];
    CHAR *data = NULL;
    DWORD read;
    BOOL rc;
    NTSTATUS status;
    DWORD timeout, tick, spent, max_time;
    char *report;

    USAGE device_usages[] = {HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_GENERIC_GAMEPAD};
    HANDLE device = get_device(HID_USAGE_PAGE_GENERIC, device_usages, 2, GENERIC_READ);

    if (!device)
        device = get_device(0x0, NULL, 0x0, GENERIC_READ);

    if (!device)
    {
        trace("No device found for reading\n");
        return;
    }
    rc = HidD_GetProductString(device, device_name, sizeof(device_name));
    ok(rc, "Failed to get product string(0x%lx)\n", GetLastError());
    trace("Read tests on device :%s\n",wine_dbgstr_w(device_name));

    rc = HidD_GetPreparsedData(device, &ppd);
    ok(rc, "Failed to get preparsed data(0x%lx)\n", GetLastError());
    status = HidP_GetCaps(ppd, &Caps);
    ok(status == HIDP_STATUS_SUCCESS, "Failed to get Caps(0x%lx)\n", status);
    data = HeapAlloc(GetProcessHeap(), 0, Caps.InputReportByteLength);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (winetest_interactive)
    {
        max_time = READ_MAX_TIME;
        timeout = 1000;
    }
    else
        max_time = timeout = 100;
    if (winetest_interactive)
        trace("Test your device for the next %li seconds\n", max_time/1000);
    report = HeapAlloc(GetProcessHeap(), 0, 3 * Caps.InputReportByteLength);
    tick = GetTickCount();
    spent = 0;
    do
    {
        ReadFile(device, data, Caps.InputReportByteLength, NULL, &overlapped);
        if (WaitForSingleObject(overlapped.hEvent, timeout) != WAIT_OBJECT_0)
        {
            ResetEvent(overlapped.hEvent);
            spent = GetTickCount() - tick;
            trace("REMAINING: %ld ms\n", max_time - spent);
            continue;
        }
        ResetEvent(overlapped.hEvent);
        spent = GetTickCount() - tick;
        GetOverlappedResult(device, &overlapped, &read, FALSE);
        if (read)
        {
            int i;

            report[0] = 0;
            for (i = 0; i < read && i < Caps.InputReportByteLength; i++)
            {
                char bytestr[5];
                sprintf(bytestr, "%x ", (BYTE)data[i]);
                strcat(report, bytestr);
            }
            trace("Input report (%li): %s\n", read, report);

            process_data(Caps, ppd, data, read);
        }
        trace("REMAINING: %ld ms\n", max_time - spent);
    } while(spent < max_time);

    CancelIo(device);
    rc = WaitForSingleObject(overlapped.hEvent, timeout);
    ok(rc == WAIT_OBJECT_0, "Wait for object failed\n");

    CloseHandle(overlapped.hEvent);
    rc = HidD_FreePreparsedData(ppd);
    ok(rc, "Failed to free preparsed data(0x%lx)\n", GetLastError());
    CloseHandle(device);
    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, report);
}

static void test_get_input_report(void)
{
    PHIDP_PREPARSED_DATA ppd;
    HIDP_CAPS Caps;
    WCHAR device_name[128];
    CHAR *data = NULL;
    DWORD tick, spent, max_time;
    char *report;
    BOOL rc;
    NTSTATUS status;

    USAGE device_usages[] = {HID_USAGE_GENERIC_JOYSTICK, HID_USAGE_GENERIC_GAMEPAD};
    HANDLE device = get_device(HID_USAGE_PAGE_GENERIC, device_usages, 2, GENERIC_READ);

    if (!device)
        device = get_device(0x0, NULL, 0x0, GENERIC_READ);

    if (!device)
    {
        trace("No device found for testing\n");
        return;
    }
    rc = HidD_GetProductString(device, device_name, sizeof(device_name));
    ok(rc, "Failed to get product string(0x%lx)\n", GetLastError());
    trace("HidD_GetInputRpeort tests on device :%s\n",wine_dbgstr_w(device_name));

    rc = HidD_GetPreparsedData(device, &ppd);
    ok(rc, "Failed to get preparsed data(0x%lx)\n", GetLastError());
    status = HidP_GetCaps(ppd, &Caps);
    ok(status == HIDP_STATUS_SUCCESS, "Failed to get Caps(0x%lx)\n", status);
    data = HeapAlloc(GetProcessHeap(), 0, Caps.InputReportByteLength);

    if (winetest_interactive)
        max_time = READ_MAX_TIME;
    else
        max_time = 100;
    if (winetest_interactive)
        trace("Test your device for the next %li seconds\n", max_time/1000);
    report = HeapAlloc(GetProcessHeap(), 0, 3 * Caps.InputReportByteLength);
    tick = GetTickCount();
    spent = 0;
    do
    {
        int i;

        data[0] = 0; /* Just testing report ID 0 for now, That will catch most devices */
        rc = HidD_GetInputReport(device, data, Caps.InputReportByteLength);
        spent = GetTickCount() - tick;

        if (rc)
        {
            ok(data[0] == 0, "Report ID (0) is not the first byte of the data\n");
            report[0] = 0;
            for (i = 0; i < Caps.InputReportByteLength; i++)
            {
                char bytestr[5];
                sprintf(bytestr, "%x ", (BYTE)data[i]);
                strcat(report, bytestr);
            }
            trace("Input report (%i): %s\n", Caps.InputReportByteLength, report);

            process_data(Caps, ppd, data, Caps.InputReportByteLength);
        }
        else
            trace("Failed to get Input Report, (%x)\n", rc);
        trace("REMAINING: %ld ms\n", max_time - spent);
        Sleep(500);
    } while(spent < max_time);

    rc = HidD_FreePreparsedData(ppd);
    ok(rc, "Failed to free preparsed data(0x%lx)\n", GetLastError());
    CloseHandle(device);
    HeapFree(GetProcessHeap(), 0, data);
    HeapFree(GetProcessHeap(), 0, report);
}

START_TEST(device)
{
    run_for_each_device(test_device_info);
    test_read_device();
    test_get_input_report();
}
