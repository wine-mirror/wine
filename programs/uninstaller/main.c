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

extern void WINAPI Control_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow);

typedef struct {
    HKEY  root;
    WCHAR *key;
    WCHAR *descr;
    WCHAR *command;
    int active;
} uninst_entry;
static uninst_entry *entries = NULL;
static unsigned int numentries = 0;
static int oldsel = -1;
static WCHAR *sFilter;

static int FetchUninstallInformation(void);
static void UninstallProgram(void);
static int cmp_by_name(const void *a, const void *b);

static const WCHAR DisplayNameW[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR PathUninstallW[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'U','n','i','n','s','t','a','l','l',0 };
static const WCHAR UninstallCommandlineW[] = {'U','n','i','n','s','t','a','l','l','S','t','r','i','n','g',0};
static const WCHAR WindowsInstallerW[] = {'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0};
static const WCHAR SystemComponentW[] = {'S','y','s','t','e','m','C','o','m','p','o','n','e','n','t',0};

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
        if (CompareStringW(GetThreadLocale(), NORM_IGNORECASE, entries[i].key, -1, nameW, -1) == CSTR_EQUAL)
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

    /* Start the GUI control panel */
    Control_RunDLL(GetDesktopWindow(), 0, "appwiz.cpl", SW_SHOW);
    return 1;
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
static int FetchFromRootKey(HKEY root)
{
    HKEY hkeyApp;
    int i;
    DWORD sizeOfSubKeyName, displen, uninstlen, value, type, size;
    WCHAR subKeyName[256];

    sizeOfSubKeyName = 255;
    for (i=0; RegEnumKeyExW( root, i, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS; ++i)
    {
        RegOpenKeyExW(root, subKeyName, 0, KEY_READ, &hkeyApp);
        if (!RegQueryValueExW(hkeyApp, SystemComponentW, NULL, &type, (LPBYTE)&value, &size) &&
            type == REG_DWORD && value == 1)
        {
            RegCloseKey(hkeyApp);
            sizeOfSubKeyName = 255;
            continue;
        }
        if (!RegQueryValueExW(hkeyApp, DisplayNameW, NULL, NULL, NULL, &displen))
        {
            WCHAR *command;

            size = sizeof(value);
            if (!RegQueryValueExW(hkeyApp, WindowsInstallerW, NULL, &type, (LPBYTE)&value, &size) &&
                type == REG_DWORD && value == 1)
            {
                static const WCHAR fmtW[] = {'m','s','i','e','x','e','c',' ','/','x','%','s',0};
                command = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(fmtW) + lstrlenW(subKeyName)) * sizeof(WCHAR));
                wsprintfW(command, fmtW, subKeyName);
            }
            else if (!RegQueryValueExW(hkeyApp, UninstallCommandlineW, NULL, NULL, NULL, &uninstlen))
            {
                command = HeapAlloc(GetProcessHeap(), 0, uninstlen);
                RegQueryValueExW(hkeyApp, UninstallCommandlineW, 0, 0, (LPBYTE)command, &uninstlen);
            }
            else
            {
                RegCloseKey(hkeyApp);
                sizeOfSubKeyName = 255;
                continue;
            }
            numentries++;
            entries = HeapReAlloc(GetProcessHeap(), 0, entries, numentries*sizeof(uninst_entry));
            entries[numentries-1].root = root;
            entries[numentries-1].key = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(subKeyName)+1)*sizeof(WCHAR));
            lstrcpyW(entries[numentries-1].key, subKeyName);
            entries[numentries-1].descr = HeapAlloc(GetProcessHeap(), 0, displen);
            RegQueryValueExW(hkeyApp, DisplayNameW, 0, 0, (LPBYTE)entries[numentries-1].descr, &displen);
            entries[numentries-1].command = command;
            entries[numentries-1].active = 0;
            WINE_TRACE("allocated entry #%d: %s (%s), %s\n",
            numentries, wine_dbgstr_w(entries[numentries-1].key), wine_dbgstr_w(entries[numentries-1].descr), wine_dbgstr_w(entries[numentries-1].command));
            if(sFilter != NULL && StrStrIW(entries[numentries-1].descr,sFilter)==NULL)
                numentries--;
        }
        RegCloseKey(hkeyApp);
        sizeOfSubKeyName = 255;
    }
    return 1;

}

static int FetchUninstallInformation(void)
{
    static const BOOL is_64bit = sizeof(void *) > sizeof(int);
    int rc = 0;
    HKEY root;

    numentries = 0;
    oldsel = -1;
    if (!entries)
        entries = HeapAlloc(GetProcessHeap(), 0, sizeof(uninst_entry));

    if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ, &root))
    {
        rc |= FetchFromRootKey(root);
        RegCloseKey(root);
    }
    if (is_64bit &&
        !RegOpenKeyExW(HKEY_LOCAL_MACHINE, PathUninstallW, 0, KEY_READ|KEY_WOW64_32KEY, &root))
    {
        rc |= FetchFromRootKey(root);
        RegCloseKey(root);
    }
    if (!RegOpenKeyExW(HKEY_CURRENT_USER, PathUninstallW, 0, KEY_READ, &root))
    {
        rc |= FetchFromRootKey(root);
        RegCloseKey(root);
    }

    qsort(entries, numentries, sizeof(uninst_entry), cmp_by_name);
    return rc;
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
            WCHAR sAppName[MAX_STRING_LEN];
            WCHAR sUninstallFailed[MAX_STRING_LEN];
            HINSTANCE hInst = GetModuleHandleW(0);

            LoadStringW(hInst, IDS_APPNAME, sAppName, sizeof(sAppName)/sizeof(WCHAR));
            LoadStringW(hInst, IDS_UNINSTALLFAILED, sUninstallFailed, sizeof(sUninstallFailed)/sizeof(WCHAR));
            wsprintfW(errormsg, sUninstallFailed, entries[i].command);
            if(MessageBoxW(0, errormsg, sAppName, MB_YESNO | MB_ICONQUESTION)==IDYES)
            {
                /* delete the application's uninstall entry */
                RegOpenKeyExW(entries[i].root, PathUninstallW, 0, KEY_READ, &hkey);
                RegDeleteKeyW(hkey, entries[i].key);
                RegCloseKey(hkey);
            }
        }
    }
    WINE_TRACE("finished uninstall phase.\n");
}
