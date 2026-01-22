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
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(syscall);

/* Clang on x86-64, and on macOS ARM64, assumes that function parameters are extended by the caller,
 * contrary to the ABI standards */
#if (defined(__x86_64__) && defined(__clang__)) || (defined(__aarch64__) && defined(__APPLE__))

#ifdef __x86_64__

#define WRAP_FUNC(name,code) \
    extern NTSTATUS wrap_##name(void); \
    __ASM_GLOBAL_FUNC( wrap_##name, code "jmp " __ASM_NAME(#name) )
#define ARG1 "movzbl %dil,%edi\n\t"
#define ARG2 "movzbl %sil,%esi\n\t"
#define ARG3 "movzbl %dl,%edx\n\t"
#define ARG4 "movzbl %cl,%ecx\n\t"
#define ARG5 "movzbl %r8b,%r8d\n\t"
#define ARG6 "movzbl %r9b,%r9d\n\t"
#define ARG7 ARGn(7)
#define ARG8 ARGn(8)
#define ARGn(n) "andq $0xff,(" #n "-6)*8(%rsp)\n\t"

WRAP_FUNC( NtAccessCheckByTypeAndAuditAlarm, ARGn(13) )
WRAP_FUNC( NtLockFile, ARGn(9) ARGn(10) )
WRAP_FUNC( NtNotifyChangeDirectoryFile, ARGn(9) )
WRAP_FUNC( NtNotifyChangeKey, ARG7 ARGn(10) )
WRAP_FUNC( NtNotifyChangeMultipleKeys, ARGn(9) ARGn(12) )
WRAP_FUNC( NtQueryDirectoryFile, ARGn(9) ARGn(11) )
WRAP_FUNC( NtQueryEaFile, ARG5 ARGn(9) )

#define NtAccessCheckByTypeAndAuditAlarm wrap_NtAccessCheckByTypeAndAuditAlarm
#define NtLockFile wrap_NtLockFile
#define NtNotifyChangeDirectoryFile wrap_NtNotifyChangeDirectoryFile
#define NtNotifyChangeKey wrap_NtNotifyChangeKey
#define NtNotifyChangeMultipleKeys wrap_NtNotifyChangeMultipleKeys
#define NtQueryDirectoryFile wrap_NtQueryDirectoryFile
#define NtQueryEaFile wrap_NtQueryEaFile

#else

#define WRAP_FUNC(name,code) \
    extern NTSTATUS wrap_##name(void); \
    __ASM_GLOBAL_FUNC( wrap_##name, code "b " __ASM_NAME(#name) )
#define ARG1 "and w0, w0, #0xff\n\t"
#define ARG2 "and w1, w1, #0xff\n\t"
#define ARG3 "and w2, w2, #0xff\n\t"
#define ARG4 "and w3, w3, #0xff\n\t"
#define ARG5 "and w4, w4, #0xff\n\t"
#define ARG6 "and w5, w5, #0xff\n\t"
#define ARG7 "and w6, w6, #0xff\n\t"
#define ARG8 "and w7, w7, #0xff\n\t"

/* Additionally, macOS doesn't follow the standard ABI for stack packing:
 * types smaller that 64-bit are packed on the stack. The first 8 args are
 * in registers, so it only affects a few functions.
 */

WRAP_FUNC( NtCreateNamedPipeFile, /* ULONG, ULONG, ULONG, ULONG, ULONG, LARGE_INTEGER* */
           "ldr w8, [sp, #0x08]\n\t"
           "ldp x9, x10, [sp, #0x10]\n\t"
           "ldp x11, x12, [sp, #0x20]\n\t"
           "str w8, [sp, #0x04]\n\t"
           "stp w9, w10, [sp, #0x8]\n\t"
           "stp x11, x12, [sp, #0x10]\n\t" )
WRAP_FUNC( NtLockFile, /* BOOLEAN, BOOLEAN */
           "ldrb w8, [sp, #8]\n\t"
           "strb w8, [sp, #1]\n\t" )
WRAP_FUNC( NtMapViewOfSection, /* ULONG, ULONG */
           "ldr w8, [sp, #8]\n\t"
           "str w8, [sp, #4]\n\t" )
WRAP_FUNC( NtNotifyChangeKey, /* ULONG, BOOLEAN */
           ARG7
           "ldrb w8, [sp, #8]\n\t"
           "strb w8, [sp, #4]\n\t" )
WRAP_FUNC( NtNotifyChangeMultipleKeys, /* BOOLEAN, void*, ULONG, BOOLEAN */
           "ldrb w8, [sp, #0x18]\n\t"
           "strb w8, [sp, #0x14]\n\t" )
WRAP_FUNC( NtQueryEaFile, ARG5 )

#define NtCreateNamedPipeFile wrap_NtCreateNamedPipeFile
#define NtLockFile wrap_NtLockFile
#define NtMapViewOfSection wrap_NtMapViewOfSection
#define NtNotifyChangeKey wrap_NtNotifyChangeKey
#define NtNotifyChangeMultipleKeys wrap_NtNotifyChangeMultipleKeys
#define NtQueryEaFile wrap_NtQueryEaFile

#endif

WRAP_FUNC( NtAcceptConnectPort, ARG4 )
WRAP_FUNC( NtAccessCheckAndAuditAlarm, ARG8 )
WRAP_FUNC( NtAdjustGroupsToken, ARG2 )
WRAP_FUNC( NtAdjustPrivilegesToken, ARG2 )
WRAP_FUNC( NtCloseObjectAuditAlarm, ARG3 )
WRAP_FUNC( NtCommitTransaction, ARG2 )
WRAP_FUNC( NtContinue, ARG2 )
WRAP_FUNC( NtCreateEvent, ARG5 )
WRAP_FUNC( NtCreateMutant, ARG4 )
/* WRAP_FUNC( NtCreateProcess, ARG5 ) */
WRAP_FUNC( NtCreateThread, ARG8 )
WRAP_FUNC( NtDelayExecution, ARG1 )
WRAP_FUNC( NtDuplicateToken, ARG4 )
WRAP_FUNC( NtInitiatePowerAction, ARG4 )
WRAP_FUNC( NtOpenThreadToken, ARG3 )
WRAP_FUNC( NtOpenThreadTokenEx, ARG3 )
/* WRAP_FUNC( NtPrivilegeObjectAuditAlarm, ARG6 ) */
/* WRAP_FUNC( NtPrivilegedServiceAuditAlarm, ARG5 ) */
WRAP_FUNC( NtQueryDefaultLocale, ARG1 )
WRAP_FUNC( NtQueryDirectoryObject, ARG4 ARG5 )
WRAP_FUNC( NtReleaseKeyedEvent, ARG3 )
WRAP_FUNC( NtRemoveIoCompletionEx, ARG6 )
WRAP_FUNC( NtRollbackTransaction, ARG2 )
WRAP_FUNC( NtSetDebugFilterState, ARG3 )
WRAP_FUNC( NtSetDefaultLocale, ARG1 )
WRAP_FUNC( NtSetTimer, ARG5 )
WRAP_FUNC( NtSetTimerResolution, ARG2 )
WRAP_FUNC( NtSignalAndWaitForSingleObject, ARG3 )
WRAP_FUNC( NtWaitForDebugEvent, ARG2 )
WRAP_FUNC( NtWaitForKeyedEvent, ARG3 )
WRAP_FUNC( NtWaitForMultipleObjects, ARG4 )
WRAP_FUNC( NtWaitForSingleObject, ARG2 )

#define NtAcceptConnectPort wrap_NtAcceptConnectPort
#define NtAccessCheckAndAuditAlarm wrap_NtAccessCheckAndAuditAlarm
#define NtAdjustGroupsToken wrap_NtAdjustGroupsToken
#define NtAdjustPrivilegesToken wrap_NtAdjustPrivilegesToken
#define NtCloseObjectAuditAlarm wrap_NtCloseObjectAuditAlarm
#define NtCommitTransaction wrap_NtCommitTransaction
#define NtContinue wrap_NtContinue
#define NtCreateEvent wrap_NtCreateEvent
#define NtCreateMutant wrap_NtCreateMutant
#define NtCreateProcess wrap_NtCreateProcess
#define NtCreateThread wrap_NtCreateThread
#define NtDelayExecution wrap_NtDelayExecution
#define NtDuplicateToken wrap_NtDuplicateToken
#define NtInitiatePowerAction wrap_NtInitiatePowerAction
#define NtOpenThreadToken wrap_NtOpenThreadToken
#define NtOpenThreadTokenEx wrap_NtOpenThreadTokenEx
#define NtPrivilegeObjectAuditAlarm wrap_NtPrivilegeObjectAuditAlarm
#define NtPrivilegedServiceAuditAlarm wrap_NtPrivilegedServiceAuditAlarm
#define NtQueryDefaultLocale wrap_NtQueryDefaultLocale
#define NtQueryDirectoryObject wrap_NtQueryDirectoryObject
#define NtReleaseKeyedEvent wrap_NtReleaseKeyedEvent
#define NtRemoveIoCompletionEx wrap_NtRemoveIoCompletionEx
#define NtRollbackTransaction wrap_NtRollbackTransaction
#define NtSetDebugFilterState wrap_NtSetDebugFilterState
#define NtSetDefaultLocale wrap_NtSetDefaultLocale
#define NtSetTimer wrap_NtSetTimer
#define NtSetTimerResolution wrap_NtSetTimerResolution
#define NtSignalAndWaitForSingleObject wrap_NtSignalAndWaitForSingleObject
#define NtWaitForDebugEvent wrap_NtWaitForDebugEvent
#define NtWaitForKeyedEvent wrap_NtWaitForKeyedEvent
#define NtWaitForMultipleObjects wrap_NtWaitForMultipleObjects
#define NtWaitForSingleObject wrap_NtWaitForSingleObject

#undef ARG1
#undef ARG2
#undef ARG3
#undef ARG4
#undef ARG5
#undef ARG6
#undef ARG7
#undef ARG8
#undef ARGn
#undef WRAP_FUNC

#endif  /* (__x86_64__ && __clang__) || (__aarch64__ && __APPLE__) */


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
