/*
 *  ReactOS Task Manager
 *
 *  affinity.c
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
#include <winnt.h>
#include <stdio.h>
    
#include "wine/unicode.h"
#include "taskmgr.h"
#include "perfdata.h"

HANDLE        hProcessAffinityHandle;

static const WCHAR    wszUnable2Access[] = {'U','n','a','b','l','e',' ','t','o',' ',
                                            'A','c','c','e','s','s',' ','o','r',' ',
                                            'S','e','t',' ','P','r','o','c','e','s','s',' ',
                                            'A','f','f','i','n','i','t','y',0};

static INT_PTR CALLBACK
AffinityDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DWORD_PTR dwProcessAffinityMask = 0;
    DWORD_PTR dwSystemAffinityMask = 0;
    WCHAR     wstrErrorText[256];

    switch (message) {
    case WM_INITDIALOG:

        /*
         * Get the current affinity mask for the process and
         * the number of CPUs present in the system
         */
        if (!GetProcessAffinityMask(hProcessAffinityHandle, &dwProcessAffinityMask, &dwSystemAffinityMask))    {
            GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
            EndDialog(hDlg, 0);
            MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Access, MB_OK|MB_ICONSTOP);
        }

        /*
         * Enable a checkbox for each processor present in the system
         */
        if (dwSystemAffinityMask & 0x00000001)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU0), TRUE);
        if (dwSystemAffinityMask & 0x00000002)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU1), TRUE);
        if (dwSystemAffinityMask & 0x00000004)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU2), TRUE);
        if (dwSystemAffinityMask & 0x00000008)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU3), TRUE);
        if (dwSystemAffinityMask & 0x00000010)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU4), TRUE);
        if (dwSystemAffinityMask & 0x00000020)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU5), TRUE);
        if (dwSystemAffinityMask & 0x00000040)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU6), TRUE);
        if (dwSystemAffinityMask & 0x00000080)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU7), TRUE);
        if (dwSystemAffinityMask & 0x00000100)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU8), TRUE);
        if (dwSystemAffinityMask & 0x00000200)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU9), TRUE);
        if (dwSystemAffinityMask & 0x00000400)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU10), TRUE);
        if (dwSystemAffinityMask & 0x00000800)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU11), TRUE);
        if (dwSystemAffinityMask & 0x00001000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU12), TRUE);
        if (dwSystemAffinityMask & 0x00002000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU13), TRUE);
        if (dwSystemAffinityMask & 0x00004000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU14), TRUE);
        if (dwSystemAffinityMask & 0x00008000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU15), TRUE);
        if (dwSystemAffinityMask & 0x00010000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU16), TRUE);
        if (dwSystemAffinityMask & 0x00020000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU17), TRUE);
        if (dwSystemAffinityMask & 0x00040000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU18), TRUE);
        if (dwSystemAffinityMask & 0x00080000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU19), TRUE);
        if (dwSystemAffinityMask & 0x00100000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU20), TRUE);
        if (dwSystemAffinityMask & 0x00200000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU21), TRUE);
        if (dwSystemAffinityMask & 0x00400000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU22), TRUE);
        if (dwSystemAffinityMask & 0x00800000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU23), TRUE);
        if (dwSystemAffinityMask & 0x01000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU24), TRUE);
        if (dwSystemAffinityMask & 0x02000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU25), TRUE);
        if (dwSystemAffinityMask & 0x04000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU26), TRUE);
        if (dwSystemAffinityMask & 0x08000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU27), TRUE);
        if (dwSystemAffinityMask & 0x10000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU28), TRUE);
        if (dwSystemAffinityMask & 0x20000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU29), TRUE);
        if (dwSystemAffinityMask & 0x40000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU30), TRUE);
        if (dwSystemAffinityMask & 0x80000000)
            EnableWindow(GetDlgItem(hDlg, IDC_CPU31), TRUE);


        /*
         * Check each checkbox that the current process
         * has affinity with
         */
        if (dwProcessAffinityMask & 0x00000001)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU0), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000002)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU1), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000004)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU2), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000008)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU3), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000010)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU4), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000020)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU5), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000040)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU6), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000080)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU7), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000100)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU8), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000200)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU9), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000400)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU10), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00000800)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU11), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00001000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU12), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00002000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU13), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00004000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU14), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00008000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU15), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00010000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU16), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00020000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU17), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00040000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU18), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00080000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU19), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00100000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU20), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00200000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU21), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00400000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU22), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x00800000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU23), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x01000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU24), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x02000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU25), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x04000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU26), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x08000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU27), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x10000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU28), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x20000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU29), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x40000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU30), BM_SETCHECK, BST_CHECKED, 0);
        if (dwProcessAffinityMask & 0x80000000)
            SendMessageW(GetDlgItem(hDlg, IDC_CPU31), BM_SETCHECK, BST_CHECKED, 0);

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
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU0), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000001;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU1), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000002;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU2), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000004;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU3), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000008;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU4), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000010;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU5), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000020;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU6), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000040;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU7), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000080;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU8), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000100;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU9), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000200;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU10), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000400;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU11), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00000800;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU12), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00001000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU13), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00002000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU14), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00004000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU15), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00008000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU16), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00010000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU17), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00020000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU18), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00040000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU19), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00080000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU20), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00100000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU21), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00200000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU22), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00400000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU23), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x00800000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU24), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x01000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU25), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x02000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU26), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x04000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU27), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x08000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU28), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x10000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU29), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x20000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU30), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x40000000;
            if (SendMessageW(GetDlgItem(hDlg, IDC_CPU31), BM_GETCHECK, 0, 0))
                dwProcessAffinityMask |= 0x80000000;

            /*
             * Make sure they are giving the process affinity
             * with at least one processor. I'd hate to see a
             * process that is not in a wait state get deprived
             * of it's cpu time.
             */
            if (!dwProcessAffinityMask) {
                static const WCHAR wszErrorMsg[] = {'T','h','e',' ','p','r','o','c','e','s','s',' ',
                                                    'm','u','s','t',' ','h','a','v','e',' ',
                                                    'a','f','f','i','n','i','t','y',' ',
                                                    'w','i','t','h',' ','a','t',' ','l','e','a','s','t',' ',
                                                    'o','n','e',' ','p','r','o','c','e','s','s','o','r','.',0};
                static const WCHAR wszErrorTitle[] = {'I','n','v','a','l','i','d',' ','O','p','t','i','o','n',0};
                MessageBoxW(hDlg, wszErrorMsg, wszErrorTitle, MB_OK|MB_ICONSTOP);
                return TRUE;
            }

            /*
             * Try to set the process affinity
             */
            if (!SetProcessAffinityMask(hProcessAffinityHandle, dwProcessAffinityMask)) {
                GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
                EndDialog(hDlg, LOWORD(wParam));
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
    ULONG            Index;
    DWORD            dwProcessId;
    WCHAR            wstrErrorText[256];

    for (Index=0; Index<(ULONG)ListView_GetItemCount(hProcessPageListCtrl); Index++) {
        memset(&lvitem, 0, sizeof(LV_ITEMW));
        lvitem.mask = LVIF_STATE;
        lvitem.stateMask = LVIS_SELECTED;
        lvitem.iItem = Index;
        SendMessageW(hProcessPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &lvitem);
        if (lvitem.state & LVIS_SELECTED)
            break;
    }
    dwProcessId = PerfDataGetProcessId(Index);
    if ((ListView_GetSelectedCount(hProcessPageListCtrl) != 1) || (dwProcessId == 0))
        return;
    hProcessAffinityHandle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION, FALSE, dwProcessId);
    if (!hProcessAffinityHandle) {
        GetLastErrorText(wstrErrorText, sizeof(wstrErrorText)/sizeof(WCHAR));
        MessageBoxW(hMainWnd, wstrErrorText, wszUnable2Access, MB_OK|MB_ICONSTOP);
        return;
    }
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_AFFINITY_DIALOG), hMainWnd, AffinityDialogWndProc);
    if (hProcessAffinityHandle)    {
        CloseHandle(hProcessAffinityHandle);
        hProcessAffinityHandle = NULL;
    }
}
