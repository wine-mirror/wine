/*
 *  ReactOS Task Manager
 *
 *  procpage.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>
#include <winnt.h>

#include "taskmgr.h"
#include "perfdata.h"
#include "column.h"

HWND hProcessPage;                        /* Process List Property Page */

HWND hProcessPageListCtrl;                /* Process ListCtrl Window */
HWND hProcessPageHeaderCtrl;            /* Process Header Control */
HWND hProcessPageEndProcessButton;        /* Process End Process button */
HWND hProcessPageShowAllProcessesButton;/* Process Show All Processes checkbox */

static int    nProcessPageWidth;
static int    nProcessPageHeight;

static HANDLE    hProcessPageEvent = NULL;    /* When this event becomes signaled then we refresh the process list */


static void CommaSeparateNumberString(LPWSTR strNumber, int nMaxCount)
{
    WCHAR    temp[260];
    UINT    i, j, k;
    int len = lstrlenW(strNumber);

    for (i=0; i < len % 3; i++)
        temp[i] = strNumber[i];
    for (k=0,j=i; i < len; i++,j++,k++) {
        if ((k % 3 == 0) && (j > 0))
            temp[j++] = ',';
        temp[j] = strNumber[i];
    }
    temp[j++] = 0;
    memcpy(strNumber, temp, min(nMaxCount, j) * sizeof(WCHAR));
}

static void ProcessPageShowContextMenu(DWORD dwProcessId)
{
    HMENU        hMenu;
    HMENU        hSubMenu;
    HMENU        hPriorityMenu;
    POINT        pt;
    SYSTEM_INFO  si;
    HANDLE       hProcess;
    DWORD        dwProcessPriorityClass;
    WCHAR        strDebugger[260];
    DWORD        dwDebuggerSize;
    HKEY         hKey;
    UINT         Idx;
    static const WCHAR wszAeDebugRegPath[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'A','e','D','e','b','u','g',0};

    memset(&si, 0, sizeof(SYSTEM_INFO));

    GetCursorPos(&pt);
    GetSystemInfo(&si);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_PROCESS_PAGE_CONTEXT));
    hSubMenu = GetSubMenu(hMenu, 0);
    hPriorityMenu = GetSubMenu(hSubMenu, 4);

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    dwProcessPriorityClass = GetPriorityClass(hProcess);
    CloseHandle(hProcess);

    if (si.dwNumberOfProcessors < 2)
        RemoveMenu(hSubMenu, ID_PROCESS_PAGE_SETAFFINITY, MF_BYCOMMAND);

    switch (dwProcessPriorityClass)    {
    case REALTIME_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, MF_BYCOMMAND);
        break;
    case HIGH_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_HIGH, MF_BYCOMMAND);
        break;
    case ABOVE_NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_ABOVENORMAL, MF_BYCOMMAND);
        break;
    case NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_NORMAL, MF_BYCOMMAND);
        break;
    case BELOW_NORMAL_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_BELOWNORMAL, MF_BYCOMMAND);
        break;
    case IDLE_PRIORITY_CLASS:
        CheckMenuRadioItem(hPriorityMenu, ID_PROCESS_PAGE_SETPRIORITY_REALTIME, ID_PROCESS_PAGE_SETPRIORITY_LOW, ID_PROCESS_PAGE_SETPRIORITY_LOW, MF_BYCOMMAND);
        break;
    }

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wszAeDebugRegPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        static const WCHAR wszDebugger[] = {'D','e','b','u','g','g','e','r',0};
        dwDebuggerSize = 260;
        if (RegQueryValueExW(hKey, wszDebugger, NULL, NULL, (LPBYTE)strDebugger, &dwDebuggerSize) == ERROR_SUCCESS)
        {
            static const WCHAR wszDRWTSN32[] = {'D','R','W','T','S','N','3','2',0};
            for (Idx=0; Idx < lstrlenW(strDebugger); Idx++)
                strDebugger[Idx] = toupper(strDebugger[Idx]);

            if (wcsstr(strDebugger, wszDRWTSN32))
                EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
        else
            EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

        RegCloseKey(hKey);
    } else {
        EnableMenuItem(hSubMenu, ID_PROCESS_PAGE_DEBUG, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);
    DestroyMenu(hMenu);
}

static void ProcessPageOnNotify(LPARAM lParam)
{
    LPNMHDR            pnmh;
    NMLVDISPINFOW*     pnmdi;
    LVITEMW            lvitem;
    ULONG              Index, Count;
    ULONG              ColumnIndex;
    IO_COUNTERS        iocounters;
    TIME               time;
    static const WCHAR wszFmtD[] = {'%','d',0};
    static const WCHAR wszFmt02D[] = {'%','0','2','d',0};
    static const WCHAR wszUnitK[] = {' ','K',0};

    pnmh = (LPNMHDR) lParam;
    pnmdi = (NMLVDISPINFOW*) lParam;

    if (pnmh->hwndFrom == hProcessPageListCtrl)
    {
        switch (pnmh->code)
        {
        case LVN_GETDISPINFOW:

            if (!(pnmdi->item.mask & LVIF_TEXT))
                break;
            
            ColumnIndex = pnmdi->item.iSubItem;
            Index = pnmdi->item.iItem;

            if (ColumnDataHints[ColumnIndex] == COLUMN_IMAGENAME)
                PerfDataGetImageName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
            if (ColumnDataHints[ColumnIndex] == COLUMN_PID)
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetProcessId(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_USERNAME)
                PerfDataGetUserName(Index, pnmdi->item.pszText, pnmdi->item.cchTextMax);
            if (ColumnDataHints[ColumnIndex] == COLUMN_SESSIONID)
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetSessionId(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_CPUUSAGE)
                wsprintfW(pnmdi->item.pszText, wszFmt02D, PerfDataGetCPUUsage(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_CPUTIME)
            {
                DWORD dwHours;
                DWORD dwMinutes;
                DWORD dwSeconds;
                ULONGLONG secs;
                static const WCHAR timefmt[] = {'%','d',':','%','0','2','d',':','%','0','2','d',0};

                time = PerfDataGetCPUTime(Index);
                secs = time.QuadPart / 10000000;
                dwHours = secs / 3600;
                dwMinutes = (secs % 3600) / 60;
                dwSeconds = (secs % 3600) % 60;
                wsprintfW(pnmdi->item.pszText, timefmt, dwHours, dwMinutes, dwSeconds);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGE)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetWorkingSetSizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PEAKMEMORYUSAGE)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetPeakWorkingSetSizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_MEMORYUSAGEDELTA)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetWorkingSetSizeDelta(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTS)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetPageFaultCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEFAULTSDELTA)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetPageFaultCountDelta(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_VIRTUALMEMORYSIZE)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetVirtualMemorySizeBytes(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_PAGEDPOOL)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetPagedPoolUsagePages(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_NONPAGEDPOOL)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetNonPagedPoolUsagePages(Index) / 1024);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
                wcscat(pnmdi->item.pszText, wszUnitK);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_BASEPRIORITY)
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetBasePriority(Index));
            if (ColumnDataHints[ColumnIndex] == COLUMN_HANDLECOUNT)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetHandleCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_THREADCOUNT)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetThreadCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_USEROBJECTS)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetUSERObjectCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_GDIOBJECTS)
            {
                wsprintfW(pnmdi->item.pszText, wszFmtD, PerfDataGetGDIObjectCount(Index));
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADS)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.ReadOperationCount); */
                _ui64tow(iocounters.ReadOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.WriteOperationCount); */
                _ui64tow(iocounters.WriteOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHER)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.OtherOperationCount); */
                _ui64tow(iocounters.OtherOperationCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOREADBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.ReadTransferCount); */
                _ui64tow(iocounters.ReadTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOWRITEBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.WriteTransferCount); */
                _ui64tow(iocounters.WriteTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }
            if (ColumnDataHints[ColumnIndex] == COLUMN_IOOTHERBYTES)
            {
                PerfDataGetIOCounters(Index, &iocounters);
                /* wsprintfW(pnmdi->item.pszText, wszFmtD, iocounters.OtherTransferCount); */
                _ui64tow(iocounters.OtherTransferCount, pnmdi->item.pszText, 10);
                CommaSeparateNumberString(pnmdi->item.pszText, pnmdi->item.cchTextMax);
            }

            break;

        case NM_RCLICK:
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

            if ((SendMessageW(hProcessPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0) == 1) &&
                (PerfDataGetProcessId(Index) != 0))
            {
                ProcessPageShowContextMenu(PerfDataGetProcessId(Index));
            }

            break;

        }
    }
    else if (pnmh->hwndFrom == hProcessPageHeaderCtrl)
    {
        switch (pnmh->code)
        {
        case HDN_ITEMCLICKW:

            /*
             * FIXME: Fix the column sorting
             *
             *ListView_SortItems(hApplicationPageListCtrl, ApplicationPageCompareFunc, NULL);
             *bSortAscending = !bSortAscending;
             */

            break;

        case HDN_ITEMCHANGEDW:

            UpdateColumnDataHints();

            break;

        case HDN_ENDDRAG:

            UpdateColumnDataHints();

            break;

        }
    }

}

void RefreshProcessPage(void)
{
    /* Signal the event so that our refresh thread */
    /* will wake up and refresh the process page */
    SetEvent(hProcessPageEvent);
}

static DWORD WINAPI ProcessPageRefreshThread(void *lpParameter)
{
    ULONG    OldProcessorUsage = 0;
    ULONG    OldProcessCount = 0;

    WCHAR    wszCPU_Usage[255];
    WCHAR    wszProcesses[255];

    LoadStringW(hInst, IDS_STATUS_BAR_CPU_USAGE, wszCPU_Usage, ARRAY_SIZE(wszCPU_Usage));
    LoadStringW(hInst, IDS_STATUS_BAR_PROCESSES, wszProcesses, ARRAY_SIZE(wszProcesses));

    /* Create the event */
    hProcessPageEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

    /* If we couldn't create the event then exit the thread */
    if (!hProcessPageEvent)
        return 0;

    while (1) {
        DWORD    dwWaitVal;

        /* Wait on the event */
        dwWaitVal = WaitForSingleObject(hProcessPageEvent, INFINITE);

        /* If the wait failed then the event object must have been */
        /* closed and the task manager is exiting so exit this thread */
        if (dwWaitVal == WAIT_FAILED)
            return 0;

        if (dwWaitVal == WAIT_OBJECT_0) {
            WCHAR    text[256];

            /* Reset our event */
            ResetEvent(hProcessPageEvent);

            if (SendMessageW(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0) != PerfDataGetProcessCount())
                SendMessageW(hProcessPageListCtrl, LVM_SETITEMCOUNT, PerfDataGetProcessCount(), /*LVSICF_NOINVALIDATEALL|*/LVSICF_NOSCROLL);

            if (IsWindowVisible(hProcessPage))
                InvalidateRect(hProcessPageListCtrl, NULL, FALSE);

            if (OldProcessorUsage != PerfDataGetProcessorUsage()) {
                OldProcessorUsage = PerfDataGetProcessorUsage();
                wsprintfW(text, wszCPU_Usage, OldProcessorUsage);
                SendMessageW(hStatusWnd, SB_SETTEXTW, 1, (LPARAM)text);
            }
            if (OldProcessCount != PerfDataGetProcessCount()) {
                OldProcessCount = PerfDataGetProcessCount();
                wsprintfW(text, wszProcesses, OldProcessCount);
                SendMessageW(hStatusWnd, SB_SETTEXTW, 0, (LPARAM)text);
            }
        }
    }
        return 0;
}

INT_PTR CALLBACK
ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT    rc;
    int        nXDifference;
    int        nYDifference;
    int        cx, cy;

    switch (message) {
    case WM_INITDIALOG:
        /*
         * Save the width and height
         */
        GetClientRect(hDlg, &rc);
        nProcessPageWidth = rc.right;
        nProcessPageHeight = rc.bottom;

        /* Update window position */
        SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

        /*
         * Get handles to the controls
         */
        hProcessPageListCtrl = GetDlgItem(hDlg, IDC_PROCESSLIST);
        hProcessPageHeaderCtrl = (HWND)SendMessageW(hProcessPageListCtrl, LVM_GETHEADER, 0, 0);
        hProcessPageEndProcessButton = GetDlgItem(hDlg, IDC_ENDPROCESS);
        hProcessPageShowAllProcessesButton = GetDlgItem(hDlg, IDC_SHOWALLPROCESSES);

        /* Enable manual column reordering, set full select */
        SendMessageW(hProcessPageListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP,
            LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

        AddColumns();

        /*
         * Subclass the process list control so we can intercept WM_ERASEBKGND
         */
        OldProcessListWndProc = (WNDPROC)SetWindowLongPtrW(hProcessPageListCtrl, GWLP_WNDPROC, (LONG_PTR)ProcessListWndProc);

        /* Start our refresh thread */
        CloseHandle( CreateThread(NULL, 0, ProcessPageRefreshThread, NULL, 0, NULL));

        return TRUE;

    case WM_DESTROY:
        /* Close the event handle, this will make the */
        /* refresh thread exit when the wait fails */
        CloseHandle(hProcessPageEvent);

        SaveColumnSettings();

        break;

    case WM_COMMAND:
        /* Handle the button clicks */
        switch (LOWORD(wParam))
        {
                case IDC_ENDPROCESS:
                        ProcessPage_OnEndProcess();
        }
        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
        nXDifference = cx - nProcessPageWidth;
        nYDifference = cy - nProcessPageHeight;
        nProcessPageWidth = cx;
        nProcessPageHeight = cy;

        /* Reposition the application page's controls */
        GetWindowRect(hProcessPageListCtrl, &rc);
        cx = (rc.right - rc.left) + nXDifference;
        cy = (rc.bottom - rc.top) + nYDifference;
        SetWindowPos(hProcessPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
        InvalidateRect(hProcessPageListCtrl, NULL, TRUE);
        
        GetClientRect(hProcessPageEndProcessButton, &rc);
        MapWindowPoints(hProcessPageEndProcessButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
           cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageEndProcessButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageEndProcessButton, NULL, TRUE);
        
        GetClientRect(hProcessPageShowAllProcessesButton, &rc);
        MapWindowPoints(hProcessPageShowAllProcessesButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
           cx = rc.left;
        cy = rc.top + nYDifference;
        SetWindowPos(hProcessPageShowAllProcessesButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hProcessPageShowAllProcessesButton, NULL, TRUE);

        break;

    case WM_NOTIFY:
        ProcessPageOnNotify(lParam);
        break;
    }

    return 0;
}
