/*
 * Registry editing UI functions.
 *
 * Copyright (C) 2003 Dimitrie O. Paun
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
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cderr.h>
#include <stdlib.h>
#include <stdio.h>
#include <shellapi.h>

#include "main.h"
#include "regproc.h"
#include "resource.h"

static const TCHAR* editValueName;
static TCHAR* stringValueData;
static BOOL isDecimal;

INT vmessagebox(HWND hwnd, INT buttons, INT titleId, INT resId, va_list ap)
{
    TCHAR title[256];
    TCHAR errfmt[1024];
    TCHAR errstr[1024];

    if (!LoadString(hInst, titleId, title, COUNT_OF(title)))
        lstrcpy(title, "Error");

    if (!LoadString(hInst, resId, errfmt, COUNT_OF(errfmt)))
        lstrcpy(errfmt, "Unknown error string!");

    _vsntprintf(errstr, COUNT_OF(errstr), errfmt, ap);

    return MessageBox(hwnd, errstr, title, buttons);
}

INT messagebox(HWND hwnd, INT buttons, INT titleId, INT resId, ...)
{
    va_list ap;
    INT result;

    va_start(ap, resId);
    result = vmessagebox(hwnd, buttons, titleId, resId, ap);
    va_end(ap);

    return result;
}

void error(HWND hwnd, INT resId, ...)
{
    va_list ap;

    va_start(ap, resId);
    vmessagebox(hwnd, MB_OK | MB_ICONERROR, IDS_ERROR, resId, ap);
    va_end(ap);
}

BOOL change_dword_base(HWND hwndDlg, BOOL toHex)
{
    TCHAR buf[128];
    DWORD val;

    if (!GetDlgItemText(hwndDlg, IDC_VALUE_DATA, buf, COUNT_OF(buf))) return FALSE;
    if (!_stscanf(buf, toHex ? "%ld" : "%lx", &val)) return FALSE;
    wsprintf(buf, toHex ? "%lx" : "%ld", val);
    return SetDlgItemText(hwndDlg, IDC_VALUE_DATA, buf);    
}

INT_PTR CALLBACK modify_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR* valueData;
    HWND hwndValue;
    int len;

    switch(uMsg) {
    case WM_INITDIALOG:
        SetDlgItemText(hwndDlg, IDC_VALUE_NAME, editValueName);
        SetDlgItemText(hwndDlg, IDC_VALUE_DATA, stringValueData);
	CheckRadioButton(hwndDlg, IDC_DWORD_HEX, IDC_DWORD_DEC, isDecimal ? IDC_DWORD_DEC : IDC_DWORD_HEX);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_DWORD_HEX:
	    if (isDecimal && change_dword_base(hwndDlg, TRUE)) isDecimal = FALSE;
	break;
        case IDC_DWORD_DEC:
	    if (!isDecimal && change_dword_base(hwndDlg, FALSE)) isDecimal = TRUE;
	break;
        case IDOK:
            if ((hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA))) {
                if ((len = GetWindowTextLength(hwndValue))) {
                    if ((valueData = HeapReAlloc(GetProcessHeap(), 0, stringValueData, (len + 1) * sizeof(TCHAR)))) {
                        stringValueData = valueData;
                        if (!GetWindowText(hwndValue, stringValueData, len + 1))
                            *stringValueData = 0;
                    }
                }
            }
            /* Fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CreateKey(HKEY hKey)
{
    LONG lRet;
    HKEY retKey;
    TCHAR keyName[32];
    static TCHAR newKey[28] = ""; /* should be max keyName len - 4 */
    HINSTANCE hInstance;
    unsigned int keyNum = 1;
         
    /* If we have illegal parameter return with operation failure */
    if (!hKey) return FALSE;

    /* Load localized "new key" string. -4 is because we need max 4 character
	to numbering. */
    if (newKey[0] == 0) {
	hInstance = GetModuleHandle(0);
	if (!LoadString(hInstance, IDS_NEWKEY, newKey, COUNT_OF(newKey)))
    	    lstrcpy(newKey, "New Key");
    }
    lstrcpy(keyName, newKey);

    /* try to find out a name for the newly create key.
	We try it max 100 times. */
    lRet = RegOpenKey(hKey, keyName, &retKey);
    while (lRet == ERROR_SUCCESS && keyNum < 100) {
	wsprintf(keyName, "%s %u", newKey, ++keyNum);
	lRet = RegOpenKey(hKey, keyName, &retKey);
    }
    if (lRet == ERROR_SUCCESS) return FALSE;
    
    lRet = RegCreateKey(hKey, keyName, &retKey);
    return lRet == ERROR_SUCCESS;
}

BOOL ModifyValue(HWND hwnd, HKEY hKey, LPCTSTR valueName)
{
    DWORD valueDataLen;
    DWORD type;
    LONG lRet;
    BOOL result = FALSE;

    if (!hKey || !valueName) return FALSE;

    editValueName = valueName;

    lRet = RegQueryValueEx(hKey, valueName, 0, &type, 0, &valueDataLen);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }
    if ( type == REG_DWORD ) valueDataLen = 128;
    if (!(stringValueData = HeapAlloc(GetProcessHeap(), 0, valueDataLen))) {
        error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
        goto done;
    }
    lRet = RegQueryValueEx(hKey, valueName, 0, 0, stringValueData, &valueDataLen);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }

    if ( (type == REG_SZ) || (type == REG_EXPAND_SZ) ) {
        if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_STRING), hwnd, modify_dlgproc) == IDOK) {
            lRet = RegSetValueEx(hKey, valueName, 0, type, stringValueData, lstrlen(stringValueData) + 1);
            if (lRet == ERROR_SUCCESS) result = TRUE;
        }
    } else if ( type == REG_DWORD ) {
	wsprintf(stringValueData, isDecimal ? "%ld" : "%lx", *((DWORD*)stringValueData));
	if (DialogBox(0, MAKEINTRESOURCE(IDD_EDIT_DWORD), hwnd, modify_dlgproc) == IDOK) {
	     DWORD val;
	     if (_stscanf(stringValueData, isDecimal ? "%ld" : "%lx", &val)) {
		lRet = RegSetValueEx(hKey, valueName, 0, type, (BYTE*)&val, sizeof(val));
	        if (lRet == ERROR_SUCCESS) result = TRUE;
	     }
	}
    } else {
        error(hwnd, IDS_UNSUPPORTED_TYPE, type);
    }

done:
    HeapFree(GetProcessHeap(), 0, stringValueData);
    stringValueData = NULL;

    return result;
}

BOOL DeleteValue(HWND hwnd, HKEY hKey, LPCTSTR valueName)
{
    LONG lRet;

    if (!hKey || !valueName) return FALSE;

    if (messagebox(hwnd, MB_YESNO | MB_ICONEXCLAMATION, IDS_DELETE_BOX_TITLE, IDS_DELETE_BOX_TEXT, valueName) != IDYES)
	return FALSE;

    lRet = RegDeleteValue(hKey, valueName);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
    }
    return lRet == ERROR_SUCCESS;
}
