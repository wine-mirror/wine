/*
 * winex11.drv entry points
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);


HMODULE x11drv_module = 0;

/**************************************************************************
 *		wait_clipboard_mutex
 *
 * Make sure that there's only one clipboard thread per window station.
 */
static BOOL wait_clipboard_mutex(void)
{
    static const WCHAR prefix[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_'};
    WCHAR buffer[MAX_PATH + ARRAY_SIZE( prefix )];
    HANDLE mutex;

    memcpy( buffer, prefix, sizeof(prefix) );
    if (!GetUserObjectInformationW( GetProcessWindowStation(), UOI_NAME,
                                    buffer + ARRAY_SIZE( prefix ),
                                    sizeof(buffer) - sizeof(prefix), NULL ))
    {
        ERR( "failed to get winstation name\n" );
        return FALSE;
    }
    mutex = CreateMutexW( NULL, TRUE, buffer );
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE( "waiting for mutex %s\n", debugstr_w( buffer ));
        WaitForSingleObject( mutex, INFINITE );
    }
    return TRUE;
}


/**************************************************************************
 *		clipboard_wndproc
 *
 * Window procedure for the clipboard manager.
 */
static LRESULT CALLBACK clipboard_wndproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    struct clipboard_message_params params;

    switch (msg)
    {
    case WM_NCCREATE:
    case WM_CLIPBOARDUPDATE:
    case WM_RENDERFORMAT:
    case WM_TIMER:
    case WM_DESTROYCLIPBOARD:
        params.hwnd   = hwnd;
        params.msg    = msg;
        params.wparam = wp;
        params.lparam = lp;
        return x11drv_clipboard_message( &params );
    }

    return DefWindowProcW( hwnd, msg, wp, lp );
}


/**************************************************************************
 *		clipboard_thread
 *
 * Thread running inside the desktop process to manage the clipboard
 */
static DWORD WINAPI clipboard_thread( void *arg )
{
    static const WCHAR clipboard_classname[] = {'_','_','w','i','n','e','_','c','l','i','p','b','o','a','r','d','_','m','a','n','a','g','e','r',0};
    WNDCLASSW class;
    MSG msg;

    if (!wait_clipboard_mutex()) return 0;

    memset( &class, 0, sizeof(class) );
    class.lpfnWndProc   = clipboard_wndproc;
    class.lpszClassName = clipboard_classname;

    if (!RegisterClassW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        ERR( "could not register clipboard window class err %u\n", GetLastError() );
        return 0;
    }
    if (!CreateWindowW( clipboard_classname, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, 0, NULL ))
    {
        ERR( "failed to create clipboard window err %u\n", GetLastError() );
        return 0;
    }

    while (GetMessageW( &msg, 0, 0, 0 )) DispatchMessageW( &msg );
    return 0;
}


void X11DRV_InitClipboard(void)
{
    DWORD id;
    HANDLE thread = CreateThread( NULL, 0, clipboard_thread, NULL, 0, &id );

    if (thread) CloseHandle( thread );
    else ERR( "failed to create clipboard thread\n" );
}


BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    DisableThreadLibraryCalls( instance );
    x11drv_module = instance;
    return !x11drv_init( NULL );
}


/***********************************************************************
 *           wine_create_desktop (winex11.@)
 */
BOOL CDECL wine_create_desktop( UINT width, UINT height )
{
    struct create_desktop_params params = { .width = width, .height = height };
    return x11drv_create_desktop( &params );
}
