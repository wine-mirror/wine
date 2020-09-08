/*
 * Wine Driver for CoreAudio
 *
 * Copyright 2005 Emmanuel Maillard
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
#include "coreaudio.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(coreaudio);


/**************************************************************************
 * 				CoreAudio_drvLoad       [internal]	
 */
static LRESULT CoreAudio_drvLoad(void)
{
    TRACE("()\n");

    if (CoreAudio_MIDIInit() != DRV_SUCCESS)
        return DRV_FAILURE;

    return DRV_SUCCESS;
}

/**************************************************************************
 * 				CoreAudio_drvFree       [internal]	
 */
static LRESULT CoreAudio_drvFree(void)
{
    TRACE("()\n");
    CoreAudio_MIDIRelease();
    return DRV_SUCCESS;
}

/**************************************************************************
 * 				CoreAudio_drvOpen       [internal]
 */
static LRESULT CoreAudio_drvOpen(LPSTR str)
{
    TRACE("(%s)\n", str);
    return 1;
}

/**************************************************************************
 * 				CoreAudio_drvClose      [internal]
 */
static DWORD CoreAudio_drvClose(DWORD dwDevID)
{
    TRACE("(%08x)\n", dwDevID);
    return 1;
}

/**************************************************************************
 * 				DriverProc (WINECOREAUDIO.1)
 */
LRESULT CALLBACK CoreAudio_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg, 
                                 LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08lX, %p, %s (%08X), %08lX, %08lX)\n",
           dwDevID, hDriv, wMsg == DRV_LOAD ? "DRV_LOAD" :
           wMsg == DRV_FREE ? "DRV_FREE" :
           wMsg == DRV_OPEN ? "DRV_OPEN" :
           wMsg == DRV_CLOSE ? "DRV_CLOSE" :
           wMsg == DRV_ENABLE ? "DRV_ENABLE" :
           wMsg == DRV_DISABLE ? "DRV_DISABLE" :
           wMsg == DRV_QUERYCONFIGURE ? "DRV_QUERYCONFIGURE" :
           wMsg == DRV_CONFIGURE ? "DRV_CONFIGURE" :
           wMsg == DRV_INSTALL ? "DRV_INSTALL" :
           wMsg == DRV_REMOVE ? "DRV_REMOVE" : "UNKNOWN", 
           wMsg, dwParam1, dwParam2);

    switch(wMsg) {
    case DRV_LOAD:		return CoreAudio_drvLoad();
    case DRV_FREE:		return CoreAudio_drvFree();
    case DRV_OPEN:		return CoreAudio_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return CoreAudio_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "CoreAudio driver!", "CoreAudio driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
