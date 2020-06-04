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


/* create a struct security_descriptor and contained information in one contiguous piece of memory */
static NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes **ret,
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
