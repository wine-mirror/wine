/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
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
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "thread.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 * SetThreadContext [KERNEL32.@]  Sets context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadContext( HANDLE handle,           /* [in]  Handle to thread with context */
                              const CONTEXT *context ) /* [in] Address of context structure */
{
    NTSTATUS status = NtSetContextThread( handle, context );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/***********************************************************************
 * GetThreadContext [KERNEL32.@]  Retrieves context of thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadContext( HANDLE handle,     /* [in]  Handle to thread with context */
                              CONTEXT *context ) /* [out] Address of context structure */
{
    NTSTATUS status = NtGetContextThread( handle, context );
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**********************************************************************
 * SuspendThread [KERNEL32.@]  Suspends a thread.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 */
DWORD WINAPI SuspendThread( HANDLE hthread ) /* [in] Handle to the thread */
{
    DWORD ret;
    NTSTATUS status = NtSuspendThread( hthread, &ret );

    if (status)
    {
        ret = ~0U;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return ret;
}


/**********************************************************************
 * ResumeThread [KERNEL32.@]  Resumes a thread.
 *
 * Decrements a thread's suspend count.  When count is zero, the
 * execution of the thread is resumed.
 *
 * RETURNS
 *    Success: Previous suspend count
 *    Failure: 0xFFFFFFFF
 *    Already running: 0
 */
DWORD WINAPI ResumeThread( HANDLE hthread ) /* [in] Identifies thread to restart */
{
    DWORD ret;
    NTSTATUS status = NtResumeThread( hthread, &ret );

    if (status)
    {
        ret = ~0U;
        SetLastError( RtlNtStatusToDosError(status) );
    }
    return ret;
}


/**********************************************************************
 * GetThreadPriority [KERNEL32.@]  Returns priority for thread.
 *
 * RETURNS
 *    Success: Thread's priority level.
 *    Failure: THREAD_PRIORITY_ERROR_RETURN
 */
INT WINAPI GetThreadPriority(
    HANDLE hthread) /* [in] Handle to thread */
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status = NtQueryInformationThread( hthread, ThreadBasicInformation,
                                                &info, sizeof(info), NULL );

    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return THREAD_PRIORITY_ERROR_RETURN;
    }
    return info.Priority;
}


/**********************************************************************
 * SetThreadPriority [KERNEL32.@]  Sets priority for thread.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI SetThreadPriority(
    HANDLE hthread, /* [in] Handle to thread */
    INT priority)   /* [in] Thread priority level */
{
    BOOL ret;
    SERVER_START_REQ( set_thread_info )
    {
        req->handle   = hthread;
        req->priority = priority;
        req->mask     = SET_THREAD_INFO_PRIORITY;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 * GetThreadPriorityBoost [KERNEL32.@]  Returns priority boost for thread.
 *
 * Always reports that priority boost is disabled.
 *
 * RETURNS
 *    Success: TRUE.
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadPriorityBoost(
    HANDLE hthread, /* [in] Handle to thread */
    PBOOL pstate)   /* [out] pointer to var that receives the boost state */
{
    if (pstate) *pstate = FALSE;
    return NO_ERROR;
}


/**********************************************************************
 * SetThreadPriorityBoost [KERNEL32.@]  Sets priority boost for thread.
 *
 * Priority boost is not implemented. Thsi function always returns
 * FALSE and sets last error to ERROR_CALL_NOT_IMPLEMENTED
 *
 * RETURNS
 *    Always returns FALSE to indicate a failure
 */
BOOL WINAPI SetThreadPriorityBoost(
    HANDLE hthread, /* [in] Handle to thread */
    BOOL disable)   /* [in] TRUE to disable priority boost */
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.@)
 */
DWORD WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD dwThreadAffinityMask )
{
    DWORD ret;
    SERVER_START_REQ( set_thread_info )
    {
        req->handle   = hThread;
        req->affinity = dwThreadAffinityMask;
        req->mask     = SET_THREAD_INFO_AFFINITY;
        ret = !wine_server_call_err( req );
        /* FIXME: should return previous value */
    }
    SERVER_END_REQ;
    return ret;
}


/**********************************************************************
 * SetThreadIdealProcessor [KERNEL32.@]  Obtains timing information.
 *
 * RETURNS
 *    Success: Value of last call to SetThreadIdealProcessor
 *    Failure: -1
 */
DWORD WINAPI SetThreadIdealProcessor(
    HANDLE hThread,          /* [in] Specifies the thread of interest */
    DWORD dwIdealProcessor)  /* [in] Specifies the new preferred processor */
{
    FIXME("(%p): stub\n",hThread);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return -1L;
}


/* callback for QueueUserAPC */
static void CALLBACK call_user_apc( ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3 )
{
    PAPCFUNC func = (PAPCFUNC)arg1;
    func( arg2 );
}

/***********************************************************************
 *              QueueUserAPC  (KERNEL32.@)
 */
DWORD WINAPI QueueUserAPC( PAPCFUNC func, HANDLE hthread, ULONG_PTR data )
{
    NTSTATUS status = NtQueueApcThread( hthread, call_user_apc, (ULONG_PTR)func, data, 0 );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**********************************************************************
 * GetThreadTimes [KERNEL32.@]  Obtains timing information.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetThreadTimes(
    HANDLE thread,         /* [in]  Specifies the thread of interest */
    LPFILETIME creationtime, /* [out] When the thread was created */
    LPFILETIME exittime,     /* [out] When the thread was destroyed */
    LPFILETIME kerneltime,   /* [out] Time thread spent in kernel mode */
    LPFILETIME usertime)     /* [out] Time thread spent in user mode */
{
    BOOL ret = TRUE;

    if (creationtime || exittime)
    {
        /* We need to do a server call to get the creation time or exit time */
        /* This works on any thread */

        SERVER_START_REQ( get_thread_info )
        {
            req->handle = thread;
            req->tid_in = 0;
            if ((ret = !wine_server_call_err( req )))
            {
                if (creationtime)
                    RtlSecondsSince1970ToTime( reply->creation_time, (LARGE_INTEGER*)creationtime );
                if (exittime)
                    RtlSecondsSince1970ToTime( reply->exit_time, (LARGE_INTEGER*)exittime );
            }
        }
        SERVER_END_REQ;
    }
    if (ret && (kerneltime || usertime))
    {
        /* We call times(2) for kernel time or user time */
        /* We can only (portably) do this for the current thread */
        if (thread == GetCurrentThread())
        {
            ULONGLONG time;
            struct tms time_buf;
            long clocks_per_sec = sysconf(_SC_CLK_TCK);

            times(&time_buf);
            if (kerneltime)
            {
                time = (ULONGLONG)time_buf.tms_stime * 10000000 / clocks_per_sec;
                kerneltime->dwHighDateTime = time >> 32;
                kerneltime->dwLowDateTime = (DWORD)time;
            }
            if (usertime)
            {
                time = (ULONGLONG)time_buf.tms_utime * 10000000 / clocks_per_sec;
                usertime->dwHighDateTime = time >> 32;
                usertime->dwLowDateTime = (DWORD)time;
            }
        }
        else
        {
            if (kerneltime) kerneltime->dwHighDateTime = kerneltime->dwLowDateTime = 0;
            if (usertime) usertime->dwHighDateTime = usertime->dwLowDateTime = 0;
            FIXME("Cannot get kerneltime or usertime of other threads\n");
        }
    }
    return ret;
}


/**********************************************************************
 * VWin32_BoostThreadGroup [KERNEL.535]
 */
VOID WINAPI VWin32_BoostThreadGroup( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}


/**********************************************************************
 * VWin32_BoostThreadStatic [KERNEL.536]
 */
VOID WINAPI VWin32_BoostThreadStatic( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}


/***********************************************************************
 * GetCurrentThread [KERNEL32.@]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
#undef GetCurrentThread
HANDLE WINAPI GetCurrentThread(void)
{
    return (HANDLE)0xfffffffe;
}
