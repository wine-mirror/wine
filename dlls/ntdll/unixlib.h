/*
 * Ntdll Unix interface
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#ifndef __NTDLL_UNIXLIB_H
#define __NTDLL_UNIXLIB_H

#include "wine/debug.h"

struct _DISPATCHER_CONTEXT;

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 127

struct unix_funcs
{
    /* Nt* functions */
#ifdef __aarch64__
    TEB *         (WINAPI *NtCurrentTeb)(void);
#endif

    /* other Win32 API functions */
    NTSTATUS      (WINAPI *DbgUiIssueRemoteBreakin)( HANDLE process );
    LONGLONG      (WINAPI *RtlGetSystemTimePrecise)(void);
    NTSTATUS      (WINAPI *RtlWaitOnAddress)( const void *addr, const void *cmp, SIZE_T size,
                                              const LARGE_INTEGER *timeout );
    void          (WINAPI *RtlWakeAddressAll)( const void *addr );
    void          (WINAPI *RtlWakeAddressSingle)( const void *addr );

    /* fast locks */
    NTSTATUS      (CDECL *fast_RtlpWaitForCriticalSection)( RTL_CRITICAL_SECTION *crit, int timeout );
    NTSTATUS      (CDECL *fast_RtlpUnWaitCriticalSection)( RTL_CRITICAL_SECTION *crit );
    NTSTATUS      (CDECL *fast_RtlDeleteCriticalSection)( RTL_CRITICAL_SECTION *crit );
    NTSTATUS      (CDECL *fast_RtlTryAcquireSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlAcquireSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlTryAcquireSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlAcquireSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlReleaseSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlReleaseSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlWakeConditionVariable)( RTL_CONDITION_VARIABLE *variable, int count );
    NTSTATUS      (CDECL *fast_wait_cv)( RTL_CONDITION_VARIABLE *variable, const void *value,
                                         const LARGE_INTEGER *timeout );

    /* loader functions */
    NTSTATUS      (CDECL *load_so_dll)( UNICODE_STRING *nt_name, void **module );
    void          (CDECL *init_builtin_dll)( void *module );
    NTSTATUS      (CDECL *init_unix_lib)( void *module, DWORD reason, const void *ptr_in, void *ptr_out );
    NTSTATUS      (CDECL *unwind_builtin_dll)( ULONG type, struct _DISPATCHER_CONTEXT *dispatch,
                                               CONTEXT *context );
};

#endif /* __NTDLL_UNIXLIB_H */
