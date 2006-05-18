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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"

#ifdef HAVE_LIBAUDIOIO

extern LONG LIBAUDIOIO_WaveInit(void);

/**************************************************************************
 * 				LIBAUDIOIO_drvOpen			[internal]
 */
static LRESULT LIBAUDIOIO_drvOpen(LPSTR str)
{
    return 1;
}

/**************************************************************************
 * 				LIBAUDIOIO_drvClose			[internal]
 */
static LRESULT LIBAUDIOIO_drvClose(DWORD_PTR dwDevID)
{
    return 1;
}

#endif


/**************************************************************************
 * 				DriverProc
 */
LRESULT CALLBACK LIBAUDIOIO_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                       LPARAM dwParam1, LPARAM dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */

    switch(wMsg) {
#ifdef HAVE_LIBAUDIOIO
    case DRV_LOAD:		LIBAUDIOIO_WaveInit();
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
