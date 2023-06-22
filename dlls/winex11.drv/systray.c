/*
 * X11 system tray management
 *
 * Copyright (C) 2004 Mike Hearn, for CodeWeavers
 * Copyright (C) 2005 Robert Shearman
 * Copyright (C) 2008 Alexandre Julliard
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

#include "x11drv_dll.h"
#include "commctrl.h"
#include "shellapi.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

/* window procedure for foreign windows */
LRESULT WINAPI foreign_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch(msg)
    {
    case WM_WINDOWPOSCHANGED:
    {
        HWND hwnd = FindWindowW( L"Shell_TrayWnd", NULL );
        PostMessageW( hwnd, WM_USER + 0, 0, 0 );
        break;
    }
    case WM_PARENTNOTIFY:
        if (LOWORD(wparam) == WM_DESTROY)
        {
            TRACE( "%p: got parent notify destroy for win %Ix\n", hwnd, lparam );
            PostMessageW( hwnd, WM_CLOSE, 0, 0 );  /* so that we come back here once the child is gone */
        }
        return 0;
    case WM_CLOSE:
        if (GetWindow( hwnd, GW_CHILD )) return 0;  /* refuse to die if we still have children */
        break;
    }
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}
