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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#include "main.h"

#include "wine/debug.h"
#include "wine/unicode.h"
                                                                                                                             
WINE_DEFAULT_DEBUG_CHANNEL(regedit);
                                                                                                                             
ChildWnd* g_pChildWnd;

/*******************************************************************************
 * Local module support methods
 */

LPCTSTR GetRootKeyName(HKEY hRootKey)
{
    if (hRootKey == HKEY_CLASSES_ROOT) return _T("HKEY_CLASSES_ROOT");
    if (hRootKey == HKEY_CURRENT_USER) return _T("HKEY_CURRENT_USER");
    if (hRootKey == HKEY_LOCAL_MACHINE) return _T("HKEY_LOCAL_MACHINE");
    if (hRootKey == HKEY_USERS) return _T("HKEY_USERS");
    if (hRootKey == HKEY_CURRENT_CONFIG) return _T("HKEY_CURRENT_CONFIG");
    if (hRootKey == HKEY_DYN_DATA) return _T("HKEY_DYN_DATA");
    return _T("UNKNOWN HKEY, PLEASE REPORT");
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

static void ResizeWnd(ChildWnd* pChildWnd, int cx, int cy)
{
    HDWP hdwp = BeginDeferWindowPos(2);
    RECT rt = {0, 0, cx, cy};

    cx = pChildWnd->nSplitPos + SPLIT_WIDTH/2;
    DeferWindowPos(hdwp, pChildWnd->hTreeWnd, 0, rt.left, rt.top, pChildWnd->nSplitPos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    DeferWindowPos(hdwp, pChildWnd->hListWnd, 0, rt.left+cx  , rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
    EndDeferWindowPos(hdwp);
}

static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    RECT rt;
    HDC hdc;

    GetClientRect(hWnd, &rt);
    hdc = BeginPaint(hWnd, &ps);
    FillRect(ps.hdc, &rt, GetSysColorBrush(COLOR_BTNFACE));
    EndPaint(hWnd, &ps);
}

static LPTSTR CombinePaths(LPCTSTR pPaths[], int nPaths) {
    int i, len, pos;
    LPTSTR combined;
    for (i=0, len=0; i<nPaths; i++) {
        if (pPaths[i] && *pPaths[i]) {
            len += lstrlen(pPaths[i])+1;
        }
    }
    combined = HeapAlloc(GetProcessHeap(), 0, len * sizeof(TCHAR));
    *combined = '\0';
    for (i=0, pos=0; i<nPaths; i++) {
        if (pPaths[i] && *pPaths[i]) {
            int llen = _tcslen(pPaths[i]);
            if (!*combined)
                _tcscpy(combined, pPaths[i]);
            else {
                combined[pos++] = (TCHAR)'\\';
                _tcscpy(combined+pos, pPaths[i]);
            }
            pos += llen;
        }
    }
    return combined;
}

LPTSTR GetItemFullPath(HWND hwndTV, HTREEITEM hItem, BOOL bFull) {
    LPCTSTR parts[3] = {_T(""), _T(""), _T("")};
    TCHAR text[260];
    HKEY hRootKey = NULL;
    if (!hItem)
        hItem = TreeView_GetSelection(hwndTV);
    parts[2] = GetItemPath(hwndTV, hItem, &hRootKey);
    if (!parts[2])
        parts[2] = _T("");
    if (!bFull && !hRootKey && !*parts[2])
        return NULL;
    if (hRootKey)
        parts[1] = GetRootKeyName(hRootKey);
    if (bFull) {
        DWORD dwSize = sizeof(text)/sizeof(TCHAR);
        GetComputerName(text, &dwSize);
        parts[0] = text;
    }
    return CombinePaths(parts, 3);
}

static void OnTreeSelectionChanged(HWND hwndTV, HWND hwndLV, HTREEITEM hItem, BOOL bRefreshLV)
{
    if (bRefreshLV) {
        LPCTSTR keyPath;
        HKEY hRootKey = NULL;
        keyPath = GetItemPath(hwndTV, hItem, &hRootKey);
        RefreshListView(hwndLV, hRootKey, keyPath, NULL);
    }
    UpdateStatusBar();
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
    ChildWnd* pChildWnd = g_pChildWnd;
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
        pChildWnd->nFocusPanel = !pChildWnd->nFocusPanel;
        SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        break;
    default:
        return FALSE;
    }
    return TRUE;
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
    static int last_split;
    ChildWnd* pChildWnd = g_pChildWnd;

    switch (message) {
    case WM_CREATE:
        g_pChildWnd = pChildWnd = HeapAlloc(GetProcessHeap(), 0, sizeof(ChildWnd));
        if (!pChildWnd) return 0;
        _tcsncpy(pChildWnd->szPath, _T("My Computer"), MAX_PATH);
        pChildWnd->nSplitPos = 250;
        pChildWnd->hWnd = hWnd;
        pChildWnd->hTreeWnd = CreateTreeView(hWnd, pChildWnd->szPath, TREE_WINDOW);
        pChildWnd->hListWnd = CreateListView(hWnd, LIST_WINDOW/*, pChildWnd->szPath*/);
        pChildWnd->nFocusPanel = 1;
        SetFocus(pChildWnd->hTreeWnd);
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
            if (pt.x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && pt.x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                SetCursor(LoadCursor(0, IDC_SIZEWE));
                return TRUE;
            }
        }
        goto def;
    case WM_DESTROY:
        HeapFree(GetProcessHeap(), 0, pChildWnd);
        pChildWnd = NULL;
        PostQuitMessage(0);
        break;
    case WM_LBUTTONDOWN: {
            RECT rt;
            int x = LOWORD(lParam);
            GetClientRect(hWnd, &rt);
            if (x>=pChildWnd->nSplitPos-SPLIT_WIDTH/2 && x<pChildWnd->nSplitPos+SPLIT_WIDTH/2+1) {
                last_split = pChildWnd->nSplitPos;
                draw_splitbar(hWnd, last_split);
                SetCapture(hWnd);
            }
            break;
        }

    case WM_LBUTTONUP:
        if (GetCapture() == hWnd) {
            RECT rt;
            int x = LOWORD(lParam);
            draw_splitbar(hWnd, last_split);
            last_split = -1;
            GetClientRect(hWnd, &rt);
            pChildWnd->nSplitPos = x;
            ResizeWnd(pChildWnd, rt.right, rt.bottom);
            ReleaseCapture();
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
                ResizeWnd(pChildWnd, rt.right, rt.bottom);
                last_split = -1;
                ReleaseCapture();
                SetCursor(LoadCursor(0, IDC_ARROW));
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
        if (pChildWnd != NULL) {
            SetFocus(pChildWnd->nFocusPanel? pChildWnd->hListWnd: pChildWnd->hTreeWnd);
        }
        break;

    case WM_TIMER:
        break;

    case WM_NOTIFY:
        if ((int)wParam == TREE_WINDOW) {
            switch (((LPNMHDR)lParam)->code) {
            case TVN_ITEMEXPANDING:
                return !OnTreeExpanding(pChildWnd->hTreeWnd, (NMTREEVIEW*)lParam);
            case TVN_SELCHANGED:
                OnTreeSelectionChanged(pChildWnd->hTreeWnd, pChildWnd->hListWnd,
                    ((NMTREEVIEW *)lParam)->itemNew.hItem, TRUE);
                break;
	    case NM_SETFOCUS:
		pChildWnd->nFocusPanel = 0;
		break;
            case NM_RCLICK: {
		POINT pt;
                GetCursorPos(&pt);
		TrackPopupMenu(GetSubMenu(hPopupMenus, PM_NEW),
			       TPM_RIGHTBUTTON, pt.x, pt.y, 0, hFrameWnd, NULL);
		break;
            }
	    case TVN_ENDLABELEDIT: {
		HKEY hRootKey;
	        LPNMTVDISPINFO dispInfo = (LPNMTVDISPINFO)lParam;
		LPCTSTR path = GetItemPath(pChildWnd->hTreeWnd, 0, &hRootKey);
	        BOOL res = RenameKey(hWnd, hRootKey, path, dispInfo->item.pszText);
		if (res) {
		    TVITEMEX item;
		    item.mask = TVIF_HANDLE | TVIF_TEXT;
		    item.hItem = TreeView_GetSelection(pChildWnd->hTreeWnd);
		    item.pszText = dispInfo->item.pszText;
                    SendMessage( pChildWnd->hTreeWnd, TVM_SETITEMW, 0, (LPARAM)&item );
		}
		return res;
	    }
            default:
                return 0; /* goto def; */
            }
        } else
            if ((int)wParam == LIST_WINDOW) {
		if (((LPNMHDR)lParam)->code == NM_SETFOCUS) {
		    pChildWnd->nFocusPanel = 1;
		} else if (!SendMessage(pChildWnd->hListWnd, WM_NOTIFY_REFLECT, wParam, lParam)) {
                    goto def;
                }
            }
        break;

    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && pChildWnd != NULL) {
            ResizeWnd(pChildWnd, LOWORD(lParam), HIWORD(lParam));
        }
        /* fall through */
default: def:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
