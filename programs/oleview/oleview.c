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
#include "commdlg.h"
#include "shellapi.h"
#include "wine/unicode.h"

GLOBALS globals;
static const WCHAR wszRegEdit[] = { '\\','r','e','g','e','d','i','t','.','e','x','e','\0' };
static const WCHAR wszFormat[] = { '<','o','b','j','e','c','t','\n',' ',' ',' ',
    'c','l','a','s','s','i','d','=','\"','c','l','s','i','d',':','%','s','\"','\n',
    '>','\n','<','/','o','b','j','e','c','t','>','\0' };

static INT_PTR CALLBACK SysConfProc(HWND hDlgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HKEY hKey;
    WCHAR buffer[MAX_LOAD_STRING];
    DWORD bufSize;

    WCHAR wszReg[] = { 'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','O','L','E','\\','\0' };
    WCHAR wszEnableDCOM[] = { 'E','n','a','b','l','e','D','C','O','M','\0' };
    WCHAR wszEnableRemote[] = { 'E','n','a','b','l','e',
        'R','e','m','o','t','e','C','o','n','n','e','c','t','\0' };
    WCHAR wszYes[] = { 'Y', '\0' };
    WCHAR wszNo[] = { 'N', '\0' };

    switch(uMsg)
    {
        case WM_INITDIALOG:
            if(RegOpenKeyW(HKEY_LOCAL_MACHINE, wszReg, &hKey) != ERROR_SUCCESS)
                RegCreateKeyW(HKEY_LOCAL_MACHINE, wszReg, &hKey);

            bufSize = sizeof(buffer);
            if(RegGetValueW(hKey, NULL, wszEnableDCOM, RRF_RT_REG_SZ,
                        NULL, buffer, &bufSize) != ERROR_SUCCESS)
            {
                bufSize = sizeof(wszYes);
                RegSetValueExW(hKey, wszEnableDCOM, 0, REG_SZ, (BYTE*)wszYes, bufSize);
            }

            CheckDlgButton(hDlgWnd, IDC_ENABLEDCOM,
                    buffer[0]=='Y' ? BST_CHECKED : BST_UNCHECKED);

            bufSize = sizeof(buffer);
            if(RegGetValueW(hKey, NULL, wszEnableRemote, RRF_RT_REG_SZ,
                        NULL, buffer, &bufSize) != ERROR_SUCCESS)
            {
                bufSize = sizeof(wszYes);
                RegSetValueExW(hKey, wszEnableRemote, 0, REG_SZ, (BYTE*)wszYes, bufSize);
            }

            CheckDlgButton(hDlgWnd, IDC_ENABLEREMOTE,
                    buffer[0]=='Y' ? BST_CHECKED : BST_UNCHECKED);
            
            RegCloseKey(hKey);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDOK:
                bufSize = sizeof(wszYes);

                RegOpenKeyW(HKEY_LOCAL_MACHINE, wszReg, &hKey);

                RegSetValueExW(hKey, wszEnableDCOM, 0, REG_SZ,
                        IsDlgButtonChecked(hDlgWnd, IDC_ENABLEDCOM) == BST_CHECKED ?
                        (BYTE*)wszYes : (BYTE*)wszNo, bufSize);

                RegSetValueExW(hKey, wszEnableRemote, 0, REG_SZ,
                        IsDlgButtonChecked(hDlgWnd, IDC_ENABLEREMOTE) == BST_CHECKED ?
                        (BYTE*)wszYes : (BYTE*)wszNo, bufSize);

                RegCloseKey(hKey);

                EndDialog(hDlgWnd, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hDlgWnd, IDCANCEL);
                return TRUE;
            }
    }

    return FALSE;
}

static INT_PTR CALLBACK CreateInstOnProc(HWND hDlgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hEdit;

    switch(uMsg)
    {
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDOK:
                memset(globals.wszMachineName, 0, sizeof(WCHAR[MAX_LOAD_STRING]));
                hEdit = GetDlgItem(hDlgWnd, IDC_MACHINE);

                if (GetWindowTextLengthW(hEdit)>0)
                    GetWindowTextW(hEdit, globals.wszMachineName, MAX_LOAD_STRING);

                EndDialog(hDlgWnd, IDOK);
                return TRUE;
            case IDCANCEL:
                EndDialog(hDlgWnd, IDCANCEL);
                return TRUE;
            }
    }

    return FALSE;
}

static void InitOpenFileName(HWND hWnd, OPENFILENAMEW *pofn, WCHAR *wszFilter,
        WCHAR *wszTitle, WCHAR *wszFileName)
{
    memset(pofn, 0, sizeof(OPENFILENAMEW));
    pofn->lStructSize = sizeof(OPENFILENAMEW);
    pofn->hwndOwner = hWnd;
    pofn->hInstance = globals.hMainInst;

    pofn->lpstrTitle = wszTitle;
    pofn->lpstrFilter = wszFilter;
    pofn->nFilterIndex = 0;
    pofn->lpstrFile = wszFileName;
    pofn->nMaxFile = MAX_LOAD_STRING;
    pofn->Flags = OFN_HIDEREADONLY | OFN_ENABLESIZING;
}

static void CopyClsid(HTREEITEM item)
{
    TVITEMW tvi;

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.hItem = item;
    tvi.cchTextMax = MAX_LOAD_STRING;
    SendMessageW(globals.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);

    if(OpenClipboard(globals.hMainWnd) && EmptyClipboard() && tvi.lParam)
    {
        HANDLE hClipData = GlobalAlloc(GHND, sizeof(WCHAR[MAX_LOAD_STRING]));
        LPVOID pLoc = GlobalLock(hClipData);

        lstrcpyW(pLoc, ((ITEM_INFO *)tvi.lParam)->clsid);
        GlobalUnlock(hClipData);
        SetClipboardData(CF_UNICODETEXT, hClipData);
        CloseClipboard();
    }
}

static void CopyHTMLTag(HTREEITEM item)
{
    TVITEMW tvi;

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.hItem = item;
    tvi.cchTextMax = MAX_LOAD_STRING;
    SendMessageW(globals.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);

    if(OpenClipboard(globals.hMainWnd) && EmptyClipboard() && tvi.lParam)
    {
        HANDLE hClipData = GlobalAlloc(GHND, sizeof(WCHAR[MAX_LOAD_STRING]));
        LPVOID pLoc = GlobalLock(hClipData);
        int clsidLen = lstrlenW(((ITEM_INFO *)tvi.lParam)->clsid)-1;

        ((ITEM_INFO *)tvi.lParam)->clsid[clsidLen] = '\0';
        wsprintfW(pLoc, wszFormat, ((ITEM_INFO *)tvi.lParam)->clsid+1);
        ((ITEM_INFO *)tvi.lParam)->clsid[clsidLen] = '}';

        GlobalUnlock(hClipData);
        SetClipboardData(CF_UNICODETEXT, hClipData);
        CloseClipboard();
    }
}

static void ResizeChild(void)
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
    TVITEMW tvi;
    HTREEITEM parent;
    HMENU hMenu = GetMenu(globals.hMainWnd);

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.hItem = item;
    SendMessageW(globals.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);

    parent = (HTREEITEM)SendMessageW(globals.hTree, TVM_GETNEXTITEM,
            TVGN_PARENT, (LPARAM)item);

    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, FALSE);
    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, FALSE);
    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, FALSE);

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
            SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, TRUE);
        }
        else 
        {
            EnableMenuItem(hMenu, IDM_CREATEINST, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_RELEASEINST, MF_ENABLED);
            SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, TRUE);
        }
    }
    else if(tvi.lParam && 
            (((ITEM_INFO *)tvi.lParam)->cFlag&INTERFACE || parent==tree.hTL))
    {
        EnableMenuItem(hMenu, IDM_TYPEINFO, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATEINSTON, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_RELEASEINST, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_HTMLTAG, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_VIEW, MF_ENABLED);
        SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, TRUE);
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

    if(parent==tree.hAID || parent==tree.hGBCC)
        EnableMenuItem(hMenu, IDM_COPYCLSID, MF_ENABLED);
}

static int MenuCommand(WPARAM wParam, HWND hWnd)
{
    BOOL vis;
    HTREEITEM hSelect;
    WCHAR wszAbout[MAX_LOAD_STRING];

    switch(wParam)
    {
        case IDM_ABOUT:
            LoadStringW(globals.hMainInst, IDS_ABOUT, wszAbout, ARRAY_SIZE(wszAbout));
            ShellAboutW(hWnd, wszAbout, NULL, NULL);
            break;
        case IDM_COPYCLSID:
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            CopyClsid(hSelect);
            break;
        case IDM_HTMLTAG:
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            CopyHTMLTag(hSelect);
            break;
        case IDM_CREATEINST:
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            CreateInst(hSelect, NULL);
            SendMessageW(globals.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hSelect);
            break;
        case IDM_CREATEINSTON:
            if(DialogBoxW(0, MAKEINTRESOURCEW(DLG_CREATEINSTON),
                        hWnd, CreateInstOnProc) == IDCANCEL) break;
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            CreateInst(hSelect, globals.wszMachineName);
            SendMessageW(globals.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)hSelect);
            break;
        case IDM_RELEASEINST:
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            ReleaseInst(hSelect);
            RefreshMenu(hSelect);
            RefreshDetails(hSelect);
            break;
        case IDM_EXPERT:
            globals.bExpert = !globals.bExpert;
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    globals.bExpert ? MF_CHECKED : MF_UNCHECKED);
            EmptyTree();
            if(globals.bExpert) AddTreeEx();
            else AddTree();
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)TVI_ROOT);
            SendMessageW(globals.hTree, TVM_SELECTITEM, 0, (LPARAM)hSelect);
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
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)TVI_ROOT);
            SendMessageW(globals.hTree, TVM_SELECTITEM, 0, (LPARAM)hSelect);
            RefreshMenu(hSelect);
            break;
        case IDM_REGEDIT:
        {
            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            WCHAR app[MAX_PATH];

            GetWindowsDirectoryW(app, MAX_PATH - ARRAY_SIZE(wszRegEdit));
            lstrcatW( app, wszRegEdit );
            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            if (CreateProcessW(app, app, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            break;
        }
        case IDM_STATUSBAR:
            vis = IsWindowVisible(globals.hStatusBar);
            ShowWindow(globals.hStatusBar, vis ? SW_HIDE : SW_SHOW);
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            ResizeChild();
            break;
        case IDM_SYSCONF:
            DialogBoxW(0, MAKEINTRESOURCEW(DLG_SYSCONF), hWnd, SysConfProc);
            break;
        case IDM_TOOLBAR:
            vis = IsWindowVisible(globals.hToolBar);
            ShowWindow(globals.hToolBar, vis ? SW_HIDE : SW_SHOW);
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            ResizeChild();
            break;
        case IDM_TYPELIB:
            {
            static const WCHAR filterW[] = {'%','s','%','c','*','.','t','l','b',';','*','.','o','l','b',';','*','.','d','l','l',';','*','.','o','c','x',';','*','.','e','x','e','%','c','%','s','%','c','*','.','*','%','c',0};
            OPENFILENAMEW ofn;
            static WCHAR wszTitle[MAX_LOAD_STRING];
            static WCHAR wszName[MAX_LOAD_STRING];
            WCHAR filter_typelib[MAX_LOAD_STRING], filter_all[MAX_LOAD_STRING], filter[MAX_PATH];

            LoadStringW(globals.hMainInst, IDS_OPEN, wszTitle, ARRAY_SIZE(wszTitle));
            LoadStringW(globals.hMainInst, IDS_OPEN_FILTER_TYPELIB, filter_typelib, ARRAY_SIZE(filter_typelib));
            LoadStringW(globals.hMainInst, IDS_OPEN_FILTER_ALL, filter_all, ARRAY_SIZE(filter_all));
            wsprintfW( filter, filterW, filter_typelib, 0, 0, filter_all, 0, 0 );
            InitOpenFileName(hWnd, &ofn, filter, wszTitle, wszName);
            if(GetOpenFileNameW(&ofn)) CreateTypeLibWindow(globals.hMainInst, wszName);
            break;
            }
        case IDM_VIEW:
            hSelect = (HTREEITEM)SendMessageW(globals.hTree,
                    TVM_GETNEXTITEM, TVGN_CARET, 0);
            if(IsInterface(hSelect)) InterfaceViewer(hSelect);
            else CreateTypeLibWindow(globals.hMainInst, NULL);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
    }
    return 0;
}

static void UpdateStatusBar(int itemID)
{
    WCHAR info[MAX_LOAD_STRING];

    if(!LoadStringW(globals.hMainInst, itemID, info, ARRAY_SIZE(info)))
        LoadStringW(globals.hMainInst, IDS_READY, info, ARRAY_SIZE(info));

    SendMessageW(globals.hStatusBar, SB_SETTEXTW, 0, (LPARAM)info);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            OleInitialize(NULL);
            PaneRegisterClassW();
            TypeLibRegisterClassW();
            if(!CreatePanedWindow(hWnd, &globals.hPaneWnd, globals.hMainInst))
                PostQuitMessage(0);
            SetLeft(globals.hPaneWnd, CreateTreeWindow(globals.hMainInst));
            SetRight(globals.hPaneWnd, CreateDetailsWindow(globals.hMainInst));
            SetFocus(globals.hTree);
            break;
        case WM_COMMAND:
            MenuCommand(LOWORD(wParam), hWnd);
            break;
        case WM_DESTROY:
            EmptyTree();
            OleUninitialize();
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
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL InitApplication(HINSTANCE hInst)
{
    WNDCLASSW wc;
    WCHAR wszAppName[MAX_LOAD_STRING];

    LoadStringW(hInst, IDS_APPNAME, wszAppName, ARRAY_SIZE(wszAppName));

    memset(&wc, 0, sizeof(WNDCLASSW));
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDM_MENU);
    wc.lpszClassName = wszAppName;

    if(!RegisterClassW(&wc))
        return FALSE;

    return TRUE;
}

static BOOL InitInstance(HINSTANCE hInst, int nCmdShow)
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

    LoadStringW(hInst, IDS_APPNAME, wszAppName, ARRAY_SIZE(wszAppName));
    LoadStringW(hInst, IDS_APPTITLE, wszTitle, ARRAY_SIZE(wszTitle));

    hWnd = CreateWindowW(wszAppName, wszTitle, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInst, NULL);
    if(!hWnd) return FALSE;

    globals.hStatusBar = CreateStatusWindowW(WS_VISIBLE|WS_CHILD,
            wszTitle, hWnd, 0);

    globals.hToolBar = CreateToolbarEx(hWnd, WS_CHILD|WS_VISIBLE, 0, 1, hInst,
            IDB_TOOLBAR, tB, 10, 16, 16, 16, 16, sizeof(TBBUTTON));
    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_CREATEINST, FALSE);
    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_RELEASEINST, FALSE);
    SendMessageW(globals.hToolBar, TB_ENABLEBUTTON, IDM_VIEW, FALSE);

    globals.hMainWnd = hWnd;
    globals.hMainInst = hInst;
    globals.bExpert = TRUE;
    globals.dwClsCtx = CLSCTX_INPROC_SERVER|CLSCTX_LOCAL_SERVER;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HANDLE hAccelTable;

    if(!InitApplication(hInst))
        return FALSE;

    if(!InitInstance(hInst, nCmdShow))
        return FALSE;

    hAccelTable = LoadAcceleratorsW(hInst, MAKEINTRESOURCEW(IDA_OLEVIEW));

    while(GetMessageW(&msg, NULL, 0, 0))
    {
        if(TranslateAcceleratorW(globals.hMainWnd, hAccelTable, &msg)) continue;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.wParam;
}
