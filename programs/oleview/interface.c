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

static IUnknown *GetInterface(void)
{
    HTREEITEM hSelect;
    TVITEM tvi;
    CLSID clsid;
    IUnknown *unk;

    hSelect = (HTREEITEM)SendMessageW(globals.hTree, TVM_GETNEXTITEM,
            TVGN_CARET, 0);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = hSelect;
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    CLSIDFromString(((ITEM_INFO *)tvi.lParam)->clsid, &clsid);

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = (HTREEITEM)SendMessageW(globals.hTree, TVM_GETNEXTITEM,
            TVGN_PARENT, (LPARAM)hSelect);
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    IUnknown_QueryInterface(((ITEM_INFO *)tvi.lParam)->pU, &clsid, (void *)&unk);

    return unk;
}

static INT_PTR CALLBACK InterfaceViewerProc(HWND hDlgWnd, UINT uMsg,
        WPARAM wParam, LPARAM lParam)
{
    DIALOG_INFO *di;
    HWND hObject;
    IUnknown *unk;
    HRESULT hRes;
    ULARGE_INTEGER size;
    WCHAR wszSize[MAX_LOAD_STRING];
    WCHAR wszBuf[MAX_LOAD_STRING];
    WCHAR wszFormat[] = { '%','d',' ','%','s','\0' };

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
                if(hRes == S_OK)
                    LoadString(globals.hMainInst, IDS_FALSE, wszBuf,
                            sizeof(wszBuf)/sizeof(wszBuf[0]));
                else LoadString(globals.hMainInst, IDS_TRUE, wszBuf,
                        sizeof(wszBuf)/sizeof(wszBuf[0]));
                hObject = GetDlgItem(hDlgWnd, IDC_ISDIRTY);
                SetWindowText(hObject, wszBuf);
                return TRUE;
            case IDC_GETSIZEMAX_BUTTON:
                unk = GetInterface();
                IPersistStream_GetSizeMax((IPersistStream *)unk, &size);
                IUnknown_Release(unk);
                LoadString(globals.hMainInst, IDS_BYTES, wszBuf,
                        sizeof(wszBuf)/sizeof(wszBuf[0]));
                wsprintfW(wszSize, wszFormat, U(size).LowPart, wszBuf);
                hObject = GetDlgItem(hDlgWnd, IDC_GETSIZEMAX);
                SetWindowText(hObject, wszSize);
                return TRUE;
            }
    }
    return FALSE;
}

static void IPersistStreamInterfaceViewer(WCHAR *clsid, WCHAR *wszName)
{
    DIALOG_INFO di;
    WCHAR wszClassMoniker[] = { 'C','l','a','s','s','M','o','n','i','k','e','r','\0' };

    if(wszName[0] == '{')
        di.wszLabel = wszClassMoniker;
    else di.wszLabel = wszName;
    di.wszIdentifier = clsid;

    DialogBoxParam(0, MAKEINTRESOURCE(DLG_IPERSISTSTREAM_IV),
            globals.hMainWnd, InterfaceViewerProc, (LPARAM)&di);
}

static void IPersistInterfaceViewer(WCHAR *clsid, WCHAR *wszName)
{
    DIALOG_INFO di;
    WCHAR wszClassMoniker[] = { 'C','l','a','s','s','M','o','n','i','k','e','r','\0' };

    if(wszName[0] == '{')
        di.wszLabel = wszClassMoniker;
    else di.wszLabel = wszName;
    di.wszIdentifier = clsid;

    DialogBoxParam(0, MAKEINTRESOURCE(DLG_IPERSIST_IV),
            globals.hMainWnd, InterfaceViewerProc, (LPARAM)&di);
}

static void DefaultInterfaceViewer(WCHAR *clsid, WCHAR *wszName)
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
    WCHAR wszParent[MAX_LOAD_STRING];
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

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_TEXT;
    tvi.hItem = (HTREEITEM)SendMessageW(globals.hTree, TVM_GETNEXTITEM,
            TVGN_PARENT, (LPARAM)item);
    tvi.cchTextMax = MAX_LOAD_STRING;
    tvi.pszText = wszParent;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    if(!memcmp(clsid, wszIPersistStream, sizeof(wszIPersistStream)))
        IPersistStreamInterfaceViewer(clsid, wszParent);

    else if(!memcmp(clsid, wszIPersist, sizeof(wszIPersist)))
        IPersistInterfaceViewer(clsid, wszParent);

    else DefaultInterfaceViewer(clsid, wszName);
}
