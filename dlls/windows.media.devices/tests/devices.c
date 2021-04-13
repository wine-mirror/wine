/*
 * Copyright 2021 Andrew Eikum for CodeWeavers
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#define COBJMACROS
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "mmdeviceapi.h"
#include "audioclient.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_Devices
#include "windows.media.devices.h"

#include "wine/test.h"

static HRESULT (WINAPI *pRoGetActivationFactory)(HSTRING, REFIID, void **);
static HRESULT (WINAPI *pRoInitialize)(RO_INIT_TYPE);
static void    (WINAPI *pRoUninitialize)(void);
static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);

static void test_MediaDeviceStatics(void)
{
    static const WCHAR *media_device_statics_name = L"Windows.Media.Devices.MediaDevice";

    IMediaDeviceStatics *media_device_statics = NULL;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    HSTRING str;
    HRESULT hr;

    hr = pWindowsCreateString(media_device_statics_name, wcslen(media_device_statics_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#x\n", hr);
    pWindowsDeleteString(str);

    /* interface tests */
    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IMediaDeviceStatics, (void **)&media_device_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IMediaDeviceStatics failed, hr %#x\n", hr);

    hr = IMediaDeviceStatics_QueryInterface(media_device_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IMediaDeviceStatics_QueryInterface IID_IInspectable failed, hr %#x\n", hr);
    ok(tmp_inspectable == inspectable, "IMediaDeviceStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IMediaDeviceStatics_QueryInterface(media_device_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IMediaDeviceStatics_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);
    ok(tmp_agile_object == agile_object, "IMediaDeviceStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    /* cleanup */
    IMediaDeviceStatics_Release(media_device_statics);
}

START_TEST(devices)
{
    HMODULE combase;
    HRESULT hr;

    if (!(combase = LoadLibraryW(L"combase.dll")))
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return;
    }

#define LOAD_FUNCPTR(x) \
    if (!(p##x = (void*)GetProcAddress(combase, #x))) \
    { \
        win_skip("Failed to find %s in combase.dll, skipping tests.\n", #x); \
        return; \
    }

    LOAD_FUNCPTR(RoGetActivationFactory);
    LOAD_FUNCPTR(RoInitialize);
    LOAD_FUNCPTR(RoUninitialize);
    LOAD_FUNCPTR(WindowsCreateString);
    LOAD_FUNCPTR(WindowsDeleteString);
#undef LOAD_FUNCPTR

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    test_MediaDeviceStatics();

    pRoUninitialize();
}
