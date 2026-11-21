/*
 * RISC-V64 NTDLL syscall bridge smoke tests.
 *
 * Copyright 2026
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
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

#if defined(__riscv) && __riscv_xlen == 64
static void test_syscall_thread_identity(void)
{
    THREAD_BASIC_INFORMATION info;
    NTSTATUS status;
    unsigned int i;

    for (i = 0; i < 32; ++i)
    {
        memset( &info, 0, sizeof(info) );
        status = NtQueryInformationThread( GetCurrentThread(), ThreadBasicInformation,
                                           &info, sizeof(info), NULL );
        ok( status == STATUS_SUCCESS, "NtQueryInformationThread returned %#lx\n", status );
        if (status != STATUS_SUCCESS) break;
        ok( HandleToULong(info.ClientId.UniqueThread) == GetCurrentThreadId(),
            "Wine thread ID changed: server %lu current %lu\n",
            HandleToULong(info.ClientId.UniqueThread), GetCurrentThreadId() );
    }
}

static void test_syscall_process_identity(void)
{
    PROCESS_BASIC_INFORMATION info;
    NTSTATUS status;
    memset( &info, 0, sizeof(info) );
    status = NtQueryInformationProcess( GetCurrentProcess(), ProcessBasicInformation,
                                        &info, sizeof(info), NULL );
    ok( status == STATUS_SUCCESS, "NtQueryInformationProcess returned %#lx\n", status );
    if (status == STATUS_SUCCESS)
        ok( info.UniqueProcessId == (ULONG_PTR)GetCurrentProcessId(),
            "Wine process ID mismatch: %Iu vs %lu\n",
            info.UniqueProcessId, GetCurrentProcessId() );
}
#endif

START_TEST(syscall)
{
#if defined(__riscv) && __riscv_xlen == 64
    test_syscall_thread_identity();
    test_syscall_process_identity();
#else
    win_skip( "RISC-V64 native syscall bridge tests are architecture-specific\n" );
#endif
}
