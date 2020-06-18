/*
 * NT process handling
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 2018 Alexandre Julliard
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

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "windef.h"
#include "winternl.h"
#include "ntdll_misc.h"
#include "wine/exception.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));


/*
 *	Process object
 */

/******************************************************************************
 *  NtTerminateProcess			[NTDLL.@]
 *
 *  Native applications must kill themselves when done
 */
NTSTATUS WINAPI NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    return unix_funcs->NtTerminateProcess( handle, exit_code );
}

/******************************************************************************
 *  RtlGetCurrentPeb  [NTDLL.@]
 *
 */
PEB * WINAPI RtlGetCurrentPeb(void)
{
    return NtCurrentTeb()->Peb;
}

/***********************************************************************
 *           __wine_make_process_system   (NTDLL.@)
 *
 * Mark the current process as a system process.
 * Returns the event that is signaled when all non-system processes have exited.
 */
HANDLE CDECL __wine_make_process_system(void)
{
    HANDLE ret = 0;
    SERVER_START_REQ( make_process_system )
    {
        if (!wine_server_call( req )) ret = wine_server_ptr_handle( reply->event );
    }
    SERVER_END_REQ;
    return ret;
}

ULONG_PTR get_system_affinity_mask(void)
{
    ULONG num_cpus = NtCurrentTeb()->Peb->NumberOfProcessors;
    if (num_cpus >= sizeof(ULONG_PTR) * 8) return ~(ULONG_PTR)0;
    return ((ULONG_PTR)1 << num_cpus) - 1;
}

/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.@]
*  ZwQueryInformationProcess		[NTDLL.@]
*
*/
NTSTATUS WINAPI NtQueryInformationProcess( HANDLE handle, PROCESSINFOCLASS class, void *info,
                                           ULONG size, ULONG *ret_len )
{
    return unix_funcs->NtQueryInformationProcess( handle, class, info, size, ret_len );
}

/******************************************************************************
 * NtSetInformationProcess [NTDLL.@]
 * ZwSetInformationProcess [NTDLL.@]
 */
NTSTATUS WINAPI NtSetInformationProcess( HANDLE handle, PROCESSINFOCLASS class, void *info, ULONG size )
{
    return unix_funcs->NtSetInformationProcess( handle, class, info, size );
}

/******************************************************************************
 * NtFlushInstructionCache [NTDLL.@]
 * ZwFlushInstructionCache [NTDLL.@]
 */
NTSTATUS WINAPI NtFlushInstructionCache( HANDLE handle, const void *addr, SIZE_T size )
{
#if defined(__x86_64__) || defined(__i386__)
    /* no-op */
#elif defined(HAVE___CLEAR_CACHE)
    if (handle == GetCurrentProcess())
    {
        __clear_cache( (char *)addr, (char *)addr + size );
    }
    else
    {
        static int once;
        if (!once++) FIXME( "%p %p %ld other process not supported\n", handle, addr, size );
    }
#else
    static int once;
    if (!once++) FIXME( "%p %p %ld\n", handle, addr, size );
#endif
    return STATUS_SUCCESS;
}

/**********************************************************************
 * NtFlushProcessWriteBuffers [NTDLL.@]
 */
void WINAPI NtFlushProcessWriteBuffers(void)
{
    static int once = 0;
    if (!once++) FIXME( "stub\n" );
}

/******************************************************************
 *		NtOpenProcess [NTDLL.@]
 *		ZwOpenProcess [NTDLL.@]
 */
NTSTATUS  WINAPI NtOpenProcess(PHANDLE handle, ACCESS_MASK access,
                               const OBJECT_ATTRIBUTES* attr, const CLIENT_ID* cid)
{
    NTSTATUS    status;

    SERVER_START_REQ( open_process )
    {
        req->pid        = HandleToULong(cid->UniqueProcess);
        req->access     = access;
        req->attributes = attr ? attr->Attributes : 0;
        status = wine_server_call( req );
        if (!status) *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
}

/******************************************************************************
 * NtResumeProcess
 * ZwResumeProcess
 */
NTSTATUS WINAPI NtResumeProcess( HANDLE handle )
{
    NTSTATUS ret;

    SERVER_START_REQ( resume_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}

/******************************************************************************
 * NtSuspendProcess
 * ZwSuspendProcess
 */
NTSTATUS WINAPI NtSuspendProcess( HANDLE handle )
{
    NTSTATUS ret;

    SERVER_START_REQ( suspend_process )
    {
        req->handle = wine_server_obj_handle( handle );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *           restart_process
 */
NTSTATUS restart_process( RTL_USER_PROCESS_PARAMETERS *params, NTSTATUS status )
{
    static const WCHAR argsW[] = {'%','s','%','s',' ','-','-','a','p','p','-','n','a','m','e',' ','"','%','s','"',' ','%','s',0};
    static const WCHAR winevdm[] = {'w','i','n','e','v','d','m','.','e','x','e',0};
    static const WCHAR comW[] = {'.','c','o','m',0};
    static const WCHAR pifW[] = {'.','p','i','f',0};

    WCHAR *p, *cmdline;
    UNICODE_STRING pathW, cmdW;

    /* check for .com or .pif extension */
    if (status == STATUS_INVALID_IMAGE_NOT_MZ &&
        (p = wcsrchr( params->ImagePathName.Buffer, '.' )) &&
        (!wcsicmp( p, comW ) || !wcsicmp( p, pifW )))
        status = STATUS_INVALID_IMAGE_WIN_16;

    switch (status)
    {
    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_NO_MEMORY:
    case STATUS_INVALID_IMAGE_FORMAT:
    case STATUS_INVALID_IMAGE_NOT_MZ:
        if (!RtlDosPathNameToNtPathName_U( params->ImagePathName.Buffer, &pathW, NULL, NULL ))
            return status;
        status = unix_funcs->exec_process( &pathW, &params->CommandLine, status );
        break;
    case STATUS_INVALID_IMAGE_WIN_16:
    case STATUS_INVALID_IMAGE_NE_FORMAT:
    case STATUS_INVALID_IMAGE_PROTECT:
        cmdline = RtlAllocateHeap( GetProcessHeap(), 0,
                                   (wcslen(system_dir) + wcslen(winevdm) + 16 +
                                    wcslen(params->ImagePathName.Buffer) +
                                    wcslen(params->CommandLine.Buffer)) * sizeof(WCHAR));
        if (!cmdline) return STATUS_NO_MEMORY;
        NTDLL_swprintf( cmdline, argsW, (is_win64 || is_wow64) ? syswow64_dir : system_dir,
                  winevdm, params->ImagePathName.Buffer, params->CommandLine.Buffer );
        RtlInitUnicodeString( &pathW, winevdm );
        RtlInitUnicodeString( &cmdW, cmdline );
        status = unix_funcs->exec_process( &pathW, &cmdW, status );
        break;
    }
    return status;
}


/**********************************************************************
 *           NtCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateUserProcess( HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr,
                                     ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                     OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                     ULONG process_flags, ULONG thread_flags,
                                     RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                     PS_ATTRIBUTE_LIST *attr )
{
    return unix_funcs->NtCreateUserProcess( process_handle_ptr, thread_handle_ptr,
                                            process_access, thread_access,
                                            process_attr, thread_attr,
                                            process_flags, thread_flags,
                                            params, info, attr );
}


/**********************************************************************
 *           RtlCreateUserProcess  (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserProcess( UNICODE_STRING *path, ULONG attributes,
                                      RTL_USER_PROCESS_PARAMETERS *params,
                                      SECURITY_DESCRIPTOR *process_descr,
                                      SECURITY_DESCRIPTOR *thread_descr,
                                      HANDLE parent, BOOLEAN inherit, HANDLE debug, HANDLE exception,
                                      RTL_USER_PROCESS_INFORMATION *info )
{
    OBJECT_ATTRIBUTES process_attr, thread_attr;
    PS_CREATE_INFO create_info;
    ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[5] ) / sizeof(ULONG_PTR)];
    PS_ATTRIBUTE_LIST *attr = (PS_ATTRIBUTE_LIST *)buffer;
    UINT pos = 0;

    RtlNormalizeProcessParams( params );

    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
    attr->Attributes[pos].Size         = path->Length;
    attr->Attributes[pos].ValuePtr     = path->Buffer;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_CLIENT_ID;
    attr->Attributes[pos].Size         = sizeof(info->ClientId);
    attr->Attributes[pos].ValuePtr     = &info->ClientId;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_IMAGE_INFO;
    attr->Attributes[pos].Size         = sizeof(info->ImageInformation);
    attr->Attributes[pos].ValuePtr     = &info->ImageInformation;
    attr->Attributes[pos].ReturnLength = NULL;
    pos++;
    if (parent)
    {
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_PARENT_PROCESS;
        attr->Attributes[pos].Size         = sizeof(parent);
        attr->Attributes[pos].ValuePtr     = parent;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
    }
    if (debug)
    {
        attr->Attributes[pos].Attribute    = PS_ATTRIBUTE_DEBUG_PORT;
        attr->Attributes[pos].Size         = sizeof(debug);
        attr->Attributes[pos].ValuePtr     = debug;
        attr->Attributes[pos].ReturnLength = NULL;
        pos++;
    }
    attr->TotalLength = offsetof( PS_ATTRIBUTE_LIST, Attributes[pos] );

    InitializeObjectAttributes( &process_attr, NULL, 0, NULL, process_descr );
    InitializeObjectAttributes( &thread_attr, NULL, 0, NULL, thread_descr );

    return NtCreateUserProcess( &info->Process, &info->Thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                &process_attr, &thread_attr,
                                inherit ? PROCESS_CREATE_FLAGS_INHERIT_HANDLES : 0,
                                THREAD_CREATE_FLAGS_CREATE_SUSPENDED, params,
                                &create_info, attr );
}

/***********************************************************************
 *      DbgUiRemoteBreakin (NTDLL.@)
 */
void WINAPI DbgUiRemoteBreakin( void *arg )
{
    TRACE( "\n" );
    if (NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            DbgBreakPoint();
        }
        __EXCEPT_ALL
        {
            /* do nothing */
        }
        __ENDTRY
    }
    RtlExitUserThread( STATUS_SUCCESS );
}

/***********************************************************************
 *      DbgUiIssueRemoteBreakin (NTDLL.@)
 */
NTSTATUS WINAPI DbgUiIssueRemoteBreakin( HANDLE process )
{
    return unix_funcs->DbgUiIssueRemoteBreakin( process );
}
