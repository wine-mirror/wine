/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Wine Driver for Open Sound System
 *
 * Copyright 	1999 Eric Pouech
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "driver.h"
#include "mmddk.h"
#include "oss.h"

static	struct WINE_OSS* oss = NULL;

/**************************************************************************
 * 				OSS_drvOpen			[internal]	
 */
static	DWORD	OSS_drvOpen(LPSTR str)
{
    if (oss)
	return 0;
    
    /* I know, this is ugly, but who cares... */
    oss = (struct WINE_OSS*)1;
    return 1;
}

/**************************************************************************
 * 				OSS_drvClose			[internal]	
 */
static	DWORD	OSS_drvClose(DWORD dwDevID)
{
    if (oss) {
	oss = NULL;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				OSS_DriverProc			[internal]
 */
LONG CALLBACK	OSS_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return OSS_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return OSS_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "OSS MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}


