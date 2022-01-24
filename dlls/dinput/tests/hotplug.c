/*
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"
#include "dinputd.h"
#include "devguid.h"
#include "mmsystem.h"

#include "wine/hid.h"

#include "dinput_test.h"

static BOOL test_input_lost( DWORD version )
{
#include "psh_hid_macros.h"
    static const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Physical),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 6),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
#include "pop_hid_macros.h"

    static const HIDP_CAPS hid_caps =
    {
        .InputReportByteLength = 1,
    };
    static const DIPROPDWORD buffer_size =
    {
        .diph =
        {
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwSize = sizeof(DIPROPDWORD),
            .dwHow = DIPH_DEVICE,
            .dwObj = 0,
        },
        .dwData = UINT_MAX,
    };

    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    DIDEVICEOBJECTDATA objdata[32] = {{0}};
    WCHAR cwd[MAX_PATH], tempdir[MAX_PATH];
    IDirectInputDevice8W *device = NULL;
    ULONG ref, count, size;
    DIJOYSTATE2 state;
    HRESULT hr;

    winetest_push_context( "%#x", version );

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    cleanup_registry_keys();
    if (!dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned %#x\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, 0, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND );
    ok( hr == DI_OK, "SetCooperativeLevel returned %#x\n", hr );
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_BUFFERSIZE, &buffer_size.diph );
    ok( hr == DI_OK, "SetProperty returned %#x\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DI_OK, "GetDeviceState returned %#x\n", hr );
    size = version < 0x0800 ? sizeof(DIDEVICEOBJECTDATA_DX3) : sizeof(DIDEVICEOBJECTDATA);
    count = 1;
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DI_OK, "GetDeviceData returned %#x\n", hr );
    ok( count == 0, "got %u expected %u\n", count, 0 );

    pnp_driver_stop();

    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_INPUTLOST, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_INPUTLOST, "GetDeviceData returned %#x\n", hr );
    hr = IDirectInputDevice8_Poll( device );
    ok( hr == DIERR_INPUTLOST, "Poll returned: %#x\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceData( device, size, objdata, &count, DIGDD_PEEK );
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceData returned %#x\n", hr );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_NOEFFECT, "Unacquire returned: %#x\n", hr );

    dinput_driver_start( report_desc, sizeof(report_desc), &hid_caps, NULL, 0 );

    hr = IDirectInputDevice8_Acquire( device );
    todo_wine
    ok( hr == DIERR_UNPLUGGED, "Acquire returned %#x\n", hr );
    hr = IDirectInputDevice8_GetDeviceState( device, sizeof(state), &state );
    todo_wine
    ok( hr == DIERR_NOTACQUIRED, "GetDeviceState returned %#x\n", hr );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %d\n", ref );

done:
    pnp_driver_stop();
    cleanup_registry_keys();
    SetCurrentDirectoryW( cwd );

    winetest_pop_context();
    return device != NULL;
}

START_TEST( hotplug )
{
    if (!dinput_test_init()) return;

    CoInitialize( NULL );
    if (test_input_lost( 0x500 ))
    {
        test_input_lost( 0x700 );
        test_input_lost( 0x800 );
    }
    CoUninitialize();

    dinput_test_exit();
}
