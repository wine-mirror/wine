/*
 * Copyright 2010 Ričardas Barkauskas
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

/* How does this DLL work?
 * This DLL is used to probe and configure wireless access points using the
 * available wireless interfaces. Most functions are tied to a handle that is
 * first obtained by calling WlanOpenHandle. Usually it is followed by a call
 * to WlanEnumInterfaces and then for each interface a WlanScan call is made.
 * WlanScan starts a parallel access point discovery that delivers the ready
 * response through the callback registered by WlanRegisterNotification. After
 * that the program calls WlanGetAvailableNetworkList or WlanGetNetworkBssList.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "wlanapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlanapi);

#define WLAN_MAGIC 0x574c414e /* WLAN */

static struct wine_wlan
{
    DWORD magic, cli_version;
} handle_table[16];

static struct wine_wlan* handle_index(HANDLE handle)
{
    ULONG_PTR i = (ULONG_PTR)handle - 1;

    if (i < ARRAY_SIZE(handle_table) && handle_table[i].magic == WLAN_MAGIC)
        return &handle_table[i];

    return NULL;
}

static HANDLE handle_new(struct wine_wlan **entry)
{
    ULONG_PTR i;

    for (i = 0; i < ARRAY_SIZE(handle_table); i++)
    {
        if (handle_table[i].magic == 0)
        {
            *entry = &handle_table[i];
            return (HANDLE)(i + 1);
        }
    }

    return NULL;
}

DWORD WINAPI WlanEnumInterfaces(HANDLE handle, void *reserved, WLAN_INTERFACE_INFO_LIST **interface_list)
{
    struct wine_wlan *wlan;
    WLAN_INTERFACE_INFO_LIST *ret_list;

    FIXME("(%p, %p, %p) semi-stub\n", handle, reserved, interface_list);

    if (!handle || reserved || !interface_list)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index(handle);
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    ret_list = WlanAllocateMemory(sizeof(WLAN_INTERFACE_INFO_LIST));
    if (!ret_list)
        return ERROR_NOT_ENOUGH_MEMORY;

    memset(&ret_list->InterfaceInfo[0], 0, sizeof(WLAN_INTERFACE_INFO));
    ret_list->dwNumberOfItems = 0;
    ret_list->dwIndex = 0; /* unused in this function */
    *interface_list = ret_list;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanCloseHandle(HANDLE handle, void *reserved)
{
    struct wine_wlan *wlan;

    TRACE("(%p, %p)\n", handle, reserved);

    if (!handle || reserved)
        return ERROR_INVALID_PARAMETER;

    wlan = handle_index(handle);
    if (!wlan)
        return ERROR_INVALID_HANDLE;

    wlan->magic = 0;
    return ERROR_SUCCESS;
}

DWORD WINAPI WlanOpenHandle(DWORD client_version, void *reserved, DWORD *negotiated_version, HANDLE *handle)
{
    struct wine_wlan *wlan;
    HANDLE ret_handle;

    TRACE("(%lu, %p, %p, %p)\n", client_version, reserved, negotiated_version, handle);

    if (reserved || !negotiated_version || !handle)
        return ERROR_INVALID_PARAMETER;

    if (client_version != 1 && client_version != 2)
        return ERROR_NOT_SUPPORTED;

    ret_handle = handle_new(&wlan);
    if (!ret_handle)
        return ERROR_REMOTE_SESSION_LIMIT_EXCEEDED;

    wlan->magic = WLAN_MAGIC;
    wlan->cli_version = *negotiated_version = client_version;
    *handle = ret_handle;

    return ERROR_SUCCESS;
}

DWORD WINAPI WlanScan(HANDLE handle, const GUID *guid, const DOT11_SSID *ssid,
                      const WLAN_RAW_DATA *raw, void *reserved)
{
    FIXME("(%p, %s, %p, %p, %p) stub\n",
          handle, wine_dbgstr_guid(guid), ssid, raw, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanRegisterNotification(HANDLE handle, DWORD notify_source, BOOL ignore_dup,
                                      WLAN_NOTIFICATION_CALLBACK callback, void *context,
                                      void *reserved, DWORD *notify_prev)
{
    FIXME("(%p, %ld, %d, %p, %p, %p, %p) stub\n",
          handle, notify_source, ignore_dup, callback, context, reserved, notify_prev);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanGetAvailableNetworkList(HANDLE handle, const GUID *guid, DWORD flags,
                                         void *reserved, WLAN_AVAILABLE_NETWORK_LIST **network_list)
{
    FIXME("(%p, %s, 0x%lx, %p, %p) stub\n",
          handle, wine_dbgstr_guid(guid), flags, reserved, network_list);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanQueryInterface(HANDLE handle, const GUID *guid, WLAN_INTF_OPCODE opcode,
                    void *reserved, DWORD *data_size, void **data, WLAN_OPCODE_VALUE_TYPE *opcode_type)
{
    FIXME("(%p, %s, 0x%x, %p, %p, %p, %p) stub\n",
          handle, wine_dbgstr_guid(guid), opcode, reserved, data_size, data, opcode_type);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQueryProperty(HANDLE handle, WLAN_HOSTED_NETWORK_OPCODE opcode,
                                            DWORD *data_size, void **data,
                                            WLAN_OPCODE_VALUE_TYPE *opcode_type, void *reserved)
{
    FIXME("(%p, 0x%x, %p, %p, %p, %p) stub\n",
          handle, opcode, data_size, data, opcode_type, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQuerySecondaryKey(HANDLE handle, DWORD *key_size, unsigned char *key,
                                                BOOL *passphrase, BOOL *persistent,
                                                WLAN_HOSTED_NETWORK_REASON *error, void *reserved)
{
    FIXME("(%p, %p, %p, %p, %p, %p, %p) stub\n",
          handle, key_size, key, passphrase, persistent, error, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanHostedNetworkQueryStatus(HANDLE handle, WLAN_HOSTED_NETWORK_STATUS *status, void *reserved)
{
    FIXME("(%p, %p, %p) stub\n", handle, status, reserved);

    return ERROR_CALL_NOT_IMPLEMENTED;
}

void WINAPI WlanFreeMemory(void *ptr)
{
    TRACE("(%p)\n", ptr);

    HeapFree(GetProcessHeap(), 0, ptr);
}

void *WINAPI WlanAllocateMemory(DWORD size)
{
    void *ret;

    TRACE("(%ld)\n", size);

    if (!size)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    ret = HeapAlloc(GetProcessHeap(), 0, size);
    if (!ret)
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return ret;
}
