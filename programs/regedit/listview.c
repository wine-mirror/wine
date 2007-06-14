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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>

#include "main.h"

#include "wine/unicode.h"
static INT Image_String;
static INT Image_Binary;

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
static LPTSTR g_currentPath;
static HKEY g_currentRootKey;
static TCHAR g_szValueNotSet[64];

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 200, 175, 400 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

static LPTSTR get_item_text(HWND hwndLV, int item)
{
    LPTSTR newStr, curStr;
    unsigned int maxLen = 128;

    curStr = HeapAlloc(GetProcessHeap(), 0, maxLen);
    if (!curStr) return NULL;
    if (item == 0) return NULL; /* first item is ALWAYS a default */
    do {
        ListView_GetItemText(hwndLV, item, 0, curStr, maxLen);
	if (_tcslen(curStr) < maxLen - 1) return curStr;
	newStr = HeapReAlloc(GetProcessHeap(), 0, curStr, maxLen * 2);
	if (!newStr) break;
	curStr = newStr;
	maxLen *= 2;
    } while (TRUE);
    HeapFree(GetProcessHeap(), 0, curStr);
    return NULL;
}

LPCTSTR GetValueName(HWND hwndLV)
{
    INT item;

    if (g_valueName != LPSTR_TEXTCALLBACK)
        HeapFree(GetProcessHeap(), 0,  g_valueName);
    g_valueName = NULL;

    item = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED);
    if (item == -1) return NULL;

    g_valueName = get_item_text(hwndLV, item);

    return g_valueName;
}

/* convert '\0' separated string list into ',' separated string list */
static void MakeMULTISZDisplayable(LPTSTR multi)
{
    do
    {
        for (; *multi; multi++)
            ;
        if (*(multi+1))
        {
            *multi = ',';
            multi++;
        }
    } while (*multi);
}

/*******************************************************************************
 * Local module support methods
 */
static void AddEntryToList(HWND hwndLV, LPTSTR Name, DWORD dwValType, 
    void* ValBuf, DWORD dwCount, BOOL bHighlight)
{
    LINE_INFO* linfo;
    LVITEM item;
    int index;

    linfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINE_INFO) + dwCount);
    linfo->dwValType = dwValType;
    linfo->val_len = dwCount;
    memcpy(&linfo[1], ValBuf, dwCount);
    
    if (Name)
        linfo->name = _tcsdup(Name);
    else
        linfo->name = NULL;

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    item.iItem = ListView_GetItemCount(hwndLV);/*idx;  */
    item.iSubItem = 0;
    item.state = 0;
    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    item.pszText = Name ? Name : LPSTR_TEXTCALLBACK;
    item.cchTextMax = Name ? _tcslen(item.pszText) : 0;
    if (bHighlight) {
        item.stateMask = item.state = LVIS_FOCUSED | LVIS_SELECTED;
    }
    switch (dwValType)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
    case REG_MULTI_SZ:
        item.iImage = Image_String;
        break;
    default:
        item.iImage = Image_Binary;
        break;
    }
    item.lParam = (LPARAM)linfo;

#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif

    index = ListView_InsertItem(hwndLV, &item);
    if (index != -1) {
        switch (dwValType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            if (ValBuf) {
                ListView_SetItemText(hwndLV, index, 2, ValBuf);
            } else {
                ListView_SetItemText(hwndLV, index, 2, g_szValueNotSet);
            }
            break;
        case REG_DWORD: {
                TCHAR buf[64];
                wsprintf(buf, _T("0x%08X (%d)"), *(DWORD*)ValBuf, *(DWORD*)ValBuf);
                ListView_SetItemText(hwndLV, index, 2, buf);
            }
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
        case REG_MULTI_SZ:
            MakeMULTISZDisplayable(ValBuf);
            ListView_SetItemText(hwndLV, index, 2, ValBuf);
            break;
        default:
          {
            TCHAR szText[128];
            LoadString(hInst, IDS_REGISTRY_VALUE_CANT_DISPLAY, szText, COUNT_OF(szText));
            ListView_SetItemText(hwndLV, index, 2, szText);
            break;
          }
        }
    }
}

static BOOL InitListViewImageList(HWND hWndListView)
{
    HIMAGELIST himl;
    HICON hicon;
    INT cx = GetSystemMetrics(SM_CXSMICON);
    INT cy = GetSystemMetrics(SM_CYSMICON);

    himl = ImageList_Create(cx, cy, ILC_MASK, 0, 2);
    if (!himl)
        return FALSE;

    hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_STRING),
        IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
    Image_String = ImageList_AddIcon(himl, hicon);

    hicon = LoadImage(hInst, MAKEINTRESOURCE(IDI_BIN),
        IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
    Image_Binary = ImageList_AddIcon(himl, hicon);

    SendMessage( hWndListView, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM) himl );

    /* fail if some of the icons failed to load */
    if (ImageList_GetImageCount(himl) < 2)
        return FALSE;

    return TRUE;
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
    static TCHAR reg_szT[]               = {'R','E','G','_','S','Z',0},
                 reg_expand_szT[]        = {'R','E','G','_','E','X','P','A','N','D','_','S','Z',0},
                 reg_binaryT[]           = {'R','E','G','_','B','I','N','A','R','Y',0},
                 reg_dwordT[]            = {'R','E','G','_','D','W','O','R','D',0},
                 reg_dword_big_endianT[] = {'R','E','G','_','D','W','O','R','D','_',
                                            'B','I','G','_','E','N','D','I','A','N',0},
                 reg_multi_szT[]         = {'R','E','G','_','M','U','L','T','I','_','S','Z',0},
                 reg_linkT[]             = {'R','E','G','_','L','I','N','K',0},
                 reg_resource_listT[]    = {'R','E','G','_','R','E','S','O','U','R','C','E','_','L','I','S','T',0},
                 reg_noneT[]             = {'R','E','G','_','N','O','N','E',0},
                 emptyT[]                = {0};

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0;

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = (LPSTR)g_pszDefaultValueName;
        break;
    case 1:
        switch (((LINE_INFO*)plvdi->item.lParam)->dwValType) {
        case REG_SZ:
            plvdi->item.pszText = reg_szT;
            break;
        case REG_EXPAND_SZ:
            plvdi->item.pszText = reg_expand_szT;
            break;
        case REG_BINARY:
            plvdi->item.pszText = reg_binaryT;
            break;
        case REG_DWORD:
            plvdi->item.pszText = reg_dwordT;
            break;
        case REG_DWORD_BIG_ENDIAN:
            plvdi->item.pszText = reg_dword_big_endianT;
            break;
        case REG_MULTI_SZ:
            plvdi->item.pszText = reg_multi_szT;
            break;
        case REG_LINK:
            plvdi->item.pszText = reg_linkT;
            break;
        case REG_RESOURCE_LIST:
            plvdi->item.pszText = reg_resource_listT;
            break;
        case REG_NONE:
            plvdi->item.pszText = reg_noneT;
            break;
        default:
          {
            TCHAR szUnknownFmt[64];
            LoadString(hInst, IDS_REGISTRY_UNKNOWN_TYPE, szUnknownFmt, COUNT_OF(szUnknownFmt));
            wsprintf(buffer, szUnknownFmt, plvdi->item.lParam);
            plvdi->item.pszText = buffer;
            break;
          }
        }
        break;
    case 2:
        plvdi->item.pszText = g_szValueNotSet;
        break;
    case 3:
        plvdi->item.pszText = emptyT;
        break;
    }
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LINE_INFO*l, *r;
    l = (LINE_INFO*)lParam1;
    r = (LINE_INFO*)lParam2;
    if (!l->name) return -1;
    if (!r->name) return +1;
        
    if (g_columnToSort == ~0UL) 
        g_columnToSort = 0;
    
    if (g_columnToSort == 1 && l->dwValType != r->dwValType)
        return g_invertSort ? (int)r->dwValType - (int)l->dwValType : (int)l->dwValType - (int)r->dwValType;
    if (g_columnToSort == 2) {
        /* FIXME: Sort on value */
    }
    return g_invertSort ? _tcscmp(r->name, l->name) : _tcscmp(l->name, r->name);
}

HWND StartValueRename(HWND hwndLV)
{
    int item;

    item = ListView_GetNextItem(hwndLV, -1, LVNI_FOCUSED | LVNI_SELECTED);
    if (item < 1) { /* cannot rename default key */
        MessageBeep(MB_ICONHAND);
        return 0;
    }
    return ListView_EditLabel(hwndLV, item);
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
    case WM_NOTIFY_REFLECT:
        switch (((LPNMHDR)lParam)->code) {
	
        case LVN_BEGINLABELEDIT:
            if (!((NMLVDISPINFO *)lParam)->item.iItem)
                return 1;
            return 0;
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
                    
            SendMessage(hWnd, LVM_SORTITEMS, (WPARAM)hWnd, (LPARAM)CompareFunc);
            break;
	case LVN_ENDLABELEDIT: {
	        LPNMLVDISPINFO dispInfo = (LPNMLVDISPINFO)lParam;
		LPTSTR valueName = get_item_text(hWnd, dispInfo->item.iItem);
                LONG ret;
                if (!valueName) return -1; /* cannot rename a default value */
	        ret = RenameValue(hWnd, g_currentRootKey, g_currentPath, valueName, dispInfo->item.pszText);
		if (ret)
                    RefreshListView(hWnd, g_currentRootKey, g_currentPath, dispInfo->item.pszText);
		HeapFree(GetProcessHeap(), 0, valueName);
		return 0;
	    }
        case NM_RETURN: {
                int cnt = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED | LVNI_SELECTED);
	        if (cnt != -1)
		    SendMessage(hFrameWnd, WM_COMMAND, ID_EDIT_MODIFY, 0);
	    }
	    break;
        case NM_DBLCLK: {
                NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;
                LVHITTESTINFO info;

                /* if (nmitem->hdr.hwndFrom != hWnd) break; unnecessary because of WM_NOTIFY_REFLECT */
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
                    ListView_SetItemState(hWnd, -1, 0, LVIS_FOCUSED|LVIS_SELECTED);
                    ListView_SetItemState(hWnd, info.iItem, LVIS_FOCUSED|LVIS_SELECTED,
                        LVIS_FOCUSED|LVIS_SELECTED);
		    SendMessage(hFrameWnd, WM_COMMAND, ID_EDIT_MODIFY, 0);
                }
            }
            break;

        default:
            return 0; /* shouldn't call default ! */
        }
        break;
    case WM_CONTEXTMENU: {
        int cnt = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
        TrackPopupMenu(GetSubMenu(hPopupMenus, cnt == -1 ? PM_NEW : PM_MODIFYVALUE),
                       TPM_RIGHTBUTTON, (short)LOWORD(lParam), (short)HIWORD(lParam),
                       0, hFrameWnd, NULL);
        break;
    }
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

    /* prepare strings */
    LoadString(hInst, IDS_REGISTRY_VALUE_NOT_SET, g_szValueNotSet, COUNT_OF(g_szValueNotSet));

    /* Get the dimensions of the parent window's client area, and create the list view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("List View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | LVS_REPORT | LVS_EDITLABELS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, (HMENU)id, hInst, NULL);
    if (!hwndLV) return NULL;
    SendMessage(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    /* Initialize the image list */
    if (!InitListViewImageList(hwndLV)) goto fail;
    if (!CreateListColumns(hwndLV)) goto fail;
    g_orgListWndProc = (WNDPROC) SetWindowLongPtr(hwndLV, GWLP_WNDPROC, (LPARAM)ListWndProc);
    return hwndLV;
fail:
    DestroyWindow(hwndLV);
    return NULL;
}

BOOL RefreshListView(HWND hwndLV, HKEY hKeyRoot, LPCTSTR keyPath, LPCTSTR highlightValue)
{
    BOOL result = FALSE;
    DWORD max_sub_key_len;
    DWORD max_val_name_len, valNameLen;
    DWORD max_val_size, valSize;
    DWORD val_count, index, valType;
    TCHAR* valName = 0;
    BYTE* valBuf = 0;
    HKEY hKey = 0;
    LONG errCode;
    INT count, i;
    LVITEM item;

    if (!hwndLV) return FALSE;

    SendMessage(hwndLV, WM_SETREDRAW, FALSE, 0);

    errCode = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_READ, &hKey);
    if (errCode != ERROR_SUCCESS) goto done;

    count = ListView_GetItemCount(hwndLV);
    for (i = 0; i < count; i++) {
        item.mask = LVIF_PARAM;
        item.iItem = i;
        SendMessage( hwndLV, LVM_GETITEM, 0, (LPARAM)&item );
        free(((LINE_INFO*)item.lParam)->name);
        HeapFree(GetProcessHeap(), 0, (void*)item.lParam);
    }
    g_columnToSort = ~0UL;
    SendMessage( hwndLV, LVM_DELETEALLITEMS, 0, 0L );

    /* get size information and resize the buffers if necessary */
    errCode = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL, 
                              &val_count, &max_val_name_len, &max_val_size, NULL, NULL);
    if (errCode != ERROR_SUCCESS) goto done;

    /* account for the terminator char */
    max_val_name_len++;
    max_val_size++;

    valName = HeapAlloc(GetProcessHeap(), 0, max_val_name_len * sizeof(TCHAR));
    valBuf = HeapAlloc(GetProcessHeap(), 0, max_val_size);
    if (RegQueryValueEx(hKey, NULL, NULL, &valType, valBuf, &valSize) == ERROR_FILE_NOT_FOUND) { 
        AddEntryToList(hwndLV, NULL, REG_SZ, NULL, 0, !highlightValue);
    }
    for(index = 0; index < val_count; index++) {
        BOOL bSelected = (valName == highlightValue); /* NOT a bug, we check for double NULL here */
        valNameLen = max_val_name_len;
        valSize = max_val_size;
	valType = 0;
        errCode = RegEnumValue(hKey, index, valName, &valNameLen, NULL, &valType, valBuf, &valSize);
	if (errCode != ERROR_SUCCESS) goto done;
        valBuf[valSize] = 0;
        if (valName && highlightValue && !_tcscmp(valName, highlightValue))
            bSelected = TRUE;
        AddEntryToList(hwndLV, valName[0] ? valName : NULL, valType, valBuf, valSize, bSelected);
    }
    SendMessage(hwndLV, LVM_SORTITEMS, (WPARAM)hwndLV, (LPARAM)CompareFunc);

    g_currentRootKey = hKeyRoot;
    if (keyPath != g_currentPath) {
	HeapFree(GetProcessHeap(), 0, g_currentPath);
	g_currentPath = HeapAlloc(GetProcessHeap(), 0, (lstrlen(keyPath) + 1) * sizeof(TCHAR));
	if (!g_currentPath) goto done;
	lstrcpy(g_currentPath, keyPath);
    }

    result = TRUE;

done:
    HeapFree(GetProcessHeap(), 0, valBuf);
    HeapFree(GetProcessHeap(), 0, valName);
    SendMessage(hwndLV, WM_SETREDRAW, TRUE, 0);
    if (hKey) RegCloseKey(hKey);

    return result;
}
