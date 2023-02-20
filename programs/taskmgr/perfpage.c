/*
 *  ReactOS Task Manager
 *
 *  perfpage.c
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
#include "graphctl.h"

TGraphCtrl PerformancePageCpuUsageHistoryGraph;
TGraphCtrl PerformancePageMemUsageHistoryGraph;

HWND		hPerformancePage;					/*  Performance Property Page */
HWND		hPerformancePageCpuUsageGraph;				/*  CPU Usage Graph */
HWND		hPerformancePageMemUsageGraph;				/*  MEM Usage Graph */
HWND		hPerformancePageCpuUsageHistoryGraph;			/*  CPU Usage History Graph */
HWND		hPerformancePageMemUsageHistoryGraph;			/*  Memory Usage History Graph */
HWND		hPerformancePageTotalsFrame;				/*  Totals Frame */
HWND		hPerformancePageCommitChargeFrame;			/*  Commit Charge Frame */
HWND		hPerformancePageKernelMemoryFrame;			/*  Kernel Memory Frame */
HWND		hPerformancePagePhysicalMemoryFrame;			/*  Physical Memory Frame */
HWND		hPerformancePageCpuUsageFrame;
HWND		hPerformancePageMemUsageFrame;
HWND		hPerformancePageCpuUsageHistoryFrame;
HWND		hPerformancePageMemUsageHistoryFrame;
HWND		hPerformancePageCommitChargeTotalEdit;			/*  Commit Charge Total Edit Control */
HWND		hPerformancePageCommitChargeLimitEdit;			/*  Commit Charge Limit Edit Control */
HWND		hPerformancePageCommitChargePeakEdit;			/*  Commit Charge Peak Edit Control */
HWND		hPerformancePageKernelMemoryTotalEdit;			/*  Kernel Memory Total Edit Control */
HWND		hPerformancePageKernelMemoryPagedEdit;			/*  Kernel Memory Paged Edit Control */
HWND		hPerformancePageKernelMemoryNonPagedEdit;		/*  Kernel Memory NonPaged Edit Control */
HWND		hPerformancePagePhysicalMemoryTotalEdit;		/*  Physical Memory Total Edit Control */
HWND		hPerformancePagePhysicalMemoryAvailableEdit;	/*  Physical Memory Available Edit Control */
HWND		hPerformancePagePhysicalMemorySystemCacheEdit;	/*  Physical Memory System Cache Edit Control */
HWND		hPerformancePageTotalsHandleCountEdit;			/*  Total Handles Edit Control */
HWND		hPerformancePageTotalsProcessCountEdit;			/*  Total Processes Edit Control */
HWND		hPerformancePageTotalsThreadCountEdit;			/*  Total Threads Edit Control */


static int	nPerformancePageWidth;
static int	nPerformancePageHeight;
static HANDLE	hPerformancePageEvent = NULL;	/*  When this event becomes signaled then we refresh the performance page */

static void AdjustFrameSize(HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference, int pos)
{
    RECT	rc;
    int		cx, cy, sx, sy;

    GetClientRect(hCntrl, &rc);
    MapWindowPoints(hCntrl, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)));
    if (pos) {
        cx = rc.left;
        cy = rc.top;
        sx = rc.right - rc.left;
        switch (pos) {
        case 1:
            break;
        case 2:
            cy += nYDifference / 2;
            break;
        case 3:
            sx += nXDifference;
            break;
        case 4:
            cy += nYDifference / 2;
            sx += nXDifference;
            break;
        }
        sy = rc.bottom - rc.top + nYDifference / 2;
        SetWindowPos(hCntrl, NULL, cx, cy, sx, sy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOZORDER);
    } else {
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hCntrl, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
    }
    InvalidateRect(hCntrl, NULL, TRUE);
}
		 
static void AdjustControlPosition(HWND hCntrl, HWND hDlg, int nXDifference, int nYDifference)
{
    AdjustFrameSize(hCntrl, hDlg, nXDifference, nYDifference, 0);
}
		 
static void AdjustCntrlPos(int ctrl_id, HWND hDlg, int nXDifference, int nYDifference)
{
    AdjustFrameSize(GetDlgItem(hDlg, ctrl_id), hDlg, nXDifference, nYDifference, 0);
}
void RefreshPerformancePage(void)
{
	/*  Signal the event so that our refresh thread */
	/*  will wake up and refresh the performance page */
	SetEvent(hPerformancePageEvent);
}

static DWORD WINAPI PerformancePageRefreshThread(void *lpParameter)
{
	ULONG	CommitChargeTotal;
	ULONG	CommitChargeLimit;
	ULONG	CommitChargePeak;

	ULONG	KernelMemoryTotal;
	ULONG	KernelMemoryPaged;
	ULONG	KernelMemoryNonPaged;

	ULONG	PhysicalMemoryTotal;
	ULONG	PhysicalMemoryAvailable;
	ULONG	PhysicalMemorySystemCache;

	ULONG	TotalHandles;
	ULONG	TotalThreads;
	ULONG	TotalProcesses;

	WCHAR	Text[256];

	static const WCHAR    wszFormatDigit[] = {'%','u',0};
	WCHAR    wszMemUsage[255];

        LoadStringW(hInst, IDS_STATUS_BAR_MEMORY_USAGE, wszMemUsage, ARRAY_SIZE(wszMemUsage));

	/*  Create the event */
	hPerformancePageEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

	/*  If we couldn't create the event then exit the thread */
	if (!hPerformancePageEvent)
		return 0;

	while (1)
	{
		DWORD	dwWaitVal;

		/*  Wait on the event */
		dwWaitVal = WaitForSingleObject(hPerformancePageEvent, INFINITE);

		/*  If the wait failed then the event object must have been */
		/*  closed and the task manager is exiting so exit this thread */
		if (dwWaitVal == WAIT_FAILED)
			return 0;

		if (dwWaitVal == WAIT_OBJECT_0)
		{
			ULONG CpuUsage;
			ULONG CpuKernelUsage;
			int nBarsUsed1, nBarsUsed2;
			DWORD_PTR args[2];

			/*  Reset our event */
			ResetEvent(hPerformancePageEvent);

			/* 
			 *  Update the commit charge info
			 */ 
			CommitChargeTotal = PerfDataGetCommitChargeTotalK();
			CommitChargeLimit = PerfDataGetCommitChargeLimitK();
			CommitChargePeak = PerfDataGetCommitChargePeakK();
			wsprintfW(Text, wszFormatDigit, CommitChargeTotal);
			SetWindowTextW(hPerformancePageCommitChargeTotalEdit, Text);
			wsprintfW(Text, wszFormatDigit, CommitChargeLimit);
			SetWindowTextW(hPerformancePageCommitChargeLimitEdit, Text);
			wsprintfW(Text, wszFormatDigit, CommitChargePeak);

			SetWindowTextW(hPerformancePageCommitChargePeakEdit, Text);

			args[0] = CommitChargeTotal;
			args[1] = CommitChargeLimit;
			FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
			               wszMemUsage, 0, 0, Text, ARRAY_SIZE(Text), (va_list *)args);
			SendMessageW(hStatusWnd, SB_SETTEXTW, 2, (LPARAM)Text);

			/* 
			 *  Update the kernel memory info
			 */ 
			KernelMemoryTotal = PerfDataGetKernelMemoryTotalK();
			KernelMemoryPaged = PerfDataGetKernelMemoryPagedK();
			KernelMemoryNonPaged = PerfDataGetKernelMemoryNonPagedK();
			wsprintfW(Text, wszFormatDigit, KernelMemoryTotal);
			SetWindowTextW(hPerformancePageKernelMemoryTotalEdit, Text);
			wsprintfW(Text, wszFormatDigit, KernelMemoryPaged);
			SetWindowTextW(hPerformancePageKernelMemoryPagedEdit, Text);
			wsprintfW(Text, wszFormatDigit, KernelMemoryNonPaged);
			SetWindowTextW(hPerformancePageKernelMemoryNonPagedEdit, Text);

			/* 
			 *  Update the physical memory info
			 */ 
			PhysicalMemoryTotal = PerfDataGetPhysicalMemoryTotalK();
			PhysicalMemoryAvailable = PerfDataGetPhysicalMemoryAvailableK();
			PhysicalMemorySystemCache = PerfDataGetPhysicalMemorySystemCacheK();
			wsprintfW(Text, wszFormatDigit, PhysicalMemoryTotal);
			SetWindowTextW(hPerformancePagePhysicalMemoryTotalEdit, Text);
			wsprintfW(Text, wszFormatDigit, PhysicalMemoryAvailable);
			SetWindowTextW(hPerformancePagePhysicalMemoryAvailableEdit, Text);
			wsprintfW(Text, wszFormatDigit, PhysicalMemorySystemCache);
			SetWindowTextW(hPerformancePagePhysicalMemorySystemCacheEdit, Text);

			/* 
			 *  Update the totals info
			 */ 
			TotalHandles = PerfDataGetSystemHandleCount();
			TotalThreads = PerfDataGetTotalThreadCount();
			TotalProcesses = PerfDataGetProcessCount();
			wsprintfW(Text, wszFormatDigit, TotalHandles);
			SetWindowTextW(hPerformancePageTotalsHandleCountEdit, Text);
			wsprintfW(Text, wszFormatDigit, TotalThreads);
			SetWindowTextW(hPerformancePageTotalsThreadCountEdit, Text);
			wsprintfW(Text, wszFormatDigit, TotalProcesses);
			SetWindowTextW(hPerformancePageTotalsProcessCountEdit, Text);

			/* 
			 *  Redraw the graphs
			 */ 
			InvalidateRect(hPerformancePageCpuUsageGraph, NULL, FALSE);
			InvalidateRect(hPerformancePageMemUsageGraph, NULL, FALSE);

                        /*
                         *  Get the CPU usage
                         */
                        CpuUsage = PerfDataGetProcessorUsage();
                        CpuKernelUsage = PerfDataGetProcessorSystemUsage();

                        /*
                         *  Get the memory usage
                         */
                        CommitChargeTotal = (ULONGLONG)PerfDataGetCommitChargeTotalK();
                        CommitChargeLimit = (ULONGLONG)PerfDataGetCommitChargeLimitK();
                        nBarsUsed1 = CommitChargeLimit ? ((CommitChargeTotal * 100) / CommitChargeLimit) : 0;

                        PhysicalMemoryTotal = PerfDataGetPhysicalMemoryTotalK();
                        PhysicalMemoryAvailable = PerfDataGetPhysicalMemoryAvailableK();
                        nBarsUsed2 = PhysicalMemoryTotal ? ((PhysicalMemoryAvailable * 100) / PhysicalMemoryTotal) : 0;


                        GraphCtrl_AppendPoint(&PerformancePageCpuUsageHistoryGraph, CpuUsage, CpuKernelUsage, 0.0, 0.0);
                        GraphCtrl_AppendPoint(&PerformancePageMemUsageHistoryGraph, nBarsUsed1, nBarsUsed2, 0.0, 0.0);
                        /* PerformancePageMemUsageHistoryGraph.SetRange(0.0, 100.0, 10) ; */
                        InvalidateRect(hPerformancePageMemUsageHistoryGraph, NULL, FALSE);
                        InvalidateRect(hPerformancePageCpuUsageHistoryGraph, NULL, FALSE);
                }
	}
        return 0;
}

INT_PTR CALLBACK
PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT	rc;
	int		nXDifference;
	int		nYDifference;

    switch (message) {
	case WM_INITDIALOG:
		
		/*  Save the width and height */
		GetClientRect(hDlg, &rc);
		nPerformancePageWidth = rc.right;
		nPerformancePageHeight = rc.bottom;

		/*  Update window position */
		SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

		/* 
		 *  Get handles to all the controls
		 */ 
		hPerformancePageTotalsFrame = GetDlgItem(hDlg, IDC_TOTALS_FRAME);
		hPerformancePageCommitChargeFrame = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_FRAME);
		hPerformancePageKernelMemoryFrame = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_FRAME);
		hPerformancePagePhysicalMemoryFrame = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_FRAME);

		hPerformancePageCpuUsageFrame = GetDlgItem(hDlg, IDC_CPU_USAGE_FRAME);
		hPerformancePageMemUsageFrame = GetDlgItem(hDlg, IDC_MEM_USAGE_FRAME);
		hPerformancePageCpuUsageHistoryFrame = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_FRAME);
		hPerformancePageMemUsageHistoryFrame = GetDlgItem(hDlg, IDC_MEMORY_USAGE_HISTORY_FRAME);

		hPerformancePageCommitChargeTotalEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_TOTAL);
		hPerformancePageCommitChargeLimitEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_LIMIT);
		hPerformancePageCommitChargePeakEdit = GetDlgItem(hDlg, IDC_COMMIT_CHARGE_PEAK);
		hPerformancePageKernelMemoryTotalEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_TOTAL);
		hPerformancePageKernelMemoryPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_PAGED);
		hPerformancePageKernelMemoryNonPagedEdit = GetDlgItem(hDlg, IDC_KERNEL_MEMORY_NONPAGED);
		hPerformancePagePhysicalMemoryTotalEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_TOTAL);
		hPerformancePagePhysicalMemoryAvailableEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_AVAILABLE);
		hPerformancePagePhysicalMemorySystemCacheEdit = GetDlgItem(hDlg, IDC_PHYSICAL_MEMORY_SYSTEM_CACHE);
		hPerformancePageTotalsHandleCountEdit = GetDlgItem(hDlg, IDC_TOTALS_HANDLE_COUNT);
		hPerformancePageTotalsProcessCountEdit = GetDlgItem(hDlg, IDC_TOTALS_PROCESS_COUNT);
		hPerformancePageTotalsThreadCountEdit = GetDlgItem(hDlg, IDC_TOTALS_THREAD_COUNT);

        hPerformancePageCpuUsageGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_GRAPH);
		hPerformancePageMemUsageGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_GRAPH);
		hPerformancePageMemUsageHistoryGraph = GetDlgItem(hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);
        hPerformancePageCpuUsageHistoryGraph = GetDlgItem(hDlg, IDC_CPU_USAGE_HISTORY_GRAPH);
		
		GetClientRect(hPerformancePageCpuUsageHistoryGraph, &rc);
        /*  create the control */
        /* PerformancePageCpuUsageHistoryGraph.Create(0, rc, hDlg, IDC_CPU_USAGE_HISTORY_GRAPH); */
        GraphCtrl_Create(&PerformancePageCpuUsageHistoryGraph, hPerformancePageCpuUsageHistoryGraph, hDlg, IDC_CPU_USAGE_HISTORY_GRAPH);
        /*  customize the control */
        GraphCtrl_SetRange(&PerformancePageCpuUsageHistoryGraph, 0.0, 100.0, 10);
        GraphCtrl_SetBackgroundColor(&PerformancePageCpuUsageHistoryGraph, RGB(0, 0, 0)) ;
        GraphCtrl_SetGridColor(&PerformancePageCpuUsageHistoryGraph, RGB(152, 205, 152)) ;
        GraphCtrl_SetPlotColor(&PerformancePageCpuUsageHistoryGraph, 0, RGB(255, 0, 0)) ;
        GraphCtrl_SetPlotColor(&PerformancePageCpuUsageHistoryGraph, 1, RGB(0, 255, 0)) ;

        GetClientRect(hPerformancePageMemUsageHistoryGraph, &rc);
        GraphCtrl_Create(&PerformancePageMemUsageHistoryGraph, hPerformancePageMemUsageHistoryGraph, hDlg, IDC_MEM_USAGE_HISTORY_GRAPH);
        GraphCtrl_SetRange(&PerformancePageMemUsageHistoryGraph, 0.0, 100.0, 10) ;
        GraphCtrl_SetBackgroundColor(&PerformancePageMemUsageHistoryGraph, RGB(0, 0, 0)) ;
        GraphCtrl_SetGridColor(&PerformancePageMemUsageHistoryGraph, RGB(152, 215, 152)) ;
        GraphCtrl_SetPlotColor(&PerformancePageMemUsageHistoryGraph, 0, RGB(255, 255, 0)) ;
		/*  Start our refresh thread */
#ifdef RUN_PERF_PAGE
        CloseHandle( CreateThread(NULL, 0, PerformancePageRefreshThread, NULL, 0, NULL));
#endif

		/* 
		 *  Subclass graph buttons
		 */ 
        OldGraphWndProc = (WNDPROC)SetWindowLongPtrW(hPerformancePageCpuUsageGraph, GWLP_WNDPROC, (LONG_PTR)Graph_WndProc);
        SetWindowLongPtrW(hPerformancePageMemUsageGraph, GWLP_WNDPROC, (LONG_PTR)Graph_WndProc);
        OldGraphCtrlWndProc = (WNDPROC)SetWindowLongPtrW(hPerformancePageMemUsageHistoryGraph, GWLP_WNDPROC, (LONG_PTR)GraphCtrl_WndProc);
        SetWindowLongPtrW(hPerformancePageCpuUsageHistoryGraph, GWLP_WNDPROC, (LONG_PTR)GraphCtrl_WndProc);
        return TRUE;

	case WM_COMMAND:
		break;

	case WM_SIZE:
        do {
		int		cx, cy;

 		if (wParam == SIZE_MINIMIZED)
			return 0;

		cx = LOWORD(lParam);
		cy = HIWORD(lParam);
		nXDifference = cx - nPerformancePageWidth;
		nYDifference = cy - nPerformancePageHeight;
		nPerformancePageWidth = cx;
		nPerformancePageHeight = cy;
        } while (0);

		/*  Reposition the performance page's controls */
        AdjustFrameSize(hPerformancePageTotalsFrame, hDlg, 0, nYDifference, 0);
        AdjustFrameSize(hPerformancePageCommitChargeFrame, hDlg, 0, nYDifference, 0);
        AdjustFrameSize(hPerformancePageKernelMemoryFrame, hDlg, 0, nYDifference, 0);
        AdjustFrameSize(hPerformancePagePhysicalMemoryFrame, hDlg, 0, nYDifference, 0);
        AdjustCntrlPos(IDS_COMMIT_CHARGE_TOTAL, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_COMMIT_CHARGE_LIMIT, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_COMMIT_CHARGE_PEAK, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_KERNEL_MEMORY_TOTAL, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_KERNEL_MEMORY_PAGED, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_KERNEL_MEMORY_NONPAGED, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_PHYSICAL_MEMORY_TOTAL, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_PHYSICAL_MEMORY_AVAILABLE, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_PHYSICAL_MEMORY_SYSTEM_CACHE, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_TOTALS_HANDLE_COUNT, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_TOTALS_PROCESS_COUNT, hDlg, 0, nYDifference);
        AdjustCntrlPos(IDS_TOTALS_THREAD_COUNT, hDlg, 0, nYDifference);

        AdjustControlPosition(hPerformancePageCommitChargeTotalEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageCommitChargeLimitEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageCommitChargePeakEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageKernelMemoryTotalEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageKernelMemoryPagedEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageKernelMemoryNonPagedEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePagePhysicalMemoryTotalEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePagePhysicalMemoryAvailableEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePagePhysicalMemorySystemCacheEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageTotalsHandleCountEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageTotalsProcessCountEdit, hDlg, 0, nYDifference);
        AdjustControlPosition(hPerformancePageTotalsThreadCountEdit, hDlg, 0, nYDifference);

        {
            static int lastX, lastY;

            nXDifference += lastX;
            nYDifference += lastY;
            lastX = lastY = 0;
            if (nXDifference % 2) {
                if (nXDifference > 0) {
                    nXDifference--;
                    lastX++;
                } else {
                    nXDifference++;
                    lastX--;
                }
            }
            if (nYDifference % 2) {
                if (nYDifference > 0) {
                    nYDifference--;
                    lastY++;
                } else {
                    nYDifference++;
                    lastY--;
                }
            }
        }

        AdjustFrameSize(hPerformancePageCpuUsageFrame, hDlg, nXDifference, nYDifference, 1);
        AdjustFrameSize(hPerformancePageMemUsageFrame, hDlg, nXDifference, nYDifference, 2);
        AdjustFrameSize(hPerformancePageCpuUsageHistoryFrame, hDlg, nXDifference, nYDifference, 3);
        AdjustFrameSize(hPerformancePageMemUsageHistoryFrame, hDlg, nXDifference, nYDifference, 4);
        AdjustFrameSize(hPerformancePageCpuUsageGraph, hDlg, nXDifference, nYDifference, 1);
        AdjustFrameSize(hPerformancePageMemUsageGraph, hDlg, nXDifference, nYDifference, 2);
        AdjustFrameSize(hPerformancePageCpuUsageHistoryGraph, hDlg, nXDifference, nYDifference, 3);
        AdjustFrameSize(hPerformancePageMemUsageHistoryGraph, hDlg, nXDifference, nYDifference, 4);
		break;
	}
    return 0;
}

void PerformancePage_OnViewShowKernelTimes(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);

	/*  Check or uncheck the show 16-bit tasks menu item */
	if (GetMenuState(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND) & MF_CHECKED)
	{
		CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);
		TaskManagerSettings.ShowKernelTimes = FALSE;
	}
	else
	{
		CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
		TaskManagerSettings.ShowKernelTimes = TRUE;
	}

	RefreshPerformancePage();
}

void PerformancePage_OnViewCPUHistoryOneGraphAll(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;
	HMENU	hCPUHistoryMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);
	hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

	TaskManagerSettings.CPUHistory_OneGraphPerCPU = FALSE;
	CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
}

void PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void)
{
	HMENU	hMenu;
	HMENU	hViewMenu;
	HMENU	hCPUHistoryMenu;

	hMenu = GetMenu(hMainWnd);
	hViewMenu = GetSubMenu(hMenu, 2);
	hCPUHistoryMenu = GetSubMenu(hViewMenu, 3);

	TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
	CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
}
