/*
 * Regedit frame window
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cderr.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>

#include "main.h"
#include "regproc.h"

/********************************************************************************
 * Global and Local Variables:
 */

static TCHAR favoritesKey[] =  _T("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\RegEdit\\Favorites");
static BOOL bInMenuLoop = FALSE;        /* Tells us if we are in the menu loop */
static TCHAR favoriteName[128];
static TCHAR searchString[128];
static int searchMask = SEARCH_KEYS | SEARCH_VALUES | SEARCH_CONTENT;

static TCHAR FileNameBuffer[_MAX_PATH];
static TCHAR FileTitleBuffer[_MAX_PATH];
static TCHAR FilterBuffer[_MAX_PATH];

/*******************************************************************************
 * Local module support methods
 */

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
    RECT rt;
    /*
    	if (IsWindowVisible(hToolBar)) {
    		SendMessage(hToolBar, WM_SIZE, 0, 0);
    		GetClientRect(hToolBar, &rt);
    		prect->top = rt.bottom+3;
    		prect->bottom -= rt.bottom+3;
    	}
     */
    if (IsWindowVisible(hStatusBar)) {
        SetupStatusBar(hWnd, TRUE);
        GetClientRect(hStatusBar, &rt);
        prect->bottom -= rt.bottom;
    }
    MoveWindow(g_pChildWnd->hWnd, prect->left, prect->top, prect->right, prect->bottom, TRUE);
}

static void resize_frame_client(HWND hWnd)
{
    RECT rect;

    GetClientRect(hWnd, &rect);
    resize_frame_rect(hWnd, &rect);
}

/********************************************************************************/

static void OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    /* Update the status bar pane sizes */
    nParts = -1;
    SendMessage(hStatusBar, SB_SETPARTS, 1, (long)&nParts);
    bInMenuLoop = TRUE;
    SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)_T(""));
}

static void OnExitMenuLoop(HWND hWnd)
{
    bInMenuLoop = FALSE;
    /* Update the status bar pane sizes*/
    SetupStatusBar(hWnd, TRUE);
    UpdateStatusBar();
}

static void UpdateMenuItems(HMENU hMenu) {
    HWND hwndTV = g_pChildWnd->hTreeWnd;
    BOOL bIsKeySelected = FALSE;
    HKEY hRootKey = NULL;
    LPCTSTR keyName;
    keyName = GetItemPath(hwndTV, TreeView_GetSelection(hwndTV), &hRootKey);
    if (keyName && *keyName) { /* can't modify root keys */
        bIsKeySelected = TRUE;
    }
    EnableMenuItem(hMenu, ID_EDIT_FIND, MF_ENABLED | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_FINDNEXT, MF_ENABLED | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_MODIFY, (bIsKeySelected ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_DELETE, (bIsKeySelected ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_RENAME, (bIsKeySelected ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_FAVORITES_ADDTOFAVORITES, (hRootKey ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_FAVORITES_REMOVEFAVORITE, 
        (GetMenuItemCount(hMenu)>2 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
}

static void OnInitMenuPopup(HWND hWnd, HMENU hMenu, short wItem)
{
    if (wItem == 3) {
        HKEY hKey;
        while(GetMenuItemCount(hMenu)>2)
            DeleteMenu(hMenu, 2, MF_BYPOSITION);
        if (RegOpenKeyEx(HKEY_CURRENT_USER, favoritesKey, 
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            TCHAR namebuf[KEY_MAX_LEN];
            BYTE valuebuf[4096];
            int i = 0;
            BOOL sep = FALSE;
            DWORD ksize, vsize, type;
            LONG error;
            do {
                ksize = KEY_MAX_LEN;
                vsize = sizeof(valuebuf);
                error = RegEnumValue(hKey, i, namebuf, &ksize, NULL, &type, valuebuf, &vsize);
                if (error != ERROR_SUCCESS)
                    break;
                if (type == REG_SZ) {
                    if (!sep) {
                        AppendMenu(hMenu, MF_SEPARATOR, -1, NULL);
                        sep = TRUE;
                    }
                    AppendMenu(hMenu, MF_STRING, ID_FAVORITE_FIRST+i, namebuf);
                }
                i++;
            } while(error == ERROR_SUCCESS);
            RegCloseKey(hKey);
        }
    }
    UpdateMenuItems(hMenu);
}

static void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    TCHAR str[100];

    _tcscpy(str, _T(""));
    if (nFlags & MF_POPUP) {
        if (hSysMenu != GetMenu(hWnd)) {
            if (nItemID == 2) nItemID = 5;
        }
    }
    if (LoadString(hInst, nItemID, str, 100)) {
        /* load appropriate string*/
        LPTSTR lpsz = str;
        /* first newline terminates actual string*/
        lpsz = _tcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)str);
}

void SetupStatusBar(HWND hWnd, BOOL bResize)
{
    RECT  rc;
    int nParts;
    GetClientRect(hWnd, &rc);
    nParts = rc.right;
    /*    nParts = -1;*/
    if (bResize)
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
    SendMessage(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
    UpdateStatusBar();
}

void UpdateStatusBar(void)
{
    LPTSTR fullPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, TRUE);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)fullPath);
    HeapFree(GetProcessHeap(), 0, fullPath);
}

static void toggle_child(HWND hWnd, UINT cmd, HWND hchild)
{
    BOOL vis = IsWindowVisible(hchild);
    HMENU hMenuView = GetSubMenu(hMenuFrame, ID_VIEW_MENU);

    CheckMenuItem(hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);
    ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);
    resize_frame_client(hWnd);
}

static BOOL CheckCommDlgError(HWND hWnd)
{
    DWORD dwErrorCode = CommDlgExtendedError();
    switch (dwErrorCode) {
    case CDERR_DIALOGFAILURE:
        break;
    case CDERR_FINDRESFAILURE:
        break;
    case CDERR_NOHINSTANCE:
        break;
    case CDERR_INITIALIZATION:
        break;
    case CDERR_NOHOOK:
        break;
    case CDERR_LOCKRESFAILURE:
        break;
    case CDERR_NOTEMPLATE:
        break;
    case CDERR_LOADRESFAILURE:
        break;
    case CDERR_STRUCTSIZE:
        break;
    case CDERR_LOADSTRFAILURE:
        break;
    case FNERR_BUFFERTOOSMALL:
        break;
    case CDERR_MEMALLOCFAILURE:
        break;
    case FNERR_INVALIDFILENAME:
        break;
    case CDERR_MEMLOCKFAILURE:
        break;
    case FNERR_SUBCLASSFAILURE:
        break;
    default:
        break;
    }
    return TRUE;
}

static void ExportRegistryFile_StoreSelection(HWND hdlg, OPENFILENAME *pOpenFileName)
{
    if (IsDlgButtonChecked(hdlg, IDC_EXPORT_SELECTED))
    {
        INT len = SendDlgItemMessage(hdlg, IDC_EXPORT_PATH, WM_GETTEXTLENGTH, 0, 0);
        pOpenFileName->lCustData = (LPARAM)HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(TCHAR));
        SendDlgItemMessage(hdlg, IDC_EXPORT_PATH, WM_GETTEXT, len+1, pOpenFileName->lCustData);
    }
    else
        pOpenFileName->lCustData = (LPARAM)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TCHAR));
}

static UINT_PTR CALLBACK ExportRegistryFile_OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    static OPENFILENAME* pOpenFileName;
    OFNOTIFY *pOfNotify;
    LPTSTR path;

    switch (uiMsg) {
    case WM_INITDIALOG:
        pOpenFileName = (OPENFILENAME*)lParam;
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_EXPORT_PATH && HIWORD(wParam) == EN_UPDATE)
            CheckRadioButton(hdlg, IDC_EXPORT_ALL, IDC_EXPORT_SELECTED, IDC_EXPORT_SELECTED);
        break;
    case WM_NOTIFY:
        pOfNotify = (OFNOTIFY*)lParam;
        switch (pOfNotify->hdr.code)
        {
            case CDN_INITDONE:
                path = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
                SendDlgItemMessage(hdlg, IDC_EXPORT_PATH, WM_SETTEXT, 0, (LPARAM)path);
                HeapFree(GetProcessHeap(), 0, path);
                CheckRadioButton(hdlg, IDC_EXPORT_ALL, IDC_EXPORT_SELECTED, IDC_EXPORT_ALL);
                break;
            case CDN_FILEOK:
                ExportRegistryFile_StoreSelection(hdlg, pOpenFileName);
                break;
        }
        break;
    default:
        break;
    }
    return 0L;
}


static BOOL InitOpenFileName(HWND hWnd, OPENFILENAME *pofn)
{
    memset(pofn, 0, sizeof(OPENFILENAME));
    pofn->lStructSize = sizeof(OPENFILENAME);
    pofn->hwndOwner = hWnd;
    pofn->hInstance = hInst;

    if (FilterBuffer[0] == 0)
        LoadString(hInst, IDS_FILEDIALOG_FILTER, FilterBuffer, _MAX_PATH);
    pofn->lpstrFilter = FilterBuffer;
    pofn->nFilterIndex = 1;
    pofn->lpstrFile = FileNameBuffer;
    pofn->nMaxFile = _MAX_PATH;
    pofn->lpstrFileTitle = FileTitleBuffer;
    pofn->nMaxFileTitle = _MAX_PATH;
    pofn->Flags = OFN_HIDEREADONLY;
    /* some other fields may be set by the caller */
    return TRUE;
}

static BOOL ImportRegistryFile(HWND hWnd)
{
    OPENFILENAME ofn;
    TCHAR title[128];

    InitOpenFileName(hWnd, &ofn);
    LoadString(hInst, IDS_FILEDIALOG_IMPORT_TITLE, title, COUNT_OF(title));
    ofn.lpstrTitle = title;
    if (GetOpenFileName(&ofn)) {
        if (!import_registry_file(ofn.lpstrFile)) {
            /*printf("Can't open file \"%s\"\n", ofn.lpstrFile);*/
            return FALSE;
        }
    } else {
        CheckCommDlgError(hWnd);
    }
    return TRUE;
}


static BOOL ExportRegistryFile(HWND hWnd)
{
    OPENFILENAME ofn;
    TCHAR ExportKeyPath[_MAX_PATH];
    TCHAR title[128];

    ExportKeyPath[0] = _T('\0');
    InitOpenFileName(hWnd, &ofn);
    LoadString(hInst, IDS_FILEDIALOG_EXPORT_TITLE, title, COUNT_OF(title));
    ofn.lpstrTitle = title;
    ofn.lCustData = 0; 
    ofn.Flags = OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpfnHook = ExportRegistryFile_OFNHookProc;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_EXPORT_TEMPLATE);
    if (GetSaveFileName(&ofn)) {
        BOOL result;
        result = export_registry_key(ofn.lpstrFile, (LPTSTR)ofn.lCustData);
        if (!result) {
            /*printf("Can't open file \"%s\"\n", ofn.lpstrFile);*/
            return FALSE;
        }
    } else {
        CheckCommDlgError(hWnd);
    }
    return TRUE;
}

static BOOL PrintRegistryHive(HWND hWnd, LPCTSTR path)
{
#if 1
    PRINTDLG pd;

    ZeroMemory(&pd, sizeof(PRINTDLG));
    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner   = hWnd;
    pd.hDevMode    = NULL;     /* Don't forget to free or store hDevMode*/
    pd.hDevNames   = NULL;     /* Don't forget to free or store hDevNames*/
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies     = 1;
    pd.nFromPage   = 0xFFFF;
    pd.nToPage     = 0xFFFF;
    pd.nMinPage    = 1;
    pd.nMaxPage    = 0xFFFF;
    if (PrintDlg(&pd)) {
        /* GDI calls to render output. */
        DeleteDC(pd.hDC); /* Delete DC when done.*/
    }
#else
    HRESULT hResult;
    PRINTDLGEX pd;

    hResult = PrintDlgEx(&pd);
    if (hResult == S_OK) {
        switch (pd.dwResultAction) {
        case PD_RESULT_APPLY:
            /*The user clicked the Apply button and later clicked the Cancel button. This indicates that the user wants to apply the changes made in the property sheet, but does not yet want to print. The PRINTDLGEX structure contains the information specified by the user at the time the Apply button was clicked. */
            break;
        case PD_RESULT_CANCEL:
            /*The user clicked the Cancel button. The information in the PRINTDLGEX structure is unchanged. */
            break;
        case PD_RESULT_PRINT:
            /*The user clicked the Print button. The PRINTDLGEX structure contains the information specified by the user. */
            break;
        default:
            break;
        }
    } else {
        switch (hResult) {
        case E_OUTOFMEMORY:
            /*Insufficient memory. */
            break;
        case E_INVALIDARG:
            /* One or more arguments are invalid. */
            break;
        case E_POINTER:
            /*Invalid pointer. */
            break;
        case E_HANDLE:
            /*Invalid handle. */
            break;
        case E_FAIL:
            /*Unspecified error. */
            break;
        default:
            break;
        }
        return FALSE;
    }
#endif
    return TRUE;
}

static BOOL CopyKeyName(HWND hWnd, LPCTSTR keyName)
{
    BOOL result;

    result = OpenClipboard(hWnd);
    if (result) {
        result = EmptyClipboard();
        if (result) {
            int len = (_tcslen(keyName)+1)*sizeof(TCHAR);
            HANDLE hClipData = GlobalAlloc(GHND, len);
            LPVOID pLoc = GlobalLock(hClipData);
            _tcscpy(pLoc, keyName);
            GlobalUnlock(hClipData);
            hClipData = SetClipboardData(CF_TEXT, hClipData);

        } else {
            /* error emptying clipboard*/
            /* DWORD dwError = GetLastError(); */
            ;
        }
        if (!CloseClipboard()) {
            /* error closing clipboard*/
            /* DWORD dwError = GetLastError(); */
            ;
        }
    } else {
        /* error opening clipboard*/
        /* DWORD dwError = GetLastError(); */
        ;
    }
    return result;
}

static INT_PTR CALLBACK find_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_NAME);
            
    switch(uMsg) {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            CheckDlgButton(hwndDlg, IDC_FIND_KEYS, searchMask&SEARCH_KEYS ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_FIND_VALUES, searchMask&SEARCH_VALUES ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_FIND_CONTENT, searchMask&SEARCH_CONTENT ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_FIND_WHOLE, searchMask&SEARCH_WHOLE ? BST_CHECKED : BST_UNCHECKED);
            SendMessage(hwndValue, EM_SETLIMITTEXT, 127, 0);
            SetWindowText(hwndValue, searchString);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_VALUE_NAME:
                if (HIWORD(wParam) == EN_UPDATE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK),  GetWindowTextLength(hwndValue)>0);
                    return TRUE;
                }
                break;
            case IDOK:
                if (GetWindowTextLength(hwndValue)>0) {
                    int mask = 0;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_KEYS)) mask |= SEARCH_KEYS;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_VALUES)) mask |= SEARCH_VALUES;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_CONTENT)) mask |= SEARCH_CONTENT;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_WHOLE)) mask |= SEARCH_WHOLE;
                    searchMask = mask;
                    GetWindowText(hwndValue, searchString, 128);
                    EndDialog(hwndDlg, IDOK);
                }
                return TRUE;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}
                    
static INT_PTR CALLBACK addtofavorites_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_NAME);
            
    switch(uMsg) {
        case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            SendMessage(hwndValue, EM_SETLIMITTEXT, 127, 0);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_VALUE_NAME:
                if (HIWORD(wParam) == EN_UPDATE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK),  GetWindowTextLength(hwndValue)>0);
                    return TRUE;
                }
                break;
            case IDOK:
                if (GetWindowTextLength(hwndValue)>0) {
                    GetWindowText(hwndValue, favoriteName, 128);
                    EndDialog(hwndDlg, IDOK);
                }
                return TRUE;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}
                    
static INT_PTR CALLBACK removefavorite_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_NAME_LIST);
            
    switch(uMsg) {
        case WM_INITDIALOG: {
            HKEY hKey;
            int i = 0;
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            if (RegOpenKeyEx(HKEY_CURRENT_USER, favoritesKey, 
                0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                TCHAR namebuf[KEY_MAX_LEN];
                BYTE valuebuf[4096];
                DWORD ksize, vsize, type;
                LONG error;
                do {
                    ksize = KEY_MAX_LEN;
                    vsize = sizeof(valuebuf);
                    error = RegEnumValue(hKey, i, namebuf, &ksize, NULL, &type, valuebuf, &vsize);
                    if (error != ERROR_SUCCESS)
                        break;
                    if (type == REG_SZ) {
                        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)namebuf);
                    }
                    i++;
                } while(error == ERROR_SUCCESS);
                RegCloseKey(hKey);
            }
            else
                return FALSE;
            EnableWindow(GetDlgItem(hwndDlg, IDOK), i != 0);
            SendMessage(hwndList, LB_SETCURSEL, 0, 0);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_NAME_LIST:
                if (HIWORD(wParam) == LBN_SELCHANGE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK),  lParam != -1);
                    return TRUE;
                }
                break;
            case IDOK: {
                int pos = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
                int len = SendMessage(hwndList, LB_GETTEXTLEN, pos, 0);
                if (len>0) {
                    LPTSTR lpName = HeapAlloc(GetProcessHeap(), 0, sizeof(TCHAR)*(len+1));
                    SendMessage(hwndList, LB_GETTEXT, pos, (LPARAM)lpName);
                    if (len>127)
                        lpName[127] = '\0';
                    _tcscpy(favoriteName, lpName);
                    EndDialog(hwndDlg, IDOK);
                    HeapFree(GetProcessHeap(), 0, lpName);
                }
                return TRUE;
            }
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}
                    
/*******************************************************************************
 *
 *  FUNCTION: _CmdWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes WM_COMMAND messages for the main frame window.
 *
 */
static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HKEY hKeyRoot = 0;
    LPCTSTR keyPath;
    LPCTSTR valueName;
    TCHAR newKey[MAX_NEW_KEY_LEN];
    DWORD valueType;

    keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
    valueName = GetValueName(g_pChildWnd->hListWnd);

    if (LOWORD(wParam) >= ID_FAVORITE_FIRST && LOWORD(wParam) <= ID_FAVORITE_LAST) {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, favoritesKey, 
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            TCHAR namebuf[KEY_MAX_LEN];
            BYTE valuebuf[4096];
            DWORD ksize = KEY_MAX_LEN, vsize = sizeof(valuebuf), type = 0;
            if (RegEnumValue(hKey, LOWORD(wParam) - ID_FAVORITE_FIRST, namebuf, &ksize, NULL, 
                &type, valuebuf, &vsize) == ERROR_SUCCESS) {
                SendMessage( g_pChildWnd->hTreeWnd, TVM_SELECTITEM, TVGN_CARET,
                             (LPARAM) FindPathInTree(g_pChildWnd->hTreeWnd, (TCHAR *)valuebuf) );
            }
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    switch (LOWORD(wParam)) {
    case ID_REGISTRY_IMPORTREGISTRYFILE:
        ImportRegistryFile(hWnd);
        break;
    case ID_REGISTRY_EXPORTREGISTRYFILE:
        ExportRegistryFile(hWnd);
        break;
    case ID_REGISTRY_CONNECTNETWORKREGISTRY:
        break;
    case ID_REGISTRY_DISCONNECTNETWORKREGISTRY:
        break;
    case ID_REGISTRY_PRINT:
        PrintRegistryHive(hWnd, _T(""));
        break;
    case ID_EDIT_DELETE:
	if (GetFocus() == g_pChildWnd->hTreeWnd) {
	    if (keyPath == 0 || *keyPath == 0) {
	        MessageBeep(MB_ICONHAND); 
            } else if (DeleteKey(hWnd, hKeyRoot, keyPath)) {
		DeleteNode(g_pChildWnd->hTreeWnd, 0);
            }
	} else if (GetFocus() == g_pChildWnd->hListWnd) {
	    if (DeleteValue(hWnd, hKeyRoot, keyPath, valueName))
		RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, NULL);
	}
        break;
    case ID_EDIT_MODIFY:
        if (ModifyValue(hWnd, hKeyRoot, keyPath, valueName))
            RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, valueName);
        break;
    case ID_EDIT_FIND:
    case ID_EDIT_FINDNEXT:
    {
        HTREEITEM hItem;
        if (LOWORD(wParam) == ID_EDIT_FIND &&
            DialogBox(0, MAKEINTRESOURCE(IDD_FIND), hWnd, find_dlgproc) != IDOK)
            break;
        if (!*searchString)
            break;
        hItem = TreeView_GetSelection(g_pChildWnd->hTreeWnd);
        if (hItem) {
            int row = ListView_GetNextItem(g_pChildWnd->hListWnd, -1, LVNI_FOCUSED);
            HCURSOR hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
            hItem = FindNext(g_pChildWnd->hTreeWnd, hItem, searchString, searchMask, &row);
            SetCursor(hcursorOld);
            if (hItem) {
                SendMessage( g_pChildWnd->hTreeWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM) hItem );
                InvalidateRect(g_pChildWnd->hTreeWnd, NULL, TRUE);
                UpdateWindow(g_pChildWnd->hTreeWnd);
                if (row != -1) {
                    ListView_SetItemState(g_pChildWnd->hListWnd, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
                    ListView_SetItemState(g_pChildWnd->hListWnd, row, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
                    SetFocus(g_pChildWnd->hListWnd);
                } else {
                    SetFocus(g_pChildWnd->hTreeWnd);
                }
            } else {
                error(hWnd, IDS_NOTFOUND, searchString);
            }
        }
        break;
    }
    case ID_EDIT_COPYKEYNAME:
    {
        LPTSTR fullPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
        if (fullPath) {
            CopyKeyName(hWnd, fullPath);
            HeapFree(GetProcessHeap(), 0, fullPath);
        }
        break;
    }
    case ID_EDIT_NEW_KEY:
	if (CreateKey(hWnd, hKeyRoot, keyPath, newKey)) {
	    if (InsertNode(g_pChildWnd->hTreeWnd, 0, newKey))
	        StartKeyRename(g_pChildWnd->hTreeWnd);
	}
	break;
    case ID_EDIT_NEW_STRINGVALUE:
	valueType = REG_SZ;
	goto create_value;
    case ID_EDIT_NEW_MULTI_STRINGVALUE:
	valueType = REG_MULTI_SZ;
	goto create_value;
    case ID_EDIT_NEW_BINARYVALUE:
	valueType = REG_BINARY;
	goto create_value;
    case ID_EDIT_NEW_DWORDVALUE:
	valueType = REG_DWORD;
	/* fall through */
    create_value:
	if (CreateValue(hWnd, hKeyRoot, keyPath, valueType, newKey)) {
	    RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, newKey);
            StartValueRename(g_pChildWnd->hListWnd);
	    /* FIXME: start rename */
	}
	break;
    case ID_EDIT_RENAME:
	if (keyPath == 0 || *keyPath == 0) {
	    MessageBeep(MB_ICONHAND); 
	} else if (GetFocus() == g_pChildWnd->hTreeWnd) {
	    StartKeyRename(g_pChildWnd->hTreeWnd);
	} else if (GetFocus() == g_pChildWnd->hListWnd) {
	    StartValueRename(g_pChildWnd->hListWnd);
	}
	break;
    break;
    case ID_REGISTRY_PRINTERSETUP:
        /*PRINTDLG pd;*/
        /*PrintDlg(&pd);*/
        /*PAGESETUPDLG psd;*/
        /*PageSetupDlg(&psd);*/
        break;
    case ID_REGISTRY_OPENLOCAL:
        break;
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_FAVORITES_ADDTOFAVORITES:
    {
    	HKEY hKey;
    	LPTSTR lpKeyPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
    	if (lpKeyPath) {
            if (DialogBox(0, MAKEINTRESOURCE(IDD_ADDFAVORITE), hWnd, addtofavorites_dlgproc) == IDOK) {
                if (RegCreateKeyEx(HKEY_CURRENT_USER, favoritesKey, 
                    0, NULL, 0, 
                    KEY_READ|KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    RegSetValueEx(hKey, favoriteName, 0, REG_SZ, (BYTE *)lpKeyPath, (_tcslen(lpKeyPath)+1)*sizeof(TCHAR));
                    RegCloseKey(hKey);
                }
            }
            HeapFree(GetProcessHeap(), 0, lpKeyPath);
        }
        break;
    }
    case ID_FAVORITES_REMOVEFAVORITE:
    {
        if (DialogBox(0, MAKEINTRESOURCE(IDD_DELFAVORITE), hWnd, removefavorite_dlgproc) == IDOK) {
            HKEY hKey;
            if (RegOpenKeyEx(HKEY_CURRENT_USER, favoritesKey, 
                0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                RegDeleteValue(hKey, favoriteName); 
                RegCloseKey(hKey);
            }
        }
        break;
    }
    case ID_VIEW_REFRESH:
        RefreshTreeView(g_pChildWnd->hTreeWnd);
        RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, NULL);
        break;
   /*case ID_OPTIONS_TOOLBAR:*/
   /*	toggle_child(hWnd, LOWORD(wParam), hToolBar);*/
   /*    break;*/
    case ID_VIEW_STATUSBAR:
        toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;
    case ID_HELP_HELPTOPICS:
        WinHelp(hWnd, _T("regedit"), HELP_FINDER, 0);
        break;
    case ID_HELP_ABOUT:
        ShowAboutBox(hWnd);
        break;
    case ID_VIEW_SPLIT: {
        RECT rt;
        POINT pt, pts;
        GetClientRect(g_pChildWnd->hWnd, &rt);
        pt.x = rt.left + g_pChildWnd->nSplitPos;
        pt.y = (rt.bottom / 2);
        pts = pt;
        if(ClientToScreen(g_pChildWnd->hWnd, &pts)) {
            SetCursorPos(pts.x, pts.y);
            SetCursor(LoadCursor(0, IDC_SIZEWE));
            SendMessage(g_pChildWnd->hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
        }
        return TRUE;
    }
    default:
        return FALSE;
    }

    return TRUE;
}

/********************************************************************************
 *
 *  FUNCTION: FrameWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes messages for the main frame window.
 *
 *  WM_COMMAND  - process the application menu
 *  WM_DESTROY  - post a quit message and return
 *
 */

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        CreateWindowEx(0, szChildClass, _T("regedit child window"), WS_CHILD | WS_VISIBLE,
                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                       hWnd, NULL, hInst, 0);
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam))
            return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_ACTIVATE:
        if (LOWORD(hWnd)) 
            SetFocus(g_pChildWnd->hWnd);
        break;
    case WM_SIZE:
        resize_frame_client(hWnd);
        break;
    case WM_TIMER:
        break;
    case WM_ENTERMENULOOP:
        OnEnterMenuLoop(hWnd);
        break;
    case WM_EXITMENULOOP:
        OnExitMenuLoop(hWnd);
        break;
    case WM_INITMENUPOPUP:
        if (!HIWORD(lParam))
            OnInitMenuPopup(hWnd, (HMENU)wParam, LOWORD(lParam));
        break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    case WM_DESTROY:
        WinHelp(hWnd, _T("regedit"), HELP_QUIT, 0);
        PostQuitMessage(0);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
