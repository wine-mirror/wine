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

#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

#define STATUS_WINDOW   2001
#define TREE_WINDOW     2002
#define LIST_WINDOW     2003

#define	SPLIT_WIDTH	5

#define MAX_NEW_KEY_LEN  128
#define KEY_MAX_LEN      1024

#define REG_FORMAT_5     1
#define REG_FORMAT_4     2

/* Pop-Up Menus */
#define PM_COMPUTER      0
#define PM_TREEVIEW      1
#define PM_NEW_VALUE     2
#define PM_MODIFY_VALUE  3

/* HexEdit Class */
#define HEM_SETDATA (WM_USER+0)
#define HEM_GETDATA (WM_USER+1)

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

typedef struct tagLINE_INFO
{
    WCHAR  *name;
    DWORD   dwValType;
    void   *val;
    size_t  val_len;
} LINE_INFO;

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

extern const WCHAR szChildClass[];
extern WCHAR g_pszDefaultValueName[];

extern DWORD g_columnToSort;
extern BOOL g_invertSort;
extern WCHAR *g_currentPath;
extern HKEY g_currentRootKey;

/* Registry class names and their indexes */
extern const WCHAR* reg_class_namesW[];
#define INDEX_HKEY_LOCAL_MACHINE    0
#define INDEX_HKEY_USERS            1
#define INDEX_HKEY_CLASSES_ROOT     2
#define INDEX_HKEY_CURRENT_CONFIG   3
#define INDEX_HKEY_CURRENT_USER     4
#define INDEX_HKEY_DYN_DATA         5

/* about.c */
void ShowAboutBox(HWND hWnd);

/* childwnd.c */
LPWSTR GetItemFullPath(HWND hwndTV, HTREEITEM hItem, BOOL bFull);
LRESULT CALLBACK ChildWndProc(HWND, UINT, WPARAM, LPARAM);

/* edit.c */
BOOL CreateKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPWSTR newKeyName);
BOOL CreateValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, DWORD valueType, LPWSTR valueName);
BOOL ModifyValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName);
BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath);
BOOL DeleteValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName);
BOOL RenameValue(HWND hwnd, HKEY hRootKey, LPCWSTR keyPath, LPCWSTR oldName, LPCWSTR newName);
BOOL RenameKey(HWND hwnd, HKEY hRootKey, LPCWSTR keyPath, LPCWSTR newName);
int WINAPIV messagebox(HWND hwnd, int buttons, int titleId, int resId, ...);

/* framewnd.c */
LRESULT CALLBACK FrameWndProc(HWND, UINT, WPARAM, LPARAM);
void SetupStatusBar(HWND hWnd, BOOL bResize);
void UpdateStatusBar(void);

/* hexedit.c */
void HexEdit_Register(void);

/* listview.c */
BOOL update_listview_path(const WCHAR *path);
void format_value_data(HWND hwndLV, int index, DWORD type, void *data, DWORD size);
int AddEntryToList(HWND hwndLV, WCHAR *Name, DWORD dwValType, void *ValBuf, DWORD dwCount, int pos);
void OnGetDispInfo(NMLVDISPINFOW *plvdi);
HWND CreateListView(HWND hwndParent, UINT id);
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
BOOL RefreshListView(HWND hwndLV, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR highlightValue);
HWND StartValueRename(HWND hwndLV);
LPWSTR GetItemText(HWND hwndLV, UINT item);
WCHAR *GetValueName(HWND hwndLV);
BOOL ListWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result);
BOOL IsDefaultValue(HWND hwndLV, int i);

/* regedit.c */
void WINAPIV output_message(unsigned int id, ...);
void WINAPIV error_exit(void);

/* regproc.c */
char *GetMultiByteString(const WCHAR *strW);
BOOL import_registry_file(FILE *reg_file);
void delete_registry_key(WCHAR *reg_key_name);
BOOL export_registry_key(WCHAR *file_name, WCHAR *path, DWORD format);

/* treeview.c */
HWND CreateTreeView(HWND hwndParent, LPWSTR pHostName, UINT id);
BOOL RefreshTreeView(HWND hWndTV);
BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEWW* pnmtv);
LPWSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey);
BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem);
HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPWSTR name);
HWND StartKeyRename(HWND hwndTV);
HTREEITEM FindPathInTree(HWND hwndTV, LPCWSTR lpKeyName);
HTREEITEM FindNext(HWND hwndTV, HTREEITEM hItem, LPCWSTR sstring, int mode, int *row);

#endif /* __MAIN_H__ */
