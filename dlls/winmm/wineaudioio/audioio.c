/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for Libaudioio
 * Derived From WineOSS
 * Copyright 	1999 Eric Pouech
 * Modifications by Robert Lunnon 2002
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

#ifdef HAVE_LIBAUDIOIO

static	struct WINE_LIBAUDIOIO* audioio = NULL;

extern LONG LIBAUDIOIO_WaveInit(void);
extern BOOL LIBAUDIOIO_MidiInit(void);

/**************************************************************************
 * 				LIBAUDIOIO_drvOpen			[internal]
 */
static	DWORD	LIBAUDIOIO_drvOpen(LPSTR str)
{
    if (audioio)
	return 0;

    /* I know, this is ugly, but who cares... */
    audioio = (struct WINE_LIBAUDIOIO*)1;
    return 1;
}

/**************************************************************************
 * 				LIBAUDIOIO_drvClose			[internal]
 */
static	DWORD	LIBAUDIOIO_drvClose(DWORD dwDevID)
{
    if (audioio) {
	audioio = NULL;
	return 1;
    }
    return 0;
}

#endif


/**************************************************************************
 * 				DriverProc
 */
LONG CALLBACK	LIBAUDIOIO_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg,
			       DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch(wMsg) {
#ifdef HAVE_LIBAUDIOIO
    case DRV_LOAD:		LIBAUDIOIO_WaveInit();
#ifdef HAVE_LIBAUDIOIO_MIDI
    				LIBAUDIOIO_MidiInit();
#endif
				return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return LIBAUDIOIO_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return LIBAUDIOIO_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Libaudioio MultiMedia Driver !", "Libaudioio Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
#endif
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
