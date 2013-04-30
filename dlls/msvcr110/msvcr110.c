/*
 * msvcr110 specific functions
 *
 * Copyright 2013 Stefan Leichter
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

#include "stdlib.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/*********************************************************************
 *  DllMain (MSVCR110.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
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
 *  __crtSetUnhandledExceptionFilter (MSVCR110.@)
 */
LPTOP_LEVEL_EXCEPTION_FILTER CDECL MSVCR110__crtSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER filter)
{
    return SetUnhandledExceptionFilter(filter);
}

/*********************************************************************
 *  __crtGetShowWindowMode (MSVCR110.@)
 */
int CDECL MSVCR110__crtGetShowWindowMode(void)
{
    STARTUPINFOW si;

    GetStartupInfoW(&si);
    TRACE("window=%d\n", si.wShowWindow);
    return si.wShowWindow;
}
