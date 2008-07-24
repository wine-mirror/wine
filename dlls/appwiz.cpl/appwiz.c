/*
 * Add/Remove Programs applet
 * Partially based on Wine Uninstaller
 *
 * Copyright 2000 Andreas Mohr
 * Copyright 2004 Hannu Valtonen
 * Copyright 2005 Jonathan Ernst
 * Copyright 2001-2002, 2008 Owen Rudge
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
 *
 */

#include "config.h"
#include "wine/port.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winreg.h>
#include <shellapi.h>
#include <cpl.h>

#include "res.h"

WINE_DEFAULT_DEBUG_CHANNEL(appwizcpl);

static HINSTANCE hInst;

/******************************************************************************
 * Name       : DllMain
 * Description: Entry point for DLL file
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            hInst = hinstDLL;
            break;
    }
    return TRUE;
}

/******************************************************************************
 * Name       : CPlApplet
 * Description: Entry point for Control Panel applets
 * Parameters : hwndCPL - hWnd of the Control Panel
 *              message - reason for calling function
 *              lParam1 - additional parameter
 *              lParam2 - additional parameter
 * Returns    : Dependant on message
 */
LONG CALLBACK CPlApplet(HWND hwndCPL, UINT message, LPARAM lParam1, LPARAM lParam2)
{
    switch (message)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = ICO_MAIN;
            appletInfo->idName = IDS_CPL_TITLE;
            appletInfo->idInfo = IDS_CPL_DESC;
            appletInfo->lData = 0;

            break;
        }
    }

    return FALSE;
}
