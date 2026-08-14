/*
 * The Wine project - Xinput Joystick Library
 * Copyright 2008 Andrew Fenn
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

#include <windows.h>
#include <stdio.h>

#include "winioctl.h"
#include "xinput.h"
#include "shlwapi.h"
#include "setupapi.h"
#include "ddk/hidsdi.h"
#include "ddk/hidclass.h"
#include "wine/test.h"

static DWORD (WINAPI *pXInputGetState)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputGetStateEx)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputGetCapabilities)(DWORD,DWORD,XINPUT_CAPABILITIES*);
static DWORD (WINAPI *pXInputSetState)(DWORD, XINPUT_VIBRATION*);
static void  (WINAPI *pXInputEnable)(BOOL);
static DWORD (WINAPI *pXInputGetKeystroke)(DWORD, DWORD, PXINPUT_KEYSTROKE);
static DWORD (WINAPI *pXInputGetDSoundAudioDeviceGuids)(DWORD, GUID*, GUID*);
static DWORD (WINAPI *pXInputGetBatteryInformation)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);

static void dump_gamepad(XINPUT_GAMEPAD *data)
{
    trace("-- Gamepad Variables --\n");
    trace("Gamepad.wButtons: %#x\n", data->wButtons);
    trace("Gamepad.bLeftTrigger: %d\n", data->bLeftTrigger);
    trace("Gamepad.bRightTrigger: %d\n", data->bRightTrigger);
    trace("Gamepad.sThumbLX: %d\n", data->sThumbLX);
    trace("Gamepad.sThumbLY: %d\n", data->sThumbLY);
    trace("Gamepad.sThumbRX: %d\n", data->sThumbRX);
    trace("Gamepad.sThumbRY: %d\n\n", data->sThumbRY);
}

static void test_set_state(void)
{
    XINPUT_VIBRATION vibrator;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&vibrator, sizeof(XINPUT_VIBRATION));

        vibrator.wLeftMotorSpeed = 32767;
        vibrator.wRightMotorSpeed = 32767;
        result = pXInputSetState(controllerNum, &vibrator);
        if (result == ERROR_DEVICE_NOT_CONNECTED) continue;

        Sleep(250);
        vibrator.wLeftMotorSpeed = 0;
        vibrator.wRightMotorSpeed = 0;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState returned %lu\n", result);

        /* Disabling XInput here, queueing a vibration and then re-enabling XInput
         * is used to prove that vibrations are auto enabled when resuming XInput.
         * If XInputEnable(1) is removed below the vibration will never play. */
        if (pXInputEnable) pXInputEnable(0);

        Sleep(250);
        vibrator.wLeftMotorSpeed = 65535;
        vibrator.wRightMotorSpeed = 65535;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState returned %lu\n", result);

        if (pXInputEnable) pXInputEnable(1);
        Sleep(250);

        vibrator.wLeftMotorSpeed = 0;
        vibrator.wRightMotorSpeed = 0;
        result = pXInputSetState(controllerNum, &vibrator);
        ok(result == ERROR_SUCCESS, "XInputSetState returned %lu\n", result);
    }

    result = pXInputSetState(XUSER_MAX_COUNT+1, &vibrator);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputSetState returned %lu\n", result);
}

static void test_get_state(void)
{
    XINPUT_STATE state;
    DWORD controllerNum, i, result, good = XUSER_MAX_COUNT;

    for (i = 0; i < (pXInputGetStateEx ? 2 : 1); i++)
    {
        for (controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
        {
            ZeroMemory(&state, sizeof(state));

            if (i == 0)
                result = pXInputGetState(controllerNum, &state);
            else
                result = pXInputGetStateEx(controllerNum, &state);
            ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED,
               "%s returned %lu\n", i == 0 ? "XInputGetState" : "XInputGetStateEx", result);

            if (ERROR_DEVICE_NOT_CONNECTED == result)
            {
                skip("Controller %lu is not connected\n", controllerNum);
                continue;
            }

            trace("-- Results for controller %lu --\n", controllerNum);
            if (i == 0)
            {
                good = controllerNum;
                trace("XInputGetState: %lu\n", result);
            }
            else
                trace("XInputGetStateEx: %lu\n", result);
            trace("State->dwPacketNumber: %lu\n", state.dwPacketNumber);
            dump_gamepad(&state.Gamepad);
        }
    }

    result = pXInputGetState(0, NULL);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned %lu\n", result);

    result = pXInputGetState(XUSER_MAX_COUNT, &state);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned %lu\n", result);

    result = pXInputGetState(XUSER_MAX_COUNT+1, &state);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned %lu\n", result);
    if (pXInputGetStateEx)
    {
        result = pXInputGetStateEx(XUSER_MAX_COUNT, &state);
        ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned %lu\n", result);

        result = pXInputGetStateEx(XUSER_MAX_COUNT+1, &state);
        ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned %lu\n", result);
    }

    if (winetest_interactive && good < XUSER_MAX_COUNT)
    {
        DWORD now = GetTickCount(), packet = 0;
        XINPUT_GAMEPAD *game = &state.Gamepad;

        trace("You have 20 seconds to test the joystick freely\n");
        do
        {
            Sleep(100);
            pXInputGetState(good, &state);
            if (state.dwPacketNumber == packet)
                continue;

            packet = state.dwPacketNumber;
            trace("Buttons 0x%04X Triggers %3d/%3d LT %6d/%6d RT %6d/%6d\n",
                  game->wButtons, game->bLeftTrigger, game->bRightTrigger,
                  game->sThumbLX, game->sThumbLY, game->sThumbRX, game->sThumbRY);
        }
        while(GetTickCount() - now < 20000);
        trace("Test over...\n");
    }
}

static void test_get_keystroke(void)
{
    XINPUT_KEYSTROKE keystroke;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&keystroke, sizeof(XINPUT_KEYSTROKE));

        result = pXInputGetKeystroke(controllerNum, XINPUT_FLAG_GAMEPAD, &keystroke);
        ok(result == ERROR_EMPTY || result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED,
           "XInputGetKeystroke returned %lu\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %lu is not connected\n", controllerNum);
        }
    }

    ZeroMemory(&keystroke, sizeof(XINPUT_KEYSTROKE));
    result = pXInputGetKeystroke(XUSER_MAX_COUNT+1, XINPUT_FLAG_GAMEPAD, &keystroke);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetKeystroke returned %lu\n", result);
}

static void test_get_capabilities(void)
{
    XINPUT_CAPABILITIES capabilities;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));

        result = pXInputGetCapabilities(controllerNum, XINPUT_FLAG_GAMEPAD, &capabilities);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetCapabilities returned %lu\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %lu is not connected\n", controllerNum);
            continue;
        }

        /* Important to show that the results changed between 1.3 and 1.4 XInput version */
        dump_gamepad(&capabilities.Gamepad);
    }

    ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));
    result = pXInputGetCapabilities(XUSER_MAX_COUNT+1, XINPUT_FLAG_GAMEPAD, &capabilities);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetCapabilities returned %lu\n", result);
}

static void test_get_dsoundaudiodevice(void)
{
    DWORD controllerNum;
    DWORD result;
    GUID soundRender, soundCapture;
    GUID testGuid = {0xFFFFFFFF, 0xFFFF, 0xFFFF, {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}};
    GUID emptyGuid = {0x0, 0x0, 0x0, {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}};

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        soundRender = soundCapture = testGuid;
        result = pXInputGetDSoundAudioDeviceGuids(controllerNum, &soundRender, &soundCapture);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetDSoundAudioDeviceGuids returned %lu\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %lu is not connected\n", controllerNum);
            continue;
        }

        if (!IsEqualGUID(&soundRender, &emptyGuid))
            ok(!IsEqualGUID(&soundRender, &testGuid), "Broken GUID returned for sound render device\n");
        else
            trace("Headset phone not attached\n");

        if (!IsEqualGUID(&soundCapture, &emptyGuid))
            ok(!IsEqualGUID(&soundCapture, &testGuid), "Broken GUID returned for sound capture device\n");
        else
            trace("Headset microphone not attached\n");
    }

    result = pXInputGetDSoundAudioDeviceGuids(0, NULL, &soundCapture);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetDSoundAudioDeviceGuids returned %lu\n", result);

    result = pXInputGetDSoundAudioDeviceGuids(0, &soundRender, NULL);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetDSoundAudioDeviceGuids returned %lu\n", result);

    result = pXInputGetDSoundAudioDeviceGuids(XUSER_MAX_COUNT+1, &soundRender, &soundCapture);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetDSoundAudioDeviceGuids returned %lu\n", result);
}

static void test_get_batteryinformation(void)
{
    DWORD controllerNum;
    DWORD result;
    XINPUT_BATTERY_INFORMATION batteryInfo;

    for(controllerNum = 0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&batteryInfo, sizeof(XINPUT_BATTERY_INFORMATION));

        result = pXInputGetBatteryInformation(controllerNum, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetBatteryInformation returned %lu\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            ok(batteryInfo.BatteryLevel == BATTERY_TYPE_DISCONNECTED, "Failed to report device as being disconnected.\n");
            skip("Controller %lu is not connected\n", controllerNum);
        }
    }

    result = pXInputGetBatteryInformation(XUSER_MAX_COUNT+1, BATTERY_DEVTYPE_GAMEPAD, &batteryInfo);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetBatteryInformation returned %lu\n", result);
}

#define check_member_(file, line, val, exp, fmt, member)               \
        ok_(file, line)((val).member == (exp).member,                  \
                        "got " #member " " fmt ", expected " fmt "\n", \
                        (val).member, (exp).member)
#define check_member(val, exp, fmt, member) check_member_(__FILE__, __LINE__, val, exp, fmt, member)

static void check_hid_caps(DWORD index, HANDLE device,  PHIDP_PREPARSED_DATA preparsed,
                           HIDD_ATTRIBUTES *attrs, HIDP_CAPS *hid_caps)
{
    const HIDP_CAPS expect_hid_caps =
    {
        .Usage = HID_USAGE_GENERIC_GAMEPAD,
        .UsagePage = HID_USAGE_PAGE_GENERIC,
        .InputReportByteLength =
                attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff ? 16 :
                15,
        .OutputReportByteLength = 0,
        .FeatureReportByteLength = 0,
        .NumberLinkCollectionNodes = 4,
        .NumberInputButtonCaps = 1,
        .NumberInputValueCaps = 6,
        .NumberInputDataIndices =
                attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff ? 22 :
                16,
        .NumberFeatureButtonCaps = 0,
        .NumberFeatureValueCaps = 0,
        .NumberFeatureDataIndices = 0,
    };
    const HIDP_BUTTON_CAPS expect_button_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_GAMEPAD,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = TRUE,
            .IsAbsolute = TRUE,
            .Range.UsageMin = 0x01,
            .Range.UsageMax =
                    attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff ? 0x10 :
                    0x0a,
            .Range.DataIndexMin = 5,
            .Range.DataIndexMax =
                    attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff ? 20 :
                    14,
        },
    };
    const HIDP_VALUE_CAPS expect_value_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 2,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 16,
            .ReportCount = 1,
            .LogicalMax = -1,
            .PhysicalMax = -1,
            .NotRange.Usage = HID_USAGE_GENERIC_Y,
            .NotRange.DataIndex = 0,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 2,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 16,
            .ReportCount = 1,
            .LogicalMax = -1,
            .PhysicalMax = -1,
            .NotRange.Usage = HID_USAGE_GENERIC_X,
            .NotRange.DataIndex = 1,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 2,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 2,
            .IsAbsolute = TRUE,
            .BitSize = 16,
            .ReportCount = 1,
            .LogicalMax = -1,
            .PhysicalMax = -1,
            .NotRange.Usage = HID_USAGE_GENERIC_RY,
            .NotRange.DataIndex = 2,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 2,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 2,
            .IsAbsolute = TRUE,
            .BitSize = 16,
            .ReportCount = 1,
            .LogicalMax = -1,
            .PhysicalMax = -1,
            .NotRange.Usage = HID_USAGE_GENERIC_RX,
            .NotRange.DataIndex = 3,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 2,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 3,
            .IsAbsolute = TRUE,
            .BitSize = 16,
            .ReportCount = 1,
            .LogicalMax = -1,
            .PhysicalMax = -1,
            .NotRange.Usage = HID_USAGE_GENERIC_Z,
            .NotRange.DataIndex = 4,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .BitField = 66,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkUsage = HID_USAGE_GENERIC_GAMEPAD,
            .IsAbsolute = TRUE,
            .HasNull = TRUE,
            .BitSize = 4,
            .Units = 14,
            .ReportCount = 1,
            .LogicalMin = 1,
            .LogicalMax = 8,
            .PhysicalMin = 0x0000,
            .PhysicalMax = 0x103b,
            .NotRange.Usage = HID_USAGE_GENERIC_HATSWITCH,
            .NotRange.DataIndex =
                    attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff ? 21 :
                    15,
        },
    };
    static const HIDP_LINK_COLLECTION_NODE expect_collections[] =
    {
        {
            .LinkUsage = HID_USAGE_GENERIC_GAMEPAD,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .CollectionType = 1,
            .NumberOfChildren = 3,
            .FirstChild = 3,
        },
        { .LinkUsagePage = HID_USAGE_PAGE_GENERIC, .NextSibling = 0, },
        { .LinkUsagePage = HID_USAGE_PAGE_GENERIC, .NextSibling = 1, },
        { .LinkUsagePage = HID_USAGE_PAGE_GENERIC, .NextSibling = 2, },
    };

    HIDP_LINK_COLLECTION_NODE collections[16];
    HIDP_BUTTON_CAPS button_caps[16];
    HIDP_VALUE_CAPS value_caps[16];
    XINPUT_STATE last_state, state;
    XINPUT_CAPABILITIES xi_caps;
    char buffer[200] = {0};
    ULONG length, value;
    USAGE usages[15];
    NTSTATUS status;
    USHORT count;
    DWORD res;
    BOOL ret;
    int i;

    res = pXInputGetCapabilities(index, 0, &xi_caps);
    ok(res == ERROR_SUCCESS, "XInputGetCapabilities %lu returned %lu\n", index, res);

    res = pXInputGetState(index, &state);
    ok(res == ERROR_SUCCESS, "XInputGetState %lu returned %lu\n", index, res);

    ok(hid_caps->UsagePage == HID_USAGE_PAGE_GENERIC, "unexpected usage page %04x\n", hid_caps->UsagePage);
    ok(hid_caps->Usage == HID_USAGE_GENERIC_GAMEPAD, "unexpected usage %04x\n", hid_caps->Usage);

    check_member(*hid_caps, expect_hid_caps, "%04x", Usage);
    check_member(*hid_caps, expect_hid_caps, "%04x", UsagePage);
    check_member(*hid_caps, expect_hid_caps, "%d", InputReportByteLength);
    check_member(*hid_caps, expect_hid_caps, "%d", OutputReportByteLength);
    check_member(*hid_caps, expect_hid_caps, "%d", FeatureReportByteLength);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberLinkCollectionNodes);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberInputButtonCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberInputValueCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberInputDataIndices);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberOutputButtonCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberOutputValueCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberOutputDataIndices);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberFeatureButtonCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberFeatureValueCaps);
    check_member(*hid_caps, expect_hid_caps, "%d", NumberFeatureDataIndices);

    length = hid_caps->NumberLinkCollectionNodes;
    status = HidP_GetLinkCollectionNodes(collections, &length, preparsed);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetLinkCollectionNodes returned %#lx\n", status);
    ok(length == ARRAY_SIZE(expect_collections), "got %lu collections\n", length);

    for (i = 0; i < min(length, ARRAY_SIZE(expect_collections)); ++i)
    {
        winetest_push_context("collections[%d]", i);
        check_member(collections[i], expect_collections[i], "%04x", LinkUsage);
        check_member(collections[i], expect_collections[i], "%04x", LinkUsagePage);
        check_member(collections[i], expect_collections[i], "%d", Parent);
        check_member(collections[i], expect_collections[i], "%d", NumberOfChildren);
        check_member(collections[i], expect_collections[i], "%d", NextSibling);
        check_member(collections[i], expect_collections[i], "%d", FirstChild);
        check_member(collections[i], expect_collections[i], "%d", CollectionType);
        check_member(collections[i], expect_collections[i], "%d", IsAlias);
        winetest_pop_context();
    }

    count = hid_caps->NumberInputButtonCaps;
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &count, preparsed);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetButtonCaps returned %#lx\n", status);
    ok(count == ARRAY_SIZE(expect_button_caps), "got %d button caps\n", count);

    for (i = 0; i < ARRAY_SIZE(expect_button_caps); ++i)
    {
        winetest_push_context("button_caps[%d]", i);
        check_member(button_caps[i], expect_button_caps[i], "%04x", UsagePage);
        check_member(button_caps[i], expect_button_caps[i], "%d", ReportID);
        check_member(button_caps[i], expect_button_caps[i], "%d", IsAlias);
        check_member(button_caps[i], expect_button_caps[i], "%d", BitField);
        check_member(button_caps[i], expect_button_caps[i], "%d", LinkCollection);
        check_member(button_caps[i], expect_button_caps[i], "%04x", LinkUsage);
        check_member(button_caps[i], expect_button_caps[i], "%04x", LinkUsagePage);
        check_member(button_caps[i], expect_button_caps[i], "%d", IsRange);
        check_member(button_caps[i], expect_button_caps[i], "%d", IsStringRange);
        check_member(button_caps[i], expect_button_caps[i], "%d", IsDesignatorRange);
        check_member(button_caps[i], expect_button_caps[i], "%d", IsAbsolute);

        if (!button_caps[i].IsRange && !expect_button_caps[i].IsRange)
        {
            check_member(button_caps[i], expect_button_caps[i], "%04x", NotRange.Usage);
            check_member(button_caps[i], expect_button_caps[i], "%d", NotRange.DataIndex);
        }
        else if (button_caps[i].IsRange && expect_button_caps[i].IsRange)
        {
            check_member(button_caps[i], expect_button_caps[i], "%04x", Range.UsageMin);
            check_member(button_caps[i], expect_button_caps[i], "%04x", Range.UsageMax);
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.DataIndexMin);
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.DataIndexMax);
        }

        if (!button_caps[i].IsRange && !expect_button_caps[i].IsRange)
            check_member(button_caps[i], expect_button_caps[i], "%d", NotRange.StringIndex);
        else if (button_caps[i].IsStringRange && expect_button_caps[i].IsStringRange)
        {
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.StringMin);
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.StringMax);
        }

        if (!button_caps[i].IsDesignatorRange && !expect_button_caps[i].IsDesignatorRange)
            check_member(button_caps[i], expect_button_caps[i], "%d", NotRange.DesignatorIndex);
        else if (button_caps[i].IsDesignatorRange && expect_button_caps[i].IsDesignatorRange)
        {
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.DesignatorMin);
            check_member(button_caps[i], expect_button_caps[i], "%d", Range.DesignatorMax);
        }
        winetest_pop_context();
    }

    count = hid_caps->NumberInputValueCaps;
    status = HidP_GetValueCaps(HidP_Input, value_caps, &count, preparsed);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_GetValueCaps returned %#lx\n", status);
    ok(count == ARRAY_SIZE(expect_value_caps), "got %d value caps\n", count);

    for (i = 0; i < min(count, ARRAY_SIZE(expect_value_caps)); ++i)
    {
        winetest_push_context("value_caps[%d]", i);
        check_member(value_caps[i], expect_value_caps[i], "%04x", UsagePage);
        check_member(value_caps[i], expect_value_caps[i], "%d", ReportID);
        check_member(value_caps[i], expect_value_caps[i], "%d", IsAlias);
        check_member(value_caps[i], expect_value_caps[i], "%d", BitField);
        check_member(value_caps[i], expect_value_caps[i], "%d", LinkCollection);
        check_member(value_caps[i], expect_value_caps[i], "%d", LinkUsage);
        check_member(value_caps[i], expect_value_caps[i], "%d", LinkUsagePage);
        check_member(value_caps[i], expect_value_caps[i], "%d", IsRange);
        check_member(value_caps[i], expect_value_caps[i], "%d", IsStringRange);
        check_member(value_caps[i], expect_value_caps[i], "%d", IsDesignatorRange);
        check_member(value_caps[i], expect_value_caps[i], "%d", IsAbsolute);

        check_member(value_caps[i], expect_value_caps[i], "%d", HasNull);
        check_member(value_caps[i], expect_value_caps[i], "%d", BitSize);
        check_member(value_caps[i], expect_value_caps[i], "%d", ReportCount);
        check_member(value_caps[i], expect_value_caps[i], "%#lx", UnitsExp);
        check_member(value_caps[i], expect_value_caps[i], "%#lx", Units);
        check_member(value_caps[i], expect_value_caps[i], "%+ld", LogicalMin);
        check_member(value_caps[i], expect_value_caps[i], "%+ld", LogicalMax);
        check_member(value_caps[i], expect_value_caps[i], "%+ld", PhysicalMin);
        check_member(value_caps[i], expect_value_caps[i], "%+ld", PhysicalMax);

        if (!value_caps[i].IsRange && !expect_value_caps[i].IsRange)
        {
            check_member(value_caps[i], expect_value_caps[i], "%04x", NotRange.Usage);
            check_member(value_caps[i], expect_value_caps[i], "%d", NotRange.DataIndex);
        }
        else if (value_caps[i].IsRange && expect_value_caps[i].IsRange)
        {
            check_member(value_caps[i], expect_value_caps[i], "%04x", Range.UsageMin);
            check_member(value_caps[i], expect_value_caps[i], "%04x", Range.UsageMax);
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.DataIndexMin);
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.DataIndexMax);
        }

        if (!value_caps[i].IsRange && !expect_value_caps[i].IsRange)
            check_member(value_caps[i], expect_value_caps[i], "%d", NotRange.StringIndex);
        else if (value_caps[i].IsStringRange && expect_value_caps[i].IsStringRange)
        {
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.StringMin);
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.StringMax);
        }

        if (!value_caps[i].IsDesignatorRange && !expect_value_caps[i].IsDesignatorRange)
            check_member(value_caps[i], expect_value_caps[i], "%d", NotRange.DesignatorIndex);
        else if (value_caps[i].IsDesignatorRange && expect_value_caps[i].IsDesignatorRange)
        {
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.DesignatorMin);
            check_member(value_caps[i], expect_value_caps[i], "%d", Range.DesignatorMax);
        }
        winetest_pop_context();
    }

    status = HidP_InitializeReportForID(HidP_Input, 0, preparsed, buffer, hid_caps->InputReportByteLength);
    ok(status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status);

    SetLastError(0xdeadbeef);
    memset(buffer, 0, sizeof(buffer));
    ret = HidD_GetInputReport(device, buffer, hid_caps->InputReportByteLength);
    ok(!ret, "HidD_GetInputReport succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "HidD_GetInputReport returned error %lu\n", GetLastError());

    if (!winetest_interactive) skip("skipping interactive tests\n");
    /* ReadFile on Xbox One For Windows controller seems to never succeed */
    else if (attrs->VendorID == 0x045e && attrs->ProductID == 0x02ff) skip("skipping interactive tests (Xbox One For Windows)\n");
    else
    {
        res = pXInputGetState(index, &last_state);
        ok(res == ERROR_SUCCESS, "XInputGetState returned %#lx\n", res);

        trace("press A button on gamepad %lu\n", index);

        do
        {
            Sleep(5);
            res = pXInputGetState(index, &state);
            ok(res == ERROR_SUCCESS, "XInputGetState returned %#lx\n", res);
        } while (res == ERROR_SUCCESS && state.dwPacketNumber == last_state.dwPacketNumber);
        ok(state.Gamepad.wButtons & XINPUT_GAMEPAD_A, "unexpected button state %#x\n", state.Gamepad.wButtons);

        /* now read as many reports from the device to get a consistent final state */
        for (i = 0; i < (state.dwPacketNumber - last_state.dwPacketNumber); ++i)
        {
            SetLastError(0xdeadbeef);
            memset(buffer, 0, sizeof(buffer));
            length = hid_caps->InputReportByteLength;
            ret = ReadFile(device, buffer, hid_caps->InputReportByteLength, &length, NULL);
            ok(ret, "ReadFile failed, last error %lu\n", GetLastError());
            ok(length == hid_caps->InputReportByteLength, "ReadFile returned length %lu\n", length);
        }
        last_state = state;

        length = ARRAY_SIZE(usages);
        status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &length, preparsed, buffer, hid_caps->InputReportByteLength);
        ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#lx\n", status);
        ok(length == 1, "got length %lu\n", length);
        ok(usages[0] == 1, "got usages[0] %u\n", usages[0]);

        trace("release A on gamepad %lu\n", index);

        do
        {
            Sleep(5);
            res = pXInputGetState(index, &state);
            ok(res == ERROR_SUCCESS, "XInputGetState returned %#lx\n", res);
        } while (res == ERROR_SUCCESS && state.dwPacketNumber == last_state.dwPacketNumber);
        ok(!state.Gamepad.wButtons, "unexpected button state %#x\n", state.Gamepad.wButtons);

        /* now read as many reports from the device to get a consistent final state */
        for (i = 0; i < (state.dwPacketNumber - last_state.dwPacketNumber); ++i)
        {
            SetLastError(0xdeadbeef);
            memset(buffer, 0, sizeof(buffer));
            length = hid_caps->InputReportByteLength;
            ret = ReadFile(device, buffer, hid_caps->InputReportByteLength, &length, NULL);
            ok(ret, "ReadFile failed, last error %lu\n", GetLastError());
            ok(length == hid_caps->InputReportByteLength, "ReadFile returned length %lu\n", length);
        }
        last_state = state;

        length = ARRAY_SIZE(usages);
        status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &length, preparsed, buffer, hid_caps->InputReportByteLength);
        ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#lx\n", status);
        ok(length == 0, "got length %lu\n", length);

        trace("press both LT and RT on gamepad %lu\n", index);

        do
        {
            do
            {
                Sleep(5);
                res = pXInputGetState(index, &state);
                ok(res == ERROR_SUCCESS, "XInputGetState returned %#lx\n", res);
            } while (res == ERROR_SUCCESS && state.dwPacketNumber == last_state.dwPacketNumber);

            /* now read as many reports from the device to get a consistent final state */
            for (i = 0; i < (state.dwPacketNumber - last_state.dwPacketNumber); ++i)
            {
                SetLastError(0xdeadbeef);
                memset(buffer, 0, sizeof(buffer));
                length = hid_caps->InputReportByteLength;
                ret = ReadFile(device, buffer, hid_caps->InputReportByteLength, &length, NULL);
                ok(ret, "ReadFile failed, last error %lu\n", GetLastError());
                ok(length == hid_caps->InputReportByteLength, "ReadFile returned length %lu\n", length);
            }
            last_state = state;

            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status);
            ok(value == state.Gamepad.sThumbLX + 32768, "got LX value %lu\n", value);
            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status);
            ok(value == 32767 - state.Gamepad.sThumbLY, "got LY value %lu\n", value);
            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status);
            ok(value == state.Gamepad.sThumbRX + 32768, "got LX value %lu\n", value);
            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status);
            ok(value == 32767 - state.Gamepad.sThumbRY, "got LY value %lu\n", value);
            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status);
            ok(value == 32768 + (state.Gamepad.bLeftTrigger - state.Gamepad.bRightTrigger) * 128, "got Z value %lu (RT %d, LT %d)\n",
               value, state.Gamepad.bRightTrigger, state.Gamepad.bLeftTrigger);
            value = 0;
            status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RZ, &value, preparsed, buffer, hid_caps->InputReportByteLength);
            ok(status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#lx\n", status);
        } while (ret && (state.Gamepad.bRightTrigger != 255 || state.Gamepad.bLeftTrigger != 255));
    }
}

static BOOL try_open_hid_device(const WCHAR *path, HANDLE *device, PHIDP_PREPARSED_DATA *preparsed,
                                HIDD_ATTRIBUTES *attrs, HIDP_CAPS *caps)
{
    PHIDP_PREPARSED_DATA preparsed_data = NULL;
    HANDLE device_file;

    device_file = CreateFileW(path, FILE_READ_ACCESS | FILE_WRITE_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (device_file == INVALID_HANDLE_VALUE) return FALSE;

    if (!HidD_GetPreparsedData(device_file, &preparsed_data)) goto failed;
    if (!HidD_GetAttributes(device_file, attrs)) goto failed;
    if (HidP_GetCaps(preparsed_data, caps) != HIDP_STATUS_SUCCESS) goto failed;

    *device = device_file;
    *preparsed = preparsed_data;
    return TRUE;

failed:
    CloseHandle(device_file);
    HidD_FreePreparsedData(preparsed_data);
    return FALSE;
}

static void test_hid_reports(void)
{
    static const WCHAR prefix[] = L"\\\\?\\HID#VID_0000&PID_0000";
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *detail = (void *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA devinfo = {sizeof(devinfo)};
    PHIDP_PREPARSED_DATA preparsed;
    HIDD_ATTRIBUTES attrs;
    HIDP_CAPS caps;
    HDEVINFO set;
    HANDLE device;
    UINT32 i = 0, cnt = 0;
    GUID hid;

    HidD_GetHidGuid(&hid);

    set = SetupDiGetClassDevsW(&hid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed, error %lu\n", GetLastError());

    while (SetupDiEnumDeviceInterfaces(set, NULL, &hid, i++, &iface))
    {
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(set, &iface, detail, sizeof(buffer), NULL, &devinfo))
            continue;

        if (!try_open_hid_device(detail->DevicePath, &device, &preparsed, &attrs, &caps))
            continue;

        if (wcslen(detail->DevicePath) <= wcslen(prefix) ||
            wcsnicmp(detail->DevicePath + wcslen(prefix), L"&IG_", 4 ))
            continue;

        trace("found xinput HID device %s\n", wine_dbgstr_w(detail->DevicePath));
        check_hid_caps(cnt++, device, preparsed, &attrs, &caps);

        CloseHandle(device);
        HidD_FreePreparsedData(preparsed);
    }

    SetupDiDestroyDeviceInfoList(set);
}

START_TEST(xinput)
{
    struct
    {
        const char *name;
        int version;
    } libs[] = {
        { "xinput1_1.dll",   1 },
        { "xinput1_2.dll",   2 },
        { "xinput1_3.dll",   3 },
        { "xinput1_4.dll",   4 },
        { "xinput9_1_0.dll", 0 } /* legacy for XP/Vista */
    };
    HMODULE hXinput;
    void *pXInputGetStateEx_Ordinal;
    int i;

    for (i = 0; i < ARRAY_SIZE(libs); i++)
    {
        hXinput = LoadLibraryA( libs[i].name );

        if (!hXinput)
        {
            win_skip("Could not load %s\n", libs[i].name);
            continue;
        }
        trace("Testing %s\n", libs[i].name);

        pXInputEnable = (void*)GetProcAddress(hXinput, "XInputEnable");
        pXInputSetState = (void*)GetProcAddress(hXinput, "XInputSetState");
        pXInputGetState = (void*)GetProcAddress(hXinput, "XInputGetState");
        pXInputGetStateEx = (void*)GetProcAddress(hXinput, "XInputGetStateEx"); /* Win >= 8 */
        pXInputGetStateEx_Ordinal = (void*)GetProcAddress(hXinput, (LPCSTR) 100);
        pXInputGetKeystroke = (void*)GetProcAddress(hXinput, "XInputGetKeystroke");
        pXInputGetCapabilities = (void*)GetProcAddress(hXinput, "XInputGetCapabilities");
        pXInputGetDSoundAudioDeviceGuids = (void*)GetProcAddress(hXinput, "XInputGetDSoundAudioDeviceGuids");
        pXInputGetBatteryInformation = (void*)GetProcAddress(hXinput, "XInputGetBatteryInformation");

        /* XInputGetStateEx may not be present by name, use ordinal in this case */
        if (!pXInputGetStateEx)
            pXInputGetStateEx = pXInputGetStateEx_Ordinal;

        test_hid_reports();
        test_set_state();
        test_get_state();
        test_get_capabilities();

        if (libs[i].version != 4)
            test_get_dsoundaudiodevice();
        else
            ok(!pXInputGetDSoundAudioDeviceGuids, "XInputGetDSoundAudioDeviceGuids exists in %s\n", libs[i].name);

        if (libs[i].version > 2)
        {
            test_get_keystroke();
            test_get_batteryinformation();
            ok(pXInputGetStateEx != NULL, "XInputGetStateEx not found in %s\n", libs[i].name);
        }
        else
        {
            ok(!pXInputGetKeystroke, "XInputGetKeystroke exists in %s\n", libs[i].name);
            ok(!pXInputGetStateEx, "XInputGetStateEx exists in %s\n", libs[i].name);
            ok(!pXInputGetBatteryInformation, "XInputGetBatteryInformation exists in %s\n", libs[i].name);
            if (libs[i].version == 0)
                ok(!pXInputEnable, "XInputEnable exists in %s\n", libs[i].name);
        }

        FreeLibrary(hXinput);
    }
}
