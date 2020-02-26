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

static void output_writeconsole(const WCHAR *str, DWORD len)
{
    DWORD written, ret, lenA;
    char *strA;

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &written, NULL);
    if (ret) return;

    /* WriteConsole fails if its output is redirected to a file.
     * If this occurs, we should use an OEM codepage and call WriteFile.
     */
    lenA = WideCharToMultiByte(GetConsoleOutputCP(), 0, str, len, NULL, 0, NULL, NULL);
    strA = HeapAlloc(GetProcessHeap(), 0, lenA);
    if (strA)
    {
        WideCharToMultiByte(GetConsoleOutputCP(), 0, str, len, strA, lenA, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &written, FALSE);
        HeapFree(GetProcessHeap(), 0, strA);
    }
}

static void output_formatstring(const WCHAR *fmt, __ms_va_list va_args)
{
    WCHAR *str;
    DWORD len;

    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (LPWSTR)&str, 0, &va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
    {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }
    output_writeconsole(str, len);
    LocalFree(str);
}

static void WINAPIV output_message(unsigned int id, ...)
{
    WCHAR fmt[1024];
    __ms_va_list va_args;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }
    __ms_va_start(va_args, id);
    output_formatstring(fmt, va_args);
    __ms_va_end(va_args);
}

static void WINAPIV output_array(WCHAR *fmt, ...)
{
    __ms_va_list va_args;

    __ms_va_start(va_args, fmt);
    output_formatstring(fmt, va_args);
    __ms_va_end(va_args);
}

/**
 * Used to output program list when used with --list
 */
static void ListUninstallPrograms(void)
{
    unsigned int i;
    static WCHAR fmtW[] = {'%','1','|','|','|','%','2','\n',0};

    FetchUninstallInformation();

    for (i=0; i < numentries; i++)
        output_array(fmtW, entries[i].key, entries[i].descr);
}


static void RemoveSpecificProgram(WCHAR *nameW)
{
    unsigned int i;

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
        output_message(STRING_NO_APP_MATCH, nameW);
}


int __cdecl wmain(int argc, WCHAR *argv[])
{
    LPCWSTR token = NULL;
    static const WCHAR helpW[] = { '-','-','h','e','l','p',0 };
    static const WCHAR listW[] = { '-','-','l','i','s','t',0 };
    static const WCHAR removeW[] = { '-','-','r','e','m','o','v','e',0 };
    int i = 1;
    BOOL is_wow64;

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR filename[MAX_PATH];
        void *redir;
        DWORD exit_code;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        GetModuleFileNameW( 0, filename, MAX_PATH );

        Wow64DisableWow64FsRedirection( &redir );
        if (CreateProcessW( filename, GetCommandLineW(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            WINE_TRACE( "restarting %s\n", wine_dbgstr_w(filename) );
            WaitForSingleObject( pi.hProcess, INFINITE );
            GetExitCodeProcess( pi.hProcess, &exit_code );
            ExitProcess( exit_code );
        }
        else WINE_ERR( "failed to restart 64-bit %s, err %d\n", wine_dbgstr_w(filename), GetLastError() );
        Wow64RevertWow64FsRedirection( redir );
    }

    while( i<argc )
    {
        token = argv[i++];
        
        if( !lstrcmpW( token, helpW ) )
        {
            output_message(STRING_HEADER);
            output_message(STRING_USAGE);
            return 0;
        }
        else if( !lstrcmpW( token, listW ) )
        {
            ListUninstallPrograms();
            return 0;
        }
        else if( !lstrcmpW( token, removeW ) )
        {
            if( i >= argc )
            {
                output_message(STRING_PARAMETER_REQUIRED);
                return 1;
            }

            RemoveSpecificProgram( argv[i++] );
            return 0;
        }
        else 
        {
            output_message(STRING_INVALID_OPTION, token);
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
static int __cdecl cmp_by_name(const void *a, const void *b)
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
        size = sizeof(value);
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

            LoadStringW(hInst, IDS_APPNAME, sAppName, ARRAY_SIZE(sAppName));
            LoadStringW(hInst, IDS_UNINSTALLFAILED, sUninstallFailed, ARRAY_SIZE(sUninstallFailed));
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
