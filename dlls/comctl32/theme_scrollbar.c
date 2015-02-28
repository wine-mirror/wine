/*
 * Theming - Scrollbar control
 *
 * Copyright (c) 2015 Mark Harmstone
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
#include "vssym32.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(theme_scroll);

LRESULT CALLBACK THEMING_ScrollbarSubclassProc (HWND hwnd, UINT msg,
                                                WPARAM wParam, LPARAM lParam,
                                                ULONG_PTR dwRefData)
{
    const WCHAR* themeClass = WC_SCROLLBARW;
    HTHEME theme;
    LRESULT result;

    TRACE("(%p, 0x%x, %lu, %lu, %lu)\n", hwnd, msg, wParam, lParam, dwRefData);

    switch (msg) {
        case WM_CREATE:
            result = THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
            OpenThemeData(hwnd, themeClass);
            return result;

        case WM_DESTROY:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

        case WM_THEMECHANGED:
            theme = GetWindowTheme(hwnd);
            CloseThemeData(theme);
            OpenThemeData(hwnd, themeClass);
            break;

        case WM_SYSCOLORCHANGE:
            theme = GetWindowTheme(hwnd);
            if (!theme) return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
            /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
             * which will do the repaint. */
            break;

        default:
            return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
    }

    return 0;
}
