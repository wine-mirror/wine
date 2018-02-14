/*
 * The Wine project - Xinput Joystick HID interface
 * Copyright 2018 Aric Stewart
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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "wingdi.h"
#include "winnls.h"
#include "winternl.h"

#include "setupapi.h"
#include "devpkey.h"
#include "hidusage.h"
#include "ddk/hidsdi.h"
#include "initguid.h"
#include "devguid.h"

#include "xinput.h"
#include "xinput_private.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

#define XINPUT_GAMEPAD_GUIDE 0x0400

static CRITICAL_SECTION hid_xinput_crit;
static CRITICAL_SECTION_DEBUG hid_critsect_debug =
{
    0, 0, &hid_xinput_crit,
    { &hid_critsect_debug.ProcessLocksList, &hid_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": hid_xinput_crit") }
};
static CRITICAL_SECTION hid_xinput_crit = { &hid_critsect_debug, -1, 0, 0, 0, 0 };

struct hid_platform_private {
    PHIDP_PREPARSED_DATA ppd;
    HANDLE device;
    WCHAR *device_path;
    BOOL enabled;

    CRITICAL_SECTION crit;

    DWORD report_length;
    BYTE current_report;
    CHAR *reports[2];

    LONG ThumbLXRange[3];
    LONG ThumbLYRange[3];
    LONG LeftTriggerRange[3];
    LONG ThumbRXRange[3];
    LONG ThumbRYRange[3];
    LONG RightTriggerRange[3];
};

static DWORD last_check = 0;

static void MarkUsage(struct hid_platform_private *private, WORD usage, LONG min, LONG max, USHORT bits)
{
    switch (usage)
    {
        case HID_USAGE_GENERIC_X:
            private->ThumbLXRange[0] = min;
            private->ThumbLXRange[1] = bits;
            private->ThumbLXRange[2] = max - min;
        break;
        case HID_USAGE_GENERIC_Y:
            private->ThumbLYRange[0] = min;
            private->ThumbLYRange[1] = bits;
            private->ThumbLYRange[2] = max - min;
        break;
        case HID_USAGE_GENERIC_Z:
            private->LeftTriggerRange[0] = min;
            private->LeftTriggerRange[1] = bits;
            private->LeftTriggerRange[2] = max - min;
        break;
        case HID_USAGE_GENERIC_RX:
            private->ThumbRXRange[0] = min;
            private->ThumbRXRange[1] = bits;
            private->ThumbRXRange[2] = max - min;
        break;
        case HID_USAGE_GENERIC_RY:
            private->ThumbRYRange[0] = min;
            private->ThumbRYRange[1] = bits;
            private->ThumbRYRange[2] = max - min;
        break;
        case HID_USAGE_GENERIC_RZ:
            private->RightTriggerRange[0] = min;
            private->RightTriggerRange[1] = bits;
            private->RightTriggerRange[2] = max - min;
        break;
    }
}

static BOOL VerifyGamepad(PHIDP_PREPARSED_DATA ppd, XINPUT_CAPABILITIES *xinput_caps, struct hid_platform_private *private, HIDP_CAPS *caps)
{
    HIDP_BUTTON_CAPS *button_caps;
    HIDP_VALUE_CAPS *value_caps;

    int i;
    int button_count = 0;
    USHORT button_caps_count = 0;
    USHORT value_caps_count = 0;

    /* Count buttons */
    memset(xinput_caps, 0, sizeof(XINPUT_CAPABILITIES));

    button_caps_count = caps->NumberInputButtonCaps;
    button_caps = HeapAlloc(GetProcessHeap(), 0, sizeof(*button_caps) * button_caps_count);
    HidP_GetButtonCaps(HidP_Input, button_caps, &button_caps_count, ppd);
    for (i = 0; i < button_caps_count; i++)
    {
        if (button_caps[i].UsagePage != HID_USAGE_PAGE_BUTTON)
            continue;
        if (button_caps[i].IsRange)
            button_count = max(button_count, button_caps[i].Range.UsageMax);
        else
            button_count = max(button_count, button_caps[i].NotRange.Usage);
    }
    HeapFree(GetProcessHeap(), 0, button_caps);
    if (button_count < 14)
        WARN("Too few buttons, Continue\n");
    xinput_caps->Gamepad.wButtons = 0xffff;

    value_caps_count = caps->NumberInputValueCaps;
    value_caps = HeapAlloc(GetProcessHeap(), 0, sizeof(*value_caps) * value_caps_count);
    HidP_GetValueCaps(HidP_Input, value_caps, &value_caps_count, ppd);
    for (i = 0; i < value_caps_count; i++)
    {
        if (value_caps[i].UsagePage != HID_USAGE_PAGE_GENERIC)
            continue;
        if (value_caps[i].IsRange)
        {
            int u;
            for (u = value_caps[i].Range.UsageMin; u <=value_caps[i].Range.UsageMax; u++)
                MarkUsage(private, u,  value_caps[i].LogicalMin, value_caps[i].LogicalMax, value_caps[i].BitSize);
        }
        else
            MarkUsage(private, value_caps[i].NotRange.Usage, value_caps[i].LogicalMin, value_caps[i].LogicalMax, value_caps[i].BitSize);
    }
    HeapFree(GetProcessHeap(), 0, value_caps);

    if (private->LeftTriggerRange[1])
        xinput_caps->Gamepad.bLeftTrigger = (1u << (sizeof(xinput_caps->Gamepad.bLeftTrigger) + 1)) - 1;
    else
        WARN("Missing axis LeftTrigger\n");
    if (private->RightTriggerRange[1])
        xinput_caps->Gamepad.bRightTrigger = (1u << (sizeof(xinput_caps->Gamepad.bRightTrigger) + 1)) - 1;
    else
        WARN("Missing axis RightTrigger\n");
    if (private->ThumbLXRange[1])
        xinput_caps->Gamepad.sThumbLX = (1u << (sizeof(xinput_caps->Gamepad.sThumbLX) + 1)) - 1;
    else
        WARN("Missing axis ThumbLX\n");
    if (private->ThumbLYRange[1])
        xinput_caps->Gamepad.sThumbLY = (1u << (sizeof(xinput_caps->Gamepad.sThumbLY) + 1)) - 1;
    else
        WARN("Missing axis ThumbLY\n");
    if (private->ThumbRXRange[1])
        xinput_caps->Gamepad.sThumbRX = (1u << (sizeof(xinput_caps->Gamepad.sThumbRX) + 1)) - 1;
    else
        WARN("Missing axis ThumbRX\n");
    if (private->ThumbRYRange[1])
        xinput_caps->Gamepad.sThumbRY = (1u << (sizeof(xinput_caps->Gamepad.sThumbRY) + 1)) - 1;
    else
        WARN("Missing axis ThumbRY\n");

    xinput_caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    xinput_caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;

    value_caps_count = caps->NumberOutputValueCaps;
    if (value_caps_count > 0)
        xinput_caps->Flags |= XINPUT_CAPS_FFB_SUPPORTED;

    return TRUE;
}

static void build_private(struct hid_platform_private *private, PHIDP_PREPARSED_DATA ppd, HIDP_CAPS *caps, HANDLE device, WCHAR *path)
{
    size_t size;
    private->ppd = ppd;
    private->device = device;
    private->report_length = caps->InputReportByteLength + 1;
    private->current_report = 0;
    private->reports[0] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, private->report_length);
    private->reports[1] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, private->report_length);
    size = (strlenW(path) + 1) * sizeof(WCHAR);
    private->device_path = HeapAlloc(GetProcessHeap(), 0, size);
    memcpy(private->device_path, path, size);
    private->enabled = TRUE;

    InitializeCriticalSection(&private->crit);
    private->crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JoystickImpl*->generic.base.crit");
}

void HID_find_gamepads(xinput_controller *devices)
{
    HDEVINFO device_info_set;
    GUID hid_guid;
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *data;
    PHIDP_PREPARSED_DATA ppd;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    HANDLE device = INVALID_HANDLE_VALUE;
    HIDP_CAPS Caps;
    DWORD idx,didx;
    int i;

    idx = GetTickCount();
    if ((idx - last_check) < 2000)
        return;
    last_check = idx;

    HidD_GetHidGuid(&hid_guid);

    EnterCriticalSection(&hid_xinput_crit);

    device_info_set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE);

    data = HeapAlloc(GetProcessHeap(), 0 , sizeof(*data) + detail_size);
    data->cbSize = sizeof(*data);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    idx = didx = 0;
    while (SetupDiEnumDeviceInterfaces(device_info_set, NULL, &hid_guid, idx++,
           &interface_data) && didx < XUSER_MAX_COUNT)
    {
        static const WCHAR ig[] = {'I','G','_',0};
        if (!SetupDiGetDeviceInterfaceDetailW(device_info_set,
                &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
            continue;

        if (!strstrW(data->DevicePath, ig))
            continue;

        for (i = 0; i < XUSER_MAX_COUNT; i++)
        {
            struct hid_platform_private *private = devices[i].platform_private;
            if (devices[i].connected && !strcmpW(data->DevicePath, private->device_path))
                break;
        }
        if (i != XUSER_MAX_COUNT)
            continue;
        device = CreateFileW(data->DevicePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (device == INVALID_HANDLE_VALUE)
            continue;

        HidD_GetPreparsedData(device, &ppd);
        HidP_GetCaps(ppd, &Caps);
        if (Caps.UsagePage == HID_USAGE_PAGE_GENERIC &&
            (Caps.Usage == HID_USAGE_GENERIC_GAMEPAD ||
             Caps.Usage == HID_USAGE_GENERIC_JOYSTICK ||
             Caps.Usage == 0x8 /* Multi-axis Controller */))
        {
            struct hid_platform_private *private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct hid_platform_private));
            if (VerifyGamepad(ppd, &(devices[didx].caps), private, &Caps))
            {
                TRACE("Found gamepad %i %s\n",didx, debugstr_w(data->DevicePath));
                devices[didx].connected = TRUE;
                build_private(private, ppd, &Caps, device, data->DevicePath);
                devices[didx].platform_private = private;
                didx++;
            }
            else
            {
                CloseHandle(device);
                HidD_FreePreparsedData(ppd);
                HeapFree(GetProcessHeap(), 0, private);
            }
        }
        else
        {
            CloseHandle(device);
            HidD_FreePreparsedData(ppd);
        }
        device = INVALID_HANDLE_VALUE;
    }
    HeapFree(GetProcessHeap(), 0, data);
    SetupDiDestroyDeviceInfoList(device_info_set);
    LeaveCriticalSection(&hid_xinput_crit);
    return;
}

static void remove_gamepad(xinput_controller *device)
{
    if (device->connected)
    {
        struct hid_platform_private *private = device->platform_private;

        EnterCriticalSection(&private->crit);
        CloseHandle(private->device);
        HeapFree(GetProcessHeap(), 0, private->reports[0]);
        HeapFree(GetProcessHeap(), 0, private->reports[1]);
        HeapFree(GetProcessHeap(), 0, private->device_path);
        HidD_FreePreparsedData(private->ppd);
        device->platform_private = NULL;
        device->connected = FALSE;
        LeaveCriticalSection(&private->crit);
        DeleteCriticalSection(&private->crit);
        HeapFree(GetProcessHeap(), 0, private);
    }
}

void HID_destroy_gamepads(xinput_controller *devices)
{
    int i;
    EnterCriticalSection(&hid_xinput_crit);
    for (i = 0; i < XUSER_MAX_COUNT; i++)
        remove_gamepad(&devices[i]);
    LeaveCriticalSection(&hid_xinput_crit);
}

#define SIGN(v,b) ((b==8)?(BYTE)v:(b==16)?(SHORT)v:(INT)v)
#define SCALE_SHORT(v,r) (SHORT)((((0xffff)*(SIGN(v,r[1]) - r[0]))/r[2])-32767)
#define SCALE_BYTE(v,r) (BYTE)((((0xff)*(SIGN(v,r[1]) - r[0]))/r[2]))

void HID_update_state(xinput_controller* device)
{
    struct hid_platform_private *private = device->platform_private;
    int i;
    CHAR *report = private->reports[(private->current_report)%2];
    CHAR *target_report = private->reports[(private->current_report+1)%2];

    USAGE buttons[15];
    ULONG button_length;
    ULONG value;

    if (!private->enabled)
        return;

    EnterCriticalSection(&private->crit);
    if (!HidD_GetInputReport(private->device, target_report, private->report_length))
    {
        if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_INVALID_HANDLE)
            remove_gamepad(device);
        else
            ERR("Failed to get Input Report (%x)\n", GetLastError());
        LeaveCriticalSection(&private->crit);
        return;
    }
    if (memcmp(report, target_report, private->report_length) == 0)
    {
        LeaveCriticalSection(&private->crit);
        return;
    }

    private->current_report = (private->current_report+1)%2;

    device->state.dwPacketNumber++;
    button_length = 15;
    HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, buttons, &button_length, private->ppd, target_report, private->report_length);

    device->state.Gamepad.wButtons = 0;
    for (i = 0; i < button_length; i++)
    {
        switch (buttons[i])
        {
            case 1: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_A; break;
            case 2: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_B; break;
            case 3: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_X; break;
            case 4: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_Y; break;
            case 5: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER; break;
            case 6: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
            case 7: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB; break;
            case 8: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB; break;

            case 9: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_START; break;
            case 10: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK; break;
            case 11: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE; break;
            case 12: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP; break;
            case 13: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN; break;
            case 14: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT; break;
            case 15: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; break;
        }
    }

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.sThumbLX = SCALE_SHORT(value, private->ThumbLXRange);

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.sThumbLY = -SCALE_SHORT(value, private->ThumbLYRange);

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.sThumbRX = SCALE_SHORT(value, private->ThumbRXRange);

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.sThumbRY = -SCALE_SHORT(value, private->ThumbRYRange);

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RZ, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.bRightTrigger = SCALE_BYTE(value, private->RightTriggerRange);

    HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value, private->ppd, target_report, private->report_length);
    device->state.Gamepad.bLeftTrigger = SCALE_BYTE(value, private->LeftTriggerRange);
    LeaveCriticalSection(&private->crit);
}

DWORD HID_set_state(xinput_controller* device, XINPUT_VIBRATION* state)
{
    struct hid_platform_private *private = device->platform_private;

    struct {
        BYTE report;
        BYTE pad1[2];
        BYTE left;
        BYTE right;
        BYTE pad2[3];
    } report;

    if (device->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
    {
        device->caps.Vibration.wLeftMotorSpeed = state->wLeftMotorSpeed;
        device->caps.Vibration.wRightMotorSpeed = state->wRightMotorSpeed;

        if (private->enabled)
        {
            BOOLEAN rc;

            report.report = 0;
            report.pad1[0] = 0x8;
            report.pad1[1] = 0x0;
            report.left = (BYTE)(state->wLeftMotorSpeed / 255);
            report.right = (BYTE)(state->wRightMotorSpeed / 255);
            memset(&report.pad2, 0, sizeof(report.pad2));

            EnterCriticalSection(&private->crit);
            rc = HidD_SetOutputReport(private->device, &report, sizeof(report));
            LeaveCriticalSection(&private->crit);
            if (rc)
                return ERROR_SUCCESS;
            return GetLastError();
        }
        return ERROR_SUCCESS;
    }

    return ERROR_NOT_SUPPORTED;
}

void HID_enable(xinput_controller* device, BOOL enable)
{
    struct hid_platform_private *private = device->platform_private;

    if (device->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
    {
        EnterCriticalSection(&private->crit);
        if (private->enabled && !enable)
        {
            XINPUT_VIBRATION state;
            state.wLeftMotorSpeed = 0;
            state.wRightMotorSpeed = 0;
            HID_set_state(device, &state);
        }
        else if (!private->enabled && enable)
        {
            HID_set_state(device, &device->caps.Vibration);
        }
        LeaveCriticalSection(&private->crit);
    }

    private->enabled = enable;
}
