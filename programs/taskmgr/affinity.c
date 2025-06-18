/*
 *  ReactOS Task Manager
 *
 *  affinity.c
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
 *  Copyright (C) 2008  Vladimir Pankratov
 *  Copyright (C) 2019  Isira Seneviratne
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

HANDLE        hProcessAffinityHandle;

WCHAR    wszUnable2Access[255];

static INT_PTR CALLBACK
AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD_PTR dwProcessAffinityMask = 0;
    DWORD_PTR dwSystemAffinityMask = 0;
    WCHAR     wstrErrorText[256];
    int       i;

    switch (message) {
    case WM_INITDIALOG:

        /*
         * Get the current affinity mask for the process and
         * the number of CPUs present in the system
         */
        if (!GetProcessAffinityMask(hProcessAffinityHandle, &dwProcessAffinityMask, &dwSystemAffinityMask))    {
            GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
            EndDialog(hDlg, 0);
            LoadStringW(hInst, IDS_AFFINITY_UNABLE2ACCESS, wszUnable2Access, ARRAY_SIZE(wszUnable2Access));
            MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Access, MB_OK|MB_ICONSTOP);
        }

        /*
         * Enable a checkbox for each processor present in the system
         */
        for (i = 0; i < 32; i++)
            if (dwSystemAffinityMask & (1 << i))
                EnableWindow(GetDlgItem(hDlg, IDC_CPU0 + i), TRUE);


        /*
         * Check each checkbox that the current process
         * has affinity with
         */
        for (i = 0; i < 32; i++)
            if (dwProcessAffinityMask & (1 << i))
                SendMessageW(GetDlgItem(hDlg, IDC_CPU0 + i), BM_SETCHECK, BST_CHECKED, 0);

        return TRUE;

    case WM_COMMAND:

        /*
         * If the user has cancelled the dialog box
         * then just close it
         */
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        /*
         * The user has clicked OK -- so now we have
         * to adjust the process affinity mask
         */
        if (LOWORD(wParam) == IDOK) {
            /*
             * First we have to create a mask out of each
             * checkbox that the user checked.
             */
            for (i = 0; i < 32; i++)
                if (SendMessageW(GetDlgItem(hDlg, IDC_CPU0 + i), BM_GETCHECK, 0, 0))
                    dwProcessAffinityMask |= (1 << i);

            /*
             * Make sure they are giving the process affinity
             * with at least one processor. I'd hate to see a
             * process that is not in a wait state get deprived
             * of its cpu time.
             */
            if (!dwProcessAffinityMask) {
                WCHAR wszErrorMsg[255];
                WCHAR wszErrorTitle[255];
                LoadStringW(hInst, IDS_AFFINITY_ERROR_MESSAGE, wszErrorMsg, ARRAY_SIZE(wszErrorMsg));
                LoadStringW(hInst, IDS_AFFINITY_ERROR_TITLE, wszErrorTitle, ARRAY_SIZE(wszErrorTitle));
                MessageBoxW(hDlg, wszErrorMsg, wszErrorTitle, MB_OK|MB_ICONSTOP);
                return TRUE;
            }

            /*
             * Try to set the process affinity
             */
            if (!SetProcessAffinityMask(hProcessAffinityHandle, dwProcessAffinityMask)) {
                GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
                EndDialog(hDlg, LOWORD(wParam));
                LoadStringW(hInst, IDS_AFFINITY_UNABLE2ACCESS, wszUnable2Access, ARRAY_SIZE(wszUnable2Access));
                MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Access, MB_OK|MB_ICONSTOP);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

void ProcessPage_OnSetAffinity(void)
{
    LV_ITEMW         lvitem;
    ULONG            Index, Count;
    DWORD            dwProcessId;
    WCHAR            wstrErrorText[256];

    Count = SendMessageW(hProcessPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (Index=0; Index<Count; Index++) {
        memset(&lvitem, 0, sizeof(LV_ITEMW));
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        SendMessageW(hProcessPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &lvitem);
        if (lvitem.state & LVIS_SELECTED)
            break;
    }

    Count = SendMessageW(hProcessPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0);
    dwProcessId = PerfDataGetProcessId(Index);
    if ((Count != 1) || (dwProcessId == 0))
        return;
    hProcessAffinityHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION, FALSE, dwProcessId);
    if (!hProcessAffinityHandle) {
        GetLastErrorText(wstrErrorText, ARRAY_SIZE(wstrErrorText));
        LoadStringW(hInst, IDS_AFFINITY_UNABLE2ACCESS, wszUnable2Access, ARRAY_SIZE(wszUnable2Access));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Access, MB_OK|MB_ICONSTOP);
        return;
    }
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_AFFINITY_DIALOG), hMainWnd, AffinityDialogWndProc);
    if (hProcessAffinityHandle)    {
        CloseHandle(hProcessAffinityHandle);
        hProcessAffinityHandle = NULL;
    }
}
