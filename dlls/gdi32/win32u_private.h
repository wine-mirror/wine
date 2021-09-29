/*
 * Unix call wrappers
 *
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef __WINE_WIN32U_PRIVATE
#define __WINE_WIN32U_PRIVATE

#include "wine/gdi_driver.h"
#include "winuser.h"

struct user_callbacks
{
    HWND (WINAPI *pGetDesktopWindow)(void);
    UINT (WINAPI *pGetDpiForSystem)(void);
    BOOL (WINAPI *pGetMonitorInfoW)( HMONITOR, LPMONITORINFO );
    INT (WINAPI *pGetSystemMetrics)(INT);
    BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    BOOL (WINAPI *pEnumDisplayMonitors)( HDC, LPRECT, MONITORENUMPROC, LPARAM );
    BOOL (WINAPI *pEnumDisplaySettingsW)(LPCWSTR, DWORD, LPDEVMODEW );
    BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)( DPI_AWARENESS_CONTEXT );
    HWND (WINAPI *pWindowFromDC)( HDC );
};

#endif /* __WINE_WIN32U_PRIVATE */
