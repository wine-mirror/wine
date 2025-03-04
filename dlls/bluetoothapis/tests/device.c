/*
 * Tests for Bluetooth device methods
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

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include <wine/test.h>

extern void test_for_all_radios( const char *file, int line, void (*test)( HANDLE radio, void *data ), void *data );

static const char *debugstr_bluetooth_address( const BYTE *addr )
{
    return wine_dbg_sprintf( "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
}

void test_radio_BluetoothFindFirstDevice( HANDLE radio, void *data )
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS search_params;
    BLUETOOTH_DEVICE_INFO device_info;
    HBLUETOOTH_DEVICE_FIND hfind;
    DWORD err, exp;
    BOOL success;

    search_params.dwSize = sizeof( search_params );
    search_params.cTimeoutMultiplier = 200;
    search_params.fIssueInquiry = TRUE;
    search_params.fReturnUnknown = TRUE;
    search_params.hRadio = radio;
    device_info.dwSize = sizeof( device_info );
    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( &search_params, &device_info );
    ok( !hfind, "Expected %p to be NULL\n", hfind );
    err = GetLastError();
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );

    search_params.cTimeoutMultiplier = 5;
    search_params.fIssueInquiry = winetest_interactive;
    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( &search_params, &device_info );
    err = GetLastError();
    exp = hfind ? ERROR_SUCCESS : ERROR_NO_MORE_ITEMS;
    todo_wine_if( !radio ) ok( err == exp, "%lu != %lu\n", err, exp );

    if (hfind)
    {
        success = BluetoothFindDeviceClose( hfind );
        ok( success, "BluetoothFindDeviceClose failed: %lu\n", GetLastError() );
    }
}

void test_BluetoothFindFirstDevice( void )
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS search_params = {0};
    BLUETOOTH_DEVICE_INFO device_info = {0};
    HBLUETOOTH_DEVICE_FIND hfind;
    DWORD err;

    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( NULL, NULL );
    ok( !hfind, "Expected %p to be NULL\n", hfind );
    err = GetLastError();
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );

    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( &search_params, NULL );
    ok( !hfind, "Expected %p to be NULL\n", hfind );
    err = GetLastError();
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );

    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( &search_params, &device_info );
    ok( !hfind, "Expected %p to be NULL\n", hfind );
    err = GetLastError();
    ok( err == ERROR_REVISION_MISMATCH, "%lu != %d\n", err, ERROR_REVISION_MISMATCH );

    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothFindFirstDevice, NULL );
}

void test_radio_BluetoothFindNextDevice( HANDLE radio, void *data )
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS search_params = *(BLUETOOTH_DEVICE_SEARCH_PARAMS *)data;
    BLUETOOTH_DEVICE_INFO info = {0};
    HBLUETOOTH_DEVICE_FIND hfind;
    BOOL success;
    DWORD err, i = 0;

    search_params.hRadio = radio;
    info.dwSize = sizeof( info );

    SetLastError( 0xdeadbeef );
    hfind = BluetoothFindFirstDevice( &search_params, &info );
    err = GetLastError();
    ok( !!hfind || err == ERROR_NO_MORE_ITEMS, "BluetoothFindFirstDevice failed: %lu\n", GetLastError() );
    if (!hfind)
    {
        skip( "No devices found.\n" );
        return;
    }

    for (;;)
    {
        BLUETOOTH_DEVICE_INFO info2 = {0};
        WCHAR buf[256];
        BOOL matches;

        matches = (info.fConnected && search_params.fReturnConnected)
            || (info.fAuthenticated && search_params.fReturnAuthenticated)
            || (info.fRemembered && search_params.fReturnRemembered)
            || (!info.fRemembered && search_params.fReturnUnknown);
        ok( matches, "Device does not match filter constraints\n" );
        trace( "device %lu: %s\n", i, debugstr_bluetooth_address( info.Address.rgBytes) );
        trace( "  name: %s\n", debugstr_w( info.szName ) );
        trace( "  class: %#lx\n", info.ulClassofDevice );
        trace( "  connected: %d, authenticated: %d, remembered: %d\n", info.fConnected, info.fAuthenticated,
               info.fRemembered );
        if (GetTimeFormatEx( NULL, TIME_FORCE24HOURFORMAT, &info.stLastSeen, NULL, buf, ARRAY_SIZE( buf ) ))
            trace( "  last seen: %s UTC\n", debugstr_w( buf ) );

        info2.dwSize = sizeof( info2 );
        info2.Address = info.Address;
        err = BluetoothGetDeviceInfo( radio, &info2 );
        ok( !err, "BluetoothGetDeviceInfo failed: %lu\n", err );
        ok( !memcmp( info2.Address.rgBytes, info.Address.rgBytes, sizeof( info.Address.rgBytes ) ), "%s != %s\n",
            debugstr_bluetooth_address( info2.Address.rgBytes ), debugstr_bluetooth_address( info.Address.rgBytes ) );
        ok( !wcscmp( info2.szName, info.szName ), "%s != %s\n", debugstr_w( info2.szName ), debugstr_w( info.szName ) );
        ok( info2.fAuthenticated == info.fAuthenticated, "%d != %d\n", info.fAuthenticated, info.fAuthenticated );
        ok( info2.fRemembered == info.fRemembered, "%d != %d\n", info.fRemembered, info.fRemembered );
        ok( info2.fAuthenticated == info.fAuthenticated, "%d != %d\n", info.fAuthenticated, info.fAuthenticated );

        memset( &info, 0, sizeof( info ));
        info.dwSize = sizeof( info );
        SetLastError( 0xdeadbeef );
        success = BluetoothFindNextDevice( hfind, &info );
        err = GetLastError();
        ok( success || err == ERROR_NO_MORE_ITEMS, "BluetoothFindNextDevice failed: %lu\n", err );
        if (!success)
            break;
    }

    ok( BluetoothFindDeviceClose( hfind ), "BluetoothFindDeviceClose failed: %lu\n", GetLastError() );
}

void test_BluetoothFindNextDevice( void )
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS params = {0};
    DWORD err;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = BluetoothFindNextDevice( NULL, NULL );
    ok( !ret, "Expected BluetoothFindNextDevice to return FALSE\n" );
    err = GetLastError();
    ok( err == ERROR_INVALID_HANDLE, "%lu != %d\n", err, ERROR_INVALID_HANDLE );

    params.dwSize = sizeof( params );
    params.fReturnUnknown = TRUE;
    params.fReturnRemembered = TRUE;
    params.fReturnConnected = TRUE;
    params.fReturnAuthenticated = TRUE;

    if (winetest_interactive)
    {
        params.fIssueInquiry = TRUE;
        params.cTimeoutMultiplier = 5;
    }

    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothFindNextDevice, &params );
}

void test_BluetoothFindDeviceClose( void )
{
    DWORD err;

    SetLastError( 0xdeadbeef );
    ok( !BluetoothFindDeviceClose( NULL ), "Expected BluetoothFindDeviceClose to return FALSE\n" );
    err = GetLastError();
    ok( err == ERROR_INVALID_HANDLE, "%lu != %d\n", err, ERROR_INVALID_HANDLE );
}

START_TEST( device )
{
    test_BluetoothFindFirstDevice();
    test_BluetoothFindDeviceClose();
    test_BluetoothFindNextDevice();
}
