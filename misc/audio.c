/*
 * Sample Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#include "stdio.h"
#include "windows.h"
#include "win.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"


/**************************************************************************
* 				DriverProc			[sample driver]
*/
LRESULT DriverProc(DWORD dwDevID, HDRVR hDriv, WORD wMsg, 
						DWORD dwParam1, DWORD dwParam2)
{
	switch(wMsg) {
		case DRV_LOAD:
			return (LRESULT)1L;
		case DRV_FREE:
			return (LRESULT)1L;
		case DRV_OPEN:
			return (LRESULT)1L;
		case DRV_CLOSE:
			return (LRESULT)1L;
		case DRV_ENABLE:
			return (LRESULT)1L;
		case DRV_DISABLE:
			return (LRESULT)1L;
		case DRV_QUERYCONFIGURE:
			return (LRESULT)1L;
		case DRV_CONFIGURE:
			MessageBox((HWND)NULL, "Sample MultiMedia Linux Driver !", 
								"MMLinux Driver", MB_OK);
			return (LRESULT)1L;
		case DRV_INSTALL:
			return (LRESULT)DRVCNF_RESTART;
		case DRV_REMOVE:
			return (LRESULT)DRVCNF_RESTART;
		default:
			return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
		}
}

/**************************************************************************
* 				wodMessage			[sample driver]
*/
DWORD wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
}

/**************************************************************************
* 				widMessage			[sample driver]
*/
DWORD widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
}

/**************************************************************************
* 				auxMessage			[sample driver]
*/
DWORD auxMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
}

/**************************************************************************
* 				midMessage			[sample driver]
*/
DWORD midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
}

/**************************************************************************
* 				modMessage			[sample driver]
*/
DWORD modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
}


/*
BOOL DriverCallback(DWORD dwCallBack, UINT uFlags, HANDLE hDev, 
		WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
*/


