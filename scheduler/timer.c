/*
 * Win32 waitable timers
 *
 * Copyright 1999 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "file.h"  /* for FILETIME routines */
#include "server.h"


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.861)
 */
HANDLE WINAPI CreateWaitableTimerA( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCSTR name )
{
    struct create_timer_request *req = get_req_buffer();

    req->manual  = manual;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyAtoW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_TIMER );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           CreateWaitableTimerW    (KERNEL32.862)
 */
HANDLE WINAPI CreateWaitableTimerW( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCWSTR name )
{
    struct create_timer_request *req = get_req_buffer();

    req->manual  = manual;
    req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
    server_strcpyW( req->name, name );
    SetLastError(0);
    server_call( REQ_CREATE_TIMER );
    if (req->handle == -1) return 0;
    return req->handle;
}


/***********************************************************************
 *           OpenWaitableTimerA    (KERNEL32.881)
 */
HANDLE WINAPI OpenWaitableTimerA( DWORD access, BOOL inherit, LPCSTR name )
{
    struct open_timer_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyAtoW( req->name, name );
    server_call( REQ_OPEN_TIMER );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *           OpenWaitableTimerW    (KERNEL32.882)
 */
HANDLE WINAPI OpenWaitableTimerW( DWORD access, BOOL inherit, LPCWSTR name )
{
    struct open_timer_request *req = get_req_buffer();

    req->access  = access;
    req->inherit = inherit;
    server_strcpyW( req->name, name );
    server_call( REQ_OPEN_TIMER );
    if (req->handle == -1) return 0; /* must return 0 on failure, not -1 */
    return req->handle;
}


/***********************************************************************
 *           SetWaitableTimer    (KERNEL32.894)
 */
BOOL WINAPI SetWaitableTimer( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                              PTIMERAPCROUTINE callback, LPVOID arg, BOOL resume )
{
    FILETIME ft;
    DWORD remainder;
    struct set_timer_request *req = get_req_buffer();

    if (when->s.HighPart < 0)  /* relative time */
    {
        DWORD low;
        GetSystemTimeAsFileTime( &ft );
        low = ft.dwLowDateTime;
        ft.dwLowDateTime -= when->s.LowPart;
        ft.dwHighDateTime -= when->s.HighPart;
        if (low < ft.dwLowDateTime) ft.dwHighDateTime--; /* overflow */
    }
    else  /* absolute time */
    {
        ft.dwLowDateTime = when->s.LowPart;
        ft.dwHighDateTime = when->s.HighPart;
    }

    if (!ft.dwLowDateTime && !ft.dwHighDateTime)
    {
        /* special case to start timeout on now+period without too many calculations */
        req->sec  = 0;
        req->usec = 0;
    }
    else
    {
        req->sec  = DOSFS_FileTimeToUnixTime( &ft, &remainder );
        req->usec = remainder / 10;  /* convert from 100-ns to us units */
    }
    req->handle   = handle;
    req->period   = period;
    req->callback = callback;
    req->arg      = arg;
    if (resume) SetLastError( ERROR_NOT_SUPPORTED ); /* set error but can still succeed */
    return !server_call( REQ_SET_TIMER );
}


/***********************************************************************
 *           CancelWaitableTimer    (KERNEL32.857)
 */
BOOL WINAPI CancelWaitableTimer( HANDLE handle )
{
    struct cancel_timer_request *req = get_req_buffer();
    req->handle = handle;
    return !server_call( REQ_CANCEL_TIMER );
}
