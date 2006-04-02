/*
 * Copyright (c) 2005 Robert Reif
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

#define DIRECTINPUT_VERSION 0x0700

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dinput.h"
#include "dxerr8.h"
#include "dinput_test.h"

const char * get_file_version(const char * file_name)
{
    static char version[32];
    DWORD size;
    DWORD handle;

    size = GetFileVersionInfoSizeA(file_name, &handle);
    if (size) {
        char * data = HeapAlloc(GetProcessHeap(), 0, size);
        if (data) {
            if (GetFileVersionInfoA(file_name, handle, size, data)) {
                VS_FIXEDFILEINFO *pFixedVersionInfo;
                UINT len;
                if (VerQueryValueA(data, "\\", (LPVOID *)&pFixedVersionInfo, &len)) {
                    sprintf(version, "%ld.%ld.%ld.%ld",
                            pFixedVersionInfo->dwFileVersionMS >> 16,
                            pFixedVersionInfo->dwFileVersionMS & 0xffff,
                            pFixedVersionInfo->dwFileVersionLS >> 16,
                            pFixedVersionInfo->dwFileVersionLS & 0xffff);
                } else
                    sprintf(version, "not available");
            } else
                sprintf(version, "failed");

            HeapFree(GetProcessHeap(), 0, data);
        } else
            sprintf(version, "failed");
    } else
        sprintf(version, "not available");

    return version;
}

static void keyboard_tests(DWORD version)
{
    HRESULT hr;
    LPDIRECTINPUT pDI;
    LPDIRECTINPUTDEVICE pKeyboard;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    BYTE kbd_state[256];
    ULONG ref;

    hr = DirectInputCreate(hInstance, version, &pDI, NULL);
    ok(SUCCEEDED(hr), "DirectInputCreate() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;
    
    
    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr))
    {
        IDirectInput_Release(pDI);
        return;
    }

    hr = IDirectInputDevice_SetDataFormat(pKeyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, NULL, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetCooperativeLevel() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState(10,) should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState() should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetDeviceState(10,) should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %s\n", DXGetErrorString8(hr));
    
    ref = IDirectInput_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %ld\n", ref);
}

START_TEST(keyboard)
{
    CoInitialize(NULL);

    trace("DLL Version: %s\n", get_file_version("dinput.dll"));

    keyboard_tests(0x0700);

    CoUninitialize();
}
