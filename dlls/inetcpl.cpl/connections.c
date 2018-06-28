/*
 * Copyright 2018 Piotr cabna for CodeWeavers
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
 *
 */

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wininet.h>
#include <winuser.h>
#include <winreg.h>

#include "inetcpl.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

static const WCHAR internet_settings[] = {'S','o','f','t','w','a','r','e','\\',
    'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0};
static const WCHAR proxy_enable[] = {'P','r','o','x','y','E','n','a','b','l','e',0};
static const WCHAR proxy_server[] = {'P','r','o','x','y','S','e','r','v','e','r',0};

static BOOL initdialog_done;

static void connections_on_initdialog(HWND hwnd)
{
    DWORD type, size, enabled;
    WCHAR address[INTERNET_MAX_URL_LENGTH], *port;
    HKEY hkey;
    LONG res;

    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER),
            EM_LIMITTEXT, INTERNET_MAX_URL_LENGTH-10, 0);
    SendMessageW(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), EM_LIMITTEXT, 8, 0);

    res = RegOpenKeyW(HKEY_CURRENT_USER, internet_settings, &hkey);
    if(res)
        return;

    size = sizeof(enabled);
    res = RegQueryValueExW(hkey, proxy_enable, NULL, &type, (BYTE*)&enabled, &size);
    if(res || type != REG_DWORD)
        enabled = 0;
    size = sizeof(address);
    res = RegQueryValueExW(hkey, proxy_server, NULL, &type, (BYTE*)address, &size);
    if(res || type != REG_SZ)
        address[0] = 0;
    RegCloseKey(hkey);

    TRACE("ProxyEnable = %x\n", enabled);
    TRACE("ProxyServer = %s\n", wine_dbgstr_w(address));

    if(enabled)
    {
        CheckDlgButton(hwnd, IDC_USE_PROXY_SERVER, BST_CHECKED);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), TRUE);
    }

    port = strchrW(address, ':');
    if(port)
    {
        *port = 0;
        port++;
    }
    SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_SERVER, address);
    if(port)
        SetDlgItemTextW(hwnd, IDC_EDIT_PROXY_PORT, port);

    return;
}

static INT_PTR connections_on_command(HWND hwnd, WPARAM wparam)
{
    BOOL checked;

    switch (wparam)
    {
        case IDC_USE_PROXY_SERVER:
            checked = IsDlgButtonChecked(hwnd, IDC_USE_PROXY_SERVER);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_SERVER), checked);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PROXY_PORT), checked);
            /* fallthrough */
        case MAKEWPARAM(IDC_EDIT_PROXY_SERVER, EN_CHANGE):
        case MAKEWPARAM(IDC_EDIT_PROXY_PORT, EN_CHANGE):
            if(initdialog_done)
                SendMessageW(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            return TRUE;
    }

    return FALSE;
}

static INT_PTR connections_on_notify(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    WCHAR addr[INTERNET_MAX_URL_LENGTH];
    PSHNOTIFY *psn = (PSHNOTIFY*)lparam;
    DWORD addr_len, port_len, enabled;
    LRESULT res;
    HKEY hkey;

    if(psn->hdr.code != PSN_APPLY)
        return FALSE;

    res = RegOpenKeyW(HKEY_CURRENT_USER, internet_settings, &hkey);
    if(res)
        return FALSE;

    enabled = IsDlgButtonChecked(hwnd, IDC_USE_PROXY_SERVER);
    res = RegSetValueExW(hkey, proxy_enable, 0, REG_DWORD,
            (BYTE*)&enabled, sizeof(enabled));
    if(res)
    {
        RegCloseKey(hkey);
        return FALSE;
    }
    TRACE("ProxyEnable set to %x\n", enabled);

    addr_len = GetDlgItemTextW(hwnd, IDC_EDIT_PROXY_SERVER,
            addr, sizeof(addr)/sizeof(addr[0]));
    if(addr_len)
    {
        addr[addr_len++] = ':';
        port_len = GetDlgItemTextW(hwnd, IDC_EDIT_PROXY_PORT,
            addr+addr_len, sizeof(addr)/sizeof(addr[0])-addr_len);
        if(!port_len)
        {
            addr[addr_len++] = '8';
            addr[addr_len++] = '0';
            addr[addr_len] = 0;
        }

        res = RegSetValueExW(hkey, proxy_server, 0, REG_SZ,
                (BYTE*)addr, (addr_len+port_len)*sizeof(WCHAR));
    }
    else
    {
        res = RegDeleteValueW(hkey, proxy_server);
        if(res == ERROR_FILE_NOT_FOUND)
            res = ERROR_SUCCESS;
    }
    TRACE("ProxtServer set to %s\n", wine_dbgstr_w(addr));
    RegCloseKey(hkey);
    return !res;
}

INT_PTR CALLBACK connections_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            connections_on_initdialog(hwnd);
            initdialog_done = TRUE;
            break;
        case WM_COMMAND:
            return connections_on_command(hwnd, wparam);
        case WM_NOTIFY:
            return connections_on_notify(hwnd, wparam, lparam);
    }
    return FALSE;
}
