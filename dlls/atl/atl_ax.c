/*
 * Active Template Library ActiveX functions (atl.dll)
 *
 * Copyright 2006 Andrey Turkin
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
#include "winuser.h"
#include "winerror.h"
#include "winuser.h"
#include "wine/debug.h"
#include "objbase.h"
#include "objidl.h"
#include "ole2.h"
#include "exdisp.h"
#include "atlbase.h"
#include "atliface.h"
#include "atlwin.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/**********************************************************************
 * AtlAxWin class window procedure
 */
LRESULT static CALLBACK AtlAxWin_wndproc( HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    if ( wMsg == WM_CREATE )
    {
            DWORD len = GetWindowTextLengthW( hWnd ) + 1;
            WCHAR *ptr = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
            if (!ptr)
                return 1;
            GetWindowTextW( hWnd, ptr, len );
            AtlAxCreateControl( ptr, hWnd, NULL, NULL );
            HeapFree( GetProcessHeap(), 0, ptr );
            return 0;
    }
    return DefWindowProcW( hWnd, wMsg, wParam, lParam );
}

/***********************************************************************
 *           AtlAxWinInit          [ATL.@]
 * Initializes the control-hosting code: registering the AtlAxWin, 
 * AtlAxWin7 and AtlAxWinLic7 window classes and some messages.
 *
 * RETURNS
 *  TRUE or FALSE
 */
 
BOOL WINAPI AtlAxWinInit(void)
{
    WNDCLASSEXW wcex;
    const WCHAR AtlAxWin[] = {'A','t','l','A','x','W','i','n',0};

    FIXME("semi-stub\n");

    if ( FAILED( OleInitialize(NULL) ) )
        return FALSE;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = 0;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = GetModuleHandleW( NULL );
    wcex.hIcon         = NULL;
    wcex.hCursor       = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName  = NULL;
    wcex.hIconSm       = 0;

    wcex.lpfnWndProc   = AtlAxWin_wndproc;
    wcex.lpszClassName = AtlAxWin;
    if ( !RegisterClassExW( &wcex ) )
        return FALSE;

    return TRUE;
}
