/*
 * Wine Drivers functions
 *
 * Copyright 1994 Martin Ayotte
*/

#include <stdio.h>
#include "windows.h"
#include "win.h"
#include "callback.h"
#include "driver.h"
#include "module.h"
#include "stddebug.h"
#include "debug.h"

LPDRIVERITEM lpDrvItemList = NULL;

void LoadStartupDrivers(void)
{
    HDRVR16 hDrv;
    char  str[256];
    LPSTR ptr;

    if (GetPrivateProfileString( "drivers", NULL, "", str, sizeof(str),
				 "SYSTEM.INI" ) < 2)
    {
    	fprintf( stderr,
		 "LoadStartupDrivers // can't find drivers section in system.ini\n" );
	return;
    }

    ptr = str;
    while (lstrlen32A( ptr ) != 0)
    {
	dprintf_driver( stddeb, "LoadStartupDrivers // str='%s'\n", ptr );
	hDrv = OpenDriver( ptr, "drivers", 0L );
	dprintf_driver( stddeb, "LoadStartupDrivers // hDrv=%04x\n", hDrv );
	ptr += lstrlen32A(ptr) + 1;
    }
    dprintf_driver( stddeb, "LoadStartupDrivers // end of list !\n" );

    return;
}

/**************************************************************************
 *				SendDriverMessage		[USER.251]
 */
LRESULT SendDriverMessage(HDRVR16 hDriver, UINT16 msg, LPARAM lParam1,
			  LPARAM lParam2)
{
    LPDRIVERITEM lpdrv;
    LRESULT retval;

    dprintf_driver( stddeb, "SendDriverMessage(%04x, %04X, %08lX, %08lX)\n",
		    hDriver, msg, lParam1, lParam2 );

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDriver );
    if (lpdrv == NULL || lpdrv->dis.hDriver != hDriver)
    {
	GlobalUnlock16( hDriver );
	return 0;
    }

    retval = CallDriverProc( lpdrv->lpDrvProc, 0L /* FIXME */, hDriver, msg,
			     lParam1, lParam2 );

    dprintf_driver( stddeb, "SendDriverMessage // retval = %ld\n", retval );

    GlobalUnlock16( hDriver );
    return retval;
}

/**************************************************************************
 *				OpenDriver		        [USER.252]
 */
HDRVR16 OpenDriver(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
    HDRVR16 hDrvr;
    LPDRIVERITEM lpdrv, lpnewdrv;
    char DrvName[128];
    WORD ordinal;

    dprintf_driver( stddeb,"OpenDriver('%s', '%s', %08lX);\n",
		    lpDriverName, lpSectionName, lParam );

    if (lpSectionName == NULL) lpSectionName = "drivers";
    GetPrivateProfileString( lpSectionName, lpDriverName, "", DrvName,
			     sizeof(DrvName), "SYSTEM.INI" );
    dprintf_driver( stddeb,"OpenDriver // DrvName='%s'\n", DrvName );
    if (lstrlen32A(DrvName) < 1) return 0;

    lpdrv = lpDrvItemList;
    while (lpdrv)			/* XXX find it... like this? */
    {
	if (!lstrcmpi32A( lpDriverName, lpdrv->dis.szAliasName ))
	{
	    lpdrv->count++;
	    SendDriverMessage( lpdrv->dis.hDriver, DRV_OPEN, 0L, lParam );
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
    lpnewdrv->dis.hModule = LoadModule( DrvName, (LPVOID)-1 );
    if (!lpnewdrv->dis.hModule)
    {
	GlobalUnlock16( hDrvr );
	GlobalFree16( hDrvr );
	return 0;
    }
    lpnewdrv->dis.hDriver = hDrvr;
    lstrcpy32A( lpnewdrv->dis.szAliasName, lpDriverName );
    lpnewdrv->count = 1;
    ordinal = MODULE_GetOrdinal( lpnewdrv->dis.hModule, "DRIVERPROC" );
    if (!ordinal ||
        !(lpnewdrv->lpDrvProc = MODULE_GetEntryPoint( lpnewdrv->dis.hModule,
                                                      ordinal )))
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

    SendDriverMessage( hDrvr, DRV_LOAD, 0L, lParam );
    SendDriverMessage( hDrvr, DRV_ENABLE, 0L, lParam );
    SendDriverMessage( hDrvr, DRV_OPEN, 0L, lParam );

    dprintf_driver( stddeb, "OpenDriver // hDrvr=%04x loaded !\n", hDrvr );
    return hDrvr;
}

/**************************************************************************
 *				CloseDriver				[USER.253]
 */
LRESULT CloseDriver(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    LPDRIVERITEM lpdrv;

    dprintf_driver( stddeb, "CloseDriver(%04x, %08lX, %08lX);\n",
		    hDrvr, lParam1, lParam2 );

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr)
    {
	SendDriverMessage( hDrvr, DRV_CLOSE, lParam1, lParam2 );
	if (--lpdrv->count == 0)
	{
	    SendDriverMessage( hDrvr, DRV_DISABLE, lParam1, lParam2 );
	    SendDriverMessage( hDrvr, DRV_FREE, lParam1, lParam2 );

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

        dprintf_driver( stddeb, "CloseDriver // hDrvr=%04x closed !\n",
		        hDrvr );
	return TRUE;
    }
    return FALSE;
}

/**************************************************************************
 *				GetDriverModuleHandle	[USER.254]
 */
HMODULE16 GetDriverModuleHandle(HDRVR16 hDrvr)
{
    LPDRIVERITEM lpdrv;
    HMODULE16 hModule = 0;

    dprintf_driver( stddeb, "GetDriverModuleHandle(%04x);\n", hDrvr);

    lpdrv = (LPDRIVERITEM)GlobalLock16( hDrvr );
    if (lpdrv != NULL && lpdrv->dis.hDriver == hDrvr)
    {
	hModule = lpdrv->dis.hModule;
	GlobalUnlock16( hDrvr );
    }
    return hModule;
}

/**************************************************************************
 *				DefDriverProc			[USER.255]
 */
LRESULT DefDriverProc(DWORD dwDevID, HDRVR hDriv, UINT16 wMsg, 
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
	MessageBox( (HWND)NULL, "Driver isn't configurable !", 
		    "Wine Driver", MB_OK );
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
BOOL16 GetDriverInfo(HDRVR16 hDrvr, LPDRIVERINFOSTRUCT16 lpDrvInfo)
{
    LPDRIVERITEM lpdrv;

    dprintf_driver( stddeb, "GetDriverInfo(%04x, %p);\n", hDrvr, lpDrvInfo );

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
HDRVR16 GetNextDriver(HDRVR16 hDrvr, DWORD dwFlags)
{
    LPDRIVERITEM lpdrv;
    HDRVR16 hRetDrv = 0;

    dprintf_driver( stddeb, "GetNextDriver(%04x, %08lX);\n", hDrvr, dwFlags );

    if (hDrvr == 0)
    {
	if (lpDrvItemList == NULL)
	{
	    dprintf_driver(stddeb, "GetNextDriver // drivers list empty !\n");
	    LoadStartupDrivers();
	    if (lpDrvItemList == NULL) return 0;
	}
	dprintf_driver( stddeb,"GetNextDriver // return first %04x !\n",
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

    dprintf_driver( stddeb, "GetNextDriver // return %04x !\n", hRetDrv );
    return hRetDrv;
}
