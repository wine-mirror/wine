/*
 * NT threads support
 *
 * Copyright 1996, 2003 Alexandre Julliard
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

#include "ntstatus.h"
#include "thread.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *              NtOpenThread   (NTDLL.@)
 *              ZwOpenThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThread( HANDLE *handle, ACCESS_MASK access,
                              const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    NTSTATUS ret;

    SERVER_START_REQ( open_thread )
    {
        req->tid     = (thread_id_t)id->UniqueThread;
        req->access  = access;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        ret = wine_server_call( req );
        *handle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtSuspendThread   (NTDLL.@)
 *              ZwSuspendThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtSuspendThread( HANDLE handle, PULONG count )
{
    NTSTATUS ret;

    SERVER_START_REQ( suspend_thread )
    {
        req->handle = handle;
        if (!(ret = wine_server_call( req ))) *count = reply->count;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtResumeThread   (NTDLL.@)
 *              ZwResumeThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtResumeThread( HANDLE handle, PULONG count )
{
    NTSTATUS ret;

    SERVER_START_REQ( resume_thread )
    {
        req->handle = handle;
        if (!(ret = wine_server_call( req ))) *count = reply->count;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtTerminateThread  (NTDLL.@)
 *              ZwTerminateThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtTerminateThread( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self, last;

    SERVER_START_REQ( terminate_thread )
    {
        req->handle    = handle;
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
        last = reply->last;
    }
    SERVER_END_REQ;

    if (self)
    {
        if (last) exit( exit_code );
        else
        {
            RtlAcquirePebLock();
            RemoveEntryList( &NtCurrentTeb()->TlsLinks );
            RtlReleasePebLock();
            SYSDEPS_ExitThread( exit_code );
        }
    }
    return ret;
}


/******************************************************************************
 *              NtQueueApcThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueueApcThread( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                  ULONG_PTR arg2, ULONG_PTR arg3 )
{
    NTSTATUS ret;
    SERVER_START_REQ( queue_apc )
    {
        req->handle = handle;
        req->user   = 1;
        req->func   = func;
        req->arg1   = (void *)arg1;
        req->arg2   = (void *)arg2;
        req->arg3   = (void *)arg3;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret;

    SERVER_START_REQ( set_thread_context )
    {
        req->handle = handle;
        req->flags  = context->ContextFlags;
        wine_server_add_data( req, context, sizeof(*context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;

    SERVER_START_REQ( get_thread_context )
    {
        req->handle = handle;
        req->flags = context->ContextFlags;
        wine_server_add_data( req, context, sizeof(*context) );
        wine_server_set_reply( req, context, sizeof(*context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueryInformationThread  (NTDLL.@)
 *              ZwQueryInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationThread( HANDLE handle, THREADINFOCLASS class,
                                          void *data, ULONG length, ULONG *ret_len )
{
    NTSTATUS status;

    switch(class)
    {
    case ThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION info;

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = handle;
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                {
                    info.ExitStatus             = reply->exit_code;
                    info.TebBaseAddress         = reply->teb;
                    info.ClientId.UniqueProcess = (HANDLE)reply->pid;
                    info.ClientId.UniqueThread  = (HANDLE)reply->tid;
                    info.AffinityMask           = reply->affinity;
                    info.Priority               = reply->priority;
                    info.BasePriority           = reply->priority;  /* FIXME */
                }
            }
            SERVER_END_REQ;
            if (status == STATUS_SUCCESS)
            {
                if (data) memcpy( data, &info, min( length, sizeof(info) ));
                if (ret_len) *ret_len = min( length, sizeof(info) );
            }
        }
        return status;
    case ThreadTimes:
    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadAffinityMask:
    case ThreadImpersonationToken:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadQuerySetWin32StartAddress:
    case ThreadZeroTlsCell:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    case ThreadIsIoPending:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/******************************************************************************
 *              NtSetInformationThread  (NTDLL.@)
 *              ZwSetInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationThread( HANDLE handle, THREADINFOCLASS class,
                                        LPCVOID data, ULONG length )
{
    switch(class)
    {
    case ThreadZeroTlsCell:
        if (handle == GetCurrentThread())
        {
            LIST_ENTRY *entry = &NtCurrentTeb()->TlsLinks;
            DWORD index;

            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            index = *(DWORD *)data;
            if (index >= 64) return STATUS_INVALID_PARAMETER;
            RtlAcquirePebLock();
            do
            {
                TEB *teb = CONTAINING_RECORD(entry, TEB, TlsLinks);
                teb->TlsSlots[index] = 0;
                entry = entry->Flink;
            } while (entry != &NtCurrentTeb()->TlsLinks);
            RtlReleasePebLock();
            return STATUS_SUCCESS;
        }
        FIXME( "ZeroTlsCell not supported on other threads\n" );
        return STATUS_NOT_IMPLEMENTED;

    case ThreadBasicInformation:
    case ThreadTimes:
    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadAffinityMask:
    case ThreadImpersonationToken:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadQuerySetWin32StartAddress:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    case ThreadIsIoPending:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}
