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
#include "wine/port.h"

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
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

HANDLE keyed_event = NULL;

static inline int interlocked_dec_if_nonzero( int *dest )
{
    int val, tmp;
    for (val = *dest;; val = tmp)
    {
        if (!val || (tmp = interlocked_cmpxchg( dest, val - 1, val )) == val)
            break;
    }
    return val;
}

/* creates a struct security_descriptor and contained information in one contiguous piece of memory */
NTSTATUS NTDLL_create_struct_sd(PSECURITY_DESCRIPTOR nt_sd, struct security_descriptor **server_sd,
                                data_size_t *server_sd_len)
{
    unsigned int len;
    PSID owner, group;
    ACL *dacl, *sacl;
    BOOLEAN owner_present, group_present, dacl_present, sacl_present;
    BOOLEAN defaulted;
    NTSTATUS status;
    unsigned char *ptr;

    if (!nt_sd)
    {
        *server_sd = NULL;
        *server_sd_len = 0;
        return STATUS_SUCCESS;
    }

    len = sizeof(struct security_descriptor);

    status = RtlGetOwnerSecurityDescriptor(nt_sd, &owner, &owner_present);
    if (status != STATUS_SUCCESS) return status;
    status = RtlGetGroupSecurityDescriptor(nt_sd, &group, &group_present);
    if (status != STATUS_SUCCESS) return status;
    status = RtlGetSaclSecurityDescriptor(nt_sd, &sacl_present, &sacl, &defaulted);
    if (status != STATUS_SUCCESS) return status;
    status = RtlGetDaclSecurityDescriptor(nt_sd, &dacl_present, &dacl, &defaulted);
    if (status != STATUS_SUCCESS) return status;

    if (owner_present)
        len += RtlLengthSid(owner);
    if (group_present)
        len += RtlLengthSid(group);
    if (sacl_present && sacl)
        len += sacl->AclSize;
    if (dacl_present && dacl)
        len += dacl->AclSize;

    /* fix alignment for the Unicode name that follows the structure */
    len = (len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    *server_sd = RtlAllocateHeap(GetProcessHeap(), 0, len);
    if (!*server_sd) return STATUS_NO_MEMORY;

    (*server_sd)->control = ((SECURITY_DESCRIPTOR *)nt_sd)->Control & ~SE_SELF_RELATIVE;
    (*server_sd)->owner_len = owner_present ? RtlLengthSid(owner) : 0;
    (*server_sd)->group_len = group_present ? RtlLengthSid(group) : 0;
    (*server_sd)->sacl_len = (sacl_present && sacl) ? sacl->AclSize : 0;
    (*server_sd)->dacl_len = (dacl_present && dacl) ? dacl->AclSize : 0;

    ptr = (unsigned char *)(*server_sd + 1);
    memcpy(ptr, owner, (*server_sd)->owner_len);
    ptr += (*server_sd)->owner_len;
    memcpy(ptr, group, (*server_sd)->group_len);
    ptr += (*server_sd)->group_len;
    memcpy(ptr, sacl, (*server_sd)->sacl_len);
    ptr += (*server_sd)->sacl_len;
    memcpy(ptr, dacl, (*server_sd)->dacl_len);

    *server_sd_len = len;

    return STATUS_SUCCESS;
}

/* frees a struct security_descriptor allocated by NTDLL_create_struct_sd */
void NTDLL_free_struct_sd(struct security_descriptor *server_sd)
{
    RtlFreeHeap(GetProcessHeap(), 0, server_sd);
}

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
    struct object_attributes objattr;
    struct security_descriptor *sd = NULL;

    if (MaximumCount <= 0 || InitialCount < 0 || InitialCount > MaximumCount)
        return STATUS_INVALID_PARAMETER;
    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    objattr.rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
    objattr.sd_len = 0;
    objattr.name_len = len;
    if (attr)
    {
        ret = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (ret != STATUS_SUCCESS) return ret;
    }

    SERVER_START_REQ( create_semaphore )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->initial = InitialCount;
        req->max     = MaximumCount;
        wine_server_add_data( req, &objattr, sizeof(objattr) );
        if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *SemaphoreHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    NTDLL_free_struct_sd( sd );

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
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *SemaphoreHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQuerySemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySemaphore( HANDLE handle, SEMAPHORE_INFORMATION_CLASS class,
                                  void *info, ULONG len, ULONG *ret_len )
{
    NTSTATUS ret;
    SEMAPHORE_BASIC_INFORMATION *out = info;

    if (class != SemaphoreBasicInformation)
    {
        FIXME("(%p,%d,%u) Unknown class\n", handle, class, len);
        return STATUS_INVALID_INFO_CLASS;
    }

    if (len != sizeof(SEMAPHORE_BASIC_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;

    SERVER_START_REQ( query_semaphore )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            out->CurrentCount = reply->current;
            out->MaximumCount = reply->max;
            if (ret_len) *ret_len = sizeof(SEMAPHORE_BASIC_INFORMATION);
        }
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 *  NtReleaseSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseSemaphore( HANDLE handle, ULONG count, PULONG previous )
{
    NTSTATUS ret;
    SERVER_START_REQ( release_semaphore )
    {
        req->handle = wine_server_obj_handle( handle );
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
NTSTATUS WINAPI NtCreateEvent( PHANDLE EventHandle, ACCESS_MASK DesiredAccess,
                               const OBJECT_ATTRIBUTES *attr, EVENT_TYPE type, BOOLEAN InitialState)
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;
    struct security_descriptor *sd = NULL;
    struct object_attributes objattr;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    objattr.rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
    objattr.sd_len = 0;
    objattr.name_len = len;
    if (attr)
    {
        ret = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (ret != STATUS_SUCCESS) return ret;
    }

    SERVER_START_REQ( create_event )
    {
        req->access = DesiredAccess;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->manual_reset = (type == NotificationEvent);
        req->initial_state = InitialState;
        wine_server_add_data( req, &objattr, sizeof(objattr) );
        if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *EventHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    NTDLL_free_struct_sd( sd );

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
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *EventHandle = wine_server_ptr_handle( reply->handle );
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
        req->handle = wine_server_obj_handle( handle );
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
        req->handle = wine_server_obj_handle( handle );
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
        req->handle = wine_server_obj_handle( handle );
        req->op     = PULSE_EVENT;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *  NtQueryEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryEvent( HANDLE handle, EVENT_INFORMATION_CLASS class,
                              void *info, ULONG len, ULONG *ret_len )
{
    NTSTATUS ret;
    EVENT_BASIC_INFORMATION *out = info;

    if (class != EventBasicInformation)
    {
        FIXME("(%p, %d, %d) Unknown class\n",
              handle, class, len);
        return STATUS_INVALID_INFO_CLASS;
    }

    if (len != sizeof(EVENT_BASIC_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;

    SERVER_START_REQ( query_event )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            out->EventType  = reply->manual_reset ? NotificationEvent : SynchronizationEvent;
            out->EventState = reply->state;
            if (ret_len) *ret_len = sizeof(EVENT_BASIC_INFORMATION);
        }
    }
    SERVER_END_REQ;

    return ret;
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
    NTSTATUS status;
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    struct security_descriptor *sd = NULL;
    struct object_attributes objattr;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    objattr.rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
    objattr.sd_len = 0;
    objattr.name_len = len;
    if (attr)
    {
        status = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (status != STATUS_SUCCESS) return status;
    }

    SERVER_START_REQ( create_mutex )
    {
        req->access  = access;
        req->attributes = (attr) ? attr->Attributes : 0;
        req->owned   = InitialOwner;
        wine_server_add_data( req, &objattr, sizeof(objattr) );
        if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *MutantHandle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    NTDLL_free_struct_sd( sd );

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
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *MutantHandle = wine_server_ptr_handle( reply->handle );
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
        req->handle = wine_server_obj_handle( handle );
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
 *	Jobs
 */

/******************************************************************************
 *              NtCreateJobObject   [NTDLL.@]
 *              ZwCreateJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtCreateJobObject( PHANDLE handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    FIXME( "stub: %p %x %s\n", handle, access, attr ? debugstr_us(attr->ObjectName) : "" );
    *handle = (HANDLE)0xdead;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *              NtOpenJobObject   [NTDLL.@]
 *              ZwOpenJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtOpenJobObject( PHANDLE handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    FIXME( "stub: %p %x %s\n", handle, access, attr ? debugstr_us(attr->ObjectName) : "" );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *              NtTerminateJobObject   [NTDLL.@]
 *              ZwTerminateJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtTerminateJobObject( HANDLE handle, NTSTATUS status )
{
    FIXME( "stub: %p %x\n", handle, status );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *              NtQueryInformationJobObject   [NTDLL.@]
 *              ZwQueryInformationJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtQueryInformationJobObject( HANDLE handle, JOBOBJECTINFOCLASS class, PVOID info,
                                             ULONG len, PULONG ret_len )
{
    FIXME( "stub: %p %u %p %u %p\n", handle, class, info, len, ret_len );
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *              NtSetInformationJobObject   [NTDLL.@]
 *              ZwSetInformationJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationJobObject( HANDLE handle, JOBOBJECTINFOCLASS class, PVOID info, ULONG len )
{
    FIXME( "stub: %p %u %p %u\n", handle, class, info, len );
    return STATUS_SUCCESS;
}

/******************************************************************************
 *              NtIsProcessInJob   [NTDLL.@]
 *              ZwIsProcessInJob   [NTDLL.@]
 */
NTSTATUS WINAPI NtIsProcessInJob( HANDLE process, HANDLE job )
{
    FIXME( "stub: %p %p\n", process, job );
    return STATUS_PROCESS_NOT_IN_JOB;
}

/******************************************************************************
 *              NtAssignProcessToJobObject   [NTDLL.@]
 *              ZwAssignProcessToJobObject   [NTDLL.@]
 */
NTSTATUS WINAPI NtAssignProcessToJobObject( HANDLE job, HANDLE process )
{
    FIXME( "stub: %p %p\n", job, process );
    return STATUS_SUCCESS;
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
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        req->manual  = (timer_type == NotificationTimer);
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
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
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        status = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
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
        req->handle   = wine_server_obj_handle( handle );
        req->period   = period;
        req->expire   = when->QuadPart;
        req->callback = wine_server_client_ptr( callback );
        req->arg      = wine_server_client_ptr( callback_arg );
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
        req->handle = wine_server_obj_handle( handle );
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
    TIMER_BASIC_INFORMATION * basic_info = TimerInformation;
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
            req->handle = wine_server_obj_handle( TimerHandle );
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



/* wait operations */

/******************************************************************
 *		NtWaitForMultipleObjects (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                          BOOLEAN wait_any, BOOLEAN alertable,
                                          const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT i, flags = SELECT_INTERRUPTIBLE;

    if (!count || count > MAXIMUM_WAIT_OBJECTS) return STATUS_INVALID_PARAMETER_1;

    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.wait.op = wait_any ? SELECT_WAIT : SELECT_WAIT_ALL;
    for (i = 0; i < count; i++) select_op.wait.handles[i] = wine_server_obj_handle( handles[i] );
    return server_select( &select_op, offsetof( select_op_t, wait.handles[count] ), flags, timeout );
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
    select_op_t select_op;
    UINT flags = SELECT_INTERRUPTIBLE;

    if (!hSignalObject) return STATUS_INVALID_HANDLE;

    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.signal_and_wait.op = SELECT_SIGNAL_AND_WAIT;
    select_op.signal_and_wait.wait = wine_server_obj_handle( hWaitObject );
    select_op.signal_and_wait.signal = wine_server_obj_handle( hSignalObject );
    return server_select( &select_op, sizeof(select_op.signal_and_wait), flags, timeout );
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
        return server_select( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, timeout );

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


/******************************************************************************
 *              NtCreateKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateKeyedEvent( HANDLE *handle, ACCESS_MASK access,
                                    const OBJECT_ATTRIBUTES *attr, ULONG flags )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;
    struct security_descriptor *sd = NULL;
    struct object_attributes objattr;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    objattr.rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
    objattr.sd_len = 0;
    objattr.name_len = len;
    if (attr)
    {
        ret = NTDLL_create_struct_sd( attr->SecurityDescriptor, &sd, &objattr.sd_len );
        if (ret != STATUS_SUCCESS) return ret;
    }

    SERVER_START_REQ( create_keyed_event )
    {
        req->access = access;
        req->attributes = attr ? attr->Attributes : 0;
        wine_server_add_data( req, &objattr, sizeof(objattr) );
        if (objattr.sd_len) wine_server_add_data( req, sd, objattr.sd_len );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    NTDLL_free_struct_sd( sd );
    return ret;
}

/******************************************************************************
 *              NtOpenKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKeyedEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    DWORD len = attr && attr->ObjectName ? attr->ObjectName->Length : 0;
    NTSTATUS ret;

    if (len >= MAX_PATH * sizeof(WCHAR)) return STATUS_NAME_TOO_LONG;

    SERVER_START_REQ( open_keyed_event )
    {
        req->access  = access;
        req->attributes = attr ? attr->Attributes : 0;
        req->rootdir = wine_server_obj_handle( attr ? attr->RootDirectory : 0 );
        if (len) wine_server_add_data( req, attr->ObjectName->Buffer, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************************
 *              NtWaitForKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForKeyedEvent( HANDLE handle, const void *key,
                                     BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT flags = SELECT_INTERRUPTIBLE;

    if ((ULONG_PTR)key & 1) return STATUS_INVALID_PARAMETER_1;
    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.keyed_event.op     = SELECT_KEYED_EVENT_WAIT;
    select_op.keyed_event.handle = wine_server_obj_handle( handle );
    select_op.keyed_event.key    = wine_server_client_ptr( key );
    return server_select( &select_op, sizeof(select_op.keyed_event), flags, timeout );
}

/******************************************************************************
 *              NtReleaseKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseKeyedEvent( HANDLE handle, const void *key,
                                     BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT flags = SELECT_INTERRUPTIBLE;

    if ((ULONG_PTR)key & 1) return STATUS_INVALID_PARAMETER_1;
    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.keyed_event.op     = SELECT_KEYED_EVENT_RELEASE;
    select_op.keyed_event.handle = wine_server_obj_handle( handle );
    select_op.keyed_event.key    = wine_server_client_ptr( key );
    return server_select( &select_op, sizeof(select_op.keyed_event), flags, timeout );
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
        req->rootdir    = wine_server_obj_handle( ObjectAttributes ? ObjectAttributes->RootDirectory : 0 );
        req->concurrent = NumberOfConcurrentThreads;
        if (ObjectAttributes && ObjectAttributes->ObjectName)
            wine_server_add_data( req, ObjectAttributes->ObjectName->Buffer,
                                       ObjectAttributes->ObjectName->Length );
        if (!(status = wine_server_call( req )))
            *CompletionPort = wine_server_ptr_handle( reply->handle );
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
                                   SIZE_T NumberOfBytesTransferred )
{
    NTSTATUS status;

    TRACE("(%p, %lx, %lx, %x, %lx)\n", CompletionPort, CompletionKey,
          CompletionValue, Status, NumberOfBytesTransferred);

    SERVER_START_REQ( add_completion )
    {
        req->handle      = wine_server_obj_handle( CompletionPort );
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
            req->handle = wine_server_obj_handle( CompletionPort );
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
        req->rootdir    = wine_server_obj_handle( ObjectAttributes->RootDirectory );
        wine_server_add_data( req, ObjectAttributes->ObjectName->Buffer,
                                   ObjectAttributes->ObjectName->Length );
        if (!(status = wine_server_call( req )))
            *CompletionPort = wine_server_ptr_handle( reply->handle );
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
                ULONG *info = CompletionInformation;

                if (RequiredLength) *RequiredLength = sizeof(*info);
                if (BufferLength != sizeof(*info))
                    status = STATUS_INFO_LENGTH_MISMATCH;
                else
                {
                    SERVER_START_REQ( query_completion )
                    {
                        req->handle = wine_server_obj_handle( CompletionPort );
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

NTSTATUS NTDLL_AddCompletion( HANDLE hFile, ULONG_PTR CompletionValue,
                              NTSTATUS CompletionStatus, ULONG Information )
{
    NTSTATUS status;

    SERVER_START_REQ( add_fd_completion )
    {
        req->handle      = wine_server_obj_handle( hFile );
        req->cvalue      = CompletionValue;
        req->status      = CompletionStatus;
        req->information = Information;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************
 *              RtlRunOnceInitialize (NTDLL.@)
 */
void WINAPI RtlRunOnceInitialize( RTL_RUN_ONCE *once )
{
    once->Ptr = NULL;
}

/******************************************************************
 *              RtlRunOnceBeginInitialize (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceBeginInitialize( RTL_RUN_ONCE *once, ULONG flags, void **context )
{
    if (flags & RTL_RUN_ONCE_CHECK_ONLY)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
        if ((val & 3) != 2) return STATUS_UNSUCCESSFUL;
        if (context) *context = (void *)(val & ~3);
        return STATUS_SUCCESS;
    }

    for (;;)
    {
        ULONG_PTR next, val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 0:  /* first time */
            if (!interlocked_cmpxchg_ptr( &once->Ptr,
                                          (flags & RTL_RUN_ONCE_ASYNC) ? (void *)3 : (void *)1, 0 ))
                return STATUS_PENDING;
            break;

        case 1:  /* in progress, wait */
            if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
            next = val & ~3;
            if (interlocked_cmpxchg_ptr( &once->Ptr, (void *)((ULONG_PTR)&next | 1),
                                         (void *)val ) == (void *)val)
                NtWaitForKeyedEvent( keyed_event, &next, FALSE, NULL );
            break;

        case 2:  /* done */
            if (context) *context = (void *)(val & ~3);
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            return STATUS_PENDING;
        }
    }
}

/******************************************************************
 *              RtlRunOnceComplete (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceComplete( RTL_RUN_ONCE *once, ULONG flags, void *context )
{
    if ((ULONG_PTR)context & 3) return STATUS_INVALID_PARAMETER;

    if (flags & RTL_RUN_ONCE_INIT_FAILED)
    {
        if (context) return STATUS_INVALID_PARAMETER;
        if (flags & RTL_RUN_ONCE_ASYNC) return STATUS_INVALID_PARAMETER;
    }
    else context = (void *)((ULONG_PTR)context | 2);

    for (;;)
    {
        ULONG_PTR val = (ULONG_PTR)once->Ptr;

        switch (val & 3)
        {
        case 1:  /* in progress */
            if (interlocked_cmpxchg_ptr( &once->Ptr, context, (void *)val ) != (void *)val) break;
            val &= ~3;
            while (val)
            {
                ULONG_PTR next = *(ULONG_PTR *)val;
                NtReleaseKeyedEvent( keyed_event, (void *)val, FALSE, NULL );
                val = next;
            }
            return STATUS_SUCCESS;

        case 3:  /* in progress, async */
            if (!(flags & RTL_RUN_ONCE_ASYNC)) return STATUS_INVALID_PARAMETER;
            if (interlocked_cmpxchg_ptr( &once->Ptr, context, (void *)val ) != (void *)val) break;
            return STATUS_SUCCESS;

        default:
            return STATUS_UNSUCCESSFUL;
        }
    }
}

/******************************************************************
 *              RtlRunOnceExecuteOnce (NTDLL.@)
 */
DWORD WINAPI RtlRunOnceExecuteOnce( RTL_RUN_ONCE *once, PRTL_RUN_ONCE_INIT_FN func,
                                    void *param, void **context )
{
    DWORD ret = RtlRunOnceBeginInitialize( once, 0, context );

    if (ret != STATUS_PENDING) return ret;

    if (!func( once, param, context ))
    {
        RtlRunOnceComplete( once, RTL_RUN_ONCE_INIT_FAILED, NULL );
        return STATUS_UNSUCCESSFUL;
    }

    return RtlRunOnceComplete( once, 0, context ? *context : NULL );
}


/* SRW locks implementation
 *
 * The memory layout used by the lock is:
 *
 * 32 31            16               0
 *  ________________ ________________
 * | X| #exclusive  |    #shared     |
 *  ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
 * Since there is no space left for a separate counter of shared access
 * threads inside the locked section the #shared field is used for multiple
 * purposes. The following table lists all possible states the lock can be
 * in, notation: [X, #exclusive, #shared]:
 *
 * [0,   0,   N] -> locked by N shared access threads, if N=0 its unlocked
 * [0, >=1, >=1] -> threads are requesting exclusive locks, but there are
 * still shared access threads inside. #shared should not be incremented
 * anymore!
 * [1, >=1, >=0] -> lock is owned by an exclusive thread and the #shared
 * counter can be used again to count the number of threads waiting in the
 * queue for shared access.
 *
 * the following states are invalid and will never occur:
 * [0, >=1,   0], [1,   0, >=0]
 *
 * The main problem arising from the fact that we have no separate counter
 * of shared access threads inside the locked section is that in the state
 * [0, >=1, >=1] above we cannot add additional waiting threads to the
 * shared access queue - it wouldn't be possible to distinguish waiting
 * threads and those that are still inside. To solve this problem the lock
 * uses the following approach: a thread that isn't able to allocate a
 * shared lock just uses the exclusive queue instead. As soon as the thread
 * is woken up it is in the state [1, >=1, >=0]. In this state it's again
 * possible to use the shared access queue. The thread atomically moves
 * itself to the shared access queue and releases the exclusive lock, so
 * that the "real" exclusive access threads have a chance. As soon as they
 * are all ready the shared access threads are processed.
 */

#define SRWLOCK_MASK_IN_EXCLUSIVE     0x80000000
#define SRWLOCK_MASK_EXCLUSIVE_QUEUE  0x7fff0000
#define SRWLOCK_MASK_SHARED_QUEUE     0x0000ffff
#define SRWLOCK_RES_EXCLUSIVE         0x00010000
#define SRWLOCK_RES_SHARED            0x00000001

#ifdef WORDS_BIGENDIAN
#define srwlock_key_exclusive(lock)   (&lock->Ptr)
#define srwlock_key_shared(lock)      ((void *)((char *)&lock->Ptr + 2))
#else
#define srwlock_key_exclusive(lock)   ((void *)((char *)&lock->Ptr + 2))
#define srwlock_key_shared(lock)      (&lock->Ptr)
#endif

static inline void srwlock_check_invalid( unsigned int val )
{
    /* Throw exception if it's impossible to acquire/release this lock. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) == SRWLOCK_MASK_EXCLUSIVE_QUEUE ||
            (val & SRWLOCK_MASK_SHARED_QUEUE) == SRWLOCK_MASK_SHARED_QUEUE)
        RtlRaiseStatus(STATUS_RESOURCE_NOT_OWNED);
}

static inline unsigned int srwlock_lock_exclusive( unsigned int *dest, int incr )
{
    unsigned int val, tmp;
    /* Atomically modifies the value of *dest by adding incr. If the shared
     * queue is empty and there are threads waiting for exclusive access, then
     * sets the mark SRWLOCK_MASK_IN_EXCLUSIVE to signal other threads that
     * they are allowed again to use the shared queue counter. */
    for (val = *dest;; val = tmp)
    {
        tmp = val + incr;
        srwlock_check_invalid( tmp );
        if ((tmp & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(tmp & SRWLOCK_MASK_SHARED_QUEUE))
            tmp |= SRWLOCK_MASK_IN_EXCLUSIVE;
        if ((tmp = interlocked_cmpxchg( (int *)dest, tmp, val )) == val)
            break;
    }
    return val;
}

static inline unsigned int srwlock_unlock_exclusive( unsigned int *dest, int incr )
{
    unsigned int val, tmp;
    /* Atomically modifies the value of *dest by adding incr. If the queue of
     * threads waiting for exclusive access is empty, then remove the
     * SRWLOCK_MASK_IN_EXCLUSIVE flag (only the shared queue counter will
     * remain). */
    for (val = *dest;; val = tmp)
    {
        tmp = val + incr;
        srwlock_check_invalid( tmp );
        if (!(tmp & SRWLOCK_MASK_EXCLUSIVE_QUEUE))
            tmp &= SRWLOCK_MASK_SHARED_QUEUE;
        if ((tmp = interlocked_cmpxchg( (int *)dest, tmp, val )) == val)
            break;
    }
    return val;
}

static inline void srwlock_leave_exclusive( RTL_SRWLOCK *lock, unsigned int val )
{
    /* Used when a thread leaves an exclusive section. If there are other
     * exclusive access threads they are processed first, followed by
     * the shared waiters. */
    if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
        NtReleaseKeyedEvent( keyed_event, srwlock_key_exclusive(lock), FALSE, NULL );
    else
    {
        val &= SRWLOCK_MASK_SHARED_QUEUE; /* remove SRWLOCK_MASK_IN_EXCLUSIVE */
        while (val--)
            NtReleaseKeyedEvent( keyed_event, srwlock_key_shared(lock), FALSE, NULL );
    }
}

static inline void srwlock_leave_shared( RTL_SRWLOCK *lock, unsigned int val )
{
    /* Wake up one exclusive thread as soon as the last shared access thread
     * has left. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_SHARED_QUEUE))
        NtReleaseKeyedEvent( keyed_event, srwlock_key_exclusive(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlInitializeSRWLock (NTDLL.@)
 *
 * NOTES
 *  Please note that SRWLocks do not keep track of the owner of a lock.
 *  It doesn't make any difference which thread for example unlocks an
 *  SRWLock (see corresponding tests). This implementation uses two
 *  keyed events (one for the exclusive waiters and one for the shared
 *  waiters) and is limited to 2^15-1 waiting threads.
 */
void WINAPI RtlInitializeSRWLock( RTL_SRWLOCK *lock )
{
    lock->Ptr = NULL;
}

/***********************************************************************
 *              RtlAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Unlike RtlAcquireResourceExclusive this function doesn't allow
 *  nested calls from the same thread. "Upgrading" a shared access lock
 *  to an exclusive access lock also doesn't seem to be supported.
 */
void WINAPI RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    if (srwlock_lock_exclusive( (unsigned int *)&lock->Ptr, SRWLOCK_RES_EXCLUSIVE ))
        NtWaitForKeyedEvent( keyed_event, srwlock_key_exclusive(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlAcquireSRWLockShared (NTDLL.@)
 *
 * NOTES
 *   Do not call this function recursively - it will only succeed when
 *   there are no threads waiting for an exclusive lock!
 */
void WINAPI RtlAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    unsigned int val, tmp;
    /* Acquires a shared lock. If it's currently not possible to add elements to
     * the shared queue, then request exclusive access instead. */
    for (val = *(unsigned int *)&lock->Ptr;; val = tmp)
    {
        if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_IN_EXCLUSIVE))
            tmp = val + SRWLOCK_RES_EXCLUSIVE;
        else
            tmp = val + SRWLOCK_RES_SHARED;
        if ((tmp = interlocked_cmpxchg( (int *)&lock->Ptr, tmp, val )) == val)
            break;
    }

    /* Drop exclusive access again and instead requeue for shared access. */
    if ((val & SRWLOCK_MASK_EXCLUSIVE_QUEUE) && !(val & SRWLOCK_MASK_IN_EXCLUSIVE))
    {
        NtWaitForKeyedEvent( keyed_event, srwlock_key_exclusive(lock), FALSE, NULL );
        val = srwlock_unlock_exclusive( (unsigned int *)&lock->Ptr, (SRWLOCK_RES_SHARED
                                        - SRWLOCK_RES_EXCLUSIVE) ) - SRWLOCK_RES_EXCLUSIVE;
        srwlock_leave_exclusive( lock, val );
    }

    if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
        NtWaitForKeyedEvent( keyed_event, srwlock_key_shared(lock), FALSE, NULL );
}

/***********************************************************************
 *              RtlReleaseSRWLockExclusive (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock )
{
    srwlock_leave_exclusive( lock, srwlock_unlock_exclusive( (unsigned int *)&lock->Ptr,
                             - SRWLOCK_RES_EXCLUSIVE ) - SRWLOCK_RES_EXCLUSIVE );
}

/***********************************************************************
 *              RtlReleaseSRWLockShared (NTDLL.@)
 */
void WINAPI RtlReleaseSRWLockShared( RTL_SRWLOCK *lock )
{
    srwlock_leave_shared( lock, srwlock_lock_exclusive( (unsigned int *)&lock->Ptr,
                          - SRWLOCK_RES_SHARED ) - SRWLOCK_RES_SHARED );
}

/***********************************************************************
 *              RtlTryAcquireSRWLockExclusive (NTDLL.@)
 *
 * NOTES
 *  Similar to AcquireSRWLockExclusive recusive calls are not allowed
 *  and will fail with return value FALSE.
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    return interlocked_cmpxchg( (int *)&lock->Ptr, SRWLOCK_MASK_IN_EXCLUSIVE |
                                SRWLOCK_RES_EXCLUSIVE, 0 ) == 0;
}

/***********************************************************************
 *              RtlTryAcquireSRWLockShared (NTDLL.@)
 */
BOOLEAN WINAPI RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    unsigned int val, tmp;
    for (val = *(unsigned int *)&lock->Ptr;; val = tmp)
    {
        if (val & SRWLOCK_MASK_EXCLUSIVE_QUEUE)
            return FALSE;
        if ((tmp = interlocked_cmpxchg( (int *)&lock->Ptr, val + SRWLOCK_RES_SHARED, val )) == val)
            break;
    }
    return TRUE;
}

/***********************************************************************
 *           RtlInitializeConditionVariable   (NTDLL.@)
 *
 * Initializes the condition variable with NULL.
 *
 * PARAMS
 *  variable [O] condition variable
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlInitializeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    variable->Ptr = NULL;
}

/***********************************************************************
 *           RtlWakeConditionVariable   (NTDLL.@)
 *
 * Wakes up one thread waiting on the condition variable.
 *
 * PARAMS
 *  variable [I/O] condition variable to wake up.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  The calling thread does not have to own any lock in order to call
 *  this function.
 */
void WINAPI RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    if (interlocked_dec_if_nonzero( (int *)&variable->Ptr ))
        NtReleaseKeyedEvent( keyed_event, &variable->Ptr, FALSE, NULL );
}

/***********************************************************************
 *           RtlWakeAllConditionVariable   (NTDLL.@)
 *
 * See WakeConditionVariable, wakes up all waiting threads.
 */
void WINAPI RtlWakeAllConditionVariable( RTL_CONDITION_VARIABLE *variable )
{
    int val = interlocked_xchg( (int *)&variable->Ptr, 0 );
    while (val-- > 0)
        NtReleaseKeyedEvent( keyed_event, &variable->Ptr, FALSE, NULL );
}

/***********************************************************************
 *           RtlSleepConditionVariableCS   (NTDLL.@)
 *
 * Atomically releases the critical section and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the critical section again and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  crit      [I/O] critical section to leave temporarily
 *  timeout   [I]   timeout
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 */
NTSTATUS WINAPI RtlSleepConditionVariableCS( RTL_CONDITION_VARIABLE *variable, RTL_CRITICAL_SECTION *crit,
                                             const LARGE_INTEGER *timeout )
{
    NTSTATUS status;
    interlocked_xchg_add( (int *)&variable->Ptr, 1 );
    RtlLeaveCriticalSection( crit );

    status = NtWaitForKeyedEvent( keyed_event, &variable->Ptr, FALSE, timeout );
    if (status != STATUS_SUCCESS)
    {
        if (!interlocked_dec_if_nonzero( (int *)&variable->Ptr ))
            status = NtWaitForKeyedEvent( keyed_event, &variable->Ptr, FALSE, NULL );
    }

    RtlEnterCriticalSection( crit );
    return status;
}

/***********************************************************************
 *           RtlSleepConditionVariableSRW   (NTDLL.@)
 *
 * Atomically releases the SRWLock and suspends the thread,
 * waiting for a Wake(All)ConditionVariable event. Afterwards it enters
 * the SRWLock again with the same access rights and returns.
 *
 * PARAMS
 *  variable  [I/O] condition variable
 *  lock      [I/O] SRWLock to leave temporarily
 *  timeout   [I]   timeout
 *  flags     [I]   type of the current lock (exclusive / shared)
 *
 * RETURNS
 *  see NtWaitForKeyedEvent for all possible return values.
 *
 * NOTES
 *  the behaviour is undefined if the thread doesn't own the lock.
 */
NTSTATUS WINAPI RtlSleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock,
                                              const LARGE_INTEGER *timeout, ULONG flags )
{
    NTSTATUS status;
    interlocked_xchg_add( (int *)&variable->Ptr, 1 );

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlReleaseSRWLockShared( lock );
    else
        RtlReleaseSRWLockExclusive( lock );

    status = NtWaitForKeyedEvent( keyed_event, &variable->Ptr, FALSE, timeout );
    if (status != STATUS_SUCCESS)
    {
        if (!interlocked_dec_if_nonzero( (int *)&variable->Ptr ))
            status = NtWaitForKeyedEvent( keyed_event, &variable->Ptr, FALSE, NULL );
    }

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        RtlAcquireSRWLockShared( lock );
    else
        RtlAcquireSRWLockExclusive( lock );
    return status;
}
