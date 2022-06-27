/*
 * Dialog theming support
 *
 * Copyright 2022 Zhiyi Zhang for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "uxthemedll.h"
#include "vssym32.h"

extern ATOM atDialogThemeEnabled;
static const WCHAR wine_dialog_brush[] = L"wine_dialog_brush";

static HBRUSH get_dialog_background_brush(HWND hwnd, BOOL create)
{
    HBITMAP bitmap, old_bitmap;
    HDC hdc, hdc_screen;
    HBRUSH brush;
    HTHEME theme;
    DWORD flag;
    HRESULT hr;
    RECT rect;
    SIZE size;

    if (!IsThemeActive())
        return NULL;

    flag = HandleToUlong(GetPropW(hwnd, (LPCWSTR)MAKEINTATOM(atDialogThemeEnabled)));
    if (flag != ETDT_ENABLETAB && flag != ETDT_ENABLEAEROWIZARDTAB)
        return NULL;

    brush = GetPropW(hwnd, wine_dialog_brush);
    if (brush)
        return brush;

    if (!create)
        return NULL;

    theme = OpenThemeData(NULL, L"Tab");
    if (!theme)
        return NULL;

    hr = GetThemePartSize(theme, NULL, TABP_BODY, 0, NULL, TS_TRUE, &size);
    if (FAILED(hr))
    {
        size.cx = 10;
        size.cy = 600;
    }

    hdc_screen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdc_screen);
    bitmap = CreateCompatibleBitmap(hdc_screen, size.cx, size.cy);
    old_bitmap = SelectObject(hdc, bitmap);

    SetRect(&rect, 0, 0, size.cx, size.cy);
    /* FIXME: XP draws the tab body bitmap directly without transparency even if there is */
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DFACE));
    hr = DrawThemeBackground(theme, hdc, TABP_BODY, 0, &rect, NULL);
    if (SUCCEEDED(hr))
    {
        brush = CreatePatternBrush(bitmap);
        SetPropW(hwnd, wine_dialog_brush, brush);
    }

    SelectObject(hdc, old_bitmap);
    DeleteDC(hdc);
    ReleaseDC(NULL, hdc_screen);
    CloseThemeData(theme);
    return brush;
}

static void destroy_dialog_brush(HWND hwnd)
{
    LOGBRUSH logbrush;
    HBRUSH brush;

    brush = GetPropW(hwnd, wine_dialog_brush);
    if (brush)
    {
        RemovePropW(hwnd, wine_dialog_brush);
        if (GetObjectW(brush, sizeof(logbrush), &logbrush) == sizeof(logbrush))
            DeleteObject((HBITMAP)logbrush.lbHatch);
        DeleteObject(brush);
    }
}

LRESULT WINAPI UXTHEME_DefDlgProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, BOOL unicode)
{
    POINT org, old_org;
    WNDPROC dlgproc;
    HBRUSH brush;
    LRESULT lr;
    RECT rect;
    HDC hdc;

    switch (msg)
    {
    case WM_NCDESTROY:
    {
        destroy_dialog_brush(hwnd);
        break;
    }
    case WM_THEMECHANGED:
    {
        destroy_dialog_brush(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    case WM_ERASEBKGND:
    {
        dlgproc = (WNDPROC)GetWindowLongPtrW(hwnd, DWLP_DLGPROC);
        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 0);
        lr = LOWORD(CallWindowProcW(dlgproc, hwnd, msg, wp, lp));
        if (lr || !IsWindow(hwnd))
            return GetWindowLongPtrW(hwnd, DWLP_MSGRESULT);

        brush = get_dialog_background_brush(hwnd, TRUE);
        if (!brush)
        {
            /* Copied from DEFDLG_Proc() */
            brush = (HBRUSH)SendMessageW(hwnd, WM_CTLCOLORDLG, wp, (LPARAM)hwnd);
            if (!brush)
                brush = (HBRUSH)DefWindowProcW(hwnd, WM_CTLCOLORDLG, wp, (LPARAM)hwnd);
            if (brush)
            {
                hdc = (HDC)wp;
                GetClientRect(hwnd, &rect);
                DPtoLP(hdc, (LPPOINT)&rect, 2);
                FillRect(hdc, &rect, brush);
            }
            return TRUE;
        }

        /* Using FillRect() to draw background could introduce a tiling effect if the destination
         * rectangle is larger than the pattern brush size, which is usually 10x600. This bug is
         * visible on property sheet pages if system DPI is set to 192. However, the same bug also
         * exists on XP and explains why vista+ don't use gradient tab body background anymore */
        hdc = (HDC)wp;
        GetViewportOrgEx(hdc, &org);
        SetBrushOrgEx(hdc, org.x, org.y, &old_org);
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, brush);
        SetBrushOrgEx(hdc, old_org.x, old_org.y, NULL);
        return TRUE;
    }
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    {
        dlgproc = (WNDPROC)GetWindowLongPtrW(hwnd, DWLP_DLGPROC);
        lr = CallWindowProcW(dlgproc, hwnd, msg, wp, lp);
        if (lr || !IsWindow(hwnd))
            return lr;

        brush = get_dialog_background_brush(hwnd, FALSE);
        if (!brush)
            return DefWindowProcW(hwnd, msg, wp, lp);

        hdc = (HDC)wp;
        SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        SetBkMode(hdc, TRANSPARENT);

        org.x = 0;
        org.y = 0;
        MapWindowPoints((HWND)lp, hwnd, &org, 1);
        SetBrushOrgEx(hdc, -org.x, -org.y, NULL);
        return (LRESULT)brush;
    }
    }

    return user_api.pDefDlgProc(hwnd, msg, wp, lp, unicode);
}
