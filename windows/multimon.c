/*
 * Multimonitor APIs
 *
 * Copyright 1998 Turchanov Sergey
 */

#include "windows.h"

#define xPRIMARY_MONITOR ((HMONITOR)0x12340042)

HMONITOR WINAPI MonitorFromPoint(POINT32 ptScreenCoords, DWORD dwFlags)
{
    if ((dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST)) ||
        ((ptScreenCoords.x >= 0) &&
        (ptScreenCoords.x < GetSystemMetrics32(SM_CXSCREEN)) &&
        (ptScreenCoords.y >= 0) &&
        (ptScreenCoords.y < GetSystemMetrics32(SM_CYSCREEN))))
    {
        return xPRIMARY_MONITOR;
    }
        return NULL;
}

HMONITOR WINAPI MonitorFromRect(LPRECT32 lprcScreenCoords, DWORD dwFlags)
{
    if ((dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST)) ||
        ((lprcScreenCoords->right > 0) &&
        (lprcScreenCoords->bottom > 0) &&
        (lprcScreenCoords->left < GetSystemMetrics32(SM_CXSCREEN)) &&
        (lprcScreenCoords->top < GetSystemMetrics32(SM_CYSCREEN))))
    {
        return xPRIMARY_MONITOR;
    }
    return NULL;
}

HMONITOR WINAPI MonitorFromWindow(HWND32 hWnd, DWORD dwFlags)
{
    WINDOWPLACEMENT32 wp;

    if (dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST))
        return xPRIMARY_MONITOR;

    if (IsIconic32(hWnd) ? 
            GetWindowPlacement32(hWnd, &wp) : 
            GetWindowRect32(hWnd, &wp.rcNormalPosition)) {

        return MonitorFromRect(&wp.rcNormalPosition, dwFlags);
    }

    return NULL;
}

BOOL32 WINAPI GetMonitorInfo32A(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    RECT32 rcWork;

    if ((hMonitor == xPRIMARY_MONITOR) &&
        lpMonitorInfo &&
        (lpMonitorInfo->cbSize >= sizeof(MONITORINFO)) &&
        SystemParametersInfo32A(SPI_GETWORKAREA, 0, &rcWork, 0))
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top  = 0;
        lpMonitorInfo->rcMonitor.right  = GetSystemMetrics32(SM_CXSCREEN);
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics32(SM_CYSCREEN);
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;
	
	if (lpMonitorInfo->cbSize >= sizeof(MONITORINFOEX32A))
            lstrcpy32A(((MONITORINFOEX32A*)lpMonitorInfo)->szDevice, "DISPLAY");

        return TRUE;
    }

    return FALSE;
}

BOOL32 WINAPI GetMonitorInfo32W(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    RECT32 rcWork;

    if ((hMonitor == xPRIMARY_MONITOR) &&
        lpMonitorInfo &&
        (lpMonitorInfo->cbSize >= sizeof(MONITORINFO)) &&
        SystemParametersInfo32W(SPI_GETWORKAREA, 0, &rcWork, 0))
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top  = 0;
        lpMonitorInfo->rcMonitor.right  = GetSystemMetrics32(SM_CXSCREEN);
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics32(SM_CYSCREEN);
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;

        if (lpMonitorInfo->cbSize >= sizeof(MONITORINFOEX32W))
            lstrcpy32W(((MONITORINFOEX32W*)lpMonitorInfo)->szDevice, (LPCWSTR)"D\0I\0S\0P\0L\0A\0Y\0\0");

        return TRUE;
    }

    return FALSE;
}

BOOL32 WINAPI EnumDisplayMonitors(
        HDC32             hdcOptionalForPainting,
        LPRECT32         lprcEnumMonitorsThatIntersect,
        MONITORENUMPROC lpfnEnumProc,
        LPARAM          dwData)
{
    RECT32 rcLimit;

    if (!lpfnEnumProc)
        return FALSE;

    rcLimit.left   = 0;
    rcLimit.top    = 0;
    rcLimit.right  = GetSystemMetrics32(SM_CXSCREEN);
    rcLimit.bottom = GetSystemMetrics32(SM_CYSCREEN);

    if (hdcOptionalForPainting)
    {
        RECT32    rcClip;
        POINT32   ptOrg;

        switch (GetClipBox32(hdcOptionalForPainting, &rcClip))
        {
        default:
            if (!GetDCOrgEx(hdcOptionalForPainting, &ptOrg))
                return FALSE;

            OffsetRect32(&rcLimit, -ptOrg.x, -ptOrg.y);
            if (IntersectRect32(&rcLimit, &rcLimit, &rcClip) &&
                (!lprcEnumMonitorsThatIntersect ||
                     IntersectRect32(&rcLimit, &rcLimit, lprcEnumMonitorsThatIntersect))) {

                break;
            }
            //fall thru
        case NULLREGION:
             return TRUE;
        case ERROR:
             return FALSE;
        }
    } else {
        if (    lprcEnumMonitorsThatIntersect &&
                !IntersectRect32(&rcLimit, &rcLimit, lprcEnumMonitorsThatIntersect)) {

            return TRUE;
        }
    }

    return lpfnEnumProc(
            xPRIMARY_MONITOR,
            hdcOptionalForPainting,
            &rcLimit,
            dwData);
}
