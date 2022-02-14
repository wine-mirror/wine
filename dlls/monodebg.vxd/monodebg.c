/*
 * MONODEBG VxD implementation
 *
 * Copyright 1998 Marcus Meissner
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

/* NOTE: this VxD is used by some Origin games */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);


/***********************************************************************
 *           DeviceIoControl   (MONODEBG.VXD.@)
 */
BOOL WINAPI MONODEBG_DeviceIoControl(DWORD dwIoControlCode,
                                     LPVOID lpvInBuffer, DWORD cbInBuffer,
                                     LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                     LPDWORD lpcbBytesReturned,
                                     LPOVERLAPPED lpOverlapped)
{
    switch (dwIoControlCode)
    {
    case 1:  /* version */
        *(LPDWORD)lpvOutBuffer = 0x20004; /* WC SecretOps */
        break;
    case 9:  /* debug output */
        ERR("%s\n",debugstr_a(lpvInBuffer));
        break;
    default:
        FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
              dwIoControlCode,
              lpvInBuffer,cbInBuffer,
              lpvOutBuffer,cbOutBuffer,
              lpcbBytesReturned,
              lpOverlapped );
        break;
    }
    return TRUE;
}
