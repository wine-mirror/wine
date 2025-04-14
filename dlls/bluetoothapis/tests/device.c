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
#include <winuser.h>

#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include <wine/debug.h>
#include <wine/test.h>

extern void test_for_all_radios( const char *file, int line, void (*test)( HANDLE radio, void *data ), void *data );

static const char *debugstr_bluetooth_address( const BYTE *addr )
{
    return wine_dbg_sprintf( "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
}

static const char *debugstr_BLUETOOTH_DEVICE_INFO( const BLUETOOTH_DEVICE_INFO *info )
{
    WCHAR last_seen[256];
    WCHAR last_used[256];

    last_seen[0] = last_used[0] = 0;

    GetTimeFormatEx( NULL, TIME_FORCE24HOURFORMAT, &info->stLastSeen, NULL, last_seen, ARRAY_SIZE( last_seen ) );
    GetTimeFormatEx( NULL, TIME_FORCE24HOURFORMAT, &info->stLastUsed, NULL, last_used, ARRAY_SIZE( last_used ) );

    return wine_dbg_sprintf( "{ Address: %s ulClassOfDevice: %#lx fConnected: %d fRemembered: %d fAuthenticated: %d "
                             "stLastSeen: %s stLastUsed: %s szName: %s }",
                             debugstr_bluetooth_address( info->Address.rgBytes ), info->ulClassofDevice,
                             info->fConnected, info->fRemembered, info->fAuthenticated, debugstr_w( last_seen ),
                             debugstr_w( last_used ), debugstr_w( info->szName ) );
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
        BOOL matches;

        matches = (info.fConnected && search_params.fReturnConnected)
            || (info.fAuthenticated && search_params.fReturnAuthenticated)
            || (info.fRemembered && search_params.fReturnRemembered)
            || (!info.fRemembered && search_params.fReturnUnknown);
        ok( matches, "Device does not match filter constraints\n" );
        trace( "device %lu: %s\n", i, debugstr_BLUETOOTH_DEVICE_INFO( &info ) );

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

    success = BluetoothFindDeviceClose( hfind );
    ok( success, "BluetoothFindDeviceClose failed: %lu\n", GetLastError() );
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

static HANDLE auth_events[2];

static void test_auth_callback_params( int line, const BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS *auth_params )
{
    ok_( __FILE__, line )( !!auth_params, "Expected authentication params to not be NULL\n" );
    if (auth_params)
    {
        ULARGE_INTEGER ft = {0};

        trace_( __FILE__, line )( "Device: %s\n", debugstr_BLUETOOTH_DEVICE_INFO( &auth_params->deviceInfo ) );
        trace_( __FILE__, line )( "Method: %#x\n", auth_params->authenticationMethod );
        trace_( __FILE__, line )( "Numeric value: %lu\n", auth_params->Numeric_Value );

        SystemTimeToFileTime( &auth_params->deviceInfo.stLastSeen, (FILETIME *)&ft );
        ok( ft.QuadPart, "Expected stLastSeen to be greater than zero\n" );
        ft.QuadPart = 0;
        SystemTimeToFileTime( &auth_params->deviceInfo.stLastUsed, (FILETIME *)&ft );
        ok( ft.QuadPart, "Expected stLastUsed to be greater than zero\n" );
    }
}

static CALLBACK BOOL auth_ex_callback( void *data, BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS *auth_params )
{
    char msg[400];

    test_auth_callback_params( __LINE__, auth_params );
    if (auth_params)
    {
        DWORD ret, exp;
        BLUETOOTH_AUTHENTICATE_RESPONSE resp = {0};

        /* Invalid parameters with an active registration */
        ret = BluetoothSendAuthenticationResponseEx( NULL, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );

        resp.bthAddressRemote.ullLong = 0xdeadbeefcafe;
        ret = BluetoothSendAuthenticationResponseEx( NULL, &resp );
        ok( ret == ERROR_INVALID_PARAMETER, "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );

        /* Non-existent device. */
        resp.authMethod = auth_params->authenticationMethod;
        ret = BluetoothSendAuthenticationResponseEx( NULL, &resp );
        ok( ret == ERROR_DEVICE_NOT_CONNECTED, "%lu != %d\n", ret, ERROR_DEVICE_NOT_CONNECTED );

        /* Invalid auth method */
        resp.bthAddressRemote = auth_params->deviceInfo.Address;
        resp.authMethod = 0xdeadbeef;
        ret = BluetoothSendAuthenticationResponseEx( NULL, &resp );
        ok( ret == ERROR_INVALID_PARAMETER, "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );

        resp.authMethod = auth_params->authenticationMethod;
        resp.numericCompInfo.NumericValue = auth_params->Numeric_Value;

        snprintf( msg, ARRAY_SIZE( msg ), "Accept auth request from %s (address %s, method %d, passkey %lu)?",
                  debugstr_w( auth_params->deviceInfo.szName ),
                  debugstr_bluetooth_address( auth_params->deviceInfo.Address.rgBytes ),
                  auth_params->authenticationMethod, auth_params->Numeric_Value );
        ret = MessageBoxA( NULL, msg, __FILE__, MB_YESNO );
        ok( ret, "MessageBoxA failed: %lu\n", GetLastError() );

        resp.negativeResponse = ret != IDYES;

        ret = BluetoothSendAuthenticationResponseEx( NULL, &resp );
        exp = resp.negativeResponse ? ERROR_NOT_AUTHENTICATED : ERROR_SUCCESS;
        todo_wine_if( auth_params->authenticationMethod != BLUETOOTH_AUTHENTICATION_METHOD_NUMERIC_COMPARISON )
            ok( ret == exp, "%lu != %lu\n", ret, exp );

        ret = BluetoothSendAuthenticationResponseEx( NULL, &resp );
        todo_wine_if( auth_params->authenticationMethod != BLUETOOTH_AUTHENTICATION_METHOD_NUMERIC_COMPARISON )
            ok( ret == ERROR_NOT_READY, "%lu != %d\n", ret, ERROR_NOT_READY );
    }
    SetEvent( auth_events[0] );
    return TRUE;
}

static CALLBACK BOOL auth_ex_callback_2( void *data, BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS *auth_params )
{
    test_auth_callback_params( __LINE__, auth_params );
    SetEvent( auth_events[1] );
    return TRUE;
}

static void test_BluetoothRegisterForAuthenticationEx( void )
{
    HBLUETOOTH_AUTHENTICATION_REGISTRATION hreg = NULL, hreg2 = NULL;
    BLUETOOTH_DEVICE_INFO info;
    BOOL success;
    DWORD err;

    err = BluetoothRegisterForAuthenticationEx( NULL, NULL, NULL, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );

    info.dwSize = sizeof( info ) + 1;
    err = BluetoothRegisterForAuthenticationEx( &info, &hreg, NULL, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );

    auth_events[0] = CreateEventA( NULL, FALSE, FALSE, NULL );
    auth_events[1] = CreateEventA( NULL, FALSE, FALSE, NULL );

    BluetoothEnableIncomingConnections( NULL, TRUE );
    BluetoothEnableDiscovery( NULL, TRUE );

    err = BluetoothRegisterForAuthenticationEx( NULL, &hreg, auth_ex_callback, NULL );
    ok( !err, "BluetoothRegisterForAuthenticationEx failed: %lu\n", err );
    ok( !!hreg, "Handle was not filled\n" );

    err = BluetoothRegisterForAuthenticationEx( NULL, &hreg2, auth_ex_callback_2, NULL );
    ok( !err, "BluetoothRegisterForAuthenticationEx failed: %lu\n", err );
    ok( !!hreg2, "Handle was not filled\n" );

    trace( "Waiting for incoming authentication requests.\n" );
    err = WaitForMultipleObjects( 2, auth_events, TRUE, 60000 );
    ok( !err, "WaitForMultipleObjects failed: %lu\n", err );

    success = BluetoothUnregisterAuthentication( hreg );
    ok( success, "BluetoothUnregisterAuthentication failed: %lu\n", GetLastError() );
    success = BluetoothUnregisterAuthentication( hreg2 );
    ok( success, "BluetoothUnregisterAuthentication failed: %lu\n", GetLastError() );

    BluetoothEnableDiscovery( NULL, FALSE );
}


static void test_radio_BluetoothSendAuthenticationResponseEx( HANDLE radio , void *data )
{
    BLUETOOTH_AUTHENTICATE_RESPONSE resp = {0};
    DWORD ret;

    ret = BluetoothSendAuthenticationResponseEx( radio, NULL );

    ok( ret == ERROR_INVALID_PARAMETER || (!radio && ret == ERROR_NO_MORE_ITEMS),
        "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );

    ret = BluetoothSendAuthenticationResponseEx( radio, &resp );
    ok( ret == ERROR_INVALID_PARAMETER || (!radio && ret == ERROR_NO_MORE_ITEMS),
        "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );

    resp.bthAddressRemote.ullLong = 0xdeadbeefcafe;
    ret = BluetoothSendAuthenticationResponseEx( radio, &resp );
    ok( ret == ERROR_INVALID_PARAMETER || (!radio && ret == ERROR_NO_MORE_ITEMS),
        "%lu != %d\n", ret, ERROR_INVALID_PARAMETER );
}

/* Test BluetoothSendAuthenticationResponseEx when no authentication registrations exist. */
static void test_BluetoothSendAuthenticationResponseEx( void )
{
    test_radio_BluetoothSendAuthenticationResponseEx( NULL, (void *)__LINE__ );
    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothSendAuthenticationResponseEx, NULL );
}

static void test_radio_BluetoothRemoveDevice( HANDLE radio, void *data )
{
    BLUETOOTH_DEVICE_SEARCH_PARAMS search_params = {0};
    BLUETOOTH_DEVICE_INFO device_info = {0};
    HBLUETOOTH_DEVICE_FIND find;
    DWORD err;

    device_info.dwSize = sizeof( device_info );
    search_params.dwSize = sizeof( search_params );
    search_params.fReturnAuthenticated = TRUE;
    search_params.fReturnRemembered = TRUE;
    search_params.fReturnUnknown = TRUE;
    search_params.fReturnConnected = TRUE;
    search_params.hRadio = radio;

    find = BluetoothFindFirstDevice( &search_params, &device_info );
    err = GetLastError();
    ok( find || err == ERROR_NO_MORE_ITEMS, "BluetoothFindFirstDevice failed: %lu\n", err );
    if (!find)
    {
        skip( "No devices found.\n" );
        return;
    }

    do {
        DWORD exp;
        winetest_push_context( "%s (%s)", debugstr_w( device_info.szName ),
                               debugstr_bluetooth_address( device_info.Address.rgBytes ) );
        err = BluetoothRemoveDevice( &device_info.Address );
        exp = device_info.fRemembered ? ERROR_SUCCESS : ERROR_NOT_FOUND;
        ok( err == exp, "%lu != %lu\n", err, exp );
        err = BluetoothGetDeviceInfo( radio, &device_info );
        ok( !err || err == ERROR_NOT_FOUND, "BluetoothGetDeviceInfo failed: %lu\n", err );
        if (!err)
            ok( !device_info.fRemembered, "%d != %d\n", device_info.fRemembered, 0 );
        winetest_pop_context();
    } while (BluetoothFindNextDevice( find, &device_info ));

    BluetoothFindDeviceClose( find );
}

static void test_BluetoothRemoveDevice( void )
{
    DWORD err;
    BLUETOOTH_ADDRESS addr_zeros = {0};

    err = BluetoothRemoveDevice( &addr_zeros );
    ok( err == ERROR_NOT_FOUND, "%lu != %d\n", err, ERROR_NOT_FOUND );

    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothRemoveDevice, NULL );
}

START_TEST( device )
{
    test_BluetoothFindFirstDevice();
    test_BluetoothFindDeviceClose();
    test_BluetoothFindNextDevice();

    if (winetest_interactive)
        test_BluetoothRegisterForAuthenticationEx();
    test_BluetoothSendAuthenticationResponseEx();
    test_BluetoothRemoveDevice();
}
