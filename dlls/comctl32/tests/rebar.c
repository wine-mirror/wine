/* Unit tests for rebar.
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

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "wine/test.h"

static HWND hMainWnd;
static HWND hRebar;

#define expect_eq(expr, value, type, format) { type ret = expr; ok((value) == ret, #expr " expected " format "  got " format "\n", (value), (ret)); }

static void rebuild_rebar(HWND *hRebar)
{
    if (hRebar)
        DestroyWindow(*hRebar);

    *hRebar = CreateWindow(REBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
        hMainWnd, (HMENU)17, GetModuleHandle(NULL), NULL);
    SendMessageA(*hRebar, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FONT), 0);
}

static LRESULT CALLBACK MyWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

static void expect_band_content(UINT uBand, UINT fStyle, COLORREF clrFore,
    COLORREF clrBack, LPCSTR lpText, int iImage, HWND hwndChild,
    UINT cxMinChild, UINT cyMinChild, UINT cx, HBITMAP hbmBack, UINT wID,
    /*UINT cyChild, UINT cyMaxChild, UINT cyIntegral,*/ UINT cxIdeal, LPARAM lParam,
    UINT cxHeader)
{
    CHAR buf[MAX_PATH] = "abc";
    REBARBANDINFO rb;

    memset(&rb, 0xdd, sizeof(rb));
    rb.cbSize = sizeof(rb);
    rb.fMask = RBBIM_BACKGROUND | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_COLORS
        | RBBIM_HEADERSIZE | RBBIM_ID | RBBIM_IDEALSIZE | RBBIM_IMAGE | RBBIM_LPARAM
        | RBBIM_SIZE | RBBIM_STYLE | RBBIM_TEXT;
    rb.lpText = buf;
    rb.cch = MAX_PATH;
    ok(SendMessageA(hRebar, RB_GETBANDINFOA, uBand, (LPARAM)&rb), "RB_GETBANDINFO failed\n");
    expect_eq(rb.fStyle, fStyle, int, "%x");
    todo_wine expect_eq(rb.clrFore, clrFore, COLORREF, "%x");
    todo_wine expect_eq(rb.clrBack, clrBack, int, "%x");
    expect_eq(strcmp(rb.lpText, lpText), 0, int, "%d");
    expect_eq(rb.iImage, iImage, int, "%x");
    expect_eq(rb.hwndChild, hwndChild, HWND, "%p");
    expect_eq(rb.cxMinChild, cxMinChild, int, "%d");
    expect_eq(rb.cyMinChild, cyMinChild, int, "%d");
    expect_eq(rb.cx, cx, int, "%d");
    expect_eq(rb.hbmBack, hbmBack, HBITMAP, "%p");
    expect_eq(rb.wID, wID, int, "%d");
    /* in Windows the values of cyChild, cyMaxChild and cyIntegral can't be read */
    todo_wine expect_eq(rb.cyChild, 0xdddddddd, int, "%x");
    todo_wine expect_eq(rb.cyMaxChild, 0xdddddddd, int, "%x");
    todo_wine expect_eq(rb.cyIntegral, 0xdddddddd, int, "%x");
    expect_eq(rb.cxIdeal, cxIdeal, int, "%d");
    expect_eq(rb.lParam, lParam, LPARAM, "%lx");
    expect_eq(rb.cxHeader, cxHeader, int, "%d");
}

static void bandinfo_test()
{
    REBARBANDINFOA rb;
    CHAR szABC[] = "ABC";
    CHAR szABCD[] = "ABCD";

    rebuild_rebar(&hRebar);
    rb.cbSize = sizeof(REBARBANDINFO);
    rb.fMask = 0;
    ok(SendMessageA(hRebar, RB_INSERTBANDA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0, 0, 0);

    rb.fMask = RBBIM_CHILDSIZE;
    rb.cxMinChild = 15;
    rb.cyMinChild = 20;
    rb.cyChild = 30;
    rb.cyMaxChild = 20;
    rb.cyIntegral = 10;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 0);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 35);

    rb.cbSize = sizeof(REBARBANDINFO);
    rb.fMask = 0;
    ok(SendMessageA(hRebar, RB_INSERTBANDA, 1, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(1, 0, 0, GetSysColor(COLOR_3DFACE), "", -1, NULL, 0, 0, 0, NULL, 0, 0, 0, 9);
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 40);

    rb.fMask = RBBIM_HEADERSIZE;
    rb.cxHeader = 50;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 50);

    rb.cxHeader = 5;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 5);

    rb.fMask = RBBIM_TEXT;
    rb.lpText = szABCD;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0x40000000, 0, GetSysColor(COLOR_3DFACE), "ABCD", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 5);
    rb.fMask = RBBIM_STYLE | RBBIM_TEXT;
    rb.fStyle = 0;
    rb.lpText = szABC;
    ok(SendMessageA(hRebar, RB_SETBANDINFOA, 0, (LPARAM)&rb), "RB_INSERTBAND failed\n");
    expect_band_content(0, 0, 0, GetSysColor(COLOR_3DFACE), "ABC", -1, NULL, 15, 20, 0, NULL, 0, 0, 0, 40);

    DestroyWindow(hRebar);
}

START_TEST(rebar)
{
    INITCOMMONCONTROLSEX icc;
    WNDCLASSA wc;
    MSG msg;
    RECT rc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_COOL_CLASSES;
    InitCommonControlsEx(&icc);

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(IDC_IBEAM));
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = MyWndProc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);
    ShowWindow(hMainWnd, SW_SHOW);

    bandinfo_test();
    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);
}
