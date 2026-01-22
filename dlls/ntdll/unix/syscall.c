/*
 * Unix interface for NT system calls
 *
 * Copyright 2025, 2026 Alexandre Julliard
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winbase.h"
#include "winternl.h"
#include "unix_private.h"
#include "ntsyscalls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(syscall);

static void stub_syscall( const char *name )
{
    CONTEXT context = { .ContextFlags = CONTEXT_FULL };
    EXCEPTION_RECORD rec =
    {
        .ExceptionCode = EXCEPTION_WINE_STUB,
        .ExceptionFlags = EXCEPTION_NONCONTINUABLE,
        .NumberParameters = 2,
        .ExceptionInformation[0] = (ULONG_PTR)"ntdll",
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

static void * const syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) name,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static BYTE syscall_args[ARRAY_SIZE(syscalls)] =
{
#define SYSCALL_ENTRY(id,name,args) args,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

SYSTEM_SERVICE_TABLE KeServiceDescriptorTable[4] =
{
    { (ULONG_PTR *)syscalls, NULL, ARRAY_SIZE(syscalls), syscall_args }
};

static const char *ntsyscall_names[] =
{
#define SYSCALL_ENTRY(id,name,args) #name,
    ALL_SYSCALLS
#undef SYSCALL_ENTRY
};

static const char **syscall_names[4] = { ntsyscall_names };
static const char **usercall_names;

void ntdll_add_syscall_debug_info( UINT idx, const char **names, const char **user_names )
{
    syscall_names[idx] = names;
    usercall_names = user_names;
}

BOOLEAN KeAddSystemServiceTable( ULONG_PTR *funcs, ULONG_PTR *counters, ULONG limit,
                                 BYTE *arguments, ULONG index )
{
    if (index >= ARRAY_SIZE(KeServiceDescriptorTable)) return FALSE;
    KeServiceDescriptorTable[index].ServiceTable  = funcs;
    KeServiceDescriptorTable[index].CounterTable  = counters;
    KeServiceDescriptorTable[index].ServiceLimit  = limit;
    KeServiceDescriptorTable[index].ArgumentTable = arguments;
    return TRUE;
}

void trace_syscall( UINT id, ULONG_PTR *args, ULONG len )
{
    UINT idx = (id >> 12) & 3, num = id & 0xfff;
    const char **names = syscall_names[idx];

    if (names && names[num])
        TRACE( "\1SysCall  %s(", names[num] );
    else
        TRACE( "\1SysCall  %04x(", id );

    len /= sizeof(ULONG_PTR);
    for (ULONG i = 0; i < len; i++)
    {
        TRACE( "%08lx", args[i] );
        if (i < len - 1) TRACE( "," );
    }
    TRACE( ")\n" );
}

void trace_sysret( UINT id, ULONG_PTR retval )
{
    UINT idx = (id >> 12) & 3, num = id & 0xfff;
    const char **names = syscall_names[idx];

    if (names && names[num])
        TRACE( "\1SysRet   %s() retval=%08lx\n", names[num], retval );
    else
        TRACE( "\1SysRet   %04x() retval=%08lx\n", id, retval );
}

void trace_usercall( UINT id, ULONG_PTR *args, ULONG len )
{
    if (usercall_names)
        TRACE("\1UserCall %s(%p,%u)\n", usercall_names[id], args, len );
    else
        TRACE("\1UserCall %04x(%p,%u)\n", id, args, len );
}

void trace_userret( void *ret_ptr, ULONG len, NTSTATUS status, UINT id )
{
    if (usercall_names)
        TRACE("\1UserRet  %s(%p,%u) retval=%08x\n", usercall_names[id], ret_ptr, len, status );
    else
        TRACE("\1UserRet  %04x(%p,%u) retval=%08x\n", id, ret_ptr, len, status );
}
