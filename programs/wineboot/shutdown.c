/*
 * Copyright (C) 2006 Alexandre Julliard
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "tlhelp32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineboot);

struct window_info
{
    HWND  hwnd;
    DWORD pid;
    DWORD tid;
};

static UINT win_count;
static UINT win_max;
static struct window_info *windows;
static DWORD desktop_pid;

/* store a new window; callback for EnumWindows */
static BOOL CALLBACK enum_proc( HWND hwnd, LPARAM lp )
{
    if (win_count >= win_max)
    {
        UINT new_count = win_max * 2;
        struct window_info *new_win = HeapReAlloc( GetProcessHeap(), 0, windows,
                                                   new_count * sizeof(windows[0]) );
        if (!new_win) return FALSE;
        windows = new_win;
        win_max = new_count;
    }
    windows[win_count].hwnd = hwnd;
    windows[win_count].tid = GetWindowThreadProcessId( hwnd, &windows[win_count].pid );
    win_count++;
    return TRUE;
}

/* compare two window info structures; callback for qsort */
static int cmp_window( const void *ptr1, const void *ptr2 )
{
    const struct window_info *info1 = ptr1;
    const struct window_info *info2 = ptr2;
    int ret = info1->pid - info2->pid;
    if (!ret) ret = info1->tid - info2->tid;
    return ret;
}

/* build the list of all windows (FIXME: handle multiple desktops) */
static BOOL get_all_windows(void)
{
    win_count = 0;
    win_max = 16;
    windows = HeapAlloc( GetProcessHeap(), 0, win_max * sizeof(windows[0]) );
    if (!windows) return FALSE;
    if (!EnumWindows( enum_proc, 0 )) return FALSE;
    /* sort windows by processes */
    qsort( windows, win_count, sizeof(windows[0]), cmp_window );
    return TRUE;
}

/* send WM_QUERYENDSESSION and WM_ENDSESSION to all windows of a given process */
/* FIXME: should display a confirmation dialog if process doesn't respond to the messages */
static DWORD_PTR send_end_session_messages( struct window_info *win, UINT count, UINT flags )
{
    unsigned int i;
    DWORD_PTR result, ret = 1;

    /* don't kill the desktop process */
    if (win[0].pid == desktop_pid) return 1;

    for (i = 0; ret && i < count; i++)
    {
        if (SendMessageTimeoutW( win[i].hwnd, WM_QUERYENDSESSION, 0, 0, flags, 0, &result ))
        {
            WINE_TRACE( "sent MW_QUERYENDSESSION hwnd %p pid %04lx result %ld\n",
                        win[i].hwnd, win[i].pid, result );
            ret = result;
        }
        else win[i].hwnd = 0;  /* ignore this window */
    }

    for (i = 0; i < count; i++)
    {
        if (!win[i].hwnd) continue;
        WINE_TRACE( "sending WM_ENDSESSION hwnd %p pid %04lx wp %ld\n", win[i].hwnd, win[i].pid, ret );
        SendMessageTimeoutW( win[i].hwnd, WM_ENDSESSION, ret, 0, flags, 0, &result );
    }

    if (ret)
    {
        HANDLE handle = OpenProcess( PROCESS_TERMINATE, FALSE, win[0].pid );
        if (handle)
        {
            WINE_TRACE( "terminating process %04lx\n", win[0].pid );
            TerminateProcess( handle, 0 );
            CloseHandle( handle );
        }
    }
    return ret;
}

/* close all top-level windows and terminate processes cleanly */
BOOL shutdown_close_windows( BOOL force )
{
    UINT send_flags = force ? SMTO_ABORTIFHUNG : SMTO_NORMAL;
    DWORD_PTR result = 1;
    UINT i, n;

    if (!get_all_windows()) return FALSE;

    GetWindowThreadProcessId( GetDesktopWindow(), &desktop_pid );

    for (i = n = 0; result && i < win_count; i++, n++)
    {
        if (n && windows[i-1].pid != windows[i].pid)
        {
            result = send_end_session_messages( windows + i - n, n, send_flags );
            n = 0;
        }
    }
    if (n && result)
        result = send_end_session_messages( windows + win_count - n, n, send_flags );

    HeapFree( GetProcessHeap(), 0, windows );

    return (result != 0);
}

/* forcibly kill all processes without any cleanup */
void kill_processes( BOOL kill_desktop )
{
    BOOL res;
    UINT killed;
    HANDLE handle, snapshot;
    PROCESSENTRY32W process;

    GetWindowThreadProcessId( GetDesktopWindow(), &desktop_pid );

    do
    {
        if (!(snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ))) break;

        killed = 0;
        process.dwSize = sizeof(process);
        for (res = Process32FirstW( snapshot, &process ); res; res = Process32NextW( snapshot, &process ))
        {
            if (process.th32ProcessID == GetCurrentProcessId()) continue;
            if (process.th32ProcessID == desktop_pid) continue;
            WINE_TRACE("killing process %04lx %s\n",
                       process.th32ProcessID, wine_dbgstr_w(process.szExeFile) );
            if (!(handle = OpenProcess( PROCESS_TERMINATE, FALSE, process.th32ProcessID )))
                continue;
            if (TerminateProcess( handle, 0 )) killed++;
            CloseHandle( handle );
        }
        CloseHandle( snapshot );
    } while (killed > 0);

    if (desktop_pid && kill_desktop)  /* do this last */
    {
        if ((handle = OpenProcess( PROCESS_TERMINATE, FALSE, desktop_pid )))
        {
            TerminateProcess( handle, 0 );
            CloseHandle( handle );
        }
    }
}
