/*
 * Uninstaller
 *
 * Copyright 2000 Andreas Mohr
 * Copyright 2004 Hannu Valtonen
 * Copyright 2005 Jonathan Ernst
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

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shlwapi.h>
#include "resource.h"
#include "regstr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uninstaller);

typedef struct {
    WCHAR *key;
    WCHAR *descr;
    WCHAR *command;
    int active;
} uninst_entry;
static uninst_entry *entries = NULL;
static unsigned int numentries = 0;
static int list_need_update = 1;
static int oldsel = -1;
static WCHAR *sFilter;
static WCHAR sAppName[MAX_STRING_LEN];
static WCHAR sAboutTitle[MAX_STRING_LEN];
static WCHAR sAbout[MAX_STRING_LEN];
static WCHAR sRegistryKeyNotAvailable[MAX_STRING_LEN];
static WCHAR sUninstallFailed[MAX_STRING_LEN];

static int FetchUninstallInformation(void);
static void UninstallProgram(void);
static void UpdateList(HWND hList);
static int cmp_by_name(const void *a, const void *b);
static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);


static const WCHAR BackSlashW[] = { '\\', 0 };
static const WCHAR DisplayNameW[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR PathUninstallW[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'U','n','i','n','s','t','a','l','l',0 };
static const WCHAR UninstallCommandlineW[] = {'U','n','i','n','s','t','a','l','l','S','t','r','i','n','g',0};


/**
 * Used to output program list when used with --list
 */
static void ListUninstallPrograms(void)
{
    unsigned int i;
    int lenDescr, lenKey;
    char *descr;
    char *key;

    FetchUninstallInformation();

    for (i=0; i < numentries; i++)
    {
        lenDescr = WideCharToMultiByte(CP_UNIXCP, 0, entries[i].descr, -1, NULL, 0, NULL, NULL); 
        lenKey = WideCharToMultiByte(CP_UNIXCP, 0, entries[i].key, -1, NULL, 0, NULL, NULL); 
        descr = HeapAlloc(GetProcessHeap(), 0, lenDescr);
        key = HeapAlloc(GetProcessHeap(), 0, lenKey);
        WideCharToMultiByte(CP_UNIXCP, 0, entries[i].descr, -1, descr, lenDescr, NULL, NULL);
        WideCharToMultiByte(CP_UNIXCP, 0, entries[i].key, -1, key, lenKey, NULL, NULL);
        printf("%s|||%s\n", key, descr);
        HeapFree(GetProcessHeap(), 0, descr);
        HeapFree(GetProcessHeap(), 0, key);
    }
}


static void RemoveSpecificProgram(WCHAR *nameW)
{
    unsigned int i;
    int lenName;
    char *name;

    FetchUninstallInformation();

    for (i=0; i < numentries; i++)
    {
        if (lstrcmpW(entries[i].key, nameW) == 0)
        {
            entries[i].active++;
            break;
        }
    }

    if (i < numentries)
        UninstallProgram();
    else
    {
        lenName = WideCharToMultiByte(CP_UNIXCP, 0, nameW, -1, NULL, 0, NULL, NULL); 
        name = HeapAlloc(GetProcessHeap(), 0, lenName);
        WideCharToMultiByte(CP_UNIXCP, 0, nameW, -1, name, lenName, NULL, NULL);
        fprintf(stderr, "Error: could not match application [%s]\n", name);
        HeapFree(GetProcessHeap(), 0, name);
    }
}


int wmain(int argc, WCHAR *argv[])
{
    LPCWSTR token = NULL;
    HINSTANCE hInst = GetModuleHandleW(0);
    static const WCHAR listW[] = { '-','-','l','i','s','t',0 };
    static const WCHAR removeW[] = { '-','-','r','e','m','o','v','e',0 };
    int i = 1;

    while( i<argc )
    {
        token = argv[i++];
        
        /* Handle requests just to list the applications */
        if( !lstrcmpW( token, listW ) )
        {
            ListUninstallPrograms();
            return 0;
        }
        else if( !lstrcmpW( token, removeW ) )
        {
            if( i >= argc )
            {
                WINE_ERR( "The remove option requires a parameter.\n");
                return 1;
            }

            RemoveSpecificProgram( argv[i++] );
            return 0;
        }
        else 
        {
            WINE_ERR( "unknown option %s\n",wine_dbgstr_w(token));
            return 1;
        }
    }

    /* Load MessageBox's strings */
    LoadStringW(hInst, IDS_APPNAME, sAppName, sizeof(sAppName)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_ABOUTTITLE, sAboutTitle, sizeof(sAboutTitle)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_ABOUT, sAbout, sizeof(sAbout)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_REGISTRYKEYNOTAVAILABLE, sRegistryKeyNotAvailable, sizeof(sRegistryKeyNotAvailable)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_UNINSTALLFAILED, sUninstallFailed, sizeof(sUninstallFailed)/sizeof(WCHAR));

    return DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_UNINSTALLER), NULL, DlgProc);
}


/**
 * Used to sort entries by name.
 */
static int cmp_by_name(const void *a, const void *b)
{
    return lstrcmpiW(((const uninst_entry *)a)->descr, ((const uninst_entry *)b)->descr);
}


/**
 * Fetch information from the uninstall key.
 */
static int FetchUninstallInformation(void)
{
    HKEY hkeyUninst, hkeyApp;
    int i;
    DWORD sizeOfSubKeyName, displen, uninstlen;
    WCHAR subKeyName[256];
    WCHAR key_app[1024];
    WCHAR *p;
  
    numentries = 0;
    oldsel = -1;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ, &hkeyUninst) != ERROR_SUCCESS)
        return 0;

    if (!entries)
        entries = HeapAlloc(GetProcessHeap(), 0, sizeof(uninst_entry));

    lstrcpyW(key_app, PathUninstallW);
    lstrcatW(key_app, BackSlashW);
    p = key_app+lstrlenW(PathUninstallW)+1;

    sizeOfSubKeyName = 255;
    for (i=0; RegEnumKeyExW( hkeyUninst, i, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS; ++i)
    {
        lstrcpyW(p, subKeyName);
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, key_app, 0, KEY_READ, &hkeyApp);
        if ((RegQueryValueExW(hkeyApp, DisplayNameW, 0, 0, NULL, &displen) == ERROR_SUCCESS)
         && (RegQueryValueExW(hkeyApp, UninstallCommandlineW, 0, 0, NULL, &uninstlen) == ERROR_SUCCESS))
        {
            numentries++;
            entries = HeapReAlloc(GetProcessHeap(), 0, entries, numentries*sizeof(uninst_entry));
            entries[numentries-1].key = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(subKeyName)+1)*sizeof(WCHAR));
            lstrcpyW(entries[numentries-1].key, subKeyName);
            entries[numentries-1].descr = HeapAlloc(GetProcessHeap(), 0, displen);
            RegQueryValueExW(hkeyApp, DisplayNameW, 0, 0, (LPBYTE)entries[numentries-1].descr, &displen);
            entries[numentries-1].command = HeapAlloc(GetProcessHeap(), 0, uninstlen);
            entries[numentries-1].active = 0;
            RegQueryValueExW(hkeyApp, UninstallCommandlineW, 0, 0, (LPBYTE)entries[numentries-1].command, &uninstlen);
            WINE_TRACE("allocated entry #%d: %s (%s), %s\n",
            numentries, wine_dbgstr_w(entries[numentries-1].key), wine_dbgstr_w(entries[numentries-1].descr), wine_dbgstr_w(entries[numentries-1].command));
            if(sFilter != NULL && StrStrIW(entries[numentries-1].descr,sFilter)==NULL)
                numentries--;
        }
        RegCloseKey(hkeyApp);
        sizeOfSubKeyName = 255;
    }
    qsort(entries, numentries, sizeof(uninst_entry), cmp_by_name);
    RegCloseKey(hkeyUninst);
    return 1;
}


static void UninstallProgram(void)
{
    unsigned int i;
    WCHAR errormsg[1024];
    BOOL res;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    DWORD exit_code;
    HKEY hkey;
    for (i=0; i < numentries; i++)
    {
        if (!(entries[i].active)) /* don't uninstall this one */
            continue;
        WINE_TRACE("uninstalling %s\n", wine_dbgstr_w(entries[i].descr));
        memset(&si, 0, sizeof(STARTUPINFOW));
        si.cb = sizeof(STARTUPINFOW);
        si.wShowWindow = SW_NORMAL;
        res = CreateProcessW(NULL, entries[i].command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info);
        if (res)
        {   /* wait for the process to exit */
            WaitForSingleObject(info.hProcess, INFINITE);
            res = GetExitCodeProcess(info.hProcess, &exit_code);
            WINE_TRACE("%d: %08x\n", res, exit_code);
        }
        else
        {
            wsprintfW(errormsg, sUninstallFailed, entries[i].command);
            if(MessageBoxW(0, errormsg, sAppName, MB_YESNO | MB_ICONQUESTION)==IDYES)
            {
                /* delete the application's uninstall entry */
                RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ, &hkey);
                RegDeleteKeyW(hkey, entries[i].key);
                RegCloseKey(hkey);
            }
        }
    }
    WINE_TRACE("finished uninstall phase.\n");
    list_need_update = 1;
}


static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    TEXTMETRICW tm;
    HDC hdc;
    HWND hList = GetDlgItem(hwnd, IDC_LIST);
    switch(Message)
    {
        case WM_INITDIALOG:
            hdc = GetDC(hwnd);
            GetTextMetricsW(hdc, &tm);
            UpdateList(hList);
            ReleaseDC(hwnd, hdc);
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_FILTER:
                {
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        int len = GetWindowTextLengthW(GetDlgItem(hwnd, IDC_FILTER));
                        list_need_update = 1;
                        if(len > 0)
                        {
                            sFilter = (WCHAR*)GlobalAlloc(GPTR, (len + 1)*sizeof(WCHAR));
                            GetDlgItemTextW(hwnd, IDC_FILTER, sFilter, len + 1);
                        }
                        else sFilter = NULL;
                        UpdateList(hList);
                    }
                    break;
                }
                case IDC_UNINSTALL:
                {
                    int count = SendMessageW(hList, LB_GETSELCOUNT, 0, 0);
                    if(count != 0)
                    {
                        UninstallProgram();
                        UpdateList(hList);
                    }
                    break;
                }
                case IDC_LIST:
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                       int sel = SendMessageW(hList, LB_GETCURSEL, 0, 0);
                       if (oldsel != -1)
                       {
                           entries[oldsel].active ^= 1; /* toggle */
                           WINE_TRACE("toggling %d old %s\n", entries[oldsel].active,
                           wine_dbgstr_w(entries[oldsel].descr));
                       }
                       entries[sel].active ^= 1; /* toggle */
                       WINE_TRACE("toggling %d %s\n", entries[sel].active,
                       wine_dbgstr_w(entries[sel].descr));
                       oldsel = sel;
                   }
                    break;
                case IDC_ABOUT:
                    MessageBoxW(0, sAbout, sAboutTitle, MB_OK);
                    break;
                case IDCANCEL:
                case IDC_EXIT:
                    EndDialog(hwnd, 0);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}


static void UpdateList(HWND hList)
{
    unsigned int i;
    if (list_need_update)
    {
        int prevsel;
        prevsel = SendMessageW(hList, LB_GETCURSEL, 0, 0);
        if (!(FetchUninstallInformation()))
        {
            MessageBoxW(0, sRegistryKeyNotAvailable, sAppName, MB_OK);
            PostQuitMessage(0);
            return;
        }
        SendMessageW(hList, LB_RESETCONTENT, 0, 0);
        SendMessageW(hList, WM_SETREDRAW, FALSE, 0);
        for (i=0; i < numentries; i++)
        {
            WINE_TRACE("adding %s\n", wine_dbgstr_w(entries[i].descr));
            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)entries[i].descr);
        }
        WINE_TRACE("setting prevsel %d\n", prevsel);
        if (prevsel != -1)
            SendMessageW(hList, LB_SETCURSEL, prevsel, 0 );
        SendMessageW(hList, WM_SETREDRAW, TRUE, 0);
        list_need_update = 0;
    }
}
