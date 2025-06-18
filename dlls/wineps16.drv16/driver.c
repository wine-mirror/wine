/*
 * Exported functions from the PostScript driver.
 *
 * Copyright 1998 Huw D M Davies
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
#include "wine/winuser16.h"
#include "wownt32.h"
#include "winspool.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);


/**************************************************************
 *	AdvancedSetupDialog	[WINEPS16.93]
 */
WORD WINAPI PSDRV_AdvancedSetupDialog16(HWND16 hwnd, HANDLE16 hDriver,
                                        LPDEVMODEA devin, LPDEVMODEA devout)
{
    TRACE("hwnd = %04x, hDriver = %04x devin=%p devout=%p\n", hwnd, hDriver, devin, devout);
    return IDCANCEL;
}

/***************************************************************
 *	ExtDeviceMode	[WINEPS16.90]
 */
INT16 WINAPI PSDRV_ExtDeviceMode16(HWND16 hwnd, HANDLE16 hDriver,
				   LPDEVMODEA lpdmOutput, LPSTR lpszDevice,
				   LPSTR lpszPort, LPDEVMODEA lpdmInput,
				   LPSTR lpszProfile, WORD fwMode)

{
    return DocumentPropertiesA( HWND_32(hwnd), 0, lpszDevice, lpdmOutput, lpdmInput, fwMode );
}

/**************************************************************
 *     DeviceCapabilities [WINEPS16.91]
 */
DWORD WINAPI PSDRV_DeviceCapabilities16(LPCSTR lpszDevice,
			       LPCSTR lpszPort, WORD fwCapability,
			       LPSTR lpszOutput, LPDEVMODEA lpdm)
{
    int i, ret;
    POINT *pt;
    POINT16 *pt16;

    if (fwCapability != DC_PAPERSIZE || !lpszOutput)
        return DeviceCapabilitiesA( lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );

    /* for DC_PAPERSIZE, map POINT to POINT16 */

    ret = DeviceCapabilitiesA( lpszDevice, lpszPort, DC_PAPERSIZE, NULL, lpdm );
    if (ret <= 0) return ret;

    pt16 = (POINT16 *)lpszOutput;
    pt = HeapAlloc( GetProcessHeap(), 0, ret * sizeof(POINT) );
    ret = DeviceCapabilitiesA( lpszDevice, lpszPort, DC_PAPERSIZE, (LPSTR)pt, lpdm );
    for (i = 0; i < ret; i++)
    {
        pt16[i].x = pt[i].x;
        pt16[i].y = pt[i].y;
    }
    HeapFree( GetProcessHeap(), 0, pt );
    return ret;
}

/***************************************************************
 *	DeviceMode	[WINEPS16.13]
 *
 */
void WINAPI PSDRV_DeviceMode16(HWND16 hwnd, HANDLE16 hDriver, LPSTR lpszDevice, LPSTR lpszPort)
{
    PSDRV_ExtDeviceMode16( hwnd, hDriver, NULL, lpszDevice, lpszPort, NULL, NULL, DM_PROMPT );
}
