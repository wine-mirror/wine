/*
 * Internet control panel applet
 *
 * Copyright 2010 Detlef Riekenberg
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

#define NONAMELESSUNION

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <cpl.h>

#include "wine/debug.h"

#include "inetcpl.h"


WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

/*********************************************************************
 *  DllMain (inetcpl.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", hdll, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
    }
    return TRUE;
}

/*********************************************************************
 * CPlApplet (inetcpl.@)
 *
 * Control Panel entry point
 *
 * PARAMS
 *  hWnd    [I] Handle for the Control Panel Window
 *  command [I] CPL_* Command
 *  lParam1 [I] first extra Parameter
 *  lParam2 [I] second extra Parameter
 *
 * RETURNS
 *  Depends on the command
 *
 */
LONG CALLBACK CPlApplet(HWND hWnd, UINT command, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p, %u, 0x%lx, 0x%lx)\n", hWnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = 0;
            appletInfo->idName = IDS_CPL_NAME;
            appletInfo->idInfo = IDS_CPL_INFO;
            appletInfo->lData = 0;
            return TRUE;
        }

        case CPL_DBLCLK:
            FIXME("not implemented yet\n");
            break;
    }

    return FALSE;
}
