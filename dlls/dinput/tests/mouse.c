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

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"

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

static const HRESULT SetCoop_child_window[16] =  {
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_HANDLE,     E_HANDLE,     E_INVALIDARG,
    E_INVALIDARG, E_INVALIDARG, E_INVALIDARG, E_INVALIDARG};

static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
        diff = time - GetTickCount();
    }
}

static void test_set_coop(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pMouse = NULL;
    int i;
    HWND child;

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);
    if (FAILED(hr)) return;

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, NULL, i);
        ok(hr == SetCoop_null_window[i], "SetCooperativeLevel(NULL, %d): %#lx\n", i, hr);
    }
    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, i);
        ok(hr == SetCoop_real_window[i], "SetCooperativeLevel(hwnd, %d): %#lx\n", i, hr);
    }

    child = CreateWindowA("static", "Title", WS_CHILD | WS_VISIBLE, 10, 10, 50, 50, hwnd, NULL,
                          NULL, NULL);
    ok(child != NULL, "err: %lu\n", GetLastError());

    for (i=0; i<16; i++)
    {
        hr = IDirectInputDevice_SetCooperativeLevel(pMouse, child, i);
        ok(hr == SetCoop_child_window[i], "SetCooperativeLevel(child, %d): %#lx\n", i, hr);
    }

    DestroyWindow(child);
    if (pMouse) IUnknown_Release(pMouse);
}

static void test_acquire(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    IDirectInputDeviceA *pMouse = NULL;
    DIMOUSESTATE m_state;
    HWND hwnd2;
    DIPROPDWORD di_op;
    DIDEVICEOBJECTDATA mouse_state;
    DWORD cnt;
    int i;

    if (! SetForegroundWindow(hwnd))
    {
        skip("Not running as foreground app, skipping acquire tests\n");
        return;
    }

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysMouse, &pMouse, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInputDevice_SetCooperativeLevel(pMouse, hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
    ok(hr == S_OK, "SetCooperativeLevel: %#lx\n", hr);

    memset(&di_op, 0, sizeof(di_op));
    di_op.dwData = 5;
    di_op.diph.dwHow = DIPH_DEVICE;
    di_op.diph.dwSize = sizeof(DIPROPDWORD);
    di_op.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    hr = IDirectInputDevice_SetProperty(pMouse, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&di_op);
    ok(hr == S_OK, "SetProperty() failed: %#lx\n", hr);

    hr = IDirectInputDevice_SetDataFormat(pMouse, &c_dfDIMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %#lx\n", hr);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Unacquire() should have failed: %#lx\n", hr);
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %#lx\n", hr);
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_FALSE, "IDirectInputDevice_Acquire() should have failed: %#lx\n", hr);

    /* Foreground coop level requires window to have focus */
    /* Create a temporary window, this should make dinput
     * lose mouse input */
    hwnd2 = CreateWindowA("static", "Temporary", WS_VISIBLE, 10, 210, 200, 200, NULL, NULL, NULL,
                          NULL);
    ok(hwnd2 != NULL, "CreateWindowA failed with %lu\n", GetLastError());
    flush_events();

    hr = IDirectInputDevice_GetDeviceState(pMouse, sizeof(m_state), &m_state);
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceState() should have failed: %#lx\n", hr);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == DIERR_OTHERAPPHASPRIO, "Acquire() should have failed: %#lx\n", hr);

    SetActiveWindow( hwnd );
    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Acquire() failed: %#lx\n", hr);

    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %#lx cnt:%lu\n", hr, cnt);

    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %#lx cnt:%lu\n", hr, cnt);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    mouse_event(MOUSEEVENTF_MOVE, 10, 10, 0, 0);
    hr = IDirectInputDevice_Unacquire(pMouse);
    ok(hr == S_OK, "Failed: %#lx\n", hr);

    hr = IDirectInputDevice_Acquire(pMouse);
    ok(hr == S_OK, "Failed: %#lx\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == S_OK && cnt > 0, "GetDeviceData() failed: %#lx cnt:%lu\n", hr, cnt);

    /* Check for buffer overflow */
    for (i = 0; i < 6; i++)
        mouse_event(MOUSEEVENTF_MOVE, 10 + i, 10 + i, 0, 0);

    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %#lx cnt:%lu\n", hr, cnt);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(pMouse, sizeof(mouse_state), &mouse_state, &cnt, 0);
    ok(hr == DI_OK && cnt == 1, "GetDeviceData() failed: %#lx cnt:%lu\n", hr, cnt);

    IUnknown_Release(pMouse);

    DestroyWindow( hwnd2 );
}

static void mouse_tests(void)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL;
    HWND hwnd;
    ULONG ref = 0;

    hr = DirectInputCreateA(instance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (hr == DIERR_OLDDIRECTINPUTVERSION)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreateA() failed: %#lx\n", hr);
    if (FAILED(hr)) return;

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
                         NULL, NULL);
    ok(hwnd != NULL, "err: %lu\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        test_set_coop(pDI, hwnd);
        test_acquire(pDI, hwnd);

        DestroyWindow(hwnd);
    }
    if (pDI) ref = IUnknown_Release(pDI);
    ok(!ref, "IDirectInput_Release() reference count = %lu\n", ref);
}

START_TEST(mouse)
{
    dinput_test_init();

    mouse_tests();

    dinput_test_exit();
}
