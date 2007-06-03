/*
 * Copyright (c) 2005 Robert Reif
 * Copyright (c) 2006 Vitaliy Margolen
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
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "wingdi.h"
#include "dinput.h"
#include "dxerr8.h"
#include "dinput_test.h"

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
    LPDIRECTINPUTDEVICE pMouse = NULL;
    int i;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %s\n", i, DXGetErrorString8(hr));
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %s\n", i, DXGetErrorString8(hr));
    }

    if (pMouse) IUnknown_Release(pMouse);
}

static void test_acquire(LPDIRECTINPUT pDI, HWND hwnd)
{
    HRESULT hr;
    LPDIRECTINPUTDEVICE pMouse = NULL;
    DIMOUSESTATE2 m_state;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    ok(hr == S_OK, "SetCooperativeLevel: %s\n", DXGetErrorString8(hr));

    hr = IDirectInputDevice_SetDataFormat(pMouse, &c_dfDIMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %s\n", DXGetErrorString8(hr));
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have failed: %s\n", DXGetErrorString8(hr));

    /* Foreground coop level requires window to have focus */
    /* This should make dinput loose mouse input */
    SetActiveWindow( 0 );

    hr = IDirectInputDevice_GetDeviceState(pMouse, sizeof(m_state), &m_state);
    todo_wine
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceState() should have failed: %s\n", DXGetErrorString8(hr));
    /* Workaround so we can test other things. Remove when Wine is fixed */
    IDirectInputDevice_Unacquire(pMouse);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == DIERR_OTHERAPPHASPRIO, "Acquire() should have failed: %s\n", DXGetErrorString8(hr));

    SetActiveWindow( hwnd );
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Acquire() failed: %s\n", DXGetErrorString8(hr));

    if (pMouse) IUnknown_Release(pMouse);
}

static void mouse_tests(void)
{
    HRESULT hr;
    LPDIRECTINPUT pDI = NULL;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwnd;
    ULONG ref = 0;

    hr = DirectInputCreate(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    ok(SUCCEEDED(hr), "DirectInputCreate() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        test_set_coop(pDI, hwnd);
        test_acquire(pDI, hwnd);

        DestroyWindow(hwnd);
    }
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %d\n", ref);
}

START_TEST(mouse)
{
    CoInitialize(NULL);

    trace("DLL Version: %s\n", get_file_version("dinput.dll"));

    mouse_tests();

    CoUninitialize();
}
