/*
 * VDHCP VxD implementation
 *
 * Copyright 2003 Juan Lang
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);


/***********************************************************************
 *           DeviceIoControl   (VDHCP.VXD.@)
 */
BOOL WINAPI VDHCP_DeviceIoControl(DWORD dwIoControlCode, LPVOID lpvInBuffer,
                                  DWORD cbInBuffer,
                                  LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                  LPDWORD lpcbBytesReturned,
                                  LPOVERLAPPED lpOverlapped)
{
    DWORD error;

    switch (dwIoControlCode)
    {
    case 1:
    {
        /* since IpReleaseAddress/IpRenewAddress are not implemented, say there
         * are no DHCP adapters
         */
        error = ERROR_FILE_NOT_FOUND;
        break;
    }

    /* FIXME: don't know what this means */
    case 5:
        if (lpcbBytesReturned)
            *lpcbBytesReturned = sizeof(DWORD);
        if (lpvOutBuffer && cbOutBuffer >= sizeof(DWORD))
        {
            *(LPDWORD)lpvOutBuffer = 0;
            error = NO_ERROR;
        }
        else
            error = ERROR_BUFFER_OVERFLOW;
        break;

    default:
        FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
                dwIoControlCode,
                lpvInBuffer,cbInBuffer,
                lpvOutBuffer,cbOutBuffer,
                lpcbBytesReturned,
                lpOverlapped);
        error = ERROR_NOT_SUPPORTED;
        break;
    }
    if (error)
        SetLastError(error);
    return error == NO_ERROR;
}
