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
#include "wine/unicode.h"

#include "resource.h"

/* Window type defaults */

#define WINTYPE_DEFAULT_X           280
#define WINTYPE_DEFAULT_Y           100
#define WINTYPE_DEFAULT_WIDTH       740
#define WINTYPE_DEFAULT_HEIGHT      640
#define WINTYPE_DEFAULT_NAVWIDTH    250

typedef struct tagHHInfo
{
    HH_WINTYPEW *pHHWinType;
    HINSTANCE hInstance;
    LPWSTR szCmdLine;
    HWND hwndTabCtrl;
    HFONT hFont;
} HHInfo;

extern HINSTANCE hhctrl_hinstance;

static LPWSTR HH_ANSIToUnicode(LPCSTR ansi)
{
    LPWSTR unicode;
    int count;

    count = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
    unicode = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, unicode, count);

    return unicode;
}

/* Loads a string from the resource file */
static LPWSTR HH_LoadString(DWORD dwID)
{
    LPWSTR string = NULL;
    int iSize;

    iSize = LoadStringW(hhctrl_hinstance, dwID, NULL, 0);
    iSize += 2; /* some strings (tab text) needs double-null termination */

    string = HeapAlloc(GetProcessHeap(), 0, iSize * sizeof(WCHAR));
    LoadStringW(hhctrl_hinstance, dwID, string, iSize);

    return string;
}

/* Child Window */

static const WCHAR szChildClass[] = {
    'H','H',' ','C','h','i','l','d',0
};

static const WCHAR szEmpty[] = {0};

LRESULT CALLBACK Child_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static void HH_RegisterChildWndClass(HHInfo *pHHInfo)
{
    WNDCLASSEXW wcex;

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = 0;
    wcex.lpfnWndProc    = (WNDPROC)Child_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = pHHInfo->hInstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_3DFACE);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szChildClass;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    RegisterClassExW(&wcex);
}

/* Toolbar */

#define ICON_SIZE   20

static void TB_AddButton(TBBUTTON *pButtons, DWORD dwIndex, DWORD dwID)
{
    /* FIXME: Load the correct button bitmaps */
    pButtons[dwIndex].iBitmap = STD_PRINT;
    pButtons[dwIndex].idCommand = dwID;
    pButtons[dwIndex].fsState = TBSTATE_ENABLED;
    pButtons[dwIndex].fsStyle = BTNS_BUTTON;
    pButtons[dwIndex].dwData = 0;
    pButtons[dwIndex].iString = 0;
}

static void TB_AddButtonsFromFlags(TBBUTTON *pButtons, DWORD dwButtonFlags, LPDWORD pdwNumButtons)
{
    *pdwNumButtons = 0;

    if (dwButtonFlags & HHWIN_BUTTON_EXPAND)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_EXPAND);

    if (dwButtonFlags & HHWIN_BUTTON_BACK)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_BACK);

    if (dwButtonFlags & HHWIN_BUTTON_FORWARD)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_FORWARD);

    if (dwButtonFlags & HHWIN_BUTTON_STOP)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_STOP);

    if (dwButtonFlags & HHWIN_BUTTON_REFRESH)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_REFRESH);

    if (dwButtonFlags & HHWIN_BUTTON_HOME)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_HOME);

    if (dwButtonFlags & HHWIN_BUTTON_SYNC)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_SYNC);

    if (dwButtonFlags & HHWIN_BUTTON_OPTIONS)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_OPTIONS);

    if (dwButtonFlags & HHWIN_BUTTON_PRINT)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_PRINT);

    if (dwButtonFlags & HHWIN_BUTTON_JUMP1)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_JUMP1);

    if (dwButtonFlags & HHWIN_BUTTON_JUMP2)
        TB_AddButton(pButtons,(*pdwNumButtons)++, IDTB_JUMP2);

    if (dwButtonFlags & HHWIN_BUTTON_ZOOM)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_ZOOM);

    if (dwButtonFlags & HHWIN_BUTTON_TOC_NEXT)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_TOC_NEXT);

    if (dwButtonFlags & HHWIN_BUTTON_TOC_PREV)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_TOC_PREV);
}

static BOOL HH_AddToolbar(HHInfo *pHHInfo)
{
    HWND hToolbar;
    HWND hwndParent = pHHInfo->pHHWinType->hwndHelp;
    DWORD toolbarFlags = pHHInfo->pHHWinType->fsToolBarFlags;
    TBBUTTON buttons[IDTB_TOC_PREV - IDTB_EXPAND];
    TBADDBITMAP tbAB;
    DWORD dwStyles, dwExStyles;
    DWORD dwNumButtons, dwIndex;

    /* FIXME: Remove the following line once we read the CHM file */
    toolbarFlags = HHWIN_BUTTON_EXPAND | HHWIN_BUTTON_BACK | HHWIN_BUTTON_STOP |
                   HHWIN_BUTTON_REFRESH | HHWIN_BUTTON_HOME | HHWIN_BUTTON_PRINT;
    TB_AddButtonsFromFlags(buttons, toolbarFlags, &dwNumButtons);

    dwStyles = WS_CHILDWINDOW | WS_VISIBLE | TBSTYLE_FLAT |
               TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | CCS_NODIVIDER;
    dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;

    hToolbar = CreateWindowExW(dwExStyles, TOOLBARCLASSNAMEW, NULL, dwStyles,
                               0, 0, 0, 0, hwndParent, NULL,
                               pHHInfo->hInstance, NULL);
    if (!hToolbar)
        return FALSE;

    SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(ICON_SIZE, ICON_SIZE));
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hToolbar, WM_SETFONT, (WPARAM)pHHInfo->hFont, TRUE);

    /* FIXME: Load correct icons for all buttons */
    tbAB.hInst = HINST_COMMCTRL;
    tbAB.nID = IDB_STD_LARGE_COLOR;
    SendMessageW(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbAB);

    for (dwIndex = 0; dwIndex < dwNumButtons; dwIndex++)
    {
        LPWSTR szBuf = HH_LoadString(buttons[dwIndex].idCommand);
        DWORD dwLen = strlenW(szBuf);
        szBuf[dwLen + 2] = 0; /* Double-null terminate */

        buttons[dwIndex].iString = (DWORD)SendMessageW(hToolbar, TB_ADDSTRINGW, 0, (LPARAM)szBuf);
        HeapFree(GetProcessHeap(), 0, szBuf);
    }

    SendMessageW(hToolbar, TB_ADDBUTTONSW, dwNumButtons, (LPARAM)&buttons);
    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
    ShowWindow(hToolbar, SW_SHOW);

    pHHInfo->pHHWinType->hwndToolBar = hToolbar;
    return TRUE;
}

/* Navigation Pane */

static void NP_GetNavigationRect(HHInfo *pHHInfo, RECT *rc)
{
    HWND hwndParent = pHHInfo->pHHWinType->hwndHelp;
    HWND hwndToolbar = pHHInfo->pHHWinType->hwndToolBar;
    RECT rectWND, rectTB;

    GetClientRect(hwndParent, &rectWND);
    GetClientRect(hwndToolbar, &rectTB);

    rc->left = 0;
    rc->top = rectTB.bottom;
    rc->bottom = rectWND.bottom - rectTB.bottom;

    if (pHHInfo->pHHWinType->fsValidMembers & HHWIN_PARAM_NAV_WIDTH)
        rc->right = pHHInfo->pHHWinType->iNavWidth;
    else
        rc->right = WINTYPE_DEFAULT_NAVWIDTH;
}

static void NP_CreateTab(HINSTANCE hInstance, HWND hwndTabCtrl, DWORD dwStrID, DWORD dwIndex)
{
    TCITEMW tie;
    LPWSTR tabText = HH_LoadString(dwStrID);

    tie.mask = TCIF_TEXT;
    tie.pszText = tabText;

    TabCtrl_InsertItemW(hwndTabCtrl, dwIndex, &tie);
    HeapFree(GetProcessHeap(), 0, tabText);
}

static BOOL HH_AddNavigationPane(HHInfo *pHHInfo)
{
    HWND hWnd, hwndTabCtrl;
    HWND hwndParent = pHHInfo->pHHWinType->hwndHelp;
    DWORD dwStyles = WS_CHILDWINDOW | WS_VISIBLE;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
    DWORD dwIndex = 0;
    RECT rc;

    NP_GetNavigationRect(pHHInfo, &rc);

    hWnd = CreateWindowExW(dwExStyles, szChildClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, pHHInfo->hInstance, NULL);
    if (!hWnd)
        return FALSE;

    hwndTabCtrl = CreateWindowExW(dwExStyles, WC_TABCONTROLW, szEmpty, dwStyles,
                                  0, 0, rc.right, rc.bottom, hWnd,
                                  NULL, pHHInfo->hInstance, NULL);
    if (!hwndTabCtrl)
        return FALSE;

    /* FIXME: Check which tabs to include when we read the CHM file */
    NP_CreateTab(pHHInfo->hInstance, hwndTabCtrl, IDS_CONTENTS, dwIndex++);
    NP_CreateTab(pHHInfo->hInstance, hwndTabCtrl, IDS_INDEX, dwIndex++);
    NP_CreateTab(pHHInfo->hInstance, hwndTabCtrl, IDS_SEARCH, dwIndex++);
    NP_CreateTab(pHHInfo->hInstance, hwndTabCtrl, IDS_FAVORITES, dwIndex++);

    SendMessageW(hwndTabCtrl, WM_SETFONT, (WPARAM)pHHInfo->hFont, TRUE);

    pHHInfo->hwndTabCtrl = hwndTabCtrl;
    pHHInfo->pHHWinType->hwndNavigation = hWnd;
    return TRUE;
}

/* HTML Pane */

static void HP_GetHTMLRect(HHInfo *pHHInfo, RECT *rc)
{
    HWND hwndParent = pHHInfo->pHHWinType->hwndHelp;
    HWND hwndToolbar = pHHInfo->pHHWinType->hwndToolBar;
    HWND hwndNavigation = pHHInfo->pHHWinType->hwndNavigation;
    RECT rectTB, rectWND, rectNP;

    GetClientRect(hwndParent, &rectWND);
    GetClientRect(hwndToolbar, &rectTB);
    GetClientRect(hwndNavigation, &rectNP);

    rc->left = rectNP.right;
    rc->top = rectTB.bottom;
    rc->right = rectWND.right - rectNP.right;
    rc->bottom = rectWND.bottom - rectTB.bottom;
}

static BOOL HH_AddHTMLPane(HHInfo *pHHInfo)
{
    HWND hWnd;
    HWND hwndParent = pHHInfo->pHHWinType->hwndHelp;
    DWORD dwStyles = WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE;
    RECT rc;

    HP_GetHTMLRect(pHHInfo, &rc);

    hWnd = CreateWindowExW(dwExStyles, szChildClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, pHHInfo->hInstance, NULL);
    if (!hWnd)
        return FALSE;

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pHHInfo);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    pHHInfo->pHHWinType->hwndHTML = hWnd;
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

    HH_RegisterChildWndClass(pHHInfo);

    if (!HH_AddNavigationPane(pHHInfo))
        return FALSE;

    if (!HH_AddHTMLPane(pHHInfo))
        return FALSE;

    return TRUE;
}

static HHInfo *HH_OpenHH(HINSTANCE hInstance, LPWSTR szCmdLine)
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
    HeapFree(GetProcessHeap(), 0, pHHInfo->szCmdLine);
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
