/*
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

#include "wine/test.h"
#include "windef.h"
#include "dinput.h"
#include "dxerr8.h"

static const DIOBJECTDATAFORMAT obj_data_format[] = {
  { &GUID_YAxis, 16, DIDFT_OPTIONAL|DIDFT_AXIS  |DIDFT_MAKEINSTANCE(1), 0},
  { &GUID_Button,15, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(3), 0},
  { &GUID_Key,    0, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(16),0},
  { &GUID_Key,    1, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(17),0},
  { &GUID_Key,    2, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(18),0},
  { &GUID_Key,    3, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(19),0},
  { &GUID_Key,    4, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(20),0},
  { &GUID_Key,    5, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(21),0},
  { &GUID_Key,    6, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(22),0},
  { &GUID_Key,    7, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(23),0},
  { &GUID_Key,    8, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(24),0},
  { &GUID_Key,    9, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(25),0},
  { &GUID_Key,   10, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(26),0},
  { &GUID_Key,   11, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(27),0},
  { &GUID_Key,   12, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(28),0},
  { NULL,        13, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(5),0},

  { &GUID_Button,14, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(32),0}
};

static const DIDATAFORMAT data_format = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    32,
    sizeof(obj_data_format) / sizeof(obj_data_format[0]),
    (LPDIOBJECTDATAFORMAT)obj_data_format
};

static BOOL CALLBACK enum_callback(LPCDIDEVICEOBJECTINSTANCE oi, LPVOID info)
{
    if (winetest_debug > 1)
        trace(" Type:%4x Ofs:%3d Flags:%08x Name:%s\n",
              oi->dwType, oi->dwOfs, oi->dwFlags, oi->tszName);
    (*(int*)info)++;
    return DIENUM_CONTINUE;
}

static void test_object_info(LPDIRECTINPUTDEVICE device, HWND hwnd)
{
    HRESULT hr;
    DIPROPDWORD dp;
    DIDEVICEOBJECTINSTANCE obj_info;
    int cnt = 0, cnt1 = 0;

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %s\n", DXGetErrorString8(hr));

    hr = IDirectInputDevice_SetDataFormat(device, &data_format);
    ok(SUCCEEDED(hr), "SetDataFormat() failed: %s\n", DXGetErrorString8(hr));

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt1, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %s\n", DXGetErrorString8(hr));
    if (0) /* fails for joystick only */
    ok(cnt == cnt1, "Enum count changed from %d to %d\n", cnt, cnt1);

    /* No need to test devices without axis */
    obj_info.dwSize = sizeof(obj_info);
    hr = IDirectInputDevice_GetObjectInfo(device, &obj_info, 16, DIPH_BYOFFSET);
    if (SUCCEEDED(hr))
    {
        /* No device supports per axis relative/absolute mode */
        memset(&dp, 0, sizeof(dp));
        dp.diph.dwSize = sizeof(DIPROPDWORD);
        dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dp.diph.dwHow = DIPH_BYOFFSET;
        dp.diph.dwObj = 16;
        dp.dwData = DIPROPAXISMODE_ABS;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_UNSUPPORTED, "SetProperty() returned: %s\n", DXGetErrorString8(hr));
        dp.diph.dwHow = DIPH_DEVICE;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_INVALIDPARAM, "SetProperty() returned: %s\n", DXGetErrorString8(hr));
        dp.diph.dwObj = 0;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DI_OK, "SetProperty() failed: %s\n", DXGetErrorString8(hr));

        /* Cannot change mode while acquired */
        hr = IDirectInputDevice_Acquire(device);
        ok(hr == DI_OK, "Acquire() failed: %s\n", DXGetErrorString8(hr));
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_ACQUIRED, "SetProperty() returned: %s\n", DXGetErrorString8(hr));
    }
}

struct enum_data
{
    LPDIRECTINPUT pDI;
    HWND hwnd;
};

static BOOL CALLBACK enum_devices(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    struct enum_data *data = (struct enum_data*)pvRef;
    LPDIRECTINPUTDEVICE device;
    HRESULT hr;

    hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance, &device, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %s\n", DXGetErrorString8(hr));
    if (SUCCEEDED(hr))
    {
        trace("Testing device \"%s\"\n", lpddi->tszInstanceName);
        test_object_info(device, data->hwnd);
        IUnknown_Release(device);
    }
    return DIENUM_CONTINUE;
}

static void device_tests(void)
{
    HRESULT hr;
    LPDIRECTINPUT pDI = NULL;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HWND hwnd;
    struct enum_data data;

    hr = DirectInputCreate(hInstance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (hr == DIERR_OLDDIRECTINPUTVERSION)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreate() failed: %s\n", DXGetErrorString8(hr));
    if (FAILED(hr)) return;

    hwnd = CreateWindow("static", "Title", WS_OVERLAPPEDWINDOW,
                        10, 10, 200, 200, NULL, NULL, NULL, NULL);
    ok(hwnd != NULL, "err: %d\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        data.pDI = pDI;
        data.hwnd = hwnd;
        hr = IDirectInput_EnumDevices(pDI, 0, enum_devices, &data, DIEDFL_ALLDEVICES);
        ok(SUCCEEDED(hr), "IDirectInput_EnumDevices() failed: %s\n", DXGetErrorString8(hr));

        DestroyWindow(hwnd);
    }
    if (pDI) IUnknown_Release(pDI);
}

START_TEST(device)
{
    CoInitialize(NULL);

    device_tests();

    CoUninitialize();
}
