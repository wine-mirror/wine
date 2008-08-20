/*
 *  ReactOS Task Manager
 *
 *  debug.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *  Copyright (C) 2008  Vladimir Pankratov
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

#define WIN32_LEAN_AND_MEAN    /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <winnt.h>

#include "wine/unicode.h"
#include "taskmgr.h"
#include "perfdata.h"

void ProcessPage_OnDebug(void)
{
    LVITEMW              lvitem;
    ULONG                Index;
    DWORD                dwProcessId;
    WCHAR                wstrErrorText[256];
    HKEY                 hKey;
    WCHAR                wstrDebugPath[256];
    WCHAR                wstrDebugger[256];
    DWORD                dwDebuggerSize;
    PROCESS_INFORMATION  pi;
    STARTUPINFOW         si;
    HANDLE               hDebugEvent;

    static const WCHAR    wszWarnTitle[] = {'T','a','s','k',' ','M','a','n','a','g','e','r',' ',
                                            'W','a','r','n','i','n','g',0};
    static const WCHAR    wszUnable2Debug[] = {'U','n','a','b','l','e',' ','t','o',' ','D','e','b','u','g',' ',
                                              'P','r','o','c','e','s','s',0};
    static const WCHAR    wszSubKey[] = {'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'W','i','n','d','o','w','s',' ','N','T','\\',
                                         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                         'A','e','D','e','b','u','g',0};
    static const WCHAR    wszDebugger[] = {'D','e','b','u','g','g','e','r',0};
    static const WCHAR    wszWarnMsg[] = {'W','A','R','N','I','N','G',':',' ','D','e','b','u','g','g','i','n','g',' ',
                                          't','h','i','s',' ','p','r','o','c','e','s','s',' ','m','a','y',' ',
                                          'r','e','s','u','l','t',' ','i','n',' ','l','o','s','s',' ','o','f',' ',
                                          'd','a','t','a','.','\n','A','r','e',' ','y','o','u',' ','s','u','r','e',' ',
                                          'y','o','u',' ','w','i','s','h',' ','t','o',' ','a','t','t','a','c','h',' ',
                                          't','h','e',' ','d','e','b','u','g','g','e','r','?',0};

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        lvitem.iSubItem = 0;

        SendMessageW(hProcessPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    if (MessageBoxW(hMainWnd, wszWarnMsg, wszWarnTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    dwDebuggerSize = 260;
    if (RegQueryValueExW(hKey, wszDebugger, NULL, NULL, (LPBYTE)wstrDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hDebugEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!hDebugEvent)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    wsprintfW(wstrDebugPath, wstrDebugger, dwProcessId, hDebugEvent);

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
    memset(&si, 0, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!CreateProcessW(NULL, wstrDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hDebugEvent);
}
