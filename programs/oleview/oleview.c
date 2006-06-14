/*
 * OleView (oleview.c)
 *
 * Copyright 2006 Piotr Caban
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

#include "main.h"

GLOBALS globals;
static WCHAR wszRegEdit[] = { 'r','e','g','e','d','i','t','.','e','x','e','\0' };

void ResizeChild(void)
{
    RECT client, stat, tool;

    MoveWindow(globals.hStatusBar, 0, 0, 0, 0, TRUE);
    MoveWindow(globals.hToolBar, 0, 0, 0, 0, TRUE);

    if(IsWindowVisible(globals.hStatusBar))
        GetClientRect(globals.hStatusBar, &stat);
    else stat.bottom = 0;

    if(IsWindowVisible(globals.hToolBar))
    {
        GetClientRect(globals.hToolBar, &tool);
        tool.bottom += 2;
    }
    else tool.bottom = 0;

    GetClientRect(globals.hMainWnd, &client);
    MoveWindow(globals.hPaneWnd, 0, tool.bottom,
            client.right, client.bottom-tool.bottom-stat.bottom, TRUE);
}

void RefreshMenu(HTREEITEM item)
{
    TVITEM tvi;
    HTREEITEM parent;
    HMENU hMenu = GetMenu(globals.hMainWnd);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = item;
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, FALSE);
    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, FALSE);
    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, FALSE);

    if(tvi.lParam && ((ITEM_INFO *)tvi.lParam)->cFlag&SHOWALL)
    {
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_HTMLTAG, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_VIEW, MF_GRAYED);

        if(!((ITEM_INFO *)tvi.lParam)->loaded)
        {
            EnableMenuItem(hMenu, IDM_CREATEINST, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_RELEASEINST, MF_GRAYED);
            SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, TRUE);
        }
        else 
        {
            EnableMenuItem(hMenu, IDM_CREATEINST, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_RELEASEINST, MF_ENABLED);
            SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, TRUE);
        }
    }
    else if(tvi.lParam && ((ITEM_INFO *)tvi.lParam)->cFlag&INTERFACE)
    {
        EnableMenuItem(hMenu, IDM_TYPEINFO, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_RELEASEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_HTMLTAG, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_VIEW, MF_ENABLED);
        SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, TRUE);
    }
    else
    {
        EnableMenuItem(hMenu, IDM_TYPEINFO, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_RELEASEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_HTMLTAG, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_VIEW, MF_GRAYED);
    }
    parent = TreeView_GetParent(globals.hTree, item);
    if(parent==tree.hAID || parent==tree.hGBCC)
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_ENABLED);
}

int MenuCommand(WPARAM wParam, HWND hWnd)
{
    BOOL vis;
    HTREEITEM hSelect;
    WCHAR wszAbout[MAX_LOAD_STRING];
    WCHAR wszAboutVer[MAX_LOAD_STRING];

    switch(wParam)
    {
        case IDM_ABOUT:
            LoadString(globals.hMainInst, IDS_ABOUT, wszAbout,
                    sizeof(WCHAR[MAX_LOAD_STRING]));
            LoadString(globals.hMainInst, IDS_ABOUTVER, wszAboutVer,
                    sizeof(WCHAR[MAX_LOAD_STRING]));
            ShellAbout(hWnd, wszAbout, wszAboutVer, NULL);
            break;
        case IDM_CREATEINST:
            hSelect = TreeView_GetSelection(globals.hTree);
            CreateInst(hSelect);
            SendMessage(globals.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hSelect);
            break;
        case IDM_RELEASEINST:
            hSelect = TreeView_GetSelection(globals.hTree);
            ReleaseInst(hSelect);
            RefreshMenu(hSelect);
            break;
        case IDM_EXPERT:
            globals.bExpert = !globals.bExpert;
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    globals.bExpert ? MF_CHECKED : MF_UNCHECKED);
            EmptyTree();
            if(globals.bExpert) AddTreeEx();
            else AddTree();
            hSelect = TreeView_GetChild(globals.hTree, TVI_ROOT);
            SendMessage(globals.hTree, TVM_SELECTITEM, 0, (LPARAM)hSelect);
            RefreshMenu(hSelect);
            break;
        case IDM_FLAG_INSERV:
            vis = globals.dwClsCtx&CLSCTX_INPROC_SERVER;
            globals.dwClsCtx = globals.dwClsCtx&(~CLSCTX_INPROC_SERVER);
            globals.dwClsCtx = globals.dwClsCtx|((~vis)&CLSCTX_INPROC_SERVER);
            if(!globals.dwClsCtx) globals.dwClsCtx = vis;
            else CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            break;
        case IDM_FLAG_INHANDL:
            vis = globals.dwClsCtx&CLSCTX_INPROC_HANDLER;
            globals.dwClsCtx = globals.dwClsCtx&(~CLSCTX_INPROC_HANDLER);
            globals.dwClsCtx = globals.dwClsCtx|((~vis)&CLSCTX_INPROC_HANDLER);
            if(!globals.dwClsCtx) globals.dwClsCtx = vis;
            else CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            break;
        case IDM_FLAG_LOCSERV:
            vis = globals.dwClsCtx&CLSCTX_LOCAL_SERVER;
            globals.dwClsCtx = globals.dwClsCtx&(~CLSCTX_LOCAL_SERVER);
            globals.dwClsCtx = globals.dwClsCtx|((~vis)&CLSCTX_LOCAL_SERVER);
            if(!globals.dwClsCtx) globals.dwClsCtx = vis;
            else CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            break;
        case IDM_FLAG_REMSERV:
            vis = globals.dwClsCtx&CLSCTX_REMOTE_SERVER;
            globals.dwClsCtx = globals.dwClsCtx&(~CLSCTX_REMOTE_SERVER);
            globals.dwClsCtx = globals.dwClsCtx|((~vis)&CLSCTX_REMOTE_SERVER);
            if(!globals.dwClsCtx) globals.dwClsCtx = vis;
            else CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            break;
        case IDM_REFRESH:
            EmptyTree();
            if(globals.bExpert) AddTreeEx();
            else AddTree();
            hSelect = TreeView_GetChild(globals.hTree, TVI_ROOT);
            SendMessage(globals.hTree, TVM_SELECTITEM, 0, (LPARAM)hSelect);
            RefreshMenu(hSelect);
            break;
        case IDM_REGEDIT:
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            CreateProcess(NULL, wszRegEdit, NULL, NULL, FALSE, 0,\
                    NULL, NULL, &si, &pi);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            break;
        }
        case IDM_STATUSBAR:
            vis = IsWindowVisible(globals.hStatusBar);
            ShowWindow(globals.hStatusBar, vis ? SW_HIDE : SW_SHOW);
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            ResizeChild();
            break;
        case IDM_TOOLBAR:
            vis = IsWindowVisible(globals.hToolBar);
            ShowWindow(globals.hToolBar, vis ? SW_HIDE : SW_SHOW);
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            ResizeChild();
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
    }
    return 0;
}

void UpdateStatusBar(int itemID)
{
    WCHAR info[MAX_LOAD_STRING];

    if(!LoadString(globals.hMainInst, itemID, info, sizeof(WCHAR[MAX_LOAD_STRING])))
        LoadString(globals.hMainInst, IDS_READY, info, sizeof(WCHAR[MAX_LOAD_STRING]));

    SendMessage(globals.hStatusBar, SB_SETTEXT, 0, (LPARAM)info);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            OleInitialize(NULL);
            if(!CreatePanedWindow(hWnd, &globals.hPaneWnd, globals.hMainInst))
                PostQuitMessage(0);
            SetLeft(globals.hPaneWnd, CreateTreeWindow(globals.hMainInst));
            SetFocus(globals.hTree);
            break;
        case WM_COMMAND:
            MenuCommand(LOWORD(wParam), hWnd);
            break;
        case WM_DESTROY:
            OleUninitialize();
            EmptyTree();
            PostQuitMessage(0);
            break;
        case WM_MENUSELECT:
            UpdateStatusBar(LOWORD(wParam));
            break;
        case WM_SETFOCUS:
            SetFocus(globals.hTree);
            break;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED) break;
            ResizeChild();
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL InitApplication(HINSTANCE hInst)
{
    WNDCLASS wc;
    WCHAR wszAppName[MAX_LOAD_STRING];

    LoadString(hInst, IDS_APPNAME, wszAppName, sizeof(WCHAR[MAX_LOAD_STRING]));

    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDM_MENU);
    wc.lpszClassName = wszAppName;

    if(!RegisterClass(&wc))
        return FALSE;

    return TRUE;
}

BOOL InitInstance(HINSTANCE hInst, int nCmdShow)
{
    HWND hWnd;
    WCHAR wszAppName[MAX_LOAD_STRING];
    WCHAR wszTitle[MAX_LOAD_STRING];
    TBBUTTON tB[] = {
        {0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
        {0, IDM_BIND, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
        {1, IDM_TYPELIB, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
        {0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
        {2, IDM_REGEDIT, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
        {0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
        {3, IDM_CREATEINST, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
        {4, IDM_RELEASEINST, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
        {0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
        {5, IDM_VIEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0}
    };

    LoadString(hInst, IDS_APPNAME, wszAppName, sizeof(WCHAR[MAX_LOAD_STRING]));
    LoadString(hInst, IDS_APPTITLE, wszTitle, sizeof(WCHAR[MAX_LOAD_STRING]));

    hWnd = CreateWindow(wszAppName, wszTitle, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);
    if(!hWnd) return FALSE;

    globals.hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD,
            (LPWSTR)wszTitle, hWnd, 0);

    globals.hToolBar = CreateToolbarEx(hWnd, WS_CHILD|WS_VISIBLE, 0, 1, hInst,
            IDB_TOOLBAR, tB, 10, 16, 16, 16, 16, sizeof(TBBUTTON));
    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, FALSE);
    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, FALSE);
    SendMessage(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, FALSE);

    globals.hMainWnd = hWnd;
    globals.hMainInst = hInst;
    globals.bExpert = TRUE;
    globals.dwClsCtx = CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
                LPWSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;
   
    if(!hPrevInst)
    {
        if(!InitApplication(hInst))
            return FALSE;
    }

    if(!InitInstance(hInst, nCmdShow))
        return FALSE;

    hAccelTable = LoadAccelerators(hInst, MAKEINTRESOURCE(IDA_OLEVIEW));

    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(TranslateAccelerator(globals.hMainWnd, hAccelTable, &msg)) continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
