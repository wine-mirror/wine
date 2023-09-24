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
#include <shellapi.h>
#include <shlwapi.h>

#include "main.h"

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

static BOOL update_registry_value(HWND hwndDlg, struct edit_params *params)
{
    HWND hwndValue;
    unsigned int len;
    WCHAR *buf;
    LONG ret;

    hwndValue = GetDlgItem(hwndDlg, IDC_VALUE_DATA);
    len = GetWindowTextLengthW(hwndValue);
    buf = malloc((len + 1) * sizeof(WCHAR));
    len = GetWindowTextW(hwndValue, buf, len + 1);

    free(params->data);

    switch (params->type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
        params->data = buf;
        params->size = (len + 1) * sizeof(WCHAR);
        break;
    case REG_DWORD:
        params->size = sizeof(DWORD);
        params->data = malloc(params->size);
        swscanf(buf, isDecimal ? L"%lu" : L"%lx", params->data);
        free(buf);
        break;
    case REG_QWORD:
        params->size = sizeof(UINT64);
        params->data = malloc(params->size);
        swscanf(buf, isDecimal ? L"%I64u" : L"%I64x", params->data);
        free(buf);
        break;
    case REG_MULTI_SZ:
    {
        WCHAR *tmp = malloc((len + 2) * sizeof(WCHAR));
        unsigned int i, j;

        for (i = 0, j = 0; i < len; i++)
        {
            if (buf[i] == '\r' && buf[i + 1] == '\n')
            {
                if (tmp[j - 1]) tmp[j++] = 0;
                i++;
            }
            else tmp[j++] = buf[i];
        }

        tmp[j++] = 0;
        tmp[j++] = 0;

        free(buf);
        params->data = tmp;
        params->size = j * sizeof(WCHAR);
        break;
    }
    default: /* hex data types */
        free(buf);
        params->size = SendMessageW(hwndValue, HEM_GETDATA, 0, 0);
        params->data = malloc(params->size);
        SendMessageW(hwndValue, HEM_GETDATA, (WPARAM)params->size, (LPARAM)params->data);
    }

    ret = RegSetValueExW(params->hkey, params->value_name, 0, params->type, (BYTE *)params->data, params->size);
    if (ret) error_code_messagebox(hwndDlg, IDS_SET_VALUE_FAILED);

    return !ret;
}

static void set_dlgproc_value_name(HWND hwndDlg, struct edit_params *params)
{
    if (params->value_name)
        SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, params->value_name);
    else
        SetDlgItemTextW(hwndDlg, IDC_VALUE_NAME, g_pszDefaultValueName);
}

static INT_PTR CALLBACK modify_string_dlgproc(HWND hwndDlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct edit_params *params;
    int ret = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
        params = (struct edit_params *)lparam;
        SetWindowLongPtrW(hwndDlg, DWLP_USER, (ULONG_PTR)params);
        set_dlgproc_value_name(hwndDlg, params);
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, params->data);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            params = (struct edit_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            ret = update_registry_value(hwndDlg, params);
            /* fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, ret);
            return TRUE;
        }
    }
    return FALSE;
}

static void set_dword_edit_limit(HWND hwndDlg, DWORD type)
{
    if (isDecimal)
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, EM_SETLIMITTEXT, type == REG_DWORD ? 10 : 20, 0);
    else
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, EM_SETLIMITTEXT, type == REG_DWORD ? 8 : 16, 0);
}

static void change_dword_base(HWND hwndDlg, BOOL toHex, DWORD type)
{
    WCHAR buf[64];
    unsigned int len;
    UINT64 val;

    len = GetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, buf, ARRAY_SIZE(buf));
    if (!len) SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, L"0");

    if ((isDecimal && !toHex) || (!isDecimal && toHex))
        return;

    if (len)
    {
        swscanf(buf, toHex ? L"%I64u" : L"%I64x", &val);
        swprintf(buf, ARRAY_SIZE(buf), toHex ? L"%I64x" : L"%I64u", val);
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, buf);
    }

    isDecimal = !toHex;

    set_dword_edit_limit(hwndDlg, type);
}

static INT_PTR CALLBACK modify_dword_dlgproc(HWND hwndDlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static struct edit_params *params;
    WCHAR buf[64];
    int ret = 0;

    switch (msg)
    {
    case WM_INITDIALOG:
        params = (struct edit_params *)lparam;
        SetWindowLongPtrW(hwndDlg, DWLP_USER, (ULONG_PTR)params);
        set_dlgproc_value_name(hwndDlg, params);
        SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, params->data);
        CheckRadioButton(hwndDlg, IDC_DWORD_HEX, IDC_DWORD_DEC, IDC_DWORD_HEX);
        isDecimal = FALSE;
        if (params->type == REG_QWORD && LoadStringW(GetModuleHandleW(0), IDS_EDIT_QWORD, buf, ARRAY_SIZE(buf)))
            SetWindowTextW(hwndDlg, buf);
        set_dword_edit_limit(hwndDlg, params->type);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDC_DWORD_HEX:
	    change_dword_base(hwndDlg, TRUE, params->type);
            break;
        case IDC_DWORD_DEC:
	    change_dword_base(hwndDlg, FALSE, params->type);
            break;
        case IDOK:
            params = (struct edit_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            if (!SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, EM_LINELENGTH, 0, 0))
                SetDlgItemTextW(hwndDlg, IDC_VALUE_DATA, L"0");
            ret = update_registry_value(hwndDlg, params);
            /* fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, ret);
            return TRUE;
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK modify_binary_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct edit_params *params;
    int ret = 0;

    switch(uMsg) {
    case WM_INITDIALOG:
        params = (struct edit_params *)lParam;
        SetWindowLongPtrW(hwndDlg, DWLP_USER, (ULONG_PTR)params);
        set_dlgproc_value_name(hwndDlg, params);
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, HEM_SETDATA, (WPARAM)params->size, (LPARAM)params->data);
        SendDlgItemMessageW(hwndDlg, IDC_VALUE_DATA, WM_SETFONT, (WPARAM) GetStockObject(ANSI_FIXED_FONT), TRUE);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            params = (struct edit_params *)GetWindowLongPtrW(hwndDlg, DWLP_USER);
            ret = update_registry_value(hwndDlg, params);
            /* fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, ret);
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL read_value(HWND hwnd, struct edit_params *params)
{
    LONG ret;
    WCHAR *buf = NULL;

    if ((ret = RegQueryValueExW(params->hkey, params->value_name, NULL, &params->type, NULL, &params->size)))
    {
        if (ret == ERROR_FILE_NOT_FOUND && !params->value_name)
        {
            params->type = REG_SZ;
            params->size = sizeof(WCHAR);
            params->data = malloc(params->size);
            *(WCHAR *)params->data = 0;
            return TRUE;
        }

        goto error;
    }

    buf = malloc(params->size + sizeof(WCHAR));

    if (RegQueryValueExW(params->hkey, params->value_name, NULL, &params->type, (BYTE *)buf, &params->size))
        goto error;

    if (params->size % sizeof(WCHAR) == 0)
        buf[params->size / sizeof(WCHAR)] = 0;

    params->data = buf;

    return TRUE;

error:
    error_code_messagebox(hwnd, IDS_BAD_VALUE, params->value_name);
    free(buf);
    params->data = NULL;
    return FALSE;
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

static void format_dlgproc_string(struct edit_params *params)
{
    int i, j, count, len;
    WCHAR *str, *buf;

    if (params->type == REG_DWORD || params->type == REG_QWORD)
    {
        UINT64 value = *((UINT64 *)params->data);

        params->data = realloc(params->data, 32 * sizeof(WCHAR));
        swprintf(params->data, 32, params->type == REG_DWORD ? L"%lx" : L"%I64x", value);
        return;
    }

    /* REG_MULTI_SZ */
    len = params->size / sizeof(WCHAR);
    str = params->data;

    for (i = 0, count = 0; i < len; i++)
    {
        if (!str[i] && str[i + 1]) count++;
    }

    buf = malloc(params->size + (count * sizeof(WCHAR)));

    for (i = 0, j = 0; i < len; i++)
    {
        if (!str[i] && str[i + 1])
        {
            buf[j++] = '\r';
            buf[j++] = '\n';
        }
        else buf[j++] = str[i];
    }

    free(params->data);
    str = NULL;
    params->data = buf;
}

BOOL ModifyValue(HWND hwnd, HKEY hKeyRoot, LPCWSTR keyPath, LPCWSTR valueName)
{
    struct edit_params params;
    BOOL result = FALSE;

    if (RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &params.hkey))
    {
        error_code_messagebox(hwnd, IDS_SET_VALUE_FAILED);
        return FALSE;
    }

    params.value_name = valueName;

    if (!read_value(hwnd, &params))
    {
        RegCloseKey(params.hkey);
        return FALSE;
    }

    switch (params.type)
    {
    case REG_SZ:
    case REG_EXPAND_SZ:
        result = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_EDIT_STRING), hwnd,
                                 modify_string_dlgproc, (LPARAM)&params);
        break;
    case REG_DWORD:
    case REG_QWORD:
        format_dlgproc_string(&params);
        result = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_EDIT_DWORD), hwnd,
                                 modify_dword_dlgproc, (LPARAM)&params);
        break;
    case REG_MULTI_SZ:
        format_dlgproc_string(&params);
        result = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_EDIT_MULTI_STRING), hwnd,
                                 modify_string_dlgproc, (LPARAM)&params);
        break;
    default: /* hex data types */
        result = DialogBoxParamW(NULL, MAKEINTRESOURCEW(IDD_EDIT_BINARY), hwnd,
                                 modify_binary_dlgproc, (LPARAM)&params);
    }

    /* Update the listview item with the new data string */
    if (result)
    {
        int index = SendMessageW(g_pChildWnd->hListWnd, LVM_GETNEXTITEM, -1,
                                 MAKELPARAM(LVNI_FOCUSED | LVNI_SELECTED, 0));

        format_value_data(g_pChildWnd->hListWnd, index, params.type, params.data, params.size);
    }

    free(params.data);
    RegCloseKey(params.hkey);
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
    struct edit_params params;
    BOOL result = FALSE;

    if (!oldName) return FALSE;
    if (!newName) return FALSE;

    if (RegOpenKeyExW(hKeyRoot, keyPath, 0, KEY_READ | KEY_SET_VALUE, &params.hkey))
    {
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
        return FALSE;
    }

    if (!RegQueryValueExW(params.hkey, newName, NULL, NULL, NULL, NULL))
    {
        error_code_messagebox(hwnd, IDS_VALUE_EXISTS, oldName);
        goto done;
    }

    params.value_name = oldName;
    if (!read_value(hwnd, &params)) goto done;

    if (RegSetValueExW(params.hkey, newName, 0, params.type, (BYTE *)params.data, params.size))
    {
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
        goto done;
    }

    if (RegDeleteValueW(params.hkey, oldName))
    {
        RegDeleteValueW(params.hkey, newName);
        error_code_messagebox(hwnd, IDS_RENAME_VALUE_FAILED);
        goto done;
    }

    result = TRUE;

done:
    free(params.data);
    RegCloseKey(params.hkey);
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

	parentPath = wcsdup(keyPath);
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
    RegCloseKey(parentKey);
    free(parentPath);
    return result;
}
