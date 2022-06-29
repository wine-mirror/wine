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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cderr.h>
#include <stdlib.h>
#include <shellapi.h>
#include <shlwapi.h>

#include "wine/heap.h"
#include "main.h"

static const WCHAR* editValueName;
static WCHAR* stringValueData;
static BOOL isDecimal;

struct edit_params
{
    HKEY         hkey;
    const WCHAR *value_name;
    DWORD        type;
    void        *data;
    DWORD        size;
};

static int vmessagebox(HWND hwnd, int buttons, int titleId, int resId, va_list va_args)
{
    WCHAR title[256];
    WCHAR fmt[1024];
    WCHAR *str;
    int ret;

    LoadStringW(hInst, titleId, title, ARRAY_SIZE(title));
    LoadStringW(hInst, resId, fmt, ARRAY_SIZE(fmt));

    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                   fmt, 0, 0, (WCHAR *)&str, 0, &va_args);
    ret = MessageBoxW(hwnd, str, title, buttons);
    LocalFree(str);

    return ret;
}

int WINAPIV messagebox(HWND hwnd, int buttons, int titleId, int resId, ...)
{
    va_list ap;
    INT result;

    va_start(ap, resId);
    result = vmessagebox(hwnd, buttons, titleId, resId, ap);
    va_end(ap);

    return result;
}

static void WINAPIV error_code_messagebox(HWND hwnd, unsigned int msg_id, ...)
{
    va_list ap;

    va_start(ap, msg_id);
    vmessagebox(hwnd, MB_OK|MB_ICONERROR, IDS_ERROR, msg_id, ap);
    va_end(ap);
}

static BOOL change_dword_base(HWND hwndDlg, BOOL toHex)
{
    WCHAR buf[128];
    UINT64 val;

    if (!GetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, buf, ARRAY_SIZE(buf))) return FALSE;
    if (!swscanf(buf, toHex ? L"%I64u" : L"%I64x", &val)) return FALSE;
    wsprintfW(buf, toHex ? L"%I64x" : L"%I64u", val);
    return SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, buf);
}

static INT_PTR CALLBACK modify_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndValue;
    WCHAR buff[64];
    int len;

    switch(uMsg) {
    case WM_INITDIALOG:
        SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, editValueName);
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, stringValueData);
        CheckRadioButton(hwndDlg, IDC_DWORD_HEX, IDC_DWORD_DEC, IDC_DWORD_HEX);
        isDecimal = FALSE;
        if (lParam == REG_QWORD && LoadStringW(GetModuleHandleW(0), IDS_EDIT_QWORD, buff, ARRAY_SIZE(buff)))
            SetWindowTextW(hwndDlg, buff);
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
                len = GetWindowTextLengthW(hwndValue);
                stringValueData = heap_xrealloc(stringValueData, (len + 1) * sizeof(WCHAR));
                if (!GetWindowTextW(hwndValue, stringValueData, len + 1))
                    *stringValueData = 0;
                }
            /* Fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK bin_modify_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct edit_params *params;
    BYTE *data;
    LONG size;
    LONG lRet;

    switch(uMsg) {
    case WM_INITDIALOG:
        params = (struct edit_params *)lParam;
        SetWindowLongPtrW(hwndDlg, DWLP_USER, (ULONG_PTR)params);
        if (params->value_name)
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, params->value_name);
        else
            SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, g_pszDefaultValueName);
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, HEM_SETDATA, (WPARAM)params->size, (LPARAM)params->data);
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, WM_SETFONT, (WPARAM) GetStockObject(ANSI_FIXED_FONT), TRUE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            params = (struct edit_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            size = SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, HEM_GETDATA, 0, 0);
            data = heap_xalloc(size);

            SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, HEM_GETDATA, (WPARAM)size, (LPARAM)data);
            lRet = RegSetValueExW(params->hkey, params->value_name, 0, params->type, data, size);
            heap_free(data);

            if (lRet == ERROR_SUCCESS)
                EndDialog(hwndDlg, 1);
            else
            {
                error_code_messagebox(hwndDlg, IDS_SET_VALUE_FAILED);
                EndDialog(hwndDlg, 0);
            }
            return TRUE;
        case IDCANCEL:
            EndDialog(hwndDlg, 0);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL value_exists(HWND hwnd, HKEY hKey, const WCHAR *value_name)
{
    return !RegQueryValueExW(hKey, value_name, NULL, NULL, NULL, NULL);
}

static LPWSTR read_value(HWND hwnd, HKEY hKey, LPCWSTR valueName, DWORD *lpType, LONG *len)
{
    DWORD valueDataLen;
    LPWSTR buffer = NULL;
    LONG lRet;

    lRet = RegQueryValueExW(hKey, valueName, NULL, lpType, NULL, &valueDataLen);
    if (lRet) {
        if (lRet == ERROR_FILE_NOT_FOUND && !valueName) { /* no default value here, make it up */
            if (len) *len = 1;
            if (lpType) *lpType = REG_SZ;
            buffer = heap_xalloc(sizeof(WCHAR));
            *buffer = '\0';
            return buffer;
        }
        error_code_messagebox(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }
    if ( *lpType == REG_DWORD ) valueDataLen = sizeof(DWORD);
    buffer = heap_xalloc(valueDataLen + sizeof(WCHAR));
    lRet = RegQueryValueExW(hKey, valueName, 0, 0, (LPBYTE)buffer, &valueDataLen);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_BAD_VALUE, valueName);
        goto done;
    }
    if((valueDataLen % sizeof(WCHAR)) == 0)
        buffer[valueDataLen / sizeof(WCHAR)] = 0;
    if(len) *len = valueDataLen;
    return buffer;

done:
    heap_free(buffer);
    return NULL;
}

BOOL CreateKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPWSTR keyName)
{
    BOOL result = FALSE;
    LONG lRet = ERROR_SUCCESS;
    HKEY retKey = NULL;
    WCHAR newKey[MAX_NEW_KEY_LEN - 4];
    int keyNum;
    HKEY hKey;
         
    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_CREATE_SUB_KEY, &hKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_CREATE_KEY_FAILED);
	goto done;
    }

    if (!LoadStringW(GetModuleHandleW(0), IDS_NEWKEY, newKey, ARRAY_SIZE(newKey))) goto done;

    /* try to find a name for the key being created (maximum = 100 attempts) */
    for (keyNum = 1; keyNum < 100; keyNum++) {
	wsprintfW(keyName, newKey, keyNum);
	lRet = RegOpenKeyW(hKey, keyName, &retKey);
	if (lRet) break;
	RegCloseKey(retKey);
    }
    if (lRet == ERROR_SUCCESS) goto done;
    
    lRet = RegCreateKeyW(hKey, keyName, &retKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_CREATE_KEY_FAILED);
	goto done;
    }

    result = TRUE;

done:
    RegCloseKey(retKey);
    return result;
}

BOOL ModifyValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName)
{
    BOOL result = FALSE;
    DWORD type;
    LONG lRet;
    HKEY hKey;
    LONG len;

    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
	return FALSE;
    }

    editValueName = valueName ? valueName : g_pszDefaultValueName;
    if(!(stringValueData = read_value(hwnd, hKey, valueName, &type, &len))) goto done;

    if ( (type == REG_SZ) || (type == REG_EXPAND_SZ) ) {
        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_STRING), hwnd, modify_dlgproc) == IDOK) {
            lRet = RegSetValueExW(hKey, valueName, 0, type, (LPBYTE)stringValueData, (lstrlenW(stringValueData) + 1) * sizeof(WCHAR));
            if (lRet == ERROR_SUCCESS) result = TRUE;
            else error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
        }
    } else if ( type == REG_DWORD ) {
        DWORD value = *((DWORD*)stringValueData);
        stringValueData = heap_xrealloc(stringValueData, 64);
        wsprintfW(stringValueData, L"%x", value);
        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_DWORD), hwnd, modify_dlgproc) == IDOK)
        {
            DWORD val;
            if (swscanf(stringValueData, isDecimal ? L"%lu" : L"%lx", &val))
            {
                lRet = RegSetValueExW(hKey, valueName, 0, type, (BYTE*)&val, sizeof(val));
                if (lRet == ERROR_SUCCESS) result = TRUE;
                else error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
            }
        }
    } else if ( type == REG_QWORD ) {
        UINT64 value = *((UINT64 *)stringValueData);
        stringValueData = heap_xrealloc(stringValueData, 64);
        swprintf(stringValueData, 64, L"%I64x", value);
        if (DialogBoxParamW(0, MAKEINTRESOURCEW(IDD_EDIT_DWORD), hwnd, modify_dlgproc, type) == IDOK)
        {
            if (swscanf(stringValueData, isDecimal ? L"%I64u" : L"%I64x", &value))
            {
                lRet = RegSetValueExW(hKey, valueName, 0, type, (BYTE *)&value, sizeof(value));
                if (lRet == ERROR_SUCCESS) result = TRUE;
                else error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
            }
        }
    } else if ( type == REG_MULTI_SZ ) {
        WCHAR char1 = '\r', char2 = '\n';
        WCHAR *tmpValueData = NULL;
        INT i, j, count;

        for ( i = 0, count = 0; i < len - 1; i++)
            if ( !stringValueData[i] && stringValueData[i + 1] )
                count++;
        tmpValueData = heap_xalloc((len + count) * sizeof(WCHAR));

        for ( i = 0, j = 0; i < len - 1; i++)
        {
            if ( !stringValueData[i] && stringValueData[i + 1])
            {
                tmpValueData[j++] = char1;
                tmpValueData[j++] = char2;
            }
            else
                tmpValueData[j++] = stringValueData[i];
        }
        tmpValueData[j] = stringValueData[i];
        heap_free(stringValueData);
        stringValueData = tmpValueData;
        tmpValueData = NULL;

        if (DialogBoxW(0, MAKEINTRESOURCEW(IDD_EDIT_MULTI_STRING), hwnd, modify_dlgproc) == IDOK)
        {
            len = lstrlenW( stringValueData );
            tmpValueData = heap_xalloc((len + 2) * sizeof(WCHAR));

            for ( i = 0, j = 0; i < len - 1; i++)
            {
                if ( stringValueData[i] == char1 && stringValueData[i + 1] == char2)
                {
                    if ( tmpValueData[j - 1] != 0)
                        tmpValueData[j++] = 0;
                    i++;
                }
                else
                    tmpValueData[j++] = stringValueData[i];
            }
            tmpValueData[j++] = stringValueData[i];
            tmpValueData[j++] = 0;
            tmpValueData[j++] = 0;
            heap_free(stringValueData);
            stringValueData = tmpValueData;

            lRet = RegSetValueExW(hKey, valueName, 0, type, (LPBYTE)stringValueData, j * sizeof(WCHAR));
            if (lRet == ERROR_SUCCESS) result = TRUE;
            else error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
        }
    }
    else /* hex data types */
    {
        struct edit_params params;

        params.hkey = hKey;
        params.value_name = valueName;
        params.type = type;
        params.data = stringValueData;
        params.size = len;
        result = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_EDIT_BINARY), hwnd,
                                 bin_modify_dlgproc, (LPARAM)&params);
    }

    /* Update the listview item with the new data string */
    if (result)
    {
        int index = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1,
                                 MAKELPARAM(LVNI_FOCUSED | LVNI_SELECTED, 0));
        heap_free(stringValueData);
        stringValueData = read_value(hwnd, hKey, valueName, &type, &len);
        format_value_data(g_pChildWnd->hListWnd, index, type, stringValueData, len);
    }

done:
    heap_free(stringValueData);
    stringValueData = NULL;
    RegCloseKey(hKey);
    return result;
}

BOOL DeleteKey(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath)
{
    BOOL result = FALSE;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ|KEY_SET_VALUE, &hKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_DELETE_KEY_FAILED);
	return FALSE;
    }
    
    if (messagebox(hwnd, MB_YESNO | MB_ICONEXCLAMATION, IDS_DELETE_KEY_TITLE,
                   IDS_DELETE_KEY_TEXT) != IDYES)
	goto done;
	
    lRet = SHDeleteKeyW(hKeyRoot, keyPath);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_BAD_KEY, keyPath);
	goto done;
    }
    result = TRUE;
    
done:
    RegCloseKey(hKey);
    return result;
}

BOOL DeleteValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName)
{
    BOOL result = FALSE;
    LONG lRet;
    HKEY hKey;

    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet) return FALSE;

    lRet = RegDeleteValueW(hKey, valueName);
    if (lRet && valueName) {
        error_code_messagebox(hwnd, IDS_BAD_VALUE, valueName);
    }
    if (lRet) goto done;
    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}

BOOL CreateValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, DWORD valueType, LPWSTR valueName)
{
    LONG lRet = ERROR_SUCCESS;
    WCHAR newValue[256];
    UINT64 value = 0;
    DWORD size_bytes;
    BOOL result = FALSE;
    int valueNum, index;
    HKEY hKey;
    LVITEMW item;
         
    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_CREATE_VALUE_FAILED);
	return FALSE;
    }

    if (!LoadStringW(GetModuleHandleW(0), IDS_NEWVALUE, newValue, ARRAY_SIZE(newValue))) goto done;

    /* try to find a name for the value being created (maximum = 100 attempts) */
    for (valueNum = 1; valueNum < 100; valueNum++) {
	wsprintfW(valueName, newValue, valueNum);
	lRet = RegQueryValueExW(hKey, valueName, 0, 0, 0, 0);
	if (lRet == ERROR_FILE_NOT_FOUND) break;
    }
    if (lRet != ERROR_FILE_NOT_FOUND) {
        error_code_messagebox(hwnd, IDS_CREATE_VALUE_FAILED);
	goto done;
    }

    switch (valueType)
    {
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            size_bytes = sizeof(DWORD);
            break;
        case REG_QWORD:
            size_bytes = sizeof(UINT64);
            break;
        case REG_BINARY:
            size_bytes = 0;
            break;
        case REG_MULTI_SZ:
            size_bytes = 2 * sizeof(WCHAR);
            break;
        default: /* REG_NONE, REG_SZ, REG_EXPAND_SZ */
            size_bytes = sizeof(WCHAR);
    }

    lRet = RegSetValueExW(hKey, valueName, 0, valueType, (BYTE *)&value, size_bytes);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_CREATE_VALUE_FAILED);
	goto done;
    }

    /* Add the new item to the listview */
    index = AddEntryToList(g_pChildWnd->hListWnd, valueName, valueType, (BYTE *)&value, size_bytes, -1);
    item.state = LVIS_FOCUSED | LVIS_SELECTED;
    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    SendMessageW(g_pChildWnd->hListWnd, LVM_SETITEMSTATE, index, (LPARAM)&item);

    result = TRUE;

done:
    RegCloseKey(hKey);
    return result;
}

BOOL RenameValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR oldName, LPCWSTR newName)
{
    LPWSTR value = NULL;
    DWORD type;
    LONG len, lRet;
    BOOL result = FALSE;
    HKEY hKey;

    if (!oldName) return FALSE;
    if (!newName) return FALSE;

    lRet = RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &hKey);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
	return FALSE;
    }

    if (value_exists(hwnd, hKey, newName)) {
        error_code_messagebox(hwnd, IDS_VALUE_EXISTS, oldName);
        goto done;
    }
    value = read_value(hwnd, hKey, oldName, &type, &len);
    if(!value) goto done;
    lRet = RegSetValueExW(hKey, newName, 0, type, (BYTE*)value, len);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
	goto done;
    }
    lRet = RegDeleteValueW(hKey, oldName);
    if (lRet) {
	RegDeleteValueW(hKey, newName);
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
	goto done;
    }
    result = TRUE;

done:
    heap_free(value);
    RegCloseKey(hKey);
    return result;
}


BOOL RenameKey(HWND hwnd, HKEY hRootKey, LPCWSTR keyPath, LPCWSTR newName)
{
    LPWSTR parentPath = 0;
    LPCWSTR srcSubKey = 0;
    HKEY parentKey = 0;
    HKEY destKey = 0;
    BOOL result = FALSE;
    LONG lRet;
    DWORD disposition;

    if (!keyPath || !newName) return FALSE;

    if (!wcsrchr(keyPath, '\\')) {
	parentKey = hRootKey;
	srcSubKey = keyPath;
    } else {
	LPWSTR srcSubKey_copy;

	parentPath = heap_xalloc((lstrlenW(keyPath) + 1) * sizeof(WCHAR));
	lstrcpyW(parentPath, keyPath);
	srcSubKey_copy = wcsrchr(parentPath, '\\');
	*srcSubKey_copy = 0;
	srcSubKey = srcSubKey_copy + 1;
	lRet = RegOpenKeyExW(hRootKey, parentPath, 0, KEY_READ | KEY_CREATE_SUB_KEY, &parentKey);
	if (lRet) {
            error_code_messagebox(hwnd, IDS_RENAME_KEY_FAILED);
	    goto done;
	}
    }

    /* The following fails if the old name is the same as the new name. */
    if (!lstrcmpW(srcSubKey, newName)) goto done;

    lRet = RegCreateKeyExW(parentKey, newName, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL /* FIXME */, &destKey, &disposition);
    if (disposition == REG_OPENED_EXISTING_KEY)
        lRet = ERROR_FILE_EXISTS;
    if (lRet) {
        error_code_messagebox(hwnd, IDS_KEY_EXISTS, srcSubKey);
        goto done;
    }

    /* FIXME: SHCopyKey does not copy the security attributes */
    lRet = SHCopyKeyW(parentKey, srcSubKey, destKey, 0);
    if (lRet) {
        RegCloseKey(destKey);
        RegDeleteKeyW(parentKey, newName);
        error_code_messagebox(hwnd, IDS_RENAME_KEY_FAILED);
        goto done;
    }

    lRet = SHDeleteKeyW(hRootKey, keyPath);
    if (lRet) {
        error_code_messagebox(hwnd, IDS_RENAME_KEY_FAILED);
        goto done;
    }

    result = TRUE;

done:
    RegCloseKey(destKey);
    if (parentKey) {
        RegCloseKey(parentKey); 
        heap_free(parentPath);
    }
    return result;
}
