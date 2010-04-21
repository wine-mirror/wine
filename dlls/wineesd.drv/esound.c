/*
 * Wine Driver for EsounD Sound Server
 * http://www.tux.org/~ricdude/EsounD.html
 *
 * Copyright 2004 Zhangrong Huang <hzhr@users.sourceforge.net>
 *
 * Code massively copied from Eric Pouech's OSS driver
 * and Chris Morgan aRts driver
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

/**************************************************************************
 * 				DriverProc (WINEESD.@)
 */
LRESULT CALLBACK ESD_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                LPARAM dwParam1, LPARAM dwParam2)
{
    switch(wMsg) {
#ifdef HAVE_ESD
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_QUERYCONFIGURE:
        return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "EsounD MultiMedia Driver!", "EsounD Driver", MB_OK);	return 1;
#endif
    default:
	return 0;
    }
}
