/*
 *  ReactOS Task Manager
 *
 *  applpage.c
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

#include "taskmgr.h"

typedef struct
{
    HWND    hWnd;
    WCHAR   wszTitle[256];
    HICON   hIcon;
    BOOL    bHung;
} APPLICATION_PAGE_LIST_ITEM, *LPAPPLICATION_PAGE_LIST_ITEM;

HWND            hApplicationPage;               /* Application List Property Page */
HWND            hApplicationPageListCtrl;       /* Application ListCtrl Window */
HWND            hApplicationPageEndTaskButton;  /* Application End Task button */
HWND            hApplicationPageSwitchToButton; /* Application Switch To button */
HWND            hApplicationPageNewTaskButton;  /* Application New Task button */
static int      nApplicationPageWidth;
static int      nApplicationPageHeight;
static HANDLE   hApplicationPageEvent = NULL;   /* When this event becomes signaled then we refresh the app list */
static BOOL     bSortAscending = TRUE;

static void ApplicationPageUpdate(void)
{
    /* Enable or disable the "End Task" & "Switch To" buttons */
    if (SendMessageW(hApplicationPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0))
    {
        EnableWindow(hApplicationPageEndTaskButton, TRUE);
        EnableWindow(hApplicationPageSwitchToButton, TRUE);
    }
    else
    {
        EnableWindow(hApplicationPageEndTaskButton, FALSE);
        EnableWindow(hApplicationPageSwitchToButton, FALSE);
    }

    /* If we are on the applications tab, then the windows menu will */
    /* be present on the menu bar so enable & disable the menu items */
    if (SendMessageW(hTabWnd, TCM_GETCURSEL, 0, 0) == 0)
    {
        HMENU   hMenu;
        HMENU   hWindowsMenu;
        UINT    count;

        hMenu = GetMenu(hMainWnd);
        hWindowsMenu = GetSubMenu(hMenu, 3);
        count = SendMessageW(hApplicationPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0);

        /* Only one item selected */
        if (count == 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_ENABLED);
        }
        /* More than one item selected */
        else if (count > 1)
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
        /* No items selected */
        else
        {
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
            EnableMenuItem(hWindowsMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        }
    }
}

static void AddOrUpdateHwnd(HWND hWnd, WCHAR *wszTitle, HICON hIcon, BOOL bHung)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    HIMAGELIST                      hImageListLarge;
    HIMAGELIST                      hImageListSmall;
    LV_ITEMW                        item;
    int                             i, count;
    BOOL                            bAlreadyInList = FALSE;
    BOOL                            bItemRemoved = FALSE;

    memset(&item, 0, sizeof(LV_ITEMW));

    /* Get the image lists */
    hImageListLarge = (HIMAGELIST)SendMessageW(hApplicationPageListCtrl, LVM_GETIMAGELIST, LVSIL_NORMAL, 0);
    hImageListSmall = (HIMAGELIST)SendMessageW(hApplicationPageListCtrl, LVM_GETIMAGELIST, LVSIL_SMALL, 0);

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    /* Check to see if it's already in our list */
    for (i=0; i<count; i++)
    {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_IMAGE|LVIF_PARAM;
        item.iItem = i;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);

        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        if (pAPLI->hWnd == hWnd)
        {
            bAlreadyInList = TRUE;
            break;
        }
    }

    /* If it is already in the list then update it if necessary */
    if (bAlreadyInList)
    {
        /* Check to see if anything needs updating */
        if ((pAPLI->hIcon != hIcon) ||
            (lstrcmpW(pAPLI->wszTitle, wszTitle) != 0) ||
            (pAPLI->bHung != bHung))
        {
            /* Update the structure */
            pAPLI->hIcon = hIcon;
            pAPLI->bHung = bHung;
            lstrcpyW(pAPLI->wszTitle, wszTitle);

            /* Update the image list */
            ImageList_ReplaceIcon(hImageListLarge, item.iItem, hIcon);
            ImageList_ReplaceIcon(hImageListSmall, item.iItem, hIcon);

            /* Update the list view */
            count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
            SendMessageW(hApplicationPageListCtrl, LVM_REDRAWITEMS, 0, count);
            /* UpdateWindow(hApplicationPageListCtrl); */
            InvalidateRect(hApplicationPageListCtrl, NULL, 0);
        }
    }
    /* It is not already in the list so add it */
    else
    {
        pAPLI = HeapAlloc(GetProcessHeap(), 0, sizeof(APPLICATION_PAGE_LIST_ITEM));

        pAPLI->hWnd = hWnd;
        pAPLI->hIcon = hIcon;
        pAPLI->bHung = bHung;
        lstrcpyW(pAPLI->wszTitle, wszTitle);

        /* Add the item to the list */
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
        ImageList_AddIcon(hImageListLarge, hIcon);
        item.iImage = ImageList_AddIcon(hImageListSmall, hIcon);
        item.pszText = LPSTR_TEXTCALLBACKW;
        item.iItem = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
        item.lParam = (LPARAM)pAPLI;
        SendMessageW(hApplicationPageListCtrl, LVM_INSERTITEMW, 0, (LPARAM) &item);
    }


    /* Check to see if we need to remove any items from the list */
    for (i=SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0)-1; i>=0; i--)
    {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_IMAGE|LVIF_PARAM;
        item.iItem = i;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);

        pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
        if (!IsWindow(pAPLI->hWnd)||
            (lstrlenW(pAPLI->wszTitle) <= 0) ||
            !IsWindowVisible(pAPLI->hWnd) ||
            (GetParent(pAPLI->hWnd) != NULL) ||
            (GetWindow(pAPLI->hWnd, GW_OWNER) != NULL) ||
            (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
        {
            ImageList_Remove(hImageListLarge, item.iItem);
            ImageList_Remove(hImageListSmall, item.iItem);

            SendMessageW(hApplicationPageListCtrl, LVM_DELETEITEM, item.iItem, 0);
            HeapFree(GetProcessHeap(), 0, pAPLI);
            bItemRemoved = TRUE;
        }
    }

    /*
     * If an item was removed from the list then
     * we need to resync all the items with the
     * image list
     */
    if (bItemRemoved)
    {
        count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
        for (i=0; i<count; i++)
        {
            memset(&item, 0, sizeof(LV_ITEMW));
            item.mask = LVIF_IMAGE;
            item.iItem = i;
            item.iImage = i;
            SendMessageW(hApplicationPageListCtrl, LVM_SETITEMW, 0, (LPARAM) &item);
        }
    }

    ApplicationPageUpdate();
}

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    HICON hIcon;
    WCHAR wszText[256];
    BOOL  bLargeIcon = TaskManagerSettings.View_LargeIcons;

    /* Skip our window */
    if (hWnd == hMainWnd)
        return TRUE;

    /* Check and see if this is a top-level app window */
    if (!GetWindowTextW(hWnd, wszText, ARRAY_SIZE(wszText)) || !IsWindowVisible(hWnd) ||
        (GetParent(hWnd) != NULL) || (GetWindow(hWnd, GW_OWNER) != NULL) ||
        (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW))
    {
        return TRUE; /* Skip this window */
    }

    /* Get the icon for this window */
    hIcon = NULL;
    SendMessageTimeoutW(hWnd, WM_GETICON, bLargeIcon ? ICON_BIG /*1*/ : ICON_SMALL /*0*/, 0, 0, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
    {
        hIcon = (HICON)GetClassLongPtrW(hWnd, bLargeIcon ? GCLP_HICON : GCLP_HICONSM);
        if (!hIcon) hIcon = (HICON)GetClassLongPtrW(hWnd, bLargeIcon ? GCLP_HICONSM : GCLP_HICON);
        if (!hIcon) SendMessageTimeoutW(hWnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (PDWORD_PTR)&hIcon);
        if (!hIcon) SendMessageTimeoutW(hWnd, WM_GETICON, bLargeIcon ? ICON_SMALL /*0*/ : ICON_BIG /*1*/, 0, 0, 1000, (PDWORD_PTR)&hIcon);
    }

    if (!hIcon)
        hIcon = LoadIconW(hInst, bLargeIcon ? MAKEINTRESOURCEW(IDI_WINDOW) : MAKEINTRESOURCEW(IDI_WINDOWSM));

    AddOrUpdateHwnd(hWnd, wszText, hIcon, IsHungAppWindow(hWnd));

    return TRUE;
}

static DWORD WINAPI ApplicationPageRefreshThread(void *lpParameter)
{
    /* Create the event */
    hApplicationPageEvent = CreateEventW(NULL, TRUE, TRUE, NULL);

    /* If we couldn't create the event then exit the thread */
    if (!hApplicationPageEvent)
        return 0;

    while (1)
    {
        DWORD   dwWaitVal;

        /* Wait on the event */
        dwWaitVal = WaitForSingleObject(hApplicationPageEvent, INFINITE);

        /* If the wait failed then the event object must have been */
        /* closed and the task manager is exiting so exit this thread */
        if (dwWaitVal == WAIT_FAILED)
            return 0;

        if (dwWaitVal == WAIT_OBJECT_0)
        {
            /* Reset our event */
            ResetEvent(hApplicationPageEvent);

            /*
             * FIXME:
             *
             * Should this be EnumDesktopWindows() instead?
             */
            EnumWindows(EnumWindowsProc, 0);
        }
    }
}

static void ApplicationPageShowContextMenu1(void)
{
    HMENU   hMenu;
    HMENU   hSubMenu;
    POINT   pt;

    GetCursorPos(&pt);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATION_PAGE_CONTEXT1));
    hSubMenu = GetSubMenu(hMenu, 0);

    if (TaskManagerSettings.View_LargeIcons)
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);
    else if (TaskManagerSettings.View_SmallIcons)
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);
    else
        CheckMenuRadioItem(hSubMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hMenu);
}

static void ApplicationPageShowContextMenu2(void)
{
    HMENU   hMenu;
    HMENU   hSubMenu;
    UINT    count;
    POINT   pt;

    GetCursorPos(&pt);

    hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATION_PAGE_CONTEXT2));
    hSubMenu = GetSubMenu(hMenu, 0);

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0);
    if (count == 1)
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_ENABLED);
    }
    else if (count > 1)
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEHORIZONTALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_TILEVERTICALLY, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MINIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_MAXIMIZE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_CASCADE, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
        EnableMenuItem(hSubMenu, ID_WINDOWS_BRINGTOFRONT, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
    }

    SetMenuDefaultItem(hSubMenu, ID_APPLICATION_PAGE_SWITCHTO, MF_BYCOMMAND);

    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON, pt.x, pt.y, 0, hMainWnd, NULL);

    DestroyMenu(hMenu);
}

static int CALLBACK ApplicationPageCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPAPPLICATION_PAGE_LIST_ITEM Param1;
    LPAPPLICATION_PAGE_LIST_ITEM Param2;

    if (bSortAscending) {
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
    } else {
        Param1 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam2;
        Param2 = (LPAPPLICATION_PAGE_LIST_ITEM)lParam1;
    }
    return lstrcmpW(Param1->wszTitle, Param2->wszTitle);
}

static void ApplicationPageOnNotify(WPARAM wParam, LPARAM lParam)
{
    LPNMHDR         pnmh;
    LV_DISPINFOW*   pnmdi;
    LPAPPLICATION_PAGE_LIST_ITEM pAPLI;
    WCHAR    wszNotResponding[255];
    WCHAR    wszRunning[255];

    LoadStringW(hInst, IDS_APPLICATION_NOT_RESPONDING, wszNotResponding, ARRAY_SIZE(wszNotResponding));
    LoadStringW(hInst, IDS_APPLICATION_RUNNING, wszRunning, ARRAY_SIZE(wszRunning));

    pnmh = (LPNMHDR) lParam;
    pnmdi = (LV_DISPINFOW*) lParam;

    if (pnmh->hwndFrom == hApplicationPageListCtrl) {
        switch (pnmh->code) {
        case LVN_ITEMCHANGED:
            ApplicationPageUpdate();
            break;
            
        case LVN_GETDISPINFOW:
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)pnmdi->item.lParam;

            /* Update the item text */
            if (pnmdi->item.iSubItem == 0)
            {
                lstrcpynW(pnmdi->item.pszText, pAPLI->wszTitle, pnmdi->item.cchTextMax);
            }

            /* Update the item status */
            else if (pnmdi->item.iSubItem == 1)
            {
                if (pAPLI->bHung)
                    lstrcpynW(pnmdi->item.pszText, wszNotResponding, pnmdi->item.cchTextMax);
                else
                    lstrcpynW(pnmdi->item.pszText, wszRunning, pnmdi->item.cchTextMax);
            }

            break;

        case NM_RCLICK:

            if (SendMessageW(hApplicationPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0) < 1)
            {
                ApplicationPageShowContextMenu1();
            }
            else
            {
                ApplicationPageShowContextMenu2();
            }

            break;

        case NM_DBLCLK:

            ApplicationPage_OnSwitchTo();

            break;
        }
    }
    else if (pnmh->hwndFrom == (HWND)SendMessageW(hApplicationPageListCtrl, LVM_GETHEADER, 0, 0))
    {
        switch (pnmh->code)
        {
        case NM_RCLICK:

            if (SendMessageW(hApplicationPageListCtrl, LVM_GETSELECTEDCOUNT, 0, 0) < 1)
            {
                ApplicationPageShowContextMenu1();
            }
            else
            {
                ApplicationPageShowContextMenu2();
            }

            break;

        case HDN_ITEMCLICKW:

            SendMessageW(hApplicationPageListCtrl, LVM_SORTITEMS, 0, (LPARAM) ApplicationPageCompareFunc);
            bSortAscending = !bSortAscending;

            break;
        }
    }

}

void RefreshApplicationPage(void)
{
    /* Signal the event so that our refresh thread */
    /* will wake up and refresh the application page */
    SetEvent(hApplicationPageEvent);
}

static void UpdateApplicationListControlViewSetting(void)
{
    DWORD   dwStyle = GetWindowLongW(hApplicationPageListCtrl, GWL_STYLE);

    dwStyle &= ~LVS_REPORT;
    dwStyle &= ~LVS_ICON;
    dwStyle &= ~LVS_LIST;
    dwStyle &= ~LVS_SMALLICON;

    if (TaskManagerSettings.View_LargeIcons)
        dwStyle |= LVS_ICON;
    else if (TaskManagerSettings.View_SmallIcons)
        dwStyle |= LVS_SMALLICON;
    else
        dwStyle |= LVS_REPORT;

    SetWindowLongW(hApplicationPageListCtrl, GWL_STYLE, dwStyle);

    RefreshApplicationPage();
}

void ApplicationPage_OnViewLargeIcons(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = TRUE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = FALSE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_LARGE, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnViewSmallIcons(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = TRUE;
    TaskManagerSettings.View_Details = FALSE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_SMALL, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnViewDetails(void)
{
    HMENU   hMenu;
    HMENU   hViewMenu;

    hMenu = GetMenu(hMainWnd);
    hViewMenu = GetSubMenu(hMenu, 2);

    TaskManagerSettings.View_LargeIcons = FALSE;
    TaskManagerSettings.View_SmallIcons = FALSE;
    TaskManagerSettings.View_Details = TRUE;
    CheckMenuRadioItem(hViewMenu, ID_VIEW_LARGE, ID_VIEW_DETAILS, ID_VIEW_DETAILS, MF_BYCOMMAND);

    UpdateApplicationListControlViewSetting();
}

void ApplicationPage_OnWindowsTileHorizontally(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;
    HWND*                           hWndArray;
    int                             nWndCount;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    hWndArray = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(HWND) * count);
    nWndCount = 0;

    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);

        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;

            if (pAPLI) {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }
    TileWindows(NULL, MDITILE_HORIZONTAL, NULL, nWndCount, hWndArray);
    HeapFree(GetProcessHeap(), 0, hWndArray);
}

void ApplicationPage_OnWindowsTileVertically(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;
    HWND*                           hWndArray;
    int                             nWndCount;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    hWndArray = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(HWND) * count);
    nWndCount = 0;

    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);

        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }

    TileWindows(NULL, MDITILE_VERTICAL, NULL, nWndCount, hWndArray);
    HeapFree(GetProcessHeap(), 0, hWndArray);
}

void ApplicationPage_OnWindowsMinimize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                ShowWindow(pAPLI->hWnd, SW_MINIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsMaximize(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                ShowWindow(pAPLI->hWnd, SW_MAXIMIZE);
            }
        }
    }
}

void ApplicationPage_OnWindowsCascade(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;
    HWND*                           hWndArray;
    int                             nWndCount;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    hWndArray = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(HWND) * count);
    nWndCount = 0;

    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                hWndArray[nWndCount] = pAPLI->hWnd;
                nWndCount++;
            }
        }
    }
    CascadeWindows(NULL, 0, NULL, nWndCount, hWndArray);
    HeapFree(GetProcessHeap(), 0, hWndArray);
}

void ApplicationPage_OnWindowsBringToFront(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        if (IsIconic(pAPLI->hWnd))
            ShowWindow(pAPLI->hWnd, SW_RESTORE);
        BringWindowToTop(pAPLI->hWnd);
    }
}

void ApplicationPage_OnSwitchTo(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);

        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        SwitchToThisWindow(pAPLI->hWnd, TRUE);
        if (TaskManagerSettings.MinimizeOnUse)
            ShowWindow(hMainWnd, SW_MINIMIZE);
    }
}

void ApplicationPage_OnEndTask(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM    pAPLI = NULL;
    LV_ITEMW                        item;
    int                             i, count;

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            if (pAPLI) {
                PostMessageW(pAPLI->hWnd, WM_CLOSE, 0, 0);
            }
        }
    }
}

void ApplicationPage_OnGotoProcess(void)
{
    LPAPPLICATION_PAGE_LIST_ITEM pAPLI = NULL;
    LV_ITEMW item;
    int i, count;
    /* NMHDR nmhdr; */

    count = SendMessageW(hApplicationPageListCtrl, LVM_GETITEMCOUNT, 0, 0);
    for (i=0; i<count; i++) {
        memset(&item, 0, sizeof(LV_ITEMW));
        item.mask = LVIF_STATE|LVIF_PARAM;
        item.iItem = i;
        item.stateMask = (UINT)-1;
        SendMessageW(hApplicationPageListCtrl, LVM_GETITEMW, 0, (LPARAM) &item);
        if (item.state & LVIS_SELECTED) {
            pAPLI = (LPAPPLICATION_PAGE_LIST_ITEM)item.lParam;
            break;
        }
    }
    if (pAPLI) {
        DWORD   dwProcessId;

        GetWindowThreadProcessId(pAPLI->hWnd, &dwProcessId);
        /*
         * Switch to the process tab
         */
        SendMessageW(hTabWnd, TCM_SETCURFOCUS, 1, 0);
        /*
         * FIXME: Select the process item in the list
         */
    }
}

INT_PTR CALLBACK
ApplicationPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT        rc;
    int         nXDifference;
    int         nYDifference;
    int         cx, cy;
    LVCOLUMNW   column;

    WCHAR wszTask[255];
    WCHAR wszStatus[255];

    LoadStringW(hInst, IDS_APPLICATION_TASK, wszTask, ARRAY_SIZE(wszTask));
    LoadStringW(hInst, IDS_APPLICATION_STATUS, wszStatus, ARRAY_SIZE(wszStatus));

    switch (message) {
    case WM_INITDIALOG:

        /* Save the width and height */
        GetClientRect(hDlg, &rc);
        nApplicationPageWidth = rc.right;
        nApplicationPageHeight = rc.bottom;

        /* Update window position */
        SetWindowPos(hDlg, NULL, 15, 30, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);

        /* Get handles to the controls */
        hApplicationPageListCtrl = GetDlgItem(hDlg, IDC_APPLIST);
        hApplicationPageEndTaskButton = GetDlgItem(hDlg, IDC_ENDTASK);
        hApplicationPageSwitchToButton = GetDlgItem(hDlg, IDC_SWITCHTO);
        hApplicationPageNewTaskButton = GetDlgItem(hDlg, IDC_NEWTASK);

        /* Initialize the application page's controls */
        column.mask = LVCF_TEXT|LVCF_WIDTH;
        column.pszText = wszTask;
        column.cx = 250;
        /* Add the "Task" column */
        SendMessageW(hApplicationPageListCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM) &column);
        column.mask = LVCF_TEXT|LVCF_WIDTH;
        column.pszText = wszStatus;
        column.cx = 95;
        /* Add the "Status" column */
        SendMessageW(hApplicationPageListCtrl, LVM_INSERTCOLUMNW, 1, (LPARAM) &column);

        SendMessageW(hApplicationPageListCtrl, LVM_SETIMAGELIST, LVSIL_SMALL,
                    (LPARAM) ImageList_Create(16, 16, ILC_COLOR8|ILC_MASK, 0, 1));
        SendMessageW(hApplicationPageListCtrl, LVM_SETIMAGELIST, LVSIL_NORMAL,
                    (LPARAM) ImageList_Create(32, 32, ILC_COLOR8|ILC_MASK, 0, 1));

        UpdateApplicationListControlViewSetting();

        /* Start our refresh thread */
        CloseHandle( CreateThread(NULL, 0, ApplicationPageRefreshThread, NULL, 0, NULL));

        return TRUE;

    case WM_DESTROY:
        /* Close the event handle, this will make the */
        /* refresh thread exit when the wait fails */
        CloseHandle(hApplicationPageEvent);
        break;

    case WM_COMMAND:

        /* Handle the button clicks */
        switch (LOWORD(wParam))
        {
        case IDC_ENDTASK:
            ApplicationPage_OnEndTask();
            break;
        case IDC_SWITCHTO:
            ApplicationPage_OnSwitchTo();
            break;
        case IDC_NEWTASK:
            SendMessageW(hMainWnd, WM_COMMAND, MAKEWPARAM(ID_FILE_NEW, 0), 0);
            break;
        }

        break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
        nXDifference = cx - nApplicationPageWidth;
        nYDifference = cy - nApplicationPageHeight;
        nApplicationPageWidth = cx;
        nApplicationPageHeight = cy;

        /* Reposition the application page's controls */
        GetWindowRect(hApplicationPageListCtrl, &rc);
        cx = (rc.right - rc.left) + nXDifference;
        cy = (rc.bottom - rc.top) + nYDifference;
        SetWindowPos(hApplicationPageListCtrl, NULL, 0, 0, cx, cy, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageListCtrl, NULL, TRUE);

        GetClientRect(hApplicationPageEndTaskButton, &rc);
        MapWindowPoints(hApplicationPageEndTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageEndTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageEndTaskButton, NULL, TRUE);

        GetClientRect(hApplicationPageSwitchToButton, &rc);
        MapWindowPoints(hApplicationPageSwitchToButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageSwitchToButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageSwitchToButton, NULL, TRUE);

        GetClientRect(hApplicationPageNewTaskButton, &rc);
        MapWindowPoints(hApplicationPageNewTaskButton, hDlg, (LPPOINT)(&rc), (sizeof(RECT)/sizeof(POINT)) );
        cx = rc.left + nXDifference;
        cy = rc.top + nYDifference;
        SetWindowPos(hApplicationPageNewTaskButton, NULL, cx, cy, 0, 0, SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
        InvalidateRect(hApplicationPageNewTaskButton, NULL, TRUE);

        break;

    case WM_NOTIFY:
        ApplicationPageOnNotify(wParam, lParam);
        break;

    }

  return 0;
}
