/*
 * Win32 waitable timers
 *
 * Copyright 1999 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "file.h"  /* for FILETIME routines */
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(timer);


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerA( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWaitableTimerW( sa, manual, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWaitableTimerW( sa, manual, buffer );
}


/***********************************************************************
 *           CreateWaitableTimerW    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerW( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCWSTR name )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( create_timer )
    {
        req->manual  = manual;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           OpenWaitableTimerA    (KERNEL32.@)
 */
HANDLE WINAPI OpenWaitableTimerA( DWORD access, BOOL inherit, LPCSTR name )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return OpenWaitableTimerW( access, inherit, NULL );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return OpenWaitableTimerW( access, inherit, buffer );
}


/***********************************************************************
 *           OpenWaitableTimerW    (KERNEL32.@)
 */
HANDLE WINAPI OpenWaitableTimerW( DWORD access, BOOL inherit, LPCWSTR name )
{
    HANDLE ret;
    DWORD len = name ? strlenW(name) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ( open_timer )
    {
        req->access  = access;
        req->inherit = inherit;
        wine_server_add_data( req, name, len * sizeof(WCHAR) );
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           SetWaitableTimer    (KERNEL32.@)
 */
BOOL WINAPI SetWaitableTimer( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                              PTIMERAPCROUTINE callback, LPVOID arg, BOOL resume )
{
    BOOL ret;
    LARGE_INTEGER exp = *when;

    if (exp.s.HighPart < 0)  /* relative time */
    {
        LARGE_INTEGER now;
        NtQuerySystemTime( &now );
        exp.QuadPart = RtlLargeIntegerSubtract( now.QuadPart, exp.QuadPart );
    }

    SERVER_START_REQ( set_timer )
    {
        if (!exp.s.LowPart && !exp.s.HighPart)
        {
            /* special case to start timeout on now+period without too many calculations */
            req->sec  = 0;
            req->usec = 0;
        }
        else
        {
            DWORD remainder;
            FILETIME ft;

            ft.dwLowDateTime	= exp.s.LowPart;
            ft.dwHighDateTime	= exp.s.HighPart;
            req->sec  = DOSFS_FileTimeToUnixTime( &ft, &remainder );
            req->usec = remainder / 10;  /* convert from 100-ns to us units */
        }
        req->handle   = handle;
        req->period   = period;
        req->callback = callback;
        req->arg      = arg;
        if (resume) SetLastError( ERROR_NOT_SUPPORTED ); /* set error but can still succeed */
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           CancelWaitableTimer    (KERNEL32.@)
 */
BOOL WINAPI CancelWaitableTimer( HANDLE handle )
{
    BOOL ret;
    SERVER_START_REQ( cancel_timer )
    {
        req->handle = handle;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           CreateTimerQueue  (KERNEL32.@)
 */
HANDLE WINAPI CreateTimerQueue()
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return NULL;
}


/***********************************************************************
 *           DeleteTimerQueueEx  (KERNEL32.@)
 */
BOOL WINAPI DeleteTimerQueueEx(HANDLE TimerQueue, HANDLE CompletionEvent)
{
    FIXME("(%p, %p): stub\n", TimerQueue, CompletionEvent);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *           CreateTimerQueueTimer  (KERNEL32.@)
 *
 * Creates a timer-queue timer. This timer expires at the specified due
 * time (in ms), then after every specified period (in ms). When the timer
 * expires, the callback function is called.
 *
 * RETURNS
 *   nonzero on success or zero on faillure
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI CreateTimerQueueTimer( PHANDLE phNewTimer, HANDLE TimerQueue,
				   WAITORTIMERCALLBACK Callback, PVOID Parameter,
				   DWORD DueTime, DWORD Period, ULONG Flags )
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}

/***********************************************************************
 *           DeleteTimerQueueTimer  (KERNEL32.@)
 *
 * Cancels a timer-queue timer. 
 *
 * RETURNS
 *   nonzero on success or zero on faillure
 *
 * BUGS
 *   Unimplemented
 */
BOOL WINAPI DeleteTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer,
				   HANDLE CompletionEvent )
{
    FIXME("stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}
