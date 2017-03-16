/*
 * Copyright 2011 Stefan Leichter
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
#include "windef.h"
#include "winbase.h"
#include "dhcpcsdk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dhcpcsvc);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

void WINAPI DhcpCApiCleanup(void)
{
    FIXME(": stub\n");
}

DWORD WINAPI DhcpCApiInitialize(LPDWORD version)
{
    *version = 2; /* 98, XP, and 8 */
    FIXME(": stub\n");
    return ERROR_SUCCESS;
}

DWORD WINAPI DhcpRequestParams( DWORD flags, void *reserved, WCHAR *adaptername, DHCPCAPI_CLASSID *classid,
                                DHCPCAPI_PARAMS_ARRAY sendparams, DHCPCAPI_PARAMS_ARRAY recdparams,
                                BYTE *buffer, DWORD *size, WCHAR *requestidstr )
{
    FIXME("(%08x, %p, %s, %p, %u, %u, %p, %p, %s): stub\n", flags, reserved, debugstr_w(adaptername), classid,
            sendparams.nParams, recdparams.nParams, buffer, size, debugstr_w(requestidstr));
    return ERROR_SUCCESS;
}
