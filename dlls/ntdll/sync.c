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
	FIXME("(%p,%d,%p,0x%08x,%p) stub!\n",
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
      FIXME("(%p,%d)\n", handle, *PulseCount);

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
	IN  EVENT_INFORMATION_CLASS EventInformationClass,
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
    FIXME("(%p %u %p %u %p): stub!\n", 
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

    TRACE("(%p,%p,%p,%p,%08x,0x%08x,%p) stub\n",
          handle, when, callback, callback_arg, resume, period, state);

    SERVER_START_REQ( set_timer )
    {
        req->handle   = handle;
        req->period   = period;
        req->expire   = when->QuadPart;
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

    TRACE("(%p,%d,%p,0x%08x,%p)\n", TimerHandle, TimerInformationClass,
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
            basic_info->RemainingTime.QuadPart = reply->when;
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
    FIXME("(%u,%u,%p), stub!\n",
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
 *              invoke_apc
 *
 * Invoke a single APC. Return TRUE if a user APC has been run.
 */
static BOOL invoke_apc( const apc_call_t *call, apc_result_t *result )
{
    BOOL user_apc = FALSE;

    memset( result, 0, sizeof(*result) );

    switch (call->type)
    {
    case APC_NONE:
        break;
    case APC_USER:
        call->user.func( call->user.args[0], call->user.args[1], call->user.args[2] );
        user_apc = TRUE;
        break;
    case APC_TIMER:
        call->timer.func( call->timer.arg, (DWORD)call->timer.time, (DWORD)(call->timer.time >> 32) );
        user_apc = TRUE;
        break;
    case APC_ASYNC_IO:
        result->type = call->type;
        result->async_io.status = call->async_io.func( call->async_io.user,
                                                       call->async_io.sb,
                                                       call->async_io.status );
        break;
    case APC_VIRTUAL_ALLOC:
        result->type = call->type;
        result->virtual_alloc.addr = call->virtual_alloc.addr;
        result->virtual_alloc.size = call->virtual_alloc.size;
        result->virtual_alloc.status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                                                &result->virtual_alloc.addr,
                                                                call->virtual_alloc.zero_bits,
                                                                &result->virtual_alloc.size,
                                                                call->virtual_alloc.op_type,
                                                                call->virtual_alloc.prot );
        break;
    case APC_VIRTUAL_FREE:
        result->type = call->type;
        result->virtual_free.addr = call->virtual_free.addr;
        result->virtual_free.size = call->virtual_free.size;
        result->virtual_free.status = NtFreeVirtualMemory( NtCurrentProcess(),
                                                           &result->virtual_free.addr,
                                                           &result->virtual_free.size,
                                                           call->virtual_free.op_type );
        break;
    case APC_VIRTUAL_QUERY:
    {
        MEMORY_BASIC_INFORMATION info;
        result->type = call->type;
        result->virtual_query.status = NtQueryVirtualMemory( NtCurrentProcess(),
                                                             call->virtual_query.addr,
                                                             MemoryBasicInformation, &info,
                                                             sizeof(info), NULL );
        if (result->virtual_query.status == STATUS_SUCCESS)
        {
            result->virtual_query.base       = info.BaseAddress;
            result->virtual_query.alloc_base = info.AllocationBase;
            result->virtual_query.size       = info.RegionSize;
            result->virtual_query.state      = info.State;
            result->virtual_query.prot       = info.Protect;
            result->virtual_query.alloc_prot = info.AllocationProtect;
            result->virtual_query.alloc_type = info.Type;
        }
        break;
    }
    case APC_VIRTUAL_PROTECT:
        result->type = call->type;
        result->virtual_protect.addr = call->virtual_protect.addr;
        result->virtual_protect.size = call->virtual_protect.size;
        result->virtual_protect.status = NtProtectVirtualMemory( NtCurrentProcess(),
                                                                 &result->virtual_protect.addr,
                                                                 &result->virtual_protect.size,
                                                                 call->virtual_protect.prot,
                                                                 &result->virtual_protect.prot );
        break;
    case APC_VIRTUAL_FLUSH:
        result->type = call->type;
        result->virtual_flush.addr = call->virtual_flush.addr;
        result->virtual_flush.size = call->virtual_flush.size;
        result->virtual_flush.status = NtFlushVirtualMemory( NtCurrentProcess(),
                                                             &result->virtual_flush.addr,
                                                             &result->virtual_flush.size, 0 );
        break;
    case APC_VIRTUAL_LOCK:
        result->type = call->type;
        result->virtual_lock.addr = call->virtual_lock.addr;
        result->virtual_lock.size = call->virtual_lock.size;
        result->virtual_lock.status = NtLockVirtualMemory( NtCurrentProcess(),
                                                           &result->virtual_lock.addr,
                                                           &result->virtual_lock.size, 0 );
        break;
    case APC_VIRTUAL_UNLOCK:
        result->type = call->type;
        result->virtual_unlock.addr = call->virtual_unlock.addr;
        result->virtual_unlock.size = call->virtual_unlock.size;
        result->virtual_unlock.status = NtUnlockVirtualMemory( NtCurrentProcess(),
                                                               &result->virtual_unlock.addr,
                                                               &result->virtual_unlock.size, 0 );
        break;
    case APC_MAP_VIEW:
    {
        LARGE_INTEGER offset;
        result->type = call->type;
        result->map_view.addr   = call->map_view.addr;
        result->map_view.size   = call->map_view.size;
        offset.u.LowPart        = call->map_view.offset_low;
        offset.u.HighPart       = call->map_view.offset_high;
        result->map_view.status = NtMapViewOfSection( call->map_view.handle, NtCurrentProcess(),
                                                      &result->map_view.addr, call->map_view.zero_bits,
                                                      0, &offset, &result->map_view.size, ViewShare,
                                                      call->map_view.alloc_type, call->map_view.prot );
        NtClose( call->map_view.handle );
        break;
    }
    case APC_UNMAP_VIEW:
        result->type = call->type;
        result->unmap_view.status = NtUnmapViewOfSection( NtCurrentProcess(), call->unmap_view.addr );
        break;
    case APC_CREATE_THREAD:
    {
        CLIENT_ID id;
        result->type = call->type;
        result->create_thread.status = RtlCreateUserThread( NtCurrentProcess(), NULL,
                                                            call->create_thread.suspend, NULL,
                                                            call->create_thread.reserve,
                                                            call->create_thread.commit,
                                                            call->create_thread.func,
                                                            call->create_thread.arg,
                                                            &result->create_thread.handle, &id );
        result->create_thread.tid = HandleToULong(id.UniqueThread);
        break;
    }
    default:
        server_protocol_error( "get_apc_request: bad type %d\n", call->type );
        break;
    }
    return user_apc;
}

/***********************************************************************
 *           NTDLL_queue_process_apc
 */
NTSTATUS NTDLL_queue_process_apc( HANDLE process, const apc_call_t *call, apc_result_t *result )
{
    for (;;)
    {
        NTSTATUS ret;
        HANDLE handle = 0;
        BOOL self = FALSE;

        SERVER_START_REQ( queue_apc )
        {
            req->process = process;
            req->call = *call;
            if (!(ret = wine_server_call( req )))
            {
                handle = reply->handle;
                self = reply->self;
            }
        }
        SERVER_END_REQ;
        if (ret != STATUS_SUCCESS) return ret;

        if (self)
        {
            invoke_apc( call, result );
        }
        else
        {
            NtWaitForSingleObject( handle, FALSE, NULL );

            SERVER_START_REQ( get_apc_result )
            {
                req->handle = handle;
                if (!(ret = wine_server_call( req ))) *result = reply->result;
            }
            SERVER_END_REQ;

            if (!ret && result->type == APC_NONE) continue;  /* APC didn't run, try again */
            if (ret) NtClose( handle );
        }
        return ret;
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
    BOOL user_apc = FALSE;
    obj_handle_t apc_handle = 0;
    apc_call_t call;
    apc_result_t result;
    timeout_t abs_timeout = timeout ? timeout->QuadPart : TIMEOUT_INFINITE;

    memset( &result, 0, sizeof(result) );

    for (;;)
    {
        SERVER_START_REQ( select )
        {
            req->flags    = flags;
            req->cookie   = &cookie;
            req->signal   = signal_object;
            req->prev_apc = apc_handle;
            req->timeout  = abs_timeout;
            wine_server_add_data( req, &result, sizeof(result) );
            wine_server_add_data( req, handles, count * sizeof(HANDLE) );
            ret = wine_server_call( req );
            abs_timeout = reply->timeout;
            apc_handle  = reply->apc_handle;
            call        = reply->call;
        }
        SERVER_END_REQ;
        if (ret == STATUS_PENDING) ret = wait_reply( &cookie );
        if (ret != STATUS_USER_APC) break;
        if (invoke_apc( &call, &result ))
        {
            /* if we ran a user apc we have to check once more if an object got signaled,
             * but we don't want to wait */
            abs_timeout = 0;
            user_apc = TRUE;
        }
        signal_object = 0;  /* don't signal it multiple times */
    }

    if (ret == STATUS_TIMEOUT && user_apc) ret = STATUS_USER_APC;

    /* A test on Windows 2000 shows that Windows always yields during
       a wait, but a wait that is hit by an event gets a priority
       boost as well.  This seems to model that behavior the closest.  */
    if (ret == STATUS_TIMEOUT) NtYieldExecution();

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
    /* if alertable, we need to query the server */
    if (alertable)
        return NTDLL_wait_for_multiple_objects( 0, NULL, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE,
                                                timeout, 0 );

    if (!timeout || timeout->QuadPart == TIMEOUT_INFINITE)  /* sleep forever */
    {
        for (;;) select( 0, NULL, NULL, NULL, NULL );
    }
    else
    {
        LARGE_INTEGER now;
        timeout_t when, diff;

        if ((when = timeout->QuadPart) < 0)
        {
            NtQuerySystemTime( &now );
            when = now.QuadPart - when;
        }

        /* Note that we yield after establishing the desired timeout */
        NtYieldExecution();
        if (!when) return STATUS_SUCCESS;

        for (;;)
        {
            struct timeval tv;
            NtQuerySystemTime( &now );
            diff = (when - now.QuadPart + 9) / 10;
            if (diff <= 0) break;
            tv.tv_sec  = diff / 1000000;
            tv.tv_usec = diff % 1000000;
            if (select( 0, NULL, NULL, NULL, &tv ) != -1) break;
        }
    }
    return STATUS_SUCCESS;
}

/******************************************************************
 *              NtCreateIoCompletion (NTDLL.@)
 *              ZwCreateIoCompletion (NTDLL.@)
 *
 * Creates I/O completion object.
 *
 * PARAMS
 *      CompletionPort            [O] created completion object handle will be placed there
 *      DesiredAccess             [I] desired access to a handle (combination of IO_COMPLETION_*)
 *      ObjectAttributes          [I] completion object attributes
 *      NumberOfConcurrentThreads [I] desired number of concurrent active worker threads
 *
 */
NTSTATUS WINAPI NtCreateIoCompletion( PHANDLE CompletionPort, ACCESS_MASK DesiredAccess,
                                      POBJECT_ATTRIBUTES ObjectAttributes, ULONG NumberOfConcurrentThreads )
{
    NTSTATUS status;

    TRACE("(%p, %x, %p, %d)\n", CompletionPort, DesiredAccess,
          ObjectAttributes, NumberOfConcurrentThreads);

    if (!CompletionPort)
        return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( create_completion )
    {
        req->access     = DesiredAccess;
        req->attributes = ObjectAttributes ? ObjectAttributes->Attributes : 0;
        req->rootdir    = ObjectAttributes ? ObjectAttributes->RootDirectory : NULL;
        req->concurrent = NumberOfConcurrentThreads;
        if (ObjectAttributes && ObjectAttributes->ObjectName)
            wine_server_add_data( req, ObjectAttributes->ObjectName->Buffer,
                                       ObjectAttributes->ObjectName->Length );
        if (!(status = wine_server_call( req )))
            *CompletionPort = reply->handle;
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *              NtSetIoCompletion (NTDLL.@)
 *              ZwSetIoCompletion (NTDLL.@)
 *
 * Inserts completion message into queue
 *
 * PARAMS
 *      CompletionPort           [I] HANDLE to completion object
 *      CompletionKey            [I] completion key
 *      CompletionValue          [I] completion value (usually pointer to OVERLAPPED)
 *      Status                   [I] operation status
 *      NumberOfBytesTransferred [I] number of bytes transferred
 */
NTSTATUS WINAPI NtSetIoCompletion( HANDLE CompletionPort, ULONG_PTR CompletionKey,
                                   ULONG_PTR CompletionValue, NTSTATUS Status,
                                   ULONG NumberOfBytesTransferred )
{
    NTSTATUS status;

    TRACE("(%p, %lx, %lx, %x, %d)\n", CompletionPort, CompletionKey,
          CompletionValue, Status, NumberOfBytesTransferred);

    SERVER_START_REQ( add_completion )
    {
        req->handle      = CompletionPort;
        req->ckey        = CompletionKey;
        req->cvalue      = CompletionValue;
        req->status      = Status;
        req->information = NumberOfBytesTransferred;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *              NtRemoveIoCompletion (NTDLL.@)
 *              ZwRemoveIoCompletion (NTDLL.@)
 *
 * (Wait for and) retrieve first completion message from completion object's queue
 *
 * PARAMS
 *      CompletionPort  [I] HANDLE to I/O completion object
 *      CompletionKey   [O] completion key
 *      CompletionValue [O] Completion value given in NtSetIoCompletion or in async operation
 *      iosb            [O] IO_STATUS_BLOCK of completed asynchronous operation
 *      WaitTime        [I] optional wait time in NTDLL format
 *
 */
NTSTATUS WINAPI NtRemoveIoCompletion( HANDLE CompletionPort, PULONG_PTR CompletionKey,
                                      PULONG_PTR CompletionValue, PIO_STATUS_BLOCK iosb,
                                      PLARGE_INTEGER WaitTime )
{
    NTSTATUS status;

    TRACE("(%p, %p, %p, %p, %p)\n", CompletionPort, CompletionKey,
          CompletionValue, iosb, WaitTime);

    for(;;)
    {
        SERVER_START_REQ( remove_completion )
        {
            req->handle = CompletionPort;
            if (!(status = wine_server_call( req )))
            {
                *CompletionKey    = reply->ckey;
                *CompletionValue  = reply->cvalue;
                iosb->Information = reply->information;
                iosb->u.Status    = reply->status;
            }
        }
        SERVER_END_REQ;
        if (status != STATUS_PENDING) break;

        status = NtWaitForSingleObject( CompletionPort, FALSE, WaitTime );
        if (status != WAIT_OBJECT_0) break;
    }
    return status;
}

/******************************************************************
 *              NtOpenIoCompletion (NTDLL.@)
 *              ZwOpenIoCompletion (NTDLL.@)
 *
 * Opens I/O completion object
 *
 * PARAMS
 *      CompletionPort     [O] completion object handle will be placed there
 *      DesiredAccess      [I] desired access to a handle (combination of IO_COMPLETION_*)
 *      ObjectAttributes   [I] completion object name
 *
 */
NTSTATUS WINAPI NtOpenIoCompletion( PHANDLE CompletionPort, ACCESS_MASK DesiredAccess,
                                    POBJECT_ATTRIBUTES ObjectAttributes )
{
    NTSTATUS status;

    TRACE("(%p, 0x%x, %p)\n", CompletionPort, DesiredAccess, ObjectAttributes);

    if (!CompletionPort || !ObjectAttributes || !ObjectAttributes->ObjectName)
        return STATUS_INVALID_PARAMETER;

    SERVER_START_REQ( open_completion )
    {
        req->access     = DesiredAccess;
        req->rootdir    = ObjectAttributes->RootDirectory;
        wine_server_add_data( req, ObjectAttributes->ObjectName->Buffer,
                                   ObjectAttributes->ObjectName->Length );
        if (!(status = wine_server_call( req )))
            *CompletionPort = reply->handle;
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *              NtQueryIoCompletion (NTDLL.@)
 *              ZwQueryIoCompletion (NTDLL.@)
 *
 * Requests information about given I/O completion object
 *
 * PARAMS
 *      CompletionPort        [I] HANDLE to completion port to request
 *      InformationClass      [I] information class
 *      CompletionInformation [O] user-provided buffer for data
 *      BufferLength          [I] buffer length
 *      RequiredLength        [O] required buffer length
 *
 */
NTSTATUS WINAPI NtQueryIoCompletion( HANDLE CompletionPort, IO_COMPLETION_INFORMATION_CLASS InformationClass,
                                     PVOID CompletionInformation, ULONG BufferLength, PULONG RequiredLength )
{
    NTSTATUS status;

    TRACE("(%p, %d, %p, 0x%x, %p)\n", CompletionPort, InformationClass, CompletionInformation,
          BufferLength, RequiredLength);

    if (!CompletionInformation) return STATUS_INVALID_PARAMETER;
    switch( InformationClass )
    {
        case IoCompletionBasicInformation:
            {
                ULONG *info = (ULONG *)CompletionInformation;

                if (RequiredLength) *RequiredLength = sizeof(*info);
                if (BufferLength != sizeof(*info))
                    status = STATUS_INFO_LENGTH_MISMATCH;
                else
                {
                    SERVER_START_REQ( query_completion )
                    {
                        req->handle = CompletionPort;
                        if (!(status = wine_server_call( req )))
                            *info = reply->depth;
                    }
                    SERVER_END_REQ;
                }
            }
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            break;
    }
    return status;
}
