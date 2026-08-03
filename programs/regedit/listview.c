/*
 * Regedit listviews
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * Copyright (C) 2008 Alexander N. Sørnes <alex@thehandofagony.com>
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
#include <winternl.h>
#include <commctrl.h>

#include "main.h"

static INT Image_String;
static INT Image_Binary;

/*******************************************************************************
 * Global and Local Variables:
 */

DWORD   g_columnToSort = ~0U;
BOOL    g_invertSort = FALSE;
WCHAR  *g_currentPath;
HKEY    g_currentRootKey;
static  WCHAR g_szValueNotSet[64];

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 200, 175, 400 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

LPWSTR GetItemText(HWND hwndLV, UINT item)
{
    WCHAR *curStr;
    unsigned int maxLen = 128;
    if (item == 0) return NULL; /* first item is ALWAYS a default */

    curStr = malloc(maxLen * sizeof(WCHAR));
    do {
        ListView_GetItemTextW(hwndLV, item, 0, curStr, maxLen);
        if (lstrlenW(curStr) < maxLen - 1) return curStr;
        maxLen *= 2;
        curStr = realloc(curStr, maxLen * sizeof(WCHAR));
    } while (TRUE);
    free(curStr);
    return NULL;
}

WCHAR *GetValueName(HWND hwndLV)
{
    INT item;

    item = SendMessageW(hwndLV, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_FOCUSED, 0));
    if (item == -1) return NULL;

    return GetItemText(hwndLV, item);
}

BOOL update_listview_path(const WCHAR *path)
{
    free(g_currentPath);

    g_currentPath = wcsdup(path);

    return TRUE;
}

/* convert '\0' separated string list into ',' separated string list */
static void MakeMULTISZDisplayable(LPWSTR multi)
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
void format_value_data(HWND hwndLV, int index, DWORD type, void *data, DWORD size)
{
    switch (type)
    {
        case REG_SZ:
        case REG_EXPAND_SZ:
            ListView_SetItemTextW(hwndLV, index, 2, data ? data : g_szValueNotSet);
            break;
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
        {
            DWORD value = *(DWORD *)data;
            WCHAR buf[64];
            if (type == REG_DWORD_BIG_ENDIAN)
                value = RtlUlongByteSwap(value);
            wsprintfW(buf, L"0x%08x (%u)", value, value);
            ListView_SetItemTextW(hwndLV, index, 2, buf);
            break;
        }
        case REG_QWORD:
        {
            UINT64 value = *(UINT64 *)data;
            WCHAR buf[64];
            swprintf(buf, ARRAY_SIZE(buf), L"0x%08I64x (%I64u)", value, value);
            ListView_SetItemTextW(hwndLV, index, 2, buf);
            break;
        }
        case REG_MULTI_SZ:
            MakeMULTISZDisplayable(data);
            ListView_SetItemTextW(hwndLV, index, 2, data);
            break;
        case REG_BINARY:
        case REG_NONE:
        default:
        {
            unsigned int i;
            BYTE *pData = data;
            WCHAR *strBinary = malloc(size * sizeof(WCHAR) * 3 + sizeof(WCHAR));
            for (i = 0; i < size; i++)
                wsprintfW( strBinary + i*3, L"%02X ", pData[i] );
            strBinary[size * 3] = 0;
            ListView_SetItemTextW(hwndLV, index, 2, strBinary);
            free(strBinary);
            break;
        }
    }
}

int AddEntryToList(HWND hwndLV, WCHAR *Name, DWORD dwValType, void *ValBuf, DWORD dwCount, int pos)
{
    LINE_INFO *linfo;
    LVITEMW item = { 0 };
    int index;

    linfo = malloc(sizeof(LINE_INFO));
    linfo->dwValType = dwValType;
    linfo->val_len = dwCount;
    linfo->name = wcsdup(Name);

    if (ValBuf && dwCount)
    {
        linfo->val = malloc(dwCount);
        memcpy(linfo->val, ValBuf, dwCount);
    }
    else linfo->val = NULL;

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
    item.iItem = (pos == -1) ? SendMessageW(hwndLV, LVM_GETITEMCOUNT, 0, 0) : pos;
    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    item.pszText = Name ? Name : LPSTR_TEXTCALLBACKW;
    item.cchTextMax = Name ? lstrlenW(item.pszText) : 0;

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

    if ((index = ListView_InsertItemW(hwndLV, &item)) != -1)
        format_value_data(hwndLV, index, dwValType, ValBuf, dwCount);
    return index;
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

    hicon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_STRING),
        IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
    Image_String = ImageList_AddIcon(himl, hicon);

    hicon = LoadImageW(hInst, MAKEINTRESOURCEW(IDI_BIN),
        IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
    Image_Binary = ImageList_AddIcon(himl, hicon);

    SendMessageW( hWndListView, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM) himl );

    /* fail if some of the icons failed to load */
    if (ImageList_GetImageCount(himl) < 2)
        return FALSE;

    return TRUE;
}

static BOOL CreateListColumns(HWND hWndListView)
{
    WCHAR szText[50];
    int index;
    LVCOLUMNW lvC;

    /* Create columns. */
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    /* Load the column labels from the resource file. */
    for (index = 0; index < MAX_LIST_COLUMNS; index++) {
        lvC.iSubItem = index;
        lvC.cx = default_column_widths[index];
        lvC.fmt = column_alignment[index];
        LoadStringW(hInst, IDS_LIST_COLUMN_FIRST + index, szText, ARRAY_SIZE(szText));
        if (ListView_InsertColumnW(hWndListView, index, &lvC) == -1) return FALSE;
    }
    return TRUE;
}

/* OnGetDispInfo - processes the LVN_GETDISPINFO notification message.  */
void OnGetDispInfo(NMLVDISPINFOW *plvdi)
{
    static WCHAR buffer[200];
    static WCHAR reg_szT[]               = L"REG_SZ",
                 reg_expand_szT[]        = L"REG_EXPAND_SZ",
                 reg_binaryT[]           = L"REG_BINARY",
                 reg_dwordT[]            = L"REG_DWORD",
                 reg_dword_big_endianT[] = L"REG_DWORD_BIG_ENDIAN",
                 reg_qwordT[]            = L"REG_QWORD",
                 reg_multi_szT[]         = L"REG_MULTI_SZ",
                 reg_linkT[]             = L"REG_LINK",
                 reg_resource_listT[]    = L"REG_RESOURCE_LIST",
                 reg_noneT[]             = L"REG_NONE",
                 emptyT[]                = L"";

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0;

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = g_pszDefaultValueName;
        break;
    case 1:
    {
        DWORD data_type = ((LINE_INFO *)plvdi->item.lParam)->dwValType;

        switch (data_type) {
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
        case REG_QWORD:
            plvdi->item.pszText = reg_qwordT;
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
            wsprintfW(buffer, L"0x%x", data_type);
            plvdi->item.pszText = buffer;
            break;
          }
        }
        break;
    }
    case 2:
        plvdi->item.pszText = g_szValueNotSet;
        break;
    case 3:
        plvdi->item.pszText = emptyT;
        break;
    }
}

int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LINE_INFO*l, *r;
    l = (LINE_INFO*)lParam1;
    r = (LINE_INFO*)lParam2;
    if (!l->name) return -1;
    if (!r->name) return +1;
        
    if (g_columnToSort == ~0U)
        g_columnToSort = 0;
    
    if (g_columnToSort == 1)
        return g_invertSort ? (int)r->dwValType - (int)l->dwValType : (int)l->dwValType - (int)r->dwValType;
    if (g_columnToSort == 2) {
        /* FIXME: Sort on value */
        return 0;
    }
    return g_invertSort ? lstrcmpiW(r->name, l->name) : lstrcmpiW(l->name, r->name);
}

HWND StartValueRename(HWND hwndLV)
{
    int item;

    item = SendMessageW(hwndLV, LVM_GETNEXTITEM, -1, MAKELPARAM(LVNI_FOCUSED | LVNI_SELECTED, 0));
    if (item < 1) { /* cannot rename default key */
        MessageBeep(MB_ICONHAND);
        return 0;
    }
    return (HWND)SendMessageW(hwndLV, LVM_EDITLABELW, item, 0);
}

HWND CreateListView(HWND hwndParent, UINT id)
{
    RECT rcClient;
    HWND hwndLV;

    /* prepare strings */
    LoadStringW(hInst, IDS_REGISTRY_VALUE_NOT_SET, g_szValueNotSet, ARRAY_SIZE(g_szValueNotSet));

    /* Get the dimensions of the parent window's client area, and create the list view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndLV = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"List View",
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS |
                            LVS_REPORT | LVS_EDITLABELS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, ULongToHandle(id), hInst, NULL);
    if (!hwndLV) return NULL;
    SendMessageW(hwndLV, LVM_SETUNICODEFORMAT, TRUE, 0);
    SendMessageW(hwndLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    /* Initialize the image list */
    if (!InitListViewImageList(hwndLV)) goto fail;
    if (!CreateListColumns(hwndLV)) goto fail;
    return hwndLV;
fail:
    DestroyWindow(hwndLV);
    return NULL;
}

BOOL RefreshListView(HWND hwndLV, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR highlightValue)
{
    BOOL result = FALSE;
    DWORD max_sub_key_len;
    DWORD max_val_name_len, valNameLen;
    DWORD max_val_size, valSize;
    DWORD val_count, index, valType;
    WCHAR* valName = 0;
    BYTE* valBuf = 0;
    HKEY hKey = 0;
    LONG errCode;
    LVITEMW item;

    if (!hwndLV) return FALSE;

    SendMessageW(hwndLV, WM_SETREDRAW, FALSE, 0);

    errCode = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ, &hKey);
    if (errCode != ERROR_SUCCESS) goto done;

    g_columnToSort = ~0U;
    SendMessageW(hwndLV, LVM_DELETEALLITEMS, 0, 0);

    /* get size information and resize the buffers if necessary */
    errCode = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, &max_sub_key_len, NULL,
                              &val_count, &max_val_name_len, &max_val_size, NULL, NULL);
    if (errCode != ERROR_SUCCESS) goto done;

    /* account for the terminator char */
    max_val_name_len++;
    max_val_size++;

    valName = malloc(max_val_name_len * sizeof(WCHAR));
    valBuf = malloc(max_val_size);

    valSize = max_val_size;
    if (RegQueryValueExW(hKey, NULL, NULL, &valType, valBuf, &valSize) == ERROR_FILE_NOT_FOUND) {
        AddEntryToList(hwndLV, NULL, REG_SZ, NULL, 0, -1);
    }
    for(index = 0; index < val_count; index++) {
        valNameLen = max_val_name_len;
        valSize = max_val_size;
	valType = 0;
        errCode = RegEnumValueW(hKey, index, valName, &valNameLen, NULL, &valType, valBuf, &valSize);
	if (errCode != ERROR_SUCCESS) goto done;
        valBuf[valSize] = 0;
        AddEntryToList(hwndLV, valName[0] ? valName : NULL, valType, valBuf, valSize, -1);
    }

    memset(&item, 0, sizeof(item));
    if (!highlightValue)
    {
        item.state = item.stateMask = LVIS_FOCUSED;
        SendMessageW(hwndLV, LVM_SETITEMSTATE, 0, (LPARAM)&item);
    }

    SendMessageW(hwndLV, LVM_SORTITEMS, (WPARAM)hwndLV, (LPARAM)CompareFunc);

    g_currentRootKey = hKeyRoot;
    if (keyPath != g_currentPath && !update_listview_path(keyPath))
        goto done;

    result = TRUE;

done:
    free(valBuf);
    free(valName);
    SendMessageW(hwndLV, WM_SETREDRAW, TRUE, 0);
    if (hKey) RegCloseKey(hKey);

    return result;
}
