/*
 * Process synchronisation
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#include "windef.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sync);

#define TICKSPERSEC 10000000

HANDLE keyed_event = 0;

#ifdef __linux__

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_WAIT_BITSET 9
#define FUTEX_WAKE_BITSET 10

static int futex_private = 128;

static inline int futex_wait( const int *addr, int val, struct timespec *timeout )
{
    return syscall( __NR_futex, addr, FUTEX_WAIT | futex_private, val, timeout, 0, 0 );
}

static inline int futex_wake( const int *addr, int val )
{
    return syscall( __NR_futex, addr, FUTEX_WAKE | futex_private, val, NULL, 0, 0 );
}

static inline int futex_wait_bitset( const int *addr, int val, struct timespec *timeout, int mask )
{
    return syscall( __NR_futex, addr, FUTEX_WAIT_BITSET | futex_private, val, timeout, 0, mask );
}

static inline int futex_wake_bitset( const int *addr, int val, int mask )
{
    return syscall( __NR_futex, addr, FUTEX_WAKE_BITSET | futex_private, val, NULL, 0, mask );
}

static inline int use_futexes(void)
{
    static int supported = -1;

    if (supported == -1)
    {
        futex_wait( &supported, 10, NULL );
        if (errno == ENOSYS)
        {
            futex_private = 0;
            futex_wait( &supported, 10, NULL );
        }
        supported = (errno != ENOSYS);
    }
    return supported;
}

static int *get_futex(void **ptr)
{
    if (sizeof(void *) == 8)
        return (int *)((((ULONG_PTR)ptr) + 3) & ~3);
    else if (!(((ULONG_PTR)ptr) & 3))
        return (int *)ptr;
    else
        return NULL;
}

static void timespec_from_timeout( struct timespec *timespec, const LARGE_INTEGER *timeout )
{
    LARGE_INTEGER now;
    timeout_t diff;

    if (timeout->QuadPart > 0)
    {
        NtQuerySystemTime( &now );
        diff = timeout->QuadPart - now.QuadPart;
    }
    else
        diff = -timeout->QuadPart;

    timespec->tv_sec  = diff / TICKSPERSEC;
    timespec->tv_nsec = (diff % TICKSPERSEC) * 100;
}

#endif


/* create a struct security_descriptor and contained information in one contiguous piece of memory */
NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
                                  data_size_t *ret_len )
{
    unsigned int len = sizeof(**ret);
    PSID owner = NULL, group = NULL;
    ACL *dacl, *sacl;
    BOOLEAN dacl_present, sacl_present, defaulted;
    PSECURITY_DESCRIPTOR sd;
    NTSTATUS status;

    *ret = NULL;
    *ret_len = 0;

    if (!attr) return STATUS_SUCCESS;

    if (attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;

    if ((sd = attr->SecurityDescriptor))
    {
        len += sizeof(struct security_descriptor);

        if ((status = RtlGetOwnerSecurityDescriptor( sd, &owner, &defaulted ))) return status;
        if ((status = RtlGetGroupSecurityDescriptor( sd, &group, &defaulted ))) return status;
        if ((status = RtlGetSaclSecurityDescriptor( sd, &sacl_present, &sacl, &defaulted ))) return status;
        if ((status = RtlGetDaclSecurityDescriptor( sd, &dacl_present, &dacl, &defaulted ))) return status;
        if (owner) len += RtlLengthSid( owner );
        if (group) len += RtlLengthSid( group );
        if (sacl_present && sacl) len += sacl->AclSize;
        if (dacl_present && dacl) len += dacl->AclSize;

        /* fix alignment for the Unicode name that follows the structure */
        len = (len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    }

    if (attr->ObjectName)
    {
        if (attr->ObjectName->Length & (sizeof(WCHAR) - 1)) return STATUS_OBJECT_NAME_INVALID;
        len += attr->ObjectName->Length;
    }
    else if (attr->RootDirectory) return STATUS_OBJECT_NAME_INVALID;

    len = (len + 3) & ~3;  /* DWORD-align the entire structure */

    *ret = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
    if (!*ret) return STATUS_NO_MEMORY;

    (*ret)->rootdir = wine_server_obj_handle( attr->RootDirectory );
    (*ret)->attributes = attr->Attributes;

    if (attr->SecurityDescriptor)
    {
        struct security_descriptor *descr = (struct security_descriptor *)(*ret + 1);
        unsigned char *ptr = (unsigned char *)(descr + 1);

        descr->control = ((SECURITY_DESCRIPTOR *)sd)->Control & ~SE_SELF_RELATIVE;
        if (owner) descr->owner_len = RtlLengthSid( owner );
        if (group) descr->group_len = RtlLengthSid( group );
        if (sacl_present && sacl) descr->sacl_len = sacl->AclSize;
        if (dacl_present && dacl) descr->dacl_len = dacl->AclSize;

        memcpy( ptr, owner, descr->owner_len );
        ptr += descr->owner_len;
        memcpy( ptr, group, descr->group_len );
        ptr += descr->group_len;
        memcpy( ptr, sacl, descr->sacl_len );
        ptr += descr->sacl_len;
        memcpy( ptr, dacl, descr->dacl_len );
        (*ret)->sd_len = (sizeof(*descr) + descr->owner_len + descr->group_len + descr->sacl_len +
                          descr->dacl_len + sizeof(WCHAR) - 1) & ~(sizeof(WCHAR) - 1);
    }

    if (attr->ObjectName)
    {
        unsigned char *ptr = (unsigned char *)(*ret + 1) + (*ret)->sd_len;
        (*ret)->name_len = attr->ObjectName->Length;
        memcpy( ptr, attr->ObjectName->Buffer, (*ret)->name_len );
    }

    *ret_len = len;
    return STATUS_SUCCESS;
}


static NTSTATUS validate_open_object_attributes( const OBJECT_ATTRIBUTES *attr )
{
    if (!attr || attr->Length != sizeof(*attr)) return STATUS_INVALID_PARAMETER;

    if (attr->ObjectName)
    {
        if (attr->ObjectName->Length & (sizeof(WCHAR) - 1)) return STATUS_OBJECT_NAME_INVALID;
    }
    else if (attr->RootDirectory) return STATUS_OBJECT_NAME_INVALID;

    return STATUS_SUCCESS;
}


/******************************************************************************
 *              NtCreateSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateSemaphore( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                   LONG initial, LONG max )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if (max <= 0 || initial < 0 || initial > max) return STATUS_INVALID_PARAMETER;
    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_semaphore )
    {
        req->access  = access;
        req->initial = initial;
        req->max     = max;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}


/******************************************************************************
 *              NtOpenSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenSemaphore( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_semaphore )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQuerySemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySemaphore( HANDLE handle, SEMAPHORE_INFORMATION_CLASS class,
                                  void *info, ULONG len, ULONG *ret_len )
{
    NTSTATUS ret;
    SEMAPHORE_BASIC_INFORMATION *out = info;

    TRACE("(%p, %u, %p, %u, %p)\n", handle, class, info, len, ret_len);

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
 *              NtReleaseSemaphore (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseSemaphore( HANDLE handle, ULONG count, ULONG *previous )
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


/**************************************************************************
 *              NtCreateEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                               EVENT_TYPE type, BOOLEAN state )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_event )
    {
        req->access = access;
        req->manual_reset = (type == NotificationEvent);
        req->initial_state = state;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}


/******************************************************************************
 *              NtOpenEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_event )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtSetEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtSetEvent( HANDLE handle, LONG *prev_state )
{
    NTSTATUS ret;

    SERVER_START_REQ( event_op )
    {
        req->handle = wine_server_obj_handle( handle );
        req->op     = SET_EVENT;
        ret = wine_server_call( req );
        if (!ret && prev_state) *prev_state = reply->state;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtResetEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtResetEvent( HANDLE handle, LONG *prev_state )
{
    NTSTATUS ret;

    SERVER_START_REQ( event_op )
    {
        req->handle = wine_server_obj_handle( handle );
        req->op     = RESET_EVENT;
        ret = wine_server_call( req );
        if (!ret && prev_state) *prev_state = reply->state;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtClearEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtClearEvent( HANDLE handle )
{
    /* FIXME: same as NtResetEvent ??? */
    return NtResetEvent( handle, NULL );
}


/******************************************************************************
 *              NtPulseEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtPulseEvent( HANDLE handle, LONG *prev_state )
{
    NTSTATUS ret;

    SERVER_START_REQ( event_op )
    {
        req->handle = wine_server_obj_handle( handle );
        req->op     = PULSE_EVENT;
        ret = wine_server_call( req );
        if (!ret && prev_state) *prev_state = reply->state;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueryEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryEvent( HANDLE handle, EVENT_INFORMATION_CLASS class,
                              void *info, ULONG len, ULONG *ret_len )
{
    NTSTATUS ret;
    EVENT_BASIC_INFORMATION *out = info;

    TRACE("(%p, %u, %p, %u, %p)\n", handle, class, info, len, ret_len);

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


/******************************************************************************
 *              NtCreateMutant (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateMutant( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                BOOLEAN owned )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_mutex )
    {
        req->access  = access;
        req->owned   = owned;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}


/**************************************************************************
 *              NtOpenMutant (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenMutant( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_mutex )
    {
        req->access  = access;
        req->attributes = attr->Attributes;
        req->rootdir = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *              NtReleaseMutant (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseMutant( HANDLE handle, LONG *prev_count )
{
    NTSTATUS ret;

    SERVER_START_REQ( release_mutex )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
        if (prev_count) *prev_count = 1 - reply->prev_count;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************
 *              NtQueryMutant (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryMutant( HANDLE handle, MUTANT_INFORMATION_CLASS class,
                               void *info, ULONG len, ULONG *ret_len )
{
    NTSTATUS ret;
    MUTANT_BASIC_INFORMATION *out = info;

    TRACE("(%p, %u, %p, %u, %p)\n", handle, class, info, len, ret_len);

    if (class != MutantBasicInformation)
    {
        FIXME( "(%p, %d, %d) Unknown class\n", handle, class, len );
        return STATUS_INVALID_INFO_CLASS;
    }

    if (len != sizeof(MUTANT_BASIC_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;

    SERVER_START_REQ( query_mutex )
    {
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            out->CurrentCount   = 1 - reply->count;
            out->OwnedByCaller  = reply->owned;
            out->AbandonedState = reply->abandoned;
            if (ret_len) *ret_len = sizeof(MUTANT_BASIC_INFORMATION);
        }
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *		NtCreateTimer (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateTimer( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                               TIMER_TYPE type )
{
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if (type != NotificationTimer && type != SynchronizationTimer) return STATUS_INVALID_PARAMETER;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_timer )
    {
        req->access  = access;
        req->manual  = (type == NotificationTimer);
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;

}


/**************************************************************************
 *		NtOpenTimer (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenTimer( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_timer )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *		NtSetTimer (NTDLL.@)
 */
NTSTATUS WINAPI NtSetTimer( HANDLE handle, const LARGE_INTEGER *when, PTIMER_APC_ROUTINE callback,
                            void *arg, BOOLEAN resume, ULONG period, BOOLEAN *state )
{
    NTSTATUS ret = STATUS_SUCCESS;

    TRACE( "(%p,%p,%p,%p,%08x,0x%08x,%p)\n", handle, when, callback, arg, resume, period, state );

    SERVER_START_REQ( set_timer )
    {
        req->handle   = wine_server_obj_handle( handle );
        req->period   = period;
        req->expire   = when->QuadPart;
        req->callback = wine_server_client_ptr( callback );
        req->arg      = wine_server_client_ptr( arg );
        ret = wine_server_call( req );
        if (state) *state = reply->signaled;
    }
    SERVER_END_REQ;

    /* set error but can still succeed */
    if (resume && ret == STATUS_SUCCESS) return STATUS_TIMER_RESUME_IGNORED;
    return ret;
}


/**************************************************************************
 *		NtCancelTimer (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelTimer( HANDLE handle, BOOLEAN *state )
{
    NTSTATUS ret;

    SERVER_START_REQ( cancel_timer )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
        if (state) *state = reply->signaled;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *		NtQueryTimer (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryTimer( HANDLE handle, TIMER_INFORMATION_CLASS class,
                              void *info, ULONG len, ULONG *ret_len )
{
    TIMER_BASIC_INFORMATION *basic_info = info;
    NTSTATUS ret;
    LARGE_INTEGER now;

    TRACE( "(%p,%d,%p,0x%08x,%p)\n", handle, class, info, len, ret_len );

    switch (class)
    {
    case TimerBasicInformation:
        if (len < sizeof(TIMER_BASIC_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;

        SERVER_START_REQ( get_timer_info )
        {
            req->handle = wine_server_obj_handle( handle );
            ret = wine_server_call(req);
            /* convert server time to absolute NTDLL time */
            basic_info->RemainingTime.QuadPart = reply->when;
            basic_info->TimerState = reply->signaled;
        }
        SERVER_END_REQ;

        /* convert into relative time */
        if (basic_info->RemainingTime.QuadPart > 0) NtQuerySystemTime( &now );
        else
        {
            RtlQueryPerformanceCounter( &now );
            basic_info->RemainingTime.QuadPart = -basic_info->RemainingTime.QuadPart;
        }

        if (now.QuadPart > basic_info->RemainingTime.QuadPart)
            basic_info->RemainingTime.QuadPart = 0;
        else
            basic_info->RemainingTime.QuadPart -= now.QuadPart;

        if (ret_len) *ret_len = sizeof(TIMER_BASIC_INFORMATION);
        return ret;
    }

    FIXME( "Unhandled class %d\n", class );
    return STATUS_INVALID_INFO_CLASS;
}


/******************************************************************
 *		NtWaitForMultipleObjects (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForMultipleObjects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                                          BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT i, flags = SELECT_INTERRUPTIBLE;

    if (!count || count > MAXIMUM_WAIT_OBJECTS) return STATUS_INVALID_PARAMETER_1;

    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.wait.op = wait_any ? SELECT_WAIT : SELECT_WAIT_ALL;
    for (i = 0; i < count; i++) select_op.wait.handles[i] = wine_server_obj_handle( handles[i] );
    return server_wait( &select_op, offsetof( select_op_t, wait.handles[count] ), flags, timeout );
}


/******************************************************************
 *		NtWaitForSingleObject (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForSingleObject( HANDLE handle, BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    return NtWaitForMultipleObjects( 1, &handle, FALSE, alertable, timeout );
}


/******************************************************************
 *		NtSignalAndWaitForSingleObject (NTDLL.@)
 */
NTSTATUS WINAPI NtSignalAndWaitForSingleObject( HANDLE signal, HANDLE wait,
                                                BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT flags = SELECT_INTERRUPTIBLE;

    if (!signal) return STATUS_INVALID_HANDLE;

    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.signal_and_wait.op = SELECT_SIGNAL_AND_WAIT;
    select_op.signal_and_wait.wait = wine_server_obj_handle( wait );
    select_op.signal_and_wait.signal = wine_server_obj_handle( signal );
    return server_wait( &select_op, sizeof(select_op.signal_and_wait), flags, timeout );
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
    if (alertable) return server_wait( NULL, 0, SELECT_INTERRUPTIBLE | SELECT_ALERTABLE, timeout );

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
    NTSTATUS ret;
    data_size_t len;
    struct object_attributes *objattr;

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_keyed_event )
    {
        req->access = access;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}


/******************************************************************************
 *              NtOpenKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenKeyedEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_keyed_event )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
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

    if (!handle) handle = keyed_event;
    if ((ULONG_PTR)key & 1) return STATUS_INVALID_PARAMETER_1;
    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.keyed_event.op     = SELECT_KEYED_EVENT_WAIT;
    select_op.keyed_event.handle = wine_server_obj_handle( handle );
    select_op.keyed_event.key    = wine_server_client_ptr( key );
    return server_wait( &select_op, sizeof(select_op.keyed_event), flags, timeout );
}


/******************************************************************************
 *              NtReleaseKeyedEvent (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseKeyedEvent( HANDLE handle, const void *key,
                                     BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    select_op_t select_op;
    UINT flags = SELECT_INTERRUPTIBLE;

    if (!handle) handle = keyed_event;
    if ((ULONG_PTR)key & 1) return STATUS_INVALID_PARAMETER_1;
    if (alertable) flags |= SELECT_ALERTABLE;
    select_op.keyed_event.op     = SELECT_KEYED_EVENT_RELEASE;
    select_op.keyed_event.handle = wine_server_obj_handle( handle );
    select_op.keyed_event.key    = wine_server_client_ptr( key );
    return server_wait( &select_op, sizeof(select_op.keyed_event), flags, timeout );
}


/***********************************************************************
 *             NtCreateSection (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                 const LARGE_INTEGER *size, ULONG protect,
                                 ULONG sec_flags, HANDLE file )
{
    NTSTATUS ret;
    unsigned int file_access;
    data_size_t len;
    struct object_attributes *objattr;

    switch (protect & 0xff)
    {
    case PAGE_READONLY:
    case PAGE_EXECUTE_READ:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_WRITECOPY:
        file_access = FILE_READ_DATA;
        break;
    case PAGE_READWRITE:
    case PAGE_EXECUTE_READWRITE:
        if (sec_flags & SEC_IMAGE) file_access = FILE_READ_DATA;
        else file_access = FILE_READ_DATA | FILE_WRITE_DATA;
        break;
    case PAGE_EXECUTE:
    case PAGE_NOACCESS:
        file_access = 0;
        break;
    default:
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    if ((ret = alloc_object_attributes( attr, &objattr, &len ))) return ret;

    SERVER_START_REQ( create_mapping )
    {
        req->access      = access;
        req->flags       = sec_flags;
        req->file_handle = wine_server_obj_handle( file );
        req->file_access = file_access;
        req->size        = size ? size->QuadPart : 0;
        wine_server_add_data( req, objattr, len );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    RtlFreeHeap( GetProcessHeap(), 0, objattr );
    return ret;
}


/***********************************************************************
 *             NtOpenSection (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS ret;

    if ((ret = validate_open_object_attributes( attr ))) return ret;

    SERVER_START_REQ( open_mapping )
    {
        req->access     = access;
        req->attributes = attr->Attributes;
        req->rootdir    = wine_server_obj_handle( attr->RootDirectory );
        if (attr->ObjectName)
            wine_server_add_data( req, attr->ObjectName->Buffer, attr->ObjectName->Length );
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return ret;
}


#ifdef __linux__

/* Futex-based SRW lock implementation:
 *
 * Since we can rely on the kernel to release all threads and don't need to
 * worry about NtReleaseKeyedEvent(), we can simplify the layout a bit. The
 * layout looks like this:
 *
 *    31 - Exclusive lock bit, set if the resource is owned exclusively.
 * 30-16 - Number of exclusive waiters. Unlike the fallback implementation,
 *         this does not include the thread owning the lock, or shared threads
 *         waiting on the lock.
 *    15 - Does this lock have any shared waiters? We use this as an
 *         optimization to avoid unnecessary FUTEX_WAKE_BITSET calls when
 *         releasing an exclusive lock.
 *  14-0 - Number of shared owners. Unlike the fallback implementation, this
 *         does not include the number of shared threads waiting on the lock.
 *         Thus the state [1, x, >=1] will never occur.
 */

#define SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT        0x80000000
#define SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK    0x7fff0000
#define SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_INC     0x00010000
#define SRWLOCK_FUTEX_SHARED_WAITERS_BIT        0x00008000
#define SRWLOCK_FUTEX_SHARED_OWNERS_MASK        0x00007fff
#define SRWLOCK_FUTEX_SHARED_OWNERS_INC         0x00000001

/* Futex bitmasks; these are independent from the bits in the lock itself. */
#define SRWLOCK_FUTEX_BITSET_EXCLUSIVE  1
#define SRWLOCK_FUTEX_BITSET_SHARED     2

NTSTATUS CDECL fast_RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    int old, new, *futex;
    NTSTATUS ret;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    do
    {
        old = *futex;

        if (!(old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT)
                && !(old & SRWLOCK_FUTEX_SHARED_OWNERS_MASK))
        {
            /* Not locked exclusive or shared. We can try to grab it. */
            new = old | SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT;
            ret = STATUS_SUCCESS;
        }
        else
        {
            new = old;
            ret = STATUS_TIMEOUT;
        }
    } while (InterlockedCompareExchange( futex, new, old ) != old);

    return ret;
}

NTSTATUS CDECL fast_RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    int old, new, *futex;
    BOOLEAN wait;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    /* Atomically increment the exclusive waiter count. */
    do
    {
        old = *futex;
        new = old + SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_INC;
        assert(new & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK);
    } while (InterlockedCompareExchange( futex, new, old ) != old);

    for (;;)
    {
        do
        {
            old = *futex;

            if (!(old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT)
                    && !(old & SRWLOCK_FUTEX_SHARED_OWNERS_MASK))
            {
                /* Not locked exclusive or shared. We can try to grab it. */
                new = old | SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT;
                assert(old & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK);
                new -= SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_INC;
                wait = FALSE;
            }
            else
            {
                new = old;
                wait = TRUE;
            }
        } while (InterlockedCompareExchange( futex, new, old ) != old);

        if (!wait)
            return STATUS_SUCCESS;

        futex_wait_bitset( futex, new, NULL, SRWLOCK_FUTEX_BITSET_EXCLUSIVE );
    }

    return STATUS_SUCCESS;
}

NTSTATUS CDECL fast_RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    int new, old, *futex;
    NTSTATUS ret;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    do
    {
        old = *futex;

        if (!(old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT)
                && !(old & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK))
        {
            /* Not locked exclusive, and no exclusive waiters. We can try to
             * grab it. */
            new = old + SRWLOCK_FUTEX_SHARED_OWNERS_INC;
            assert(new & SRWLOCK_FUTEX_SHARED_OWNERS_MASK);
            ret = STATUS_SUCCESS;
        }
        else
        {
            new = old;
            ret = STATUS_TIMEOUT;
        }
    } while (InterlockedCompareExchange( futex, new, old ) != old);

    return ret;
}

NTSTATUS CDECL fast_RtlAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    int old, new, *futex;
    BOOLEAN wait;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    for (;;)
    {
        do
        {
            old = *futex;

            if (!(old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT)
                    && !(old & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK))
            {
                /* Not locked exclusive, and no exclusive waiters. We can try
                 * to grab it. */
                new = old + SRWLOCK_FUTEX_SHARED_OWNERS_INC;
                assert(new & SRWLOCK_FUTEX_SHARED_OWNERS_MASK);
                wait = FALSE;
            }
            else
            {
                new = old | SRWLOCK_FUTEX_SHARED_WAITERS_BIT;
                wait = TRUE;
            }
        } while (InterlockedCompareExchange( futex, new, old ) != old);

        if (!wait)
            return STATUS_SUCCESS;

        futex_wait_bitset( futex, new, NULL, SRWLOCK_FUTEX_BITSET_SHARED );
    }

    return STATUS_SUCCESS;
}

NTSTATUS CDECL fast_RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock )
{
    int old, new, *futex;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    do
    {
        old = *futex;

        if (!(old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT))
        {
            ERR("Lock %p is not owned exclusive! (%#x)\n", lock, *futex);
            return STATUS_RESOURCE_NOT_OWNED;
        }

        new = old & ~SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT;

        if (!(new & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK))
            new &= ~SRWLOCK_FUTEX_SHARED_WAITERS_BIT;
    } while (InterlockedCompareExchange( futex, new, old ) != old);

    if (new & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK)
        futex_wake_bitset( futex, 1, SRWLOCK_FUTEX_BITSET_EXCLUSIVE );
    else if (old & SRWLOCK_FUTEX_SHARED_WAITERS_BIT)
        futex_wake_bitset( futex, INT_MAX, SRWLOCK_FUTEX_BITSET_SHARED );

    return STATUS_SUCCESS;
}

NTSTATUS CDECL fast_RtlReleaseSRWLockShared( RTL_SRWLOCK *lock )
{
    int old, new, *futex;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &lock->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    do
    {
        old = *futex;

        if (old & SRWLOCK_FUTEX_EXCLUSIVE_LOCK_BIT)
        {
            ERR("Lock %p is owned exclusive! (%#x)\n", lock, *futex);
            return STATUS_RESOURCE_NOT_OWNED;
        }
        else if (!(old & SRWLOCK_FUTEX_SHARED_OWNERS_MASK))
        {
            ERR("Lock %p is not owned shared! (%#x)\n", lock, *futex);
            return STATUS_RESOURCE_NOT_OWNED;
        }

        new = old - SRWLOCK_FUTEX_SHARED_OWNERS_INC;
    } while (InterlockedCompareExchange( futex, new, old ) != old);

    /* Optimization: only bother waking if there are actually exclusive waiters. */
    if (!(new & SRWLOCK_FUTEX_SHARED_OWNERS_MASK) && (new & SRWLOCK_FUTEX_EXCLUSIVE_WAITERS_MASK))
        futex_wake_bitset( futex, 1, SRWLOCK_FUTEX_BITSET_EXCLUSIVE );

    return STATUS_SUCCESS;
}

static NTSTATUS wait_cv( int *futex, int val, const LARGE_INTEGER *timeout )
{
    struct timespec timespec;
    int ret;

    if (timeout && timeout->QuadPart != TIMEOUT_INFINITE)
    {
        timespec_from_timeout( &timespec, timeout );
        ret = futex_wait( futex, val, &timespec );
    }
    else
        ret = futex_wait( futex, val, NULL );

    if (ret == -1 && errno == ETIMEDOUT)
        return STATUS_TIMEOUT;
    return STATUS_WAIT_0;
}

NTSTATUS CDECL fast_RtlSleepConditionVariableCS( RTL_CONDITION_VARIABLE *variable,
                                                 RTL_CRITICAL_SECTION *cs, const LARGE_INTEGER *timeout )
{
    NTSTATUS status;
    int val, *futex;

    if (!use_futexes())
        return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &variable->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    val = *futex;

    RtlLeaveCriticalSection( cs );
    status = wait_cv( futex, val, timeout );
    RtlEnterCriticalSection( cs );
    return status;
}

NTSTATUS CDECL fast_RtlSleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock,
                                                  const LARGE_INTEGER *timeout, ULONG flags )
{
    NTSTATUS status;
    int val, *futex;

    if (!use_futexes())
        return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &variable->Ptr )) || !get_futex( &lock->Ptr ))
        return STATUS_NOT_IMPLEMENTED;

    val = *futex;

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        fast_RtlReleaseSRWLockShared( lock );
    else
        fast_RtlReleaseSRWLockExclusive( lock );

    status = wait_cv( futex, val, timeout );

    if (flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED)
        fast_RtlAcquireSRWLockShared( lock );
    else
        fast_RtlAcquireSRWLockExclusive( lock );
    return status;
}

NTSTATUS CDECL fast_RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable, int count )
{
    int *futex;

    if (!use_futexes()) return STATUS_NOT_IMPLEMENTED;

    if (!(futex = get_futex( &variable->Ptr )))
        return STATUS_NOT_IMPLEMENTED;

    InterlockedIncrement( futex );
    futex_wake( futex, count );
    return STATUS_SUCCESS;
}

#else

NTSTATUS CDECL fast_RtlTryAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlAcquireSRWLockExclusive( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlTryAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlAcquireSRWLockShared( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlReleaseSRWLockExclusive( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlReleaseSRWLockShared( RTL_SRWLOCK *lock )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlSleepConditionVariableSRW( RTL_CONDITION_VARIABLE *variable, RTL_SRWLOCK *lock,
                                                  const LARGE_INTEGER *timeout, ULONG flags )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlSleepConditionVariableCS( RTL_CONDITION_VARIABLE *variable,
                                                 RTL_CRITICAL_SECTION *cs, const LARGE_INTEGER *timeout )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS CDECL fast_RtlWakeConditionVariable( RTL_CONDITION_VARIABLE *variable, int count )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif
