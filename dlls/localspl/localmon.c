/*
 * Implementation of the Local Printmonitor
 *
 * Copyright 2006 Detlef Riekenberg
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

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winver.h"
#include "winnls.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/*****************************************************
 *      InitializePrintMonitor  (LOCALSPL.@)
 *
 * Initialize the Monitor for the Local Ports
 *
 * PARAMS
 *  regroot [I] Registry-Path, where the settings are stored
 *
 * RETURNS
 *  Success: Pointer to a MONITOREX Structure
 *  Failure: NULL
 *
 * NOTES
 *  The fixed location "HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports"
 *  is used to store the Ports (IniFileMapping from "win.ini", Section "Ports").
 *  Native localspl.dll fails, when no valid Port-Entry is present.
 *
 */

LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR regroot)
{
    static MONITOREX mymonitorex =
    {
        sizeof(MONITOREX) - sizeof(DWORD)
    };

    TRACE("(%s)\n", debugstr_w(regroot));
    /* Parameter "regroot" is ignored on NT4.0 (localmon.dll) */
    if (!regroot || !regroot[0]) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    TRACE("=> %p\n", &mymonitorex);
    /* Native windows returns always the same pointer on success */
    return &mymonitorex;
}
