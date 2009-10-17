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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

/* ###################################################
 * #                     MCI                         #
 * ###################################################
 */

#include <pshpack1.h>
#define MCI_MAX_THUNKS      32

static struct mci_thunk
{
    BYTE        popl_eax;       /* popl  %eax (return address) */
    BYTE        pushl_func;     /* pushl $pfn16 (16bit callback function) */
    YIELDPROC16 yield16;
    BYTE        pushl_eax;      /* pushl %eax */
    BYTE        jmp;            /* ljmp MCI_Yield1632 */
    DWORD       callback;
    MCIDEVICEID id;
} *MCI_Thunks;

#include <poppack.h>

static CRITICAL_SECTION mci_cs;
static CRITICAL_SECTION_DEBUG mci_critsect_debug =
{
    0, 0, &mci_cs,
    { &mci_critsect_debug.ProcessLocksList, &mci_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mmsystem_mci_cs") }
};
static CRITICAL_SECTION mci_cs = { &mci_critsect_debug, -1, 0, 0, 0, 0 };

static UINT MCI_Yield1632(DWORD pfn16, MCIDEVICEID id, DWORD yield_data)
{
    WORD args[8];

    if (!pfn16)
    {
        UserYield16();
        return 0;
    }

    /* 16 bit func, call it */
    TRACE("Function (16 bit) !\n");

    args[2] = (MCIDEVICEID16)id;
    args[1] = HIWORD(yield_data);
    args[0] = LOWORD(yield_data);
    return WOWCallback16Ex(pfn16, WCB16_PASCAL, sizeof(args), args, NULL);
}

/******************************************************************
 *		MCI_AddThunk
 *
 */
static struct mci_thunk*       MCI_AddThunk(MCIDEVICEID id, YIELDPROC16 pfn16)
{
     struct mci_thunk* thunk;

     if (!MCI_Thunks)
     {
         MCI_Thunks = VirtualAlloc(NULL, MCI_MAX_THUNKS * sizeof(*MCI_Thunks), MEM_COMMIT,
                                   PAGE_EXECUTE_READWRITE);
         if (!MCI_Thunks) return NULL;
         for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
         {
             thunk->popl_eax     = 0x58;   /* popl  %eax */
             thunk->pushl_func   = 0x68;   /* pushl $pfn16 */
             thunk->yield16      = 0;
             thunk->pushl_eax    = 0x50;   /* pushl %eax */
             thunk->jmp          = 0xe9;   /* jmp MCI_Yield1632 */
             thunk->callback     = (char *)MCI_Yield1632 - (char *)(&thunk->callback + 1);
             thunk->id           = 0;
         }
     }
     for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
     {
         if (thunk->yield16 == 0)
         {
             thunk->yield16 = pfn16;
             thunk->id      = id;
             return thunk;
         }
     }
     FIXME("Out of mci-thunks. Bump MCI_MAX_THUNKS\n");
     return NULL;
}

/******************************************************************
 *		MCI_HasThunk
 *
 */
static struct mci_thunk*    MCI_HasThunk(YIELDPROC pfn)
{
    struct mci_thunk* thunk;

    if (!MCI_Thunks) return NULL;
    for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
    {
        if ((YIELDPROC)thunk == pfn) return thunk;
    }
    return NULL;
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC16 fpYieldProc, DWORD dwYieldData)
{
    struct mci_thunk*   thunk;
    BOOL                ret;

    TRACE("(%u, %p, %08x)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(thunk = MCI_AddThunk(uDeviceID, fpYieldProc)))
        return FALSE;
    ret = mciSetYieldProc(uDeviceID, (YIELDPROC)thunk, dwYieldData);
    if (!ret) thunk->yield16 = NULL;
    return ret;
}

/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC16 WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
{
    YIELDPROC           yield;
    DWORD               data;
    struct mci_thunk*   thunk;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    yield = mciGetYieldProc(uDeviceID, &data);
    if (!yield || !(thunk = MCI_HasThunk(yield))) return NULL;

    if (lpdwYieldData) *lpdwYieldData = data;
    return thunk->yield16;
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
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD p2)
{
    DWORD		dwRet;
    BOOL                to32;
    DWORD_PTR           dwParam2 = p2;

    TRACE("(%04X, %u, %08X, %08lX)\n", wDevID, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case MCI_CLOSE:
    case MCI_OPEN:
    case MCI_SYSINFO:
    case MCI_BREAK:
    case MCI_SOUND:
        to32 = TRUE;
	break;
    default:
        /* FIXME: this is suboptimal. If MCI driver is a 16bit one, we'll be
         * doing 16=>32W, then 32W=>16 conversions.
         * We could directly call the 16bit driver if we had the information.
         */
        to32 = TRUE;
    }
    if (to32) {
        MMSYSTEM_MapType res;

	dwRet = MCIERR_INVALID_DEVICE_ID;

        switch (res = MCI_MapMsg16To32W(wMsg, dwParam1, &dwParam2)) {
        case MMSYSTEM_MAP_MSGERROR:
            TRACE("Not handled yet (%u)\n", wMsg);
            dwRet = MCIERR_DRIVER_INTERNAL;
            break;
        case MMSYSTEM_MAP_NOMEM:
            TRACE("Problem mapping msg=%u from 16 to 32a\n", wMsg);
            dwRet = MCIERR_OUT_OF_MEMORY;
            break;
        case MMSYSTEM_MAP_OK:
        case MMSYSTEM_MAP_OKMEM:
            dwRet = mciSendCommandW(wDevID, wMsg, dwParam1, dwParam2);
            if (res == MMSYSTEM_MAP_OKMEM)
                MCI_UnMapMsg16To32W(wMsg, dwParam1, dwParam2);
            break;
        }
    }
    else
    {
#if 0
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
            dwRet = SendDriverMessage(hdrv, wMsg, dwParam1, dwParam2);
	}
#endif
    }
    if (wMsg == MCI_CLOSE && dwRet == 0 && MCI_Thunks)
    {
        /* free yield thunks, if any */
        unsigned    i;
        for (i = 0; i < MCI_MAX_THUNKS; i++)
        {
            if (MCI_Thunks[i].id == wDevID)
                MCI_Thunks[i].yield16 = NULL;
        }
    }
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
    return mciGetDeviceIDFromElementIDA(dwElementID, lpstrType);
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
    return mciDriverYield(uDeviceID);
}

/**************************************************************************
 * 				mciSendString			[MMSYSTEM.702]
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrRet,
			     UINT16 uRetLen, HWND16 hwndCallback)
{
    return mciSendStringA(lpstrCommand, lpstrRet, uRetLen, HWND_32(hwndCallback));
}
