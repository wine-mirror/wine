/*
 * VNETBIOS VxD implementation
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "nb30.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

/***********************************************************************
 *           DeviceIoControl   (VNETBIOS.VXD.@)
 */
BOOL WINAPI VNETBIOS_DeviceIoControl( DWORD code, LPVOID lpvInBuffer, DWORD cbInBuffer,
                                      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                      LPDWORD lpcbBytesReturned, LPOVERLAPPED lpOverlapped )
{
    switch(code)
    {
    case 256:
        Netbios(lpvInBuffer);
        return TRUE;
    default:
        FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
              code, lpvInBuffer,cbInBuffer, lpvOutBuffer,cbOutBuffer,
              lpcbBytesReturned, lpOverlapped);
        return FALSE;
    }
}
