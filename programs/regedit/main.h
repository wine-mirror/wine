/*
 * Regedit definitions
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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "resource.h"


#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define	SPLIT_WIDTH	5

#define COUNT_OF(a) (sizeof(a)/sizeof(a[0]))

#define PM_MODIFYVALUE  0
#define PM_NEW          1

#define MAX_NEW_KEY_LEN 128

#define WM_NOTIFY_REFLECT (WM_USER+1024)

/* HexEdit Class */
#define HEM_SETDATA (WM_USER+0)
#define HEM_GETDATA (WM_USER+1)

extern HINSTANCE hInst;

/******************************************************************************/

enum OPTION_FLAGS {
    OPTIONS_AUTO_REFRESH            	   = 0x01,
    OPTIONS_READ_ONLY_MODE          	   = 0x02,
    OPTIONS_CONFIRM_ON_DELETE       	   = 0x04,
    OPTIONS_SAVE_ON_EXIT         	   = 0x08,
    OPTIONS_DISPLAY_BINARY_DATA    	   = 0x10,
    OPTIONS_VIEW_TREE_ONLY       	   = 0x20,
    OPTIONS_VIEW_DATA_ONLY      	   = 0x40,
};

enum SEARCH_FLAGS {
    SEARCH_WHOLE                           = 0x01,
    SEARCH_KEYS                            = 0x02,
    SEARCH_VALUES                          = 0x04,
    SEARCH_CONTENT                         = 0x08,
};

typedef struct {
    HWND    hWnd;
    HWND    hTreeWnd;
    HWND    hListWnd;
    int     nFocusPanel;      /* 0: left  1: right */
    int	    nSplitPos;
    WINDOWPLACEMENT pos;
    WCHAR   szPath[MAX_PATH];
} ChildWnd;
extern ChildWnd* g_pChildWnd;

/*******************************************************************************
 * Global Variables:
 */
extern HINSTANCE hInst;
extern HWND      hFrameWnd;
extern HMENU     hMenuFrame;
extern HWND      hStatusBar;
extern HMENU     hPopupMenus;
extern HFONT     hFont;
extern enum OPTION_FLAGS Options;

extern WCHAR szTitle[];
extern const WCHAR szFrameClass[];
extern const WCHAR szChildClass[];
extern const WCHAR szHexEditClass[];
extern WCHAR g_pszDefaultValueName[];

/* Registry class names and their indexes */
extern const WCHAR* reg_class_namesW[];
#define INDEX_HKEY_LOCAL_MACHINE    0
#define INDEX_HKEY_USERS            1
#define INDEX_HKEY_CLASSES_ROOT     2
#define INDEX_HKEY_CURRENT_CONFIG   3
#define INDEX_HKEY_CURRENT_USER     4
#define INDEX_HKEY_DYN_DATA         5



/* about.c */
extern void ShowAboutBox(HWND hWnd);

/* childwnd.c */
extern LPWSTR GetItemFullPath(HWND hwndTV, HTREEITEM hItem, BOOL bFull);
extern LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);

/* framewnd.c */
extern LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
extern void SetupStatusBar(HWND hWnd, BOOL bResize);
extern void UpdateStatusBar(void);

/* listview.c */
extern HWND CreateListView(HWND hwndParent, UINT id);
extern BOOL RefreshListView(HWND hwndLV, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR highlightValue);
extern HWND StartValueRename(HWND hwndLV);
extern LPWSTR GetItemText(HWND hwndLV, UINT item);
extern LPCWSTR GetValueName(HWND hwndLV);
extern BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
extern BOOL IsDefaultValue(HWND hwndLV, int i);

/* treeview.c */
extern HWND CreateTreeView(HWND hwndParent, LPWSTR pHostName, UINT id);
extern BOOL RefreshTreeView(HWND hWndTV);
extern BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEWW* pnmtv);
extern LPWSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey);
extern BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem);
extern HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPWSTR name);
extern HWND StartKeyRename(HWND hwndTV);
extern HTREEITEM FindPathInTree(HWND hwndTV, LPCWSTR lpKeyName);
extern HTREEITEM FindNext(HWND hwndTV, HTREEITEM hItem, LPCWSTR sstring, int mode, int *row);

/* edit.c */
extern BOOL CreateKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPWSTR newKeyName);
extern BOOL CreateValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, DWORD valueType, LPWSTR valueName);
extern BOOL ModifyValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName);
extern BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath);
extern BOOL DeleteValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName, BOOL showMessageBox);
extern BOOL RenameValue(HWND hwnd, HKEY hRootKey, LPCWSTR keyPath, LPCWSTR oldName, LPCWSTR newName);
extern BOOL RenameKey(HWND hwnd, HKEY hRootKey, LPCWSTR keyPath, LPCWSTR newName);
extern int __cdecl messagebox(HWND hwnd, int buttons, int titleId, int resId, ...);

/* hexedit.c */
extern void HexEdit_Register(void);

#endif /* __MAIN_H__ */
