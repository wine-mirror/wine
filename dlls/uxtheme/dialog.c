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
    BOOL themingActive = IsThemeDialogTextureEnabled (hWnd);
    BOOL doTheming = themingActive && (theme != NULL);
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

    case WM_THEMECHANGED:
        CloseThemeData ( theme );
	OpenThemeData( hWnd, themeClass );
	InvalidateRect( hWnd, NULL, TRUE );
	return 0;

    case WM_ERASEBKGND:
        if (!doTheming) return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
        {
            RECT rc;
            WNDPROC dlgp = (WNDPROC)GetWindowLongPtrW (hWnd, DWLP_DLGPROC);
            if (!CallWindowProcW(dlgp, hWnd, msg, wParam, lParam))
            {
                /* Draw background*/
                GetClientRect (hWnd, &rc);
                if (IsThemePartDefined (theme, WP_DIALOG, 0))
                    /* Although there is a theme for the WINDOW class/DIALOG part, 
                     * but I[res] haven't seen Windows using it yet... Even when
                     * dialog theming is activated, the good ol' BTNFACE 
                     * background seems to be used. */
#if 0
                    DrawThemeBackground (theme, (HDC)wParam, WP_DIALOG, 0, &rc, 
                        NULL);
#endif
                    return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
                else 
                /* We might have gotten a TAB theme class, so check if we can 
                 * draw as a tab page. */
                if (IsThemePartDefined (theme, TABP_BODY, 0))
                    DrawThemeBackground (theme, (HDC)wParam, TABP_BODY, 0, &rc, 
                        NULL);
                else
                    return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
            }
            return 1;
        }

    case WM_CTLCOLORSTATIC:
        if (!doTheming) return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
        {
            WNDPROC dlgp = (WNDPROC)GetWindowLongPtrW (hWnd, DWLP_DLGPROC);
            LRESULT result = CallWindowProcW(dlgp, hWnd, msg, wParam, lParam);
            if (!result)
            {
                /* Override defaults with more suitable values when themed */
                HDC controlDC = (HDC)wParam;
                HWND controlWnd = (HWND)lParam;
                WCHAR controlClass[32];
                RECT rc;

                GetClassNameW (controlWnd, controlClass, ARRAY_SIZE(controlClass));
                if (lstrcmpiW (controlClass, WC_STATICW) == 0)
                {
                    /* Static control - draw parent background and set text to 
                     * transparent, so it looks right on tab pages. */
                    GetClientRect (controlWnd, &rc);
                    DrawThemeParentBackground (controlWnd, controlDC, &rc);
                    SetBkMode (controlDC, TRANSPARENT);

                    /* Return NULL brush since we painted the BG already */
                    return (LRESULT)GetStockObject (NULL_BRUSH);
                }
                else
                    return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);

            }
            return result;
        }

    default: 
	/* Call old proc */
        return user_api.pDefDlgProc(hWnd, msg, wParam, lParam, unicode);
    }
    return 0;
}
