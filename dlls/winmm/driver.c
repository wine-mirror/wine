/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * WINE Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1998 Marcus Meissner
 * Copyright 1999 Eric Pouech
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

#include <string.h>
#include "heap.h"
#include "windef.h"
#include "mmddk.h"
#include "winemm.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(driver);

static LPWINE_DRIVER	lpDrvItemList = NULL;

/**************************************************************************
 *			DRIVER_GetNumberOfModuleRefs		[internal]
 *
 * Returns the number of open drivers which share the same module.
 */
static	unsigned DRIVER_GetNumberOfModuleRefs(HMODULE hModule, WINE_DRIVER** found)
{
    LPWINE_DRIVER	lpDrv;
    unsigned		count = 0;

    if (found) *found = NULL;
    for (lpDrv = lpDrvItemList; lpDrv; lpDrv = lpDrv->lpNextItem)
    {
	if (!(lpDrv->dwFlags & WINE_GDF_16BIT) && lpDrv->d.d32.hModule == hModule)
        {
            if (found && !*found) *found = lpDrv;
	    count++;
	}
    }
    return count;
}

/**************************************************************************
 *				DRIVER_FindFromHDrvr		[internal]
 *
 * From a hDrvr being 32 bits, returns the WINE internal structure.
 */
LPWINE_DRIVER	DRIVER_FindFromHDrvr(HDRVR hDrvr)
{
    LPWINE_DRIVER	d = (LPWINE_DRIVER)hDrvr;

    if (hDrvr && HeapValidate(GetProcessHeap(), 0, d) && d->dwMagic == WINE_DI_MAGIC) {
	return d;
    }
    return NULL;
}

/**************************************************************************
 *				DRIVER_SendMessage		[internal]
 */
static LRESULT inline DRIVER_SendMessage(LPWINE_DRIVER lpDrv, UINT msg,
					LPARAM lParam1, LPARAM lParam2)
{
    if (lpDrv->dwFlags & WINE_GDF_16BIT) {
	LRESULT		ret;
	int		map = 0;
	TRACE("Before sdm16 call hDrv=%04x wMsg=%04x p1=%08lx p2=%08lx\n",
	      lpDrv->d.d16.hDriver16, msg, lParam1, lParam2);

	if ((map = DRIVER_MapMsg32To16(msg, &lParam1, &lParam2)) >= 0) {
	    ret = SendDriverMessage16(lpDrv->d.d16.hDriver16, msg, lParam1, lParam2);
	    if (map == 1)
		DRIVER_UnMapMsg32To16(msg, lParam1, lParam2);
	} else {
	    ret = 0;
	}
	return ret;
    }
    TRACE("Before func32 call proc=%p driverID=%08lx hDrv=%08x wMsg=%04x p1=%08lx p2=%08lx\n",
	  lpDrv->d.d32.lpDrvProc, lpDrv->d.d32.dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);
    return lpDrv->d.d32.lpDrvProc(lpDrv->d.d32.dwDriverID, (HDRVR)lpDrv, msg, lParam1, lParam2);
}

/**************************************************************************
 *				SendDriverMessage		[WINMM.@]
 *				DrvSendMessage			[WINMM.@]
 */
LRESULT WINAPI SendDriverMessage(HDRVR hDriver, UINT msg, LPARAM lParam1,
				 LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv;
    LRESULT 		retval = 0;

    TRACE("(%04x, %04X, %08lX, %08lX)\n", hDriver, msg, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDriver)) != NULL) {
	retval = DRIVER_SendMessage(lpDrv, msg, lParam1, lParam2);
    } else {
	WARN("Bad driver handle %u\n", hDriver);
    }
    TRACE("retval = %ld\n", retval);

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
    if (!(lpDrv->dwFlags & WINE_GDF_16BIT)) {
        /* last of this driver in list ? */
	if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, NULL) == 1) {
	    DRIVER_SendMessage(lpDrv, DRV_DISABLE, 0L, 0L);
	    DRIVER_SendMessage(lpDrv, DRV_FREE,    0L, 0L);
	}
    }

    if (lpDrv->lpPrevItem)
	lpDrv->lpPrevItem->lpNextItem = lpDrv->lpNextItem;
    else
	lpDrvItemList = lpDrv->lpNextItem;
    if (lpDrv->lpNextItem)
	lpDrv->lpNextItem->lpPrevItem = lpDrv->lpPrevItem;
    /* trash magic number */
    lpDrv->dwMagic ^= 0xa5a5a5a5;

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
    lpNewDrv->dwMagic = WINE_DI_MAGIC;
    /* First driver to be loaded for this module, need to load correctly the module */
    if (!(lpNewDrv->dwFlags & WINE_GDF_16BIT)) {
        /* first of this driver in list ? */
	if (DRIVER_GetNumberOfModuleRefs(lpNewDrv->d.d32.hModule, NULL) == 0) {
	    if (DRIVER_SendMessage(lpNewDrv, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) {
		TRACE("DRV_LOAD failed on driver 0x%08lx\n", (DWORD)lpNewDrv);
		return FALSE;
	    }
	    /* returned value is not checked */
	    DRIVER_SendMessage(lpNewDrv, DRV_ENABLE, 0L, 0L);
	}
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

    if (!(lpNewDrv->dwFlags & WINE_GDF_16BIT)) {
	/* Now just open a new instance of a driver on this module */
	lpNewDrv->d.d32.dwDriverID = DRIVER_SendMessage(lpNewDrv, DRV_OPEN, lParam1, lParam2);

	if (lpNewDrv->d.d32.dwDriverID == 0) {
	    TRACE("DRV_OPEN failed on driver 0x%08lx\n", (DWORD)lpNewDrv);
	    DRIVER_RemoveFromList(lpNewDrv);
	    return FALSE;
	}
    }
    return TRUE;
}

/**************************************************************************
 *				DRIVER_GetLibName		[internal]
 *
 */
BOOL	DRIVER_GetLibName(LPCSTR keyName, LPCSTR sectName, LPSTR buf, int sz)
{
    /* should also do some registry diving */
    return GetPrivateProfileStringA(sectName, keyName, "", buf, sz, "SYSTEM.INI");
}

/**************************************************************************
 *				DRIVER_TryOpenDriver32		[internal]
 *
 * Tries to load a 32 bit driver whose DLL's (module) name is fn
 */
LPWINE_DRIVER	DRIVER_TryOpenDriver32(LPCSTR fn, LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    HMODULE		hModule = 0;
    LPSTR		ptr;
    LPCSTR		cause = 0;

    TRACE("(%s, %08lX);\n", debugstr_a(fn), lParam2);

    if ((ptr = strchr(fn, ' ')) != NULL) {
	*ptr++ = '\0';
	while (*ptr == ' ') ptr++;
	if (*ptr == '\0') ptr = NULL;
    }

    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));
    if (lpDrv == NULL) {cause = "OOM"; goto exit;}

    if ((hModule = LoadLibraryA(fn)) == 0) {cause = "Not a 32 bit lib"; goto exit;}

    lpDrv->d.d32.lpDrvProc = (DRIVERPROC)GetProcAddress(hModule, "DriverProc");
    if (lpDrv->d.d32.lpDrvProc == NULL) {cause = "no DriverProc"; goto exit;}

    lpDrv->dwFlags          = 0;
    lpDrv->d.d32.hModule    = hModule;
    lpDrv->d.d32.dwDriverID = 0;

    /* Win32 installable drivers must support a two phase opening scheme:
     * + first open with NULL as lParam2 (session instance),
     * + then do a second open with the real non null lParam2)
     */
    if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, NULL) == 0 && lParam2)
    {
        LPWINE_DRIVER   ret;

        if (!DRIVER_AddToList(lpDrv, (LPARAM)ptr, 0L))
        {
            cause = "load0 failed";
            goto exit;
        }
        ret = DRIVER_TryOpenDriver32(fn, lParam2);
        if (!ret)
        {
            CloseDriver((HDRVR)lpDrv, 0L, 0L);
            cause = "load1 failed";
            goto exit;
        }
        return ret;
    }

    if (!DRIVER_AddToList(lpDrv, (LPARAM)ptr, lParam2))
    {cause = "load failed"; goto exit;}

    TRACE("=> %p\n", lpDrv);
    return lpDrv;
 exit:
    FreeLibrary(hModule);
    HeapFree(GetProcessHeap(), 0, lpDrv);
    TRACE("Unable to load 32 bit module %s: %s\n", debugstr_a(fn), cause);
    return NULL;
}

/**************************************************************************
 *				DRIVER_TryOpenDriver16		[internal]
 *
 * Tries to load a 16 bit driver whose DLL's (module) name is lpFileName.
 */
static	LPWINE_DRIVER	DRIVER_TryOpenDriver16(LPCSTR fn, LPCSTR sn, LPARAM lParam2)
{
    LPWINE_DRIVER 	lpDrv = NULL;
    LPCSTR		cause = 0;

    TRACE("(%s, %08lX);\n", debugstr_a(sn), lParam2);

    lpDrv = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_DRIVER));
    if (lpDrv == NULL) {cause = "OOM"; goto exit;}

    /* FIXME: shall we do some black magic here on sn ?
     *	drivers32 => drivers
     *	mci32 => mci
     * ...
     */
    lpDrv->d.d16.hDriver16 = OpenDriver16(fn, sn, lParam2);
    if (lpDrv->d.d16.hDriver16 == 0) {cause = "Not a 16 bit driver"; goto exit;}
    lpDrv->dwFlags = WINE_GDF_16BIT;

    if (!DRIVER_AddToList(lpDrv, 0, lParam2)) {cause = "load failed"; goto exit;}

    TRACE("=> %p\n", lpDrv);
    return lpDrv;
 exit:
    HeapFree(GetProcessHeap(), 0, lpDrv);
    TRACE("Unable to load 32 bit module %s: %s\n", debugstr_a(fn), cause);
    return NULL;
}

/**************************************************************************
 *				OpenDriverA		        [WINMM.@]
 *				DrvOpenA			[WINMM.@]
 * (0,1,DRV_LOAD  ,0       ,0)
 * (0,1,DRV_ENABLE,0       ,0)
 * (0,1,DRV_OPEN  ,buf[256],0)
 */
HDRVR WINAPI OpenDriverA(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv = NULL;
    char 		libName[128];
    LPCSTR		lsn = lpSectionName;

    TRACE("(%s, %s, 0x%08lx);\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName), lParam2);

    if (lsn == NULL) {
	lstrcpynA(libName, lpDriverName, sizeof(libName));

	if ((lpDrv = DRIVER_TryOpenDriver32(libName, lParam2)))
	    goto the_end;
	lsn = "Drivers32";
    }
    if (DRIVER_GetLibName(lpDriverName, lsn, libName, sizeof(libName)) &&
	(lpDrv = DRIVER_TryOpenDriver32(libName, lParam2)))
	goto the_end;

    if (!(lpDrv = DRIVER_TryOpenDriver16(lpDriverName, lpSectionName, lParam2)))
	TRACE("Failed to open driver %s from system.ini file, section %s\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName));
 the_end:
    if (lpDrv)	TRACE("=> %08lx\n", (DWORD)lpDrv);
    return (HDRVR)lpDrv;
}

/**************************************************************************
 *				OpenDriver 		        [WINMM.@]
 *				DrvOpen				[WINMM.@]
 */
HDRVR WINAPI OpenDriverW(LPCWSTR lpDriverName, LPCWSTR lpSectionName, LPARAM lParam)
{
    LPSTR 		dn = HEAP_strdupWtoA(GetProcessHeap(), 0, lpDriverName);
    LPSTR 		sn = HEAP_strdupWtoA(GetProcessHeap(), 0, lpSectionName);
    HDRVR		ret = OpenDriverA(dn, sn, lParam);

    if (dn) HeapFree(GetProcessHeap(), 0, dn);
    if (sn) HeapFree(GetProcessHeap(), 0, sn);
    return ret;
}

/**************************************************************************
 *			CloseDriver				[WINMM.@]
 *			DrvClose				[WINMM.@]
 */
LRESULT WINAPI CloseDriver(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    LPWINE_DRIVER	lpDrv;

    TRACE("(%04x, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL)
    {
	if (lpDrv->dwFlags & WINE_GDF_16BIT)
	    CloseDriver16(lpDrv->d.d16.hDriver16, lParam1, lParam2);
	else
        {
	    DRIVER_SendMessage(lpDrv, DRV_CLOSE, lParam1, lParam2);
            lpDrv->d.d32.dwDriverID = 0;
        }
	if (DRIVER_RemoveFromList(lpDrv)) {
            if (!(lpDrv->dwFlags & WINE_GDF_16BIT))
            {
                LPWINE_DRIVER       lpDrv0;

                /* if driver has an opened session instance, we have to close it too */
                if (DRIVER_GetNumberOfModuleRefs(lpDrv->d.d32.hModule, &lpDrv0) == 1)
                {
                    DRIVER_SendMessage(lpDrv0, DRV_CLOSE, 0L, 0L);
                    lpDrv0->d.d32.dwDriverID = 0;
                    DRIVER_RemoveFromList(lpDrv0);
                    FreeLibrary(lpDrv->d.d32.hModule);
                    HeapFree(GetProcessHeap(), 0, lpDrv0);
                }
                FreeLibrary(lpDrv->d.d32.hModule);
            }
            HeapFree(GetProcessHeap(), 0, lpDrv);
            return TRUE;
        }
    }
    WARN("Failed to close driver\n");
    return FALSE;
}

/**************************************************************************
 *				GetDriverFlags		[WINMM.@]
 * [in] hDrvr handle to the driver
 *
 * Returns:
 *	0x00000000 if hDrvr is an invalid handle
 *	0x80000000 if hDrvr is a valid 32 bit driver
 *	0x90000000 if hDrvr is a valid 16 bit driver
 *
 * native WINMM doesn't return those flags
 *	0x80000000 for a valid 32 bit driver and that's it
 *	(I may have mixed up the two flags :-(
 */
DWORD	WINAPI GetDriverFlags(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    DWORD		ret = 0;

    TRACE("(%04x)\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL) {
	ret = WINE_GDF_EXIST | lpDrv->dwFlags;
    }
    return ret;
}

/**************************************************************************
 *				GetDriverModuleHandle	[WINMM.@]
 *				DrvGetModuleHandle	[WINMM.@]
 */
HMODULE WINAPI GetDriverModuleHandle(HDRVR hDrvr)
{
    LPWINE_DRIVER 	lpDrv;
    HMODULE		hModule = 0;

    TRACE("(%04x);\n", hDrvr);

    if ((lpDrv = DRIVER_FindFromHDrvr(hDrvr)) != NULL) {
	if (!(lpDrv->dwFlags & WINE_GDF_16BIT))
	    hModule = lpDrv->d.d32.hModule;
    }
    TRACE("=> %04x\n", hModule);
    return hModule;
}

/**************************************************************************
 * 				DefDriverProc			  [WINMM.@]
 * 				DrvDefDriverProc		  [WINMM.@]
 */
LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hDrv,
			     UINT Msg, LPARAM lParam1, LPARAM lParam2)
{
    switch (Msg) {
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_ENABLE:
    case DRV_DISABLE:
        return 1;
    case DRV_INSTALL:
    case DRV_REMOVE:
        return DRV_SUCCESS;
    default:
        return 0;
    }
}
