/*
 *  ReactOS Task Manager
 *
 *  column.c
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

#include "wine/unicode.h"
#include "taskmgr.h"
#include "column.h"

UINT    ColumnDataHints[25];

/* Column Headers; Begin */
static WCHAR wszImageName[255];
static WCHAR wszPID[255];
static WCHAR wszUserName[255];
static WCHAR wszSessionID[255];
static WCHAR wszCPU[255];
static WCHAR wszCPUTime[255];
static WCHAR wszMemUsage[255];
static WCHAR wszPeakMemUsage[255];
static WCHAR wszMemDelta[255];
static WCHAR wszPageFaults[255];
static WCHAR wszPFDelta[255];
static WCHAR wszVMSize[255];
static WCHAR wszPagedPool[255];
static WCHAR wszNPPool[255];
static WCHAR wszBasePri[255];
static WCHAR wszHandles[255];
static WCHAR wszThreads[255];
static WCHAR wszUSERObjects[255];
static WCHAR wszGDIObjects[255];
static WCHAR wszIOReads[255];
static WCHAR wszIOWrites[255];
static WCHAR wszIOOther[255];
static WCHAR wszIOReadBytes[255];
static WCHAR wszIOWriteBytes[255];
static WCHAR wszIOOtherBytes[255];
/* Column Headers; End */

static void load_column_headers(void)
{
    LoadStringW(hInst, IDS_IMAGENAME, wszImageName, sizeof(wszImageName)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PID, wszPID, sizeof(wszPID)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_USERNAME, wszUserName, sizeof(wszUserName)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_SESSIONID, wszSessionID, sizeof(wszSessionID)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_CPUUSAGE, wszCPU, sizeof(wszCPU)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_CPUTIME, wszCPUTime, sizeof(wszCPUTime)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_MEMORYUSAGE, wszMemUsage, sizeof(wszMemUsage)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PEAKMEMORYUSAGE, wszPeakMemUsage, sizeof(wszPeakMemUsage)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_MEMORYUSAGEDELTA, wszMemDelta, sizeof(wszMemDelta)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PAGEFAULTS, wszPageFaults, sizeof(wszPageFaults)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PAGEFAULTSDELTA, wszPFDelta, sizeof(wszPFDelta)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_VIRTUALMEMORYSIZE, wszVMSize, sizeof(wszVMSize)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_PAGEDPOOL, wszPagedPool, sizeof(wszPagedPool)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_NONPAGEDPOOL, wszNPPool, sizeof(wszNPPool)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_BASEPRIORITY, wszBasePri, sizeof(wszBasePri)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_HANDLECOUNT, wszHandles, sizeof(wszHandles)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_THREADCOUNT, wszThreads, sizeof(wszThreads)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_USEROBJECTS, wszUSERObjects, sizeof(wszUSERObjects)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_GDIOBJECTS, wszGDIObjects, sizeof(wszGDIObjects)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOREADS, wszIOReads, sizeof(wszIOReads)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOWRITES, wszIOWrites, sizeof(wszIOWrites)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOOTHER, wszIOOther, sizeof(wszIOOther)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOREADBYTES, wszIOReadBytes, sizeof(wszIOReadBytes)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOWRITEBYTES, wszIOWriteBytes, sizeof(wszIOWriteBytes)/sizeof(WCHAR));
    LoadStringW(hInst, IDS_IOOTHERBYTES, wszIOOtherBytes, sizeof(wszIOOtherBytes)/sizeof(WCHAR));
}

static int InsertColumn(int nCol, LPCWSTR lpszColumnHeading, int nFormat, int nWidth, int nSubItem)
{
    LVCOLUMNW    column;

    column.mask = LVCF_TEXT|LVCF_FMT;
    column.pszText = (LPWSTR)lpszColumnHeading;
    column.fmt = nFormat;

    if (nWidth != -1)
    {
        column.mask |= LVCF_WIDTH;
        column.cx = nWidth;
    }

    if (nSubItem != -1)
    {
        column.mask |= LVCF_SUBITEM;
        column.iSubItem = nSubItem;
    }

    return ListView_InsertColumnW(hProcessPageListCtrl, nCol, &column);
}

void AddColumns(void)
{
    int        size;

    load_column_headers();

    if (TaskManagerSettings.Column_ImageName)
        InsertColumn(0, wszImageName, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[0], -1);
    if (TaskManagerSettings.Column_PID)
        InsertColumn(1, wszPID, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[1], -1);
    if (TaskManagerSettings.Column_UserName)
        InsertColumn(2, wszUserName, LVCFMT_LEFT, TaskManagerSettings.ColumnSizeArray[2], -1);
    if (TaskManagerSettings.Column_SessionID)
        InsertColumn(3, wszSessionID, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[3], -1);
    if (TaskManagerSettings.Column_CPUUsage)
        InsertColumn(4, wszCPU, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[4], -1);
    if (TaskManagerSettings.Column_CPUTime)
        InsertColumn(5, wszCPUTime, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[5], -1);
    if (TaskManagerSettings.Column_MemoryUsage)
        InsertColumn(6, wszMemUsage, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[6], -1);
    if (TaskManagerSettings.Column_PeakMemoryUsage)
        InsertColumn(7, wszPeakMemUsage, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[7], -1);
    if (TaskManagerSettings.Column_MemoryUsageDelta)
        InsertColumn(8, wszMemDelta, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[8], -1);
    if (TaskManagerSettings.Column_PageFaults)
        InsertColumn(9, wszPageFaults, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[9], -1);
    if (TaskManagerSettings.Column_PageFaultsDelta)
        InsertColumn(10, wszPFDelta, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[10], -1);
    if (TaskManagerSettings.Column_VirtualMemorySize)
        InsertColumn(11, wszVMSize, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[11], -1);
    if (TaskManagerSettings.Column_PagedPool)
        InsertColumn(12, wszPagedPool, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[12], -1);
    if (TaskManagerSettings.Column_NonPagedPool)
        InsertColumn(13, wszNPPool, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[13], -1);
    if (TaskManagerSettings.Column_BasePriority)
        InsertColumn(14, wszBasePri, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[14], -1);
    if (TaskManagerSettings.Column_HandleCount)
        InsertColumn(15, wszHandles, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[15], -1);
    if (TaskManagerSettings.Column_ThreadCount)
        InsertColumn(16, wszThreads, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[16], -1);
    if (TaskManagerSettings.Column_USERObjects)
        InsertColumn(17, wszUSERObjects, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[17], -1);
    if (TaskManagerSettings.Column_GDIObjects)
        InsertColumn(18, wszGDIObjects, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[18], -1);
    if (TaskManagerSettings.Column_IOReads)
        InsertColumn(19, wszIOReads, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[19], -1);
    if (TaskManagerSettings.Column_IOWrites)
        InsertColumn(20, wszIOWrites, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[20], -1);
    if (TaskManagerSettings.Column_IOOther)
        InsertColumn(21, wszIOOther, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[21], -1);
    if (TaskManagerSettings.Column_IOReadBytes)
        InsertColumn(22, wszIOReadBytes, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[22], -1);
    if (TaskManagerSettings.Column_IOWriteBytes)
        InsertColumn(23, wszIOWriteBytes, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[23], -1);
    if (TaskManagerSettings.Column_IOOtherBytes)
        InsertColumn(24, wszIOOtherBytes, LVCFMT_RIGHT, TaskManagerSettings.ColumnSizeArray[24], -1);

    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageListCtrl, LVM_SETCOLUMNORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    UpdateColumnDataHints();
}

static INT_PTR CALLBACK ColumnsDialogWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_INITDIALOG:

        if (TaskManagerSettings.Column_ImageName)
            SendMessageW(GetDlgItem(hDlg, IDC_IMAGENAME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PID)
            SendMessageW(GetDlgItem(hDlg, IDC_PID), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_UserName)
            SendMessageW(GetDlgItem(hDlg, IDC_USERNAME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_SessionID)
            SendMessageW(GetDlgItem(hDlg, IDC_SESSIONID), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_CPUUsage)
            SendMessageW(GetDlgItem(hDlg, IDC_CPUUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_CPUTime)
            SendMessageW(GetDlgItem(hDlg, IDC_CPUTIME), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_MemoryUsage)
            SendMessageW(GetDlgItem(hDlg, IDC_MEMORYUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PeakMemoryUsage)
            SendMessageW(GetDlgItem(hDlg, IDC_PEAKMEMORYUSAGE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_MemoryUsageDelta)
            SendMessageW(GetDlgItem(hDlg, IDC_MEMORYUSAGEDELTA), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PageFaults)
            SendMessageW(GetDlgItem(hDlg, IDC_PAGEFAULTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PageFaultsDelta)
            SendMessageW(GetDlgItem(hDlg, IDC_PAGEFAULTSDELTA), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_VirtualMemorySize)
            SendMessageW(GetDlgItem(hDlg, IDC_VIRTUALMEMORYSIZE), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_PagedPool)
            SendMessageW(GetDlgItem(hDlg, IDC_PAGEDPOOL), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_NonPagedPool)
            SendMessageW(GetDlgItem(hDlg, IDC_NONPAGEDPOOL), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_BasePriority)
            SendMessageW(GetDlgItem(hDlg, IDC_BASEPRIORITY), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_HandleCount)
            SendMessageW(GetDlgItem(hDlg, IDC_HANDLECOUNT), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_ThreadCount)
            SendMessageW(GetDlgItem(hDlg, IDC_THREADCOUNT), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_USERObjects)
            SendMessageW(GetDlgItem(hDlg, IDC_USEROBJECTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_GDIObjects)
            SendMessageW(GetDlgItem(hDlg, IDC_GDIOBJECTS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOReads)
            SendMessageW(GetDlgItem(hDlg, IDC_IOREADS), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOWrites)
            SendMessageW(GetDlgItem(hDlg, IDC_IOWRITES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOOther)
            SendMessageW(GetDlgItem(hDlg, IDC_IOOTHER), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOReadBytes)
            SendMessageW(GetDlgItem(hDlg, IDC_IOREADBYTES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOWriteBytes)
            SendMessageW(GetDlgItem(hDlg, IDC_IOWRITEBYTES), BM_SETCHECK, BST_CHECKED, 0);
        if (TaskManagerSettings.Column_IOOtherBytes)
            SendMessageW(GetDlgItem(hDlg, IDC_IOOTHERBYTES), BM_SETCHECK, BST_CHECKED, 0);

        return TRUE;

    case WM_COMMAND:

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            TaskManagerSettings.Column_ImageName = SendMessageW(GetDlgItem(hDlg, IDC_IMAGENAME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PID = SendMessageW(GetDlgItem(hDlg, IDC_PID), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_UserName = SendMessageW(GetDlgItem(hDlg, IDC_USERNAME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_SessionID = SendMessageW(GetDlgItem(hDlg, IDC_SESSIONID), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_CPUUsage = SendMessageW(GetDlgItem(hDlg, IDC_CPUUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_CPUTime = SendMessageW(GetDlgItem(hDlg, IDC_CPUTIME), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_MemoryUsage = SendMessageW(GetDlgItem(hDlg, IDC_MEMORYUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PeakMemoryUsage = SendMessageW(GetDlgItem(hDlg, IDC_PEAKMEMORYUSAGE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_MemoryUsageDelta = SendMessageW(GetDlgItem(hDlg, IDC_MEMORYUSAGEDELTA), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PageFaults = SendMessageW(GetDlgItem(hDlg, IDC_PAGEFAULTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PageFaultsDelta = SendMessageW(GetDlgItem(hDlg, IDC_PAGEFAULTSDELTA), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_VirtualMemorySize = SendMessageW(GetDlgItem(hDlg, IDC_VIRTUALMEMORYSIZE), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_PagedPool = SendMessageW(GetDlgItem(hDlg, IDC_PAGEDPOOL), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_NonPagedPool = SendMessageW(GetDlgItem(hDlg, IDC_NONPAGEDPOOL), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_BasePriority = SendMessageW(GetDlgItem(hDlg, IDC_BASEPRIORITY), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_HandleCount = SendMessageW(GetDlgItem(hDlg, IDC_HANDLECOUNT), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_ThreadCount = SendMessageW(GetDlgItem(hDlg, IDC_THREADCOUNT), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_USERObjects = SendMessageW(GetDlgItem(hDlg, IDC_USEROBJECTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_GDIObjects = SendMessageW(GetDlgItem(hDlg, IDC_GDIOBJECTS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOReads = SendMessageW(GetDlgItem(hDlg, IDC_IOREADS), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOWrites = SendMessageW(GetDlgItem(hDlg, IDC_IOWRITES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOOther = SendMessageW(GetDlgItem(hDlg, IDC_IOOTHER), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOReadBytes = SendMessageW(GetDlgItem(hDlg, IDC_IOREADBYTES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOWriteBytes = SendMessageW(GetDlgItem(hDlg, IDC_IOWRITEBYTES), BM_GETCHECK, 0, 0);
            TaskManagerSettings.Column_IOOtherBytes = SendMessageW(GetDlgItem(hDlg, IDC_IOOTHERBYTES), BM_GETCHECK, 0, 0);

            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        break;
    }

    return 0;
}

void SaveColumnSettings(void)
{
    HDITEMW    hditem;
    int        i;
    WCHAR      text[256];
    int        size;

    /* Reset column data */
    for (i=0; i<25; i++)
        TaskManagerSettings.ColumnOrderArray[i] = i;

    TaskManagerSettings.Column_ImageName = FALSE;
    TaskManagerSettings.Column_PID = FALSE;
    TaskManagerSettings.Column_CPUUsage = FALSE;
    TaskManagerSettings.Column_CPUTime = FALSE;
    TaskManagerSettings.Column_MemoryUsage = FALSE;
    TaskManagerSettings.Column_MemoryUsageDelta = FALSE;
    TaskManagerSettings.Column_PeakMemoryUsage = FALSE;
    TaskManagerSettings.Column_PageFaults = FALSE;
    TaskManagerSettings.Column_USERObjects = FALSE;
    TaskManagerSettings.Column_IOReads = FALSE;
    TaskManagerSettings.Column_IOReadBytes = FALSE;
    TaskManagerSettings.Column_SessionID = FALSE;
    TaskManagerSettings.Column_UserName = FALSE;
    TaskManagerSettings.Column_PageFaultsDelta = FALSE;
    TaskManagerSettings.Column_VirtualMemorySize = FALSE;
    TaskManagerSettings.Column_PagedPool = FALSE;
    TaskManagerSettings.Column_NonPagedPool = FALSE;
    TaskManagerSettings.Column_BasePriority = FALSE;
    TaskManagerSettings.Column_HandleCount = FALSE;
    TaskManagerSettings.Column_ThreadCount = FALSE;
    TaskManagerSettings.Column_GDIObjects = FALSE;
    TaskManagerSettings.Column_IOWrites = FALSE;
    TaskManagerSettings.Column_IOWriteBytes = FALSE;
    TaskManagerSettings.Column_IOOther = FALSE;
    TaskManagerSettings.Column_IOOtherBytes = FALSE;
    TaskManagerSettings.ColumnSizeArray[0] = 105;
    TaskManagerSettings.ColumnSizeArray[1] = 50;
    TaskManagerSettings.ColumnSizeArray[2] = 107;
    TaskManagerSettings.ColumnSizeArray[3] = 70;
    TaskManagerSettings.ColumnSizeArray[4] = 35;
    TaskManagerSettings.ColumnSizeArray[5] = 70;
    TaskManagerSettings.ColumnSizeArray[6] = 70;
    TaskManagerSettings.ColumnSizeArray[7] = 100;
    TaskManagerSettings.ColumnSizeArray[8] = 70;
    TaskManagerSettings.ColumnSizeArray[9] = 70;
    TaskManagerSettings.ColumnSizeArray[10] = 70;
    TaskManagerSettings.ColumnSizeArray[11] = 70;
    TaskManagerSettings.ColumnSizeArray[12] = 70;
    TaskManagerSettings.ColumnSizeArray[13] = 70;
    TaskManagerSettings.ColumnSizeArray[14] = 60;
    TaskManagerSettings.ColumnSizeArray[15] = 60;
    TaskManagerSettings.ColumnSizeArray[16] = 60;
    TaskManagerSettings.ColumnSizeArray[17] = 60;
    TaskManagerSettings.ColumnSizeArray[18] = 60;
    TaskManagerSettings.ColumnSizeArray[19] = 70;
    TaskManagerSettings.ColumnSizeArray[20] = 70;
    TaskManagerSettings.ColumnSizeArray[21] = 70;
    TaskManagerSettings.ColumnSizeArray[22] = 70;
    TaskManagerSettings.ColumnSizeArray[23] = 70;
    TaskManagerSettings.ColumnSizeArray[24] = 70;

    /* Get header order */
    size = SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0);
    SendMessageW(hProcessPageHeaderCtrl, HDM_GETORDERARRAY, (WPARAM) size, (LPARAM) &TaskManagerSettings.ColumnOrderArray);

    /* Get visible columns */
    for (i=0; i<SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); i++) {
        memset(&hditem, 0, sizeof(HDITEMW));

        hditem.mask = HDI_TEXT|HDI_WIDTH;
        hditem.pszText = text;
        hditem.cchTextMax = 256;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMW, i, (LPARAM) &hditem);

        if (strcmpW(text, wszImageName) == 0)
        {
            TaskManagerSettings.Column_ImageName = TRUE;
            TaskManagerSettings.ColumnSizeArray[0] = hditem.cxy;
        }
        if (strcmpW(text, wszPID) == 0)
        {
            TaskManagerSettings.Column_PID = TRUE;
            TaskManagerSettings.ColumnSizeArray[1] = hditem.cxy;
        }
        if (strcmpW(text, wszUserName) == 0)
        {
            TaskManagerSettings.Column_UserName = TRUE;
            TaskManagerSettings.ColumnSizeArray[2] = hditem.cxy;
        }
        if (strcmpW(text, wszSessionID) == 0)
        {
            TaskManagerSettings.Column_SessionID = TRUE;
            TaskManagerSettings.ColumnSizeArray[3] = hditem.cxy;
        }
        if (strcmpW(text, wszCPU) == 0)
        {
            TaskManagerSettings.Column_CPUUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[4] = hditem.cxy;
        }
        if (strcmpW(text, wszCPUTime) == 0)
        {
            TaskManagerSettings.Column_CPUTime = TRUE;
            TaskManagerSettings.ColumnSizeArray[5] = hditem.cxy;
        }
        if (strcmpW(text, wszMemUsage) == 0)
        {
            TaskManagerSettings.Column_MemoryUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[6] = hditem.cxy;
        }
        if (strcmpW(text, wszPeakMemUsage) == 0)
        {
            TaskManagerSettings.Column_PeakMemoryUsage = TRUE;
            TaskManagerSettings.ColumnSizeArray[7] = hditem.cxy;
        }
        if (strcmpW(text, wszMemDelta) == 0)
        {
            TaskManagerSettings.Column_MemoryUsageDelta = TRUE;
            TaskManagerSettings.ColumnSizeArray[8] = hditem.cxy;
        }
        if (strcmpW(text, wszPageFaults) == 0)
        {
            TaskManagerSettings.Column_PageFaults = TRUE;
            TaskManagerSettings.ColumnSizeArray[9] = hditem.cxy;
        }
        if (strcmpW(text, wszPFDelta) == 0)
        {
            TaskManagerSettings.Column_PageFaultsDelta = TRUE;
            TaskManagerSettings.ColumnSizeArray[10] = hditem.cxy;
        }
        if (strcmpW(text, wszVMSize) == 0)
        {
            TaskManagerSettings.Column_VirtualMemorySize = TRUE;
            TaskManagerSettings.ColumnSizeArray[11] = hditem.cxy;
        }
        if (strcmpW(text, wszPagedPool) == 0)
        {
            TaskManagerSettings.Column_PagedPool = TRUE;
            TaskManagerSettings.ColumnSizeArray[12] = hditem.cxy;
        }
        if (strcmpW(text, wszNPPool) == 0)
        {
            TaskManagerSettings.Column_NonPagedPool = TRUE;
            TaskManagerSettings.ColumnSizeArray[13] = hditem.cxy;
        }
        if (strcmpW(text, wszBasePri) == 0)
        {
            TaskManagerSettings.Column_BasePriority = TRUE;
            TaskManagerSettings.ColumnSizeArray[14] = hditem.cxy;
        }
        if (strcmpW(text, wszHandles) == 0)
        {
            TaskManagerSettings.Column_HandleCount = TRUE;
            TaskManagerSettings.ColumnSizeArray[15] = hditem.cxy;
        }
        if (strcmpW(text, wszThreads) == 0)
        {
            TaskManagerSettings.Column_ThreadCount = TRUE;
            TaskManagerSettings.ColumnSizeArray[16] = hditem.cxy;
        }
        if (strcmpW(text, wszUSERObjects) == 0)
        {
            TaskManagerSettings.Column_USERObjects = TRUE;
            TaskManagerSettings.ColumnSizeArray[17] = hditem.cxy;
        }
        if (strcmpW(text, wszGDIObjects) == 0)
        {
            TaskManagerSettings.Column_GDIObjects = TRUE;
            TaskManagerSettings.ColumnSizeArray[18] = hditem.cxy;
        }
        if (strcmpW(text, wszIOReads) == 0)
        {
            TaskManagerSettings.Column_IOReads = TRUE;
            TaskManagerSettings.ColumnSizeArray[19] = hditem.cxy;
        }
        if (strcmpW(text, wszIOWrites) == 0)
        {
            TaskManagerSettings.Column_IOWrites = TRUE;
            TaskManagerSettings.ColumnSizeArray[20] = hditem.cxy;
        }
        if (strcmpW(text, wszIOOther) == 0)
        {
            TaskManagerSettings.Column_IOOther = TRUE;
            TaskManagerSettings.ColumnSizeArray[21] = hditem.cxy;
        }
        if (strcmpW(text, wszIOReadBytes) == 0)
        {
            TaskManagerSettings.Column_IOReadBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[22] = hditem.cxy;
        }
        if (strcmpW(text, wszIOWriteBytes) == 0)
        {
            TaskManagerSettings.Column_IOWriteBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[23] = hditem.cxy;
        }
        if (strcmpW(text, wszIOOtherBytes) == 0)
        {
            TaskManagerSettings.Column_IOOtherBytes = TRUE;
            TaskManagerSettings.ColumnSizeArray[24] = hditem.cxy;
        }
    }
}

void ProcessPage_OnViewSelectColumns(void)
{
    int        i;

    if (DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_COLUMNS_DIALOG), hMainWnd, ColumnsDialogWndProc) == IDOK)
    {
        for (i=SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0)-1; i>=0; i--)
        {
            SendMessageW(hProcessPageListCtrl, LVM_DELETECOLUMN, 0, i);
        }

        for (i=0; i<25; i++)
            TaskManagerSettings.ColumnOrderArray[i] = i;

        TaskManagerSettings.ColumnSizeArray[0] = 105;
        TaskManagerSettings.ColumnSizeArray[1] = 50;
        TaskManagerSettings.ColumnSizeArray[2] = 107;
        TaskManagerSettings.ColumnSizeArray[3] = 70;
        TaskManagerSettings.ColumnSizeArray[4] = 35;
        TaskManagerSettings.ColumnSizeArray[5] = 70;
        TaskManagerSettings.ColumnSizeArray[6] = 70;
        TaskManagerSettings.ColumnSizeArray[7] = 100;
        TaskManagerSettings.ColumnSizeArray[8] = 70;
        TaskManagerSettings.ColumnSizeArray[9] = 70;
        TaskManagerSettings.ColumnSizeArray[10] = 70;
        TaskManagerSettings.ColumnSizeArray[11] = 70;
        TaskManagerSettings.ColumnSizeArray[12] = 70;
        TaskManagerSettings.ColumnSizeArray[13] = 70;
        TaskManagerSettings.ColumnSizeArray[14] = 60;
        TaskManagerSettings.ColumnSizeArray[15] = 60;
        TaskManagerSettings.ColumnSizeArray[16] = 60;
        TaskManagerSettings.ColumnSizeArray[17] = 60;
        TaskManagerSettings.ColumnSizeArray[18] = 60;
        TaskManagerSettings.ColumnSizeArray[19] = 70;
        TaskManagerSettings.ColumnSizeArray[20] = 70;
        TaskManagerSettings.ColumnSizeArray[21] = 70;
        TaskManagerSettings.ColumnSizeArray[22] = 70;
        TaskManagerSettings.ColumnSizeArray[23] = 70;
        TaskManagerSettings.ColumnSizeArray[24] = 70;

        AddColumns();
    }
}

void UpdateColumnDataHints(void)
{
    HDITEMW            hditem;
    WCHAR            text[256];
    ULONG            Index;

    for (Index=0; Index<(ULONG)SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMCOUNT, 0, 0); Index++)
    {
        memset(&hditem, 0, sizeof(HDITEMW));

        hditem.mask = HDI_TEXT;
        hditem.pszText = text;
        hditem.cchTextMax = 260;

        SendMessageW(hProcessPageHeaderCtrl, HDM_GETITEMW, Index, (LPARAM) &hditem);

        if (strcmpW(text, wszImageName) == 0)
            ColumnDataHints[Index] = COLUMN_IMAGENAME;
        if (strcmpW(text, wszPID) == 0)
            ColumnDataHints[Index] = COLUMN_PID;
        if (strcmpW(text, wszUserName) == 0)
            ColumnDataHints[Index] = COLUMN_USERNAME;
        if (strcmpW(text, wszSessionID) == 0)
            ColumnDataHints[Index] = COLUMN_SESSIONID;
        if (strcmpW(text, wszCPU) == 0)
            ColumnDataHints[Index] = COLUMN_CPUUSAGE;
        if (strcmpW(text, wszCPUTime) == 0)
            ColumnDataHints[Index] = COLUMN_CPUTIME;
        if (strcmpW(text, wszMemUsage) == 0)
            ColumnDataHints[Index] = COLUMN_MEMORYUSAGE;
        if (strcmpW(text, wszPeakMemUsage) == 0)
            ColumnDataHints[Index] = COLUMN_PEAKMEMORYUSAGE;
        if (strcmpW(text, wszMemDelta) == 0)
            ColumnDataHints[Index] = COLUMN_MEMORYUSAGEDELTA;
        if (strcmpW(text, wszPageFaults) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEFAULTS;
        if (strcmpW(text, wszPFDelta) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEFAULTSDELTA;
        if (strcmpW(text, wszVMSize) == 0)
            ColumnDataHints[Index] = COLUMN_VIRTUALMEMORYSIZE;
        if (strcmpW(text, wszPagedPool) == 0)
            ColumnDataHints[Index] = COLUMN_PAGEDPOOL;
        if (strcmpW(text, wszNPPool) == 0)
            ColumnDataHints[Index] = COLUMN_NONPAGEDPOOL;
        if (strcmpW(text, wszBasePri) == 0)
            ColumnDataHints[Index] = COLUMN_BASEPRIORITY;
        if (strcmpW(text, wszHandles) == 0)
            ColumnDataHints[Index] = COLUMN_HANDLECOUNT;
        if (strcmpW(text, wszThreads) == 0)
            ColumnDataHints[Index] = COLUMN_THREADCOUNT;
        if (strcmpW(text, wszUSERObjects) == 0)
            ColumnDataHints[Index] = COLUMN_USEROBJECTS;
        if (strcmpW(text, wszGDIObjects) == 0)
            ColumnDataHints[Index] = COLUMN_GDIOBJECTS;
        if (strcmpW(text, wszIOReads) == 0)
            ColumnDataHints[Index] = COLUMN_IOREADS;
        if (strcmpW(text, wszIOWrites) == 0)
            ColumnDataHints[Index] = COLUMN_IOWRITES;
        if (strcmpW(text, wszIOOther) == 0)
            ColumnDataHints[Index] = COLUMN_IOOTHER;
        if (strcmpW(text, wszIOReadBytes) == 0)
            ColumnDataHints[Index] = COLUMN_IOREADBYTES;
        if (strcmpW(text, wszIOWriteBytes) == 0)
            ColumnDataHints[Index] = COLUMN_IOWRITEBYTES;
        if (strcmpW(text, wszIOOtherBytes) == 0)
            ColumnDataHints[Index] = COLUMN_IOOTHERBYTES;
    }
}
