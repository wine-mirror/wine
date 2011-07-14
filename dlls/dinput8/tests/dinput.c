/*
 * Copyright (c) 2011 Andrew Nguyen
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

#define DIRECTINPUT_VERSION 0x0800

#define COBJMACROS
#include <initguid.h>
#include <windows.h>
#include <dinput.h>

#include "wine/test.h"

HINSTANCE hInstance;

static BOOL CALLBACK dummy_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    ok(0, "Callback was invoked with parameters (%p, %p)\n", instance, context);
    return DIENUM_STOP;
}

static void test_preinitialization(void)
{
    static const struct
    {
        REFGUID rguid;
        BOOL pdev;
        HRESULT expected_hr;
    } create_device_tests[] =
    {
        {NULL, FALSE, E_POINTER},
        {NULL, TRUE, E_POINTER},
        {&GUID_Unknown, FALSE, E_POINTER},
        {&GUID_Unknown, TRUE, DIERR_NOTINITIALIZED},
        {&GUID_SysMouse, FALSE, E_POINTER},
        {&GUID_SysMouse, TRUE, DIERR_NOTINITIALIZED},
    };

    static const struct
    {
        DWORD dwDevType;
        LPDIENUMDEVICESCALLBACKA lpCallback;
        DWORD dwFlags;
        HRESULT expected_hr;
        int todo;
    } enum_devices_tests[] =
    {
        {0, NULL, 0, DIERR_INVALIDPARAM},
        {0, NULL, ~0u, DIERR_INVALIDPARAM},
        {0, dummy_callback, 0, DIERR_NOTINITIALIZED},
        {0, dummy_callback, ~0u, DIERR_INVALIDPARAM, 1},
        {0xdeadbeef, NULL, 0, DIERR_INVALIDPARAM},
        {0xdeadbeef, NULL, ~0u, DIERR_INVALIDPARAM},
        {0xdeadbeef, dummy_callback, 0, DIERR_INVALIDPARAM, 1},
        {0xdeadbeef, dummy_callback, ~0u, DIERR_INVALIDPARAM, 1},
    };

    IDirectInput8A *pDI;
    HRESULT hr;
    int i;
    IDirectInputDevice8A *pDID;

    hr = CoCreateInstance(&CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectInput8A, (void **)&pDI);
    if (FAILED(hr))
    {
        skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    for (i = 0; i < sizeof(create_device_tests)/sizeof(create_device_tests[0]); i++)
    {
        if (create_device_tests[i].pdev) pDID = (void *)0xdeadbeef;
        hr = IDirectInput8_CreateDevice(pDI, create_device_tests[i].rguid,
                                            create_device_tests[i].pdev ? &pDID : NULL,
                                            NULL);
        ok(hr == create_device_tests[i].expected_hr, "[%d] IDirectInput8_CreateDevice returned 0x%08x\n", i, hr);
        if (create_device_tests[i].pdev)
            ok(pDID == NULL, "[%d] Output interface pointer is %p\n", i, pDID);
    }

    for (i = 0; i < sizeof(enum_devices_tests)/sizeof(enum_devices_tests[0]); i++)
    {
        hr = IDirectInput8_EnumDevices(pDI, enum_devices_tests[i].dwDevType,
                                           enum_devices_tests[i].lpCallback,
                                           NULL,
                                           enum_devices_tests[i].dwFlags);
        if (enum_devices_tests[i].todo)
        {
            todo_wine
            ok(hr == enum_devices_tests[i].expected_hr, "[%d] IDirectInput8_EnumDevice returned 0x%08x\n", i, hr);
        }
        else
            ok(hr == enum_devices_tests[i].expected_hr, "[%d] IDirectInput8_EnumDevice returned 0x%08x\n", i, hr);
    }

    hr = IDirectInput8_GetDeviceStatus(pDI, NULL);
    todo_wine
    ok(hr == E_POINTER, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_Unknown);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, NULL, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_DirectInput8Create(void)
{
    static const struct
    {
        BOOL hinst;
        DWORD dwVersion;
        REFIID riid;
        BOOL ppdi;
        HRESULT expected_hr;
    } invalid_param_list[] =
    {
        {FALSE, 0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, 0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, 0,                       &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION,     &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {FALSE, DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_INVALIDPARAM},
        {TRUE,  0,                       &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  0,                       &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  0,                       &IID_IDirectInput8A, TRUE,  DIERR_NOTINITIALIZED},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION,     &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION - 1, &IID_IDirectInput8A, TRUE,  DIERR_BETADIRECTINPUTVERSION},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInputA,  TRUE,  DIERR_NOINTERFACE},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, FALSE, E_POINTER},
        {TRUE,  DIRECTINPUT_VERSION + 1, &IID_IDirectInput8A, TRUE,  DIERR_OLDDIRECTINPUTVERSION},
    };

    static REFIID no_interface_list[] = {&IID_IDirectInputA, &IID_IDirectInputW,
                                         &IID_IDirectInput2A, &IID_IDirectInput2W,
                                         &IID_IDirectInput7A, &IID_IDirectInput7W,
                                         &IID_IDirectInputDeviceA, &IID_IDirectInputDeviceW,
                                         &IID_IDirectInputDevice2A, &IID_IDirectInputDevice2W,
                                         &IID_IDirectInputDevice7A, &IID_IDirectInputDevice7W,
                                         &IID_IDirectInputDevice8A, &IID_IDirectInputDevice8W,
                                         &IID_IDirectInputEffect};

    static REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInput8A, &IID_IDirectInput8W};

    int i;
    IUnknown *pUnk;
    HRESULT hr;

    for (i = 0; i < sizeof(invalid_param_list)/sizeof(invalid_param_list[0]); i++)
    {
        if (invalid_param_list[i].ppdi) pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(invalid_param_list[i].hinst ? hInstance : NULL,
                                invalid_param_list[i].dwVersion,
                                invalid_param_list[i].riid,
                                invalid_param_list[i].ppdi ? (void **)&pUnk : NULL,
                                NULL);
        ok(hr == invalid_param_list[i].expected_hr, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        if (invalid_param_list[i].ppdi)
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < sizeof(no_interface_list)/sizeof(no_interface_list[0]); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, no_interface_list[i], (void **)&pUnk, NULL);
        ok(hr == DIERR_NOINTERFACE, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
    }

    for (i = 0; i < sizeof(iid_list)/sizeof(iid_list[0]); i++)
    {
        pUnk = NULL;
        hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, iid_list[i], (void **)&pUnk, NULL);
        ok(hr == DI_OK, "[%d] DirectInput8Create returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk)
            IUnknown_Release(pUnk);
    }
}

static void test_QueryInterface(void)
{
    static REFIID iid_list[] = {&IID_IUnknown, &IID_IDirectInput8A, &IID_IDirectInput8W};

    static const struct
    {
        REFIID riid;
        int test_todo;
    } no_interface_list[] =
    {
        {&IID_IDirectInputA, 1},
        {&IID_IDirectInputW, 1},
        {&IID_IDirectInput2A, 1},
        {&IID_IDirectInput2W, 1},
        {&IID_IDirectInput7A, 1},
        {&IID_IDirectInput7W, 1},
        {&IID_IDirectInputDeviceA},
        {&IID_IDirectInputDeviceW},
        {&IID_IDirectInputDevice2A},
        {&IID_IDirectInputDevice2W},
        {&IID_IDirectInputDevice7A},
        {&IID_IDirectInputDevice7W},
        {&IID_IDirectInputDevice8A},
        {&IID_IDirectInputDevice8W},
        {&IID_IDirectInputEffect},
    };

    IDirectInput8A *pDI;
    HRESULT hr;
    IUnknown *pUnk;
    int i;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_QueryInterface(pDI, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);

    pUnk = (void *)0xdeadbeef;
    hr = IDirectInput8_QueryInterface(pDI, NULL, (void **)&pUnk);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);
    ok(pUnk == (void *)0xdeadbeef, "Output interface pointer is %p\n", pUnk);

    hr = IDirectInput8_QueryInterface(pDI, &IID_IUnknown, NULL);
    ok(hr == E_POINTER, "IDirectInput8_QueryInterface returned 0x%08x\n", hr);

    for (i = 0; i < sizeof(iid_list)/sizeof(iid_list[0]); i++)
    {
        pUnk = NULL;
        hr = IDirectInput8_QueryInterface(pDI, iid_list[i], (void **)&pUnk);
        ok(hr == S_OK, "[%d] IDirectInput8_QueryInterface returned 0x%08x\n", i, hr);
        ok(pUnk != NULL, "[%d] Output interface pointer is NULL\n", i);
        if (pUnk) IUnknown_Release(pUnk);
    }

    for (i = 0; i < sizeof(no_interface_list)/sizeof(no_interface_list[0]); i++)
    {
        pUnk = (void *)0xdeadbeef;
        hr = IDirectInput8_QueryInterface(pDI, no_interface_list[i].riid, (void **)&pUnk);
        if (no_interface_list[i].test_todo)
        {
            todo_wine
            ok(hr == E_NOINTERFACE, "[%d] IDirectInput8_QueryInterface returned 0x%08x\n", i, hr);
            todo_wine
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);

            if (pUnk) IUnknown_Release(pUnk);
        }
        else
        {
            ok(hr == E_NOINTERFACE, "[%d] IDirectInput8_QueryInterface returned 0x%08x\n", i, hr);
            ok(pUnk == NULL, "[%d] Output interface pointer is %p\n", i, pUnk);
        }
    }

    IDirectInput8_Release(pDI);
}

static void test_CreateDevice(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;
    IDirectInputDevice8A *pDID;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_CreateDevice(pDI, NULL, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput8_CreateDevice(pDI, NULL, &pDID, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_Unknown, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    pDID = (void *)0xdeadbeef;
    hr = IDirectInput8_CreateDevice(pDI, &GUID_Unknown, &pDID, NULL);
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);
    ok(pDID == NULL, "Output interface pointer is %p\n", pDID);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_SysMouse, NULL, NULL);
    ok(hr == E_POINTER, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    hr = IDirectInput8_CreateDevice(pDI, &GUID_SysMouse, &pDID, NULL);
    ok(hr == DI_OK, "IDirectInput8_CreateDevice returned 0x%08x\n", hr);

    IDirectInputDevice_Release(pDID);
    IDirectInput8_Release(pDI);
}

struct enum_devices_test
{
    unsigned int device_count;
    BOOL return_value;
};

static BOOL CALLBACK enum_devices_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    struct enum_devices_test *enum_test = context;

    enum_test->device_count++;
    return enum_test->return_value;
}

static void test_EnumDevices(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;
    struct enum_devices_test enum_test, enum_test_return;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_EnumDevices(pDI, 0, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    /* Test crashes on Wine. */
    if (0)
    {
        hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, NULL, ~0u);
        ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    }

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, NULL, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, 0);
    todo_wine
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    hr = IDirectInput8_EnumDevices(pDI, 0xdeadbeef, enum_devices_callback, NULL, ~0u);
    todo_wine
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_CONTINUE;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count != 0, "Device count is %u\n", enum_test.device_count);

    /* Enumeration only stops with an explicit DIENUM_STOP. */
    enum_test_return.device_count = 0;
    enum_test_return.return_value = 42;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test_return, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test_return.device_count == enum_test.device_count,
       "Device count is %u vs. %u\n", enum_test_return.device_count, enum_test.device_count);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_STOP;
    hr = IDirectInput8_EnumDevices(pDI, 0, enum_devices_callback, &enum_test, 0);
    ok(hr == DI_OK, "IDirectInput8_EnumDevices returned 0x%08x\n", hr);
    ok(enum_test.device_count == 1, "Device count is %u\n", enum_test.device_count);

    IDirectInput8_Release(pDI);
}

static void test_GetDeviceStatus(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_GetDeviceStatus(pDI, NULL);
    todo_wine
    ok(hr == E_POINTER, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_Unknown);
    todo_wine
    ok(hr == DIERR_DEVICENOTREG, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    hr = IDirectInput8_GetDeviceStatus(pDI, &GUID_SysMouse);
    ok(hr == DI_OK, "IDirectInput8_GetDeviceStatus returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_RunControlPanel(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    if (winetest_interactive)
    {
        hr = IDirectInput8_RunControlPanel(pDI, NULL, 0);
        ok(hr == S_OK, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

        hr = IDirectInput8_RunControlPanel(pDI, GetDesktopWindow(), 0);
        ok(hr == S_OK, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);
    }

    hr = IDirectInput8_RunControlPanel(pDI, NULL, ~0u);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, 0);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    hr = IDirectInput8_RunControlPanel(pDI, (HWND)0xdeadbeef, ~0u);
    ok(hr == E_HANDLE, "IDirectInput8_RunControlPanel returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

static void test_Initialize(void)
{
    IDirectInput8A *pDI;
    HRESULT hr;

    hr = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, &IID_IDirectInput8A, (void **)&pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%08x\n", hr);
        return;
    }

    hr = IDirectInput8_Initialize(pDI, NULL, 0);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, NULL, DIRECTINPUT_VERSION);
    ok(hr == DIERR_INVALIDPARAM, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions less than DIRECTINPUT_VERSION yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION - 1);
    ok(hr == DIERR_BETADIRECTINPUTVERSION, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Invalid DirectInput versions greater than DIRECTINPUT_VERSION yield DIERR_BETADIRECTINPUTVERSION. */
    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION + 1);
    ok(hr == DIERR_OLDDIRECTINPUTVERSION, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    hr = IDirectInput8_Initialize(pDI, hInstance, DIRECTINPUT_VERSION);
    ok(hr == DI_OK, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    /* Parameters are still validated after successful initialization. */
    hr = IDirectInput8_Initialize(pDI, hInstance, 0);
    ok(hr == DIERR_NOTINITIALIZED, "IDirectInput8_Initialize returned 0x%08x\n", hr);

    IDirectInput8_Release(pDI);
}

START_TEST(dinput)
{
    hInstance = GetModuleHandleA(NULL);

    CoInitialize(NULL);
    test_preinitialization();
    test_DirectInput8Create();
    test_QueryInterface();
    test_CreateDevice();
    test_EnumDevices();
    test_GetDeviceStatus();
    test_RunControlPanel();
    test_Initialize();
    CoUninitialize();
}
