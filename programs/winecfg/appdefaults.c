/*
 * Application defaults page
 *
 * Copyright 2003 Mike Hearn
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

#include <stdio.h>

#include "winecfg.h"
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/debug.h>

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

int appSettings = EDITING_GLOBAL; /* start by editing global */
char *currentApp; /* the app we are currently editing, or NULL if editing global */

char *getSectionForApp(char *section) {
    static char *lastResult = NULL;
    if (lastResult) HeapFree(GetProcessHeap(), 0, lastResult);
    lastResult = HeapAlloc(GetProcessHeap(), 0, strlen("AppDefaults\\") + strlen(currentApp) + 2 /* \\ */ + strlen(section) + 1 /* terminator */);
    sprintf(lastResult, "AppDefaults\\%s\\%s", currentApp, section);
    return lastResult;
}

static void configureFor(HWND dialog, int mode) {
    CheckRadioButton(dialog, IDC_EDITING_GLOBAL, IDC_EDITING_APP, mode == EDITING_APP ? IDC_EDITING_APP : IDC_EDITING_GLOBAL);
    if (mode == EDITING_GLOBAL) {
	disable(IDC_LIST_APPS);
	disable(IDC_ADD_APPDEFAULT);
	disable(IDC_REMOVE_APPDEFAULT);
	if (currentApp) HeapFree(GetProcessHeap(), 0, currentApp);
    } else {
	enable(IDC_LIST_APPS);
	enable(IDC_ADD_APPDEFAULT);
	enable(IDC_REMOVE_APPDEFAULT);
    }
    appSettings = mode;
}

/* fill the dialog with the current appdefault entries */
static void refreshDialog(HWND dialog) {
    HKEY key;
    char *subKeyName = HeapAlloc(GetProcessHeap(), 0, MAX_NAME_LENGTH);
    DWORD sizeOfSubKeyName = MAX_NAME_LENGTH;
    int i, itemIndex;
    
    WINE_TRACE("\n");
    
    /* Clear the listbox */
    SendMessageA(GetDlgItem(dialog, IDC_LIST_APPS), LB_RESETCONTENT, 0, 0);

    return_if_fail(
	RegCreateKey(HKEY_LOCAL_MACHINE, WINE_KEY_ROOT "\\AppDefaults", &key) == ERROR_SUCCESS
    );
    
    /* Iterate over each subkey in the AppDefaults tree */
    for (i = 0;
	 RegEnumKeyEx(key, i, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS;
	 ++i, sizeOfSubKeyName = MAX_NAME_LENGTH) {

	WINE_TRACE("appdefault entry=%s\n", subKeyName);
	itemIndex = SendMessageA(GetDlgItem(dialog, IDC_LIST_APPS), LB_ADDSTRING ,(WPARAM) -1, (LPARAM) subKeyName);
    }

    configureFor(dialog, appSettings);

    WINE_TRACE("done\n");
    RegCloseKey(key);
    HeapFree(GetProcessHeap(), 0, subKeyName);
}

static void onAppsListSelChange(HWND dialog) {
    int newPos = SendDlgItemMessage(dialog, IDC_LIST_APPS, LB_GETCURSEL, 0, 0);
    int appLen = SendDlgItemMessage(dialog, IDC_LIST_APPS, LB_GETTEXTLEN, newPos, 0);
    if (currentApp) HeapFree(GetProcessHeap(), 0, currentApp);
    currentApp = HeapAlloc(GetProcessHeap(), 0, appLen+1);
    return_if_fail(
	SendDlgItemMessage(dialog, IDC_LIST_APPS, LB_GETTEXT, newPos, (LPARAM) currentApp) != LB_ERR
    );
    WINE_TRACE("new selection is %s\n", currentApp);
}

INT_PTR CALLBACK
AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_COMMAND: switch (LOWORD(wParam)) {
	    case IDC_EDITING_APP:
		configureFor(hDlg, EDITING_APP);
		break;
	    case IDC_EDITING_GLOBAL:
		configureFor(hDlg, EDITING_GLOBAL);
		break;
	    case IDC_ADD_APPDEFAULT:
		WRITEME(hDlg);
		break;
	    case IDC_REMOVE_APPDEFAULT:
		WRITEME(hDlg);
		break;
	    case IDC_LIST_APPS:
		if (HIWORD(wParam) == LBN_SELCHANGE) onAppsListSelChange(hDlg);
		break;
	}
	break;

	case WM_NOTIFY: switch(((LPNMHDR)lParam)->code) {
	    case PSN_KILLACTIVE:
		SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
		break;
	    case PSN_APPLY:
		SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
		break;
	    case PSN_SETACTIVE:
		refreshDialog(hDlg);
		break;
		    
	};
	break;

	case WM_INITDIALOG:
	    WINE_TRACE("Init appdefaults\n");
	    break;
	    
    }
    return FALSE;
}
