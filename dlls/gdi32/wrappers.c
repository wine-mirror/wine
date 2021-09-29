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

#include "gdi_private.h"
#include "win32u_private.h"


static void *get_user_proc( const char *name, BOOL force_load )
{
    ANSI_STRING name_str;
    FARPROC proc = NULL;
    static HMODULE user32;

    if (!user32)
    {
        UNICODE_STRING str;
        NTSTATUS status;
        RtlInitUnicodeString( &str, L"user32.dll" );
        status = force_load
            ? LdrLoadDll( NULL, 0, &str, &user32 )
            : LdrGetDllHandle( NULL, 0, &str, &user32 );
        if (status < 0) return NULL;
    }
    RtlInitAnsiString( &name_str, name );
    LdrGetProcedureAddress( user32, &name_str, 0, (void**)&proc );
    return proc;
}

static HWND WINAPI call_GetDesktopWindow(void)
{
    static HWND (WINAPI *pGetDesktopWindow)(void);
    if (!pGetDesktopWindow)
        pGetDesktopWindow = get_user_proc( "GetDesktopWindow", TRUE );
    return pGetDesktopWindow ? pGetDesktopWindow() : NULL;
}

static UINT WINAPI call_GetDpiForSystem(void)
{
    static UINT (WINAPI *pGetDpiForSystem)(void);
    if (!pGetDpiForSystem)
        pGetDpiForSystem = get_user_proc( "GetDpiForSystem", FALSE );
    return pGetDpiForSystem ? pGetDpiForSystem() : 96;
}

static BOOL WINAPI call_GetMonitorInfoW( HMONITOR monitor, MONITORINFO *info )
{
    static BOOL (WINAPI *pGetMonitorInfoW)( HMONITOR, LPMONITORINFO );
    if (!pGetMonitorInfoW)
        pGetMonitorInfoW = get_user_proc( "GetMonitorInfoW", FALSE );
    return pGetMonitorInfoW && pGetMonitorInfoW( monitor, info );
}

static INT WINAPI call_GetSystemMetrics( INT metric )
{
    static INT (WINAPI *pGetSystemMetrics)(INT);
    if (!pGetSystemMetrics)
        pGetSystemMetrics = get_user_proc( "GetSystemMetrics", FALSE );
    return pGetSystemMetrics ? pGetSystemMetrics( metric ) : 0;
}

static BOOL WINAPI call_GetWindowRect( HWND hwnd, LPRECT rect )
{
    static BOOL (WINAPI *pGetWindowRect)( HWND hwnd, LPRECT rect );
    if (!pGetWindowRect)
        pGetWindowRect = get_user_proc( "GetWindowRect", FALSE );
    return pGetWindowRect && pGetWindowRect( hwnd, rect );
}

static BOOL WINAPI call_EnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc,
                                             LPARAM lparam )
{
    static BOOL (WINAPI *pEnumDisplayMonitors)( HDC, LPRECT, MONITORENUMPROC, LPARAM );
    if (!pEnumDisplayMonitors)
        pEnumDisplayMonitors = get_user_proc( "EnumDisplayMonitors", FALSE );
    return pEnumDisplayMonitors && pEnumDisplayMonitors( hdc, rect, proc, lparam );
}

static BOOL WINAPI call_EnumDisplaySettingsW( const WCHAR *device, DWORD mode, DEVMODEW *devmode )
{
    static BOOL (WINAPI *pEnumDisplaySettingsW)(LPCWSTR, DWORD, LPDEVMODEW );
    if (!pEnumDisplaySettingsW)
        pEnumDisplaySettingsW = get_user_proc( "EnumDisplaySettingsW", FALSE );
    return pEnumDisplaySettingsW && pEnumDisplaySettingsW( device, mode, devmode );
}

static BOOL WINAPI call_RedrawWindow( HWND hwnd, const RECT *rect, HRGN rgn, UINT flags )
{
    static BOOL (WINAPI *pRedrawWindow)( HWND, const RECT*, HRGN, UINT );
    if (!pRedrawWindow)
        pRedrawWindow = get_user_proc( "RedrawWindow", FALSE );
    return pRedrawWindow && pRedrawWindow( hwnd, rect, rgn, flags );
}

static DPI_AWARENESS_CONTEXT WINAPI call_SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT ctx )
{
    static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)( DPI_AWARENESS_CONTEXT );
    if (!pSetThreadDpiAwarenessContext)
        pSetThreadDpiAwarenessContext = get_user_proc( "SetThreadDpiAwarenessContext", FALSE );
    return pSetThreadDpiAwarenessContext ? pSetThreadDpiAwarenessContext( ctx ) : 0;
}

static HWND WINAPI call_WindowFromDC( HDC hdc )
{
    static HWND (WINAPI *pWindowFromDC)( HDC );
    if (!pWindowFromDC)
        pWindowFromDC = get_user_proc( "WindowFromDC", FALSE );
    return pWindowFromDC ? pWindowFromDC( hdc ) : NULL;
}

static const struct user_callbacks callbacks =
{
    call_GetDesktopWindow,
    call_GetDpiForSystem,
    call_GetMonitorInfoW,
    call_GetSystemMetrics,
    call_GetWindowRect,
    call_EnumDisplayMonitors,
    call_EnumDisplaySettingsW,
    call_RedrawWindow,
    call_SetThreadDpiAwarenessContext,
    call_WindowFromDC,
};

const struct user_callbacks *user_callbacks = &callbacks;
