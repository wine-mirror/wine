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

LPDRIVERITEM lpDrvItemList = NULL;

/**************************************************************************
 *				SendDriverMessage		[USER.251]
 */
LRESULT WINAPI SendDriverMessage(HDRVR hDriver, WORD msg, LPARAM lParam1, LPARAM lParam2)
{
	printf("SendDriverMessage(%04X, %04X, %08X, %08X);\n",
						hDriver, msg, lParam1, lParam2);
}

/**************************************************************************
 *				OpenDriver				[USER.252]
 */
HDRVR OpenDriver(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
	HDRVR			hDrvr;
	LPDRIVERITEM	lpnewdrv;
	LPDRIVERITEM	lpdrv = lpDrvItemList;
	char			DrvName[128];
	printf("OpenDriver('%s', '%s', %08X);\n",
		lpDriverName, lpSectionName, lParam);
	if (lpSectionName == NULL) lpSectionName = "drivers";
	GetPrivateProfileString(lpSectionName, lpDriverName,
		"", DrvName, sizeof(DrvName), "SYSTEM.INI");
	printf("OpenDriver // DrvName='%s'\n", DrvName);
	if (strlen(DrvName) < 1) return 0;
	while (lpdrv != NULL) {
		if (lpdrv->lpNextItem == NULL) break;
		lpdrv = lpdrv->lpNextItem;
		}
	hDrvr = GlobalAlloc(GMEM_MOVEABLE, sizeof(DRIVERITEM));
	lpnewdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpnewdrv == NULL) return 0;
	lpnewdrv->dis.length = sizeof(DRIVERINFOSTRUCT);
	lpnewdrv->dis.hModule = LoadImage("DrvName", DLL, 0);
	if (lpnewdrv->dis.hModule == 0) {
		GlobalUnlock(hDrvr);
		GlobalFree(hDrvr);
		return 0;
		}
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
	printf("OpenDriver // hDrvr=%04X loaded !\n", hDrvr);
	return hDrvr;
}

/**************************************************************************
 *				CloseDriver				[USER.253]
 */
LRESULT CloseDriver(HDRVR hDrvr, LPARAM lParam1, LPARAM lParam2)
{
	LPDRIVERITEM	lpdrv;
	printf("CloseDriver(%04X, %08X, %08X);\n", hDrvr, lParam1, lParam2);
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr) {
		if (lpdrv->lpPrevItem)
			((LPDRIVERITEM)lpdrv->lpPrevItem)->lpNextItem = lpdrv->lpNextItem;
		if (lpdrv->lpNextItem)
			((LPDRIVERITEM)lpdrv->lpNextItem)->lpPrevItem = lpdrv->lpPrevItem;
		GlobalUnlock(hDrvr);
		GlobalFree(hDrvr);
		printf("CloseDriver // hDrvr=%04X closed !\n", hDrvr);
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
	printf("GetDriverModuleHandle(%04X);\n", hDrvr);
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
	printf("GetDriverInfo(%04X, %08X);\n", hDrvr, lpDrvInfo);
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
	printf("GetNextDriver(%04X, %08X);\n", hDrvr, dwFlags);
	lpdrv = (LPDRIVERITEM) GlobalLock(hDrvr);
	if (lpdrv != NULL) {
		if (dwFlags & GND_REVERSE) 
			if (lpdrv->lpPrevItem) 
				hRetDrv = ((LPDRIVERITEM)lpdrv->lpPrevItem)->dis.hDriver;
		if (dwFlags & GND_FORWARD) 
			if (lpdrv->lpNextItem) 
				hRetDrv = ((LPDRIVERITEM)lpdrv->lpNextItem)->dis.hDriver;
		GlobalUnlock(hDrvr);
		}
	return hRetDrv;
	
}


