/*
 * Theming - Dialogs
 *
 * Copyright (c) 2005 by Frank Richter
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "uxthemedll.h"
#include "vssym32.h"
#include "wine/debug.h"

LRESULT WINAPI UXTHEME_DefDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL unicode)
{
    HTHEME theme = GetWindowTheme ( hWnd );
    static const WCHAR themeClass[] = L"Window";
    LRESULT result;

    switch (msg)
    {
    case WM_CREATE:
        result = user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
	theme = OpenThemeData( hWnd, themeClass );
	return result;
    
    case WM_DESTROY:
        CloseThemeData ( theme );
        SetWindowTheme( hWnd, NULL, NULL );
        OpenThemeData( hWnd, NULL );
        return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);

    default: 
	/* Call old proc */
        return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
    }
    return 0;
}
