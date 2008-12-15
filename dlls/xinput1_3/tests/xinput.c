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

#include "xinput.h"
#include "wine/test.h"

static DWORD (WINAPI *pXInputGetState)(DWORD, XINPUT_STATE*);
static DWORD (WINAPI *pXInputGetCapabilities)(DWORD,DWORD,XINPUT_CAPABILITIES*);

static void test_get_state(void)
{

    XINPUT_STATE controllerState;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum=0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&controllerState, sizeof(XINPUT_STATE));

        result = pXInputGetState(controllerNum, &controllerState);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetState failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %d is not connected\n", controllerNum);
        }
        else
        {
            trace("-- Results for controller %d --\n", controllerNum);
            trace("XInputGetState: %d\n", result);
            trace("State->dwPacketNumber: %d\n", controllerState.dwPacketNumber);
            trace("Gamepad Variables --\n");
            trace("Gamepad.wButtons: %#x\n", controllerState.Gamepad.wButtons);
            trace("Gamepad.bLeftTrigger: 0x%08x\n", controllerState.Gamepad.bLeftTrigger);
            trace("Gamepad.bRightTrigger: 0x%08x\n", controllerState.Gamepad.bRightTrigger);
            trace("Gamepad.sThumbLX: %d\n", controllerState.Gamepad.sThumbLX);
            trace("Gamepad.sThumbLY: %d\n", controllerState.Gamepad.sThumbLY);
            trace("Gamepad.sThumbRX: %d\n", controllerState.Gamepad.sThumbRX);
            trace("Gamepad.sThumbRY: %d\n", controllerState.Gamepad.sThumbRY);
        }
    }

    ZeroMemory(&controllerState, sizeof(XINPUT_STATE));
    result = pXInputGetState(XUSER_MAX_COUNT+1, &controllerState);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetState returned (%d)\n", result);
}

static void test_get_capabilities(void)
{
    XINPUT_CAPABILITIES capabilities;
    DWORD controllerNum;
    DWORD result;

    for(controllerNum=0; controllerNum < XUSER_MAX_COUNT; controllerNum++)
    {
        ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));

        result = pXInputGetCapabilities(controllerNum, XINPUT_FLAG_GAMEPAD, &capabilities);
        ok(result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED, "XInputGetCapabilities failed with (%d)\n", result);

        if (ERROR_DEVICE_NOT_CONNECTED == result)
        {
            skip("Controller %d is not connected\n", controllerNum);
        }
    }

    ZeroMemory(&capabilities, sizeof(XINPUT_CAPABILITIES));
    result = pXInputGetCapabilities(XUSER_MAX_COUNT+1, XINPUT_FLAG_GAMEPAD, &capabilities);
    ok(result == ERROR_BAD_ARGUMENTS, "XInputGetCapabilities returned (%d)\n", result);
}

START_TEST(xinput)
{
    HMODULE hXinput;
    hXinput = LoadLibraryA( "xinput1_3.dll" );

    if (!hXinput)
    {
        win_skip("Could not load xinput1_3.dll\n");
        return;
    }

    pXInputGetState = (void*)GetProcAddress(hXinput, "XInputGetState");
    pXInputGetCapabilities = (void*)GetProcAddress(hXinput, "XInputGetCapabilities");
    test_get_state();
    test_get_capabilities();
    FreeLibrary(hXinput);
}
