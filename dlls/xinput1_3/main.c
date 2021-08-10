/*
 * The Wine project - Xinput Joystick Library
 * Copyright 2008 Andrew Fenn
 * Copyright 2018 Aric Stewart
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
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

#include "wine/debug.h"

/* Not defined in the headers, used only by XInputGetStateEx */
#define XINPUT_GAMEPAD_GUIDE 0x0400

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

struct xinput_controller
{
    CRITICAL_SECTION crit;
    XINPUT_CAPABILITIES caps;
    XINPUT_STATE state;
    XINPUT_GAMEPAD last_keystroke;
    XINPUT_VIBRATION vibration;
    HANDLE device;
    WCHAR device_path[MAX_PATH];
    BOOL enabled;

    struct
    {
        PHIDP_PREPARSED_DATA preparsed;
        HIDP_CAPS caps;
        HIDP_VALUE_CAPS lx_caps;
        HIDP_VALUE_CAPS ly_caps;
        HIDP_VALUE_CAPS lt_caps;
        HIDP_VALUE_CAPS rx_caps;
        HIDP_VALUE_CAPS ry_caps;
        HIDP_VALUE_CAPS rt_caps;

        HANDLE read_event;
        OVERLAPPED read_ovl;

        char *input_report_buf;
        char *output_report_buf;
    } hid;
};

/* xinput_crit guards controllers array */
static CRITICAL_SECTION xinput_crit;
static CRITICAL_SECTION_DEBUG xinput_critsect_debug =
{
    0, 0, &xinput_crit,
    { &xinput_critsect_debug.ProcessLocksList, &xinput_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": xinput_crit") }
};
static CRITICAL_SECTION xinput_crit = { &xinput_critsect_debug, -1, 0, 0, 0, 0 };

static struct xinput_controller controllers[XUSER_MAX_COUNT];
static CRITICAL_SECTION_DEBUG controller_critsect_debug[XUSER_MAX_COUNT] =
{
    {
        0, 0, &controllers[0].crit,
        { &controller_critsect_debug[0].ProcessLocksList, &controller_critsect_debug[0].ProcessLocksList },
          0, 0, { (DWORD_PTR)(__FILE__ ": controllers[0].crit") }
    },
    {
        0, 0, &controllers[1].crit,
        { &controller_critsect_debug[1].ProcessLocksList, &controller_critsect_debug[1].ProcessLocksList },
          0, 0, { (DWORD_PTR)(__FILE__ ": controllers[1].crit") }
    },
    {
        0, 0, &controllers[2].crit,
        { &controller_critsect_debug[2].ProcessLocksList, &controller_critsect_debug[2].ProcessLocksList },
          0, 0, { (DWORD_PTR)(__FILE__ ": controllers[2].crit") }
    },
    {
        0, 0, &controllers[3].crit,
        { &controller_critsect_debug[3].ProcessLocksList, &controller_critsect_debug[3].ProcessLocksList },
          0, 0, { (DWORD_PTR)(__FILE__ ": controllers[3].crit") }
    },
};

static struct xinput_controller controllers[XUSER_MAX_COUNT] =
{
    {{ &controller_critsect_debug[0], -1, 0, 0, 0, 0 }},
    {{ &controller_critsect_debug[1], -1, 0, 0, 0, 0 }},
    {{ &controller_critsect_debug[2], -1, 0, 0, 0, 0 }},
    {{ &controller_critsect_debug[3], -1, 0, 0, 0, 0 }},
};

static HANDLE stop_event;
static HANDLE done_event;
static HANDLE update_event;

static BOOL find_opened_device(SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail, int *free_slot)
{
    int i;

    *free_slot = XUSER_MAX_COUNT;
    for (i = XUSER_MAX_COUNT; i > 0; i--)
    {
        if (!controllers[i - 1].device) *free_slot = i - 1;
        else if (!wcscmp(detail->DevicePath, controllers[i - 1].device_path)) return TRUE;
    }
    return FALSE;
}

static void check_value_caps(struct xinput_controller *controller, USHORT usage, HIDP_VALUE_CAPS *caps)
{
    switch (usage)
    {
    case HID_USAGE_GENERIC_X: controller->hid.lx_caps = *caps; break;
    case HID_USAGE_GENERIC_Y: controller->hid.ly_caps = *caps; break;
    case HID_USAGE_GENERIC_Z: controller->hid.lt_caps = *caps; break;
    case HID_USAGE_GENERIC_RX: controller->hid.rx_caps = *caps; break;
    case HID_USAGE_GENERIC_RY: controller->hid.ry_caps = *caps; break;
    case HID_USAGE_GENERIC_RZ: controller->hid.rt_caps = *caps; break;
    }
}

static BOOL controller_check_caps(struct xinput_controller *controller, PHIDP_PREPARSED_DATA preparsed)
{
    XINPUT_CAPABILITIES *caps = &controller->caps;
    HIDP_BUTTON_CAPS *button_caps;
    HIDP_VALUE_CAPS *value_caps;
    NTSTATUS status;
    int i, u;
    int button_count = 0;

    /* Count buttons */
    memset(caps, 0, sizeof(XINPUT_CAPABILITIES));

    if (!(button_caps = malloc(sizeof(*button_caps) * controller->hid.caps.NumberInputButtonCaps))) return FALSE;
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &controller->hid.caps.NumberInputButtonCaps, preparsed);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetButtonCaps returned %#x\n", status);
    else for (i = 0; i < controller->hid.caps.NumberInputButtonCaps; i++)
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
    caps->Gamepad.wButtons = 0xffff;

    if (!(value_caps = malloc(sizeof(*value_caps) * controller->hid.caps.NumberInputValueCaps))) return FALSE;
    status = HidP_GetValueCaps(HidP_Input, value_caps, &controller->hid.caps.NumberInputValueCaps, preparsed);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetValueCaps returned %#x\n", status);
    else for (i = 0; i < controller->hid.caps.NumberInputValueCaps; i++)
    {
        HIDP_VALUE_CAPS *caps = value_caps + i;
        if (caps->UsagePage != HID_USAGE_PAGE_GENERIC) continue;
        if (!caps->IsRange) check_value_caps(controller, caps->NotRange.Usage, caps);
        else for (u = caps->Range.UsageMin; u <=caps->Range.UsageMax; u++) check_value_caps(controller, u, value_caps + i);
    }
    free(value_caps);

    if (!controller->hid.lt_caps.UsagePage) WARN("Missing axis LeftTrigger\n");
    else caps->Gamepad.bLeftTrigger = (1u << (sizeof(caps->Gamepad.bLeftTrigger) + 1)) - 1;
    if (!controller->hid.rt_caps.UsagePage) WARN("Missing axis RightTrigger\n");
    else caps->Gamepad.bRightTrigger = (1u << (sizeof(caps->Gamepad.bRightTrigger) + 1)) - 1;
    if (!controller->hid.lx_caps.UsagePage) WARN("Missing axis ThumbLX\n");
    else caps->Gamepad.sThumbLX = (1u << (sizeof(caps->Gamepad.sThumbLX) + 1)) - 1;
    if (!controller->hid.ly_caps.UsagePage) WARN("Missing axis ThumbLY\n");
    else caps->Gamepad.sThumbLY = (1u << (sizeof(caps->Gamepad.sThumbLY) + 1)) - 1;
    if (!controller->hid.rx_caps.UsagePage) WARN("Missing axis ThumbRX\n");
    else caps->Gamepad.sThumbRX = (1u << (sizeof(caps->Gamepad.sThumbRX) + 1)) - 1;
    if (!controller->hid.ry_caps.UsagePage) WARN("Missing axis ThumbRY\n");
    else caps->Gamepad.sThumbRY = (1u << (sizeof(caps->Gamepad.sThumbRY) + 1)) - 1;

    caps->Type = XINPUT_DEVTYPE_GAMEPAD;
    caps->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;

    if (controller->hid.caps.NumberOutputValueCaps > 0)
    {
        caps->Flags |= XINPUT_CAPS_FFB_SUPPORTED;
        caps->Vibration.wLeftMotorSpeed = 255;
        caps->Vibration.wRightMotorSpeed = 255;
    }

    return TRUE;
}

static DWORD HID_set_state(struct xinput_controller *controller, XINPUT_VIBRATION *state)
{
    char *output_report_buf = controller->hid.output_report_buf;
    ULONG output_report_len = controller->hid.caps.OutputReportByteLength;

    if (controller->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED)
    {
        controller->vibration.wLeftMotorSpeed = state->wLeftMotorSpeed;
        controller->vibration.wRightMotorSpeed = state->wRightMotorSpeed;

        if (controller->enabled)
        {
            memset(output_report_buf, 0, output_report_len);
            output_report_buf[0] = /* report id */ 0;
            output_report_buf[1] = 0x8;
            output_report_buf[3] = (BYTE)(state->wLeftMotorSpeed / 256);
            output_report_buf[4] = (BYTE)(state->wRightMotorSpeed / 256);

            if (!HidD_SetOutputReport(controller->device, output_report_buf, output_report_len))
            {
                WARN("unable to set output report, HidD_SetOutputReport failed with error %u\n", GetLastError());
                return GetLastError();
            }

            return ERROR_SUCCESS;
        }
    }

    return ERROR_SUCCESS;
}

static void controller_enable(struct xinput_controller *controller)
{
    XINPUT_VIBRATION state = controller->vibration;

    if (controller->enabled) return;
    if (controller->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED) HID_set_state(controller, &state);
    controller->enabled = TRUE;

    memset(&controller->hid.read_ovl, 0, sizeof(controller->hid.read_ovl));
    controller->hid.read_ovl.hEvent = controller->hid.read_event;
    ReadFile(controller->device, controller->hid.input_report_buf, controller->hid.caps.InputReportByteLength, NULL, &controller->hid.read_ovl);
    SetEvent(update_event);
}

static void controller_disable(struct xinput_controller *controller)
{
    XINPUT_VIBRATION state = {0};

    if (!controller->enabled) return;
    if (controller->caps.Flags & XINPUT_CAPS_FFB_SUPPORTED) HID_set_state(controller, &state);
    controller->enabled = FALSE;

    CancelIoEx(controller->device, &controller->hid.read_ovl);
    SetEvent(update_event);
}

static BOOL controller_init(struct xinput_controller *controller, PHIDP_PREPARSED_DATA preparsed,
                            HIDP_CAPS *caps, HANDLE device, WCHAR *device_path)
{
    HANDLE event = NULL;

    controller->hid.caps = *caps;
    if (!controller_check_caps(controller, preparsed)) goto failed;
    if (!(event = CreateEventA(NULL, FALSE, FALSE, NULL))) goto failed;

    TRACE("Found gamepad %s\n", debugstr_w(device_path));

    controller->hid.preparsed = preparsed;
    controller->hid.read_event = event;
    if (!(controller->hid.input_report_buf = calloc(1, controller->hid.caps.InputReportByteLength))) goto failed;
    if (!(controller->hid.output_report_buf = calloc(1, controller->hid.caps.OutputReportByteLength))) goto failed;

    memset(&controller->state, 0, sizeof(controller->state));
    memset(&controller->vibration, 0, sizeof(controller->vibration));
    lstrcpynW(controller->device_path, device_path, MAX_PATH);
    controller->enabled = FALSE;

    EnterCriticalSection(&controller->crit);
    controller->device = device;
    controller_enable(controller);
    LeaveCriticalSection(&controller->crit);
    return TRUE;

failed:
    free(controller->hid.input_report_buf);
    free(controller->hid.output_report_buf);
    memset(&controller->hid, 0, sizeof(controller->hid));
    CloseHandle(event);
    return FALSE;
}

static void update_controller_list(void)
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    PHIDP_PREPARSED_DATA preparsed;
    HIDP_CAPS caps;
    NTSTATUS status;
    HDEVINFO set;
    HANDLE device;
    DWORD idx;
    GUID guid;
    int i;

    HidD_GetHidGuid(&guid);

    set = SetupDiGetClassDevsW(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    detail->cbSize = sizeof(*detail);

    idx = 0;
    while (SetupDiEnumDeviceInterfaces(set, NULL, &guid, idx++, &iface))
    {
        if (!SetupDiGetDeviceInterfaceDetailW(set, &iface, detail, sizeof(buffer), NULL, NULL))
            continue;

        if (!wcsstr(detail->DevicePath, L"IG_")) continue;

        if (find_opened_device(detail, &i)) continue; /* already opened */
        if (i == XUSER_MAX_COUNT) break; /* no more slots */

        device = CreateFileW(detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
        if (device == INVALID_HANDLE_VALUE) continue;

        preparsed = NULL;
        if (!HidD_GetPreparsedData(device, &preparsed))
            WARN("ignoring HID device, HidD_GetPreparsedData failed with error %u\n", GetLastError());
        else if ((status = HidP_GetCaps(preparsed, &caps)) != HIDP_STATUS_SUCCESS)
            WARN("ignoring HID device, HidP_GetCaps returned %#x\n", status);
        else if (caps.UsagePage != HID_USAGE_PAGE_GENERIC)
            WARN("ignoring HID device, unsupported usage page %04x\n", caps.UsagePage);
        else if (caps.Usage != HID_USAGE_GENERIC_GAMEPAD && caps.Usage != HID_USAGE_GENERIC_JOYSTICK &&
                 caps.Usage != HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER)
            WARN("ignoring HID device, unsupported usage %04x:%04x\n", caps.UsagePage, caps.Usage);
        else if (!controller_init(&controllers[i], preparsed, &caps, device, detail->DevicePath))
            WARN("ignoring HID device, failed to initialize\n");
        else
            continue;

        CloseHandle(device);
        HidD_FreePreparsedData(preparsed);
    }

    SetupDiDestroyDeviceInfoList(set);
}

static void controller_destroy(struct xinput_controller *controller)
{
    EnterCriticalSection(&controller->crit);

    if (controller->device)
    {
        controller_disable(controller);
        CloseHandle(controller->device);
        controller->device = NULL;

        free(controller->hid.input_report_buf);
        free(controller->hid.output_report_buf);
        HidD_FreePreparsedData(controller->hid.preparsed);
        memset(&controller->hid, 0, sizeof(controller->hid));
    }

    LeaveCriticalSection(&controller->crit);
}

static void stop_update_thread(void)
{
    int i;

    SetEvent(stop_event);
    WaitForSingleObject(done_event, INFINITE);

    CloseHandle(stop_event);
    CloseHandle(done_event);
    CloseHandle(update_event);

    for (i = 0; i < XUSER_MAX_COUNT; i++) controller_destroy(&controllers[i]);
}

static LONG sign_extend(ULONG value, const HIDP_VALUE_CAPS *caps)
{
    UINT sign = 1 << (caps->BitSize - 1);
    if (sign <= 1 || caps->LogicalMin >= 0) return value;
    return value - ((value & sign) << 1);
}

static LONG scale_value(ULONG value, const HIDP_VALUE_CAPS *caps, LONG min, LONG max)
{
    LONG tmp = sign_extend(value, caps);
    if (caps->LogicalMin > caps->LogicalMax) return 0;
    if (caps->LogicalMin > tmp || caps->LogicalMax < tmp) return 0;
    return min + MulDiv(tmp - caps->LogicalMin, max - min, caps->LogicalMax - caps->LogicalMin);
}

static void read_controller_state(struct xinput_controller *controller)
{
    ULONG read_len, report_len = controller->hid.caps.InputReportByteLength;
    char *report_buf = controller->hid.input_report_buf;
    XINPUT_STATE state;
    NTSTATUS status;
    USAGE buttons[11];
    ULONG i, button_length, value;

    if (!GetOverlappedResult(controller->device, &controller->hid.read_ovl, &read_len, TRUE))
    {
        if (GetLastError() == ERROR_OPERATION_ABORTED) return;
        if (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_INVALID_HANDLE) controller_destroy(controller);
        else ERR("Failed to read input report, GetOverlappedResult failed with error %u\n", GetLastError());
        return;
    }

    button_length = ARRAY_SIZE(buttons);
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, buttons, &button_length, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsages HID_USAGE_PAGE_BUTTON returned %#x\n", status);

    state.Gamepad.wButtons = 0;
    for (i = 0; i < button_length; i++)
    {
        switch (buttons[i])
        {
        case 1: state.Gamepad.wButtons |= XINPUT_GAMEPAD_A; break;
        case 2: state.Gamepad.wButtons |= XINPUT_GAMEPAD_B; break;
        case 3: state.Gamepad.wButtons |= XINPUT_GAMEPAD_X; break;
        case 4: state.Gamepad.wButtons |= XINPUT_GAMEPAD_Y; break;
        case 5: state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER; break;
        case 6: state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
        case 7: state.Gamepad.wButtons |= XINPUT_GAMEPAD_BACK; break;
        case 8: state.Gamepad.wButtons |= XINPUT_GAMEPAD_START; break;
        case 9: state.Gamepad.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB; break;
        case 10: state.Gamepad.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB; break;
        case 11: state.Gamepad.wButtons |= XINPUT_GAMEPAD_GUIDE; break;
        }
    }

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_HATSWITCH returned %#x\n", status);
    else switch (value)
    {
    /* 8 1 2
     * 7 0 3
     * 6 5 4 */
    case 0: break;
    case 1: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP; break;
    case 2: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT; break;
    case 3: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; break;
    case 4: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN; break;
    case 5: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN; break;
    case 6: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT; break;
    case 7: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT; break;
    case 8: state.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP; break;
    }

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_X returned %#x\n", status);
    else state.Gamepad.sThumbLX = scale_value(value, &controller->hid.lx_caps, -32768, 32767);

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_Y returned %#x\n", status);
    else state.Gamepad.sThumbLY = -scale_value(value, &controller->hid.ly_caps, -32768, 32767) - 1;

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RX returned %#x\n", status);
    else state.Gamepad.sThumbRX = scale_value(value, &controller->hid.rx_caps, -32768, 32767);

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RY returned %#x\n", status);
    else state.Gamepad.sThumbRY = -scale_value(value, &controller->hid.ry_caps, -32768, 32767) - 1;

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RZ, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_RZ returned %#x\n", status);
    else state.Gamepad.bRightTrigger = scale_value(value, &controller->hid.rt_caps, 0, 255);

    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value, controller->hid.preparsed, report_buf, report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue HID_USAGE_PAGE_GENERIC / HID_USAGE_GENERIC_Z returned %#x\n", status);
    else state.Gamepad.bLeftTrigger = scale_value(value, &controller->hid.lt_caps, 0, 255);

    EnterCriticalSection(&controller->crit);
    if (controller->enabled)
    {
        state.dwPacketNumber = controller->state.dwPacketNumber + 1;
        controller->state = state;
        memset(&controller->hid.read_ovl, 0, sizeof(controller->hid.read_ovl));
        controller->hid.read_ovl.hEvent = controller->hid.read_event;
        ReadFile(controller->device, controller->hid.input_report_buf, controller->hid.caps.InputReportByteLength, NULL, &controller->hid.read_ovl);
    }
    LeaveCriticalSection(&controller->crit);
}

static DWORD WINAPI hid_update_thread_proc(void *param)
{
    struct xinput_controller *devices[XUSER_MAX_COUNT + 2];
    HANDLE events[XUSER_MAX_COUNT + 2];
    DWORD i, count = 2, ret = WAIT_TIMEOUT;

    do
    {
        EnterCriticalSection(&xinput_crit);
        if (ret == WAIT_TIMEOUT) update_controller_list();
        if (ret < count - 2) read_controller_state(devices[ret]);

        count = 0;
        for (i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            if (!controllers[i].device) continue;
            EnterCriticalSection(&controllers[i].crit);
            if (controllers[i].enabled)
            {
                devices[count] = controllers + i;
                events[count] = controllers[i].hid.read_event;
                count++;
            }
            LeaveCriticalSection(&controllers[i].crit);
        }
        events[count++] = update_event;
        events[count++] = stop_event;
        LeaveCriticalSection(&xinput_crit);
    }
    while ((ret = WaitForMultipleObjectsEx( count, events, FALSE, 2000, TRUE )) < count - 1 || ret == WAIT_TIMEOUT);

    if (ret != count - 1) ERR("update thread exited unexpectedly, ret %u\n", ret);
    SetEvent(done_event);
    return ret;
}

static BOOL WINAPI start_update_thread_once( INIT_ONCE *once, void *param, void **context )
{
    HANDLE thread;

    stop_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!stop_event) ERR("failed to create stop event, error %u\n", GetLastError());

    done_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!done_event) ERR("failed to create stop event, error %u\n", GetLastError());

    update_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!update_event) ERR("failed to create update event, error %u\n", GetLastError());

    thread = CreateThread(NULL, 0, hid_update_thread_proc, NULL, 0, NULL);
    if (!thread) ERR("failed to create update thread, error %u\n", GetLastError());
    CloseHandle(thread);

    /* do it once now, to resolve delayed imports and populate the initial list */
    EnterCriticalSection(&xinput_crit);
    update_controller_list();
    LeaveCriticalSection(&xinput_crit);

    return TRUE;
}

static void start_update_thread(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&init_once, start_update_thread_once, NULL, NULL);
}

static BOOL controller_lock(struct xinput_controller *controller)
{
    if (!controller->device) return FALSE;

    EnterCriticalSection(&controller->crit);

    if (!controller->device)
    {
        LeaveCriticalSection(&controller->crit);
        return FALSE;
    }

    return TRUE;
}

static void controller_unlock(struct xinput_controller *controller)
{
    LeaveCriticalSection(&controller->crit);
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        stop_update_thread();
        break;
    }
    return TRUE;
}

void WINAPI DECLSPEC_HOTPATCH XInputEnable(BOOL enable)
{
    int index;

    TRACE("(enable %d)\n", enable);

    /* Setting to false will stop messages from XInputSetState being sent
    to the controllers. Setting to true will send the last vibration
    value (sent to XInputSetState) to the controller and allow messages to
    be sent */
    start_update_thread();

    for (index = 0; index < XUSER_MAX_COUNT; index++)
    {
        if (!controller_lock(&controllers[index])) continue;
        if (enable) controller_enable(&controllers[index]);
        else controller_disable(&controllers[index]);
        controller_unlock(&controllers[index]);
    }
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputSetState(DWORD index, XINPUT_VIBRATION *vibration)
{
    DWORD ret;

    TRACE("(index %u, vibration %p)\n", index, vibration);

    start_update_thread();

    if (index >= XUSER_MAX_COUNT) return ERROR_BAD_ARGUMENTS;
    if (!controller_lock(&controllers[index])) return ERROR_DEVICE_NOT_CONNECTED;

    ret = HID_set_state(&controllers[index], vibration);

    controller_unlock(&controllers[index]);

    return ret;
}

/* Some versions of SteamOverlayRenderer hot-patch XInputGetStateEx() and call
 * XInputGetState() in the hook, so we need a wrapper. */
static DWORD xinput_get_state(DWORD index, XINPUT_STATE *state)
{
    if (!state) return ERROR_BAD_ARGUMENTS;

    start_update_thread();

    if (index >= XUSER_MAX_COUNT) return ERROR_BAD_ARGUMENTS;
    if (!controller_lock(&controllers[index])) return ERROR_DEVICE_NOT_CONNECTED;

    *state = controllers[index].state;
    controller_unlock(&controllers[index]);

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetState(DWORD index, XINPUT_STATE *state)
{
    DWORD ret;

    TRACE("(index %u, state %p)!\n", index, state);

    ret = xinput_get_state(index, state);
    if (ret != ERROR_SUCCESS) return ret;

    /* The main difference between this and the Ex version is the media guide button */
    state->Gamepad.wButtons &= ~XINPUT_GAMEPAD_GUIDE;

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetStateEx(DWORD index, XINPUT_STATE *state)
{
    TRACE("(index %u, state %p)!\n", index, state);

    return xinput_get_state(index, state);
}

static const int JS_STATE_OFF = 0;
static const int JS_STATE_LOW = 1;
static const int JS_STATE_HIGH = 2;

static int joystick_state(const SHORT value)
{
    if (value > 20000) return JS_STATE_HIGH;
    if (value < -20000) return JS_STATE_LOW;
    return JS_STATE_OFF;
}

static WORD js_vk_offs(const int x, const int y)
{
    if (y == JS_STATE_OFF)
    {
      /*if (x == JS_STATE_OFF) shouldn't get here */
        if (x == JS_STATE_LOW) return 3; /* LEFT */
      /*if (x == JS_STATE_HIGH)*/ return 2; /* RIGHT */
    }
    if (y == JS_STATE_HIGH)
    {
        if (x == JS_STATE_OFF) return 0; /* UP */
        if (x == JS_STATE_LOW) return 4; /* UPLEFT */
      /*if (x == JS_STATE_HIGH)*/ return 5; /* UPRIGHT */
    }
  /*if (y == JS_STATE_LOW)*/
    {
        if (x == JS_STATE_OFF) return 1; /* DOWN */
        if (x == JS_STATE_LOW) return 7; /* DOWNLEFT */
      /*if (x == JS_STATE_HIGH)*/ return 6; /* DOWNRIGHT */
    }
}

static DWORD check_joystick_keystroke(const DWORD index, XINPUT_KEYSTROKE *keystroke, const SHORT *cur_x,
                                      const SHORT *cur_y, SHORT *last_x, SHORT *last_y, const WORD base_vk)
{
    int cur_vk = 0, cur_x_st, cur_y_st;
    int last_vk = 0, last_x_st, last_y_st;

    cur_x_st = joystick_state(*cur_x);
    cur_y_st = joystick_state(*cur_y);
    if (cur_x_st || cur_y_st)
        cur_vk = base_vk + js_vk_offs(cur_x_st, cur_y_st);

    last_x_st = joystick_state(*last_x);
    last_y_st = joystick_state(*last_y);
    if (last_x_st || last_y_st)
        last_vk = base_vk + js_vk_offs(last_x_st, last_y_st);

    if (cur_vk != last_vk)
    {
        if (last_vk)
        {
            /* joystick was set, and now different. send a KEYUP event, and set
             * last pos to centered, so the appropriate KEYDOWN event will be
             * sent on the next call. */
            keystroke->VirtualKey = last_vk;
            keystroke->Unicode = 0; /* unused */
            keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
            keystroke->UserIndex = index;
            keystroke->HidCode = 0;

            *last_x = 0;
            *last_y = 0;

            return ERROR_SUCCESS;
        }

        /* joystick was unset, send KEYDOWN. */
        keystroke->VirtualKey = cur_vk;
        keystroke->Unicode = 0; /* unused */
        keystroke->Flags = XINPUT_KEYSTROKE_KEYDOWN;
        keystroke->UserIndex = index;
        keystroke->HidCode = 0;

        *last_x = *cur_x;
        *last_y = *cur_y;

        return ERROR_SUCCESS;
    }

    *last_x = *cur_x;
    *last_y = *cur_y;

    return ERROR_EMPTY;
}

static BOOL trigger_is_on(const BYTE value)
{
    return value > 30;
}

static DWORD check_for_keystroke(const DWORD index, XINPUT_KEYSTROKE *keystroke)
{
    struct xinput_controller *controller = &controllers[index];
    const XINPUT_GAMEPAD *cur;
    DWORD ret = ERROR_EMPTY;
    int i;

    static const struct
    {
        int mask;
        WORD vk;
    } buttons[] = {
        { XINPUT_GAMEPAD_DPAD_UP, VK_PAD_DPAD_UP },
        { XINPUT_GAMEPAD_DPAD_DOWN, VK_PAD_DPAD_DOWN },
        { XINPUT_GAMEPAD_DPAD_LEFT, VK_PAD_DPAD_LEFT },
        { XINPUT_GAMEPAD_DPAD_RIGHT, VK_PAD_DPAD_RIGHT },
        { XINPUT_GAMEPAD_START, VK_PAD_START },
        { XINPUT_GAMEPAD_BACK, VK_PAD_BACK },
        { XINPUT_GAMEPAD_LEFT_THUMB, VK_PAD_LTHUMB_PRESS },
        { XINPUT_GAMEPAD_RIGHT_THUMB, VK_PAD_RTHUMB_PRESS },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, VK_PAD_LSHOULDER },
        { XINPUT_GAMEPAD_RIGHT_SHOULDER, VK_PAD_RSHOULDER },
        { XINPUT_GAMEPAD_A, VK_PAD_A },
        { XINPUT_GAMEPAD_B, VK_PAD_B },
        { XINPUT_GAMEPAD_X, VK_PAD_X },
        { XINPUT_GAMEPAD_Y, VK_PAD_Y },
        /* note: guide button does not send an event */
    };

    if (!controller_lock(controller)) return ERROR_DEVICE_NOT_CONNECTED;

    cur = &controller->state.Gamepad;

    /*** buttons ***/
    for (i = 0; i < ARRAY_SIZE(buttons); ++i)
    {
        if ((cur->wButtons & buttons[i].mask) ^ (controller->last_keystroke.wButtons & buttons[i].mask))
        {
            keystroke->VirtualKey = buttons[i].vk;
            keystroke->Unicode = 0; /* unused */
            if (cur->wButtons & buttons[i].mask)
            {
                keystroke->Flags = XINPUT_KEYSTROKE_KEYDOWN;
                controller->last_keystroke.wButtons |= buttons[i].mask;
            }
            else
            {
                keystroke->Flags = XINPUT_KEYSTROKE_KEYUP;
                controller->last_keystroke.wButtons &= ~buttons[i].mask;
            }
            keystroke->UserIndex = index;
            keystroke->HidCode = 0;
            ret = ERROR_SUCCESS;
            goto done;
        }
    }

    /*** triggers ***/
    if (trigger_is_on(cur->bLeftTrigger) ^ trigger_is_on(controller->last_keystroke.bLeftTrigger))
    {
        keystroke->VirtualKey = VK_PAD_LTRIGGER;
        keystroke->Unicode = 0; /* unused */
        keystroke->Flags = trigger_is_on(cur->bLeftTrigger) ? XINPUT_KEYSTROKE_KEYDOWN : XINPUT_KEYSTROKE_KEYUP;
        keystroke->UserIndex = index;
        keystroke->HidCode = 0;
        controller->last_keystroke.bLeftTrigger = cur->bLeftTrigger;
        ret = ERROR_SUCCESS;
        goto done;
    }

    if (trigger_is_on(cur->bRightTrigger) ^ trigger_is_on(controller->last_keystroke.bRightTrigger))
    {
        keystroke->VirtualKey = VK_PAD_RTRIGGER;
        keystroke->Unicode = 0; /* unused */
        keystroke->Flags = trigger_is_on(cur->bRightTrigger) ? XINPUT_KEYSTROKE_KEYDOWN : XINPUT_KEYSTROKE_KEYUP;
        keystroke->UserIndex = index;
        keystroke->HidCode = 0;
        controller->last_keystroke.bRightTrigger = cur->bRightTrigger;
        ret = ERROR_SUCCESS;
        goto done;
    }

    /*** joysticks ***/
    ret = check_joystick_keystroke(index, keystroke, &cur->sThumbLX, &cur->sThumbLY,
            &controller->last_keystroke.sThumbLX,
            &controller->last_keystroke.sThumbLY, VK_PAD_LTHUMB_UP);
    if (ret == ERROR_SUCCESS)
        goto done;

    ret = check_joystick_keystroke(index, keystroke, &cur->sThumbRX, &cur->sThumbRY,
            &controller->last_keystroke.sThumbRX,
            &controller->last_keystroke.sThumbRY, VK_PAD_RTHUMB_UP);
    if (ret == ERROR_SUCCESS)
        goto done;

done:
    controller_unlock(controller);

    return ret;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetKeystroke(DWORD index, DWORD reserved, PXINPUT_KEYSTROKE keystroke)
{
    TRACE("(index %u, reserved %u, keystroke %p)\n", index, reserved, keystroke);

    if (index >= XUSER_MAX_COUNT && index != XUSER_INDEX_ANY) return ERROR_BAD_ARGUMENTS;

    if (index == XUSER_INDEX_ANY)
    {
        int i;
        for (i = 0; i < XUSER_MAX_COUNT; ++i)
            if (check_for_keystroke(i, keystroke) == ERROR_SUCCESS)
                return ERROR_SUCCESS;
        return ERROR_EMPTY;
    }

    return check_for_keystroke(index, keystroke);
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES *capabilities)
{
    TRACE("(index %u, flags 0x%x, capabilities %p)\n", index, flags, capabilities);

    start_update_thread();

    if (index >= XUSER_MAX_COUNT) return ERROR_BAD_ARGUMENTS;

    if (!controller_lock(&controllers[index])) return ERROR_DEVICE_NOT_CONNECTED;

    if (flags & XINPUT_FLAG_GAMEPAD && controllers[index].caps.SubType != XINPUT_DEVSUBTYPE_GAMEPAD)
    {
        controller_unlock(&controllers[index]);
        return ERROR_DEVICE_NOT_CONNECTED;
    }

    memcpy(capabilities, &controllers[index].caps, sizeof(*capabilities));

    controller_unlock(&controllers[index]);

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetDSoundAudioDeviceGuids(DWORD index, GUID *render_guid, GUID *capture_guid)
{
    FIXME("(index %u, render guid %p, capture guid %p) Stub!\n", index, render_guid, capture_guid);

    if (index >= XUSER_MAX_COUNT) return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].device) return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetBatteryInformation(DWORD index, BYTE type, XINPUT_BATTERY_INFORMATION* battery)
{
    static int once;

    if (!once++) FIXME("(index %u, type %u, battery %p) Stub!\n", index, type, battery);

    if (index >= XUSER_MAX_COUNT) return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].device) return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}
