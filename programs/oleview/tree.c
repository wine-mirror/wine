/*
 * OleView (tree.c)
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

TREE tree;
static const WCHAR wszCLSID[] = { 'C','L','S','I','D','\\','\0' };
static const WCHAR wszAppID[] = { 'A','p','p','I','D','\\','\0' };
static const WCHAR wszTypeLib[] = { 'T','y','p','e','L','i','b','\\','\0' };
static const WCHAR wszInterface[] = { 'I','n','t','e','r','f','a','c','e','\\','\0' };
static const WCHAR wszComponentCategories[] = { 'C','o','m','p','o','n','e','n','t',
    ' ','C','a','t','e','g','o','r','i','e','s','\\','\0' };

LPARAM CreateITEM_INFO(INT flag, const WCHAR *info, const WCHAR *clsid)
{
    ITEM_INFO *reg;

    reg = HeapAlloc(GetProcessHeap(), 0, sizeof(ITEM_INFO));
    memset(reg, 0, sizeof(ITEM_INFO));

    reg->cFlag = flag;
    strcpyW(reg->info, info);
    if(clsid) strcpyW(reg->clsid, clsid);

    return (LPARAM)reg;
}

void AddBaseEntries(void)
{
    TVINSERTSTRUCT tvis;
    WCHAR name[MAX_LOAD_STRING];

    tvis.item.mask = TVIF_TEXT|TVIF_CHILDREN|TVIF_PARAM;
    /* FIXME add TVIF_IMAGE */
    tvis.item.pszText = name;
    tvis.item.cchTextMax = MAX_LOAD_STRING;
    tvis.item.cChildren = 1;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvis.hParent = TVI_ROOT;

    LoadString(globals.hMainInst, IDS_TREE_I, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = CreateITEM_INFO(REGTOP, wszInterface, NULL);
    tree.hI = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_TL, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = CreateITEM_INFO(REGTOP, wszTypeLib, NULL);
    tree.hTL = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_AID, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = CreateITEM_INFO(REGTOP|REGPATH, wszAppID, NULL);
    tree.hAID = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_OC, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = (LPARAM)NULL;
    tree.hOC = TreeView_InsertItem(globals.hTree, &tvis);


    tvis.hParent = tree.hOC;
    LoadString(globals.hMainInst, IDS_TREE_AO, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = CreateITEM_INFO(REGTOP, wszCLSID, NULL);
    tree.hAO = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_CLO, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tree.hCLO = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_O1O, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = (LPARAM)NULL;
    tree.hO1O = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_GBCC, tvis.item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tvis.item.lParam = CreateITEM_INFO(REGTOP|REGPATH, wszComponentCategories, NULL);
    tree.hGBCC = TreeView_InsertItem(globals.hTree, &tvis);

    SendMessage(globals.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tree.hOC);
}

void EmptyTree(void)
{
    HTREEITEM cur, del;
    TVITEM tvi;

    tvi.mask = TVIF_PARAM;
    cur = TreeView_GetChild(globals.hTree, TVI_ROOT);

    while(TRUE)
    {
        del = cur;
        cur = TreeView_GetChild(globals.hTree, del);

        if(!cur) cur = TreeView_GetNextSibling(globals.hTree, del);
        if(!cur)
        {
            cur = TreeView_GetPrevSibling(globals.hTree, del);
            if(!cur) cur = TreeView_GetParent(globals.hTree, del);

            tvi.hItem = del;
            SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

            if(tvi.lParam)
                HeapFree(GetProcessHeap(), 0, (ITEM_INFO *)tvi.lParam);

            SendMessage(globals.hTree, TVM_DELETEITEM, 0, (LPARAM)del);

            if(!cur) break;
        }
    }
}

void AddTreeEx(void)
{
    AddBaseEntries();
}

LRESULT CALLBACK TreeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            globals.hTree = CreateWindow(WC_TREEVIEW, NULL,
                    WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
                    0, 0, 0, 0, hWnd, (HMENU)TREE_WINDOW, globals.hMainInst, NULL);
            AddTreeEx();
            break;
        case WM_SIZE:
            MoveWindow(globals.hTree, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

HWND CreateTreeWindow(HINSTANCE hInst)
{
    WNDCLASS wct;
    const WCHAR wszTreeClass[] = { 'T','R','E','E','\0' };

    memset(&wct, 0, sizeof(WNDCLASS));
    wct.lpfnWndProc = TreeProc;
    wct.lpszClassName = wszTreeClass;

    if(!RegisterClass(&wct)) return NULL;

    return CreateWindowEx(WS_EX_CLIENTEDGE, wszTreeClass, NULL, WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, globals.hPaneWnd, NULL, hInst, NULL);
}
