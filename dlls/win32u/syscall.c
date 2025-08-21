/*
 * Unix interface for Win32 syscalls
 *
 * Copyright (C) 2021 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "ntuser.h"
#include "wine/unixlib.h"
#include "win32syscalls.h"

ULONG_PTR zero_bits = 0;

static void stub_syscall( const char *name )
{
    CONTEXT context = { .ContextFlags = CONTEXT_FULL };
    EXCEPTION_RECORD rec =
    {
        .ExceptionCode = EXCEPTION_WINE_STUB,
        .ExceptionFlags = EXCEPTION_NONCONTINUABLE,
        .NumberParameters = 2,
        .ExceptionInformation[0] = (ULONG_PTR)"win32u",
        .ExceptionInformation[1] = (ULONG_PTR)name,
    };
    NtGetContextThread( GetCurrentThread(), &context );
#ifdef __i386__
    rec.ExceptionAddress = (void *)context.Eip;
#elif defined __x86_64__
    rec.ExceptionAddress = (void *)context.Rip;
#elif defined __arm__ || defined __aarch64__
    rec.ExceptionAddress = (void *)context.Pc;
#endif
    NtRaiseException( &rec, &context, TRUE );
}

#define SYSCALL_STUB(name) static void name(void) { stub_syscall( #name ); }
ALL_SYSCALL_STUBS

static ULONG_PTR syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) (ULONG_PTR)name,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static BYTE arguments[ARRAY_SIZE(syscalls)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static const char *syscall_names[] =
{
#define SYSCALL_ENTRY(id,name,args) #name,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static const char *usercall_names[NtUserCallCount] =
{
#define USER32_CALLBACK_ENTRY(name) "NtUser" #name,
    ALL_USER32_CALLBACKS
#undef USER32_CALLBACK_ENTRY
};

static NTSTATUS init( void *args )
{
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset)
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation(SystemEmulationBasicInformation, &info, sizeof(info), NULL);
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
#endif
    KeAddSystemServiceTable( syscalls, NULL, ARRAY_SIZE(syscalls), arguments, 1 );
    ntdll_add_syscall_debug_info( 1, syscall_names, usercall_names );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init,
};
