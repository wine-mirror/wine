/*
 * Win32 waitable timers
 *
 * Copyright 1999 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>
#include "winerror.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "file.h"  /* for FILETIME routines */
#include "wine/server.h"


/***********************************************************************
 *           CreateWaitableTimerA    (KERNEL32.@)
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
    SERVER_START_VAR_REQ( create_timer, len * sizeof(WCHAR) )
    {
        req->manual  = manual;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( create_timer, len * sizeof(WCHAR) )
    {
        req->manual  = manual;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SetLastError(0);
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
}


/***********************************************************************
 *           OpenWaitableTimerA    (KERNEL32.@)
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
    SERVER_START_VAR_REQ( open_timer, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        if (len) MultiByteToWideChar( CP_ACP, 0, name, strlen(name), server_data_ptr(req), len );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
    return ret;
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
    SERVER_START_VAR_REQ( open_timer, len * sizeof(WCHAR) )
    {
        req->access  = access;
        req->inherit = inherit;
        memcpy( server_data_ptr(req), name, len * sizeof(WCHAR) );
        SERVER_CALL_ERR();
        ret = req->handle;
    }
    SERVER_END_VAR_REQ;
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
            req->sec  = DOSFS_FileTimeToUnixTime( (FILETIME *)&exp, &remainder );
            req->usec = remainder / 10;  /* convert from 100-ns to us units */
        }
        req->handle   = handle;
        req->period   = period;
        req->callback = callback;
        req->arg      = arg;
        if (resume) SetLastError( ERROR_NOT_SUPPORTED ); /* set error but can still succeed */
        ret = !SERVER_CALL_ERR();
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
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}
