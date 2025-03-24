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
#include <stdint.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"

#define COBJMACROS
#include "dinput.h"
#include "dinputd.h"
#include "roapi.h"
#include "unknwn.h"
#include "winstring.h"

#include "wine/hid.h"

#include "dinput_test.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#define WIDL_using_Windows_Foundation_Numerics
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Haptics
#define WIDL_using_Windows_Gaming_Input
#define WIDL_using_Windows_Gaming_Input_ForceFeedback
#include "windows.gaming.input.h"
#include "windows.gaming.input.forcefeedback.h"
#undef Size

static HRESULT (WINAPI *pRoGetActivationFactory)( HSTRING, REFIID, void** );
static HRESULT (WINAPI *pRoInitialize)( RO_INIT_TYPE );
static HRESULT (WINAPI *pWindowsCreateString)( const WCHAR*, UINT32, HSTRING* );
static HRESULT (WINAPI *pWindowsDeleteString)( HSTRING str );
static const WCHAR* (WINAPI *pWindowsGetStringRawBuffer)( HSTRING, UINT32* );

static BOOL load_combase_functions(void)
{
    HMODULE combase = GetModuleHandleW( L"combase.dll" );

#define LOAD_FUNC(m, f) if (!(p ## f = (void *)GetProcAddress( m, #f ))) goto failed;
    LOAD_FUNC( combase, RoGetActivationFactory );
    LOAD_FUNC( combase, RoInitialize );
    LOAD_FUNC( combase, WindowsCreateString );
    LOAD_FUNC( combase, WindowsDeleteString );
    LOAD_FUNC( combase, WindowsGetStringRawBuffer );
#undef LOAD_FUNC

    return TRUE;

failed:
    win_skip("Failed to load combase.dll functions, skipping tests\n");
    return FALSE;
}

struct check_objects_todos
{
    BOOL type;
    BOOL guid;
    BOOL usage;
    BOOL name;
};

struct check_objects_params
{
    DWORD version;
    UINT index;
    UINT expect_count;
    const DIDEVICEOBJECTINSTANCEW *expect_objs;
    const struct check_objects_todos *todo_objs;
    BOOL todo_extra;
};

static BOOL CALLBACK check_objects( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    static const DIDEVICEOBJECTINSTANCEW unexpected_obj = {0};
    static const struct check_objects_todos todo_none = {0};
    struct check_objects_params *params = args;
    const DIDEVICEOBJECTINSTANCEW *exp = params->expect_objs + params->index;
    const struct check_objects_todos *todo;

    if (!params->todo_objs) todo = &todo_none;
    else todo = params->todo_objs + params->index;

    todo_wine_if( params->todo_extra && params->index >= params->expect_count )
    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) return DIENUM_STOP;

    winetest_push_context( "obj[%d]", params->index );

    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) exp = &unexpected_obj;

    check_member( *obj, *exp, "%lu", dwSize );
    todo_wine_if( todo->guid )
    check_member_guid( *obj, *exp, guidType );
    todo_wine_if( params->version < 0x700 && (obj->dwType & DIDFT_BUTTON) )
    check_member( *obj, *exp, "%#lx", dwOfs );
    todo_wine_if( todo->type )
    check_member( *obj, *exp, "%#lx", dwType );
    check_member( *obj, *exp, "%#lx", dwFlags );
    if (!localized) todo_wine_if( todo->name ) check_member_wstr( *obj, *exp, tszName );
    check_member( *obj, *exp, "%lu", dwFFMaxForce );
    check_member( *obj, *exp, "%lu", dwFFForceResolution );
    check_member( *obj, *exp, "%u", wCollectionNumber );
    check_member( *obj, *exp, "%u", wDesignatorIndex );
    check_member( *obj, *exp, "%#04x", wUsagePage );
    todo_wine_if( todo->usage )
    check_member( *obj, *exp, "%#04x", wUsage );
    check_member( *obj, *exp, "%#lx", dwDimension );
    check_member( *obj, *exp, "%#04x", wExponent );
    check_member( *obj, *exp, "%u", wReportId );

    winetest_pop_context();

    params->index++;
    return DIENUM_CONTINUE;
}

struct check_effects_params
{
    UINT index;
    UINT expect_count;
    const DIEFFECTINFOW *expect_effects;
};

static BOOL CALLBACK check_effects( const DIEFFECTINFOW *effect, void *args )
{
    static const DIEFFECTINFOW unexpected_effect = {0};
    struct check_effects_params *params = args;
    const DIEFFECTINFOW *exp = params->expect_effects + params->index;

    winetest_push_context( "effect[%d]", params->index );

    ok( params->index < params->expect_count, "unexpected extra object\n" );
    if (params->index >= params->expect_count) exp = &unexpected_effect;

    check_member( *effect, *exp, "%lu", dwSize );
    check_member_guid( *effect, *exp, guid );
    check_member( *effect, *exp, "%#lx", dwEffType );
    check_member( *effect, *exp, "%#lx", dwStaticParams );
    check_member( *effect, *exp, "%#lx", dwDynamicParams );
    check_member_wstr( *effect, *exp, tszName );

    winetest_pop_context();
    params->index++;

    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_effect_count( const DIEFFECTINFOW *effect, void *args )
{
    DWORD *count = args;
    *count = *count + 1;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK check_no_created_effect_objects( IDirectInputEffect *effect, void *context )
{
    ok( 0, "unexpected effect %p\n", effect );
    return DIENUM_CONTINUE;
}

struct check_created_effect_params
{
    IDirectInputEffect *expect_effect;
    DWORD count;
};

static BOOL CALLBACK check_created_effect_objects( IDirectInputEffect *effect, void *context )
{
    struct check_created_effect_params *params = context;
    ULONG ref;

    ok( effect == params->expect_effect, "got effect %p, expected %p\n", effect, params->expect_effect );
    params->count++;

    IDirectInputEffect_AddRef( effect );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 1, "got ref %lu, expected 1\n", ref );
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK enum_device_count( const DIDEVICEINSTANCEW *devinst, void *context )
{
    DWORD *count = context;
    *count = *count + 1;
    return DIENUM_CONTINUE;
}

static void check_dinput_devices( DWORD version, DIDEVICEINSTANCEW *devinst )
{
    IDirectInput8W *di8;
    IDirectInputW *di;
    ULONG count, ref;
    HRESULT hr;

    if (version >= 0x800)
    {
        hr = DirectInput8Create( instance, version, &IID_IDirectInput8W, (void **)&di8, NULL );
        ok( hr == DI_OK, "DirectInput8Create returned %#lx\n", hr );

        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, NULL, NULL, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, enum_device_count, &count, 0xdeadbeef );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        hr = IDirectInput8_EnumDevices( di8, 0xdeadbeef, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 3, "got count %lu, expected 0\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_DEVICE, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 0, "got count %lu, expected 0\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_POINTER, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        todo_wine
        ok( count == 3, "got count %lu, expected 3\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_KEYBOARD, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        todo_wine
        ok( count == 3, "got count %lu, expected 3\n", count );
        count = 0;
        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_GAMECTRL, enum_device_count, &count,
                                        DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 1, "got count %lu, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 1, "got count %lu, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_FORCEFEEDBACK );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        if (IsEqualGUID( &devinst->guidFFDriver, &GUID_NULL )) ok( count == 0, "got count %lu, expected 0\n", count );
        else ok( count == 1, "got count %lu, expected 1\n", count );

        count = 0;
        hr = IDirectInput8_EnumDevices( di8, (devinst->dwDevType & 0xff) + 1, enum_device_count, &count, DIEDFL_ALLDEVICES );
        if ((devinst->dwDevType & 0xff) != DI8DEVTYPE_SUPPLEMENTAL) ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        else ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        ok( count == 0, "got count %lu, expected 0\n", count );

        ref = IDirectInput8_Release( di8 );
        ok( ref == 0, "Release returned %ld\n", ref );
    }
    else
    {
        hr = DirectInputCreateEx( instance, version, &IID_IDirectInput2W, (void **)&di, NULL );
        ok( hr == DI_OK, "DirectInputCreateEx returned %#lx\n", hr );

        hr = IDirectInput_EnumDevices( di, 0, NULL, NULL, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, 0xdeadbeef );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        hr = IDirectInput_EnumDevices( di, 0xdeadbeef, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, DIEDFL_INCLUDEHIDDEN );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );

        count = 0;
        hr = IDirectInput_EnumDevices( di, 0, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 3, "got count %lu, expected 0\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_DEVICE, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 0, "got count %lu, expected 0\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_MOUSE, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        todo_wine
        ok( count == 3, "got count %lu, expected 3\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_KEYBOARD, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        todo_wine
        ok( count == 3, "got count %lu, expected 3\n", count );
        count = 0;
        hr = IDirectInput_EnumDevices( di, DIDEVTYPE_JOYSTICK, enum_device_count, &count,
                                       DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 1, "got count %lu, expected 1\n", count );

        count = 0;
        hr = IDirectInput_EnumDevices( di, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        ok( count == 1, "got count %lu, expected 1\n", count );

        count = 0;
        hr = IDirectInput_EnumDevices( di, (devinst->dwDevType & 0xff), enum_device_count, &count, DIEDFL_FORCEFEEDBACK );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        if (IsEqualGUID( &devinst->guidFFDriver, &GUID_NULL )) ok( count == 0, "got count %lu, expected 0\n", count );
        else ok( count == 1, "got count %lu, expected 1\n", count );

        hr = IDirectInput_EnumDevices( di, 0x14, enum_device_count, &count, DIEDFL_ALLDEVICES );
        ok( hr == DIERR_INVALIDPARAM, "EnumDevices returned: %#lx\n", hr );

        ref = IDirectInput_Release( di );
        ok( ref == 0, "Release returned %ld\n", ref );
    }
}

static void test_periodic_effect( IDirectInputDevice8W *device, HANDLE file, DWORD version )
{
    struct hid_expect expect_download[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x01,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
        /* start command when DIEP_START is set */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {0x02,0x01,0x01,0x01},
        },
    };
    struct hid_expect expect_download_0[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x00},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x00,0x00,0x00,0x00,0x00,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_download_1[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_download_2[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
    };
    struct hid_expect expect_download_3[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x01,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0xff,0xff,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
    };
    struct hid_expect expect_download_4[] =
    {
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {0x05,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x02,0x08,0xff,0xff,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xd5},
        },
    };
    struct hid_expect expect_update[] =
    {
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0xff, 0xff, version >= 0x700 ? 0x06 : 0x00, 0x00, 0x01, 0x55, 0xd5},
        },
    };
    struct hid_expect expect_set_envelope[] =
    {
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06, 0x19, 0x4c, 0x01, 0x00, 0x04, 0x00},
        },
    };
    struct hid_expect expect_start =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x01},
    };
    struct hid_expect expect_start_solo =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x02, 0x01},
    };
    struct hid_expect expect_start_0 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x00},
    };
    struct hid_expect expect_start_4 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x01, 0x04},
    };
    struct hid_expect expect_stop =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x03, 0x00},
    };
    struct hid_expect expect_unload[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {0x02,0x01,0x03,0x00},
        },
        /* device reset, when unloaded from Unacquire */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1,0x01},
        },
    };
    struct hid_expect expect_acquire[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0x19},
        },
    };
    struct hid_expect expect_reset[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    static const DWORD expect_axes_init[2] = {0};
    const DIEFFECT expect_desc_init =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwTriggerButton = -1,
        .rgdwAxes = (void *)expect_axes_init,
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] =
    {
        +3000,
        -6000,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DIPERIODIC expect_periodic =
    {
        .dwMagnitude = 1000,
        .lOffset = 2000,
        .dwPhase = 3000,
        .dwPeriod = 4000,
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 3,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = sizeof(DIPERIODIC),
        .lpvTypeSpecificParams = (void *)&expect_periodic,
        .dwStartDelay = 6000,
    };
    static const LONG expect_directions_0[3] = {0};
    static const DIENVELOPE expect_envelope_0 = {.dwSize = sizeof(DIENVELOPE)};
    static const DIPERIODIC expect_periodic_0 = {0};
    const DIEFFECT expect_desc_0 =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .cAxes = 3,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions_0,
        .lpEnvelope = (void *)&expect_envelope_0,
        .cbTypeSpecificParams = sizeof(DIPERIODIC),
        .lpvTypeSpecificParams = (void *)&expect_periodic_0,
    };
    struct check_created_effect_params check_params = {0};
    IDirectInputEffect *effect;
    DIPERIODIC periodic = {0};
    DIENVELOPE envelope = {0};
    LONG directions[4] = {0};
    DIEFFECT desc = {0};
    DWORD axes[4] = {0};
    ULONG i, ref, flags;
    HRESULT hr;
    GUID guid;

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, NULL, NULL );
    ok( hr == E_POINTER, "CreateEffect returned %#lx\n", hr );
    hr = IDirectInputDevice8_CreateEffect( device, NULL, NULL, &effect, NULL );
    ok( hr == E_POINTER, "CreateEffect returned %#lx\n", hr );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_NULL, NULL, &effect, NULL );
    ok( hr == DIERR_DEVICENOTREG, "CreateEffect returned %#lx\n", hr );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    if (hr != DI_OK) return;

    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_no_created_effect_objects, effect, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "EnumCreatedEffectObjects returned %#lx\n", hr );
    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#lx\n", hr );
    ok( check_params.count == 1, "got count %lu, expected 1\n", check_params.count );

    hr = IDirectInputEffect_Initialize( effect, NULL, version, &GUID_Sine );
    ok( hr == DIERR_INVALIDPARAM, "Initialize returned %#lx\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, 0x800 - (version - 0x700), &GUID_Sine );
    if (version == 0x800)
    {
        todo_wine
        ok( hr == DIERR_BETADIRECTINPUTVERSION, "Initialize returned %#lx\n", hr );
    }
    else
    {
        todo_wine
        ok( hr == DIERR_OLDDIRECTINPUTVERSION, "Initialize returned %#lx\n", hr );
    }
    hr = IDirectInputEffect_Initialize( effect, instance, 0, &GUID_Sine );
    todo_wine
    ok( hr == DIERR_NOTINITIALIZED, "Initialize returned %#lx\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, NULL );
    ok( hr == E_POINTER, "Initialize returned %#lx\n", hr );

    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_NULL );
    ok( hr == DIERR_DEVICENOTREG, "Initialize returned %#lx\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_Sine );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );
    hr = IDirectInputEffect_Initialize( effect, instance, version, &GUID_Square );
    ok( hr == DI_OK, "Initialize returned %#lx\n", hr );

    hr = IDirectInputEffect_GetEffectGuid( effect, NULL );
    ok( hr == E_POINTER, "GetEffectGuid returned %#lx\n", hr );
    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_Square ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_Square ) );

    hr = IDirectInputEffect_GetParameters( effect, NULL, 0 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, NULL, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX5) + 2;
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX5);
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_STARTDELAY );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    desc.dwSize = sizeof(DIEFFECT_DX6);
    hr = IDirectInputEffect_GetParameters( effect, &desc, 0 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    desc.dwDuration = 0xdeadbeef;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", dwDuration );
    memset( &desc, 0xcd, sizeof(desc) );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    desc.dwFlags = 0;
    desc.dwStartDelay = 0xdeadbeef;
    flags = DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL |
            (version >= 0x700 ? DIEP_STARTDELAY : 0);
    hr = IDirectInputEffect_GetParameters( effect, &desc, flags );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%lu", dwGain );
    if (version >= 0x700) check_member( desc, expect_desc_init, "%lu", dwStartDelay );
    else ok( desc.dwStartDelay == 0xdeadbeef, "got dwStartDelay %#lx\n", desc.dwStartDelay );
    check_member( desc, expect_desc_init, "%lu", dwTriggerRepeatInterval );

    memset( &desc, 0xcd, sizeof(desc) );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    desc.dwFlags = 0;
    desc.lpEnvelope = NULL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == E_POINTER, "GetParameters returned %#lx\n", hr );
    desc.lpEnvelope = &envelope;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    envelope.dwSize = sizeof(DIENVELOPE);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );

    desc.dwFlags = 0;
    desc.cAxes = 0;
    desc.rgdwAxes = NULL;
    desc.rglDirection = NULL;
    desc.lpEnvelope = NULL;
    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = NULL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%#lx", dwTriggerButton );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    desc.dwFlags = DIEFF_OBJECTIDS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%#lx", dwTriggerButton );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    desc.dwFlags |= DIEFF_CARTESIAN;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );
    desc.dwFlags |= DIEFF_POLAR;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );
    desc.dwFlags |= DIEFF_SPHERICAL;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );

    desc.dwFlags |= DIEFF_SPHERICAL;
    desc.cAxes = 2;
    desc.rgdwAxes = axes;
    desc.rglDirection = directions;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    check_member( desc, expect_desc_init, "%lu", rgdwAxes[0] );
    check_member( desc, expect_desc_init, "%lu", rgdwAxes[1] );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    ok( desc.dwFlags == DIEFF_OBJECTIDS, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_OBJECTIDS );

    desc.dwFlags |= DIEFF_SPHERICAL;
    desc.lpEnvelope = &envelope;
    desc.cbTypeSpecificParams = sizeof(periodic);
    desc.lpvTypeSpecificParams = &periodic;
    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc_init, "%lu", dwDuration );
    check_member( desc, expect_desc_init, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%lu", dwGain );
    check_member( desc, expect_desc_init, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc_init, "%lu", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    check_member( desc, expect_desc_init, "%lu", rgdwAxes[0] );
    check_member( desc, expect_desc_init, "%lu", rgdwAxes[1] );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%lu", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc_init, "%lu", dwStartDelay );
    else ok( desc.dwStartDelay == 0xcdcdcdcd, "got dwStartDelay %#lx\n", desc.dwStartDelay );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Download returned %#lx\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, NULL, DIEP_NODOWNLOAD );
    ok( hr == E_POINTER, "SetParameters returned %#lx\n", hr );
    memset( &desc, 0, sizeof(desc) );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    desc.dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DURATION );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DURATION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    desc.dwTriggerButton = -1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DURATION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", dwDuration );
    check_member( desc, expect_desc_init, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc_init, "%lu", dwGain );
    check_member( desc, expect_desc_init, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc_init, "%lu", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%lu", cbTypeSpecificParams );
    check_member( desc, expect_desc_init, "%lu", dwStartDelay );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    flags = DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL | DIEP_NODOWNLOAD;
    if (version >= 0x700) flags |= DIEP_STARTDELAY;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.dwDuration = 0;
    flags = DIEP_DURATION | DIEP_GAIN | DIEP_SAMPLEPERIOD | DIEP_TRIGGERREPEATINTERVAL;
    if (version >= 0x700) flags |= DIEP_STARTDELAY;
    hr = IDirectInputEffect_GetParameters( effect, &desc, flags );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", dwDuration );
    check_member( desc, expect_desc, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc, "%lu", dwGain );
    check_member( desc, expect_desc_init, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc, "%lu", dwTriggerRepeatInterval );
    check_member( desc, expect_desc_init, "%lu", cAxes );
    check_member( desc, expect_desc_init, "%p", rglDirection );
    check_member( desc, expect_desc_init, "%p", lpEnvelope );
    check_member( desc, expect_desc_init, "%lu", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc, "%lu", dwStartDelay );
    else ok( desc.dwStartDelay == 0, "got dwStartDelay %#lx\n", desc.dwStartDelay );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    desc.lpEnvelope = NULL;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.lpEnvelope = &envelope;
    envelope.dwSize = 0;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_ENVELOPE | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    desc.lpEnvelope = &envelope;
    envelope.dwSize = sizeof(DIENVELOPE);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ENVELOPE );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( envelope, expect_envelope, "%lu", dwAttackLevel );
    check_member( envelope, expect_envelope, "%lu", dwAttackTime );
    check_member( envelope, expect_envelope, "%lu", dwFadeLevel );
    check_member( envelope, expect_envelope, "%lu", dwFadeTime );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    desc.dwFlags = 0;
    desc.cAxes = 0;
    desc.rgdwAxes = NULL;
    desc.rglDirection = NULL;
    desc.lpEnvelope = NULL;
    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = NULL;
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &desc, flags | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );

    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    desc.cAxes = 1;
    desc.rgdwAxes = axes;
    desc.rgdwAxes[0] = DIJOFS_X;
    desc.dwTriggerButton = DIJOFS_BUTTON( 1 );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_AXES | DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON | DIEP_NODOWNLOAD );
    ok( hr == DIERR_ALREADYINITIALIZED, "SetParameters returned %#lx\n", hr );

    desc.cAxes = 0;
    desc.dwFlags = DIEFF_OBJECTIDS;
    desc.rgdwAxes = axes;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc, "%lu", cAxes );
    check_member( desc, expect_desc, "%lu", rgdwAxes[0] );
    check_member( desc, expect_desc, "%lu", rgdwAxes[1] );
    check_member( desc, expect_desc, "%lu", rgdwAxes[2] );

    desc.dwFlags = DIEFF_OBJECTOFFSETS;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_AXES | DIEP_TRIGGERBUTTON );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    ok( desc.dwTriggerButton == 0x30, "got %#lx expected %#x\n", desc.dwTriggerButton, 0x30 );
    ok( desc.rgdwAxes[0] == 8, "got %#lx expected %#x\n", desc.rgdwAxes[0], 8 );
    ok( desc.rgdwAxes[1] == 0, "got %#lx expected %#x\n", desc.rgdwAxes[1], 0 );
    ok( desc.rgdwAxes[2] == 4, "got %#lx expected %#x\n", desc.rgdwAxes[2], 4 );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 0;
    desc.rglDirection = directions;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    desc.cAxes = 3;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 3;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    desc.dwFlags = DIEFF_SPHERICAL;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_SPHERICAL, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_SPHERICAL );
    check_member( desc, expect_desc, "%lu", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", cAxes );
    ok( desc.rglDirection[0] == 3000, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 3000 );
    ok( desc.rglDirection[1] == 30000, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], 30000 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %ld expected %d\n", desc.rglDirection[2], 0 );
    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 2;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_CARTESIAN, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_CARTESIAN );
    check_member( desc, expect_desc, "%lu", cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", cAxes );
    ok( desc.rglDirection[0] == 4330, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 4330 );
    ok( desc.rglDirection[1] == 2500, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], 2500 );
    ok( desc.rglDirection[2] == -8660, "got rglDirection[2] %ld expected %d\n", desc.rglDirection[2], -8660 );
    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 3;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DIERR_INCOMPLETEEFFECT, "Download returned %#lx\n", hr );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    desc.cbTypeSpecificParams = 0;
    desc.lpvTypeSpecificParams = &periodic;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
    desc.cbTypeSpecificParams = sizeof(DIPERIODIC);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( periodic, expect_periodic, "%lu", dwMagnitude );
    check_member( periodic, expect_periodic, "%ld", lOffset );
    check_member( periodic, expect_periodic, "%lu", dwPhase );
    check_member( periodic, expect_periodic, "%lu", dwPeriod );

    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DIERR_NOTDOWNLOADED, "Start returned %#lx\n", hr );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DIERR_NOTDOWNLOADED, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_download, 3 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_NOEFFECT, "Download returned %#lx\n", hr );

    hr = IDirectInputEffect_Start( effect, 1, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "Start returned %#lx\n", hr );

    set_hid_expect( file, &expect_start_solo, sizeof(expect_start_solo) );
    hr = IDirectInputEffect_Start( effect, 1, DIES_SOLO );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start_4, sizeof(expect_start_4) );
    hr = IDirectInputEffect_Start( effect, 4, 0 );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start_0, sizeof(expect_start_4) );
    hr = IDirectInputEffect_Start( effect, 0, 0 );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_download, 4 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_START );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_download, 3 * sizeof(struct hid_expect) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, 2 * sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Start returned %#lx\n", hr );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_NOEFFECT, "Unload returned %#lx\n", hr );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

    desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTIDS;
    desc.cAxes = 2;
    desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
    desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR;
    desc.rglDirection[0] = 3000;
    desc.rglDirection[1] = 0;
    desc.rglDirection[2] = 0;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_DIRECTION | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.rglDirection[0] = 0;

    desc.dwFlags = DIEFF_SPHERICAL;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_SPHERICAL, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_SPHERICAL );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 30000, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 30000 );
    ok( desc.rglDirection[1] == 0, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], 0 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %ld expected %d\n", desc.rglDirection[2], 0 );

    desc.dwFlags = DIEFF_CARTESIAN;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_CARTESIAN, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_CARTESIAN );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 5000, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 5000 );
    ok( desc.rglDirection[1] == -8660, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], -8660 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %ld expected %d\n", desc.rglDirection[2], 0 );

    desc.dwFlags = DIEFF_POLAR;
    desc.cAxes = 1;
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
    ok( desc.dwFlags == DIEFF_POLAR, "got flags %#lx, expected %#x\n", desc.dwFlags, DIEFF_POLAR );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    ok( desc.cAxes == 2, "got cAxes %lu expected 2\n", desc.cAxes );
    ok( desc.rglDirection[0] == 3000, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 3000 );
    ok( desc.rglDirection[1] == 0, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], 0 );
    ok( desc.rglDirection[2] == 0, "got rglDirection[2] %ld expected %d\n", desc.rglDirection[2], 0 );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );

    for (i = 1; i < 4; i++)
    {
        struct hid_expect expect_directions[] =
        {
            /* set periodic */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 5,
                .report_len = 2,
                .report_buf = {0x05,0x19},
            },
            /* set envelope */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 6,
                .report_len = 7,
                .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
            },
            /* update effect */
            {0},
            /* effect control */
            {
                .code = IOCTL_HID_WRITE_REPORT,
                .report_id = 2,
                .report_len = 4,
                .report_buf = {0x02,0x01,0x03,0x00},
            },
        };
        struct hid_expect expect_spherical =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = { 0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00, 0x01,
                            i >= 2 ? 0x55 : 0, i >= 3 ? 0x1c : 0 },
        };
        struct hid_expect expect_cartesian =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00,
                           0x01, i >= 2 ? 0x63 : 0, i >= 3 ? 0x1d : 0},
        };
        struct hid_expect expect_polar =
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03, 0x01, 0x02, 0x08, 0x01, 0x00, version >= 0x700 ? 0x06 : 0x00, 0x00,
                           0x01, i >= 2 ? 0x3f : 0, i >= 3 ? 0x00 : 0},
        };

        winetest_push_context( "%lu axes", i );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

        desc.dwFlags = DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
        desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR;
        desc.rgdwAxes[2] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR;
        desc.rglDirection[0] = 0;
        desc.rglDirection[1] = 0;
        desc.rglDirection[2] = 0;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_AXES | DIEP_NODOWNLOAD );
        ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

        desc.dwFlags = DIEFF_CARTESIAN;
        desc.cAxes = i == 3 ? 2 : 3;
        desc.rglDirection[0] = 1000;
        desc.rglDirection[1] = 2000;
        desc.rglDirection[2] = 3000;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
        ok( hr == DIERR_INVALIDPARAM, "SetParameters returned %#lx\n", hr );
        desc.cAxes = i;
        hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_DIRECTION | DIEP_NODOWNLOAD );
        ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

        desc.dwFlags = DIEFF_SPHERICAL;
        desc.cAxes = i;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
        ok( desc.cAxes == i, "got cAxes %lu expected 2\n", desc.cAxes );
        if (i == 1)
        {
            ok( desc.rglDirection[0] == 0, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 0 );
            ok( desc.rglDirection[1] == 0xcdcdcdcd, "got rglDirection[1] %ld expected %d\n",
                desc.rglDirection[1], 0xcdcdcdcd );
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %ld expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        }
        else
        {
            ok( desc.rglDirection[0] == 6343, "got rglDirection[0] %ld expected %d\n",
                desc.rglDirection[0], 6343 );
            if (i == 2)
            {
                ok( desc.rglDirection[1] == 0, "got rglDirection[1] %ld expected %d\n",
                    desc.rglDirection[1], 0 );
                ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %ld expected %d\n",
                    desc.rglDirection[2], 0xcdcdcdcd );
            }
            else
            {
                ok( desc.rglDirection[1] == 5330, "got rglDirection[1] %ld expected %d\n",
                    desc.rglDirection[1], 5330 );
                ok( desc.rglDirection[2] == 0, "got rglDirection[2] %ld expected %d\n",
                    desc.rglDirection[2], 0 );
            }
        }

        desc.dwFlags = DIEFF_CARTESIAN;
        desc.cAxes = i;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
        ok( desc.cAxes == i, "got cAxes %lu expected 2\n", desc.cAxes );
        ok( desc.rglDirection[0] == 1000, "got rglDirection[0] %ld expected %d\n", desc.rglDirection[0], 1000 );
        if (i == 1)
            ok( desc.rglDirection[1] == 0xcdcdcdcd, "got rglDirection[1] %ld expected %d\n",
                desc.rglDirection[1], 0xcdcdcdcd );
        else
            ok( desc.rglDirection[1] == 2000, "got rglDirection[1] %ld expected %d\n",
                desc.rglDirection[1], 2000 );
        if (i <= 2)
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %ld expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        else
            ok( desc.rglDirection[2] == 3000, "got rglDirection[2] %ld expected %d\n",
                desc.rglDirection[2], 3000 );

        desc.dwFlags = DIEFF_POLAR;
        desc.cAxes = 1;
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        if (i != 2) ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
        else ok( hr == DIERR_MOREDATA, "GetParameters returned %#lx\n", hr );
        desc.cAxes = 3;
        memset( desc.rglDirection, 0xcd, 3 * sizeof(LONG) );
        hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_DIRECTION );
        if (i != 2) ok( hr == DIERR_INVALIDPARAM, "GetParameters returned %#lx\n", hr );
        else
        {
            ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
            ok( desc.cAxes == i, "got cAxes %lu expected 2\n", desc.cAxes );
            ok( desc.rglDirection[0] == 15343, "got rglDirection[0] %ld expected %d\n",
                desc.rglDirection[0], 15343 );
            ok( desc.rglDirection[1] == 0, "got rglDirection[1] %ld expected %d\n", desc.rglDirection[1], 0 );
            ok( desc.rglDirection[2] == 0xcdcdcdcd, "got rglDirection[2] %ld expected %d\n",
                desc.rglDirection[2], 0xcdcdcdcd );
        }

        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %ld\n", ref );

        desc = expect_desc;
        desc.dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes = axes;
        desc.rglDirection = directions;
        desc.rglDirection[0] = 3000;
        desc.rglDirection[1] = 4000;
        desc.rglDirection[2] = 5000;
        flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
        expect_directions[2] = expect_spherical;
        set_hid_expect( file, expect_directions, sizeof(expect_directions) );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %ld\n", ref );
        set_hid_expect( file, NULL, 0 );

        desc = expect_desc;
        desc.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTIDS;
        desc.cAxes = i;
        desc.rgdwAxes = axes;
        desc.rglDirection = directions;
        desc.rglDirection[0] = 6000;
        desc.rglDirection[1] = 7000;
        desc.rglDirection[2] = 8000;
        flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
        expect_directions[2] = expect_cartesian;
        set_hid_expect( file, expect_directions, sizeof(expect_directions) );
        hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
        ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
        ref = IDirectInputEffect_Release( effect );
        ok( ref == 0, "Release returned %ld\n", ref );
        set_hid_expect( file, NULL, 0 );

        if (i == 2)
        {
            desc = expect_desc;
            desc.dwFlags = DIEFF_POLAR | DIEFF_OBJECTIDS;
            desc.cAxes = i;
            desc.rgdwAxes = axes;
            desc.rglDirection = directions;
            desc.rglDirection[0] = 9000;
            desc.rglDirection[1] = 10000;
            desc.rglDirection[2] = 11000;
            flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
            expect_directions[2] = expect_polar;
            set_hid_expect( file, expect_directions, sizeof(expect_directions) );
            hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &desc, &effect, NULL );
            ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
            ref = IDirectInputEffect_Release( effect );
            ok( ref == 0, "Release returned %ld\n", ref );
            set_hid_expect( file, NULL, 0 );
        }

        winetest_pop_context();
    }

    /* zero-ed effect parameters are sent */

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

    set_hid_expect( file, expect_download_0, sizeof(expect_download_0) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc_0, flags );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

    set_hid_expect( file, expect_download_1, sizeof(expect_download_1) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc_0, (flags & ~DIEP_ENVELOPE) );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );


    set_hid_expect( file, expect_download_2, sizeof(expect_download_2) );
    flags = version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    desc = expect_desc;
    desc.dwDuration = INFINITE;
    desc.dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_DURATION | DIEP_TRIGGERBUTTON );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_update, sizeof(expect_update) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */
    desc = expect_desc;
    desc.dwDuration = INFINITE;
    desc.dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_DURATION | DIEP_TRIGGERBUTTON );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_update, sizeof(expect_update) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    desc = expect_desc;
    desc.lpEnvelope = &envelope;
    desc.lpEnvelope->dwAttackTime = 1000;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_ENVELOPE );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_set_envelope, sizeof(expect_set_envelope) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_download_3, sizeof(expect_download_3) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.lpEnvelope = NULL;
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_NODOWNLOAD | DIEP_ENVELOPE );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    set_hid_expect( file, expect_download_4, sizeof(expect_download_4) );
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, 0 );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* these updates are sent asynchronously */

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Sine, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */
}

static void test_condition_effect( IDirectInputDevice8W *device, HANDLE file, DWORD version )
{
    struct hid_expect expect_create[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_1[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_create_2[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0xf1},
        },
    };
    struct hid_expect expect_create_3[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_4[] =
    {
        /* set condition, saturation 5000 */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0xa6,0x19,0xd9,0x7f,0x7f,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_create_5[] =
    {
        /* set condition, saturation out-of-bounds (-1) */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 8,
            .report_buf = {0x07,0x00,0xa6,0x19,0xd9,0xff,0xff,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x03,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_destroy =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x03, 0x00},
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] = {
        +3000,
        0,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONDITION expect_condition[4] =
    {
        {
            .lOffset = -500,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = -4000,
            .dwNegativeSaturation = -5000,
            .lDeadBand = 6000,
        },
        {
            .lOffset = 6000,
            .lPositiveCoefficient = 5000,
            .lNegativeCoefficient = -4000,
            .dwPositiveSaturation = 3000,
            .dwNegativeSaturation = 2000,
            .lDeadBand = 1000,
        },
        {
            .lOffset = -7000,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = 5000,
            .dwNegativeSaturation = 5000,
            .lDeadBand = 1000,
        },
        {
            .lOffset = -7000,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = 11000,
            .dwNegativeSaturation = -11000,
            .lDeadBand = 1000,
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 2,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = (void *)expect_condition,
        .dwStartDelay = 6000,
    };
    struct check_created_effect_params check_params = {0};
    DIENVELOPE envelope = {.dwSize = sizeof(DIENVELOPE)};
    DICONDITION condition[2] = {{0}};
    IDirectInputEffect *effect;
    LONG directions[4] = {0};
    DWORD axes[4] = {0};
    DIEFFECT desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .cAxes = 4,
        .rgdwAxes = axes,
        .rglDirection = directions,
        .lpEnvelope = &envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = condition,
    };
    HRESULT hr;
    ULONG ref;
    GUID guid;

    set_hid_expect( file, expect_create, sizeof(expect_create) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#lx\n", hr );
    ok( check_params.count == 1, "got count %lu, expected 1\n", check_params.count );

    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_Spring ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_Spring ) );

    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", dwDuration );
    check_member( desc, expect_desc, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc, "%lu", dwGain );
    check_member( desc, expect_desc, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc, "%lu", dwTriggerRepeatInterval );
    check_member( desc, expect_desc, "%lu", cAxes );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[0] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[1] );
    check_member( desc, expect_desc, "%ld", rglDirection[0] );
    check_member( desc, expect_desc, "%ld", rglDirection[1] );
    check_member( desc, expect_desc, "%lu", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc, "%lu", dwStartDelay );
    else ok( desc.dwStartDelay == 0, "got dwStartDelay %#lx\n", desc.dwStartDelay );
    check_member( envelope, expect_envelope, "%lu", dwAttackLevel );
    check_member( envelope, expect_envelope, "%lu", dwAttackTime );
    check_member( envelope, expect_envelope, "%lu", dwFadeLevel );
    check_member( envelope, expect_envelope, "%lu", dwFadeTime );
    check_member( condition[0], expect_condition[0], "%ld", lOffset );
    check_member( condition[0], expect_condition[0], "%ld", lPositiveCoefficient );
    check_member( condition[0], expect_condition[0], "%ld", lNegativeCoefficient );
    check_member( condition[0], expect_condition[0], "%lu", dwPositiveSaturation );
    check_member( condition[0], expect_condition[0], "%lu", dwNegativeSaturation );
    check_member( condition[0], expect_condition[0], "%ld", lDeadBand );
    check_member( condition[1], expect_condition[1], "%ld", lOffset );
    check_member( condition[1], expect_condition[1], "%ld", lPositiveCoefficient );
    check_member( condition[1], expect_condition[1], "%ld", lNegativeCoefficient );
    check_member( condition[1], expect_condition[1], "%lu", dwPositiveSaturation );
    check_member( condition[1], expect_condition[1], "%lu", dwNegativeSaturation );
    check_member( condition[1], expect_condition[1], "%ld", lDeadBand );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 1;
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DIERR_INVALIDPARAM, "CreateEffect returned %#lx\n", hr );
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_1, sizeof(expect_create_1) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 3;
    desc.rglDirection = directions;
    desc.rglDirection[0] = +3000;
    desc.rglDirection[1] = -2000;
    desc.rglDirection[2] = +1000;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_2, sizeof(expect_create_2) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 2;
    desc.rgdwAxes = axes;
    desc.rgdwAxes[0] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR;
    desc.rgdwAxes[1] = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR;
    desc.rglDirection = directions;
    desc.rglDirection[0] = +3000;
    desc.rglDirection[1] = -2000;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[1];
    set_hid_expect( file, expect_create_3, sizeof(expect_create_3) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    desc = expect_desc;
    desc.cAxes = 0;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[0];
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DIERR_MOREDATA, "SetParameters returned %#lx\n", hr );
    ok( desc.cbTypeSpecificParams == 1 * sizeof(DICONDITION), "got %lu\n", desc.cbTypeSpecificParams );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_SetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );
    desc.cbTypeSpecificParams = 0 * sizeof(DICONDITION);
    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_TYPESPECIFICPARAMS );
    ok( hr == DI_OK, "SetParameters returned %#lx\n", hr );
    ok( desc.cbTypeSpecificParams == 0 * sizeof(DICONDITION), "got %lu\n", desc.cbTypeSpecificParams );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );


    /* check saturation coefficients */
    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[2];

    set_hid_expect( file, expect_create_4, sizeof(expect_create_4) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = 1 * sizeof(DICONDITION);
    desc.lpvTypeSpecificParams = (void *)&expect_condition[3];

    set_hid_expect( file, expect_create_5, sizeof(expect_create_5) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );
}

static void test_constantforce_effect( IDirectInputDevice8W *device, HANDLE file, DWORD version )
{
    struct hid_expect expect_create_0[] =
    {
        /* set constant force, magnitude 5000 */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 9,
            .report_len = 3,
            .report_buf = {0x09,0x88,0x13},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x04,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_1[] =
    {
        /* set constantforce, magnitude 10000 */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 9,
            .report_len = 3,
            .report_buf = {0x09,0x10,0x27},
        },
        /* update envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x04,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };

    struct hid_expect expect_create_2[] =
    {
        /* set constantforce, magnitude -10000 */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 9,
            .report_len = 3,
            .report_buf = {0x09,0xf0,0xd8},
        },
        /* update envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 7,
            .report_buf = {0x06,0x19,0x4c,0x02,0x00,0x04,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {0x03,0x01,0x04,0x08,0x01,0x00,version >= 0x700 ? 0x06 : 0x00,0x00,0x01,0x3f,0x00},
        },
    };
    struct hid_expect expect_destroy =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {0x02, 0x01, 0x03, 0x00},
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] = {
        +3000,
        0,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONSTANTFORCE input_constant_force[4] =
    {
        {
            .lMagnitude = 5000
        },
        {
            .lMagnitude = 10000
        },
        {
            .lMagnitude = 16000
        },
        {
            .lMagnitude = -16000
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 3,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = sizeof(DICONSTANTFORCE),
        .lpvTypeSpecificParams = (void *)input_constant_force,
        .dwStartDelay = 6000,
    };
    struct check_created_effect_params check_params = {0};
    DIENVELOPE envelope = {.dwSize = sizeof(DIENVELOPE)};
    DICONSTANTFORCE constant_force[1] = {{0}};
    IDirectInputEffect *effect;
    LONG directions[3] = {0,0,0};
    DWORD axes[3] = {0,0,0};
    DIEFFECT desc =
    {
        .dwSize = version >= 0x700 ? sizeof(DIEFFECT_DX6) : sizeof(DIEFFECT_DX5),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .cAxes = 3,
        .rgdwAxes = axes,
        .rglDirection = directions,
        .lpEnvelope = &envelope,
        .cbTypeSpecificParams = sizeof(DICONSTANTFORCE),
        .lpvTypeSpecificParams = constant_force,
    };
    HRESULT hr;
    ULONG ref;
    GUID guid;

    set_hid_expect( file, expect_create_0, sizeof(expect_create_0) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_ConstantForce, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#lx\n", hr );
    ok( check_params.count == 1, "got count %lu, expected 1\n", check_params.count );

    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_ConstantForce ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_ConstantForce ) );

    hr = IDirectInputEffect_GetParameters( effect, &desc, version >= 0x700 ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5 );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", dwDuration );
    check_member( desc, expect_desc, "%lu", dwSamplePeriod );
    check_member( desc, expect_desc, "%lu", dwGain );
    check_member( desc, expect_desc, "%#lx", dwTriggerButton );
    check_member( desc, expect_desc, "%lu", dwTriggerRepeatInterval );
    check_member( desc, expect_desc, "%lu", cAxes );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[0] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[1] );
    check_member( desc, expect_desc, "%ld", rglDirection[0] );
    check_member( desc, expect_desc, "%ld", rglDirection[1] );
    check_member( desc, expect_desc, "%lu", cbTypeSpecificParams );
    if (version >= 0x700) check_member( desc, expect_desc, "%lu", dwStartDelay );
    else ok( desc.dwStartDelay == 0, "got dwStartDelay %#lx\n", desc.dwStartDelay );
    check_member( envelope, expect_envelope, "%lu", dwAttackLevel );
    check_member( envelope, expect_envelope, "%lu", dwAttackTime );
    check_member( envelope, expect_envelope, "%lu", dwFadeLevel );
    check_member( envelope, expect_envelope, "%lu", dwFadeTime );
    check_member( constant_force[0], input_constant_force[0], "%ld", lMagnitude );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = 2 * sizeof(DICONSTANTFORCE);
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_ConstantForce, &desc, &effect, NULL );
    ok( hr == DIERR_INVALIDPARAM, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );


    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    desc.lpvTypeSpecificParams = (void *)&input_constant_force[1];

    set_hid_expect( file, expect_create_1, sizeof(expect_create_1) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_ConstantForce, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );


    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    desc.lpvTypeSpecificParams = (void *)&input_constant_force[2];

    set_hid_expect( file, expect_create_1, sizeof(expect_create_1) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_ConstantForce, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );


    desc = expect_desc;
    desc.cAxes = 1;
    desc.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    desc.lpvTypeSpecificParams = (void *)&input_constant_force[3];

    set_hid_expect( file, expect_create_2, sizeof(expect_create_2) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_ConstantForce, &desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );
}


static BOOL test_force_feedback_joystick( DWORD version )
{
#include "psh_hid_macros.h"
    const unsigned char report_descriptor[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 6),
                INPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MAXIMUM(1, 0x7f),
                LOGICAL_MINIMUM(1, 0x00),
                REPORT_SIZE(1, 7),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    USAGE(1, PID_USAGE_DC_STOP_ALL_EFFECTS),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 2),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    USAGE(1, PID_USAGE_ET_CONSTANT_FORCE),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 4),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 4),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Z),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 3),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 4),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(2, 0x00ff),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 0x00008ca0),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_PERIODIC_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_ENVELOPE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_ATTACK_LEVEL),
                USAGE(1, PID_USAGE_FADE_LEVEL),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_ATTACK_TIME),
                USAGE(1, PID_USAGE_FADE_TIME),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                PHYSICAL_MAXIMUM(1, 0),
                UNIT_EXPONENT(1, 0),
                UNIT(1, 0),
            END_COLLECTION,


            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 7),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_CP_OFFSET),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,


            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 8),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONSTANT_FORCE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 9),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(2, 0xd8f0),
                LOGICAL_MAXIMUM(2, 0x2710),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_descriptor) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps =
        {
            .InputReportByteLength = 5,
            .OutputReportByteLength = 11,
        },
        .attributes = default_attributes,
    };
    const DIDEVCAPS expect_caps =
    {
        .dwSize = sizeof(DIDEVCAPS),
        .dwFlags = DIDC_FORCEFEEDBACK | DIDC_ATTACHED | DIDC_EMULATED | DIDC_STARTDELAY |
                   DIDC_FFFADE | DIDC_FFATTACK | DIDC_DEADBAND | DIDC_SATURATION,
        .dwDevType = version >= 0x800 ? DIDEVTYPE_HID | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DI8DEVTYPE_JOYSTICK
                                      : DIDEVTYPE_HID | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_JOYSTICK,
        .dwAxes = 3,
        .dwButtons = 2,
        .dwFFSamplePeriod = 1000000,
        .dwFFMinTimeResolution = 1000000,
        .dwHardwareRevision = 1,
        .dwFFDriverVersion = 1,
    };
    struct hid_expect expect_acquire_autocenter_on[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0x19},
        },
    };
    struct hid_expect expect_acquire_autocenter_off[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x02},
            .broken_id = 8, /* Win8 sends them in the reverse order */
        },
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0x19},
            .broken_id = 1, /* Win8 sends them in the reverse order */
        },
    };
    struct hid_expect expect_reset[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    struct hid_expect expect_set_device_gain_1 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 8,
        .report_len = 2,
        .report_buf = {8, 0x19},
    };
    struct hid_expect expect_set_device_gain_2 =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 8,
        .report_len = 2,
        .report_buf = {8, 0x33},
    };
    struct hid_expect expect_stop_all[] =
    {
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x02},
        },
    };
    const DIDEVICEINSTANCEW expect_devinst =
    {
        .dwSize = sizeof(DIDEVICEINSTANCEW),
        .guidInstance = expect_guid_product,
        .guidProduct = expect_guid_product,
        .dwDevType = version >= 0x800 ? DIDEVTYPE_HID | (DI8DEVTYPEJOYSTICK_LIMITED << 8) | DI8DEVTYPE_JOYSTICK
                                      : DIDEVTYPE_HID | (DIDEVTYPEJOYSTICK_UNKNOWN << 8) | DIDEVTYPE_JOYSTICK,
        .tszInstanceName = L"Wine Test",
        .tszProductName = L"Wine Test",
        .guidFFDriver = IID_IDirectInputPIDDriver,
        .wUsagePage = HID_USAGE_PAGE_GENERIC,
        .wUsage = HID_USAGE_GENERIC_JOYSTICK,
    };
    const DIDEVICEOBJECTINSTANCEW expect_objects_5[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(0)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"X Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = 0x4,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(1)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Y Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwOfs = 0x8,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(2)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Z Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0x30,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(0)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 0",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x1,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = 0x31,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(1)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 1",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x2,
            .wReportId = 1,
        },
    };
    const DIDEVICEOBJECTINSTANCEW expect_objects[] =
    {
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_ZAxis,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(2)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Z Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_YAxis,
            .dwOfs = 0x4,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(1)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"Y Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_XAxis,
            .dwOfs = 0x8,
            .dwType = DIDFT_ABSAXIS|DIDFT_MAKEINSTANCE(0)|DIDFT_FFACTUATOR,
            .dwFlags = DIDOI_ASPECTPOSITION|DIDOI_FFACTUATOR,
            .tszName = L"X Axis",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = version >= 0x800 ? 0x6c : 0x10,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(0)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 0",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x1,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Button,
            .dwOfs = version >= 0x800 ? 0x6d : 0x11,
            .dwType = DIDFT_PSHBUTTON|DIDFT_MAKEINSTANCE(1)|DIDFT_FFEFFECTTRIGGER,
            .dwFlags = DIDOI_FFEFFECTTRIGGER,
            .tszName = L"Button 1",
            .wCollectionNumber = 1,
            .wUsagePage = HID_USAGE_PAGE_BUTTON,
            .wUsage = 0x2,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x74 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(12)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"DC Device Reset",
            .wCollectionNumber = 4,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DC_DEVICE_RESET,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x75 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(13)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"DC Stop All Effects",
            .wCollectionNumber = 4,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DC_STOP_ALL_EFFECTS,
            .wReportId = 1,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x10 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(14)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Effect Block Index",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_BLOCK_INDEX,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x76 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(15)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Start",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_START,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x77 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(16)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Start Solo",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_START_SOLO,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x78 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(17)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Op Effect Stop",
            .wCollectionNumber = 6,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_OP_EFFECT_STOP,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x14 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(18)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Loop Count",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_LOOP_COUNT,
            .wReportId = 2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x18 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(19)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Effect Block Index",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_BLOCK_INDEX,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x79 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(20)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Square",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SQUARE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7a : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(21)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Sine",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SINE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7b : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(22)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Spring",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_SPRING,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(23)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"ET Constant Force",
            .wCollectionNumber = 8,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ET_CONSTANT_FORCE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7d : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(24)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Z Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Z,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7e : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(25)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Y Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_Y,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x7f : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(26)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"X Axis",
            .wCollectionNumber = 9,
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_X,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x80 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(27)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Direction Enable",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DIRECTION_ENABLE,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x1c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(28)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Start Delay",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_START_DELAY,
            .wReportId = 3,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x20 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(29)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Duration",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DURATION,
            .wReportId = 3,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x24 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(30)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Trigger Button",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_TRIGGER_BUTTON,
            .wReportId = 3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x28 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(31)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 31",
            .wCollectionNumber = 10,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 2,
            .wReportId = 3,
            .wExponent = -2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x2c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(32)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 32",
            .wCollectionNumber = 10,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 1,
            .wReportId = 3,
            .wExponent = -2,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x30 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(33)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Magnitude",
            .wCollectionNumber = 11,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_MAGNITUDE,
            .wReportId = 5,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x34 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(34)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Fade Level",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_FADE_LEVEL,
            .wReportId = 6,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x38 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(35)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Attack Level",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ATTACK_LEVEL,
            .wReportId = 6,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x3c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(36)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Fade Time",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_FADE_TIME,
            .wReportId = 6,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x40 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(37)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Attack Time",
            .wCollectionNumber = 12,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_ATTACK_TIME,
            .wReportId = 6,
            .dwDimension = 0x1003,
            .wExponent = -3,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x44 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(38)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 38",
            .wCollectionNumber = 14,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 2,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x48 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(39)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Unknown 39",
            .wCollectionNumber = 14,
            .wUsagePage = HID_USAGE_PAGE_ORDINAL,
            .wUsage = 1,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x4c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(40)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"CP Offset",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_CP_OFFSET,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x50 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(41)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Negative Coefficient",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_NEGATIVE_COEFFICIENT,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x54 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(42)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Positive Coefficient",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_POSITIVE_COEFFICIENT,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x58 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(43)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Negative Saturation",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_NEGATIVE_SATURATION,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x5c : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(44)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Positive Saturation",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_POSITIVE_SATURATION,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x60 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(45)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Dead Band",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEAD_BAND,
            .wReportId = 7,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x64 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(46)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Device Gain",
            .wCollectionNumber = 15,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_GAIN,
            .wReportId = 8,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwOfs = version >= 0x800 ? 0x68 : 0,
            .dwType = DIDFT_NODATA|DIDFT_MAKEINSTANCE(47)|DIDFT_OUTPUT,
            .dwFlags = 0x80008000,
            .tszName = L"Magnitude",
            .wCollectionNumber = 16,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_MAGNITUDE,
            .wReportId = 9,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(0),
            .tszName = L"Collection 0 - Joystick",
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_JOYSTICK,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(1),
            .tszName = L"Collection 1 - Joystick",
            .wUsagePage = HID_USAGE_PAGE_GENERIC,
            .wUsage = HID_USAGE_GENERIC_JOYSTICK,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(2),
            .tszName = L"Collection 2 - PID State Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_STATE_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(3),
            .tszName = L"Collection 3 - PID Device Control Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_CONTROL_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(4),
            .tszName = L"Collection 4 - PID Device Control",
            .wCollectionNumber = 3,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_CONTROL,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(5),
            .tszName = L"Collection 5 - Effect Operation Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_OPERATION_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(6),
            .tszName = L"Collection 6 - Effect Operation",
            .wCollectionNumber = 5,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_OPERATION,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(7),
            .tszName = L"Collection 7 - Set Effect Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_EFFECT_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(8),
            .tszName = L"Collection 8 - Effect Type",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_EFFECT_TYPE,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(9),
            .tszName = L"Collection 9 - Axes Enable",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_AXES_ENABLE,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(10),
            .tszName = L"Collection 10 - Direction",
            .wCollectionNumber = 7,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DIRECTION,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(11),
            .tszName = L"Collection 11 - Set Periodic Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_PERIODIC_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(12),
            .tszName = L"Collection 12 - Set Envelope Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_ENVELOPE_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(13),
            .tszName = L"Collection 13 - Set Condition Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_CONDITION_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(14),
            .tszName = L"Collection 14 - Type Specific Block Offset",
            .wCollectionNumber = 13,
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(15),
            .tszName = L"Collection 15 - Device Gain Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_DEVICE_GAIN_REPORT,
        },
        {
            .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
            .guidType = GUID_Unknown,
            .dwType = DIDFT_COLLECTION|DIDFT_NODATA|DIDFT_MAKEINSTANCE(16),
            .tszName = L"Collection 16 - Set Constant Force Report",
            .wUsagePage = HID_USAGE_PAGE_PID,
            .wUsage = PID_USAGE_SET_CONSTANT_FORCE_REPORT,
        },
    };
    const DIEFFECTINFOW expect_effects[] =
    {
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_ConstantForce,
            .dwEffType = DIEFT_CONSTANTFORCE | DIEFT_STARTDELAY | DIEFT_FFFADE | DIEFT_FFATTACK,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .tszName = L"GUID_ConstantForce",
        },
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Square,
            .dwEffType = DIEFT_PERIODIC | DIEFT_STARTDELAY | DIEFT_FFFADE | DIEFT_FFATTACK,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .tszName = L"GUID_Square",
        },
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Sine,
            .dwEffType = DIEFT_PERIODIC | DIEFT_STARTDELAY | DIEFT_FFFADE | DIEFT_FFATTACK,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE,
            .tszName = L"GUID_Sine",
        },
        {
            .dwSize = sizeof(DIEFFECTINFOW),
            .guid = GUID_Spring,
            .dwEffType = DIEFT_CONDITION | DIEFT_STARTDELAY | DIEFT_DEADBAND | DIEFT_SATURATION,
            .dwStaticParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION,
            .dwDynamicParams = DIEP_AXES | DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_STARTDELAY |
                              DIEP_DURATION,
            .tszName = L"GUID_Spring",
        }
    };

    struct check_objects_todos todo_objects_5[ARRAY_SIZE(expect_objects_5)] =
    {
        {.guid = TRUE, .type = TRUE, .usage = TRUE, .name = TRUE},
        {0},
        {.guid = TRUE, .type = TRUE, .usage = TRUE, .name = TRUE},
    };
    struct check_objects_params check_objects_params =
    {
        .version = version,
        .expect_count = version < 0x700 ? ARRAY_SIZE(expect_objects_5) : ARRAY_SIZE(expect_objects),
        .expect_objs = version < 0x700 ? expect_objects_5 : expect_objects,
        .todo_objs = version < 0x700 ? todo_objects_5 : NULL,
        .todo_extra = version < 0x700,
    };
    struct check_effects_params check_effects_params =
    {
        .expect_count = ARRAY_SIZE(expect_effects),
        .expect_effects = expect_effects,
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    IDirectInputDevice8W *device = NULL;
    DIDEVICEOBJECTDATA objdata = {0};
    DIEFFECTINFOW effectinfo = {0};
    DIEFFESCAPE escape = {0};
    DIDEVCAPS caps = {0};
    char buffer[1024];
    ULONG res, ref;
    HANDLE file;
    HRESULT hr;
    HWND hwnd;

    winetest_push_context( "%#lx", version );
    cleanup_registry_keys();

    desc.report_descriptor_len = sizeof(report_descriptor);
    memcpy( desc.report_descriptor_buf, report_descriptor, sizeof(report_descriptor) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    check_dinput_devices( version, &devinst );

    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member( devinst, expect_devinst, "%lu", dwSize );
    todo_wine
    check_member_guid( devinst, expect_devinst, guidInstance );
    check_member_guid( devinst, expect_devinst, guidProduct );
    check_member( devinst, expect_devinst, "%#lx", dwDevType );
    check_member_wstr( devinst, expect_devinst, tszInstanceName );
    check_member_wstr( devinst, expect_devinst, tszProductName );
    check_member_guid( devinst, expect_devinst, guidFFDriver );
    check_member( devinst, expect_devinst, "%04x", wUsagePage );
    check_member( devinst, expect_devinst, "%04x", wUsage );

    caps.dwSize = sizeof(DIDEVCAPS);
    hr = IDirectInputDevice8_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetCapabilities returned %#lx\n", hr );
    check_member( caps, expect_caps, "%lu", dwSize );
    check_member( caps, expect_caps, "%#lx", dwFlags );
    check_member( caps, expect_caps, "%#lx", dwDevType );
    check_member( caps, expect_caps, "%lu", dwAxes );
    check_member( caps, expect_caps, "%lu", dwButtons );
    check_member( caps, expect_caps, "%lu", dwPOVs );
    check_member( caps, expect_caps, "%lu", dwFFSamplePeriod );
    check_member( caps, expect_caps, "%lu", dwFFMinTimeResolution );
    check_member( caps, expect_caps, "%lu", dwFirmwareRevision );
    check_member( caps, expect_caps, "%lu", dwHardwareRevision );
    check_member( caps, expect_caps, "%lu", dwFFDriverVersion );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    ok( prop_dword.dwData == 10000, "got %lu expected %u\n", prop_dword.dwData, 10000 );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );

    hr = IDirectInputDevice8_EnumObjects( device, check_objects, &check_objects_params, DIDFT_ALL );
    ok( hr == DI_OK, "EnumObjects returned %#lx\n", hr );
    ok( check_objects_params.index >= check_objects_params.expect_count, "missing %u objects\n",
        check_objects_params.expect_count - check_objects_params.index );

    res = 0;
    hr = IDirectInputDevice8_EnumEffects( device, check_effect_count, &res, 0xfe );
    ok( hr == DI_OK, "EnumEffects returned %#lx\n", hr );
    ok( res == 0, "got %lu expected %u\n", res, 0 );
    res = 0;
    hr = IDirectInputDevice8_EnumEffects( device, check_effect_count, &res, DIEFT_PERIODIC );
    ok( hr == DI_OK, "EnumEffects returned %#lx\n", hr );
    ok( res == 2, "got %lu expected %u\n", res, 2 );
    hr = IDirectInputDevice8_EnumEffects( device, check_effects, &check_effects_params, DIEFT_ALL );
    ok( hr == DI_OK, "EnumEffects returned %#lx\n", hr );
    ok( check_effects_params.index >= check_effects_params.expect_count, "missing %u effects\n",
        check_effects_params.expect_count - check_effects_params.index );

    effectinfo.dwSize = sizeof(DIEFFECTINFOW);
    hr = IDirectInputDevice8_GetEffectInfo( device, &effectinfo, &GUID_ConstantForce );
    ok( hr == DI_OK, "GetEffectInfo returned %#lx\n", hr );
    check_member_guid( effectinfo, expect_effects[0], guid );
    check_member( effectinfo, expect_effects[0], "%#lx", dwEffType );
    check_member( effectinfo, expect_effects[0], "%#lx", dwStaticParams );
    check_member( effectinfo, expect_effects[0], "%#lx", dwDynamicParams );
    check_member_wstr( effectinfo, expect_effects[0], tszName );

    effectinfo.dwSize = sizeof(DIEFFECTINFOW);
    hr = IDirectInputDevice8_GetEffectInfo( device, &effectinfo, &GUID_Square );
    ok( hr == DI_OK, "GetEffectInfo returned %#lx\n", hr );
    check_member_guid( effectinfo, expect_effects[1], guid );
    check_member( effectinfo, expect_effects[1], "%#lx", dwEffType );
    check_member( effectinfo, expect_effects[1], "%#lx", dwStaticParams );
    check_member( effectinfo, expect_effects[1], "%#lx", dwDynamicParams );
    check_member_wstr( effectinfo, expect_effects[1], tszName );

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned: %#lx\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );

    file = CreateFileW( prop_guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    hwnd = create_foreground_window( FALSE );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#lx\n", hr );

    prop_dword.diph.dwHow = DIPH_BYUSAGE;
    prop_dword.diph.dwObj = MAKELONG( HID_USAGE_GENERIC_X, HID_USAGE_PAGE_GENERIC );
    prop_dword.dwData = DIPROPAUTOCENTER_ON;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DIERR_UNSUPPORTED, "SetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );
    prop_dword.diph.dwHow = DIPH_DEVICE;
    prop_dword.diph.dwObj = 0;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );

    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DIERR_INVALIDPARAM, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_READONLY, "SetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetForceFeedbackState returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "SendForceFeedbackCommand returned %#lx\n", hr );

    escape.dwSize = sizeof(DIEFFESCAPE);
    escape.dwCommand = 0;
    escape.lpvInBuffer = buffer;
    escape.cbInBuffer = 10;
    escape.lpvOutBuffer = buffer + 10;
    escape.cbOutBuffer = 10;
    hr = IDirectInputDevice8_Escape( device, &escape );
    todo_wine
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "Escape returned: %#lx\n", hr );

    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#lx\n", hr );
    prop_dword.dwData = DIPROPAUTOCENTER_OFF;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire_autocenter_off, sizeof(expect_acquire_autocenter_off) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    prop_dword.dwData = DIPROPAUTOCENTER_ON;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_AUTOCENTER, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_AUTOCENTER returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire_autocenter_on, sizeof(expect_acquire_autocenter_on) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, &expect_set_device_gain_2, sizeof(expect_set_device_gain_2) );
    prop_dword.dwData = 2000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, &expect_set_device_gain_1, sizeof(expect_set_device_gain_1) );
    prop_dword.dwData = 1000;
    hr = IDirectInputDevice8_SetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "SetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IDirectInputDevice8_Escape( device, &escape );
    todo_wine
    ok( hr == DIERR_UNSUPPORTED, "Escape returned: %#lx\n", hr );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    todo_wine
    ok( hr == 0x80040301, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    todo_wine
    ok( hr == 0x80040301, "GetForceFeedbackState returned %#lx\n", hr );

    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, 0xdeadbeef );
    ok( hr == DIERR_INVALIDPARAM, "SendForceFeedbackCommand returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire_autocenter_on, sizeof(expect_acquire_autocenter_on) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    set_hid_expect( file, expect_stop_all, sizeof(expect_stop_all) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_STOPALL );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_PAUSE );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_CONTINUE );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSON );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSOFF );
    ok( hr == HIDP_STATUS_USAGE_NOT_FOUND, "SendForceFeedbackCommand returned %#lx\n", hr );

    objdata.dwOfs = 0x1e;
    objdata.dwData = 0x80;
    res = 1;
    hr = IDirectInputDevice8_SendDeviceData( device, sizeof(DIDEVICEOBJECTDATA), &objdata, &res, 0 );
    if (version < 0x800) ok( hr == DI_OK, "SendDeviceData returned %#lx\n", hr );
    else todo_wine ok( hr == DIERR_INVALIDPARAM, "SendDeviceData returned %#lx\n", hr );

    test_constantforce_effect( device, file, version );
    test_periodic_effect( device, file, version );
    test_condition_effect( device, file, version );

    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    DestroyWindow( hwnd );
    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );
    cleanup_registry_keys();
    winetest_pop_context();

    return device != NULL;
}

static void test_condition_effect_six_axes( IDirectInputDevice8W *device, HANDLE file )
{
    struct hid_expect expect_create[] =
    {
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0x7f,0x7f,0x7f,0xff,0xff,0xff},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0x80,0x7f,0x80,0xff,0xff,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0x7f,0xff,0x80,0x00,0xff,0xff},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0xff,0x7f,0xff,0xff,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 9,
            .report_buf = {0x07,0x00,0x00,0x7f,0x7f,0xff,0xff,0x00,0xff},
        },
        /* create effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 15,
            .report_buf = {0x03,0x01,0x03,0x40,0x01,0x00,0x06,0x00,0x01,0x7f,0x2a,0xea,0xf5,0x1f,0x00},
        },
    };
    struct hid_expect expect_destroy[] =
    {
        /* effect operation */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {0x02,0x01,0x03,0x00},
        },
    };
    static const DWORD expect_axes[6] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 4 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 5 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 3 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[6] =
    {
        9000,
        6000,
        33000,
        34500,
        4500,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONDITION expect_condition[6] =
    {
        {
            .lOffset = -10000,
            .lPositiveCoefficient = -10000,
            .lNegativeCoefficient = -10000,
            .dwPositiveSaturation = 0,
            .dwNegativeSaturation = 0,
            .lDeadBand = 0,
        },
        {
            .lOffset = 10000,
            .lPositiveCoefficient = 10000,
            .lNegativeCoefficient = 10000,
            .dwPositiveSaturation = 10000,
            .dwNegativeSaturation = 10000,
            .lDeadBand = 10000,
        },
        {
            .lOffset = -10000,
            .lPositiveCoefficient = +10000,
            .lNegativeCoefficient = -10000,
            .dwPositiveSaturation = +10000,
            .dwNegativeSaturation = -10000,
            .lDeadBand = 0,
        },
        {
            .lOffset = +10000,
            .lPositiveCoefficient = 0,
            .lNegativeCoefficient = -10000,
            .dwPositiveSaturation = 0,
            .dwNegativeSaturation = -10000,
            .lDeadBand = 20000,
        },
        {
            .lOffset = 0,
            .lPositiveCoefficient = +10000,
            .lNegativeCoefficient = 0,
            .dwPositiveSaturation = +10000,
            .dwNegativeSaturation = 0,
            .lDeadBand = 0,
        },
        {
            .lOffset = 10000,
            .lPositiveCoefficient = +10000,
            .lNegativeCoefficient = 0,
            .dwPositiveSaturation = +10000,
            .dwNegativeSaturation = 0,
            .lDeadBand = 20000,
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = sizeof(DIEFFECT_DX6),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 6,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = 6 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = (void *)expect_condition,
        .dwStartDelay = 6000,
    };
    struct check_created_effect_params check_params = {0};
    DIENVELOPE envelope = {.dwSize = sizeof(DIENVELOPE)};
    DICONDITION condition[6] = {{0}};
    IDirectInputEffect *effect;
    LONG directions[6] = {0};
    DWORD axes[6] = {0};
    DIEFFECT desc =
    {
        .dwSize = sizeof(DIEFFECT_DX6),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .cAxes = 6,
        .rgdwAxes = axes,
        .rglDirection = directions,
        .lpEnvelope = &envelope,
        .cbTypeSpecificParams = 6 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = condition,
    };
    HRESULT hr;
    ULONG ref;
    GUID guid;

    set_hid_expect( file, expect_create, sizeof(expect_create) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &expect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    check_params.expect_effect = effect;
    hr = IDirectInputDevice8_EnumCreatedEffectObjects( device, check_created_effect_objects, &check_params, 0 );
    ok( hr == DI_OK, "EnumCreatedEffectObjects returned %#lx\n", hr );
    ok( check_params.count == 1, "got count %lu, expected 1\n", check_params.count );

    hr = IDirectInputEffect_GetEffectGuid( effect, &guid );
    ok( hr == DI_OK, "GetEffectGuid returned %#lx\n", hr );
    ok( IsEqualGUID( &guid, &GUID_Spring ), "got guid %s, expected %s\n", debugstr_guid( &guid ),
        debugstr_guid( &GUID_Spring ) );

    hr = IDirectInputEffect_GetParameters( effect, &desc, DIEP_ALLPARAMS );
    ok( hr == DI_OK, "GetParameters returned %#lx\n", hr );
    check_member( desc, expect_desc, "%lu", cAxes );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[0] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[1] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[2] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[3] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[4] );
    check_member( desc, expect_desc, "%#lx", rgdwAxes[5] );
    check_member( desc, expect_desc, "%ld", rglDirection[0] );
    check_member( desc, expect_desc, "%ld", rglDirection[1] );
    check_member( desc, expect_desc, "%ld", rglDirection[2] );
    check_member( desc, expect_desc, "%ld", rglDirection[3] );
    check_member( desc, expect_desc, "%ld", rglDirection[4] );
    check_member( desc, expect_desc, "%ld", rglDirection[5] );
    check_member( desc, expect_desc, "%lu", cbTypeSpecificParams );

    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );
}

static BOOL test_force_feedback_six_axes(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_descriptor[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                USAGE(1, HID_USAGE_GENERIC_RX),
                USAGE(1, HID_USAGE_GENERIC_RY),
                USAGE(1, HID_USAGE_GENERIC_RZ),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 6),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 6),
                INPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MAXIMUM(1, 0x7f),
                LOGICAL_MINIMUM(1, 0x00),
                REPORT_SIZE(1, 7),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    USAGE(1, PID_USAGE_DC_STOP_ALL_EFFECTS),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 2),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    USAGE(1, PID_USAGE_ET_CONSTANT_FORCE),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 4),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 4),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Z),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_RX),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_RY),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_RZ),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 6),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|3),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|4),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|5),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|6),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(2, 0x00ff),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 0x00008ca0),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 6),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_PERIODIC_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_ENVELOPE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_ATTACK_LEVEL),
                USAGE(1, PID_USAGE_FADE_LEVEL),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_ATTACK_TIME),
                USAGE(1, PID_USAGE_FADE_TIME),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                PHYSICAL_MAXIMUM(1, 0),
                UNIT_EXPONENT(1, 0),
                UNIT(1, 0),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 7),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|3),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|4),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|5),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|6),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 6),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                REPORT_SIZE(1, 2),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_CP_OFFSET),
                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,


            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 8),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONSTANT_FORCE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 9),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(2, 0xd8f0),
                LOGICAL_MAXIMUM(2, 0x2710),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_descriptor) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps =
        {
            .InputReportByteLength = 8,
            .OutputReportByteLength = 15,
        },
        .attributes = default_attributes,
    };
    const DIDEVCAPS expect_caps =
    {
        .dwSize = sizeof(DIDEVCAPS),
        .dwFlags = DIDC_FORCEFEEDBACK | DIDC_ATTACHED | DIDC_EMULATED | DIDC_STARTDELAY |
                   DIDC_FFFADE | DIDC_FFATTACK | DIDC_DEADBAND | DIDC_SATURATION,
        .dwDevType = DIDEVTYPE_HID | (DI8DEVTYPE1STPERSON_LIMITED << 8) | DI8DEVTYPE_1STPERSON,
        .dwAxes = 6,
        .dwButtons = 2,
        .dwFFSamplePeriod = 1000000,
        .dwFFMinTimeResolution = 1000000,
        .dwHardwareRevision = 1,
        .dwFFDriverVersion = 1,
    };
    const DIDEVICEINSTANCEW expect_devinst =
    {
        .dwSize = sizeof(DIDEVICEINSTANCEW),
        .guidInstance = expect_guid_product,
        .guidProduct = expect_guid_product,
        .dwDevType = DIDEVTYPE_HID | (DI8DEVTYPE1STPERSON_LIMITED << 8) | DI8DEVTYPE_1STPERSON,
        .tszInstanceName = L"Wine Test",
        .tszProductName = L"Wine Test",
        .guidFFDriver = IID_IDirectInputPIDDriver,
        .wUsagePage = HID_USAGE_PAGE_GENERIC,
        .wUsage = HID_USAGE_GENERIC_JOYSTICK,
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    struct hid_expect expect_acquire[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        /* device gain */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 2,
            .report_buf = {8, 0xff},
        },
    };
    struct hid_expect expect_reset[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    IDirectInputDevice8W *device = NULL;
    DIDEVCAPS caps = {0};
    DWORD version = 0x800;
    HANDLE file;
    HRESULT hr;
    HWND hwnd;
    ULONG ref;

    winetest_push_context( "%#lx", version );
    cleanup_registry_keys();

    desc.report_descriptor_len = sizeof(report_descriptor);
    memcpy( desc.report_descriptor_buf, report_descriptor, sizeof(report_descriptor) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;
    if (FAILED(hr = dinput_test_create_device( version, &devinst, &device ))) goto done;

    check_dinput_devices( version, &devinst );

    hr = IDirectInputDevice8_GetDeviceInfo( device, &devinst );
    ok( hr == DI_OK, "GetDeviceInfo returned %#lx\n", hr );
    check_member( devinst, expect_devinst, "%lu", dwSize );
    check_member_guid( devinst, expect_devinst, guidProduct );
    todo_wine check_member( devinst, expect_devinst, "%#lx", dwDevType );
    check_member_wstr( devinst, expect_devinst, tszInstanceName );
    check_member_wstr( devinst, expect_devinst, tszProductName );
    check_member_guid( devinst, expect_devinst, guidFFDriver );
    check_member( devinst, expect_devinst, "%04x", wUsagePage );
    check_member( devinst, expect_devinst, "%04x", wUsage );

    caps.dwSize = sizeof(DIDEVCAPS);
    hr = IDirectInputDevice8_GetCapabilities( device, &caps );
    ok( hr == DI_OK, "GetCapabilities returned %#lx\n", hr );
    check_member( caps, expect_caps, "%lu", dwSize );
    check_member( caps, expect_caps, "%#lx", dwFlags );
    todo_wine check_member( caps, expect_caps, "%#lx", dwDevType );
    check_member( caps, expect_caps, "%lu", dwAxes );
    check_member( caps, expect_caps, "%lu", dwButtons );
    check_member( caps, expect_caps, "%lu", dwPOVs );
    check_member( caps, expect_caps, "%lu", dwFFSamplePeriod );
    check_member( caps, expect_caps, "%lu", dwFFMinTimeResolution );
    check_member( caps, expect_caps, "%lu", dwFirmwareRevision );
    check_member( caps, expect_caps, "%lu", dwHardwareRevision );
    check_member( caps, expect_caps, "%lu", dwFFDriverVersion );

    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFGAIN, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFGAIN returned %#lx\n", hr );
    ok( prop_dword.dwData == 10000, "got %lu expected %u\n", prop_dword.dwData, 10000 );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );

    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned: %#lx\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );

    file = CreateFileW( prop_guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    hwnd = create_foreground_window( FALSE );

    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#lx\n", hr );
    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 );

    test_condition_effect_six_axes( device, file );

    set_hid_expect( file, expect_reset, sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    DestroyWindow( hwnd );
    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );
    cleanup_registry_keys();
    winetest_pop_context();
    return device != NULL;
}

static void test_device_managed_effect(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_descriptor[] = {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 6),
                INPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    USAGE(1, PID_USAGE_DC_DEVICE_PAUSE),
                    USAGE(1, PID_USAGE_DC_DEVICE_CONTINUE),
                    USAGE(1, PID_USAGE_DC_ENABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_DISABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_STOP_ALL_EFFECTS),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 6),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 6),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Z),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 3),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 4),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(2, 0x00ff),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 0x00008ca0),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 4),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_PARAMETER_BLOCK_OFFSET),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_CP_OFFSET),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(1, 0x80),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(2, 0xd8f0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_FREE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_POOL_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_RAM_POOL_SIZE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_SIMULTANEOUS_EFFECTS_MAX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEVICE_MANAGED_POOL),
                USAGE(1, PID_USAGE_SHARED_PARAMETER_BLOCKS),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_CREATE_NEW_EFFECT_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_LOAD_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_BLOCK_LOAD_STATUS),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_SUCCESS),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_FULL),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_ERROR),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_RAM_POOL_AVAILABLE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_descriptor) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps =
        {
            .InputReportByteLength = 5,
            .OutputReportByteLength = 11,
            .FeatureReportByteLength = 5,
        },
        .attributes = default_attributes,
    };
    struct hid_expect expect_acquire[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        /* device gain */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 2,
            .report_buf = {6, 0xff},
        },
    };
    struct hid_expect expect_reset[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
    };
    struct hid_expect expect_enable_actuators[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x04},
        },
    };
    struct hid_expect expect_disable_actuators[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x05},
        },
    };
    struct hid_expect expect_stop_all[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x06},
        },
    };
    struct hid_expect expect_device_pause[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x02},
        },
    };
    struct hid_expect expect_device_continue[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x03},
        },
    };
    struct hid_expect expect_create[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x01,0x00,0x06,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_2[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x02,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x02,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x02,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x02,0x03,0x08,0x01,0x00,0x06,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_delay[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x01,0x00,0xff,0x7f,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_create_duration[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 2,
            .report_buf = {2,0x03},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x00,0xf9,0x19,0xd9,0xff,0xff,0x99},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 9,
            .report_buf = {4,0x01,0x01,0x4c,0x3f,0xcc,0x4c,0x33,0x19},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 11,
            .report_buf = {3,0x01,0x03,0x08,0x00,0x00,0x00,0x00,0x01,0x55,0x00},
        },
    };
    struct hid_expect expect_start =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x01, 0x01, 0x01},
    };
    struct hid_expect expect_start_2 =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x02, 0x02, 0x01},
    };
    struct hid_expect expect_stop =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x01, 0x03, 0x00},
    };
    struct hid_expect expect_stop_2 =
    {
        /* effect control */
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2, 0x02, 0x03, 0x00},
    };
    struct hid_expect expect_destroy[] =
    {
        /* effect operation */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x01,0x03,0x00},
        },
        /* block free */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {5,0x01},
        },
    };
    struct hid_expect expect_destroy_2[] =
    {
        /* effect operation */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x02,0x03,0x00},
        },
        /* block free */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {5,0x02},
        },
    };
    struct hid_expect device_state_input[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0xff,0x00,0xff},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x12,0x34,0x56,0xff},
        },
    };
    struct hid_expect device_state_input_0[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0xff,0x00,0xff},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x56,0x12,0x34,0xff},
        },
    };
    struct hid_expect device_state_input_1[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x00,0x01,0x01},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x65,0x43,0x21,0x00},
        },
    };
    struct hid_expect device_state_input_2[] =
    {
        /* effect state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x03,0x00,0x01},
        },
        /* device state */
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x12,0x34,0x56,0xff},
        },
    };
    struct hid_expect expect_pool[] =
    {
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x10,0x00,0x04,0x03},
            .todo = TRUE,
        },
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0x10,0x00,0x04,0x03},
            .todo = TRUE,
        },
    };
    static const DWORD expect_axes[3] =
    {
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 2 ) | DIDFT_FFACTUATOR,
        DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE( 1 ) | DIDFT_FFACTUATOR,
    };
    static const LONG expect_directions[3] = {
        +3000,
        0,
        0,
    };
    static const DIENVELOPE expect_envelope =
    {
        .dwSize = sizeof(DIENVELOPE),
        .dwAttackLevel = 1000,
        .dwAttackTime = 2000,
        .dwFadeLevel = 3000,
        .dwFadeTime = 4000,
    };
    static const DICONDITION expect_condition[3] =
    {
        {
            .lOffset = -500,
            .lPositiveCoefficient = 2000,
            .lNegativeCoefficient = -3000,
            .dwPositiveSaturation = -4000,
            .dwNegativeSaturation = -5000,
            .lDeadBand = 6000,
        },
        {
            .lOffset = 6000,
            .lPositiveCoefficient = 5000,
            .lNegativeCoefficient = -4000,
            .dwPositiveSaturation = 3000,
            .dwNegativeSaturation = 2000,
            .lDeadBand = 1000,
        },
        {
            .lOffset = -7000,
            .lPositiveCoefficient = -8000,
            .lNegativeCoefficient = 9000,
            .dwPositiveSaturation = 10000,
            .dwNegativeSaturation = 11000,
            .lDeadBand = -12000,
        },
    };
    const DIEFFECT expect_desc =
    {
        .dwSize = sizeof(DIEFFECT_DX6),
        .dwFlags = DIEFF_SPHERICAL | DIEFF_OBJECTIDS,
        .dwDuration = 1000,
        .dwSamplePeriod = 2000,
        .dwGain = 3000,
        .dwTriggerButton = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( 0 ) | DIDFT_FFEFFECTTRIGGER,
        .dwTriggerRepeatInterval = 5000,
        .cAxes = 2,
        .rgdwAxes = (void *)expect_axes,
        .rglDirection = (void *)expect_directions,
        .lpEnvelope = (void *)&expect_envelope,
        .cbTypeSpecificParams = 2 * sizeof(DICONDITION),
        .lpvTypeSpecificParams = (void *)expect_condition,
        .dwStartDelay = 6000,
    };
    DIPROPGUIDANDPATH prop_guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIPROPDWORD prop_dword =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPDWORD),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    IDirectInputDevice8W *device;
    IDirectInputEffect *effect, *effect2;
    DIEFFECT effect_desc;
    HANDLE file, event;
    ULONG res, ref;
    DWORD flags;
    HRESULT hr;
    HWND hwnd;

    cleanup_registry_keys();

    desc.report_descriptor_len = sizeof(report_descriptor);
    memcpy( desc.report_descriptor_buf, report_descriptor, sizeof(report_descriptor) );
    desc.expect_size = sizeof(expect_pool);
    memcpy( desc.expect, expect_pool, sizeof(expect_pool) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;
    if (FAILED(hr = dinput_test_create_device( DIRECTINPUT_VERSION, &devinst, &device ))) goto done;

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &prop_guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );
    file = CreateFileW( prop_guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    hwnd = create_foreground_window( FALSE );

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( event != NULL, "CreateEventW failed, last error %lu\n", GetLastError() );
    hr = IDirectInputDevice8_SetEventNotification( device, event );
    ok( hr == DI_OK, "SetEventNotification returned: %#lx\n", hr );
    hr = IDirectInputDevice8_SetCooperativeLevel( device, hwnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );
    ok( hr == DI_OK, "SetCooperativeLevel returned: %#lx\n", hr );
    hr = IDirectInputDevice8_SetDataFormat( device, &c_dfDIJoystick2 );
    ok( hr == DI_OK, "SetDataFormat returned: %#lx\n", hr );

    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetForceFeedbackState returned %#lx\n", hr );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_RESET );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "SendForceFeedbackCommand returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#lx\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED | DIGFFS_EMPTY;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#lx\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input, sizeof(struct hid_expect) );
    res = WaitForSingleObject( event, 100 );
    ok( res == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx\n", res );
    send_hid_input( file, device_state_input, sizeof(device_state_input) );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_EMPTY | DIGFFS_ACTUATORSON | DIGFFS_POWERON |
            DIGFFS_SAFETYSWITCHON | DIGFFS_USERFFSWITCHON;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, NULL, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );

    hr = IDirectInputEffect_GetEffectStatus( effect, NULL );
    ok( hr == E_POINTER, "GetEffectStatus returned %#lx\n", hr );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );

    flags = DIEP_ALLPARAMS;
    hr = IDirectInputEffect_SetParameters( effect, &expect_desc, flags | DIEP_NODOWNLOAD );
    ok( hr == DI_DOWNLOADSKIPPED, "SetParameters returned %#lx\n", hr );

    set_hid_expect( file, expect_reset, sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTEXCLUSIVEACQUIRED, "GetEffectStatus returned %#lx\n", hr );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IDirectInputDevice8_Acquire( device );
    ok( hr == DI_OK, "Acquire returned: %#lx\n", hr );
    wait_hid_expect( file, 100 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED | DIGFFS_EMPTY;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_create, sizeof(expect_create) );
    hr = IDirectInputEffect_Download( effect );
    ok( hr == DI_OK, "Download returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    prop_dword.dwData = 0xdeadbeef;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_FFLOAD, &prop_dword.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_FFLOAD returned %#lx\n", hr );
    ok( prop_dword.dwData == 0, "got DIPROP_FFLOAD %#lx\n", prop_dword.dwData );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, DIES_NODOWNLOAD );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_create_2, sizeof(expect_create_2) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &expect_desc, &effect2, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    set_hid_expect( file, &expect_start_2, sizeof(expect_start_2) );
    hr = IDirectInputEffect_Start( effect2, 1, DIES_SOLO );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect2, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, &expect_stop_2, sizeof(expect_stop_2) );
    hr = IDirectInputEffect_Stop( effect2 );
    ok( hr == DI_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect2, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_destroy_2, sizeof(expect_destroy_2) );
    ref = IDirectInputEffect_Release( effect2 );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    /* sending commands has no direct effect on status */
    set_hid_expect( file, expect_stop_all, sizeof(expect_stop_all) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_STOPALL );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_device_pause, sizeof(expect_device_pause) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_PAUSE );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_device_continue, sizeof(expect_device_continue) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_CONTINUE );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_disable_actuators, sizeof(expect_disable_actuators) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSOFF );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_enable_actuators, sizeof(expect_enable_actuators) );
    hr = IDirectInputDevice8_SendForceFeedbackCommand( device, DISFFC_SETACTUATORSON );
    ok( hr == DI_OK, "SendForceFeedbackCommand returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_STOPPED;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_0, sizeof(device_state_input_0) );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWERON | DIGFFS_SAFETYSWITCHON | DIGFFS_USERFFSWITCHON;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_1, sizeof(device_state_input_1) );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_ACTUATORSOFF | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    send_hid_input( file, device_state_input_2, sizeof(device_state_input_2) );
    res = WaitForSingleObject( event, 5000 );
    ok( res == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx\n", res );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IDirectInputEffect_Stop( effect );
    ok( hr == DI_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF | DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    hr = IDirectInputEffect_Unload( effect );
    ok( hr == DI_OK, "Unload returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DIERR_NOTDOWNLOADED, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, expect_pool, sizeof(struct hid_expect) );
    res = 0xdeadbeef;
    hr = IDirectInputDevice8_GetForceFeedbackState( device, &res );
    ok( hr == DI_OK, "GetForceFeedbackState returned %#lx\n", hr );
    flags = DIGFFS_EMPTY | DIGFFS_PAUSED | DIGFFS_ACTUATORSON | DIGFFS_POWEROFF |
            DIGFFS_SAFETYSWITCHOFF | DIGFFS_USERFFSWITCHOFF;
    ok( res == flags, "got state %#lx\n", res );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );

    /* start delay has no direct effect on effect status */
    effect_desc = expect_desc;
    effect_desc.dwStartDelay = 32767000;
    set_hid_expect( file, expect_create_delay, sizeof(expect_create_delay) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &effect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    /* duration has no direct effect on effect status */
    effect_desc = expect_desc;
    effect_desc.dwDuration = 100;
    effect_desc.dwStartDelay = 0;
    set_hid_expect( file, expect_create_duration, sizeof(expect_create_duration) );
    hr = IDirectInputDevice8_CreateEffect( device, &GUID_Spring, &effect_desc, &effect, NULL );
    ok( hr == DI_OK, "CreateEffect returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == 0, "got status %#lx\n", res );
    set_hid_expect( file, &expect_start, sizeof(expect_start) );
    hr = IDirectInputEffect_Start( effect, 1, 0 );
    ok( hr == DI_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );
    Sleep( 100 );
    res = 0xdeadbeef;
    hr = IDirectInputEffect_GetEffectStatus( effect, &res );
    ok( hr == DI_OK, "GetEffectStatus returned %#lx\n", hr );
    ok( res == DIEGES_PLAYING, "got status %#lx\n", res );
    set_hid_expect( file, expect_destroy, sizeof(expect_destroy) );
    ref = IDirectInputEffect_Release( effect );
    ok( ref == 0, "Release returned %ld\n", ref );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_reset, sizeof(struct hid_expect) );
    hr = IDirectInputDevice8_Unacquire( device );
    ok( hr == DI_OK, "Unacquire returned: %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );

    ref = IDirectInputDevice8_Release( device );
    ok( ref == 0, "Release returned %ld\n", ref );

    DestroyWindow( hwnd );
    CloseHandle( event );
    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );
    cleanup_registry_keys();
    winetest_pop_context();
}

#define check_interface( a, b, c ) check_interface_( __LINE__, a, b, c )
static void check_interface_( unsigned int line, void *iface_ptr, REFIID iid, BOOL supported )
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected;
    IUnknown *unk;

    expected = supported ? S_OK : E_NOINTERFACE;
    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == expected, "got hr %#lx, expected %#lx.\n", hr, expected );
    if (SUCCEEDED(hr)) IUnknown_Release( unk );
}

#define check_runtimeclass( a, b ) check_runtimeclass_( __LINE__, (IInspectable *)a, b )
static void check_runtimeclass_( int line, IInspectable *inspectable, const WCHAR *class_name )
{
    const WCHAR *buffer;
    UINT32 length;
    HSTRING str;
    HRESULT hr;

    hr = IInspectable_GetRuntimeClassName( inspectable, &str );
    ok_(__FILE__, line)( hr == S_OK, "GetRuntimeClassName returned %#lx\n", hr );
    buffer = pWindowsGetStringRawBuffer( str, &length );
    ok_(__FILE__, line)( !wcscmp( buffer, class_name ), "got class name %s\n", debugstr_w(buffer) );
    pWindowsDeleteString( str );
}

struct controller_handler
{
    IEventHandler_RawGameController IEventHandler_RawGameController_iface;
    HANDLE event;
    BOOL invoked;
};

static inline struct controller_handler *impl_from_IEventHandler_RawGameController( IEventHandler_RawGameController *iface )
{
    return CONTAINING_RECORD( iface, struct controller_handler, IEventHandler_RawGameController_iface );
}

static HRESULT WINAPI controller_handler_QueryInterface( IEventHandler_RawGameController *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IEventHandler_RawGameController ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI controller_handler_AddRef( IEventHandler_RawGameController *iface )
{
    return 2;
}

static ULONG WINAPI controller_handler_Release( IEventHandler_RawGameController *iface )
{
    return 1;
}

static HRESULT WINAPI controller_handler_Invoke( IEventHandler_RawGameController *iface,
                                                 IInspectable *sender, IRawGameController *controller )
{
    struct controller_handler *impl = impl_from_IEventHandler_RawGameController( iface );

    if (winetest_debug > 1) trace( "iface %p, sender %p, controller %p\n", iface, sender, controller );

    ok( sender == NULL, "got sender %p\n", sender );
    impl->invoked = TRUE;
    SetEvent( impl->event );

    return S_OK;
}

static const IEventHandler_RawGameControllerVtbl controller_handler_vtbl =
{
    controller_handler_QueryInterface,
    controller_handler_AddRef,
    controller_handler_Release,
    controller_handler_Invoke,
};

static struct controller_handler controller_added = {{&controller_handler_vtbl}};

#define check_bool_async( a, b, c, d, e ) check_bool_async_( __LINE__, a, b, c, d, e )
static void check_bool_async_( int line, IAsyncOperation_boolean *async, UINT32 expect_id, AsyncStatus expect_status,
                               HRESULT expect_hr, BOOLEAN expect_result )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;
    BOOLEAN result;

    hr = IAsyncOperation_boolean_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( async_id == expect_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) todo_wine_if(FAILED(expect_hr)) ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );

    result = !expect_result;
    hr = IAsyncOperation_boolean_GetResults( async, &result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr, "GetResults returned %#lx\n", hr );
        ok_(__FILE__, line)( result == expect_result, "got result %u\n", result );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

#define check_result_async( a, b, c, d, e ) check_result_async_( __LINE__, a, b, c, d, e )
static void check_result_async_( int line, IAsyncOperation_ForceFeedbackLoadEffectResult *async, UINT32 expect_id,
                                 AsyncStatus expect_status, HRESULT expect_hr, ForceFeedbackLoadEffectResult expect_result )
{
    ForceFeedbackLoadEffectResult result;
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IAsyncOperation_ForceFeedbackLoadEffectResult_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( async_id == expect_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) todo_wine_if(FAILED(expect_hr)) ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );

    result = !expect_result;
    hr = IAsyncOperation_ForceFeedbackLoadEffectResult_GetResults( async, &result );
    switch (expect_status)
    {
    case Completed:
    case Error:
        todo_wine_if(FAILED(expect_hr))
        ok_(__FILE__, line)( hr == expect_hr, "GetResults returned %#lx\n", hr );
        ok_(__FILE__, line)( result == expect_result, "got result %u\n", result );
        break;
    case Canceled:
    case Started:
    default:
        ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "GetResults returned %#lx\n", hr );
        break;
    }
}

struct bool_async_handler
{
    IAsyncOperationCompletedHandler_boolean IAsyncOperationCompletedHandler_boolean_iface;
    LONG refcount;

    IAsyncOperation_boolean *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct bool_async_handler *impl_from_IAsyncOperationCompletedHandler_boolean( IAsyncOperationCompletedHandler_boolean *iface )
{
    return CONTAINING_RECORD( iface, struct bool_async_handler, IAsyncOperationCompletedHandler_boolean_iface );
}

static HRESULT WINAPI bool_async_handler_QueryInterface( IAsyncOperationCompletedHandler_boolean *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_boolean ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI bool_async_handler_AddRef( IAsyncOperationCompletedHandler_boolean *iface )
{
    struct bool_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_boolean( iface );
    return InterlockedIncrement( &impl->refcount );
}

static ULONG WINAPI bool_async_handler_Release( IAsyncOperationCompletedHandler_boolean *iface )
{
    struct bool_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_boolean( iface );
    ULONG ref = InterlockedDecrement( &impl->refcount );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI bool_async_handler_Invoke( IAsyncOperationCompletedHandler_boolean *iface,
                                                 IAsyncOperation_boolean *async, AsyncStatus status )
{
    struct bool_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_boolean( iface );

    if (winetest_debug > 1) trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_booleanVtbl bool_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    bool_async_handler_QueryInterface,
    bool_async_handler_AddRef,
    bool_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<boolean> methods ***/
    bool_async_handler_Invoke,
};

static IAsyncOperationCompletedHandler_boolean *bool_async_handler_create( HANDLE event )
{
    struct bool_async_handler *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;
    impl->IAsyncOperationCompletedHandler_boolean_iface.lpVtbl = &bool_async_handler_vtbl;
    impl->event = event;
    impl->refcount = 1;

    return &impl->IAsyncOperationCompletedHandler_boolean_iface;
}

#define await_bool( a ) await_bool_( __LINE__, a )
static void await_bool_( int line, IAsyncOperation_boolean *async )
{
    IAsyncOperationCompletedHandler_boolean *handler;
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(__FILE__, line)( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    handler = bool_async_handler_create( event );
    ok_(__FILE__, line)( !!handler, "bool_async_handler_create failed\n" );
    hr = IAsyncOperation_boolean_put_Completed( async, handler );
    ok_(__FILE__, line)( hr == S_OK, "put_Completed returned %#lx\n", hr );
    IAsyncOperationCompletedHandler_boolean_Release( handler );

    ret = WaitForSingleObject( event, 5000 );
    ok_(__FILE__, line)( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( event );
    ok_(__FILE__, line)( ret, "CloseHandle failed, error %lu\n", GetLastError() );
}

struct result_async_handler
{
    IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult_iface;
    LONG refcount;

    IAsyncOperation_ForceFeedbackLoadEffectResult *async;
    AsyncStatus status;
    BOOL invoked;
    HANDLE event;
};

static inline struct result_async_handler *impl_from_IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult( IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *iface )
{
    return CONTAINING_RECORD( iface, struct result_async_handler, IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult_iface );
}

static HRESULT WINAPI result_async_handler_QueryInterface( IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult ))
    {
        IUnknown_AddRef( iface );
        *out = iface;
        return S_OK;
    }

    if (winetest_debug > 1) trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI result_async_handler_AddRef( IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *iface )
{
    struct result_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult( iface );
    return InterlockedIncrement( &impl->refcount );
}

static ULONG WINAPI result_async_handler_Release( IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *iface )
{
    struct result_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult( iface );
    ULONG ref = InterlockedDecrement( &impl->refcount );
    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI result_async_handler_Invoke( IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *iface,
                                                   IAsyncOperation_ForceFeedbackLoadEffectResult *async, AsyncStatus status )
{
    struct result_async_handler *impl = impl_from_IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult( iface );

    if (winetest_debug > 1) trace( "iface %p, async %p, status %u\n", iface, async, status );

    ok( !impl->invoked, "invoked twice\n" );
    impl->invoked = TRUE;
    impl->async = async;
    impl->status = status;
    if (impl->event) SetEvent( impl->event );

    return S_OK;
}

static IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResultVtbl result_async_handler_vtbl =
{
    /*** IUnknown methods ***/
    result_async_handler_QueryInterface,
    result_async_handler_AddRef,
    result_async_handler_Release,
    /*** IAsyncOperationCompletedHandler<ForceFeedbackLoadEffectResult> methods ***/
    result_async_handler_Invoke,
};

static IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *result_async_handler_create( HANDLE event )
{
    struct result_async_handler *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;
    impl->IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult_iface.lpVtbl = &result_async_handler_vtbl;
    impl->event = event;
    impl->refcount = 1;

    return &impl->IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult_iface;
}

#define await_result( a ) await_result_( __LINE__, a )
static void await_result_( int line, IAsyncOperation_ForceFeedbackLoadEffectResult *async )
{
    IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult *handler;
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(__FILE__, line)( !!event, "CreateEventW failed, error %lu\n", GetLastError() );

    handler = result_async_handler_create( event );
    ok_(__FILE__, line)( !!handler, "result_async_handler_create failed\n" );
    hr = IAsyncOperation_ForceFeedbackLoadEffectResult_put_Completed( async, handler );
    ok_(__FILE__, line)( hr == S_OK, "put_Completed returned %#lx\n", hr );
    IAsyncOperationCompletedHandler_ForceFeedbackLoadEffectResult_Release( handler );

    ret = WaitForSingleObject( event, 5000 );
    ok_(__FILE__, line)( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = CloseHandle( event );
    ok_(__FILE__, line)( ret, "CloseHandle failed, error %lu\n", GetLastError() );
}

static void test_windows_gaming_input(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Physical),
                REPORT_ID(1, 1),

                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 127),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 127),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 3),
                INPUT(1, Data|Var|Abs),

                USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 8),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 8),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs|Null),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 5),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_STATE_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_DEVICE_PAUSED),
                USAGE(1, PID_USAGE_ACTUATORS_ENABLED),
                USAGE(1, PID_USAGE_SAFETY_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_OVERRIDE_SWITCH),
                USAGE(1, PID_USAGE_ACTUATOR_POWER),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 5),
                INPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 3),
                INPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_PLAYING),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_PID),
            USAGE(1, PID_USAGE_DEVICE_CONTROL_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_DEVICE_CONTROL),
                COLLECTION(1, Logical),
                    USAGE(1, PID_USAGE_DC_DEVICE_RESET),
                    USAGE(1, PID_USAGE_DC_DEVICE_PAUSE),
                    USAGE(1, PID_USAGE_DC_DEVICE_CONTINUE),
                    USAGE(1, PID_USAGE_DC_ENABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_DISABLE_ACTUATORS),
                    USAGE(1, PID_USAGE_DC_STOP_ALL_EFFECTS),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 6),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 6),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_EFFECT_OPERATION_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_OPERATION),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_OP_EFFECT_START),
                    USAGE(1, PID_USAGE_OP_EFFECT_START_SOLO),
                    USAGE(1, PID_USAGE_OP_EFFECT_STOP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_LOOP_COUNT),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_EFFECT_REPORT),
            COLLECTION(1, Report),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    USAGE(1, PID_USAGE_ET_CONSTANT_FORCE),
                    USAGE(1, PID_USAGE_ET_RAMP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 5),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 5),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    OUTPUT(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_AXES_ENABLE),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_X),
                    USAGE(4, (HID_USAGE_PAGE_GENERIC << 16)|HID_USAGE_GENERIC_Y),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 1),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,
                USAGE(1, PID_USAGE_DIRECTION_ENABLE),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
                REPORT_COUNT(1, 5),
                OUTPUT(1, Cnst|Var|Abs),

                USAGE(1, PID_USAGE_DURATION),
                USAGE(1, PID_USAGE_TRIGGER_REPEAT_INTERVAL),
                USAGE(1, PID_USAGE_SAMPLE_PERIOD),
                USAGE(1, PID_USAGE_START_DELAY),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 4),
                OUTPUT(1, Data|Var|Abs),
                UNIT(1, 0),
                UNIT_EXPONENT(1, 0),

                USAGE(1, PID_USAGE_TRIGGER_BUTTON),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x08),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x08),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DIRECTION),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    UNIT(1, 0x14),        /* Eng Rot:Angular Pos */
                    UNIT_EXPONENT(1, -2), /* 10^-2 */
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(4, 360),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(4, 36000),
                    UNIT(1, 0),
                    REPORT_SIZE(1, 16),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                    UNIT_EXPONENT(1, 0),
                    UNIT(1, 0),
                END_COLLECTION,
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONDITION_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 4),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_PARAMETER_BLOCK_OFFSET),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_TYPE_SPECIFIC_BLOCK_OFFSET),
                COLLECTION(1, Logical),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|1),
                    USAGE(4, (HID_USAGE_PAGE_ORDINAL << 16)|2),
                    LOGICAL_MINIMUM(1, 0),
                    LOGICAL_MAXIMUM(1, 1),
                    PHYSICAL_MINIMUM(1, 0),
                    PHYSICAL_MAXIMUM(1, 1),
                    REPORT_SIZE(1, 2),
                    REPORT_COUNT(1, 2),
                    OUTPUT(1, Data|Var|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_CP_OFFSET),
                LOGICAL_MINIMUM(2, -10000),
                LOGICAL_MAXIMUM(2, +10000),
                PHYSICAL_MINIMUM(2, -10000),
                PHYSICAL_MAXIMUM(2, +10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_COEFFICIENT),
                USAGE(1, PID_USAGE_NEGATIVE_COEFFICIENT),
                LOGICAL_MINIMUM(2, -10000),
                LOGICAL_MAXIMUM(2, +10000),
                PHYSICAL_MINIMUM(2, -10000),
                PHYSICAL_MAXIMUM(2, +10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_POSITIVE_SATURATION),
                USAGE(1, PID_USAGE_NEGATIVE_SATURATION),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 10000),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEAD_BAND),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 10000),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_FREE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 5),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_DEVICE_GAIN_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 6),

                USAGE(1, PID_USAGE_DEVICE_GAIN),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_PERIODIC_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 7),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 10000),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_OFFSET),
                LOGICAL_MINIMUM(2, -10000),
                LOGICAL_MAXIMUM(2, +10000),
                PHYSICAL_MINIMUM(2, -10000),
                PHYSICAL_MAXIMUM(2, +10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_PHASE),
                UNIT(1, 0x14), /* Eng Rot:Angular Pos */
                UNIT_EXPONENT(1, -2),
                LOGICAL_MINIMUM(2, -180),
                LOGICAL_MAXIMUM(2, +180),
                PHYSICAL_MINIMUM(2, -18000),
                PHYSICAL_MAXIMUM(2, +18000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_PERIOD),
                UNIT(2, 0x1003), /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                UNIT_EXPONENT(1, 0),
                UNIT(1, 0), /* None */
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_ENVELOPE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 8),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_ATTACK_LEVEL),
                USAGE(1, PID_USAGE_FADE_LEVEL),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x2710),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_ATTACK_TIME),
                USAGE(1, PID_USAGE_FADE_TIME),
                UNIT(2, 0x1003),      /* Eng Lin:Time */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(2, 0x7fff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
                UNIT_EXPONENT(1, 0),
                UNIT(1, 0),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_CONSTANT_FORCE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 9),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_MAGNITUDE),
                LOGICAL_MINIMUM(2, -10000),
                LOGICAL_MAXIMUM(2, +10000),
                PHYSICAL_MINIMUM(2, -10000),
                PHYSICAL_MAXIMUM(2, +10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_SET_RAMP_FORCE_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 10),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                OUTPUT(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_RAMP_START),
                USAGE(1, PID_USAGE_RAMP_END),
                LOGICAL_MINIMUM(2, -10000),
                LOGICAL_MAXIMUM(2, +10000),
                PHYSICAL_MINIMUM(2, -10000),
                PHYSICAL_MAXIMUM(2, +10000),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                OUTPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_POOL_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 1),

                USAGE(1, PID_USAGE_RAM_POOL_SIZE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_SIMULTANEOUS_EFFECTS_MAX),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_DEVICE_MANAGED_POOL),
                USAGE(1, PID_USAGE_SHARED_PARAMETER_BLOCKS),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_CREATE_NEW_EFFECT_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 2),

                USAGE(1, PID_USAGE_EFFECT_TYPE),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_ET_SQUARE),
                    USAGE(1, PID_USAGE_ET_SINE),
                    USAGE(1, PID_USAGE_ET_SPRING),
                    USAGE(1, PID_USAGE_ET_CONSTANT_FORCE),
                    USAGE(1, PID_USAGE_ET_RAMP),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 5),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 5),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(4, (HID_USAGE_PAGE_GENERIC<<16)|HID_USAGE_GENERIC_BYTE_COUNT),
                LOGICAL_MINIMUM(1, 0x7f),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 0x7f),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Ary|Abs),
            END_COLLECTION,

            USAGE(1, PID_USAGE_BLOCK_LOAD_REPORT),
            COLLECTION(1, Logical),
                REPORT_ID(1, 3),

                USAGE(1, PID_USAGE_EFFECT_BLOCK_INDEX),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 0x7f),
                PHYSICAL_MINIMUM(1, 1),
                PHYSICAL_MAXIMUM(1, 0x7f),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),

                USAGE(1, PID_USAGE_BLOCK_LOAD_STATUS),
                COLLECTION(1, NamedArray),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_SUCCESS),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_FULL),
                    USAGE(1, PID_USAGE_BLOCK_LOAD_ERROR),
                    LOGICAL_MINIMUM(1, 1),
                    LOGICAL_MAXIMUM(1, 3),
                    PHYSICAL_MINIMUM(1, 1),
                    PHYSICAL_MAXIMUM(1, 3),
                    REPORT_SIZE(1, 8),
                    REPORT_COUNT(1, 1),
                    FEATURE(1, Data|Ary|Abs),
                END_COLLECTION,

                USAGE(1, PID_USAGE_RAM_POOL_AVAILABLE),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(4, 0xffff),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(4, 0xffff),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps =
        {
            .InputReportByteLength = 6,
            .OutputReportByteLength = 18,
            .FeatureReportByteLength = 5,
        },
        .attributes = default_attributes,
    };
    struct hid_expect expect_init[] =
    {
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0xff,0x7f,0x7f,0x03},
            .todo = TRUE,
        },
    };
    struct hid_expect expect_acquire[] =
    {
        /* device pool */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 1,
            .report_len = 5,
            .report_buf = {1,0xff,0x7f,0x7f,0x03},
            .todo = TRUE,
        },
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x06},
            .todo = TRUE,
        },
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x05},
            .todo = TRUE,
        },
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        /* device gain */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 2,
            .report_buf = {6, 0xff},
        },
    };
    static struct hid_expect expect_set_gain =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 6,
        .report_len = 2,
        .report_buf = {6, 0x7f},
    };
    static struct hid_expect expect_pause =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x02},
    };
    static struct hid_expect expect_resume =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x03},
    };
    static struct hid_expect expect_stop =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x06},
    };
    static struct hid_expect expect_disable =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x05},
    };
    static struct hid_expect expect_enable =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x04},
    };
    static struct hid_expect expect_enable_fail =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .ret_status = STATUS_NOT_SUPPORTED,
        .report_id = 1,
        .report_len = 2,
        .report_buf = {1, 0x04},
    };
    struct hid_expect expect_reset[] =
    {
        /* device control */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 1,
            .report_len = 2,
            .report_buf = {1, 0x01},
        },
        /* device gain */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 6,
            .report_len = 2,
            .report_buf = {6, 0x7f},
        },
    };
    struct hid_expect expect_create_periodic[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x02,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 10,
            .report_buf = {7,0x01,0xa0,0x0f,0xd0,0x07,0x70,0xff,0x0a,0x00},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 8,
            .report_buf = {8,0x01,0x4c,0x7f,0x28,0x00,0x50,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x02,0x04,0x78,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0xff,0xff,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_create_periodic_neg[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x02,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set periodic */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 7,
            .report_len = 10,
            .report_buf = {7,0x01,0x10,0x27,0x00,0x00,0x70,0xff,0xe8,0x03},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x02,0x04,0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x1a,0x00,0x00,0x00},
        },
    };
    struct hid_expect expect_create_condition[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x03,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 12,
            .report_buf = {4,0x01,0x00,0x70,0x17,0x7b,0x02,0xe9,0x04,0x4c,0x66,0x7f},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x03,0x04,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x99,0x00,0x00,0x00},
        },
    };
    struct hid_expect expect_create_condition_neg[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x03,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set condition */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 4,
            .report_len = 12,
            .report_buf = {4,0x01,0x00,0x70,0x17,0x7b,0x02,0xe9,0x04,0x4c,0x66,0x7f},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x03,0x04,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xcf,0x00,0x00,0x00},
        },
    };
    struct hid_expect expect_create_constant[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x04,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set constant */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 9,
            .report_len = 4,
            .report_buf = {9,0x01,0xc8,0x00},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 8,
            .report_buf = {8,0x01,0x19,0x4c,0x14,0x00,0x3c,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x04,0x04,0x5a,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0xff,0x7f,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_create_constant_neg[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x04,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set constant */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 9,
            .report_len = 4,
            .report_buf = {9,0x01,0x18,0xfc},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x04,0x04,0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x7f,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_create_ramp[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x05,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set ramp */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 10,
            .report_len = 6,
            .report_buf = {10,0x01,0xc8,0x00,0x20,0x03},
        },
        /* set envelope */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 8,
            .report_len = 8,
            .report_buf = {8,0x01,0x19,0x4c,0x14,0x00,0x3c,0x00},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x05,0x04,0x5a,0x00,0x00,0x00,0x00,0x00,0x0a,0x00,0xff,0xff,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_create_ramp_inf[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x05,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set ramp */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 10,
            .report_len = 6,
            .report_buf = {10,0x01,0xe8,0x03,0xa0,0x0f},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x05,0x04,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_create_ramp_neg[] =
    {
        /* create new effect */
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = 2,
            .report_len = 3,
            .report_buf = {2,0x05,0x00},
        },
        /* block load */
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = 3,
            .report_len = 5,
            .report_buf = {3,0x01,0x01,0x00,0x00},
        },
        /* set ramp */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 10,
            .report_len = 6,
            .report_buf = {10,0x01,0x18,0xfc,0x60,0xf0},
        },
        /* update effect */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 3,
            .report_len = 18,
            .report_buf = {3,0x01,0x05,0x04,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x4e,0x01,0x00,0x00},
        },
    };
    struct hid_expect expect_effect_start =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2,0x01,0x01,0x01},
    };
    struct hid_expect expect_effect_stop =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = 2,
        .report_len = 4,
        .report_buf = {2,0x01,0x03,0x00},
    };
    struct hid_expect expect_unload[] =
    {
        /* effect stop */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 2,
            .report_len = 4,
            .report_buf = {2,0x01,0x03,0x00},
        },
        /* effect free */
        {
            .code = IOCTL_HID_WRITE_REPORT,
            .report_id = 5,
            .report_len = 2,
            .report_buf = {5,0x01},
        },
    };
    static const WCHAR *condition_effect_class_name = RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConditionForceEffect;
    static const WCHAR *periodic_effect_class_name = RuntimeClass_Windows_Gaming_Input_ForceFeedback_PeriodicForceEffect;
    static const WCHAR *constant_effect_class_name = RuntimeClass_Windows_Gaming_Input_ForceFeedback_ConstantForceEffect;
    static const WCHAR *force_feedback_motor = RuntimeClass_Windows_Gaming_Input_ForceFeedback_ForceFeedbackMotor;
    static const WCHAR *ramp_effect_class_name = RuntimeClass_Windows_Gaming_Input_ForceFeedback_RampForceEffect;
    static const WCHAR *controller_class_name = RuntimeClass_Windows_Gaming_Input_RawGameController;

    DIPROPGUIDANDPATH guid_path =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPGUIDANDPATH),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    TimeSpan delay = {100000}, attack_duration = {200000}, release_duration = {300000}, duration = {400000}, infinite_duration = {INT64_MAX};
    Vector3 direction = {0.1, 0.2, 0.3}, end_direction = {0.4, 0.5, 0.6};
    DIDEVICEINSTANCEW devinst = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    IAsyncOperation_ForceFeedbackLoadEffectResult *result_async;
    IAsyncOperationCompletedHandler_boolean *tmp_handler;
    IVectorView_RawGameController *controllers_view;
    IConditionForceEffectFactory *condition_factory;
    IRawGameControllerStatics *controller_statics;
    EventRegistrationToken controller_added_token;
    IPeriodicForceEffectFactory *periodic_factory;
    struct bool_async_handler *bool_async_handler;
    ForceFeedbackEffectAxes supported_axes, axes;
    IVectorView_ForceFeedbackMotor *motors_view;
    IConditionForceEffect *condition_effect;
    ConditionForceEffectKind condition_kind;
    IActivationFactory *activation_factory;
    IPeriodicForceEffect *periodic_effect;
    IConstantForceEffect *constant_effect;
    PeriodicForceEffectKind periodic_kind;
    IAsyncOperation_boolean *bool_async;
    IRawGameController *raw_controller;
    ForceFeedbackEffectState state;
    IRampForceEffect *ramp_effect;
    IInspectable *tmp_inspectable;
    IForceFeedbackEffect *effect;
    IDirectInputDevice8W *device;
    IForceFeedbackMotor *motor;
    BOOLEAN paused, enabled;
    IAsyncInfo *async_info;
    DOUBLE gain;
    HSTRING str;
    HANDLE file;
    UINT32 size;
    HRESULT hr;
    DWORD ret;

    if (!load_combase_functions()) return;

    cleanup_registry_keys();

    hr = pRoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == RPC_E_CHANGED_MODE, "RoInitialize returned %#lx\n", hr );

    hr = pWindowsCreateString( controller_class_name, wcslen( controller_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IRawGameControllerStatics, (void **)&controller_statics );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );

    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( controller_class_name ) );
        goto done;
    }

    controller_added.event = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok( !!controller_added.event, "CreateEventW failed, error %lu\n", GetLastError() );

    hr = IRawGameControllerStatics_add_RawGameControllerAdded( controller_statics, &controller_added.IEventHandler_RawGameController_iface,
                                                               &controller_added_token );
    ok( hr == S_OK, "add_RawGameControllerAdded returned %#lx\n", hr );
    ok( controller_added_token.value, "got token %I64u\n", controller_added_token.value );

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    desc.expect_size = sizeof(expect_init);
    memcpy( desc.expect, expect_init, sizeof(expect_init) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;
    ret = WaitForSingleObject( controller_added.event, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( controller_added.event );

    if (FAILED(hr = dinput_test_create_device( 0x800, &devinst, &device ))) goto done;
    hr = IDirectInputDevice8_GetProperty( device, DIPROP_GUIDANDPATH, &guid_path.diph );
    ok( hr == DI_OK, "GetProperty DIPROP_GUIDANDPATH returned %#lx\n", hr );
    IDirectInputDevice8_Release( device );

    file = CreateFileW( guid_path.wszPath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    hr = IRawGameControllerStatics_remove_RawGameControllerAdded( controller_statics, controller_added_token );
    ok( hr == S_OK, "remove_RawGameControllerAdded returned %#lx\n", hr );

    hr = IRawGameControllerStatics_get_RawGameControllers( controller_statics, &controllers_view );
    ok( hr == S_OK, "get_RawGameControllers returned %#lx\n", hr );
    hr = IVectorView_RawGameController_get_Size( controllers_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 1, "got size %u\n", size );
    hr = IVectorView_RawGameController_GetAt( controllers_view, 0, &raw_controller );
    ok( hr == S_OK, "GetAt returned %#lx\n", hr );
    IVectorView_RawGameController_Release( controllers_view );

    set_hid_expect( file, expect_acquire, sizeof(expect_acquire) );
    hr = IRawGameController_get_ForceFeedbackMotors( raw_controller, &motors_view );
    ok( hr == S_OK, "get_ForceFeedbackMotors returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    hr = IVectorView_ForceFeedbackMotor_get_Size( motors_view, &size );
    ok( hr == S_OK, "get_Size returned %#lx\n", hr );
    ok( size == 1, "got size %u\n", size );
    hr = IVectorView_ForceFeedbackMotor_GetAt( motors_view, 0, &motor );
    ok( hr == S_OK, "GetAt returned %#lx\n", hr );
    IVectorView_ForceFeedbackMotor_Release( motors_view );

    check_interface( motor, &IID_IUnknown, TRUE );
    check_interface( motor, &IID_IInspectable, TRUE );
    check_interface( motor, &IID_IAgileObject, TRUE );
    check_interface( motor, &IID_IForceFeedbackMotor, TRUE );
    check_runtimeclass( motor, force_feedback_motor );

    paused = TRUE;
    hr = IForceFeedbackMotor_get_AreEffectsPaused( motor, &paused );
    ok( hr == S_OK, "get_AreEffectsPaused returned %#lx\n", hr );
    ok( paused == FALSE, "got paused %u\n", paused );

    gain = 12345.6;
    hr = IForceFeedbackMotor_get_MasterGain( motor, &gain );
    ok( hr == S_OK, "get_MasterGain returned %#lx\n", hr );
    ok( gain == 1.0, "got gain %f\n", gain );
    set_hid_expect( file, &expect_set_gain, sizeof(expect_set_gain) );
    hr = IForceFeedbackMotor_put_MasterGain( motor, 0.5 );
    ok( hr == S_OK, "put_MasterGain returned %#lx\n", hr );
    wait_hid_expect( file, 100 ); /* device gain reports are written asynchronously */

    enabled = FALSE;
    hr = IForceFeedbackMotor_get_IsEnabled( motor, &enabled );
    ok( hr == S_OK, "get_IsEnabled returned %#lx\n", hr );
    ok( enabled == TRUE, "got enabled %u\n", enabled );

    /* SupportedAxes always returns ForceFeedbackEffectAxes_X on Windows,
     * no matter which axis is available for FFB in the Set Effects report,
     * or whether a X axis is declared at all.
     */

    supported_axes = 0xdeadbeef;
    hr = IForceFeedbackMotor_get_SupportedAxes( motor, &supported_axes );
    ok( hr == S_OK, "get_SupportedAxes returned %#lx\n", hr );
    axes = ForceFeedbackEffectAxes_X | ForceFeedbackEffectAxes_Y;
    ok( supported_axes == axes || broken( supported_axes == ForceFeedbackEffectAxes_X ),
        "got axes %#x\n", supported_axes );

    set_hid_expect( file, &expect_pause, sizeof(expect_pause) );
    hr = IForceFeedbackMotor_PauseAllEffects( motor );
    ok( hr == S_OK, "PauseAllEffects returned %#lx\n", hr );
    set_hid_expect( file, &expect_resume, sizeof(expect_resume) );
    hr = IForceFeedbackMotor_ResumeAllEffects( motor );
    ok( hr == S_OK, "ResumeAllEffects returned %#lx\n", hr );
    set_hid_expect( file, &expect_stop, sizeof(expect_stop) );
    hr = IForceFeedbackMotor_StopAllEffects( motor );
    ok( hr == S_OK, "StopAllEffects returned %#lx\n", hr );
    set_hid_expect( file, NULL, 0 );


    set_hid_expect( file, &expect_disable, sizeof(expect_disable) );
    hr = IForceFeedbackMotor_TryDisableAsync( motor, &bool_async );
    ok( hr == S_OK, "TryDisableAsync returned %#lx\n", hr );
    wait_hid_expect( file, 100 );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );

    check_interface( bool_async, &IID_IUnknown, TRUE );
    check_interface( bool_async, &IID_IInspectable, TRUE );
    check_interface( bool_async, &IID_IAgileObject, TRUE );
    check_interface( bool_async, &IID_IAsyncInfo, TRUE );
    check_interface( bool_async, &IID_IAsyncOperation_boolean, TRUE );
    check_runtimeclass( bool_async, L"Windows.Foundation.IAsyncOperation`1<Boolean>" );

    hr = IAsyncOperation_boolean_get_Completed( bool_async, &tmp_handler );
    ok( hr == S_OK, "get_Completed returned %#lx\n", hr );
    ok( tmp_handler == NULL, "got handler %p\n", tmp_handler );

    bool_async_handler = impl_from_IAsyncOperationCompletedHandler_boolean( bool_async_handler_create( NULL ) );
    ok( !!bool_async_handler, "bool_async_handler_create failed\n" );
    hr = IAsyncOperation_boolean_put_Completed( bool_async, &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );
    ok( hr == S_OK, "put_Completed returned %#lx\n", hr );
    ok( bool_async_handler->invoked, "handler not invoked\n" );
    ok( bool_async_handler->async == bool_async, "got async %p\n", bool_async_handler->async );
    ok( bool_async_handler->status == Completed, "got status %u\n", bool_async_handler->status );
    hr = IAsyncOperation_boolean_get_Completed( bool_async, &tmp_handler );
    ok( hr == S_OK, "get_Completed returned %#lx\n", hr );
    ok( tmp_handler == NULL, "got handler %p\n", tmp_handler );
    IAsyncOperationCompletedHandler_boolean_Release( &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );

    bool_async_handler = impl_from_IAsyncOperationCompletedHandler_boolean( bool_async_handler_create( NULL ) );
    ok( !!bool_async_handler, "bool_async_handler_create failed\n" );
    hr = IAsyncOperation_boolean_put_Completed( bool_async, &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );
    ok( hr == E_ILLEGAL_DELEGATE_ASSIGNMENT, "put_Completed returned %#lx\n", hr );
    ok( !bool_async_handler->invoked, "handler invoked\n" );
    ok( bool_async_handler->async == NULL, "got async %p\n", bool_async_handler->async );
    ok( bool_async_handler->status == Started, "got status %u\n", bool_async_handler->status );
    IAsyncOperationCompletedHandler_boolean_Release( &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );

    hr = IAsyncOperation_boolean_QueryInterface( bool_async, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IAsyncInfo_Cancel( async_info );
    ok( hr == S_OK, "Cancel returned %#lx\n", hr );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "Close returned %#lx\n", hr );
    check_bool_async( bool_async, 1, 4, S_OK, FALSE );
    IAsyncInfo_Release( async_info );

    IAsyncOperation_boolean_Release( bool_async );


    set_hid_expect( file, &expect_enable_fail, sizeof(expect_enable_fail) );
    hr = IForceFeedbackMotor_TryEnableAsync( motor, &bool_async );
    ok( hr == S_OK, "TryEnableAsync returned %#lx\n", hr );
    wait_hid_expect( file, 100 );
    check_bool_async( bool_async, 1, Error, 0x8685400d, FALSE );

    bool_async_handler = impl_from_IAsyncOperationCompletedHandler_boolean( bool_async_handler_create( NULL ) );
    ok( !!bool_async_handler, "bool_async_handler_create failed\n" );
    hr = IAsyncOperation_boolean_put_Completed( bool_async, &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );
    ok( hr == S_OK, "put_Completed returned %#lx\n", hr );
    ok( bool_async_handler->invoked, "handler not invoked\n" );
    ok( bool_async_handler->async == bool_async, "got async %p\n", bool_async_handler->async );
    ok( bool_async_handler->status == Error, "got status %u\n", bool_async_handler->status );
    IAsyncOperationCompletedHandler_boolean_Release( &bool_async_handler->IAsyncOperationCompletedHandler_boolean_iface );

    hr = IAsyncOperation_boolean_QueryInterface( bool_async, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IAsyncInfo_Cancel( async_info );
    ok( hr == S_OK, "Cancel returned %#lx\n", hr );
    check_bool_async( bool_async, 1, Error, 0x8685400d, FALSE );
    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "Close returned %#lx\n", hr );
    check_bool_async( bool_async, 1, 4, 0x8685400d, FALSE );
    IAsyncInfo_Release( async_info );

    IAsyncOperation_boolean_Release( bool_async );


    set_hid_expect( file, &expect_enable, sizeof(expect_enable) );
    hr = IForceFeedbackMotor_TryEnableAsync( motor, &bool_async );
    ok( hr == S_OK, "TryEnableAsync returned %#lx\n", hr );
    wait_hid_expect( file, 100 );
    IAsyncOperation_boolean_Release( bool_async );


    set_hid_expect( file, expect_reset, sizeof(expect_reset) );
    hr = IForceFeedbackMotor_TryResetAsync( motor, &bool_async );
    ok( hr == S_OK, "TryResetAsync returned %#lx\n", hr );
    wait_hid_expect( file, 100 );
    IAsyncOperation_boolean_Release( bool_async );


    hr = pWindowsCreateString( force_feedback_motor, wcslen( force_feedback_motor ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IInspectable, (void **)&tmp_inspectable );
    ok( hr == REGDB_E_CLASSNOTREG, "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );


    hr = pWindowsCreateString( periodic_effect_class_name, wcslen( periodic_effect_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IPeriodicForceEffectFactory, (void **)&periodic_factory );
    ok( hr == S_OK, "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );

    check_interface( periodic_factory, &IID_IUnknown, TRUE );
    check_interface( periodic_factory, &IID_IInspectable, TRUE );
    check_interface( periodic_factory, &IID_IAgileObject, TRUE );
    check_interface( periodic_factory, &IID_IActivationFactory, TRUE );
    check_interface( periodic_factory, &IID_IPeriodicForceEffectFactory, TRUE );
    check_interface( periodic_factory, &IID_IConditionForceEffectFactory, FALSE );

    hr = IPeriodicForceEffectFactory_QueryInterface( periodic_factory, &IID_IActivationFactory, (void **)&activation_factory );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IActivationFactory_ActivateInstance( activation_factory, &tmp_inspectable );
    ok( hr == E_NOTIMPL, "ActivateInstance returned %#lx\n", hr );
    IActivationFactory_Release( activation_factory );

    hr = IPeriodicForceEffectFactory_CreateInstance( periodic_factory, PeriodicForceEffectKind_SawtoothWaveUp, &effect );
    ok( hr == S_OK, "CreateInstance returned %#lx\n", hr );

    check_interface( effect, &IID_IUnknown, TRUE );
    check_interface( effect, &IID_IInspectable, TRUE );
    check_interface( effect, &IID_IAgileObject, TRUE );
    check_interface( effect, &IID_IForceFeedbackEffect, TRUE );
    check_interface( effect, &IID_IPeriodicForceEffect, TRUE );
    check_interface( effect, &IID_IConditionForceEffect, FALSE );
    check_runtimeclass( effect, periodic_effect_class_name );

    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IPeriodicForceEffect, (void **)&periodic_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IPeriodicForceEffect_get_Kind( periodic_effect, &periodic_kind );
    ok( hr == S_OK, "get_Kind returned %#lx\n", hr );
    ok( periodic_kind == PeriodicForceEffectKind_SawtoothWaveUp, "got kind %u\n", periodic_kind );
    hr = IPeriodicForceEffect_SetParameters( periodic_effect, direction, 1.0, 0.1, 0.0, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    hr = IPeriodicForceEffect_SetParametersWithEnvelope( periodic_effect, direction, 100.0, 0.1, 0.2, 0.3, 0.4, 0.5,
                                                         delay, attack_duration, duration, release_duration, 1 );
    ok( hr == S_OK, "SetParametersWithEnvelope returned %#lx\n", hr );
    IPeriodicForceEffect_Release( periodic_effect );

    gain = 12345.6;
    hr = IForceFeedbackEffect_get_Gain( effect, &gain );
    ok( hr == S_OK, "get_Gain returned %#lx\n", hr );
    ok( gain == 1.0, "got gain %f\n", gain );
    hr = IForceFeedbackEffect_put_Gain( effect, 0.5 );
    ok( hr == S_FALSE, "put_Gain returned %#lx\n", hr );
    state = 0xdeadbeef;
    hr = IForceFeedbackEffect_get_State( effect, &state );
    ok( hr == S_OK, "get_State returned %#lx\n", hr );
    ok( state == ForceFeedbackEffectState_Stopped, "got state %#x\n", state );
    hr = IForceFeedbackEffect_Start( effect );
    todo_wine
    ok( hr == 0x86854003, "Start returned %#lx\n", hr );
    hr = IForceFeedbackEffect_Stop( effect );
    todo_wine
    ok( hr == 0x86854003, "Stop returned %#lx\n", hr );

    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Error, 0x86854008, ForceFeedbackLoadEffectResult_EffectNotSupported );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );

    hr = IForceFeedbackEffect_Start( effect );
    todo_wine
    ok( hr == 0x86854003, "Start returned %#lx\n", hr );
    hr = IForceFeedbackEffect_Stop( effect );
    todo_wine
    ok( hr == 0x86854003, "Stop returned %#lx\n", hr );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    todo_wine
    ok( hr == 0x86854003, "TryUnloadEffectAsync returned %#lx\n", hr );

    IForceFeedbackEffect_Release( effect );


    hr = IPeriodicForceEffectFactory_CreateInstance( periodic_factory, PeriodicForceEffectKind_SineWave, &effect );
    ok( hr == S_OK, "CreateInstance returned %#lx\n", hr );

    check_interface( effect, &IID_IUnknown, TRUE );
    check_interface( effect, &IID_IInspectable, TRUE );
    check_interface( effect, &IID_IAgileObject, TRUE );
    check_interface( effect, &IID_IForceFeedbackEffect, TRUE );
    check_interface( effect, &IID_IPeriodicForceEffect, TRUE );
    check_interface( effect, &IID_IConditionForceEffect, FALSE );
    check_runtimeclass( effect, periodic_effect_class_name );

    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IPeriodicForceEffect, (void **)&periodic_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IPeriodicForceEffect_get_Kind( periodic_effect, &periodic_kind );
    ok( hr == S_OK, "get_Kind returned %#lx\n", hr );
    ok( periodic_kind == PeriodicForceEffectKind_SineWave, "got kind %u\n", periodic_kind );
    hr = IPeriodicForceEffect_SetParameters( periodic_effect, direction, 1.0, 0.1, 0.0, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    hr = IPeriodicForceEffect_SetParametersWithEnvelope( periodic_effect, direction, 100.0, 0.1, 0.2, 0.3, 0.4, 0.5,
                                                         delay, duration, duration, duration, 1 );
    ok( hr == S_OK, "SetParametersWithEnvelope returned %#lx\n", hr );
    IPeriodicForceEffect_Release( periodic_effect );

    /* Windows.Gaming.Input always uses the X and Y directions on Windows,
     * ignoring what is declared in the Axes Enable collection at the
     * Set Effects report, or even the existence of the axes in the HID
     * report. It ignores the Z direction, at least on HID PID devices.
     * DirectInput works properly in such cases on Windows.
     */

    set_hid_expect( file, expect_create_periodic, sizeof(expect_create_periodic) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );

    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IPeriodicForceEffect, (void **)&periodic_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    direction.X = -direction.X;
    hr = IPeriodicForceEffect_SetParameters( periodic_effect, direction, 1.0, 0.1, 0.0, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    direction.X = -direction.X;
    IPeriodicForceEffect_Release( periodic_effect );

    set_hid_expect( file, expect_create_periodic_neg, sizeof(expect_create_periodic_neg) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    IForceFeedbackEffect_Release( effect );

    IPeriodicForceEffectFactory_Release( periodic_factory );


    hr = pWindowsCreateString( condition_effect_class_name, wcslen( condition_effect_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IConditionForceEffectFactory, (void **)&condition_factory );
    ok( hr == S_OK, "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );

    check_interface( condition_factory, &IID_IUnknown, TRUE );
    check_interface( condition_factory, &IID_IInspectable, TRUE );
    check_interface( condition_factory, &IID_IAgileObject, TRUE );
    check_interface( condition_factory, &IID_IActivationFactory, TRUE );
    check_interface( condition_factory, &IID_IConditionForceEffectFactory, TRUE );
    check_interface( condition_factory, &IID_IPeriodicForceEffectFactory, FALSE );

    hr = IConditionForceEffectFactory_QueryInterface( condition_factory, &IID_IActivationFactory, (void **)&activation_factory );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IActivationFactory_ActivateInstance( activation_factory, &tmp_inspectable );
    ok( hr == E_NOTIMPL, "ActivateInstance returned %#lx\n", hr );
    IActivationFactory_Release( activation_factory );


    hr = IConditionForceEffectFactory_CreateInstance( condition_factory, ConditionForceEffectKind_Spring, &effect );
    ok( hr == S_OK, "CreateInstance returned %#lx\n", hr );

    check_interface( effect, &IID_IUnknown, TRUE );
    check_interface( effect, &IID_IInspectable, TRUE );
    check_interface( effect, &IID_IAgileObject, TRUE );
    check_interface( effect, &IID_IForceFeedbackEffect, TRUE );
    check_interface( effect, &IID_IConditionForceEffect, TRUE );
    check_interface( effect, &IID_IPeriodicForceEffect, FALSE );
    check_runtimeclass( effect, condition_effect_class_name );

    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IConditionForceEffect, (void **)&condition_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IConditionForceEffect_get_Kind( condition_effect, &condition_kind );
    ok( hr == S_OK, "get_Kind returned %#lx\n", hr );
    ok( condition_kind == ConditionForceEffectKind_Spring, "got kind %u\n", condition_kind );
    hr = IConditionForceEffect_SetParameters( condition_effect, direction, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    IConditionForceEffect_Release( condition_effect );

    set_hid_expect( file, expect_create_condition, sizeof(expect_create_condition) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );

    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IConditionForceEffect, (void **)&condition_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    direction.X = -direction.X;
    hr = IConditionForceEffect_SetParameters( condition_effect, direction, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6 );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    direction.X = -direction.X;
    IConditionForceEffect_Release( condition_effect );

    set_hid_expect( file, expect_create_condition_neg, sizeof(expect_create_condition_neg) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    IForceFeedbackEffect_Release( effect );

    IConditionForceEffectFactory_Release( condition_factory );


    hr = pWindowsCreateString( constant_effect_class_name, wcslen( constant_effect_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IActivationFactory, (void **)&activation_factory );
    ok( hr == S_OK, "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );

    hr = IActivationFactory_ActivateInstance( activation_factory, &tmp_inspectable );
    ok( hr == S_OK, "ActivateInstance returned %#lx\n", hr );
    IActivationFactory_Release( activation_factory );

    hr = IInspectable_QueryInterface( tmp_inspectable, &IID_IForceFeedbackEffect, (void **)&effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    IInspectable_Release( tmp_inspectable );

    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IConstantForceEffect, (void **)&constant_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IConstantForceEffect_SetParameters( constant_effect, direction, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    hr = IConstantForceEffect_SetParametersWithEnvelope( constant_effect, direction, 0.1, 0.2, 0.3,
                                                         delay, attack_duration, duration, release_duration, 1 );
    ok( hr == S_OK, "SetParametersWithEnvelope returned %#lx\n", hr );
    IConstantForceEffect_Release( constant_effect );

    gain = 12345.6;
    hr = IForceFeedbackEffect_get_Gain( effect, &gain );
    ok( hr == S_OK, "get_Gain returned %#lx\n", hr );
    ok( gain == 1.0, "get_MasterGain returned %f\n", gain );
    hr = IForceFeedbackEffect_put_Gain( effect, 0.5 );
    ok( hr == S_FALSE, "put_Gain returned %#lx\n", hr );
    state = 0xdeadbeef;
    hr = IForceFeedbackEffect_get_State( effect, &state );
    ok( hr == S_OK, "get_State returned %#lx\n", hr );
    ok( state == ForceFeedbackEffectState_Stopped, "get_State returned %#lx\n", hr );
    hr = IForceFeedbackEffect_Start( effect );
    todo_wine
    ok( hr == 0x86854003, "Start returned %#lx\n", hr );
    hr = IForceFeedbackEffect_Stop( effect );
    todo_wine
    ok( hr == 0x86854003, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_create_constant, sizeof(expect_create_constant) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );

    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IConstantForceEffect, (void **)&constant_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    direction.X = -direction.X;
    direction.Y = -direction.Y;
    direction.Z = -direction.Z;
    hr = IConstantForceEffect_SetParameters( constant_effect, direction, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    direction.X = -direction.X;
    direction.Y = -direction.Y;
    direction.Z = -direction.Z;
    IConstantForceEffect_Release( constant_effect );

    set_hid_expect( file, expect_create_constant_neg, sizeof(expect_create_constant_neg) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );

    IForceFeedbackEffect_Release( effect );


    hr = pWindowsCreateString( ramp_effect_class_name, wcslen( ramp_effect_class_name ), &str );
    ok( hr == S_OK, "WindowsCreateString returned %#lx\n", hr );
    hr = pRoGetActivationFactory( str, &IID_IActivationFactory, (void **)&activation_factory );
    ok( hr == S_OK, "RoGetActivationFactory returned %#lx\n", hr );
    pWindowsDeleteString( str );

    hr = IActivationFactory_ActivateInstance( activation_factory, &tmp_inspectable );
    ok( hr == S_OK, "ActivateInstance returned %#lx\n", hr );
    IActivationFactory_Release( activation_factory );

    hr = IInspectable_QueryInterface( tmp_inspectable, &IID_IForceFeedbackEffect, (void **)&effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    IInspectable_Release( tmp_inspectable );

    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IRampForceEffect, (void **)&ramp_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    hr = IRampForceEffect_SetParameters( ramp_effect, direction, end_direction, duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    hr = IRampForceEffect_SetParametersWithEnvelope( ramp_effect, direction, end_direction, 0.1, 0.2, 0.3,
                                                     delay, attack_duration, duration, release_duration, 1 );
    ok( hr == S_OK, "SetParametersWithEnvelope returned %#lx\n", hr );
    IRampForceEffect_Release( ramp_effect );

    set_hid_expect( file, expect_create_ramp, sizeof(expect_create_ramp) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_start, sizeof(expect_effect_start) );
    hr = IForceFeedbackEffect_Start( effect );
    ok( hr == S_OK, "Start returned %#lx\n", hr );

    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );
    set_hid_expect( file, &expect_effect_stop, sizeof(expect_effect_stop) );
    hr = IForceFeedbackEffect_Stop( effect );
    ok( hr == S_OK, "Stop returned %#lx\n", hr );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IRampForceEffect, (void **)&ramp_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    hr = IRampForceEffect_SetParameters( ramp_effect, direction, end_direction, infinite_duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    IRampForceEffect_Release( ramp_effect );

    set_hid_expect( file, expect_create_ramp_inf, sizeof(expect_create_ramp_inf) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    hr = IForceFeedbackEffect_QueryInterface( effect, &IID_IRampForceEffect, (void **)&ramp_effect );
    ok( hr == S_OK, "QueryInterface returned %#lx\n", hr );
    direction.X = -direction.X;
    direction.Y = -direction.Y;
    end_direction.X = -end_direction.X;
    end_direction.Z = -end_direction.Z;
    hr = IRampForceEffect_SetParameters( ramp_effect, direction, end_direction, infinite_duration );
    ok( hr == S_OK, "SetParameters returned %#lx\n", hr );
    direction.X = -direction.X;
    direction.Y = -direction.Y;
    end_direction.X = -end_direction.X;
    end_direction.Z = -end_direction.Z;
    IRampForceEffect_Release( ramp_effect );

    set_hid_expect( file, expect_create_ramp_neg, sizeof(expect_create_ramp_neg) );
    hr = IForceFeedbackMotor_LoadEffectAsync( motor, effect, &result_async );
    ok( hr == S_OK, "LoadEffectAsync returned %#lx\n", hr );
    await_result( result_async );
    check_result_async( result_async, 1, Completed, S_OK, ForceFeedbackLoadEffectResult_Succeeded );
    IAsyncOperation_ForceFeedbackLoadEffectResult_Release( result_async );
    set_hid_expect( file, NULL, 0 );

    set_hid_expect( file, expect_unload, sizeof(expect_unload) );
    hr = IForceFeedbackMotor_TryUnloadEffectAsync( motor, effect, &bool_async );
    ok( hr == S_OK, "TryUnloadEffectAsync returned %#lx\n", hr );
    await_bool( bool_async );
    check_bool_async( bool_async, 1, Completed, S_OK, TRUE );
    IAsyncOperation_boolean_Release( bool_async );
    set_hid_expect( file, NULL, 0 );


    IForceFeedbackEffect_Release( effect );


    IForceFeedbackMotor_Release( motor );

    IRawGameController_Release( raw_controller );

    CloseHandle( file );
    IRawGameControllerStatics_Release( controller_statics );

done:
    hid_device_stop( &desc, 1 );
    cleanup_registry_keys();
}

START_TEST( force_feedback )
{
    dinput_test_init();
    if (!bus_device_start()) goto done;

    if (test_force_feedback_joystick( 0x800 ))
    {
        test_force_feedback_joystick( 0x500 );
        test_force_feedback_joystick( 0x700 );
        test_device_managed_effect();
        test_force_feedback_six_axes();
        test_windows_gaming_input(); /* keep it last, seems to mess with other tests */
    }

done:
    bus_device_stop();
    dinput_test_exit();
}
