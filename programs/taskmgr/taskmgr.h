/*
 *  ReactOS Task Manager
 *
 *  taskmgr.h
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

#ifndef __TASKMGR_H__
#define __TASKMGR_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
/*MF
typedef struct _IO_COUNTERS {
	ULONGLONG  ReadOperationCount;
	ULONGLONG  WriteOperationCount;
	ULONGLONG  OtherOperationCount;
	ULONGLONG ReadTransferCount;
	ULONGLONG WriteTransferCount;
	ULONGLONG OtherTransferCount;
} IO_COUNTERS, *PIO_COUNTERS;
*/
#endif /* _MSC_VER */

#include "resource.h"

#define RUN_APPS_PAGE
#define RUN_PROC_PAGE
#define RUN_PERF_PAGE

#define STATUS_WINDOW	2001

typedef struct
{
	/* Window size & position settings */
	BOOL	Maximized;
	int	Left;
	int	Top;
	int	Right;
	int	Bottom;

	/* Tab settings */
	int	ActiveTabPage;

	/* Options menu settings */
	BOOL	AlwaysOnTop;
	BOOL	MinimizeOnUse;
	BOOL	HideWhenMinimized;
	BOOL	Show16BitTasks;

	/* Update speed settings */
	/* How many half-seconds in between updates (i.e. 0 - Paused, 1 - High, 2 - Normal, 4 - Low) */
	int	UpdateSpeed; 

	/* Applications page settings */
	BOOL	View_LargeIcons;
	BOOL	View_SmallIcons;
	BOOL	View_Details;

	/* Processes page settings */
	BOOL	ShowProcessesFromAllUsers; /* Server-only? */
	BOOL	Column_ImageName;
	BOOL	Column_PID;
	BOOL	Column_CPUUsage;
	BOOL	Column_CPUTime;
	BOOL	Column_MemoryUsage;
	BOOL	Column_MemoryUsageDelta;
	BOOL	Column_PeakMemoryUsage;
	BOOL	Column_PageFaults;
	BOOL	Column_USERObjects;
	BOOL	Column_IOReads;
	BOOL	Column_IOReadBytes;
	BOOL	Column_SessionID; /* Server-only? */
	BOOL	Column_UserName; /* Server-only? */
	BOOL	Column_PageFaultsDelta;
	BOOL	Column_VirtualMemorySize;
	BOOL	Column_PagedPool;
	BOOL	Column_NonPagedPool;
	BOOL	Column_BasePriority;
	BOOL	Column_HandleCount;
	BOOL	Column_ThreadCount;
	BOOL	Column_GDIObjects;
	BOOL	Column_IOWrites;
	BOOL	Column_IOWriteBytes;
	BOOL	Column_IOOther;
	BOOL	Column_IOOtherBytes;
	int	ColumnOrderArray[25];
	int	ColumnSizeArray[25];
	int	SortColumn;
	BOOL	SortAscending;

	/* Performance page settings */
	BOOL	CPUHistory_OneGraphPerCPU;
	BOOL	ShowKernelTimes;

} TASKMANAGER_SETTINGS, *LPTASKMANAGER_SETTINGS;

/* Global Variables: */
extern	HINSTANCE	hInst;						/* current instance */
extern	HWND		hMainWnd;					/* Main Window */
extern	HWND		hStatusWnd;					/* Status Bar Window */
extern	HWND		hTabWnd;					/* Tab Control Window */
extern	int			nMinimumWidth;				/* Minimum width of the dialog (OnSize()'s cx) */
extern	int			nMinimumHeight;				/* Minimum height of the dialog (OnSize()'s cy) */
extern	int			nOldWidth;					/* Holds the previous client area width */
extern	int			nOldHeight;					/* Holds the previous client area height */
extern	TASKMANAGER_SETTINGS	TaskManagerSettings;

extern WNDPROC OldProcessListWndProc;
extern WNDPROC OldGraphWndProc;

extern HWND hProcessPage;				/* Process List Property Page */
extern HWND hProcessPageListCtrl;			/* Process ListCtrl Window */
extern HWND hProcessPageHeaderCtrl;			/* Process Header Control */
extern HWND hProcessPageEndProcessButton;		/* Process End Process button */
extern HWND hProcessPageShowAllProcessesButton;		/* Process Show All Processes checkbox */
extern HWND hPerformancePage;				/* Performance Property Page */

extern HWND hApplicationPage;                /* Application List Property Page */
extern HWND hApplicationPageListCtrl;        /* Application ListCtrl Window */
extern HWND hApplicationPageEndTaskButton;   /* Application End Task button */
extern HWND hApplicationPageSwitchToButton;  /* Application Switch To button */
extern HWND hApplicationPageNewTaskButton;   /* Application New Task button */


/* Forward declarations of functions included in this code module: */
void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr);
void Font_DrawText(HDC hDC, LPWSTR lpwszText, int x, int y);

#define OPTIONS_MENU_INDEX    1

void TaskManager_OnOptionsAlwaysOnTop(void);
void TaskManager_OnOptionsMinimizeOnUse(void);
void TaskManager_OnOptionsHideWhenMinimized(void);
void TaskManager_OnOptionsShow16BitTasks(void);
void TaskManager_OnFileNew(void);

LPWSTR GetLastErrorText( LPWSTR lpwszBuf, DWORD dwSize );

void OnAbout(void);

void ProcessPage_OnSetAffinity(void);
void ProcessPage_OnDebug(void);
void ProcessPage_OnEndProcess(void);
void ProcessPage_OnEndProcessTree(void);
void ProcessPage_OnSetPriorityRealTime(void);
void ProcessPage_OnSetPriorityHigh(void);
void ProcessPage_OnSetPriorityAboveNormal(void);
void ProcessPage_OnSetPriorityNormal(void);
void ProcessPage_OnSetPriorityBelowNormal(void);
void ProcessPage_OnSetPriorityLow(void);
void ProcessPage_OnDebugChannels(void);

#define WM_ONTRAYICON   WM_USER + 5

BOOL TrayIcon_ShellAddTrayIcon(void);
BOOL TrayIcon_ShellRemoveTrayIcon(void);
BOOL TrayIcon_ShellUpdateTrayIcon(void);

void PerformancePage_OnViewShowKernelTimes(void);
void PerformancePage_OnViewCPUHistoryOneGraphAll(void);
void PerformancePage_OnViewCPUHistoryOneGraphPerCPU(void);

void ApplicationPage_OnViewLargeIcons(void);
void ApplicationPage_OnViewSmallIcons(void);
void ApplicationPage_OnViewDetails(void);
void ApplicationPage_OnWindowsTileHorizontally(void);
void ApplicationPage_OnWindowsTileVertically(void);
void ApplicationPage_OnWindowsMinimize(void);
void ApplicationPage_OnWindowsMaximize(void);
void ApplicationPage_OnWindowsCascade(void);
void ApplicationPage_OnWindowsBringToFront(void);
void ApplicationPage_OnSwitchTo(void);
void ApplicationPage_OnEndTask(void);
void ApplicationPage_OnGotoProcess(void);

void RefreshApplicationPage(void);
void RefreshPerformancePage(void);
void RefreshProcessPage(void);

INT_PTR CALLBACK ApplicationPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Graph_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProcessPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PerformancePageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#ifdef __cplusplus
}
#endif

#endif /* __TASKMGR_H__ */
