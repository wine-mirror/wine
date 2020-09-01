/*
 * ndis.sys
 *
 * Copyright 2014 Austin English
 * Copyright 2016 André Hentschel
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

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "netioapi.h"
#include "ntddndis.h"
#include "ddk/wdm.h"
#include "ddk/ndis.h"
#include "winreg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ndis);

static void add_key(const WCHAR *guidstrW, const MIB_IF_ROW2 *netdev)
{
    HKEY card_key;
    WCHAR keynameW[100];

    swprintf( keynameW, ARRAY_SIZE(keynameW), L"Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\%d", netdev->InterfaceIndex );
    if (RegCreateKeyExW( HKEY_LOCAL_MACHINE, keynameW, 0, NULL,
                 REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &card_key, NULL ) == ERROR_SUCCESS)
    {
        RegSetValueExW( card_key, L"Description", 0, REG_SZ, (BYTE *)netdev->Description, (lstrlenW(netdev->Description) + 1) * sizeof(WCHAR) );
        RegSetValueExW( card_key, L"ServiceName", 0, REG_SZ, (BYTE *)guidstrW, (lstrlenW(guidstrW) + 1) * sizeof(WCHAR) );
        RegCloseKey( card_key );
    }
}

static void create_network_devices(DRIVER_OBJECT *driver)
{
    MIB_IF_TABLE2 *table;
    ULONG i;

    if (GetIfTable2( &table ) != NO_ERROR)
        return;

    for (i = 0; i < table->NumEntries; i++)
    {
        GUID *guid = &table->Table[i].InterfaceGuid;
        WCHAR guidstrW[39];

        swprintf( guidstrW, ARRAY_SIZE(guidstrW), L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                  guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                  guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5],
                  guid->Data4[6], guid->Data4[7] );

        add_key( guidstrW, &table->Table[i] );
    }

    FreeMibTable( table );
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    TRACE("(%p, %s)\n", driver, debugstr_w(path->Buffer));

    create_network_devices( driver );

    return STATUS_SUCCESS;
}

NDIS_STATUS WINAPI NdisAllocateMemoryWithTag(void **address, UINT length, ULONG tag)
{
    FIXME("(%p, %u, %u): stub\n", address, length, tag);
    return NDIS_STATUS_FAILURE;
}

void WINAPI NdisAllocateSpinLock(NDIS_SPIN_LOCK *lock)
{
    FIXME("(%p): stub\n", lock);
}

void WINAPI NdisRegisterProtocol(NDIS_STATUS *status, NDIS_HANDLE *handle,
                                 NDIS_PROTOCOL_CHARACTERISTICS *prot, UINT len)
{
    FIXME("(%p, %p, %p, %u): stub\n", status, handle, prot, len);
    *status = NDIS_STATUS_FAILURE;
}

CCHAR WINAPI NdisSystemProcessorCount(void)
{
    SYSTEM_INFO si;

    TRACE("()\n");
    GetSystemInfo(&si);

    return si.dwNumberOfProcessors;
}
