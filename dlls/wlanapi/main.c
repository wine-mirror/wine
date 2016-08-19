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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "wlanapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(wlanapi);

DWORD WINAPI WlanEnumInterfaces(HANDLE client, void *reserved, WLAN_INTERFACE_INFO_LIST **interface_list)
{
    FIXME("(%p, %p, %p) stub\n", client, reserved, interface_list);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanCloseHandle(HANDLE client_handle, VOID *reserved)
{
    FIXME("(%p, %p) stub\n", client_handle, reserved);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI WlanOpenHandle(DWORD clientVersion, PVOID reserved,
        PDWORD negotiatedVersion, PHANDLE clientHandle)
{
    FIXME("(%d, %p, %p, %p) stub\n",clientVersion, reserved, negotiatedVersion, clientHandle);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}
