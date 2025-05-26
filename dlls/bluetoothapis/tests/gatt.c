/*
 * Tests for Bluetooth GATT methods
 *
 * Copyright 2025 Vibhav Pant
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
#include <stdint.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <setupapi.h>

#include <setupapi.h>

#include <initguid.h>
#include <bthledef.h>
#include <bluetoothleapis.h>
#include <devpkey.h>
#include <ddk/bthguid.h>

#include <wine/test.h>

static void test_for_all_le_devices( int line, void (*func)( HANDLE, void * ), void *data )
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof( WCHAR )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    SP_DEVICE_INTERFACE_DATA iface_data;
    HDEVINFO devinfo;
    DWORD idx = 0, ret, n = 0;
    BOOL found = FALSE;

    devinfo = SetupDiGetClassDevsW( &GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    ret = GetLastError();
    ok( devinfo != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed: %lu\n", ret );

    iface_detail->cbSize = sizeof( *iface_detail );
    iface_data.cbSize = sizeof( iface_data );
    while (SetupDiEnumDeviceInterfaces( devinfo, NULL, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, idx++,
                                        &iface_data ))
    {
        SP_DEVINFO_DATA devinfo_data;
        HANDLE device;
        DEVPROPTYPE type;
        WCHAR addr_str[13];
        BOOL success;

        devinfo_data.cbSize = sizeof( devinfo_data );
        success = SetupDiGetDeviceInterfaceDetailW( devinfo, &iface_data, iface_detail, sizeof( buffer ), NULL,
                                                    &devinfo_data );
        ok( success, "SetupDiGetDeviceInterfaceDetailW failed: %lu\n", GetLastError() );
        addr_str[0] = '\0';
        success = SetupDiGetDevicePropertyW( devinfo, &devinfo_data, &DEVPKEY_Bluetooth_DeviceAddress, &type,
                                             (BYTE *)addr_str, sizeof( addr_str ), NULL, 0 );
        ok( success, "SetupDiGetDevicePropertyW failed: %lu\n", GetLastError() );
        ok( type == DEVPROP_TYPE_STRING, "got type %lu\n", type );
        device = CreateFileW( iface_detail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
        winetest_push_context( "device %lu", n++ );
        trace( "Address: %s\n", debugstr_w( addr_str ) );
        func( device, data );
        winetest_pop_context();
        CloseHandle( device );
        found = TRUE;
    }

    SetupDiDestroyDeviceInfoList( devinfo );
    if (!found)
        skip_( __FILE__, line )( "No LE devices found.\n" );
}

static const char *debugstr_BTH_LE_UUID( const BTH_LE_UUID *uuid )
{
    if (uuid->IsShortUuid)
        return wine_dbg_sprintf("{ IsShortUuid=1 {%#x} }", uuid->Value.ShortUuid );
    return wine_dbg_sprintf( "{ IsShortUuid=0 %s }", debugstr_guid( &uuid->Value.LongUuid ) );
}

static const char *debugstr_BTH_LE_GATT_SERVICE( const BTH_LE_GATT_SERVICE *svc )
{
    return wine_dbg_sprintf( "{ %s %#x }", debugstr_BTH_LE_UUID( &svc->ServiceUuid ), svc->AttributeHandle );
}

static const char *debugstr_BTH_LE_GATT_CHARACTERISTIC( const BTH_LE_GATT_CHARACTERISTIC *chrc )
{
    return wine_dbg_sprintf( "{ %#x %s %#x %#x %d %d %d %d %d %d %d %d }", chrc->ServiceHandle,
                             debugstr_BTH_LE_UUID( &chrc->CharacteristicUuid ), chrc->AttributeHandle,
                             chrc->CharacteristicValueHandle, chrc->IsBroadcastable, chrc->IsReadable, chrc->IsWritable,
                             chrc->IsWritableWithoutResponse, chrc->IsSignedWritable, chrc->IsNotifiable,
                             chrc->IsIndicatable, chrc->HasExtendedProperties );
}

static void test_service_BluetoothGATTGetCharacteristics( HANDLE device, BTH_LE_GATT_SERVICE *service )
{
    HRESULT ret;
    USHORT actual = 0, actual2 = 0, i;
    BTH_LE_GATT_CHARACTERISTIC *buf = NULL, dummy;

    ret = BluetoothGATTGetCharacteristics( device, service, 0, &dummy, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, NULL, 0, NULL, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, service, 0, NULL, &actual, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ), "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, service, actual, NULL, &actual2, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ), "got ret %#lx\n", ret );
    ok( actual == actual2, "%u != %u\n", actual, actual2 );

    buf = calloc( actual, sizeof( *buf ) );
    ret = BluetoothGATTGetCharacteristics( device, service, actual, buf, &actual, 0 );
    ok( ret == S_OK, "BluetoothGATTGetCharacteristics failed: %#lx\n", ret );

    for (i = 0; i < actual; i++)
        trace( "characteristic %u: %s\n", i, debugstr_BTH_LE_GATT_CHARACTERISTIC( &buf[i] ) );

    free( buf );
}

static void test_device_BluetoothGATTGetServices( HANDLE device, void *param )
{
    HRESULT ret;
    USHORT actual = 0, i;
    BTH_LE_GATT_SERVICE *buf;

    ret = BluetoothGATTGetServices( device, 0, NULL, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );

    buf = calloc( 1, sizeof( *buf ) );
    ret = BluetoothGATTGetServices( device, 1, buf, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );
    ret = BluetoothGATTGetServices( NULL, 1, buf, &actual, 0 );
    ok( ret == E_HANDLE, "got ret %#lx\n", ret );
    ret = BluetoothGATTGetServices( INVALID_HANDLE_VALUE, 1, buf, &actual, 0 );
    ok( ret == E_HANDLE, "got ret %#lx\n", ret );
    ret = BluetoothGATTGetServices( device, 0, buf, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetServices( device, 0, NULL, &actual, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_NOT_FOUND ) || ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ),
        "BluetoothGATTGetServices failed: %#lx\n", ret );
    if (!actual)
    {
        free( buf );
        return;
    }

    buf = calloc( actual, sizeof( *buf ) );
    ret = BluetoothGATTGetServices( device, actual, buf, &actual, 0 );
    ok( ret == S_OK, "BluetoothGATTGetServices failed: %#lx\n", ret );
    if (FAILED( ret ))
    {
        skip( "BluetoothGATTGetServices failed.\n" );
        free( buf );
        return;
    }

    for (i = 0; i < actual; i++)
    {
        winetest_push_context( "service %u", i );
        trace( "%s\n", debugstr_BTH_LE_GATT_SERVICE( &buf[i] ) );
        test_service_BluetoothGATTGetCharacteristics( device, &buf[i] );
        winetest_pop_context();
    }

    free( buf );
}

static void test_BluetoothGATTGetServices( void )
{
    HRESULT ret;

    ret = BluetoothGATTGetServices( NULL, 0, NULL, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );

    test_for_all_le_devices( __LINE__, test_device_BluetoothGATTGetServices, NULL );
}

static void test_device_BluetoothGATTGetCharacteristics( HANDLE device, void *data )
{
    HRESULT ret;
    USHORT actual = 0;
    BTH_LE_GATT_SERVICE svc = {.ServiceUuid = {TRUE, {.ShortUuid = 0xdead}}, 0xbeef};

    ret = BluetoothGATTGetCharacteristics( device, NULL, 0, NULL, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, &svc, 0, NULL, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, &svc, 0, NULL, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );
}

static void test_BluetoothGATTGetCharacteristic( void )
{
    HRESULT ret;
    USHORT actual = 0;
    BTH_LE_GATT_SERVICE svc;

    ret = BluetoothGATTGetCharacteristics( NULL, NULL, 0, NULL, NULL, 0 );
    ok( ret == E_POINTER, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( NULL, &svc, 0, NULL, &actual, 0 );
    ok( ret == E_HANDLE, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( INVALID_HANDLE_VALUE, &svc, 0, NULL, &actual, 0 );
    ok( ret == E_HANDLE, "got ret %#lx\n", ret );

    test_for_all_le_devices( __LINE__, test_device_BluetoothGATTGetCharacteristics, NULL );
}

START_TEST( gatt )
{
    test_BluetoothGATTGetServices();
    test_BluetoothGATTGetCharacteristic();
}
