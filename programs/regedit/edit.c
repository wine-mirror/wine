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

static LPTSTR read_value(HWND hwnd, HKEY hKey, LPCTSTR valueName, DWORD *lpType, LONG *len)
{
    DWORD valueDataLen;
    LPTSTR buffer = NULL;
    LONG lRet;

    lRet = RegQueryValueEx(hKey, valueName, 0, lpType, 0, &valueDataLen);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }
    if ( *lpType == REG_DWORD ) valueDataLen = sizeof(DWORD);
    if (!(buffer = HeapAlloc(GetProcessHeap(), 0, valueDataLen))) {
        error(hwnd, IDS_TOO_BIG_VALUE, valueDataLen);
        goto done;
    }
    lRet = RegQueryValueEx(hKey, valueName, 0, 0, buffer, &valueDataLen);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }

    if(len) *len = valueDataLen;
    return buffer;

done:
    HeapFree(GetProcessHeap(), 0, buffer);
    return NULL;
}

BOOL CreateKey(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath)
{
    BOOL result = FALSE;
    LONG lRet = ERROR_SUCCESS;
    HKEY retKey;
    TCHAR keyName[32];
    TCHAR newKey[COUNT_OF(keyName) - 4];
    int keyNum;
    HKEY hKey;
         
    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_CREATE_SUB_KEY, &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;

    if (!LoadString(GetModuleHandle(0), IDS_NEWKEY, newKey, COUNT_OF(newKey))) goto done;

    /* try to find out a name for the newly create key (max 100 times) */
    for (keyNum = 1; keyNum < 100; keyNum++) {
	wsprintf(keyName, newKey, keyNum);
	lRet = RegOpenKey(hKey, keyName, &retKey);
	if (lRet != ERROR_SUCCESS) break;
	RegCloseKey(retKey);
    }
    if (lRet == ERROR_SUCCESS) goto done;
    
    lRet = RegCreateKey(hKey, keyName, &retKey);
    if (lRet != ERROR_SUCCESS) goto done;
    result = TRUE;

done:
    RegCloseKey(retKey);
    return result;
}

BOOL ModifyValue(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath, LPCTSTR valueName)
{
    BOOL result = FALSE;
    DWORD type;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;

    editValueName = valueName;
    if (!lstrcmp(valueName, _T("(Default)")))
        valueName = NULL;
    if(!(stringValueData = read_value(hwnd, hKey, valueName, &type, 0))) goto done;

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
    RegCloseKey(hKey);
    return result;
}

BOOL DeleteValue(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath, LPCTSTR valueName)
{
    BOOL result = FALSE;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;

    if (messagebox(hwnd, MB_YESNO | MB_ICONEXCLAMATION, IDS_DELETE_BOX_TITLE, IDS_DELETE_BOX_TEXT, valueName) != IDYES)
	goto done;

    lRet = RegDeleteValue(hKey, valueName);
    if (lRet != ERROR_SUCCESS) {
        error(hwnd, IDS_BAD_VALUE, valueName);
    }
    if (lRet != ERROR_SUCCESS) goto done;
    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}

BOOL CreateValue(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath, DWORD valueType)
{
    LONG lRet = ERROR_SUCCESS;
    TCHAR valueName[32];
    TCHAR newValue[COUNT_OF(valueName) - 4];
    DWORD valueDword = 0;
    BOOL result = FALSE;
    int valueNum;
    HKEY hKey;
         
    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;

    if (!LoadString(GetModuleHandle(0), IDS_NEWVALUE, newValue, COUNT_OF(newValue))) goto done;

    /* try to find out a name for the newly create key (max 100 times) */
    for (valueNum = 1; valueNum < 100; valueNum++) {
	wsprintf(valueName, newValue, valueNum);
	lRet = RegQueryValueEx(hKey, valueName, 0, 0, 0, 0);
	if (lRet != ERROR_SUCCESS) break;
    }
    if (lRet == ERROR_SUCCESS) goto done;
   
    lRet = RegSetValueEx(hKey, valueName, 0, valueType, (BYTE*)&valueDword, sizeof(DWORD));
    if (lRet != ERROR_SUCCESS) goto done;
    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}

BOOL RenameValue(HWND hwnd, HKEY hKeyRoot, LPCTSTR keyPath, LPCTSTR oldName, LPCTSTR newName)
{
    LPTSTR value = NULL;
    DWORD type;
    LONG len, lRet;
    BOOL result = FALSE;
    HKEY hKey;

    if (!newName) return FALSE;

    lRet = RegOpenKeyEx(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet != ERROR_SUCCESS) return FALSE;
    value = read_value(hwnd, hKey, oldName, &type, &len);
    if(!value) goto done;
    lRet = RegSetValueEx(hKey, newName, 0, type, (BYTE*)value, len);
    if (lRet != ERROR_SUCCESS) goto done;
    lRet = RegDeleteValue(hKey, oldName);
    if (lRet != ERROR_SUCCESS) {
	RegDeleteValue(hKey, newName);
	goto done;
    }
    result = TRUE;

done:
    HeapFree(GetProcessHeap(), 0, value);
    RegCloseKey(hKey);
    return result;
}
