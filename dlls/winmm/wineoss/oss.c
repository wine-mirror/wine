/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for Open Sound System
 *
 * Copyright 	1999 Eric Pouech
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "oss.h"

#ifdef HAVE_OSS

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

#endif


/**************************************************************************
 * 				DriverProc (WINEOSS.1)
 */
LONG CALLBACK	OSS_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg,
			       DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch(wMsg) {
#ifdef HAVE_OSS
    case DRV_LOAD:		OSS_WaveInit();
#ifdef HAVE_OSS_MIDI
    				OSS_MidiInit();
#endif
				return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return OSS_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return OSS_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "OSS MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
#endif
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}


