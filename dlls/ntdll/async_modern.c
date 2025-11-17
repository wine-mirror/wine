/*
 * NTDLL Modern Async and Threading Extensions
 * Enhanced support for async I/O and modern threading patterns
 * Required by modern applications with heavy async workloads
 *
 * Copyright 2025 Wine Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

/***********************************************************************
 *          NtCreateWaitCompletionPacket  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateWaitCompletionPacket( HANDLE *handle, ACCESS_MASK access,
                                                OBJECT_ATTRIBUTES *attr )
{
    FIXME( "handle %p, access %#lx, attr %p: stub\n", handle, access, attr );

    if (!handle)
        return STATUS_INVALID_PARAMETER;

    /* Return a fake handle for now - proper implementation would create kernel object */
    *handle = (HANDLE)0xdeadbeef;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtAssociateWaitCompletionPacket  (NTDLL.@)
 */
NTSTATUS WINAPI NtAssociateWaitCompletionPacket( HANDLE packet, HANDLE io_completion,
                                                  HANDLE handle, void *key,
                                                  void *context, NTSTATUS status,
                                                  ULONG_PTR info, BOOLEAN *already_signaled )
{
    FIXME( "packet %p, io_completion %p, handle %p: stub\n", packet, io_completion, handle );

    if (already_signaled)
        *already_signaled = FALSE;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtCancelWaitCompletionPacket  (NTDLL.@)
 */
NTSTATUS WINAPI NtCancelWaitCompletionPacket( HANDLE packet, BOOLEAN remove_signaled )
{
    FIXME( "packet %p, remove_signaled %d: stub\n", packet, remove_signaled );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtSetInformationWorkerFactory  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationWorkerFactory( HANDLE factory, DWORD class, void *info, ULONG len )
{
    FIXME( "factory %p, class %lu, info %p, len %lu: stub\n", factory, class, info, len );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtQueryInformationWorkerFactory  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationWorkerFactory( HANDLE factory, DWORD class, void *info,
                                                   ULONG len, ULONG *ret_len )
{
    FIXME( "factory %p, class %lu, info %p, len %lu: stub\n", factory, class, info, len );

    if (ret_len)
        *ret_len = 0;

    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *          NtCreateWorkerFactory  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateWorkerFactory( HANDLE *handle, ACCESS_MASK access,
                                        OBJECT_ATTRIBUTES *attr, HANDLE completion,
                                        HANDLE process, void *start_routine,
                                        void *start_parameter, ULONG max_threads,
                                        SIZE_T stack_reserve, SIZE_T stack_commit )
{
    FIXME( "handle %p, completion %p, max_threads %lu: stub\n", handle, completion, max_threads );

    if (!handle)
        return STATUS_INVALID_PARAMETER;

    /* Return fake handle - proper implementation would create worker factory */
    *handle = (HANDLE)0xcafebabe;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtReleaseWorkerFactoryWorker  (NTDLL.@)
 */
NTSTATUS WINAPI NtReleaseWorkerFactoryWorker( HANDLE factory )
{
    FIXME( "factory %p: stub\n", factory );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtShutdownWorkerFactory  (NTDLL.@)
 */
NTSTATUS WINAPI NtShutdownWorkerFactory( HANDLE factory, LONG *pending_worker_count )
{
    FIXME( "factory %p, pending %p: stub\n", factory, pending_worker_count );

    if (pending_worker_count)
        *pending_worker_count = 0;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtWorkerFactoryWorkerReady  (NTDLL.@)
 */
NTSTATUS WINAPI NtWorkerFactoryWorkerReady( HANDLE factory )
{
    TRACE( "factory %p\n", factory );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtAlertThreadByThreadId  (NTDLL.@)
 */
NTSTATUS WINAPI NtAlertThreadByThreadId( HANDLE tid )
{
    FIXME( "tid %p: stub\n", tid );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtWaitForAlertByThreadId  (NTDLL.@)
 */
NTSTATUS WINAPI NtWaitForAlertByThreadId( void *address, LARGE_INTEGER *timeout )
{
    FIXME( "address %p, timeout %p: stub\n", address, timeout );

    /* Simulate immediate wake - proper implementation would wait */
    return STATUS_ALERTED;
}

/***********************************************************************
 *          NtCreateIRTimer  (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateIRTimer( HANDLE *handle, ACCESS_MASK access )
{
    FIXME( "handle %p, access %#lx: stub\n", handle, access );

    if (!handle)
        return STATUS_INVALID_PARAMETER;

    *handle = (HANDLE)0xfeedf00d;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *          NtSetIRTimer  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetIRTimer( HANDLE handle, LARGE_INTEGER *due_time )
{
    FIXME( "handle %p, due_time %p: stub\n", handle, due_time );
    return STATUS_SUCCESS;
}
