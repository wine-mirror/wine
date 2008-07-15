/*
 * Theming - Button control
 *
 * Copyright (c) 2008 by Reece H. Dunn
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
#include "tmschema.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(themingbutton);

#define BUTTON_TYPE 0x0f /* bit mask for the available button types */

typedef void (*pfThemedPaint)(HTHEME theme, HWND hwnd, HDC hdc);

static void GB_draw(HTHEME theme, HWND hwnd, HDC hDC)
{
    RECT bgRect, textRect;
    SIZE textExtent;
    HFONT font = (HFONT)SendMessageW(hwnd, WM_GETFONT, 0, 0);
    HFONT hPrevFont = font ? SelectObject(hDC, font) : NULL;
    int state = IsWindowEnabled(hwnd) ? GBS_NORMAL : GBS_DISABLED;
    WCHAR text[MAX_PATH];
    int len = MAX_PATH;

    GetClientRect(hwnd, &bgRect);
    textRect = bgRect;

    len = GetWindowTextW(hwnd, text, len);

    GetTextExtentPoint32W(hDC, text, len, &textExtent);

    bgRect.top += (textExtent.cy / 2);
    textRect.left += 10;
    textRect.bottom = textRect.top + textExtent.cy;
    textRect.right = textRect.left + textExtent.cx + 4;

    ExcludeClipRect(hDC, textRect.left, textRect.top, textRect.right, textRect.bottom);

    DrawThemeBackground(theme, hDC, BP_GROUPBOX, state, &bgRect, NULL);

    SelectClipRgn(hDC, NULL);

    textRect.left += 2;
    textRect.right -= 2;
    DrawThemeText(theme, hDC, BP_GROUPBOX, state, text, len, 0, 0, &textRect);

    if (hPrevFont) SelectObject(hDC, hPrevFont);
}

static const pfThemedPaint btnThemedPaintFunc[BUTTON_TYPE + 1] =
{
    NULL, /* BS_PUSHBUTTON */
    NULL, /* BS_DEFPUSHBUTTON */
    NULL, /* BS_CHECKBOX */
    NULL, /* BS_AUTOCHECKBOX */
    NULL, /* BS_RADIOBUTTON */
    NULL, /* BS_3STATE */
    NULL, /* BS_AUTO3STATE */
    GB_draw, /* BS_GROUPBOX */
    NULL, /* BS_USERBUTTON */
    NULL, /* BS_AUTORADIOBUTTON */
    NULL, /* Not defined */
    NULL, /* BS_OWNERDRAW */
    NULL, /* Not defined */
    NULL, /* Not defined */
    NULL, /* Not defined */
    NULL, /* Not defined */
};

static BOOL BUTTON_Paint(HTHEME theme, HWND hwnd, HDC hParamDC)
{
    PAINTSTRUCT ps;
    HDC hDC;
    DWORD dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    pfThemedPaint paint = btnThemedPaintFunc[ dwStyle & BUTTON_TYPE ];

    if (paint)
    {
        hDC = hParamDC ? hParamDC : BeginPaint(hwnd, &ps);
        paint(theme, hwnd, hDC);
        if (!hParamDC) EndPaint(hwnd, &ps);
        return TRUE;
    }

    return FALSE; /* Delegate drawing to the non-themed code. */
}

/**********************************************************************
 * The button control subclass window proc.
 */
LRESULT CALLBACK THEMING_ButtonSubclassProc(HWND hwnd, UINT msg,
                                            WPARAM wParam, LPARAM lParam,
                                            ULONG_PTR dwRefData)
{
    const WCHAR* themeClass = WC_BUTTONW;
    HTHEME theme;
    LRESULT result;

    switch (msg)
    {
    case WM_CREATE:
        result = THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
        OpenThemeData(hwnd, themeClass);
        return result;

    case WM_DESTROY:
        theme = GetWindowTheme(hwnd);
        CloseThemeData (theme);
        return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

    case WM_THEMECHANGED:
        theme = GetWindowTheme(hwnd);
        CloseThemeData (theme);
        OpenThemeData(hwnd, themeClass);
        break;

    case WM_SYSCOLORCHANGE:
        theme = GetWindowTheme(hwnd);
	if (!theme) return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
        /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
	 * which will do the repaint. */
        break;

    case WM_PAINT:
        theme = GetWindowTheme(hwnd);
        if (theme && BUTTON_Paint(theme, hwnd, (HDC)wParam))
            return 0;
        else
            return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

    case WM_ENABLE:
        theme = GetWindowTheme(hwnd);
        if (theme) RedrawWindow(hwnd, NULL, NULL,
                                RDW_FRAME | RDW_INVALIDATE | RDW_UPDATENOW);
        return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);

    default:
	/* Call old proc */
	return THEMING_CallOriginalClass(hwnd, msg, wParam, lParam);
    }
    return 0;
}
