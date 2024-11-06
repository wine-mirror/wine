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

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>
#include <winnt.h>

#include "taskmgr.h"
#include "perfdata.h"

void ProcessPage_OnDebug(void)
{
    LVITEMW              lvitem;
    ULONG                Index, Count;
    DWORD                dwProcessId;
    WCHAR                wstrErrorText[256];
    HKEY                 hKey;
    WCHAR                wstrDebugPath[256];
    WCHAR                wstrDebugger[256];
    DWORD                dwDebuggerSize;
    PROCESS_INFORMATION  pi;
    STARTUPINFOW         si;
    HANDLE               hDebugEvent;

    WCHAR    wszWarnTitle[255];
    WCHAR    wszUnable2Debug[255];
    WCHAR    wszWarnMsg[255];

    LoadStringW(hInst, IDS_WARNING_TITLE, wszWarnTitle, ARRAY_SIZE(wszWarnTitle));
    LoadStringW(hInst, IDS_DEBUG_UNABLE2DEBUG, wszUnable2Debug, ARRAY_SIZE(wszUnable2Debug));
    LoadStringW(hInst, IDS_DEBUG_MESSAGE, wszWarnMsg, ARRAY_SIZE(wszWarnMsg));

    Count = SendMessageW(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (Index=0; Index<Count; Index++)
    {
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        lvitem.iSubItem = 0;

        SendMessageW(hProcessPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    Count = SendMessageW(hProcessPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0);
    dwProcessId = PerfDataGetProcessId(Index);
    if ((Count != 1) || (dwProcessId == 0))
        return;

    if (MessageBoxW(hMainWnd, wszWarnMsg, wszWarnTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
    {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    dwDebuggerSize = 260;
    if (RegQueryValueExW(hKey, L"Debugger", NULL, NULL, (LPBYTE)wstrDebugger, &dwDebuggerSize) != ERROR_SUCCESS)
    {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);

    hDebugEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hDebugEvent)
    {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
        return;
    }

    wsprintfW(wstrDebugPath, wstrDebugger, dwProcessId, hDebugEvent);

    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
    memset(&si, 0, sizeof(STARTUPINFOW));
    si.cb = sizeof(STARTUPINFOW);
    if (!CreateProcessW(NULL, wstrDebugPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Debug, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hDebugEvent);
}
