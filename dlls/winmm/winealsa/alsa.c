/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for ALSA
 *
 * Copyright 	2002 Eric Pouech
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
#include "alsa.h"

#ifdef HAVE_ALSA

static	struct WINE_ALSA* alsa = NULL;

/**************************************************************************
 * 				ALSA_drvOpen			[internal]
 */
static	DWORD	ALSA_drvOpen(LPSTR str)
{
    if (alsa)
	return 0;

    /* I know, this is ugly, but who cares... */
    alsa = (struct WINE_ALSA*)1;
    return 1;
}

/**************************************************************************
 * 				ALSA_drvClose			[internal]
 */
static	DWORD	ALSA_drvClose(DWORD dwDevID)
{
    if (alsa) {
	alsa = NULL;
	return 1;
    }
    return 0;
}

#endif


/**************************************************************************
 * 				DriverProc (WINEALSA.@)
 */
LONG CALLBACK	ALSA_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg,
			       DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch(wMsg) {
#ifdef HAVE_ALSA
    case DRV_LOAD:		ALSA_WaveInit();
				return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return ALSA_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return ALSA_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "ALSA MultiMedia Driver !", "ALSA Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
#endif
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
