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

#define DIRECTINPUT_VERSION 0x0700

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"
#include "dinputd.h"

#include "dinput_test.h"

#include "initguid.h"

static const DWORD dinput_versions[] =
{
    0x0300,
    0x0500,
    0x050A,
    0x05B2,
    0x0602,
    0x061A,
    0x0700,
    0x0800,
};

static REFIID dinput7_interfaces[] =
{
    &IID_IDirectInputA,
    &IID_IDirectInputW,
    &IID_IDirectInput2A,
    &IID_IDirectInput2W,
    &IID_IDirectInput7A,
    &IID_IDirectInput7W,
};

static REFIID dinput8_interfaces[] =
{
    &IID_IDirectInput8A,
    &IID_IDirectInput8W,
    &IID_IDirectInputJoyConfig8,
};

static HRESULT direct_input_create( DWORD version, IDirectInputA **out )
{
    HRESULT hr;
    if (version < 0x800) hr = DirectInputCreateA( instance, version, out, NULL );
    else hr = DirectInput8Create( instance, version, &IID_IDirectInput8A, (void **)out, NULL );
    if (FAILED(hr)) win_skip( "Failed to instantiate a IDirectInput instance, hr %#lx\n", hr );
    return hr;
}

static BOOL CALLBACK dummy_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    ok(0, "Callback was invoked with parameters (%p, %p)\n", instance, context);
    return DIENUM_STOP;
}

static HRESULT WINAPI outer_QueryInterface( IUnknown *iface, REFIID iid, void **obj )
{
    ok( 0, "unexpected call %s\n", debugstr_guid( iid ) );

    if (IsEqualGUID( iid, &IID_IUnknown ))
    {
        *obj = iface;
        return S_OK;
    }

    ok( 0, "unexpected call %s\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI outer_AddRef( IUnknown *iface )
{
    ok( 0, "unexpected call\n" );
    return 2;
}

static ULONG WINAPI outer_Release( IUnknown *iface )
{
    ok( 0, "unexpected call\n" );
    return 1;
}

static IUnknownVtbl outer_vtbl =
{
    outer_QueryInterface,
    outer_AddRef,
    outer_Release,
};

static IUnknown outer = {&outer_vtbl};

static void test_CoCreateInstance( DWORD version )
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
    } enum_devices_tests[] =
    {
        {0, NULL, 0, DIERR_INVALIDPARAM},
        {0, NULL, ~0u, DIERR_INVALIDPARAM},
        {0, dummy_callback, 0, DIERR_NOTINITIALIZED},
        {0, dummy_callback, ~0u, DIERR_INVALIDPARAM},
        {0xdeadbeef, NULL, 0, DIERR_INVALIDPARAM},
        {0xdeadbeef, NULL, ~0u, DIERR_INVALIDPARAM},
        {0xdeadbeef, dummy_callback, 0, DIERR_INVALIDPARAM},
        {0xdeadbeef, dummy_callback, ~0u, DIERR_INVALIDPARAM},
    };

    IDirectInputDeviceA *device;
    IDirectInputA *dinput;
    IUnknown *unknown;
    HRESULT hr;
    LONG ref;
    int i;

    if (version < 0x800) hr = CoCreateInstance( &CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER,
                                                &IID_IDirectInputA, (void **)&dinput );
    else hr = CoCreateInstance( &CLSID_DirectInput8, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IDirectInput8A, (void **)&dinput );
    if (FAILED(hr))
    {
        win_skip( "Failed to instantiate a IDirectInput instance, hr %#lx\n", hr );
        return;
    }

    if (version < 0x800) hr = CoCreateInstance( &CLSID_DirectInput, &outer, CLSCTX_INPROC_SERVER,
                                                &IID_IDirectInputA, (void **)&unknown );
    else hr = CoCreateInstance( &CLSID_DirectInput8, &outer, CLSCTX_INPROC_SERVER,
                                &IID_IDirectInput8A, (void **)&unknown );
    ok( hr == CLASS_E_NOAGGREGATION, "CoCreateInstance returned %#lx\n", hr );
    if (SUCCEEDED( hr )) IUnknown_Release( unknown );

    for (i = 0; i < ARRAY_SIZE(create_device_tests); i++)
    {
        winetest_push_context( "%u", i );
        if (create_device_tests[i].pdev) device = (void *)0xdeadbeef;
        hr = IDirectInput_CreateDevice( dinput, create_device_tests[i].rguid,
                                        create_device_tests[i].pdev ? &device : NULL, NULL );
        ok( hr == create_device_tests[i].expected_hr, "CreateDevice returned %#lx\n", hr );
        if (create_device_tests[i].pdev) ok( device == NULL, "got device %p\n", device );
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(enum_devices_tests); i++)
    {
        winetest_push_context( "%u", i );
        hr = IDirectInput_EnumDevices( dinput, enum_devices_tests[i].dwDevType,
                                       enum_devices_tests[i].lpCallback, NULL, enum_devices_tests[i].dwFlags );
        ok( hr == enum_devices_tests[i].expected_hr, "EnumDevice returned %#lx\n", hr );
        winetest_pop_context();
    }

    hr = IDirectInput_GetDeviceStatus( dinput, NULL );
    ok( hr == E_POINTER, "GetDeviceStatus returned %#lx\n", hr );

    hr = IDirectInput_GetDeviceStatus( dinput, &GUID_Unknown );
    ok( hr == DIERR_NOTINITIALIZED, "GetDeviceStatus returned %#lx\n", hr );

    hr = IDirectInput_GetDeviceStatus( dinput, &GUID_SysMouse );
    ok( hr == DIERR_NOTINITIALIZED, "GetDeviceStatus returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, NULL, 0 );
    ok( hr == DIERR_NOTINITIALIZED, "RunControlPanel returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, NULL, ~0u );
    ok( hr == DIERR_INVALIDPARAM, "RunControlPanel returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, (HWND)0xdeadbeef, 0 );
    ok( hr == E_HANDLE, "RunControlPanel returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, (HWND)0xdeadbeef, ~0u );
    ok( hr == E_HANDLE, "RunControlPanel returned %#lx\n", hr );

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %ld\n", ref );
}

static void test_DirectInputCreate( DWORD version )
{
    IUnknown *unknown;
    struct
    {
        HMODULE instance;
        DWORD version;
        REFIID iid;
        IUnknown **out;
        IUnknown *expect_out;
        HRESULT expected_hr;
    } create_tests[] =
    {
        {NULL,       0,           NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       0,           NULL, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version,     NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version,     NULL, &unknown, NULL,               version == 0x300 ? DI_OK : DIERR_INVALIDPARAM},
        {NULL,       version - 1, NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version - 1, NULL, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version + 1, NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version + 1, NULL, &unknown, NULL,               DIERR_INVALIDPARAM},
        {instance,   0,           NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   0,           NULL, &unknown, NULL,               DIERR_NOTINITIALIZED},
        {instance,   version,     NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, NULL, &unknown, NULL,               version <= 0x700 ? DIERR_BETADIRECTINPUTVERSION : DIERR_OLDDIRECTINPUTVERSION},
        {instance,   version + 1, NULL, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version + 1, NULL, &unknown, NULL,               version < 0x700 ? DIERR_BETADIRECTINPUTVERSION : DIERR_OLDDIRECTINPUTVERSION},
    };
    HRESULT hr;
    int i;

    unknown = (void *)0xdeadbeef;
    hr = DirectInputCreateW( instance, version, (IDirectInputW **)&unknown, &outer );
    ok( hr == DI_OK, "DirectInputCreateW returned %#lx\n", hr );
    ok( unknown == NULL, "got IUnknown %p\n", unknown );

    for (i = 0; i < ARRAY_SIZE(create_tests); i++)
    {
        winetest_push_context( "%u", i );
        unknown = (void *)0xdeadbeef;
        hr = DirectInputCreateW( create_tests[i].instance, create_tests[i].version, (IDirectInputW **)create_tests[i].out, NULL );
        todo_wine_if(i == 3 && version == 0x300)
        ok( hr == create_tests[i].expected_hr, "DirectInputCreateEx returned %#lx\n", hr );
        if (SUCCEEDED(hr)) IUnknown_Release( unknown );
        else ok( unknown == create_tests[i].expect_out, "got IUnknown %p\n", unknown );
        winetest_pop_context();
    }
}

static void test_DirectInputCreateEx( DWORD version )
{
    IUnknown *unknown;
    struct
    {
        HMODULE instance;
        DWORD version;
        REFIID iid;
        IUnknown **out;
        IUnknown *expect_out;
        HRESULT expected_hr;
    } create_tests[] =
    {
        {NULL,       0,           &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       0,           &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       0,           &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       0,           &IID_IDirectInputA, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version,     &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version,     &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version,     &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version,     &IID_IDirectInputA, &unknown, NULL,               version == 0x300 ? DI_OK : DIERR_INVALIDPARAM},
        {NULL,       version - 1, &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version - 1, &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version - 1, &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version - 1, &IID_IDirectInputA, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version + 1, &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version + 1, &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {NULL,       version + 1, &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version + 1, &IID_IDirectInputA, &unknown, NULL,               DIERR_INVALIDPARAM},
        {instance,   0,           &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   0,           &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   0,           &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   0,           &IID_IDirectInputA, &unknown, NULL,               DIERR_NOTINITIALIZED},
        {instance,   version,     &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version,     &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version,     &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version - 1, &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version - 1, &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, &IID_IDirectInputA, &unknown, NULL,               version <= 0x700 ? DIERR_BETADIRECTINPUTVERSION : DIERR_OLDDIRECTINPUTVERSION},
        {instance,   version + 1, &IID_IUnknown,      NULL,     (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version + 1, &IID_IUnknown,      &unknown, (void *)0xdeadbeef, DIERR_NOINTERFACE},
        {instance,   version + 1, &IID_IDirectInputA, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version + 1, &IID_IDirectInputA, &unknown, NULL,               version < 0x700 ? DIERR_BETADIRECTINPUTVERSION : DIERR_OLDDIRECTINPUTVERSION},
    };
    HRESULT hr;
    int i;

    unknown = (void *)0xdeadbeef;
    hr = DirectInputCreateEx( instance, version, &IID_IDirectInputW, (void **)&unknown, &outer );
    ok( hr == DI_OK, "DirectInputCreateW returned %#lx\n", hr );
    ok( unknown == NULL, "got IUnknown %p\n", unknown );

    for (i = 0; i < ARRAY_SIZE(create_tests); i++)
    {
        winetest_push_context( "%u", i );
        unknown = (void *)0xdeadbeef;
        hr = DirectInputCreateEx( create_tests[i].instance, create_tests[i].version, create_tests[i].iid,
                                  (void **)create_tests[i].out, NULL );
        todo_wine_if( version == 0x300 && i == 7 )
        ok( hr == create_tests[i].expected_hr, "DirectInputCreateEx returned %#lx\n", hr );
        if (SUCCEEDED(hr)) IUnknown_Release( unknown );
        else ok( unknown == create_tests[i].expect_out, "got IUnknown %p\n", unknown );
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dinput8_interfaces); i++)
    {
        winetest_push_context( "%u", i );
        unknown = (void *)0xdeadbeef;
        hr = DirectInputCreateEx( instance, version, dinput8_interfaces[i], (void **)&unknown, NULL );
        ok( hr == DIERR_NOINTERFACE, "DirectInputCreateEx returned %#lx\n", hr );
        ok( unknown == (void *)0xdeadbeef, "got IUnknown %p\n", unknown );
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dinput7_interfaces); i++)
    {
        winetest_push_context( "%u", i );
        unknown = NULL;
        hr = DirectInputCreateEx( instance, version, dinput7_interfaces[i], (void **)&unknown, NULL );
        if (version < 0x800) ok( hr == DI_OK, "DirectInputCreateEx returned %#lx\n", hr );
        else ok( hr == DIERR_OLDDIRECTINPUTVERSION, "DirectInputCreateEx returned %#lx\n", hr );
        if (version < 0x800) ok( unknown != NULL, "got IUnknown NULL\n" );
        else ok( unknown == NULL, "got IUnknown %p\n", unknown );
        if (unknown) IUnknown_Release( unknown );
        winetest_pop_context();
    }
}

static void test_DirectInput8Create( DWORD version )
{
    IUnknown *unknown;
    struct
    {
        HMODULE instance;
        DWORD version;
        REFIID iid;
        IUnknown **out;
        IUnknown *expect_out;
        HRESULT expected_hr;
    } create_tests[] =
    {
        {NULL,       0,           &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       0,           &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {NULL,       0,           &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       0,           &IID_IDirectInput8A, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version,     &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version,     &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {NULL,       version,     &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version,     &IID_IDirectInput8A, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version - 1, &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version - 1, &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {NULL,       version - 1, &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version - 1, &IID_IDirectInput8A, &unknown, NULL,               DIERR_INVALIDPARAM},
        {NULL,       version + 1, &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version + 1, &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {NULL,       version + 1, &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {NULL,       version + 1, &IID_IDirectInput8A, &unknown, NULL,               DIERR_INVALIDPARAM},
        {instance,   0,           &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   0,           &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {instance,   0,           &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   0,           &IID_IDirectInput8A, &unknown, NULL,               DIERR_NOTINITIALIZED},
        {instance,   version,     &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version,     &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {instance,   version,     &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {instance,   version - 1, &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version - 1, &IID_IDirectInput8A, &unknown, NULL,               DIERR_BETADIRECTINPUTVERSION},
        {instance,   version + 1, &IID_IDirectInputA,  NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version + 1, &IID_IDirectInputA,  &unknown, NULL,               DIERR_NOINTERFACE},
        {instance,   version + 1, &IID_IDirectInput8A, NULL,     (void *)0xdeadbeef, E_POINTER},
        {instance,   version + 1, &IID_IDirectInput8A, &unknown, NULL,               version <= 0x700 ? DIERR_BETADIRECTINPUTVERSION : DIERR_OLDDIRECTINPUTVERSION},
    };
    HRESULT hr;
    int i;

    unknown = (void *)0xdeadbeef;
    hr = DirectInput8Create( instance, version, &IID_IDirectInput8W, (void **)&unknown, &outer );
    ok( hr == DI_OK, "DirectInputCreateW returned %#lx\n", hr );
    ok( unknown == NULL, "got IUnknown %p\n", unknown );

    for (i = 0; i < ARRAY_SIZE(create_tests); i++)
    {
        winetest_push_context( "%u", i );
        unknown = (void *)0xdeadbeef;
        hr = DirectInput8Create( create_tests[i].instance, create_tests[i].version, create_tests[i].iid,
                                 (void **)create_tests[i].out, NULL );
        ok( hr == create_tests[i].expected_hr, "DirectInput8Create returned %#lx\n", hr );
        if (SUCCEEDED(hr)) IUnknown_Release( unknown );
        else ok( unknown == create_tests[i].expect_out, "got IUnknown %p\n", unknown );
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dinput7_interfaces); i++)
    {
        winetest_push_context( "%u", i );
        unknown = (void *)0xdeadbeef;
        hr = DirectInput8Create( instance, version, dinput7_interfaces[i], (void **)&unknown, NULL );
        ok( hr == DIERR_NOINTERFACE, "DirectInput8Create returned %#lx\n", hr );
        ok( unknown == NULL, "got IUnknown %p\n", unknown );
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dinput8_interfaces); i++)
    {
        winetest_push_context( "%u", i );
        unknown = NULL;
        hr = DirectInput8Create( instance, version, dinput8_interfaces[i], (void **)&unknown, NULL );
        if (i == 2) ok( hr == DIERR_NOINTERFACE, "DirectInput8Create returned %#lx\n", hr );
        else if (version == 0x800) ok( hr == DI_OK, "DirectInput8Create returned %#lx\n", hr );
        else ok( hr == DIERR_BETADIRECTINPUTVERSION, "DirectInput8Create returned %#lx\n", hr );
        if (i == 2) ok( unknown == NULL, "got IUnknown %p\n", unknown );
        else if (version == 0x800) ok( unknown != NULL, "got NULL IUnknown\n" );
        else ok( unknown == NULL, "got IUnknown %p\n", unknown );
        if (unknown) IUnknown_Release( unknown );
        winetest_pop_context();
    }
}

static void test_QueryInterface( DWORD version )
{
    IUnknown *iface, *tmp_iface;
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;
    int i;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    hr = IDirectInput_QueryInterface( dinput, NULL, NULL );
    ok( hr == E_POINTER, "QueryInterface returned %#lx\n", hr );

    iface = (void *)0xdeadbeef;
    hr = IDirectInput_QueryInterface( dinput, NULL, (void **)&iface );
    ok( hr == E_POINTER, "QueryInterface returned %#lx\n", hr );
    ok( iface == (void *)0xdeadbeef, "Output interface pointer is %p\n", iface );

    hr = IDirectInput_QueryInterface( dinput, &IID_IUnknown, NULL );
    ok( hr == E_POINTER, "QueryInterface returned %#lx\n", hr );

    hr = IDirectInput_QueryInterface( dinput, &IID_IUnknown, (void **)&iface );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    ok( iface != NULL, "got iface NULL\n" );
    ref = IUnknown_Release( iface );
    ok( ref == 1, "Release returned %lu\n", ref );

    for (i = 0; i < ARRAY_SIZE(dinput7_interfaces); i++)
    {
        winetest_push_context("%u", i);
        iface = (void *)0xdeadbeef;
        hr = IDirectInput_QueryInterface( dinput, dinput7_interfaces[i], (void **)&iface );
        if (version >= 0x800)
        {
            ok( hr == E_NOINTERFACE, "QueryInterface returned %#lx\n", hr );
            ok( iface == NULL, "got iface %p\n", iface );
        }
        else
        {
            ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
            ok( iface != NULL, "got iface NULL\n" );
            ref = IUnknown_Release( iface );
            ok( ref == 1, "Release returned %lu\n", ref );
        }
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dinput8_interfaces); i++)
    {
        winetest_push_context("%u", i);
        iface = (void *)0xdeadbeef;
        hr = IDirectInput_QueryInterface( dinput, dinput8_interfaces[i], (void **)&iface );
        if (version < 0x800)
        {
            todo_wine_if(i == 2)
            ok( hr == E_NOINTERFACE, "QueryInterface returned %#lx\n", hr );
            todo_wine_if(i == 2)
            ok( iface == NULL, "got iface %p\n", iface );
        }
        else
        {
            ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
            ok( iface != NULL, "got iface NULL\n" );
            ref = IUnknown_Release( iface );
            ok( ref == 1, "Release returned %lu\n", ref );
        }
        winetest_pop_context();
    }

    if (version < 0x800)
    {
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInputA, (void **)&iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputA) failed: %#lx\n", hr );
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInput2A, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInput2A) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IID_IDirectInput2A iface differs from IID_IDirectInputA\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInput7A, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInput7A) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IID_IDirectInput7A iface differs from IID_IDirectInputA\n" );
        IUnknown_Release( tmp_iface );
        IUnknown_Release( iface );

        hr = IUnknown_QueryInterface( dinput, &IID_IUnknown, (void **)&iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IUnknown) failed: %#lx\n", hr );
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInputW, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInputW) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IID_IDirectInputW iface differs from IID_IUnknown\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInput2W, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInput2W) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IID_IDirectInput2W iface differs from IID_IUnknown\n" );
        IUnknown_Release( tmp_iface );
        hr = IUnknown_QueryInterface( dinput, &IID_IDirectInput7W, (void **)&tmp_iface );
        ok( SUCCEEDED(hr), "IUnknown_QueryInterface(IID_IDirectInput7W) failed: %#lx\n", hr );
        ok( tmp_iface == iface, "IID_IDirectInput7W iface differs from IID_IUnknown\n" );
        IUnknown_Release( tmp_iface );
        IUnknown_Release( iface );
    }

    ref = IDirectInput_Release( dinput );
    todo_wine_if( version < 0x800 )
    ok( ref == 0, "Release returned %lu\n", ref );
}

static void test_CreateDevice( DWORD version )
{
    IDirectInputDeviceA *device;
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    hr = IDirectInput_CreateDevice( dinput, NULL, NULL, NULL );
    ok( hr == E_POINTER, "CreateDevice returned %#lx\n", hr );

    device = (void *)0xdeadbeef;
    hr = IDirectInput_CreateDevice( dinput, NULL, &device, NULL );
    ok( hr == E_POINTER, "CreateDevice returned %#lx\n", hr );
    ok( device == NULL, "Output interface pointer is %p\n", device );

    hr = IDirectInput_CreateDevice( dinput, &GUID_Unknown, NULL, NULL );
    ok( hr == E_POINTER, "CreateDevice returned %#lx\n", hr );

    device = (void *)0xdeadbeef;
    hr = IDirectInput_CreateDevice( dinput, &GUID_Unknown, &device, NULL );
    ok( hr == DIERR_DEVICENOTREG, "CreateDevice returned %#lx\n", hr );
    ok( device == NULL, "Output interface pointer is %p\n", device );

    hr = IDirectInput_CreateDevice( dinput, &GUID_SysMouse, NULL, NULL );
    ok( hr == E_POINTER, "CreateDevice returned %#lx\n", hr );

    hr = IDirectInput_CreateDevice( dinput, &GUID_SysMouse, &device, NULL );
    ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );

    ref = IDirectInputDevice_Release( device );
    ok( ref == 0, "Release returned %lu\n", ref );
    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %lu\n", ref );
}

struct enum_devices_test
{
    unsigned int device_count;
    BOOL return_value;
    DWORD version;
};

DEFINE_GUID( pidvid_product_guid, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D' );

static BOOL CALLBACK enum_devices_callback(const DIDEVICEINSTANCEA *instance, void *context)
{
    BYTE type = GET_DIDEVICE_TYPE( instance->dwDevType );
    struct enum_devices_test *enum_test = context;

    if (winetest_debug > 1)
    {
        trace( "---- Device Information ----\n"
               "Product Name  : %s\n"
               "Instance Name : %s\n"
               "devType       : 0x%#lx\n"
               "GUID Product  : %s\n"
               "GUID Instance : %s\n"
               "HID Page      : 0x%04x\n"
               "HID Usage     : 0x%04x\n",
               instance->tszProductName, instance->tszInstanceName, instance->dwDevType,
               wine_dbgstr_guid( &instance->guidProduct ),
               wine_dbgstr_guid( &instance->guidInstance ), instance->wUsagePage, instance->wUsage );
    }

    if (enum_test->version >= 0x800)
    {
        if (type == DI8DEVTYPE_KEYBOARD || type == DI8DEVTYPE_MOUSE)
        {
            const char *device = (type == DI8DEVTYPE_KEYBOARD) ? "Keyboard" : "Mouse";
            ok( IsEqualGUID( &instance->guidInstance, &instance->guidProduct ),
                "%s guidInstance (%s) does not match guidProduct (%s)\n", device,
                wine_dbgstr_guid( &instance->guidInstance ), wine_dbgstr_guid( &instance->guidProduct ) );
        }
    }
    else
    {
        if (type == DIDEVTYPE_KEYBOARD || type == DIDEVTYPE_MOUSE)
        {
            const char *device = (type == DIDEVTYPE_KEYBOARD) ? "Keyboard" : "Mouse";
            ok( IsEqualGUID( &instance->guidInstance, &instance->guidProduct ),
                "%s guidInstance (%s) does not match guidProduct (%s)\n", device,
                wine_dbgstr_guid( &instance->guidInstance ), wine_dbgstr_guid( &instance->guidProduct ) );
        }

        if (type == DIDEVTYPE_KEYBOARD)
            ok( IsEqualGUID( &instance->guidProduct, &GUID_SysKeyboard ),
                "Keyboard guidProduct (%s) does not match GUID_SysKeyboard (%s)\n",
                wine_dbgstr_guid( &instance->guidProduct ), wine_dbgstr_guid( &GUID_SysMouse ) );
        else if (type == DIDEVTYPE_MOUSE)
            ok( IsEqualGUID( &instance->guidProduct, &GUID_SysMouse ),
                "Mouse guidProduct (%s) does not match GUID_SysMouse (%s)\n",
                wine_dbgstr_guid( &instance->guidProduct ), wine_dbgstr_guid( &GUID_SysMouse ) );
        else
        {
            ok( instance->guidProduct.Data2 == pidvid_product_guid.Data2,
                "guidProduct.Data2 is %04x\n", instance->guidProduct.Data2 );
            ok( instance->guidProduct.Data3 == pidvid_product_guid.Data3,
                "guidProduct.Data3 is %04x\n", instance->guidProduct.Data3 );
            ok( !memcmp( instance->guidProduct.Data4, pidvid_product_guid.Data4,
                         sizeof(pidvid_product_guid.Data4) ),
                "guidProduct.Data4 does not match: %s\n", wine_dbgstr_guid( &instance->guidProduct ) );
        }
    }

    enum_test->device_count++;
    return enum_test->return_value;
}

static void test_EnumDevices( DWORD version )
{
    struct enum_devices_test enum_test = {.version = version}, enum_test_return = {.version = version};
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    hr = IDirectInput_EnumDevices( dinput, 0, NULL, NULL, 0 );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    hr = IDirectInput_EnumDevices( dinput, 0, NULL, NULL, ~0u );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    /* Test crashes on Wine. */
    if (0)
    {
        hr = IDirectInput_EnumDevices( dinput, 0, enum_devices_callback, NULL, ~0u );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );
    }

    hr = IDirectInput_EnumDevices( dinput, 0xdeadbeef, NULL, NULL, 0 );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    hr = IDirectInput_EnumDevices( dinput, 0xdeadbeef, NULL, NULL, ~0u );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    hr = IDirectInput_EnumDevices( dinput, 0xdeadbeef, enum_devices_callback, NULL, 0 );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    hr = IDirectInput_EnumDevices( dinput, 0xdeadbeef, enum_devices_callback, NULL, ~0u );
    ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned %#lx\n", hr );

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_CONTINUE;
    hr = IDirectInput_EnumDevices( dinput, 0, enum_devices_callback, &enum_test, 0 );
    ok( hr == DI_OK, "EnumDevices returned %#lx\n", hr );
    ok(enum_test.device_count != 0, "Device count is %u\n", enum_test.device_count);

    /* Enumeration only stops with an explicit DIENUM_STOP. */
    enum_test_return.device_count = 0;
    enum_test_return.return_value = 42;
    hr = IDirectInput_EnumDevices( dinput, 0, enum_devices_callback, &enum_test_return, 0 );
    ok( hr == DI_OK, "EnumDevices returned %#lx\n", hr );
    ok(enum_test_return.device_count == enum_test.device_count,
       "Device count is %u vs. %u\n", enum_test_return.device_count, enum_test.device_count);

    enum_test.device_count = 0;
    enum_test.return_value = DIENUM_STOP;
    hr = IDirectInput_EnumDevices( dinput, 0, enum_devices_callback, &enum_test, 0 );
    ok( hr == DI_OK, "EnumDevices returned %#lx\n", hr );
    ok(enum_test.device_count == 1, "Device count is %u\n", enum_test.device_count);

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %lu\n", ref );
}

static void test_GetDeviceStatus( DWORD version )
{
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    hr = IDirectInput_GetDeviceStatus( dinput, NULL );
    ok( hr == E_POINTER, "GetDeviceStatus returned %#lx\n", hr );

    hr = IDirectInput_GetDeviceStatus( dinput, &GUID_Unknown );
    todo_wine
    ok( hr == DIERR_DEVICENOTREG, "GetDeviceStatus returned %#lx\n", hr );

    hr = IDirectInput_GetDeviceStatus( dinput, &GUID_SysMouse );
    ok( hr == DI_OK, "GetDeviceStatus returned %#lx\n", hr );

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %lu\n", ref );
}

static void test_Initialize( DWORD version )
{
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    hr = IDirectInput_Initialize( dinput, NULL, 0 );
    ok( hr == DIERR_INVALIDPARAM, "Initialize returned %#lx\n", hr );

    hr = IDirectInput_Initialize( dinput, NULL, version );
    if (version == 0x300) todo_wine ok( hr == S_OK, "Initialize returned %#lx\n", hr );
    else ok( hr == DIERR_INVALIDPARAM, "Initialize returned %#lx\n", hr );

    hr = IDirectInput_Initialize( dinput, instance, 0 );
    ok( hr == DIERR_NOTINITIALIZED, "Initialize returned %#lx\n", hr );

    hr = IDirectInput_Initialize( dinput, instance, version - 1 );
    ok( hr == DIERR_BETADIRECTINPUTVERSION, "Initialize returned %#lx\n", hr );

    hr = IDirectInput_Initialize( dinput, instance, version + 1 );
    if (version >= 0x700) ok( hr == DIERR_OLDDIRECTINPUTVERSION, "Initialize returned %#lx\n", hr );
    else ok( hr == DIERR_BETADIRECTINPUTVERSION, "Initialize returned %#lx\n", hr );

    /* Parameters are still validated after successful initialization. */
    hr = IDirectInput_Initialize( dinput, instance, 0 );
    ok( hr == DIERR_NOTINITIALIZED, "Initialize returned %#lx\n", hr );

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %lu\n", ref );
}

static void test_RunControlPanel( DWORD version )
{
    IDirectInputA *dinput;
    HRESULT hr;
    ULONG ref;

    if (FAILED(hr = direct_input_create( version, &dinput ))) return;

    if (winetest_interactive)
    {
        hr = IDirectInput_RunControlPanel( dinput, NULL, 0 );
        ok( hr == S_OK, "RunControlPanel returned %#lx\n", hr );

        hr = IDirectInput_RunControlPanel( dinput, GetDesktopWindow(), 0 );
        ok( hr == S_OK, "RunControlPanel returned %#lx\n", hr );
    }

    hr = IDirectInput_RunControlPanel( dinput, NULL, ~0u );
    ok( hr == DIERR_INVALIDPARAM, "RunControlPanel returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, (HWND)0xdeadbeef, 0 );
    ok( hr == E_HANDLE, "RunControlPanel returned %#lx\n", hr );

    hr = IDirectInput_RunControlPanel( dinput, (HWND)0xdeadbeef, ~0u );
    ok( hr == E_HANDLE, "RunControlPanel returned %#lx\n", hr );

    ref = IDirectInput_Release( dinput );
    ok( ref == 0, "Release returned %lu\n", ref );
}

static void test_DirectInputJoyConfig8(void)
{
    IDirectInputA *pDI;
    IDirectInputDeviceA *pDID;
    IDirectInputJoyConfig8 *pDIJC;
    DIJOYCONFIG info;
    HRESULT hr;
    int i;

    hr = DirectInputCreateA(instance, DIRECTINPUT_VERSION, &pDI, NULL);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputA instance: 0x%#lx\n", hr);
        return;
    }

    hr = IDirectInput_QueryInterface(pDI, &IID_IDirectInputJoyConfig8, (void **)&pDIJC);
    if (FAILED(hr))
    {
        win_skip("Failed to instantiate a IDirectInputJoyConfig8 instance: 0x%#lx\n", hr);
        return;
    }

    info.dwSize = sizeof(info);
    hr = DI_OK;
    i = 0;

    /* Enumerate all connected joystick GUIDs and try to create the respective devices */
    for (i = 0; SUCCEEDED(hr); i++)
    {
        hr = IDirectInputJoyConfig8_GetConfig(pDIJC, i, &info, DIJC_GUIDINSTANCE);

        ok (hr == DI_OK || hr == DIERR_NOMOREITEMS,
           "IDirectInputJoyConfig8_GetConfig returned %#lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = IDirectInput_CreateDevice(pDI, &info.guidInstance, &pDID, NULL);
            ok( SUCCEEDED(hr), "CreateDevice failed with guid from GetConfig hr = 0x%#lx\n", hr );
            IDirectInputDevice_Release(pDID);
        }
    }

    IDirectInputJoyConfig8_Release(pDIJC);
    IDirectInput_Release(pDI);
}

struct enum_semantics_test
{
    unsigned int device_count;
    DWORD first_remaining;
    BOOL mouse;
    BOOL keyboard;
    DIACTIONFORMATA *action_format;
    const char *username;
};

static DIACTIONA actionMapping[] = {
    /* axis */
    {0, 0x01008A01 /* DIAXIS_DRIVINGR_STEER */, 0, {"Steer.\0"}},
    /* button */
    {1, 0x01000C01 /* DIBUTTON_DRIVINGR_SHIFTUP */, 0, {"Upshift.\0"}},
    /* keyboard key */
    {2, DIKEYBOARD_SPACE, 0, {"Missile.\0"}},
    /* mouse button */
    {3, DIMOUSE_BUTTON0, 0, {"Select\0"}},
    /* mouse axis */
    {4, DIMOUSE_YAXIS, 0, {"Y Axis\0"}}};
/* By placing the memory pointed to by lptszActionName right before memory with PAGE_NOACCESS
 * one can find out that the regular ansi string termination is not respected by
 * EnumDevicesBySemantics. Adding a double termination, making it a valid wide string termination,
 * made the test succeed. Therefore it looks like ansi version of EnumDevicesBySemantics forwards
 * the string to the wide variant without conversation. */

static BOOL CALLBACK enum_semantics_callback( const DIDEVICEINSTANCEA *lpddi, IDirectInputDevice8A *lpdid,
                                              DWORD dwFlags, DWORD dwRemaining, void *context )
{
    struct enum_semantics_test *data = context;

    if (context == NULL) return DIENUM_STOP;

    if (!data->device_count)
    {
        data->first_remaining = dwRemaining;
    }
    ok( dwRemaining == data->first_remaining - data->device_count,
        "enum semantics remaining devices is wrong, expected %ld, had %ld\n",
        data->first_remaining - data->device_count, dwRemaining );
    data->device_count++;

    if (IsEqualGUID( &lpddi->guidInstance, &GUID_SysKeyboard )) data->keyboard = TRUE;

    if (IsEqualGUID( &lpddi->guidInstance, &GUID_SysMouse )) data->mouse = TRUE;

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK set_action_map_callback( const DIDEVICEINSTANCEA *lpddi, IDirectInputDevice8A *lpdid,
                                              DWORD dwFlags, DWORD dwRemaining, void *context )
{
    HRESULT hr;
    struct enum_semantics_test *data = context;

    /* Building and setting an action map */
    /* It should not use any pre-stored mappings so we use DIDBAM_INITIALIZE */
    hr = IDirectInputDevice8_BuildActionMap( lpdid, data->action_format, NULL, DIDBAM_INITIALIZE );
    ok( SUCCEEDED(hr), "BuildActionMap failed hr=%#lx\n", hr );

    hr = IDirectInputDevice8_SetActionMap( lpdid, data->action_format, data->username, 0 );
    ok( SUCCEEDED(hr), "SetActionMap failed hr=%#lx\n", hr );

    return DIENUM_CONTINUE;
}

static void test_EnumDevicesBySemantics(void)
{
    DIACTIONFORMATA action_format;
    const GUID ACTION_MAPPING_GUID = {0x1, 0x2, 0x3, {0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb}};
    struct enum_semantics_test data = {0, 0, FALSE, FALSE, &action_format, NULL};
    IDirectInput8A *dinput;
    int device_total = 0;
    HRESULT hr;

    hr = DirectInput8Create( instance, 0x800, &IID_IDirectInput8A, (void **)&dinput, NULL );
    if (FAILED(hr))
    {
        win_skip( "Failed to instantiate a IDirectInputA instance: 0x%#lx\n", hr );
        return;
    }

    memset( &action_format, 0, sizeof(action_format) );
    action_format.dwSize = sizeof(action_format);
    action_format.dwActionSize = sizeof(DIACTIONA);
    action_format.dwNumActions = ARRAY_SIZE(actionMapping);
    action_format.dwDataSize = 4 * action_format.dwNumActions;
    action_format.rgoAction = actionMapping;
    action_format.guidActionMap = ACTION_MAPPING_GUID;
    action_format.dwGenre = 0x01000000; /* DIVIRTUAL_DRIVING_RACE */
    action_format.dwBufferSize = 32;

    /* Test enumerating all attached and installed devices */
    data.keyboard = FALSE;
    data.mouse = FALSE;
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, enum_semantics_callback,
                                               &data, DIEDBSFL_ATTACHEDONLY );
    ok( data.device_count > 0, "EnumDevicesBySemantics did not call the callback hr=%#lx\n", hr );
    ok( data.keyboard, "EnumDevicesBySemantics should enumerate the keyboard\n" );
    ok( data.mouse, "EnumDevicesBySemantics should enumerate the mouse\n" );

    /* Enumerate Force feedback devices. We should get no mouse nor keyboard */
    data.keyboard = FALSE;
    data.mouse = FALSE;
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, enum_semantics_callback,
                                               &data, DIEDBSFL_FORCEFEEDBACK );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( !data.keyboard, "Keyboard should not be enumerated when asking for forcefeedback\n" );
    ok( !data.mouse, "Mouse should not be enumerated when asking for forcefeedback\n" );

    /* Enumerate available devices. That is devices not owned by any user.
       Before setting the action map for all devices we still have them available. */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, enum_semantics_callback,
                                               &data, DIEDBSFL_AVAILABLEDEVICES );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count > 0,
        "There should be devices available before action mapping available=%d\n", data.device_count );

    /* Keep the device total */
    device_total = data.device_count;

    /* There should be no devices for any user. No device should be enumerated with DIEDBSFL_THISUSER.
       MSDN defines that all unowned devices are also enumerated but this doesn't seem to be happening. */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, "Sh4d0w M4g3", &action_format,
                                               enum_semantics_callback, &data, DIEDBSFL_THISUSER );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == 0, "No devices should be assigned for this user assigned=%d\n", data.device_count );

    /* This enumeration builds and sets the action map for all devices with a NULL username */
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, set_action_map_callback,
                                               &data, DIEDBSFL_ATTACHEDONLY );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%#lx\n", hr );

    /* After a successful action mapping we should have no devices available */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, enum_semantics_callback,
                                               &data, DIEDBSFL_AVAILABLEDEVICES );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == 0, "No device should be available after action mapping available=%d\n",
        data.device_count );

    /* Now we'll give all the devices to a specific user */
    data.username = "Sh4d0w M4g3";
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, set_action_map_callback,
                                               &data, DIEDBSFL_ATTACHEDONLY );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed: hr=%#lx\n", hr );

    /* Testing with the default user, DIEDBSFL_THISUSER has no effect */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format,
                                               enum_semantics_callback, &data, DIEDBSFL_THISUSER );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == device_total, "THISUSER has no effect with NULL username owned=%d, expected=%d\n",
        data.device_count, device_total );

    /* Using an empty user string is the same as passing NULL, DIEDBSFL_THISUSER has no effect */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, "", &action_format, enum_semantics_callback, &data, DIEDBSFL_THISUSER );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == device_total, "THISUSER has no effect with \"\" as username owned=%d, expected=%d\n",
        data.device_count, device_total );

    /* Testing with a user with no ownership of the devices */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, "Ninja Brian", &action_format,
                                               enum_semantics_callback, &data, DIEDBSFL_THISUSER );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == 0, "This user should own no devices owned=%d\n", data.device_count );

    /* Sh4d0w M4g3 has ownership of all devices */
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, "Sh4d0w M4g3", &action_format,
                                               enum_semantics_callback, &data, DIEDBSFL_THISUSER );
    ok( SUCCEEDED(hr), "EnumDevicesBySemantics failed hr=%#lx\n", hr );
    ok( data.device_count == device_total, "This user should own %d devices owned=%d\n",
        device_total, data.device_count );

    /* The call fails with a zeroed GUID */
    memset( &action_format.guidActionMap, 0, sizeof(GUID) );
    data.device_count = 0;
    hr = IDirectInput8_EnumDevicesBySemantics( dinput, NULL, &action_format, enum_semantics_callback, NULL, 0 );
    todo_wine
    ok( FAILED(hr), "EnumDevicesBySemantics succeeded with invalid GUID hr=%#lx\n", hr );

    IDirectInput8_Release( dinput );
}

START_TEST(dinput)
{
    DWORD i;

    dinput_test_init();

    test_CoCreateInstance( 0x700 );
    test_CoCreateInstance( 0x800 );

    for (i = 0; i < ARRAY_SIZE(dinput_versions); i++)
    {
        winetest_push_context( "%#lx", dinput_versions[i] );
        test_DirectInputCreate( dinput_versions[i] );
        test_DirectInputCreateEx( dinput_versions[i] );
        test_DirectInput8Create( dinput_versions[i] );
        test_QueryInterface( dinput_versions[i] );
        test_CreateDevice( dinput_versions[i] );
        test_EnumDevices( dinput_versions[i] );
        test_GetDeviceStatus( dinput_versions[i] );
        test_RunControlPanel( dinput_versions[i] );
        test_Initialize( dinput_versions[i] );
        winetest_pop_context();
    }

    test_DirectInputJoyConfig8();
    test_EnumDevicesBySemantics();

    dinput_test_exit();
}
