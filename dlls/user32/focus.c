/*
 * Focus and activation functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1995 Alex Korobka
 * Copyright 1994, 2002 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "win.h"
#include "imm.h"
#include "user_private.h"
#include "wine/server.h"


/*******************************************************************
 *		FOCUS_MouseActivate
 *
 * Activate a window as a result of a mouse click
 */
BOOL FOCUS_MouseActivate( HWND hwnd )
{
    return NtUserCallHwndParam( hwnd, TRUE, NtUserSetForegroundWindow );
}


/*******************************************************************
 *		SetForegroundWindow  (USER32.@)
 */
BOOL WINAPI SetForegroundWindow( HWND hwnd )
{
    return NtUserCallHwndParam( hwnd, FALSE, NtUserSetForegroundWindow );
}


/*******************************************************************
 *		GetActiveWindow  (USER32.@)
 */
HWND WINAPI GetActiveWindow(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndActive : 0;
}


/*****************************************************************
 *		GetFocus  (USER32.@)
 */
HWND WINAPI GetFocus(void)
{
    GUITHREADINFO info;
    info.cbSize = sizeof(info);
    return NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ) ? info.hwndFocus : 0;
}


/***********************************************************************
*		SetShellWindowEx (USER32.@)
* hwndShell =    Progman[Program Manager]
*                |-> SHELLDLL_DefView
* hwndListView = |   |-> SysListView32
*                |   |   |-> tooltips_class32
*                |   |
*                |   |-> SysHeader32
*                |
*                |-> ProxyTarget
*/
BOOL WINAPI SetShellWindowEx(HWND hwndShell, HWND hwndListView)
{
    BOOL ret;

    if (GetShellWindow())
        return FALSE;

    if (GetWindowLongW(hwndShell, GWL_EXSTYLE) & WS_EX_TOPMOST)
        return FALSE;

    if (hwndListView != hwndShell)
        if (GetWindowLongW(hwndListView, GWL_EXSTYLE) & WS_EX_TOPMOST)
            return FALSE;

    if (hwndListView && hwndListView!=hwndShell)
        SetWindowPos(hwndListView, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

    SetWindowPos(hwndShell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

    SERVER_START_REQ(set_global_windows)
    {
        req->flags          = SET_GLOBAL_SHELL_WINDOWS;
        req->shell_window   = wine_server_user_handle( hwndShell );
        req->shell_listview = wine_server_user_handle( hwndListView );
        ret = !wine_server_call_err(req);
    }
    SERVER_END_REQ;

    return ret;
}


/*******************************************************************
*		SetShellWindow (USER32.@)
*/
BOOL WINAPI SetShellWindow(HWND hwndShell)
{
    return SetShellWindowEx(hwndShell, hwndShell);
}


/*******************************************************************
*		GetShellWindow (USER32.@)
*/
HWND WINAPI GetShellWindow(void)
{
    HWND hwndShell = 0;

    SERVER_START_REQ(set_global_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            hwndShell = wine_server_ptr_handle( reply->old_shell_window );
    }
    SERVER_END_REQ;

    return hwndShell;
}


/***********************************************************************
 *		SetProgmanWindow (USER32.@)
 */
HWND WINAPI SetProgmanWindow ( HWND hwnd )
{
    SERVER_START_REQ(set_global_windows)
    {
        req->flags          = SET_GLOBAL_PROGMAN_WINDOW;
        req->progman_window = wine_server_user_handle( hwnd );
        if (wine_server_call_err( req )) hwnd = 0;
    }
    SERVER_END_REQ;
    return hwnd;
}


/***********************************************************************
 *		GetProgmanWindow (USER32.@)
 */
HWND WINAPI GetProgmanWindow(void)
{
    HWND ret = 0;

    SERVER_START_REQ(set_global_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            ret = wine_server_ptr_handle( reply->old_progman_window );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		SetTaskmanWindow (USER32.@)
 * NOTES
 *   hwnd = MSTaskSwWClass
 *          |-> SysTabControl32
 */
HWND WINAPI SetTaskmanWindow ( HWND hwnd )
{
    SERVER_START_REQ(set_global_windows)
    {
        req->flags          = SET_GLOBAL_TASKMAN_WINDOW;
        req->taskman_window = wine_server_user_handle( hwnd );
        if (wine_server_call_err( req )) hwnd = 0;
    }
    SERVER_END_REQ;
    return hwnd;
}

/***********************************************************************
 *		GetTaskmanWindow (USER32.@)
 */
HWND WINAPI GetTaskmanWindow(void)
{
    HWND ret = 0;

    SERVER_START_REQ(set_global_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            ret = wine_server_ptr_handle( reply->old_taskman_window );
    }
    SERVER_END_REQ;
    return ret;
}
