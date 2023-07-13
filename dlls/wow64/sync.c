/*
 * WoW64 synchronization objects and functions
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


static void put_object_type_info( OBJECT_TYPE_INFORMATION32 *info32, const OBJECT_TYPE_INFORMATION *info )
{
    if (info->TypeName.Length)
    {
        memcpy( info32 + 1, info->TypeName.Buffer, info->TypeName.Length + sizeof(WCHAR) );
        info32->TypeName.Length        = info->TypeName.Length;
        info32->TypeName.MaximumLength = info->TypeName.Length + sizeof(WCHAR);
        info32->TypeName.Buffer        = PtrToUlong( info32 + 1 );
    }
    else memset( &info32->TypeName, 0, sizeof(info32->TypeName) );
    info32->TotalNumberOfObjects       = info->TotalNumberOfObjects;
    info32->TotalNumberOfHandles       = info->TotalNumberOfHandles;
    info32->TotalPagedPoolUsage        = info->TotalPagedPoolUsage;
    info32->TotalNonPagedPoolUsage     = info->TotalNonPagedPoolUsage;
    info32->TotalNamePoolUsage         = info->TotalNamePoolUsage;
    info32->TotalHandleTableUsage      = info->TotalHandleTableUsage;
    info32->HighWaterNumberOfObjects   = info->HighWaterNumberOfObjects;
    info32->HighWaterNumberOfHandles   = info->HighWaterNumberOfHandles;
    info32->HighWaterPagedPoolUsage    = info->HighWaterPagedPoolUsage;
    info32->HighWaterNonPagedPoolUsage = info->HighWaterNonPagedPoolUsage;
    info32->HighWaterNamePoolUsage     = info->HighWaterNamePoolUsage;
    info32->HighWaterHandleTableUsage  = info->HighWaterHandleTableUsage;
    info32->InvalidAttributes          = info->InvalidAttributes;
    info32->GenericMapping             = info->GenericMapping;
    info32->ValidAccessMask            = info->ValidAccessMask;
    info32->SecurityRequired           = info->SecurityRequired;
    info32->MaintainHandleCount        = info->MaintainHandleCount;
    info32->TypeIndex                  = info->TypeIndex;
    info32->ReservedByte               = info->ReservedByte;
    info32->PoolType                   = info->PoolType;
    info32->DefaultPagedPoolCharge     = info->DefaultPagedPoolCharge;
    info32->DefaultNonPagedPoolCharge  = info->DefaultNonPagedPoolCharge;
}


static JOBOBJECT_BASIC_LIMIT_INFORMATION *job_basic_limit_info_32to64( JOBOBJECT_BASIC_LIMIT_INFORMATION *out,
                                                                       const JOBOBJECT_BASIC_LIMIT_INFORMATION32 *in )
{
    out->PerProcessUserTimeLimit = in->PerProcessUserTimeLimit;
    out->PerJobUserTimeLimit     = in->PerJobUserTimeLimit;
    out->LimitFlags              = in->LimitFlags;
    out->MinimumWorkingSetSize   = in->MinimumWorkingSetSize;
    out->MaximumWorkingSetSize   = in->MaximumWorkingSetSize;
    out->ActiveProcessLimit      = in->ActiveProcessLimit;
    out->Affinity                = in->Affinity;
    out->PriorityClass           = in->PriorityClass;
    out->SchedulingClass         = in->SchedulingClass;
    return out;
}


static void put_job_basic_limit_info( JOBOBJECT_BASIC_LIMIT_INFORMATION32 *info32,
                                      const JOBOBJECT_BASIC_LIMIT_INFORMATION *info )
{
    info32->PerProcessUserTimeLimit = info->PerProcessUserTimeLimit;
    info32->PerJobUserTimeLimit     = info->PerJobUserTimeLimit;
    info32->LimitFlags              = info->LimitFlags;
    info32->MinimumWorkingSetSize   = info->MinimumWorkingSetSize;
    info32->MaximumWorkingSetSize   = info->MaximumWorkingSetSize;
    info32->ActiveProcessLimit      = info->ActiveProcessLimit;
    info32->Affinity                = info->Affinity;
    info32->PriorityClass           = info->PriorityClass;
    info32->SchedulingClass         = info->SchedulingClass;
}


void put_section_image_info( SECTION_IMAGE_INFORMATION32 *info32, const SECTION_IMAGE_INFORMATION *info )
{
    if (info->Machine == IMAGE_FILE_MACHINE_AMD64 || info->Machine == IMAGE_FILE_MACHINE_ARM64)
    {
        info32->TransferAddress    = 0x81231234;  /* sic */
        info32->MaximumStackSize   = 0x100000;
        info32->CommittedStackSize = 0x10000;
    }
    else
    {
        info32->TransferAddress    = PtrToUlong( info->TransferAddress );
        info32->MaximumStackSize   = info->MaximumStackSize;
        info32->CommittedStackSize = info->CommittedStackSize;
    }
    info32->ZeroBits                    = info->ZeroBits;
    info32->SubSystemType               = info->SubSystemType;
    info32->MinorSubsystemVersion       = info->MinorSubsystemVersion;
    info32->MajorSubsystemVersion       = info->MajorSubsystemVersion;
    info32->MajorOperatingSystemVersion = info->MajorOperatingSystemVersion;
    info32->MinorOperatingSystemVersion = info->MinorOperatingSystemVersion;
    info32->ImageCharacteristics        = info->ImageCharacteristics;
    info32->DllCharacteristics          = info->DllCharacteristics;
    info32->Machine                     = info->Machine;
    info32->ImageContainsCode           = info->ImageContainsCode;
    info32->ImageFlags                  = info->ImageFlags;
    info32->LoaderFlags                 = info->LoaderFlags;
    info32->ImageFileSize               = info->ImageFileSize;
    info32->CheckSum                    = info->CheckSum;
}


/**********************************************************************
 *           wow64_NtAcceptConnectPort
 */
NTSTATUS WINAPI wow64_NtAcceptConnectPort( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ULONG id = get_ulong( &args );
    LPC_MESSAGE *msg = get_ptr( &args );
    BOOLEAN accept = get_ulong( &args );
    LPC_SECTION_WRITE *write = get_ptr( &args );
    LPC_SECTION_READ *read = get_ptr( &args );

    FIXME( "%p %lu %p %u %p %p: stub\n", handle_ptr, id, msg, accept, write, read );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtCancelTimer
 */
NTSTATUS WINAPI wow64_NtCancelTimer( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN *state = get_ptr( &args );

    return NtCancelTimer( handle, state );
}


/**********************************************************************
 *           wow64_NtClearEvent
 */
NTSTATUS WINAPI wow64_NtClearEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtClearEvent( handle );
}


/**********************************************************************
 *           wow64_NtCompareObjects
 */
NTSTATUS WINAPI wow64_NtCompareObjects( UINT *args )
{
    HANDLE first = get_handle( &args );
    HANDLE second = get_handle( &args );

    return NtCompareObjects( first, second );
}


/**********************************************************************
 *           wow64_NtCompleteConnectPort
 */
NTSTATUS WINAPI wow64_NtCompleteConnectPort( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtCompleteConnectPort( handle );
}


/**********************************************************************
 *           wow64_NtConnectPort
 */
NTSTATUS WINAPI wow64_NtConnectPort( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    SECURITY_QUALITY_OF_SERVICE *qos = get_ptr( &args );
    LPC_SECTION_WRITE *write = get_ptr( &args );
    LPC_SECTION_READ *read = get_ptr( &args );
    ULONG *max_len = get_ptr( &args );
    void *info = get_ptr( &args );
    ULONG *info_len = get_ptr( &args );

    FIXME( "%p %p %p %p %p %p %p %p: stub\n",
           handle_ptr, name32, qos, write, read, max_len, info, info_len );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtCreateDebugObject
 */
NTSTATUS WINAPI wow64_NtCreateDebugObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ULONG flags = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateDebugObject( &handle, access, objattr_32to64( &attr, attr32 ), flags );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateDirectoryObject
 */
NTSTATUS WINAPI wow64_NtCreateDirectoryObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateDirectoryObject( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateEvent
 */
NTSTATUS WINAPI wow64_NtCreateEvent( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    EVENT_TYPE type = get_ulong( &args );
    BOOLEAN state = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateEvent( &handle, access, objattr_32to64( &attr, attr32 ), type, state );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateIoCompletion
 */
NTSTATUS WINAPI wow64_NtCreateIoCompletion( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ULONG threads = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateIoCompletion( &handle, access, objattr_32to64( &attr, attr32 ), threads );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateJobObject
 */
NTSTATUS WINAPI wow64_NtCreateJobObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateJobObject( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateKeyedEvent
 */
NTSTATUS WINAPI wow64_NtCreateKeyedEvent( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ULONG flags = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateKeyedEvent( &handle, access, objattr_32to64( &attr, attr32 ), flags );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateMutant
 */
NTSTATUS WINAPI wow64_NtCreateMutant( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    BOOLEAN owned = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateMutant( &handle, access, objattr_32to64( &attr, attr32 ), owned );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreatePort
 */
NTSTATUS WINAPI wow64_NtCreatePort( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    ULONG info_len = get_ulong( &args );
    ULONG data_len = get_ulong( &args );
    ULONG *reserved = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreatePort( &handle, objattr_32to64( &attr, attr32 ), info_len, data_len, reserved );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateSection
 */
NTSTATUS WINAPI wow64_NtCreateSection( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    const LARGE_INTEGER *size = get_ptr( &args );
    ULONG protect = get_ulong( &args );
    ULONG flags = get_ulong( &args );
    HANDLE file = get_handle( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateSection( &handle, access, objattr_32to64( &attr, attr32 ), size, protect, flags, file );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateSemaphore
 */
NTSTATUS WINAPI wow64_NtCreateSemaphore( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    LONG initial = get_ulong( &args );
    LONG max = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateSemaphore( &handle, access, objattr_32to64( &attr, attr32 ), initial, max );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateSymbolicLinkObject
 */
NTSTATUS WINAPI wow64_NtCreateSymbolicLinkObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    UNICODE_STRING32 *target32 = get_ptr( &args );

    struct object_attr64 attr;
    UNICODE_STRING target;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateSymbolicLinkObject( &handle, access, objattr_32to64( &attr, attr32 ),
                                         unicode_str_32to64( &target, target32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtCreateTimer
 */
NTSTATUS WINAPI wow64_NtCreateTimer( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    TIMER_TYPE type = get_ulong( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateTimer( &handle, access, objattr_32to64( &attr, attr32 ), type );
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtDebugContinue
 */
NTSTATUS WINAPI wow64_NtDebugContinue( UINT *args )
{
    HANDLE handle = get_handle( &args );
    CLIENT_ID32 *id32 = get_ptr( &args );
    NTSTATUS status = get_ulong( &args );

    CLIENT_ID id;

    return NtDebugContinue( handle, client_id_32to64( &id, id32 ), status );
}


/**********************************************************************
 *           wow64_NtDelayExecution
 */
NTSTATUS WINAPI wow64_NtDelayExecution( UINT *args )
{
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtDelayExecution( alertable, timeout );
}


/**********************************************************************
 *           wow64_NtDuplicateObject
 */
NTSTATUS WINAPI wow64_NtDuplicateObject( UINT *args )
{
    HANDLE source_process = get_handle( &args );
    HANDLE source_handle = get_handle( &args );
    HANDLE dest_process = get_handle( &args );
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    ULONG attributes = get_ulong( &args );
    ULONG options = get_ulong( &args );

    HANDLE handle = 0;
    NTSTATUS status;

    if (handle_ptr) *handle_ptr = 0;
    status = NtDuplicateObject( source_process, source_handle, dest_process, &handle,
                                access, attributes, options );
    if (handle_ptr) put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtListenPort
 */
NTSTATUS WINAPI wow64_NtListenPort( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LPC_MESSAGE *msg = get_ptr( &args );

    FIXME( "%p %p: stub\n", handle, msg );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtMakeTemporaryObject
 */
NTSTATUS WINAPI wow64_NtMakeTemporaryObject( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtMakeTemporaryObject( handle );
}


/**********************************************************************
 *           wow64_NtOpenDirectoryObject
 */
NTSTATUS WINAPI wow64_NtOpenDirectoryObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenDirectoryObject( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenEvent
 */
NTSTATUS WINAPI wow64_NtOpenEvent( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenEvent( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenIoCompletion
 */
NTSTATUS WINAPI wow64_NtOpenIoCompletion( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenIoCompletion( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenJobObject
 */
NTSTATUS WINAPI wow64_NtOpenJobObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenJobObject( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenKeyedEvent
 */
NTSTATUS WINAPI wow64_NtOpenKeyedEvent( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenKeyedEvent( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenMutant
 */
NTSTATUS WINAPI wow64_NtOpenMutant( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenMutant( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenSection
 */
NTSTATUS WINAPI wow64_NtOpenSection( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenSection( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenSemaphore
 */
NTSTATUS WINAPI wow64_NtOpenSemaphore( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenSemaphore( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenSymbolicLinkObject
 */
NTSTATUS WINAPI wow64_NtOpenSymbolicLinkObject( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenSymbolicLinkObject( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtOpenTimer
 */
NTSTATUS WINAPI wow64_NtOpenTimer( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );

    struct object_attr64 attr;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtOpenTimer( &handle, access, objattr_32to64( &attr, attr32 ));
    put_handle( handle_ptr, handle );
    return status;
}


/**********************************************************************
 *           wow64_NtPulseEvent
 */
NTSTATUS WINAPI wow64_NtPulseEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG *prev_state = get_ptr( &args );

    return NtPulseEvent( handle, prev_state );
}


/**********************************************************************
 *           wow64_NtQueryDirectoryObject
 */
NTSTATUS WINAPI wow64_NtQueryDirectoryObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    DIRECTORY_BASIC_INFORMATION32 *info32 = get_ptr( &args );
    ULONG size32 = get_ulong( &args );
    BOOLEAN single_entry = get_ulong( &args );
    BOOLEAN restart = get_ulong( &args );
    ULONG *context = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );
    ULONG retsize;

    NTSTATUS status;
    DIRECTORY_BASIC_INFORMATION *info;
    ULONG size = size32 + 2 * sizeof(*info) - 2 * sizeof(*info32);

    if (!single_entry) FIXME( "not implemented\n" );
    info = Wow64AllocateTemp( size );
    status = NtQueryDirectoryObject( handle, info, size, single_entry, restart, context, &retsize );
    if (!status)
    {
        info32->ObjectName.Buffer            = PtrToUlong( info32 + 2 );
        info32->ObjectName.Length            = info->ObjectName.Length;
        info32->ObjectName.MaximumLength     = info->ObjectName.MaximumLength;
        info32->ObjectTypeName.Buffer        = info32->ObjectName.Buffer + info->ObjectName.MaximumLength;
        info32->ObjectTypeName.Length        = info->ObjectTypeName.Length;
        info32->ObjectTypeName.MaximumLength = info->ObjectTypeName.MaximumLength;
        memset( info32 + 1, 0, sizeof(*info32) );
        size = info->ObjectName.MaximumLength + info->ObjectTypeName.MaximumLength;
        memcpy( info32 + 2, info + 2, size );
        if (retlen) *retlen = 2 * sizeof(*info32) + size;
    }
    else if (retlen && status == STATUS_BUFFER_TOO_SMALL)
        *retlen = retsize - 2 * sizeof(*info) + 2 * sizeof(*info32);
    else if (retlen && status == STATUS_NO_MORE_ENTRIES)
        *retlen = 0;
    return status;
}


/**********************************************************************
 *           wow64_NtQueryEvent
 */
NTSTATUS WINAPI wow64_NtQueryEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    EVENT_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtQueryEvent( handle, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQueryInformationJobObject
 */
NTSTATUS WINAPI wow64_NtQueryInformationJobObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    JOBOBJECTINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;

    switch (class)
    {
    case JobObjectBasicAccountingInformation:   /* JOBOBJECT_BASIC_ACCOUNTING_INFORMATION */
        return NtQueryInformationJobObject( handle, class, ptr, len, retlen );

    case JobObjectBasicLimitInformation:   /* JOBOBJECT_BASIC_LIMIT_INFORMATION */
        if (len >= sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION32))
        {
            JOBOBJECT_BASIC_LIMIT_INFORMATION32 *info32 = ptr;
            JOBOBJECT_BASIC_LIMIT_INFORMATION info;

            status = NtQueryInformationJobObject( handle, class, &info, sizeof(info), NULL );
            if (!status) put_job_basic_limit_info( info32, &info );
            if (retlen) *retlen = sizeof(*info32);
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case JobObjectBasicProcessIdList:   /* JOBOBJECT_BASIC_PROCESS_ID_LIST */
        if (len >= sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST32))
        {
            JOBOBJECT_BASIC_PROCESS_ID_LIST32 *info32 = ptr;
            JOBOBJECT_BASIC_PROCESS_ID_LIST *info;
            ULONG i, count, size;

            count = (len - offsetof( JOBOBJECT_BASIC_PROCESS_ID_LIST32, ProcessIdList )) / sizeof(info32->ProcessIdList[0]);
            size = offsetof( JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList[count] );
            info = Wow64AllocateTemp( size );
            status = NtQueryInformationJobObject( handle, class, info, size, NULL );
            if (!status)
            {
                info32->NumberOfAssignedProcesses = info->NumberOfAssignedProcesses;
                info32->NumberOfProcessIdsInList  = info->NumberOfProcessIdsInList;
                for (i = 0; i < info->NumberOfProcessIdsInList; i++)
                    info32->ProcessIdList[i] = info->ProcessIdList[i];
                if (retlen) *retlen = offsetof( JOBOBJECT_BASIC_PROCESS_ID_LIST32, ProcessIdList[i] );
            }
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    case JobObjectExtendedLimitInformation:   /* JOBOBJECT_EXTENDED_LIMIT_INFORMATION */
        if (len >= sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION32))
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION32 *info32 = ptr;
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;

            status = NtQueryInformationJobObject( handle, class, &info, sizeof(info), NULL );
            if (!status)
            {
                put_job_basic_limit_info( &info32->BasicLimitInformation, &info.BasicLimitInformation );
                info32->IoInfo                = info.IoInfo;
                info32->ProcessMemoryLimit    = info.ProcessMemoryLimit;
                info32->JobMemoryLimit        = info.JobMemoryLimit;
                info32->PeakProcessMemoryUsed = info.PeakProcessMemoryUsed;
                info32->PeakJobMemoryUsed     = info.PeakJobMemoryUsed;
            }
            if (retlen) *retlen = sizeof(*info32);
            return status;
        }
        else return STATUS_INFO_LENGTH_MISMATCH;

    default:
        if (class >= MaxJobObjectInfoClass) return STATUS_INVALID_PARAMETER;
        FIXME( "unsupported class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtQueryIoCompletion
 */
NTSTATUS WINAPI wow64_NtQueryIoCompletion( UINT *args )
{
    HANDLE handle = get_handle( &args );
    IO_COMPLETION_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtQueryIoCompletion( handle, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQueryMutant
 */
NTSTATUS WINAPI wow64_NtQueryMutant( UINT *args )
{
    HANDLE handle = get_handle( &args );
    MUTANT_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtQueryMutant( handle, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQueryObject
 */
NTSTATUS WINAPI wow64_NtQueryObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    OBJECT_INFORMATION_CLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    NTSTATUS status;
    ULONG ret_size;

    switch (class)
    {
    case ObjectBasicInformation:   /* OBJECT_BASIC_INFORMATION */
    case ObjectHandleFlagInformation:   /* OBJECT_HANDLE_FLAG_INFORMATION */
        return NtQueryObject( handle, class, ptr, len, retlen );

    case ObjectNameInformation:   /* OBJECT_NAME_INFORMATION */
    {
        ULONG size = len + sizeof(OBJECT_NAME_INFORMATION) - sizeof(OBJECT_NAME_INFORMATION32);
        OBJECT_NAME_INFORMATION32 *info32 = ptr;
        OBJECT_NAME_INFORMATION *info = Wow64AllocateTemp( size );

        if (!(status = NtQueryObject( handle, class, info, size, &ret_size )))
        {
            if (len >= sizeof(*info32) + info->Name.MaximumLength)
            {
                if (info->Name.Length)
                {
                    memcpy( info32 + 1, info->Name.Buffer, info->Name.Length + sizeof(WCHAR) );
                    info32->Name.Length = info->Name.Length;
                    info32->Name.MaximumLength = info->Name.Length + sizeof(WCHAR);
                    info32->Name.Buffer = PtrToUlong( info32 + 1 );
                }
                else memset( &info32->Name, 0, sizeof(info32->Name) );
            }
            else status = STATUS_INFO_LENGTH_MISMATCH;
            if (retlen) *retlen = sizeof(*info32) + info->Name.MaximumLength;
        }
        else if (status == STATUS_INFO_LENGTH_MISMATCH || status == STATUS_BUFFER_OVERFLOW)
        {
            if (retlen) *retlen = ret_size - sizeof(*info) + sizeof(*info32);
        }
        return status;
    }

    case ObjectTypeInformation:   /* OBJECT_TYPE_INFORMATION */
    {
        ULONG_PTR buffer[(sizeof(OBJECT_TYPE_INFORMATION) + 64) / sizeof(ULONG_PTR)];
        OBJECT_TYPE_INFORMATION *info = (OBJECT_TYPE_INFORMATION *)buffer;
        OBJECT_TYPE_INFORMATION32 *info32 = ptr;

        if (!(status = NtQueryObject( handle, class, info, sizeof(buffer), NULL )))
        {
            if (len >= sizeof(*info32) + info->TypeName.MaximumLength)
                put_object_type_info( info32, info );
            else
                status = STATUS_INFO_LENGTH_MISMATCH;
            if (retlen) *retlen = sizeof(*info32) + info->TypeName.Length + sizeof(WCHAR);
        }
        return status;
    }

    case ObjectTypesInformation:   /* OBJECT_TYPES_INFORMATION */
    {
        OBJECT_TYPES_INFORMATION *info, *info32 = ptr;
        /* assume at most 32 types, with an average 16-char name */
        ULONG ret_size, size = 32 * (sizeof(OBJECT_TYPE_INFORMATION) + 16 * sizeof(WCHAR));

        info = Wow64AllocateTemp( size );
        if (!(status = NtQueryObject( handle, class, info, size, &ret_size )))
        {
            OBJECT_TYPE_INFORMATION *type;
            OBJECT_TYPE_INFORMATION32 *type32;
            ULONG align = TYPE_ALIGNMENT( OBJECT_TYPE_INFORMATION ) - 1;
            ULONG align32 = TYPE_ALIGNMENT( OBJECT_TYPE_INFORMATION32 ) - 1;
            ULONG i, pos = (sizeof(*info) + align) & ~align, pos32 = (sizeof(*info32) + align32) & ~align32;

            if (pos32 <= len) info32->NumberOfTypes = info->NumberOfTypes;
            for (i = 0; i < info->NumberOfTypes; i++)
            {
                type = (OBJECT_TYPE_INFORMATION *)((char *)info + pos);
                type32 = (OBJECT_TYPE_INFORMATION32 *)((char *)ptr + pos32);
                pos += sizeof(*type) + ((type->TypeName.MaximumLength + align) & ~align);
                pos32 += sizeof(*type32) + ((type->TypeName.MaximumLength + align32) & ~align32);
                if (pos32 <= len) put_object_type_info( type32, type );
            }
            if (pos32 > len) status = STATUS_INFO_LENGTH_MISMATCH;
            if (retlen) *retlen = pos32;
        }
        return status;
    }

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtQueryPerformanceCounter
 */
NTSTATUS WINAPI wow64_NtQueryPerformanceCounter( UINT *args )
{
    LARGE_INTEGER *counter = get_ptr( &args );
    LARGE_INTEGER *frequency = get_ptr( &args );

    return NtQueryPerformanceCounter( counter, frequency );
}


/**********************************************************************
 *           wow64_NtQuerySection
 */
NTSTATUS WINAPI wow64_NtQuerySection( UINT *args )
{
    HANDLE handle = get_handle( &args );
    SECTION_INFORMATION_CLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    SIZE_T size = get_ulong( &args );
    ULONG *ret_ptr = get_ptr( &args );

    NTSTATUS status;
    SIZE_T ret_size = 0;

    switch (class)
    {
    case SectionBasicInformation:
    {
        SECTION_BASIC_INFORMATION info;
        SECTION_BASIC_INFORMATION32 *info32 = ptr;

        if (size < sizeof(*info32)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!(status = NtQuerySection( handle, class, &info, sizeof(info), &ret_size )))
        {
            info32->BaseAddress = PtrToUlong( info.BaseAddress );
            info32->Attributes  = info.Attributes;
            info32->Size        = info.Size;
            ret_size = sizeof(*info32);
        }
        break;
    }
    case SectionImageInformation:
    {
        SECTION_IMAGE_INFORMATION info;
        SECTION_IMAGE_INFORMATION32 *info32 = ptr;

        if (size < sizeof(*info32)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!(status = NtQuerySection( handle, class, &info, sizeof(info), &ret_size )))
        {
            put_section_image_info( info32, &info );
            ret_size = sizeof(*info32);
        }
        break;
    }
    default:
	FIXME( "class %u not implemented\n", class );
	return STATUS_NOT_IMPLEMENTED;
    }
    put_size( ret_ptr, ret_size );
    return status;
}


/**********************************************************************
 *           wow64_NtQuerySemaphore
 */
NTSTATUS WINAPI wow64_NtQuerySemaphore( UINT *args )
{
    HANDLE handle = get_handle( &args );
    SEMAPHORE_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtQuerySemaphore( handle, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQuerySymbolicLinkObject
 */
NTSTATUS WINAPI wow64_NtQuerySymbolicLinkObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    UNICODE_STRING32 *target32 = get_ptr( &args );
    ULONG *retlen = get_ptr( &args );

    UNICODE_STRING target;
    NTSTATUS status;

    status = NtQuerySymbolicLinkObject( handle, unicode_str_32to64( &target, target32 ), retlen );
    if (!status) target32->Length = target.Length;
    return status;
}


/**********************************************************************
 *           wow64_NtQueryTimer
 */
NTSTATUS WINAPI wow64_NtQueryTimer( UINT *args )
{
    HANDLE handle = get_handle( &args );
    TIMER_INFORMATION_CLASS class = get_ulong( &args );
    void *info = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtQueryTimer( handle, class, info, len, retlen );
}


/**********************************************************************
 *           wow64_NtQueryTimerResolution
 */
NTSTATUS WINAPI wow64_NtQueryTimerResolution( UINT *args )
{
    ULONG *min_res = get_ptr( &args );
    ULONG *max_res = get_ptr( &args );
    ULONG *current_res = get_ptr( &args );

    return NtQueryTimerResolution( min_res, max_res, current_res );
}


/**********************************************************************
 *           wow64_NtRegisterThreadTerminatePort
 */
NTSTATUS WINAPI wow64_NtRegisterThreadTerminatePort( UINT *args )
{
    HANDLE handle = get_handle( &args );

    return NtRegisterThreadTerminatePort( handle );
}


/**********************************************************************
 *           wow64_NtReleaseKeyedEvent
 */
NTSTATUS WINAPI wow64_NtReleaseKeyedEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    void *key = get_ptr( &args );
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtReleaseKeyedEvent( handle, key, alertable, timeout );
}


/**********************************************************************
 *           wow64_NtReleaseMutant
 */
NTSTATUS WINAPI wow64_NtReleaseMutant( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG *prev_count = get_ptr( &args );

    return NtReleaseMutant( handle, prev_count );
}


/**********************************************************************
 *           wow64_NtReleaseSemaphore
 */
NTSTATUS WINAPI wow64_NtReleaseSemaphore( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG count = get_ulong( &args );
    ULONG *previous = get_ptr( &args );

    return NtReleaseSemaphore( handle, count, previous );
}


/**********************************************************************
 *           wow64_NtReplyWaitReceivePort
 */
NTSTATUS WINAPI wow64_NtReplyWaitReceivePort( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG *id = get_ptr( &args );
    LPC_MESSAGE *reply = get_ptr( &args );
    LPC_MESSAGE *msg = get_ptr( &args );

    FIXME( "%p %p %p %p: stub\n", handle, id, reply, msg );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtRequestWaitReplyPort
 */
NTSTATUS WINAPI wow64_NtRequestWaitReplyPort( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LPC_MESSAGE *msg_in = get_ptr( &args );
    LPC_MESSAGE *msg_out = get_ptr( &args );

    FIXME( "%p %p %p: stub\n", handle, msg_in, msg_out );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtResetEvent
 */
NTSTATUS WINAPI wow64_NtResetEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG *prev_state = get_ptr( &args );

    return NtResetEvent( handle, prev_state );
}


/**********************************************************************
 *           wow64_NtSecureConnectPort
 */
NTSTATUS WINAPI wow64_NtSecureConnectPort( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    UNICODE_STRING32 *name32 = get_ptr( &args );
    SECURITY_QUALITY_OF_SERVICE *qos = get_ptr( &args );
    LPC_SECTION_WRITE *write = get_ptr( &args );
    SID *sid = get_ptr( &args );
    LPC_SECTION_READ *read = get_ptr( &args );
    ULONG *max_len = get_ptr( &args );
    void *info = get_ptr( &args );
    ULONG *info_len = get_ptr( &args );

    FIXME( "%p %p %p %p %p %p %p %p %p: stub\n",
           handle_ptr, name32, qos, write, sid, read, max_len, info, info_len );
    return STATUS_NOT_IMPLEMENTED;
}


/**********************************************************************
 *           wow64_NtSetEvent
 */
NTSTATUS WINAPI wow64_NtSetEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG *prev_state = get_ptr( &args );

    return NtSetEvent( handle, prev_state );
}


/**********************************************************************
 *           wow64_NtSetInformationDebugObject
 */
NTSTATUS WINAPI wow64_NtSetInformationDebugObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    DEBUGOBJECTINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );
    ULONG *retlen = get_ptr( &args );

    return NtSetInformationDebugObject( handle, class, ptr, len, retlen );
}


/**********************************************************************
 *           wow64_NtSetInformationJobObject
 */
NTSTATUS WINAPI wow64_NtSetInformationJobObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    JOBOBJECTINFOCLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    switch (class)
    {
    case JobObjectBasicLimitInformation:   /* JOBOBJECT_BASIC_LIMIT_INFORMATION */
        if (len == sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION32))
        {
            JOBOBJECT_BASIC_LIMIT_INFORMATION info;

            return NtSetInformationJobObject( handle, class, job_basic_limit_info_32to64( &info, ptr ),
                                              sizeof(info) );
        }
        else return STATUS_INVALID_PARAMETER;

    case JobObjectBasicUIRestrictions:
        FIXME( "unsupported class JobObjectBasicUIRestrictions\n" );
        return STATUS_SUCCESS;

    case JobObjectAssociateCompletionPortInformation:   /* JOBOBJECT_ASSOCIATE_COMPLETION_PORT */
        if (len == sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT32))
        {
            JOBOBJECT_ASSOCIATE_COMPLETION_PORT32 *info32 = ptr;
            JOBOBJECT_ASSOCIATE_COMPLETION_PORT info;

            info.CompletionKey  = ULongToPtr( info32->CompletionKey );
            info.CompletionPort = LongToHandle( info32->CompletionPort );
            return NtSetInformationJobObject( handle, class, &info, sizeof(info) );
        }
        else return STATUS_INVALID_PARAMETER;

    case JobObjectExtendedLimitInformation:   /* JOBOBJECT_EXTENDED_LIMIT_INFORMATION */
        if (len == sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION32))
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION32 *info32 = ptr;
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;

            info.IoInfo                = info32->IoInfo;
            info.ProcessMemoryLimit    = info32->ProcessMemoryLimit;
            info.JobMemoryLimit        = info32->JobMemoryLimit;
            info.PeakProcessMemoryUsed = info32->PeakProcessMemoryUsed;
            info.PeakJobMemoryUsed     = info32->PeakJobMemoryUsed;
            return NtSetInformationJobObject( handle, class,
                                              job_basic_limit_info_32to64( &info.BasicLimitInformation,
                                                                           &info32->BasicLimitInformation ),
                                              sizeof(info) );
        }
        else return STATUS_INVALID_PARAMETER;

    default:
        if (class >= MaxJobObjectInfoClass) return STATUS_INVALID_PARAMETER;
        FIXME( "unsupported class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtSetInformationObject
 */
NTSTATUS WINAPI wow64_NtSetInformationObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    OBJECT_INFORMATION_CLASS class = get_ulong( &args );
    void *ptr = get_ptr( &args );
    ULONG len = get_ulong( &args );

    switch (class)
    {
    case ObjectHandleFlagInformation:   /* OBJECT_HANDLE_FLAG_INFORMATION */
        return NtSetInformationObject( handle, class, ptr, len );

    default:
        FIXME( "unsupported class %u\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/**********************************************************************
 *           wow64_NtSetIoCompletion
 */
NTSTATUS WINAPI wow64_NtSetIoCompletion( UINT *args )
{
    HANDLE handle = get_handle( &args );
    ULONG_PTR key = get_ulong( &args );
    ULONG_PTR value = get_ulong( &args );
    NTSTATUS status = get_ulong( &args );
    SIZE_T count = get_ulong( &args );

    return NtSetIoCompletion( handle, key, value, status, count );
}


/**********************************************************************
 *           wow64_NtSetTimer
 */
NTSTATUS WINAPI wow64_NtSetTimer( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LARGE_INTEGER *when = get_ptr( &args );
    ULONG apc = get_ulong( &args );
    ULONG apc_param = get_ulong( &args );
    BOOLEAN resume = get_ulong( &args );
    ULONG period = get_ulong( &args );
    BOOLEAN *state = get_ptr( &args );

    return NtSetTimer( handle, when, apc_32to64( apc ), apc_param_32to64( apc, apc_param ),
                       resume, period, state );
}


/**********************************************************************
 *           wow64_NtSetTimerResolution
 */
NTSTATUS WINAPI wow64_NtSetTimerResolution( UINT *args )
{
    ULONG res = get_ulong( &args );
    BOOLEAN set = get_ulong( &args );
    ULONG *current_res = get_ptr( &args );

    return NtSetTimerResolution( res, set, current_res );
}


/**********************************************************************
 *           wow64_NtSignalAndWaitForSingleObject
 */
NTSTATUS WINAPI wow64_NtSignalAndWaitForSingleObject( UINT *args )
{
    HANDLE signal = get_handle( &args );
    HANDLE wait = get_handle( &args );
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtSignalAndWaitForSingleObject( signal, wait, alertable, timeout );
}


/**********************************************************************
 *           wow64_NtTerminateJobObject
 */
NTSTATUS WINAPI wow64_NtTerminateJobObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    NTSTATUS status = get_ulong( &args );

    return NtTerminateJobObject( handle, status );
}


/**********************************************************************
 *           wow64_NtTestAlert
 */
NTSTATUS WINAPI wow64_NtTestAlert( UINT *args )
{
    return NtTestAlert();
}


/**********************************************************************
 *           wow64_NtTraceControl
 */
NTSTATUS WINAPI wow64_NtTraceControl( UINT *args )
{
    ULONG code = get_ulong( &args );
    void *inbuf = get_ptr( &args );
    ULONG inbuf_len = get_ulong( &args );
    void *outbuf = get_ptr( &args );
    ULONG outbuf_len = get_ulong( &args );
    ULONG *size = get_ptr( &args );

    return NtTraceControl( code, inbuf, inbuf_len, outbuf, outbuf_len, size );
}


/**********************************************************************
 *           wow64_NtWaitForAlertByThreadId
 */
NTSTATUS WINAPI wow64_NtWaitForAlertByThreadId( UINT *args )
{
    const void *address = get_ptr( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtWaitForAlertByThreadId( address, timeout );
}


/* helper to wow64_NtWaitForDebugEvent; retrieve machine from PE image */
static NTSTATUS get_image_machine( HANDLE handle, USHORT *machine )
{
    IMAGE_DOS_HEADER dos_hdr;
    IMAGE_NT_HEADERS nt_hdr;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER offset;
    FILE_POSITION_INFORMATION pos_info;
    NTSTATUS status;

    offset.QuadPart = 0;
    status = NtReadFile( handle, NULL, NULL, NULL,
                         &iosb, &dos_hdr, sizeof(dos_hdr), &offset, NULL );
    if (!status)
    {
        offset.QuadPart = dos_hdr.e_lfanew;
        status = NtReadFile( handle, NULL, NULL, NULL, &iosb,
                             &nt_hdr, FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader), &offset, NULL );
        if (!status)
            *machine = nt_hdr.FileHeader.Machine;
        /* Reset file pos at beginning of file */
        pos_info.CurrentByteOffset.QuadPart = 0;
        NtSetInformationFile( handle, &iosb, &pos_info, sizeof(pos_info), FilePositionInformation );
    }
    return status;
}

/* helper to wow64_NtWaitForDebugEvent; only pass debug events for current machine */
static BOOL filter_out_state_change( HANDLE handle, DBGUI_WAIT_STATE_CHANGE *state )
{
    BOOL filter_out;

    switch (state->NewState)
    {
    case DbgLoadDllStateChange:
        filter_out = ((ULONG64)state->StateInfo.LoadDll.BaseOfDll >> 32) != 0;
        if (!filter_out)
        {
            USHORT machine;
            filter_out = !get_image_machine( state->StateInfo.LoadDll.FileHandle, &machine) && machine != current_machine;
        }
        break;
    case DbgUnloadDllStateChange:
        filter_out = ((ULONG_PTR)state->StateInfo.UnloadDll.BaseAddress >> 32) != 0;
        break;
    default:
        filter_out = FALSE;
        break;
    }
    if (filter_out)
    {
        if (state->NewState == DbgLoadDllStateChange)
            NtClose( state->StateInfo.LoadDll.FileHandle );
        NtDebugContinue( handle, &state->AppClientId, DBG_CONTINUE );
    }
    return filter_out;
}


/**********************************************************************
 *           wow64_NtWaitForDebugEvent
 */
NTSTATUS WINAPI wow64_NtWaitForDebugEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN alertable = get_ulong( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );
    DBGUI_WAIT_STATE_CHANGE32 *state32 = get_ptr( &args );

    ULONG i;
    DBGUI_WAIT_STATE_CHANGE state;
    NTSTATUS status;

    do
    {
        status = NtWaitForDebugEvent( handle, alertable, timeout, &state );
    } while (!status && filter_out_state_change( handle, &state ));

    if (!status)
    {
        state32->NewState = state.NewState;
        state32->AppClientId.UniqueProcess = HandleToULong( state.AppClientId.UniqueProcess );
        state32->AppClientId.UniqueThread = HandleToULong( state.AppClientId.UniqueThread );
        switch (state.NewState)
        {
#define COPY_ULONG(field) state32->StateInfo.field = state.StateInfo.field
#define COPY_PTR(field)   state32->StateInfo.field = PtrToUlong( state.StateInfo.field )
        case DbgCreateThreadStateChange:
            COPY_PTR( CreateThread.HandleToThread );
            COPY_PTR( CreateThread.NewThread.StartAddress );
            COPY_ULONG( CreateThread.NewThread.SubSystemKey );
            break;
        case DbgCreateProcessStateChange:
            COPY_PTR( CreateProcessInfo.HandleToProcess );
            COPY_PTR( CreateProcessInfo.HandleToThread );
            COPY_PTR( CreateProcessInfo.NewProcess.FileHandle );
            COPY_PTR( CreateProcessInfo.NewProcess.BaseOfImage );
            COPY_PTR( CreateProcessInfo.NewProcess.InitialThread.StartAddress );
            COPY_ULONG( CreateProcessInfo.NewProcess.InitialThread.SubSystemKey );
            COPY_ULONG( CreateProcessInfo.NewProcess.DebugInfoFileOffset );
            COPY_ULONG( CreateProcessInfo.NewProcess.DebugInfoSize );
            break;
        case DbgExitThreadStateChange:
        case DbgExitProcessStateChange:
            COPY_ULONG( ExitThread.ExitStatus );
            break;
        case DbgExceptionStateChange:
        case DbgBreakpointStateChange:
        case DbgSingleStepStateChange:
            COPY_ULONG( Exception.FirstChance );
            COPY_ULONG( Exception.ExceptionRecord.ExceptionCode );
            COPY_ULONG( Exception.ExceptionRecord.ExceptionFlags );
            COPY_ULONG( Exception.ExceptionRecord.NumberParameters );
            COPY_PTR( Exception.ExceptionRecord.ExceptionRecord );
            COPY_PTR( Exception.ExceptionRecord.ExceptionAddress );
            for (i = 0; i < state.StateInfo.Exception.ExceptionRecord.NumberParameters; i++)
                COPY_ULONG( Exception.ExceptionRecord.ExceptionInformation[i] );
            break;
        case DbgLoadDllStateChange:
            COPY_PTR( LoadDll.FileHandle );
            COPY_PTR( LoadDll.BaseOfDll );
            COPY_ULONG( LoadDll.DebugInfoFileOffset );
            COPY_ULONG( LoadDll.DebugInfoSize );
            COPY_PTR( LoadDll.NamePointer );
            break;
        case DbgUnloadDllStateChange:
            COPY_PTR( UnloadDll.BaseAddress );
            break;
        default:
            break;
        }
#undef COPY_ULONG
#undef COPY_PTR
    }
    return status;
}


/**********************************************************************
 *           wow64_NtWaitForKeyedEvent
 */
NTSTATUS WINAPI wow64_NtWaitForKeyedEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    const void *key = get_ptr( &args );
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtWaitForKeyedEvent( handle, key, alertable, timeout );
}


/**********************************************************************
 *           wow64_NtWaitForMultipleObjects
 */
NTSTATUS WINAPI wow64_NtWaitForMultipleObjects( UINT *args )
{
    DWORD count = get_ulong( &args );
    LONG *handles_ptr = get_ptr( &args );
    BOOLEAN wait_any = get_ulong( &args );
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    DWORD i;

    for (i = 0; i < count && i < MAXIMUM_WAIT_OBJECTS; i++) handles[i] = LongToHandle( handles_ptr[i] );
    return NtWaitForMultipleObjects( count, handles, wait_any, alertable, timeout );
}


/**********************************************************************
 *           wow64_NtWaitForSingleObject
 */
NTSTATUS WINAPI wow64_NtWaitForSingleObject( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN alertable = get_ulong( &args );
    const LARGE_INTEGER *timeout = get_ptr( &args );

    return NtWaitForSingleObject( handle, alertable, timeout );
}


/**********************************************************************
 *           wow64_NtYieldExecution
 */
NTSTATUS WINAPI wow64_NtYieldExecution( UINT *args )
{
    return NtYieldExecution();
}


/**********************************************************************
 *           wow64_NtCreateTransaction
 */
NTSTATUS WINAPI wow64_NtCreateTransaction( UINT *args )
{
    ULONG *handle_ptr = get_ptr( &args );
    ACCESS_MASK access = get_ulong( &args );
    OBJECT_ATTRIBUTES32 *attr32 = get_ptr( &args );
    GUID *guid = get_ptr( &args );
    HANDLE tm = get_handle( &args );
    ULONG options = get_ulong( &args );
    ULONG isol_level = get_ulong( &args );
    ULONG isol_flags = get_ulong( &args );
    LARGE_INTEGER *timeout = get_ptr( &args );
    UNICODE_STRING32 *desc32 = get_ptr( &args );

    struct object_attr64 attr;
    UNICODE_STRING desc;
    HANDLE handle = 0;
    NTSTATUS status;

    *handle_ptr = 0;
    status = NtCreateTransaction( &handle, access, objattr_32to64( &attr, attr32 ), guid, tm, options,
            isol_level, isol_flags, timeout, unicode_str_32to64( &desc, desc32 ));
    put_handle( handle_ptr, handle );

    return status;
}


/**********************************************************************
 *           wow64_NtCommitTransaction
 */
NTSTATUS WINAPI wow64_NtCommitTransaction( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN wait = get_ulong( &args );

    return NtCommitTransaction( handle, wait );
}


/**********************************************************************
 *           wow64_NtRollbackTransaction
 */
NTSTATUS WINAPI wow64_NtRollbackTransaction( UINT *args )
{
    HANDLE handle = get_handle( &args );
    BOOLEAN wait = get_ulong( &args );

    return NtRollbackTransaction( handle, wait );
}
