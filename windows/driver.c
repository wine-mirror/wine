/*
 * WINE Drivers functions
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1998 Marcus Meissner
 */

#include "windows.h"
#include "heap.h"
#include "win.h"
#include "callback.h"
#include "driver.h"
#include "module.h"
#include "debug.h"
#include <string.h>

LPDRIVERITEM lpDrvItemList = NULL;
LPDRIVERITEM32A lpDrvItemList32 = NULL;

/**************************************************************************
 *	LoadStartupDrivers
 */
static void LoadStartupDrivers(void)
{
    HDRVR16 hDrv;
    char  str[256];
    LPSTR ptr;

    if (GetPrivateProfileString32A( "drivers", NULL, "", str, sizeof(str),
				 "SYSTEM.INI" ) < 2)
    {
    	ERR( driver,"Can't find drivers section in system.ini\n" );
	return;
    }

    ptr = str;
    while (lstrlen32A( ptr ) != 0)
    {
	TRACE(driver, "str='%s'\n", ptr );
	hDrv = OpenDriver16( ptr, "drivers", 0L );
	TRACE(driver, "hDrv=%04x\n", hDrv );
	ptr += lstrlen32A(ptr) + 1;
    }
    TRACE(driver, "end of list !\n" );
    return;
}

/**************************************************************************
 *				SendDriverMessage		[USER.251]
 */
LRESULT WINAPI SendDriverMessage16(HDRVR16 hDriver, UINT16 msg, LPARAM lParam1,
                                   LPARAM lParam2)
{
    LPDRIVERITEM lpdrv;
    LRESULT retval;

    TRACE(driver, "(%04x, %04X, %08lX, %08lX)\n",
		    hDriver, msg, lParam1, lParam2 );

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDriver );
    if (lpdrv == NULL || lpdrv->dis.hDriver != hDriver)
    {
	GlobalUnlock16( hDriver );
	return 0;
    }

    retval = Callbacks->CallDriverProc( lpdrv->lpDrvProc, 0L /* FIXME */,
                                        hDriver, msg, lParam1, lParam2 );

    TRACE(driver, "retval = %ld\n", retval );

    GlobalUnlock16( hDriver );
    return retval;
}

/**************************************************************************
 *				SendDriverMessage		[WINMM.19]
 */
LRESULT WINAPI SendDriverMessage32(HDRVR32 hDriver, UINT32 msg, LPARAM lParam1,
                                   LPARAM lParam2)
{
    LPDRIVERITEM32A lpdrv;
    LRESULT retval;

    TRACE(driver, "(%04x, %04X, %08lX, %08lX)\n",
		    hDriver, msg, lParam1, lParam2 );

    lpdrv = (LPDRIVERITEM32A)hDriver;
    if (!lpdrv)
	return 0;

    retval = lpdrv->driverproc(lpdrv->dis.hDriver,hDriver,msg,lParam1,lParam2);

    TRACE(driver, "retval = %ld\n", retval );

    return retval;
}

/**************************************************************************
 *				OpenDriver		        [USER.252]
 */
HDRVR16 WINAPI OpenDriver16(
	LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam
) {
    HDRVR16 hDrvr;
    LPDRIVERITEM lpdrv, lpnewdrv;
    char DrvName[128];

    TRACE(driver,"('%s', '%s', %08lX);\n",
		    lpDriverName, lpSectionName, lParam );

    if (lpSectionName == NULL) {
    	lstrcpyn32A(DrvName,lpDriverName,sizeof(DrvName));
	FIXME(driver,"sectionname NULL, assuming directfilename (%s)\n",lpDriverName);
    } else {
        GetPrivateProfileString32A( lpSectionName, lpDriverName, "", DrvName,
				     sizeof(DrvName), "SYSTEM.INI" );
    }
    TRACE(driver,"DrvName='%s'\n", DrvName );
    if (lstrlen32A(DrvName) < 1) return 0;

    lpdrv = lpDrvItemList;
    while (lpdrv)			/* XXX find it... like this? */
    {
	if (!strcasecmp( lpDriverName, lpdrv->dis.szAliasName ))
	{
	    lpdrv->count++;
	    SendDriverMessage16( lpdrv->dis.hDriver, DRV_OPEN, 0L, lParam );
	    return lpdrv->dis.hDriver;
	}
	lpdrv = lpdrv->lpNextItem;
    }

    lpdrv = lpDrvItemList;	/* find end of list */
    if (lpdrv != NULL)
      while (lpdrv->lpNextItem != NULL)
	lpdrv = lpdrv->lpNextItem;

    hDrvr = GlobalAlloc16( GMEM_MOVEABLE, sizeof(DRIVERITEM) );
    lpnewdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpnewdrv == NULL) return 0;
    lpnewdrv->dis.length = sizeof( DRIVERINFOSTRUCT16 );
    lpnewdrv->dis.hModule = LoadModule16( DrvName, (LPVOID)-1 );
    if (!lpnewdrv->dis.hModule)
    {
	GlobalUnlock16( hDrvr );
	GlobalFree16( hDrvr );
	return 0;
    }
    lpnewdrv->dis.hDriver = hDrvr;
    lstrcpy32A( lpnewdrv->dis.szAliasName, lpDriverName );
    lpnewdrv->count = 1;
    if (!(lpnewdrv->lpDrvProc = (DRIVERPROC16)WIN32_GetProcAddress16(lpnewdrv->dis.hModule, "DRIVERPROC" )))
    {
	FreeModule16( lpnewdrv->dis.hModule );
	GlobalUnlock16( hDrvr );
	GlobalFree16( hDrvr );
	return 0;
    }

    lpnewdrv->lpNextItem = NULL;
    if (lpDrvItemList == NULL)
    {
	lpDrvItemList = lpnewdrv;
	lpnewdrv->lpPrevItem = NULL;
    }
    else
    {
	lpdrv->lpNextItem = lpnewdrv;
	lpnewdrv->lpPrevItem = lpdrv;
    }

    SendDriverMessage16( hDrvr, DRV_LOAD, 0L, lParam );
    SendDriverMessage16( hDrvr, DRV_ENABLE, 0L, lParam );
    SendDriverMessage16( hDrvr, DRV_OPEN, 0L, lParam );

    TRACE(driver, "hDrvr=%04x loaded !\n", hDrvr );
    return hDrvr;
}

/**************************************************************************
 *				OpenDriver		        [WINMM.15]
 * (0,1,DRV_LOAD  ,0       ,0)
 * (0,1,DRV_ENABLE,0       ,0)
 * (0,1,DRV_OPEN  ,buf[256],0)
 */
HDRVR32 WINAPI OpenDriver32A(
	LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam
) {
    HDRVR32 hDrvr;
    LPDRIVERITEM32A lpdrv, lpnewdrv;
    char DrvName[128],*tmpbuf;

    TRACE(driver,"('%s', '%s', %08lX);\n",
		    lpDriverName,lpSectionName,lParam );

    if (lpSectionName == NULL) {
    	lstrcpyn32A(DrvName,lpDriverName,sizeof(DrvName));
	FIXME(driver,"sectionname NULL, assuming directfilename (%s)\n",lpDriverName);
    } else  {
	GetPrivateProfileString32A( lpSectionName, lpDriverName, "", DrvName,
				 sizeof(DrvName), "system.ini" );
    }
    TRACE(driver,"DrvName='%s'\n", DrvName );
    if (lstrlen32A(DrvName) < 1) return 0;

    lpdrv = lpDrvItemList32;
    while (lpdrv) {
	if (!strcasecmp( lpDriverName, lpdrv->dis.szAliasName )) {
	    lpdrv->count++;
	    lpdrv->dis.hDriver = SendDriverMessage32( lpdrv->dis.hDriver, DRV_OPEN, ""/*FIXME*/, 0L );
	    return (HDRVR32)lpdrv;
	}
	lpdrv = lpdrv->next;
    }

    hDrvr = (HDRVR32)HeapAlloc(SystemHeap,0, sizeof(DRIVERITEM32A) );
    if (!hDrvr) {
    	ERR(driver,"out of memory");
	return 0;
    }
    lpnewdrv = (DRIVERITEM32A*)hDrvr;
    lpnewdrv->dis.length = sizeof( DRIVERINFOSTRUCT32A );
    lpnewdrv->dis.hModule = LoadLibrary32A( DrvName );
    if (!lpnewdrv->dis.hModule) {
    	FIXME(driver,"could not load library %s\n",DrvName);
	HeapFree( SystemHeap,0,(LPVOID)hDrvr );
	return 0;
    }
    lstrcpy32A( lpnewdrv->dis.szAliasName, lpDriverName );
    lpnewdrv->count = 1;
    if (!(lpnewdrv->driverproc = (DRIVERPROC32)GetProcAddress32(lpnewdrv->dis.hModule, "DriverProc" )))
    {
    	FIXME(driver,"no 'DriverProc' found in %s\n",DrvName);
	FreeModule32( lpnewdrv->dis.hModule );
	HeapFree( SystemHeap,0, (LPVOID)hDrvr );
	return 0;
    }

    lpnewdrv->next = lpDrvItemList32;
    lpDrvItemList32 = lpnewdrv;

    SendDriverMessage32( hDrvr, DRV_LOAD, 0L, lParam );
    SendDriverMessage32( hDrvr, DRV_ENABLE, 0L, lParam );
    tmpbuf = HeapAlloc(SystemHeap,0,256);
    lpnewdrv->dis.hDriver=SendDriverMessage32(hDrvr,DRV_OPEN,tmpbuf,lParam);
    HeapFree(SystemHeap,0,tmpbuf);
    TRACE(driver, "hDrvr=%04x loaded !\n", hDrvr );
    return hDrvr;
}

/**************************************************************************
 *				OpenDriver		        [WINMM.15]
 */
HDRVR32 WINAPI OpenDriver32W(
	LPCWSTR lpDriverName, LPCWSTR lpSectionName, LPARAM lParam
) {
	LPSTR dn = HEAP_strdupWtoA(GetProcessHeap(),0,lpDriverName);
	LPSTR sn = HEAP_strdupWtoA(GetProcessHeap(),0,lpSectionName);
	HDRVR32	ret = OpenDriver32A(dn,sn,lParam);

	if (dn) HeapFree(GetProcessHeap(),0,dn);
	if (sn) HeapFree(GetProcessHeap(),0,sn);
	return ret;
}

/**************************************************************************
 *			CloseDriver				[USER.253]
 */
LRESULT WINAPI CloseDriver16(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    LPDRIVERITEM lpdrv;

    TRACE(driver, "(%04x, %08lX, %08lX);\n",
		    hDrvr, lParam1, lParam2 );

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr)
    {
	SendDriverMessage16( hDrvr, DRV_CLOSE, lParam1, lParam2 );
	if (--lpdrv->count == 0)
	{
	    SendDriverMessage16( hDrvr, DRV_DISABLE, lParam1, lParam2 );
	    SendDriverMessage16( hDrvr, DRV_FREE, lParam1, lParam2 );

	    if (lpdrv->lpPrevItem)
	      lpdrv->lpPrevItem->lpNextItem = lpdrv->lpNextItem;
            else
                lpDrvItemList = lpdrv->lpNextItem;
	    if (lpdrv->lpNextItem)
	      lpdrv->lpNextItem->lpPrevItem = lpdrv->lpPrevItem;

	    FreeModule16( lpdrv->dis.hModule );
	    GlobalUnlock16( hDrvr );
	    GlobalFree16( hDrvr );
	}

        TRACE(driver, "hDrvr=%04x closed !\n",
		        hDrvr );
	return TRUE;
    }
    return FALSE;
}

/**************************************************************************
 *				GetDriverModuleHandle	[USER.254]
 */
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16 hDrvr)
{
    LPDRIVERITEM lpdrv;
    HMODULE16 hModule = 0;

    TRACE(driver, "(%04x);\n", hDrvr);

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr)
    {
	hModule = lpdrv->dis.hModule;
	GlobalUnlock16( hDrvr );
    }
    return hModule;
}

/**************************************************************************
 *				GetDriverModuleHandle	[USER.254]
 */
HMODULE32 WINAPI GetDriverModuleHandle32(HDRVR32 hDrvr)
{
    LPDRIVERITEM32A lpdrv = (LPDRIVERITEM32A)hDrvr;

    TRACE(driver, "(%04x);\n", hDrvr);
    if (!lpdrv)
    	return 0;
    return lpdrv->dis.hModule;
}

/**************************************************************************
 *				DefDriverProc16			[USER.255]
 */
LRESULT WINAPI DefDriverProc16(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg, 
                               LPARAM lParam1, LPARAM lParam2)
{
    switch(wMsg)
    {
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
	MessageBox16( 0, "Driver isn't configurable !", "Wine Driver", MB_OK );
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
BOOL16 WINAPI GetDriverInfo(HDRVR16 hDrvr, LPDRIVERINFOSTRUCT16 lpDrvInfo)
{
    LPDRIVERITEM lpdrv;

    TRACE(driver, "(%04x, %p);\n", hDrvr, lpDrvInfo );

    if (lpDrvInfo == NULL) return FALSE;

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv == NULL) return FALSE;
    memcpy( lpDrvInfo, &lpdrv->dis, sizeof(DRIVERINFOSTRUCT16) );
    GlobalUnlock16( hDrvr );

    return TRUE;
}

/**************************************************************************
 *				GetNextDriver			[USER.257]
 */
HDRVR16 WINAPI GetNextDriver(HDRVR16 hDrvr, DWORD dwFlags)
{
    LPDRIVERITEM lpdrv;
    HDRVR16 hRetDrv = 0;

    TRACE(driver, "(%04x, %08lX);\n", hDrvr, dwFlags );

    if (hDrvr == 0)
    {
	if (lpDrvItemList == NULL)
	{
	    TRACE(driver, "drivers list empty !\n");
	    LoadStartupDrivers();
	    if (lpDrvItemList == NULL) return 0;
	}
	TRACE(driver,"return first %04x !\n",
		        lpDrvItemList->dis.hDriver );
	return lpDrvItemList->dis.hDriver;
    }

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv != NULL)
    {
	if (dwFlags & GND_REVERSE)
	{
	    if (lpdrv->lpPrevItem) 
	      hRetDrv = lpdrv->lpPrevItem->dis.hDriver;
	}
	else
	{
	    if (lpdrv->lpNextItem) 
	      hRetDrv = lpdrv->lpNextItem->dis.hDriver;
	}
	GlobalUnlock16( hDrvr );
    }

    TRACE(driver, "return %04x !\n", hRetDrv );
    return hRetDrv;
}
