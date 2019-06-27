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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "xinput.h"
#include "xinput_private.h"

/* Not defined in the headers, used only by XInputGetStateEx */
#define XINPUT_GAMEPAD_GUIDE 0x0400

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

xinput_controller controllers[XUSER_MAX_COUNT];

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(inst);
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            HID_destroy_gamepads(controllers);
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
    HID_find_gamepads(controllers);

    for (index = 0; index < XUSER_MAX_COUNT; index ++)
    {
        if (!controllers[index].connected) continue;
        HID_enable(&controllers[index], enable);
    }
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputSetState(DWORD index, XINPUT_VIBRATION* vibration)
{
    TRACE("(index %u, vibration %p)\n", index, vibration);

    HID_find_gamepads(controllers);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return HID_set_state(&controllers[index], vibration);
}

/* Some versions of SteamOverlayRenderer hot-patch XInputGetStateEx() and call
 * XInputGetState() in the hook, so we need a wrapper. */
static DWORD xinput_get_state(DWORD index, XINPUT_STATE *state)
{
    HID_find_gamepads(controllers);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    HID_update_state(&controllers[index]);
    memcpy(state, &controllers[index].state, sizeof(XINPUT_STATE));

    return ERROR_SUCCESS;
}


DWORD WINAPI DECLSPEC_HOTPATCH XInputGetState(DWORD index, XINPUT_STATE* state)
{
    DWORD ret;

    TRACE("(index %u, state %p)!\n", index, state);

    ret = xinput_get_state(index, state);
    if (ret != ERROR_SUCCESS)
        return ret;

    /* The main difference between this and the Ex version is the media guide button */
    state->Gamepad.wButtons &= ~XINPUT_GAMEPAD_GUIDE;

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetStateEx(DWORD index, XINPUT_STATE* state)
{
    TRACE("(index %u, state %p)!\n", index, state);

    return xinput_get_state(index, state);
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetKeystroke(DWORD index, DWORD reserved, PXINPUT_KEYSTROKE keystroke)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(index %u, reserved %u, keystroke %p) Stub!\n", index, reserved, keystroke);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetCapabilities(DWORD index, DWORD flags, XINPUT_CAPABILITIES* capabilities)
{
    TRACE("(index %u, flags 0x%x, capabilities %p)\n", index, flags, capabilities);

    HID_find_gamepads(controllers);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;
    if (flags & XINPUT_FLAG_GAMEPAD && controllers[index].caps.SubType != XINPUT_DEVSUBTYPE_GAMEPAD)
        return ERROR_DEVICE_NOT_CONNECTED;

    memcpy(capabilities, &controllers[index].caps, sizeof(*capabilities));

    return ERROR_SUCCESS;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetDSoundAudioDeviceGuids(DWORD index, GUID* render_guid, GUID* capture_guid)
{
    FIXME("(index %u, render guid %p, capture guid %p) Stub!\n", index, render_guid, capture_guid);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}

DWORD WINAPI DECLSPEC_HOTPATCH XInputGetBatteryInformation(DWORD index, BYTE type, XINPUT_BATTERY_INFORMATION* battery)
{
    static int once;

    if (!once++)
        FIXME("(index %u, type %u, battery %p) Stub!\n", index, type, battery);

    if (index >= XUSER_MAX_COUNT)
        return ERROR_BAD_ARGUMENTS;
    if (!controllers[index].connected)
        return ERROR_DEVICE_NOT_CONNECTED;

    return ERROR_NOT_SUPPORTED;
}
