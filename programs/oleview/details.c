/*
 * OleView (details.c)
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

DETAILS details;

void RefreshDetails(HTREEITEM item)
{
    TVITEM tvi;
    WCHAR wszBuf[MAX_LOAD_STRING];
    WCHAR wszStaticText[MAX_LOAD_STRING];
    const WCHAR wszFormat[] = { '%','s','\n','%','s','\0' };
    BOOL show;

    memset(&tvi, 0, sizeof(TVITEM));
    memset(&wszStaticText, 0, sizeof(WCHAR[MAX_LOAD_STRING]));
    tvi.mask = TVIF_TEXT;
    tvi.hItem = item;
    tvi.pszText = wszBuf;
    tvi.cchTextMax = MAX_LOAD_STRING;
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    if(tvi.lParam)
        wsprintfW(wszStaticText, wszFormat, tvi.pszText, ((ITEM_INFO *)tvi.lParam)->clsid);
    else strcpyW(wszStaticText, tvi.pszText);

    SetWindowText(details.hStatic, wszStaticText);

    SendMessage(details.hTab, TCM_SETCURSEL, 0, 0);

    if(tvi.lParam && ((ITEM_INFO *)tvi.lParam)->cFlag & SHOWALL)
    {
        if(TabCtrl_GetItemCount(details.hTab) == 1)
        {
            TCITEM tci;
            memset(&tci, 0, sizeof(TCITEM));
            tci.mask = TCIF_TEXT;
            tci.pszText = wszBuf;
            tci.cchTextMax = sizeof(WCHAR[MAX_LOAD_STRING]);

            LoadString(globals.hMainInst, IDS_TAB_IMPL,
                    wszBuf, sizeof(WCHAR[MAX_LOAD_STRING]));
            SendMessage(details.hTab, TCM_INSERTITEM, 1, (LPARAM)&tci);

            LoadString(globals.hMainInst, IDS_TAB_ACTIV,
                    wszBuf, sizeof(WCHAR[MAX_LOAD_STRING]));
            SendMessage(details.hTab, TCM_INSERTITEM, 2, (LPARAM)&tci);
        }
    }
    else
    {
        SendMessage(details.hTab, TCM_DELETEITEM, 2, 0);
        SendMessage(details.hTab, TCM_DELETEITEM, 1, 0);
    }

    show = CreateRegPath(item, wszBuf, MAX_LOAD_STRING);
    ShowWindow(details.hTab, show ? SW_SHOW : SW_HIDE);

    /* FIXME Next line deals with TreeView_EnsureVisible bug */
    SendMessage(details.hReg, TVM_ENSUREVISIBLE, 0,
            SendMessage(details.hReg, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)TVI_ROOT));
    SendMessage(details.hReg, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
}

void CreateTabCtrl(HWND hWnd)
{
    TCITEM tci;
    WCHAR buffer[MAX_LOAD_STRING];

    memset(&tci, 0, sizeof(TCITEM));
    tci.mask = TCIF_TEXT;
    tci.pszText = buffer;
    tci.cchTextMax = sizeof(WCHAR[MAX_LOAD_STRING]);

    details.hTab = CreateWindow(WC_TABCONTROL, NULL, WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)TAB_WINDOW, globals.hMainInst, NULL);
    ShowWindow(details.hTab, SW_HIDE);

    LoadString(globals.hMainInst, IDS_TAB_REG, buffer, sizeof(WCHAR[MAX_LOAD_STRING]));
    SendMessage(details.hTab, TCM_INSERTITEM, 0, (LPARAM)&tci);

    details.hReg = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
            WS_CHILD|WS_VISIBLE|TVS_HASLINES,
            0, 0, 0, 0, details.hTab, NULL, globals.hMainInst, NULL);
}

LRESULT CALLBACK DetailsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int sel;

    switch(uMsg)
    {
        case WM_CREATE:
            {
                const WCHAR wszStatic[] = { 'S','t','a','t','i','c','\0' };

                details.hStatic = CreateWindow(wszStatic, NULL, WS_CHILD|WS_VISIBLE,
                        0, 0, 0, 0, hWnd, NULL, globals.hMainInst, NULL);
                CreateTabCtrl(hWnd);
            }
            break;
        case WM_SIZE:
            MoveWindow(details.hStatic, 0, 0, LOWORD(lParam), 40, TRUE);
            MoveWindow(details.hTab, 3, 40, LOWORD(lParam)-6, HIWORD(lParam)-43, TRUE);
            MoveWindow(details.hReg, 10, 34, LOWORD(lParam)-26,
                    HIWORD(lParam)-87, TRUE);
            break;
        case WM_NOTIFY:
            if((int)wParam != TAB_WINDOW) break;
            switch(((LPNMHDR)lParam)->code)
            {
                case TCN_SELCHANGE:
                    ShowWindow(details.hReg, SW_HIDE);
                    sel = TabCtrl_GetCurSel(details.hTab);

                    if(sel==0) ShowWindow(details.hReg, SW_SHOW);
                    break;
            }
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

HWND CreateDetailsWindow(HINSTANCE hInst)
{
    WNDCLASS wcd;
    const WCHAR wszDetailsClass[] = { 'D','E','T','A','I','L','S','\0' };
    
    memset(&wcd, 0, sizeof(WNDCLASS));
    wcd.lpfnWndProc = DetailsProc;
    wcd.lpszClassName = wszDetailsClass;
    wcd.hbrBackground = (HBRUSH)COLOR_WINDOW;

    if(!RegisterClass(&wcd)) return NULL;

    globals.hDetails = CreateWindowEx(WS_EX_CLIENTEDGE, wszDetailsClass, NULL,
            WS_CHILD|WS_VISIBLE, 0, 0, 0, 0, globals.hPaneWnd, NULL, hInst, NULL);

    return globals.hDetails;
}
