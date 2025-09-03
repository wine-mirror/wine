/*
 * Unit test suite for kernel mode graphics driver
 *
 * Copyright 2019 Zhiyi Zhang
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "dwmapi.h"
#include "ddk/d3dkmthk.h"
#include "initguid.h"
#include "setupapi.h"
#include "ntddvdeo.h"
#include "devpkey.h"
#include "cfgmgr32.h"

#include "wine/test.h"

static const WCHAR display1W[] = L"\\\\.\\DISPLAY1";

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

static const char *debugstr_ok( const char *cond )
{
    int c, n = 0;
    /* skip possible casts */
    while ((c = *cond++))
    {
        if (c == '(') n++;
        if (!n) break;
        if (c == ')') n--;
    }
    if (!strchr( cond - 1, '(' )) return wine_dbg_sprintf( "got %s", cond - 1 );
    return wine_dbg_sprintf( "%.*s returned", (int)strcspn( cond - 1, "( " ), cond - 1 );
}

#define ok_ex( r, op, e, t, f, ... )                                                               \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v op (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_nt( e, r )       ok_ex( r, ==, e, NTSTATUS, "%#lx" )

#define check_d3dkmt_global( a ) check_d3dkmt_global_( __LINE__, a )
static void check_d3dkmt_global_( int line, UINT_PTR handle )
{
    ok_( __FILE__, line )( handle & 0xc0000000, "got %#Ix\n", handle );
    ok_( __FILE__, line )( (handle & 0x3f) == 2, "got %#Ix\n", handle );
}

#define check_d3dkmt_local( a, b ) check_d3dkmt_local_( __LINE__, a, b )
static void check_d3dkmt_local_( int line, D3DKMT_HANDLE handle, D3DKMT_HANDLE *next_local )
{
    ok_(__FILE__, line)( handle & 0xc0000000, "got %#x\n", handle );
    ok_(__FILE__, line)( handle && !(handle & 0x3f), "got %#x\n", handle );
    if (next_local && *next_local) ok_(__FILE__, line)( handle == *next_local, "got %#x, expected %#x\n", handle, *next_local );
    if (next_local) *next_local = handle + 0x40;
}

static BOOL get_primary_adapter_name( WCHAR *name )
{
    DISPLAY_DEVICEW dd;
    DWORD adapter_idx;

    dd.cb = sizeof(dd);
    for (adapter_idx = 0; EnumDisplayDevicesW( NULL, adapter_idx, &dd, 0 ); ++adapter_idx)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW( name, dd.DeviceName );
            return TRUE;
        }
    }

    return FALSE;
}

static void test_D3DKMTOpenAdapterFromGdiDisplayName(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    DWORD i;

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );
    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Invalid parameters */
    status = D3DKMTOpenAdapterFromGdiDisplayName( NULL );
    ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );

    memset( &open_adapter_gdi_desc, 0, sizeof(open_adapter_gdi_desc) );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
    {
        lstrcpyW( open_adapter_gdi_desc.DeviceName, display_device.DeviceName );
        status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        else
        {
            ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );
            continue;
        }
        check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );
        ok( open_adapter_gdi_desc.AdapterLuid.LowPart || open_adapter_gdi_desc.AdapterLuid.HighPart,
            "Expect LUID not zero.\n" );

        close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }
}

static void test_D3DKMTOpenAdapterFromHdc(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMHDC open_adapter_hdc_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    INT adapter_count = 0;
    NTSTATUS status;
    HDC hdc;
    DWORD i;

    /* Invalid parameters */
    /* Passing a NULL pointer crashes on Windows 10 >= 2004 */
    if (0) status = D3DKMTOpenAdapterFromHdc( NULL );

    memset( &open_adapter_hdc_desc, 0, sizeof(open_adapter_hdc_desc) );
    status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
    {
        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;

        adapter_count++;

        hdc = CreateDCW( 0, display_device.DeviceName, 0, NULL );
        open_adapter_hdc_desc.hDc = hdc;
        status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
        todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        todo_wine check_d3dkmt_local( open_adapter_hdc_desc.hAdapter, &next_local );
        DeleteDC( hdc );

        if (status == STATUS_SUCCESS)
        {
            close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
            status = D3DKMTCloseAdapter( &close_adapter_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        }
    }

    /* HDC covering more than two adapters is invalid for D3DKMTOpenAdapterFromHdc */
    hdc = GetDC( 0 );
    open_adapter_hdc_desc.hDc = hdc;
    status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
    ReleaseDC( 0, hdc );
    todo_wine ok( status == (adapter_count > 1 ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS),
                  "Got unexpected return code %#lx.\n", status );
    if (status == STATUS_SUCCESS)
    {
        check_d3dkmt_local( open_adapter_hdc_desc.hAdapter, &next_local );
        close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }
}

static void test_D3DKMTEnumAdapters2(void)
{
    D3DKMT_ENUMADAPTERS2 enum_adapters_2_desc = {0};
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    UINT i;

    /* Invalid parameters */
    status = D3DKMTEnumAdapters2( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Query the array to allocate */
    memset( &enum_adapters_2_desc, 0, sizeof(enum_adapters_2_desc) );
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ok( enum_adapters_2_desc.NumAdapters == 32 /* win10 and older */ || enum_adapters_2_desc.NumAdapters == 34 /* win11 */,
        "Got unexpected value %lu.\n", enum_adapters_2_desc.NumAdapters );

    /* Allocate the array */
    enum_adapters_2_desc.pAdapters = calloc( enum_adapters_2_desc.NumAdapters, sizeof(D3DKMT_ADAPTERINFO) );
    ok( !!enum_adapters_2_desc.pAdapters, "Expect not null.\n" );

    /* Enumerate adapters */
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ok( enum_adapters_2_desc.NumAdapters, "Expect not zero.\n" );

    for (i = 0; i < enum_adapters_2_desc.NumAdapters; ++i)
    {
        check_d3dkmt_local( enum_adapters_2_desc.pAdapters[i].hAdapter, &next_local );
        ok( enum_adapters_2_desc.pAdapters[i].AdapterLuid.LowPart ||
            enum_adapters_2_desc.pAdapters[i].AdapterLuid.HighPart,
            "Expect LUID not zero.\n" );

        close_adapter_desc.hAdapter = enum_adapters_2_desc.pAdapters[i].hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }

    /* Check for insufficient buffer */
    enum_adapters_2_desc.NumAdapters = 0;
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_BUFFER_TOO_SMALL, "Got unexpected return code %#lx.\n", status );

    free( enum_adapters_2_desc.pAdapters );
}

static void test_D3DKMTCloseAdapter(void)
{
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTCloseAdapter( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &close_adapter_desc, 0, sizeof(close_adapter_desc) );
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCreateDevice(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTCreateDevice( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    /* Create device */
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );
    ok( create_device_desc.pCommandBuffer == NULL, "Expect null.\n" );
    ok( create_device_desc.CommandBufferSize == 0, "Got wrong value %#x.\n", create_device_desc.CommandBufferSize );
    ok( create_device_desc.pAllocationList == NULL, "Expect null.\n" );
    ok( create_device_desc.AllocationListSize == 0, "Got wrong value %#x.\n", create_device_desc.AllocationListSize );
    ok( create_device_desc.pPatchLocationList == NULL, "Expect null.\n" );
    ok( create_device_desc.PatchLocationListSize == 0, "Got wrong value %#x.\n",
        create_device_desc.PatchLocationListSize );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTDestroyDevice(void)
{
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTDestroyDevice( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &destroy_device_desc, 0, sizeof(destroy_device_desc) );
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCheckVidPnExclusiveOwnership(void)
{
    static const DWORD timeout = 1000;
    static const DWORD wait_step = 100;
    D3DKMT_CREATEDEVICE create_device_desc, create_device_desc2;
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    D3DKMT_HANDLE next_local = 0;
    DWORD total_time;
    NTSTATUS status;
    INT i;

    /* Test cases using single device */
    static const struct test_data1
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
        NTSTATUS expected_set_status;
        NTSTATUS expected_check_status;
    }
    tests1[] =
    {
        /* 0 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 10 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 20 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        /* 30 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 40 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 50 */
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED + 1, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
    };

    /* Test cases using two devices consecutively */
    static const struct test_data2
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type1;
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type2;
        NTSTATUS expected_set_status1;
        NTSTATUS expected_set_status2;
        NTSTATUS expected_check_status;
    }
    tests2[] =
    {
        /* 0 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 10 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
    };

    /* Invalid parameters */
    status = D3DKMTCheckVidPnExclusiveOwnership( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &check_owner_desc, 0, sizeof(check_owner_desc) );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Test cases */
    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    for (i = 0; i < ARRAY_SIZE(tests1); ++i)
    {
        set_owner_desc.hDevice = create_device_desc.hDevice;
        if (tests1[i].owner_type != -1)
        {
            owner_type = tests1[i].owner_type;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
        }
        else
        {
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
        }
        status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
        ok( status == tests1[i].expected_set_status, "Got unexpected return code %#lx at test %d.\n", status, i );

        status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
        /* If don't sleep, D3DKMTCheckVidPnExclusiveOwnership may get
         * STATUS_GRAPHICS_PRESENT_UNOCCLUDED instead of STATUS_SUCCESS */
        if ((tests1[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep( wait_step );
                total_time += wait_step;
                status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok( status == tests1[i].expected_check_status, "Got unexpected return code %#lx at test %d.\n", status, i );
    }

    /* Set owner and unset owner using different devices */
    memset( &create_device_desc2, 0, sizeof(create_device_desc2) );
    create_device_desc2.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc2 );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc2.hDevice, &next_local );

    /* Set owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset owner with the second device */
    set_owner_desc.hDevice = create_device_desc2.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* No effect */
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* Proves that the correct device is needed to unset owner */
    ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
        "Got unexpected return code %#lx.\n", status );

    /* Set owner with the first device, set owner again with the second device */
    for (i = 0; i < ARRAY_SIZE(tests2); ++i)
    {
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            owner_type = tests2[i].set_owner_type1;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            /* If don't sleep, D3DKMTSetVidPnSourceOwner may return STATUS_OK for
             * D3DKMT_VIDPNSOURCEOWNER_SHARED. Other owner type doesn't seems to be affected. */
            if (tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_SHARED) Sleep( timeout );
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == tests2[i].expected_set_status1, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            owner_type = tests2[i].set_owner_type2;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == tests2[i].expected_set_status2, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
        if ((tests2[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep( wait_step );
                total_time += wait_step;
                status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok( status == tests2[i].expected_check_status, "Got unexpected return code %#lx at test %d.\n", status, i );

        /* Unset owner with first device */
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        /* Unset owner with second device */
        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i );
        }
    }

    /* Destroy devices holding ownership */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    set_owner_desc.hDevice = create_device_desc2.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    /* So ownership is released when device is destroyed. otherwise the return code should be
     * STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE */
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc2.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTSetVidPnSourceOwner(void)
{
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc = {0};
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCheckOcclusion(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CHECKOCCLUSION occlusion_desc;
    NTSTATUS expected_occlusion, status;
    D3DKMT_HANDLE next_local = 0;
    INT i, adapter_count = 0;
    HWND hwnd, hwnd2;
    HRESULT hr;

    /* NULL parameter check */
    status = D3DKMTCheckOcclusion( NULL );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = NULL;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    hwnd = CreateWindowA( "static", "static1", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 200, 200, 0, 0, 0, 0 );
    ok( hwnd != NULL, "Failed to create window.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Minimized state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow( hwnd, SW_MINIMIZE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOWNORMAL );

    /* Invisible state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow( hwnd, SW_HIDE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOW );

    /* hwnd2 covers hwnd */
    hwnd2 = CreateWindowA( "static", "static2", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200,
                           200, 0, 0, 0, 0 );
    ok( hwnd2 != NULL, "Failed to create window.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Composition doesn't affect D3DKMTCheckOcclusion */
    hr = DwmEnableComposition( DWM_EC_DISABLECOMPOSITION );
    ok( hr == S_OK, "Failed to disable composition.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    /* This result means that D3DKMTCheckOcclusion doesn't check composition status despite MSDN says it will */
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    ShowWindow( hwnd, SW_MINIMIZE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOWNORMAL );

    ShowWindow( hwnd, SW_HIDE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOW );

    hr = DwmEnableComposition( DWM_EC_ENABLECOMPOSITION );
    ok( hr == S_OK, "Failed to enable composition.\n" );

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* D3DKMTCheckVidPnExclusiveOwnership gets STATUS_GRAPHICS_PRESENT_UNOCCLUDED sometimes and with
     * some delay, it will always return STATUS_SUCCESS. So there are some timing issues here. */
    ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
        "Got unexpected return code %#lx.\n", status );

    /* Test D3DKMTCheckOcclusion relationship with video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
        if ((display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) adapter_count++;
    /* STATUS_GRAPHICS_PRESENT_OCCLUDED on single monitor system. STATUS_SUCCESS on multiple monitor system. */
    expected_occlusion = adapter_count > 1 ? STATUS_SUCCESS : STATUS_GRAPHICS_PRESENT_OCCLUDED;

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    /* Note hwnd2 is not actually occluded but D3DKMTCheckOcclusion reports STATUS_GRAPHICS_PRESENT_OCCLUDED as well */
    SetWindowPos( hwnd2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
    ShowWindow( hwnd2, SW_SHOW );
    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    /* Now hwnd is HWND_TOPMOST. Still reports STATUS_GRAPHICS_PRESENT_OCCLUDED */
    ok( SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ),
        "Failed to SetWindowPos.\n" );
    ok( GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_TOPMOST, "No WS_EX_TOPMOST style.\n" );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    DestroyWindow( hwnd2 );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    flaky ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
              "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    DestroyWindow( hwnd );
}

static void test_D3DKMTOpenAdapterFromDeviceName_deviface( const GUID *devinterface_guid, NTSTATUS expected_status, BOOL todo )
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    DEVPROPTYPE type;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;
    LUID luid;
    BOOL ret;

    set = SetupDiGetClassDevsW( devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError() );

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);
    device_name.pDeviceName = iface_data->DevicePath;

    i = 0;
    while (SetupDiEnumDeviceInterfaces( set, NULL, devinterface_guid, i, &iface ))
    {
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device_data );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

        status = D3DKMTOpenAdapterFromDeviceName( &device_name );
        ok( status == expected_status, "Got status %#lx, expected %#lx.\n", status, expected_status );

        if (!status)
        {
            check_d3dkmt_local( device_name.hAdapter, &next_local );

            ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPROPKEY_GPU_LUID, &type,
                                             (BYTE *)&luid, sizeof(luid), NULL, 0 );
            ok( ret || GetLastError() == ERROR_NOT_FOUND, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

            if (ret)
            {
                ret = RtlEqualLuid( &luid, &device_name.AdapterLuid );
                ok( ret, "Luid does not match.\n" );
            }
            else
            {
                skip( "Luid not found.\n" );
            }

            close_adapter_desc.hAdapter = device_name.hAdapter;
            status = D3DKMTCloseAdapter( &close_adapter_desc );
            ok( !status, "Got unexpected status %#lx.\n", status );
        }
        ++i;
    }
    if (!i) win_skip( "No devices found.\n" );

    SetupDiDestroyDeviceInfoList( set );
}

static void test_D3DKMTOpenAdapterFromDeviceName(void)
{
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    NTSTATUS status;

    /* Make sure display devices are initialized. */
    SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );

    status = D3DKMTOpenAdapterFromDeviceName( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );

    memset( &device_name, 0, sizeof(device_name) );
    status = D3DKMTOpenAdapterFromDeviceName( &device_name );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );

    winetest_push_context( "GUID_DEVINTERFACE_DISPLAY_ADAPTER" );
    test_D3DKMTOpenAdapterFromDeviceName_deviface( &GUID_DEVINTERFACE_DISPLAY_ADAPTER, STATUS_INVALID_PARAMETER, TRUE );
    winetest_pop_context();

    winetest_push_context( "GUID_DISPLAY_DEVICE_ARRIVAL" );
    test_D3DKMTOpenAdapterFromDeviceName_deviface( &GUID_DISPLAY_DEVICE_ARRIVAL, STATUS_SUCCESS, FALSE );
    winetest_pop_context();
}

static void test_D3DKMTQueryVideoMemoryInfo(void)
{
    static const D3DKMT_MEMORY_SEGMENT_GROUP groups[] = {D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL, D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_desc;
    D3DKMT_QUERYVIDEOMEMORYINFO query_memory_info;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    unsigned int i;
    BOOL ret;

    ret = get_primary_adapter_name( open_adapter_desc.DeviceName );
    ok( ret, "Failed to get primary adapter name.\n" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_desc.hAdapter, &next_local );

    /* Normal query */
    for (i = 0; i < ARRAY_SIZE(groups); ++i)
    {
        winetest_push_context( "group %d", groups[i] );

        query_memory_info.hProcess = NULL;
        query_memory_info.hAdapter = open_adapter_desc.hAdapter;
        query_memory_info.PhysicalAdapterIndex = 0;
        query_memory_info.MemorySegmentGroup = groups[i];
        status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
        todo_wine_if( status == STATUS_INVALID_PARAMETER ) /* fails on Wine without a Vulkan adapter */
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        ok( query_memory_info.Budget >= query_memory_info.AvailableForReservation,
            "Unexpected budget %I64u and reservation %I64u.\n", query_memory_info.Budget,
            query_memory_info.AvailableForReservation );
        ok( query_memory_info.CurrentUsage <= query_memory_info.Budget,
            "Unexpected current usage %I64u.\n", query_memory_info.CurrentUsage );
        ok( query_memory_info.CurrentReservation == 0, "Unexpected current reservation %I64u.\n",
            query_memory_info.CurrentReservation );

        winetest_pop_context();
    }

    /* Query using the current process handle */
    query_memory_info.hProcess = GetCurrentProcess();
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    todo_wine_if( status == STATUS_INVALID_PARAMETER ) /* fails on Wine without a Vulkan adapter */
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Query using a process handle without PROCESS_QUERY_INFORMATION privilege */
    query_memory_info.hProcess = OpenProcess( PROCESS_SET_INFORMATION, FALSE, GetCurrentProcessId() );
    ok( !!query_memory_info.hProcess, "OpenProcess failed, error %ld.\n", GetLastError() );
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_ACCESS_DENIED, "Got unexpected return code %#lx.\n", status );
    CloseHandle( query_memory_info.hProcess );
    query_memory_info.hProcess = NULL;

    /* Query using an invalid process handle */
    query_memory_info.hProcess = (HANDLE)0xdeadbeef;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_HANDLE, "Got unexpected return code %#lx.\n", status );
    query_memory_info.hProcess = NULL;

    /* Query using an invalid adapter handle */
    query_memory_info.hAdapter = (D3DKMT_HANDLE)0xdeadbeef;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
    query_memory_info.hAdapter = open_adapter_desc.hAdapter;

    /* Query using an invalid adapter index */
    query_memory_info.PhysicalAdapterIndex = 99;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
    query_memory_info.PhysicalAdapterIndex = 0;

    /* Query using an invalid memory segment group */
    query_memory_info.MemorySegmentGroup = D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL + 1;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_gpu_device_properties_guid( const GUID *devinterface_guid )
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    WCHAR device_id[256];
    DEVPROPTYPE type;
    unsigned int i;
    UINT32 value;
    HDEVINFO set;
    BOOL ret;

    /* Make sure display devices are initialized. */
    SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );

    set = SetupDiGetClassDevsW( devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError() );

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);

    i = 0;
    while (SetupDiEnumDeviceInterfaces( set, NULL, devinterface_guid, i, &iface ))
    {
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device_data );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_MatchingDeviceId, &type,
                                         (BYTE *)device_id, sizeof(device_id), NULL, 0 );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );
        ok( type == DEVPROP_TYPE_STRING, "Got type %ld.\n", type );

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_BusNumber, &type,
                                         (BYTE *)&value, sizeof(value), NULL, 0 );
        if (!wcsicmp( device_id, L"root\\basicrender" ) ||
            !wcsicmp( device_id, L"root\\basicdisplay" ))
            ok( !ret, "Found Bus Id.\n" );
        else
        {
            ok( ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(),
                debugstr_w(device_id));
            ok( type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type );
        }

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_RemovalPolicy, &type,
                                         (BYTE *)&value, sizeof(value), NULL, 0 );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(), debugstr_w(device_id));
        ok( value == CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL || value == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL ||
            value == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL, "Got value %d.\n", value );
        ok( type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type );
        ++i;
    }
    SetupDiDestroyDeviceInfoList( set );
}

static void test_gpu_device_properties(void)
{
    winetest_push_context( "GUID_DEVINTERFACE_DISPLAY_ADAPTER" );
    test_gpu_device_properties_guid( &GUID_DEVINTERFACE_DISPLAY_ADAPTER );
    winetest_pop_context();
    winetest_push_context( "GUID_DISPLAY_DEVICE_ARRIVAL" );
    test_gpu_device_properties_guid( &GUID_DISPLAY_DEVICE_ARRIVAL );
    winetest_pop_context();
}

static void test_D3DKMTQueryAdapterInfo(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_desc;
    D3DKMT_QUERYADAPTERINFO query_adapter_info;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    unsigned char buffer[1024];
    NTSTATUS status;
    unsigned int i;
    BOOL ret;

    static const struct
    {
        KMTQUERYADAPTERINFOTYPE type;
        UINT size;
    }
    tests[] =
    {
        {KMTQAITYPE_CHECKDRIVERUPDATESTATUS, sizeof(BOOL)},
        {KMTQAITYPE_DRIVERVERSION, sizeof(D3DKMT_DRIVERVERSION)},
    };

    ret = get_primary_adapter_name( open_adapter_desc.DeviceName );
    ok( ret, "Failed to get primary adapter name.\n" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_desc.hAdapter, &next_local );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context( "type %d", tests[i].type );

        /* NULL buffer */
        query_adapter_info.hAdapter = open_adapter_desc.hAdapter;
        query_adapter_info.Type = tests[i].type;
        query_adapter_info.pPrivateDriverData = NULL;
        query_adapter_info.PrivateDriverDataSize = tests[i].size;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

        /* Insufficient buffer size */
        query_adapter_info.pPrivateDriverData = buffer;
        query_adapter_info.PrivateDriverDataSize = tests[i].size - 1;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

        /* Normal */
        query_adapter_info.pPrivateDriverData = buffer;
        query_adapter_info.PrivateDriverDataSize = tests[i].size;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        if (status != STATUS_SUCCESS)
        {
            winetest_pop_context();
            continue;
        }

        switch (tests[i].type)
        {
        case KMTQAITYPE_CHECKDRIVERUPDATESTATUS:
        {
            BOOL *value = query_adapter_info.pPrivateDriverData;
            ok( *value == FALSE, "Expected %d, got %d.\n", FALSE, *value );
            break;
        }
        case KMTQAITYPE_DRIVERVERSION:
        {
            D3DKMT_DRIVERVERSION *value = query_adapter_info.pPrivateDriverData;
            ok( *value >= KMT_DRIVERVERSION_WDDM_1_3, "Expected %d >= %d.\n", *value, KMT_DRIVERVERSION_WDDM_1_3 );
            break;
        }
        default:
            ok( 0, "Type %d is not handled.\n", tests[i].type );
            break;
        }

        winetest_pop_context();
    }

    close_adapter_desc.hAdapter = open_adapter_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCreateSynchronizationObject( void )
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter = {0};
    D3DKMT_DESTROYSYNCHRONIZATIONOBJECT destroy = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT2 create2 = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT create = {0};
    D3DKMT_OPENSYNCHRONIZATIONOBJECT open = {0};
    D3DKMT_DESTROYDEVICE destroy_device = {0};
    D3DKMT_CREATEDEVICE create_device = {0};
    D3DKMT_CLOSEADAPTER close_adapter = {0};
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    wcscpy( open_adapter.DeviceName, L"\\\\.\\DISPLAY1" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_adapter.hAdapter, &next_local );
    create_device.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCreateDevice( &create_device );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create_device.hDevice, &next_local );

    /* D3DKMTCreateSynchronizationObject creates a local D3DKMT_HANDLE */
    status = D3DKMTCreateSynchronizationObject( NULL );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = create_device.hDevice;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = 0;
    create.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = create_device.hDevice;
    create.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create.hSyncObject, &next_local );
    destroy.hSyncObject = create.hSyncObject;

    /* local handles are monotonically increasing */
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create.hSyncObject, &next_local );

    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    /* destroying multiple times fails */
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy.hSyncObject = create.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    create.Info.Type = D3DDDI_SEMAPHORE;
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create.hSyncObject, &next_local );
    destroy.hSyncObject = create.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    create.Info.Type = D3DDDI_FENCE;
    status = D3DKMTCreateSynchronizationObject( &create );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );


    /* D3DKMTCreateSynchronizationObject2 can create local D3DKMT_HANDLE */
    status = D3DKMTCreateSynchronizationObject2( NULL );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = create_device.hDevice;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = 0;
    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = create_device.hDevice;
    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );


    create2.Info.Type = D3DDDI_SEMAPHORE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    create2.Info.Type = D3DDDI_FENCE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    create2.Info.Type = D3DDDI_CPU_NOTIFICATION;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_INVALID_HANDLE, status );
    create2.Info.CPUNotification.Event = CreateEventW( NULL, FALSE, FALSE, NULL );
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    CloseHandle( create2.Info.CPUNotification.Event );
    create2.Info.CPUNotification.Event = NULL;

    create2.Info.Type = D3DDDI_MONITORED_FENCE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    if (status == STATUS_SUCCESS)
    {
        todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
        destroy.hSyncObject = create2.hSyncObject;
        status = D3DKMTDestroySynchronizationObject( &destroy );
        todo_wine ok_nt( STATUS_SUCCESS, status );
    }


    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create2.Info.Flags.Shared = 1;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    todo_wine check_d3dkmt_global( create2.Info.SharedHandle );
    destroy.hSyncObject = create2.hSyncObject;

    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    todo_wine check_d3dkmt_global( create2.Info.SharedHandle );

    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    /* cannot destroy the global D3DKMT_HANDLE */
    destroy.hSyncObject = create2.Info.SharedHandle;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );


    /* D3DKMTOpenSynchronizationObject creates a new local D3DKMT_HANDLE */
    open.hSharedHandle = 0x1eadbeed;
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSharedHandle = 0;
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSyncObject = create2.hSyncObject;
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSyncObject = 0x1eadbeed;
    open.hSharedHandle = create2.Info.SharedHandle;
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( open.hSyncObject, &next_local );

    destroy.hSyncObject = open.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    /* destroying multiple times fails */
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );

    /* the D3DKMT object can still be opened */
    open.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( open.hSyncObject, &next_local );

    destroy.hSyncObject = open.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    /* the global D3DKMT_HANDLE is destroyed with last reference */
    status = D3DKMTOpenSynchronizationObject( &open );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );


    /* NtSecuritySharing requires Shared, doesn't creates a global handle */
    create2.Info.Flags.Shared = 0;
    create2.Info.Flags.NtSecuritySharing = 1;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_INVALID_PARAMETER, status );
    if (broken( !status ))
    {
        check_d3dkmt_local( create2.hSyncObject, &next_local );
        destroy.hSyncObject = create2.hSyncObject;
        status = D3DKMTDestroySynchronizationObject( &destroy );
        ok_nt( STATUS_SUCCESS, status );
    }

    create2.Info.Flags.Shared = 1;
    create2.Info.Flags.NtSecuritySharing = 1;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    todo_wine ok_nt( STATUS_SUCCESS, status );
    todo_wine check_d3dkmt_local( create2.hSyncObject, &next_local );
    ok( create2.Info.SharedHandle == 0x1eadbeed || !create2.Info.SharedHandle,
        "got Info.SharedHandle %#x\n", create2.Info.SharedHandle );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    todo_wine ok_nt( STATUS_SUCCESS, status );


    destroy_device.hDevice = create_device.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device );
    ok_nt( STATUS_SUCCESS, status );
    close_adapter.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter );
    ok_nt( STATUS_SUCCESS, status );
}

START_TEST( d3dkmt )
{
    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    test_D3DKMTOpenAdapterFromGdiDisplayName();
    test_D3DKMTOpenAdapterFromHdc();
    test_D3DKMTEnumAdapters2();
    test_D3DKMTCloseAdapter();
    test_D3DKMTCreateDevice();
    test_D3DKMTDestroyDevice();
    test_D3DKMTCheckVidPnExclusiveOwnership();
    test_D3DKMTSetVidPnSourceOwner();
    test_D3DKMTCheckOcclusion();
    test_D3DKMTOpenAdapterFromDeviceName();
    test_D3DKMTQueryAdapterInfo();
    test_D3DKMTQueryVideoMemoryInfo();
    test_gpu_device_properties();
    test_D3DKMTCreateSynchronizationObject();
}
