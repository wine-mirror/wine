/* Unit test suite for combo boxes.
 *
 * Copyright 2007 Mikolaj Zalewski
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "wine/test.h"

HWND hMainWnd;

#define expect_eq(expr, value, type, fmt); { type val = expr; ok(val == (value), #expr " expected " #fmt " got " #fmt "\n", (value), val); }
#define expect_rect(r, _left, _top, _right, _bottom) ok(r.left == _left && r.top == _top && \
    r.bottom == _bottom && r.right == _right, "Invalid rect (%d,%d) (%d,%d) vs (%d,%d) (%d,%d)\n", \
    r.left, r.top, r.right, r.bottom, _left, _top, _right, _bottom);

static HWND build_combo(DWORD style)
{
    return CreateWindow("ComboBox", "Combo", WS_VISIBLE|WS_CHILD|style, 5, 5, 100, 100, hMainWnd, NULL, NULL, 0);
}

static int font_height(HFONT hFont)
{
    TEXTMETRIC tm;
    HFONT hFontOld;
    HDC hDC;

    hDC = CreateCompatibleDC(NULL);
    hFontOld = SelectObject(hDC, hFont);
    GetTextMetrics(hDC, &tm);
    SelectObject(hDC, hFontOld);
    DeleteDC(hDC);

    return tm.tmHeight;
}

static INT CALLBACK is_font_installed_proc(const LOGFONT *elf, const TEXTMETRIC *tm, DWORD type, LPARAM lParam)
{
    return 0;
}

static int is_font_installed(const char *name)
{
    HDC hdc = GetDC(NULL);
    BOOL ret = !EnumFontFamilies(hdc, name, is_font_installed_proc, 0);
    ReleaseDC(NULL, hdc);
    return ret;
}

static void test_setfont(DWORD style)
{
    HWND hCombo = build_combo(style);
    HFONT hFont1 = CreateFont(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
    HFONT hFont2 = CreateFont(8, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
    RECT r;
    int i;

    trace("Style %x\n", style);
    GetClientRect(hCombo, &r);
    expect_rect(r, 0, 0, 100, 24);
    SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
    MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
    todo_wine expect_rect(r, 5, 5, 105, 105);

    if (!is_font_installed("Marlett"))
    {
        skip("Marlett font not available\n");
        return;
    }

    if (font_height(hFont1) == 10 && font_height(hFont2) == 8)
    {
        SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 99);

        SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont2, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 16);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 97);

        SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont1, FALSE);
        GetClientRect(hCombo, &r);
        expect_rect(r, 0, 0, 100, 18);
        SendMessageA(hCombo, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&r);
        MapWindowPoints(HWND_DESKTOP, hMainWnd, (LPPOINT)&r, 2);
        todo_wine expect_rect(r, 5, 5, 105, 99);
    }
    else
        skip("Invalid Marlett font heights\n");

    for (i = 1; i < 30; i++)
    {
        HFONT hFont = CreateFont(i, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Marlett");
        int height = font_height(hFont);

        SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont, FALSE);
        GetClientRect(hCombo, &r);
        expect_eq(r.bottom - r.top, height + 8, int, "%d");
        SendMessage(hCombo, WM_SETFONT, 0, FALSE);
        DeleteObject(hFont);
    }

    DestroyWindow(hCombo);
    DeleteObject(hFont1);
    DeleteObject(hFont2);
}

START_TEST(combo)
{
    hMainWnd = CreateWindow("static", "Test", WS_OVERLAPPEDWINDOW, 10, 10, 300, 300, NULL, NULL, NULL, 0);
    ShowWindow(hMainWnd, SW_SHOW);

    test_setfont(CBS_DROPDOWN);
    test_setfont(CBS_DROPDOWNLIST);
    DestroyWindow(hMainWnd);
}
