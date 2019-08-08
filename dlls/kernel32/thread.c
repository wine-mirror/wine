/*
 * Win32 threads
 *
 * Copyright 1996 Alexandre Julliard
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
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/asm.h"
#include "wine/debug.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *           FreeLibraryAndExitThread (KERNEL32.@)
 */
void WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}


/***********************************************************************
 * Wow64SetThreadContext [KERNEL32.@]
 */
BOOL WINAPI Wow64SetThreadContext( HANDLE handle, const WOW64_CONTEXT *context)
{
#ifdef __i386__
    NTSTATUS status = NtSetContextThread( handle, (const CONTEXT *)context );
#elif defined(__x86_64__)
    NTSTATUS status = RtlWow64SetThreadContext( handle, context );
#else
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    FIXME("not implemented on this platform\n");
#endif
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 * Wow64GetThreadContext [KERNEL32.@]
 */
BOOL WINAPI Wow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context)
{
#ifdef __i386__
    NTSTATUS status = NtGetContextThread( handle, (CONTEXT *)context );
#elif defined(__x86_64__)
    NTSTATUS status = RtlWow64GetThreadContext( handle, context );
#else
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    FIXME("not implemented on this platform\n");
#endif
    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.@)
 */
DWORD_PTR WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask )
{
    NTSTATUS                    status;
    THREAD_BASIC_INFORMATION    tbi;

    status = NtQueryInformationThread( hThread, ThreadBasicInformation, 
                                       &tbi, sizeof(tbi), NULL );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    status = NtSetInformationThread( hThread, ThreadAffinityMask, 
                                     &dwThreadAffinityMask,
                                     sizeof(dwThreadAffinityMask));
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return 0;
    }
    return tbi.AffinityMask;
}


/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32.@)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE hthread, DWORD sel, LPLDT_ENTRY ldtent )
{
    THREAD_DESCRIPTOR_INFORMATION tdi;
    NTSTATUS status;

    tdi.Selector = sel;
    status = NtQueryInformationThread( hthread, ThreadDescriptorTableEntry, &tdi, sizeof(tdi), NULL);
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    *ldtent = tdi.Entry;
    return TRUE;
}


/***********************************************************************
 *              QueueUserWorkItem  (KERNEL32.@)
 */
BOOL WINAPI QueueUserWorkItem( LPTHREAD_START_ROUTINE Function, PVOID Context, ULONG Flags )
{
    NTSTATUS status;

    TRACE("(%p,%p,0x%08x)\n", Function, Context, Flags);

    status = RtlQueueWorkItem( Function, Context, Flags );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
}

/***********************************************************************
 * GetCurrentThread [KERNEL32.@]  Gets pseudohandle for current thread
 *
 * RETURNS
 *    Pseudohandle for the current thread
 */
HANDLE WINAPI KERNEL32_GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}

/**********************************************************************
 *		SetLastError (KERNEL32.@)
 *
 * Sets the last-error code.
 *
 * RETURNS
 * Nothing.
 */
void WINAPI KERNEL32_SetLastError( DWORD error ) /* [in] Per-thread error code */
{
    NtCurrentTeb()->LastErrorValue = error;
}

/**********************************************************************
 *              GetLastError (KERNEL32.@)
 *
 * Get the last-error code.
 *
 * RETURNS
 *  last-error code.
 */
DWORD WINAPI KERNEL32_GetLastError(void)
{
    return NtCurrentTeb()->LastErrorValue;
}

/***********************************************************************
 *		GetCurrentProcessId (KERNEL32.@)
 *
 * Get the current process identifier.
 *
 * RETURNS
 *  current process identifier
 */
DWORD WINAPI KERNEL32_GetCurrentProcessId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess);
}

/***********************************************************************
 *		GetCurrentThreadId (KERNEL32.@)
 *
 * Get the current thread identifier.
 *
 * RETURNS
 *  current thread identifier
 */
DWORD WINAPI KERNEL32_GetCurrentThreadId(void)
{
    return HandleToULong(NtCurrentTeb()->ClientId.UniqueThread);
}

/***********************************************************************
 *           GetProcessHeap    (KERNEL32.@)
 */
HANDLE WINAPI KERNEL32_GetProcessHeap(void)
{
    return NtCurrentTeb()->Peb->ProcessHeap;
}

/***********************************************************************
 *              CallbackMayRunLong (KERNEL32.@)
 */
BOOL WINAPI CallbackMayRunLong( TP_CALLBACK_INSTANCE *instance )
{
    NTSTATUS status;

    TRACE( "%p\n", instance );

    status = TpCallbackMayRunLong( instance );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }

    return TRUE;
}
