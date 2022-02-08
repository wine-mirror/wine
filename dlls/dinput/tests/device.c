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

#include <limits.h>

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
    ARRAY_SIZE(obj_data_format),
    (LPDIOBJECTDATAFORMAT)obj_data_format
};

static BOOL CALLBACK enum_callback(const DIDEVICEOBJECTINSTANCEA *oi, void *info)
{
    if (winetest_debug > 1)
        trace(" Type:%#lx Ofs:%3ld Flags:%#lx Name:%s\n",
              oi->dwType, oi->dwOfs, oi->dwFlags, oi->tszName);
    (*(int*)info)++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK enum_type_callback(const DIDEVICEOBJECTINSTANCEA *oi, void *info)
{
    DWORD expected = *(DWORD*)info;
    ok (expected & DIDFT_GETTYPE(oi->dwType), "EnumObjects() enumerated wrong type for obj %s, expected: %#lx got: %#lx\n", oi->tszName, expected, oi->dwType);
    return DIENUM_CONTINUE;
}

static void test_object_info(IDirectInputDeviceA *device, HWND hwnd)
{
    HRESULT hr;
    DIPROPDWORD dp;
    DIDEVICEOBJECTINSTANCEA obj_info;
    DWORD obj_types[] = {DIDFT_BUTTON, DIDFT_AXIS, DIDFT_POV};
    int type_index;
    int cnt1 = 0;
    DWORD cnt = 0;
    DIDEVICEOBJECTDATA buffer[5];

    hr = IDirectInputDevice_EnumObjects(device, NULL, &cnt, DIDFT_ALL);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_EnumObjects returned %#lx, expected %#lx\n", hr, DIERR_INVALIDPARAM);

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %#lx\n", hr);

    hr = IDirectInputDevice_SetDataFormat(device, &data_format);
    ok(SUCCEEDED(hr), "SetDataFormat() failed: %#lx\n", hr);

    hr = IDirectInputDevice_EnumObjects(device, enum_callback, &cnt1, DIDFT_ALL);
    ok(SUCCEEDED(hr), "EnumObjects() failed: %#lx\n", hr);
    if (0) /* fails for joystick only */
    ok(cnt == cnt1, "Enum count changed from %lu to %u\n", cnt, cnt1);

    /* Testing EnumObjects with different types of device objects */
    for (type_index=0; type_index < ARRAY_SIZE(obj_types); type_index++)
    {
        hr = IDirectInputDevice_EnumObjects(device, enum_type_callback, &obj_types[type_index], obj_types[type_index]);
        ok(SUCCEEDED(hr), "EnumObjects() failed: %#lx\n", hr);
    }

    /* Test buffered mode */
    memset(&dp, 0, sizeof(dp));
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow = DIPH_DEVICE;
    dp.diph.dwObj = 0;
    dp.dwData = UINT_MAX;

    hr = IDirectInputDevice_GetProperty(device, DIPROP_BUFFERSIZE, &dp.diph);
    ok(hr == DI_OK, "Failed: %#lx\n", hr);
    ok(dp.dwData == 0, "got %ld\n", dp.dwData);

    dp.dwData = UINT_MAX;
    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %#lx\n", hr);

    dp.dwData = 0;
    hr = IDirectInputDevice_GetProperty(device, DIPROP_BUFFERSIZE, &dp.diph);
    ok(hr == DI_OK, "Failed: %#lx\n", hr);
    ok(dp.dwData == UINT_MAX, "got %ld\n", dp.dwData);

    dp.dwData = 0;
    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %#lx\n", hr);
    cnt = 5;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK && cnt == 5, "GetDeviceData() failed: %#lx cnt: %ld\n", hr, cnt);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTBUFFERED, "GetDeviceData() should have failed: %#lx\n", hr);
    IDirectInputDevice_Acquire(device);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTBUFFERED, "GetDeviceData() should have failed: %#lx\n", hr);
    IDirectInputDevice_Unacquire(device);

    dp.dwData = 20;
    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %#lx\n", hr);
    cnt = 5;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %#lx\n", hr);
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), buffer, &cnt, 0);
    ok(hr == DIERR_NOTACQUIRED, "GetDeviceData() should have failed: %#lx\n", hr);
    hr = IDirectInputDevice_Acquire(device);
    ok(hr == DI_OK, "Acquire() failed: %#lx\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %#lx\n", hr);
    hr = IDirectInputDevice_Unacquire(device);
    ok(hr == DI_OK, "Unacquire() failed: %#lx\n", hr);
    cnt = 1;
    hr = IDirectInputDevice_GetDeviceData(device, sizeof(buffer[0]), buffer, &cnt, 0);
    ok(hr == DI_OK, "GetDeviceData() failed: %#lx\n", hr);

    hr = IDirectInputDevice_GetObjectInfo(device, NULL, 16, DIPH_BYOFFSET);
    ok(hr == E_POINTER, "IDirectInputDevice_GetObjectInfo returned %#lx, expected %#lx\n", hr, E_POINTER);

    obj_info.dwSize = 1;
    hr = IDirectInputDevice_GetObjectInfo(device, &obj_info, 16, DIPH_BYOFFSET);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetObjectInfo returned %#lx, expected %#lx\n", hr, DIERR_INVALIDPARAM);
    obj_info.dwSize = 0xdeadbeef;
    hr = IDirectInputDevice_GetObjectInfo(device, &obj_info, 16, DIPH_BYOFFSET);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInputDevice_GetObjectInfo returned %#lx, expected %#lx\n", hr, DIERR_INVALIDPARAM);

    /* No need to test devices without axis */
    obj_info.dwSize = sizeof(obj_info);
    hr = IDirectInputDevice_GetObjectInfo(device, &obj_info, 16, DIPH_BYOFFSET);
    if (SUCCEEDED(hr))
    {
        /* No device supports per axis relative/absolute mode */
        dp.diph.dwHow = DIPH_BYOFFSET;
        dp.diph.dwObj = 16;
        dp.dwData = DIPROPAXISMODE_ABS;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_UNSUPPORTED, "SetProperty() returned: %#lx\n", hr);
        dp.diph.dwHow = DIPH_DEVICE;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_INVALIDPARAM, "SetProperty() returned: %#lx\n", hr);
        dp.diph.dwObj = 0;
        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DI_OK, "SetProperty() failed: %#lx\n", hr);

        /* Cannot change mode while acquired */
        hr = IDirectInputDevice_Acquire(device);
        ok(hr == DI_OK, "Acquire() failed: %#lx\n", hr);

        hr = IDirectInputDevice_SetProperty(device, DIPROP_AXISMODE, &dp.diph);
        ok(hr == DIERR_ACQUIRED, "SetProperty() returned: %#lx\n", hr);
        hr = IDirectInputDevice_Unacquire(device);
        ok(hr == DI_OK, "Unacquire() failed: %#lx\n", hr);
    }

    /* Reset buffer size */
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow = DIPH_DEVICE;
    dp.diph.dwObj = 0;
    dp.dwData = 0;
    hr = IDirectInputDevice_SetProperty(device, DIPROP_BUFFERSIZE, (LPCDIPROPHEADER)&dp.diph);
    ok(hr == DI_OK, "SetProperty() failed: %#lx\n", hr);
}

struct enum_data
{
    IDirectInputA *pDI;
    HWND hwnd;
    BOOL tested_product_creation;
};

static BOOL CALLBACK enum_devices(const DIDEVICEINSTANCEA *lpddi, void *pvRef)
{
    struct enum_data *data = pvRef;
    IDirectInputDeviceA *device, *obj = NULL;
    DIDEVICEINSTANCEA ddi2;
    HRESULT hr;
    IUnknown *iface, *tmp_iface;

    hr = IDirectInput_GetDeviceStatus(data->pDI, &lpddi->guidInstance);
    ok(hr == DI_OK, "IDirectInput_GetDeviceStatus() failed: %#lx\n", hr);

    if (hr == DI_OK)
    {
        hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidInstance, &device, NULL);
        ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);
        trace("Testing device %p \"%s\"\n", device, lpddi->tszInstanceName);

        hr = IUnknown_QueryInterface(device, &IID_IDirectInputDevice2A, (LPVOID*)&obj);
        ok(SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice2A) failed: %#lx\n", hr);
        test_object_info(obj, data->hwnd);
        IUnknown_Release(obj);
        obj = NULL;

        hr = IUnknown_QueryInterface(device, &IID_IDirectInputDevice2W, (LPVOID*)&obj);
        ok(SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice2W) failed: %#lx\n", hr);
        test_object_info(obj, data->hwnd);
        IUnknown_Release(obj);

        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDeviceA, (void **)&iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDeviceA) failed: %#lx\n", hr );
        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDevice2A, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice2A) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IDirectInputDevice2A iface differs from IDirectInputDeviceA\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDevice7A, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice7A) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IDirectInputDevice7A iface differs from IDirectInputDeviceA\n" );
        IUnknown_Release( tmp_iface );
        IUnknown_Release( iface );

        hr = IUnknown_QueryInterface( device, &IID_IUnknown, (void **)&iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IUnknown) failed: %#lx\n", hr );
        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDeviceW, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDeviceW) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IDirectInputDeviceW iface differs from IUnknown\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDevice2W, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice2W) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IDirectInputDevice2W iface differs from IUnknown\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( device, &IID_IDirectInputDevice7W, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputDevice7W) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IDirectInputDevice7W iface differs from IUnknown\n" );
        IUnknown_Release( tmp_iface );
        IUnknown_Release( iface );

        IUnknown_Release(device);

        if (!IsEqualGUID(&lpddi->guidInstance, &lpddi->guidProduct))
        {
            data->tested_product_creation = TRUE;
            hr = IDirectInput_CreateDevice(data->pDI, &lpddi->guidProduct, &device, NULL);
            ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);

            ddi2.dwSize = sizeof(ddi2);
            hr = IDirectInputDevice_GetDeviceInfo(device, &ddi2);
            ok(SUCCEEDED(hr), "IDirectInput_GetDeviceInfo failed: %#lx\n", hr);

            ok(IsEqualGUID(&lpddi->guidProduct, &ddi2.guidProduct), "Product GUIDs do not match. Expected %s, got %s\n", debugstr_guid(&lpddi->guidProduct), debugstr_guid(&ddi2.guidProduct));
            ok(IsEqualGUID(&ddi2.guidProduct, &ddi2.guidInstance), "Instance GUID should equal product GUID. Expected %s, got %s\n", debugstr_guid(&ddi2.guidProduct), debugstr_guid(&ddi2.guidInstance));
            /* we cannot compare guidInstances as we may get a different device */

            IUnknown_Release(device);
        }

    }
    return DIENUM_CONTINUE;
}

struct overlapped_state
{
        BYTE  keys[4];
        DWORD extra_element;
};

static const DIOBJECTDATAFORMAT obj_overlapped_slider_format[] = {
    { &GUID_Key,    0, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_A),0},
    { &GUID_Key,    1, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_S),0},
    { &GUID_Key,    2, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_D),0},
    { &GUID_Key,    3, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_F),0},
    { &GUID_Slider, 0, DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,DIDOI_ASPECTPOSITION},
};

static const DIDATAFORMAT overlapped_slider_format = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(struct overlapped_state),
    ARRAY_SIZE(obj_overlapped_slider_format),
    (LPDIOBJECTDATAFORMAT)obj_overlapped_slider_format
};

static const DIOBJECTDATAFORMAT obj_overlapped_pov_format[] = {
    { &GUID_Key,    0, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_A),0},
    { &GUID_Key,    1, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_S),0},
    { &GUID_Key,    2, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_D),0},
    { &GUID_Key,    3, DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_MAKEINSTANCE(DIK_F),0},
    { &GUID_POV,    0, DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE,0},
};

static const DIDATAFORMAT overlapped_pov_format = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(struct overlapped_state),
    ARRAY_SIZE(obj_overlapped_pov_format),
    (LPDIOBJECTDATAFORMAT)obj_overlapped_pov_format
};

static void pump_messages(void)
{
    MSG msg;

    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

#define wait_for_device_data_and_discard(device) wait_for_device_data_and_discard_(__LINE__, device)
static BOOL wait_for_device_data_and_discard_(int line, IDirectInputDeviceA *device)
{
    DWORD cnt;
    HRESULT hr;
    DWORD start_time;

    pump_messages();

    start_time = GetTickCount();
    do
    {
        cnt = 10;
        hr = IDirectInputDevice_GetDeviceData(device, sizeof(DIDEVICEOBJECTDATA_DX3), NULL, &cnt, 0);
        ok_(__FILE__, line)(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceData() failed: %#lx\n", hr);
        ok_(__FILE__, line)(cnt == 0 || cnt == 1, "Unexpected number of events: %lu\n", cnt);
    } while (cnt != 1 && (GetTickCount() - start_time < 500));

    return cnt == 1;
}

#define acquire_and_wait(device, valid_dik) acquire_and_wait_(__LINE__, device, valid_dik)
static void acquire_and_wait_(int line, IDirectInputDeviceA *device, DWORD valid_dik)
{
    HRESULT hr;
    int tries = 2;

    hr = IDirectInputDevice_Acquire(device);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Acquire() failed: %#lx\n", hr);

    do
    {
        keybd_event(0, valid_dik, KEYEVENTF_SCANCODE, 0);
    } while (!wait_for_device_data_and_discard(device) && tries--);

    keybd_event(0, valid_dik, KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP, 0);
    ok_(__FILE__, line)(wait_for_device_data_and_discard(device),
                        "Timed out while waiting for injected events to be picked up by DirectInput.\n");
}

void overlapped_format_tests(IDirectInputA *pDI, HWND hwnd)
{
    HRESULT hr;
    struct overlapped_state state;
    IDirectInputDeviceA *keyboard = NULL;
    DIPROPDWORD dp;

    SetFocus(hwnd);

    hr = IDirectInput_CreateDevice(pDI, &GUID_SysKeyboard, &keyboard, NULL);
    ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);

    /* test overlapped slider - default value 0 */
    hr = IDirectInputDevice_SetDataFormat(keyboard, &overlapped_slider_format);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %#lx\n", hr);

    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwHow = DIPH_DEVICE;
    dp.diph.dwObj = 0;
    dp.dwData = 10;
    hr = IDirectInputDevice_SetProperty(keyboard, DIPROP_BUFFERSIZE, &dp.diph);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetProperty() failed: %#lx\n", hr);

    acquire_and_wait(keyboard, DIK_F);

    /* press D */
    keybd_event(0, DIK_D, KEYEVENTF_SCANCODE, 0);
    ok(wait_for_device_data_and_discard(keyboard),
       "Timed out while waiting for injected events to be picked up by DirectInput.\n");

    memset(&state, 0xFF, sizeof(state));
    hr = IDirectInputDevice_GetDeviceState(keyboard, sizeof(state), &state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %#lx\n", hr);

    ok(state.keys[0] == 0x00, "key A should be still up\n");
    ok(state.keys[1] == 0x00, "key S should be still up\n");
    ok(state.keys[2] == 0x80, "keydown for D did not register\n");
    ok(state.keys[3] == 0x00, "key F should be still up\n");
    ok(state.extra_element == 0, "State struct was not memset to zero\n");

    /* release D */
    keybd_event(0, DIK_D, KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP, 0);
    ok(wait_for_device_data_and_discard(keyboard),
            "Timed out while waiting for injected events to be picked up by DirectInput.\n");

    hr = IDirectInputDevice_Unacquire(keyboard);
    ok(SUCCEEDED(hr), "IDirectInputDevice_Unacquire() failed: %#lx\n", hr);

    /* test overlapped pov - default value - 0xFFFFFFFF */
    hr = IDirectInputDevice_SetDataFormat(keyboard, &overlapped_pov_format);
    ok(SUCCEEDED(hr), "IDirectInputDevice_SetDataFormat() failed: %#lx\n", hr);

    acquire_and_wait(keyboard, DIK_F);

    /* press D */
    keybd_event(0, DIK_D, KEYEVENTF_SCANCODE, 0);
    ok(wait_for_device_data_and_discard(keyboard),
       "Timed out while waiting for injected events to be picked up by DirectInput.\n");

    memset(&state, 0xFF, sizeof(state));
    hr = IDirectInputDevice_GetDeviceState(keyboard, sizeof(state), &state);
    ok(SUCCEEDED(hr), "IDirectInputDevice_GetDeviceState() failed: %#lx\n", hr);

    ok(state.keys[0] == 0xFF, "key state should have been overwritten by the overlapped POV\n");
    ok(state.keys[1] == 0xFF, "key state should have been overwritten by the overlapped POV\n");
    ok(state.keys[2] == 0xFF, "key state should have been overwritten by the overlapped POV\n");
    ok(state.keys[3] == 0xFF, "key state should have been overwritten by the overlapped POV\n");
    ok(state.extra_element == 0, "State struct was not memset to zero\n");

    /* release D */
    keybd_event(0, DIK_D, KEYEVENTF_SCANCODE|KEYEVENTF_KEYUP, 0);
    ok(wait_for_device_data_and_discard(keyboard),
            "Timed out while waiting for injected events to be picked up by DirectInput.\n");

    if (keyboard) IUnknown_Release(keyboard);
}

static void device_tests(void)
{
    HRESULT hr;
    IDirectInputA *pDI = NULL, *obj = NULL;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    HWND hwnd;
    struct enum_data data;

    hr = CoCreateInstance(&CLSID_DirectInput, 0, 1, &IID_IDirectInput2A, (LPVOID*)&pDI);
    if (hr == DIERR_OLDDIRECTINPUTVERSION || hr == DIERR_DEVICENOTREG)
    {
        skip("Tests require a newer dinput version\n");
        return;
    }
    ok(SUCCEEDED(hr), "DirectInputCreateA() failed: %#lx\n", hr);
    if (FAILED(hr)) return;

    hr = IDirectInput_Initialize(pDI, hInstance, DIRECTINPUT_VERSION);
    ok(SUCCEEDED(hr), "Initialize() failed: %#lx\n", hr);
    if (FAILED(hr)) return;

    hr = IUnknown_QueryInterface(pDI, &IID_IDirectInput2W, (LPVOID*)&obj);
    ok(SUCCEEDED(hr), "QueryInterface(IDirectInput7W) failed: %#lx\n", hr);

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
                         NULL, NULL);
    ok(hwnd != NULL, "err: %ld\n", GetLastError());
    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOW);

        data.pDI = pDI;
        data.hwnd = hwnd;
        data.tested_product_creation = FALSE;
        hr = IDirectInput_EnumDevices(pDI, 0, enum_devices, &data, DIEDFL_ALLDEVICES);
        ok(SUCCEEDED(hr), "IDirectInput_EnumDevices() failed: %#lx\n", hr);

        if (!data.tested_product_creation) winetest_skip("Device creation using product GUID not tested\n");

        /* If GetDeviceStatus returns DI_OK the device must exist */
        hr = IDirectInput_GetDeviceStatus(pDI, &GUID_Joystick);
        if (hr == DI_OK)
        {
            IDirectInputDeviceA *device = NULL;

            hr = IDirectInput_CreateDevice(pDI, &GUID_Joystick, &device, NULL);
            ok(SUCCEEDED(hr), "IDirectInput_CreateDevice() failed: %#lx\n", hr);
            if (device) IUnknown_Release(device);
        }

        overlapped_format_tests(pDI, hwnd);

        DestroyWindow(hwnd);
    }
    if (obj) IUnknown_Release(obj);
    if (pDI) IUnknown_Release(pDI);
}

START_TEST(device)
{
    CoInitialize(NULL);

    device_tests();

    CoUninitialize();
}
