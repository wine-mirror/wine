/*
 * MMSYSTEM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2003,2009 Eric Pouech
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
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wownt32.h"
#include "winnls.h"

#include "wine/winuser16.h"
#include "winemm16.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

/* ###################################################
 * #                     MCI                         #
 * ###################################################
 */

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC16 fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08x)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->lpfnYieldProc = (YIELDPROC)fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = FALSE;

    return TRUE;
}

/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC16 WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return NULL;
    }
    if (!wmd->lpfnYieldProc) {
	WARN("No proc set\n");
	return NULL;
    }
    if (wmd->bIs32) {
	WARN("Proc is 32 bit\n");
	return NULL;
    }
    if (lpdwYieldData) *lpdwYieldData = wmd->dwYieldData;
    return (YIELDPROC16)wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
    return mciGetErrorStringA(wError, lpstrBuffer, uLength);
}

/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify16(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE("(%04X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageA(HWND_32(hWndCallBack), MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(UINT16 uDeviceID)
{
    return mciGetDriverData(uDeviceID);
}

/**************************************************************************
 * 			mciSetDriverData			[MMSYSTEM.707]
 */
BOOL16 WINAPI mciSetDriverData16(UINT16 uDeviceID, DWORD data)
{
    return mciSetDriverData(uDeviceID, data);
}

/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet;

    TRACE("(%04X, %s, %08X, %08X)\n",
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2, FALSE);
    dwRet = MCI_CleanUp(dwRet, wMsg, (DWORD)MapSL(dwParam2));
    TRACE("=> %d\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciGetDeviceID		       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID16(LPCSTR lpstrName)
{
    TRACE("(\"%s\")\n", lpstrName);

    return mciGetDeviceIDA(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID16(DWORD dwElementID, LPCSTR lpstrType)
{
    FIXME("(%u, %s) stub\n", dwElementID, lpstrType);
    return 0;
}

/**************************************************************************
 * 				mciGetCreatorTask		[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask16(UINT16 uDeviceID)
{
    return HTASK_16(mciGetCreatorTask(uDeviceID));
}

/**************************************************************************
 * 				mciDriverYield			[MMSYSTEM.710]
 */
UINT16 WINAPI mciDriverYield16(UINT16 uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    UINT16		ret = 0;

    /*    TRACE("(%04x)\n", uDeviceID); */

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc || wmd->bIs32) {
	UserYield16();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}

/**************************************************************************
 * 				mciSendString			[MMSYSTEM.702]
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrRet,
			     UINT16 uRetLen, HWND16 hwndCallback)
{
    return mciSendStringA(lpstrCommand, lpstrRet, uRetLen, HWND_32(hwndCallback));
}
