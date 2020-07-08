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

#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"

#include "kernel_private.h"


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
    return set_ntstatus( NtSetContextThread( handle, (const CONTEXT *)context ));
#elif defined(__x86_64__)
    return set_ntstatus( RtlWow64SetThreadContext( handle, context ));
#else
    return set_ntstatus( STATUS_NOT_IMPLEMENTED );
#endif
}

/***********************************************************************
 * Wow64GetThreadContext [KERNEL32.@]
 */
BOOL WINAPI Wow64GetThreadContext( HANDLE handle, WOW64_CONTEXT *context)
{
#ifdef __i386__
    return set_ntstatus( NtGetContextThread( handle, (CONTEXT *)context ));
#elif defined(__x86_64__)
    return set_ntstatus( RtlWow64GetThreadContext( handle, context ));
#else
    return set_ntstatus( STATUS_NOT_IMPLEMENTED );
#endif
}


/**********************************************************************
 *           SetThreadAffinityMask   (KERNEL32.@)
 */
DWORD_PTR WINAPI SetThreadAffinityMask( HANDLE thread, DWORD_PTR mask )
{
    THREAD_BASIC_INFORMATION tbi;

    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadBasicInformation, &tbi, sizeof(tbi), NULL )))
        return 0;
    if (!set_ntstatus( NtSetInformationThread( thread, ThreadAffinityMask, &mask, sizeof(mask))))
        return 0;
    return tbi.AffinityMask;
}


/***********************************************************************
 *           GetThreadSelectorEntry   (KERNEL32.@)
 */
BOOL WINAPI GetThreadSelectorEntry( HANDLE thread, DWORD sel, LDT_ENTRY *ldtent )
{
    THREAD_DESCRIPTOR_INFORMATION tdi;

    tdi.Selector = sel;
    if (!set_ntstatus( NtQueryInformationThread( thread, ThreadDescriptorTableEntry,
                                                 &tdi, sizeof(tdi), NULL )))
        return FALSE;
    *ldtent = tdi.Entry;
    return TRUE;
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
