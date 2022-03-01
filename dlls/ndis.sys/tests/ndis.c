/*
 * Unit tests for ndis ioctls
 *
 * Copyright (c) 2020 Isabella Bosia
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winioctl.h>
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string.h>
#include <winternl.h>
#include <winnt.h>
#include "wine/test.h"

static void test_device(const WCHAR *service_name, const MIB_IF_ROW2 *row)
{
    DWORD size;
    int ret;
    NDIS_MEDIUM medium;
    UCHAR addr[IF_MAX_PHYS_ADDRESS_LENGTH];
    HANDLE netdev = INVALID_HANDLE_VALUE;
    int oid;
    UNICODE_STRING str;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;
    WCHAR meoW[47];

    swprintf( meoW, ARRAY_SIZE(meoW), L"\\Device\\%s", service_name );
    RtlInitUnicodeString( &str, meoW );
    InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
    status = NtOpenFile( &netdev, GENERIC_READ, &attr, &iosb, FILE_SHARE_READ, 0 );

    if (status != STATUS_SUCCESS)
    {
        skip( "Couldn't open the device (status = %ld)\n", status );
        return;
    }

    oid = 0xdeadbeef;
    iosb.Status = 0xdeadbeef;
    iosb.Information = 0xdeadbeef;
    status = NtDeviceIoControlFile( netdev, NULL, NULL, NULL, &iosb,
            IOCTL_NDIS_QUERY_GLOBAL_STATS, &oid, sizeof(oid), &medium, sizeof(medium) );
    ok(status == STATUS_INVALID_PARAMETER, "got status %#lx\n", status);
    ok(iosb.Status == 0xdeadbeef, "got %#lx\n", iosb.Status);
    ok(iosb.Information == 0xdeadbeef, "got size %#Ix\n", iosb.Information);

    oid = OID_GEN_MEDIA_SUPPORTED;
    ret = DeviceIoControl( netdev, IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &oid, sizeof(oid), &medium, sizeof(medium), &size, NULL );
    ok( ret, "OID_GEN_MEDIA_SUPPORTED failed (ret = %d)\n", ret );
    ok( medium == row->MediaType, "Wrong media type\n" );

    oid = OID_GEN_MEDIA_IN_USE;
    ret = DeviceIoControl( netdev, IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &oid, sizeof(oid), &medium, sizeof(medium), &size, NULL );
    ok( ret, "OID_GEN_MEDIA_IN_USE failed (ret = %d)\n", ret );
    ok( medium == row->MediaType, "Wrong media type\n" );

    oid = OID_802_3_PERMANENT_ADDRESS;
    ret = DeviceIoControl( netdev, IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &oid, sizeof(oid), addr, sizeof(addr), &size, NULL );
    ok( ret, "OID_802_3_PERMANENT_ADDRESS failed (ret = %d)\n", ret );
    ok( row->PhysicalAddressLength == size && !memcmp( row->PermanentPhysicalAddress, addr, size ),
            "Wrong permanent address\n" );

    oid = OID_802_3_CURRENT_ADDRESS;
    ret = DeviceIoControl( netdev, IOCTL_NDIS_QUERY_GLOBAL_STATS,
            &oid, sizeof(oid), addr, sizeof(addr), &size, NULL );
    ok( ret, "OID_802_3_CURRENT_ADDRESS failed (ret = %d)\n", ret );
    ok( row->PhysicalAddressLength == size && !memcmp( row->PhysicalAddress, addr, size ),
            "Wrong current address\n" );
}

static void test_ndis_ioctl(void)
{
    HKEY nics, sub_key;
    LSTATUS ret;
    WCHAR card[16];
    WCHAR description[100], service_name[100];
    DWORD size, type, i = 0;

    ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards", 0, KEY_READ, &nics );
    ok( ret == ERROR_SUCCESS, "NetworkCards key missing\n" );

    while (1)
    {
        MIB_IF_ROW2 row = {{0}};
        GUID guid;

        size = sizeof(card);
        ret = RegEnumKeyExW( nics, i, card, &size, NULL, NULL, NULL, NULL );
        if (ret != ERROR_SUCCESS)
            break;
        i++;

        ret = RegOpenKeyExW( nics, card, 0, KEY_READ, &sub_key );
        ok( ret == ERROR_SUCCESS, "Could not open network card subkey\n" );

        size = sizeof(service_name);
        ret = RegQueryValueExW( sub_key, L"ServiceName", NULL, &type, (BYTE *)service_name, &size );
        ok( ret == ERROR_SUCCESS && type == REG_SZ, "Wrong ServiceName\n" );

        CLSIDFromString( service_name, (LPCLSID)&guid );
        ConvertInterfaceGuidToLuid( &guid, &row.InterfaceLuid );

        ret = GetIfEntry2(&row);
        ok( ret == NO_ERROR, "GetIfEntry2 failed\n" );

        ok( IsEqualGUID( &guid, &row.InterfaceGuid ), "Wrong ServiceName\n" );

        size = sizeof(description);
        ret = RegQueryValueExW( sub_key, L"Description", NULL, &type, (BYTE *)description, &size );
        ok( ret == ERROR_SUCCESS && type == REG_SZ, "Wrong Description\n" );

        trace( "testing device <%s>\n", wine_dbgstr_w(description) );
        test_device( service_name, &row );

        RegCloseKey( sub_key );
    }

    if (i == 0)
        skip( "Network card subkeys missing\n" );

    RegCloseKey( nics );
}

START_TEST(ndis)
{
    test_ndis_ioctl();
}
