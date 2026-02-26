/*
 * Tests for Bluetooth GATT methods
 *
 * Copyright 2025-2026 Vibhav Pant
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

static void le_to_uuid( const BTH_LE_UUID *le_uuid, GUID *uuid )
{
    if (le_uuid->IsShortUuid)
    {
        *uuid = BTH_LE_ATT_BLUETOOTH_BASE_GUID;
        uuid->Data1 = le_uuid->Value.ShortUuid;
    }
    else
        *uuid = le_uuid->Value.LongUuid;
}

static void test_for_all_le_devices( int line, void (*func)( HANDLE, const WCHAR *, void * ), void *data )
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
        func( device, addr_str, data );
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

static BTH_LE_GATT_CHARACTERISTIC_VALUE *get_characteristic_value( int line, HANDLE service, BTH_LE_GATT_CHARACTERISTIC *chrc, ULONG flags )
{
    BTH_LE_GATT_CHARACTERISTIC_VALUE *val;
    USHORT actual;
    HRESULT ret;
    ULONG size;

    ret = BluetoothGATTGetCharacteristicValue( service, chrc, 0, NULL, &actual, flags );
    ok_( __FILE__, line )( HRESULT_CODE( ret ) == ERROR_MORE_DATA, "%lu != %d\n", HRESULT_CODE( ret ), ERROR_MORE_DATA );
    if (FAILED( ret ) && HRESULT_CODE( ret ) != ERROR_MORE_DATA)
        return NULL;

    size = actual;
    val = calloc( 1, size );
    ret = BluetoothGATTGetCharacteristicValue( service, chrc, size, val, &actual, flags );
    ok_( __FILE__, line )( ret == S_OK, "BluetoothGATTGetCharacteristicValue failed: %#lx\n", ret );
    if (FAILED( ret ))
    {
        free( val );
        return NULL;
    }
    ok_( __FILE__, line )( actual == size, "%u != %lu\n", actual, size );
    return val;
}

static void test_service_BluetoothGATTGetCharacteristicValue( HANDLE service, BTH_LE_GATT_CHARACTERISTIC *chrc )
{
    BTH_LE_GATT_CHARACTERISTIC_VALUE *val1, *val2;
    char buf[5];
    USHORT actual = 0;
    HRESULT ret;

    ret = BluetoothGATTGetCharacteristicValue( NULL, NULL, 0, NULL, NULL, 0 );
    ok( HRESULT_CODE( ret ) == ERROR_INVALID_HANDLE, "%lu != %d\n", HRESULT_CODE( ret ), ERROR_INVALID_HANDLE );
    ret = BluetoothGATTGetCharacteristicValue( service, NULL, 0, NULL, NULL, 0 );
    ok( HRESULT_CODE( ret ) == ERROR_INVALID_PARAMETER, "%lu != %d\n", HRESULT_CODE ( ret ), ERROR_INVALID_PARAMETER );
    ret = BluetoothGATTGetCharacteristicValue( service, NULL, 0, NULL, &actual, 0 );
    ok( HRESULT_CODE( ret ) == ERROR_INVALID_PARAMETER, "%lu != %d\n", HRESULT_CODE ( ret ), ERROR_INVALID_PARAMETER );
    ret = BluetoothGATTGetCharacteristicValue( service, chrc, 0, NULL, NULL, 0 );
    ok( HRESULT_CODE( ret ) == ERROR_INVALID_PARAMETER, "%lu != %d\n", HRESULT_CODE ( ret ), ERROR_INVALID_PARAMETER );
    ret = BluetoothGATTGetCharacteristicValue( service, chrc, sizeof( buf ), (void *)&buf, &actual, 0 );
    ok( HRESULT_CODE( ret ) == ERROR_INVALID_PARAMETER, "%lu != %d\n", HRESULT_CODE( ret ), ERROR_INVALID_PARAMETER );
    ret = BluetoothGATTGetCharacteristicValue( service, chrc, 100, NULL, NULL, 0 );
    ok( ret == E_POINTER, "%#lx != %#lx\n", ret, E_POINTER );
    ret = BluetoothGATTGetCharacteristicValue( service, chrc, 100, NULL, &actual, 0 );
    ok( ret == E_POINTER, "%#lx != %#lx\n", ret, E_POINTER );

    if (!chrc->IsReadable)
    {
        ret = BluetoothGATTGetCharacteristicValue( service, chrc, 0, NULL, &actual, 0 );
        ok( HRESULT_CODE( ret ) == ERROR_INVALID_ACCESS, "%lu != %d\n", HRESULT_CODE( ret ), ERROR_INVALID_ACCESS );
        return;
    }

    val1 = get_characteristic_value( __LINE__, service, chrc, 0 );
    if (val1)
        trace( "value: %lu bytes\n", val1->DataSize );
    free( val1 );

    val1 = get_characteristic_value( __LINE__, service, chrc, BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE );
    val2 = get_characteristic_value( __LINE__, service, chrc, BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_CACHE );
    if (val1 && val2)
    {
        ok( val1->DataSize == val2->DataSize, "%lu != %lu\n", val1->DataSize, val2->DataSize );
        if (val1->DataSize == val2->DataSize)
            ok( !memcmp( val1->Data, val2->Data, val1->DataSize ),
                "Cached value does not match value previously read from device.\n" );
    }
    else
        skip( "Couldn't read characteristic value.\n" );
    free( val1 );
    free( val2 );
}

static void test_service_BluetoothGATTGetCharacteristics( HANDLE service, const BTH_LE_GATT_CHARACTERISTIC *chars, USHORT len )
{
    HRESULT ret;
    USHORT actual = 0, i, match = 0;
    BTH_LE_GATT_CHARACTERISTIC *chars2;

    ret = BluetoothGATTGetCharacteristics( service, NULL, 0, NULL, &actual, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ), "got ret %#lx\n", ret );
    if (!actual)
        return;

    actual = len;
    chars2 = calloc( actual, sizeof( *chars2 ) );
    ret = BluetoothGATTGetCharacteristics( service, NULL, actual, chars2, &actual, 0 );
    ok( ret == S_OK, "BluetoothGATTGetCharacteristics failed: %#lx\n", ret );
    ok( actual == len, "%u != %u\n", actual, len );

     /* Characteristics retrieved using the device and GATT service handle should match. */
    for (i = 0; i < actual; i++)
    {
        const BTH_LE_GATT_CHARACTERISTIC *chrc1 = &chars[i];
        USHORT j;

        winetest_push_context( "chars[%u]", i );
        for (j = 0; j < actual; j++)
        {
            const BTH_LE_GATT_CHARACTERISTIC *chrc2 = &chars2[j];

            if (IsBthLEUuidMatch( chrc1->CharacteristicUuid, chrc2->CharacteristicUuid )
                && chrc1->ServiceHandle == chrc2->ServiceHandle
                && chrc1->AttributeHandle == chrc2->AttributeHandle)
            {
                winetest_push_context( "chars2[%u]", j );
                match++;
                ok( chrc1->ServiceHandle == chrc2->ServiceHandle, "%#x != %#x\n", chrc1->ServiceHandle,
                    chrc2->ServiceHandle );
                ok( chrc1->AttributeHandle == chrc2->AttributeHandle, "%#x != %#x\n", chrc1->AttributeHandle,
                    chrc2->AttributeHandle );
                ok( chrc1->CharacteristicValueHandle == chrc2->CharacteristicValueHandle, "%#x != %#x\n",
                    chrc1->CharacteristicValueHandle, chrc2->CharacteristicValueHandle );

#define TEST_BOOL(field) ok( chrc1->field == chrc2->field, "%d != %d\n", chrc1->field, chrc2->field )
                TEST_BOOL(IsBroadcastable);
                TEST_BOOL(IsReadable);
                TEST_BOOL(IsWritable);
                TEST_BOOL(IsWritableWithoutResponse);
                TEST_BOOL(IsSignedWritable);
                TEST_BOOL(IsNotifiable);
                TEST_BOOL(IsIndicatable);
                TEST_BOOL(HasExtendedProperties);
#undef TEST_BOOL
                winetest_pop_context();
                break;
            }
        }
        winetest_pop_context();
    }

    ok( actual == match, "%u != %u\n", actual, match );
    free( chars2 );
}

static void test_BluetoothGATTGetCharacteristics( HANDLE device, HANDLE service, BTH_LE_GATT_SERVICE *service_info )
{
    HRESULT ret;
    USHORT actual = 0, actual2 = 0, i;
    BTH_LE_GATT_CHARACTERISTIC *buf = NULL, dummy;

    ret = BluetoothGATTGetCharacteristics( device, service, 0, &dummy, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, NULL, 0, NULL, &actual, 0 );
    ok( ret == E_INVALIDARG, "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, service_info, 0, NULL, &actual, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ), "got ret %#lx\n", ret );

    ret = BluetoothGATTGetCharacteristics( device, service_info, actual, NULL, &actual2, 0 );
    ok( ret == HRESULT_FROM_WIN32( ERROR_MORE_DATA ), "got ret %#lx\n", ret );
    ok( actual == actual2, "%u != %u\n", actual, actual2 );

    buf = calloc( actual, sizeof( *buf ) );
    ret = BluetoothGATTGetCharacteristics( device, service_info, actual, buf, &actual, 0 );
    ok( ret == S_OK, "BluetoothGATTGetCharacteristics failed: %#lx\n", ret );

    for (i = 0; i < actual; i++)
    {
        winetest_push_context( "characteristic %u", i );
        trace( "%s\n", debugstr_BTH_LE_GATT_CHARACTERISTIC( &buf[i] ) );
        if (service)
            test_service_BluetoothGATTGetCharacteristicValue( service, &buf[i] );
        winetest_pop_context();
    }

    if (service)
        test_service_BluetoothGATTGetCharacteristics( service, buf, actual );
    else
        skip( "Could not obtain handle to GATT service.\n" );
    free( buf );
}

static HANDLE get_gatt_service_iface( const WCHAR *device_addr_str, const GUID *service_uuid )
{
    char buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof( WCHAR )];
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)buffer;
    HANDLE device = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA iface_data;
    DWORD idx = 0, ret;
    HDEVINFO devinfo;

    devinfo = SetupDiGetClassDevsW( &GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, NULL, NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );
    ret = GetLastError();
    ok( devinfo != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed: %lu\n", ret );
    if (devinfo == INVALID_HANDLE_VALUE)
        return NULL;

    iface_detail->cbSize = sizeof( *iface_detail );
    iface_data.cbSize = sizeof( iface_data );
    while (SetupDiEnumDeviceInterfaces( devinfo, NULL, &GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, idx++,
                                        &iface_data ))
    {
        SP_DEVINFO_DATA devinfo_data = {0};
        WCHAR addr_str[13];
        DEVPROPTYPE type;
        GUID uuid = {0};
        BOOL success;

        devinfo_data.cbSize = sizeof( devinfo_data );
        success = SetupDiGetDeviceInterfaceDetailW( devinfo, &iface_data, iface_detail, sizeof( buffer ), NULL,
                                                    &devinfo_data );
        ok( success, "SetupDiGetDeviceInterfaceDetailW failed: %lu\n", GetLastError() );
        addr_str[0] = 0;
        success = SetupDiGetDevicePropertyW( devinfo, &devinfo_data, &DEVPKEY_Bluetooth_DeviceAddress, &type, (BYTE *)addr_str,
                                             sizeof( addr_str ), NULL, 0 );
        ok( success, "SetupDiGetDevicePropertyW failed: %lu\n", GetLastError() );
        ok( type == DEVPROP_TYPE_STRING, "got type %#lx\n", type );
        if (wcsicmp( addr_str, device_addr_str ))
            continue;
        success = SetupDiGetDevicePropertyW( devinfo, &devinfo_data, &DEVPKEY_Bluetooth_ServiceGUID, &type,
                                             (BYTE *)&uuid, sizeof( uuid ), NULL, 0 );
        ok( success, "SetupDiGetDevicePropertyW failed: %lu\n", GetLastError() );
        ok( type == DEVPROP_TYPE_GUID, "got type %#lx\n", type );
        if (!IsEqualGUID( &uuid, service_uuid ))
            continue;

        device = CreateFileW( iface_detail->DevicePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        ret = GetLastError();
        break;
    }
    SetupDiDestroyDeviceInfoList( devinfo );
    ok( device != INVALID_HANDLE_VALUE || broken( ret == ERROR_GEN_FAILURE ),
        "Could not find device interface for GATT service %s (error %lu) \n", debugstr_guid( service_uuid ), ret );
    return device == INVALID_HANDLE_VALUE ? NULL : device;
}

static void test_device_BluetoothGATTGetServices( HANDLE device, const WCHAR *device_addr_str, void *param )
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
        HANDLE service;
        GUID uuid;

        winetest_push_context( "service %u", i );
        trace( "%s\n", debugstr_BTH_LE_GATT_SERVICE( &buf[i] ) );

        le_to_uuid( &buf[i].ServiceUuid, &uuid );
        service = get_gatt_service_iface( device_addr_str, &uuid );
        test_BluetoothGATTGetCharacteristics( device, service, &buf[i] );
        if (service)
            CloseHandle( service );

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

static void test_device_BluetoothGATTGetCharacteristics( HANDLE device, const WCHAR *addr, void *data )
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
    BTH_LE_GATT_SERVICE svc = {0};

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
