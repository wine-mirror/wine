/*
 * Win32 waitable timers
 *
 * Copyright 1999 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "wine/unicode.h"
#include "file.h"  /* for FILETIME routines */
#include "server.h"


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.861)
 */
HANDLE WINAPI CreateWaitableTimerA( SECURITY_ATTRIBUTES *sa, BOOL manual, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct create_timer_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->manual  = manual;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        server_call( REQ_CREATE_TIMER );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           CreateWaitableTimerW    (KERNEL32.862)
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
    SERVER_START_REQ
    {
        struct create_timer_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->manual  = manual;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        server_call( REQ_CREATE_TIMER );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenWaitableTimerA    (KERNEL32.881)
 */
HANDLE WINAPI OpenWaitableTimerA( DWORD access, BOOL inherit, LPCSTR name )
{
    HANDLE ret;
    DWORD len = name ? MultiByteToWideChar( CP_ACP, 0, name, strlen(name), NULL, 0 ) : 0;
    if (len >= MAX_PATH)
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    SERVER_START_REQ
    {
        struct open_timer_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        server_call( REQ_OPEN_TIMER );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           OpenWaitableTimerW    (KERNEL32.882)
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
    SERVER_START_REQ
    {
        struct open_timer_request *req = server_alloc_req( sizeof(*req), len * sizeof(WCHAR) );

        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        server_call( REQ_OPEN_TIMER );
        ret = req->handle;
    }
    SERVER_END_REQ;
    if (ret == -1) ret = 0; /* must return 0 on failure, not -1 */
    return ret;
}


/***********************************************************************
 *           SetWaitableTimer    (KERNEL32.894)
 */
BOOL WINAPI SetWaitableTimer( HANDLE handle, const LARGE_INTEGER *when, LONG period,
                              PTIMERAPCROUTINE callback, LPVOID arg, BOOL resume )
{
    BOOL ret;
    FILETIME ft;
    DWORD remainder;

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

    SERVER_START_REQ
    {
        struct set_timer_request *req = server_alloc_req( sizeof(*req), 0 );

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
        ret = !server_call( REQ_SET_TIMER );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           CancelWaitableTimer    (KERNEL32.857)
 */
BOOL WINAPI CancelWaitableTimer( HANDLE handle )
{
    BOOL ret;
    SERVER_START_REQ
    {
        struct cancel_timer_request *req = server_alloc_req( sizeof(*req), 0 );
        req->handle = handle;
        ret = !server_call( REQ_CANCEL_TIMER );
    }
    SERVER_END_REQ;
    return ret;
}
