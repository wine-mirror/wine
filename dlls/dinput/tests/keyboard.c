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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define DIRECTINPUT_VERSION 0x0700

#define COBJMACROS
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
    static char backslash[] = "\\";
    DWORD size;
    DWORD handle;

    size = GetFileVersionInfoSizeA(file_name, &handle);
    if (size) {
        char * data = HeapAlloc(GetProcessHeap(), 0, size);
        if (data) {
            if (GetFileVersionInfoA(file_name, handle, size, data)) {
                VS_FIXEDFILEINFO *pFixedVersionInfo;
                UINT len;
                if (VerQueryValueA(data, backslash, (LPVOID *)&pFixedVersionInfo, &len)) {
                    sprintf(version, "%d.%d.%d.%d",
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

static void acquire_tests(LPDIRECTINPUT pDI, HWND hwnd)
{
    HRESULT hr;
    LPDIRECTINPUTDEVICE pKeyboard;
    BYTE kbd_state[256];

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_SetDataFormat(pKeyboard, &c_dfDIKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, NULL, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetCooperativeLevel() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState(10,) should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(hr == DIERR_NOTACQUIRED, "IDirectInputDevice_GetDeviceState() should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Unacquire(pKeyboard);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Acquire(pKeyboard);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, 10, kbd_state);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetDeviceState(10,) should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_GetDeviceState(pKeyboard, sizeof(kbd_state), kbd_state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %s\n", DXGetErrorString8(hr));

    if (pKeyboard) IUnknown_Release(pKeyboard);
}

static const HRESULT SetCoop_null_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static const HRESULT SetCoop_real_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, S_OK,         S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_NOTIMPL,    S_OK,         E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static void test_set_coop(LPDIRECTINPUT pDI, HWND hwnd)
{
    HRESULT hr;
    LPDIRECTINPUTDEVICE pKeyboard = NULL;
    int i;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &pKeyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %s\n", i, DXGetErrorString8(hr));
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pKeyboard, hwnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %s\n", i, DXGetErrorString8(hr));
    }

    if (pKeyboard) IUnknown_Release(pKeyboard);
}

static void keyboard_tests(DWORD version)
{
    HRESULT hr;
    LPDIRECTINPUT pDI = NULL;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwnd;
    ULONG ref = 0;

    hr = DirectInputCreate(hInstance, version, &pDI, NULL);
    if (hr == DIERR_OLDDIRECTINPUTVERSION)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreate() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());

    if (hwnd)
    {
        acquire_tests(pDI, hwnd);
        test_set_coop(pDI, hwnd);
    }

    DestroyWindow(hwnd);
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %d\n", ref);
}

START_TEST(keyboard)
{
    CoInitialize(NULL);

    trace("DLL Version: %s\n", get_file_version("dinput.dll"));

    keyboard_tests(0x0700);

    CoUninitialize();
}
