/*
 * SetupAPI device installer
 *
 * Copyright 2000 Andreas Mohr for Codeweavers
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

#include "windef.h"
#include "winbase.h"
#include "setupx16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

/***********************************************************************
 *		DiGetClassDevs (SETUPX.304)
 * Return a list of installed system devices.
 * Uses HKLM\\ENUM to list devices.
 */
RETERR16 WINAPI DiGetClassDevs16(LPLPDEVICE_INFO16 lplpdi,
                                 LPCSTR lpszClassName, HWND16 hwndParent, INT16 iFlags)
{
    LPDEVICE_INFO16 lpdi;

    FIXME("(%p, '%s', %04x, %04x), semi-stub.\n",
          lplpdi, lpszClassName, hwndParent, iFlags);
    lpdi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO16));
    lpdi->cbSize = sizeof(DEVICE_INFO16);
    *lplpdi = lpdi;
    return OK;
}
