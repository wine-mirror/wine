/*
 * Implementation of the Local Printmonitor User Interface
 *
 * Copyright 2007 Detlef Riekenberg
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

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"

#include "winspool.h"
#include "ddk/winsplp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(localui);

static HINSTANCE LOCALUI_hInstance;

/*****************************************************
 *   localui_AddPortUI [exported through MONITORUI]
 *
 * Display a Dialog to add a local Port
 *
 * PARAMS
 *  pName       [I] Servername or NULL (local Computer)
 *  hWnd        [I] Handle to parent Window for the Dialog-Box or NULL
 *  pMonitorName[I] Name of the Monitor, that should be used to add a Port or NULL
 *  ppPortName  [O] PTR to PTR of a buffer, that receive the Name of the new Port or NULL
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_AddPortUI(PCWSTR pName, HWND hWnd, PCWSTR pMonitorName, PWSTR *ppPortName)
{
    FIXME("(%s, %p, %s, %p) stub\n", debugstr_w(pName), hWnd, debugstr_w(pMonitorName), ppPortName);
    return TRUE;
}


/*****************************************************
 *   localui_ConfigurePortUI [exported through MONITORUI]
 *
 * Display the Configuration-Dialog for a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window for the Dialog-Box or NULL
 *  pPortName [I] Name of the Port, that should be configured
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_ConfigurePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    FIXME("(%s, %p, %s) stub\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));
    return TRUE;
}

/*****************************************************
 *   localui_DeletePortUI [exported through MONITORUI]
 *
 * Delete a specific Port
 *
 * PARAMS
 *  pName     [I] Servername or NULL (local Computer)
 *  hWnd      [I] Handle to parent Window
 *  pPortName [I] Name of the Port, that should be deleted
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 *
 */
static BOOL WINAPI localui_DeletePortUI(PCWSTR pName, HWND hWnd, PCWSTR pPortName)
{
    FIXME("(%s, %p, %s) stub\n", debugstr_w(pName), hWnd, debugstr_w(pPortName));
    return TRUE;
}

/*****************************************************
 *      InitializePrintMonitorUI  (LOCALUI.@)
 *
 * Initialize the User-Interface for the Local Ports
 *
 * RETURNS
 *  Success: Pointer to a MONITORUI Structure
 *  Failure: NULL
 *
 */

PMONITORUI WINAPI InitializePrintMonitorUI(void)
{
    static MONITORUI mymonitorui =
    {
        sizeof(MONITORUI),
        localui_AddPortUI,
        localui_ConfigurePortUI,
        localui_DeletePortUI
    };

    TRACE("=> %p\n", &mymonitorui);
    return &mymonitorui;
}

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            LOCALUI_hInstance = hinstDLL;
            break;
    }
    return TRUE;
}
