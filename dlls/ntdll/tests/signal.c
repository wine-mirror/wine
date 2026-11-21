/*
 * RISC-V64 signal and exception transition smoke tests.
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
#include "winnt.h"
#include "winternl.h"
#include "wine/test.h"

#if defined(__riscv) && __riscv_xlen == 64
static LONG WINAPI exception_handler( EXCEPTION_POINTERS *exception )
{
    if (exception->ExceptionRecord->ExceptionCode == 0xe0424242)
        return EXCEPTION_CONTINUE_EXECUTION;
    return EXCEPTION_CONTINUE_SEARCH;
}

static void test_signal_exception(void)
{
    PVOID handler = AddVectoredExceptionHandler( 1, exception_handler );
    unsigned int i;

    ok( !!handler, "failed to install vectored exception handler\n" );
    if (!handler) return;
    for (i = 0; i < 8; ++i)
        RaiseException( 0xe0424242, 0, 0, NULL );
    ok( RemoveVectoredExceptionHandler( handler ), "failed to remove handler\n" );
}

static void test_capture_context(void)
{
    CONTEXT context;
    memset( &context, 0, sizeof(context) );
    RtlCaptureContext( &context );
    ok( context.Pc != 0, "RtlCaptureContext returned null PC\n" );
    ok( context.Gpr.X[2] != 0, "RtlCaptureContext returned null SP\n" );
    ok( !(context.Gpr.X[2] & 15), "unaligned SP %#I64x\n", context.Gpr.X[2] );
}
#endif

START_TEST(signal)
{
#if defined(__riscv) && __riscv_xlen == 64
    test_capture_context();
    test_signal_exception();
#else
    win_skip( "RISC-V64 native signal transition tests are architecture-specific\n" );
#endif
}
