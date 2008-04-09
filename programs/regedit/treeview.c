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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <stdio.h>
#include <wine/debug.h>
#include <shlwapi.h>

#include "main.h"
#include "regproc.h"

WINE_DEFAULT_DEBUG_CHANNEL(regedit);

/* Global variables and constants  */
/* Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images.  */
/* CX_BITMAP and CY_BITMAP - width and height of an icon.  */
/* NUM_BITMAPS - number of bitmaps to add to the image list.  */
int Image_Open;
int Image_Closed;
int Image_Root;

#define CX_ICON    16
#define CY_ICON    16
#define NUM_ICONS    3

static BOOL UpdateExpandingTree(HWND hwndTV, HTREEITEM hItem, int state);

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

LPTSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey)
{
    int pathLen = 0, maxLen;
    TCHAR *pathBuffer;

    pathBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!pathBuffer) return NULL;
    *pathBuffer = 0;
    maxLen = HeapSize(GetProcessHeap(), 0, pathBuffer);
    if (maxLen == (SIZE_T) - 1) return NULL;
    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return NULL;
    if (!get_item_path(hwndTV, hItem, phRootKey, &pathBuffer, &pathLen, &maxLen)) return NULL;
    return pathBuffer;
}

static LPTSTR get_path_component(LPCTSTR *lplpKeyName) {
     LPCTSTR lpPos = *lplpKeyName;
     LPTSTR lpResult = NULL;
     int len;
     if (!lpPos)
         return NULL;
     while(*lpPos && *lpPos != '\\')
         lpPos++;
     if (*lpPos && lpPos == *lplpKeyName)
         return NULL;
     len = (lpPos+1-(*lplpKeyName)) * sizeof(TCHAR);
     lpResult = HeapAlloc(GetProcessHeap(), 0, len);
     if (!lpResult) /* that would be very odd */
         return NULL;
     memcpy(lpResult, *lplpKeyName, len-1);
     lpResult[len-1] = '\0';
     *lplpKeyName = *lpPos ? lpPos+1 : NULL;
     return lpResult;
}

HTREEITEM FindPathInTree(HWND hwndTV, LPCTSTR lpKeyName) {
    TVITEMEX tvi;
    TCHAR buf[261]; /* tree view has 260 character limitation on item name */
    HTREEITEM hItem, hOldItem;

    buf[260] = '\0';
    hItem = TreeView_GetRoot(hwndTV);
    SendMessage(hwndTV, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItem );
    hItem = TreeView_GetChild(hwndTV, hItem);
    hOldItem = hItem;
    while(1) {
        LPTSTR lpItemName = get_path_component(&lpKeyName);
        if (lpItemName) {
            while(hItem) {
                tvi.mask = TVIF_TEXT | TVIF_HANDLE;
                tvi.hItem = hItem;
                tvi.pszText = buf;
                tvi.cchTextMax = 260;
                SendMessage(hwndTV, TVM_GETITEM, 0, (LPARAM) &tvi);
                if (!_tcsicmp(tvi.pszText, lpItemName)) {
                     SendMessage(hwndTV, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItem );
                     if (!lpKeyName)
                         return hItem;
                     hOldItem = hItem;
                     hItem = TreeView_GetChild(hwndTV, hItem);
                     break;
                }
                hItem = TreeView_GetNextSibling(hwndTV, hItem);
            }
            HeapFree(GetProcessHeap(), 0, lpItemName);
            if (!hItem)
                return hOldItem;
        }
        else
            return hItem;
    }
}

BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem)
{
    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return FALSE;
    return TreeView_DeleteItem(hwndTV, hItem);
}

/* Add an entry to the tree. Only give hKey for root nodes (HKEY_ constants) */
static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPTSTR label, HKEY hKey, DWORD dwChildren)
{
    TVINSERTSTRUCT tvins;

    if (hKey) {
        if (RegQueryInfoKey(hKey, 0, 0, 0, &dwChildren, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
            dwChildren = 0;
        }
    }

    tvins.u.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvins.u.item.pszText = label;
    tvins.u.item.cchTextMax = lstrlen(label);
    tvins.u.item.iImage = Image_Closed;
    tvins.u.item.iSelectedImage = Image_Open;
    tvins.u.item.cChildren = dwChildren;
    tvins.u.item.lParam = (LPARAM)hKey;
    tvins.hInsertAfter = (HTREEITEM)(hKey ? TVI_LAST : TVI_SORT);
    tvins.hParent = hParent;
    return TreeView_InsertItem(hwndTV, &tvins);
}

static BOOL match_string(LPCTSTR sstring1, LPCTSTR sstring2, int mode)
{
    if (mode & SEARCH_WHOLE)
        return !stricmp(sstring1, sstring2);
    else
        return NULL != StrStrI(sstring1, sstring2);
}

static BOOL match_item(HWND hwndTV, HTREEITEM hItem, LPCTSTR sstring, int mode, int *row)
{
    TVITEM item;
    TCHAR keyname[KEY_MAX_LEN];
    item.mask = TVIF_TEXT;
    item.hItem = hItem;
    item.pszText = keyname;
    item.cchTextMax = KEY_MAX_LEN;
    if (!TreeView_GetItem(hwndTV, &item)) return FALSE;
    if ((mode & SEARCH_KEYS) && match_string(keyname, sstring, mode)) {
        *row = -1;
        return TRUE;
    }

    if (mode & (SEARCH_VALUES | SEARCH_CONTENT)) {
        int i, adjust;
        TCHAR valName[KEY_MAX_LEN], *KeyPath;
        HKEY hKey, hRoot;
        DWORD lenName;
        
        KeyPath = GetItemPath(hwndTV, hItem, &hRoot);

        if (!KeyPath || !hRoot)
             return FALSE;

        if (RegOpenKeyEx(hRoot, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            HeapFree(GetProcessHeap(), 0, KeyPath);
            return FALSE;
        }
            
        HeapFree(GetProcessHeap(), 0, KeyPath);
        lenName = KEY_MAX_LEN;
        adjust = 0;
        /* RegEnumValue won't return empty default value, so fake it when dealing with *row,
           which corresponds to list view rows, not value ids */
        if (ERROR_SUCCESS == RegEnumValue(hKey, 0, valName, &lenName, NULL, NULL, NULL, NULL) && *valName)
            adjust = 1;
        
        i = (*row)-adjust;
        if (i < 0) i = 0;
        while(1) {
            DWORD lenValue = 0, type = 0;
            lenName = KEY_MAX_LEN;
            
            if (ERROR_SUCCESS != RegEnumValue(hKey, 
                i, valName, &lenName, NULL, &type, NULL, &lenValue))
                break;
            
            if (mode & SEARCH_VALUES) {
                if (match_string(valName, sstring, mode)) {
                    RegCloseKey(hKey);
                    *row = i+adjust;
                    return TRUE;
                }
            }
            
            if ((mode & SEARCH_CONTENT) && (type == REG_EXPAND_SZ || type == REG_SZ)) {
                LPTSTR buffer;
                buffer = HeapAlloc(GetProcessHeap(), 0, lenValue);
                RegEnumValue(hKey, i, valName, &lenName, NULL, &type, (LPBYTE)buffer, &lenValue);
                if (match_string(buffer, sstring, mode)) {
                    HeapFree(GetProcessHeap(), 0, buffer);
                    RegCloseKey(hKey);
                    *row = i+adjust;
                    return TRUE;
                }
                HeapFree(GetProcessHeap(), 0, buffer);
            }
                            
            i++;
        }
        RegCloseKey(hKey);
    }        
    return FALSE;
}

HTREEITEM FindNext(HWND hwndTV, HTREEITEM hItem, LPCTSTR sstring, int mode, int *row)
{
    HTREEITEM hTry, hLast;
    
    hLast = hItem;
    (*row)++;
    if (match_item(hwndTV, hLast, sstring, mode & ~SEARCH_KEYS, row)) {
        return hLast;
    }
    *row = 0;
    
    while(hLast) {
        /* first look in subtree */
        /* no children? maybe we haven't loaded them yet? */
        if (!TreeView_GetChild(hwndTV, hLast)) {
            UpdateExpandingTree(hwndTV, hLast, TreeView_GetItemState(hwndTV, hLast, -1));
        }
        hTry = TreeView_GetChild(hwndTV, hLast);
        if (hTry) {
            if (match_item(hwndTV, hTry, sstring, mode, row))
                return hTry;
            hLast = hTry;
            continue;
        }
        /* no more children, maybe there are any siblings? */
        hTry = TreeView_GetNextSibling(hwndTV, hLast);
        if (hTry) {
            if (match_item(hwndTV, hTry, sstring, mode, row))
                return hTry;
            hLast = hTry;
            continue;
        }
        /* no more siblings, look at the next siblings in parent(s) */
        hLast = TreeView_GetParent(hwndTV, hLast);
        if (!hLast)
            return NULL;
        while (hLast && (hTry = TreeView_GetNextSibling(hwndTV, hLast)) == NULL) {
            hLast = TreeView_GetParent(hwndTV, hLast);
        }
        if (match_item(hwndTV, hTry, sstring, mode, row))
            return hTry;
        hLast = hTry;
    }
    return NULL;
}

static BOOL RefreshTreeItem(HWND hwndTV, HTREEITEM hItem)
{
    HKEY hRoot, hKey, hSubKey;
    HTREEITEM childItem;
    LPTSTR KeyPath;
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    LPSTR Name;
    TVITEM tvItem;
    
    hRoot = NULL;
    KeyPath = GetItemPath(hwndTV, hItem, &hRoot);

    if (!KeyPath || !hRoot)
        return FALSE;

    if (*KeyPath) {
        if (RegOpenKeyEx(hRoot, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            WINE_TRACE("RegOpenKeyEx failed, \"%s\" was probably removed.\n", KeyPath);
            return FALSE;
        }
    } else {
        hKey = hRoot;
    }
    HeapFree(GetProcessHeap(), 0, KeyPath);

    if (RegQueryInfoKey(hKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
        return FALSE;
    }

    /* Set the number of children again */
    tvItem.mask = TVIF_CHILDREN;
    tvItem.hItem = hItem;
    tvItem.cChildren = dwCount;
    if (!TreeView_SetItem(hwndTV, &tvItem)) {
        return FALSE;
    }

    /* We don't have to bother with the rest if it's not expanded. */
    if (TreeView_GetItemState(hwndTV, hItem, TVIS_EXPANDED) == 0) {
        RegCloseKey(hKey);
        return TRUE;
    }

    dwMaxSubKeyLen++; /* account for the \0 terminator */
    if (!(Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR)))) {
        return FALSE;
    }
    tvItem.cchTextMax = dwMaxSubKeyLen;
    if (!(tvItem.pszText = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR)))) {
        return FALSE;
    }

    /* Now go through all the children in the registry, and check if any have to be added. */
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++) {
        DWORD cName = dwMaxSubKeyLen, dwSubCount;
        BOOL found;

        found = FALSE;
        if (RegEnumKeyEx(hKey, dwIndex, Name, &cName, 0, 0, 0, NULL) != ERROR_SUCCESS) {
            continue;
        }

        /* Find the number of children of the node. */
        dwSubCount = 0;
        if (RegOpenKeyEx(hKey, Name, 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS) {
            if (RegQueryInfoKey(hSubKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
                dwSubCount = 0;
            }
            RegCloseKey(hSubKey);
        }

        /* Check if the node is already in there. */
        for (childItem = TreeView_GetChild(hwndTV, hItem); childItem;
                childItem = TreeView_GetNextSibling(hwndTV, childItem)) {
            tvItem.mask = TVIF_TEXT;
            tvItem.hItem = childItem;
            if (!TreeView_GetItem(hwndTV, &tvItem)) {
                return FALSE;
            }

            if (!stricmp(tvItem.pszText, Name)) {
                found = TRUE;
                break;
            }
        }

        if (found == FALSE) {
            WINE_TRACE("New subkey %s\n", Name);
            AddEntryToTree(hwndTV, hItem, Name, NULL, dwSubCount);
        }
    }
    HeapFree(GetProcessHeap(), 0, Name);
    HeapFree(GetProcessHeap(), 0, tvItem.pszText);
    RegCloseKey(hKey);

    /* Now go through all the children in the tree, and check if any have to be removed. */
    childItem = TreeView_GetChild(hwndTV, hItem);
    while (childItem) {
        HTREEITEM nextItem = TreeView_GetNextSibling(hwndTV, childItem);
        if (RefreshTreeItem(hwndTV, childItem) == FALSE) {
            SendMessage(hwndTV, TVM_DELETEITEM, 0, (LPARAM)childItem);
        }
        childItem = nextItem;
    }

    return TRUE;
}

BOOL RefreshTreeView(HWND hwndTV)
{
    HTREEITEM hItem;
    HTREEITEM hSelectedItem;
    HCURSOR hcursorOld;

    WINE_TRACE("\n");
    hSelectedItem = TreeView_GetSelection(hwndTV);
    hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SendMessage(hwndTV, WM_SETREDRAW, FALSE, 0);

    hItem = TreeView_GetChild(hwndTV, TreeView_GetRoot(hwndTV));
    while (hItem) {
        RefreshTreeItem(hwndTV, hItem);
        hItem = TreeView_GetNextSibling(hwndTV, hItem);
    }

    SendMessage(hwndTV, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwndTV, NULL, FALSE);
    SetCursor(hcursorOld);
    
    /* We reselect the currently selected node, this will prompt a refresh of the listview. */
    SendMessage(hwndTV, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hSelectedItem);
    return TRUE;
}

HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPTSTR name)
{
    TCHAR buf[MAX_NEW_KEY_LEN];
    HTREEITEM hNewItem = 0;
    TVITEMEX item;

    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return FALSE;
    if (TreeView_GetItemState(hwndTV, hItem, TVIS_EXPANDEDONCE)) {
	hNewItem = AddEntryToTree(hwndTV, hItem, name, 0, 0);
    } else {
	item.mask = TVIF_CHILDREN | TVIF_HANDLE;
	item.hItem = hItem;
	if (!TreeView_GetItem(hwndTV, &item)) return FALSE;
	item.cChildren = 1;
	if (!TreeView_SetItem(hwndTV, &item)) return FALSE;
    }
    SendMessage(hwndTV, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItem );
    if (!hNewItem) {
	for(hNewItem = TreeView_GetChild(hwndTV, hItem); hNewItem; hNewItem = TreeView_GetNextSibling(hwndTV, hNewItem)) {
	    item.mask = TVIF_HANDLE | TVIF_TEXT;
	    item.hItem = hNewItem;
	    item.pszText = buf;
	    item.cchTextMax = COUNT_OF(buf);
	    if (!TreeView_GetItem(hwndTV, &item)) continue;
	    if (lstrcmp(name, item.pszText) == 0) break;
	}	
    }
    if (hNewItem)
        SendMessage(hwndTV, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hNewItem);

    return hNewItem;
}

HWND StartKeyRename(HWND hwndTV)
{
    HTREEITEM hItem;

    if(!(hItem = TreeView_GetSelection(hwndTV))) return 0;
    return TreeView_EditLabel(hwndTV, hItem);
}

static BOOL InitTreeViewItems(HWND hwndTV, LPTSTR pHostName)
{
    TVINSERTSTRUCT tvins;
    HTREEITEM hRoot;
    static TCHAR hkcr[] = {'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T',0},
                 hkcu[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R',0},
                 hklm[] = {'H','K','E','Y','_','L','O','C','A','L','_','M','A','C','H','I','N','E',0},
                 hku[]  = {'H','K','E','Y','_','U','S','E','R','S',0},
                 hkcc[] = {'H','K','E','Y','_','C','U','R','R','E','N','T','_','C','O','N','F','I','G',0},
                 hkdd[] = {'H','K','E','Y','_','D','Y','N','_','D','A','T','A',0};

    tvins.u.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    /* Set the text of the item.  */
    tvins.u.item.pszText = pHostName;
    tvins.u.item.cchTextMax = lstrlen(pHostName);
    /* Assume the item is not a parent item, so give it an image.  */
    tvins.u.item.iImage = Image_Root;
    tvins.u.item.iSelectedImage = Image_Root;
    tvins.u.item.cChildren = 5;
    /* Save the heading level in the item's application-defined data area.  */
    tvins.u.item.lParam = (LPARAM)NULL;
    tvins.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvins.hParent = TVI_ROOT;
    /* Add the item to the tree view control.  */
    if (!(hRoot = TreeView_InsertItem(hwndTV, &tvins))) return FALSE;

    if (!AddEntryToTree(hwndTV, hRoot, hkcr, HKEY_CLASSES_ROOT, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, hkcu, HKEY_CURRENT_USER, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, hklm, HKEY_LOCAL_MACHINE, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, hku, HKEY_USERS, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, hkcc, HKEY_CURRENT_CONFIG, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, hkdd, HKEY_DYN_DATA, 1)) return FALSE;

    /* expand and select host name */
    SendMessage(hwndTV, TVM_EXPAND, TVE_EXPAND, (LPARAM)hRoot );
    SendMessage(hwndTV, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hRoot);
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
    HICON hico;       /* handle to icon  */

    /* Create the image list.  */
    if ((himl = ImageList_Create(CX_ICON, CY_ICON,
                                 ILC_MASK, 0, NUM_ICONS)) == NULL)
        return FALSE;

    /* Add the open file, closed file, and document bitmaps.  */
    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_OPEN_FILE));
    Image_Open = ImageList_AddIcon(himl, hico);

    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLOSED_FILE));
    Image_Closed = ImageList_AddIcon(himl, hico);

    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ROOT));
    Image_Root = ImageList_AddIcon(himl, hico);

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < NUM_ICONS)
    {
      return FALSE;
    }

    /* Associate the image list with the tree view control.  */
    SendMessage(hwndTV, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)himl);

    return TRUE;
}

BOOL UpdateExpandingTree(HWND hwndTV, HTREEITEM hItem, int state)
{
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    HKEY hRoot, hNewKey, hKey;
    LPTSTR keyPath;
    LPTSTR Name;
    LONG errCode;
    HCURSOR hcursorOld;

    static int expanding;
    if (expanding) return FALSE;
    if (state & TVIS_EXPANDEDONCE ) {
        return TRUE;
    }
    expanding = TRUE;
    hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SendMessage(hwndTV, WM_SETREDRAW, FALSE, 0);

    keyPath = GetItemPath(hwndTV, hItem, &hRoot);
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

        errCode = RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, 0, 0, 0, 0);
        if (errCode != ERROR_SUCCESS) continue;
        errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_QUERY_VALUE, &hKey);
        if (errCode == ERROR_SUCCESS) {
            errCode = RegQueryInfoKey(hKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0);
            RegCloseKey(hKey);
        }
        if (errCode != ERROR_SUCCESS) dwSubCount = 0;
        AddEntryToTree(hwndTV, hItem, Name, NULL, dwSubCount);
    }
    RegCloseKey(hNewKey);
    HeapFree(GetProcessHeap(), 0, Name);

done:
    TreeView_SetItemState(hwndTV, hItem, TVIS_EXPANDEDONCE, TVIS_EXPANDEDONCE);
    SendMessage(hwndTV, WM_SETREDRAW, TRUE, 0);
    SetCursor(hcursorOld);
    expanding = FALSE;
    HeapFree(GetProcessHeap(), 0, keyPath);

    return TRUE;
}

BOOL OnTreeExpanding(HWND hwndTV, NMTREEVIEW* pnmtv)
{
    return UpdateExpandingTree(hwndTV, pnmtv->itemNew.hItem, pnmtv->itemNew.state);
}


/*
 * CreateTreeView - creates a tree view control.
 * Returns the handle to the new control if successful, or NULL otherwise.
 * hwndParent - handle to the control's parent window.
 */
HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, UINT id)
{
    RECT rcClient;
    HWND hwndTV;

    /* Get the dimensions of the parent window's client area, and create the tree view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndTV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, _T("Tree View"),
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, (HMENU)ULongToHandle(id), hInst, NULL);
    /* Initialize the image list, and add items to the control.  */
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pHostName)) {
        DestroyWindow(hwndTV);
        return NULL;
    }
    return hwndTV;
}
