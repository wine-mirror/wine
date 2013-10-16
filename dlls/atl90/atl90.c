/*
 * Copyright 2013 Zhenbo Li
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "atlbase.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/***********************************************************************
 *           AtlGetVersion              [atl90.@]
 */
DWORD WINAPI AtlGetVersion(void *reserved)
{
    TRACE("version %04x (%p)\n", _ATL_VER, reserved);
    return _ATL_VER;
}

/**********************************************************************
 * AtlAxWin class window procedure
 */
static LRESULT CALLBACK AtlAxWin_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if ( msg == WM_CREATE )
    {
            DWORD len = GetWindowTextLengthW( hwnd ) + 1;
            WCHAR *ptr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
            if (!ptr)
                return 1;
            GetWindowTextW( hwnd, ptr, len );
            AtlAxCreateControlEx( ptr, hwnd, NULL, NULL, NULL, NULL, NULL );
            HeapFree( GetProcessHeap(), 0, ptr );
            return 0;
    }
    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

BOOL WINAPI AtlAxWinInit(void)
{
    WNDCLASSEXW wcex;
    const WCHAR AtlAxWin90[] = {'A','t','l','A','x','W','i','n','9','0',0};
    const WCHAR AtlAxWinLic90[] = {'A','t','l','A','x','W','i','n','L','i','c','9','0',0};

    FIXME("version %04x semi-stub\n", _ATL_VER);

    if ( FAILED( OleInitialize(NULL) ) )
        return FALSE;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = GetModuleHandleW( NULL );
    wcex.hIcon         = NULL;
    wcex.hCursor       = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName  = NULL;
    wcex.hIconSm       = 0;

    wcex.lpfnWndProc   = AtlAxWin_wndproc;
    wcex.lpszClassName = AtlAxWin90;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    wcex.lpszClassName = AtlAxWinLic90;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    return TRUE;
}
