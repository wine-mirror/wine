/*
 * Wine Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 */

static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#include <stdio.h>
#include "windows.h"
#include "win.h"
#include "user.h"
#include "dlls.h"
#include "driver.h"
#include "stddebug.h"
/* #define DEBUG_DRIVER /* */
/* #undef  DEBUG_DRIVER /* */
#include "debug.h"

LPDRIVERITEM lpDrvItemList = NULL;

void LoadStartupDrivers()
{
	HDRVR	hDrv;
	char	str[256];
	LPSTR	ptr	= str;
	LPSTR	file = "SYSTEM.INI";
	if (GetPrivateProfileString("drivers", NULL, 
		"", str, sizeof(str), file) < 2) {
        	fprintf(stderr,
		"LoadStartupDrivers // can't find drivers section in '%s'\n", 
		file);
		return;
		}
	while(strlen(ptr) != 0) {
        	dprintf_driver(stddeb,"LoadStartupDrivers // str='%s'\n", ptr);
		hDrv = OpenDriver(ptr, "drivers", 0L);
        	dprintf_driver(stddeb,
			"LoadStartupDrivers // hDrv=%04X\n", hDrv);
		ptr += strlen(ptr) + 1;
		}
    	dprintf_driver(stddeb,"LoadStartupDrivers // end of list !\n");
}

/**************************************************************************
 *				SendDriverMessage		[USER.251]
 */
LRESULT WINAPI SendDriverMessage(HDRVR hDriver, WORD msg, LPARAM lParam1, LPARAM lParam2)
{
	dprintf_driver(stdnimp,"SendDriverMessage(%04X, %04X, %08lX, %08lX);\n",
						hDriver, msg, lParam1, lParam2);
}

/**************************************************************************
 *				OpenDriver		        [USER.252]
 */
HDRVR OpenDriver(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
	HDRVR			hDrvr;
	LPDRIVERITEM	lpnewdrv;
	LPDRIVERITEM	lpdrv = lpDrvItemList;
	char			DrvName[128];
    	dprintf_driver(stddeb,"OpenDriver('%s', '%s', %08lX);\n",
		lpDriverName, lpSectionName, lParam);
	if (lpSectionName == NULL) lpSectionName = "drivers";
	GetPrivateProfileString(lpSectionName, lpDriverName,
		"", DrvName, sizeof(DrvName), "SYSTEM.INI");
    	dprintf_driver(stddeb,"OpenDriver // DrvName='%s'\n", DrvName);
	if (strlen(DrvName) < 1) return 0;
	while (lpdrv != NULL) {
		if (lpdrv->lpNextItem == NULL) break;
		lpdrv = lpdrv->lpNextItem;
		}
	hDrvr = GlobalAlloc(GMEM_MOVEABLE, sizeof(DRIVERITEM));
	lpnewdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpnewdrv == NULL) return 0;
	lpnewdrv->dis.length = sizeof(DRIVERINFOSTRUCT);
	lpnewdrv->dis.hModule = 0;
/*	lpnewdrv->dis.hModule = LoadImage(DrvName, DLL, 0);
	if (lpnewdrv->dis.hModule == 0) {
		GlobalUnlock(hDrvr);
		GlobalFree(hDrvr);
		return 0;
		} */
	lpnewdrv->dis.hDriver = hDrvr;
	strcpy(lpnewdrv->dis.szAliasName, lpDriverName);
	lpnewdrv->count = 0;
	lpnewdrv->lpNextItem = NULL;
	if (lpDrvItemList == NULL || lpdrv == NULL) {
		lpDrvItemList = lpnewdrv;
		lpnewdrv->lpPrevItem = NULL;
		}
	else {
		lpdrv->lpNextItem = lpnewdrv;
		lpnewdrv->lpPrevItem = lpdrv;
		}
	lpnewdrv->lpDrvProc = NULL;
    	dprintf_driver(stddeb,"OpenDriver // hDrvr=%04X loaded !\n", hDrvr);
	return hDrvr;
}

/**************************************************************************
 *				CloseDriver				[USER.253]
 */
LRESULT CloseDriver(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
	LPDRIVERITEM	lpdrv;
    	dprintf_driver(stddeb,
		"CloseDriver(%04X, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr) {
		if (lpdrv->lpPrevItem)
			((LPDRIVERITEM)lpdrv->lpPrevItem)->lpNextItem = lpdrv->lpNextItem;
		if (lpdrv->lpNextItem)
			((LPDRIVERITEM)lpdrv->lpNextItem)->lpPrevItem = lpdrv->lpPrevItem;
		GlobalUnlock(hDrvr);
		GlobalFree(hDrvr);
        dprintf_driver(stddeb,"CloseDriver // hDrvr=%04X closed !\n", hDrvr);
		return TRUE;
		}
	return FALSE;
}

/**************************************************************************
 *				GetDriverModuleHandle	[USER.254]
 */
HANDLE GetDriverModuleHandle(HDRVR hDrvr)
{
	LPDRIVERITEM	lpdrv;
	HANDLE			hModule = 0;
    	dprintf_driver(stddeb,"GetDriverModuleHandle(%04X);\n", hDrvr);
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv != NULL) {
		hModule = lpdrv->dis.hModule;
		GlobalUnlock(hDrvr);
		}
	return hModule;
}

/**************************************************************************
 *				DefDriverProc			[USER.255]
 */
LRESULT DefDriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	switch(wMsg) {
		case DRV_LOAD:
			return (LRESULT)0L;
		case DRV_FREE:
			return (LRESULT)0L;
		case DRV_OPEN:
			return (LRESULT)0L;
		case DRV_CLOSE:
			return (LRESULT)0L;
		case DRV_ENABLE:
			return (LRESULT)0L;
		case DRV_DISABLE:
			return (LRESULT)0L;
		case DRV_QUERYCONFIGURE:
			return (LRESULT)0L;
		case DRV_CONFIGURE:
			MessageBox((HWND)NULL, "Driver isn't configurable !", 
									"Wine Driver", MB_OK);
			return (LRESULT)0L;
		case DRV_INSTALL:
			return (LRESULT)DRVCNF_RESTART;
		case DRV_REMOVE:
			return (LRESULT)DRVCNF_RESTART;
		default:
			return (LRESULT)0L;
		}
}

/**************************************************************************
 *				GetDriverInfo			[USER.256]
 */
BOOL GetDriverInfo(HDRVR hDrvr, LPDRIVERINFOSTRUCT lpDrvInfo)
{
	LPDRIVERITEM	lpdrv;
    	dprintf_driver(stddeb,"GetDriverInfo(%04X, %p);\n", hDrvr, lpDrvInfo);
	if (lpDrvInfo == NULL) return FALSE;
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv == NULL) return FALSE;
	memcpy(lpDrvInfo, &lpdrv->dis, sizeof(DRIVERINFOSTRUCT));
	GlobalUnlock(hDrvr);
	return TRUE;
}

/**************************************************************************
 *				GetNextDriver			[USER.257]
 */
HDRVR GetNextDriver(HDRVR hDrvr, DWORD dwFlags)
{
	LPDRIVERITEM	lpdrv;
	HDRVR			hRetDrv = 0;
    	dprintf_driver(stddeb,"GetNextDriver(%04X, %08lX);\n", hDrvr, dwFlags);
	if (hDrvr == 0) {
		if (lpDrvItemList == NULL) {
            		dprintf_driver(stddeb,
				"GetNextDriver // drivers list empty !\n");
			LoadStartupDrivers();
			if (lpDrvItemList == NULL) return 0;
			}
        	dprintf_driver(stddeb,"GetNextDriver // return first %04X !\n",
					lpDrvItemList->dis.hDriver);
		return lpDrvItemList->dis.hDriver;
		}
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv != NULL) {
		if (dwFlags & GND_REVERSE) {
			if (lpdrv->lpPrevItem) 
				hRetDrv = ((LPDRIVERITEM)lpdrv->lpPrevItem)->dis.hDriver;
			}
		else {
			if (lpdrv->lpNextItem) 
				hRetDrv = ((LPDRIVERITEM)lpdrv->lpNextItem)->dis.hDriver;
			}
		GlobalUnlock(hDrvr);
		}
    	dprintf_driver(stddeb,"GetNextDriver // return %04X !\n", hRetDrv);
	return hRetDrv;
}


