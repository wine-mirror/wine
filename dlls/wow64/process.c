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
#include "ddk/ntddk.h"
#include "wow64_private.h"
#include "wine/asm.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wow);


static BOOL is_process_wow64( HANDLE handle )
{
    ULONG_PTR info;

    if (handle == GetCurrentProcess()) return TRUE;
    if (NtQueryInformationProcess( handle, ProcessWow64Information, &info, sizeof(info), NULL ))
        return FALSE;
    return !!info;
}


static BOOL is_process_id_wow64( const CLIENT_ID *id )
{
    HANDLE handle;
    BOOL ret = FALSE;

    if (id->UniqueProcess == ULongToHandle(GetCurrentProcessId())) return TRUE;
    if (!NtOpenProcess( &handle, PROCESS_QUERY_LIMITED_INFORMATION, NULL, id ))
    {
        ret = is_process_wow64( handle );
        NtClose( handle );
    }
    return ret;
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
        case PS_ATTRIBUTE_PARENT_PROCESS:
        case PS_ATTRIBUTE_DEBUG_PORT:
        case PS_ATTRIBUTE_TOKEN:
            ret->Attributes[i].Size     = sizeof(HANDLE);
            ret->Attributes[i].ValuePtr = LongToHandle( attr32->Attributes[i].Value );
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

void put_vm_counters( VM_COUNTERS_EX32 *info32, const VM_COUNTERS_EX *info, ULONG size )
{
    info32->PeakVirtualSize            = info->PeakVirtualSize;
    info32->VirtualSize                = info->VirtualSize;
    info32->PageFaultCount             = info->PageFaultCount;
    info32->PeakWorkingSetSize         = info->PeakWorkingSetSize;
    info32->WorkingSetSize             = info->WorkingSetSize;
    info32->QuotaPeakPagedPoolUsage    = info->QuotaPeakPagedPoolUsage;
    info32->QuotaPagedPoolUsage        = info->QuotaPagedPoolUsage;
    info32->QuotaPeakNonPagedPoolUsage = info->QuotaPeakNonPagedPoolUsage;
    info32->QuotaNonPagedPoolUsage     = info->QuotaNonPagedPoolUsage;
    info32->PagefileUsage              = info->PagefileUsage;
    info32->PeakPagefileUsage          = info->PeakPagefileUsage;
    if (size == sizeof(VM_COUNTERS_EX32)) info32->PrivateUsage = info->PrivateUsage;
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
 *           wow64_NtAlertThreadByThreadId
 */
NTSTATUS WINAPI wow64_NtAlertThreadByThreadId( UINT *args )
{
    HANDLE tid = get_handle( &args );

    return NtAlertThreadByThreadId( tid );
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

    FIXME( "%p %lx %p %p %p %p %p %u: stub\n", handle_ptr, access, attr32, process,
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
 *           wow64_NtFlushProcessWriteBuffers
 */
NTSTATUS WINAPI wow64_NtFlushProcessWriteBuffers( UINT *args )
{
    return NtFlushProcessWriteBuffers();
}


/**********************************************************************
 *           wow64_NtGetNextProcess
 */
NTSTATUS WINAPI wow64_NtGetNextProcess( UINT *args )
{
    HANDLE process = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtGetNextProcess( process, access, attributes, flags, &handle );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtGetNextThread
 */
NTSTATUS WINAPI wow64_NtGetNextThread( UINT *args )
{
    HANDLE process = get_handle( &args );
    HANDLE thread = get_handle( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    ULONG *handle_ptr = get_ptr( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtGetNextThread( process, thread, access, attributes, flags, &handle );
    put_handle( handle_ptr, handle );
    return status;
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
 *           wow64_NtQueryInformationProcess
 */
NTSTATUS WINAPI wow64_NtQueryInformationProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    PROCESSINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case ProcessBasicInformation:  /* PROCESS_BASIC_INFORMATION */
        if (len == sizeof(PROCESS_BASIC_INFORMATION32))
        {
            PROCESS_BASIC_INFORMATION info;
            PROCESS_BASIC_INFORMATION32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                if (is_process_wow64( handle ))
                    info32->PebBaseAddress = PtrToUlong( info.PebBaseAddress ) + 0x1000;
                else
                    info32->PebBaseAddress = 0;
                info32->ExitStatus = info.ExitStatus;
                info32->AffinityMask = info.AffinityMask;
                info32->BasePriority = info.BasePriority;
                info32->UniqueProcessId = info.UniqueProcessId;
                info32->InheritedFromUniqueProcessId = info.InheritedFromUniqueProcessId;
                if (retlen) *retlen = sizeof(*info32);
            }
            return status;
        }
        if (retlen) *retlen = sizeof(PROCESS_BASIC_INFORMATION32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessIoCounters:  /* IO_COUNTERS */
    case ProcessTimes:  /* KERNEL_USER_TIMES */
    case ProcessDefaultHardErrorMode:  /* ULONG */
    case ProcessPriorityClass:  /* PROCESS_PRIORITY_CLASS */
    case ProcessHandleCount:  /* ULONG */
    case ProcessSessionInformation:  /* ULONG */
    case ProcessDebugFlags:  /* ULONG */
    case ProcessExecuteFlags:  /* ULONG */
    case ProcessCookie:  /* ULONG */
    case ProcessCycleTime:  /* PROCESS_CYCLE_TIME_INFORMATION */
        /* FIXME: check buffer alignment */
        return NtQueryInformationProcess( handle, class, ptr, len, retlen );

    case ProcessQuotaLimits:  /* QUOTA_LIMITS */
        if (len == sizeof(QUOTA_LIMITS32))
        {
            QUOTA_LIMITS info;
            QUOTA_LIMITS32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                info32->PagedPoolLimit        = info.PagedPoolLimit;
                info32->NonPagedPoolLimit     = info.NonPagedPoolLimit;
                info32->MinimumWorkingSetSize = info.MinimumWorkingSetSize;
                info32->MaximumWorkingSetSize = info.MaximumWorkingSetSize;
                info32->PagefileLimit         = info.PagefileLimit;
                info32->TimeLimit             = info.TimeLimit;
                if (retlen) *retlen = len;
            }
            return status;
        }
        if (retlen) *retlen = sizeof(QUOTA_LIMITS32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessVmCounters:  /* VM_COUNTERS_EX */
        if (len == sizeof(VM_COUNTERS32) || len == sizeof(VM_COUNTERS_EX32))
        {
            VM_COUNTERS_EX info;
            VM_COUNTERS_EX32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                put_vm_counters( info32, &info, len );
                if (retlen) *retlen = len;
            }
            return status;
        }
        if (retlen) *retlen = sizeof(VM_COUNTERS_EX32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessDebugPort:  /* ULONG_PTR */
    case ProcessAffinityMask:  /* ULONG_PTR */
    case ProcessWow64Information:  /* ULONG_PTR */
    case ProcessDebugObjectHandle:  /* HANDLE */
        if (retlen) *(volatile ULONG *)retlen |= 0;
        if (len == sizeof(ULONG))
        {
            ULONG_PTR data;

            if (!(status = NtQueryInformationProcess( handle, class, &data, sizeof(data), NULL )))
            {
                *(ULONG *)ptr = data;
                if (retlen) *retlen = sizeof(ULONG);
            }
            else if (status == STATUS_PORT_NOT_SET)
            {
                if (!ptr) return STATUS_ACCESS_VIOLATION;
                if (retlen) *retlen = sizeof(ULONG);
            }
            return status;
        }
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessImageFileName:
    case ProcessImageFileNameWin32:  /* UNICODE_STRING + string */
        {
            ULONG retsize, size = len + sizeof(UNICODE_STRING) - sizeof(UNICODE_STRING32);
            UNICODE_STRING *str = Wow64AllocateTemp( size );
            UNICODE_STRING32 *str32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, str, size, &retsize )))
            {
                str32->Length = str->Length;
                str32->MaximumLength = str->MaximumLength;
                str32->Buffer = PtrToUlong( str32 + 1 );
                memcpy( str32 + 1, str->Buffer, str->MaximumLength );
            }
            if (retlen) *retlen = retsize + sizeof(UNICODE_STRING32) - sizeof(UNICODE_STRING);
            return status;
        }

    case ProcessImageInformation:  /* SECTION_IMAGE_INFORMATION */
        if (len == sizeof(SECTION_IMAGE_INFORMATION32))
        {
            SECTION_IMAGE_INFORMATION info;
            SECTION_IMAGE_INFORMATION32 *info32 = ptr;

            if (!(status = NtQueryInformationProcess( handle, class, &info, sizeof(info), NULL )))
            {
                put_section_image_info( info32, &info );
                if (retlen) *retlen = sizeof(*info32);
            }
            return status;
        }
        if (retlen) *retlen = sizeof(SECTION_IMAGE_INFORMATION32);
        return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessWineLdtCopy:
        return STATUS_NOT_IMPLEMENTED;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtQueryInformationThread
 */
NTSTATUS WINAPI wow64_NtQueryInformationThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    THREADINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case ThreadBasicInformation:  /* THREAD_BASIC_INFORMATION */
    {
        THREAD_BASIC_INFORMATION32 info32;
        THREAD_BASIC_INFORMATION info;

        status = NtQueryInformationThread( handle, class, &info, sizeof(info), NULL );
        if (!status)
        {
            info32.ExitStatus = info.ExitStatus;
            info32.TebBaseAddress = is_process_id_wow64( &info.ClientId ) && info.TebBaseAddress ?
                                    PtrToUlong(info.TebBaseAddress) + 0x2000 : 0;
            info32.ClientId.UniqueProcess = HandleToULong( info.ClientId.UniqueProcess );
            info32.ClientId.UniqueThread = HandleToULong( info.ClientId.UniqueThread );
            info32.AffinityMask = info.AffinityMask;
            info32.Priority = info.Priority;
            info32.BasePriority = info.BasePriority;
            memcpy( ptr, &info32, min( len, sizeof(info32) ));
            if (retlen) *retlen = min( len, sizeof(info32) );
        }
        return status;
    }

    case ThreadTimes:  /* KERNEL_USER_TIMES */
    case ThreadEnableAlignmentFaultFixup:  /* set only */
    case ThreadAmILastThread:  /* ULONG */
    case ThreadIsIoPending:  /* ULONG */
    case ThreadIsTerminated: /* ULONG */
    case ThreadHideFromDebugger:  /* BOOLEAN */
    case ThreadSuspendCount:  /* ULONG */
    case ThreadPriorityBoost:   /* ULONG */
    case ThreadIdealProcessorEx: /* PROCESSOR_NUMBER */
        /* FIXME: check buffer alignment */
        return NtQueryInformationThread( handle, class, ptr, len, retlen );

    case ThreadAffinityMask:  /* ULONG_PTR */
    case ThreadQuerySetWin32StartAddress:  /* PRTL_THREAD_START_ROUTINE */
    {
        ULONG_PTR data;

        status = NtQueryInformationThread( handle, class, &data, sizeof(data), NULL );
        if (!status)
        {
            memcpy( ptr, &data, min( len, sizeof(ULONG) ));
            if (retlen) *retlen = min( len, sizeof(ULONG) );
        }
        return status;
    }

    case ThreadDescriptorTableEntry:  /* THREAD_DESCRIPTOR_INFORMATION */
        return RtlWow64GetThreadSelectorEntry( handle, ptr, len, retlen );

    case ThreadWow64Context:  /* WOW64_CONTEXT* */
        return STATUS_INVALID_INFO_CLASS;

    case ThreadGroupInformation:  /* GROUP_AFFINITY */
    {
        GROUP_AFFINITY info;

        status = NtQueryInformationThread( handle, class, &info, sizeof(info), NULL );
        if (!status)
        {
            GROUP_AFFINITY32 info32 = { info.Mask, info.Group };
            memcpy( ptr, &info32, min( len, sizeof(info32) ));
            if (retlen) *retlen = min( len, sizeof(info32) );
        }
        return status;
    }

    case ThreadNameInformation:  /* THREAD_NAME_INFORMATION */
    {
        THREAD_NAME_INFORMATION *info;
        THREAD_NAME_INFORMATION32 *info32 = ptr;
        ULONG size, ret_size;

        if (len >= sizeof(*info32))
        {
            size = sizeof(*info) + len - sizeof(*info32);
            info = Wow64AllocateTemp( size );
            status = NtQueryInformationThread( handle, class, info, size, &ret_size );
            if (!status)
            {
                info32->ThreadName.Length = info->ThreadName.Length;
                info32->ThreadName.MaximumLength = info->ThreadName.MaximumLength;
                info32->ThreadName.Buffer = PtrToUlong( info32 + 1 );
                memcpy( info32 + 1, info + 1, min( len, info->ThreadName.MaximumLength ));
            }
        }
        else status = NtQueryInformationThread( handle, class, NULL, 0, &ret_size );

        if (retlen && (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL))
            *retlen = sizeof(*info32) + ret_size - sizeof(*info);
        return status;
    }

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
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
 *           wow64_NtQueueApcThreadEx
 */
NTSTATUS WINAPI wow64_NtQueueApcThreadEx( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE reserve_handle = get_handle( &args );
    ULONG func = get_ulong( &args );
    ULONG arg1 = get_ulong( &args );
    ULONG arg2 = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );

    return NtQueueApcThreadEx2( handle, reserve_handle, 0, apc_32to64( func ),
                                (ULONG_PTR)apc_param_32to64( func, arg1 ), arg2, arg3 );
}


/**********************************************************************
 *           wow64_NtQueueApcThreadEx
 */
NTSTATUS WINAPI wow64_NtQueueApcThreadEx2( UINT *args )
{
    HANDLE handle = get_handle( &args );
    HANDLE reserve_handle = get_handle( &args );
    ULONG flags = get_ulong( &args );
    ULONG func = get_ulong( &args );
    ULONG arg1 = get_ulong( &args );
    ULONG arg2 = get_ulong( &args );
    ULONG arg3 = get_ulong( &args );

    return NtQueueApcThreadEx2( handle, reserve_handle, flags, apc_32to64( func ),
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
 *           wow64_NtSetInformationProcess
 */
NTSTATUS WINAPI wow64_NtSetInformationProcess( UINT *args )
{
    HANDLE handle = get_handle( &args );
    PROCESSINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    NTSTATUS status;

    switch (class)
    {
    case ProcessDefaultHardErrorMode:   /* ULONG */
    case ProcessPriorityClass:   /* PROCESS_PRIORITY_CLASS */
    case ProcessBasePriority:   /* ULONG */
    case ProcessExecuteFlags:   /* ULONG */
    case ProcessPagePriority:   /* MEMORY_PRIORITY_INFORMATION */
    case ProcessPowerThrottlingState:   /* PROCESS_POWER_THROTTLING_STATE */
    case ProcessLeapSecondInformation:   /* PROCESS_LEAP_SECOND_INFO */
    case ProcessWineGrantAdminToken:   /* NULL */
        return NtSetInformationProcess( handle, class, ptr, len );

    case ProcessAccessToken: /* PROCESS_ACCESS_TOKEN */
        if (len == sizeof(PROCESS_ACCESS_TOKEN32))
        {
            PROCESS_ACCESS_TOKEN32 *stack = ptr;
            PROCESS_ACCESS_TOKEN info;

            info.Thread = ULongToHandle( stack->Thread );
            info.Token = ULongToHandle( stack->Token );
            return NtSetInformationProcess( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessAffinityMask:   /* ULONG_PTR */
        if (len == sizeof(ULONG))
        {
            ULONG_PTR mask = *(ULONG *)ptr;
            return NtSetInformationProcess( handle, class, &mask, sizeof(mask) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ProcessInstrumentationCallback:   /* PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION */
        if (len >= sizeof(ULONG))
        {
            FIXME( "ProcessInstrumentationCallback stub\n" );
            return STATUS_SUCCESS;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessThreadStackAllocation:   /* PROCESS_STACK_ALLOCATION_INFORMATION(_EX) */
        if (len == sizeof(PROCESS_STACK_ALLOCATION_INFORMATION_EX32))
        {
            PROCESS_STACK_ALLOCATION_INFORMATION_EX32 *stack = ptr;
            PROCESS_STACK_ALLOCATION_INFORMATION_EX info;

            info.PreferredNode = stack->PreferredNode;
            info.Reserved0 = stack->Reserved0;
            info.Reserved1 = stack->Reserved1;
            info.Reserved2 = stack->Reserved2;
            info.AllocInfo.ReserveSize = stack->AllocInfo.ReserveSize;
            info.AllocInfo.ZeroBits = get_zero_bits( stack->AllocInfo.ZeroBits );
            if (!(status = NtSetInformationProcess( handle, class, &info, sizeof(info) )))
                stack->AllocInfo.StackBase = PtrToUlong( info.AllocInfo.StackBase );
            return status;
        }
        else if (len == sizeof(PROCESS_STACK_ALLOCATION_INFORMATION32))
        {
            PROCESS_STACK_ALLOCATION_INFORMATION32 *stack = ptr;
            PROCESS_STACK_ALLOCATION_INFORMATION info;

            info.ReserveSize = stack->ReserveSize;
            info.ZeroBits = get_zero_bits( stack->ZeroBits );
            if (!(status = NtSetInformationProcess( handle, class, &info, sizeof(info) )))
                stack->StackBase = PtrToUlong( info.StackBase );
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case ProcessManageWritesToExecutableMemory: /* MANAGE_WRITES_TO_EXECUTABLE_MEMORY */
        return STATUS_INVALID_INFO_CLASS;

    case ProcessWineMakeProcessSystem:   /* HANDLE* */
        if (len == sizeof(ULONG))
        {
            HANDLE event = 0;
            status = NtSetInformationProcess( handle, class, &event, sizeof(HANDLE *) );
            put_handle( ptr, event );
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
}


/**********************************************************************
 *           wow64_NtSetInformationThread
 */
NTSTATUS WINAPI wow64_NtSetInformationThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    THREADINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    switch (class)
    {
    case ThreadZeroTlsCell:   /* ULONG */
    case ThreadPriority:   /* ULONG */
    case ThreadBasePriority:   /* ULONG */
    case ThreadHideFromDebugger:   /* void */
    case ThreadEnableAlignmentFaultFixup:   /* BOOLEAN */
    case ThreadPowerThrottlingState:  /* THREAD_POWER_THROTTLING_STATE */
    case ThreadIdealProcessor:   /* ULONG */
    case ThreadPriorityBoost:   /* ULONG */
        return NtSetInformationThread( handle, class, ptr, len );

    case ThreadImpersonationToken:   /* HANDLE */
        if (len == sizeof(ULONG))
        {
            HANDLE token = LongToHandle( *(ULONG *)ptr );
            return NtSetInformationThread( handle, class, &token, sizeof(token) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadAffinityMask:  /* ULONG_PTR */
    case ThreadQuerySetWin32StartAddress:   /* PRTL_THREAD_START_ROUTINE */
        if (len == sizeof(ULONG))
        {
            ULONG_PTR mask = *(ULONG *)ptr;
            return NtSetInformationThread( handle, class, &mask, sizeof(mask) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadWow64Context:  /* WOW64_CONTEXT* */
    case ThreadManageWritesToExecutableMemory: /* MANAGE_WRITES_TO_EXECUTABLE_MEMORY */
        return STATUS_INVALID_INFO_CLASS;

    case ThreadGroupInformation:   /* GROUP_AFFINITY */
        if (len == sizeof(GROUP_AFFINITY32))
        {
            GROUP_AFFINITY32 *info32 = ptr;
            GROUP_AFFINITY info = { info32->Mask, info32->Group };

            return NtSetInformationThread( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INVALID_PARAMETER;

    case ThreadNameInformation:   /* THREAD_NAME_INFORMATION */
    case ThreadWineNativeThreadName:
        if (len == sizeof(THREAD_NAME_INFORMATION32))
        {
            THREAD_NAME_INFORMATION32 *info32 = ptr;
            THREAD_NAME_INFORMATION info;

            if (!unicode_str_32to64( &info.ThreadName, &info32->ThreadName ))
                return STATUS_ACCESS_VIOLATION;
            return NtSetInformationThread( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_INVALID_INFO_CLASS;
    }
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

    NTSTATUS status;

    if (!handle && pBTCpuProcessTerm) pBTCpuProcessTerm( handle, FALSE, 0 );
    status = NtTerminateProcess( handle, exit_code );
    if (!handle && pBTCpuProcessTerm) pBTCpuProcessTerm( handle, TRUE, status );
    return status;
}


/**********************************************************************
 *           wow64_NtTerminateThread
 */
NTSTATUS WINAPI wow64_NtTerminateThread( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG exit_code = get_ulong( &args );

    if (pBTCpuThreadTerm) pBTCpuThreadTerm( handle, exit_code );

    return NtTerminateThread( handle, exit_code );
}


/**********************************************************************
 *           wow64_NtWow64QueryInformationProcess64
 */
NTSTATUS WINAPI wow64_NtWow64QueryInformationProcess64( UINT *args )
{
    HANDLE handle = get_handle( &args );
    PROCESSINFOCLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG size = get_ulong( &args );
    ULONG *ret_len = get_ptr( &args );

    switch (class)
    {
    case ProcessBasicInformation:
        return NtQueryInformationProcess( handle, class, info, size, ret_len );
    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}
