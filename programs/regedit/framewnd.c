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
#include <commctrl.h>
#include <commdlg.h>
#include <cderr.h>
#include <shellapi.h>

#include "main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(regedit);

/********************************************************************************
 * Global and Local Variables:
 */

static const WCHAR favoritesKey[] =  L"Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit\\Favorites";
static BOOL bInMenuLoop = FALSE;        /* Tells us if we are in the menu loop */
static WCHAR favoriteName[128];
static WCHAR searchString[128];
static int searchMask = SEARCH_KEYS | SEARCH_VALUES | SEARCH_CONTENT;

static WCHAR FileNameBuffer[_MAX_PATH];
static WCHAR FileTitleBuffer[_MAX_PATH];
static WCHAR FilterBuffer[_MAX_PATH];
static WCHAR expandW[32], collapseW[32];
static WCHAR modifyW[32], modify_binaryW[64];

/*******************************************************************************
 * Local module support methods
 */

static void resize_frame_rect(HWND hWnd, PRECT prect)
{
    if (IsWindowVisible(hStatusBar)) {
        RECT rt;

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
    WCHAR empty = 0;

    /* Update the status bar pane sizes */
    nParts = -1;
    SendMessageW(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
    bInMenuLoop = TRUE;
    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)&empty);
}

static void OnExitMenuLoop(HWND hWnd)
{
    bInMenuLoop = FALSE;
    /* Update the status bar pane sizes*/
    SetupStatusBar(hWnd, TRUE);
    UpdateStatusBar();
}

static void update_expand_or_collapse_item(HWND hwndTV, HTREEITEM selection, HMENU hMenu)
{
    TVITEMW item;
    MENUITEMINFOW info;

    item.hItem = selection;
    item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_STATE;
    item.stateMask = TVIS_EXPANDED;
    SendMessageW(hwndTV, TVM_GETITEMW, 0, (LPARAM)&item);

    info.cbSize = sizeof(MENUITEMINFOW);
    info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE;
    info.fType = MFT_STRING;
    info.fState = MFS_ENABLED;
    info.dwTypeData = expandW;

    if (!item.cChildren)
    {
        info.fState = MFS_GRAYED;
        goto update;
    }

    if (item.state & TVIS_EXPANDED)
        info.dwTypeData = collapseW;

update:
    SetMenuItemInfoW(hMenu, ID_TREE_EXPAND_COLLAPSE, FALSE, &info);
}

static void update_modify_items(HMENU hMenu, int index)
{
    unsigned int state = MF_ENABLED;

    if (index == -1)
        state = MF_GRAYED;

    EnableMenuItem(hMenu, ID_EDIT_MODIFY, state | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_MODIFY_BIN, state | MF_BYCOMMAND);
}

static void update_delete_and_rename_items(HMENU hMenu, WCHAR *keyName, int index)
{
    unsigned int state_d = MF_ENABLED, state_r = MF_ENABLED;

    if (!g_pChildWnd->nFocusPanel)
    {
        if (!keyName || !*keyName)
            state_d = state_r = MF_GRAYED;
    }
    else if (index < 1)
    {
        state_r = MF_GRAYED;
        if (index == -1) state_d = MF_GRAYED;
    }

    EnableMenuItem(hMenu, ID_EDIT_DELETE, state_d | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_EDIT_RENAME, state_r | MF_BYCOMMAND);
}

static void update_new_items_and_copy_keyname(HMENU hMenu, WCHAR *keyName)
{
    unsigned int state = MF_ENABLED, i;
    unsigned int items[] = {ID_EDIT_NEW_KEY, ID_EDIT_NEW_STRINGVALUE, ID_EDIT_NEW_BINARYVALUE,
                            ID_EDIT_NEW_DWORDVALUE, ID_EDIT_NEW_QWORDVALUE, ID_EDIT_NEW_MULTI_STRINGVALUE,
                            ID_EDIT_NEW_EXPANDVALUE, ID_EDIT_COPYKEYNAME};

    if (!keyName)
        state = MF_GRAYED;

    for (i = 0; i < ARRAY_SIZE(items); i++)
        EnableMenuItem(hMenu, items[i], state | MF_BYCOMMAND);
}

static void UpdateMenuItems(HMENU hMenu) {
    HWND hwndTV = g_pChildWnd->hTreeWnd;
    HKEY hRootKey = NULL;
    LPWSTR keyName;
    HTREEITEM selection;
    int index;

    selection = (HTREEITEM)SendMessageW(hwndTV, TVM_GETNEXTITEM, TVGN_CARET, 0);
    keyName = GetItemPath(hwndTV, selection, &hRootKey);
    index = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1,
                         MAKELPARAM(LVNI_SELECTED, 0));

    update_expand_or_collapse_item(hwndTV, selection, hMenu);
    update_modify_items(hMenu, index);
    update_delete_and_rename_items(hMenu, keyName, index);
    update_new_items_and_copy_keyname(hMenu, keyName);
    EnableMenuItem(hMenu, ID_FAVORITES_ADDTOFAVORITES, (hRootKey ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    EnableMenuItem(hMenu, ID_FAVORITES_REMOVEFAVORITE, 
        (GetMenuItemCount(hMenu)>2 ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);

    free(keyName);
}

static void add_remove_modify_menu_items(HMENU hMenu)
{
    if (!g_pChildWnd->nFocusPanel)
    {
        while (GetMenuItemCount(hMenu) > 9)
            DeleteMenu(hMenu, 0, MF_BYPOSITION);
    }
    else if (GetMenuItemCount(hMenu) < 10)
    {
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_EDIT_MODIFY_BIN, modify_binaryW);
        InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_EDIT_MODIFY, modifyW);
    }
}

static int add_favourite_key_items(HMENU hMenu, HWND hList)
{
    HKEY hkey;
    LONG rc;
    DWORD num_values, max_value_len, value_len, type, i = 0;
    WCHAR *value_name;

    rc = RegOpenKeyExW(HKEY_CURRENT_USER, favoritesKey, 0, KEY_READ, &hkey);
    if (rc != ERROR_SUCCESS) return 0;

    rc = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &num_values,
                          &max_value_len, NULL, NULL, NULL);
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegQueryInfoKey failed: %ld\n", rc);
        goto exit;
    }

    if (!num_values) goto exit;

    max_value_len++;
    value_name = malloc(max_value_len * sizeof(WCHAR));

    if (hMenu) AppendMenuW(hMenu, MF_SEPARATOR, 0, 0);

    for (i = 0; i < num_values; i++)
    {
        value_len = max_value_len;
        rc = RegEnumValueW(hkey, i, value_name, &value_len, NULL, &type, NULL, NULL);
        if (rc == ERROR_SUCCESS && type == REG_SZ)
        {
            if (hMenu)
                AppendMenuW(hMenu, MF_ENABLED | MF_STRING, ID_FAVORITE_FIRST + i, value_name);
            else if (hList)
                SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)value_name);
        }
    }

    free(value_name);
exit:
    RegCloseKey(hkey);
    return i;
}

static void OnInitMenuPopup(HWND hWnd, HMENU hMenu)
{
    if (hMenu == GetSubMenu(hMenuFrame, ID_EDIT_MENU))
        add_remove_modify_menu_items(hMenu);
    else if (hMenu == GetSubMenu(hMenuFrame, ID_FAVORITES_MENU))
    {
        while (GetMenuItemCount(hMenu) > 2)
            DeleteMenu(hMenu, 2, MF_BYPOSITION);

        add_favourite_key_items(hMenu, NULL);
    }

    UpdateMenuItems(hMenu);
}

static void OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    WCHAR str[100];

    str[0] = 0;
    if (nFlags & MF_POPUP) {
        if (hSysMenu != GetMenu(hWnd)) {
            if (nItemID == 2) nItemID = 5;
        }
    }
    if (LoadStringW(hInst, nItemID, str, 100)) {
        /* load appropriate string*/
        LPWSTR lpsz = str;
        /* first newline terminates actual string*/
        lpsz = wcschr(lpsz, '\n');
        if (lpsz != NULL)
            *lpsz = '\0';
    }
    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)str);
}

void SetupStatusBar(HWND hWnd, BOOL bResize)
{
    RECT  rc;
    int nParts;
    GetClientRect(hWnd, &rc);
    nParts = rc.right;
    /*    nParts = -1;*/
    if (bResize)
        SendMessageW(hStatusBar, WM_SIZE, 0, 0);
    SendMessageW(hStatusBar, SB_SETPARTS, 1, (LPARAM)&nParts);
    UpdateStatusBar();
}

void UpdateStatusBar(void)
{
    LPWSTR fullPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, TRUE);
    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)fullPath);
    free(fullPath);
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

static void ExportRegistryFile_StoreSelection(HWND hdlg, OPENFILENAMEW *pOpenFileName)
{
    if (IsDlgButtonChecked(hdlg, IDC_EXPORT_SELECTED))
    {
        INT len = SendDlgItemMessageW(hdlg, IDC_EXPORT_PATH, WM_GETTEXTLENGTH, 0, 0);
        pOpenFileName->lCustData = (LPARAM)malloc((len + 1) * sizeof(WCHAR));
        SendDlgItemMessageW(hdlg, IDC_EXPORT_PATH, WM_GETTEXT, len+1, pOpenFileName->lCustData);
    }
    else
    {
        pOpenFileName->lCustData = (LPARAM)malloc(sizeof(WCHAR));
        *(WCHAR *)pOpenFileName->lCustData = 0;
    }
}

static UINT_PTR CALLBACK ExportRegistryFile_OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    static OPENFILENAMEW* pOpenFileName;
    OFNOTIFYW *pOfNotify;

    switch (uiMsg) {
    case WM_INITDIALOG:
        pOpenFileName = (OPENFILENAMEW*)lParam;
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_EXPORT_PATH && HIWORD(wParam) == EN_UPDATE)
            CheckRadioButton(hdlg, IDC_EXPORT_ALL, IDC_EXPORT_SELECTED, IDC_EXPORT_SELECTED);
        break;
    case WM_NOTIFY:
        pOfNotify = (OFNOTIFYW*)lParam;
        switch (pOfNotify->hdr.code)
        {
            case CDN_INITDONE:
            {
                BOOL export_branch = FALSE;
                WCHAR* path = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
                SendDlgItemMessageW(hdlg, IDC_EXPORT_PATH, WM_SETTEXT, 0, (LPARAM)path);
                if (path && path[0])
                    export_branch = TRUE;
                free(path);
                CheckRadioButton(hdlg, IDC_EXPORT_ALL, IDC_EXPORT_SELECTED, export_branch ? IDC_EXPORT_SELECTED : IDC_EXPORT_ALL);
                break;
            }
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


static BOOL InitOpenFileName(HWND hWnd, OPENFILENAMEW *pofn)
{
    memset(pofn, 0, sizeof(OPENFILENAMEW));
    pofn->lStructSize = sizeof(OPENFILENAMEW);
    pofn->hwndOwner = hWnd;
    pofn->hInstance = hInst;

    if (FilterBuffer[0] == 0)
    {
        WCHAR filter_reg[MAX_PATH], filter_reg4[MAX_PATH], filter_all[MAX_PATH];

        LoadStringW(hInst, IDS_FILEDIALOG_FILTER_REG, filter_reg, MAX_PATH);
        LoadStringW(hInst, IDS_FILEDIALOG_FILTER_REG4, filter_reg4, MAX_PATH);
        LoadStringW(hInst, IDS_FILEDIALOG_FILTER_ALL, filter_all, MAX_PATH);
        swprintf( FilterBuffer, ARRAY_SIZE(FilterBuffer), L"%s%c*.reg%c%s%c*.reg%c%s%c*.*%c",
                  filter_reg, 0, 0, filter_reg4, 0, 0, filter_all, 0, 0 );
    }
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

static BOOL import_registry_filename(LPWSTR filename)
{
    BOOL Success;
    FILE* reg_file = _wfopen(filename, L"rb");

    if(!reg_file)
        return FALSE;

    Success = import_registry_file(reg_file);

    if(fclose(reg_file) != 0)
        Success = FALSE;

    return Success;
}

static BOOL ImportRegistryFile(HWND hWnd)
{
    OPENFILENAMEW ofn;
    WCHAR title[128];
    HKEY root_key = NULL;
    WCHAR *key_path;

    InitOpenFileName(hWnd, &ofn);
    ofn.Flags |= OFN_ENABLESIZING;
    LoadStringW(hInst, IDS_FILEDIALOG_IMPORT_TITLE, title, ARRAY_SIZE(title));
    ofn.lpstrTitle = title;
    if (GetOpenFileNameW(&ofn)) {
        if (!import_registry_filename(ofn.lpstrFile)) {
            messagebox(hWnd, MB_OK|MB_ICONERROR, IDS_APP_TITLE, IDS_IMPORT_FAILED, ofn.lpstrFile);
            return FALSE;
        } else {
            messagebox(hWnd, MB_OK|MB_ICONINFORMATION, IDS_APP_TITLE,
                       IDS_IMPORT_SUCCESSFUL, ofn.lpstrFile);
        }
    } else {
        CheckCommDlgError(hWnd);
    }
    RefreshTreeView(g_pChildWnd->hTreeWnd);

    key_path = GetItemPath(g_pChildWnd->hTreeWnd, 0, &root_key);
    RefreshListView(g_pChildWnd->hListWnd, root_key, key_path, NULL);
    free(key_path);

    return TRUE;
}


static BOOL ExportRegistryFile(HWND hWnd)
{
    OPENFILENAMEW ofn;
    WCHAR title[128];

    InitOpenFileName(hWnd, &ofn);
    LoadStringW(hInst, IDS_FILEDIALOG_EXPORT_TITLE, title, ARRAY_SIZE(title));
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = L"reg";
    ofn.Flags = OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpfnHook = ExportRegistryFile_OFNHookProc;
    ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_EXPORT_TEMPLATE);
    if (GetSaveFileNameW(&ofn)) {
        BOOL result;
        result = export_registry_key(ofn.lpstrFile, (LPWSTR)ofn.lCustData, ofn.nFilterIndex);
        if (!result) {
            FIXME("Registry export failed.\n");
            return FALSE;
        }
    } else {
        CheckCommDlgError(hWnd);
    }
    return TRUE;
}

static BOOL PrintRegistryHive(HWND hWnd, LPCWSTR path)
{
#if 1
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(PRINTDLGW));
    pd.lStructSize = sizeof(PRINTDLGW);
    pd.hwndOwner   = hWnd;
    pd.hDevMode    = NULL;     /* Don't forget to free or store hDevMode*/
    pd.hDevNames   = NULL;     /* Don't forget to free or store hDevNames*/
    pd.Flags       = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
    pd.nCopies     = 1;
    pd.nFromPage   = 0xFFFF;
    pd.nToPage     = 0xFFFF;
    pd.nMinPage    = 1;
    pd.nMaxPage    = 0xFFFF;
    if (PrintDlgW(&pd)) {
        FIXME("printing is not yet implemented.\n");
        /* GDI calls to render output. */
        DeleteDC(pd.hDC); /* Delete DC when done.*/
    }
#else
    HRESULT hResult;
    PRINTDLGEXW pd;

    hResult = PrintDlgExW(&pd);
    if (hResult == S_OK) {
        switch (pd.dwResultAction) {
        case PD_RESULT_APPLY:
            /*The user clicked the Apply button and later clicked the Cancel button. This indicates that the user wants to apply the changes made in the property sheet, but does not yet want to print. The PRINTDLGEX structure contains the information specified by the user at the time the Apply button was clicked. */
            FIXME("printing is not yet implemented.\n");
            break;
        case PD_RESULT_CANCEL:
            /*The user clicked the Cancel button. The information in the PRINTDLGEX structure is unchanged. */
            break;
        case PD_RESULT_PRINT:
            FIXME("printing is not yet implemented.\n");
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

static BOOL CopyKeyName(HWND hWnd, LPCWSTR keyName)
{
    BOOL result;

    result = OpenClipboard(hWnd);
    if (result) {
        result = EmptyClipboard();
        if (result) {
            int len = (lstrlenW(keyName)+1)*sizeof(WCHAR);
            HANDLE hClipData = GlobalAlloc(GHND, len);
            LPVOID pLoc = GlobalLock(hClipData);
            lstrcpyW(pLoc, keyName);
            GlobalUnlock(hClipData);
            SetClipboardData(CF_UNICODETEXT, hClipData);

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
            SendMessageW(hwndValue, EM_SETLIMITTEXT, 127, 0);
            SetWindowTextW(hwndValue, searchString);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_VALUE_NAME:
                if (HIWORD(wParam) == EN_UPDATE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK),  GetWindowTextLengthW(hwndValue)>0);
                    return TRUE;
                }
                break;
            case IDOK:
                if (GetWindowTextLengthW(hwndValue)>0) {
                    int mask = 0;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_KEYS)) mask |= SEARCH_KEYS;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_VALUES)) mask |= SEARCH_VALUES;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_CONTENT)) mask |= SEARCH_CONTENT;
                    if (IsDlgButtonChecked(hwndDlg, IDC_FIND_WHOLE)) mask |= SEARCH_WHOLE;
                    searchMask = mask;
                    GetWindowTextW(hwndValue, searchString, 128);
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
        {
            HTREEITEM selected;
            TVITEMW item;
            WCHAR buf[128];

            selected = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_CARET, 0);

            item.mask = TVIF_HANDLE | TVIF_TEXT;
            item.hItem = selected;
            item.pszText = buf;
            item.cchTextMax = ARRAY_SIZE(buf);
            SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETITEMW, 0, (LPARAM)&item);

            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            SetWindowTextW(hwndValue, buf);
            SendMessageW(hwndValue, EM_SETLIMITTEXT, 127, 0);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_VALUE_NAME:
                if (HIWORD(wParam) == EN_UPDATE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), GetWindowTextLengthW(hwndValue) > 0);
                    return TRUE;
                }
                break;
            case IDOK:
                if (GetWindowTextLengthW(hwndValue)>0) {
                    GetWindowTextW(hwndValue, favoriteName, 128);
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
        case WM_INITDIALOG:
            if (!add_favourite_key_items(NULL, hwndList))
                return FALSE;
            SendMessageW(hwndList, LB_SETCURSEL, 0, 0);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
            case IDC_NAME_LIST:
                if (HIWORD(wParam) == LBN_SELCHANGE) {
                    EnableWindow(GetDlgItem(hwndDlg, IDOK),  lParam != -1);
                    return TRUE;
                }
                break;
            case IDOK: {
                int pos = SendMessageW(hwndList, LB_GETCURSEL, 0, 0);
                int len = SendMessageW(hwndList, LB_GETTEXTLEN, pos, 0);
                if (len>0) {
                    WCHAR *lpName = malloc((len + 1) * sizeof(WCHAR));
                    SendMessageW(hwndList, LB_GETTEXT, pos, (LPARAM)lpName);
                    if (len>127)
                        lpName[127] = '\0';
                    lstrcpyW(favoriteName, lpName);
                    EndDialog(hwndDlg, IDOK);
                    free(lpName);
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
    DWORD valueType;

    if (LOWORD(wParam) >= ID_FAVORITE_FIRST && LOWORD(wParam) <= ID_FAVORITE_LAST) {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, favoritesKey,
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            WCHAR namebuf[KEY_MAX_LEN];
            BYTE valuebuf[4096];
            DWORD ksize = KEY_MAX_LEN, vsize = sizeof(valuebuf), type = 0;
            if (RegEnumValueW(hKey, LOWORD(wParam) - ID_FAVORITE_FIRST, namebuf, &ksize, NULL,
                &type, valuebuf, &vsize) == ERROR_SUCCESS) {
                SendMessageW( g_pChildWnd->hTreeWnd, TVM_SELECTITEM, TVGN_CARET,
                             (LPARAM) FindPathInTree(g_pChildWnd->hTreeWnd, (WCHAR *)valuebuf) );
            }
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    switch (LOWORD(wParam)) {
    case ID_REGISTRY_IMPORTREGISTRYFILE:
        ImportRegistryFile(hWnd);
        break;
    case ID_EDIT_EXPORT:
    case ID_REGISTRY_EXPORTREGISTRYFILE:
        ExportRegistryFile(hWnd);
        break;
    case ID_REGISTRY_PRINT:
    {
        const WCHAR empty = 0;
        PrintRegistryHive(hWnd, &empty);
        break;
    }
    case ID_EDIT_DELETE:
    {
        HWND hWndDelete = GetFocus();
        if (hWndDelete == g_pChildWnd->hTreeWnd) {
	    WCHAR* keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
	    if (keyPath == 0 || *keyPath == 0) {
	        MessageBeep(MB_ICONHAND);
            } else if (DeleteKey(hWnd, hKeyRoot, keyPath)) {
		DeleteNode(g_pChildWnd->hTreeWnd, 0);
            }
            free(keyPath);
        } else if (hWndDelete == g_pChildWnd->hListWnd) {
            unsigned int num_selected, index, focus_idx;
            WCHAR *keyPath;

            num_selected = SendMessageW(g_pChildWnd->hListWnd, LVM_GETSELECTEDCOUNT, 0, 0L);

            if (!num_selected)
                break;
            else if (num_selected == 1)
                index = IDS_DELETE_VALUE_TEXT;
            else
                index = IDS_DELETE_VALUE_TEXT_MULTIPLE;

            if (messagebox(hWnd, MB_YESNO | MB_ICONEXCLAMATION, IDS_DELETE_VALUE_TITLE, index) != IDYES)
                break;

            keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);

            focus_idx = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_FOCUSED, 0));
            index = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0));

            while (index != -1)
            {
                WCHAR *valueName = GetItemText(g_pChildWnd->hListWnd, index);
                if (!DeleteValue(hWnd, hKeyRoot, keyPath, valueName))
                {
                    free(valueName);
                    break;
                }
                free(valueName);
                SendMessageW(g_pChildWnd->hListWnd, LVM_DELETEITEM, index, 0L);
                /* the default value item is always visible, so add it back in */
                if (!index)
                {
                    AddEntryToList(g_pChildWnd->hListWnd, NULL, REG_SZ, NULL, 0, 0);
                    if (!focus_idx)
                    {
                        LVITEMW item;
                        item.state = item.stateMask = LVIS_FOCUSED;
                        SendMessageW(g_pChildWnd->hListWnd, LVM_SETITEMSTATE, 0, (LPARAM)&item);
                    }
                }
                index = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_SELECTED, 0));
            }
            free(keyPath);
        } else if (IsChild(g_pChildWnd->hTreeWnd, hWndDelete) ||
                   IsChild(g_pChildWnd->hListWnd, hWndDelete)) {
            SendMessageW(hWndDelete, WM_KEYDOWN, VK_DELETE, 0);
        }
        break;
    }
    case ID_EDIT_MODIFY:
    case ID_EDIT_MODIFY_BIN:
    {
        WCHAR *valueName = GetValueName(g_pChildWnd->hListWnd);
        WCHAR *keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        ModifyValue(hWnd, hKeyRoot, keyPath, valueName);
        free(keyPath);
        free(valueName);
        break;
    }
    case ID_EDIT_FIND:
    case ID_EDIT_FINDNEXT:
    {
        HTREEITEM hItem;
        if (LOWORD(wParam) == ID_EDIT_FIND &&
            DialogBoxW(0, MAKEINTRESOURCEW(IDD_FIND), hWnd, find_dlgproc) != IDOK)
            break;
        if (!*searchString)
            break;
        hItem = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
        if (hItem) {
            int row = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_FOCUSED, 0));
            HCURSOR hcursorOld = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));
            hItem = FindNext(g_pChildWnd->hTreeWnd, hItem, searchString, searchMask, &row);
            SetCursor(hcursorOld);
            if (hItem) {
                SendMessageW( g_pChildWnd->hTreeWnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM) hItem );
                InvalidateRect(g_pChildWnd->hTreeWnd, NULL, TRUE);
                UpdateWindow(g_pChildWnd->hTreeWnd);
                if (row != -1) {
                    LVITEMW item;

                    item.state = 0;
                    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    SendMessageW(g_pChildWnd->hListWnd, LVM_SETITEMSTATE, (UINT)-1, (LPARAM)&item);

                    item.state = LVIS_FOCUSED | LVIS_SELECTED;
                    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
                    SendMessageW(g_pChildWnd->hListWnd, LVM_SETITEMSTATE, row, (LPARAM)&item);
                    SetFocus(g_pChildWnd->hListWnd);
                } else {
                    SetFocus(g_pChildWnd->hTreeWnd);
                }
            } else {
                messagebox(hWnd, MB_OK|MB_ICONINFORMATION, IDS_APP_TITLE, IDS_NOTFOUND, searchString);
            }
        }
        break;
    }
    case ID_EDIT_COPYKEYNAME:
    {
        LPWSTR fullPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
        if (fullPath) {
            CopyKeyName(hWnd, fullPath);
            free(fullPath);
        }
        break;
    }
    case ID_EDIT_NEW_KEY:
    {
        WCHAR newKeyW[MAX_NEW_KEY_LEN];
        WCHAR* keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        if (CreateKey(hWnd, hKeyRoot, keyPath, newKeyW)) {
            if (InsertNode(g_pChildWnd->hTreeWnd, 0, newKeyW))
                StartKeyRename(g_pChildWnd->hTreeWnd);
        }
        free(keyPath);
    }
	break;
    case ID_EDIT_NEW_STRINGVALUE:
	valueType = REG_SZ;
	goto create_value;
    case ID_EDIT_NEW_EXPANDVALUE:
	valueType = REG_EXPAND_SZ;
	goto create_value;
    case ID_EDIT_NEW_MULTI_STRINGVALUE:
	valueType = REG_MULTI_SZ;
	goto create_value;
    case ID_EDIT_NEW_BINARYVALUE:
	valueType = REG_BINARY;
	goto create_value;
    case ID_EDIT_NEW_DWORDVALUE:
	valueType = REG_DWORD;
        goto create_value;
    case ID_EDIT_NEW_QWORDVALUE:
	valueType = REG_QWORD;
	/* fall through */
    create_value:
    {
        WCHAR* keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        WCHAR newKey[MAX_NEW_KEY_LEN];
        if (CreateValue(hWnd, hKeyRoot, keyPath, valueType, newKey))
            StartValueRename(g_pChildWnd->hListWnd);
        free(keyPath);
    }
	break;
    case ID_EDIT_RENAME:
    {
        WCHAR* keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        if (!keyPath) {
            MessageBeep(MB_ICONHAND);
            break;
        } else if (*keyPath && GetFocus() == g_pChildWnd->hTreeWnd) {
            StartKeyRename(g_pChildWnd->hTreeWnd);
        } else if (GetFocus() == g_pChildWnd->hListWnd) {
            StartValueRename(g_pChildWnd->hListWnd);
        }
        free(keyPath);
	break;
    }
    case ID_TREE_EXPAND_COLLAPSE:
    {
        HTREEITEM selected = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
        SendMessageW(g_pChildWnd->hTreeWnd, TVM_EXPAND, TVE_TOGGLE, (LPARAM)selected);
        break;
    }
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
	LPWSTR lpKeyPath = GetItemFullPath(g_pChildWnd->hTreeWnd, NULL, FALSE);
    	if (lpKeyPath) {
            if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_ADDFAVORITE), hWnd, addtofavorites_dlgproc) == IDOK) {
                if (RegCreateKeyExW(HKEY_CURRENT_USER, favoritesKey,
                    0, NULL, 0, 
                    KEY_READ|KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    RegSetValueExW(hKey, favoriteName, 0, REG_SZ, (BYTE *)lpKeyPath, (lstrlenW(lpKeyPath)+1)*sizeof(WCHAR));
                    RegCloseKey(hKey);
                }
            }
            free(lpKeyPath);
        }
        break;
    }
    case ID_FAVORITES_REMOVEFAVORITE:
    {
        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_DELFAVORITE), hWnd, removefavorite_dlgproc) == IDOK) {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, favoritesKey,
                0, KEY_READ|KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                RegDeleteValueW(hKey, favoriteName);
                RegCloseKey(hKey);
            }
        }
        break;
    }
    case ID_VIEW_REFRESH:
    {
        WCHAR* keyPath = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hKeyRoot);
        RefreshTreeView(g_pChildWnd->hTreeWnd);
        RefreshListView(g_pChildWnd->hListWnd, hKeyRoot, keyPath, NULL);
        free(keyPath);
    }
        break;
   /*case ID_OPTIONS_TOOLBAR:*/
   /*	toggle_child(hWnd, LOWORD(wParam), hToolBar);*/
   /*    break;*/
    case ID_VIEW_STATUSBAR:
        toggle_child(hWnd, LOWORD(wParam), hStatusBar);
        break;
    case ID_HELP_HELPTOPICS:
    {
        WinHelpW(hWnd, L"regedit", HELP_FINDER, 0);
        break;
    }
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
            SetCursor(LoadCursorW(0, (LPCWSTR)IDC_SIZEWE));
            SendMessageW(g_pChildWnd->hWnd, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
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
        CreateWindowExW(0, szChildClass, L"regedit child window", WS_CHILD | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        hWnd, NULL, hInst, 0);
        LoadStringW(hInst, IDS_EXPAND, expandW, ARRAY_SIZE(expandW));
        LoadStringW(hInst, IDS_COLLAPSE, collapseW, ARRAY_SIZE(collapseW));
        LoadStringW(hInst, IDS_EDIT_MODIFY, modifyW, ARRAY_SIZE(modifyW));
        LoadStringW(hInst, IDS_EDIT_MODIFY_BIN, modify_binaryW, ARRAY_SIZE(modify_binaryW));
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam))
            return DefWindowProcW(hWnd, message, wParam, lParam);
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
            OnInitMenuPopup(hWnd, (HMENU)wParam);
        break;
    case WM_MENUSELECT:
        OnMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    case WM_DESTROY:
    {
        WinHelpW(hWnd, L"regedit", HELP_QUIT, 0);
        PostQuitMessage(0);
    }
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
