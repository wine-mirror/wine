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

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineboot);

#define MESSAGE_TIMEOUT     5000

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
        struct window_info *new_win = realloc( windows, new_count * sizeof(windows[0]) );
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
static int __cdecl cmp_window( const void *ptr1, const void *ptr2 )
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
    windows = malloc( win_max * sizeof(windows[0]) );
    if (!windows) return FALSE;
    if (!EnumWindows( enum_proc, 0 )) return FALSE;
    /* sort windows by processes */
    qsort( windows, win_count, sizeof(windows[0]), cmp_window );
    return TRUE;
}

struct callback_data
{
    UINT window_count;
    BOOL timed_out;
    LRESULT result;
};

static void CALLBACK end_session_message_callback( HWND hwnd, UINT msg, ULONG_PTR data, LRESULT lresult )
{
    struct callback_data *cb_data = (struct callback_data *)data;

    WINE_TRACE( "received response %s hwnd %p lresult %Id\n",
                msg == WM_QUERYENDSESSION ? "WM_QUERYENDSESSION" : (msg == WM_ENDSESSION ? "WM_ENDSESSION" : "Unknown"),
                hwnd, lresult );

    /* If the window was destroyed while the message was in its queue, SendMessageCallback()
       calls us with a default 0 result.  Ignore it. */
    if (!lresult && !IsWindow( hwnd ))
    {
        WINE_TRACE( "window was destroyed; ignoring FALSE lresult\n" );
        lresult = TRUE;
    }

    /* we only care if a WM_QUERYENDSESSION response is FALSE */
    cb_data->result = cb_data->result && lresult;

    /* cheap way of ref-counting callback_data whilst freeing memory at correct
     * time */
    if (!(cb_data->window_count--) && cb_data->timed_out)
        free( cb_data );
}

struct endtask_dlg_data
{
    struct window_info *win;
    BOOL cancelled;
    BOOL terminated;
};

static INT_PTR CALLBACK endtask_dlg_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct endtask_dlg_data *data;
    HANDLE handle;

    switch (msg)
    {
    case WM_INITDIALOG:
        SetWindowLongPtrW( hwnd, DWLP_USER, lparam );
        ShowWindow( hwnd, SW_SHOWNORMAL );
        return TRUE;
    case WM_COMMAND:
        data = (struct endtask_dlg_data *)GetWindowLongPtrW( hwnd, DWLP_USER );
        switch (wparam)
        {
        case MAKEWPARAM(IDOK, BN_CLICKED):
            handle = OpenProcess( PROCESS_TERMINATE, FALSE, data->win[0].pid );
            if (handle)
            {
                WINE_TRACE( "terminating process %04lx\n", data->win[0].pid );
                TerminateProcess( handle, 0 );
                CloseHandle( handle );
                data->terminated = TRUE;
            }
            return TRUE;
        case MAKEWPARAM(IDCANCEL, BN_CLICKED):
            data->cancelled = TRUE;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/* Sends a message to a set of windows, displaying a dialog if the window
 * doesn't respond to the message within a set amount of time.
 * If the process has already been terminated, the function returns -1.
 * If the user or application cancels the process, the function returns 0.
 * Otherwise the function returns 0. */
static LRESULT send_messages_with_timeout_dialog(
    struct window_info *win, UINT count, HANDLE process_handle,
    UINT msg, WPARAM wparam, LPARAM lparam )
{
    unsigned int i;
    DWORD ret;
    DWORD start_time;
    struct callback_data *cb_data;
    HWND hwnd_endtask = NULL;
    struct endtask_dlg_data dlg_data;
    LRESULT result;

    cb_data = malloc( sizeof(*cb_data) );
    if (!cb_data)
        return 1;

    cb_data->result = TRUE; /* we only care if a WM_QUERYENDSESSION response is FALSE */
    cb_data->timed_out = FALSE;
    cb_data->window_count = count;

    dlg_data.win = win;
    dlg_data.terminated = FALSE;
    dlg_data.cancelled = FALSE;

    for (i = 0; i < count; i++)
    {
        if (!SendMessageCallbackW( win[i].hwnd, msg, wparam, lparam,
                                   end_session_message_callback, (ULONG_PTR)cb_data ))
            cb_data->window_count --;
    }

    start_time = GetTickCount();
    while (TRUE)
    {
        DWORD current_time = GetTickCount();

        ret = MsgWaitForMultipleObjects( 1, &process_handle, FALSE,
                                         MESSAGE_TIMEOUT - (current_time - start_time),
                                         QS_ALLINPUT );
        if (ret == WAIT_OBJECT_0) /* process exited */
        {
            free( cb_data );
            result = 1;
            goto cleanup;
        }
        else if (ret == WAIT_OBJECT_0 + 1) /* window message */
        {
            MSG msg;
            while(PeekMessageW( &msg, NULL, 0, 0, PM_REMOVE ))
            {
                if (!hwnd_endtask || !IsDialogMessageW( hwnd_endtask, &msg ))
                {
                    TranslateMessage( &msg );
                    DispatchMessageW( &msg );
                }
            }
            if (!cb_data->window_count)
            {
                result = dlg_data.terminated || cb_data->result;
                free( cb_data );
                if (!result)
                    goto cleanup;
                break;
            }
            if (dlg_data.cancelled)
            {
                cb_data->timed_out = TRUE;
                result = 0;
                goto cleanup;
            }
        }
        else if ((ret == WAIT_TIMEOUT) && !hwnd_endtask)
        {
            hwnd_endtask = CreateDialogParamW( GetModuleHandleW(NULL),
                                               MAKEINTRESOURCEW(IDD_ENDTASK),
                                               NULL, endtask_dlg_proc,
                                               (LPARAM)&dlg_data );
        }
        else break;
    }

    result = 1;

cleanup:
    if (hwnd_endtask) DestroyWindow( hwnd_endtask );
    return result;
}

/* send WM_QUERYENDSESSION and WM_ENDSESSION to all windows of a given process */
static DWORD_PTR send_end_session_messages( struct window_info *win, UINT count, UINT flags )
{
    LRESULT result, end_session;
    HANDLE process_handle;
    DWORD ret;

    /* FIXME: Use flags to implement EWX_FORCEIFHUNG! */
    /* don't kill the desktop process */
    if (win[0].pid == desktop_pid) return 1;

    process_handle = OpenProcess( SYNCHRONIZE, FALSE, win[0].pid );
    if (!process_handle)
        return 1;

    end_session = send_messages_with_timeout_dialog( win, count, process_handle,
                                                     WM_QUERYENDSESSION, 0, 0 );
    if (end_session == -1)
    {
        CloseHandle( process_handle );
        return 1;
    }

    result = send_messages_with_timeout_dialog( win, count, process_handle,
                                                WM_ENDSESSION, end_session, 0 );
    if (end_session == 0)
    {
        CloseHandle( process_handle );
        return 0;
    }
    if (result == -1)
    {
        CloseHandle( process_handle );
        return 1;
    }

    /* Check whether the app quit on its own */
    ret = WaitForSingleObject( process_handle, 0 );
    CloseHandle( process_handle );
    if (ret == WAIT_TIMEOUT)
    {
        /* If not, it returned from all WM_ENDSESSION and is finished cleaning
         * up, so we can safely kill the process. */
        HANDLE handle = OpenProcess( PROCESS_TERMINATE, FALSE, win[0].pid );
        if (handle)
        {
            WINE_TRACE( "terminating process %04lx\n", win[0].pid );
            TerminateProcess( handle, 0 );
            CloseHandle( handle );
        }
    }
    return 1;
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

    free( windows );

    return (result != 0);
}

static BOOL CALLBACK shutdown_one_desktop( LPWSTR name, LPARAM force )
{
    HDESK hdesk;

    WINE_TRACE("Shutting down desktop %s\n", wine_dbgstr_w(name));

    hdesk = OpenDesktopW( name, 0, FALSE, GENERIC_ALL );
    if (hdesk == NULL)
    {
        WINE_ERR("Cannot open desktop %s, err=%li\n", wine_dbgstr_w(name), GetLastError());
        return FALSE;
    }

    if (!SetThreadDesktop( hdesk ))
    {
        CloseDesktop( hdesk );
        WINE_ERR("Cannot set thread desktop %s, err=%li\n", wine_dbgstr_w(name), GetLastError());
        return FALSE;
    }

    CloseDesktop( hdesk );

    return shutdown_close_windows( force );
}

BOOL shutdown_all_desktops( BOOL force )
{
    BOOL ret;
    HDESK prev_desktop;

    prev_desktop = GetThreadDesktop(GetCurrentThreadId());

    ret = EnumDesktopsW( NULL, shutdown_one_desktop, (LPARAM)force );

    SetThreadDesktop(prev_desktop);

    return ret;
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
