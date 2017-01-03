/*
 * Regedit child window
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
#include <stdio.h>

#include "main.h"
#include "regproc.h"

#include "wine/debug.h"
#include "wine/unicode.h"
                                                                                                                             
WINE_DEFAULT_DEBUG_CHANNEL(regedit);

ChildWnd* g_pChildWnd;
static int last_split;

static const WCHAR wszLastKey[] = {'L','a','s','t','K','e','y',0};
static const WCHAR wszKeyName[] = {'S','o','f','t','w','a','r','e','\\',
                                   'M','i','c','r','o','s','o','f','t','\\',
                                   'W','i','n','d','o','w','s','\\',
                                   'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                   'A','p','p','l','e','t','s','\\','R','e','g','e','d','i','t',0};

/*******************************************************************************
 * Local module support methods
 */

static LPCWSTR GetRootKeyName(HKEY hRootKey)
{
    if(hRootKey == HKEY_CLASSES_ROOT)
        return reg_class_namesW[INDEX_HKEY_CLASSES_ROOT];
    if(hRootKey == HKEY_CURRENT_USER)
        return reg_class_namesW[INDEX_HKEY_CURRENT_USER];
    if(hRootKey == HKEY_LOCAL_MACHINE)
        return reg_class_namesW[INDEX_HKEY_LOCAL_MACHINE];
    if(hRootKey == HKEY_USERS)
        return reg_class_namesW[INDEX_HKEY_USERS];
    if(hRootKey == HKEY_CURRENT_CONFIG)
        return reg_class_namesW[INDEX_HKEY_CURRENT_CONFIG];
    if(hRootKey == HKEY_DYN_DATA)
        return reg_class_namesW[INDEX_HKEY_DYN_DATA];
    else
    {
        static const WCHAR unknown_key[] = {'U','N','K','N','O','W','N',' ','H','K','E','Y',',',' ',
                                            'P','L','E','A','S','E',' ','R','E','P','O','R','T',0};
        return unknown_key;
    }
}

static void draw_splitbar(HWND hWnd, int x)
{
    RECT rt;
    HDC hdc = GetDC(hWnd);

    GetClientRect(hWnd, &rt);
    rt.left = x - SPLIT_WIDTH/2;
    rt.right = x + SPLIT_WIDTH/2+1;
    InvertRect(hdc, &rt);
    ReleaseDC(hWnd, hdc);
}

static void ResizeWnd(int cx, int cy)
{
    HDWP hdwp = BeginDeferWindowPos(2);
    RECT rt = {0, 0, cx, cy};

    cx = g_pChildWnd->nSplitPos + SPLIT_WIDTH/2;
    DeferWindowPos(hdwp, g_pChildWnd->hTreeWnd, 0, rt.left, rt.top, g_pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, g_pChildWnd->hListWnd, 0, rt.left+cx  , rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    EndDeferWindowPos(hdwp);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rt;

    GetClientRect(hWnd, &rt);
    BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetSysColorBrush(COLOR_BTNFACE));
    EndPaint(hWnd, &ps);
}

static LPWSTR CombinePaths(LPCWSTR pPaths[], int nPaths) {
    int i, len, pos;
    LPWSTR combined;
    for (i=0, len=0; i<nPaths; i++) {
        if (pPaths[i] && *pPaths[i]) {
            len += lstrlenW(pPaths[i])+1;
        }
    }
    combined = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    *combined = '\0';
    for (i=0, pos=0; i<nPaths; i++) {
        if (pPaths[i] && *pPaths[i]) {
            int llen = lstrlenW(pPaths[i]);
            if (!*combined)
                lstrcpyW(combined, pPaths[i]);
            else {
                combined[pos++] = '\\';
                lstrcpyW(combined+pos, pPaths[i]);
            }
            pos += llen;
        }
    }
    return combined;
}

static LPWSTR GetPathRoot(HWND hwndTV, HTREEITEM hItem, BOOL bFull) {
    LPCWSTR parts[2] = {0,0};
    WCHAR text[260];
    HKEY hRootKey = NULL;
    if (!hItem)
        hItem = (HTREEITEM)SendMessageW(hwndTV, TVM_GETNEXTITEM, TVGN_CARET, 0);
    HeapFree(GetProcessHeap(), 0, GetItemPath(hwndTV, hItem, &hRootKey));
    if (!bFull && !hRootKey)
        return NULL;
    if (hRootKey)
        parts[1] = GetRootKeyName(hRootKey);
    if (bFull) {
        DWORD dwSize = sizeof(text)/sizeof(WCHAR);
        GetComputerNameW(text, &dwSize);
        parts[0] = text;
    }
    return CombinePaths(parts, 2);
}

LPWSTR GetItemFullPath(HWND hwndTV, HTREEITEM hItem, BOOL bFull) {
    LPWSTR parts[2];
    LPWSTR ret;
    HKEY hRootKey = NULL;

    parts[0] = GetPathRoot(hwndTV, hItem, bFull);
    parts[1] = GetItemPath(hwndTV, hItem, &hRootKey);
    ret = CombinePaths((LPCWSTR *)parts, 2);
    HeapFree(GetProcessHeap(), 0, parts[0]);
    HeapFree(GetProcessHeap(), 0, parts[1]);
    return ret;
}

static LPWSTR GetPathFullPath(HWND hwndTV, LPWSTR path) {
    LPWSTR parts[2];
    LPWSTR ret;

    parts[0] = GetPathRoot(hwndTV, 0, TRUE);
    parts[1] = path;
    ret = CombinePaths((LPCWSTR*)parts, 2);
    HeapFree(GetProcessHeap(), 0, parts[0]);
    return ret;
}

static void OnTreeSelectionChanged(HWND hwndTV, HWND hwndLV, HTREEITEM hItem, BOOL bRefreshLV)
{
    if (bRefreshLV) {
        LPWSTR keyPath;
        HKEY hRootKey = NULL;
        keyPath = GetItemPath(hwndTV, hItem, &hRootKey);
        RefreshListView(hwndLV, hRootKey, keyPath, NULL);
        HeapFree(GetProcessHeap(), 0, keyPath);
    }
    UpdateStatusBar();
}

/*******************************************************************************
 * finish_splitbar [internal]
 *
 * make the splitbar invisible and resize the windows
 * (helper for ChildWndProc)
 */
static void finish_splitbar(HWND hWnd, int x)
{
    RECT rt;

    draw_splitbar(hWnd, last_split);
    last_split = -1;
    GetClientRect(hWnd, &rt);
    g_pChildWnd->nSplitPos = x;
    ResizeWnd(rt.right, rt.bottom);
    ReleaseCapture();
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
    switch (LOWORD(wParam)) {
        /* Parse the menu selections: */
    case ID_REGISTRY_EXIT:
        DestroyWindow(hWnd);
        break;
    case ID_VIEW_REFRESH:
        WINE_TRACE("Is this ever called or is it just dead code?\n");
        /* TODO */
        break;
    case ID_SWITCH_PANELS:
        g_pChildWnd->nFocusPanel = !g_pChildWnd->nFocusPanel;
        SetFocus(g_pChildWnd->nFocusPanel? g_pChildWnd->hListWnd: g_pChildWnd->hTreeWnd);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/*******************************************************************************
 * get_last_key [internal]
 *
 * open last key
 *
 */
static void get_last_key(HWND hwndTV)
{
    HKEY hkey;
    WCHAR wszVal[KEY_MAX_LEN];
    DWORD dwSize = sizeof(wszVal);

    if (RegCreateKeyExW(HKEY_CURRENT_USER, wszKeyName, 0, NULL, 0, KEY_READ, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hkey, wszLastKey, NULL, NULL, (LPBYTE)wszVal, &dwSize) == ERROR_SUCCESS)
        {
            HTREEITEM selection;
            if (!strcmpW(wszVal, g_pChildWnd->szPath))
                selection = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            else
                selection = FindPathInTree(hwndTV, wszVal);
            SendMessageW(hwndTV, TVM_SELECTITEM, TVGN_CARET, (LPARAM)selection);
        }

        RegCloseKey(hkey);
    }
}

/*******************************************************************************
 * set_last_key [internal]
 *
 * save last key
 *
 */
static void set_last_key(HWND hwndTV)
{
    HKEY hkey;

    if (RegCreateKeyExW(HKEY_CURRENT_USER, wszKeyName, 0, NULL, 0, KEY_WRITE, NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        HTREEITEM selection = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
        HTREEITEM root = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        WCHAR *value;

        if (selection == root)
            value = g_pChildWnd->szPath;
        else
            value = GetItemFullPath(g_pChildWnd->hTreeWnd, selection, FALSE);
        RegSetValueExW(hkey, wszLastKey, 0, REG_SZ, (LPBYTE)value, (lstrlenW(value) + 1) * sizeof(WCHAR));
        if (selection != root)
            HeapFree(GetProcessHeap(), 0, value);
        RegCloseKey(hkey);
    }
}

/*******************************************************************************
 *
 *  FUNCTION: ChildWndProc(HWND, unsigned, WORD, LONG)
 *
 *  PURPOSE:  Processes messages for the child windows.
 *
 *  WM_COMMAND  - process the application menu
 *  WM_PAINT    - Paint the main window
 *  WM_DESTROY  - post a quit message and return
 *
 */
LRESULT CALLBACK ChildWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        g_pChildWnd = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
        if (!g_pChildWnd) return 0;
        LoadStringW(hInst, IDS_REGISTRY_ROOT_NAME, g_pChildWnd->szPath, MAX_PATH);
        g_pChildWnd->nSplitPos = 250;
        g_pChildWnd->hWnd = hWnd;
        g_pChildWnd->hTreeWnd = CreateTreeView(hWnd, g_pChildWnd->szPath, TREE_WINDOW);
        g_pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, g_pChildWnd->szPath*/);
        g_pChildWnd->nFocusPanel = 1;
        SetFocus(g_pChildWnd->hTreeWnd);
        get_last_key(g_pChildWnd->hTreeWnd);
        break;
    case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            goto def;
        }
        break;
    case WM_PAINT:
        OnPaint(hWnd);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            if (pt.x>=g_pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<g_pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                SetCursor(LoadCursorW(0, (LPCWSTR)IDC_SIZEWE));
                return TRUE;
            }
        }
        goto def;
    case WM_DESTROY:
        set_last_key(g_pChildWnd->hTreeWnd);
        HeapFree(GetProcessHeap(), 0, g_pChildWnd);
        g_pChildWnd = NULL;
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN: {
            RECT rt;
            int x = (short)LOWORD(lParam);
            GetClientRect(hWnd, &rt);
            if (x>=g_pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<g_pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                last_split = g_pChildWnd->nSplitPos;
                draw_splitbar(hWnd, last_split);
                SetCapture(hWnd);
            }
            break;
        }

    /* WM_RBUTTONDOWN sets the splitbar the same way as WM_LBUTTONUP */
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
        if (GetCapture() == hWnd) {
            finish_splitbar(hWnd, LOWORD(lParam));
        }
        break;

    case WM_CAPTURECHANGED:
        if (GetCapture()==hWnd && last_split>=0)
            draw_splitbar(hWnd, last_split);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            if (GetCapture() == hWnd) {
                RECT rt;
                draw_splitbar(hWnd, last_split);
                GetClientRect(hWnd, &rt);
                ResizeWnd(rt.right, rt.bottom);
                last_split = -1;
                ReleaseCapture();
                SetCursor(LoadCursorW(0, (LPCWSTR)IDC_ARROW));
            }
        break;

    case WM_MOUSEMOVE:
        if (GetCapture() == hWnd) {
            RECT rt;
            int x = LOWORD(lParam);
            HDC hdc = GetDC(hWnd);
            GetClientRect(hWnd, &rt);
            rt.left = last_split-SPLIT_WIDTH/2;
            rt.right = last_split+SPLIT_WIDTH/2+1;
            InvertRect(hdc, &rt);
            last_split = x;
            rt.left = x-SPLIT_WIDTH/2;
            rt.right = x+SPLIT_WIDTH/2+1;
            InvertRect(hdc, &rt);
            ReleaseDC(hWnd, hdc);
        }
        break;

    case WM_SETFOCUS:
        if (g_pChildWnd != NULL) {
            SetFocus(g_pChildWnd->nFocusPanel? g_pChildWnd->hListWnd: g_pChildWnd->hTreeWnd);
        }
        break;

    case WM_TIMER:
        break;

    case WM_NOTIFY:
        if (((int)wParam == TREE_WINDOW) && (g_pChildWnd != NULL)) {
            switch (((LPNMHDR)lParam)->code) {
            case TVN_ITEMEXPANDINGW:
                return !OnTreeExpanding(g_pChildWnd->hTreeWnd, (NMTREEVIEWW*)lParam);
            case TVN_SELCHANGEDW:
                OnTreeSelectionChanged(g_pChildWnd->hTreeWnd, g_pChildWnd->hListWnd,
                    ((NMTREEVIEWW *)lParam)->itemNew.hItem, TRUE);
                break;
	    case NM_SETFOCUS:
		g_pChildWnd->nFocusPanel = 0;
		break;
            case NM_RCLICK: {
		POINT pt;
                GetCursorPos(&pt);
		TrackPopupMenu(GetSubMenu(hPopupMenus, PM_NEW),
			       TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
		break;
            }
            case TVN_BEGINLABELEDITW: {
                HKEY hRootKey;
                LPWSTR path;

                if (!GetWindowLongPtrW(g_pChildWnd->hTreeWnd, GWLP_USERDATA))
                    return 1;

                path = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
                if (!path || !*path) return 1;
                return 0;
            }
	    case TVN_ENDLABELEDITW: {
		HKEY hRootKey;
	        LPNMTVDISPINFOW dispInfo = (LPNMTVDISPINFOW)lParam;
		LPWSTR path = GetItemPath(g_pChildWnd->hTreeWnd, 0, &hRootKey);
	        BOOL res = RenameKey(hWnd, hRootKey, path, dispInfo->item.pszText);
		if (res) {
		    TVITEMEXW item;
                    LPWSTR fullPath = GetPathFullPath(g_pChildWnd->hTreeWnd,
                     dispInfo->item.pszText);
		    item.mask = TVIF_HANDLE | TVIF_TEXT;
		    item.hItem = (HTREEITEM)SendMessageW(g_pChildWnd->hTreeWnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
		    item.pszText = dispInfo->item.pszText;
                    SendMessageW( g_pChildWnd->hTreeWnd, TVM_SETITEMW, 0, (LPARAM)&item );
                    SendMessageW(hStatusBar, SB_SETTEXTW, 0, (LPARAM)fullPath);
                    HeapFree(GetProcessHeap(), 0, fullPath);
		}
                HeapFree(GetProcessHeap(), 0, path);
                SetWindowLongPtrW(g_pChildWnd->hTreeWnd, GWLP_USERDATA, 0);
		return res;
	    }
            default:
                return 0; /* goto def; */
            }
        } else
            if (((int)wParam == LIST_WINDOW) && (g_pChildWnd != NULL)) {
		if (((LPNMHDR)lParam)->code == NM_SETFOCUS) {
		    g_pChildWnd->nFocusPanel = 1;
		} else if (!SendMessageW(g_pChildWnd->hListWnd, WM_NOTIFY_REFLECT, wParam, lParam)) {
                    goto def;
                }
            }
        break;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && g_pChildWnd != NULL) {
            ResizeWnd(LOWORD(lParam), HIWORD(lParam));
        }
        /* fall through */
default: def:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}
