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

#include "wine/unicode.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(localspl);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n",hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;           /* prefer native version */

        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinstDLL );
            break;
    }
    return TRUE;
}


/*****************************************************
 *   InitializePrintMonitor  (LOCALSPL.@)
 */

LPMONITOREX WINAPI InitializePrintMonitor(LPWSTR regroot)
{
    FIXME("(%s) stub\n", debugstr_w(regroot));
    return NULL;
}
