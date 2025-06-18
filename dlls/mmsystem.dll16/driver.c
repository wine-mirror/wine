/*
 * WINE Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1998 Marcus Meissner
 * Copyright 1999,2000 Eric Pouech
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
 *
 * TODO:
 *	- LoadModule count and clean up is not handled correctly (it's not a
 *	  problem as long as FreeLibrary is not working correctly)
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "mmddk.h"
#include "wine/mmsystem16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

typedef struct tagWINE_DRIVER
{
    /* as usual LPWINE_DRIVER == hDriver32 */
    HDRVR16			hDriver16;
    HMODULE16			hModule16;
    DRIVERPROC16          	lpDrvProc;
    DWORD		  	dwDriverID;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

static LPWINE_DRIVER	lpDrvItemList = NULL;


/**************************************************************************
 *			DRIVER_GetNumberOfModuleRefs		[internal]
 *
 * Returns the number of open drivers which share the same module.
 */
static	WORD	DRIVER_GetNumberOfModuleRefs(LPWINE_DRIVER lpNewDrv)
{
    LPWINE_DRIVER	lpDrv;
    WORD		count = 0;

    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem) {
	if (lpDrv->hModule16 == lpNewDrv->hModule16) {
	    count++;
	}
    }
    return count;
}

/**************************************************************************
 *				DRIVER_FindFromHDrvr16		[internal]
 *
 * From a hDrvr being 16 bits, returns the WINE internal structure.
 */
static	LPWINE_DRIVER	DRIVER_FindFromHDrvr16(HDRVR16 hDrvr)
{
    LPWINE_DRIVER	lpDrv;

    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem) {
	if (lpDrv->hDriver16 == hDrvr) {
	    break;
	}
    }
    return lpDrv;
}

/**************************************************************************
 *				DRIVER_SendMessage		[internal]
 */
static inline LRESULT DRIVER_SendMessage(LPWINE_DRIVER lpDrv, UINT16 msg,
					 LPARAM lParam1, LPARAM lParam2)
{
    WORD args[8];
    DWORD ret;

    TRACE("Before CallDriverProc proc=%p driverID=%08lx wMsg=%04x p1=%08Ix p2=%08Ix\n",
	  lpDrv->lpDrvProc, lpDrv->dwDriverID, msg, lParam1, lParam2);

    args[7] = HIWORD(lpDrv->dwDriverID);
    args[6] = LOWORD(lpDrv->dwDriverID);
    args[5] = lpDrv->hDriver16;
    args[4] = msg;
    args[3] = HIWORD(lParam1);
    args[2] = LOWORD(lParam1);
    args[1] = HIWORD(lParam2);
    args[0] = LOWORD(lParam2);
    WOWCallback16Ex( (DWORD)lpDrv->lpDrvProc, WCB16_PASCAL, sizeof(args), args, &ret );
    return ret;
}

/**************************************************************************
 *		DrvSendMessage (MMSYSTEM.1102)
 */
LRESULT WINAPI DrvSendMessage16(HDRVR16 hDriver, UINT16 msg, LPARAM lParam1, LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv;
    LRESULT 		retval = 0;

    TRACE("(%04x, %04X, %08Ix, %08Ix)\n", hDriver, msg, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr16(hDriver)) != NULL) {
	retval = DRIVER_SendMessage(lpDrv, msg, lParam1, lParam2);
    } else {
	WARN("Bad driver handle %u\n", hDriver);
    }

    TRACE("retval = %Id\n", retval);
    return retval;
}

/**************************************************************************
 *				DRIVER_RemoveFromList		[internal]
 *
 * Generates all the logic to handle driver closure / deletion
 * Removes a driver struct to the list of open drivers.
 */
static	BOOL	DRIVER_RemoveFromList(LPWINE_DRIVER lpDrv)
{
    lpDrv->dwDriverID = 0;
    if (DRIVER_GetNumberOfModuleRefs(lpDrv) == 1) {
	DRIVER_SendMessage(lpDrv, DRV_DISABLE, 0L, 0L);
	DRIVER_SendMessage(lpDrv, DRV_FREE,    0L, 0L);
    }

    if (lpDrv->lpPrevItem)
	lpDrv->lpPrevItem->lpNextItem = lpDrv->lpNextItem;
    else
	lpDrvItemList = lpDrv->lpNextItem;
    if (lpDrv->lpNextItem)
	lpDrv->lpNextItem->lpPrevItem = lpDrv->lpPrevItem;

    return TRUE;
}

/**************************************************************************
 *				DRIVER_AddToList		[internal]
 *
 * Adds a driver struct to the list of open drivers.
 * Generates all the logic to handle driver creation / open.
 */
static	BOOL	DRIVER_AddToList(LPWINE_DRIVER lpNewDrv, LPARAM lParam1, LPARAM lParam2)
{
    /* First driver to be loaded for this module, need to load correctly the module */
    if (DRIVER_GetNumberOfModuleRefs(lpNewDrv) == 0) {
	if (DRIVER_SendMessage(lpNewDrv, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) {
	    TRACE("DRV_LOAD failed on driver %p\n", lpNewDrv);
	    return FALSE;
	}
	/* returned value is not checked */
	DRIVER_SendMessage(lpNewDrv, DRV_ENABLE, 0L, 0L);
    }

    lpNewDrv->lpNextItem = NULL;
    if (lpDrvItemList == NULL) {
	lpDrvItemList = lpNewDrv;
	lpNewDrv->lpPrevItem = NULL;
    } else {
	LPWINE_DRIVER	lpDrv = lpDrvItemList;	/* find end of list */
	while (lpDrv->lpNextItem != NULL)
	    lpDrv = lpDrv->lpNextItem;

	lpDrv->lpNextItem = lpNewDrv;
	lpNewDrv->lpPrevItem = lpDrv;
    }
    /* Now just open a new instance of a driver on this module */
    lpNewDrv->dwDriverID = DRIVER_SendMessage(lpNewDrv, DRV_OPEN, lParam1, lParam2);

    if (lpNewDrv->dwDriverID == 0) {
	TRACE("DRV_OPEN failed on driver %p\n", lpNewDrv);
	DRIVER_RemoveFromList(lpNewDrv);
	return FALSE;
    }

    return TRUE;
}

/**************************************************************************
 *				DRIVER_TryOpenDriver16		[internal]
 *
 * Tries to load a 16 bit driver whose DLL's (module) name is lpFileName.
 */
static	LPWINE_DRIVER	DRIVER_TryOpenDriver16(LPSTR lpFileName, LPARAM lParam2)
{
    static	WORD	DRIVER_hDrvr16Counter /* = 0 */;
    LPWINE_DRIVER 	lpDrv = NULL;
    HMODULE16		hModule;
    DRIVERPROC16	lpProc;
    LPSTR		ptr;

    TRACE("('%s', %08Ix);\n", lpFileName, lParam2);

    if (strlen(lpFileName) < 1) return lpDrv;

    if ((ptr = strchr(lpFileName, ' ')) != NULL) {
	*ptr++ = '\0';
	while (*ptr == ' ') ptr++;
	if (*ptr == '\0') ptr = NULL;
    }

    if ((hModule = LoadLibrary16(lpFileName)) < 32) goto exit;
    if ((lpProc = (DRIVERPROC16)GetProcAddress16(hModule, "DRIVERPROC")) == NULL)
	goto exit;

    if ((lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER))) == NULL)
	goto exit;

    lpDrv->dwDriverID  = 0;
    while (DRIVER_FindFromHDrvr16(++DRIVER_hDrvr16Counter));
    lpDrv->hDriver16   = DRIVER_hDrvr16Counter;
    lpDrv->hModule16   = hModule;
    lpDrv->lpDrvProc   = lpProc;

    if (!DRIVER_AddToList(lpDrv, (LPARAM)ptr, lParam2)) goto exit;

    return lpDrv;
 exit:
    TRACE("Unable to load 16 bit module (%s): %04x\n", lpFileName, hModule);
    if (hModule >= 32)	FreeLibrary16(hModule);
    HeapFree(GetProcessHeap(), 0, lpDrv);
    return NULL;
}

/**************************************************************************
 *		DrvOpen (MMSYSTEM.1100)
 */
HDRVR16 WINAPI DrvOpen16(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv = NULL;
    char		drvName[128];

    TRACE("(%s, %s, %08Ix);\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName), lParam2);

    if (!lpDriverName || !*lpDriverName) return 0;

    if (lpSectionName == NULL) {
	strcpy(drvName, lpDriverName);

	if ((lpDrv = DRIVER_TryOpenDriver16(drvName, lParam2)))
	    goto the_end;
	/* in case hDriver is NULL, search in Drivers section */
	lpSectionName = "Drivers";
    }
    if (GetPrivateProfileStringA(lpSectionName, lpDriverName, "",
				 drvName, sizeof(drvName), "SYSTEM.INI") > 0) {
	lpDrv = DRIVER_TryOpenDriver16(drvName, lParam2);
    }
    if (!lpDrv) {
	TRACE("Failed to open driver %s from system.ini file, section %s\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName));
	return 0;
    }
 the_end:
    TRACE("=> %04x / %p\n", lpDrv->hDriver16, lpDrv);
    return lpDrv->hDriver16;
}

/**************************************************************************
 *		DrvClose16 (MMSYSTEM.1101)
 */
LRESULT WINAPI DrvClose16(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv;

    TRACE("(%04x, %08Ix, %08Ix);\n", hDrvr, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr16(hDrvr)) != NULL) {
	DRIVER_SendMessage(lpDrv, DRV_CLOSE, lParam1, lParam2);

	if (DRIVER_RemoveFromList(lpDrv)) {
	    HeapFree(GetProcessHeap(), 0, lpDrv);
	    return TRUE;
	}
    }
    WARN("Failed to close driver\n");
    return FALSE;
}

/**************************************************************************
 *		DrvGetModuleHandle (MMSYSTEM.1103)
 */
HMODULE16 WINAPI DrvGetModuleHandle16(HDRVR16 hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    HMODULE16 		hModule = 0;

    TRACE("(%04x);\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr16(hDrvr)) != NULL) {
	hModule = lpDrv->hModule16;
    }
    TRACE("=> %04x\n", hModule);
    return hModule;
}

/**************************************************************************
 *		DrvDefDriverProc (MMSYSTEM.1104)
 */
LRESULT WINAPI DrvDefDriverProc16(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg,
                                  LPARAM lParam1, LPARAM lParam2)
{
    TRACE("devID=0x%08lx hDrv=0x%04x wMsg=%04x lP1=0x%08Ix lP2=0x%08Ix\n",
	  dwDevID, hDriv, wMsg, lParam1, lParam2);

    switch(wMsg) {
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_ENABLE:
    case DRV_DISABLE:
	return (LRESULT)1L;
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_QUERYCONFIGURE:
	return (LRESULT)0L;
    case DRV_CONFIGURE:
	MessageBoxA(0, "Driver isn't configurable !", "Wine Driver", MB_OK);
	return (LRESULT)0L;
    case DRV_INSTALL:
    case DRV_REMOVE:
	return DRV_SUCCESS;
    default:
	return (LRESULT)0L;
    }
}

/**************************************************************************
 *		DriverProc (MMSYSTEM.6)
 */
LRESULT WINAPI DriverProc16(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg,
			    DWORD dwParam1, DWORD dwParam2)
{
    return DrvDefDriverProc16(dwDevID, hDrv, wMsg, dwParam1, dwParam2);
}
