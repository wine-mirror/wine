/*
 * WineCfg libraries tabsheet
 *
 * Copyright 2004 Robert van Herk
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
 *
 */

#define NONAMELESSUNION
#include <windows.h>
#include <commdlg.h>
#include <wine/debug.h>
#include <stdio.h>
#include "winecfg.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

typedef enum _DLGMODE
{
	DLL,
	APP,
	GLOBAL,
} DLGMODE;

typedef enum _DLLMODE {
	BUILTIN,
	NATIVE
} DLLMODE;

typedef struct _DLLOVERRIDE
{
	char* lpcKey;    /*The actual dll name*/
	DLLMODE mode;
} DLLOVERRIDE, *LPDLLOVERRIDE;

static LPDLLOVERRIDE CreateDLLOverride(char* lpcKey)
{
	LPDLLOVERRIDE out = HeapAlloc(GetProcessHeap(),0,sizeof(DLLOVERRIDE));
	out->lpcKey = strdup (lpcKey);
	return out;
}

static VOID FreeDLLOverride(LPDLLOVERRIDE ldo)
{
	if (ldo->lpcKey)
		free(ldo->lpcKey);
	HeapFree(GetProcessHeap(),0,ldo);
}

typedef struct _APPL
{
	BOOL isGlobal;
	char* lpcApplication;
	char* lpcSection; /*Registry section*/
} APPL, *LPAPPL;

static LPAPPL CreateAppl(BOOL isGlobal, char* application, char* section)
{
	LPAPPL out;
	out = HeapAlloc(GetProcessHeap(),0,sizeof(APPL));
	out->lpcApplication = strdup(application);
	out->lpcSection = strdup(section);
	out->isGlobal = isGlobal;
	return out;
}

static VOID FreeAppl(LPAPPL lpAppl)
{
	if (lpAppl->lpcApplication)
		free(lpAppl->lpcApplication); /* The strings were strdup-ped, so we use "free" */
	if (lpAppl->lpcSection)
		free(lpAppl->lpcSection);
	HeapFree(GetProcessHeap(),0,lpAppl);
}

typedef struct _ITEMTAG
{
	LPAPPL lpAppl;
	LPDLLOVERRIDE lpDo;
} ITEMTAG, *LPITEMTAG;

static LPITEMTAG CreateItemTag()
{
	LPITEMTAG out;
	out = HeapAlloc(GetProcessHeap(),0,sizeof(ITEMTAG));
	out->lpAppl = 0;
	out->lpDo = 0;
	return out;
}

static VOID FreeItemTag(LPITEMTAG lpit)
{
	if (lpit->lpAppl)
		FreeAppl(lpit->lpAppl);
	if (lpit->lpDo)
		FreeDLLOverride(lpit->lpDo);
	HeapFree(GetProcessHeap(),0,lpit);
}

static VOID UpdateDLLList(HWND hDlg, char* dll)
{
	/*Add if it isn't already in*/
	if (SendDlgItemMessage(hDlg, IDC_DLLLIST, CB_FINDSTRING, 1, (LPARAM) dll) == CB_ERR)
		SendDlgItemMessage(hDlg,IDC_DLLLIST,CB_ADDSTRING,0,(LPARAM) dll);
}

static VOID LoadLibrarySettings(LPAPPL appl /*DON'T FREE, treeview will own this*/, HWND hDlg, HWND hwndTV)
{
	HKEY key;
	int i;
	DWORD size;
	DWORD readSize;
	char name [255];
	char read [255];
	LPITEMTAG lpIt;
	TVINSERTSTRUCT tis;	
	HTREEITEM hParent;
	LPDLLOVERRIDE lpdo;
	
	WINE_TRACE("opening %s\n", appl->lpcSection);
	if (RegOpenKey (configKey, appl->lpcSection, &key) == ERROR_SUCCESS)
	{
		i = 0;
		size = 255;
		readSize = 255;
		
		lpIt = CreateItemTag();
		lpIt->lpAppl = appl;

		tis.hParent = NULL;
		tis.hInsertAfter = TVI_LAST;
		tis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
		tis.u.item.pszText = appl->lpcApplication;
		tis.u.item.lParam = (LPARAM)lpIt;
		hParent = TreeView_InsertItem(hwndTV,&tis);
		tis.hParent = hParent;

		while (RegEnumValue(key, i, name, &size, NULL, NULL, read, &readSize) == ERROR_SUCCESS)
		{                           
			WINE_TRACE("Reading value %s, namely %s\n", name, read);
			
			lpIt = CreateItemTag();
			lpdo = CreateDLLOverride(name);
			lpIt->lpDo = lpdo;
			tis.u.item.lParam = (LPARAM)lpIt;
			tis.u.item.pszText = name;
			if (strncmp (read, "built", 5) == 0)
				lpdo->mode = BUILTIN;
			else
				lpdo->mode = NATIVE;

			TreeView_InsertItem(hwndTV,&tis);
			UpdateDLLList(hDlg, name);
			i ++; size = 255; readSize = 255;
		}
		RegCloseKey(key);
	}
}

static VOID SetEnabledDLLControls(HWND dialog, DLGMODE dlgmode)
{
	if (dlgmode == DLL) {
		enable(IDC_RAD_BUILTIN);
		enable(IDC_RAD_NATIVE);
		enable(IDC_DLLS_REMOVEDLL);
	} else {
		disable(IDC_RAD_BUILTIN);
		disable(IDC_RAD_NATIVE);
		disable(IDC_DLLS_REMOVEDLL);
	}

	if (dlgmode == APP) {
		enable(IDC_DLLS_REMOVEAPP);
	}
	else {
		disable(IDC_DLLS_REMOVEAPP);
	}
}

static VOID OnInitLibrariesDlg(HWND hDlg)
{
	HWND hwndTV;
	LPAPPL lpAppl;
	HKEY applKey;
	int i;
	DWORD size;
	char appl [255];
	char lpcKey [255];
	FILETIME ft;

	hwndTV = GetDlgItem(hDlg,IDC_TREE_DLLS);
	lpAppl = CreateAppl(TRUE,"Global DLL Overrides", "DllOverrides");
	LoadLibrarySettings(lpAppl, hDlg, hwndTV);
	
	/*And now the application specific stuff:*/
	if (RegOpenKey(configKey, "AppDefaults", &applKey) == ERROR_SUCCESS) {
		i = 0;
		size = 255;
		while (RegEnumKeyEx(applKey, i, appl, &size, NULL, NULL, NULL, &ft) == ERROR_SUCCESS)
		{
			sprintf(lpcKey, "AppDefaults\\%s\\DllOverrides", appl);
			lpAppl = CreateAppl(FALSE,appl, lpcKey);
			LoadLibrarySettings(lpAppl, hDlg, hwndTV);
			i++; size = 255;
		}
		RegCloseKey(applKey);
	}

	SetEnabledDLLControls(hDlg, GLOBAL);
}

static VOID OnTreeViewChangeItem(HWND hDlg, HWND hTV)
{
	TVITEM ti;
	LPITEMTAG lpit;

	ti.mask = TVIF_PARAM;
	ti.hItem = TreeView_GetSelection(hTV);
	if (TreeView_GetItem (hTV, &ti))
	{
		lpit = (LPITEMTAG) ti.lParam;
		if (lpit->lpDo)
		{
			WINE_TRACE("%s\n", lpit->lpDo->lpcKey);
			if (lpit->lpDo->mode == BUILTIN) {
				CheckRadioButton(hDlg, IDC_RAD_BUILTIN, IDC_RAD_NATIVE, IDC_RAD_BUILTIN);
			} else {
				CheckRadioButton(hDlg, IDC_RAD_BUILTIN, IDC_RAD_NATIVE, IDC_RAD_NATIVE);
			}
			SetEnabledDLLControls(hDlg, DLL);
		} else {
			if (lpit->lpAppl)
			{
				if (lpit->lpAppl->isGlobal == TRUE)
					SetEnabledDLLControls(hDlg, GLOBAL);
				else
					SetEnabledDLLControls(hDlg, APP);
			}
		}
	}
}

static VOID SetDLLMode(HWND hDlg, DLLMODE mode)
{
	HWND hTV;
	TVITEM ti;
	LPITEMTAG lpit;
	char* cMode;
	TVITEM tiPar;
	LPITEMTAG lpitPar;

	hTV = GetDlgItem(hDlg, IDC_TREE_DLLS);
	ti.mask = TVIF_PARAM;
	ti.hItem = TreeView_GetSelection(hTV);
	if (TreeView_GetItem (hTV, &ti))
	{
		lpit = (LPITEMTAG) ti.lParam;
		if (lpit->lpDo)
		{
			lpit->lpDo->mode = mode;
			if (mode == NATIVE)
				cMode = "native, builtin";
			else
				cMode = "builtin, native";

			/*Find parent, so we can read registry section*/
			tiPar.mask = TVIF_PARAM;
			tiPar.hItem = TreeView_GetParent(hTV, ti.hItem);
			if (TreeView_GetItem(hTV,&tiPar))
			{
				lpitPar = (LPITEMTAG) tiPar.lParam;
				if (lpitPar->lpAppl)
				{
					addTransaction(lpitPar->lpAppl->lpcSection, lpit->lpDo->lpcKey, ACTION_SET, cMode);
				}
			}
		}
	}
}

static VOID OnBuiltinClick(HWND hDlg)
{
	SetDLLMode(hDlg, BUILTIN);
}

static VOID OnNativeClick(HWND hDlg)
{
	SetDLLMode(hDlg, NATIVE);
}

static VOID OnTreeViewDeleteItem(NMTREEVIEW* nmt)
{
	FreeItemTag((LPITEMTAG)(nmt->itemOld.lParam));
}

static VOID OnAddDLLClick(HWND hDlg)
{
	HWND hTV;
	TVITEM ti;
	LPITEMTAG lpit;
	LPITEMTAG lpitNew;
	TVITEM childti;
	char dll [255];
	BOOL doAdd;
	TVINSERTSTRUCT tis;

	hTV = GetDlgItem(hDlg, IDC_TREE_DLLS);
	ti.mask = TVIF_PARAM;
	ti.hItem = TreeView_GetSelection(hTV);
	if (TreeView_GetItem (hTV, &ti))
	{
		lpit = (LPITEMTAG) ti.lParam;
		if (lpit->lpDo) { /*Is this a DLL override (that is: a subitem), then find the parent*/
			ti.hItem = TreeView_GetParent(hTV, ti.hItem);
			if (TreeView_GetItem(hTV,&ti)) {
				lpit = (LPITEMTAG) ti.lParam;
			} else return;
		}
	} else return;
	/*Now we should have an parent item*/
	if (lpit->lpAppl)
	{
		lpitNew = CreateItemTag();
		SendDlgItemMessage(hDlg,IDC_DLLLIST,WM_GETTEXT,(WPARAM)255, (LPARAM) dll);
		if (strlen(dll) > 0) {
			/*Is the dll already in the list? If so, don't do it*/
			doAdd = TRUE;
			childti.mask = TVIF_PARAM;
			if ((childti.hItem = TreeView_GetNextItem(hTV, ti.hItem, TVGN_CHILD))) {
				/*Retrieved first child*/
				while (TreeView_GetItem (hTV, &childti))
				{
					if (strcmp(((LPITEMTAG)childti.lParam)->lpDo->lpcKey,dll) == 0) {
						doAdd = FALSE;
						break;
					}
					childti.hItem = TreeView_GetNextItem(hTV, childti.hItem, TVGN_NEXT);
				}
			}
			if (doAdd)
			{
				lpitNew->lpDo = CreateDLLOverride(dll);
				lpitNew->lpDo->mode = NATIVE;
				tis.hInsertAfter = TVI_LAST;
				tis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
				tis.u.item.pszText = dll;
				tis.u.item.lParam = (LPARAM)lpitNew;
				tis.hParent = ti.hItem;
				TreeView_InsertItem(hTV,&tis);
				UpdateDLLList(hDlg, dll);
				addTransaction(lpit->lpAppl->lpcSection, dll, ACTION_SET, "native, builtin");
			} else MessageBox(hDlg, "A DLL with that name is already in this list...", "", MB_OK | MB_ICONINFORMATION);
		}
	} else return;
}

static VOID OnRemoveDLLClick(HWND hDlg)
{
	HWND hTV;
	TVITEM ti;
	LPITEMTAG lpit;
	TVITEM tiPar;
	LPITEMTAG lpitPar;

	hTV = GetDlgItem(hDlg, IDC_TREE_DLLS);
	ti.mask = TVIF_PARAM;
	ti.hItem = TreeView_GetSelection(hTV);
	if (TreeView_GetItem (hTV, &ti)) {
		lpit = (LPITEMTAG) ti.lParam;
		if (lpit->lpDo)
		{
			/*Find parent for section*/
			tiPar.mask = TVIF_PARAM;
			tiPar.hItem = TreeView_GetParent(hTV, ti.hItem);
			if (TreeView_GetItem(hTV,&tiPar))
			{
				lpitPar = (LPITEMTAG) tiPar.lParam;
				if (lpitPar->lpAppl)
				{
					addTransaction(lpitPar->lpAppl->lpcSection, lpit->lpDo->lpcKey, ACTION_REMOVE, NULL);
					TreeView_DeleteItem(hTV,ti.hItem);
				}
			}
		}
	}
}

static VOID OnAddApplicationClick(HWND hDlg)
{
	char szFileTitle [255];
	char szFile [255];
	char lpcKey [255];

	TVINSERTSTRUCT tis;
	LPITEMTAG lpit;
	OPENFILENAME ofn = { sizeof(OPENFILENAME),
		0, /*hInst*/0, "Wine Programs (*.exe,*.exe.so)\0*.exe;*.exe.so\0", NULL, 0, 0, NULL,
		0, NULL, 0, NULL, NULL,
		OFN_SHOWHELP, 0, 0, NULL, 0, NULL };

	ofn.lpstrFileTitle = szFileTitle;
	ofn.lpstrFileTitle[0] = '\0';
	ofn.nMaxFileTitle = sizeof(szFileTitle);
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);

	if (GetOpenFileName(&ofn))
	{
		tis.hParent = NULL;
		tis.hInsertAfter = TVI_LAST;
		tis.u.item.mask = TVIF_TEXT | TVIF_PARAM;
		tis.u.item.pszText = szFileTitle;
		lpit = CreateItemTag();
		sprintf(lpcKey, "AppDefaults\\%s\\DllOverrides", szFileTitle);
		lpit->lpAppl = CreateAppl(FALSE,szFileTitle,lpcKey);
		tis.u.item.lParam = (LPARAM)lpit;
		TreeView_InsertItem(GetDlgItem(hDlg,IDC_TREE_DLLS), &tis);
		setConfigValue(lpcKey,NULL,NULL);
	}
}

static VOID OnRemoveApplicationClick(HWND hDlg)
{
	HWND hTV;
	TVITEM ti;
	LPITEMTAG lpit;

	hTV = GetDlgItem(hDlg, IDC_TREE_DLLS);
	ti.mask = TVIF_PARAM;
	ti.hItem = TreeView_GetSelection(hTV);
	if (TreeView_GetItem (hTV, &ti)) {
		lpit = (LPITEMTAG) ti.lParam;
		if (lpit->lpAppl)
		{
			addTransaction(lpit->lpAppl->lpcSection, NULL, ACTION_REMOVE, NULL);
			TreeView_DeleteItem(hTV,ti.hItem);
		}
	}
}

INT_PTR CALLBACK
LibrariesDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		OnInitLibrariesDlg(hDlg);
		break;  
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case TVN_SELCHANGED: {
				switch(LOWORD(wParam)) {
				case IDC_TREE_DLLS:
					OnTreeViewChangeItem(hDlg, GetDlgItem(hDlg,IDC_TREE_DLLS));
					break;
				}
			}
			break;
		case TVN_DELETEITEM:
			OnTreeViewDeleteItem ((LPNMTREEVIEW)lParam);
			break;
		}
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam)) {
		case BN_CLICKED:
			switch(LOWORD(wParam)) {
			case IDC_RAD_BUILTIN:
				OnBuiltinClick(hDlg);
				break;
			case IDC_RAD_NATIVE:
				OnNativeClick(hDlg);
				break;
			case IDC_DLLS_ADDAPP:
				OnAddApplicationClick(hDlg);
				break;
			case IDC_DLLS_REMOVEAPP:
				OnRemoveApplicationClick(hDlg);
				break;
			case IDC_DLLS_ADDDLL:
				OnAddDLLClick(hDlg);
				break;
			case IDC_DLLS_REMOVEDLL:
				OnRemoveDLLClick(hDlg);
				break;
			}
			break;
		}
		break;
	}

	return 0;
}
