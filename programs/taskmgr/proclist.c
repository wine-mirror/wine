/*
 *  ReactOS Task Manager
 *
 *  proclist.c
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

#include "taskmgr.h"
#include "perfdata.h"


WNDPROC                OldProcessListWndProc;


LRESULT CALLBACK ProcessListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBRUSH    hbrBackground;
    int     count;
    RECT    rcItem;
    RECT    rcClip;
    HDC        hDC;
    int        DcSave;

    switch (message)
    {
    case WM_ERASEBKGND:

        /*
         * The list control produces a nasty flicker
         * when the user is resizing the window because
         * it erases the background to white, then
         * paints the list items over it.
         *
         * We will clip the drawing so that it only
         * erases the parts of the list control that
         * show only the background.
         */

        /*
         * Get the device context and save its state
         * to be restored after we're done
         */
        hDC = (HDC) wParam;
        DcSave = SaveDC(hDC);

        /*
         * Get the background brush
         */
        hbrBackground = (HBRUSH) GetClassLongPtrW(hWnd, GCLP_HBRBACKGROUND);

        /*
         * Calculate the clip rect by getting the RECT
         * of the first and last items and adding them up.
         *
         * We also have to get the item's icon RECT and
         * subtract it from our clip rect because we don't
         * use icons in this list control.
         */
        rcClip.left = LVIR_BOUNDS;
        SendMessageW(hWnd, LVM_GETITEMRECT, 0, (LPARAM) &rcClip);
        rcItem.left = LVIR_BOUNDS;
        count = SendMessageW(hWnd, LVM_GETITEMCOUNT, 0, 0);
        SendMessageW(hWnd, LVM_GETITEMRECT, count - 1, (LPARAM) &rcItem);

        rcClip.bottom = rcItem.bottom;
        rcItem.left = LVIR_ICON;
        SendMessageW(hWnd, LVM_GETITEMRECT, 0, (LPARAM) &rcItem);
        rcClip.left = rcItem.right;

        /*
         * Now exclude the clip rect
         */
        ExcludeClipRect(hDC, rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

        /*
         * Now erase the background
         *
         *
         * FIXME: Should I erase it myself or
         * pass down the updated HDC and let
         * the default handler do it?
         */
        GetClientRect(hWnd, &rcItem);
        FillRect(hDC, &rcItem, hbrBackground);

        /*
         * Now restore the DC state that we
         * saved earlier
         */
        RestoreDC(hDC, DcSave);
        
        return TRUE;
    }

    /*
     * We pass on all messages except WM_ERASEBKGND
     */
    return CallWindowProcW(OldProcessListWndProc, hWnd, message, wParam, lParam);
}
