/*
 * WoW64 process (and thread) functions
 *
 * Copyright 2021 Alexandre Julliard
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winternl.h"
#include "wow64_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


static SIZE_T get_machine_context_size( USHORT machine )
{
    switch (machine)
    {
    case IMAGE_FILE_MACHINE_I386:  return sizeof(I386_CONTEXT);
    case IMAGE_FILE_MACHINE_ARMNT: return sizeof(ARM_CONTEXT);
    case IMAGE_FILE_MACHINE_AMD64: return sizeof(AMD64_CONTEXT);
    case IMAGE_FILE_MACHINE_ARM64: return sizeof(ARM64_NT_CONTEXT);
    default: return 0;
    }
}


static BOOL is_process_wow64( HANDLE handle )
{
    ULONG_PTR info;

    if (handle == GetCurrentProcess()) return TRUE;
    if (NtQueryInformationProcess( handle, ProcessWow64Information, &info, sizeof(info), NULL ))
        return FALSE;
    return !!info;
}


static RTL_USER_PROCESS_PARAMETERS *process_params_32to64( RTL_USER_PROCESS_PARAMETERS **params,
                                                           RTL_USER_PROCESS_PARAMETERS32 *params32 )
{
    UNICODE_STRING image, dllpath, curdir, cmdline, title, desktop, shell, runtime;
    RTL_USER_PROCESS_PARAMETERS *ret;

    *params = NULL;
    if (RtlCreateProcessParametersEx( &ret, unicode_str_32to64( &image, &params32->ImagePathName ),
                                      unicode_str_32to64( &dllpath, &params32->DllPath ),
                                      unicode_str_32to64( &curdir, &params32->CurrentDirectory.DosPath ),
                                      unicode_str_32to64( &cmdline, &params32->CommandLine ),
                                      ULongToPtr( params32->Environment ),
                                      unicode_str_32to64( &title, &params32->WindowTitle ),
                                      unicode_str_32to64( &desktop, &params32->Desktop ),
                                      unicode_str_32to64( &shell, &params32->ShellInfo ),
                                      unicode_str_32to64( &runtime, &params32->RuntimeInfo ),
                                      PROCESS_PARAMS_FLAG_NORMALIZED ))
        return NULL;

    ret->DebugFlags            = params32->DebugFlags;
    ret->ConsoleHandle         = LongToHandle( params32->ConsoleHandle );
    ret->ConsoleFlags          = params32->ConsoleFlags;
    ret->hStdInput             = LongToHandle( params32->hStdInput );
    ret->hStdOutput            = LongToHandle( params32->hStdOutput );
    ret->hStdError             = LongToHandle( params32->hStdError );
    ret->dwX                   = params32->dwX;
    ret->dwY                   = params32->dwY;
    ret->dwXSize               = params32->dwXSize;
    ret->dwYSize               = params32->dwYSize;
    ret->dwXCountChars         = params32->dwXCountChars;
    ret->dwYCountChars         = params32->dwYCountChars;
    ret->dwFillAttribute       = params32->dwFillAttribute;
    ret->dwFlags               = params32->dwFlags;
    ret->wShowWindow           = params32->wShowWindow;
    ret->EnvironmentVersion    = params32->EnvironmentVersion;
    ret->PackageDependencyData = ULongToPtr( params32->PackageDependencyData );
    ret->ProcessGroupId        = params32->ProcessGroupId;
    ret->LoaderThreads         = params32->LoaderThreads;
    *params = ret;
    return ret;
}


static void put_ps_create_info( PS_CREATE_INFO32 *info32, const PS_CREATE_INFO *info )
{
    info32->State = info->State;
    switch (info->State)
    {
    case PsCreateInitialState:
        info32->InitState.InitFlags            = info->InitState.InitFlags;
        info32->InitState.AdditionalFileAccess = info->InitState.AdditionalFileAccess;
        break;
    case PsCreateFailOnSectionCreate:
        info32->FailSection.FileHandle = HandleToLong( info->FailSection.FileHandle );
        break;
    case PsCreateFailExeFormat:
        info32->ExeFormat.DllCharacteristics = info->ExeFormat.DllCharacteristics;
        break;
    case PsCreateFailExeName:
        info32->ExeName.IFEOKey = HandleToLong( info->ExeName.IFEOKey );
        break;
    case PsCreateSuccess:
        info32->SuccessState.OutputFlags                 = info->SuccessState.OutputFlags;
        info32->SuccessState.FileHandle                  = HandleToLong( info->SuccessState.FileHandle );
        info32->SuccessState.SectionHandle               = HandleToLong( info->SuccessState.SectionHandle );
        info32->SuccessState.UserProcessParametersNative = info->SuccessState.UserProcessParametersNative;
        info32->SuccessState.UserProcessParametersWow64  = info->SuccessState.UserProcessParametersWow64;
        info32->SuccessState.CurrentParameterFlags       = info->SuccessState.CurrentParameterFlags;
        info32->SuccessState.PebAddressNative            = info->SuccessState.PebAddressNative;
        info32->SuccessState.PebAddressWow64             = info->SuccessState.PebAddressWow64;
        info32->SuccessState.ManifestAddress             = info->SuccessState.ManifestAddress;
        info32->SuccessState.ManifestSize                = info->SuccessState.ManifestSize;
        break;
    default:
        break;
    }
}


static PS_ATTRIBUTE_LIST *ps_attributes_32to64( PS_ATTRIBUTE_LIST **attr, const PS_ATTRIBUTE_LIST32 *attr32 )
{
    PS_ATTRIBUTE_LIST *ret;
    ULONG i, count;

    if (!attr32) return NULL;
    count = (attr32->TotalLength - sizeof(attr32->TotalLength)) / sizeof(PS_ATTRIBUTE32);
    ret = Wow64AllocateTemp( offsetof(PS_ATTRIBUTE_LIST, Attributes[count]) );
    ret->TotalLength = offsetof( PS_ATTRIBUTE_LIST, Attributes[count] );
    for (i = 0; i < count; i++)
    {
        ret->Attributes[i].Attribute    = attr32->Attributes[i].Attribute;
        ret->Attributes[i].Size         = attr32->Attributes[i].Size;
        ret->Attributes[i].Value        = attr32->Attributes[i].Value;
        ret->Attributes[i].ReturnLength = NULL;
        switch (ret->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_IMAGE_NAME:
            {
                OBJECT_ATTRIBUTES attr;
                UNICODE_STRING path;

                path.Length = ret->Attributes[i].Size;
                path.Buffer = ret->Attributes[i].ValuePtr;
                InitializeObjectAttributes( &attr, &path, OBJ_CASE_INSENSITIVE, 0, 0 );
                if (get_file_redirect( &attr ))
                {
                    ret->Attributes[i].Size = attr.ObjectName->Length;
                    ret->Attributes[i].ValuePtr = attr.ObjectName->Buffer;
                }
            }
            break;
        case PS_ATTRIBUTE_HANDLE_LIST:
        case PS_ATTRIBUTE_JOB_LIST:
            {
                ULONG j, handles_count = attr32->Attributes[i].Size / sizeof(ULONG);

                ret->Attributes[i].Size     = handles_count * sizeof(HANDLE);
                ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
                for (j = 0; j < handles_count; j++)
                    ((HANDLE *)ret->Attributes[i].ValuePtr)[j] =
                        LongToHandle( ((LONG *)ULongToPtr(attr32->Attributes[i].Value))[j] );
            }
            break;
        case PS_ATTRIBUTE_CLIENT_ID:
            ret->Attributes[i].Size     = sizeof(CLIENT_ID);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        case PS_ATTRIBUTE_IMAGE_INFO:
            ret->Attributes[i].Size     = sizeof(SECTION_IMAGE_INFORMATION);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        case PS_ATTRIBUTE_TEB_ADDRESS:
            ret->Attributes[i].Size     = sizeof(TEB *);
            ret->Attributes[i].ValuePtr = Wow64AllocateTemp( ret->Attributes[i].Size );
            break;
        }
    }
    *attr = ret;
    return ret;
}


static void put_ps_attributes( PS_ATTRIBUTE_LIST32 *attr32, const PS_ATTRIBUTE_LIST *attr )
{
    ULONG i;

    if (!attr32) return;
    for (i = 0; i < (attr32->TotalLength - sizeof(attr32->TotalLength)) / sizeof(PS_ATTRIBUTE32); i++)
    {
        switch (attr->Attributes[i].Attribute)
        {
        case PS_ATTRIBUTE_CLIENT_ID:
        {
            CLIENT_ID32 id32;
            ULONG size = min( attr32->Attributes[i].Size, sizeof(id32) );
            put_client_id( &id32, attr->Attributes[i].ValuePtr );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &id32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        case PS_ATTRIBUTE_IMAGE_INFO:
        {
            SECTION_IMAGE_INFORMATION32 info32;
            ULONG size = min( attr32->Attributes[i].Size, sizeof(info32) );
            put_section_image_info( &info32, attr->Attributes[i].ValuePtr );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &info32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        case PS_ATTRIBUTE_TEB_ADDRESS:
        {
            TEB **teb = attr->Attributes[i].ValuePtr;
            ULONG teb32 = PtrToUlong( *teb ) + 0x2000;
            ULONG size = min( attr->Attributes[i].Size, sizeof(teb32) );
            memcpy( ULongToPtr( attr32->Attributes[i].Value ), &teb32, size );
            if (attr32->Attributes[i].ReturnLength)
                *(ULONG *)ULongToPtr(attr32->Attributes[i].ReturnLength) = size;
            break;
        }
        }
    }
}


/**********************************************************************
 *           wow64_NtAlertResumeThread
 */
NTSTATUS WINAPI wow64_NtAlertResumeThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtAlertResumeThread( handle, count );
}


/**********************************************************************
 *           wow64_NtAlertThread
 */
NTSTATUS WINAPI wow64_NtAlertThread( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtAlertThread( handle );
}


/**********************************************************************
 *           wow64_NtAssignProcessToJobObject
 */
NTSTATUS WINAPI wow64_NtAssignProcessToJobObject( UINT *args )
{
    HANDLE job = get_handle( &args );
    HANDLE process = get_handle( &args );

    return NtAssignProcessToJobObject( job, process );
}


/**********************************************************************
 *           wow64_NtCreateThread
 */
NTSTATUS WINAPI wow64_NtCreateThread( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    HANDLE process = get_handle( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );
    I386_CONTEXT *context = get_ptr( &args );
    void *initial_teb = get_ptr( &args );
    BOOLEAN suspended = get_ulong( &args );

    FIXME( "%p %x %p %p %p %p %p %u: stub\n", handle_ptr, access, attr32, process,
           id32, context, initial_teb, suspended );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtCreateThreadEx
 */
NTSTATUS WINAPI wow64_NtCreateThreadEx( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    HANDLE process = get_handle( &args );
    PRTL_THREAD_START_ROUTINE start = get_ptr( &args );
    void *param = get_ptr( &args );
    ULONG flags = get_ulong( &args );
    ULONG_PTR zero_bits = get_ulong( &args );
    SIZE_T stack_commit = get_ulong( &args );
    SIZE_T stack_reserve = get_ulong( &args );
    PS_ATTRIBUTE_LIST32 *attr_list32 = get_ptr( &args );

    struct object_attr64 attr;
    PS_ATTRIBUTE_LIST *attr_list;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    if (is_process_wow64( process ))
    {
        status = NtCreateThreadEx( &handle, access, objattr_32to64( &attr, attr32 ), process,
                                   start, param, flags, get_zero_bits( zero_bits ),
                                   stack_commit, stack_reserve,
                                   ps_attributes_32to64( &attr_list, attr_list32 ));
        put_ps_attributes( attr_list32, attr_list );
    }
    else status = STATUS_ACCESS_DENIED;

    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateUserProcess
 */
NTSTATUS WINAPI wow64_NtCreateUserProcess( UINT *args )
{
    ULONG *process_handle_ptr = get_ptr( &args );
    ULONG *thread_handle_ptr = get_ptr( &args );
    ACCESS_MASK process_access = get_ulong( &args );
    ACCESS_MASK thread_access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *process_attr32 = get_ptr( &args );
    OBJECT_ATTRIBUTES32 *thread_attr32 = get_ptr( &args );
    ULONG process_flags = get_ulong( &args );
    ULONG thread_flags = get_ulong( &args );
    RTL_USER_PROCESS_PARAMETERS32 *params32 = get_ptr( &args );
    PS_CREATE_INFO32 *info32 = get_ptr( &args );
    PS_ATTRIBUTE_LIST32 *attr32 = get_ptr( &args );

    struct object_attr64 process_attr, thread_attr;
    RTL_USER_PROCESS_PARAMETERS *params;
    PS_CREATE_INFO info;
    PS_ATTRIBUTE_LIST *attr;
    HANDLE process_handle = 0, thread_handle = 0;

    NTSTATUS status;

    *process_handle_ptr = *thread_handle_ptr = 0;
    status = NtCreateUserProcess( &process_handle, &thread_handle, process_access, thread_access,
                                  objattr_32to64( &process_attr, process_attr32 ),
                                  objattr_32to64( &thread_attr, thread_attr32 ),
                                  process_flags, thread_flags,
                                  process_params_32to64( &params, params32),
                                  &info, ps_attributes_32to64( &attr, attr32 ));
    put_handle( process_handle_ptr, process_handle );
    put_handle( thread_handle_ptr, thread_handle );
    put_ps_create_info( info32, &info );
    put_ps_attributes( attr32, attr );
    RtlDestroyProcessParameters( params );
    return status;
}


/**********************************************************************
 *           wow64_NtDebugActiveProcess
 */
NTSTATUS WINAPI wow64_NtDebugActiveProcess( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE debug = get_handle( &args );

    return NtDebugActiveProcess( process, debug );
}


/**********************************************************************
 *           wow64_NtFlushInstructionCache
 */
NTSTATUS WINAPI wow64_NtFlushInstructionCache( UINT *args )
{
    HANDLE process = get_handle( &args );
    const void *addr = get_ptr( &args );
    SIZE_T size = get_ulong( &args );

    return NtFlushInstructionCache( process, addr, size );
}


/**********************************************************************
 *           wow64_NtFlushProcessWriteBuffers
 */
NTSTATUS WINAPI wow64_NtFlushProcessWriteBuffers( UINT *args )
{
    NtFlushProcessWriteBuffers();
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           wow64_NtGetContextThread
 */
NTSTATUS WINAPI wow64_NtGetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return NtQueryInformationThread( handle, ThreadWow64Context, context,
                                     get_machine_context_size( current_machine ), NULL );
}


/**********************************************************************
 *           wow64_NtIsProcessInJob
 */
NTSTATUS WINAPI wow64_NtIsProcessInJob( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE job = get_handle( &args );

    return NtIsProcessInJob( process, job );
}


/**********************************************************************
 *           wow64_NtOpenProcess
 */
NTSTATUS WINAPI wow64_NtOpenProcess( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    CLIENT_ID id;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenProcess( &handle, access, objattr_32to64( &attr, attr32 ), client_id_32to64( &id, id32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenThread
 */
NTSTATUS WINAPI wow64_NtOpenThread( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    CLIENT_ID id;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenThread( &handle, access, objattr_32to64( &attr, attr32 ), client_id_32to64( &id, id32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtQueueApcThread
 */
NTSTATUS WINAPI wow64_NtQueueApcThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG func = get_ulong( &args );
    ULONG arg1 = get_ulong( &args );
    ULONG arg2 = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );

    return NtQueueApcThread( handle, apc_32to64( func ),
                             (ULONG_PTR)apc_param_32to64( func, arg1 ), arg2, arg3 );
}


/**********************************************************************
 *           wow64_NtRemoveProcessDebug
 */
NTSTATUS WINAPI wow64_NtRemoveProcessDebug( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE debug = get_handle( &args );

    return NtRemoveProcessDebug( process, debug );
}


/**********************************************************************
 *           wow64_NtResumeProcess
 */
NTSTATUS WINAPI wow64_NtResumeProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtResumeProcess( handle );
}


/**********************************************************************
 *           wow64_NtResumeThread
 */
NTSTATUS WINAPI wow64_NtResumeThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtResumeThread( handle, count );
}


/**********************************************************************
 *           wow64_NtSetContextThread
 */
NTSTATUS WINAPI wow64_NtSetContextThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    WOW64_CONTEXT *context = get_ptr( &args );

    return NtSetInformationThread( handle, ThreadWow64Context,
                                   context, get_machine_context_size( current_machine ));
}


/**********************************************************************
 *           wow64_NtSetThreadExecutionState
 */
NTSTATUS WINAPI wow64_NtSetThreadExecutionState( UINT *args )
{
    EXECUTION_STATE new_state = get_ulong( &args );
    EXECUTION_STATE *old_state = get_ptr( &args );

    return NtSetThreadExecutionState( new_state, old_state );
}


/**********************************************************************
 *           wow64_NtSuspendProcess
 */
NTSTATUS WINAPI wow64_NtSuspendProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtSuspendProcess( handle );
}


/**********************************************************************
 *           wow64_NtSuspendThread
 */
NTSTATUS WINAPI wow64_NtSuspendThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *count = get_ptr( &args );

    return NtSuspendThread( handle, count );
}


/**********************************************************************
 *           wow64_NtTerminateProcess
 */
NTSTATUS WINAPI wow64_NtTerminateProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG exit_code = get_ulong( &args );

    return NtTerminateProcess( handle, exit_code );
}


/**********************************************************************
 *           wow64_NtTerminateThread
 */
NTSTATUS WINAPI wow64_NtTerminateThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG exit_code = get_ulong( &args );

    return NtTerminateThread( handle, exit_code );
}
