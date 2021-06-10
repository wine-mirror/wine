/*
 *  ReactOS Task Manager
 *
 * taskmgr.c : Defines the entry point for the application.
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

#include "resource.h"
#include "taskmgr.h"
#include "perfdata.h"
#include "column.h"

#define STATUS_WINDOW   2001

/* Global Variables: */
HINSTANCE hInst;                 /* current instance */

HWND hMainWnd;                   /* Main Window */
HWND hStatusWnd;                 /* Status Bar Window */
HWND hTabWnd;                    /* Tab Control Window */

int  nMinimumWidth;              /* Minimum width of the dialog (OnSize()'s cx) */
int  nMinimumHeight;             /* Minimum height of the dialog (OnSize()'s cy) */

int  nOldWidth;                  /* Holds the previous client area width */
int  nOldHeight;                 /* Holds the previous client area height */

BOOL bInMenuLoop = FALSE;        /* Tells us if we are in the menu loop */

TASKMANAGER_SETTINGS TaskManagerSettings;


void FillSolidRect(HDC hDC, LPCRECT lpRect, COLORREF clr)
{
    SetBkColor(hDC, clr);
    ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
}

static void FillSolidRect2(HDC hDC, int x, int y, int cx, int cy, COLORREF clr)
{
    RECT rect;

    SetBkColor(hDC, clr);
    SetRect(&rect, x, y, x + cx, y + cy);
    ExtTextOutW(hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

static void Draw3dRect(HDC hDC, int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
{
    FillSolidRect2(hDC, x, y, cx - 1, 1, clrTopLeft);
    FillSolidRect2(hDC, x, y, 1, cy - 1, clrTopLeft);
    FillSolidRect2(hDC, x + cx, y, -1, cy, clrBottomRight);
    FillSolidRect2(hDC, x, y + cy, cx, -1, clrBottomRight);
}

static BOOL OnCreate(HWND hWnd)
{
    HMENU   hMenu;
    HMENU   hEditMenu;
    HMENU   hViewMenu;
    HMENU   hUpdateSpeedMenu;
    HMENU   hCPUHistoryMenu;
    int     nActivePage;
    int     nParts[3];
    RECT    rc;
    TCITEMW item;

    static WCHAR wszApplications[255];
    static WCHAR wszProcesses[255];
    static WCHAR wszPerformance[255];

    LoadStringW(hInst, IDS_APPLICATIONS, wszApplications, ARRAY_SIZE(wszApplications));
    LoadStringW(hInst, IDS_PROCESSES, wszProcesses, ARRAY_SIZE(wszProcesses));
    LoadStringW(hInst, IDS_PERFORMANCE, wszPerformance, ARRAY_SIZE(wszPerformance));

    SendMessageW(hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIconW(hInst, MAKEINTRESOURCEW(IDI_TASKMANAGER)));
    SendMessageW(hMainWnd, WM_SETICON, ICON_SMALL,
                 (LPARAM)LoadImageW(hInst, MAKEINTRESOURCEW(IDI_TASKMANAGER), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                    LR_SHARED));

    /* Initialize the Windows Common Controls DLL */
    InitCommonControls();

    /* Get the minimum window sizes */
    GetWindowRect(hWnd, &rc);
    nMinimumWidth = (rc.right - rc.left);
    nMinimumHeight = (rc.bottom - rc.top);

    /* Create the status bar */
    hStatusWnd = CreateStatusWindowW(WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS|SBT_NOBORDERS, NULL, hWnd, STATUS_WINDOW);
    if(!hStatusWnd)
        return FALSE;

    /* Create the status bar panes */
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = 400;
    SendMessageW(hStatusWnd, SB_SETPARTS, 3, (LPARAM)nParts);

    /* Create tab pages */
    hTabWnd = GetDlgItem(hWnd, IDC_TAB);
#if 1
    hApplicationPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_APPLICATION_PAGE), hWnd, ApplicationPageWndProc);
    hProcessPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PROCESS_PAGE), hWnd, ProcessPageWndProc);
    hPerformancePage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PERFORMANCE_PAGE), hWnd, PerformancePageWndProc);
#else
    hApplicationPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_APPLICATION_PAGE), hTabWnd, ApplicationPageWndProc);
    hProcessPage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PROCESS_PAGE), hTabWnd, ProcessPageWndProc);
    hPerformancePage = CreateDialogW(hInst, MAKEINTRESOURCEW(IDD_PERFORMANCE_PAGE), hTabWnd, PerformancePageWndProc);
#endif

    /* Insert tabs */
    memset(&item, 0, sizeof(TCITEMW));
    item.mask = TCIF_TEXT;
    item.pszText = wszApplications;
    SendMessageW(hTabWnd, TCM_INSERTITEMW, 0, (LPARAM)&item);
    memset(&item, 0, sizeof(TCITEMW));
    item.mask = TCIF_TEXT;
    item.pszText = wszProcesses;
    SendMessageW(hTabWnd, TCM_INSERTITEMW, 1, (LPARAM)&item);
    memset(&item, 0, sizeof(TCITEMW));
    item.mask = TCIF_TEXT;
    item.pszText = wszPerformance;
    SendMessageW(hTabWnd, TCM_INSERTITEMW, 2, (LPARAM)&item);

    /* Size everything correctly */
    GetClientRect(hWnd, &rc);
    nOldWidth = rc.right;
    nOldHeight = rc.bottom;

    if ((TaskManagerSettings.Left != 0) ||
        (TaskManagerSettings.Top != 0) ||
        (TaskManagerSettings.Right != 0) ||
        (TaskManagerSettings.Bottom != 0))
        MoveWindow(hWnd, TaskManagerSettings.Left, TaskManagerSettings.Top, TaskManagerSettings.Right - TaskManagerSettings.Left, TaskManagerSettings.Bottom - TaskManagerSettings.Top, TRUE);

    if (TaskManagerSettings.Maximized)
        ShowWindow(hWnd, SW_MAXIMIZE);

    /* Set the always on top style */
    hMenu = GetMenu(hWnd);
    hEditMenu = GetSubMenu(hMenu, 1);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);
    hCPUHistoryMenu = GetSubMenu(hViewMenu, 7);

    /* Check or uncheck the always on top menu item */
    if (TaskManagerSettings.AlwaysOnTop) {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    } else {
        CheckMenuItem(hEditMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }

    /* Check or uncheck the minimize on use menu item */
    if (TaskManagerSettings.MinimizeOnUse)
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the hide when minimized menu item */
    if (TaskManagerSettings.HideWhenMinimized)
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_UNCHECKED);

    /* Check or uncheck the show 16-bit tasks menu item */
    if (TaskManagerSettings.Show16BitTasks)
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hEditMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_UNCHECKED);

    if (TaskManagerSettings.View_LargeIcons)
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
    else if (TaskManagerSettings.View_SmallIcons)
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    if (TaskManagerSettings.ShowKernelTimes)
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
    else
        CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);

    if (TaskManagerSettings.UpdateSpeed == 1)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_HIGH, MF_BYCOMMAND);
    else if (TaskManagerSettings.UpdateSpeed == 2)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_NORMAL, MF_BYCOMMAND);
    else if (TaskManagerSettings.UpdateSpeed == 4)
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_LOW, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_PAUSED, MF_BYCOMMAND);

    if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hCPUHistoryMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);

    nActivePage = TaskManagerSettings.ActiveTabPage;
    SendMessageW(hTabWnd, TCM_SETCURFOCUS, 0, 0);
    SendMessageW(hTabWnd, TCM_SETCURFOCUS, 1, 0);
    SendMessageW(hTabWnd, TCM_SETCURFOCUS, 2, 0);
    SendMessageW(hTabWnd, TCM_SETCURFOCUS, nActivePage, 0);

    if (TaskManagerSettings.UpdateSpeed == 1)
        SetTimer(hWnd, 1, 1000, NULL);
    else if (TaskManagerSettings.UpdateSpeed == 2)
        SetTimer(hWnd, 1, 2000, NULL);
    else if (TaskManagerSettings.UpdateSpeed == 4)
        SetTimer(hWnd, 1, 4000, NULL);

    /*
     * Refresh the performance data
     * Sample it twice so we can establish
     * the delta values & cpu usage
     */
    PerfDataRefresh();
    PerfDataRefresh();

    RefreshApplicationPage();
    RefreshProcessPage();
    RefreshPerformancePage();

    TrayIcon_ShellAddTrayIcon();

    return TRUE;
}

/* OnSize()
 * This function handles all the sizing events for the application
 * It re-sizes every window, and child window that needs re-sizing
 */
static void OnSize( UINT nType, int cx, int cy )
{
    int     nParts[3];
    int     nXDifference;
    int     nYDifference;
    RECT    rc;

    if (nType == SIZE_MINIMIZED)
    {
        if(TaskManagerSettings.HideWhenMinimized)
        {
          ShowWindow(hMainWnd, SW_HIDE);
        }
        return;
    }

    nXDifference = cx - nOldWidth;
    nYDifference = cy - nOldHeight;
    nOldWidth = cx;
    nOldHeight = cy;

    /* Update the status bar size */
    GetWindowRect(hStatusWnd, &rc);
    SendMessageW(hStatusWnd, WM_SIZE, nType, MAKELPARAM(cx, cy + (rc.bottom - rc.top)));

    /* Update the status bar pane sizes */
    nParts[0] = bInMenuLoop ? -1 : 100;
    nParts[1] = 210;
    nParts[2] = cx;
    SendMessageW(hStatusWnd, SB_SETPARTS, bInMenuLoop ? 1 : 3, (LPARAM)nParts);

    /* Resize the tab control */
    GetWindowRect(hTabWnd, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hTabWnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);

    /* Resize the application page */
    GetWindowRect(hApplicationPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hApplicationPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
    
    /* Resize the process page */
    GetWindowRect(hProcessPage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hProcessPage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
    
    /* Resize the performance page */
    GetWindowRect(hPerformancePage, &rc);
    cx = (rc.right - rc.left) + nXDifference;
    cy = (rc.bottom - rc.top) + nYDifference;
    SetWindowPos(hPerformancePage, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
}

static void LoadSettings(void)
{
    HKEY    hKey;
    int     i;
    DWORD   dwSize;

   static const WCHAR    wszSubKey[] = {'S','o','f','t','w','a','r','e','\\',
                                        'W','i','n','e','\\','T','a','s','k','M','a','n','a','g','e','r',0};
   static const WCHAR    wszPreferences[] = {'P','r','e','f','e','r','e','n','c','e','s',0};

    /* Window size & position settings */
    TaskManagerSettings.Maximized = FALSE;
    TaskManagerSettings.Left = 0;
    TaskManagerSettings.Top = 0;
    TaskManagerSettings.Right = 0;
    TaskManagerSettings.Bottom = 0;

    /* Tab settings */
    TaskManagerSettings.ActiveTabPage = 0;

    /* Options menu settings */
    TaskManagerSettings.AlwaysOnTop = FALSE;
    TaskManagerSettings.MinimizeOnUse = TRUE;
    TaskManagerSettings.HideWhenMinimized = TRUE;
    TaskManagerSettings.Show16BitTasks = TRUE;

    /* Update speed settings */
    TaskManagerSettings.UpdateSpeed = 2;

    /* Applications page settings */
    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = TRUE;

    /* Processes page settings */
    TaskManagerSettings.ShowProcessesFromAllUsers = FALSE; /* Server-only? */
    TaskManagerSettings.Column_ImageName = TRUE;
    TaskManagerSettings.Column_PID = TRUE;
    TaskManagerSettings.Column_CPUUsage = TRUE;
    TaskManagerSettings.Column_CPUTime = TRUE;
    TaskManagerSettings.Column_MemoryUsage = TRUE;
    TaskManagerSettings.Column_MemoryUsageDelta = FALSE;
    TaskManagerSettings.Column_PeakMemoryUsage = FALSE;
    TaskManagerSettings.Column_PageFaults = FALSE;
    TaskManagerSettings.Column_USERObjects = FALSE;
    TaskManagerSettings.Column_IOReads = FALSE;
    TaskManagerSettings.Column_IOReadBytes = FALSE;
    TaskManagerSettings.Column_SessionID = FALSE; /* Server-only? */
    TaskManagerSettings.Column_UserName = FALSE; /* Server-only? */
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

    for (i = 0; i < 25; i++) {
        TaskManagerSettings.ColumnOrderArray[i] = i;
    }
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

    TaskManagerSettings.SortColumn = 1;
    TaskManagerSettings.SortAscending = TRUE;

    /* Performance page settings */
    TaskManagerSettings.CPUHistory_OneGraphPerCPU = TRUE;
    TaskManagerSettings.ShowKernelTimes = FALSE;

    /* Open the key */
    /* @@ Wine registry key: HKCU\Software\Wine\TaskManager */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, wszSubKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return;
    /* Read the settings */
    dwSize = sizeof(TASKMANAGER_SETTINGS);
    RegQueryValueExW(hKey, wszPreferences, NULL, NULL, (LPBYTE)&TaskManagerSettings, &dwSize);

    /* Close the key */
    RegCloseKey(hKey);
}

static void SaveSettings(void)
{
    HKEY hKey;

    static const WCHAR wszSubKey3[] = {'S','o','f','t','w','a','r','e','\\',
                                       'W','i','n','e','\\','T','a','s','k','M','a','n','a','g','e','r',0};
    static const WCHAR wszPreferences[] = {'P','r','e','f','e','r','e','n','c','e','s',0};

    /* Open (or create) the key */

    /* @@ Wine registry key: HKCU\Software\Wine\TaskManager */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wszSubKey3, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return;
    /* Save the settings */
    RegSetValueExW(hKey, wszPreferences, 0, REG_BINARY, (LPBYTE)&TaskManagerSettings, sizeof(TASKMANAGER_SETTINGS));
    /* Close the key */
    RegCloseKey(hKey);
}

static void TaskManager_OnRestoreMainWindow(void)
{
  BOOL OnTop;

  OnTop = (GetWindowLongW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;

  OpenIcon(hMainWnd);
  SetForegroundWindow(hMainWnd);
  SetWindowPos(hMainWnd, (OnTop ? HWND_TOPMOST : HWND_TOP), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
}

static void TaskManager_OnEnterMenuLoop(HWND hWnd)
{
    int nParts;

    /* Update the status bar pane sizes */
    nParts = -1;
    SendMessageW(hStatusWnd, SB_SETPARTS, 1, (LPARAM)&nParts);
    bInMenuLoop = TRUE;
    SendMessageW(hStatusWnd, SB_SETTEXTW, 0, 0);
}

static void TaskManager_OnExitMenuLoop(HWND hWnd)
{
    RECT  rc;
    int   nParts[3];
    WCHAR text[256];

    WCHAR wszCPU_Usage[255];
    WCHAR wszProcesses[255];

    LoadStringW(hInst, IDS_STATUS_BAR_CPU_USAGE, wszCPU_Usage, ARRAY_SIZE(wszCPU_Usage));
    LoadStringW(hInst, IDS_STATUS_BAR_PROCESSES, wszProcesses, ARRAY_SIZE(wszProcesses));

    bInMenuLoop = FALSE;
    /* Update the status bar pane sizes */
    GetClientRect(hWnd, &rc);
    nParts[0] = 100;
    nParts[1] = 210;
    nParts[2] = rc.right;
    SendMessageW(hStatusWnd, SB_SETPARTS, 3, (LPARAM)nParts);
    SendMessageW(hStatusWnd, SB_SETTEXTW, 0, 0);
    wsprintfW(text, wszCPU_Usage, PerfDataGetProcessorUsage());
    SendMessageW(hStatusWnd, SB_SETTEXTW, 1, (LPARAM)text);
    wsprintfW(text, wszProcesses, PerfDataGetProcessCount());
    SendMessageW(hStatusWnd, SB_SETTEXTW, 0, (LPARAM)text);
}

static void TaskManager_OnMenuSelect(HWND hWnd, UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    WCHAR wstr[256] = {0};

    LoadStringW(hInst, nItemID, wstr, ARRAY_SIZE(wstr));
    SendMessageW(hStatusWnd, SB_SETTEXTW, 0, (LPARAM)wstr);
}

static void TaskManager_OnViewUpdateSpeedHigh(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 1;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_HIGH, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 1000, NULL);
}

static void TaskManager_OnViewUpdateSpeedNormal(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 2;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_NORMAL, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 2000, NULL);
}

static void TaskManager_OnViewUpdateSpeedLow(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);

    TaskManagerSettings.UpdateSpeed = 4;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_LOW, MF_BYCOMMAND);

    KillTimer(hMainWnd, 1);
    SetTimer(hMainWnd, 1, 4000, NULL);
}

static void TaskManager_OnViewUpdateSpeedPaused(void)
{
    HMENU hMenu;
    HMENU hViewMenu;
    HMENU hUpdateSpeedMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hUpdateSpeedMenu = GetSubMenu(hViewMenu, 1);
    TaskManagerSettings.UpdateSpeed = 0;
    CheckMenuRadioItem(hUpdateSpeedMenu, ID_VIEW_UPDATESPEED_HIGH, ID_VIEW_UPDATESPEED_PAUSED, ID_VIEW_UPDATESPEED_PAUSED, MF_BYCOMMAND);
    KillTimer(hMainWnd, 1);
}

static void TaskManager_OnTabWndSelChange(void)
{
    int   i;
    HMENU hMenu;
    HMENU hOptionsMenu;
    HMENU hViewMenu;
    HMENU hSubMenu;

    WCHAR wszLargeIcons[255];
    WCHAR wszSmallIcons[255];
    WCHAR wszDetails[255];
    WCHAR wszWindows[255];
    WCHAR wszSelectColumns[255];
    WCHAR wszShow16bTasks[255];
    WCHAR wszOneGraphAllCPU[255];
    WCHAR wszOneGraphPerCPU[255];
    WCHAR wszCPUHistory[255];
    WCHAR wszShowKernelTimes[255];

    LoadStringW(hInst, IDS_VIEW_LARGE, wszLargeIcons, ARRAY_SIZE(wszLargeIcons));
    LoadStringW(hInst, IDS_VIEW_SMALL, wszSmallIcons, ARRAY_SIZE(wszSmallIcons));
    LoadStringW(hInst, IDS_VIEW_DETAILS, wszDetails, ARRAY_SIZE(wszDetails));
    LoadStringW(hInst, IDS_WINDOWS, wszWindows, ARRAY_SIZE(wszWindows));
    LoadStringW(hInst, IDS_VIEW_SELECTCOLUMNS, wszSelectColumns, ARRAY_SIZE(wszSelectColumns));
    LoadStringW(hInst, IDS_OPTIONS_SHOW16BITTASKS, wszShow16bTasks, ARRAY_SIZE(wszShow16bTasks));
    LoadStringW(hInst, IDS_VIEW_CPUHISTORY_ONEGRAPHALL, wszOneGraphAllCPU, ARRAY_SIZE(wszOneGraphAllCPU));
    LoadStringW(hInst, IDS_VIEW_CPUHISTORY_ONEGRAPHPERCPU, wszOneGraphPerCPU, ARRAY_SIZE(wszOneGraphPerCPU));
    LoadStringW(hInst, IDS_VIEW_CPUHISTORY, wszCPUHistory, ARRAY_SIZE(wszCPUHistory));
    LoadStringW(hInst, IDS_VIEW_SHOWKERNELTIMES, wszShowKernelTimes, ARRAY_SIZE(wszShowKernelTimes));

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);
    hOptionsMenu = GetSubMenu(hMenu, 1);
    TaskManagerSettings.ActiveTabPage = SendMessageW(hTabWnd, TCM_GETCURSEL, 0, 0);
    for (i = GetMenuItemCount(hViewMenu) - 1; i > 2; i--) {
        hSubMenu = GetSubMenu(hViewMenu, i);
        if (hSubMenu)
            DestroyMenu(hSubMenu);
        RemoveMenu(hViewMenu, i, MF_BYPOSITION);
    }
    RemoveMenu(hOptionsMenu, 3, MF_BYPOSITION);
    switch (TaskManagerSettings.ActiveTabPage) {
    case 0:
        ShowWindow(hApplicationPage, SW_SHOW);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hApplicationPage);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_LARGE, wszLargeIcons);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SMALL, wszSmallIcons);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_DETAILS, wszDetails);

        if (GetMenuItemCount(hMenu) <= 4) {
            hSubMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_WINDOWSMENU));
            InsertMenuW(hMenu, 3, MF_BYPOSITION|MF_POPUP, (UINT_PTR)hSubMenu, wszWindows);
            DrawMenuBar(hMainWnd);
        }
        if (TaskManagerSettings.View_LargeIcons)
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
        else if (TaskManagerSettings.View_SmallIcons)
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
        else
            CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);
        /*
         * Give the application list control focus
         */
        SetFocus(hApplicationPageListCtrl);
        break;

    case 1:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_SHOW);
        ShowWindow(hPerformancePage, SW_HIDE);
        BringWindowToTop(hProcessPage);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SELECTCOLUMNS, wszSelectColumns);
        AppendMenuW(hOptionsMenu, MF_STRING, ID_OPTIONS_SHOW16BITTASKS, wszShow16bTasks);
        if (TaskManagerSettings.Show16BitTasks)
            CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
        if (GetMenuItemCount(hMenu) > 4)
        {
            RemoveMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }
        /*
         * Give the process list control focus
         */
        SetFocus(hProcessPageListCtrl);
        break;

    case 2:
        ShowWindow(hApplicationPage, SW_HIDE);
        ShowWindow(hProcessPage, SW_HIDE);
        ShowWindow(hPerformancePage, SW_SHOW);
        BringWindowToTop(hPerformancePage);
        if (GetMenuItemCount(hMenu) > 4) {
            RemoveMenu(hMenu, 3, MF_BYPOSITION);
            DrawMenuBar(hMainWnd);
        }
        hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHALL, wszOneGraphAllCPU);
        AppendMenuW(hSubMenu, MF_STRING, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, wszOneGraphPerCPU);
        AppendMenuW(hViewMenu, MF_STRING|MF_POPUP, (UINT_PTR)hSubMenu, wszCPUHistory);
        AppendMenuW(hViewMenu, MF_STRING, ID_VIEW_SHOWKERNELTIMES, wszShowKernelTimes);
        if (TaskManagerSettings.ShowKernelTimes)
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_CHECKED);
        else
            CheckMenuItem(hViewMenu, ID_VIEW_SHOWKERNELTIMES, MF_BYCOMMAND|MF_UNCHECKED);
        if (TaskManagerSettings.CPUHistory_OneGraphPerCPU)
            CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, MF_BYCOMMAND);
        else
            CheckMenuRadioItem(hSubMenu, ID_VIEW_CPUHISTORY_ONEGRAPHALL, ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU, ID_VIEW_CPUHISTORY_ONEGRAPHALL, MF_BYCOMMAND);
        /*
         * Give the tab control focus
         */
        SetFocus(hTabWnd);
        break;
    }
}

LPWSTR GetLastErrorText(LPWSTR lpwszBuf, DWORD dwSize)
{
    DWORD  dwRet;
    LPWSTR lpwszTemp = NULL;
    static const WCHAR    wszFormat[] = {'%','s',' ','(','%','u',')',0};

    dwRet = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           NULL,
                           GetLastError(),
                           LANG_NEUTRAL,
                           (LPWSTR)&lpwszTemp,
                           0,
                           NULL );

    /* supplied buffer is not long enough */
    if (!dwRet || ( dwSize < dwRet+14)) {
        lpwszBuf[0] = '\0';
    } else {
        lpwszTemp[lstrlenW(lpwszTemp)-2] = '\0';  /* remove cr and newline character */
        swprintf(lpwszBuf, dwSize, wszFormat, lpwszTemp, GetLastError());
    }
    if (lpwszTemp) {
        LocalFree(lpwszTemp);
    }
    return lpwszBuf;
}

/* Message handler for dialog box. */
static INT_PTR CALLBACK
TaskManagerWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static const WCHAR wszTaskmgr[] = {'t','a','s','k','m','g','r',0};
    HDC             hdc;
    PAINTSTRUCT     ps;
    LPRECT          pRC;
    RECT            rc;
    LPNMHDR         pnmh;
    WINDOWPLACEMENT wp;

    switch (message) {
    case WM_INITDIALOG:
        hMainWnd = hDlg;
        return OnCreate(hDlg);

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        /* Process menu commands */
        switch (LOWORD(wParam))
        {
        case ID_FILE_NEW:
            TaskManager_OnFileNew();
            break;
        case ID_OPTIONS_ALWAYSONTOP:
            TaskManager_OnOptionsAlwaysOnTop();
            break;
        case ID_OPTIONS_MINIMIZEONUSE:
            TaskManager_OnOptionsMinimizeOnUse();
            break;
        case ID_OPTIONS_HIDEWHENMINIMIZED:
            TaskManager_OnOptionsHideWhenMinimized();
            break;
        case ID_OPTIONS_SHOW16BITTASKS:
            TaskManager_OnOptionsShow16BitTasks();
            break;
        case ID_RESTORE:
            TaskManager_OnRestoreMainWindow();
            break;
        case ID_VIEW_LARGE:
            ApplicationPage_OnViewLargeIcons();
            break;
        case ID_VIEW_SMALL:
            ApplicationPage_OnViewSmallIcons();
            break;
        case ID_VIEW_DETAILS:
            ApplicationPage_OnViewDetails();
            break;
        case ID_VIEW_SHOWKERNELTIMES:
            PerformancePage_OnViewShowKernelTimes();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHALL:
            PerformancePage_OnViewCPUHistoryOneGraphAll();
            break;
        case ID_VIEW_CPUHISTORY_ONEGRAPHPERCPU:
            PerformancePage_OnViewCPUHistoryOneGraphPerCPU();
            break;
        case ID_VIEW_UPDATESPEED_HIGH:
            TaskManager_OnViewUpdateSpeedHigh();
            break;
        case ID_VIEW_UPDATESPEED_NORMAL:
            TaskManager_OnViewUpdateSpeedNormal();
            break;
        case ID_VIEW_UPDATESPEED_LOW:
            TaskManager_OnViewUpdateSpeedLow();
            break;
        case ID_VIEW_UPDATESPEED_PAUSED:
            TaskManager_OnViewUpdateSpeedPaused();
            break;
        case ID_VIEW_SELECTCOLUMNS:
            ProcessPage_OnViewSelectColumns();
            break;
        case ID_VIEW_REFRESH:
            PostMessageW(hDlg, WM_TIMER, 0, 0);
            break;
        case ID_WINDOWS_TILEHORIZONTALLY:
            ApplicationPage_OnWindowsTileHorizontally();
            break;
        case ID_WINDOWS_TILEVERTICALLY:
            ApplicationPage_OnWindowsTileVertically();
            break;
        case ID_WINDOWS_MINIMIZE:
            ApplicationPage_OnWindowsMinimize();
            break;
        case ID_WINDOWS_MAXIMIZE:
            ApplicationPage_OnWindowsMaximize();
            break;
        case ID_WINDOWS_CASCADE:
            ApplicationPage_OnWindowsCascade();
            break;
        case ID_WINDOWS_BRINGTOFRONT:
            ApplicationPage_OnWindowsBringToFront();
            break;
        case ID_APPLICATION_PAGE_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case ID_APPLICATION_PAGE_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case ID_APPLICATION_PAGE_GOTOPROCESS:
            ApplicationPage_OnGotoProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESS:
            ProcessPage_OnEndProcess();
            break;
        case ID_PROCESS_PAGE_ENDPROCESSTREE:
            ProcessPage_OnEndProcessTree();
            break;
        case ID_PROCESS_PAGE_DEBUG:
            ProcessPage_OnDebug();
            break;
        case ID_PROCESS_PAGE_SETAFFINITY:
            ProcessPage_OnSetAffinity();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_REALTIME:
            ProcessPage_OnSetPriorityRealTime();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_HIGH:
            ProcessPage_OnSetPriorityHigh();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_ABOVENORMAL:
            ProcessPage_OnSetPriorityAboveNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_NORMAL:
            ProcessPage_OnSetPriorityNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_BELOWNORMAL:
            ProcessPage_OnSetPriorityBelowNormal();
            break;
        case ID_PROCESS_PAGE_SETPRIORITY_LOW:
            ProcessPage_OnSetPriorityLow();
            break;
        case ID_PROCESS_PAGE_DEBUGCHANNELS:
            ProcessPage_OnDebugChannels();
            break;
        case ID_HELP_TOPICS:
            WinHelpW(hDlg, wszTaskmgr, HELP_FINDER, 0);
            break;
        case ID_HELP_ABOUT:
            OnAbout();
            break;
        case ID_FILE_EXIT:
            EndDialog(hDlg, IDOK);
            break;
        }     
        break;

    case WM_ONTRAYICON:
        switch(lParam)
        {
        case WM_RBUTTONDOWN:
            {
            POINT pt;
            BOOL OnTop;
            HMENU hMenu, hPopupMenu;
            
            GetCursorPos(&pt);
            
            OnTop = (GetWindowLongW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
            
            hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_TRAY_POPUP));
            hPopupMenu = GetSubMenu(hMenu, 0);
            
            if(IsWindowVisible(hMainWnd))
            {
              DeleteMenu(hPopupMenu, ID_RESTORE, MF_BYCOMMAND);
            }
            else
            {
              SetMenuDefaultItem(hPopupMenu, ID_RESTORE, FALSE);
            }
            
            if(OnTop)
            {
              CheckMenuItem(hPopupMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND | MF_CHECKED);
            }
            
            SetForegroundWindow(hMainWnd);
            TrackPopupMenuEx(hPopupMenu, 0, pt.x, pt.y, hMainWnd, NULL);
            
            DestroyMenu(hMenu);
            break;
            }
        case WM_LBUTTONDBLCLK:
            TaskManager_OnRestoreMainWindow();
            break;
        }
        break;

    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        if ((pnmh->hwndFrom == hTabWnd) &&
            (pnmh->idFrom == IDC_TAB) &&
            (pnmh->code == TCN_SELCHANGE))
        {
            TaskManager_OnTabWndSelChange();
        }
        break;

    case WM_NCPAINT:
        hdc = GetDC(hDlg);
        GetClientRect(hDlg, &rc);
        Draw3dRect(hdc, rc.left, rc.top, rc.right, rc.top + 2, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
        ReleaseDC(hDlg, hdc);
        break;

    case WM_PAINT:
        hdc = BeginPaint(hDlg, &ps);
        GetClientRect(hDlg, &rc);
        Draw3dRect(hdc, rc.left, rc.top, rc.right, rc.top + 2, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));
        EndPaint(hDlg, &ps);
        break;

    case WM_SIZING:
        /* Make sure the user is sizing the dialog */
        /* in an acceptable range */
        pRC = (LPRECT)lParam;
        if ((wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_BOTTOMLEFT)) {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->left = pRC->right - nMinimumWidth;
        } else {
            /* If the width is too small enlarge it to the minimum */
            if (nMinimumWidth > (pRC->right - pRC->left))
                pRC->right = pRC->left + nMinimumWidth;
        }
        if ((wParam == WMSZ_TOP) || (wParam == WMSZ_TOPLEFT) || (wParam == WMSZ_TOPRIGHT)) {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->top = pRC->bottom - nMinimumHeight;
        } else {
            /* If the height is too small enlarge it to the minimum */
            if (nMinimumHeight > (pRC->bottom - pRC->top))
                pRC->bottom = pRC->top + nMinimumHeight;
        }
        return TRUE;

    case WM_SIZE:
        /* Handle the window sizing in its own function */
        OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_DESTROY:
        ShowWindow(hDlg, SW_HIDE);
        TrayIcon_ShellRemoveTrayIcon();
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hDlg, &wp);
        TaskManagerSettings.Left = wp.rcNormalPosition.left;
        TaskManagerSettings.Top = wp.rcNormalPosition.top;
        TaskManagerSettings.Right = wp.rcNormalPosition.right;
        TaskManagerSettings.Bottom = wp.rcNormalPosition.bottom;
        if (IsZoomed(hDlg) || (wp.flags & WPF_RESTORETOMAXIMIZED))
            TaskManagerSettings.Maximized = TRUE;
        else
            TaskManagerSettings.Maximized = FALSE;
        return DefWindowProcW(hDlg, message, wParam, lParam);

    case WM_TIMER:
        /* Refresh the performance data */
        PerfDataRefresh();
        RefreshApplicationPage();
        RefreshProcessPage();
        RefreshPerformancePage();
        TrayIcon_ShellUpdateTrayIcon();
        break;

    case WM_ENTERMENULOOP:
        TaskManager_OnEnterMenuLoop(hDlg);
        break;
    case WM_EXITMENULOOP:
        TaskManager_OnExitMenuLoop(hDlg);
        break;
    case WM_MENUSELECT:
        TaskManager_OnMenuSelect(hDlg, LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
        break;
    }

    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    HANDLE hProcess;
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 

    InitCommonControls();

    /* Initialize global variables */
    hInst = hInstance;

    /* Change our priority class to HIGH */
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
    SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
    CloseHandle(hProcess);

    /* Now let's get the SE_DEBUG_NAME privilege
     * so that we can debug processes 
     */

    /* Get a token for this process.  */
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        static const WCHAR SeDebugPrivilegeW[] = {'S','e','D','e','b','u','g','P','r','i','v','i','l','e','g','e',0};

        /* Get the LUID for the debug privilege.  */
        LookupPrivilegeValueW(NULL, SeDebugPrivilegeW, &tkp.Privileges[0].Luid);

        tkp.PrivilegeCount = 1;  /* one privilege to set */
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

        /* Get the debug privilege for this process. */
        AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
    }

    /* Load our settings from the registry */
    LoadSettings();

    /* Initialize perf data */
    if (!PerfDataInitialize()) {
        return -1;
    }

    DialogBoxW(hInst, (LPWSTR)IDD_TASKMGR_DIALOG, NULL, TaskManagerWndProc);
 
    /* Save our settings to the registry */
    SaveSettings();
    return 0;
}
