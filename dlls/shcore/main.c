/*
 * Copyright 2016 Sebastian Lackner
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
#include "shellscalingapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shcore);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;  /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI GetProcessDpiAwareness(HANDLE process, PROCESS_DPI_AWARENESS *value)
{
    if (GetProcessDpiAwarenessInternal( process, (DPI_AWARENESS *)value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI SetProcessDpiAwareness(PROCESS_DPI_AWARENESS value)
{
    if (SetProcessDpiAwarenessInternal( value )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT WINAPI GetDpiForMonitor(HMONITOR monitor, MONITOR_DPI_TYPE type, UINT *x, UINT *y)
{
    if (GetDpiForMonitorInternal( monitor, type, x, y )) return S_OK;
    return HRESULT_FROM_WIN32( GetLastError() );
}
