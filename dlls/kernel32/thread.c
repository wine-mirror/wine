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

#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winternl.h"

#include "wine/asm.h"
#include "wine/debug.h"
#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);

/***********************************************************************
 *           BaseThreadInitThunk (KERNEL32.@)
 */
#ifdef __i386__
__ASM_FASTCALL_FUNC( BaseThreadInitThunk, 12,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl %ebx\n\t"
                    __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                    "movl 8(%ebp),%ebx\n\t"
                    /* deliberately mis-align the stack by 8, Doom 3 needs this */
                    "pushl 4(%ebp)\n\t"  /* Driller expects readable address at this offset */
                    "pushl 4(%ebp)\n\t"
                    "pushl %ebx\n\t"
                    "call *%edx\n\t"
                    "movl %eax,(%esp)\n\t"
                    "call " __ASM_STDCALL( "RtlExitUserThread", 4 ))
#else
void __fastcall BaseThreadInitThunk( DWORD unknown, LPTHREAD_START_ROUTINE entry, void *arg )
{
    RtlExitUserThread( entry( arg ) );
}
#endif

/***********************************************************************
 *           FreeLibraryAndExitThread (KERNEL32.@)
 */
void WINAPI FreeLibraryAndExitThread(HINSTANCE hLibModule, DWORD dwExitCode)
{
    FreeLibrary(hLibModule);
    ExitThread(dwExitCode);
}


/***********************************************************************
 * Wow64GetThreadSelectorEntry [KERNEL32.@]
 */
BOOL WINAPI Wow64GetThreadSelectorEntry( HANDLE thread, DWORD selector, WOW64_LDT_ENTRY *selector_entry)
{
    FIXME("(%p %lu %p): stub\n", thread, selector, selector_entry);
    return set_ntstatus( STATUS_NOT_IMPLEMENTED );
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
