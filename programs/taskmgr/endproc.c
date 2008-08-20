/*
 *  ReactOS Task Manager
 *
 *  endproc.c
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

static const WCHAR    wszWarnMsg[] = {'W','A','R','N','I','N','G',':',' ','T','e','r','m','i','n','a','t','i','n','g',' ','a',' ','p','r','o','c','e','s','s',' ','c','a','n',' ','c','a','u','s','e',' ','u','n','d','e','s','i','r','e','d','\n','r','e','s','u','l','t','s',' ','i','n','c','l','u','d','i','n','g',' ','l','o','s','s',' ','o','f',' ','d','a','t','a',' ','a','n','d',' ','s','y','s','t','e','m',' ','i','n','s','t','a','b','i','l','i','t','y','.',' ','T','h','e','\n','p','r','o','c','e','s','s',' ','w','i','l','l',' ','n','o','t',' ','b','e',' ','g','i','v','e','n',' ','t','h','e',' ','c','h','a','n','c','e',' ','t','o',' ','s','a','v','e',' ','i','t','s',' ','s','t','a','t','e',' ','o','r','\n','d','a','t','a',' ','b','e','f','o','r','e',' ','i','t',' ','i','s',' ','t','e','r','m','i','n','a','t','e','d','.',' ','A','r','e',' ','y','o','u',' ','s','u','r','e',' ','y','o','u',' ','w','a','n','t',' ','t','o','\n','t','e','r','m','i','n','a','t','e',' ','t','h','e',' ','p','r','o','c','e','s','s','?',0};
static const WCHAR    wszWarnTitle[] = {'T','a','s','k',' ','M','a','n','a','g','e','r',' ','W','a','r','n','i','n','g',0};
static const WCHAR    wszUnable2Terminate[] = {'U','n','a','b','l','e',' ','t','o',' ','T','e','r','m','i','n','a','t','e',' ','P','r','o','c','e','s','s',0};

void ProcessPage_OnEndProcess(void)
{
    LVITEMW          lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE           hProcess;
    WCHAR            wstrErrorText[256];

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
        return;

    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText,wszUnable2Terminate, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!TerminateProcess(hProcess, 0))
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText,wszUnable2Terminate, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}

void ProcessPage_OnEndProcessTree(void)
{
    LVITEMW          lvitem;
    ULONG            Index;
    DWORD            dwProcessId;
    HANDLE           hProcess;
    WCHAR            wstrErrorText[256];

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
        return;

    hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);

    if (!hProcess)
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText,wszUnable2Terminate, MB_OK|MB_ICONSTOP);
        return;
    }

    if (!TerminateProcess(hProcess, 0))
    {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText,wszUnable2Terminate, MB_OK|MB_ICONSTOP);
    }

    CloseHandle(hProcess);
}
