/*
 * WinMM joystick driver common code
 *
 * Copyright 1997 Andreas Mohr
 * Copyright 2000 Wolfgang Schwotzer
 * Copyright 2002 David Hagood
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

#include "joystick.h"


/**************************************************************************
 *                              DriverProc (JOYSTICK.@)
 */
LRESULT CALLBACK JSTCK_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg, LPARAM dwParam1, LPARAM dwParam2)
{
    switch(wMsg) {
    case DRV_LOAD:              return 1;
    case DRV_FREE:              return 1;
    case DRV_OPEN:              return driver_open((LPSTR)dwParam1, dwParam2);
    case DRV_CLOSE:             return driver_close(dwDevID);
    case DRV_ENABLE:            return 1;
    case DRV_DISABLE:           return 1;
    case DRV_QUERYCONFIGURE:    return 1;
    case DRV_CONFIGURE:         MessageBoxA(0, "JoyStick MultiMedia Driver !", "JoyStick Driver", MB_OK);       return 1;
    case DRV_INSTALL:           return DRVCNF_RESTART;
    case DRV_REMOVE:            return DRVCNF_RESTART;

    case JDD_GETNUMDEVS:        return 1;
    case JDD_GETDEVCAPS:        return driver_joyGetDevCaps(dwDevID, (LPJOYCAPSW)dwParam1, dwParam2);
    case JDD_GETPOS:            return driver_joyGetPos(dwDevID, (LPJOYINFO)dwParam1);
    case JDD_SETCALIBRATION:
    case JDD_CONFIGCHANGED:     return JOYERR_NOCANDO;
    case JDD_GETPOSEX:          return driver_joyGetPosEx(dwDevID, (LPJOYINFOEX)dwParam1);
    default:
        return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
