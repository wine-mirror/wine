/*
 * Unit test suite for ntdll exceptions
 *
 * Copyright 2005 Alexandre Julliard
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

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x500 /* For NTSTATUS */
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winternl.h"
#include "excpt.h"
#include "wine/test.h"

#ifdef __i386__

static struct _TEB * (WINAPI *pNtCurrentTeb)(void);
static NTSTATUS  (WINAPI *pNtGetContextThread)(HANDLE,CONTEXT*);

/* Test various instruction combinations that cause a protection fault on the i386,
 * and check what the resulting exception looks like.
 */

static const struct exception
{
    BYTE     code[18];   /* asm code */
    BYTE     offset;     /* offset of faulting instruction */
    BYTE     length;     /* length of faulting instruction */
    NTSTATUS status;     /* expected status code */
    DWORD    nb_params;  /* expected number of parameters */
    DWORD    params[4];  /* expected parameters */
} exceptions[] =
{
    /* test some privileged instructions */
    { { 0xfb, 0xc3 },  /* sti; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6c, 0xc3 },  /* insb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6d, 0xc3 },  /* insl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6e, 0xc3 },  /* outsb (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x6f, 0xc3 },  /* outsl (%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe4, 0x11, 0xc3 },  /* inb $0x11,%al; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe5, 0x11, 0xc3 },  /* inl $0x11,%eax; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe6, 0x11, 0xc3 },  /* outb %al,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xe7, 0x11, 0xc3 },  /* outl %eax,$0x11; ret */
      0, 2, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xed, 0xc3 },  /* inl (%dx),%eax; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xee, 0xc3 },  /* outb %al,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xef, 0xc3 },  /* outl %eax,(%dx); ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xf4, 0xc3 },  /* hlt; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0xfa, 0xc3 },  /* cli; ret */
      0, 1, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test long jump to invalid selector */
    { { 0xea, 0, 0, 0, 0, 0, 0, 0xc3 },  /* ljmp $0,$0; ret */
      0, 7, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test iret to invalid selector */
    { { 0x6a, 0x00, 0x6a, 0x00, 0x6a, 0x00, 0xcf, 0x83, 0xc4, 0x0c, 0xc3 },
      /* pushl $0; pushl $0; pushl $0; iret; addl $12,%esp; ret */
      6, 1, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test loading an invalid selector */
    { { 0xb8, 0xef, 0xbe, 0x00, 0x00, 0x8e, 0xe8, 0xc3 },  /* mov $beef,%ax; mov %ax,%gs; ret */
      5, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xbee8 } },

    /* test accessing a zero selector */
    { { 0x06, 0x31, 0xc0, 0x8e, 0xc0, 0x26, 0xa1, 0, 0, 0, 0, 0x07, 0xc3 },
          /* push %es; xor %eax,%eax; mov %ax,%es; mov %es:(0),%ax; pop %es */
      5, 6, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moving %cs -> %ss */
    { { 0x0e, 0x17, 0x58, 0xc3 },  /* pushl %cs; popl %ss; popl %eax; ret */
      1, 1, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test overlong instruction (limit is 16 bytes) */
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 16, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0xfa,0xc3 },
      0, 15, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test invalid interrupt */
    { { 0xcd, 0xff, 0xc3 },   /* int $0xff; ret */
      0, 2, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test moves to/from Crx */
    { { 0x0f, 0x20, 0xc0, 0xc3 },  /* movl %cr0,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x20, 0xe0, 0xc3 },  /* movl %cr4,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x22, 0xc0, 0xc3 },  /* movl %eax,%cr0; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x22, 0xe0, 0xc3 },  /* movl %eax,%cr4; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test moves to/from Drx */
    { { 0x0f, 0x21, 0xc0, 0xc3 },  /* movl %dr0,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xc8, 0xc3 },  /* movl %dr1,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x21, 0xf8, 0xc3 },  /* movl %dr7,%eax; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc0, 0xc3 },  /* movl %eax,%dr0; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xc8, 0xc3 },  /* movl %eax,%dr1; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },
    { { 0x0f, 0x23, 0xf8, 0xc3 },  /* movl %eax,%dr7; ret */
      0, 3, STATUS_PRIVILEGED_INSTRUCTION, 0 },

    /* test memory reads */
    { { 0xa1, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffc,%eax; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xfffffffc } },
    { { 0xa1, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffd,%eax; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0xa1, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xfffffffe,%eax; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0xa1, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl 0xffffffff,%eax; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },

    /* test memory writes */
    { { 0xa3, 0xfc, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffc; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 1, 0xfffffffc } },
    { { 0xa3, 0xfd, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffd; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0xa3, 0xfe, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xfffffffe; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
    { { 0xa3, 0xff, 0xff, 0xff, 0xff, 0xc3 },  /* movl %eax,0xffffffff; ret */
      0, 5, STATUS_ACCESS_VIOLATION, 2, { 0, 0xffffffff } },
};

static int got_exception;

static DWORD handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    const struct exception *except = *(const struct exception **)(frame + 1);
    unsigned int i, entry = except - exceptions;

    got_exception++;
    trace( "exception: %lx flags:%lx addr:%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress );

    ok( rec->ExceptionCode == except->status,
        "%u: Wrong exception code %lx/%lx\n", entry, rec->ExceptionCode, except->status );
    ok( rec->ExceptionAddress == except->code + except->offset,
        "%u: Wrong exception address %p/%p\n", entry,
        rec->ExceptionAddress, except->code + except->offset );

    ok( rec->NumberParameters == except->nb_params,
        "%u: Wrong number of parameters %lu/%lu\n", entry, rec->NumberParameters, except->nb_params );
    for (i = 0; i < rec->NumberParameters; i++)
        ok( rec->ExceptionInformation[i] == except->params[i],
            "%u: Wrong parameter %d: %lx/%lx\n",
            entry, i, rec->ExceptionInformation[i], except->params[i] );

    /* don't handle exception if it's not the address we expected */
    if (rec->ExceptionAddress != except->code + except->offset) return ExceptionContinueSearch;

    context->Eip += except->length;
    return ExceptionContinueExecution;
}

static void test_prot_fault(void)
{
    unsigned int i;
    struct
    {
        EXCEPTION_REGISTRATION_RECORD frame;
        const struct exception       *except;
    } exc_frame;

    pNtCurrentTeb = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "NtCurrentTeb" );
    if (!pNtCurrentTeb)
    {
        trace( "NtCurrentTeb not found, skipping tests\n" );
        return;
    }

    exc_frame.frame.Handler = handler;
    exc_frame.frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    for (i = 0; i < sizeof(exceptions)/sizeof(exceptions[0]); i++)
    {
        void (*func)(void) = (void *)exceptions[i].code;
        exc_frame.except = &exceptions[i];
        got_exception = 0;
        func();
        if (!i && !got_exception)
        {
            trace( "No exception, assuming win9x, no point in testing further\n" );
            break;
        }
        ok( got_exception == (exceptions[i].status != 0),
            "%u: bad exception count %d\n", i, got_exception );
    }
    pNtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;
}

static DWORD dreg_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                      CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    context->Eip += 2;	/* Skips the popl (%eax) */
    context->Dr0 = 0x42424242;
    context->Dr1 = 0;
    context->Dr2 = 0;
    context->Dr3 = 0;
    context->Dr6 = 0;
    context->Dr7 = 0x155;
    return ExceptionContinueExecution;
}

static BYTE code[5] = {
	0x31, 0xc0, /* xor    %eax,%eax */
	0x8f, 0x00, /* popl   (%eax) - cause exception */
        0xc3        /* ret */
};

static void test_debug_regs(void)
{
    CONTEXT ctx;
    NTSTATUS res;
    struct
    {
        EXCEPTION_REGISTRATION_RECORD frame;
    } exc_frame;
    void (*func)(void) = (void*)code;

    pNtCurrentTeb = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "NtCurrentTeb" );
    if (!pNtCurrentTeb)
    {
        trace( "NtCurrentTeb not found, skipping tests\n" );
        return;
    }
    pNtGetContextThread = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "NtGetContextThread" );
    if (!pNtGetContextThread)
    {
        trace( "NtGetContextThread not found, skipping tests\n" );
        return;
    }

    exc_frame.frame.Handler = dreg_handler;
    exc_frame.frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    func();
    pNtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    res = pNtGetContextThread(GetCurrentThread(), &ctx);
    ok (res == STATUS_SUCCESS,"NtGetContextThread failed with %lx\n", res);
    ok(ctx.Dr0 == 0x42424242,"failed to set debugregister 0 to 0x42424242, got %lx\n", ctx.Dr0);
    ok(ctx.Dr7 == 0x155,"failed to set debugregister 7 to 0x155, got %lx\n", ctx.Dr7);
}

/* test the single step exception behaviour */
static int gotsinglesteps = 0;
static DWORD single_step_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    gotsinglesteps++;
    ok (!(context->EFlags & 0x100), "eflags has single stepping bit set\n");
    return ExceptionContinueExecution;
}

static BYTE single_stepcode[] = {
    0x9c,		/* pushf */
    0x58,		/* pop   %eax */
    0x0d,0,1,0,0,	/* or    $0x100,%eax */
    0x50,		/* push   %eax */
    0x9d,		/* popf    */
    0x35,0,1,0,0,	/* xor    $0x100,%eax */
    0x50,		/* push   %eax */
    0x9d,		/* popf    */
    0xc3
};

static void test_single_step(void)
{
    struct {
        EXCEPTION_REGISTRATION_RECORD frame;
    } exc_frame;
    void (*func)(void) = (void *)single_stepcode;

    pNtCurrentTeb = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "NtCurrentTeb" );
    if (!pNtCurrentTeb) {
        trace( "NtCurrentTeb not found, skipping tests\n" );
        return;
    }
    exc_frame.frame.Handler = single_step_handler;
    exc_frame.frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;
    func();
    ok(gotsinglesteps == 1, "expected 1 single step exceptions, got %d\n", gotsinglesteps);
    pNtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;
}

/* Test the alignment check (AC) flag handling. */
static BYTE align_check_code[] = {
    0x55,                  	/* push   %ebp */
    0x89,0xe5,             	/* mov    %esp,%ebp */
    0x9c,                  	/* pushf   */
    0x58,                  	/* pop    %eax */
    0x0d,0,0,4,0,       	/* or     $0x40000,%eax */
    0x50,                  	/* push   %eax */
    0x9d,                  	/* popf    */
    0x8b,0x45,8,           	/* mov    0x8(%ebp),%eax */
    0xc7,0x40,1,42,0,0,0, 	/* movl   $42,0x1(%eax) */
    0x9c,                  	/* pushf   */
    0x58,                  	/* pop    %eax */
    0x35,0,0,4,0,       	/* xor    $0x40000,%eax */
    0x50,                  	/* push   %eax */
    0x9d,                  	/* popf    */
    0x5d,                  	/* pop    %ebp */
    0xc3,                  	/* ret     */
};

static int gotalignfaults = 0;
static DWORD align_check_handler( EXCEPTION_RECORD *rec, EXCEPTION_REGISTRATION_RECORD *frame,
                                  CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **dispatcher )
{
    ok (!(context->EFlags & 0x40000), "eflags has AC bit set\n");
    gotalignfaults++;
    return ExceptionContinueExecution;
}

static void test_align_faults(void)
{
    int twoints[2];
    struct {
        EXCEPTION_REGISTRATION_RECORD frame;
    } exc_frame;
    void (*func1)(int*) = (void *)align_check_code;

    pNtCurrentTeb = (void *)GetProcAddress( GetModuleHandleA("ntdll.dll"), "NtCurrentTeb" );
    if (!pNtCurrentTeb) {
        trace( "NtCurrentTeb not found, skipping tests\n" );
        return;
    }
    exc_frame.frame.Handler = align_check_handler;
    exc_frame.frame.Prev = pNtCurrentTeb()->Tib.ExceptionList;
    pNtCurrentTeb()->Tib.ExceptionList = &exc_frame.frame;

    gotalignfaults=0;
    func1(twoints);
    ok(gotalignfaults == 0, "got %d alignment faults, expected 0\n", gotalignfaults);
    pNtCurrentTeb()->Tib.ExceptionList = exc_frame.frame.Prev;
}

#endif  /* __i386__ */

START_TEST(exception)
{
#ifdef __i386__
    test_prot_fault();
    test_debug_regs();
    test_single_step();
    test_align_faults();
#endif
}
