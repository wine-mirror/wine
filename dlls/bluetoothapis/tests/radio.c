/*
 * Tests for Bluetooth radio methods
 *
 * Copyright 2024 Vibhav Pant
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

#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include <wine/test.h>

void test_BluetoothFindFirstRadio( void )
{
    HANDLE radio, dummy = (HANDLE)0xdeadbeef;
    BLUETOOTH_FIND_RADIO_PARAMS find_params;
    HBLUETOOTH_RADIO_FIND find;
    DWORD err, exp;

    radio = dummy;
    SetLastError( 0xdeadbeef );
    find = BluetoothFindFirstRadio( NULL, &radio );
    ok( !find, "Expected %p to be NULL\n", find );
    err = GetLastError();
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );
    ok( radio == dummy, "%p != %p\n", radio, dummy );

    radio = dummy;
    find_params.dwSize = 0;
    SetLastError( 0xdeadbeef );
    find = BluetoothFindFirstRadio( &find_params, &radio );
    ok( !find, "Expected %p to be NULL\n", find );
    err = GetLastError();
    ok( err == ERROR_REVISION_MISMATCH, "%lu != %d\n", err, ERROR_REVISION_MISMATCH );
    ok( radio == dummy, "%p != %p\n", radio, dummy );

    find_params.dwSize = sizeof( find_params );
    SetLastError( 0xdeadbeef );
    find = BluetoothFindFirstRadio( &find_params, &radio );
    err = GetLastError();
    exp = find ? ERROR_SUCCESS : ERROR_NO_MORE_ITEMS;
    ok( err == exp, "%lu != %lu\n", err, exp );
    if (find)
    {
        BOOL ret;

        CloseHandle( radio );
        ret = BluetoothFindRadioClose( find );
        ok( ret, "BluetoothFindRadioClose failed: %lu\n", GetLastError() );
    }
}

void test_BluetoothFindNextRadio( void )
{
    HANDLE radio, dummy = (HANDLE)0xdeadbeef;
    BLUETOOTH_FIND_RADIO_PARAMS find_params;
    HBLUETOOTH_RADIO_FIND find;
    DWORD err;
    BOOL ret;

    find_params.dwSize = sizeof( find_params );
    find = BluetoothFindFirstRadio( &find_params, &radio );
    if (!find)
    {
        skip( "No Bluetooth radios found.\n" );
        return;
    }
    CloseHandle( radio );

    radio = dummy;
    SetLastError( 0xdeadbeef );
    ret = BluetoothFindNextRadio( NULL, &radio );
    ok( !ret, "Expected BluetoothFindNextRadio to return FALSE\n" );
    err = GetLastError();
    ok( err == ERROR_INVALID_HANDLE, "%lu != %d\n", err, ERROR_INVALID_HANDLE );
    ok( radio == dummy, "%p != %p\n", radio, dummy );

    for(;;)
    {
        SetLastError( 0xdeadbeef );
        ret = BluetoothFindNextRadio( find, &radio );
        if (!ret)
        {
            err = GetLastError();
            ok( err == ERROR_NO_MORE_ITEMS, "%lu != %d\n", err, ERROR_NO_MORE_ITEMS );
            break;
        }
        CloseHandle( radio );
    }
    ret = BluetoothFindRadioClose( find );
    ok( ret, "BluetoothFindRadioClose failed: %lu\n", GetLastError() );
}

void test_BluetoothFindRadioClose( void )
{
    DWORD err;

    SetLastError( 0xdeadbeef );
    ok( !BluetoothFindRadioClose( NULL ), "Expected BluetoothFindRadioClose to return FALSE\n" );
    err = GetLastError();
    ok( err == ERROR_INVALID_HANDLE, "%lu != %d\n", err, ERROR_INVALID_HANDLE );
}

void test_for_all_radios( const char *file, int line, void (*test)( HANDLE radio, void *data ), void *data )
{
    DWORD err, idx = 0;
    HANDLE radio;
    HBLUETOOTH_RADIO_FIND find;
    BLUETOOTH_FIND_RADIO_PARAMS find_params;

    find_params.dwSize = sizeof( find_params );
    find = BluetoothFindFirstRadio( &find_params, &radio );
    if (!find)
    {
        err = GetLastError();
        ok( err == ERROR_NO_MORE_ITEMS, "%lu != %d\n", err, ERROR_NO_MORE_ITEMS );
        skip_(file, line)( "No Bluetooth radios found.\n" );
        return;
    }

    for(;;)
    {
        BOOL ret;

        winetest_push_context( "radio %lu", idx++ );
        test( radio, data );
        winetest_pop_context();

        CloseHandle( radio );
        ret = BluetoothFindNextRadio( find, &radio );
        if (!ret)
        {
            err = GetLastError();
            ok( err == ERROR_NO_MORE_ITEMS, "%lu != %d\n", err, ERROR_NO_MORE_ITEMS );
            break;
        }
    }
    BluetoothFindRadioClose( find );
}

void test_BluetoothGetRadioInfo( HANDLE radio, void *data )
{
    DWORD err;
    BLUETOOTH_RADIO_INFO info = {0};

    err = BluetoothGetRadioInfo( NULL, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );
    err = BluetoothGetRadioInfo( radio, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "%lu != %d\n", err, ERROR_INVALID_PARAMETER );
    err = BluetoothGetRadioInfo( radio, &info );
    ok( err == ERROR_REVISION_MISMATCH, "%lu != %d\n", err, ERROR_REVISION_MISMATCH );

    info.dwSize = sizeof( info );
    err = BluetoothGetRadioInfo( radio, &info );
    ok( !err, "BluetoothGetRadioInfo failed: %lu\n", err );
    if (err)
        return;

    trace( "address: %x:%x:%x:%x:%x:%x\n", info.address.rgBytes[0], info.address.rgBytes[1], info.address.rgBytes[2],
           info.address.rgBytes[3], info.address.rgBytes[4], info.address.rgBytes[5] );
    trace( "name: %s\n", debugstr_w( info.szName ) );
    trace( "class: %lx\n", info.ulClassofDevice );
    trace( "LMP subversion: %x\n", info.lmpSubversion );
    trace( "manufacturer: %x\n", info.manufacturer );
}

void test_radio_BluetoothIsConnectable( HANDLE radio, void *data )
{
    BOOL *result = data;

    *result |= BluetoothIsConnectable( radio );
}

void test_BluetoothIsConnectable( void )
{
    BOOL ret;
    BOOL result = FALSE;

    ret = BluetoothIsConnectable( NULL );
    /* If ret is true, then at least one radio must be connectable. If ret returns false, then no radios are connectable. */
    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothIsConnectable, &result );

    ok( ret == result, "%d != %d\n", ret, result );
}

void test_radio_BluetoothIsDiscoverable( HANDLE radio, void *data )
{
    BOOL *result = data;

    *result |= BluetoothIsDiscoverable( radio );
}

void test_BluetoothIsDiscoverable( void )
{
    BOOL ret;
    BOOL result = FALSE;

    ret = BluetoothIsDiscoverable( NULL );
    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothIsDiscoverable, &result );

    ok( ret == result, "%d != %d\n", ret, result );
}

void test_radio_BluetoothEnableIncomingConnections( HANDLE radio, void *data )
{
    BOOL connectable;
    BOOL *result = data;

    *result |= BluetoothEnableIncomingConnections( radio, TRUE );
    ok( *result, "%d != 1\n", *result );
    if (*result)
    {
        connectable = BluetoothIsConnectable( radio );
        ok( connectable, "%d != 1\n", connectable );
    }
    else
        skip("BluetoothEnableIncomingConnections failed, skipping.\n");
}

void test_BluetoothEnableIncomingConnections( void )
{
    BOOL result = FALSE;

    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothEnableIncomingConnections, &result );
    if (result)
    {
        BOOL connectable;
        connectable = BluetoothIsConnectable( NULL );
        ok( connectable, "%d != 1\n", connectable );
    }
}

void test_radio_BluetoothEnableDiscovery( HANDLE radio, void *data )
{
    BOOL ret;

    if (BluetoothIsDiscoverable( radio ))
    {
        ret = BluetoothEnableDiscovery( radio, FALSE );
        ok( ret, "%d != 1\n", ret );
    }

    /* Enabling discovery requires incoming connections to be enabled as well, so this should result in an error. */
    BluetoothEnableIncomingConnections( radio, FALSE );
    ret = BluetoothEnableDiscovery( radio, TRUE );
    ok( !ret, "%d != 0\n", ret );

    BluetoothEnableIncomingConnections( radio, TRUE );
    ret = BluetoothEnableDiscovery( radio, TRUE );
    ok( ret, "%d != 1\n", ret );
    ret = BluetoothIsDiscoverable( radio );
    ok ( ret, "%d != 1\n", ret );
}

void test_BluetoothEnableDiscovery( void )
{
    test_for_all_radios( __FILE__, __LINE__, test_radio_BluetoothEnableDiscovery, NULL );
}

START_TEST( radio )
{
    test_BluetoothFindFirstRadio();
    test_BluetoothFindNextRadio();
    test_BluetoothFindRadioClose();

    test_for_all_radios( __FILE__, __LINE__, test_BluetoothGetRadioInfo, NULL );
    test_BluetoothIsDiscoverable();
    test_BluetoothIsConnectable();
    test_BluetoothEnableIncomingConnections();
    test_BluetoothEnableDiscovery();
}
