/*
 *  ReactOS Task Manager
 *
 *  trayicon.c
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

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <commctrl.h>
#include <winnt.h>
#include <shellapi.h>

#include "taskmgr.h"
#include "perfdata.h"

static HICON TrayIcon_GetProcessorUsageIcon(void)
{
    HICON        hTrayIcon = NULL;
    HDC            hScreenDC = NULL;
    HDC            hDC = NULL;
    HBITMAP        hBitmap = NULL;
    HBITMAP        hOldBitmap;
    HBITMAP        hBitmapMask = NULL;
    ICONINFO    iconInfo;
    ULONG        ProcessorUsage;
    int            nLinesToDraw;
    HBRUSH        hBitmapBrush = NULL;
    RECT        rc;

    /*
     * Get a handle to the screen DC
     */
    hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        goto done;
    
    /*
     * Create our own DC from it
     */
    hDC = CreateCompatibleDC(hScreenDC);
    if (!hDC)
        goto done;

    /*
     * Load the bitmaps
     */
    hBitmap = LoadBitmapW(hInst, MAKEINTRESOURCEW(IDB_TRAYICON));
    hBitmapMask = LoadBitmapW(hInst, MAKEINTRESOURCEW(IDB_TRAYMASK));
    if (!hBitmap || !hBitmapMask)
        goto done;

    hBitmapBrush = CreateSolidBrush(RGB(0, 255, 0));
    if (!hBitmapBrush)
        goto done;
    
    /*
     * Select the bitmap into our device context
     * so we can draw on it.
     */
    hOldBitmap = SelectObject(hDC, hBitmap);

    /*
     * Get the cpu usage
     */
    ProcessorUsage = PerfDataGetProcessorUsage();

    /*
     * Calculate how many lines to draw
     * since we have 11 rows of space
     * to draw the cpu usage instead of
     * just having 10.
     */
    nLinesToDraw = (ProcessorUsage + (ProcessorUsage / 10)) / 11;
    SetRect(&rc, 3, 12 - nLinesToDraw, 13, 13);

    /*
     * Now draw the cpu usage
     */
    if (nLinesToDraw)
        FillRect(hDC, &rc, hBitmapBrush);

    /*
     * Now that we are done drawing put the
     * old bitmap back.
     */
    SelectObject(hDC, hOldBitmap);

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = hBitmapMask;
    iconInfo.hbmColor = hBitmap;

    hTrayIcon = CreateIconIndirect(&iconInfo);

done:
    /*
     * Cleanup
     */
    if (hScreenDC)
        ReleaseDC(NULL, hScreenDC);
    if (hDC)
        DeleteDC(hDC);
    if (hBitmapBrush)
        DeleteObject(hBitmapBrush);
    if (hBitmap)
        DeleteObject(hBitmap);
    if (hBitmapMask)
        DeleteObject(hBitmapMask);
    
    /*
     * Return the newly created tray icon (if successful)
     */
    return hTrayIcon;
}

BOOL TrayIcon_ShellAddTrayIcon(void)
{
    NOTIFYICONDATAW    nid;
    HICON            hIcon = NULL;
    BOOL            bRetVal;
    WCHAR           wszCPU_Usage[255];

    LoadStringW(hInst, IDS_STATUS_BAR_CPU_USAGE, wszCPU_Usage, ARRAY_SIZE(wszCPU_Usage));

    memset(&nid, 0, sizeof(NOTIFYICONDATAW));

    hIcon = TrayIcon_GetProcessorUsageIcon();

    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;
    wsprintfW(nid.szTip, wszCPU_Usage, PerfDataGetProcessorUsage());

    bRetVal = Shell_NotifyIconW(NIM_ADD, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}

BOOL TrayIcon_ShellRemoveTrayIcon(void)
{
    NOTIFYICONDATAW    nid;
    BOOL            bRetVal;
    
    memset(&nid, 0, sizeof(NOTIFYICONDATAW));
    
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    nid.uCallbackMessage = WM_ONTRAYICON;
    
    bRetVal = Shell_NotifyIconW(NIM_DELETE, &nid);
    
    return bRetVal;
}

BOOL TrayIcon_ShellUpdateTrayIcon(void)
{
    NOTIFYICONDATAW    nid;
    HICON            hIcon = NULL;
    BOOL            bRetVal;
    WCHAR           wszCPU_Usage[255];

    LoadStringW(hInst, IDS_STATUS_BAR_CPU_USAGE, wszCPU_Usage, ARRAY_SIZE(wszCPU_Usage));

    memset(&nid, 0, sizeof(NOTIFYICONDATAW));
    
    hIcon = TrayIcon_GetProcessorUsageIcon();
    
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;
    wsprintfW(nid.szTip, wszCPU_Usage, PerfDataGetProcessorUsage());
    
    bRetVal = Shell_NotifyIconW(NIM_MODIFY, &nid);
    
    if (hIcon)
        DestroyIcon(hIcon);
    
    return bRetVal;
}
