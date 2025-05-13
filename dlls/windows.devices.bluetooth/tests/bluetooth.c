/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Networking
#include "windows.networking.connectivity.h"
#include "windows.networking.h"
#define WIDL_using_Windows_Devices_Bluetooth
#include "windows.devices.bluetooth.rfcomm.h"
#include "windows.devices.bluetooth.h"

#include "wine/test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    IUnknown_Release( unk );
}

static void test_BluetoothAdapterStatics(void)
{
    static const WCHAR *default_res = L"System.Devices.InterfaceClassGuid:=\"{92383B0E-F90E-4AC9-8D44-8C2D0D0EBDA2}\" "
                                      L"AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True";
    static const WCHAR *bluetoothadapter_statics_name = L"Windows.Devices.Bluetooth.BluetoothAdapter";
    IBluetoothAdapterStatics *bluetoothadapter_statics;
    IActivationFactory *factory;
    HSTRING str, default_str;
    HRESULT hr;
    INT32 res;
    LONG ref;

    hr = WindowsCreateString( bluetoothadapter_statics_name, wcslen( bluetoothadapter_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( bluetoothadapter_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IBluetoothAdapterStatics, (void **)&bluetoothadapter_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IBluetoothAdapterStatics_GetDeviceSelector( bluetoothadapter_statics, NULL );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    hr = IBluetoothAdapterStatics_GetDeviceSelector( bluetoothadapter_statics, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCreateString( default_res, wcslen(default_res), &default_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, default_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got unexpected string %s.\n", debugstr_hstring(str) );

    WindowsDeleteString( str );
    WindowsDeleteString( default_str );
    ref = IBluetoothAdapterStatics_Release( bluetoothadapter_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(bluetooth)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_BluetoothAdapterStatics();

    RoUninitialize();
}
