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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "wine/winuser16.h"
#include "wownt32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static HMODULE wineps;
static INT (CDECL *pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
static DWORD (CDECL *pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);

static HMODULE load_wineps(void)
{
    if (!wineps)
    {
        wineps = LoadLibraryA( "wineps.drv" );
        pExtDeviceMode = (void *)GetProcAddress( wineps, "ExtDeviceMode" );
        pDeviceCapabilities = (void *)GetProcAddress( wineps, "DeviceCapabilities" );
    }
    return wineps;
}

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
    if (!load_wineps() || !pExtDeviceMode) return -1;
    return pExtDeviceMode( NULL, HWND_32(hwnd), lpdmOutput, lpszDevice,
                           lpszPort, lpdmInput, lpszProfile, fwMode );
}

/**************************************************************
 *     DeviceCapabilities [WINEPS16.91]
 */
DWORD WINAPI PSDRV_DeviceCapabilities16(LPCSTR lpszDevice,
			       LPCSTR lpszPort, WORD fwCapability,
			       LPSTR lpszOutput, LPDEVMODEA lpdm)
{
    if (!load_wineps() || !pDeviceCapabilities) return 0;
    return pDeviceCapabilities( NULL, lpszDevice, lpszPort, fwCapability, lpszOutput, lpdm );
}

/***************************************************************
 *	DeviceMode	[WINEPS16.13]
 *
 */
void WINAPI PSDRV_DeviceMode16(HWND16 hwnd, HANDLE16 hDriver, LPSTR lpszDevice, LPSTR lpszPort)
{
    PSDRV_ExtDeviceMode16( hwnd, hDriver, NULL, lpszDevice, lpszPort, NULL, NULL, DM_PROMPT );
}
