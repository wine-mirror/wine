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
#define WIDL_using_Windows_Devices_Usb
#include "windows.devices.usb.h"

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

static void test_UsbDevicesStatics(void)
{
    static const WCHAR *format = L"System.Devices.InterfaceClassGuid:=\"{DEE824EF-729B-4A0E-9C14-B7117D33A817}\""
                                 L" AND System.Devices.InterfaceEnabled:=System.StructuredQueryType.Boolean#True"
                                 L" AND System.DeviceInterface.WinUsb.UsbVendorId:=%d"
                                 L" AND System.DeviceInterface.WinUsb.UsbProductId:=%d";
    static const WCHAR *usb_device_statics_name = L"Windows.Devices.Usb.UsbDevice";
    IUsbDeviceStatics *usb_device_statics;
    IActivationFactory *factory;
    UINT32 vendor, product;
    WCHAR buffer[254 + 20];
    HSTRING str, wine_str;
    HRESULT hr;
    INT32 res;
    LONG ref;

    hr = WindowsCreateString( usb_device_statics_name, wcslen( usb_device_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( usb_device_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IUsbDeviceStatics, (void **)&usb_device_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IUsbDeviceStatics_GetDeviceSelectorVidPidOnly( usb_device_statics, 5, 10, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    vendor = 20;
    product = 23;
    hr = IUsbDeviceStatics_GetDeviceSelectorVidPidOnly( usb_device_statics, vendor, product, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    swprintf( buffer, ARRAYSIZE(buffer), format, (INT32)vendor, (INT32)product );
    hr = WindowsCreateString( buffer, wcslen(buffer), &wine_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, wine_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got string %s.\n", debugstr_hstring(str) );
    WindowsDeleteString( str );
    WindowsDeleteString( wine_str );

    vendor = 4294967195; /* Vendor = -101 */
    product = -1294967195;
    hr = IUsbDeviceStatics_GetDeviceSelectorVidPidOnly( usb_device_statics, vendor, product, &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    swprintf( buffer, ARRAYSIZE(buffer), format, (INT32)vendor, (INT32)product );
    hr = WindowsCreateString( buffer, wcslen(buffer), &wine_str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = WindowsCompareStringOrdinal( str, wine_str, &res );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !res, "got string %s.\n", debugstr_hstring(str) );
    WindowsDeleteString( str );
    WindowsDeleteString( wine_str );

    ref = IUsbDeviceStatics_Release( usb_device_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(usb)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_UsbDevicesStatics();

    RoUninitialize();
}
