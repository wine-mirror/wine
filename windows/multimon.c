/*
 * Multimonitor APIs
 *
 * Copyright 1998 Turchanov Sergey
 */

#include "monitor.h"
#include "windef.h"
#include "wingdi.h"
#include "winbase.h"
#include "winuser.h"

/**********************************************************************/

MONITOR_DRIVER *MONITOR_Driver;

/**********************************************************************/

#define xPRIMARY_MONITOR ((HMONITOR)0x12340042)

MONITOR MONITOR_PrimaryMonitor;

/***********************************************************************
 *              MONITOR_GetMonitor
 */
#if 0
static MONITOR *MONITOR_GetMonitor(HMONITOR hMonitor)
{
  if(hMonitor == xPRIMARY_MONITOR)
    {
      return &MONITOR_PrimaryMonitor;
    }
  else
    {
      return NULL;
    }
}
#endif

/***********************************************************************
 *              MONITOR_IsSingleWindow
 */
BOOL MONITOR_IsSingleWindow(MONITOR *pMonitor)
{
  return MONITOR_Driver->pIsSingleWindow(pMonitor);
}

/***********************************************************************
 *              MONITOR_GetWidth
 */
int MONITOR_GetWidth(MONITOR *pMonitor)
{
  return MONITOR_Driver->pGetWidth(pMonitor);
}

/***********************************************************************
 *              MONITOR_GetHeight
 */
int MONITOR_GetHeight(MONITOR *pMonitor)
{
  return MONITOR_Driver->pGetHeight(pMonitor);
}

/***********************************************************************
 *              MONITOR_GetDepth
 */
int MONITOR_GetDepth(MONITOR *pMonitor)
{
  return MONITOR_Driver->pGetDepth(pMonitor);
}

/***********************************************************************
 *              MONITOR_GetScreenSaveActive
 */
BOOL MONITOR_GetScreenSaveActive(MONITOR *pMonitor)
{
  return MONITOR_Driver->pGetScreenSaveActive(pMonitor);
}

/***********************************************************************
 *              MONITOR_SetScreenSaveActive
 */
void MONITOR_SetScreenSaveActive(MONITOR *pMonitor, BOOL bActivate)
{
  MONITOR_Driver->pSetScreenSaveActive(pMonitor, bActivate);
}

/***********************************************************************
 *              MONITOR_GetScreenSaveTimeout
 */
int MONITOR_GetScreenSaveTimeout(MONITOR *pMonitor)
{
  return MONITOR_Driver->pGetScreenSaveTimeout(pMonitor);
}

/***********************************************************************
 *              MONITOR_SetScreenSaveTimeout
 */
void MONITOR_SetScreenSaveTimeout(MONITOR *pMonitor, int nTimeout)
{
  MONITOR_Driver->pSetScreenSaveTimeout(pMonitor, nTimeout);
}


/**********************************************************************/

HMONITOR WINAPI MonitorFromPoint(POINT ptScreenCoords, DWORD dwFlags)
{
    if ((dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST)) ||
        ((ptScreenCoords.x >= 0) &&
        (ptScreenCoords.x < GetSystemMetrics(SM_CXSCREEN)) &&
        (ptScreenCoords.y >= 0) &&
        (ptScreenCoords.y < GetSystemMetrics(SM_CYSCREEN))))
    {
        return xPRIMARY_MONITOR;
    }
        return NULL;
}

HMONITOR WINAPI MonitorFromRect(LPRECT lprcScreenCoords, DWORD dwFlags)
{
    if ((dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST)) ||
        ((lprcScreenCoords->right > 0) &&
        (lprcScreenCoords->bottom > 0) &&
        (lprcScreenCoords->left < GetSystemMetrics(SM_CXSCREEN)) &&
        (lprcScreenCoords->top < GetSystemMetrics(SM_CYSCREEN))))
    {
        return xPRIMARY_MONITOR;
    }
    return NULL;
}

HMONITOR WINAPI MonitorFromWindow(HWND hWnd, DWORD dwFlags)
{
    WINDOWPLACEMENT wp;

    if (dwFlags & (MONITOR_DEFAULTTOPRIMARY | MONITOR_DEFAULTTONEAREST))
        return xPRIMARY_MONITOR;

    if (IsIconic(hWnd) ? 
            GetWindowPlacement(hWnd, &wp) : 
            GetWindowRect(hWnd, &wp.rcNormalPosition)) {

        return MonitorFromRect(&wp.rcNormalPosition, dwFlags);
    }

    return NULL;
}

BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    RECT rcWork;

    if ((hMonitor == xPRIMARY_MONITOR) &&
        lpMonitorInfo &&
        (lpMonitorInfo->cbSize >= sizeof(MONITORINFO)) &&
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWork, 0))
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top  = 0;
        lpMonitorInfo->rcMonitor.right  = GetSystemMetrics(SM_CXSCREEN);
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;
	
	if (lpMonitorInfo->cbSize >= sizeof(MONITORINFOEXA))
            lstrcpyA(((MONITORINFOEXA*)lpMonitorInfo)->szDevice, "DISPLAY");

        return TRUE;
    }

    return FALSE;
}

BOOL WINAPI GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    RECT rcWork;

    if ((hMonitor == xPRIMARY_MONITOR) &&
        lpMonitorInfo &&
        (lpMonitorInfo->cbSize >= sizeof(MONITORINFO)) &&
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0))
    {
        lpMonitorInfo->rcMonitor.left = 0;
        lpMonitorInfo->rcMonitor.top  = 0;
        lpMonitorInfo->rcMonitor.right  = GetSystemMetrics(SM_CXSCREEN);
        lpMonitorInfo->rcMonitor.bottom = GetSystemMetrics(SM_CYSCREEN);
        lpMonitorInfo->rcWork = rcWork;
        lpMonitorInfo->dwFlags = MONITORINFOF_PRIMARY;

        if (lpMonitorInfo->cbSize >= sizeof(MONITORINFOEXW))
            lstrcpyW(((MONITORINFOEXW*)lpMonitorInfo)->szDevice, (LPCWSTR)"D\0I\0S\0P\0L\0A\0Y\0\0");

        return TRUE;
    }

    return FALSE;
}

BOOL WINAPI EnumDisplayMonitors(
        HDC             hdcOptionalForPainting,
        LPRECT         lprcEnumMonitorsThatIntersect,
        MONITORENUMPROC lpfnEnumProc,
        LPARAM          dwData)
{
    RECT rcLimit;

    if (!lpfnEnumProc)
        return FALSE;

    rcLimit.left   = 0;
    rcLimit.top    = 0;
    rcLimit.right  = GetSystemMetrics(SM_CXSCREEN);
    rcLimit.bottom = GetSystemMetrics(SM_CYSCREEN);

    if (hdcOptionalForPainting)
    {
        RECT    rcClip;
        POINT   ptOrg;

        switch (GetClipBox(hdcOptionalForPainting, &rcClip))
        {
        default:
            if (!GetDCOrgEx(hdcOptionalForPainting, &ptOrg))
                return FALSE;

            OffsetRect(&rcLimit, -ptOrg.x, -ptOrg.y);
            if (IntersectRect(&rcLimit, &rcLimit, &rcClip) &&
                (!lprcEnumMonitorsThatIntersect ||
                     IntersectRect(&rcLimit, &rcLimit, lprcEnumMonitorsThatIntersect))) {

                break;
            }
            /*fall thru */
        case NULLREGION:
             return TRUE;
        case ERROR:
             return FALSE;
        }
    } else {
        if (    lprcEnumMonitorsThatIntersect &&
                !IntersectRect(&rcLimit, &rcLimit, lprcEnumMonitorsThatIntersect)) {

            return TRUE;
        }
    }

    return lpfnEnumProc(
            xPRIMARY_MONITOR,
            hdcOptionalForPainting,
            &rcLimit,
            dwData);
}
