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


#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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


WINE_DEFAULT_DEBUG_CHANNEL(xinput);

#define XINPUT_GAMEPAD_GUIDE 0x0400

struct axis_info
{
    LONG min;
    LONG range;
    USHORT bits;
};

struct hid_platform_private {
    PHIDP_PREPARSED_DATA ppd;
    HIDP_CAPS caps;

    HANDLE device;
    WCHAR *device_path;
    BOOL enabled;

    char *input_report_buf[2];
    char *output_report_buf;

    struct axis_info lx, ly, ltrigger, rx, ry, rtrigger;
};

static DWORD last_check = 0;

static void MarkUsage(struct hid_platform_private *private, WORD usage, LONG min, LONG max, USHORT bits)
{
    struct axis_info info = {min, max-min, bits};

    switch (usage)
    {
        case HID_USAGE_GENERIC_X: private->lx = info; break;
        case HID_USAGE_GENERIC_Y: private->ly = info; break;
        case HID_USAGE_GENERIC_Z: private->ltrigger = info; break;
        case HID_USAGE_GENERIC_RX: private->rx = info; break;
        case HID_USAGE_GENERIC_RY: private->ry = info; break;
        case HID_USAGE_GENERIC_RZ: private->rtrigger = info; break;
    }
}

static BOOL VerifyGamepad(PHIDP_PREPARSED_DATA ppd, XINPUT_CAPABILITIES *xinput_caps, struct hid_platform_private *private)
{
    HIDP_BUTTON_CAPS *button_caps;
    HIDP_VALUE_CAPS *value_caps;
    NTSTATUS status;

    int i;
    int button_count = 0;

    /* Count buttons */
    memset(xinput_caps, 0, sizeof(XINPUT_CAPABILITIES));

    if (!(button_caps = malloc(sizeof(*button_caps) * private->caps.NumberInputButtonCaps))) return FALSE;
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &private->caps.NumberInputButtonCaps, ppd);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetButtonCaps returned %#x\n", status);
    else for (i = 0; i < private->caps.NumberInputButtonCaps; i++)
    {
        if (button_caps[i].UsagePage != HID_USAGE_PAGE_BUTTON)
            continue;
        if (button_caps[i].IsRange)
            button_count = max(button_count, button_caps[i].Range.UsageMax);
        else
            button_count = max(button_count, button_caps[i].NotRange.Usage);
    }
    free(button_caps);
    if (button_count < 11)
        WARN("Too few buttons, continuing anyway\n");
    xinput_caps->Gamepad.wButtons = 0xffff;

    if (!(value_caps = malloc(sizeof(*value_caps) * private->caps.NumberInputValueCaps))) return FALSE;
    status = HidP_GetValueCaps(HidP_Input, value_caps, &private->caps.NumberInputValueCaps, ppd);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetValueCaps returned %#x\n", status);
    else for (i = 0; i < private->caps.NumberInputValueCaps; i++)
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
    free(value_caps);

    if (private->ltrigger.bits)
        xinput_caps->Gamepad.bLeftTrigger = (1u << (sizeof(xinput_caps->Gamepad.bLeftTrigger) + 1)) - 1;
    else
        WARN("Missing axis LeftTrigger\n");
    if (private->rtrigger.bits)
        xinput_caps->Gamepad.bRightTrigger = (1u << (sizeof(xinput_caps->Gamepad.bRightTrigger) + 1)) - 1;
    else
        WARN("Missing axis RightTrigger\n");
    if (private->lx.bits)
        xinput_caps->Gamepad.sThumbLX = (1u << (sizeof(xinput_caps->Gamepad.sThumbLX) + 1)) - 1;
    else
        WARN("Missing axis ThumbLX\n");
    if (private->ly.bits)
        xinput_caps->Gamepad.sThumbLY = (1u << (sizeof(xinput_caps->Gamepad.sThumbLY) + 1)) - 1;
    else
        WARN("Missing axis ThumbLY\n");
    if (private->rx.bits)
        xinput_caps->Gamepad.sThumbRX = (1u << (sizeof(xinput_caps->Gamepad.sThumbRX) + 1)) - 1;
    else
        WARN("Missing axis ThumbRX\n");
    if (private->ry.bits)
        xinput_caps->Gamepad.sThumbRY = (1u << (sizeof(xinput_caps->Gamepad.sThumbRY) + 1)) - 1;
    else
        WARN("Missing axis ThumbRY\n");

    xinput_caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    xinput_caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;

    if (private->caps.NumberOutputValueCaps > 0)
    {
        xinput_caps->Flags |= XINPUT_CAPS_FFB_SUPPORTED;
        xinput_caps->Vibration.wLeftMotorSpeed = 255;
        xinput_caps->Vibration.wRightMotorSpeed = 255;
    }

    return TRUE;
}

static BOOL init_controller(xinput_controller *controller, PHIDP_PREPARSED_DATA ppd, HIDP_CAPS *caps, HANDLE device, WCHAR *device_path)
{
    size_t size;
    struct hid_platform_private *private;

    if (!(private = calloc(1, sizeof(struct hid_platform_private)))) return FALSE;
    private->caps = *caps;
    if (!VerifyGamepad(ppd, &controller->caps, private)) goto failed;

    TRACE("Found gamepad %s\n", debugstr_w(device_path));

    private->ppd = ppd;
    private->device = device;
    if (!(private->input_report_buf[0] = calloc(1, private->caps.InputReportByteLength))) goto failed;
    if (!(private->input_report_buf[1] = calloc(1, private->caps.InputReportByteLength))) goto failed;
    if (!(private->output_report_buf = calloc(1, private->caps.OutputReportByteLength))) goto failed;
    size = (lstrlenW(device_path) + 1) * sizeof(WCHAR);
    if (!(private->device_path = malloc(size))) goto failed;
    memcpy(private->device_path, device_path, size);
    private->enabled = TRUE;

    memset(&controller->state, 0, sizeof(controller->state));
    memset(&controller->vibration, 0, sizeof(controller->vibration));

    controller->platform_private = private;
    return TRUE;

failed:
    free(private->device_path);
    free(private->input_report_buf[0]);
    free(private->input_report_buf[1]);
    free(private->output_report_buf);
    free(private);
    return FALSE;
}

void HID_find_gamepads(xinput_controller *devices)
{
    HDEVINFO device_info_set;
    GUID hid_guid;
    SP_DEVICE_INTERFACE_DATA interface_data;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *data;
    PHIDP_PREPARSED_DATA ppd;
    DWORD detail_size = MAX_PATH * sizeof(WCHAR);
    HANDLE device;
    HIDP_CAPS caps;
    NTSTATUS status;
    DWORD idx;
    int i, open_device_idx;

    idx = GetTickCount();
    if ((idx - last_check) < 2000)
        return;

    EnterCriticalSection(&xinput_crit);

    if ((idx - last_check) < 2000)
    {
        LeaveCriticalSection(&xinput_crit);
        return;
    }
    last_check = idx;

    HidD_GetHidGuid(&hid_guid);

    device_info_set = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (!(data = malloc(sizeof(*data) + detail_size))) goto done;
    data->cbSize = sizeof(*data);

    ZeroMemory(&interface_data, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    idx = 0;
    while (SetupDiEnumDeviceInterfaces(device_info_set, NULL, &hid_guid, idx++,
           &interface_data))
    {
        if (!SetupDiGetDeviceInterfaceDetailW(device_info_set,
                &interface_data, data, sizeof(*data) + detail_size, NULL, NULL))
            continue;

        if (!wcsstr(data->DevicePath, L"IG_"))
            continue;

        open_device_idx = -1;
        for (i = 0; i < XUSER_MAX_COUNT; i++)
        {
            struct hid_platform_private *private = devices[i].platform_private;
            if (devices[i].platform_private)
            {
                if (!wcscmp(data->DevicePath, private->device_path))
                    break;
            }
            else if(open_device_idx < 0)
                open_device_idx = i;
        }
        if (i != XUSER_MAX_COUNT)
            /* this device is already opened */
            continue;
        if (open_device_idx < 0)
            /* no open device slots */
            break;
        device = CreateFileW(data->DevicePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (device == INVALID_HANDLE_VALUE)
            continue;

        ppd = NULL;
        if (!HidD_GetPreparsedData(device, &ppd))
            WARN("ignoring HID device, HidD_GetPreparsedData failed with error %u\n", GetLastError());
        else if ((status = HidP_GetCaps(ppd, &caps)) != HIDP_STATUS_SUCCESS)
            WARN("ignoring HID device, HidP_GetCaps returned %#x\n", status);
        else if (caps.UsagePage != HID_USAGE_PAGE_GENERIC)
            WARN("ignoring HID device, unsupported usage page %04x\n", caps.UsagePage);
        else if (caps.Usage != HID_USAGE_GENERIC_GAMEPAD && caps.Usage != HID_USAGE_GENERIC_JOYSTICK && caps.Usage != HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER)
            WARN("ignoring HID device, unsupported usage %04x:%04x\n", caps.UsagePage, caps.Usage);
        else if (!init_controller(&devices[open_device_idx], ppd, &caps, device, data->DevicePath))
            WARN("ignoring HID device, failed to initialize\n");
        else
            goto done;

        CloseHandle(device);
        HidD_FreePreparsedData(ppd);
    }

done:
    free(data);
    SetupDiDestroyDeviceInfoList(device_info_set);
    LeaveCriticalSection(&xinput_crit);
}

static void remove_gamepad(xinput_controller *device)
{
    EnterCriticalSection(&device->crit);

    if (device->platform_private)
    {
        struct hid_platform_private *private = device->platform_private;

        device->platform_private = NULL;

        CloseHandle(private->device);
        free(private->input_report_buf[0]);
        free(private->input_report_buf[1]);
        free(private->output_report_buf);
        free(private->device_path);
        HidD_FreePreparsedData(private->ppd);
        free(private);
    }

    LeaveCriticalSection(&device->crit);
}

void HID_destroy_gamepads(xinput_controller *devices)
{
    int i;
    for (i = 0; i < XUSER_MAX_COUNT; i++)
        remove_gamepad(&devices[i]);
}

static SHORT scale_short(LONG value, const struct axis_info *axis)
{
    return ((((ULONGLONG)(value - axis->min)) * 0xffff) / axis->range) - 32768;
}

static BYTE scale_byte(LONG value, const struct axis_info *axis)
{
    return (((ULONGLONG)(value - axis->min)) * 0xff) / axis->range;
}

void HID_update_state(xinput_controller *device, XINPUT_STATE *state)
{
    struct hid_platform_private *private = device->platform_private;
    int i;
    char **report_buf = private->input_report_buf, *tmp;
    ULONG report_len = private->caps.InputReportByteLength;
    NTSTATUS status;

    USAGE buttons[11];
    ULONG button_length, hat_value;
    LONG value;

    if (!private->enabled)
        return;

    if (!HidD_GetInputReport(private->device, report_buf[0], report_len))
    {
        if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_INVALID_HANDLE)
        {
            EnterCriticalSection(&xinput_crit);
            remove_gamepad(device);
            LeaveCriticalSection(&xinput_crit);
        }
        else ERR("Failed to get input report, HidD_GetInputReport failed with error %u\n", GetLastError());
        return;
    }

    if (memcmp(report_buf[0], report_buf[1], report_len) != 0)
    {
        device->state.dwPacketNumber++;
        button_length = ARRAY_SIZE(buttons);
        status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, buttons, &button_length, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsages HID_USAGE_PAGE_BUTTON returned %#x\n", status);

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
                case 7: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK; break;
                case 8: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_START; break;
                case 9: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB; break;
                case 10: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB; break;
                case 11: device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE; break;
            }
        }

        status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, &hat_value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_HATSWITCH returned %#x\n", status);
        else
        {
            switch(hat_value){
                /* 8 1 2
                 * 7 0 3
                 * 6 5 4 */
                case 0:
                    break;
                case 1:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
                    break;
                case 2:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
                    break;
                case 3:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
                    break;
                case 4:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN;
                    break;
                case 5:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
                    break;
                case 6:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
                    break;
                case 7:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
                    break;
                case 8:
                    device->state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP;
                    break;
            }
        }

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_X returned %#x\n", status);
        else device->state.Gamepad.sThumbLX = scale_short(value, &private->lx);

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_Y returned %#x\n", status);
        else device->state.Gamepad.sThumbLY = -scale_short(value, &private->ly) - 1;

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RX returned %#x\n", status);
        else device->state.Gamepad.sThumbRX = scale_short(value, &private->rx);

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RY returned %#x\n", status);
        else device->state.Gamepad.sThumbRY = -scale_short(value, &private->ry) - 1;

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RZ, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RZ returned %#x\n", status);
        else device->state.Gamepad.bRightTrigger = scale_byte(value, &private->rtrigger);

        status = HidP_GetScaledUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value, private->ppd, report_buf[0], report_len);
        if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetScaledUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_Z returned %#x\n", status);
        else device->state.Gamepad.bLeftTrigger = scale_byte(value, &private->ltrigger);
    }

    tmp = report_buf[0];
    report_buf[0] = report_buf[1];
    report_buf[1] = tmp;
    memcpy(state, &device->state, sizeof(*state));
}

DWORD HID_set_state(xinput_controller* device, XINPUT_VIBRATION* state)
{
    struct hid_platform_private *private = device->platform_private;
    char *output_report_buf = private->output_report_buf;
    ULONG output_report_len = private->caps.OutputReportByteLength;

    if (device->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
    {
        device->vibration.wLeftMotorSpeed = state->wLeftMotorSpeed;
        device->vibration.wRightMotorSpeed = state->wRightMotorSpeed;

        if (private->enabled)
        {
            memset(output_report_buf, 0, output_report_len);
            output_report_buf[0] = /* report id */ 0;
            output_report_buf[1] = 0x8;
            output_report_buf[3] = (BYTE)(state->wLeftMotorSpeed / 256);
            output_report_buf[4] = (BYTE)(state->wRightMotorSpeed / 256);

            if (!HidD_SetOutputReport(private->device, output_report_buf, output_report_len))
            {
                WARN("unable to set output report, HidD_SetOutputReport failed with error %u\n", GetLastError());
                return GetLastError();
            }

            return ERROR_SUCCESS;
        }
    }

    return ERROR_SUCCESS;
}

void HID_enable(xinput_controller* device, BOOL enable)
{
    struct hid_platform_private *private = device->platform_private;

    if (device->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
    {
        if (private->enabled && !enable)
        {
            XINPUT_VIBRATION state;
            state.wLeftMotorSpeed = 0;
            state.wRightMotorSpeed = 0;
            HID_set_state(device, &state);
        }
        else if (!private->enabled && enable)
        {
            HID_set_state(device, &device->vibration);
        }
    }

    private->enabled = enable;
}
