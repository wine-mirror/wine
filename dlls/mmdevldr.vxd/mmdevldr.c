/*
 * MMDEVLDR VxD implementation
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Ulrich Weigand
 * Copyright 1998 Patrik Stridvall
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);


/***********************************************************************
 *           DeviceIoControl   (MMDEVLDR.VXD.@)
 */
BOOL WINAPI MMDEVLDR_DeviceIoControl(DWORD dwIoControlCode,
                                     LPVOID lpvInBuffer, DWORD cbInBuffer,
                                     LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                     LPDWORD lpcbBytesReturned,
                                     LPOVERLAPPED lpOverlapped)
{
    FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
          dwIoControlCode,
          lpvInBuffer,cbInBuffer,
          lpvOutBuffer,cbOutBuffer,
          lpcbBytesReturned,
          lpOverlapped );

    switch (dwIoControlCode)
    {
    case 5:
        /* Hmm. */
        *(DWORD*)lpvOutBuffer=0;
        *lpcbBytesReturned=sizeof(DWORD);
        return TRUE;
    default:
        return FALSE;
    }
}
