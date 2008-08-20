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
#include <malloc.h>
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

    static const WCHAR    wszWarnMsg[] = {'W','A','R','N','I','N','G',':',' ','C','h','a','n','g','i','n','g',
' ','t','h','e',' ','p','r','i','o','r','i','t','y',' ','c','l','a','s','s',' ','o','f',' ','t','h','i','s',' ','p','r','o','c','e','s','s',' ','m','a','y','\n','c','a','u','s','e',' ','u','n','d','e','s','i','r','e','d',' ','r','e','s','u','l','t','s',' ','i','n','c','l','u','d','i','n','g',' ','s','y','s','t','e','m',' ','i','n','s','t','a','b','i','l','i','t','y','.',' ','A','r','e',' ','y','o','u','\n','s','u','r','e',' ','y','o','u',' ','w','a','n','t',' ','t','o',' ','c','h','a','n','g','e',' ','t','h','e',' ','p','r','i','o','r','i','t','y',' ','c','l','a','s','s','?',0};
    static const WCHAR    wszWarnTitle[] = {'T','a','s','k',' ','M','a','n','a','g','e','r',' ','W','a','r','n','i','n','g',0};
    static const WCHAR    wszUnable2Change[] = {'U','n','a','b','l','e',' ','t','o',' ','C','h','a','n','g','e',' ','P','r','i','o','r','i','t','y',0};

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
