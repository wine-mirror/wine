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

    NTSTATUS status;
    DIRECTORY_BASIC_INFORMATION *info;
    ULONG size = size32 + sizeof(*info) - sizeof(*info32);

    if (!single_entry) FIXME( "not implemented\n" );
    info = RtlAllocateHeap( GetProcessHeap(), 0, size );
    status = NtQueryDirectoryObject( handle, info, size, single_entry, restart, context, NULL );
    if (!status)
    {
        info32->ObjectName.Buffer            = PtrToUlong( info32 + 1 );
        info32->ObjectName.Length            = info->ObjectName.Length;
        info32->ObjectName.MaximumLength     = info->ObjectName.MaximumLength;
        info32->ObjectTypeName.Buffer        = info32->ObjectName.Buffer + info->ObjectName.MaximumLength;
        info32->ObjectTypeName.Length        = info->ObjectTypeName.Length;
        info32->ObjectTypeName.MaximumLength = info->ObjectTypeName.MaximumLength;
        size = info->ObjectName.MaximumLength + info->ObjectTypeName.MaximumLength;
        memcpy( info32 + 1, info + 1, size );
        if (retlen) *retlen = sizeof(*info32) + size;
    }
    RtlFreeHeap( GetProcessHeap(), 0, info );
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
 *           wow64_NtQueryPerformanceCounter
 */
NTSTATUS WINAPI wow64_NtQueryPerformanceCounter( UINT *args )
{
    LARGE_INTEGER *counter = get_ptr( &args );
    LARGE_INTEGER *frequency = get_ptr( &args );

    return NtQueryPerformanceCounter( counter, frequency );
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
 *           wow64_NtResetEvent
 */
NTSTATUS WINAPI wow64_NtResetEvent( UINT *args )
{
    HANDLE handle = get_handle( &args );
    LONG *prev_state = get_ptr( &args );

    return NtResetEvent( handle, prev_state );
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
    NTSTATUS status = NtWaitForDebugEvent( handle, alertable, timeout, &state );

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
