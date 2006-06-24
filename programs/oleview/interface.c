/*
 * OleView (interface.c)
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

typedef struct
{
    WCHAR *wszLabel;
    WCHAR *wszIdentifier;
}DIALOG_INFO;

BOOL IsInterface(HTREEITEM item)
{
    TVITEM tvi;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = item;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    if(!tvi.lParam) return FALSE;

    if(((ITEM_INFO*)tvi.lParam)->cFlag & INTERFACE) return TRUE;
    return FALSE;
}

IUnknown *GetInterface(void)
{
    HTREEITEM hSelect;
    TVITEM tvi;
    CLSID clsid;
    IUnknown *unk;

    hSelect = TreeView_GetSelection(globals.hTree);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = hSelect;
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    CLSIDFromString(((ITEM_INFO *)tvi.lParam)->clsid, &clsid);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = TreeView_GetParent(globals.hTree, hSelect);
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    IUnknown_QueryInterface(((ITEM_INFO *)tvi.lParam)->pU, &clsid, (void *)&unk);

    return unk;
}

INT_PTR CALLBACK InterfaceViewerProc(HWND hDlgWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    DIALOG_INFO *di;
    HWND hObject;
    IUnknown *unk;
    HRESULT hRes;
    ULARGE_INTEGER size;
    WCHAR wszSize[MAX_LOAD_STRING];
    WCHAR wszTRUE[] = { 'T','R','U','E','\0' };
    WCHAR wszFALSE[] = { 'F','A','L','S','E','\0' };
    WCHAR wszFormat[] = { '%','d',' ','b','y','t','e','s','\0' };

    switch(uMsg)
    {
        case WM_INITDIALOG:
            di = (DIALOG_INFO *)lParam;
            hObject = GetDlgItem(hDlgWnd, IDC_LABEL);
            SetWindowText(hObject, di->wszLabel);
            hObject = GetDlgItem(hDlgWnd, IDC_IDENTIFIER);
            SetWindowText(hObject, di->wszIdentifier);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                EndDialog(hDlgWnd, IDCANCEL);
                return TRUE;
            case IDC_ISDIRTY_BUTTON:
                unk = GetInterface();
                hRes = IPersistStream_IsDirty((IPersistStream *)unk);
                IUnknown_Release(unk);
                hObject = GetDlgItem(hDlgWnd, IDC_ISDIRTY);
                SetWindowText(hObject, hRes ==  S_OK ? wszFALSE : wszTRUE);
                return TRUE;
            case IDC_GETSIZEMAX_BUTTON:
                unk = GetInterface();
                IPersistStream_GetSizeMax((IPersistStream *)unk, &size);
                IUnknown_Release(unk);
                wsprintfW(wszSize, wszFormat, size);
                hObject = GetDlgItem(hDlgWnd, IDC_GETSIZEMAX);
                SetWindowText(hObject, wszSize);
                return TRUE;
            }
    }
    return FALSE;
}

void IPersistStreamInterfaceViewer(WCHAR *clsid)
{
    DIALOG_INFO di;
    WCHAR wszClassMoniker[] = { 'C','l','a','s','s','M','o','n','i','k','e','r','\0' };

    di.wszLabel = wszClassMoniker;
    di.wszIdentifier = clsid;

    DialogBoxParam(0, MAKEINTRESOURCE(DLG_IPERSISTSTREAM_IV),
            globals.hMainWnd, InterfaceViewerProc, (LPARAM)&di);
}

void IPersistInterfaceViewer(WCHAR *clsid)
{
    DIALOG_INFO di;
    WCHAR wszClassMoniker[] = { 'C','l','a','s','s','M','o','n','i','k','e','r','\0' };

    di.wszLabel = wszClassMoniker;
    di.wszIdentifier = clsid;

    DialogBoxParam(0, MAKEINTRESOURCE(DLG_IPERSIST_IV),
            globals.hMainWnd, InterfaceViewerProc, (LPARAM)&di);
}

void DefaultInterfaceViewer(WCHAR *clsid, WCHAR *wszName)
{
    DIALOG_INFO di;

    di.wszLabel = wszName;
    di.wszIdentifier = clsid;

    DialogBoxParam(0, MAKEINTRESOURCE(DLG_DEFAULT_IV),
            globals.hMainWnd, InterfaceViewerProc, (LPARAM)&di);
}

void InterfaceViewer(HTREEITEM item)
{
    TVITEM tvi;
    WCHAR *clsid;
    WCHAR wszName[MAX_LOAD_STRING];
    WCHAR wszIPersistStream[] = { '{','0','0','0','0','0','1','0','9','-',
        '0','0','0','0','-','0','0','0','0','-','C','0','0','0','-',
        '0','0','0','0','0','0','0','0','0','0','4','6','}','\0' };
    WCHAR wszIPersist[] = { '{','0','0','0','0','0','1','0','C','-',
        '0','0','0','0','-','0','0','0','0','-','C','0','0','0','-',
        '0','0','0','0','0','0','0','0','0','0','4','6','}','\0' };

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_TEXT;
    tvi.hItem = item;
    tvi.cchTextMax = MAX_LOAD_STRING;
    tvi.pszText = wszName;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    clsid = ((ITEM_INFO*)tvi.lParam)->clsid;

    if(!memcmp(clsid, wszIPersistStream, sizeof(wszIPersistStream)))
        IPersistStreamInterfaceViewer(clsid);

    else if(!memcmp(clsid, wszIPersist, sizeof(wszIPersist)))
        IPersistInterfaceViewer(clsid);

    else DefaultInterfaceViewer(clsid, wszName);
}
