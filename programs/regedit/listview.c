/*
 * Regedit listviews
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "main.h"

typedef struct tagLINE_INFO
{
    DWORD dwValType;
    LPTSTR name;
    void* val;
    size_t val_len;
} LINE_INFO;

/*******************************************************************************
 * Global and Local Variables:
 */

static WNDPROC g_orgListWndProc;
static DWORD g_columnToSort = ~0UL;
static BOOL  g_invertSort = FALSE;
static LPTSTR g_valueName;
static LPCTSTR g_currentValue;
static LPTSTR g_currentPath;
static HKEY g_currentRootKey;

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 200, 175, 400 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

LPCTSTR GetValueName(HWND hwndLV)
{
    int item, len, maxLen;
    LPTSTR newStr;

    if (!g_valueName) g_valueName = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!g_valueName) return NULL;
    *g_valueName = 0;
    maxLen = HeapSize(GetProcessHeap(), 0, g_valueName);
    if (maxLen == (SIZE_T) - 1) return NULL;

    item = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED);
    if (item == -1) return NULL;
    do {
        ListView_GetItemText(hwndLV, item, 0, g_valueName, maxLen);
	len = _tcslen(g_valueName);
	if (len < maxLen - 1) break;
	newStr = HeapReAlloc(GetProcessHeap(), 0, g_valueName, maxLen * 2);
	if (!newStr) return NULL;
	g_valueName = newStr;
	maxLen *= 2;
    } while (TRUE);

    return g_valueName;
}

/*******************************************************************************
 * Local module support methods
 */
static void AddEntryToList(HWND hwndLV, LPTSTR Name, DWORD dwValType, void* ValBuf, DWORD dwCount)
{
    LINE_INFO* linfo;
    LVITEM item;
    int index;

    linfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINE_INFO) + dwCount);
    linfo->dwValType = dwValType;
    linfo->val_len = dwCount;
    memcpy(&linfo[1], ValBuf, dwCount);
    linfo->name = _tcsdup(Name);

    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = 0;/*idx;  */
    item.iSubItem = 0;
    item.state = 0;
    item.stateMask = 0;
    item.pszText = Name;
    item.cchTextMax = _tcslen(item.pszText);
    if (item.cchTextMax == 0)
        item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = 0;
    item.lParam = (LPARAM)linfo;

    /*    item.lParam = (LPARAM)ValBuf; */
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif

    index = ListView_InsertItem(hwndLV, &item);
    if (index != -1) {
        /*        LPTSTR pszText = NULL; */
        LPTSTR pszText = _T("value");
        switch (dwValType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            ListView_SetItemText(hwndLV, index, 2, ValBuf);
            break;
        case REG_DWORD: {
                TCHAR buf[64];
                wsprintf(buf, _T("0x%08X (%d)"), *(DWORD*)ValBuf, *(DWORD*)ValBuf);
                ListView_SetItemText(hwndLV, index, 2, buf);
            }
            /*            lpsRes = convertHexToDWORDStr(lpbData, dwLen); */
            break;
        case REG_BINARY: {
                unsigned int i;
                LPBYTE pData = (LPBYTE)ValBuf;
                LPTSTR strBinary = HeapAlloc(GetProcessHeap(), 0, dwCount * sizeof(TCHAR) * 3 + 1);
                for (i = 0; i < dwCount; i++)
                    wsprintf( strBinary + i*3, _T("%02X "), pData[i] );
                strBinary[dwCount * 3] = 0;
                ListView_SetItemText(hwndLV, index, 2, strBinary);
                HeapFree(GetProcessHeap(), 0, strBinary);
            }
            break;
        default:
            /*            lpsRes = convertHexToHexCSV(lpbData, dwLen); */
            ListView_SetItemText(hwndLV, index, 2, pszText);
            break;
        }
    }
}

static BOOL CreateListColumns(HWND hWndListView)
{
    TCHAR szText[50];
    int index;
    LV_COLUMN lvC;

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    /* Load the column labels from the resource file. */
    for (index = 0; index < MAX_LIST_COLUMNS; index++) {
        lvC.iSubItem = index;
        lvC.cx = default_column_widths[index];
        lvC.fmt = column_alignment[index];
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, sizeof(szText)/sizeof(TCHAR));
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) return FALSE;
    }
    return TRUE;
}

/* OnGetDispInfo - processes the LVN_GETDISPINFO notification message.  */

static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static TCHAR buffer[200];

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0;

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = _T("(Default)");
        break;
    case 1:
        switch (((LINE_INFO*)plvdi->item.lParam)->dwValType) {
        case REG_SZ:
            plvdi->item.pszText = _T("REG_SZ");
            break;
        case REG_EXPAND_SZ:
            plvdi->item.pszText = _T("REG_EXPAND_SZ");
            break;
        case REG_BINARY:
            plvdi->item.pszText = _T("REG_BINARY");
            break;
        case REG_DWORD:
            plvdi->item.pszText = _T("REG_DWORD");
            break;
        case REG_DWORD_BIG_ENDIAN:
            plvdi->item.pszText = _T("REG_DWORD_BIG_ENDIAN");
            break;
        case REG_MULTI_SZ:
            plvdi->item.pszText = _T("REG_MULTI_SZ");
            break;
        case REG_LINK:
            plvdi->item.pszText = _T("REG_LINK");
            break;
        case REG_RESOURCE_LIST:
            plvdi->item.pszText = _T("REG_RESOURCE_LIST");
            break;
        case REG_NONE:
            plvdi->item.pszText = _T("REG_NONE");
            break;
        default:
            wsprintf(buffer, _T("unknown(%d)"), plvdi->item.lParam);
            plvdi->item.pszText = buffer;
            break;
        }
        break;
    case 2:
        plvdi->item.pszText = _T("(value not set)");
        break;
    case 3:
        plvdi->item.pszText = _T("");
        break;
    }
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LINE_INFO*l, *r;
    l = (LINE_INFO*)lParam1;
    r = (LINE_INFO*)lParam2;
        
    if (g_columnToSort == ~0UL) 
        g_columnToSort = 0;
    
    if (g_columnToSort == 1 && l->dwValType != r->dwValType)
        return g_invertSort ? (int)r->dwValType - (int)l->dwValType : (int)l->dwValType - (int)r->dwValType;
    if (g_columnToSort == 2) {
        /* FIXME: Sort on value */
    }
    return g_invertSort ? _tcscmp(r->name, l->name) : _tcscmp(l->name, r->name);
}

static void ListViewPopUpMenu(HWND hWnd, POINT pt)
{
}

BOOL StartValueRename(HWND hwndLV, HKEY hRootKey, LPCTSTR path)
{
    int item;

    item = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED);
    if (item == -1) return FALSE;
    if (!(g_currentValue = GetValueName(hwndLV))) return FALSE;
    g_currentRootKey = hRootKey;
    HeapFree(GetProcessHeap(), 0, g_currentPath);
    g_currentPath = HeapAlloc(GetProcessHeap(), 0, (lstrlen(path) + 1) * sizeof(TCHAR));
    if (!g_currentPath) return FALSE;
    lstrcpy(g_currentPath, path);
    if (!ListView_EditLabel(hwndLV, item)) return FALSE;
    return TRUE;
}

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
        /*    case ID_FILE_OPEN: */
        /*        break; */
    default:
        return FALSE;
    }
    return TRUE;
}

static LRESULT CALLBACK ListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
        break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case LVN_GETDISPINFO:
            OnGetDispInfo((NMLVDISPINFO*)lParam);
            break;
        case LVN_COLUMNCLICK:
            if (g_columnToSort == ((LPNMLISTVIEW)lParam)->iSubItem)
                g_invertSort = !g_invertSort;
            else {
                g_columnToSort = ((LPNMLISTVIEW)lParam)->iSubItem;
                g_invertSort = FALSE;
            }
                    
            ListView_SortItems(hWnd, CompareFunc, hWnd);
            break;
	case LVN_ENDLABELEDIT:
	    return RenameValue(hWnd, g_currentRootKey, g_currentPath, g_currentValue, ((LPNMLVDISPINFO)lParam)->item.pszText);
        case NM_DBLCLK: {
                NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;
                LVHITTESTINFO info;

                if (nmitem->hdr.hwndFrom != hWnd) break;
                /*            if (nmitem->hdr.idFrom != IDW_LISTVIEW) break;  */
                /*            if (nmitem->hdr.code != ???) break;  */
#ifdef _MSC_VER
                switch (nmitem->uKeyFlags) {
                case LVKF_ALT:     /*  The ALT key is pressed.   */
                    /* properties dialog box ? */
                    break;
                case LVKF_CONTROL: /*  The CTRL key is pressed. */
                    /* run dialog box for providing parameters... */
                    break;
                case LVKF_SHIFT:   /*  The SHIFT key is pressed.    */
                    break;
                }
#endif
                info.pt.x = nmitem->ptAction.x;
                info.pt.y = nmitem->ptAction.y;
                if (ListView_HitTest(hWnd, &info) != -1) {
                    LVITEM item;
                    item.mask = LVIF_PARAM;
                    item.iItem = info.iItem;
                    if (ListView_GetItem(hWnd, &item)) {}
                }
            }
            break;

        case NM_RCLICK: {
                int idx;
                LV_HITTESTINFO lvH;
                NM_LISTVIEW* pNm = (NM_LISTVIEW*)lParam;
                lvH.pt.x = pNm->ptAction.x;
                lvH.pt.y = pNm->ptAction.y;
                idx = ListView_HitTest(hWnd, &lvH);
                if (idx != -1) {
                    POINT pt;
                    GetCursorPos(&pt);
                    ListViewPopUpMenu(hWnd, pt);
                    return idx;
                }
            }
            break;

        default:
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU: {
        POINTS pt = MAKEPOINTS(lParam);
        int cnt = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
        TrackPopupMenu(GetSubMenu(hPopupMenus, cnt == -1 ? PM_NEW : PM_MODIFYVALUE), 
		       TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
        break;
    }
    case WM_KEYDOWN:
        if (wParam == VK_TAB) {
            /*TODO: SetFocus(Globals.hDriveBar) */
            /*SetFocus(child->nFocusPanel? child->left.hWnd: child->right.hWnd); */
        }
        /* fall thru... */
    default:
        return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}


HWND CreateListView(HWND hwndParent, int id)
{
    RECT rcClient;
    HWND hwndLV;

    /* Get the dimensions of the parent window's client area, and create the list view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("List View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_EDITLABELS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, (HMENU)id, hInst, NULL);
    if (!hwndLV) return NULL;
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);

    /* Initialize the image list, and add items to the control.  */
    /*
    if (!InitListViewImageLists(hwndLV)) goto fail;
    if (!InitListViewItems(hwndLV, szName)) goto fail;
    */
    if (!CreateListColumns(hwndLV)) goto fail;
    g_orgListWndProc = SubclassWindow(hwndLV, ListWndProc);
    return hwndLV;
fail:
    DestroyWindow(hwndLV);
    return NULL;
}

BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPCTSTR keyPath)
{
    DWORD max_sub_key_len;
    DWORD max_val_name_len;
    DWORD max_val_size;
    DWORD val_count;
    HKEY hNewKey;
    LONG errCode;
    INT count, i;
    LVITEM item;

    if (!hwndLV) return FALSE;

    SendMessage(hwndLV, WM_SETREDRAW, FALSE, 0);
    count = ListView_GetItemCount(hwndLV);
    for (i = 0; i < count; i++) {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        ListView_GetItem(hwndLV, &item);
        free(((LINE_INFO*)item.lParam)->name);
        HeapFree(GetProcessHeap(), 0, (void*)item.lParam);
    }
    g_columnToSort = ~0UL;
    ListView_DeleteAllItems(hwndLV);

    errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
    if (errCode != ERROR_SUCCESS) return FALSE;

    /* get size information and resize the buffers if necessary */
    errCode = RegQueryInfoKey(hNewKey, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL, 
                              &val_count, &max_val_name_len, &max_val_size, NULL, NULL);

    #define BUF_HEAD_SPACE 2 /* FIXME: check why this is required with ROS ??? */

    if (errCode == ERROR_SUCCESS) {
        TCHAR* ValName = HeapAlloc(GetProcessHeap(), 0, ++max_val_name_len * sizeof(TCHAR) + BUF_HEAD_SPACE);
        DWORD dwValNameLen = max_val_name_len;
        BYTE* ValBuf = HeapAlloc(GetProcessHeap(), 0, ++max_val_size/* + BUF_HEAD_SPACE*/);
        DWORD dwValSize = max_val_size;
        DWORD dwIndex = 0L;
        DWORD dwValType;
        /*                if (RegQueryValueEx(hNewKey, NULL, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) { */
        /*                    AddEntryToList(hwndLV, _T("(Default)"), dwValType, ValBuf, dwValSize); */
        /*                } */
        /*                dwValSize = max_val_size; */
        while (RegEnumValue(hNewKey, dwIndex, ValName, &dwValNameLen, NULL, &dwValType, ValBuf, &dwValSize) == ERROR_SUCCESS) {
            ValBuf[dwValSize] = 0;
            AddEntryToList(hwndLV, ValName, dwValType, ValBuf, dwValSize);
            dwValNameLen = max_val_name_len;
            dwValSize = max_val_size;
            dwValType = 0L;
            ++dwIndex;
        }
        HeapFree(GetProcessHeap(), 0, ValBuf);
        HeapFree(GetProcessHeap(), 0, ValName);
    }
    ListView_SortItems(hwndLV, CompareFunc, hwndLV);
    RegCloseKey(hNewKey);
    SendMessage(hwndLV, WM_SETREDRAW, TRUE, 0);

    return TRUE;
}
