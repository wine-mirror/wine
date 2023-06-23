/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

LRESULT system_tray_call( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, void *data )
{
    switch (msg)
    {
    case WINE_SYSTRAY_NOTIFY_ICON:
        return user_driver->pNotifyIcon( hwnd, wparam, data );
    case WINE_SYSTRAY_CLEANUP_ICONS:
        user_driver->pCleanupIcons( hwnd );
        return 0;

    case WINE_SYSTRAY_DOCK_INIT:
        user_driver->pSystrayDockInit( hwnd );
        return 0;
    case WINE_SYSTRAY_DOCK_INSERT:
        return user_driver->pSystrayDockInsert( hwnd, wparam, lparam, data );
    case WINE_SYSTRAY_DOCK_CLEAR:
        user_driver->pSystrayDockClear( hwnd );
        return 0;
    case WINE_SYSTRAY_DOCK_REMOVE:
        return user_driver->pSystrayDockRemove( hwnd );

    default:
        FIXME( "Unknown NtUserSystemTrayCall msg %#x\n", msg );
        break;
    }

    return -1;
}
