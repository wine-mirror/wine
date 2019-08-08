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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/asm.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *           FreeLibraryAndExitThread (KERNEL32.@)
 */
void WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}


/***********************************************************************
 * Wow64SetThreadContext [KERNEL32.@]
 */
BOOL WINAPI Wow64SetThreadContext( HANDLE handle, const WOW64_CONTEXT *context)
{
#ifdef __i386__
    NTSTATUS status = NtSetContextThread( handle, (const CONTEXT *)context );
#elif defined(__x86_64__)
    NTSTATUS status = RtlWow64SetThreadContext( handle, context );
#else
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    FIXME("not implemented on this platform\n");
#endif
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 * Wow64GetThreadContext [KERNEL32.@]
 */
BOOL WINAPI Wow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context)
{
#ifdef __i386__
    NTSTATUS status = NtGetContextThread( handle, (CONTEXT *)context );
#elif defined(__x86_64__)
    NTSTATUS status = RtlWow64GetThreadContext( handle, context );
#else
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    FIXME("not implemented on this platform\n");
#endif
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.@)
 */
DWORD_PTR WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask )
{
    NTSTATUS                    status;
    THREAD_BASIC_INFORMATION    tbi;

    status = NtQueryInformationThread( hThread, ThreadBasicInformation, 
                                       &tbi, sizeof(tbi), NULL );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    status = NtSetInformationThread( hThread, ThreadAffinityMask, 
                                     &dwThreadAffinityMask,
                                     sizeof(dwThreadAffinityMask));
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return tbi.AffinityMask;
}


/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32.@)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE hthread, DWORD sel, LPLDT_ENTRY ldtent )
{
    THREAD_DESCRIPTOR_INFORMATION tdi;
    NTSTATUS status;

    tdi.Selector = sel;
    status = NtQueryInformationThread( hthread, ThreadDescriptorTableEntry, &tdi, sizeof(tdi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    *ldtent = tdi.Entry;
    return TRUE;
}


/***********************************************************************
 *              QueueUserWorkItem  (KERNEL32.@)
 */
BOOL WINAPI QueueUserWorkItem( LPTHREAD_START_ROUTINE Function, PVOID Context, ULONG Flags )
{
    NTSTATUS status;

    TRACE("(%p,%p,0x%08x)\n", Function, Context, Flags);

    status = RtlQueueWorkItem( Function, Context, Flags );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 * GetCurrentThread [KERNEL32.@]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
HANDLE WINAPI KERNEL32_GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}

/**********************************************************************
 *		SetLastError (KERNEL32.@)
 *
 * Sets the last-error code.
 *
 * RETURNS
 * Nothing.
 */
void WINAPI KERNEL32_SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->LastErrorValue = error;
}

/**********************************************************************
 *              GetLastError (KERNEL32.@)
 *
 * Get the last-error code.
 *
 * RETURNS
 *  last-error code.
 */
DWORD WINAPI KERNEL32_GetLastError(void)
{
    return NtCurrentTeb()->LastErrorValue;
}

/***********************************************************************
 *		GetCurrentProcessId (KERNEL32.@)
 *
 * Get the current process identifier.
 *
 * RETURNS
 *  current process identifier
 */
DWORD WINAPI KERNEL32_GetCurrentProcessId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess);
}

/***********************************************************************
 *		GetCurrentThreadId (KERNEL32.@)
 *
 * Get the current thread identifier.
 *
 * RETURNS
 *  current thread identifier
 */
DWORD WINAPI KERNEL32_GetCurrentThreadId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueThread);
}

/***********************************************************************
 *           GetProcessHeap    (KERNEL32.@)
 */
HANDLE WINAPI KERNEL32_GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}

/***********************************************************************
 *              CallbackMayRunLong (KERNEL32.@)
 */
BOOL WINAPI CallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    NTSTATUS status;

    TRACE( "%p\n", instance );

    status = TpCallbackMayRunLong( instance );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              CreateThreadpool (KERNEL32.@)
 */
PTP_POOL WINAPI CreateThreadpool( PVOID reserved )
{
    TP_POOL *pool;
    NTSTATUS status;

    TRACE( "%p\n", reserved );

    status = TpAllocPool( &pool, reserved );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return pool;
}

/***********************************************************************
 *              CreateThreadpoolCleanupGroup (KERNEL32.@)
 */
PTP_CLEANUP_GROUP WINAPI CreateThreadpoolCleanupGroup( void )
{
    TP_CLEANUP_GROUP *group;
    NTSTATUS status;

    TRACE( "\n" );

    status = TpAllocCleanupGroup( &group );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return group;
}

/***********************************************************************
 *              CreateThreadpoolIo (KERNEL32.@)
 */
PTP_IO WINAPI CreateThreadpoolIo( HANDLE handle, PTP_WIN32_IO_CALLBACK callback,
                                  PVOID userdata, TP_CALLBACK_ENVIRON *environment )
{
    FIXME("(%p, %p, %p, %p): stub\n", handle, callback, userdata, environment);
    return FALSE;
}

/***********************************************************************
 *              CreateThreadpoolTimer (KERNEL32.@)
 */
PTP_TIMER WINAPI CreateThreadpoolTimer( PTP_TIMER_CALLBACK callback, PVOID userdata,
                                        TP_CALLBACK_ENVIRON *environment )
{
    TP_TIMER *timer;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocTimer( &timer, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return timer;
}

/***********************************************************************
 *              CreateThreadpoolWait (KERNEL32.@)
 */
PTP_WAIT WINAPI CreateThreadpoolWait( PTP_WAIT_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WAIT *wait;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWait( &wait, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return wait;
}

/***********************************************************************
 *              CreateThreadpoolWork (KERNEL32.@)
 */
PTP_WORK WINAPI CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpAllocWork( &work, callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return NULL;
    }

    return work;
}

/***********************************************************************
 *              SetThreadpoolTimer (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolTimer( TP_TIMER *timer, FILETIME *due_time,
                                DWORD period, DWORD window_length )
{
    LARGE_INTEGER timeout;

    TRACE( "%p, %p, %u, %u\n", timer, due_time, period, window_length );

    if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetTimer( timer, due_time ? &timeout : NULL, period, window_length );
}

/***********************************************************************
 *              SetThreadpoolWait (KERNEL32.@)
 */
VOID WINAPI SetThreadpoolWait( TP_WAIT *wait, HANDLE handle, FILETIME *due_time )
{
    LARGE_INTEGER timeout;

    TRACE( "%p, %p, %p\n", wait, handle, due_time );

    if (!handle)
    {
        due_time = NULL;
    }
    else if (due_time)
    {
        timeout.u.LowPart = due_time->dwLowDateTime;
        timeout.u.HighPart = due_time->dwHighDateTime;
    }

    TpSetWait( wait, handle, due_time ? &timeout : NULL );
}

/***********************************************************************
 *              TrySubmitThreadpoolCallback (KERNEL32.@)
 */
BOOL WINAPI TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                         TP_CALLBACK_ENVIRON *environment )
{
    NTSTATUS status;

    TRACE( "%p, %p, %p\n", callback, userdata, environment );

    status = TpSimpleTryPost( callback, userdata, environment );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}
