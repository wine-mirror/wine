/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
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
#include "winuser.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "mciqtz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciqtz);

static DWORD MCIQTZ_mciClose(UINT, DWORD, LPMCI_GENERIC_PARMS);
static DWORD MCIQTZ_mciStop(UINT, DWORD, LPMCI_GENERIC_PARMS);

/*======================================================================*
 *                  	    MCI QTZ implementation			*
 *======================================================================*/

HINSTANCE MCIQTZ_hInstance = 0;

/***********************************************************************
 *		DllMain (MCIQTZ.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
	MCIQTZ_hInstance = hInstDLL;
	break;
    }
    return TRUE;
}

/**************************************************************************
 *                              MCIQTZ_drvOpen                  [internal]
 */
static DWORD MCIQTZ_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIQTZ* wma;

    TRACE("%s, %p\n", debugstr_w(str), modp);

    /* session instance */
    if (!modp)
        return 0xFFFFFFFF;

    wma = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIQTZ));
    if (!wma)
        return 0;

    wma->wDevID = modp->wDeviceID;
    mciSetDriverData(wma->wDevID, (DWORD_PTR)wma);

    return modp->wDeviceID;
}

/**************************************************************************
 *                              MCIQTZ_drvClose         [internal]
 */
static DWORD MCIQTZ_drvClose(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("%04x\n", dwDevID);

    /* finish all outstanding things */
    MCIQTZ_mciClose(dwDevID, MCI_WAIT, NULL);

    wma = (WINE_MCIQTZ*)mciGetDriverData(dwDevID);

    if (wma) {
        HeapFree(GetProcessHeap(), 0, wma);
        return 1;
    }

    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 *                              MCIQTZ_drvConfigure             [internal]
 */
static DWORD MCIQTZ_drvConfigure(DWORD dwDevID)
{
    WINE_MCIQTZ* wma;

    TRACE("%04x\n", dwDevID);

    MCIQTZ_mciStop(dwDevID, MCI_WAIT, NULL);

    wma = (WINE_MCIQTZ*)mciGetDriverData(dwDevID);

    if (wma) {
        MessageBoxA(0, "Sample QTZ Wine Driver !", "MM-Wine Driver", MB_OK);
        return 1;
    }

    return 0;
}

/**************************************************************************
 *                              MCIQTZ_mciGetOpenDev            [internal]
 */
static WINE_MCIQTZ* MCIQTZ_mciGetOpenDev(UINT wDevID)
{
    WINE_MCIQTZ* wma = (WINE_MCIQTZ*)mciGetDriverData(wDevID);

    if (!wma) {
        WARN("Invalid wDevID=%u\n", wDevID);
        return 0;
    }
    return wma;
}

/***************************************************************************
 *                              MCIQTZ_mciClose                 [internal]
 */
static DWORD MCIQTZ_mciClose(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    MCIQTZ_mciStop(wDevID, MCI_WAIT, NULL);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/***************************************************************************
 *                              MCIQTZ_mciStop                  [internal]
 */
static DWORD MCIQTZ_mciStop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIQTZ* wma;

    TRACE("(%04x, %08X, %p)\n", wDevID, dwFlags, lpParms);

    wma = MCIQTZ_mciGetOpenDev(wDevID);
    if (!wma)
        return MCIERR_INVALID_DEVICE_ID;

    return 0;
}

/*======================================================================*
 *                  	    MCI QTZ entry points			*
 *======================================================================*/

/**************************************************************************
 * 				DriverProc (MCIQTZ.@)
 */
LRESULT CALLBACK MCIQTZ_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lX, %p, %08X, %08lX, %08lX)\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
        case DRV_LOAD:                  return 1;
        case DRV_FREE:                  return 1;
        case DRV_OPEN:                  return MCIQTZ_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
        case DRV_CLOSE:                 return MCIQTZ_drvClose(dwDevID);
        case DRV_ENABLE:                return 1;
        case DRV_DISABLE:               return 1;
        case DRV_QUERYCONFIGURE:        return 1;
        case DRV_CONFIGURE:             return MCIQTZ_drvConfigure(dwDevID);
        case DRV_INSTALL:               return DRVCNF_RESTART;
        case DRV_REMOVE:                return DRVCNF_RESTART;
    }

    /* session instance */
    if (dwDevID == 0xFFFFFFFF)
        return 1;

    FIXME("Unsupported command [%u]\n", wMsg);

    return MCIERR_UNRECOGNIZED_COMMAND;
}
