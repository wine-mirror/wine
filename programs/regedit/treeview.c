/*
 * Regedit treeview
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

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include "main.h"

/* Global variables and constants  */
/* Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images.  */
/* CX_BITMAP and CY_BITMAP - width and height of an icon.  */
/* NUM_BITMAPS - number of bitmaps to add to the image list.  */
int Image_Open;
int Image_Closed;
int Image_Root;

static LPTSTR pathBuffer;

#define CX_BITMAP    16
#define CY_BITMAP    16
#define NUM_BITMAPS  3

static BOOL get_item_path(HWND hwndTV, HTREEITEM hItem, HKEY* phKey, LPTSTR* pKeyPath, int* pPathLen, int* pMaxLen)
{
    TVITEM item;
    int maxLen, len;
    LPTSTR newStr;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hwndTV, &item)) return FALSE;

    if (item.lParam) {
	/* found root key with valid key value */
	*phKey = (HKEY)item.lParam;
	return TRUE;
    }

    if(!get_item_path(hwndTV, TreeView_GetParent(hwndTV, hItem), phKey, pKeyPath, pPathLen, pMaxLen)) return FALSE;
    if (*pPathLen) {
        (*pKeyPath)[*pPathLen] = _T('\\');
        ++(*pPathLen);
    }

    do {
        item.mask = TVIF_TEXT;
        item.hItem = hItem;
        item.pszText = *pKeyPath + *pPathLen;
        item.cchTextMax = maxLen = *pMaxLen - *pPathLen;
        if (!TreeView_GetItem(hwndTV, &item)) return FALSE;
        len = _tcslen(item.pszText);
	if (len < maxLen - 1) {
            *pPathLen += len;
            break;
	}
	newStr = HeapReAlloc(GetProcessHeap(), 0, *pKeyPath, *pMaxLen * 2);
	if (!newStr) return FALSE;
	*pKeyPath = newStr;
	*pMaxLen *= 2;
    } while(TRUE);

    return TRUE;
}

LPCTSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey)
{
    int pathLen = 0, maxLen;

    if (!pathBuffer) pathBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!pathBuffer) return NULL;
    *pathBuffer = 0;
    maxLen = HeapSize(GetProcessHeap(), 0, pathBuffer);
    if (maxLen == (SIZE_T) - 1) return NULL;
    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return NULL;
    if (!get_item_path(hwndTV, hItem, phRootKey, &pathBuffer, &pathLen, &maxLen)) return NULL;
    return pathBuffer;
}

static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPTSTR label, HKEY hKey, DWORD dwChildren)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvi.pszText = label;
    tvi.cchTextMax = lstrlen(tvi.pszText);
    tvi.iImage = Image_Closed;
    tvi.iSelectedImage = Image_Open;
    tvi.cChildren = dwChildren;
    tvi.lParam = (LPARAM)hKey;
    tvins.u.item = tvi;
    tvins.hInsertAfter = (HTREEITEM)(hKey ? TVI_LAST : TVI_SORT);
    tvins.hParent = hParent;
    return TreeView_InsertItem(hwndTV, &tvins);
}


static BOOL InitTreeViewItems(HWND hwndTV, LPTSTR pHostName)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hRoot;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    /* Set the text of the item.  */
    tvi.pszText = pHostName;
    tvi.cchTextMax = lstrlen(tvi.pszText);
    /* Assume the item is not a parent item, so give it an image.  */
    tvi.iImage = Image_Root;
    tvi.iSelectedImage = Image_Root;
    tvi.cChildren = 5;
    /* Save the heading level in the item's application-defined data area.  */
    tvi.lParam = (LPARAM)NULL;
    tvins.u.item = tvi;
    tvins.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvins.hParent = TVI_ROOT;
    /* Add the item to the tree view control.  */
    if (!(hRoot = TreeView_InsertItem(hwndTV, &tvins))) return FALSE;

    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_USERS"), HKEY_USERS, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG, 1)) return FALSE;
    
    /* expand and select host name */
    TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);
    TreeView_Select(hwndTV, hRoot, TVGN_CARET);
    return TRUE;
}


/*
 * InitTreeViewImageLists - creates an image list, adds three bitmaps
 * to it, and associates the image list with a tree view control.
 * Returns TRUE if successful, or FALSE otherwise.
 * hwndTV - handle to the tree view control.
 */
static BOOL InitTreeViewImageLists(HWND hwndTV)
{
    HIMAGELIST himl;  /* handle to image list  */
    HBITMAP hbmp;     /* handle to bitmap  */

    /* Create the image list.  */
    if ((himl = ImageList_Create(CX_BITMAP, CY_BITMAP,
                                 FALSE, NUM_BITMAPS, 0)) == NULL)
        return FALSE;

    /* Add the open file, closed file, and document bitmaps.  */
    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OPEN_FILE));
    Image_Open = ImageList_Add(himl, hbmp, NULL);
    DeleteObject(hbmp);

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLOSED_FILE));
    Image_Closed = ImageList_Add(himl, hbmp, NULL);
    DeleteObject(hbmp);

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ROOT));
    Image_Root = ImageList_Add(himl, hbmp, NULL);
    DeleteObject(hbmp);

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < 3) return FALSE;

    /* Associate the image list with the tree view control.  */
    TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL);

    return TRUE;
}

BOOL OnTreeExpanding(HWND hwndTV, NMTREEVIEW* pnmtv)
{
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    HKEY hRoot, hNewKey, hKey;
    LPCTSTR keyPath;
    LPTSTR Name;
    LONG errCode;

    static int expanding;
    if (expanding) return FALSE;
    if (pnmtv->itemNew.state & TVIS_EXPANDEDONCE ) {
        return TRUE;
    }
    expanding = TRUE;

    keyPath = GetItemPath(hwndTV, pnmtv->itemNew.hItem, &hRoot);
    if (!keyPath) goto done;

    if (*keyPath) {
        errCode = RegOpenKeyEx(hRoot, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode != ERROR_SUCCESS) goto done;
    } else {
	hNewKey = hRoot;
    }

    errCode = RegQueryInfoKey(hNewKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0);
    if (errCode != ERROR_SUCCESS) goto done;
    dwMaxSubKeyLen++; /* account for the \0 terminator */
    Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR));
    if (!Name) goto done;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++) {
	DWORD cName = dwMaxSubKeyLen, dwSubCount;
        FILETIME LastWriteTime;

        errCode = RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, 0, 0, 0, &LastWriteTime);
	if (errCode != ERROR_SUCCESS) continue;
	errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_QUERY_VALUE, &hKey);
	if (errCode == ERROR_SUCCESS) {
	    errCode = RegQueryInfoKey(hKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0);
	    RegCloseKey(hKey);
	}
	if (errCode != ERROR_SUCCESS) dwSubCount = 0;
        AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwSubCount);
    }
    RegCloseKey(hNewKey);
    HeapFree(GetProcessHeap(), 0, Name);

done:
    expanding = FALSE;

    return TRUE;
}


/*
 * CreateTreeView - creates a tree view control.
 * Returns the handle to the new control if successful, or NULL otherwise.
 * hwndParent - handle to the control's parent window.
 */
HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, int id)
{
    RECT rcClient;
    HWND hwndTV;

    /* Get the dimensions of the parent window's client area, and create the tree view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndTV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, _T("Tree View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, (HMENU)id, hInst, NULL);
    /* Initialize the image list, and add items to the control.  */
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pHostName)) {
        DestroyWindow(hwndTV);
        return NULL;
    }
    return hwndTV;
}
