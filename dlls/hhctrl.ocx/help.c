/*
 * Help Viewer Implementation
 *
 * Copyright 2005 James Hawkins
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "htmlhelp.h"
#include "ole2.h"

/* Window type defaults */

#define WINTYPE_DEFAULT_X       280
#define WINTYPE_DEFAULT_Y       100
#define WINTYPE_DEFAULT_WIDTH   740
#define WINTYPE_DEFAULT_HEIGHT  640

typedef struct tagHHInfo
{
    HH_WINTYPEW *pHHWinType;
    HINSTANCE hInstance;
    LPCWSTR szCmdLine;
    HFONT hFont;
} HHInfo;

static LPWSTR HH_ANSIToUnicode(LPCSTR ansi)
{
    LPWSTR unicode;
    int count;

    count = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
    unicode = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, count);

    return unicode;
}

/* Toolbar */

static BOOL HH_AddToolbar(HHInfo *pHHInfo)
{
    return TRUE;
}

/* Navigation Pane */

static BOOL HH_AddNavigationPane(HHInfo *pHHInfo)
{
    return TRUE;
}

/* HTML Pane */

static BOOL HH_AddHTMLPane(HHInfo *pHHInfo)
{
    return TRUE;
}

/* Viewer Window */

LRESULT CALLBACK Help_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {

        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static BOOL HH_CreateHelpWindow(HHInfo *pHHInfo)
{
    HWND hWnd;
    HINSTANCE hInstance = pHHInfo->hInstance;
    WNDCLASSEXW wcex;
    DWORD dwStyles, dwExStyles;
    DWORD x, y, width, height;

    static const WCHAR windowClassW[] = {
        'H','H',' ', 'P','a','r','e','n','t',0
    };

    static const WCHAR windowTitleW[] = {
        'H','T','M','L',' ','H','e','l','p',0
    };

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)Help_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = windowClassW;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    RegisterClassExW(&wcex);

    dwStyles = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR |
                 WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;

    /* these will be loaded from the CHM file in the future if they're provided */
    x = WINTYPE_DEFAULT_X;
    y = WINTYPE_DEFAULT_Y;
    width = WINTYPE_DEFAULT_WIDTH;
    height = WINTYPE_DEFAULT_HEIGHT;

    hWnd = CreateWindowExW(dwExStyles, windowClassW, windowTitleW, dwStyles,
                           x, y, width, height, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pHHInfo);

    pHHInfo->pHHWinType->hwndHelp = hWnd;
    return TRUE;
}

static void HH_CreateFont(HHInfo *pHHInfo)
{
    LOGFONTW lf;

    GetObjectW(GetStockObject(ANSI_VAR_FONT), sizeof(LOGFONTW), &lf);
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;

    pHHInfo->hFont = CreateFontIndirectW(&lf);
}

static void HH_InitRequiredControls(DWORD dwControls)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = dwControls;
    InitCommonControlsEx(&icex);
}

/* Creates the whole package */
static BOOL HH_CreateViewer(HHInfo *pHHInfo)
{
    HH_CreateFont(pHHInfo);

    if (!HH_CreateHelpWindow(pHHInfo))
        return FALSE;

    HH_InitRequiredControls(ICC_BAR_CLASSES);

    if (!HH_AddToolbar(pHHInfo))
        return FALSE;

    if (!HH_AddNavigationPane(pHHInfo))
        return FALSE;

    if (!HH_AddHTMLPane(pHHInfo))
        return FALSE;

    return TRUE;
}

static HHInfo *HH_OpenHH(HINSTANCE hInstance, LPCWSTR szCmdLine)
{
    HHInfo *pHHInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HHInfo));

    pHHInfo->pHHWinType = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HH_WINTYPEW));
    pHHInfo->hInstance = hInstance;
    pHHInfo->szCmdLine = szCmdLine;

    return pHHInfo;
}

static void HH_Close(HHInfo *pHHInfo)
{
    if (!pHHInfo)
        return;

    HeapFree(GetProcessHeap(), 0, pHHInfo->pHHWinType);
}

/* FIXME: Check szCmdLine for bad arguments */
int WINAPI doWinMain(HINSTANCE hInstance, LPSTR szCmdLine)
{
    MSG msg;
    HHInfo *pHHInfo;

    if (OleInitialize(NULL) != S_OK)
        return -1;

    pHHInfo = HH_OpenHH(hInstance, HH_ANSIToUnicode(szCmdLine));
    if (!pHHInfo || !HH_CreateViewer(pHHInfo))
    {
        OleUninitialize();
        return -1;
    }

    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    HH_Close(pHHInfo);
    HeapFree(GetProcessHeap(), 0, pHHInfo);
    OleUninitialize();

    return 0;
}
