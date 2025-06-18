/*
 *  ReactOS Task Manager
 *
 *  optnmenu.c
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

#include "taskmgr.h"

void TaskManager_OnOptionsAlwaysOnTop(void)
{
    HMENU    hMenu;
    HMENU    hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the always on top menu item
     * and update main window.
     */
    if ((GetWindowLongW(hMainWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0)
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_UNCHECKED);
        TaskManagerSettings.AlwaysOnTop = FALSE;
        SetWindowPos(hMainWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }
    else
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_ALWAYSONTOP, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.AlwaysOnTop = TRUE;
        SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }
}

void TaskManager_OnOptionsMinimizeOnUse(void)
{
    HMENU    hMenu;
    HMENU    hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the minimize on use menu item.
     */
    if (GetMenuState(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND) & MF_CHECKED)
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_UNCHECKED);
        TaskManagerSettings.MinimizeOnUse = FALSE;
    }
    else
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_MINIMIZEONUSE, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.MinimizeOnUse = TRUE;
    }
}

void TaskManager_OnOptionsHideWhenMinimized(void)
{
    HMENU    hMenu;
    HMENU    hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * Check or uncheck the hide when minimized menu item.
     */
    if (GetMenuState(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND) & MF_CHECKED)
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_UNCHECKED);
        TaskManagerSettings.HideWhenMinimized = FALSE;
    }
    else
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_HIDEWHENMINIMIZED, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.HideWhenMinimized = TRUE;
    }
}

void TaskManager_OnOptionsShow16BitTasks(void)
{
    HMENU    hMenu;
    HMENU    hOptionsMenu;

    hMenu = GetMenu(hMainWnd);
    hOptionsMenu = GetSubMenu(hMenu, OPTIONS_MENU_INDEX);

    /*
     * FIXME: Currently this is useless because the
     * current implementation doesn't list the 16-bit
     * processes. I believe that would require querying
     * each ntvdm.exe process for its children.
     */

    /*
     * Check or uncheck the show 16-bit tasks menu item
     */
    if (GetMenuState(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND) & MF_CHECKED)
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_UNCHECKED);
        TaskManagerSettings.Show16BitTasks = FALSE;
    }
    else
    {
        CheckMenuItem(hOptionsMenu, ID_OPTIONS_SHOW16BITTASKS, MF_BYCOMMAND|MF_CHECKED);
        TaskManagerSettings.Show16BitTasks = TRUE;
    }

    /*
     * Refresh the list of processes.
     */
    RefreshProcessPage();
}
