/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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
 *           AtlGetVersion              [atl110.@]
 */
DWORD WINAPI AtlGetVersion(void *pReserved)
{
    TRACE("version %04x (%p)\n", _ATL_VER, pReserved);
   return _ATL_VER;
}

/**********************************************************************
 * AtlAxWin class window procedure
 */
static LRESULT CALLBACK AtlAxWin_wndproc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    if ( wMsg == WM_CREATE )
    {
            DWORD len = GetWindowTextLengthW( hWnd ) + 1;
            WCHAR *ptr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
            if (!ptr)
                return 1;
            GetWindowTextW( hWnd, ptr, len );
            AtlAxCreateControlEx( ptr, hWnd, NULL, NULL, NULL, NULL, NULL );
            HeapFree( GetProcessHeap(), 0, ptr );
            return 0;
    }
    return DefWindowProcW( hWnd, wMsg, wParam, lParam );
}

BOOL WINAPI AtlAxWinInit(void)
{
    WNDCLASSEXW wcex;
    const WCHAR AtlAxWin110[] = {'A','t','l','A','x','W','i','n','1','1','0',0};
    const WCHAR AtlAxWinLic110[] = {'A','t','l','A','x','W','i','n','L','i','c','1','1','0',0};

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
    wcex.lpszClassName = AtlAxWin110;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    wcex.lpszClassName = AtlAxWinLic110;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    return TRUE;
}
