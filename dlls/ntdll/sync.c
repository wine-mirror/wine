/*
 *	Process synchronisation
 *
 * Copyright 1996, 1997, 1998 Marcus Meissner
 * Copyright 1997, 1999 Alexandre Julliard
 * Copyright 1999, 2000 Juergen Schmied
 * Copyright 2003 Eric Pouech
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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SCHED_H
# include <sched.h>
#endif
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "thread.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


/*
 *	Semaphores
 */

/******************************************************************************
 *  NtCreateSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateSemaphore( OUT PHANDLE SemaphoreHandle,
                                   IN ACCESS_MASK access,
                                   IN const OBJECT_ATTRIBUTES *attr OPTIONAL,
                                   IN LONG InitialCount,
                                   IN LONG MaximumCount )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if (MaximumCount <= 0 || InitialCount < 0 || InitialCount > MaximumCount)
        return STATUS_INVALID_PARAMETER;
    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( create_semaphore )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        req->initial = InitialCount;
        req->max     = MaximumCount;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *SemaphoreHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtOpenSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenSemaphore( OUT PHANDLE SemaphoreHandle,
                                 IN ACCESS_MASK access,
                                 IN const OBJECT_ATTRIBUTES *attr )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_semaphore )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *SemaphoreHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQuerySemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySemaphore(
	HANDLE SemaphoreHandle,
	SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
	PVOID SemaphoreInformation,
	ULONG Length,
	PULONG ReturnLength)
{
	FIXME("(%p,%d,%p,0x%08lx,%p) stub!\n",
	SemaphoreHandle, SemaphoreInformationClass, SemaphoreInformation, Length, ReturnLength);
	return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtReleaseSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseSemaphore( HANDLE handle, ULONG count, PULONG previous )
{
    NTSTATUS ret;
    SERVER_START_REQ( release_semaphore )
    {
        req->handle = handle;
        req->count  = count;
        if (!(ret = wine_server_call( req )))
        {
            if (previous) *previous = reply->prev_count;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*
 *	Events
 */

/**************************************************************************
 * NtCreateEvent (NTDLL.@)
 * ZwCreateEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN const OBJECT_ATTRIBUTES *attr,
	IN BOOLEAN ManualReset,
	IN BOOLEAN InitialState)
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( create_event )
    {
        req->access = DesiredAccess;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        req->manual_reset = ManualReset;
        req->initial_state = InitialState;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *EventHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtOpenEvent (NTDLL.@)
 *  ZwOpenEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenEvent(
	OUT PHANDLE EventHandle,
	IN ACCESS_MASK DesiredAccess,
	IN const OBJECT_ATTRIBUTES *attr )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_event )
    {
        req->access  = DesiredAccess;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *EventHandle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *  NtSetEvent (NTDLL.@)
 *  ZwSetEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtSetEvent( HANDLE handle, PULONG NumberOfThreadsReleased )
{
    NTSTATUS ret;

    /* FIXME: set NumberOfThreadsReleased */

    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = SET_EVENT;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtResetEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtResetEvent( HANDLE handle, PULONG NumberOfThreadsReleased )
{
    NTSTATUS ret;

    /* resetting an event can't release any thread... */
    if (NumberOfThreadsReleased) *NumberOfThreadsReleased = 0;

    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = RESET_EVENT;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtClearEvent (NTDLL.@)
 *
 * FIXME
 *   same as NtResetEvent ???
 */
NTSTATUS WINAPI NtClearEvent ( HANDLE handle )
{
    return NtResetEvent( handle, NULL );
}

/******************************************************************************
 *  NtPulseEvent (NTDLL.@)
 *
 * FIXME
 *   PulseCount
 */
NTSTATUS WINAPI NtPulseEvent( HANDLE handle, PULONG PulseCount )
{
    NTSTATUS ret;

    if (PulseCount)
      FIXME("(%p,%ld)\n", handle, *PulseCount);

    SERVER_START_REQ( event_op )
    {
        req->handle = handle;
        req->op     = PULSE_EVENT;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQueryEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryEvent (
	IN  HANDLE EventHandle,
	IN  UINT EventInformationClass,
	OUT PVOID EventInformation,
	IN  ULONG EventInformationLength,
	OUT PULONG  ReturnLength)
{
	FIXME("(%p)\n", EventHandle);
	return STATUS_SUCCESS;
}

/*
 *	Mutants (known as Mutexes in Kernel32)
 */

/******************************************************************************
 *              NtCreateMutant                          [NTDLL.@]
 *              ZwCreateMutant                          [NTDLL.@]
 */
NTSTATUS WINAPI NtCreateMutant(OUT HANDLE* MutantHandle,
                               IN ACCESS_MASK access,
                               IN const OBJECT_ATTRIBUTES* attr OPTIONAL,
                               IN BOOLEAN InitialOwner)
{
    NTSTATUS    status;
    DWORD       len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( create_mutex )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        req->owned   = InitialOwner;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *MutantHandle = reply->handle;
    }
    SERVER_END_REQ;
    return status;
}

/**************************************************************************
 *		NtOpenMutant				[NTDLL.@]
 *		ZwOpenMutant				[NTDLL.@]
 */
NTSTATUS WINAPI NtOpenMutant(OUT HANDLE* MutantHandle, 
                             IN ACCESS_MASK access, 
                             IN const OBJECT_ATTRIBUTES* attr )
{
    NTSTATUS    status;
    DWORD       len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_mutex )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *MutantHandle = reply->handle;
    }
    SERVER_END_REQ;
    return status;
}

/**************************************************************************
 *		NtReleaseMutant				[NTDLL.@]
 *		ZwReleaseMutant				[NTDLL.@]
 */
NTSTATUS WINAPI NtReleaseMutant( IN HANDLE handle, OUT PLONG prev_count OPTIONAL)
{
    NTSTATUS    status;

    SERVER_START_REQ( release_mutex )
    {
        req->handle = handle;
        status = wine_server_call( req );
        if (prev_count) *prev_count = reply->prev_count;
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *		NtQueryMutant                   [NTDLL.@]
 *		ZwQueryMutant                   [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryMutant(IN HANDLE handle, 
                              IN MUTANT_INFORMATION_CLASS MutantInformationClass, 
                              OUT PVOID MutantInformation, 
                              IN ULONG MutantInformationLength, 
                              OUT PULONG ResultLength OPTIONAL )
{
    FIXME("(%p %u %p %lu %p): stub!\n", 
          handle, MutantInformationClass, MutantInformation, MutantInformationLength, ResultLength);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 *	Timers
 */

/**************************************************************************
 *		NtCreateTimer				[NTDLL.@]
 *		ZwCreateTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtCreateTimer(OUT HANDLE *handle,
                              IN ACCESS_MASK access,
                              IN const OBJECT_ATTRIBUTES *attr OPTIONAL,
                              IN TIMER_TYPE timer_type)
{
    DWORD       len = (attr && attr->ObjectName) ? attr->ObjectName->Length : 0;
    NTSTATUS    status;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    if (timer_type != NotificationTimer && timer_type != SynchronizationTimer)
        return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( create_timer )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        req->manual  = (timer_type == NotificationTimer) ? TRUE : FALSE;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *handle = reply->handle;
    }
    SERVER_END_REQ;
    return status;

}

/**************************************************************************
 *		NtOpenTimer				[NTDLL.@]
 *		ZwOpenTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtOpenTimer(OUT PHANDLE handle,
                            IN ACCESS_MASK access,
                            IN const OBJECT_ATTRIBUTES* attr )
{
    DWORD       len = (attr && attr->ObjectName) ? attr->ObjectName->Length : 0;
    NTSTATUS    status;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_timer )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->rootdir = attr ? attr->RootDirectory : 0;
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *handle = reply->handle;
    }
    SERVER_END_REQ;
    return status;
}

/**************************************************************************
 *		NtSetTimer				[NTDLL.@]
 *		ZwSetTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtSetTimer(IN HANDLE handle,
                           IN const LARGE_INTEGER* when,
                           IN PTIMER_APC_ROUTINE callback,
                           IN PVOID callback_arg,
                           IN BOOLEAN resume,
                           IN ULONG period OPTIONAL,
                           OUT PBOOLEAN state OPTIONAL)
{
    NTSTATUS    status = STATUS_SUCCESS;

    TRACE("(%p,%p,%p,%p,%08x,0x%08lx,%p) stub\n",
          handle, when, callback, callback_arg, resume, period, state);

    SERVER_START_REQ( set_timer )
    {
        if (!when->u.LowPart && !when->u.HighPart)
        {
            /* special case to start timeout on now+period without too many calculations */
            req->expire.sec  = 0;
            req->expire.usec = 0;
        }
        else NTDLL_get_server_timeout( &req->expire, when );

        req->handle   = handle;
        req->period   = period;
        req->callback = callback;
        req->arg      = callback_arg;
        status = wine_server_call( req );
        if (state) *state = reply->signaled;
    }
    SERVER_END_REQ;

    /* set error but can still succeed */
    if (resume && status == STATUS_SUCCESS) return STATUS_TIMER_RESUME_IGNORED;
    return status;
}

/**************************************************************************
 *		NtCancelTimer				[NTDLL.@]
 *		ZwCancelTimer				[NTDLL.@]
 */
NTSTATUS WINAPI NtCancelTimer(IN HANDLE handle, OUT BOOLEAN* state)
{
    NTSTATUS    status;

    SERVER_START_REQ( cancel_timer )
    {
        req->handle = handle;
        status = wine_server_call( req );
        if (state) *state = reply->signaled;
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************************
 *  NtQueryTimer (NTDLL.@)
 *
 * Retrieves information about a timer.
 *
 * PARAMS
 *  TimerHandle           [I] The timer to retrieve information about.
 *  TimerInformationClass [I] The type of information to retrieve.
 *  TimerInformation      [O] Pointer to buffer to store information in.
 *  Length                [I] The length of the buffer pointed to by TimerInformation.
 *  ReturnLength          [O] Optional. The size of buffer actually used.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_INFO_LENGTH_MISMATCH, if Length doesn't match the required data
 *           size for the class specified.
 *           STATUS_INVALID_INFO_CLASS, if an invalid TimerInformationClass was specified.
 *           STATUS_ACCESS_DENIED, if TimerHandle does not have TIMER_QUERY_STATE access
 *           to the timer.
 */
NTSTATUS WINAPI NtQueryTimer(
    HANDLE TimerHandle,
    TIMER_INFORMATION_CLASS TimerInformationClass,
    PVOID TimerInformation,
    ULONG Length,
    PULONG ReturnLength)
{
    TIMER_BASIC_INFORMATION * basic_info = (TIMER_BASIC_INFORMATION *)TimerInformation;
    NTSTATUS status;
    LARGE_INTEGER now;

    TRACE("(%p,%d,%p,0x%08lx,%p)\n", TimerHandle, TimerInformationClass,
       TimerInformation, Length, ReturnLength);

    switch (TimerInformationClass)
    {
    case TimerBasicInformation:
        if (Length < sizeof(TIMER_BASIC_INFORMATION))
            return STATUS_INFO_LENGTH_MISMATCH;

        SERVER_START_REQ(get_timer_info)
        {
            req->handle = TimerHandle;
            status = wine_server_call(req);

            /* convert server time to absolute NTDLL time */
            NTDLL_from_server_timeout(&basic_info->RemainingTime, &reply->when);
            basic_info->TimerState = reply->signaled;
        }
        SERVER_END_REQ;

        /* convert from absolute into relative time */
        NtQuerySystemTime(&now);
        if (now.QuadPart > basic_info->RemainingTime.QuadPart)
            basic_info->RemainingTime.QuadPart = 0;
        else
            basic_info->RemainingTime.QuadPart -= now.QuadPart;

        if (ReturnLength) *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);

        return status;
    }

    FIXME("Unhandled class %d\n", TimerInformationClass);
    return STATUS_INVALID_INFO_CLASS;
}


/******************************************************************************
 * NtQueryTimerResolution [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryTimerResolution(OUT ULONG* min_resolution,
                                       OUT ULONG* max_resolution,
                                       OUT ULONG* current_resolution)
{
    FIXME("(%p,%p,%p), stub!\n",
          min_resolution, max_resolution, current_resolution);

    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 * NtSetTimerResolution [NTDLL.@]
 */
NTSTATUS WINAPI NtSetTimerResolution(IN ULONG resolution,
                                     IN BOOLEAN set_resolution,
                                     OUT ULONG* current_resolution )
{
    FIXME("(%lu,%u,%p), stub!\n",
          resolution, set_resolution, current_resolution);

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *              wait_reply
 *
 * Wait for a reply on the waiting pipe of the current thread.
 */
static int wait_reply( void *cookie )
{
    int signaled;
    struct wake_up_reply reply;
    for (;;)
    {
        int ret;
        ret = read( ntdll_get_thread_data()->wait_fd[0], &reply, sizeof(reply) );
        if (ret == sizeof(reply))
        {
            if (!reply.cookie) break;  /* thread got killed */
            if (reply.cookie == cookie) return reply.signaled;
            /* we stole another reply, wait for the real one */
            signaled = wait_reply( cookie );
            /* and now put the wrong one back in the pipe */
            for (;;)
            {
                ret = write( ntdll_get_thread_data()->wait_fd[1], &reply, sizeof(reply) );
                if (ret == sizeof(reply)) break;
                if (ret >= 0) server_protocol_error( "partial wakeup write %d\n", ret );
                if (errno == EINTR) continue;
                server_protocol_perror("wakeup write");
            }
            return signaled;
        }
        if (ret >= 0) server_protocol_error( "partial wakeup read %d\n", ret );
        if (errno == EINTR) continue;
        server_protocol_perror("wakeup read");
    }
    /* the server closed the connection; time to die... */
    server_abort_thread(0);
}


/***********************************************************************
 *              call_apcs
 *
 * Call outstanding APCs.
 */
static void call_apcs( BOOL alertable )
{
    FARPROC proc;
    LARGE_INTEGER time;
    void *arg1, *arg2, *arg3;

    for (;;)
    {
        int type = APC_NONE;
        SERVER_START_REQ( get_apc )
        {
            req->alertable = alertable;
            if (!wine_server_call( req )) type = reply->type;
            proc = reply->func;
            arg1 = reply->arg1;
            arg2 = reply->arg2;
            arg3 = reply->arg3;
        }
        SERVER_END_REQ;

        switch (type)
        {
        case APC_NONE:
            return;  /* no more APCs */
        case APC_USER:
            proc( arg1, arg2, arg3 );
            break;
        case APC_TIMER:
            /* convert sec/usec to NT time */
            RtlSecondsSince1970ToTime( (time_t)arg1, &time );
            time.QuadPart += (DWORD)arg2 * 10;
            proc( arg3, time.u.LowPart, time.u.HighPart );
            break;
        case APC_ASYNC_IO:
            NtCurrentTeb()->num_async_io--;
            proc( arg1, (IO_STATUS_BLOCK*)arg2, (ULONG)arg3 );
            break;
        default:
            server_protocol_error( "get_apc_request: bad type %d\n", type );
            break;
        }
    }
}


/***********************************************************************
 *              NTDLL_wait_for_multiple_objects
 *
 * Implementation of NtWaitForMultipleObjects
 */
NTSTATUS NTDLL_wait_for_multiple_objects( UINT count, const HANDLE *handles, UINT flags,
                                          const LARGE_INTEGER *timeout, HANDLE signal_object )
{
    NTSTATUS ret;
    int cookie;

    if (timeout) flags |= SELECT_TIMEOUT;
    for (;;)
    {
        SERVER_START_REQ( select )
        {
            req->flags   = flags;
            req->cookie  = &cookie;
            req->signal  = signal_object;
            NTDLL_get_server_timeout( &req->timeout, timeout );
            wine_server_add_data( req, handles, count * sizeof(HANDLE) );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (ret == STATUS_PENDING) ret = wait_reply( &cookie );
        if (ret != STATUS_USER_APC) break;
        call_apcs( (flags & SELECT_ALERTABLE) != 0 );
        if (flags & SELECT_ALERTABLE) break;
        signal_object = 0;  /* don't signal it multiple times */
    }

    /* A test on Windows 2000 shows that Windows always yields during
       a wait, but a wait that is hit by an event gets a priority
       boost as well.  This seems to model that behavior the closest.  */
    if (ret == WAIT_TIMEOUT) NtYieldExecution();

    return ret;
}


/* wait operations */

/******************************************************************
 *		NtWaitForMultipleObjects (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                          BOOLEAN wait_all, BOOLEAN alertable,
                                          const LARGE_INTEGER *timeout )
{
    UINT flags = SELECT_INTERRUPTIBLE;

    if (!count || count > MAXIMUM_WAIT_OBJECTS) return STATUS_INVALID_PARAMETER_1;

    if (wait_all) flags |= SELECT_ALL;
    if (alertable) flags |= SELECT_ALERTABLE;
    return NTDLL_wait_for_multiple_objects( count, handles, flags, timeout, 0 );
}


/******************************************************************
 *		NtWaitForSingleObject (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForSingleObject(HANDLE handle, BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    return NtWaitForMultipleObjects( 1, &handle, FALSE, alertable, timeout );
}


/******************************************************************
 *		NtSignalAndWaitForSingleObject (NTDLL.@)
 */
NTSTATUS WINAPI NtSignalAndWaitForSingleObject( HANDLE hSignalObject, HANDLE hWaitObject,
                                                BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    UINT flags = SELECT_INTERRUPTIBLE;

    if (!hSignalObject) return STATUS_INVALID_HANDLE;
    if (alertable) flags |= SELECT_ALERTABLE;
    return NTDLL_wait_for_multiple_objects( 1, &hWaitObject, flags, timeout, hSignalObject );
}


/******************************************************************
 *		NtYieldExecution (NTDLL.@)
 */
NTSTATUS WINAPI NtYieldExecution(void)
{
#ifdef HAVE_SCHED_YIELD
    sched_yield();
    return STATUS_SUCCESS;
#else
    return STATUS_NO_YIELD_PERFORMED;
#endif
}


/******************************************************************
 *		NtDelayExecution (NTDLL.@)
 */
NTSTATUS WINAPI NtDelayExecution( BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    /* if alertable or async I/O in progress, we need to query the server */
    if (alertable || NtCurrentTeb()->num_async_io)
    {
        UINT flags = SELECT_INTERRUPTIBLE;
        if (alertable) flags |= SELECT_ALERTABLE;
        return NTDLL_wait_for_multiple_objects( 0, NULL, flags, timeout, 0 );
    }

    if (!timeout)  /* sleep forever */
    {
        for (;;) select( 0, NULL, NULL, NULL, NULL );
    }
    else
    {
        abs_time_t when;

        NTDLL_get_server_timeout( &when, timeout );

        /* Note that we yield after establishing the desired timeout */
        NtYieldExecution();

        for (;;)
        {
            struct timeval tv;
            gettimeofday( &tv, 0 );
            tv.tv_sec = when.sec - tv.tv_sec;
            if ((tv.tv_usec = when.usec - tv.tv_usec) < 0)
            {
                tv.tv_usec += 1000000;
                tv.tv_sec--;
            }
            /* if our yield already passed enough time, we're done */
            if (tv.tv_sec < 0) break;

            if (select( 0, NULL, NULL, NULL, &tv ) != -1) break;
        }
    }
    return STATUS_SUCCESS;
}

/******************************************************************
 *		NtCreateIoCompletion (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateIoCompletion( PHANDLE CompletionPort, ACCESS_MASK DesiredAccess,
                                      POBJECT_ATTRIBUTES ObjectAttributes, ULONG NumberOfConcurrentThreads )
{
    FIXME("(%p, %lx, %p, %ld)\n", CompletionPort, DesiredAccess,
          ObjectAttributes, NumberOfConcurrentThreads);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtSetIoCompletion( HANDLE CompletionPort, ULONG_PTR CompletionKey,
                                   PIO_STATUS_BLOCK iosb, ULONG NumberOfBytesTransferred,
                                   ULONG NumberOfBytesToTransfer )
{
    FIXME("(%p, %lx, %p, %ld, %ld)\n", CompletionPort, CompletionKey,
          iosb, NumberOfBytesTransferred, NumberOfBytesToTransfer);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtRemoveIoCompletion( HANDLE CompletionPort, PULONG_PTR CompletionKey,
                                      PIO_STATUS_BLOCK iosb, PULONG OperationStatus,
                                      PLARGE_INTEGER WaitTime )
{
    FIXME("(%p, %p, %p, %p, %p)\n", CompletionPort, CompletionKey,
          iosb, OperationStatus, WaitTime);
    return STATUS_NOT_IMPLEMENTED;
}
