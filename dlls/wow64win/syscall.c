/*
 * WoW64 syscall wrapping
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
#include "rtlsupportapi.h"
#include "wow64win_private.h"

static void DECLSPEC_NORETURN stub_syscall( const char *name )
{
    EXCEPTION_RECORD record;

    record.ExceptionCode    = EXCEPTION_WINE_STUB;
    record.ExceptionFlags   = EXCEPTION_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = stub_syscall;
    record.NumberParameters = 2;
    record.ExceptionInformation[0] = (ULONG_PTR)"win32u";
    record.ExceptionInformation[1] = (ULONG_PTR)name;
    for (;;) RtlRaiseException( &record );
}

#define SYSCALL_STUB(name) NTSTATUS WINAPI wow64_ ## name( UINT *args ) { stub_syscall( #name ); }
ALL_SYSCALL_STUBS

static void * const win32_syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) wow64_ ## name,
    ALL_SYSCALLS32
#undef SYSCALL_ENTRY
};

static BYTE arguments[ARRAY_SIZE(win32_syscalls)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
    ALL_SYSCALLS32
#undef SYSCALL_ENTRY
};

const SYSTEM_SERVICE_TABLE sdwhwin32 =
{
    (ULONG_PTR *)win32_syscalls,
    NULL,
    ARRAY_SIZE(win32_syscalls),
    arguments
};


BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, void *reserved )
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    LdrDisableThreadCalloutsForDll( inst );
    NtCurrentTeb()->Peb->KernelCallbackTable = user_callbacks;
    return TRUE;
}
