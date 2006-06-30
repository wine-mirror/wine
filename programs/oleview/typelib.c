/*
 * OleView (typelib.c)
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

TYPELIB typelib;
static const WCHAR wszTypeLib[] = { 'T','Y','P','E','L','I','B','\0' };

void PopulateTree(void)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvis;
    ITypeLib *pTypeLib;
    INT count, i;
    BSTR bstrName;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = TreeView_GetSelection(globals.hTree);

    U(tvis).item.mask = TVIF_TEXT|TVIF_CHILDREN;
    U(tvis).item.cChildren = 1;
    tvis.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvis.hParent = TVI_ROOT;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    if(FAILED(LoadTypeLib(((ITEM_INFO*)tvi.lParam)->path, &pTypeLib))) return;

    count = ITypeLib_GetTypeInfoCount(pTypeLib);

    for(i=-1; i<count; i++) {
        ITypeLib_GetDocumentation(pTypeLib, i, &bstrName, NULL, NULL, NULL);

        U(tvis).item.cchTextMax = SysStringLen(bstrName);
        U(tvis).item.pszText = bstrName;

        if(i==-1)
            tvis.hParent = (HTREEITEM)SendMessage(typelib.hTree,
                    TVM_INSERTITEM, 0, (LPARAM)&tvis);
        else SendMessage(typelib.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);

        SysFreeString(bstrName);
    }
    SendMessage(typelib.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tvis.hParent);

    ITypeLib_Release(pTypeLib);
}

void TypeLibResizeChild(void)
{
    RECT client, stat;

    MoveWindow(typelib.hStatusBar, 0, 0, 0, 0, TRUE);

    if(IsWindowVisible(typelib.hStatusBar))
        GetClientRect(typelib.hStatusBar, &stat);
    else stat.bottom = 0;

    GetClientRect(globals.hTypeLibWnd, &client);
    MoveWindow(typelib.hPaneWnd, 0, 0,
            client.right, client.bottom-stat.bottom, TRUE);
}

void UpdateTypeLibStatusBar(int itemID)
{
    WCHAR info[MAX_LOAD_STRING];

    if(!LoadString(globals.hMainInst, itemID, info, sizeof(WCHAR[MAX_LOAD_STRING])))
        LoadString(globals.hMainInst, IDS_READY, info, sizeof(WCHAR[MAX_LOAD_STRING]));

    SendMessage(typelib.hStatusBar, SB_SETTEXT, 0, (LPARAM)info);
}

LRESULT CALLBACK TypeLibProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
        {
            if(!CreatePanedWindow(hWnd, &typelib.hPaneWnd, globals.hMainInst))
                DestroyWindow(hWnd);
            typelib.hTree = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
                    WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
                    0, 0, 0, 0, typelib.hPaneWnd, NULL, globals.hMainInst, NULL);
            typelib.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL,
                    WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_HSCROLL|WS_VSCROLL,
                    0, 0, 0, 0, typelib.hPaneWnd, NULL, globals.hMainInst, NULL);

            SetLeft(typelib.hPaneWnd, typelib.hTree);
            SetRight(typelib.hPaneWnd, typelib.hEdit);

            PopulateTree();
            SetFocus(typelib.hTree);
            break;
        }
        case WM_MENUSELECT:
            UpdateTypeLibStatusBar(LOWORD(wParam));
            break;
        case WM_SETFOCUS:
            SetFocus(typelib.hTree);
            break;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED) break;
            TypeLibResizeChild();
            break;
        case WM_DESTROY:
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL TypeLibRegisterClass(void)
{
    WNDCLASS wcc;

    memset(&wcc, 0, sizeof(WNDCLASS));
    wcc.lpfnWndProc = TypeLibProc;
    wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcc.lpszMenuName = MAKEINTRESOURCE(IDM_TYPELIB);
    wcc.lpszClassName = wszTypeLib;

    if(!RegisterClass(&wcc))
        return FALSE;

    return TRUE;
}

BOOL CreateTypeLibWindow(HINSTANCE hInst)
{
    WCHAR wszTitle[MAX_LOAD_STRING];
    LoadString(hInst, IDS_TYPELIBTITLE, wszTitle, sizeof(WCHAR[MAX_LOAD_STRING]));

    globals.hTypeLibWnd = CreateWindow(wszTypeLib, wszTitle,
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInst, NULL);
    if(!globals.hTypeLibWnd) return FALSE;

    typelib.hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD,
            (LPWSTR)wszTitle, globals.hTypeLibWnd, 0);

    TypeLibResizeChild();
    return TRUE;
}
