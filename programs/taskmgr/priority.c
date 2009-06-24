/*
 *  ReactOS Task Manager
 *
 *  priority.c
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
#include <memory.h>
#include <stdio.h>
#include <winnt.h>
    
#include "wine/unicode.h"
#include "taskmgr.h"
#include "perfdata.h"

static void DoSetPriority(DWORD priority)
{
    LVITEMW          lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE           hProcess;
    WCHAR            wstrErrorText[256];

    WCHAR    wszWarnMsg[255];
    WCHAR    wszWarnTitle[255];
    WCHAR    wszUnable2Change[255];

    LoadStringW(hInst, IDS_PRIORITY_CHANGE_MESSAGE, wszWarnMsg, sizeof(wszWarnMsg)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_WARNING_TITLE, wszWarnTitle, sizeof(wszWarnTitle)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PRIORITY_UNABLE2CHANGE, wszUnable2Change, sizeof(wszUnable2Change)/sizeof(WCHAR));

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++)
    {
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        lvitem.iSubItem = 0;

        SendMessageW(hProcessPageListCtrl, LVM_GETITEMW, 0, (LPARAM)&lvitem);

        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    dwProcessId = PerfDataGetProcessId(Index);

    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;

    if (MessageBoxW(hMainWnd, wszWarnMsg, wszWarnTitle, MB_YESNO|MB_ICONWARNING) != IDYES)
        return;

    hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Change, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!SetPriorityClass(hProcess, priority))
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Change, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnSetPriorityRealTime(void)
{
    DoSetPriority(REALTIME_PRIORITY_CLASS);
}

void ProcessPage_OnSetPriorityHigh(void)
{
    DoSetPriority(HIGH_PRIORITY_CLASS);
}

void ProcessPage_OnSetPriorityAboveNormal(void)
{
    DoSetPriority(ABOVE_NORMAL_PRIORITY_CLASS);
}

void ProcessPage_OnSetPriorityNormal(void)
{
    DoSetPriority(NORMAL_PRIORITY_CLASS);
}

void ProcessPage_OnSetPriorityBelowNormal(void)
{
    DoSetPriority(BELOW_NORMAL_PRIORITY_CLASS);
}

void ProcessPage_OnSetPriorityLow(void)
{
    DoSetPriority(IDLE_PRIORITY_CLASS);
}
